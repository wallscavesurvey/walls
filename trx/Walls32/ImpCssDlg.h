// impdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImpCssDlg dialog

#include "afxwin.h"
class CImpCssDlg : public CDialog
{
// Construction
public:
	enum { IDD = IDD_IMPCSSDLG };

	CImpCssDlg(CWnd* pParent = NULL);	// standard constructor
	CString	m_WpjPath;
	CString	m_CssPath;

// Implementation
private:
	CString m_csChrs_repl;
	BOOL m_bCombine;
	BOOL m_bAllowColons;
	BOOL m_bImporting;
	BOOL m_bAttachSrc;

	BOOL Check_Chrs_repl();

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    virtual BOOL OnInitDialog();
	virtual void OnCancel();
 
	// Generated message map functions
	afx_msg void OnChangeChrsRepl();
	afx_msg void OnBnClickedReplDflt();
	afx_msg void OnWpjBrowse();
	afx_msg void OnCssBrowse();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};
