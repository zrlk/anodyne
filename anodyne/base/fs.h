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

#ifndef ANODYNE_BASE_FS_H_
#define ANODYNE_BASE_FS_H_

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "anodyne/base/paths.h"
#include "third_party/status/status_or.h"

namespace anodyne {

/// \brief Maps paths to file content.
class FileSystem {
 public:
  FileSystem() {}
  FileSystem(FileSystem&) = delete;
  FileSystem& operator=(FileSystem&) = delete;
  /// \brief Retrieve file content for `path`.
  /// \param path path to inspect.
  /// \return the file content, on success.
  virtual StatusOr<std::string> GetFileContent(absl::string_view path) = 0;
  /// \brief Gets the current working directory (which is the directory that
  /// relative paths are implicitly concatenated with) as an absolute path.
  virtual absl::optional<Path> GetWorkingDirectory() = 0;
};

/// \brief Maps paths to file content on the local machine's filesystem.
class RealFileSystem : public FileSystem {
 public:
  RealFileSystem() {}
  RealFileSystem(RealFileSystem&) = delete;
  RealFileSystem& operator=(RealFileSystem&) = delete;
  StatusOr<std::string> GetFileContent(absl::string_view path) override;
  absl::optional<Path> GetWorkingDirectory() override;
};

}  // namespace anodyne

#endif  // defined(ANODYNE_BASE_FS_H_)
