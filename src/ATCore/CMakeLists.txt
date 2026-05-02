# ATCore - core emulator primitives (scheduling, device glue, serialization,
# numerics, etc). Depends on VDCPP (vd2_system).

add_library(atcore STATIC
    source/stdafx.cpp
    source/address.cpp
    source/asyncdispatcherimpl.cpp
    source/atascii.cpp
    source/bussignal.cpp
    source/checksum.cpp
    source/configvar.cpp
    source/consoleoutput.cpp
    source/crc.cpp
    source/decmath.cpp
    source/deferredevent.cpp
    source/deviceimpl.cpp
    source/deviceparentimpl.cpp
    source/devicesioimpl.cpp
    source/devicestorageimpl.cpp
    source/enumparse.cpp
    source/fft.cpp
    source/fft_scalar.cpp
    source/logging.cpp
    source/md5.cpp
    source/memoryutils.cpp
    source/notifylist.cpp
    source/profile.cpp
    source/progress.cpp
    source/propertyset.cpp
    source/randomization.cpp
    source/savestate.cpp
    source/scheduler.cpp
    source/serialization.cpp
    source/sioutils.cpp
    source/snapshotimpl.cpp
    source/vfs.cpp
    # Arch-specific SIMD variants:
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},AMD64>>:source/checksum_intrin.cpp>
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},AMD64>>:source/fft_sse2.cpp>
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},AMD64>>:source/fft_avx2.cpp>
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},aarch64>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},arm64>>:source/checksum_arm64.cpp>
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},aarch64>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},arm64>>:source/fft_neon.cpp>
    # timerserviceimpl_win32.cpp is Win32-only; replacement POSIX version pending.
)

target_include_directories(atcore
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(atcore
    PUBLIC
        vd2_system
        atappbase
)
