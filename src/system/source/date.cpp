//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2011 Avery Lee, All Rights Reserved.
//
//	Cross-platform Qt port: the original used Win32 FILETIME/SYSTEMTIME and
//	GetSystemTimeAsFileTime + TzSpecificLocalTimeToSystemTime +
//	GetDateFormatW/GetTimeFormatW. VDDate::mTicks is defined as 100-ns ticks
//	since 1601-01-01 UTC (the Windows epoch). We keep that canonical form and
//	convert to/from POSIX time_t when talking to the OS.

#include <stdafx.h>
#include <vd2/system/date.h>
#include <vd2/system/VDString.h>

#include <sys/time.h>
#include <time.h>
#include <wchar.h>

static constexpr uint64 kWin32EpochToUnixTicks = 116444736000000000ull;

static_assert(+VDDateInterval{0} == VDDateInterval{0});
static_assert(+VDDateInterval{1} == VDDateInterval{1});
static_assert(-VDDateInterval{0} == VDDateInterval{0});
static_assert(-VDDateInterval{1} == VDDateInterval{-1});

static_assert(VDDateInterval{0}.Abs().mDeltaTicks == 0);
static_assert(VDDateInterval{1}.Abs().mDeltaTicks == 1);
static_assert(VDDateInterval{-1}.Abs().mDeltaTicks == 1);

static_assert(VDDateInterval{ 1} != VDDateInterval{0});
static_assert(VDDateInterval{ 0} == VDDateInterval{0});
static_assert(VDDateInterval{ 0} >= VDDateInterval{0});
static_assert(VDDateInterval{ 1} >= VDDateInterval{0});
static_assert(VDDateInterval{ 1} >  VDDateInterval{0});
static_assert(VDDateInterval{ 0} <= VDDateInterval{0});
static_assert(VDDateInterval{-1} <= VDDateInterval{0});
static_assert(VDDateInterval{-1} <  VDDateInterval{0});

static_assert(!(VDDateInterval{ 0} != VDDateInterval{0}));
static_assert(!(VDDateInterval{ 1} == VDDateInterval{0}));
static_assert(!(VDDateInterval{-1} >= VDDateInterval{0}));
static_assert(!(VDDateInterval{ 0} >  VDDateInterval{0}));
static_assert(!(VDDateInterval{ 1} <= VDDateInterval{0}));
static_assert(!(VDDateInterval{ 0} <  VDDateInterval{0}));

static_assert(VDDateInterval{0}.ToSeconds() == 0.0f);
static_assert(VDDateInterval{10000000}.ToSeconds() == 1.0f);
static_assert(VDDateInterval{-10000000}.ToSeconds() == -1.0f);
static_assert(VDDateInterval::FromSeconds(0).mDeltaTicks == 0);
static_assert(VDDateInterval::FromSeconds(1.0f).mDeltaTicks == 10000000);
static_assert(VDDateInterval::FromSeconds(-1.0f).mDeltaTicks == -10000000);

static_assert(VDDate{0} - VDDate{0} == VDDateInterval{0});
static_assert(VDDate{0} - VDDate{1} == VDDateInterval{-1});
static_assert(VDDate{1} - VDDate{0} == VDDateInterval{1});
static_assert(VDDate{1000} + VDDateInterval{1} == VDDate{1001});
static_assert(VDDateInterval{1} + VDDate{1000} == VDDate{1001});
static_assert(VDDate{1000} - VDDateInterval{1} == VDDate{999});

VDDate VDGetCurrentDate() {
	timespec ts{};
	clock_gettime(CLOCK_REALTIME, &ts);

	VDDate r;
	r.mTicks = kWin32EpochToUnixTicks
		+ (uint64)ts.tv_sec * 10000000ull
		+ (uint64)ts.tv_nsec / 100ull;

	return r;
}

sint64 VDGetDateAsTimeT(const VDDate& date) {
	return ((sint64)date.mTicks - (sint64)kWin32EpochToUnixTicks) / 10000000;
}

VDExpandedDate VDGetLocalDate(const VDDate& date) {
	VDExpandedDate r = {0};

	const time_t secs = (time_t)VDGetDateAsTimeT(date);
	struct tm lt{};
#if defined(_WIN32)
	if (localtime_s(&lt, &secs) != 0)
		return r;
#else
	if (!localtime_r(&secs, &lt))
		return r;
#endif

	r.mYear         = (sint16)(lt.tm_year + 1900);
	r.mMonth        = (uint8)(lt.tm_mon + 1);
	r.mDayOfWeek    = (uint8)lt.tm_wday;
	r.mDay          = (uint8)lt.tm_mday;
	r.mHour         = (uint8)lt.tm_hour;
	r.mMinute       = (uint8)lt.tm_min;
	r.mSecond       = (uint8)lt.tm_sec;

	// Pick up sub-second resolution from the original tick count.
	const uint64 sinceUnix = date.mTicks - kWin32EpochToUnixTicks;
	r.mMilliseconds = (uint16)((sinceUnix / 10000ull) % 1000ull);

	return r;
}

VDDate VDDateFromLocalDate(const VDExpandedDate& date) {
	struct tm lt{};
	lt.tm_year  = date.mYear - 1900;
	lt.tm_mon   = date.mMonth - 1;
	lt.tm_mday  = date.mDay;
	lt.tm_hour  = date.mHour;
	lt.tm_min   = date.mMinute;
	lt.tm_sec   = date.mSecond;
	lt.tm_isdst = -1;

	const time_t secs = mktime(&lt);
	if (secs == (time_t)-1)
		return VDDate{};

	VDDate r;
	r.mTicks = kWin32EpochToUnixTicks
		+ (uint64)secs * 10000000ull
		+ (uint64)date.mMilliseconds * 10000ull;
	return r;
}

namespace {
	void AppendStrftime(VDStringW& dst, const struct tm& lt, const char *format) {
		char buf[128];
		if (strftime(buf, sizeof buf, format, &lt)) {
			for (const char *p = buf; *p; ++p)
				dst.push_back((wchar_t)(unsigned char)*p);
		}
	}

	struct tm ToLocalTm(const VDExpandedDate& ed) {
		struct tm lt{};
		lt.tm_year  = ed.mYear - 1900;
		lt.tm_mon   = ed.mMonth - 1;
		lt.tm_mday  = ed.mDay;
		lt.tm_hour  = ed.mHour;
		lt.tm_min   = ed.mMinute;
		lt.tm_sec   = ed.mSecond;
		lt.tm_wday  = ed.mDayOfWeek;
		lt.tm_isdst = -1;
		return lt;
	}
}

void VDAppendLocalDateString(VDStringW& dst, const VDExpandedDate& ed) {
	struct tm lt = ToLocalTm(ed);
	AppendStrftime(dst, lt, "%x");
}

void VDAppendLocalTimeString(VDStringW& dst, const VDExpandedDate& ed) {
	struct tm lt = ToLocalTm(ed);
	AppendStrftime(dst, lt, "%R");
}
