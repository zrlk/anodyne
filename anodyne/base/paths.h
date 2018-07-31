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

#ifndef ANODYNE_BASE_PATHS_H_
#define ANODYNE_BASE_PATHS_H_

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

#include <iostream>

namespace anodyne {

/// \brief A lexical path. May be absolute or relative.
class Path {
 public:
  /// \return an empty relative path
  Path() {}

  /// \return `path` as a `Path`
  static Path Clean(absl::string_view path);

  /// \return this path before rhs, respecting any ..s
  absl::optional<Path> Concat(absl::string_view rhs) const;

  /// \return the parent of this path
  absl::optional<Path> Parent() const;

  /// \return rhs relativized against this path; both paths must be absolute
  absl::optional<Path> Relativize(const Path& rhs) const;

  /// \return this path, as a string.
  const std::string& get() const { return path_; }

  /// \return whether this path is absolute.
  bool is_absolute() const { return !path_.empty() && path_[0] == '/'; }

 private:
  explicit Path(absl::string_view path) : path_(path) {}

  /// The path itself.
  std::string path_;
};

}  // namespace anodyne

#endif  // defined(ANODYNE_BASE_PATHS_H_)
