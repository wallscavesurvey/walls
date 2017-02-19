// hagapp.h : main header file for Walls3D application
// written in July 1995
// Gerbert Orasche
// (C) Computer Supported New Media (IICM)
// Graz University Of Technology
//

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#ifndef SC_VIEWER_HGAPP_H
#define SC_VIEWER_HGAPP_H

#include "resource.h"       // main symbols
#include "pcutil/mfcutil.h"


/////////////////////////////////////////////////////////////////////////////
// CPsviewApp:
// See psview.cpp for the implementation of this class
//

//class CSceneViewer;

class CSceneApp : public CApplication
{
public:
    CSceneApp();
    ~CSceneApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSceneApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	//}}AFX_VIRTUAL
#ifdef _MAC
	virtual BOOL CreateInitialDocument();
#endif

// members
public:
	virtual CDocument * OpenDocumentFile(LPCTSTR lpszFileName);
  // main viewer class
  //CSceneViewer* scViewer_;

  static bool m_bCmdLineOpen;

protected:
    BOOL FirstInstance();

    //{{AFX_MSG(CSceneApp)
    afx_msg void OnAppAbout();
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

extern CSceneApp NEAR theApp;
/*#include "afxdisp.h"
COleTemplateServer m_server;*/

#endif
