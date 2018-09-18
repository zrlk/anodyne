package(default_visibility = ["//anodyne:default_visibility"])

cc_library(
    name = "npm_utils",
    srcs = ["npm_package.cc"],
    hdrs = ["npm_package.h"],
    deps = [
        "@com_github_google_glog//:glog",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "npm_extractor",
    srcs = ["npm_extractor.cc"],
    hdrs = ["npm_extractor.h"],
    deps = [
        ":npm_utils",
        "//anodyne/base:digest",
        "//anodyne/base:fs",
        "//anodyne/base:source_map",
        "//anodyne/extract",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@io_kythe//kythe/cxx/common/indexing:output",
    ],
)

cc_test(
    name = "npm_extractor_test",
    srcs = ["npm_extractor_test.cc"],
    deps = [
        ":npm_extractor",
        "//anodyne/base:memfs",
        "@com_google_googletest//:gtest_main",
    ],
)
