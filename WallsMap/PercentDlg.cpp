// PercentDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "PercentDlg.h"

// CPercentDlg dialog

IMPLEMENT_DYNAMIC(CPercentDlg, CDialog)
CPercentDlg::CPercentDlg(double percent, CWnd* pParent /*=NULL*/)
	: CDialog(CPercentDlg::IDD, pParent)
{
	m_csPercent.Format("%.3f", m_fPercent = percent);
}

CPercentDlg::~CPercentDlg()
{
}

void CPercentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PERCENT, m_cePercent);
	DDX_Text(pDX, IDC_PERCENT, m_csPercent);
	DDV_MaxChars(pDX, m_csPercent, 10);
	if (pDX->m_bSaveAndValidate) {
		if (CheckZoomLevel(pDX, IDC_PERCENT, m_csPercent))
			m_fPercent = atof(m_csPercent);
	}
}

BEGIN_MESSAGE_MAP(CPercentDlg, CDialog)
END_MESSAGE_MAP()


// CPercentDlg message handlers

BOOL CPercentDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CEdit *ce = (CEdit *)GetDlgItem(IDC_PERCENT);
	ce->SetSel(0, -1);
	ce->SetFocus();

	return FALSE;
}
