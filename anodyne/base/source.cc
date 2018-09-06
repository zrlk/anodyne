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

#include "glog/logging.h"

#include <algorithm>

namespace anodyne {

std::string Range::ToString(const Source& source) const {
  const File* lhs = source.FindFile(begin);
  const File* rhs = source.FindFile(end);
  std::stringstream out;
  if (!lhs) {
    out << "(bad range lhs)";
  } else if (!rhs) {
    out << "(bad range rhs)";
  } else if (lhs != rhs) {
    out << "(range spans files)";
  } else {
    out << lhs->id().ToString() << ":";
    auto lhs_lc = lhs->contents().Utf8LineColForOffset(begin.data() -
                                                       lhs->begin().data());
    auto rhs_lc =
        rhs->contents().Utf8LineColForOffset(end.data() - rhs->begin().data());
    if (lhs_lc.first < 0) {
      out << "(bad range lhs in file)";
    } else if (rhs_lc.first < 0) {
      out << "(bad range rhs in file)";
    } else {
      out << lhs_lc.first << ":" << lhs_lc.second << "-";
      if (lhs_lc.first != rhs_lc.first) {
        out << rhs_lc.first << ":";
      }
      out << rhs_lc.second;
    }
  }
  return out.str();
}

absl::string_view File::Text(Location begin, Location end) const {
  if (end.data() <= begin.data() || begin.data() < begin_.data() ||
      end.data() > end_.data()) {
    return "";
  }
  return absl::string_view(contents_.content())
      .substr(begin.data() - begin_.data(), end.data() - begin.data());
}

const File* Source::FindFile(
    absl::string_view repository, absl::string_view path,
    absl::string_view root,
    std::function<std::unique_ptr<SourceBuffer>(const FileId&)> callback) {
  auto file_id_view = std::make_tuple(repository, path, root);
  const auto& i = file_map_.find(file_id_view);
  if (i != file_map_.end()) {
    return i->second;
  }
  FileId id;
  id.repository_id = std::string(repository);
  id.local_path = std::string(path);
  id.root_path = std::string(root);
  auto new_sb = callback(id);
  if (new_sb == nullptr) {
    return nullptr;
  }
  int to_allocate = new_sb->max_offset();
  // Make sure that all files have at least one unique location.
  if (to_allocate == 0) to_allocate = 1;
  LOG(INFO) << "allocating " << to_allocate << " bytes of address space";
  files_.emplace_back(
      absl::make_unique<File>(id, std::move(*new_sb), max_location_));
  max_location_ = max_location_.offset(to_allocate);
  file_map_[file_id_view] = files_.back().get();
  return files_.back().get();
}

const File* Source::FindFile(Location loc) const {
  if (!loc.is_valid() || files_.empty()) return nullptr;
  if (loc.data() == 1) {
    return files_[0].get();
  }
  const auto i =
      std::lower_bound(files_.begin(), files_.end(), loc,
                       [](const std::unique_ptr<File>& f, Location search) {
                         return f->begin().data() < search.data();
                       });
  if (i == files_.end()) {
    // We already know that loc > the last file's beginning.
    return (loc.data() < files_.back()->end().data()) ? files_.back().get()
                                                      : nullptr;
  } else {
    // We've fixed the boundary condition for the first offset above.
    return (loc.data() == (*i)->begin().data()) ? i->get() : (i - 1)->get();
  }
}

}  // namespace anodyne
