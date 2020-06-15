#pragma once
#include "afxwin.h"


// CTableReplDlg dialog

class CDBGridDlg;

class CTableReplDlg : public CDialog
{
	DECLARE_DYNAMIC(CTableReplDlg)

public:
	CTableReplDlg(WORD nFld, CWnd* pParent = NULL);   // standard constructor
	virtual ~CTableReplDlg();

	// Dialog Data
	enum { IDD = IDD_TABLE_REPL };

	CString m_csFindWhat;
	CString m_csReplWith;
	BOOL m_bExtended;
	BOOL m_bMatchCase;
	BOOL m_bMatchWord;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedSelectAll();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

private:
	CDBGridDlg *m_pGDlg;
	int m_fLen;
	WORD m_nFld;
};
