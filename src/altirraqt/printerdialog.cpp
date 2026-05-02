//	Altirra - Qt port
//	Printer Output dialog implementation.
//
//	Wires Qt's QPlainTextEdit to the simulator's IATPrinterOutputManager.
//	The manager emits OnAddedOutput/OnRemovingOutput notifications when a
//	printer device creates or destroys a text output. We attach to the
//	first text output by default (matching upstream), and let the user
//	switch between outputs via a combo box if there are multiple.
//
//	Each ATPrinterOutput accumulates printed bytes in an internal Unicode
//	buffer. We register an OnInvalidation callback; on each fire, we copy
//	the new tail (mLastTextOffset .. GetLength()) into the QPlainTextEdit.
//	This is the same flow as upstream's ATPrinterOutputWindow.
//
//	Lifecycle: callbacks are unregistered in the destructor. The dialog
//	itself is created lazily on first menu invocation and held in a
//	function-local static, so it persists across show/close cycles. Closing
//	the window only hides it; destruction happens at process shutdown.

#include "printerdialog.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QString>
#include <QVBoxLayout>

#include <algorithm>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/function.h>
#include <vd2/system/refcount.h>
#include <vd2/system/VDString.h>
#include <at/atcore/deviceprinter.h>
#include <irqcontroller.h>
#include <printeroutput.h>
#include <simulator.h>

namespace {

class ATPrinterOutputDialog : public QDialog {
public:
	ATPrinterOutputDialog(QWidget *parent, ATSimulator *sim)
		: QDialog(parent), mpSim(sim)
	{
		setWindowTitle(tr("Printer Output"));
		// Non-modal: the simulator keeps running while the window is open.
		setModal(false);
		// Don't auto-delete on close — we keep the dialog around so output
		// accumulates across show/hide cycles and the user's switch state
		// is preserved.
		setAttribute(Qt::WA_DeleteOnClose, false);
		resize(720, 480);

		auto *layout = new QVBoxLayout(this);

		auto *headerRow = new QHBoxLayout;
		headerRow->addWidget(new QLabel(tr("Output:"), this));
		mpOutputCombo = new QComboBox(this);
		mpOutputCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		headerRow->addWidget(mpOutputCombo, 1);
		layout->addLayout(headerRow);

		mpEditor = new QPlainTextEdit(this);
		mpEditor->setReadOnly(true);
		mpEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
		mpEditor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		layout->addWidget(mpEditor, 1);

		auto *buttonRow = new QHBoxLayout;
		buttonRow->addStretch(1);
		mpClearButton = new QPushButton(tr("&Clear"), this);
		mpSaveButton  = new QPushButton(tr("&Save..."), this);
		mpCloseButton = new QPushButton(tr("Close"), this);
		buttonRow->addWidget(mpClearButton);
		buttonRow->addWidget(mpSaveButton);
		buttonRow->addWidget(mpCloseButton);
		layout->addLayout(buttonRow);

		connect(mpClearButton, &QPushButton::clicked, this, &ATPrinterOutputDialog::onClear);
		connect(mpSaveButton,  &QPushButton::clicked, this, &ATPrinterOutputDialog::onSave);
		connect(mpCloseButton, &QPushButton::clicked, this, &QDialog::close);
		connect(mpOutputCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		        this, &ATPrinterOutputDialog::onOutputSelected);

		// Bind the manager listeners. ATNotifyList stores raw pointers to
		// vdfunctions; we keep them as members so the addresses remain
		// valid for the lifetime of the dialog.
		mAddedOutputFn = [this](ATPrinterOutput& o) { onAddedOutput(o); };
		mRemovingOutputFn = [this](ATPrinterOutput& o) { onRemovingOutput(o); };

		mpOutputMgr = static_cast<ATPrinterOutputManager *>(&mpSim->GetPrinterOutputManager());
		mpOutputMgr->OnAddedOutput.Add(&mAddedOutputFn);
		mpOutputMgr->OnRemovingOutput.Add(&mRemovingOutputFn);

		// Pick up any pre-existing text outputs.
		for (uint32 i = 0, n = mpOutputMgr->GetOutputCount(); i < n; ++i)
			addOutputToCombo(&mpOutputMgr->GetOutput(i));

		updatePlaceholder();

		if (mpOutputCombo->count() > 0)
			mpOutputCombo->setCurrentIndex(0);
	}

	~ATPrinterOutputDialog() override {
		detachFromOutput();

		if (mpOutputMgr) {
			mpOutputMgr->OnAddedOutput.Remove(&mAddedOutputFn);
			mpOutputMgr->OnRemovingOutput.Remove(&mRemovingOutputFn);
			mpOutputMgr = nullptr;
		}
	}

private:
	void addOutputToCombo(ATPrinterOutput *output) {
		const VDStringW name(output->GetName());
		mpOutputCombo->addItem(QString::fromWCharArray(name.c_str(), (int)name.size()),
		                      QVariant::fromValue<void *>(output));
	}

	int findOutputIndex(ATPrinterOutput *output) const {
		const int n = mpOutputCombo->count();
		for (int i = 0; i < n; ++i) {
			if (mpOutputCombo->itemData(i).value<void *>() == output)
				return i;
		}
		return -1;
	}

	void onAddedOutput(ATPrinterOutput& output) {
		addOutputToCombo(&output);

		// If we weren't attached to anything, attach to this new output.
		if (!mpAttached)
			mpOutputCombo->setCurrentIndex(mpOutputCombo->count() - 1);
		else
			updatePlaceholder();
	}

	void onRemovingOutput(ATPrinterOutput& output) {
		const int idx = findOutputIndex(&output);
		if (idx < 0)
			return;

		const bool wasAttached = (mpAttached == &output);

		// Drop our attachment first so detachFromOutput() doesn't try to
		// touch a dying object via the combo's selection change.
		if (wasAttached)
			detachFromOutput();

		// Remove from combo. This may trigger currentIndexChanged, which
		// our handler will treat as a no-op since mpAttached is null.
		mpOutputCombo->removeItem(idx);

		updatePlaceholder();

		// Re-attach to whatever's selected now, if anything.
		if (wasAttached && mpOutputCombo->count() > 0)
			onOutputSelected(mpOutputCombo->currentIndex());
	}

	void onOutputSelected(int idx) {
		ATPrinterOutput *target = nullptr;
		if (idx >= 0 && idx < mpOutputCombo->count())
			target = static_cast<ATPrinterOutput *>(
				mpOutputCombo->itemData(idx).value<void *>());

		if (target == mpAttached)
			return;

		detachFromOutput();
		mpEditor->clear();

		if (target)
			attachToOutput(*target);
	}

	void attachToOutput(ATPrinterOutput& output) {
		mpAttached = &output;
		mLastTextOffset = 0;

		mpAttached->SetOnInvalidation([this] { syncFromOutput(); });

		// Pull whatever has accumulated already.
		syncFromOutput();
	}

	void detachFromOutput() {
		if (mpAttached) {
			mpAttached->SetOnInvalidation(nullptr);
			mpAttached = nullptr;
		}
		mLastTextOffset = 0;
	}

	void syncFromOutput() {
		if (!mpAttached)
			return;

		const size_t total = mpAttached->GetLength();
		if (total > mLastTextOffset) {
			const wchar_t *tail = mpAttached->GetTextPointer(mLastTextOffset);
			const size_t tailLen = total - mLastTextOffset;

			QString chunk = QString::fromWCharArray(tail, (int)tailLen);

			// QPlainTextEdit::appendPlainText always inserts a paragraph
			// separator; we want raw insertion so embedded \n and trailing
			// state are preserved. Use insertPlainText at the end.
			QTextCursor cursor = mpEditor->textCursor();
			cursor.movePosition(QTextCursor::End);
			cursor.insertText(chunk);
			mpEditor->setTextCursor(cursor);
			mpEditor->ensureCursorVisible();

			mLastTextOffset = total;
		}

		mpAttached->Revalidate();
	}

	void onClear() {
		mpEditor->clear();
		mLastTextOffset = 0;

		if (mpAttached) {
			mpAttached->Clear();
			mpAttached->Revalidate();
		}
	}

	void onSave() {
		const QString path = QFileDialog::getSaveFileName(
			this,
			tr("Save Printer Output"),
			QStringLiteral("printer-output.txt"),
			tr("Text files (*.txt);;All files (*)"));
		if (path.isEmpty())
			return;

		QSaveFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QMessageBox::warning(this, tr("Save Printer Output"),
				tr("Could not open file for writing:\n%1").arg(file.errorString()));
			return;
		}

		const QByteArray bytes = mpEditor->toPlainText().toUtf8();
		if (file.write(bytes) != bytes.size() || !file.commit()) {
			QMessageBox::warning(this, tr("Save Printer Output"),
				tr("Could not write file:\n%1").arg(file.errorString()));
			return;
		}
	}

	void updatePlaceholder() {
		if (mpOutputCombo->count() == 0) {
			mpEditor->setPlaceholderText(tr(
				"No printer outputs yet.\n\n"
				"Attach a printer device under System -> Configure System "
				"-> Devices to start collecting output."));
		} else {
			mpEditor->setPlaceholderText(QString());
		}
	}

	ATSimulator *mpSim = nullptr;
	ATPrinterOutputManager *mpOutputMgr = nullptr;
	ATPrinterOutput *mpAttached = nullptr;
	size_t mLastTextOffset = 0;

	QComboBox       *mpOutputCombo = nullptr;
	QPlainTextEdit  *mpEditor = nullptr;
	QPushButton     *mpClearButton = nullptr;
	QPushButton     *mpSaveButton = nullptr;
	QPushButton     *mpCloseButton = nullptr;

	vdfunction<void(ATPrinterOutput&)> mAddedOutputFn;
	vdfunction<void(ATPrinterOutput&)> mRemovingOutputFn;
};

ATPrinterOutputDialog *g_printerDialog = nullptr;

}  // namespace

void ATShowPrinterOutputDialog(QWidget *parent, ATSimulator *sim) {
	if (!g_printerDialog)
		g_printerDialog = new ATPrinterOutputDialog(parent, sim);

	g_printerDialog->show();
	g_printerDialog->raise();
	g_printerDialog->activateWindow();
}
