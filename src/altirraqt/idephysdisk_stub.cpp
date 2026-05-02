//	altirraqt — stub implementations for IDE physical disk + VHD image classes.
//
//	The real implementations live in upstream's idephysdisk.cpp /
//	idevhdimage.cpp and call DeviceIoControl / VHD-specific Win32 APIs.
//	On Linux we satisfy the linker with stubs that throw if the user
//	actually tries to mount a physical disk or VHD image; raw IDE
//	images via ATIDERawImage still work.

#include <vd2/system/VDString.h>
#include <vd2/system/error.h>

#include <at/atcore/blockdevice.h>
#include <at/atcore/deviceimpl.h>
#include <at/atcore/devicestorageimpl.h>

#include <idephysdisk.h>
#include <idevhdimage.h>

bool ATIDEIsPhysicalDiskPath(const wchar_t *) { return false; }
sint64 ATIDEGetPhysicalDiskSize(const wchar_t *) { return 0; }

ATIDEPhysicalDisk::ATIDEPhysicalDisk() : mhDisk(nullptr), mpBuffer(nullptr), mSectorCount(0) {}
ATIDEPhysicalDisk::~ATIDEPhysicalDisk() = default;

int ATIDEPhysicalDisk::AddRef()  { return ATDevice::AddRef(); }
int ATIDEPhysicalDisk::Release() { return ATDevice::Release(); }
void *ATIDEPhysicalDisk::AsInterface(uint32 iid) {
	if (iid == IATBlockDevice::kTypeID)
		return static_cast<IATBlockDevice *>(this);
	return ATDevice::AsInterface(iid);
}
void ATIDEPhysicalDisk::GetDeviceInfo(ATDeviceInfo&) {}
void ATIDEPhysicalDisk::GetSettings(ATPropertySet&) {}
bool ATIDEPhysicalDisk::SetSettings(const ATPropertySet&) { return true; }
void ATIDEPhysicalDisk::Shutdown() {}
ATBlockDeviceGeometry ATIDEPhysicalDisk::GetGeometry() const { return {}; }
uint32 ATIDEPhysicalDisk::GetSerialNumber() const { return 0; }
void ATIDEPhysicalDisk::Init(const wchar_t *) {
	throw MyError("Physical IDE disk passthrough is not supported in the Qt port.");
}
void ATIDEPhysicalDisk::Flush() {}
void ATIDEPhysicalDisk::ReadSectors(void *, uint32, uint32) {
	throw MyError("Physical IDE disk passthrough is not supported in the Qt port.");
}
void ATIDEPhysicalDisk::WriteSectors(const void *, uint32, uint32) {
	throw MyError("Physical IDE disk passthrough is not supported in the Qt port.");
}

ATIDEVHDImage::ATIDEVHDImage() = default;
ATIDEVHDImage::~ATIDEVHDImage() = default;

int ATIDEVHDImage::AddRef()  { return ATDevice::AddRef(); }
int ATIDEVHDImage::Release() { return ATDevice::Release(); }
void *ATIDEVHDImage::AsInterface(uint32 iid) {
	if (iid == IATBlockDevice::kTypeID)
		return static_cast<IATBlockDevice *>(this);
	return ATDevice::AsInterface(iid);
}
void ATIDEVHDImage::GetDeviceInfo(ATDeviceInfo&) {}
void ATIDEVHDImage::GetSettings(ATPropertySet&) {}
bool ATIDEVHDImage::SetSettings(const ATPropertySet&) { return true; }
void ATIDEVHDImage::Shutdown() {}
uint32 ATIDEVHDImage::GetSectorCount() const { return 0; }
VDStringW ATIDEVHDImage::GetVHDDirectAccessPath() const { return {}; }
void ATIDEVHDImage::GetSettingsBlurb(VDStringW&) {}
uint32 ATIDEVHDImage::GetVHDTimestamp() const { return 0; }
ATBlockDeviceGeometry ATIDEVHDImage::GetGeometry() const { return {}; }
uint32 ATIDEVHDImage::GetSerialNumber() const { return 0; }
void ATIDEVHDImage::Init(const wchar_t *, bool, bool) {
	throw MyError("VHD image support is not yet ported to the Qt build.");
}
void ATIDEVHDImage::Flush() {}
void ATIDEVHDImage::ReadSectors(void *, uint32, uint32) {
	throw MyError("VHD image support is not yet ported to the Qt build.");
}
void ATIDEVHDImage::WriteSectors(const void *, uint32, uint32) {
	throw MyError("VHD image support is not yet ported to the Qt build.");
}
