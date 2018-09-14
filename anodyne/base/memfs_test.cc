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

#include "anodyne/base/memfs.h"
#include "gtest/gtest.h"

namespace anodyne {
namespace {

TEST(MemFs, StoresFiles) {
  MemoryFileSystem memfs;
  EXPECT_TRUE(memfs.InsertFile("foo", "bar").ok());
  memfs.SetWorkingDirectory("dir");
  EXPECT_TRUE(memfs.InsertFile("three", "four").ok());
  auto four = memfs.GetFileContent("three");
  auto four_abs = memfs.GetFileContent("/dir/three");
  auto bar = memfs.GetFileContent("../foo");
  auto bar_abs = memfs.GetFileContent("/foo");
  auto none = memfs.GetFileContent("none");
  EXPECT_TRUE(!none);
  EXPECT_TRUE(four && *four == "four");
  EXPECT_TRUE(four_abs && *four_abs == "four");
  EXPECT_TRUE(bar && *bar == "bar");
  EXPECT_TRUE(bar_abs && *bar_abs == "bar");
}

}  // anonymous namespace
}  // namespace anodyne
