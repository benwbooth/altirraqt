#!/usr/bin/env bash
#	tests/soak.sh
#
#	Boots the emulator with the bundled AltirraOS ROMs (no image), runs
#	for $FRAMES frames, hashes the framebuffer, and compares the digest
#	against tests/baseline-soak.txt. Non-zero exit on mismatch.
#
#	Update the baseline with:
#	  build/src/altirraqt/altirraqt --soak 60:tests/baseline-soak.txt

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build/src/altirraqt/altirraqt"
BASELINE="${ROOT}/tests/baseline-soak.txt"
FRAMES="${FRAMES:-60}"
ACTUAL="$(mktemp -t altirraqt-soak.XXXXXX)"
trap 'rm -f "$ACTUAL"' EXIT

if [[ ! -x "$BIN" ]]; then
    echo "soak: binary not built at $BIN" >&2
    exit 2
fi
if [[ ! -f "$BASELINE" ]]; then
    echo "soak: missing baseline $BASELINE" >&2
    echo "  Generate: $BIN --soak ${FRAMES}:${BASELINE}" >&2
    exit 2
fi

timeout 30 "$BIN" --soak "${FRAMES}:${ACTUAL}" >/dev/null 2>&1
expected="$(cat "$BASELINE" | tr -d '[:space:]')"
actual="$(cat "$ACTUAL"   | tr -d '[:space:]')"

if [[ "$expected" == "$actual" ]]; then
    echo "soak: OK ($FRAMES frames, hash $actual)"
    exit 0
fi

echo "soak: MISMATCH" >&2
echo "  expected: $expected" >&2
echo "  actual:   $actual" >&2
exit 1
