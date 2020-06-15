// griddlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "compview.h"
#include "plotview.h" 
#include "griddlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGridDlg dialog

static char *GetFl(double f)
{
	return GetFloatStr(LEN_SCALE(f), 4);
}

static BOOL CheckFl(const char *buf, double *pf, BOOL bNeg)
{
	double bFeet = LEN_SCALE(*pf);

	if (!CheckFlt(buf, &bFeet, bNeg ? -1000000.0 : 1.0, 1000000.0, 4)) return FALSE;
	*pf = LEN_INVSCALE(bFeet);
	return TRUE;
}

CGridDlg::CGridDlg(GRIDFORMAT *pGF, const char *pNameOfOrigin, CWnd* pParent /*=NULL*/)
	: CDialog(CGridDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGridDlg)
	//}}AFX_DATA_INIT

	m_pGF = pGF;
	m_pNameOfOrigin = pNameOfOrigin;
	m_GridNorth = GetFloatStr(pGF->fGridNorth, 4);
	m_EastOffset = GetFl(pGF->fEastOffset);
	m_NorthOffset = GetFl(pGF->fNorthOffset);
	m_VertOffset = GetFl(pGF->fVertOffset);
	m_HorzOffset = GetFl(pGF->fHorzOffset);
	m_TickInterval = GetFl(pGF->fTickInterval);
	m_EastInterval = GetFl(pGF->fEastInterval);
	m_NorthEastRatio = GetFloatStr(pGF->fNorthEastRatio, 4);
	m_VertInterval = GetFl(pGF->fVertInterval);
	m_HorzVertRatio = GetFloatStr(pGF->fHorzVertRatio, 4);
}

UINT CGridDlg::CheckData()
{
	GRIDFORMAT gf = *m_pGF;

	if (!CheckFl(m_EastOffset, &gf.fEastOffset, TRUE)) return IDC_EASTOFFSET;
	if (!CheckFl(m_NorthOffset, &gf.fNorthOffset, TRUE)) return IDC_NORTHOFFSET;
	if (!CheckFl(m_VertOffset, &gf.fVertOffset, TRUE)) return IDC_VERTOFFSET;
	if (!CheckFl(m_HorzOffset, &gf.fHorzOffset, TRUE)) return IDC_HORZOFFSET;

	if (!CheckFl(m_TickInterval, &gf.fTickInterval, FALSE)) return IDC_TICKINTERVAL;
	if (!CheckFl(m_EastInterval, &gf.fEastInterval, FALSE)) return IDC_EASTINTERVAL;
	if (!CheckFl(m_VertInterval, &gf.fVertInterval, FALSE)) return IDC_VERTINTERVAL;

	if (!CheckFlt(m_GridNorth, &gf.fGridNorth, -180.0, 180.0, 4)) return IDC_GRIDNORTH;
	if (!CheckFlt(m_NorthEastRatio, &gf.fNorthEastRatio, 0.0, 1000.0, 4)) return IDC_NORTHEASTRATIO;
	if (!CheckFlt(m_HorzVertRatio, &gf.fHorzVertRatio, 0.0, 1000.0, 4)) return IDC_HORZVERTRATIO;

	gf.wUseTickmarks = GetCheck(IDC_USETICKMARKS);

	if (memcmp(&gf, m_pGF, sizeof(GRIDFORMAT))) {
		m_bChanged = TRUE;
		*m_pGF = gf;
	}
	else m_bChanged = FALSE;

	return 0;
}

void CGridDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGridDlg)
	DDX_Text(pDX, IDC_TICKINTERVAL, m_TickInterval);
	DDV_MaxChars(pDX, m_TickInterval, 10);
	DDX_Text(pDX, IDC_EASTINTERVAL, m_EastInterval);
	DDV_MaxChars(pDX, m_EastInterval, 10);
	DDX_Text(pDX, IDC_NORTHEASTRATIO, m_NorthEastRatio);
	DDV_MaxChars(pDX, m_NorthEastRatio, 10);
	DDX_Text(pDX, IDC_VERTINTERVAL, m_VertInterval);
	DDV_MaxChars(pDX, m_VertInterval, 10);
	DDX_Text(pDX, IDC_HORZVERTRATIO, m_HorzVertRatio);
	DDV_MaxChars(pDX, m_HorzVertRatio, 10);
	DDX_Text(pDX, IDC_EASTOFFSET, m_EastOffset);
	DDV_MaxChars(pDX, m_EastOffset, 10);
	DDX_Text(pDX, IDC_HORZOFFSET, m_HorzOffset);
	DDV_MaxChars(pDX, m_HorzOffset, 10);
	DDX_Text(pDX, IDC_NORTHOFFSET, m_NorthOffset);
	DDV_MaxChars(pDX, m_NorthOffset, 10);
	DDX_Text(pDX, IDC_VERTOFFSET, m_VertOffset);
	DDV_MaxChars(pDX, m_VertOffset, 10);
	DDX_Text(pDX, IDC_GRIDNORTH, m_GridNorth);
	DDV_MaxChars(pDX, m_GridNorth, 10);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate) {
		UINT id = CheckData();
		if (id) {
			pDX->m_idLastControl = id;
			//***7.1 pDX->m_hWndLastControl=GetDlgItem(id)->m_hWnd;
			pDX->m_bEditLastControl = TRUE;
			pDX->Fail();
		}
	}
}

IMPLEMENT_DYNAMIC(CGridDlg, CDialog)

BEGIN_MESSAGE_MAP(CGridDlg, CDialog)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//{{AFX_MSG_MAP(CGridDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGridDlg message handlers

void CGridDlg::FeetTitle(UINT id)
{
	CWnd *pWnd = GetDlgItem(id);
	char buf[40];
	pWnd->GetWindowText(buf, 39);
	char *p = strchr(buf, '(');
	if (p) {
		strcpy(p + 1, "ft):");
		pWnd->SetWindowText(buf);
	}
}

BOOL CGridDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetCheck(IDC_USETICKMARKS, m_pGF->wUseTickmarks);
	if (m_pNameOfOrigin) GetDlgItem(IDC_ST_GRIDORIGIN)->SetWindowText(m_pNameOfOrigin);

	if (LEN_ISFEET()) {
		FeetTitle(IDC_ST_EASTOFFSET);
		FeetTitle(IDC_ST_NORTHOFFSET);
		FeetTitle(IDC_ST_VERTOFFSET);
		FeetTitle(IDC_ST_HORZOFFSET);

		FeetTitle(IDC_ST_TICKINTERVAL);
		FeetTitle(IDC_ST_EASTINTERVAL);
		FeetTitle(IDC_ST_VERTINTERVAL);
	}

	CenterWindow();
	//GetDlgItem(IDC_FRAMEWIDTH)->SetFocus();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

LRESULT CGridDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(9, HELP_CONTEXT);
	return TRUE;
}
