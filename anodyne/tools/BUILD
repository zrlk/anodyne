load(":v8_heap_gen.bzl", "v8_heap_gen")

cc_binary(
    name = "v8_heap_gen",
    srcs = ["v8_heap_gen.cc"],
    deps = ["//third_party/v8"],
)

v8_heap_gen(
    name = "base_heap",
    out_prefix = "base_heap",
)

cc_test(
    name = "v8_heap_gen_test",
    srcs = ["v8_heap_gen_test.cc"],
    deps = [
        ":base_heap",
        "//third_party:gtest_main",
    ],
)
