// EditImageDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "EditImageDlg.h"


// CEditImageDlg dialog

IMPLEMENT_DYNAMIC(CEditImageDlg, CDialog)

CEditImageDlg::CEditImageDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditImageDlg::IDD, pParent)
{
}

CEditImageDlg::~CEditImageDlg()
{
}

void CEditImageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CAPTION, m_csTitle);
	DDV_MaxChars(pDX, m_csTitle, 80);
	DDX_Text(pDX, IDC_DESCRIPTION, m_csDesc);
	DDV_MaxChars(pDX, m_csDesc, 2048);
	DDX_Text(pDX, IDC_PATHNAME, m_csPath);
	DDV_MaxChars(pDX, m_csPath, 250);
}

BEGIN_MESSAGE_MAP(CEditImageDlg, CDialog)
	//ON_BN_CLICKED(IDC_BROWSE, &CEditImageDlg::OnBnClickedBrowse)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()


BOOL CEditImageDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(!m_bIsEditable) {
		SetRO(IDC_CAPTION);
		SetRO(IDC_DESCRIPTION);
	}
	else {
		VERIFY(m_ceTitle.SubclassDlgItem(IDC_CAPTION,this));
		VERIFY(m_ceDesc.SubclassDlgItem(IDC_DESCRIPTION,this));
	}
	GetDlgItem(IDC_ST_SIZE)->SetWindowText(m_csSize);

	if(m_bIsEditable) {
		GetDlgItem(IDC_DESCRIPTION)->SetFocus();
	}
	else {
		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
		GetDlgItem(IDCANCEL)->SetFocus();
	}
	return FALSE;  // return TRUE unless you set the focus to a control
}

LRESULT CEditImageDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}
