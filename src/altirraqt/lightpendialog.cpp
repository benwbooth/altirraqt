//	Altirra - Qt port
//	Light Pen / Gun dialog implementation.

#include "lightpendialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/refcount.h>
#include <vd2/system/vdalloc.h>
#include <vd2/system/vectors.h>
#include <at/atcore/deviceport.h>
#include <at/atcore/enumparse.h>
#include <inputcontroller.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

class ATLightPenDialog : public QDialog {
public:
	ATLightPenDialog(QWidget *parent, ATSimulator *sim)
		: QDialog(parent), mpSim(sim)
	{
		setWindowTitle(tr("Light Pen / Gun"));
		resize(360, 240);

		auto *lpp = mpSim->GetLightPenPort();

		auto *layout = new QVBoxLayout(this);

		auto makeAdjustGroup = [this](const QString& title, vdint2 initial,
		                              QSpinBox **xOut, QSpinBox **yOut) {
			auto *box = new QGroupBox(title);
			auto *form = new QFormLayout(box);
			auto *xSpin = new QSpinBox; xSpin->setRange(-64, 64); xSpin->setValue(initial.x);
			auto *ySpin = new QSpinBox; ySpin->setRange(-64, 64); ySpin->setValue(initial.y);
			form->addRow(tr("Horizontal:"), xSpin);
			form->addRow(tr("Vertical:"),   ySpin);
			*xOut = xSpin;
			*yOut = ySpin;
			return box;
		};

		auto *gunBox = makeAdjustGroup(tr("Gun (CX-75)"),
		                               lpp->GetAdjust(false), &mpGunX, &mpGunY);
		auto *penBox = makeAdjustGroup(tr("Light Pen"),
		                               lpp->GetAdjust(true),  &mpPenX, &mpPenY);

		auto *adjustRow = new QHBoxLayout;
		adjustRow->addWidget(gunBox);
		adjustRow->addWidget(penBox);
		layout->addLayout(adjustRow);

		mpNoiseCombo = new QComboBox;
		mpNoiseCombo->addItems({tr("None"),
		                        tr("Low (CX-75 + 800)"),
		                        tr("High (CX-75 + XL/XE)")});
		mpNoiseCombo->setCurrentIndex((int)lpp->GetNoiseMode());
		auto *noiseForm = new QFormLayout;
		noiseForm->addRow(tr("Noise mode:"), mpNoiseCombo);
		layout->addLayout(noiseForm);

		auto *btnRow = new QHBoxLayout;
		auto *resetBtn = new QPushButton(tr("Reset offsets"));
		btnRow->addWidget(resetBtn);
		btnRow->addStretch();
		layout->addLayout(btnRow);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		layout->addWidget(box);

		connect(box, &QDialogButtonBox::accepted, this, &ATLightPenDialog::onAccept);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
		connect(resetBtn, &QPushButton::clicked, [this]{
			mpGunX->setValue(0); mpGunY->setValue(0);
			mpPenX->setValue(0); mpPenY->setValue(0);
		});
	}

private:
	void onAccept() {
		auto *lpp = mpSim->GetLightPenPort();
		lpp->SetAdjust(false, vdint2{ mpGunX->value(), mpGunY->value() });
		lpp->SetAdjust(true,  vdint2{ mpPenX->value(), mpPenY->value() });
		lpp->SetNoiseMode((ATLightPenNoiseMode)mpNoiseCombo->currentIndex());
		accept();
	}

	ATSimulator *mpSim;
	QSpinBox    *mpGunX, *mpGunY;
	QSpinBox    *mpPenX, *mpPenY;
	QComboBox   *mpNoiseCombo;
};

} // namespace

void ATShowLightPenDialog(QWidget *parent, ATSimulator *sim) {
	ATLightPenDialog dlg(parent, sim);
	dlg.exec();
}
