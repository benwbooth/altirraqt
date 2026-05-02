//	altirraqt — Debugger source-files pane.
//
//	Reads the loaded debugger's source-file table (via IATDebugger::EnumSourceFiles
//	plus IATDebuggerSymbolLookup::GetSourceFilePath) and shows each file in a
//	tab. Highlights the line at the current PC and toggles a breakpoint when
//	the user clicks the gutter.

#pragma once

class QWidget;
class ATSimulator;

void ATShowDebuggerSourcePane(QWidget *parent, ATSimulator *sim);
