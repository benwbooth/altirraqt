//	altirraqt — Alt video output viewer.

#include "altvideodialog.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#undef signals
#undef slots

#include <vd2/system/vdtypes.h>
#include <vd2/system/refcount.h>
#include <vd2/Kasumi/pixmap.h>
#include <vd2/Kasumi/pixmapops.h>
#include <vd2/Kasumi/pixmaputils.h>

#include <at/atcore/devicevideo.h>
#include <devicemanager.h>
#include <simulator.h>

namespace {

QImage pixmapToQImage(const VDPixmap& src) {
	if (src.w <= 0 || src.h <= 0 || !src.data) return {};

	// Convert anything to XRGB8888 via Kasumi blit, then wrap as QImage.
	VDPixmapBuffer staging(src.w, src.h, nsVDPixmap::kPixFormat_XRGB8888);
	VDPixmapBlt(staging, src);

	// XRGB8888 in vd2 is BGRA byte order on little-endian. Qt's
	// QImage::Format_RGB32 stores 0xffRRGGBB and reads BGRA bytes too,
	// so a direct memcpy works.
	QImage out(staging.w, staging.h, QImage::Format_RGB32);
	for (int y = 0; y < staging.h; ++y)
		std::memcpy(out.scanLine(y),
		            (const uint8 *)staging.data + y * staging.pitch,
		            staging.w * 4);
	return out;
}

class AltVideoView : public QWidget {
public:
	AltVideoView(QWidget *parent, IATDeviceVideoOutput *output)
		: QWidget(parent), mpOutput(output)
	{
		setMinimumSize(640, 480);
		setAutoFillBackground(true);
		mTimer = new QTimer(this);
		mTimer->setInterval(16);
		QObject::connect(mTimer, &QTimer::timeout, this, [this]{
			if (mpOutput) {
				// 5 ticks @ 300Hz = 60Hz.
				mpOutput->Tick(5);
				mpOutput->UpdateFrame();
			}
			update();
		});
		mTimer->start();
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.fillRect(rect(), Qt::black);
		if (!mpOutput) return;
		const QImage frame = pixmapToQImage(mpOutput->GetFrameBuffer());
		if (frame.isNull()) return;
		// Letterbox-fit while preserving aspect.
		const QSize want = frame.size();
		const QSize have = size();
		double sx = (double)have.width()  / (double)want.width();
		double sy = (double)have.height() / (double)want.height();
		double s  = std::min(sx, sy);
		const int dw = (int)(want.width()  * s);
		const int dh = (int)(want.height() * s);
		const int dx = (have.width()  - dw) / 2;
		const int dy = (have.height() - dh) / 2;
		p.drawImage(QRect(dx, dy, dw, dh), frame);
	}

private:
	IATDeviceVideoOutput *mpOutput;
	QTimer *mTimer = nullptr;
};

QPointer<QDialog> gDialog;

} // namespace

void ATShowAltVideoDialog(QWidget *parent, ATSimulator *sim) {
	if (gDialog) {
		gDialog->raise();
		gDialog->activateWindow();
		return;
	}
	if (!sim) return;
	auto *vm = sim->GetDeviceManager()->GetService<IATDeviceVideoManager>();
	if (!vm) return;

	auto *dlg = new QDialog(parent);
	gDialog = dlg;
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->setWindowTitle(QObject::tr("Alt Video Output"));
	dlg->resize(800, 600);

	auto *root = new QVBoxLayout(dlg);
	auto *tabs = new QTabWidget(dlg);
	root->addWidget(tabs, 1);

	struct OutputEntry { IATDeviceVideoOutput *out; uint32 idx; uint32 activity; };
	std::vector<OutputEntry> entries;
	for (uint32 i = 0; i < vm->GetOutputCount(); ++i) {
		IATDeviceVideoOutput *out = vm->GetOutput(i);
		if (!out) continue;
		auto *view = new AltVideoView(dlg, out);
		const QString name = QString::fromWCharArray(out->GetDisplayName());
		const int tabIdx = tabs->addTab(view,
			name.isEmpty() ? QString::fromUtf8(out->GetName()) : name);
		entries.push_back({out, (uint32)tabIdx, out->GetActivityCounter()});
	}
	const bool any = !entries.empty();

	// If autoswitching is enabled, default to the most active output.
	if (any && QSettings().value(
		QStringLiteral("view/altOutputAutoswitch"), false).toBool())
	{
		uint32 best = 0;
		for (size_t i = 1; i < entries.size(); ++i)
			if (entries[i].activity > entries[best].activity) best = i;
		tabs->setCurrentIndex((int)entries[best].idx);
	}
	if (!any) {
		auto *empty = new QLabel(QObject::tr(
			"No alt-video outputs are registered.\n\n"
			"Attach an XEP80 (or other alt-video) device via\n"
			"System → Device Manager... and reopen this window."), dlg);
		empty->setAlignment(Qt::AlignCenter);
		tabs->addTab(empty, QObject::tr("(none)"));
	}

	auto *btnRow = new QHBoxLayout();
	auto *closeBtn = new QPushButton(QObject::tr("Close"));
	btnRow->addStretch();
	btnRow->addWidget(closeBtn);
	root->addLayout(btnRow);
	QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

	dlg->show();
}
