//	Altirra - Qt port
//	Debugger History pane implementation.
//
//	Pulls instruction-history entries from the active debug target's
//	IATDebugTargetHistory interface (queried via AsInterface). Each
//	pause refreshes the most recent N entries and renders them as
//	disassembled lines into a QListWidget.
//
//	An "Enable history" toggle wires through SetHistoryEnabled — the
//	CPU only fills the buffer when this is on, and it costs cycles, so
//	the user opts in.

#include "debuggerhistorypane.h"

#include <QCheckBox>
#include <QDockWidget>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cstdio>

#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/unknown.h>
#include <at/atcpu/history.h>
#include <at/atdebugger/target.h>
#include <cpu.h>
#include <debugger.h>
#include <disasm.h>
#include <irqcontroller.h>
#include <simeventmanager.h>
#include <simulator.h>

namespace {

class ATHistoryWidget : public QWidget, public IATSimulatorCallback {
public:
	ATHistoryWidget(QWidget *parent, ATSimulator *sim)
		: QWidget(parent), mpSim(sim)
	{
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);
		layout->setSpacing(2);

		auto *header = new QHBoxLayout;
		mpEnable = new QCheckBox(tr("Enable history"), this);
		mpEnable->setChecked(mpSim->GetCPU().IsHistoryEnabled());
		header->addWidget(mpEnable);
		header->addWidget(new QLabel(tr("Show last:"), this));
		mpCount = new QSpinBox(this);
		mpCount->setRange(1, 4096);
		mpCount->setValue(256);
		header->addWidget(mpCount);
		mpRefresh = new QPushButton(tr("Refresh"), this);
		header->addWidget(mpRefresh);
		header->addStretch(1);
		layout->addLayout(header);

		mpList = new QListWidget(this);
		mpList->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		mpList->setUniformItemSizes(true);
		layout->addWidget(mpList, 1);

		connect(mpEnable, &QCheckBox::toggled, this, [this](bool on){
			mpSim->GetCPU().SetHistoryEnabled(on);
		});
		connect(mpRefresh, &QPushButton::clicked, this, [this]{ refresh(); });
		connect(mpCount, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){ refresh(); });

		mpSim->GetEventManager()->AddCallback(this);
		refresh();
	}

	~ATHistoryWidget() override {
		if (mpSim)
			mpSim->GetEventManager()->RemoveCallback(this);
	}

	void OnSimulatorEvent(ATSimulatorEvent ev) override {
		switch (ev) {
			case kATSimEvent_CPUSingleStep:
			case kATSimEvent_CPUStackBreakpoint:
			case kATSimEvent_CPUPCBreakpoint:
			case kATSimEvent_AnonymousPause:
			case kATSimEvent_SimPause:
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
		IATDebugTarget *target = dbg->GetTarget();
		if (!target) return;
		auto *hist = vdpoly_cast<IATDebugTargetHistory *>(target);
		if (!hist || !hist->GetHistoryEnabled()) {
			mpList->addItem(tr("(history disabled — tick the checkbox to enable)"));
			return;
		}

		const auto range = hist->GetHistoryRange();
		const uint32 begin = range.first;
		const uint32 end = range.second;
		if (begin == end) return;

		const uint32 want = (uint32)mpCount->value();
		const uint32 total = end - begin;
		const uint32 n = total < want ? total : want;
		const uint32 startIdx = end - n;

		// Pull entries in chunks; use a small fixed buffer.
		constexpr uint32 kChunk = 64;
		const ATCPUHistoryEntry *ents[kChunk];
		const ATDebugDisasmMode disasmMode = target->GetDisasmMode();

		for (uint32 i = 0; i < n; i += kChunk) {
			const uint32 chunk = (n - i > kChunk) ? kChunk : (n - i);
			const uint32 got = hist->ExtractHistory(ents, startIdx + i, chunk);
			for (uint32 j = 0; j < got; ++j) {
				const ATCPUHistoryEntry& he = *ents[j];

				VDStringA buf;
				ATDisassembleInsn(buf,
					target,
					disasmMode,
					he,
					/*decodeReferences=*/false,
					/*decodeRefsHistory=*/true,
					/*showPCAddress=*/true,
					/*showCodeBytes=*/true,
					/*showLabels=*/false);

				char prefix[24];
				std::snprintf(prefix, sizeof prefix, "%08u  ", (unsigned)he.mCycle);

				QString line = QString::fromUtf8(prefix) + QString::fromUtf8(buf.c_str());
				// Trim trailing newline if any.
				while (line.endsWith(QChar('\n')) || line.endsWith(QChar('\r')))
					line.chop(1);
				mpList->addItem(line);
			}
		}

		if (mpList->count() > 0)
			mpList->scrollToBottom();
	}

	ATSimulator *mpSim = nullptr;
	QCheckBox   *mpEnable = nullptr;
	QSpinBox    *mpCount = nullptr;
	QPushButton *mpRefresh = nullptr;
	QListWidget *mpList = nullptr;
};

QDockWidget *g_dock = nullptr;

}  // namespace

void ATShowDebuggerHistoryPane(QMainWindow *parent, ATSimulator *sim) {
	if (!g_dock) {
		g_dock = new QDockWidget(QObject::tr("History"), parent);
		g_dock->setObjectName(QStringLiteral("DebuggerHistoryDock"));
		g_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
		auto *w = new ATHistoryWidget(g_dock, sim);
		g_dock->setWidget(w);
		parent->addDockWidget(Qt::BottomDockWidgetArea, g_dock);
	}
	g_dock->show();
	g_dock->raise();
}
