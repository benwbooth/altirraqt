# Avery Lee's BSD-licensed Atari ROMs, built from 6502/65816 assembly with
# Tomasz Biela's MADS cross-assembler. Each rule turns a `.s`/`.xasm` source
# tree into a binary `.rom` blob. The blobs are then wrapped into a Qt
# resource file and consumed by qtoshelper's ATLoadKernelResource.
#
# When -DPREBUILT_KERNEL_ROMS_DIR=<path> is passed (or the
# PREBUILT_KERNEL_ROMS_DIR env var is set), cmake skips MADS entirely and
# copies the pre-built `.rom` blobs from that directory. Used by the
# Flatpak build (where the SDK has neither MADS nor `fpc` to build it).

if(DEFINED ENV{PREBUILT_KERNEL_ROMS_DIR} AND NOT PREBUILT_KERNEL_ROMS_DIR)
    set(PREBUILT_KERNEL_ROMS_DIR "$ENV{PREBUILT_KERNEL_ROMS_DIR}")
endif()

if(NOT PREBUILT_KERNEL_ROMS_DIR)
    find_program(MADS mads REQUIRED
        DOC "Mad-Assembler (6502/65816 cross-assembler) — used to build Atari kernel ROMs from source")
endif()

set(KERNEL_SHARED   ${CMAKE_CURRENT_SOURCE_DIR}/source/Shared)
set(KERNEL_AUTOGEN  ${CMAKE_CURRENT_SOURCE_DIR}/autobuild)
set(KERNEL_VERSIONH ${CMAKE_SOURCE_DIR}/src/Altirra/autobuild_default)

set(KERNEL_OUT_DIR  ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${KERNEL_OUT_DIR})

# Helper to register a single MADS invocation as a build rule.
# Args:
#   tag      - logical name (e.g. "kernelxl")
#   src      - source path (relative to Kernel dir)
#   out      - output filename (e.g. kernelxl.rom)
#   ARGN     - extra mads flags
function(at_mads tag src out)
    set(dst ${KERNEL_OUT_DIR}/${out})
    if(PREBUILT_KERNEL_ROMS_DIR)
        # Copy from the supplied prebuilt directory.
        set(prebuilt "${PREBUILT_KERNEL_ROMS_DIR}/${out}")
        if(NOT EXISTS "${prebuilt}")
            message(FATAL_ERROR "PREBUILT_KERNEL_ROMS_DIR=${PREBUILT_KERNEL_ROMS_DIR} is missing ${out}")
        endif()
        add_custom_command(
            OUTPUT ${dst}
            COMMAND ${CMAKE_COMMAND} -E copy "${prebuilt}" "${dst}"
            DEPENDS "${prebuilt}"
            COMMENT "Copy prebuilt ROM ${out}"
        )
    else()
        add_custom_command(
            OUTPUT ${dst}
            COMMAND ${MADS}
                -i:${KERNEL_SHARED}
                -i:${KERNEL_AUTOGEN}
                -i:${KERNEL_VERSIONH}
                ${ARGN}
                -o:${dst}
                ${CMAKE_CURRENT_SOURCE_DIR}/${src}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src}
            COMMENT "MADS ${out}"
        )
    endif()
    set(KERNEL_ROM_${tag} ${dst} PARENT_SCOPE)
    list(APPEND KERNEL_ROM_FILES ${dst})
    set(KERNEL_ROM_FILES ${KERNEL_ROM_FILES} PARENT_SCOPE)
endfunction()

# Core ROMs — single-pass MADS builds. The peripheral firmwares (850
# modem handler, 1030 modem, Rapidus accelerator, Ultimate1MB BIOS) need a
# separate LZ-pack step using Altirra's own ATCompiler tool — those are
# deferred to a later port pass.
# MADS uses $XXXX for hex literals. CMake's `$` needs single escape outside
# VERBATIM mode (just `-b:$d800` works because CMake doesn't expand non-${...}
# `$` references). Without VERBATIM, ninja writes the args literally.
# Use decimal addresses to avoid CMake/shell `$` escape pain.
# $a000=40960  $b000=45056  $c000=49152  $d800=55296  $f800=63488
at_mads(kernel       source/main.xasm                kernel.rom        -d:_KERNEL_XLXE=0 -s -p -b:55296)
at_mads(kernelxl     source/main.xasm                kernelxl.rom      -d:_KERNEL_XLXE=1 -s -p -b:49152)
at_mads(kernel816    source/main.xasm                kernel816.rom     -d:_KERNEL_XLXE=1 -d:_KERNEL_816=1 -s -p -b:49152)
at_mads(nokernel     source/NoKernel/main.xasm       nokernel.rom      -s -p -b:49152)
at_mads(superkernel  source/SuperKernel/main.xasm    superkernel.rom   -s -p -b:63488)
at_mads(nocartridge  source/NoCartridge/main.s       nocartridge.rom   -s -p -b:45056)
at_mads(nohdbios     source/NoHDBIOS/main.s          nohdbios.rom      -s -p -b:55296)
at_mads(nogame       source/NoGame/main.s            nogame.rom        -s -p -b:40960)
at_mads(nomio        source/Placeholders/nomio.s     nomio.rom         -s -p)
at_mads(noblackbox   source/Placeholders/noblackbox.s noblackbox.rom   -s -p)

add_custom_target(altirra_roms ALL DEPENDS ${KERNEL_ROM_FILES})
set(KERNEL_ROM_FILES "${KERNEL_ROM_FILES}" CACHE INTERNAL "Built ROM file list")
