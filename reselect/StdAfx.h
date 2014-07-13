// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__EDA4B782_A216_43F4_95B4_CF9203BA8538__INCLUDED_)
#define AFX_STDAFX_H__EDA4B782_A216_43F4_95B4_CF9203BA8538__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS
#define NO_WARN_MBCS_MFC_DEPRECATION
#ifndef _DEBUG
#ifndef _USING_V110_SDK71_
#define _USING_V110_SDK71_  //needed for release if targeting XP for some reason -- causes two warnings safe to ignore
#endif
#endif

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0501		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0501 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0501	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxinet.h> 
#include <afxhtml.h>

#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <trx_str.h>
#include <dbf_file.h>
#include <trx_file.h>

#endif // !defined(AFX_STDAFX_H__EDA4B782_A216_43F4_95B4_CF9203BA8538__INCLUDED_)
