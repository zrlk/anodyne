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

#include "anodyne/base/source_buffer.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"

namespace anodyne {
namespace {

// The code to generate these example maps and generated strings is in
// //anodyne/tools/source_map_test_gen

// These are the result of making substitutions in the various source
// files under //anodyne/tools/source_map_test_gen.
constexpr char kAsciiOnlyGen[] = R"(S1Big
text text
S2Big)";
constexpr char kUtf162UGen[] = "s x s";
constexpr char kUtf82BGen[] = "s ¢2 s";
constexpr char kUtf83BGen[] = "s €3 s";
constexpr char kUtf84BGen[] = "s 𐐷4 s";

// These map the strings above to their original text.
constexpr char kAsciiOnly[] =
    R"({"version":3,"sources":["ascii_only"],"names":["s1","s2"],"mappings":"AAAAA,KAAE;AACF,IAAI,CAAC,IAAI;AACTC,KAAE","file":"ascii_only.map"})";
constexpr char kUtf162U[] =
    R"({"version":3,"sources":["utf16_2u"],"names":["r","𐐷"],"mappings":"AAAAA,CAAC,CAACC,CAAE,CAACD,CAAC;AACN","file":"utf16_2u.map"})";
constexpr char kUtf82B[] =
    R"({"version":3,"sources":["utf8_2b"],"names":["r","u2"],"mappings":"AAAAA,CAAC,CAACC,EAAE,CAACD,CAAC;AACN","file":"utf8_2b.map"})";
constexpr char kUtf83B[] =
    R"({"version":3,"sources":["utf8_3b"],"names":["r","u3"],"mappings":"AAAAA,CAAC,CAACC,EAAE,CAACD,CAAC;AACN","file":"utf8_3b.map"})";
constexpr char kUtf84B[] =
    R"({"version":3,"sources":["utf8_4b"],"names":["r","u4"],"mappings":"AAAAA,CAAC,CAACC,GAAE,CAACD,CAAC;AACN","file":"utf8_4b.map"})";

TEST(SourceBufferTest, AsciiOnly) {
  SourceMap map;
  ASSERT_TRUE(map.ParseFromJson("ascii_only", kAsciiOnly, true));
  SourceBuffer buffer(absl::string_view(kAsciiOnlyGen), std::move(map));
  EXPECT_EQ(0, buffer.OffsetForUtf16Offset(0));
  EXPECT_EQ(16, buffer.OffsetForUtf16Offset(16));
  EXPECT_EQ(21, buffer.OffsetForUtf16Offset(21));
  const auto* sub_segment = buffer.SegmentForOffset(16);
  ASSERT_FALSE(sub_segment == nullptr);
  EXPECT_EQ(0, sub_segment->generated_col);
  EXPECT_EQ(2, sub_segment->generated_line);
  EXPECT_EQ(0, sub_segment->source_col);
  EXPECT_EQ(2, sub_segment->source_line);
  EXPECT_EQ(std::make_pair(0, 0), buffer.Utf8LineColForOffset(0));
  EXPECT_EQ(std::make_pair(0, 1), buffer.Utf8LineColForOffset(1));
  EXPECT_EQ(std::make_pair(0, 5), buffer.Utf8LineColForOffset(5));
  EXPECT_EQ(std::make_pair(0, 6), buffer.Utf8LineColForOffset(6));
  EXPECT_EQ(std::make_pair(1, 1), buffer.Utf8LineColForOffset(8));
  EXPECT_EQ(std::make_pair(2, 4), buffer.Utf8LineColForOffset(21));
  EXPECT_EQ(std::make_pair(-1, -1), buffer.Utf8LineColForOffset(22));
}

TEST(SourceBufferTest, Utf162U) {
  SourceMap map;
  ASSERT_TRUE(map.ParseFromJson("utf16_2u", kUtf162U, true));
  SourceBuffer buffer(absl::string_view(kUtf162UGen), std::move(map));
  // The source file has a two-UTF-16-code-unit character at column 2.
  const auto* two_unit = buffer.SegmentForOffset(2);
  const auto* post_two_unit = buffer.SegmentForOffset(3);
  ASSERT_FALSE(two_unit == nullptr);
  ASSERT_FALSE(post_two_unit == nullptr);
  EXPECT_EQ(2, two_unit->generated_col);
  EXPECT_EQ(2, two_unit->source_col);
  EXPECT_EQ(3, post_two_unit->generated_col);
  EXPECT_EQ(4, post_two_unit->source_col);
}

TEST(SourceBufferTest, Utf82B) {
  SourceMap map;
  ASSERT_TRUE(map.ParseFromJson("utf8_2b", kUtf82B, true));
  SourceBuffer buffer(absl::string_view(kUtf82BGen), std::move(map));
  EXPECT_EQ(2, buffer.OffsetForUtf16Offset(2));
  EXPECT_EQ(4, buffer.OffsetForUtf16Offset(3));
  EXPECT_EQ(5, buffer.OffsetForUtf16Offset(4));
}

TEST(SourceBufferTest, Utf83B) {
  SourceMap map;
  ASSERT_TRUE(map.ParseFromJson("utf8_3b", kUtf83B, true));
  SourceBuffer buffer(absl::string_view(kUtf83BGen), std::move(map));
  EXPECT_EQ(2, buffer.OffsetForUtf16Offset(2));
  EXPECT_EQ(5, buffer.OffsetForUtf16Offset(3));
  EXPECT_EQ(6, buffer.OffsetForUtf16Offset(4));
}

TEST(SourceBufferTest, Utf84B) {
  SourceMap map;
  ASSERT_TRUE(map.ParseFromJson("utf8_4b", kUtf84B, true));
  SourceBuffer buffer(absl::string_view(kUtf84BGen), std::move(map));
  EXPECT_EQ(2, buffer.OffsetForUtf16Offset(2));
  EXPECT_EQ(6, buffer.OffsetForUtf16Offset(3));
  // Sneaky: 𐐷 takes two UTF-16 code units to represent.
  EXPECT_EQ(7, buffer.OffsetForUtf16Offset(5));
}

}  // anonymous namespace
}  // namespace anodyne
