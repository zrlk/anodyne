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

function Parse(js: string) {
  const resultFile = ts.createSourceFile("input.js", js, ts.ScriptTarget.Latest, /*setParentNodes*/ false);
  // It's possible that it would be better to return the object graph directly
  // and crawl over it using the V8 client API. Initial experiments suggested
  // that this would involve a lot of fighting with handles.
  return JSON.stringify(resultFile);
}
