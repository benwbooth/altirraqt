//	Altirra - Qt port
//	Audio Levels dialog. Live-tunes the simulator's master volume, mute,
//	and per-mix levels (Drive / Covox / Modem / Cassette / Other) via the
//	IATAudioOutput interface.

#pragma once

class QWidget;
class QSettings;
class ATSimulator;

void ATShowAudioLevelsDialog(QWidget *parent, ATSimulator *sim, QSettings *settings = nullptr);
