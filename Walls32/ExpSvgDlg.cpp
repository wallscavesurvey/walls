// ExpSvgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "compile.h"
#include "segview.h"
#include "wall_shp.h"
#include "ExpSvgDlg.h"
#include "SvgAdv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExpSvgDlg dialog

BOOL CExpSvgDlg::init3chk(UINT flag)
{
	return ((m_uFlags&flag) != 0) +
		((m_uFlags&(flag << 16)) != 0);
}

static UINT get3chk(BOOL chk, UINT flag)
{
	return (chk != 0)*flag + (chk == 2)*(flag << 16);
}

CExpSvgDlg::CExpSvgDlg(VIEWFORMAT *pVF, SVG_VIEWFORMAT *pSVG, CWnd* pParent /*=NULL*/)
	: CDialog(CExpSvgDlg::IDD, pParent)
{
	m_bExists = 0;
	m_pVF = pVF;
	m_pSVG = pSVG;

	//Dialogs will call LoadAdvParams() if necessary --
	pSVG->advParams.decimals = 0;

	if (!(m_uFlags = pSV->m_pGridFormat->svgflags)) m_uFlags = SVG_DFLTOPTS;

	m_GridInt = GetFloatStr(LEN_SCALE(pSV->m_pGridFormat->fEastInterval), 6);

	UINT uInt = pSV->m_pGridFormat->wSubInt + 1;
	if (uInt > 10) uInt = 1;
	m_GridSub = GetIntStr(uInt);

	//{{AFX_DATA_INIT(CExpSvgDlg)
	m_bShpFlags = init3chk(SVG_FLAGS);
	m_bShpGrid = init3chk(SVG_GRID);
	m_bShpWalls = init3chk(SVG_WALLS);
	m_bShpNotes = init3chk(SVG_NOTES);
	m_bShpStations = init3chk(SVG_LABELS);
	m_bShpVectors = init3chk(SVG_VECTORS);
	m_bAdjustable = (m_uFlags&SVG_ADJUSTABLE) != 0;
	m_bShpDetail = init3chk(SVG_DETAIL);
	m_bShpLegend = (m_uFlags&SVG_LEGEND) != 0;
	m_bUseStyles = (m_uFlags&SVG_USESTYLES) != 0;;
	//}}AFX_DATA_INIT
}

int CExpSvgDlg::CheckData()
{
	if (Checked(IDC_ADJUSTABLE)) {
		UINT id = 0;
		if (!Checked(IDC_SHP_VECTORS)) id = IDC_SHP_VECTORS;
		else if (!Checked(IDC_SHP_STATIONS)) id = IDC_SHP_STATIONS;
		if (id) {
			AfxMessageBox(IDS_ERR_SVG_ADJUST);
			return id;
		}
	}

	{
		double d;
		if (!CheckFlt(m_GridInt, &d, 0, 100000, 4)) return IDC_GRIDINT;
		m_pSVG->gridint = LEN_INVSCALE(d);
	}

	LPCSTR p = trx_Stpext(m_pathname);

	if (*p == 0) {
		m_pathname += ".svgz";
		GetDlgItem(IDC_2DPATH)->SetWindowText(m_pathname);
	}
	else {
		CString ext(p);
		if (ext.CompareNoCase(".svg") && ext.CompareNoCase(".svgz")) {
			AfxMessageBox("If specified, output file extension must be .svg or .svgz.");
			return IDC_2DPATH;
		}
	}

	//Now let's see if the directory needs to be created --
	int e = DirCheck((char *)(const char *)m_pathname, TRUE);
	if (!e) return IDC_2DPATH;
	if (e != 2) {
		//The directory exists. Check for file existence --
		if (_access(m_pathname, 6) != 0) {
			if (errno != ENOENT) {
				//file not rewritable
				AfxMessageBox(IDS_ERR_FILE_RW);
				return IDC_2DPATH;
			}
		}
		else {
			//File exists and is rewritable.
			//We'll set m_bExists so that an adjustment options dialog is presented
			//in CSegView::OnSvgExport() --
			m_bExists = 1;
		}
	}

	if (m_bShpWalls || m_bShpDetail || m_bShpLegend) {
		if (m_MergePath.IsEmpty() || _access(m_MergePath, 4) != 0) {
			CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_MERGEFILE1, m_MergePath);
			return IDC_MERGEPATH;
		}
		if (!_stricmp(trx_Stpnam(m_MergePath), trx_Stpnam(m_pathname))) {
			AfxMessageBox(IDS_ERR_MERGEFILE);
			return IDC_MERGEPATH;
		}
		if (m_bUseMerge2 && !m_MergePath2.IsEmpty()) {
			p = trx_Stpnam(m_MergePath2);
			if (!_stricmp(p, trx_Stpnam(m_MergePath)) || !_stricmp(p, trx_Stpnam(m_pathname))) {
				AfxMessageBox(IDS_ERR_MERGEFILE);
				return IDC_MERGEPATH2;
			}
			if (_access(m_MergePath2, 4) != 0) {
				CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_MERGEFILE1, m_MergePath);
				return IDC_MERGEPATH2;
			}
			m_pSVG->mrgpath2 = m_MergePath2;
		}
		else {
			m_pSVG->mrgpath2 = "";
		}
		m_pSVG->mrgpath = m_MergePath;
	}
	else {
		m_bUseMerge2 = 0;
		m_pSVG->mrgpath = m_pSVG->mrgpath2 = "";
	}

	m_pSVG->svgpath = m_pathname;

	return 0;
}

void CExpSvgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpSvgDlg)
	DDX_Text(pDX, IDC_2DPATH, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 250);
	DDX_CBString(pDX, IDC_GRIDSUB, m_GridSub);
	DDX_Text(pDX, IDC_GRIDINT, m_GridInt);
	DDV_MaxChars(pDX, m_GridInt, 10);
	DDX_Text(pDX, IDC_2DTITLE, m_title);
	DDV_MaxChars(pDX, m_title, 80);
	DDX_Check(pDX, IDC_CHK_MERGE2, m_bUseMerge2);
	DDX_Check(pDX, IDC_SHP_FLAGS, m_bShpFlags);
	DDX_Check(pDX, IDC_SHP_GRID, m_bShpGrid);
	DDX_Check(pDX, IDC_SHP_MERGEDCONTENT, m_bShpWalls);
	DDX_Check(pDX, IDC_SHP_NOTES, m_bShpNotes);
	DDX_Check(pDX, IDC_SHP_STATIONS, m_bShpStations);
	DDX_Check(pDX, IDC_SHP_VECTORS, m_bShpVectors);
	DDX_Check(pDX, IDC_ADJUSTABLE, m_bAdjustable);
	DDX_Text(pDX, IDC_MERGEPATH, m_MergePath);
	DDV_MaxChars(pDX, m_MergePath, 250);
	DDX_Text(pDX, IDC_MERGEPATH2, m_MergePath2);
	DDV_MaxChars(pDX, m_MergePath2, 250);
	DDX_Check(pDX, IDC_SHP_DETAIL, m_bShpDetail);
	DDX_Check(pDX, IDC_SHP_LEGEND, m_bShpLegend);
	DDX_Check(pDX, IDC_USESTYLES, m_bUseStyles);
	//}}AFX_DATA_MAP

	DDX_Check(pDX, IDC_EXECUTEW2D, m_bView);

	if (pDX->m_bSaveAndValidate) {

		int i = strlen(m_pathname) - 1;
		if (i >= 0 && (m_pathname[i] == '\\' || m_pathname[i] == '/')) m_pathname.GetBufferSetLength(i - 1);

		int id = CheckData(); //sets m_bExists=1 if file exists and is rewritable

		if (id != 0) {
			pDX->m_idLastControl = id;
			//***7.1 pDX->m_hWndLastControl=GetDlgItem(id)->m_hWnd;
			pDX->m_bEditLastControl = TRUE;
			pDX->Fail();
			return;
		}

		//Initialize SVG_VIEWFORMAT from m_pVF and grid settings --
		m_pSVG->view = m_pVF->fPanelView; /*degrees*/
		m_pSVG->width = m_pVF->fFrameWidth*72.0; /*points*/
		m_pSVG->height = m_pVF->fFrameHeight*72.0; /*points*/
		m_pSVG->scale = m_pSVG->width / pPV->m_fPanelWidth; /* pts per meter: scale*meters=points */
		//metric coordinates of upper left corner --
		m_pSVG->centerEast = m_pVF->fCenterEast;
		m_pSVG->centerNorth = m_pVF->fCenterNorth;
		m_pSVG->title = m_title;

		WORD w = (WORD)atoi(m_GridSub);
		m_pSVG->gridsub = w;

		m_uFlags = (
			get3chk(m_bShpFlags, SVG_FLAGS) +
			get3chk(m_bShpGrid, SVG_GRID) +
			get3chk(m_bShpWalls, SVG_WALLS) +
			get3chk(m_bShpDetail, SVG_DETAIL) +
			get3chk(m_bShpLegend, SVG_LEGEND) +
			get3chk(m_bShpNotes, SVG_NOTES) +
			get3chk(m_bShpStations, SVG_LABELS) +
			get3chk(m_bShpVectors, SVG_VECTORS) +
			m_bUseStyles * SVG_USESTYLES +
			m_bAdjustable * SVG_ADJUSTABLE);

		m_pSVG->flags &= ~(SVG_SAVEDFLAGMASK + SVG_EXISTS);

		m_pSVG->flags |= (m_uFlags + m_bExists * SVG_EXISTS);

		pSV->m_pGridFormat->wSubInt = w - 1;
		pSV->m_pGridFormat->fEastInterval = m_pSVG->gridint;
		pSV->m_pGridFormat->svgflags = (m_uFlags&SVG_SAVEDFLAGMASK);
		pSV->m_bChanged = TRUE;

		if (!m_pSVG->advParams.decimals)
			CSvgAdv::LoadAdvParams(&m_pSVG->advParams, (m_uFlags&SVG_MERGEDCONTENT) != 0);

		if (!(m_uFlags&SVG_MERGEDCONTENT)) {
			double labelmeters = m_pSVG->labelSize / (m_pSVG->scale*10.0); //label size in world meters
			if (labelmeters > 5.0) {
				if (IDYES != CMsgBox(MB_YESNO | MB_ICONINFORMATION, IDS_SVG_BIGLABELS1, labelmeters)) {
					EndDialog(IDCANCEL);
					pDX->Fail();
					return;
				}
			}
		}

		if ((m_uFlags&SVG_MERGEDCONTENT) != 0) CLineDoc::SaveAllOpenFiles();

		i = pSV->ExportSVG(this, m_pSVG);

		if (i) {
			if (i != SVG_ERR_VECNOTFOUND) {
				pDX->m_idLastControl = IDC_2DPATH;
				//***7.1 pDX->m_hWndLastControl=GetDlgItem(IDC_2DPATH)->m_hWnd;
				pDX->m_bEditLastControl = TRUE;
				pDX->Fail();
				return;
			}
			EndDialog(IDCANCEL);
			pDX->Fail();
			return;
		}
	}
}

BEGIN_MESSAGE_MAP(CExpSvgDlg, CDialog)
	//{{AFX_MSG_MAP(CExpSvgDlg)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_ADVANCED, OnAdvanced)
	ON_BN_CLICKED(IDC_SHP_MERGEDCONTENT, OnToggleLayer)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	ON_BN_CLICKED(IDBROWSE2, OnBrowse2)
	ON_BN_CLICKED(IDBROWSE3, OnBrowse3)
	ON_BN_CLICKED(IDC_ADJUSTABLE, OnAdjustable)
	ON_BN_CLICKED(IDC_SHP_LEGEND, OnToggleLayer)
	ON_BN_CLICKED(IDC_SHP_DETAIL, OnToggleLayer)
	ON_BN_CLICKED(IDC_CHK_MERGE2, OnChkMerge2)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpSvgDlg message handlers

void CExpSvgDlg::Browse(UINT id)
{
	BrowseFiles((CEdit *)GetDlgItem(id), m_pathname, ".svgz", IDS_SVG_FILES, IDS_SVG_BROWSE);
}

void CExpSvgDlg::OnBrowse()
{
	Browse(IDC_2DPATH);
}

void CExpSvgDlg::OnBrowse2()
{
	Browse(IDC_MERGEPATH);
}

void CExpSvgDlg::OnBrowse3()
{
	Browse(IDC_MERGEPATH2);
}

void CExpSvgDlg::EnableMergeWindows(BOOL bChk)
{
	GetDlgItem(IDC_MERGEPATH)->EnableWindow(bChk);
	GetDlgItem(IDBROWSE2)->EnableWindow(bChk);
	GetDlgItem(IDC_CHK_MERGE2)->EnableWindow(bChk);
	GetDlgItem(IDC_MERGEPATH2)->EnableWindow(m_bUseMerge2 && bChk);
	GetDlgItem(IDBROWSE3)->EnableWindow(m_bUseMerge2 && bChk);
}

BOOL CExpSvgDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	char buf[60];

	double width = m_pVF->fFrameWidth;
	double height = m_pVF->fFrameHeight;
	double wview = pPV->m_fPanelWidth;
	BOOL bInches = pPV->m_bInches;
	BOOL bFeet = LEN_ISFEET();

	DimensionStr(buf, width, height, bInches);
	GetDlgItem(IDC_ST_FRAMESIZE)->SetWindowText(buf);

	EnableMergeWindows(m_bShpWalls || m_bShpDetail || m_bShpLegend);

	if (!bInches) width *= 2.54;
	if (bFeet) wview /= 0.3048;

	_snprintf(buf, 60, "1 %s = %.2f %s", bInches ? "in" : "cm",
		wview / width, bFeet ? "ft" : "m");
	GetDlgItem(IDC_ST_MAPSCALE)->SetWindowText(buf);
	GetDlgItem(IDC_ST_VIEW)->SetWindowText(GetFloatStr(m_pVF->fPanelView, 2));
	GetDlgItem(IDC_ST_GRIDUNITS)->SetWindowText(bFeet ? "ft" : "m");
	OnAdjustable();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

HBRUSH CExpSvgDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_STATIC) {
		switch (pWnd->GetDlgCtrlID()) {
		case IDC_ST_FRAMESIZE:
		case IDC_ST_MAPSCALE:
		case IDC_ST_VIEW:
		case IDC_ST_PROGRESS:
			pDC->SetTextColor(RGB_DKRED);
		}
	}
	return hbr;
}

void CExpSvgDlg::StatusMsg(char *msg)
{
	GetDlgItem(IDC_ST_PROGRESS)->SetWindowText(msg);
}

void CExpSvgDlg::OnAdvanced()
{
	if (!m_pSVG->advParams.decimals) CSvgAdv::LoadAdvParams(&m_pSVG->advParams, TRUE);
	CSvgAdv dlg(&m_pSVG->advParams);
	if (IDOK == dlg.DoModal()) CSvgAdv::SaveAdvParams(&m_pSVG->advParams);
}

void CExpSvgDlg::OnChkMerge2()
{
	m_bUseMerge2 = !m_bUseMerge2;
	EnableMergeWindows(TRUE);
}

void CExpSvgDlg::OnToggleLayer()
{
	EnableMergeWindows(Checked(IDC_SHP_MERGEDCONTENT) ||
		Checked(IDC_SHP_DETAIL) || Checked(IDC_SHP_LEGEND));
}

void CExpSvgDlg::OnAdjustable()
{
	if (Checked(IDC_ADJUSTABLE)) {
		if (!Checked(IDC_SHP_VECTORS)) SetCheck(IDC_SHP_VECTORS, 2);
		if (!Checked(IDC_SHP_STATIONS)) SetCheck(IDC_SHP_STATIONS, 2);
	}
}

LRESULT CExpSvgDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(21, HELP_CONTEXT);
	return TRUE;
}

