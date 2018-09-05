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

#ifndef ANODYNE_TOOLS_TT_CODEGEN_H_
#define ANODYNE_TOOLS_TT_CODEGEN_H_

#include "absl/strings/string_view.h"
#include "anodyne/base/source.h"
#include "anodyne/tools/tt_parser.h"

#include <stdio.h>
#include <tuple>

namespace anodyne {
class TtGenerator {
 public:
  /// \brief Generate code for for the definitions in `parser`.
  /// \param parser the parser containing definitions.
  /// \param h_relative_path the path `cc` should use to refer to `h`.
  /// \param h an open file handle for writing header code.
  /// \param cc an open file handle for writing implementation code.
  static bool GenerateCode(const TtParser& parser,
                           absl::string_view h_relative_path, FILE* h,
                           FILE* cc) {
    return TtGenerator(parser, h_relative_path, h, cc).Generate();
  }
  /// \brief Generate code for the matchers in `parser`.
  /// \param parser the parser containing matchers.
  /// \param m an open file handle for writing matcher code.
  static bool GenerateMatchers(const TtParser& parser, FILE* m) {
    return TtGenerator(parser, "", m, nullptr).GenerateMatchers();
  }

 private:
  TtGenerator(const TtParser& parser, absl::string_view h_relative_path,
              FILE* h, FILE* cc)
      : parser_(parser), h_relative_path_(h_relative_path), h_(h), cc_(cc) {}
  /// \brief Generate the header and source preamble for datatype definitions.
  /// \return false on failure.
  bool GeneratePreamble();
  /// \brief Generate the header and source postamble for datatype definitions.
  /// \return false on failure.
  bool GeneratePostamble();
  /// \brief Generate the header and source files for datatype definitions.
  /// \return false on failure.
  bool Generate();
  /// \brief Generate the implementation code for matchers.
  /// \return false on failure.
  bool GenerateMatchers();
  /// A parser containing definitions or matchers.
  const TtParser& parser_;
  /// If we're generating definitions, the relative path the definition
  /// .cc should use to refer to its .h.
  std::string h_relative_path_;
  /// An open file handle to a header, if generating definitions.
  FILE* h_;
  /// An open file handle to the implementation file.
  FILE* cc_;
};
}  // namespace anodyne

#endif  // ANODYNE_TOOLS_TT_CODEGEN_H_
