//	Altirra - Qt port
//	Audio recorder: writes the simulator's POKEY/audio-mix output to a
//	16-bit stereo PCM WAV file. Tapped via IATAudioTap on the audio
//	output, which feeds samples at the audio output's native sampling
//	rate.

#pragma once

#include <cstdio>

#include <vd2/system/vdtypes.h>
#include <at/ataudio/audiooutput.h>

class ATAudioRecorder final : public IATAudioTap {
public:
	// Open `path` for writing as a WAV with the given sample rate; throws
	// std::runtime_error on failure. When `stereo` is false, the recorder
	// downmixes left/right into a single mono channel and writes a mono
	// WAV header. Must call Finalize() before destruction for the WAV
	// header sizes to be correct (the destructor calls it defensively if
	// you forget).
	//
	// `writeHeader == false` produces a headerless raw PCM file: just
	// interleaved int16 samples (little-endian) at the given sample rate
	// and channel count. Useful for the Record Raw Audio menu item — the
	// resulting file can be imported into Audacity / sox by specifying
	// "raw" and the same sample rate and channels.
	ATAudioRecorder(const char *path, uint32 sampleRate, bool stereo = true,
	                bool writeHeader = true);
	~ATAudioRecorder();

	void Finalize();

	// While paused, WriteRawAudio drops samples on the floor so the WAV
	// has a continuous timeline without the inactive interval.
	void Pause()  { mPaused = true; }
	void Resume() { mPaused = false; }
	bool IsPaused() const { return mPaused; }

	// IATAudioTap. left/right are float buffers of `count` samples each.
	// `right` may be null for mono input — the recorder still writes a
	// stereo WAV by duplicating the left channel.
	void WriteRawAudio(const float *left, const float *right,
	                   uint32 count, uint32 timestamp) override;

	// Number of sample frames written (i.e. left+right pairs).
	uint64 GetFramesWritten() const { return mFramesWritten; }

private:
	std::FILE *mFile;
	uint32     mSampleRate;
	bool       mStereo;
	bool       mWriteHeader = true;
	uint64     mFramesWritten = 0;
	bool       mFinalized = false;
	bool       mPaused = false;
};
