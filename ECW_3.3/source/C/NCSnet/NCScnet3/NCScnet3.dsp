# Microsoft Developer Studio Project File - Name="NCScnet3" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=NCScnet3 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NCScnet3.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NCScnet3.mak" CFG="NCScnet3 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NCScnet3 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCScnet3 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/NCS/Source/C/NCSnet/NCScnet3", PEKAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NCScnet3 - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NCSCNET3_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NCSCNET3_EXPORTS" /D "NCS_POST_VERSION2" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib ncsutil.lib ws2_32.lib winmm.lib /nologo /dll /map:"..\..\..\..\map\NCScnet3.map" /machine:I386 /def:".\NCScnet.def" /out:"..\..\..\..\bin\NCScnet.dll" /implib:"..\..\..\..\lib\NCScnet.lib" /MAPINFO:EXPORTS /MAPINFO:LINES
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "NCScnet3 - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NCSCNET3_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NCSCNET3_EXPORTS" /D "NCS_POST_VERSION2" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib ncsutild.lib ws2_32.lib winmm.lib /nologo /dll /pdb:"..\..\..\..\bin\NCScnetd.pdb" /map:"..\..\..\..\map\NCScnet3d.map" /debug /machine:I386 /def:".\NCScnet.def" /out:"..\..\..\..\bin\NCScnetd.dll" /implib:"..\..\..\..\lib\NCScnetd.lib" /pdbtype:sept /MAPINFO:EXPORTS /MAPINFO:LINES
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "NCScnet3 - Win32 Release"
# Name "NCScnet3 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\NCScnet.cpp
# End Source File
# Begin Source File

SOURCE=.\NCScnet.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\NCSGetPasswordDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSGetRequest.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSPostRequest.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSRequest.cpp
# End Source File
# Begin Source File

SOURCE=.\NCSSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\PasswordDialog.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cnet.h
# End Source File
# Begin Source File

SOURCE=.\cnetdefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCScnet.h
# End Source File
# Begin Source File

SOURCE=.\NCSException.h
# End Source File
# Begin Source File

SOURCE=.\NCSGetRequest.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSnet.h
# End Source File
# Begin Source File

SOURCE=.\NCSPostRequest.h
# End Source File
# Begin Source File

SOURCE=.\NCSProxy.h
# End Source File
# Begin Source File

SOURCE=.\NCSRequest.h
# End Source File
# Begin Source File

SOURCE=.\NCSSocket.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icon_key.ico
# End Source File
# End Group
# End Target
# End Project
