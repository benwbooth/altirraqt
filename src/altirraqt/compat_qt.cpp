//	altirraqt — Qt compat-database engine.

#include "compat_qt.h"

#include <QFile>
#include <QSettings>
#include <QString>

// Qt's #define signals public collides with VDSignalBase::waitMultiple's
// `signals` parameter; same for `slots`. Drop the macros before pulling in
// upstream headers.
#undef signals
#undef slots

#include <vector>

#include <vd2/system/vdstl.h>
#include <vd2/system/refcount.h>

#include <at/atcore/checksum.h>
#include <at/atio/blobimage.h>
#include <at/atio/cartridgeimage.h>
#include <at/atio/cassetteimage.h>
#include <at/atio/diskimage.h>
#include <at/atio/image.h>

#include <compatdb.h>
#include <cartridge.h>
#include <cassette.h>
#include <cpu.h>
#include <cpumemory.h>
#include <disk.h>
#include <diskinterface.h>
#include <firmwaremanager.h>
#include <simulator.h>

namespace {

// Lifetime-managed database bytes + view. The view is a non-owning pointer
// into mDbBytes; both must move together.
struct GlobalCompatState {
	std::vector<uint8> mDbBytes;
	ATCompatDBView     mView;
	QString            mPath;
};

GlobalCompatState& g() {
	static GlobalCompatState s;
	return s;
}

bool gAutoApply = false;
bool gAutoApplyLoaded = false;

void ensureAutoApplyLoaded() {
	if (gAutoApplyLoaded) return;
	QSettings s;
	gAutoApply = s.value(QStringLiteral("compat/autoApply"), true).toBool();
	gAutoApplyLoaded = true;
}

QString sha256Hex(const ATChecksumSHA256& d) {
	QString out;
	out.reserve(64);
	static const char *hex = "0123456789abcdef";
	for (int i = 0; i < 32; ++i) {
		out.append(QChar::fromLatin1(hex[(d.mDigest[i] >> 4) & 0xF]));
		out.append(QChar::fromLatin1(hex[d.mDigest[i] & 0xF]));
	}
	return out;
}

std::optional<ATChecksumSHA256> imageSha256(IATImage *image) {
	if (!image) return std::nullopt;
	return image->GetImageFileSHA256();
}

// Local marker mirroring upstream's ATCompatMarker (compatengine.cpp owns
// the upstream definition; we don't compile that file, so define a shim
// here that's layout-compatible enough to drive ATCompatDBView lookups).
struct ATCompatMarker_local {
	ATCompatRuleType mRuleType;
	uint64           mValue[4];

	static ATCompatMarker_local fromSha256(ATCompatRuleType t, const ATChecksumSHA256& d) {
		ATCompatMarker_local m{};
		m.mRuleType = t;
		memcpy(m.mValue, d.mDigest, 32);
		return m;
	}
};

void collectMarkers(IATImage *image, std::vector<ATCompatMarker_local>& out) {
	if (!image) return;
	out.clear();

	if (auto *disk = vdpoly_cast<IATDiskImage *>(image); disk && !disk->IsDynamic()) {
		if (auto sha = disk->GetImageFileSHA256(); sha.has_value())
			out.push_back(ATCompatMarker_local::fromSha256(kATCompatRuleType_DiskFileSHA256, *sha));
		ATCompatMarker_local m{};
		m.mRuleType = kATCompatRuleType_DiskChecksum;
		m.mValue[0] = disk->GetImageChecksum();
		out.push_back(m);
	}

	if (auto *cart = vdpoly_cast<IATCartridgeImage *>(image)) {
		if (auto sha = cart->GetImageFileSHA256(); sha.has_value())
			out.push_back(ATCompatMarker_local::fromSha256(kATCompatRuleType_CartFileSHA256, *sha));
		ATCompatMarker_local m{};
		m.mRuleType = kATCompatRuleType_CartChecksum;
		m.mValue[0] = cart->GetChecksum();
		out.push_back(m);
	}

	if (auto *tape = vdpoly_cast<IATCassetteImage *>(image)) {
		if (auto sha = tape->GetImageFileSHA256(); sha.has_value())
			out.push_back(ATCompatMarker_local::fromSha256(kATCompatRuleType_TapeFileSHA256, *sha));
	}

	if (auto *blob = vdpoly_cast<IATBlobImage *>(image);
		blob && blob->GetImageType() == kATImageType_Program)
	{
		if (auto sha = blob->GetImageFileSHA256(); sha.has_value())
			out.push_back(ATCompatMarker_local::fromSha256(kATCompatRuleType_ExeFileSHA256, *sha));
		ATCompatMarker_local m{};
		m.mRuleType = kATCompatRuleType_ExeChecksum;
		m.mValue[0] = blob->GetChecksum();
		out.push_back(m);
	}
}

const ATCompatDBTitle *findMatchingTitle(const std::vector<ATCompatMarker_local>& markers,
                                         std::vector<ATCompatKnownTag>& outTags)
{
	if (!g().mView.IsValid()) return nullptr;
	if (markers.empty())     return nullptr;

	vdfastvector<const ATCompatDBRule *> matchingRules;
	matchingRules.reserve(markers.size());

	for (const auto& m : markers) {
		uint64 ruleValue;
		if (ATCompatIsLargeRuleType(m.mRuleType)) {
			ruleValue = g().mView.FindLargeRuleBlob(m.mValue);
			if (!ruleValue) continue;
		} else {
			ruleValue = m.mValue[0];
		}
		const auto rng = g().mView.FindMatchingRules(m.mRuleType, ruleValue);
		for (auto it = rng.first; it != rng.second; ++it)
			matchingRules.push_back(it);
	}

	if (matchingRules.empty()) return nullptr;

	vdfastvector<const ATCompatDBTitle *> titles;
	g().mView.FindMatchingTitles(titles, matchingRules.data(), matchingRules.size());

	for (const ATCompatDBTitle *title : titles) {
		outTags.clear();
		for (const auto& tagId : title->mTagIds) {
			ATCompatKnownTag t = g().mView.GetKnownTag(tagId);
			if (t != kATCompatKnownTag_None)
				outTags.push_back(t);
		}
		if (!outTags.empty())
			return title;
	}
	return nullptr;
}

bool hasInternalBASIC(ATHardwareMode mode) {
	switch (mode) {
		case kATHardwareMode_800XL:
		case kATHardwareMode_1200XL:
		case kATHardwareMode_XEGS:
		case kATHardwareMode_130XE:
		case kATHardwareMode_1400XL:
			return true;
		default:
			return false;
	}
}

void switchKernelTo(ATSimulator& sim, ATSpecificFirmwareType ftype) {
	auto *fw = sim.GetFirmwareManager();
	if (!fw) return;
	const uint64 id = fw->GetSpecificFirmware(ftype);
	if (!id) return;

	switch (ftype) {
		case kATSpecificFirmwareType_OSA:
		case kATSpecificFirmwareType_OSB:
			sim.SetHardwareMode(kATHardwareMode_800);
			break;
		case kATSpecificFirmwareType_XLOSr2:
		case kATSpecificFirmwareType_XLOSr4:
			if (sim.GetHardwareMode() != kATHardwareMode_800XL
				&& sim.GetHardwareMode() != kATHardwareMode_130XE
				&& sim.GetHardwareMode() != kATHardwareMode_1400XL)
				sim.SetHardwareMode(kATHardwareMode_800XL);
			break;
		default:
			break;
	}
	sim.SetKernel(id);
	sim.LoadROMs();
}

void switchBasicTo(ATSimulator& sim, ATSpecificFirmwareType ftype) {
	auto *fw = sim.GetFirmwareManager();
	if (!fw) return;
	const uint64 id = fw->GetSpecificFirmware(ftype);
	if (!id) return;
	sim.SetBasic(id);
	sim.LoadROMs();
}

void applyTags(ATSimulator& sim, const std::vector<ATCompatKnownTag>& tags) {
	bool basic = false, nobasic = false;

	// First pass: kernel / hardware-mode tags.
	for (auto t : tags) {
		switch (t) {
			case kATCompatKnownTag_OSA:  switchKernelTo(sim, kATSpecificFirmwareType_OSA);  break;
			case kATCompatKnownTag_OSB:  switchKernelTo(sim, kATSpecificFirmwareType_OSB);  break;
			case kATCompatKnownTag_XLOS: switchKernelTo(sim, kATSpecificFirmwareType_XLOSr2); break;
			case kATCompatKnownTag_NoFloatingDataBus:
				switch (sim.GetHardwareMode()) {
					case kATHardwareMode_130XE:
					case kATHardwareMode_XEGS:
						sim.SetHardwareMode(kATHardwareMode_800XL);
						sim.LoadROMs();
						break;
					default: break;
				}
				break;
			default: break;
		}
	}

	// Second pass: feature tags.
	for (auto t : tags) {
		switch (t) {
			case kATCompatKnownTag_BASIC:     basic = true; break;
			case kATCompatKnownTag_BASICRevA: switchBasicTo(sim, kATSpecificFirmwareType_BASICRevA); basic = true; break;
			case kATCompatKnownTag_BASICRevB: switchBasicTo(sim, kATSpecificFirmwareType_BASICRevB); basic = true; break;
			case kATCompatKnownTag_BASICRevC: switchBasicTo(sim, kATSpecificFirmwareType_BASICRevC); basic = true; break;
			case kATCompatKnownTag_NoBASIC:   nobasic = true; break;

			case kATCompatKnownTag_AccurateDiskTiming:
				sim.SetDiskAccurateTimingEnabled(true);
				sim.SetDiskSIOPatchEnabled(false);
				break;
			case kATCompatKnownTag_NoU1MB:
				sim.SetUltimate1MBEnabled(false);
				break;
			case kATCompatKnownTag_Undocumented6502: {
				auto& cpu = sim.GetCPU();
				sim.SetCPUMode(kATCPUMode_6502, 1);
				cpu.SetIllegalInsnsEnabled(true);
				break;
			}
			case kATCompatKnownTag_No65C816HighAddressing:
				sim.SetHighMemoryBanks(-1);
				break;
			case kATCompatKnownTag_WritableDisk: {
				auto& di = sim.GetDiskInterface(0);
				if (!(di.GetWriteMode() & kATMediaWriteMode_AllowWrite))
					di.SetWriteMode(kATMediaWriteMode_VRWSafe);
				break;
			}
			case kATCompatKnownTag_60Hz:
				if (sim.IsVideo50Hz())
					sim.SetVideoStandard(kATVideoStandard_NTSC);
				break;
			case kATCompatKnownTag_50Hz:
				if (!sim.IsVideo50Hz())
					sim.SetVideoStandard(kATVideoStandard_PAL);
				break;
			default: break;
		}
	}

	if (basic || nobasic) {
		const bool internalBasic = hasInternalBASIC(sim.GetHardwareMode());
		if (basic) {
			sim.SetBASICEnabled(true);
			if (!internalBasic)
				sim.LoadCartridgeBASIC();
		} else if (nobasic) {
			if (internalBasic)
				sim.SetBASICEnabled(false);
		}
	}
}

} // namespace

bool ATCompatLoadDatabase(const QString& path) {
	if (path.isEmpty()) {
		g().mDbBytes.clear();
		g().mView = ATCompatDBView();
		g().mPath.clear();
		QSettings().setValue(QStringLiteral("compat/dbPath"), QString());
		return true;
	}

	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		return false;

	const QByteArray bytes = f.readAll();
	if (bytes.size() < (qsizetype)sizeof(ATCompatDBHeader))
		return false;

	std::vector<uint8> buf(bytes.size());
	memcpy(buf.data(), bytes.constData(), bytes.size());

	const auto *hdr = reinterpret_cast<const ATCompatDBHeader *>(buf.data());
	if (memcmp(hdr->mSignature, ATCompatDBHeader::kSignature, 16) != 0)
		return false;
	if (!hdr->Validate(buf.size()))
		return false;

	g().mDbBytes = std::move(buf);
	g().mView = ATCompatDBView(reinterpret_cast<const ATCompatDBHeader *>(g().mDbBytes.data()));
	g().mPath = path;

	QSettings().setValue(QStringLiteral("compat/dbPath"), path);
	return true;
}

const ATCompatDBView& ATCompatGetView() { return g().mView; }
QString ATCompatGetDatabasePath()       { return g().mPath; }

const ATCompatDBHeader *ATCompatGetHeader() {
	if (g().mDbBytes.empty()) return nullptr;
	return reinterpret_cast<const ATCompatDBHeader *>(g().mDbBytes.data());
}

bool ATCompatIsAutoApplyEnabled() {
	ensureAutoApplyLoaded();
	return gAutoApply;
}
void ATCompatSetAutoApplyEnabled(bool on) {
	ensureAutoApplyLoaded();
	gAutoApply = on;
	QSettings().setValue(QStringLiteral("compat/autoApply"), on);
}

bool ATCompatIsDisabledForImage(IATImage *image) {
	auto sha = imageSha256(image);
	if (!sha) return false;
	const QString key = QStringLiteral("compat/") + sha256Hex(*sha) + QStringLiteral("/disabled");
	return QSettings().value(key, false).toBool();
}

void ATCompatDisableForImage(IATImage *image) {
	auto sha = imageSha256(image);
	if (!sha) return;
	const QString key = QStringLiteral("compat/") + sha256Hex(*sha) + QStringLiteral("/disabled");
	QSettings().setValue(key, true);
}

const ATCompatDBTitle *ATCompatApplyForImage(ATSimulator& sim, IATImage *image) {
	if (!image) return nullptr;
	if (!g().mView.IsValid()) return nullptr;
	if (!ATCompatIsAutoApplyEnabled()) return nullptr;
	if (ATCompatIsDisabledForImage(image)) return nullptr;

	std::vector<ATCompatMarker_local> markers;
	collectMarkers(image, markers);
	if (markers.empty()) return nullptr;

	std::vector<ATCompatKnownTag> tags;
	const ATCompatDBTitle *title = findMatchingTitle(markers, tags);
	if (!title || tags.empty()) return nullptr;

	applyTags(sim, tags);
	return title;
}

QString ATCompatApplyAfterMount(ATSimulator& sim) {
	if (!g().mView.IsValid() || !ATCompatIsAutoApplyEnabled())
		return QString();

	auto tryImage = [&](IATImage *img) -> QString {
		if (!img) return QString();
		const ATCompatDBTitle *title = ATCompatApplyForImage(sim, img);
		if (!title) return QString();
		return QString::fromUtf8(title->mName.c_str());
	};

	for (int i = 0; i < 2; ++i) {
		if (auto *cart = sim.GetCartridge(i)) {
			if (QString name = tryImage(cart->GetImage()); !name.isEmpty())
				return name;
		}
	}
	for (int i = 0; i < 8; ++i) {
		if (IATDiskImage *img = sim.GetDiskInterface(i).GetDiskImage())
			if (QString name = tryImage(img); !name.isEmpty())
				return name;
	}
	if (IATCassetteImage *img = sim.GetCassette().GetImage())
		if (QString name = tryImage(img); !name.isEmpty())
			return name;

	return QString();
}
