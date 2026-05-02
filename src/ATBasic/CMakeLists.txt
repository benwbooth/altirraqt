# Avery Lee's BSD-licensed Altirra BASIC. Same MADS-based build as Kernel.
# When PREBUILT_KERNEL_ROMS_DIR/atbasic.bin exists, copy instead of
# rebuilding (Flatpak bypass; same cmake variable as src/Kernel).

if(DEFINED ENV{PREBUILT_KERNEL_ROMS_DIR} AND NOT PREBUILT_KERNEL_ROMS_DIR)
    set(PREBUILT_KERNEL_ROMS_DIR "$ENV{PREBUILT_KERNEL_ROMS_DIR}")
endif()

set(ATBASIC_OUT ${CMAKE_CURRENT_BINARY_DIR}/atbasic.bin)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

if(PREBUILT_KERNEL_ROMS_DIR AND EXISTS "${PREBUILT_KERNEL_ROMS_DIR}/atbasic.bin")
    add_custom_command(
        OUTPUT ${ATBASIC_OUT}
        COMMAND ${CMAKE_COMMAND} -E copy
            "${PREBUILT_KERNEL_ROMS_DIR}/atbasic.bin" "${ATBASIC_OUT}"
        DEPENDS "${PREBUILT_KERNEL_ROMS_DIR}/atbasic.bin"
        COMMENT "Copy prebuilt atbasic.bin"
    )
else()
    find_program(MADS mads REQUIRED)
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
endif()

add_custom_target(altirra_basic_rom ALL DEPENDS ${ATBASIC_OUT})
set(ATBASIC_ROM_FILE ${ATBASIC_OUT} CACHE INTERNAL "ATBasic 8K cart ROM")
