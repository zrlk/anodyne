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

#include "anodyne/js/npm_extractor.h"
#include "anodyne/base/digest.h"
#include "anodyne/base/memfs.h"
#include "gtest/gtest.h"
#include "kythe/cxx/common/index_writer.h"
#include "kythe/cxx/common/json_proto.h"

namespace anodyne {
namespace {
struct MemoryIndex {
  /// Maps hashes to units.
  std::map<std::string, kythe::proto::IndexedCompilation> units;
  /// Maps hashes to file content.
  std::map<std::string, std::string> files;
  /// Whether this index was closed.
  bool closed = false;
  /// Convenience function to get a particular unit.
  const kythe::proto::IndexedCompilation* GetUnit(
      const std::string& hash) const {
    auto it = units.find(hash);
    if (it == units.end()) return nullptr;
    return &it->second;
  }
  /// Convenience function to get a particular file.
  const std::string* GetFile(const std::string& hash) const {
    auto it = files.find(hash);
    if (it == files.end()) return nullptr;
    return &it->second;
  }
};

/// \brief Wraps a MemoryIndex.
class MemoryIndexWriter : public kythe::IndexWriterInterface {
 public:
  MemoryIndexWriter(MemoryIndex* index) : memory_index_(index) {}
  kythe::StatusOr<std::string> WriteUnit(
      const kythe::proto::IndexedCompilation& unit) override {
    auto json = kythe::WriteMessageAsJsonToString(unit);
    if (!json) return json;
    auto hash = Sha256(*json);
    memory_index_->units[hash] = unit;
    return hash;
  }
  kythe::StatusOr<std::string> WriteFile(absl::string_view content) override {
    auto hash = Sha256(content);
    memory_index_->files[hash] = std::string(content);
    return hash;
  }
  kythe::Status Close() override {
    memory_index_->closed = true;
    return kythe::OkStatus();
  }

 private:
  MemoryIndex* memory_index_;
};

class ExtractorTest {
 public:
  bool Run(absl::string_view root_path) {
    NpmExtractor extractor;
    return extractor.Extract(
        &memfs_,
        kythe::IndexWriter(absl::make_unique<MemoryIndexWriter>(&index_)),
        root_path);
  }
  const MemoryIndex& index() { return index_; }
  MemoryFileSystem* memfs() { return &memfs_; }

 private:
  MemoryFileSystem memfs_;
  MemoryIndex index_;
};

TEST(ExtractorTest, NoPackageJson) {
  ExtractorTest xt;
  EXPECT_FALSE(xt.Run("root"));
  EXPECT_FALSE(xt.index().closed);
}

TEST(ExtractorTest, BadPackageJson) {
  ExtractorTest xt;
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root").ok());
  ASSERT_TRUE(xt.memfs()->InsertFile("root/package.json", "!").ok());
  EXPECT_FALSE(xt.Run("root"));
  EXPECT_FALSE(xt.index().closed);
}

constexpr char kHelloWorld[] = R"(
"hello, world";
)";

TEST(ExtractorTest, NoDependencies) {
  ExtractorTest xt;
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root").ok());
  ASSERT_TRUE(xt.memfs()
                  ->InsertFile("root/package.json", R"(
{
  "name": "indexme",
  "version": "1.0.0",
  "description": "please index me",
  "main": "index.js"
}
)")
                  .ok());
  ASSERT_TRUE(xt.memfs()->InsertFile("root/index.js", kHelloWorld).ok());
  EXPECT_TRUE(xt.Run("root"));
  EXPECT_TRUE(xt.index().closed);
  ASSERT_EQ(1, xt.index().units.size());
  auto* unit = &xt.index().units.begin()->second;
  ASSERT_EQ(2, unit->unit().required_input_size());
  EXPECT_EQ("npm/indexme@1.0.0", unit->unit().v_name().corpus());
  bool found_index = false;
  bool found_package = false;
  for (const auto& ri : unit->unit().required_input()) {
    if (ri.info().path() == "index.js") {
      auto* file = xt.index().GetFile(ri.info().digest());
      ASSERT_FALSE(file == nullptr);
      EXPECT_EQ(kHelloWorld, *file);
      found_index = true;
    }
    if (ri.info().path() == "package.json") found_package = true;
  }
  EXPECT_TRUE(found_index);
  EXPECT_TRUE(found_package);
}

TEST(ExtractorTest, Dependencies) {
  ExtractorTest xt;
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root").ok());
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root/node_modules").ok());
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root/node_modules/a").ok());
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root/node_modules/b").ok());
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root/node_modules/c").ok());
  ASSERT_TRUE(xt.memfs()
                  ->InsertFile("root/package.json", R"(
{
  "name": "root",
  "version": "1.0.0",
  "main": "index.js",
  "dependencies": {
    "a": "^2.0.0"
  }
}
)")
                  .ok());
  ASSERT_TRUE(xt.memfs()
                  ->InsertFile("root/node_modules/a/package.json", R"(
{
  "name": "a",
  "version": "2.1.0",
  "main": "index.js",
  "dependencies": {
    "b": "^2.0.0",
    "c": "^2.0.0"
  }
}
)")
                  .ok());
  ASSERT_TRUE(xt.memfs()
                  ->InsertFile("root/node_modules/b/package.json", R"(
{
  "name": "b",
  "version": "2.2.0",
  "main": "index.js",
  "dependencies": {
    "c": "^2.0.0"
  }
}
)")
                  .ok());
  ASSERT_TRUE(xt.memfs()
                  ->InsertFile("root/node_modules/c/package.json", R"(
{
  "name": "c",
  "version": "2.3.0",
  "main": "index.js"
}
)")
                  .ok());
  ASSERT_TRUE(xt.memfs()->InsertFile("root/index.js", "root").ok());
  ASSERT_TRUE(xt.memfs()->InsertFile("root/node_modules/a/index.js", "a").ok());
  ASSERT_TRUE(xt.memfs()->InsertFile("root/node_modules/b/index.js", "b").ok());
  ASSERT_TRUE(xt.memfs()->InsertFile("root/node_modules/c/index.js", "c").ok());
  EXPECT_TRUE(xt.Run("root"));
  EXPECT_TRUE(xt.index().closed);
  ASSERT_EQ(1, xt.index().units.size());
  auto* unit = &xt.index().units.begin()->second;
  EXPECT_EQ(8, unit->unit().required_input_size());
}

TEST(ExtractorTest, SourceMap) {
  ExtractorTest xt;
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root").ok());
  ASSERT_TRUE(xt.memfs()->InsertDirectory("root/src").ok());
  ASSERT_TRUE(xt.memfs()
                  ->InsertFile("root/package.json", R"(
{
  "name": "root",
  "version": "1.0.0",
  "main": "index.js"
}
)")
                  .ok());
  ASSERT_TRUE(xt.memfs()->InsertFile("root/index.js", "root").ok());
  ASSERT_TRUE(xt.memfs()
                  ->InsertFile("root/index.js.map", R"(
    {
      "version": 3,
      "file": "index.js",
      "sourceRoot": "",
      "sources": ["src/index.sj"],
      "sourcesContent": [null],
      "names": [],
      "mappings": ""
    }
)")
                  .ok());
  ASSERT_TRUE(xt.memfs()->InsertFile("root/src/index.sj", "toor").ok());
  EXPECT_TRUE(xt.Run("root"));
  EXPECT_TRUE(xt.index().closed);
  ASSERT_EQ(1, xt.index().units.size());
  auto* unit = &xt.index().units.begin()->second;
  EXPECT_EQ(4, unit->unit().required_input_size());
}

}  // anonymous namespace
}  // namespace anodyne
