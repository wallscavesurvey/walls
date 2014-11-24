#pragma once
#include "afxwin.h"
#include "RulerRichEditCtrl.h"

// CTableFillDlg dialog

class CDBGridDlg;

class CTableFillDlg : public CResizeDlg
{
	DECLARE_DYNAMIC(CTableFillDlg)

public:
	CTableFillDlg(WORD nFld,CWnd* pParent = NULL);   // standard constructor
	virtual ~CTableFillDlg();

// Dialog Data
	enum { IDD = IDD_TABLE_FILL };

public:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
	DECLARE_MESSAGE_MAP()
	char m_fTyp;

private:
	CStatic	m_placement;
	CRulerRichEditCtrl	m_rtf;
    CDBGridDlg *m_pGDlg;
	WORD m_nFld;

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedEditPaste();
	afx_msg void OnBnClickedSelectAll();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

public:
	CString m_csFillText;
	BOOL m_bYesNo;
	bool m_bMemo;
};
