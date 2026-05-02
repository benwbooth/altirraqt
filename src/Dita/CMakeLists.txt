# Dita - VirtualDub UI services (accelerator tables, file/color dialogs).
# Upstream services.cpp is Win32-only; we provide a Qt-backed services_qt.cpp
# instead. accel.cpp is portable and ports verbatim.

add_library(dita STATIC
    source/stdafx.cpp
    source/accel.cpp
    source/accel_qt.cpp
    source/services_qt.cpp
)

target_include_directories(dita
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(dita
    PUBLIC
        vd2_system
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
