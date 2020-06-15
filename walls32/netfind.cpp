// netfind.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "netfind.h"
#include "wall_srv.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CTabComboHist hist_netfind(6);

BOOL CNetFindDlg::m_bMatchCase = 0;

/////////////////////////////////////////////////////////////////////////////
// CNetFindDlg dialog


CNetFindDlg::CNetFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNetFindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNetFindDlg)
	m_Names = hist_netfind.GetFirstString();
	//}}AFX_DATA_INIT
	m_bUseCase = m_bMatchCase;
}

void CNetFindDlg::DoDataExchange(CDataExchange* pDX)
{
	char names[SIZ_EDIT2NAMEBUF]; //64

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetFindDlg)
	DDX_Text(pDX, IDC_FINDSTATION, m_Names);
	DDV_MaxChars(pDX, m_Names, 60);
	DDX_Check(pDX, IDC_MATCHCASE, m_bUseCase);
	//}}AFX_DATA_MAP 

	if (pDX->m_bSaveAndValidate) {
		m_Names.TrimLeft();
		m_Names.TrimRight();
		if (parse_2names(strcpy(names, m_Names), TRUE) ||
			!CPrjDoc::FindVector(m_bUseCase, 0)) goto _fail;
		hist_netfind.InsertAsFirst(m_Names);
		m_bMatchCase = m_bUseCase;
		return;
	_fail:
		pDX->m_idLastControl = IDC_FINDSTATION;
		//***7.1 pDX->m_hWndLastControl=GetDlgItem(IDC_FINDSTATION)->m_hWnd;
		pDX->m_bEditLastControl = TRUE;
		pDX->Fail();
	}
}

BEGIN_MESSAGE_MAP(CNetFindDlg, CDialog)
	//{{AFX_MSG_MAP(CNetFindDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNetFindDlg message handlers

BOOL CNetFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	VERIFY(m_ce.SubclassDlgItem(IDC_FINDSTATION, this));
	m_ce.LoadHistory(&hist_netfind);
	m_ce.SetEditSel(0, -1);
	m_ce.SetFocus();
	return FALSE;
}
