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

#ifndef ANODYNE_JS_NPM_PACKAGE_H_
#define ANODYNE_JS_NPM_PACKAGE_H_

#include "absl/strings/string_view.h"

#include <memory>
#include <vector>

namespace anodyne {

/// \brief A dependency declaration from one package to another.
struct NpmDependency {
  /// The id of the package.
  std::string package_id;
  /// The semver version spec of the package.
  std::string version_spec;
};

/// \brief An npm package and its dependency declarations.
///
/// See https://docs.npmjs.com/files/package.json for reference.
class NpmPackage {
 public:
  /// \brief Parse a package from json.
  /// \param friendly_id an identifier to use in error messages.
  /// \param json the contents of the package's `package.json`.
  /// \return a package on success or null on failure.
  static std::unique_ptr<NpmPackage> ParseFromJson(
      absl::string_view friendly_id, absl::string_view json);
  /// \return the package's `_id` or empty if none was present.
  const std::string& npm_id() const { return npm_id_; }
  /// \return the package's `name` or empty if none was present.
  const std::string& name() const { return name_; }
  /// \return the package's `version` or empty if none was present.
  const std::string& version() const { return version_; }
  /// \return a list of dependencies for this package.
  const std::vector<NpmDependency>& dependencies() const {
    return dependencies_;
  }
  /// \return the package's `main` source file, or empty if none was present.
  const std::string& main_source_file() const { return main_source_file_; }

 private:
  NpmPackage() {}

  /// This package's dependencies.
  std::vector<NpmDependency> dependencies_;
  /// This package's npm id, or empty.
  std::string npm_id_;
  /// This package's name, or empty.
  std::string name_;
  /// This package's version, or empty.
  std::string version_;
  /// This package's main source file, or empty.
  std::string main_source_file_;
};

}  // namespace anodyne

#endif  // defined(ANODYNE_JS_NPM_PACKAGE_H__)
