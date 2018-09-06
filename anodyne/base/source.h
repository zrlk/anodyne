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

#ifndef ANODYNE_BASE_SOURCE_H__
#define ANODYNE_BASE_SOURCE_H__

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "anodyne/base/source_buffer.h"

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace anodyne {

/// \brief A point in source text.
///
/// Locations should be passed by value.
class Location {
 public:
  /// The opaque type used for storing the `Location`'s value.
  using Rep = uint32_t;

  /// \return an invalid location.
  Location() : data_(0) {}
  Location(const Location& o) : data_(o.data_) {}
  Location& operator=(const Location& o) {
    data_ = o.data_;
    return *this;
  }

  /// \return a new `Location` with internal representation `data`.
  static Location FromRep(Rep data) {
    Location l;
    l.data_ = data;
    return l;
  }

  /// \return whether this `Location` was not marked as explicitly invalid
  /// (e.g., because it built from its default constructor). Given `offset`
  /// and `FromRep`, it's possible to build out-of-bounds `Location`s fairly
  /// easily.
  bool is_valid() const { return data_ != 0; }

  /// \return a `Location` `count` bytes offset from this one.
  Location offset(int count) const {
    Location o;
    o.data_ = data_ + count;
    return o;
  }

  /// \return an opaque representation of this `Location`'s value.
  Rep data() const { return data_; }

 private:
  /// Opaque data.
  Rep data_;
};

class Source;

/// \brief A range of text in a source file.
struct Range {
  /// The beginning of the range.
  Location begin;
  /// The end of the range (exclusive).
  Location end;
  /// \return if the beginning and the end of this range are both
  /// valid, given the definition of `Location::is_valid`.
  bool is_valid() const { return begin.is_valid() && end.is_valid(); }
  /// \return a new `Range` covering `[this.begin, o.end)`
  Range merge(const Range& o) const { return Range{begin, o.end}; }
  /// \return a string representation of this `Range`.
  std::string ToString(const Source& source) const;
};

/// \brief An identifier for a particular object in a repository.
struct FileId {
  /// An identifier for this file's repository.
  std::string repository_id;
  /// This file's virtual path in its repository.
  std::string local_path;
  /// This file's actual root path in its repository (for generated code).
  std::string root_path;
  /// Return a debug display string for this ID.
  std::string ToString() const {
    if (root_path.empty()) {
      return absl::StrCat(repository_id, "/", local_path);
    } else {
      return absl::StrCat(repository_id, "/", local_path, " (", root_path, ")");
    }
  }
};

/// \brief A named buffer of source text.
class File {
 public:
  File(const FileId& id, SourceBuffer&& contents, Location begin)
      : id_(id),
        contents_(std::move(contents)),
        begin_(begin),
        end_(begin.offset(contents_.max_offset())) {}
  File(const File&) = delete;
  File& operator=(const File&) = delete;

  /// \brief Gets source text from [`begin`, `end`).
  absl::string_view Text(Location begin, Location end) const;

  /// \brief Gets source text from the given range.
  absl::string_view Text(Range range) const {
    return Text(range.begin, range.end);
  }

  /// \brief Returns the identity of this file.
  const FileId& id() const { return id_; }

  /// \brief Returns this file's beginning location.
  Location begin() const { return begin_; }

  /// \brief Returns this file's ending location.
  Location end() const { return end_; }

  /// \brief Return this file's underlying buffer.
  const SourceBuffer& contents() const { return contents_; }

 private:
  FileId id_;
  SourceBuffer contents_;
  Location begin_;
  Location end_;
};

/// \brief Manages source text and mapping `Locations` to and from files.
class Source {
 public:
  Source() { max_location_ = Location::FromRep(1); }
  Source(const Source&) = delete;
  Source& operator=(const Source&) = delete;
  /// \return The `File` with the given repository, path, and root; or null.
  /// If a `callback` is provided, its result will be stored for that triple.
  /// The `File` is owned by this `Source`.
  const File* FindFile(
      absl::string_view repository, absl::string_view path,
      absl::string_view root,
      std::function<std::unique_ptr<SourceBuffer>(const FileId&)> callback);
  /// \return The `File` in which `loc` is stored, or null. The `File` is owned
  /// by this `Source`.
  const File* FindFile(Location loc) const;
  /// \return a location suitable for builtin objects referenced by real objects
  Location builtin_location(Location in) const { return Location{}; }
  /// \return a range suitable for builtin objects referenced by real objects
  Range builtin_range(Range in) const {
    return Range{builtin_location(in.begin), builtin_location(in.end)};
  }

 private:
  std::vector<std::unique_ptr<File>> files_;
  std::map<std::tuple<absl::string_view, absl::string_view, absl::string_view>,
           const File*>
      file_map_;
  Location max_location_;
};
}  // namespace anodyne

#endif  // ANODYNE_BASE_SOURCE_H__
