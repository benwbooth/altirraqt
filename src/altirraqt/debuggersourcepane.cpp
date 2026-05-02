//	altirraqt — Debugger source-files pane.

#include "debuggersourcepane.h"

#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointer>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#undef signals
#undef slots

#include <vd2/system/vdtypes.h>
#include <vd2/system/vdstl.h>
#include <vd2/system/VDString.h>

#include <at/atdebugger/symbols.h>

#include <debugger.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

struct SourceTab {
	QString    path;     // host path
	uint32     moduleId  = 0;
	uint16     fileId    = 0;
	int        pcLine    = -1;  // 0-based; -1 = none
};

class GutteredEdit : public QPlainTextEdit {
public:
	explicit GutteredEdit(QWidget *parent) : QPlainTextEdit(parent) {
		setReadOnly(true);
		setLineWrapMode(NoWrap);
		QFont f = font();
		f.setFamily(QStringLiteral("monospace"));
		setFont(f);
		mGutter = new QWidget(this);
		mGutter->installEventFilter(reinterpret_cast<QObject *>(this));
		connect(this, &QPlainTextEdit::blockCountChanged,
			this, &GutteredEdit::updateGutterWidth);
		connect(this, &QPlainTextEdit::updateRequest,
			this, &GutteredEdit::updateGutter);
		updateGutterWidth(0);
	}

	void setPcLine(int line) {
		mPcLine = line;
		update();
		mGutter->update();
		if (line >= 0) {
			QTextCursor c(document()->findBlockByNumber(line));
			c.movePosition(QTextCursor::StartOfBlock);
			setTextCursor(c);
			ensureCursorVisible();
		}
	}

	void setBreakpointToggle(std::function<void(int /*line*/)> fn) {
		mToggle = std::move(fn);
	}

	int gutterWidth() const {
		const int digits = QString::number(blockCount()).size();
		return 32 + fontMetrics().horizontalAdvance(QChar('9')) * std::max(3, digits);
	}

protected:
	void resizeEvent(QResizeEvent *e) override {
		QPlainTextEdit::resizeEvent(e);
		QRect cr = contentsRect();
		mGutter->setGeometry(QRect(cr.left(), cr.top(), gutterWidth(), cr.height()));
	}

	bool eventFilter(QObject *o, QEvent *e) override {
		if (o == reinterpret_cast<QObject *>(mGutter) && e->type() == QEvent::MouseButtonPress) {
			auto *me = static_cast<QMouseEvent *>(e);
			QTextCursor cur = cursorForPosition(me->pos());
			const int line = cur.blockNumber();
			if (mToggle) mToggle(line);
			return true;
		}
		return QPlainTextEdit::eventFilter(o, e);
	}

private:
	void updateGutterWidth(int) {
		setViewportMargins(gutterWidth(), 0, 0, 0);
	}
	void updateGutter(const QRect& r, int dy) {
		if (dy) mGutter->scroll(0, dy);
		else    mGutter->update(0, r.y(), mGutter->width(), r.height());
	}

	void paintEvent(QPaintEvent *e) override {
		QPlainTextEdit::paintEvent(e);
		// Highlight the PC line.
		if (mPcLine >= 0 && mPcLine < blockCount()) {
			QTextBlock blk = document()->findBlockByNumber(mPcLine);
			QRectF r = blockBoundingGeometry(blk).translated(contentOffset());
			QPainter p(viewport());
			p.fillRect(r.toAlignedRect(), QColor(255, 240, 160, 120));
		}
	}

	QWidget *mGutter = nullptr;
	int mPcLine = -1;
	std::function<void(int)> mToggle;
};

class SourcePane : public QWidget {
public:
	SourcePane(QWidget *parent, ATSimulator *sim) : QWidget(parent), mpSim(sim) {
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		mTabs = new QTabWidget(this);
		mTabs->setTabsClosable(true);
		layout->addWidget(mTabs);

		connect(mTabs, &QTabWidget::tabCloseRequested,
			this, [this](int i){ mTabs->removeTab(i); mEdits.erase(mEdits.begin() + i); mFiles.erase(mFiles.begin() + i); });

		mTimer = new QTimer(this);
		mTimer->setInterval(200);
		connect(mTimer, &QTimer::timeout, this, &SourcePane::refresh);
		mTimer->start();

		rebuildFromDebugger();
	}

private:
	void rebuildFromDebugger() {
		IATDebugger *dbg = ATGetDebugger();
		IATDebuggerSymbolLookup *sl = ATGetDebuggerSymbolLookup();
		if (!dbg || !sl) return;

		struct Found { VDStringW path; uint32 mid; uint16 fid; };
		std::vector<Found> files;
		dbg->EnumSourceFiles([&](const wchar_t *path, uint32 /*moduleId*/) {
			uint32 mid = 0;
			uint16 fid = 0;
			if (sl->LookupFile(path, mid, fid)) {
				ATDebuggerSourceFileInfo info;
				VDStringW p;
				if (sl->GetSourceFilePath(mid, fid, info))
					p = info.mSourcePath;
				else
					p = path;
				files.push_back({std::move(p), mid, fid});
			}
		});

		// Add tabs for any new file; keep existing tabs.
		for (const auto& f : files) {
			QString qpath = QString::fromStdWString(f.path.c_str());
			bool already = false;
			for (const auto& t : mFiles) {
				if (t.path == qpath) { already = true; break; }
			}
			if (already) continue;

			QFile fp(qpath);
			QString contents;
			if (fp.open(QIODevice::ReadOnly | QIODevice::Text))
				contents = QString::fromUtf8(fp.readAll());
			else
				contents = QStringLiteral("(could not read source: %1)").arg(qpath);

			auto *edit = new GutteredEdit(this);
			edit->setPlainText(contents);

			SourceTab st;
			st.path     = qpath;
			st.moduleId = f.mid;
			st.fileId   = f.fid;

			edit->setBreakpointToggle([this, st](int line) {
				IATDebugger *dbg = ATGetDebugger();
				IATDebuggerSymbolLookup *sl = ATGetDebuggerSymbolLookup();
				if (!dbg || !sl) return;
				vdfastvector<ATSourceLineInfo> lines;
				sl->GetLinesForFile(st.moduleId, st.fileId, lines);
				for (const auto& li : lines) {
					if ((int)li.mLine == line + 1) {
						dbg->ToggleBreakpoint(li.mOffset);
						break;
					}
				}
			});

			mFiles.push_back(std::move(st));
			mEdits.push_back(edit);
			mTabs->addTab(edit, QFileInfo(qpath).fileName());
		}
	}

	void refresh() {
		IATDebugger *dbg = ATGetDebugger();
		IATDebuggerSymbolLookup *sl = ATGetDebuggerSymbolLookup();
		if (!dbg || !sl) return;
		// Pick up newly-loaded files.
		rebuildFromDebugger();

		// Move PC highlight.
		const uint32 pc = dbg->GetPC();
		uint32 mid = 0;
		ATSourceLineInfo li{};
		if (!sl->LookupLine(pc, false, mid, li)) return;

		for (size_t i = 0; i < mFiles.size(); ++i) {
			if (mFiles[i].moduleId == mid && mFiles[i].fileId == li.mFileId) {
				if (mFiles[i].pcLine != (int)li.mLine - 1) {
					mFiles[i].pcLine = (int)li.mLine - 1;
					mEdits[i]->setPcLine(mFiles[i].pcLine);
					mTabs->setCurrentIndex((int)i);
				}
				return;
			}
		}
	}

	ATSimulator *mpSim = nullptr;
	QTabWidget *mTabs = nullptr;
	QTimer *mTimer = nullptr;
	std::vector<SourceTab> mFiles;
	std::vector<GutteredEdit *> mEdits;
};

QPointer<QDockWidget> gDock;

} // namespace

void ATShowDebuggerSourcePane(QWidget *parent, ATSimulator *sim) {
	if (gDock) {
		gDock->show();
		gDock->raise();
		return;
	}
	auto *mw = qobject_cast<QMainWindow *>(parent);
	if (!mw) return;
	auto *dock = new QDockWidget(QObject::tr("Source"), mw);
	dock->setObjectName(QStringLiteral("debuggerSourceDock"));
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	auto *pane = new SourcePane(dock, sim);
	dock->setWidget(pane);
	mw->addDockWidget(Qt::RightDockWidgetArea, dock);
	gDock = dock;
}
