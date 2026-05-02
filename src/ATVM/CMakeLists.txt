# ATVM - scripting VM used by Altirra's built-in device scripting.
# Depends on VDCPP only.

add_library(atvm STATIC
    source/stdafx.cpp
    source/compiler.cpp
    source/vm.cpp
)

target_include_directories(atvm
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(atvm
    PUBLIC
        vd2_system
)
