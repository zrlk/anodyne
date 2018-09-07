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

#include "anodyne/tools/tt_parser.h"
#include <assert.h>

namespace anodyne {

bool TtParser::Unescape(const char* yytext, std::string* out) {
  if (out == nullptr || *yytext != '\"') {
    return false;
  }
  ++yytext;  // Skip initial ".
  out->clear();
  char current = *yytext++;  // yytext will always immediately follow `current`.
  for (; current != '\0' && current != '\"'; current = *yytext++) {
    if (current == '\\') {
      current = *yytext++;
      switch (current) {
        case '\"':
          out->push_back(current);
          break;
        case '\\':
          out->push_back(current);
          break;
        case 'n':
          out->push_back('\n');
          break;
        default:
          return false;
      }
    } else {
      out->push_back(current);
    }
  }
  return (current == '\"' && *yytext == '\0');
}

bool TtParser::ParseFile(const File* file, bool needs_cleaned) {
  had_errors_ = false;
  file_ = file;
  initial_location_ = Range{file->begin(), file->begin()};
  ScanBeginString(file->Text(file->begin(), file->end()), trace_lex_,
                  needs_cleaned);
  yy::TtParserImpl parser(*this);
  parser.set_debug_level(trace_parse_);
  int result = parser.parse();
  ScanEnd();
  file_ = nullptr;
  return result == 0 && !had_errors_;
}

void TtParser::Error(const Range& location, const std::string& message) {
  // TODO: replace with a PrettyPrinter
  auto line_col = file_->contents().Utf8LineColForOffset(location.begin.data() -
                                                         file_->begin().data());
  std::cerr << "When trying " << file_->id().ToString() << " ("
            << line_col.first + 1 << ":" << line_col.second + 1
            << ") : " << message << std::endl;
  had_errors_ = true;
}

void TtParser::PushPatternCtorOrVariable(const Range& loc,
                                         const std::string& ident) {
  auto id = std::unique_ptr<TtPat>(new TtPat());
  id->kind = TtPat::Kind::kVariable;
  id->ident = ident;
  id->loc = loc;
  pat_stack_.emplace_back(std::move(id));
}

void TtParser::ApplyCtorPattern(const Range& loc, const std::string& ident,
                                size_t pat_count) {
  assert(pat_stack_.size() >= pat_count);
  auto pat = std::unique_ptr<TtPat>(new TtPat());
  pat->kind = TtPat::Kind::kCtorApp;
  pat->ident = ident;
  pat->loc = loc;
  for (size_t i = 0; i < pat_count; ++i) {
    pat->children.emplace_back(
        std::move(pat_stack_[pat_stack_.size() - pat_count + i]));
  }
  pat_stack_.resize(pat_stack_.size() - pat_count);
  pat_stack_.emplace_back(std::move(pat));
}

void TtParser::ApplyListPattern(const Range& loc, size_t pat_count) {
  assert(pat_stack_.size() >= pat_count);
  auto pat = std::unique_ptr<TtPat>(new TtPat());
  pat->kind = TtPat::Kind::kList;
  pat->loc = loc;
  for (size_t i = 0; i < pat_count; ++i) {
    pat->children.emplace_back(
        std::move(pat_stack_[pat_stack_.size() - pat_count + i]));
  }
  pat_stack_.resize(pat_stack_.size() - pat_count);
  pat_stack_.emplace_back(std::move(pat));
}

void TtParser::ApplyOptionPattern(const Range& loc, bool is_some) {
  assert(!is_some || pat_stack_.size() > 0);
  auto pat = std::unique_ptr<TtPat>(new TtPat());
  pat->loc = loc;
  if (is_some) {
    pat->children.emplace_back(std::move(pat_stack_.back()));
    pat->kind = TtPat::Kind::kSome;
    pat_stack_.pop_back();
  } else {
    pat->kind = TtPat::Kind::kNone;
  }
  pat_stack_.emplace_back(std::move(pat));
}

void TtParser::ApplyMatch(const Range& loc, size_t clause_count) {
  assert(pat_stack_.size() >= clause_count);
  auto m = std::unique_ptr<TtMatch>(new TtMatch());
  m->loc = loc;
  for (size_t i = 0; i < clause_count; ++i) {
    std::unique_ptr<TtClause> clause =
        std::unique_ptr<TtClause>(new TtClause());
    clause->pat = std::move(pat_stack_[pat_stack_.size() - clause_count + i]);
    m->clauses.emplace_back(std::move(clause));
  }
  pat_stack_.resize(pat_stack_.size() - clause_count);
  matches_.emplace_back(std::move(m));
}

void TtParser::PushIdentifier(const Range& loc, const std::string& ident,
                              const std::string& label, bool is_array,
                              bool is_option, bool is_hash) {
  auto id = std::unique_ptr<TtTypeNode>(new TtTypeNode());
  id->kind = TtTypeNode::Kind::kIdentifier;
  id->ident = ident;
  id->loc = loc;
  id->label = label;
  id->is_array = is_array;
  id->is_option = is_option;
  id->is_hash = is_hash;
  type_stack_.emplace_back(std::move(id));
}

void TtParser::ApplyStar(const Range& loc) {
  assert(type_stack_.size() >= 2);
  auto rhs = std::move(type_stack_.back());
  type_stack_.pop_back();
  if (type_stack_.back()->kind == TtTypeNode::Kind::kTuple) {
    type_stack_.back()->children.push_back(std::move(rhs));
  } else {
    auto lhs = std::move(type_stack_.back());
    type_stack_.pop_back();
    auto star = std::unique_ptr<TtTypeNode>(new TtTypeNode());
    star->kind = TtTypeNode::Kind::kTuple;
    star->loc = loc;
    star->children.push_back(std::move(lhs));
    star->children.push_back(std::move(rhs));
    type_stack_.emplace_back(std::move(star));
  }
}

void TtParser::ApplyCtorDecl(const Range& loc, const std::string& ident) {
  if (ctor_to_datatype_.find(ident) != ctor_to_datatype_.end()) {
    Error(loc,
          ident + " used elsewhere as a ctor in " + ctor_to_datatype_[ident]);
  }
  next_datatype_.ctors.emplace_back();
  next_datatype_.ctors.back().ident = ident;
  next_datatype_.ctors.back().loc = loc;
  if (type_stack_.size() >= 1) {
    next_datatype_.ctors.back().type = std::move(type_stack_.back());
    type_stack_.pop_back();
  }
}

void TtParser::ApplyJsonDeclopt(const std::string& arg) {
  next_datatype_.derive_json = true;
  next_datatype_.json_arg = arg;
}

void TtParser::ApplyTypeDecl(const Range& loc, const std::string& ident) {
  assert(next_datatype_.ctors.size() >= 1);
  if (datatypes_.find(ident) != datatypes_.end()) {
    Error(loc, ident + " multiply defined");
  } else {
    for (const auto& ctor : next_datatype_.ctors) {
      ctor_to_datatype_[ctor.ident] = ident;
    }
    next_datatype_.raw_ident = ident;
    std::string qualifier;
    for (char c : ident) {
      if (c == '.') {
        next_datatype_.qualified_ident.append("::");
        next_datatype_.qualified_ident.append(qualifier);
        next_datatype_.qualifiers.push_back(qualifier);
        qualifier.clear();
      } else {
        qualifier.push_back(c);
      }
    }
    if (qualifier.empty()) {
      Error(loc, ident + " has empty unqualified name");
    }
    next_datatype_.unqualified_ident = qualifier;
    next_datatype_.qualified_ident.append("::");
    next_datatype_.qualified_ident.append(qualifier);
    datatypes_.emplace(std::make_pair(ident, std::move(next_datatype_)));
  }
  next_datatype_ = TtDatatype{};
}

void TtParser::Error(const std::string& message) {
  // TODO: replace with a PrettyPrinter
  std::cerr << "When trying " << file_->id().ToString() << ": " << message
            << std::endl;
  had_errors_ = true;
}

void TtParser::ScanBeginString(absl::string_view data, bool trace_scanning,
                               bool needs_cleaned) {
  if (!needs_cleaned) {
    SetScanBuffer(data, trace_scanning);
    return;
  }
  std::string yy_buf;
  std::vector<int> paren_level;
  enum class CleanState {
    kEntry,
    kBcplComment,
    kBlockComment,
    kPassthroughUntilEndBlockComment,
    kStringLiteral,
    kCharLiteral
  } state = CleanState::kEntry;
  for (size_t i = 0; i < data.size(); ++i) {
    char cm1 = (i < data.size() - 1 ? data[i - 1] : 'a');
    char c0 = data[i];
    char c1 = (i + 1 < data.size() ? data[i + 1] : 0);
    char c2 = (i + 2 < data.size() ? data[i + 2] : 0);
    switch (state) {
      // Replace all uninteresting text (like code or non-special comments) with
      // whitespace while breaking out pattern expressions from /*| ... */
      // comments. In addition, make sure to preserve the outermost parens for
      // match( ... ) expressions to support nested matches.
      case CleanState::kEntry:
        if (c0 == '/' && c1 == '/') {
          state = CleanState::kBcplComment;
          break;
        }
        if (c0 == '/' && c1 == '*') {
          if (c2 == '|') {
            yy_buf.append("  ");
            ++i;
            state = CleanState::kPassthroughUntilEndBlockComment;
            break;
          } else {
            state = CleanState::kBlockComment;
          }
        }
        if (!isalnum(cm1) && c0 == '_' && c1 == '_' && c2 == 'm') {
          if (data.find("__match", i) == i) {
            yy_buf.append("  match");
            i += 6;
            paren_level.push_back(0);
            break;
          }
        }
        if (c0 == '\r' || c0 == '\n') {
          yy_buf.push_back(c0);
          break;
        }
        if (c0 == '\"') {
          state = CleanState::kStringLiteral;
        }
        if (c0 == '\'') {
          state = CleanState::kCharLiteral;
        }
        if (c0 == '(') {
          if (!paren_level.empty()) {
            ++paren_level.back();
            if (paren_level.back() == 1) {
              yy_buf.push_back('(');
              break;
            }
          }
        }
        if (c0 == ')') {
          if (!paren_level.empty()) {
            --paren_level.back();
            if (paren_level.back() == 0) {
              yy_buf.push_back(')');
              paren_level.pop_back();
              break;
            }
          }
        }
        yy_buf.push_back(' ');
        break;
      // Erase a BCPL comment.
      case CleanState::kBcplComment:
        if (c0 == '\n') {
          yy_buf.push_back(c0);
          state = CleanState::kEntry;
        } else {
          yy_buf.push_back(' ');
        }
        break;
      // Erase a block comment.
      case CleanState::kBlockComment:
        if (c0 == '\n' || c0 == '\r') {
          yy_buf.push_back(c0);
        } else if (c0 == '*' && c1 == '/') {
          yy_buf.append("  ");
          state = CleanState::kEntry;
        } else {
          yy_buf.push_back(' ');
        }
        break;
      // Copy source text until the current block comment ends.
      case CleanState::kPassthroughUntilEndBlockComment:
        if (c0 == '*' && c1 == '/') {
          yy_buf.append("  ");
          ++i;
          state = CleanState::kEntry;
        } else {
          yy_buf.push_back(c0);
        }
        break;
      // Erase a string literal.
      case CleanState::kStringLiteral:
        if (c0 == '\\' && c1 == '\"') {
          yy_buf.push_back(' ');
          ++i;
        } else if (c0 == '\"') {
          state = CleanState::kEntry;
        }
        yy_buf.push_back(' ');
        break;
      // Erase a character literal.
      case CleanState::kCharLiteral:
        if (c0 == '\\' && c1 == '\'') {
          yy_buf.push_back(' ');
          ++i;
        } else if (c0 == '\'') {
          state = CleanState::kEntry;
        }
        yy_buf.push_back(' ');
        break;
    }
  }
  SetScanBuffer(yy_buf, trace_scanning);
}
}  // namespace anodyne
