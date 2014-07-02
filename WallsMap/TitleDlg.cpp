// TitleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "TitleDlg.h"
#include ".\titledlg.h"


// CTitleDlg dialog

IMPLEMENT_DYNAMIC(CTitleDlg, CDialog)
CTitleDlg::CTitleDlg(LPCSTR pTitle,LPCSTR pWinTitle,CWnd* pParent /*=NULL*/)
	: CDialog(CTitleDlg::IDD, pParent)
	, m_title(pTitle)
	, m_pWinTitle(pWinTitle)
	, m_bRO(FALSE)
{
}

CTitleDlg::~CTitleDlg()
{
}

void CTitleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_TITLE, m_title);
	DDV_MaxChars(pDX, m_title, 80);
}

BEGIN_MESSAGE_MAP(CTitleDlg, CDialog)
END_MESSAGE_MAP()

// CTitleDlg message handlers

BOOL CTitleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	if(m_pWinTitle) SetWindowText(m_pWinTitle);
	VERIFY(m_cea.SubclassDlgItem(IDC_TITLE,this));
	if(m_bRO) {
		m_cea.SetReadOnly();
		GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
		GetDlgItem(IDOK)->SetFocus();
	}
	else {
		m_cea.SetSel(0,-1);
		m_cea.SetFocus();
	}
	return FALSE;  // return TRUE unless you set the focus to a control
}
