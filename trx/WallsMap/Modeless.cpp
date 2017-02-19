// Modeless.cpp : implementation file
//

#include "stdafx.h"

CModeless * CModeless::m_pModeless=NULL;

IMPLEMENT_DYNAMIC(CModeless, CDialog)

CModeless::CModeless(LPCSTR title,LPCSTR msg,UINT delay) : CDialog(CModeless::IDD, NULL)
{
	ASSERT(!m_pModeless);
	m_nTimer=0;
	m_bCloseOnTimeout=false;
	VERIFY(Create(CModeless::IDD,GetDesktopWindow()));
	SetWindowText(title);
	GetDlgItem(IDC_ST_MODELESS_MSG)->SetWindowText(msg);
	SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	ShowWindow(SW_SHOW);
	VERIFY(m_nTimer=SetTimer(1114,delay,NULL));
}

CModeless::~CModeless()
{
	m_pModeless=NULL;
}

BEGIN_MESSAGE_MAP(CModeless, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CModeless message handlers

void CModeless::PostNcDestroy()
{
	m_pModeless=NULL;
	if(m_nTimer) {
		VERIFY(KillTimer(m_nTimer));
		m_nTimer=0;
	}
	CDialog::PostNcDestroy();
	delete this;
}

void CModeless::OnTimer(UINT nIDEvent)
{
   if(nIDEvent==m_nTimer) {
	   VERIFY(KillTimer(m_nTimer));
	   m_nTimer=0;
	   if(m_bCloseOnTimeout && m_pModeless) {
		   DestroyWindow();
	   }
   }
}

void CModeless::CloseOnTimeout()
{
	if(!m_nTimer && m_pModeless) DestroyWindow();
	m_bCloseOnTimeout=true;
}

