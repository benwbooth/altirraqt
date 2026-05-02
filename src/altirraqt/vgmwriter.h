//	Altirra - Qt port
//	VGM (Video Game Music) writer for POKEY register streams. Implements
//	the IATRegisterWriteLogger interface so it receives every POKEY
//	register write at cycle precision; converts cycle deltas into 44100
//	Hz sample-tick units for the VGM container, and emits POKEY register
//	writes (VGM cmd 0xBB) with delta-only encoding.
//
//	Output is a VGM 1.72 file with an empty GD3 tag. Foobar2000 (with
//	the foo_input_vgmstream component or libvgm-based plugin) and
//	libvgm-based players (vgmplay, libvgm-tools) decode it natively.

#pragma once

#include <cstdio>
#include <vector>

#include <vd2/system/vdtypes.h>

class ATSimulator;
class IATPokeyWriteSink;

class ATQtVgmRecorder {
public:
	// Open `path` for writing and start hooking POKEY register writes
	// from `sim`. Throws std::runtime_error on file-open failure.
	ATQtVgmRecorder(ATSimulator& sim, const char *path);
	~ATQtVgmRecorder();

	ATQtVgmRecorder(const ATQtVgmRecorder&) = delete;
	ATQtVgmRecorder& operator=(const ATQtVgmRecorder&) = delete;

	// Internal — invoked by the IATRegisterWriteLogger sink.
	void onWrite(uint32 cycle, uint16 address, uint8 value);

private:
	void writeByte(uint8 b);
	void writeLE16(uint16 v);
	void writeLE32(uint32 v);
	void writeBytes(const void *p, size_t n);
	void emitDelay(uint32 samples);
	void flushRegisterChanges();

	ATSimulator *mpSim = nullptr;
	IATPokeyWriteSink *mpSink = nullptr;
	std::FILE *mFile = nullptr;
	bool mFinalized = false;
	bool mStereo = false;

	uint32 mLastCycle = 0;
	uint64 mSampleAccumF32 = 0;     // upper 32 bits = whole samples carry
	uint32 mSamplesPerCycleF32 = 0; // 32.32 fixed-point samples-per-cycle
	uint32 mSampleCount = 0;
	uint32 mBytesWrittenAfterHeader = 0; // counts post-header bytes
	bool   mRecordingStarted = false;
	bool   mInitialPending = true;

	uint8 mPrev[32] {};
	uint8 mNext[32] {};

	// Header buffer (256 bytes). We rewrite this on Finalize().
	uint8 mHeader[256] {};
};
