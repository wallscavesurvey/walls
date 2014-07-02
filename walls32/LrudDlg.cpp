// LrudDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
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

CLrudDlg::CLrudDlg(CSegView *pSV,double *pfThickPenPts,CWnd* pParent /*=NULL*/)
	: CDialog(CLrudDlg::IDD, pParent)
{
	m_pfThickPenPts=pfThickPenPts;

	//{{AFX_DATA_INIT(CLrudDlg)
	m_iLrudStyle = pSV->LrudStyleIdx();
	m_bSVGUseIfAvail = theApp.m_bIsVersionNT && pSV->IsSVGUseIfAvail();
	m_ThickPenPts = GetFloatStr(*pfThickPenPts,2);
	//}}AFX_DATA_INIT
	m_iEnlarge=pSV->LrudEnlarge();
	m_LrudExag=GetFloatStr(fLrudExag,1);
	m_pSV=pSV;
}

void CLrudDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLrudDlg)
	DDX_Radio(pDX, IDC_LRUDSOLID, m_iLrudStyle);
	DDX_Check(pDX, IDC_SVG_USE, m_bSVGUseIfAvail);
	DDX_Text(pDX, IDC_THICKPENPTS, m_ThickPenPts);
	DDV_MaxChars(pDX, m_ThickPenPts, 10);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_LRUDEXAG, m_LrudExag);
	DDV_MaxChars(pDX, m_LrudExag, 6);

	if(pDX->m_bSaveAndValidate) {
		if(!CheckFlt(m_ThickPenPts,m_pfThickPenPts,0.0,20.0,2)) {
			pDX->m_idLastControl=IDC_THICKPENPTS;
			//***7.1 pDX->m_hWndLastControl=GetDlgItem(IDC_THICKPENPTS)->m_hWnd;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
		else {
			double d=fLrudExag;
			if(!CheckFlt(m_LrudExag,&d,0.0,50.0,0)) {
				pDX->m_idLastControl=IDC_LRUDEXAG;
				pDX->m_bEditLastControl=TRUE;
				pDX->Fail();
				return;
			}
			fLrudExag=(float)d;
			m_iEnlarge=((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_ENLARGE))->GetPos();
		}
	}
}

BEGIN_MESSAGE_MAP(CLrudDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CLrudDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLrudDlg message handlers

BOOL CLrudDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CSpinButtonCtrl *sp=(CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_ENLARGE);
    sp->SetRange(0,10);
	sp->SetPos(m_iEnlarge);

	CString status;

	status.Format(IDS_LRUD_DLGTITLE,CPrjDoc::m_pReviewNode->Title());
	SetWindowText(status);

	if(theApp.m_bIsVersionNT)
		status.Format(IDS_SVG_STATUS,
			CPrjDoc::m_pReviewNode->IsProcessingSVG()?"ENABLED":"DISABLED",
			m_pSV->IsSVGAvail()?"AVAILABLE":"UNAVAILABLE");
	else {
		GetDlgItem(IDC_SVG_USE)->EnableWindow(FALSE);
		status.Format(IDS_SVG_UNAVAILABLE);
	}

	GetDlgItem(IDC_ST_SVG_STATUS)->SetWindowText(status);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CLrudDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(144);
	return TRUE;
}
