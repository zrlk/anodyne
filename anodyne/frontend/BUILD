package(default_visibility = ["//anodyne:default_visibility"])

cc_binary(
    name = "extractor",
    srcs = [
        "extractor.cc",
    ],
    deps = [
        "//anodyne/js:npm_extractor",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/strings",
        "@io_kythe//kythe/cxx/common:kzip_writer",
        "@io_kythe//kythe/cxx/common/indexing:output",
    ],
)
