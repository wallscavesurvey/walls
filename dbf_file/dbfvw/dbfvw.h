// dbfvw.h : main header file for the DBFVW application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "mainfrm.h"
#include "dbffrm.h"
#include "dbrcdoc.h"
#include "dbfdoc.h"
#include "dbrcview.h"
#include "dbfview.h"

/////////////////////////////////////////////////////////////////////////////
// CDbfvwApp:
// See dbfvw.cpp for the implementation of this class
//

class CDbfvwApp : public CWinApp
{
public:
    CDbfvwApp();

// Operations
    CDocument *OpenDatabase(char *pszPathName);
	void UpdateIniFileWithDocPath(const char* pszPathName);
    BOOL DoPromptOpenFile(CString& pathName,DWORD lFlags,
         int numFilters,CString &strFilter);

// Overrides
	virtual CDocument *OpenDocumentFile(LPCSTR pszPathName);
	virtual void AddToRecentFileList(const char* pszPathName);
	virtual BOOL InitInstance();

// Implementation

        //{{AFX_MSG(CDbfvwApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CDbfvwApp theApp;

/////////////////////////////////////////////////////////////////////////////
