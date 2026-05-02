//	Altirra - Qt port
//	SAP type R writer Qt wrapper.

#include "sapwriter_qt.h"

#include <stdexcept>
#include <string>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name. Undef before including VDCPP.
#undef signals
#undef slots

#include <vd2/system/error.h>
#include <vd2/system/VDString.h>

#include <constants.h>
#include <irqcontroller.h>
#include <sapwriter.h>
#include <simulator.h>

namespace {

// Convert UTF-8 path to a wide string for the upstream API. SAP paths
// are usually ASCII; non-ASCII is rare but we still want to round-trip
// cleanly on Linux (where wchar_t is 32-bit).
std::wstring utf8ToWide(const char *s) {
	if (!s) return {};
	std::wstring out;
	out.reserve(std::char_traits<char>::length(s));
	const auto *p = (const unsigned char *)s;
	while (*p) {
		uint32_t cp;
		if (*p < 0x80) { cp = *p++; }
		else if ((*p & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
			cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); p += 2;
		} else if ((*p & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80
		           && (p[2] & 0xC0) == 0x80) {
			cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
			p += 3;
		} else if ((*p & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80
		           && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
			cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12)
			   | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
			p += 4;
		} else { ++p; continue; }
		out.push_back((wchar_t)cp);
	}
	return out;
}

} // namespace

ATQtSapRecorder::ATQtSapRecorder(ATSimulator& sim, const char *path) {
	const std::wstring wpath = utf8ToWide(path);
	mpWriter = ATCreateSAPWriter();
	if (!mpWriter)
		throw std::runtime_error("Could not create SAP writer.");

	const ATVideoStandard vs = sim.GetVideoStandard();
	const bool pal = (vs != kATVideoStandard_NTSC && vs != kATVideoStandard_PAL60);

	try {
		mpWriter->Init(sim.GetEventManager(), &sim.GetPokey(),
		               sim.GetUIRenderer(), wpath.c_str(), pal);
	} catch (const MyError& e) {
		delete mpWriter;
		mpWriter = nullptr;
		throw std::runtime_error(std::string(e.c_str()));
	}
}

ATQtSapRecorder::~ATQtSapRecorder() {
	if (mpWriter) {
		mpWriter->Shutdown();
		delete mpWriter;
		mpWriter = nullptr;
	}
}
