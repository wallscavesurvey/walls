#if !defined(AFX_LRUDDLG_H__04C0F481_7A2B_4E63_A739_4177A88A8E62__INCLUDED_)
#define AFX_LRUDDLG_H__04C0F481_7A2B_4E63_A739_4177A88A8E62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LrudDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLrudDlg dialog

class CSegView;

class CLrudDlg : public CDialog
{
// Construction
public:
	CLrudDlg(CSegView *pSV,double *pfThickPenPts,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLrudDlg)
	enum { IDD = IDD_LRUDSTYLE };
	int		m_iLrudStyle;
	BOOL	m_bSVGUseIfAvail;
	CString	m_ThickPenPts;
	//}}AFX_DATA
	CString m_LrudExag;
	double *m_pfThickPenPts;
	int m_iEnlarge;
	CSegView *m_pSV;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLrudDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CLrudDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LRUDDLG_H__04C0F481_7A2B_4E63_A739_4177A88A8E62__INCLUDED_)
