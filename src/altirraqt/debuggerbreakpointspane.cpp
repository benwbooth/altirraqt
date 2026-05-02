//	Altirra - Qt port
//	Debugger Breakpoints pane implementation.
//
//	Pulls the active user-breakpoint list via
//	IATDebugger::GetBreakpointList + GetBreakpointInfo, and renders one
//	row per breakpoint into a QTableWidget. The Add button prompts for
//	a hex PC and calls ToggleBreakpoint; the Remove button calls
//	ClearUserBreakpoint. The Clear All button calls ClearAllBreakpoints.

#include "debuggerbreakpointspane.h"

#include <QDockWidget>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <cstdio>

#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/vdstl.h>
#include <debugger.h>
#include <irqcontroller.h>
#include <simeventmanager.h>
#include <simulator.h>

namespace {

class ATBreakpointsWidget : public QWidget, public IATSimulatorCallback {
public:
	ATBreakpointsWidget(QWidget *parent, ATSimulator *sim)
		: QWidget(parent), mpSim(sim)
	{
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);
		layout->setSpacing(2);

		mpTable = new QTableWidget(0, 4, this);
		mpTable->setHorizontalHeaderLabels({tr("ID"), tr("Address"), tr("Length"), tr("Kind")});
		mpTable->horizontalHeader()->setStretchLastSection(true);
		mpTable->verticalHeader()->setVisible(false);
		mpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		mpTable->setSelectionMode(QAbstractItemView::SingleSelection);
		mpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		mpTable->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		layout->addWidget(mpTable, 1);

		auto *buttons = new QHBoxLayout;
		mpAddPC = new QPushButton(tr("Add PC..."), this);
		mpAddRead = new QPushButton(tr("Add Read..."), this);
		mpAddWrite = new QPushButton(tr("Add Write..."), this);
		mpRemove = new QPushButton(tr("Remove"), this);
		mpClearAll = new QPushButton(tr("Clear All"), this);
		buttons->addWidget(mpAddPC);
		buttons->addWidget(mpAddRead);
		buttons->addWidget(mpAddWrite);
		buttons->addWidget(mpRemove);
		buttons->addWidget(mpClearAll);
		buttons->addStretch(1);
		layout->addLayout(buttons);

		connect(mpAddPC, &QPushButton::clicked, this, [this]{
			uint addr;
			if (!promptHex(tr("Add PC breakpoint"), addr)) return;
			IATDebugger *dbg = ATGetDebugger();
			if (dbg) dbg->ToggleBreakpoint((uint16)addr);
			refresh();
		});
		connect(mpAddRead, &QPushButton::clicked, this, [this]{
			uint addr;
			if (!promptHex(tr("Add read access breakpoint"), addr)) return;
			IATDebugger *dbg = ATGetDebugger();
			if (dbg) dbg->ToggleAccessBreakpoint((uint16)addr, /*write=*/false);
			refresh();
		});
		connect(mpAddWrite, &QPushButton::clicked, this, [this]{
			uint addr;
			if (!promptHex(tr("Add write access breakpoint"), addr)) return;
			IATDebugger *dbg = ATGetDebugger();
			if (dbg) dbg->ToggleAccessBreakpoint((uint16)addr, /*write=*/true);
			refresh();
		});
		connect(mpRemove, &QPushButton::clicked, this, [this]{
			const int row = mpTable->currentRow();
			if (row < 0) return;
			QTableWidgetItem *idItem = mpTable->item(row, 0);
			if (!idItem) return;
			const uint32 useridx = (uint32)idItem->data(Qt::UserRole).toUInt();
			IATDebugger *dbg = ATGetDebugger();
			if (dbg) dbg->ClearUserBreakpoint(useridx, true);
			refresh();
		});
		connect(mpClearAll, &QPushButton::clicked, this, [this]{
			IATDebugger *dbg = ATGetDebugger();
			if (dbg) dbg->ClearAllBreakpoints();
			refresh();
		});

		mpSim->GetEventManager()->AddCallback(this);
		refresh();
	}

	~ATBreakpointsWidget() override {
		if (mpSim)
			mpSim->GetEventManager()->RemoveCallback(this);
	}

	void OnSimulatorEvent(ATSimulatorEvent ev) override {
		switch (ev) {
			case kATSimEvent_CPUPCBreakpointsUpdated:
			case kATSimEvent_ColdReset:
			case kATSimEvent_WarmReset:
			case kATSimEvent_StateLoaded:
			case kATSimEvent_SimPause:
				refresh();
				break;
			default:
				break;
		}
	}

private:
	bool promptHex(const QString& title, uint& out) {
		bool ok = false;
		const QString text = QInputDialog::getText(this, title,
			tr("Address (hex):"), QLineEdit::Normal, {}, &ok);
		if (!ok || text.isEmpty()) return false;
		out = text.trimmed().toUInt(&ok, 16);
		return ok;
	}

	void refresh() {
		mpTable->setRowCount(0);

		IATDebugger *dbg = ATGetDebugger();
		if (!dbg) return;

		vdfastvector<uint32> ids;
		dbg->GetBreakpointList(ids);

		for (uint32 id : ids) {
			ATDebuggerBreakpointInfo info;
			if (!dbg->GetBreakpointInfo(id, info)) continue;

			const int row = mpTable->rowCount();
			mpTable->insertRow(row);

			char idstr[16];
			std::snprintf(idstr, sizeof idstr, "%u", (unsigned)id);
			auto *idItem = new QTableWidgetItem(QString::fromUtf8(idstr));
			idItem->setData(Qt::UserRole, (uint)id);
			mpTable->setItem(row, 0, idItem);

			char addrstr[32];
			if (info.mAddress >= 0)
				std::snprintf(addrstr, sizeof addrstr, "$%04X", (unsigned)info.mAddress);
			else
				std::snprintf(addrstr, sizeof addrstr, "(insn)");
			mpTable->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(addrstr)));

			char lenstr[16];
			std::snprintf(lenstr, sizeof lenstr, "%u", (unsigned)info.mLength);
			mpTable->setItem(row, 2, new QTableWidgetItem(QString::fromUtf8(lenstr)));

			QString kind;
			if (info.mbBreakOnPC)    kind += QStringLiteral("PC ");
			if (info.mbBreakOnRead)  kind += QStringLiteral("R ");
			if (info.mbBreakOnWrite) kind += QStringLiteral("W ");
			if (info.mbBreakOnInsn)  kind += QStringLiteral("Insn ");
			if (kind.isEmpty()) kind = QStringLiteral("?");
			mpTable->setItem(row, 3, new QTableWidgetItem(kind.trimmed()));
		}
	}

	ATSimulator *mpSim = nullptr;
	QTableWidget *mpTable = nullptr;
	QPushButton *mpAddPC = nullptr;
	QPushButton *mpAddRead = nullptr;
	QPushButton *mpAddWrite = nullptr;
	QPushButton *mpRemove = nullptr;
	QPushButton *mpClearAll = nullptr;
};

QDockWidget *g_dock = nullptr;

}  // namespace

void ATShowDebuggerBreakpointsPane(QMainWindow *parent, ATSimulator *sim) {
	if (!g_dock) {
		g_dock = new QDockWidget(QObject::tr("Breakpoints"), parent);
		g_dock->setObjectName(QStringLiteral("DebuggerBreakpointsDock"));
		g_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
		auto *w = new ATBreakpointsWidget(g_dock, sim);
		g_dock->setWidget(w);
		parent->addDockWidget(Qt::BottomDockWidgetArea, g_dock);
	}
	g_dock->show();
	g_dock->raise();
}
