#!/bin/sh
set -e

echo "=== Building & Running Golden Vectors Test Suite ==="
CC="${CC:-cc}"
CFLAGS="${CFLAGS:--std=c99 -Wall -Wextra -O2}"
$CC $CFLAGS -o tests/golden_vectors tests/golden_vectors.c src/genome.c src/decoder.c src/state.c src/mutation.c src/scheduler.c src/diag.c
./tests/golden_vectors
rm -f tests/golden_vectors
