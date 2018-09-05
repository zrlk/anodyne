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

// tt ("tree tool") generates tagged union types.
//
// There is documentation at
//     https://github.com/google/anodyne/blob/master/anodyne/tools/tt.md
//
// `tt output-prefix input.tt` reads tree definitions from `input.tt` and
//     writes implementation files `output-prefix.cc` and `output-prefix.h`
// `tt output-prefix input.cc` reads pattern definitions from `input.cc` and
//     writes the implementation file `output-prefix.matchers.h`

#include "absl/memory/memory.h"
#include "anodyne/base/fs.h"
#include "anodyne/base/source.h"
#include "anodyne/tools/tt_codegen.h"
#include "anodyne/tools/tt_parser.h"

#include <stdio.h>
#include <string>

namespace {

/// \brief Unlinks a given path on destruction.
class AutoUnlink {
 public:
  AutoUnlink(absl::string_view path) : path_(path) {}
  ~AutoUnlink() {
    if (path_.empty()) return;
    if (unlink(path_.c_str()) != 0) {
      perror(absl::StrCat("could not unlink ", path_).c_str());
    }
  }
  /// \brief Don't unlink the path.
  void Disarm() { path_.clear(); }

 private:
  /// Path to unlink, or empty.
  std::string path_;
};

/// \brief Emit tree definitions from `source_content` using `dest_file_prefix`
/// to name output.
int BuildTreeDefs(const anodyne::File* source_content,
                  absl::string_view dest_file_prefix) {
  anodyne::TtParser parser;
  if (!parser.ParseFile(source_content, false)) {
    fprintf(stderr, "could not parse %s as tt source\n",
            source_content->id().ToString().c_str());
    return 1;
  }
  std::string cc_path = absl::StrCat(dest_file_prefix, ".cc");
  auto* cc_out = fopen(cc_path.c_str(), "w");
  if (cc_out == nullptr) {
    perror(absl::StrCat("could not open ", cc_path, ": ").c_str());
    return 1;
  }
  AutoUnlink unlink_cc(cc_path);
  std::string h_path = absl::StrCat(dest_file_prefix, ".h");
  auto* h_out = fopen(h_path.c_str(), "w");
  if (h_out == nullptr) {
    perror(absl::StrCat("could not open ", h_path, ": ").c_str());
    return 1;
  }
  AutoUnlink unlink_h(h_path);
  if (!anodyne::TtGenerator::GenerateCode(parser, h_path, h_out, cc_out)) {
    return 1;
  }
  if (fclose(cc_out) != 0) {
    perror(absl::StrCat("could not close ", cc_path, ": ").c_str());
    return 1;
  }
  if (fclose(h_out) != 0) {
    perror(absl::StrCat("could not close ", h_path, ": ").c_str());
    return 1;
  }
  unlink_cc.Disarm();
  unlink_h.Disarm();
  return 0;
}

/// \brief Emit matchers from `source_content` using `dest_file_prefix`
/// to name output.
int BuildMatchers(const anodyne::File* source_content,
                  absl::string_view dest_file_prefix) {
  anodyne::TtParser parser;
  if (!parser.ParseFile(source_content, true)) {
    fprintf(stderr, "could not parse %s as matcher source\n",
            source_content->id().ToString().c_str());
    return 1;
  }
  std::string m_path = absl::StrCat(dest_file_prefix, ".matchers.h");
  auto* m_out = fopen(m_path.c_str(), "w");
  AutoUnlink unlink_m(m_path);
  if (m_out == nullptr) {
    perror(absl::StrCat("could not open ", m_path, ": ").c_str());
    return 1;
  }
  if (!anodyne::TtGenerator::GenerateMatchers(parser, m_out)) {
    return 1;
  }
  if (fclose(m_out) != 0) {
    perror(absl::StrCat("could not close ", m_path, ": ").c_str());
    return 1;
  }
  unlink_m.Disarm();
  return 0;
}

}  // anonymous namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, R"(usage: %s dest-file-prefix defs.tt
         Writes to dest-file-prefix{.cc, .h}
      %s dest-file-prefix patterns.cc
         Writes to dest-file-prefix{.matchers.h}
)",
            argv[0], argv[0]);
    return 1;
  }
  anodyne::Source source;
  anodyne::RealFileSystem fs;
  auto fs_lookup =
      [&](const anodyne::FileId& id) -> std::unique_ptr<anodyne::SourceBuffer> {
    auto content = fs.GetFileContent(id.local_path);
    if (!content) {
      return nullptr;
    }
    return absl::make_unique<anodyne::SourceBuffer>(std::move(*content),
                                                    anodyne::SourceMap{});
  };
  auto dest_file_prefix = absl::string_view(argv[1]);
  auto source_file = absl::string_view(argv[2]);
  const auto* source_content = source.FindFile("", source_file, "", fs_lookup);
  if (!source_content) {
    fprintf(stderr, "could not open %s\n", argv[2]);
    return 1;
  }
  if (source_file.rfind(".tt") != absl::string_view::npos) {
    return BuildTreeDefs(source_content, dest_file_prefix);
  }
  return BuildMatchers(source_content, dest_file_prefix);
}
