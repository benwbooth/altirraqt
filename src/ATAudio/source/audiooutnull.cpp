//	Altirra - Atari 800/800XL/5200 emulator
//	Null audio output backend
//	Copyright (C) 2024 Ben Booth (Qt port)
//
//	Minimal silent backend that stands in for the Win32 WaveOut/WASAPI/XAudio2/
//	DirectSound outputs used upstream. ATAudio calls these factory functions
//	from audiooutput.cpp; a real Qt backend can be swapped in later. For now
//	the emulator runs without producing sound through ATAudio.

#include <stdafx.h>
#include <vd2/system/time.h>
#include <at/ataudio/audioout.h>

namespace {

class ATAudioOutputNull final : public IVDAudioOutput {
public:
	uint32 GetPreferredSamplingRate(const wchar_t *) const override { return 48000; }

	bool Init(uint32 bufsize, uint32 bufcount, const tWAVEFORMATEX *wf, const wchar_t *) override {
		if (wf) {
			mSamplingRate = wf->nSamplesPerSec;
			mBlockAlign = wf->nBlockAlign ? wf->nBlockAlign : 4;
		}
		mStartTick = VDGetPreciseTick();
		return true;
	}

	void Shutdown() override {}
	void GoSilent() override { mSilent = true; }

	bool IsSilent() override { return mSilent; }
	bool IsFrozen() override { return false; }
	uint32 GetAvailSpace() override { return 1 << 20; }
	uint32 GetBufferLevel() override { return 0; }
	uint32 EstimateHWBufferLevel(bool *underflowDetected) override {
		if (underflowDetected) *underflowDetected = false;
		return 0;
	}

	sint32 GetPosition() override {
		return (sint32)(GetPositionTime() * 1000.0);
	}

	sint32 GetPositionBytes() override {
		return (sint32)(GetPositionTime() * (double)(mSamplingRate * mBlockAlign));
	}

	double GetPositionTime() override {
		const double elapsed = (double)(VDGetPreciseTick() - mStartTick) * VDGetPreciseSecondsPerTick();
		return elapsed;
	}

	uint32 GetMixingRate() const override { return mSamplingRate; }

	bool Start() override { return true; }
	bool Stop() override { return true; }
	bool Flush() override { return true; }

	bool Write(const void *, uint32) override { return true; }
	bool Finalize(uint32) override { return true; }

private:
	uint32 mSamplingRate = 48000;
	uint32 mBlockAlign = 4;
	uint64 mStartTick = 0;
	bool mSilent = false;
};

} // namespace

IVDAudioOutput *VDCreateAudioOutputWaveOutW32()     { return new ATAudioOutputNull; }
IVDAudioOutput *VDCreateAudioOutputDirectSoundW32() { return new ATAudioOutputNull; }
IVDAudioOutput *VDCreateAudioOutputXAudio2W32()     { return new ATAudioOutputNull; }
IVDAudioOutput *VDCreateAudioOutputWASAPIW32()      { return new ATAudioOutputNull; }
