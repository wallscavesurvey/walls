#if !defined(AFX_ZIPDLG_H__A80A08D5_CB86_4D97_9CE3_DC254D4E16CB__INCLUDED_)
#define AFX_ZIPDLG_H__A80A08D5_CB86_4D97_9CE3_DC254D4E16CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ZipDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CZipDlg dialog

class CZipDlg : public CDialog
{
// Construction
public:
	CZipDlg(CPrjDoc *pDoc,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CZipDlg)
	enum { IDD = IDD_ZIPDLG };
	CString	m_pathname;
	BOOL	m_bOpenZIP;
	//}}AFX_DATA

	static BOOL	m_bIncludeNTA;
	CPrjDoc *m_pDoc;
	CString m_Message;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CZipDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void freeFileArray();
	BOOL addBranchFiles(CPrjListNode *pNode);
	BOOL addFile(LPCSTR path);
	BOOL addSavedGrads(LPCSTR fspec);

	char **m_pFile;
	int m_iFileCnt,m_iFileLen;

	// Generated message map functions
	//{{AFX_MSG(CZipDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse();
	//}}AFX_MSG
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CZipCreateDlg dialog

class CZipCreateDlg : public CDialog
{
// Construction
public:
	CZipCreateDlg(CWnd* pParent = NULL);   // standard constructor

	LPCSTR m_pMessage;
	CString m_Title;

	BOOL m_bFcn;

// Dialog Data
	//{{AFX_DATA(CZipCreateDlg)
	enum { IDD = IDD_ZIPCREATED };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CZipCreateDlg)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CZipCreateDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnEmailZip();
	afx_msg void OnOpenZip();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ZIPDLG_H__A80A08D5_CB86_4D97_9CE3_DC254D4E16CB__INCLUDED_)
