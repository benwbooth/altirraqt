//	Altirra - Qt port
//	HUD customize dialog implementation.

#include "customizehuddialog.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

#include <at/qtdisplay/qtvideodisplay.h>

namespace {

struct HudElement {
	const char *key;     // QSettings key suffix and QtDisplay element name
	const char *label;   // dialog text
};

constexpr HudElement kElements[] = {
	{"fps",      "FPS counter (top-right)"},
	{"drives",   "Drive activity (bottom-right)"},
	{"cassette", "Cassette indicator (bottom-left)"},
	{"console",  "Console-button hints (top-left)"},
	{"pads",     "Pad/joystick position (top-centre)"},
};

} // namespace

void ATApplyHudSettings(QWidget *displayWidget, QSettings *settings) {
	if (!displayWidget || !settings) return;
	for (const auto& e : kElements) {
		const QString key = QStringLiteral("view/hud/") + QLatin1String(e.key);
		const bool on = settings->value(key, false).toBool();
		ATQtVideoDisplaySetHudEnabled(displayWidget, e.key, on);
	}
}

void ATShowCustomizeHudDialog(QWidget *parent, QWidget *displayWidget, QSettings *settings) {
	if (!displayWidget) return;

	QDialog dlg(parent);
	dlg.setWindowTitle(QObject::tr("Customize HUD"));
	dlg.resize(360, 260);

	auto *layout = new QVBoxLayout(&dlg);
	auto *intro = new QLabel(QObject::tr(
		"Choose which on-screen overlay elements appear on top of the\n"
		"emulator picture. The status bar continues to show all of them."));
	intro->setWordWrap(true);
	layout->addWidget(intro);

	std::vector<QCheckBox *> checks;
	for (const auto& e : kElements) {
		auto *cb = new QCheckBox(QObject::tr(e.label));
		const QString key = QStringLiteral("view/hud/") + QLatin1String(e.key);
		const bool on = settings ? settings->value(key, false).toBool() : false;
		cb->setChecked(on);
		layout->addWidget(cb);
		checks.push_back(cb);
	}

	auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	layout->addWidget(box);
	QObject::connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

	if (dlg.exec() != QDialog::Accepted) return;

	for (size_t i = 0; i < checks.size(); ++i) {
		const bool on = checks[i]->isChecked();
		const auto& e = kElements[i];
		ATQtVideoDisplaySetHudEnabled(displayWidget, e.key, on);
		if (settings)
			settings->setValue(QStringLiteral("view/hud/") + QLatin1String(e.key), on);
	}
}
