// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifdef _USE_THEMES
//#if 0
#if _MSC_VER >= 1400
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#define USEXPTHEMES
#endif
#endif

// NOTE: Targeting XP (0x0501) deactivates tooltips in CWallsMapView!!!
// Also, ECW SDK supposedly requires 0x0500 to prevent "<L_TYPE_raw>" error messages.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
//#define WINVER 0x0500		// Change this to the appropriate value to target Windows 2000 or later.
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0500	// Change this to the appropriate value to target Windows 2000 or later.
#define _WIN32_WINNT 0x0501	// Changed to 0x0501 to target Windows XP or later (AFTER tooltip structure in CWallsMapView was shortened!!!!).
#endif						

/*
#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
//#define _WIN32_WINDOWS 0x0501 // Change this to the appropriate value to target Windows Me or later.
#endif
*/

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0500	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxpriv.h> //WM_COMMANDHELP
//#include <afxsock.h>		// MFC socket extensions
#include <afxdlgs.h>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <io.h>
#include <process.h>
#include <dos_io.h>
#include <errno.h>
#include <trx_str.h>

#define _USE_TIMER

