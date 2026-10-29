#!/usr/bin/env bash
set -eu

compiler=${1:?usage: build.sh /path/to/karc [work-directory]}
workdir=${2:-/tmp/karc-sqlite-validation}
scriptdir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$workdir"
workdir=$(CDPATH= cd -- "$workdir" && pwd)
archive="$workdir/sqlite-amalgamation-3530400.zip"

if [ ! -f "$archive" ]; then
    curl -fL https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip -o "$archive"
fi
printf '1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d  %s\n' "$archive" | sha256sum -c -
unzip -j -o "$archive" sqlite-amalgamation-3530400/sqlite3.c sqlite-amalgamation-3530400/sqlite3.h -d "$workdir"

printf '#include "sqlite3.c"\n' > "$workdir/sqlite.c"
cp "$scriptdir/smoke.c" "$workdir/smoke.c"
rm -f "$workdir/libsqlite3.a" "$workdir/sqlite-smoke"
"$compiler" --emit lib "$workdir/sqlite.c" -o "$workdir/libsqlite3.a"
test -s "$workdir/libsqlite3.a"
"$compiler" --emit bin "$workdir/smoke.c" -o "$workdir/sqlite-smoke"
test -s "$workdir/sqlite-smoke"
"$workdir/sqlite-smoke"
