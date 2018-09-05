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

/// \brief Returns the path to use when including `file`.
std::string LocalPathFor(absl::string_view file) { return std::string(file); }
}  // anonymous namespace

bool TtGenerator::GeneratePreamble() {
  fprintf(h_, "#ifndef %s\n", HeaderGuardFor(h_relative_path_).c_str());
  fprintf(h_, "#define %s\n", HeaderGuardFor(h_relative_path_).c_str());
  fprintf(h_, "#include \"%s\"\n",
          LocalPathFor("anodyne/base/arena.h").c_str());
  fprintf(h_, "#include \"%s\"\n",
          LocalPathFor("anodyne/base/symbol_table.h").c_str());
  fprintf(h_, "#include \"%s\"\n",
          LocalPathFor("anodyne/base/source.h").c_str());
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
  // TODO: Write out definition code.
  if (!GeneratePostamble()) {
    return false;
  }
  return true;
}

bool TtGenerator::GenerateMatchers() {
  fprintf(h_, "#include <type_traits>\n");
  // TODO: Write out pattern code.
  return true;
}
}  // namespace anodyne
