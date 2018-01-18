# Anodyne: Type analysis of dynamic languages

NOTE: This is not an official Google product.

Anodyne is an open-source experimental project to implement a type analysis tool
for commonly used manifestly-typed imperative programming languages.

## Building

Anodyne can be built with Bazel. There are some additional dependencies that
are not managed by Bazel that you must provide. They may be hard- or soft-linked
to these locations:

  * v8 (7dbfec50e3f2f66a3d0035f2eff91c5bdf472a0e via [depot_tools](http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up))
    * libraries in third_party/v8/lib (linking v8/out.gn/x64.release/obj is OK)
    * headers in third_party/v8/include (linking v8/include is OK)
    * Be sure to build using [GN](https://github.com/v8/v8/wiki/Building-with-GN): (`tools/dev/v8gen.py x64.release -- v8_monolithic=true v8_use_external_startup_data=false && ninja -C out.gn/x64.release`)
  * typescript (8173733ead1bf6e72397cd68e522fe25423a56a4 via https://github.com/Microsoft/TypeScript.git)
    * compiler source in third_party/typescript/compiler (linking typescript/src/compiler is OK)
    * compiler libraries in third_party/typescript/lib (linking typescript/lib is OK)
    * tsc.js in third_party/typescript/tsc.js (linking typescript/built/local/tsc.js is OK)
    * node capable of running tsc.js in third_party/typescript/node
