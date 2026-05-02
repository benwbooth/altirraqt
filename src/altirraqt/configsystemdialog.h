//	Altirra - Qt port
//	Configure System dialog. Combines a few core simulator knobs (memory
//	size, RAM-on-power-up pattern, video standard) so users can change
//	them without diving into upstream's per-setting menu items.

#pragma once

class QWidget;
class QSettings;
class ATSimulator;

void ATShowConfigSystemDialog(QWidget *parent, ATSimulator *sim, QSettings *settings = nullptr);
