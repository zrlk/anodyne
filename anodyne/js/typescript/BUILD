package(default_visibility = ["//anodyne:default_visibility"])

load("//anodyne/tools:v8_heap_gen.bzl", "v8_heap_gen")

# Build a TypeScript project file that works inside the Bazel build tree.
genrule(
    name = "tsconfig",
    srcs = [
        "tsconfig.json.in",
        "index.ts",
        "//third_party/typescript:compiler/core.ts",
    ],
    outs = ["tsconfig.json"],
    cmd = "$(location :bazel_tsconfig.sh) $(location :tsconfig.json) $(location //third_party/typescript:compiler/core.ts) $(location :index.ts)",
    tools = ["bazel_tsconfig.sh"],
    visibility = ["//visibility:private"],
)

# Build the TypeScript compiler along with our code for interfacing with it.
genrule(
    name = "index_js",
    srcs = [
        "tsconfig.json",
        "index.ts",
        "//third_party/typescript:compiler/core.ts",
        "//third_party/typescript:compiler/performance.ts",
        "//third_party/typescript:compiler/sys.ts",
        "//third_party/typescript:compiler/types.ts",
        "//third_party/typescript:compiler/scanner.ts",
        "//third_party/typescript:compiler/parser.ts",
        "//third_party/typescript:compiler/utilities.ts",
        "//third_party/typescript:compiler/binder.ts",
        "//third_party/typescript:compiler/symbolWalker.ts",
        "//third_party/typescript:compiler/checker.ts",
        "//third_party/typescript:compiler/factory.ts",
        "//third_party/typescript:compiler/visitor.ts",
        "//third_party/typescript:compiler/transformers/utilities.ts",
        "//third_party/typescript:compiler/transformers/ts.ts",
        "//third_party/typescript:compiler/transformers/jsx.ts",
        "//third_party/typescript:compiler/transformers/esnext.ts",
        "//third_party/typescript:compiler/transformers/es2017.ts",
        "//third_party/typescript:compiler/transformers/es2016.ts",
        "//third_party/typescript:compiler/transformers/es2015.ts",
        "//third_party/typescript:compiler/transformers/es5.ts",
        "//third_party/typescript:compiler/transformers/generators.ts",
        "//third_party/typescript:compiler/transformers/destructuring.ts",
        "//third_party/typescript:compiler/transformers/module/module.ts",
        "//third_party/typescript:compiler/transformers/module/system.ts",
        "//third_party/typescript:compiler/transformers/module/es2015.ts",
        "//third_party/typescript:compiler/transformer.ts",
        "//third_party/typescript:compiler/comments.ts",
        "//third_party/typescript:compiler/sourcemap.ts",
        "//third_party/typescript:compiler/declarationEmitter.ts",
        "//third_party/typescript:compiler/emitter.ts",
        "//third_party/typescript:compiler/watchUtilities.ts",
        "//third_party/typescript:compiler/program.ts",
        "//third_party/typescript:compiler/builder.ts",
        "//third_party/typescript:compiler/resolutionCache.ts",
        "//third_party/typescript:compiler/watch.ts",
        "//third_party/typescript:compiler/commandLineParser.ts",
        "//third_party/typescript:compiler/diagnosticInformationMap.generated.ts",
        "//third_party/typescript:compiler/moduleNameResolver.ts",
        "//third_party/typescript:lib/lib.d.ts",
    ],
    outs = ["index.js"],
    # tsc looks up paths relative to the location of tsconfig.json.
    # bazel likes paths to be relative to the workspace root.
    cmd = "cp $(location :tsconfig.json) . && $(location //third_party/typescript:node) $(location //third_party/typescript:tsc.js)",
    tools = [
        "//third_party/typescript:node",
        "//third_party/typescript:tsc.js",
    ],
    visibility = ["//visibility:private"],
)

v8_heap_gen(
    name = "frontend",
    src = "index.js",
    out_prefix = "frontend",
)
