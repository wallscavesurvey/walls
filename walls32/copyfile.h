// copyfile.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCopyFileDlg dialog

class CCopyFileDlg : public CDialog
{
	// Construction
public:
	CCopyFileDlg(LPSTR pTitle, CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyFileDlg)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CString m_szDateSrc;
	CString m_szDateTgt;

	// Implementation
protected:
	LPSTR m_pTitle;

	// Generated message map functions
	//{{AFX_MSG(CCopyFileDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSkip();
	afx_msg void OnReplaceAll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

int MyCopyFile(const char *source, char *target, int bOverwrite);
extern BOOL bMoveFile;
extern BOOL bCopiedFile;