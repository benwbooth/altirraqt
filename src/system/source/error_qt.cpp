//	Altirra - Qt port
//	POSIX/Qt-lite replacement for error_win32.cpp.
//	Copyright (C) 2026 Ben Booth
//
//	Upstream uses Win32 MessageBoxW (plus an OS-taskbar progress indicator) to
//	surface exceptions. We just print to stderr. When the UI layer lands, a
//	Qt-aware dispatcher can install a hook that routes through QMessageBox.

#include <stdafx.h>
#include <cstdio>
#include <vd2/system/Error.h>
#include <vd2/system/text.h>
#include <vd2/system/VDString.h>

void VDPostException(VDExceptionPostContext /*context*/, const char *message, const char *title) {
	std::fprintf(stderr, "[%s] %s\n",
		title ? title : "error",
		message ? message : "(null)");
}

void VDPostException(VDExceptionPostContext /*context*/, const wchar_t *message, const wchar_t *title) {
	const VDStringA t = VDTextWToA(title ? title : L"error");
	const VDStringA m = VDTextWToA(message ? message : L"(null)");
	std::fprintf(stderr, "[%s] %s\n", t.c_str(), m.c_str());
}
