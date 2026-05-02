//	altirraqt — Atari screen-text grabber.
//
//	Walks ANTIC's display-list history for the current frame and decodes
//	character-mode rows (modes 2/3/6/7) into ATASCII / hex / Unicode.
//	Used by the Edit menu's Copy Frame ... actions.

#pragma once

#include <QString>

class ATSimulator;

enum class ATScreenCopyMode {
	ASCII,
	Escaped,
	Hex,
	Unicode,
};

// Returns the screen contents of the current frame as a single string,
// rows separated by `\n`. Empty if no character-mode rows are visible.
QString ATGrabScreenText(ATSimulator& sim, ATScreenCopyMode mode);
