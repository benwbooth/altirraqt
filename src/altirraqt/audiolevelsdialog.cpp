//	Altirra - Qt port
//	Audio Levels dialog implementation.

#include "audiolevelsdialog.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QSlider>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <at/atcore/audiomixer.h>
#include <at/ataudio/audiooutput.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

struct MixOpt { ATAudioMix mix; const char *label; const char *settingKey; };
constexpr MixOpt kMixes[] = {
	{kATAudioMix_Drive,    "Disk drive", "audio/mix/drive"},
	{kATAudioMix_Covox,    "Covox",      "audio/mix/covox"},
	{kATAudioMix_Modem,    "Modem",      "audio/mix/modem"},
	{kATAudioMix_Cassette, "Cassette",   "audio/mix/cassette"},
	{kATAudioMix_Other,    "Other",      "audio/mix/other"},
};

// Slider scale: 0..1000 → 0..2.0 linear gain (master cap doubles unity).
int gainToTick(float g) { return (int)(std::clamp(g, 0.0f, 2.0f) / 2.0f * 1000.0f + 0.5f); }
float tickToGain(int t) { return (float)t / 1000.0f * 2.0f; }

class ATAudioLevelsDialog : public QDialog {
public:
	ATAudioLevelsDialog(QWidget *parent, ATSimulator *sim, QSettings *settings)
		: QDialog(parent), mpSim(sim), mpSettings(settings)
	{
		setWindowTitle(tr("Audio Levels"));
		resize(420, 360);

		auto *out = sim->GetAudioOutput();
		mInitialMaster = out->GetVolume();
		mInitialMute   = out->GetMute();
		for (size_t i = 0; i < std::size(kMixes); ++i)
			mInitialMix[i] = out->GetMixLevel(kMixes[i].mix);

		auto *layout = new QVBoxLayout(this);

		// Master section.
		auto *masterBox = new QGroupBox(tr("Master"));
		auto *masterForm = new QFormLayout(masterBox);
		mpMute = new QCheckBox(tr("Mute"));
		mpMute->setChecked(mInitialMute);
		masterForm->addRow(QString(), mpMute);
		mpMaster = makeLevelRow(out->GetVolume(), masterForm, tr("Volume:"));
		layout->addWidget(masterBox);

		// Per-mix section.
		auto *mixBox = new QGroupBox(tr("Per-source mix"));
		auto *mixForm = new QFormLayout(mixBox);
		for (size_t i = 0; i < std::size(kMixes); ++i) {
			mpMixSliders[i] = makeLevelRow(out->GetMixLevel(kMixes[i].mix),
			                               mixForm,
			                               tr(kMixes[i].label) + ":");
		}
		layout->addWidget(mixBox, 1);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		layout->addWidget(box);
		connect(box, &QDialogButtonBox::accepted, this, &ATAudioLevelsDialog::onAccept);
		connect(box, &QDialogButtonBox::rejected, this, &ATAudioLevelsDialog::onCancel);

		connect(mpMute, &QCheckBox::toggled, [out](bool muted){
			out->SetMute(muted);
		});
		connect(mpMaster, &QSlider::valueChanged, [out](int t){
			out->SetVolume(tickToGain(t));
		});
		for (size_t i = 0; i < std::size(kMixes); ++i) {
			const auto m = kMixes[i].mix;
			connect(mpMixSliders[i], &QSlider::valueChanged, [out, m](int t){
				out->SetMixLevel(m, tickToGain(t));
			});
		}
	}

private:
	QSlider *makeLevelRow(float initial, QFormLayout *form, const QString& label) {
		auto *slider = new QSlider(Qt::Horizontal);
		slider->setRange(0, 1000);
		slider->setValue(gainToTick(initial));
		auto *valueLabel = new QLabel;
		valueLabel->setMinimumWidth(48);
		auto updateLabel = [valueLabel](int t){
			valueLabel->setText(QStringLiteral("%1%").arg((int)(tickToGain(t) * 100.0f + 0.5f)));
		};
		updateLabel(slider->value());
		connect(slider, &QSlider::valueChanged, valueLabel, updateLabel);
		auto *row = new QHBoxLayout;
		row->addWidget(slider, 3);
		row->addWidget(valueLabel);
		form->addRow(label, row);
		return slider;
	}

	void onAccept() {
		if (mpSettings) {
			auto *out = mpSim->GetAudioOutput();
			mpSettings->setValue(QStringLiteral("audio/volume"), out->GetVolume());
			mpSettings->setValue(QStringLiteral("audio/mute"),   out->GetMute());
			for (size_t i = 0; i < std::size(kMixes); ++i)
				mpSettings->setValue(QString::fromUtf8(kMixes[i].settingKey),
				                     out->GetMixLevel(kMixes[i].mix));
		}
		accept();
	}

	void onCancel() {
		auto *out = mpSim->GetAudioOutput();
		out->SetVolume(mInitialMaster);
		out->SetMute(mInitialMute);
		for (size_t i = 0; i < std::size(kMixes); ++i)
			out->SetMixLevel(kMixes[i].mix, mInitialMix[i]);
		reject();
	}

	ATSimulator *mpSim;
	QSettings   *mpSettings;
	QSlider     *mpMaster;
	QCheckBox   *mpMute;
	QSlider     *mpMixSliders[std::size(kMixes)] {};
	float        mInitialMaster;
	bool         mInitialMute;
	float        mInitialMix[std::size(kMixes)] {};
};

} // namespace

void ATShowAudioLevelsDialog(QWidget *parent, ATSimulator *sim, QSettings *settings) {
	ATAudioLevelsDialog dlg(parent, sim, settings);
	dlg.exec();
}
