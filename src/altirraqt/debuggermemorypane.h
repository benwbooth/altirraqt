//	Altirra - Qt port
//	Debugger Memory pane. QDockWidget showing a hex+ASCII dump of a
//	user-specified address range, refreshed on simulator pause / reset.

#pragma once

class QMainWindow;
class ATSimulator;

void ATShowDebuggerMemoryPane(QMainWindow *parent, ATSimulator *sim);
