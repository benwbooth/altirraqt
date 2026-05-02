# Tessa - VirtualDub 3D rendering abstraction layer. The top-level layer is
# portable; the D3D11 backend (in source/D3D11/) is Win32-specific and is
# not included here. If/when a real Altirra display driver is wired up on
# Qt (QRhi/OpenGL), a Tessa backend can be written for it.

add_library(tessa STATIC
    source/stdafx.cpp
    source/Config.cpp
    source/Context.cpp
    source/Format.cpp
    source/Options.cpp
    source/Program.cpp
)

target_include_directories(tessa
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(tessa
    PUBLIC
        vd2_system
)
