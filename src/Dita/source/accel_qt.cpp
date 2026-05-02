//	Altirra - Qt port
//	Minimal Qt-backed implementation of accelerator → string conversion.
//	Copyright (C) 2026 Ben Booth
//
//	Upstream's w32accel.cpp uses GetKeyNameText, MapVirtualKey and the input
//	locale layout to produce a localized string like "Ctrl+Alt+F". For this
//	port we use a small lookup table for modifiers and a hex fallback for the
//	virtual key code. Friendlier strings can be added later via Qt's
//	QKeySequence machinery, but only the input-mapping UI shows them.

#include <stdafx.h>
#include <vd2/system/VDString.h>
#include <vd2/Dita/accel.h>

#include <cstdio>

void VDUIGetAcceleratorString(const VDUIAccelerator& accel, VDStringW& s) {
	s.clear();

	if (accel.mModifiers & VDUIAccelerator::kModCtrl)  s += L"Ctrl+";
	if (accel.mModifiers & VDUIAccelerator::kModAlt)   s += L"Alt+";
	if (accel.mModifiers & VDUIAccelerator::kModShift) s += L"Shift+";

	wchar_t buf[16];
	std::swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"0x%04X", accel.mVirtKey);
	s += buf;
}

bool VDUIGetVkAcceleratorForChar(VDUIAccelerator& accel, wchar_t c) {
	accel.mVirtKey = (uint32)c;
	accel.mModifiers = 0;
	return true;
}

bool VDUIGetCharAcceleratorForVk(VDUIAccelerator& accel) {
	return false;
}
