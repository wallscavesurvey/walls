#if !defined(AFX_EXPSEFD_H__7AB9E932_E347_11D1_AFB3_000000000000__INCLUDED_)
#define AFX_EXPSEFD_H__7AB9E932_E347_11D1_AFB3_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// expsefd.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CExpSefDlg dialog
class CPrjDoc;

class CExpSefDlg : public CDialog
{
// Construction
public:
	CExpSefDlg(CWnd* pParent = NULL);   // standard constructor

	void StatusMsg(char *msg)
	{
		GetDlgItem(IDC_ST_STATUS)->SetWindowText(msg);
	}

// Dialog Data
	//{{AFX_DATA(CExpSefDlg)
	enum { IDD = IDD_EXPORTSEF };
	CString	m_pathname;
	//}}AFX_DATA

	static BOOL	m_bConvertPfx;
	static BOOL	m_bIncludeDet;
	static BOOL	m_bForceFeet;
	static BOOL m_bFixedCols;
	CPrjDoc *m_pDoc;
	CPrjListNode *m_pNode;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpSefDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CExpSefDlg)
	afx_msg void OnBrowse();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPSEFD_H__7AB9E932_E347_11D1_AFB3_000000000000__INCLUDED_)
