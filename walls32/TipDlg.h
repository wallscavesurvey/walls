#if !defined(TIPDLG_H_INCLUDED_)
#define TIPDLG_H_INCLUDED_

// CG: This file added by 'Tip of the Day' component.

/////////////////////////////////////////////////////////////////////////////
// CTipDlg dialog

#define TIP_MINIDHELP	100
#define TIP_MAXIDHELP	1000

class CTipDlg : public CDialog
{
// Construction
public:
	CTipDlg(CWnd* pParent = NULL);	 // standard constructor

// Dialog Data
	//{{AFX_DATA(CTipDlg)
	// enum { IDD = IDD_TIP };
	BOOL	m_bStartup;
	CString	m_strTip;
	//}}AFX_DATA

protected:
	BOOL m_idMax;
	int	 m_id;
	int  m_idHelp;
	int  m_iVersion;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTipDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTipDlg();

protected:
	// Generated message map functions
	afx_msg void OnPrevTip(); 
	//{{AFX_MSG(CTipDlg)
	afx_msg void OnNextTip();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    afx_msg void OnTellMeMore();
};

#endif // !defined(TIPDLG_H_INCLUDED_)
