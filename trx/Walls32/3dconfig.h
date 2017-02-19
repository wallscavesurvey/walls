// 3dconfig.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// C3DConfigDlg dialog

class C3DConfigDlg : public CDialog
{
// Construction
public:
	C3DConfigDlg(BOOL b3D,CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(C3DConfigDlg)
	enum { IDD = IDD_3DCONFIG };
	CString	m_3DPath;
	CString	m_3DViewer;
	BOOL	m_3DStatus;
	//}}AFX_DATA

// Implementation
protected:
	BOOL m_b3D;
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	//{{AFX_MSG(C3DConfigDlg)
	afx_msg void OnBrowse();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
