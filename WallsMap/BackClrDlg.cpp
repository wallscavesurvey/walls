// BackClrDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include ".\backclrdlg.h"


// CBackClrDlg dialog

IMPLEMENT_DYNAMIC(CBackClrDlg, CDialog)

CBackClrDlg::CBackClrDlg(LPCSTR pTitle, COLORREF cr, CWnd* pParent) :
	m_pTitle(pTitle), m_cr(cr), CDialog(CBackClrDlg::IDD, pParent) {}

CBackClrDlg::~CBackClrDlg()
{
}

void CBackClrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BKCOLOR, m_BtnBkg);
}


BEGIN_MESSAGE_MAP(CBackClrDlg, CDialog)
	ON_MESSAGE(CPN_SELCHANGE, OnChgColor)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()


// CBackClrDlg message handlers

BOOL CBackClrDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(m_pTitle);
	m_BtnBkg.SetColor(m_cr);
	m_ToolTips.Create(this);
	m_BtnBkg.AddToolTip(&m_ToolTips);

	return TRUE;  // return TRUE unless you set the focus to a control
}

LRESULT CBackClrDlg::OnChgColor(WPARAM clr, LPARAM id)
{
	m_cr = clr;
	return TRUE;
}

LRESULT CBackClrDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}


