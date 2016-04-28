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

#ifndef AST_LUA_H_
#define AST_LUA_H_

#include <string>
#include "util/location.h"
#include "util/pretty_printer.h"
#include "util/trees.h"

namespace lua {

using Location = util::SourceRange;
using Symbol = st::Symbol;

class Var;
class Tuple;
class ElseIf;
class If;
class Literal;
class ArgsReference;  // ...
class Function;
class Index;
class DirectIndex;
class UnaryOp;
class BinaryOp;
class Call;
class FunctionBinding;
class VarBinding;
class Block;
class Field;
class TableConstructor;
class While;

class Node : public st::ArenaObject {
 public:
  explicit Node(const Location &location) : location_(location) {}

  /// \brief Returns the location where the `Node` was found if it came
  /// from source text.
  const Location &location() const { return location_; }

  /// \brief Dumps the `Node` to `printer`.
  virtual void Dump(const st::SymbolTable &symbol_table,
                    st::PrettyPrinter *printer) {}

  virtual Var *AsVar() { return nullptr; }
  virtual Tuple *AsTuple() { return nullptr; }
  virtual ElseIf *AsElseIf() { return nullptr; }
  virtual If *AsIf() { return nullptr; }
  virtual Literal *AsLiteral() { return nullptr; }
  virtual ArgsReference *AsArgsReference() { return nullptr; }
  virtual Function *AsFunction() { return nullptr; }
  virtual Index *AsIndex() { return nullptr; }
  virtual DirectIndex *AsDirectIndex() { return nullptr; }
  virtual UnaryOp *AsUnaryOp() { return nullptr; }
  virtual BinaryOp *AsBinaryOp() { return nullptr; }
  virtual Call *AsCall() { return nullptr; }
  virtual FunctionBinding *AsFunctionBinding() { return nullptr; }
  virtual VarBinding *AsVarBinding() { return nullptr; }
  virtual Block *AsBlock() { return nullptr; }
  virtual Field *AsField() { return nullptr; }
  virtual TableConstructor *AsTableConstructor() { return nullptr; }
  virtual While *AsWhile() { return nullptr; }

 private:
  /// \brief The location where the `Node` can be found in source text, if any.
  Location location_;
};

class Block : public Node {
 public:
  enum class Kind { ReturnNone, ReturnExp, Break, NoTerminator };
  Block(const Location &location, Tuple *stmts,
        const std::pair<lua::Tuple *, lua::Block::Kind> &last_stat)
      : Node(location),
        stmts_(stmts),
        kind_(last_stat.second),
        return_exps_(last_stat.first) {}
  Block *AsBlock() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;
  Kind kind() const { return kind_; }
  Tuple *stmts() const { return stmts_; }

 private:
  Kind kind_;
  Tuple *stmts_;
  Tuple *return_exps_;
};

class While : public Node {
 public:
  While(const Location &location, Node *condition, Node *block)
      : Node(location), condition_(condition), block_(block) {}
  While *AsWhile() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Node *condition_;
  Node *block_;
};

class ElseIf : public Node {
 public:
  ElseIf(const Location &location, Node *exp, Node *block)
      : Node(location), exp_(exp), block_(block) {}
  ElseIf *AsElseIf() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;
  Node *exp() const { return exp_; }
  Node *block() const { return block_; }

 private:
  Node *exp_;
  Node *block_;
};

class Function : public Node {
 public:
  Function(const Location &location, Tuple *bindings, bool varargs, Node *body)
      : Node(location), bindings_(bindings), varargs_(varargs), body_(body) {}
  Function *AsFunction() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Tuple *bindings_;
  bool varargs_;
  Node *body_;
};

class FunctionBinding : public Node {
 public:
  enum class Kind { Local, Global, GlobalMember };
  FunctionBinding(const Location &location, Symbol local_name, Node *body)
      : Node(location), kind_(Kind::Local), symbol_(local_name), body_(body) {}
  FunctionBinding(const Location &location, Tuple *path, Node *body)
      : Node(location), kind_(Kind::Global), path_(path), body_(body) {}
  FunctionBinding(const Location &location, Tuple *path, Symbol member,
                  Node *body)
      : Node(location),
        kind_(Kind::GlobalMember),
        path_(path),
        symbol_(member),
        body_(body) {}

  FunctionBinding *AsFunctionBinding() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Kind kind_;
  Tuple *path_;
  Symbol symbol_;
  Node *body_;
};

class VarBinding : public Node {
 public:
  VarBinding(const Location &location, bool local, Tuple *vars, Tuple *inits)
      : Node(location), local_(local), vars_(vars), inits_(inits) {}
  VarBinding *AsVarBinding() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  bool local_;
  Tuple *vars_;
  Tuple *inits_;
};

class If : public Node {
 public:
  If(const Location &location, Node *exp, Node *then, Tuple *elseives,
     Node *elze)
      : Node(location),
        exp_(exp),
        elseives_(elseives),
        then_(then),
        else_(elze) {}
  If *AsIf() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;
  Node *exp() const { return exp_; }
  Tuple *elseives() const { return elseives_; }
  Node *then() const { return then_; }
  Node *elze() const { return else_; }

 private:
  Node *exp_;
  Tuple *elseives_;
  Node *then_;
  Node *else_;
};

/// \brief A tuple of zero or more elements.
class Tuple : public Node {
 public:
  /// \brief Constructs a new `Tuple`
  /// \param location Mark with this location
  /// \param element_count The number of elements in the tuple
  /// \param elements A preallocated buffer of `Node*` such that
  /// the total size of the buffer is equal to
  /// `element_count * sizeof(Node *)`
  Tuple(const Location &location, size_t element_count, Node **elements)
      : Node(location), element_count_(element_count), elements_(elements) {}
  Tuple *AsTuple() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;
  void DumpAsStmts(const st::SymbolTable &, st::PrettyPrinter *);
  /// \brief Returns the number of elements in the `Tuple`.
  size_t size() const { return element_count_; }
  /// \brief Returns the `index`th element of the `Tuple`, counting from zero.
  Node *element(size_t index) const {
    if (index >= element_count_) {
      abort();
    }
    return elements_[index];
  }
  Node **begin() const { return &elements_[0]; }
  Node **end() const { return &elements_[element_count_]; }

 private:
  /// The number of `Node *`s in `elements_`
  size_t element_count_;
  /// Storage for the `Tuple`'s elements.
  Node **elements_;
};

class Call : public Node {
 public:
  Call(const Location &location, Node *function, Tuple *args)
      : Node(location), function_(function), args_(args), member_(false) {}
  Call(const Location &location, Node *function, Tuple *args,
       Symbol member_symbol)
      : Node(location),
        function_(function),
        args_(args),
        member_(true),
        symbol_(member_symbol) {}
  Call *AsCall() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Node *function_;
  Tuple *args_;
  bool member_;
  Symbol symbol_;
};

class Var : public Node {
 public:
  Var(const Location &location, Symbol symbol)
      : Node(location), symbol_(symbol) {}
  Symbol symbol() const { return symbol_; }
  Var *AsVar() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Symbol symbol_;
};

class Literal : public Node {
 public:
  enum class Type {
    String,
    Number,
    Nil,
    True,
    False,
  };

  Literal(const Location &location, Type type) : Node(location), type_(type) {}
  Literal(const Location &location, Type type, Symbol symbol)
      : Node(location), type_(type), symbol_(symbol) {}
  Type type() const { return type_; }
  Symbol symbol() const { return symbol_; }
  Literal *AsLiteral() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Type type_;
  Symbol symbol_;
};

class ArgsReference : public Node {
 public:
  explicit ArgsReference(const Location &location) : Node(location) {}
  ArgsReference *AsArgsReference() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;
};

class Index : public Node {
 public:
  Index(const Location &location, Node *lhs, Node *subscript)
      : Node(location), lhs_(lhs), subscript_(subscript) {}
  Index *AsIndex() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Node *lhs_;
  Node *subscript_;
};

class DirectIndex : public Node {
 public:
  DirectIndex(const Location &location, Node *lhs, Symbol subscript)
      : Node(location), lhs_(lhs), subscript_(subscript) {}
  DirectIndex *AsDirectIndex() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Node *lhs_;
  Symbol subscript_;
};

class UnaryOp : public Node {
 public:
  enum class Op { Not, Length, Negate };
  UnaryOp(const Location &location, Op op, Node *operand)
      : Node(location), op_(op), operand_(operand) {}
  UnaryOp *AsUnaryOp() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Op op_;
  Node *operand_;
};

class BinaryOp : public Node {
 public:
  enum class Op {
    Or,
    And,
    LessThan,
    LessThanEqual,
    GreaterThan,
    GreaterThanEqual,
    NotEqual,
    Equal,
    Concatenate,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Exponent
  };
  BinaryOp(const Location &location, Op op, Node *lhs, Node *rhs)
      : Node(location), op_(op), lhs_(lhs), rhs_(rhs) {}
  BinaryOp *AsBinaryOp() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Op op_;
  Node *lhs_;
  Node *rhs_;
};

class TableConstructor : public Node {
 public:
  TableConstructor(const Location &location, Tuple *fields)
      : Node(location), fields_(fields) {}
  TableConstructor *AsTableConstructor() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Tuple *fields_;
};

class Field : public Node {
 public:
  enum class Kind { Exp, Label, Bracket };
  Field(const Location &location, Node *exp)
      : Node(location), kind_(Kind::Exp), exp_(exp) {}
  Field(const Location &location, Symbol symbol, Node *exp)
      : Node(location), kind_(Kind::Label), exp_(exp), symbol_(symbol) {}
  Field(const Location &location, Node *index, Node *exp)
      : Node(location), kind_(Kind::Bracket), exp_(exp), index_(index) {}

  Field *AsField() override { return this; }
  void Dump(const st::SymbolTable &, st::PrettyPrinter *) override;

 private:
  Kind kind_;
  Node *exp_;
  Symbol symbol_;
  Node *index_;
};
}

#endif  // AST_LUA_H_
