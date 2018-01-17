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

#include "libplatform/libplatform.h"
#include "v8.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>

// v8_heap_gen output/prefix image.js
// produces output/prefix.h and output/prefix.cc, which provide
// the global `v8::StartupData kIsolateInitBlob`. This contains the isolate heap
// resulting from image.js.

namespace {
void GetFileContent(const char* filename, std::string* out) {
  int fd = ::open(filename, 0);
  if (fd < 0) {
    ::fprintf(stderr, "can't open %s\n", filename);
    ::exit(1);
  }
  struct stat fd_stat;
  if (::fstat(fd, &fd_stat) < 0) {
    ::fprintf(stderr, "can't stat %s\n", filename);
    ::exit(1);
  }
  out->resize(fd_stat.st_size);
  if (::read(fd, const_cast<char*>(out->data()), fd_stat.st_size) !=
      fd_stat.st_size) {
    ::fprintf(stderr, "can't read %s\n", filename);
    ::exit(1);
  }
  ::close(fd);
}
void EmitData(FILE* f, const std::string& name, const char* data, size_t len) {
  ::fprintf(f, "namespace {\n");
  ::fprintf(f, "  const unsigned char k%sData[] = {", name.c_str());
  for (const auto* p = data; p < data + len; ++p) {
    if (p != data) {
      ::fprintf(f, ", ");
    }
    ::fprintf(f, "0x%x", static_cast<unsigned char>(*p));
  }
  ::fprintf(f, "};\n\n");
  ::fprintf(f, "  const size_t k%sLen = %zu;\n", name.c_str(), len);
  ::fprintf(f, "}\n\n");
  ::fprintf(f, "v8::StartupData k%sBlob = {(const char*)k%sData, k%sLen};\n\n",
            name.c_str(), name.c_str(), name.c_str());
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    ::fprintf(stderr, "usage: %s output/prefix image.js", argv[0]);
    return 1;
  }
  std::string image;
  GetFileContent(argv[2], &image);

  v8::Platform* platform = v8::platform::CreateDefaultPlatform();
  v8::V8::InitializePlatform(platform);
  v8::V8::Initialize();
  v8::StartupData data = v8::V8::CreateSnapshotDataBlob(image.c_str());

  std::string cc_name = argv[1];
  std::string h_name = argv[1];
  cc_name.append(".cc");
  h_name.append(".h");
  FILE* cc = ::fopen(cc_name.c_str(), "wb");
  if (!cc) {
    ::fprintf(stderr, "can't open %s for writing", cc_name.c_str());
    return 1;
  }
  FILE* h = ::fopen(h_name.c_str(), "wb");
  if (!h) {
    ::fprintf(stderr, "can't open %s for writing", h_name.c_str());
    return 1;
  }
  ::fprintf(h, "#include \"v8.h\"\n\n");
  ::fprintf(h,
            "/// \\brief `StartupData` for starting an isolate with a prebuilt "
            "heap.\n");
  ::fprintf(h, "extern v8::StartupData kIsolateInitBlob;\n");
  ::fprintf(cc, "#include \"v8.h\"\n\n");
  EmitData(cc, "IsolateInit", data.data, data.raw_size);
  if (::fclose(cc)) {
    ::fprintf(stderr, "can't close %s\n", cc_name.c_str());
    return 1;
  }
  if (::fclose(h)) {
    ::fprintf(stderr, "can't close %s\n", h_name.c_str());
    return 1;
  }
  return 0;
}
