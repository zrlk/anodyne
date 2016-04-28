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

#include "lexparse/lua_parser.h"

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
  return new (arena_) lua::Tuple(util::SourceRange(), count, PopNodes(count));
}

lua::DirectIndex *LuaParser::CreateDirectIndex(
    const util::SourceRange &location, lua::Node *lhs,
    const std::string &for_text) {
  return new (arena_)
      lua::DirectIndex(location, lhs, symbol_table_->intern(for_text));
}

lua::Call *LuaParser::CreateMemberCall(const util::SourceRange &location,
                                       lua::Node *function, lua::Tuple *args,
                                       const std::string &for_text) {
  return new (arena_)
      lua::Call(location, function, args, symbol_table_->intern(for_text));
}

lua::Literal *LuaParser::CreateNumberLiteral(const util::SourceRange &location,
                                             const std::string &for_text) {
  return new (arena_) lua::Literal(location, lua::Literal::Type::Number,
                                   symbol_table_->intern(for_text));
}

lua::Literal *LuaParser::CreateStringLiteral(const util::SourceRange &location,
                                             const std::string &for_text) {
  return new (arena_) lua::Literal(location, lua::Literal::Type::String,
                                   symbol_table_->intern(for_text));
}

lua::Var *LuaParser::CreateVar(const util::SourceRange &location,
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
lua::Node *LuaParser::DesugarRepeat(const util::SourceRange &location,
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
lua::Node *LuaParser::DesugarFor(const util::SourceRange &location,
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
lua::Node *LuaParser::DesugarFor(const util::SourceRange &location, lua::Var *v,
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

void LuaParser::Error(const util::SourceRange &location,
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
