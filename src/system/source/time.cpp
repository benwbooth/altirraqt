//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2004 Avery Lee, All Rights Reserved.
//
//	Cross-platform Qt port: the original implementation used GetTickCount(),
//	QueryPerformanceCounter, winmm's timeBeginPeriod/timeGetTime and Win32's
//	SetTimer. POSIX port uses std::chrono::steady_clock and a std::thread-
//	based lazy timer.

#include <stdafx.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <vd2/system/time.h>
#include <vd2/system/thread.h>

using clock_type = std::chrono::steady_clock;

namespace {
	const clock_type::time_point kEpoch = clock_type::now();

	uint64 ms_since_epoch() {
		return (uint64)std::chrono::duration_cast<std::chrono::milliseconds>(
			clock_type::now() - kEpoch).count();
	}
}

uint32 VDGetCurrentTick() {
	return (uint32)ms_since_epoch();
}

uint64 VDGetCurrentTick64() {
	return ms_since_epoch();
}

uint64 VDGetPreciseTick() {
	return (uint64)std::chrono::duration_cast<std::chrono::nanoseconds>(
		clock_type::now() - kEpoch).count();
}

uint64 VDGetPreciseTicksPerSecondI() {
	return 1000000000ull;
}

double VDGetPreciseTicksPerSecond() {
	return 1.0e9;
}

double VDGetPreciseSecondsPerTick() {
	return 1.0e-9;
}

uint32 VDGetAccurateTick() {
	return VDGetCurrentTick();
}

///////////////////////////////////////////////////////////////////////////////

VDCallbackTimer::VDCallbackTimer()
	: mpCB(nullptr)
	, mTimerAccuracy(0)
	, mTimerPeriod(0)
	, mbExit(false)
	, mbPrecise(false)
{
}

VDCallbackTimer::~VDCallbackTimer() {
	Shutdown();
}

bool VDCallbackTimer::Init(IVDTimerCallback *pCB, uint32 period_ms) {
	return Init2(pCB, period_ms * 10000);
}

bool VDCallbackTimer::Init2(IVDTimerCallback *pCB, uint32 period_100ns) {
	return Init3(pCB, period_100ns, period_100ns >> 1, true);
}

bool VDCallbackTimer::Init3(IVDTimerCallback *pCB, uint32 period_100ns, uint32 /*accuracy_100ns*/, bool precise) {
	Shutdown();

	mpCB = pCB;
	mbExit = false;
	mbPrecise = precise;

	mTimerPeriod = period_100ns;
	mTimerPeriodAdjustment = 0;
	mTimerPeriodDelta = 0;

	return ThreadStart();
}

void VDCallbackTimer::Shutdown() {
	if (isThreadActive()) {
		mbExit = true;
		msigExit.signal();
		ThreadWait();
	}
	mTimerAccuracy = 0;
}

void VDCallbackTimer::SetRateDelta(int delta_100ns) {
	mTimerPeriodDelta = delta_100ns;
}

void VDCallbackTimer::AdjustRate(int adjustment_100ns) {
	mTimerPeriodAdjustment += adjustment_100ns;
}

bool VDCallbackTimer::IsTimerRunning() const {
	return const_cast<VDCallbackTimer *>(this)->isThreadActive();
}

void VDCallbackTimer::ThreadRun() {
	uint32 timerPeriod = mTimerPeriod;
	uint32 periodHi = timerPeriod / 10000;
	uint32 periodLo = timerPeriod % 10000;
	uint32 nextTimeHi = VDGetAccurateTick() + periodHi;
	uint32 nextTimeLo = periodLo;

	uint32 maxDelay = mTimerPeriod / 2000;

	while (!mbExit) {
		uint32 waitMs;
		if (!mbPrecise) {
			waitMs = periodHi;
		} else {
			uint32 currentTime = VDGetAccurateTick();
			sint32 delta = (sint32)(nextTimeHi - currentTime);

			if (delta > 0)
				waitMs = (uint32)delta > maxDelay ? maxDelay : (uint32)delta;
			else
				waitMs = 0;

			if ((uint32)std::abs(delta) > maxDelay) {
				nextTimeHi = currentTime + periodHi;
				nextTimeLo = periodLo;
			} else {
				nextTimeLo += periodLo;
				nextTimeHi += periodHi;
				if (nextTimeLo >= 10000) {
					nextTimeLo -= 10000;
					++nextTimeHi;
				}
			}
		}

		if (waitMs && msigExit.tryWait(waitMs))
			break;

		if (mbExit)
			break;

		mpCB->TimerCallback();

		int adjust = mTimerPeriodAdjustment.xchg(0);
		int perdelta = mTimerPeriodDelta;

		if (adjust || perdelta) {
			timerPeriod += adjust;
			periodHi = (timerPeriod + perdelta) / 10000;
			periodLo = (timerPeriod + perdelta) % 10000;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

struct VDLazyTimer::Impl {
	std::thread worker;
	std::mutex mutex;
	std::condition_variable cv;
	vdfunction<void()> fn;
	uint32 delayMs = 0;
	bool periodic = false;
	bool stop = false;
	bool running = false;

	~Impl() {
		RequestStop();
	}

	void RequestStop() {
		{
			std::lock_guard<std::mutex> g(mutex);
			stop = true;
		}
		cv.notify_all();
		if (worker.joinable())
			worker.join();
		running = false;
		stop = false;
	}

	void Start(vdfunction<void()> f, uint32 delay, bool isPeriodic) {
		RequestStop();

		std::lock_guard<std::mutex> g(mutex);
		fn = std::move(f);
		delayMs = delay;
		periodic = isPeriodic;
		stop = false;
		running = true;

		worker = std::thread([this] {
			std::unique_lock<std::mutex> lk(mutex);
			for (;;) {
				if (cv.wait_for(lk, std::chrono::milliseconds(delayMs),
						[this] { return stop; }))
					return;

				auto f = fn;
				bool again = periodic;
				lk.unlock();
				if (f)
					f();
				lk.lock();

				if (!again || stop)
					return;
			}
		});
	}
};

VDLazyTimer::VDLazyTimer()
	: mpImpl(new Impl)
{
}

VDLazyTimer::~VDLazyTimer() {
	delete mpImpl;
}

void VDLazyTimer::SetOneShot(IVDTimerCallback *pCB, uint32 delay) {
	SetOneShotFn([=]() { pCB->TimerCallback(); }, delay);
}

void VDLazyTimer::SetOneShotFn(const vdfunction<void()>& fn, uint32 delay) {
	mpImpl->Start(fn, delay, false);
}

void VDLazyTimer::SetPeriodic(IVDTimerCallback *pCB, uint32 delay) {
	SetPeriodicFn([=]() { pCB->TimerCallback(); }, delay);
}

void VDLazyTimer::SetPeriodicFn(const vdfunction<void()>& fn, uint32 delay) {
	mpImpl->Start(fn, delay, true);
}

void VDLazyTimer::Stop() {
	mpImpl->RequestStop();
}
