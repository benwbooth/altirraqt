# Riza - VirtualDub helper library, bitmap handling. Portable C++.

add_library(riza STATIC
    source/stdafx.cpp
    source/bitmap.cpp
)

target_include_directories(riza
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(riza
    PUBLIC
        vd2_system
)
