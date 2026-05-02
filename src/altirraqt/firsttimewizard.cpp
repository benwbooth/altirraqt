//	Altirra - Qt port
//	First Time Setup wizard implementation.

#include "firsttimewizard.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <constants.h>

namespace {

// Wizard fields written to QSettings on accept. We accumulate them in a
// shared struct so each page can stash its choice and the final commit
// step can write them all at once.
struct WizardResult {
	int  hardwareMode  = (int)kATHardwareMode_800XL;
	int  videoStandard = (int)kATVideoStandard_NTSC;
	bool basicEnabled  = false;
	int  volumePercent = 100;
	bool muted         = false;
	bool apply         = false;
};

class WelcomePage : public QWizardPage {
public:
	WelcomePage() {
		setTitle(QObject::tr("Welcome"));
		auto *layout = new QVBoxLayout(this);
		auto *label = new QLabel(QObject::tr(
			"Welcome to altirraqt.\n\n"
			"This wizard will walk you through the initial configuration "
			"(hardware mode, video standard, BASIC, audio defaults). "
			"You can change every setting later from the menus."));
		label->setWordWrap(true);
		layout->addWidget(label);
	}
};

class HardwarePage : public QWizardPage {
public:
	HardwarePage(WizardResult& result) : mpResult(&result) {
		setTitle(QObject::tr("Hardware Mode"));
		setSubTitle(QObject::tr("Pick the Atari machine to emulate."));

		auto *layout = new QVBoxLayout(this);
		mpGroup = new QButtonGroup(this);

		struct Opt { int mode; const char *label; };
		// Mirror the order/labels from the Configure System dialog.
		static const Opt kOpts[] = {
			{(int)kATHardwareMode_800,    "Atari 800"},
			{(int)kATHardwareMode_800XL,  "Atari 800XL (default)"},
			{(int)kATHardwareMode_1200XL, "Atari 1200XL"},
			{(int)kATHardwareMode_130XE,  "Atari 130XE"},
			{(int)kATHardwareMode_XEGS,   "Atari XEGS"},
			{(int)kATHardwareMode_5200,   "Atari 5200"},
		};

		for (const auto& o : kOpts) {
			auto *rb = new QRadioButton(QObject::tr(o.label));
			mpGroup->addButton(rb, o.mode);
			layout->addWidget(rb);
			if (o.mode == (int)kATHardwareMode_800XL)
				rb->setChecked(true);
		}
	}

	bool validatePage() override {
		mpResult->hardwareMode = mpGroup->checkedId();
		if (mpResult->hardwareMode < 0)
			mpResult->hardwareMode = (int)kATHardwareMode_800XL;
		return true;
	}

private:
	WizardResult  *mpResult;
	QButtonGroup  *mpGroup;
};

class VideoPage : public QWizardPage {
public:
	VideoPage(WizardResult& result) : mpResult(&result) {
		setTitle(QObject::tr("Video Standard"));
		setSubTitle(QObject::tr("Pick the video standard to emulate."));

		auto *layout = new QVBoxLayout(this);
		mpGroup = new QButtonGroup(this);

		struct Opt { int std; const char *label; };
		static const Opt kOpts[] = {
			{(int)kATVideoStandard_NTSC,    "NTSC (default)"},
			{(int)kATVideoStandard_PAL,     "PAL"},
			{(int)kATVideoStandard_SECAM,   "SECAM"},
			{(int)kATVideoStandard_PAL60,   "PAL-60"},
			{(int)kATVideoStandard_NTSC50,  "NTSC-50"},
		};

		for (const auto& o : kOpts) {
			auto *rb = new QRadioButton(QObject::tr(o.label));
			mpGroup->addButton(rb, o.std);
			layout->addWidget(rb);
			if (o.std == (int)kATVideoStandard_NTSC)
				rb->setChecked(true);
		}
	}

	bool validatePage() override {
		mpResult->videoStandard = mpGroup->checkedId();
		if (mpResult->videoStandard < 0)
			mpResult->videoStandard = (int)kATVideoStandard_NTSC;
		return true;
	}

private:
	WizardResult  *mpResult;
	QButtonGroup  *mpGroup;
};

class BasicPage : public QWizardPage {
public:
	BasicPage(WizardResult& result) : mpResult(&result) {
		setTitle(QObject::tr("BASIC"));
		setSubTitle(QObject::tr("Boot with BASIC enabled or disabled."));

		auto *layout = new QVBoxLayout(this);
		mpEnabled = new QCheckBox(QObject::tr("Boot with Internal BASIC enabled"));
		layout->addWidget(mpEnabled);
	}

	bool validatePage() override {
		mpResult->basicEnabled = mpEnabled->isChecked();
		return true;
	}

private:
	WizardResult *mpResult;
	QCheckBox    *mpEnabled;
};

class AudioPage : public QWizardPage {
public:
	AudioPage(WizardResult& result) : mpResult(&result) {
		setTitle(QObject::tr("Audio Defaults"));
		setSubTitle(QObject::tr("Set the master volume and mute state."));

		auto *form = new QFormLayout(this);

		mpVolume = new QSlider(Qt::Horizontal);
		mpVolume->setRange(0, 200);
		mpVolume->setValue(100);

		auto *valueLabel = new QLabel(QStringLiteral("100%"));
		valueLabel->setMinimumWidth(48);
		QObject::connect(mpVolume, &QSlider::valueChanged, valueLabel, [valueLabel](int v){
			valueLabel->setText(QStringLiteral("%1%").arg(v));
		});
		auto *row = new QHBoxLayout;
		row->addWidget(mpVolume, 3);
		row->addWidget(valueLabel);
		form->addRow(QObject::tr("Master volume:"), row);

		mpMute = new QCheckBox(QObject::tr("Start muted"));
		form->addRow(QString(), mpMute);
	}

	bool validatePage() override {
		mpResult->volumePercent = mpVolume->value();
		mpResult->muted         = mpMute->isChecked();
		return true;
	}

private:
	WizardResult *mpResult;
	QSlider      *mpVolume;
	QCheckBox    *mpMute;
};

class ConfirmPage : public QWizardPage {
public:
	ConfirmPage(WizardResult& result) : mpResult(&result) {
		setTitle(QObject::tr("Apply"));
		setSubTitle(QObject::tr("Apply settings now?"));

		auto *layout = new QVBoxLayout(this);
		mpYes = new QRadioButton(QObject::tr("Yes — apply settings"));
		auto *no  = new QRadioButton(QObject::tr("No — abort the wizard"));
		mpYes->setChecked(true);
		layout->addWidget(mpYes);
		layout->addWidget(no);
	}

	bool validatePage() override {
		mpResult->apply = mpYes->isChecked();
		return true;
	}

private:
	WizardResult  *mpResult;
	QRadioButton  *mpYes;
};

} // namespace

bool ATRunFirstTimeWizard(QWidget *parent, QSettings *settings) {
	if (!settings) return false;

	WizardResult result;

	QWizard wizard(parent);
	wizard.setWindowTitle(QObject::tr("First Time Setup"));
	wizard.addPage(new WelcomePage);
	wizard.addPage(new HardwarePage(result));
	wizard.addPage(new VideoPage(result));
	wizard.addPage(new BasicPage(result));
	wizard.addPage(new AudioPage(result));
	wizard.addPage(new ConfirmPage(result));
	wizard.setOption(QWizard::NoCancelButton, false);
	wizard.setOption(QWizard::IndependentPages, false);

	if (wizard.exec() != QDialog::Accepted) return false;
	if (!result.apply) return false;

	settings->setValue(QStringLiteral("system/hardwareMode"),  result.hardwareMode);
	settings->setValue(QStringLiteral("system/videoStandard"), result.videoStandard);
	settings->setValue(QStringLiteral("system/basicEnabled"),  result.basicEnabled);
	// Master volume slider runs 0..200% but the audio output API takes a
	// 0..2 linear gain.
	settings->setValue(QStringLiteral("audio/volume"), result.volumePercent / 100.0f);
	settings->setValue(QStringLiteral("audio/mute"),   result.muted);
	settings->sync();
	return true;
}
