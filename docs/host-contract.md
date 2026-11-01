# Host contract

What karc needs from the framework, and what it no longer needs.

## Text builtins

KArc imports these helpers from the Kira `KLF` library:

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

`sourceText` obtains the unit's byte count and copies its UTF-8 text through
the KLF kernel services. The library calls those services through RIDL.

`sourceName` and `fileName` expose registered source names. `includeText` and
`includeExists` look up registered sources by name; KArc resolves an include
relative to the including source before making that lookup. The framework does
not read arbitrary host paths from definition code.

Arrays with `.count`, `.append(value)`, indexing and indexed assignment are
also required, with copy-on-write updating the original variable or field.

## Kernel services

Lowering builds KIR through the general low-level builder services
(`klf.kir.*`), addressed by opaque handles: module, signature, function,
block, value, address, global and type. `app/ir/kir.kira` is the main place that names them.
Nothing below the frontend knows a C++ type: karc keeps
its own type spellings and materializes each one into a `KirType` handle
through `klf.kir.type.integer`, `.address`, `.array`, `.pointer`, `.function`,
`.record` and `.unit`.

Raw heap storage goes through `klf.kir.value.heap_alloc_raw` and
`klf.kir.value.heap_free_raw`. The definition reads the requested output kind
from the kernel-owned `emit` flag (`session.flag("emit")`).

Diagnostics go through `klf.diag.error(code, start, end)`; the codes are in
`diagnostics.md`.

## Driver boundary

`app/Language.kira` declares `KArc` as a Kira `Language`. KLF reads its
metadata, builds the Kira package and calls its RIDL phase entry points. The
kernel drives the four phases without a separate harness or textual KIR.

A phase artifact carries a payload word the definition owns, so the parsed
tree travels: `parse` lexes and parses the unit and hands the tree out with
`artifactHandle`, `elaborate` reads it back with `artifactValue` and checks it,
and `lower` reads the same tree off the program artifact. Nothing is parsed
twice. `units` returns one unit per input file, so a program can span several
files and a call may cross units.

KCore builds Kira, and Kira builds KArc's native frontend through `klf build`.
The generated driver supplies the builder services and artifact frames, so
KArc can lower translation units and emit KIR libraries or executables. KArc
programs can run through either the KLF VM or the native backend. The former
KCore definition remains under `definitions/` for migration comparison.
