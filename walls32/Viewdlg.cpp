// viewdlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "pageview.h"
#include "linkedit.h"
#include "viewdlg.h"
#include "compile.h"
#include "pltfind.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define MAX_FLTALLOW 100000000.0

static CViewDlg *pThisDlg=NULL;
static BOOL bFixMatchCase=FALSE;

/////////////////////////////////////////////////////////////////////////////
// CViewDlg dialog

static char *GetFl(double f)
{
	return GetFloatStr(LEN_SCALE(f),4);
}

static BOOL CheckFl(const char *buf,double *pf,BOOL bNeg)
{
	double bFeet=LEN_SCALE(*pf);
	
	if(!CheckFlt(buf,&bFeet,bNeg?-MAX_FLTALLOW:1.0,MAX_FLTALLOW,4)) return FALSE;
	*pf=LEN_INVSCALE(bFeet);
	return TRUE;
}

CViewDlg::CViewDlg(VIEWFORMAT *pVF,const char *pNameOfOrigin,CWnd* pParent /*=NULL*/)
	: CDialog(CViewDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CViewDlg)
	//}}AFX_DATA_INIT
	
    pThisDlg=this;
    m_bMatchCase=bFixMatchCase;
	m_pVF=pVF;
	m_CenterRef=m_pNameOfOrigin=pNameOfOrigin;
	m_CenterEast=GetFl(pVF->fCenterEast);
	m_CenterNorth=GetFl(pVF->fCenterNorth);
	m_CenterUp=GetFl(pVF->fCenterUp);
	m_PanelView=GetFloatStr(pVF->fPanelView,4);

	double w=(m_bInches=VF_ISINCHES(pVF->bFlags))?1.0:CENTIMETERS_PER_INCH;
	m_FrameWidth = GetFloatStr(pVF->fFrameWidth*w,4);
	m_FrameHeight = GetFloatStr(pVF->fFrameHeight*w,4);
	m_fScaleRatio = pVF->fPanelWidth*INCHES_PER_METER/pVF->fFrameWidth;
	//m_fMetersPerCM = LEN_SCALE(pVF->fPanelWidth)/(pVF->fFrameWidth*w);
	if(LEN_ISFEET()) {
		if(VF_ISINCHES(pVF->bFlags)) m_fMetersPerCM=m_fScaleRatio/12.0;
		else m_fMetersPerCM=m_fScaleRatio/30.48;
	}
	else {
		if(VF_ISINCHES(pVF->bFlags)) m_fMetersPerCM=m_fScaleRatio/(12.0/0.3048);
		else m_fMetersPerCM=m_fScaleRatio/100.0;
	}

	//if(LEN_ISFEET()) m_fMetersPerCM = m_fScaleRatio/(12.0*w);
	//else m_fMetersPerCM = m_fScaleRatio/(100.0*w); 
}

UINT CViewDlg::CheckData()
{
	VIEWFORMAT vf=*m_pVF;
		
	if(!CheckFl(m_CenterEast,&vf.fCenterEast,TRUE)) return IDC_CENTEREAST;
	if(!CheckFl(m_CenterNorth,&vf.fCenterNorth,TRUE)) return IDC_CENTERNORTH;
	if(!CheckFl(m_CenterUp,&vf.fCenterUp,TRUE)) return IDC_CENTERUP;
	if(!CheckFlt(m_PanelView,&vf.fPanelView,0.0,360,4)) return IDC_PANELVIEW;
	if(vf.fPanelView>=359.99995) vf.fPanelView=0.0;

	m_CenterRef.Trim();
    if(strcmp(m_pNameOfOrigin,m_CenterRef) && !m_CenterRef.IsEmpty()) {
      if(!CPrjDoc::GetRefOffsets(m_CenterRef,m_bMatchCase)) return IDC_CENTERREF;

      vf.fCenterEast+=FixXYZ[0];
	  vf.fCenterNorth+=FixXYZ[1];
	  vf.fCenterUp+=FixXYZ[2];
	  hist_pltfind.InsertAsFirst(m_CenterRef);
    }
    
	double pageUnitsPerIn=(m_bInches?1.0:CENTIMETERS_PER_INCH);
	double h=vf.fFrameHeight*pageUnitsPerIn;
	double w=vf.fFrameWidth*pageUnitsPerIn;

	if(!CheckFlt(m_FrameHeight,&h,1.0,MAX_FRAMEWIDTHINCHES*pageUnitsPerIn,4)) return IDC_FRAMEHEIGHT;
	if(!CheckFlt(m_FrameWidth,&w,1.0,MAX_FRAMEWIDTHINCHES*pageUnitsPerIn,4)) return IDC_FRAMEWIDTH;

	if(!m_leScaleRatio.InRange()) UpdateRatioFromMeters();

	vf.fFrameHeight=h/pageUnitsPerIn;
	vf.fFrameWidth=w/pageUnitsPerIn;

	vf.bFlags&=~VF_INCHUNITS;
	vf.bFlags|=m_bInches;
	if(m_bInches) w*=CENTIMETERS_PER_INCH;

	vf.fPanelWidth=w*m_fScaleRatio/100.0;
	
	if(memcmp(&vf,m_pVF,sizeof(VIEWFORMAT))) {
	   vf.bChanged=TRUE;
	   *m_pVF=vf;
	}
	bFixMatchCase=m_bMatchCase;
    return 0;
}

void CViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewDlg)
	DDX_Text(pDX, IDC_CENTEREAST, m_CenterEast);
	DDV_MaxChars(pDX, m_CenterEast, 20);
	DDX_Text(pDX, IDC_CENTERNORTH, m_CenterNorth);
	DDV_MaxChars(pDX, m_CenterNorth, 20);
	DDX_Text(pDX, IDC_CENTERREF, m_CenterRef);
    DDV_MaxChars(pDX, m_CenterRef, 40);
    DDX_Check(pDX, IDC_MATCHCASE, m_bMatchCase);
	DDX_Check(pDX, IDC_SAVEVIEW, m_bSaveView);
	DDX_Text(pDX, IDC_PANELVIEW, m_PanelView);
	DDV_MaxChars(pDX, m_PanelView, 20);
	DDX_Text(pDX, IDC_FRAMEHEIGHT, m_FrameHeight);
	DDV_MaxChars(pDX, m_FrameHeight, 16);
	DDX_Text(pDX, IDC_FRAMEWIDTH, m_FrameWidth);
	DDV_MaxChars(pDX, m_FrameWidth, 16);
	DDX_Text(pDX, IDC_CENTERUP, m_CenterUp);
	DDV_MaxChars(pDX, m_CenterUp, 20);
	//}}AFX_DATA_MAP
	
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

IMPLEMENT_DYNAMIC(CViewDlg,CDialog)

BEGIN_MESSAGE_MAP(CViewDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CViewDlg)
	ON_BN_CLICKED(IDC_INCHES, OnInches)
	ON_BN_CLICKED(IDC_CENTIMETERS, OnInches)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewDlg message handlers

void CViewDlg::RefreshPageUnits()
{
	char buf[30];
	((CWnd *)GetDlgItem(IDC_ST_ONECM))->SetWindowText(m_bInches?"1 in =":"1 cm =");
	pGV->PageDimensionStr(buf,m_bInches);
	GetDlgItem(IDC_ST_PRINTAREA)->SetWindowText(buf);
}

static BOOL upd_meters(int fcn);

static BOOL upd_ratio(int fcn)
{
	if(!fcn) {
		if(pThisDlg->m_leMetersPerCM.InRange()) return TRUE;
		pThisDlg->m_leScaleRatio.UpdateText(TRUE);
	}
	else if(fcn<0) {
		pThisDlg->m_leMetersPerCM.UpdateText(FALSE);
		return TRUE;
	}
	pThisDlg->UpdateMetersFromRatio();
	return TRUE;
}

static BOOL upd_meters(int fcn)
{
	if(fcn==0) {
		if(pThisDlg->m_leScaleRatio.InRange()) return TRUE;
		pThisDlg->m_leMetersPerCM.UpdateText(TRUE);
	}
	else if(fcn<0) {
		pThisDlg->m_leScaleRatio.UpdateText(FALSE);
		return TRUE;
	}
	pThisDlg->UpdateRatioFromMeters();
	return TRUE;
}

void CViewDlg::UpdateRatioFromMeters()
{
    //  ft/in  ratio= mpercm*12
	//  m/in   ratio= mpercm*12/0.3048
	//  ft/cm  ratio= mpercm*30.48
	//  m/cm   ratio= mpercm*100

	double ratio=m_fMetersPerCM;
    if(m_bInches) ratio*=(LEN_ISFEET()?12.0:(12.0/0.3048));
	else ratio*=(LEN_ISFEET()?30.48:100.0);
	m_fScaleRatio=ratio;
	m_leScaleRatio.UpdateText(TRUE);
}

void CViewDlg::UpdateMetersFromRatio()
{
    //  ft/in  ratio/12 = mpercm
	//  m/in   ratio*0.3048/12 = mpercm
	//  ft/cm  ratio/30.48 = mpercm
	//  m/cm   ratio/100 = mpercm

	double div;
    if(m_bInches) div=LEN_ISFEET()?12.0:(12.0/0.3048);
	else div=LEN_ISFEET()?30.48:100.0;

	m_fMetersPerCM=m_fScaleRatio/div;
	m_leMetersPerCM.UpdateText(TRUE);
}

BOOL CViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	char title[60];
	
	LoadCMsg(title,sizeof(title),IDS_VIEWDLGTITLE,m_pVF->bProfile?"Profile":"Plan");
	SetWindowText(title);

	SetCheck(IDC_INCHES,m_bInches);
	SetCheck(IDC_CENTIMETERS,!m_bInches);

	m_leScaleRatio.Init(this,IDC_SCALERATIO,1,100000000,-4,&m_fScaleRatio,0,NULL,upd_ratio);
	m_leMetersPerCM.Init(this,IDC_METERSPERCM,0.01,10000000,-4,&m_fMetersPerCM,0,NULL,upd_meters);

	pGV->RefreshPageInfo();
	RefreshPageUnits();

	sprintf(title,"from %s north",VIEW_UNITSTR());

	GetDlgItem(IDC_ST_TRUEORGRID)->SetWindowText(title);
	GetDlgItem(IDC_ST_METERS)->SetWindowText(LEN_UNITSTR());

	CenterWindow();
	
	CEdit *ce=(CEdit *)GetDlgItem(IDC_PANELVIEW);
	ce->SetSel(0,-1);
	ce->SetFocus();
	
	return FALSE;  //We set the focus to a control
}

LRESULT CViewDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(110,HELP_CONTEXT);
	return TRUE;
}

void CViewDlg::SwitchPageUnits(UINT id,BOOL bInches)
{
    if(id==IDC_METERSPERCM) {
		if(!m_leMetersPerCM.InRange()) UpdateMetersFromRatio();
		else if(!m_leScaleRatio.InRange()) UpdateRatioFromMeters();
		if(bInches) m_fMetersPerCM*=2.54;
		else m_fMetersPerCM/=2.54;
		m_leMetersPerCM.UpdateText(TRUE);
	}
	else {
		double mperc;
		CString s;
		GetDlgItem(id)->GetWindowText(s);
		if(CheckFlt(s,&mperc,0.1,1000000000,4)) {
		  if(bInches) mperc*=2.54;
		  else mperc/=2.54;
		  GetDlgItem(id)->SetWindowText(GetFloatStr(mperc,4));
		}
	}
}

void CViewDlg::OnInches() 
{
	if(m_bInches!=(IsDlgButtonChecked(IDC_INCHES)!=0)) {
	  m_bInches=!m_bInches;
      SwitchPageUnits(IDC_METERSPERCM,m_bInches);
      SwitchPageUnits(IDC_FRAMEWIDTH,!m_bInches);
      SwitchPageUnits(IDC_FRAMEHEIGHT,!m_bInches);
	  RefreshPageUnits();
	}
}

HBRUSH CViewDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
    if(nCtlColor==CTLCOLOR_STATIC && pWnd->GetDlgCtrlID()==IDC_ST_PRINTAREA)
		pDC->SetTextColor(RGB_DKRED);
	return hbr;
}
