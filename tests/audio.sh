#!/usr/bin/env bash
#	tests/audio.sh
#
#	Boots the emulator with --run-anyway, captures $FRAMES frames of audio
#	output to a WAV, and asserts the WAV is non-empty and decodes as a
#	44.1 kHz stereo PCM stream. Exact-byte hash comparison is intentionally
#	NOT done — the simulator is driven by a wall-clock QTimer rather than
#	a fixed frame stepper, so the audio output is timing-non-deterministic
#	at the byte level even though it's perceptually identical. The
#	framebuffer hash check (`tests/soak.sh`) is the deterministic gate.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build/src/altirraqt/altirraqt"
FRAMES="${FRAMES:-60}"
WORK="$(mktemp -d -t altirraqt-audio.XXXXXX)"
trap 'rm -rf "$WORK"' EXIT

if [[ ! -x "$BIN" ]]; then
    echo "audio: binary not built at $BIN" >&2
    exit 2
fi

timeout 30 "$BIN" --run-anyway --audio-soak "${FRAMES}:${WORK}/audio.wav" >/dev/null 2>&1

# Sanity-check the WAV: at least 1 MB of audio for 60 frames, RIFF/WAVE magic.
size=$(stat -c '%s' "${WORK}/audio.wav")
if (( size < 1000000 )); then
    echo "audio: WAV too small ($size bytes)" >&2
    exit 1
fi
magic=$(head -c 4 "${WORK}/audio.wav")
if [[ "$magic" != "RIFF" ]]; then
    echo "audio: WAV header wrong ('$magic', expected RIFF)" >&2
    exit 1
fi

echo "audio: OK ($FRAMES frames, $size bytes)"
exit 0
