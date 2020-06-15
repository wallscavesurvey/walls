// vecdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVecDlg dialog

class CVecDlg : public CDialog
{
	// Construction
public:
	CVecDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVecDlg)
	enum { IDD = IDD_VECLIMITS };
	CString	m_Azimuth;
	CString	m_Distance;
	CString	m_VertAngle;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CVecDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
