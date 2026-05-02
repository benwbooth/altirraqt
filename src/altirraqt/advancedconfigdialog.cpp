//	Altirra - Qt port
//	Advanced Configuration dialog implementation. Live-applies each
//	change as the user edits it, matching the Audio Levels dialog style.

#include "advancedconfigdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <at/atio/cassetteimage.h>
#include <at/ataudio/audiooutput.h>
#include <cassette.h>
#include <cpu.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

struct DecodeOpt { ATCassetteTurboDecodeAlgorithm v; const char *label; };
constexpr DecodeOpt kDecodeOpts[] = {
	{ATCassetteTurboDecodeAlgorithm::SlopeNoFilter,           "Slope (no filter)"},
	{ATCassetteTurboDecodeAlgorithm::SlopeFilter,             "Slope (filter)"},
	{ATCassetteTurboDecodeAlgorithm::PeakFilter,              "Peak (filter)"},
	{ATCassetteTurboDecodeAlgorithm::PeakFilterBalanceLoHi,   "Peak (filter, balance Lo-Hi)"},
	{ATCassetteTurboDecodeAlgorithm::PeakFilterBalanceHiLo,   "Peak (filter, balance Hi-Lo)"},
};

class ATAdvancedConfigDialog : public QDialog {
public:
	ATAdvancedConfigDialog(QWidget *parent, ATSimulator *sim, QSettings *settings)
		: QDialog(parent), mpSim(sim), mpSettings(settings)
	{
		setWindowTitle(tr("Advanced Configuration"));
		resize(540, 380);

		auto *layout = new QVBoxLayout(this);
		auto *tabs = new QTabWidget(this);
		layout->addWidget(tabs, 1);

		tabs->addTab(buildEngineTab(),   tr("Engine"));
		tabs->addTab(buildAudioTab(),    tr("Audio"));
		tabs->addTab(buildDisplayTab(),  tr("Display"));
		tabs->addTab(buildCassetteTab(), tr("Cassette"));

		auto *box = new QDialogButtonBox(QDialogButtonBox::Close, this);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::accept);
		layout->addWidget(box);
	}

private:
	QWidget *buildEngineTab() {
		auto *page = new QWidget;
		auto *form = new QFormLayout(page);

		auto& cpu = mpSim->GetCPU();

		// CPU history buffer is a single bool (fixed-size ring); enabling it
		// gives the History debugger pane real data. The "depth" combo from
		// the spec maps to nothing in the current core, so we expose just
		// the on/off knob.
		auto *historyBox = new QCheckBox(tr("Enable CPU history (for History debugger pane)"));
		historyBox->setChecked(cpu.IsHistoryEnabled());
		connect(historyBox, &QCheckBox::toggled, [this](bool on){
			mpSim->GetCPU().SetHistoryEnabled(on);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("engine/historyEnabled"), on);
		});
		form->addRow(QString(), historyBox);

		// Stop-on-BRK is wired through the CPU API and is the kind of
		// "advanced" toggle that fits on this tab.
		auto *stopBrkBox = new QCheckBox(tr("Stop on BRK instruction"));
		stopBrkBox->setChecked(cpu.GetStopOnBRK());
		connect(stopBrkBox, &QCheckBox::toggled, [this](bool on){
			mpSim->GetCPU().SetStopOnBRK(on);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("engine/stopOnBrk"), on);
		});
		form->addRow(QString(), stopBrkBox);

		auto *illegalBox = new QCheckBox(tr("Allow illegal 6502 instructions"));
		illegalBox->setChecked(cpu.AreIllegalInsnsEnabled());
		connect(illegalBox, &QCheckBox::toggled, [this](bool on){
			mpSim->GetCPU().SetIllegalInsnsEnabled(on);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("engine/illegalInsns"), on);
		});
		form->addRow(QString(), illegalBox);

		auto *nmiBlockBox = new QCheckBox(tr("Allow NMI blocking by 3-cycle BRK"));
		nmiBlockBox->setChecked(cpu.IsNMIBlockingEnabled());
		connect(nmiBlockBox, &QCheckBox::toggled, [this](bool on){
			mpSim->GetCPU().SetNMIBlockingEnabled(on);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("engine/nmiBlocking"), on);
		});
		form->addRow(QString(), nmiBlockBox);

		return page;
	}

	QWidget *buildAudioTab() {
		auto *page = new QWidget;
		auto *form = new QFormLayout(page);

		auto *out = mpSim->GetAudioOutput();

		// SetFiltersEnabled has no public getter; persist & apply via
		// QSettings, defaulting to "on" (matches the upstream default).
		const bool filtersOn = mpSettings
			? mpSettings->value(QStringLiteral("audio/filtersEnabled"), true).toBool()
			: true;
		out->SetFiltersEnabled(filtersOn);

		auto *filtersBox = new QCheckBox(tr("Audio filters enabled"));
		filtersBox->setChecked(filtersOn);
		connect(filtersBox, &QCheckBox::toggled, [this](bool on){
			mpSim->GetAudioOutput()->SetFiltersEnabled(on);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("audio/filtersEnabled"), on);
		});
		form->addRow(QString(), filtersBox);

		auto *latencyBox = new QSpinBox;
		latencyBox->setRange(10, 200);
		latencyBox->setSuffix(tr(" ms"));
		latencyBox->setValue(out->GetLatency());
		connect(latencyBox, qOverload<int>(&QSpinBox::valueChanged), [this](int ms){
			mpSim->GetAudioOutput()->SetLatency(ms);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("audio/latencyMs"), ms);
		});
		form->addRow(tr("Output latency:"), latencyBox);

		auto *extraBox = new QSpinBox;
		extraBox->setRange(0, 200);
		extraBox->setSuffix(tr(" ms"));
		extraBox->setValue(out->GetExtraBuffer());
		connect(extraBox, qOverload<int>(&QSpinBox::valueChanged), [this](int ms){
			mpSim->GetAudioOutput()->SetExtraBuffer(ms);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("audio/extraBufferMs"), ms);
		});
		form->addRow(tr("Extra buffer:"), extraBox);

		return page;
	}

	QWidget *buildDisplayTab() {
		auto *page = new QWidget;
		auto *form = new QFormLayout(page);

		// Pause-when-inactive lives in QSettings under system/pauseWhenInactive
		// and is wired in menus.cpp. We mirror it here so users can toggle it
		// from one place. The actual ApplicationStateChanged hook is rebuilt
		// when ATBuildAltirraMenuBar runs, so we just write the QSettings key.
		const bool pauseInactive = mpSettings
			? mpSettings->value(QStringLiteral("system/pauseWhenInactive"), false).toBool()
			: false;
		auto *pauseBox = new QCheckBox(tr("Pause when window is inactive"));
		pauseBox->setChecked(pauseInactive);
		connect(pauseBox, &QCheckBox::toggled, [this](bool on){
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("system/pauseWhenInactive"), on);
		});
		form->addRow(QString(), pauseBox);

		const bool indMargin = mpSettings
			? mpSettings->value(QStringLiteral("view/indicatorMargin"), false).toBool()
			: false;
		auto *indBox = new QCheckBox(tr("Reserve indicator margin at bottom of display"));
		indBox->setChecked(indMargin);
		connect(indBox, &QCheckBox::toggled, [this](bool on){
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("view/indicatorMargin"), on);
		});
		form->addRow(QString(), indBox);

		return page;
	}

	QWidget *buildCassetteTab() {
		auto *page = new QWidget;
		auto *form = new QFormLayout(page);

		auto& cas = mpSim->GetCassette();

		auto *algoCombo = new QComboBox;
		const ATCassetteTurboDecodeAlgorithm cur = cas.GetTurboDecodeAlgorithm();
		int curIdx = 0;
		for (size_t i = 0; i < std::size(kDecodeOpts); ++i) {
			algoCombo->addItem(tr(kDecodeOpts[i].label), (int)kDecodeOpts[i].v);
			if (kDecodeOpts[i].v == cur) curIdx = (int)i;
		}
		algoCombo->setCurrentIndex(curIdx);
		connect(algoCombo, qOverload<int>(&QComboBox::currentIndexChanged), [this, algoCombo](int){
			const auto v = (ATCassetteTurboDecodeAlgorithm)algoCombo->currentData().toInt();
			mpSim->GetCassette().SetTurboDecodeAlgorithm(v);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("cassette/turboDecodeAlgorithm"), (int)v);
		});
		form->addRow(tr("Turbo decode algorithm:"), algoCombo);

		// SetRandomizedStartEnabled has no public getter; the QSettings key
		// is the source of truth. Default off (matches the cassette ctor).
		const bool randStart = mpSettings
			? mpSettings->value(QStringLiteral("cassette/randomizedStart"), false).toBool()
			: false;
		cas.SetRandomizedStartEnabled(randStart);

		auto *randBox = new QCheckBox(tr("Randomized start (jitter tape position on cold reset)"));
		randBox->setChecked(randStart);
		connect(randBox, &QCheckBox::toggled, [this](bool on){
			mpSim->GetCassette().SetRandomizedStartEnabled(on);
			if (mpSettings)
				mpSettings->setValue(QStringLiteral("cassette/randomizedStart"), on);
		});
		form->addRow(QString(), randBox);

		return page;
	}

	ATSimulator *mpSim;
	QSettings   *mpSettings;
};

} // namespace

void ATShowAdvancedConfigDialog(QWidget *parent, ATSimulator *sim, QSettings *settings) {
	ATAdvancedConfigDialog dlg(parent, sim, settings);
	dlg.exec();
}
