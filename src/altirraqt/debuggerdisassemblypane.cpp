//	Altirra - Qt port
//	Debugger Disassembly pane implementation.
//
//	Renders ~64 instructions around the current PC into a custom
//	QListWidget-style text view. Each line begins with a 1-character
//	gutter ('*' if a PC breakpoint is set there) followed by address,
//	bytes, and the disassembled mnemonic. Clicking the gutter toggles a
//	breakpoint via IATDebugger::ToggleBreakpoint.
//
//	Disassembly comes from the upstream ATDisassembleInsn() helpers,
//	which are now linked into the Qt build via altirra_core.

#include "debuggerdisassemblypane.h"

#include <QAction>
#include <QDockWidget>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>

#include <vector>

#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <at/atcpu/history.h>
#include <at/atdebugger/symbols.h>
#include <cpu.h>
#include <cpumemory.h>
#include <debugger.h>
#include <disasm.h>
#include <QFile>
#include <QFileInfo>
#include <irqcontroller.h>
#include <simeventmanager.h>
#include <simulator.h>

namespace {

class ATDisassemblyWidget : public QWidget, public IATSimulatorCallback {
public:
	ATDisassemblyWidget(QWidget *parent, ATSimulator *sim)
		: QWidget(parent), mpSim(sim)
	{
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);
		layout->setSpacing(2);

		auto *header = new QHBoxLayout;
		header->addWidget(new QLabel(tr("Address:"), this));
		mpAddrEntry = new QLineEdit(this);
		mpAddrEntry->setPlaceholderText(tr("hex (PC if blank)"));
		mpAddrEntry->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		header->addWidget(mpAddrEntry, 1);
		mpFollowPc = new QPushButton(tr("Follow PC"), this);
		header->addWidget(mpFollowPc);
		layout->addLayout(header);

		mpList = new QListWidget(this);
		mpList->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		mpList->setUniformItemSizes(true);
		mpList->setSelectionMode(QAbstractItemView::SingleSelection);
		layout->addWidget(mpList, 1);

		connect(mpAddrEntry, &QLineEdit::editingFinished, this, [this]{
			const QString s = mpAddrEntry->text().trimmed();
			bool ok = false;
			uint addr = s.toUInt(&ok, 16);
			if (ok) {
				mFollowPC = false;
				mAddress = (uint16)addr;
				refresh();
			} else if (s.isEmpty()) {
				mFollowPC = true;
				refresh();
			}
		});
		connect(mpFollowPc, &QPushButton::clicked, this, [this]{
			mFollowPC = true;
			refresh();
		});
		connect(mpList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
			// If the click was within the gutter (first 2 chars), toggle BP.
			// We approximate by checking the click position via stored userData.
			if (!item) return;
			const uint32 addr = item->data(Qt::UserRole).toUInt();
			IATDebugger *dbg = ATGetDebugger();
			if (!dbg) return;
			dbg->ToggleBreakpoint(addr);
			refresh();
		});

		mpSim->GetEventManager()->AddCallback(this);
		refresh();
	}

	~ATDisassemblyWidget() override {
		if (mpSim)
			mpSim->GetEventManager()->RemoveCallback(this);
	}

	void OnSimulatorEvent(ATSimulatorEvent ev) override {
		switch (ev) {
			case kATSimEvent_CPUSingleStep:
			case kATSimEvent_CPUStackBreakpoint:
			case kATSimEvent_CPUPCBreakpoint:
			case kATSimEvent_CPUPCBreakpointsUpdated:
			case kATSimEvent_AnonymousPause:
			case kATSimEvent_AnonymousInterrupt:
			case kATSimEvent_ColdReset:
			case kATSimEvent_WarmReset:
			case kATSimEvent_StateLoaded:
			case kATSimEvent_SimPause:
			case kATSimEvent_SimResume:
				refresh();
				break;
			default:
				break;
		}
	}

private:
	void refresh() {
		mpList->clear();

		IATDebugger *dbg = ATGetDebugger();
		if (!dbg) return;

		uint16 pc = dbg->GetPC();
		uint16 startAddr = mFollowPC ? pc : mAddress;

		// Walk back a couple of instructions worth of bytes — without a
		// proper anchor, just back up by 16 bytes and let alignment sort
		// itself out.
		uint16 addr = startAddr - 16;
		auto& cpu = mpSim->GetCPU();
		auto& mem = mpSim->GetCPUMemory();

		IATDebuggerSymbolLookup *sl = ATGetDebuggerSymbolLookup();
		uint32 lastPrintedFid = 0xFFFFFFFFu;
		uint16 lastPrintedLine = 0xFFFF;

		for (int i = 0; i < 80; ++i) {
			const uint16 lineAddr = addr;

			// If a source line maps to this address, emit it as a
			// non-clickable italic header before the instruction.
			if (sl) {
				uint32 mid = 0;
				ATSourceLineInfo li{};
				if (sl->LookupLine(lineAddr, false, mid, li)
					&& li.mOffset == lineAddr
					&& (li.mFileId != lastPrintedFid || li.mLine != lastPrintedLine))
				{
					ATDebuggerSourceFileInfo info;
					QString src;
					if (sl->GetSourceFilePath(mid, li.mFileId, info)) {
						QFile f(QString::fromStdWString(info.mSourcePath.c_str()));
						if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
							int row = 0;
							while (!f.atEnd()) {
								const QByteArray ln = f.readLine();
								if (++row == li.mLine) {
									src = QString::fromUtf8(ln).trimmed();
									break;
								}
							}
						}
					}
					if (!src.isEmpty()) {
						auto *header = new QListWidgetItem(
							QStringLiteral("    %1:%2: %3")
								.arg(QFileInfo(QString::fromStdWString(info.mSourcePath.c_str())).fileName())
								.arg(li.mLine).arg(src));
						QFont fH = mpList->font();
						fH.setItalic(true);
						header->setFont(fH);
						header->setForeground(QColor(80, 120, 180));
						header->setFlags(header->flags() & ~Qt::ItemIsSelectable);
						mpList->addItem(header);
					}
					lastPrintedFid  = li.mFileId;
					lastPrintedLine = li.mLine;
				}
			}

			VDStringA buf;
			const uint16 next = ATDisassembleInsn(buf, lineAddr, false);
			const uint16 len = (uint16)(next - lineAddr);

			VDStringA bytes;
			for (uint16 b = 0; b < len && b < 4; ++b) {
				char tmp[8];
				std::snprintf(tmp, sizeof tmp, "%02X ",
				              mem.DebugReadByte(lineAddr + b));
				bytes += tmp;
			}
			while (bytes.size() < 12) bytes += ' ';

			char gutter = ' ';
			if (dbg->IsBreakpointAtPC(lineAddr))
				gutter = '*';
			char marker = (lineAddr == pc) ? '>' : ' ';

			char line[160];
			std::snprintf(line, sizeof line, "%c %c %04X  %s %s",
			              gutter, marker, lineAddr, bytes.c_str(), buf.c_str());

			auto *item = new QListWidgetItem(QString::fromUtf8(line));
			item->setData(Qt::UserRole, (uint)lineAddr);
			if (lineAddr == pc) {
				QFont f = mpList->font();
				f.setBold(true);
				item->setFont(f);
			}
			mpList->addItem(item);

			if (next <= addr) break;  // wrap / single-byte stuck case
			addr = next;
			if (addr < lineAddr) break;  // 16-bit wrap
			(void)cpu;
		}

		// Try to keep the PC line visible.
		for (int i = 0; i < mpList->count(); ++i) {
			if (mpList->item(i)->data(Qt::UserRole).toUInt() == pc) {
				mpList->scrollToItem(mpList->item(i), QAbstractItemView::PositionAtCenter);
				break;
			}
		}
	}

	ATSimulator *mpSim = nullptr;
	QLineEdit   *mpAddrEntry = nullptr;
	QPushButton *mpFollowPc = nullptr;
	QListWidget *mpList = nullptr;
	bool         mFollowPC = true;
	uint16       mAddress = 0;
};

QDockWidget *g_dock = nullptr;

}  // namespace

void ATShowDebuggerDisassemblyPane(QMainWindow *parent, ATSimulator *sim) {
	if (!g_dock) {
		g_dock = new QDockWidget(QObject::tr("Disassembly"), parent);
		g_dock->setObjectName(QStringLiteral("DebuggerDisassemblyDock"));
		g_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
		auto *w = new ATDisassemblyWidget(g_dock, sim);
		g_dock->setWidget(w);
		parent->addDockWidget(Qt::RightDockWidgetArea, g_dock);
	}
	g_dock->show();
	g_dock->raise();
}
