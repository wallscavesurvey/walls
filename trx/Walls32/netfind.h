// netfind.h : header file
//
#include "tabcombo.h"

#define SIZ_EDIT2NAMEBUF 64
extern CTabComboHist hist_netfind;

/////////////////////////////////////////////////////////////////////////////
// CNetFindDlg dialog

class CNetFindDlg : public CDialog
{
// Construction
public:
	CNetFindDlg(CWnd* pParent = NULL);	// standard constructor

	static BOOL	m_bMatchCase;
	BOOL m_bUseCase;

	//{{AFX_DATA(CNetFindDlg)
	enum { IDD = IDD_NETFINDDLG };
	CString	m_Names;
	//}}AFX_DATA

// Implementation
protected:
	CTabCombo m_ce;

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CNetFindDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
