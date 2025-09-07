// PageOffDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "seghier.h"
#include "PageOffDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPageOffDlg dialog

void CPageOffDlg::GetCenterOffsets()
{
	m_LeftOffset = GetFloatStr(m_fCenterLeft, 3);
	m_TopOffset = GetFloatStr(m_fCenterTop, 3);
}

CPageOffDlg::CPageOffDlg(VIEWFORMAT *pVF, double fPW, double fPH, CWnd* pParent /*=NULL*/)
	: CDialog(CPageOffDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPageOffDlg)
	//}}AFX_DATA_INIT

	double w = VF_ISINCHES(pVF->bFlags) ? 1.0 : 2.54;

	m_pVF = pVF;
	m_bInsideFrame = VF_ISINSIDE(pVF->bFlags);
	m_iLegendPos = VF_LEGPOS(pVF->bFlags);
	m_fCenterLeft = (fPW - pVF->fFrameWidth)*w / 2.0;
	m_fCenterTop = (fPH - pVF->fFrameHeight)*w / 2.0;
	if (m_bCenterOfPage = (pVF->fPageOffsetX == FLT_MIN)) GetCenterOffsets();
	else {
		m_LeftOffset = GetFloatStr(pVF->fPageOffsetX*w, 3);
		m_TopOffset = GetFloatStr(pVF->fPageOffsetY*w, 3);
	}
	m_Overlap = GetFloatStr(pVF->fOverlap*w, 3);
}

UINT CPageOffDlg::CheckData()
{
	VIEWFORMAT vf = *m_pVF;

	double w = VF_ISINCHES(vf.bFlags) ? 1.0 : 2.54;

	double f = vf.fOverlap*w;
	if (!CheckFlt(m_Overlap, &f, 0.0, 100.0, 3)) return IDC_OVERLAP;
	if (f != vf.fOverlap*w) vf.fOverlap = (float)(f / w);

	if (m_bCenterOfPage) {
		vf.fPageOffsetX = vf.fPageOffsetY = FLT_MIN;
	}
	else {
		if (vf.fPageOffsetX != FLT_MIN) {
			m_fCenterLeft = vf.fPageOffsetX*w;
			m_fCenterTop = vf.fPageOffsetY*w;
		}
		if (!CheckFlt(m_LeftOffset, &m_fCenterLeft, -100.0, 100.0, 3)) return IDC_LEFTOFFSET;
		if (!CheckFlt(m_TopOffset, &m_fCenterTop, -100.0, 100.0, 3)) return IDC_TOPOFFSET;
		vf.fPageOffsetX = (float)(m_fCenterLeft / w);
		vf.fPageOffsetY = (float)(m_fCenterTop / w);
	}

	vf.bFlags &= ~VF_LEGMASK;
	vf.bFlags |= (m_iLegendPos << 2);
	vf.bFlags &= ~VF_INSIDEFRAME;
	vf.bFlags |= (m_bInsideFrame << 1);

	if (memcmp(&vf, m_pVF, sizeof(VIEWFORMAT))) {
		vf.bChanged = TRUE;
		*m_pVF = vf;
	}
	return 0;
}


void CPageOffDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageOffDlg)
	DDX_Check(pDX, IDC_CENTEROFPAGE, m_bCenterOfPage);
	DDX_Text(pDX, IDC_LEFTOFFSET, m_LeftOffset);
	DDV_MaxChars(pDX, m_LeftOffset, 20);
	DDX_Text(pDX, IDC_TOPOFFSET, m_TopOffset);
	DDV_MaxChars(pDX, m_TopOffset, 20);
	DDX_Text(pDX, IDC_OVERLAP, m_Overlap);
	DDV_MaxChars(pDX, m_Overlap, 20);
	DDX_Check(pDX, IDC_INSIDEFRAME, m_bInsideFrame);
	DDX_CBIndex(pDX, IDC_LEGENDPOS, m_iLegendPos);
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

BEGIN_MESSAGE_MAP(CPageOffDlg, CDialog)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//{{AFX_MSG_MAP(CPageOffDlg)
	ON_BN_CLICKED(IDC_CENTEROFPAGE, OnCenterOfPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageOffDlg message handlers

BOOL CPageOffDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (VF_ISINCHES(m_pVF->bFlags)) {
		char *p = "in";
		GetDlgItem(IDC_ST_CM1)->SetWindowText(p);
		GetDlgItem(IDC_ST_CM2)->SetWindowText(p);
		GetDlgItem(IDC_ST_CM3)->SetWindowText(p);
	}

	if (m_bCenterOfPage) {
		Enable(IDC_LEFTOFFSET, FALSE);
		Enable(IDC_TOPOFFSET, FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CPageOffDlg::OnCenterOfPage()
{
	m_bCenterOfPage = IsDlgButtonChecked(IDC_CENTEROFPAGE);
	if (m_bCenterOfPage) {
		GetCenterOffsets();
		SetText(IDC_LEFTOFFSET, m_LeftOffset);
		SetText(IDC_TOPOFFSET, m_TopOffset);
	}
	Enable(IDC_LEFTOFFSET, !m_bCenterOfPage);
	Enable(IDC_TOPOFFSET, !m_bCenterOfPage);
	if (!m_bCenterOfPage) GetDlgItem(IDC_LEFTOFFSET)->SetFocus();
}

LRESULT CPageOffDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(141);
	return TRUE;
}


