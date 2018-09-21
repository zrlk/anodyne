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

#include "anodyne/js/npm_extractor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "anodyne/base/digest.h"
#include "anodyne/base/paths.h"
#include "anodyne/base/source_map.h"
#include "anodyne/js/npm_package.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "kythe/cxx/common/status_or.h"
#include "kythe/proto/analysis.pb.h"

#include <deque>
#include <memory>
#include <unordered_map>

namespace anodyne {
namespace {
/// \return the absolute path to the npm project root.
/// \param raw_hint the directory containing or the path to package.json,
/// which may be user-provided.
absl::optional<Path> FindNpmDirectory(FileSystem* fs,
                                      absl::string_view raw_hint) {
  auto path = fs->MakeCleanAbsolutePath(raw_hint);
  auto kind = fs->GetFileKind(path->get());
  if (!kind) {
    LOG(ERROR) << kind.status();
    return absl::nullopt;
  }
  if (*kind == FileKind::kDirectory) {
    auto concat = path->Concat("package.json");
    if (!concat) {
      LOG(ERROR) << "Bad path when looking for package.json";
      return absl::nullopt;
    }
    path = *concat;
    kind = fs->GetFileKind(path->get());
    if (!kind) {
      LOG(ERROR) << kind.status();
      return absl::nullopt;
    }
  }
  if (*kind != FileKind::kRegular) {
    LOG(ERROR) << "We expect package.json to be a regular file";
    return absl::nullopt;
  }
  return path->Parent();
}

/// \brief Extracts npm packages.
///
/// Add the package root(s) by calling AddRootPackage with the paths to
/// the directories containing their package.json manifests. Then call
/// Complete() to write out the data.
class NpmExtractorPass {
 public:
  NpmExtractorPass(FileSystem* fs, kythe::IndexWriter sink)
      : fs_(fs), sink_(std::move(sink)) {}
  NpmExtractorPass& operator=(NpmExtractorPass&) = delete;
  NpmExtractorPass(NpmExtractorPass&) = delete;
  /// \brief Adds the root package from an installed npm package.
  /// \param root absolute path to the package's root directory.
  void AddRootPackage(const Path& root) {
    NpmPackage* package = AddPackage(root, true);
    if (package == nullptr) {
      return;
    }
    VNameForPackage(*package, unit()->mutable_v_name());
    auto deps = *root.Concat("node_modules");
    do {
      while (!dependencies_.empty()) {
        auto& dep = dependencies_.front();
        if (packages_.find(dep.package_id) == packages_.end()) {
          if (dep.package_id.find("/") != std::string::npos) {
            LOG(WARNING) << "invalid package id (contains /)";
            had_errors_ = true;
            dependencies_.pop_front();
            continue;
          }
          AddPackage(*deps.Concat(dep.package_id), false);
        }
        dependencies_.pop_front();
        if (package == nullptr) {
          return;
        }
      }
    } while (!dependencies_.empty());
  }

  /// \brief Write out the compilation.
  /// \return false if there were errors.
  bool Complete() {
    if (had_errors_) {
      return false;
    }
    auto wrote = sink_.WriteUnit(compilation_);
    if (!wrote) {
      LOG(ERROR) << "writing unit: " << wrote.status();
      auto closed = sink_.Close();
      if (!closed.ok()) {
        LOG(ERROR) << "closing index: " << closed;
      }
      return false;
    }
    auto closed = sink_.Close();
    if (!closed.ok()) {
      LOG(ERROR) << "closing index: " << closed;
      return false;
    }
    return true;
  }

 private:
  /// \brief Populates `*out` with a VName for `package`.
  void VNameForPackage(const NpmPackage& package, kythe::proto::VName* out) {
    out->set_language("anodynejs");
    out->set_root("");
    out->set_corpus(
        absl::StrCat("npm/", package.name(), "@", package.version()));
    out->set_path("");
    out->set_signature("");
  }
  /// \brief Inserts a package into the compilation.
  /// \param root absolute path to the package's root directory.
  /// \param is_root whether this package is the root package and not a
  /// dependency; if true, source files for this package will be stored.
  /// \return null on failure; otherwise a pointer to the package.
  NpmPackage* AddPackage(const Path& root, bool is_root) {
    LOG(INFO) << "adding npm package in " << root.get();
    auto package = *root.Concat("package.json");
    auto maybe_content = fs_->GetFileContent(package.get());
    if (!maybe_content) {
      LOG(WARNING) << "reading " << package.get() << ": "
                   << maybe_content.status();
      had_errors_ = true;
      return nullptr;
    }
    auto parsed = NpmPackage::ParseFromJson(package.get(), *maybe_content);
    if (parsed == nullptr) {
      had_errors_ = true;
      return nullptr;
    }
    if (parsed->name().empty()) {
      LOG(ERROR) << "npm package in " << root.get() << " has no name";
      had_errors_ = true;
      return nullptr;
    }
    kythe::proto::VName base_vname;
    VNameForPackage(*parsed, &base_vname);
    auto rel_path = root.Relativize(package);
    if (!rel_path) {
      LOG(ERROR) << "package name couldn't be relativized";
      had_errors_ = true;
      return nullptr;
    }
    base_vname.set_path(rel_path->get());
    AddFile(rel_path->get(), *maybe_content, base_vname);
    if (!AddMainSourceFile(base_vname, *parsed, root, is_root)) return nullptr;
    for (const auto& dep : parsed->dependencies()) {
      dependencies_.push_back(dep);
    }
    auto id = parsed->name();
    packages_[id] = std::move(parsed);
    return packages_[id].get();
  }

  /// \brief Attempts to add the main source file for `package` to the unit
  /// named `base_vname`.
  /// \param root absolute path to the package's root directory.
  /// \param is_root whether this package is the root package and not a
  /// dependency; if true, source files for this package will be stored.
  /// \return false if there were errors; note that having no main source file
  /// is not considered an error.
  bool AddMainSourceFile(const kythe::proto::VName& base_vname,
                         const NpmPackage& package, const Path& root,
                         bool is_root) {
    if (package.main_source_file().empty()) return true;
    auto local_path = root.Concat(package.main_source_file());
    if (!local_path) return true;
    auto path = root.Relativize(*local_path);
    if (!path) return true;
    auto maybe_content = fs_->GetFileContent(local_path->get());
    kythe::proto::VName file_vname = base_vname;
    if (maybe_content) {
      file_vname.set_path(path->get());
      AddFile(path->get(), *maybe_content, file_vname);
      if (is_root) {
        unit()->add_source_file(path->get());
      }
    } else {
      LOG(WARNING) << "reading " << local_path->get() << ": "
                   << maybe_content.status();
      had_errors_ = true;
      return false;
    }
    // TODO: There are other ways to link source maps (e.g., some
    // compilers will add "//# sourceMappingURL=/foo/bar/baz.map"
    // to the generated .js file).
    auto source_map_path = local_path->get() + ".map";
    maybe_content = fs_->GetFileContent(source_map_path);
    if (!maybe_content) return true;
    LOG(INFO) << "found a source map for " << local_path->get();
    SourceMap map;
    if (map.ParseFromJson(source_map_path, *maybe_content, false)) {
      file_vname.set_path(path->get() + ".map");
      AddFile(path->get() + ".map", *maybe_content, file_vname);
      auto parent = path->Parent();
      if (parent) {
        AddSourceMapSources(root, *parent, map, file_vname);
      } else {
        LOG(WARNING) << "no parent for " << path->get();
      }
    } else {
      LOG(WARNING) << "failed to parse " << local_path->get();
    }
    return true;
  }

  /// \param package_local_root root directory for the package on the local
  /// machine (`/foo/bar/node_modules/bam`)
  /// \param source_map_parent directory containing the source map relative to
  /// package_root (`build`)
  void AddSourceMapSources(const Path& package_local_root,
                           const Path& source_map_parent, const SourceMap& map,
                           const kythe::proto::VName& base_vname) {
    auto local_parent_path_maybe =
        package_local_root.Concat(source_map_parent.get());
    if (!local_parent_path_maybe) {
      LOG(WARNING) << "Bad source map paths. local root: "
                   << package_local_root.get()
                   << " parent: " << source_map_parent.get();
      return;
    }
    auto local_parent_path = *local_parent_path_maybe;
    kythe::proto::VName map_vname = base_vname;
    for (const auto& file : map.sources()) {
      auto fixed_path = local_parent_path.Concat(file.path);
      if (!fixed_path) {
        LOG(WARNING) << "Bad file path: " << file.path;
        continue;
      }
      auto rel_path = package_local_root.Relativize(*fixed_path);
      if (!rel_path) {
        LOG(WARNING) << "Couldn't relativize path: " << file.path;
        continue;
      }
      map_vname.set_path(rel_path->get());
      if (!file.content.empty()) {
        AddFile(rel_path->get(), file.content, map_vname);
        LOG(INFO) << "Adding source map source with content "
                  << rel_path->get();
      } else {
        std::string data;
        auto maybe_content = fs_->GetFileContent(fixed_path->get());
        if (maybe_content) {
          LOG(INFO) << "adding source map " << rel_path->get();
          AddFile(rel_path->get(), data, map_vname);
        } else {
          LOG(WARNING) << "getting source map " << rel_path->get() << ": "
                       << maybe_content.status();
        }
      }
    }
  }

  /// \brief Inserts a file into the compilation.
  /// \param path the path to insert the file at (relative to the package root).
  /// \param content the file's content.
  /// \param vname the file's VName.
  /// \return false on failure.
  bool AddFile(absl::string_view path, absl::string_view content,
               const kythe::proto::VName& vname) {
    auto maybe_digest = sink_.WriteFile(content);
    if (!maybe_digest) {
      LOG(ERROR) << "adding file " << path << ": " << maybe_digest.status();
      had_errors_ = true;
      return false;
    }
    auto* input = unit()->add_required_input();
    (*input->mutable_v_name()) = vname;
    input->mutable_info()->set_path(std::string(path));
    input->mutable_info()->set_digest(*maybe_digest);
    return true;
  }

  /// \return the compilation unit we're building.
  kythe::proto::CompilationUnit* unit() { return compilation_.mutable_unit(); }

  /// The filesystem to use. Unowned.
  FileSystem* fs_;
  /// Packages we've loaded, indexed by name.
  std::unordered_map<std::string, std::unique_ptr<NpmPackage>> packages_;
  /// The compilation we're building.
  kythe::proto::IndexedCompilation compilation_;
  /// Where to write the compilation.
  kythe::IndexWriter sink_;
  /// Whether we had errors.
  bool had_errors_ = false;
  /// Dependencies to be processed.
  std::deque<NpmDependency> dependencies_;
};
}  // anonymous namespace

bool NpmExtractor::Extract(FileSystem* file_system, kythe::IndexWriter sink,
                           absl::string_view root_path) {
  NpmExtractorPass pass(file_system, std::move(sink));
  if (root_path.empty()) {
    root_path = ".";
  }
  auto npm_root = FindNpmDirectory(file_system, root_path);
  if (!npm_root) {
    LOG(WARNING) << "Couldn't find npm project as " << root_path;
    return false;
  }
  pass.AddRootPackage(*npm_root);
  return pass.Complete();
}
}  // namespace anodyne
