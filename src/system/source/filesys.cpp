//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2004 Avery Lee, All Rights Reserved.
//
//	Beginning with 1.6.0, the VirtualDub system library is licensed
//	differently than the remainder of VirtualDub.  This particular file is
//	thus licensed as follows (the "zlib" license):
//
//	This software is provided 'as-is', without any express or implied
//	warranty.  In no event will the authors be held liable for any
//	damages arising from the use of this software.
//
//	Permission is granted to anyone to use this software for any purpose,
//	including commercial applications, and to alter it and redistribute it
//	freely, subject to the following restrictions:
//
//	1.	The origin of this software must not be misrepresented; you must
//		not claim that you wrote the original software. If you use this
//		software in a product, an acknowledgment in the product
//		documentation would be appreciated but is not required.
//	2.	Altered source versions must be plainly marked as such, and must
//		not be misrepresented as being the original software.
//	3.	This notice may not be removed or altered from any source
//		distribution.

#include <stdafx.h>
#include <ctype.h>
#include <string.h>

#include <vd2/system/VDString.h>
#include <vd2/system/filesys.h>
#include <vd2/system/Error.h>
#include <vd2/system/vdstl.h>
#include <vd2/system/strutil.h>
#include <vd2/system/text.h>

///////////////////////////////////////////////////////////////////////////

namespace {
	template<class T, class U>
	inline T splitimpL(const T& string, const U *s) {
		const U *p = string.c_str();
		return T(p, s - p);
	}

	template<class T, class U>
	inline T splitimpR(const T& string, const U *s) {
		return T(s);
	}
}

///////////////////////////////////////////////////////////////////////////

const char *VDFileSplitFirstDir(const char *s) {
	const char *start = s;

	while(*s++)
		if (s[-1] == ':' || s[-1] == '\\' || s[-1] == '/')
			return s;

	return start;
}

const wchar_t *VDFileSplitFirstDir(const wchar_t *s) {
	const wchar_t *start = s;

	while(*s++)
		if (s[-1] == L':' || s[-1] == L'\\' || s[-1] == L'/')
			return s;

	return start;
}

const char *VDFileSplitPath(const char *s) {
	const char *lastsep = s;

	while(*s++)
		if (s[-1] == ':' || s[-1] == '\\' || s[-1] == '/')
			lastsep = s;

	return lastsep;
}

const wchar_t *VDFileSplitPath(const wchar_t *s) {
	const wchar_t *lastsep = s;

	while(*s++)
		if (s[-1] == L':' || s[-1] == L'\\' || s[-1] == L'/')
			lastsep = s;

	return lastsep;
}

VDString  VDFileSplitPathLeft (const VDString&  s) { return splitimpL(s, VDFileSplitPath(s.c_str())); }
VDStringW VDFileSplitPathLeft (const VDStringW& s) { return splitimpL(s, VDFileSplitPath(s.c_str())); }
VDString  VDFileSplitPathRight(const VDString&  s) { return splitimpR(s, VDFileSplitPath(s.c_str())); }
VDStringW VDFileSplitPathRight(const VDStringW& s) { return splitimpR(s, VDFileSplitPath(s.c_str())); }

const char *VDFileSplitPath(const char *s, const char *t) {
	const char *lastsep = s;

	while(s != t) {
		++s;
		if (s[-1] == ':' || s[-1] == '\\' || s[-1] == '/')
			lastsep = s;
	}

	return lastsep;
}

const wchar_t *VDFileSplitPath(const wchar_t *s, const wchar_t *t) {
	const wchar_t *lastsep = s;

	while(s != t) {
		++s;

		if (s[-1] == L':' || s[-1] == L'\\' || s[-1] == L'/')
			lastsep = s;
	}

	return lastsep;
}

VDStringSpanA VDFileSplitPathLeftSpan(const VDStringSpanA& s) {
	return VDStringSpanA(s.begin(), VDFileSplitPath(s.begin(), s.end()));
}

VDStringSpanA VDFileSplitPathRightSpan(const VDStringSpanA& s) {
	return VDStringSpanA(VDFileSplitPath(s.begin(), s.end()), s.end());
}

VDStringSpanW VDFileSplitPathLeftSpan(const VDStringSpanW& s) {
	return VDStringSpanW(s.begin(), VDFileSplitPath(s.begin(), s.end()));
}

VDStringSpanW VDFileSplitPathRightSpan(const VDStringSpanW& s) {
	return VDStringSpanW(VDFileSplitPath(s.begin(), s.end()), s.end());
}

const char *VDFileSplitRoot(const char *s) {
	// Test for a UNC path.
	if (s[0] == '\\' && s[1] == '\\') {
		// For these, we scan for the fourth backslash.
		s += 2;
		for(int i=0; i<2; ++i) {
			while(*s && *s != '\\')
				++s;
			if (*s == '\\')
				++s;
		}
		return s;
	}

	const char *t = s;

	for(;;) {
		const char c = *t;

		if (c == ':') {
			if (t[1] == '/' || t[1] == '\\')
				return t+2;

			return t+1;
		}

		// check if it was a valid drive identifier
		if (!isalpha((unsigned char)c) && (s == t || !isdigit((unsigned char)c)))
			return s;

		++t;
	}
}

const wchar_t *VDFileSplitRoot(const wchar_t *s) {
	// Test for a UNC path.
	if (s[0] == L'\\' && s[1] == L'\\') {
		// For these, we scan for the fourth backslash.
		s += 2;
		for(int i=0; i<2; ++i) {
			while(*s && *s != L'\\')
				++s;
			if (*s == L'\\')
				++s;
		}
		return s;
	}

	const wchar_t *t = s;

	for(;;) {
		const wchar_t c = *t;

		if (c == L':') {
			if (t[1] == L'/' || t[1] == L'\\')
				return t+2;

			return t+1;
		}

		// check if it was a valid drive identifier
		if (!iswalpha(c) && (s == t || !iswdigit(c)))
			return s;

		++t;
	}
}

VDString  VDFileSplitRoot(const VDString&  s) { return splitimpL(s, VDFileSplitRoot(s.c_str())); }
VDStringW VDFileSplitRoot(const VDStringW& s) { return splitimpL(s, VDFileSplitRoot(s.c_str())); }

const char *VDFileSplitExt(const char *s, const char *t) {
	const char *const end = t;

	while(t>s) {
		--t;

		if (*t == '.')
			return t;

		if (*t == ':' || *t == '\\' || *t == '/')
			break;
	}

	return end;
}

const wchar_t *VDFileSplitExt(const wchar_t *s, const wchar_t *t) {
	const wchar_t *const end = t;

	while(t>s) {
		--t;

		if (*t == L'.')
			return t;

		if (*t == L':' || *t == L'\\' || *t == L'/')
			break;
	}

	return end;
}

const char *VDFileSplitExt(const char *s) {
	const char *t = s;

	while(*t)
		++t;

	return VDFileSplitExt(s, t);
}

const wchar_t *VDFileSplitExt(const wchar_t *s) {
	const wchar_t *t = s;

	while(*t)
		++t;

	return VDFileSplitExt(s, t);
}

VDString  VDFileSplitExtLeft (const VDString&  s) { return splitimpL(s, VDFileSplitExt(s.c_str())); }
VDStringW VDFileSplitExtLeft (const VDStringW& s) { return splitimpL(s, VDFileSplitExt(s.c_str())); }
VDString  VDFileSplitExtRight(const VDString&  s) { return splitimpR(s, VDFileSplitExt(s.c_str())); }
VDStringW VDFileSplitExtRight(const VDStringW& s) { return splitimpR(s, VDFileSplitExt(s.c_str())); }

VDStringSpanA VDFileSplitExtLeftSpan (const VDStringSpanA& s) { return VDStringSpanA(s.begin(), VDFileSplitExt(s.begin(), s.end())); }
VDStringSpanW VDFileSplitExtLeftSpan (const VDStringSpanW& s) { return VDStringSpanW(s.begin(), VDFileSplitExt(s.begin(), s.end())); }
VDStringSpanA VDFileSplitExtRightSpan(const VDStringSpanA& s) { return VDStringSpanA(VDFileSplitExt(s.begin(), s.end()), s.end()); }
VDStringSpanW VDFileSplitExtRighSpant(const VDStringSpanW& s) { return VDStringSpanW(VDFileSplitExt(s.begin(), s.end()), s.end()); }

/////////////////////////////////////////////////////////////////////////////

bool VDFileWildMatch(const char *pattern, const char *path) {
	// What we do here is split the string into segments that are bracketed
	// by sequences of asterisks. The trick is that the first match for a
	// segment as the best possible match, so we can continue. So we just
	// take each segment at a time and walk it forward until we find the
	// first match or we fail.
	//
	// Time complexity is O(NM), where N=length of string and M=length of
	// the pattern. In practice, it's rather fast.

	bool star = false;
	int i = 0;
	for(;;) {
		char c = (char)tolower((unsigned char)pattern[i]);
		if (c == '*') {
			star = true;
			pattern += i+1;
			if (!*pattern)
				return true;
			path += i;
			i = 0;
			continue;
		}

		char d = (char)tolower((unsigned char)path[i]);
		++i;

		if (c == '?') {		// ? matches any character but null.
			if (!d)
				return false;
		} else if (c != d) {	// Literal character must match itself.
			// If we're at the end of the string or there is no
			// previous asterisk (anchored search), there's no other
			// match to find.
			if (!star || !d || !i)
				return false;

			// Restart segment search at next position in path.
			++path;
			i = 0;
			continue;
		}

		if (!c)
			return true;
	}
}

bool VDFileWildMatch(const wchar_t *pattern, const wchar_t *path) {
	// What we do here is split the string into segments that are bracketed
	// by sequences of asterisks. The trick is that the first match for a
	// segment as the best possible match, so we can continue. So we just
	// take each segment at a time and walk it forward until we find the
	// first match or we fail.
	//
	// Time complexity is O(NM), where N=length of string and M=length of
	// the pattern. In practice, it's rather fast.

	bool star = false;
	int i = 0;
	for(;;) {
		wchar_t c = towlower(pattern[i]);
		if (c == L'*') {
			star = true;
			pattern += i+1;
			if (!*pattern)
				return true;
			path += i;
			i = 0;
			continue;
		}

		wchar_t d = towlower(path[i]);
		++i;

		if (c == L'?') {		// ? matches any character but null.
			if (!d)
				return false;
		} else if (c != d) {	// Literal character must match itself.
			// If we're at the end of the string or there is no
			// previous asterisk (anchored search), there's no other
			// match to find.
			if (!star || !d || !i)
				return false;

			// Restart segment search at next position in path.
			++path;
			i = 0;
			continue;
		}

		if (!c)
			return true;
	}
}

/////////////////////////////////////////////////////////////////////////////

VDParsedPath::VDParsedPath()
	: mbIsRelative(true)
{
}

VDParsedPath::VDParsedPath(const wchar_t *path)
	: mbIsRelative(true)
{
	// Check if the string contains a root, such as a drive or UNC specifier.
	const wchar_t *rootSplit = VDFileSplitRoot(path);
	if (rootSplit != path) {
		mRoot.assign(path, rootSplit);

		for(VDStringW::iterator it(mRoot.begin()), itEnd(mRoot.end()); it != itEnd; ++it) {
			if (*it == L'/')
				*it = L'\\';
		}

		mbIsRelative = (mRoot.back() == L':');

		// If the path is UNC, strip a trailing backslash.
		if (mRoot.size() >= 3 && mRoot[0] == L'\\' && mRoot[1] == L'\\' && mRoot.back() == L'\\')
			mRoot.resize(mRoot.size() - 1);

		path = rootSplit;
	}

	// Parse out additional components.
	for(;;) {
		// Skip any separators.
		wchar_t c = *path++;

		while(c == L'\\' || c == L'/')
			c = *path++;

		// If we've hit a null, we're done.
		if (!c)
			break;

		// Have we hit a semicolon? If so, we've found an NTFS stream identifier and we're done.
		if (c == L';') {
			mStream = path;
			break;
		}

		// Skip until we hit a separator or a null.
		const wchar_t *compStart = path - 1;

		while(c && c != L'\\' && c != L'/' && c != L';') {
			c = *path++;
		}

		--path;

		const wchar_t *compEnd = path;

		// Check if we've got a component that starts with .
		const size_t compLen = compEnd - compStart;
		if (*compStart == L'.') {
			// Is it . (current)?
			if (compLen == 1) {
				// Yes -- just ditch it.
				continue;
			}

			// Is it .. (parent)?
			if (compLen == 2 && compStart[1] == L'.') {
				// Yes -- see if we have a previous component we can remove. If we don't have a component,
				// then what we do depends on whether we have an absolute path or not.
				if (!mComponents.empty() && (!mbIsRelative || mComponents.back() != L"..")) {
					mComponents.pop_back();
				} else if (mbIsRelative) {
					// We've got a relative path. This means that we need to preserve this ..; we push
					// it into the root section to prevent it from being backed up over by another ...
					mComponents.push_back() = L"..";
				}

				// Otherwise, we have an absolute path, and we can just drop this.
				continue;
			}
		}

		// Copy the component.
		mComponents.push_back().assign(compStart, compEnd);
	}
}

VDStringW VDParsedPath::ToString() const {
	VDStringW s(mRoot);

	// If we have a UNC path root with something after it, we must ensure a separator.
	if (!mbIsRelative && !s.empty() && !VDIsPathSeparator(s.back()) && !mComponents.empty())
		s += L'\\';

	bool first = true;
	for(Components::const_iterator it(mComponents.begin()), itEnd(mComponents.end()); it != itEnd; ++it) {
		if (!first)
			s += L'\\';
		else
			first = false;

		s.append(*it);
	}

	// If the string is still empty, throw in a .
	if (s.empty())
		s = L".";

	if (!mStream.empty()) {
		s += L';';
		s.append(mStream);
	}

	return s;
}

/////////////////////////////////////////////////////////////////////////////

VDStringW VDFileGetCanonicalPath(const wchar_t *path) {
	return VDParsedPath(path).ToString();
}

VDStringW VDFileGetRelativePath(const wchar_t *basePath, const wchar_t *pathToConvert, bool allowAscent) {
	VDParsedPath base(basePath);
	VDParsedPath path(pathToConvert);

	// Fail if either path is relative.
	if (base.IsRelative() || path.IsRelative())
		return VDStringW();

	// Fail if the roots don't match.
	if (vdwcsicmp(base.GetRoot(), path.GetRoot()))
		return VDStringW();

	// Figure out how many components are in common.
	size_t n1 = base.GetComponentCount();
	size_t n2 = path.GetComponentCount();
	size_t nc = 0;

	while(nc < n1 && nc < n2 && !vdwcsicmp(base.GetComponent(nc), path.GetComponent(nc)))
		++nc;

	// Check how many extra components are in the base; these need to become .. identifiers.
	VDParsedPath relPath;

	if (n1 > nc) {
		if (!allowAscent)
			return VDStringW();

		while(n1 > nc) {
			relPath.AddComponent(L"..");
			--n1;
		}
	}

	// Append extra components from path.
	while(nc < n2) {
		relPath.AddComponent(path.GetComponent(nc++));
	}

	// Copy stream.
	relPath.SetStream(path.GetStream());

	return relPath.ToString();
}

bool VDFileIsRelativePath(const wchar_t *path) {
	VDParsedPath ppath(path);

	return ppath.IsRelative();
}

VDStringW VDFileResolvePath(const wchar_t *basePath, const wchar_t *pathToResolve) {
	if (VDFileIsRelativePath(pathToResolve))
		return VDFileGetCanonicalPath(VDMakePath(basePath, pathToResolve).c_str());

	return VDStringW(pathToResolve);
}


/////////////////////////////////////////////////////////////////////////////
//
// POSIX port of the filesystem/path I/O portion of this file. The original
// Win32 section used GetFullPathNameW, CreateDirectoryW, FindFirstFileW,
// GetFileAttributesW, MoveFileW, DeleteFileW, etc. We implement the same
// entry points via POSIX (stat, opendir, realpath, rename, unlink, mkdir,
// statvfs, readlink) under the convention that paths are UTF-8 narrow when
// crossing into the OS.

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

static constexpr uint64 kWin32EpochToUnixTicks_Filesys = 116444736000000000ull;

namespace {
	VDStringA narrow_path(const wchar_t *path) {
		return path ? VDTextWToU8(path, -1) : VDStringA();
	}

	VDStringW widen_path(const char *path) {
		return path ? VDTextU8ToW(path, -1) : VDStringW();
	}

	VDDate DateFromTimespec(const struct timespec& ts) {
		VDDate d;
		d.mTicks = kWin32EpochToUnixTicks_Filesys
			+ (uint64)ts.tv_sec * 10000000ull
			+ (uint64)ts.tv_nsec / 100ull;
		return d;
	}

	uint32 AttrsFromStat(const struct stat& st) {
		uint32 attrs = 0;
		if (S_ISDIR(st.st_mode))  attrs |= kVDFileAttr_Directory;
		if (S_ISLNK(st.st_mode))  attrs |= kVDFileAttr_Link;
		if (!(st.st_mode & S_IWUSR)) attrs |= kVDFileAttr_ReadOnly;
		return attrs;
	}
}

sint64 VDGetDiskFreeSpace(const wchar_t *path) {
	VDStringA p = narrow_path(path);
	struct statvfs vfs{};
	if (statvfs(p.c_str(), &vfs) != 0)
		return -1;

	return (sint64)vfs.f_bavail * (sint64)vfs.f_frsize;
}

bool VDDoesPathExist(const wchar_t *fileName) {
	VDStringA p = narrow_path(fileName);
	struct stat st{};
	return stat(p.c_str(), &st) == 0;
}

void VDCreateDirectory(const wchar_t *path) {
	VDStringA p = narrow_path(path);
	if (!p.empty() && (p.back() == '/' || p.back() == '\\'))
		p.pop_back();

	if (mkdir(p.c_str(), 0777) != 0 && errno != EEXIST)
		throw VDException("Cannot create directory \"%s\": %s", p.c_str(), strerror(errno));
}

void VDRemoveDirectory(const wchar_t *path) {
	VDStringA p = narrow_path(path);
	if (!p.empty() && (p.back() == '/' || p.back() == '\\'))
		p.pop_back();

	if (rmdir(p.c_str()) != 0)
		throw VDException("Cannot remove directory \"%s\": %s", p.c_str(), strerror(errno));
}

void VDSetDirectoryCreationTime(const wchar_t *path, const VDDate& date) {
	VDStringA p = narrow_path(path);
	const uint64 sinceUnix = date.mTicks - kWin32EpochToUnixTicks_Filesys;
	struct timespec times[2];
	times[0].tv_sec  = (time_t)(sinceUnix / 10000000ull);
	times[0].tv_nsec = (long)((sinceUnix % 10000000ull) * 100ull);
	times[1] = times[0];
	(void)utimensat(AT_FDCWD, p.c_str(), times, 0);
}

bool VDRemoveFile(const wchar_t *path) {
	VDStringA p = narrow_path(path);
	return unlink(p.c_str()) == 0;
}

void VDRemoveFileEx(const wchar_t *path) {
	if (!VDRemoveFile(path)) {
		VDStringA p = narrow_path(path);
		throw VDException("Cannot delete \"%s\": %s", p.c_str(), strerror(errno));
	}
}

void VDMoveFile(const wchar_t *srcPath, const wchar_t *dstPath) {
	VDStringA s = narrow_path(srcPath);
	VDStringA d = narrow_path(dstPath);
	if (rename(s.c_str(), d.c_str()) != 0)
		throw VDException("Cannot rename \"%s\" to \"%s\": %s", s.c_str(), d.c_str(), strerror(errno));
}

uint64 VDFileGetLastWriteTime(const wchar_t *path) {
	VDStringA p = narrow_path(path);
	struct stat st{};
	if (stat(p.c_str(), &st) != 0)
		return 0;

#if defined(__APPLE__)
	struct timespec ts = st.st_mtimespec;
#else
	struct timespec ts = st.st_mtim;
#endif
	return DateFromTimespec(ts).mTicks;
}

VDStringW VDFileGetRootPath(const wchar_t *) {
	// POSIX has a single root "/". There is no per-volume concept.
	return VDStringW(L"/");
}

VDStringW VDGetFullPath(const wchar_t *partialPath) {
	VDStringA p = narrow_path(partialPath);
	char *resolved = realpath(p.c_str(), nullptr);
	if (!resolved) {
		// realpath fails if the path doesn't exist; fall back to prepending cwd.
		if (!p.empty() && p[0] == '/')
			return widen_path(p.c_str());

		char cwdBuf[PATH_MAX];
		if (getcwd(cwdBuf, sizeof cwdBuf)) {
			VDStringA joined(cwdBuf);
			joined += '/';
			joined += p;
			return widen_path(joined.c_str());
		}
		return VDStringW(partialPath);
	}

	VDStringW r = widen_path(resolved);
	free(resolved);
	return r;
}

VDStringW VDGetLongPath(const wchar_t *s) {
	return VDGetFullPath(s);
}

VDStringW VDMakePath(const wchar_t *base, const wchar_t *file) {
	return VDMakePath(VDStringSpanW(base), VDStringSpanW(file));
}

VDStringW VDMakePath(const VDStringSpanW& base, const VDStringSpanW& file) {
	if (base.empty())
		return VDStringW(file);

	VDStringW result(base);
	const wchar_t c = result[result.size() - 1];

	if (c != L'/' && c != L'\\' && c != L':')
		result += L'/';

	result.append(file);
	return result;
}

bool VDFileIsPathEqual(const wchar_t *path1, const wchar_t *path2) {
	for (;; ++path1, ++path2) {
		wchar_t c = *path1;
		wchar_t d = *path2;
		if (c == L'\\') c = L'/';
		if (d == L'\\') d = L'/';
		if (c != d) return false;
		if (!c) return true;
	}
}

void VDFileFixDirPath(VDStringW& path) {
	if (!path.empty()) {
		wchar_t c = path[path.size()-1];
		if (c != L'/' && c != L'\\' && c != L':')
			path += L'/';
	}
}

VDStringW VDGetLocalModulePath() {
	// POSIX has no per-DLL handle; fall back to the executable's directory.
	VDStringW progFile = VDGetProgramFilePath();
	const wchar_t *slash = VDFileSplitPath(progFile.c_str());
	return VDStringW(progFile.data(), slash - progFile.data());
}

VDStringW VDGetProgramPath() {
	VDStringW progFile = VDGetProgramFilePath();
	const wchar_t *slash = VDFileSplitPath(progFile.c_str());
	return VDStringW(progFile.data(), slash - progFile.data());
}

VDStringW VDGetProgramFilePath() {
	char buf[PATH_MAX];
#if defined(__APPLE__)
	uint32_t size = sizeof buf;
	if (_NSGetExecutablePath(buf, &size) != 0)
		throw VDException("Unable to get program path: buffer too small (%u)", size);
	// Resolve symlinks / relative components for a canonical path.
	char real[PATH_MAX];
	if (realpath(buf, real))
		return widen_path(real);
	return widen_path(buf);
#else
	ssize_t len = readlink("/proc/self/exe", buf, sizeof buf - 1);
	if (len < 0)
		throw VDException("Unable to get program path: %s", strerror(errno));
	buf[len] = 0;
	return widen_path(buf);
#endif
}

VDStringW VDGetSystemPath() {
	// POSIX doesn't have a "system directory" in the Win32 sense. Return the
	// standard /usr/bin as a reasonable placeholder for code that logs this.
	return VDStringW(L"/usr/bin");
}

void VDGetRootPaths(vdvector<VDStringW>& paths) {
	paths.push_back() = L"/";
}

VDStringW VDGetRootVolumeLabel(const wchar_t *) {
	return VDStringW();
}

uint32 VDFileGetAttributes(const wchar_t *path) {
	VDStringA p = narrow_path(path);
	struct stat st{};
	if (lstat(p.c_str(), &st) != 0)
		return kVDFileAttr_Invalid;
	return AttrsFromStat(st);
}

void VDFileSetAttributes(const wchar_t *path, uint32 attrsToChange, uint32 newAttrs) {
	if (!(attrsToChange & kVDFileAttr_ReadOnly))
		return;

	VDStringA p = narrow_path(path);
	struct stat st{};
	if (stat(p.c_str(), &st) != 0)
		throw VDException("Cannot stat \"%s\": %s", p.c_str(), strerror(errno));

	mode_t mode = st.st_mode;
	if (newAttrs & kVDFileAttr_ReadOnly)
		mode &= ~(mode_t)(S_IWUSR | S_IWGRP | S_IWOTH);
	else
		mode |= S_IWUSR;

	if (chmod(p.c_str(), mode) != 0)
		throw VDException("Cannot change attributes on \"%s\": %s", p.c_str(), strerror(errno));
}

/////////////////////////////////////////////////////////////////////////////

namespace {
	struct DirIterState {
		DIR *dir = nullptr;
		VDStringW basePath;     // absolute directory to search in
		VDStringW pattern;      // wildcard pattern (or L"*")
	};
}

VDDirectoryIterator::VDDirectoryIterator(const wchar_t *path)
	: mSearchPath(path)
	, mpHandle(nullptr)
	, mbSearchComplete(false)
{
	// Search paths in the original API look like "/foo/bar/*.dat". Split off
	// the directory and the wildcard.
	mBasePath = VDFileSplitPathLeft(mSearchPath);
	VDFileFixDirPath(mBasePath);

	auto *st = new DirIterState;
	st->basePath = mBasePath;
	st->pattern  = VDFileSplitPathRight(mSearchPath);
	if (st->pattern.empty())
		st->pattern = L"*";
	mpHandle = st;
}

VDDirectoryIterator::~VDDirectoryIterator() {
	auto *st = (DirIterState *)mpHandle;
	if (st) {
		if (st->dir)
			closedir(st->dir);
		delete st;
	}
}

bool VDDirectoryIterator::Next() {
	if (mbSearchComplete)
		return false;

	auto *st = (DirIterState *)mpHandle;
	if (!st->dir) {
		VDStringA dirA = VDTextWToU8(st->basePath.c_str(), -1);
		if (dirA.empty()) dirA = ".";
		// strip trailing slash except for root
		if (dirA.size() > 1 && (dirA.back() == '/' || dirA.back() == '\\'))
			dirA.pop_back();

		st->dir = opendir(dirA.c_str());
		if (!st->dir) {
			mbSearchComplete = true;
			return false;
		}
	}

	for (;;) {
		errno = 0;
		struct dirent *ent = readdir(st->dir);
		if (!ent) {
			mbSearchComplete = true;
			return false;
		}

		if (ent->d_name[0] == '.' && (!ent->d_name[1] ||
				(ent->d_name[1] == '.' && !ent->d_name[2])))
			continue;

		VDStringW name = VDTextU8ToW(ent->d_name, -1);
		if (!VDFileWildMatch(st->pattern.c_str(), name.c_str()))
			continue;

		mFilename = name;

		VDStringA fullA = VDTextWToU8(st->basePath.c_str(), -1) + ent->d_name;
		struct stat st2{};
		if (lstat(fullA.c_str(), &st2) != 0) {
			mFileSize = 0;
			mbDirectory = false;
			mAttributes = kVDFileAttr_Invalid;
		} else {
			mFileSize = (sint64)st2.st_size;
			mbDirectory = S_ISDIR(st2.st_mode);
			mAttributes = AttrsFromStat(st2);

		#if defined(__APPLE__)
			mLastWriteDate = DateFromTimespec(st2.st_mtimespec);
			mCreationDate  = DateFromTimespec(st2.st_ctimespec);
		#else
			mLastWriteDate = DateFromTimespec(st2.st_mtim);
			mCreationDate  = DateFromTimespec(st2.st_ctim);
		#endif
		}
		return true;
	}
}

bool VDDirectoryIterator::ResolveLinkSize() {
	if (IsDirectory() || !IsLink())
		return true;

	VDStringA fullA = VDTextWToU8(GetFullPath().c_str(), -1);
	struct stat st{};
	if (stat(fullA.c_str(), &st) != 0)
		return false;

	mFileSize = (sint64)st.st_size;
	return true;
}
