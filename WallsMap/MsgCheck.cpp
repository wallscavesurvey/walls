// MsgCheck.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MsgCheck.h"


// CMsgCheck dialog

IMPLEMENT_DYNAMIC(CMsgCheck, CDialog)

CMsgCheck::CMsgCheck(UINT idd, LPCSTR pMsg, LPCSTR pTitle /*=NULL*/, LPCSTR pPromptMsg /*=NULL*/, CWnd* pParent /*=NULL*/)
	: CDialog(idd, pParent)
	, m_bNotAgain(FALSE)
	, m_pTitle(pTitle)
	, m_pMsg(pMsg)
	, m_pPromptMsg(pPromptMsg)
{

}

CMsgCheck::~CMsgCheck()
{
}

void CMsgCheck::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHK_NOTAGAIN, m_bNotAgain);
}


BEGIN_MESSAGE_MAP(CMsgCheck, CDialog)
END_MESSAGE_MAP()


// CMsgCheck message handlers

BOOL CMsgCheck::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (m_pTitle) SetWindowText(m_pTitle);

	GetDlgItem(IDC_ST_MESSAGE)->SetWindowText(m_pMsg);

	if (m_pPromptMsg) {
		if (*m_pPromptMsg == 0) GetDlgItem(IDC_CHK_NOTAGAIN)->ShowWindow(SW_HIDE);
		else GetDlgItem(IDC_CHK_NOTAGAIN)->SetWindowText(m_pPromptMsg);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
}
