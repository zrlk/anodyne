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
