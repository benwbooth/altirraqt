//	Altirra - Qt port
//	Qt-backed video display widget implementing IVDVideoDisplay.
//	Copyright (C) 2026 Ben Booth
//
//	GPL-2.0-or-later (matches Altirra)

#pragma once

#include <vd2/system/function.h>
#include <vd2/VDDisplay/display.h>

#include <QStringList>

class QFont;
class QWidget;

// Modifier flag bits passed alongside a key code in the keyboard callback.
enum ATQtKeyModifier : uint32 {
	kATQtKeyMod_Shift   = 0x01,
	kATQtKeyMod_Control = 0x02,
	kATQtKeyMod_Alt     = 0x04,
};

// Keyboard event callback. `qtKey` is the Qt::Key value, `modifiers` is a
// bitmask of ATQtKeyModifier, and `pressed` is true on key-down, false on
// key-up. The display only forwards the event; mapping to Atari POKEY
// scan codes is done by the caller.
using ATQtKeyEventFn = vdfunction<void(int qtKey, uint32 modifiers, bool pressed)>;

// Create a new Qt video display widget. The returned widget is a
// QOpenGLWidget (as a QWidget*) that also implements the IVDVideoDisplay
// interface — use dynamic_cast or ATQtVideoDisplayAsInterface() to get
// the display interface.
QWidget *ATCreateQtVideoDisplay(QWidget *parent);

// Returns the IVDVideoDisplay interface for a widget previously created
// with ATCreateQtVideoDisplay(). Returns nullptr if the widget is not a
// Qt video display.
IVDVideoDisplay *ATQtVideoDisplayAsInterface(QWidget *widget);

// Install a keyboard callback that fires whenever the widget receives a
// Qt key press/release event. Pass an empty function to clear.
void ATQtVideoDisplaySetKeyHandler(QWidget *widget, ATQtKeyEventFn fn);

// Mouse movement callback. (nx, ny) are the cursor position normalized to
// the widget's interior, range [0, 1]. Use these to drive paddle/light pen.
using ATQtMouseMoveFn   = vdfunction<void(float nx, float ny)>;
// Mouse button event. button: 0=left, 1=right, 2=middle. pressed: down/up.
using ATQtMouseButtonFn = vdfunction<void(int button, bool pressed)>;

void ATQtVideoDisplaySetMouseMoveHandler  (QWidget *widget, ATQtMouseMoveFn fn);
void ATQtVideoDisplaySetMouseButtonHandler(QWidget *widget, ATQtMouseButtonFn fn);

// Stretch modes for fitting the source frame inside the widget. Mirror
// upstream Altirra's display stretch options.
enum class ATQtStretchMode : int {
	FitWindow,           // Fill the widget edge-to-edge, ignoring source aspect.
	PreserveAspect,      // Letter-/pillar-box so the visible source aspect is preserved.
	SquarePixels,        // Host pixels square (no PAR correction); largest fit.
	IntegralSquare,      // Square host pixels; integer-only scale.
	IntegralAspect,      // PAR-correct, integer-only scale.
};

void   ATQtVideoDisplaySetStretchMode(QWidget *widget, ATQtStretchMode mode);
// Pixel aspect ratio of the *source* frame: width/height. Values >1 mean
// each source pixel is wider than it is tall. Used by the PreserveAspect
// and IntegralAspect modes.
void   ATQtVideoDisplaySetPixelAspectRatio(QWidget *widget, double par);

// Scanline overlay strength. 0 disables; values in (0, 1] darken the
// inter-row gap by that fraction, simulating a CRT phosphor pattern.
void   ATQtVideoDisplaySetScanlineIntensity(QWidget *widget, float strength);

// Replace the default fragment shader with a user-provided GLSL 330-core
// program. Empty string restores the default. The user shader sees the same
// uniforms as the default (u_tex, u_texel, u_sharpness, u_scanline,
// u_tex_height, u_sharp_bilin) plus must declare `in vec2 v_uv;` and
// `out vec4 out_color;`. Returns true on successful compile/link.
bool   ATQtVideoDisplaySetCustomShader(QWidget *widget, const char *fragmentSource);

// Last compile error string (empty if none). Useful for surfacing failures
// to the user after a load attempt.
const char *ATQtVideoDisplayGetShaderError(QWidget *widget);

// Sharp-bilinear filtering: bilinear texture sample with the UV remapped so
// each source pixel renders as a flat tile with a one-host-pixel-wide
// bilinear ramp at the boundary. Crisper than plain bilinear, smoother than
// point. The widget still uses GL_LINEAR sampling, so the underlying filter
// mode set via IVDVideoDisplay::SetFilterMode should be kFilterBilinear.
void   ATQtVideoDisplaySetSharpBilinear(QWidget *widget, bool enabled);

// Enhanced-text mode: when enabled, the widget skips the GL framebuffer
// blit and instead asks `provider` for the current screen as a list of
// rows, then renders each row with the supplied host font via QPainter.
// `provider` may return an empty list — the widget paints background
// only. Pass `enabled=false` to disable; the provider is preserved so
// re-enabling doesn't require re-registering it.
using ATQtScreenTextProvider = vdfunction<QStringList()>;
void   ATQtVideoDisplaySetEnhancedText        (QWidget *widget, bool enabled);
void   ATQtVideoDisplaySetEnhancedTextFont    (QWidget *widget, const QFont& font);
void   ATQtVideoDisplaySetEnhancedTextProvider(QWidget *widget, ATQtScreenTextProvider fn);

// Reserve `pixels` host pixels at the bottom of the drawn area. The
// emulator frame is letterboxed above this margin so the host status bar
// or other indicators don't overlap the image. Pass 0 to disable.
void   ATQtVideoDisplaySetIndicatorMargin(QWidget *widget, int pixels);

// Pan / Zoom — when enabled, mouse drag pans and the wheel zooms the
// rendered frame. Pan/zoom is layered on top of whatever stretch mode is
// active, so the picture still letterboxes/integer-fits correctly.
void   ATQtVideoDisplaySetPanZoomEnabled(QWidget *widget, bool enabled);
void   ATQtVideoDisplayResetPanZoom    (QWidget *widget);
void   ATQtVideoDisplayResetPan        (QWidget *widget);
void   ATQtVideoDisplayResetZoom       (QWidget *widget);

// On-screen HUD overlays painted on top of the GL frame. Each element is
// addressed by name; see qtvideodisplay.cpp for the recognised set:
//    "fps", "drives", "cassette", "console", "pads"
// Each element is independently toggleable. Defaults are off.
void   ATQtVideoDisplaySetHudEnabled(QWidget *widget, const char *element, bool enabled);

// Frame recorder callback — invoked once per painted frame after the GL
// quad is drawn, with a host-resolution QImage capture of the framebuffer.
// Use this to feed a video recorder. Pass an empty function to clear.
// The callback runs on the GL thread; recorders should avoid blocking
// I/O (or do their I/O in fire-and-forget mode).
using ATQtVideoFrameFn = vdfunction<void(const class QImage&)>;
void   ATQtVideoDisplaySetFrameRecorder(QWidget *widget, ATQtVideoFrameFn fn);

// Per-frame HUD state setters — typically called from the UI's indicator
// timer with the same data the status bar uses.
void   ATQtVideoDisplaySetHudFps         (QWidget *widget, double fps);
void   ATQtVideoDisplaySetHudDrives      (QWidget *widget, const bool motors[8], const bool errors[8]);
void   ATQtVideoDisplaySetHudCassette    (QWidget *widget, bool loaded, float positionSec, float lengthSec);
void   ATQtVideoDisplaySetHudConsole     (QWidget *widget, bool start, bool select, bool option);
void   ATQtVideoDisplaySetHudPads        (QWidget *widget, float padX, float padY,
                                          bool padTriggerA, bool padTriggerB);
