//	altirraqt — Qt compat-database engine.
//	Loads upstream's binary `.atcptab` format (compatdb.cpp does the
//	parsing for us) and applies the matched title's tags to the running
//	simulator. Replaces upstream's compatengine.cpp, which is heavily
//	tied to Win32 (HWND-bearing dialog calls, IDR_COMPATDB resource
//	loader, and g_ATOptions).

#pragma once

#include <QString>
#include <vector>

class ATCompatDBView;
struct ATCompatDBHeader;
struct ATCompatDBTitle;
class ATSimulator;
class IATImage;

// Load an external compat database from a host file path. Returns true on
// success. The previous database (if any) is replaced. Pass an empty path
// to unload the database.
bool ATCompatLoadDatabase(const QString& path);

// Currently-loaded database view. Empty (IsValid() == false) when nothing
// is loaded.
const ATCompatDBView& ATCompatGetView();

// Direct pointer to the loaded database header (nullptr if none). Useful
// for enumerating the title list, which the public ATCompatDBView API
// only supports through rule lookup.
const ATCompatDBHeader *ATCompatGetHeader();

// Path the database was loaded from (empty if none).
QString ATCompatGetDatabasePath();

// Auto-apply toggle. When on, any image mounted via the menu mount paths
// triggers a lookup + apply of matched tags before the next ColdReset.
// Persists in QSettings under `compat/autoApply`.
bool ATCompatIsAutoApplyEnabled();
void ATCompatSetAutoApplyEnabled(bool on);

// Look up the given image (disk / cart / cassette / blob) in the database
// and apply the matched title's tags to the simulator. No-op if no
// database is loaded, the image is dynamic, or the title's hash is on the
// per-image disable list (`compat/<sha>/disabled`). Returns the matched
// title (for display) or nullptr.
const ATCompatDBTitle *ATCompatApplyForImage(ATSimulator& sim, IATImage *image);

// Per-title disable: store/clear the SHA-256 of the matched image in
// QSettings so a future mount of the same image skips compat tweaks.
void ATCompatDisableForImage(IATImage *image);
bool ATCompatIsDisabledForImage(IATImage *image);

// Convenience: walk every image currently attached to the simulator and
// apply the database to the first that matches. Call right after Load /
// LoadCartridge / LoadDisk / cassette Load and before ColdReset.
// Returns the matched title's name (UTF-8) or an empty string.
QString ATCompatApplyAfterMount(ATSimulator& sim);
