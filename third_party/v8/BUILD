licenses(["notice"])

# The V8 API is necessary to make use of generated heap snapshots.
package(default_visibility = ["//visibility:public"])

exports_files([
    "snapshot/snapshot_blob.bin",
    "snapshot/natives_blob.bin",
])

cc_library(
    name = "v8",
    # Do not sort; srcs are link-order dependent.
    srcs = [
      "lib/libv8_libplatform.a",
      "lib/libv8_base.a",
      "lib/libv8_libbase.a",
      "lib/libv8_external_snapshot.a",
      "lib/libv8_libsampler.a",
      "lib/third_party/icu/libicui18n.a",
      "lib/third_party/icu/libicuuc.a",
      "lib/src/inspector/libinspector.a",
    ],
    hdrs = glob([
        "include/*.h",
        "include/libplatform/*.h",
    ]),
    includes = ["include"],
    linkopts = [
        "-lpthread",
    ],
    linkstatic = 1,
    alwayslink = 1,
)
