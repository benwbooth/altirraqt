; NSIS installer script for altirraqt.
;
; Invoked from .github/workflows/build.yml as:
;   makensis -DSRCDIR=dist/altirraqt -DOUTFILE=altirraqt-windows-x86_64-installer.exe \
;            -DAPPVERSION=$(git describe --tags --always) \
;            packaging/windows/altirraqt.nsi
;
; SRCDIR points at the directory windeployqt populated (altirraqt.exe + Qt DLLs).

!ifndef SRCDIR
  !define SRCDIR "..\..\dist\altirraqt"
!endif
!ifndef OUTFILE
  !define OUTFILE "altirraqt-windows-x86_64-installer.exe"
!endif
!ifndef APPVERSION
  !define APPVERSION "dev"
!endif

!define APPNAME "altirraqt"
!define COMPANY "VirtualDub"
!define WEBSITE "https://github.com/benwbooth/altirraqt"

Name "${APPNAME} ${APPVERSION}"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

; --- Pages ---------------------------------------------------------------
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

; --- Install -------------------------------------------------------------
Section "Install"
  SetOutPath "$INSTDIR"
  File /r "${SRCDIR}\*.*"

  ; Registry entries for Add/Remove Programs.
  WriteRegStr HKLM "Software\${APPNAME}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\${APPNAME}" "Version"    "${APPVERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                  "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                  "DisplayVersion" "${APPVERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                  "Publisher" "${COMPANY}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                  "URLInfoAbout" "${WEBSITE}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                  "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                  "DisplayIcon" "$\"$INSTDIR\altirraqt.exe$\""

  ; Shortcuts.
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\altirraqt.exe"
  CreateShortcut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\altirraqt.exe"

  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; --- Uninstall -----------------------------------------------------------
Section "Uninstall"
  Delete "$DESKTOP\${APPNAME}.lnk"
  RMDir /r "$SMPROGRAMS\${APPNAME}"
  RMDir /r "$INSTDIR"

  DeleteRegKey HKLM "Software\${APPNAME}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
SectionEnd
