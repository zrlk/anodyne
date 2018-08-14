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

#ifndef ANODYNE_BASE_SYMBOL_TABLE_H_
#define ANODYNE_BASE_SYMBOL_TABLE_H_

#include "absl/strings/string_view.h"
#include "anodyne/base/arena.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace anodyne {

/// \brief a symbol in some SymbolTable.
using Symbol = uint32_t;

/// \brief Interns strings of bytes.
class SymbolTable {
 public:
  SymbolTable() {}
  SymbolTable(const SymbolTable&) = delete;
  SymbolTable& operator=(const SymbolTable&) = delete;
  /// \brief Given a non-gensym `Symbol`, return a string that can be used to
  /// look it up.
  absl::string_view Text(Symbol symbol) const {
    assert(symbol < kGensymBase);
    return symbol_text_[symbol];
  }
  /// \return whether `symbol` was generated with `Gensym`.
  bool is_gensym(Symbol symbol) const { return symbol >= kGensymBase; }
  /// \return a text equivalent for `symbol`.
  std::string Expand(Symbol symbol) const {
    if (is_gensym(symbol)) {
      return "gensym-" + std::to_string(symbol);
    } else {
      return std::string(Text(symbol));
    }
  }
  /// \brief Returns the `Symbol` equivalent for `text`.
  Symbol Intern(absl::string_view text) {
    assert(symbol_text_.size() < kGensymBase);
    const auto it = symbol_map_.find(std::string(text));
    if (it != symbol_map_.end()) {
      return it->second;
    }
    symbol_map_[std::string(text)] = static_cast<Symbol>(symbol_text_.size());
    symbol_text_.push_back(std::string(text));
    return symbol_text_.size() - 1;
  }
  /// \brief Return a symbol guaranteed to never match one of the other symbols
  /// in this `SymbolTable`.
  Symbol Gensym() { return gensym_++; }

 private:
  std::unordered_map<std::string, Symbol> symbol_map_;
  std::vector<std::string> symbol_text_;
  static constexpr uint32_t kGensymBase = 0x80000000;
  uint32_t gensym_ = kGensymBase;
};

/// \brief Specialization for storing slices of Symbols (which aren't
/// `const T*`).
template <>
class ArenaSlice<Symbol> {
 public:
  ArenaSlice(size_t length, const Symbol* contents)
      : length_(length), contents_(contents) {}
  ArenaSlice() {}
  Symbol operator[](size_t index) const { return contents_[index]; }
  size_t size() const { return length_; }

 private:
  size_t length_ = 0;
  const Symbol* contents_ = nullptr;
};

/// \brief Specialization for storing options of Symbols (which aren't
/// `const T*`).
template <>
class ArenaOption<Symbol> {
 public:
  ArenaOption(bool is_some, Symbol content)
      : is_some_(is_some), content_(content) {}
  bool is_some() const { return is_some_; }
  Symbol get() const { return content_; }

 private:
  bool is_some_;
  Symbol content_;
};

inline ArenaOption<Symbol> Some(Symbol content) {
  return ArenaOption<Symbol>(true, content);
}

inline ArenaOption<Symbol> None() { return ArenaOption<Symbol>(false, 0); }
}  // namespace anodyne

#endif  // ANODYNE_BASE_SYMBOL_TABLE_H_
