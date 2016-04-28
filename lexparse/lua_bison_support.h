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

#ifndef LEXPARSE_LUA_PARSE_H_
#define LEXPARSE_LUA_PARSE_H_

#include "ast/lua.h"

// Required by generated code.
#define YY_DECL                                               \
  int lexparse::LuaParser::lex(YySemanticValue *yylval_param, \
                               ::util::SourceRange *yylloc,   \
                               ::lexparse::LuaParser &context)

#define YYLLOC_DEFAULT(Cur, Rhs, N)                                       \
  do {                                                                    \
    if (N != 0) {                                                         \
      (Cur) = ::util::SourceRange((Cur).file(), YYRHSLOC(Rhs, 1).begin(), \
                                  YYRHSLOC(Rhs, N).end());                \
    } else {                                                              \
      (Cur) = ::util::SourceRange((Cur).file(), YYRHSLOC(Rhs, 0).begin(), \
                                  YYRHSLOC(Rhs, 0).end());                \
    }                                                                     \
  } while (0)

namespace lexparse {
class LuaParser;
}
struct YySemanticValue {
  std::string string;
  lua::Node *node;
  lua::Block *block;
  std::pair<lua::Tuple *, lua::Block::Kind> last_stat;
  int int_;
  size_t size_t_;
};
#define YYSTYPE YySemanticValue

#endif  // LEXPARSE_LUA_PARSE_H_
