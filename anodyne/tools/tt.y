// This is a parser for tt.
// See https://github.com/google/anodyne/blob/master/anodyne/tools/tt.md for
// more details.
%skeleton "lalr1.cc"
%defines
%define "parser_class_name" {TtParserImpl}
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
#include <string>
#include "anodyne/tools/tt_bison_support.h"
#ifdef yylex
#undef yylex
#endif
#define yylex anodyne::TtParser::lex
namespace anodyne {
class TtParser;
}
// Satisfy bison-internal code.
inline ::std::ostream& operator<<(::std::ostream& lhs,
                                  const ::anodyne::Range& rhs) {
  return lhs;
}
%}
%parse-param { ::anodyne::TtParser &context }
%lex-param { ::anodyne::TtParser &context }
%locations
%define api.location.type {::anodyne::Range}
%initial-action
{
  @$ = context.initial_location();
};
%define "parse.trace"
%{
#include "anodyne/tools/tt_parser.h"
%}
%token
  END_OF_FILE 0 "end of file"
  LPAREN  "("
  RPAREN  ")"
  LBRACE  "{"
  RBRACE  "}"
  COMMA   ","
  COLON   ":"
  SEMICOLON ";"
  BRACKETS "[]"
  WHAT "?"
  PIPE "|"
  EQUALS "="
  STAR "*"
  HASH "#"

  TYPE "type"
  OF "of"
  MATCH "match"
  JSON "json"
  SOME "some"
  NONE "none"
;
%token <string> IDENTIFIER "identifier"
%token <string> NUMBER "number"
%token <string> STRING "string"

%type <size_t_> toplevel;
%type <size_t_> decl;
%type <size_t_> ctordecls;
%type <size_t_> ctordecl;
%type <size_t_> type0;
%type <size_t_> type1;
%type <size_t_> clauses;
%type <size_t_> clause;
%type <size_t_> patmatch;
%type <size_t_> pats;
%type <size_t_> pat0;
%type <size_t_> pat1;
%type <size_t_> declopts;
%type <size_t_> declopt;

%error-verbose
%%
%start toplevel;

// Stay left-recursive.
//   rule:
//     subrule
//   | rule subrule

toplevel:
  /* empty */ { $$ = 0; }
| patmatch toplevel { $$ = 0; }
| decl toplevel { $$ = 0; }

patmatch:
  MATCH LPAREN clauses RPAREN { context.ApplyMatch(@4, $3); $$ = 0; }

clauses:
  /* empty */ { $$ = 0; }
| clause clauses { $$ = $1 + $2; }

clause:
  PIPE pat0 { $$ = 1; }
| patmatch { $$ = 0; }

pats:
  pat0 { $$ = 1; }
| pat0 COMMA pats { $$ = $3 + 1; }

pat0:
  pat1 { $$ = $1; }
| SOME pat1 { context.ApplyOptionPattern(@1, true); $$ = 0; }
| NONE { context.ApplyOptionPattern(@1, false); $$ = 0; }

pat1:
  IDENTIFIER { context.PushPatternCtorOrVariable(@1, $1); $$ = 0; }
| LBRACE pats RBRACE {
    context.ApplyListPattern(@1, $2);
    $$ = 0;
}
| IDENTIFIER IDENTIFIER {
    context.PushPatternCtorOrVariable(@2, $2);
    context.ApplyCtorPattern(@1, $1, 1);
    $$ = 0;
}
| IDENTIFIER LPAREN pats RPAREN {
    context.ApplyCtorPattern(@1, $1, $3);
    $$ = 0;
}

decl:
  TYPE declopts IDENTIFIER EQUALS ctordecls { context.ApplyTypeDecl(@3, $3); $$ = 0; }

declopt:
  JSON STRING { context.ApplyJsonDeclopt($2); $$ = 0; }

declopts:
  /* empty */ { $$ = 0; }
| declopt declopts { $$ = 0; }

ctordecls:
  ctordecl { $$ = 0; }
| ctordecl PIPE ctordecls { $$ = 0; }

ctordecl:
  IDENTIFIER OF type0 { context.ApplyCtorDecl(@1, $1); $$ = 0; }
| IDENTIFIER { context.ApplyCtorDecl(@1, $1); $$ = 0; }

type0:
  type1 STAR type0 { context.ApplyStar(@2); $$ = 0; }
| type1 { $$ = 0; }

type1:
  IDENTIFIER { context.PushIdentifier(@1, $1, "", false, false); $$ = 0; }
| IDENTIFIER COLON IDENTIFIER { context.PushIdentifier(@1, $3, $1, false, false); $$ = 0; }
| IDENTIFIER BRACKETS { context.PushIdentifier(@1, $1, "", true, false); $$ = 0; }
| IDENTIFIER COLON IDENTIFIER BRACKETS { context.PushIdentifier(@1, $3, $1, true, false); $$ = 0; }
| IDENTIFIER WHAT { context.PushIdentifier(@1, $1, "", false, true); $$ = 0; }
| IDENTIFIER COLON IDENTIFIER WHAT { context.PushIdentifier(@1, $3, $1, false, true); $$ = 0; }
| IDENTIFIER COLON IDENTIFIER HASH { context.PushIdentifier(@1, $3, $1, false, false, true); $$ = 0; } // Special case for JSON decoding.

%%
void yy::TtParserImpl::error(const location_type &l,
                             const std::string &m) {
  context.Error(l, m);
}
