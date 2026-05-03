//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2004 Avery Lee, All Rights Reserved.
//
//	Cross-platform Qt port: VDFile originally wrapped CreateFileW and used a
//	Win32 HANDLE. This rewrite keeps the public surface but stores an int
//	file descriptor and uses open/read/write/lseek/close/fstat. Unbuffered
//	I/O flags become no-ops (POSIX lets the kernel decide; O_DIRECT has too
//	many sharp edges to set by default).

#include <stdafx.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(_WIN32)
#include <io.h>
#include <sys/utime.h>
#else
#include <utime.h>
#endif
#include <wchar.h>

#include <vd2/system/error.h>
#include <vd2/system/date.h>
#include <vd2/system/filesys.h>
#include <vd2/system/VDString.h>
#include <vd2/system/text.h>
#include <vd2/system/file.h>

using namespace nsVDFile;

static constexpr uint64 kWin32EpochToUnixTicks_File = 116444736000000000ull;

namespace {
	VDStringA to_utf8(const wchar_t *s) {
		return s ? VDTextWToU8(s, -1) : VDStringA();
	}

	VDStringA to_utf8(const char *s) {
		return s ? VDStringA(s) : VDStringA();
	}

	VDDate DateFromTimespec(const struct timespec& ts) {
		VDDate d;
		d.mTicks = kWin32EpochToUnixTicks_File
			+ (uint64)ts.tv_sec * 10000000ull
			+ (uint64)ts.tv_nsec / 100ull;
		return d;
	}

	void TimespecFromDate(const VDDate& date, struct timespec& ts) {
		const uint64 sinceUnix = date.mTicks - kWin32EpochToUnixTicks_File;
		ts.tv_sec  = (time_t)(sinceUnix / 10000000ull);
		ts.tv_nsec = (long)((sinceUnix % 10000000ull) * 100ull);
	}
}

///////////////////////////////////////////////////////////////////////////////

VDFile::VDFile(const char *pszFileName, uint32 flags)
	: mhFile(kVDFileHandleInvalid)
{
	open(pszFileName, flags);
}

VDFile::VDFile(const wchar_t *pwszFileName, uint32 flags)
	: mhFile(kVDFileHandleInvalid)
{
	open(pwszFileName, flags);
}

VDFile::VDFile(VDFileHandle h)
	: mhFile(h)
{
	if (h != kVDFileHandleInvalid) {
		off_t pos = lseek(h, 0, SEEK_CUR);
		mFilePosition = (pos < 0) ? 0 : (sint64)pos;
	}
}

vdnothrow VDFile::VDFile(VDFile&& other) vdnoexcept
	: mhFile(other.mhFile)
	, mpFilename(std::move(other.mpFilename))
	, mFilePosition(other.mFilePosition)
{
	other.mhFile = kVDFileHandleInvalid;
}

VDFile::~VDFile() {
	closeNT();
}

vdnothrow VDFile& VDFile::operator=(VDFile&& other) vdnoexcept {
	std::swap(mhFile, other.mhFile);
	std::swap(mpFilename, other.mpFilename);
	std::swap(mFilePosition, other.mFilePosition);
	return *this;
}

void VDFile::open(const char *pszFilename, uint32 flags) {
	uint32 err = open_internal(pszFilename, nullptr, flags);
	if (err)
		throw VDException("Cannot open file \"%s\": %s", pszFilename ? pszFilename : "", strerror(err));
}

void VDFile::open(const wchar_t *pwszFilename, uint32 flags) {
	uint32 err = open_internal(nullptr, pwszFilename, flags);
	if (err) {
		VDStringA utf8 = to_utf8(pwszFilename);
		throw VDException("Cannot open file \"%s\": %s", utf8.c_str(), strerror(err));
	}
}

bool VDFile::openNT(const wchar_t *pwszFilename, uint32 flags) {
	return open_internal(nullptr, pwszFilename, flags) == 0;
}

bool VDFile::tryOpen(const wchar_t *pwszFilename, uint32 flags) {
	uint32 err = open_internal(nullptr, pwszFilename, flags);
	if (err == ENOENT)
		return false;
	if (err) {
		VDStringA utf8 = to_utf8(pwszFilename);
		throw VDException("Cannot open file \"%s\": %s", utf8.c_str(), strerror(err));
	}
	return true;
}

bool VDFile::openAlways(const wchar_t *pwszFilename, uint32 flags) {
	// Matches the Win32 behaviour: returns true when the file previously
	// existed (EEXIST when combined with O_CREAT|O_EXCL), false when newly
	// created. We just defer to open_internal and approximate via stat.
	struct stat existed{};
	bool wasPresent = (stat(to_utf8(pwszFilename).c_str(), &existed) == 0);

	uint32 err = open_internal(nullptr, pwszFilename, flags);
	if (err) {
		VDStringA utf8 = to_utf8(pwszFilename);
		throw VDException("Cannot open file \"%s\": %s", utf8.c_str(), strerror(err));
	}
	return wasPresent;
}

uint32 VDFile::open_internal(const char *pszFilename, const wchar_t *pwszFilename, uint32 flags) {
	close();

	// At least one of read/write must be set.
	VDASSERT(flags & (kRead | kWrite));
	// Exactly one creation mode must be set.
	VDASSERT(flags & kCreationMask);

	VDStringA pathUtf8 = pszFilename ? to_utf8(pszFilename) : to_utf8(pwszFilename);

	// Retain just the leaf filename for error messages.
	{
		VDStringW fullW = pszFilename ? VDTextU8ToW(pszFilename, -1) : VDStringW(pwszFilename);
		const wchar_t *leaf = VDFileSplitPath(fullW.c_str());
		const size_t n = wcslen(leaf);
		wchar_t *dst = (wchar_t *)malloc(sizeof(wchar_t) * (n + 1));
		if (!dst) return ENOMEM;
		memcpy(dst, leaf, sizeof(wchar_t) * n);
		dst[n] = 0;
		mpFilename = dst;
	}

	int openFlags = 0;
	switch (flags & (kRead | kWrite)) {
	case kRead:         openFlags = O_RDONLY; break;
	case kWrite:        openFlags = O_WRONLY; break;
	case kRead|kWrite:  openFlags = O_RDWR;   break;
	}

	const uint32 creation = flags & kCreationMask;
	switch (creation) {
	case kOpenExisting:     /* default */                           break;
	case kOpenAlways:       openFlags |= O_CREAT;                   break;
	case kCreateAlways:     openFlags |= O_CREAT | O_TRUNC;         break;
	case kCreateNew:        openFlags |= O_CREAT | O_EXCL;          break;
	case kTruncateExisting: openFlags |= O_TRUNC;                   break;
	default:
		return EINVAL;
	}

	// Deny flags / unbuffered / write-through are all Win32-specific
	// optimizations without a portable POSIX analogue; ignore them.

	int fd = ::open(pathUtf8.c_str(), openFlags, 0666);
	if (fd < 0)
		return errno;

	mhFile = fd;
	mFilePosition = 0;
	return 0;
}

bool VDFile::closeNT() {
	if (mhFile != kVDFileHandleInvalid) {
		int fd = mhFile;
		mhFile = kVDFileHandleInvalid;
		if (::close(fd) != 0)
			return false;
	}
	return true;
}

void VDFile::close() {
	if (!closeNT())
		throw VDException("Cannot close file: %s", strerror(errno));
}

bool VDFile::truncateNT() {
	return ftruncate(mhFile, (off_t)mFilePosition) == 0;
}

void VDFile::truncate() {
	if (!truncateNT())
		throw VDException("Cannot truncate file: %s", strerror(errno));
}

bool VDFile::extendValidNT(sint64 pos) {
	// POSIX doesn't have SetFileValidData. Pre-allocate via the
	// platform's closest equivalent and truncate to size.
#if defined(__linux__)
	return posix_fallocate(mhFile, 0, (off_t)pos) == 0;
#elif defined(__APPLE__)
	fstore_t store{F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, (off_t)pos, 0};
	if (fcntl(mhFile, F_PREALLOCATE, &store) == -1) {
		store.fst_flags = F_ALLOCATEALL;
		if (fcntl(mhFile, F_PREALLOCATE, &store) == -1)
			return false;
	}
	return ftruncate(mhFile, (off_t)pos) == 0;
#else
	return ftruncate(mhFile, (off_t)pos) == 0;
#endif
}

void VDFile::extendValid(sint64 pos) {
	if (!extendValidNT(pos))
		throw VDException("Cannot extend file: %s", strerror(errno));
}

bool VDFile::enableExtendValid() {
	// No process-wide privilege needed on POSIX.
	return true;
}

long VDFile::readData(void *buffer, long length) {
	ssize_t got = ::read(mhFile, buffer, (size_t)length);
	if (got < 0)
		throw VDException("Cannot read from file: %s", strerror(errno));

	mFilePosition += got;
	return (long)got;
}

void VDFile::read(void *buffer, long length) {
	long got = readData(buffer, length);
	if (got != length)
		throw VDException("Cannot read from file: premature end of file");
}

long VDFile::writeData(const void *buffer, long length) {
	ssize_t wrote = ::write(mhFile, buffer, (size_t)length);
	if (wrote < 0 || wrote != (ssize_t)length)
		throw VDException("Cannot write to file: %s", strerror(errno));

	mFilePosition += wrote;
	return (long)wrote;
}

void VDFile::write(const void *buffer, long length) {
	if (length != writeData(buffer, length))
		throw VDException("Cannot write to file: unable to write all data");
}

bool VDFile::seekNT(sint64 newPos, eSeekMode mode) {
	int whence;
	switch (mode) {
	case kSeekStart: whence = SEEK_SET; break;
	case kSeekCur:   whence = SEEK_CUR; break;
	case kSeekEnd:   whence = SEEK_END; break;
	default: return false;
	}

	off_t rc = lseek(mhFile, (off_t)newPos, whence);
	if (rc < 0)
		return false;

	mFilePosition = (sint64)rc;
	return true;
}

void VDFile::seek(sint64 newPos, eSeekMode mode) {
	if (!seekNT(newPos, mode))
		throw VDException("Cannot seek within file: %s", strerror(errno));
}

bool VDFile::skipNT(sint64 delta) {
	if (!delta) return true;

	char buf[1024];
	if (delta > 0 && delta <= (sint64)sizeof buf)
		return (long)delta == readData(buf, (long)delta);
	return seekNT(delta, kSeekCur);
}

void VDFile::skip(sint64 delta) {
	if (!delta) return;

	char buf[1024];
	if (delta > 0 && delta <= (sint64)sizeof buf) {
		if ((long)delta != readData(buf, (long)delta))
			throw VDException("Cannot seek within file");
	} else {
		seek(delta, kSeekCur);
	}
}

sint64 VDFile::size() const {
	struct stat st{};
	if (fstat(mhFile, &st) != 0)
		throw VDException("Cannot retrieve size of file: %s", strerror(errno));
	return (sint64)st.st_size;
}

sint64 VDFile::tell() const {
	return mFilePosition;
}

bool VDFile::flushNT() {
#if defined(_WIN32)
	return _commit(mhFile) == 0;
#else
	return fsync(mhFile) == 0;
#endif
}

void VDFile::flush() {
	if (!flushNT())
		throw VDException("Cannot flush file: %s", strerror(errno));
}

bool VDFile::isOpen() const {
	return mhFile != kVDFileHandleInvalid;
}

VDFileHandle VDFile::getRawHandle() const {
	return mhFile;
}

uint32 VDFile::getAttributes() const {
	if (mhFile == kVDFileHandleInvalid)
		return 0;

	struct stat st{};
	if (fstat(mhFile, &st) != 0)
		return 0;

	uint32 attrs = 0;
	if (S_ISDIR(st.st_mode))  attrs |= kVDFileAttr_Directory;
#ifdef S_ISLNK
	if (S_ISLNK(st.st_mode))  attrs |= kVDFileAttr_Link;
#endif
	if (!(st.st_mode & S_IWUSR)) attrs |= kVDFileAttr_ReadOnly;
	return attrs;
}

VDDate VDFile::getCreationTime() const {
	if (mhFile == kVDFileHandleInvalid)
		return VDDate{};

	struct stat st{};
	if (fstat(mhFile, &st) != 0)
		return VDDate{};

#if defined(__APPLE__)
	return DateFromTimespec(st.st_birthtimespec);
#elif defined(_WIN32)
	return DateFromTimespec(timespec{(time_t)st.st_ctime, 0});
#else
	// ext4/xfs don't expose birth time through fstat(); fall back to mtime.
	return DateFromTimespec(st.st_ctim);
#endif
}

void VDFile::setCreationTime(const VDDate&) {
	// POSIX has no way to modify the inode birth time portably; no-op.
}

VDDate VDFile::getLastWriteTime() const {
	if (mhFile == kVDFileHandleInvalid)
		return VDDate{};

	struct stat st{};
	if (fstat(mhFile, &st) != 0)
		return VDDate{};

#if defined(__APPLE__)
	return DateFromTimespec(st.st_mtimespec);
#elif defined(_WIN32)
	return DateFromTimespec(timespec{(time_t)st.st_mtime, 0});
#else
	return DateFromTimespec(st.st_mtim);
#endif
}

void VDFile::setLastWriteTime(const VDDate& date) {
	if (mhFile == kVDFileHandleInvalid)
		return;

	struct timespec times[2];
	TimespecFromDate(date, times[0]);
	times[1] = times[0];
#if defined(_WIN32)
	struct _utimbuf buf{times[0].tv_sec, times[1].tv_sec};
	(void)_futime(mhFile, &buf);
#else
	(void)futimens(mhFile, times);
#endif
}

void *VDFile::AllocUnbuffer(size_t nBytes) {
	const size_t alignment = 4096;
#if defined(_WIN32)
	return _aligned_malloc(nBytes, alignment);
#else
	void *p = nullptr;
	if (posix_memalign(&p, alignment, nBytes) != 0)
		return nullptr;
	return p;
#endif
}

void VDFile::FreeUnbuffer(void *p) {
#if defined(_WIN32)
	_aligned_free(p);
#else
	free(p);
#endif
}
