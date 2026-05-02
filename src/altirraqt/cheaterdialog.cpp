//	Altirra - Qt port
//	Cheater dialog implementation.

#include "cheaterdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/error.h>
#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <cheatengine.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

QString fmtAddr(uint32 a) {
	return QStringLiteral("$%1").arg(a, 4, 16, QLatin1Char('0')).toUpper();
}

QString fmtValue(uint32 v, bool bit16) {
	const int width = bit16 ? 4 : 2;
	return QStringLiteral("$%1 (%2)")
		.arg(v, width, 16, QLatin1Char('0')).toUpper()
		.arg(v);
}

// Edit/add-cheat sub-dialog: hex address + value + 8/16-bit toggle.
class ATCheatEditDialog : public QDialog {
public:
	ATCheatEditDialog(QWidget *parent, ATCheatEngine::Cheat& cheat)
		: QDialog(parent), mCheat(cheat)
	{
		setWindowTitle(tr("Edit Cheat"));

		auto *form = new QFormLayout;
		mpAddr = new QLineEdit(QStringLiteral("$%1").arg(cheat.mAddress, 4, 16, QLatin1Char('0')));
		mpValue = new QLineEdit(QStringLiteral("$%1").arg(cheat.mValue,
		                                                  cheat.mb16Bit ? 4 : 2, 16, QLatin1Char('0')));
		mpBit16 = new QCheckBox(tr("16-bit"));
		mpBit16->setChecked(cheat.mb16Bit);
		form->addRow(tr("Address (hex):"), mpAddr);
		form->addRow(tr("Value (hex or # for decimal):"), mpValue);
		form->addRow(QString(), mpBit16);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(box, &QDialogButtonBox::accepted, this, &ATCheatEditDialog::onAccept);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

		auto *layout = new QVBoxLayout(this);
		layout->addLayout(form);
		layout->addWidget(box);
	}

private:
	// Accept "$1234", "1234" (hex), "#4660" (decimal). Returns false on parse failure.
	static bool parseValue(const QString& text, uint32& out) {
		QString t = text.trimmed();
		if (t.isEmpty()) return false;
		bool ok;
		uint32 v;
		if (t.startsWith('$')) {
			v = t.mid(1).toUInt(&ok, 16);
		} else if (t.startsWith('#')) {
			v = t.mid(1).toUInt(&ok, 10);
		} else {
			v = t.toUInt(&ok, 16);
		}
		if (!ok) return false;
		out = v;
		return true;
	}

	void onAccept() {
		uint32 addr, value;
		if (!parseValue(mpAddr->text(), addr)) {
			QMessageBox::warning(this, tr("Invalid value"), tr("Could not parse address."));
			return;
		}
		if (!parseValue(mpValue->text(), value)) {
			QMessageBox::warning(this, tr("Invalid value"), tr("Could not parse value."));
			return;
		}
		mCheat.mAddress = addr;
		mCheat.mValue   = value;
		mCheat.mb16Bit  = mpBit16->isChecked();
		accept();
	}

	ATCheatEngine::Cheat& mCheat;
	QLineEdit *mpAddr;
	QLineEdit *mpValue;
	QCheckBox *mpBit16;
};

class ATCheaterDialog : public QDialog {
public:
	ATCheaterDialog(QWidget *parent, ATSimulator *sim)
		: QDialog(parent), mpSim(sim)
	{
		setWindowTitle(tr("Cheater"));
		resize(720, 480);

		// Lazily create the engine on first open, then keep it alive so
		// existing cheats persist between dialog openings.
		mpSim->SetCheatEngineEnabled(true);
		mpEngine = mpSim->GetCheatEngine();

		auto *outer = new QHBoxLayout(this);

		// --- Search panel ------------------------------------------------
		auto *searchBox = new QGroupBox(tr("Search"));
		auto *searchLayout = new QVBoxLayout(searchBox);

		auto *modeRow = new QHBoxLayout;
		mpModeCombo = new QComboBox;
		mpModeCombo->addItems({
			tr("Reset (start over)"),
			tr("Equal"),
			tr("Not equal"),
			tr("Less than"),
			tr("Less or equal"),
			tr("Greater than"),
			tr("Greater or equal"),
			tr("Equal to value..."),
		});
		mpModeCombo->setCurrentIndex(0);
		mpValueEdit = new QSpinBox;
		mpValueEdit->setRange(-32768, 65535);
		mpValueEdit->setValue(0);
		mpBit16 = new QCheckBox(tr("16-bit"));
		modeRow->addWidget(mpModeCombo, 1);
		modeRow->addWidget(mpValueEdit);
		modeRow->addWidget(mpBit16);
		searchLayout->addLayout(modeRow);

		auto *updateBtn = new QPushButton(tr("&Update"));
		searchLayout->addWidget(updateBtn);

		mpResults = new QTableWidget(0, 2);
		mpResults->setHorizontalHeaderLabels({tr("Address"), tr("Value")});
		mpResults->horizontalHeader()->setStretchLastSection(true);
		mpResults->verticalHeader()->setVisible(false);
		mpResults->setSelectionBehavior(QAbstractItemView::SelectRows);
		mpResults->setSelectionMode(QAbstractItemView::SingleSelection);
		mpResults->setEditTriggers(QAbstractItemView::NoEditTriggers);
		searchLayout->addWidget(mpResults, 1);

		auto *xferRow = new QHBoxLayout;
		auto *xferBtn    = new QPushButton(tr("Add to Cheats"));
		auto *xferAllBtn = new QPushButton(tr("Add All"));
		xferRow->addWidget(xferBtn);
		xferRow->addWidget(xferAllBtn);
		xferRow->addStretch();
		searchLayout->addLayout(xferRow);

		outer->addWidget(searchBox, 1);

		// --- Active cheats panel ----------------------------------------
		auto *activeBox = new QGroupBox(tr("Active Cheats"));
		auto *activeLayout = new QVBoxLayout(activeBox);

		mpActive = new QTableWidget(0, 3);
		mpActive->setHorizontalHeaderLabels({tr("On"), tr("Address"), tr("Value")});
		mpActive->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		mpActive->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
		mpActive->horizontalHeader()->setStretchLastSection(true);
		mpActive->verticalHeader()->setVisible(false);
		mpActive->setSelectionBehavior(QAbstractItemView::SelectRows);
		mpActive->setSelectionMode(QAbstractItemView::SingleSelection);
		mpActive->setEditTriggers(QAbstractItemView::NoEditTriggers);
		activeLayout->addWidget(mpActive, 1);

		auto *btnRow = new QHBoxLayout;
		auto *addBtn    = new QPushButton(tr("&Add..."));
		auto *editBtn   = new QPushButton(tr("&Edit..."));
		auto *delBtn    = new QPushButton(tr("&Delete"));
		auto *loadBtn   = new QPushButton(tr("&Load..."));
		auto *saveBtn   = new QPushButton(tr("&Save..."));
		btnRow->addWidget(addBtn);
		btnRow->addWidget(editBtn);
		btnRow->addWidget(delBtn);
		btnRow->addStretch();
		btnRow->addWidget(loadBtn);
		btnRow->addWidget(saveBtn);
		activeLayout->addLayout(btnRow);

		auto *closeBox = new QDialogButtonBox(QDialogButtonBox::Close);
		connect(closeBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
		activeLayout->addWidget(closeBox);

		outer->addWidget(activeBox, 1);

		// --- Wiring ------------------------------------------------------
		connect(mpModeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
			this, &ATCheaterDialog::updateValueEnable);
		connect(updateBtn,   &QPushButton::clicked, this, &ATCheaterDialog::onUpdate);
		connect(xferBtn,     &QPushButton::clicked, this, &ATCheaterDialog::onTransfer);
		connect(xferAllBtn,  &QPushButton::clicked, this, &ATCheaterDialog::onTransferAll);
		connect(mpResults,   &QTableWidget::doubleClicked, this, &ATCheaterDialog::onTransfer);
		connect(addBtn,      &QPushButton::clicked, this, &ATCheaterDialog::onAdd);
		connect(editBtn,     &QPushButton::clicked, this, &ATCheaterDialog::onEdit);
		connect(delBtn,      &QPushButton::clicked, this, &ATCheaterDialog::onDelete);
		connect(loadBtn,     &QPushButton::clicked, this, &ATCheaterDialog::onLoad);
		connect(saveBtn,     &QPushButton::clicked, this, &ATCheaterDialog::onSave);
		connect(mpActive,    &QTableWidget::doubleClicked, this, &ATCheaterDialog::onEdit);
		connect(mpActive,    &QTableWidget::itemChanged,
			this, &ATCheaterDialog::onActiveItemChanged);

		updateValueEnable();
		refreshActive();
	}

private:
	void updateValueEnable() {
		mpValueEdit->setEnabled(mpModeCombo->currentIndex() == 7);
	}

	void onUpdate() {
		const bool bit16 = mpBit16->isChecked();
		const int mode = mpModeCombo->currentIndex();
		ATCheatSnapshotMode m;
		uint32 v = 0;
		switch (mode) {
			case 0: m = kATCheatSnapMode_Replace;      break;
			case 1: m = kATCheatSnapMode_Equal;        break;
			case 2: m = kATCheatSnapMode_NotEqual;     break;
			case 3: m = kATCheatSnapMode_Less;         break;
			case 4: m = kATCheatSnapMode_LessEqual;    break;
			case 5: m = kATCheatSnapMode_Greater;      break;
			case 6: m = kATCheatSnapMode_GreaterEqual; break;
			case 7: {
				const int sv = mpValueEdit->value();
				const int lo = bit16 ? -32768 : -128;
				const int hi = bit16 ?  65535 :  255;
				if (sv < lo || sv > hi) {
					QMessageBox::warning(this, tr("Cheater"),
						tr("Search value out of range for %1-bit search.")
							.arg(bit16 ? 16 : 8));
					return;
				}
				v = (uint32)sv;
				m = kATCheatSnapMode_EqualRef;
				break;
			}
			default: return;
		}
		mpEngine->Snapshot(m, v, bit16);
		refreshResults();
	}

	void refreshResults() {
		mpResults->setRowCount(0);
		constexpr uint32 kMax = 250;
		uint32 ids[kMax];
		const uint32 n = mpEngine->GetValidOffsets(ids, kMax);
		const bool bit16 = mpBit16->isChecked();

		if (!n) {
			mpResults->setRowCount(1);
			mpResults->setItem(0, 0, new QTableWidgetItem(tr("No results — try again.")));
			return;
		}
		if (n >= kMax) {
			mpResults->setRowCount(1);
			mpResults->setItem(0, 0,
				new QTableWidgetItem(tr("Too many results (%1).").arg(n)));
			return;
		}

		mpResults->setRowCount((int)n);
		for (uint32 i = 0; i < n; ++i) {
			const uint32 cur = mpEngine->GetOffsetCurrentValue(ids[i], bit16);
			auto *addrItem = new QTableWidgetItem(fmtAddr(ids[i]));
			addrItem->setData(Qt::UserRole, ids[i]);
			addrItem->setData(Qt::UserRole + 1, cur);
			addrItem->setData(Qt::UserRole + 2, bit16);
			mpResults->setItem((int)i, 0, addrItem);
			mpResults->setItem((int)i, 1, new QTableWidgetItem(fmtValue(cur, bit16)));
		}
	}

	void onTransfer() {
		const int row = mpResults->currentRow();
		if (row < 0) return;
		auto *item = mpResults->item(row, 0);
		if (!item) return;
		ATCheatEngine::Cheat c;
		c.mAddress  = item->data(Qt::UserRole).toUInt();
		c.mValue    = (uint16)item->data(Qt::UserRole + 1).toUInt();
		c.mb16Bit   = item->data(Qt::UserRole + 2).toBool();
		c.mbEnabled = false;   // start disabled — user enables when ready
		mpEngine->AddCheat(c);
		refreshActive();
	}

	void onTransferAll() {
		const int n = mpResults->rowCount();
		for (int i = 0; i < n; ++i) {
			auto *item = mpResults->item(i, 0);
			if (!item) continue;
			ATCheatEngine::Cheat c;
			c.mAddress  = item->data(Qt::UserRole).toUInt();
			c.mValue    = (uint16)item->data(Qt::UserRole + 1).toUInt();
			c.mb16Bit   = item->data(Qt::UserRole + 2).toBool();
			c.mbEnabled = true;
			mpEngine->AddCheat(c);
		}
		refreshActive();
	}

	void refreshActive() {
		mpReentry = true;
		mpActive->setRowCount(0);
		const uint32 n = mpEngine->GetCheatCount();
		mpActive->setRowCount((int)n);
		for (uint32 i = 0; i < n; ++i) {
			const auto& c = mpEngine->GetCheatByIndex(i);
			auto *enable = new QTableWidgetItem;
			enable->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			enable->setCheckState(c.mbEnabled ? Qt::Checked : Qt::Unchecked);
			enable->setData(Qt::UserRole, i);
			mpActive->setItem((int)i, 0, enable);
			mpActive->setItem((int)i, 1, new QTableWidgetItem(fmtAddr(c.mAddress)));
			mpActive->setItem((int)i, 2, new QTableWidgetItem(fmtValue(c.mValue, c.mb16Bit)));
		}
		mpReentry = false;
	}

	void onActiveItemChanged(QTableWidgetItem *item) {
		if (mpReentry || !item || item->column() != 0) return;
		const uint32 idx = item->data(Qt::UserRole).toUInt();
		if (idx >= mpEngine->GetCheatCount()) return;
		ATCheatEngine::Cheat c = mpEngine->GetCheatByIndex(idx);
		c.mbEnabled = (item->checkState() == Qt::Checked);
		mpEngine->UpdateCheat(idx, c);
	}

	void onAdd() {
		ATCheatEngine::Cheat c{};
		ATCheatEditDialog dlg(this, c);
		if (dlg.exec() == QDialog::Accepted) {
			c.mbEnabled = true;
			mpEngine->AddCheat(c);
			refreshActive();
		}
	}

	void onEdit() {
		const int row = mpActive->currentRow();
		if (row < 0) return;
		auto *enable = mpActive->item(row, 0);
		if (!enable) return;
		const uint32 idx = enable->data(Qt::UserRole).toUInt();
		if (idx >= mpEngine->GetCheatCount()) return;
		ATCheatEngine::Cheat c = mpEngine->GetCheatByIndex(idx);
		ATCheatEditDialog dlg(this, c);
		if (dlg.exec() == QDialog::Accepted) {
			mpEngine->UpdateCheat(idx, c);
			refreshActive();
		}
	}

	void onDelete() {
		const int row = mpActive->currentRow();
		if (row < 0) return;
		auto *enable = mpActive->item(row, 0);
		if (!enable) return;
		const uint32 idx = enable->data(Qt::UserRole).toUInt();
		mpEngine->RemoveCheatByIndex(idx);
		refreshActive();
	}

	void onLoad() {
		const QString path = QFileDialog::getOpenFileName(this,
			tr("Load cheats"), {}, tr("Altirra cheats (*.atcheats);;All files (*)"));
		if (path.isEmpty()) return;
		try {
			mpEngine->Load(path.toStdWString().c_str());
		} catch (const MyError& e) {
			QMessageBox::warning(this, tr("Load failed"), QString::fromUtf8(e.c_str()));
		}
		refreshActive();
	}

	void onSave() {
		const QString path = QFileDialog::getSaveFileName(this,
			tr("Save cheats"), QStringLiteral("cheats.atcheats"),
			tr("Altirra cheats (*.atcheats)"));
		if (path.isEmpty()) return;
		try {
			mpEngine->Save(path.toStdWString().c_str());
		} catch (const MyError& e) {
			QMessageBox::warning(this, tr("Save failed"), QString::fromUtf8(e.c_str()));
		}
	}

	ATSimulator   *mpSim;
	ATCheatEngine *mpEngine;

	QComboBox    *mpModeCombo;
	QSpinBox     *mpValueEdit;
	QCheckBox    *mpBit16;
	QTableWidget *mpResults;
	QTableWidget *mpActive;

	bool          mpReentry = false;
};

} // namespace

void ATShowCheaterDialog(QWidget *parent, ATSimulator *sim) {
	ATCheaterDialog dlg(parent, sim);
	dlg.exec();
}
