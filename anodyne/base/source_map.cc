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

#include "anodyne/base/source_map.h"

#include "absl/strings/str_cat.h"
#include "anodyne/base/paths.h"
#include "glog/logging.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

namespace anodyne {
namespace {
static constexpr int kDecode64[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1,
    -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1};
int64_t Decode64(absl::string_view buffer, size_t p, int64_t* out) {
  bool cont;
  int64_t accum = 0;
  int shift = 0;
  do {
    if (p >= buffer.size()) {
      return -1;
    }
    int pd = kDecode64[static_cast<unsigned int>(buffer[p++])];
    if (pd < 0) {
      return -1;
    }
    cont = (pd & 32) == 32;
    accum += (pd & 0x1f) << shift;
    shift += 5;
  } while (cont);
  *out = ((accum & 1) == 1) ? -(accum >> 1) : (accum >> 1);
  return p;
}
}  // anonymous namespace

bool SourceMap::ParseFromJson(absl::string_view friendly_id,
                              absl::string_view json, bool decode_mappings) {
  // TODO: The file is allowed to be gzip-compressed. (#14)
  // TODO: Some people will prepend )]} to the map data. (#15)
  // TODO: Multipart maps. (#16)
  decode_mappings = true;  // XXX remove this XXX
  rapidjson::Document doc;
  doc.Parse(json.data(), json.size());
  if (doc.HasParseError()) {
    LOG(WARNING) << "couldn't parse source map: "
                 << rapidjson::GetParseError_En(doc.GetParseError()) << " near "
                 << doc.GetErrorOffset();
    return false;
  }
  if (!doc.IsObject()) {
    LOG(WARNING) << friendly_id << ": source map doesn't describe an object.";
    return false;
  }
  const auto sections = doc.FindMember("sections");
  const auto version = doc.FindMember("version");
  const auto source_root = doc.FindMember("sourceRoot");
  const auto sources = doc.FindMember("sources");
  const auto sourcesContent = doc.FindMember("sourcesContent");
  const auto names = doc.FindMember("names");
  if (sections != doc.MemberEnd()) {
    LOG(WARNING) << friendly_id << ": uses unsupported sections";
    return false;
  }
  if (version != doc.MemberEnd() && version->value.IsInt64() &&
      version->value.GetInt64() != 3) {
    LOG(WARNING) << friendly_id << ": unsupported version";
    return false;
  }
  std::string root;
  if (source_root != doc.MemberEnd()) {
    if (!source_root->value.IsString()) {
      LOG(WARNING) << friendly_id << ": bad sourceRoot";
      return false;
    }
    root = source_root->value.GetString();
  }
  if (sources != doc.MemberEnd()) {
    if (!sources->value.IsArray()) {
      LOG(WARNING) << friendly_id << ": bad sources";
      return false;
    }
    const auto& array = sources->value.GetArray();
    for (const auto& v : array) {
      if (!v.IsString()) {
        LOG(WARNING) << friendly_id << ": non-string source";
        return false;
      }
      sources_.emplace_back(SourceMapFile{
          root.empty() ? v.GetString() : absl::StrCat(root, "/", v.GetString()),
          ""});
    }
  }
  if (sourcesContent != doc.MemberEnd()) {
    if (!sourcesContent->value.IsArray()) {
      LOG(WARNING) << friendly_id << ": bad sourcesContent";
      return false;
    }
    const auto& array = sourcesContent->value.GetArray();
    if (array.Size() > sources_.size()) {
      LOG(WARNING) << friendly_id << ": more content than sources";
      return false;
    }
    for (size_t i = 0; i < sources_.size(); ++i) {
      if (array[i].IsNull()) continue;
      if (!array[i].IsString()) {
        LOG(WARNING) << friendly_id << ": bad content";
        return false;
      }
      sources_[i].content = array[i].GetString();
    }
  }
  if (names != doc.MemberEnd()) {
    if (!names->value.IsArray()) {
      LOG(WARNING) << friendly_id << ": bad names";
      return false;
    }
    const auto& array = names->value.GetArray();
    for (const auto& v : array) {
      if (!v.IsString()) {
        LOG(WARNING) << friendly_id << ": bad name";
        return false;
      }
      names_.push_back(v.GetString());
    }
  }
  if (decode_mappings) {
    const auto mappings = doc.FindMember("mappings");
    if (mappings != doc.MemberEnd()) {
      if (!mappings->value.IsString()) {
        LOG(WARNING) << friendly_id << ": bad mappings";
        return false;
      }
      if (!ParseMappings(mappings->value.GetString())) {
        LOG(WARNING) << friendly_id << ": errors during decoding mappings";
        return false;
      }
    }
  }
  return true;
}

bool SourceMap::ParseMappings(absl::string_view mappings) {
  int64_t p = 0;
  int64_t fields[5];
  int current_field = 0;
  SourceMapSegment segment = {0, 0, 0, 0, 0, 0};
  bool bad_map = false;
  auto end_segment = [&] {
    segment.generated_col += fields[0];
    if (current_field > 1) {
      segment.source += fields[1];
      if (segment.source < 0 || segment.source >= sources_.size()) {
        LOG(ERROR) << "Bad segment source: " << segment.source;
        bad_map = true;
        return;
      }
      segment.source_line += fields[2];
      segment.source_col += fields[3];
      if (current_field == 5) {
        segment.name += fields[4];
        if (segment.name < 0 || segment.name >= names_.size()) {
          LOG(ERROR) << "Bad segment name: " << segment.name;
          bad_map = true;
          return;
        }
      }
    }
    segments_.push_back(segment);
    if (current_field != 5) {
      segments_.back().name = -1;
    }
    current_field = 0;
  };
  while (p < mappings.size() && !bad_map) {
    char c = mappings[p];
    if (c == ';') {
      end_segment();
      segment.generated_col = 0;
      segment.generated_line++;
      ++p;
    } else if (c == ',') {
      end_segment();
      ++p;
    } else {
      p = Decode64(mappings, p, &fields[current_field++]);
      if (p < 0) {
        return false;
      }
      if (p == mappings.size()) {
        end_segment();
      }
    }
  }
  return !bad_map;
}
}  // namespace anodyne
