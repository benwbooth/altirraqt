//	Altirra - Qt port
//	Video recorder implementation: writes a Microsoft AVI 1.0 (RIFF)
//	container.
//
//	Layout (single-stream video, no OpenDML):
//	  RIFF 'AVI '
//	    LIST 'hdrl'
//	      'avih' MainAVIHeader (56 bytes)
//	      LIST 'strl'
//	        'strh' AVIStreamHeader (56 bytes)
//	        'strf' BITMAPINFOHEADER (40 bytes)
//	    LIST 'movi'
//	      ['00db' | '00dc'] frame data...
//	    'idx1' AVIOLDINDEX (16 bytes per frame)
//
//	BI_RGB frames are stored bottom-up 24bpp BGR with rows padded to
//	multiples of 4 bytes — the standard Windows DIB layout. MJPEG frames
//	are written as ordinary JPEG byte streams.

#include "videorecorder.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <QBuffer>
#include <QByteArray>
#include <QImage>

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
void writeLE32At(std::FILE *f, long off, uint32 v) {
	std::fflush(f);
	const long here = std::ftell(f);
	std::fseek(f, off, SEEK_SET);
	writeLE32(f, v);
	std::fseek(f, here, SEEK_SET);
}
void writeFourcc(std::FILE *f, const char fourcc[4]) {
	std::fwrite(fourcc, 1, 4, f);
}

constexpr uint32 kAVIIF_KEYFRAME = 0x00000010;

} // namespace

ATVideoRecorder::ATVideoRecorder(const char *path, int width, int height,
                                  double frameRate, Codec codec,
                                  int jpegQuality)
	: mWidth(std::max(2, width & ~1)) // even; some decoders are picky
	, mHeight(std::max(2, height & ~1))
	, mFrameRate(frameRate > 0 ? frameRate : 60.0)
	, mCodec(codec)
	, mJpegQuality(std::clamp(jpegQuality, 1, 100))
{
	mFile = std::fopen(path, "wb");
	if (!mFile) {
		throw std::runtime_error(std::string("Could not open ") + path
		                         + " for writing.");
	}

	// Approximate fixed-point frame rate as a rational number
	// (microsecondsPerFrame, dwRate/dwScale). VLC/mpv accept
	// dwRate=round(rate*1000), dwScale=1000.
	const uint32 dwScale = 1000;
	const uint32 dwRate  = (uint32)(mFrameRate * 1000.0 + 0.5);
	const uint32 microsPerFrame = (uint32)(1000000.0 / mFrameRate + 0.5);

	const char *codecFourcc = (codec == Codec::MJPEG) ? "MJPG" : "DIB ";
	const uint32 biCompression = (codec == Codec::MJPEG)
	                              ? 0x47504A4Du /*'MJPG'*/ : 0u; /*BI_RGB*/
	const uint32 biBitCount    = (codec == Codec::MJPEG) ? 24u : 24u;

	// RIFF header.
	writeFourcc(mFile, "RIFF");
	mRiffSizeOffset = std::ftell(mFile);
	writeLE32(mFile, 0);                                // patched in Finalize
	writeFourcc(mFile, "AVI ");

	// LIST 'hdrl' — main header + stream list.
	// Compute hdrl LIST size up front: 4 ('hdrl' fourcc)
	//   + 8 + 56 (avih)
	//   + 8 + (4 + 8 + 56 + 8 + 40) (strl LIST: type + strh + strf BMI)
	const uint32 strlPayload = 4 + (8 + 56) + (8 + 40);
	const uint32 hdrlPayload = 4 + (8 + 56) + (8 + strlPayload);
	writeFourcc(mFile, "LIST");
	writeLE32(mFile, hdrlPayload);
	writeFourcc(mFile, "hdrl");

	// 'avih' MainAVIHeader.
	writeFourcc(mFile, "avih");
	writeLE32(mFile, 56);
	writeLE32(mFile, microsPerFrame);                   // dwMicroSecPerFrame
	mAvihMaxBytesPerSec = std::ftell(mFile);
	writeLE32(mFile, 0);                                // dwMaxBytesPerSec — patched
	writeLE32(mFile, 0);                                // dwPaddingGranularity
	writeLE32(mFile, 0x00000910);                       // dwFlags: HASINDEX|ISINTERLEAVED|TRUSTCKTYPE
	mAvihTotalFramesOff = std::ftell(mFile);
	writeLE32(mFile, 0);                                // dwTotalFrames — patched
	writeLE32(mFile, 0);                                // dwInitialFrames
	writeLE32(mFile, 1);                                // dwStreams
	mAvihSuggBufSizeOff = std::ftell(mFile);
	writeLE32(mFile, 0);                                // dwSuggestedBufferSize — patched
	writeLE32(mFile, mWidth);                           // dwWidth
	writeLE32(mFile, mHeight);                          // dwHeight
	writeLE32(mFile, 0);                                // dwReserved[0]
	writeLE32(mFile, 0);                                // dwReserved[1]
	writeLE32(mFile, 0);                                // dwReserved[2]
	writeLE32(mFile, 0);                                // dwReserved[3]

	// LIST 'strl'.
	writeFourcc(mFile, "LIST");
	writeLE32(mFile, strlPayload);
	writeFourcc(mFile, "strl");

	// 'strh' AVIStreamHeader.
	writeFourcc(mFile, "strh");
	writeLE32(mFile, 56);
	writeFourcc(mFile, "vids");                         // fccType
	std::fwrite(codecFourcc, 1, 4, mFile);              // fccHandler
	writeLE32(mFile, 0);                                // dwFlags
	writeLE16(mFile, 0);                                // wPriority
	writeLE16(mFile, 0);                                // wLanguage
	writeLE32(mFile, 0);                                // dwInitialFrames
	writeLE32(mFile, dwScale);                          // dwScale
	writeLE32(mFile, dwRate);                           // dwRate
	writeLE32(mFile, 0);                                // dwStart
	mStrhLengthOff = std::ftell(mFile);
	writeLE32(mFile, 0);                                // dwLength — patched
	mStrhSuggBufSizeOff = std::ftell(mFile);
	writeLE32(mFile, 0);                                // dwSuggestedBufferSize — patched
	writeLE32(mFile, 0xFFFFFFFFu);                      // dwQuality (-1)
	writeLE32(mFile, 0);                                // dwSampleSize
	writeLE16(mFile, 0);                                // rcFrame.left
	writeLE16(mFile, 0);                                // rcFrame.top
	writeLE16(mFile, (uint16)mWidth);                   // rcFrame.right
	writeLE16(mFile, (uint16)mHeight);                  // rcFrame.bottom

	// 'strf' BITMAPINFOHEADER (40 bytes).
	writeFourcc(mFile, "strf");
	writeLE32(mFile, 40);
	writeLE32(mFile, 40);                               // biSize
	mStrfWidthOff = std::ftell(mFile);
	writeLE32(mFile, mWidth);
	mStrfHeightOff = std::ftell(mFile);
	writeLE32(mFile, mHeight);
	writeLE16(mFile, 1);                                // biPlanes
	writeLE16(mFile, (uint16)biBitCount);
	writeLE32(mFile, biCompression);
	// biSizeImage: uncompressed = stride*height; for MJPEG just the max
	// chunk size, patched below.
	const uint32 stride = (uint32)((mWidth * 3 + 3) & ~3);
	writeLE32(mFile, codec == Codec::Uncompressed
	                 ? stride * (uint32)mHeight : 0u);
	writeLE32(mFile, 0);                                // biXPelsPerMeter
	writeLE32(mFile, 0);                                // biYPelsPerMeter
	writeLE32(mFile, 0);                                // biClrUsed
	writeLE32(mFile, 0);                                // biClrImportant

	// LIST 'movi' (data follows; size patched on finalise).
	writeFourcc(mFile, "LIST");
	mMoviChunkSizeOff = std::ftell(mFile);
	writeLE32(mFile, 0);                                // patched in Finalize
	writeFourcc(mFile, "movi");
	mMoviStartOff = std::ftell(mFile);                  // first byte of frame data
}

ATVideoRecorder::~ATVideoRecorder() {
	if (!mFinalized) Finalize();
}

void ATVideoRecorder::writeChunkHeader(const char fourcc[4], uint32 size) {
	std::fwrite(fourcc, 1, 4, mFile);
	writeLE32(mFile, size);
}

void ATVideoRecorder::appendIndexEntry(const char fourcc[4],
                                        uint32 offsetFromMovi,
                                        uint32 size, bool keyframe) {
	IndexEntry e;
	std::memcpy(e.fourcc, fourcc, 4);
	e.flags       = keyframe ? kAVIIF_KEYFRAME : 0;
	e.chunkOffset = offsetFromMovi;
	e.chunkSize   = size;
	mIndex.push_back(e);
}

void ATVideoRecorder::WriteFrame(const QImage& imageIn) {
	if (!mFile || mFinalized || imageIn.isNull()) return;

	// Normalise size and format. We always work in RGB888 internally.
	QImage img = imageIn;
	if (img.width() != mWidth || img.height() != mHeight)
		img = img.scaled(mWidth, mHeight, Qt::IgnoreAspectRatio,
		                 Qt::SmoothTransformation);
	if (img.format() != QImage::Format_RGB888)
		img = img.convertToFormat(QImage::Format_RGB888);

	if (mCodec == Codec::Uncompressed) {
		// Pad rows to 4-byte stride and emit BGR bottom-up.
		const uint32 strideOut = (uint32)((mWidth * 3 + 3) & ~3);
		const uint32 payload   = strideOut * (uint32)mHeight;
		const long chunkStart  = std::ftell(mFile);
		writeChunkHeader("00db", payload);
		std::vector<uint8> row(strideOut, 0);
		for (int y = mHeight - 1; y >= 0; --y) {
			const uchar *src = img.constScanLine(y);
			for (int x = 0; x < mWidth; ++x) {
				row[x * 3 + 0] = src[x * 3 + 2]; // B
				row[x * 3 + 1] = src[x * 3 + 1]; // G
				row[x * 3 + 2] = src[x * 3 + 0]; // R
			}
			std::fwrite(row.data(), 1, strideOut, mFile);
		}
		// Pad odd-byte chunks.
		if (payload & 1) std::fputc(0, mFile);

		const uint32 offsetFromMovi = (uint32)(chunkStart - mMoviStartOff + 8);
		// Note: the dwChunkOffset field in idx1 entries points to the chunk's
		// dwChunkId (the start of the 8-byte header), measured *from* the
		// movi list's "movi" fourcc (i.e. mMoviStartOff - 4).
		(void)offsetFromMovi;
		const uint32 trueOffset = (uint32)(chunkStart - (mMoviStartOff - 4));
		appendIndexEntry("00db", trueOffset, payload, true);

		mTotalSampleBytes += payload;
		mMaxChunkSize = std::max(mMaxChunkSize, payload);
	} else {
		// MJPEG: encode through Qt.
		QByteArray jpeg;
		{
			QBuffer buf(&jpeg);
			buf.open(QIODevice::WriteOnly);
			img.save(&buf, "JPEG", mJpegQuality);
		}
		const uint32 payload  = (uint32)jpeg.size();
		const long chunkStart = std::ftell(mFile);
		writeChunkHeader("00dc", payload);
		std::fwrite(jpeg.constData(), 1, payload, mFile);
		if (payload & 1) std::fputc(0, mFile);

		const uint32 trueOffset = (uint32)(chunkStart - (mMoviStartOff - 4));
		appendIndexEntry("00dc", trueOffset, payload, true);

		mTotalSampleBytes += payload;
		mMaxChunkSize = std::max(mMaxChunkSize, payload);
	}
	++mFrameCount;
}

void ATVideoRecorder::Finalize() {
	if (mFinalized || !mFile) return;
	mFinalized = true;

	// Patch 'movi' LIST size.
	const long moviEnd  = std::ftell(mFile);
	const uint32 moviChunkSize = (uint32)(moviEnd - mMoviChunkSizeOff - 4);
	writeLE32At(mFile, mMoviChunkSizeOff, moviChunkSize);
	std::fseek(mFile, moviEnd, SEEK_SET);

	// Write idx1 chunk.
	const uint32 idxBytes = (uint32)(mIndex.size() * 16);
	writeFourcc(mFile, "idx1");
	writeLE32(mFile, idxBytes);
	for (const IndexEntry& e : mIndex) {
		std::fwrite(e.fourcc, 1, 4, mFile);
		writeLE32(mFile, e.flags);
		writeLE32(mFile, e.chunkOffset);
		writeLE32(mFile, e.chunkSize);
	}

	// Patch RIFF size = (file size - 8).
	const long fileEnd = std::ftell(mFile);
	writeLE32At(mFile, mRiffSizeOffset, (uint32)(fileEnd - 8));

	// Patch frame count and bandwidth fields.
	writeLE32At(mFile, mAvihTotalFramesOff, (uint32)mFrameCount);
	writeLE32At(mFile, mStrhLengthOff,      (uint32)mFrameCount);
	const uint32 bytesPerSec = mFrameCount
		? (uint32)((double)mTotalSampleBytes * mFrameRate
		           / (double)mFrameCount + 0.5)
		: 0;
	writeLE32At(mFile, mAvihMaxBytesPerSec,  bytesPerSec);
	writeLE32At(mFile, mAvihSuggBufSizeOff,  mMaxChunkSize);
	writeLE32At(mFile, mStrhSuggBufSizeOff,  mMaxChunkSize);

	std::fseek(mFile, fileEnd, SEEK_SET);
	std::fclose(mFile);
	mFile = nullptr;
	mIndex.clear();
}
