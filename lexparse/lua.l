%{
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
// Started from the calc++ example code as part of the Bison-3.0 distribution.
// assertions.h
#include "lexparse/lua_lex.h"
#include <assert.h>

// The offset of the current token (as byte offset).
static size_t loc_ofs;
%}
%option noyywrap nounput batch debug noinput bison-bridge
id    [a-zA-Z_][a-zA-Z_0-9]*
int   [0-9]+
blank [ \t]

%{
  // Code run each time a pattern is matched.
  #define YY_USER_ACTION  *yylloc = ::util::SourceRange(yylloc->file(), yylloc->begin(), yylloc->end().offset(yyleng)); loc_ofs += yyleng;
%}

/* The lexer has the following states:
 *   INITIAL: Normal parsing state.
 *   BRACKET: We're in one or more long brackets.                             */
%s BRACKET
%%

%{
  // Code run each time yylex is called.
  *yylloc = ::util::SourceRange(yylloc->file(), yylloc->end(), yylloc->end());
%}

<INITIAL>{
{blank}+ *yylloc = ::util::SourceRange(yylloc->file(), yylloc->end(), yylloc->end());
\n       *yylloc = ::util::SourceRange(yylloc->file(), yylloc->end().offset_lines(yyleng), yylloc->end().offset_lines(yyleng));
"("      return yy::LuaParserImpl::token::LPAREN;
")"      return yy::LuaParserImpl::token::RPAREN;
"{"      return yy::LuaParserImpl::token::LBRACE;
"}"      return yy::LuaParserImpl::token::RBRACE;
","      return yy::LuaParserImpl::token::COMMA;
":"      return yy::LuaParserImpl::token::COLON;
";"      return yy::LuaParserImpl::token::SEMICOLON;
"and"      return yy::LuaParserImpl::token::AND;
"or"      return yy::LuaParserImpl::token::OR;
"["      return yy::LuaParserImpl::token::LBRACKET;
"]"      return yy::LuaParserImpl::token::RBRACKET;
"."      return yy::LuaParserImpl::token::DOT;
"="      return yy::LuaParserImpl::token::EQUALS;
"+"      return yy::LuaParserImpl::token::PLUS;
"-"      return yy::LuaParserImpl::token::MINUS;
"*"      return yy::LuaParserImpl::token::STAR;
"/"      return yy::LuaParserImpl::token::SLASH;
"^"      return yy::LuaParserImpl::token::HAT;
"%"      return yy::LuaParserImpl::token::PERCENT;
".."      return yy::LuaParserImpl::token::DOTDOT;
"<"      return yy::LuaParserImpl::token::LT;
">"      return yy::LuaParserImpl::token::GT;
"<="      return yy::LuaParserImpl::token::LTE;
">="      return yy::LuaParserImpl::token::GTE;
"=="      return yy::LuaParserImpl::token::EQEQ;
"~="      return yy::LuaParserImpl::token::NOTEQ;
"not"      return yy::LuaParserImpl::token::NOT;
"#"      return yy::LuaParserImpl::token::HASH;
"..."      return yy::LuaParserImpl::token::DOTDOTDOT;
"do"      return yy::LuaParserImpl::token::DO;
"end"      return yy::LuaParserImpl::token::END;
"while"      return yy::LuaParserImpl::token::WHILE;
"repeat"      return yy::LuaParserImpl::token::REPEAT;
"until"      return yy::LuaParserImpl::token::UNTIL;
"if"      return yy::LuaParserImpl::token::IF;
"in"      return yy::LuaParserImpl::token::IN;
"then"      return yy::LuaParserImpl::token::THEN;
"elseif"      return yy::LuaParserImpl::token::ELSEIF;
"else"      return yy::LuaParserImpl::token::ELSE;
"for"      return yy::LuaParserImpl::token::FOR;
"function"      return yy::LuaParserImpl::token::FUNCTION;
"local"      return yy::LuaParserImpl::token::LOCAL;
"return"      return yy::LuaParserImpl::token::RETURN;
"break"      return yy::LuaParserImpl::token::BREAK;
"nil"      return yy::LuaParserImpl::token::NIL;
"false"      return yy::LuaParserImpl::token::FALSE;
"true"      return yy::LuaParserImpl::token::TRUE;

\[\=*\[     context.EnterRawText(yyleng - 2); BEGIN(BRACKET);
"--"\[\=*\[ context.EnterRawComment(yyleng - 4); BEGIN(BRACKET);
"--".*[^\n] { /* comment to drop on the floor */ }

{int}    yylval->string = yytext; return yy::LuaParserImpl::token::NUMBER;
{id}     yylval->string = yytext; return yy::LuaParserImpl::token::IDENTIFIER;
\"(\\.|[^\\"])*\" {
                   std::string out;
                   if (!context.Unescape(yytext, &out)) {
                     context.Error(*yylloc, "invalid literal string");
                   }
                   yylval->string = std::move(out);
                   return yy::LuaParserImpl::token::STRING;
                 }
.        context.Error(*yylloc, "invalid character");
}  /* INITIAL state */

<BRACKET>{
.*\]\=*\] { bool was_comment;
            std::string out;
            if (context.ExitRawRegion(yytext, *yylloc, &was_comment, &out)) {
              BEGIN(INITIAL);
              if (!was_comment) {
                // Drop comments on the floor.
                yylval->string = std::move(out);
                return yy::LuaParserImpl::token::STRING;
              }
            }
         }
<<EOF>>  {  context.Error(*yylloc, "unterminated raw text at end of input");
            context.save_eof(*yylloc, loc_ofs);
            return yy::LuaParserImpl::token::END_OF_FILE; }

}  /* BRACKET state */

<<EOF>>  {
            context.save_eof(*yylloc, loc_ofs);
            return yy::LuaParserImpl::token::END_OF_FILE;
         }
%%
namespace lexparse {

static YY_BUFFER_STATE stringBufferState = nullptr;
static std::string *kNoFile = new std::string("no-file");

void LuaParser::ScanBeginString(const std::string &data,
                                bool trace_scanning) {
  BEGIN(INITIAL);
  loc_ofs = 0;
  yy_flex_debug = trace_scanning;
  assert(stringBufferState == nullptr);
  stringBufferState = yy_scan_bytes(data.c_str(), data.size());
}

void LuaParser::ScanBeginFile(bool trace_scanning) {
  BEGIN(INITIAL);
  loc_ofs = 0;
  yy_flex_debug = trace_scanning;
  if (file().empty() || file() == "-") {
    yyin = stdin;
  } else if (!(yyin = fopen(file().c_str(), "r"))) {
    Error("cannot open " + file() + ": " + strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void LuaParser::ScanEnd(const ::util::SourceRange &eof_loc,
                        size_t eof_loc_ofs) {
  if (stringBufferState) {
    yy_delete_buffer(stringBufferState);
    stringBufferState = nullptr;
  } else {
    fclose(yyin);
  }
}

}

