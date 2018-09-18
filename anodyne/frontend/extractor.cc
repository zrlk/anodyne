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

// Attempts to extract one or more compilation units from the (root) of a
// provided directory.
//   eg: extractor ../npm_project

#include "anodyne/base/fs.h"
#include "anodyne/js/npm_extractor.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"
#include "kythe/cxx/common/kzip_writer.h"

DEFINE_string(kzip, "kzip archive to write; must not currently exist.", "");

namespace anodyne {
namespace {

int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  google::InitGoogleLogging(argv[0]);
  gflags::SetVersionString("0.1");
  gflags::SetUsageMessage("extractor ../npm_project");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::vector<std::string> final_args(argv, argv + argc);
  if (final_args.size() != 2) {
    fprintf(stderr, "expected a single positional argument\n");
    return 1;
  }
  if (FLAGS_kzip.empty()) {
    fprintf(stderr, "no --kzip provided.\n");
    return 1;
  }
  auto index_writer = kythe::KzipWriter::Create(FLAGS_kzip);
  if (!index_writer) {
    std::cerr << "couldn't open kzip at " << FLAGS_kzip << ": "
              << index_writer.status() << std::endl;
    return 1;
  }
  RealFileSystem fs;
  NpmExtractor extractor;
  return extractor.Extract(&fs, std::move(*index_writer), final_args[1]) ? 0
                                                                         : 1;
}

}  // anonymous namespace
}  // namespace anodyne

int main(int argc, char* argv[]) { return anodyne::main(argc, argv); }
