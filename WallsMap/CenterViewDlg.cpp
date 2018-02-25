// CenterViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "CenterViewDlg.h"
//#include "afxdialogex.h"


// CCenterViewDlg dialog

IMPLEMENT_DYNAMIC(CCenterViewDlg, CDialog)

CCenterViewDlg::CCenterViewDlg(const CFltPoint &fpt,CWnd* pParent /*=NULL*/)
	: CDialog(CCenterViewDlg::IDD, pParent)
	, m_bZoom(false)
    , m_bInitFlag(false)
	, m_bOutOfZone(false)
	, m_fptOrg(fpt)
	, m_pDoc(((CWallsMapView *)pParent)->GetDocument())
	, m_iNadDoc(m_pDoc->LayerSet().m_iNad)
	, m_iZoneDoc(m_pDoc->LayerSet().m_iZone)
	, m_bNadToggle(m_pDoc->IsGeoRefSupported())
	, m_bMark(TRUE)
{
}

CCenterViewDlg::~CCenterViewDlg()
{
}

void CCenterViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_MARK_PT, m_bMark);
	DDX_Control(pDX, IDC_EDIT_EAST, m_ceEast);
	DDX_Control(pDX, IDC_EDIT_NORTH, m_ceNorth);

	if(pDX->m_bSaveAndValidate) {
		ASSERT(IsLocationValid());
		GetEditedPoint(m_fpt);
		//If necessary, convert displayed coordinates to doc's (or view's) CRS --
		if(m_bNadToggle) {
			//establish format for next centering operation and for selected points --
			m_pDoc->m_bShowAltGeo=(m_bUtmDisplayed != (m_iZoneDoc!=0));
			m_pDoc->m_bShowAltNad=m_iNadSel!=m_iNadDoc;
			if(m_pDoc->m_bShowAltNad || m_pDoc->m_bShowAltGeo || (m_bUtmDisplayed && m_iZoneSel!=m_iZoneDoc))
			{  
				VERIFY(GetConvertedPoint(m_fpt,m_iNadDoc,m_iZoneDoc?&m_iZoneDoc:NULL,m_iNadSel,m_bUtmDisplayed?m_iZoneSel:0));
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CCenterViewDlg, CDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_ZOOM4X, &CCenterViewDlg::OnBnClickedZoom4x)
	ON_EN_CHANGE(IDC_EDIT_NORTH, &CCenterViewDlg::OnEnChangeEditNorth)
	ON_EN_CHANGE(IDC_EDIT_EAST, &CCenterViewDlg::OnEnChangeEditEast)
	//ON_EN_CHANGE(IDC_UTMZONE, OnEnChangeEditZone)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedUndoEdit)
	ON_BN_CLICKED(IDC_COPYLOC, &CCenterViewDlg::OnBnClickedCopyloc)
	ON_BN_CLICKED(IDC_PASTELOC, &CCenterViewDlg::OnBnClickedPasteloc)
	ON_BN_CLICKED(IDC_TYPEGEO, OnBnClickedTypegeo)
	ON_BN_CLICKED(IDC_TYPEUTM, OnBnClickedTypegeo)
	ON_BN_CLICKED(IDC_DATUM1, OnBnClickedDatum)
	ON_BN_CLICKED(IDC_DATUM2, OnBnClickedDatum)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_UTMZONE, OnSpinUtm)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

HBRUSH CCenterViewDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{

	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if(!m_bOutOfZone || !IsLocationValid()) {
		return hbr;
	}
	int id;
	if((id=pWnd->GetDlgCtrlID())==IDC_EDIT_NORTH || id==IDC_EDIT_EAST /*|| id==IDC_ST_OUTOFZONE*/) {
		ASSERT(m_bUtmDisplayed);
		pDC->SetTextColor(RGB(0x80,0,0));
    }
	return hbr;
}

void CCenterViewDlg::SetCoordLabels()
{
	SetText(IDC_ST_EASTING,m_bUtmDisplayed?"Easting":"Latitude");
	SetText(IDC_ST_NORTHING,m_bUtmDisplayed?"Northing":"Longitude");
}

bool CCenterViewDlg::InitFormat()
{
	m_fpt=m_fptOrg;
	if(m_bNadToggle) {
		m_iNadSel=m_pDoc->m_bShowAltNad?(3-m_iNadDoc):m_iNadDoc;
		if(m_pDoc->m_bShowAltGeo)
			m_bUtmDisplayed=(m_iZoneDoc==0);
		else
			m_bUtmDisplayed=(m_iZoneDoc!=0);

		if(!(m_iZoneSel=m_iZoneDoc)) m_iZoneSel=GetZone(m_fpt);
		if(m_bUtmDisplayed!=(m_iZoneDoc!=0) || m_iNadDoc!=m_iNadSel) {
			if(!GetConvertedPoint(m_fpt,m_iNadSel,m_bUtmDisplayed?&m_iZoneSel:0,m_iNadDoc,m_iZoneDoc)) {
				ASSERT(0);
				return false;
			}
		}
	}
	return true;
}

bool CCenterViewDlg::SetCoordFormat()
{
	//Called from OnInitDialog() --

	CString datum;
	m_pDoc->GetDatumName(datum);
	
	if(!m_bNadToggle) {
		m_fpt=m_fptOrg;
		m_bUtmDisplayed=m_pDoc->IsProjected(); //Easting Northing
		m_iNadSel=m_iNadDoc;
		Hide(IDC_DATUM1);
		Hide(IDC_DATUM2);
		Hide(IDC_TYPEGEO);
		Hide(IDC_TYPEUTM);
		Hide(IDC_UTMZONE);
		Hide(IDC_SPIN_UTMZONE);
		Show(IDC_ST_DATUM);
		SetText(IDC_ST_DATUM,datum);
	}
	else {
		bool bOut=false;
		if(m_iZoneDoc) {
			bOut=fabs(m_fptOrg.x)>=10000000.0 || fabs(m_fptOrg.y)>=10000000.0;
			if(!bOut) {
				double lat,lon;
				geo_UTM2LatLon(m_iZoneDoc,m_fptOrg.x,m_fptOrg.y,&lat,&lon,(m_iNadDoc==2)?geo_NAD27_datum:geo_WGS84_datum);
				bOut=abs(abs(GetZone(lat,lon))-abs(m_iZoneDoc))>2;
			}
		}

		if(bOut || !InitFormat()) {
			AfxMessageBox("Point is too far outside the range of the current map projection.");
			return false;
		}

		Show(IDC_DATUM1);
		Show(IDC_DATUM2);
		Show(IDC_TYPEGEO);
		Show(IDC_TYPEUTM);
		Show(IDC_UTMZONE);
		Hide(IDC_ST_DATUM);
		CheckRadioButton(IDC_DATUM1,IDC_DATUM2,(m_iNadSel==1)?IDC_DATUM1:IDC_DATUM2);
		CheckRadioButton(IDC_TYPEGEO,IDC_TYPEUTM,m_bUtmDisplayed?IDC_TYPEUTM:IDC_TYPEGEO);
	}
	SetCoordLabels();
	return true;
}

BOOL CCenterViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(!SetCoordFormat()) {
		OnCancel();
		return true;
	}
	RefreshCoordinates(m_fpt);
	Disable(IDC_RESTORE);

	GetDlgItem(IDOK)->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
}

void CCenterViewDlg::FixZoneMsg(int zone)
{
	Show(IDC_ST_OUTOFZONE,m_bOutOfZone);
	if(m_bOutOfZone) {
		ASSERT(m_bUtmDisplayed && zone);
		CString s;
		s.Format("In UTM zone %d",zone);
		SetText(IDC_ST_OUTOFZONE,s);
	}
	GetDlgItem(IDC_EDIT_EAST)->Invalidate();
	GetDlgItem(IDC_EDIT_NORTH)->Invalidate();
}

void CCenterViewDlg::SetOutOfZone()
{
	//assumes m_f0, m_f1, and m_iZoneSel set appropriately
	int zone=0;
	m_bOutOfZone=false;
	if(!m_bUtmDisplayed) {
		m_iZoneSel=GetTrueZone();
	}
	else {
		zone=GetTrueZone();
		m_bOutOfZone=(zone!=m_iZoneSel);
	}
	FixZoneMsg(zone);
	SetText(IDC_UTMZONE,GetIntStr(m_iZoneSel));
}

void CCenterViewDlg::RefreshCoordinates(const CFltPoint &fpt)
{
	//Display fpt according to current settings 

	char buf[32];
	buf[31]=0;

	m_f0=m_bUtmDisplayed?fpt.x:fpt.y;
	m_f1=m_bUtmDisplayed?fpt.y:fpt.x;

	if(m_bNadToggle) {
		Show(IDC_SPIN_UTMZONE,m_bUtmDisplayed);
		SetOutOfZone();
	}

	m_bInitFlag=true;
	_snprintf(buf,31,m_bUtmDisplayed?"%.2f":"%.7f",m_f0);
	SetText(IDC_EDIT_EAST,buf);
	_snprintf(buf,31,m_bUtmDisplayed?"%.2f":"%.7f",m_f1);
	SetText(IDC_EDIT_NORTH,buf);
	m_bInitFlag=false;
	m_bValidEast=m_bValidNorth=m_bValidZone=true;
}

void CCenterViewDlg::GetEditedPoint(CFltPoint &fpt)
{
	//Set fpt to edited value while preserving displayed datum and geo/utm type --
	ASSERT(IsLocationValid());

	CString s;

	//Dialog:
	//Easting		Northing
	//Latitude		Longitude
	//IDC_EDIT_EAST	IDC_EDIT_NORTH

	GetText(IDC_EDIT_EAST,s);
	VERIFY(GetCoordinate(s,m_f0,true,m_bUtmDisplayed));
	GetText(IDC_EDIT_NORTH,s);
	VERIFY(GetCoordinate(s,m_f1,false,m_bUtmDisplayed));

	if(m_bUtmDisplayed) {
		fpt.x=m_f0;
		fpt.y=m_f1;
	}
	else {
		fpt.x=m_f1;
		fpt.y=m_f0;
	}
}

// CCenterViewDlg message handlers
void CCenterViewDlg::OnBnClickedTypegeo()
{
	if(m_bUtmDisplayed!=IsChecked(IDC_TYPEGEO))
		return;
	ASSERT(m_bNadToggle && IsLocationValid());
	CFltPoint fpt;
	GetEditedPoint(fpt); //before utm/geo change

	if(!GetConvertedPoint(fpt,m_iNadSel,m_bUtmDisplayed?0:&m_iZoneSel,m_iNadSel,m_bUtmDisplayed?m_iZoneSel:0)) {
		ASSERT(0);
		return;
	}

	m_bUtmDisplayed=!m_bUtmDisplayed;

	SetCoordLabels();
	RefreshCoordinates(fpt);
	Enable(IDC_RESTORE);
}

void CCenterViewDlg::OnBnClickedDatum()
{
	ASSERT(m_iNadSel==(IsChecked(IDC_DATUM1)?2:1));
	ASSERT(m_bNadToggle && IsLocationValid());
	CFltPoint fpt;
	GetEditedPoint(fpt); //before datum change

	if(!GetConvertedPoint(fpt,3-m_iNadSel,m_bUtmDisplayed?&m_iZoneSel:0,m_iNadSel,m_bUtmDisplayed?m_iZoneSel:0)) {
		ASSERT(0);
		return;
	}
	m_iNadSel=3-m_iNadSel;
	//m_pDoc->m_bShowAltNad=(m_iNadSel!=m_iNadDoc);

	RefreshCoordinates(fpt);
	Enable(IDC_RESTORE);
}

void CCenterViewDlg::OnSpinUtm(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ASSERT(m_bNadToggle && m_bUtmDisplayed);
	ASSERT(IsLocationValid());

	*pResult=TRUE; //dom't allow change

	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	char buf[12];
	GetText(IDC_UTMZONE,buf,12);
	int zone=atoi(buf);
	ASSERT(zone && zone>=-60 && zone<=60);
	zone-=pNMUpDown->iDelta;
	if(!zone) zone-=pNMUpDown->iDelta;
	else if(zone>60) zone=-60;
	else if(zone<-60) zone=60;
	if(m_iZoneDoc && abs(abs(zone)-abs(m_iZoneDoc))>2) return;
	CFltPoint fpt;
	GetEditedPoint(fpt);
	if(!m_iZoneDoc) {
		int tzone=GetTrueZone();
		if(abs(abs(zone)-abs(tzone))>2) return;
	}
	if(!GetConvertedPoint(fpt,m_iNadSel,&zone,m_iNadSel,m_iZoneSel)) {
		return;
	}
	m_iZoneSel=zone;
	RefreshCoordinates(fpt);
	Enable(IDC_RESTORE);
	*pResult = FALSE; //allow change
}

void CCenterViewDlg::OnBnClickedZoom4x()
{
	m_bZoom=true;
	OnOK();
}

int CCenterViewDlg::GetTrueZone()
{
	if(m_bUtmDisplayed) {
		double lat,lon;
		geo_UTM2LatLon(m_iZoneSel,m_f0,m_f1,&lat,&lon,(m_iNadSel==2)?geo_NAD27_datum:geo_WGS84_datum);
		return GetZone(lat,lon);
	}
	return GetZone(m_f0,m_f1);
}

void CCenterViewDlg::OnEnChangeEditCoord(bool bValid)
{
	int zone=0;
	bool bZoneOK=true;

	if(bValid) {
		zone=GetTrueZone();
		if(m_bNadToggle && m_iZoneDoc && abs(abs(zone)-abs(m_iZoneDoc))>2)
			bZoneOK=false;
	}

	bValid=(bValid && bZoneOK);

	if(m_bNadToggle && (bValid && bZoneOK) != IsLocationValid()) {
		Enable(IDC_DATUM1,bValid);
		Enable(IDC_DATUM2,bValid);
		Enable(IDC_TYPEGEO,bValid);
		Enable(IDC_TYPEUTM,bValid);
		Enable(IDC_UTMZONE,m_bUtmDisplayed && bValid);
		Show(IDC_SPIN_UTMZONE,m_bUtmDisplayed && bValid);
	}
	m_bValidZone=bZoneOK;
	Enable(IDC_COPYLOC,bValid);
	Enable(IDC_ZOOM4X,bValid);
	Enable(IDOK,bValid);
	Enable(IDC_RESTORE);
	if(!m_bNadToggle) return;

	m_bOutOfZone=false;

	if(bValid) {
		if(m_bUtmDisplayed) {
			m_bOutOfZone=(zone!=m_iZoneSel);
		}
		else if(zone!=m_iZoneSel) {
			SetText(IDC_UTMZONE,GetIntStr(m_iZoneSel=zone));
		}
	}
	FixZoneMsg(zone);
}

void CCenterViewDlg::OnEnChangeEditEast()
{
	if(m_bInitFlag) return;

	CString s;
	GetText(IDC_EDIT_EAST,s);
	bool bValid=GetCoordinate(s,m_f0,true,m_bUtmDisplayed);
	OnEnChangeEditCoord(bValid && m_bValidNorth);
	m_bValidEast=bValid;
}

void CCenterViewDlg::OnEnChangeEditNorth()
{
	if(m_bInitFlag) return;

	CString s;
	GetText(IDC_EDIT_NORTH,s);
	bool bValid=GetCoordinate(s,m_f1,false,m_bUtmDisplayed);
	OnEnChangeEditCoord(bValid && m_bValidEast);
	m_bValidNorth=bValid;
}

void CCenterViewDlg::OnBnClickedUndoEdit()
{
	InitFormat();
	RefreshCoordinates(m_fpt);
	if(m_bNadToggle) {
		Enable(IDC_DATUM1,true);
		Enable(IDC_DATUM2,true);
		Enable(IDC_TYPEGEO,true);
		Enable(IDC_TYPEUTM,true);
		Show(IDC_SPIN_UTMZONE,true);
		Enable(IDC_UTMZONE,m_bUtmDisplayed);
	}
	Disable(IDC_RESTORE);
	Enable(IDC_COPYLOC);
	Enable(IDC_ZOOM4X);
	Enable(IDOK);
}

void CCenterViewDlg::OnBnClickedCopyloc()
{
	ASSERT(IsLocationValid());
	CString sEast;
	GetDlgItem(IDC_EDIT_EAST)->GetWindowText(sEast);
	sEast.Trim();
	sEast.AppendChar('\t');
	CString sNorth;
	GetDlgItem(IDC_EDIT_NORTH)->GetWindowText(sNorth);
	sNorth.Trim();
	sEast+=sNorth;
	ClipboardCopy(sEast);
}

void CCenterViewDlg::OnBnClickedPasteloc()
{
	CString csPaste;
	if(ClipboardPaste(csPaste,42)) {
		CString s(csPaste);
		bool butm=m_bUtmDisplayed;
		int ix=CheckCoordinatePair(s,butm,&m_f0,&m_f1);
		if(!ix || !m_bNadToggle && butm!=m_bUtmDisplayed)
			goto _errmsg;

		ElimSlash(csPaste);

		if(m_bNadToggle && !butm && m_iZoneDoc && abs(abs(GetZone(m_f0,m_f1))-abs(m_iZoneDoc))>2)
			goto _errmsg;

		Show(IDC_ST_OUTOFZONE,m_bOutOfZone=false);

		cfg_quotes=FALSE;
		int n=cfg_GetArgv((LPSTR)(LPCSTR)csPaste,CFG_PARSE_ALL);
		cfg_quotes=TRUE;
		ASSERT(n>=2 && n<=8 && (n&1)==0);
		n>>=1;
		s=cfg_argv[0];
		for(int i=1;i<n;i++) {
			s+=' '; s+=cfg_argv[i];
		}
		SetText((ix>0)?IDC_EDIT_EAST:IDC_EDIT_NORTH,s);
		s=cfg_argv[n];
		for(int i=n+1;i<2*n;i++) {
			s+=' '; s+=cfg_argv[i];
		}
		SetText((ix>0)?IDC_EDIT_NORTH:IDC_EDIT_EAST,s);

		if(m_bNadToggle) {
			int zone=0;
			m_bOutOfZone=false;
			if(!butm) {
				//have just pasted valid lat/long in boxes -- m_f0/m_f1 are lat/long
				m_iZoneSel=GetZone(m_f0,m_f1);
				SetText(IDC_UTMZONE,GetIntStr(m_iZoneSel));
				if(m_bUtmDisplayed) {
					//auto switch to lat/long!
					m_bUtmDisplayed=false;
					//SetRO(IDC_UTMZONE,TRUE); //always RO
					Show(IDC_SPIN_UTMZONE,FALSE);
					CheckRadioButton(IDC_TYPEGEO,IDC_TYPEUTM,IDC_TYPEGEO);
					SetCoordLabels();
				}
				FixZoneMsg(zone);
			}
			else {
				//have just pasted valid utm in boxes -- m_f0 and m_f1 are easting and northing
				if(!m_bUtmDisplayed) {
					//auto switch to utm, keeping iZoneSel (possibly out-of-zone)!
					m_bUtmDisplayed=true;
					Show(IDC_SPIN_UTMZONE,TRUE);
					CheckRadioButton(IDC_TYPEGEO,IDC_TYPEUTM,IDC_TYPEUTM);
					SetCoordLabels();
					zone=GetTrueZone(); //was zone=m_iZoneSel;
					m_bOutOfZone=zone!=m_iZoneSel;
					FixZoneMsg(zone); //if m_bOutOfZone, display true zone==zone
				}
				else {
					//we are updating utm coords.
					//get zone corrseponding to m_f0,m_f1 == easting,northing
					zone=GetTrueZone(); //assumes iZoneSel is selected zone
					m_bOutOfZone=zone!=m_iZoneSel;
					FixZoneMsg(zone); //if m_bOutOfZone, display true zone==zone
				}
			}
		}

		m_bValidEast=m_bValidNorth=m_bValidZone=true;
		Enable(IDC_COPYLOC);
		Enable(IDC_ZOOM4X);
		Enable(IDOK);
		Enable(IDC_RESTORE);
		return;
	}

_errmsg:
	//failure msg --
	AfxMessageBox("The clipboard's content is not a cooordinate pair "
		"or is outside the range of the view's coordinate system.");
}

LRESULT CCenterViewDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1010);
	return TRUE;
}
