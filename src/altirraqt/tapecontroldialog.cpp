//	Altirra - Qt port
//	Tape Control dialog implementation.

#include "tapecontroldialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <cassette.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

QString formatTime(float seconds) {
	if (seconds < 0.0f) seconds = 0.0f;
	const int total = (int)(seconds + 0.5f);
	const int mm = total / 60;
	const int ss = total % 60;
	return QStringLiteral("%1:%2")
		.arg(mm, 2, 10, QLatin1Char('0'))
		.arg(ss, 2, 10, QLatin1Char('0'));
}

class ATTapeControlDialog : public QDialog {
public:
	ATTapeControlDialog(QWidget *parent, ATSimulator *sim)
		: QDialog(parent), mpSim(sim)
	{
		setWindowTitle(tr("Tape Control"));
		resize(420, 180);

		auto *layout = new QVBoxLayout(this);

		mpStatusLabel = new QLabel(this);
		mpStatusLabel->setAlignment(Qt::AlignCenter);
		layout->addWidget(mpStatusLabel);

		// Position slider — 0..1000 ticks across full tape length.
		auto *posRow = new QHBoxLayout;
		mpPositionLabel = new QLabel(QStringLiteral("00:00"), this);
		mpLengthLabel   = new QLabel(QStringLiteral("00:00"), this);
		mpSlider        = new QSlider(Qt::Horizontal, this);
		mpSlider->setRange(0, 1000);
		posRow->addWidget(mpPositionLabel);
		posRow->addWidget(mpSlider, 1);
		posRow->addWidget(mpLengthLabel);
		layout->addLayout(posRow);

		// Transport row.
		auto *btnRow = new QHBoxLayout;
		auto *rewindBtn = new QPushButton(tr("|<< Rewind"), this);
		auto *playBtn   = new QPushButton(tr("> Play"), this);
		auto *pauseBtn  = new QPushButton(tr("|| Pause"), this);
		auto *stopBtn   = new QPushButton(tr("[] Stop"), this);
		auto *recordBtn = new QPushButton(tr("() Record"), this);
		btnRow->addWidget(rewindBtn);
		btnRow->addWidget(playBtn);
		btnRow->addWidget(pauseBtn);
		btnRow->addWidget(stopBtn);
		btnRow->addWidget(recordBtn);
		layout->addLayout(btnRow);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Close, this);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::accept);
		layout->addWidget(box);

		connect(rewindBtn, &QPushButton::clicked, [this]{
			mpSim->GetCassette().RewindToStart();
		});
		connect(playBtn, &QPushButton::clicked, [this]{
			auto& c = mpSim->GetCassette();
			if (c.IsPaused()) c.SetPaused(false);
			c.Play();
		});
		connect(pauseBtn, &QPushButton::clicked, [this]{
			auto& c = mpSim->GetCassette();
			c.SetPaused(!c.IsPaused());
		});
		connect(stopBtn, &QPushButton::clicked, [this]{
			mpSim->GetCassette().Stop();
		});
		connect(recordBtn, &QPushButton::clicked, [this]{
			mpSim->GetCassette().Record();
		});

		// Slider drag → SeekToTime. Use sliderMoved (only fires for user
		// interaction) so our own programmatic refresh() doesn't recurse.
		connect(mpSlider, &QSlider::sliderMoved, [this](int v){
			auto& c = mpSim->GetCassette();
			const float len = c.GetLength();
			c.SeekToTime((v / 1000.0f) * len);
		});

		mpTimer = new QTimer(this);
		mpTimer->setInterval(100);
		connect(mpTimer, &QTimer::timeout, this, &ATTapeControlDialog::refresh);
		mpTimer->start();

		refresh();
	}

private:
	void refresh() {
		auto& c = mpSim->GetCassette();
		const bool loaded = c.IsLoaded();

		const float len = loaded ? c.GetLength() : 0.0f;
		const float pos = loaded ? c.GetPosition() : 0.0f;

		mpPositionLabel->setText(formatTime(pos));
		mpLengthLabel  ->setText(formatTime(len));

		if (!mpSlider->isSliderDown() && len > 0.0f) {
			const int v = (int)((pos / len) * 1000.0f + 0.5f);
			mpSlider->blockSignals(true);
			mpSlider->setValue(v);
			mpSlider->blockSignals(false);
		}

		QString status;
		if (!loaded)                  status = tr("No tape loaded");
		else if (c.IsRecordEnabled()) status = c.IsPaused() ? tr("Recording (paused)")
		                                                    : tr("Recording");
		else if (c.IsPlayEnabled())   status = c.IsPaused() ? tr("Playing (paused)")
		                                                    : tr("Playing");
		else                          status = tr("Stopped");
		mpStatusLabel->setText(status);
	}

	ATSimulator *mpSim;
	QLabel      *mpStatusLabel;
	QLabel      *mpPositionLabel;
	QLabel      *mpLengthLabel;
	QSlider     *mpSlider;
	QTimer      *mpTimer;
};

} // namespace

void ATShowTapeControlDialog(QWidget *parent, ATSimulator *sim) {
	ATTapeControlDialog dlg(parent, sim);
	dlg.exec();
}
