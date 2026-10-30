#!/bin/sh
# Integration test: run every guest image under visor and compare its
# console output and exit status with the expected file beside it.
# Requires /dev/kvm; skips cleanly where hardware virtualization is absent.
set -e
cd "$(dirname "$0")/.."
VISOR=${VISOR:-build/visor}

if [ ! -e /dev/kvm ]; then
    echo "SKIP: /dev/kvm not available on this host"
    exit 0
fi

fail=0
for img in build/guests/*.img; do
    name=$(basename "$img" .img)
    want="guests/$name.expected"
    [ -f "$want" ] || continue
    got=$("$VISOR" run "$img" | tr -d '\r')
    if [ "$got" != "$(cat "$want")" ]; then
        echo "FAIL $name:" >&2
        echo "  want: $(cat "$want")" >&2
        echo "  got:  $got" >&2
        fail=1
    else
        echo "PASS $name"
    fi
done
[ $fail -eq 0 ] && echo "GUEST TESTS PASSED" || exit 1
