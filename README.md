# Anodyne: Type analysis of dynamic languages

NOTE: This is not an official Google product.

Anodyne is an open-source experimental project to implement a type analysis tool
for commonly used manifestly-typed imperative programming languages.

## Building

Anodyne can be built with Bazel. There are some additional dependencies that
are not managed by Bazel that you must provide. They may be hard- or soft-linked
to these locations:

  * v8 (17a6ec1b88f39beaa24a31c132ee27bc024c2d6b via [depot_tools](http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up))
    * libraries in third_party/v8/lib (linking v8/out.gn/x64.release/obj is OK)
    * snapshots in third_party/v8/snapshot (linking v8/out.gn/x64.release is OK)
    * headers in third_party/v8/include (linking v8/include is OK)
