;
; nlarn.nsi
;
; Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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

; Pages

Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

; The license

LicenseData LICENSE.txt
LicenseForceSelection radiobuttons "I accept" "I decline"

;--------------------------------

; The stuff to install
Section "NLarn (required)"

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put file there
  File "nlarn.exe"
  File "pdcurses.dll"
  File "libglib-2.0-0.dll"
  File "libz-1.dll"
  File "README.txt"
  File "LICENSE"
  File "nlarn.ini-sample"

  SetOutPath "$INSTDIR\lib"
  File "lib\fortune"
  File "lib\maze"
  File "lib\nlarn.hlp"
  File "lib\nlarn.msg"

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
