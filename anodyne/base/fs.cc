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

#include "anodyne/base/fs.h"
#include "absl/strings/str_cat.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace anodyne {
StatusOr<Path> FileSystem::MakeCleanAbsolutePath(absl::string_view path) {
  auto cleaned = Path::Clean(path);
  if (cleaned.is_absolute()) {
    return cleaned;
  }
  auto cwd = GetWorkingDirectory();
  if (!cwd) {
    return UnknownError("Couldn't get working directory");
  }
  auto absolute = cwd->Concat(path);
  if (!absolute) {
    return UnknownError(absl::StrCat("Bad path ", path));
  }
  return *absolute;
}

StatusOr<std::string> RealFileSystem::GetFileContent(absl::string_view path) {
  auto filename = std::string(path);
  int fd = ::open(filename.c_str(), 0);
  if (fd < 0) {
    return UnknownError(absl::StrCat("Can't open ", filename));
  }
  struct stat fd_stat;
  if (::fstat(fd, &fd_stat) < 0) {
    return UnknownError(absl::StrCat("Can't stat ", filename));
  }
  std::string out;
  out.resize(fd_stat.st_size);
  if (::read(fd, const_cast<char*>(out.data()), fd_stat.st_size) !=
      fd_stat.st_size) {
    return UnknownError(absl::StrCat("Can't read ", filename));
  }
  ::close(fd);
  return out;
}

StatusOr<FileKind> RealFileSystem::GetFileKind(absl::string_view path) {
  struct stat buf;
  int stat_ok = ::stat(std::string(path).c_str(), &buf);
  if (stat_ok < 0) {
    return UnknownError(absl::StrCat("Couldn't stat input path ", path));
  }
  if (S_ISDIR(buf.st_mode)) {
    return FileKind::kDirectory;
  }
  if (S_ISREG(buf.st_mode)) {
    return FileKind::kRegular;
  }
  return UnknownError(absl::StrCat("Unsupported file kind at ", path));
}

absl::optional<Path> RealFileSystem::GetWorkingDirectory() {
  auto len = ::pathconf(".", _PC_PATH_MAX);
  char* buf = (char*)::malloc(len);
  ::getcwd(buf, len);
  auto path = Path::Clean(absl::string_view(buf, len));
  ::free(buf);
  return path;
}
}  // namespace anodyne
