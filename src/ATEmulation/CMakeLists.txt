# ATEmulation - device emulation helpers (ACIA, CRTC, CTC, RIOT, VIA, RTCs,
# flash, SCSI, etc.). Depends on ATCore + VDCPP; no Win32 code.

add_library(atemulation STATIC
    source/stdafx.cpp
    source/acia.cpp
    source/acia6850.cpp
    source/crtc.cpp
    source/ctc.cpp
    source/diskutils.cpp
    source/eeprom.cpp
    source/flash.cpp
    source/phone.cpp
    source/riot.cpp
    source/rtcds1305.cpp
    source/rtcmcp7951x.cpp
    source/rtcv3021.cpp
    source/scsi.cpp
    source/via.cpp
)

target_include_directories(atemulation
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(atemulation
    PUBLIC
        atcore
        atio
)
