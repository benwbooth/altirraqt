//	Altirra - Qt port
//	Debugger Memory pane implementation.
//
//	Address bar at the top + 16-bytes-per-line hex/ASCII dump below.
//	Reads bytes via ATSimulator::GetCPUMemory().DebugReadByte (so it
//	doesn't trigger watchpoints). Refreshes on the same simulator-event
//	set as the registers/disassembly panes.

#include "debuggermemorypane.h"

#include <QDockWidget>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cstdio>

#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <cpumemory.h>
#include <irqcontroller.h>
#include <simeventmanager.h>
#include <simulator.h>

namespace {

class ATMemoryWidget : public QWidget, public IATSimulatorCallback {
public:
	ATMemoryWidget(QWidget *parent, ATSimulator *sim)
		: QWidget(parent), mpSim(sim)
	{
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);
		layout->setSpacing(2);

		auto *header = new QHBoxLayout;
		header->addWidget(new QLabel(tr("Address (hex):"), this));
		mpAddrEntry = new QLineEdit(this);
		mpAddrEntry->setText(QStringLiteral("0000"));
		mpAddrEntry->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		header->addWidget(mpAddrEntry, 1);
		header->addWidget(new QLabel(tr("Lines:"), this));
		mpRowSpin = new QSpinBox(this);
		mpRowSpin->setRange(1, 4096);
		mpRowSpin->setValue(32);
		header->addWidget(mpRowSpin);
		mpGo = new QPushButton(tr("Go"), this);
		header->addWidget(mpGo);
		layout->addLayout(header);

		mpView = new QPlainTextEdit(this);
		mpView->setReadOnly(true);
		mpView->setLineWrapMode(QPlainTextEdit::NoWrap);
		mpView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		layout->addWidget(mpView, 1);

		connect(mpGo, &QPushButton::clicked, this, [this]{ refresh(); });
		connect(mpAddrEntry, &QLineEdit::returnPressed, this, [this]{ refresh(); });
		connect(mpRowSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){ refresh(); });

		mpSim->GetEventManager()->AddCallback(this);
		refresh();
	}

	~ATMemoryWidget() override {
		if (mpSim)
			mpSim->GetEventManager()->RemoveCallback(this);
	}

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
				refresh();
				break;
			default:
				break;
		}
	}

private:
	void refresh() {
		bool ok = false;
		uint addr = mpAddrEntry->text().trimmed().toUInt(&ok, 16);
		if (!ok) addr = 0;
		const int rows = mpRowSpin->value();

		auto& mem = mpSim->GetCPUMemory();
		QString out;
		out.reserve(rows * 80);

		for (int r = 0; r < rows; ++r) {
			const uint16 base = (uint16)(addr + r * 16);
			char hex[64];
			char asc[20];
			int hexOff = 0;
			int ascOff = 0;
			for (int i = 0; i < 16; ++i) {
				const uint8 b = mem.DebugReadByte((uint16)(base + i));
				hexOff += std::snprintf(hex + hexOff, sizeof(hex) - hexOff,
				                        "%02X ", b);
				asc[ascOff++] = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
			}
			asc[ascOff] = 0;
			char line[128];
			std::snprintf(line, sizeof line, "%04X: %s |%s|\n", base, hex, asc);
			out += QString::fromUtf8(line);
		}
		mpView->setPlainText(out);
	}

	ATSimulator *mpSim = nullptr;
	QLineEdit   *mpAddrEntry = nullptr;
	QSpinBox    *mpRowSpin = nullptr;
	QPushButton *mpGo = nullptr;
	QPlainTextEdit *mpView = nullptr;
};

QDockWidget *g_dock = nullptr;

}  // namespace

void ATShowDebuggerMemoryPane(QMainWindow *parent, ATSimulator *sim) {
	if (!g_dock) {
		g_dock = new QDockWidget(QObject::tr("Memory"), parent);
		g_dock->setObjectName(QStringLiteral("DebuggerMemoryDock"));
		g_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
		auto *w = new ATMemoryWidget(g_dock, sim);
		g_dock->setWidget(w);
		parent->addDockWidget(Qt::RightDockWidgetArea, g_dock);
	}
	g_dock->show();
	g_dock->raise();
}
