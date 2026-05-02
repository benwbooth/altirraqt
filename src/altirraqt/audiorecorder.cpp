//	Altirra - Qt port
//	Audio recorder implementation.

#include "audiorecorder.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

void writeLE16(std::FILE *f, uint16 v) {
	uint8 b[2] = { (uint8)(v & 0xff), (uint8)(v >> 8) };
	std::fwrite(b, 1, 2, f);
}
void writeLE32(std::FILE *f, uint32 v) {
	uint8 b[4] = {
		(uint8)(v & 0xff), (uint8)((v >> 8) & 0xff),
		(uint8)((v >> 16) & 0xff), (uint8)((v >> 24) & 0xff)
	};
	std::fwrite(b, 1, 4, f);
}

// Float [-1,1] → int16 with clipping.
int16 floatToS16(float v) {
	const float scaled = v * 32767.0f;
	if (scaled >= 32767.0f) return 32767;
	if (scaled <= -32768.0f) return -32768;
	return (int16)scaled;
}

} // namespace

ATAudioRecorder::ATAudioRecorder(const char *path, uint32 sampleRate, bool stereo,
                                 bool writeHeader)
	: mSampleRate(sampleRate)
	, mStereo(stereo)
	, mWriteHeader(writeHeader)
{
	mFile = std::fopen(path, "wb");
	if (!mFile) {
		throw std::runtime_error(std::string("Could not open ") + path
		                         + " for writing.");
	}

	if (!writeHeader) return;

	const uint16 numChannels = stereo ? 2 : 1;
	const uint16 blockAlign  = (uint16)(numChannels * 2);
	const uint32 byteRate    = sampleRate * blockAlign;

	// 16-bit PCM. Sizes (RIFF, data) are zero now and patched in Finalize().
	std::fwrite("RIFF", 1, 4, mFile);
	writeLE32(mFile, 0);                         // chunk size — patched
	std::fwrite("WAVE", 1, 4, mFile);
	std::fwrite("fmt ", 1, 4, mFile);
	writeLE32(mFile, 16);                        // PCM fmt size
	writeLE16(mFile, 1);                         // PCM
	writeLE16(mFile, numChannels);
	writeLE32(mFile, sampleRate);                // sample rate
	writeLE32(mFile, byteRate);                  // byte rate
	writeLE16(mFile, blockAlign);
	writeLE16(mFile, 16);                        // bits per sample
	std::fwrite("data", 1, 4, mFile);
	writeLE32(mFile, 0);                         // data size — patched
}

ATAudioRecorder::~ATAudioRecorder() {
	if (!mFinalized) Finalize();
}

void ATAudioRecorder::Finalize() {
	if (mFinalized || !mFile) return;
	mFinalized = true;

	if (mWriteHeader) {
		const uint32 bytesPerFrame = mStereo ? 4u : 2u;
		const uint32 dataBytes = (uint32)(mFramesWritten * bytesPerFrame);
		const uint32 riffBytes = 36 + dataBytes;            // header minus the first 8

		std::fflush(mFile);
		std::fseek(mFile, 4, SEEK_SET);
		writeLE32(mFile, riffBytes);
		std::fseek(mFile, 40, SEEK_SET);
		writeLE32(mFile, dataBytes);
	}
	std::fclose(mFile);
	mFile = nullptr;
}

void ATAudioRecorder::WriteRawAudio(const float *left, const float *right,
                                    uint32 count, uint32 /*timestamp*/) {
	if (!mFile || !left || count == 0 || mPaused) return;

	// Convert in 256-sample chunks to keep the stack buffer small.
	int16 buf[512]; // 256 frames × up to 2 channels
	uint32 i = 0;
	while (i < count) {
		const uint32 chunk = std::min<uint32>(count - i, 256);
		if (mStereo) {
			for (uint32 k = 0; k < chunk; ++k) {
				const float l = left[i + k];
				const float r = right ? right[i + k] : l;
				buf[2 * k]     = floatToS16(l);
				buf[2 * k + 1] = floatToS16(r);
			}
			std::fwrite(buf, sizeof(int16), chunk * 2, mFile);
		} else {
			for (uint32 k = 0; k < chunk; ++k) {
				const float l = left[i + k];
				const float mono = right ? 0.5f * (l + right[i + k]) : l;
				buf[k] = floatToS16(mono);
			}
			std::fwrite(buf, sizeof(int16), chunk, mFile);
		}
		i += chunk;
	}
	mFramesWritten += count;
}
