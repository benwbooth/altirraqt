//	Altirra - Qt port
//	Joystick Keys dialog. Lets the user rebind the five keys that drive
//	the host-keyboard joystick (port 1): up / down / left / right / fire.

#pragma once

class QWidget;
class QSettings;

void ATShowJoystickKeysDialog(QWidget *parent, QSettings *settings);
