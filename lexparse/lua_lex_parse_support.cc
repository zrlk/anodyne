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

#include "lua_lex.h"

namespace lua {
void Var::Dump(const st::SymbolTable &symbol_table,
               st::PrettyPrinter *printer) {
  const auto &text = symbol_table.text(symbol_);
  printer->Print(text);
}

void Tuple::Dump(const st::SymbolTable &symbol_table,
                 st::PrettyPrinter *printer) {
  for (size_t v = 0; v < element_count_; ++v) {
    elements_[v]->Dump(symbol_table, printer);
    if (v + 1 < element_count_) {
      printer->Print(", ");
    }
  }
}

void Tuple::DumpAsStmts(const st::SymbolTable &symbol_table,
                        st::PrettyPrinter *printer) {
  for (size_t v = 0; v < element_count_; ++v) {
    elements_[v]->Dump(symbol_table, printer);
    if (v + 1 < element_count_) {
      printer->Print(";\n");
    }
  }
}

void Literal::Dump(const st::SymbolTable &symbol_table,
                   st::PrettyPrinter *printer) {
  switch (type_) {
    case Literal::Type::String: {
      const auto &text = symbol_table.text(symbol_);
      printer->Print("\"");
      printer->Print(text);
      printer->Print("\"");
      break;
    }
    case Literal::Type::Number: {
      const auto &text = symbol_table.text(symbol_);
      printer->Print(text);
      break;
    }
    case Literal::Type::Nil:
      printer->Print("nil");
      break;
    case Literal::Type::True:
      printer->Print("true");
      break;
    case Literal::Type::False:
      printer->Print("false");
      break;
  }
}

void Function::Dump(const st::SymbolTable &symbol_table,
                    st::PrettyPrinter *printer) {
  printer->Print("(");
  bindings_->Dump(symbol_table, printer);
  if (varargs_) {
    if (bindings_->size()) {
      printer->Print(", ");
    }
    printer->Print("...");
  }
  printer->Print(") ");
  body_->Dump(symbol_table, printer);
}

void While::Dump(const st::SymbolTable &symbol_table,
                 st::PrettyPrinter *printer) {
  printer->Print("while ");
  condition_->Dump(symbol_table, printer);
  block_->Dump(symbol_table, printer);
}

void If::Dump(const st::SymbolTable &symbol_table, st::PrettyPrinter *printer) {
  printer->Print("if ");
  exp_->Dump(symbol_table, printer);
  printer->Print(" then ");
  then_->Dump(symbol_table, printer);
  printer->Print(" ");
  elseives_->DumpAsStmts(symbol_table, printer);
  printer->Print("else ");
  else_->Dump(symbol_table, printer);
  printer->Print(" end");
}

void ElseIf::Dump(const st::SymbolTable &symbol_table,
                  st::PrettyPrinter *printer) {
  printer->Print("elseif ");
  exp_->Dump(symbol_table, printer);
  printer->Print(" then ");
  block_->Dump(symbol_table, printer);
  printer->Print(" ");
}

void ArgsReference::Dump(const st::SymbolTable &symbol_table,
                         st::PrettyPrinter *printer) {
  printer->Print("...");
}

// TODO: use precedence to add parens.
void UnaryOp::Dump(const st::SymbolTable &symbol_table,
                   st::PrettyPrinter *printer) {
  switch (op_) {
    case UnaryOp::Op::Not:
      printer->Print("not");
      break;
    case UnaryOp::Op::Length:
      printer->Print("#");
      break;
    case UnaryOp::Op::Negate:
      printer->Print("-");
      break;
  }
  operand_->Dump(symbol_table, printer);
}

// TODO: use precedence to add parens.
void BinaryOp::Dump(const st::SymbolTable &symbol_table,
                    st::PrettyPrinter *printer) {
  lhs_->Dump(symbol_table, printer);
  switch (op_) {
    case BinaryOp::Op::Or:
      printer->Print(" or ");
      break;
    case BinaryOp::Op::And:
      printer->Print(" and ");
      break;
    case BinaryOp::Op::LessThan:
      printer->Print(" < ");
      break;
    case BinaryOp::Op::LessThanEqual:
      printer->Print(" <= ");
      break;
    case BinaryOp::Op::GreaterThan:
      printer->Print(" > ");
      break;
    case BinaryOp::Op::GreaterThanEqual:
      printer->Print(" >= ");
      break;
    case BinaryOp::Op::NotEqual:
      printer->Print(" ~= ");
      break;
    case BinaryOp::Op::Equal:
      printer->Print(" == ");
      break;
    case BinaryOp::Op::Concatenate:
      printer->Print(" .. ");
      break;
    case BinaryOp::Op::Add:
      printer->Print(" + ");
      break;
    case BinaryOp::Op::Subtract:
      printer->Print(" - ");
      break;
    case BinaryOp::Op::Multiply:
      printer->Print(" * ");
      break;
    case BinaryOp::Op::Divide:
      printer->Print(" / ");
      break;
    case BinaryOp::Op::Modulo:
      printer->Print(" % ");
      break;
    case BinaryOp::Op::Exponent:
      printer->Print(" ^ ");
      break;
  }
  rhs_->Dump(symbol_table, printer);
}

void DirectIndex::Dump(const st::SymbolTable &symbol_table,
                       st::PrettyPrinter *printer) {
  lhs_->Dump(symbol_table, printer);
  const auto &text = symbol_table.text(subscript_);
  printer->Print(".");
  printer->Print(text);
}

void Index::Dump(const st::SymbolTable &symbol_table,
                 st::PrettyPrinter *printer) {
  lhs_->Dump(symbol_table, printer);
  printer->Print("[");
  subscript_->Dump(symbol_table, printer);
  printer->Print("]");
}

void Call::Dump(const st::SymbolTable &symbol_table,
                st::PrettyPrinter *printer) {
  function_->Dump(symbol_table, printer);
  if (member_) {
    const auto &text = symbol_table.text(symbol_);
    printer->Print(":");
    printer->Print(text);
  }
  printer->Print("(");
  args_->Dump(symbol_table, printer);
  printer->Print(")");
}

// TODO: ensure we obey the grammar when pretty-printing.
void FunctionBinding::Dump(const st::SymbolTable &symbol_table,
                           st::PrettyPrinter *printer) {
  switch (kind_) {
    case FunctionBinding::Kind::Local: {
      const auto &text = symbol_table.text(symbol_);
      printer->Print("local function ");
      printer->Print(text);
      printer->Print(" ");
    } break;
    case FunctionBinding::Kind::Global: {
      printer->Print("function ");
      path_->Dump(symbol_table, printer);
      printer->Print(" ");
    } break;
    case FunctionBinding::Kind::GlobalMember: {
      const auto &text = symbol_table.text(symbol_);
      printer->Print("function ");
      path_->Dump(symbol_table, printer);
      printer->Print(":");
      printer->Print(text);
      printer->Print(" ");
    } break;
  }
  body_->Dump(symbol_table, printer);
}

void VarBinding::Dump(const st::SymbolTable &symbol_table,
                      st::PrettyPrinter *printer) {
  if (local_) {
    printer->Print("local ");
  }
  vars_->Dump(symbol_table, printer);
  if (inits_->size()) {
    printer->Print(" = ");
    inits_->Dump(symbol_table, printer);
  }
}

void Block::Dump(const st::SymbolTable &symbol_table,
                 st::PrettyPrinter *printer) {
  printer->Print("do\n");
  stmts_->DumpAsStmts(symbol_table, printer);
  switch (kind_) {
    case Block::Kind::ReturnExp:
      printer->Print("return ");
      return_exps_->Dump(symbol_table, printer);
      printer->Print("\n");
      break;
    case Block::Kind::Break:
      printer->Print("break\n");
      break;
    case Block::Kind::ReturnNone:
      printer->Print("return\n");
      break;
    default:
      break;
  }
  printer->Print("end\n");
}

void TableConstructor::Dump(const st::SymbolTable &symbol_table,
                            st::PrettyPrinter *printer) {
  printer->Print("{");
  fields_->DumpAsStmts(symbol_table, printer);
  printer->Print("}");
}

void Field::Dump(const st::SymbolTable &symbol_table,
                 st::PrettyPrinter *printer) {
  switch (kind_) {
    case Field::Kind::Label: {
      const auto &text = symbol_table.text(symbol_);
      printer->Print(text);
      printer->Print(" = ");
      break;
    }
    case Field::Kind::Bracket:
      printer->Print("[");
      index_->Dump(symbol_table, printer);
      printer->Print("] = ");
      break;
    default:
      break;
  }
  exp_->Dump(symbol_table, printer);
}
}

namespace lexparse {
void LuaParser::AppendChunk(lua::Node *node) {
  st::FileHandlePrettyPrinter pperr(stderr);
  node->Dump(*symbol_table_, &pperr);
}

void LuaParser::PushNode(lua::Node *node) { node_stack_.push_back(node); }

lua::Node **LuaParser::PopNodes(size_t count) {
  if (count > node_stack_.size()) {
    abort();
  }
  lua::Node **nodes = (lua::Node **)arena_->New(count * sizeof(lua::Node *));
  size_t start = node_stack_.size() - count;
  for (size_t c = 0; c < count; ++c) {
    nodes[c] = node_stack_[start + c];
  }
  node_stack_.resize(start);
  return nodes;
}

lua::Tuple *LuaParser::PopTuple(size_t count) {
  return new (arena_) lua::Tuple(yy::location(), count, PopNodes(count));
}

lua::DirectIndex *LuaParser::CreateDirectIndex(const yy::location &location,
                                               lua::Node *lhs,
                                               const std::string &for_text) {
  return new (arena_)
      lua::DirectIndex(location, lhs, symbol_table_->intern(for_text));
}

lua::Call *LuaParser::CreateMemberCall(const yy::location &location,
                                       lua::Node *function, lua::Tuple *args,
                                       const std::string &for_text) {
  return new (arena_)
      lua::Call(location, function, args, symbol_table_->intern(for_text));
}

lua::Literal *LuaParser::CreateNumberLiteral(const yy::location &location,
                                             const std::string &for_text) {
  return new (arena_) lua::Literal(location, lua::Literal::Type::Number,
                                   symbol_table_->intern(for_text));
}

lua::Literal *LuaParser::CreateStringLiteral(const yy::location &location,
                                             const std::string &for_text) {
  return new (arena_) lua::Literal(location, lua::Literal::Type::String,
                                   symbol_table_->intern(for_text));
}

lua::Var *LuaParser::CreateVar(const yy::location &location,
                               const std::string &for_text) {
  return new (arena_) lua::Var(location, symbol_table_->intern(for_text));
}

/*
  repeat
    local x = 1
    local function setx(i) x = i return true end
    setx(2)
    return print(x)
  until setx(3)
  => 2
  (e.g., if the repeat block has a terminator, the until expression isn't
  evaluated.)
*/
lua::Node *LuaParser::DesugarRepeat(const yy::location &location,
                                    lua::Block *block, lua::Node *condition) {
  if (block->kind() == lua::Block::Kind::NoTerminator) {
    auto *true_cond =
        new (arena_) lua::Literal(location, lua::Literal::Type::True);
    auto *until_block = new (arena_)
        lua::Block(location, empty_tuple_,
                   std::make_pair(nullptr, lua::Block::Kind::Break));
    auto *until_cond = new (arena_)
        lua::If(location, true_cond, until_block, empty_tuple_, empty_block());
    for (auto *node : *block->stmts()) {
      PushNode(node);
    }
    PushNode(until_cond);
    auto *while_body = PopTuple(block->stmts()->size() + 1);
    return new (arena_) lua::While(location, true_cond, while_body);
  } else {
    // Warn about dead code?
    return block;
  }
}

/*
  The spec gives that
     for var_1, ..., var_n in explist do block end
  desugars to:
     do
       local f, s, var = explist
       while true do
         local var_1, ..., var_n = f(s, var)
         var = var_1
         if var == nil then break end
         block
       end
     end
*/
lua::Node *LuaParser::DesugarFor(const yy::location &location,
                                 lua::Tuple *namelist, lua::Tuple *explist,
                                 lua::Node *block) {
  auto *f = CreateUnutterableVar(location, "f");
  auto *s = CreateUnutterableVar(location, "s");
  auto *var = CreateUnutterableVar(location, "var");
  PushNode(s);
  PushNode(var);
  auto *f_args = PopTuple(2);
  PushNode(new (arena_) lua::Call(location, f, f_args));
  auto *f_call = PopTuple(1);
  PushNode(var);
  auto *var_tuple = PopTuple(1);
  // It's syntactically invalid for namelist to be empty.
  PushNode(namelist->element(0));
  auto *var_1_tuple = PopTuple(1);
  PushNode(new (arena_) lua::VarBinding(location, true, namelist, f_call));
  PushNode(new (arena_)
               lua::VarBinding(location, false, var_tuple, var_1_tuple));
  PushNode(new (arena_) lua::If(
      location,
      new (arena_) lua::BinaryOp(
          location, lua::BinaryOp::Op::Equal, var,
          new (arena_) lua::Literal(location, lua::Literal::Type::Nil)),
      new (arena_) lua::Block(location, empty_tuple_,
                              std::make_pair(nullptr, lua::Block::Kind::Break)),
      empty_tuple_, empty_block()));
  PushNode(block);
  auto *while_body = PopTuple(4);
  PushNode(f);
  PushNode(s);
  PushNode(var);
  auto *inits = PopTuple(3);
  PushNode(new (arena_) lua::VarBinding(location, true, inits, explist));
  PushNode(new (arena_) lua::While(
      location, new (arena_) lua::Literal(location, lua::Literal::Type::True),
      while_body));
  auto *block_body = PopTuple(2);
  return new (arena_)
      lua::Block(location, block_body,
                 std::make_pair(nullptr, lua::Block::Kind::NoTerminator));
}

/*
  The spec gives that
     for v = e1, e2, e3 do block end
  desugars to:
     do
       local var, limit, step = tonumber(e1), tonumber(e2), tonumber(e3)
       if not (var and limit and step) then error() end
       while (step > 0 and var <= limit) or (step <= 0 and var >= limit) do
         local v = var
         block
         var = var + step
       end
     end
  this is a fiblet as we have:
    > function error(x) print("custom error") end
    > for x = "a", 2 do print(x) end
    stdin:1: 'for' initial value must be a number
    stack traceback:
      stdin:1: in main chunk
      [C]: at 0x00404d60
    > error("...")
    custom error
  e.g., error() (and similarly, tonumber()) aren't looked up in the context.
*/
lua::Node *LuaParser::DesugarFor(const yy::location &location, lua::Var *v,
                                 lua::Node *init, lua::Node *limit,
                                 lua::Node *step, lua::Node *block) {
  PushNode(init);
  auto *tonumber_init =
      new (arena_) lua::Call(location, primitive_tonumber(), PopCallArgs(1));
  PushNode(limit);
  auto *tonumber_limit =
      new (arena_) lua::Call(location, primitive_tonumber(), PopCallArgs(1));
  PushNode(step);
  auto *tonumber_step =
      new (arena_) lua::Call(location, primitive_tonumber(), PopCallArgs(1));
  PushNode(tonumber_init);
  PushNode(tonumber_limit);
  PushNode(tonumber_step);
  auto *local_inits = PopTuple(3);
  auto *var = CreateUnutterableVar(location, "var");
  PushNode(var);
  auto *limit_var = CreateUnutterableVar(location, "limit");
  PushNode(limit_var);
  auto *step_var = CreateUnutterableVar(location, "step");
  PushNode(step_var);
  auto *local_vars = PopTuple(3);
  auto *zero = CreateNumberLiteral(location, "0");
  auto *local_init =
      new (arena_) lua::VarBinding(location, true, local_vars, local_inits);
  auto *call_error =
      new (arena_) lua::Call(location, primitive_error(), empty_tuple_);
  auto *error_check = new (arena_) lua::If(
      location, new (arena_) lua::UnaryOp(
                    location, lua::UnaryOp::Op::Not,
                    new (arena_) lua::BinaryOp(
                        location, lua::BinaryOp::Op::And,
                        new (arena_) lua::BinaryOp(
                            location, lua::BinaryOp::Op::And, var, limit_var),
                        step_var)),
      call_error, empty_tuple_, empty_block());
  auto *lhs_cond = new (arena_) lua::BinaryOp(
      location, lua::BinaryOp::Op::And,
      new (arena_) lua::BinaryOp(location, lua::BinaryOp::Op::GreaterThan,
                                 step_var, zero),
      new (arena_) lua::BinaryOp(location, lua::BinaryOp::Op::LessThanEqual,
                                 var, limit_var));
  auto *rhs_cond = new (arena_) lua::BinaryOp(
      location, lua::BinaryOp::Op::And,
      new (arena_) lua::BinaryOp(location, lua::BinaryOp::Op::LessThanEqual,
                                 step_var, zero),
      new (arena_) lua::BinaryOp(location, lua::BinaryOp::Op::GreaterThanEqual,
                                 var, limit_var));
  PushNode(v);
  auto *v_tuple = PopTuple(1);
  PushNode(var);
  auto *var_tuple = PopTuple(1);
  PushNode(new (arena_)
               lua::BinaryOp(location, lua::BinaryOp::Op::Add, var, step_var));
  auto *inc_tuple = PopTuple(1);
  PushNode(new (arena_) lua::VarBinding(location, true, v_tuple, var_tuple));
  PushNode(block);
  PushNode(new (arena_) lua::VarBinding(location, false, var_tuple, inc_tuple));
  auto *cond_body = PopTuple(3);
  auto *inner_while = new (arena_) lua::While(
      location, new (arena_) lua::BinaryOp(location, lua::BinaryOp::Op::Or,
                                           lhs_cond, rhs_cond),
      cond_body);
  PushNode(local_init);
  PushNode(error_check);
  PushNode(inner_while);
  auto *block_body = PopTuple(3);
  return new (arena_)
      lua::Block(location, block_body,
                 std::make_pair(nullptr, lua::Block::Kind::NoTerminator));
}

bool LuaParser::Unescape(const char *yytext, std::string *out) {
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

bool LuaParser::ParseString(const std::string &content,
                            const std::string &fake_filename) {
  had_errors_ = false;
  file_ = fake_filename;
  ScanBeginString(content, trace_lex_);
  yy::LuaParserImpl parser(*this);
  parser.set_debug_level(trace_parse_);
  int result = parser.parse();
  ScanEnd(last_eof_, last_eof_ofs_);
  return result == 0 && !had_errors_;
}

bool LuaParser::ParseFile(const std::string &filename) {
  file_ = filename;
  had_errors_ = false;
  ScanBeginFile(trace_lex_);
  yy::LuaParserImpl parser(*this);
  parser.set_debug_level(trace_parse_);
  int result = parser.parse();
  ScanEnd(last_eof_, last_eof_ofs_);
  return result == 0 && !had_errors_;
}

void LuaParser::Error(const yy::location &location,
                      const std::string &message) {
  // TODO: replace with a PrettyPrinter
  std::cerr << location << ": " << message << std::endl;
  had_errors_ = true;
}

void LuaParser::Error(const std::string &message) {
  // TODO: replace with a PrettyPrinter
  std::cerr << "When trying " << file() << ": " << message << std::endl;
  had_errors_ = true;
}
}
