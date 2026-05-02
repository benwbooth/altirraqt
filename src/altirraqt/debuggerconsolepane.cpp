//	Altirra - Qt port
//	Debugger Console pane implementation.
//
//	Hosts a QPlainTextEdit output area + QLineEdit command line in a
//	QDockWidget docked on the QMainWindow's bottom area. Output flows in
//	via ATDebuggerConsoleSink(), which is called by ATConsoleWrite() in
//	stubs.cpp; commands flow out via ATConsoleExecuteCommand().
//
//	Lifecycle:
//	  - The dock and its inner widget are created lazily on first show
//	    and parented to the main window.
//	  - The pane registers itself with a process-global listener slot;
//	    only one pane is ever active at a time. Re-opening the pane
//	    after closing reuses the same instance (stored in a function-
//	    local static).

#include "debuggerconsolepane.h"

#include <QCoreApplication>
#include <QDockWidget>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWidget>

#include <atomic>
#include <cstdio>

// ATConsoleExecuteCommand lives in the upstream debugger.cpp (now compiled
// in). It isn't declared in any public header — declare it locally.
extern void ATConsoleExecuteCommand(const char *s, bool echo);

namespace {

class ATDebuggerConsolePane;

// Global, single-active-listener pointer. ATDebuggerConsoleSink reads it.
// Atomic because ATConsoleWrite can be called from any thread that touches
// the simulator; the Qt UI updates are dispatched onto the GUI thread via
// queued connection.
std::atomic<ATDebuggerConsolePane *> g_console{nullptr};

class ATDebuggerConsolePane : public QWidget {
public:
	explicit ATDebuggerConsolePane(QWidget *parent)
		: QWidget(parent)
	{
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);
		layout->setSpacing(2);

		mpOutput = new QPlainTextEdit(this);
		mpOutput->setReadOnly(true);
		mpOutput->setLineWrapMode(QPlainTextEdit::NoWrap);
		mpOutput->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		mpOutput->setMaximumBlockCount(10000);
		layout->addWidget(mpOutput, 1);

		mpEntry = new QLineEdit(this);
		mpEntry->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		mpEntry->setPlaceholderText(tr("Enter debugger command..."));
		mpEntry->installEventFilter(this);
		layout->addWidget(mpEntry, 0);

		connect(mpEntry, &QLineEdit::returnPressed, this, &ATDebuggerConsolePane::onSubmit);
	}

	~ATDebuggerConsolePane() override {
		// Drop the global listener pointer if it's us.
		ATDebuggerConsolePane *self = this;
		g_console.compare_exchange_strong(self, nullptr);
	}

	void registerAsSink() {
		g_console.store(this);
	}

	// Marshal text into the QPlainTextEdit. Always invoked on the GUI
	// thread because we hop via QMetaObject::invokeMethod from the sink.
	void appendText(QString s) {
		QTextCursor cursor = mpOutput->textCursor();
		cursor.movePosition(QTextCursor::End);
		cursor.insertText(s);
		mpOutput->setTextCursor(cursor);
		mpOutput->ensureCursorVisible();
	}

protected:
	bool eventFilter(QObject *watched, QEvent *event) override {
		if (watched == mpEntry && event->type() == QEvent::KeyPress) {
			auto *ke = static_cast<QKeyEvent *>(event);
			if (ke->key() == Qt::Key_Up) {
				historyPrev();
				return true;
			}
			if (ke->key() == Qt::Key_Down) {
				historyNext();
				return true;
			}
		}
		return QWidget::eventFilter(watched, event);
	}

private:
	void onSubmit() {
		const QString text = mpEntry->text();
		if (!text.isEmpty()) {
			mHistory.append(text);
			if (mHistory.size() > 200) mHistory.removeFirst();
			mHistoryIdx = mHistory.size();
		}
		mpEntry->clear();

		const QByteArray utf8 = text.toUtf8();
		// echo=true so the prompt and command are written to the output
		// via ATConsoleWrite (which routes back to us).
		ATConsoleExecuteCommand(utf8.constData(), true);
	}

	void historyPrev() {
		if (mHistory.isEmpty()) return;
		if (mHistoryIdx > 0) --mHistoryIdx;
		mpEntry->setText(mHistory.value(mHistoryIdx));
	}

	void historyNext() {
		if (mHistoryIdx < mHistory.size() - 1) {
			++mHistoryIdx;
			mpEntry->setText(mHistory.value(mHistoryIdx));
		} else {
			mHistoryIdx = mHistory.size();
			mpEntry->clear();
		}
	}

	QPlainTextEdit *mpOutput = nullptr;
	QLineEdit      *mpEntry = nullptr;
	QStringList     mHistory;
	int             mHistoryIdx = 0;
};

QDockWidget *g_dock = nullptr;
ATDebuggerConsolePane *g_pane = nullptr;

}  // namespace

bool ATDebuggerConsoleSink(const char *s) {
	if (!s) return false;
	auto *p = g_console.load();
	if (!p) return false;

	// Marshal onto the GUI thread; ATConsoleWrite may be called from a
	// background simulator-step path.
	QString text = QString::fromUtf8(s);
	QMetaObject::invokeMethod(p, [p, text = std::move(text)]() mutable {
		p->appendText(std::move(text));
	}, Qt::QueuedConnection);
	return true;
}

void ATShowDebuggerConsolePane(QMainWindow *parent) {
	if (!g_dock) {
		g_dock = new QDockWidget(QObject::tr("Debugger Console"), parent);
		g_dock->setObjectName(QStringLiteral("DebuggerConsoleDock"));
		g_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
		g_pane = new ATDebuggerConsolePane(g_dock);
		g_dock->setWidget(g_pane);
		parent->addDockWidget(Qt::BottomDockWidgetArea, g_dock);
		g_pane->registerAsSink();
	}
	g_dock->show();
	g_dock->raise();
}
