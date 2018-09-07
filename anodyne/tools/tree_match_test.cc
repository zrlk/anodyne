// This file checks that the generated code for tree matchers compiles.

#include "anodyne/tools/testdata/test_defs.tt.h"
#include "anodyne/tools/tree_match_test.cc.matchers.h"

void single_level(const test::exp* e) {
  __match(e,
    /*| App (lhs, rhs) */ 0,
    /*| Lam (id, e) */ 1,
    /*| Id (id) */ 2,
    /*| Unit (u) */ 3);
}

void multi_level(const test::exp* e) {
  __match(e, /*| App (Unit (u), Unit (o)) */ 0);
}
