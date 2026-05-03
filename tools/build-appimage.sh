#!/usr/bin/env bash
#	tools/build-appimage.sh
#
#	Build a self-contained AppImage of altirraqt.
#
#	Strategy A — linuxdeploy + linuxdeploy-plugin-qt + appimagetool.
#	When linuxdeploy is downloadable and the host has a system Qt6
#	(qmake6 in PATH), we use the conventional walk-the-needed-libs
#	approach: typical output ~100 MB.
#
#	Strategy B — `nix bundle --bundler github:ralismark/nix-appimage`.
#	Used when no linuxdeploy/system-Qt6 is available (NixOS host
#	without /usr/lib/qt6, offline, etc.). Bundles the full Nix closure
#	~450 MB but works on any glibc Linux.
#
#	Output: dist/altirraqt-<git-describe>-<arch>.AppImage

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${ROOT}/dist"
CACHE="${ROOT}/tools/cache"
BUILD="${ROOT}/build"
mkdir -p "$DIST" "$CACHE"

ARCH="$(uname -m)"
VERSION="$(git -C "$ROOT" describe --tags --always 2>/dev/null || echo dev)"

use_linuxdeploy() {
    command -v qmake6 >/dev/null 2>&1 || command -v qmake >/dev/null 2>&1
}

if use_linuxdeploy; then
    # ---- Strategy A ----------------------------------------------------
    LD_AI="${CACHE}/linuxdeploy-${ARCH}.AppImage"
    LD_QT="${CACHE}/linuxdeploy-plugin-qt-${ARCH}.AppImage"

    if [[ ! -x "$LD_AI" ]]; then
        echo "=== fetching linuxdeploy ==="
        wget -q -O "$LD_AI" \
          "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
        chmod +x "$LD_AI"
    fi
    if [[ ! -x "$LD_QT" ]]; then
        echo "=== fetching linuxdeploy-plugin-qt ==="
        wget -q -O "$LD_QT" \
          "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage"
        chmod +x "$LD_QT"
    fi

    # 1) Build against system Qt6.
    echo "=== cmake build (system Qt6) ==="
    cmake_opts=(-S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release)
    if [[ -n "${PREBUILT_KERNEL_ROMS_DIR:-}" ]]; then
        cmake_opts+=("-DPREBUILT_KERNEL_ROMS_DIR=${PREBUILT_KERNEL_ROMS_DIR}")
    fi
    cmake "${cmake_opts[@]}"
    cmake --build "$BUILD" --target altirraqt -j

    # 2) Stage AppDir via cmake install.
    APPDIR="${BUILD}/AppDir"
    rm -rf "$APPDIR"
    DESTDIR="$APPDIR" cmake --install "$BUILD" --prefix /usr

    # 3) Walk Qt deps + write AppImage.
    OUT="${DIST}/altirraqt-${VERSION}-${ARCH}.AppImage"
    rm -f "$OUT"
    export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake)}"
    export VERSION="$VERSION"
    export OUTPUT="${OUT}"
    cd "$BUILD"
    "$LD_AI" --appdir "$APPDIR" \
             --plugin qt \
             --output appimage \
             --desktop-file "${APPDIR}/usr/share/applications/altirraqt.desktop"

    # linuxdeploy honors $OUTPUT (some versions) but also drops a copy
    # in $PWD. Pick whichever exists and move to dist/.
    if [[ -f "$OUT" ]]; then :
    elif compgen -G "$BUILD/altirraqt-*.AppImage" >/dev/null; then
        mv -f "$BUILD"/altirraqt-*"${ARCH}.AppImage" "$OUT" 2>/dev/null \
          || mv -f "$BUILD"/altirraqt-*.AppImage "$OUT"
    elif compgen -G "$ROOT/altirraqt-*.AppImage" >/dev/null; then
        mv -f "$ROOT"/altirraqt-*.AppImage "$OUT"
    else
        echo "linuxdeploy didn't produce an AppImage" >&2
        exit 1
    fi
    chmod +x "$OUT"
    cd "$ROOT"
else
    # ---- Strategy B ----------------------------------------------------
    if ! command -v nix >/dev/null 2>&1; then
        echo "Neither qmake6 nor nix are on PATH; cannot build AppImage." >&2
        exit 2
    fi
    echo "=== bundling .#default into AppImage (nix-appimage) ==="
    TMP_LINK="${ROOT}/altirraqt.AppImage"
    rm -f "$TMP_LINK"
    ( cd "$ROOT" && nix bundle --bundler github:ralismark/nix-appimage .#default )
    OUT="${DIST}/altirraqt-${VERSION}-${ARCH}.AppImage"
    cp -L "$TMP_LINK" "$OUT"
    chmod +x "$OUT"
    rm -f "$TMP_LINK"
fi

echo
echo "AppImage: $OUT"
ls -lh "$OUT"
