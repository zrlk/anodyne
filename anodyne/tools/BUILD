load(":v8_heap_gen.bzl", "v8_heap_gen")

cc_binary(
    name = "v8_heap_gen",
    srcs = ["v8_heap_gen.cc"],
    deps = ["//third_party/v8"],
)

v8_heap_gen(
    name = "test_heap",
    src = "v8_heap_gen_test.js",
    out_prefix = "test_heap",
)

cc_test(
    name = "v8_heap_gen_test",
    srcs = ["v8_heap_gen_test.cc"],
    deps = [
        ":test_heap",
        "//third_party:gtest_main",
    ],
)
