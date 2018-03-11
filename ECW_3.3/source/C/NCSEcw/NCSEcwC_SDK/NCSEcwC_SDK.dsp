# Microsoft Developer Studio Project File - Name="NCSEcwC_SDK" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=NCSEcwC_SDK - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NCSEcwC_SDK.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NCSEcwC_SDK.mak" CFG="NCSEcwC_SDK - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NCSEcwC_SDK - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSEcwC_SDK - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSEcwC_SDK - Win32 Debug64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCSEcwC_SDK - Win32 Release64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/NCS/Source/C/NCSEcw/NCSEcwC_SDK", NGBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NCSEcwC_SDK - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "ECW_COMPRESS_SDK_VERSION" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCSEcw.lib NCSUtil.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcwC.pdb" /debug /machine:I386 /out:"../../../../bin/NCSEcwC.dll" /implib:"../../../../lib/NCSEcwC.lib" /libpath:"../../../../lib" /fixed:no
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "NCSEcwC_SDK - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "NCSECWC_EXPORTS" /D "ECW_COMPRESS_SDK_VERSION" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCSEcwd.lib NCSUtild.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcwCd.pdb" /debug /machine:I386 /def:"..\NCSEcwC\ncsecwc.def" /out:"../../../../bin/NCSEcwCd.dll" /implib:"../../../../lib/NCSEcwCd.lib" /pdbtype:sept /libpath:"../../../../lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "NCSEcwC_SDK - Win32 Debug64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NCSEcwC_SDK___Win32_Debug64"
# PROP BASE Intermediate_Dir "NCSEcwC_SDK___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NCSECWC_EXPORTS" /D "ECW_COMPRESS" /D "ECW_COMPRESS_SDK_VERSION" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "NCSECWC_EXPORTS" /D "ECW_COMPRESS_SDK_VERSION" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCSEcwd.lib NCSUtild.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcwCd.pdb" /debug /machine:I386 /def:"..\NCSEcwC\ncsecwc.def" /out:"../../../../bin/NCSEcwCd.dll" /implib:"../../../../lib/NCSEcwCd.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCSEcwd.lib NCSUtild.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/Win64\NCSEcwCd.pdb" /debug /machine:IX86 /def:"..\NCSEcwC\ncsecwc.def" /out:"../../../../bin/Win64\NCSEcwCd.dll" /implib:"../../../../lib/Win64\NCSEcwCd.lib" /pdbtype:sept /libpath:"../../../../lib" /machine:AMD64
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "NCSEcwC_SDK - Win32 Release64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NCSEcwC_SDK___Win32_Release64"
# PROP BASE Intermediate_Dir "NCSEcwC_SDK___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ECW_COMPRESS_SDK_VERSION" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ECW_COMPRESS_SDK_VERSION" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCSEcw.lib NCSUtil.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/NCSEcwC.pdb" /debug /machine:I386 /out:"../../../../bin/NCSEcwC.dll" /implib:"../../../../lib/NCSEcwC.lib" /fixed:no
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib NCSEcw.lib NCSUtil.lib /nologo /subsystem:windows /dll /pdb:"../../../../bin/Win64\NCSEcwC.pdb" /debug /machine:IX86 /out:"../../../../bin/Win64\NCSEcwC.dll" /implib:"../../../../lib/Win64\NCSEcwC.lib" /libpath:"../../../../lib" /machine:AMD64 /fixed:no
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "NCSEcwC_SDK - Win32 Release"
# Name "NCSEcwC_SDK - Win32 Debug"
# Name "NCSEcwC_SDK - Win32 Debug64"
# Name "NCSEcwC_SDK - Win32 Release64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\shared_src\compress.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSEcwC\ncsecwc.def

!IF  "$(CFG)" == "NCSEcwC_SDK - Win32 Release"

!ELSEIF  "$(CFG)" == "NCSEcwC_SDK - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSEcwC_SDK - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "NCSEcwC_SDK - Win32 Release64"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NCSEcwC\ncsecwc.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\include\ECW.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSECWCompress.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSECWCompressClient.h
# End Source File
# Begin Source File

SOURCE=..\ncsecwc\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
