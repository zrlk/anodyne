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
#include "absl/strings/str_cat.h"

namespace anodyne {

StatusOr<std::string> MemoryFileSystem::GetFileContent(absl::string_view path) {
  auto cleaned = MakeCleanAbsolutePath(path);
  if (!cleaned) {
    return cleaned;
  }
  const auto it = files_.find(*cleaned);
  if (it == files_.end()) {
    return UnknownError(absl::StrCat("Couldn't find ", path));
  }
  return it->second;
}

StatusOr<std::string> MemoryFileSystem::MakeCleanAbsolutePath(
    absl::string_view path) {
  auto cleaned = Path::Clean(path);
  if (cleaned.is_absolute()) {
    return cleaned.get();
  }
  auto absolute = cwd_.Concat(path);
  if (!absolute) {
    return UnknownError(absl::StrCat("Bad path ", path));
  }
  return absolute->get();
}

Status MemoryFileSystem::SetWorkingDirectory(absl::string_view path) {
  auto cleaned = MakeCleanAbsolutePath(path);
  if (!cleaned.ok()) {
    return cleaned.status();
  }
  cwd_ = Path::Clean(*cleaned);
  return OkStatus();
}

Status MemoryFileSystem::InsertFile(absl::string_view path,
                                    absl::string_view content) {
  auto cleaned = MakeCleanAbsolutePath(path);
  if (!cleaned) {
    return UnknownError(absl::StrCat("Bad path ", path));
  }
  files_[*cleaned] = std::string(content);
  return OkStatus();
}

}  // namespace anodyne
