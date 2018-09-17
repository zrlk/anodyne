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
TEST(MemFs, Overwrites) {
  MemoryFileSystem memfs;
  EXPECT_TRUE(memfs.InsertFile("foo", "foo").ok());
  EXPECT_TRUE(memfs.InsertDirectory("dir").ok());
  EXPECT_TRUE(memfs.InsertDirectory("dir").ok());
  EXPECT_TRUE(memfs.InsertFile("foo", "bar").ok());
  auto bar = memfs.GetFileContent("foo");
  EXPECT_TRUE(bar && *bar == "bar");
  EXPECT_FALSE(memfs.InsertDirectory("foo").ok());
  EXPECT_FALSE(memfs.InsertFile("dir", "dir").ok());
}

TEST(MemFs, StoresFiles) {
  MemoryFileSystem memfs;
  EXPECT_TRUE(memfs.InsertFile("foo", "bar").ok());
  EXPECT_TRUE(memfs.InsertDirectory("dir").ok());
  EXPECT_TRUE(memfs.SetWorkingDirectory("dir").ok());
  EXPECT_TRUE(memfs.InsertFile("three", "four").ok());
  auto four = memfs.GetFileContent("three");
  auto four_abs = memfs.GetFileContent("/dir/three");
  auto bar = memfs.GetFileContent("../foo");
  auto bar_abs = memfs.GetFileContent("/foo");
  auto none = memfs.GetFileContent("none");
  auto dir = memfs.GetFileKind(".");
  auto dir_abs = memfs.GetFileKind("/dir");
  EXPECT_TRUE(!none);
  EXPECT_TRUE(four && *four == "four");
  EXPECT_TRUE(four_abs && *four_abs == "four");
  EXPECT_TRUE(bar && *bar == "bar");
  EXPECT_TRUE(bar_abs && *bar_abs == "bar");
  EXPECT_TRUE(dir && *dir == FileKind::kDirectory);
  EXPECT_TRUE(dir_abs && *dir_abs == FileKind::kDirectory);
}

}  // anonymous namespace
}  // namespace anodyne
