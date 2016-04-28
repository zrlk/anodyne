// This is a parser for Lua 5.1.
// Started from the calc++ example code as part of the Bison-3.0 distribution.
// NOTE: This file must remain compatible with Bison 2.3.
%skeleton "lalr1.cc"
%defines
%define "parser_class_name" "LuaParserImpl"
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
#include <string>
#include "lexparse/lua_parse.h"
#ifdef yylex
#undef yylex
#endif
#define yylex lexparse::LuaParser::lex
namespace lexparse {
class LuaParser;
}
%}
%parse-param { ::lexparse::LuaParser &context }
%lex-param { ::lexparse::LuaParser &context }
%locations
%define api.location.type {util::SourceRange}
%initial-action
{
  @$ = ::util::SourceRange(&context.file(), ::util::SourceLocation(1, 1),
                           ::util::SourceLocation(1, 1));
};
%define "parse.trace"
%{
#include "lexparse/lua_lex.h"
#define newAst new (context.arena_) lua::
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
  AND "and"
  OR "or"
  LBRACKET "["
  RBRACKET "]"  
  DOT "."
  EQUALS "="
  PLUS "+"
  MINUS "-"
  STAR "*"
  SLASH "/"
  HAT "^"
  PERCENT "%"
  DOTDOT ".."
  LT "<"
  GT ">"
  LTE "<="
  GTE ">="
  EQEQ "=="
  NOTEQ "~="
  NOT "not"
  HASH "#"
  DOTDOTDOT "..."

  DO "do"
  END "end"
  WHILE "while"
  REPEAT "repeat"
  UNTIL "until"
  IF "if"
  IN "in"
  THEN "then"
  ELSEIF "elseif"
  ELSE "else"
  FOR "for"
  FUNCTION "function"
  LOCAL "local"
  RETURN "return"
  BREAK "break"
  NIL "nil"
  FALSE "false"
  TRUE "true"
;
%token <string> IDENTIFIER "identifier"
%token <string> NUMBER "number"
%token <string> STRING "string"
%type <node> chunk
%type <node> stat
%type <size_t_> elseives

%type <size_t_> stats
%type <block> block

%type <node> var
%type <node> prefixexp
%type <node> exp
%type <node> exp0
%type <node> exp1
%type <node> exp2
%type <node> exp3
%type <node> exp4
%type <node> exp5
%type <node> exp6
%type <node> exp7
%type <size_t_> varlist
%type <size_t_> explist
%type <size_t_> namelist
%type <last_stat> laststat
%type <node> functioncall
%type <size_t_> args
%type <size_t_> funcname_nocolon
%type <node> tableconstructor
%type <size_t_> fieldlist
%type <node> field
%type <node> function
%type <node> funcbody

// %type <node> exp
// %type <node> exp_tuple_star
// %type <size_t_> exp_tuple_plus
%error-verbose
%%
%start chunk;

// please to be left-recursive
//   rule:
//     subrule
//   | rule subrule

stats:
  /* empty */ { $$ = 0; }
| stats stat { context.PushNode($2); $$ = $1 + 1; }
| stats SEMICOLON { $$ = $1; }

chunk:
  block { context.AppendChunk($1); }

block:
  stats laststat { $$ = newAst Block(@1, context.PopTuple($1), $2); }
| stats laststat SEMICOLON { $$ = newAst Block(@1, context.PopTuple($1), $2); }
| stats { $$ = newAst Block(@1, context.PopTuple($1), std::make_pair(nullptr, lua::Block::Kind::NoTerminator)); }

stat:
  varlist EQUALS explist { auto *exps = context.PopTuple($3); auto *vars = context.PopTuple($1); $$ = newAst VarBinding(@2, false, vars, exps); }
| functioncall { $$ = $1; }
| DO block END { $$ = $2; }
| WHILE exp DO block END { $$ = newAst While(@1, $2, $4); }
| REPEAT block UNTIL exp { $$ = context.DesugarRepeat(@1, $2, $4); }
| IF exp THEN block elseives END { $$ = newAst If(@1, $2, $4, context.PopElseives($5), context.empty_block()); }
| IF exp THEN block elseives ELSE block END { $$ = newAst If(@1, $2, $4, context.PopElseives($5), $7); }
| FOR IDENTIFIER EQUALS exp COMMA exp DO block END { $$ = context.DesugarFor(@1, context.CreateVar(@2, $2), $4, $6, context.CreateNumberLiteral(@1, "1"), $8); }
| FOR IDENTIFIER EQUALS exp COMMA exp COMMA exp DO block END { $$ = context.DesugarFor(@1, context.CreateVar(@2, $2), $4, $6, $8, $10); }
| FOR namelist IN explist DO block END { auto *exps = context.PopTuple($4); auto *vars = context.PopTuple($2); $$ = context.DesugarFor(@1, vars, exps, $6); }
| FUNCTION funcname_nocolon funcbody { $$ = newAst FunctionBinding(@1, context.PopTuple($2), $3); }
| FUNCTION funcname_nocolon COLON IDENTIFIER funcbody { $$ = newAst FunctionBinding(@4, context.PopTuple($2), context.Intern($4), $5); }
| LOCAL FUNCTION IDENTIFIER funcbody { $$ = newAst FunctionBinding(@3, context.Intern($3), $4); }
| LOCAL namelist { $$ = newAst VarBinding(@1, true, context.PopTuple($2), context.empty_var_inits()); }
| LOCAL namelist EQUALS explist { auto *exps = context.PopTuple($4); auto *vars = context.PopTuple($2); $$ = newAst VarBinding(@1, true, vars, exps); }

elseives:
  /* empty */ { $$ = 0; }
| elseives ELSEIF exp THEN block { context.PushNode(newAst ElseIf(@2, $3, $5)); $$ = $1 + 1; }

laststat:
  RETURN { $$ = std::make_pair(nullptr, lua::Block::Kind::ReturnNone); }
| RETURN explist { $$ = std::make_pair(context.PopTuple($2), lua::Block::Kind::ReturnExp); }
| BREAK { $$ = std::make_pair(nullptr, lua::Block::Kind::Break); }

funcname_nocolon:
  IDENTIFIER { context.PushNode(context.CreateVar(@1, $1)); $$ = 1; }
| funcname_nocolon DOT IDENTIFIER { context.PushNode(context.CreateVar(@3, $3)); $$ = $1 + 1; }

varlist:
  var { context.PushNode($1); $$ = 1; }
| varlist COMMA var { context.PushNode($3); $$ = $1 + 1; }

var:
  IDENTIFIER { $$ = context.CreateVar(@1, $1); }
| prefixexp LBRACKET exp RBRACKET { $$ = newAst Index(@2, $1, $3); }
| prefixexp DOT IDENTIFIER { $$ = context.CreateDirectIndex(@3, $1, $3); }

namelist:
  IDENTIFIER { context.PushNode(context.CreateVar(@1, $1)); $$ = 1; }
| namelist COMMA IDENTIFIER { context.PushNode(context.CreateVar(@3, $3)); $$ = $1 + 1; }

explist:
  exp { context.PushNode($1); $$ = 1; }
| explist COMMA exp { context.PushNode($3); $$ = $1 + 1; }

exp:
  exp0 { $$ = $1; }
| exp OR exp0 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Or, $1, $3); }

exp0:
  exp1 { $$ = $1; }
| exp0 AND exp1 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::And, $1, $3); }

exp1:
  exp2 { $$ = $1; }
| exp1 LT exp2 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::LessThan, $1, $3); }
| exp1 LTE exp2 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::LessThanEqual, $1, $3); }
| exp1 GT exp2 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::GreaterThan, $1, $3); }
| exp1 GTE exp2 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::GreaterThanEqual, $1, $3); }
| exp1 NOTEQ exp2 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::NotEqual, $1, $3); }
| exp1 EQEQ exp2 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Equal, $1, $3); }

exp2:  // DOTDOT is right-associative
  exp3 { $$ = $1; }
| exp3 DOTDOT exp2 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Concatenate, $1, $3); }

exp3:
  exp4 { $$ = $1; }
| exp3 PLUS exp4 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Add, $1, $3); }
| exp3 MINUS exp4 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Subtract, $1, $3); }

exp4:
  exp5 { $$ = $1; }
| exp4 STAR exp5 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Multiply, $1, $3); }
| exp4 SLASH exp5 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Divide, $1, $3); }
| exp4 PERCENT exp5 { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Modulo, $1, $3); }

exp5:
  exp6 { $$ = $1; }
| NOT exp5 { $$ = newAst UnaryOp(@1, lua::UnaryOp::Op::Not, $2); }
| HASH exp5 { $$ = newAst UnaryOp(@1, lua::UnaryOp::Op::Length, $2); }
| MINUS exp5 { $$ = newAst UnaryOp(@1, lua::UnaryOp::Op::Negate, $2); }

exp6:  // HAT is right-associative
  exp7 { $$ = $1; }
| exp7 HAT exp6  { $$ = newAst BinaryOp(@2, lua::BinaryOp::Op::Exponent, $1, $3); }

exp7:
  NIL { $$ = newAst Literal(@1, lua::Literal::Type::Nil); }
| FALSE { $$ = newAst Literal(@1, lua::Literal::Type::False); }
| TRUE { $$ = newAst Literal(@1, lua::Literal::Type::True); }
| NUMBER { $$ = context.CreateNumberLiteral(@1, $1); }
| STRING { $$ = context.CreateStringLiteral(@1, $1); }
| DOTDOTDOT { $$ = newAst ArgsReference(@1); }
| function { $$ = $1; }
| prefixexp { $$ = $1; }
| tableconstructor { $$ = $1; }

prefixexp:
  var { $$ = $1; }
| functioncall { $$ = $1; }

functioncall:
  prefixexp args { $$ = newAst Call(@1, $1, context.PopCallArgs($2)); }
| prefixexp COLON IDENTIFIER args { $$ = context.CreateMemberCall(@3, $1, context.PopCallArgs($4), $3); }

args:
  LPAREN RPAREN { $$ = 0; }
| LPAREN explist RPAREN { $$ = $2; }
| STRING { context.PushNode(context.CreateStringLiteral(@1, $1)); $$ = 1; }
| tableconstructor { context.PushNode($1); $$ = 1; }

function:
  FUNCTION funcbody { $$ = $2; }

funcbody:
  LPAREN RPAREN block END { $$ = newAst Function(@1, context.empty_function_args(), false, $3); }
| LPAREN DOTDOTDOT RPAREN block END { $$ = newAst Function(@1, context.empty_function_args(), true, $4); }
| LPAREN namelist RPAREN block END { $$ = newAst Function(@1, context.PopFunctionArgs($2), false, $4); }
| LPAREN namelist COMMA DOTDOTDOT RPAREN block END { $$ = newAst Function(@1, context.PopFunctionArgs($2), true, $6); }

tableconstructor:
  LBRACE fieldlist fieldlist_lastterminator RBRACE { $$ = newAst TableConstructor(@1, context.PopTuple($2)); }
| LBRACE RBRACE { $$ = newAst TableConstructor(@1, context.empty_fields());  }

fieldlist:
  fieldlist COMMA field { context.PushNode($3); $$ = $1 + 1; }
| fieldlist SEMICOLON field { context.PushNode($3); $$ = $1 + 1; }
| field { context.PushNode($1); $$ = 1; }

fieldlist_lastterminator:
  /* empty */ | COMMA | SEMICOLON { }

field:
  exp { $$ = newAst Field(@1, $1); }
| IDENTIFIER EQUALS exp { $$ = newAst Field(@1, context.Intern($1), $3); }
| LBRACKET exp RBRACKET EQUALS exp { $$ = newAst Field(@1, $2, $5); }

%%
void yy::LuaParserImpl::error(const location_type &l,
                              const std::string &m) {
  context.Error(l, m);
}
