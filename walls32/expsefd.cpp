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

BOOL CExpSefDlg::m_bIncludeDet=FALSE;
BOOL CExpSefDlg::m_bConvertPfx=FALSE;
BOOL CExpSefDlg::m_bFixedCols=FALSE;
BOOL CExpSefDlg::m_bForceFeet=FALSE;

CExpSefDlg::CExpSefDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExpSefDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExpSefDlg)
	//}}AFX_DATA_INIT
}


void CExpSefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpSefDlg)
	DDX_Text(pDX, IDC_PATHNAME, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 250);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_INCLUDEDET, m_bIncludeDet);
	DDX_Check(pDX, IDC_CONVERTPFX, m_bConvertPfx);
	DDX_Check(pDX, IDC_FORCEFEET, m_bForceFeet);
	DDX_Check(pDX, IDC_FIXEDCOLS, m_bFixedCols);

	if(pDX->m_bSaveAndValidate) {
		//Now let's see if the directory needs to be created --
	  if(CheckDirectory(m_pathname)) {
			pDX->m_idLastControl=IDC_PATHNAME;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
	  }
	  	  
	  m_pDoc->ExportSEF(this,m_pNode,m_pathname,
		  EXP_INCDET*m_bIncludeDet+
		  EXP_CVTPFX*m_bConvertPfx+
		  EXP_FORCEFEET*m_bForceFeet+
		  EXP_FIXEDCOLS*m_bFixedCols
		  );
	}
}


BEGIN_MESSAGE_MAP(CExpSefDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CExpSefDlg)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	//}}AFX_MSG_MAP
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

