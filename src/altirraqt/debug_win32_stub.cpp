//	Altirra - Qt port
//	Companion to src/h/at/atnativeui/debug_win32.h.
//
//	`tracenative.cpp` calls ATGetNameForWindowMessageW32() to label tick
//	events on the "Window Msg" trace channel. There is no Win32 window
//	message stream on Linux — the Qt port runs its own event loop — so
//	we always return nullptr; tracenative falls back to formatting the
//	raw numeric id ("0x%X"). All other tracer machinery (frames, CPU
//	regions) works unchanged.

#include <vd2/system/vdtypes.h>
#include <at/atnativeui/debug_win32.h>

const char *ATGetNameForWindowMessageW32(uint32 /*msgId*/) {
	return nullptr;
}
