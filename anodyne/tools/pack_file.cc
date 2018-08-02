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

#include "anodyne/base/fs.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

int main(int argc, char** argv) {
  if (argc != 4) {
    ::fprintf(stderr, "usage: %s output/prefix symbol binary", argv[0]);
    return 1;
  }
  std::string symbol = argv[2];
  anodyne::RealFileSystem fs;
  auto image = fs.GetFileContent(argv[3]);
  if (!image) {
    ::fprintf(stderr, "couldn't get file content: %s\n",
              image.status().ToString().c_str());
    return 1;
  }

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
  ::fprintf(h, "extern const char %s[];\n", symbol.c_str());
  ::fprintf(h, "extern const unsigned int %s_length;\n", symbol.c_str());
  ::fprintf(cc, "#include \"%s\"\n", h_name.c_str());
  ::fprintf(cc, "const char %s[] = {", symbol.c_str());
  for (const auto* p = image->data(); p < image->data() + image->size(); ++p) {
    if (p != image->data()) {
      ::fprintf(cc, ", ");
    }
    ::fprintf(cc, "0x%x", static_cast<unsigned char>(*p));
  }
  ::fprintf(cc, "};\n");
  ::fprintf(cc, "const unsigned int %s_length = %zu;\n", symbol.c_str(),
            image->size());
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
