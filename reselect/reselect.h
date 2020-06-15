// reselect.h : main header file for the RESELECT application
//

#if !defined(AFX_RESELECT_H__0EB3D5DA_76DF_4661_B431_CBFCA2FDBB0A__INCLUDED_)
#define AFX_RESELECT_H__0EB3D5DA_76DF_4661_B431_CBFCA2FDBB0A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CReselectApp:
// See reselect.cpp for the implementation of this class
//

class CReselectApp : public CWinApp
{
public:
	CReselectApp();
	~CReselectApp();

	class CImpIDispatch* m_pDispOM;

	CSingleDocTemplate* m_pDocTemplate;

	char m_texbib_keybuf[128];
	char m_nwbb_keybuf[128];
	char m_texbib_nambuf[128];
	char m_nwbb_nambuf[128];
	char m_texbib_date[32];
	char m_nwbb_date[32];
	UINT m_texbib_bFlags, m_nwbb_bFlags;

	CString m_link;
	char m_szDBFilePath[_MAX_PATH];

	void ExecuteUrl(LPCSTR url);
	//bool TestURL(LPCSTR url);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReselectApp)
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CReselectApp)
	afx_msg void OnAppAbout();
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


extern CReselectApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESELECT_H__0EB3D5DA_76DF_4661_B431_CBFCA2FDBB0A__INCLUDED_)
