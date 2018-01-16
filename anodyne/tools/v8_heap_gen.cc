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

// v8_heap_gen output/prefix snapshot_blob.bin natives_blob.bin [image.js]
// produces output/prefix.h and output/prefix.cc, which provide
// `::InitV8FromBuiltinBuffers()` and the global
// `v8::StartupData kIsolateInitBlob`. These initialize V8 with its builtins
// and contain the isolate heap resulting from image.js (respectively).

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
void EmitData(FILE* f, const std::string& name, const char* data, size_t len,
              bool public_blob) {
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
  if (public_blob) {
    ::fprintf(f,
              "v8::StartupData k%sBlob = {(const char*)k%sData, k%sLen};\n\n",
              name.c_str(), name.c_str(), name.c_str());
  }
  ::fprintf(f, "namespace {\n");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 4 && argc != 5) {
    ::fprintf(stderr,
              "usage: %s output/prefix snapshot_blob.bin natives_blob.bin "
              "[image.js]\n",
              argv[0]);
    return 1;
  }
  std::string snapshot, natives, image;
  GetFileContent(argv[2], &snapshot);
  GetFileContent(argv[3], &natives);
  if (argc == 5) {
    GetFileContent(argv[4], &image);
  }
  v8::StartupData data;
  v8::StartupData v8natives, v8snapshot;
  v8natives.data = natives.data();
  v8natives.raw_size = natives.size();
  v8snapshot.data = snapshot.data();
  v8snapshot.raw_size = snapshot.size();
  v8::V8::SetNativesDataBlob(&v8natives);
  v8::V8::SetSnapshotDataBlob(&v8snapshot);
  v8::Platform* platform = v8::platform::CreateDefaultPlatform();
  v8::V8::InitializePlatform(platform);
  v8::V8::Initialize();
  data = v8::V8::CreateSnapshotDataBlob(image.c_str());

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
  ::fprintf(
      h, "/// \\brief Initialize V8 using the builtin snapshot and natives.\n");
  ::fprintf(h, "void InitV8FromBuiltinBuffers();\n\n");
  ::fprintf(h,
            "/// \\brief `StartupData` for starting an isolate with a prebuilt "
            "heap.\n");
  ::fprintf(h, "extern v8::StartupData kIsolateInitBlob;\n");
  ::fprintf(cc, "#include \"v8.h\"\n\n");
  ::fprintf(cc, "namespace {\n");
  EmitData(cc, "Natives", natives.data(), natives.size(),
           /*public_blob=*/false);
  EmitData(cc, "Snapshot", snapshot.data(), snapshot.size(),
           /*public_blob=*/false);
  EmitData(cc, "IsolateInit", data.data, data.raw_size, /*public_blob=*/true);
  ::fprintf(cc, "}\n\n");
  ::fprintf(cc, "void InitV8FromBuiltinBuffers() {\n");
  ::fprintf(
      cc,
      "  v8::StartupData natives { (const char*)kNativesData, kNativesLen "
      "};\n");
  ::fprintf(cc,
            "  v8::StartupData snapshot { (const char*)kSnapshotData, "
            "kSnapshotLen };\n");
  ::fprintf(cc, "  v8::V8::SetNativesDataBlob(&natives);\n");
  ::fprintf(cc, "  v8::V8::SetSnapshotDataBlob(&snapshot);\n");
  ::fprintf(cc, "}\n");
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
