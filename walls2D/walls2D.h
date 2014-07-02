// walls2D.h : main header file for the WALLS2D application
//

#if !defined(AFX_WALLS2D_H__2267E92D_1180_419A_AE24_4D31EBB502B0__INCLUDED_)
#define AFX_WALLS2D_H__2267E92D_1180_419A_AE24_4D31EBB502B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWalls2DApp:
// See walls2D.cpp for the implementation of this class
//

struct DPOINT {
public:
	DPOINT(double X,double Y) : x(X),y(Y) {};
	DPOINT() {x=0.0;y=0.0;};
	double x,y;
};

class CWalls2DApp : public CWinApp
{
public:
	CWalls2DApp();
	~CWalls2DApp();

	class CImpIDispatch* m_pDispOM;
	DPOINT m_coord;
	static BOOL m_bPanning;
	static BOOL m_bNotHomed;
	static CDocument *m_pDocument;
	static CString m_csTitle;
	static BOOL m_bCoordPage;
	
	CSingleDocTemplate* m_pDocTemplate;
	CString m_wallsLink,m_adobeLink;
	LPSTR m_pszTempHTM;   // HTM template file in app directory
	LPSTR m_pszDfltSVG;   // Default SVG file in app directory

	void Navigate2Links(UINT id);
	LPCSTR GetRecentFile(UINT nIndex);
	LPCSTR GetRecentFile();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWalls2DApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CWalls2DApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	afx_msg BOOL OnOpenRecentFile(UINT nID);
	DECLARE_MESSAGE_MAP()
};

extern CWalls2DApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WALLS2D_H__2267E92D_1180_419A_AE24_4D31EBB502B0__INCLUDED_)
