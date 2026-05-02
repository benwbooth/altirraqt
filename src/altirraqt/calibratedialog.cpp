//	Altirra - Qt port
//	Display calibration dialog implementation.

#include "calibratedialog.h"

#include <QDialog>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScreen>
#include <QString>
#include <QWidget>
#include <QWindow>

namespace {

class ATCalibrateDialog : public QDialog {
public:
	explicit ATCalibrateDialog(QWidget *parent)
		: QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog)
	{
		setAttribute(Qt::WA_DeleteOnClose, false);
		setWindowTitle(tr("Calibrate"));
		setStyleSheet(QStringLiteral("background: black;"));
		setFocusPolicy(Qt::StrongFocus);
		setMouseTracking(false);
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.fillRect(rect(), Qt::black);

		const int W = width();
		const int H = height();
		if (W <= 0 || H <= 0) return;

		// Top quarter: 16-step greyscale ramp.
		const int rampH = H / 5;
		for (int i = 0; i < 16; ++i) {
			const int x0 = (i * W) / 16;
			const int x1 = ((i + 1) * W) / 16;
			const int v = (i * 255) / 15;
			p.fillRect(QRect(x0, 0, x1 - x0, rampH), QColor(v, v, v));
		}

		// Below ramp: 6 colour bars (R, G, B, C, M, Y), full saturation.
		static const QColor kBars[6] = {
			QColor(255,   0,   0),
			QColor(  0, 255,   0),
			QColor(  0,   0, 255),
			QColor(  0, 255, 255),
			QColor(255,   0, 255),
			QColor(255, 255,   0),
		};
		const int barsH = H / 5;
		for (int i = 0; i < 6; ++i) {
			const int x0 = (i * W) / 6;
			const int x1 = ((i + 1) * W) / 6;
			p.fillRect(QRect(x0, rampH, x1 - x0, barsH), kBars[i]);
		}

		// Geometry guides: white cross-hair at the centre + tick marks at
		// the four edges.
		QPen pen(Qt::white);
		pen.setWidth(1);
		p.setPen(pen);
		const int cx = W / 2;
		const int cy = H / 2;
		p.drawLine(0, cy, W, cy);
		p.drawLine(cx, 0, cx, H);

		// Corner crosses.
		const int s = 24;
		auto cross = [&](int x, int y) {
			p.drawLine(x - s, y, x + s, y);
			p.drawLine(x, y - s, x, y + s);
		};
		cross(s + 4, s + 4);
		cross(W - s - 4, s + 4);
		cross(s + 4, H - s - 4);
		cross(W - s - 4, H - s - 4);

		// Edge ticks every 10% along each side.
		for (int i = 1; i < 10; ++i) {
			const int x = (i * W) / 10;
			const int y = (i * H) / 10;
			p.drawLine(x, 0, x, 8);
			p.drawLine(x, H - 8, x, H);
			p.drawLine(0, y, 8, y);
			p.drawLine(W - 8, y, W, y);
		}

		// Centre circle for convergence/aspect check.
		p.setPen(QPen(Qt::white, 1));
		p.setBrush(Qt::NoBrush);
		const int r = std::min(W, H) / 6;
		p.drawEllipse(QPoint(cx, cy), r, r);

		// Help text near the bottom.
		QFont f = p.font();
		f.setPointSize(14);
		p.setFont(f);
		p.setPen(Qt::white);
		p.drawText(QRect(0, H - 40, W, 30),
		           Qt::AlignCenter,
		           tr("Click anywhere or press Esc to close"));
	}

	void keyPressEvent(QKeyEvent *e) override {
		if (e->key() == Qt::Key_Escape) {
			accept();
			return;
		}
		QDialog::keyPressEvent(e);
	}

	void mousePressEvent(QMouseEvent *) override {
		accept();
	}
};

} // namespace

void ATShowCalibrateDialog(QWidget *parent) {
	ATCalibrateDialog dlg(parent);
	// Cover the parent's screen.
	QScreen *scr = nullptr;
	if (parent && parent->windowHandle()) scr = parent->windowHandle()->screen();
	if (!scr) scr = QGuiApplication::primaryScreen();
	if (scr) {
		const QRect g = scr->geometry();
		dlg.setGeometry(g);
	}
	dlg.showFullScreen();
	dlg.exec();
}
