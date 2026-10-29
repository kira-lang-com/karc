# Host contract

What karc needs from the framework, and what it no longer needs.

## Text builtins

The definition language has no string library, so `klf-kira-lower` implements
these inline:

```kira
function utf8Count(text: String) -> Int
function utf8Byte(text: String, offset: Int) -> Int
function utf8Slice(text: String, start: Int, end: Int) -> String
function joinText(parts: [String]) -> String
function decimalText(value: Int) -> String
function sourceText() -> String
function sourceName() -> String
function fileName(session: Session, file: SourceFile) -> String
function includeText(path: String) -> String
function includeExists(path: String) -> Bool
```

`sourceText` assembles the text of the unit being compiled from
`klf.source.len` and `klf.source.byte`; the service ABI is i64-only, so the
text cannot cross it in one piece.

`sourceName` and `fileName` expose registered source names. `includeText` and
`includeExists` look up registered sources by name; KArc resolves an include
relative to the including source before making that lookup. The framework does
not read arbitrary host paths from definition code.

Arrays with `.count`, `.append(value)`, indexing and indexed assignment are
also required, with copy-on-write updating the original variable or field.

## Kernel services

Lowering builds KIR through the general low-level builder services
(`klf.kir.*`), addressed by opaque handles: module, signature, function,
block, value, address, global and type. `definitions/kir.klfd` is the only
place that names them. Nothing below the frontend knows a C++ type: karc keeps
its own type spellings and materializes each one into a `KirType` handle
through `klf.kir.type.integer`, `.address`, `.array`, `.pointer`, `.function`,
`.record` and `.unit`.

Raw heap storage goes through `klf.kir.value.heap_alloc_raw` and
`klf.kir.value.heap_free_raw`. The definition reads the requested output kind
from the kernel-owned `emit` flag (`session.flag("emit")`).

Diagnostics go through `klf.diag.error(code, start, end)`; the codes are in
`diagnostics.md`.

## Driver boundary

`definitions/language.klfd` is a `construct KArc() extends Language<Tree,
Program>` package. The kernel loads the directory, reads the metadata and
drives the four phases, so there is no textual KIR and no separate harness
entry point.

A phase artifact carries a payload word the definition owns, so the parsed
tree travels: `parse` lexes and parses the unit and hands the tree out with
`artifactHandle`, `elaborate` reads it back with `artifactValue` and checks it,
and `lower` reads the same tree off the program artifact. Nothing is parsed
twice. `units` returns one unit per input file, so a program can span several
files and a call may cross units.

The definition is available through both the VM bootstrap path and the native
frontend produced by `klf build`. The native runtime supplies the builder
services and artifact heap in the generated driver, so the native frontend can
lower translation units and emit KIR libraries or executables.
