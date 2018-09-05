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

def tt_library(name, src, out_prefix, builds_json = False):
    """Generate C++ datatype definitions from a tt definition.

    Args:
      name: The name of the rule.
      src: The .tt source file.
      out_prefix: The generated source file prefix (to which .h and .cc will be
                  appended).
      builds_json: Whether any of the definitions in the .tt require json
                   support.
    """
    native.genrule(
        name = name + "_tt_gen",
        srcs = [src],
        tools = ["//anodyne/tools:tt"],
        cmd = "./$(location //anodyne/tools:tt) $(@D)/%s $(location %s)" % (out_prefix, src),
        outs = [out_prefix + ".cc", out_prefix + ".h"],
    )
    _json_deps = []
    if builds_json:
        _json_deps = ["//third_party/rapidjson"]
    out_header = out_prefix + ".h"
    native.cc_library(
        name = name,
        srcs = [out_prefix + ".cc"],
        hdrs = [out_header],
        includes = [out_header],
        deps = ["//anodyne/base", "//anodyne/base:symbol_table", "//anodyne/base:source"] + _json_deps,
    )

def tt_matchers(name, src):
    """Generate C++ matchers using tt.

    Args:
      name: The name of the rule.
      src: The C++ source file.
    """
    native.genrule(
      name = name + "_internal_matchers",
      srcs = [src],
      tools = ["//anodyne/tools:tt"],
      cmd = "./$(location //anodyne/tools:tt) $(@D)/%s $(location %s)" % (src, src),
      outs = [src + ".matchers.h"],
    )
    native.filegroup(
      name = name,
      srcs = [src, src + ".matchers.h"],
    )
