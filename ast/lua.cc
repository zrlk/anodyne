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

#include "ast/lua.h"

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
