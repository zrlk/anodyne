# tt

`tt` is a tool for generating code that builds and consumes representations of syntax graphs.

## Usage

The `tt` command performs one of two operations based on the extension of its input:

  * `tt output-prefix input.tt` reads tree definitions from `input.tt` and writes implementation files `output-prefix.cc` and `output-prefix.h`
  * `tt output-prefix input.cc` reads pattern definitions from `input.cc` and writes the implementation file `output-prefix.matchers.h`

## Tree description language

### Base types

  * An `ident` is an Anodyne `Symbol`.
  * A `range` is an Anodyne `Range`.
  * `unit` is Anodyne's `Unit`.

### Derived types
  * If `T` is a base type, `T[]` is an `ArenaSlice` of T.
  * If `T` is a base type, `T?` is an `ArenaOption` of T.
  * If `S` is a sum type, `S#` is also a sum type, but is processed specially during JSON decoding.

### Sum types

Sum types are defined as:

    type [json "context"] fully.qualified.name =
        TagA of [[label:]type [* ...]]
    [ | TagB of ... ]

For example:

    type anodyne.core.boolean = True | False

defines a sum type `anodyne.core.boolean` with two tags, `True` and `False`.

    type anodyne.core.exp =
      Ident of range * ident
    | App of range * anodyne.core.exp * anodyne.core.exp[]
    | Bool of range * anodyne.core.boolean

defines a recursive sum type with three tags.

    type json "void*" anodyne.js.exp =
      ExpressionStatement of range * expression:anodyne.js.exp
    | SourceFile of statements:anodyne.js.exp[]
    | StringLiteral of range * text:ident

defines a recursive sum type with three tags as well, but the data stored for each tag have labels that are used during JSON decoding. It also has a corresponding context of type `void*` that is passed around during decoding.

## Patterns

`tt`-generated values are eliminated using patterns, which have the general form

    match(subject-value,
         | pattern result
         | pattern result)

The grammar for patterns, described in greater detail below, is:

 * `_` : always matches regardless of subject.
 * `id` : always matches and stores the match in a new target-language variable named id.
 * `id1` `id2` : matches when the subject is a sum type with tag `id1` that has a single data item, which is stored in a new target-language variable named `id2`.
 * `id1` ( [ `pattern` [, `pattern` ] ] ) : matches when the subject is a sum type with tag `id1` that has data items matching the provided list of patterns.
 * { [ `pattern` [, `pattern` ] ] } : matches when the subject is a slice that contains data items matching the provided list of patterns.
 * Some `pattern` : matches when the subject is an option containing some data item matching `pattern`.
 * None : matches when the subject is `None`.

Patterns are evaluated in arbitrary order. When a top-level pattern is found to match the `subject-value`, the `result` expression is evaluated with any new variables bound. If no pattern matches, the program is aborted.

### Patterns in C++

Patterns are embedded directly into source files in comments marked with `/*|` and ending in `*/`. To eliminate an instance of `anodyne.core.exp`, for example, you could write:

    int branch = __match(exp,
        /*| Ident (r, i) */ 0,
        /*| App (r, f, a) */ 1,
        /*| Bool (r, b) */ 2);

As an artifact of implementation, there may only be one instance of `__match` on any given line. Furthermore, because `__match` is a macro, you must guard against the preprocessor's hunger for commas outside of parens. A light way to do this is to use a lambda-expression:

    int branch = __match(exp,
        /*| Ident (r, i) */ ([&]{ return 0; })(),
        /*| App (r, f, a) */ ([&]{ return 1; })(),
        /*| Bool (r, b) */ ([&]{ return 2; })());

(Usually this needs to be deployed only when a given branch's code is several lines long, typically because you can't write a reasonable expression and must resort to statements.)

As described in the grammar, you can use `_` to indicate that you are uninterested in the data in a particular position:

    auto loc = __match(exp,
      /*| Ident (r, _) */ r.begin(),
      /*| App _ */ Range { },
      /*| _ */ Range { });

Slices are eliminated using a comma-separated list of patterns between braces:

    /*| App  (_, _, {stmt1, stmt2})*/

Options are eliminated using the `Some` and `None` keywords:

    /*| MaybePair (Some t, None)*/

We provide a `BODY` macro:

    #define BODY(e) (([&]{ e })())

which can eliminate some line noise:

    int branch = __match(exp,
        /*| Ident (r, i) */ BODY( return 0; ),
        /*| App (r, f, a) */ BODY( return 1; ),
        /*| Bool (r, b) */ BODY( return 2; ));

## C++ interface

The sum type `anodyne.core.exp` is referred to in C++ code as `const anodyne::core::exp*`. The implementation of this type is opaque. You must destructure sum types using patterns. Sum types are immutable.

Each tag in a sum type turns into a constructor function defined in the same namespace as the sum's C++ type. For example, `anodyne::core::Ident(some_range, some_ident)` will return a `const anodyne::core::exp*` allocated in the current context's arena.

## JSON decoding

TODO: JSON decoding.
