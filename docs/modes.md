# File kinds

The extension selects the input mode. The C++ and ARC modes follow
KLF-RIDL section 28.1. C input support is being added for unchanged C
libraries such as SQLite.

| Extension | Mode | Memory |
| --- | --- | --- |
| `.c` | C input (in development) | Manual, platform calling convention |
| `.cpp`, `.cc`, `.cxx`, `.h`, `.hpp`, `.inl` | C++ | Manual, exactly as clang |
| `.krc`, `.krch` | ARC | Counted, mandatory |

## `.c` is C input

A `.c` translation unit uses C linkage and the platform calling convention.
Function-scoped labels and `goto` support forward, backward, and nested targets.
Switch dispatch supports case labels within nested blocks and loops.
`new` and `delete` are identifiers, and ordinary character constants have
type `int`. It shares Karc's lexer, preprocessor, parser and lowering code;
C conformance and the SQLite compilation target are still in development.

## `.cpp` is clang

A `.cpp` file is C++20 with clang's semantics and nothing added. `new` and
`delete` pair up because the programmer pairs them, `malloc` and `free` are
there, pointers are plain addresses, and the type traits report what clang
reports. Nothing is retained or released behind your back. The point of this
mode is that an existing tree — Unreal, libc++, anything — builds under `karc`
and means what it meant under clang.

## `.krc` is the dialect

A `.krc` file is the same C++20 with automatic reference counting made
mandatory and manual memory management removed. Every storage location of
object pointer type is a strong reference: initializing retains, assigning
retains the new value and releases the old, and going out of scope releases.
Destructors run at the last release, deterministically, on the thread that
released.

Because lifetimes are the compiler's, the constructs that could unbalance them
are gone. Section 28.5.1 of the spec is the full list; the short form:

- `delete` and `delete[]` do not exist (`KARC-E0101`).
- Placement `new` does not exist (`KARC-E0102`).
- You cannot cast a pointer to an integer or back (`KARC-E0103`).
- A `union` cannot hold a pointer (`KARC-E0104`).
- You cannot declare `operator new` or `operator delete` (`KARC-E0105`).
- `malloc`, `calloc`, `realloc` and `free` are not declared (`KARC-E0106`).
- `memcpy`, `memmove` and `memset` are rejected over any type that holds a
  strong pointer (`KARC-E0110`).

All of these are errors, never warnings. A `.krc` file that compiles has no
manual memory management in it, and that is the whole guarantee.

The two additions are the ownership qualifiers ARC needs, spelled as
declaration specifiers rather than attributes:

```
weak Node* prev;      // reads yield a strong temporary or nullptr
unowned Node* raw;    // not counted; you promise it outlives the use
```

Everything else — classes, templates, concepts, lambdas, overloading,
`constexpr`, the preprocessor — is the language `.cpp` gets.

## Mixing them

`.krc` and `.cpp` are one compiler, not two languages meeting at a boundary.
Same preprocessor, same parser, same overload set, same Itanium mangling, one
KIR module. They call each other with ordinary calls and inline into each
other. There is no wrapper, no thunk and no FFI between them.

One rule covers the whole crossing: **a storage location takes its policy from
the file that declares it.** Not the file that uses it, not the translation
unit it lands in.

| Declaration | Mode that decides |
| --- | --- |
| Local, parameter, temporary | the file with the function body |
| Data member | the file with the class definition |
| Global, static member | the file with the definition |
| Template instantiation | the file with the template definition |

Three consequences, and they are the whole story.

**A C++ type has exactly one KIR type.** One defining file, one mode, one
policy. `struct Node { Node* next; };` written in a `.krch` has a strong `next`
everywhere it is included — `.cpp` files too, which then retain when they copy
a `Node` and report it non-trivially-copyable. Written in a `.hpp`, `next` is
trivial everywhere, `.krc` files included. There is never a second layout or a
second mangling for one class, so sharing types across modes needs no
conversion at all. That is the normal case, not the special one.

**Locations a `.krc` file declares are strong even when the type isn't.** A
`.krc` function with a `Node*` local counts that local, whatever header `Node`
came from. Its `next` member is still trivial if `Node` came from a `.hpp`.
Each location has one declaring file; that file decides.

**Values in flight are borrowed.** A pointer being passed, returned or
evaluated is not a storage location and is never counted. Only storing it is,
which is why calling across modes costs nothing.

### Where you do have to write it differently

Four cases, and only these four:

- The callee keeps a pointer it was passed → its declaration needs
  `[[karc::retains(param)]]`, and it balances with `karc::release(p)`.
- A `.krc` function returns a pointer into `.cpp` code that will own it →
  return `karc::retained(p)` to hand over a +1.
- You want to pass the address of a strong location (`Node**`) to `.cpp` code →
  you cannot (`KARC-E0108`). A pointer to strong storage and a pointer to
  trivial storage are different types. Pass by value, or route through an
  `unowned` local, whose storage is trivial in both modes.
- The same entity defined in both a `.krc` and a `.cpp` file → `KARC-E0109`.
  One mangled name is lowered once, so it has one mode.

`memcpy` over a type holding strong pointers is `KARC-E0110` in both modes, for
the same reason the policy is the same in both: it belongs to the type.

Section 28.7 of the spec is the normative version.

To move a `.cpp` tree over file by file without renaming anything, use
`#pragma karc arc` at the top of a file or `-fkarc-arc=<glob>` on the command
line. A file that enters ARC mode that way is bound by every rule above.

## What the definition implements today

The definition in `definitions/` compiles the subset in `frontend.md`, which
has no classes, no `new` and no destructors, so the two modes do not yet
differ in what they lower. What exists now is the declaration: the extension
list, the package default of manual memory, and the `arc` file mode keyed to
`.krc` and `.krch`. The rules above are the contract the frontend grows into.
