//	Altirra - Qt port
//	Qt-backed IVDAudioOutput minidriver. Uses QAudioSink to play 16-bit
//	stereo PCM produced by ATAudioOutput's resampler/mixer.
//	Copyright (C) 2026 Ben Booth
//
//	GPL-2.0-or-later (matches Altirra)
//
//	Replaces audiooutnull.cpp's no-op factories. The four
//	VDCreateAudioOutput*W32() entry points all return the same Qt sink — the
//	upstream `mActiveApi` distinction is meaningful only on Windows; here
//	there's just one host audio path.

#include <stdafx.h>

#include <atomic>
#include <cstdint>
#include <cstring>

#include <QAudioFormat>
#include <QAudioSink>
#include <QByteArray>
#include <QIODevice>
#include <QMediaDevices>

// Qt's qtmetamacros.h #defines `signals` as `public QT_ANNOTATE_ACCESS_SPECIFIER(qt_signal)`,
// which collides with VDThread::waitMultiple's `signals` parameter name in
// vd2/system/thread.h. Undef before pulling in any VDCPP headers that touch
// thread/signal types.
#undef signals
#undef slots

#include <vd2/system/time.h>
#include <at/ataudio/audioout.h>

namespace {

class ATAudioOutputQt final : public IVDAudioOutput {
public:
	uint32 GetPreferredSamplingRate(const wchar_t *) const override { return 48000; }

	bool Init(uint32 bufsize, uint32 bufcount, const tWAVEFORMATEX *wf, const wchar_t *) override {
		if (wf) {
			mSamplingRate = wf->nSamplesPerSec;
			mChannels     = wf->nChannels ? wf->nChannels : 2;
			mBitsPerSample = wf->wBitsPerSample ? wf->wBitsPerSample : 16;
			mBlockAlign   = wf->nBlockAlign ? wf->nBlockAlign : (mChannels * mBitsPerSample / 8);
		}

		QAudioFormat fmt;
		fmt.setSampleRate((int)mSamplingRate);
		fmt.setChannelCount((int)mChannels);
		fmt.setSampleFormat(mBitsPerSample == 16 ? QAudioFormat::Int16 : QAudioFormat::UInt8);

		const QAudioDevice dev = QMediaDevices::defaultAudioOutput();
		if (dev.isNull() || !dev.isFormatSupported(fmt))
			return false;

		mpSink = new QAudioSink(dev, fmt);
		mpSink->setBufferSize((qsizetype)(bufsize * bufcount));
		mpIO = mpSink->start();
		mStartTick = VDGetPreciseTick();

		return mpIO != nullptr;
	}

	void Shutdown() override {
		if (mpSink) {
			mpSink->stop();
			delete mpSink;
			mpSink = nullptr;
			mpIO = nullptr;
		}
	}

	void GoSilent() override { mSilent = true; }
	bool IsSilent() override { return mSilent; }
	bool IsFrozen() override { return false; }

	uint32 GetAvailSpace() override {
		// Conservative: report room for a full buffer's worth of samples.
		return mpSink ? (uint32)mpSink->bytesFree() : (uint32)(1 << 20);
	}

	uint32 GetBufferLevel() override {
		// Bytes currently queued (buffer size - free).
		if (!mpSink) return 0;
		return (uint32)(mpSink->bufferSize() - mpSink->bytesFree());
	}

	uint32 EstimateHWBufferLevel(bool *underflowDetected) override {
		if (underflowDetected) *underflowDetected = false;
		return GetBufferLevel();
	}

	sint32 GetPosition() override     { return (sint32)(GetPositionTime() * 1000.0); }
	sint32 GetPositionBytes() override {
		return (sint32)(GetPositionTime() * (double)(mSamplingRate * mBlockAlign));
	}

	double GetPositionTime() override {
		const double elapsed = (double)(VDGetPreciseTick() - mStartTick) * VDGetPreciseSecondsPerTick();
		return elapsed;
	}

	uint32 GetMixingRate() const override { return mSamplingRate; }

	bool Start() override {
		if (mpSink) mpSink->resume();
		return true;
	}

	bool Stop() override {
		if (mpSink) mpSink->suspend();
		return true;
	}

	bool Flush() override {
		// QAudioSink has no explicit flush; samples drain naturally.
		return true;
	}

	bool Write(const void *data, uint32 len) override {
		if (!mpIO || mSilent) return true;
		const qsizetype written = mpIO->write((const char *)data, (qsizetype)len);
		// Best-effort: drop on overrun rather than block POKEY's timer-driven thread.
		(void)written;
		return true;
	}

	bool Finalize(uint32) override { return true; }

private:
	uint32 mSamplingRate  = 48000;
	uint32 mChannels      = 2;
	uint32 mBitsPerSample = 16;
	uint32 mBlockAlign    = 4;
	uint64 mStartTick     = 0;
	bool   mSilent        = false;

	QAudioSink *mpSink = nullptr;
	QIODevice  *mpIO   = nullptr;
};

} // namespace

IVDAudioOutput *VDCreateAudioOutputWaveOutW32()     { return new ATAudioOutputQt; }
IVDAudioOutput *VDCreateAudioOutputDirectSoundW32() { return new ATAudioOutputQt; }
IVDAudioOutput *VDCreateAudioOutputXAudio2W32()     { return new ATAudioOutputQt; }
IVDAudioOutput *VDCreateAudioOutputWASAPIW32()      { return new ATAudioOutputQt; }
