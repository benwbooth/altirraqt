# ATDevices - concrete device implementations (disk drives, serial devices,
# modems, parallel-to-serial, loopback, test devices, etc.).
# Depends on ATCore + ATNetworkSockets + VDCPP.

add_library(atdevices STATIC
    source/stdafx.cpp
    source/blockdevwritefilter.cpp
    source/computereyes.cpp
    source/corvus.cpp
    source/devices.cpp
    source/diskdrive.cpp
    source/dongle.cpp
    source/exeloader.cpp
    source/loopback.cpp
    source/modemsound.cpp
    source/netserial.cpp
    source/paralleltoserial.cpp
    source/parfilewriter.cpp
    source/serialsplitter.cpp
    source/sioclock.cpp
    source/supersalt.cpp
    source/testdevicesiohs.cpp
    source/testdevicesiopoll.cpp
    source/videogenerator.cpp
)

target_include_directories(atdevices
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/autogen
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(atdevices
    PUBLIC
        atcore
        atnetworksockets
)
