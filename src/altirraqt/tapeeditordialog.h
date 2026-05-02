//	Altirra - Qt port
//	Tape Editor dialog. Lists the regions (blocks) of the loaded cassette
//	image and exposes the subset of mutations the IATCassetteImage API
//	supports — insert blank region, delete region — plus save.

#pragma once

class QWidget;
class ATSimulator;

void ATShowTapeEditorDialog(QWidget *parent, ATSimulator *sim);
