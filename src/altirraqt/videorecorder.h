//	Altirra - Qt port
//	Video recorder: writes the simulator's display frame stream to an
//	AVI file. Two encodings are offered:
//	  * Uncompressed BI_RGB (24bpp) — fully decoded by VLC, mpv, ffmpeg.
//	    No external library; large files.
//	  * MJPEG ('MJPG' fourcc) — compresses each frame as a baseline JPEG
//	    via Qt's built-in JPEG encoder. Plays in VLC and mpv.
//
//	The writer maintains an in-memory index that's appended as an idx1
//	chunk on Finalize(). Audio is omitted from the AVI in this first
//	pass — users wanting synchronised audio can record a parallel WAV
//	through ATAudioRecorder.

#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include <vd2/system/vdtypes.h>

class QImage;

class ATVideoRecorder {
public:
	enum class Codec {
		Uncompressed,   // BI_RGB 24bpp DIB-style frames
		MJPEG,          // Baseline JPEG per frame
	};

	// Open `path` for writing. `width`/`height` are the pixel size of
	// every frame written through WriteFrame(); incoming images are
	// rescaled to this size if they don't match. `frameRate` is the
	// display refresh rate (typically 50, 59.92 or 60). `jpegQuality`
	// is in [1..100] and only used for MJPEG. Throws std::runtime_error
	// on open failure.
	ATVideoRecorder(const char *path, int width, int height,
	                double frameRate, Codec codec,
	                int jpegQuality = 85);
	~ATVideoRecorder();

	// Append `image` as the next frame. The image is converted to the
	// recorder's pixel format (RGB888 for uncompressed, JPEG-encoded
	// for MJPEG) and written. Silently no-ops if Finalize has been
	// called or the file failed to open.
	void WriteFrame(const QImage& image);

	// Patch up the AVI header sizes and write the index. Idempotent;
	// the destructor calls it defensively if you forget.
	void Finalize();

	bool IsOpen() const { return mFile != nullptr && !mFinalized; }
	uint64 GetFramesWritten() const { return mFrameCount; }

private:
	void writeChunkHeader(const char fourcc[4], uint32 size);
	void appendIndexEntry(const char fourcc[4], uint32 offsetFromMovi,
	                      uint32 size, bool keyframe);

	std::FILE *mFile = nullptr;
	int mWidth;
	int mHeight;
	double mFrameRate;
	Codec mCodec;
	int mJpegQuality;
	uint64 mFrameCount = 0;
	uint64 mTotalSampleBytes = 0; // sum of frame payload sizes
	uint32 mMaxChunkSize = 0;     // largest single frame chunk
	bool mFinalized = false;

	// File offsets we'll patch up on finalise.
	long mRiffSizeOffset      = 0;
	long mAvihTotalFramesOff  = 0;
	long mAvihMaxBytesPerSec  = 0;
	long mAvihSuggBufSizeOff  = 0;
	long mStrhLengthOff       = 0;
	long mStrfWidthOff        = 0;
	long mStrfHeightOff       = 0;
	long mStrhSuggBufSizeOff  = 0;
	long mMoviChunkSizeOff    = 0;
	long mMoviStartOff        = 0;

	struct IndexEntry {
		char     fourcc[4];
		uint32   flags;          // AVIIF_KEYFRAME etc.
		uint32   chunkOffset;    // offset from start of movi list (data position)
		uint32   chunkSize;
	};
	std::vector<IndexEntry> mIndex;
};
