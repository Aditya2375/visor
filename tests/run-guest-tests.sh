#!/bin/sh
# Integration test: run every guest image under visor and compare its
# console output, exit status, and the host-side exit report against the
# expected file beside it. Requires /dev/kvm; skips cleanly where hardware
# virtualization is absent.
set -e
cd "$(dirname "$0")/.."
VISOR=${VISOR:-build/visor}

if [ ! -e /dev/kvm ]; then
    echo "SKIP: /dev/kvm not available on this host"
    exit 0
fi

fail=0
err=$(mktemp)
trap 'rm -f "$err"' EXIT
for img in build/guests/*.img; do
    name=$(basename "$img" .img)
    if [ "$name" = spin ]; then
        # watchdog guest: must be killed by the timer, with the diagnostic
        s_rc=0
        "$VISOR" run "$img" >/dev/null 2>"$err" || s_rc=$?
        if [ "$s_rc" -eq 0 ] || ! grep -q "watchdog expired" "$err"; then
            echo "FAIL spin: watchdog did not stop the guest (rc=$s_rc)" >&2
            fail=1
        else
            echo "PASS spin (watchdog)"
        fi
        continue
    fi
    want="guests/$name.expected"
    [ -f "$want" ] || continue
    rc=0
    out=$("$VISOR" run "$img" 2>"$err") || rc=$?
    got=$(printf '%s' "$out" | tr -d '\r')
    if [ "$got" != "$(cat "$want")" ]; then
        echo "FAIL $name output:" >&2
        echo "  want: $(cat "$want")" >&2
        echo "  got:  $got" >&2
        fail=1
    elif [ "$rc" -ne 0 ]; then
        echo "FAIL $name: exit status $rc" >&2
        fail=1
    elif ! grep -q "guest exit: 0" "$err"; then
        echo "FAIL $name: missing host-side 'guest exit: 0' report" >&2
        fail=1
    else
        echo "PASS $name"
    fi
done
[ $fail -eq 0 ] && echo "GUEST TESTS PASSED" || exit 1
