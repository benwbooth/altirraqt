//	Altirra - Qt port
//	Disk Drives dialog. Replaces upstream's per-drive menu attach/detach
//	commands with a single panel listing all 8 drives with mount/eject
//	and write-mode controls.

#pragma once

class QMainWindow;
class ATSimulator;

void ATShowDiskDrivesDialog(QMainWindow *parent, ATSimulator *sim);
