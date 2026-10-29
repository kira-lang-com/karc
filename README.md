# KArc

KArc is the C++ hosted language of the Kira Language Framework (KLF spec
section 28: `name = "karc"`, `driver = "karc"`, `memory = .Unmanaged`,
`convention = .Ridl`, one unit per source file, file mode `arc`). It is adding
`.c` input support for unchanged C libraries such as SQLite. Its established
modes are `.cpp`, clang's C++ with manual memory management, and
`.krc` is the dialect where reference counting is mandatory and manual memory
management is removed from the language — see `docs/modes.md`. It is a peer
of Kira, C and Swift — a language whose compiler KLF *produces from a
definition* written in KLF Kira. KArc is not a Rust program and this repo is not
a Rust project; the Rust code is the framework (`../kira-language-framework`),
which KArc targets.

This repository contains KArc's definition and standard library. C goes
through this frontend as well: `.c` files are KArc's C input mode. The old
`../klf-c` package is retired.

## What's here

- `definitions/` — the KArc language definition in KLF Kira: lexer, parser,
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

The kernel loads `definitions/`, reads the `Language` declaration and runs
`units`, `parse`, `elaborate` and `lower`. `lower` builds a `KirModule` through
the kernel's structural builder services — there is no textual KIR, no harness
entry point and no JavaScript anywhere in the pipeline. The same package also
compiles to a native `karc` binary through Cranelift, and that binary is the
compiler; the VM is the bootstrap and the parity oracle.

`klf build ../karc --out <dir>` goes one step further and produces the whole
toolchain: the `karc` driver, the std compiled by it to native libraries and
bytecode, and its headers. Programs built with that driver link the std on
either engine (`docs/std.md`).

## Validation

Validation runs in the framework repo, where the Rust tests live next to the
code they exercise (`crates/klf-driver/tests/karc_*.rs`): each C++ program is
compiled through the executor, checked against the raw KIR verifier and run, on
the VM and, through the Cranelift-built frontend, natively — where the KIR must
come out byte-identical to the VM's on every example.

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
