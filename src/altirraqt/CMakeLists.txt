# altirraqt - top-level Qt application (currently a link-smoke-test shell).

qt_add_resources(ALTIRRAQT_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/altirraqt.qrc)

add_executable(altirraqt WIN32 MACOSX_BUNDLE
    ${ALTIRRAQT_RESOURCES}
    main.cpp
    menus.cpp
    compat_qt.cpp
    compatdatabasedialog.cpp
    diskdrivesdialog.cpp
    adjustcolorsdialog.cpp
    tapecontroldialog.cpp
    cheaterdialog.cpp
    lightpendialog.cpp
    lightpenrecalibrate.cpp
    customizehuddialog.cpp
    calibratedialog.cpp
    configsystemdialog.cpp
    diskexplorerdialog.cpp
    audiorecorder.cpp
    videorecorder.cpp
    sapwriter_qt.cpp
    vgmwriter.cpp
    audiolevelsdialog.cpp
    advancedconfigdialog.cpp
    firsttimewizard.cpp
    printerdialog.cpp
    profileseditordialog.cpp
    tapeeditordialog.cpp
    joystickkeys.cpp
    joystickkeysdialog.cpp
    debuggerconsolepane.cpp
    debuggerregisterspane.cpp
    debuggerdisassemblypane.cpp
    debuggermemorypane.cpp
    debuggerwatchpane.cpp
    debuggerbreakpointspane.cpp
    debuggerhistorypane.cpp
    debuggersourcepane.cpp
    helpdialog.cpp
    keyboardlayoutdialog.cpp
    qt_register_devices.cpp
    idephysdisk_stub.cpp
    win32_compat.cpp
    altvideodialog.cpp
    genericdialog_qt.cpp
    screentext.cpp
    stubs.cpp
    timerservice_qt.cpp
    directorywatcher_qt.cpp
    modemdriver_qt.cpp
    debug_win32_stub.cpp
)

target_include_directories(altirraqt
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/Altirra/h
)

# altirra_core, qtoshelper and dita have circular static-archive references.
# GNU ld / lld need an explicit --start-group/--end-group pass; MSVC link.exe
# and Apple ld64 already do iterative archive resolution and reject the flag.
target_link_libraries(altirraqt
    PRIVATE
        qtdisplay
        $<$<NOT:$<OR:$<PLATFORM_ID:Windows>,$<PLATFORM_ID:Darwin>>>:-Wl,--start-group>
        altirra_core
        qtoshelper
        dita
        $<$<NOT:$<OR:$<PLATFORM_ID:Windows>,$<PLATFORM_ID:Darwin>>>:-Wl,--end-group>
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
)
