#!/usr/bin/env bash
#	tools/build-flatpak.sh
#
#	Build a Flatpak bundle of altirraqt using the manifest at
#	packaging/flatpak/com.virtualdub.altirraqt.yml.
#
#	Output: dist/altirraqt-<version>.flatpak (a single-file bundle that
#	users can install with `flatpak install altirraqt-*.flatpak`).
#
#	Requires: flatpak, flatpak-builder, the org.kde.Platform/Sdk//6.7
#	          runtime + SDK (auto-installed on first run).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${ROOT}/dist"
WORK="${ROOT}/build-flatpak"
REPO="${WORK}/repo"
APPID="com.virtualdub.altirraqt"
MANIFEST="${ROOT}/packaging/flatpak/${APPID}.yml"
mkdir -p "$DIST"

VERSION="$(git -C "$ROOT" describe --tags --always 2>/dev/null || echo dev)"

if ! command -v flatpak-builder >/dev/null 2>&1; then
    echo "flatpak-builder not found on PATH." >&2
    echo "Install via: flatpak install flathub org.flatpak.Builder" >&2
    echo "  (or via your distro's flatpak-builder package)." >&2
    exit 2
fi

# Ensure the runtime / SDK we depend on are installed (idempotent).
flatpak --user remote-add --if-not-exists \
    flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak --user install -y flathub \
    org.kde.Platform//6.7 org.kde.Sdk//6.7

echo "=== flatpak-builder ==="
flatpak-builder \
    --user --install-deps-from=flathub \
    --force-clean \
    --repo="$REPO" \
    "$WORK/build" \
    "$MANIFEST"

OUT="${DIST}/altirraqt-${VERSION}.flatpak"
echo "=== flatpak bundle ==="
flatpak build-bundle "$REPO" "$OUT" "$APPID"

echo
echo "Flatpak written to: $OUT"
ls -lh "$OUT"
echo
echo "Install locally with:"
echo "  flatpak --user install --bundle $OUT"
echo "Run with:"
echo "  flatpak run $APPID"
