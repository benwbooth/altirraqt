# vdjson - JSON reader/writer. Depends on VDCPP only.

add_library(vdjson STATIC
    source/stdafx.cpp
    source/jsonnametable.cpp
    source/jsonoutput.cpp
    source/jsonreader.cpp
    source/jsonvalue.cpp
    source/jsonwriter.cpp
)

target_include_directories(vdjson
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(vdjson
    PUBLIC
        vd2_system
)
