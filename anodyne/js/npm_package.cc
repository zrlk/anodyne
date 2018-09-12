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

#include "anodyne/js/npm_package.h"
#include "glog/logging.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

namespace anodyne {

std::unique_ptr<NpmPackage> NpmPackage::ParseFromJson(
    absl::string_view friendly_id, absl::string_view json) {
  rapidjson::Document doc;
  doc.Parse(json.data(), json.size());
  if (doc.HasParseError()) {
    LOG(WARNING) << "couldn't parse package.json: "
                 << rapidjson::GetParseError_En(doc.GetParseError()) << " near "
                 << doc.GetErrorOffset();
    return nullptr;
  }
  if (!doc.IsObject()) {
    LOG(WARNING) << friendly_id << ": package.json doesn't describe an object.";
    return nullptr;
  }
  const auto npm_id = doc.FindMember("_id");
  const auto name = doc.FindMember("name");
  const auto version = doc.FindMember("version");
  const auto deps = doc.FindMember("dependencies");
  const auto main = doc.FindMember("main");
  auto package = std::unique_ptr<NpmPackage>(new NpmPackage());
  if (npm_id != doc.MemberEnd() && npm_id->value.IsString()) {
    VLOG(0) << friendly_id << ": package.json has an _id; assuming it's npm's.";
    package->npm_id_ = npm_id->value.GetString();
  }
  if (name != doc.MemberEnd() && name->value.IsString()) {
    package->name_ = name->value.GetString();
  }
  if (version != doc.MemberEnd() && version->value.IsString()) {
    package->version_ = version->value.GetString();
  }
  if (main != doc.MemberEnd() && main->value.IsString()) {
    package->main_source_file_ = main->value.GetString();
  }
  if (deps != doc.MemberEnd() && deps->value.IsObject()) {
    for (auto& v : deps->value.GetObject()) {
      if (v.name.IsString() && v.value.IsString()) {
        package->dependencies_.emplace_back();
        package->dependencies_.back().package_id = v.name.GetString();
        package->dependencies_.back().version_spec = v.value.GetString();
      }
    }
  }
  return std::move(package);
}

}  // namespace anodyne
