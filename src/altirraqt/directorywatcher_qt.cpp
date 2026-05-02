//	Altirra - Qt port
//	Real implementation of ATDirectoryWatcher backed by QFileSystemWatcher.
//
//	Upstream uses ReadDirectoryChangesW on Windows and a polling fallback
//	otherwise. The Qt port replaces both with QFileSystemWatcher, which Qt
//	implements over inotify on Linux, FSEvents on macOS, and
//	ReadDirectoryChangesW on Windows. Qt also internally falls back to
//	polling if a native watcher can't be installed (e.g. on a network
//	mount), so SetShouldUsePolling() just needs to record the preference;
//	there's no separate polling path to switch into.
//
//	The simulator polls CheckForChanges() from the GUI thread (via the
//	zero-interval QTimer in main.cpp that drives the emulation tick, which
//	eventually calls into ReadPhysicalSector on the virtual-folder disk
//	images). QFileSystemWatcher's signals also fire on the thread that owns
//	the watcher (the GUI thread, since we construct it from Init(), which is
//	called from the simulator). So all access to mbAllChanged and
//	mChangedDirs happens on a single thread and the upstream VDCriticalSection
//	(mMutex) is taken purely to keep the structure faithful to the upstream
//	contract — it is not load-bearing here.
//
//	The upstream class is VDThread-derived. We never call ThreadStart(), so
//	the thread never attaches; ThreadRun() is defined as a no-op solely to
//	satisfy the pure-virtual base. We repurpose the Win32-typed members:
//	  mhDir            -> QFileSystemWatcher * (cast)
//	  mhExitEvent      -> unused, kept null
//	  mhDirChangeEvent -> unused, kept null
//	  mpChangeBuffer   -> unused, kept null
//	  mChangeBufferSize-> unused, kept zero
//	The upstream-shaped members (mBasePath, mbRecursive, mMutex,
//	mChangedDirs, mbAllChanged) carry their normal meaning.

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QString>
#include <QStringList>

// Qt's `signals` keyword macro clashes with a parameter named `signals` in
// vd2/system/thread.h. Drop the macro before including any VD/Altirra header
// — we use Q_SIGNALS etc. directly above for connect() without needing the
// keyword form.
#ifdef signals
#undef signals
#endif

#include <vd2/system/filesys.h>
#include <vd2/system/thread.h>

#include <directorywatcher.h>

namespace {
	// Cap on how many directories we'll add to a single watcher in recursive
	// mode. inotify on Linux defaults to 8192 watches per user across the
	// whole system, so adding tens of thousands of subdirectories silently
	// fails. The upstream limit (in PollDirectory) is a nesting cap of 8;
	// we apply both the same nesting cap and an absolute count cap.
	constexpr int kMaxRecursiveNesting = 8;
	constexpr int kMaxWatchedPaths     = 1024;

	QFileSystemWatcher *AsWatcher(void *p) {
		return static_cast<QFileSystemWatcher *>(p);
	}

	void AddRecursive(QFileSystemWatcher *watcher, const QString& path,
	                  int nesting, int& budget) {
		if (budget <= 0)
			return;

		watcher->addPath(path);
		--budget;

		if (nesting >= kMaxRecursiveNesting)
			return;

		QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
		while (it.hasNext()) {
			it.next();
			AddRecursive(watcher, it.filePath(), nesting + 1, budget);
		}
	}

	// Convert a host-system absolute path back to a path relative to base,
	// matching upstream's NotifyAllChanged()/RunNotifyThread() contract: the
	// caller stores base-relative paths (or the empty string for the root
	// itself) into mChangedDirs.
	VDStringW MakeRelative(const VDStringW& base, const QString& absPath) {
		const std::wstring abs = absPath.toStdWString();
		const VDStringW& rel = VDFileGetRelativePath(base.c_str(), abs.c_str(), false);
		// VDFileGetRelativePath gives "." for the root; upstream stores the
		// empty string in that case (see RunNotifyThread).
		if (rel == L".")
			return VDStringW();
		return rel;
	}
}

bool ATDirectoryWatcher::sbShouldUsePolling = false;

void ATDirectoryWatcher::SetShouldUsePolling(bool enabled) {
	// QFileSystemWatcher already handles native vs. polling fallback
	// internally; the flag is recorded for parity with upstream's API.
	sbShouldUsePolling = enabled;
}

ATDirectoryWatcher::ATDirectoryWatcher()
	: VDThread("Altirra directory watcher")
	, mhDir(nullptr)
	, mhExitEvent(nullptr)
	, mhDirChangeEvent(nullptr)
	, mpChangeBuffer(nullptr)
	, mChangeBufferSize(0)
	, mbRecursive(false)
	, mbAllChanged(false)
{
}

ATDirectoryWatcher::~ATDirectoryWatcher() {
	Shutdown();
}

void ATDirectoryWatcher::Init(const wchar_t *basePath, bool recursive) {
	Shutdown();

	mBasePath = basePath ? VDStringW(basePath) : VDStringW();
	mbRecursive = recursive;

	if (mBasePath.empty())
		return;

	const QString qBase = QString::fromWCharArray(mBasePath.c_str(),
		(int)mBasePath.size());
	if (!QFileInfo(qBase).isDir()) {
		// Nothing to watch; the simulator will simply never see changes.
		// This matches polling-mode semantics on a missing directory.
		return;
	}

	auto *watcher = new QFileSystemWatcher();
	mhDir = watcher;

	int budget = kMaxWatchedPaths;
	if (recursive) {
		AddRecursive(watcher, qBase, 0, budget);
	} else {
		watcher->addPath(qBase);
	}

	// Both signals are connected to the same handler. The QFileSystemWatcher
	// is the connection's context, so the lambdas are torn down when the
	// watcher is deleted in Shutdown(). `this` outlives the watcher (we
	// destroy the watcher from Shutdown(), which runs before ~ATDirectoryWatcher
	// returns), so capturing it directly is safe.
	auto onChange = [this, watcher](const QString& path) {
		// On many filesystems a write-then-replace results in the original
		// path being removed from the watcher's set. Re-add it so we don't
		// silently stop watching after the first change.
		if (!watcher->files().contains(path) && !watcher->directories().contains(path)) {
			QFileInfo fi(path);
			if (fi.exists())
				watcher->addPath(path);
		}

		vdsynchronized(mMutex) {
			mbAllChanged = true;

			const VDStringW rel = MakeRelative(mBasePath, path);
			mChangedDirs.insert(rel);
		}
	};

	QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, watcher, onChange);
	QObject::connect(watcher, &QFileSystemWatcher::fileChanged, watcher, onChange);
}

void ATDirectoryWatcher::Shutdown() {
	if (mhDir) {
		// Plain delete is fine here: QFileSystemWatcher only owns its
		// internal engine, not any active slot stack frame, so we don't need
		// deleteLater().
		delete AsWatcher(mhDir);
		mhDir = nullptr;
	}

	vdsynchronized(mMutex) {
		mChangedDirs.clear();
		mbAllChanged = false;
	}

	// mhExitEvent / mhDirChangeEvent / mpChangeBuffer are never allocated in
	// the Qt build; nothing to free here.
}

bool ATDirectoryWatcher::CheckForChanges() {
	bool changed = false;

	vdsynchronized(mMutex) {
		changed = mbAllChanged;

		if (changed) {
			mbAllChanged = false;
			mChangedDirs.clear();
		} else if (!mChangedDirs.empty()) {
			mChangedDirs.clear();
			changed = true;
		}
	}

	return changed;
}

bool ATDirectoryWatcher::CheckForChanges(vdfastvector<wchar_t>& strheap) {
	bool allChanged = false;
	strheap.clear();

	vdsynchronized(mMutex) {
		allChanged = mbAllChanged;

		if (allChanged) {
			mbAllChanged = false;
		} else {
			for (const VDStringW& s : mChangedDirs) {
				const wchar_t *t = s.c_str();
				strheap.insert(strheap.end(), t, t + s.size() + 1);
			}
		}

		mChangedDirs.clear();
	}

	return allChanged;
}

void ATDirectoryWatcher::ThreadRun() {
	// QFileSystemWatcher delivers via Qt signals on the GUI thread; no
	// background thread is needed and ThreadStart() is never called. This
	// override exists only to satisfy the pure-virtual base.
}

void ATDirectoryWatcher::RunPollThread() {
	// Polling is handled inside QFileSystemWatcher when no native engine is
	// available; we never run a poll loop ourselves.
}

void ATDirectoryWatcher::PollDirectory(uint32 *, const VDStringSpanW&, uint32) {
	// See RunPollThread().
}

void ATDirectoryWatcher::RunNotifyThread() {
	// See ThreadRun().
}

void ATDirectoryWatcher::NotifyAllChanged() {
	vdsynchronized(mMutex) {
		mbAllChanged = true;
	}
}
