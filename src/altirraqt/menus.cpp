//	Altirra - Qt port
//	Top-level menu construction. Mirrors the layout of upstream
//	Altirra/res/menu_default.txt as closely as possible. Items whose
//	backend isn't ported yet are added but disabled, so the menu
//	hierarchy is identical to the original Windows build.

#include "menus.h"
#include "diskdrivesdialog.h"
#include "adjustcolorsdialog.h"
#include "advancedconfigdialog.h"
#include "tapecontroldialog.h"
#include "tapeeditordialog.h"
#include "cheaterdialog.h"
#include "firsttimewizard.h"
#include "lightpendialog.h"
#include "audiolevelsdialog.h"
#include "profileseditordialog.h"
#include "audiorecorder.h"
#include "videorecorder.h"
#include "sapwriter_qt.h"
#include "vgmwriter.h"
#include "calibratedialog.h"
#include "customizehuddialog.h"
#include "configsystemdialog.h"
#include "diskexplorerdialog.h"
#include "joystickkeys.h"
#include "joystickkeysdialog.h"
#include "lightpenrecalibrate.h"
#include "printerdialog.h"
#include "debuggerconsolepane.h"
#include "debuggerregisterspane.h"
#include "debuggerdisassemblypane.h"
#include "debuggermemorypane.h"
#include "debuggerwatchpane.h"
#include "debuggerbreakpointspane.h"
#include "debuggerhistorypane.h"

// Defined in main.cpp; flipped by the View → Show FPS toggle.
extern bool g_atShowFps;

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QGuiApplication>
#include <QImage>
#include <QInputDialog>
#include <QKeySequence>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>

#include <vector>

#include <vd2/VDDisplay/display.h>
#include <at/qtdisplay/qtvideodisplay.h>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name. Undef before including VDCPP.
#undef signals
#undef slots

#include <vd2/system/error.h>
#include <vd2/system/file.h>
#include <vd2/system/VDString.h>
#include <vd2/system/refcount.h>
#include <vd2/system/vdalloc.h>
#include <vd2/system/vectors.h>
#include <at/atio/cassetteimage.h>
#include <at/atcore/audiomixer.h>
#include <at/atcore/media.h>
#include <at/atio/cartridgeimage.h>
#include <cartridge.h>
#include <cassette.h>
#include <constants.h>
#include <cpu.h>
#include <debugger.h>
#include <disk.h>
#include <at/atcore/device.h>
#include <at/atcore/deviceport.h>
#include <at/atcore/enumparse.h>
#include "altvideodialog.h"
#include "compat_qt.h"
#include "compatdatabasedialog.h"
#include "debuggersourcepane.h"
#include "helpdialog.h"
#include "keyboardlayoutdialog.h"
#include "screentext.h"

#include <QClipboard>
#include <QFontDialog>
#include <QTimer>
#include <at/ataudio/pokey.h>

#include <antic.h>
#include <devicemanager.h>
#include <at/ataudio/audiooutput.h>
#include <diskinterface.h>
#include <gtia.h>
#include <inputcontroller.h>
#include <irqcontroller.h>
#include <sapconverter.h>
#include <simulator.h>

// Helper to mount + ColdReset and update the MRU. Exposed via menus.h
// so callers outside this file (drag/drop, --image) can reuse the path.
void ATBootImage(ATAltirraMenuContext& ctx, const QString& path) {
	if (path.isEmpty()) return;
	const std::wstring w = path.toStdWString();
	if (!ctx.sim->Load(w.c_str(), kATMediaWriteMode_RO, nullptr)) {
		QMessageBox::warning(ctx.window, QObject::tr("Mount failed"),
		                     QObject::tr("Could not load: %1").arg(path));
		return;
	}
	const QString compatTitle = ATCompatApplyAfterMount(*ctx.sim);

	// Auto-reload per-image saved symbols (debugger/symbols/<sha256>).
	if (ctx.settings) {
		auto trySha = [&](IATImage *img) {
			if (!img) return;
			auto sha = img->GetImageFileSHA256();
			if (!sha) return;
			QString key;
			key.reserve(64);
			static const char *hex = "0123456789abcdef";
			for (int i = 0; i < 32; ++i) {
				key.append(QChar::fromLatin1(hex[(sha->mDigest[i] >> 4) & 0xF]));
				key.append(QChar::fromLatin1(hex[sha->mDigest[i] & 0xF]));
			}
			const QString p = ctx.settings->value(
				QStringLiteral("debugger/symbols/") + key).toString();
			if (p.isEmpty()) return;
			IATDebugger *dbg = ATGetDebugger();
			if (!dbg) return;
			const std::wstring w = p.toStdWString();
			try {
				(void)dbg->LoadSymbols(w.c_str(), true, nullptr, true);
			} catch (const MyError&) {}
		};
		for (int i = 0; i < 2; ++i)
			if (auto *cart = ctx.sim->GetCartridge(i)) trySha(cart->GetImage());
		for (int i = 0; i < 8; ++i)
			trySha(ctx.sim->GetDiskInterface(i).GetDiskImage());
		if (IATCassetteImage *img = ctx.sim->GetCassette().GetImage())
			trySha(img);
	}

	ctx.sim->ColdReset();

	if (ctx.settings) {
		QStringList mru = ctx.settings->value(QStringLiteral("file/mruImages"))
		                                .toStringList();
		mru.removeAll(path);
		mru.prepend(path);
		while (mru.size() > 10) mru.removeLast();
		ctx.settings->setValue(QStringLiteral("file/mruImages"), mru);
	}

	const QString msg = compatTitle.isEmpty()
		? QObject::tr("Booted: %1").arg(QFileInfo(path).fileName())
		: QObject::tr("Booted: %1 — compat profile: %2")
		      .arg(QFileInfo(path).fileName(), compatTitle);
	ctx.window->statusBar()->showMessage(msg, 4000);
}

void ATApplyAllSettings(ATAltirraMenuContext& ctx) {
	if (!ctx.settings) return;
	auto& settings = *ctx.settings;
	auto *sim = ctx.sim;

	// Audio: volume, mute, mix levels.
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
	}

	// System: BASIC, hardware mode, memory, CPU mode, video standard,
	// power-on delay.
	if (settings.contains(QStringLiteral("system/basicEnabled")))
		sim->SetBASICEnabled(settings.value(QStringLiteral("system/basicEnabled")).toBool());
	if (settings.contains(QStringLiteral("system/hardwareMode"))) {
		sim->SetHardwareMode((ATHardwareMode)
			settings.value(QStringLiteral("system/hardwareMode")).toInt());
	}
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

	// View: overscan + vertical override.
	if (settings.contains(QStringLiteral("view/overscanMode")))
		sim->GetGTIA().SetOverscanMode((ATGTIAEmulator::OverscanMode)
			settings.value(QStringLiteral("view/overscanMode")).toInt());
	if (settings.contains(QStringLiteral("view/verticalOverscan")))
		sim->GetGTIA().SetVerticalOverscanMode((ATGTIAEmulator::VerticalOverscanMode)
			settings.value(QStringLiteral("view/verticalOverscan")).toInt());

	// Cassette decode toggles.
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

	// Display widget: filter mode, sharpness, scanlines, stretch,
	// indicator margin.
	if (ctx.display && settings.contains(QStringLiteral("view/filterMode")))
		ctx.display->SetFilterMode((IVDVideoDisplay::FilterMode)
			settings.value(QStringLiteral("view/filterMode")).toInt());
	if (ctx.display) {
		const int sharp = settings.value(QStringLiteral("view/filterSharpness"), 0).toInt();
		ctx.display->SetPixelSharpness(sharp * 0.5f, sharp * 0.5f);
	}
	if (ctx.displayWidget) {
		const int scanIdx = settings.value(QStringLiteral("view/scanlines"), 0).toInt();
		static constexpr float kScanStrength[4] = { 0.00f, 0.20f, 0.45f, 0.70f };
		const int s = (scanIdx >= 0 && scanIdx < 4) ? scanIdx : 0;
		ATQtVideoDisplaySetScanlineIntensity(ctx.displayWidget, kScanStrength[s]);

		const int stretch = settings.value(QStringLiteral("view/stretchMode"),
			(int)ATQtStretchMode::PreserveAspect).toInt();
		ATQtVideoDisplaySetStretchMode(ctx.displayWidget, (ATQtStretchMode)stretch);

		const bool indMargin = settings.value(QStringLiteral("view/indicatorMargin"), false).toBool();
		ATQtVideoDisplaySetIndicatorMargin(ctx.displayWidget, indMargin ? 24 : 0);

		const bool sharpBilin = settings.value(QStringLiteral("view/sharpBilinear"), false).toBool();
		ATQtVideoDisplaySetSharpBilinear(ctx.displayWidget, sharpBilin);

		// Re-apply the last screen-FX shader (preset or custom file).
		const QString preset = settings.value(QStringLiteral("view/effectPreset")).toString();
		const QString customPath = settings.value(QStringLiteral("view/customShader")).toString();
		if (!preset.isEmpty()) {
			static const struct { const char *name; const char *src; } kBuiltins[] = {
				{"CRT",       /* CRT */
R"GLSL(#version 330 core
in vec2 v_uv; out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel; uniform vec2 u_sharpness;
uniform float u_scanline; uniform float u_tex_height; uniform float u_sharp_bilin;
void main(){
    vec4 c=texture(u_tex,v_uv);
    float row=v_uv.y*u_tex_height; float gap=abs(2.0*fract(row)-1.0);
    c.rgb*=1.0-0.6*gap;
    float colCycle=mod(gl_FragCoord.x,3.0);
    vec3 mask=vec3(colCycle<1.0?1.1:0.85, (colCycle<2.0&&colCycle>=1.0)?1.1:0.85, colCycle>=2.0?1.1:0.85);
    c.rgb*=mask; out_color=vec4(clamp(c.rgb,0.0,1.0),1.0);
})GLSL"},
				{"Phosphor",
R"GLSL(#version 330 core
in vec2 v_uv; out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel; uniform vec2 u_sharpness;
uniform float u_scanline; uniform float u_tex_height; uniform float u_sharp_bilin;
void main(){ vec4 c=texture(u_tex,v_uv);
    vec4 c1=texture(u_tex,v_uv+vec2(u_texel.x,0.0));
    vec4 c2=texture(u_tex,v_uv-vec2(u_texel.x,0.0));
    out_color=vec4(clamp((0.5*c+0.25*(c1+c2)).rgb,0.0,1.0),1.0);
})GLSL"},
				{"Composite",
R"GLSL(#version 330 core
in vec2 v_uv; out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel; uniform vec2 u_sharpness;
uniform float u_scanline; uniform float u_tex_height; uniform float u_sharp_bilin;
void main(){ vec4 c0=texture(u_tex,v_uv);
    vec4 cl=texture(u_tex,v_uv-vec2(u_texel.x,0.0));
    vec4 cr=texture(u_tex,v_uv+vec2(u_texel.x,0.0));
    float Y=dot(c0.rgb,vec3(0.299,0.587,0.114));
    vec3 chroma=0.5*((cl.rgb-dot(cl.rgb,vec3(0.299,0.587,0.114)))+(cr.rgb-dot(cr.rgb,vec3(0.299,0.587,0.114))));
    out_color=vec4(clamp(vec3(Y)+chroma,0.0,1.0),1.0);
})GLSL"},
				{"Scanline+",
R"GLSL(#version 330 core
in vec2 v_uv; out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel; uniform vec2 u_sharpness;
uniform float u_scanline; uniform float u_tex_height; uniform float u_sharp_bilin;
void main(){ vec4 c=texture(u_tex,v_uv);
    float row=v_uv.y*u_tex_height; float gap=abs(2.0*fract(row)-1.0);
    c.rgb*=1.0-0.85*gap;
    float l=dot(c.rgb,vec3(0.3,0.59,0.11));
    if(l>0.6){
        vec4 nl=texture(u_tex,v_uv-vec2(u_texel.x,0.0));
        vec4 nr=texture(u_tex,v_uv+vec2(u_texel.x,0.0));
        c.rgb=mix(c.rgb,c.rgb+0.4*(nl.rgb+nr.rgb),0.4);
    }
    out_color=vec4(clamp(c.rgb,0.0,1.0),1.0);
})GLSL"},
			};
			for (const auto& b : kBuiltins) {
				if (preset == QString::fromUtf8(b.name)) {
					ATQtVideoDisplaySetCustomShader(ctx.displayWidget, b.src);
					break;
				}
			}
		} else if (!customPath.isEmpty()) {
			QFile f(customPath);
			if (f.open(QIODevice::ReadOnly)) {
				const QByteArray src = f.readAll();
				ATQtVideoDisplaySetCustomShader(ctx.displayWidget, src.constData());
			}
		} else {
			ATQtVideoDisplaySetCustomShader(ctx.displayWidget, "");
		}
	}

	// Display analysis output (View → Output).
	{
		using GA = ATGTIAEmulator::AnalysisMode;
		using AA = ATAnticEmulator::AnalysisMode;
		const int idx = settings.value(QStringLiteral("view/outputIndex"), 0).toInt();
		sim->GetAntic().SetAnalysisMode(idx == 1 ? AA::kAnalyzeDMATiming : AA::kAnalyzeOff);
		GA gm = GA::kAnalyzeNone;
		if (idx == 2) gm = GA::kAnalyzeLayers;
		if (idx == 3) gm = GA::kAnalyzeColors;
		if (idx == 4) gm = GA::kAnalyzeDList;
		sim->GetGTIA().SetAnalysisMode(gm);
	}

	// Joystick keys reload.
	ATLoadJoyKeys(settings);

	// Reload ROMs in case hardware mode changed.
	sim->LoadROMs();
}

bool ATApplyProfile(QSettings& settings, const QString& name) {
	if (name.isEmpty()) return false;
	const QString prefix = QStringLiteral("profiles/") + name + QStringLiteral("/");
	settings.sync();
	const QStringList keys = settings.allKeys();
	bool found = false;
	for (const QString& k : keys) {
		if (!k.startsWith(prefix)) continue;
		const QString sub = k.mid(prefix.length());
		settings.setValue(sub, settings.value(k));
		found = true;
	}
	if (found)
		settings.setValue(QStringLiteral("system/lastProfile"), name);
	return found;
}

namespace {

void buildFileMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *fileMenu = bar->addMenu(QObject::tr("&File"));

	auto *bootAction = fileMenu->addAction(QObject::tr("&Boot Image..."));
	bootAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
	QObject::connect(bootAction, &QAction::triggered, ctx.window, [&ctx]{
		const QString path = QFileDialog::getOpenFileName(
			ctx.window,
			QObject::tr("Boot disk / cartridge / cassette image"),
			{},
			QObject::tr("Atari images (*.atr *.xex *.car *.bin *.rom *.cas *.atx);;All files (*)"));
		ATBootImage(ctx, path);
	});

	// Recently Booted submenu — pulled from QSettings on each menu open.
	auto *mruMenu = fileMenu->addMenu(QObject::tr("&Recently Booted"));
	if (ctx.settings) {
		QObject::connect(mruMenu, &QMenu::aboutToShow, [mruMenu, &ctx]{
			mruMenu->clear();
			const QStringList mru = ctx.settings->value(QStringLiteral("file/mruImages"))
			                                     .toStringList();
			if (mru.isEmpty()) {
				auto *act = mruMenu->addAction(QObject::tr("(no recently booted images)"));
				act->setEnabled(false);
				return;
			}
			for (const QString& path : mru) {
				auto *act = mruMenu->addAction(path);
				QObject::connect(act, &QAction::triggered, [&ctx, path]{
					ATBootImage(ctx, path);
				});
			}
		});
	} else {
		mruMenu->menuAction()->setEnabled(false);
	}

	fileMenu->addSeparator();

	auto *openAction = fileMenu->addAction(QObject::tr("&Open Image..."));
	openAction->setShortcut(QKeySequence::Open);
	QObject::connect(openAction, &QAction::triggered, ctx.window, [&ctx]{
		const QString path = QFileDialog::getOpenFileName(
			ctx.window,
			QObject::tr("Mount disk / cartridge / cassette image"),
			{},
			QObject::tr("Atari images (*.atr *.xex *.car *.bin *.rom *.cas *.atx);;All files (*)"));
		if (path.isEmpty()) return;
		const std::wstring w = path.toStdWString();
		if (ctx.sim->Load(w.c_str(), kATMediaWriteMode_RO, nullptr)) {
			ctx.sim->ColdReset();
		} else {
			QMessageBox::warning(ctx.window, QObject::tr("Mount failed"),
			                     QObject::tr("Could not load: %1").arg(path));
		}
	});

	auto *diskDrivesAction = fileMenu->addAction(QObject::tr("Dis&k Drives..."));
	QObject::connect(diskDrivesAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDiskDrivesDialog(ctx.window, ctx.sim);
	});

	// Disk options submenu — checkable shortcuts that mirror the Advanced
	// Configuration toggles. Each writes the same QSettings key the
	// Advanced dialog persists.
	{
		auto *opts = fileMenu->addMenu(QObject::tr("Disk &Options"));

		auto *driveSounds = opts->addAction(QObject::tr("Drive &Sounds"));
		driveSounds->setCheckable(true);
		bool ds = false;
		for (int i = 0; i < 8; ++i)
			ds = ds || ctx.sim->GetDiskInterface(i).AreDriveSoundsEnabled();
		driveSounds->setChecked(ds);
		QObject::connect(driveSounds, &QAction::toggled,
			[sim = ctx.sim, &ctx](bool on){
				for (int i = 0; i < 8; ++i)
					sim->GetDiskInterface(i).SetDriveSoundsEnabled(on);
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("disk/driveSounds"), on);
			});

		auto *sectorCounter = opts->addAction(QObject::tr("Sector &Counter"));
		sectorCounter->setCheckable(true);
		sectorCounter->setChecked(ctx.sim->IsDiskSectorCounterEnabled());
		QObject::connect(sectorCounter, &QAction::toggled,
			[sim = ctx.sim, &ctx](bool on){
				sim->SetDiskSectorCounterEnabled(on);
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("disk/sectorCounter"), on);
			});

		auto *accTiming = opts->addAction(QObject::tr("&Accurate Sector Timing"));
		accTiming->setCheckable(true);
		accTiming->setChecked(ctx.sim->IsDiskAccurateTimingEnabled());
		QObject::connect(accTiming, &QAction::toggled,
			[sim = ctx.sim, &ctx](bool on){
				sim->SetDiskAccurateTimingEnabled(on);
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("disk/accurateTiming"), on);
			});

		auto *sioPatch = opts->addAction(QObject::tr("SIO &Patch"));
		sioPatch->setCheckable(true);
		sioPatch->setChecked(ctx.sim->IsDiskSIOPatchEnabled());
		QObject::connect(sioPatch, &QAction::toggled,
			[sim = ctx.sim, &ctx](bool on){
				sim->SetDiskSIOPatchEnabled(on);
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("disk/sioPatch"), on);
			});
	}

	// Attach Disk submenu — Drive 1..8.
	auto *attachMenu = fileMenu->addMenu(QObject::tr("Attach Disk"));

	// Find highest in-use drive index, then ask the simulator to cycle the
	// loaded images one slot up or down.
	auto rotateDrives = [sim = ctx.sim](int delta){
		int active = 0;
		for (int i = 14; i >= 0; --i) {
			if (sim->GetDiskDrive(i).IsEnabled()
			    || sim->GetDiskInterface(i).GetClientCount() > 1) {
				active = i + 1;
				break;
			}
		}
		if (active > 0) sim->RotateDrives(active, delta);
	};

	auto *rotateDownAction = attachMenu->addAction(QObject::tr("Rotate Down"));
	QObject::connect(rotateDownAction, &QAction::triggered, [rotateDrives]{ rotateDrives(-1); });
	auto *rotateUpAction = attachMenu->addAction(QObject::tr("Rotate Up"));
	QObject::connect(rotateUpAction, &QAction::triggered, [rotateDrives]{ rotateDrives(+1); });
	attachMenu->addSeparator();
	for (int i = 0; i < 8; ++i) {
		auto *act = attachMenu->addAction(QObject::tr("Drive %1").arg(i + 1));
		QObject::connect(act, &QAction::triggered, ctx.window, [&ctx, i]{
			const QString path = QFileDialog::getOpenFileName(
				ctx.window,
				QObject::tr("Attach disk image to drive %1").arg(i + 1),
				{},
				QObject::tr("Atari disk images (*.atr *.atx *.xfd);;All files (*)"));
			if (path.isEmpty()) return;
			const std::wstring w = path.toStdWString();
			try {
				ctx.sim->GetDiskInterface(i).LoadDisk(w.c_str());
			} catch (const MyError& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Attach failed"),
				                     QObject::tr("Drive %1: %2").arg(i + 1).arg(QString::fromUtf8(e.c_str())));
				return;
			}
			ctx.window->statusBar()->showMessage(
				QObject::tr("Mounted in D%1: %2").arg(i + 1).arg(QFileInfo(path).fileName()), 2500);
		});
	}

	// Detach Disk submenu — Drive 1..8.
	auto *detachMenu = fileMenu->addMenu(QObject::tr("Detach Disk"));
	auto *detachAll = detachMenu->addAction(QObject::tr("All"));
	QObject::connect(detachAll, &QAction::triggered, [&ctx]{
		for (int i = 0; i < 8; ++i)
			ctx.sim->GetDiskInterface(i).UnloadDisk();
		ctx.window->statusBar()->showMessage(
			QObject::tr("All disks detached"), 2500);
	});
	detachMenu->addSeparator();
	for (int i = 0; i < 8; ++i) {
		auto *act = detachMenu->addAction(QObject::tr("Drive %1").arg(i + 1));
		QObject::connect(act, &QAction::triggered, [&ctx, i]{
			ctx.sim->GetDiskInterface(i).UnloadDisk();
			ctx.window->statusBar()->showMessage(
				QObject::tr("Detached D%1:").arg(i + 1), 2500);
		});
	}

	fileMenu->addSeparator();

	auto *cassetteMenu = fileMenu->addMenu(QObject::tr("Cassette"));
	auto *tapeCtrlAction = cassetteMenu->addAction(QObject::tr("Tape Control..."));
	QObject::connect(tapeCtrlAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowTapeControlDialog(ctx.window, ctx.sim);
	});
	auto *tapeEditAction = cassetteMenu->addAction(QObject::tr("Tape Editor..."));
	QObject::connect(tapeEditAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowTapeEditorDialog(ctx.window, ctx.sim);
	});
	cassetteMenu->addSeparator();

	auto *casNewAction = cassetteMenu->addAction(QObject::tr("New Tape"));
	QObject::connect(casNewAction, &QAction::triggered, [&ctx]{
		ctx.sim->GetCassette().LoadNew();
		ctx.window->statusBar()->showMessage(
			QObject::tr("New blank tape created"), 2500);
	});

	auto *casLoadAction = cassetteMenu->addAction(QObject::tr("&Load..."));
	QObject::connect(casLoadAction, &QAction::triggered, ctx.window, [&ctx]{
		const QString path = QFileDialog::getOpenFileName(
			ctx.window,
			QObject::tr("Load cassette image"),
			{},
			QObject::tr("Cassette images (*.cas *.wav);;All files (*)"));
		if (path.isEmpty()) return;
		const std::wstring w = path.toStdWString();
		try {
			ctx.sim->GetCassette().Load(w.c_str());
		} catch (const MyError& e) {
			QMessageBox::warning(ctx.window, QObject::tr("Load failed"),
			                     QString::fromUtf8(e.c_str()));
			return;
		}
		ctx.window->statusBar()->showMessage(
			QObject::tr("Cassette loaded: %1").arg(QFileInfo(path).fileName()), 2500);
	});

	auto *casUnloadAction = cassetteMenu->addAction(QObject::tr("&Unload"));
	QObject::connect(casUnloadAction, &QAction::triggered, [&ctx]{
		ctx.sim->GetCassette().Unload();
		ctx.window->statusBar()->showMessage(
			QObject::tr("Cassette unloaded"), 2500);
	});

	auto *casSaveAction = cassetteMenu->addAction(QObject::tr("Save..."));
	QObject::connect(casSaveAction, &QAction::triggered, ctx.window, [&ctx]{
		auto *image = ctx.sim->GetCassette().GetImage();
		if (!image) {
			QMessageBox::information(ctx.window, QObject::tr("Save"),
				QObject::tr("No cassette is loaded."));
			return;
		}
		const QString path = QFileDialog::getSaveFileName(ctx.window,
			QObject::tr("Save cassette image"),
			QStringLiteral("tape.cas"),
			QObject::tr("CAS file (*.cas);;WAV file (*.wav)"));
		if (path.isEmpty()) return;
		try {
			VDFileStream fs(path.toStdWString().c_str(),
				nsVDFile::kWrite | nsVDFile::kDenyAll | nsVDFile::kCreateAlways);
			if (path.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive))
				ATSaveCassetteImageWAV(fs, image);
			else
				ATSaveCassetteImageCAS(fs, image);
		} catch (const MyError& e) {
			QMessageBox::warning(ctx.window, QObject::tr("Save failed"),
				QString::fromUtf8(e.c_str()));
			return;
		}
		ctx.window->statusBar()->showMessage(
			QObject::tr("Cassette saved: %1").arg(QFileInfo(path).fileName()), 2500);
	});

	auto *exportTapeAction = cassetteMenu->addAction(QObject::tr("Export Audio Tape..."));
	QObject::connect(exportTapeAction, &QAction::triggered, ctx.window, [&ctx]{
		auto *image = ctx.sim->GetCassette().GetImage();
		if (!image) {
			QMessageBox::information(ctx.window, QObject::tr("Export Audio Tape"),
				QObject::tr("No cassette is loaded."));
			return;
		}
		const QString path = QFileDialog::getSaveFileName(ctx.window,
			QObject::tr("Export audio tape"),
			QStringLiteral("tape.wav"),
			QObject::tr("WAV file (*.wav)"));
		if (path.isEmpty()) return;
		try {
			VDFileStream fs(path.toStdWString().c_str(),
				nsVDFile::kWrite | nsVDFile::kDenyAll | nsVDFile::kCreateAlways);
			ATSaveCassetteImageWAV(fs, image);
		} catch (const MyError& e) {
			QMessageBox::warning(ctx.window, QObject::tr("Export failed"),
				QString::fromUtf8(e.c_str()));
			return;
		}
		ctx.window->statusBar()->showMessage(
			QObject::tr("Audio tape exported: %1").arg(QFileInfo(path).fileName()), 2500);
	});

	cassetteMenu->addSeparator();

	// Decode-side toggles. These are the hand-tunable knobs that affect
	// how reliably real-world tapes load — exposing them lets a user
	// recover marginal images without touching the source.
	auto& cas = ctx.sim->GetCassette();
	auto *fskAction = cassetteMenu->addAction(QObject::tr("FSK Speed Compensation"));
	fskAction->setCheckable(true);
	fskAction->setChecked(cas.GetFSKSpeedCompensationEnabled());
	QObject::connect(fskAction, &QAction::toggled, [sim = ctx.sim, &ctx](bool on){
		sim->GetCassette().SetFSKSpeedCompensationEnabled(on);
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("cassette/fsk"), on);
	});

	auto *crossAction = cassetteMenu->addAction(QObject::tr("Crosstalk Reduction"));
	crossAction->setCheckable(true);
	crossAction->setChecked(cas.GetCrosstalkReductionEnabled());
	QObject::connect(crossAction, &QAction::toggled, [sim = ctx.sim, &ctx](bool on){
		sim->GetCassette().SetCrosstalkReductionEnabled(on);
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("cassette/crosstalk"), on);
	});

	auto *dataAsAudioAction = cassetteMenu->addAction(QObject::tr("Load Data As Audio"));
	dataAsAudioAction->setCheckable(true);
	dataAsAudioAction->setChecked(cas.IsLoadDataAsAudioEnabled());
	QObject::connect(dataAsAudioAction, &QAction::toggled, [sim = ctx.sim, &ctx](bool on){
		sim->GetCassette().SetLoadDataAsAudioEnable(on);
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("cassette/dataAsAudio"), on);
	});

	auto *autoRewindAction = cassetteMenu->addAction(QObject::tr("Auto-Rewind on Cold Reset"));
	autoRewindAction->setCheckable(true);
	autoRewindAction->setChecked(cas.IsAutoRewindEnabled());
	QObject::connect(autoRewindAction, &QAction::toggled, [sim = ctx.sim, &ctx](bool on){
		sim->GetCassette().SetAutoRewindEnabled(on);
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("cassette/autoRewind"), on);
	});

	fileMenu->addSeparator();

	auto *loadStateAction = fileMenu->addAction(QObject::tr("Load State..."));
	loadStateAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F7));
	QObject::connect(loadStateAction, &QAction::triggered, ctx.window, [&ctx]{
		const QString path = QFileDialog::getOpenFileName(
			ctx.window,
			QObject::tr("Load save state"),
			{},
			QObject::tr("Altirra save state (*.atstate2);;All files (*)"));
		if (path.isEmpty()) return;
		const std::wstring w = path.toStdWString();
		if (!ctx.sim->Load(w.c_str(), kATMediaWriteMode_RO, nullptr)) {
			QMessageBox::warning(ctx.window, QObject::tr("Load failed"),
			                     QObject::tr("Could not load save state: %1").arg(path));
			return;
		}
		ctx.window->statusBar()->showMessage(
			QObject::tr("Loaded state: %1").arg(QFileInfo(path).fileName()), 2500);
	});

	auto *saveStateAction = fileMenu->addAction(QObject::tr("Save State..."));
	saveStateAction->setShortcut(QKeySequence(Qt::Key_F7));
	QObject::connect(saveStateAction, &QAction::triggered, ctx.window, [&ctx]{
		const QString path = QFileDialog::getSaveFileName(
			ctx.window,
			QObject::tr("Save save state"),
			QStringLiteral("altirra-state.atstate2"),
			QObject::tr("Altirra save state (*.atstate2);;All files (*)"));
		if (path.isEmpty()) return;
		const std::wstring w = path.toStdWString();
		try {
			ctx.sim->SaveState(w.c_str());
		} catch (const MyError& e) {
			QMessageBox::warning(ctx.window, QObject::tr("Save failed"),
			                     QObject::tr("Could not save state: %1").arg(QString::fromUtf8(e.c_str())));
			return;
		}
		ctx.window->statusBar()->showMessage(
			QObject::tr("Saved state: %1").arg(QFileInfo(path).fileName()), 2500);
	});

	// Quick Save / Quick Load with 9 slots. Slot N is stored at
	// ~/.config/altirraqt-quickstate-N.atstate2. Slot 1 keeps the legacy
	// F11 / Shift+F11 shortcuts so users have a single-stroke save+load.
	auto quickStatePath = [](int slot) -> QString {
		return QString::fromUtf8(qgetenv("HOME"))
			+ QStringLiteral("/.config/altirraqt-quickstate-%1.atstate2").arg(slot);
	};

	auto *quickLoadMenu = fileMenu->addMenu(QObject::tr("Quick Load State"));
	for (int n = 1; n <= 9; ++n) {
		auto *act = quickLoadMenu->addAction(QObject::tr("Slot %1").arg(n));
		if (n == 1) act->setShortcut(QKeySequence(Qt::Key_F11 | Qt::SHIFT));
		QObject::connect(act, &QAction::triggered, ctx.window,
			[&ctx, quickStatePath, n]{
			const QString path = quickStatePath(n);
			const std::wstring w = path.toStdWString();
			if (!ctx.sim->Load(w.c_str(), kATMediaWriteMode_RO, nullptr)) {
				QMessageBox::warning(ctx.window, QObject::tr("Quick Load failed"),
					QObject::tr("Slot %1 is empty — save to it first.").arg(n));
				return;
			}
			ctx.window->statusBar()->showMessage(
				QObject::tr("Quick loaded slot %1").arg(n), 2000);
		});
	}

	auto *quickSaveMenu = fileMenu->addMenu(QObject::tr("Quick Save State"));
	for (int n = 1; n <= 9; ++n) {
		auto *act = quickSaveMenu->addAction(QObject::tr("Slot %1").arg(n));
		if (n == 1) act->setShortcut(QKeySequence(Qt::Key_F11));
		QObject::connect(act, &QAction::triggered, ctx.window,
			[&ctx, quickStatePath, n]{
			const QString path = quickStatePath(n);
			const std::wstring w = path.toStdWString();
			try {
				ctx.sim->SaveState(w.c_str());
			} catch (const MyError& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Quick Save failed"),
					QString::fromUtf8(e.c_str()));
				return;
			}
			ctx.window->statusBar()->showMessage(
				QObject::tr("Quick saved to slot %1").arg(n), 2000);
		});
	}

	fileMenu->addSeparator();

	// Attach Special Cartridge — synthesizes an empty image of a specific
	// cart type and ColdResets, mirroring upstream's File → Attach Special
	// Cartridge submenu (menu_default.txt). Each entry maps to an
	// ATCartridgeMode that the simulator can instantiate via
	// LoadNewCartridge() (or LoadCartridgeBASIC() for the BASIC variant).
	// Attach Special Cartridge — every cartridge mode the simulator's
	// LoadNewCartridge() accepts, grouped by family. Selecting an entry
	// creates an empty cartridge of that mapper and ColdResets.
	auto *specialMenu = fileMenu->addMenu(QObject::tr("Attach Special Cartridge"));
	{
		struct CartEntry { const char *label; ATCartridgeMode mode; };
		struct CartFamily {
			const char *submenu;
			std::initializer_list<CartEntry> entries;
		};
		auto addEntry = [&ctx](QMenu *parent, const CartEntry& e) {
			auto *act = parent->addAction(QObject::tr(e.label));
			QObject::connect(act, &QAction::triggered, ctx.window,
				[&ctx, e]{
					try {
						ctx.sim->LoadNewCartridge((int)e.mode);
						ctx.sim->ColdReset();
						if (ctx.window)
							ctx.window->statusBar()->showMessage(
								QObject::tr("Attached %1").arg(QObject::tr(e.label)),
								3000);
					} catch (const MyError& err) {
						QMessageBox::warning(ctx.window,
							QObject::tr("Attach failed"),
							QString::fromUtf8(err.c_str()));
					}
				});
		};

		const CartFamily kFamilies[] = {
			{"&Standard sizes", {
				{"2K",  kATCartridgeMode_2K},
				{"4K",  kATCartridgeMode_4K},
				{"8K",  kATCartridgeMode_8K},
				{"16K", kATCartridgeMode_16K},
				{"Right slot 4K", kATCartridgeMode_RightSlot_4K},
				{"Right slot 8K", kATCartridgeMode_RightSlot_8K},
			}},
			{"5&200", {
				{"5200 4K",  kATCartridgeMode_5200_4K},
				{"5200 8K",  kATCartridgeMode_5200_8K},
				{"5200 16K (one chip)", kATCartridgeMode_5200_16K_OneChip},
				{"5200 16K (two chip)", kATCartridgeMode_5200_16K_TwoChip},
				{"5200 32K", kATCartridgeMode_5200_32K},
				{"5200 64K (32K banks)",  kATCartridgeMode_5200_64K_32KBanks},
				{"5200 128K (32K banks)", kATCartridgeMode_5200_128K_32KBanks},
				{"5200 256K (32K banks)", kATCartridgeMode_5200_256K_32KBanks},
				{"5200 512K (32K banks)", kATCartridgeMode_5200_512K_32KBanks},
				{"5200 Bounty Bob",       kATCartridgeMode_BountyBob5200},
				{"5200 Bounty Bob (alt)", kATCartridgeMode_BountyBob5200Alt},
			}},
			{"&XEGS", {
				{"XEGS 32K",   kATCartridgeMode_XEGS_32K},
				{"XEGS 64K",   kATCartridgeMode_XEGS_64K},
				{"XEGS 64K alt", kATCartridgeMode_XEGS_64K_Alt},
				{"XEGS 128K",  kATCartridgeMode_XEGS_128K},
				{"XEGS 256K",  kATCartridgeMode_XEGS_256K},
				{"XEGS 512K",  kATCartridgeMode_XEGS_512K},
				{"XEGS 1M",    kATCartridgeMode_XEGS_1M},
				{"Switchable XEGS 32K",  kATCartridgeMode_Switchable_XEGS_32K},
				{"Switchable XEGS 64K",  kATCartridgeMode_Switchable_XEGS_64K},
				{"Switchable XEGS 128K", kATCartridgeMode_Switchable_XEGS_128K},
				{"Switchable XEGS 256K", kATCartridgeMode_Switchable_XEGS_256K},
				{"Switchable XEGS 512K", kATCartridgeMode_Switchable_XEGS_512K},
				{"Switchable XEGS 1M",   kATCartridgeMode_Switchable_XEGS_1M},
				{"XE Multicart 8K",   kATCartridgeMode_XEMulticart_8K},
				{"XE Multicart 16K",  kATCartridgeMode_XEMulticart_16K},
				{"XE Multicart 32K",  kATCartridgeMode_XEMulticart_32K},
				{"XE Multicart 64K",  kATCartridgeMode_XEMulticart_64K},
				{"XE Multicart 128K", kATCartridgeMode_XEMulticart_128K},
				{"XE Multicart 256K", kATCartridgeMode_XEMulticart_256K},
				{"XE Multicart 512K", kATCartridgeMode_XEMulticart_512K},
				{"XE Multicart 1M",   kATCartridgeMode_XEMulticart_1M},
			}},
			{"&MaxFlash", {
				{"MaxFlash 128K (1Mbit)",                   kATCartridgeMode_MaxFlash_128K},
				{"MaxFlash 128K (1Mbit, MyIDE banking)",    kATCartridgeMode_MaxFlash_128K_MyIDE},
				{"MaxFlash 1M (8Mbit, older - bank 127)",   kATCartridgeMode_MaxFlash_1024K},
				{"MaxFlash 1M (8Mbit, newer - bank 0)",     kATCartridgeMode_MaxFlash_1024K_Bank0},
			}},
			{"M&egaCart", {
				{"MegaCart 16K",    kATCartridgeMode_MegaCart_16K},
				{"MegaCart 32K",    kATCartridgeMode_MegaCart_32K},
				{"MegaCart 64K",    kATCartridgeMode_MegaCart_64K},
				{"MegaCart 128K",   kATCartridgeMode_MegaCart_128K},
				{"MegaCart 256K",   kATCartridgeMode_MegaCart_256K},
				{"MegaCart 512K",   kATCartridgeMode_MegaCart_512K},
				{"MegaCart 512K v3 (flash)", kATCartridgeMode_MegaCart_512K_3},
				{"MegaCart 1M",     kATCartridgeMode_MegaCart_1M},
				{"MegaCart 1M v2",  kATCartridgeMode_MegaCart_1M_2},
				{"MegaCart 2M",     kATCartridgeMode_MegaCart_2M},
				{"MegaCart 4M v3 (flash)", kATCartridgeMode_MegaCart_4M_3},
				{"MegaMax 2M",      kATCartridgeMode_MegaMax_2M},
			}},
			{"&OSS / SDX", {
				{"OSS 8K",       kATCartridgeMode_OSS_8K},
				{"OSS 034M",     kATCartridgeMode_OSS_034M},
				{"OSS 043M",     kATCartridgeMode_OSS_043M},
				{"OSS M091",     kATCartridgeMode_OSS_M091},
				{"SpartaDOS X 64K",  kATCartridgeMode_SpartaDosX_64K},
				{"SpartaDOS X 128K", kATCartridgeMode_SpartaDosX_128K},
				{"Atrax SDX 64K",  kATCartridgeMode_Atrax_SDX_64K},
				{"Atrax SDX 128K", kATCartridgeMode_Atrax_SDX_128K},
			}},
			{"S&IC! / J!Cart / DCart", {
				{"SIC! 128K flash", kATCartridgeMode_SIC_128K},
				{"SIC! 256K flash", kATCartridgeMode_SIC_256K},
				{"SIC! 512K flash", kATCartridgeMode_SIC_512K},
				{"SIC+ flash",      kATCartridgeMode_SICPlus},
				{"J!Cart 128K", kATCartridgeMode_JAtariCart_8K},
				{"J!Cart 16K",  kATCartridgeMode_JAtariCart_16K},
				{"J!Cart 32K",  kATCartridgeMode_JAtariCart_32K},
				{"J!Cart 64K",  kATCartridgeMode_JAtariCart_64K},
				{"J!Cart 128K-2", kATCartridgeMode_JAtariCart_128K},
				{"J!Cart 256K", kATCartridgeMode_JAtariCart_256K},
				{"J!Cart 512K", kATCartridgeMode_JAtariCart_512K},
				{"J!Cart 1024K", kATCartridgeMode_JAtariCart_1024K},
				{"DCart",       kATCartridgeMode_DCart},
				{"JRC 6 64K",   kATCartridgeMode_JRC6_64K},
				{"JRC RAMBOX",  kATCartridgeMode_JRC_RAMBOX},
			}},
			{"T&he!Cart", {
				{"The!Cart 32MB flash",  kATCartridgeMode_TheCart_32M},
				{"The!Cart 64MB flash",  kATCartridgeMode_TheCart_64M},
				{"The!Cart 128MB flash", kATCartridgeMode_TheCart_128M},
			}},
			{"&Atrax / Williams / Blizzard", {
				{"Atrax 128K",     kATCartridgeMode_Atrax_128K},
				{"Atrax 128K (raw)", kATCartridgeMode_Atrax_128K_Raw},
				{"Williams 16K",   kATCartridgeMode_Williams_16K},
				{"Williams 32K",   kATCartridgeMode_Williams_32K},
				{"Williams 64K",   kATCartridgeMode_Williams_64K},
				{"Blizzard 4K",    kATCartridgeMode_Blizzard_4K},
				{"Blizzard 16K",   kATCartridgeMode_Blizzard_16K},
				{"Blizzard 32K",   kATCartridgeMode_Blizzard_32K},
			}},
			{"&Other", {
				{"SuperCharger 3D",          kATCartridgeMode_SuperCharger3D},
				{"AST 32K",                  kATCartridgeMode_AST_32K},
				{"COS 32K",                  kATCartridgeMode_COS32K},
				{"DB 32K",                   kATCartridgeMode_DB_32K},
				{"Diamond 64K",              kATCartridgeMode_Diamond_64K},
				{"Express 64K",              kATCartridgeMode_Express_64K},
				{"Turbosoft 64K",            kATCartridgeMode_Turbosoft_64K},
				{"Turbosoft 128K",           kATCartridgeMode_Turbosoft_128K},
				{"aDawliah 32K",             kATCartridgeMode_aDawliah_32K},
				{"aDawliah 64K",             kATCartridgeMode_aDawliah_64K},
				{"Telelink II",              kATCartridgeMode_TelelinkII},
				{"MicroCalc",                kATCartridgeMode_MicroCalc},
				{"MDDOS",                    kATCartridgeMode_MDDOS},
				{"Bounty Bob (800)",         kATCartridgeMode_BountyBob800},
				{"Pronto",                   kATCartridgeMode_Pronto},
				{"Phoenix 8K",               kATCartridgeMode_Phoenix_8K},
				{"Corina 512K (SRAM/EEPROM)", kATCartridgeMode_Corina_512K_SRAM_EEPROM},
				{"Corina 1M (EEPROM)",       kATCartridgeMode_Corina_1M_EEPROM},
			}},
		};

		for (const auto& fam : kFamilies) {
			auto *sub = specialMenu->addMenu(QObject::tr(fam.submenu));
			for (const auto& e : fam.entries)
				addEntry(sub, e);
		}
		specialMenu->addSeparator();

		// BASIC: defer to the simulator's LoadCartridgeBASIC(), which feeds
		// the bundled atbasic ROM into a kATCartridgeMode_8K cart via the
		// internal "special:basic" path. The ROM image was already imported
		// into the C++ build from the same atbasic.bin Qt resource.
		auto *basicAct = specialMenu->addAction(QObject::tr("BASIC"));
		QObject::connect(basicAct, &QAction::triggered, ctx.window, [&ctx]{
			try {
				ctx.sim->LoadCartridgeBASIC();
				ctx.sim->ColdReset();
				if (ctx.window)
					ctx.window->statusBar()->showMessage(
						QObject::tr("Attached BASIC"), 3000);
			} catch (const MyError& e) {
				QMessageBox::warning(ctx.window,
					QObject::tr("Attach failed"),
					QString::fromUtf8(e.c_str()));
			}
		});
	}

	auto *attachCartAction = fileMenu->addAction(QObject::tr("&Attach Cartridge..."));
	QObject::connect(attachCartAction, &QAction::triggered, ctx.window, [&ctx]{
		const QString path = QFileDialog::getOpenFileName(
			ctx.window,
			QObject::tr("Attach cartridge image"),
			{},
			QObject::tr("Atari cartridge images (*.car *.bin *.rom *.a52);;All files (*)"));
		if (path.isEmpty()) return;
		const std::wstring w = path.toStdWString();
		try {
			ATCartLoadContext *nullCtx = nullptr;
			if (ctx.sim->LoadCartridge(0u, w.c_str(), nullCtx))
				ctx.sim->ColdReset();
			else
				QMessageBox::warning(ctx.window, QObject::tr("Attach failed"),
				                     QObject::tr("Could not parse cartridge: %1").arg(path));
		} catch (const MyError& e) {
			QMessageBox::warning(ctx.window, QObject::tr("Attach failed"),
			                     QString::fromUtf8(e.c_str()));
		}
	});

	auto *detachCartAction = fileMenu->addAction(QObject::tr("&Detach Cartridge"));
	QObject::connect(detachCartAction, &QAction::triggered, [sim = ctx.sim]{
		sim->UnloadCartridge(0);
		sim->ColdReset();
	});

	// Secondary Cartridge submenu — slot 1 of the simulator's two cart slots.
	// Some cart hardware (e.g. piggyback-mode SpartaDOS X) needs a second
	// cart in the secondary slot. Mirrors the primary attach/detach handlers.
	{
		auto *secMenu = fileMenu->addMenu(QObject::tr("Secondary Cartridge"));

		auto *attachSecAct = secMenu->addAction(QObject::tr("&Attach..."));
		QObject::connect(attachSecAct, &QAction::triggered, ctx.window, [&ctx]{
			const QString path = QFileDialog::getOpenFileName(
				ctx.window,
				QObject::tr("Attach secondary cartridge image"),
				{},
				QObject::tr("Atari cartridge images (*.car *.bin *.rom *.a52);;All files (*)"));
			if (path.isEmpty()) return;
			const std::wstring w = path.toStdWString();
			try {
				ATCartLoadContext *nullCtx = nullptr;
				if (ctx.sim->LoadCartridge(1u, w.c_str(), nullCtx)) {
					ctx.sim->ColdReset();
					ctx.window->statusBar()->showMessage(
						QObject::tr("Attached secondary cartridge: %1")
							.arg(QFileInfo(path).fileName()), 2500);
				} else {
					QMessageBox::warning(ctx.window, QObject::tr("Attach failed"),
						QObject::tr("Could not parse cartridge: %1").arg(path));
				}
			} catch (const MyError& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Attach failed"),
					QString::fromUtf8(e.c_str()));
			}
		});

		auto *detachSecAct = secMenu->addAction(QObject::tr("&Detach"));
		QObject::connect(detachSecAct, &QAction::triggered, ctx.window, [&ctx]{
			ctx.sim->UnloadCartridge(1);
			ctx.sim->ColdReset();
			ctx.window->statusBar()->showMessage(
				QObject::tr("Detached secondary cartridge"), 2500);
		});
	}

	// Save Firmware submenu — exports the current cartridge or device
	// flash image to a host file. Each entry mirrors upstream's
	// File → Save Firmware (menu_default.txt). Cartridge writes via
	// ATCartridgeEmulator::Save (header iff .car); the four flash
	// devices route through ATSimulator::SaveStorage(storageId, path),
	// which fishes the IATDeviceFirmware out of the device manager.
	auto *saveFwMenu = fileMenu->addMenu(QObject::tr("Save Firmware"));
	{
		auto *saveCartAct = saveFwMenu->addAction(QObject::tr("&Save Cartridge..."));
		QObject::connect(saveCartAct, &QAction::triggered, ctx.window, [&ctx]{
			ATCartridgeEmulator *cart = ctx.sim->GetCartridge(0);
			const int mode = cart ? cart->GetMode() : 0;
			if (!mode) {
				QMessageBox::information(ctx.window,
					QObject::tr("Save Cartridge"),
					QObject::tr("There is no cartridge to save."));
				return;
			}
			if (mode == kATCartridgeMode_SuperCharger3D) {
				QMessageBox::information(ctx.window,
					QObject::tr("Save Cartridge"),
					QObject::tr("The current cartridge cannot be saved to an image file."));
				return;
			}
			QString selectedFilter;
			const QString path = QFileDialog::getSaveFileName(ctx.window,
				QObject::tr("Save cartridge image"),
				QStringLiteral("cartridge.car"),
				QObject::tr("Cartridge image with header (*.car);;"
				            "Raw cartridge image (*.bin *.rom)"),
				&selectedFilter);
			if (path.isEmpty()) return;
			const bool includeHeader = !selectedFilter.contains(
				QStringLiteral("*.bin"));
			try {
				cart->Save(path.toStdWString().c_str(), includeHeader);
			} catch (const MyError& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Save failed"),
					QString::fromUtf8(e.c_str()));
				return;
			}
			ctx.window->statusBar()->showMessage(
				QObject::tr("Saved firmware: %1").arg(QFileInfo(path).fileName()),
				2500);
		});

		struct FlashEntry {
			const char *label;
			int unit;       // ATStorageId offset from kATStorageId_Firmware
			const char *defaultName;
		};
		static constexpr FlashEntry kFlash[] = {
			{"Save KMK/JZ IDE / SIDE / MyIDE II Main Flash...", 0, "ide-main.rom"},
			{"Save KMK/JZ IDE SDX Flash...",                    1, "ide-sdx.rom"},
			{"Save Ultimate1MB Flash...",                       2, "u1mb.rom"},
			{"Save Rapidus Flash...",                           3, "rapidus.rom"},
		};
		for (const auto& f : kFlash) {
			auto *act = saveFwMenu->addAction(QObject::tr(f.label));
			QObject::connect(act, &QAction::triggered, ctx.window,
				[&ctx, f]{
					const ATStorageId sid = (ATStorageId)
						(kATStorageId_Firmware + f.unit);
					if (!ctx.sim->IsStoragePresent(sid)) {
						QMessageBox::information(ctx.window,
							QObject::tr("Save Firmware"),
							QObject::tr("Device not present."));
						return;
					}
					const QString path = QFileDialog::getSaveFileName(ctx.window,
						QObject::tr("Save firmware image"),
						QString::fromUtf8(f.defaultName),
						QObject::tr("Raw firmware image (*.rom);;"
						            "Binary image (*.bin);;All files (*)"));
					if (path.isEmpty()) return;
					try {
						ctx.sim->SaveStorage(sid, path.toStdWString().c_str());
					} catch (const MyError& e) {
						QMessageBox::warning(ctx.window,
							QObject::tr("Save failed"),
							QString::fromUtf8(e.c_str()));
						return;
					}
					ctx.window->statusBar()->showMessage(
						QObject::tr("Saved firmware: %1")
							.arg(QFileInfo(path).fileName()),
						2500);
				});
		}
	}

	fileMenu->addSeparator();
	auto *quitAction = fileMenu->addAction(QObject::tr("E&xit"));
	quitAction->setShortcut(QKeySequence::Quit);
	QObject::connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void buildViewMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *viewMenu = bar->addMenu(QObject::tr("&View"));

	auto *fullscreenAction = viewMenu->addAction(QObject::tr("&Full Screen"));
	fullscreenAction->setCheckable(true);
	fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
	QObject::connect(fullscreenAction, &QAction::triggered, ctx.window, [w = ctx.window](bool checked){
		if (checked) w->showFullScreen();
		else         w->showNormal();
	});

	auto *muteAction = viewMenu->addAction(QObject::tr("&Mute"));
	muteAction->setCheckable(true);
	muteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
	muteAction->setChecked(ctx.sim->GetAudioOutput()->GetMute());
	QObject::connect(muteAction, &QAction::toggled, [&ctx](bool on){
		ctx.sim->GetAudioOutput()->SetMute(on);
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("audio/mute"), on);
	});

	viewMenu->addSeparator();

	// Filter Mode submenu — wired to QtDisplay's SetFilterMode plus the
	// Qt-only "sharp bilinear" toggle (Themaister-style UV remap done in the
	// fragment shader).
	auto *filterMenu = viewMenu->addMenu(QObject::tr("Fi&lter Mode"));
	if (ctx.display) {
		const bool sbInitial = ctx.settings
			&& ctx.settings->value(QStringLiteral("view/sharpBilinear"), false).toBool();

		auto *next = filterMenu->addAction(QObject::tr("Next Mode"));
		QObject::connect(next, &QAction::triggered, [d = ctx.display, w = ctx.displayWidget, &ctx]{
			// 5-state cycle: 0..3 are upstream FilterModes, 4 is Sharp Bilinear.
			const bool wasSb = ctx.settings
				&& ctx.settings->value(QStringLiteral("view/sharpBilinear"), false).toBool();
			int idx = wasSb ? 4 : (int)d->GetFilterMode();
			idx = (idx + 1) % 5;
			const bool sb = (idx == 4);
			const auto fm = sb ? IVDVideoDisplay::kFilterBilinear
			                   : (IVDVideoDisplay::FilterMode)idx;
			d->SetFilterMode(fm);
			if (w) ATQtVideoDisplaySetSharpBilinear(w, sb);
			if (ctx.settings) {
				ctx.settings->setValue(QStringLiteral("view/filterMode"), (int)fm);
				ctx.settings->setValue(QStringLiteral("view/sharpBilinear"), sb);
			}
		});
		filterMenu->addSeparator();

		auto *grp = new QActionGroup(ctx.window);
		grp->setExclusive(true);
		struct FM { IVDVideoDisplay::FilterMode mode; bool sharp; const char *label; };
		static constexpr FM kModes[] = {
			{IVDVideoDisplay::kFilterPoint,        false, "Point"},
			{IVDVideoDisplay::kFilterBilinear,     false, "Bilinear"},
			{IVDVideoDisplay::kFilterBilinear,     true,  "Sharp Bilinear"},
			{IVDVideoDisplay::kFilterBicubic,      false, "Bicubic"},
			{IVDVideoDisplay::kFilterAnySuitable,  false, "Any Suitable"},
		};
		const auto current = ctx.display->GetFilterMode();
		for (const auto& m : kModes) {
			auto *act = filterMenu->addAction(QObject::tr(m.label));
			act->setCheckable(true);
			act->setActionGroup(grp);
			if (m.sharp == sbInitial && m.mode == current && (m.sharp || !sbInitial))
				act->setChecked(true);
			QObject::connect(act, &QAction::triggered,
				[d = ctx.display, w = ctx.displayWidget, &ctx,
				 mode = m.mode, sharp = m.sharp]{
				d->SetFilterMode(mode);
				if (w) ATQtVideoDisplaySetSharpBilinear(w, sharp);
				if (ctx.settings) {
					ctx.settings->setValue(QStringLiteral("view/filterMode"), (int)mode);
					ctx.settings->setValue(QStringLiteral("view/sharpBilinear"), sharp);
				}
			});
		}
	} else {
		filterMenu->menuAction()->setEnabled(false);
	}

	// Effects: custom GLSL fragment shader loader plus a few built-in
	// presets. None / Load... / preset names live in a single radio group.
	{
		auto *fxMenu = viewMenu->addMenu(QObject::tr("&Effects"));
		auto *fxGroup = new QActionGroup(ctx.window);
		fxGroup->setExclusive(true);

		struct Preset { const char *name; const char *src; };
		static constexpr const char *kPresetCRT = R"GLSL(#version 330 core
in vec2 v_uv;
out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel;
uniform vec2 u_sharpness;
uniform float u_scanline;
uniform float u_tex_height;
uniform float u_sharp_bilin;
void main() {
    vec4 c = texture(u_tex, v_uv);
    float row = v_uv.y * u_tex_height;
    float gap = abs(2.0 * fract(row) - 1.0);
    c.rgb *= 1.0 - 0.6 * gap;
    // simulate phosphor RGB stripe
    float colCycle = mod(gl_FragCoord.x, 3.0);
    vec3 mask = vec3(colCycle < 1.0 ? 1.1 : 0.85,
                     colCycle < 2.0 && colCycle >= 1.0 ? 1.1 : 0.85,
                     colCycle >= 2.0 ? 1.1 : 0.85);
    c.rgb *= mask;
    out_color = vec4(clamp(c.rgb, 0.0, 1.0), 1.0);
}
)GLSL";
		static constexpr const char *kPresetPhosphor = R"GLSL(#version 330 core
in vec2 v_uv;
out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel;
uniform vec2 u_sharpness;
uniform float u_scanline;
uniform float u_tex_height;
uniform float u_sharp_bilin;
void main() {
    vec4 c  = texture(u_tex, v_uv);
    vec4 c1 = texture(u_tex, v_uv + vec2( u_texel.x, 0.0));
    vec4 c2 = texture(u_tex, v_uv + vec2(-u_texel.x, 0.0));
    vec4 g  = 0.5 * c + 0.25 * (c1 + c2); // gentle phosphor smear
    out_color = vec4(clamp(g.rgb, 0.0, 1.0), 1.0);
}
)GLSL";
		static constexpr const char *kPresetComposite = R"GLSL(#version 330 core
in vec2 v_uv;
out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel;
uniform vec2 u_sharpness;
uniform float u_scanline;
uniform float u_tex_height;
uniform float u_sharp_bilin;
void main() {
    // Crude composite: per-channel horizontal blur, simulating bandwidth
    // limit on chroma vs luma.
    vec4 c0 = texture(u_tex, v_uv);
    vec4 cl = texture(u_tex, v_uv - vec2(u_texel.x, 0.0));
    vec4 cr = texture(u_tex, v_uv + vec2(u_texel.x, 0.0));
    float Y = dot(c0.rgb, vec3(0.299, 0.587, 0.114));
    vec3 chromaL = cl.rgb - dot(cl.rgb, vec3(0.299, 0.587, 0.114));
    vec3 chromaR = cr.rgb - dot(cr.rgb, vec3(0.299, 0.587, 0.114));
    vec3 chroma = 0.5 * (chromaL + chromaR);
    out_color = vec4(clamp(vec3(Y) + chroma, 0.0, 1.0), 1.0);
}
)GLSL";
		static constexpr const char *kPresetScanlinePlus = R"GLSL(#version 330 core
in vec2 v_uv;
out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel;
uniform vec2 u_sharpness;
uniform float u_scanline;
uniform float u_tex_height;
uniform float u_sharp_bilin;
void main() {
    vec4 c = texture(u_tex, v_uv);
    float row = v_uv.y * u_tex_height;
    float gap = abs(2.0 * fract(row) - 1.0);
    c.rgb *= 1.0 - 0.85 * gap;
    // Slight bloom: blend in horizontal neighbors when bright.
    float l = dot(c.rgb, vec3(0.3, 0.59, 0.11));
    if (l > 0.6) {
        vec4 nl = texture(u_tex, v_uv - vec2(u_texel.x, 0.0));
        vec4 nr = texture(u_tex, v_uv + vec2(u_texel.x, 0.0));
        c.rgb = mix(c.rgb, c.rgb + 0.4 * (nl.rgb + nr.rgb), 0.4);
    }
    out_color = vec4(clamp(c.rgb, 0.0, 1.0), 1.0);
}
)GLSL";
		static const Preset kPresets[] = {
			{"CRT",        kPresetCRT},
			{"Phosphor",   kPresetPhosphor},
			{"Composite",  kPresetComposite},
			{"Scanline+",  kPresetScanlinePlus},
		};

		auto *noneAct = fxMenu->addAction(QObject::tr("&None"));
		noneAct->setCheckable(true);
		noneAct->setActionGroup(fxGroup);
		QObject::connect(noneAct, &QAction::triggered, [w = ctx.displayWidget, &ctx]{
			if (w) ATQtVideoDisplaySetCustomShader(w, "");
			if (ctx.settings) {
				ctx.settings->remove(QStringLiteral("view/customShader"));
				ctx.settings->remove(QStringLiteral("view/effectPreset"));
			}
		});

		auto *loadAct = fxMenu->addAction(QObject::tr("&Load Shader..."));
		loadAct->setCheckable(true);
		loadAct->setActionGroup(fxGroup);
		QObject::connect(loadAct, &QAction::triggered, ctx.window,
			[w = ctx.window, dw = ctx.displayWidget, &ctx]{
				const QString path = QFileDialog::getOpenFileName(w,
					QObject::tr("Load fragment shader"), {},
					QObject::tr("GLSL fragment shaders (*.glsl *.frag);;All files (*)"));
				if (path.isEmpty()) return;
				QFile f(path);
				if (!f.open(QIODevice::ReadOnly)) {
					QMessageBox::warning(w, QObject::tr("Load failed"),
						QObject::tr("Could not read %1").arg(path));
					return;
				}
				const QByteArray src = f.readAll();
				ATQtVideoDisplaySetCustomShader(dw, src.constData());
				const QString err = QString::fromUtf8(
					ATQtVideoDisplayGetShaderError(dw));
				if (!err.isEmpty()) {
					QMessageBox::warning(w, QObject::tr("Shader compile error"), err);
					ATQtVideoDisplaySetCustomShader(dw, "");
					return;
				}
				if (ctx.settings) {
					ctx.settings->setValue(QStringLiteral("view/customShader"), path);
					ctx.settings->remove(QStringLiteral("view/effectPreset"));
				}
				w->statusBar()->showMessage(
					QObject::tr("Custom shader loaded: %1")
						.arg(QFileInfo(path).fileName()), 2500);
			});

		fxMenu->addSeparator();
		const QString savedPreset = ctx.settings
			? ctx.settings->value(QStringLiteral("view/effectPreset")).toString()
			: QString();
		const bool savedCustom = ctx.settings
			&& !ctx.settings->value(QStringLiteral("view/customShader")).toString().isEmpty();

		for (const auto& p : kPresets) {
			auto *act = fxMenu->addAction(QString::fromUtf8(p.name));
			act->setCheckable(true);
			act->setActionGroup(fxGroup);
			if (savedPreset == QString::fromUtf8(p.name)) act->setChecked(true);
			QObject::connect(act, &QAction::triggered,
				[w = ctx.window, dw = ctx.displayWidget, &ctx,
				 src = p.src, name = QString::fromUtf8(p.name)]{
					ATQtVideoDisplaySetCustomShader(dw, src);
					const QString err = QString::fromUtf8(
						ATQtVideoDisplayGetShaderError(dw));
					if (!err.isEmpty()) {
						QMessageBox::warning(w,
							QObject::tr("Shader compile error"), err);
						ATQtVideoDisplaySetCustomShader(dw, "");
						return;
					}
					if (ctx.settings) {
						ctx.settings->setValue(
							QStringLiteral("view/effectPreset"), name);
						ctx.settings->remove(QStringLiteral("view/customShader"));
					}
				});
		}

		// Initial check state.
		if (!savedPreset.isEmpty()) {
			// done by loop above
		} else if (savedCustom) {
			loadAct->setChecked(true);
		} else {
			noneAct->setChecked(true);
		}
	}

	// Filter Sharpness — applies a 5-tap unsharp mask in the QtDisplay
	// fragment shader. Five levels mirror upstream's Softer..Sharper.
	auto *sharpMenu = viewMenu->addMenu(QObject::tr("Filter Sharpness"));
	auto *sharpGroup = new QActionGroup(ctx.window);
	sharpGroup->setExclusive(true);
	struct SharpOpt { int level; const char *label; };
	static constexpr SharpOpt kSharpOpts[] = {
		{-2, "Softer"},
		{-1, "Soft"},
		{ 0, "Normal"},
		{+1, "Sharp"},
		{+2, "Sharper"},
	};
	const int savedSharp = ctx.settings
		? ctx.settings->value(QStringLiteral("view/filterSharpness"), 0).toInt()
		: 0;
	for (const auto& s : kSharpOpts) {
		auto *act = sharpMenu->addAction(QString::fromUtf8(s.label));
		act->setCheckable(true);
		act->setActionGroup(sharpGroup);
		if (s.level == savedSharp) act->setChecked(true);
		QObject::connect(act, &QAction::triggered,
			[disp = ctx.display, s = ctx.settings, level = s.level]{
			if (disp) disp->SetPixelSharpness(level * 0.5f, level * 0.5f);
			if (s) s->setValue(QStringLiteral("view/filterSharpness"), level);
		});
	}
	if (ctx.display)
		ctx.display->SetPixelSharpness(savedSharp * 0.5f, savedSharp * 0.5f);

	// Scanlines — overlay strength applied in the fragment shader. None /
	// Light / Medium / Heavy mirrors common CRT-emu options; intensities
	// chosen for visible-but-not-overwhelming gaps at typical 2-3x scales.
	auto *scanMenu = viewMenu->addMenu(QObject::tr("Scanlines"));
	auto *scanGroup = new QActionGroup(ctx.window);
	scanGroup->setExclusive(true);
	struct ScanOpt { int idx; const char *label; float strength; };
	static constexpr ScanOpt kScanOpts[] = {
		{0, "&None",   0.00f},
		{1, "&Light",  0.20f},
		{2, "&Medium", 0.45f},
		{3, "&Heavy",  0.70f},
	};
	const int savedScan = ctx.settings
		? ctx.settings->value(QStringLiteral("view/scanlines"), 0).toInt()
		: 0;
	for (const auto& o : kScanOpts) {
		auto *act = scanMenu->addAction(QObject::tr(o.label));
		act->setCheckable(true);
		act->setActionGroup(scanGroup);
		if (o.idx == savedScan) act->setChecked(true);
		QObject::connect(act, &QAction::triggered,
			[w = ctx.displayWidget, s = ctx.settings, idx = o.idx, st = o.strength]{
			ATQtVideoDisplaySetScanlineIntensity(w, st);
			if (s) s->setValue(QStringLiteral("view/scanlines"), idx);
		});
	}
	if (ctx.displayWidget && savedScan >= 0
	    && savedScan < (int)std::size(kScanOpts))
		ATQtVideoDisplaySetScanlineIntensity(ctx.displayWidget,
		                                     kScanOpts[savedScan].strength);

	// Video Frame — stretch modes for fitting the source frame inside the
	// widget. Persisted in QSettings so the chosen mode survives restarts.
	auto *frameMenu = viewMenu->addMenu(QObject::tr("Video Fra&me"));
	auto *frameGroup = new QActionGroup(ctx.window);
	frameGroup->setExclusive(true);
	struct StretchOpt { ATQtStretchMode mode; const char *label; };
	static constexpr StretchOpt kStretchOpts[] = {
		{ATQtStretchMode::FitWindow,        "&Fit to Window"},
		{ATQtStretchMode::PreserveAspect,   "&Preserve Aspect Ratio"},
		{ATQtStretchMode::SquarePixels,     "&Square Pixels"},
		{ATQtStretchMode::IntegralAspect,   "Preserve Aspect Ratio (&integer multiples)"},
		{ATQtStretchMode::IntegralSquare,   "Square Pixels (i&nteger multiples)"},
	};
	const int savedStretch = ctx.settings
		? ctx.settings->value(QStringLiteral("view/stretchMode"),
		                      (int)ATQtStretchMode::PreserveAspect).toInt()
		: (int)ATQtStretchMode::PreserveAspect;
	for (const auto& s : kStretchOpts) {
		auto *act = frameMenu->addAction(QObject::tr(s.label));
		act->setCheckable(true);
		act->setActionGroup(frameGroup);
		if ((int)s.mode == savedStretch) act->setChecked(true);
		QObject::connect(act, &QAction::triggered,
			[w = ctx.displayWidget, s = ctx.settings, mode = s.mode]{
			ATQtVideoDisplaySetStretchMode(w, mode);
			if (s) s->setValue(QStringLiteral("view/stretchMode"), (int)mode);
		});
	}
	if (ctx.displayWidget)
		ATQtVideoDisplaySetStretchMode(ctx.displayWidget, (ATQtStretchMode)savedStretch);

	// Pan/Zoom tool — toggling it changes the mouse mode of the display
	// widget (drag pans, wheel zooms). Reset items below clear offsets.
	frameMenu->addSeparator();
	auto *panZoomAction = frameMenu->addAction(QObject::tr("Pan/&Zoom Tool"));
	panZoomAction->setCheckable(true);
	const bool savedPanZoom = ctx.settings
		? ctx.settings->value(QStringLiteral("view/panZoomEnabled"), false).toBool()
		: false;
	panZoomAction->setChecked(savedPanZoom);
	if (ctx.displayWidget && savedPanZoom)
		ATQtVideoDisplaySetPanZoomEnabled(ctx.displayWidget, true);
	QObject::connect(panZoomAction, &QAction::toggled, [&ctx](bool on){
		ATQtVideoDisplaySetPanZoomEnabled(ctx.displayWidget, on);
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("view/panZoomEnabled"), on);
	});

	auto *resetPanZoomAction = frameMenu->addAction(QObject::tr("Reset Pan and Zoom"));
	QObject::connect(resetPanZoomAction, &QAction::triggered, [&ctx]{
		ATQtVideoDisplayResetPanZoom(ctx.displayWidget);
	});
	auto *resetPanAction = frameMenu->addAction(QObject::tr("Reset Panning"));
	QObject::connect(resetPanAction, &QAction::triggered, [&ctx]{
		ATQtVideoDisplayResetPan(ctx.displayWidget);
	});
	auto *resetZoomAction = frameMenu->addAction(QObject::tr("Reset Zoom"));
	QObject::connect(resetZoomAction, &QAction::triggered, [&ctx]{
		ATQtVideoDisplayResetZoom(ctx.displayWidget);
	});

	// Overscan Mode — selects how much beyond the 160-cycle visible area is
	// rendered. Wired to GTIA's OverscanMode setter; the display refreshes
	// on the next frame.
	auto *overscanMenu = viewMenu->addMenu(QObject::tr("&Overscan Mode"));
	auto *overscanGroup = new QActionGroup(ctx.window);
	overscanGroup->setExclusive(true);
	struct OverscanOpt { ATGTIAEmulator::OverscanMode mode; const char *label; };
	static constexpr OverscanOpt kOverscanOpts[] = {
		{ATGTIAEmulator::kOverscanOSScreen,    "&OS Screen (160 cc)"},
		{ATGTIAEmulator::kOverscanNormal,      "&Normal (168 cc)"},
		{ATGTIAEmulator::kOverscanWidescreen,  "&Widescreen (176 cc)"},
		{ATGTIAEmulator::kOverscanExtended,    "&Extended (192 cc)"},
		{ATGTIAEmulator::kOverscanFull,        "&Full (228 cc)"},
	};
	const auto curOverscan = ctx.sim->GetGTIA().GetOverscanMode();
	for (const auto& o : kOverscanOpts) {
		auto *act = overscanMenu->addAction(QString::fromUtf8(o.label));
		act->setCheckable(true);
		act->setActionGroup(overscanGroup);
		if (curOverscan == o.mode) act->setChecked(true);
		QObject::connect(act, &QAction::triggered, [sim = ctx.sim, &ctx, mode = o.mode]{
			sim->GetGTIA().SetOverscanMode(mode);
			if (ctx.settings)
				ctx.settings->setValue(QStringLiteral("view/overscanMode"), (int)mode);
		});
	}

	overscanMenu->addSeparator();

	auto *vertMenu = overscanMenu->addMenu(QObject::tr("&Vertical Override"));
	auto *vertGroup = new QActionGroup(ctx.window);
	vertGroup->setExclusive(true);
	struct VertOpt { ATGTIAEmulator::VerticalOverscanMode mode; const char *label; };
	static constexpr VertOpt kVertOpts[] = {
		{ATGTIAEmulator::kVerticalOverscan_Default,  "&Off"},
		{ATGTIAEmulator::kVerticalOverscan_OSScreen, "OS &Screen"},
		{ATGTIAEmulator::kVerticalOverscan_Normal,   "&Normal"},
		{ATGTIAEmulator::kVerticalOverscan_Extended, "&Extended"},
		{ATGTIAEmulator::kVerticalOverscan_Full,     "&Full"},
	};
	const auto curVert = ctx.sim->GetGTIA().GetVerticalOverscanMode();
	for (const auto& v : kVertOpts) {
		auto *act = vertMenu->addAction(QString::fromUtf8(v.label));
		act->setCheckable(true);
		act->setActionGroup(vertGroup);
		if (curVert == v.mode) act->setChecked(true);
		QObject::connect(act, &QAction::triggered, [&ctx, mode = v.mode]{
			ctx.sim->GetGTIA().SetVerticalOverscanMode(mode);
			if (ctx.settings)
				ctx.settings->setValue(QStringLiteral("view/verticalOverscan"), (int)mode);
		});
	}

	auto *indMarginAction = overscanMenu->addAction(QObject::tr("&Indicator Margin"));
	indMarginAction->setCheckable(true);
	const bool indMarginOn = ctx.settings
		? ctx.settings->value(QStringLiteral("view/indicatorMargin"), false).toBool()
		: false;
	indMarginAction->setChecked(indMarginOn);
	QObject::connect(indMarginAction, &QAction::toggled, [&ctx](bool on){
		ATQtVideoDisplaySetIndicatorMargin(ctx.displayWidget, on ? 24 : 0);
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("view/indicatorMargin"), on);
	});

	// Display Output / Analysis submenu — picks one of the GTIA / ANTIC
	// alternate visualizations (or none = normal output). Five mutually
	// exclusive states; persists as `view/outputIndex`.
	//   0 = None
	//   1 = ANTIC DMA Timing
	//   2 = GTIA Layers
	//   3 = GTIA Colors
	//   4 = GTIA Display List
	auto *outputMenu = viewMenu->addMenu(QObject::tr("Out&put"));
	{
		struct OutOpt { int idx; const char *label; };
		static constexpr OutOpt kOpts[] = {
			{0, "Normal"},
			{1, "ANTIC DMA Timing"},
			{2, "GTIA Layers"},
			{3, "GTIA Colors"},
			{4, "GTIA Display List"},
		};
		auto applyOutput = [](ATSimulator *sim, int idx) {
			using GA = ATGTIAEmulator::AnalysisMode;
			using AA = ATAnticEmulator::AnalysisMode;
			sim->GetAntic().SetAnalysisMode(idx == 1 ? AA::kAnalyzeDMATiming
			                                        : AA::kAnalyzeOff);
			GA gm = GA::kAnalyzeNone;
			if (idx == 2) gm = GA::kAnalyzeLayers;
			if (idx == 3) gm = GA::kAnalyzeColors;
			if (idx == 4) gm = GA::kAnalyzeDList;
			sim->GetGTIA().SetAnalysisMode(gm);
		};
		const int saved = ctx.settings
			? ctx.settings->value(QStringLiteral("view/outputIndex"), 0).toInt()
			: 0;
		applyOutput(ctx.sim, saved);
		auto *grp = new QActionGroup(ctx.window);
		grp->setExclusive(true);
		for (const auto& o : kOpts) {
			auto *act = outputMenu->addAction(QObject::tr(o.label));
			act->setCheckable(true);
			act->setActionGroup(grp);
			if (saved == o.idx) act->setChecked(true);
			QObject::connect(act, &QAction::triggered, [&ctx, idx = o.idx, applyOutput]{
				applyOutput(ctx.sim, idx);
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("view/outputIndex"), idx);
			});
		}
	}

	// VSync — toggles QSurfaceFormat swap interval at the root surface;
	// the change applies to subsequently swapped buffers.
	{
		auto *vsyncAction = viewMenu->addAction(QObject::tr("&VSync"));
		vsyncAction->setCheckable(true);
		const bool vsync = ctx.settings
			? ctx.settings->value(QStringLiteral("view/vsync"), true).toBool()
			: true;
		vsyncAction->setChecked(vsync);
		QObject::connect(vsyncAction, &QAction::toggled,
			[w = ctx.displayWidget, &ctx](bool on){
				if (auto *gl = qobject_cast<QOpenGLWidget *>(w)) {
					QSurfaceFormat fmt = gl->format();
					fmt.setSwapInterval(on ? 1 : 0);
					gl->setFormat(fmt);
				}
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("view/vsync"), on);
			});
	}

	// PAL Extended — toggles the wider PAL frame height (312 lines vs
	// 262 NTSC) via ATGTIA::SetOverscanPALExtended.
	{
		auto *palExt = viewMenu->addAction(QObject::tr("PAL E&xtended"));
		palExt->setCheckable(true);
		const bool initial = ctx.sim->GetGTIA().IsOverscanPALExtended();
		palExt->setChecked(initial);
		QObject::connect(palExt, &QAction::toggled,
			[sim = ctx.sim, &ctx](bool on){
				sim->GetGTIA().SetOverscanPALExtended(on);
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("view/palExtended"), on);
			});
	}

	// Reset Window Layout — clears any persisted dock-state blob so the
	// next launch falls back to the default layout. Also re-runs the
	// menu-build (the live process keeps its current dock layout until
	// restart, since QMainWindow's restoreState is one-shot at startup).
	{
		auto *reset = viewMenu->addAction(QObject::tr("Reset Window &Layout"));
		QObject::connect(reset, &QAction::triggered, ctx.window,
			[w = ctx.window, &ctx]{
				if (ctx.settings) {
					ctx.settings->remove(QStringLiteral("window/state"));
					ctx.settings->remove(QStringLiteral("window/geometry"));
				}
				w->statusBar()->showMessage(
					QObject::tr("Window layout will reset on next launch"), 3000);
			});
	}

	viewMenu->addSeparator();

	// Show FPS — flips both the on-screen HUD overlay (top-right text) and
	// the title-bar suffix in lockstep, persisted in view/showFps. The HUD
	// overlay is also independently toggleable from Tools → Customize HUD.
	auto *fpsAction = viewMenu->addAction(QObject::tr("Show FPS"));
	fpsAction->setCheckable(true);
	if (ctx.settings)
		fpsAction->setChecked(ctx.settings->value(QStringLiteral("view/showFps"), false).toBool());
	QObject::connect(fpsAction, &QAction::triggered, [&ctx](bool on){
		::g_atShowFps = on;
		if (ctx.settings) {
			ctx.settings->setValue(QStringLiteral("view/showFps"), on);
			ctx.settings->setValue(QStringLiteral("view/hud/fps"), on);
		}
		if (ctx.displayWidget)
			ATQtVideoDisplaySetHudEnabled(ctx.displayWidget, "fps", on);
		if (!on && ctx.window)
			ctx.window->setWindowTitle(QStringLiteral("Altirra (Qt port)"));
	});
	auto *adjustColorsAction = viewMenu->addAction(QObject::tr("Adjust Colors..."));
	QObject::connect(adjustColorsAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowAdjustColorsDialog(ctx.window, ctx.sim);
	});

	auto *audioLevelsAction = viewMenu->addAction(QObject::tr("Audio Levels..."));
	QObject::connect(audioLevelsAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowAudioLevelsDialog(ctx.window, ctx.sim, ctx.settings);
	});

	// Enhanced-text mode + font picker. The display widget skips its GL
	// blit and renders the screen as host-font text via QPainter,
	// reading each row through a provider installed below in main.cpp.
	{
		auto *enhAction = viewMenu->addAction(QObject::tr("&Enhanced Text"));
		enhAction->setCheckable(true);
		const bool initial = ctx.settings
			? ctx.settings->value(QStringLiteral("view/enhancedText"), false).toBool()
			: false;
		enhAction->setChecked(initial);
		QObject::connect(enhAction, &QAction::toggled,
			[w = ctx.displayWidget, &ctx](bool on){
				if (w) ATQtVideoDisplaySetEnhancedText(w, on);
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("view/enhancedText"), on);
			});

		auto *fontAction = viewMenu->addAction(QObject::tr("Enhanced Text &Font..."));
		QObject::connect(fontAction, &QAction::triggered, ctx.window,
			[w = ctx.window, dw = ctx.displayWidget, &ctx]{
				bool ok = false;
				const QFont starting = ctx.settings
					? QFont(ctx.settings->value(
						QStringLiteral("view/enhancedTextFont"),
						QStringLiteral("monospace,14")).toString().section(',', 0, 0),
						ctx.settings->value(
							QStringLiteral("view/enhancedTextFont"),
							QStringLiteral("monospace,14")).toString()
							.section(',', 1, 1).toInt())
					: QFont(QStringLiteral("monospace"), 14);
				const QFont chosen = QFontDialog::getFont(&ok, starting, w);
				if (!ok) return;
				if (dw) ATQtVideoDisplaySetEnhancedTextFont(dw, chosen);
				if (ctx.settings)
					ctx.settings->setValue(QStringLiteral("view/enhancedTextFont"),
						QStringLiteral("%1,%2").arg(chosen.family())
						                         .arg(chosen.pointSize()));
			});
	}

	// XEP80 / alt-video output viewer. Pops a non-modal window with a tab
	// per IATDeviceVideoOutput registered by the simulator.
	auto *altVideoAction = viewMenu->addAction(QObject::tr("XEP80 / Alt Video Output..."));
	QObject::connect(altVideoAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowAltVideoDialog(ctx.window, ctx.sim);
	});

	// Output autoswitching — when on, the main display widget switches to
	// the most recently active alt-video output (e.g. XEP80) automatically.
	// Persists; consulted by main.cpp's per-frame indicator timer.
	auto *autoSwitchAction = viewMenu->addAction(QObject::tr("Output &Autoswitching"));
	autoSwitchAction->setCheckable(true);
	if (ctx.settings)
		autoSwitchAction->setChecked(
			ctx.settings->value(QStringLiteral("view/altOutputAutoswitch"), false).toBool());
	QObject::connect(autoSwitchAction, &QAction::toggled, [&ctx](bool on){
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("view/altOutputAutoswitch"), on);
	});

	// Customize HUD — toggles individual on-screen overlays. The QtDisplay
	// backend tracks each independently and renders enabled ones via
	// QPainter on top of the GL frame.
	auto *customizeHudAction = viewMenu->addAction(QObject::tr("&Customize HUD..."));
	QObject::connect(customizeHudAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowCustomizeHudDialog(ctx.window, ctx.displayWidget, ctx.settings);
	});

	// Calibrate — fullscreen test pattern dialog for adjusting host display.
	auto *calibrateAction = viewMenu->addAction(QObject::tr("Calibrate..."));
	QObject::connect(calibrateAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowCalibrateDialog(ctx.window);
	});

	viewMenu->addSeparator();

	// Printer Output pane — non-modal window that mirrors text/ESC-P
	// output from any printer device the user has attached. Reuses a
	// single dialog instance across invocations.
	auto *printerOutputAction = viewMenu->addAction(QObject::tr("&Printer Output"));
	QObject::connect(printerOutputAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowPrinterOutputDialog(ctx.window, ctx.sim);
	});

	viewMenu->addSeparator();

	// Copy/Save Frame — grab from the QOpenGLWidget framebuffer.
	auto grabFrame = [w = ctx.displayWidget]() -> QImage {
		if (auto *gl = qobject_cast<QOpenGLWidget *>(w))
			return gl->grabFramebuffer();
		if (w) return w->grab().toImage();
		return {};
	};

	// Apply pixel-aspect correction. PAR > 1 means a pixel is wider than tall,
	// so we widen the image to compensate.
	auto trueAspect = [sim = ctx.sim](QImage img) -> QImage {
		if (img.isNull()) return img;
		const double par = sim->GetGTIA().GetPixelAspectRatio();
		if (par <= 0.0 || par == 1.0) return img;
		int w = img.width(), h = img.height();
		if (par > 1.0) w = (int)(w * par + 0.5);
		else           h = (int)(h / par + 0.5);
		return img.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	};

	auto *copyFrameAction = viewMenu->addAction(QObject::tr("Copy Frame to Clipboard"));
	if (ctx.displayWidget) {
		QObject::connect(copyFrameAction, &QAction::triggered, [grabFrame]{
			QGuiApplication::clipboard()->setImage(grabFrame());
		});
	} else {
		copyFrameAction->setEnabled(false);
	}

	auto *copyFrameTrueAction = viewMenu->addAction(QObject::tr("Copy Frame to Clipboard (True Aspect)"));
	if (ctx.displayWidget) {
		QObject::connect(copyFrameTrueAction, &QAction::triggered, [grabFrame, trueAspect]{
			QGuiApplication::clipboard()->setImage(trueAspect(grabFrame()));
		});
	} else {
		copyFrameTrueAction->setEnabled(false);
	}

	auto *saveFrameAction = viewMenu->addAction(QObject::tr("Save Frame..."));
	if (ctx.displayWidget) {
		QObject::connect(saveFrameAction, &QAction::triggered, ctx.window, [w = ctx.window, grabFrame]{
			const QString path = QFileDialog::getSaveFileName(
				w,
				QObject::tr("Save frame"),
				QStringLiteral("altirra-frame.png"),
				QObject::tr("PNG image (*.png);;BMP image (*.bmp);;All files (*)"));
			if (path.isEmpty()) return;
			grabFrame().save(path);
		});
	} else {
		saveFrameAction->setEnabled(false);
	}

	auto *saveFrameTrueAction = viewMenu->addAction(QObject::tr("Save Frame (True Aspect)..."));
	if (ctx.displayWidget) {
		QObject::connect(saveFrameTrueAction, &QAction::triggered, ctx.window,
			[w = ctx.window, grabFrame, trueAspect]{
			const QString path = QFileDialog::getSaveFileName(
				w,
				QObject::tr("Save frame (true aspect)"),
				QStringLiteral("altirra-frame.png"),
				QObject::tr("PNG image (*.png);;BMP image (*.bmp);;All files (*)"));
			if (path.isEmpty()) return;
			trueAspect(grabFrame()).save(path);
		});
	} else {
		saveFrameTrueAction->setEnabled(false);
	}
}

void buildSystemMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *sysMenu = bar->addMenu(QObject::tr("S&ystem"));

	auto *cfgSysAction = sysMenu->addAction(QObject::tr("Configure System..."));
	QObject::connect(cfgSysAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowConfigSystemDialog(ctx.window, ctx.sim, ctx.settings);
	});

	auto *saveProfileAction = sysMenu->addAction(QObject::tr("Save Profile..."));
	if (ctx.settings) {
		QObject::connect(saveProfileAction, &QAction::triggered, ctx.window, [&ctx]{
			bool ok = false;
			const QString name = QInputDialog::getText(ctx.window,
				QObject::tr("Save Profile"),
				QObject::tr("Profile name:"),
				QLineEdit::Normal, QString(), &ok);
			if (!ok || name.isEmpty()) return;
			auto& settings = *ctx.settings;
			settings.sync();
			const QStringList keys = settings.allKeys();
			const QString prefix = QStringLiteral("profiles/") + name + QStringLiteral("/");
			for (const QString& k : keys) {
				if (k.startsWith(QStringLiteral("profiles/"))) continue;
				if (k.startsWith(QStringLiteral("window/"))) continue;
				settings.setValue(prefix + k, settings.value(k));
			}
			settings.setValue(QStringLiteral("system/lastProfile"), name);
			ctx.window->statusBar()->showMessage(
				QObject::tr("Saved profile: %1").arg(name), 2500);
		});
	} else {
		saveProfileAction->setEnabled(false);
	}

	auto *loadProfileMenu = sysMenu->addMenu(QObject::tr("Load Profile"));
	if (ctx.settings) {
		QObject::connect(loadProfileMenu, &QMenu::aboutToShow, [loadProfileMenu, &ctx]{
			loadProfileMenu->clear();
			auto& settings = *ctx.settings;
			settings.sync();
			const QStringList keys = settings.allKeys();
			QStringList profileNames;
			for (const QString& k : keys) {
				if (!k.startsWith(QStringLiteral("profiles/"))) continue;
				const int slash = k.indexOf(QLatin1Char('/'), 9);
				if (slash < 0) continue;
				const QString name = k.mid(9, slash - 9);
				if (!profileNames.contains(name))
					profileNames.append(name);
			}
			if (profileNames.isEmpty()) {
				auto *act = loadProfileMenu->addAction(QObject::tr("(no saved profiles)"));
				act->setEnabled(false);
				return;
			}
			profileNames.sort(Qt::CaseInsensitive);
			for (const QString& name : profileNames) {
				auto *act = loadProfileMenu->addAction(name);
				QObject::connect(act, &QAction::triggered, [&ctx, name]{
					if (!ATApplyProfile(*ctx.settings, name)) {
						ctx.window->statusBar()->showMessage(
							QObject::tr("Profile not found: %1").arg(name), 2500);
						return;
					}
					ATApplyAllSettings(ctx);
					ctx.sim->ColdReset();
					// Refresh the menu bar so checked states reflect the
					// freshly-applied settings.
					ATBuildAltirraMenuBar(ctx);
					ctx.window->statusBar()->showMessage(
						QObject::tr("Loaded profile: %1").arg(name), 2500);
				});
			}
		});
	} else {
		loadProfileMenu->menuAction()->setEnabled(false);
	}

	auto *editProfilesAction = sysMenu->addAction(QObject::tr("Edit Profiles..."));
	if (ctx.settings) {
		QObject::connect(editProfilesAction, &QAction::triggered, ctx.window, [&ctx]{
			ATShowProfilesEditorDialog(ctx.window, ctx.settings);
			// Profile rename / delete may have changed system/lastProfile.
			// Rebuild the menu so the Load Profile submenu reflects the
			// fresh list on next open.
			ATBuildAltirraMenuBar(ctx);
		});
	} else {
		editProfilesAction->setEnabled(false);
	}

	sysMenu->addSeparator();

	auto *warmResetAction = sysMenu->addAction(QObject::tr("Warm Reset"));
	warmResetAction->setShortcut(QKeySequence(Qt::Key_F5));
	QObject::connect(warmResetAction, &QAction::triggered, [sim = ctx.sim]{ sim->WarmReset(); });

	auto *coldResetAction = sysMenu->addAction(QObject::tr("Cold Reset"));
	coldResetAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F5));
	QObject::connect(coldResetAction, &QAction::triggered, [sim = ctx.sim]{ sim->ColdReset(); });

	auto *coldResetCompAction = sysMenu->addAction(QObject::tr("Cold Reset (Computer Only)"));
	QObject::connect(coldResetCompAction, &QAction::triggered, [sim = ctx.sim]{ sim->ColdResetComputerOnly(); });

	auto *pauseAction = sysMenu->addAction(QObject::tr("Pause"));
	pauseAction->setCheckable(true);
	pauseAction->setShortcut(QKeySequence(Qt::Key_F9));
	QObject::connect(pauseAction, &QAction::triggered, [sim = ctx.sim](bool checked){
		if (checked) sim->Pause();
		else         sim->Resume();
	});

	sysMenu->addSeparator();

	auto *warpAction = sysMenu->addAction(QObject::tr("&Warp Speed"));
	warpAction->setCheckable(true);
	warpAction->setShortcut(QKeySequence(Qt::Key_F8));
	warpAction->setChecked(ctx.sim->IsTurboModeEnabled());
	QObject::connect(warpAction, &QAction::triggered, [sim = ctx.sim](bool on){
		sim->SetTurboModeEnabled(on);
	});

	auto *pauseInactiveAction = sysMenu->addAction(QObject::tr("Pause When Inactive"));
	pauseInactiveAction->setCheckable(true);
	if (ctx.settings)
		pauseInactiveAction->setChecked(
			ctx.settings->value(QStringLiteral("system/pauseWhenInactive"), false).toBool());
	QObject::connect(pauseInactiveAction, &QAction::triggered, [&ctx](bool on){
		// Hooks ApplicationStateChanged from the QApplication; we install
		// the listener once on first toggle. The setting persists so the
		// state survives across runs.
		static QMetaObject::Connection conn;
		QObject::disconnect(conn);
		if (on) {
			conn = QObject::connect(qApp, &QGuiApplication::applicationStateChanged,
				[sim = ctx.sim](Qt::ApplicationState st){
					if (st == Qt::ApplicationActive)        sim->Resume();
					else if (st == Qt::ApplicationInactive) sim->Pause();
				});
		}
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("system/pauseWhenInactive"), on);
	});

	sysMenu->addSeparator();

	// Power-On Delay — radio group: Auto / None / 1s / 2s / 3s.
	auto *powerOnMenu = sysMenu->addMenu(QObject::tr("Power-On Delay"));
	auto *poGrp = new QActionGroup(ctx.window);
	poGrp->setExclusive(true);
	struct PoOpt { int tenths; const char *label; };
	static constexpr PoOpt kPoOpts[] = {
		{-1, "&Auto"},
		{ 0, "&None"},
		{10, "&1 Second"},
		{20, "&2 Seconds"},
		{30, "&3 Seconds"},
	};
	const int currentDelay = ctx.sim->GetPowerOnDelay();
	for (const auto& opt : kPoOpts) {
		auto *act = powerOnMenu->addAction(QObject::tr(opt.label));
		act->setCheckable(true);
		act->setActionGroup(poGrp);
		if (opt.tenths == currentDelay) act->setChecked(true);
		QObject::connect(act, &QAction::triggered, [sim = ctx.sim, &ctx, t = opt.tenths]{
			sim->SetPowerOnDelay(t);
			if (ctx.settings)
				ctx.settings->setValue(QStringLiteral("system/powerOnDelay"), t);
		});
	}
	auto *basicAction = sysMenu->addAction(QObject::tr("Internal BASIC (Boot Without Option Key)"));
	basicAction->setCheckable(true);
	basicAction->setChecked(ctx.sim->IsBASICEnabled());
	QObject::connect(basicAction, &QAction::triggered, [&ctx](bool on){
		ctx.sim->SetBASICEnabled(on);
		ctx.sim->ColdReset();
		if (ctx.settings)
			ctx.settings->setValue(QStringLiteral("system/basicEnabled"), on);
	});

	auto *autoBootTapeAction = sysMenu->addAction(QObject::tr("Auto-Boot Tape (Hold Start)"));
	autoBootTapeAction->setCheckable(true);
	autoBootTapeAction->setChecked(ctx.sim->IsCassetteAutoBootEnabled());
	QObject::connect(autoBootTapeAction, &QAction::toggled, [sim = ctx.sim](bool on){
		sim->SetCassetteAutoBootEnabled(on);
	});

	sysMenu->addSeparator();

	// Hardware mode is not in upstream's exact menu position, but it's a
	// reasonable extension here so users can pick a machine without
	// editing a profile dialog (which we don't have ported yet).
	auto *hwModeMenu = sysMenu->addMenu(QObject::tr("Hardware Mode"));
	auto *hwGroup = new QActionGroup(ctx.window);
	hwGroup->setExclusive(true);
	struct HwOpt { ATHardwareMode mode; const char *label; };
	static constexpr HwOpt kHwOpts[] = {
		{kATHardwareMode_800,    "Atari &800"},
		{kATHardwareMode_800XL,  "Atari 800&XL"},
		{kATHardwareMode_1200XL, "Atari &1200XL"},
		{kATHardwareMode_130XE,  "Atari &130XE"},
		{kATHardwareMode_XEGS,   "Atari &XEGS"},
		{kATHardwareMode_5200,   "Atari &5200"},
	};
	const ATHardwareMode currentMode = ctx.sim->GetHardwareMode();
	for (const auto& opt : kHwOpts) {
		auto *act = hwModeMenu->addAction(QString::fromUtf8(opt.label));
		act->setCheckable(true);
		act->setActionGroup(hwGroup);
		if (currentMode == opt.mode)
			act->setChecked(true);
		QObject::connect(act, &QAction::triggered, [&ctx, mode = opt.mode]{
			ctx.sim->SetHardwareMode(mode);
			ctx.sim->LoadROMs();
			ctx.sim->ColdReset();
			if (ctx.settings)
				ctx.settings->setValue(QStringLiteral("system/hardwareMode"), (int)mode);
		});
	}

	// Console Switches submenu — base console switches plus per-device
	// button actions matching upstream menu_default.txt. Devices come and
	// go as the user changes the system config, so the per-device entries
	// re-evaluate their enabled / checked state via aboutToShow each time
	// the submenu is opened.
	auto *consoleMenu = sysMenu->addMenu(QObject::tr("Console Switches"));

	auto *kbdPresentAction = consoleMenu->addAction(QObject::tr("Keyboard Present (XEGS)"));
	kbdPresentAction->setCheckable(true);
	kbdPresentAction->setChecked(ctx.sim->IsKeyboardPresent());
	QObject::connect(kbdPresentAction, &QAction::toggled, [sim = ctx.sim](bool on){
		sim->SetKeyboardPresent(on);
	});

	auto *selfTestAction = consoleMenu->addAction(QObject::tr("Force Self-Test"));
	selfTestAction->setCheckable(true);
	selfTestAction->setChecked(ctx.sim->IsForcedSelfTest());
	QObject::connect(selfTestAction, &QAction::triggered, [sim = ctx.sim](bool on){
		sim->SetForcedSelfTest(on);
	});

	consoleMenu->addSeparator();

	// Per-device console-switch entries. Each entry corresponds to one
	// ATDeviceButton. Pressing the menu item walks every device that
	// implements IATDeviceButtons and forwards the activation; only the
	// device that owns that button responds. The "checkable" buttons map
	// to upstream commands that report a checked state through
	// IsButtonDepressed(). The remaining ones are momentary push buttons
	// (one-shot Activate(true) — or Activate(true)+Activate(false) for
	// the buttons whose device expects both edges).
	struct DevButtonEntry {
		const char    *label;
		ATDeviceButton button;
		bool           checkable;
		bool           sendRelease;   // emit Activate(false) after Activate(true)
		bool           toggle;        // Happy* — Activate(!IsDepressed)
	};
	static constexpr DevButtonEntry kDevButtons[] = {
		{ "BlackBox: Dump Screen",           kATDeviceButton_BlackBoxDumpScreen,    false, false, false },
		{ "BlackBox: Menu",                  kATDeviceButton_BlackBoxMenu,          false, false, false },
		{ "IDE Plus 2.0: Switch Disks",      kATDeviceButton_IDEPlus2SwitchDisks,   false, false, false },
		{ "IDE Plus 2.0: Write Protect",     kATDeviceButton_IDEPlus2WriteProtect,  true,  false, false },
		{ "IDE Plus 2.0: SDX Enable",        kATDeviceButton_IDEPlus2SDX,           true,  false, false },
		{ "Indus GT: Error Button",          kATDeviceButton_IndusGTError,          true,  true,  false },
		{ "Indus GT: Track Button",          kATDeviceButton_IndusGTTrack,          true,  true,  false },
		{ "Indus GT: Drive Type Button",     kATDeviceButton_IndusGTId,             true,  true,  false },
		{ "Indus GT: Boot CP/M",             kATDeviceButton_IndusGTBootCPM,        true,  false, false },
		{ "Indus GT: Change Density",        kATDeviceButton_IndusGTChangeDensity,  true,  false, false },
		{ "Happy: Slow Switch",              kATDeviceButton_HappySlow,             true,  false, true  },
		{ "Happy 1050: Write protect disk",  kATDeviceButton_HappyWPEnable,         true,  false, true  },
		{ "Happy 1050: Write enable disk",   kATDeviceButton_HappyWPDisable,        true,  false, true  },
		{ "ATR8000: Reset",                  kATDeviceButton_ATR8000Reset,          false, true,  false },
		{ "XEL-CF3: Swap",                   kATDeviceButton_XELCFSwap,             false, true,  false },
	};

	struct DevButtonAction { QAction *act; const DevButtonEntry *entry; };
	auto *devButtonActions = new std::vector<DevButtonAction>();
	devButtonActions->reserve(sizeof(kDevButtons)/sizeof(kDevButtons[0]));

	auto computeSupportMask = [sim = ctx.sim]() -> uint32 {
		uint32 mask = 0;
		for (IATDeviceButtons *p : sim->GetDeviceManager()->GetInterfaces<IATDeviceButtons>(false, false, false))
			mask |= p->GetSupportedButtons();
		return mask;
	};

	auto isDepressed = [sim = ctx.sim](ATDeviceButton btn) -> bool {
		for (IATDeviceButtons *p : sim->GetDeviceManager()->GetInterfaces<IATDeviceButtons>(false, false, false)) {
			if (p->IsButtonDepressed(btn))
				return true;
		}
		return false;
	};

	for (const auto& e : kDevButtons) {
		auto *act = consoleMenu->addAction(QObject::tr(e.label));
		if (e.checkable)
			act->setCheckable(true);
		QObject::connect(act, &QAction::triggered, [sim = ctx.sim, e]{
			auto& dm = *sim->GetDeviceManager();
			for (IATDeviceButtons *p : dm.GetInterfaces<IATDeviceButtons>(false, false, false)) {
				if (e.toggle) {
					p->ActivateButton(e.button, !p->IsButtonDepressed(e.button));
				} else {
					p->ActivateButton(e.button, true);
					if (e.sendRelease)
						p->ActivateButton(e.button, false);
				}
			}
		});
		devButtonActions->push_back({act, &e});
	}

	// On open, refresh enabled / checked state to reflect current devices.
	QObject::connect(consoleMenu, &QMenu::aboutToShow,
		[sim = ctx.sim, devButtonActions, computeSupportMask, isDepressed]{
			const uint32 mask = computeSupportMask();
			for (const auto& da : *devButtonActions) {
				const bool supported = (mask & (1u << da.entry->button)) != 0;
				da.act->setEnabled(supported);
				if (da.entry->checkable)
					da.act->setChecked(supported && isDepressed(da.entry->button));
			}
		});

	// Free the helper vector when the menu is destroyed so we don't leak.
	QObject::connect(consoleMenu, &QObject::destroyed, [devButtonActions]{
		delete devButtonActions;
	});
}

void buildInputMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *inputMenu = bar->addMenu(QObject::tr("&Input"));

	auto *captureAction = inputMenu->addAction(QObject::tr("Capture Mouse"));
	captureAction->setCheckable(true);
	captureAction->setShortcut(QKeySequence(Qt::Key_F12));
	if (ctx.displayWidget) {
		QObject::connect(captureAction, &QAction::toggled, [w = ctx.displayWidget](bool on){
			if (on) {
				w->setCursor(Qt::BlankCursor);
				w->setFocus();
				w->grabMouse();
			} else {
				w->releaseMouse();
				w->unsetCursor();
			}
		});
	} else {
		captureAction->setEnabled(false);
	}

	auto *autoCaptureAction = inputMenu->addAction(QObject::tr("Auto-Capture Mouse"));
	autoCaptureAction->setCheckable(true);
	if (ctx.displayWidget) {
		QObject::connect(autoCaptureAction, &QAction::toggled, [&ctx](bool on){
			static QMetaObject::Connection conn;
			QObject::disconnect(conn);
			if (on) {
				conn = QObject::connect(qApp, &QGuiApplication::applicationStateChanged,
					[w = ctx.displayWidget](Qt::ApplicationState st){
						if (st == Qt::ApplicationActive) {
							w->setCursor(Qt::BlankCursor);
							w->setFocus();
							w->grabMouse();
						} else if (st == Qt::ApplicationInactive) {
							w->releaseMouse();
							w->unsetCursor();
						}
					});
			}
			if (ctx.settings)
				ctx.settings->setValue(QStringLiteral("input/autoCaptureMouse"), on);
		});
		if (ctx.settings
		    && ctx.settings->value(QStringLiteral("input/autoCaptureMouse"), false).toBool()) {
			autoCaptureAction->setChecked(true);
		}
	} else {
		autoCaptureAction->setEnabled(false);
	}

	auto *lpAction = inputMenu->addAction(QObject::tr("Light Pen/Gun..."));
	QObject::connect(lpAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowLightPenDialog(ctx.window, ctx.sim);
	});

	auto *lpRecalAction = inputMenu->addAction(QObject::tr("Recalibrate Light Pen/Gun"));
	QObject::connect(lpRecalAction, &QAction::triggered, ctx.window, [&ctx]{
		ATRunLightPenRecalibrate(ctx.window, ctx.displayWidget, ctx.sim);
	});

	auto *joyKeysAction = inputMenu->addAction(QObject::tr("Joystick Keys..."));
	QObject::connect(joyKeysAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowJoystickKeysDialog(ctx.window, ctx.settings);
	});

	auto *kbdLayoutAction = inputMenu->addAction(QObject::tr("&Keyboard Layout..."));
	QObject::connect(kbdLayoutAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowKeyboardLayoutDialog(ctx.window, ctx.settings);
	});

	// Cycle Quick Maps — rotates among three saved snapshots of the
	// joystick + console mappings. Each snapshot lives under
	// `input/quickMap<N>/<atari-name>`. Activating snap M copies its
	// keys into the live `input/keyboard/...` and `input/joy*/...`
	// namespaces.
	auto *cycleAction = inputMenu->addAction(QObject::tr("&Cycle Quick Maps"));
	cycleAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab));
	QObject::connect(cycleAction, &QAction::triggered, ctx.window, [&ctx]{
		if (!ctx.settings) return;
		auto& s = *ctx.settings;
		const int cur = s.value(QStringLiteral("input/activeQuickMap"), 0).toInt();
		const int next = (cur + 1) % 3;

		auto save = [&s](int slot) {
			s.beginGroup(QStringLiteral("input/quickMap%1").arg(slot));
			for (const auto& sub : { "keyboard", "joy0", "joy1", "joy2", "joy3" }) {
				s.remove(QString::fromUtf8(sub));
				s.beginGroup(QString::fromUtf8(sub));
				QSettings live;
				live.beginGroup(QStringLiteral("input/") + QString::fromUtf8(sub));
				for (const auto& k : live.allKeys())
					s.setValue(k, live.value(k));
				live.endGroup();
				s.endGroup();
			}
			s.endGroup();
		};
		auto load = [&s](int slot) {
			s.beginGroup(QStringLiteral("input/quickMap%1").arg(slot));
			for (const auto& sub : { "keyboard", "joy0", "joy1", "joy2", "joy3" }) {
				s.beginGroup(QString::fromUtf8(sub));
				QSettings live;
				live.beginGroup(QStringLiteral("input/") + QString::fromUtf8(sub));
				live.remove(QString());
				for (const auto& k : s.allKeys())
					live.setValue(k, s.value(k));
				live.endGroup();
				s.endGroup();
			}
			s.endGroup();
		};

		// Snapshot the current live bindings to the current slot before
		// rotating, so user edits since the last cycle aren't lost.
		save(cur);
		load(next);
		s.setValue(QStringLiteral("input/activeQuickMap"), next);
		ATLoadJoyKeys(s);
		ctx.window->statusBar()->showMessage(
			QObject::tr("Quick map %1 active").arg(next + 1), 2000);
	});
}

void buildCheatMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *m = bar->addMenu(QObject::tr("C&heat"));

	auto *cheaterAction = m->addAction(QObject::tr("Cheater..."));
	QObject::connect(cheaterAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowCheaterDialog(ctx.window, ctx.sim);
	});

	m->addSeparator();

	auto *pmAction = m->addAction(QObject::tr("Disable P/M &Collisions"));
	pmAction->setCheckable(true);
	pmAction->setChecked(!ctx.sim->GetGTIA().ArePMCollisionsEnabled());
	QObject::connect(pmAction, &QAction::toggled, [sim = ctx.sim](bool disabled){
		sim->GetGTIA().SetPMCollisionsEnabled(!disabled);
	});

	auto *pfAction = m->addAction(QObject::tr("Disable &Playfield Collisions"));
	pfAction->setCheckable(true);
	pfAction->setChecked(!ctx.sim->GetGTIA().ArePFCollisionsEnabled());
	QObject::connect(pfAction, &QAction::toggled, [sim = ctx.sim](bool disabled){
		sim->GetGTIA().SetPFCollisionsEnabled(!disabled);
	});
}

void buildRecordMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *m = bar->addMenu(QObject::tr("&Record"));

	// All five recording flavours share a single mutually-exclusive slot —
	// only one can be active at a time. Each action holds its own
	// std::unique_ptr<T> alive via shared_ptr so the lambdas can move it
	// around without dangling.
	auto audioRec = std::make_shared<std::unique_ptr<ATAudioRecorder>>();
	auto rawRec   = std::make_shared<std::unique_ptr<ATAudioRecorder>>();
	auto videoRec = std::make_shared<std::unique_ptr<ATVideoRecorder>>();
	auto sapRec   = std::make_shared<std::unique_ptr<ATQtSapRecorder>>();
	auto vgmRec   = std::make_shared<std::unique_ptr<ATQtVgmRecorder>>();

	auto *recordAction = m->addAction(QObject::tr("Record &Audio..."));
	recordAction->setCheckable(true);

	auto *recordRawAction = m->addAction(QObject::tr("Record &Raw Audio..."));
	recordRawAction->setCheckable(true);

	auto *recordVideoAction = m->addAction(QObject::tr("Record &Video..."));
	recordVideoAction->setCheckable(true);

	auto *recordSapAction = m->addAction(QObject::tr("Record SAP type R..."));
	recordSapAction->setCheckable(true);

	auto *recordVgmAction = m->addAction(QObject::tr("Record V&GM..."));
	recordVgmAction->setCheckable(true);

	auto *pauseAction = m->addAction(QObject::tr("&Pause Recording"));
	pauseAction->setCheckable(true);
	pauseAction->setEnabled(false);

	// Set/clear the "any recording in progress" gate. Disables every
	// other recording action while one is running so they're mutually
	// exclusive.
	auto setExclusiveGate =
	    [recordAction, recordRawAction, recordVideoAction,
	     recordSapAction, recordVgmAction]
	    (QAction *active, bool busy) {
		QAction *all[] = { recordAction, recordRawAction, recordVideoAction,
		                   recordSapAction, recordVgmAction };
		for (QAction *a : all)
			if (a != active) a->setEnabled(!busy);
	};

	QObject::connect(recordAction, &QAction::triggered, ctx.window,
		[&ctx, recordAction, pauseAction, audioRec, setExclusiveGate](bool checked){
		if (checked) {
			// One-off modal: pick mono or stereo before the file dialog.
			QDialog optDlg(ctx.window);
			optDlg.setWindowTitle(QObject::tr("Record Audio"));
			auto *vlay = new QVBoxLayout(&optDlg);
			auto *flay = new QFormLayout;
			vlay->addLayout(flay);
			auto *channelsCombo = new QComboBox(&optDlg);
			channelsCombo->addItem(QObject::tr("Stereo"), QVariant(true));
			channelsCombo->addItem(QObject::tr("Mono"),   QVariant(false));
			channelsCombo->setCurrentIndex(0);
			flay->addRow(QObject::tr("Channels:"), channelsCombo);
			auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
				Qt::Horizontal, &optDlg);
			vlay->addWidget(box);
			QObject::connect(box, &QDialogButtonBox::accepted, &optDlg, &QDialog::accept);
			QObject::connect(box, &QDialogButtonBox::rejected, &optDlg, &QDialog::reject);
			if (optDlg.exec() != QDialog::Accepted) {
				recordAction->setChecked(false);
				return;
			}
			const bool stereo = channelsCombo->currentData().toBool();

			const QString path = QFileDialog::getSaveFileName(ctx.window,
				QObject::tr("Record audio to WAV"),
				QStringLiteral("altirra-audio.wav"),
				QObject::tr("WAV file (*.wav)"));
			if (path.isEmpty()) {
				recordAction->setChecked(false);
				return;
			}
			const auto status = ctx.sim->GetAudioOutput()->GetAudioStatus();
			const uint32 sr = (uint32)status.mSamplingRate;
			try {
				*audioRec = std::make_unique<ATAudioRecorder>(
					path.toUtf8().constData(), sr ? sr : 48000u, stereo);
			} catch (const std::exception& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Record failed"),
					QString::fromUtf8(e.what()));
				recordAction->setChecked(false);
				return;
			}
			ctx.sim->GetAudioOutput()->SetAudioTap(audioRec->get());
			pauseAction->setEnabled(true);
			pauseAction->setChecked(false);
			setExclusiveGate(recordAction, true);
		} else {
			ctx.sim->GetAudioOutput()->SetAudioTap(nullptr);
			if (*audioRec) (*audioRec)->Finalize();
			audioRec->reset();
			pauseAction->setEnabled(false);
			pauseAction->setChecked(false);
			setExclusiveGate(recordAction, false);
		}
	});

	QObject::connect(pauseAction, &QAction::toggled, [audioRec](bool on){
		if (!*audioRec) return;
		if (on) (*audioRec)->Pause();
		else    (*audioRec)->Resume();
	});

	// --- Record Raw Audio --------------------------------------------------
	// Same dialog as Record Audio plus a "container" toggle: WAV (sample-rate
	// only WAV header, no resampling/filtering — passthrough of the audio
	// output's float buffers as int16) or raw PCM (no header at all). Both
	// are 16-bit little-endian.
	QObject::connect(recordRawAction, &QAction::triggered, ctx.window,
		[&ctx, recordRawAction, rawRec, setExclusiveGate](bool checked){
		if (checked) {
			QDialog optDlg(ctx.window);
			optDlg.setWindowTitle(QObject::tr("Record Raw Audio"));
			auto *vlay = new QVBoxLayout(&optDlg);
			auto *flay = new QFormLayout;
			vlay->addLayout(flay);
			auto *channelsCombo = new QComboBox(&optDlg);
			channelsCombo->addItem(QObject::tr("Stereo"), QVariant(true));
			channelsCombo->addItem(QObject::tr("Mono"),   QVariant(false));
			channelsCombo->setCurrentIndex(0);
			flay->addRow(QObject::tr("Channels:"), channelsCombo);
			auto *containerCombo = new QComboBox(&optDlg);
			containerCombo->addItem(QObject::tr("Raw PCM (.pcm, no header)"), QVariant(false));
			containerCombo->addItem(QObject::tr("WAV (sample-rate only)"),    QVariant(true));
			containerCombo->setCurrentIndex(0);
			flay->addRow(QObject::tr("Container:"), containerCombo);
			auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
				Qt::Horizontal, &optDlg);
			vlay->addWidget(box);
			QObject::connect(box, &QDialogButtonBox::accepted, &optDlg, &QDialog::accept);
			QObject::connect(box, &QDialogButtonBox::rejected, &optDlg, &QDialog::reject);
			if (optDlg.exec() != QDialog::Accepted) {
				recordRawAction->setChecked(false);
				return;
			}
			const bool stereo      = channelsCombo->currentData().toBool();
			const bool writeHeader = containerCombo->currentData().toBool();

			const QString defaultName = writeHeader
				? QStringLiteral("altirra-audio-raw.wav")
				: QStringLiteral("altirra-audio.pcm");
			const QString filter = writeHeader
				? QObject::tr("WAV file (*.wav)")
				: QObject::tr("Raw PCM (*.pcm);;All files (*)");
			const QString path = QFileDialog::getSaveFileName(ctx.window,
				QObject::tr("Record raw audio"), defaultName, filter);
			if (path.isEmpty()) {
				recordRawAction->setChecked(false);
				return;
			}
			const auto status = ctx.sim->GetAudioOutput()->GetAudioStatus();
			const uint32 sr = (uint32)status.mSamplingRate;
			try {
				*rawRec = std::make_unique<ATAudioRecorder>(
					path.toUtf8().constData(), sr ? sr : 48000u,
					stereo, writeHeader);
			} catch (const std::exception& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Record failed"),
					QString::fromUtf8(e.what()));
				recordRawAction->setChecked(false);
				return;
			}
			ctx.sim->GetAudioOutput()->SetAudioTap(rawRec->get());
			setExclusiveGate(recordRawAction, true);
		} else {
			ctx.sim->GetAudioOutput()->SetAudioTap(nullptr);
			if (*rawRec) (*rawRec)->Finalize();
			rawRec->reset();
			setExclusiveGate(recordRawAction, false);
		}
	});

	// --- Record Video ------------------------------------------------------
	QObject::connect(recordVideoAction, &QAction::triggered, ctx.window,
		[&ctx, recordVideoAction, videoRec, setExclusiveGate](bool checked){
		if (checked) {
			if (!ctx.displayWidget) {
				QMessageBox::warning(ctx.window, QObject::tr("Record failed"),
					QObject::tr("No display widget available."));
				recordVideoAction->setChecked(false);
				return;
			}
			QDialog optDlg(ctx.window);
			optDlg.setWindowTitle(QObject::tr("Record Video"));
			auto *vlay = new QVBoxLayout(&optDlg);
			auto *flay = new QFormLayout;
			vlay->addLayout(flay);
			auto *codecCombo = new QComboBox(&optDlg);
			codecCombo->addItem(QObject::tr("Uncompressed (BI_RGB)"), QVariant((int)0));
			codecCombo->addItem(QObject::tr("MJPEG (smaller)"),       QVariant((int)1));
			codecCombo->setCurrentIndex(1);
			flay->addRow(QObject::tr("Codec:"), codecCombo);
			auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
				Qt::Horizontal, &optDlg);
			vlay->addWidget(box);
			QObject::connect(box, &QDialogButtonBox::accepted, &optDlg, &QDialog::accept);
			QObject::connect(box, &QDialogButtonBox::rejected, &optDlg, &QDialog::reject);
			if (optDlg.exec() != QDialog::Accepted) {
				recordVideoAction->setChecked(false);
				return;
			}
			const auto codec = (codecCombo->currentData().toInt() == 1)
				? ATVideoRecorder::Codec::MJPEG
				: ATVideoRecorder::Codec::Uncompressed;

			const QString path = QFileDialog::getSaveFileName(ctx.window,
				QObject::tr("Record video to AVI"),
				QStringLiteral("altirra-video.avi"),
				QObject::tr("AVI file (*.avi)"));
			if (path.isEmpty()) {
				recordVideoAction->setChecked(false);
				return;
			}

			// Compute frame rate from the simulator's video standard.
			const ATVideoStandard vs = ctx.sim->GetVideoStandard();
			const double fps = (vs == kATVideoStandard_NTSC) ? 59.9227
			                : (vs == kATVideoStandard_PAL60) ? 60.0
			                : 49.8607;

			// Use the current display widget size (host pixels) as the
			// recording resolution. grabFramebuffer() returns this size.
			const QSize size = ctx.displayWidget->size();
			int W = std::max(64, size.width());
			int H = std::max(64, size.height());

			try {
				*videoRec = std::make_unique<ATVideoRecorder>(
					path.toUtf8().constData(), W, H, fps, codec);
			} catch (const std::exception& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Record failed"),
					QString::fromUtf8(e.what()));
				recordVideoAction->setChecked(false);
				return;
			}
			// Install the frame callback. Capture by raw pointer to avoid
			// shared_ptr<unique_ptr<T>> overhead per-frame; the toggle path
			// clears the recorder pointer before destroying it, then
			// uninstalls the callback.
			auto *rec = videoRec->get();
			ATQtVideoDisplaySetFrameRecorder(ctx.displayWidget,
				[rec](const QImage& img) {
					if (rec && rec->IsOpen()) rec->WriteFrame(img);
				});
			setExclusiveGate(recordVideoAction, true);
		} else {
			ATQtVideoDisplaySetFrameRecorder(ctx.displayWidget, {});
			if (*videoRec) (*videoRec)->Finalize();
			videoRec->reset();
			setExclusiveGate(recordVideoAction, false);
		}
	});

	// --- Record SAP type R --------------------------------------------------
	QObject::connect(recordSapAction, &QAction::triggered, ctx.window,
		[&ctx, recordSapAction, sapRec, setExclusiveGate](bool checked){
		if (checked) {
			const QString path = QFileDialog::getSaveFileName(ctx.window,
				QObject::tr("Record SAP type R"),
				QStringLiteral("altirra-pokey.sap"),
				QObject::tr("SAP file (*.sap)"));
			if (path.isEmpty()) {
				recordSapAction->setChecked(false);
				return;
			}
			try {
				*sapRec = std::make_unique<ATQtSapRecorder>(
					*ctx.sim, path.toUtf8().constData());
			} catch (const std::exception& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Record failed"),
					QString::fromUtf8(e.what()));
				recordSapAction->setChecked(false);
				return;
			}
			setExclusiveGate(recordSapAction, true);
		} else {
			sapRec->reset();
			setExclusiveGate(recordSapAction, false);
		}
	});

	// --- Record VGM ---------------------------------------------------------
	QObject::connect(recordVgmAction, &QAction::triggered, ctx.window,
		[&ctx, recordVgmAction, vgmRec, setExclusiveGate](bool checked){
		if (checked) {
			const QString path = QFileDialog::getSaveFileName(ctx.window,
				QObject::tr("Record VGM"),
				QStringLiteral("altirra-pokey.vgm"),
				QObject::tr("VGM file (*.vgm)"));
			if (path.isEmpty()) {
				recordVgmAction->setChecked(false);
				return;
			}
			try {
				*vgmRec = std::make_unique<ATQtVgmRecorder>(
					*ctx.sim, path.toUtf8().constData());
			} catch (const std::exception& e) {
				QMessageBox::warning(ctx.window, QObject::tr("Record failed"),
					QString::fromUtf8(e.what()));
				recordVgmAction->setChecked(false);
				return;
			}
			setExclusiveGate(recordVgmAction, true);
		} else {
			vgmRec->reset();
			setExclusiveGate(recordVgmAction, false);
		}
	});
}

void buildDebugMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *m = bar->addMenu(QObject::tr("&Debug"));

	auto *windowMenu = m->addMenu(QObject::tr("&Window"));

	auto *consoleAct = windowMenu->addAction(QObject::tr("&Console"));
	consoleAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
	QObject::connect(consoleAct, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDebuggerConsolePane(ctx.window);
	});

	auto *registersAct = windowMenu->addAction(QObject::tr("&Registers"));
	registersAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
	QObject::connect(registersAct, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDebuggerRegistersPane(ctx.window, ctx.sim);
	});

	auto *disasmAct = windowMenu->addAction(QObject::tr("D&isassembly"));
	disasmAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));
	QObject::connect(disasmAct, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDebuggerDisassemblyPane(ctx.window, ctx.sim);
	});

	auto *memoryAct = windowMenu->addAction(QObject::tr("&Memory"));
	memoryAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_4));
	QObject::connect(memoryAct, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDebuggerMemoryPane(ctx.window, ctx.sim);
	});

	auto *watchAct = windowMenu->addAction(QObject::tr("&Watch"));
	watchAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_5));
	QObject::connect(watchAct, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDebuggerWatchPane(ctx.window, ctx.sim);
	});

	auto *bpAct = windowMenu->addAction(QObject::tr("&Breakpoints"));
	bpAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_6));
	QObject::connect(bpAct, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDebuggerBreakpointsPane(ctx.window, ctx.sim);
	});

	auto *histAct = windowMenu->addAction(QObject::tr("&History"));
	histAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_7));
	QObject::connect(histAct, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDebuggerHistoryPane(ctx.window, ctx.sim);
	});

	m->addSeparator();

	// Run/Break: if currently running, Break(); else Run().
	auto *runBreakAct = m->addAction(QObject::tr("Run / Break"));
	runBreakAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F5));
	QObject::connect(runBreakAct, &QAction::triggered, ctx.window, []{
		IATDebugger *dbg = ATGetDebugger();
		if (!dbg) return;
		if (dbg->IsRunning())
			dbg->Break();
		else
			dbg->Run(kATDebugSrcMode_Same);
	});

	auto *stepIntoAct = m->addAction(QObject::tr("Step &Into"));
	stepIntoAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F10));
	QObject::connect(stepIntoAct, &QAction::triggered, ctx.window, []{
		IATDebugger *dbg = ATGetDebugger();
		if (dbg) dbg->StepInto(kATDebugSrcMode_Same);
	});

	auto *stepOverAct = m->addAction(QObject::tr("Step &Over"));
	stepOverAct->setShortcut(QKeySequence(Qt::Key_F10));
	QObject::connect(stepOverAct, &QAction::triggered, ctx.window, []{
		IATDebugger *dbg = ATGetDebugger();
		if (dbg) dbg->StepOver(kATDebugSrcMode_Same);
	});

	auto *stepOutAct = m->addAction(QObject::tr("Step O&ut"));
	stepOutAct->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F10));
	QObject::connect(stepOutAct, &QAction::triggered, ctx.window, []{
		IATDebugger *dbg = ATGetDebugger();
		if (dbg) dbg->StepOut(kATDebugSrcMode_Same);
	});

	m->addSeparator();

	// Step Source Line — iterate StepInto until ATSourceLineMap reports a
	// different line. Useful for source-level debugging once symbols /
	// line tables are loaded.
	auto *stepSrcAct = m->addAction(QObject::tr("Step Sou&rce Line"));
	stepSrcAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F10));
	QObject::connect(stepSrcAct, &QAction::triggered, ctx.window, []{
		IATDebugger *dbg = ATGetDebugger();
		if (dbg) dbg->StepInto(kATDebugSrcMode_Source);
	});

	m->addSeparator();

	// Symbol load: route .lst/.lab/.dbg through IATDebugger::LoadSymbols.
	// Persists per-image so the next mount auto-reloads (the symbol set is
	// keyed by the most recent image's SHA-256).
	auto *loadSymsAct = m->addAction(QObject::tr("&Load Symbols..."));
	QObject::connect(loadSymsAct, &QAction::triggered, ctx.window, [&ctx]{
		const QString path = QFileDialog::getOpenFileName(ctx.window,
			QObject::tr("Load symbols"), {},
			QObject::tr("Symbol files (*.lst *.lab *.dbg *.sym);;All files (*)"));
		if (path.isEmpty()) return;
		IATDebugger *dbg = ATGetDebugger();
		if (!dbg) return;
		const std::wstring w = path.toStdWString();
		try {
			(void)dbg->LoadSymbols(w.c_str(), true, nullptr, true);
		} catch (const MyError& e) {
			QMessageBox::warning(ctx.window, QObject::tr("Load failed"),
				QString::fromUtf8(e.c_str()));
			return;
		}
		// Remember per-image so a future mount reloads the same symbols.
		auto *primary = ctx.sim->GetCartridge(0);
		IATImage *img = primary ? primary->GetImage() : nullptr;
		if (!img) {
			if (auto *disk = ctx.sim->GetDiskInterface(0).GetDiskImage())
				img = disk;
		}
		if (img && ctx.settings) {
			if (auto sha = img->GetImageFileSHA256()) {
				QString key;
				key.reserve(64);
				static const char *hex = "0123456789abcdef";
				for (int i = 0; i < 32; ++i) {
					key.append(QChar::fromLatin1(hex[(sha->mDigest[i] >> 4) & 0xF]));
					key.append(QChar::fromLatin1(hex[sha->mDigest[i] & 0xF]));
				}
				ctx.settings->setValue(
					QStringLiteral("debugger/symbols/") + key, path);
			}
		}
		ctx.window->statusBar()->showMessage(
			QObject::tr("Symbols loaded: %1")
				.arg(QFileInfo(path).fileName()), 2500);
	});

	auto *clearSymsAct = m->addAction(QObject::tr("Clea&r Custom Symbols"));
	QObject::connect(clearSymsAct, &QAction::triggered, ctx.window, [&ctx]{
		IATDebugger *dbg = ATGetDebugger();
		if (!dbg) return;
		// Walk the symbol-store list and unload everything except the
		// kernel symbols the simulator auto-loads. UnloadSymbols(0) is
		// permitted but harmless — it nukes the dynamic table.
		for (uint32 id = 1; id < 64; ++id) dbg->UnloadSymbols(id);
		ctx.window->statusBar()->showMessage(
			QObject::tr("Symbol modules unloaded"), 2000);
	});

	// Source files dock — see debuggersourcepane.cpp.
	auto *sourcePaneAct = windowMenu->addAction(QObject::tr("&Source Files"));
	sourcePaneAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_8));
	QObject::connect(sourcePaneAct, &QAction::triggered, ctx.window, [&ctx]{
		ATShowDebuggerSourcePane(ctx.window, ctx.sim);
	});
}

void buildToolsMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *m = bar->addMenu(QObject::tr("&Tools"));

	auto *firstTimeAction = m->addAction(QObject::tr("First Time Setup..."));
	QObject::connect(firstTimeAction, &QAction::triggered, ctx.window, [&ctx]{
		if (ATRunFirstTimeWizard(ctx.window, ctx.settings)) {
			// Apply the wizard's choices to the running simulator and reset
			// so the new hardware mode / video standard / BASIC state takes
			// effect immediately. Then rebuild the menu bar so checkmarks
			// reflect the new settings.
			ATApplyAllSettings(ctx);
			ctx.sim->ColdReset();
			ATBuildAltirraMenuBar(ctx);
			ctx.window->statusBar()->showMessage(
				QObject::tr("First Time Setup applied"), 2500);
		}
	});

	auto *advancedAction = m->addAction(QObject::tr("Advanced Configuration..."));
	QObject::connect(advancedAction, &QAction::triggered, ctx.window, [&ctx]{
		ATShowAdvancedConfigDialog(ctx.window, ctx.sim, ctx.settings);
	});

	m->addSeparator();

	auto *diskExpAction = m->addAction(QObject::tr("&Disk Explorer..."));
	QObject::connect(diskExpAction, &QAction::triggered, ctx.window, [w = ctx.window]{
		ATShowDiskExplorerDialog(w);
	});

	auto *sapToExeAction = m->addAction(QObject::tr("Convert SAP to EXE..."));
	QObject::connect(sapToExeAction, &QAction::triggered, ctx.window, [w = ctx.window]{
		const QString inputPath = QFileDialog::getOpenFileName(w,
			QObject::tr("Select SAP file to convert"), {},
			QObject::tr("SAP files (*.sap);;All files (*)"));
		if (inputPath.isEmpty()) return;

		const QString outputPath = QFileDialog::getSaveFileName(w,
			QObject::tr("Save converted Atari executable"),
			QFileInfo(inputPath).completeBaseName() + QStringLiteral(".xex"),
			QObject::tr("Atari executable (*.xex);;All files (*)"));
		if (outputPath.isEmpty()) return;

		const std::wstring inW  = inputPath.toStdWString();
		const std::wstring outW = outputPath.toStdWString();
		try {
			ATConvertSAPToPlayer(outW.c_str(), inW.c_str());
		} catch (const MyError& e) {
			QMessageBox::warning(w, QObject::tr("Convert failed"),
				QString::fromUtf8(e.c_str()));
			return;
		}
		w->statusBar()->showMessage(
			QObject::tr("Converted SAP to %1")
				.arg(QFileInfo(outputPath).fileName()), 2500);
	});

	auto *exportRomAction = m->addAction(QObject::tr("Export ROM set..."));
	QObject::connect(exportRomAction, &QAction::triggered, ctx.window, [w = ctx.window]{
		const QString dir = QFileDialog::getExistingDirectory(w,
			QObject::tr("Choose target folder for ROM export"));
		if (dir.isEmpty()) return;
		struct Rom { const char *res; const char *out; };
		static constexpr Rom kRoms[] = {
			{":/altirra/roms/kernel.rom",     "altirraos-800.rom"},
			{":/altirra/roms/kernelxl.rom",   "altirraos-xl.rom"},
			{":/altirra/roms/kernel816.rom",  "altirraos-816.rom"},
			{":/altirra/roms/atbasic.bin",    "atbasic.rom"},
		};
		for (const auto& r : kRoms) {
			QFile in(QString::fromUtf8(r.res));
			if (!in.open(QIODevice::ReadOnly)) continue;
			const QByteArray data = in.readAll();
			QFile out(dir + QStringLiteral("/") + QString::fromUtf8(r.out));
			if (!out.open(QIODevice::WriteOnly)) {
				QMessageBox::warning(w, QObject::tr("Export failed"),
					QObject::tr("Could not write %1.").arg(QString::fromUtf8(r.out)));
				return;
			}
			out.write(data);
		}
		QMessageBox::information(w, QObject::tr("Export ROM set"),
			QObject::tr("ROM set successfully exported."));
	});

	m->addSeparator();

	// Compatibility database: load external .atcptab, toggle auto-apply,
	// and reset per-image opt-outs.
	auto *compatMenu = m->addMenu(QObject::tr("&Compatibility"));
	auto *compatLoadAction = compatMenu->addAction(
		QObject::tr("Load Database..."));
	QObject::connect(compatLoadAction, &QAction::triggered, ctx.window,
		[w = ctx.window, &ctx]{
			const QString path = QFileDialog::getOpenFileName(w,
				QObject::tr("Load compatibility database"), {},
				QObject::tr("Compat database (*.atcptab);;All files (*)"));
			if (path.isEmpty()) return;
			if (!ATCompatLoadDatabase(path)) {
				QMessageBox::warning(w, QObject::tr("Load failed"),
					QObject::tr("File is not a valid Altirra compat database: %1")
						.arg(path));
				return;
			}
			w->statusBar()->showMessage(
				QObject::tr("Compat database loaded: %1")
					.arg(QFileInfo(path).fileName()), 2500);
		});

	auto *compatUnloadAction = compatMenu->addAction(
		QObject::tr("Unload Database"));
	QObject::connect(compatUnloadAction, &QAction::triggered, ctx.window,
		[w = ctx.window]{
			ATCompatLoadDatabase(QString());
			w->statusBar()->showMessage(
				QObject::tr("Compat database unloaded"), 2000);
		});

	compatMenu->addSeparator();

	auto *compatAutoAction = compatMenu->addAction(
		QObject::tr("Auto-Apply on Mount"));
	compatAutoAction->setCheckable(true);
	compatAutoAction->setChecked(ATCompatIsAutoApplyEnabled());
	QObject::connect(compatAutoAction, &QAction::toggled, [](bool on){
		ATCompatSetAutoApplyEnabled(on);
	});

	auto *compatViewAction = compatMenu->addAction(
		QObject::tr("View Database..."));
	QObject::connect(compatViewAction, &QAction::triggered, ctx.window,
		[w = ctx.window]{ ATShowCompatDatabaseDialog(w); });

	// Auto-restore last database path on startup.
	if (ctx.settings) {
		const QString saved = ctx.settings->value(QStringLiteral("compat/dbPath"))
		                               .toString();
		if (!saved.isEmpty() && ATCompatGetDatabasePath().isEmpty())
			ATCompatLoadDatabase(saved);
	}

	m->addSeparator();

	auto *kbAction = m->addAction(QObject::tr("&Keyboard Shortcuts..."));
	QObject::connect(kbAction, &QAction::triggered, ctx.window, [w = ctx.window]{
		QMessageBox::information(w, QObject::tr("Keyboard Shortcuts"),
			QObject::tr(
				"<h3>Keyboard Shortcuts</h3>"
				"<table cellpadding='2'>"
				"<tr><td><b>F5</b></td><td>Warm Reset</td></tr>"
				"<tr><td><b>Shift+F5</b></td><td>Cold Reset</td></tr>"
				"<tr><td><b>F7</b></td><td>Save State...</td></tr>"
				"<tr><td><b>Shift+F7</b></td><td>Load State...</td></tr>"
				"<tr><td><b>F8</b></td><td>Warp Speed</td></tr>"
				"<tr><td><b>F9</b></td><td>Pause</td></tr>"
				"<tr><td><b>F11</b></td><td>Quick Save / Full Screen</td></tr>"
				"<tr><td><b>Shift+F11</b></td><td>Quick Load</td></tr>"
				"<tr><td><b>F12</b></td><td>Capture Mouse</td></tr>"
				"<tr><td><b>Ctrl+B</b></td><td>Boot Image...</td></tr>"
				"<tr><td><b>Ctrl+Q</b></td><td>Exit</td></tr>"
				"<tr><td><b>Ctrl+W</b></td><td>Close Window</td></tr>"
				"</table>"
				"<p>Joystick keys: <b>Arrow keys</b> + <b>Left Alt</b> "
				"(joystick on port 1).</p>"
				"<p>Console keys: <b>F2</b>=Start, <b>F3</b>=Select, "
				"<b>F4</b>=Option.</p>"));
	});
}

void buildWindowMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *m = bar->addMenu(QObject::tr("&Window"));

	// Fit Image — resize the window so the display widget shows the visible
	// frame at an integer multiple. Uses the current overscan width (in
	// color clocks * 2 = pixels) and a fixed 240-line height; the window
	// chrome around the display widget (menu bar, status bar, frame) is
	// added back to the target size.
	auto *fitMenu = m->addMenu(QObject::tr("Fit Image"));
	for (int n : {1, 2, 3, 4}) {
		auto *act = fitMenu->addAction(QObject::tr("%1x").arg(n));
		QObject::connect(act, &QAction::triggered, ctx.window,
			[win = ctx.window, w = ctx.displayWidget, sim = ctx.sim, n]{
			if (!w) return;
			int srcW = 336, srcH = 240;
			switch (sim->GetGTIA().GetOverscanMode()) {
				case ATGTIAEmulator::kOverscanOSScreen:   srcW = 320; break;
				case ATGTIAEmulator::kOverscanNormal:     srcW = 336; break;
				case ATGTIAEmulator::kOverscanWidescreen: srcW = 352; break;
				case ATGTIAEmulator::kOverscanExtended:   srcW = 384; break;
				case ATGTIAEmulator::kOverscanFull:       srcW = 456; break;
				default: break;
			}
			const QSize chrome = win->size() - w->size();
			win->resize(srcW * n + chrome.width(), srcH * n + chrome.height());
		});
	}

	m->addSeparator();

	auto *closeAction = m->addAction(QObject::tr("&Close"));
	closeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
	QObject::connect(closeAction, &QAction::triggered, ctx.window, [w = ctx.window]{
		w->close();
	});
}

void buildHelpMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *m = bar->addMenu(QObject::tr("Help"));

	auto *contentsAction = m->addAction(QObject::tr("&Contents"));
	contentsAction->setShortcut(QKeySequence::HelpContents);
	QObject::connect(contentsAction, &QAction::triggered, ctx.window,
		[w = ctx.window]{ ATShowHelpContents(w); });

	auto *changelogAction = m->addAction(QObject::tr("Change &Log"));
	QObject::connect(changelogAction, &QAction::triggered, ctx.window,
		[w = ctx.window]{ ATShowHelpChangeLog(w); });

	auto *checkUpdAction = m->addAction(QObject::tr("Check for &Updates..."));
	QObject::connect(checkUpdAction, &QAction::triggered, ctx.window,
		[w = ctx.window]{ ATShowHelpCheckForUpdates(w); });

	m->addSeparator();

	// Command-Line Help — opens a non-modal dialog with the actual option
	// reference. Keep this text in sync with the QCommandLineParser setup
	// in main.cpp.
	auto *cmdHelpAction = m->addAction(QObject::tr("Command-Line Help"));
	QObject::connect(cmdHelpAction, &QAction::triggered, ctx.window, [w = ctx.window]{
		auto *dlg = new QDialog(w);
		dlg->setAttribute(Qt::WA_DeleteOnClose);
		dlg->setWindowTitle(QObject::tr("Command-Line Help"));
		dlg->resize(640, 480);

		auto *edit = new QPlainTextEdit(dlg);
		edit->setReadOnly(true);
		QFont mono(QStringLiteral("monospace"));
		mono.setStyleHint(QFont::TypeWriter);
		edit->setFont(mono);
		edit->setPlainText(QStringLiteral(
			"Usage: altirraqt [OPTIONS] [image]\n"
			"\n"
			"Altirra Atari emulator (Qt port, work-in-progress)\n"
			"\n"
			"Options:\n"
			"  -h, --help                   Display help on commandline options.\n"
			"      --help-all               Display help including Qt-specific options.\n"
			"  -v, --version                Display version information.\n"
			"\n"
			"      --kernel <file>          Path to an Atari kernel ROM (Atari 800XL OS,\n"
			"                               Atari 800 OS-B, etc.). Overrides the bundled\n"
			"                               AltirraOS for the current hardware mode.\n"
			"      --basic <file>           Path to an Atari BASIC ROM. Overrides the\n"
			"                               bundled Altirra BASIC.\n"
			"      --image <file>           Mount <file> as a disk/cassette/cartridge —\n"
			"                               Altirra's auto-detect figures out which from\n"
			"                               the contents.\n"
			"      --type <text>            Synthesise a sequence of key presses (each\n"
			"                               character of <text>) into the running\n"
			"                               emulator after a one-second startup pause.\n"
			"                               Useful for headless smoke-testing keyboard\n"
			"                               input.\n"
			"      --screenshot <s:path>    After <seconds> of run time, save a\n"
			"                               screenshot of the display widget to <path>,\n"
			"                               then quit. Argument format: \"seconds:path\".\n"
			"      --save-state-at <s:path> After <seconds>, save the current emulator\n"
			"                               state to <path>. Useful for headless\n"
			"                               smoke-testing. Argument format:\n"
			"                               \"seconds:path\".\n"
			"      --load-state <file>      Load <file> as a save state at startup\n"
			"                               (after kernel ROMs are registered, before\n"
			"                               ColdReset).\n"
		));

		auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
		QObject::connect(btnBox, &QDialogButtonBox::rejected, dlg, &QDialog::close);
		QObject::connect(btnBox, &QDialogButtonBox::accepted, dlg, &QDialog::close);

		auto *layout = new QVBoxLayout(dlg);
		layout->addWidget(edit);
		layout->addWidget(btnBox);

		dlg->show();
	});

	auto *homeAction = m->addAction(QObject::tr("Altirra Home..."));
	QObject::connect(homeAction, &QAction::triggered, []{
		QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.virtualdub.org/altirra.html")));
	});

	auto *aboutAction = m->addAction(QObject::tr("&About"));
	QObject::connect(aboutAction, &QAction::triggered, ctx.window, [w = ctx.window]{
		QMessageBox::about(w, QObject::tr("About altirraqt"),
			QObject::tr(
				"<h3>altirraqt</h3>"
				"<p>Qt port of <a href=\"https://www.virtualdub.org/altirra.html\">Altirra</a> "
				"by Avery Lee — an Atari 8-bit emulator.</p>"
				"<p>Bundled BSD-licensed kernel and BASIC ROMs (AltirraOS, Altirra BASIC) "
				"by Avery Lee, cross-assembled at build time with Tomasz Biela's MADS.</p>"
			));
	});
}

} // namespace

namespace {

void buildEditMenu(QMenuBar *bar, ATAltirraMenuContext& ctx) {
	auto *m = bar->addMenu(QObject::tr("&Edit"));

	auto *pasteAct = m->addAction(QObject::tr("&Paste Text"));
	pasteAct->setShortcut(QKeySequence::Paste);
	QObject::connect(pasteAct, &QAction::triggered, ctx.window, [&ctx]{
		const QString text = QApplication::clipboard()->text();
		if (text.isEmpty()) return;
		// Type each char through PushKey at ~80 ms intervals so POKEY's
		// scan timing has room to drain the queue.
		auto *typer = new QTimer(ctx.window);
		typer->setInterval(80);
		auto idx = std::make_shared<int>(0);
		QObject::connect(typer, &QTimer::timeout, ctx.window,
			[text, idx, typer, sim = ctx.sim]{
				if (*idx >= text.size()) {
					typer->stop();
					typer->deleteLater();
					return;
				}
				const QChar ch = text[*idx];
				++*idx;
				int qtKey = ch.toUpper().unicode();
				bool shift = ch.isLetter() && ch.isUpper();
				if (ch == QChar(' '))  qtKey = Qt::Key_Space;
				if (ch == QChar('\n')) qtKey = Qt::Key_Return;
				if (ch == QChar('\t')) qtKey = Qt::Key_Tab;
				const int sc = atariScanCodeFromQtKey(qtKey, shift);
				if (sc < 0) return;
				sim->GetPokey().PushKey((uint8)sc, /*repeat=*/false,
					/*allowQueue=*/true, /*flushQueue=*/false,
					/*useCooldown=*/true);
			});
		typer->start();
	});

	m->addSeparator();

	struct CopyOpt { const char *label; ATScreenCopyMode mode; };
	static constexpr CopyOpt kCopy[] = {
		{"Copy Frame as &Text",            ATScreenCopyMode::ASCII},
		{"Copy Frame as &Hex",             ATScreenCopyMode::Hex},
		{"Copy Frame as &Escaped ATASCII", ATScreenCopyMode::Escaped},
		{"Copy Frame as &Unicode",         ATScreenCopyMode::Unicode},
	};
	for (const auto& c : kCopy) {
		auto *act = m->addAction(QObject::tr(c.label));
		QObject::connect(act, &QAction::triggered, ctx.window,
			[w = ctx.window, sim = ctx.sim, mode = c.mode]{
				const QString out = ATGrabScreenText(*sim, mode);
				if (out.isEmpty()) {
					w->statusBar()->showMessage(
						QObject::tr("No character-mode text on screen."), 2000);
					return;
				}
				QApplication::clipboard()->setText(out);
				w->statusBar()->showMessage(
					QObject::tr("Frame copied (%1 chars)").arg(out.size()), 2000);
			});
	}
}

} // namespace

void ATBuildAltirraMenuBar(ATAltirraMenuContext& ctx) {
	auto *bar = ctx.window->menuBar();
	bar->clear();

	buildFileMenu(bar, ctx);
	buildEditMenu(bar, ctx);
	buildViewMenu(bar, ctx);
	buildSystemMenu(bar, ctx);
	buildInputMenu(bar, ctx);
	buildCheatMenu(bar, ctx);
	buildDebugMenu(bar, ctx);
	buildRecordMenu(bar, ctx);
	buildToolsMenu(bar, ctx);
	buildWindowMenu(bar, ctx);
	buildHelpMenu(bar, ctx);
}
