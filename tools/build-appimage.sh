#!/usr/bin/env bash
#	tools/build-appimage.sh
#
#	Build a self-contained AppImage of altirraqt via nix-appimage.
#	The flake's `packages.default` derivation is bundled as-is — no
#	linuxdeploy / no glibc/FHS host gymnastics — and the resulting
#	AppImage runs on any Linux system thanks to AppImage's bundled
#	dynamic linker.
#
#	Output: dist/altirraqt-<git-describe>-<arch>.AppImage
#
#	Requires: nix (with flakes enabled). The bundler ships its own
#	musl + AppImage runtime, so no other tooling is needed.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${ROOT}/dist"
mkdir -p "$DIST"

ARCH="$(uname -m)"
VERSION="$(git -C "$ROOT" describe --tags --always 2>/dev/null || echo dev)"

if ! command -v nix >/dev/null 2>&1; then
    echo "nix not found. Install Nix (https://nixos.org/download.html) and enable flakes." >&2
    exit 2
fi

echo "=== bundling .#default into AppImage ==="

# nix-appimage drops a symlink to the bundled AppImage in $PWD/altirraqt.AppImage
# (named after the derivation's mainProgram). We move it into dist/ with a
# version + arch suffix.
TMP_LINK="${ROOT}/altirraqt.AppImage"
rm -f "$TMP_LINK"
( cd "$ROOT" && nix bundle \
    --bundler github:ralismark/nix-appimage \
    .#default )

if [[ ! -L "$TMP_LINK" ]]; then
    echo "nix bundle didn't produce $TMP_LINK" >&2
    exit 1
fi

OUT="${DIST}/altirraqt-${VERSION}-${ARCH}.AppImage"
cp -L "$TMP_LINK" "$OUT"
chmod +x "$OUT"
rm -f "$TMP_LINK"

echo
echo "AppImage: $OUT"
ls -lh "$OUT"
echo
echo "Run with:  $OUT"
echo "Soak test: $OUT --soak 60:/tmp/hash.txt"
