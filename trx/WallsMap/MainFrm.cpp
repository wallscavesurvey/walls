// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "windowpl.h"
#include "selmarkerdlg.h"
#include "showshapedlg.h"
#include "EditorPrefsDlg.h"
#include "WebMapFmtDlg.h"
#include "SerialMFC.h"
#include "NMEAParser.h"
#include "GPSOptionsDlg.h"
#include "GPSPortSettingDlg.h"
#include "CompareFcnDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _USE_MYMDI
BEGIN_MESSAGE_MAP(CMyMdi, CWnd)
    //{{AFX_MSG_MAP(CMyMdi)
    ON_MESSAGE(WM_MDITILE,OnMdiTile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

LRESULT CMyMdi::OnMdiTile(WPARAM wParam, LPARAM lParam)
{
	GetMF()->UpdateViews(NULL,LHINT_PRETILE);
	LRESULT lres=Default();
	GetMF()->UpdateViews(NULL,LHINT_POSTTILE);
	return lres;
}
#endif

const UINT WM_SWITCH_EDITMODE = ::RegisterWindowMessage( _T( "_WALLSMAP_SWITCH_EDITMODE" ) );

static const LPCSTR pWinMergeSubfolder="\\WinMerge\\WinMergeU.exe";
static const LPCSTR pWinMergeArgs="/s /u \"%1\" \"%2\"";

// CMainFrame

IMPLEMENT_DYNAMIC(CSyncHint,CObject)
IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ACTIVATE()
	ON_COMMAND(ID_MEMO_DEFAULTS,OnMemoDefaults)
	ON_COMMAND(ID_PREFERENCES_GE,OnGEDefaults)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND_EX(ID_WINDOW_CASCADE, OnMDIWindowCmd)
	ON_COMMAND_EX(ID_WINDOW_TILE_HORZ, OnMDIWindowCmd)
	ON_COMMAND_EX(ID_WINDOW_TILE_VERT, OnMDIWindowCmd)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_DATUM,OnUpdate0)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_XY,OnUpdate1)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_DEG,OnUpdate2)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SIZE,OnUpdate3)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_ZOOM,OnUpdate4)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SCALE,OnUpdate5)
	ON_COMMAND(ID_FILE_CLOSEALL, OnFileCloseall)
	ON_COMMAND(ID_OPENEDITABLE, OnOpenEditable)
	ON_UPDATE_COMMAND_UI(ID_OPENEDITABLE,OnUpdateOpenEditable)
	ON_COMMAND(ID_PREF_OPENLAST, OnPrefOpenlast)
	ON_UPDATE_COMMAND_UI(ID_PREF_OPENLAST, OnUpdatePrefOpenlast)
	ON_COMMAND(ID_PREF_OPENMAXIMIZED, OnPrefOpenMaximized)
	ON_UPDATE_COMMAND_UI(ID_PREF_OPENMAXIMIZED, OnUpdatePrefOpenMaximized)
	ON_COMMAND(ID_ZOOMFORWARDIN, OnZoomforwardin)
	ON_UPDATE_COMMAND_UI(ID_ZOOMFORWARDIN, OnUpdateZoomforwardin)
	ON_COMMAND(ID_ZOOMFORWARDOUT, OnZoomforwardout)
	ON_UPDATE_COMMAND_UI(ID_ZOOMFORWARDOUT, OnUpdateZoomforwardout)
	ON_COMMAND(ID_ZOOMFROMCENTER, OnZoomfromcenter)
	ON_UPDATE_COMMAND_UI(ID_ZOOMFROMCENTER, OnUpdateZoomfromcenter)
	ON_COMMAND(ID_ZOOMFROMCURSOR, OnZoomfromcursor)
	ON_UPDATE_COMMAND_UI(ID_ZOOMFROMCURSOR, OnUpdateZoomfromcursor)
	ON_REGISTERED_MESSAGE(WM_PROPVIEWDOC,OnPropViewDoc)
	ON_COMMAND(ID_SETSELECTEDMARKER,OnSetSelectedMarker)
	ON_COMMAND(ID_PREF_EDITORNAME, &CMainFrame::OnPrefEditorname)
	ON_REGISTERED_MESSAGE(WM_SWITCH_EDITMODE,OnSwitchEditMode)
	ON_COMMAND(ID_WEBMAP_FORMAT,OnWebMapFormat)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_COMMAND(ID_HELP_OPENREADME, OnHelpOpenreadme)
	ON_COMMAND(ID_GPS_PORTSETUP, OnGPSPortSetting)
	ON_WM_SIZE()
	ON_COMMAND(ID_GPS_OPTIONS, OnGpsOptions)
	ON_COMMAND(ID_GPS_SYMBOL, OnGpsSymbol)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_DATUM,
	ID_INDICATOR_XY,
	ID_INDICATOR_DEG,
	ID_INDICATOR_SIZE,
	ID_INDICATOR_ZOOM,
	ID_INDICATOR_SCALE
};

// CMainFrame construction/destruction
// CMainFrame::CMainFrame() defined in header

CMainFrame::~CMainFrame() //virtual
{
	free(m_pComparePrg);
	free(m_pCompareArgs);
}

// toolbar buttons - IDs are command buttons
static UINT BASED_CODE buttons[] =
{
	// same order as in the bitmap 'toolbar.bmp'
	ID_FILE_OPEN,
	ID_FILE_SAVE,
		ID_SEPARATOR,
		//ID_SEPARATOR,
	ID_FILE_PROPERTIES,
	ID_ADDLAYER,
		ID_SEPARATOR,
		//ID_SEPARATOR,
	ID_VIEW_DEFAULT,
	ID_VIEW_ZOOMIN,
	ID_VIEW_ZOOMOUT,
	ID_VIEW_PAN,
	ID_TOOLMEASURE,
		ID_SEPARATOR,
		//ID_SEPARATOR,
	ID_CTR_ONPOSITION,
	ID_FIT_IMAGE,
	//ID_VIEW_NOZOOM,
	ID_ZOOM_BACK,
	ID_ZOOM_FORWARD,
		ID_SEPARATOR,
		//ID_SEPARATOR,
	ID_WINDOW_TILE_VERT,
	ID_WINDOW_TILE_HORZ,
		ID_SEPARATOR,
		//ID_SEPARATOR,
	ID_SHOW_LABELS,
	ID_SHOW_MARKERS,
		ID_SEPARATOR,
		//ID_SEPARATOR,
	ID_TOOLSELECT,
	ID_SELECT_ADD,
	ID_FINDLABEL,
		ID_SEPARATOR,
	ID_GPS_TRACKING,
	ID_GPS_CENTER,
		ID_SEPARATOR,
	ID_APP_ABOUT
};

//static preferences --

BOOL CMainFrame::m_bSync=FALSE;
CSerialMFC *CMainFrame::m_pSerial=NULL;
#define _PORT_DEF 1
#define _BAUD_DEF 4
int CMainFrame::m_nPortDef=1;
int CMainFrame::m_iBaudDef=4; //115200
const int CMainFrame::Baud_Range[5]={4800,9600,19200,38400,115200};

CDialog *CMainFrame::m_pMemoParentDlg=NULL; //for controlling focus

//Text editor --
LPSTR CMainFrame::m_pComparePrg=NULL;
LPSTR CMainFrame::m_pCompareArgs=NULL;
UINT CMainFrame::m_uPref=PRF_DEFAULTS; //(PRF_OPEN_EDITABLE|PRF_OPEN_LAST|PRF_OPEN_MAXIMIZED)
COLORREF CMainFrame::m_clrBkg=RGB(255,255,224);
COLORREF CMainFrame::m_clrTxt=RGB(0,0,0);
CLogFont CMainFrame::m_fEditFont;
bool CMainFrame::m_bChangedPref=false;
bool CMainFrame::m_bChangedEditPref=false;
bool CMainFrame::m_bWebMapFormatChg=false;
int CMainFrame::m_idxWebMapFormat=0;
int CMainFrame::m_idxWebMapFormatOrg=0;
CString CMainFrame::m_csWebMapFormat;
const TCHAR szPreferences[] = _T("Preferences");
const TCHAR szEditorName[] = _T("EditorName");
static const TCHAR szFlags[] = _T("Flags");
static const TCHAR szGPSPort[]=_T("GPSPort");
static const TCHAR szGPSMrk[]=_T("GPSMrk");
static const TCHAR szGPSMrkH[]=_T("GPSMrkH");
static const TCHAR szGPSMrkT[]=_T("GPSMrkT");
static const TCHAR szEditFont[] = _T("EditFont");
static const TCHAR szEditColors[] = _T("EditColors");
static const TCHAR szCompTextPrg[]=_T("CompTextPrg");
static const TCHAR szCompTextArgs[]=_T("CompTextArgs");
static const TCHAR szOptionsWEBidx[] = _T("OptionsWebIdx");
static const TCHAR *pszOptionsWEBMAP[DFLT_URL_COUNT] =
{
	_T("OptionsWebMap"),_T("OptionsWebMap1"),_T("OptionsWebMap2"),
	_T("OptionsWebMap3"),_T("OptionsWebMap4"),_T("OptionsWebMap5")
};

static void InitWinMerge()
{
	TCHAR pf[MAX_PATH];
	ASSERT(!CMainFrame::m_pComparePrg);
	if(SHGetSpecialFolderPath(0, pf, CSIDL_PROGRAM_FILES, FALSE)) {
		strcat(pf,pWinMergeSubfolder);
		if(!_access(pf, 0)) {
			CMainFrame::m_pComparePrg=_strdup(pf);
			CMainFrame::m_pCompareArgs=_strdup(pWinMergeArgs);
		}
	}
}

void CMainFrame::LoadPreferences()
{
	//Test area --
#ifdef _DEBUG
#if 0
	{
		double mpdlat=MetersPerDegreeLat(30.25);
		CFltPoint fp1(-97.7500,29.75),fp2(-97.7500,30.75); //12 km?
		double md=MetricDistance(fp1,fp2)*1000.0; //95921.417052819845
		double mpdlon0=distVincenty(29.75, -97.7500, 30.75, -97.7500); //96243.209989977360 (OK!)
		double mpdlond=distVincenty(30.25, -97.75, 30.25, -98.75)*0.0001;
		double mpdlon1=MetersPerDegreeLon(30.25)*0.0001;
		md=0;
	}
#endif
#endif

	CString s=theApp.GetProfileString(szPreferences,szEditFont);
	if(!s.IsEmpty()) {
		VERIFY(m_fEditFont.GetFontFromStr((LPSTR)(LPCSTR)s));
	}
	else m_fEditFont.lf.lfHeight=-15; //11 point default
	s=theApp.GetProfileString(szPreferences,szEditColors);
	if(!s.IsEmpty()) {
		if(cfg_GetArgv((LPSTR)(LPCSTR)s,CFG_PARSE_ALL)>=2) {
			m_clrTxt=(COLORREF)atol(cfg_argv[0]);
			m_clrBkg=(COLORREF)atol(cfg_argv[1]);
		}
	}

	s=theApp.GetProfileString(szPreferences, szCompTextPrg, "");
	if(!s.IsEmpty()) {
		m_pComparePrg=_strdup(s);
		s=theApp.GetProfileString(szPreferences, szCompTextArgs, "\"%1\" \"%2\"");
		m_pCompareArgs=_strdup(s);
	}
	else InitWinMerge();
	//dialog will write any changes

	s=theApp.GetProfileString(szPreferences,szGPSPort);
	if(!s.IsEmpty()) {
		if(cfg_GetArgv((LPSTR)(LPCSTR)s,CFG_PARSE_ALL)>=2) {
			int i=atol(cfg_argv[0]);
			if(i>0 && i<=16) m_nPortDef=i;
			i=atol(cfg_argv[1]);
			if(i>=0 && i<5) m_iBaudDef=i;
			if(cfg_argc>=3) {
				WORD w=(WORD)atoi(cfg_argv[2]);
				if((w&3)==3) w=2;
				CGPSPortSettingDlg::m_wFlags=w;
				if(cfg_argc>=4) CGPSPortSettingDlg::m_nConfirmCnt=atoi(cfg_argv[3]);
			}
		}
	}

	m_bWebMapFormatChg=false;
	m_idxWebMapFormat=m_idxWebMapFormatOrg=theApp.GetProfileInt(szPreferences,szOptionsWEBidx,-1);

	if(m_idxWebMapFormat==-1) {
		//upgrade from old version --
		m_csWebMapFormat=theApp.GetProfileString(szPreferences,pszOptionsWEBMAP[0],"");
		for(int i=0;i<DFLT_URL_COUNT;i++) {
			if(!m_csWebMapFormat.Compare(CWebMapFmtDlg::m_strPrgDefault[i])) {
				m_idxWebMapFormat=i;
			}
			CWebMapFmtDlg::m_csUsrDefault[i]=CWebMapFmtDlg::m_strPrgDefault[i];
		}
		if(m_idxWebMapFormat<0) {
			m_csWebMapFormat=CWebMapFmtDlg::m_strPrgDefault[m_idxWebMapFormat=0];
		}
	}
	else {
		for(int i=0;i<DFLT_URL_COUNT;i++) {
			#ifdef _DEBUG
			LPCSTR p=pszOptionsWEBMAP[i];
			p=CWebMapFmtDlg::m_strPrgDefault[i]; // default
			#endif
			CWebMapFmtDlg::m_csUsrDefault[i]=theApp.GetProfileString(szPreferences,pszOptionsWEBMAP[i],CWebMapFmtDlg::m_strPrgDefault[i]);
			#ifdef _DEBUG
			p=CWebMapFmtDlg::m_csUsrDefault[i]; // user value
			p=0;
			#endif
		}
		m_csWebMapFormat=CWebMapFmtDlg::m_csUsrDefault[m_idxWebMapFormat];
	}

	m_uPref=theApp.GetProfileInt(szPreferences,szFlags,PRF_DEFAULTS);
}

void CMainFrame::SavePreferences()
{
	if(m_bChangedEditPref) {
		CString s;
		m_fEditFont.GetStrFromFont(s);
		theApp.WriteProfileString(szPreferences,szEditFont,s);
		s.Format("%u %u",m_clrTxt,m_clrBkg);
		theApp.WriteProfileString(szPreferences,szEditColors,s);
	}

	if(CGPSPortSettingDlg::m_bGPSMarkerChg) {
		CString s;
		theApp.WriteProfileString(szPreferences,szGPSMrk,CGPSPortSettingDlg::m_mstyle_s.GetProfileStr(s));
		theApp.WriteProfileString(szPreferences,szGPSMrkH,CGPSPortSettingDlg::m_mstyle_h.GetProfileStr(s));
		theApp.WriteProfileString(szPreferences,szGPSMrkT,CGPSPortSettingDlg::m_mstyle_t.GetProfileStr(s));
	}
	if(CGPSPortSettingDlg::m_wFlags&GPS_FLG_UPDATED) {
		CString s;
		CGPSPortSettingDlg::m_wFlags&=~GPS_FLG_UPDATED;
		s.Format("%u %u %u %u",m_nPortDef,m_iBaudDef,CGPSPortSettingDlg::m_wFlags,CGPSPortSettingDlg::m_nConfirmCnt);
		theApp.WriteProfileString(szPreferences,szGPSPort,s);
	}

	if(m_bChangedPref)
		theApp.WriteProfileInt(szPreferences,szFlags,m_uPref);

	if(m_idxWebMapFormat!=m_idxWebMapFormatOrg) {
		theApp.WriteProfileInt(szPreferences,szOptionsWEBidx,m_idxWebMapFormat);
	}
	ASSERT(m_idxWebMapFormat>=0);
	if(m_bWebMapFormatChg) {
		for(int i=0;i<DFLT_URL_COUNT;i++) {
#ifdef _DEBUG
			LPCSTR p=pszOptionsWEBMAP[i];
			p=CWebMapFmtDlg::m_strPrgDefault[i]; // default
#endif
			CString &s=CWebMapFmtDlg::m_csUsrDefault[i];
			theApp.WriteProfileString(szPreferences,pszOptionsWEBMAP[i],s.Compare(CWebMapFmtDlg::m_strPrgDefault[i])?(LPCSTR)s:NULL);
		}
	}
}

void CMainFrame::DockControlBarLeftOf(CToolBar* Bar, CToolBar* LeftOf)
 {

     // get MFC to adjust the dimensions of all docked ToolBars
     // so that GetWindowRect will be accurate

     RecalcLayout(TRUE);

     CRect rect;
     LeftOf->GetWindowRect(&rect);
     rect.OffsetRect(10,0);

     DWORD dw=LeftOf->GetBarStyle();

     UINT n = (dw&CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : 0;
     n = (dw&CBRS_ALIGN_BOTTOM && n==0) ?  AFX_IDW_DOCKBAR_BOTTOM : n;
     n = (dw&CBRS_ALIGN_LEFT && n==0) ?  AFX_IDW_DOCKBAR_LEFT : n;
     n = (dw&CBRS_ALIGN_RIGHT && n==0) ?  AFX_IDW_DOCKBAR_RIGHT : n;

     // When we take the default parameters on rect, DockControlBar
     // will dock each Toolbar on a seperate line. By calculating a
     // rectangle, we are simulating a Toolbar being dragged to that
     // location and docked.

     DockControlBar(Bar,n,&rect);

 }

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this,
		TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_FLYBY | CBRS_TOOLTIPS	/*| CBRS_GRIPPER | CBRS_SIZE_DYNAMIC*/) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME) ||	!m_wndToolBar.SetButtons(buttons,
		  sizeof(buttons)/sizeof(UINT)))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	//FALSE info: "Delete these three lines if you don't want the toolbar to be dockable"
	//Just remove gripper -- need these to avoid white view windows when resizing frame!

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	VERIFY(m_dropTarget.Register(this,DROPEFFECT_COPY));
	m_dropTarget.m_pMainFrame=this;

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = lpszUniqueClass;
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style |= (LONG)(WS_VSCROLL|WS_HSCROLL);

	return TRUE;
}

// Support for CMyMdi to control tiling --
#ifdef _USE_MYMDI
BOOL CMainFrame::CreateClient(LPCREATESTRUCT lpCreateStruct, CMenu* pWindowMenu)                          
{
    //lpCreateStruct->style |= (LONG)(WS_VSCROLL|WS_HSCROLL);

    if (CMDIFrameWnd::CreateClient(lpCreateStruct, pWindowMenu))
    {
		//VERIFY(::SetClassLong(m_MyMdiClientWnd,GCL_HBRBACKGROUND,(long)::GetStockObject(GRAY_BRUSH)));
        m_MyMdiClientWnd.SubclassWindow(m_hWndMDIClient) ; 
        return TRUE ;
    }
    else return FALSE ;

}
#endif

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

// CMainFrame message handlers
void CMainFrame::OnFileOpen()
{
	// prompt the user (with all document templates)
	char newName[MAX_PATH];
	*newName=0;
	BOOL bRet=CWallsMapDoc::PromptForFileName(newName,MAX_PATH,"Open image file or shapefile",0,0);
	if(!bRet) return; // open cancelled
	CWallsMapDoc::m_bOpenAsEditable=bRet>1;
	if(theApp.OpenDocumentFile(newName)) {
		theApp.AddToRecentFileList(newName);
    }
	CWallsMapDoc::m_bOpenAsEditable=FALSE;
}

void CMainFrame::ClearStatusPanes(UINT first/*=1*/,UINT last/*=3*/)
{
	for(UINT i=first;i<=last;i++) ClearStatusPane(i);
}

/*
BOOL CMainFrame::SetStatusPane(UINT id,LPCSTR format,...)
{
	char buf[80];
	*buf=0;
    va_list marker;
    va_start(marker,format);
    _vsnprintf(buf,80,format,marker); buf[79]=0;
    va_end(marker);
	return m_wndStatusBar.SetPaneText(id,buf);
}
*/

int CMainFrame::StatusBarHt()
{
	if(!m_wndStatusBar.IsWindowVisible()) return 0;
	CRect rect;
	m_wndStatusBar.GetWindowRect(&rect);
	return rect.Height();
}

int CMainFrame::ToolBarHt()
{
	if(!m_wndToolBar.IsWindowVisible()) return 0;
	CRect rect;
	m_wndToolBar.GetWindowRect(&rect);
	return rect.Height();
}

int CMainFrame::NumDocsOpen()
{
	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	int nDocs=0;
	while(pos) {
		nDocs++;
		pTmp->GetNextDoc(pos);
	}
	return nDocs;
}

CWallsMapDoc * CMainFrame::FindOpenDoc(LPCSTR pPath)
{
	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
		CWallsMapDoc *pDoc=(CWallsMapDoc *)pTmp->GetNextDoc(pos);
		if(!_stricmp(pPath,pDoc->GetPathName()))
			return pDoc;
	}
	return NULL;
}

void CMainFrame::EnableGPSTracking()
{
	//Called when closing pGPSDlg or closing port or upon fist reception of location after opening port,
	//in only the latter case we'll have pGPSDlg and pGPSDlg->HavePos()
	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
		CWallsMapDoc *pDoc=(CWallsMapDoc *)pTmp->GetNextDoc(pos);
		if(pDoc->IsGeoRefSupported())
			pDoc->UpdateAllViews(NULL,LHINT_ENABLEGPS);
	}
}

CWallsMapDoc *CMainFrame::FirstGeoRefSupportedDoc()
{
	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
		CWallsMapDoc *pDoc=(CWallsMapDoc *)pTmp->GetNextDoc(pos);
		if(pDoc->IsGeoRefSupported()) return pDoc;
	}
	return NULL;
}

void CMainFrame::UpdateGPSTrack()
{
	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
		CWallsMapDoc *pDoc=(CWallsMapDoc *)pTmp->GetNextDoc(pos);
		if(pDoc->IsGeoRefSupported()) {
			pDoc->UpdateAllViews(NULL,LHINT_GPSTRACKPT);
		}
	}
}

void CMainFrame::UpdateViews(CWallsMapView *pSender,LPARAM lHint,CSyncHint *pHint/*=NULL*/)
{
	CWallsMapDoc *pSendDoc=(pSender?pSender->GetDocument():NULL);
	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
		CWallsMapDoc *pDoc=(CWallsMapDoc *)pTmp->GetNextDoc(pos);
		if(pDoc!=pSendDoc) pDoc->UpdateAllViews(pSender,lHint,pHint);
	}
}

BOOL CMainFrame::OnMDIWindowCmd(UINT nID)
{
	BOOL bRet;
	if(nID==ID_WINDOW_TILE_HORZ || /*nID==ID_WINDOW_TILE_VERT ||*/ nID==ID_WINDOW_CASCADE) {
		UpdateViews(NULL,LHINT_PRETILE);
		bRet=CMDIFrameWnd::OnMDIWindowCmd(nID);
		UpdateViews(NULL,LHINT_POSTTILE);
	}
	else
		bRet=CMDIFrameWnd::OnMDIWindowCmd(nID);

	return bRet;
}

void CMainFrame::RestoreWindow(int nShow)
{
  CWindowPlacement wp;
  if(!wp.Restore(this)) ShowWindow(nShow);
}

void CMainFrame::OnDestroy()
{
  CWindowPlacement wp;
  wp.Save(this);
  CFrameWnd::OnDestroy();
}

void CMainFrame::OnUpdateStatusBar(CCmdUI* pCmdUI,int id)
{
	if(CWallsMapView::m_pViewDrag) CWallsMapView::m_pViewDrag->OnUpdateStatusBar(pCmdUI,id); 
	else pCmdUI->SetText("");
}

void CMainFrame::OnUpdate0(CCmdUI* pCmdUI)
{
	OnUpdateStatusBar(pCmdUI,STAT_DATUM);
}

void CMainFrame::OnUpdate1(CCmdUI* pCmdUI)
{
	OnUpdateStatusBar(pCmdUI,STAT_XY);
}

void CMainFrame::OnUpdate2(CCmdUI* pCmdUI)
{
	OnUpdateStatusBar(pCmdUI,STAT_DEG);
}

void CMainFrame::OnUpdate3(CCmdUI* pCmdUI)
{
	OnUpdateStatusBar(pCmdUI,STAT_SIZE);
}

void CMainFrame::OnUpdate4(CCmdUI* pCmdUI)
{
	OnUpdateStatusBar(pCmdUI,STAT_ZOOM);
}

void CMainFrame::OnUpdate5(CCmdUI* pCmdUI)
{
	OnUpdateStatusBar(pCmdUI,STAT_SCALE);
}

int CMainFrame::NumSyncCompatible(CWallsMapDoc *pSender)
{
	int count=0;
	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();

	while(pos) {
		CWallsMapDoc *pDoc=(CWallsMapDoc *)pTmp->GetNextDoc(pos);
		if(pDoc!=pSender) {
			if(pDoc->IsSyncCompatible(pSender)) count++;
		}
	}
	return count;
}

void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CMDIFrameWnd::OnActivate(nState, pWndOther, bMinimized);

	if(theApp.m_bBackground) {
		theApp.m_bBackground=FALSE;
	}
}

void CMainFrame::OnFileCloseall()
{
	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();

	while(pos) {
		CWallsMapDoc *pDoc=(CWallsMapDoc *)pTmp->GetNextDoc(pos);
		ASSERT(pDoc);
		pDoc->OnCloseDocument();
	}
}

void CMainFrame::OnOpenEditable()
{
	m_bChangedPref=true;
	TogglePref(PRF_OPEN_EDITABLE);
}

void CMainFrame::OnUpdateOpenEditable(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(IsPref(PRF_OPEN_EDITABLE));
}

void CMainFrame::OnPrefOpenlast()
{
	m_bChangedPref=true;
	TogglePref(PRF_OPEN_LAST);
	if(!IsPref(PRF_OPEN_LAST))
		theApp.UpdateRecentMapFile(NULL);
}

void CMainFrame::OnUpdatePrefOpenlast(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(IsPref(PRF_OPEN_LAST));
}

void CMainFrame::OnPrefOpenMaximized()
{
	m_bChangedPref=true;
	TogglePref(PRF_OPEN_MAXIMIZED);
}

void CMainFrame::OnUpdatePrefOpenMaximized(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(IsPref(PRF_OPEN_MAXIMIZED));
}

void CMainFrame::OnZoomfromcenter()
{
	if(IsPref(PRF_ZOOM_FROMCURSOR))
		m_bChangedPref=true;
	ClearPref(PRF_ZOOM_FROMCURSOR);
}

void CMainFrame::OnUpdateZoomfromcenter(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!IsPref(PRF_ZOOM_FROMCURSOR));
}

void CMainFrame::OnZoomfromcursor()
{
	if(!IsPref(PRF_ZOOM_FROMCURSOR))
		m_bChangedPref=true;
	SetPref(PRF_ZOOM_FROMCURSOR);
}

void CMainFrame::OnUpdateZoomfromcursor(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(IsPref(PRF_ZOOM_FROMCURSOR));
}

void CMainFrame::OnZoomforwardin()
{
	if(IsPref(PRF_ZOOM_FORWARDOUT))
		m_bChangedPref=true;
	ClearPref(PRF_ZOOM_FORWARDOUT);
}

void CMainFrame::OnUpdateZoomforwardin(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!IsPref(PRF_ZOOM_FORWARDOUT));
}

void CMainFrame::OnZoomforwardout()
{
	if(!IsPref(PRF_ZOOM_FORWARDOUT))
		m_bChangedPref=true;
	SetPref(PRF_ZOOM_FORWARDOUT);
}

void CMainFrame::OnUpdateZoomforwardout(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(IsPref(PRF_ZOOM_FORWARDOUT));
}

LRESULT CMainFrame::OnPropViewDoc(WPARAM wParam,LPARAM lParam)
{
	CWallsMapDoc *pDoc=(CWallsMapDoc *)lParam;

	CMultiDocTemplate *pTmp=CWallsMapApp::m_pDocTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
		if(pDoc==(CWallsMapDoc *)pTmp->GetNextDoc(pos)) {
			if(wParam) {
				pDoc->ClearVecShprec();
			}
			pDoc->RepaintViews();
			break;
		}
	}
	return TRUE;
}

void CMainFrame::OnSetSelectedMarker()
{
	CSelMarkerDlg dlg(false);
	dlg.DoModal();
}

void CMainFrame::OnMemoDefaults()
{
	CEditorPrefsDlg dlg;
	dlg.m_clrBkg=m_clrBkg;
	dlg.m_clrTxt=m_clrTxt;
	dlg.m_bEditRTF=IsPref(PRF_EDIT_INITRTF);
	dlg.m_bEditToolbar=IsPref(PRF_EDIT_TOOLBAR);
	dlg.m_font=m_fEditFont;
	if(IDOK==dlg.DoModal()) {
		if((dlg.m_bEditRTF==TRUE) != IsPref(PRF_EDIT_INITRTF)) {
			TogglePref(PRF_EDIT_INITRTF);
			m_bChangedPref=true;
		}
		if((dlg.m_bEditToolbar==TRUE) != IsPref(PRF_EDIT_TOOLBAR)) {
			TogglePref(PRF_EDIT_TOOLBAR);
			m_bChangedPref=true;
		}
		if(m_clrBkg!=dlg.m_clrBkg || m_clrTxt!=dlg.m_clrTxt ||
			memcmp(&m_fEditFont,&dlg.m_font,sizeof(CLogFont))) {
			m_clrBkg=dlg.m_clrBkg;
			m_clrTxt=dlg.m_clrTxt;
			m_fEditFont=dlg.m_font;
			m_bChangedEditPref=true;
		}
	}
}

void CMainFrame::OnGEDefaults()
{
	CShpLayer::SetGEDefaults();
}

void CMainFrame::OnPrefEditorname()
{
	SHP_DBFILE::GetTimestampName(NULL,0);
}

void CMainFrame::OnDrop()
{
	int nFailures=0;
	for(V_FILELIST::iterator it=CFileDropTarget::m_vFiles.begin();it!=CFileDropTarget::m_vFiles.end();it++) {
		if(!theApp.OpenDocumentFile(*it))
				nFailures++;
	}
}

LRESULT CMainFrame::OnSwitchEditMode(UINT wParam, LONG lParam)
{
	EDITED_MEMO *pMemo=(EDITED_MEMO *)lParam;
	((CShpLayer *)wParam)->OpenMemo(m_pMemoParentDlg,*pMemo,TRUE);
	free(pMemo->pData);
	pMemo->pData=NULL; //not req
	delete pMemo;
	return 0;
}

BOOL CMainFrame::GetWebMapFormatURL(BOOL bFlyTo)
{
    BOOL bRet=0;
	CWebMapFmtDlg dlg;
	dlg.m_csURL=m_csWebMapFormat;
	dlg.m_idx=m_idxWebMapFormat;
	dlg.m_bFlyTo=bFlyTo;
	dlg.m_bSave=!bFlyTo || CWebMapFmtDlg::m_bSaveStatic;
	if(dlg.DoModal()==IDOK) {
		if(m_csWebMapFormat.Compare(dlg.m_csURL)) m_csWebMapFormat=dlg.m_csURL;
		m_idxWebMapFormat=dlg.m_idx;
		bRet=(bFlyTo && !dlg.m_bSave)?2:1;  //2 default selection needs replacing after fly-to
	}
	if(dlg.m_bChg) m_bWebMapFormatChg=true;
	return bRet;
}

void CMainFrame::OnWebMapFormat()
{
	GetWebMapFormatURL(FALSE);
}

bool CMainFrame::GetWebMapName(CString &name)
{
	int i=m_csWebMapFormat.Find('/');
	if(i<0) return false;
	name.SetString(m_csWebMapFormat,i);
	return true;
}

void CMainFrame::Launch_WebMap(CFltPoint &fpt,LPCSTR nam)
{
	CString url;
	url.Format("http://%s",(LPCSTR)m_csWebMapFormat);
	CString s;
	s.Format("%.7f",fpt.y);
	url.Replace("<LAT>",s);
	s.Format("%.7f",fpt.x);
	url.Replace("<LON>",s);
	if(nam) url.Replace("<NAM>",nam);
	if((int)ShellExecute(m_hWnd, "Open",url, NULL, NULL, SW_NORMAL)<=32) {
		AfxMessageBox("Unable to launch browser.");
	}
}

void CMainFrame::OnHelpOpenreadme()
{
	char buf[_MAX_PATH+40];
	LPSTR p=trx_Stpext(strcpy(buf,theApp.m_csUrlPath));
	strcpy(p,"_ReadMe.html");
	if(!_access(buf,0)) {
		if((int)ShellExecute(m_hWnd, "Open", buf, NULL, NULL, SW_NORMAL)<=32) {
			AfxMessageBox("Unable to launch browser.");
		}
		return;
	}
	AfxMessageBox("Release notes not found.");
}

LRESULT CMainFrame::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	CMDIChildWnd* pActiveChild = MDIGetActive();
	if(!pActiveChild) theApp.WinHelp(1000);
	return TRUE;
/*
	if (pActiveChild != NULL && AfxCallWndProc(pActiveChild,
	  pActiveChild->m_hWnd, WM_COMMANDHELP, wParam, lParam) != 0)
	{
		// handled by child
		return TRUE;
	}
	return FALSE;
*/
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIFrameWnd::OnSize(nType, cx, cy);
	//fixes CControlBar repainting issue when resizing with CS_HREDRAW | CS_VREDRAW removed!
	if(m_wndToolBar.m_hWnd)
		m_wndToolBar.GetParent()->Invalidate();
}

void CMainFrame::OnGPSPortSetting()
{
	if(pGPSDlg) {
		if(pGPSDlg->IsIconic()) {
			pGPSDlg->ShowWindow(SW_RESTORE);
		}
		else pGPSDlg->BringWindowToTop();
	}
	else {
		CGPSPortSettingDlg *p=new CGPSPortSettingDlg(GetDesktopWindow());
	}
}

void CMainFrame::OnGpsOptions()
{
	CGPSOptionsDlg dlg;
	WORD oldFlg=CGPSPortSettingDlg::m_wFlags;
	dlg.m_bAutoRecordStart=(CGPSPortSettingDlg::m_wFlags&GPS_FLG_AUTORECORD)!=0;
	dlg.m_bCenterPercent=(CGPSPortSettingDlg::m_wFlags&GPS_FLG_AUTOCENTER);
	ASSERT((CGPSPortSettingDlg::m_wFlags&GPS_FLG_AUTOCENTER)!=3);

	int cnts[6]={20,50,100,250,500,0};
	for(int i=0;i<6;i++)
		if(cnts[i]==CGPSPortSettingDlg::m_nConfirmCnt) {
			dlg.m_iConfirmCnt=i;
			break;
		}
	
	if(dlg.DoModal()==IDOK) {
		if(dlg.m_bAutoRecordStart) CGPSPortSettingDlg::m_wFlags|=GPS_FLG_AUTORECORD;
		else CGPSPortSettingDlg::m_wFlags&=~GPS_FLG_AUTORECORD;
		if((oldFlg&GPS_FLG_AUTOCENTER)!=(WORD)dlg.m_bCenterPercent) {
			CGPSPortSettingDlg::m_wFlags&=~GPS_FLG_AUTOCENTER;
			CGPSPortSettingDlg::m_wFlags|=(WORD)dlg.m_bCenterPercent;
			//if(FirstGeoRefSupportedDoc()) UpdateGPSTrack();
		}
		if(oldFlg!=CGPSPortSettingDlg::m_wFlags || cnts[dlg.m_iConfirmCnt]!=CGPSPortSettingDlg::m_nConfirmCnt) {
			CGPSPortSettingDlg::m_nConfirmCnt=cnts[dlg.m_iConfirmCnt];
			CGPSPortSettingDlg::m_wFlags|=GPS_FLG_UPDATED;
		}
	};
}

void CMainFrame::OnGpsSymbol()
{
	CSelMarkerDlg dlg(true);
	dlg.DoModal();
}

void CMainFrame::OnClose()
{
	if(pGPSDlg) {
		if(!pGPSDlg->ConfirmClearLog()) return;
		pGPSDlg->DestroyWindow();
	}
	CMDIFrameWnd::OnClose();
}

bool CMainFrame::TextCompOptions()
{
	LPCSTR m_pComp=m_pComparePrg?m_pComparePrg:"";
	LPCSTR pArgs=m_pCompareArgs?m_pCompareArgs:"\"%1\" \"%2\"";
	CCompareFcnDlg dlg(m_pComp,pArgs);
	if(IDOK==dlg.DoModal()) {
		dlg.m_PathName.Trim();
		if(dlg.m_PathName.CompareNoCase(m_pComp)) {
			free(m_pComparePrg);
		    m_pComparePrg=dlg.m_PathName.IsEmpty()?NULL:_strdup(dlg.m_PathName);
			theApp.WriteProfileString(szPreferences, szCompTextPrg, m_pComparePrg);
		}
		dlg.m_Arguments.Trim();
		if(!m_pComparePrg || dlg.m_Arguments.Compare(pArgs)) {
			free(m_pCompareArgs);
			m_pCompareArgs=(!m_pComparePrg || dlg.m_Arguments.IsEmpty())?NULL:_strdup(dlg.m_Arguments);
			theApp.WriteProfileString(szPreferences, szCompTextArgs,m_pCompareArgs);
		}
	}
	return false;
}

void CMainFrame::LaunchComparePrg(LPCSTR pLeft, LPCSTR pRight)
{
	ASSERT(m_pComparePrg);
	CString args;
	if(!m_pCompareArgs) args.Format("\"%s\" \"%s\"",pLeft,pRight);
	else {
		args=m_pCompareArgs;
		args.Replace("%1",pLeft);
		args.Replace("%2",pRight);
	}
	if((int)ShellExecute(NULL,"OPEN",m_pComparePrg, args, NULL, SW_NORMAL)<=32) {
		CMsgBox("Unable to launch %s.",trx_Stpnam(m_pComparePrg));
	}
}