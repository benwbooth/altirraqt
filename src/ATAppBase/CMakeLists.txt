# ATAppBase - Atari application base support.
#
# Original location: src/ATAppBase in the Altirra tree.

add_library(atappbase STATIC
    source/stdafx.cpp
    # source/crthooks.cpp       # Win32-only CRT hooks; pending port
    # source/exceptionfilter.cpp # Win32-only unhandled-exception filter; pending port
)

target_include_directories(atappbase
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/h
        ${CMAKE_SOURCE_DIR}/src/h
)

target_link_libraries(atappbase
    PUBLIC
        vd2_system
)
