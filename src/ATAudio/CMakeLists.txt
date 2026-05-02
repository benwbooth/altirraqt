# ATAudio - POKEY emulation + audio mixer/output.
# Depends on ATCore + VDCPP. Win32 audio backends are stubbed out via
# audiooutnull.cpp; a real Qt backend can be wired in later.

add_library(ataudio STATIC
    source/stdafx.cpp
    source/audioconvolutionplayer.cpp
    source/audiofilters.cpp
    source/audiooutqt.cpp
    source/audiooutput.cpp
    source/audiosamplebuffer.cpp
    source/audiosampleplayer.cpp
    source/audiosamplepool.cpp
    source/pokey.cpp
    source/pokeyrenderer.cpp
    source/pokeysavestate.cpp
    source/pokeytables.cpp
)

target_include_directories(ataudio
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(ataudio
    PUBLIC
        atcore
        Qt6::Multimedia
)
