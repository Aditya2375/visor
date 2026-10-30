#!/bin/sh
# Fail when a numeric claim in the docs drifts from the real test output.
set -e
cd "$(dirname "$0")/.."
sum=$(sh tests/unit/run_tests.sh 2>/dev/null | grep -o '^[0-9]* checks passed' | grep -o '^[0-9]*' | awk '{s+=$1} END {print s}')
for doc in README.md docs/ARCHITECTURE.md; do
    for claimed in $(grep -o '[0-9]* \(unit \)\?checks' "$doc" 2>/dev/null | grep -o '^[0-9]*' | sort -u); do
        if [ "$claimed" != "$sum" ]; then
            echo "DOC DRIFT: $doc claims $claimed checks, unit suite runs $sum" >&2
            exit 1
        fi
    done
done
echo "docs match: $sum checks"
