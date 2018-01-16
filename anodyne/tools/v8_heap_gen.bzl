# Copyright 2018 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Build a V8 heap snapshot from the JavaScript files in srcs, emitting
# out_prefix.h and out_prefix.cc, which provide the function
# `::InitV8FromBuiltinBuffers()` and the global
# `v8::StartupData kIsolateInitBlob`.
def v8_heap_gen(name, srcs=[], out_prefix=''):
  if out_prefix == '':
    fail("out_prefix must not be empty")
  all_srcs = ""
  for src in srcs:
    all_srcs += "$(location %s)" % src
  native.genrule(
    name = name + "_gen",
    srcs = srcs + [
        "//third_party/v8:snapshot/snapshot_blob.bin",
        "//third_party/v8:snapshot/natives_blob.bin",
    ],
    outs = [out_prefix + ".cc", out_prefix + ".h"],
    tools = ["//anodyne/tools:v8_heap_gen"],
    cmd = "./$(location //anodyne/tools:v8_heap_gen) $(@D)/%s $(location //third_party/v8:snapshot/snapshot_blob.bin) $(location //third_party/v8:snapshot/natives_blob.bin) %s" % (out_prefix, all_srcs)
  )
  native.cc_library(
    name = name,
    srcs = [out_prefix + ".cc"],
    hdrs = [out_prefix + ".h"],
    includes = [out_prefix + ".h"],
    deps = ["//third_party/v8"],
    linkstatic = 1,
    alwayslink = 1,
  )
