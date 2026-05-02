// Cross-platform stdafx for the VDCPP system library (Qt port).
//
// The original Altirra version pulls in <windows.h>. For the Qt port we keep
// this header minimal and rely on POSIX / Qt equivalents inside individual
// source files.

#ifndef f_SYSTEM_STDAFX_H
#define f_SYSTEM_STDAFX_H

struct IUnknown;

#include <vd2/system/vdtypes.h>
#include <vd2/system/atomic.h>
#include <vd2/system/thread.h>
#include <vd2/system/error.h>
#include <vd2/system/vdstl.h>
#include <vd2/system/vdstl_hashmap.h>

#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

#endif
