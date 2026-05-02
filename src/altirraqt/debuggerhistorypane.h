//	Altirra - Qt port
//	Debugger History pane. QDockWidget rendering the simulator's
//	instruction-history ring buffer as a scrollable list.

#pragma once

class QMainWindow;
class ATSimulator;

void ATShowDebuggerHistoryPane(QMainWindow *parent, ATSimulator *sim);
