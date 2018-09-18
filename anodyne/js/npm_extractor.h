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

#ifndef ANODYNE_JS_NPM_EXTRACTOR_H_
#define ANODYNE_JS_NPM_EXTRACTOR_H_

#include "anodyne/extract/extractor.h"

namespace anodyne {

/// \brief An Extractor that can handle installed npm projects.
class NpmExtractor : public Extractor {
 public:
  bool Extract(FileSystem* file_system, kythe::IndexWriter sink,
               absl::string_view root_path) override;
};

}  // namespace anodyne

#endif  // defined(ANODYNE_JS_NPM_EXTRACTOR_H_)
