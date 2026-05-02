//	Altirra - Qt port
//	Disk Explorer dialog implementation.

#include "diskexplorerdialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/error.h>
#include <vd2/system/refcount.h>
#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <vd2/system/vdstl.h>
#include <at/atio/diskfs.h>
#include <at/atio/diskimage.h>

namespace {

class ATDiskExplorerDialog : public QDialog {
public:
	ATDiskExplorerDialog(QWidget *parent)
		: QDialog(parent)
	{
		setWindowTitle(tr("Disk Explorer"));
		resize(640, 420);

		auto *layout = new QVBoxLayout(this);

		auto *topRow = new QHBoxLayout;
		auto *openBtn = new QPushButton(tr("&Open Image..."));
		auto *upBtn   = new QPushButton(tr("Up"));
		auto *extractBtn = new QPushButton(tr("&Extract..."));
		topRow->addWidget(openBtn);
		topRow->addWidget(upBtn);
		topRow->addStretch();
		topRow->addWidget(extractBtn);
		layout->addLayout(topRow);

		mpInfoLabel = new QLabel(tr("(no image)"));
		layout->addWidget(mpInfoLabel);

		mpTable = new QTableWidget(0, 3);
		mpTable->setHorizontalHeaderLabels({tr("Name"), tr("Size"), tr("Sectors")});
		mpTable->horizontalHeader()->setStretchLastSection(true);
		mpTable->verticalHeader()->setVisible(false);
		mpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		mpTable->setSelectionMode(QAbstractItemView::SingleSelection);
		mpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		layout->addWidget(mpTable, 1);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Close);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::accept);
		layout->addWidget(box);

		connect(openBtn,   &QPushButton::clicked, this, &ATDiskExplorerDialog::onOpen);
		connect(upBtn,     &QPushButton::clicked, this, &ATDiskExplorerDialog::onUp);
		connect(extractBtn,&QPushButton::clicked, this, &ATDiskExplorerDialog::onExtract);
		connect(mpTable,   &QTableWidget::doubleClicked, this, &ATDiskExplorerDialog::onActivate);
	}

private:
	void onOpen() {
		const QString path = QFileDialog::getOpenFileName(this,
			tr("Open disk image"), {},
			tr("Atari disk images (*.atr *.atx *.xfd);;All files (*)"));
		if (path.isEmpty()) return;
		try {
			vdrefptr<IATDiskImage> img;
			ATLoadDiskImage(path.toStdWString().c_str(), ~img);
			vdautoptr<IATDiskFS> fs(ATDiskMountImage(img, /*readOnly=*/true));
			if (!fs) {
				QMessageBox::warning(this, tr("Disk Explorer"),
					tr("Could not detect a supported filesystem on this image."));
				return;
			}
			mpImage = img;
			mpFS.reset(fs.release());
			mCwd = ATDiskFSKey::None;
			mDirStack.clear();
			refresh();
		} catch (const MyError& e) {
			QMessageBox::warning(this, tr("Disk Explorer"),
				QString::fromUtf8(e.c_str()));
		}
	}

	void onUp() {
		if (!mpFS || mDirStack.empty()) return;
		mCwd = mDirStack.back();
		mDirStack.pop_back();
		refresh();
	}

	void onActivate(const QModelIndex& idx) {
		if (!mpFS) return;
		const int row = idx.row();
		if (row < 0 || row >= (int)mEntries.size()) return;
		const auto& e = mEntries[row];
		if (e.mbIsDirectory) {
			mDirStack.push_back(mCwd);
			mCwd = e.mKey;
			refresh();
		} else {
			extractEntry(e);
		}
	}

	void onExtract() {
		const int row = mpTable->currentRow();
		if (row < 0 || row >= (int)mEntries.size()) return;
		const auto& e = mEntries[row];
		if (e.mbIsDirectory) return;
		extractEntry(e);
	}

	void extractEntry(const ATDiskFSEntryInfo& e) {
		const QString suggested = QString::fromUtf8(e.mFileName.c_str());
		const QString outPath = QFileDialog::getSaveFileName(this,
			tr("Extract %1").arg(suggested), suggested);
		if (outPath.isEmpty()) return;
		try {
			vdfastvector<uint8> data;
			mpFS->ReadFile(e.mKey, data);
			QFile out(outPath);
			if (!out.open(QIODevice::WriteOnly)) {
				QMessageBox::warning(this, tr("Extract failed"),
					tr("Could not open file for writing."));
				return;
			}
			out.write((const char *)data.data(), (qint64)data.size());
		} catch (const MyError& err) {
			QMessageBox::warning(this, tr("Extract failed"),
				QString::fromUtf8(err.c_str()));
		}
	}

	void refresh() {
		mEntries.clear();
		mpTable->setRowCount(0);
		if (!mpFS) {
			mpInfoLabel->setText(tr("(no image)"));
			return;
		}

		ATDiskFSInfo info;
		mpFS->GetInfo(info);
		mpInfoLabel->setText(tr("%1 — %2 free blocks of %3 bytes")
			.arg(QString::fromUtf8(info.mFSType.c_str()))
			.arg(info.mFreeBlocks)
			.arg(info.mBlockSize));

		ATDiskFSEntryInfo e;
		auto h = mpFS->FindFirst(mCwd, e);
		if (h == ATDiskFSFindHandle::Invalid) return;
		do {
			mEntries.push_back(e);
		} while (mpFS->FindNext(h, e));
		mpFS->FindEnd(h);

		mpTable->setRowCount((int)mEntries.size());
		for (size_t i = 0; i < mEntries.size(); ++i) {
			const auto& en = mEntries[i];
			QString name = QString::fromUtf8(en.mFileName.c_str());
			if (en.mbIsDirectory) name += QStringLiteral("/");
			mpTable->setItem((int)i, 0, new QTableWidgetItem(name));
			mpTable->setItem((int)i, 1,
				new QTableWidgetItem(en.mbIsDirectory ? QString()
				                                      : QString::number(en.mBytes)));
			mpTable->setItem((int)i, 2, new QTableWidgetItem(QString::number(en.mSectors)));
		}
	}

	vdrefptr<IATDiskImage>      mpImage;
	std::unique_ptr<IATDiskFS>  mpFS;
	ATDiskFSKey                 mCwd = ATDiskFSKey::None;
	std::vector<ATDiskFSKey>    mDirStack;
	std::vector<ATDiskFSEntryInfo> mEntries;

	QTableWidget *mpTable;
	QLabel       *mpInfoLabel;
};

} // namespace

void ATShowDiskExplorerDialog(QWidget *parent) {
	ATDiskExplorerDialog dlg(parent);
	dlg.exec();
}
