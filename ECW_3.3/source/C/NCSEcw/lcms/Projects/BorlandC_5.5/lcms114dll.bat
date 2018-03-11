@echo off
echo.
echo This will build the littlecms DLL using Borland C 5.5 compiler.
echo.
echo Press Ctrl-C to abort, or
pause
bcc32 @lcms114dll.lst
if errorlevel 0 tlink32 @lcms114dll.lk
if errorlevel 0 brc32 -fe ..\..\bin\lcms.dll lcms.rc
del *.obj
del ..\..\src\*.res
echo Done!
								
					
