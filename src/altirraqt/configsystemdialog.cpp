//	Altirra - Qt port
//	Configure System dialog implementation.

#include "configsystemdialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSettings>
#include <QVBoxLayout>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name.
#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <constants.h>
#include <cpu.h>
#include <irqcontroller.h>
#include <simulator.h>

namespace {

struct MemOpt { ATMemoryMode mode; const char *label; };
constexpr MemOpt kMemOpts[] = {
	{kATMemoryMode_8K,           "8K"},
	{kATMemoryMode_16K,          "16K"},
	{kATMemoryMode_24K,          "24K"},
	{kATMemoryMode_32K,          "32K"},
	{kATMemoryMode_40K,          "40K"},
	{kATMemoryMode_48K,          "48K"},
	{kATMemoryMode_52K,          "52K"},
	{kATMemoryMode_64K,          "64K"},
	{kATMemoryMode_128K,         "128K (130XE)"},
	{kATMemoryMode_256K,         "256K (Rambo)"},
	{kATMemoryMode_320K,         "320K (Rambo)"},
	{kATMemoryMode_320K_Compy,   "320K (Compy)"},
	{kATMemoryMode_576K,         "576K"},
	{kATMemoryMode_576K_Compy,   "576K (Compy)"},
	{kATMemoryMode_1088K,        "1088K"},
};

struct ClearOpt { ATMemoryClearMode mode; const char *label; };
constexpr ClearOpt kClearOpts[] = {
	{kATMemoryClearMode_Zero,    "Zero"},
	{kATMemoryClearMode_Random,  "Randomized"},
	{kATMemoryClearMode_DRAM1,   "DRAM pattern 1"},
	{kATMemoryClearMode_DRAM2,   "DRAM pattern 2"},
};

struct VidOpt { ATVideoStandard std; const char *label; };
constexpr VidOpt kVidOpts[] = {
	{kATVideoStandard_NTSC,    "NTSC"},
	{kATVideoStandard_PAL,     "PAL"},
	{kATVideoStandard_SECAM,   "SECAM"},
	{kATVideoStandard_PAL60,   "PAL-60"},
	{kATVideoStandard_NTSC50,  "NTSC-50"},
};

struct CpuOpt { ATCPUMode mode; const char *label; };
constexpr CpuOpt kCpuOpts[] = {
	{kATCPUMode_6502,   "6502 (stock)"},
	{kATCPUMode_65C02,  "65C02"},
	{kATCPUMode_65C816, "65C816 (Rapidus)"},
};

struct SubOpt { uint32 mult; const char *label; };
constexpr SubOpt kSubOpts[] = {
	{1,  "1.79 MHz (1x)"},
	{2,  "3.58 MHz (2x)"},
	{4,  "7.16 MHz (4x)"},
	{8,  "14.32 MHz (8x)"},
	{16, "28.64 MHz (16x)"},
};

class ATConfigSystemDialog : public QDialog {
public:
	ATConfigSystemDialog(QWidget *parent, ATSimulator *sim, QSettings *settings)
		: QDialog(parent), mpSim(sim), mpSettings(settings)
	{
		setWindowTitle(tr("Configure System"));
		resize(360, 220);

		auto *layout = new QVBoxLayout(this);
		auto *form = new QFormLayout;

		mpMem = new QComboBox;
		for (size_t i = 0; i < std::size(kMemOpts); ++i) {
			mpMem->addItem(QString::fromUtf8(kMemOpts[i].label));
			if (kMemOpts[i].mode == sim->GetMemoryMode()) mpMem->setCurrentIndex((int)i);
		}
		form->addRow(tr("Memory:"), mpMem);

		mpClear = new QComboBox;
		for (size_t i = 0; i < std::size(kClearOpts); ++i) {
			mpClear->addItem(QString::fromUtf8(kClearOpts[i].label));
			if (kClearOpts[i].mode == sim->GetMemoryClearMode()) mpClear->setCurrentIndex((int)i);
		}
		form->addRow(tr("Power-on RAM:"), mpClear);

		mpVid = new QComboBox;
		for (size_t i = 0; i < std::size(kVidOpts); ++i) {
			mpVid->addItem(QString::fromUtf8(kVidOpts[i].label));
			if (kVidOpts[i].std == sim->GetVideoStandard()) mpVid->setCurrentIndex((int)i);
		}
		form->addRow(tr("Video standard:"), mpVid);

		mpCpu = new QComboBox;
		for (size_t i = 0; i < std::size(kCpuOpts); ++i) {
			mpCpu->addItem(QString::fromUtf8(kCpuOpts[i].label));
			if (kCpuOpts[i].mode == sim->GetCPUMode()) mpCpu->setCurrentIndex((int)i);
		}
		form->addRow(tr("CPU type:"), mpCpu);

		mpSub = new QComboBox;
		for (size_t i = 0; i < std::size(kSubOpts); ++i) {
			mpSub->addItem(QString::fromUtf8(kSubOpts[i].label));
			if (kSubOpts[i].mult == sim->GetCPUSubCycles()) mpSub->setCurrentIndex((int)i);
		}
		form->addRow(tr("CPU clock:"), mpSub);

		layout->addLayout(form);

		auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		layout->addWidget(box);
		connect(box, &QDialogButtonBox::accepted, this, &ATConfigSystemDialog::onAccept);
		connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	}

private:
	void onAccept() {
		const int mi = mpMem->currentIndex();
		const int ci = mpClear->currentIndex();
		const int vi = mpVid->currentIndex();
		const int cpui = mpCpu->currentIndex();
		const int subi = mpSub->currentIndex();
		if (mi < 0 || ci < 0 || vi < 0 || cpui < 0 || subi < 0) {
			reject(); return;
		}

		mpSim->SetMemoryMode(kMemOpts[mi].mode);
		mpSim->SetMemoryClearMode(kClearOpts[ci].mode);
		mpSim->SetVideoStandard(kVidOpts[vi].std);
		mpSim->SetCPUMode(kCpuOpts[cpui].mode, kSubOpts[subi].mult);

		if (mpSettings) {
			mpSettings->setValue(QStringLiteral("system/memoryMode"),      (int)kMemOpts[mi].mode);
			mpSettings->setValue(QStringLiteral("system/memoryClearMode"), (int)kClearOpts[ci].mode);
			mpSettings->setValue(QStringLiteral("system/videoStandard"),   (int)kVidOpts[vi].std);
			mpSettings->setValue(QStringLiteral("system/cpuMode"),         (int)kCpuOpts[cpui].mode);
			mpSettings->setValue(QStringLiteral("system/cpuSubCycles"),    (int)kSubOpts[subi].mult);
		}

		mpSim->ColdReset();
		accept();
	}

	ATSimulator *mpSim;
	QSettings   *mpSettings;
	QComboBox *mpMem;
	QComboBox *mpClear;
	QComboBox *mpVid;
	QComboBox *mpCpu;
	QComboBox *mpSub;
};

} // namespace

void ATShowConfigSystemDialog(QWidget *parent, ATSimulator *sim, QSettings *settings) {
	ATConfigSystemDialog dlg(parent, sim, settings);
	dlg.exec();
}
