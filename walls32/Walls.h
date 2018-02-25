// walls.h : main header file for the WALLS application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols 
#include "mainfrm.h"

#define _MAX_LEN_DOSFNAME 8
#define WALLS_FILEOPEN 1212001


/////////////////////////////////////////////////////////////////////////////
// CWallsApp:
// See walls.cpp for the implementation of this class
//

class CWallsApp : public CWinApp
{
	BOOL CWallsApp::FirstInstance();
public:
    static CMultiDocTemplate *m_pPrjTemplate;
    static CMultiDocTemplate *m_pSrvTemplate;
    static CMultiDocTemplate *m_pRevTemplate;
    static CMultiDocTemplate *m_pMapTemplate;
    
	CWallsApp();

// Operations
	void UpdateIniFileWithDocPath(const char* pszPathName);
	BOOL GetPrinterPageInfo(BOOL *pbLandscape,int *hres,int *vres,int *pxperinX,int *pxperinY);
	static void GetDefaultFrameSize(double &fWidth,double &fHeight,BOOL bPrinter);
	int SetLandscape(int bLandscape=1);
	CPrjDoc * GetOpenProject();
	CPrjDoc * FindOpenProject(LPCSTR pathname);
	void SendFileOpenCmd(CWnd *pWnd,LPCSTR pCmd);

// Overrides
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void AddToRecentFileList(const char* pszPathName);
    virtual void WinHelp(DWORD dwData,UINT nCmd=HELP_CONTEXT);
	virtual BOOL OnDDECommand(LPTSTR lpszCommand);
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	#if _MFC_VER >= 0x0700
	virtual void WinHelpInternal(DWORD_PTR dwData, UINT nCmd /*= HELP_CONTEXT*/);
	#endif
    
// Implementation
	#ifdef _USEHTML
	BOOL m_bWinHelp;
	afx_msg void OnHelpContents();
	#endif
	BOOL m_bDDEFileOpen;
	DWORD dwOsMajorVersion;
	LRESULT RunCommand(int fcn,LPCSTR pData);
	bool IsProjectOpen(LPCSTR pathName);

	afx_msg void OnFilePrintSetup();

    //{{AFX_MSG(CWallsApp)
	afx_msg void OnAppAbout();
	afx_msg void OnProjectNew();
	afx_msg void OnFileOpen();
	afx_msg void OnImportSef();
	afx_msg void OnImportCss();
	afx_msg void OnGpsDownload();
	afx_msg BOOL OnOpenRecentFile(UINT nID);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
	CDocument* OpenWallsDocument(LPCTSTR lpszFileName);
	void ShowTipAtStartup(void);
	void ShowTipOfTheDay(void);
	HANDLE m_hWalls2Mutex;
	BOOL m_bFirstInstance;
	UINT m_nID_RecentFile;
};

extern CWallsApp NEAR theApp;
extern LPCTSTR lpszUniqueClass;

/////////////////////////////////////////////////////////////////////////////
