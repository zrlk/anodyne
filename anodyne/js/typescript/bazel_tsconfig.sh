#!/bin/bash
# Substitutes paths in tsconfig.json.in to produce a new project file.
# Paths should be relative to the workspace root.
#
# Example:
# bazel_tsconfig.sh \
#     bazel-out/k8-fastbuild/genfiles/anodyne/js/typescript/tsconfig.json \
#     third_party/typescript/compiler/core.ts \
#     anodyne/js/typescript/index.ts
BINARY_DIR=$(dirname "$1")
TS_SOURCE_PATH=$(dirname "$2")
SOURCE_DIR=$(dirname "$3")
sed "s|\${CMAKE_CURRENT_BINARY_DIR}|${BINARY_DIR}|g
s|\${TYPESCRIPT_SOURCE_PATH}|${TS_SOURCE_PATH}|g
s|\${CMAKE_CURRENT_SOURCE_DIR}|${SOURCE_DIR}|g" < "${SOURCE_DIR}/tsconfig.json.in" > "${BINARY_DIR}/tsconfig.json"
