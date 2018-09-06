package(default_visibility = ["//anodyne:default_visibility"])

cc_library(
    name = "fs",
    srcs = [
        "fs.cc",
        "paths.cc",
    ],
    hdrs = [
        "fs.h",
        "paths.h",
    ],
    deps = [
        "//third_party/status:status_or",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
    ],
)

cc_library(
    name = "source_map",
    srcs = ["source_map.cc"],
    hdrs = ["source_map.h"],
    deps = [
        ":fs",
        "@com_github_google_glog//:glog",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "source_map_test",
    srcs = ["source_map_test.cc"],
    deps = [
        ":source_map",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "source_buffer",
    srcs = ["source_buffer.cc"],
    hdrs = ["source_buffer.h"],
    deps = [
        ":source_map",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "source_buffer_test",
    srcs = ["source_buffer_test.cc"],
    deps = [
        ":source_buffer",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "source",
    srcs = ["source.cc"],
    hdrs = ["source.h"],
    deps = [
        ":source_buffer",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "source_test",
    srcs = ["source_test.cc"],
    deps = [
        ":source",
        ":source_buffer",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "symbol_table",
    hdrs = ["symbol_table.h"],
    deps = [
        ":base",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "symbol_table_test",
    srcs = ["symbol_table_test.cc"],
    deps = [
        ":base",
        ":symbol_table",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "paths_test",
    srcs = ["paths_test.cc"],
    deps = [
        ":fs",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "base",
    hdrs = [
        "arena.h",
        "trees.h",
    ],
)

cc_test(
    name = "arena_test",
    srcs = ["arena_test.cc"],
    deps = [
        ":base",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "context",
    hdrs = [
        "context.h",
    ],
    srcs = [
        "context.cc",
    ],
    deps = [
        ":base",
        ":source",
        ":symbol_table",
    ],
)
