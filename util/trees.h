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

#ifndef UTIL_TREES_H_
#define UTIL_TREES_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace st {

/// \brief Given a `SymbolTable`, uniquely identifies some string of text.
/// If two `Symbol`s are equal, their original text is equal.
typedef size_t Symbol;

/// \brief Maps strings to `Symbol`s.
class SymbolTable {
 public:
  /// \brief Returns the `Symbol` associated with `string`, or makes a new one.
  Symbol intern(const std::string &string) {
    const auto old = symbols_.find(string);
    if (old != symbols_.end()) {
      return old->second;
    }
    Symbol next_symbol = symbols_.size();
    symbols_[string] = next_symbol;
    // Note that references to elements of `unordered_map` are not invalidated
    // upon insert (really, upon rehash), so keeping around pointers in
    // `reverse_map_` is safe.
    reverse_map_.push_back(&symbols_.find(string)->first);
    return next_symbol;
  }
  /// \brief Returns the text associated with `symbol`.
  const std::string &text(Symbol symbol) const { return *reverse_map_[symbol]; }

 private:
  /// Maps text to unique `Symbol`s.
  std::unordered_map<std::string, Symbol> symbols_;
  /// Maps `Symbol`s back to their original text.
  std::vector<const std::string *> reverse_map_;
};

/// \brief Performs bump-pointer allocation of pointer-aligned memory.
///
/// AST nodes do not need to be deallocated piecemeal. The interpreter
/// does not permit uncontrolled mutable state: `EVars` are always unset
/// when history is rewound, so links to younger memory that have leaked out
/// into older memory at a choice point are severed when that choice point is
/// reconsidered. This means that entire swaths of memory can safely be
/// deallocated at once without calling individual destructors.
///
/// \warning Since `Arena`-allocated objects never have their destructors
/// called, any non-POD members they have will in turn never be destroyed.
class Arena {
 public:
  Arena() : next_block_index_(0) {}

  ~Arena() {
    for (auto &b : blocks_) {
      delete[] b;
    }
  }

  /// \brief Allocate `bytes` bytes, aligned to `kPointerSize`, allocating
  /// new blocks from the system if necessary.
  void *New(size_t bytes) {
    // Align to kPointerSize bytes.
    bytes = (bytes + kPointerSize - 1) & kPointerSizeMask;
    if (bytes >= kBlockSize) {
      abort();
    }
    offset_ += bytes;
    if (offset_ > kBlockSize) {
      if (next_block_index_ == blocks_.size()) {
        char *next_block = new char[kBlockSize];
        blocks_.push_back(next_block);
        current_block_ = next_block;
      } else {
        current_block_ = blocks_[next_block_index_];
      }
      ++next_block_index_;
      offset_ = bytes;
    }
    return current_block_ + offset_ - bytes;
  }

 private:
  /// The size of a pointer on this machine. We support only machines with
  /// power-of-two address size and alignment requirements.
  const size_t kPointerSize = sizeof(void *);
  /// `kPointerSize` (a power of two) sign-extended from its first set bit.
  const size_t kPointerSizeMask = ((~kPointerSize) + 1);
  /// The size of allocation requests to make from the normal heap.
  const size_t kBlockSize = 1024 * 64;

  /// The next offset in the current block to allocate. Should always be
  /// `<= kBlockSize`. If it is `== kBlockSize`, the current block is
  /// exhausted and the `Arena` moves on to the next block, allocating one
  /// if necessary.
  size_t offset_ = kBlockSize;
  /// The index of the next block to allocate from. Should always be
  /// `<= blocks_.size()`. If it is `== blocks_.size()`, a new block is
  /// allocated before the next `New` request completes.
  size_t next_block_index_;
  /// The block from which the `Arena` is currently making allocations. May
  /// be `nullptr` if no allocations have yet been made.
  char *current_block_;
  /// All blocks that the `Arena` has allocated so far.
  std::vector<char *> blocks_;
};

/// \brief An object that can be allocated inside an `Arena`.
class ArenaObject {
 public:
  void *operator new(size_t size, Arena *arena) { return arena->New(size); }
  void operator delete(void *, size_t) { abort(); }
  void operator delete(void *ptr, Arena *arena) { abort(); }
};
}  // namespace st

#endif  // UTIL_TREES_H_
