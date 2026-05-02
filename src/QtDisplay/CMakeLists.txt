# QtDisplay - Qt-backed display driver for Altirra.
#
# Provides a QOpenGLWidget-based implementation of IVDVideoDisplay. The widget
# is a regular Qt child widget that accepts pixmap frames via the
# IVDVideoDisplay interface and renders them with an OpenGL textured quad.
# This replaces the upstream Win32 IVDVideoDisplayMinidriver stack
# (displaydrvd3d9.cpp / displaygdi.cpp / displaydrv3d.cpp).

add_library(qtdisplay STATIC
    source/qtvideodisplay.cpp
)

target_include_directories(qtdisplay
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(qtdisplay
    PUBLIC
        vddisplay
        kasumi
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
)
