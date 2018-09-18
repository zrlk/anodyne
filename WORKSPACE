http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "94ad0467a0de3331de86216cbc05636051be274bf2160f6e86f07345213ba45b",
    strip_prefix = "gflags-77592648e3f3be87d6c7123eb81cbad75f9aef5a",
    url = "https://github.com/gflags/gflags/archive/77592648e3f3be87d6c7123eb81cbad75f9aef5a.zip",
)

http_archive(
    name = "com_github_google_glog",
    sha256 = "f8c9457f351775181f0b99c342baf7ec99383f27bc1bdf6fac9e427411542d63",
    strip_prefix = "glog-028d37889a1e80e8a07da1b8945ac706259e5fd8",
    url = "https://github.com/google/glog/archive/028d37889a1e80e8a07da1b8945ac706259e5fd8.zip",
)

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-c742b72354a84958b6a061755249822eeef87d06",
    url = "https://github.com/abseil/abseil-cpp/archive/c742b72354a84958b6a061755249822eeef87d06.zip",
)

http_archive(
    name = "com_google_googletest",
    sha256 = "89cebb92b9a7eb32c53e180ccc0db8f677c3e838883c5fbd07e6412d7e1f12c7",
    strip_prefix = "googletest-d175c8bf823e709d570772b038757fadf63bc632",
    url = "https://github.com/google/googletest/archive/d175c8bf823e709d570772b038757fadf63bc632.zip",
)

new_http_archive(
    name = "com_github_tencent_rapidjson",
    build_file = "third_party/rapidjson.BUILD",
    sha256 = "8e00c38829d6785a2dfb951bb87c6974fa07dfe488aa5b25deec4b8bc0f6a3ab",
    strip_prefix = "rapidjson-1.1.0",
    url = "https://github.com/Tencent/rapidjson/archive/v1.1.0.zip",
)

new_http_archive(
    name = "net_zlib",
    build_file = "third_party/zlib.BUILD",
    sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
    strip_prefix = "zlib-1.2.11",
    urls = [
        "https://zlib.net/zlib-1.2.11.tar.gz",
    ],
)

bind(
    name = "zlib",  # required by @com_google_protobuf
    actual = "@net_zlib//:zlib",
)

# Required by com_google_protobuf.
http_archive(
    name = "bazel_skylib",
    sha256 = "ca4e3b8e4da9266c3a9101c8f4704fe2e20eb5625b2a6a7d2d7d45e3dd4efffd",
    strip_prefix = "bazel-skylib-0.5.0",
    urls = ["https://github.com/bazelbuild/bazel-skylib/archive/0.5.0.zip"],
)

new_http_archive(
    name = "com_google_protobuf",
    build_file = "third_party/protobuf.BUILD",
    sha256 = "08608786f26c2ae4e5ff854560289779314b60179b5df824836303e2c0fae407",
    strip_prefix = "protobuf-964201af37f8a0009440a52a30a66317724a52c3",
    urls = ["https://github.com/google/protobuf/archive/964201af37f8a0009440a52a30a66317724a52c3.zip"],
)

http_archive(
    name = "io_kythe",
    url = "https://github.com/google/kythe/archive/b1b1ca47db19c26888c2c065f3031b50fa94a486.zip",
    strip_prefix = "kythe-b1b1ca47db19c26888c2c065f3031b50fa94a486",
)

# Required by io_kythe
http_archive(
    name = "io_bazel_rules_go",
    sha256 = "8b68d0630d63d95dacc0016c3bb4b76154fe34fca93efd65d1c366de3fcb4294",
    url = "https://github.com/bazelbuild/rules_go/releases/download/0.12.1/rules_go-0.12.1.tar.gz",
)

new_http_archive(
    name = "com_github_tencent_rapidjson",
    build_file = "third_party/rapidjson.BUILD",
    sha256 = "8e00c38829d6785a2dfb951bb87c6974fa07dfe488aa5b25deec4b8bc0f6a3ab",
    strip_prefix = "rapidjson-1.1.0",
    url = "https://github.com/Tencent/rapidjson/archive/v1.1.0.zip",
)

# Required by io_kythe
new_http_archive(
    name = "org_libzip",
    build_file = "third_party/libzip.BUILD",
    sha256 = "a5d22f0c87a2625450eaa5e10db18b8ee4ef17042102d04c62e311993a2ba363",
    strip_prefix = "libzip-rel-1-5-1",
    urls = [
        # Bazel does not like the official download link at libzip.org,
        # so use the GitHub release tag.
        "https://github.com/nih-at/libzip/archive/rel-1-5-1.zip",
    ],
)

# Required by io_kythe
http_archive(
    name = "boringssl",  # Must match upstream workspace name.
    # Gitiles creates gzip files with an embedded timestamp, so we cannot use
    # sha256 to validate the archives.  We must rely on the commit hash and https.
    # Commits must come from the master-with-bazel branch.
    url = "https://boringssl.googlesource.com/boringssl/+archive/4be3aa87917b20fedc45fa1fc5b6a2f3738612ad.tar.gz",
)

new_http_archive(
    name = "com_github_tencent_rapidjson",
    build_file = "third_party/rapidjson.BUILD",
    sha256 = "8e00c38829d6785a2dfb951bb87c6974fa07dfe488aa5b25deec4b8bc0f6a3ab",
    strip_prefix = "rapidjson-1.1.0",
    url = "https://github.com/Tencent/rapidjson/archive/v1.1.0.zip",
)
