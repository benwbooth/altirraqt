//	Altirra - Qt port
//	VGM 1.72 writer implementation. Mirrors the upstream
//	src/Altirra/source/vgmwriter.cpp logic but uses plain stdio and
//	writes the GD3 tag as raw UTF-16LE bytes (avoiding the wchar_t
//	width portability issue that keeps upstream's vgmwriter.cpp out of
//	the Linux build).

#include "vgmwriter.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

// Qt's qtmetamacros.h #defines `signals`/`slots` as keywords; vd2/system
// uses `signals` as a parameter name. Undef before including VDCPP.
#undef signals
#undef slots

#include <vd2/system/error.h>
#include <vd2/system/vdtypes.h>
#include <vd2/system/VDString.h>

#include <at/atcore/scheduler.h>
#include <at/ataudio/pokey.h>

#include <irqcontroller.h>
#include <simulator.h>

// IATRegisterWriteLogger requires <vdspan.h>; pull in stl span/iter helpers.
#include <vd2/system/vdstl.h>

class IATPokeyWriteSink : public IATRegisterWriteLogger {
public:
	explicit IATPokeyWriteSink(ATQtVgmRecorder *r) : mpRec(r) {}
	void LogRegisterWrites(vdspan<const ATMemoryWriteLogEntry> entries) override {
		for (const auto& e : entries)
			mpRec->onWrite(e.mCycle, e.mAddress, e.mValue);
	}
private:
	ATQtVgmRecorder *mpRec;
};

namespace {

// 'V''g''m'' '
constexpr uint32 kVgmMagic = 0x206D6756u;

// VGM POKEY commands (1.72 device 0x90):
//   0xBB rr vv  — write 'vv' to POKEY register 'rr'  (single chip)
//   For dual chip support, set bit 30 in the POKEY clock field of the
//   header (0xB0..0xB3) and use rr | 0x80 to address the second chip.
//
// Delays:
//   0x61 nn nn  — wait nn samples (16-bit LE)
//   0x62        — wait 735 samples (NTSC frame)
//   0x63        — wait 882 samples (PAL frame)
//   0x70+n      — wait n+1 samples (1..16)
//   0x66        — end of sound data

} // namespace

ATQtVgmRecorder::ATQtVgmRecorder(ATSimulator& sim, const char *path)
	: mpSim(&sim)
{
	mFile = std::fopen(path, "wb");
	if (!mFile)
		throw std::runtime_error(std::string("Could not open ") + path
		                         + " for writing.");

	const ATPokeyEmulator& pokey = sim.GetPokey();
	mStereo = pokey.IsStereoEnabled();

	// Build a placeholder header, written to disk verbatim now and patched
	// on Finalize().
	std::memset(mHeader, 0, sizeof mHeader);

	// Magic 'Vgm '.
	mHeader[0] = 'V'; mHeader[1] = 'g'; mHeader[2] = 'm'; mHeader[3] = ' ';

	// Version 1.72.
	mHeader[8]  = 0x72;
	mHeader[9]  = 0x01;
	mHeader[10] = 0x00;
	mHeader[11] = 0x00;

	// VGM data offset (relative to its field at 0x34) — body starts at 0x100.
	const uint32 dataOff = 0x100u - 0x34u;
	mHeader[0x34] = (uint8)(dataOff & 0xff);
	mHeader[0x35] = (uint8)((dataOff >> 8) & 0xff);
	mHeader[0x36] = (uint8)((dataOff >> 16) & 0xff);
	mHeader[0x37] = (uint8)((dataOff >> 24) & 0xff);

	// POKEY clock and dual-chip bit (bit 30) at 0xB0.
	const double pokeyClock = sim.GetScheduler()->GetRate().asDouble();
	uint32 clockField = (uint32)(pokeyClock + 0.5);
	if (mStereo) clockField |= 1u << 30;
	mHeader[0xB0] = (uint8)(clockField & 0xff);
	mHeader[0xB1] = (uint8)((clockField >> 8) & 0xff);
	mHeader[0xB2] = (uint8)((clockField >> 16) & 0xff);
	mHeader[0xB3] = (uint8)((clockField >> 24) & 0xff);

	// Write provisional header.
	std::fwrite(mHeader, 1, sizeof mHeader, mFile);

	// Initialise sample-rate conversion factor (32.32 fixed point).
	if (pokeyClock > 0.0) {
		const double ratio = 44100.0 / pokeyClock;
		mSamplesPerCycleF32 = (uint32)(ratio * 4294967296.0 + 0.5);
	}

	// Snapshot current POKEY registers as the baseline for delta encoding.
	ATPokeyRegisterState rstate {};
	pokey.GetRegisterState(rstate);
	std::memcpy(mPrev, rstate.mReg, sizeof mPrev);
	std::memcpy(mNext, mPrev, sizeof mNext);

	mLastCycle = sim.GetScheduler()->GetTick();

	// Hot-start recording if any voice is already audible — saves a chunk
	// of leading silence.
	uint8 vols = rstate.mReg[1] | rstate.mReg[3] | rstate.mReg[5] | rstate.mReg[7];
	if (mStereo)
		vols |= rstate.mReg[0x11] | rstate.mReg[0x13]
		      | rstate.mReg[0x15] | rstate.mReg[0x17];
	if (vols & 0x0F) mRecordingStarted = true;

	// Hook the simulator's POKEY register write logger.
	mpSink = new IATPokeyWriteSink(this);
	sim.SetPokeyWriteLogger(mpSink);
}

ATQtVgmRecorder::~ATQtVgmRecorder() {
	// Detach from simulator.
	if (mpSim) {
		mpSim->SetPokeyWriteLogger(nullptr);
		mpSim = nullptr;
	}
	delete mpSink;
	mpSink = nullptr;

	if (mFile && !mFinalized) {
		mFinalized = true;

		// End-of-data marker.
		writeByte(0x66);

		// Empty GD3 tag at this point. mBytesWrittenAfterHeader is the
		// offset into the body. Total file offset of the GD3 chunk = 0x100
		// + mBytesWrittenAfterHeader. The header field at 0x14 is
		// "GD3 offset relative to itself" = (file offset of GD3) - 0x14.
		const uint32 gd3FileOff = 0x100u + mBytesWrittenAfterHeader;
		const uint32 gd3RelOff  = gd3FileOff - 0x14u;

		// Build empty GD3: 11 wide-string fields all "" (just a single L'\0').
		// Ten separators + final terminator = 11 null shorts = 22 bytes.
		// Each empty string = 1 wide null = 2 bytes. 11 * 2 = 22 bytes.
		const uint32 gd3DataLen = 11u * 2u;
		std::fwrite("Gd3 ", 1, 4, mFile);
		writeLE32(0x0100u);         // GD3 version 1.00
		writeLE32(gd3DataLen);
		for (uint32 i = 0; i < 11; ++i) {
			writeByte(0x00);
			writeByte(0x00);
		}
		// Total file size now.
		std::fflush(mFile);
		const long fileEnd = std::ftell(mFile);

		// Patch header fields:
		//   0x04 (eof_offset): file size - 4
		//   0x14 (gd3_offset): gd3FileOff - 0x14
		//   0x18 (total_samples)
		const uint32 eofOff = (uint32)(fileEnd - 4);
		auto setLE32 = [this](uint32 off, uint32 v) {
			mHeader[off + 0] = (uint8)(v & 0xff);
			mHeader[off + 1] = (uint8)((v >> 8) & 0xff);
			mHeader[off + 2] = (uint8)((v >> 16) & 0xff);
			mHeader[off + 3] = (uint8)((v >> 24) & 0xff);
		};
		setLE32(0x04, eofOff);
		setLE32(0x14, gd3RelOff);
		setLE32(0x18, mSampleCount);

		// Rewrite header.
		std::fseek(mFile, 0, SEEK_SET);
		std::fwrite(mHeader, 1, sizeof mHeader, mFile);
		std::fseek(mFile, fileEnd, SEEK_SET);
		std::fclose(mFile);
		mFile = nullptr;
	}
}

void ATQtVgmRecorder::writeByte(uint8 b) {
	std::fwrite(&b, 1, 1, mFile);
	++mBytesWrittenAfterHeader;
}
void ATQtVgmRecorder::writeLE16(uint16 v) {
	uint8 b[2] = { (uint8)(v & 0xff), (uint8)(v >> 8) };
	std::fwrite(b, 1, 2, mFile);
	mBytesWrittenAfterHeader += 2;
}
void ATQtVgmRecorder::writeLE32(uint32 v) {
	uint8 b[4] = {
		(uint8)(v & 0xff), (uint8)((v >> 8) & 0xff),
		(uint8)((v >> 16) & 0xff), (uint8)((v >> 24) & 0xff),
	};
	std::fwrite(b, 1, 4, mFile);
	mBytesWrittenAfterHeader += 4;
}
void ATQtVgmRecorder::writeBytes(const void *p, size_t n) {
	std::fwrite(p, 1, n, mFile);
	mBytesWrittenAfterHeader += (uint32)n;
}

void ATQtVgmRecorder::emitDelay(uint32 samples) {
	while (samples >= 65535) {
		writeByte(0x61);
		writeByte(0xff);
		writeByte(0xff);
		samples -= 65535;
	}
	if (samples) {
		if (samples == 735) {
			writeByte(0x62);
		} else if (samples == 882) {
			writeByte(0x63);
		} else if (samples <= 16) {
			writeByte((uint8)(0x6F + samples));
		} else {
			writeByte(0x61);
			writeByte((uint8)(samples & 0xff));
			writeByte((uint8)((samples >> 8) & 0xff));
		}
	}
}

void ATQtVgmRecorder::flushRegisterChanges() {
	const uint32 mask = mStereo ? 0x1Fu : 0x0Fu;

	if (mInitialPending) {
		// Force every relevant register to be considered "changed" the
		// first time we have a real delay so the output starts from a
		// known state.
		mInitialPending = false;
		for (uint32 i = 0; i < 32; ++i) {
			if ((i & 0x0F) > 0x08 && (i & 0x0F) != 0x0F) continue;
			if (!mStereo && i >= 0x10) break;
			mPrev[i] = ~mNext[i];
		}
	}

	for (uint32 i = 0; i < 32; ++i) {
		if (!mStereo && i >= 0x10) break;
		const uint32 lo = i & 0x0F;
		// Only emit AUDF1..4, AUDC1..4, AUDCTL, SKCTL.
		if (lo > 0x08 && lo != 0x0F) continue;
		if (mPrev[i] == mNext[i]) continue;
		mPrev[i] = mNext[i];

		// VGM POKEY (device 0x90) command 0xBB rr vv. For the second chip
		// in a dual config, set bit 7 of the register byte.
		uint8 reg = (uint8)(i & mask);
		if (i & 0x10) reg |= 0x80;
		writeByte(0xBB);
		writeByte(reg);
		writeByte(mNext[i]);
	}
}

void ATQtVgmRecorder::onWrite(uint32 cycle, uint16 address, uint8 value) {
	if (mFinalized || !mFile) return;

	const uint32 lo = address & 0x0F;
	// Filter to audio registers only.
	switch (lo) {
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x04: case 0x05: case 0x06: case 0x07:
		case 0x08: case 0x0F:
			break;
		default:
			return;
	}

	const uint32 mask = mStereo ? 0x1Fu : 0x0Fu;
	const uint32 reg  = address & mask;

	// Cycle-delta -> sample-delta accumulation.
	const uint32 dcyc = cycle - mLastCycle;
	if (dcyc) {
		mLastCycle = cycle;
		mSampleAccumF32 += (uint64)dcyc * mSamplesPerCycleF32;
		uint32 dsamples = (uint32)(mSampleAccumF32 >> 32);
		mSampleAccumF32 &= 0xFFFFFFFFu;

		if (dsamples && mRecordingStarted) {
			flushRegisterChanges();
			mSampleCount += dsamples;
			emitDelay(dsamples);
		}
	}

	// Hot-start once a voice goes audible. AUDC* registers are at lo==0x01,
	// 0x03, 0x05, 0x07. Bottom nibble is volume (0..15).
	if (!mRecordingStarted) {
		if ((lo & 0x09) == 0x01 && (value & 0x0F)) {
			mRecordingStarted = true;
			mLastCycle = cycle;
			mSampleAccumF32 = 0;
		}
	}

	mNext[reg] = value;
}
