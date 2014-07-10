// SymDlg.cpp : implementation file
//

#include "stdafx.h"
#include "D2D-SymView.h"
#include "SymDlg.h"

// CSymDlg dialog

static int m_nSavedCX,m_nSavedCY,m_nSavedX,m_nSavedY;

IMPLEMENT_DYNAMIC(CSymDlg, CDialog)

CSymDlg::CSymDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSymDlg::IDD, pParent), m_pView((CD2DSymView *)pParent)
{
}

CSymDlg::~CSymDlg()
{
}

void CSymDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_SQUARES, m_iSymTyp);
	DDX_Check(pDX, IDC_DOT_CENTER, m_bDotCenter);
	DDX_Text(pDX, IDC_EDIT_SYM_SZ, m_iSymSize);
	DDV_MinMaxInt(pDX, m_iSymSize, 0, 100);
	DDX_Text(pDX, IDC_EDIT_LINE_SZ, m_fLineSize);
	DDV_MinMaxDouble(pDX, m_fLineSize, 0.0, 10.0);
	DDX_Text(pDX, IDC_EDIT_VEC_SZ, m_fVecSize);
	DDV_MinMaxDouble(pDX, m_fVecSize, 0.0, 10.0);
	DDX_Control(pDX, IDC_SPINSIZE_OUT, m_Spin_Outline);
	DDX_Control(pDX, IDC_SPINSIZE_VEC, m_Spin_Vecsize);
	DDX_Control(pDX,IDC_COLOR_SYM,m_BtnSym);
	DDX_Control(pDX,IDC_COLOR_LINE,m_BtnLine);
	DDX_Control(pDX,IDC_COLOR_VEC,m_BtnVec);
	DDX_Control(pDX,IDC_COLOR_BACK,m_BtnBack);
	DDX_Control(pDX, IDC_OPACITY_SYM, m_sliderSym);
	DDX_Control(pDX, IDC_OPACITY_VEC, m_sliderVec);
	DDX_Slider(pDX, IDC_OPACITY_SYM, m_iOpacitySym);
	DDX_Slider(pDX, IDC_OPACITY_VEC, m_iOpacityVec);
	DDX_Check(pDX, IDC_ANTIALIAS, m_bAntialias);
	DDX_Check(pDX, IDC_BACKSYM, m_bBackSym);
	DDX_Radio(pDX, IDC_GDI, m_bGDI);
	DDX_Control(pDX, IDC_VEC_COUNT, m_cbVecCount);
	DDX_CBIndex(pDX, IDC_VEC_COUNT, m_iVecCountIdx);

	if(!pDX->m_bSaveAndValidate) {
		m_BtnSym.SetColor(m_clrSym);
		m_BtnLine.SetColor(m_clrLine);
		m_BtnVec.SetColor(m_clrVec);
		m_BtnBack.SetColor(m_clrBack);
		m_Spin_Outline.SetRange(0,100);
		m_Spin_Vecsize.SetRange(0,100);
	}
	m_Spin_Outline.SetPos((int)(m_fLineSize*10));
	m_Spin_Vecsize.SetPos((int)(m_fVecSize*10));
}

BEGIN_MESSAGE_MAP(CSymDlg, CDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_APPLY, &CSymDlg::OnBnClickedApply)
    ON_MESSAGE(CPN_SELCHANGE, OnChgColor)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPINSIZE, OnSymSize)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPINSIZE_OUT, OnOutSize)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPINSIZE_VEC, OnOutSize)
	ON_BN_CLICKED(IDC_SQUARES, &CSymDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_CIRCLES, &CSymDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_TRIANGLES, &CSymDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_DOT_CENTER, &CSymDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_ANTIALIAS, &CSymDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_BACKSYM, &CSymDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_CAIRO, &CSymDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_DIRECT2D, &CSymDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_GDI, &CSymDlg::OnBnClickedApply)
	ON_CBN_SELCHANGE(IDC_VEC_COUNT, &CSymDlg::OnBnClickedApply)
END_MESSAGE_MAP()


// CSymDlg message handlers

BOOL CSymDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	//m_cbVecCount.SetRe

    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPINSIZE))->SetRange(0,100);

	m_sliderSym.SetRange(0,100);
	m_sliderVec.SetRange(0,100);

	if(!m_nSavedCX) {
		CRect r;
		GetWindowRect(&r);
		m_nSavedX = r.left;
		m_nSavedY = r.top;
		m_nSavedCX=r.Width();
		m_nSavedCY=r.Height();
		AfxGetMainWnd()->GetWindowRect(&r);
		m_nSavedX=r.right-m_nSavedCX;
		m_nSavedY=r.top;
	}
	CDialog::SetWindowPos(0,m_nSavedX,m_nSavedY,0,0,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE|SWP_NOREDRAW);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSymDlg::OnChgColor(WPARAM clr,LPARAM id)
{
	m_pView->UpdateColor(id,clr);
	return TRUE;
}

void CSymDlg::OnBnClickedApply()
{
	if(UpdateData())
		m_pView->UpdateSymbols(*this);
}

BOOL CSymDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
		// Slider Control
	int id=0;
	int i;
	if(wParam == m_sliderSym.GetDlgCtrlID())
	{
		i=m_sliderSym.GetPos();
		id=IDC_ST_OPACITY_SYM;
	}
	else if(wParam == m_sliderVec.GetDlgCtrlID())
	{
		i=m_sliderVec.GetPos();
		id=IDC_ST_OPACITY_VEC;
	}
	if(id) {
		CString s;
		s.Format("%3d %%",i);
		GetDlgItem(id)->SetWindowText(s);
		m_pView->UpdateOpacity(id,i);
	}
	return CDialog::OnNotify(wParam, lParam, pResult);
}

void CSymDlg::OnSymSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int i=pNMUpDown->iPos+pNMUpDown->iDelta;
	if (i<=1) {
		i = (pNMUpDown->iDelta>0) ? 2 : 0;
		((CSpinButtonCtrl *)GetDlgItem(IDC_SPINSIZE))->SetPos(i?1:0);
	}
	else if(i>100) i=100;
	m_iSymSize = i;
	m_pView->UpdateSymSize(i);
	*pResult = 0;
}

void CSymDlg::OnOutSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int i=pNMUpDown->iPos+pNMUpDown->iDelta;
	*pResult=1;
	if(i<0 || i>100) 
		return;

	UINT id=0;
	if(pNMHDR->idFrom==IDC_SPINSIZE_OUT)
		id=IDC_EDIT_LINE_SZ;
	else if(pNMHDR->idFrom==IDC_SPINSIZE_VEC)
		id=IDC_EDIT_VEC_SZ;
	if(!id) return;

	char buf[8];
	sprintf(buf,"%.1f",i/10.0);
	GetDlgItem(id)->SetWindowTextA(buf);
   
	if(UpdateData()) {
		m_pView->UpdateSymbols(*this);
	}
}

void CSymDlg::OnDestroy()
{
	CRect r;
	GetWindowRect(&r);
	m_nSavedX = r.left;
	m_nSavedY = r.top;
	m_nSavedCX=r.Width();
	m_nSavedCY=r.Height();

	CDialog::OnDestroy();
}
