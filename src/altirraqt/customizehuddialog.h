//	Altirra - Qt port
//	HUD customize dialog. Toggles each on-screen overlay element. Settings
//	are persisted under `view/hud/<name>` and applied to the QtDisplay
//	widget.

#pragma once

class QSettings;
class QWidget;

// Show the HUD customize dialog modally. Reads/writes persisted settings
// and pushes the chosen on/off states into the QtDisplay widget via
// ATQtVideoDisplaySetHudEnabled.
void ATShowCustomizeHudDialog(QWidget *parent, QWidget *displayWidget, QSettings *settings);

// Apply persisted view/hud/<name> settings to the display widget. Call
// this at startup after the display widget is created so the user's
// previous choices light up immediately.
void ATApplyHudSettings(QWidget *displayWidget, QSettings *settings);
