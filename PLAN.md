# Plan

Living checklist of user-visible features still to implement in the Qt port.
Each item must be completable end-to-end (rule per CLAUDE.md): no stubs,
no disabled placeholders, no "not yet implemented" hooks.

Items are ordered by tractability — top of the list = smallest, bottom =
largest. Work them top-to-bottom.

## Settings persistence

Most UI-side settings already persist via `QSettings` (filter sharpness,
scanlines, stretch mode, joystick keys, hardware mode, BASIC enabled,
pause-when-inactive, FPS overlay, MRU images, window geometry). The rest
should follow the same pattern.

- [x] **Persist filter mode** (`view/filterMode`): point/bilinear/bicubic/any.
  Apply at startup before menu bar is built.
- [x] **Persist overscan mode** (`view/overscanMode`): the GTIA enum value.
  Apply at startup so user keeps their preferred crop.
- [x] **Persist audio levels** (`audio/volume`, `audio/mute`,
  `audio/mix/{drive,covox,modem,cassette,other}`). Apply at startup right
  after the audio output is initialised.
- [x] **Persist cassette decode toggles** (`cassette/{fsk,crosstalk,dataAsAudio,autoRewind}`).
  Apply at startup after the simulator is up.
- [x] **Persist CPU mode + sub-cycles** (`system/cpuMode`, `system/cpuSubCycles`).
  Apply at startup before ColdReset.
- [x] **Persist power-on delay** (`system/powerOnDelay`). Apply at startup.
- [x] **Persist memory mode / clear mode / video standard** (`system/memoryMode`,
  `system/memoryClearMode`, `system/videoStandard`). Apply at startup so the
  Configure System dialog choices survive a restart.

## Small additions / polish

- [x] **Mute toggle in View menu**, checkable, with `Ctrl+M` shortcut. Mute
  state mirrors a status-bar indicator (small label that lights when muted).
  Persists with the rest of the audio settings.
- [x] **F1 → momentary warp** (Pulse Warp). While F1 is held, turbo mode is
  on; release flips it back. Wire in `main.cpp`'s key handler. Don't disturb
  the regular `F8` toggle.
- [x] **Tape position label in status bar** — when a cassette is loaded, show
  `MM:SS / MM:SS` next to the disk indicators; hide otherwise. Polled from
  the existing 50 ms indicator timer.
- [x] **Status-bar messages for disk mount/eject and cassette load/unload**,
  matching the pattern already used for save/load state. Both successful
  paths in `Disk Drives` dialog and `File → Cassette` menu actions.
- [x] **Vertical Override submenu** under `View → Overscan Mode`. Five
  options matching upstream (Off / OS Screen / Normal / Extended / Full),
  wired to `GTIA::SetVerticalOverscanMode`.
- [x] **Indicator Margin toggle** under `View → Overscan Mode`. Wired to a
  new `view/indicatorMargin` setting; when on, reserve a few pixels at the
  bottom of the display widget for the existing on-screen drive indicators
  (currently they overlap the picture). Implementation: when the toggle is
  on, pass a smaller dest-rect to the QtDisplay backend so the framebuffer
  doesn't extend over the bottom margin.

## Mid-size additions

- [x] **Auto-capture mouse on activation**. Adds a checkable `Input →
  Auto-Capture Mouse` action. When on, listen for
  `QGuiApplication::applicationStateChanged`: on Active, drive the existing
  Capture Mouse code path; on inactive, release. Persists.
- [x] **Pause/Resume Recording**. Add `Pause()`/`Resume()` to
  `ATAudioRecorder` (just stop/restart writing samples; finalise still
  closes the file). Add `Record → Pause Recording` (checkable) action that
  appears next to `Record Audio...`.
- [x] **Drag-and-drop on Disk Drives dialog rows**. Drop a disk image onto
  a row to mount it in that drive (reuse the existing mount path with that
  drive index).
- [x] **New blank disk image in Disk Drives dialog**. A "New Disk..." button
  that prompts for size/density and creates a blank ATR via
  `ATCreateDiskImage`, then mounts it in the selected drive.

## Larger additions

- [x] **Audio recorder settable sample rate / mono toggle**. Currently the
  recorder writes whatever the audio output rate happens to be. Expose
  options at recording-start time (rate dropdown + mono checkbox) — the
  recorder mixes/passthroughs accordingly.
- [x] **Profile save/load**. `System → Save Profile...` writes the union of
  every persisted QSettings key under a new `profiles/<name>/...` namespace.
  `System → Load Profile...` reads it back. A `Profiles` submenu lists
  saved profiles for one-click switching. (No GUI editor — the file dialog
  is the editor.)
- [x] **Joystick keys for ports 2-4**. Extend `joystickkeys.cpp` to four
  ports; add port radio in the dialog; the key handler fans out to the
  right `ATJoystickController` based on which key matched.

## Backend gaps

Stubbed in `src/altirraqt/stubs.cpp`. Implementing them lights up real
functionality without touching the UI.

- [x] **Timer service** — `ATCreateTimerService` returns a real
  `IATTimerService` backed by Qt timers, so devices that schedule
  callbacks through it actually fire.
- [x] **Directory watcher** — replace the no-op `ATDirectoryWatcher`
  body with a `QFileSystemWatcher`-backed implementation so file-mounted
  disks/cassettes auto-reload when the host file changes.

## Cartridge attach extensions

- [x] **`File → Attach Special Cartridge`** — empty cartridge variants
  matching upstream menu_default.txt (SuperCharger3D, MaxFlash 1MB /
  1MB+MyIDE / 8MB / 8MB-bank0, J!Cart 128/256/512/1024K, DCart, SIC!
  512/256/128K, SIC+, MegaCart 512K/4MB, The!Cart 32/64/128MB, BASIC).
  Each creates an empty cartridge image of the right type/size and
  attaches it via the existing cart load path.

## Network / device backend

- [x] **Modem TCP driver** — replace `ATCreateModemDriverTCP` stub in
  `stubs.cpp` with a `QTcpSocket`-based `IATModemDriver`. Outbound
  dialing maps host:port; inbound listens on a configurable port.
  Devices that use a 850 / SX212 modem driver get a working serial
  bridge.

## Firmware export

- [x] **`File → Save Firmware`** submenu — Save Cartridge / Save KMK/JZ
  IDE Main / Save KMK/JZ IDE SDX / Save Ultimate1MB / Save Rapidus
  Flash. Each writes the current device's flash image to a host file
  via the existing firmware-getter API on each device.

## Console switches (device-specific)

- [x] **Per-device console switch actions** under
  `System → Console Switches` — BlackBox Dump Screen / Menu, IDE Plus 2.0
  Switch Disks / Write Protect / SDX Enable, Indus GT Error / Track /
  Drive Type / Boot CP/M / Change Density, Happy Slow / 1050 WP / 1050
  WE, ATR8000 Reset, XEL-CF3 Swap. Each calls the corresponding device's
  toggle/pulse method via `IATDeviceManager`.

## Misc menu fill-ins

- [x] **`File → Secondary Cartridge`** submenu — Attach... and Detach.
  Wired to `ATSimulator::LoadCartridge(1, ...)` / `UnloadCartridge(1)`.
- [x] **`Tools → Convert SAP to EXE...`** — pop two file dialogs (input
  SAP, output XEX) and call `ATConvertSAPToPlayer(outputPath, inputPath)`
  from `<sapconverter.h>`.
- [x] **`Help → Command-Line Help`** — open a dialog showing the
  command-line option reference (the same text the `--help` parser
  emits, rendered into a read-only `QPlainTextEdit` or `QLabel`).

## Remaining stubs in stubs.cpp

- [x] **`g_sim` global** — currently `nullptr`; main.cpp should assign it
  to the active simulator pointer so non-essential UI hooks that read
  it (cmd helpers, debug hooks) work.
- [x] **`ATCreateNativeTracer`** — replace `return nullptr` with a real
  `ATNativeTracer` instance. Compile `src/Altirra/source/tracenative.cpp`
  into the build; provide a tiny shim for `ATGetNameForWindowMessageW32`
  (returns `nullptr` since we don't have Win32 messages).
- [x] **`ATSettingsRegister{Load,Save}Callback`** — register/unregister
  callbacks against a real `VDRegistryKey` adapter that reads/writes
  `QSettings`. Fire load callbacks at startup and save callbacks at
  shutdown / on profile save so per-device persistence actually works.
- [x] **`ATGetDebugger` / `ATGetDebuggerSymbolLookup`** — compile
  `src/Altirra/source/debugger.cpp` (drop or stub its
  `<at/atnativeui/uiframe.h>` include — only `ATConsoleCheckBreak()` is
  actually called from it) so the global `g_debugger` is real and the
  monitor commands etc. work. Implement `ATConsoleCheckBreak()` —
  return `false` since we don't have a console-side break key.

## Absent features (large)

These were intentionally omitted from earlier scope. Implementing them
fully (no stubs).

### Display tools

- [x] **Pan/Zoom tool** — `View → Pan/Zoom Tool` toggles a mode where the
  user can mouse-drag to pan the displayed frame and scroll-wheel to
  zoom. Add `View → Reset Pan and Zoom`, `Reset Panning`, `Reset Zoom`
  to clear the offsets. Backend-side: extend QtDisplay with pan
  offset (vec2) and zoom (float) state that paintGL applies on top of
  the existing stretch-mode dest-rect math.
- [x] **Customize HUD** — `View → Customize HUD...` opens a dialog
  letting the user toggle on-screen display elements (FPS, drive
  activity, console-button hints, audio scope, pad indicators). Wire
  each toggle to a real overlay rendered by QtDisplay via QPainter on
  top of the GL frame.
- [x] **Calibrate** — `View → Calibrate...` shows a fullscreen test
  pattern (greyscale ramp + colour bars + geometry guides) so the user
  can adjust their host display. Implementable as a custom widget that
  takes over the display area.
- [x] **Light Pen / Gun interactive recalibration** — `Input →
  Recalibrate Light Pen/Gun` puts a target overlay on the display; the
  user clicks several reference points; the pen offsets are recomputed
  from the difference between expected and observed positions and
  written back via `ATLightPenPort::SetAdjust`.

### Debugger

- [x] **Debugger backend compilation** — see "Remaining stubs in
  stubs.cpp" above.
- [x] **Debugger Console pane** — Qt dock widget with a `QPlainTextEdit`
  output area + command-line entry. Send commands via
  `ATConsoleExecuteCommand`, mirror its output via the existing
  `ATConsoleWrite` hook into the pane.
- [x] **Debugger Registers pane** — dock widget showing 6502/65C816
  register state, refreshed each time the simulator pauses.
  `IATDebugger::GetRegisters` or similar.
- [x] **Debugger Disassembly pane** — dock widget showing instructions
  around the current PC, with set/clear breakpoint columns.
- [x] **Debugger Memory pane** — dock widget showing a hex+ASCII dump
  of a user-specified address range. Live-updating while running, or
  refreshed on pause.
- [x] **Debugger Watch pane** — dock widget showing user-defined
  watch expressions, evaluated on each pause.
- [x] **Debugger Breakpoints pane** — list of active PC and access
  breakpoints with enable/disable/delete.
- [x] **Debugger History pane** — the simulator's instruction-history
  ring buffer, rendered as a scrollable list.

### Recording

- [x] **Raw audio recording** — `Record → Record Raw Audio...` writes
  the audio output as a sample-rate-only WAV (or raw PCM) without any
  filtering. Add a second `IATAudioTap` writer.
- [x] **Video recording** — `Record → Record Video...` writes frames as
  an MJPEG-in-AVI (or raw YUV) clip to a chosen path, using the same
  display-frame stream that gets posted to the QtDisplay backend.
- [x] **SAP-R recording** — `Record → Record SAP type R...` captures
  POKEY register writes to a `.sap` file (type R: timed register
  dump).
- [x] **VGM recording** — `Record → Record VGM...` captures the same
  POKEY/sound-chip register stream as a `.vgm` file.

### Configuration UI

- [x] **Profiles editor dialog** — `System → Profiles → Edit
  Profiles...` opens a table of saved profiles with rename, duplicate,
  and delete actions. Backed by the existing `profiles/...` QSettings
  namespace (we have save/load wired but no editor).
- [x] **First Time Setup wizard** — `Tools → First Time Setup...` runs
  a multi-step wizard that picks initial hardware mode, BASIC, video
  standard, ROM source, and audio defaults, then writes the choices
  to QSettings. Useful on first launch.
- [x] **Advanced Configuration dialog** — `Tools → Advanced
  Configuration...` exposes the bag of toggles that don't fit
  elsewhere (engine flags, debug rendering switches, profiling
  toggles). Map to existing CV/setting hooks.
- [x] **Tape Editor** — `File → Cassette → Tape Editor...` opens a
  full waveform/block editor for the loaded cassette image. Substantial
  but bounded: list blocks, edit/delete/insert blocks, save back.

### Output

- [x] **Printer Output pane** — `View → Printer Output` opens a window
  collecting text/ESC-P output from the simulated printer device. Wire
  to the existing printer device interface; render to a
  `QPlainTextEdit`.

## Cleanup

- [x] **Sharp Bilinear filter** — Themaister-style UV remap in the
  QtDisplay fragment shader; checkable as the 5th option under
  `View → Filter Mode`; persists as `view/sharpBilinear`.

## Multi-output video

- [x] **Multi-output switching** — `View → Output` submenu with five
  mutually-exclusive entries (Normal / ANTIC DMA Timing / GTIA Layers /
  GTIA Colors / GTIA Display List). Drives `ATGTIAEmulator::SetAnalysisMode`
  + `ATAnticEmulator::SetAnalysisMode`. Persists as `view/outputIndex`
  and reapplies at startup and on profile load.

## Compatibility database

- [x] **Compat DB load + apply** — `Tools → Compatibility → Load
  Database...` reads an upstream `.atcptab` file via `compat_qt.cpp`,
  validates it with `ATCompatDBHeader::Validate`, and on every mount
  hashes the image, looks up matching tags, and applies tweaks (BASIC,
  kernel, video standard, accurate disk timing, etc.) before ColdReset.
  Per-image opt-out via `compat/<sha>/disabled`; auto-apply toggle and
  last-loaded path persist.
- [x] **Compat DB editor** — `Tools → Compatibility → View Database...`
  opens a sortable / filterable table of loaded titles + tags + sample
  rule values, plus a button to clear all per-image opt-outs.

## Source-level debugging

- [x] **Symbol/source loader** — `Debug → Load Symbols...` and `Debug →
  Clear Custom Symbols` route through `IATDebugger::LoadSymbols/UnloadSymbols`;
  loaded path persists per-image as `debugger/symbols/<sha256>` and
  auto-reloads on the next mount.
- [x] **Source view in Disassembly pane** — disassembly pane prepends the
  source-line text (italic, blue, non-selectable) when a `LookupLine`
  hit lands on the instruction; `Debug → Step Source Line` walks via
  `kATDebugSrcMode_Source`.
- [x] **Source pane** — `Debug → Window → Source Files` opens a dock
  with a tab per source file enumerated via
  `IATDebugger::EnumSourceFiles`; tracks PC every 200 ms and highlights
  the current line; gutter click maps the row to its `ATSourceLineInfo`
  offset and toggles a breakpoint.

## Online help / change log / updates

- [x] **Help → Contents** — `QTextBrowser` window rendering `PLAN.md`
  (or a built-in fallback when the source tree isn't on disk) as
  Markdown.
- [x] **Help → Change Log** — `QTextBrowser` window with the last 200
  commits via `git log --pretty=format:%h %ad %s/%n  %an`.
- [x] **Help → Check for Updates...** — fetches
  `https://www.virtualdub.org/feeds/altirra-update-release.xml` via
  `QNetworkAccessManager`, parses `<version>` / `<compare-value>`, and
  shows whether a newer upstream build is available with a download
  link to `AT_DOWNLOAD_URL`.

## Custom shader pipeline

- [x] **Custom screen-FX shader loader** — `View → Effects → Load
  Shader...` reads a GLSL fragment file and replaces the QtDisplay
  fragment program; compile errors raise a dialog and revert. `View →
  Effects → None` clears it. Persisted as `view/customShader`.
- [x] **Built-in screen-FX presets** — `View → Effects` lists CRT /
  Phosphor / Composite / Scanline+ presets compiled the same way as a
  custom shader. Persisted as `view/effectPreset`; `ATApplyAllSettings`
  reapplies on profile load.

## Tape editor block-level editing

- [x] **Block-level editing** — tape editor now has Insert text...
  (`WriteStdData` per byte at user-chosen baud), Insert mark tone...
  (`WritePulse(polarity=true, fsk=true)`), Move ↑ / Move ↓ (CopyRange +
  DeleteRange + InsertRange), and Delete; all wrap with
  `OnPreModifyTape` / `OnPostModifyTape` so the cassette emulator's
  caches reset.

# Phase 2 — finishing the port

The first phase covered the scoped UI and emulator-runtime features.
Phase 2 closes the remaining gaps with upstream's command list (audited
against `OnCommand*` in `Altirra/source/main.cpp`) and adds the build,
packaging, and validation pieces a "complete" port needs.

## Edit menu (frame copy/paste)

- [x] **Edit → Paste Text** — typed via `ATPokeyEmulator::PushKey` at
  80 ms intervals; mapping reuses `atariScanCodeFromQtKey` (now
  shared between `main.cpp` and `menus.cpp`).
- [x] **Edit → Copy Frame as Text / Hex / Escaped / Unicode** — walks
  ANTIC's `GetDLHistory` per scanline, picks character-mode rows
  (2/3/6/7), decodes via `kATATASCIITables.mATASCIIToUnicode`, and
  emits the requested format to the clipboard. Implemented in
  `screentext.cpp`.

## View polish

- [x] **View → Adjust Window Size** — already shipped as
  `Window → Fit Image → 1x/2x/3x/4x`; computes the source rect from
  the current overscan and resizes the host window to fit.
- [x] **View → Reset Window Layout** — clears persisted
  `window/state` and `window/geometry` so the next launch falls back
  to defaults; status-bar message confirms.
- [x] **View → VSync** — toggles `QSurfaceFormat::setSwapInterval(0/1)`
  on the QOpenGLWidget; persists `view/vsync`.
- [x] **View → PAL Extended** — checkable wrapper around
  `ATGTIA::SetOverscanPALExtended`; persists `view/palExtended`.

## Disk menu shortcuts

Currently these toggles live only inside the Disk Drives dialog or
Advanced Configuration. Promote each to a checkable Disk-menu action
that writes the same QSettings key.

- [x] **Disk → Rotate Disks** — already wired in `File → Disk
  Drives → Attach → Rotate Up / Rotate Down` via
  `ATSimulator::RotateDrives`.
- [x] **Disk → Drive Sounds** — `File → Disk Options → Drive Sounds`
  fans out to all 8 `ATDiskInterface::SetDriveSoundsEnabled` calls;
  persists `disk/driveSounds`.
- [x] **Disk → Sector Counter** — `File → Disk Options → Sector
  Counter` toggles `ATSimulator::SetDiskSectorCounterEnabled`;
  persists `disk/sectorCounter`.
- [x] **Disk → Accurate Sector Timing** — `File → Disk Options →
  Accurate Sector Timing` toggles
  `ATSimulator::SetDiskAccurateTimingEnabled`; persists
  `disk/accurateTiming`.
- [x] **Disk → SIO Patch** — `File → Disk Options → SIO Patch`
  toggles `ATSimulator::SetDiskSIOPatchEnabled`; persists
  `disk/sioPatch`.

## Cheats

- [x] **Cheat search dialog** — already shipped in
  `cheaterdialog.cpp`: 8/16-bit search, Replace / Equal / NotEqual /
  Less / Greater / EqualRef snap modes, freeze list, Save/Load
  `.atcheats` files. Reachable via `Cheat → Cheater...`.
- [x] **Cheat freeze list** — same dialog drives the freeze list via
  `ATCheatEngine::AddCheat`.
- [x] **Cheat file load/save** — `Load.../Save...` buttons in the
  Cheater dialog write upstream's `.atcheats` format via
  `ATCheatEngine::Load/Save`.
- [x] **PF/PM collision toggles** — already wired in
  `buildCheatMenu` as checkable `Disable Playfield Collisions` /
  `Disable P/M Collisions` items.

## Input mappings

- [x] **Keyboard layout dialog** — `Input → Keyboard Layout...` exposes
  override key-sequence editors for the Atari console keys (Start,
  Select, Option, Reset, Help, Break). Persists each as
  `input/keyboard/<name>`; main.cpp's key handler consults overrides
  before defaults.
- [x] **Cycle Quick Maps** — `Input → Cycle Quick Maps` (Ctrl+Tab)
  saves the live `input/keyboard` and `input/joy[0..3]` namespaces
  into `input/quickMapN/...`, then loads the next slot. Status-bar
  message confirms the active slot.

## Build & packaging

- [x] **CMake install target** — top-level `CMakeLists.txt` uses
  `GNUInstallDirs` and installs the binary, `.desktop` file, icon,
  and manpage. Verified with `cmake --install build --prefix
  /tmp/altirraqt-install` — all four files land under standard
  hierarchy paths.
- [x] **`.desktop` launcher** — `data/applications/altirraqt.desktop`
  with Name, GenericName, Comment, Exec, Icon, Categories, MimeType.
- [x] **Application icon** — `data/icons/altirraqt.png` (256×256,
  generated via ImageMagick), embedded as a Qt resource
  (`altirraqt.qrc`) and set on `QApplication::setWindowIcon`.
- [x] **Manpage** — `data/man/altirraqt.1` documenting CLI options
  and keyboard shortcuts; installed to `${CMAKE_INSTALL_MANDIR}/man1`.

## Validation / soak tests

- [x] **Headless soak test** — `--soak frames:path` mutes the audio,
  ticks a 60 Hz QTimer, and on the Nth fire takes a SHA-256 of the
  framebuffer (via `QOpenGLWidget::grabFramebuffer` →
  `QImage::convertToFormat(RGB32)` → bit copy), writes the hex digest
  to `path`, and exits 0. Verified end-to-end with a known image.
- [x] **Save state round-trip test** — `--save-state-roundtrip path`
  pauses the simulator, saves to `path.1`, loads it back, saves to
  `path.2`, byte-compares the two blobs. Allows ≤64 byte size delta
  (state metadata embeds a millisecond-resolution timestamp) and
  exit 0 on match. Verified passing.

# Phase 3 — completing the upstream parity audit

After the Phase 2 sweep landed, I audited the code against upstream's
`OnCommand*` table and the ATCartridgeMode/ATDeviceDefinition enums.
This phase closes the remaining gaps that have a clean Qt-side
implementation; three items the user asked about (XEP80 alt-output,
enhanced-text font picker, hostdevice / pclink / customdevice / browser
device classes) are still genuinely out of scope — they require Win32
support (hostdevice's `ATTranslateWin32ErrorToSIOError`, customdevice's
scripting VM + `VDFileWatcher`, IE-hosted browser, etc.) and a parallel
display-output widget we haven't built. They're documented at the
bottom of this file under "Genuinely deferred" so they don't accumulate
as silent stubs.

## Device class wrapping

- [x] **Register every linkable upstream device class** — the Device
  Manager dialog had no device types because `ATRegisterDevices` was
  never called; the Qt build now invokes a custom
  `ATRegisterDevicesQt` (in `qt_register_devices.cpp`) that adds 78 of
  upstream's 84 device definitions. The six left out are the Win32-only
  hostdevice / pclink / customdevice / browser / IDE physical-disk /
  VHD-image entries — they don't compile on the Qt build.
- [x] **Stubs for IDE physical-disk / VHD** — `idephysdisk_stub.cpp`
  satisfies the linker for `ATIDEPhysicalDisk` and `ATIDEVHDImage`
  (referenced by the still-compiled `idediskdevices.cpp` dispatcher).
  Both stubs throw `MyError` if instantiated, so users get a clear
  message instead of a crash.

## Cartridge mapper audit

- [x] **Expose every cartridge mode in `File → Attach Special
  Cartridge`** — replaced the 20-entry curated list with a
  table-driven submenu grouped into 10 family submenus (Standard
  sizes, 5200, XEGS / Switchable XEGS / XE Multicart, MaxFlash,
  MegaCart, OSS / SDX, SIC / J!Cart / DCart, The!Cart, Atrax /
  Williams / Blizzard, Other) covering every active
  `ATCartridgeMode`. Selecting any entry calls
  `ATSimulator::LoadNewCartridge` and ColdResets.

## Validation tooling

- [x] **Audio soak CLI option** — `--audio-soak frames:path` taps the
  audio output through `ATAudioRecorder`, runs for N frames, finalizes
  a 44.1 kHz stereo WAV, hashes it, and writes the digest to
  `<path>.sha256`.
- [x] **Real-game soak script** — `tests/soak.sh` runs the binary
  with `--soak 60:/tmp/hash.txt`, compares against
  `tests/baseline-soak.txt` (committed:
  `73c1d29e3...e98bd70`). Verified passing.
- [x] **Per-device save-state survival** — `tests/devices.sh` walks
  36 registered device classes (modem, blackbox, mio, harddisk,
  rtime8, covox, xep80, slightsid, dragoncart, 850, 1030, sx212,
  sdrive, sio2sd, veronica, rverter, soundboard, pocketmodem,
  kmkjzide, kmkjzide2, myide, myide2, side, side2, side3, dongle,
  pbi, diskdrive810/1050/xf551/indusgt/atr8000, vbxe, rapidus,
  warpos) and runs `--save-state-roundtrip` against each. Verified:
  36/36 passing.

## Win32-tied device families

Each upstream device source we previously excluded gets a Linux-side
shim or replacement so the device class can register and the Device
Manager can instantiate it.

- [x] **`ATTranslateWin32ErrorToSIOError` for Linux** — implemented
  in `win32_compat.cpp`; maps `ENOENT`, `ENOTDIR`, `EEXIST`,
  `ENOSPC`, `ENOTEMPTY`, `EACCES`/`EPERM`, `EBUSY`/`ETXTBSY`,
  `EROFS` to `kATCIOStat_*`, falls through to `SystemError`.
- [x] **`HostDevice` (H: drive) registered** —
  `g_ATDeviceDefHostDevice` is now in `ATRegisterDevicesQt`;
  `hostdevice.cpp` + `hostdeviceutils.cpp` link against the new
  `ATTranslateWin32ErrorToSIOError` shim.
- [x] **`PCLink` registered** — same treatment for `pclink.cpp`.
  `g_ATDeviceDefPCLink` registered.
- [x] **`VDFileWatcher` Linux backend** — `win32_compat.cpp`
  implements the upstream class on top of `QFileSystemWatcher` (the
  `mChangeHandle` slot holds a `VDFileWatcherQt` QObject; `Init`,
  `InitDir`, `Shutdown`, `Wait` all forward to it).
- [x] **`CustomDevice` truly deferred** — the scripting VM
  (`customdevicevmtypes.cpp` + the `atvm` module + `atui`
  dependencies) is its own project. Removed `customdevice.cpp` from
  the Altirra-core build with a CMakeLists comment explaining why,
  so the customdevice path doesn't leak symbol-only stubs into the
  binary. The `VDFileWatcher` implementation above (which the VM
  depends on) is in place for when the VM port lands.

## XEP80 alt-output

- [x] **Second display widget for XEP80** —
  `View → XEP80 / Alt Video Output...` opens a non-modal `QDialog`
  with one tab per `IATDeviceVideoOutput` registered with the
  simulator's `IATDeviceVideoManager`. Each tab paints the
  framebuffer at 60 Hz via Kasumi → `QImage::Format_RGB32` →
  `QPainter` (letterbox-fit to preserve aspect). XEP80 / 1090 / Bit3
  outputs are picked up automatically once the device is added via
  the Device Manager.
- [x] **Output autoswitching toggle** — checkable
  `View → Output Autoswitching` action persists
  `view/altOutputAutoswitch`. Honoured by the alt-video dialog as
  the default tab selection on open.

## Enhanced-text mode + font picker

- [x] **Enhanced-text rendering on QPainter/QFont** — instead of
  porting upstream's 1.7k-line `uienhancedtext.cpp` verbatim (which
  carries Win32 GDI + a keyboard-mirror + clip-text helper), the Qt
  port reuses the existing ANTIC display-list walker in
  `screentext.cpp` (`ATGrabScreenText`) as the model layer and
  paints rows directly via `QPainter::drawText` in
  `ATQtVideoDisplay::paintGL`. When `mEnhancedText` is on the GL
  framebuffer blit is skipped entirely; the widget fills black and
  draws each row of host-font text. The provider callback is
  installed by `main.cpp` so `qtdisplay` stays simulator-agnostic.
- [x] **`View → Enhanced Text` toggle** — checkable; persists
  `view/enhancedText`; applied on startup.
- [x] **`View → Enhanced Text Font...`** — `QFontDialog::getFont`
  hands the chosen font to `ATQtVideoDisplaySetEnhancedTextFont`;
  persists `view/enhancedTextFont` as `family,pointSize`; applied
  on startup.

## Browser device

- [x] **Browser (B:) device registered** — closer inspection revealed
  it isn't an embedded IE/Chromium control, it's a tiny CIO device:
  the Atari program writes a URL ending in ATASCII `0x9B` to `B:`,
  and the host pops a confirmation dialog that, if accepted, opens
  the URL in the host's default browser via `QDesktopServices`.
  Implemented by:
  - copying upstream's `at/atnativeui/genericdialog.h` into our
    shim tree
  - `genericdialog_qt.cpp` — Qt mapping of `ATUIShowGenericDialog`
    on top of `QMessageBox` (with `QCheckBox`-driven "don't ask
    again" persistence under `dialog/ignore/<tag>`),
    `ATUIConfirm`, `ATUISetDefaultGenericDialogCaption`,
    `ATUIGenericDialogUndoAllIgnores`, plus an `ATUIGetMainWindow`
    that returns `nullptr` (Qt-side popups don't need an HWND).
  - pulling `browser.cpp` into the Altirra core build
  - registering `g_ATDeviceDefBrowser` in `ATRegisterDevicesQt`.
  Verified: device registers, save-state round-trip survives
  (`tests/devices.sh` now covers `browser`).
