cmake_minimum_required(VERSION 3.21)

project(altirraqt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Multimedia Network)
set(CMAKE_AUTOMOC ON)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

include(CTest)
include(GNUInstallDirs)

add_subdirectory(src)

# ---------------------------------------------------------------------------
# Install: altirraqt binary, .desktop launcher, application icon, manpage.
# Uses GNUInstallDirs so distros can override DATAROOTDIR / BINDIR.
# ---------------------------------------------------------------------------

set_target_properties(altirraqt PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH FALSE
    SKIP_BUILD_RPATH FALSE)

install(TARGETS altirraqt
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION ${CMAKE_INSTALL_BINDIR})   # macOS .app

install(FILES data/applications/altirraqt.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

install(FILES data/icons/altirraqt.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps)

install(FILES data/man/altirraqt.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

install(FILES data/metainfo/com.virtualdub.altirraqt.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)

install(FILES LICENSE AUTHORS
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/altirraqt)
