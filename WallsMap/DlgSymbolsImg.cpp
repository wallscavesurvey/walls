// SymImgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "MapLayer.h"
#include "resource.h"
#include "DlgSymbolsImg.h"
#include "afxdialogex.h"

// CDlgSymbolsImg dialog

IMPLEMENT_DYNAMIC(CDlgSymbolsImg, CDialog)

void CDlgSymbolsImg::InitData(CImageLayer *pLayer)
{
	//Initialize current and saved settings (not controls) with settings for pLayer.
	m_pLayer = pLayer;
	m_bHiddenChg = false;

	//Save original settings in case of cancel --
	m_bAlphaSav = pLayer->m_bAlpha;
	m_crTransColorSav = pLayer->m_crTransColor;
	m_bUseTransColorSav = pLayer->m_bUseTransColor;
	m_iLastToleranceSav = pLayer->m_iLastTolerance;

	m_bUseTransColor = m_bUseTransColorSav; //bool to int
	m_crTransColor = m_crTransColorSav & 0xffffff;
	m_bTolerance = CImageLayer::GetTolerance(m_crTransColorSav); // 0 <= GetTolerance() <= 6    labels: (2,4,8,16,32,64)
	m_bMatchSimilarColors = (m_bTolerance > 0);
	if (!m_bMatchSimilarColors) {
		m_bTolerance = m_iLastToleranceSav - 1; //-1 if not supprted (1 band)
	}
	else {
		m_bTolerance--;
	}
	m_iOpacity = (m_bAlphaSav * 100) / 255;
}

CDlgSymbolsImg::CDlgSymbolsImg(CImageLayer *pLayer, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSymbolsImg::IDD, pParent)
{
	//Initialize current and saved settings (not controls) with settings for pLayer.
	InitData(pLayer);
	if (Create(CDlgSymbolsImg::IDD, pParent)) {
		CImageLayer::m_pDlgSymbolsImg = this;
	}
}

void CDlgSymbolsImg::NewLayer(CImageLayer *pLayer /*=0*/)
{
	if (!pLayer && !IsLayerUpdated()) return;
	//Set view and layer to the saved state without modifying dialog state --
	RestoreData();
	if (pLayer) {
		pLayer->InitDlgTitle(this);
	}
	else pLayer = m_pLayer;
	//Initialize current and saved settings (not controls) with new settings for pLayer.
	InitData(pLayer);
	//Update controls --
	UpdateData(0);
}

CDlgSymbolsImg::~CDlgSymbolsImg()
{
	CImageLayer::m_pDlgSymbolsImg = NULL;
}

BOOL CDlgSymbolsImg::IsLayerUpdated()
{
	// Compare current layer settings with saved state.
	// return >0 if changed, 2 if view refresh would be needed when reverting
	BOOL b = (m_pLayer->m_bAlpha != m_bAlphaSav || m_pLayer->m_bUseTransColor != m_bUseTransColorSav) ? 2 : 0;
	if (!b) {
		b = m_pLayer->m_crTransColor != m_crTransColorSav;
		if (b && (m_pLayer->m_bUseTransColor || m_bUseTransColorSav)) b++;
	}
	return b;
}

bool CDlgSymbolsImg::IsDialogUpdated()
{
	// Compare current dialog settings with saved state.
	// return true if any setting has changed
	if ((BYTE)((m_iOpacity * 255) / 100) != m_bAlphaSav || (m_bUseTransColor != 0) != m_bUseTransColorSav ||
		m_crTransColor != (m_crTransColorSav & 0xffffff)) return true;

	int itol = CImageLayer::GetTolerance(m_crTransColorSav); // 0 <= GetTolerance() <= 6    labels: (2,4,8,16,32,64)
	if (m_bMatchSimilarColors != (itol > 0)) return true;

	if (!m_bMatchSimilarColors) {
		return m_bTolerance != m_iLastToleranceSav - 1; //-1 if not supprted (1 band)
	}
	return m_bTolerance != itol - 1;
}

void CDlgSymbolsImg::UpdateDoc()
{
	//Update view and layer with dialog settings without changing saved settings
	bool bChg = false;
	BYTE bAlpha = (BYTE)((m_iOpacity * 255) / 100);
	if (bAlpha != m_pLayer->m_bAlpha) {
		m_pLayer->m_bAlpha = bAlpha;
		bChg = true;
	}
	bChg = bChg || m_bUseTransColor != (BOOL)m_pLayer->m_bUseTransColor;

	BYTE bError = (m_bUseTransColor && m_bMatchSimilarColors) ? (1 << (m_bTolerance + 1)) : 0;
	if (bError) m_pLayer->m_iLastTolerance = m_bTolerance + 1;
	COLORREF newColor = (m_crTransColor | (bError << 24));

	bChg = bChg || m_bUseTransColor && m_pLayer->m_crTransColor != newColor;

	if (bChg || m_pLayer->m_crTransColor != newColor) {
		//view refresh may be unnecessary
		m_pLayer->m_bUseTransColor = m_bUseTransColor != 0;
		m_pLayer->m_crTransColor = newColor;
	}

	if (bChg && m_pLayer->IsVisible())
		m_pLayer->m_pDoc->RefreshViews();
	GetDlgItem(IDOK)->EnableWindow(IsLayerUpdated() > 0);
}

void CDlgSymbolsImg::RestoreData()
{
	//Set view and layer to the saved state without modifying dialog state --
	BOOL b = IsLayerUpdated();
	if (b) {
		m_pLayer->m_crTransColor = m_crTransColorSav;
		m_pLayer->m_bUseTransColor = m_bUseTransColorSav;
		m_pLayer->m_iLastTolerance = m_iLastToleranceSav;
		m_pLayer->m_bAlpha = m_bAlphaSav;
		if (b > 1 && m_pLayer->IsVisible())
			m_pLayer->m_pDoc->RefreshViews();
	}
}

void CDlgSymbolsImg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TRANSCLR, m_BtnTransClr);
	DDX_Control(pDX, IDC_OPACITY_IMG, m_sliderOpacity);
	DDX_Slider(pDX, IDC_OPACITY_IMG, m_iOpacity);
	DDV_MinMaxInt(pDX, m_iOpacity, 0, 100);
	DDX_Check(pDX, IDC_USE_TRANSCLR, m_bUseTransColor);
	DDX_Check(pDX, IDC_MAKE_SIMILAR, m_bMatchSimilarColors);
	DDX_Radio(pDX, IDC_TOLERANCE, m_bTolerance);
	DDX_Control(pDX, IDC_PREVIEW, m_Preview);

	if (!pDX->m_bSaveAndValidate) {
		m_BtnTransClr.SetColor(m_crTransColor);
		UpdateOpacityLabel();
		m_sliderOpacity.SetFocus();
		GetDlgItem(IDOK)->EnableWindow(0);
	}
}

BEGIN_MESSAGE_MAP(CDlgSymbolsImg, CDialog)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedAccept)
	ON_MESSAGE(CPN_SELCHANGE, OnChgColor)
	ON_BN_CLICKED(IDC_USE_TRANSCLR, &CDlgSymbolsImg::OnBnClickedUseTransclr)
	ON_BN_CLICKED(IDC_MAKE_SIMILAR, &CDlgSymbolsImg::OnBnClickedMakeSimilar)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_TOLERANCE, IDC_TOLERANCE6, OnBnClickedTolerance)
	ON_BN_CLICKED(IDC_MAX_OPACITY, &CDlgSymbolsImg::OnBnClickedMaxOpacity)
	ON_BN_CLICKED(IDC_PREVIEW, &CDlgSymbolsImg::OnBnClickedPreview)
END_MESSAGE_MAP()

// CDlgSymbolsImg message handlers

void CDlgSymbolsImg::UpdateOpacityLabel()
{
	CString s;
	s.Format("%3d %%", m_iOpacity);
	GetDlgItem(IDC_ST_OPACITY_IMG)->SetWindowText(s);
	GetDlgItem(IDC_MAX_OPACITY)->EnableWindow(m_iOpacity < 100);
	BOOL bEnable = m_bUseTransColor && m_bTolerance >= 0;
	GetDlgItem(IDC_MAKE_SIMILAR)->EnableWindow(bEnable);
	bEnable = bEnable && m_bMatchSimilarColors;
	for (int id = IDC_TOLERANCE; id < IDC_TOLERANCE + 6; id++) {
		GetDlgItem(id)->EnableWindow(bEnable);
	}
}

BOOL CDlgSymbolsImg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_Preview.SetCheck(m_bPreview = true);

	m_pLayer->InitDlgTitle(this);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CDlgSymbolsImg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Slider Control
	if (wParam == IDC_OPACITY_IMG)
	{
		int i = m_sliderOpacity.GetPos();
		if (i != m_iOpacity) {
			m_iOpacity = i;
			UpdateOpacityLabel();
			if (m_bPreview) UpdateDoc();
			else SetHiddenChg();
		}
	}
	return CDialog::OnNotify(wParam, lParam, pResult);
}


void CDlgSymbolsImg::OnBnClickedMaxOpacity()
{
	m_sliderOpacity.SetPos(m_iOpacity = 100);
	UpdateOpacityLabel();
	if (m_bPreview) UpdateDoc();
	else SetHiddenChg();
}


LRESULT CDlgSymbolsImg::OnChgColor(WPARAM clr, LPARAM id)
{
	if (id == IDC_TRANSCLR && m_crTransColor != clr) {
		m_crTransColor = clr;
		if (m_bUseTransColor) {
			if (m_bPreview) UpdateDoc();
			else SetHiddenChg();
		}
	}
	return TRUE;
}

void CDlgSymbolsImg::OnBnClickedUseTransclr()
{
	m_bUseTransColor = !m_bUseTransColor;
	UpdateOpacityLabel();
	if (m_bPreview) UpdateDoc();
	else SetHiddenChg();
}


void CDlgSymbolsImg::OnBnClickedMakeSimilar()
{
	m_bMatchSimilarColors = !m_bMatchSimilarColors;
	UpdateOpacityLabel();
	if (m_bPreview) UpdateDoc();
	else SetHiddenChg();
}

void CDlgSymbolsImg::OnBnClickedTolerance(UINT id)
{
	if (id >= IDC_TOLERANCE && id < IDC_TOLERANCE + 6) {
		m_bTolerance = id - IDC_TOLERANCE;
		if (m_bPreview) UpdateDoc();
		else SetHiddenChg();
	}
}

void CDlgSymbolsImg::PostNcDestroy()
{
	delete this;
}

void CDlgSymbolsImg::OnBnClickedCancel()
{
	OnClose();
}

void CDlgSymbolsImg::OnClose()
{
	//Close button or cancel
	//Set view and layer to the saved state without modifying dialog state --
	RestoreData();
	CImageLayer::m_pDlgSymbolsImg = NULL;
	DestroyWindow();
}

void CDlgSymbolsImg::OnBnClickedAccept()
{
	if (!m_bPreview) {
		//The dialog settings are likely different from the layer and saved settings
		//so update layer and screen, then reinitialize saved settings
		if (m_bHiddenChg) {
			UpdateDoc();
		}
	}
	m_bHiddenChg = false;
	if (!m_bPreview || IsLayerUpdated()) {
		//The layer settings and screen do not match the saved state
		//Initialize dialog and saved settings with settings for pLayer.
		InitData(m_pLayer);
		UpdateData(0); //update controls also
		m_pLayer->m_pDoc->SetChanged();
		GetDlgItem(IDOK)->EnableWindow(0);
	}
	else ASSERT(0);
}

void CDlgSymbolsImg::OnBnClickedPreview()
{
	if (m_bPreview) {
		//dialog settings should already match those of layer and screen
		//We need to restore screen and layer to the saved settings without changing
		//current dialog settings --
		RestoreData();
		if ((m_bHiddenChg = IsDialogUpdated())) //sometimes false positive after accept!!
			GetDlgItem(IDOK)->EnableWindow(1);

		//hencefoth, simply adjust dialog settings, keeping screen and layer the same
		m_bPreview = false;
	}
	else {
		//may need to update layer and screen with dialog seetings we've been
		//freely adjusting, still keeping saved settings the same
		if (m_bHiddenChg) {
			//some controls have been adjusted
			UpdateDoc();
		}
		m_bHiddenChg = false; //not needed
		m_bPreview = true;
	}
}
