// Cross-platform Qt port: original implementation used CreateProcessW.
// POSIX port uses fork()+execvp() and delegates argument splitting to /bin/sh
// via QProcess::startDetached when Qt is available. Here we keep the dependency
// footprint tight and use posix_spawn.

#include <stdafx.h>
#include <vd2/system/process.h>
#include <vd2/system/VDString.h>
#include <vd2/system/text.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

namespace {
	VDStringA narrow(const wchar_t *s) {
		return s ? VDTextWToA(s, -1) : VDStringA();
	}

	// Very small whitespace-respecting argument splitter. Handles double-quoted
	// runs and backslash escapes; matches the subset the callers actually pass.
	void append_args(std::vector<VDStringA>& out, const VDStringA& cmdLine) {
		VDStringA cur;
		bool inQuotes = false;
		bool escape = false;
		for (char c : cmdLine) {
			if (escape) {
				cur.push_back(c);
				escape = false;
				continue;
			}
			if (c == '\\') { escape = true; continue; }
			if (c == '"') { inQuotes = !inQuotes; continue; }
			if (!inQuotes && (c == ' ' || c == '\t')) {
				if (!cur.empty()) { out.push_back(std::move(cur)); cur.clear(); }
				continue;
			}
			cur.push_back(c);
		}
		if (!cur.empty()) out.push_back(std::move(cur));
	}
}

void VDLaunchProgram(const wchar_t *path, const wchar_t *args) {
	const VDStringA pathA = narrow(path);
	const VDStringA argsA = narrow(args);

#if defined(_WIN32)
	// Build a single command line: "path" args
	VDStringA cmd("\"");
	cmd += pathA;
	cmd += "\"";
	if (!argsA.empty()) {
		cmd += " ";
		cmd += argsA;
	}
	STARTUPINFOA si{};
	si.cb = sizeof si;
	PROCESS_INFORMATION pi{};
	if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE,
	                    0, nullptr, nullptr, &si, &pi))
		throw VDException("Unable to launch process (error %lu)", (unsigned long)GetLastError());
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
#else
	std::vector<VDStringA> argv;
	argv.push_back(pathA);
	append_args(argv, argsA);

	std::vector<char *> rawArgv;
	rawArgv.reserve(argv.size() + 1);
	for (auto& a : argv)
		rawArgv.push_back(const_cast<char *>(a.c_str()));
	rawArgv.push_back(nullptr);

	pid_t pid = 0;
	int rc = posix_spawnp(&pid, pathA.c_str(), nullptr, nullptr, rawArgv.data(), environ);
	if (rc != 0)
		throw VDException("Unable to launch process: %s", strerror(rc));
#endif
}
