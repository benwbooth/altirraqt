//	altirraqt — Qt-side device-class registration.
//
//	Mirrors upstream's ATRegisterDevices(...) in
//	src/Altirra/source/devices.cpp, minus the device classes whose
//	implementation depends on Win32-specific helpers we haven't ported:
//
//	  * HostDevice, PCLink — Win32 file-error → SIO-error translation
//	  * Custom (scripting) — the Avery scripting VM, file-watcher,
//	    custom-network engine
//	  * Browser — IE-hosted browser device
//	  * 1090 80-col board, ComputerEyes, VideoGenerator, VideoStillImage,
//	    Bit3 FullView — depend on the alt-video output sink path that
//	    has no Qt-side implementation yet (XEP80 alt-output is the only
//	    alt video we wire today)
//
//	Everything else upstream registers — IDE, KMK/JZ, MIDI Mate, SX212,
//	XEP80, R-Time 8, every disk-drive emulator, VBXE, Rapidus, etc. —
//	is included here so the Device Manager dialog can instantiate them.

#include <vd2/system/refcount.h>
#include <vd2/system/VDString.h>
#include <at/atcore/device.h>
#include <devicemanager.h>

extern const ATDeviceDefinition g_ATDeviceDefModem;
extern const ATDeviceDefinition g_ATDeviceDefBlackBox;
extern const ATDeviceDefinition g_ATDeviceDefBlackBoxFloppy;
extern const ATDeviceDefinition g_ATDeviceDefMIO;
extern const ATDeviceDefinition g_ATDeviceDefHardDisks;
extern const ATDeviceDefinition g_ATDeviceDefIDERawImage;
extern const ATDeviceDefinition g_ATDeviceDefRTime8;
extern const ATDeviceDefinition g_ATDeviceDefHostDevice;
extern const ATDeviceDefinition g_ATDeviceDefPCLink;
extern const ATDeviceDefinition g_ATDeviceDefBrowser;
extern const ATDeviceDefinition g_ATDeviceDefCovox;
extern const ATDeviceDefinition g_ATDeviceDefXEP80;
extern const ATDeviceDefinition g_ATDeviceDefSlightSID;
extern const ATDeviceDefinition g_ATDeviceDefDragonCart;
extern const ATDeviceDefinition g_ATDeviceDefSIOClock;
extern const ATDeviceDefinition g_ATDeviceDefTestSIOPoll3;
extern const ATDeviceDefinition g_ATDeviceDefTestSIOPoll4;
extern const ATDeviceDefinition g_ATDeviceDefTestSIOHighSpeed;
extern const ATDeviceDefinition g_ATDeviceDefPrinter;
extern const ATDeviceDefinition g_ATDeviceDef850;
extern const ATDeviceDefinition g_ATDeviceDef850Full;
extern const ATDeviceDefinition g_ATDeviceDef835Modem;
extern const ATDeviceDefinition g_ATDeviceDef1030Modem;
extern const ATDeviceDefinition g_ATDeviceDefXM301Modem;
extern const ATDeviceDefinition g_ATDeviceDefSX212;
extern const ATDeviceDefinition g_ATDeviceDefSDrive;
extern const ATDeviceDefinition g_ATDeviceDefSIO2SD;
extern const ATDeviceDefinition g_ATDeviceDefVeronica;
extern const ATDeviceDefinition g_ATDeviceDefRVerter;
extern const ATDeviceDefinition g_ATDeviceDefSoundBoard;
extern const ATDeviceDefinition g_ATDeviceDefPocketModem;
extern const ATDeviceDefinition g_ATDeviceDefKMKJZIDE;
extern const ATDeviceDefinition g_ATDeviceDefKMKJZIDE2;
extern const ATDeviceDefinition g_ATDeviceDefMyIDED1xx;
extern const ATDeviceDefinition g_ATDeviceDefMyIDED5xx;
extern const ATDeviceDefinition g_ATDeviceDefMyIDE2;
extern const ATDeviceDefinition g_ATDeviceDefSIDE;
extern const ATDeviceDefinition g_ATDeviceDefSIDE2;
extern const ATDeviceDefinition g_ATDeviceDefSIDE3;
extern const ATDeviceDefinition g_ATDeviceDefDongle;
extern const ATDeviceDefinition g_ATDeviceDefPBIDisk;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrive810;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrive810Archiver;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveHappy810;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrive1050;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveUSDoubler;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveSpeedy1050;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveSpeedyXF;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveHappy1050;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveSuperArchiver;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveSuperArchiverBW;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveTOMS1050;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveTygrys1050;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrive1050Duplicator;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrive1050Turbo;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrive1050TurboII;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveISPlate;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveIndusGT;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveXF551;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveATR8000;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrivePercomRFD;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrivePercomAT;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrivePercomATSPD;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrive810Turbo;
extern const ATDeviceDefinition g_ATDeviceDefDiskDriveAMDC;
extern const ATDeviceDefinition g_ATDeviceDefDiskDrive815;
extern const ATDeviceDefinition g_ATDeviceDefVBXE;
extern const ATDeviceDefinition g_ATDeviceDefXELCF;
extern const ATDeviceDefinition g_ATDeviceDefXELCF3;
extern const ATDeviceDefinition g_ATDeviceDefRapidus;
extern const ATDeviceDefinition g_ATDeviceDefWarpOS;
extern const ATDeviceDefinition g_ATDeviceDefBlockDevVFAT16;
extern const ATDeviceDefinition g_ATDeviceDefBlockDevVFAT32;
extern const ATDeviceDefinition g_ATDeviceDefBlockDevVSDFS;
extern const ATDeviceDefinition g_ATDeviceDefBlockDevTemporaryWriteFilter;
extern const ATDeviceDefinition g_ATDeviceDefSimCovox;
extern const ATDeviceDefinition g_ATDeviceDef1400XL;
extern const ATDeviceDefinition g_ATDeviceDef1450XLDiskController;
extern const ATDeviceDefinition g_ATDeviceDef1450XLDiskControllerFull;

// Win32-only configurer registration is replaced by a no-op so the device
// manager simply doesn't surface per-device "Configure..." dialogs. Future
// work could port specific configurers (e.g. IDE image picker).
static void ATRegisterDeviceConfigurersStub(ATDeviceManager&) {}

void ATRegisterDevicesQt(ATDeviceManager& dm) {
	static constexpr const ATDeviceDefinition *kDeviceDefs[] = {
		&g_ATDeviceDefModem,
		&g_ATDeviceDefBlackBox,
		&g_ATDeviceDefBlackBoxFloppy,
		&g_ATDeviceDefMIO,
		&g_ATDeviceDefHardDisks,
		&g_ATDeviceDefIDERawImage,
		&g_ATDeviceDefRTime8,
		&g_ATDeviceDefHostDevice,
		&g_ATDeviceDefPCLink,
		&g_ATDeviceDefBrowser,
		&g_ATDeviceDefCovox,
		&g_ATDeviceDefXEP80,
		&g_ATDeviceDefSlightSID,
		&g_ATDeviceDefDragonCart,
		&g_ATDeviceDefSIOClock,
		&g_ATDeviceDefTestSIOPoll3,
		&g_ATDeviceDefTestSIOPoll4,
		&g_ATDeviceDefTestSIOHighSpeed,
		&g_ATDeviceDefPrinter,
		&g_ATDeviceDef850,
		&g_ATDeviceDef850Full,
		&g_ATDeviceDef835Modem,
		&g_ATDeviceDef1030Modem,
		&g_ATDeviceDefXM301Modem,
		&g_ATDeviceDefSX212,
		&g_ATDeviceDefSDrive,
		&g_ATDeviceDefSIO2SD,
		&g_ATDeviceDefVeronica,
		&g_ATDeviceDefRVerter,
		&g_ATDeviceDefSoundBoard,
		&g_ATDeviceDefPocketModem,
		&g_ATDeviceDefKMKJZIDE,
		&g_ATDeviceDefKMKJZIDE2,
		&g_ATDeviceDefMyIDED1xx,
		&g_ATDeviceDefMyIDED5xx,
		&g_ATDeviceDefMyIDE2,
		&g_ATDeviceDefSIDE,
		&g_ATDeviceDefSIDE2,
		&g_ATDeviceDefSIDE3,
		&g_ATDeviceDefDongle,
		&g_ATDeviceDefPBIDisk,
		&g_ATDeviceDefDiskDrive810,
		&g_ATDeviceDefDiskDrive810Archiver,
		&g_ATDeviceDefDiskDriveHappy810,
		&g_ATDeviceDefDiskDrive1050,
		&g_ATDeviceDefDiskDriveUSDoubler,
		&g_ATDeviceDefDiskDriveSpeedy1050,
		&g_ATDeviceDefDiskDriveSpeedyXF,
		&g_ATDeviceDefDiskDriveHappy1050,
		&g_ATDeviceDefDiskDriveSuperArchiver,
		&g_ATDeviceDefDiskDriveSuperArchiverBW,
		&g_ATDeviceDefDiskDriveTOMS1050,
		&g_ATDeviceDefDiskDriveTygrys1050,
		&g_ATDeviceDefDiskDrive1050Duplicator,
		&g_ATDeviceDefDiskDrive1050Turbo,
		&g_ATDeviceDefDiskDrive1050TurboII,
		&g_ATDeviceDefDiskDriveISPlate,
		&g_ATDeviceDefDiskDriveIndusGT,
		&g_ATDeviceDefDiskDriveXF551,
		&g_ATDeviceDefDiskDriveATR8000,
		&g_ATDeviceDefDiskDrivePercomRFD,
		&g_ATDeviceDefDiskDrivePercomAT,
		&g_ATDeviceDefDiskDrivePercomATSPD,
		&g_ATDeviceDefDiskDrive810Turbo,
		&g_ATDeviceDefDiskDriveAMDC,
		&g_ATDeviceDefDiskDrive815,
		&g_ATDeviceDefVBXE,
		&g_ATDeviceDefXELCF,
		&g_ATDeviceDefXELCF3,
		&g_ATDeviceDefRapidus,
		&g_ATDeviceDefWarpOS,
		&g_ATDeviceDefBlockDevVFAT16,
		&g_ATDeviceDefBlockDevVFAT32,
		&g_ATDeviceDefBlockDevVSDFS,
		&g_ATDeviceDefBlockDevTemporaryWriteFilter,
		&g_ATDeviceDefSimCovox,
		&g_ATDeviceDef1400XL,
		&g_ATDeviceDef1450XLDiskController,
		&g_ATDeviceDef1450XLDiskControllerFull,
	};
	for (const auto *def : kDeviceDefs)
		dm.AddDeviceDefinition(def);

	ATRegisterDeviceConfigurersStub(dm);
}
