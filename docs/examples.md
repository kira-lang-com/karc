# KArc examples

One directory per program, each holding its KArc (C++) source as `app.cpp`.

- `fibonacci` — iterative Fibonacci: a function with a parameter, mutable
  locals and a `while` loop; `main` returns `fibonacci(10)` = 55.
- `strings` — pointer-style `strlen`/`strcpy`/`strcmp`/`strchr`.
- `bytebuf` — a struct embedding a fixed buffer, summed through its array field.
- `libc` — string, memory and ctype routines over byte pointers and `void*`.
- `calc` — an expression evaluator over a character buffer.
- `crc32` — a table-driven checksum.
- `bytecode-vm` — a small interpreter loop over an instruction array.

The framework test suite compiles each one through the kernel and runs it, so
the expected result of every example is recorded there rather than as a
checked-in artifact. Programs stay within the subset documented in
`frontend.md`.
