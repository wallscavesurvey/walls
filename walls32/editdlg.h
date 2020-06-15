// CEditDlg dialog

class CEditDlg : public CDialog
{
	// Construction
public:
	CEditDlg(UINT uFlags, int iTyp, CWnd* pParent = NULL);

	// Dialog Data
		//{{AFX_DATA(CEditDlg)
	enum { IDD = IDD_EDITOPTIONS };
	BOOL	m_bGridLines;
	BOOL	m_bTabLines;
	BOOL	m_bEnterTabs;
	UINT	m_nTabInterval;
	BOOL	m_bAutoSeq;
	//}}AFX_DATA

	UINT m_nHeight;
	UINT m_nWidth;
	UINT m_uFlags;
	int m_iTyp;

	// Implementation
protected:

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CEditDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
