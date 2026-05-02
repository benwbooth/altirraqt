#!/usr/bin/env bash
#	tools/build-appimage.sh
#
#	Build a self-contained AppImage of altirraqt.
#
#	Workflow:
#	  1. cmake configure + build (release).
#	  2. cmake --install into a staging AppDir.
#	  3. linuxdeploy + linuxdeploy-plugin-qt to bundle Qt6 libs and the
#	     platform/imageformats/multimedia plugins.
#	  4. appimagetool packages AppDir into altirraqt-<version>-x86_64.AppImage
#	     under dist/.
#
#	Requires: cmake, ninja, linuxdeploy, linuxdeploy-plugin-qt,
#	          appimagetool. The script downloads them on demand into
#	          tools/cache/ if they aren't on $PATH.
#
#	On Nix systems it automatically wraps the cmake invocation in
#	`nix develop --command` so the dev shell's Qt is used.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build-appimage"
APPDIR="${ROOT}/build-appimage/AppDir"
DIST="${ROOT}/dist"
CACHE="${ROOT}/tools/cache"
mkdir -p "$CACHE" "$DIST"

ARCH="$(uname -m)"
VERSION="$(git -C "$ROOT" describe --tags --always 2>/dev/null || echo dev)"

CMAKE_PREFIX="${APPDIR}/usr"

CMAKE_RUNNER=()
if command -v nix >/dev/null 2>&1 && [[ -f "${ROOT}/flake.nix" ]]; then
    CMAKE_RUNNER=(nix develop --command)
fi

fetch() {
    local url="$1" out="$2"
    if [[ ! -x "$out" ]]; then
        echo "fetch: $url"
        curl -fL -o "$out" "$url"
        chmod +x "$out"
    fi
}

# Resolve linuxdeploy + qt plugin + appimagetool.
LINUXDEPLOY="$(command -v linuxdeploy || true)"
LINUXDEPLOY_QT="$(command -v linuxdeploy-plugin-qt || true)"
APPIMAGETOOL="$(command -v appimagetool || true)"

if [[ -z "$LINUXDEPLOY" ]]; then
    LINUXDEPLOY="$CACHE/linuxdeploy-${ARCH}.AppImage"
    fetch "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage" "$LINUXDEPLOY"
fi
if [[ -z "$LINUXDEPLOY_QT" ]]; then
    LINUXDEPLOY_QT="$CACHE/linuxdeploy-plugin-qt-${ARCH}.AppImage"
    fetch "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage" "$LINUXDEPLOY_QT"
fi
if [[ -z "$APPIMAGETOOL" ]]; then
    APPIMAGETOOL="$CACHE/appimagetool-${ARCH}.AppImage"
    fetch "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-${ARCH}.AppImage" "$APPIMAGETOOL"
fi

echo "=== configure + build ==="
"${CMAKE_RUNNER[@]}" cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
"${CMAKE_RUNNER[@]}" cmake --build "$BUILD" -j"$(nproc)"

echo "=== install staging ==="
rm -rf "$APPDIR"
"${CMAKE_RUNNER[@]}" cmake --install "$BUILD" --prefix "$CMAKE_PREFIX"

# linuxdeploy expects the .desktop to live at AppDir/usr/share/applications,
# which our install rules already do, plus an icon at the AppDir root or a
# matching path under hicolor. Both are present.

echo "=== bundle Qt + appimagetool ==="
export ARCH
export VERSION
export OUTPUT="${DIST}/altirraqt-${VERSION}-${ARCH}.AppImage"
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --plugin qt \
    --output appimage \
    --desktop-file "$APPDIR/usr/share/applications/altirraqt.desktop" \
    --icon-file   "$APPDIR/usr/share/icons/hicolor/256x256/apps/altirraqt.png"

echo
echo "AppImage written to: $OUTPUT"
ls -lh "$OUTPUT"
