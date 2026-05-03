# altirraqt tests

These scripts exercise the headless CLI options and gate determinism
regressions.

## `soak.sh`

Boots the emulator with the bundled AltirraOS ROMs (no image) and asserts
that the framebuffer hash after `${FRAMES:-60}` rendered frames matches
`baseline-soak.txt`. Catches CPU-/ANTIC-/GTIA-side regressions that would
shift pixels.

Re-baseline after intentional rendering changes:

    build/src/altirraqt/altirraqt --soak 60:tests/baseline-soak.txt

## `devices.sh`

Walks every device class registered via `ATRegisterDevicesQt` and runs a
state-save → load → save round-trip with `--save-state-roundtrip`. Any
class whose state isn't byte-stable after a load fails the script.

## `audio.sh`

`--audio-soak frames:path` writes a WAV alongside its SHA-256. The
shell test asserts the WAV is non-empty and starts with the `RIFF`
magic; it does **not** byte-compare against a baseline because the
simulator is QTimer-driven (wall-clock paced) rather than fixed-frame
stepped, so the audio stream is perceptually identical run-to-run but
timing-non-deterministic at the byte level. Use `tests/soak.sh` for
the deterministic framebuffer regression gate.

## Running everything

    cmake --build build -j8
    tests/all.sh

`all.sh` runs `soak.sh`, `audio.sh`, and `devices.sh` in sequence and
exits non-zero if any of them fails.
