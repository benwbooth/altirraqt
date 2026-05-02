# Altirra - main emulator core. 220 non-UI/non-Win32 source files plus
# architecture-specific SIMD artifacting code. UI sources (ui*.cpp) and the
# *_win32.cpp shims are deferred to a separate Qt UI pass.

set(ALTIRRA_COMMON_SOURCES
    source/1025full.cpp
    source/1029full.cpp
    source/1030.cpp
    source/1030full.cpp
    source/1090.cpp
    source/1400xl.cpp
    source/1450xld.cpp
    source/820full.cpp
    source/850full.cpp
    # about.cpp - About dialog, UI-dependent
    # allocator.cpp - MSVC CRT debug allocator wrappers (_DEBUG only)

    source/antic.cpp
    source/artifacting.cpp
    source/artifacting_filters.cpp
    source/artifacting_pal_scalar.cpp
    source/assembler.cpp
    source/audiomonitor.cpp
    source/audiorawsource.cpp
    source/audiosampleplayer.cpp
    source/audiostocksamples.cpp
    source/audiowriter.cpp
    source/autosavemanager.cpp
    source/aviwriter.cpp
    source/bit3.cpp
    source/bkptmanager.cpp
    source/blackbox.cpp
    source/blackboxfloppy.cpp
    source/blockdevdiskadapter.cpp
    source/blockdevvirtfat32.cpp
    # browser.cpp - UI
    source/cartdetect.cpp
    source/cartridge.cpp
    source/cartridgeport.cpp
    source/cassette.cpp
    source/cheatengine.cpp
    # cmd*.cpp — all command implementations depend on the UI command manager
    # (at/atui/uicommandmanager.h) and Dita services. Re-enable these when
    # porting the UI layer.
    source/common_png.cpp
    source/compatdb.cpp
    source/compatedb.cpp
    source/compatengine.cpp
    # console.cpp — debugger console UI, hooks atnativeui
    source/constants.cpp
    source/covox.cpp
    source/cpu6502.cpp
    source/cpu6502ill.cpp
    source/cpu65c02.cpp
    source/cpu65c816.cpp
    source/cpu.cpp
    source/cpuheatmap.cpp
    source/cpuhookmanager.cpp
    source/cputracer.cpp
    source/cs8900a.cpp
    source/browser.cpp
    # customdevice.cpp — depends on customdevicevmtypes.cpp + atvm + atui;
    # the full scripting VM port is tracked under PLAN.md "Genuinely
    # deferred" (it's not on the active checklist).
    # customdevicevmtypes.cpp — pulls in atui
    source/debugdisplay.cpp
    # debuggerautotest.cpp — atui-dependent
    # debugger.cpp — upstream includes <at/atnativeui/uiframe.h>; replaced by
    # a portable shim in src/h/at/atnativeui/uiframe.h that declares only the
    # one symbol debugger.cpp actually references (ATActivateUIPane). That
    # symbol plus ATConsoleCheckBreak() get no-op definitions in stubs.cpp.
    # No upstream-source edits required.
    source/debugger.cpp
    source/debuggerexp.cpp
    source/debuggerlog.cpp
    source/debuggersettings.cpp
    source/debugtarget.cpp
    source/decmath.cpp
    source/decode_png.cpp
    source/devicemanager.cpp
    source/devices.cpp
    # devxcmd*.cpp — device-extension commands, atnativeui dependencies
    # directorywatcher.cpp — Win32 ReadDirectoryChangesW; needs inotify port

    source/disasm.cpp
    source/disk.cpp
    source/diskdrive815.cpp
    source/diskdriveamdc.cpp
    source/diskdriveatr8000.cpp
    source/diskdrivefullbase.cpp
    source/diskdrivefull.cpp
    source/diskdriveindusgt.cpp
    source/diskdrivepercom.cpp
    source/diskdrivespeedyxf.cpp
    source/diskdrivexf551.cpp
    source/diskinterface.cpp
    source/diskprofile.cpp
    source/disktrace.cpp
    source/diskvirtimagebase.cpp
    source/diskvirtimage.cpp
    source/diskvirtimagesdfs.cpp
    source/dragoncart.cpp
    source/encode_png.cpp
    source/errordecode.cpp
    source/fdc.cpp
    source/firmwaredetect.cpp
    source/firmwaremanager.cpp
    source/gtia.cpp
    source/gtiarenderer.cpp
    source/gtiatables.cpp
    source/hlebasicloader.cpp
    source/hleciohook.cpp
    source/hlefastboothook.cpp
    source/hlefpaccelerator.cpp
    source/hleprogramloader.cpp
    source/hleutils.cpp
    source/hostdevice.cpp
    source/hostdeviceutils.cpp
    source/ide.cpp
    source/idediskdevices.cpp
    # idephysdisk.cpp — direct physical disk access; Win32-specific

    source/iderawimage.cpp
    # idevhdimage.cpp — VHD uses UTF-16; needs char16_t port for non-Windows wchar_t

    source/inputcontroller.cpp
    source/inputdefs.cpp
    source/inputmanager.cpp
    source/inputmap.cpp
    source/irqcontroller.cpp
    # joystick.cpp — DirectInput; defer (Qt gamepad later)

    source/kmkjzide.cpp
    # leakdetector.cpp — Win32 SymInitialize

    # main.cpp — Win32 WinMain; replaced by Qt entry point later
    source/memorymanager.cpp
    # midimate.cpp — Win32 MIDI API; defer

    source/mio.cpp
    source/misccvars.cpp
    source/mmu.cpp
    source/modem.cpp
    # modemtcp.cpp — Win32 event-driven thread glue; defer

    source/mpp1000e.cpp
    source/multiplexer.cpp
    source/myide.cpp
    source/options.cpp
    # oshelper.cpp — Win32 shell helpers; replaced by Qt equivalents later
    # ostracing.cpp — Win32 ETW tracing

    source/palettegenerator.cpp
    source/palettesolver.cpp
    source/pbi.cpp
    source/pbidisk.cpp
    source/pclink.cpp
    source/pia.cpp
    source/pocketmodem.cpp
    source/pokeysavecompat.cpp
    source/pokeytrace.cpp
    source/portmanager.cpp
    source/printer1020.cpp
    source/printer1020font.cpp
    source/printerbase.cpp
    source/printer.cpp
    source/printerfont.cpp
    source/printeroutput.cpp
    source/printerttfencoder.cpp
    source/profiler.cpp
    # profilerui.cpp — UI panel, atnativeui dependency
    source/rapidus.cpp
    source/rs232.cpp
    source/rtime8.cpp
    source/rverter.cpp
    source/sapconverter.cpp
    source/sapwriter.cpp
    source/savestate.cpp
    source/savestateio.cpp
    source/savestatetypes.cpp
    source/scsidisk.cpp
    source/sdrive.cpp
    # settings.cpp — registry-backed settings dialogs, UI-dependent
    source/side3.cpp
    source/side.cpp
    source/simeventmanager.cpp
    source/simulator.cpp
    source/sio2sd.cpp
    source/siomanager.cpp
    source/slightsid.cpp
    source/soundboard.cpp
    # startuplogger.cpp — Win32 early logging
    source/stdafx.cpp
    source/sx212.cpp
    source/textdom.cpp
    # texteditor.cpp — UI widget
    source/thepill.cpp
    source/trace.cpp
    source/tracecpu.cpp
    source/tracefileencoding.cpp
    source/tracefileformat.cpp
    source/traceimporta800.cpp
    source/traceio.cpp
    # tracenative.cpp links via at/atnativeui/debug_win32.h, which the Qt
    # port stubs to a single "return nullptr" symbol provided by
    # altirraqt/debug_win32_stub.cpp. The tracer itself is portable.
    source/tracenative.cpp
    source/tracetape.cpp
    source/tracevideo.cpp
    source/ultimate1mb.cpp
    # updatecheck.cpp / updatefeed.cpp — Win32 update network path

    source/vbxe.cpp
    source/vbxestate.cpp
    source/verifier.cpp
    source/veronica.cpp
    source/vgmplayer.cpp
    # vgmwriter.cpp — GD3 requires UTF-16; needs char16_t port for non-Windows wchar_t
    source/videomanager.cpp
    source/videostillimage.cpp
    # videowriter.cpp — Win32 IMFMediaSink / MFStartup; defer

    source/virtualscreen.cpp
    source/warpos.cpp
    source/xelcf.cpp
    source/xep80.cpp
)

add_library(altirra_core STATIC ${ALTIRRA_COMMON_SOURCES})

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    target_sources(altirra_core PRIVATE
        source/artifacting_ntsc_sse2.cpp
        source/artifacting_ntsc_sse2_intrin.cpp
        source/artifacting_pal_sse2.cpp
        source/artifacting_pal_sse2_intrin.cpp
        source/artifacting_sse2_intrin.cpp
    )
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    target_sources(altirra_core PRIVATE
        source/artifacting_neon.cpp
        source/artifacting_ntsc_neon.cpp
        source/artifacting_pal_neon.cpp
    )
endif()

target_include_directories(altirra_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/h
        ${CMAKE_CURRENT_SOURCE_DIR}/h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/autogen
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(altirra_core
    PUBLIC
        atcore
        atcpu
        atio
        ataudio
        atdevices
        atemulation
        atnetwork
        atnetworksockets
        atdebugger
        atvm
        kasumi
        vddisplay
        vdjson
        dita
)
