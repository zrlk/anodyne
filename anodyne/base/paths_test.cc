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

#include "anodyne/base/paths.h"

#include "gtest/gtest.h"

namespace anodyne {
namespace {

std::string Flat(const absl::optional<Path> path) {
  return path ? path->get() : "0";
}

TEST(PathsTest, Clean) {
  EXPECT_EQ("/a/c", Flat(Path::Clean("/../../a/c")));
  EXPECT_EQ("", Flat(Path::Clean("")));
  EXPECT_EQ(" ", Flat(Path::Clean(" ")));
  EXPECT_EQ("/", Flat(Path::Clean("/")));
  EXPECT_EQ("/", Flat(Path::Clean("/./")));
  EXPECT_EQ("a/c", Flat(Path::Clean("a/c")));
  EXPECT_EQ("a/c", Flat(Path::Clean("a//c")));
  EXPECT_EQ("a/c", Flat(Path::Clean("a/c/.")));
  EXPECT_EQ("a/c", Flat(Path::Clean("a/c/b/..")));
  EXPECT_EQ("/a/c", Flat(Path::Clean("/../a/c")));
  EXPECT_EQ("/a/c", Flat(Path::Clean("/../a/b/../././/c")));
  EXPECT_EQ("/a/c", Flat(Path::Clean("/../a/b/../././//c")));
  EXPECT_EQ("/Users", Flat(Path::Clean("/Users")));
  EXPECT_EQ("/foo/bar.cc", Flat(Path::Clean("/foo/./bar.cc")));
}

TEST(PathsTest, Parent) {
  EXPECT_EQ("0", Flat(Path::Clean("").Parent()));
  EXPECT_EQ("0", Flat(Path::Clean("/").Parent()));
  EXPECT_EQ("/", Flat(Path::Clean("/foo").Parent()));
  EXPECT_EQ("/foo", Flat(Path::Clean("/foo/bar").Parent()));
  EXPECT_EQ("", Flat(Path::Clean("foo").Parent()));
  EXPECT_EQ("foo", Flat(Path::Clean("foo/bar").Parent()));
}

TEST(PathsTest, Relativize) {
  EXPECT_EQ("0", Flat(Path::Clean("foo").Relativize(Path::Clean("/foo"))));
  EXPECT_EQ("0", Flat(Path::Clean("/foo").Relativize(Path::Clean("foo"))));
  EXPECT_EQ("", Flat(Path::Clean("/foo").Relativize(Path::Clean("/foo"))));
  EXPECT_EQ("bar",
            Flat(Path::Clean("/foo").Relativize(Path::Clean("/foo/bar"))));
  EXPECT_EQ("0", Flat(Path::Clean("/foo").Relativize(Path::Clean("/foobar"))));
  EXPECT_EQ("bar/baz",
            Flat(Path::Clean("/foo").Relativize(Path::Clean("/foo/bar/baz"))));
  EXPECT_EQ(
      "baz",
      Flat(Path::Clean("/foo/bar").Relativize(Path::Clean("/foo/bar/baz"))));
}

TEST(PathsTest, Concat) {
  EXPECT_EQ("0", Flat(Path::Clean("").Concat("/")));
  EXPECT_EQ("/", Flat(Path::Clean("/").Concat("")));
  EXPECT_EQ("/", Flat(Path::Clean("/").Concat("./")));
  EXPECT_EQ("/foo/bar.cc", Flat(Path::Clean("/foo").Concat("./bar.cc")));
  EXPECT_EQ("/foo", Flat(Path::Clean("/").Concat("foo")));
  EXPECT_EQ("foo/bar", Flat(Path::Clean("foo").Concat("bar")));
}
}  // anonymous namespace
}  // namespace anodyne
