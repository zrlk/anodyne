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

#ifndef ANODYNE_BASE_SOURCE_MAP_H_
#define ANODYNE_BASE_SOURCE_MAP_H_

#include "absl/strings/string_view.h"

#include <vector>

namespace anodyne {

struct SourceMapFile {
  /// The path to this file (with sourceRoot prepended).
  /// Note: If the sources are not absolute URLs after prepending of the
  /// "sourceRoot", the sources are resolved relative to the SourceMap (like
  /// resolving script src in a html document).
  std::string path;
  /// The content of this file (if it was provided).
  std::string content;
};

struct SourceMapSegment {
  int64_t generated_line;
  int64_t generated_col;
  int64_t source_line;
  int64_t source_col;
  /// Negative if unset.
  int64_t name;
  int64_t source;
};

/// \brief A source map. (See
/// https://docs.google.com/document/d/1U1RGAehQwRypUTovF1KRlpiOFze0b-_2gc6fAH0KY0k/)
///
/// This class shouldn't grow functionality beyond that which is required to
/// deserialize source maps. Look elsewhere for optimized lookup or conversion
/// to/from byte offsets.
class SourceMap {
 public:
  /// \brief Replaces this source map with the contents of `json`.
  /// \param decode_mappings true if the mappings field should be decoded.
  /// \return true if the file could be loaded successfully.
  bool ParseFromJson(absl::string_view friendly_id, absl::string_view json,
                     bool decode_mappings);
  const std::vector<SourceMapFile>& sources() const { return sources_; }
  const std::vector<SourceMapSegment>& segments() const { return segments_; }
  const std::vector<std::string>& names() const { return names_; }

 private:
  /// \brief Parses the encoded `mappings` field.
  bool ParseMappings(absl::string_view mappings);
  /// \brief All sources from the map.
  std::vector<SourceMapFile> sources_;
  /// \brief All names from the map.
  std::vector<std::string> names_;
  /// \brief All segments from the map.
  ///
  /// All (non-negative) `name` and `source` fields are guaranteed to be in
  /// range of `names_` and `sources_`, but the `*_line` and `*_col` fields
  /// are not.
  std::vector<SourceMapSegment> segments_;
};

}  // namespace anodyne

#endif  // defined(ANODYNE_BASE_SOURCE_MAP_H_)
