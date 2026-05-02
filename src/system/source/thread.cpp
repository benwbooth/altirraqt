//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2004 Avery Lee, All Rights Reserved.
//
//	Cross-platform Qt port: original implementation used Win32
//	CreateThread/CRITICAL_SECTION/SRWLOCK/etc. Replaced with std::thread and
//	the std::sync primitives; POSIX getpid/pthread used only for the thread
//	and process ID helpers.

#include <stdafx.h>

#include <vd2/system/vdtypes.h>
#include <vd2/system/thread.h>
#include <vd2/system/tls.h>
#include <vd2/system/bitmath.h>

#include <chrono>
#include <functional>
#include <new>
#include <system_error>
#include <thread>

#include <pthread.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/prctl.h>
#endif

namespace {
	struct ThreadHandle {
		std::thread worker;
	};

	VDThreadID current_tid_native() {
	#if defined(__linux__)
		return (VDThreadID)syscall(SYS_gettid);
	#elif defined(__APPLE__)
		uint64_t tid = 0;
		pthread_threadid_np(nullptr, &tid);
		return (VDThreadID)tid;
	#else
		return (VDThreadID)(uintptr_t)pthread_self();
	#endif
	}
}

VDThreadID VDGetCurrentThreadID() {
	return current_tid_native();
}

VDProcessId VDGetCurrentProcessId() {
	return (VDProcessId)getpid();
}

uint32 VDGetLogicalProcessorCount() {
	unsigned n = std::thread::hardware_concurrency();
	return n ? n : 1;
}

void VDSetThreadDebugName(VDThreadID, const char *name) {
#if defined(__linux__)
	if (name)
		prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
#elif defined(__APPLE__)
	if (name)
		pthread_setname_np(name);
#else
	(void)name;
#endif
}

void VDThreadSleep(int milliseconds) {
	if (milliseconds < 0)
		milliseconds = 0;
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

///////////////////////////////////////////////////////////////////////////

VDThread::VDThread(const char *pszDebugName)
	: mpszDebugName(pszDebugName)
	, mhThread(nullptr)
	, mThreadID(0)
{
}

VDThread::~VDThread() throw() {
	if (isThreadAttached())
		ThreadWait();
}

bool VDThread::ThreadStart() {
	if (isThreadAttached())
		return false;

	auto *h = new(std::nothrow) ThreadHandle;
	if (!h)
		return false;

	try {
		h->worker = std::thread(&VDThread::StaticThreadStart, this);
	} catch (const std::system_error&) {
		delete h;
		return false;
	}

	mhThread = h;
	mThreadID = 0; // populated inside the worker
	return true;
}

void VDThread::StaticThreadStart(VDThread *pThis) {
	pThis->mThreadID = current_tid_native();

	if (pThis->mpszDebugName)
		VDSetThreadDebugName(pThis->mThreadID, pThis->mpszDebugName);

	VDInitThreadData(pThis->mpszDebugName ? pThis->mpszDebugName : "Thread");
	pThis->ThreadRun();
	VDDeinitThreadData();
}

void VDThread::ThreadWait() {
	if (!mhThread)
		return;

	auto *h = static_cast<ThreadHandle *>(mhThread);
	if (h->worker.joinable())
		h->worker.join();

	delete h;
	mhThread = nullptr;
	mThreadID = 0;
}

void VDThread::ThreadCancelSynchronousIo() {
	// No portable equivalent; Win32's CancelSynchronousIo has no POSIX counterpart.
}

bool VDThread::isThreadActive() {
	if (!mhThread)
		return false;

	auto *h = static_cast<ThreadHandle *>(mhThread);
	return h->worker.joinable();
}

bool VDThread::IsCurrentThread() const {
	if (!mhThread)
		return false;

	auto *h = static_cast<ThreadHandle *>(mhThread);
	return h->worker.get_id() == std::this_thread::get_id();
}

void VDThread::ThreadDetach() {
	if (!mhThread)
		return;

	auto *h = static_cast<ThreadHandle *>(mhThread);
	if (h->worker.joinable())
		h->worker.detach();

	delete h;
	mhThread = nullptr;
	mThreadID = 0;
}

///////////////////////////////////////////////////////////////////////////

VDSignalBase::VDSignalBase() = default;

VDSignalBase::~VDSignalBase() = default;

void VDSignalBase::signal() {
	{
		std::lock_guard<std::mutex> g(mMutex);
		mSignaled = true;
	}
	mCond.notify_all();
}

bool VDSignalBase::check() {
	std::lock_guard<std::mutex> g(mMutex);
	if (mSignaled) {
		if (mAutoReset)
			mSignaled = false;
		return true;
	}
	return false;
}

void VDSignalBase::wait() {
	std::unique_lock<std::mutex> g(mMutex);
	mCond.wait(g, [&]{ return mSignaled; });
	if (mAutoReset)
		mSignaled = false;
}

int VDSignalBase::wait(VDSignalBase *second) {
	const VDSignalBase *arr[2] = {this, second};
	return waitMultiple(arr, 2);
}

int VDSignalBase::wait(VDSignalBase *second, VDSignalBase *third) {
	const VDSignalBase *arr[3] = {this, second, third};
	return waitMultiple(arr, 3);
}

int VDSignalBase::waitMultiple(const VDSignalBase *const *signals, int count) {
	// Simple polling implementation; fine for the infrequent paths that use it.
	for (;;) {
		for (int i = 0; i < count; ++i) {
			auto *s = const_cast<VDSignalBase *>(signals[i]);
			if (s->check())
				return i;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

bool VDSignalBase::tryWait(uint32 timeoutMillisec) {
	std::unique_lock<std::mutex> g(mMutex);
	if (!mCond.wait_for(g, std::chrono::milliseconds(timeoutMillisec),
			[&]{ return mSignaled; }))
		return false;
	if (mAutoReset)
		mSignaled = false;
	return true;
}

VDSignal::VDSignal() {
	mAutoReset = true;
	mSignaled = false;
}

VDSignalPersistent::VDSignalPersistent() {
	mAutoReset = false;
	mSignaled = false;
}

void VDSignalPersistent::unsignal() {
	std::lock_guard<std::mutex> g(mMutex);
	mSignaled = false;
}

///////////////////////////////////////////////////////////////////////////

VDSemaphore::VDSemaphore(int initial)
	: mCount(initial)
{
}

VDSemaphore::~VDSemaphore() = default;

void VDSemaphore::Reset(int count) {
	std::lock_guard<std::mutex> g(mMutex);
	mCount = count;
	mCond.notify_all();
}

void VDSemaphore::Wait() {
	std::unique_lock<std::mutex> g(mMutex);
	mCond.wait(g, [&]{ return mCount > 0; });
	--mCount;
}

bool VDSemaphore::Wait(int timeoutMillis) {
	std::unique_lock<std::mutex> g(mMutex);
	if (!mCond.wait_for(g, std::chrono::milliseconds(timeoutMillis),
			[&]{ return mCount > 0; }))
		return false;
	--mCount;
	return true;
}

bool VDSemaphore::TryWait() {
	std::lock_guard<std::mutex> g(mMutex);
	if (mCount <= 0)
		return false;
	--mCount;
	return true;
}

void VDSemaphore::Post() {
	{
		std::lock_guard<std::mutex> g(mMutex);
		++mCount;
	}
	mCond.notify_one();
}

///////////////////////////////////////////////////////////////////////////

void VDRWLock::LockExclusive() noexcept {
	mLock.lock();
}

void VDRWLock::UnlockExclusive() noexcept {
	mLock.unlock();
}

///////////////////////////////////////////////////////////////////////////

void VDConditionVariable::Wait(VDRWLock& rwLock) noexcept {
	mCV.wait(rwLock.native());
}

void VDConditionVariable::NotifyOne() noexcept {
	mCV.notify_one();
}

void VDConditionVariable::NotifyAll() noexcept {
	mCV.notify_all();
}
