package(default_visibility = ["//anodyne:default_visibility"])

load(":pack_file.bzl", "pack_file")
load(":v8_heap_gen.bzl", "v8_heap_gen")
load(":lexyacc.bzl", "genlex", "genyacc")

genyacc(
    name = "tt_yacc",
    src = "tt.y",
    extra_outs = [
        "stack.hh",
    ],
    header_out = "tt.y.hh",
    source_out = "tt.y.cc",
)

genlex(
    name = "tt_lex",
    src = "tt.l",
    out = "tt.l.cc",
    includes = [":tt_yacc"],
)

cc_library(
    name = "tt_lib",
    srcs = [
        "tt_parser.cc",
        ":tt_lex",
        ":tt_yacc",
    ],
    hdrs = [
        "tt_bison_support.h",
        "tt_parser.h",
        ":tt_yacc",
    ],
    deps = [
        "//anodyne/base:source",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "pack_file",
    srcs = ["pack_file.cc"],
    deps = ["//anodyne/base:fs"],
)

pack_file(
    name = "test_pack",
    src = "test_pack.txt",
    out_prefix = "test_pack",
    symbol = "TestPack",
)

cc_test(
    name = "pack_file_test",
    srcs = ["pack_file_test.cc"],
    deps = [
        ":test_pack",
        "//third_party:gtest_main",
    ],
)

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
