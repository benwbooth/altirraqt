# ATIO - disk/cassette/cartridge image I/O, audio decoders (FLAC, vorbis, wav).
# Depends on ATCore + VDCPP.

add_library(atio STATIC
    source/stdafx.cpp
    source/atfs.cpp
    source/audioreader.cpp
    source/audioreaderflac.cpp
    source/audioreadervorbis.cpp
    source/audioreaderwav.cpp
    source/audioutils.cpp
    source/blobimage.cpp
    source/cartridgeimage.cpp
    source/cartridgetypes.cpp
    source/cassetteaudiofilters.cpp
    source/cassetteblock.cpp
    source/cassettedecoder.cpp
    source/cassetteimage.cpp
    source/diskfs.cpp
    source/diskfsarc.cpp
    source/diskfscpm.cpp
    source/diskfsdos2.cpp
    source/diskfsdos3.cpp
    source/diskfssdx2.cpp
    source/diskfssdx2util.cpp
    source/diskfsutil.cpp
    source/diskimage.cpp
    source/image.cpp
    source/partitiondiskview.cpp
    source/partitiontable.cpp
    source/programimage.cpp
    source/savestate.cpp
    source/tapepiecetable.cpp
    source/vorbisbitreader.cpp
    source/vorbisdecoder.cpp
    source/vorbismisc.cpp
    source/wav.cpp
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},AMD64>>:source/audioreaderflac_x86.cpp>
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},aarch64>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},arm64>>:source/audioreaderflac_arm64.cpp>
)

target_include_directories(atio
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/autogen
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(atio
    PUBLIC
        atcore
)
