//	Altirra - Qt port
//	Portable shim for the upstream Win32-only header.
//
//	The full upstream `<at/atnativeui/uiframe.h>` defines a docking-frame
//	system built on HWND / HFONT / HMONITOR — none of which apply on
//	Linux. The Qt port hosts the debugger console as a regular Qt dock
//	widget, so the frame system isn't needed.
//
//	The only symbol upstream's `debugger.cpp` actually references through
//	this header is `ATActivateUIPane` — used to bring the disassembly
//	pane forward when stepping into source-less code. The Qt port has
//	no debugger pane wired yet (the PLAN.md "Debugger Console pane"
//	item is still open), so this declaration is satisfied by a no-op
//	defined alongside the other UI-pane glue in altirraqt/stubs.cpp.
//	Once the Qt debugger panes land, the implementation moves there.
//
//	Including this shim instead of the Win32 original keeps the upstream
//	debugger source bit-identical apart from the existing `#include
//	<at/atnativeui/uiframe.h>` line — which still resolves, just to the
//	portable forward-declarations below.

#ifndef f_AT_ATNATIVEUI_UIFRAME_H
#define f_AT_ATNATIVEUI_UIFRAME_H

#include <vd2/system/vdtypes.h>

class ATContainerWindow;
class ATFrameWindow;
class ATUIPane;

void ATActivateUIPane(uint32 id, bool giveFocus, bool visible = true, uint32 relid = 0, int reldock = 0);

#endif
