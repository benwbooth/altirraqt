//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2004 Avery Lee, All Rights Reserved.
//
//	Cross-platform Qt port: the original header pulled in win32/miniwindows.h
//	and wrapped Win32's multimedia timer + thunk-based SetTimer callback.
//	We keep the same API but rebuild it on std::chrono + std::thread.

#ifndef f_VD2_SYSTEM_TIME_H
#define f_VD2_SYSTEM_TIME_H

#include <vd2/system/vdtypes.h>
#include <vd2/system/atomic.h>
#include <vd2/system/function.h>
#include <vd2/system/thread.h>

#include <atomic>

class VDFunctionThunkInfo;

uint32 VDGetCurrentTick();
uint64 VDGetCurrentTick64();

uint64 VDGetPreciseTick();
uint64 VDGetPreciseTicksPerSecondI();
double VDGetPreciseTicksPerSecond();
double VDGetPreciseSecondsPerTick();

uint32 VDGetAccurateTick();

class VDINTERFACE IVDTimerCallback {
public:
	virtual void TimerCallback() = 0;
};

class VDCallbackTimer : private VDThread {
public:
	VDCallbackTimer();
	~VDCallbackTimer();

	bool Init(IVDTimerCallback *pCB, uint32 period_ms);
	bool Init2(IVDTimerCallback *pCB, uint32 period_100ns);
	bool Init3(IVDTimerCallback *pCB, uint32 period_100ns, uint32 accuracy_100ns, bool precise);
	void Shutdown();

	void SetRateDelta(int delta_100ns);
	void AdjustRate(int adjustment_100ns);

	bool IsTimerRunning() const;

private:
	void ThreadRun() override;

	IVDTimerCallback *mpCB;
	unsigned		mTimerAccuracy;
	uint32			mTimerPeriod;
	VDAtomicInt		mTimerPeriodDelta;
	VDAtomicInt		mTimerPeriodAdjustment;

	VDSignal		msigExit;

	std::atomic<bool>	mbExit;
	bool			mbPrecise;
};


class VDLazyTimer {
	VDLazyTimer(const VDLazyTimer&) = delete;
	VDLazyTimer& operator=(const VDLazyTimer&) = delete;
public:
	VDLazyTimer();
	~VDLazyTimer();

	void SetOneShot(IVDTimerCallback *pCB, uint32 delay);
	void SetOneShotFn(const vdfunction<void()>& fn, uint32 delay);
	void SetPeriodic(IVDTimerCallback *pCB, uint32 delay);
	void SetPeriodicFn(const vdfunction<void()>& fn, uint32 delay);
	void Stop();

private:
	struct Impl;
	Impl *mpImpl = nullptr;
};

#endif
