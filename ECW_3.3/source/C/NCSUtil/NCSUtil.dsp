# Microsoft Developer Studio Project File - Name="NCSUtil" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=NCSUtil - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NCSUtil.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NCSUtil.mak" CFG="NCSUtil - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NCSUtil - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSUtil - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSUtil - Win32 Debug64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSUtil - Win32 Release64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/NCS/Source/C/NCSUtil", LHAAAAAA"
# PROP Scc_LocalPath ".\"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NCSUtil - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "NCSUTIL_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "NCSUTIL_EXPORTS" /D "NCSMALLOCLOG" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib imagehlp.lib shlwapi.lib /nologo /dll /pdb:"..\..\..\bin\NCSUtil.pdb" /map:"../../../map/NCSUtil.map" /debug /machine:I386 /def:".\NCSUtil.def" /out:"..\..\..\bin\NCSUtil.dll" /implib:"../../../lib/NCSUtil.lib" /libpath:"../../../lib" /fixed:no /MAPINFO:EXPORTS /MAPINFO:LINES
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "NCSUTIL_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "NCSUTIL_EXPORTS" /D "NCSMALLOCLOG" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib imagehlp.lib shlwapi.lib /nologo /dll /pdb:"../../../bin/NCSUtild.pdb" /debug /machine:I386 /def:"NCSUtil.def" /out:"..\..\..\bin\NCSUtild.dll" /implib:"../../../lib/NCSUtild.lib" /pdbtype:sept /libpath:"../../../lib"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=rem copy Debug\NCSUtild.dll ..\..\Bin	rem copy Debug\NCSUtild.pdb ..\..\Bin	rem copy Debug\NCSUtild.dll p:\Inetpub\wwwroot\ecwp	rem copy Debug\NCSUtild.pdb p:\Inetpub\wwwroot\ecwp
# End Special Build Tool

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Debug64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NCSUtil___Win32_Debug640"
# PROP BASE Intermediate_Dir "NCSUtil___Win32_Debug640"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "NCSUTIL_EXPORTS" /D "NCSMALLOCLOG" /YX"NCSUtil.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "NCSUTIL_EXPORTS" /D "NCSMALLOCLOG" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib imagehlp.lib /nologo /dll /pdb:"../../../bin/NCSUtild.pdb" /debug /machine:I386 /def:"NCSUtil.def" /out:"..\..\..\bin\NCSUtild.dll" /implib:"../../../lib/NCSUtild.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib imagehlp.lib shlwapi.lib /nologo /dll /pdb:"../../../bin/Win64\NCSUtild.pdb" /debug /machine:IX86 /def:"NCSUtil.def" /out:"..\..\..\bin\Win64\NCSUtild.dll" /implib:"../../../lib/Win64\NCSUtild.lib" /libpath:"../../../lib" /machine:AMD64 /machine:AMD64 /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=rem copy Debug\NCSUtild.dll ..\..\Bin	rem copy Debug\NCSUtild.pdb ..\..\Bin	rem copy Debug\NCSUtild.dll p:\Inetpub\wwwroot\ecwp	rem copy Debug\NCSUtild.pdb p:\Inetpub\wwwroot\ecwp
# End Special Build Tool

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Release64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NCSUtil___Win32_Release640"
# PROP BASE Intermediate_Dir "NCSUtil___Win32_Release640"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /I "../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "NCSUTIL_EXPORTS" /D "NCSMALLOCLOG" /FR /YX"NCSUtil.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "NCSUTIL_EXPORTS" /D "NCSMALLOCLOG" /FR /YX"NCSUtil.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib imagehlp.lib /nologo /dll /pdb:"..\..\..\bin\NCSUtil.pdb" /map:"../../../map/NCSUtil.map" /debug /machine:I386 /def:".\NCSUtil.def" /out:"..\..\..\bin\NCSUtil.dll" /implib:"../../../lib/NCSUtil.lib" /fixed:no /MAPINFO:EXPORTS /MAPINFO:LINES
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib imagehlp.lib shlwapi.lib /nologo /dll /pdb:"..\..\..\bin\Win64\NCSUtil.pdb" /map:"../../../map/NCSUtil.map" /debug /machine:IX86 /def:".\NCSUtil.def" /out:"..\..\..\bin\Win64\NCSUtil.dll" /implib:"../../../lib/Win64\NCSUtil.lib" /libpath:"../../../lib" /machine:AMD64 /fixed:no /MAPINFO:EXPORTS /MAPINFO:LINES
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "NCSUtil - Win32 Release"
# Name "NCSUtil - Win32 Debug"
# Name "NCSUtil - Win32 Debug64"
# Name "NCSUtil - Win32 Release64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CNCSBase64Coder.cpp
# End Source File
# Begin Source File

SOURCE=.\CNCSMetabaseEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\CNCSMultiSZ.cpp
# End Source File
# Begin Source File

SOURCE=.\dynamiclib.c
# End Source File
# Begin Source File

SOURCE=.\error.c
# End Source File
# Begin Source File

SOURCE=.\exception_catch.c
# End Source File
# Begin Source File

SOURCE=.\file.c
# End Source File
# Begin Source File

SOURCE=.\log.cpp
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\malloc.c

!IF  "$(CFG)" == "NCSUtil - Win32 Release"

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Debug"

# ADD CPP /YX"NCSUtil.h"

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Debug64"

# ADD BASE CPP /YX"NCSUtil.h"
# ADD CPP /YX"NCSUtil.h"

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Release64"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mutex.c
# End Source File
# Begin Source File

SOURCE=.\NCSBase64.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSCoordinateConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSCoordinateSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSCoordinateTransform.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSCrypto.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSError.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSExtent.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSExtents.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSLog.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSMutex.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSObject.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSObjectList.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSPoint.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSPrefs.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSPrefsWin.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSPrefsXML.cpp

!IF  "$(CFG)" == "NCSUtil - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Release64"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NCSRasterCoordinateConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSScreenPoint.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSServerState.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSString.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSThread.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSUtil.def

!IF  "$(CFG)" == "NCSUtil - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Debug"

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Debug64"

!ELSEIF  "$(CFG)" == "NCSUtil - Win32 Release64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ncsutil.rc
# End Source File
# Begin Source File

SOURCE=.\NCSWorldPoint.cpp
# End Source File
# Begin Source File

SOURCE=.\pool.c
# End Source File
# Begin Source File

SOURCE=.\quadtree.cpp
# End Source File
# Begin Source File

SOURCE=.\queue.c
# End Source File
# Begin Source File

SOURCE=.\thread.c
# End Source File
# Begin Source File

SOURCE=.\timer.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CNCSBase64Coder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\CNCSMetabaseEdit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\CNCSMultiSZ.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSArray.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSBase64.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSCoordinateConverter.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSCoordinateSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSCoordinateTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSCrypto.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSDefs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSDynamicLib.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSError.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSErrors.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSExtent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSExtents.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSFile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSFileIO.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSLog.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSMalloc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSMemPool.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSMisc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSMutex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSObject.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSObjectList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSPackets.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSPrefs.h
# End Source File
# Begin Source File

SOURCE=.\NCSPrefsWin.h
# End Source File
# Begin Source File

SOURCE=.\NCSPrefsXML.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSQuadTree.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSQueue.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSScreenPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSServerState.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSString.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSThread.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSTimeStamp.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\include\NCSWorldPoint.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\MacPortREADME.txt
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
