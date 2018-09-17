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
    return cleaned.status();
  }
  const auto it = files_.find(cleaned->get());
  if (it == files_.end()) {
    return UnknownError(absl::StrCat("Couldn't find ", path));
  }
  return it->second.content;
}

StatusOr<FileKind> MemoryFileSystem::GetFileKind(absl::string_view path) {
  auto cleaned = MakeCleanAbsolutePath(path);
  if (!cleaned) {
    return cleaned.status();
  }
  const auto it = files_.find(cleaned->get());
  if (it == files_.end()) {
    return UnknownError(absl::StrCat("Couldn't find ", path));
  }
  return it->second.kind;
}

Status MemoryFileSystem::SetWorkingDirectory(absl::string_view path) {
  auto cleaned = MakeCleanAbsolutePath(path);
  if (!cleaned.ok()) {
    return cleaned.status();
  }
  cwd_ = *cleaned;
  return OkStatus();
}

Status MemoryFileSystem::InsertFile(absl::string_view path,
                                    absl::string_view content) {
  auto cleaned = MakeCleanAbsolutePath(path);
  if (!cleaned) {
    return UnknownError(absl::StrCat("Bad path ", path));
  }
  auto previous = files_.find(cleaned->get());
  if (previous != files_.end() &&
      previous->second.kind == FileKind::kDirectory) {
    return UnknownError(absl::StrCat("Already a directory: ", path));
  }
  files_[cleaned->get()] = File{FileKind::kRegular, std::string(content)};
  return OkStatus();
}

Status MemoryFileSystem::InsertDirectory(absl::string_view path) {
  auto cleaned = MakeCleanAbsolutePath(path);
  if (!cleaned) {
    return UnknownError(absl::StrCat("Bad path ", path));
  }
  auto previous = files_.find(cleaned->get());
  if (previous != files_.end() && previous->second.kind == FileKind::kRegular) {
    return UnknownError(absl::StrCat("Already a file: ", path));
  }
  files_[cleaned->get()] = File{FileKind::kDirectory, ""};
  return OkStatus();
}

}  // namespace anodyne
