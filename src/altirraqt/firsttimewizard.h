//	Altirra - Qt port
//	First Time Setup wizard. Walks the user through initial choices
//	(hardware mode, video standard, BASIC, audio defaults) and writes
//	them to QSettings before the simulator boots for the first time.

#pragma once

class QWidget;
class QSettings;

// Run the wizard modally. If the user accepts, the chosen settings are
// written to `settings` and the function returns true. Returns false on
// cancel; in that case `settings` is left untouched.
bool ATRunFirstTimeWizard(QWidget *parent, QSettings *settings);
