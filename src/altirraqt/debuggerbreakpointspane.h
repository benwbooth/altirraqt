//	Altirra - Qt port
//	Debugger Breakpoints pane. QDockWidget listing PC and access
//	breakpoints currently registered with IATDebugger. Each row has an
//	address column, kind column ("PC" / "R" / "W" / "RW" / "Insn"), and
//	a delete button.

#pragma once

class QMainWindow;
class ATSimulator;

void ATShowDebuggerBreakpointsPane(QMainWindow *parent, ATSimulator *sim);
