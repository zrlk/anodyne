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

#include "anodyne/base/source_buffer.h"

#include "glog/logging.h"

#include <algorithm>

namespace anodyne {
namespace {
// UTF-8 cheat sheet, thanks to https://en.wikipedia.org/wiki/UTF-8

// First pt |  Last pt | B1       | B2       | B3       | B4       |
// 0x0      |     0x7F | 0xxxxxxx | -        | -        | -        |
// 0x80     |    0x7FF | 110xxxxx | 10xxxxxx | -        | -        |
// 0x800    |   0xFFFF | 1110xxxx | 10xxxxxx | 10xxxxxx | -        |
// 0x10000  | 0x10FFFF | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |

constexpr int kTwoByteMark = 0xC0;
constexpr int kThreeByteMark = 0xE0;
constexpr int kFourByteMark = 0xF0;
constexpr int kContinuationMark = 0x80;

/// \brief Reads a single UTF-8 code point.
/// \param buffer the buffer to read from.
/// \param pos the position in the buffer.
/// \param bound the length of the buffer.
/// \param out_pos set to the position immediately following the code point
/// at pos.
/// \return the value of the code point at pos.
int32_t ReadUtf8(const unsigned char* buffer, int pos, int bound,
                 int* out_pos) {
  int c = buffer[pos++];
  if ((c & 0x80) == 0) {
    *out_pos = pos;
    return c;
  }
  int code_point = 0;
  int bytes_left = 0;
  if ((c & kFourByteMark) == kFourByteMark) {
    code_point = c & 0x1F;
    bytes_left = 3;
  } else if ((c & kThreeByteMark) == kThreeByteMark) {
    code_point = c & 0x0F;
    bytes_left = 2;
  } else if ((c & kTwoByteMark) == kTwoByteMark) {
    code_point = c & 0x07;
    bytes_left = 1;
  }
  for (; pos < bound && bytes_left > 0; ++pos, --bytes_left) {
    code_point <<= 6;
    c = buffer[pos];
    if ((c & kContinuationMark) != kContinuationMark) {
      // Weird encoding.
      break;
    }
    code_point |= (c & 0x3F);
  }
  *out_pos = pos;
  return code_point;
}

// UTF-16 encoding, via https://en.wikipedia.org/wiki/UTF-16
// 0x0000-0xD7FF and 0xE000-0xFFFF => pass-through as one code unit
// 0xD800-0xDFFF => reserved for surrogate encoding
// 0x10000-0x10FFFF => for code point c:
//   c -= 0x10000
//   high surrogate is (c >> 10) + 0xD800
//   low surrogate is (c & 0x3FF) + 0xDC00

/// \brief Counts number of UTF-16 code units necessary to encode `code_point`.
/// \param code_point the unicode code point to encode.
/// \return the number of code units necessary to encode `code_point` (which
/// will either be one or two).
int Utf16CodeUnitsFor(int32_t code_point) {
  return code_point >= 0x10000 ? 2 : 1;
}
}  // anonymous namespace

SourceBuffer::SourceBuffer(std::string&& content, SourceMap&& source_map)
    : content_(content), source_map_(source_map) {
  // TODO: Right now we assume that the incoming file is in UTF-8 with Unix
  // line endings. If this is not the case, we should convert it.
  int segment_index = 0;
  const auto& segments = source_map_.segments();
  const SourceMapSegment* segment =
      segment_index >= segments.size() ? nullptr : &segments[segment_index];
  int utf16_col = 0;
  line_to_offset_.push_back(0);
  line_to_utf16_offset_.push_back(0);
  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(content_.data());
  int utf16_offset = 0;
  for (int utf8_offset = 0; utf8_offset < content_.size();) {
    int32_t c = ReadUtf8(data, utf8_offset, content_.size(), &utf8_offset);
    int utf16_size = Utf16CodeUnitsFor(c);
    utf16_offset += utf16_size;
    utf16_col += utf16_size;
    if (c == '\n') {
      line_to_offset_.push_back(utf8_offset + 1);
      line_to_utf16_offset_.push_back(utf16_offset + 1);
      utf16_col = 0;
    }
    while (segment != nullptr &&
           (segment->generated_line < line_to_offset_.size() - 1 ||
            segment->generated_col < utf16_col)) {
      ++segment_index;
      segment =
          segment_index >= segments.size() ? nullptr : &segments[segment_index];
    }
    if (segment != nullptr &&
        segment->generated_line == line_to_offset_.size() - 1 &&
        segment->generated_col == utf16_col) {
      offset_to_segment_[utf8_offset] = segment_index;
    }
  }
  max_offset_ = content_.size();
}

int SourceBuffer::OffsetForUtf16LineCol(int line, int col) const {
  if (line < 0 || line >= line_to_offset_.size()) {
    return -1;
  }
  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(content_.data());
  int utf8_offset = line_to_offset_[line];
  for (int utf16_col = 0; utf8_offset < content_.size() && utf16_col < col;) {
    int32_t c = ReadUtf8(data, utf8_offset, content_.size(), &utf8_offset);
    utf16_col += Utf16CodeUnitsFor(c);
  }
  return utf8_offset;
}

int SourceBuffer::OffsetForUtf16Offset(int offset) const {
  if (line_to_utf16_offset_.empty() || line_to_offset_.empty()) return -1;
  auto i = std::lower_bound(line_to_utf16_offset_.begin(),
                            line_to_utf16_offset_.end(), offset);
  // No offset tables or offset is on the last line in the table.
  if (i == line_to_utf16_offset_.end()) {
    return OffsetForUtf16LineCol(line_to_utf16_offset_.size() - 1,
                                 offset - line_to_utf16_offset_.back());
  }
  if (*i == offset) {
    return line_to_offset_[*i];
  }
  int line = line_to_utf16_offset_.end() - i - 1;
  if (line < 0) {
    // We always insert a mapping from line 0 to offset 0, so this should not
    // be possible.
    LOG(ERROR) << "invariant broken: negative current line";
    return -1;
  }
  return OffsetForUtf16LineCol(line, offset - line_to_utf16_offset_[line]);
}

std::pair<int, int> SourceBuffer::Utf8LineColForOffset(int offset) const {
  if (offset > max_offset_ || line_to_offset_.empty()) {
    return std::make_pair(-1, -1);
  }
  auto i =
      std::lower_bound(line_to_offset_.begin(), line_to_offset_.end(), offset);
  if (i == line_to_offset_.end()) {
    return std::make_pair(line_to_offset_.size() - 1,
                          offset - line_to_offset_.back());
  }
  if (*i == offset) {
    return std::make_pair(i - line_to_offset_.begin(), 0);
  }
  return std::make_pair(i - line_to_offset_.begin() - 1, offset - *(i - 1));
}

const SourceMapSegment* SourceBuffer::SegmentForOffset(int offset) const {
  auto i = offset_to_segment_.find(offset);
  return (i != offset_to_segment_.end()) ? &source_map_.segments()[i->second]
                                         : nullptr;
}

}  // namespace anodyne
