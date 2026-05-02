//	Altirra - Qt port
//	Debugger Registers pane implementation.
//
//	Renders the active CPU's register file as a fixed-width text block in
//	a read-only QPlainTextEdit. We register an IATSimulatorCallback so we
//	get notified on pause/resume/cold-reset/single-step etc., and refresh
//	the view whenever any of those fire.
//
//	The CPU mode (6502 / 65C02 / 65C816) is read from
//	ATSimulator::GetCPUMode(); only the relevant fields are shown.

#include "debuggerregisterspane.h"

#include <QDockWidget>
#include <QFontDatabase>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include <cstdio>

#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <cpu.h>
#include <irqcontroller.h>
#include <simeventmanager.h>
#include <simulator.h>

namespace {

class ATDebuggerRegistersWidget : public QWidget, public IATSimulatorCallback {
public:
	ATDebuggerRegistersWidget(QWidget *parent, ATSimulator *sim)
		: QWidget(parent), mpSim(sim)
	{
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);

		mpView = new QPlainTextEdit(this);
		mpView->setReadOnly(true);
		mpView->setLineWrapMode(QPlainTextEdit::NoWrap);
		mpView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		layout->addWidget(mpView, 1);

		mpSim->GetEventManager()->AddCallback(this);
		refresh();
	}

	~ATDebuggerRegistersWidget() override {
		if (mpSim)
			mpSim->GetEventManager()->RemoveCallback(this);
	}

	// IATSimulatorCallback
	void OnSimulatorEvent(ATSimulatorEvent ev) override {
		switch (ev) {
			case kATSimEvent_CPUSingleStep:
			case kATSimEvent_CPUStackBreakpoint:
			case kATSimEvent_CPUPCBreakpoint:
			case kATSimEvent_ReadBreakpoint:
			case kATSimEvent_WriteBreakpoint:
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
		auto& cpu = mpSim->GetCPU();
		const ATCPUMode mode = mpSim->GetCPUMode();

		char buf[1024];
		int off = 0;
		auto append = [&](const char *fmt, auto... args) {
			off += std::snprintf(buf + off, sizeof(buf) - off, fmt, args...);
		};

		const uint8 P = cpu.GetP();
		auto flagchar = [&](uint8 mask, char c) -> char {
			return (P & mask) ? c : '-';
		};

		switch (mode) {
			case kATCPUMode_6502:
			default:
				append("PC = %04X    P = %02X  [%c%c%c%c%c%c%c%c]\n",
				       cpu.GetPC(), P,
				       flagchar(0x80, 'N'),
				       flagchar(0x40, 'V'),
				       flagchar(0x20, '1'),
				       flagchar(0x10, 'B'),
				       flagchar(0x08, 'D'),
				       flagchar(0x04, 'I'),
				       flagchar(0x02, 'Z'),
				       flagchar(0x01, 'C'));
				append("A  = %02X      X = %02X    Y = %02X    S = %02X\n",
				       cpu.GetA(), cpu.GetX(), cpu.GetY(), cpu.GetS());
				break;

			case kATCPUMode_65C02:
				append("PC = %04X    P = %02X  [%c%c%c%c%c%c%c%c]   (65C02)\n",
				       cpu.GetPC(), P,
				       flagchar(0x80, 'N'),
				       flagchar(0x40, 'V'),
				       flagchar(0x20, '1'),
				       flagchar(0x10, 'B'),
				       flagchar(0x08, 'D'),
				       flagchar(0x04, 'I'),
				       flagchar(0x02, 'Z'),
				       flagchar(0x01, 'C'));
				append("A  = %02X      X = %02X    Y = %02X    S = %02X\n",
				       cpu.GetA(), cpu.GetX(), cpu.GetY(), cpu.GetS());
				break;

			case kATCPUMode_65C816: {
				const bool emu = cpu.GetEmulationFlag();
				append("PC = %02X:%04X (%s)    P = %02X  [%c%c%c%c%c%c%c%c]\n",
				       cpu.GetK(), cpu.GetPC(),
				       emu ? "emu" : "native",
				       P,
				       flagchar(0x80, 'N'),
				       flagchar(0x40, 'V'),
				       flagchar(0x20, 'M'),
				       flagchar(0x10, 'X'),
				       flagchar(0x08, 'D'),
				       flagchar(0x04, 'I'),
				       flagchar(0x02, 'Z'),
				       flagchar(0x01, 'C'));
				append("A  = %02X%02X    X = %02X%02X  Y = %02X%02X  S = %02X%02X\n",
				       cpu.GetAH(), cpu.GetA(),
				       cpu.GetXH(), cpu.GetX(),
				       cpu.GetYH(), cpu.GetY(),
				       cpu.GetSH(), cpu.GetS());
				append("DBR= %02X     D = %04X    K = %02X\n",
				       cpu.GetB(), cpu.GetD(), cpu.GetK());
				break;
			}
		}

		const ATCPUSubMode submode = cpu.GetCPUSubMode();
		(void)submode;

		mpView->setPlainText(QString::fromUtf8(buf));
	}

	ATSimulator *mpSim = nullptr;
	QPlainTextEdit *mpView = nullptr;
};

QDockWidget *g_dock = nullptr;

}  // namespace

void ATShowDebuggerRegistersPane(QMainWindow *parent, ATSimulator *sim) {
	if (!g_dock) {
		g_dock = new QDockWidget(QObject::tr("Registers"), parent);
		g_dock->setObjectName(QStringLiteral("DebuggerRegistersDock"));
		g_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
		auto *w = new ATDebuggerRegistersWidget(g_dock, sim);
		g_dock->setWidget(w);
		parent->addDockWidget(Qt::RightDockWidgetArea, g_dock);
	}
	g_dock->show();
	g_dock->raise();
}
