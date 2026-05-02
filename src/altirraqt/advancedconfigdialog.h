//	Altirra - Qt port
//	Advanced Configuration dialog. Multi-tab dialog exposing the toggles
//	that don't fit in the System / View / Cassette menus.

#pragma once

class QWidget;
class QSettings;
class ATSimulator;

void ATShowAdvancedConfigDialog(QWidget *parent, ATSimulator *sim, QSettings *settings = nullptr);
