//	altirraqt — Keyboard Layout dialog.

#include "keyboardlayoutdialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

namespace {

struct Row {
	const char *name;
	const char *label;
	int         defaultKey;  // Qt::Key
};

constexpr Row kRows[] = {
	{"start",  "Atari Start",  0x01000031 /* Qt::Key_F2 */},
	{"select", "Atari Select", 0x01000032 /* Qt::Key_F3 */},
	{"option", "Atari Option", 0x01000033 /* Qt::Key_F4 */},
	{"reset",  "Atari Reset",  0x01000035 /* Qt::Key_F5 */},
	{"help",   "Atari Help",   0x01000036 /* Qt::Key_F6 */},
	{"break",  "Atari Break",  0x01000020 /* Qt::Key_Pause */},
};

QString keyName(int qtKey) {
	if (qtKey == 0) return QObject::tr("(default)");
	return QKeySequence(qtKey).toString(QKeySequence::NativeText);
}

} // namespace

int ATKeyboardLayoutGetOverride(QSettings& s, const char *name) {
	if (!name) return 0;
	const QString key = QStringLiteral("input/keyboard/") + QString::fromUtf8(name);
	return s.value(key, 0).toInt();
}

void ATShowKeyboardLayoutDialog(QWidget *parent, QSettings *settings) {
	QDialog dlg(parent);
	dlg.setWindowTitle(QObject::tr("Keyboard Layout"));
	dlg.resize(420, 320);

	auto *root = new QVBoxLayout(&dlg);
	auto *intro = new QLabel(QObject::tr(
		"Override the host key that produces each Atari console action. "
		"Empty = use the built-in default."));
	intro->setWordWrap(true);
	root->addWidget(intro);

	auto *form = new QFormLayout();
	std::vector<QKeySequenceEdit *> edits;
	for (const auto& r : kRows) {
		auto *edit = new QKeySequenceEdit(&dlg);
		edit->setMaximumSequenceLength(1);
		const int saved = settings ? ATKeyboardLayoutGetOverride(*settings, r.name) : 0;
		if (saved)
			edit->setKeySequence(QKeySequence(saved));
		form->addRow(QObject::tr(r.label) + QStringLiteral("  (default: ")
			+ keyName(r.defaultKey) + QStringLiteral(")"), edit);
		edits.push_back(edit);
	}
	root->addLayout(form);

	auto *btnBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel
		| QDialogButtonBox::RestoreDefaults, &dlg);
	root->addWidget(btnBox);

	QObject::connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
	QObject::connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	QObject::connect(btnBox->button(QDialogButtonBox::RestoreDefaults),
		&QPushButton::clicked, &dlg, [&edits]{
			for (auto *e : edits) e->clear();
		});

	if (dlg.exec() != QDialog::Accepted) return;
	if (!settings) return;
	for (size_t i = 0; i < std::size(kRows); ++i) {
		const auto seq = edits[i]->keySequence();
		const QString key = QStringLiteral("input/keyboard/")
			+ QString::fromUtf8(kRows[i].name);
		if (seq.isEmpty())
			settings->remove(key);
		else
			settings->setValue(key, (int)seq[0].key());
	}
}
