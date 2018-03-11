# Microsoft Developer Studio Project File - Name="NCSEcw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=NCSEcw - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NCSEcw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NCSEcw.mak" CFG="NCSEcw - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NCSEcw - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSEcw - Win32 Debug64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSEcw - Win32 Release64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSEcw - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/NCS/Source/C/NCSEcw/NCSEcw", GWBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../../include" /I "../lcms/include" /I "P:\Development\ermdev\ERM\Source\INCLUDE" /I "P:\Development\ermdev\ERM\Source\LIB\ERMAPPER\GEOtiff" /D "_DEBUG" /D "NCSECW_EXPORTS" /D "_WINDLL" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 NCScnetd.lib ncsutild.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcwd.pdb" /debug /machine:I386 /def:".\NCSEcw.def" /out:"../../../../bin/NCSEcwd.dll" /implib:"../../../../lib/NCSEcwd.lib" /pdbtype:sept /libpath:"../../../../lib" /EXPORT:DllCanUnloadNow,PRIVATE /EXPORT:DllGetClassObject,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=\bin\NCSEcwd.dll
InputPath=\bin\NCSEcwd.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NCSEcw___Win32_Debug64"
# PROP BASE Intermediate_Dir "NCSEcw___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "NCSECW_EXPORTS" /D "_WINDLL" /D "_AFXDLL" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../../include" /I "../lcms/include" /I "P:\Development\ermdev\ERM\Source\INCLUDE" /I "P:\Development\ermdev\ERM\Source\LIB\ERMAPPER\GEOtiff" /D "_DEBUG" /D "NCSECW_EXPORTS" /D "_WINDLL" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 NCScnetd.lib ncsutild.lib ../lcms112/lib/MS/lcms112d.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcwd.pdb" /debug /machine:I386 /def:".\NCSEcw.def" /out:"../../../../bin/NCSEcwd.dll" /implib:"../../../../lib/NCSEcwd.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 NCScnetd.lib ncsutild.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/Win64\NCSEcwd.pdb" /debug /machine:IX86 /def:".\NCSEcw.def" /out:"../../../../bin/Win64\NCSEcwd.dll" /implib:"../../../../lib/Win64\NCSEcwd.lib" /pdbtype:sept /libpath:"../../../../lib" /machine:AMD64 /EXPORT:DllCanUnloadNow,PRIVATE /EXPORT:DllGetClassObject,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\Debug64
TargetPath=\bin\Win64\NCSEcwd.dll
InputPath=\bin\Win64\NCSEcwd.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NCSEcw___Win32_Release64"
# PROP BASE Intermediate_Dir "NCSEcw___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /Op /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /FR /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Op /I "../../../include" /I "../lcms/include" /I "P:\Development\ermdev\ERM\Source\INCLUDE" /I "P:\Development\ermdev\ERM\Source\LIB\ERMAPPER\GEOtiff" /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCScnet.lib ncsutil.lib ../lcms112/lib/MS/lcms112.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcw.pdb" /map:"../../../../map/NCSEcw.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"../../../../bin/NCSEcw.dll" /implib:"../../../../lib/NCSEcw.lib" /MAPINFO:EXPORTS /MAPINFO:LINES
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCScnet.lib ncsutil.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/Win64\NCSEcw.pdb" /map:"../../../../map/Win64\NCSEcw.map" /debug /machine:IX86 /nodefaultlib:"libc.lib" /out:"../../../../bin/Win64\NCSEcw.dll" /implib:"../../../../lib/Win64\NCSEcw.lib" /libpath:"../../../../lib" /machine:AMD64 /MAPINFO:EXPORTS /MAPINFO:LINES /EXPORT:DllCanUnloadNow,PRIVATE /EXPORT:DllGetClassObject,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\Release64
TargetPath=\bin\Win64\NCSEcw.dll
InputPath=\bin\Win64\NCSEcw.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NCSEcw___Win32_Release"
# PROP BASE Intermediate_Dir "NCSEcw___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /Op /I "M:\Development\ERM\Source\LIB\ERMAPPER\GEOtiff" /I "M:\Development\ERM\Source\INCLUDE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /FR /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Op /I "../../../include" /I "../lcms/include" /I "P:\Development\ermdev\ERM\Source\INCLUDE" /I "P:\Development\ermdev\ERM\Source\LIB\ERMAPPER\GEOtiff" /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCScnet.lib ncsutil.lib ../lcms112/lib/MS/lcms112.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcw.pdb" /map:"../../../../map/NCSEcw.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"../../../../bin/NCSEcw.dll" /implib:"../../../../lib/NCSEcw.lib" /MAPINFO:EXPORTS /MAPINFO:LINES /fixed:no
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCScnet.lib ncsutil.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcw.pdb" /map:"../../../../map/NCSEcw.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"../../../../bin/NCSEcw.dll" /implib:"../../../../lib/NCSEcw.lib" /libpath:"../../../../lib" /MAPINFO:EXPORTS /MAPINFO:LINES /fixed:no /EXPORT:DllCanUnloadNow,PRIVATE /EXPORT:DllGetClassObject,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\Release
TargetPath=\bin\NCSEcw.dll
InputPath=\bin\NCSEcw.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "NCSEcw - Win32 Debug"
# Name "NCSEcw - Win32 Debug64"
# Name "NCSEcw - Win32 Release64"
# Name "NCSEcw - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\shared_src\build_pyr.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmscnvrt.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmserr.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsgamma.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsgmt.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsintrp.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsio1.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmslut.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsmatsh.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsmtrx.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsnamed.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmspack.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmspcs.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmssamp.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsvirt.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmswtpnt.c
# End Source File
# Begin Source File

SOURCE=..\lcms\src\cmsxform.c
# End Source File
# Begin Source File

SOURCE=..\shared_src\collapse_pyr.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O2
# SUBTRACT BASE CPP /YX
# ADD CPP /O2
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ComNCSRenderer.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /YX
# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ecw_jni.c
# End Source File
# Begin Source File

SOURCE=.\ecw_jni_config.c
# End Source File
# Begin Source File

SOURCE=..\shared_src\ecw_open.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\shared_src\ecw_read.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\shared_src\fileio_compress.c
# End Source File
# Begin Source File

SOURCE=..\shared_src\fileio_decompress.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NCSAffineTransform.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSBlockFile.cpp
# End Source File
# Begin Source File

SOURCE=.\ncscbm.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /MDd /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD BASE CPP /MDd /YX
# ADD CPP /MDd /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /MD /YX
# ADD CPP /MD /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /MD
# SUBTRACT BASE CPP /YX
# ADD CPP /MD
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ncscbmidwt.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ncscbmnet.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ncscbmopen.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ncscbmpurge.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NCSEcw.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NCSEcw.def

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NCSEcw.idl
# ADD MTL /I "../../../include" /tlb ".\NCSEcw.tlb" /h "NCSEcwCom.h" /iid "NCSEcw_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\NCSEcw.rc
# End Source File
# Begin Source File

SOURCE=.\NCSFile.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\NCSGDT2\NCSGDTEpsg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSGDT2\NCSGDTEPSGKey.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSGDT2\NCSGDTEpsgPcsKey.cpp
# End Source File
# Begin Source File

SOURCE=..\..\NCSGDT2\NCSGDTLocation.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSHuffmanCoder.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O2
# ADD CPP /O2

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2BitsPerComponentBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2Box.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2CaptureResolutionBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2ChannelDefinitionBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2ColorSpecificationBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2ComponentMappingBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2ContiguousCodestreamBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2DataEntryURLBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2DefaultDisplayResolutionBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2File.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2FileTypeBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2FileView.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2GMLGeoLocationBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2HeaderBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2ImageHeaderBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2IntellectualPropertyBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2PaletteBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2PCSBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2ResolutionBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2SignatureBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2SuperBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2UUIDBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2UUIDInfoBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2UUIDListBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2WorldBox.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJP2XMLBox.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O1
# ADD CPP /O1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O1
# ADD CPP /O1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPC.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCCOCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCCodeBlock.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCCodingStyleParameter.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCCODMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCCOMMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCComponent.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O2
# ADD CPP /O2

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCComponentDepthType.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCCRGMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCDCShiftNode.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCDump.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCEcwpIOStream.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCEOCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCEPHMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCFileIOStream.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCICCNode.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCIOStream.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCMainHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCMCTNode.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCMemoryIOStream.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCMQCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCNode.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCNodeTiler.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPACKET.CPP
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPacketLengthType.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPaletteNode.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPLMMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPLTMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPOCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPPMMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPPTMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCPrecinct.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD BASE CPP /Od
# ADD CPP /Od

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O2
# ADD CPP /O2

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O2
# ADD CPP /O2

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCProgression.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCProgressionOrderType.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCQCCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCQCDMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCQuantizationParameter.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCResample.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCResolution.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /O2
# ADD CPP /O2

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O2
# ADD CPP /O2

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCRGNMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCSEGMENT.CPP
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCSIZMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCSOCMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCSODMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCSOPMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCSOTMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCSubBand.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCT1CODER.CPP

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCTagTree.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCTilePartHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCTLMMarker.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSJPCYCbCrNode.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSOutputFile.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /YX
# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NCSRenderer.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NCSWorldFile.cpp
# End Source File
# Begin Source File

SOURCE=..\shared_src\pack.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O2
# ADD CPP /O2

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\shared_src\qmf_util.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\shared_src\quantize.c
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD BASE CPP /Yc"stdafx.h"
# SUBTRACT BASE CPP /D "_ATL_MIN_CRT"
# ADD CPP /Yc"stdafx.h"
# SUBTRACT CPP /D "_ATL_MIN_CRT"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# SUBTRACT BASE CPP /D "_ATL_MIN_CRT" /YX /Yc
# SUBTRACT CPP /D "_ATL_MIN_CRT" /YX /Yc

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\shared_src\unpack.c

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

# ADD BASE CPP /O2
# SUBTRACT BASE CPP /YX
# ADD CPP /O2
# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\updates.txt

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ComNCSRenderer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\ECW.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\JNCSEcwConfig.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\JNCSFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSAffineTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSBlockFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSEcw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSECWClient.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSECWCompress.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSECWCompressClient.h
# End Source File
# Begin Source File

SOURCE=.\NCSEcwCP.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSFile.h
# End Source File
# Begin Source File

SOURCE=..\..\NCSGDT2\NCSGDTEpsg.h
# End Source File
# Begin Source File

SOURCE=..\..\NCSGDT2\NCSGDTEPSGKey.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSGDTLocation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSGeoTIFFBoxUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSHuffmanCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJP2Box.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJP2File.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJP2FileView.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJP2SuperBox.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCCOCMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCCodeBlock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCCodingStyleParameter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCCODMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCCOMMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCComponentDepthType.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCCRGMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCDCShiftNode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCDefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCDump.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCEcwpIOStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCEOCMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCEPHMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCFileIOStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCICCNode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCIOStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCMainHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCMCTNode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCMemoryIOStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCMQCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCNode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCNodeTiler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCPACKET.H
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCPacketLengthType.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCPaletteNode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCPLMMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCPLTMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCPOCMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCPPMMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCPPTMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCPrecinct.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCProgression.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCProgressionOrderType.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCQCCMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCQCDMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCQuantizationParameter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCRect.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCResample.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCResolution.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCRGNMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCSEGMENT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCSIZMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCSOCMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCSODMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCSOPMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCSOTMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCSubBand.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\NCSJPCT1Coder.H
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCTagTree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCTilePartHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCTLMMarker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSJPCYCbCr2RGBNode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSRenderer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSWorldFile.h
# End Source File
# Begin Source File

SOURCE=..\shared_src\qsmodel.h
# End Source File
# Begin Source File

SOURCE=..\shared_src\rangecod.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ComNCSRenderer.rgs
# End Source File
# Begin Source File

SOURCE=".\NCSRenderer(32).bmp"
# End Source File
# Begin Source File

SOURCE=.\NCSRenderer.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=..\NCSJP2\Doxyfile
# End Source File
# Begin Source File

SOURCE=..\NCSJP2\NCSMakeDoc.bat

!IF  "$(CFG)" == "NCSEcw - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release64"

!ELSEIF  "$(CFG)" == "NCSEcw - Win32 Release"

!ENDIF 

# End Source File
# End Target
# End Project
