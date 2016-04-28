/*
 * Copyright 2016 Google Inc. All rights reserved.
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

#include <iostream>
#include "lexparse/lua_parser.h"
#include "util/pretty_printer.h"
#include "util/trees.h"

int main(int argc, char** argv) {
  st::Arena arena;
  st::SymbolTable symbol_table;
  lexparse::LuaParser parser(&arena, &symbol_table, true, true);
  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    if (std::cin.eof() || std::cin.bad()) {
      break;
    }
    if (!parser.ParseString(line, "stdin")) {
      std::cerr << "(Parse error.)\n";
    }
  }
  return 0;
}
