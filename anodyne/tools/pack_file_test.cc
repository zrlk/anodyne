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

#include "anodyne/tools/test_pack.h"
#include "gtest/gtest.h"

TEST(PackFile, SmokeTest) {
  EXPECT_EQ(3, TestPack_length);
  EXPECT_EQ('f', TestPack[0]);
  EXPECT_EQ('o', TestPack[1]);
  EXPECT_EQ('o', TestPack[2]);
}
