# Kasumi - VirtualDub image processing (pixel format conversion, blit,
# resample, region ops). Upstream ships hand-written x86 MASM (.asm) and
# ARM64 MASM (.asm64) alongside the C++ reference paths. We skip the
# assembly files entirely — clang takes the C++ fallbacks and the
# intrinsics-based SIMD paths on supported architectures.

set(KASUMI_COMMON_SOURCES
    source/stdafx.cpp
    source/alphablt.cpp
    source/blitter.cpp
    source/blt.cpp
    source/blt_reference.cpp
    source/blt_reference_pal.cpp
    source/blt_reference_rgb.cpp
    source/blt_reference_yuv.cpp
    source/blt_reference_yuv2yuv.cpp
    source/blt_reference_yuvrev.cpp
    source/blt_setup.cpp
    source/blt_spanutils.cpp
    source/blt_uberblit.cpp
    source/pixel.cpp
    source/pixmaputils.cpp
    source/region.cpp
    source/resample.cpp
    source/resample_kernels.cpp
    source/resample_stages.cpp
    source/resample_stages_reference.cpp
    source/stretchblt_reference.cpp
    source/uberblit.cpp
    source/uberblit_16f.cpp
    source/uberblit_gen.cpp
    source/uberblit_resample.cpp
    source/uberblit_resample_special.cpp
    source/uberblit_swizzle.cpp
    source/uberblit_ycbcr_generic.cpp
)

add_library(kasumi STATIC ${KASUMI_COMMON_SOURCES})

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    # Only include SIMD paths that use intrinsics. The *_x86.cpp files use
    # MSVC __declspec(naked) inline asm for 32-bit x86 and won't build on
    # x86_64/clang; the reference C++ paths are used instead.
    target_sources(kasumi PRIVATE
        source/region_sse2.cpp
        source/resample_stages_x64.cpp
        source/uberblit_ycbcr_sse2_intrin.cpp
    )
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    target_sources(kasumi PRIVATE
        source/blt_spanutils_arm64.cpp
        source/region_neon.cpp
        source/resample_stages_arm64.cpp
    )
endif()

target_include_directories(kasumi
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/h
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(kasumi
    PUBLIC
        vd2_system
)
