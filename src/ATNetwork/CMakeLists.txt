# ATNetwork - protocol stack (IP/TCP/UDP/DHCP, virtual ethernet bus).
# Depends on ATCore + VDCPP; no host socket API.

add_library(atnetwork STATIC
    source/stdafx.cpp
    source/dhcpd.cpp
    source/ethernetbus.cpp
    source/ethernetframe.cpp
    source/gatewayserver.cpp
    source/ipstack.cpp
    source/socket.cpp
    source/tcp.cpp
    source/tcpstack.cpp
    source/udp.cpp
    source/udpstack.cpp
)

target_include_directories(atnetwork
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(atnetwork
    PUBLIC
        atcore
)
