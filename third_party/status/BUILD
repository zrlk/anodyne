package(default_visibility = ["//anodyne:default_visibility"])

licenses(["notice"])

cc_library(
    name = "status",
    srcs = ["status.cc"],
    hdrs = ["status.h"],
    deps = [
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "status_or",
    hdrs = ["status_or.h"],
    deps = [
        ":status",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
    ],
)
