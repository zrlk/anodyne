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

#include "ast/systemd.h"

namespace systemd {

void LogicalValue::Dump(PrintContext* context) { val_->Dump(context); }

void LogicalApp::Dump(PrintContext* context) {
  switch (symbol_) {
    case FunctionSymbol::Tag:
      context->printer()->Print("tag");
      break;
    case FunctionSymbol::Sel:
      context->printer()->Print("sel");
      break;
    case FunctionSymbol::Upd:
      context->printer()->Print("upd");
      break;
    case FunctionSymbol::Plus:
      context->printer()->Print("+");
      break;
  }
  context->printer()->Print("(");
  for (size_t i = 0; i < value_count_; ++i) {
    if (i != 0) {
      context->printer()->Print(", ");
    }
    values_[i]->Dump(context);
  }
  context->printer()->Print(")");
}

void Predicate::Dump(PrintContext* context) {
  switch (kind_) {
    case Kind::Equals:
      context->printer()->Print("=");
      break;
    case Kind::LessThan:
      context->printer()->Print("<");
      break;
  }
  context->printer()->Print("(");
  for (size_t i = 0; i < value_count_; ++i) {
    if (i != 0) {
      context->printer()->Print(", ");
    }
    values_[i]->Dump(context);
  }
  context->printer()->Print(")");
}

void HasType::Dump(PrintContext* context) {
  lhs_->Dump(context);
  context->printer()->Print(" :: ");
  rhs_->Dump(context);
}

void Conj::Dump(PrintContext* context) {
  lhs_->Dump(context);
  context->printer()->Print(" /\\ ");
  rhs_->Dump(context);
}

void Disj::Dump(PrintContext* context) {
  lhs_->Dump(context);
  context->printer()->Print(" \\/");
  rhs_->Dump(context);
}

void Not::Dump(PrintContext* context) {
  context->printer()->Print("not ");
  body_->Dump(context);
}

void Type::Dump(PrintContext* context) {
  context->printer()->Print("{");
  context->PushBinding();
  context->PrintAtIndex(0);
  context->printer()->Print("|");
  formula_->Dump(context);
  context->printer()->Print("}");
  context->PopBinding();
}

void Forall::Dump(PrintContext* context) {
  if (tyvar_count_) {
    context->printer()->Print("forall ");
    for (size_t t = 0; t < tyvar_count_; ++t) {
      context->PushBinding();
      context->PrintAtIndex(0);
    }
    context->printer()->Print(". ");
  }
  lhs_->Dump(context);
  context->PushBinding();
  context->printer()->Print(" -> ");
  context->PrintAtIndex(0);
  context->printer()->Print(". ");
  rhs_->Dump(context);
  context->PopBinding();
  for (size_t t = 0; t < tyvar_count_; ++t) {
    context->PopBinding();
  }
}

void Tyvar::Dump(PrintContext* context) { context->PrintAtIndex(index_); }

void Tyapp::Dump(PrintContext* context) {
  context->printer()->Print(context->symbol_table().text(ctor_));
  context->printer()->Print("[");
  for (size_t i = 0; i < term_count_; ++i) {
    if (i != 0) {
      context->printer()->Print(", ");
    }
    terms_[i]->Dump(context);
  }
  context->printer()->Print("]");
}

void App::Dump(PrintContext* context) {
  if (type_count_ != 0) {
    context->printer()->Print("[");
    for (size_t i = 0; i < type_count_; ++i) {
      if (i != 0) {
        context->printer()->Print(", ");
      }
      types_[i]->Dump(context);
    }
    context->printer()->Print("]");
  }
  lhs_->Dump(context);
  context->printer()->Print(" ");
  rhs_->Dump(context);
}

void If::Dump(PrintContext* context) {
  context->printer()->Print("if ");
  cond_->Dump(context);
  context->printer()->Print(" then ");
  then_->Dump(context);
  context->printer()->Print(" else ");
  elze_->Dump(context);
}

void Let::Dump(PrintContext* context) {
  context->printer()->Print("let ");
  val_->Dump(context);
  context->printer()->Print(" <- ");
  context->PushBinding();
  context->PrintAtIndex(0);
  context->printer()->Print(" in ");
  body_->Dump(context);
  context->PopBinding();
}

void Var::Dump(PrintContext* context) { context->PrintAtIndex(index_); }

void Lam::Dump(PrintContext* context) {
  context->PushBinding();
  context->printer()->Print("\\");
  context->PrintAtIndex(0);
  context->printer()->Print(".");
  exp_->Dump(context);
  context->PopBinding();
}

void Constant::Dump(PrintContext* context) {
  switch (kind_) {
    case Kind::True:
      context->printer()->Print("true");
      break;
    case Kind::False:
      context->printer()->Print("false");
      break;
    case Kind::Null:
      context->printer()->Print("null");
      break;
    case Kind::Number:
      context->printer()->Print(context->symbol_table().text(symbol_));
      break;
    case Kind::String:
      context->printer()->Print(context->symbol_table().text(symbol_));
      break;
    case Kind::Dict:
      context->printer()->Print("{}");
      break;
    case Kind::Tagof:
      context->printer()->Print("tagof");
      break;
    case Kind::Get:
      context->printer()->Print("get");
      break;
    case Kind::Fix:
      context->printer()->Print("fix");
      break;
  }
}

void Dict::Dump(PrintContext* context) {
  previous_->Dump(context);
  context->printer()->Print("[");
  key_->Dump(context);
  context->printer()->Print("=>");
  value_->Dump(context);
  context->printer()->Print("]");
}

void Instance::Dump(PrintContext* context) {
  context->printer()->Print(context->symbol_table().text(ctor_));
  context->printer()->Print("(");
  for (size_t i = 0; i < count_; ++i) {
    if (i != 0) {
      context->printer()->Print(", ");
    }
    values_[i]->Dump(context);
  }
  context->printer()->Print(")");
}

void Datatype::Dump(PrintContext* context) {
  context->printer()->Print("type ");
  context->printer()->Print(context->symbol_table().text(ctor_));
  if (tyvar_count_) {
    context->printer()->Print("[");
    for (size_t t = 0; t < tyvar_count_; ++t) {
      context->PushBinding();
      switch (variances_[t]) {
        case Variance::Covariant:
          context->printer()->Print("+");
          break;
        case Variance::Contravariant:
          context->printer()->Print("-");
          break;
        case Variance::Invariant:
          context->printer()->Print("=");
          break;
      }
      context->PrintAtIndex(0);
    }
    context->printer()->Print("] ");
  }
  context->printer()->Print(" = {");
  for (size_t f = 0; f < field_count_; ++f) {
    if (f != 0) {
      context->printer()->Print("; ");
      context->printer()->Print(context->symbol_table().text(field_names_[f]));
      context->printer()->Print(" : ");
      field_types_[f]->Dump(context);
    }
  }
  context->printer()->Print("}");
  for (size_t t = 0; t < tyvar_count_; ++t) {
    context->PopBinding();
  }
}
}
