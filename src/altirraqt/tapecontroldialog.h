//	Altirra - Qt port
//	Tape Control dialog. Shows cassette transport state (Play/Stop/Pause/
//	Record), position vs. length, and a seek slider. Polls the cassette
//	emulator on a timer to keep the UI in sync while the tape runs.

#pragma once

class QWidget;
class ATSimulator;

void ATShowTapeControlDialog(QWidget *parent, ATSimulator *sim);
