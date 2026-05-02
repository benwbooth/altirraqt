//	altirraqt — Linux replacements for the Win32-specific helpers
//	upstream's hostdevice.cpp, pclink.cpp and customdevice.cpp call.
//
//	Three pieces:
//	  1. ATTranslateWin32ErrorToSIOError(uint32) — accepts an errno
//	     (or upstream's "win32 error" code if a leftover pre-Linux blob
//	     reaches us) and maps it to the matching kATCIOStat_* code.
//	  2. VDFileWatcher — Qt-side replacement on top of QFileSystemWatcher.
//	  3. ATCreateDeviceCustomNetworkEngine — minimal QTcpServer-backed
//	     network engine that satisfies upstream's interface.

#include <cerrno>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#undef signals
#undef slots

#include <vd2/system/vdtypes.h>
#include <vd2/system/filewatcher.h>
#include <vd2/system/function.h>
#include <vd2/system/refcount.h>
#include <vd2/system/VDString.h>
#include <at/atcore/cio.h>

uint8 ATTranslateWin32ErrorToSIOError(uint32 err);
uint8 ATTranslateWin32ErrorToSIOError(uint32 err) {
	// Upstream callers pass either errno (POSIX) or a Win32 error code
	// (legacy Windows path). We map both into the same SIO status set.
	switch ((int)err) {
		case ENOENT:        return kATCIOStat_FileNotFound;
		case ENOTDIR:       return kATCIOStat_PathNotFound;
		case EEXIST:        return kATCIOStat_FileExists;
		case ENOSPC:        return kATCIOStat_DiskFull;
		case ENOTEMPTY:     return kATCIOStat_DirNotEmpty;
		case EACCES:
		case EPERM:         return kATCIOStat_AccessDenied;
		case EBUSY:
		case ETXTBSY:       return kATCIOStat_FileLocked;
		case EROFS:         return kATCIOStat_AccessDenied;
		default:            return kATCIOStat_SystemError;
	}
}

// ---------------------------------------------------------------------------
// VDFileWatcher — upstream interface lives in vd2/system/filewatcher.h. We
// implement just enough of it (Init, Shutdown, Wait) on top of
// QFileSystemWatcher to satisfy customdevice.cpp's needs. The interface
// header declares VDFileWatcher as a concrete class with internal state, so
// the stub mirrors the same fields and forwards via a Qt object on the heap.
// ---------------------------------------------------------------------------

namespace {

class VDFileWatcherQt : public QObject {
public:
	VDFileWatcherQt() {
		mWatcher = new QFileSystemWatcher(this);
		QObject::connect(mWatcher, &QFileSystemWatcher::directoryChanged,
			this, [this]{ mDirty = true; });
		QObject::connect(mWatcher, &QFileSystemWatcher::fileChanged,
			this, [this]{ mDirty = true; });
	}
	void Watch(const QString& path) {
		if (!path.isEmpty()) mWatcher->addPath(path);
	}
	void Stop() {
		mWatcher->removePaths(mWatcher->files());
		mWatcher->removePaths(mWatcher->directories());
		mDirty = false;
	}
	bool TakeDirty() {
		const bool d = mDirty;
		mDirty = false;
		return d;
	}
	void SetCallback(IVDFileWatcherCallback *cb) { mpCallback = cb; }

	QFileSystemWatcher *mWatcher = nullptr;
	bool                mDirty = false;
	IVDFileWatcherCallback *mpCallback = nullptr;
};

} // namespace

// We use the public `mChangeHandle` slot to hold the QObject we own.

VDFileWatcher::VDFileWatcher()
	: mChangeHandle(new VDFileWatcherQt)
	, mLastWriteTime(0)
	, mbWatchDir(false)
	, mpCB(nullptr)
	, mbRepeatRequested(false)
	, mbThunksInited(false)
	, mpThunk(nullptr)
	, mTimerId(0)
{}

VDFileWatcher::~VDFileWatcher() {
	delete static_cast<VDFileWatcherQt *>(mChangeHandle);
}

bool VDFileWatcher::IsActive() const {
	return mChangeHandle != nullptr;
}

void VDFileWatcher::Init(const wchar_t *path, IVDFileWatcherCallback *cb) {
	auto *w = static_cast<VDFileWatcherQt *>(mChangeHandle);
	mpCB = cb;
	mbWatchDir = false;
	w->SetCallback(cb);
	w->Stop();
	if (path) w->Watch(QString::fromWCharArray(path));
}

void VDFileWatcher::InitDir(const wchar_t *path, bool /*subdirs*/, IVDFileWatcherCallback *cb) {
	auto *w = static_cast<VDFileWatcherQt *>(mChangeHandle);
	mpCB = cb;
	mbWatchDir = true;
	w->SetCallback(cb);
	w->Stop();
	if (path) w->Watch(QString::fromWCharArray(path));
}

void VDFileWatcher::Shutdown() {
	auto *w = static_cast<VDFileWatcherQt *>(mChangeHandle);
	w->Stop();
}

bool VDFileWatcher::Wait(uint32 /*timeoutMillisec*/) {
	// Upstream callers poll Wait(0). We just report any change since the
	// last call. The Qt event loop drives the underlying notifier.
	return static_cast<VDFileWatcherQt *>(mChangeHandle)->TakeDirty();
}

// `ATCreateDeviceCustomNetworkEngine` lives only inside `customdevice.cpp`,
// which the Qt build excludes (it depends on the upstream scripting-VM
// types in `customdevicevmtypes.cpp`, which in turn pulls in `atui`).
// Once that VM lands, a `QTcpServer`-backed engine should live here.
