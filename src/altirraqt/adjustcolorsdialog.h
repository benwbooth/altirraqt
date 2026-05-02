//	Altirra - Qt port
//	Adjust Colors dialog. Lets the user tune GTIA's color synthesis
//	parameters live: hue start/range, brightness, contrast, saturation,
//	gamma, artifact hue/saturation/sharpness, RGB shifts.

#pragma once

class QWidget;
class ATSimulator;

void ATShowAdjustColorsDialog(QWidget *parent, ATSimulator *sim);
