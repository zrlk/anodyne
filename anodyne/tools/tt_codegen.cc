/*
 * Copyright 2018 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "anodyne/tools/tt_codegen.h"

namespace anodyne {
namespace {
/// \brief Returns a header guard for the include path `file`.
///
/// For example, `path/to/file.h` becomes `PATH_TO_FILE_H_`
std::string HeaderGuardFor(absl::string_view file) {
  std::string guard;
  guard.reserve(file.size() + 1);
  for (char c : file) {
    if (!isalnum(c)) {
      guard.push_back('_');
    } else {
      guard.push_back(toupper(c));
    }
  }
  guard.push_back('_');
  return guard;
}

/// \brief Generate code in `out` that deletes unwanted default members from
/// `name`.
void DeleteBadCtors(const std::string& name, FILE* out) {
  fprintf(out, "  %s(const %s& o) = delete;\n", name.c_str(), name.c_str());
  fprintf(out, "  %s& operator=(const %s& o) = delete;\n", name.c_str(),
          name.c_str());
}

/// \brief Returns the path to use when including `file`.
std::string LocalPathFor(absl::string_view file) { return std::string(file); }

/// \brief Automatically enters and exits the namespace specified by
/// `qualifiers`.
class Namespace {
 public:
  Namespace(const std::vector<std::string>& qualifiers, FILE* out)
      : qualifiers_(qualifiers), out_(out) {
    for (const auto& ns : qualifiers_) {
      fprintf(out_, "namespace %s {\n", ns.c_str());
    }
  }
  ~Namespace() {
    for (const auto& ns : qualifiers_) {
      fprintf(out_, "}  // namespace %s\n", ns.c_str());
    }
  }

 private:
  const std::vector<std::string>& qualifiers_;
  FILE* out_;
};
}  // anonymous namespace

bool TtGenerator::GeneratePreamble() {
  fprintf(h_, "#ifndef %s\n", HeaderGuardFor(h_relative_path_).c_str());
  fprintf(h_, "#define %s\n", HeaderGuardFor(h_relative_path_).c_str());
  fprintf(h_, "#include \"%s\"\n",
          LocalPathFor("anodyne/base/arena.h").c_str());
  fprintf(h_, "#include \"%s\"\n",
          LocalPathFor("anodyne/base/context.h").c_str());
  fprintf(h_, "#include \"%s\"\n",
          LocalPathFor("anodyne/base/symbol_table.h").c_str());
  fprintf(h_, "#include \"%s\"\n",
          LocalPathFor("anodyne/base/source.h").c_str());
  fprintf(h_, "#include \"%s\"\n",
          LocalPathFor("anodyne/base/trees.h").c_str());
  for (const auto& datatype : parser_.datatypes()) {
    if (datatype.second.derive_json) {
      fprintf(h_, "#include \"%s\"\n",
              LocalPathFor("rapidjson/document.h").c_str());
      break;
    }
  }
  fprintf(h_, "#include <tuple>\n");
  fprintf(cc_, "#include \"%s\"\n", h_relative_path_.c_str());
  return true;
}

bool TtGenerator::GeneratePostamble() {
  fprintf(h_, "#endif  // defined(%s)\n",
          HeaderGuardFor(h_relative_path_).c_str());
  return true;
}

bool TtGenerator::Generate() {
  if (!GeneratePreamble()) {
    return false;
  }
  // Forward declarations in case we have mutual recursion.
  for (const auto& datatype : parser_.datatypes()) {
    Namespace ns(datatype.second.qualifiers, h_);
    fprintf(h_, "class %s;\n", datatype.second.unqualified_ident.c_str());
    for (const auto& ctor : datatype.second.ctors) {
      fprintf(h_, "class Unboxed%s;\n", ctor.ident.c_str());
    }
  }
  // Datatype implementation.
  for (const auto& datatype : parser_.datatypes()) {
    Namespace ns(datatype.second.qualifiers, h_);
    if (!GenerateDatatypeRep(datatype.second)) {
      return false;
    }
  }
  // Constructor implementation.
  for (const auto& datatype : parser_.datatypes()) {
    Namespace ns(datatype.second.qualifiers, h_);
    for (const auto& ctor : datatype.second.ctors) {
      if (!GenerateCtorRep(datatype.second, ctor)) {
        return false;
      }
    }
  }
  if (!GeneratePostamble()) {
    return false;
  }
  return true;
}

bool TtGenerator::CompileSlowPatternAdmissibleCheck(const TtPat& pat,
                                                    const std::string& path) {
  switch (pat.kind) {
    case TtPat::Kind::kCtorApp: {
      fprintf(h_,
              "(%s->tag() == "
              "std::remove_reference<decltype(*(%s))>::type::Tag::k%s)",
              path.c_str(), path.c_str(), pat.ident.c_str());
      for (size_t c = 0; c < pat.children.size(); ++c) {
        fprintf(h_, " && (");
        if (!CompileSlowPatternAdmissibleCheck(
                *(pat.children[c]), path + "->As" + pat.ident + "()->m_" +
                                        std::to_string(c) + "_")) {
          return false;
        }
        fprintf(h_, ")");
      }
    } break;
    case TtPat::Kind::kVariable: {
      fprintf(h_, "true");
    } break;
    case TtPat::Kind::kList: {
      fprintf(h_, "(%s.size() == %lu)", path.c_str(), pat.children.size());
      for (size_t c = 0; c < pat.children.size(); ++c) {
        fprintf(h_, " && (");
        if (!CompileSlowPatternAdmissibleCheck(
                *(pat.children[c]), path + "[" + std::to_string(c) + "]")) {
          return false;
        }
        fprintf(h_, ")");
      }
    } break;
    case TtPat::Kind::kSome: {
      fprintf(h_, "(%s.is_some()) && (", path.c_str());
      if (!CompileSlowPatternAdmissibleCheck(*(pat.children[0]),
                                             path + ".get()")) {
        return false;
      }
      fprintf(h_, ")");
    } break;
    case TtPat::Kind::kNone: {
      fprintf(h_, "(!%s.is_some())", path.c_str());
    } break;
  }
  return true;
}

bool TtGenerator::CompileSlowPatternBindings(const TtPat& pat,
                                             const std::string& path) {
  switch (pat.kind) {
    case TtPat::Kind::kCtorApp: {
      for (size_t c = 0; c < pat.children.size(); ++c) {
        if (!CompileSlowPatternBindings(*(pat.children[c]),
                                        path + "->As" + pat.ident + "()->m_" +
                                            std::to_string(c) + "_")) {
          return false;
        }
      }
    } break;
    case TtPat::Kind::kVariable: {
      if (pat.ident != "_") {
        // Squelch "unused variable" noise.
        fprintf(h_, "  const auto %s = %s; (void)%s; \\\n", pat.ident.c_str(),
                path.c_str(), pat.ident.c_str());
      }
    } break;
    case TtPat::Kind::kList: {
      for (size_t c = 0; c < pat.children.size(); ++c) {
        if (!CompileSlowPatternBindings(*(pat.children[c]),
                                        path + "[" + std::to_string(c) + "]")) {
          return false;
        }
      }
    } break;
    case TtPat::Kind::kNone: {
      // nothing to be done
    } break;
    case TtPat::Kind::kSome: {
      return CompileSlowPatternBindings(*(pat.children[0]), path + ".get()");
    } break;
  }
  return true;
}

bool TtGenerator::CompileSlowPattern(const TtPat& pat,
                                     const std::string& cont) {
  fprintf(h_, " (");
  if (!CompileSlowPatternAdmissibleCheck(pat, "__disc")) {
    return false;
  }
  fprintf(h_, ") { \\\n");
  if (!CompileSlowPatternBindings(pat, "__disc")) {
    return false;
  }
  fprintf(h_, " %s } \\\n", cont.c_str());
  return true;
}

bool TtGenerator::GenerateMatchers() {
  fprintf(h_, "#include <type_traits>\n");
  for (const auto& matcher : parser_.matchers()) {
    const auto* file = source_.FindFile(matcher->loc.begin);
    if (file == nullptr) {
      return Error(matcher->loc, "Missing file.");
    }
    int ofs = file->OffsetFor(matcher->loc.begin);
    if (ofs < 0) {
      return Error(matcher->loc, "Bad offset in file.");
    }
    auto line_col = file->contents().Utf8LineColForOffset(ofs);
    fprintf(h_, "#define __match_%d(__indisc", line_col.first + 1);
    for (size_t c = 0; c < matcher->clauses.size(); ++c) {
      fprintf(h_, ", __case%lu", c);
    }
    fprintf(h_, ") \\\n");
    fprintf(h_, "  (([&](decltype(__indisc) __disc) { \\\n");
    for (size_t c = 0; c < matcher->clauses.size(); ++c) {
      if (c == 0) {
        fprintf(h_, "  if ");
      } else {
        fprintf(h_, "  else if ");
      }
      if (!CompileSlowPattern(*matcher->clauses[c]->pat,
                              "return __case" + std::to_string(c) + ";")) {
        return false;
      }
    }
    fprintf(h_, "  abort(); })(__indisc))\n");
  }
  fprintf(h_, R"(#ifndef __match
#define __match_dispatch_id(id) __match_##id
#define __match_dispatch(id, ...) __match_dispatch_id(id)(__VA_ARGS__)
#define __match(...) __match_dispatch(__LINE__, __VA_ARGS__)
#endif
)");
  return true;
}

bool TtGenerator::Error(Range range, absl::string_view message) {
  std::cerr << range.ToString(source_) << ": " << message;
  return false;
}

bool TtGenerator::GenerateDatatypeRep(const TtDatatype& datatype) {
  fprintf(h_, "class %s : public ::anodyne::ArenaObject {\n",
          datatype.unqualified_ident.c_str());
  fprintf(h_, " public:\n");
  fprintf(h_, "  enum class Tag {\n");
  for (size_t c = 0; c < datatype.ctors.size(); ++c) {
    fprintf(h_, "    k%s = %lu,\n", datatype.ctors[c].ident.c_str(), c);
  }
  fprintf(h_, "  };\n");
  fprintf(h_, "  const Tag tag() const { return tag_; }\n");
  for (const auto& ctor : datatype.ctors) {
    fprintf(h_, "  inline const Unboxed%s* As%s() const;\n", ctor.ident.c_str(),
            ctor.ident.c_str());
  }
  fprintf(h_, "  inline void Dump(absl::string_view prefix) const;\n");
  fprintf(h_, " protected:\n");
  fprintf(h_, "  %s(Tag t) : tag_(t) { }\n",
          datatype.unqualified_ident.c_str());
  DeleteBadCtors(datatype.unqualified_ident, h_);
  fprintf(h_, " private:\n");
  fprintf(h_, "  Tag tag_;\n");
  fprintf(h_, "};\n");
  return true;
}

bool TtGenerator::GenerateCtorRep(const TtDatatype& datatype,
                                  const TtConstructor& constructor) {
  fprintf(h_, "class Unboxed%s : public %s {\n", constructor.ident.c_str(),
          datatype.unqualified_ident.c_str());
  fprintf(h_, " public:\n");
  std::vector<Type> decomposed_type;
  if (!DecomposeCtorType(constructor, &decomposed_type)) {
    return false;
  }
  fprintf(h_, "  Unboxed%s(", constructor.ident.c_str());
  for (size_t i = 0; i < decomposed_type.size(); ++i) {
    if (i != 0) {
      fprintf(h_, ", ");
    }
    fprintf(h_, "%s m_%lu", decomposed_type[i].c_str(), i);
  }
  fprintf(h_, ") : %s(%s::Tag::k%s)", datatype.unqualified_ident.c_str(),
          datatype.unqualified_ident.c_str(), constructor.ident.c_str());
  for (size_t i = 0; i < decomposed_type.size(); ++i) {
    fprintf(h_, ", m_%lu_(m_%lu)", i, i);
  }
  fprintf(h_, " {}\n");
  DeleteBadCtors(("Unboxed" + constructor.ident).c_str(), h_);
  for (size_t i = 0; i < decomposed_type.size(); ++i) {
    fprintf(h_, "  %s m_%lu_;\n", decomposed_type[i].c_str(), i);
  }
  fprintf(h_, "};\n");
  fprintf(h_,
          "inline const Unboxed%s* %s::As%s() const { return tag_ == Tag::k%s "
          "? static_cast<const Unboxed%s*>(this) : nullptr; }\n",
          constructor.ident.c_str(), datatype.unqualified_ident.c_str(),
          constructor.ident.c_str(), constructor.ident.c_str(),
          constructor.ident.c_str());
  fprintf(h_, "inline const %s* %s(", datatype.unqualified_ident.c_str(),
          constructor.ident.c_str());
  for (size_t i = 0; i < decomposed_type.size(); ++i) {
    if (i != 0) {
      fprintf(h_, ", ");
    }
    fprintf(h_, "%s m_%lu", decomposed_type[i].c_str(), i);
  }
  fprintf(h_, ") {\n");
  fprintf(h_,
          "  return new (::anodyne::Context::Current()->arena()) Unboxed%s(",
          constructor.ident.c_str());
  for (size_t i = 0; i < decomposed_type.size(); ++i) {
    if (i != 0) {
      fprintf(h_, ", ");
    }
    fprintf(h_, "m_%lu", i);
  }
  fprintf(h_, ");\n}\n");
  return true;
}

bool TtGenerator::DecomposeCtorType(const TtConstructor& constructor,
                                    std::vector<Type>* type_out) {
  if (constructor.type == nullptr) {
    return true;
  }
  switch (constructor.type->kind) {
    case TtTypeNode::Kind::kTuple: {
      for (const auto& kid : constructor.type->children) {
        if (!DecomposeType(*kid, type_out)) {
          return false;
        }
      }
    } break;
    case TtTypeNode::Kind::kIdentifier: {
      if (!DecomposeIdentType(*constructor.type, type_out)) {
        return false;
      }
    } break;
  }
  return true;
}

bool TtGenerator::DecomposeType(const TtTypeNode& type,
                                std::vector<Type>* type_out) {
  switch (type.kind) {
    case TtTypeNode::Kind::kTuple: {
      for (const auto& kid : type.children) {
        if (!DecomposeType(*kid, type_out)) {
          return false;
        }
      }
    } break;
    case TtTypeNode::Kind::kIdentifier:
      return DecomposeIdentType(type, type_out);
  }
  return true;
}

bool TtGenerator::DecomposeIdentType(const TtTypeNode& type,
                                     std::vector<Type>* type_out) {
  Type append_out;
  auto dt = parser_.datatypes().find(type.ident);
  if (dt != parser_.datatypes().end()) {
    if (type.is_array) {
      if (type.is_option) {
        Error(type.loc, "array and option are not miscible");
        return false;
      }
      append_out = "::anodyne::ArenaSlice<" + dt->second.qualified_ident + ">";
    } else if (type.is_option) {
      append_out = "::anodyne::ArenaOption<" + dt->second.qualified_ident + ">";
    } else {
      append_out = "const " + dt->second.qualified_ident + "*";
    }
    type_out->push_back(append_out);
    return true;
  }
  if (type.ident == "ident") {
    if (type.is_array) {
      append_out = "::anodyne::ArenaSlice<::anodyne::Symbol>";
    } else if (type.is_option) {
      append_out = "::anodyne::ArenaOption<::anodyne::Symbol>";
    } else {
      append_out = "const ::anodyne::Symbol";
    }
    type_out->push_back(append_out);
    return true;
  }
  if (type.ident == "unit") {
    append_out = "::anodyne::Unit";
    type_out->push_back(append_out);
    return true;
  }
  if (type.ident == "range") {
    append_out = "::anodyne::Range";
    type_out->push_back(append_out);
    return true;
  }
  return Error(type.loc, type.ident + ": identifier unknown");
}

}  // namespace anodyne
