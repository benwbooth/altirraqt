//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2004 Avery Lee, All Rights Reserved.
//
//	Cross-platform Qt port: originally wrapped MessageBoxA + IsProcessor
//	FeaturePresent + Win32 FPU/MMX/SSE state traps. On POSIX we:
//	  - emit asserts to stderr and trap into the debugger via SIGTRAP / raise(),
//	  - route VDDebugPrint to stderr,
//	  - drop the FPU/MMX/SSE state-trap scaffolding entirely (x87/MMX only
//	    survive in Win32 32-bit builds; x86_64 uses SSE throughout).

#include <stdafx.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <signal.h>

#include <vd2/system/vdtypes.h>
#include <vd2/system/cpuaccel.h>
#include <vd2/system/debug.h>
#include <vd2/system/thread.h>

#ifdef _DEBUG

namespace {
	std::mutex            g_ignoredMutex;
	std::vector<void *>   g_ignoredRet;

	bool IsIgnored(void *ret) {
		std::lock_guard<std::mutex> g(g_ignoredMutex);
		return std::find(g_ignoredRet.begin(), g_ignoredRet.end(), ret) != g_ignoredRet.end();
	}

	void AddIgnored(void *ret) {
		std::lock_guard<std::mutex> g(g_ignoredMutex);
		if (std::find(g_ignoredRet.begin(), g_ignoredRet.end(), ret) == g_ignoredRet.end())
			g_ignoredRet.push_back(ret);
	}

	VDAssertResult InteractiveAssert(const char *label, const char *exp, const char *file, int line) {
		void *retaddr = __builtin_return_address(0);
		if (IsIgnored(retaddr))
			return kVDAssertIgnore;

		fprintf(stderr,
			"\n*** %s failed in %s(%d):\n  %s\n"
			"[b]reak, [c]ontinue, [i]gnore (default: break)? ",
			label, file, line, exp);
		fflush(stderr);

		char buf[16] = {};
		if (!fgets(buf, sizeof buf, stdin))
			return kVDAssertBreak;

		switch (buf[0]) {
			case 'c': case 'C':
				return kVDAssertContinue;
			case 'i': case 'I':
				AddIgnored(retaddr);
				return kVDAssertIgnore;
			default:
				return kVDAssertBreak;
		}
	}
}

VDAssertResult VDAssert(const char *exp, const char *file, int line) {
	return InteractiveAssert("Assert", exp, file, line);
}

VDAssertResult VDAssertPtr(const char *exp, const char *file, int line) {
	char buf[512];
	snprintf(buf, sizeof buf, "(%s) is not a valid pointer", exp);
	return InteractiveAssert("Pointer assert", buf, file, line);
}

#endif

void VDProtectedAutoScopeICLWorkaround() {}

void VDDebugPrint(const char *format, ...) {
	char buf[4096];

	va_list val;
	va_start(val, format);
	vsnprintf(buf, sizeof buf, format, val);
	va_end(val);

	fputs(buf, stderr);
}

///////////////////////////////////////////////////////////////////////////

namespace {
	IVDExternalCallTrap *g_pExCallTrap;
}

void VDSetExternalCallTrap(IVDExternalCallTrap *trap) {
	g_pExCallTrap = trap;
}

bool IsMMXState() {
	return false;
}

void ClearMMXState() {
}

void VDClearEvilCPUStates() {
}

void VDPreCheckExternalCodeCall(const char *file, int line) {
	(void)file; (void)line;
}

void VDPostCheckExternalCodeCall(const wchar_t *mpContext, const char *mpFile, int mLine) {
	(void)mpContext; (void)mpFile; (void)mLine;
}
