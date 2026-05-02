#!/usr/bin/env bash
#	tests/all.sh — run the full validation suite.

set -uo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fail=0

run() {
    local name="$1"
    local script="$2"
    echo "=== $name ==="
    if ! "$script"; then
        echo "FAIL: $name"
        fail=$((fail+1))
    fi
}

run "soak (deterministic framebuffer hash)" "${ROOT}/tests/soak.sh"
run "audio (smoke / WAV header)"            "${ROOT}/tests/audio.sh"
run "devices (save-state round-trip)"       "${ROOT}/tests/devices.sh"

if (( fail == 0 )); then
    echo "ALL OK"
    exit 0
fi
echo "$fail test(s) failed"
exit 1
