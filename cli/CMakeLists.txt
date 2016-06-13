# Copyright 2016 Google Inc. All rights reserved.
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

add_executable(cli cli.cc)
target_link_libraries(cli util lua_lex_parse)
IF(NOT MSVC)
  target_link_libraries(cli ${Z3_BUILD_PATH}/libz3.a)
ELSE()
  target_link_libraries(cli debug ${Z3_BUILD_PATH}/Debug/libz3.lib)
  target_link_libraries(cli optimized ${Z3_BUILD_PATH}/Release/libz3.lib)
ENDIF()
