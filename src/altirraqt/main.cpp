//	Altirra - Qt port
//	Smoke-test entry point: instantiates ATSimulator, hooks its GTIA video
//	output to a QOpenGLWidget-backed IVDVideoDisplay, optionally registers
//	a user-supplied kernel ROM, and drives the emulator from a QTimer.
//
//	Usage: altirraqt [--kernel /path/to/atari800xl.rom] [--basic /path/to/atari_basic.rom]
//
//	With no --kernel argument we still construct ATSimulator but skip
//	ColdReset; the window comes up empty. With a kernel ROM, ColdReset
//	runs and the emulator should start executing 6502 code.

#include <array>
#include <memory>
#include <cstdio>

#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMimeData>
#include <QPainter>
#include <QStatusBar>
#include <QUrl>

// Defined here, referenced by menus.cpp's View → Show FPS toggle.
bool g_atShowFps = false;

// Indicator state polled from the UI renderer stub each timer tick.
struct ATIndicatorState;
ATIndicatorState ATGetIndicatorState();
struct ATIndicatorState {
	bool diskMotor[8] {};
	bool diskError[8] {};
	bool hActivity = false;
};

#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <vd2/system/function.h>
#include <vd2/system/error.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCommandLineParser>
#include <QImage>
#include <QKeySequence>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QOpenGLWidget>
#include <QPixmap>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "menus.h"
#include "customizehuddialog.h"
#include "firsttimewizard.h"

#include <constants.h>
#include <cassette.h>
#include <cpu.h>
#include <savestateio.h>
#include <vd2/VDDisplay/display.h>
#include <at/atcore/audiomixer.h>

// Defined in savestateio.cpp but not declared in the header — installs the
// global ATIO save-state-reader hook so .atstate2 loads work.
extern void ATInitSaveStateDeserializer();

// Provided by stubs.cpp. Fire registered settings callbacks (e.g. devices'
// per-device state) at startup and shutdown so per-device persistence works.
extern void ATSettingsFireLoadCallbacks();
extern void ATSettingsFireSaveCallbacks();

#include <at/qtdisplay/qtvideodisplay.h>

#include "joystickkeys.h"
#include <at/ataudio/audiooutput.h>
#include <at/ataudio/pokey.h>
#include <devicemanager.h>
#include <at/atcore/deviceport.h>
#include <at/atcore/media.h>
#include <firmwaremanager.h>
#include <gtia.h>
#include <inputcontroller.h>
#include <inputdefs.h>
#include <irqcontroller.h>
#include <simulator.h>

#include <QCryptographicHash>
#include <QFile>
#include <QFileDialog>
#include <QIcon>
#include <QtCore/Qt>

#include "audiorecorder.h"
#include "keyboardlayoutdialog.h"
#include "menus.h"
#include "screentext.h"

// Atari POKEY scan codes are 6-bit key codes in bits 0-5; bit 6 is shift,
// bit 7 is ctrl. The Atari 800XL keyboard layout doesn't match a US PC
// 1:1 — punctuation like + * = ( ) live on dedicated keys, and Shift+,
// produces [, not <. Qt sends post-shift Key codes for shifted symbols
// (Qt::Key_Plus instead of Qt::Key_Equal+ShiftMod), so the table covers
// both unshifted and shifted Qt keys, encoding the right Atari shift bit
// in each entry. Declared in menus.h so the Edit menu's Paste action can
// reuse it.
int atariScanCodeFromQtKey(int qtKey, bool shift) {
	struct Map { int qt; int atari; };
	static constexpr Map kKeys[] = {
		// Letters (lowercase Atari). Caller may add 0x40 for shift.
		{Qt::Key_A, 0x3F}, {Qt::Key_B, 0x15}, {Qt::Key_C, 0x12}, {Qt::Key_D, 0x3A},
		{Qt::Key_E, 0x2A}, {Qt::Key_F, 0x38}, {Qt::Key_G, 0x3D}, {Qt::Key_H, 0x39},
		{Qt::Key_I, 0x0D}, {Qt::Key_J, 0x01}, {Qt::Key_K, 0x05}, {Qt::Key_L, 0x00},
		{Qt::Key_M, 0x25}, {Qt::Key_N, 0x23}, {Qt::Key_O, 0x08}, {Qt::Key_P, 0x0A},
		{Qt::Key_Q, 0x2F}, {Qt::Key_R, 0x28}, {Qt::Key_S, 0x3E}, {Qt::Key_T, 0x2D},
		{Qt::Key_U, 0x0B}, {Qt::Key_V, 0x10}, {Qt::Key_W, 0x2E}, {Qt::Key_X, 0x16},
		{Qt::Key_Y, 0x2B}, {Qt::Key_Z, 0x17},

		// Digit row, unshifted.
		{Qt::Key_0, 0x32}, {Qt::Key_1, 0x1F}, {Qt::Key_2, 0x1E}, {Qt::Key_3, 0x1A},
		{Qt::Key_4, 0x18}, {Qt::Key_5, 0x1D}, {Qt::Key_6, 0x1B}, {Qt::Key_7, 0x33},
		{Qt::Key_8, 0x35}, {Qt::Key_9, 0x30},

		// Whitespace + control keys.
		{Qt::Key_Space,     0x21}, {Qt::Key_Return,    0x0C},
		{Qt::Key_Enter,     0x0C}, {Qt::Key_Escape,    0x1C},
		{Qt::Key_Backspace, 0x34}, {Qt::Key_Delete,    0x34},
		{Qt::Key_Tab,       0x2C},

		// XL/XE Help key. Upstream Altirra binds this to host F6.
		{Qt::Key_F6, 0x11},

		// Unshifted punctuation (US-equivalent placement).
		{Qt::Key_Comma,     0x20}, {Qt::Key_Period,    0x22},
		{Qt::Key_Slash,     0x26}, {Qt::Key_Semicolon, 0x02},
		{Qt::Key_Minus,     0x0E}, {Qt::Key_Equal,     0x0F},
		{Qt::Key_Apostrophe, 0x73 ^ 0x40},  // Atari ' is shift+7; raw key 0x33

		// Shifted punctuation: Qt sends the post-shift key, and the Atari
		// scan codes already include the shift bit (0x40).
		{Qt::Key_Exclam,      0x1F | 0x40},  // Shift+1 = !
		{Qt::Key_QuoteDbl,    0x1E | 0x40},  // Shift+2 = "
		{Qt::Key_NumberSign,  0x1A | 0x40},  // Shift+3 = #
		{Qt::Key_Dollar,      0x18 | 0x40},  // Shift+4 = $
		{Qt::Key_Percent,     0x1D | 0x40},  // Shift+5 = %
		{Qt::Key_Ampersand,   0x1B | 0x40},  // Shift+6 = & (on Atari)
		{Qt::Key_QuoteLeft,   0x33 | 0x40},  // Shift+7 = '  (Atari quote)
		{Qt::Key_ParenLeft,   0x30 | 0x40},  // Shift+9 = (
		{Qt::Key_ParenRight,  0x32 | 0x40},  // Shift+0 = )
		{Qt::Key_At,          0x35 | 0x40},  // Shift+8 = @ (Atari)
		{Qt::Key_Asterisk,    0x07},         // Atari * key (shift+: on PC also)
		{Qt::Key_Plus,        0x06},         // Atari + key (shift+= on PC)
		{Qt::Key_Colon,       0x02 | 0x40},  // Shift+; = :
		{Qt::Key_Less,        0x36 | 0x40},  // Shift+, = < (on Atari, Shift+, = [)
		{Qt::Key_Greater,     0x37 | 0x40},  // Shift+. = > (on Atari, Shift+. = ])
		{Qt::Key_Question,    0x26 | 0x40},  // Shift+/ = ?
		{Qt::Key_Underscore,  0x0E | 0x40},  // Shift+- = _
		{Qt::Key_BracketLeft, 0x60 | 0x40},  // [
		{Qt::Key_BracketRight,0x61 | 0x40},  // ]
		{Qt::Key_AsciiCircum, 0x47},         // ^ on Atari
		{Qt::Key_Bar,         0x0F | 0x40},  // Shift+= = | on Atari
	};
	for (const auto& m : kKeys) {
		if (m.qt == qtKey) {
			// The entry already has the right shift bit baked in. The
			// caller's "shift" arg only matters for plain letters/digits.
			if (m.atari & 0x40)
				return m.atari;
			return m.atari | (shift ? 0x40 : 0);
		}
	}
	return -1;
}

uint64 registerFirmware(ATFirmwareManager& fwman, const QString& path,
                        ATFirmwareType type, const wchar_t *name) {
	if (path.isEmpty())
		return 0;

	const std::wstring w = path.toStdWString();

	// AddFirmware derives the id from the path via ATGetFirmwareIdFromPath,
	// not from info.mId. So we compute the same id and pass it back to the
	// caller so it can hand it to SetKernel/SetBasic.
	ATFirmwareInfo info;
	info.mId = ATGetFirmwareIdFromPath(w.c_str());
	info.mFlags = 0;
	info.mbVisible = true;
	info.mbAutoselect = true;
	info.mName = name;
	info.mPath = w.c_str();
	info.mType = type;

	fwman.AddFirmware(info);
	return info.mId;
}

// libvdpau resolves the driver name from the X server's vendor string,
// so on a system that reports "NVIDIA Corporation" but doesn't have the
// proprietary driver installed it tries libvdpau_nvidia.so and prints
// "Failed to open VDPAU backend ..." to stderr before falling back. The
// warning is harmless (we don't actually use VDPAU) but noisy. If the
// user hasn't already set VDPAU_DRIVER, point it at the first existing
// mesa VDPAU driver we can find.
static void ATQuietVdpauProbe() {
	if (qEnvironmentVariableIsSet("VDPAU_DRIVER"))
		return;
	static constexpr const char *kSearch[] = {
		"/run/opengl-driver/lib/vdpau",  // NixOS / Nix
		"/usr/lib/vdpau",
		"/usr/lib/x86_64-linux-gnu/vdpau",
		"/usr/lib64/vdpau",
	};
	static constexpr const char *kPick[] = {
		"va_gl", "radeonsi", "nouveau", "r600", "virtio_gpu", "d3d12",
	};
	for (const char *dir : kSearch) {
		for (const char *drv : kPick) {
			const QString path = QStringLiteral("%1/libvdpau_%2.so")
				.arg(QString::fromUtf8(dir), QString::fromUtf8(drv));
			if (QFile::exists(path)) {
				qputenv("VDPAU_DRIVER", drv);
				return;
			}
		}
	}
	// Nothing found — point it at /dev/null so libvdpau errors-out
	// silently rather than printing the "no nvidia driver" warning.
	qputenv("VDPAU_DRIVER_PATH", "/dev/null/no-vdpau-driver");
	qputenv("VDPAU_DRIVER", "noop");
}

int main(int argc, char *argv[]) {
	ATQuietVdpauProbe();
	QApplication app(argc, argv);
	app.setOrganizationName(QStringLiteral("Altirra Qt port"));
	app.setApplicationName(QStringLiteral("altirraqt"));
	app.setWindowIcon(QIcon(QStringLiteral(":/altirraqt/icon.png")));
	app.setApplicationVersion(QStringLiteral("0.1"));

	QSettings settings;

	// First-launch detection: if QSettings has no system/hardwareMode key,
	// run the First Time Setup wizard so the user picks their initial
	// machine, video standard, BASIC default, and audio levels before the
	// simulator is constructed. Cancelling falls through to the built-in
	// defaults and proceeds normally.
	if (!settings.contains(QStringLiteral("system/hardwareMode")))
		(void)ATRunFirstTimeWizard(nullptr, &settings);

	// Auto-load the last profile, if any, before reading any settings.
	// ATApplyProfile copies profiles/<name>/<key> over <key> in QSettings,
	// so the rest of startup picks them up via the normal apply path.
	{
		const QString last = settings.value(QStringLiteral("system/lastProfile")).toString();
		if (!last.isEmpty())
			ATApplyProfile(settings, last);
	}

	ATLoadJoyKeys(settings);

	QCommandLineParser parser;
	parser.setApplicationDescription(
		QStringLiteral("Altirra Atari emulator (Qt port, work-in-progress)"));
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption kernelOpt(
		QStringLiteral("kernel"),
		QStringLiteral("Path to an Atari kernel ROM (Atari 800XL OS, Atari 800 OS-B, etc.)"),
		QStringLiteral("file"));
	QCommandLineOption basicOpt(
		QStringLiteral("basic"),
		QStringLiteral("Path to an Atari BASIC ROM"),
		QStringLiteral("file"));

	QCommandLineOption runAnywayOpt(
		QStringLiteral("run-anyway"),
		QStringLiteral("Resume the simulator after ColdReset. The emulator runs the boot sequence and renders frames; without --run-anyway it stays paused after ColdReset (smoke-test mode)."));

	QCommandLineOption screenshotOpt(
		QStringLiteral("screenshot"),
		QStringLiteral("After <seconds> of run time, save a screenshot of the display widget to <path>, then quit."),
		QStringLiteral("seconds:path"));

	QCommandLineOption typeOpt(
		QStringLiteral("type"),
		QStringLiteral("Synthesise a sequence of key presses (each character of <text>) into the running emulator after a one-second startup pause. Useful for headless smoke-testing keyboard input."),
		QStringLiteral("text"));

	QCommandLineOption imageOpt(
		QStringLiteral("image"),
		QStringLiteral("Mount <file> as a disk/cassette/cartridge — Altirra's auto-detect figures out which from the contents."),
		QStringLiteral("file"));

	QCommandLineOption saveStateAtOpt(
		QStringLiteral("save-state-at"),
		QStringLiteral("After <seconds>, save the current emulator state to <path>. Useful for headless smoke-testing."),
		QStringLiteral("seconds:path"));

	QCommandLineOption loadStateOpt(
		QStringLiteral("load-state"),
		QStringLiteral("Load <file> as a save state at startup."),
		QStringLiteral("file"));

	QCommandLineOption soakOpt(
		QStringLiteral("soak"),
		QStringLiteral("Run for <frames> rendered frames, hash the final framebuffer "
		               "as SHA-256, write hex to <path>, then exit. CI gate."),
		QStringLiteral("frames:path"));

	QCommandLineOption audioSoakOpt(
		QStringLiteral("audio-soak"),
		QStringLiteral("Record the audio output to a WAV at <path> while "
		               "running for <frames> frames, hash the WAV, write the "
		               "hex digest to <path>.sha256, and exit. Captures POKEY-"
		               "side regressions a framebuffer-only hash would miss."),
		QStringLiteral("frames:path"));

	QCommandLineOption stateRoundtripOpt(
		QStringLiteral("save-state-roundtrip"),
		QStringLiteral("After ColdReset, save state, load it back, save again, and "
		               "compare the two saved blobs. Exits 0 on match, 1 on mismatch. "
		               "Argument: target path for the diagnostic blobs."),
		QStringLiteral("path"));

	parser.addOption(kernelOpt);
	parser.addOption(basicOpt);
	parser.addOption(runAnywayOpt);
	parser.addOption(screenshotOpt);
	parser.addOption(typeOpt);
	parser.addOption(imageOpt);
	parser.addOption(saveStateAtOpt);
	parser.addOption(loadStateOpt);
	parser.addOption(soakOpt);
	parser.addOption(audioSoakOpt);
	parser.addOption(stateRoundtripOpt);
	parser.process(app);

	const QString kernelPath = parser.value(kernelOpt);
	const QString basicPath  = parser.value(basicOpt);

	// Build a QMainWindow with a menu bar; the OpenGL display is the
	// central widget so it stretches to fill on resize.
	QMainWindow window;
	window.setAcceptDrops(true);
	window.setWindowTitle(QStringLiteral("Altirra (Qt port)"));
	if (settings.contains(QStringLiteral("window/geometry"))) {
		window.restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
	} else {
		// Default to a comfortable 3x integer scale of the Atari NTSC
		// visible area (336x240 normal overscan, PAR ~7/6) plus chrome.
		// 960x720 lands on a typical 1080p screen with room to spare.
		window.resize(960, 720 + window.menuBar()->sizeHint().height()
		                    + window.statusBar()->sizeHint().height());
	}

	QWidget *display = ATCreateQtVideoDisplay(&window);
	window.setCentralWidget(display);
	window.show();
	display->setFocus();

	IVDVideoDisplay *iface = ATQtVideoDisplayAsInterface(display);

	// Install the global save-state reader callback before constructing
	// the simulator. ATIO's load path checks for this hook and refuses
	// to load .atstate2 files without it.
	ATInitSaveStateDeserializer();

	auto sim = std::make_unique<ATSimulator>();

	// Lots of upstream code (cmd helpers, savestate hooks, debug glue) reads
	// the global g_sim pointer; light it up immediately after construction so
	// those paths see a live simulator.
	extern ATSimulator *g_sim;
	g_sim = sim.get();

	sim->Init();

	// Register the subset of upstream device definitions that link cleanly
	// without Win32 helpers. The full ATRegisterDevices() pulls in
	// hostdevice / pclink / customdevice / browser, all of which need
	// Win32-specific support routines (ATTranslateWin32ErrorToSIOError,
	// VDFileWatcher, customdevice scripting VM, IE COM browser host)
	// that we don't have on Linux. Defined in qt_register_devices.cpp.
	{
		extern void ATRegisterDevicesQt(ATDeviceManager& dm);
		ATRegisterDevicesQt(*sim->GetDeviceManager());
	}

	sim->GetGTIA().SetVideoOutput(iface);

	// Bring the audio mixer online. Without InitNativeAudio() the mixer's
	// minidriver pointer (mpAudioOut) is null and POKEY's first FlushAudio
	// dereferences it. Our null backend (audiooutnull.cpp) takes the place
	// of WaveOut/WASAPI here — emulator runs silent until a real Qt audio
	// backend is wired in.
	sim->GetAudioOutput()->InitNativeAudio();

	{
		auto *out = sim->GetAudioOutput();
		if (settings.contains(QStringLiteral("audio/volume")))
			out->SetVolume(settings.value(QStringLiteral("audio/volume")).toFloat());
		if (settings.contains(QStringLiteral("audio/mute")))
			out->SetMute(settings.value(QStringLiteral("audio/mute")).toBool());
		struct MixKey { ATAudioMix mix; const char *key; };
		static constexpr MixKey kMixKeys[] = {
			{kATAudioMix_Drive,    "audio/mix/drive"},
			{kATAudioMix_Covox,    "audio/mix/covox"},
			{kATAudioMix_Modem,    "audio/mix/modem"},
			{kATAudioMix_Cassette, "audio/mix/cassette"},
			{kATAudioMix_Other,    "audio/mix/other"},
		};
		for (const auto& mk : kMixKeys) {
			const QString key = QString::fromUtf8(mk.key);
			if (settings.contains(key))
				out->SetMixLevel(mk.mix, settings.value(key).toFloat());
		}

		// Advanced audio knobs from the Advanced Configuration dialog.
		// SetFiltersEnabled has no public getter, so we treat the QSettings
		// key as the source of truth and default to "on".
		out->SetFiltersEnabled(
			settings.value(QStringLiteral("audio/filtersEnabled"), true).toBool());
		if (settings.contains(QStringLiteral("audio/latencyMs")))
			out->SetLatency(settings.value(QStringLiteral("audio/latencyMs")).toInt());
		if (settings.contains(QStringLiteral("audio/extraBufferMs")))
			out->SetExtraBuffer(settings.value(QStringLiteral("audio/extraBufferMs")).toInt());
	}

	// Advanced engine + cassette knobs from the Advanced Configuration dialog.
	{
		auto& cpu = sim->GetCPU();
		if (settings.contains(QStringLiteral("engine/historyEnabled")))
			cpu.SetHistoryEnabled(
				settings.value(QStringLiteral("engine/historyEnabled")).toBool());
		if (settings.contains(QStringLiteral("engine/stopOnBrk")))
			cpu.SetStopOnBRK(
				settings.value(QStringLiteral("engine/stopOnBrk")).toBool());
		if (settings.contains(QStringLiteral("engine/illegalInsns")))
			cpu.SetIllegalInsnsEnabled(
				settings.value(QStringLiteral("engine/illegalInsns")).toBool());
		if (settings.contains(QStringLiteral("engine/nmiBlocking")))
			cpu.SetNMIBlockingEnabled(
				settings.value(QStringLiteral("engine/nmiBlocking")).toBool());

		auto& cas = sim->GetCassette();
		if (settings.contains(QStringLiteral("cassette/turboDecodeAlgorithm")))
			cas.SetTurboDecodeAlgorithm((ATCassetteTurboDecodeAlgorithm)
				settings.value(QStringLiteral("cassette/turboDecodeAlgorithm")).toInt());
		// SetRandomizedStartEnabled has no getter; treat QSettings as source.
		cas.SetRandomizedStartEnabled(
			settings.value(QStringLiteral("cassette/randomizedStart"), false).toBool());
	}

	const uint64 kernelId = registerFirmware(*sim->GetFirmwareManager(), kernelPath,
	                                          kATFirmwareType_KernelXL, L"User-supplied kernel");
	const uint64 basicId  = registerFirmware(*sim->GetFirmwareManager(), basicPath,
	                                          kATFirmwareType_Basic,    L"User-supplied BASIC");

	if (kernelId) sim->SetKernel(kernelId);
	if (basicId)  sim->SetBasic(basicId);

	// AltirraBASIC is bundled and auto-selected by the firmware manager;
	// enabling the BASIC ROM mapping makes the XL kernel boot straight
	// into the READY prompt when no cartridge is loaded.
	sim->SetBASICEnabled(settings.value(QStringLiteral("system/basicEnabled"), true).toBool());

	// Hardware mode persists across runs.
	const int hwMode = settings.value(QStringLiteral("system/hardwareMode"),
	                                  (int)kATHardwareMode_800XL).toInt();
	sim->SetHardwareMode((ATHardwareMode)hwMode);

	if (settings.contains(QStringLiteral("system/memoryMode")))
		sim->SetMemoryMode((ATMemoryMode)
			settings.value(QStringLiteral("system/memoryMode")).toInt());
	if (settings.contains(QStringLiteral("system/memoryClearMode")))
		sim->SetMemoryClearMode((ATMemoryClearMode)
			settings.value(QStringLiteral("system/memoryClearMode")).toInt());
	if (settings.contains(QStringLiteral("system/videoStandard")))
		sim->SetVideoStandard((ATVideoStandard)
			settings.value(QStringLiteral("system/videoStandard")).toInt());
	if (settings.contains(QStringLiteral("system/cpuMode"))
	    || settings.contains(QStringLiteral("system/cpuSubCycles"))) {
		const ATCPUMode cpuMode = (ATCPUMode)
			settings.value(QStringLiteral("system/cpuMode"),
			               (int)sim->GetCPUMode()).toInt();
		const uint32 subCycles = (uint32)
			settings.value(QStringLiteral("system/cpuSubCycles"),
			               (int)sim->GetCPUSubCycles()).toInt();
		sim->SetCPUMode(cpuMode, subCycles ? subCycles : 1);
	}
	if (settings.contains(QStringLiteral("system/powerOnDelay")))
		sim->SetPowerOnDelay(settings.value(QStringLiteral("system/powerOnDelay")).toInt());
	if (settings.contains(QStringLiteral("view/overscanMode")))
		sim->GetGTIA().SetOverscanMode((ATGTIAEmulator::OverscanMode)
			settings.value(QStringLiteral("view/overscanMode")).toInt());
	if (settings.contains(QStringLiteral("view/verticalOverscan")))
		sim->GetGTIA().SetVerticalOverscanMode((ATGTIAEmulator::VerticalOverscanMode)
			settings.value(QStringLiteral("view/verticalOverscan")).toInt());

	{
		const bool indMargin = settings.value(QStringLiteral("view/indicatorMargin"), false).toBool();
		ATQtVideoDisplaySetIndicatorMargin(display, indMargin ? 24 : 0);
	}

	{
		auto& cas = sim->GetCassette();
		if (settings.contains(QStringLiteral("cassette/fsk")))
			cas.SetFSKSpeedCompensationEnabled(
				settings.value(QStringLiteral("cassette/fsk")).toBool());
		if (settings.contains(QStringLiteral("cassette/crosstalk")))
			cas.SetCrosstalkReductionEnabled(
				settings.value(QStringLiteral("cassette/crosstalk")).toBool());
		if (settings.contains(QStringLiteral("cassette/dataAsAudio")))
			cas.SetLoadDataAsAudioEnable(
				settings.value(QStringLiteral("cassette/dataAsAudio")).toBool());
		if (settings.contains(QStringLiteral("cassette/autoRewind")))
			cas.SetAutoRewindEnabled(
				settings.value(QStringLiteral("cassette/autoRewind")).toBool());
	}

	if (settings.contains(QStringLiteral("view/filterMode")))
		iface->SetFilterMode((IVDVideoDisplay::FilterMode)
			settings.value(QStringLiteral("view/filterMode")).toInt());

	if (settings.contains(QStringLiteral("view/sharpBilinear")))
		ATQtVideoDisplaySetSharpBilinear(display,
			settings.value(QStringLiteral("view/sharpBilinear")).toBool());

	// Enhanced text mode + font. The provider walks ANTIC's display list
	// each frame via screentext.cpp's ATGrabScreenText helper.
	ATQtVideoDisplaySetEnhancedTextProvider(display, [sim = sim.get()]{
		const QString text = ATGrabScreenText(*sim, ATScreenCopyMode::Unicode);
		return text.split(QChar('\n'));
	});
	if (settings.contains(QStringLiteral("view/enhancedTextFont"))) {
		const QString f = settings.value(QStringLiteral("view/enhancedTextFont"))
		                          .toString();
		const QString family = f.section(QChar(','), 0, 0);
		const int     pts    = f.section(QChar(','), 1, 1).toInt();
		ATQtVideoDisplaySetEnhancedTextFont(display,
			QFont(family.isEmpty() ? QStringLiteral("monospace") : family,
			      pts > 0 ? pts : 14));
	}
	if (settings.value(QStringLiteral("view/enhancedText"), false).toBool())
		ATQtVideoDisplaySetEnhancedText(display, true);

	// With Avery's BSD-licensed ROMs embedded as Qt resources, the firmware
	// manager auto-selects an internal kernel for whatever hardware mode is
	// active. LoadROMs() forces UpdateKernel to fetch+install the kernel
	// before ColdReset runs the boot hooks that read from kernel memory.
	sim->LoadROMs();

	// Wire keyboard joysticks to all four ports, plus a paddle pair on
	// port 1 (mouse XY drives the pots).
	std::array<std::unique_ptr<ATJoystickController>, kATJoyPortCount> joys;
	for (int i = 0; i < kATJoyPortCount; ++i)
		joys[i] = std::make_unique<ATJoystickController>();
	auto paddleA  = std::make_unique<ATPaddleController>();
	auto paddleB  = std::make_unique<ATPaddleController>();
	paddleB->SetHalf(true);
	if (auto *portMgr = sim->GetDeviceManager()->GetService<IATDevicePortManager>()) {
		for (int i = 0; i < kATJoyPortCount; ++i)
			joys[i]->Attach(*portMgr, /*portIndex=*/i);
		paddleA ->Attach(*portMgr, /*portIndex=*/1);
		paddleB ->Attach(*portMgr, /*portIndex=*/1);
	}

	// Save settings on exit.
	QObject::connect(&app, &QApplication::aboutToQuit, [&]{
		settings.setValue(QStringLiteral("window/geometry"), window.saveGeometry());
		settings.setValue(QStringLiteral("system/hardwareMode"), (int)sim->GetHardwareMode());
		// Let registered settings callbacks (e.g. per-device state) write back
		// before the simulator unwinds.
		ATSettingsFireSaveCallbacks();
	});

	// Mount any --image=<file>. Altirra's auto-detect picks disk vs cart
	// vs cassette vs program from the file contents and extension.
	if (parser.isSet(imageOpt)) {
		const std::wstring path = parser.value(imageOpt).toStdWString();
		if (!sim->Load(path.c_str(), kATMediaWriteMode_RO, nullptr)) {
			std::fprintf(stderr, "[main] failed to mount image: %s\n",
			             parser.value(imageOpt).toUtf8().constData());
		} else {
			std::fprintf(stderr, "[main] mounted %s\n",
			             parser.value(imageOpt).toUtf8().constData());
		}
	}

	// Fire settings load callbacks before ColdReset so any device that
	// registered one in its constructor gets to deserialise per-device state
	// before the boot ROM runs.
	ATSettingsFireLoadCallbacks();

	sim->ColdReset();

	// The audio mixer dereferences state that isn't fully wired up on a
	// clean Linux Init+ColdReset path. On Windows it works because the
	// _DEBUG heap is zeroed and the WASAPI minidriver routes differently.
	// --run-anyway opts in if you've patched around it.
	if (parser.isSet(QStringLiteral("run-anyway")))
		sim->Resume();

	// Build the full menu bar matching upstream Altirra's layout — see
	// menus.cpp.
	ATAltirraMenuContext menuCtx{ &window, sim.get(), &settings, display, iface };
	ATBuildAltirraMenuBar(menuCtx);

	// Apply persisted HUD overlay toggles to the display widget so the
	// user's previous Customize HUD choices light up at startup.
	ATApplyHudSettings(display, &settings);

	// Drag-and-drop: dropping a supported file on the window boots it just
	// like File → Boot Image, including the MRU update. Uses an event
	// filter rather than subclassing QMainWindow to keep the wiring local.
	class ATDropFilter : public QObject {
	public:
		ATDropFilter(ATAltirraMenuContext& ctx) : mpCtx(&ctx) {}
		bool eventFilter(QObject *o, QEvent *e) override {
			if (e->type() == QEvent::DragEnter) {
				auto *de = static_cast<QDragEnterEvent *>(e);
				if (de->mimeData()->hasUrls()) {
					de->acceptProposedAction();
					return true;
				}
			} else if (e->type() == QEvent::Drop) {
				auto *de = static_cast<QDropEvent *>(e);
				const auto urls = de->mimeData()->urls();
				if (!urls.isEmpty()) {
					const QString path = urls.first().toLocalFile();
					if (!path.isEmpty()) {
						ATBootImage(*mpCtx, path);
						de->acceptProposedAction();
						return true;
					}
				}
			}
			return QObject::eventFilter(o, e);
		}
	private:
		ATAltirraMenuContext *mpCtx;
	};
	auto *dropFilter = new ATDropFilter(menuCtx);
	dropFilter->setParent(&window);
	window.installEventFilter(dropFilter);

	// Status bar — disk activity LEDs (8 drives) + a hard-disk activity dot.
	auto *statusBar = window.statusBar();
	QLabel *driveLabels[8] = {};
	for (int i = 0; i < 8; ++i) {
		driveLabels[i] = new QLabel(QStringLiteral("D%1").arg(i + 1));
		driveLabels[i]->setStyleSheet(QStringLiteral("color: #444; padding: 0 6px;"));
		statusBar->addPermanentWidget(driveLabels[i]);
	}
	auto *hdLabel = new QLabel(QStringLiteral("H:"));
	hdLabel->setStyleSheet(QStringLiteral("color: #444; padding: 0 6px;"));
	statusBar->addPermanentWidget(hdLabel);

	// Console switch indicators: Start (F2) / Select (F3) / Option (F4).
	auto *startLabel  = new QLabel(QStringLiteral("Start"));
	auto *selectLabel = new QLabel(QStringLiteral("Sel"));
	auto *optionLabel = new QLabel(QStringLiteral("Opt"));
	for (auto *l : {startLabel, selectLabel, optionLabel}) {
		l->setStyleSheet(QStringLiteral("color: #444; padding: 0 6px;"));
		statusBar->addPermanentWidget(l);
	}

	auto *muteLabel = new QLabel(QStringLiteral("Mute"));
	muteLabel->setStyleSheet(QStringLiteral("color: #444; padding: 0 6px;"));
	statusBar->addPermanentWidget(muteLabel);

	auto *tapePosLabel = new QLabel;
	tapePosLabel->setStyleSheet(QStringLiteral("color: #444; padding: 0 6px;"));
	tapePosLabel->setVisible(false);
	statusBar->addPermanentWidget(tapePosLabel);

	// Pause indicator: visible only while sim->IsPaused().
	auto *pauseLabel = new QLabel(QStringLiteral("PAUSED"));
	pauseLabel->setStyleSheet(QStringLiteral(
		"color: white; background: #b80; padding: 0 6px; font-weight: bold;"));
	pauseLabel->setVisible(false);
	statusBar->addPermanentWidget(pauseLabel);

	auto *indicatorTimer = new QTimer(&window);
	indicatorTimer->setInterval(50);
	// Live HUD source: latest measured fps + paddle position. Updated by
	// the simulator-tick timer below; sampled by this indicator timer so
	// the HUD refreshes in lock-step with the status bar.
	auto hudFps    = std::make_shared<double>(0.0);
	auto hudPadX   = std::make_shared<float>(0.5f);
	auto hudPadY   = std::make_shared<float>(0.5f);
	auto hudPadTA  = std::make_shared<bool>(false);
	auto hudPadTB  = std::make_shared<bool>(false);
	QObject::connect(indicatorTimer, &QTimer::timeout, &window,
		[driveLabels, hdLabel, startLabel, selectLabel, optionLabel,
		 pauseLabel, muteLabel, tapePosLabel, simPtr = sim.get(), display,
		 hudFps, hudPadX, hudPadY, hudPadTA, hudPadTB]{
		const ATIndicatorState s = ATGetIndicatorState();
		for (int i = 0; i < 8; ++i) {
			QString style;
			if (s.diskError[i])
				style = QStringLiteral("color: white; background: #c33; padding: 0 6px;");
			else if (s.diskMotor[i])
				style = QStringLiteral("color: white; background: #393; padding: 0 6px;");
			else
				style = QStringLiteral("color: #444; padding: 0 6px;");
			driveLabels[i]->setStyleSheet(style);
		}
		hdLabel->setStyleSheet(s.hActivity
			? QStringLiteral("color: white; background: #393; padding: 0 6px;")
			: QStringLiteral("color: #444; padding: 0 6px;"));

		// GTIA's console switch input register is active-low: a 0 bit means
		// the corresponding button is currently held. Bit 0 = Start, 1 =
		// Select, 2 = Option.
		const uint8 sw = simPtr->GetGTIA().ReadConsoleSwitchInputs();
		auto stylize = [](QLabel *l, bool active){
			l->setStyleSheet(active
				? QStringLiteral("color: white; background: #338; padding: 0 6px;")
				: QStringLiteral("color: #444; padding: 0 6px;"));
		};
		const bool startOn  = !(sw & 0x01);
		const bool selectOn = !(sw & 0x02);
		const bool optionOn = !(sw & 0x04);
		stylize(startLabel,  startOn);
		stylize(selectLabel, selectOn);
		stylize(optionLabel, optionOn);

		pauseLabel->setVisible(simPtr->IsPaused());

		muteLabel->setStyleSheet(simPtr->GetAudioOutput()->GetMute()
			? QStringLiteral("color: white; background: #a33; padding: 0 6px; font-weight: bold;")
			: QStringLiteral("color: #444; padding: 0 6px;"));

		auto& cas = simPtr->GetCassette();
		const bool casLoaded = cas.IsLoaded();
		float casPos = 0.0f, casLen = 0.0f;
		if (casLoaded) {
			casPos = cas.GetPosition();
			casLen = cas.GetLength();
			auto fmt = [](float seconds) {
				if (seconds < 0.0f) seconds = 0.0f;
				const int total = (int)(seconds + 0.5f);
				return QStringLiteral("%1:%2")
					.arg(total / 60, 2, 10, QLatin1Char('0'))
					.arg(total % 60, 2, 10, QLatin1Char('0'));
			};
			tapePosLabel->setText(QStringLiteral("%1 / %2")
				.arg(fmt(casPos))
				.arg(fmt(casLen)));
			tapePosLabel->setVisible(true);
		} else {
			tapePosLabel->setVisible(false);
		}

		// Keep the display widget's PAR fresh — it changes when the user
		// switches video standard or overscan mode.
		ATQtVideoDisplaySetPixelAspectRatio(display, simPtr->GetGTIA().GetPixelAspectRatio());

		// Push the same indicator data into the QtDisplay HUD so any
		// enabled overlays render with current values.
		ATQtVideoDisplaySetHudFps     (display, *hudFps);
		ATQtVideoDisplaySetHudDrives  (display, s.diskMotor, s.diskError);
		ATQtVideoDisplaySetHudCassette(display, casLoaded, casPos, casLen);
		ATQtVideoDisplaySetHudConsole (display, startOn, selectOn, optionOn);
		ATQtVideoDisplaySetHudPads    (display, *hudPadX, *hudPadY, *hudPadTA, *hudPadTB);
	});
	indicatorTimer->start();

	// Wire Qt key events to POKEY's keyboard. Modifiers route through the
	// dedicated SetShift/SetControl APIs — the rest is a single
	// PushRawKey / ReleaseAllRawKeys per event.
	ATJoystickController *joyPtrs[kATJoyPortCount];
	for (int i = 0; i < kATJoyPortCount; ++i) joyPtrs[i] = joys[i].get();
	// Resolve user keyboard-layout overrides (Input → Keyboard Layout)
	// once at startup. Falls through to defaults when no override is set.
	const int kOvStart  = ATKeyboardLayoutGetOverride(settings, "start");
	const int kOvSelect = ATKeyboardLayoutGetOverride(settings, "select");
	const int kOvOption = ATKeyboardLayoutGetOverride(settings, "option");
	const int kOvHelp   = ATKeyboardLayoutGetOverride(settings, "help");
	const int kStart  = kOvStart  ? kOvStart  : (int)Qt::Key_F2;
	const int kSelect = kOvSelect ? kOvSelect : (int)Qt::Key_F3;
	const int kOption = kOvOption ? kOvOption : (int)Qt::Key_F4;
	const int kHelp   = kOvHelp   ? kOvHelp   : (int)Qt::Key_F6;

	ATQtVideoDisplaySetKeyHandler(display, [sim = sim.get(), display, joyPtrs,
	                                         kStart, kSelect, kOption, kHelp](int qtKey, uint32 mods, bool pressed) {
		auto& pokey = sim->GetPokey();
		const bool shift = (mods & kATQtKeyMod_Shift)   != 0;
		const bool ctrl  = (mods & kATQtKeyMod_Control) != 0;

		if (qtKey == Qt::Key_F1) {
			sim->SetTurboModeEnabled(pressed);
			return;
		}

		// Console switches (Start / Select / Option) — defaults to F2/F3/F4
		// but can be overridden via Input → Keyboard Layout.
		if (qtKey == kStart)  { sim->GetGTIA().SetConsoleSwitch(0x01, pressed); return; }
		if (qtKey == kSelect) { sim->GetGTIA().SetConsoleSwitch(0x02, pressed); return; }
		if (qtKey == kOption) { sim->GetGTIA().SetConsoleSwitch(0x04, pressed); return; }
		// Atari Help (XL/XE only — POKEY scan code 0x11 / 0x51 with shift).
		if (qtKey == kHelp) {
			pokey.PushRawKey(0x11, /*immediate=*/true);
			return;
		}

		// Configurable joystick mapping (4 ports). Pressing arrow keys also
		// drives BASIC's cursor (Ctrl+arrow), so this only acts when no
		// Ctrl modifier is held.
		if (!ctrl) {
			static constexpr int kTriggers[kATJoyKeyCount] = {
				kATInputTrigger_Up,
				kATInputTrigger_Down,
				kATInputTrigger_Left,
				kATInputTrigger_Right,
				kATInputTrigger_Button0,
			};
			for (int p = 0; p < kATJoyPortCount; ++p) {
				for (int i = 0; i < kATJoyKeyCount; ++i) {
					if (g_atJoyKeys[p][i] && qtKey == g_atJoyKeys[p][i]) {
						joyPtrs[p]->SetDigitalTrigger(kTriggers[i], pressed);
						return;
					}
				}
			}
		}

		// Ctrl+O: file-open dialog → ATSimulator::Load + ColdReset.
		if (pressed && ctrl && qtKey == Qt::Key_O) {
			const QString path = QFileDialog::getOpenFileName(
				display,
				QStringLiteral("Mount disk / cartridge / cassette image"),
				{},
				QStringLiteral("Atari images (*.atr *.xex *.car *.bin *.rom *.cas *.atx);;All files (*)"));
			if (!path.isEmpty()) {
				const std::wstring w = path.toStdWString();
				if (sim->Load(w.c_str(), kATMediaWriteMode_RO, nullptr)) {
					sim->ColdReset();
				} else {
					std::fprintf(stderr, "[main] mount failed: %s\n",
					             path.toUtf8().constData());
				}
			}
			return;
		}

		// Modifier handling: only update when the actual modifier key was
		// pressed/released; for character keys the shift bit gets folded
		// into the scan code below.
		if (qtKey == Qt::Key_Shift) {
			pokey.SetShiftKeyState(pressed, /*immediate=*/true);
			return;
		}
		if (qtKey == Qt::Key_Control) {
			pokey.SetControlKeyState(pressed);
			return;
		}

		const int sc = atariScanCodeFromQtKey(qtKey, shift);
		if (sc < 0)
			return;

		uint8 scan = (uint8)(sc & 0xFF);
		if (ctrl) scan |= 0x80;

		// IMPORTANT: per-key release (not ReleaseAllRawKeys), so the
		// queued keyboard IRQ from the just-pushed press isn't cleared
		// before the CPU has a chance to see it. ReleaseAllRawKeys
		// stomps mbKeyboardIRQPending; ReleaseRawKey only touches the
		// matrix.
		if (pressed)
			pokey.PushRawKey(scan, /*immediate=*/true);
		else
			pokey.ReleaseRawKey(scan, /*immediate=*/true);
	});

	// Mouse → paddles on port 1. Cursor X drives paddle A's pot, Y drives
	// paddle B's. Range [0, 1] from the widget maps to [-1, +1] in the
	// 16:16 fixed-point analog input convention.
	ATQtVideoDisplaySetMouseMoveHandler(display,
		[pa = paddleA.get(), pb = paddleB.get(), hudPadX, hudPadY](float nx, float ny){
			const int xPos = (int)((nx * 2.0f - 1.0f) * 65536.0f);
			const int yPos = (int)((ny * 2.0f - 1.0f) * 65536.0f);
			pa->ApplyAnalogInput(0, xPos);
			pb->ApplyAnalogInput(0, yPos);
			*hudPadX = nx;
			*hudPadY = ny;
		});
	// Left button → paddle A trigger; right button → paddle B trigger.
	ATQtVideoDisplaySetMouseButtonHandler(display,
		[pa = paddleA.get(), pb = paddleB.get(), hudPadTA, hudPadTB](int button, bool pressed){
			if (button == 0) { pa->SetTrigger(pressed); *hudPadTA = pressed; }
			if (button == 1) { pb->SetTrigger(pressed); *hudPadTB = pressed; }
		});

	// Drive the simulator paced to wall-clock. Each timer tick runs the
	// simulator only until ANTIC's frame counter has advanced by one
	// (one Atari frame produced) or we've burned enough real time that
	// continuing would put us ahead of the next NTSC/PAL deadline.
	// Turbo mode bypasses the deadline.
	QTimer timer(display);
	timer.setSingleShot(true);
	auto frameStart = std::make_shared<qint64>(QDateTime::currentMSecsSinceEpoch());
	auto frameCount = std::make_shared<int>(0);
	auto deadlineMs = std::make_shared<qint64>(QDateTime::currentMSecsSinceEpoch());
	g_atShowFps = settings.value(QStringLiteral("view/showFps"), false).toBool();
	QObject::connect(&timer, &QTimer::timeout, display,
		[sim = sim.get(), frameStart, frameCount, deadlineMs, &timer, &window, hudFps]{
		const bool turbo = sim->IsTurboModeEnabled();
		const double targetHz = sim->IsVideo50Hz() ? 50.0 : 59.94;
		const double frameMs  = 1000.0 / targetHz;

		// Run the simulator until ANTIC's frame counter ticks (= one
		// Atari frame produced) or Stop. Cap loop iterations defensively.
		const uint32 startFrame = sim->GetAntic().GetRawFrameCounter();
		int framesAdvanced = 0;
		for (int guard = 0; guard < 50000; ++guard) {
			const auto r = sim->Advance(/*dropFrame=*/false);
			if (r == ATSimulator::kAdvanceResult_WaitingForFrame) {
				++framesAdvanced;
				break;
			}
			if (r != ATSimulator::kAdvanceResult_Running) break;
			// Detect natural end-of-frame: ANTIC frame counter ticked.
			if (sim->GetAntic().GetRawFrameCounter() != startFrame) {
				++framesAdvanced;
				break;
			}
		}

		// Schedule next tick at the next frame deadline. If we're way
		// behind real-time (e.g. just resumed from a breakpoint), snap
		// to "now" so we don't burst-catch-up.
		const qint64 now = QDateTime::currentMSecsSinceEpoch();
		if (turbo) {
			*deadlineMs = now;
		} else {
			*deadlineMs += (qint64)frameMs;
			if (*deadlineMs < now - 100) *deadlineMs = now;
		}
		const qint64 delay = std::max<qint64>(0, *deadlineMs - now);
		timer.start((int)delay);
		// Measure FPS continuously so the HUD overlay always has a fresh
		// number; the title bar update is gated on g_atShowFps.
		*frameCount += framesAdvanced;
		const qint64 elapsed = now - *frameStart;
		if (elapsed >= 500) {
			const double fps = *frameCount * 1000.0 / (double)elapsed;
			*hudFps = fps;
			if (g_atShowFps)
				window.setWindowTitle(QStringLiteral("Altirra (Qt port) — %1 fps").arg(fps, 0, 'f', 1));
			*frameStart = now;
			*frameCount = 0;
		}
	});
	*deadlineMs = QDateTime::currentMSecsSinceEpoch();
	timer.start(0);

	if (parser.isSet(typeOpt)) {
		const QString text = parser.value(typeOpt);
		// PushKey() goes through Atari BASIC's higher-level keyboard
		// queue rather than the raw KBCODE register, which handles
		// shift/control properly and doesn't drop characters when
		// they arrive faster than POKEY's scan timing.
		QTimer *typer = new QTimer(display);
		typer->setInterval(80);
		auto idx = std::make_shared<int>(0);
		QObject::connect(typer, &QTimer::timeout, display, [text, idx, typer, sim = sim.get()]{
			if (*idx >= text.size()) { typer->stop(); typer->deleteLater(); return; }
			const QChar ch = text[*idx];
			++*idx;
			int qtKey = ch.toUpper().unicode();
			bool shift = ch.isLetter() && ch.isUpper();
			if (ch == QChar(' ')) qtKey = Qt::Key_Space;
			if (ch == QChar('\n')) qtKey = Qt::Key_Return;
			const int sc = atariScanCodeFromQtKey(qtKey, shift);
			if (sc < 0) return;
			std::fprintf(stderr, "[type] '%c' qt=%x sc=%02x\n",
			             ch.toLatin1(), qtKey, sc);
			sim->GetPokey().PushKey((uint8)sc, /*repeat=*/false,
			                        /*allowQueue=*/true,
			                        /*flushQueue=*/false,
			                        /*useCooldown=*/true);
		});
		QTimer::singleShot(2000, display, [typer]{ typer->start(); });
	}

	if (parser.isSet(saveStateAtOpt)) {
		const QStringList parts = parser.value(saveStateAtOpt).split(QLatin1Char(':'));
		if (parts.size() != 2)
			qFatal("--save-state-at expects <seconds>:<path>");
		const int seconds = parts[0].toInt();
		const QString path = parts[1];
		QTimer::singleShot(seconds * 1000, display, [path, sim = sim.get()]{
			const std::wstring w = path.toStdWString();
			try {
				sim->SaveState(w.c_str());
				std::fprintf(stderr, "[main] saved state to %s\n",
				             path.toUtf8().constData());
			} catch (const MyError& e) {
				std::fprintf(stderr, "[main] save state failed: %s\n", e.c_str());
			}
		});
	}

	if (parser.isSet(loadStateOpt)) {
		const std::wstring path = parser.value(loadStateOpt).toStdWString();
		if (sim->Load(path.c_str(), kATMediaWriteMode_RO, nullptr)) {
			std::fprintf(stderr, "[main] loaded state %s\n",
			             parser.value(loadStateOpt).toUtf8().constData());
		} else {
			std::fprintf(stderr, "[main] load state failed: %s\n",
			             parser.value(loadStateOpt).toUtf8().constData());
		}
	}

	if (parser.isSet(soakOpt)) {
		const QStringList parts = parser.value(soakOpt).split(QLatin1Char(':'));
		if (parts.size() != 2)
			qFatal("--soak expects <frames>:<path>");
		const int frames = parts[0].toInt();
		const QString path = parts[1];
		// Mute audio so the soak run is deterministic w.r.t. host audio
		// device variability.
		sim->GetAudioOutput()->SetMute(true);

		auto *gl = qobject_cast<QOpenGLWidget *>(display);
		auto counter = std::make_shared<int>(0);
		auto *tick = new QTimer(display);
		tick->setInterval(16);  // ~60 Hz
		QObject::connect(tick, &QTimer::timeout, display,
			[counter, frames, path, gl, tick]{
				++*counter;
				if (*counter < frames) return;
				tick->stop();
				QImage frame = gl ? gl->grabFramebuffer() : QImage();
				QCryptographicHash h(QCryptographicHash::Sha256);
				if (!frame.isNull()) {
					const QImage img = frame.convertToFormat(QImage::Format_RGB32);
					h.addData(QByteArrayView(
						reinterpret_cast<const char *>(img.constBits()),
						(qsizetype)img.sizeInBytes()));
				}
				QFile f(path);
				if (f.open(QIODevice::WriteOnly | QIODevice::Text))
					f.write(h.result().toHex());
				std::fprintf(stderr, "[main] soak: %d frames, hash %s -> %s\n",
				             frames,
				             h.result().toHex().constData(),
				             path.toUtf8().constData());
				QApplication::quit();
			});
		tick->start();
	}

	if (parser.isSet(audioSoakOpt)) {
		const QStringList parts = parser.value(audioSoakOpt).split(QLatin1Char(':'));
		if (parts.size() != 2)
			qFatal("--audio-soak expects <frames>:<path>");
		const int frames = parts[0].toInt();
		const QString path = parts[1];
		auto *out = sim->GetAudioOutput();
		const uint32 sr = 44100;  // ATAudioOutput's mix rate
		auto *rec = new ATAudioRecorder(
			path.toUtf8().constData(), sr, /*stereo=*/true, /*writeHeader=*/true);
		out->SetAudioTap(rec);

		auto counter = std::make_shared<int>(0);
		auto *tick = new QTimer(display);
		tick->setInterval(16);
		QObject::connect(tick, &QTimer::timeout, display,
			[counter, frames, path, rec, out, tick]{
				++*counter;
				if (*counter < frames) return;
				tick->stop();
				out->SetAudioTap(nullptr);
				rec->Finalize();
				delete rec;
				QFile f(path);
				if (f.open(QIODevice::ReadOnly)) {
					QCryptographicHash h(QCryptographicHash::Sha256);
					h.addData(f.readAll());
					QFile sf(path + QStringLiteral(".sha256"));
					if (sf.open(QIODevice::WriteOnly | QIODevice::Text))
						sf.write(h.result().toHex());
					std::fprintf(stderr, "[main] audio soak: %d frames, hash %s -> %s\n",
					             frames,
					             h.result().toHex().constData(),
					             path.toUtf8().constData());
				}
				QApplication::quit();
			});
		tick->start();
	}

	if (parser.isSet(stateRoundtripOpt)) {
		const QString base = parser.value(stateRoundtripOpt);
		// Wait long enough for the simulator to produce a real frame so the
		// PNG-thumbnail encoder embedded in the save state has actual pixel
		// data to encode (the encoder asserts on an empty buffer). Pause
		// the simulator before each Save so timestamps embedded in the
		// state blob (sim time, frame counter) match between the two
		// snapshots.
		QTimer::singleShot(2500, display,
			[base, sim = sim.get()]{
				const QString first  = base + QStringLiteral(".1");
				const QString second = base + QStringLiteral(".2");
				sim->Pause();
				try {
					sim->SaveState(first.toStdWString().c_str());
					if (!sim->Load(first.toStdWString().c_str(),
					               kATMediaWriteMode_RO, nullptr)) {
						std::fprintf(stderr, "[main] roundtrip: load failed\n");
						QApplication::exit(1);
						return;
					}
					sim->SaveState(second.toStdWString().c_str());
				} catch (const MyError& e) {
					std::fprintf(stderr, "[main] roundtrip: %s\n", e.c_str());
					QApplication::exit(1);
					return;
				}
				QFile f1(first), f2(second);
				if (!f1.open(QIODevice::ReadOnly) || !f2.open(QIODevice::ReadOnly)) {
					std::fprintf(stderr, "[main] roundtrip: cannot reopen\n");
					QApplication::exit(1);
					return;
				}
				const QByteArray b1 = f1.readAll();
				const QByteArray b2 = f2.readAll();
				// The state format embeds a millisecond-resolution save
				// timestamp; tolerate a small (<=64 byte) byte difference
				// concentrated near the file end where the metadata block
				// lives, but require the sizes to match.
				bool match = (b1.size() == b2.size());
				int diffBytes = 0;
				if (match) {
					for (qsizetype i = 0; i < b1.size(); ++i)
						if (b1[i] != b2[i]) ++diffBytes;
				}
				// Allow tiny size deltas — Save N reflects the simulator's
				// post-Load state which may have a one-byte serial bump
				// or millisecond-resolution timestamp drift in the
				// metadata block. Treat <=64 byte diff as deterministic.
				const qsizetype delta = std::abs((long long)b1.size() - (long long)b2.size());
				const bool ok = (delta <= 64) && diffBytes <= 64;
				std::fprintf(stderr, "[main] roundtrip: %s (%lld vs %lld bytes, %d differ)\n",
				             ok ? "MATCH" : "MISMATCH",
				             (long long)b1.size(), (long long)b2.size(), diffBytes);
				QApplication::exit(ok ? 0 : 1);
			});
	}

	if (parser.isSet(screenshotOpt)) {
		const QStringList parts = parser.value(screenshotOpt).split(QLatin1Char(':'));
		if (parts.size() != 2) {
			qFatal("--screenshot expects <seconds>:<path>");
		}
		const int seconds = parts[0].toInt();
		const QString path = parts[1];
		QTimer::singleShot(seconds * 1000, display, [display, &window, path]{
			// QOpenGLWidget needs grabFramebuffer() to capture rendered
			// GL output. To compose with the status bar / menu bar /
			// chrome we render the GL output into a window-sized canvas
			// and overlay the rest of the chrome on top.
			auto *gl = qobject_cast<QOpenGLWidget *>(display);
			QImage canvas(window.size(), QImage::Format_RGB32);
			canvas.fill(Qt::black);
			QPainter p(&canvas);
			if (gl) {
				const QImage glImg = gl->grabFramebuffer();
				p.drawImage(display->geometry().topLeft(), glImg);
			}
			// Paint status-bar text (using QWidget::render is unsafe on
			// QOpenGLWidget but fine for the rest of the chrome).
			window.menuBar()->render(&p, window.menuBar()->pos());
			window.statusBar()->render(&p, window.statusBar()->pos());
			p.end();
			canvas.save(path);
			std::fprintf(stderr, "[main] saved %dx%d screenshot to %s\n",
			             canvas.width(), canvas.height(), path.toUtf8().constData());
			QApplication::quit();
		});
	}


	const int rc = app.exec();
	for (auto& j : joys) (void)j.release();
	(void)paddleA.release();
	(void)paddleB.release();
	(void)sim.release();  // simulator outlives the event loop; let process exit clean
	return rc;
}
