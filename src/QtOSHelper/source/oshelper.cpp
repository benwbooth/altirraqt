//	Altirra - Qt port
//	Qt/POSIX implementation of Altirra's oshelper.h API.
//	Copyright (C) 2026 Ben Booth
//
//	GPL-2.0-or-later (matches Altirra)
//
//	Replaces upstream Altirra/source/oshelper.cpp. The upstream is ~820 lines
//	of Win32 (LoadResource/GlobalAlloc/SHGetFolderPath/etc). This port provides
//	equivalent functionality using Qt (QClipboard, QImage, QDesktopServices)
//	and POSIX (chmod, unlink, /dev/urandom). Win32 resources don't exist here,
//	so ATLoadKernelResource / ATLockResource / ATLoadMiscResource all return
//	failure — ROMs are loaded from disk through firmwaremanager's normal path
//	instead of being embedded in the executable.

#include <stdafx.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <random>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QString>
#include <QUrl>
#include <QWidget>

#include <vd2/system/file.h>
#include <vd2/system/filesys.h>
#include <vd2/system/VDString.h>
#include <vd2/Kasumi/pixmap.h>
#include <vd2/Kasumi/pixmaputils.h>
#include <vd2/Kasumi/pixmapops.h>
#include <at/atcore/enumparseimpl.h>

// IDR_* enum mirrors the upstream Win32 resource ids that Altirra source files
// pass to ATLoadKernelResource(). Defining them here keeps oshelper insulated
// from the Win32 .rc file.
#include <resource.h>

#include <QFile>

#include <oshelper.h>

AT_DEFINE_ENUM_TABLE_BEGIN(ATProcessEfficiencyMode)
	{ ATProcessEfficiencyMode::Default,     "default" },
	{ ATProcessEfficiencyMode::Performance, "performance" },
	{ ATProcessEfficiencyMode::Efficiency,  "efficiency" },
AT_DEFINE_ENUM_TABLE_END(ATProcessEfficiencyMode, ATProcessEfficiencyMode::Default)

namespace {

QString wcharToQString(const wchar_t *s) {
	if (!s) return {};
	return QString::fromWCharArray(s);
}

QImage pixmapToQImage(const VDPixmap& px) {
	VDPixmapBuffer staging(px.w, px.h, nsVDPixmap::kPixFormat_XRGB8888);
	VDPixmapBlt(staging, px);
	return QImage(
		(const uchar *)staging.data,
		staging.w, staging.h,
		(int)staging.pitch,
		QImage::Format_RGB32
	).copy();  // copy because staging goes out of scope
}

} // namespace

//-----------------------------------------------------------------------------
// Resource loading — Win32 resources are emulated via Qt's resource system.
// Avery's BSD-licensed Atari ROMs are MADS-assembled at build time and
// embedded under the /altirra/roms/ resource prefix (see CMakeLists.txt +
// altirra_roms.qrc.in). Each IDR_* maps to the corresponding alias.
//
// Resource ids that aren't bundled (e.g. IDR_U1MBBIOS / IDR_RAPIDUS* — those
// need a separate LZ-pack build step that's not yet ported) return failure;
// the upstream firmware manager falls back to file-system loading or skips
// the device.
//-----------------------------------------------------------------------------

namespace {

QString romPathForResource(int id) {
	switch (id) {
		case IDR_KERNEL:        return QStringLiteral(":/altirra/roms/kernel.rom");
		case IDR_KERNELXL:      return QStringLiteral(":/altirra/roms/kernelxl.rom");
		case IDR_KERNEL816:     return QStringLiteral(":/altirra/roms/kernel816.rom");
		case IDR_NOKERNEL:      return QStringLiteral(":/altirra/roms/nokernel.rom");
		case IDR_5200KERNEL:    return QStringLiteral(":/altirra/roms/superkernel.rom");
		case IDR_NOCARTRIDGE:   return QStringLiteral(":/altirra/roms/nocartridge.rom");
		case IDR_NOHDBIOS:      return QStringLiteral(":/altirra/roms/nohdbios.rom");
		case IDR_NOGAME:        return QStringLiteral(":/altirra/roms/nogame.rom");
		case IDR_NOMIO:         return QStringLiteral(":/altirra/roms/nomio.rom");
		case IDR_NOBLACKBOX:    return QStringLiteral(":/altirra/roms/noblackbox.rom");
		case IDR_BASIC:         return QStringLiteral(":/altirra/roms/atbasic.bin");
		default:                return {};
	}
}

// Forward decl of the rcc-generated registration function. Defined at file
// scope (not the anonymous namespace) so it matches the C++ symbol that rcc
// emits.
} // namespace

extern int qInitResources_altirra_roms();

namespace {

void ensureRomResourcesRegistered() {
	// Static libraries can discard the rcc-generated TU's static-init unless
	// something keeps the symbol alive. Explicitly invoking it here also
	// makes sure /altirra/roms/* is mounted before any caller uses it.
	static int once = []{ qInitResources_altirra_roms(); return 1; }();
	(void)once;
}

QByteArray loadResourceBytes(int id) {
	ensureRomResourcesRegistered();
	const QString path = romPathForResource(id);
	if (path.isEmpty())
		return {};
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		return {};
	return f.readAll();
}

} // namespace

const void *ATLockResource(uint32 id, size_t& size) {
	// The "lock and keep mapped" semantics of the Win32 API don't translate
	// directly — Qt's QResource gives us a pointer-to-mapped-data when the
	// resource is built in. We satisfy the contract by stashing the bytes in
	// a static cache keyed by id; pointers remain valid for the process
	// lifetime, which matches every caller's expectation.
	static std::unordered_map<int, QByteArray> cache;
	auto& slot = cache[(int)id];
	if (slot.isEmpty()) {
		slot = loadResourceBytes((int)id);
		if (slot.isEmpty()) { size = 0; return nullptr; }
	}
	size = (size_t)slot.size();
	return slot.constData();
}

bool ATLoadKernelResource(int id, void *dst, uint32 offset, uint32 len, bool allowPartial) {
	const QByteArray bytes = loadResourceBytes(id);
	if (bytes.isEmpty()) return false;
	if (offset > (uint32)bytes.size()) return false;
	const uint32 avail = (uint32)bytes.size() - offset;
	const uint32 take = (avail < len) ? avail : len;
	if (!allowPartial && take < len) return false;
	std::memcpy(dst, bytes.constData() + offset, take);
	if (take < len) std::memset((char *)dst + take, 0, len - take);
	return true;
}

bool ATLoadKernelResource(int id, vdfastvector<uint8>& data) {
	const QByteArray bytes = loadResourceBytes(id);
	data.clear();
	if (bytes.isEmpty()) return false;
	data.assign((const uint8 *)bytes.constData(),
	            (const uint8 *)bytes.constData() + bytes.size());
	return true;
}

bool ATLoadKernelResourceLZPacked(int /*id*/, vdfastvector<uint8>& data) {
	// Upstream stores some ROMs LZ-packed (Ultimate1MB BIOS, Rapidus). The
	// pack/unpack tooling lives in ATCompiler, which isn't ported yet.
	data.clear();
	return false;
}

bool ATLoadMiscResource(int id, vdfastvector<uint8>& data) {
	return ATLoadKernelResource(id, data);
}

bool ATLoadImageResource(uint32 /*id*/, VDPixmapBuffer& /*buf*/) {
	return false;
}

//-----------------------------------------------------------------------------
// File attributes
//-----------------------------------------------------------------------------

void ATFileSetReadOnlyAttribute(const wchar_t *path, bool readOnly) {
	if (!path) return;

	const QByteArray utf8 = wcharToQString(path).toUtf8();
	struct stat st;
	if (stat(utf8.constData(), &st) != 0)
		return;

	mode_t mode = st.st_mode;
	if (readOnly)
		mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	else
		mode |= (S_IWUSR);

	(void)chmod(utf8.constData(), mode);
}

//-----------------------------------------------------------------------------
// Clipboard
//-----------------------------------------------------------------------------

void ATCopyFrameToClipboard(const VDPixmap& px) {
	if (auto *cb = QGuiApplication::clipboard()) {
		cb->setImage(pixmapToQImage(px));
	}
}

void ATCopyTextToClipboard(void * /*hwnd*/, const char *s) {
	if (!s) return;
	if (auto *cb = QGuiApplication::clipboard())
		cb->setText(QString::fromUtf8(s));
}

void ATCopyTextToClipboard(void * /*hwnd*/, const wchar_t *s) {
	if (!s) return;
	if (auto *cb = QGuiApplication::clipboard())
		cb->setText(wcharToQString(s));
}

//-----------------------------------------------------------------------------
// Image I/O via QImage
//-----------------------------------------------------------------------------

void ATLoadFrame(VDPixmapBuffer& px, const wchar_t *filename) {
	px.clear();
	if (!filename) return;

	QImage img(wcharToQString(filename));
	if (img.isNull()) return;

	img = img.convertToFormat(QImage::Format_RGB32);
	px.init(img.width(), img.height(), nsVDPixmap::kPixFormat_XRGB8888);
	for (int y = 0; y < img.height(); ++y)
		memcpy(px.GetPixelRow<uint8>(y), img.constScanLine(y), img.width() * 4);
}

void ATLoadFrameFromMemory(VDPixmapBuffer& px, const void *mem, size_t len) {
	px.clear();
	if (!mem || !len) return;

	QImage img;
	if (!img.loadFromData((const uchar *)mem, (int)len))
		return;

	img = img.convertToFormat(QImage::Format_RGB32);
	px.init(img.width(), img.height(), nsVDPixmap::kPixFormat_XRGB8888);
	for (int y = 0; y < img.height(); ++y)
		memcpy(px.GetPixelRow<uint8>(y), img.constScanLine(y), img.width() * 4);
}

void ATSaveFrame(const VDPixmap& px, const wchar_t *filename) {
	if (!filename) return;
	QImage img = pixmapToQImage(px);
	img.save(wcharToQString(filename));
}

//-----------------------------------------------------------------------------
// Window placement (HWND-based upstream; no-op here — Qt widgets manage their
// own geometry, and the UI-layer port will add settings-backed placement).
//-----------------------------------------------------------------------------

void ATUISaveWindowPlacement(void * /*hwnd*/, const char * /*name*/) {}
void ATUISaveWindowPlacement(const char * /*name*/, const vdrect32& /*r*/, bool /*isMaximized*/, uint32 /*dpi*/) {}
void ATUIRestoreWindowPlacement(void * /*hwnd*/, const char * /*name*/, int /*nCmdShow*/, bool /*sizeOnly*/) {}

void ATUIEnableEditControlAutoComplete(void * /*hwnd*/) {}

//-----------------------------------------------------------------------------
// Help + URL launch
//-----------------------------------------------------------------------------

VDStringW ATGetHelpPath() {
	// Canonical install layout: {app}/share/altirra/Altirra.chm sibling of the
	// binary. Caller falls back gracefully if this path does not resolve.
	return VDMakePath(VDGetProgramPath().c_str(), L"Altirra.chm");
}

void ATShowHelp(void * /*hwnd*/, const wchar_t *filename) {
	const VDStringW path = filename ? VDStringW(filename) : ATGetHelpPath();
	ATLaunchURL(path.c_str());
}

void ATLaunchURL(const wchar_t *url) {
	if (!url) return;
	const QString s = wcharToQString(url);
	if (s.contains(QStringLiteral("://")))
		QDesktopServices::openUrl(QUrl(s));
	else
		QDesktopServices::openUrl(QUrl::fromLocalFile(s));
}

void ATLaunchFileForEdit(const wchar_t *file) {
	if (!file) return;
	QDesktopServices::openUrl(QUrl::fromLocalFile(wcharToQString(file)));
}

//-----------------------------------------------------------------------------
// Privileges & elevation (no elevation concept on POSIX — always fail/no-op)
//-----------------------------------------------------------------------------

bool ATIsUserAdministrator() {
	return geteuid() == 0;
}

void ATRelaunchElevated(VDGUIHandle /*parent*/, const wchar_t * /*params*/) {}
void ATRelaunchElevatedWithEscapedArgs(VDGUIHandle /*parent*/, vdspan<const wchar_t *> /*args*/) {}

//-----------------------------------------------------------------------------
// GUID generation — v4 style, 16 random bytes with version/variant bits set.
//-----------------------------------------------------------------------------

void ATGenerateGuid(uint8 guid[16]) {
	std::random_device rd;
	for (int i = 0; i < 16; ++i)
		guid[i] = (uint8)(rd() & 0xff);
	guid[6] = (guid[6] & 0x0F) | 0x40;  // version 4
	guid[8] = (guid[8] & 0x3F) | 0x80;  // RFC 4122 variant
}

//-----------------------------------------------------------------------------
// File manager integration (xdg-open on the containing directory).
//-----------------------------------------------------------------------------

void ATShowFileInSystemExplorer(const wchar_t *filename) {
	if (!filename) return;
	const QFileInfo info(wcharToQString(filename));
	const QString dir = info.absoluteDir().absolutePath();
	QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

//-----------------------------------------------------------------------------
// Process efficiency — Win32 EcoQoS equivalent. No direct POSIX analogue;
// setpriority() is imperfect (only affects scheduling when starved), so this
// is intentionally a no-op for now.
//-----------------------------------------------------------------------------

void ATSetProcessEfficiencyMode(ATProcessEfficiencyMode /*mode*/) {}
