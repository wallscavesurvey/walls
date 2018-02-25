// mainfrm.cpp : implementation of the CMainFrame class
//
#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "txtfrm.h"
#include "lineview.h"
#include "wedview.h"
#include "review.h"
#include "plotview.h"
#include "segview.h"
#include "mapdlg.h"
#include "mapview.h"
#include "editdlg.h"
#include "vecdlg.h"
#include "3dconfig.h"
#include "magdlg.h"
#include "itemprop.h"
#include "compview.h"
#include "compile.h"
//#include "PictHolder.h"
#include "mapframe.h"

#include <direct.h>

//NOTE: This header was copied from ..\vc\mfc\src
//#include <afximpl.h>

#define _SMALL_STATUS_FONT

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

const UINT NEAR WM_FINDREPLACE=::RegisterWindowMessage(FINDMSGSTRING);
const UINT WM_RETFOCUS = ::RegisterWindowMessage("RETFOCUS");
const UINT WM_PROPVIEWLB = ::RegisterWindowMessage("PROPVIEWLB");

#ifdef _USE_FILETIMER
const UINT WM_TIMERREFRESH = ::RegisterWindowMessage("TIMERREFRESH");
#endif

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_INITMENU()
#ifdef _USE_FILETIMER
	ON_WM_TIMER()
#endif
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EDITORFONT, OnEditorFont)
	ON_COMMAND(ID_SURVEYFONT, OnSurveyFont)
	ON_COMMAND(ID_BOOKFONT, OnBookFont)
	ON_COMMAND(ID_REVIEWFONT, OnReviewFont)
	ON_COMMAND(ID_CHECKNAMES, OnChecknames)
	ON_UPDATE_COMMAND_UI(ID_CHECKNAMES, OnUpdateChecknames)
	ON_COMMAND(ID_DISPLAYOPTIONS, OnDisplayOptions)
	ON_COMMAND(ID_PRINTOPTIONS, OnPrintOptions)
	ON_COMMAND(ID_OPT_EDITCURRENT, OnOptEditCurrent)
	ON_UPDATE_COMMAND_UI(ID_OPT_EDITCURRENT, OnUpdateOptEditCurrent)
	ON_COMMAND(ID_OPT_EDITOTHER, OnOptEditOther)
	ON_COMMAND(ID_OPT_EDITSRV, OnOptEditSrv)
	ON_COMMAND(ID_PRINTLABELS, OnPrintLabels)
	ON_COMMAND(ID_SCREENLABELS, OnScreenLabels)
	ON_WM_SYSCOMMAND()
	ON_UPDATE_COMMAND_UI(ID_WINDOW_CLOSEALL, OnUpdateWindowCloseAll)
	ON_COMMAND(ID_VECLIMITS, OnVecLimits)
	ON_UPDATE_COMMAND_UI(ID_EDIT_AUTOSEQ, OnUpdateEditAutoseq)
	ON_WM_DESTROY()
	ON_COMMAND(ID_3D_CONFIG, On3dConfig)
    ON_COMMAND(ID_PRINTNOTES, OnPrintNotes)
	ON_COMMAND(ID_SCREENNOTES, OnScreenNotes)
    ON_COMMAND(ID_FRAMEFONT, OnFrameFont)
	ON_WM_SYSCOLORCHANGE()
	ON_COMMAND(ID_EXPORTOPTIONS, OnExportOptions)
	ON_COMMAND(ID_EXPORTLABELS, OnExportLabels)
	ON_COMMAND(ID_EXPORTNOTES, OnExportNotes)
	ON_COMMAND(ID_SWITCHTOMAP, OnSwitchToMap)
	ON_UPDATE_COMMAND_UI(ID_SWITCHTOMAP, OnUpdateSwitchToMap)
	ON_COMMAND(ID_MAGREF, OnMagRef)
	ON_UPDATE_COMMAND_UI(ID_MAGREF, OnUpdateMagRef)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_MOVEBRANCHFILES, OnMoveBranchFiles)
	ON_COMMAND(ID_MOVEBRANCHLINKS, OnMoveBranchLinks)
	ON_COMMAND(ID_COPYBRANCHFILES, OnCopyBranchFiles)
	ON_UPDATE_COMMAND_UI(ID_COPYBRANCHFILES, OnUpdateCopyBranchFiles)
	ON_COMMAND(ID_COPYBRANCHLINKS, OnCopyBranchLinks)
	ON_UPDATE_COMMAND_UI(ID_COPYBRANCHLINKS, OnUpdateCopyBranchLinks)
	ON_COMMAND(ID_MOVECOPYCANCEL, OnMoveCopyCancel)
	ON_COMMAND(ID_2D_CONFIG, On2dConfig)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDITVECTOR, OnEditVector)
	ON_COMMAND(ID_LOCATEVECTOR, OnLocateVector)
	ON_COMMAND(ID_LAUNCH3D, OnLaunch3d)
	ON_UPDATE_COMMAND_UI(ID_LAUNCH3D, OnUpdateLaunch)
	ON_COMMAND(ID_LAUNCHLOG, OnLaunchLog)
	ON_UPDATE_COMMAND_UI(ID_LAUNCHLOG, OnUpdateLaunch)
	ON_COMMAND(ID_LAUNCHLST, OnLaunchLst)
	ON_UPDATE_COMMAND_UI(ID_LAUNCHLST, OnUpdateDetails)
	ON_COMMAND(ID_LAUNCH2D, OnLaunch2D)
	ON_UPDATE_COMMAND_UI(ID_LAUNCH2D, OnUpdateLaunch)
	ON_COMMAND(ID_MARKERSTYLE, OnMarkerStyle)
	ON_UPDATE_COMMAND_UI(ID_MARKERSTYLE, OnUpdateMarkerStyle)
	ON_COMMAND(ID_SVG_EXPORT, OnSvgExport)
	ON_UPDATE_COMMAND_UI(ID_SVG_EXPORT, OnUpdateSvgExport)
	ON_COMMAND(ID_EXPORT3D, On3DExport)
	ON_UPDATE_COMMAND_UI(ID_EXPORT3D, OnUpdateSvgExport)
	ON_COMMAND(ID_WINDOW_CLOSEALL, OnWindowCloseAll)
	ON_COMMAND(ID_DETAILS, OnDetails)
	ON_UPDATE_COMMAND_UI(ID_DETAILS, OnUpdateDetails)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_PFXNAM,OnUpdateLine)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SEL,OnUpdateLine)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_UTM,OnUpdateLine)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE,OnUpdateLine)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR,OnUpdateIns)
	ON_REGISTERED_MESSAGE(WM_FINDREPLACE,OnFindReplace)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_MESSAGE(WM_COPYDATA,OnCopyData)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars

// toolbar buttons - IDs are command buttons
static UINT BASED_CODE buttons[] =
{
	// same order as in the bitmap 'toolbar.bmp'
	ID_FILE_OPEN,
	ID_FILE_SAVE,
	ID_FILE_PRINT,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_EDIT_CUT,
	ID_EDIT_COPY,
	ID_EDIT_PASTE,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_EDIT_UNDO,
	ID_EDIT_REDO,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_FIND_PREV,
	ID_EDIT_FIND,
	ID_FIND_NEXT,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_EDIT_AUTOSEQ,
	ID_EDIT_REVERSE,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_TRAV_TOGGLE,
	ID_MEASUREDIST,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_CHECKNAMES,
	ID_SWITCHTOMAP,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_LAUNCHLOG,
	ID_MARKERSTYLE,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_DETAILS,
	ID_LAUNCHLST,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_EXPORT3D,
	ID_LAUNCH3D,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_SVG_EXPORT,
	ID_LAUNCH2D,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_MAGREF,
		ID_SEPARATOR,
		ID_SEPARATOR,
	ID_APP_ABOUT
};

static UINT BASED_CODE indicators[] =
{
	ID_SEPARATOR,			// status line indicator
	ID_INDICATOR_PFXNAM,
	ID_INDICATOR_UTM,
	ID_INDICATOR_SEL,
	ID_INDICATOR_OVR,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_LINE,
	ID_INDICATOR_COL
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

//Struct and enum declared in MainFrame.h --
//enum e_ref {REF_FMTDM=1,REF_FMTDMS=2,REF_FMTMASK=3,REF_LONDIR=4,REF_LATDIR=8};
PRJREF CMainFrame::m_reflast(
	3334618.00,	//north;
	600285.000,	//east;
	333,		//int elev meters;
	0.523f,		//float conv;
	19.51f,		//float latsec;
	31.646f,	//float lonsec;
	14,			//zone
	8,			//BYTE latmin;
	57,			//BYTE lonmin;
	30,			//BYTE latdeg;
	97,			//BYTE londeg;
	REF_LONDIR+REF_FMTDMS,	//BYTE flags;
	REF_WGS84,	//BYTE datum (>REF_MAXDATUM);
	"WGS 1984"
);

UINT CMainFrame::m_EditFlags[2]=
  {(6<<8)+E_TABLINES+E_GRIDLINES,(6<<8)+E_ENTERTABS+E_TABLINES+E_GRIDLINES+E_AUTOSEQ};

UINT CMainFrame::m_EditWidth[2]=
  {CWedView::INITIAL_WINDOW_COLS,CWedView::INITIAL_WINDOW_COLS};
	
UINT CMainFrame::m_EditHeight[2]=
  {CWedView::MAX_WINDOW_LINES,CWedView::MAX_WINDOW_LINES};

BOOL CMainFrame::m_bIns=TRUE;
int CMainFrame::m_cyStatus;

#if 0
POS_MDI CMainFrame::m_POS;
#endif

static char BASED_CODE szLimits[] = "Limits";
static char BASED_CODE szMap[] = "Map";
static char BASED_CODE szSaveView[] = "SaveView";
static char BASED_CODE szCompile[] = "Compile Options";
static char BASED_CODE szViewer[] = "Viewer";
static char BASED_CODE szPath[] = "Path";
static char BASED_CODE szStatus[] = "Status";
static char BASED_CODE sz3DFiles[] = "3D Files";
static char BASED_CODE sz2DFiles[] = "2D Files";
//static char BASED_CODE sz2DInt[] = "GridInt";
//static char BASED_CODE sz2DSub[] = "GridSub";

void CMainFrame::XferVecLimits(BOOL bSave)
{
	static BOOL bSwitchToMap;
	static BOOL bSaveView;
		
	if(bSave) {
	    if(CPrjDoc::m_bNewLimits) {
			char str[80];
			sprintf(str,"%.1f %.1f %.1f",
				CPrjDoc::m_LnLimit,
				CPrjDoc::m_AzLimit,
				CPrjDoc::m_VtLimit);
	
			theApp.WriteProfileString(szCompile,szLimits,str);
		}
		if(CPrjDoc::m_bSwitchToMap!=bSwitchToMap)
			theApp.WriteProfileInt(szCompile,szMap,CPrjDoc::m_bSwitchToMap);
		if(CPrjDoc::m_bSaveView!=bSaveView)
			theApp.WriteProfileInt(szCompile,szSaveView,CPrjDoc::m_bSaveView);
	}
	else {
		CString str=theApp.GetProfileString(szCompile,szLimits,NULL);
		if(!str.IsEmpty()) {
			tok_init(str.GetBuffer(1));
			CPrjDoc::m_LnLimit=tok_dbl();
			CPrjDoc::m_AzLimit=tok_dbl();
			CPrjDoc::m_VtLimit=tok_dbl();
		}
		CPrjDoc::m_bSwitchToMap=bSwitchToMap=
			theApp.GetProfileInt(szCompile,szMap,1);
		CPrjDoc::m_bSaveView=bSaveView=
			theApp.GetProfileInt(szCompile,szSaveView,1);
	}
}

static char BASED_CODE szEditOptions[] = "Editor Options"; 
static char BASED_CODE szEditWindow[] = "Editor Window"; 
static char BASED_CODE szEditType[2][8] = {"Others","Surveys"}; 

#define EDIT_VERSION 1
void CMainFrame::LoadEditFlags(int iType)
{
  int tabstops,tablines,entertabs,gridlines,autoseq;
  CString str=theApp.GetProfileString(szEditOptions,(LPSTR)szEditType[iType]);
  if(!str.IsEmpty()) {
     tok_init(str.GetBuffer(1));
     if(tok_int()==EDIT_VERSION) {
	     tablines=tok_int()!=0;
	     gridlines=tok_int()!=0;
	     entertabs=tok_int()!=0;
	     tabstops=tok_int();
	     autoseq=(iType && tok_int()!=0);
	     m_EditFlags[iType]=(tabstops<<8)+E_ENTERTABS*entertabs+
	       E_TABLINES*tablines+E_GRIDLINES*gridlines+E_AUTOSEQ*autoseq;
	 }
     else m_EditFlags[iType]|=E_CHANGED;
  }
  str=theApp.GetProfileString(szEditWindow,(LPSTR)szEditType[iType]);
  if(!str.IsEmpty()) {
     tok_init(str.GetBuffer(1));
	 m_EditWidth[iType]=tok_int();
	 m_EditHeight[iType]=tok_int();
  }
}

void CMainFrame::SaveEditFlags(int iType)
{
  char buf[40];
  
  if((m_EditFlags[iType]&E_CHANGED)!=0) {
	  UINT u=m_EditFlags[iType];
	  sprintf(buf,"%d %d %d %d %d",EDIT_VERSION,(u&E_TABLINES)!=0,(u&E_GRIDLINES)!=0,
		(u&E_ENTERTABS)!=0,u>>8);
		//***7.1 (u&E_ENTERTABS!=0),u>>8);

	  if(u&E_AUTOSEQ) strcat(buf," 1");
	  theApp.WriteProfileString(szEditOptions,(LPSTR)szEditType[iType],buf);
  }

  if(m_EditWidth[iType]&E_SIZECHG) {
	  sprintf(buf,"%d %d",m_EditWidth[iType]&~E_SIZECHG,m_EditHeight[iType]);
	  theApp.WriteProfileString(szEditWindow,(LPSTR)szEditType[iType],buf);
  }
}

CMainFrame::CMainFrame() : m_ListBoxRes1( IDR_LISTICONS, 17, 
		IDC_NODROP, IDC_COPY, IDC_COPYMULTIPLE,
		IDC_MOVE, IDC_MOVEMULTIPLE)
{
	// TODO: add member initialization code here
	m_pFirstPrjDoc=NULL;
	m_pFirstLineDoc=NULL;
	app_pMagDlg=NULL;
}

CMainFrame::~CMainFrame()
{
}

#if 0
//Public static utilities used for window placement in view's OnInitialUpdate --
POSITION CMainFrame::GetFirstMdiFrame(CDocument *pDoc)
{
  m_POS.pTemp=pDoc->GetDocTemplate();
  POSITION dPos=m_POS.pTemp->GetFirstDocPosition();
  while(dPos) {
    m_POS.dPos=dPos;
    pDoc=m_POS.pTemp->GetNextDoc(dPos);
    if(m_POS.vPos=pDoc->GetFirstViewPosition()) return (POSITION)&m_POS;
  }
  return NULL;
}

CFrameWnd *CMainFrame::GetNextMdiFrame(POSITION &pos,CRect *pRect,UINT *pCmd)
{
    if(pos!=(POSITION)&m_POS) return NULL;
    
	POSITION dPos=m_POS.dPos;
	CDocument *pDoc=m_POS.pTemp->GetNextDoc(dPos);
	CView *pView=pDoc->GetNextView(m_POS.vPos);
	
	ASSERT(pView);
	
	while(!m_POS.vPos) {
	  if(!(m_POS.dPos=dPos)) {
	    pos=NULL;
	    break;
	  }
	  pDoc=m_POS.pTemp->GetNextDoc(dPos);
      m_POS.vPos=pDoc->GetFirstViewPosition();
	}
	
	CFrameWnd *pFrame=pView->GetParentFrame();
	WINDOWPLACEMENT wp;
	wp.length=sizeof(wp);
	pFrame->GetWindowPlacement(&wp);
	if(pRect) *pRect=wp.rcNormalPosition;
	if(pCmd) *pCmd=wp.showCmd;
	return pFrame;
}
#endif
	
void CMainFrame::IconEraseBkgnd(CDC* pDC)
{
//	CRect rect;
//	HBRUSH hbr=(HBRUSH)::GetStockObject(GRAY_BRUSH); 
//	CBrush* pOldBrush = (CBrush*)pDC->SelectObject(CBrush::FromHandle(hbr));
//	pDC->GetClipBox(&rect);
//	pDC->PatBlt(rect.left,rect.top,rect.Width(),rect.Height(),PATCOPY);
//	pDC->SelectObject(pOldBrush);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.
	// The WIN32 LoadFrame() supports this option via a parameter.
	//cs.style &= ~(LONG)FWS_ADDTOTITLE;

    // Use the specific class name we established earlier
	cs.lpszClass = lpszUniqueClass;

	if(!CMDIFrameWnd::PreCreateWindow(cs)) return FALSE;
    //if(cs.lpszName) cs.cx=CTxtView::InitialFrameWidth();
	return TRUE;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	::SetClassLong(m_hWnd,GCL_HBRBACKGROUND,(long)::GetStockObject(GRAY_BRUSH));

	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadBitmap(IDR_MAINFRAME) ||
		!m_wndToolBar.SetButtons(buttons,
		  sizeof(buttons)/sizeof(UINT)))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	//Delete these three lines if you don't want the toolbar to
	//be dockable
#ifdef _MAKE_DOCKABLE
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY | CBRS_FLOAT_MULTI);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
#endif

	//Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE("Failed to create status bar\n");
		return -1;		// fail to create
	}
    
	//Added this to give inset appearance to first pane --

	UINT style,id;
	int width;
	m_wndStatusBar.GetPaneInfo(0,id,style,width);
	m_wndStatusBar.SetPaneInfo(0,id,SBPS_STRETCH|SBPS_NORMAL,width);

	//Is there a better way to get bar width?
	{
	    TEXTMETRIC tm;
		HFONT hStatusFont=(HFONT)m_wndStatusBar.GetFont();
		CClientDC dcScreen(NULL);
		HGDIOBJ hOldFont = dcScreen.SelectObject(hStatusFont);
		VERIFY(dcScreen.GetTextMetrics(&tm));
		dcScreen.SelectObject(hOldFont);
		//Set public static member for access by view's OnInitialUpdate() --
        m_cyStatus=(tm.tmHeight-tm.tmInternalLeading);
	}

#ifdef _USE_FILETIMER
	SetTimer(0x123,10000,NULL); //30 secs
#endif

	return 0;
}

LRESULT CMainFrame::OnFindReplace(WPARAM wParam,LPARAM lParam)
{
	CFindReplaceDialog *pFindDlg=CFindReplaceDialog::GetNotifier(lParam);

    //We assume only that the view's dialog class was derived from
    //CFindReplaceDialog, that the view contains public (static)
    //members: m_pSearchDlg, m_pFocusView, and OnTerminateSearch(),
    //and performs search operations within non-static OnSearch() --

	if(pFindDlg==(CFindReplaceDialog *)CWedView::m_pSearchDlg) {

	  if(pFindDlg->IsTerminating()) {
		//The documentation is murky at best, but destroying the dialog
		//window and/or object is apparently being handled by Windows and MFC
		//and should NOT be done at this point. When the last view is closed
		//we can (within the view's OnDestroy()) post an IDCANCEL message
		//to the dialog, which would bring us to here --

		//Give the view class an opporunity to save dialog options, restore
		//status bar's "Ready" message, or whatever --
	    CWedView::OnTerminateSearch();

	    //The dialog object will hereafter cease to exist, so let's nullify
	    //the view's pointer to it --
	    CWedView::m_pSearchDlg=NULL;
	    return 1L;
	  }

	  //If view windows exist, let the last one that had the focus perform
	  //operations via the public CWedView::m_pSearchDlg member. CWedView can
	  //maintain the m_pFocusView pointer in OnSetFocus() and OnDestroy() --
	  if(CWedView::m_pFocusView) {
		CWedView::m_pFocusView->OnSearch();
		return 1L;
	  }
	}
	return 0L;
}

// Support for MYMDI which paints the window background
//
BOOL CMainFrame::CreateClient(LPCREATESTRUCT lpCreateStruct, CMenu* pWindowMenu)                          
{
    lpCreateStruct->style |= (LONG)(WS_VSCROLL|WS_HSCROLL);

    if (CMDIFrameWnd::CreateClient(lpCreateStruct, pWindowMenu))
    {
		//VERIFY(::SetClassLong(m_MyMdiClientWnd,GCL_HBRBACKGROUND,(long)::GetStockObject(GRAY_BRUSH)));
        m_MyMdiClientWnd.SubclassWindow(m_hWndMDIClient) ; 
        return TRUE ;
    }
    else return FALSE ;

}

/////////////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnSysColorChange()
{
	m_ListBoxRes1.SysColorChanged();	
	CMDIFrameWnd::OnSysColorChange();
}

void CMainFrame::OnUpdateIns(CCmdUI* pCmdUI)
{
  pCmdUI->Enable(!m_bIns);
}

void CMainFrame::ToggleIns()
{
	m_bIns=!m_bIns;
}

void CMainFrame::OnUpdateLine(CCmdUI* pCmdUI)
{
  pCmdUI->Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CMyMdi

#ifdef _USE_IMAGE
CMyMdi::CMyMdi()
{

#ifdef _USE_OLE
	m_Picture.Load(IDR_IMAGE_JPEG,"IMAGES");
#else
    VERIFY(m_bmpBackGround.LoadBitmap(IDB_MDIFACE)); 
	BITMAP bm;
	if(m_bmpBackGround.GetSafeHandle() && m_bmpBackGround.GetBitmap(&bm)) {
		m_szBmp = CSize(bm.bmWidth, bm.bmHeight); // size of marble bitmap
	}
	else {
		m_szBmp = CSize(0,0);
		ASSERT(FALSE);
	}
#endif
}

CMyMdi::~CMyMdi()
{
#ifndef _USE_OLE
    if(m_bmpBackGround.GetSafeHandle()) m_bmpBackGround.DeleteObject();
#endif
}

#else

CMyMdi::CMyMdi()
{
}
CMyMdi::~CMyMdi()
{
}
#endif	

BEGIN_MESSAGE_MAP(CMyMdi, CWnd)
    //{{AFX_MSG_MAP(CMyMdi)
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyMdi message handlers

BOOL CMyMdi::OnEraseBkgnd(CDC* pDC)
{
 
#ifdef _USE_IMAGE
	#ifdef _USE_OLE
	if(m_Picture.IsValid()) {
		CBitmap bm,*pbmOld;
		BOOL bOK=bm.CreateCompatibleBitmap(pDC,m_Picture.m_Width,m_Picture.m_Height);
		ASSERT(bOK);
		if(bOK) {
			CDC memdc;
			if(memdc.CreateCompatibleDC(pDC) && (pbmOld=memdc.SelectObject(&bm))) {
				if(m_Picture.Show(&memdc,CRect(0,0,m_Picture.m_Width,m_Picture.m_Height))) {
				  CRect rc;
				  GetClientRect(&rc);
				  for (int y=0; y < rc.Height(); y += m_Picture.m_Height) { // for each row:
					 for (int x=0; x < rc.Width(); x += m_Picture.m_Width) { // for each column:
						pDC->BitBlt(x, y, m_Picture.m_Width, m_Picture.m_Height, &memdc, 0, 0, SRCCOPY); // copy
					 }
				  }
				  memdc.SelectObject(pbmOld);
				  return TRUE;
				}
				memdc.SelectObject(pbmOld);
			}
		}
	}
	#else
		if(m_szBmp.cx) {
		  CDC memdc;
		  memdc.CreateCompatibleDC(pDC);
		  CBitmap* pOldBitmap;
		  if(pOldBitmap=memdc.SelectObject(&m_bmpBackGround)) {
			  CRect rc;
			  GetClientRect(&rc);
			  const CSize& sz = m_szBmp;
			  for (int y=0; y < rc.Height(); y += sz.cy) { // for each row:
				 for (int x=0; x < rc.Width(); x += sz.cx) { // for each column:
					pDC->BitBlt(x, y, sz.cx, sz.cy, &memdc, 0, 0, SRCCOPY); // copy
				 }
			  }
			  memdc.SelectObject(pOldBitmap);
			  return TRUE;
		  }
		}
	#endif
#endif
	return CWnd::OnEraseBkgnd(pDC);
}
      
void CMyMdi::OnSize(UINT nType, int cx, int cy)
{
    Default();
    return;                
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIFrameWnd::OnSize(nType, cx, cy);
	m_MyMdiClientWnd.SendMessage(WM_MDIICONARRANGE);
}

void CMainFrame::OnBookFont()
{
	 CPrjView::ChangeFont(&CPrjView::m_BookFont);
}

void CMainFrame::OnSurveyFont()
{
	 CPrjView::ChangeFont(&CPrjView::m_SurveyFont);
}

void CMainFrame::OnEditorFont()
{
     CWedView::ChangeFont();
}

void CMainFrame::OnReviewFont()
{
     CReView::ChangeFont();
}

void CMainFrame::OnChecknames()
{
	CPrjDoc::m_bCheckNames=!CPrjDoc::m_bCheckNames;
}

void CMainFrame::OnUpdateChecknames(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(CPrjDoc::m_bCheckNames);	
}

void CMainFrame::SetOptions(BOOL bPrinter)
{
	CMapDlg dlg(bPrinter,this);
	dlg.DoModal();
}

void CMainFrame::OnDisplayOptions()
{
	SetOptions(0);
}

void CMainFrame::OnPrintOptions()
{
    SetOptions(1);
}

void CMainFrame::OnExportOptions() 
{
	SetOptions(2);
}

BOOL CMainFrame::EditOptions(UINT *pFlags,int typ)
{
    
    UINT uFlags=*pFlags&~E_CHANGED;
    
    CEditDlg dlg(uFlags,typ+1,this);
    
	if(dlg.DoModal()==IDOK) {
		
		if(dlg.m_uFlags!=uFlags) {
	    
			if(typ<0) {
			  *pFlags=dlg.m_uFlags;
			  return TRUE;
			}
		    
			*pFlags=(dlg.m_uFlags|E_CHANGED);
		    
			if(typ) {
	    		dlg.m_uFlags&=~E_AUTOSEQ;
	    		//No view updates required for just change in autoseq default --
	    		if((uFlags&~E_AUTOSEQ)==dlg.m_uFlags) return TRUE;
			}
		    
			uFlags=dlg.m_uFlags;
		    
			CMultiDocTemplate *pTmp=CWallsApp::m_pSrvTemplate;
			POSITION pos = pTmp->GetFirstDocPosition();
			while(pos) {
			  CLineDoc *pDoc=(CLineDoc *)pTmp->GetNextDoc(pos);
			  if(pDoc->IsSrvFile()==typ) {
				//We will not change the transient flag E_AUTOSEQ --
				pDoc->m_EditFlags=uFlags+(pDoc->m_EditFlags&E_AUTOSEQ);
				pDoc->UpdateAllViews(NULL,CWedView::LHINT_GRID);
			  }
			}
		}
		return TRUE;
	}
	return FALSE;
}

void CMainFrame::OnOptEditCurrent()
{
    CMDIChildWnd *pFrame=MDIGetActive();
    if(pFrame) {
	    CLineDoc *pDoc=(CLineDoc *)pFrame->GetActiveDocument();
	    if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CLineDoc))) {
	      if(EditOptions(&pDoc->m_EditFlags,-1)) {
		    pDoc->UpdateAllViews(NULL,CWedView::LHINT_GRID);
		  }
		}
	}
}

void CMainFrame::OnUpdateOptEditCurrent(CCmdUI* pCmdUI)
{
    CMDIChildWnd *pFrame=MDIGetActive(); //Looking for a CTxtFrame
    pCmdUI->Enable(pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CTxtFrame)));
}

void CMainFrame::OnOptEditOther()
{
	EditOptions(&m_EditFlags[0],0);
}

void CMainFrame::OnOptEditSrv()
{
	EditOptions(&m_EditFlags[1],1);
}

void CMainFrame::ChangeLabelFont(PRJFONT *pF)
{
	DWORD flags=CF_ANSIONLY|CF_NOOEMFONTS|CF_NOSIMULATIONS|CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS;
	pF->Change(flags);
}

void CMainFrame::OnPrintLabels()
{
	ChangeLabelFont(CPlotView::m_mfPrint.pfLabelFont);
}

void CMainFrame::OnScreenLabels()
{
	ChangeLabelFont(CPlotView::m_mfFrame.pfLabelFont);
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	//Experimentation alone (double-clicking system menu) revealed
	//requirement to trap undocumented 0xF063 !!??
	if(nID==SC_CLOSE && CPrjDoc::m_bComputing) return;
	
	CMDIFrameWnd::OnSysCommand(nID, lParam);
}

BOOL CMainFrame::LineDocOpened(LPCSTR pathname)
{
	return CLineDoc::GetLineDoc(pathname)!=NULL;
}

BOOL CMainFrame::CloseLineDoc(LPCSTR pathname,BOOL bNoSave)
{
	CLineDoc *pDoc=CLineDoc::GetLineDoc(pathname);
	return pDoc?pDoc->Close(bNoSave):TRUE;
}

void CMainFrame::OnWindowCloseAll()
{
	//Close all open text files --      
	CMultiDocTemplate *pTmp=theApp.m_pSrvTemplate;
			
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos != NULL) {
	    CLineDoc *pDoc=(CLineDoc *)pTmp->GetNextDoc(pos);
	    POSITION vpos=pDoc->GetFirstViewPosition();
	    if(vpos) {
	      pDoc->GetNextView(vpos)->PostMessage(WM_COMMAND,ID_FILE_CLOSE);
	    }
	}
}

void CMainFrame::OnUpdateWindowCloseAll(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(NULL!=theApp.m_pSrvTemplate->GetFirstDocPosition());
}

void CMainFrame::OnVecLimits()
{
	CVecDlg dlg;
	dlg.m_Azimuth=GetFloatStr(CPrjDoc::m_AzLimit,1);
	dlg.m_Distance=GetFloatStr(CPrjDoc::m_LnLimit,1);
	dlg.m_VertAngle=GetFloatStr(CPrjDoc::m_VtLimit,1);
	if(dlg.DoModal()==IDOK) {
	   CPrjDoc::m_AzLimit=fabs(atof(dlg.m_Azimuth));
	   CPrjDoc::m_LnLimit=fabs(atof(dlg.m_Distance));
	   CPrjDoc::m_VtLimit=fabs(atof(dlg.m_VertAngle));
	   CPrjDoc::m_bNewLimits=TRUE;
	}
}

void CMainFrame::OnUpdateEditAutoseq(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(FALSE);
}

void CMainFrame::RestoreWindow(int nShow)
{
  CWindowPlacement wp;
  if(!wp.Restore(this)) ShowWindow(nShow);
}

////////////////////////////////////////////////////////////////
// CWindowPlacement 1996 Microsoft Systems Journal.
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.

CWindowPlacement::CWindowPlacement()
{
   // Note: "length" is inherited from WINDOWPLACEMENT
   length = sizeof(WINDOWPLACEMENT);
}

CWindowPlacement::~CWindowPlacement()
{
}

//////////////////
// Restore window placement from profile key
BOOL CWindowPlacement::Restore(CWnd* pWnd)
{
   if(!GetProfileWP()) return FALSE;
   // Only restore if window intersets the screen.
   //
   CRect rcTemp, rcScreen(0,0,GetSystemMetrics(SM_CXSCREEN),
      GetSystemMetrics(SM_CYSCREEN));
   if (!::IntersectRect(&rcTemp, &rcNormalPosition, &rcScreen))
      return FALSE;

   pWnd->SetWindowPlacement(this);  // set placement
   return TRUE;
}

//////////////////
// Get window placement from profile.
BOOL CWindowPlacement::GetProfileWP()
{
    CString str=theApp.GetProfileString("Screen","rect",NULL);
	
	showCmd=SW_SHOWNORMAL;
	flags=0;

	if(!str.IsEmpty()) {
		tok_init(str.GetBuffer(1));
		rcNormalPosition.left=tok_int();
		rcNormalPosition.top=tok_int();
		rcNormalPosition.right=tok_int();
		rcNormalPosition.bottom=tok_int();
		if(tok_int()) { 
			showCmd=SW_SHOWMAXIMIZED;
			flags=WPF_RESTORETOMAXIMIZED;
		}
	}
	else {
		RECT rc;
		UINT nWidth;
		::GetWindowRect(::GetDesktopWindow(), &rc);
		nWidth=(rc.right-rc.left)/10;
		rcNormalPosition.left=nWidth;
		rcNormalPosition.right=rc.right-nWidth;
		nWidth=(rc.bottom-rc.top)/10;
		rcNormalPosition.top=nWidth;
		rcNormalPosition.bottom=rc.bottom-nWidth;
	}
	ptMinPosition=CPoint(0,0);
	ptMaxPosition=CPoint(-::GetSystemMetrics(SM_CXBORDER),-::GetSystemMetrics(SM_CYBORDER));
	return TRUE;
}

////////////////
// Save window placement in app profile
void CWindowPlacement::Save(CWnd* pWnd)
{
   pWnd->GetWindowPlacement(this);
   WriteProfileWP();
}

//////////////////
// Write window placement to app profile
void CWindowPlacement::WriteProfileWP()
{
	char str[80];
	sprintf(str,"%d %d %d %d %d",
		rcNormalPosition.left,
		rcNormalPosition.top,
		rcNormalPosition.right,
		rcNormalPosition.bottom,
		showCmd==SW_SHOWMAXIMIZED);
	theApp.WriteProfileString("Screen","rect",str);
}

void CMainFrame::OnDestroy()
{
  CWindowPlacement wp;
  if(app_pMagDlg) app_pMagDlg->DestroyWindow();
  wp.Save(this);
  CMDIFrameWnd::OnDestroy();
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

void CMainFrame::On3dConfig()
{
	OnViewerConfig(TRUE);
}

void CMainFrame::On2dConfig()
{
	OnViewerConfig(FALSE);
}

void CMainFrame::OnViewerConfig(BOOL b3D)
{
	C3DConfigDlg dlg(b3D);

    dlg.m_3DViewer=GetViewerProg(b3D);
	dlg.m_3DPath=Get3DFilePath(b3D,TRUE);
    dlg.m_3DStatus=GetViewerStatus(b3D);
    if(dlg.DoModal()==IDOK) {
		char *sz=b3D?sz3DFiles:sz2DFiles;
		theApp.WriteProfileString(sz,szViewer,dlg.m_3DViewer);
		theApp.WriteProfileString(sz,szPath,dlg.m_3DPath);
		theApp.WriteProfileInt(sz,szStatus,dlg.m_3DStatus);
	}
}

CString CMainFrame::Get3DFilePath(BOOL b3D,BOOL bConfig)
{
	CString str=theApp.GetProfileString(b3D?sz3DFiles:sz2DFiles,szPath);
	int i=str.GetLength();
	if(i || !bConfig) {
		if(!i) {
		  char pathbuf[_MAX_PATH];
		  CPrjDoc *pDoc=theApp.GetOpenProject();
		  if(pDoc) pDoc->GetPathPrefix(pathbuf,NULL);
		  else {
			strcpy(pathbuf,theApp.m_pszHelpFilePath);
			*trx_Stpnam(pathbuf)=0;
		  }
		  str=pathbuf;
		  i=str.GetLength();
		}
		if(i--) {
		  if(str[i]=='\\'|| str[i]=='/') str.GetBufferSetLength(i);
		}
	}
	return str;
}

CString CMainFrame::GetViewerProg(BOOL b3D)
{
	char *sz=b3D?sz3DFiles:sz2DFiles;
	CString str=theApp.GetProfileString(sz,szViewer);
	if(!str.GetLength()) {
	  str=GetWallsPath();
	  if(b3D) str+="walls3d.exe";
	  else str+="walls2d.exe"; 
	}
	return str;
}

CString CMainFrame::Get3DPathName(CPrjListNode *pNode,BOOL b3D)
{
	char name[128];
	CString path;

	if(!b3D) path="_w2d";
	_snprintf(name,128,"\\%s%s.%s",pNode->BaseName(),(LPCSTR)path,
		CPrjDoc::WorkTyp[b3D?CPrjDoc::TYP_3D:CPrjDoc::TYP_2D]);
	path=Get3DFilePath(b3D);
	path+=name;
	return path;
}
 
void CMainFrame::GetMrgPathNames(CString &merge1, CString &merge2)
{
	CPrjListNode *pNode=CPrjDoc::m_pReviewNode->FindFirstSVG();
	if(pNode) {
		merge1=CPrjDoc::m_pReviewDoc->SurveyPath(pNode);
		pNode=CPrjDoc::m_pReviewNode->FindFirstSVG(pNode);
		if(pNode)
			merge2=CPrjDoc::m_pReviewDoc->SurveyPath(pNode);
		return;
	}

	char name[138];
	_snprintf(name,138,"\\%s_mrg.svgz",CPrjDoc::m_pReviewNode->BaseName());
	merge1=Get3DFilePath(FALSE);
	merge1+=name;
}
 
BOOL CMainFrame::GetViewerStatus(BOOL b3D)
{
	return theApp.GetProfileInt(b3D?sz3DFiles:sz2DFiles,szStatus,1);
}
void CMainFrame::SaveViewerStatus(BOOL bStatus,BOOL b3D)
{
	theApp.WriteProfileInt(b3D?sz3DFiles:sz2DFiles,szStatus,bStatus);
}
BOOL CMainFrame::Launch3D(const char *name,BOOL b3D)
{
	  UINT wReturn;
	  int drv=0;
	  char buf[_MAX_PATH];
	  char cmdline[_MAX_PATH];

	  CString path=Get3DFilePath(b3D);
	  
	  if(!path.IsEmpty()) {
	    _getcwd(buf,_MAX_PATH);
	    if(!_chdir(path)) {
	      drv=_getdrive();
	      if(path[1]==':') {
	        int d=path[0];
	        _chdrive(toupper(d)-'A'+1);
	      }
	    }
	  }
	  
 	  path=GetViewerProg(b3D);
 	
 	  ASSERT(!path.IsEmpty());  
 	
	  if(_snprintf(cmdline,_MAX_PATH,*name?"%s \"%s\"":"%s",(const char *)path,name)<0) wReturn=1;
 	
	  else {
		  //wReturn = ::WinExec(cmdline,SW_SHOWNORMAL); //nothing documented for wReturn==1
			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory( &si, sizeof(si) );
			si.cb = sizeof(si);
			ZeroMemory( &pi, sizeof(pi) );


			// Start the child process. 
			if( !CreateProcess( NULL,   // No module name (use command line)
				cmdline,        // Command line
				NULL,           // Process handle not inheritable
				NULL,           // Thread handle not inheritable
				FALSE,          // Set handle inheritance to FALSE
				0,              // No creation flags
				NULL,           // Use parent's environment block
				NULL,           // Use parent's starting directory 
				&si,            // Pointer to STARTUPINFO structure
				&pi )           // Pointer to PROCESS_INFORMATION structure
			) 
			{
				CMsgBox(MB_ICONEXCLAMATION,"CreateProcess failed (%d).\n", GetLastError());
				wReturn=0;
			}
			else wReturn=32;
	  }
		
	  if(drv) {
	    _chdrive(drv);
	    _chdir(buf);
	  }
	  
	  if (wReturn < 32) {
	    CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_EXECUTE1,(const char *)path);
	    return FALSE;
	  }
	  return TRUE;
}

void CMainFrame::OnPrintNotes()
{
	ChangeLabelFont(CPlotView::m_mfPrint.pfNoteFont);
}

void CMainFrame::OnScreenNotes()
{
	ChangeLabelFont(CPlotView::m_mfFrame.pfNoteFont);
}

void CMainFrame::OnFrameFont()
{
	ChangeLabelFont(CPlotView::m_mfPrint.pfFrameFont);
}

void CMainFrame::OnExportLabels() 
{
	ChangeLabelFont(CPlotView::m_mfExport.pfLabelFont);
}

void CMainFrame::OnExportNotes() 
{
	ChangeLabelFont(CPlotView::m_mfExport.pfNoteFont);
}

void CMainFrame::OnSwitchToMap() 
{
	CPrjDoc::m_bSwitchToMap=!CPrjDoc::m_bSwitchToMap;
}

void CMainFrame::OnUpdateSwitchToMap(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(CPrjDoc::m_bSwitchToMap);	
}

void CMainFrame::ConvertUTM2LL(double *pXY)
{
	MD.east=pXY[0];
	MD.north=pXY[1];
	//zone should be already set
	ASSERT(MD.zone==CPrjDoc::m_pReviewNode->GetAssignedRef()->zone);
	MF(MAG_CVTUTM2LL3); //assumes reasonable east,north, can be out-of-zone
	pXY[0]=MD.lon.fdeg*MD.lon.isgn;
	pXY[1]=MD.lat.fdeg*MD.lat.isgn;
}

void CMainFrame::GetRef(PRJREF *pr)
{
	pr->east=MD.east;
	pr->north=MD.north;
	pr->zone=(short)MD.zone;
	pr->conv=MD.conv;
	pr->elev=MD.elev;
	pr->datum=MD.datum;
	pr->latdeg=MD.lat.ideg;
	pr->latmin=MD.lat.imin;
	pr->latsec=(float)(MD.lat.fsec);
	pr->londeg=MD.lon.ideg;
	pr->lonmin=MD.lon.imin;
	pr->lonsec=(float)(MD.lon.fsec);
	pr->flags &= ~(REF_LATDIR+REF_LONDIR+REF_FMTMASK);
	pr->flags |= MD.format;
	if(MD.lat.isgn<0) pr->flags|=REF_LATDIR;
	if(MD.lon.isgn<0) pr->flags|=REF_LONDIR;
}

void CMainFrame::PutRef(PRJREF *pr,MAGDATA *pMD /*=&app_MagData*/)
{
	if(pr->datum>REF_MAXDATUM) {
	  pMD->datum=(pr->datum==REF_NAD27)?MF_NAD27():MF_WGS84();
	}
	else pMD->datum=pr->datum;
	pMD->east=pr->east;
	pMD->north=pr->north;
	pMD->zone=pr->zone;
	pMD->format=2;
	pMD->lat.isgn=(pr->flags&REF_LATDIR)?-1:1;
	pMD->lat.ideg=pr->latdeg;
	pMD->lat.imin=pr->latmin;
	pMD->lat.fsec=(double)pr->latsec;
	pMD->lon.isgn=(pr->flags&REF_LONDIR)?-1:1;
	pMD->lon.ideg=pr->londeg;
	pMD->lon.imin=pr->lonmin;
	pMD->lon.fsec=(double)pr->lonsec;
	pMD->elev=pr->elev;
}

void CMainFrame::CheckDatumName(PRJREF *pr)
{
	if(!*pr->datumname && pr->datum) {
		PutRef(pr);
		MF(MAG_GETDATUMNAME);
		strcpy(pr->datumname,MD.modname);
	}
}

void CMainFrame::BrowseNewProject(CDialog *pDlg, CString &prjPath)
{
	CString strFilter;
	if(!AddFilter(strFilter, IDS_WPJ_FILES)) return;
	if(!DoPromptPathName(prjPath,0 /*OFN_OVERWRITEPROMPT*/, 1, strFilter, FALSE, IDS_WPJBROWSE, PRJ_PROJECT_EXTW)) return;

	pDlg->GetDlgItem(IDC_IMPPRJPATH)->SetWindowText(prjPath);

	CEdit *pw=(CEdit *)pDlg->GetDlgItem(IDC_IMPPRJPATH);
	pw->SetSel(0, -1);
	pw->SetFocus();
}

void CMainFrame::BrowseFileInput(CDialog *pDlg, CString &srcPath, CString &outPath, UINT ids_outfile)
{
    bool bSEF=ids_outfile!=IDS_CSS_FILES;
	CString strFilter;
	if(!AddFilter(strFilter, ids_outfile)) return;
	if(!DoPromptPathName(srcPath, OFN_HIDEREADONLY|OFN_FILEMUSTEXIST, 1, strFilter, TRUE,
		bSEF?IDS_IMPBROWSE:IDS_CSSBROWSE, bSEF?".SEF":".MAK")) return;

	pDlg->GetDlgItem(IDC_IMPDATPATH)->SetWindowText(srcPath);

	SetCurrentDir(srcPath);

	CEdit *pw=(CEdit *)pDlg->GetDlgItem(IDC_IMPPRJPATH);

	if(!pw->GetWindowTextLength()) {
		char *p=strFilter.GetBuffer(srcPath.GetLength()+4);
		strcpy(p, srcPath);
		strcpy(trx_Stpext(p), PRJ_PROJECT_EXTW);
		pw->SetWindowText(p);
		outPath=p;
		strFilter.ReleaseBuffer();
		pw->SetSel(0, -1);
		pw->SetFocus();
	}
}



void CMainFrame::InitRef(PRJREF *pr,MAGDATA *pMD /*=&app_MagData*/)
{
    PutRef(pr,pMD);
	if((pr->flags&REF_FMTMASK)!=2) {
		MF(MAG_FIXFORMAT);
		pMD->format=(pr->flags&REF_FMTMASK);
	}
	time_t t;
	time(&t);
	struct tm *ptm=localtime(&t);
	pMD->m=ptm->tm_mon+1;
	pMD->d=1;
	pMD->y=ptm->tm_year+1900;
}

void CMainFrame::OnMagRef() 
{
	PRJREF *pr=&m_reflast;
	//Let's check the current project --
	CPrjDoc *pDoc=theApp.GetOpenProject();
	if(pDoc) {
		pr=pDoc->GetSelectedNode()->GetDefinedRef();
	}
	InitRef(pr);
	app_pMagDlg=new CMagDlg(CWnd::GetDesktopWindow());
}

void CMainFrame::OnUpdateMagRef(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!app_pMagDlg);	
}


BOOL CMainFrame::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	//return CMDIFrameWnd::OnEraseBkgnd(pDC);
	return TRUE;
}


int moveBranch_id;
BOOL moveBranch_SameList;

void CMainFrame::OnMoveBranchFiles() 
{
	moveBranch_id=MV_FILES;
}

void CMainFrame::OnMoveBranchLinks() 
{
	moveBranch_id=0;
}

void CMainFrame::OnCopyBranchFiles() 
{
	moveBranch_id=MV_COPY|MV_FILES;
}

void CMainFrame::OnUpdateCopyBranchFiles(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!moveBranch_SameList);
}

void CMainFrame::OnCopyBranchLinks() 
{
	moveBranch_id=MV_COPY;
}

void CMainFrame::OnUpdateCopyBranchLinks(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!moveBranch_SameList);
}

void CMainFrame::OnMoveCopyCancel() 
{
	moveBranch_id=-1;
}

void CMainFrame::OnContextMenu(CWnd* pWnd,CPoint point) 
{
	if(pWnd!=&m_wndStatusBar || !CPrjDoc::m_pReviewNode || !CPrjDoc::m_bVectorDisplayed) return;
	ASSERT(CPrjDoc::m_iFindVector && CPrjDoc::m_bVectorDisplayed==2 ||
		   CPrjDoc::m_iFindStation && CPrjDoc::m_bVectorDisplayed==1);

	CRect rect;
	m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_INDICATOR_PFXNAM),&rect);
	pWnd->ClientToScreen(&rect);
	if(!rect.PtInRect(point)) return;

	CMenu menu;
	if(menu.LoadMenu(IDR_BAR_CONTEXT)) {
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,pWnd->GetParentFrame());
	}
}

void CMainFrame::OnEditVector() 
{
	ASSERT(CPrjDoc::m_iFindVector && CPrjDoc::m_bVectorDisplayed==2 ||
		CPrjDoc::m_iFindStation && CPrjDoc::m_bVectorDisplayed==1);

	if(CPrjDoc::m_bVectorDisplayed==2) {
		VERIFY(!dbNTP.Go(CPrjDoc::m_iFindVector));
	}
	else {
		VERIFY(!dbNTP.Go(CPrjDoc::m_iFindStation));
		if(!pPLT->vec_id) {
			VERIFY(!dbNTP.Next());
		}
	}
	ASSERT(pPLT->vec_id);
	CPrjDoc::LoadVecFile((pPLT->vec_id+1)/2);
}

void CMainFrame::OnLocateVector() 
{
	ASSERT(CPrjDoc::m_iFindVector && CPrjDoc::m_bVectorDisplayed==2 ||
		CPrjDoc::m_iFindStation && CPrjDoc::m_bVectorDisplayed==1);

	if(CPrjDoc::m_bVectorDisplayed==1) {
		VERIFY(!dbNTP.Go(CPrjDoc::m_iFindStation));
		if(pCV->GoToComponent()>1) {
			pCV->PlotTraverse(0);
			VERIFY(!dbNTP.Go(CPrjDoc::m_iFindStation));
		}
		CPrjDoc::LocateStation();
	}
	else {
		VERIFY(!dbNTP.Go(CPrjDoc::m_iFindVector));
		CPrjDoc::LocateOnMap();
	}
}

LRESULT CMainFrame::OnCopyData(WPARAM,LPARAM lParam)
{
	return theApp.RunCommand((int)((COPYDATASTRUCT *)lParam)->dwData,(LPCSTR)((COPYDATASTRUCT *)lParam)->lpData);
	//return TRUE;
}

#ifdef _USEHTML
BOOL CMainFrame::OnHelpInfo(HELPINFO* pHelpInfo)
{
	return TRUE;
}
#endif

void CMainFrame::OnLaunch3d()
{
    CPrjDoc::m_pReviewDoc->LaunchFile(NULL,CPrjDoc::TYP_3D);
}

void CMainFrame::OnLaunch2D() 
{
    CPrjDoc::m_pReviewDoc->LaunchFile(NULL,CPrjDoc::TYP_2D);
}

void CMainFrame::OnLaunchLog() 
{
    CPrjDoc::m_pReviewDoc->LaunchFile(CPrjDoc::m_pReviewNode,CPrjDoc::TYP_LOG);
}

void CMainFrame::OnUpdateLaunch(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(CPrjDoc::m_pReviewNode!=NULL);
}

void CMainFrame::OnMarkerStyle() 
{
	ASSERT(pCV!=NULL && pSV->m_pRoot);
	if(pSV->InitSegTree()) pSV->OnSymbols();
}

void CMainFrame::OnUpdateMarkerStyle(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(pCV!=NULL && pSV->m_pRoot);
}

void CMainFrame::OnSvgExport() 
{
	ASSERT(pCV!=NULL);
	if(pSV->InitSegTree()) {
		pPV->ClearTracker();
		pSV->OnSvgExport();
	}
}

void CMainFrame::OnUpdateSvgExport(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(pCV!=NULL && pSV->m_pRoot);
}

void CMainFrame::On3DExport() 
{
	//Uses OnUpdateSvgExport --
	ASSERT(pCV!=NULL);
	if(pSV->InitSegTree()) pSV->On3DExport();
}

void CMainFrame::OnLaunchLst()
{
	//Uses OnUpdateDetails --
	ASSERT(pCV!=NULL);
	if(pSV->InitSegTree()) pSV->OnLaunchLst(NULL);
}

void CMainFrame::OnDetails() 
{
	ASSERT(pCV && pSV->m_pRoot);
	if(pSV->InitSegTree()) pSV->OnDetails();
}

void CMainFrame::OnUpdateDetails(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(pCV && pSV->IsSegSelected());
}

void CMainFrame::OnClose()
{
	CView* pView = GetActiveView();
	if(pView && pView->IsKindOf(RUNTIME_CLASS(CPreviewView)) && m_lpfnCloseProc == _AfxPreviewCloseProc) {
		(*m_lpfnCloseProc)(this);
		ASSERT(m_lpfnCloseProc==NULL);
		return;
	}
#ifdef _USE_FILETIMER
	KillTimer(0x123);
#endif
	CMDIFrameWnd::OnClose();
}

void CMainFrame::SetHierRefresh()
{
	//Required for OpenContainingFolder() when run on Vaio!!
	CMultiDocTemplate *pTmp=theApp.m_pPrjTemplate;

	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos != NULL) {
		CPrjDoc *pDoc=(CPrjDoc *)pTmp->GetNextDoc(pos);
		ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc)));
		POSITION vpos=pDoc->GetFirstViewPosition();
		if(vpos) {
			CPrjView *pView=(CPrjView *)pDoc->GetNextView(vpos);
			if(pView && pView->IsKindOf(RUNTIME_CLASS(CPrjView))) {
				pView->m_PrjList.m_bRefresh=TRUE; //refresh listbox background upon first write
			}
		}
	}
}

#ifdef _USE_FILETIMER
void CMainFrame::OnTimer(UINT nIDEvent)
{
 	if(nIDEvent==0x123) {
		CMultiDocTemplate *pTmp=theApp.m_pPrjTemplate;
				
		POSITION pos = pTmp->GetFirstDocPosition();
		while(pos != NULL) {
			CPrjDoc *pDoc=(CPrjDoc *)pTmp->GetNextDoc(pos);
			ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc)));
			POSITION vpos=pDoc->GetFirstViewPosition();
			if(vpos) {
			  CPrjView *pView=(CPrjView *)pDoc->GetNextView(vpos);
			  if(pView && pView->IsKindOf(RUNTIME_CLASS(CPrjView))) {
				pView->PostMessage(WM_TIMERREFRESH);
			  }
#ifdef _DEBUG
			  else
				  ASSERT(0);
#endif
			}
		}
    }
}
#endif