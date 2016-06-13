/*
 * Copyright 2016 Google Inc. All rights reserved.
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

#ifndef UTIL_LOCATION_H_
#define UTIL_LOCATION_H_

#include <ostream>

namespace util {
class SourceLocation {
 public:
  SourceLocation() : line_(0), column_(0) {}
  SourceLocation(unsigned line, unsigned column)
      : line_(line), column_(column) {}
  SourceLocation(const SourceLocation& o)
      : line_(o.line_), column_(o.column_) {}
  SourceLocation& operator=(const SourceLocation& o) {
    line_ = o.line_;
    column_ = o.column_;
    return *this;
  }
  unsigned line() const { return line_; }
  unsigned column() const { return column_; }
  SourceLocation offset(unsigned amount) const {
    return SourceLocation(line_, column_ + amount);
  }
  SourceLocation offset_lines(unsigned amount) const {
    return SourceLocation(line_ + amount, 1);
  }

 private:
  unsigned line_;
  unsigned column_;
};

class SourceRange {
 public:
  SourceRange() {}
  SourceRange(const std::string* file, SourceLocation begin, SourceLocation end)
      : file_(file), begin_(begin), end_(end) {}
  SourceRange(const SourceRange& o)
      : file_(o.file_), begin_(o.begin_), end_(o.end_) {}
  SourceRange& operator=(const SourceRange& o) {
    file_ = o.file_;
    begin_ = o.begin_;
    end_ = o.end_;
    return *this;
  }
  const std::string* file() const { return file_; }
  SourceLocation begin() const { return begin_; }
  SourceLocation end() const { return end_; }

 private:
  const std::string* file_ = nullptr;
  SourceLocation begin_;
  SourceLocation end_;
};

std::ostream& operator<<(std::ostream& stream, const SourceRange& loc);
}

#endif
