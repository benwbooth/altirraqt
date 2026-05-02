//	Altirra - Qt port
//	Wrapper around the upstream IATSAPWriter (sapwriter.cpp). Implements
//	a simple RAII handle: construct to begin recording, destruct to flush
//	and close. The upstream writer requires a raw filename pointer and
//	the simulator's event manager / pokey / UI renderer.

#pragma once

#include <memory>
#include <string>

class ATSimulator;
class IATSAPWriter;

class ATQtSapRecorder {
public:
	// Begin recording POKEY register state to `path`. Throws
	// std::runtime_error on file-open failure (translated from MyError
	// internally). Internally registers a VBLANK callback with the
	// simulator's event manager that snapshots POKEY's 9 audio
	// registers per frame (18 in stereo).
	ATQtSapRecorder(ATSimulator& sim, const char *path);
	~ATQtSapRecorder();

	ATQtSapRecorder(const ATQtSapRecorder&) = delete;
	ATQtSapRecorder& operator=(const ATQtSapRecorder&) = delete;

private:
	IATSAPWriter *mpWriter = nullptr;
};
