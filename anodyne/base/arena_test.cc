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

#include "anodyne/base/arena.h"

#include "gtest/gtest.h"

namespace anodyne {
namespace {

struct Zero : public ArenaObject {};
struct One : public ArenaObject {
  uint8_t one;
};
struct Eight : public ArenaObject {
  uint64_t eight;
};
struct Nine : public ArenaObject {
  uint8_t nine[9];
};
struct Big : public ArenaObject {
  uint8_t big[1024 * 64];
};
struct Huge : public ArenaObject {
  uint8_t huge[1024 * 65];
};

static_assert(sizeof(Zero) == 1, "strange environment");
static_assert(sizeof(One) == 1, "strange environment");
static_assert(sizeof(Eight) == 8, "strange environment");
static_assert(sizeof(Nine) == 9, "strange environment");

constexpr size_t kPointerSize = sizeof(void*);
ptrdiff_t offset(void* a, void* b) {
  return static_cast<char*>(b) - static_cast<char*>(a);
}

TEST(ArenaTest, Allocate) {
  Arena arena;
  auto* zero = new (&arena) Zero();
  auto* one = new (&arena) One();
  auto* huge = new (&arena) Huge();
  auto* eight = new (&arena) Eight();
  auto* nine = new (&arena) Nine();
  EXPECT_EQ(1, arena.block_count());
  EXPECT_EQ(1, arena.huge_block_count());
  EXPECT_EQ(kPointerSize, offset(zero, one));
  EXPECT_EQ(2 * kPointerSize, offset(zero, eight));
  EXPECT_EQ(3 * kPointerSize, offset(zero, nine));
}

TEST(ArenaTest, AllocateBig) {
  Arena arena;
  auto* one = new (&arena) One();
  auto* big = new (&arena) Big();
  auto* another = new (&arena) One();
  // [one, ...] [big] [one...
  EXPECT_EQ(3, arena.block_count());
  EXPECT_EQ(0, arena.huge_block_count());
}

}  // anonymous namespace
}  // namespace anodyne
