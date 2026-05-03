// Slow scalar fallback implementations for the SSE2-MASM symbols upstream
// Altirra ships as hand-written x86-64 assembly. We don't carry the .asm
// here, so on toolchains that don't dead-strip the unresolved references
// from resample_stages_x64.cpp (notably MinGW's ld.lld) we link these
// in instead.
//
// They are slow and only used when MinGW's lld actually keeps the call
// edges live; on Linux/lld and macOS/ld64 the calling Process() functions
// are dead-stripped before this matters.

#include <stdafx.h>
#include <vd2/system/vdtypes.h>

#if defined(_WIN32) && (defined(__x86_64__) || defined(_M_X64))

extern "C" long __cdecl vdasm_resize_table_col_SSE2(
    uint32 *out, const uint32 *const *in_table,
    const sint32 *filter, int filter_width, uint32 w)
{
    for (uint32 x = 0; x < w; ++x) {
        sint32 r = 0x2000, g = 0x2000, b = 0x2000, a = 0x2000;
        for (int t = 0; t < filter_width; ++t) {
            const uint32 px = in_table[t][x];
            const sint32 c = filter[t * 4];
            r += (sint32)((px >> 16) & 0xff) * c;
            g += (sint32)((px >> 8 ) & 0xff) * c;
            b += (sint32)((px      ) & 0xff) * c;
            a += (sint32)((px >> 24) & 0xff) * c;
        }
        auto clamp8 = [](sint32 v){ v >>= 14; return (uint32)(v < 0 ? 0 : v > 255 ? 255 : v); };
        out[x] = (clamp8(a) << 24) | (clamp8(r) << 16) | (clamp8(g) << 8) | clamp8(b);
    }
    return (long)w;
}

extern "C" long __cdecl vdasm_resize_table_row_SSE2(
    uint32 *out, const uint32 *in,
    const sint32 *filter, int filter_width, uint32 w,
    long accum, long frac)
{
    long u = accum;
    for (uint32 x = 0; x < w; ++x) {
        const sint32 *fk = filter + ((u >> 8) & 0xff) * (filter_width * 4);
        const uint32 *src = in + (u >> 16);
        sint32 r = 0x2000, g = 0x2000, b = 0x2000, a = 0x2000;
        for (int t = 0; t < filter_width; ++t) {
            const uint32 px = src[t];
            const sint32 c = fk[t * 4];
            r += (sint32)((px >> 16) & 0xff) * c;
            g += (sint32)((px >> 8 ) & 0xff) * c;
            b += (sint32)((px      ) & 0xff) * c;
            a += (sint32)((px >> 24) & 0xff) * c;
        }
        auto clamp8 = [](sint32 v){ v >>= 14; return (uint32)(v < 0 ? 0 : v > 255 ? 255 : v); };
        out[x] = (clamp8(a) << 24) | (clamp8(r) << 16) | (clamp8(g) << 8) | clamp8(b);
        u += frac;
    }
    return u;
}

#endif
