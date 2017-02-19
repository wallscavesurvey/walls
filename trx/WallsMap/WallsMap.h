// WallsMap.h : main header file for the WallsMap application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "logfont.h"

extern LPCTSTR lpszUniqueClass;

//=========================================================
//Taken from walls32/mainfrm.h -- should be moved to wallmag.h:
enum e_ref {REF_FMTDM=1,REF_FMTDMS=2,REF_FMTMASK=3,REF_LONDIR=4,REF_LATDIR=8};

#define REF_NAD27 254
#define REF_MAXDATUM 250
#define REF_NUMFLDS 14
#define REF_SIZDATUMNAME 20

typedef struct { //2*8 + 4*4 + 2 + 6 + 20 = 60 bytes
	double north;
	double east;
	int elev;
	float conv;
	float latsec;
	float lonsec;
	short zone;
	BYTE latmin;
	BYTE lonmin;
	BYTE latdeg;
	BYTE londeg;
	BYTE flags;
	BYTE datum;
	char datumname[REF_SIZDATUMNAME];
} MAGREF; //was PRJREF

extern HINSTANCE mag_hinst;
void mag_GetRef(MAGREF *mr);
void mag_PutRef(MAGREF *mr);
HINSTANCE LoadMagDLL(MAGREF *mr=NULL);

//=========================================================
// See WallsMap.cpp for the implementation of this class
//


#define _CHK_COMMANDLINE

class CWallsMapDoc;

class CWallsMapApp : public CWinApp
{
public:
	CWallsMapApp();

#ifdef _CHK_COMMANDLINE
	void ParseCommandLine0(CCommandLineInfo& rCmdInfo);
#endif
	void UpdateRecentMapFile(const char* pszPathName);

private:
	BOOL RegisterWallsMapClass();

// Overrides
public:
	HCURSOR m_hCursorZoomIn,m_hCursorZoomOut,m_hCursorHand,m_hCursorMeasure,
		m_hCursorCross,m_hCursorArrow,m_hCursorSelect,m_hCursorSelectAdd;

    static CMultiDocTemplate *m_pDocTemplate;
	static BOOL m_bBackground;
	static BOOL m_bNoAddRecent;
	CString m_csUrlPath;

// Overrides
	virtual void WinHelpInternal(DWORD_PTR dwData, UINT nCmd = HELP_CONTEXT);
	virtual void WinHelp(DWORD dwData,UINT nCmd=HELP_CONTEXT);
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void AddToRecentFileList(const char* pszPathName);
	
// Implementation
	afx_msg BOOL OnOpenRecentFile(UINT nID);
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CWallsMapApp theApp;

class CMainFrame;

__inline CMainFrame * GetMF() {return (CMainFrame *)theApp.m_pMainWnd;}

class COpenExDlg : public CDialog
{
// Construction
public:
	COpenExDlg(CString &msg,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	CString &m_msg;

	//{{AFX_DATA(CErrorDlg)
	enum { IDD = IDD_OPENEXISTING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CErrorDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CErrorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnOpenExisting();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
