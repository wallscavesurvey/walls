// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//		are changed infrequently
//
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS
#define NO_WARN_MBCS_MFC_DEPRECATION

#define VISTA_INC

#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0600		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0600		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#ifndef _WIN32_IE			// Allow use of features specific to IE 5.0 or later.
#define _WIN32_IE 0x0900	// Change this to the appropriate value to target IE 5.0 or later.
#endif

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#define _USEHTML

#include <afxwin.h>			// MFC core and standard components
#include <afxext.h> 		// MFC extensions (including VB)
#include <afxcmn.h>
#include <limits.h>         // for INT_MAX
#include <math.h>
#include <float.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <afxpriv.h>

#include <trx_str.h>

#ifdef _USEHTML
#include <htmlhelp.h>
#endif

#define WALLS_RELEASE 7		// Rel B6 (tooltip reinitialize if changed)
#include "utility.h"
#include "prjfont.h"

