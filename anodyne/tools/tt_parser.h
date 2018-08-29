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

#ifndef ANODYNE_TOOLS_TT_PARSER_H_
#define ANODYNE_TOOLS_TT_PARSER_H_

#include <map>
#include <memory>

#ifdef YY_NO_UNISTD_H
inline int isatty(int) { return 0; }
#endif

namespace yy {
class TtParserImpl;
}

#include "anodyne/base/source.h"
// clang-format off
// Don't reorder these includes.
#include "anodyne/tools/tt_bison_support.h"
#include "anodyne/tools/tt.y.hh"
// clang-format on

namespace anodyne {
/// \brief Represents a type, which is either a tuple of types or
/// a (single, optional, or array of) base types.
struct TtTypeNode {
  enum class Kind { kTuple, kIdentifier } kind;
  /// The label for this node, if any and if it's a kIdentifier.
  std::string label;
  /// The identifier for this node, if it's a kIdentifier.
  std::string ident;
  /// Where this node was defined.
  Range loc;
  /// Whether this is an array (if it's a kIdentifier).
  bool is_array = false;
  /// Whether this is optional (if it's a kIdentifier).
  bool is_option = false;
  /// Whether this is a hash (if it's a kIdentifier).
  bool is_hash = false;
  /// Children of this node (if it's a kTuple).
  std::vector<std::unique_ptr<TtTypeNode>> children;
};
/// \brief A single type constructor.
struct TtConstructor {
  /// The name of the constructor.
  std::string ident;
  /// Where the constructor was defined.
  Range loc;
  /// The type of the constructor, or null if it carries no data.
  std::unique_ptr<TtTypeNode> type;
};
/// \brief A single datatype.
struct TtDatatype {
  /// The identifier as written ("core.exp")
  std::string raw_ident;
  /// The last node of the identifier ("exp")
  std::string unqualified_ident;
  /// The identifier as a C++ qualified name ("core::exp")
  std::string qualified_ident;
  /// All qualifiers except the final one ({"core"})
  std::vector<std::string> qualifiers;
  /// Where this datatype was defined.
  Range loc;
  /// Constructors for this datatype.
  std::vector<TtConstructor> ctors;
  /// Whether json import support should be generated.
  bool derive_json = false;
  /// The text to use for the json context.
  std::string json_arg;
};
/// \brief A node in a pattern.
struct TtPat {
  enum class Kind { kVariable, kCtorApp, kList, kSome, kNone } kind;
  /// The identifier for the pattern (for kVariable or kCtorApp).
  std::string ident;
  /// Where this node was defined.
  Range loc;
  /// Children of this node (the args for kCtorApp, members for kList,
  /// and the optional type for kSome).
  std::vector<std::unique_ptr<TtPat>> children;
};
/// \brief A clause in a match expression.
struct TtClause {
  /// The pattern to match.
  std::unique_ptr<TtPat> pat;
};
/// \brief A single match expression.
struct TtMatch {
  /// Where the match happened.
  Range loc;
  /// Clauses in the expression.
  std::vector<std::unique_ptr<TtClause>> clauses;
};
class TtParser {
 public:
  /// \param trace_lex Dump lexing debug information
  /// \param trace_parse Dump parsing debug information
  TtParser(bool trace_lex = false, bool trace_parse = false)
      : trace_lex_(trace_lex), trace_parse_(trace_parse) {}

  /// \param file the file to parse.
  /// \param needs_cleaned whether the file has embedded patterns and therefore
  /// must be scrubbed of other code.
  /// \return true if there were no errors.
  bool ParseFile(const File* to_parse, bool needs_cleaned);

  /// \brief Unescapes a string literal (which is expected to include
  /// terminating quotes). Escape codes supported are \\, \n, and \".
  /// \param yytext literal string to escape
  /// \param out pointer to a string to overwrite with `yytext` unescaped.
  /// \return true if `yytext` was a valid literal string; false otherwise.
  static bool Unescape(const char* yytext, std::string* out);

  /// \return The initial location of the file being parsed.
  Range initial_location() const { return initial_location_; }

  /// \brief pushes an identifier to the type stack.
  /// \param ident the identifier to push.
  /// \param label the label for the identifier.
  /// \param is_array whether the identifier is an array (marked as []).
  /// \param is_option whether the identifier is an option (marked as ?).
  /// \param is_hash whether the identifier is a hash (marked as #).
  void PushIdentifier(const Range&, const std::string& ident,
                      const std::string& label, bool is_array, bool is_option,
                      bool is_hash = false);

  /// \brief creates a tuple from the top two items on the type stack. If the
  /// second-topmost item is a tuple, appends the topmost item to the tuple.
  /// Otherwise, creates a new tuple with the second-topmost item as the first
  /// element and the topmost as the second.
  void ApplyStar(const Range&);

  /// \brief creates a new constructor, using the topmost type on the stack
  /// (or none if the stack is empty).
  void ApplyCtorDecl(const Range&, const std::string& ident);

  /// \brief finishes the datatype under construction.
  void ApplyTypeDecl(const Range&, const std::string& ident);

  /// \brief sets the current datatype's json context string to `arg` and turns
  /// on json support for that datatype.
  void ApplyJsonDeclopt(const std::string& arg);

  /// \brief pushes `ident` to the pattern stack.
  void PushPatternCtorOrVariable(const Range&, const std::string& ident);

  /// \brief constructs a new option pattern. If `is_some` is true, uses the
  /// top of the pattern stack to match `Some exp`. Otherwise, matches `None`.
  /// Pushes the result to the pattern stack.
  void ApplyOptionPattern(const Range&, bool is_some);

  /// \brief constructs a new ctor pattern using the top `pat_count` clauses
  /// from the pattern stack, then pushes the pattern to the stack.
  void ApplyCtorPattern(const Range&, const std::string& ident,
                        size_t pat_count);

  /// \brief constructs a new list pattern using the top `pat_count` clauses
  /// from the pattern stack, then pushes the pattern to the stack.
  void ApplyListPattern(const Range&, size_t pat_count);

  /// \brief constructs a new matcher using the top `clause_count` clauses from
  /// the pattern stack.
  void ApplyMatch(const Range&, size_t clause_count);

  /// \return a map from string identifiers to all defined datatypes.
  const std::map<std::string, TtDatatype>& datatypes() const {
    return datatypes_;
  }

  /// \return a map from string identifiers to their containing datatypes.
  const std::map<std::string, std::string>& ctor_to_datatype() const {
    return ctor_to_datatype_;
  }

  /// \return all matchers that have been parsed.
  const std::vector<std::unique_ptr<TtMatch>>& matchers() const {
    return matches_;
  }

 private:
  friend class yy::TtParserImpl;

  /// \brief Sets the scan buffer to a premarked string and turns on
  /// tracing.
  /// \note Implemented in `tt.l`.
  void SetScanBuffer(absl::string_view scan_buffer, bool trace_scanning);

  /// Did we encounter errors during lexing or parsing?
  bool had_errors_ = false;

  /// Are we dumping lexer trace information?
  bool trace_lex_;

  /// Are we dumping parser trace information?
  bool trace_parse_;

  /// \note Implemented by generated code care of flex.
  static int lex(YYSTYPE*, Range*, TtParser& context);

  /// \brief Used by the lexer and parser to report errors.
  /// \param location Source location where an error occurred.
  /// \param message Text of the error.
  void Error(const Range& location, const std::string& message);

  /// \brief Used by the lexer and parser to report errors.
  /// \param message Text of the error.
  void Error(const std::string& message);

  /// \brief Initializes the lexer to scan from a UTF-8 string.
  void ScanBeginString(absl::string_view data, bool trace_scanning,
                       bool needs_cleaned);

  /// \brief Handles end-of-scan actions and destroys any buffers.
  void ScanEnd();

  /// The file being parsed.
  const File* file_;

  /// The initial location of the file being parsed.
  Range initial_location_;

  /// The datatype being built.
  TtDatatype next_datatype_;

  /// The match being built.
  TtMatch next_match_;

  /// The stack of types as they're parsed.
  std::vector<std::unique_ptr<TtTypeNode>> type_stack_;

  /// The stack of patterns as they're parsed.
  std::vector<std::unique_ptr<TtPat>> pat_stack_;

  /// Maps from datatype names to datatype records.
  std::map<std::string, TtDatatype> datatypes_;

  /// Maps from constructor names to their datatypes.
  std::map<std::string, std::string> ctor_to_datatype_;

  /// Matches that have been parsed.
  std::vector<std::unique_ptr<TtMatch>> matches_;
};
}  // namespace anodyne

#endif  // ANODYNE_TOOLS_TT_PARSER_H_
