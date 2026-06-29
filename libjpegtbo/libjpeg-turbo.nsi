!include x64.nsh
Name "libjpeg-turbo SDK for Visual C++"
OutFile "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}libjpeg-turbo-1.4.1-vc.exe"
InstallDir C:\Users\jedwa\walls\libjpegtbo

SetCompressor bzip2

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "libjpeg-turbo SDK for Visual C++ (required)"
!ifdef WIN64
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	${Endif}
!endif
	SectionIn RO
!ifdef GCC
	IfFileExists $SYSDIR/libturbojpeg.dll exists 0
!else
	IfFileExists $SYSDIR/turbojpeg.dll exists 0
!endif
	goto notexists
	exists:
!ifdef GCC
	MessageBox MB_OK "An existing version of the libjpeg-turbo SDK for Visual C++ is already installed.  Please uninstall it first."
!else
	MessageBox MB_OK "An existing version of the libjpeg-turbo SDK for Visual C++ or the TurboJPEG SDK is already installed.  Please uninstall it first."
!endif
	quit

	notexists:
	SetOutPath $SYSDIR
!ifdef GCC
	File "C:/Users/jedwa/walls/libjpegtbo\libturbojpeg.dll"
!else
	File "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}turbojpeg.dll"
!endif
	SetOutPath $INSTDIR\bin
!ifdef GCC
	File "C:/Users/jedwa/walls/libjpegtbo\libturbojpeg.dll"
!else
	File "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}turbojpeg.dll"
!endif
!ifdef GCC
	File "/oname=libjpeg-62.dll" "C:/Users/jedwa/walls/libjpegtbo\sharedlib\libjpeg-*.dll"
!else
	File "C:/Users/jedwa/walls/libjpegtbo\sharedlib\${BUILDDIR}jpeg62.dll"
!endif
	File "C:/Users/jedwa/walls/libjpegtbo\sharedlib\${BUILDDIR}cjpeg.exe"
	File "C:/Users/jedwa/walls/libjpegtbo\sharedlib\${BUILDDIR}djpeg.exe"
	File "C:/Users/jedwa/walls/libjpegtbo\sharedlib\${BUILDDIR}jpegtran.exe"
	File "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}tjbench.exe"
	File "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}rdjpgcom.exe"
	File "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}wrjpgcom.exe"
	SetOutPath $INSTDIR\lib
!ifdef GCC
	File "C:/Users/jedwa/walls/libjpegtbo\libturbojpeg.dll.a"
	File "C:/Users/jedwa/walls/libjpegtbo\libturbojpeg.a"
	File "C:/Users/jedwa/walls/libjpegtbo\sharedlib\libjpeg.dll.a"
	File "C:/Users/jedwa/walls/libjpegtbo\libjpeg.a"
!else
	File "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}turbojpeg.lib"
	File "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}turbojpeg-static.lib"
	File "C:/Users/jedwa/walls/libjpegtbo\sharedlib\${BUILDDIR}jpeg.lib"
	File "C:/Users/jedwa/walls/libjpegtbo\${BUILDDIR}jpeg-static.lib"
!endif
!ifdef JAVA
	SetOutPath $INSTDIR\classes
	File "C:/Users/jedwa/walls/libjpegtbo\java\${BUILDDIR}turbojpeg.jar"
!endif
	SetOutPath $INSTDIR\include
	File "C:/Users/jedwa/walls/libjpegtbo\jconfig.h"
	File "C:/Users/jedwa/walls/libjpeg-turbo\jerror.h"
	File "C:/Users/jedwa/walls/libjpeg-turbo\jmorecfg.h"
	File "C:/Users/jedwa/walls/libjpeg-turbo\jpeglib.h"
	File "C:/Users/jedwa/walls/libjpeg-turbo\turbojpeg.h"
	SetOutPath $INSTDIR\doc
	File "C:/Users/jedwa/walls/libjpeg-turbo\README"
	File "C:/Users/jedwa/walls/libjpeg-turbo\README-turbo.txt"
	File "C:/Users/jedwa/walls/libjpeg-turbo\example.c"
	File "C:/Users/jedwa/walls/libjpeg-turbo\libjpeg.txt"
	File "C:/Users/jedwa/walls/libjpeg-turbo\structure.txt"
	File "C:/Users/jedwa/walls/libjpeg-turbo\usage.txt"
	File "C:/Users/jedwa/walls/libjpeg-turbo\wizard.txt"

	WriteRegStr HKLM "SOFTWARE\libjpeg-turbo 1.4.1" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo 1.4.1" "DisplayName" "libjpeg-turbo SDK v1.4.1 for Visual C++"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo 1.4.1" "UninstallString" '"$INSTDIR\uninstall_1.4.1.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo 1.4.1" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo 1.4.1" "NoRepair" 1
	WriteUninstaller "uninstall_1.4.1.exe"
SectionEnd

Section "Uninstall"
!ifdef WIN64
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	${Endif}
!endif

	SetShellVarContext all

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo 1.4.1"
	DeleteRegKey HKLM "SOFTWARE\libjpeg-turbo 1.4.1"

!ifdef GCC
	Delete $INSTDIR\bin\libjpeg-62.dll
	Delete $INSTDIR\bin\libturbojpeg.dll
	Delete $SYSDIR\libturbojpeg.dll
	Delete $INSTDIR\lib\libturbojpeg.dll.a"
	Delete $INSTDIR\lib\libturbojpeg.a"
	Delete $INSTDIR\lib\libjpeg.dll.a"
	Delete $INSTDIR\lib\libjpeg.a"
!else
	Delete $INSTDIR\bin\jpeg62.dll
	Delete $INSTDIR\bin\turbojpeg.dll
	Delete $SYSDIR\turbojpeg.dll
	Delete $INSTDIR\lib\jpeg.lib
	Delete $INSTDIR\lib\jpeg-static.lib
	Delete $INSTDIR\lib\turbojpeg.lib
	Delete $INSTDIR\lib\turbojpeg-static.lib
!endif
!ifdef JAVA
	Delete $INSTDIR\classes\turbojpeg.jar
!endif
	Delete $INSTDIR\bin\cjpeg.exe
	Delete $INSTDIR\bin\djpeg.exe
	Delete $INSTDIR\bin\jpegtran.exe
	Delete $INSTDIR\bin\tjbench.exe
	Delete $INSTDIR\bin\rdjpgcom.exe
	Delete $INSTDIR\bin\wrjpgcom.exe
	Delete $INSTDIR\include\jconfig.h"
	Delete $INSTDIR\include\jerror.h"
	Delete $INSTDIR\include\jmorecfg.h"
	Delete $INSTDIR\include\jpeglib.h"
	Delete $INSTDIR\include\turbojpeg.h"
	Delete $INSTDIR\uninstall_1.4.1.exe
	Delete $INSTDIR\doc\README
	Delete $INSTDIR\doc\README-turbo.txt
	Delete $INSTDIR\doc\example.c
	Delete $INSTDIR\doc\libjpeg.txt
	Delete $INSTDIR\doc\structure.txt
	Delete $INSTDIR\doc\usage.txt
	Delete $INSTDIR\doc\wizard.txt

	RMDir "$INSTDIR\include"
	RMDir "$INSTDIR\lib"
	RMDir "$INSTDIR\doc"
!ifdef JAVA
	RMDir "$INSTDIR\classes"
!endif
	RMDir "$INSTDIR\bin"
	RMDir "$INSTDIR"

SectionEnd
