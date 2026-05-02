//	Altirra - Qt port
//	Stubs for symbols referenced by altirra_core but living in source files
//	excluded from the Qt build (console/debugger/settings/tracing/etc.).
//	Each block is the minimal definition needed to satisfy the linker; real
//	implementations come back as the UI layer is ported.

#include <cstdarg>
#include <cstdio>

#include <vd2/system/VDString.h>
#include <vd2/system/refcount.h>
#include <vd2/system/function.h>
#include <vd2/Kasumi/pixmap.h>
#include <vd2/VDDisplay/display.h>

#include <at/atcore/asyncdispatcher.h>
#include <at/atcore/enumparseimpl.h>

//-----------------------------------------------------------------------------
// g_sim — global pointer normally set by main.cpp; lots of files use it for
// non-essential debug/UI hooks. Null is acceptable; checks elsewhere are safe.
//-----------------------------------------------------------------------------

#include <irqcontroller.h>
#include <simulator.h>
ATSimulator *g_sim = nullptr;

//-----------------------------------------------------------------------------
// Console output — upstream routes these to the debugger console. When the
// Qt Debugger Console pane is open, ATDebuggerConsoleSink() consumes the
// text and pushes it into the QPlainTextEdit. Otherwise we fall through to
// stderr so debug output is still visible in headless / pre-UI runs.
//-----------------------------------------------------------------------------

#include "debuggerconsolepane.h"

void ATConsoleWrite(const char *s) {
	if (!s) return;
	if (!ATDebuggerConsoleSink(s))
		std::fputs(s, stderr);
}

void ATConsolePrintfImpl(const char *format, ...) {
	va_list val;
	va_start(val, format);
	char buf[2048];
	const int n = std::vsnprintf(buf, sizeof buf, format, val);
	va_end(val);
	if (n > 0) {
		if (!ATDebuggerConsoleSink(buf))
			std::fputs(buf, stderr);
	}
}

void ATConsoleTaggedPrintfImpl(const char *format, ...) {
	va_list val;
	va_start(val, format);
	char buf[2048];
	const int n = std::vsnprintf(buf, sizeof buf, format, val);
	va_end(val);
	if (n > 0) {
		if (!ATDebuggerConsoleSink(buf))
			std::fputs(buf, stderr);
	}
}

//-----------------------------------------------------------------------------
// Debugger glue — ATGetDebugger() / ATGetDebuggerSymbolLookup() are now real
// (defined in upstream src/Altirra/source/debugger.cpp, compiled into the
// build). The supporting Win32-only console / UI-pane / source-window /
// fullscreen hooks the debugger calls into are all UI plumbing that doesn't
// exist on the Qt side yet (the user-facing Debugger Console / Disassembly /
// source-window panes are tracked separately in PLAN.md). For each one
// the only correct portable definition right now is the "nothing to do" /
// "no such pane" / "no source window" answer.
//
//   * ATConsoleCheckBreak — Win32 polls a console-side break key. Qt has no
//     such console; false (no break requested) is correct.
//   * ATIsDebugConsoleActive — query for the visible debugger console
//     window. We have none; false is correct.
//   * ATOpenConsole — would create / show the debugger console window.
//     There's nothing to open; no-op.
//   * ATSetFullscreen — Win32 leaves fullscreen when the debugger pauses.
//     Qt fullscreen lives in main.cpp (toggleable manually); the debugger
//     UI isn't wired in yet, so the simulator-pause callback that calls
//     this has no UI to interact with. No-op until the debugger pane lands.
//   * ATGetUIPane / ATActivateUIPane — UI-pane registry. No panes exist;
//     null / no-op.
//   * ATOpenSourceWindow — opens a source-view window for stepping. No
//     source-view UI; nullptr is correct.
//   * ATConsoleOpenLogFile / ATConsoleCloseLogFile / ATConsoleCloseLogFileNT
//     — back the .logopen / .logclose debugger commands, which are only
//     reachable via the Debugger Console pane. With no pane, these
//     commands are unreachable; no-ops keep the linker happy. When the
//     console pane lands and ATConsoleWrite starts teeing through a real
//     log file, these definitions move alongside it.
//-----------------------------------------------------------------------------

class ATUIPane;
class IATSourceWindow;
struct ATDebuggerSourceFileInfo;

bool ATConsoleCheckBreak() { return false; }
bool ATIsDebugConsoleActive() { return false; }
void ATOpenConsole() {}
void ATSetFullscreen(bool) {}
ATUIPane *ATGetUIPane(uint32) { return nullptr; }
void ATActivateUIPane(uint32, bool, bool, uint32, int) {}
IATSourceWindow *ATOpenSourceWindow(const ATDebuggerSourceFileInfo &, bool) { return nullptr; }
void ATConsoleOpenLogFile(const wchar_t *) {}
void ATConsoleCloseLogFileNT() {}
void ATConsoleCloseLogFile() {}

//-----------------------------------------------------------------------------
// UI renderer — full no-op IATUIRenderer. Disk/cassette/audio status updates
// dispatch here; values are silently dropped until a real Qt-backed UI
// renderer is written.
//-----------------------------------------------------------------------------

#include <uirender.h>

namespace {
class ATUIRendererStub final : public IATUIRenderer {
public:
	int AddRef()  override { return ++mRef; }
	int Release() override { int n = --mRef; if (!n) delete this; return n; }

	// IATDeviceIndicatorManager
	void SetStatusFlags(uint32) override {}
	void ResetStatusFlags(uint32, uint32) override {}
	void PulseStatusFlags(uint32) override {}
	void SetStatusCounter(uint32, uint32) override {}
	void SetDiskLEDState(uint32 i, sint32 led) override {
		if (i < kMaxDrives) mDiskLED[i] = led;
	}
	void SetDiskMotorActivity(uint32 i, bool on) override {
		if (i < kMaxDrives) mDiskMotor[i] = on;
	}
	void SetDiskErrorState(uint32 i, bool err) override {
		if (i < kMaxDrives) mDiskError[i] = err;
	}
	void SetHActivity(bool on) override   { mHActivity = on; }
	void SetIDEActivity(bool, uint32) override {}
	void SetPCLinkActivity(bool) override {}
	void SetFlashWriteActivity() override {}
	void SetCartridgeActivity(sint32, sint32) override {}
	void SetCassetteIndicatorVisible(bool v) override { mCassetteVisible = v; }
	void SetCassettePosition(float pos, float, bool, bool) override { mCassettePos = pos; }
	void SetRecordingPosition() override {}
	void SetRecordingPositionPaused() override {}
	void SetRecordingPosition(float, sint64, bool) override {}
	void SetModemConnection(const char *) override {}
	void SetStatusMessage(const wchar_t *) override {}

	uint32 AllocateErrorSourceId() override { return ++mErrSrcId; }
	void   ClearErrors(uint32) override {}
	void   ReportError(uint32, const wchar_t *) override {}

	// IATUIRenderer
	bool IsVisible() const override { return false; }
	void SetVisible(bool) override {}
	void SetCyclesPerSecond(double) override {}
	void SetLedStatus(uint8) override {}
	void SetHeldButtonStatus(uint8) override {}
	void SetPendingHoldMode(bool) override {}
	void SetPendingHeldKey(int) override {}
	void SetPendingHeldButtons(uint8) override {}
	void ClearWatchedValue(int) override {}
	void SetWatchedValue(int, uint32, WatchFormat) override {}
	void SetTracingSize(sint64) override {}
	void SetAudioStatus(const ATUIAudioStatus *) override {}
	void SetAudioMonitor(bool, ATAudioMonitor *) override {}
	void SetAudioDisplayEnabled(bool, bool) override {}
	void SetAudioScopeEnabled(bool) override {}
	void SetSlightSID(ATSlightSIDEmulator *) override {}
	vdrect32 GetPadArea() const override { return vdrect32(0, 0, 0, 0); }
	void SetPadInputEnabled(bool) override {}
	void SetFpsIndicator(float) override {}
	void SetMessage(StatusPriority, const wchar_t *) override {}
	void ClearMessage(StatusPriority) override {}
	void SetHoverTip(int, int, const wchar_t *) override {}
	void SetPaused(bool) override {}
	void SetUIManager(ATUIManager *) override {}
	void Relayout(int, int) override {}
	void Update() override {}
	sint32 GetIndicatorSafeHeight() const override { return 0; }
	void AddIndicatorSafeHeightChangedHandler(const vdfunction<void()> *) override {}
	void RemoveIndicatorSafeHeightChangedHandler(const vdfunction<void()> *) override {}
	void BeginCustomization() override {}

public:
	// Indicator state — read by main.cpp's status-bar updater.
	static constexpr unsigned kMaxDrives = 8;
	bool   mDiskMotor[kMaxDrives] {};
	sint32 mDiskLED[kMaxDrives]   {};
	bool   mDiskError[kMaxDrives] {};
	bool   mHActivity     = false;
	bool   mCassetteVisible = false;
	float  mCassettePos     = 0.0f;

private:
	int mRef = 0;
	uint32 mErrSrcId = 0;
};

// Singleton handle so main.cpp can poll indicator state. Set by
// ATCreateUIRenderer when the simulator first asks for one.
ATUIRendererStub *g_atUIRenderer = nullptr;

} // namespace

// Public hook for main.cpp / menus.cpp to observe indicator state.
struct ATIndicatorState {
	bool diskMotor[8] {};
	bool diskError[8] {};
	bool hActivity   = false;
};

ATIndicatorState ATGetIndicatorState() {
	ATIndicatorState s;
	if (g_atUIRenderer) {
		for (unsigned i = 0; i < ATUIRendererStub::kMaxDrives; ++i) {
			s.diskMotor[i] = g_atUIRenderer->mDiskMotor[i];
			s.diskError[i] = g_atUIRenderer->mDiskError[i];
		}
		s.hActivity = g_atUIRenderer->mHActivity;
	}
	return s;
}

void ATCreateUIRenderer(IATUIRenderer **pp) {
	if (!pp) return;
	auto *r = new ATUIRendererStub;
	g_atUIRenderer = r;
	r->AddRef();
	*pp = r;
}

//-----------------------------------------------------------------------------
// Timer service — real implementation lives in timerservice_qt.cpp (QTimer-
// backed). The Win32 impl in atcore is excluded from this build.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Modem TCP driver — real implementation lives in modemdriver_qt.cpp
// (QTcpSocket / QTcpServer backed).
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Native tracer — real impl lives in upstream src/Altirra/source/tracenative.cpp
// (now compiled into altirra_core); ATGetNameForWindowMessageW32 is provided
// by debug_win32_stub.cpp.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Settings load/save callbacks — real registration backed by ATNotifyList,
// matching the upstream contract in src/Altirra/h/settings.h. Devices register
// callbacks here so they can serialize per-device state through a VDRegistryKey
// adapter (which on Linux is backed by VDRegistryProviderMemory). The callbacks
// fire on demand from main.cpp via ATSettingsFireLoadCallbacks /
// ATSettingsFireSaveCallbacks at startup and shutdown.
//-----------------------------------------------------------------------------

#include <vd2/system/registry.h>
#include <at/atcore/notifylist.h>
#include <settings.h>

namespace {
	ATNotifyList<const ATSettingsLoadSaveCallback *>& SettingsLoadCallbacks() {
		static ATNotifyList<const ATSettingsLoadSaveCallback *> s;
		return s;
	}

	ATNotifyList<const ATSettingsLoadSaveCallback *>& SettingsSaveCallbacks() {
		static ATNotifyList<const ATSettingsLoadSaveCallback *> s;
		return s;
	}
}

void ATSettingsRegisterLoadCallback(const ATSettingsLoadSaveCallback *fn) {
	SettingsLoadCallbacks().Add(fn);
}

void ATSettingsUnregisterLoadCallback(const ATSettingsLoadSaveCallback *fn) {
	SettingsLoadCallbacks().Remove(fn);
}

void ATSettingsRegisterSaveCallback(const ATSettingsLoadSaveCallback *fn) {
	SettingsSaveCallbacks().Add(fn);
}

void ATSettingsUnregisterSaveCallback(const ATSettingsLoadSaveCallback *fn) {
	SettingsSaveCallbacks().Remove(fn);
}

// Dispatch helpers called by main.cpp at startup (load) and shutdown (save).
// We use a single anonymous-profile id (0) and the "all categories" mask:
// this Qt port has one active profile and stores everything via QSettings;
// the upstream multi-profile machinery isn't wired up. The VDRegistryKey we
// pass devices is rooted at "AltirraQt\\Settings" in the in-memory provider.
void ATSettingsFireLoadCallbacks() {
	VDRegistryKey key("AltirraQt\\Settings", false, false);
	SettingsLoadCallbacks().NotifyAllDirect(
		[&](const ATSettingsLoadSaveCallback *cb) {
			(*cb)(0u, kATSettingsCategory_All, key);
		}
	);
}

void ATSettingsFireSaveCallbacks() {
	VDRegistryKey key("AltirraQt\\Settings", false, true);
	SettingsSaveCallbacks().NotifyAllDirect(
		[&](const ATSettingsLoadSaveCallback *cb) {
			(*cb)(0u, kATSettingsCategory_All, key);
		}
	);
}

//-----------------------------------------------------------------------------
// VDVideoDisplayFrame — owned by GTIA's frame queue. Upstream impl lives in
// display.cpp (Win32 display manager, excluded). Refcount + dtor only.
//-----------------------------------------------------------------------------

VDVideoDisplayFrame::VDVideoDisplayFrame() : mRefCount(0) {}
VDVideoDisplayFrame::~VDVideoDisplayFrame() {}

int VDVideoDisplayFrame::AddRef() { return ++mRefCount; }
int VDVideoDisplayFrame::Release() {
	int rc = --mRefCount;
	if (!rc)
		delete this;
	return rc;
}

// Directory watcher: real implementation lives in directorywatcher_qt.cpp.
