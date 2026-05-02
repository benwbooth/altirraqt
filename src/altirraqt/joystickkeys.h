//	Altirra - Qt port
//	Joystick-to-keyboard mapping. The on-screen joysticks are driven by
//	five host keys per port: up / down / left / right / fire. Defaults
//	bind port 0 to arrow keys + Left-Alt (the long-standing Altirra
//	convention); ports 1-3 are unbound by default. The mapping is shared
//	between main.cpp's key dispatcher and the Joystick Keys dialog.

#pragma once

#include <array>

class QSettings;

enum ATJoyKeyIndex : int {
	kATJoyKey_Up = 0,
	kATJoyKey_Down,
	kATJoyKey_Left,
	kATJoyKey_Right,
	kATJoyKey_Fire,
	kATJoyKeyCount
};

constexpr int kATJoyPortCount = 4;

// Qt::Key codes, one per direction/button per port. 0 means unbound.
extern std::array<std::array<int, kATJoyKeyCount>, kATJoyPortCount> g_atJoyKeys;

const char *ATJoyKeyLabel(ATJoyKeyIndex idx);
const char *ATJoyKeySettingName(int port, ATJoyKeyIndex idx);

void ATLoadJoyKeys(QSettings& settings);
void ATSaveJoyKeys(QSettings& settings);
