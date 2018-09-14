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

#ifndef ANODYNE_BASE_MEMFS_H_
#define ANODYNE_BASE_MEMFS_H_

#include "anodyne/base/fs.h"

#include <unordered_map>

namespace anodyne {

/// \brief An in-memory filesystem for tests.
class MemoryFileSystem : public FileSystem {
 public:
  MemoryFileSystem() : cwd_(Path::Clean("/")) {}
  MemoryFileSystem(MemoryFileSystem&) = delete;
  MemoryFileSystem& operator=(MemoryFileSystem&) = delete;
  StatusOr<std::string> GetFileContent(absl::string_view path) override;
  absl::optional<Path> GetWorkingDirectory() override { return cwd_; }

  /// \brief adds (or replaces) a file in the filesystem.
  /// \param path the destination path; if it's relative, it will be
  /// made absolute using the current working directory.
  /// \param content the file's content.
  Status InsertFile(absl::string_view path, absl::string_view content);
  /// \brief sets the working directory to `path`.
  Status SetWorkingDirectory(absl::string_view path);

 private:
  /// \brief tries to make a clean, absolute path from a string.
  StatusOr<std::string> MakeCleanAbsolutePath(absl::string_view path);
  /// The current working directory, if there is one.
  Path cwd_;
  /// A map from absolute clean paths to file content.
  std::unordered_map<std::string, std::string> files_;
};

}  // namespace anodyne

#endif  // defined(ANODYNE_BASE_MEMFS_H_)
