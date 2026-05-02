//	altirraqt — Keyboard Layout dialog.
//
//	Lets the user remap the Atari console keys (Start/Select/Option/Reset/
//	Help/Break) to alternate host-key sequences. Persists each as
//	`input/keyboard/<atari-name>` = Qt key code. main.cpp's keyboard
//	handler consults these overrides before the default mapping.

#pragma once

class QSettings;
class QWidget;

void ATShowKeyboardLayoutDialog(QWidget *parent, QSettings *settings);

// Look up the user override for a console key. Returns 0 if no
// override is set. Names: "start", "select", "option", "reset", "help",
// "break".
int  ATKeyboardLayoutGetOverride(QSettings& s, const char *name);
