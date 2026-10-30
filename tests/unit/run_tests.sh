#!/bin/sh
# Host unit tests for visor's pure logic: image loading bounds, exit
# dispatch (ports/MMIO), and argument/file utilities.
set -e
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/tests}
mkdir -p "$OUT"
CFLAGS="-std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc"
cc $CFLAGS tests/unit/test_load.c src/vm.c src/exit.c -o "$OUT/test_load"
cc $CFLAGS tests/unit/test_exit.c src/exit.c -o "$OUT/test_exit"
cc $CFLAGS tests/unit/test_util.c src/util.c -o "$OUT/test_util"
"$OUT/test_load"
"$OUT/test_exit"
"$OUT/test_util"
echo "UNIT TESTS PASSED"
