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

find_package(BISON 3.0.2)
find_package(FLEX 2.5.39)

BISON_TARGET(lua_parse_cc lua.y ${CMAKE_CURRENT_BINARY_DIR}/lua_parse.cc VERBOSE ${CMAKE_CURRENT_BINARY_DIR}/lua_parse.output)
FLEX_TARGET(lua_lex_cc lua.l ${CMAKE_CURRENT_BINARY_DIR}/lua_lex.cc)
ADD_FLEX_BISON_DEPENDENCY(lua_lex_cc lua_parse_cc)
add_library(lua_lex_parse ${BISON_lua_parse_cc_OUTPUTS} ${FLEX_lua_lex_cc_OUTPUTS} lua_parser.cc)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/lua_parse.cc PROPERTIES COMPILE_FLAGS -Wno-deprecated-register)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/lua_lex.cc PROPERTIES COMPILE_FLAGS -Wno-deprecated-register)
target_link_libraries(lua_lex_parse ast util)
