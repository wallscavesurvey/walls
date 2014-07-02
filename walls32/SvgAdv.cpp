// SvgAdv.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "wall_shp.h"
#include "svgadv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const TCHAR szSvgExp[] = _T("SvgExport");
static const TCHAR szMaxCoordChg[] = _T("MaxChgXY");
static const TCHAR szVecsUsed[] = _T("VecsUsed");
static const TCHAR szFlags[] = _T("Flags");
static const TCHAR szDecimals[] = _T("Decimals");
static const TCHAR szLenExp[] = _T("LenExp");
static const TCHAR szLenMin[] = _T("LenMin");
static const TCHAR szDistExp[] = _T("DistExp");
static const TCHAR szDistOff[] = _T("DistOff");

void CSvgAdv::LoadAdvParams(SVG_ADV_PARAMS *pAdv,BOOL bMerging)
{
	pAdv->decimals=(WORD)AfxGetApp()->GetProfileInt(szSvgExp,szDecimals,SVG_DECIMALS_DFLT);
	pAdv->uFlags=(WORD)AfxGetApp()->GetProfileInt(szSvgExp,szFlags,SVG_ADV_FLAGS_DFLT);
	if(bMerging) {
		pAdv->vecs_used=(WORD)AfxGetApp()->GetProfileInt(szSvgExp,szVecsUsed,SVG_VECTOR_CNT_DFLT);
		pAdv->dist_exp=(float)atof(AfxGetApp()->GetProfileString(szSvgExp,szDistExp,SVG_DISTANCE_EXP_DFLT));
		pAdv->dist_off=(float)atof(AfxGetApp()->GetProfileString(szSvgExp,szDistOff,SVG_DISTANCE_OFF_DFLT));
		pAdv->maxCoordChg=(float)atof(AfxGetApp()->GetProfileString(szSvgExp,szMaxCoordChg,SVG_MAXCOORDCHG_DFLT));
		pAdv->len_exp=(float)atof(AfxGetApp()->GetProfileString(szSvgExp,szLenExp,SVG_LENGTH_EXP_DFLT));
		pAdv->len_min=(float)atof(AfxGetApp()->GetProfileString(szSvgExp,szLenMin,SVG_LENGTH_MIN_DFLT));
	}
}

void CSvgAdv::SaveAdvParams(SVG_ADV_PARAMS *pAdv)
{
	AfxGetApp()->WriteProfileInt(szSvgExp,szDecimals,pAdv->decimals);
	AfxGetApp()->WriteProfileInt(szSvgExp,szVecsUsed,pAdv->vecs_used);
	AfxGetApp()->WriteProfileInt(szSvgExp,szFlags,pAdv->uFlags);
	AfxGetApp()->WriteProfileString(szSvgExp,szDistExp,GetFloatStr(pAdv->dist_exp,2));
	AfxGetApp()->WriteProfileString(szSvgExp,szDistOff,GetFloatStr(pAdv->dist_off,4));
	AfxGetApp()->WriteProfileString(szSvgExp,szLenExp,GetFloatStr(pAdv->len_exp,2));
	AfxGetApp()->WriteProfileString(szSvgExp,szLenMin,GetFloatStr(pAdv->len_min,2));
	AfxGetApp()->WriteProfileString(szSvgExp,szMaxCoordChg,GetFloatStr(pAdv->maxCoordChg,2));
}

/////////////////////////////////////////////////////////////////////////////
// CSvgAdv dialog

CSvgAdv::CSvgAdv(SVG_ADV_PARAMS *pAdv,CWnd* pParent /*=NULL*/)
	: CDialog(CSvgAdv::IDD, pParent)
{
	m_pAdv=pAdv;

	m_vector_cnt = GetIntStr(pAdv->vecs_used);
	m_decimals = GetIntStr(pAdv->decimals);
	m_distance_exp = GetFloatStr(pAdv->dist_exp,2);
	m_distance_off = GetFloatStr(pAdv->dist_off,4);
	m_length_exp = GetFloatStr(pAdv->len_exp,2);
	m_minveclen = GetFloatStr(pAdv->len_min,2);
	m_maxCoordChg = GetFloatStr(pAdv->maxCoordChg,2);
	m_bUseLabelPos=(pAdv->uFlags&SVG_ADV_USELABELPOS)!=0;
	m_bUseLineWidths=(pAdv->uFlags&SVG_ADV_USELINEWIDTHS)!=0;
	m_bLrudBars=(pAdv->uFlags&SVG_ADV_LRUDBARS)!=0;
	m_bLrudBoxes=(pAdv->uFlags&SVG_ADV_LRUDBOXES)!=0;
	//{{AFX_DATA_INIT(CSvgAdv)
	//}}AFX_DATA_INIT
}

UINT CSvgAdv::CheckData()
{
	SVG_ADV_PARAMS adv;
	double f;

	if(!CheckFlt(m_distance_exp,&f,0.0,10.0,2)) return IDC_DISTANCE_EXP;
	adv.dist_exp=(float)f;
	if(!CheckFlt(m_distance_off,&f,0.0,10.0,4)) return IDC_DISTANCE_OFF;
	adv.dist_off=(float)f;
	if(!CheckFlt(m_length_exp,&f,0.0,10.0,2)) return IDC_LENGTH_EXP;
	adv.len_exp=(float)f;
	if(!CheckFlt(m_minveclen,&f,0.0,10.0,2)) return IDC_LENGTH_EXP;
	adv.len_min=(float)f;
	if(!CheckFlt(m_maxCoordChg,&f,0.0,10.0,2)) return IDC_MAXCOORDCHG;
	adv.maxCoordChg=(float)f;
	adv.decimals=atoi(m_decimals);
	adv.vecs_used=atoi(m_vector_cnt);
	adv.uFlags=SVG_ADV_USELABELPOS*m_bUseLabelPos+SVG_ADV_LRUDBARS*m_bLrudBars+
		SVG_ADV_LRUDBOXES*m_bLrudBoxes+SVG_ADV_USELINEWIDTHS*m_bUseLineWidths;
	*m_pAdv=adv;
	return 0;
}

void CSvgAdv::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSvgAdv)
	DDX_CBString(pDX, IDC_DECIMALS, m_decimals);
	DDV_MaxChars(pDX, m_decimals, 1);
	DDX_Text(pDX, IDC_DISTANCE_EXP, m_distance_exp);
	DDV_MaxChars(pDX, m_distance_exp, 20);
	DDX_Text(pDX, IDC_DISTANCE_OFF, m_distance_off);
	DDV_MaxChars(pDX, m_distance_off, 20);
	DDX_Text(pDX, IDC_LENGTH_EXP, m_length_exp);
	DDV_MaxChars(pDX, m_length_exp, 20);
	DDX_CBString(pDX, IDC_VECTOR_CNT, m_vector_cnt);
	DDV_MaxChars(pDX, m_vector_cnt, 2);
	DDX_Text(pDX, IDC_MINVECLEN, m_minveclen);
	DDV_MaxChars(pDX, m_minveclen, 10);
	DDX_Text(pDX, IDC_MAXCOORDCHG, m_maxCoordChg);
	DDV_MaxChars(pDX, m_maxCoordChg, 20);
	DDX_Check(pDX, IDC_USELABELPOS, m_bUseLabelPos);
	DDX_Check(pDX, IDC_USELINEWIDTHS, m_bUseLineWidths);
	DDX_Check(pDX, IDC_LRUDBARS, m_bLrudBars);
	//}}AFX_DATA_MAP

	DDX_Check(pDX, IDC_LRUDBOXES, m_bLrudBoxes);

	if(pDX->m_bSaveAndValidate) {
		UINT id=CheckData();
		if(id) {
			pDX->m_idLastControl=id;
			//***7.1 pDX->m_hWndLastControl=GetDlgItem(id)->m_hWnd;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
	} 
}

BEGIN_MESSAGE_MAP(CSvgAdv, CDialog)
	//{{AFX_MSG_MAP(CSvgAdv)
	ON_BN_CLICKED(IDC_DEFAULTS, OnDefaults)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_BN_CLICKED(IDC_LRUDBARS, OnBnClickedLrudbars)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSvgAdv message handlers

void CSvgAdv::OnDefaults() 
{
	m_vector_cnt = GetIntStr(SVG_VECTOR_CNT_DFLT);
	m_decimals = GetIntStr(SVG_DECIMALS_DFLT);
	m_distance_exp = SVG_DISTANCE_EXP_DFLT;
	m_distance_off = SVG_DISTANCE_OFF_DFLT;
	m_length_exp = SVG_LENGTH_EXP_DFLT;
	m_maxCoordChg = SVG_MAXCOORDCHG_DFLT;
	m_bUseLabelPos = SVG_USELABELPOS_DFLT;
	m_bUseLineWidths = SVG_USELINEWIDTHS_DFLT;
	m_bLrudBars = m_bLrudBoxes=FALSE;
	UpdateData(FALSE);
}

LRESULT CSvgAdv::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(23,HELP_CONTEXT);
	return TRUE;
}


void CSvgAdv::OnBnClickedLrudbars()
{
	GetDlgItem(IDC_LRUDBOXES)->EnableWindow(IsDlgButtonChecked(IDC_LRUDBARS));
}

BOOL CSvgAdv::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_LRUDBOXES)->EnableWindow(m_bLrudBars);

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
