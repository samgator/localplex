; SPDX-License-Identifier: GPL-3.0-or-later
; installer.nsi: NSIS installer script for localplex on Windows

!include "MUI2.nsh"
!include "FileFunc.nsh"

!ifndef PRODUCT_VERSION
  !define PRODUCT_VERSION "0.1.0"
!endif

!ifndef SOURCE_DIR
  !define SOURCE_DIR "..\..\build-windows\package"
!endif

!ifndef OUT_FILE
  !define OUT_FILE "..\..\dist\localplex-${PRODUCT_VERSION}-setup.exe"
!endif

Name "localplex"
OutFile "${OUT_FILE}"
InstallDir "$PROGRAMFILES64\localplex"
InstallDirRegKey HKLM "Software\localplex" "Install_Dir"
RequestExecutionLevel admin

; UI Configuration
!define MUI_ABORTWARNING
!define MUI_ICON "..\..\assets\icons\localplex.ico"
!define MUI_UNICON "..\..\assets\icons\localplex.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  File /r "${SOURCE_DIR}\*.*"

  ; Write Uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; Registry Keys
  WriteRegStr HKLM "Software\localplex" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\localplex" "DisplayName" "localplex"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\localplex" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\localplex" "Publisher" "samgator"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\localplex" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\localplex" "DisplayIcon" '"$INSTDIR\localplex.exe"'

  ; Shortcuts
  CreateDirectory "$SMPROGRAMS\localplex"
  CreateShortcut "$SMPROGRAMS\localplex\localplex.lnk" "$INSTDIR\localplex.exe"
  CreateShortcut "$SMPROGRAMS\localplex\Uninstall localplex.lnk" "$INSTDIR\uninstall.exe"
  CreateShortcut "$DESKTOP\localplex.lnk" "$INSTDIR\localplex.exe"
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\localplex.lnk"
  Delete "$SMPROGRAMS\localplex\Uninstall localplex.lnk"
  Delete "$SMPROGRAMS\localplex\localplex.lnk"
  RMDir "$SMPROGRAMS\localplex"

  RMDir /r "$INSTDIR"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\localplex"
  DeleteRegKey HKLM "Software\localplex"
SectionEnd
