# VDCPP system library (Qt port).
#
# Original location: src/system in the Altirra tree.
#
# Files are added here as they are translated from Win32 to POSIX/Qt.
# Files that still have unresolved Win32 dependencies are left out until ported.

add_library(vd2_system STATIC
    source/stdafx.cpp
    source/binary.cpp
    source/bitmath.cpp
    source/cpuaccel.cpp
    source/constexpr.cpp
    source/date.cpp
    source/debug.cpp
    source/error.cpp
    source/error_qt.cpp
    source/file.cpp
    source/filestream.cpp
    source/filesys.cpp
    source/Fraction.cpp
    source/event.cpp
    source/function.cpp
    source/halffloat.cpp
    source/hash.cpp
    source/int128.cpp
    source/linearalloc.cpp
    source/math.cpp
    source/memory.cpp
    source/process.cpp
    source/refcount.cpp
    source/registry.cpp
    source/registrymemory.cpp
    source/stdaccel.cpp
    source/strutil.cpp
    source/text.cpp
    source/thread.cpp
    source/time.cpp
    source/tls.cpp
    source/vdalloc.cpp
    source/vdstl.cpp
    source/vdstl_hash.cpp
    source/vdstl_hashtable.cpp
    source/VDString.cpp
    source/vectors.cpp
    source/zip.cpp
)

target_link_libraries(vd2_system PRIVATE ZLIB::ZLIB)

target_include_directories(vd2_system
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(vd2_system
    PUBLIC
        Threads::Threads
)

target_compile_definitions(vd2_system
    PUBLIC
        $<$<CONFIG:Debug>:_DEBUG>
)
