//	Altirra - Qt port
//	Tiny shim for the upstream Win32-only header.
//
//	Upstream `tracenative.cpp` includes this header to look up readable
//	names for Win32 window messages while it logs the host event loop.
//	The Qt port has no such message stream — the function exists, but
//	always returns nullptr, which `tracenative.cpp` already treats as
//	"no name available; format the numeric id instead". That keeps the
//	tracenative code path live (frames + CPU regions still trace) on
//	Linux without any upstream-source edits.

#ifndef f_AT_ATNATIVEUI_DEBUG_WIN32_H
#define f_AT_ATNATIVEUI_DEBUG_WIN32_H

#include <vd2/system/vdtypes.h>

const char *ATGetNameForWindowMessageW32(uint32 msgId);

#endif
