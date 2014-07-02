// prjfind.h : header file
//
#include "tabcombo.h"

/////////////////////////////////////////////////////////////////////////////
// CPrjFindDlg dialog

class CPrjFindDlg : public CDialog
{
// Construction
public:
	CPrjFindDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CPrjFindDlg)
	enum { IDD = IDD_PRJFINDDLG };
	CString	m_FindString;
	BOOL	m_bMatchCase;
	BOOL	m_bMatchWholeWord;
	BOOL	m_bSearchDetached;
	BOOL	m_bUseRegEx;
	//}}AFX_DATA
	int m_nNumChecked;
	WORD m_wFlags;
	CTabCombo m_tabEdit;

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CPrjFindDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClearChks();
	afx_msg void OnSelchangeFindstring();
	afx_msg void OnEditchangeFindstring();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
