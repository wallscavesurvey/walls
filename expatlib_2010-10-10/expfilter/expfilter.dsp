# Microsoft Developer Studio Project File - Name="expfilter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=expfilter - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "expfilter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "expfilter.mak" CFG="expfilter - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "expfilter - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "expfilter - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "expfilter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="expat_1.95.2" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "expfilter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="expat_1.95.2" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "expfilter - Win32 Release"
# Name "expfilter - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\expfilter.c
# End Source File
# Begin Source File

SOURCE=..\xmlparse.c

!IF  "$(CFG)" == "expfilter - Win32 Release"

# ADD CPP /D "_DEBUG" /D "COMPILED_FROM_DSP" /D "_WINDOWS" /D "_CONSOLE_" /D "EXPAT_EXPORTS" /D VERSION=\"expat_1.95.2\"
# SUBTRACT CPP /D "NDEBUG" /D "_CONSOLE" /D VERSION="expat_1.95.2"

!ELSEIF  "$(CFG)" == "expfilter - Win32 Debug"

# ADD CPP /D "_CONSOLE_" /D "COMPILED_FROM_DSP" /D "_WINDOWS" /D "EXPAT_EXPORTS" /D VERSION=\"expat_1.95.2\"
# SUBTRACT CPP /D "_CONSOLE" /D VERSION="expat_1.95.2"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\xmlrole.c

!IF  "$(CFG)" == "expfilter - Win32 Release"

# ADD CPP /D "_DEBUG" /D "COMPILED_FROM_DSP" /D "_WINDOWS" /D "_CONSOLE_" /D "EXPAT_EXPORTS" /D VERSION=\"expat_1.95.2\"
# SUBTRACT CPP /D "NDEBUG" /D "_CONSOLE" /D VERSION="expat_1.95.2"

!ELSEIF  "$(CFG)" == "expfilter - Win32 Debug"

# ADD CPP /D "_CONSOLE_" /D "COMPILED_FROM_DSP" /D "_WINDOWS" /D "EXPAT_EXPORTS" /D VERSION=\"expat_1.95.2\"
# SUBTRACT CPP /D "_CONSOLE" /D VERSION="expat_1.95.2"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\xmltok.c

!IF  "$(CFG)" == "expfilter - Win32 Release"

# ADD CPP /D "_DEBUG" /D "COMPILED_FROM_DSP" /D "_WINDOWS" /D "_CONSOLE_" /D "EXPAT_EXPORTS" /D VERSION=\"expat_1.95.2\"
# SUBTRACT CPP /D "NDEBUG" /D "_CONSOLE" /D VERSION="expat_1.95.2"

!ELSEIF  "$(CFG)" == "expfilter - Win32 Debug"

# ADD CPP /D "_CONSOLE_" /D "COMPILED_FROM_DSP" /D "_WINDOWS" /D "EXPAT_EXPORTS" /D VERSION=\"expat_1.95.2\"
# SUBTRACT CPP /D "_CONSOLE" /D VERSION="expat_1.95.2"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\xmltok_impl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\xmltok_ns.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Data files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=E:\SVG\Tamapatz\symbols.svg
# End Source File
# Begin Source File

SOURCE=E:\SVG\Tamapatz\symbols_ai.svg
# End Source File
# Begin Source File

SOURCE=E:\SVG\Tamapatz\vecs_id.svg
# End Source File
# Begin Source File

SOURCE=E:\SVG\Tamapatz\vecs_id_ai.svg
# End Source File
# Begin Source File

SOURCE=E:\SVG\Tamapatz\vecs_id_aif.svg
# End Source File
# Begin Source File

SOURCE=E:\SVG\Tamapatz\vecs_idf.svg
# End Source File
# End Group
# End Target
# End Project
