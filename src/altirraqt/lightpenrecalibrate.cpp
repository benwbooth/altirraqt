//	Altirra - Qt port
//	Interactive light-pen / gun recalibration implementation.

#include "lightpenrecalibrate.h"

#include <QDialog>
#include <QEventLoop>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QStatusBar>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>

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

// Modal child overlay placed over the display widget. Captures mouse
// presses, draws a crosshair at the current target, advances on each
// click. Esc cancels.
class ATRecalibrateOverlay : public QWidget {
public:
	ATRecalibrateOverlay(QWidget *parent, ATSimulator *sim, QStatusBar *statusBar)
		: QWidget(parent), mpSim(sim), mpStatus(statusBar)
	{
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_TransparentForMouseEvents, false);
		setFocusPolicy(Qt::StrongFocus);
		setMouseTracking(true);
		setCursor(Qt::CrossCursor);
		// Save current offsets so Esc restores them.
		auto *lpp = mpSim->GetLightPenPort();
		mPriorPen = lpp->GetAdjust(true);
		mPriorGun = lpp->GetAdjust(false);

		// Reset offsets to zero during calibration so the user's clicks
		// give an absolute pen position; otherwise prior offsets would bias
		// the captured deltas.
		lpp->SetAdjust(true,  vdint2{0, 0});
		lpp->SetAdjust(false, vdint2{0, 0});

		setGeometry(parent->rect());
		raise();
		show();
		setFocus();
	}

	bool isCommitted() const { return mCommitted; }

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);
		p.fillRect(rect(), QColor(0, 0, 0, 160));

		const QPointF tgt = targetPoint(mIndex);

		// Crosshair.
		QPen pen(QColor(255, 220, 0));
		pen.setWidth(2);
		p.setPen(pen);
		const int s = 24;
		p.drawLine(QPointF(tgt.x() - s, tgt.y()), QPointF(tgt.x() + s, tgt.y()));
		p.drawLine(QPointF(tgt.x(), tgt.y() - s), QPointF(tgt.x(), tgt.y() + s));
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(tgt, 10.0, 10.0);
		p.drawEllipse(tgt, 2.0, 2.0);

		// Instructions text near top.
		QFont f = p.font();
		f.setPointSize(14);
		f.setBold(true);
		p.setFont(f);
		p.setPen(Qt::white);
		const QString msg = QStringLiteral(
			"Click on the crosshair as it appears (%1 / 4) — Esc cancels")
			.arg(mIndex + 1);
		p.drawText(QRect(0, 24, width(), 32), Qt::AlignCenter, msg);
	}

	void mousePressEvent(QMouseEvent *event) override {
		if (event->button() != Qt::LeftButton) {
			QWidget::mousePressEvent(event);
			return;
		}

		const QPointF tgt = targetPoint(mIndex);
		const QPointF clk = event->position();
		// Pen position scale: ATLightPenPort coordinates are in colour-clock
		// (X) and scan-line (Y) units. The expected mapping for a
		// full-overscan widget would need the actual rendered pixmap
		// geometry; for an interactive offset adjustment what matters is
		// the *delta* in pen units. We sample the offset in widget pixels
		// and scale to the typical colour-clock / scanline density of the
		// emulator output (≈ widget_w / 376 cc, widget_h / 240 lines).
		// Choose a stable mapping based on widget size.
		const float ppX = (float)width()  / 376.0f;
		const float ppY = (float)height() / 240.0f;
		const int dx = (int)std::lround((clk.x() - tgt.x()) / ppX);
		const int dy = (int)std::lround((clk.y() - tgt.y()) / ppY);
		mDeltas[mIndex] = vdint2{dx, dy};

		++mIndex;
		if (mIndex >= 4) {
			commit();
			return;
		}
		update();
	}

	void keyPressEvent(QKeyEvent *event) override {
		if (event->key() == Qt::Key_Escape) {
			cancel();
			return;
		}
		QWidget::keyPressEvent(event);
	}

private:
	QPointF targetPoint(int idx) const {
		const float w = (float)width();
		const float h = (float)height();
		switch (idx) {
			case 0: return QPointF(w * 0.25f, h * 0.25f);
			case 1: return QPointF(w * 0.75f, h * 0.25f);
			case 2: return QPointF(w * 0.25f, h * 0.75f);
			default:return QPointF(w * 0.75f, h * 0.75f);
		}
	}

	void commit() {
		// Average the captured deltas. The ATLightPenPort offset moves the
		// reported pen position by `+offset` when the gun trigger fires, so
		// to compensate for an observed shift of `+delta` we apply
		// `-delta`.
		int sx = 0, sy = 0;
		for (int i = 0; i < 4; ++i) {
			sx += mDeltas[i].x;
			sy += mDeltas[i].y;
		}
		const int avgX = -sx / 4;
		const int avgY = -sy / 4;
		const vdint2 off{
			std::clamp(avgX, -64, 64),
			std::clamp(avgY, -64, 64)
		};
		auto *lpp = mpSim->GetLightPenPort();
		lpp->SetAdjust(true,  off);
		lpp->SetAdjust(false, off);
		mCommitted = true;
		if (mpStatus)
			mpStatus->showMessage(QStringLiteral(
				"Light pen / gun recalibrated: offset (%1, %2)")
				.arg(off.x).arg(off.y), 4000);
		close();
	}

	void cancel() {
		// Restore previous offsets.
		auto *lpp = mpSim->GetLightPenPort();
		lpp->SetAdjust(true,  mPriorPen);
		lpp->SetAdjust(false, mPriorGun);
		mCommitted = false;
		close();
	}

	ATSimulator *mpSim;
	QStatusBar  *mpStatus;
	int          mIndex = 0;
	vdint2       mDeltas[4] {};
	vdint2       mPriorPen{0, 0};
	vdint2       mPriorGun{0, 0};
	bool         mCommitted = false;
};

} // namespace

void ATRunLightPenRecalibrate(QMainWindow *window, QWidget *displayWidget, ATSimulator *sim) {
	if (!displayWidget || !sim) return;

	// Show a brief instructional dialog first.
	{
		QMessageBox box(window);
		box.setWindowTitle(QObject::tr("Recalibrate Light Pen / Gun"));
		box.setText(QObject::tr(
			"Four crosshairs will appear on the display, one at a time.\n"
			"Click on each crosshair as accurately as you can.\n\n"
			"Press Esc at any point to cancel."));
		box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		box.setDefaultButton(QMessageBox::Ok);
		if (box.exec() != QMessageBox::Ok) return;
	}

	// Construct the overlay; it owns the modal-loop logic. Use an event
	// loop tied to the overlay's lifetime so the call returns when the
	// user finishes or cancels.
	QStatusBar *statusBar = window ? window->statusBar() : nullptr;
	auto *overlay = new ATRecalibrateOverlay(displayWidget, sim, statusBar);
	overlay->setAttribute(Qt::WA_DeleteOnClose, true);
	// Run the event loop until the overlay is closed.
	QEventLoop loop;
	QObject::connect(overlay, &QObject::destroyed, &loop, &QEventLoop::quit);
	loop.exec();
}
