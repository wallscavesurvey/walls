#pragma once
#include "afxwin.h"


// CTableFillDlg dialog

class CDBGridDlg;

class CTableFillDlg : public CDialog
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

private:
    CDBGridDlg *m_pGDlg;
	WORD m_nFld;

public:
	CString m_csFillText;
	virtual BOOL OnInitDialog();
	BOOL m_bYesNo;
};
