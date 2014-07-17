
!define PRODUCT_NAME "WallsMap"
;This nsi file normally located in \Work12\WallsMap\doc\TSS, with binaries in \Work12\bin
!define BIN_DIR "\Work12\bin"
!define PRODUCT_VERSION "0.3"
!define PRODUCT_PUBLISHER "David McKenzie"
!define PRODUCT_WEB_SITE "http://davidmck.fmnetdesign.com/WallsMap-TSS.htm"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\WallsMap.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
; !define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

!include "MUI2.nsh"


!ifndef BUILD_DATE
  !define BUILD_DATE '2014-07-13'
!endif

!define WNDCLASS "WallsMapClass"
!define WNDTITLE ""

Function .onInit
ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "ProductName"
; ReadRegStr $1 HKLM "SYSTEM\CurrentControlSet\Control\Windows" "CSDVersion"
; MessageBox MB_ICONSTOP|MB_OK "$0 SP Level $1"
StrCmp $0 "" noInstall 0
StrCmp $0 "Microsoft Windows 2000" noInstall 0
StrCmp $0 "Microsoft Windows XP" noInstall continueTest
noInstall:
MessageBox MB_ICONSTOP|MB_OK "Sorry, WallsMap is not compatible with your version of Windows. At least Windows Vista is required."
Abort
continueTest:
FindWindow $0 "${WNDCLASS}" "${WNDTITLE}"
StrCmp $0 0 continueInstall
MessageBox MB_ICONSTOP|MB_OK "This setup can't proceed if WallsMap is running. Please close all instances of WallsMap and try again."
Abort
continueInstall:
FunctionEnd

;--------------------------------
;General
  ;Name and file
  Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
 
  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING
; !define MUI_HEADERIMAGE
; !define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\nsis.bmp"

!define MUI_COMPONENTSPAGE_SMALLDESC

;--------------------------------
; Pages

	!define MUI_WELCOMEPAGE_TITLE "WallsMap v0.3 Setup (TSS Edition)$\r$\n\
	Build ${BUILD_DATE}"
	!define MUI_WELCOMEPAGE_TEXT "This setup will guide you through the installation of the TSS edition of WallsMap v0.3.$\r$\n$\r$\n\
	NOTE: The TSS edition is the same as the public version, but no sample project is included and program's About box contains a different link.$\r$\n$\r$\n$_CLICK"
!insertmacro MUI_PAGE_WELCOME

/*
	!define MUI_PAGE_HEADER_SUBTEXT "Choose the folder in which to install the TSS_Data (${BUILD_DATE}) project."
	!define MUI_DIRECTORYPAGE_TEXT_TOP "The folder you select will be the immediate parent of folders TSS_Data, \
	TSS Maps, and TSS Photos. Although TSS Maps and TSS Photos (if selected) are completely replaced if \
	they exist, only the distributed shapefiles are replaced in an existing TSS_Data folder.$\r$\n$\r$\n\
	IMPORTANT: The default initially displayed below is the location you chose the last \
	time you ran this setup (if ever). Whatever the case, be sure to choose a location other \
	than $PROGRAMFILES. Unlike XP, Vista and Win7 restrict user access to that folder."
*/
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\WallsMap.exe"
; !define MUI_FINISHPAGE_RUN_PARAMETERS "$\"${PUBLIC}\texas_towns.ntl$\""
; !define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\WallsMap.chm"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\WallsMap_ReadMe.txt"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

OutFile "${BIN_DIR}\WallsMap-TSS_setup.exe"
InstallDir "$PROGRAMFILES\WallsMap"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUninstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite on
  File "${BIN_DIR}\WallsMap.exe"
  File "${BIN_DIR}\WallsMap_sid.dll"
  File "${BIN_DIR}\WallsMap.chm"
  File "WallsMap_ReadMe.txt"

; Shortcuts
  CreateShortCut "$DESKTOP\WallsMap.lnk" "$INSTDIR\WallsMap.exe"
  WriteINIStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}" 

SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
 
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\WallsMap.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\WallsMap.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"

  DeleteRegKey HKCR "Applications\WallsMap.exe"
  WriteRegStr HKCR "Applications\WallsMap.exe\shell" "" "open"
  WriteRegStr HKCR "Applications\WallsMap.exe\shell\open\command" "" '$INSTDIR\WallsMap.exe "%1"'
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" "" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".bmp" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".ecw" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".gif" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".j2k" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".jp2" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".jpc" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".jpeg" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".jpg" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".pcx" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".pgm" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".png" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".ppm" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".shp" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".sid" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".tif" ""
  WriteRegStr HKCR "Applications\WallsMap.exe\SupportedTypes" ".tiff" ""
  WriteRegStr HKCR ".shp\OpenWithList\WallsMap.exe" "" ""
  WriteRegStr HKCR ".jp2\OpenWithList\WallsMap.exe" "" ""
  WriteRegStr HKCR ".ecw\OpenWithList\WallsMap.exe" "" ""
  WriteRegStr HKCR ".sid\OpenWithList\WallsMap.exe" "" ""
  DeleteRegKey HKCR ".ntl"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ntl"
  WriteRegStr HKCR ".ntl" "" "NtlFile"
  DeleteRegKey HKCR "NtlFile"
  WriteRegStr HKCR "NtlFile" "" "WallsMap Layer File"
  WriteRegStr HKCR "NtlFile\DefaultIcon" "" "$INSTDIR\WallsMap.exe,0"
  WriteRegStr HKCR "NtlFile\shell" "" "open"
  WriteRegStr HKCR "NtlFile\shell\open\command" "" '$INSTDIR\WallsMap.exe "%1"'
  
  DeleteRegKey HKCR ".nti"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nti"
  WriteRegStr HKCR ".nti" "" "NtiFile"
  DeleteRegKey HKCR "NtiFile"
  WriteRegStr HKCR "NtiFile" "" "WallsMap Image File"
  WriteRegStr HKCR "NtiFile\DefaultIcon" "" "$INSTDIR\WallsMap.exe,0"
  WriteRegStr HKCR "NtiFile\shell" "" "open"
  WriteRegStr HKCR "NtiFile\shell\open\command" "" '$INSTDIR\WallsMap.exe "%1"'
  System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
  FindWindow $0 "${WNDCLASS}" "${WNDTITLE}"
  StrCmp $0 0 continueUnInstall
  MessageBox MB_ICONSTOP|MB_OK "Uninstallation can't proceed if WallsMap is running. Please close all instances of WallsMap and try again."
  Abort
  continueUnInstall:
FunctionEnd

Section Uninstall
  ; !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
  ; Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\WallsMap_ReadMe.txt"
  Delete "$INSTDIR\WallsMap.chm"
  Delete "$INSTDIR\WallsMap.exe"
  Delete "$INSTDIR\WallsMap_sid.dll"
  Delete "$INSTDIR\WallsMap.url"
  Delete "$DESKTOP\WallsMap.lnk"
  RMDir "$INSTDIR"
  DeleteRegKey HKCR "Applications\WallsMap.exe"
  DeleteRegKey HKCR ".jp2\OpenWithList\WallsMap.exe"
  DeleteRegKey HKCR ".ecw\OpenWithList\WallsMap.exe"
  DeleteRegKey HKCR ".sid\OpenWithList\WallsMap.exe"
  DeleteRegKey HKCR ".shp\OpenWithList\WallsMap.exe"
  DeleteRegKey HKCR ".nti"
  DeleteRegKey HKCU "Software\NZ Applications\WallsMap"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nti"
  DeleteRegKey HKCR "NtiFile"
  DeleteRegKey HKCR ".ntl"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ntl"
  DeleteRegKey HKCR "NtlFile"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

  SetAutoClose true
SectionEnd