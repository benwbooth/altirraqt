//	VirtualDub - Video processing and capture application
//	System library component
//	Copyright (C) 1998-2004 Avery Lee, All Rights Reserved.
//
//	Beginning with 1.6.0, the VirtualDub system library is licensed
//	differently than the remainder of VirtualDub.  This particular file is
//	thus licensed as follows (the "zlib" license):
//
//	This software is provided 'as-is', without any express or implied
//	warranty.  In no event will the authors be held liable for any
//	damages arising from the use of this software.
//
//	Permission is granted to anyone to use this software for any purpose,
//	including commercial applications, and to alter it and redistribute it
//	freely, subject to the following restrictions:
//
//	1.	The origin of this software must not be misrepresented; you must
//		not claim that you wrote the original software. If you use this
//		software in a product, an acknowledgment in the product
//		documentation would be appreciated but is not required.
//	2.	Altered source versions must be plainly marked as such, and must
//		not be misrepresented as being the original software.
//	3.	This notice may not be removed or altered from any source
//		distribution.

// Cross-platform Qt port: the original registry.cpp contained a Win32
// VDRegistryProviderW32 backed by HKCU/HKLM. On POSIX we have no equivalent
// system-wide key/value store, so VDGetDefaultRegistryProvider() now returns
// an in-memory VDRegistryProviderMemory. The UI layer is expected to load /
// persist it to a file via VDRegistryCopy at startup and shutdown.

#include <stdafx.h>
#include <vd2/system/VDString.h>
#include <vd2/system/registry.h>
#include <vd2/system/registrymemory.h>

namespace {
	VDRegistryProviderMemory& DefaultMemoryProvider() {
		static VDRegistryProviderMemory s_provider;
		return s_provider;
	}
}

static IVDRegistryProvider *g_pVDRegistryProvider = nullptr;

IVDRegistryProvider *VDGetDefaultRegistryProvider() {
	return &DefaultMemoryProvider();
}

IVDRegistryProvider *VDGetRegistryProvider() {
	if (!g_pVDRegistryProvider)
		g_pVDRegistryProvider = VDGetDefaultRegistryProvider();
	return g_pVDRegistryProvider;
}

void VDSetRegistryProvider(IVDRegistryProvider *provider) {
	g_pVDRegistryProvider = provider;
}

///////////////////////////////////////////////////////////////////////////

VDRegistryKey::VDRegistryKey(const char *keyName, bool global, bool write) {
	IVDRegistryProvider *provider = VDGetRegistryProvider();
	void *rootKey = global ? provider->GetMachineKey() : provider->GetUserKey();

	mKey = provider->CreateKey(rootKey, keyName, write);
}

VDRegistryKey::VDRegistryKey(VDRegistryKey& baseKey, const char *name, bool write) {
	IVDRegistryProvider *provider = VDGetRegistryProvider();
	void *rootKey = baseKey.getRawHandle();

	mKey = rootKey ? provider->CreateKey(rootKey, name, write) : NULL;
}

VDRegistryKey::VDRegistryKey(VDRegistryKey&& src)
	: mKey(src.mKey)
{
	src.mKey = nullptr;
}

VDRegistryKey::~VDRegistryKey() {
	if (mKey)
		VDGetRegistryProvider()->CloseKey(mKey);
}

VDRegistryKey& VDRegistryKey::operator=(VDRegistryKey&& src) {
	if (&src != this) {
		if (mKey)
			VDGetRegistryProvider()->CloseKey(mKey);

		mKey = src.mKey;
		src.mKey = nullptr;
	}

	return *this;
}

bool VDRegistryKey::setBool(const char *name, bool v) const {
	return mKey && VDGetRegistryProvider()->SetBool(mKey, name, v);
}

bool VDRegistryKey::setInt(const char *name, int i) const {
	return mKey && VDGetRegistryProvider()->SetInt(mKey, name, i);
}

bool VDRegistryKey::setString(const char *name, const char *s) const {
	return mKey && VDGetRegistryProvider()->SetString(mKey, name, s);
}

bool VDRegistryKey::setString(const char *name, const wchar_t *s) const {
	return mKey && VDGetRegistryProvider()->SetString(mKey, name, s);
}

bool VDRegistryKey::setBinary(const char *name, const char *data, int len) const {
	return mKey && VDGetRegistryProvider()->SetBinary(mKey, name, data, len);
}

VDRegistryKey::Type VDRegistryKey::getValueType(const char *name) const {
	Type type = kTypeUnknown;

	if (mKey) {
		switch(VDGetRegistryProvider()->GetType(mKey, name)) {
			case IVDRegistryProvider::kTypeInt:
				type = kTypeInt;
				break;

			case IVDRegistryProvider::kTypeString:
				type = kTypeString;
				break;

			case IVDRegistryProvider::kTypeBinary:
				type = kTypeBinary;
				break;
		}
	}

	return type;
}

bool VDRegistryKey::getBool(const char *name, bool def) const {
	bool v;
	return mKey && VDGetRegistryProvider()->GetBool(mKey, name, v) ? v : def;
}

int VDRegistryKey::getInt(const char *name, int def) const {
	int v;
	return mKey && VDGetRegistryProvider()->GetInt(mKey, name, v) ? v : def;
}

int VDRegistryKey::getEnumInt(const char *pszName, int maxVal, int def) const {
	int v = getInt(pszName, def);

	if (v<0 || v>=maxVal)
		v = def;

	return v;
}

bool VDRegistryKey::getString(const char *name, VDStringA& str) const {
	return mKey && VDGetRegistryProvider()->GetString(mKey, name, str);
}

bool VDRegistryKey::getString(const char *name, VDStringW& str) const {
	return mKey && VDGetRegistryProvider()->GetString(mKey, name, str);
}

int VDRegistryKey::getBinaryLength(const char *name) const {
	return mKey ? VDGetRegistryProvider()->GetBinaryLength(mKey, name) : -1;
}

bool VDRegistryKey::getBinary(const char *name, char *buf, int maxlen) const {
	return mKey && VDGetRegistryProvider()->GetBinary(mKey, name, buf, maxlen);
}

bool VDRegistryKey::removeValue(const char *name) {
	return mKey && VDGetRegistryProvider()->RemoveValue(mKey, name);
}

bool VDRegistryKey::removeKey(const char *name) {
	return mKey && VDGetRegistryProvider()->RemoveKey(mKey, name);
}

bool VDRegistryKey::removeKeyRecursive(const char *name) {
	return mKey && VDGetRegistryProvider()->RemoveKeyRecursive(mKey, name);
}

///////////////////////////////////////////////////////////////////////////////

VDRegistryValueIterator::VDRegistryValueIterator(const VDRegistryKey& key)
	: mEnumerator(key.getRawHandle() ? VDGetRegistryProvider()->EnumValuesBegin(key.getRawHandle()) : NULL)
{
}

VDRegistryValueIterator::~VDRegistryValueIterator() {
	if (mEnumerator)
		VDGetRegistryProvider()->EnumValuesClose(mEnumerator);
}

const char *VDRegistryValueIterator::Next() {
	return mEnumerator ? VDGetRegistryProvider()->EnumValuesNext(mEnumerator) : NULL;
}

///////////////////////////////////////////////////////////////////////////////

VDRegistryKeyIterator::VDRegistryKeyIterator(const VDRegistryKey& key)
	: mEnumerator(key.getRawHandle() ? VDGetRegistryProvider()->EnumKeysBegin(key.getRawHandle()) : NULL)
{
}

VDRegistryKeyIterator::~VDRegistryKeyIterator() {
	if (mEnumerator)
		VDGetRegistryProvider()->EnumKeysClose(mEnumerator);
}

const char *VDRegistryKeyIterator::Next() {
	return mEnumerator ? VDGetRegistryProvider()->EnumKeysNext(mEnumerator) : NULL;
}

///////////////////////////////////////////////////////////////////////////////

VDString VDRegistryAppKey::s_appbase;

VDRegistryAppKey::VDRegistryAppKey() : VDRegistryKey(s_appbase.c_str()) {
}

VDRegistryAppKey::VDRegistryAppKey(const char *pszKey, bool write, bool global)
	: VDRegistryKey((s_appbase + pszKey).c_str(), global, write)
{
}

void VDRegistryAppKey::setDefaultKey(const char *pszAppName) {
	s_appbase = pszAppName;
}

const char *VDRegistryAppKey::getDefaultKey() {
	return s_appbase.c_str();
}

///////////////////////////////////////////////////////////////////////////////

void VDRegistryCopy2(IVDRegistryProvider& dstProvider, void *dstParentKey, const char *dstPath, IVDRegistryProvider& srcProvider, void *srcParentKey, const char *srcPath) {
	void *srcKey = srcProvider.CreateKey(srcParentKey, srcPath, false);
	if (srcKey) {
		void *dstKey = dstProvider.CreateKey(dstParentKey, dstPath, true);

		if (dstKey) {
			void *srcValueEnum = srcProvider.EnumValuesBegin(srcKey);
			if (srcValueEnum) {
				while(const char *valueName = srcProvider.EnumValuesNext(srcValueEnum)) {
					switch(srcProvider.GetType(srcKey, valueName)) {
						case IVDRegistryProvider::kTypeInt:
							if (int ival = 0; srcProvider.GetInt(srcKey, valueName, ival))
								dstProvider.SetInt(dstKey, valueName, ival);
							break;
						case IVDRegistryProvider::kTypeString:
							if (VDStringW sval; srcProvider.GetString(srcKey, valueName, sval))
								dstProvider.SetString(dstKey, valueName, sval.c_str());
							break;
						case IVDRegistryProvider::kTypeBinary:
							if (int len = srcProvider.GetBinaryLength(srcKey, valueName); len >= 0) {
								vdblock<char> buf(len);

								if (srcProvider.GetBinary(srcKey, valueName, buf.data(), len))
									dstProvider.SetBinary(dstKey, valueName, buf.data(), len);
							}
							break;
					}
				}

				srcProvider.EnumValuesClose(srcValueEnum);
			}

			void *srcKeyEnum = srcProvider.EnumKeysBegin(srcKey);
			if (srcKeyEnum) {
				while(const char *subKeyName = srcProvider.EnumKeysNext(srcKeyEnum)) {
					VDRegistryCopy2(dstProvider, dstKey, subKeyName, srcProvider, srcKey, subKeyName);
				}

				srcProvider.EnumKeysClose(srcKeyEnum);
			}

			dstProvider.CloseKey(dstKey);
		}

		srcProvider.CloseKey(srcKey);
	}
}

void VDRegistryCopy(IVDRegistryProvider& dstProvider, const char *dstPath, IVDRegistryProvider& srcProvider, const char *srcPath) {
	VDRegistryCopy2(dstProvider, dstProvider.GetUserKey(), dstPath, srcProvider, srcProvider.GetUserKey(), srcPath);
}
