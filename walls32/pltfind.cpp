// pltfind.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "pltfind.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPltFindDlg dialog

CTabComboHist hist_pltfind(6);
BOOL CPltFindDlg::m_bMatchCase=FALSE;

CPltFindDlg::CPltFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPltFindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPltFindDlg)
	//}}AFX_DATA_INIT
	m_Name=hist_pltfind.GetFirstString();
	m_bUseCase=m_bMatchCase;
}

static BOOL IsToken(LPCSTR p)
{
	while(*p) if(isspace((BYTE)*p) || *p++==',') return FALSE;
	return TRUE;
}

void CPltFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPltFindDlg)
	DDX_Text(pDX, IDC_FINDSTATION, m_Name);
	DDV_MaxChars(pDX, m_Name, 40);
	DDX_Check(pDX, IDC_MATCHCASE, m_bUseCase);
	//}}AFX_DATA_MAP
	
	if(pDX->m_bSaveAndValidate) {
		m_Name.Trim();
		if(!IsToken(m_Name)) {
			AfxMessageBox(IDS_ERR_NOTSTANAME);
			goto _fail;
		}

		int e=parse_name(m_Name);
		if(e) {
			AfxMessageBox(e);
			goto _fail;
		}

	    if(!CPrjDoc::FindStation(m_Name,m_bUseCase,0)) goto _fail;
		hist_pltfind.InsertAsFirst(m_Name);
		m_bMatchCase=m_bUseCase;
	}
	return;

_fail:
	pDX->m_idLastControl=IDC_FINDSTATION;
	//***7.1 pDX->m_hWndLastControl=GetDlgItem(IDC_FINDSTATION)->m_hWnd;
	pDX->m_bEditLastControl=TRUE;
	pDX->Fail();
}

BEGIN_MESSAGE_MAP(CPltFindDlg, CDialog)
	//{{AFX_MSG_MAP(CPltFindDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPltFindDlg message handlers

BOOL CPltFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	VERIFY(m_ce.SubclassDlgItem(IDC_FINDSTATION,this));
	m_ce.LoadHistory(&hist_pltfind);
	m_ce.SetEditSel(0,-1);
	m_ce.SetFocus();
	return FALSE;  // return TRUE  unless you set the focus to a control
}
