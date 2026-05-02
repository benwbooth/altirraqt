//	Altirra - Qt port
//	Debugger Disassembly pane. QDockWidget showing disassembled
//	instructions around the current PC. Clicking the breakpoint gutter
//	at the start of a line toggles a PC breakpoint at that address.

#pragma once

class QMainWindow;
class ATSimulator;

void ATShowDebuggerDisassemblyPane(QMainWindow *parent, ATSimulator *sim);
