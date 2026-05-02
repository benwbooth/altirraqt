//	Altirra - Qt port
//	Profiles editor dialog implementation.

#include "profileseditordialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

// Pull every profile name out of the settings store. Profile keys live
// under "profiles/<name>/<subkey>"; we collect the unique <name> set.
QStringList collectProfileNames(QSettings& settings) {
	settings.sync();
	const QStringList keys = settings.allKeys();
	QStringList names;
	for (const QString& k : keys) {
		if (!k.startsWith(QStringLiteral("profiles/"))) continue;
		const int slash = k.indexOf(QLatin1Char('/'), 9);
		if (slash < 0) continue;
		const QString name = k.mid(9, slash - 9);
		if (!names.contains(name))
			names.append(name);
	}
	names.sort(Qt::CaseInsensitive);
	return names;
}

int countProfileKeys(QSettings& settings, const QString& name) {
	const QString prefix = QStringLiteral("profiles/") + name + QStringLiteral("/");
	int n = 0;
	for (const QString& k : settings.allKeys())
		if (k.startsWith(prefix))
			++n;
	return n;
}

// Move every "profiles/<oldName>/X" key to "profiles/<newName>/X". If
// system/lastProfile pointed at <oldName>, retarget it.
void renameProfile(QSettings& settings, const QString& oldName, const QString& newName) {
	if (oldName == newName) return;
	const QString oldPrefix = QStringLiteral("profiles/") + oldName + QStringLiteral("/");
	const QString newPrefix = QStringLiteral("profiles/") + newName + QStringLiteral("/");
	const QStringList keys = settings.allKeys();
	for (const QString& k : keys) {
		if (!k.startsWith(oldPrefix)) continue;
		const QString sub = k.mid(oldPrefix.length());
		settings.setValue(newPrefix + sub, settings.value(k));
		settings.remove(k);
	}
	if (settings.value(QStringLiteral("system/lastProfile")).toString() == oldName)
		settings.setValue(QStringLiteral("system/lastProfile"), newName);
}

void duplicateProfile(QSettings& settings, const QString& src, const QString& dst) {
	const QString srcPrefix = QStringLiteral("profiles/") + src + QStringLiteral("/");
	const QString dstPrefix = QStringLiteral("profiles/") + dst + QStringLiteral("/");
	const QStringList keys = settings.allKeys();
	for (const QString& k : keys) {
		if (!k.startsWith(srcPrefix)) continue;
		const QString sub = k.mid(srcPrefix.length());
		settings.setValue(dstPrefix + sub, settings.value(k));
	}
}

void deleteProfile(QSettings& settings, const QString& name) {
	const QString prefix = QStringLiteral("profiles/") + name + QStringLiteral("/");
	const QStringList keys = settings.allKeys();
	for (const QString& k : keys) {
		if (k.startsWith(prefix))
			settings.remove(k);
	}
	if (settings.value(QStringLiteral("system/lastProfile")).toString() == name)
		settings.remove(QStringLiteral("system/lastProfile"));
}

class ATProfilesEditorDialog : public QDialog {
public:
	ATProfilesEditorDialog(QWidget *parent, QSettings *settings)
		: QDialog(parent), mpSettings(settings)
	{
		setWindowTitle(tr("Edit Profiles"));
		resize(520, 360);

		auto *layout = new QVBoxLayout(this);

		mpTable = new QTableWidget(0, 3, this);
		mpTable->setHorizontalHeaderLabels({tr("Profile"), tr("# of keys"), tr("Last loaded")});
		mpTable->horizontalHeader()->setStretchLastSection(false);
		mpTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		mpTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
		mpTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
		mpTable->verticalHeader()->setVisible(false);
		mpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		mpTable->setSelectionMode(QAbstractItemView::SingleSelection);
		layout->addWidget(mpTable, 1);

		auto *btnRow = new QHBoxLayout;
		auto *renameBtn    = new QPushButton(tr("Rename"), this);
		auto *duplicateBtn = new QPushButton(tr("Duplicate..."), this);
		auto *deleteBtn    = new QPushButton(tr("Delete"), this);
		btnRow->addWidget(renameBtn);
		btnRow->addWidget(duplicateBtn);
		btnRow->addWidget(deleteBtn);
		btnRow->addStretch(1);
		layout->addLayout(btnRow);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Close, this);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::accept);
		layout->addWidget(box);

		connect(renameBtn,    &QPushButton::clicked, this, &ATProfilesEditorDialog::onRename);
		connect(duplicateBtn, &QPushButton::clicked, this, &ATProfilesEditorDialog::onDuplicate);
		connect(deleteBtn,    &QPushButton::clicked, this, &ATProfilesEditorDialog::onDelete);

		reload();
	}

private:
	void reload() {
		if (!mpSettings) return;
		const QStringList names = collectProfileNames(*mpSettings);
		const QString last = mpSettings->value(QStringLiteral("system/lastProfile")).toString();

		mReloading = true;
		mpTable->setRowCount(names.size());
		for (int i = 0; i < names.size(); ++i) {
			auto *nameItem = new QTableWidgetItem(names[i]);
			// Stash the original name so onRename can find it.
			nameItem->setData(Qt::UserRole, names[i]);
			nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
			mpTable->setItem(i, 0, nameItem);

			auto *countItem = new QTableWidgetItem(QString::number(countProfileKeys(*mpSettings, names[i])));
			countItem->setFlags(countItem->flags() & ~Qt::ItemIsEditable);
			countItem->setTextAlignment(Qt::AlignCenter);
			mpTable->setItem(i, 1, countItem);

			auto *lastItem = new QTableWidgetItem(names[i] == last ? tr("yes") : QString());
			lastItem->setFlags(lastItem->flags() & ~Qt::ItemIsEditable);
			lastItem->setTextAlignment(Qt::AlignCenter);
			mpTable->setItem(i, 2, lastItem);
		}
		mReloading = false;
	}

	QString currentName() const {
		const int row = mpTable->currentRow();
		if (row < 0) return {};
		auto *item = mpTable->item(row, 0);
		if (!item) return {};
		// UserRole holds the original name (resilient to in-place edits).
		return item->data(Qt::UserRole).toString();
	}

	void onRename() {
		if (!mpSettings) return;
		const int row = mpTable->currentRow();
		if (row < 0) return;
		auto *item = mpTable->item(row, 0);
		if (!item) return;
		const QString oldName = item->data(Qt::UserRole).toString();
		const QString newName = item->text().trimmed();
		if (newName.isEmpty()) {
			QMessageBox::warning(this, tr("Rename profile"),
				tr("Profile name cannot be empty."));
			item->setText(oldName);
			return;
		}
		if (newName == oldName) return;
		if (newName.contains(QLatin1Char('/'))) {
			QMessageBox::warning(this, tr("Rename profile"),
				tr("Profile name cannot contain a slash."));
			item->setText(oldName);
			return;
		}
		const QStringList existing = collectProfileNames(*mpSettings);
		if (existing.contains(newName, Qt::CaseInsensitive)) {
			QMessageBox::warning(this, tr("Rename profile"),
				tr("A profile named \"%1\" already exists.").arg(newName));
			item->setText(oldName);
			return;
		}
		renameProfile(*mpSettings, oldName, newName);
		mpSettings->sync();
		reload();
	}

	void onDuplicate() {
		if (!mpSettings) return;
		const QString src = currentName();
		if (src.isEmpty()) {
			QMessageBox::information(this, tr("Duplicate profile"),
				tr("Select a profile to duplicate."));
			return;
		}
		bool ok = false;
		const QString dst = QInputDialog::getText(this,
			tr("Duplicate profile"),
			tr("New profile name:"),
			QLineEdit::Normal,
			src + tr(" (copy)"), &ok).trimmed();
		if (!ok || dst.isEmpty()) return;
		if (dst.contains(QLatin1Char('/'))) {
			QMessageBox::warning(this, tr("Duplicate profile"),
				tr("Profile name cannot contain a slash."));
			return;
		}
		const QStringList existing = collectProfileNames(*mpSettings);
		if (existing.contains(dst, Qt::CaseInsensitive)) {
			QMessageBox::warning(this, tr("Duplicate profile"),
				tr("A profile named \"%1\" already exists.").arg(dst));
			return;
		}
		duplicateProfile(*mpSettings, src, dst);
		mpSettings->sync();
		reload();
	}

	void onDelete() {
		if (!mpSettings) return;
		const QString name = currentName();
		if (name.isEmpty()) {
			QMessageBox::information(this, tr("Delete profile"),
				tr("Select a profile to delete."));
			return;
		}
		const auto reply = QMessageBox::question(this, tr("Delete profile"),
			tr("Delete profile \"%1\"? This cannot be undone.").arg(name),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (reply != QMessageBox::Yes) return;
		deleteProfile(*mpSettings, name);
		mpSettings->sync();
		reload();
	}

	QSettings    *mpSettings;
	QTableWidget *mpTable = nullptr;
	bool          mReloading = false;
};

} // namespace

void ATShowProfilesEditorDialog(QWidget *parent, QSettings *settings) {
	if (!settings) return;
	ATProfilesEditorDialog dlg(parent, settings);
	dlg.exec();
}
