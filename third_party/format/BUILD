package(default_visibility = ["//visibility:public"])

licenses(["restricted"])  # LGPL

filegroup(
    name = "license",
    srcs = ["LICENSE"],
)

cc_library(
    name = "format",
    srcs = [
        "format.cc",
    ],
    hdrs = [
        "format.h",
    ],
    deps = [
        "@com_google_absl//absl/strings",
    ],
)
