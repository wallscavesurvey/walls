// aboutdlg.cpp : implementation file
//

#include "stdafx.h"
#include <dos.h>
#include "walls.h"
#include "aboutdlg.h"
#include <direct.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUpLnkDlg dialog

CUpLnkDlg::CUpLnkDlg(LPCSTR pURL,CWnd* pParent /*=NULL*/)
	: CDialog(CUpLnkDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUpLnkDlg)
	m_NewURL = pURL;
	//}}AFX_DATA_INIT
}


void CUpLnkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUpLnkDlg)
	DDX_Text(pDX, IDC_NEWURL, m_NewURL);
	DDV_MaxChars(pDX, m_NewURL, 250);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUpLnkDlg, CDialog)
	//{{AFX_MSG_MAP(CUpLnkDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpLnkDlg message handlers

BOOL CUpLnkDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	CEdit *ce=(CEdit *)GetDlgItem(IDC_NEWURL);
	ce->SetSel(0,-1);
	ce->SetFocus();
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_BN_CLICKED(IDC_HELPCONTENTS, &CAboutDlg::OnBnClickedHelp)
	ON_BN_CLICKED(IDC_UPDATELINK, OnUpdateLink)
	ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAboutDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAboutDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog()
{
	m_textLink.SubclassDlgItem(IDC_WALLSURL,this);
	CDialog::OnInitDialog();
	CString url=AfxGetApp()->GetProfileString("Tip","Url",NULL);
	if(!url.IsEmpty()) {
		if(!strstr(url,"davidmck") &&
			!strstr(url,".txspeleologicalsurvey") &&
		  !strstr(url,"/tnhc/")) m_textLink.SetLink(url);
	}
	CenterWindow();
	GetDlgItem(IDOK)->SetFocus();
	return FALSE;
	//return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAboutDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	EndDialog(IDOK);
}

void CAboutDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	EndDialog(IDOK);
}

void CAboutDlg::OnUpdateLink() 
{
	CUpLnkDlg dlg(m_textLink.GetLink());

	if(IDOK==dlg.DoModal()) {
		m_textLink.SetLink(dlg.m_NewURL);
		VERIFY(AfxGetApp()->WriteProfileString("Tip","Url",m_textLink.GetLink()));
	}
}

void CAboutDlg::OnBnClickedHelp()
{
	AfxGetApp()->WinHelp(1000);
	EndDialog(IDOK);
}


LRESULT CAboutDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	OnBnClickedHelp();
	return TRUE;
}

