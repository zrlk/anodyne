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

#ifndef ANODYNE_BASE_ARENA_H_
#define ANODYNE_BASE_ARENA_H_

#include <cstddef>
#include <cstdlib>
#include <vector>

namespace anodyne {

/// \brief Performs bump-pointer allocation of pointer-aligned memory.
/// \warning Since `Arena`-allocated objects never have their destructors
/// called, any non-POD members they have will in turn never be destroyed.
class Arena {
 public:
  Arena() : next_block_index_(0) {}
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;
  ~Arena() {
    for (auto& b : blocks_) {
      delete[] b;
    }
  }

  /// \brief Allocate `bytes` bytes, aligned to `kPointerSize`, allocating
  /// new blocks from the system if necessary.
  void* New(size_t bytes) {
    // Align to kPointerSize bytes.
    bytes = (bytes + kPointerSize - 1) & kPointerSizeMask;
    if (bytes > kBlockSize) {
      char* huge_block = new char[bytes];
      huge_blocks_.push_back(huge_block);
      return huge_block;
    }
    offset_ += bytes;
    if (offset_ > kBlockSize) {
      if (next_block_index_ == blocks_.size()) {
        char* next_block = new char[kBlockSize];
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

  /// \return the number of normal blocks in use.
  const size_t block_count() const { return blocks_.size(); }
  /// \return the number of huge blocks in use.
  const size_t huge_block_count() const { return huge_blocks_.size(); }

 private:
  friend class ContextBinding;
  /// The size of a pointer on this machine. We support only machines with
  /// power-of-two address size and alignment requirements.
  const size_t kPointerSize = sizeof(void*);
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
  char* current_block_;
  /// All blocks that the `Arena` has allocated so far.
  std::vector<char*> blocks_;
  /// All huge blocks that the `Arena` has allocated so far.
  std::vector<char*> huge_blocks_;
};

/// \brief An object that can be allocated inside an `Arena`.
class ArenaObject {
 public:
  void* operator new(size_t size, Arena* arena) { return arena->New(size); }
  void operator delete(void*, size_t) { abort(); }
  void operator delete(void* ptr, Arena* arena) { abort(); }
};

/// \brief An array of references that can be allocated inside an `Arena`.
template <typename T>
class ArenaSlice {
 public:
  ArenaSlice(size_t length, const T** contents)
      : length_(length), contents_(contents) {}
  ArenaSlice() {}
  const T* operator[](size_t index) const { return contents_[index]; }
  size_t size() const { return length_; }

 private:
  size_t length_ = 0;
  const T** contents_ = nullptr;
};

/// \brief A reference that may or may not be occupied.
template <typename T>
class ArenaOption {
 public:
  explicit ArenaOption(const T* content) : content_(content) {}
  bool is_some() const { return content_ != nullptr; }
  const T* get() const { return content_; }

 private:
  const T* content_ = nullptr;
};

template <typename T>
ArenaOption<T> Some(const T* content) {
  return ArenaOption<T>(content);
}
template <typename T>
ArenaOption<T> None() {
  return ArenaOption<T>(nullptr);
}

}  // namespace anodyne

#endif  // ANODYNE_BASE_ARENA_H_
