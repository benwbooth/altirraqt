//	altirraqt — Help dialogs.

#include "helpdialog.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QString>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>
#include <QXmlStreamReader>

#undef signals
#undef slots

#include <vd2/system/vdtypes.h>
#include <versioninfo.h>

namespace {

QString findRepoRoot() {
	// The binary lives at <root>/build/src/altirraqt/altirraqt; walk up
	// until we find a CLAUDE.md or PLAN.md sibling.
	QDir d(QCoreApplication::applicationDirPath());
	for (int i = 0; i < 8 && d.cdUp(); ++i) {
		if (d.exists(QStringLiteral("PLAN.md"))) return d.absolutePath();
	}
	return QString();
}

QPointer<QDialog> gContents;
QPointer<QDialog> gChangelog;

QDialog *makeBrowserDialog(QWidget *parent, const QString& title) {
	auto *dlg = new QDialog(parent);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->setWindowTitle(title);
	dlg->resize(820, 640);
	auto *root = new QVBoxLayout(dlg);
	auto *browser = new QTextBrowser(dlg);
	browser->setOpenExternalLinks(true);
	root->addWidget(browser, 1);
	auto *btnRow = new QHBoxLayout();
	auto *closeBtn = new QPushButton(QObject::tr("Close"));
	btnRow->addStretch();
	btnRow->addWidget(closeBtn);
	root->addLayout(btnRow);
	QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);
	dlg->setProperty("browser", QVariant::fromValue(static_cast<QObject *>(browser)));
	return dlg;
}

uint64 parseVersionXml(const QByteArray& xml, QString& latestText) {
	// Upstream feed schema is a simple XML with <version> and
	// <compare-value> nodes. We accept either form.
	QXmlStreamReader r(xml);
	uint64 cmp = 0;
	while (!r.atEnd() && !r.hasError()) {
		r.readNext();
		if (!r.isStartElement()) continue;
		const QString name = r.name().toString().toLower();
		if (name == QStringLiteral("version")
			|| name == QStringLiteral("title"))
		{
			latestText = r.readElementText().trimmed();
		} else if (name == QStringLiteral("compare-value")
			|| name == QStringLiteral("compare_value"))
		{
			bool ok = false;
			cmp = r.readElementText().trimmed().toULongLong(&ok);
			if (!ok) cmp = 0;
		}
	}
	return cmp;
}

} // namespace

void ATShowHelpContents(QWidget *parent) {
	if (gContents) {
		gContents->raise();
		gContents->activateWindow();
		return;
	}
	auto *dlg = makeBrowserDialog(parent, QObject::tr("Altirra Help"));
	gContents = dlg;
	auto *browser = qobject_cast<QTextBrowser *>(
		dlg->property("browser").value<QObject *>());

	const QString root = findRepoRoot();
	QString text;
	if (!root.isEmpty()) {
		QFile f(root + QStringLiteral("/PLAN.md"));
		if (f.open(QIODevice::ReadOnly | QIODevice::Text))
			text = QString::fromUtf8(f.readAll());
	}
	if (text.isEmpty()) {
		text = QObject::tr(
			"# Altirra (Qt port)\n\n"
			"Atari 8-bit emulator. See PLAN.md and CLAUDE.md in the source\n"
			"tree for the current feature checklist.\n\n"
			"## Keyboard\n\n"
			"- F1: Pulse Warp (held)\n"
			"- F2/F3/F4: Console Start / Select / Option\n"
			"- F5: Warm Reset; Shift+F5: Cold Reset\n"
			"- F6: Help (Atari OS scancode)\n"
			"- F7: Save State; Shift+F7: Load State\n"
			"- F8: Warp Speed toggle\n"
			"- F9: Pause\n"
			"- F11: Quick Save / Full Screen\n"
			"- F12: Capture Mouse\n"
			"- Ctrl+Q: Exit\n");
	}
	browser->setMarkdown(text);
	dlg->show();
}

void ATShowHelpChangeLog(QWidget *parent) {
	if (gChangelog) {
		gChangelog->raise();
		gChangelog->activateWindow();
		return;
	}
	auto *dlg = makeBrowserDialog(parent, QObject::tr("Change Log"));
	gChangelog = dlg;
	auto *browser = qobject_cast<QTextBrowser *>(
		dlg->property("browser").value<QObject *>());

	QString text;
	const QString root = findRepoRoot();
	if (!root.isEmpty()) {
		QProcess p;
		p.setWorkingDirectory(root);
		p.start(QStringLiteral("git"),
			{QStringLiteral("log"),
			 QStringLiteral("--pretty=format:%h  %ad  %s%n  %an"),
			 QStringLiteral("--date=short"),
			 QStringLiteral("-200")});
		if (p.waitForFinished(5000) && p.exitCode() == 0)
			text = QString::fromUtf8(p.readAllStandardOutput());
	}
	if (text.isEmpty())
		text = QObject::tr("(no git log available)");
	browser->setPlainText(text);
	dlg->show();
}

void ATShowHelpCheckForUpdates(QWidget *parent) {
	const QUrl url(QString::fromWCharArray(AT_UPDATE_CHECK_URL_REL));
	auto *mgr = new QNetworkAccessManager();
	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::UserAgentHeader,
		QString::fromWCharArray(AT_HTTP_USER_AGENT));
	auto *reply = mgr->get(req);

	QObject::connect(reply, &QNetworkReply::finished, parent,
		[parent, reply, mgr]{
			QPointer<QWidget> p(parent);
			const auto err = reply->error();
			const QByteArray body = reply->readAll();
			reply->deleteLater();
			mgr->deleteLater();
			if (!p) return;
			if (err != QNetworkReply::NoError) {
				QMessageBox::warning(p, QObject::tr("Check for Updates"),
					QObject::tr("Could not reach update server: %1")
						.arg(reply->errorString()));
				return;
			}
			QString latest;
			const uint64 cmpUpstream = parseVersionXml(body, latest);
			const uint64 cmpLocal = AT_VERSION_COMPARE_VALUE;
			QString msg;
			if (cmpUpstream == 0 && latest.isEmpty()) {
				msg = QObject::tr("Could not parse update feed.");
			} else if (cmpUpstream > cmpLocal) {
				msg = QObject::tr(
					"<p>A newer Altirra build is available: <b>%1</b>.</p>"
					"<p>Download: <a href='%2'>%2</a></p>")
					.arg(latest.isEmpty() ? QObject::tr("(unknown)") : latest,
					     QString::fromWCharArray(AT_DOWNLOAD_URL));
			} else {
				msg = QObject::tr(
					"<p>You're up to date.</p>"
					"<p>Local version: <b>%1</b><br>"
					"Latest published: <b>%2</b></p>")
					.arg(QString::fromUtf8(AT_VERSION), latest);
			}
			QMessageBox::information(p, QObject::tr("Check for Updates"), msg);
		});
}
