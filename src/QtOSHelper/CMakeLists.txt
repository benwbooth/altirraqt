# QtOSHelper - Qt/POSIX implementations of Altirra's oshelper.h API.
# Provides clipboard, image I/O, URL launch, file attributes, GUID gen, etc.
# Also embeds Avery Lee's BSD-licensed Atari ROMs via Qt resources, so
# ATLoadKernelResource() can return them by IDR_* id.

set(KERNEL_BUILD_DIR  ${CMAKE_BINARY_DIR}/src/Kernel)
set(ATBASIC_BUILD_DIR ${CMAKE_BINARY_DIR}/src/ATBasic)

# Generate a .qrc with absolute paths to the freshly-built ROMs, then run
# rcc on it. Using qt_add_resources with absolute paths embeds via the
# /altirra/roms/<name> resource prefix.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/altirra_roms.qrc.in
    ${CMAKE_CURRENT_BINARY_DIR}/altirra_roms.qrc
    @ONLY
)

# Tell CMake the qrc inputs depend on the ROM build artifacts so rcc reruns.
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/altirra_roms.qrc
    PROPERTY OBJECT_DEPENDS
        ${KERNEL_BUILD_DIR}/kernel.rom
        ${KERNEL_BUILD_DIR}/kernelxl.rom
        ${KERNEL_BUILD_DIR}/kernel816.rom
        ${KERNEL_BUILD_DIR}/nokernel.rom
        ${KERNEL_BUILD_DIR}/superkernel.rom
        ${KERNEL_BUILD_DIR}/nocartridge.rom
        ${KERNEL_BUILD_DIR}/nohdbios.rom
        ${KERNEL_BUILD_DIR}/nogame.rom
        ${KERNEL_BUILD_DIR}/nomio.rom
        ${KERNEL_BUILD_DIR}/noblackbox.rom
        ${ATBASIC_BUILD_DIR}/atbasic.bin
)

qt_add_resources(QTOSHELPER_ROM_RESOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/altirra_roms.qrc
)

add_library(qtoshelper STATIC
    source/oshelper.cpp
    ${QTOSHELPER_ROM_RESOURCES}
)

# Make sure the ROMs are built before we try to embed them.
add_dependencies(qtoshelper altirra_roms altirra_basic_rom)

target_include_directories(qtoshelper
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_SOURCE_DIR}/src/Altirra/h
        ${CMAKE_SOURCE_DIR}/src/Altirra/source  # for resource.h (Win32 IDR_* defines)
)

target_link_libraries(qtoshelper
    PUBLIC
        vd2_system
        atcore
        kasumi
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
