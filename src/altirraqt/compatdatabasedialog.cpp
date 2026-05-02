//	altirraqt — Compatibility database viewer dialog.

#include "compatdatabasedialog.h"
#include "compat_qt.h"

#include <QAbstractTableModel>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTableView>
#include <QVBoxLayout>

#undef signals
#undef slots

#include <vd2/system/vdtypes.h>
#include <vd2/system/vdstl.h>
#include <compatdb.h>

namespace {

// Row entry holding precomputed strings for fast filter / display.
struct Entry {
	QString name;
	QString tags;          // joined "tag1, tag2, …"
	QString sample;        // first rule value (sha-256 hex or 8-byte hex), if any
	const ATCompatDBTitle *pTitle = nullptr;
};

class CompatModel : public QAbstractTableModel {
public:
	using QAbstractTableModel::QAbstractTableModel;

	int rowCount(const QModelIndex& parent = {}) const override {
		return parent.isValid() ? 0 : (int)mRows.size();
	}
	int columnCount(const QModelIndex& parent = {}) const override {
		return parent.isValid() ? 0 : 3;
	}

	QVariant headerData(int section, Qt::Orientation o, int role) const override {
		if (role != Qt::DisplayRole || o != Qt::Horizontal) return {};
		switch (section) {
			case 0: return QStringLiteral("Title");
			case 1: return QStringLiteral("Tags");
			case 2: return QStringLiteral("Sample SHA-256 / Checksum");
		}
		return {};
	}

	QVariant data(const QModelIndex& idx, int role) const override {
		if (!idx.isValid() || role != Qt::DisplayRole) return {};
		const auto& e = mRows[(size_t)idx.row()];
		switch (idx.column()) {
			case 0: return e.name;
			case 1: return e.tags;
			case 2: return e.sample;
		}
		return {};
	}

	void rebuild() {
		beginResetModel();
		mRows.clear();
		const ATCompatDBHeader *real = ATCompatGetHeader();
		if (!real) { endResetModel(); return; }

		const auto& titles = real->mTitleTable;
		const auto& tags   = real->mTagTable;
		for (uint32 i = 0; i < titles.size(); ++i) {
			const ATCompatDBTitle& t = titles.data()[i];
			Entry e;
			e.pTitle = &t;
			e.name   = QString::fromUtf8(t.mName.c_str());
			QStringList tagNames;
			for (const auto& id : t.mTagIds) {
				if (id < tags.size())
					tagNames << QString::fromUtf8(tags.data()[id].mKey.c_str());
			}
			e.tags = tagNames.join(QStringLiteral(", "));

			// Sample: scan rules for one referencing this title (via
			// alias). First match wins.
			for (uint32 r = 0; r < real->mRuleTable.size(); ++r) {
				const auto& rule = real->mRuleTable.data()[r];
				if (rule.mAliasId >= real->mAliasTable.size()) continue;
				const auto& alias = real->mAliasTable.data()[rule.mAliasId];
				if (alias.mTitleId != i) continue;
				e.sample = QStringLiteral("%1:%2")
				              .arg((uint32)rule.mValueHi, 8, 16, QLatin1Char('0'))
				              .arg(rule.mValueLo, 8, 16, QLatin1Char('0'));
				break;
			}
			mRows.push_back(std::move(e));
		}
		endResetModel();
	}

	const Entry *entryAt(int row) const {
		if (row < 0 || (size_t)row >= mRows.size()) return nullptr;
		return &mRows[(size_t)row];
	}

private:
	std::vector<Entry> mRows;
};

QPointer<QDialog> gDialog;

} // namespace

void ATShowCompatDatabaseDialog(QWidget *parent) {
	if (gDialog) {
		gDialog->raise();
		gDialog->activateWindow();
		return;
	}

	auto *dlg = new QDialog(parent);
	gDialog = dlg;
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->setWindowTitle(QObject::tr("Compatibility Database"));
	dlg->resize(720, 480);

	auto *root = new QVBoxLayout(dlg);

	auto *header = new QHBoxLayout();
	auto *pathLbl = new QLabel(dlg);
	const QString p = ATCompatGetDatabasePath();
	pathLbl->setText(p.isEmpty()
		? QObject::tr("(no database loaded)")
		: QObject::tr("Loaded: %1").arg(p));
	pathLbl->setWordWrap(true);
	header->addWidget(pathLbl, 1);
	root->addLayout(header);

	auto *filterEdit = new QLineEdit(dlg);
	filterEdit->setPlaceholderText(QObject::tr("Filter titles or tags..."));
	root->addWidget(filterEdit);

	auto *model = new CompatModel(dlg);
	model->rebuild();

	auto *proxy = new QSortFilterProxyModel(dlg);
	proxy->setSourceModel(model);
	proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
	proxy->setFilterKeyColumn(-1);

	auto *table = new QTableView(dlg);
	table->setModel(proxy);
	table->setSortingEnabled(true);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->horizontalHeader()->setStretchLastSection(true);
	table->verticalHeader()->setVisible(false);
	root->addWidget(table, 1);

	QObject::connect(filterEdit, &QLineEdit::textChanged,
		[proxy](const QString& t){ proxy->setFilterFixedString(t); });

	auto *btnRow = new QHBoxLayout();
	auto *clearOptOuts = new QPushButton(QObject::tr("Clear all per-image opt-outs"));
	auto *closeBtn = new QPushButton(QObject::tr("Close"));
	btnRow->addStretch();
	btnRow->addWidget(clearOptOuts);
	btnRow->addWidget(closeBtn);
	root->addLayout(btnRow);

	QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);
	QObject::connect(clearOptOuts, &QPushButton::clicked, dlg, [dlg]{
		QSettings s;
		s.beginGroup(QStringLiteral("compat"));
		const QStringList children = s.childGroups();
		for (const QString& c : children) {
			if (c.size() == 64) // sha256 hex length
				s.remove(c);
		}
		s.endGroup();
		QMessageBox::information(dlg, QObject::tr("Compatibility Database"),
			QObject::tr("All per-image compat opt-outs cleared."));
	});

	dlg->show();
}
