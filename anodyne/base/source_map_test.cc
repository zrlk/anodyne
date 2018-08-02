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

#include "anodyne/base/source_map.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"

namespace anodyne {
namespace {

std::string Segment(const SourceMapSegment& s) {
  return absl::StrCat(
      "[", std::to_string(s.source_line), ",", std::to_string(s.source_col),
      "]->[", std::to_string(s.generated_line), ",",
      std::to_string(s.generated_col), "] (", std::to_string(s.name), "#",
      std::to_string(s.source), ")");
}

bool MakeExampleWithMappings(const absl::string_view mappings,
    SourceMap* map) {
  return map->ParseFromJson("example", absl::StrCat(R"(
    {
      "version": 3,
      "file": "out.js",
      "sourceRoot": "",
      "sources": ["foo.js", "bar.js"],
      "sourcesContent": [null, null],
      "names": ["src", "maps", "are", "fun"],
      "mappings": ")", mappings, R"("
    }
  )"), true);
}

TEST(SourceMaps, DecodesAlternate) {
  SourceMap map;
  // From
  // https://blogs.msdn.microsoft.com/davidni/2016/03/14/source-maps-under-the-hood-vlq-base64-and-yoda/
  ASSERT_TRUE(MakeExampleWithMappings("AACKA,IACIC,MACTC;", &map));
  ASSERT_EQ(3, map.segments().size());
  EXPECT_EQ("[1,5]->[0,0] (0#0)", Segment(map.segments()[0]));
  EXPECT_EQ("[2,9]->[0,4] (1#0)", Segment(map.segments()[1]));
  EXPECT_EQ("[3,0]->[0,10] (2#0)", Segment(map.segments()[2]));
}

TEST(SourceMaps, DecodesAnotherAlternate) {
  SourceMap map;
  // From https://github.com/mozilla/source-map/blob/master/test/util.js
  ASSERT_TRUE(MakeExampleWithMappings(
      "CAAC,IAAI,IAAM,SAAUA,GAClB,OAAOC,IAAID;CCDb,IAAI,IAAM,SAAUE,GAClB,OAAOA",
      &map));
  ASSERT_EQ(13, map.segments().size());
  EXPECT_EQ("[0,1]->[0,1] (-1#0)", Segment(map.segments()[0]));
  EXPECT_EQ("[0,5]->[0,5] (-1#0)", Segment(map.segments()[1]));
  EXPECT_EQ("[0,11]->[0,9] (-1#0)", Segment(map.segments()[2]));
  EXPECT_EQ("[0,21]->[0,18] (0#0)", Segment(map.segments()[3]));
  EXPECT_EQ("[1,3]->[0,21] (-1#0)", Segment(map.segments()[4]));
  EXPECT_EQ("[1,10]->[0,28] (1#0)", Segment(map.segments()[5]));
  EXPECT_EQ("[1,14]->[0,32] (0#0)", Segment(map.segments()[6]));
  EXPECT_EQ("[0,1]->[1,1] (-1#1)", Segment(map.segments()[7]));
  EXPECT_EQ("[0,5]->[1,5] (-1#1)", Segment(map.segments()[8]));
  EXPECT_EQ("[0,11]->[1,9] (-1#1)", Segment(map.segments()[9]));
  EXPECT_EQ("[0,21]->[1,18] (2#1)", Segment(map.segments()[10]));
  EXPECT_EQ("[1,3]->[1,21] (-1#1)", Segment(map.segments()[11]));
  EXPECT_EQ("[1,10]->[1,28] (2#1)", Segment(map.segments()[12]));
}

TEST(SourceMaps, DecodesExample) {
  SourceMap map;
  // Unfortunately, it looks like the example string in the spec is nonsense.
  ASSERT_TRUE(MakeExampleWithMappings("A,AAAB;;ABCDE;", &map));
  ASSERT_EQ(4, map.names().size());
  EXPECT_EQ("src", map.names()[0]);
  EXPECT_EQ("maps", map.names()[1]);
  EXPECT_EQ("are", map.names()[2]);
  EXPECT_EQ("fun", map.names()[3]);
  ASSERT_EQ(2, map.sources().size());
  EXPECT_EQ("foo.js", map.sources()[0].path);
  EXPECT_EQ("bar.js", map.sources()[1].path);
  EXPECT_EQ("", map.sources()[0].content);
  EXPECT_EQ("", map.sources()[1].content);
}

}  // anonymous namespace
}  // namespace anodyne
