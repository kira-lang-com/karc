# Small C++ program frontend in KLF Kira

This extends the earlier arithmetic-only sketch into concrete lexer, parser, name-resolution and KIR-emission code. The demonstration is iterative Fibonacci with a function call, parameter, mutable locals and a loop. `main` returns 55; the program does not print or use libc.

```cpp
int fibonacci(int n) {
    int a = 0;
    int b = 1;
    int i = 0;
    while (i < n) {
        int next = a + b;
        a = b;
        b = next;
        i = i + 1;
    }
    return a;
}

int main() {
    return fibonacci(10);
}
```

## Files

- `token.klfd`: the `TokenKind` enum, the `reservedWord` keyword classifier and the source lexer, mirroring klf-kira-syntax's `token.rs`.
- `ast.klfd`: the flat, index-based AST nodes and the shared parser state (`peek`/`consume`/`expect` over the token stream).
- `parser_expr.klfd`, `parser_stmt.klfd`, `parser_type.klfd`, `parser_decl.klfd`: the precedence expression parser, the statement/declarator parser, the type parser and the top-level declaration parser, split as klf-kira-syntax splits `parser/{exprs,stmts,types,decls}`.
- `kir.klfd`: the KIR builder — one constructor per instruction, each calling the kernel's structural service, so lowering never names a service directly.
- `lower_core.klfd`: the `Lowerer` state, the karc-type-to-`KirType` bridge, scopes, slots and conversions.
- `lower_values.klfd`, `lower_expr.klfd`, `lower_call.klfd`, `lower_stmt.klfd`: struct copies and pointer helpers, expression lowering, call lowering, statement lowering.
- `lower_module.klfd`: the module pass — records, string literals, globals and function declarations, then one function body at a time.
- `elaborate.klfd`: the whole-tree checks the `elaborate` phase runs over the parsed tree.
- `diagnostics.klfd`: the numeric diagnostic codes, spelled out in `diagnostics.md`.
- `language.klfd`: the `construct KArc() extends Language<Tree, Program>` declaration, its declared diagnostic specs and its four phases.
- `host-contract.md`: the host/prelude functions and the remaining driver boundary.

The accepted program above lives in `../examples/fibonacci/app.cpp`. Further
examples show the wider subset:

- `../examples/strings`: idiomatic pointer-style `strlen`/`strcpy`/`strcmp`/`strchr`.
- `../examples/bytebuf`: a struct that embeds a fixed buffer, summed through its array field.
- `../examples/libc`: a broad libc unit (string, memory and ctype routines over byte pointers and `void*`, plus a struct).
- `../examples/calc`, `../examples/crc32`, `../examples/bytecode-vm`: an expression evaluator, a table-driven checksum and a small interpreter loop.

Every example is compiled through the kernel and run by the framework test
suite.

## Supported subset

A source file contains one or more definitions of `int` functions with named `int` parameters. Local variables may be declared `int`, `char`, `bool` or `long`, and fixed-size arrays of those (`T name[N];`, uninitialized) with indexed read and write `a[i]`; `char` holds a signed 8-bit value (integer conversions truncate on store and sign-extend on use) and `bool` holds a zero/nonzero value (assignment converts any int to 0 or 1); `long` is 64-bit. Expressions compute with the usual integer promotions: `char`/`bool` promote to `int`, and mixing `long` with `int` promotes to `long`, so long arithmetic avoids 32-bit overflow. Function parameters and return types may also be `int`, `char`, `bool` or `long`; a parameter may be a pointer/array (`T* p` or `T p[]`), receiving a caller's array by reference for indexed access; arguments and return values are converted to the declared type at the call and return boundaries and promoted back to `int` for use. Functions can call previously defined functions or themselves; no forward declarations or overloads are included. `main` takes no parameters, `(int argc, char** argv)`, or those and `char** envp`. Functions may also return `void`, in which case a bare `return;` or falling off the end returns implicitly. Expressions include decimal integer literals, local names, calls, parentheses, the arithmetic operators `+`, `-`, `*`, `/`, `%`, the bitwise operators `&`, `|`, `^` and the shifts `<<`, `>>` (arithmetic on signed values), the relational operators `<`, `>`, `<=`, `>=`, and the equality operators `==`, `!=`, with ordinary C precedence (multiplicative, additive, shift, relational, equality, bitwise-and, bitwise-xor, bitwise-or) and left associativity. Comparisons produce bool and promote to int when used in arithmetic, arguments, assignments or returns. Integer loop conditions use zero/nonzero conversion. Division and remainder are signed; division by zero and `INT_MIN / -1` trap at run time like the other checked operations.

Enums are supported as named integer constants: `enum Name { A, B = 5, C };` gives each enumerator a value (sequential from 0, or continuing after an explicit `= N`), usable anywhere an integer is, and `enum Name` is an `int`-typed variable. Function pointers are supported through a typedef `typedef RET (*NAME)(PARAMS);`: a parameter of that type receives a function (passed by name, which decays to a `func.ref`) and calling it (`op(args)`) lowers to a `call.indirect` with the typedef's signature. Type aliases are supported: a top-level `typedef <type> Name;` (over any built-in, pointer or `struct` type) registers `Name` as a type usable for locals, parameters, returns, globals and further typedefs. Anonymous-struct typedefs are not yet included. Structs are supported: a top-level `struct Name { T field; ... };` defines a C-layout aggregate (scalar or fixed-array fields), a local `struct Name v;` allocates one (scalar fields zero-initialized), and `v.field` reads or writes a field. Structs lower to a KIR `type ... = struct c` declaration with a trivial policy, and field access uses `field.addr`. A struct pointer parameter (`struct Name* p`) is supported, with `p->field` reading and writing the pointee's field, so a function can mutate a caller's struct passed by `&`. Whole-struct assignment (`p2 = p1;`) copies field by field. A function may return a struct by value (`struct P make(...)`), lowered with an sret out-pointer; the result is bound with `struct P p = make(...);`. A struct may also be passed by value (`int f(struct P p)`): the callee receives a pointer to the caller's copy and copies it into an independent local on entry, so mutating the parameter does not affect the caller, as C requires; the struct's fields must be scalar. Nested structs are not yet included.

A file-scope array may carry an aggregate initializer (`int table[] = {1, 2, 3};`, or `int t[5] = {1, 2};` where the rest is zero-filled), lowered to a byte-initialized global. File-scope variables are supported: a top-level `T name;`, `T name = <literal>;` or `T name[N];` declares a global of a scalar type or a fixed array. A scalar global takes an optional integer-literal initializer (decimal, hex or character; zero when omitted); arrays are zero-initialized. Globals are visible to every function, keep their value across calls, and a local or parameter of the same name shadows the global within its scope. Each global lowers to a KIR `global` declaration and is reached through `global.addr`; its storage is allocated once at program start and outlives every call.

Statements are initialized `int` local declarations, assignments, braced `while`, `for` and `do`/`while` loops, braced `if`/`else` conditionals (including `else if` chains), `switch` statements with `case`/`default` labels (fall-through unless broken), and `return`. Nested loops and conditionals are supported. `break` and `continue` exit or advance the innermost loop, with `continue` running the `for` step. `return` may appear early — inside a branch or loop — and every return path deallocates the locals before returning. Arrays may be multi-dimensional (`int a[M][N];`, and deeper), with `a[i][j]` indexing for both reading and writing; each declared dimension is a fixed bound and indexing chains `elem.addr` per subscript. A single declaration may declare several comma-separated variables of the shared base type (`int a = 1, b = 2;`, or a mix of scalars and arrays `int a = 0, buf[4];`), including in a `for` initializer; each declarator follows the single-variable rules and later ones see the earlier ones. Variables declared in a loop body have block scope and are initialized each iteration. Reusing a name after its scope ends is supported; shadowing a currently visible local is deliberately rejected. Locals and parameters are mutable.

Integer literals may be up to 64 bits; a literal that does not fit a signed 32-bit value is a 64-bit constant. Signed 32-bit arithmetic traps on overflow, including at runtime. Unary minus negates its operand; `&name` takes the address of a local (as a pointer argument) and `*p` dereferences a pointer parameter for reading or, as `*p = value;`, writing. `sizeof(<type>)` is a compile-time integer byte count (`int`/`long` are 4/8, `char`/`bool` are 1, a pointer type is 8); only a type operand is in the subset, since `sizeof <expr>` would need type inference. Compound assignments `+= -= *= /= %= &= |= ^= <<= >>=` and prefix/postfix `++`/`--` are supported as statements and as the `for` step; the logical operators `!`, `&&`, `||` are supported (the last two short-circuit) and `~` is bitwise complement; other unary operators are outside the subset. Unsigned integers are supported: `unsigned char`, `unsigned` / `unsigned int` and `unsigned long` (and the explicit `signed` forms) carry their signedness through declarations, parameters, returns, fields and pointers. An unsigned value zero-extends on use, and an unsigned operand makes arithmetic wrap instead of trapping and selects the unsigned division, remainder, right-shift and ordering opcodes, so `unsigned char` byte buffers and unsigned math behave as C requires. Storage-class and cv qualifiers (`const`, `static`, `extern`, `inline`, `register`, `volatile`) are accepted and ignored wherever a declaration begins. A scalar local declared without an initializer (`int x;`) is zero-initialized rather than left indeterminate, so it is always safe to read. No ARC allocation, classes, exceptions, templates, preprocessor, standard library, I/O or declaration-only functions are included. This is a small KArc frontend starting point, not C++20 conformance.

Pointers are first-class values. A pointer local (`T* p`) may be declared (uninitialized pointers and `= 0` are null), assigned, returned and passed; an array name decays to a pointer to its first element; `&x` takes a variable's address; `*p` and `p[i]` read or write through a pointer. Pointer arithmetic `p + i` / `p - i` scales by the pointee size, `p - q` yields an element count, the relational and equality operators compare addresses, and a pointer compares against `0` for a null check. String literals (`"..."`, with the same escapes as character literals) are `char*` values: each becomes a NUL-terminated file-scope `char` array and the literal yields its address, usable as an argument, initializer or return value. The lexer accepts ASCII identifiers and skips whitespace plus `//` and `/* */` comments. The token preprocessor expands object-like and function-like `#define` macros, `#undef`, `#if`/`#ifdef`/`#ifndef`/`#elif`/`#else`/`#endif`, resolves registered quoted and angle-bracket `#include` paths relative to the including source, and accepts `#pragma` and `#line`. Function-like macros support stringizing (`#parameter`) without expanding the argument first. Token pasting (`##`) joins unexpanded operands and rescans the resulting token, including empty arguments. Backslash-newline splicing runs before tokenization and preserves physical source offsets for diagnostics. Other preprocessing characters are discarded in excluded conditional branches and rejected if they survive in active source. Reserved underscore identifiers are accepted for platform headers. Comments cannot be nested. Decimal and hexadecimal (`0x`/`0X`) integer literals are accepted, as are character literals (`'a'`, and the escapes `\n \t \r \0 \\ \' \"`), which are integer character codes; octal, binary, numeric suffixes and digit separators are rejected.

Declarations inside `extern "C"` (one declaration or a braced group) are
named by their identifier alone, so they link against platform C symbols; every
other function keeps KArc's `name#<signature hash>` linkage. `short` and
`unsigned short` are 16-bit, and parameters may be pointers to pointers.
A parameter list may end in `...` (see `std.md` for the calling convention).
`new`/`delete` expressions allocate through the std. `sizeof` of a struct and
struct pointer arithmetic use the C layout (`layout.klfd`). An empty statement
`;` is accepted.

Classes (`parser_class.klfd`, `lower_class.klfd`): member functions defined in
the class body become `Owner::name` functions with a `this` parameter, as
out-of-class definitions already did. A constructor `Owner(...)` may have a
member initializer list; a destructor is `~Owner()`. `T c;`, `T c(args)`,
`T c{args}`, `T c = T(args)` and `new T(args)` zero the object and run the
constructor that takes that many arguments; a class without constructors is
initialized member by member from `{args}`. `delete p` runs the destructor
when `p` is not null. A class local is destroyed, innermost first, when its
block ends and on `return`, `break` and `continue`. A member call is resolved
by the static type of its receiver. Not yet supported: copy and move
constructors (a returned local is not destroyed, and assignment copies bytes),
constructors for `new T[n]` elements, inheritance, virtual functions and
operator overloading. The
builtins `__builtin_klf_alloc(size, align)` and `__builtin_klf_free(p)` lower to
KIR `heap.alloc.raw` and `heap.free.raw`; they are what the std's allocator is
written over. A system header (`#include <cstring>`) is read when the toolchain
provides it and skipped otherwise. `main` is only required when linking an
executable (`--emit bin`).

## C++ language support

Beyond the C subset above, the frontend compiles the C++ the std is written
in (`std.md`):

- **Classes.** Member functions, constructors with initializer lists,
  destructors, static data members, member typedefs and `using` aliases,
  conversion operators, and the implicit special members: a class without its
  own destructor, copy constructor or copy assignment destroys, copies and
  assigns member by member. Class locals are destroyed in reverse order at the
  end of their scope and on `return`, `break` and `continue`; temporaries at
  the end of their statement unless a declaration, assignment or return moves
  them. A class that needs copying is passed by value as a copy the caller
  makes and destroys.
- **Inheritance and virtual functions.** A base is a subobject at the front
  of the derived class; members are found through bases and pointers and
  references convert to a base. A polymorphic object points at its class's
  table of function pointers, which calls through a receiver or an unqualified
  name dispatch through; `delete` through a base pointer destroys the dynamic
  type.
- **Operators and references.** Member and free `operator@` for binary
  operators, `[]`, `()`, unary `*` and `->`, `++`/`--`, `=` and compound
  assignment; reference parameters, results and locals.
- **Overloading** by argument type, with converting constructors applied to
  arguments, initializations, assignments and returns.
- **Templates.** Class templates are instantiated while parsing (types through
  generated typedefs, value parameters, defaults, alias templates, nested
  `>>`); function templates are instantiated by the lowering from the call's
  argument types, or from explicit arguments, and member function templates
  as members of their class. A template's last parameter may be a pack: an
  instance expands `pattern...` once per element, `sizeof...` and the four
  fold forms; a pack pattern between `;`s declares one member per element,
  which is how `std::tuple` holds its elements. A parameter such as
  `const vector<T>&` deduces `T` from the class instance it receives. A
  generic lambda (`auto` parameters) has a member template operator(), and a
  function template may return `auto`. A braced list passed as an argument
  initializes the parameter: a `std::initializer_list`, a class, or a scalar. `T(args)` constructs a class
  or converts to a scalar type, also through a typedef or template parameter.
- **Lambdas.** A lambda is a class with one field per capture (by copy, by
  reference, or `[=]`/`[&]` for every enclosing local it uses) and an
  `operator()`; its result type is `-> T` or the type of its first `return`.
  Lambdas work with the std's algorithms, which take any callable.
- **Other forms.** `new`/`delete` (also placement `new` and `delete[]`),
  range-based `for` over arrays and classes with `begin`/`end`, `auto`, braced
  initialization and `std::initializer_list`, variadic functions,
  `throw`/`try`/`catch` (see docs/std.md), `T{args}` temporaries, `::name`, `get<N>(value)`, and clang's
  `__is_same`-style type-trait builtins.
- **C++17 and C++20 forms.** Structured bindings (`auto [a, b] = pair;`,
  `for (auto& [k, v] : map)`, over pairs, tuples, structs and arrays),
  default member initializers, `dynamic_cast` and `typeid` on polymorphic
  classes, user-defined literals (`100ms` is a call of `operator""ms`),
  concepts, requires-clauses and `auto` parameters (see docs/std.md),
  range-based `for` over a braced list, function-local statics
  (initialized when first reached) and `decltype` of literals, casts,
  class temporaries and variables (a lambda's is its closure class);
  `decltype` of other expressions is `auto`.

- **Lambdas capturing `this`** (`[this]`, `[=]`, `[&]`) reach the
  enclosing object's members and member functions by name and through
  `this`.
- **Templates.** A function template whose defaulted template argument
  or signature names a type that does not exist (`enable_if_t<false>`)
  is set aside, as SFINAE does; `if constexpr` with a constant condition
  keeps only the branch taken. Designated initializers
  (`{.height = 50}`) initialize the members they name.
- **Multiple inheritance.** A polymorphic base after the first keeps its
  own table pointer; the complete class gives it a table whose overridden
  entries are thunks moving `this` back to the complete object
  (`lower_thunk.klfd`). Each table records its subobject's offset, which
  `delete` and `dynamic_cast` use to reach the complete object; casts and
  comparisons between base and derived pointers adjust by the base's offset.
- **Coroutines.** A function whose body uses `co_await`, `co_yield` or
  `co_return` becomes a ramp under its own name, a resume function and a
  destroy function (`lower_coroutine.klfd`). The ramp allocates a frame,
  copies the parameters into it, constructs `R::promise_type` at byte 32
  and returns `get_return_object()`; every local of the resume function
  lives in the frame, and each suspension records its point in the frame's
  state and returns. `await_suspend` may return void, `bool` or a handle to
  resume next; a written `co_await` goes through the promise's
  `await_transform` and the awaitable's `operator co_await`. Lambdas may be
  coroutines. Locals still in scope when a suspended coroutine is
  destroyed are not destroyed.

Not supported yet: capturing `*this` by copy, virtual inheritance, SFINAE on expressions (only on types),
overloads told apart only by constraints, `constexpr` evaluation beyond
integer constants, `coroutine_traits` specializations (the promise is always
`R::promise_type`).

## Lowering behavior

Namespace scopes and `using` declarations are accepted around the supported
declarations. Qualified fixed-width integer names map to the matching KIR
integer types, and decimal or hexadecimal literals accept C++ digit separators
and integer suffixes. `double` arithmetic, comparisons and conversions lower to
KIR `f64` operations; `float` is `double`.

The frontend uses a flat index-based AST, keeping its types nonrecursive. Name and call checking run as part of lowering; any error discards all output. No placeholder implementation is substituted on error.

Mutable variables lower to entry-block stack allocations with trivial loads and stores. Parameters are stored into their slots. Loops lower to condition, body and exit blocks. Comparison results are i1; integer values are i32. All slots are deallocated in reverse order before the final return. Hoisting the storage for block locals to the entry block is safe for this subset: the values are trivial, initialization remains inside the loop, and their addresses cannot escape.

Every declaration is named from its span in the source, so a KIR symbol is the
C++ identifier itself: `main` is `@main`. A global the source has no name for,
such as the storage behind a string literal, gets a generated name from the
kernel. `--emit lib` keeps `main` local for translation-unit libraries;
`--export main` preserves it for the designated program entry archive.

A KArc program is a C process (`entry = .Process` in `language.klfd`): the
`main` the backend adds passes `argc`, `argv` and `envp` to the program's
`main`, whose value is the exit status. A runtime trap is reported on stderr
and aborts. The bytecode runner, `karc-run file.klbc [ARGUMENT...]`, runs a
program the same way.

## Validation

Lowering builds KIR through the kernel's builder services, so there is no
textual KIR to check: the module the kernel receives is the module karc built.
The framework drives the whole thing as a language package — it loads
`definitions/`, reads the `Language` declaration and runs the four phases — and
then verifies the module against the raw verifier and runs it. Those tests are
Rust and live in the framework repo, in `crates/klf-driver/tests/karc_*.rs`:
expressions, control flow, aggregates, pointers, whole programs, the examples,
and a native-backend set that runs the lowered programs through Cranelift.

## File modes

The subset described here is what the definition lowers today, and it lowers
the same way in either file mode: it has no classes, no `new` and no
destructors, so nothing in it yet distinguishes `.cpp` from `.krc`. What the
two modes will differ in is in `modes.md`.
