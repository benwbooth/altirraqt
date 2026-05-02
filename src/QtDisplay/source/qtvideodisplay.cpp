//	Altirra - Qt port
//	Qt-backed video display widget (QOpenGLWidget + IVDVideoDisplay).
//	Copyright (C) 2026 Ben Booth
//
//	GPL-2.0-or-later (matches Altirra)
//
//	Replaces upstream Win32 display drivers (displaydrvd3d9.cpp / displaygdi.cpp
//	/ displaydrv3d.cpp). Pixmap frames posted via IVDVideoDisplay are uploaded
//	to a GL texture and drawn as a fullscreen textured quad.
//
//	Limitations vs. the full Win32 driver:
//	  * No screen-FX pipeline (gamma, scanlines, bloom, distortion) — the
//	    renderer draws a plain bilinear/point-sampled quad.
//	  * No HDR support.
//	  * No custom effect / shader loading.
//	  * VSync behaviour is inherited from Qt's swap interval; explicit
//	    adaptive-vsync timing is not implemented.
//	  * Only a subset of source pixel formats is handled (XRGB8888,
//	    RGB565, Pal8). Other formats fall back to a Kasumi blit into an
//	    XRGB8888 staging buffer.
//	These can be added incrementally; the display.h API surface is fully
//	stubbed so the rest of the emulator links.

#include <at/qtdisplay/qtvideodisplay.h>

#include <QColor>
#include <QFont>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QString>
#include <QWheelEvent>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>

#include <vd2/system/atomic.h>
#include <vd2/system/refcount.h>
#include <vd2/Kasumi/pixmap.h>
#include <vd2/Kasumi/pixmapops.h>
#include <vd2/Kasumi/pixmaputils.h>

namespace {

class ATQtVideoDisplay final : public QOpenGLWidget, protected QOpenGLFunctions, public IVDVideoDisplay {
public:
	explicit ATQtVideoDisplay(QWidget *parent);
	~ATQtVideoDisplay() override;

	// IVDVideoDisplay
	void Destroy() override;
	void Reset() override;
	void SetSourceMessage(const wchar_t *msg) override;
	bool SetSource(bool autoUpdate, const VDPixmap& src, bool allowConversion) override;
	bool SetSourcePersistent(bool autoUpdate, const VDPixmap& src, bool allowConversion,
	                         const VDVideoDisplayScreenFXInfo *, IVDVideoDisplayScreenFXEngine *) override;
	void SetSourceSubrect(const vdrect32 *r) override;
	void SetSourceSolidColor(uint32 color) override;

	void SetReturnFocus(bool) override {}
	void SetTouchEnabled(bool) override {}
	void SetUse16Bit(bool) override {}
	void SetHDREnabled(bool) override {}
	void SetFullScreen(bool, uint32, uint32, uint32) override {}
	void SetCustomDesiredRefreshRate(float, float, float) override {}
	void SetDestRect(const vdrect32 *r, uint32 backgroundColor) override;
	void SetDestRectF(const vdrect32f *r, uint32 backgroundColor) override;
	void SetPixelSharpness(float sx, float sy) override {
		mPixelSharpness[0] = std::clamp(sx, -1.0f, 1.0f);
		mPixelSharpness[1] = std::clamp(sy, -1.0f, 1.0f);
		QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection);
	}
	void SetCompositor(IVDDisplayCompositor *) override {}
	void SetSDRBrightness(float) override {}

	void PostBuffer(VDVideoDisplayFrame *frame) override;
	bool RevokeBuffer(bool, VDVideoDisplayFrame **ppFrame) override;
	void FlushBuffers() override;

	void Invalidate() override;
	void Update(int mode) override;
	void Cache() override {}
	void SetCallback(IVDVideoDisplayCallback *p) override { mpCallback = p; }
	void SetOnFrameStatusUpdated(vdfunction<void(int)> fn) override { mOnFrameStatusUpdated = std::move(fn); }

	void SetAccelerationMode(AccelerationMode) override {}

	FilterMode GetFilterMode() override { return mFilterMode; }
	void SetFilterMode(FilterMode m) override { mFilterMode = m; }
	float GetSyncDelta() const override { return 0.0f; }

	int GetQueuedFrames() const override { return 0; }
	bool IsFramePending() const override { return false; }
	VDDVSyncStatus GetVSyncStatus() const override { return {}; }

	vdrect32 GetMonitorRect() override {
		return vdrect32(0, 0, width(), height());
	}

	bool IsScreenFXPreferred() const override { return false; }
	VDDHDRAvailability IsHDRCapable() const override { return VDDHDRAvailability::NoMinidriverSupport; }

	bool MapNormSourcePtToDest(vdfloat2&) const override { return true; }
	bool MapNormDestPtToSource(vdfloat2&) const override { return true; }

	void SetProfileHook(const vdfunction<void(ProfileEvent, uintptr)>&) override {}
	void RequestCapture(vdfunction<void(const VDPixmap *)> fn) override { mPendingCapture = std::move(fn); }

	void setKeyHandler(ATQtKeyEventFn fn)               { mKeyHandler         = std::move(fn); }
	void setMouseMoveHandler(ATQtMouseMoveFn fn)        { mMouseMoveHandler   = std::move(fn); }
	void setMouseButtonHandler(ATQtMouseButtonFn fn)    { mMouseButtonHandler = std::move(fn); }
	void setStretchMode(ATQtStretchMode m) { mStretchMode = m; QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void setPixelAspectRatio(double par)   { if (par > 0.0) mPixelAspectRatio = par; }
	void setScanlineIntensity(float s)     { mScanlineIntensity = std::clamp(s, 0.0f, 1.0f); QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void setIndicatorMargin(int pixels)    { mIndicatorMargin = std::max(0, pixels); QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void setSharpBilinear(bool on)         { mSharpBilinear = on; QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void setEnhancedText(bool on)          { mEnhancedText = on; QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void setEnhancedTextFont(const QFont& f) { mEnhFont = f; QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void setEnhancedTextProvider(ATQtScreenTextProvider fn) { mEnhProvider = std::move(fn); }
	bool setCustomShader(const QString& src) {
		// Compiling needs the GL context current; defer to paintGL via a
		// pending-source field. paintGL will call recompileCustomShader().
		mPendingShaderSrc = src;
		mPendingShaderApply = true;
		QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection);
		// Return whatever last-known compile state we have. The actual
		// compile happens in paintGL; result is queried via shaderError().
		return mLastCompileOk;
	}
	QString shaderError() const { return mShaderError; }

	void setPanZoomEnabled(bool on)        { mPanZoomEnabled = on; QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void resetPan()                        { mPanOffsetX = 0.0f; mPanOffsetY = 0.0f; QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void resetZoom()                       { mZoom = 1.0f; QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }
	void resetPanZoom()                    { mPanOffsetX = 0.0f; mPanOffsetY = 0.0f; mZoom = 1.0f; QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection); }

	void setHudEnabled(const char *name, bool on) {
		if (!name) return;
		mHudEnabled[std::string(name)] = on;
		QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection);
	}
	void setHudFps(double fps)             { mHudFps = fps; }
	void setHudDrives(const bool motors[8], const bool errors[8]) {
		for (int i = 0; i < 8; ++i) { mHudDriveMotor[i] = motors[i]; mHudDriveError[i] = errors[i]; }
	}
	void setHudCassette(bool loaded, float pos, float length) {
		mHudCassetteLoaded = loaded; mHudCassettePos = pos; mHudCassetteLen = length;
	}
	void setHudConsole(bool start, bool select, bool option) {
		mHudConsoleStart = start; mHudConsoleSelect = select; mHudConsoleOption = option;
	}
	void setHudPads(float x, float y, bool tA, bool tB) {
		mHudPadX = x; mHudPadY = y; mHudPadTriggerA = tA; mHudPadTriggerB = tB;
	}
	void setFrameRecorder(ATQtVideoFrameFn fn) { mFrameRecorder = std::move(fn); }

protected:
	// QOpenGLWidget
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	// QWidget
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private:
	void convertToStaging(const VDPixmap& src);
	void uploadPendingFrame();
	bool isHudOn(const char *name) const {
		auto it = mHudEnabled.find(std::string(name));
		return it != mHudEnabled.end() && it->second;
	}
	void paintHudOverlay(QPainter& p);

	std::mutex mFrameMutex;
	VDPixmapBuffer mStaging;           // XRGB8888 staging of the latest frame
	bool mStagingDirty = false;
	bool mHasFrame = false;
	uint32 mSolidColor = 0;
	bool mShowSolidColor = false;
	// GTIA hands posted frames over and expects them back later via
	// RevokeBuffer for recycling. Holding a small ring keeps frame-pool
	// allocations bounded and lets ~ATFrameBuffer eventually run as the
	// frames cycle out.
	std::deque<VDVideoDisplayFrame *> mFramePool;
	static constexpr size_t kMaxPoolFrames = 4;
	vdrect32f mDestRect{0, 0, 0, 0};
	bool mHasDestRect = false;
	uint32 mBackgroundColor = 0;
	FilterMode mFilterMode = kFilterAnySuitable;
	ATQtStretchMode mStretchMode = ATQtStretchMode::PreserveAspect;
	double mPixelAspectRatio = 7.0 / 6.0; // ~Atari NTSC default
	float mPixelSharpness[2] = {0.0f, 0.0f};
	float mScanlineIntensity = 0.0f;
	int mIndicatorMargin = 0;
	bool mSharpBilinear = false;
	bool mEnhancedText  = false;
	QFont mEnhFont{QStringLiteral("monospace"), 14};
	ATQtScreenTextProvider mEnhProvider;
	QOpenGLShaderProgram *mpCustomProgram = nullptr;
	QString  mPendingShaderSrc;
	bool     mPendingShaderApply = false;
	QString  mShaderError;
	bool     mLastCompileOk = true;

	IVDVideoDisplayCallback *mpCallback = nullptr;
	vdfunction<void(int)> mOnFrameStatusUpdated;
	vdfunction<void(const VDPixmap *)> mPendingCapture;

	// GL resources (owned on the GL thread)
	QOpenGLTexture *mpTexture = nullptr;
	QOpenGLShaderProgram *mpProgram = nullptr;
	QOpenGLBuffer mVbo{QOpenGLBuffer::VertexBuffer};
	QOpenGLVertexArrayObject mVao;
	int mTexWidth = 0;
	int mTexHeight = 0;

	ATQtKeyEventFn      mKeyHandler;
	ATQtMouseMoveFn     mMouseMoveHandler;
	ATQtMouseButtonFn   mMouseButtonHandler;

	// Pan/Zoom (clip-space units; mZoom multiplies the half-extents).
	bool   mPanZoomEnabled = false;
	float  mPanOffsetX     = 0.0f;
	float  mPanOffsetY     = 0.0f;
	float  mZoom           = 1.0f;
	bool   mPanDragging    = false;
	QPointF mPanDragLast;

	// HUD overlays.
	std::unordered_map<std::string, bool> mHudEnabled;
	double mHudFps = 0.0;
	bool   mHudDriveMotor[8] {};
	bool   mHudDriveError[8] {};
	bool   mHudCassetteLoaded = false;
	float  mHudCassettePos = 0.0f;
	float  mHudCassetteLen = 0.0f;
	bool   mHudConsoleStart  = false;
	bool   mHudConsoleSelect = false;
	bool   mHudConsoleOption = false;
	float  mHudPadX = 0.5f;
	float  mHudPadY = 0.5f;
	bool   mHudPadTriggerA = false;
	bool   mHudPadTriggerB = false;

	ATQtVideoFrameFn mFrameRecorder;
};

constexpr const char *kVertexSrc = R"(
#version 330 core
layout(location=0) in vec2 in_pos;
layout(location=1) in vec2 in_uv;
out vec2 v_uv;
uniform vec4 u_rect; // x0,y0,x1,y1 in clip space
void main() {
    vec2 p = mix(u_rect.xy, u_rect.zw, in_pos);
    gl_Position = vec4(p, 0.0, 1.0);
    v_uv = in_uv;
}
)";

// Fragment shader: 5-tap unsharp mask. u_sharpness ∈ [-1, +1] per axis.
// Positive values run the unsharp mask:
//     out = c + s*(c - (e+w)/2)     (horizontal axis; analog vertical)
// Negative values blend the centre with the cardinal-neighbor average:
//     out = c + |s|*((e+w)/2 - c)   (low-pass)
// Combining horizontal and vertical:
//     out = c + sH + sV
// where sH and sV switch on sign of u_sharpness.x and .y respectively.
constexpr const char *kFragmentSrc = R"(
#version 330 core
in vec2 v_uv;
out vec4 out_color;
uniform sampler2D u_tex;
uniform vec2 u_texel;       // 1 / textureSize, in UV
uniform vec2 u_sharpness;   // x = horizontal, y = vertical
uniform float u_scanline;   // 0 = off; (0,1] = darken row gap
uniform float u_tex_height; // source rows (= 1/u_texel.y), for scanline period
uniform float u_sharp_bilin;// 1 = sharp-bilinear UV remap; 0 = pass-through
void main() {
    vec2 uv = v_uv;
    if (u_sharp_bilin > 0.5) {
        // Themaister-style sharp bilinear: each source pixel renders as a
        // flat tile, with a 1-host-pixel-wide bilinear ramp at the boundary.
        // dx = source pixels traversed per host pixel = fwidth(srcUv).
        // Remapping the fract through (frac-0.5)/dx + 0.5 gives a step
        // whose ramp occupies dx source-pixel units (= 1 host pixel).
        vec2 texSize = vec2(1.0) / u_texel;
        vec2 srcUv  = uv * texSize;
        vec2 srcUvI = floor(srcUv);
        vec2 srcUvF = srcUv - srcUvI;
        vec2 dx     = fwidth(srcUv);
        vec2 t      = clamp((srcUvF - 0.5) / max(dx, vec2(1e-6)) + 0.5,
                            0.0, 1.0);
        uv = (srcUvI + t) * u_texel;
    }
    vec4 c = texture(u_tex, uv);
    if (u_sharpness != vec2(0.0)) {
        vec4 e = texture(u_tex, uv + vec2( u_texel.x, 0.0));
        vec4 w = texture(u_tex, uv + vec2(-u_texel.x, 0.0));
        vec4 n = texture(u_tex, uv + vec2(0.0, -u_texel.y));
        vec4 s = texture(u_tex, uv + vec2(0.0,  u_texel.y));
        float spx = max(u_sharpness.x, 0.0);
        float snx = max(-u_sharpness.x, 0.0);
        float spy = max(u_sharpness.y, 0.0);
        float sny = max(-u_sharpness.y, 0.0);
        vec4 sharpH = spx * (c - 0.5 * (e + w));
        vec4 sharpV = spy * (c - 0.5 * (n + s));
        vec4 blurH  = snx * (0.5 * (e + w) - c);
        vec4 blurV  = sny * (0.5 * (n + s) - c);
        c += sharpH + sharpV + blurH + blurV;
    }
    if (u_scanline > 0.0) {
        // Triangle wave across one source row: 0 at row centre, 1 at the
        // boundary between two rows. Darken by u_scanline at the boundary.
        float row = v_uv.y * u_tex_height;
        float gap = abs(2.0 * fract(row) - 1.0);
        c.rgb *= 1.0 - u_scanline * gap;
    }
    // Force opaque alpha — the source XRGB8888 texture's X byte isn't
    // guaranteed to be 0xff after VDPixmapBlt, and the host compositor
    // honours QOpenGLWidget's framebuffer alpha. Without this the window
    // appears transparent to the desktop behind it.
    out_color = vec4(clamp(c.rgb, 0.0, 1.0), 1.0);
}
)";

ATQtVideoDisplay::ATQtVideoDisplay(QWidget *parent)
	: QOpenGLWidget(parent)
{
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setMouseTracking(true);
}

ATQtVideoDisplay::~ATQtVideoDisplay() {
	makeCurrent();
	delete mpProgram;
	delete mpCustomProgram;
	delete mpTexture;
	mVbo.destroy();
	mVao.destroy();
	doneCurrent();
}

void ATQtVideoDisplay::Destroy() {
	deleteLater();
}

void ATQtVideoDisplay::Reset() {
	std::lock_guard<std::mutex> lock(mFrameMutex);
	mHasFrame = false;
	mShowSolidColor = false;
	mStagingDirty = false;
	update();
}

void ATQtVideoDisplay::SetSourceMessage(const wchar_t *) {
	// Status messages are not rendered in this minimal driver. A future
	// improvement could overlay them with QPainter during paintGL.
}

bool ATQtVideoDisplay::SetSource(bool autoUpdate, const VDPixmap& src, bool /*allowConversion*/) {
	{
		std::lock_guard<std::mutex> lock(mFrameMutex);
		convertToStaging(src);
		mHasFrame = true;
		mShowSolidColor = false;
		mStagingDirty = true;
	}
	if (autoUpdate)
		QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection);
	return true;
}

bool ATQtVideoDisplay::SetSourcePersistent(bool autoUpdate, const VDPixmap& src, bool allowConversion,
                                           const VDVideoDisplayScreenFXInfo *, IVDVideoDisplayScreenFXEngine *) {
	return SetSource(autoUpdate, src, allowConversion);
}

void ATQtVideoDisplay::SetSourceSubrect(const vdrect32 *) {
	// Subrect cropping not yet implemented — source is always rendered in full.
}

void ATQtVideoDisplay::SetSourceSolidColor(uint32 color) {
	std::lock_guard<std::mutex> lock(mFrameMutex);
	mSolidColor = color;
	mShowSolidColor = true;
	mHasFrame = false;
	QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection);
}

void ATQtVideoDisplay::SetDestRect(const vdrect32 *r, uint32 bg) {
	std::lock_guard<std::mutex> lock(mFrameMutex);
	if (r) {
		mDestRect = vdrect32f((float)r->left, (float)r->top, (float)r->right, (float)r->bottom);
		mHasDestRect = true;
	} else {
		mHasDestRect = false;
	}
	mBackgroundColor = bg;
}

void ATQtVideoDisplay::SetDestRectF(const vdrect32f *r, uint32 bg) {
	std::lock_guard<std::mutex> lock(mFrameMutex);
	if (r) {
		mDestRect = *r;
		mHasDestRect = true;
	} else {
		mHasDestRect = false;
	}
	mBackgroundColor = bg;
}

void ATQtVideoDisplay::PostBuffer(VDVideoDisplayFrame *frame) {
	if (!frame)
		return;

	// Display contract: PostBuffer takes a +1 reference on the frame, kept
	// alive in our pool until RevokeBuffer hands it back (or we drop it).
	frame->AddRef();
	SetSource(true, frame->mPixmap, frame->mbAllowConversion);

	std::lock_guard<std::mutex> lock(mFrameMutex);
	mFramePool.push_back(frame);
	while (mFramePool.size() > kMaxPoolFrames) {
		VDVideoDisplayFrame *old = mFramePool.front();
		mFramePool.pop_front();
		old->Release();
	}
}

bool ATQtVideoDisplay::RevokeBuffer(bool, VDVideoDisplayFrame **ppFrame) {
	if (!ppFrame) return false;
	std::lock_guard<std::mutex> lock(mFrameMutex);
	if (mFramePool.empty()) { *ppFrame = nullptr; return false; }
	*ppFrame = mFramePool.front();
	mFramePool.pop_front();
	return true;
}

void ATQtVideoDisplay::FlushBuffers() {
	std::lock_guard<std::mutex> lock(mFrameMutex);
	for (VDVideoDisplayFrame *f : mFramePool)
		f->Release();
	mFramePool.clear();
}

void ATQtVideoDisplay::Invalidate() {
	QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection);
}

void ATQtVideoDisplay::Update(int /*mode*/) {
	QMetaObject::invokeMethod(this, [this]{ update(); }, Qt::QueuedConnection);
}

void ATQtVideoDisplay::convertToStaging(const VDPixmap& src) {
	if (src.w <= 0 || src.h <= 0 || !src.data)
		return;

	// Ensure staging buffer is XRGB8888 at the source dimensions.
	if (mStaging.format != nsVDPixmap::kPixFormat_XRGB8888
	    || mStaging.w != src.w || mStaging.h != src.h) {
		mStaging.init(src.w, src.h, nsVDPixmap::kPixFormat_XRGB8888);
	}

	VDPixmapBlt(mStaging, src);
}

void ATQtVideoDisplay::initializeGL() {
	initializeOpenGLFunctions();

	mpProgram = new QOpenGLShaderProgram(this);
	mpProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexSrc);
	mpProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentSrc);
	mpProgram->link();

	mVao.create();
	mVao.bind();
	mVbo.create();
	mVbo.bind();
	// x,y,u,v quad covering the rect specified by u_rect
	const float verts[] = {
		0.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 0.0f,
	};
	mVbo.allocate(verts, sizeof verts);

	mpProgram->bind();
	mpProgram->enableAttributeArray(0);
	mpProgram->enableAttributeArray(1);
	mpProgram->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
	mpProgram->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
	mpProgram->release();
	mVbo.release();
	mVao.release();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void ATQtVideoDisplay::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);
}

void ATQtVideoDisplay::keyPressEvent(QKeyEvent *event) {
	if (event->isAutoRepeat()) {
		event->ignore();
		return;
	}
	if (mKeyHandler) {
		const Qt::KeyboardModifiers mods = event->modifiers();
		uint32 m = 0;
		if (mods & Qt::ShiftModifier)   m |= kATQtKeyMod_Shift;
		if (mods & Qt::ControlModifier) m |= kATQtKeyMod_Control;
		if (mods & Qt::AltModifier)     m |= kATQtKeyMod_Alt;
		mKeyHandler(event->key(), m, /*pressed=*/true);
		event->accept();
		return;
	}
	QOpenGLWidget::keyPressEvent(event);
}

void ATQtVideoDisplay::mouseMoveEvent(QMouseEvent *event) {
	if (mPanZoomEnabled) {
		if (mPanDragging && width() > 0 && height() > 0) {
			const QPointF cur = event->position();
			const QPointF d = cur - mPanDragLast;
			mPanDragLast = cur;
			// Convert pixel delta to clip-space delta (full widget = 2 NDC units).
			mPanOffsetX += (float)( 2.0 * d.x() / (double)width());
			mPanOffsetY += (float)(-2.0 * d.y() / (double)height());
			update();
		}
		event->accept();
		return;
	}
	if (mMouseMoveHandler && width() > 0 && height() > 0) {
		const float nx = std::clamp((float)event->position().x() / (float)width(),  0.0f, 1.0f);
		const float ny = std::clamp((float)event->position().y() / (float)height(), 0.0f, 1.0f);
		mMouseMoveHandler(nx, ny);
		event->accept();
		return;
	}
	QOpenGLWidget::mouseMoveEvent(event);
}

void ATQtVideoDisplay::mousePressEvent(QMouseEvent *event) {
	if (mPanZoomEnabled) {
		if (event->button() == Qt::LeftButton) {
			mPanDragging = true;
			mPanDragLast = event->position();
		}
		event->accept();
		return;
	}
	if (mMouseButtonHandler) {
		int b = -1;
		switch (event->button()) {
			case Qt::LeftButton:   b = 0; break;
			case Qt::RightButton:  b = 1; break;
			case Qt::MiddleButton: b = 2; break;
			default: break;
		}
		if (b >= 0) {
			mMouseButtonHandler(b, true);
			event->accept();
			return;
		}
	}
	QOpenGLWidget::mousePressEvent(event);
}

void ATQtVideoDisplay::mouseReleaseEvent(QMouseEvent *event) {
	if (mPanZoomEnabled) {
		if (event->button() == Qt::LeftButton) {
			mPanDragging = false;
		}
		event->accept();
		return;
	}
	if (mMouseButtonHandler) {
		int b = -1;
		switch (event->button()) {
			case Qt::LeftButton:   b = 0; break;
			case Qt::RightButton:  b = 1; break;
			case Qt::MiddleButton: b = 2; break;
			default: break;
		}
		if (b >= 0) {
			mMouseButtonHandler(b, false);
			event->accept();
			return;
		}
	}
	QOpenGLWidget::mouseReleaseEvent(event);
}

void ATQtVideoDisplay::wheelEvent(QWheelEvent *event) {
	if (mPanZoomEnabled) {
		// 120 units per notch on most platforms; ~12% per notch.
		const double steps = (double)event->angleDelta().y() / 120.0;
		const double factor = std::pow(1.125, steps);
		mZoom = (float)std::clamp((double)mZoom * factor, 0.25, 16.0);
		update();
		event->accept();
		return;
	}
	QOpenGLWidget::wheelEvent(event);
}

void ATQtVideoDisplay::keyReleaseEvent(QKeyEvent *event) {
	if (event->isAutoRepeat()) {
		event->ignore();
		return;
	}
	if (mKeyHandler) {
		const Qt::KeyboardModifiers mods = event->modifiers();
		uint32 m = 0;
		if (mods & Qt::ShiftModifier)   m |= kATQtKeyMod_Shift;
		if (mods & Qt::ControlModifier) m |= kATQtKeyMod_Control;
		if (mods & Qt::AltModifier)     m |= kATQtKeyMod_Alt;
		mKeyHandler(event->key(), m, /*pressed=*/false);
		event->accept();
		return;
	}
	QOpenGLWidget::keyReleaseEvent(event);
}

void ATQtVideoDisplay::uploadPendingFrame() {
	if (!mStagingDirty || mStaging.w <= 0 || mStaging.h <= 0)
		return;

	// Recreate texture if dimensions changed.
	if (!mpTexture || mTexWidth != mStaging.w || mTexHeight != mStaging.h) {
		delete mpTexture;
		mpTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
		mpTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
		mpTexture->setSize(mStaging.w, mStaging.h);
		mpTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
		mpTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
		mpTexture->allocateStorage();
		mTexWidth = mStaging.w;
		mTexHeight = mStaging.h;
	}

	// VDPixmap XRGB8888 is BGRA in little-endian byte order.
	QOpenGLPixelTransferOptions opts;
	opts.setRowLength(mStaging.pitch / 4);
	mpTexture->setData(0, 0, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, mStaging.data, &opts);

	// Apply chosen filter mode.
	if (mpTexture) {
		auto f = (mFilterMode == kFilterPoint)
			? QOpenGLTexture::Nearest
			: QOpenGLTexture::Linear;
		mpTexture->setMinMagFilters(f, f);
	}

	mStagingDirty = false;
}

void ATQtVideoDisplay::paintGL() {
	std::lock_guard<std::mutex> lock(mFrameMutex);

	// Enhanced-text mode: skip the GL framebuffer blit. paintGL still has
	// to clear the GL surface (otherwise we double-paint last frame
	// underneath), but the actual text rendering happens via QPainter
	// after the GL pass below.
	if (mEnhancedText) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		QPainter p(this);
		p.fillRect(rect(), Qt::black);
		p.setFont(mEnhFont);
		p.setPen(QColor(220, 220, 220));
		QFontMetrics fm(mEnhFont);
		const int rowH = fm.height();
		QStringList rows;
		if (mEnhProvider) rows = mEnhProvider();
		for (int i = 0; i < rows.size(); ++i)
			p.drawText(8, 8 + (i + 1) * rowH, rows[i]);
		// Frame recorder still gets called below.
		if (mFrameRecorder) {
			QImage frame = grabFramebuffer();
			mFrameRecorder(frame);
		}
		return;
	}

	const float r = ((mBackgroundColor >> 16) & 0xff) / 255.0f;
	const float g = ((mBackgroundColor >>  8) & 0xff) / 255.0f;
	const float b = ( mBackgroundColor        & 0xff) / 255.0f;
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (mShowSolidColor) {
		const float sr = ((mSolidColor >> 16) & 0xff) / 255.0f;
		const float sg = ((mSolidColor >>  8) & 0xff) / 255.0f;
		const float sb = ( mSolidColor        & 0xff) / 255.0f;
		glClearColor(sr, sg, sb, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	if (!mHasFrame)
		return;

	uploadPendingFrame();
	if (!mpTexture)
		return;

	// Compute clip-space rect for the destination quad. The indicator
	// margin reserves N host pixels at the bottom of the drawn area; the
	// fit math runs against the reduced height, then the resulting rect
	// is anchored so its bottom edge sits at the top of the reserved
	// strip.
	const float marginPx = (float)mIndicatorMargin;
	const float winH     = (float)height();
	const float marginNdc = (winH > 0.0f) ? (2.0f * marginPx / winH) : 0.0f;
	float x0 = -1.0f, y0 = -1.0f, x1 = 1.0f, y1 = 1.0f;
	if (mHasDestRect) {
		const float w = (float)width();
		const float h = (float)height();
		if (w > 0 && h > 0) {
			x0 = (mDestRect.left   / w) * 2.0f - 1.0f;
			x1 = (mDestRect.right  / w) * 2.0f - 1.0f;
			y0 = 1.0f - (mDestRect.bottom / h) * 2.0f;
			y1 = 1.0f - (mDestRect.top    / h) * 2.0f;
		}
	} else {
		const float wW = (float)width();
		const float hW = winH - marginPx;
		if (wW > 0 && hW > 0 && mTexWidth > 0 && mTexHeight > 0) {
			// "Logical" source size with PAR applied (width adjusted so
			// that mLogicalW:mTexHeight is the displayed aspect ratio).
			const double parW = (double)mTexWidth * mPixelAspectRatio;
			const double parH = (double)mTexHeight;

			float sx = 1.0f, sy = 1.0f;
			switch (mStretchMode) {
			case ATQtStretchMode::FitWindow:
				// Stretch edge-to-edge.
				break;
			case ATQtStretchMode::PreserveAspect: {
				const double srcAspect = parW / parH;
				const double winAspect = (double)wW / (double)hW;
				if (winAspect > srcAspect) sx = (float)(srcAspect / winAspect);
				else                       sy = (float)(winAspect / srcAspect);
				break;
			}
			case ATQtStretchMode::SquarePixels: {
				// Largest fit with no PAR correction (host pixels square).
				const double srcAspect = (double)mTexWidth / (double)mTexHeight;
				const double winAspect = (double)wW / (double)hW;
				if (winAspect > srcAspect) sx = (float)(srcAspect / winAspect);
				else                       sy = (float)(winAspect / srcAspect);
				break;
			}
			case ATQtStretchMode::IntegralSquare: {
				// Integer multiple of source size with square host pixels.
				const int n = std::max(1, std::min(
					(int)(wW / mTexWidth),
					(int)(hW / mTexHeight)));
				sx = (float)((double)(n * mTexWidth)  / wW);
				sy = (float)((double)(n * mTexHeight) / hW);
				break;
			}
			case ATQtStretchMode::IntegralAspect: {
				// Integer multiple in source pixels, PAR-correct in display.
				// Choose the largest n such that n*mTexHeight fits and
				// n*parW fits.
				const int n = std::max(1, std::min(
					(int)(wW / parW),
					(int)(hW / mTexHeight)));
				sx = (float)((double)(n * parW)        / wW);
				sy = (float)((double)(n * mTexHeight)  / hW);
				break;
			}
			}
			x0 = -sx; x1 = sx;
			// Map sy (fraction of the reduced-height area) into NDC over
			// the full window, anchored so the picture's bottom sits at
			// the top of the reserved indicator strip.
			const float halfNdc = sy * (hW / winH);
			const float centerNdc = marginNdc * 0.5f;
			y0 = centerNdc - halfNdc;
			y1 = centerNdc + halfNdc;
		}
	}

	// Apply pan/zoom on top of the stretch-mode rect: scale the half-extents
	// by mZoom and translate by mPanOffset. Then re-clip to [-1, 1] so the
	// picture stays inside the widget.
	{
		const float cx = 0.5f * (x0 + x1);
		const float cy = 0.5f * (y0 + y1);
		float hx = 0.5f * (x1 - x0) * mZoom;
		float hy = 0.5f * (y1 - y0) * mZoom;
		float ncx = cx + mPanOffsetX;
		float ncy = cy + mPanOffsetY;
		x0 = std::clamp(ncx - hx, -1.0f, 1.0f);
		x1 = std::clamp(ncx + hx, -1.0f, 1.0f);
		y0 = std::clamp(ncy - hy, -1.0f, 1.0f);
		y1 = std::clamp(ncy + hy, -1.0f, 1.0f);
	}

	// Apply pending custom-shader compile (must run with GL context current).
	if (mPendingShaderApply) {
		mPendingShaderApply = false;
		delete mpCustomProgram;
		mpCustomProgram = nullptr;
		if (!mPendingShaderSrc.isEmpty()) {
			auto *prog = new QOpenGLShaderProgram(this);
			const QByteArray vsrc(kVertexSrc);
			const QByteArray fsrc = mPendingShaderSrc.toUtf8();
			bool ok = prog->addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);
			ok = ok && prog->addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
			ok = ok && prog->link();
			if (ok) {
				prog->bind();
				prog->enableAttributeArray(0);
				prog->enableAttributeArray(1);
				prog->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
				prog->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
				prog->release();
				mpCustomProgram = prog;
				mShaderError.clear();
				mLastCompileOk = true;
			} else {
				mShaderError = prog->log();
				mLastCompileOk = false;
				delete prog;
			}
		} else {
			mShaderError.clear();
			mLastCompileOk = true;
		}
	}

	QOpenGLShaderProgram *active = mpCustomProgram ? mpCustomProgram : mpProgram;
	active->bind();
	mpTexture->bind(0);
	active->setUniformValue("u_tex", 0);
	active->setUniformValue("u_rect", QVector4D(x0, y0, x1, y1));
	active->setUniformValue("u_texel",
		QVector2D(mTexWidth  ? 1.0f / (float)mTexWidth  : 0.0f,
		          mTexHeight ? 1.0f / (float)mTexHeight : 0.0f));
	active->setUniformValue("u_sharpness",
		QVector2D(mPixelSharpness[0], mPixelSharpness[1]));
	active->setUniformValue("u_scanline",   mScanlineIntensity);
	active->setUniformValue("u_tex_height", (float)mTexHeight);
	active->setUniformValue("u_sharp_bilin", mSharpBilinear ? 1.0f : 0.0f);

	mVao.bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	mVao.release();

	mpTexture->release();
	active->release();

	// HUD overlay (drawn on top of the GL frame via QPainter on this widget).
	// Only construct the painter if at least one element is enabled — the
	// QPainter::begin() call alone is non-trivial.
	bool anyHud = false;
	for (const auto& kv : mHudEnabled) { if (kv.second) { anyHud = true; break; } }
	if (anyHud) {
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);
		paintHudOverlay(p);
	}

	// Frame recorder hook — grab the just-rendered GL framebuffer and pass
	// it to whoever installed a recorder. grabFramebuffer() is a member
	// function on QOpenGLWidget that returns a QImage copy of the back
	// buffer in the widget's pixel format (typically RGB32). This runs
	// inside paintGL so the GL context is current and no extra make-current
	// dance is needed.
	if (mFrameRecorder) {
		QImage frame = grabFramebuffer();
		mFrameRecorder(frame);
	}
}

void ATQtVideoDisplay::paintHudOverlay(QPainter& p) {
	const int W = width();
	const int H = height();
	if (W <= 0 || H <= 0) return;

	QFont f = p.font();
	f.setPointSizeF(std::max(8.0, f.pointSizeF()));
	f.setBold(true);
	p.setFont(f);

	const QColor kFg(255, 255, 255, 230);
	const QColor kBg(0, 0, 0, 140);
	const QColor kHi(80, 220, 120, 230);
	const QColor kErr(220, 80, 80, 230);
	const QColor kPad(80, 160, 220, 230);

	auto drawBoxedText = [&](QRectF r, const QString& s, QColor fg, QColor bg) {
		p.setPen(Qt::NoPen);
		p.setBrush(bg);
		p.drawRoundedRect(r, 4.0, 4.0);
		p.setPen(fg);
		p.drawText(r, Qt::AlignCenter, s);
	};

	// FPS — top-right.
	if (isHudOn("fps")) {
		const QString s = QStringLiteral("%1 fps").arg(mHudFps, 0, 'f', 1);
		QFontMetrics fm(p.font());
		const int tw = fm.horizontalAdvance(s) + 16;
		const int th = fm.height() + 6;
		drawBoxedText(QRectF(W - tw - 8, 8, tw, th), s, kFg, kBg);
	}

	// Console-button hints — top-left. Three letters S / E / O lit when held.
	if (isHudOn("console")) {
		QFontMetrics fm(p.font());
		const int th = fm.height() + 6;
		const int cw = fm.horizontalAdvance(QStringLiteral("Sel")) + 14;
		struct Hint { const char *label; bool on; } hints[] = {
			{"Start", mHudConsoleStart},
			{"Sel",   mHudConsoleSelect},
			{"Opt",   mHudConsoleOption},
		};
		int x = 8;
		for (auto& h : hints) {
			const int hw = fm.horizontalAdvance(QString::fromUtf8(h.label)) + 14;
			drawBoxedText(QRectF(x, 8, hw, th), QString::fromUtf8(h.label),
			              h.on ? kFg : QColor(180, 180, 180, 200),
			              h.on ? QColor(60, 90, 200, 200) : kBg);
			x += hw + 4;
			(void)cw;
		}
	}

	// Drive activity — bottom-right. Small lit "D1".."D8" when motor is on.
	if (isHudOn("drives")) {
		QFontMetrics fm(p.font());
		const int th = fm.height() + 6;
		int x = W - 8;
		for (int i = 7; i >= 0; --i) {
			if (!mHudDriveMotor[i] && !mHudDriveError[i]) continue;
			const QString s = QStringLiteral("D%1").arg(i + 1);
			const int tw = fm.horizontalAdvance(s) + 12;
			x -= tw + 4;
			QColor bg = mHudDriveError[i] ? kErr : kHi;
			drawBoxedText(QRectF(x, H - th - 8, tw, th), s, kFg, bg);
		}
	}

	// Cassette — bottom-left. "MM:SS / MM:SS" when loaded.
	if (isHudOn("cassette") && mHudCassetteLoaded) {
		auto fmt = [](float seconds) {
			if (seconds < 0.0f) seconds = 0.0f;
			const int total = (int)(seconds + 0.5f);
			return QStringLiteral("%1:%2")
				.arg(total / 60, 2, 10, QLatin1Char('0'))
				.arg(total % 60, 2, 10, QLatin1Char('0'));
		};
		const QString s = QStringLiteral("Tape %1 / %2")
			.arg(fmt(mHudCassettePos))
			.arg(fmt(mHudCassetteLen));
		QFontMetrics fm(p.font());
		const int tw = fm.horizontalAdvance(s) + 16;
		const int th = fm.height() + 6;
		drawBoxedText(QRectF(8, H - th - 8, tw, th), s, kFg, kBg);
	}

	// Pad indicator — small box centred near top middle showing the cursor
	// position as a dot. Triggers light up the L/R sides.
	if (isHudOn("pads")) {
		const int pw = 80, ph = 80;
		const int px = (W - pw) / 2;
		const int py = 12;
		p.setPen(Qt::NoPen);
		p.setBrush(kBg);
		p.drawRoundedRect(QRectF(px, py, pw, ph), 6.0, 6.0);
		// Crosshair guides.
		p.setPen(QColor(180, 180, 180, 120));
		p.drawLine(QPointF(px + pw * 0.5, py + 4), QPointF(px + pw * 0.5, py + ph - 4));
		p.drawLine(QPointF(px + 4, py + ph * 0.5), QPointF(px + pw - 4, py + ph * 0.5));
		// Dot.
		const float dx = std::clamp(mHudPadX, 0.0f, 1.0f);
		const float dy = std::clamp(mHudPadY, 0.0f, 1.0f);
		p.setBrush(kPad);
		p.setPen(Qt::NoPen);
		p.drawEllipse(QPointF(px + 6 + dx * (pw - 12), py + 6 + dy * (ph - 12)), 4.0, 4.0);
		// Trigger LEDs.
		p.setBrush(mHudPadTriggerA ? kHi : QColor(80, 80, 80, 180));
		p.drawEllipse(QPointF(px + 6, py + ph - 6), 3.0, 3.0);
		p.setBrush(mHudPadTriggerB ? kHi : QColor(80, 80, 80, 180));
		p.drawEllipse(QPointF(px + pw - 6, py + ph - 6), 3.0, 3.0);
	}
}

} // namespace

QWidget *ATCreateQtVideoDisplay(QWidget *parent) {
	return new ATQtVideoDisplay(parent);
}

IVDVideoDisplay *ATQtVideoDisplayAsInterface(QWidget *widget) {
	return dynamic_cast<ATQtVideoDisplay *>(widget);
}

void ATQtVideoDisplaySetKeyHandler(QWidget *widget, ATQtKeyEventFn fn) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setKeyHandler(std::move(fn));
}

void ATQtVideoDisplaySetMouseMoveHandler(QWidget *widget, ATQtMouseMoveFn fn) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setMouseMoveHandler(std::move(fn));
}

void ATQtVideoDisplaySetMouseButtonHandler(QWidget *widget, ATQtMouseButtonFn fn) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setMouseButtonHandler(std::move(fn));
}

void ATQtVideoDisplaySetStretchMode(QWidget *widget, ATQtStretchMode mode) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setStretchMode(mode);
}

void ATQtVideoDisplaySetPixelAspectRatio(QWidget *widget, double par) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setPixelAspectRatio(par);
}

void ATQtVideoDisplaySetScanlineIntensity(QWidget *widget, float strength) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setScanlineIntensity(strength);
}

void ATQtVideoDisplaySetSharpBilinear(QWidget *widget, bool enabled) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setSharpBilinear(enabled);
}

void ATQtVideoDisplaySetEnhancedText(QWidget *widget, bool enabled) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setEnhancedText(enabled);
}

void ATQtVideoDisplaySetEnhancedTextFont(QWidget *widget, const QFont& font) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setEnhancedTextFont(font);
}

void ATQtVideoDisplaySetEnhancedTextProvider(QWidget *widget, ATQtScreenTextProvider fn) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setEnhancedTextProvider(std::move(fn));
}

bool ATQtVideoDisplaySetCustomShader(QWidget *widget, const char *fragmentSource) {
	auto *d = dynamic_cast<ATQtVideoDisplay *>(widget);
	if (!d) return false;
	return d->setCustomShader(QString::fromUtf8(fragmentSource ? fragmentSource : ""));
}

const char *ATQtVideoDisplayGetShaderError(QWidget *widget) {
	static thread_local QByteArray cached;
	auto *d = dynamic_cast<ATQtVideoDisplay *>(widget);
	if (!d) return "";
	cached = d->shaderError().toUtf8();
	return cached.constData();
}

void ATQtVideoDisplaySetIndicatorMargin(QWidget *widget, int pixels) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setIndicatorMargin(pixels);
}

void ATQtVideoDisplaySetPanZoomEnabled(QWidget *widget, bool enabled) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setPanZoomEnabled(enabled);
}

void ATQtVideoDisplayResetPanZoom(QWidget *widget) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->resetPanZoom();
}

void ATQtVideoDisplayResetPan(QWidget *widget) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->resetPan();
}

void ATQtVideoDisplayResetZoom(QWidget *widget) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->resetZoom();
}

void ATQtVideoDisplaySetHudEnabled(QWidget *widget, const char *element, bool enabled) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setHudEnabled(element, enabled);
}

void ATQtVideoDisplaySetHudFps(QWidget *widget, double fps) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setHudFps(fps);
}

void ATQtVideoDisplaySetHudDrives(QWidget *widget, const bool motors[8], const bool errors[8]) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setHudDrives(motors, errors);
}

void ATQtVideoDisplaySetHudCassette(QWidget *widget, bool loaded, float positionSec, float lengthSec) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setHudCassette(loaded, positionSec, lengthSec);
}

void ATQtVideoDisplaySetHudConsole(QWidget *widget, bool start, bool select, bool option) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setHudConsole(start, select, option);
}

void ATQtVideoDisplaySetHudPads(QWidget *widget, float padX, float padY,
                                 bool padTriggerA, bool padTriggerB) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setHudPads(padX, padY, padTriggerA, padTriggerB);
}

void ATQtVideoDisplaySetFrameRecorder(QWidget *widget, ATQtVideoFrameFn fn) {
	if (auto *d = dynamic_cast<ATQtVideoDisplay *>(widget))
		d->setFrameRecorder(std::move(fn));
}
