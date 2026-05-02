//	Altirra - Qt port
//	Debugger Console pane. QDockWidget hosting a read-only output area
//	and a command-line entry. Submitting a command calls
//	ATConsoleExecuteCommand(); output produced via ATConsoleWrite() is
//	routed back into the pane through a global listener registered by
//	the dock.

#pragma once

class QMainWindow;

void ATShowDebuggerConsolePane(QMainWindow *parent);

// Sink used by ATConsoleWrite() in stubs.cpp to forward output to the
// active console pane (or stderr if none is open). Returns true if the
// text was consumed by a live pane.
bool ATDebuggerConsoleSink(const char *s);
