// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS         // remove support for MFC controls in dialogs
#define NO_WARN_MBCS_MFC_DEPRECATION
#define _CRT_SECURE_NO_WARNINGS          //Avoid strcpy, etc., error messages

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

//#include <windows.h>
#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
/*
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
*/

#include <vector>
#include <algorithm>
#include <assert.h>
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <time.h>
#include <io.h>
#include <errno.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <ctype.h>
#include <sys\stat.h>

#include <trx_str.h>
#include "resource.h"
#include "dos_io.h"
#include "utility.h"

typedef std::vector<CString> VEC_CSTR;
typedef VEC_CSTR::iterator it_cstr;
typedef VEC_CSTR::const_iterator cit_cstr;
//