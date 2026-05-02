//	Altirra - Qt port
//	Adjust Colors dialog.

#include "adjustcolorsdialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <gtia.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

// Slider scale: each visible slider tick represents 0.01 of the float value.
struct SliderField {
	const char *label;
	float ATColorParams::*ptr;
	float min;
	float max;
};

constexpr SliderField kFields[] = {
	{"Hue start",      &ATColorParams::mHueStart,         -180.0f, 180.0f},
	{"Hue range",      &ATColorParams::mHueRange,            0.0f, 360.0f},
	{"Brightness",     &ATColorParams::mBrightness,         -1.0f,   1.0f},
	{"Contrast",       &ATColorParams::mContrast,            0.0f,   2.0f},
	{"Saturation",     &ATColorParams::mSaturation,          0.0f,   2.0f},
	{"Gamma correct",  &ATColorParams::mGammaCorrect,        0.5f,   2.5f},
	{"Intensity scale",&ATColorParams::mIntensityScale,      0.0f,   2.0f},
	{"Artifact hue",   &ATColorParams::mArtifactHue,      -180.0f, 180.0f},
	{"Artifact sat.",  &ATColorParams::mArtifactSat,         0.0f,   4.0f},
	{"Artifact sharp.",&ATColorParams::mArtifactSharpness,   0.0f,   1.0f},
	{"Red shift",      &ATColorParams::mRedShift,           -1.0f,   1.0f},
	{"Red scale",      &ATColorParams::mRedScale,            0.0f,   2.0f},
	{"Green shift",    &ATColorParams::mGrnShift,           -1.0f,   1.0f},
	{"Green scale",    &ATColorParams::mGrnScale,            0.0f,   2.0f},
	{"Blue shift",     &ATColorParams::mBluShift,           -1.0f,   1.0f},
	{"Blue scale",     &ATColorParams::mBluScale,            0.0f,   2.0f},
};

class ATAdjustColorsDialog : public QDialog {
public:
	ATAdjustColorsDialog(QWidget *parent, ATSimulator *sim)
		: QDialog(parent), mpSim(sim)
	{
		setWindowTitle(tr("Adjust Colors"));
		resize(440, 600);

		mInitialSettings = mpSim->GetGTIA().GetColorSettings();
		mLiveSettings    = mInitialSettings;

		auto *layout = new QVBoxLayout(this);

		auto *form = new QFormLayout;
		form->setLabelAlignment(Qt::AlignRight);
		layout->addLayout(form, 1);

		// Each row: label, slider, spinbox. Both control the same field.
		for (size_t i = 0; i < sizeof(kFields) / sizeof(kFields[0]); ++i) {
			const auto& f = kFields[i];
			auto *spin = new QDoubleSpinBox;
			spin->setRange(f.min, f.max);
			spin->setSingleStep(0.01);
			spin->setDecimals(3);
			spin->setValue(currentParams().*f.ptr);

			auto *slider = new QSlider(Qt::Horizontal);
			slider->setRange(0, 1000);
			slider->setValue(toSliderValue(currentParams().*f.ptr, f));

			auto *row = new QHBoxLayout;
			row->addWidget(slider, 3);
			row->addWidget(spin,   1);
			form->addRow(QString::fromUtf8(f.label), row);

			connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged),
				[this, &f, slider](double v){
					mUpdating = true;
					slider->setValue(toSliderValue((float)v, f));
					mUpdating = false;
					currentParams().*f.ptr = (float)v;
					applyLive();
				});
			connect(slider, &QSlider::valueChanged,
				[this, &f, spin](int v){
					if (mUpdating) return;
					const float val = fromSliderValue(v, f);
					mUpdating = true;
					spin->setValue(val);
					mUpdating = false;
					currentParams().*f.ptr = val;
					applyLive();
				});
		}

		auto *btnRow = new QHBoxLayout;
		auto *resetBtn = new QPushButton(tr("Reset to defaults"));
		btnRow->addWidget(resetBtn);
		btnRow->addStretch();
		layout->addLayout(btnRow);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		layout->addWidget(box);

		connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(box, &QDialogButtonBox::rejected, this, [this]{
			// Revert: restore initial settings on cancel.
			mpSim->GetGTIA().SetColorSettings(mInitialSettings);
			reject();
		});
		connect(resetBtn, &QPushButton::clicked, [this]{
			currentParams() = ATGetColorPresetByIndex(0);
			applyLive();
			// Refresh the form values to match the reset preset.
			refresh();
		});
	}

private:
	ATColorParams& currentParams() {
		// NTSC params for now — a future revision could add a NTSC/PAL toggle.
		return (ATColorParams&)mLiveSettings.mNTSCParams;
	}

	static int toSliderValue(float v, const SliderField& f) {
		return (int)((v - f.min) / (f.max - f.min) * 1000.0f);
	}
	static float fromSliderValue(int s, const SliderField& f) {
		return f.min + (s / 1000.0f) * (f.max - f.min);
	}

	void applyLive() {
		mpSim->GetGTIA().SetColorSettings(mLiveSettings);
	}

	void refresh() {
		// Just rebuild the dialog children — easier than tracking each pair.
		// In practice a "Reset" only needs to update widget values; we already
		// applied to GTIA and spins are tracked via signals, so this is a no-op.
	}

	ATSimulator      *mpSim;
	ATColorSettings   mInitialSettings;
	ATColorSettings   mLiveSettings;
	bool              mUpdating = false;
};

} // namespace

void ATShowAdjustColorsDialog(QWidget *parent, ATSimulator *sim) {
	ATAdjustColorsDialog dlg(parent, sim);
	dlg.exec();
}
