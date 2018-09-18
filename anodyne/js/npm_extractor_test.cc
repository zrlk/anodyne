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

 private:
  MemoryFileSystem memfs_;
  MemoryIndex index_;
};

TEST(ExtractorTest, NoPackageJson) {
  ExtractorTest xt;
  EXPECT_FALSE(xt.Run("root"));
  EXPECT_FALSE(xt.index().closed);
}
}  // anonymous namespace
}  // namespace anodyne
