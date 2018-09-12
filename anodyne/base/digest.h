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

#ifndef ANODYNE_BASE_DIGEST_H_
#define ANODYNE_BASE_DIGEST_H_

#include "absl/strings/string_view.h"

#include <string>

namespace anodyne {

/// \brief Returns the lowercase-string-hex-encoded sha256 digest of `content`.
std::string Sha256(absl::string_view content);

}  // namespace anodyne

#endif  // defined(ANODYNE_BASE_DIGEST_H_)
