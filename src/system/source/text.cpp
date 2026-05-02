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

// Cross-platform Qt port: the original file leaned on
// WideCharToMultiByte/MultiByteToWideChar + CP_ACP + FindNLSStringEx. On POSIX
// our "narrow" strings are UTF-8, so the narrow↔wide routines route through the
// existing VDTextWToU8 / VDTextU8ToW helpers already defined in this file.

#include <stdafx.h>
#include <vector>
#include <algorithm>
#include <wctype.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <vd2/system/vdtypes.h>
#include <vd2/system/vdstdc.h>
#include <vd2/system/vdstl.h>
#include <vd2/system/text.h>
#include <vd2/system/tls.h>
#include <vd2/system/VDString.h>

VDStringA VDTextWToU8(const VDStringSpanW& s);
VDStringA VDTextWToU8(const wchar_t *s, int length);
VDStringW VDTextU8ToW(const VDStringSpanA& s);
VDStringW VDTextU8ToW(const char *s, int length);

int VDTextWToA(char *dst, int max_dst, const wchar_t *src, int max_src) {
	VDASSERTPTR(dst);
	VDASSERTPTR(src);
	VDASSERT(max_dst>0);

	*dst = 0;

	VDStringA u8 = VDTextWToU8(src, max_src);
	int len = (int)u8.size();
	if (len >= max_dst)
		len = max_dst - 1;

	memcpy(dst, u8.data(), len);
	dst[len] = 0;

	// Match Win32 semantics: returned length includes the null terminator iff
	// the input length was specified (max_src >= 0).
	return max_src < 0 ? len : len + 1;
}

int VDTextAToW(wchar_t *dst, int max_dst, const char *src, int max_src) {
	VDASSERTPTR(dst);
	VDASSERTPTR(src);
	VDASSERT(max_dst>0);

	*dst = 0;

	VDStringW w = VDTextU8ToW(src, max_src);
	int len = (int)w.size();
	if (len >= max_dst)
		len = max_dst - 1;

	for (int i = 0; i < len; ++i)
		dst[i] = w[i];
	dst[len] = 0;

	return max_src < 0 ? len : len + 1;
}

VDStringA VDTextWToA(const VDStringW& sw) {
	return VDTextWToA(sw.data(), sw.length());
}

VDStringA VDTextWToA(const wchar_t *src, int srclen) {
	VDStringA s;

	if (src) {
		int l = VDTextWToALength(src, srclen);

		if (l) {
			s.resize(l);
			VDTextWToA((char *)s.data(), l+1, src, srclen);
		}
	}

	return s;
}

VDStringW VDTextAToW(const VDStringA& s) {
	return VDTextAToW(s.data(), s.length());
}

VDStringW VDTextAToW(const char *src, int srclen) {
	VDStringW sw;

	if (src) {
		int l = VDTextAToWLength(src, srclen);

		if (l) {
			sw.resize(l);
			VDTextAToW(&sw[0], sw.length()+1, src, srclen);
		}
	}

	return sw;
}

int VDTextWToALength(const wchar_t *s, int length) {
	return (int)VDTextWToU8(s, length).size();
}

int VDTextAToWLength(const char *s, int length) {
	return (int)VDTextU8ToW(s, length).size();
}

namespace {
	bool VDIsUnicodeSurrogateFirst(wchar_t c) {
		return (c >= 0xD800 && c < 0xDC00); 
	}

	bool VDIsUnicodeSurrogateSecond(wchar_t c) {
		return (c >= 0xDC00 && c < 0xE000);
	}
};

size_t VDCodePointToU8(uint8 *dst, size_t dstLen, const wchar_t *src, size_t srcLen, size_t& srcElementsUsed) {
	uint8 *dst0 = dst;
	const wchar_t *src0 = src;

	while(srcLen--) {
		const wchar_t *src2 = src;
		uint32 c = *src2++;

		if (VDIsUnicodeSurrogateFirst(c)) {
			if (!srcLen || !VDIsUnicodeSurrogateSecond(*src2)) {
				c = '?';
			} else {
				c = 0x10000 + ((c & 0x3ff)<<10) + (*src++ & 0x3ff);
				--srcLen;
			}
		}

		if (c < 0x0080) {
			if (!dstLen--)
				break;

			*dst++ = (uint8)c;
		} else {
			if (c < 0x0800) {
				if (dstLen < 2)
					break;

				dstLen -= 2;

				*dst++ = (uint8)(0xc0 + (c>>6));
			} else {
				if (c < 0x10000) {
					if (dstLen < 3)
						break;

					dstLen -= 3;

					*dst++ = (uint8)(0xe0 + (c>>12));
				} else {
					if (dstLen < 4)
						break;

					dstLen -= 4;

					*dst++ = (uint8)(0xf0 + ((c>>18) & 0x07));
					*dst++ = (uint8)(0x80 + ((c>>12) & 0x3f));
				}

				*dst++ = (uint8)(0x80 + ((c>>6) & 0x3f));
			}

			*dst++ = (uint8)(0x80 + (c & 0x3f));
		}

		src = src2;
	}

	srcElementsUsed = (size_t)(src - src0);
	return (size_t)(dst - dst0);
}

VDStringA VDTextWToU8(const VDStringSpanW& s) {
	return VDTextWToU8(s.data(), s.length());
}

VDStringA VDTextWToU8(const wchar_t *s, int length) {
	vdfastvector<char> temp;

	if (length<0) {
		const wchar_t *t = s;
		do {
			++length;
		} while(*t++);
	}

	while(length--) {
		uint32 c = *s++;

		if (VDIsUnicodeSurrogateFirst(c)) {
			if (!length || !VDIsUnicodeSurrogateSecond(*s)) {
				VDASSERT(false);
				c = '?';
			} else {
				c = 0x10000 + ((c & 0x3ff)<<10) + (*s++ & 0x3ff);
				--length;
			}
		}

		if (c < 0x0080) {
			temp.push_back((char)c);
		} else {
			if (c < 0x0800)
				temp.push_back((char)(0xc0 + (c>>6)));
			else {
				if (c < 0x10000)
					temp.push_back((char)(0xe0 + (c>>12)));
				else {
					temp.push_back((char)(0xf0 + ((c>>18) & 0x07)));
					temp.push_back((char)(0x80 + ((c>>12) & 0x3f)));
				}
				temp.push_back((char)(0x80 + ((c>>6) & 0x3f)));
			}
			temp.push_back((char)(0x80 + (c & 0x3f)));
		}
	}

	VDStringA a(temp.data(), temp.size());

	return a;
}

VDStringW VDTextU8ToW(const VDStringSpanA& s) {
	return VDTextU8ToW(s.data(), s.length());
}

VDStringW VDTextU8ToW(const char *s, int length) {
	vdfastvector<wchar_t> temp;

	if (length<0) {
		const char *t = s;
		VDASSERT(length == -1);
		do {
			++length;
		} while(*t++);
	}

	while(length--) {
		unsigned char c = (char)*s++;
		uint32	wc = c;			// we reconstruct UTF-32 first and then split to UTF-16 if necessary

		if (c >= 0x80) {
			int required_extra = 0;

			if (c < 0xc0 || c >= 0xf7) {
				VDASSERT(false);
				break;
			}

			while(c >= 0xc0) {
				c <<= 1;
				++required_extra;
			}

			wc = (c&0x3f) >> required_extra;

			do {
				char d;

				if (!length-- || (((d=*s++)&0xc0)!=0x80))
					goto bad_sequence_exit;

				wc = (wc<<6) + (d&0x3f);
			} while(--required_extra);
		}

		// Two cases here.  If we are using UTF-16, surrogates need to be split in half.  If we are using
		// UTF-32, surrogates need to be combined.

		if constexpr (sizeof(wchar_t) > 2) {
			if (VDIsUnicodeSurrogateSecond(wc)) {
				if (temp.empty() || !VDIsUnicodeSurrogateFirst(temp.back())) {
					VDASSERT(false);
					break;
				}

				temp.back() = 0x10000 + ((temp.back()&0x3ff) << 10) + (wc & 0x3ff);
				continue;
			}
		} else {
			if (wc >= 0x10000) {
				wc -= 0x10000;
				temp.push_back(0xD800 + ((wc >> 10) & 0x3ff));
				wc = 0xDC00 + (wc&0x3ff);
			}
		}
		temp.push_back(wc);
	}
bad_sequence_exit:

	VDStringW w(temp.data(), temp.size());

	return w;
}

///////////////////////////////////////////////////////////////////////////

bool VDTextContainsSubstringMatchByLocale(VDStringSpanW sourceString, VDStringSpanW searchString) {
	// Cross-platform Qt port: Win32's FindNLSStringEx did locale-aware
	// case/width-insensitive search. POSIX approximation: fold both sides to
	// lowercase via towlower and look for a plain substring match.
	if (searchString.empty())
		return true;
	if (sourceString.size() < searchString.size())
		return false;

	std::vector<wchar_t> src(sourceString.begin(), sourceString.end());
	std::vector<wchar_t> pat(searchString.begin(), searchString.end());
	for (auto& c : src) c = (wchar_t)towlower((wint_t)c);
	for (auto& c : pat) c = (wchar_t)towlower((wint_t)c);

	const size_t slen = src.size();
	const size_t plen = pat.size();
	for (size_t i = 0; i + plen <= slen; ++i) {
		if (!memcmp(src.data() + i, pat.data(), plen * sizeof(wchar_t)))
			return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////
//
//	VirtualDub's very own printf() functions.
//
//	VD[v|a]swprintf() differs from wsprintf() in the following ways:
//
//	* The output is a string.
//	* All parameters must be passed by pointer instead of by value.
//	* The 'll' modifier permits long long / __int64 integers.
//	* [n] allows picking parameters out of order.
//	* %lc/%ls forces Unicode; %hc/%hs forces ANSI.

VDStringW VDaswprintf(const wchar_t *format, int args, const void *const *argv) {
	const void *const *argv0 = argv;
	vdfastfixedvector<wchar_t, 256> out;
	wchar_t c;

	VDStringW tempConv;

	while((c = *format)) {
		if (c != L'%') {
			const wchar_t *s = format;

			while(*s && *s != L'%')
				++s;

			int len = s - format;
			int clen = out.size();

			out.resize(clen + len);

			std::copy(format, s, &out[clen]);

			format = s;
		} else {
			++format;

			// check for %%

			if (*format == L'%') {
				++format;
				out.push_back(L'%');
				continue;
			}

			// Check for a renumbering identifier.

			if (*format == L'[') {
				++format;

				int newid = wcstol(format, const_cast<wchar_t **>(&format), 0);

				VDASSERT(newid >= 0 && newid < args);

				argv = argv0 + newid;

				VDVERIFY(*format++ == L']');
			}

			// process flags

			struct {
				bool bLeftAlign:1,		// pad right with spaces (priority over zero pad)
					bZeroPad:1,			// pad left with zeroes
					bPositiveSign:1,	// prefix with + or -; priority over bPositiveBlank
					bPositiveBlank:1,	// prefix with space for nonnegative
					bPrefix:1;			// prefix with 0, 0x, 0X, or force decimal point
			} flags={false};
			int width = 0;
			int precision = -1;

			for(;;) {
				c = *format;

				if (c == L'0')
					flags.bZeroPad = true;
				else if (c == L' ')
					flags.bPositiveBlank = true;
				else if (c == L'#')
					flags.bPrefix = true;
				else if (c == L'-')
					flags.bLeftAlign = true;
				else if (c == L'+')
					flags.bPositiveSign = true;
				else
					break;

				++format;
			}

			// process width

			c = *format;
			if (c == L'*') {
				++format;
				width = *(int *)*argv++;
			} else if (iswdigit(c))
				width = (int)wcstol(format, const_cast<wchar_t **>(&format), 0);

			// process precision

			if (*format == L'.') {
				c = *++format;

				if (c == L'*') {
					++format;
					precision = *(int *)*argv++;
				} else if (iswdigit(c))
					precision = (int)wcstol(format, const_cast<wchar_t **>(&format), 0);
			}

			// process flags

			enum { kDefault, kLong, kLongLong, kShort } size = kDefault;

			c = *format;

			if (c == L'l') {
				++format;
				size = kLong;

				if (*format == L'l') {
					++format;
					size = kLongLong;
				}

			} else if (c == L'h') {
				++format;
				size = kShort;
			}

			// process format character

			wchar_t xf[32], buf[32], *pxf = xf, *pbuf0 = buf, *pbuf = buf;
			int zero_pad = 0;

			switch(*format++) {
			case L'd':
			case L'i':
			case L'o':
			case L'u':
			case L'x':
			case L'X':
				*pxf++ = '%';
				if (flags.bPrefix)
					*pxf++ = '#';
				if (flags.bPositiveBlank)
					*pxf++ = ' ';
				if (flags.bPositiveSign)
					*pxf++ = '+';

				switch(size) {
				case kShort:
					*pxf++ = 'h';
					*pxf++ = format[-1];
					*pxf = 0;
					pbuf += vdswprintf(pbuf, sizeof buf / sizeof buf[0], xf, *(const short *)*argv++);
					break;
				case kDefault:
					*pxf++ = format[-1];
					*pxf = 0;
					pbuf += vdswprintf(pbuf, sizeof buf / sizeof buf[0], xf, *(const int *)*argv++);
					break;
				case kLong:
					*pxf++ = 'l';
					*pxf++ = format[-1];
					*pxf = 0;
					pbuf += vdswprintf(pbuf, sizeof buf / sizeof buf[0], xf, *(const long *)*argv++);
					break;
				case kLongLong:
					*pxf++ = 'l';
					*pxf++ = 'l';
					*pxf++ = format[-1];
					*pxf = 0;
					pbuf += vdswprintf(pbuf, sizeof buf / sizeof buf[0], xf, *(const int64 *)*argv++);
					break;
				default:
					VDNEVERHERE;
				}

				if (pbuf - pbuf0 < precision)
					zero_pad = precision - (pbuf - pbuf0);

				break;

			case L'c':
				if (size == kShort) {
					char buf[2] = {*(const char *)*argv++, 0};
					pbuf += VDTextAToW(pbuf, 4, buf);
				} else
					*pbuf++ = *(const wchar_t *)*argv++;
				break;

			case L's':
				if (size == kShort) {
					const char *s = *(const char *const *)*argv++;
					int maxsrc = strlen(s);

					if (precision >= 0 && precision < maxsrc)
						maxsrc = precision;

					tempConv = VDTextAToW(s, maxsrc);
					pbuf0 = const_cast<wchar_t *>(tempConv.c_str());

					pbuf = pbuf0 + tempConv.size();
				} else {
					pbuf = pbuf0 = *(wchar_t *const *)*argv++;

					while(*pbuf && precision) {
						++pbuf;
						--precision;
					}
				}
				break;

			case L'e':
			case L'E':
			case L'f':
			case L'F':
			case L'g':
			case L'G':
				// We place an artificial limit of 256 characters on the precision value.
				{
					if (precision > 256)
						precision = 256;

					tempConv.resize(256);
					pbuf0 = pbuf = const_cast<wchar_t *>(tempConv.data());

					*pxf++ = '%';
					if (flags.bPrefix)
						*pxf++ = '#';
					if (flags.bPositiveBlank)
						*pxf++ = ' ';
					if (flags.bPositiveSign)
						*pxf++ = '+';
					if (precision>=0) {
						*pxf++ = '.';
						*pxf++ = '*';
					}
					*pxf++ = format[-1];
					*pxf = 0;

					if (precision >= 0)
						pbuf += vdswprintf(pbuf, 256, xf, precision, *(const double *)*argv++);
					else
						pbuf += vdswprintf(pbuf, 256, xf, *(const double *)*argv++);
				}
				break;

			case L'n':	// no flags honored; precision ignored
				*(int *)(*argv++) = out.size();
				continue;
			case L'p':	// no flags honored; precision ignored
				pbuf += vdswprintf(pbuf, sizeof buf / sizeof buf[0], L"%p", *(void *const *)*argv++);
				break;

			case L'z':
				switch(*format++) {
				case L's':
					{
						int64 value;

						switch(size) {
						case kShort:	value = *(const short *)*argv++;	break;
						case kDefault:	value = *(const int *)*argv++;		break;
						case kLong:		value = *(const long *)*argv++;		break;
						case kLongLong:	value = *(const int64 *)*argv++;	break;
							break;
						default:
							VDNEVERHERE;
						}

						if (value < 0)
							*pbuf++ = L'-';
						else if (flags.bPositiveSign)
							*pbuf++ = L'+';
						else if (flags.bPositiveBlank)
							*pbuf++ = L' ';

						if (value < (VD64(10) << 10))
							pbuf += vdswprintf(pbuf, (buf + sizeof(buf) / sizeof(buf[0])) - pbuf, L"%d bytes", (int)value);
						else if (value < (VD64(10) << 20))
							pbuf += vdswprintf(pbuf, (buf + sizeof(buf) / sizeof(buf[0])) - pbuf, L"%d KB", (int)((sint32)value >> 10));
						else if (value < (VD64(10) << 30))
							pbuf += vdswprintf(pbuf, (buf + sizeof(buf) / sizeof(buf[0])) - pbuf, L"%d MB", (int)((sint32)value >> 20));
						else if (value < (VD64(10) << 40))
							pbuf += vdswprintf(pbuf, (buf + sizeof(buf) / sizeof(buf[0])) - pbuf, L"%d GB", (int)(value >> 30));
						else
							pbuf += vdswprintf(pbuf, (buf + sizeof(buf) / sizeof(buf[0])) - pbuf, L"%d TB", (int)(value >> 40));
					}

					break;
				}
				break;

			}

			int string_width = (pbuf - pbuf0) + zero_pad;
			int string_delta = width - string_width;

			if (!flags.bLeftAlign && string_delta > 0) {
				int siz = out.size();
				out.resize(siz + string_delta, flags.bZeroPad ? L'0' : L' ');
			}

			if (zero_pad) {
				int siz = out.size();
				out.resize(siz + zero_pad);
				std::fill(&out[siz], &out[siz+zero_pad], L'0');
			}

			if (pbuf != pbuf0) {
				int siz = out.size();
				out.resize(siz + (pbuf - pbuf0));

				std::copy(pbuf0, pbuf, &out[siz]);
			}

			if (flags.bLeftAlign && string_delta > 0) {
				int siz = out.size();
				out.resize(siz + string_delta);
				std::fill(&out[siz], &out[siz+string_delta], L' ');
			}
		}
	}

	out.push_back(0);

	return VDStringW(out.data());
}

VDStringW VDvswprintf(const wchar_t *format, int args, va_list val) {
	if (args < 16) {
		const void *argv[16];

		for(int i=0; i<args; ++i)
			argv[i] = va_arg(val, const void *);

		va_end(val);

		return VDaswprintf(format, args, argv);
	} else {
		vdblock<const void *> argv(args);

		for(int i=0; i<args; ++i)
			argv[i] = va_arg(val, const void *);

		va_end(val);

		return VDaswprintf(format, args, argv.data());
	}
}

VDStringW VDswprintf(const wchar_t *format, int args, ...) {
	va_list val;

	va_start(val, args);
	VDStringW r = VDvswprintf(format, args, val);
	va_end(val);

	return r;
}
