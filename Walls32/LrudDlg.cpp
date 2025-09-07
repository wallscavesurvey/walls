// LrudDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "segview.h"
#include "LrudDlg.h"
#ifndef __AFXCMN_H__
#include <afxcmn.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLrudDlg dialog

extern float fLrudExag;

CLrudDlg::CLrudDlg(CPrjDoc *pDoc, CSegView *pSV, CWnd* pParent /*=NULL*/)
	: CDialog(CLrudDlg::IDD, pParent), m_pDoc(pDoc), m_pSV(pSV)
{
	m_fThickPenPts = pSV->m_fThickPenPts;
	m_iLrudStyle = pSV->LrudStyleIdx();
	m_bSVGUseIfAvail = pSV->IsSVGUseIfAvail();
	m_ThickPenPts = GetFloatStr(m_fThickPenPts, 2);
	m_iEnlarge = pSV->LrudEnlarge();
	m_LrudExag = GetFloatStr(fLrudExag, 1);
	m_bMapsActive = 0;
}

void CLrudDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_LRUDSOLID, m_iLrudStyle);
	DDX_Check(pDX, IDC_SVG_USE, m_bSVGUseIfAvail);
	DDX_Text(pDX, IDC_THICKPENPTS, m_ThickPenPts);
	DDV_MaxChars(pDX, m_ThickPenPts, 10);
	DDX_Text(pDX, IDC_LRUDEXAG, m_LrudExag);
	DDV_MaxChars(pDX, m_LrudExag, 6);

	if (pDX->m_bSaveAndValidate) {
		bool bRefreshMaps = false;
		if (!CheckFlt(m_ThickPenPts, &m_fThickPenPts, 0.0, 20.0, 2)) {
			pDX->m_idLastControl = IDC_THICKPENPTS;
			pDX->m_bEditLastControl = TRUE;
			pDX->Fail();
		}
		else {
			double d = fLrudExag;
			if (!CheckFlt(m_LrudExag, &d, 0.0, 50.0, 0)) {
				pDX->m_idLastControl = IDC_LRUDEXAG;
				pDX->m_bEditLastControl = TRUE;
				pDX->Fail();
				return;
			}
			if (fLrudExag != (float)d) {
				fLrudExag = (float)d;
				bRefreshMaps = true;
			}
			m_iEnlarge = ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_ENLARGE))->GetPos();
		}

		if (CSegView::m_fThickPenPts != m_fThickPenPts) {
			AfxGetApp()->WriteProfileString(CPlotView::szMapFormat, CSegView::szThickPenPts, m_ThickPenPts);
			CSegView::m_fThickPenPts = m_fThickPenPts;
			bRefreshMaps = true;
		}

		if (m_iLrudStyle != m_pSV->LrudStyleIdx() || m_iEnlarge != m_pSV->LrudEnlarge() ||
			m_bSVGUseIfAvail != m_pSV->IsSVGUseIfAvail()) {
			m_pSV->SetLrudStyleIdx(m_iLrudStyle);
			m_pSV->SetLrudEnlarge(m_iEnlarge);
			m_pSV->SetSVGUseIfAvail(m_bSVGUseIfAvail);
			bRefreshMaps = true;
			CSegView::m_bChanged = TRUE;
		}
		if (m_bMapsActive && bRefreshMaps) {
			m_pDoc->UpdateMapViews(FALSE, M_LRUDS);
		}
	}
}

BEGIN_MESSAGE_MAP(CLrudDlg, CDialog)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_BN_CLICKED(IDC_SVG_USE, OnBnClickedSVGUse)
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_LRUDSOLID, IDC_LRUDBOTH, OnBnClickedLRUD)
	ON_EN_CHANGE(IDC_EDIT_ENLARGE, OnEditChgEnlarge)
	ON_EN_CHANGE(IDC_LRUDEXAG, OnEditChgExag)
	ON_EN_CHANGE(IDC_THICKPENPTS, OnEditThickPenPts)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLrudDlg message handlers

BOOL CLrudDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CSpinButtonCtrl *sp = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_ENLARGE);
	sp->SetRange(0, 10);
	sp->SetPos(m_iEnlarge);

	CString status;

	status.Format(IDS_LRUD_DLGTITLE, CPrjDoc::m_pReviewNode->Title());
	SetWindowText(status);

	status.Format(IDS_SVG_STATUS,
		CPrjDoc::m_pReviewNode->IsProcessingSVG() ? "ENABLED" : "DISABLED",
		m_pSV->IsSVGAvail() ? "AVAILABLE" : "UNAVAILABLE");

	GetDlgItem(IDC_ST_SVG_STATUS)->SetWindowText(status);

	m_bMapsActive = m_pDoc->UpdateMapViews(TRUE, M_LRUDS) != 0;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CLrudDlg::HasChanged()
{
	return m_iLrudStyle != m_pSV->LrudStyleIdx() || m_iEnlarge != m_pSV->LrudEnlarge() ||
		m_bSVGUseIfAvail != m_pSV->IsSVGUseIfAvail() || CSegView::m_fThickPenPts != m_fThickPenPts ||
		atof(m_LrudExag) != fLrudExag;
}

void CLrudDlg::OnBnClickedLRUD(UINT id)
{
	if (m_bMapsActive && id >= IDC_LRUDSOLID && id <= IDC_LRUDSOLID + 2) {
		m_iLrudStyle = id - IDC_LRUDSOLID;
		EnableApply(HasChanged());
	}
}

void CLrudDlg::OnBnClickedSVGUse()
{
	if (m_bMapsActive && m_pSV->IsSVGAvail()) {
		m_bSVGUseIfAvail = ((CButton *)GetDlgItem(IDC_SVG_USE))->GetCheck();
		EnableApply(HasChanged());
	}
}

void CLrudDlg::OnEditThickPenPts()
{
	if (m_bMapsActive && m_pSV->IsSVGAvail()) {
		GetDlgItem(IDC_THICKPENPTS)->GetWindowText(m_ThickPenPts);
		m_fThickPenPts = atof(m_ThickPenPts);
		EnableApply(HasChanged());
	}
}

void CLrudDlg::OnEditChgEnlarge()
{
	if (m_bMapsActive) {
		m_iEnlarge = ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_ENLARGE))->GetPos();
		EnableApply(HasChanged());
	}
}

void CLrudDlg::OnEditChgExag()
{
	if (m_bMapsActive) {
		GetDlgItem(IDC_LRUDEXAG)->GetWindowText(m_LrudExag);
		EnableApply(HasChanged());
	}
}

void CLrudDlg::OnBnClickedApply()
{
	ASSERT(m_bMapsActive);
	if (UpdateData()) {
		EnableApply(0);
	}
}

LRESULT CLrudDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(144);
	return TRUE;
}
