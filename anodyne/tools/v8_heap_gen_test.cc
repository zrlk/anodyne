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

#include "anodyne/tools/test_heap.h"
#include "gtest/gtest.h"
#include "libplatform/libplatform.h"
#include "v8.h"

// Check to see if we can run a V8 example with a v8_heap_gen snapshot.
TEST(V8HeapGen, SmokeTest) {
  using namespace v8;
  Platform* platform = platform::CreateDefaultPlatform();
  V8::InitializePlatform(platform);
  V8::Initialize();
  Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  create_params.snapshot_blob = &kIsolateInitBlob;
  Isolate* isolate = Isolate::New(create_params);
  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = Context::New(isolate);
    Context::Scope context_scope(context);
    Local<String> source =
        String::NewFromUtf8(isolate, "hello()", NewStringType::kNormal)
            .ToLocalChecked();
    Local<Script> script = Script::Compile(context, source).ToLocalChecked();
    Local<Value> result = script->Run(context).ToLocalChecked();
    String::Utf8Value utf8(result);
    EXPECT_TRUE(::strcmp(*utf8, "Hello, world!") == 0);
  }
  isolate->Dispose();
  V8::Dispose();
  V8::ShutdownPlatform();
  delete platform;
  delete create_params.array_buffer_allocator;
}
