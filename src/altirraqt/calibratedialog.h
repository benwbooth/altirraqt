//	Altirra - Qt port
//	Display calibration test pattern. Pops a fullscreen, frameless dialog
//	showing a known greyscale ramp + colour bars + geometry guides so the
//	user can adjust their host display. Esc / click closes.

#pragma once

class QWidget;

void ATShowCalibrateDialog(QWidget *parent);
