// ExportNTI_Adv.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MapLayer.h"
#include "ExportNTI.h"
#include "ExportNTI_Adv.h"

// CExportNTI_Adv dialog

static const TCHAR szNtiExp[] = _T("NtiExport");
static const TCHAR szMethod[] = _T("Method");
static const TCHAR szFilter[] = _T("Filter");
//static const TCHAR szQuality[] = _T("Quality");
//static const TCHAR szRate[] = _T("Rate");
static const TCHAR szRateF[] = _T("RateF");
static const TCHAR szQualityF[] = _T("QualityF");
static const TCHAR szOverwrite[] = _T("Overwrite");
static const TCHAR szPreset[] = _T("Preset");

BOOL CExportNTI_Adv::m_AdvParamsLoaded = FALSE;
BOOL CExportNTI_Adv::m_bNoOverviews = FALSE;

NTI_ADV_PARAMS CExportNTI_Adv::m_Adv;

void CExportNTI_Adv::LoadAdvParams()
{
	if (!m_AdvParamsLoaded) {

		CString s(theApp.GetProfileString(szNtiExp, szRateF, "5"));
		m_Adv.fRate = (float)atof(s);
		if (m_Adv.fRate<1.0f || m_Adv.fRate>50.0f) m_Adv.fRate = 5.0f;

		s = theApp.GetProfileString(szNtiExp, szQualityF, "80");
		m_Adv.fQuality = (float)atof(s);
		if (m_Adv.fQuality<20.0f || m_Adv.fQuality>100.0f) m_Adv.fQuality = 80.0f;
		m_Adv.bPreset = (WORD)theApp.GetProfileInt(szNtiExp, szPreset, 0);   //Default
		if (m_Adv.bPreset >= NTI_WEBP_NUM_PRESETS) m_Adv.bPreset = 0;

		m_Adv.bMethod = (WORD)theApp.GetProfileInt(szNtiExp, szMethod, NTI_FCNJP2K);
		if (!m_Adv.bMethod || m_Adv.bMethod > NTI_FCNWEBP) m_Adv.bMethod = NTI_FCNJP2K;

		m_Adv.bFilter = (WORD)theApp.GetProfileInt(szNtiExp, szFilter, 0);
		if (m_Adv.bFilter > 3) m_Adv.bFilter = 0;

		m_Adv.bOverwrite = (WORD)theApp.GetProfileInt(szNtiExp, szOverwrite, 0);

		m_Adv.bGet_ssim = FALSE;
		m_AdvParamsLoaded = TRUE;
	}
	m_Adv.iTolerance = 5; //for now
}

void CExportNTI_Adv::SaveAdvParams()
{
	theApp.WriteProfileString(szNtiExp, szQualityF, GetFloatStr(m_Adv.fQuality, 1));
	theApp.WriteProfileString(szNtiExp, szRateF, GetFloatStr(m_Adv.fRate, 1));
	theApp.WriteProfileInt(szNtiExp, szMethod, m_Adv.bMethod);
	theApp.WriteProfileInt(szNtiExp, szFilter, m_Adv.bFilter);
	theApp.WriteProfileInt(szNtiExp, szOverwrite, m_Adv.bOverwrite);
	theApp.WriteProfileInt(szNtiExp, szPreset, m_Adv.bPreset);
}

bool CExportNTI_Adv::GetRateStr(CString &args)
{
	float rate = m_Adv.fRate;
	if (rate > 0.0f) args.Format("%s%%", GetFloatStr(rate, 1));
	else if (!rate) args = "Best";
	else {
		args = "Lossless";
		return false;
	}
	return true;
}

IMPLEMENT_DYNAMIC(CExportNTI_Adv, CDialog)

CExportNTI_Adv::CExportNTI_Adv(CImageLayer *pLayer, BOOL bSSIM_ok, CWnd* pParent /*=NULL*/)
	: CDialog(CExportNTI_Adv::IDD, pParent)
	, m_bOverwrite(m_Adv.bOverwrite)
	, m_bFilter(m_Adv.bFilter)
	, m_bPreset(m_Adv.bPreset)
	, m_bSSIM_ok(bSSIM_ok)
	, m_pLayer(pLayer)
{
#ifdef _USE_LZMA
	// dlg order:  {-1, M_loco, m_Lzma, M_zlib, M_jp2k, M_webp} //order of radio buttons
	// fcn order:  {NTI_FCNNONE,NTI_FCNZLIB,NTI_FCNLOCO,NTI_FCNJP2K,NTI_FCNWEBP,NTI_FCNLZMA};
	int dlgMeth[5] = { -1,         M_zlib,     M_loco,     M_jp2k,     M_webp,     M_lzma }; //program method index to dialog method index
#else
	// dlg order:  {-1, M_loco, M_zlib, M_jp2k, M_webp} //order of radio buttons
	// fcn order:  {NTI_FCNNONE,NTI_FCNZLIB,NTI_FCNLOCO,NTI_FCNJP2K,NTI_FCNWEBP};
	int dlgMeth[5] = { -1,         M_zlib,     M_loco,     M_jp2k,     M_webp }; //program method index to dialog method index
#endif
	m_bMethod = dlgMeth[m_Adv.bMethod];
	ASSERT(m_bMethod >= 0 && m_bMethod <= M_webp);
	m_csQuality = GetFloatStr(m_Adv.fQuality, 1);
	if (m_Adv.fRate > 0.0f) {
		m_bRateLossless = false;
		m_csRate = GetFloatStr(m_Adv.fRate, 1);
	}
	else {
		m_bRateLossless = (m_Adv.fRate < 0.0f);
		m_csRate = m_bRateLossless ? "Lossless" : "Best lossy";
	}
	m_bGet_ssim = m_Adv.bGet_ssim ? ComparableWithSSIM() : FALSE;
}

CExportNTI_Adv::~CExportNTI_Adv()
{
}

bool CExportNTI_Adv::CheckRate()
{
	CString s(m_csRate);
	s.Trim();
	if (s.IsEmpty()) return false;
	if (isdigit(s[0])) {
		double r = atof(s);
		if (r<1.0 || r>50.0) return false;
	}
	else {
		if (!strchr("BbLl", s[0])) return false;
	}
	return true;
}

float CExportNTI_Adv::ParseRate()
{
	m_csRate.Trim();
	ASSERT(!m_csRate.IsEmpty());
	char c = m_csRate[0];
	if (isdigit(c)) {
		double r = atof(m_csRate);
		return (float)atof(GetFloatStr(r, 1));
	}
	return (c == 'B' || c == 'b') ? 0.0f : -1.0f;
}

void CExportNTI_Adv::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_OVERWRITE, m_bOverwrite);
	DDX_Radio(pDX, IDC_METHOD_LOCO, m_bMethod);
	DDX_Radio(pDX, IDC_RMG_G_BMG, m_bFilter);
	DDX_Check(pDX, IDC_GET_SSIM, m_bGet_ssim);
	DDX_CBIndex(pDX, IDC_WEBP_PRESET, m_bPreset);
	DDX_Check(pDX, IDC_NO_OVERVIEWS, m_bNoOverviews);
	DDX_Control(pDX, IDC_TOLERANCE, m_cbTolerance);
	DDX_Control(pDX, IDC_WEBP_CBQUALITY, m_cbQuality);
	DDX_Control(pDX, IDC_JP2K_CBRATE, m_cbRate);

	if (pDX->m_bSaveAndValidate) {
#ifdef _USE_LZMA
		// dlg order:  { M_loco,      M_lzma,      M_zlib,      M_jp2k,      M_webp} //order of radio buttons
		int pgmMeth[5] = { NTI_FCNLOCO, NTI_FCNLZMA, NTI_FCNZLIB, NTI_FCNJP2K, NTI_FCNWEBP }; //dialog index to program index
#else
		// dlg order:  {M_loco,      M_zlib,      M_jp2k,      M_webp} //order of radio buttons
		int pgmMeth[5] = { NTI_FCNLOCO, NTI_FCNZLIB, NTI_FCNJP2K, NTI_FCNWEBP }; //dialog index to program index
#endif
		m_cbRate.GetWindowText(m_csRate);
		if (!CheckRate()) {
			AfxMessageBox("% Size must be a number between 1 and 50 if not \"Best lossy\" or \"Lossless\".");
			pDX->m_idLastControl = IDC_JP2K_CBRATE;
			pDX->Fail();
			return;
		}
		{
			m_cbQuality.GetWindowText(m_csQuality);
			float fQual = (float)atof(GetFloatStr(atof(m_csQuality), 1));
			if (fQual<20.0 || fQual>100.0f) {
				AfxMessageBox("Quality must be a number ranging from 20 to 100.");
				pDX->m_idLastControl = IDC_WEBP_CBQUALITY;
				pDX->Fail();
				return;
			}
			m_Adv.fQuality = fQual;
		}
		m_Adv.fRate = ParseRate();
		m_Adv.bMethod = pgmMeth[m_bMethod];
		m_Adv.bFilter = m_bFilter;
		m_Adv.bOverwrite = m_bOverwrite;
		int i = m_cbTolerance.GetCurSel();
		if (i >= 0 && i <= 6) m_Adv.iTolerance = i;
		m_Adv.bGet_ssim = m_bGet_ssim && IsLossyMethod();
		m_Adv.bPreset = m_bPreset;
		SaveAdvParams();
	}
	else {
		m_cbRate.SetWindowText(m_csRate);
		m_cbQuality.SetWindowText(m_csQuality);
		EnableControls();
		if (!m_pLayer->m_bUseTransColor) {
			Enable(IDC_TOLERANCE, 0);
		}
		else {
			int tol = m_Adv.iTolerance; // 0 <= tol <= 6
			if (m_pLayer->IsUsingBGRE()) {
				tol = CImageLayer::GetTolerance(m_pLayer->m_crTransColor);
			}
			VERIFY(CB_ERR != m_cbTolerance.SetCurSel(tol));
		}
	}
}

BEGIN_MESSAGE_MAP(CExportNTI_Adv, CDialog)
	ON_CBN_SELCHANGE(IDC_JP2K_CBRATE, OnChangeJP2KRate)
	ON_BN_CLICKED(IDC_DEFAULT, OnBnClickedDefault)
	ON_BN_CLICKED(IDC_METHOD_LOCO, OnBnClickedMethodLoco)
#ifdef _USE_LZMA
	ON_BN_CLICKED(IDC_METHOD_LZMA, OnBnClickedMethodLzma)
#endif
	ON_BN_CLICKED(IDC_METHOD_ZLIB, OnBnClickedMethodZlib)
	ON_BN_CLICKED(IDC_METHOD_JP2K, OnBnClickedMethodJp2k)
	ON_BN_CLICKED(IDC_METHOD_WEBP, OnBnClickedMethodWebp)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

// CExportNTI_Adv message handlers

BOOL CExportNTI_Adv::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CExportNTI_Adv::EnableControls()
{
	BOOL bEnable = (m_bMethod <= M_zlib);
	Enable(IDC_RMG_G_BMG, bEnable);
	Enable(IDC_RMB_GMB_B, bEnable);
	Enable(IDC_R_GMR_BMR, bEnable);
	Enable(IDC_NOFILTER, bEnable);
	Enable(IDC_JP2K_CBRATE, m_bMethod == M_jp2k);
	bEnable = (m_bMethod == M_webp);
	Enable(IDC_WEBP_PRESET, bEnable);
	Enable(IDC_WEBP_CBQUALITY, bEnable);
	Enable(IDC_GET_SSIM, ComparableWithSSIM());
}

void CExportNTI_Adv::OnBnClickedDefault()
{
	m_bMethod = M_jp2k; //jp2k best for phototographic imagery?
	m_bPreset = 0; //default
	m_csQuality = "80";
	m_bGet_ssim = 0;
	m_bFilter = 0;
	m_bOverwrite = 0;
	m_csRate = "5";
	m_bRateLossless = false;
	UpdateData(FALSE);
}

void CExportNTI_Adv::OnBnClickedMethodLoco()
{
	m_bMethod = M_loco;
	EnableControls();
}

#ifdef _USE_LZMA
void CExportNTI_Adv::OnBnClickedMethodLzma()
{
	m_bMethod = M_lzma;
	EnableControls();
}
#endif

void CExportNTI_Adv::OnBnClickedMethodZlib()
{
	m_bMethod = M_zlib;
	EnableControls();
}

void CExportNTI_Adv::OnBnClickedMethodJp2k()
{
	m_bMethod = M_jp2k;
	EnableControls();
}

void CExportNTI_Adv::OnBnClickedMethodWebp()
{
	m_bMethod = M_webp;
	EnableControls();
}

void CExportNTI_Adv::OnChangeJP2KRate()
{
	ASSERT(m_bMethod == M_jp2k);
	if (m_bSSIM_ok) {
		m_bRateLossless = (m_cbRate.GetCurSel() == m_cbRate.GetCount() - 1);
		Enable(IDC_GET_SSIM, m_bRateLossless == false);
	}
}

LRESULT CExportNTI_Adv::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1018);
	return TRUE;
}
