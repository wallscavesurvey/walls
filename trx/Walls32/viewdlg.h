// viewdlg.h : header file
//

#undef CTLCOL 
struct VIEWFORMAT;

/////////////////////////////////////////////////////////////////////////////
// CViewDlg dialog

class CViewDlg : public CDialog
{
	DECLARE_DYNAMIC(CViewDlg)
// Construction
public:
	CViewDlg(VIEWFORMAT *pVF,const char *pNameOfOrigin,CWnd* pParent = NULL);	// standard constructor

#ifdef CTLCOL
	WNDPROC m_oldDlgProc;
	HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
#endif

	BOOL m_bInches;
	double m_fScaleRatio,m_fMetersPerCM;

	CLinkEditF m_leScaleRatio,m_leMetersPerCM;

	const char *m_pNameOfOrigin;
	//static BOOL m_bScaleRatio;

// Dialog Data
	//{{AFX_DATA(CViewDlg)
	enum { IDD = IDD_VIEWDIALOG };
	CString	m_CenterEast;
	CString	m_CenterNorth;
	CString	m_CenterRef;
	BOOL	m_bMatchCase;
	BOOL	m_bSaveView;
	CString	m_PanelView;
	CString	m_FrameHeight;
	CString	m_FrameWidth;
	CString	m_CenterUp;
	//}}AFX_DATA

private:
	void RefreshPageUnits();
	void SwitchPageUnits(UINT id,BOOL bInches);

// Implementation
public:
	void UpdateRatioFromMeters();
	void UpdateMetersFromRatio();

protected:

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	VIEWFORMAT *m_pVF;
	
	UINT CheckData();
	
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
    
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
   
	// Generated message map functions
	//{{AFX_MSG(CViewDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnInches();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
