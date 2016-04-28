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

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 11)
include_directories(${CMAKE_BINARY_DIR} .)

set(Z3_BUILD_PATH "${CMAKE_SOURCE_DIR}/../z3/build" CACHE STRING "Path to z3 build directory")

add_subdirectory(lexparse)
add_subdirectory(cli)
add_subdirectory(util)
