# #C 0.2

> **Note:** The `0.2` release was accidentally published as `0.3`.
> The real `0.3` release will use the commit/tag `v0.3`.

## Language overview

#C is a C-like programming language that transpiles to C++.

Its syntax is intentionally close to C/C++, so many C/C++ constructs can be used directly or are supported through the generic parser/transpiler.

#C does not have generics or operator overloading.

---

## Structs

#C provides two kinds of data-only structs:

### `struct`

`struct` types are non-copyable.

```#C
struct Player {
    int x;
    int y;
};
```

### `cstruct`

`cstruct` types are copyable.

```#C
cstruct Point {
    int x;
    int y;
};
```

Structs cannot contain methods.

---

## `defer`

`defer` provides simple LIFO scope-exit execution.

Deferred statements are executed in reverse order.

```#C
defer {...};
defer {...};
```

`defer` is intended as a simple scope-exit mechanism rather than a complete exception/error-handling system.

---

## Templates

#C templates are simple textual search-and-replace templates.

```#C
@template name T
...
@templateend
```

Template arguments are substituted textually.

Templates are **not generics** and do not perform type-level instantiation.

### `<typeonly>`

`<typeonly>` restricts a template argument to a type.

### `<strsafe>`

`<strsafe>` prevents template substitution inside string and character literals.

These properties provide additional safety when performing textual substitution.

---

## Types

#C supports primitive types such as:

```text
int
uint
float
bool
char
ubyte
ibyte
fmax
imax
umax
void
```

It also supports pointers and arrays/slices.

### `typedef`

Type aliases use:

```#C
typedef name type;
```

Example:

```#C
typedef Number int;
```

which represents an alias named `Number` for `int`.

### `enum`

Enums are supported:

```#C
enum Color {
    red,
    green,
    blue
};
```

---

## Constants

`const` is supported as a type qualifier.

```#C
const int x = 10;
```

---

## Pointers

Pointers use C-style syntax:

```#C
int* ptr;
```

#C does not use C++ references as a language feature.

---

## Control flow

#C supports C/C++-style control flow, including:

* `if`
* `elif`
* `else`
* `for`
* `while`
* `do`
* `switch`
* `case`
* `default`
* `break`
* `continue`
* `return`

Example:

```#C
if (x > 0) {
    ...
} elif (x == 0) {
    ...
} else {
    ...
}
```

`elif` is provided directly by #C and corresponds to an `else if` chain.

---

## Operators and expressions

#C supports a broad range of C/C++ operators and expressions through its generic expression parser.

This includes, among others:

* arithmetic operators
* comparison operators
* logical operators
* bitwise operators
* assignment operators
* compound assignment operators such as `+=`, `-=`, `*=`, etc.
* increment/decrement operators
* indexing
* member access `.`
* pointer member access `->`
* function calls
* the ternary operator `?:`

The language does not require a separate compiler implementation for every C/C++ operator; many operators are handled generically and emitted to C++.

---

## Casts

Casts are planned for a future version.

---

## Functions

Functions use:

```#C
fn name(arguments) return_type {
    ...
}
```

Example:

```#C
fn add(int a, int b) int {
    return a + b;
}
```

The language also supports `extern` declarations.

---

## `@c++`

`@c++` allows raw C++ code to be embedded directly into #C source.

```#C
@c++
int helper(int x) {
    return x * 2;
}
@c++end
```

Embedded C++ is copied to the generated C++ source.

When a C++ function is intended to be called from #C, it must use:

```cpp
extern "C"
```

Example:

```#C
@c++
extern "C" int helper(int x) {
    return x * 2;
}
@c++end
```

---

## Modules and imports

#C has a module API system.

`@import` extracts the public declarations from a module's:

```#C
module api {
    ...
}
```

Example:

```#C
module api {

    fn name(int x) int;

}

fn name(int x) int {
    ...
}
```

The declaration inside `module api` is public, while the implementation outside it remains private to the module.

This allows modules to expose an API without exposing their implementation.

---

## Name mangling

#C uses name mangling for qualified names.

For example:

```text
a:bc:defg
```

becomes:

```text
_I1a2bc4defg
```

The number before each component represents the length of that component.

#C does not support function overloading.

Instead, different functions can use distinct names:

```#C
fn abs:int(int x) int {
    ...
}

fn abs:float(float x) float {
    ...
}
```

This allows type-specific functions without requiring overload resolution.

---

## C/C++ compatibility

Because #C transpiles to C++, many constructs from C and C++ are available without requiring a dedicated #C feature.

The compiler parses and emits many expressions and statements generically rather than implementing every C/C++ construct as a special-case language feature.

This means that compatibility with C/C++ is broader than the explicitly documented #C-specific constructs.

#C therefore intentionally avoids duplicating every feature of C/C++ with separate syntax when the existing syntax can already be handled by the generic compiler.

---

## Design philosophy

#C is intended to remain relatively close to C/C++ while providing its own syntax and compiler features where they are useful.

The language currently avoids:

* generics
* operator overloading
* C++ references
* unnecessary high-level abstractions

Many C/C++ features are instead supported directly through #C's existing syntax and generic transpilation system.
