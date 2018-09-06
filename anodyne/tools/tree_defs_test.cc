// This file checks that the generated code for tree definitions compiles.

#include "anodyne/tools/testdata/test_defs.tt.h"

void foo() {
  anodyne::Context context;
  anodyne::ContextBinding b(&context);
  const test::exp* e = test::Unit(anodyne::unit);
  e = test::App(e, e);
}
