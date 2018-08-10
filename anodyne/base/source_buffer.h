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

#ifndef ANODYNE_BASE_SOURCE_FILE_H__
#define ANODYNE_BASE_SOURCE_FILE_H__

#include "absl/strings/string_view.h"
#include "anodyne/base/source_map.h"

#include <tuple>
#include <unordered_map>

namespace anodyne {

/// \brief A buffer of source text.
///
/// Source text can come in multiple encodings. Source tools can refer to text
/// using multiple kinds of indices (line/col versus offset), which in turn
/// are affected by encoding (are columns/offsets byte indices or UTF16 short
/// indices?).
///
/// Line and column numbers are always zero-based.
class SourceBuffer {
 public:
  SourceBuffer(std::string&& content, SourceMap&& source_map);
  SourceBuffer(absl::string_view content, SourceMap&& source_map)
      : SourceBuffer(std::string(content), std::move(source_map)) {}

  /// \param line 0-based line number.
  /// \param col 0-based column number in UTF-16 units (not code points!)
  /// \return -1 for out of bounds; otherwise the byte offset for (line, col)
  /// in this file's source encoding.
  int OffsetForUtf16LineCol(int line, int col) const;

  /// \param offset file offset in UTF-16 units (not code points!)
  /// \return -1 for out of bounds; otherwise the byte offset for offset in
  /// this file's source encoding.
  int OffsetForUtf16Offset(int offset) const;

  /// \param offset file offset in bytes
  /// \return null for no mapping; otherwise, the `SourceMapSegment` located
  /// at `offset`. The `SourceBuffer` owns this memory.
  const SourceMapSegment* SegmentForOffset(int offset) const;

  /// \param offset file offset in bytes
  /// \return (line, col), where col is the number of utf-8 code units (or
  /// a negative line on error). line and col are both 0-based.
  std::pair<int, int> Utf8LineColForOffset(int offset) const;

  /// \return the file's source map.
  const SourceMap& source_map() const { return source_map_; }

  /// \return the file's content.
  const std::string& content() const { return content_; }

  /// \return the file's maximum UTF-8 offset.
  int max_offset() const { return max_offset_; }

 private:
  /// The content of this file.
  std::string content_;
  /// This file's source map.
  SourceMap source_map_;
  /// Maps byte offsets to segment indices.
  std::unordered_map<int, int> offset_to_segment_;
  /// Maps 0-based line numbers to cumulative byte counts.
  std::vector<int> line_to_offset_;
  /// Maps 0-based line numbers to cumulative UTF-16 code points.
  std::vector<int> line_to_utf16_offset_;
  /// This file's maximum UTF-8 offset.
  int max_offset_;
};

}  // namespace anodyne

#endif  // defined(ANODYNE_BASE_SOURCE_FILE_H__)
