#pragma once
#include "afxwin.h"
#include "RulerRichEditCtrl.h"

// CTableFillDlg dialog

class CDBGridDlg;

class CTableFillDlg : public CResizeDlg
{
	DECLARE_DYNAMIC(CTableFillDlg)

public:
	CTableFillDlg(WORD nFld, CWnd* pParent = NULL);   // standard constructor
	virtual ~CTableFillDlg();

	// Dialog Data
	enum { IDD = IDD_TABLE_FILL };

public:
	CString m_csFillText;
	BOOL m_bYesNo;
	bool m_bMemo;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

private:
	CStatic	m_placement;
	CRulerRichEditCtrl	m_rtf;
	CDBGridDlg *m_pGDlg;
	WORD m_nFld;
	char m_fTyp;

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedEditPaste();
	afx_msg void OnBnClickedSelectAll();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
