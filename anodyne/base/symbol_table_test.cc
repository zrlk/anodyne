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

#include "anodyne/base/symbol_table.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"

namespace anodyne {
namespace {

TEST(SymbolTable, Interns) {
  SymbolTable table;
  auto one = table.Intern("1");
  auto two = table.Intern("2");
  EXPECT_EQ(one, table.Intern("1"));
  EXPECT_NE(one, two);
  EXPECT_EQ("1", table.Text(one));
  EXPECT_EQ("2", table.Text(two));
  EXPECT_EQ("1", table.Expand(one));
  EXPECT_EQ("2", table.Expand(two));
  auto gen = table.Gensym();
  EXPECT_TRUE(table.is_gensym(gen));
  EXPECT_FALSE(table.is_gensym(one));
}

}  // anonymous namespace
}  // namespace anodyne
