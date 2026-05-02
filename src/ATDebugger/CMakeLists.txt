# ATDebugger - debugger engine (breakpoints, symbol files, history tree).
# Depends on ATCore + ATCPU + VDCPP.

add_library(atdebugger STATIC
    source/stdafx.cpp
    source/breakpointsimpl.cpp
    source/debugdevice.cpp
    source/defsymbols.cpp
    source/historytree.cpp
    source/historytreebuilder.cpp
    source/symbolio.cpp
    source/symbols.cpp
    source/target.cpp
)

target_include_directories(atdebugger
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
)

target_link_libraries(atdebugger
    PUBLIC
        atcore
        atcpu
)
