// MagDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "MagDlg.h"
#include "Dialogs.h"
#include <gpslib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//Globals --
MAGDATA app_MagData;
CMagDlg * app_pMagDlg = NULL;

#define MAX_NORTHING  10060000.0
#define MIN_NORTHING  0
#define MAX_EASTING   3000000000.0 //2,620,844,264.475
#define MIN_EASTING  -3000000000.0

enum { TO_UPS_UP, TO_UPS_DN, TO_UTM_UP, TO_UTM_DN, TO_NORTH, TO_SOUTH };

/////////////////////////////////////////////////////////////////////////////
// CMagDlg dialog


CMagDlg::CMagDlg(CWnd* pParent, bool bVecInit /*=false*/)
	: CDialog()
{
	//{{AFX_DATA_INIT(CMagDlg)
	//}}AFX_DATA_INIT
	m_bModeless = true;
	m_bVecInit = bVecInit;
	CLinkEdit::m_pFocus = NULL;
	CLinkEdit::m_bSetting = 0;
	m_pTitle = NULL;
	m_bDisplayDEG = m_bDirEnabled = TRUE;
	Create(CMagDlg::IDD, pParent);
}

CMagDlg::CMagDlg()
	: CDialog(CMagDlg::IDD, NULL)
{
	m_bVecInit = true;
	m_bModeless = false;
	CLinkEdit::m_pFocus = NULL;
	CLinkEdit::m_bSetting = 0;
	m_bDisplayDEG = m_bDirEnabled = TRUE;
}

CMagDlg::~CMagDlg()
{
	app_pMagDlg = NULL;
}

BEGIN_MESSAGE_MAP(CMagDlg, CDialog)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_FMT_D, OnFmtD)
	ON_BN_CLICKED(IDC_FMT_DM, OnFmtDm)
	ON_BN_CLICKED(IDC_FMT_DMS, OnFmtDms)
	ON_CBN_SELCHANGE(IDC_DATUM, OnSelchangeDatum)
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_BN_CLICKED(IDC_DISCARD, OnDiscard)
	ON_BN_CLICKED(IDC_LONDIR, OnLonDir)
	ON_BN_CLICKED(IDC_LATDIR, OnLatDir)
	ON_BN_CLICKED(IDC_ENTERKEY, OnEnterKey)
	ON_MESSAGE(ME_COPYCOORDS, OnCopyCoords)
	ON_MESSAGE(ME_PASTECOORDS, OnPasteCoords)
	ON_BN_CLICKED(IDC_UTMUPS, OnBnClickedUtmUps)
	ON_BN_CLICKED(IDC_ZONE_INC, OnZoneInc)
	ON_BN_CLICKED(IDC_ZONE_DEC, OnZoneDec)
END_MESSAGE_MAP()

void CMagDlg::DoDataExchange(CDataExchange* pDX)
{
	// TODO: Add your specialized code here and/or call the base class
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_UTMUPS, m_btnUTMUPS);
	DDX_Control(pDX, IDC_ZONE_INC, m_zoneInc);
	DDX_Control(pDX, IDC_ZONE_DEC, m_zoneDec);
}

bool CMagDlg::ErrorOK(CLinkEdit *pEdit, CWnd* pNewWnd)
{
	//Focus is transferring to new control with a range error in pEdit --
	if (!pNewWnd || pNewWnd->GetParent() != this)
		return true;
	switch (pNewWnd->GetDlgCtrlID()) {
	case IDC_DISCARD:
	case IDCANCEL: return true;
	}
	return false;
}

static BOOL upd_UTM0(int fcn)
{
	//callback to indicate UTM or zone edited
	return app_pMagDlg->upd_UTM(fcn);
}

static BOOL upd_DEG0(int fcn)
{
	return app_pMagDlg->upd_DEG(fcn);
}

static BOOL upd_ZONE0(int fcn)
{
	return app_pMagDlg->upd_ZONE(fcn);
}

static BOOL upd_DECL0(int fcn)
{
	return app_pMagDlg->upd_DECL(fcn);
}

void CMagDlg::EnableDIR(BOOL bEnable)
{
	if (m_bDirEnabled == bEnable) return;
	Enable(IDC_LATDIR, bEnable);
	Enable(IDC_LONDIR, bEnable);
	Enable(IDC_DATUM, bEnable);
	Enable(IDC_FMT_DMS, bEnable);
	Enable(IDC_FMT_DM, bEnable);
	Enable(IDC_FMT_D, bEnable);
	m_bDirEnabled = bEnable;
}

void CMagDlg::ShowZoneLetter()
{
	bool bSwap = m_iOutOfZone * m_MD.zone < 0;
	SetText(IDC_ST_ZONECHAR, (!bSwap && m_MD.lat.isgn < 0 || bSwap && m_MD.lat.isgn>0) ? "S" : "N");
}

void CMagDlg::DisplayLatDir()
{
	ASSERT(m_bDirEnabled);
	SetText(IDC_LATDIR, (m_MD.lat.isgn < 0) ? "South" : "North");
	ShowZoneLetter();
}

void CMagDlg::DisplayLonDir()
{
	ASSERT(m_bDirEnabled);
	SetText(IDC_LONDIR, (m_MD.lon.isgn < 0) ? "West" : "East");
}

void CMagDlg::DisplayDECL(BOOL bValid)
{
	if (!bValid) {
		EnableDIR(FALSE);
		SetText(IDC_DECLINATION, "");
		SetText(IDC_CONVERGENCE, "");
		SetText(IDC_INTENSITY, "");
		m_bValidDECL = FALSE;
		return;
	}
	if (!m_bValidDECL) {
		EnableDIR(TRUE);
		m_bValidDECL = TRUE;
	}
	SetFlt(IDC_CONVERGENCE, m_MD.conv, -2);

	if (m_MD.modnum > 0) {
		CString s;
		s.Format("%.3f", m_MD.decl);
		SetText(IDC_DECLINATION, s);
		s.Format("%.0f nT", mag_HorzIntensity);
		SetText(IDC_INTENSITY, s);
	}
	else {
		SetText(IDC_DECLINATION, "?");
	}
}

void CMagDlg::DisplayUTM(BOOL bShow)
{
	m_east.UpdateText(bShow);
	m_north.UpdateText(bShow);
}

void CMagDlg::DisplayDEG(BOOL bShow)
{
	m_bValidDEG = TRUE;
	EnableDIR(bShow);
	if (bShow) {
		DisplayLatDir();
		DisplayLonDir();
	}

	Enable(IDC_SAVE, bShow && !m_iOutOfZone);

	SetText(IDC_ST_LATLONZONE, ZoneStr(m_iOutOfZone ? m_iOutOfZone : m_MD.zone));
	if (!bShow) {
		Show(IDC_UTMUPS, 0);
	}
	Show(IDC_ST_INVALID, !bShow);

	if (!m_MD.format) {
		m_latdegF.UpdateText(bShow);
		m_londegF.UpdateText(bShow);
	}
	else {
		m_latdeg.UpdateText(bShow);
		m_londeg.UpdateText(bShow);
		if (m_MD.format == 1) {
			m_latminF.UpdateText(bShow);
			m_lonminF.UpdateText(bShow);
		}
		else {
			m_latmin.UpdateText(bShow);
			m_lonmin.UpdateText(bShow);
			m_latsecF.UpdateText(bShow);
			m_lonsecF.UpdateText(bShow);
		}
	}
	m_bDisplayDEG = bShow;
}

BOOL CMagDlg::upd_UTM(int fcn)
{
	if (!fcn) {
		if (m_bValidDEG || !m_bValidUTM)	return TRUE;
		CLinkEdit::m_pFocus = NULL; //update all fields
		fcn = 1;
	}

	if (fcn > 0) {
		fcn = m_MF(MAG_CVTUTM2LL2);
	}

	if (fcn && fcn != MAG_ERR_OUTOFZONE) {
		/*The newly entered value is out-of-range --*/
		m_bValidUTM = FALSE;
		DisplayDEG(FALSE);
		DisplayDECL(FALSE);
		return TRUE;
	}
	m_iOutOfZone = (fcn ? m_MD.zone2 : 0);
	m_bValidUTM = TRUE;
	DisplayDEG(TRUE);
	DisplayZONE();
	DisplayDECL(TRUE);
	return TRUE;
}

BOOL CMagDlg::CvtD2UTM()
{
	ASSERT(m_bValidDEG);
	bool bUPS = false;

	ASSERT(!m_iOutOfZone || !m_bUPS);

	if (m_bUPS) {
		ASSERT(abs(MD.zone) == 61);
		double lat = getDEG(&m_MD.lat, m_MD.format);
		bUPS = (lat >= 83.5 || lat <= -79.5);
	}

	int fcn = m_MF((m_iOutOfZone || bUPS) ? MAG_CVTLL2UTM3 : MAG_CVTLL2UTM);
	if (fcn && fcn != MAG_ERR_OUTOFZONE) {
		SetDegInvalid();
		return FALSE;
	}
	m_iOutOfZone = (fcn ? m_MD.zone2 : 0);
	m_bValidUTM = TRUE;
	DisplayDEG(TRUE);
	DisplayUTM(TRUE);
	DisplayZONE();
	DisplayDECL(TRUE);
	return TRUE;
}

void CMagDlg::SetDegInvalid()
{
	ASSERT(0);
	if (m_bValidDEG) {
		DisplayUTM(FALSE);
		DisplayDECL(FALSE);
		Show(IDC_UTMUPS, 0);
		Show(IDC_ST_INVALID, 1);
		Enable(IDC_SAVE, FALSE);
		m_bValidDEG = FALSE;
	}
}

BOOL CMagDlg::upd_DEG(int fcn)
{
	/*
		We are editing a DMS control or the zone --

		fcn==0	- The control is aquiring focus.
				  CLinkEdit::m_pFocus is the CLinkEdit control that
				  previously had control.

		fcn!=0	- The the user has changed the control's contents,
				  in which case the m_MD.x field has been updated.
				  CLinkEdit::m_pFocus is the current CLinkEdit control.

		fcn==-1	- The control's value is out of range.

		fcn==1	- The control's value is inside range
	*/

	if (!fcn) {
		if (m_bValidDEG && (!m_bValidUTM || m_iOutOfZone)) {
			m_iOutOfZone = 0;
			CLinkEdit::m_pFocus = NULL; //update all fields
			VERIFY(CvtD2UTM());
		}
		return TRUE;
	}

	if (fcn == 1) {
		m_iOutOfZone = 0; //for CvtD2UTM()
		m_bValidDEG = TRUE;
	}

	if (fcn < 1 || !CvtD2UTM()) {
		/*New newly entered value is out-of-range --*/
		if (fcn < 1) SetDegInvalid();
		Enable(IDC_SAVE, FALSE);
	}
	return TRUE;
}

BOOL CMagDlg::upd_DECL(int fcn)
{
	if (!fcn) {
		return TRUE;
	}

	if (fcn < 1) {
		/*New newly entered value is out-of-range --*/
		if (m_bValidDECL) DisplayDECL(FALSE);
		return TRUE;
	}

	m_MF(MAG_CVTDATE);
	DisplayDECL(TRUE);
	return TRUE;
}

void CMagDlg::SetFormat(UINT id)
{
	//0 - degreesF
	//1 - degrees, minutesF
	//2 - degrees, minutes, secondsF

	m_MF(MAG_FIXFORMAT);
	m_MD.format = id;
	Show(IDC_ST_SEC, (id == 2));
	Show(IDC_ST_MIN, !(id == 0));

	Show(IDC_LATDEG10, (id == 0));
	Show(IDC_LATMIN10, (id == 1));
	Show(IDC_LATSEC10, (id == 2));
	Show(IDC_LATDEG, !(id == 0));
	Show(IDC_SPIN_LATDEG, !(id == 0));
	Show(IDC_LATMIN, (id == 2));
	Show(IDC_SPIN_LATMIN, (id == 2));

	Show(IDC_LONDEG10, (id == 0));
	Show(IDC_LONMIN10, (id == 1));
	Show(IDC_LONSEC10, (id == 2));
	Show(IDC_LONDEG, !(id == 0));
	Show(IDC_SPIN_LONDEG, !(id == 0));
	Show(IDC_LONMIN, (id == 2));
	Show(IDC_SPIN_LONMIN, (id == 2));
	if (id == 2) {
		id = IDC_LATSEC10;
		m_latdeg.m_pLinkR = &m_latmin;
		m_londeg.m_pLinkR = &m_lonmin;
	}
	else {
		m_latdeg.m_pLinkR = &m_latminF;
		m_londeg.m_pLinkR = &m_lonminF;
		id = id ? IDC_LATMIN10 : IDC_LATDEG10;
	}
	DisplayDEG(m_bDisplayDEG);
	//GetDlgItem(id)->SetFocus(); //avoid since it clears out-of-zone
}

void CMagDlg::ToggleFormat(UINT id)
{
	((CButton *)GetDlgItem(IDC_FMT_DMS))->SetCheck(id == 2);
	((CButton *)GetDlgItem(IDC_FMT_DM))->SetCheck(id == 1);
	((CButton *)GetDlgItem(IDC_FMT_D))->SetCheck(id == 0);
	SetFormat(id);
}

/////////////////////////////////////////////////////////////////////////////
// CMagDlg message handlers

BOOL CMagDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (m_pTitle) SetWindowText(m_pTitle);

	m_btnUTMUPS.LoadBitmapA(IDB_UTM_UPS, 6, 2);

	m_MD = MD;

	int fcn = m_bVecInit ? MAG_CVTLL2UTM3 : MAG_CVTLL2UTM;

	if ((fcn = m_MF(fcn)) && fcn != MAG_ERR_OUTOFZONE) {
		ASSERT(0);
		CMainFrame::InitRef(&CMainFrame::m_reflast, &m_MD);
		fcn = m_MF(MAG_CVTLL2UTM); //recomputes UTM and zone plus conv and decl
		ASSERT(!fcn);
	}
	m_iOutOfZone = (fcn ? m_MD.zone2 : 0);

	UINT n = m_MD.datum;
	CComboBox *pLB = (CComboBox *)GetDlgItem(IDC_DATUM);

	for (m_MD.datum = 0; m_MF(MAG_GETDATUMNAME); m_MD.datum++) {
		pLB->InsertString(m_MD.datum, m_MD.modname);
	}
	pLB->SetCurSel(m_MD.datum = n);

	n = (m_MD.format == 2) ? IDC_FMT_DMS : (m_MD.format ? IDC_FMT_DM : IDC_FMT_D);
	((CButton *)GetDlgItem(n))->SetCheck(TRUE);

	if (m_MD.lat.isgn < 0) DisplayLatDir();
	if (m_MD.lon.isgn > 0) DisplayLonDir();

	CLinkEdit::m_bNoContext = false;
	CLinkEdit::m_pFocus = NULL;

	m_bValidDECL = m_bValidDEG = m_bValidUTM = TRUE;

	m_year.Init(this, IDC_YEAR, MIN_YEAR, MAX_YEAR, 5, &m_MD.y, NULL, NULL, upd_DECL0, this);
	m_month.Init(this, IDC_MONTH, 1, 12, 3, &m_MD.m, &m_year, NULL, upd_DECL0, this);

	m_elev.Init(this, IDC_ELEVATION, -1000, 600000, 6, &m_MD.elev, NULL, NULL, upd_DECL0, this);

	m_latdeg.Init(this, IDC_LATDEG, 0, 90, 3, &m_MD.lat.ideg, NULL, &m_latmin, upd_DEG0, this);
	m_londeg.Init(this, IDC_LONDEG, 0, 180, 4, &m_MD.lon.ideg, NULL, &m_lonmin, upd_DEG0, this);
	m_latmin.Init(this, IDC_LATMIN, 0, 59, 3, &m_MD.lat.imin, &m_latdeg, &m_latsecF, upd_DEG0, this);
	m_lonmin.Init(this, IDC_LONMIN, 0, 59, 3, &m_MD.lon.imin, &m_londeg, &m_lonsecF, upd_DEG0, this);

	m_latdegF.Init(this, IDC_LATDEG10, 0, 90, 7, &m_MD.lat.fdeg, BTYP_FDEG, NULL, upd_DEG0, this);
	m_londegF.Init(this, IDC_LONDEG10, 0, 180, 7, &m_MD.lon.fdeg, BTYP_FDEG, NULL, upd_DEG0, this);
	m_latminF.Init(this, IDC_LATMIN10, 0, 60 - DBL_MIN, 6, &m_MD.lat.fmin, BTYP_FMIN, &m_latdeg, upd_DEG0, this);
	m_lonminF.Init(this, IDC_LONMIN10, 0, 60 - DBL_MIN, 6, &m_MD.lon.fmin, BTYP_FMIN, &m_londeg, upd_DEG0, this);
	m_latsecF.Init(this, IDC_LATSEC10, 0, 60 - DBL_MIN, 5, &m_MD.lat.fsec, BTYP_FSEC, &m_latmin, upd_DEG0, this);
	m_lonsecF.Init(this, IDC_LONSEC10, 0, 60 - DBL_MIN, 5, &m_MD.lon.fsec, BTYP_FSEC, &m_lonmin, upd_DEG0, this);

	m_zone.SetZoneType();
	m_zone.Init(this, IDC_ZONE, 1, 60, 2, &m_MD.zone, NULL, NULL, upd_ZONE0, this);

	m_east.Init(this, IDC_UTMEAST, MIN_EASTING, MAX_EASTING, 3, &m_MD.east, BTYP_FUTM, NULL, upd_UTM0, this);
	m_north.Init(this, IDC_UTMNORTH, MIN_NORTHING, MAX_NORTHING, 3, &m_MD.north, BTYP_FUTM, NULL, upd_UTM0, this);

	m_syear.Init(this, IDC_SPIN_YEAR, &m_year);
	m_smonth.Init(this, IDC_SPIN_MONTH, &m_month);
	m_slatdeg.Init(this, IDC_SPIN_LATDEG, &m_latdeg);
	m_slatmin.Init(this, IDC_SPIN_LATMIN, &m_latmin);
	m_slondeg.Init(this, IDC_SPIN_LONDEG, &m_londeg);
	m_slonmin.Init(this, IDC_SPIN_LONMIN, &m_lonmin);

	SetFormat(m_MD.format);
	DisplayZONE();
	DisplayDECL(m_bValidDECL);

	if (!m_bModeless) {
		Show(IDCANCEL, FALSE);
		Show(IDC_SAVE, TRUE);
		Show(IDC_DISCARD, TRUE);
	}

	CenterWindow(CWnd::GetDesktopWindow());
	if (m_bModeless) SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	GetDlgItem(m_bModeless ? IDCANCEL : IDC_DISCARD)->SetFocus();
	return FALSE;
	// return TRUE unless you set the focus to a control
}

void CMagDlg::OnClose()
{
	if (m_bModeless) DestroyWindow();
	else EndDialog(IDCANCEL);
}

void CMagDlg::PostNcDestroy()
{
	if (m_bValidDEG && !m_iOutOfZone) {
		CMainFrame::GetRef(&CMainFrame::m_reflast);
	}
	if (m_bModeless) {
		ASSERT(app_pMagDlg == this);
		delete this;
	}
}

void CMagDlg::OnCancel()
{
	if (m_bModeless) DestroyWindow();
	else EndDialog(IDCANCEL);
}

HBRUSH CMagDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_STATIC) {
		switch (pWnd->GetDlgCtrlID()) {
		case IDC_ST_ZONECHAR:
			if (m_iOutOfZone*m_MD.zone < 0) goto _setRed;
			break;

		case IDC_ST_EASTING:
		case IDC_ST_NORTHING:
		{
			double lat = getDEG(&m_MD.lat, m_MD.format);
			if ((lat < 84.0 && lat >= -80.0) == (abs(m_MD.zone) < 61)) break;
		}

		//case IDC_ST_LATLONZONE:
		case IDC_ST_INVALID:
		case IDC_CONVERGENCE:
		case IDC_DECLINATION:
		case IDC_INTENSITY:
			goto _setRed;
		}
	}
	else if (m_iOutOfZone && nCtlColor == CTLCOLOR_EDIT) {
		/*
		case IDC_LATDEG:
		case IDC_LONDEG:
		case IDC_LATMIN:
		case IDC_LONMIN:
		case IDC_LATDEG10:
		case IDC_LONDEG10:
		case IDC_LATMIN10:
		case IDC_LONMIN10:
		case IDC_LATSEC10:
		case IDC_LONSEC10: break;
		*/
		if (pWnd->GetDlgCtrlID() == IDC_ZONE) {
			if (abs(m_MD.zone) != abs(m_MF(MAG_GETUTMZONE)))
				goto _setRed;
		}
	}
	return hbr;

_setRed:
	pDC->SetTextColor((nCtlColor == CTLCOLOR_STATIC) ? RGB_DKRED : RGB_RED);
	return hbr;
}

void CMagDlg::OnFmtD()
{
	SetFormat(0);
}

void CMagDlg::OnFmtDm()
{
	SetFormat(1);
}

void CMagDlg::OnFmtDms()
{
	SetFormat(2);
}

void CMagDlg::OnSelchangeDatum()
{
	if (m_bValidDEG && m_bValidUTM) {
		int datum = ((CComboBox *)GetDlgItem(IDC_DATUM))->GetCurSel();
		m_MF(MAG_CVTDATUM + datum);
		m_bValidUTM = TRUE;
		DisplayDEG(TRUE);
		DisplayUTM(TRUE);
		DisplayDECL(TRUE);
		DisplayZONE();
	}
}

void CMagDlg::OnSave()
{
	ASSERT(!m_bModeless);
	if (!m_bModeless) {
		if (!m_elev.InRange()) {
			CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_DATEELEV);
			m_elev.SetFocus();
			return;
		}
		m_MF(MAG_FIXFORMAT);
		EndDialog(IDOK);
	}
}

void CMagDlg::OnDiscard()
{
	ASSERT(!m_bModeless);
	EndDialog(IDCANCEL);
}

LRESULT CMagDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(118, HELP_CONTEXT);
	return TRUE;
}

void CMagDlg::OnLatDir()
{
	m_MD.lat.isgn *= -1;
	m_MD.zone *= -1;
	m_iOutOfZone = -m_iOutOfZone;
	DisplayLatDir();
	CvtD2UTM();
	//DisplayDEG(TRUE);
}

void CMagDlg::OnLonDir()
{
	m_MD.lon.isgn *= -1;
	DisplayLonDir();
	m_iOutOfZone = 0;
	CvtD2UTM();
}

void CMagDlg::DisplayZONE()
{
	ASSERT(m_bValidDEG);
	double lat = getDEG(&m_MD.lat, m_MD.format);
	//Set controls as required by m_MD.zone and m_MD.lat/lon, which may reveal "out-of-zone" conditions --
	if (abs(m_MD.zone) == 61) {
		m_bUPS = true;
		ASSERT(lat >= 83.5 || lat <= -79.5);
		SetText(IDC_ST_EASTING, "UPS Easting");
		SetText(IDC_ST_NORTHING, "UPS Northing");

		m_btnUTMUPS.SetImageIndex((lat > 0) ? TO_UTM_DN : TO_UTM_UP);
		Show(IDC_ST_INVALID, 0);
		m_btnUTMUPS.Show(1);

		Show(IDC_ZONE_DEC, 0);
		Show(IDC_ZONE_INC, 0);
		Show(IDC_ZONE, 0);
		Show(IDC_ST_ZONE, 0);
		Show(IDC_ST_ZONECHAR, 0);
	}
	else {
		m_bUPS = false;
		ASSERT(lat <= 84.5 || lat >= -80.5);
		SetText(IDC_ST_EASTING, "UTM Easting");
		SetText(IDC_ST_NORTHING, "UTM Northing");

		if (lat >= 83.5 || lat <= -79.5 || abs(lat) <= 0.5) {
			int n;
			if (abs(lat) <= 0.5) {
				ShowZoneLetter();
				n = (m_MD.zone < 0) ? TO_NORTH : TO_SOUTH;
			}
			else n = (lat >= 83.5) ? TO_UPS_UP : TO_UPS_DN;
			Show(IDC_ST_INVALID, 0);
			m_btnUTMUPS.SetImageIndex(n);
			m_btnUTMUPS.Show(1);
		}
		else m_btnUTMUPS.Show(0);
		Show(IDC_ZONE, 1);
		Show(IDC_ZONE_DEC, 1);
		Show(IDC_ZONE_INC, 1);
		Show(IDC_ST_ZONE, 1);
		Show(IDC_ST_ZONECHAR, 1);

		CLinkEdit::m_bSetting = 1;
		SetText(IDC_ZONE, GetIntStr(abs(m_MD.zone)));
	}
}

BOOL CMagDlg::upd_ZONE(int fcn)
{
	/*
		We are editing the Zone control --

		fcn==0	- The control is aquiring focus.
				  CLinkEdit::m_pFocus, if not NULL, is the calling CLinkEdit control.

		fcn!=0	- The the user has changed the control's contents,
				  in which case the m_MD.x field has been updated.
				  CLinkEdit::m_pFocus is the current CLinkEdit control.

		fcn==-1	- The control's value is out of range.

		fcn==1  - Zone incremented by -1 or 1 with ctrl key pressed

		fcn>1   - Zone incremented by fcn-100 by direct edit

	*/

	ASSERT(m_MD.zone && abs(m_MD.zone) < 61);

	if (!fcn) {
		if (m_bValidDEG && (!m_bValidUTM || m_iOutOfZone)) {
			m_iOutOfZone = 0;
			CLinkEdit::m_pFocus = NULL; //update all fields
			VERIFY(CvtD2UTM());
		}
		return 1;
	}

	if (fcn < 1) {
		/*New newly entered value is out-of-range --*/
		SetDegInvalid();
		return 1;
	}

	ASSERT(m_bValidDEG);

	//int zone0=m_MF(MAG_GETUTMZONE); //true zone base on lat/long only
	//int zoneoff=abs(abs(m_MD.zone)-abs(zone0));

	/*
	if(zoneoff>1 && zoneoff<59) {
		goto _restore;
	}
	*/

	if (fcn == 1) {
		//ctrl key pressed or returning from wrong zone--
		if ((fcn = m_MF(MAG_CVTLL2UTM3)) && (fcn != MAG_ERR_OUTOFZONE)) {
			goto _restore;
		}
		if (fcn) {
			m_iOutOfZone = m_MD.zone2;
			Enable(IDC_SAVE, 0);
			//no changes to degree display
		}
		else m_iOutOfZone = 0;
		DisplayUTM(TRUE);
		SetFlt(IDC_CONVERGENCE, m_MD.conv, -2);
		return 1;
	}

	//int zone0=m_MF(MAG_GETUTMZONE); //true zone based on lat/long only
	//int zoneoff=abs(abs(m_MD.zone)-abs(zone0));

	//if(m_iOutOfZone && abs(m_iOutOfZone)!=abs(m_MF(MAG_GETUTMZONE))) goto _restore;
	//signs different only near equator --
	if (abs(m_iOutOfZone) < 61 && m_iOutOfZone*m_MD.zone > 0) goto _restore;

	double lon = getDEG(&m_MD.lon, m_MD.format);
	lon += 6 * (fcn - 100);
	if (abs(lon) > 180) lon += (lon < 0) ? 360 : -360;
	getDMS(&m_MD.lon, lon, m_MD.format);
	VERIFY(!ComputeDecl(&m_MD, getDEG(&m_MD.lat, m_MD.format), lon));
	DisplayDECL(TRUE); //convergence shouldn't change
	DisplayDEG(TRUE);
	return TRUE;

_restore:
	//Restore UTM with correct UTM zone, but keep sign!
	fcn = m_MF(MAG_GETUTMZONE);
	m_MD.zone = (fcn*m_MD.zone < 0) ? -fcn : fcn;
	m_zone.m_pFocus = 0;;
	m_zone.UpdateText(1);
	m_iOutOfZone = 1; //nonzero to force zone in CvtD2UTM()
	CvtD2UTM();
	return 1;
}

void CMagDlg::IncZone(int dir)
{
	int val = m_MD.zone;
	ASSERT(val && abs(val) <= 60);
	val = abs(val) + dir;
	if (val > 60) val = 1;
	else if (val <= 0) val = 60;
	m_MD.zone = (m_MD.zone < 0) ? -val : val;
	m_zone.m_bSetting = 1;
	m_zone.SetWindowText(GetIntStr(val));
	val = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) ? 1 : (100 + dir);
	upd_ZONE(val);
}

void CMagDlg::OnZoneInc()
{
	IncZone(1);
}

void CMagDlg::OnZoneDec()
{
	IncZone(-1);
}

void CMagDlg::OnBnClickedUtmUps()
{
	ASSERT(m_bValidDEG);
	int zoneSetting = m_MD.zone;
	int zoneActual = m_MF(MAG_GETUTMZONE);

	double lat = getDEG(&m_MD.lat, m_MD.format);
	if (abs(lat) <= 0.5) {
		ASSERT(!m_iOutOfZone || abs(zoneActual) != abs(zoneSetting) || (zoneSetting < 0) != (m_iOutOfZone < 0));
		m_MD.zone = -zoneSetting;
	}
	else {
		ASSERT(lat <= -79.5 || lat >= 83.5);
		if (abs(zoneSetting) == 61) {
			//switch to UTM, zone depending on current longitude --
			m_MD.zone = zoneActual;
		}
		else {
			m_MD.zone = (zoneSetting > 0) ? 61 : -61;
		}
	}
	int fcn = m_MF(MAG_CVTLL2UTM3);

	if (fcn && fcn != MAG_ERR_OUTOFZONE) {
		/*The newly entered value is out-of-range --*/
		ASSERT(0);
		m_MD.zone = zoneSetting;
		return;
	}
	m_iOutOfZone = fcn ? m_MD.zone2 : 0;
	DisplayZONE();
	DisplayDEG(TRUE);
	DisplayUTM(TRUE);
	DisplayDECL(TRUE);
	m_bValidUTM = TRUE;
}

void CMagDlg::OnEnterKey()
{
	CWnd *pWnd = GetFocus();
	UINT id = pWnd ? (pWnd->GetDlgCtrlID()) : 0;
	if (m_bModeless) {
		if (id == IDCANCEL) DestroyWindow();
		else GetDlgItem(IDCANCEL)->SetFocus();
	}
	else {
		if (id == IDC_SAVE) {
			m_MF(MAG_FIXFORMAT);
			EndDialog(IDOK);
		}
		else if (id == IDC_DISCARD) EndDialog(IDCANCEL);
		else GetDlgItem(IDC_SAVE)->SetFocus();
	}
}

LRESULT CMagDlg::OnCopyCoords(WPARAM wParam, LPARAM)
{
	char buf1[32], buf2[32], buf3[32], data[128];
	char cLatDir, cLonDir;
	int len;

	buf1[31] = buf2[31] = buf3[31] = data[127] = 0;

	if ((BYTE)wParam < BTYP_FUTM) {
		GetDlgItem(IDC_LATDIR)->GetWindowText(buf1, 2);
		cLatDir = (*buf1 == 'S') ? 'S' : 'N';
		GetDlgItem(IDC_LONDIR)->GetWindowText(buf1, 2);
		cLonDir = (*buf1 == 'E') ? 'E' : 'W';
	}

	switch ((BYTE)wParam) {
	case BTYP_FUTM:
		m_east.GetWindowText(buf1, 31);
		m_north.GetWindowText(buf2, 31);
		_snprintf(data, 64, "%s\t%s", buf1, buf2);
		break;
	case BTYP_FDEG:
		m_latdegF.GetWindowText(buf1, 31);
		m_londegF.GetWindowText(buf2, 31);
		_snprintf(data, 64, "%s%s\t%s%s", (cLatDir == 'S') ? "-" : "", buf1, (cLonDir == 'W') ? "-" : "", buf2);
		break;
	case BTYP_FMIN:
		m_latdeg.GetWindowText(buf1, 31);
		m_latminF.GetWindowText(buf2, 31);
		len = _snprintf(data, 64, "%s %s %c\t", buf1, buf2, cLatDir);
		m_londeg.GetWindowText(buf1, 31);
		m_lonminF.GetWindowText(buf2, 31);
		_snprintf(data + len, 64, "%s %s %c", buf1, buf2, cLonDir);
		break;
	case BTYP_FSEC:
		m_latdeg.GetWindowText(buf1, 31);
		m_latmin.GetWindowText(buf2, 31);
		m_latsecF.GetWindowText(buf3, 31);
		len = _snprintf(data, 64, "%s %s %s %c\t", buf1, buf2, buf3, cLatDir);
		m_londeg.GetWindowText(buf1, 31);
		m_lonmin.GetWindowText(buf2, 31);
		m_lonsecF.GetWindowText(buf3, 31);
		_snprintf(data + len, 64, "%s %s %s %c", buf1, buf2, buf3, cLonDir);
		break;
	}

	ClipboardCopy(data, CF_TEXT);

	return 0;
}

static int GetNumericType(LPCSTR pval)
{
	bool bDigitSeen = false, bNeg = (*pval == '-');
	int prec = 0;
	if (bNeg) pval++;
	for (; *pval; pval++) {
		if (*pval == '.') {
			if (prec++)	return 0;
		}
		else {
			if (!isdigit((BYTE)*pval))	return 0;
			bDigitSeen = true;
			if (prec) prec++;
		}
	}
	if (!bDigitSeen) return 0;
	if (!prec) prec = 1; //prec == no of decimals+1
	return bNeg ? -prec : prec;
}

static int GetDeg(LPCSTR sdir, LPCSTR sdeg, LPCSTR smin, LPCSTR ssec, double *deg/*,int *prec*/)
{
	sdir = strchr("EeWwNnSs", *sdir);
	if (!sdir) return false;
	char c = toupper(*sdir);
	int isgn = (c == 'E' || c == 'N') ? 1 : -1;
	int i0 = (c == 'N' || c == 'S') ? 0 : 1;
	//deg[0] is latitude
	if (deg[i0] != 1000.0 || GetNumericType(sdeg) <= 0 || (GetNumericType(smin)) <= 0 ||
		ssec && (GetNumericType(ssec)) <= 0) return -1;
	deg[i0] = atof(sdeg) + atof(smin) / 60 + (ssec ? (atof(ssec) / 3600) : 0);
	if (deg[i0] > (i0 ? 180.0 : 90.0)) return false;
	deg[i0] *= isgn;
	return i0;
}

static int GetCoordinate(LPSTR arg, double *val)
{
	int iret = 2;
	if (!GetNumericType(arg)) {
		LPCSTR p = strchr("EeWwNnSs", *arg);
		if (!p || GetNumericType(++arg) < 1) return -1;
		*val = atof(arg);
		switch (toupper(*p)) {
		case 'W': *val = -*val;
		case 'E': iret = 1; break;
		case 'S': *val = -*val;
		case 'N': iret = 0; break;
		}
	}
	else *val = atof(arg);
	return iret;
}

LRESULT CMagDlg::OnPasteCoords(WPARAM wParam, LPARAM)
{
	char buf[64];
	ASSERT(::IsClipboardFormatAvailable(CF_TEXT));
	CLinkEdit::m_pFocus = NULL; //UpdateText() will not skip control in focus

	int i0, i1;
	double deg[2];
	BYTE bTyp;
	CString errMsg;
	MAGDATA md;

	if (ClipboardPaste(buf, 63)) {
		cfg_GetArgv(buf, CFG_PARSE_ALL);

		if (cfg_argc == 2) {
			if ((i0 = GetCoordinate(cfg_argv[0], &deg[0])) < 0 || (i1 = GetCoordinate(cfg_argv[1], &deg[1])) < 0 || (i0 + i1 != 1 && i0 + i1 != 4)) goto _errFmt;
			if (i0 + i1 == 1) {
				if (fabs(deg[i0]) > 90.0 || fabs(deg[i1]) > 180.0) goto _errFmt;
				bTyp = BTYP_FDEG;
			}
			else {
				i0 = 0; i1 = 1;
				bTyp = (fabs(deg[0]) <= 90.0 && fabs(deg[1]) <= 180.0) ? BTYP_FDEG : BTYP_FUTM;
			}
			if (bTyp == BTYP_FUTM) {
				if (deg[0]<MIN_EASTING || deg[0]>MAX_EASTING || deg[1]<MIN_NORTHING || deg[1]>MAX_NORTHING) bTyp = 0;
				else {
					md = m_MD;
					md.fcn = MAG_CVTUTM2LL3; //but no err here if out of zone!
					md.east = deg[0]; md.north = deg[1];
					if (mag_GetData(&md)) bTyp = 0; //MAG_ERR_ZONERANGE
				}
				if (!bTyp) {
					errMsg.Format("The clipboard's coordinates (%s, %s) are outside the valid range for both "
						"Lat/Long degrees and %s meters.", cfg_argv[0], cfg_argv[1], ZoneStr(m_MD.zone));
					goto _errFmt;
				}
			}
		}
		else if (cfg_argc == 6) { //15 23.5 N 85 23.5 E 
			deg[0] = deg[1] = 1000;
			i0 = GetDeg(cfg_argv[2], cfg_argv[0], cfg_argv[1], 0, deg);
			if (i0 >= 0)
				i1 = GetDeg(cfg_argv[5], cfg_argv[3], cfg_argv[4], 0, deg);
			if (i0 < 0 || i1 < 0 || i0 == i1) {
				goto _errFmt;
			}
			bTyp = BTYP_FMIN;
		}
		else if (cfg_argc == 8) { //15 59 23.5 N 85 0 23.5 E 
			deg[0] = deg[1] = 1000;
			i0 = GetDeg(cfg_argv[3], cfg_argv[0], cfg_argv[1], cfg_argv[2], deg);
			if (i0 >= 0)
				i1 = GetDeg(cfg_argv[7], cfg_argv[4], cfg_argv[5], cfg_argv[6], deg);
			if (i0 < 0 || i1 < 0 || i0 == i1) {
				goto _errFmt;
			}
			bTyp = BTYP_FSEC;
		}
		else {
			goto _errFmt;
		}

		if (bTyp == BTYP_FUTM) {
			m_MD = md;
			upd_UTM(1);
			DisplayUTM(1);

			//UpdateUTM(md);
			/*
			m_east.UpdateText(1);
			m_north.UpdateText(1);
			CLinkEdit::m_bSetting=1;
			m_east.SetWindowText(GetFloatStr(m_MD.east,prec[0]));
			CLinkEdit::m_bSetting++;
			m_north.SetWindowText(GetFloatStr(m_MD.north,prec[1]));
			*/
		}
		else {
			getDMS(&m_MD.lat, deg[i0], 2);
			getDMS(&m_MD.lon, deg[i1], 2);
			upd_DEG(1);
			ToggleFormat((bTyp == BTYP_FSEC) ? 2 : (bTyp == BTYP_FMIN));
			//Preserve decimals? --
			/*
			CLinkEdit::m_bSetting=1;
			switch(bTyp) {
				case BTYP_FDEG:
					m_latdegF.SetWindowText(GetFloatStr(m_MD.lat.fdeg,prec[0]));
					CLinkEdit::m_bSetting++;
					m_londegF.SetWindowText(GetFloatStr(m_MD.lon.fdeg,prec[1]));
					break;
				case BTYP_FMIN:
					m_latminF.SetWindowText(GetFloatStr(m_MD.lat.fmin,prec[0]));
					CLinkEdit::m_bSetting++;
					m_lonminF.SetWindowText(GetFloatStr(m_MD.lon.fmin,prec[1]));
					break;
				case BTYP_FSEC:
					m_latsecF.SetWindowText(GetFloatStr(m_MD.lat.fsec,prec[0]));
					CLinkEdit::m_bSetting++;
					m_lonsecF.SetWindowText(GetFloatStr(m_MD.lon.fsec,prec[1]));
					break;
			}
			*/
		}
		return 0;
	}

_errFmt:
	{
		if (errMsg.IsEmpty()) errMsg =
			"The clipboard content is not recognizable as a pair of coordinates. "
			"See calculator's help page for a description of allowed formats.";
		CErrorDlg dlg(errMsg, this, "Paste Failure");
		dlg.DoModal();
	}
	return 0;
}
