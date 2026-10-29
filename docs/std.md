# The KArc std

`std/` is KArc's standard library, written in C++ and compiled by `karc`
itself (KLF-RIDL 28.9). Nothing in it is compiled by clang except
`karc_link.cpp`, the bootstrap listed in `native.list` (see the end).

## Layout

- `std/include/` — the headers programs include.
- `std/src/` — the out-of-line definitions of the C library half, plus
  `platform.h`, the POSIX calls the std is written over (`write`, `read`,
  `open`, `close`, `_exit`, `clock_gettime`).

Most of the C++ library is templates and inline members, so it lives in the
headers and is compiled into each program that includes it; a member function
defined in a class body, and every template instance, is unit-private, as a
C++ inline function is.

## How it is built and linked

`klf build ../karc --out <dir>` builds the `karc` driver, then runs it over
every source under `std/` except `std/include/`:

- `lib/karc/<target>/libstd.a` and `libstd.so`, native code;
- `lib/karc/bytecode/64/std.klbc`, bytecode;
- `include/karc/`, a copy of `std/include/`.

A driver installed in that sysroot finds `#include <vector>` in
`include/karc/`. `karc app.cpp -o app` links the program with `libstd.a`.
`karc app.cpp --emit klbc -o app.klbc` links `std.klbc` into the program's
bytecode, so `karc-run app.klbc` runs one image with the std inside it. Both
engines run the same std code, compiled from the same source, and every test
program in the framework's `karc_std` test prints the same output on both.

## Headers

| Area | Headers |
| --- | --- |
| C library | `<cstddef>`, `<cstdint>`, `<climits>`, `<cfloat>`, `<cstring>`, `<cctype>`, `<cstdlib>`, `<cstdio>`, `<cstdarg>`, `<cassert>`, `<cmath>`, `<cerrno>`, `<cinttypes>`, `<ctime>`, `<csignal>`, `<clocale>`, `<cstdbool>`, `<cfenv>`, `<cwchar>`, `<cwctype>`, `<cuchar>`, `<csetjmp>`, and every `<x.h>` spelling |
| Language support | `<new>`, `<initializer_list>`, `<exception>`, `<stdexcept>`, `<system_error>`, `<typeinfo>`, `<typeindex>`, `<type_traits>`, `<concepts>`, `<expected>`, `<stacktrace>`, `<limits>`, `<utility>`, `<tuple>`, `<functional>`, `<compare>`, `<variant>`, `<any>`, `<optional>`, `<source_location>`, `<version>`, `<coroutine>`, `<debugging>` |
| Strings | `<string>` (also `wstring`, `u16string`, `u32string`), `<string_view>`, `<charconv>`, `<format>` (with std::formatter specializations and ranges), `<print>`, `<regex>`, `<text_encoding>` |
| Containers | `<span>`, `<vector>`, `<array>`, `<list>`, `<forward_list>`, `<deque>`, `<stack>`, `<queue>` (with `priority_queue`), `<map>` and `<set>` (with the multi forms), `<unordered_map>`, `<unordered_set>` (with the multi forms), `<flat_map>`, `<flat_set>`, `<inplace_vector>`, `<bitset>`, `<valarray>`, `<generator>` |
| Algorithms and numbers | `<algorithm>`, `<execution>`, `<ranges>`, `<numeric>`, `<iterator>`, `<mdspan>`, `<random>`, `<chrono>`, `<ratio>`, `<bit>`, `<numbers>`, `<complex>` |
| Memory | `<memory>` (`unique_ptr` with deleters, `shared_ptr`, `weak_ptr`, `enable_shared_from_this`, the pointer casts, `allocator`), `<memory_resource>`, `<scoped_allocator>` |
| Input and output | `<iostream>`, `<istream>`, `<ostream>`, `<streambuf>`, `<iomanip>`, `<sstream>`, `<fstream>`, `<ios>`, `<iosfwd>`, `<filesystem>`, `<syncstream>`, `<spanstream>`, `<strstream>`, `<locale>` (the "C" locale), `<codecvt>` |
| Concurrency | `<thread>` (with `jthread`), `<stop_token>`, `<mutex>`, `<shared_mutex>`, `<condition_variable>`, `<future>`, `<semaphore>`, `<latch>`, `<barrier>`, `<atomic>` |

## How parts of it work

- **Allocation.** `malloc`, `calloc`, `realloc` and `free` route to the KLF
  runtime through `__builtin_klf_alloc(size, align)` and
  `__builtin_klf_free(p)`, which lower to KIR `heap.alloc.raw` and
  `heap.free.raw`; a 16-byte header keeps each block's size for `realloc`.
  `new` and `delete` call `karc_operator_new` and `karc_operator_delete` from
  `<new>`, which hand out zeroed storage.
- **Variadic functions.** A caller widens the arguments after the last named
  parameter to 64-bit words in a stack array and passes its address as a
  hidden last parameter; `va_start` reads it and `va_arg(list, T)` reads the
  next word as a `T`. It is ordinary KIR, so it behaves the same on the VM
  and natively. `printf` and its family support the flags `-0+`, width and
  precision (also `*`), the lengths `l ll z j t h`, and
  `d i u x X o c s p f F e E g G %`.
- **Floating point.** `float` is `double`: the KIR builder has only `f64`
  operations. `sqrt`, `fabs`, `floor`, `ceil` and `trunc` are builtins over
  KIR float operations; `exp`, `log`, `pow` and the trigonometric functions
  are computed from them.
- **Streams.** The standard streams and file streams read through a
  1 KiB buffer and write as they go; `fstream` reads and writes one file,
  with `seekg`/`tellg` and `seekp`/`tellp`.
- **Containers.** `vector`, `string` and `array` iterators are element
  pointers; the other containers have class iterators. `deque` is a ring
  buffer; `map`, `set` and their multi forms are AVL trees (`<__tree>`)
  ordered by a comparator parameter; the unordered containers are chained
  hash tables (`<__hash_table>`) over `std::hash` and `equal_to`, whose
  nodes also sit on one list that iterators walk. Node iterators stay valid
  until their element is erased. `queue`, `stack` and `priority_queue` take
  the container (and comparator) parameters. Every container but
  `string` takes an allocator parameter (`std::allocator` by default) and
  gets its storage from it; the node containers ask for their nodes as runs
  of the allocator's value type, so no rebinding is needed. The `std::pmr`
  containers use `polymorphic_allocator`.
- **Algorithms** in `<algorithm>` and `<numeric>` are templates over the
  iterator type, so they take either kind of iterator; `reverse_iterator`,
  `back_inserter` and `front_inserter` work with both. `sort` is an
  introsort, `stable_sort` and `inplace_merge` merge in place by rotations.
  `<execution>` adds the overloads taking a policy, which all run
  sequentially.
- **Streams.** `ostream` formats into a virtual `karc_put` and `istream`
  reads through a virtual `karc_get`; the base classes use a file
  descriptor, `ostringstream`/`istringstream` a string and
  `ofstream`/`ifstream` an opened file, so a function taking `ostream&`
  takes any of them. `stringstream` writes to a string and reads back what
  was written. Streams carry width, precision, fill and the `ios_base`
  format flags.
- **Move semantics.** A `T&&` parameter is preferred over `const T&` for a
  temporary or a `std::move`/`std::forward` result and passed over for an
  lvalue; `unique_ptr` is move-only and `vector::push_back` moves an rvalue.
  Functions declared `= delete` cannot be called.
- **Regular expressions** (`std/src/regex.cpp`) compile to a node tree that
  a backtracking matcher runs, keeping what is left to match as
  continuations on the stack.
- **Ranges.** Views are lazy iterators over a container (borrowed through
  `ref_view`) or another view (held by value), composed with `|`.
- **Wide characters.** `wchar_t` and `char32_t` are 32 bits and `char16_t` 16;
  `L"..."`, `u"..."` and `U"..."` literals are arrays of them, and the
  conversions to multibyte text use UTF-8.
- **Exceptions.** `throw` copies the value to the heap and hands it to the
  runtime in `std/src/exception.cpp` with the names it can be caught as (the
  class and its bases). The code then leaves for the nearest handler: after
  every call that may throw, a pending exception destroys the class locals
  in between and jumps to the enclosing `try`, whose handlers are tested in
  order, or returns from the function. An exception leaving `main`
  terminates the program. The std throws `out_of_range` from `at`,
  `invalid_argument` from `stoi` and its family, `bad_optional_access`,
  `bad_variant_access`, `bad_any_cast` and `bad_function_call`. `errno` is
  the std's own, since libc's is outside the memory a VM program can reach.
- **Files.** `<cstdio>` streams and `<filesystem>` are written over POSIX
  calls; `std/src/filesystem.cpp` reads directories with `getdents64` into
  the program's own memory, which a VM program needs, and uses the Linux
  AArch64 `stat` layout.
- **Globals.** A global with a constructor or a non-constant initializer
  is initialized by its unit's `__karc_global_init_N`, a KIR initializer that
  runs before `main`, in declaration order and after the std's. Global
  destructors do not run.
- **Run-time type information.** Every polymorphic class's table holds the
  chain of names it can be seen as; `dynamic_cast` looks the target up in
  it, and `typeid` of a polymorphic object reads the first name into a
  `std::type_info`.
- **Smart pointers.** A `shared_ptr`'s owners share a control block that
  counts the shared and weak owners and deletes the object through a
  virtual `dispose`, with the deleter it was given or `delete` of the
  pointer's own type; `weak_ptr::lock` and the pointer casts make new
  owners of the same block.
- **Random numbers.** The engines (`mt19937`, `mt19937_64`, the linear
  congruential, subtract-with-carry and adaptor engines) and the
  distributions are templates over any engine: a distribution reads the
  engine's output as a fraction of its range.
- **Time.** A `chrono::duration<Rep, Period>` counts ticks of `Period`
  seconds; conversions scale by the ratio of the periods, in integers when
  both counts are integral. The clocks count nanoseconds. The literal
  operators (`100ms`, `2h`, `"text"s`, `"text"sv`, `2i`) are declared at
  global scope, so they need no `using namespace`.
- **Concepts.** Before parsing, `definitions/parser_concept.klfd` turns a
  concept into a `bool` variable template, drops requires-clauses, makes a
  constrained parameter a plain `typename` and a function with `auto`
  parameters a template. A requires-expression counts as satisfied, and
  the concepts in `<concepts>` that need one hold for every type, so
  overloads told apart only by constraints are ambiguous.
- **Threads.** KArc programs run on one thread on both engines, so
  `std::thread` is a task in a cooperative scheduler (`std/src/thread.cpp`):
  its function runs to completion when something waits for it, which is
  `join`, a condition variable wait, `future::get`, a semaphore, latch or
  barrier, `yield` or `sleep_for`. Waiting runs the tasks queued before; a
  wait that no queued task can satisfy terminates, where threads would
  deadlock. `std::async` runs its function when called. A mutex records
  that it is held and `std::atomic` uses plain loads and stores.

## What is not there

- `std::function` calls take at most eight arguments.
- `std::generator` copies each yielded value into its promise and has no
  allocator or reference parameters. `jmp_buf` is a class, so `setjmp` and
  `longjmp` are macros over its words.
  `std::mdspan` has at most three dimensions.
- Type traits that would need to ask whether an expression compiles
  (`is_constructible`, `is_invocable` and the like) answer true;
  references and `const` have no type of their own, so `is_reference` and
  `is_const` answer false.
- `std::source_location::current()` does not know the caller's position.

## karc_link.cpp

`karc_link.cpp` predates this std: zero-returning stubs, compiled by clang,
that keep Harmony's unresolved library calls linkable. It should shrink as
Harmony moves onto the real std.
