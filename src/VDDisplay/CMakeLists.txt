# VDDisplay - VirtualDub display library. Only the portable rendering/effects
# core is built here; the Win32-specific backends (GDI, D3D9, D3D11) are
# skipped. A Qt-native display driver is added separately.

add_library(vddisplay STATIC
    source/stdafx.cpp
    source/bicubic.cpp
    source/bloom.cpp
    source/customeffectbase.cpp
    source/customeffectpassbase.cpp
    source/customeffectutils.cpp
    source/displaynode3d.cpp
    source/displaytypes.cpp
    source/fontbitmap.cpp
    source/logging.cpp
    source/minid3dx.cpp
    source/options.cpp
    source/rendercache.cpp
    source/renderer.cpp
    source/renderer3d.cpp
    source/renderersoft.cpp
    source/screenfx.cpp
    source/textrenderer.cpp
)

target_include_directories(vddisplay
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/h
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        ${CMAKE_CURRENT_SOURCE_DIR}/autogen
)

target_link_libraries(vddisplay
    PUBLIC
        vd2_system
        kasumi
)
