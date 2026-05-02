//	Altirra - Qt port
//	Top-level menu construction. See menus.cpp for the full layout.

#pragma once

#include <QString>

class QMainWindow;
class QSettings;
class QWidget;
class ATSimulator;
class IVDVideoDisplay;

struct ATAltirraMenuContext {
	QMainWindow      *window;
	ATSimulator      *sim;
	QSettings        *settings;
	QWidget          *displayWidget = nullptr;  // QOpenGLWidget for grab/copy
	IVDVideoDisplay  *display       = nullptr;  // for filter-mode etc.
};

void ATBuildAltirraMenuBar(ATAltirraMenuContext& ctx);

// Mount the image at `path`, ColdReset, and update the Recently Booted
// MRU list in QSettings. No-op if `path` is empty. Pops a Qt warning if
// the simulator refuses the image.
void ATBootImage(ATAltirraMenuContext& ctx, const QString& path);

// Re-apply all persisted runtime settings (audio levels, filter/scanline/
// stretch, overscan, memory + video standard + CPU mode, hardware mode,
// power-on delay, BASIC enabled, cassette decode toggles, indicator
// margin, joystick keys) to the simulator and display from
// ctx.settings. Does not call ColdReset — the caller decides.
void ATApplyAllSettings(ATAltirraMenuContext& ctx);

// Copy every key under `profiles/<name>/...` in ctx.settings to the
// matching top-level key. Returns false if no such profile exists.
bool ATApplyProfile(QSettings& settings, const QString& name);

// Defined in main.cpp. Maps a Qt::Key (post-shift) to a 7-bit Atari POKEY
// scan code (bits 0-5 = code, bit 6 = shift, bit 7 = ctrl). Returns -1 if
// the key has no Atari mapping.
int atariScanCodeFromQtKey(int qtKey, bool shift);
