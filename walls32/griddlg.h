// griddlg.h : header file
//

#undef CTLCOL

/////////////////////////////////////////////////////////////////////////////
// CGridDlg dialog

struct GRIDFORMAT;

class CGridDlg : public CDialog
{
	DECLARE_DYNAMIC(CGridDlg)
// Construction
public:
	CGridDlg(GRIDFORMAT *pgf,const char *pNameOfOrigin,CWnd* pParent = NULL);	// standard constructor
#ifdef CTLCOL
	WNDPROC m_oldDlgProc;
#endif
	BOOL m_bChanged;
	
	const char *m_pNameOfOrigin;
	
#ifdef CTLCOL
	HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
#endif

// Dialog Data
	//{{AFX_DATA(CGridDlg)
	enum { IDD = IDD_GRIDOPTIONS };
	CString	m_TickInterval;
	CString	m_EastInterval;
	CString m_NorthEastRatio;
	CString	m_VertInterval;
	CString m_HorzVertRatio;
	CString	m_EastOffset;
	CString	m_HorzOffset;
	CString	m_NorthOffset;
	CString	m_VertOffset;
	CString	m_GridNorth;
	//}}AFX_DATA

// Implementation
protected:
	GRIDFORMAT *m_pGF;
	UINT CheckData();
	void FeetTitle(UINT id);

	void Enable(int id,BOOL bEnable)
	{
	   GetDlgItem(id)->EnableWindow(bEnable);
	}
    void SetCheck(int id,BOOL bChecked)
    {
       ((CButton *)GetDlgItem(id))->SetCheck(bChecked);
    }
    BOOL GetCheck(int id)
    {
       return ((CButton *)GetDlgItem(id))->GetCheck();
    }
    	
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CGridDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
