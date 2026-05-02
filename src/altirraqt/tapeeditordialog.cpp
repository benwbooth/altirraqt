//	Altirra - Qt port
//	Tape Editor dialog implementation.
//
//	The IATCassetteImage backend models the tape as a sample-based piece
//	table; "blocks" are returned via GetRegionInfo(pos) per-position. We
//	walk the tape from sample 0 to GetDataLength() to enumerate regions,
//	display them, and expose the subset of mutators the API supports:
//
//		- Append blank region          (WriteBlankData with insert=true)
//		- Delete the selected region   (DeleteRange[start, start+len))
//		- Save back as CAS / WAV       (ATSaveCassetteImage{CAS,WAV})
//
//	Block-level editing — insertion of typed bytes (WriteStdData),
//	insertion of a continuous mark tone (WritePulse), and move-up /
//	move-down for an existing region — is built on top of the
//	piece-table mutators IATCassetteImage exposes:
//	   - WriteStdData(cursor, byte, baud, insert)
//	   - WritePulse(cursor, polarity, samples, insert, fsk)
//	   - CopyRange / InsertRange / DeleteRange (clip-based moves)
//	No upstream method moves a region in one call; the editor builds the
//	move out of CopyRange + DeleteRange + InsertRange.

#include "tapeeditordialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/error.h>
#include <vd2/system/file.h>
#include <vd2/system/VDString.h>
#include <vd2/system/refcount.h>
#include <vd2/system/vdalloc.h>
#include <at/atio/cassetteimage.h>
#include <cassette.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

const char *regionTypeName(ATCassetteRegionType t) {
	switch (t) {
		case ATCassetteRegionType::Mark:        return "Mark";
		case ATCassetteRegionType::Raw:         return "Raw / FSK";
		case ATCassetteRegionType::DecodedData: return "Decoded data";
	}
	return "?";
}

struct Region {
	ATCassetteRegionType type;
	uint32               startSample;
	uint32               lengthSamples;
};

// Walk the tape sample-by-region, collecting every region from 0 to
// GetDataLength(). GetRegionInfo returns the region containing the
// queried sample; we step past it via mRegionStart + mRegionLen. We cap
// the loop at the data length to be defensive against an image
// returning a zero-length region.
std::vector<Region> enumerateRegions(IATCassetteImage *image) {
	std::vector<Region> out;
	if (!image) return out;
	const uint32 total = image->GetDataLength();
	uint32 pos = 0;
	while (pos < total) {
		const ATCassetteRegionInfo info = image->GetRegionInfo(pos);
		Region r{ info.mRegionType, info.mRegionStart, info.mRegionLen };
		// Defensive: if the backend ever reports a zero-length region,
		// bail to avoid an infinite loop.
		if (r.lengthSamples == 0) break;
		out.push_back(r);
		pos = info.mRegionStart + info.mRegionLen;
	}
	return out;
}

class ATTapeEditorDialog : public QDialog {
public:
	ATTapeEditorDialog(QWidget *parent, ATSimulator *sim)
		: QDialog(parent), mpSim(sim)
	{
		setWindowTitle(tr("Tape Editor"));
		resize(600, 420);

		auto *layout = new QVBoxLayout(this);

		mpHeader = new QLabel(this);
		layout->addWidget(mpHeader);

		mpTable = new QTableWidget(0, 4, this);
		mpTable->setHorizontalHeaderLabels({tr("#"), tr("Type"), tr("Start (s)"), tr("Length (s)")});
		mpTable->horizontalHeader()->setStretchLastSection(true);
		mpTable->verticalHeader()->setVisible(false);
		mpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		mpTable->setSelectionMode(QAbstractItemView::SingleSelection);
		mpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		layout->addWidget(mpTable, 1);

		auto *btnRow = new QHBoxLayout;
		auto *appendBtn  = new QPushButton(tr("Append blank..."), this);
		auto *insTextBtn = new QPushButton(tr("Insert text..."), this);
		auto *insToneBtn = new QPushButton(tr("Insert mark tone..."), this);
		auto *upBtn      = new QPushButton(tr("Move ↑"), this);
		auto *downBtn    = new QPushButton(tr("Move ↓"), this);
		auto *deleteBtn  = new QPushButton(tr("Delete"), this);
		auto *saveBtn    = new QPushButton(tr("Save..."), this);
		btnRow->addWidget(appendBtn);
		btnRow->addWidget(insTextBtn);
		btnRow->addWidget(insToneBtn);
		btnRow->addWidget(upBtn);
		btnRow->addWidget(downBtn);
		btnRow->addWidget(deleteBtn);
		btnRow->addStretch(1);
		btnRow->addWidget(saveBtn);
		layout->addLayout(btnRow);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Close, this);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::accept);
		layout->addWidget(box);

		connect(appendBtn,  &QPushButton::clicked, this, &ATTapeEditorDialog::onAppendBlank);
		connect(insTextBtn, &QPushButton::clicked, this, &ATTapeEditorDialog::onInsertText);
		connect(insToneBtn, &QPushButton::clicked, this, &ATTapeEditorDialog::onInsertTone);
		connect(upBtn,      &QPushButton::clicked, this, [this]{ moveSelected(-1); });
		connect(downBtn,    &QPushButton::clicked, this, [this]{ moveSelected(+1); });
		connect(deleteBtn,  &QPushButton::clicked, this, &ATTapeEditorDialog::onDeleteRegion);
		connect(saveBtn,    &QPushButton::clicked, this, &ATTapeEditorDialog::onSave);

		reload();
	}

private:
	IATCassetteImage *image() const {
		return mpSim->GetCassette().GetImage();
	}

	float samplesToSeconds(uint32 s) const {
		return (float)s * kATCassetteSecondsPerDataSample;
	}

	uint32 secondsToSamples(float s) const {
		// Round to nearest data sample; the cassette image uses 32 KHz
		// data samples internally so 1 s ≈ 32000 samples.
		double samples = (double)s * (double)kATCassetteDataSampleRateD;
		if (samples < 0.0) samples = 0.0;
		if (samples > (double)kATCassetteDataLimit) samples = (double)kATCassetteDataLimit;
		return (uint32)(samples + 0.5);
	}

	void reload() {
		auto *img = image();
		mRegions = enumerateRegions(img);

		mpTable->setRowCount((int)mRegions.size());
		for (size_t i = 0; i < mRegions.size(); ++i) {
			const auto& r = mRegions[i];
			auto *idxItem = new QTableWidgetItem(QString::number(i + 1));
			auto *typeItem = new QTableWidgetItem(QString::fromUtf8(regionTypeName(r.type)));
			auto *startItem = new QTableWidgetItem(
				QString::number(samplesToSeconds(r.startSample), 'f', 3));
			auto *lenItem = new QTableWidgetItem(
				QString::number(samplesToSeconds(r.lengthSamples), 'f', 3));
			for (auto *it : {idxItem, typeItem, startItem, lenItem})
				it->setFlags(it->flags() & ~Qt::ItemIsEditable);
			idxItem->setTextAlignment(Qt::AlignCenter);
			startItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			lenItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			mpTable->setItem((int)i, 0, idxItem);
			mpTable->setItem((int)i, 1, typeItem);
			mpTable->setItem((int)i, 2, startItem);
			mpTable->setItem((int)i, 3, lenItem);
		}

		QString header;
		if (!img) {
			header = tr("No cassette image loaded.");
		} else {
			const float lenSec = samplesToSeconds(img->GetDataLength());
			header = tr("Cassette: %1 region(s), data length %2 s")
				.arg(mRegions.size())
				.arg(lenSec, 0, 'f', 3);
		}
		mpHeader->setText(header);
	}

	void onAppendBlank() {
		auto *img = image();
		if (!img) {
			QMessageBox::information(this, tr("Append blank region"),
				tr("No cassette image is loaded. Use File → Cassette → New Tape "
				   "or Load... first."));
			return;
		}
		bool ok = false;
		const double secs = QInputDialog::getDouble(this,
			tr("Append blank region"),
			tr("Length (seconds):"),
			1.0, 0.001, 600.0, 3, &ok);
		if (!ok) return;
		const uint32 samples = secondsToSamples((float)secs);
		if (samples == 0) {
			QMessageBox::warning(this, tr("Append blank region"),
				tr("Length is below the cassette sample resolution."));
			return;
		}
		ATCassetteWriteCursor cursor;
		cursor.mPosition = img->GetDataLength();
		cursor.mCachedBlockIndex = 0;
		// Take the cassette mutex equivalent: ask the cassette emulator to
		// put itself into modify state and notify it after the change so
		// caches reset cleanly.
		auto& cas = mpSim->GetCassette();
		cas.OnPreModifyTape();
		img->WriteBlankData(cursor, samples, /*insert=*/true);
		cas.OnPostModifyTape(cursor.mPosition);
		reload();
	}

	void onDeleteRegion() {
		auto *img = image();
		if (!img) {
			QMessageBox::information(this, tr("Delete region"),
				tr("No cassette image is loaded."));
			return;
		}
		const int row = mpTable->currentRow();
		if (row < 0 || row >= (int)mRegions.size()) {
			QMessageBox::information(this, tr("Delete region"),
				tr("Select a region to delete."));
			return;
		}
		const Region r = mRegions[row];
		const auto reply = QMessageBox::question(this, tr("Delete region"),
			tr("Delete region #%1 (%2, length %3 s)?")
				.arg(row + 1)
				.arg(QString::fromUtf8(regionTypeName(r.type)))
				.arg(samplesToSeconds(r.lengthSamples), 0, 'f', 3),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (reply != QMessageBox::Yes) return;
		auto& cas = mpSim->GetCassette();
		cas.OnPreModifyTape();
		img->DeleteRange(r.startSample, r.startSample + r.lengthSamples);
		cas.OnPostModifyTape(r.startSample);
		reload();
	}

	void onInsertText() {
		auto *img = image();
		if (!img) {
			QMessageBox::information(this, tr("Insert text"),
				tr("No cassette image is loaded."));
			return;
		}
		bool ok = false;
		const QString text = QInputDialog::getText(this, tr("Insert text"),
			tr("Bytes to write (interpreted as ASCII):"),
			QLineEdit::Normal, QStringLiteral("HELLO"), &ok);
		if (!ok || text.isEmpty()) return;
		const int baud = QInputDialog::getInt(this, tr("Insert text"),
			tr("Baud rate:"), 600, 50, 9600, 50, &ok);
		if (!ok) return;

		// Insert at the start of the selected region, or at end of tape if
		// none selected.
		uint32 pos = img->GetDataLength();
		const int row = mpTable->currentRow();
		if (row >= 0 && row < (int)mRegions.size())
			pos = mRegions[row].startSample;

		ATCassetteWriteCursor cursor;
		cursor.mPosition = pos;
		cursor.mCachedBlockIndex = 0;
		auto& cas = mpSim->GetCassette();
		cas.OnPreModifyTape();
		const QByteArray bytes = text.toLatin1();
		for (char c : bytes)
			img->WriteStdData(cursor, (uint8)c, (uint32)baud, /*insert=*/true);
		cas.OnPostModifyTape(pos);
		reload();
	}

	void onInsertTone() {
		auto *img = image();
		if (!img) {
			QMessageBox::information(this, tr("Insert tone"),
				tr("No cassette image is loaded."));
			return;
		}
		bool ok = false;
		const double secs = QInputDialog::getDouble(this, tr("Insert tone"),
			tr("Mark tone length (seconds):"),
			0.5, 0.001, 600.0, 3, &ok);
		if (!ok) return;
		const uint32 samples = secondsToSamples((float)secs);
		if (samples == 0) return;

		uint32 pos = img->GetDataLength();
		const int row = mpTable->currentRow();
		if (row >= 0 && row < (int)mRegions.size())
			pos = mRegions[row].startSample;

		ATCassetteWriteCursor cursor;
		cursor.mPosition = pos;
		cursor.mCachedBlockIndex = 0;
		auto& cas = mpSim->GetCassette();
		cas.OnPreModifyTape();
		img->WritePulse(cursor, /*polarity=*/true, samples,
			/*insert=*/true, /*fsk=*/true);
		cas.OnPostModifyTape(pos);
		reload();
	}

	void moveSelected(int dir) {
		auto *img = image();
		if (!img) return;
		const int row = mpTable->currentRow();
		if (row < 0 || row >= (int)mRegions.size()) return;
		const int otherRow = row + dir;
		if (otherRow < 0 || otherRow >= (int)mRegions.size()) return;

		const Region a = mRegions[std::min(row, otherRow)];
		const Region b = mRegions[std::max(row, otherRow)];

		// Copy B, delete it, insert it at A's start. The other region
		// shifts naturally.
		auto& cas = mpSim->GetCassette();
		cas.OnPreModifyTape();
		auto clip = img->CopyRange(b.startSample, b.lengthSamples);
		img->DeleteRange(b.startSample, b.startSample + b.lengthSamples);
		img->InsertRange(a.startSample, *clip);
		cas.OnPostModifyTape(a.startSample);

		// Reselect the moved row.
		reload();
		const int newRow = (dir < 0) ? row - 1 : row + 1;
		if (newRow >= 0 && newRow < (int)mRegions.size())
			mpTable->setCurrentCell(newRow, 0);
	}

	void onSave() {
		auto *img = image();
		if (!img) {
			QMessageBox::information(this, tr("Save"),
				tr("No cassette image is loaded."));
			return;
		}
		const QString path = QFileDialog::getSaveFileName(this,
			tr("Save cassette image"),
			QStringLiteral("tape.cas"),
			tr("CAS file (*.cas);;WAV file (*.wav)"));
		if (path.isEmpty()) return;
		try {
			VDFileStream fs(path.toStdWString().c_str(),
				nsVDFile::kWrite | nsVDFile::kDenyAll | nsVDFile::kCreateAlways);
			if (path.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive))
				ATSaveCassetteImageWAV(fs, img);
			else
				ATSaveCassetteImageCAS(fs, img);
		} catch (const MyError& e) {
			QMessageBox::warning(this, tr("Save failed"),
				QString::fromUtf8(e.c_str()));
			return;
		}
		QMessageBox::information(this, tr("Save"),
			tr("Saved: %1").arg(QFileInfo(path).fileName()));
	}

	ATSimulator       *mpSim;
	QLabel            *mpHeader = nullptr;
	QTableWidget      *mpTable  = nullptr;
	std::vector<Region> mRegions;
};

} // namespace

void ATShowTapeEditorDialog(QWidget *parent, ATSimulator *sim) {
	ATTapeEditorDialog dlg(parent, sim);
	dlg.exec();
}
