// ImpSefDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImpSefDlg dialog

class CImpSefDlg : public CDialog
{
// Construction
public:
	enum { IDD = IDD_IMPSEFDLG };

	CImpSefDlg(CWnd* pParent = NULL);	// standard constructor
	CString	m_PrjPath;
	CString	m_SefPath;

private:
	CString m_csChrs_repl;
	BOOL m_bCombine;
	BOOL m_bAllowColons;
	BOOL m_bAttachSrc;

	BOOL bImporting;

	BOOL Check_Chrs_repl();

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    virtual BOOL OnInitDialog();
	virtual void OnCancel();

	afx_msg void OnChangeChrsRepl();
	afx_msg void OnBnClickedReplDflt();
	afx_msg void OnPrjbrowse();
	afx_msg void OnSefBrowse();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};
