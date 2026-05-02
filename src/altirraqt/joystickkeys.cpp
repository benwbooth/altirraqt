//	Altirra - Qt port
//	Joystick key mapping table + persistence.

#include "joystickkeys.h"

#include <cstdio>

#include <QSettings>
#include <QString>
#include <Qt>

std::array<std::array<int, kATJoyKeyCount>, kATJoyPortCount> g_atJoyKeys = {{
	{ Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right, Qt::Key_Alt },
	{ 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0 },
}};

namespace {
constexpr const char *kSubKeys[kATJoyKeyCount] = {
	"up", "down", "left", "right", "fire",
};
constexpr const char *kLabels[kATJoyKeyCount] = {
	"Up", "Down", "Left", "Right", "Fire",
};
} // namespace

const char *ATJoyKeyLabel(ATJoyKeyIndex idx) {
	return (idx >= 0 && idx < kATJoyKeyCount) ? kLabels[idx] : "";
}

const char *ATJoyKeySettingName(int port, ATJoyKeyIndex idx) {
	if (port < 0 || port >= kATJoyPortCount) return "";
	if (idx < 0 || idx >= kATJoyKeyCount) return "";
	// Cached static buffer per port/idx pair so callers can keep the pointer.
	static char names[kATJoyPortCount][kATJoyKeyCount][32];
	static bool inited = false;
	if (!inited) {
		for (int p = 0; p < kATJoyPortCount; ++p) {
			for (int k = 0; k < kATJoyKeyCount; ++k) {
				std::snprintf(names[p][k], sizeof(names[p][k]),
					"input/joy%d/%s", p, kSubKeys[k]);
			}
		}
		inited = true;
	}
	return names[port][idx];
}

void ATLoadJoyKeys(QSettings& s) {
	for (int p = 0; p < kATJoyPortCount; ++p) {
		for (int i = 0; i < kATJoyKeyCount; ++i) {
			const QString k = QString::fromUtf8(
				ATJoyKeySettingName(p, (ATJoyKeyIndex)i));
			if (s.contains(k))
				g_atJoyKeys[p][i] = s.value(k).toInt();
		}
	}
}

void ATSaveJoyKeys(QSettings& s) {
	for (int p = 0; p < kATJoyPortCount; ++p) {
		for (int i = 0; i < kATJoyKeyCount; ++i) {
			s.setValue(QString::fromUtf8(
				ATJoyKeySettingName(p, (ATJoyKeyIndex)i)),
				g_atJoyKeys[p][i]);
		}
	}
}
