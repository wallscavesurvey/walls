// ResizeFrmDlg.cpp : implementation file
//
#include "stdafx.h"
#include "Walls.h"
#include "plotview.h"
#include "ResizeFrmDlg.h"

// CResizeFrmDlg dialog

IMPLEMENT_DYNAMIC(CResizeFrmDlg, CDialog)
CResizeFrmDlg::CResizeFrmDlg(double *pWidth,int px_per_in,CWnd* pParent /*=NULL*/)
	: CDialog(CResizeFrmDlg::IDD, pParent)
	, m_pWidth(pWidth)
	, m_iPx_per_in(-px_per_in)
	, m_bSave(FALSE)
	, m_InchWidth(GetFloatStr(*pWidth,3))
{
}

CResizeFrmDlg::~CResizeFrmDlg()
{
}

void CResizeFrmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SAVEASDEFAULT, m_bSave);
	DDX_Text(pDX, IDC_INCHWIDTH, m_InchWidth);
	DDV_MaxChars(pDX, m_InchWidth, 6);
	if(pDX->m_bSaveAndValidate) {
		if(!CheckFlt(m_InchWidth,m_pWidth,MIN_SCREEN_FRAMEWIDTH,MAX_SCREEN_FRAMEWIDTH,3)) {
			pDX->m_idLastControl=IDC_INCHWIDTH;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
	}
}

BEGIN_MESSAGE_MAP(CResizeFrmDlg, CDialog)
	ON_EN_CHANGE(IDC_INCHWIDTH, OnEnChangeInchwidth)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_INCHES, OnDeltaposSpinInches)
END_MESSAGE_MAP()

// CResizeFrmDlg message handlers

BOOL CResizeFrmDlg::OnInitDialog()
{
	((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_INCHES))->SetRange(MIN_SCREEN_FRAMEWIDTH,MAX_SCREEN_FRAMEWIDTH);
	CDialog::OnInitDialog();
	m_iPx_per_in=-m_iPx_per_in; //make positive
	OnEnChangeInchwidth();

	CEdit *ce=(CEdit *)GetDlgItem(IDC_INCHWIDTH);
	ce->SetSel(0,-1);
	ce->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CResizeFrmDlg::UpdatePixels(char *buf,double f)
{
	if(f>0.0) _snprintf(buf,32,"Pixels: %d",(int)(f*m_iPx_per_in));
	else *buf=0;
	GetDlgItem(IDC_ST_PIXELWIDTH)->SetWindowText(buf);
}

void CResizeFrmDlg::OnEnChangeInchwidth()
{
	double f;
	char buf[32];
	if(m_iPx_per_in<0) return;
	GetDlgItem(IDC_INCHWIDTH)->GetWindowText(buf,32);
	if(IsNumeric(buf) && (f=atof(buf))>=MIN_SCREEN_FRAMEWIDTH && f<=MAX_SCREEN_FRAMEWIDTH) {
		((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_INCHES))->SetPos((int)f);
	}
	else f=0.0;
	UpdatePixels(buf,f);
}

void CResizeFrmDlg::OnDeltaposSpinInches(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	char buf[32];
	// TODO: Add your control notification handler code here
	int iPos=pNMUpDown->iPos+pNMUpDown->iDelta;
	if(iPos>=MIN_SCREEN_FRAMEWIDTH && iPos<=MAX_SCREEN_FRAMEWIDTH) {
		m_iPx_per_in=-m_iPx_per_in;
		GetDlgItem(IDC_INCHWIDTH)->SetWindowText(GetIntStr(iPos));
		m_iPx_per_in=-m_iPx_per_in;
		UpdatePixels(buf,(double)iPos);
	}
	*pResult = 0;
}
