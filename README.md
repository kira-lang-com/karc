# KArc

KArc is the C++ hosted language of the Kira Language Framework (KLF spec
section 28: `name = "karc"`, `driver = "karc"`, `memory = .Unmanaged`,
`convention = .Ridl`, one unit per source file, file mode `arc`). It is adding
`.c` input support for unchanged C libraries such as SQLite. Its established
modes are `.cpp`, clang's C++ with manual memory management, and
`.krc` is the dialect where reference counting is mandatory and manual memory
management is removed from the language — see `docs/modes.md`. It is a peer
of Kira, C and Swift — a language whose compiler KLF *produces from a
frontend* written in Kira using KLF. KArc is not a Rust program and this repo is not
a Rust project; the Rust code is the framework (`../kira-language-framework`),
which KArc targets.

This repository contains KArc's definition and standard library. C goes
through this frontend as well: `.c` files are KArc's C input mode. The old
`../klf-c` package is retired.

## What's here

- `app/` — the Kira implementation of KArc, using the KLF library and RIDL.
- `definitions/` — the original KCore definition, retained during migration:
  lexer, parser,
  KIR builder, lowering, diagnostics, and the `construct KArc() extends
  Language<Tree, Program>` declaration the kernel loads.
- `std/` — KArc's std, C++ compiled by `karc` itself: headers in
  `std/include/`, definitions in `std/src/`. See `docs/std.md`.
- `examples/<name>/app.cpp` — one KArc program per directory.
- `docs/` — `modes.md` (`.cpp` versus `.krc`), `frontend.md` (supported subset
  and lowering behaviour), `std.md` (the standard library), `host-contract.md`
  (what karc needs from the framework), `diagnostics.md` (the diagnostic
  codes) and `examples.md`.

## How it runs

The KLF driver builds the Kira frontend from `app/`, reads the `Language` declaration and runs
`units`, `parse`, `elaborate` and `lower`. `lower` builds a `KirModule` through
the kernel's structural builder services — there is no textual KIR, no harness
entry point and no JavaScript anywhere in the pipeline. The same package also
compiles to a native `karc` binary through Cranelift, and that binary is the
compiler; the VM is the bootstrap and the parity oracle.

`klf build ../karc --out <dir>` is intended to produce the whole toolchain:
the `karc` driver, the std compiled by it to native libraries and bytecode,
and its headers. The old Harmony compatibility stubs are retained under
`validation/legacy-bootstrap/` for reference and are not included in the
toolchain; programs that relied on them need real implementations. See
`docs/std.md` for the toolchain layout.

## Validation

The Kira frontend's tests live in `tests/*.kira`. Run them with
`klf test . --unit` from this directory. KLF builds the frontend under test and
provides it and the VM runner to the Kira test process.

`klf test .` also builds the toolchain and runs `corpus/programs.kcore`,
including native and VM programs. The tests in `tests/native.kira` compile
and run native executables; the other execution tests use KLF's VM. The
original definition in `definitions/` remains available for comparison while
the migration is verified.

The Kira frontend also provides `karc format` for C, C++, KRC and header
files. It uses the KLF shared layout and keeps preprocessor directives intact.

The unfinished SQLite target has a pinned build and SQL smoke test in
[validation/sqlite](validation/sqlite/README.md).

This covers a small C++ subset — see `docs/frontend.md` — not C++20
conformance.

## Provenance

The first frontend was authored by Astra (GPT-6) under `frontend/`, checked
with a JavaScript translation harness because that workspace had no `klf`
executable. The harness is gone and the sources have since moved to
`definitions/`: the Kira ecosystem does not depend on Node or JavaScript, and
validation runs on the real kernel.
