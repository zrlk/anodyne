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

#ifndef ANODYNE_TOOLS_TT_BISON_SUPPORT_H_
#define ANODYNE_TOOLS_TT_BISON_SUPPORT_H_

#include "anodyne/base/source.h"

/// \brief Signature for the yacc-generated lexer.
#define YY_DECL                                               \
  int ::anodyne::TtParser::lex(YySemanticValue* yylval_param, \
                               ::anodyne::Range* yylloc,      \
                               ::anodyne::TtParser& context)

/// \brief Updates the parser's internal record of its location.
///
/// See https://www.gnu.org/software/bison/manual/html_node/Location-Default-Action.html
#define YYLLOC_DEFAULT(Cur, Rhs, N)                                           \
  do {                                                                        \
    if (N != 0) {                                                             \
      (Cur) = ::anodyne::Range{YYRHSLOC(Rhs, 1).begin, YYRHSLOC(Rhs, N).end}; \
    } else {                                                                  \
      (Cur) = ::anodyne::Range{YYRHSLOC(Rhs, 0).begin, YYRHSLOC(Rhs, 0).end}; \
    }                                                                         \
  } while (0)

namespace anodyne {
class TtParser;
}
/// \brief Possible types for semantic results for Bison productions and tokens.
struct YySemanticValue {
  std::string string;
  size_t size_t_;
};
#define YYSTYPE YySemanticValue

#endif  // ANODYNE_TOOLS_TT_BISON_SUPPORT_H_
