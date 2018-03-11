# Microsoft Developer Studio Project File - Name="NCSUtil_Static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=NCSUtil_Static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NCSUtil_Static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NCSUtil_Static.mak" CFG="NCSUtil_Static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NCSUtil_Static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "NCSUtil_Static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/NCS/Source/C/NCSUtil/NCSUtil_Static", WWHBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NCSUtil_Static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NCSUtil_Static___Win32_Release"
# PROP BASE Intermediate_Dir "NCSUtil_Static___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "../../../include" /D "NDEBUG" /D "WIN32" /D "_LIB" /D "NCSECW_STATIC_LIBS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../../../../lib/NCSUtilS.lib"

!ELSEIF  "$(CFG)" == "NCSUtil_Static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NCSUtil_Static___Win32_Debug"
# PROP BASE Intermediate_Dir "NCSUtil_Static___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../include" /D "_DEBUG" /D "WIN32" /D "_LIB" /D "NCSECW_STATIC_LIBS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../../../../lib/NCSUtilSd.lib"

!ENDIF 

# Begin Target

# Name "NCSUtil_Static - Win32 Release"
# Name "NCSUtil_Static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\CNCSBase64Coder.cpp
# End Source File
# Begin Source File

SOURCE=..\CNCSMetabaseEdit.cpp
# End Source File
# Begin Source File

SOURCE=..\CNCSMultiSZ.cpp
# End Source File
# Begin Source File

SOURCE=..\dynamiclib.c
# End Source File
# Begin Source File

SOURCE=..\error.c
# End Source File
# Begin Source File

SOURCE=..\exception_catch.c
# End Source File
# Begin Source File

SOURCE=..\file.c
# End Source File
# Begin Source File

SOURCE=..\log.cpp
# End Source File
# Begin Source File

SOURCE=..\main.c
# End Source File
# Begin Source File

SOURCE=..\malloc.c
# End Source File
# Begin Source File

SOURCE=..\mutex.c
# End Source File
# Begin Source File

SOURCE=..\NCSBase64.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSCoordinateConverter.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSCoordinateSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSCoordinateTransform.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSCrypto.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSError.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSEvent.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSExtent.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSExtents.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSLog.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSMutex.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSObject.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSObjectList.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSPoint.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSPrefs.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSPrefsWin.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSRasterCoordinateConverter.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSScreenPoint.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSServerState.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSString.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSThread.cpp
# End Source File
# Begin Source File

SOURCE=..\NCSUtil.def
# End Source File
# Begin Source File

SOURCE=..\ncsutil.rc
# End Source File
# Begin Source File

SOURCE=..\NCSWorldPoint.cpp
# End Source File
# Begin Source File

SOURCE=..\pool.c
# End Source File
# Begin Source File

SOURCE=..\quadtree.cpp
# End Source File
# Begin Source File

SOURCE=..\queue.c
# End Source File
# Begin Source File

SOURCE=..\thread.c
# End Source File
# Begin Source File

SOURCE=..\timer.c
# End Source File
# Begin Source File

SOURCE=..\util.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\CNCSBase64Coder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\CNCSMetabaseEdit.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\CNCSMultiSZ.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSArray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSBase64.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSCoordinateConverter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSCoordinateSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSCoordinateTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSCrypto.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSDefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSDynamicLib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSError.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSErrors.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSExtent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSExtents.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSFileIO.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSLog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSMalloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSMemPool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSMisc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSMutex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSObject.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSObjectList.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSPackets.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSPrefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSQuadTree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSQueue.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSScreenPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSServerState.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSThread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSTimeStamp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\NCSWorldPoint.h
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# End Group
# End Target
# End Project
