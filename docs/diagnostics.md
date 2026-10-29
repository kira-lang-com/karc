# Diagnostic codes

The kernel's diagnostic channel (`klf.diag.error`) carries a number and a byte
range, not text. karc reports the code and the span it applies to, and declares
the text of every code in the `diagnostics` member of its `Language`
declaration, so the kernel renders it with the position it points at. The
numbers are named by the functions in `definitions/diagnostics.klfd`.

Most codes are reported from the phase that runs on one translation unit.
Code 1033 is program-wide because only a linked executable (`--emit bin`) requires `main`; it is checked over every input in `units`.

| Code | Meaning |
| --- | --- |
| 1001 | the lexer rejected the source at this byte |
| 1002 | the parser rejected this token |
| 1003 | a local with this name is already visible |
| 1010 | this pointer operation is outside the subset |
| 1018 | `main` cannot be called |
| 1021 | this name is not declared |
| 1022 | this expression form is outside the subset |
| 1027 | `break` outside a loop or switch |
| 1028 | `continue` outside a loop |
| 1029 | a non-void function must end in `return` |
| 1030 | this function is defined twice |
| 1031 | `main` takes no parameters |
| 1033 | the program has no `main` |
| 1034 | this global is declared twice |
| 1035 | a `__builtin_klf_*` call has the wrong number of arguments |
| 1036 | a function template instance does not parse |
| 1037 | this call chooses a function declared `= delete` |
| 1038 | a `goto` names a label not defined in its function |
| 1039 | a label is defined twice in one function |
| 1040 | a `case` or `default` label has no enclosing switch |

A phase that reports a code fails the compilation, and the kernel renders the
diagnostic as `file:line:column: error[code]: title: explanation`.
