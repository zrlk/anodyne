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

#ifndef AST_SYSTEMD_H_
#define AST_SYSTEMD_H_

#include <functional>
#include "util/location.h"
#include "util/pretty_printer.h"
#include "util/trees.h"

/// System D from Ravi Chugh's dissertation, "Nested Refinement Types for
/// JavaScript". The dissertation uses an A-normal form presentation that
/// we preserve. We also use a locally nameless representation for variable
/// binding.
/// TODO: Algorithmic typing extensions; System !D extensions;
///       Cheney-scan collection support; Prettier printing (requires layout).
namespace systemd {
using Location = util::SourceRange;
using Symbol = st::Symbol;

class PrintContext {
 public:
  virtual const st::SymbolTable& symbol_table() = 0;
  virtual st::PrettyPrinter* printer() = 0;
  virtual void PrintAtIndex(unsigned index) = 0;
  virtual void PushBinding() = 0;
  virtual void PopBinding() = 0;
};

class Value;
class TypeTerm;

class Node : public st::ArenaObject {
 public:
  explicit Node(const Location& location) : location_(location) {}
  virtual void Dump(PrintContext* context) {}

 private:
  Location location_;
};

class Logical : public Node {
 public:
  explicit Logical(const Location& location) : Node(location) {}
};

class LogicalValue : public Logical {
 public:
  LogicalValue(const Location& location, Value* val)
      : Logical(location), val_(val) {}
  virtual void Dump(PrintContext* context);

 private:
  Value* val_;
};

class LogicalApp : public Logical {
 public:
  enum class FunctionSymbol { Tag, Sel, Upd, Plus };
  LogicalApp(const Location& location, FunctionSymbol symbol,
             size_t value_count, Logical** values)
      : Logical(location),
        symbol_(symbol),
        value_count_(value_count),
        values_(values) {}
  virtual void Dump(PrintContext* context);

 private:
  FunctionSymbol symbol_;
  size_t value_count_;
  Logical** values_;
};

class Formula : public Node {
 public:
  explicit Formula(const Location& location) : Node(location) {}
};

class Predicate : public Formula {
 public:
  enum class Kind { Equals, LessThan };
  Predicate(const Location& location, Kind kind, size_t value_count,
            Logical** values)
      : Formula(location),
        kind_(kind),
        value_count_(value_count),
        values_(values) {}
  virtual void Dump(PrintContext* context);

 private:
  Kind kind_;
  size_t value_count_;
  Logical** values_;
};

class HasType : public Formula {
 public:
  HasType(const Location& location, Logical* lhs, TypeTerm* rhs)
      : Formula(location), lhs_(lhs), rhs_(rhs) {}
  virtual void Dump(PrintContext* context);

 private:
  Logical* lhs_;
  TypeTerm* rhs_;
};

class Conj : public Formula {
 public:
  Conj(const Location& location, Formula* lhs, Formula* rhs)
      : Formula(location), lhs_(lhs), rhs_(rhs) {}
  virtual void Dump(PrintContext* context);

 private:
  Formula* lhs_;
  Formula* rhs_;
};

class Disj : public Formula {
 public:
  Disj(const Location& location, Formula* lhs, Formula* rhs)
      : Formula(location), lhs_(lhs), rhs_(rhs) {}
  virtual void Dump(PrintContext* context);

 private:
  Formula* lhs_;
  Formula* rhs_;
};

class Not : public Formula {
 public:
  Not(const Location& location, Formula* body)
      : Formula(location), body_(body) {}
  virtual void Dump(PrintContext* context);

 private:
  Formula* body_;
};

class TypeTerm : public Node {
 public:
  explicit TypeTerm(const Location& location) : Node(location) {}
};

class Type : public Node {
 public:
  explicit Type(const Location& location, Formula* formula)
      : Node(location), formula_(formula) {}
  virtual void Dump(PrintContext* context);

 private:
  Formula* formula_;
};

class Forall : public TypeTerm {
 public:
  Forall(const Location& location, size_t tyvar_count, Type* lhs, Type* rhs)
      : TypeTerm(location), tyvar_count_(tyvar_count), lhs_(lhs), rhs_(rhs) {}
  virtual void Dump(PrintContext* context);

 private:
  size_t tyvar_count_;
  Type* lhs_;
  Type* rhs_;
};

class Tyvar : public TypeTerm {
 public:
  Tyvar(const Location& location, unsigned index)
      : TypeTerm(location), index_(index) {}
  virtual void Dump(PrintContext* context);

 private:
  unsigned index_;
};

class Tyapp : public TypeTerm {
 public:
  Tyapp(const Location& location, Symbol ctor, size_t term_count,
        TypeTerm** terms)
      : TypeTerm(location),
        ctor_(ctor),
        term_count_(term_count),
        terms_(terms) {}
  virtual void Dump(PrintContext* context);

 private:
  Symbol ctor_;
  size_t term_count_;
  TypeTerm** terms_;
};

class Exp : public Node {
 public:
  explicit Exp(const Location& location) : Node(location) {}
};

class App : public Exp {
 public:
  App(const Location& location, size_t type_count, Type** types, Value* lhs,
      Value* rhs)
      : Exp(location),
        type_count_(type_count),
        types_(types),
        lhs_(lhs),
        rhs_(rhs) {}
  void Dump(PrintContext* context) override;

 private:
  size_t type_count_;
  Type** types_;
  Value* lhs_;
  Value* rhs_;
};

class If : public Exp {
 public:
  If(const Location& location, Value* cond, Exp* then, Exp* elze)
      : Exp(location), cond_(cond), then_(then), elze_(elze) {}
  void Dump(PrintContext* context) override;

 private:
  Value* cond_;
  Exp* then_;
  Exp* elze_;
};

class Let : public Exp {
 public:
  Let(const Location& location, Exp* val, Exp* body)
      : Exp(location), val_(val), body_(body) {}
  void Dump(PrintContext* context) override;

 private:
  Exp* val_;
  Exp* body_;
};

class Value : public Exp {
 public:
  explicit Value(const Location& location) : Value(location) {}
};

class Var : public Value {
 public:
  Var(const Location& location, unsigned index)
      : Value(location), index_(index) {}
  void Dump(PrintContext* context) override;

 private:
  unsigned index_;
};

class Lam : public Value {
 public:
  Lam(const Location& location, Exp* exp) : Value(location), exp_(exp) {}
  void Dump(PrintContext* context) override;

 private:
  Exp* exp_;
};

class Constant : public Value {
 public:
  enum class Kind { True, False, Null, Number, String, Dict, Tagof, Get, Fix };
  Constant(const Location& location, Kind kind)
      : Value(location), kind_(kind) {}
  Constant(const Location& location, Kind kind, Symbol symbol)
      : Value(location), kind_(kind), symbol_(symbol) {}
  void Dump(PrintContext* context) override;

 private:
  Kind kind_;
  Symbol symbol_;
};

class Dict : public Value {
 public:
  Dict(const Location& location, Value* previous, Value* key, Value* value)
      : Value(location), previous_(previous), key_(key), value_(value) {}
  void Dump(PrintContext* context) override;

 private:
  Value* previous_;
  Value* key_;
  Value* value_;
};

class Instance : public Value {
 public:
  Instance(const Location& location, Symbol ctor, size_t count, Value** values)
      : Value(location), ctor_(ctor), count_(count), values_(values) {}
  void Dump(PrintContext* context) override;

 private:
  Symbol ctor_;
  size_t count_;
  Value** values_;
};

class Datatype : public Node {
 public:
  enum class Variance { Covariant, Contravariant, Invariant };
  Datatype(const Location& location, Symbol ctor, size_t tyvar_count,
           Variance* variances, size_t field_count, Symbol* field_names,
           Type** field_types)
      : Node(location),
        ctor_(ctor),
        tyvar_count_(tyvar_count),
        variances_(variances),
        field_count_(field_count),
        field_names_(field_names_),
        field_types_(field_types_) {}
  void Dump(PrintContext* context) override;

 private:
  Symbol ctor_;
  size_t tyvar_count_;
  Variance* variances_;
  size_t field_count_;
  Symbol* field_names_;
  Type** field_types_;
};

class Sugar {
 public:
  Sugar(st::Arena* arena) : arena_(arena) {}

 private:
  st::Arena* arena_;
};
}

#endif
