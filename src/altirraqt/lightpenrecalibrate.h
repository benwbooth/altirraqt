//	Altirra - Qt port
//	Interactive light-pen / gun recalibration. Pops a target overlay over
//	the display widget; the user clicks four reference crosshairs, the
//	average offset is written back to ATLightPenPort.

#pragma once

class QMainWindow;
class QWidget;
class ATSimulator;

void ATRunLightPenRecalibrate(QMainWindow *window, QWidget *displayWidget, ATSimulator *sim);
