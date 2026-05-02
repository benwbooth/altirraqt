//	Altirra - Qt port
//	Disk Drives dialog implementation.

#include "diskdrivesdialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMainWindow>
#include <QMimeData>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/error.h>
#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <at/atcore/media.h>
#include <at/atio/diskimage.h>
#include <diskinterface.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

constexpr int kMaxDisplayedDrives = 8;

struct ModeOpt { ATMediaWriteMode mode; const char *label; };
constexpr ModeOpt kModeOpts[] = {
	{kATMediaWriteMode_RO,      "Read-only"},
	{kATMediaWriteMode_VRWSafe, "Virtual r/w (safe)"},
	{kATMediaWriteMode_VRW,     "Virtual r/w (loses on reset)"},
	{kATMediaWriteMode_RW,      "Read/write (auto-flush)"},
};

int modeToComboIndex(ATMediaWriteMode m) {
	// Match by exact value to keep the four canonical modes round-tripping.
	for (size_t i = 0; i < std::size(kModeOpts); ++i)
		if (kModeOpts[i].mode == m) return (int)i;
	// Fallback: classify by feature bits if the simulator handed us a
	// combination not in our preset list.
	if ((m & kATMediaWriteMode_AllowWrite) == 0) return 0;
	if (m & kATMediaWriteMode_AutoFlush)         return 3;
	if (m & kATMediaWriteMode_AllowFormat)       return 2;
	return 1;
}

class ATDiskDrivesDialog;

class ATDiskDropTable : public QTableWidget {
public:
	ATDiskDropTable(int rows, int cols, ATDiskDrivesDialog *parent);

protected:
	void dragEnterEvent(QDragEnterEvent *e) override;
	void dragMoveEvent(QDragMoveEvent *e) override;
	void dropEvent(QDropEvent *e) override;

private:
	ATDiskDrivesDialog *mpDialog;
};

class ATDiskDrivesDialog : public QDialog {
public:
	ATDiskDrivesDialog(QMainWindow *parent, ATSimulator *sim)
		: QDialog(parent), mpMainWindow(parent), mpSim(sim)
	{
		setWindowTitle(tr("Disk Drives"));
		resize(720, 320);

		auto *layout = new QVBoxLayout(this);

		mpTable = new ATDiskDropTable(kMaxDisplayedDrives, 3, this);
		mpTable->setAcceptDrops(true);
		mpTable->setDragDropMode(QAbstractItemView::DropOnly);
		mpTable->setDropIndicatorShown(true);
		mpTable->setHorizontalHeaderLabels({tr("Drive"), tr("Mounted Image"), tr("Mode")});
		mpTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		mpTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		mpTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
		mpTable->verticalHeader()->setVisible(false);
		mpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		mpTable->setSelectionMode(QAbstractItemView::SingleSelection);
		mpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		layout->addWidget(mpTable, 1);

		auto *btnRow = new QHBoxLayout;
		auto *mountBtn  = new QPushButton(tr("&Mount..."), this);
		auto *ejectBtn  = new QPushButton(tr("&Eject"), this);
		auto *ejectAllBtn = new QPushButton(tr("Eject &All"), this);
		auto *newBtn = new QPushButton(tr("&New Disk..."), this);
		btnRow->addWidget(mountBtn);
		btnRow->addWidget(ejectBtn);
		btnRow->addWidget(ejectAllBtn);
		btnRow->addWidget(newBtn);
		btnRow->addStretch();
		layout->addLayout(btnRow);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Close, this);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::accept);
		layout->addWidget(box);

		connect(mountBtn, &QPushButton::clicked, this, &ATDiskDrivesDialog::onMount);
		connect(ejectBtn, &QPushButton::clicked, this, &ATDiskDrivesDialog::onEject);
		connect(ejectAllBtn, &QPushButton::clicked, this, &ATDiskDrivesDialog::onEjectAll);
		connect(newBtn, &QPushButton::clicked, this, &ATDiskDrivesDialog::onNewDisk);
		connect(mpTable, &QTableWidget::doubleClicked, this, &ATDiskDrivesDialog::onMount);

		refresh();
	}

	// Called by ATDiskDropTable when a file URL is dropped onto a row.
	void mountDroppedFile(int row, const QString& path) {
		if (row < 0 || row >= kMaxDisplayedDrives) row = currentDrive();
		const std::wstring w = path.toStdWString();
		try {
			mpSim->GetDiskInterface(row).LoadDisk(w.c_str());
		} catch (const MyError& e) {
			QMessageBox::warning(this, tr("Mount failed"),
			                     QString::fromUtf8(e.c_str()));
			refresh();
			return;
		}
		status(tr("Mounted in D%1: %2").arg(row + 1).arg(QFileInfo(path).fileName()));
		refresh();
	}

private:
	void refresh() {
		for (int i = 0; i < kMaxDisplayedDrives; ++i) {
			auto& d = mpSim->GetDiskInterface(i);
			auto *driveItem  = new QTableWidgetItem(QStringLiteral("D%1:").arg(i + 1));
			auto *imageItem  = new QTableWidgetItem(
				d.IsDiskLoaded()
					? QString::fromWCharArray(d.GetPath())
					: QStringLiteral("(empty)"));
			mpTable->setItem(i, 0, driveItem);
			mpTable->setItem(i, 1, imageItem);

			// Per-row write-mode combo. Created on first refresh and reused
			// thereafter; only enabled when a disk is actually mounted.
			auto *combo = qobject_cast<QComboBox *>(mpTable->cellWidget(i, 2));
			if (!combo) {
				combo = new QComboBox(mpTable);
				for (const auto& m : kModeOpts)
					combo->addItem(tr(m.label));
				connect(combo, qOverload<int>(&QComboBox::activated),
					[this, i](int idx){
					if (idx >= 0 && idx < (int)std::size(kModeOpts))
						mpSim->GetDiskInterface(i).SetWriteMode(kModeOpts[idx].mode);
				});
				mpTable->setCellWidget(i, 2, combo);
			}
			QSignalBlocker block(combo);
			combo->setCurrentIndex(modeToComboIndex(d.GetWriteMode()));
			combo->setEnabled(d.IsDiskLoaded());
		}
	}

	int currentDrive() const {
		const int row = mpTable->currentRow();
		return row >= 0 ? row : 0;
	}

	void status(const QString& msg) {
		if (mpMainWindow)
			mpMainWindow->statusBar()->showMessage(msg, 2500);
	}

	void onMount() {
		const int drive = currentDrive();
		const QString path = QFileDialog::getOpenFileName(
			this,
			tr("Mount disk in drive %1").arg(drive + 1),
			{},
			tr("Atari disk images (*.atr *.atx *.xfd);;All files (*)"));
		if (path.isEmpty()) return;
		const std::wstring w = path.toStdWString();
		try {
			mpSim->GetDiskInterface(drive).LoadDisk(w.c_str());
		} catch (const MyError& e) {
			QMessageBox::warning(this, tr("Mount failed"),
			                     QString::fromUtf8(e.c_str()));
			refresh();
			return;
		}
		status(tr("Mounted in D%1: %2").arg(drive + 1).arg(QFileInfo(path).fileName()));
		refresh();
	}

	void onEject() {
		const int drive = currentDrive();
		mpSim->GetDiskInterface(drive).UnloadDisk();
		status(tr("Detached D%1:").arg(drive + 1));
		refresh();
	}

	void onEjectAll() {
		for (int i = 0; i < kMaxDisplayedDrives; ++i)
			mpSim->GetDiskInterface(i).UnloadDisk();
		status(tr("All disks detached"));
		refresh();
	}

	void onNewDisk() {
		QDialog dlg(this);
		dlg.setWindowTitle(tr("New Blank Disk"));
		auto *form = new QFormLayout(&dlg);

		auto *densityCombo = new QComboBox(&dlg);
		struct Density { const char *label; uint32 sectors; uint32 secSize; };
		static constexpr Density kDensities[] = {
			{"Single density (90KB, 720x128)",   720,  128},
			{"Enhanced density (130KB, 1040x128)", 1040, 128},
			{"Double density (180KB, 720x256)",  720,  256},
			{"DSDD (360KB, 1440x256)",           1440, 256},
		};
		for (const auto& d : kDensities)
			densityCombo->addItem(tr(d.label));

		auto *driveSpin = new QSpinBox(&dlg);
		driveSpin->setRange(1, kMaxDisplayedDrives);
		driveSpin->setValue(currentDrive() + 1);

		auto *pathRow = new QHBoxLayout;
		auto *pathEdit = new QLineEdit(&dlg);
		pathEdit->setPlaceholderText(tr("(leave blank for in-memory)"));
		auto *browseBtn = new QPushButton(tr("Browse..."), &dlg);
		pathRow->addWidget(pathEdit, 1);
		pathRow->addWidget(browseBtn);
		connect(browseBtn, &QPushButton::clicked, &dlg, [this, &dlg, pathEdit]{
			const QString p = QFileDialog::getSaveFileName(&dlg,
				tr("Save new disk as"),
				QStringLiteral("new-disk.atr"),
				tr("Atari disk images (*.atr);;All files (*)"));
			if (!p.isEmpty()) pathEdit->setText(p);
		});

		form->addRow(tr("Density:"), densityCombo);
		form->addRow(tr("Drive:"), driveSpin);
		form->addRow(tr("Save to file:"), pathRow);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
		connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
		connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
		form->addRow(box);

		if (dlg.exec() != QDialog::Accepted) return;

		const int densityIdx = densityCombo->currentIndex();
		if (densityIdx < 0 || densityIdx >= (int)std::size(kDensities)) return;
		const Density& d = kDensities[densityIdx];
		const int driveIdx = driveSpin->value() - 1;
		const QString savePath = pathEdit->text().trimmed();

		auto& iface = mpSim->GetDiskInterface(driveIdx);
		try {
			iface.CreateDisk(d.sectors, 3, d.secSize);
			if (!savePath.isEmpty()) {
				iface.SaveDiskAs(savePath.toStdWString().c_str(),
				                 kATDiskImageFormat_ATR);
			}
		} catch (const MyError& e) {
			QMessageBox::warning(this, tr("New disk failed"),
			                     QString::fromUtf8(e.c_str()));
			refresh();
			return;
		}
		status(tr("Created blank disk in D%1:").arg(driveIdx + 1));
		refresh();
	}

	QMainWindow      *mpMainWindow;
	ATSimulator      *mpSim;
	ATDiskDropTable  *mpTable;
};

ATDiskDropTable::ATDiskDropTable(int rows, int cols, ATDiskDrivesDialog *parent)
	: QTableWidget(rows, cols, parent), mpDialog(parent) {}

void ATDiskDropTable::dragEnterEvent(QDragEnterEvent *e) {
	if (e->mimeData()->hasUrls()) e->acceptProposedAction();
	else QTableWidget::dragEnterEvent(e);
}

void ATDiskDropTable::dragMoveEvent(QDragMoveEvent *e) {
	if (e->mimeData()->hasUrls()) e->acceptProposedAction();
	else QTableWidget::dragMoveEvent(e);
}

void ATDiskDropTable::dropEvent(QDropEvent *e) {
	const QList<QUrl> urls = e->mimeData()->urls();
	if (urls.isEmpty()) {
		QTableWidget::dropEvent(e);
		return;
	}
	const QString path = urls.first().toLocalFile();
	if (path.isEmpty()) {
		QTableWidget::dropEvent(e);
		return;
	}
	const int row = indexAt(e->position().toPoint()).row();
	mpDialog->mountDroppedFile(row, path);
	e->acceptProposedAction();
}

} // namespace

void ATShowDiskDrivesDialog(QMainWindow *parent, ATSimulator *sim) {
	ATDiskDrivesDialog dlg(parent, sim);
	dlg.exec();
}
