# SQLite compilation target

Run `bash validation/sqlite/build.sh /path/to/sysroot/bin/karc` with a Karc
toolchain built by `klf build . --out /path/to/sysroot`. The script downloads
the pinned SQLite 3.53.4 amalgamation, checks its SHA-256, and compiles the
unchanged `sqlite3.c` through a C include wrapper. It builds a native static
library, then a native executable that creates an in-memory database, inserts
two rows, and verifies their sum is 42. An exit status of zero proves both
builds and the SQL smoke test passed. No SQLite features are disabled.

This is an unfinished compiler target. SQLite 3.53.4 relies on C-only syntax
and semantics; an unchanged amalgamation fails a reference C++ build. Karc
uses its C input mode for this validation. Bundled Linux GNU POSIX headers are used for the default build. A separate
development probe with
`SQLITE_THREADSAFE=0` and `SQLITE_OS_OTHER=1` is used to uncover language
failures independently of the POSIX headers; that probe does not satisfy this
validation target.
