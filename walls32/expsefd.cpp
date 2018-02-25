// expsefd.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "expsefd.h"
#include "expsef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExpSefDlg dialog

BOOL CExpSefDlg::m_bNoConvertPfx=FALSE;
BOOL CExpSefDlg::m_bUseVdist=FALSE;
BOOL CExpSefDlg::m_bUseAllSef=TRUE;

CExpSefDlg::CExpSefDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExpSefDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExpSefDlg)
	//}}AFX_DATA_INIT
}


void CExpSefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHNAME, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 250);
	DDX_Check(pDX, IDC_NOCVTPFX, m_bNoConvertPfx);
	DDX_Check(pDX, IDC_USE_VDIST, m_bUseVdist);
	DDX_Check(pDX, IDC_USE_ALLSEF, m_bUseAllSef);

	ASSERT(!m_bUseVdist || m_bUseAllSef);

	if(pDX->m_bSaveAndValidate) {
		//Now let's see if the directory needs to be created --
	  if(CheckDirectory(m_pathname)) {
			pDX->m_idLastControl=IDC_PATHNAME;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
	  }
	  	  
	  m_pDoc->ExportSEF(m_pNode,m_pathname,EXP_NOCVTPFX*m_bNoConvertPfx+EXP_USE_VDIST*m_bUseVdist+EXP_USE_ALLSEF*m_bUseAllSef);
	}
}


BEGIN_MESSAGE_MAP(CExpSefDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_USE_ALLSEF, &CExpSefDlg::OnBnClickedUseAllsef)
	ON_BN_CLICKED(IDC_USE_VDIST, &CExpSefDlg::OnBnClickedUseVdist)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpSefDlg message handlers

void CExpSefDlg::OnBrowse() 
{
	BrowseFiles((CEdit *)GetDlgItem(IDC_PATHNAME),m_pathname,".sef",IDS_SEF_FILES,IDS_SEF_BROWSE);
}

BOOL CExpSefDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetWndTitle(this,0,m_pNode->Title());
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CExpSefDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(128,HELP_CONTEXT);
	return TRUE;
}


void CExpSefDlg::OnBnClickedUseAllsef()
{
	m_bUseAllSef=!m_bUseAllSef;
	if(!m_bUseAllSef && m_bUseVdist) {
		((CButton *)GetDlgItem(IDC_USE_VDIST))->SetCheck(m_bUseVdist=FALSE);
	}
}

void CExpSefDlg::OnBnClickedUseVdist()
{
	m_bUseVdist=!m_bUseVdist;
	if(m_bUseVdist && !m_bUseAllSef) {
		((CButton *)GetDlgItem(IDC_USE_ALLSEF))->SetCheck(m_bUseAllSef=TRUE);
	}
}
