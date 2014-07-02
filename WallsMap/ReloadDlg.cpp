// ReloadDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "ReloadDlg.h"
#include "afxdialogex.h"


// CReloadDlg dialog

IMPLEMENT_DYNAMIC(CReloadDlg, CDialog)

CReloadDlg::CReloadDlg(LPCSTR pTitle,int nSubset,int nDeleted,BOOL bRestoreLayout,CWnd* pParent /*=NULL*/)
	: CDialog(CReloadDlg::IDD, pParent)
	, m_pTitle(pTitle)
	, m_nSubset(nSubset)
	, m_nDeleted(nDeleted)
	, m_bRestoreLayout(bRestoreLayout)
{
}

CReloadDlg::~CReloadDlg()
{
}

void CReloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_UNDELETED, m_nSubset);
	DDX_Check(pDX, IDC_KEEP_LAYOUT, m_bRestoreLayout);
}


BEGIN_MESSAGE_MAP(CReloadDlg, CDialog)
END_MESSAGE_MAP()


BOOL CReloadDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString fmt,s;
	GetWindowText(fmt);
	s.Format(fmt,m_pTitle);
	SetWindowText(s);
	GetDlgItem(IDC_ST_DESC)->GetWindowTextA(fmt);
	s.Format(fmt,m_nDeleted);
	GetDlgItem(IDC_ST_DESC)->SetWindowTextA(s);

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
