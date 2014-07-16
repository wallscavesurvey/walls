; Script for TSS_Data.
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "sections.nsh"

;This nsi file normally located in \Work12\WallsMap\doc\TSS, with binaries in \Work12\bin\TSS
!define BIN_DIR "\Work12\bin"
!ifndef VERSION
  !define VERSION '2014-07-15'
!endif

!define WNDCLASS "WallsMapClass"
!define WNDTITLE ""

Function .onInit
FindWindow $0 "${WNDCLASS}" "${WNDTITLE}"
StrCmp $0 0 continueInstall
MessageBox MB_ICONSTOP|MB_OK "Setup can't proceed if WallsMap is running. Please close all instances of WallsMap and try again."
Abort
continueInstall:
StrCmp "$INSTDIR" "" +2
Return
/*
ReadEnvStr $R0 PUBLIC
Strcmp $R0 "" xplabel
StrCpy $INSTDIR "$R0\WallsMap Projects\TSS"
; MessageBox MB_OK "Final default INSTDIR: $INSTDIR"
Return
xplabel:
*/
StrCpy $R0 $LOCALAPPDATA 1
StrCpy $INSTDIR "$R0:\TSS"
FunctionEnd

; !define PUBLIC "$R0"

;--------------------------------
;General

  ; use after testing --
  SetCompressor /SOLID lzma

  ;Name and file
  Name "TSS_Data"
  OutFile "${BIN_DIR}\TSS_Data_setup.exe"

  ;Get default installation folder from registry if available
  InstallDirRegKey HKCU "Software\TSS_Data" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING
; !define MUI_HEADERIMAGE
; !define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\nsis.bmp"

!define MUI_COMPONENTSPAGE_SMALLDESC

;--------------------------------
;Pages
	!define MUI_WELCOMEPAGE_TITLE "Welcome to TSS_Data Setup$\r$\nRelease ${VERSION}"
	!define MUI_WELCOMEPAGE_TEXT "This setup will guide you through the installation of the TSS_Data WallsMap project, \
	the GIS version of the Texas Speleological Survey karst database.$\r$\n$\r$\n\
	It's recommended that you have the TSS edition of WallsMap already installed and that you have read the \
	section of the help file describing the TSS database.$\r$\n$\r$\n$_CLICK"

  !insertmacro MUI_PAGE_WELCOME
  
	!define MUI_PAGE_HEADER_TEXT "Choose Which Image Folders to Install" 
	!define MUI_PAGE_HEADER_SUBTEXT "Each selected folder requires that the corresponding \
	compressed file$\r$\n(e.g., TSS_Maps.101 be present at the location of this setup program."

	!define MUI_COMPONENTSPAGE_TEXT_TOP "NOTE: If these folders already exist at the destination you choose (next step), \
	you'll be asked during the installation whether or not their entire contents should first be deleted."
	
/*	
	!define MUI_COMPONENTSPAGE_TEXT_TOP "CAUTION:  If an image folder already exists at the destination you choose (next step), \
	it will be completely replaced, in effect deleting obsolete files that are no longer linked to the project. \
	To prevent this, cancel setup and move or rename the existing folder."
	
	!define MUI_COMPONENTSPAGE_TEXT_TOP "NOTE:  If an image folder already exists at the destination \
	you choose (next step), files in this installation will simply be copied there while overwriting any \
	files with the same name. To avoid wasting space by keeping obsolete (possibly renamed) image files, you \
	might want to Cancel this run of setup and delete the existing folders."
*/	

  !define MUI_PAGE_CUSTOMFUNCTION_LEAVE ComponentsLeave
  !insertmacro MUI_PAGE_COMPONENTS
  
	!define MUI_PAGE_HEADER_SUBTEXT "Choose the folder in which to install the TSS_Data (${VERSION}) project."
	!define MUI_DIRECTORYPAGE_TEXT_TOP "The folder you select, which must NOT be under C:\Program Files, will be the parent of TSS_Data and the two image folders. A new project script named TSS_Data_${VERSION}.ntl will be stored in TSS_Data and launched when setup finishes.$\r$\n$\r$\n\
	CAUTION: If you've made edits to any of the standard feature shapefiles (TSS Caves, TSS Sinks, TSS Shelters, \
	TSS Springs, and TSS Other), be sure to move them to another location before proceding. Otherwise they \
	will be overwritten without warning." 

  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
    ; !define MUI_FINISHPAGE_NOAUTOCLOSE
	!define MUI_FINISHPAGE_TEXT "The TSS_Data project has been successfully installed."
    !define MUI_FINISHPAGE_RUN
    ; !define MUI_FINISHPAGE_RUN_NOTCHECKED
    !define MUI_FINISHPAGE_RUN_TEXT "Launch project TSS_Data_${VERSION}.ntl"
    !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchNTL"

  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------

;Installer Sections

Section
  StrCpy $R0  0
  StrCpy $R1  0
  ${If} $R5 != 0
  IfFileExists "$INSTDIR\TSS Maps" 0 +2
  StrCpy $R0  1
  ${EndIf}
  ${If} $R6 != 0
  IfFileExists "$INSTDIR\TSS Photos" 0 +2
  StrCpy $R1  1
  ${EndIf}
  IntOp $0 $R0 + $R1
  ${If} $0 != 0
  MessageBox MB_YESNOCANCEL|MB_ICONQUESTION "Since folders TSS Maps and/or TSS Photos already exist, they could contain \
  obsolete (possibly renamed) files that are no longer linked to the project. Do you want to save space by \
  replacing the existing folders as opposed to simply copying the linked files?$\r$\n$\r$\n\
  CAUTION: Don't select YES if the folders contain newly-added files that haven't yet been submitted as a project update." IDYES cont1 IDNO cont2
  DetailPrint "Installation cancelled by user."
  Abort
  GoTo finish
  cont1:
  ${If} $R5 != 0
  RMDir /r "$INSTDIR\TSS Maps"
  ${EndIf}
  ${If} $R6 != 0
  RMDir /r "$INSTDIR\TSS Photos"
  ${EndIf}
  ${EndIf}
  
 cont2:
  SetOutPath $EXEDIR
  SetOverwrite on
  File "${BIN_DIR}\TSS\unRAR32\unRAR32.exe"
  SetOutPath $INSTDIR
  DetailPrint "Installing TSS_Data shapefiles..."
  File /r "${BIN_DIR}\TSS\TSS_Data"
 finish:
SectionEnd

Section "TSS Maps" tss_maps
  AddSize 204800
  ${If} $R5 != 0
  ; MessageBox MB_OK "Installing Maps..."
  ExecWait '"$EXEDIR\UnRAR32.exe" x -o+ "$EXEDIR\TSS_Maps.101" "$INSTDIR\"'
  IfErrors +3
  DetailPrint "TSS Maps was successfully installed."
  goto endMaps
  MessageBox MB_OK "NOTE: TSS Maps was not completely installed. Disk may be full."
endMaps:
  ${EndIf}
SectionEnd

Section "TSS Photos" tss_photos
  AddSize 102400
  ${If} $R6 != 0 
  ; MessageBox MB_OK "Installing Photos."
  ExecWait '"$EXEDIR\UnRAR32.exe" x -o+ "$EXEDIR\TSS_Photos.101" "$INSTDIR\"'
  IfErrors +3
  DetailPrint "TSS Photos was successfully installed."
  goto endPhotos
  MessageBox MB_OK "NOTE: TSS Photos was not completely installed. Disk may be full."
  endPhotos:
  ${EndIf}
SectionEnd

Section
  Delete "$EXEDIR\unRAR32.exe" 
  ; MessageBox MB_OK "Deleted $EXEDIR\7zr.exe"
  WriteRegStr HKCU "Software\TSS_Data" "" $INSTDIR
  CreateShortCut "$DESKTOP\TSS_Data ${VERSION}.lnk" "$INSTDIR\TSS_Data\TSS_Data_${VERSION}.ntl"
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TSS_Data" \
                 "DisplayName" "TSS_Data WallsMap Project"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TSS_Data" \
                 "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
SectionEnd

Function LaunchNTL
  ; MessageBox MB_OK "Reached LaunchNTL $\r$\nInstallDirectory: $INSTDIR "
  ExecShell "" "$INSTDIR\TSS_Data\TSS_Data_${VERSION}.ntl"
FunctionEnd

Function ComponentsLeave
  ; Section flags tests (requires sections.nsh be included)
  StrCpy $R5 0
  StrCpy $R6 0
  ${If} ${SectionIsSelected} ${tss_maps}
    StrCpy $R5 1
  ${EndIf}
  
  ${If} ${SectionIsSelected} ${tss_photos}
    StrCpy $R6 1
  ${EndIf}
  
  ${If} $R5 == 1
    IfFileExists "$EXEDIR\TSS_Maps.101" +3 0
    MessageBox MB_OK "The TSS Maps folder can't be installed since $EXEDIR\TSS_Maps.101 isn't present. \
	To proceed you'll need to uncheck that box."
    Abort
  ${EndIf}
  ${If} $R6 == 1
    IfFileExists "$EXEDIR\TSS_Photos.101" +3 0
    MessageBox MB_OK "The TSS Photos folder can't be installed since $EXEDIR\TSS_Maps.101 isn't present. \
 	To proceed you'll need to uncheck that box."
    Abort
  ${EndIf}
  
   IntOp $0 $R5 + $R6
   ${Unless} $0 == 2
    MessageBox MB_YESNO "Choosing not to install either TSS Maps or TSS Photos can result in broken links in memo fields.  \
	Do you want to proceed anyway?" IDYES allowSkip IDNO 0
    Abort
allowSkip:
  ${EndIf}
 FunctionEnd
;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_tss_maps ${LANG_ENGLISH} "Select to install the linked map images."
  LangString DESC_tss_photos ${LANG_ENGLISH} "Select to install the linked photos."
  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${tss_maps} $(DESC_tss_maps)
  !insertmacro MUI_DESCRIPTION_TEXT ${tss_photos} $(DESC_tss_photos)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section
Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "The TSS_Data project was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove the TSS_Data, TSS Maps, and TSS Photos folders?" IDYES +2
  Abort
  FindWindow $0 "${WNDCLASS}" "${WNDTITLE}"
  StrCmp $0 0 continueUnInstall
  MessageBox MB_ICONSTOP|MB_OK "Uninstallation can't proceed if WallsMap is running. Please close all instances of WallsMap and try again."
  Abort
  continueUnInstall:
FunctionEnd

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR\TSS Maps"
  RMDir /r "$INSTDIR\TSS Photos"
  RMDir /r "$INSTDIR\TSS_Data"
  RMDir "$INSTDIR"
  Delete "$DESKTOP\TSS_Data ${VERSION}.lnk"
  DeleteRegKey /ifempty HKCU "Software\TSS_Data"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TSS_Data"
  SetAutoClose true
SectionEnd
