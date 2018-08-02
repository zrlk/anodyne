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

# Emit the content of src to symbol symbol, defined in out_prefix.h and
# out_prefix.cc.
def pack_file(name, src='', out_prefix='', symbol=''):
  if out_prefix == '':
    fail("out_prefix must not be empty")
  if src == '':
    fail("src must not be empty")
  if symbol == '':
    fail("symbol must not be empty")
  native.genrule(
    name = name + "_gen",
    srcs = [src],
    outs = [out_prefix + ".cc", out_prefix + ".h"],
    tools = ["//anodyne/tools:pack_file"],
    cmd = "./$(location //anodyne/tools:pack_file) $(@D)/%s %s $(location %s)" % (out_prefix, symbol, src)
  )
  native.cc_library(
    name = name,
    srcs = [out_prefix + ".cc"],
    hdrs = [out_prefix + ".h"],
    includes = [out_prefix + ".h"],
    linkstatic = 1,
    alwayslink = 1,
  )
