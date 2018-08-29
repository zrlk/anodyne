%{
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

// The offset of the current token (as byte offset).
static size_t tok_offset;
%}
%option noyywrap nounput batch debug noinput bison-bridge
id    [a-zA-Z_][a-zA-Z_0-9.]*
int   [0-9]+
blank [ \t]

%{
  // Code run each time a pattern is matched.
  #define YY_USER_ACTION  *yylloc = ::anodyne::Range{yylloc->begin, \
      yylloc->end.offset(yyleng)}; tok_offset += yyleng;
%}

/* The lexer has the following states:
 *   INITIAL: Normal parsing state.
 *   COMMENT: We're in a C-style comment.                                     */
%s COMMENT
%%

%{
  // Code run each time yylex is called.
  *yylloc = ::anodyne::Range{yylloc->end, yylloc->end};
%}

<INITIAL>{
{blank}+ *yylloc = ::anodyne::Range{yylloc->end, yylloc->end};
\n       *yylloc = ::anodyne::Range{yylloc->end, yylloc->end};
"("      return yy::TtParserImpl::token::LPAREN;
")"      return yy::TtParserImpl::token::RPAREN;
"{"      return yy::TtParserImpl::token::LBRACE;
"}"      return yy::TtParserImpl::token::RBRACE;
","      return yy::TtParserImpl::token::COMMA;
":"      return yy::TtParserImpl::token::COLON;
";"      return yy::TtParserImpl::token::SEMICOLON;
"[]"     return yy::TtParserImpl::token::BRACKETS;
"?"      return yy::TtParserImpl::token::WHAT;
"|"      return yy::TtParserImpl::token::PIPE;
"="      return yy::TtParserImpl::token::EQUALS;
"*"      return yy::TtParserImpl::token::STAR;
"#"      return yy::TtParserImpl::token::HASH;

"type"      return yy::TtParserImpl::token::TYPE;
"of"        return yy::TtParserImpl::token::OF;
"match"     return yy::TtParserImpl::token::MATCH;
"json"      return yy::TtParserImpl::token::JSON;
"Some"      return yy::TtParserImpl::token::SOME;
"None"      return yy::TtParserImpl::token::NONE;

"/*"        BEGIN(COMMENT);
"//".*[^\n] { /* comment to drop on the floor */ }

{int}    yylval->string = yytext; return yy::TtParserImpl::token::NUMBER;
{id}     yylval->string = yytext; return yy::TtParserImpl::token::IDENTIFIER;
\"(\\.|[^\\"])*\" {
                   std::string out;
                   if (!context.Unescape(yytext, &out)) {
                     context.Error(*yylloc, "invalid literal string");
                   }
                   yylval->string = std::move(out);
                   return yy::TtParserImpl::token::STRING;
                 }
"<<<".*">>>" {
                       yylval->string = yytext;
                       return yy::TtParserImpl::token::STRING;
                     }
.        context.Error(*yylloc, "invalid character");
}  /* INITIAL state */

<COMMENT>{
.*"*/"   { BEGIN(INITIAL); }
<<EOF>>  {  context.Error(*yylloc, "unterminated comment at end of input");
            return yy::TtParserImpl::token::END_OF_FILE; }
}  /* COMMENT state */

<<EOF>>  {
            return yy::TtParserImpl::token::END_OF_FILE;
         }
%%
namespace anodyne {

static YY_BUFFER_STATE string_buffer_state = nullptr;

void TtParser::SetScanBuffer(absl::string_view scan_buffer,
                             bool trace_scanning) {
  BEGIN(INITIAL);
  tok_offset = 0;
  yy_flex_debug = trace_scanning;
  // yy_scan_bytes makes a copy of its buffer.
  string_buffer_state = yy_scan_bytes(scan_buffer.data(), scan_buffer.size());
}

void TtParser::ScanEnd() {
  yy_delete_buffer(string_buffer_state);
  string_buffer_state = nullptr;
}


}

