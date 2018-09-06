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

#ifndef ANODYNE_BASE_CONTEXT_H__
#define ANODYNE_BASE_CONTEXT_H__

#include "anodyne/base/arena.h"
#include "anodyne/base/source.h"
#include "anodyne/base/symbol_table.h"

#include <memory>

namespace anodyne {

class Context {
 public:
  Context() {}
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  Arena* arena() { return &arena_; }
  SymbolTable* symbol_table() { return &symbol_table_; }
  const SymbolTable* symbol_table() const { return &symbol_table_; }
  Source* source() { return &source_; }
  const Source* source() const { return &source_; }
  /// \return the current thread's currently-bound context.
  static Context* Current() { return current_; }

 private:
  friend class ContextBinding;

  Arena arena_;
  Source source_;
  SymbolTable symbol_table_;
  static thread_local Context* current_;
};

/// \brief Binds the current thread to the given context for writing.
class ContextBinding {
 public:
  ContextBinding(Context* context) : previous_context_(Context::current_) {
    Context::current_ = context;
  }
  ~ContextBinding() { Context::current_ = previous_context_; }
  ContextBinding(const ContextBinding&) = delete;
  ContextBinding& operator=(const ContextBinding&) = delete;

 private:
  Context* previous_context_;
};

}  // namespace anodyne

#endif  // ANODYNE_BASE_CONTEXT_H_
