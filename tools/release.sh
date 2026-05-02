#!/usr/bin/env bash
#	tools/release.sh
#
#	Build all release artefacts for altirraqt:
#	  * AppImage (linuxdeploy-driven, self-contained)
#	  * Flatpak  (org.kde.Platform//6.7 runtime)
#	The artefacts land in dist/.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLS="${ROOT}/tools"

step() { printf '\n\033[1;36m== %s ==\033[0m\n' "$*"; }

step "AppImage"
"${TOOLS}/build-appimage.sh"

if command -v flatpak-builder >/dev/null 2>&1; then
    step "Flatpak"
    "${TOOLS}/build-flatpak.sh"
else
    echo "(skipping Flatpak: flatpak-builder not installed — run packaging/flatpak setup first)"
fi

step "Done"
ls -lh "${ROOT}/dist"
