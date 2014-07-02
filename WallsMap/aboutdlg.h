// aboutdlg.h : header file
//
#include "StatLink.h"

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
	afx_msg void OnBnClickedMemoryUsage();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnUpdateLink();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
