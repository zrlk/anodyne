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

#include "anodyne/base/source.h"
#include "absl/memory/memory.h"
#include "anodyne/base/source_buffer.h"
#include "gtest/gtest.h"

#include <memory>

namespace anodyne {
namespace {

std::unique_ptr<SourceBuffer> SourceBufferContaining(
    absl::string_view content) {
  return absl::make_unique<SourceBuffer>(content, SourceMap{});
}

const File* AddFile(Source* source, absl::string_view repo,
                    absl::string_view content) {
  return source->FindFile(repo, "", "", [&](const FileId& id) {
    return SourceBufferContaining(content);
  });
}

TEST(SourceTest, InsertFile) {
  Source source;
  bool requested_file = false;
  const auto* file_a =
      source.FindFile("repo", "local", "root", [&](const FileId& id) {
        requested_file = id.repository_id == "repo" &&
                         id.local_path == "local" && id.root_path == "root";
        return SourceBufferContaining("file_a");
      });
  ASSERT_TRUE(requested_file);
  ASSERT_TRUE(file_a != nullptr);
  EXPECT_EQ("repo", file_a->id().repository_id);
  EXPECT_EQ("local", file_a->id().local_path);
  EXPECT_EQ("root", file_a->id().root_path);
  EXPECT_EQ("file_a", file_a->Text(file_a->begin(), file_a->end()));
}

TEST(SourceTest, FindFile) {
  Source source;
  const auto* file_a = AddFile(&source, "a", "file_a");
  const auto* file_b = AddFile(&source, "b", "file_b");
  const auto* file_c = AddFile(&source, "c", "");
  const auto* file_d = AddFile(&source, "d", "file_d");
  EXPECT_EQ(file_a, source.FindFile(file_a->begin()));
  EXPECT_EQ(file_a, source.FindFile(file_a->end().offset(-1)));
  EXPECT_EQ(file_b, source.FindFile(file_b->begin()));
  EXPECT_EQ(file_b, source.FindFile(file_b->end().offset(-1)));
  EXPECT_EQ(file_c, source.FindFile(file_c->begin()));
  EXPECT_EQ(file_d, source.FindFile(file_d->begin()));
  EXPECT_EQ(file_d, source.FindFile(file_d->end().offset(-1)));
  EXPECT_EQ(nullptr, source.FindFile(Location{}));
  EXPECT_EQ(nullptr, source.FindFile(file_d->end()));
}

}  // anonymous namespace
}  // namespace anodyne
