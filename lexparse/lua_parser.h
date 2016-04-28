/*
 * Copyright 2016 Google Inc. All rights reserved.
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

#ifndef LEXPARSE_LUA_PARSER_H_
#define LEXPARSE_LUA_PARSER_H_

namespace yy {
class LuaParserImpl;
}

#include "lexparse/lua_bison_support.h"
#include "lexparse/lua_parse.hh"

namespace lexparse {
class LuaParser {
 public:
  /// \param trace_lex Dump lexing debug information
  /// \param trace_parse Dump parsing debug information
  LuaParser(st::Arena *arena, st::SymbolTable *symbol_table,
            bool trace_lex = false, bool trace_parse = false)
      : arena_(arena),
        symbol_table_(symbol_table),
        trace_lex_(trace_lex),
        trace_parse_(trace_parse) {
    empty_tuple_ = new (arena_) lua::Tuple(util::SourceRange(), 0, nullptr);
    primitive_error_ = CreateUnutterableVar(util::SourceRange(), "$error");
    primitive_tonumber_ =
        CreateUnutterableVar(util::SourceRange(), "$tonumber");
  }

  /// \param filename The filename of the file to load
  /// \return true if there were no errors
  bool ParseFile(const std::string &filename);

  /// \param content The content to parse and load
  /// \param fake_filename Some string to use when printing errors and locations
  /// \return true if there were no errors
  bool ParseString(const std::string &content,
                   const std::string &fake_filename);

  /// \brief The name of the current file being read. It is safe to take
  /// the address of this string (which shares the lifetime of this object.)
  std::string &file() { return file_; }

  lua::Symbol Intern(const std::string &for_text) {
    return symbol_table_->intern(for_text);
  }

  lua::Var *CreateVar(const util::SourceRange &location,
                      const std::string &for_text);
  lua::Var *CreateUnutterableVar(const util::SourceRange &location,
                                 const std::string &debug_text) {
    return CreateVar(location, "$" + debug_text);
  }
  lua::DirectIndex *CreateDirectIndex(const util::SourceRange &location,
                                      lua::Node *lhs,
                                      const std::string &for_text);
  lua::Call *CreateMemberCall(const util::SourceRange &location,
                              lua::Node *function, lua::Tuple *args,
                              const std::string &for_text);

  lua::Literal *CreateStringLiteral(const util::SourceRange &location,
                                    const std::string &for_text);
  lua::Literal *CreateNumberLiteral(const util::SourceRange &location,
                                    const std::string &for_text);

  /// \brief Unescapes a string literal (which is expected to include
  /// terminating quotes).
  /// \param yytext literal string to escape
  /// \param out pointer to a string to overwrite with `yytext` unescaped.
  /// \return true if `yytext` was a valid literal string; false otherwise.
  static bool Unescape(const char *yytext, std::string *out);

  /// \brief Called by the lexer to enter a new raw text range.
  void EnterRawText(size_t number_of_equals) {
    if (trace_lex_) {
      fprintf(stderr, "EnterRawText(%lu)\n", number_of_equals);
    }
    raw_text_is_comment_ = false;
    raw_equals_count_ = number_of_equals;
  }

  /// \brief Called by the lexer to enter a new raw comment range.
  void EnterRawComment(size_t number_of_equals) {
    if (trace_lex_) {
      fprintf(stderr, "EnterRawComment(%lu)\n", number_of_equals);
    }
    raw_text_is_comment_ = true;
    raw_equals_count_ = number_of_equals;
  }

  /// \brief Called by the lexer when it thinks it might be exiting a raw
  /// comment or text range.
  /// \pre yytext matches .*\]\=*\]
  /// \return true if we've exited the range; if so, we'll set out and
  /// was_comment.
  bool ExitRawRegion(const char *yytext, const util::SourceRange &loc,
                     bool *was_comment, std::string *out) {
    if (trace_lex_) {
      fprintf(stderr, "ExitRawRegion(%s)\n", yytext);
    }
    raw_text_.append(yytext);
    if (raw_text_.size() < 2 || raw_text_[raw_text_.size() - 1] != ']') {
      Error(loc, "Internal: raw text end marker is invalid.");
      return false;
    }
    auto next_bracket = raw_text_.rfind(']', raw_text_.size() - 2);
    if (next_bracket == std::string::npos) {
      Error(loc, "Internal: raw text end marker is invalid (no second ]).");
      return false;
    }
    size_t number_of_equals = raw_text_.size() - next_bracket - 2;
    if (number_of_equals != raw_equals_count_) {
      return false;
    }
    raw_text_.resize(raw_text_.size() - number_of_equals - 2);
    *was_comment = raw_text_is_comment_;
    *out = std::move(raw_text_);
    return true;
  }

  lua::Tuple *empty_block() const { return empty_tuple_; }
  lua::Tuple *empty_function_args() const { return empty_tuple_; }
  lua::Tuple *empty_var_inits() const { return empty_tuple_; }
  lua::Tuple *empty_fields() const { return empty_tuple_; }

  lua::Node *primitive_error() const { return primitive_error_; }
  lua::Node *primitive_tonumber() const { return primitive_tonumber_; }

 private:
  friend class yy::LuaParserImpl;

  /// \brief Called by the lexer to save the end location of the current file
  /// or buffer.
  void save_eof(const util::SourceRange &eof, size_t eof_ofs) {
    last_eof_ = eof;
    last_eof_ofs_ = eof_ofs;
  }

  void AppendChunk(lua::Node *stats);

  std::vector<lua::Node *> node_stack_;
  lua::Node **PopNodes(size_t node_count);
  lua::Tuple *PopTuple(size_t node_count);
  lua::Tuple *PopElseives(size_t node_count) { return PopTuple(node_count); }
  lua::Tuple *PopFunctionArgs(size_t node_count) {
    return PopTuple(node_count);
  }
  lua::Tuple *PopCallArgs(size_t node_count) { return PopTuple(node_count); }
  void PushNode(lua::Node *node);

  lua::Node *DesugarRepeat(const util::SourceRange &location, lua::Block *block,
                           lua::Node *condition);
  lua::Node *DesugarFor(const util::SourceRange &location, lua::Tuple *namelist,
                        lua::Tuple *explist, lua::Node *block);
  lua::Node *DesugarFor(const util::SourceRange &location, lua::Var *var,
                        lua::Node *init, lua::Node *limit, lua::Node *step,
                        lua::Node *block);

  /// The arena into which we will allocate objects.
  st::Arena *arena_ = nullptr;

  /// The symbol table to use.
  st::SymbolTable *symbol_table_ = nullptr;

  /// The file we're reading from.
  std::string file_ = "stdin";

  /// Save the end-of-file location from the lexer.
  util::SourceRange last_eof_;
  size_t last_eof_ofs_;

  /// Did we encounter errors during lexing or parsing?
  bool had_errors_ = false;

  /// Are we dumping lexer trace information?
  bool trace_lex_;

  /// Are we dumping parser trace information?
  bool trace_parse_;

  size_t raw_equals_count_ = 0;
  std::string raw_text_;
  bool raw_text_is_comment_ = false;

  lua::Tuple *empty_tuple_ = nullptr;
  lua::Node *primitive_error_ = nullptr;
  lua::Node *primitive_tonumber_ = nullptr;

  /// \note Implemented by generated code care of flex.
  static int lex(YYSTYPE *, util::SourceRange *, LuaParser &context);

  /// \brief Used by the lexer and parser to report errors.
  /// \param location Source location where an error occurred.
  /// \param message Text of the error.
  void Error(const util::SourceRange &location, const std::string &message);

  /// \brief Used by the lexer and parser to report errors.
  /// \param message Text of the error.
  void Error(const std::string &message);

  /// \brief Initializes the lexer to scan from file_.
  /// \note Implemented in `lua.l`.
  void ScanBeginFile(bool trace_scanning);

  /// \brief Initializes the lexer to scan from a string.
  /// \note Implemented in `lua.l`.
  void ScanBeginString(const std::string &data, bool trace_scanning);

  /// \brief Handles end-of-scan actions and destroys any buffers.
  /// \note Implemented in `lua.l`.
  void ScanEnd(const util::SourceRange &eof_loc, size_t eof_loc_ofs);
};
}

#endif
