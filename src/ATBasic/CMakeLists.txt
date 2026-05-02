# Avery Lee's BSD-licensed Altirra BASIC. Same MADS-based build as Kernel.

find_program(MADS mads REQUIRED)

set(ATBASIC_OUT ${CMAKE_CURRENT_BINARY_DIR}/atbasic.bin)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(
    OUTPUT ${ATBASIC_OUT}
    COMMAND ${MADS}
        -c -s -d:CART=1 -b:40960
        -i:${CMAKE_CURRENT_SOURCE_DIR}/source
        -o:${ATBASIC_OUT}
        ${CMAKE_CURRENT_SOURCE_DIR}/source/atbasic.s
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/atbasic.s
    COMMENT "MADS atbasic.bin (8K cart)"
)

add_custom_target(altirra_basic_rom ALL DEPENDS ${ATBASIC_OUT})
set(ATBASIC_ROM_FILE ${ATBASIC_OUT} CACHE INTERNAL "ATBasic 8K cart ROM")
