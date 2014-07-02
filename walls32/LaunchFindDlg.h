#if !defined(AFX_LAUNCHFINDDLG_H__13370BB4_DF0C_45A1_9B0E_4AA9EAE7CDE5__INCLUDED_)
#define AFX_LAUNCHFINDDLG_H__13370BB4_DF0C_45A1_9B0E_4AA9EAE7CDE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LaunchFindDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLaunchFindDlg dialog

class CLaunchFindDlg : public CDialog
{
// Construction
public:
	CLaunchFindDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLaunchFindDlg)
	enum { IDD = IDD_LAUNCHFIND };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLaunchFindDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLaunchFindDlg)
	afx_msg void OnNoFile();
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkLaunchlb();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LAUNCHFINDDLG_H__13370BB4_DF0C_45A1_9B0E_4AA9EAE7CDE5__INCLUDED_)
