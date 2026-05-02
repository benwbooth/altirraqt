//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2004 Avery Lee, All Rights Reserved.
//
//	Cross-platform Qt port: the original header depended on Win32 critical
//	sections and SRW locks via extern "C" __declspec(dllimport) declarations.
//	We preserve the API surface (VDThread, VDCriticalSection, VDSignal,
//	VDSemaphore, VDRWLock, VDConditionVariable, vdsynchronized/vdsyncexclusive)
//	but back it with <thread>/<mutex>/<condition_variable>/<shared_mutex>.

#ifndef f_VD2_SYSTEM_THREAD_H
#define f_VD2_SYSTEM_THREAD_H

#include <vd2/system/vdtypes.h>
#include <vd2/system/atomic.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <thread>

typedef void *VDThreadHandle;
typedef uint32 VDThreadID;
typedef uint32 VDThreadId;
typedef uint32 VDProcessId;

VDThreadID VDGetCurrentThreadID();
VDProcessId VDGetCurrentProcessId();
uint32 VDGetLogicalProcessorCount();

void VDSetThreadDebugName(VDThreadID tid, const char *name);
void VDThreadSleep(int milliseconds);

///////////////////////////////////////////////////////////////////////////
//
//	VDThread
//
///////////////////////////////////////////////////////////////////////////

class VDThread {
public:
	VDThread(const char *pszDebugName = nullptr);
	~VDThread() throw();

	bool ThreadStart();
	void ThreadWait();

	void ThreadCancelSynchronousIo();

	bool isThreadActive();

	bool isThreadAttached() const {
		return mhThread != nullptr;
	}

	bool IsCurrentThread() const;

	VDThreadHandle getThreadHandle() const {
		return mhThread;
	}

	VDThreadID getThreadID() const {
		return mThreadID;
	}

	virtual void ThreadRun() = 0;

private:
	static void StaticThreadStart(VDThread *pThis);
	void ThreadDetach();

	const char *mpszDebugName;
	VDThreadHandle	mhThread;
	VDThreadID		mThreadID;
};

///////////////////////////////////////////////////////////////////////////

class VDCriticalSection {
	VDCriticalSection(const VDCriticalSection&) = delete;
	VDCriticalSection& operator=(const VDCriticalSection&) = delete;

public:
	class AutoLock {
		VDCriticalSection& cs;
	public:
		AutoLock(VDCriticalSection& csect) : cs(csect) { cs.Lock(); }
		~AutoLock() { cs.Unlock(); }

		inline operator bool() const { return false; }
	};

	VDCriticalSection() = default;
	~VDCriticalSection() = default;

	void operator++() { mMutex.lock(); }
	void operator--() { mMutex.unlock(); }

	void Lock()   { mMutex.lock(); }
	void Unlock() { mMutex.unlock(); }

	// Exposed for VDConditionVariable / friends.
	std::recursive_mutex& native() { return mMutex; }

private:
	std::recursive_mutex mMutex;
};

#define vdsynchronized2(lock) if(VDCriticalSection::AutoLock vd__lock=(lock))VDNEVERHERE;else
#define vdsynchronized1(lock) vdsynchronized2(lock)
#define vdsynchronized(lock) vdsynchronized1(lock)

///////////////////////////////////////////////////////////////////////////

class VDSignalBase {
	VDSignalBase(const VDSignalBase&) = delete;
	VDSignalBase& operator=(const VDSignalBase&) = delete;

public:
	VDSignalBase();
	~VDSignalBase();

	void signal();
	bool check();
	void wait();
	int wait(VDSignalBase *second);
	int wait(VDSignalBase *second, VDSignalBase *third);
	static int waitMultiple(const VDSignalBase *const *signals, int count);

	bool tryWait(uint32 timeoutMillisec);

	void *getHandle() { return this; }

	void operator()() { signal(); }

protected:
	std::mutex              mMutex;
	std::condition_variable mCond;
	bool                    mSignaled = false;
	bool                    mAutoReset = true;
};

class VDSignal : public VDSignalBase {
public:
	VDSignal();
};

class VDSignalPersistent : public VDSignalBase {
public:
	VDSignalPersistent();

	void unsignal();
};

///////////////////////////////////////////////////////////////////////////

class VDSemaphore {
public:
	VDSemaphore(int initial);
	~VDSemaphore();

	void *GetHandle() const { return const_cast<VDSemaphore *>(this); }

	void Reset(int count);

	void Wait();
	bool Wait(int timeoutMillis);
	bool TryWait();
	void Post();

private:
	std::mutex              mMutex;
	std::condition_variable mCond;
	int                     mCount = 0;
};

///////////////////////////////////////////////////////////////////////////

class VDRWLock {
	VDRWLock(const VDRWLock&) = delete;
	VDRWLock& operator=(const VDRWLock&) = delete;

public:
	VDRWLock() = default;

	void LockExclusive() noexcept;
	void UnlockExclusive() noexcept;

	std::shared_mutex& native() { return mLock; }

private:
	friend class VDConditionVariable;
	std::shared_mutex mLock;
};

///////////////////////////////////////////////////////////////////////////

struct VDRWAutoLockExclusive {
	VDRWAutoLockExclusive(VDRWLock& rwLock)
		: mRWLock(rwLock)
	{
		mRWLock.LockExclusive();
	}

	~VDRWAutoLockExclusive() {
		mRWLock.UnlockExclusive();
	}

	VDRWLock& mRWLock;
};

#define vdsyncexclusive(rwlock) if (VDRWAutoLockExclusive autoLock{rwlock}; false); else

///////////////////////////////////////////////////////////////////////////

class VDConditionVariable {
	VDConditionVariable(const VDConditionVariable&) = delete;
	VDConditionVariable& operator=(const VDConditionVariable&) = delete;

public:
	VDConditionVariable() = default;

	void Wait(VDRWLock& rwLock) noexcept;
	void NotifyOne() noexcept;
	void NotifyAll() noexcept;

private:
	std::condition_variable_any mCV;
};

#endif
