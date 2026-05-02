#!/usr/bin/env bash
#	tests/devices.sh
#
#	For every registered device tag, instantiate it via QSettings preset
#	(devices/devN/tag = <tag>), boot the emulator, save state, load state,
#	save again, and compare the two saved blobs. Any device that breaks
#	the state-roundtrip flips the script to non-zero exit.
#
#	The list of tags is derived from src/altirraqt/qt_register_devices.cpp
#	and matches what `ATRegisterDevicesQt` actually adds. We don't read it
#	at runtime to keep the test self-contained.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build/src/altirraqt/altirraqt"

if [[ ! -x "$BIN" ]]; then
    echo "devices: binary not built at $BIN" >&2
    exit 2
fi

# Smoke-test the no-device path first.
WORK="$(mktemp -d -t altirraqt-devices.XXXXXX)"
trap 'rm -rf "$WORK"' EXIT

failures=0
total=0

for tag in modem blackbox blackbox_floppy mio harddisk rtime8 covox xep80 \
           slightsid dragoncart 850 1030 sx212 sdrive sio2sd veronica rverter \
           soundboard pocketmodem kmkjzide kmkjzide2 myide myide2 side side2 \
           side3 dongle pbi diskdrive810 diskdrive1050 diskdrivexf551 \
           diskdriveindusgt diskdriveatr8000 vbxe rapidus warpos \
           hostdevice pclink browser; do
    total=$((total+1))
    out="$WORK/${tag}.rt"
    if timeout 15 "$BIN" --run-anyway --save-state-roundtrip "$out" \
            >"$WORK/$tag.log" 2>&1; then
        echo "devices: $tag: OK"
    else
        echo "devices: $tag: FAIL"
        failures=$((failures+1))
    fi
done

echo "devices: $total tested, $failures failed"
exit $((failures > 0 ? 1 : 0))
