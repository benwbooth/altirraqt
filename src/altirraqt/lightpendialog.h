//	Altirra - Qt port
//	Light Pen / Gun adjustment dialog. Tunes the position offsets the
//	emulator applies to gun (CX-75 trigger) and pen inputs, and selects
//	the noise model (None / CX-75+800 / CX-75+XL).

#pragma once

class QWidget;
class ATSimulator;

void ATShowLightPenDialog(QWidget *parent, ATSimulator *sim);
