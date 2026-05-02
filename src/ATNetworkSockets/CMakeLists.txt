# ATNetworkSockets - bridges ATNetwork's virtual IP stack to host sockets.
# Upstream implementation is Winsock+WSAAsyncSelect+hidden-window, which is
# Win32-specific. Provides POSIX stubs so linkage works; a real POSIX port
# using select()/epoll in a worker thread can replace nativesockets_stub.cpp
# later.

add_library(atnetworksockets STATIC
    source/stdafx.cpp
    source/nativesockets_stub.cpp
)

target_include_directories(atnetworksockets
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(atnetworksockets
    PUBLIC
        atcore
        atnetwork
)
