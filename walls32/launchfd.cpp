// launchfd.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "launchfd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLaunchFindDlg dialog


CLaunchFindDlg::CLaunchFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLaunchFindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLaunchFindDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CLaunchFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLaunchFindDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate) {
		CListBox *plb = (CListBox *)GetDlgItem(IDC_LAUNCHLB);
		int i = plb->GetCurSel();
		ASSERT(i != LB_ERR);
		if (i != LB_ERR) {
			char buf[_MAX_FNAME + 24];
			plb->GetText(i, buf);
			strcpy(trx_Stpnam(m_pathname), buf + 21);
		}
		else *m_pathname = 0;
	}
}


BEGIN_MESSAGE_MAP(CLaunchFindDlg, CDialog)
	//{{AFX_MSG_MAP(CLaunchFindDlg)
	ON_BN_CLICKED(ID_NOFILE, OnNoFile)
	ON_LBN_DBLCLK(IDC_LAUNCHLB, OnDblclkLaunchlb)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLaunchFindDlg message handlers

LPSTR CLaunchFindDlg::GetFileStr(LPSTR buf)
{
	struct tm *ptm = localtime(&m_ftime);

	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d  %s",
		ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec, m_fname);
	return buf;
}

BOOL CLaunchFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();


	CListBox *plb = (CListBox *)GetDlgItem(IDC_LAUNCHLB);
	char buf[_MAX_FNAME + 24];

	sprintf(buf, "Multiple %s files exist for %s", *(trx_Stpext(m_pathname) + 1) == 's' ? "SVG" : "VRML",
		m_strTitle);

	SetWindowText(buf);

	plb->AddString(GetFileStr(buf));
	do {
		strcpy(m_fname, m_file.name);
		m_ftime = m_file.time_write;
		plb->AddString(GetFileStr(buf));
	} while (!_findnext(m_hFile, &m_file));
	plb->SetCurSel(plb->GetCount() - 1);
	plb->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CLaunchFindDlg::OnNoFile()
{
	*m_pathname = 0;
	EndDialog(-1);
}

void CLaunchFindDlg::OnDblclkLaunchlb()
{
	OnOK();
}
