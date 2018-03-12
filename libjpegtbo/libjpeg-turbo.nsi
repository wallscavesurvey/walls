!include x64.nsh
Name "libjpeg-turbo SDK for Visual C++"
OutFile "Y:/Documents/trx/libjpegtbo\${BUILDDIR}libjpeg-turbo-1.4.1-vc.exe"
InstallDir c:\libjpeg-turbo

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
	File "Y:/Documents/trx/libjpegtbo\libturbojpeg.dll"
!else
	File "Y:/Documents/trx/libjpegtbo\${BUILDDIR}turbojpeg.dll"
!endif
	SetOutPath $INSTDIR\bin
!ifdef GCC
	File "Y:/Documents/trx/libjpegtbo\libturbojpeg.dll"
!else
	File "Y:/Documents/trx/libjpegtbo\${BUILDDIR}turbojpeg.dll"
!endif
!ifdef GCC
	File "/oname=libjpeg-62.dll" "Y:/Documents/trx/libjpegtbo\sharedlib\libjpeg-*.dll"
!else
	File "Y:/Documents/trx/libjpegtbo\sharedlib\${BUILDDIR}jpeg62.dll"
!endif
	File "Y:/Documents/trx/libjpegtbo\sharedlib\${BUILDDIR}cjpeg.exe"
	File "Y:/Documents/trx/libjpegtbo\sharedlib\${BUILDDIR}djpeg.exe"
	File "Y:/Documents/trx/libjpegtbo\sharedlib\${BUILDDIR}jpegtran.exe"
	File "Y:/Documents/trx/libjpegtbo\${BUILDDIR}tjbench.exe"
	File "Y:/Documents/trx/libjpegtbo\${BUILDDIR}rdjpgcom.exe"
	File "Y:/Documents/trx/libjpegtbo\${BUILDDIR}wrjpgcom.exe"
	SetOutPath $INSTDIR\lib
!ifdef GCC
	File "Y:/Documents/trx/libjpegtbo\libturbojpeg.dll.a"
	File "Y:/Documents/trx/libjpegtbo\libturbojpeg.a"
	File "Y:/Documents/trx/libjpegtbo\sharedlib\libjpeg.dll.a"
	File "Y:/Documents/trx/libjpegtbo\libjpeg.a"
!else
	File "Y:/Documents/trx/libjpegtbo\${BUILDDIR}turbojpeg.lib"
	File "Y:/Documents/trx/libjpegtbo\${BUILDDIR}turbojpeg-static.lib"
	File "Y:/Documents/trx/libjpegtbo\sharedlib\${BUILDDIR}jpeg.lib"
	File "Y:/Documents/trx/libjpegtbo\${BUILDDIR}jpeg-static.lib"
!endif
!ifdef JAVA
	SetOutPath $INSTDIR\classes
	File "Y:/Documents/trx/libjpegtbo\java\${BUILDDIR}turbojpeg.jar"
!endif
	SetOutPath $INSTDIR\include
	File "Y:/Documents/trx/libjpegtbo\jconfig.h"
	File "Y:/Documents/trx/libjpeg-turbo\jerror.h"
	File "Y:/Documents/trx/libjpeg-turbo\jmorecfg.h"
	File "Y:/Documents/trx/libjpeg-turbo\jpeglib.h"
	File "Y:/Documents/trx/libjpeg-turbo\turbojpeg.h"
	SetOutPath $INSTDIR\doc
	File "Y:/Documents/trx/libjpeg-turbo\README"
	File "Y:/Documents/trx/libjpeg-turbo\README-turbo.txt"
	File "Y:/Documents/trx/libjpeg-turbo\example.c"
	File "Y:/Documents/trx/libjpeg-turbo\libjpeg.txt"
	File "Y:/Documents/trx/libjpeg-turbo\structure.txt"
	File "Y:/Documents/trx/libjpeg-turbo\usage.txt"
	File "Y:/Documents/trx/libjpeg-turbo\wizard.txt"

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
