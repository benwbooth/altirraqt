//	Altirra - Qt port
//	Joystick Keys dialog implementation.

#include "joystickkeysdialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "joystickkeys.h"

namespace {

QString labelForKey(int key) {
	if (key == 0) return QObject::tr("(unset)");
	return QKeySequence(key).toString(QKeySequence::NativeText);
}

class ATJoystickKeysDialog : public QDialog {
public:
	ATJoystickKeysDialog(QWidget *parent, QSettings *settings)
		: QDialog(parent), mpSettings(settings)
	{
		setWindowTitle(tr("Joystick Keys"));
		resize(380, 280);

		mWorking = g_atJoyKeys;

		auto *layout = new QVBoxLayout(this);
		auto *intro = new QLabel(
			tr("Pick a port, click a button, then press the host key to bind "
			   "it to that joystick direction or fire button. Press Esc to "
			   "clear."));
		intro->setWordWrap(true);
		layout->addWidget(intro);

		auto *portRow = new QHBoxLayout;
		portRow->addWidget(new QLabel(tr("Port:")));
		mpPortCombo = new QComboBox(this);
		for (int p = 0; p < kATJoyPortCount; ++p)
			mpPortCombo->addItem(tr("Port %1").arg(p + 1));
		portRow->addWidget(mpPortCombo);
		portRow->addStretch();
		layout->addLayout(portRow);

		auto *form = new QFormLayout;
		layout->addLayout(form);
		for (int i = 0; i < kATJoyKeyCount; ++i) {
			mpButtons[i] = new QPushButton(labelForKey(mWorking[0][i]));
			mpButtons[i]->setCheckable(true);
			form->addRow(QString::fromUtf8(ATJoyKeyLabel((ATJoyKeyIndex)i)) + ":",
			             mpButtons[i]);
			connect(mpButtons[i], &QPushButton::toggled,
				[this, i](bool on){ if (on) startCapture(i); });
		}

		connect(mpPortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &ATJoystickKeysDialog::onPortChanged);

		auto *box = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel
			| QDialogButtonBox::Reset);
		layout->addWidget(box);
		connect(box, &QDialogButtonBox::accepted, this, &ATJoystickKeysDialog::onAccept);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
		connect(box->button(QDialogButtonBox::Reset), &QPushButton::clicked,
			this, &ATJoystickKeysDialog::onResetDefaults);
	}

protected:
	void keyPressEvent(QKeyEvent *e) override {
		if (mCapturing < 0) {
			QDialog::keyPressEvent(e);
			return;
		}
		// Treat modifier-only keypresses as the bound key (so users can
		// bind Alt/Ctrl/Shift). Esc clears the binding.
		const int k = e->key();
		const int port = mpPortCombo->currentIndex();
		if (k == Qt::Key_Escape) {
			mWorking[port][mCapturing] = 0;
		} else {
			mWorking[port][mCapturing] = k;
		}
		mpButtons[mCapturing]->setText(labelForKey(mWorking[port][mCapturing]));
		mpButtons[mCapturing]->setChecked(false);
		mCapturing = -1;
		e->accept();
	}

private:
	void startCapture(int i) {
		if (mCapturing >= 0 && mCapturing != i)
			mpButtons[mCapturing]->setChecked(false);
		mCapturing = i;
		setFocus();
	}

	void onPortChanged(int port) {
		// Cancel any in-progress capture and refresh the visible labels.
		if (mCapturing >= 0) {
			mpButtons[mCapturing]->setChecked(false);
			mCapturing = -1;
		}
		if (port < 0 || port >= kATJoyPortCount) return;
		for (int i = 0; i < kATJoyKeyCount; ++i)
			mpButtons[i]->setText(labelForKey(mWorking[port][i]));
	}

	void onAccept() {
		g_atJoyKeys = mWorking;
		if (mpSettings) ATSaveJoyKeys(*mpSettings);
		accept();
	}

	void onResetDefaults() {
		const int port = mpPortCombo->currentIndex();
		if (port == 0) {
			mWorking[0] = {
				Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right, Qt::Key_Alt,
			};
		} else if (port >= 0 && port < kATJoyPortCount) {
			mWorking[port] = { 0, 0, 0, 0, 0 };
		}
		for (int i = 0; i < kATJoyKeyCount; ++i)
			mpButtons[i]->setText(labelForKey(mWorking[port][i]));
	}

	QSettings *mpSettings;
	QComboBox *mpPortCombo {};
	QPushButton *mpButtons[kATJoyKeyCount] {};
	std::array<std::array<int, kATJoyKeyCount>, kATJoyPortCount> mWorking;
	int mCapturing = -1;
};

} // namespace

void ATShowJoystickKeysDialog(QWidget *parent, QSettings *settings) {
	ATJoystickKeysDialog dlg(parent, settings);
	dlg.exec();
}
