#!/bin/sh
# Host unit tests for visor's pure logic: guest-image loading bounds.
set -e
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/tests}
mkdir -p "$OUT"
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc \
    tests/unit/test_load.c src/vm.c src/exit.c -o "$OUT/test_load"
"$OUT/test_load"
echo "UNIT TESTS PASSED"
