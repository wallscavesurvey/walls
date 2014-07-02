// dupdlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "dupdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CDupDlg, CDialog)
	//{{AFX_MSG_MAP(CDupDlg)
	ON_BN_CLICKED(ID_ABORT, OnAbort)
	ON_BN_CLICKED(ID_DUPEDIT, OnDupEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDupDlg dialog

CDupDlg::CDupDlg(DUPLOC *pLoc1,DUPLOC *pLoc2,CWnd* pParent /*=NULL*/)
	: CDialog(CDupDlg::IDD, pParent)
{
	m_pLoc1=pLoc1;
	m_pLoc2=pLoc2;
}

BOOL CDupDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWndTitle(this,0,m_pLoc1->pName,m_pLoc2->pName);
	
	char title[128];
	_snprintf(title,128,"%s : %u",m_pLoc1->File,m_pLoc1->nLine);
    GetDlgItem(IDC_DUPNAME1)->SetWindowText(title);
	_snprintf(title,128,"%s : %u",m_pLoc2->File,m_pLoc2->nLine);
    GetDlgItem(IDC_DUPNAME2)->SetWindowText(title);
    CenterWindow();
    ::MessageBeep(MB_ICONEXCLAMATION);
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CDupDlg message handlers

void CDupDlg::OnAbort()
{
	EndDialog(IDABORT);	
}

void CDupDlg::OnDupEdit()
{
	EndDialog(IDABORT+1);	
}
