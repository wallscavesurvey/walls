// pltfind.h : header file
//
#include "tabcombo.h"

/////////////////////////////////////////////////////////////////////////////
// CPltFindDlg dialog

extern CTabComboHist hist_pltfind;

class CPltFindDlg : public CDialog
{
// Construction
public:
	CPltFindDlg(CWnd* pParent = NULL);	// standard constructor
	static BOOL m_bMatchCase;

// Dialog Data
	//{{AFX_DATA(CPltFindDlg)
	enum { IDD = IDD_PLTFINDDLG };
	CString	m_Name;
	BOOL	m_bUseCase;
	//}}AFX_DATA

// Implementation
protected:
	CTabCombo m_ce;
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CPltFindDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
