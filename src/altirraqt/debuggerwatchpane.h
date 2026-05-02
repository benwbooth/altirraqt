//	Altirra - Qt port
//	Debugger Watch pane. QDockWidget showing user-defined watch
//	expressions and their current values, evaluated each time the
//	simulator pauses (via IATDebugger::EvaluateThrow).

#pragma once

class QMainWindow;
class ATSimulator;

void ATShowDebuggerWatchPane(QMainWindow *parent, ATSimulator *sim);
