// aboutdlg.h : header file
//
#include "StatLink.h"

class CUpLnkDlg : public CDialog
{
	// Construction
public:
	CUpLnkDlg(LPCSTR pURL, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUpLnkDlg)
	enum { IDD = IDD_UPDATELINK };
	CString m_NewURL;
	// NOTE: the ClassWizard will add data members here
//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUpLnkDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CUpLnkDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog

class CAboutDlg : public CDialog
{
	// Construction
public:
	CAboutDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	// NOTE: the ClassWizard will add data members here
//}}AFX_DATA

// Implementation
protected:

	CStaticLink m_textLink;		// static text

	// Generated message map functions
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnUpdateLink();
	afx_msg void OnBnClickedHelp();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
