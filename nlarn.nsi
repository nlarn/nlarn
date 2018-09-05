;
; nlarn.nsi
;
; Copyright (C) Joachim de Groot 2009-2018  <jdegroot@web.de>
;
; NLarn is free software: you can redistribute it and/or modify it
; under the terms of the GNU General Public License as published by the
; Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; NLarn is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
; See the GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License along
; with this program.  If not, see <http://www.gnu.org/licenses/>.
;

;Include Modern UI
!include "MUI2.nsh"

; check if NLarn version number has been defined
!ifndef VERSION
  !error "VERSION has not been defined"
!endif

; The name of the installer
Name "NLarn ${VERSION}"

; The file to write
OutFile "nlarn-${VERSION}.exe"

; Compression
SetCompressor /SOLID lzma

; The default installation directory
InstallDir $PROGRAMFILES\NLarn

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NLarn" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
;Interface Configuration

!define MUI_ICON "resources/nlarn-48.ico"

;--------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


;--------------------------------
; Localisation
!insertmacro MUI_LANGUAGE "English"


;--------------------------------
; Installer executable version information
VIProductVersion "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "NLarn"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "Joachim de Groot"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright 2009-2018 Joachim de Groot"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "NLarn Installation Program"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION}"


;--------------------------------
; The stuff to install
Section "NLarn (required)"

  SectionIn RO

  ; root directory
  SetOutPath $INSTDIR
  !include "mainfiles.nsh"

  ; lib subdirectory
  SetOutPath "$INSTDIR\lib"
  !include "libfiles.nsh"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NLarn "Install_Dir" "$INSTDIR"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NLarn" "DisplayName" "NLarn ${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NLarn" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NLarn" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NLarn" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\NLarn"
  CreateShortCut "$SMPROGRAMS\NLarn\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  CreateShortCut "$SMPROGRAMS\NLarn\NLarn.lnk" "$INSTDIR\nlarn.exe"
  CreateShortCut "$SMPROGRAMS\NLarn\README.lnk" "$INSTDIR\README.html"
  CreateShortCut "$SMPROGRAMS\NLarn\Changelog.lnk" "$INSTDIR\Changelog.txt"

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NLarn"
  DeleteRegKey HKLM SOFTWARE\NLarn

  ; Remove files and uninstaller
  Delete $INSTDIR\*.*
  Delete $INSTDIR\lib\*.*

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\NLarn\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\NLarn"
  RMDir "$INSTDIR\lib"
  RMDir "$INSTDIR"

SectionEnd
