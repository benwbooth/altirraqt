#!/usr/bin/env bash
#	tools/build-roms.sh — pre-build the AltirraOS kernel + Altirra BASIC
#	ROMs on the host and leave them under build/src/Kernel and
#	build/src/ATBasic. Used by tools/build-flatpak.sh because the
#	Flatpak SDK has no Free Pascal compiler to build MADS.
#
#	On Nix this runs inside `nix develop` so MADS is on PATH.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"

CMAKE_RUNNER=()
if command -v nix >/dev/null 2>&1 && [[ -f "${ROOT}/flake.nix" ]]; then
    CMAKE_RUNNER=(nix develop --command)
fi

"${CMAKE_RUNNER[@]}" cmake -S "$ROOT" -B "$BUILD"
"${CMAKE_RUNNER[@]}" cmake --build "$BUILD" --target altirra_roms altirra_basic_rom
