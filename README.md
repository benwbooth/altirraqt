# altirraqt

A Qt6 port of [Altirra](https://www.virtualdub.org/altirra.html) — Avery Lee's accurate emulator of the Atari 800 / 800XL / 130XE / 5200 home computers and game console.

The bundled AltirraOS kernel and Altirra BASIC ROMs are BSD-licensed by Avery Lee and built from source — the emulator works out of the box without needing the original Atari ROMs.

## Install

Pre-built bundles are attached to every [release](https://github.com/benwbooth/altirraqt/releases).

### Linux — AppImage

```sh
chmod +x altirraqt-v*-x86_64.AppImage
./altirraqt-v*-x86_64.AppImage
```

Runs on any glibc Linux, no install needed.

### Linux — Flatpak

One-off bundle install:

```sh
flatpak --user install --bundle altirraqt-v*.flatpak
flatpak run com.virtualdub.altirraqt
```

Or add the [auto-update repo](https://benwbooth.github.io/altirraqt/) so `flatpak update` picks up new releases:

```sh
flatpak --user remote-add --no-gpg-verify altirraqt \
    https://benwbooth.github.io/altirraqt/altirraqt.flatpakrepo
flatpak --user install altirraqt com.virtualdub.altirraqt
```

### macOS — DMG (Apple Silicon)

Mount the DMG and drag `altirraqt.app` to `/Applications`. The app isn't notarized; on first launch macOS will say *"altirraqt is damaged"*. Clear the quarantine attribute once:

```sh
xattr -cr /Applications/altirraqt.app
```

then double-click as normal.

### Windows — Installer

Run `altirraqt-v*-windows-x86_64-installer.exe`. The installer is unsigned, so SmartScreen will warn — click *More info* → *Run anyway*. Adds Start Menu and desktop shortcuts; uninstall via Add/Remove Programs.

## Build from source

```sh
nix develop --command cmake -S . -B build
nix develop --command cmake --build build -j
```

Or for the full release bundles:

```sh
tools/release.sh           # AppImage + Flatpak
tools/build-appimage.sh    # AppImage only
tools/build-flatpak.sh     # Flatpak only
```

## License

GPL-2.0-or-later. The Altirra core (`src/Altirra/`, `src/AT*/`, etc.) is © Avery Lee under GPL-2.0; the Qt port glue (`src/altirraqt/`, `src/QtDisplay/`, `src/QtOSHelper/`) is © Ben Booth under the same license. See [LICENSE](LICENSE) and [AUTHORS](AUTHORS).

The bundled AltirraOS kernel and Altirra BASIC ROMs are © Avery Lee under a BSD-style license; both are built from source by the build.
