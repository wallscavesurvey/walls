// impdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImpDlg dialog

class CImpDlg : public CDialog
{
// Construction
public:
	CImpDlg(CWnd* pParent = NULL);	// standard constructor
	BOOL bImporting;

// Dialog Data
	//{{AFX_DATA(CImpDlg)
	enum { IDD = IDD_IMPORTDLG };
	CString	m_PrjPath;
	CString	m_SefPath;
	BOOL	m_bCombine;
	//}}AFX_DATA

// Implementation
protected:
	void ShowControl(UINT idc,BOOL bShow);

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    virtual BOOL OnInitDialog();
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
 
	// Generated message map functions
	//{{AFX_MSG(CImpDlg)
	afx_msg void OnImpbrowse();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
