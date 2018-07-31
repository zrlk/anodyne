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

#include "anodyne/base/paths.h"

#include "absl/strings/str_cat.h"
#include "glog/logging.h"

#include <vector>

namespace anodyne {
Path Path::Clean(absl::string_view path) {
  if (path.empty()) {
    return Path(path);
  }
  std::vector<absl::string_view> components;
  size_t total_size = 0;
  size_t last_component = 0;
  bool absolute = (path[0] == '/');
  if (absolute) {
    path = path.substr(1);
  }
  for (size_t p = 0; p < path.size(); ++p) {
    if (path[p] == '/') {
      if (last_component != p && path[last_component] != '/') {
        components.emplace_back(
            path.substr(last_component, p - last_component));
        total_size += p - last_component;
      }
      last_component = p + 1;
    } else if (path[p] == '.') {
      if (p + 1 != path.size() && path[p + 1] == '.') {
        if (p + 2 == path.size() || path[p + 2] == '/') {
          if (!components.empty()) {
            total_size -= components.back().size();
            components.pop_back();
          }
          p += 2;
          last_component = p + 1;
        }
      } else if (p + 1 == path.size() || path[p + 1] == '/') {
        p += 1;
        last_component = p + 1;
      }
    }
  }
  if (last_component < path.size() && path[last_component] != '/') {
    components.emplace_back(path.substr(last_component));
    total_size += path.size() - last_component;
  }
  if (total_size == 0 && !absolute) {
    return Path("");
  }
  std::string new_path;
  new_path.reserve(components.size() + total_size - (absolute ? 0 : 1));
  if (absolute) {
    new_path = "/";
  }
  bool first_component = true;
  for (const auto& component : components) {
    if (!first_component) {
      new_path.append("/");
    }
    first_component = false;
    new_path.append(component.data(), component.size());
  }
  return Path(new_path);
}

absl::optional<Path> Path::Parent() const {
  auto slash = path_.rfind("/");
  if (slash == absl::string_view::npos) {
    return path_.size() == 0 ? absl::nullopt : absl::optional<Path>(Path(""));
  }
  if (is_absolute() && slash == 0) {
    return path_.size() == 1 ? absl::nullopt
                             : absl::optional<Path>(Path(path_.substr(0, 1)));
  }
  return Path(path_.substr(0, slash));
}

absl::optional<Path> Path::Relativize(const Path& rhs) const {
  if (!is_absolute() || !rhs.is_absolute()) {
    return absl::nullopt;
  }
  auto loc = rhs.path_.find(path_);
  if (loc == absl::string_view::npos) {
    return absl::nullopt;
  }
  if (rhs.path_.size() == path_.size()) {
    return Path("");
  }
  if (rhs.path_[path_.size()] != '/') {
    return absl::nullopt;
  }
  return Path(rhs.path_.substr(path_.size() + 1));
}

absl::optional<Path> Path::Concat(absl::string_view rhs) const {
  if (!rhs.empty() && rhs[0] == '/') {
    return absl::nullopt;
  }
  return Clean(absl::StrCat(path_, "/", rhs));
}
}  // namespace anodyne
