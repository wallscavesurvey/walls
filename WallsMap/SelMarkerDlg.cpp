// SelMarkerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "ShowShapeDlg.h"
#include "SelMarkerDlg.h"
#include "GPSPortSettingDlg.h"

// CSelMarkerDlg dialog

IMPLEMENT_DYNAMIC(CSelMarkerDlg, CDialog)
CSelMarkerDlg::CSelMarkerDlg(bool bGPS, CWnd* pParent /*=NULL*/)
	: CDialog(CSelMarkerDlg::IDD, pParent)
	, m_bGPS(bGPS)
	, m_pDoc(NULL)
	, m_bSelected(0)
	, m_pRT(NULL)
	, m_pBR(NULL)
{
	if (bGPS) {
		m_pDoc = GetMF()->FirstGeoRefSupportedDoc();
		m_bActiveItems = m_pDoc != NULL;
		if (!CGPSPortSettingDlg::m_bMarkerInitialized)
			CGPSPortSettingDlg::InitGPSMarker();
		m_pmstyle_s = &CGPSPortSettingDlg::m_mstyle_s;
		m_pmstyle_h = &CGPSPortSettingDlg::m_mstyle_h;
		m_mstyle_t = CGPSPortSettingDlg::m_mstyle_t;
	}
	else {
		m_bActiveItems = app_pShowDlg != NULL && app_pShowDlg->m_pDoc->m_bMarkSelected && (app_pShowDlg->NumSelected() != 0);
		if (m_bActiveItems) m_pDoc = app_pShowDlg->m_pDoc;
		else m_pDoc = GetMF()->GetActiveDoc();
		m_pmstyle_s = &CShpLayer::m_mstyle_s;
		m_pmstyle_h = &CShpLayer::m_mstyle_h;
	}
	m_clrPrev = m_pDoc ? m_pDoc->m_clrPrev : 0;
	m_mstyle_s = *m_pmstyle_s;
	m_mstyle_h = *m_pmstyle_h;
	m_bInitFlag = true;

	VERIFY(0 <= d2d_pFactory->CreateDCRenderTarget(&d2d_props, &m_pRT)); //also calls Init2D()
	if (m_bGPS)
		//will be changing color and opacity --
		VERIFY(0 <= m_pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pBR));
}

CSelMarkerDlg::~CSelMarkerDlg()
{
	SafeRelease(&m_pBR);
	SafeRelease(&m_pRT);
}

void CSelMarkerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COLORFRM, m_rectSym);
	DDX_Control(pDX, IDC_FGCOLOR, m_BtnMrk);
	DDX_Control(pDX, IDC_BKGCOLOR, m_BtnBkg);
	DDX_Control(pDX, IDC_TAB_TYPEMARK, m_tabTypeMark);
	DDX_Control(pDX, IDC_COLORFRM_BKGND, m_BtnLbl);
	DDX_Control(pDX, IDC_OPACITY_SYM2, m_sliderSym);

	if (pDX->m_bSaveAndValidate) {
		OnBnClickedApply();
		if (m_pDoc) m_pDoc->m_clrPrev = m_clrPrev;
	}
}

BEGIN_MESSAGE_MAP(CSelMarkerDlg, CDialog)
	ON_MESSAGE(CPN_SELCHANGE, OnChgColor)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_MSIZE, OnMarkerSize)
	ON_EN_CHANGE(IDC_MSIZE, OnEnChangeMarkerSize)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_MLINEW, OnMarkerLineWidth)
	ON_EN_CHANGE(IDC_MLINEW, OnEnChangeMarkerLineWidth)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_TYPEMARK, OnTabChanged)
	ON_CBN_SELCHANGE(IDC_MSHAPE, OnSelShape)
	ON_BN_CLICKED(IDC_MOPAQUE, OnFilled)
	ON_BN_CLICKED(IDC_MCLEAR, OnClear)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
	ON_BN_CLICKED(IDC_RESTORE_DFLT, &CSelMarkerDlg::OnBnClickedRestoreDflt)
END_MESSAGE_MAP()

void CSelMarkerDlg::SetTabbedCntrls()
{
	if (m_bSelected == 0) {
		//description fields--
		if (m_bGPS) Show(IDC_ST_TRACKLINE, false);
		Show(IDC_ST_HIGHLIGHTED, false);
		Show(IDC_ST_ALLPOINTS, true);
		m_pmstyle = &m_mstyle_s;
	}
	else if (m_bSelected == 1) {
		//description fields--
		if (m_bGPS) Show(IDC_ST_TRACKLINE, false);
		Show(IDC_ST_ALLPOINTS, false);
		Show(IDC_ST_HIGHLIGHTED, true);
		m_pmstyle = &m_mstyle_h;
	}
	else {
		ASSERT(m_bGPS && m_bSelected == 2);
		Show(IDC_ST_ALLPOINTS, false);
		Show(IDC_ST_HIGHLIGHTED, false);
		Show(IDC_ST_TRACKLINE, true);
		m_pmstyle = &m_mstyle_t;
	}
	m_sliderSym.SetPos(m_pmstyle->iOpacitySym);
	UpdateOpacityLabel();

	m_bInitFlag = true;
	GetDlgItem(IDC_MLINEW)->SetWindowText(GetFloatStr(m_pmstyle->fLineWidth, 1));
	m_BtnMrk.SetColor(m_pmstyle->crMrk);
	if (m_bSelected < 2) {
		if (m_bGPS) {
			//Enable(IDC_ST_INTERIOR,1);
			//Enable(IDC_ST_SHAPE,1);
			SetText(IDC_ST_OUTLINE, "Outline:");
			Enable(IDC_ST_MSIZE_LBL, 1);
			Enable(IDC_MSIZE, 1);
			Enable(IDC_SPIN_MSIZE, 1);
			Enable(IDC_ST_BKGCOLOR_LBL, 1);
			m_BtnBkg.EnableWindow(1);
			Enable(IDC_MSHAPE, 1);
			Enable(IDC_MCLEAR, 1);
			Enable(IDC_MOPAQUE, 1);
		}
		SetText(IDC_MSIZE, GetIntStr(m_pmstyle->wSize));
		m_BtnBkg.SetColor(m_pmstyle->crBkg);
		VERIFY(CB_ERR != ((CComboBox *)GetDlgItem(IDC_MSHAPE))->SetCurSel(m_pmstyle->wShape));
		VERIFY(::CheckRadioButton(m_hWnd, IDC_MCLEAR, IDC_MOPAQUE, (m_pmstyle->wFilled&FSF_FILLED) ? IDC_MOPAQUE : IDC_MCLEAR));
		if (m_bGPS) m_BtnBkg.SetFocus();
	}
	else {
		//Enable(IDC_ST_INTERIOR,0);
		//Enable(IDC_ST_SHAPE,0);
		SetText(IDC_ST_OUTLINE, "Color:");
		Enable(IDC_ST_MSIZE_LBL, 0);
		SetText(IDC_MSIZE, "");
		Enable(IDC_MSIZE, 0);
		Enable(IDC_SPIN_MSIZE, 0);
		Enable(IDC_ST_BKGCOLOR_LBL, 0);
		m_BtnBkg.EnableWindow(0);
		Enable(IDC_MSHAPE, 0);
		Enable(IDC_MCLEAR, 0);
		Enable(IDC_MOPAQUE, 0);
	}
	m_BtnLbl.SetColor(m_clrPrev);
	m_bInitFlag = false;
}

BOOL CSelMarkerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	Enable(IDC_APPLY, m_bActiveItems);

	if (m_bGPS) {
		LPCSTR p = "The GPS placemark is a composite of two images. The top image will be displayed above the bottom one.";
		SetText(IDC_ST_ALLPOINTS, p);
		SetText(IDC_ST_HIGHLIGHTED, p);
		SetText(IDC_ST_ALLPOINTS_LBL, "Bottom");
		SetText(IDC_ST_HIGHLIGHTED_LBL, "Placemark");
		m_tabTypeMark.InsertItem(0, "Bottom Image");
		m_tabTypeMark.InsertItem(1, "Top Image");
		m_tabTypeMark.InsertItem(2, "Track Line");
		SetWindowText("Symbol for Marking GPS position");
	}
	else {
		Show(IDC_ST_TRACKLINE, false);
		m_tabTypeMark.InsertItem(0, "Selected Points");
		m_tabTypeMark.InsertItem(1, "Highlighted Point");
	}
	m_rectSym.GetClientRect(&m_rectBox);
	m_sizeBox.cx = m_rectBox.right;
	m_sizeBox.cy = m_rectBox.bottom;

	CRect rect(m_rectBox);
	rect.DeflateRect(15, 0);

	int div = rect.Width() / 4;
	m_ptCtrBox1.x = rect.left + div;
	m_ptCtrBox2.x = rect.right - div;

	m_ptCtrBox1.y = m_ptCtrBox2.y = m_sizeBox.cy / 2;

	m_rectSym.ClientToScreen(&m_rectBox);
	ScreenToClient(&m_rectBox);

	// Get temporary DC for dialog - Will be released in dc destructor
	CClientDC dc(this);
	VERIFY(m_cBmp.CreateCompatibleBitmap(&dc, m_sizeBox.cx, m_sizeBox.cy));
	VERIFY(m_dcBmp.CreateCompatibleDC(&dc));
	VERIFY(m_hBmpOld = (HBITMAP)::SelectObject(m_dcBmp.GetSafeHdc(), m_cBmp.GetSafeHandle()));

	VERIFY(0 <= m_pRT->BindDC(m_dcBmp, CRect(0, 0, m_sizeBox.cx, m_sizeBox.cy)));

	m_ToolTips.Create(this);
	m_BtnMrk.AddToolTip(&m_ToolTips);
	m_BtnBkg.AddToolTip(&m_ToolTips);

	m_tabTypeMark.SetCurSel(m_bSelected);

	SetTabbedCntrls();
	DrawSymbol();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CSelMarkerDlg::OnDestroy()
{
	CDialog::OnDestroy();
	if (m_dcBmp.GetSafeHdc()) {
		if (m_hBmpOld)
			VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(), m_hBmpOld));
		VERIFY(m_dcBmp.DeleteDC());
	}
}

void CSelMarkerDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	dc.BitBlt(m_rectBox.left, m_rectBox.top, m_rectBox.Width(), m_rectBox.Height(),
		&m_dcBmp, 0, 0, SRCCOPY);
}

void CSelMarkerDlg::DrawSymbol()
{
	//Place select symbol on bitmap --
	m_dcBmp.FillSolidRect(0, 0, m_sizeBox.cx, m_sizeBox.cy, m_clrPrev);

	m_pRT->BeginDraw();

	if (m_bGPS) {
		//track line uses OpacitySym not OpacityVec
		m_pBR->SetColor(RGBAtoD2D(m_mstyle_t.crMrk, m_mstyle_t.iOpacitySym / 100.f));
		float w = m_mstyle_t.fLineWidth;
		m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		m_pRT->DrawLine(
			CD2DPointF(m_ptCtrBox2.x - 50.f, m_ptCtrBox2.y - 20.f),
			CD2DPointF(m_ptCtrBox2.x - 20.f, m_ptCtrBox2.y - 10.f), m_pBR, w, d2d_pStrokeStyleRnd);
		m_pRT->DrawLine(
			CD2DPointF(m_ptCtrBox2.x - 20.f, m_ptCtrBox2.y - 10.f),
			CD2DPointF(m_ptCtrBox2), m_pBR, w, d2d_pStrokeStyleRnd);
	}

	//Paint lower image at left --
	CPlaceMark::PlotStyleSymbol(m_pRT, m_mstyle_s, m_ptCtrBox1);
	//Paint lower image at right --
	CPlaceMark::PlotStyleSymbol(m_pRT, m_mstyle_s, m_ptCtrBox2);
	//Paint top image at right --
	CPlaceMark::PlotStyleSymbol(m_pRT, m_mstyle_h, m_ptCtrBox2);

	VERIFY(0 <= m_pRT->EndDraw());
	InvalidateRect(&m_rectBox, FALSE);
}

LRESULT CSelMarkerDlg::OnChgColor(WPARAM clr, LPARAM id)
{
	if (id == IDC_BKGCOLOR) {
		m_pmstyle->crBkg = clr;
	}
	else if (id == IDC_FGCOLOR) {
		m_pmstyle->crMrk = clr;
	}
	else {
		ASSERT(id == IDC_COLORFRM_BKGND);
		m_clrPrev = clr;
	}
	if (id != IDC_BKGCOLOR || m_bSelected < 2)
		DrawSymbol();
	return TRUE;
}

void CSelMarkerDlg::UpdateMarkerSize(int inc0)
{
	CString s;
	GetDlgItem(IDC_MSIZE)->GetWindowText(s);
	if (!inc0 && s.IsEmpty()) return;
	int i = atoi(s) - inc0;
	if (i <= 0) i = 0;
	else if (i > CShpLayer::MARKER_MAXSIZE) i = CShpLayer::MARKER_MAXSIZE;
	else if (!(i & 1)) i -= inc0;
	if (inc0) {
		m_bInitFlag = true;
		SetText(IDC_MSIZE, GetIntStr(i));
		m_bInitFlag = false;
	}
	m_pmstyle->wSize = (WORD)i;
	DrawSymbol();
}

void CSelMarkerDlg::OnMarkerSize(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 1;
	UpdateMarkerSize(((NM_UPDOWN*)pNMHDR)->iDelta);
}

void CSelMarkerDlg::OnEnChangeMarkerSize()
{
	if (!m_bInitFlag) UpdateMarkerSize(0);
}

void CSelMarkerDlg::UpdateMarkerLineWidth(int inc0)
{
	CString s;
	GetDlgItem(IDC_MLINEW)->GetWindowText(s);
	if (!inc0 && s.IsEmpty()) return;

	double fsiz = atof(s) - inc0 * 0.2;
	if (fsiz < 0.0) fsiz = 0;
	else if (fsiz > CShpLayer::MARKER_MAXLINEW) fsiz = CShpLayer::MARKER_MAXLINEW;
	s = GetFloatStr(fsiz, 1);
	if (inc0) {
		m_bInitFlag = true;
		SetText(IDC_MLINEW, s);
		m_bInitFlag = false;
	}
	m_pmstyle->fLineWidth = (float)atof(s);
	DrawSymbol();
}

void CSelMarkerDlg::OnMarkerLineWidth(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 1;
	UpdateMarkerLineWidth(((NM_UPDOWN*)pNMHDR)->iDelta);
}

void CSelMarkerDlg::OnEnChangeMarkerLineWidth()
{
	if (!m_bInitFlag) UpdateMarkerLineWidth(0);
}

void CSelMarkerDlg::OnFilled()
{
	m_pmstyle->wFilled |= FSF_FILLED;
	DrawSymbol();
}

void CSelMarkerDlg::OnClear()
{
	m_pmstyle->wFilled &= ~FSF_FILLED;
	DrawSymbol();
}

void CSelMarkerDlg::OnSelShape()
{
	int i = ((CComboBox *)GetDlgItem(IDC_MSHAPE))->GetCurSel();
	ASSERT(i >= 0);
	m_pmstyle->wShape = i;
	DrawSymbol();
}

void CSelMarkerDlg::OnTabChanged(LPNMHDR pHdr, LRESULT* pResult)
{
	ASSERT(pHdr->idFrom == IDC_TAB_TYPEMARK);
	m_bSelected = m_tabTypeMark.GetCurSel();
	SetTabbedCntrls();
	*pResult = 1;
}

void CSelMarkerDlg::UpdateOpacityLabel()
{
	CString s;
	s.Format("%3d %%", m_pmstyle->iOpacitySym);
	SetText(IDC_ST_OPACITY_SYM, s);
}

BOOL CSelMarkerDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Slider Control
	if (wParam == IDC_OPACITY_SYM2)
	{
		int i = m_sliderSym.GetPos();
		if (i != m_pmstyle->iOpacitySym) {
			m_pmstyle->iOpacitySym = i;
			UpdateOpacityLabel();
			DrawSymbol();
		}
	}
	return CDialog::OnNotify(wParam, lParam, pResult);
}

void CSelMarkerDlg::OnBnClickedApply()
{
	bool bRepaint = false;
	if (memcmp(&m_mstyle_s, m_pmstyle_s, sizeof(SHP_MRK_STYLE))) {
		*m_pmstyle_s = m_mstyle_s;
		bRepaint = true;
	}
	if (memcmp(&m_mstyle_h, m_pmstyle_h, sizeof(SHP_MRK_STYLE))) {
		*m_pmstyle_h = m_mstyle_h;
		bRepaint = true;
	}
	if (m_bGPS && memcmp(&m_mstyle_t, &CGPSPortSettingDlg::m_mstyle_t, sizeof(SHP_MRK_STYLE))) {
		CGPSPortSettingDlg::m_mstyle_t = m_mstyle_t;
		bRepaint = true;
	}

	if (bRepaint) {
		if (!m_bGPS) {
			CShpLayer::m_bSelectedMarkerChg = true;
			if (m_bActiveItems)
				app_pShowDlg->m_pDoc->RepaintViews();
		}
		else {
			CGPSPortSettingDlg::m_bGPSMarkerChg = true;
			if (m_bActiveItems) GetMF()->UpdateGPSTrack();
		}
	}
}

void CSelMarkerDlg::OnBnClickedRestoreDflt()
{
	if (m_bGPS) {
		m_mstyle_s = CGPSPortSettingDlg::m_mstyle_sD;
		m_mstyle_h = CGPSPortSettingDlg::m_mstyle_hD;
		m_mstyle_t = CGPSPortSettingDlg::m_mstyle_tD;
	}
	else {
		m_mstyle_s = CShpLayer::m_mstyle_sD;
		m_mstyle_h = CShpLayer::m_mstyle_hD;
	}
	SetTabbedCntrls();
	DrawSymbol();
}
