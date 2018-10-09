# Build against Kythe master.  Run `bazel sync` to update to the latest commit.
http_archive(
    name = "io_kythe",
    strip_prefix = "kythe-master",
    urls = ["https://github.com/google/kythe/archive/master.zip"],
)

load("@io_kythe//:setup.bzl", "kythe_rule_repositories")

kythe_rule_repositories()

load("@io_kythe//:external.bzl", "kythe_dependencies")

bind(
    name = "zlib",  # required by @com_google_protobuf
    actual = "@net_zlib//:zlib",
)

kythe_dependencies()
