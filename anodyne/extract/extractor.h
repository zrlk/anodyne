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

#ifndef ANODYNE_EXTRACT_EXTRACTOR_H_
#define ANODYNE_EXTRACT_EXTRACTOR_H_

#include "absl/strings/string_view.h"
#include "anodyne/base/fs.h"
#include "kythe/cxx/common/index_writer.h"

namespace anodyne {

/// \brief Records details about compilations.
class Extractor {
 public:
  /// \brief Attempts to identify and extract one or more compilations from
  /// `root_path` in `file_system` and commit them to `sink`.
  /// \param file_system the file system to read from.
  /// \param sink where to record compilation details.
  /// \param root_path the root path for the compilation in `file_system`.
  /// \return true if a compilation was committed to `sink`.
  virtual bool Extract(FileSystem* file_system, kythe::IndexWriter sink,
                       absl::string_view root_path) = 0;
  virtual ~Extractor() {}
};

}  // namespace anodyne

#endif  // defined(ANODYNE_EXTRACT_EXTRACTOR_H_)
