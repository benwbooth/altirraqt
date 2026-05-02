//	Altirra - Qt port
//	Debugger Registers pane. QDockWidget showing 6502/65C02/65C816
//	register state, refreshed each time the simulator pauses (and on
//	resume, to clear the live values).

#pragma once

class QMainWindow;
class ATSimulator;

void ATShowDebuggerRegistersPane(QMainWindow *parent, ATSimulator *sim);
