// plotview.cpp : implementation file
//
#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "compview.h"
#include "linkedit.h"
#include "viewdlg.h"
#include "griddlg.h"
#include "plotview.h"
#include "pageview.h"
#include "segview.h"
#include "pltfind.h"
#include "compile.h"
#include "mapdlg.h"
#include "mapview.h"
#include "mapframe.h"
#include "MapExp.h"
#include "netfind.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define ROT_BUTTON_X 36
#define ROT_BUTTON_Y 22

#pragma pack(2)
typedef struct _PlaceableMetaHeader
{
	DWORD Key;           /* Magic number (always 9AC6CDD7h) */
	WORD  Handle;        /* Metafile HANDLE number (always 0) */
	SHORT Left;          /* Left coordinate in metafile units */
	SHORT Top;           /* Top coordinate in metafile units */
	SHORT Right;         /* Right coordinate in metafile units */
	SHORT Bottom;        /* Bottom coordinate in metafile units */
	WORD  Inch;          /* Number of metafile units per inch */
	DWORD Reserved;      /* Reserved (always 0) */
	WORD  Checksum;      /* Checksum value for previous 10 WORDs */
} PLACEABLEMETAHEADER;
#pragma pack()

/////////////////////////////////////////////////////////////////////////////
// CPlotView

IMPLEMENT_DYNCREATE(CPlotView, CPanelView)

CDC CPlotView::m_dcBmp;
CRect CPlotView::m_rectBmp;
UINT CPlotView::m_uMetaRes;
POINT * CPlotView::m_pathPoint;

//Default grid format used when initializing a new NTV workfile --
GRIDFORMAT CPlotView::m_GridFormat={
	100.0,  //fEastInterval
	1.0,	//fNorthEastRatio
	0.0,	//fEastOffset
	0.0,	//fNorthOffset
	0.0,	//fGridNorth
	100.0,	//fVertInterval
	1.0,	//fHorzVertRatio
	0.0,	//fVertOffset
	0.0,	//fHorzOffset
	100.0,  //fTickInterval
	0,		//wSubInt=1
	0,		//wUseTickmarks
	FALSE   //bChanged
};


static char * szDevType[3]={"Screen","Printer","Export"};
static char * szFontType[3]={"Labels","Notes","Frame Annotation"};
static int offX, offY;

char *CPlotView::szMapFormat="Map Format";

PRJFONT CPlotView::m_LabelScrnFont;
PRJFONT CPlotView::m_NoteScrnFont;
PRJFONT CPlotView::m_LabelPrntFont;
PRJFONT CPlotView::m_NotePrntFont;
PRJFONT CPlotView::m_FrameFont;
PRJFONT CPlotView::m_LabelExportFont;
PRJFONT CPlotView::m_NoteExportFont;

const MAPFORMAT CPlotView::m_mfDflt[3]={
{
    8.0, 	//fFrameWidthInches
    0.0,    //fFrameHeightInches (printer only)
	0,      //iFrameThick (printer only)
	3,      //iMarkerSize
	1,		//iMarkerThick
	5,      //iFlagSize
	1,		//iFlagThick
	1,		//iVectorThin
	2,		//iVectorThick
	3,		//iOffLabelX
	0,		//iOffLabelY
	6,		//iTickLength
	1,		//iTickThick
	1,		//iLabelSpace
	1,		//iLabelInc
	FALSE,	//bLblFlags (1 if heights, 2, 4, or 8 if prefixes)
	TRUE,	//bUseColors
	0,      //iFlagStyle: 0-squares,1-circles,2-triangles
	2,      //bFlagSolid: 0-clear,1-solid,2-opaque
	FALSE,	//bChanged
	FALSE,	//bPrinter	
	&m_LabelScrnFont,  //pfLabelFont
	NULL,              //pfFrameFont
	&m_NoteScrnFont    //pfNoteFont
},
{
	9.5, 	//fFrameWidthInches
	7.5,   //fFrameHeightInches (printer only)
	4,      //iFrameThick (printer only)
	3,      //iMarkerSize
	1,		//iMarkerThick
	5,      //iFlagSize
	1,		//iFlagThick
	1,		//iVectorThin
	4,		//iVectorThick
	3,		//iOffLabelX
	0,		//iOffLabelY
	6,		//iTickLength
	2,		//iTickThick
	1,		//iLabelSpace
	1,		//iLabelInc
	FALSE,	//bLblFlags
	FALSE,	//bUseColors
	0,      //iFlagStyle: 0-squares,1-circles,2-triangles
	2,      //bFlagSolid: 0-clear,1-solid,2-opaque
	FALSE,	//bChanged
	TRUE,	//bPrinter	
	&m_LabelPrntFont,  //pfLabelFont
	&m_FrameFont,      //pfFrameFont
	&m_NotePrntFont    //pfNoteFont
},
{
    9.5, 	//fFrameWidthInches
    7.5,   //fFrameHeightInches (printer only)
	1,      //iFrameThick (printer only)
	3,      //iMarkerSize
	1,		//iMarkerThick
	5,      //iFlagSize
	1,		//iFlagThick
	1,		//iVectorThin
	4,		//iVectorThick
	3,		//iOffLabelX
	0,		//iOffLabelY
	6,		//iTickLength
	2,		//iTickThick
	1,		//iLabelSpace
	1,		//iLabelInc
	FALSE,	//bLblFlags
	FALSE,	//bUseColors
	0,      //iFlagStyle: 0-squares,1-circles,2-triangles
	2,      //bFlagSolid: 0-clear,1-solid,2-opaque
	FALSE,	//bChanged
	TRUE,	//bPrinter	
	&m_LabelExportFont,  //pfLabelFont
	&m_FrameFont,      //pfFrameFont
	&m_NoteExportFont    //pfNoteFont
}};

MAPFORMAT CPlotView::m_mfFrame;
MAPFORMAT CPlotView::m_mfPrint;
MAPFORMAT CPlotView::m_mfExport;

static CPoint ptPan;
static BOOL bPanValidated;

static void PASCAL LoadMapFormat(int typ,MAPFORMAT *pMF)
{
	*pMF=CPlotView::m_mfDflt[typ];
	CWinApp* pApp = AfxGetApp();
	CString str=pApp->GetProfileString(CPlotView::szMapFormat,szDevType[typ],NULL);
	if(str.IsEmpty()) {
		return;
	}
	
    /*Sadly, use of sscanf() generates the following fatal error during linkage:
      RC : fatal error RW1031: Segment 1 and its
      relocation information is too large for load
      optimization. Make the segment LOADONCALL or
      rerun RC using the -K switch if the segment must
      be preloaded.
 	*/
		
	tok_init(str.GetBuffer(1));
	if(tok_int()==5) {
		MAPFORMAT mf=*pMF;
		mf.fFrameWidthInches=tok_dbl();
		mf.fFrameHeightInches=tok_dbl();
		mf.iFrameThick=tok_int();  //width (pixels) of frame outline
		if(typ==2) mf.iFrameThick=1; //force metafile value to 1 
		mf.iMarkerSize=tok_int();  //width of marker
		mf.iMarkerThick=tok_int(); //width of marker pen
		mf.iFlagSize=tok_int();    //width of flag
		mf.iFlagThick=tok_int();   //width of flag pen
		mf.iVectorThin=tok_int();  //width of thin vector pen
		mf.iVectorThick=tok_int(); //width of thick vector pen
		mf.iOffLabelX=tok_int();
		mf.iOffLabelY=tok_int();
		mf.iTickLength=tok_int();  //length of tickmark
		mf.iTickThick=tok_int();   //width of tickmark pen (legend outline?)	
		mf.bLblFlags=tok_int();
		mf.bUseColors=tok_int();
		if(typ!=1) mf.bUseColors=TRUE;
		mf.iFlagStyle=tok_int();
		mf.bFlagSolid=tok_int();
		mf.iLabelSpace=tok_int();
		mf.iLabelInc=tok_int();
		if(tok_count()==20) {
			*pMF=mf;
			return;
		}
	}
	pMF->bChanged=TRUE;
}	   

static void PASCAL SaveMapFormat(int typ,MAPFORMAT *pMF)
{
	int len;
	char str[128];
	
	if(!pMF->bChanged) return;
	
	strcpy(str,"5 "); //Format version
	strcat(str,GetFloatStr(pMF->fFrameWidthInches,4));
	strcat(str," ");
	strcat(str,GetFloatStr(pMF->fFrameHeightInches,4));
	len=strlen(str);
	LoadCMsg(str+len,128-len,IDS_MAPFORMAT16,
	 pMF->iFrameThick,  //width (pixels) of frame outline
	 pMF->iMarkerSize,  //width of marker
	 pMF->iMarkerThick, //width of marker pen
	 pMF->iFlagSize,    //width of flag
	 pMF->iFlagThick,   //width of flag pen
	 pMF->iVectorThin,  //width of thin vector pen
	 pMF->iVectorThick, //width of thick vector pen
     pMF->iOffLabelX,
     pMF->iOffLabelY,
     pMF->iTickLength,  //length of tickmark
	 pMF->iTickThick,   //width of tickmark pen (legend outline?)
	 pMF->bLblFlags,
	 pMF->bUseColors,
	 pMF->iFlagStyle,
	 pMF->bFlagSolid,
	 pMF->iLabelSpace,
	 pMF->iLabelInc);
	 
	AfxGetApp()->WriteProfileString(CPlotView::szMapFormat,szDevType[typ],str);
}	   

CPlotView::CPlotView()
	: CPanelView(CPlotView::IDD)
{
#ifdef VISTA_INC
	ASSERT(g_hwndTrackingTT==NULL);
#endif
	m_bCursorInBmp=FALSE;
	m_bMeasuring=FALSE;
	m_bExternUpdate=FALSE;
	m_bPrinterOptions=FALSE;
	m_pathPoint=NULL;
	m_bInches=LEN_ISFEET();
	m_bPanning=m_bPanned=false;
}

CPlotView::~CPlotView()
{
	if(m_pathPoint) free(m_pathPoint);
}

void CPlotView::DoDataExchange(CDataExchange* pDX)
{
	CPanelView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlotView)
	DDX_Control(pDX, IDC_PLTFRM, m_PltFrm);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPlotView, CPanelView)
	ON_WM_CTLCOLOR()
	ON_WM_SYSCOMMAND()
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_WM_DRAWITEM()
	ON_WM_SETCURSOR()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_COMMAND(ID_TRAV_TOGGLE, OnTravToggle)
	ON_UPDATE_COMMAND_UI(ID_TRAV_TOGGLE, OnUpdateTravToggle)
	ON_COMMAND(ID_PLT_PANTOHERE, OnPltPanToHere)
	ON_COMMAND(ID_PLT_ZOOMIN, OnPltZoomIn)
	ON_COMMAND(ID_PLT_ZOOMOUT, OnPltZoomOut)
	ON_UPDATE_COMMAND_UI(ID_PLT_RECALL, OnUpdatePltRecall)
	ON_UPDATE_COMMAND_UI(ID_PLT_DEFAULT, OnUpdatePltDefault)
	ON_COMMAND(ID_MEASUREDIST, OnMeasureDist)
	ON_UPDATE_COMMAND_UI(ID_MEASUREDIST, OnUpdateMeasureDist)
	ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateFindNext)
	ON_UPDATE_COMMAND_UI(ID_FIND_PREV, OnUpdateFindPrev)
	ON_COMMAND(ID_FIND_PREV, OnFindPrev)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_UTM, OnUpdateUTM)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SEL, OnUpdateUTM)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
	ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
	ON_COMMAND(ID_MAP_EXPORT, OnMapExport)
	ON_COMMAND(ID_PLT_DEFAULT, OnPanelDefault)
	ON_COMMAND(ID_PLT_RECALL, OnRecall)
	ON_BN_CLICKED(IDC_SYSNEXT, OnSysNext)
	ON_BN_CLICKED(IDC_SYSPREV, OnSysPrev)
	ON_BN_CLICKED(IDC_TRAVNEXT, OnTravNext)
	ON_BN_CLICKED(IDC_TRAVPREV, OnTravPrev)
	ON_BN_CLICKED(IDC_EXTERNFRAME, OnExternFrame)
	ON_BN_CLICKED(IDC_UPDATEFRAME, OnUpdateFrame)
	ON_BN_CLICKED(IDC_ZOOMIN, OnPanelZoomIn)
	ON_BN_CLICKED(IDC_DEFAULT, OnPanelDefault)
	ON_BN_CLICKED(IDC_ZOOMOUT, OnPanelZoomOut)
	ON_BN_CLICKED(IDC_PLANPROFILE, OnPlanProfile)
	ON_BN_CLICKED(IDC_EXTERNGRIDLINES, OnExternGridlines)
	ON_BN_CLICKED(IDC_ROTATELEFT, OnRotateLeft)
	ON_BN_CLICKED(IDC_ROTATERIGHT, OnRotateRight)
	ON_BN_CLICKED(IDC_TRAVSELECTED, OnTravSelected)
	ON_BN_CLICKED(IDC_TRAVFLOATED, OnTravFloated)
	ON_BN_CLICKED(IDC_EXTERNFLAGS, OnExternFlags)
	ON_BN_CLICKED(IDC_DISPLAYOPTIONS, OnDisplayOptions)
	ON_BN_CLICKED(IDC_RECALL, OnRecall)
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_BN_CLICKED(IDC_WALLS, OnWalls)
	ON_BN_CLICKED(IDC_CHGVIEW, OnChgView)
	ON_BN_CLICKED(IDC_EXTERNNOTES, OnExternNotes)
#ifdef _SAV_LBL
	ON_BN_CLICKED(IDC_EXTERNLABELS, OnExternLabels)
	ON_BN_CLICKED(IDC_EXTERNMARKERS, OnExternMarkers)
#else
	ON_BN_CLICKED(IDC_EXTERNLABELS, OnFlagToggle)
	ON_BN_CLICKED(IDC_EXTERNMARKERS, OnFlagToggle)
#endif
	ON_BN_CLICKED(IDC_GRIDOPTIONS, OnGridOptions)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapView drawing

static void LoadFont(PRJFONT *pf,int font,int dev,CLogFont *plf)
{
	char buf[80];
	sprintf(buf,"%s Font for %s",szDevType[dev],szFontType[font]);
	pf->Load(buf,dev>0,plf);
}

void CPlotView::Initialize()
{
	CLogFont lf;
	lf.Init("Arial",60);
    LoadFont(&m_LabelScrnFont,0,0,&lf);
    LoadFont(&m_LabelPrntFont,0,1,&lf);
    LoadFont(&m_LabelExportFont,0,2,&lf);
    LoadFont(&m_FrameFont,2,1,&lf);
	lf.lfItalic=TRUE;
    LoadFont(&m_NoteScrnFont,1,0,&lf);
    LoadFont(&m_NotePrntFont,1,1,&lf);
    LoadFont(&m_NoteExportFont,1,2,&lf);
    LoadMapFormat(0,&m_mfFrame);
    LoadMapFormat(1,&m_mfPrint);
    LoadMapFormat(2,&m_mfExport);
	//WHY??? ---
	//m_mfExport.iMarkerSize=m_mfFrame.iMarkerSize=m_mfPrint.iMarkerSize;
	//m_mfExport.iFlagSize=m_mfFrame.iFlagSize=m_mfPrint.iFlagSize;
	//m_mfExport.iFlagStyle=m_mfFrame.iFlagStyle=m_mfPrint.iFlagStyle;
	//m_mfExport.bFlagSolid=m_mfFrame.bFlagSolid=m_mfPrint.bFlagSolid;
	m_mfExport.fFrameWidthInches=m_mfPrint.fFrameWidthInches;
	m_mfExport.fFrameHeightInches=m_mfPrint.fFrameHeightInches;
    CMainFrame::XferVecLimits(FALSE);
	CMapView::m_hCursorPan=AfxGetApp()->LoadCursor(MAKEINTRESOURCE(AFX_IDC_MOVE4WAY));
	//CMapView::m_hCursorHand=CMapView::m_hCursorPan;
	CMapView::m_hCursorHand=AfxGetApp()->LoadCursor(IDC_HAND3);
	CMapView::m_hCursorMeasure=AfxGetApp()->LoadCursor(IDC_MEASURE);
	CMapView::m_hCursorCross=AfxGetApp()->LoadStandardCursor(IDC_CROSS);
	CMapView::m_hCursorArrow=AfxGetApp()->LoadStandardCursor(IDC_ARROW);
	CMapView::m_hCursorIdentify=AfxGetApp()->LoadCursor(IDC_IDENTIFY);
	hist_pltfind.InsertAsFirst(CString("<REF>"));
}

void CPlotView::Terminate()
{
	m_LabelScrnFont.Save();
	m_LabelPrntFont.Save();
	m_LabelExportFont.Save();
	m_NoteScrnFont.Save();
	m_NotePrntFont.Save();
	m_NoteExportFont.Save();
	m_FrameFont.Save();
    SaveMapFormat(0,&m_mfFrame);
    SaveMapFormat(1,&m_mfPrint);
    SaveMapFormat(2,&m_mfExport);
    CMainFrame::XferVecLimits(TRUE);
}

void CPlotView::OnInitialUpdate()
{
    static int dlgfont_id[]={
    IDC_TRAVNEXT,IDC_TRAVSTATIC,IDC_SYSSTATIC,
    IDC_TRAVPREV,IDC_SYSNEXT,IDC_SYSPREV,IDC_ST_PANEL,IDC_ST_FRAME,0};
      
    CPanelView::OnInitialUpdate();
	GetDlgItem(IDC_ST_FTORMETERS)->SetWindowText(LEN_UNITSTR());
	GetDlgItem(IDC_ST_TRUEORGRID)->SetWindowText(VIEW_UNITSTR());
    
    //Set fonts of most controls to a more pleasing non-bold type. NOTE: Changing the
    //font in the dialog template would adversely affect button sizes, etc. Also, non-bold
    //type styles apparently can't be specified in the template!
    
    GetReView()->SetFontsBold(GetSafeHwnd(),dlgfont_id);
    
    m_PltFrm.GetClientRect(&m_rectBmp);

    m_sizeBmp.cx = m_rectBmp.right-2; //Subtract 2 since we will InflateRect(-1,-1)
    m_sizeBmp.cy = m_rectBmp.bottom-2;

    // Convert to screen coordinates using static as base,
    // then to DIALOG (instead of static) client coords 
    // using dialog as base
    m_PltFrm.ClientToScreen(&m_rectBmp);
    ScreenToClient(&m_rectBmp);
    m_rectBmp.InflateRect(-1,-1);
        
    m_ptBmp.x = m_rectBmp.left;
    m_ptBmp.y = m_rectBmp.top;
    
    // Get temporary DC for dialog - Will be released in dc destructor
    CClientDC dc(this);
    
    //Create CBitmap for plot --
    // m_dcBmp.SelectObject will not work with planes=1, bits=4 -- Why not?
    //UINT planes=(UINT)GetDeviceCaps(dc.m_hDC, PLANES);  //=1
    //UINT bits=(UINT)GetDeviceCaps(dc.m_hDC, BITSPIXEL); //=8 (this works)
	//VERIFY(m_cBmp.CreateBitmap(m_sizeBmp.cx,m_sizeBmp.cy,planes,bits,NULL));
	                   
    //Might as well use this --
    //Allocates 160128+32 bytes of GPI private (assuming 1024x756x256) --
    //Allocates 313120+32 bytes of GPI private (assuming 1024x756x64K) --
    VERIFY(m_cBmp.CreateCompatibleBitmap(&dc,m_sizeBmp.cx,m_sizeBmp.cy));
    
    //For efficient traverse highlighting, we need another similar bitmap --
    VERIFY(m_cBmpBkg.CreateCompatibleBitmap(&dc,m_sizeBmp.cx,m_sizeBmp.cy));
    
    //Our strategy in CPrjDoc::PlotTraverse() --
    //  1) Render m_cBmpBkg with network component and highlighted system
    //     assuming view and/or system has changed.
    //  2) SRCCOPY m_cBmpBkg onto m_cBmp
    //  3) Render m_cBmp with highlighted traverse
	    
    // Create compatible memory DC to hold m_cBmp --
    VERIFY(m_dcBmp.CreateCompatibleDC(&dc));
    VERIFY(m_hBmpOld=(HBITMAP)::SelectObject(m_dcBmp.m_hDC,m_cBmp.GetSafeHandle()));
    VERIFY(m_hBrushOld=(HBRUSH)::SelectObject(m_dcBmp.m_hDC,CReView::m_hPltBrush));
    VERIFY(m_hPenOld=(HPEN)::SelectObject(m_dcBmp.m_hDC,CReView::m_PenNet.GetSafeHandle()));

    //Create another device context for painting owner-draw rotate buttons.
    //Get bitmap for buttons 2x2, 36x22 pixels each --

    VERIFY(m_bmRotate.LoadMappedBitmap(IDB_ROTATE));

    VERIFY(m_dcRotBmp.CreateCompatibleDC(&dc));
    VERIFY(m_hRotBmpOld=(HBITMAP)::SelectObject(m_dcRotBmp.m_hDC,m_bmRotate.GetSafeHandle()));

    m_hCursorBmp=AfxGetApp()->LoadStandardCursor(IDC_CROSS);
	m_tracker.m_pr=&CPlotView::m_rectBmp;
	m_tracker.m_nStyle=CRectTracker::resizeInside|CRectTracker::solidLine;
    //Compile enables IDC_UPDATEFRAME button accordingly
    ResetContents();
 }

void CPlotView::LabelProfileButton(BOOL bProfile)
{
	GetDlgItem(IDC_PLANPROFILE)->SetWindowText(bProfile?"Plan":"Profile");
}

void CPlotView::ResetContents()
{
	m_dcBmp.PatBlt(0,0,m_sizeBmp.cx,m_sizeBmp.cy,PATCOPY);
	InvalidateRect(&m_rectBmp,FALSE);
	m_recNTN=m_iTrav=m_iSys=0;
	m_bTrackerActive=FALSE;
	m_pPanPoint=NULL;
	m_pViewFormat=NULL;
	memset(m_vfSaved,0,sizeof(m_vfSaved));
	m_bCursorInBmp=m_bTracking=FALSE;
}

void CPlotView::LoadViews()
{
	BOOL bFirstView=!pPV->m_recNTN;

	ASSERT(ixSEG.Opened());

	if(!m_recNTN) {
		memcpy(m_vfSaved,&((SEGHDR *)ixSEG.ExtraPtr())->vf,sizeof(m_vfSaved));
		SetCheck(IDC_EXTERNFLAGS,(m_vfSaved[0].bFlags&VF_VIEWFLAGS)!=0);
		SetCheck(IDC_EXTERNNOTES,(m_vfSaved[0].bFlags&VF_VIEWNOTES)!=0);
#ifdef _SAV_LBL
		SetCheck(IDC_EXTERNLABELS, (m_vfSaved[0].bFlags&VF_VIEWLABEL)!=0);
		SetCheck(IDC_EXTERNMARKERS, (m_vfSaved[0].bFlags&VF_VIEWMARK)!=0);
#endif
	}
	m_recNTN=dbNTN.Position();

    for(int i=0;i<2;i++) {
		//m_vfSaved[i].bChanged=FALSE; //temp
		if(m_vfSaved[i].iComponent && m_vfSaved[i].iComponent!=m_recNTN) {
			//if(!m_vfSaved[i].bActive) m_vfSaved[i].iComponent=0; //***9/17/03
			m_vfSaved[i].iComponent=0;
			m_vfSaved[i].bChanged=TRUE;
			m_vfSaved[i].bActive=FALSE;
		}
		else if(pCV->m_bHomeTrav && m_vfSaved[i].bActive) {
			m_vfSaved[i].bChanged=TRUE;
			m_vfSaved[i].bActive=FALSE;
		}
		else m_vfSaved[i].bChanged=FALSE;
	}
	ASSERT(!m_vfSaved[1].bActive || !m_vfSaved[0].bActive);

	m_bProfile=m_vfSaved[1].bActive;
	LabelProfileButton(m_bProfile);

	//Why require this?!! (causes assertions)
	//ASSERT(VF_ISINCHES(m_vfSaved[0].bFlags)==VF_ISINCHES(m_vfSaved[1].bFlags));
	m_bInches=VF_ISINCHES(m_vfSaved[0].bFlags);
}

void CPlotView::SaveViews()
{
	ASSERT(ixSEG.Opened());
	for(int i=0;i<2;i++) {
		if(m_vfSaved[i].bChanged) {
		   memcpy(&((SEGHDR *)ixSEG.ExtraPtr())->vf[i],&m_vfSaved[i],sizeof(VIEWFORMAT));
		   ixSEG.MarkExtra();
		}
	}
}

void CPlotView::DrawFrame(CDC *pDC,CRect *pClip)
{
	CRect rect=m_rectBmp;
	rect.InflateRect(1,1);
    if(!pClip->IntersectRect(pClip,&rect)) return;

  //Draw shadowed frame --
	CPen *oldpen=(CPen *)pDC->SelectObject(&CReView::m_PenNet);
	//if(pClip->left<=rect.left) {
	  pDC->MoveTo(rect.left,rect.top);
	  pDC->LineTo(rect.left,rect.bottom);
	//}
	//if(pClip->top<=rect.top) {
	  pDC->MoveTo(rect.left,rect.top);
	  pDC->LineTo(rect.right,rect.top);
	//}

	pDC->SelectStockObject(WHITE_PEN);
	//if(pClip->bottom>=rect.bottom) {
	  pDC->MoveTo(rect.left,rect.bottom);
	  pDC->LineTo(rect.right,rect.bottom);
	//}
	//if(pClip->right>=rect.right) {
	  pDC->MoveTo(rect.right,rect.top);
	  pDC->LineTo(rect.right,rect.bottom);
	//}
	pDC->SelectObject(oldpen);
}

void CPlotView::OnDraw(CDC* pDC)
{
	// The window has been invalidated and needs to be repainted;
	
	CPanelView::OnDraw(pDC);
     
	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	
	CRect rectPlot;
	if(rectPlot.IntersectRect(&m_rectBmp,&rectClip)) {
	    bPanValidated=TRUE;
	    pDC->BitBlt(rectPlot.left,rectPlot.top,rectPlot.Width(),rectPlot.Height(),
	  	  &m_dcBmp,rectPlot.left-m_rectBmp.left,rectPlot.top-m_rectBmp.top,SRCCOPY);
	  	if(m_bTrackerActive) m_tracker.Draw(pDC);
		if(rectPlot!=rectClip) DrawFrame(pDC,&rectClip);
	}
	else {
	  DrawFrame(pDC,&rectClip);
	}
}

void CPlotView::ViewMsg()
{
	GetDlgItem(IDC_VIEW)->SetWindowText(GetFloatStr(fView,1));
	m_iView=-2;
}

void CPlotView::DisplayPanelWidth()
{
	double w;
	
	if(m_bTrackerActive) w=m_iTrackerWidth/m_xscale;
	else {
	  w=m_fPanelWidth;
	  ASSERT(fabs(m_sizeBmp.cx/m_xscale-w)<.001);
	}
	  
	//get minimal length string, 1 decimal place --
	GetDlgItem(IDC_WIDTH)->SetWindowText(GetFloatStr(LEN_SCALE(w),-1));
}

void CPlotView::EnableDefaultButtons(BOOL bDefEnable)
{
	EnableSysButtons(pCV->m_bSys+pCV->m_bIsol>1);
    EnableTravButtons(pCV->m_bSeg>1);
    EnableTraverse(pCV->m_bSeg>0);
    Enable(IDC_DEFAULT,bDefEnable);
}

void CPlotView::SetTrackerPos(int l,int t,int r,int b)
{
    if(l<0 || t<0 || r>m_sizeBmp.cx || b>m_sizeBmp.cy || r-l<MIN_TRACKERWIDTH) {
      m_bTrackerActive=FALSE;
      return;
    }
	m_tracker.m_rect.SetRect(l,t,r,b);
	m_tracker.m_rect.OffsetRect(m_ptBmp);
    m_iTrackerWidth=r-l;
	m_bTrackerActive=TRUE;
}

int CPlotView::GetTrackerPos(CPoint *pt)
{
	CRect rect=m_tracker.m_rect; //Does not include border and handles as does GetTrueRect()
	rect.NormalizeRect();
	if(pt) {
		pt->x=rect.left-m_ptBmp.x;
		pt->y=rect.top-m_ptBmp.y;
		ASSERT(pt->x>=0 && pt->y>=0);
	}
	return m_iTrackerWidth=rect.Width();
}

void CPlotView::ClearTracker(BOOL bDisp)
{
	if(m_bTrackerActive) {
		CRect rect;
		m_tracker.GetTrueRect(rect);
		m_bTrackerActive=FALSE;
		InvalidateRect(&rect);
		if(bDisp) DisplayPanelWidth();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CPlotView message handlers
void CPlotView::OnSysCommand(UINT nChar,LONG lParam)
{
	//Handle keystrokes here --
	
	CPanelView::OnSysCommand(nChar,lParam);
}

void CPlotView::OnDestroy()
{
	CPanelView::OnDestroy();
	
    VERIFY(::SelectObject(m_dcRotBmp.GetSafeHdc(),m_hRotBmpOld));
    VERIFY(m_dcRotBmp.DeleteDC());
	VERIFY(m_bmRotate.DeleteObject()); //Required according to LoadBitmap() doc
	
	//NOTE: We use API calls here to avoid MFC's exception-prone
	//FromHandle() calls --
	
    VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(),m_hBmpOld));
    VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(),m_hPenOld));
    VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(),m_hBrushOld));
    
    VERIFY(m_dcBmp.DeleteDC());
    
    // ~CBitmap() should free memory for the bitmap --
    // m_cBmp.DeleteObject();
}

BOOL CPlotView::IncSys(int dir)
{
	int nSys=pCV->m_bSys;
	
	if(nSys+pCV->m_bIsol<2) return FALSE;
	
	//There are at least two systems --
	
    CListBox *plb=(CListBox *)pCV->GetDlgItem(IDC_SYSTEMS);
    ASSERT(nSys==plb->GetCount());
    
    int iSysSel=plb->GetCurSel();
    
    if(iSysSel<0) {
      //We must be moving from the independent loops --
      iSysSel=(dir>0)?0:(nSys-1);
      pCV->SendMessage(WM_NEXTDLGCTL,(WPARAM)plb->GetSafeHwnd(),TRUE);
    }
    else if((iSysSel+=dir)<0 || iSysSel>=nSys) {
      if(pCV->m_bIsol) {
        //We are moving to the independent loops --
        plb->SetCurSel(-1);
        plb=(CListBox *)pCV->GetDlgItem(IDC_ISOLATED);
        plb->SetCurSel(0);
        pCV->SendMessage(WM_NEXTDLGCTL,(WPARAM)plb->GetSafeHwnd(),TRUE);
        
        pCV->OnIsolChg();
        iSysSel=-1;
      }
      else iSysSel=(dir>0)?0:(nSys-1); 
    }
    
    if(iSysSel>=0) {
      plb->SetCurSel(iSysSel);
      pCV->OnSysChg();
    }
    pCV->PlotTraverse(0);
    EnableTravButtons(pCV->m_bSeg>1);
	GetDocument()->RefreshMapTraverses(TRV_SEL);
	return TRUE;
}

void CPlotView::IncTrav(int dir)
{
	CListBox *plb=(CListBox *)pCV->GetDlgItem(IDC_SEGMENTS);
	int nTrav=pCV->m_bSeg-1;
	int i=plb->GetCurSel()+dir;
	
	if(nTrav<=0) return;
	
	if(i<0) i=nTrav;
	else if(i>nTrav) i=0;
	if(plb->SetCurSel(i)<0) return;
	CPrjDoc::SetTraverse(((CListBox *)pCV->GetDlgItem(IDC_SYSTEMS))->GetCurSel(),i);
    pCV->OnTravChg(); //update Float button and m_iStrFlag flags
    pCV->PlotTraverse(0);
	GetDocument()->RefreshMapTraverses(TRV_SEL);
}

void CPlotView::OnSysNext()
{
	if(IncSys(1)) GetDlgItem(IDC_SYSNEXT)->SetFocus();
}

void CPlotView::OnSysPrev()
{
	if(IncSys(-1)) GetDlgItem(IDC_SYSPREV)->SetFocus();
}

void CPlotView::OnTravNext()
{
	IncTrav(1);
}

void CPlotView::OnTravPrev()
{
	IncTrav(-1);
}

void CPlotView::RefreshMeasure()
{
    CClientDC dc(this);
    dc.SetROP2(R2_NOT);
    dc.SetBkMode(TRANSPARENT);
    dc.MoveTo(ptMeasureStart);
	dc.LineTo(ptMeasureEnd);
}

BOOL CPlotView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: Add your message handler code here and/or call default
	
	if(this==pWnd) {
	  if(nHitTest==HTCLIENT) {
	    if(m_bCursorInBmp) {
           if(!m_bTrackerActive || !m_tracker.SetCursor(this,nHitTest))
             ::SetCursor(bMeasureDist?CMapView::m_hCursorMeasure:m_hCursorBmp);
	       return TRUE;
	    }
	  }
	}
	return CPanelView::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CPlotView::TrackerOK()
{
	//Let's check if the zoomed rectangle would be less than 1 meter wide --
	if(m_bTrackerActive) {
		int w=m_tracker.m_rect.Width();
		if(w<0) w=-w;
		if(LEN_SCALE(w/m_xscale)<0.1) {
			AfxMessageBox(IDS_ERR_ZOOMLIMIT);
			return FALSE;
		}
	}
	return TRUE;
}

void CPlotView::OnFileExport()
{
	CMsgBox(MB_ICONINFORMATION,IDS_PRJ_UNAVAILABLE);
}

void CPlotView::OnExternFrame()
{
    if(!TrackerOK()) return;

	ASSERT(GetDocument()==CPrjDoc::m_pReviewDoc);
	CPrjDoc *pDoc=CPrjDoc::m_pReviewDoc;

 	UINT flags;

	if(m_bExternUpdate) {
		CMapFrame *pFrame=pDoc->FrameNodeExists(CPrjDoc::m_pReviewNode);
		if(!pFrame) {
			ASSERT(0);
			return;
		}
		if(pFrame!=pDoc->m_pMapFrame) {
			pFrame->SetDocMapFrame();
		}
		flags=(pFrame->m_flags&M_TOGGLEFLAGS);
	}
	else flags=(
		GetCheck(IDC_EXTERNMARKERS)*M_MARKERS
		+GetCheck(IDC_EXTERNLABELS)*M_NAMES
		+GetCheck(IDC_EXTERNFLAGS)*M_FLAGS
		+GetCheck(IDC_EXTERNNOTES)*M_NOTES
		+GetCheck(IDC_WALLS)*M_LRUDS
		//+GetCheck(IDC_TRAVSELECTED)*M_SELECTED
	);

	#ifndef _DEBUG
	CPrjDoc::PlotFrame(flags);
	#else
	   int n1=CPrjDoc::m_pReviewDoc->GetFrameCnt();
	   CPrjDoc::PlotFrame(flags);
	   int n2=CPrjDoc::m_pReviewDoc->GetFrameCnt();
	   ASSERT(n2-n1==(m_bExternUpdate?0:1));
	#endif
}

void CPlotView::OnUpdateFrame()
{
	if(!GetDocument()->m_pMapFrame) return;
	m_bExternUpdate=1;
	OnExternFrame();
	m_bExternUpdate=0;
} 

BOOL CPlotView::OnPreparePrinting(CPrintInfo* pInfo)
{
	UINT nPages=GetPageView()->GetNumPages();

    pInfo->SetMaxPage(nPages);
    
    // default preparation - opens dialog and creates CDC
    BOOL bRet=DoPreparePrinting(pInfo);

	if(bRet) {
		CDC dc;
		dc.Attach(pInfo->m_pPD->m_pd.hDC);
		//Compute size of frame in device pixels (m_sizePrint) --
    
		int hres=dc.GetDeviceCaps(HORZRES);   //3150 px (10.5 in) Also defines initial clipbox
		int vres=dc.GetDeviceCaps(VERTRES);   //2400 px (8 in)
		int pxperinX=dc.GetDeviceCaps(LOGPIXELSX);
		int pxperinY=dc.GetDeviceCaps(LOGPIXELSY);
		int thick=m_mfPrint.iFrameThick;
		
		m_fPrintAspectRatio=(double)pxperinY/pxperinX;

		VIEWFORMAT *pVF=&m_vfSaved[m_bProfile];

		m_sizePrint.cx=(int)(pVF->fFrameWidth*pxperinX);
		m_sizePrint.cy=(int)(pVF->fFrameHeight*pxperinY);

		m_iOverlapX=(int)(pVF->fOverlap*pxperinX);
		m_iOverlapY=(int)(pVF->fOverlap*pxperinY);

		if(pVF->fPageOffsetX==FLT_MIN) {
			offX=(hres-m_sizePrint.cx)/2;
			offY=(vres-m_sizePrint.cy)/2;
		}
		else {
			offX=(int)(pVF->fPageOffsetX*pxperinX);
			offY=(int)(pVF->fPageOffsetY*pxperinY);
		}
		if(offX<thick || (offX+m_sizePrint.cx)>hres-thick ||
		   offY<thick || (offY+m_sizePrint.cy)>vres-thick)
		     bRet=(IDOK==CMsgBox(MB_OKCANCEL,IDS_ERR_PRINTSCALE2,
			   (double)hres/pxperinX,(double)vres/pxperinY,
			   pInfo->m_bPreview?"preview":"print"));
		dc.Detach();
    }

	return bRet;
}

void CPlotView::OnBeginPrinting(CDC *pDC, CPrintInfo *pInfo)
{
    // Initialization before printing first page
    // (after setup of portrait/landscape, etc) --
    
    AfxGetApp()->BeginWaitCursor();
   
    // pInfo->m_strPageDesc="Frame %u\nFrames %u-%u\n";
    
#if 0
    //Results for LJ IIIp --
    // pInfo->m_rectDraw: left=29330, top=9871, right=20628, bottom=29737 
    // Next 4 are reversed for portrait mode --
    
    int i=pDC->GetDeviceCaps(HORZSIZE);  //267 mm  (266.7?)
        i=pDC->GetDeviceCaps(VERTSIZE);  //203 mm  (203.2?)
	    hres=pDC->GetDeviceCaps(HORZRES);   //3150 px -- Also defines initial clipbox
	    vres=pDC->GetDeviceCaps(VERTRES);   //2400 px (8 in)
	    
	    i=pDC->GetDeviceCaps(LOGPIXELSX);//300  px/in
	    i=pDC->GetDeviceCaps(LOGPIXELSY);//300  px/in
	    i=pDC->GetDeviceCaps(ASPECTX);   //300
	    i=pDC->GetDeviceCaps(ASPECTY);   //300
    
    	i=pDC->GetDeviceCaps(CLIPCAPS);  //1
    int j=i&CP_RECTANGLE;                //1
    	j=i&CP_REGION;                   //0
        
    	i=pDC->GetDeviceCaps(RASTERCAPS);//9883
    	j=i&RC_BITBLT;                   //1
    	j=i&RC_DEVBITS;                  //0
    	j=i&RC_STRETCHDIB;               //8192 (0x2000)
    	
    	i=pDC->GetDeviceCaps(LINECAPS);  //2
        j=i&LC_POLYLINE;                 //2 
#endif
}

void CPlotView::OnPrint(CDC *pDC,CPrintInfo *pInfo)
{
	//Must be done here, not in OnBeginPrinting(). This defines initial clipbox --
	pDC->SetViewportOrg(offX,offY);

	//For use by CPrjDoc::PlotFrame
	m_pDCPrint=pDC;
	m_nCurPage=pInfo->m_nCurPage;
	 
	CPrjDoc::PlotFrame(M_PRINT +
	  GetCheck(IDC_EXTERNMARKERS)*M_MARKERS+
	  GetCheck(IDC_EXTERNLABELS)*M_NAMES+
	  GetCheck(IDC_EXTERNFLAGS)*M_FLAGS+
	  GetCheck(IDC_EXTERNNOTES)*M_NOTES+
	  GetCheck(IDC_WALLS)*M_LRUDS);
}

void CPlotView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
	AfxGetApp()->EndWaitCursor();
}

void CPlotView::OnPanelDefault()
{
	//Return to the optimal view and scale at the current
	//plan/profile orientation --
	DisableButton(IDC_DEFAULT);
	GetDlgItem(IDC_ZOOMIN)->SetFocus();
	m_iView=-1;
	pCV->PlotTraverse(-2);
	//Enable Recall only if there is a saved view.
	//Save will NOT be enabled because the Default button is now disabled --
	EnableRecall(m_vfSaved[m_bProfile].iComponent!=0);
}

void CPlotView::OnChgView()
{
    VIEWFORMAT vf;
	char name[NET_SIZNAME+2];

	ClearTracker();
	
	CPrjDoc::InitViewFormat(&vf);
	//if(!m_bProfile) vf.fCenterUp=m_vfSaved[0].fCenterUp; //already done
	ASSERT(m_bProfile || vf.fCenterUp==m_vfSaved[0].fCenterUp);
	vf.bChanged=FALSE;
	
	CViewDlg dlg(&vf,CPrjDoc::GetNetNameOfOrigin(name),this);
	dlg.m_bSaveView=CPrjDoc::m_bSaveView;

	if(dlg.DoModal()==IDOK) {
		ASSERT(!m_bTrackerActive);
		if(vf.bChanged) {
			if(dlg.m_bInches!=m_bInches) {
				m_bInches=dlg.m_bInches;
				pGV->RefreshPageInfo();
			}
			m_pViewFormat=&vf;
			pCV->PlotTraverse(-2);
			m_pViewFormat=NULL;
			if(CPrjDoc::m_bSaveView=dlg.m_bSaveView) {
				vf.bActive=FALSE; //Causes bChanged to be set TRUE
				m_vfSaved[m_bProfile]=vf;
				EnableRecall(FALSE);
			}
			else {
				if(vf.bChanged) {
					m_vfSaved[m_bProfile].bChanged=TRUE;
					m_vfSaved[m_bProfile].fFrameWidth=vf.fFrameWidth;
					m_vfSaved[m_bProfile].fFrameHeight=vf.fFrameHeight;
				}
				EnableRecall(TRUE);
			}
			//Insure page units of both types are remembered --
			m_vfSaved[0].bFlags&=~VF_INCHUNITS;
			m_vfSaved[0].bFlags|=m_bInches;
			m_vfSaved[1].bFlags&=~VF_INCHUNITS;
			m_vfSaved[1].bFlags|=m_bInches;
			CPrjDoc::PlotPageGrid(0);
			pGV->RefreshScale();
		}
	}
}

void CPlotView::OnPlanProfile()
{
	//Return to default scale at opposing plan/profifle orientation
	//while retaining the current view direction --

	if(m_vfSaved[m_bProfile].bActive) {
		m_vfSaved[m_bProfile].bActive=FALSE;
		m_vfSaved[m_bProfile].bChanged=TRUE;
	}
	LabelProfileButton(m_bProfile=!m_bProfile);
	if(!pCV->m_bZoomedTrav || pCV->m_bSeg<1 || !pSV->IsTravVisible()) {
	  m_pViewFormat=&m_vfSaved[m_bProfile];
	  pCV->PlotTraverse(-2);
	  m_pViewFormat=NULL;
	  EnableRecall(m_vfSaved[m_bProfile].iComponent!=0);
	}
	else {
	  pCV->m_bHomeTrav=TRUE;
	  pCV->PlotTraverse(0);
	  pCV->m_bHomeTrav=FALSE;
	}
}

void CPlotView::OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint)
{
	//No updating from CPrjDoc for now.
	//We must override this to prevent other UpdateAllView() broadcasts
	//from invalidating the entire view (?)
}

void CPlotView::OnExternGridlines()
{
	pSV->SetGridVisible(GetCheck(IDC_EXTERNGRIDLINES));
	pSV->m_bChanged=TRUE;
	GetDocument()->RefreshMaps();
}

void CPlotView::OnGridOptions()
{
	char name[NET_SIZNAME+1];
	CGridDlg dlg(pSV->m_pGridFormat,CPrjDoc::GetNetNameOfOrigin(name),this);
	if(dlg.DoModal()==IDOK && dlg.m_bChanged) {
		pSV->m_bChanged=TRUE;
		GetDocument()->RefreshMaps();
	}
}

void CPlotView::OnWalls() 
{
	BOOL bVisible=GetCheck(IDC_WALLS);
	/*
	if(bVisible) pSV->m_StyleHdr[HDR_FLOOR].flags|=LRUD_ENABLE;
	else pSV->m_StyleHdr[HDR_FLOOR].flags&=~LRUD_ENABLE;
	*/
	pSV->SetOutlinesVisible(bVisible);
	pSV->m_bChanged=TRUE;
}

void CPlotView::OnRotateRight()
{
    m_iView=(int)fView-15;
	if(m_iView<0) m_iView+=360;
	pCV->PlotTraverse(-2);
}

void CPlotView::OnRotateLeft()
{
    m_iView=(int)fView+15;
	if(m_iView>=360) m_iView-=360;
	pCV->PlotTraverse(-2);
}

void CPlotView::OnTravSelected()
{
	BOOL bVisible=GetCheck(IDC_TRAVSELECTED);

	ASSERT(bVisible || pSV->IsTravSelected() && pSV->IsTravVisible());

	SetCheck(IDC_TRAVFLOATED, FALSE);
	pSV->SetTravSelected(TRUE); //leave bullet label as "traverse"
	//pSV->SetRawTravVisible(TRUE); //attach raw traverse
	pSV->SetTravVisible(bVisible); //attach/detach bullet
	GetDocument()->RefreshMaps();
}

void CPlotView::OnTravFloated()
{
	BOOL bVisible=GetCheck(IDC_TRAVFLOATED);

	ASSERT(bVisible && (pSV->IsTravSelected() || !pSV->IsTravVisible()) ||
	      !bVisible && !pSV->IsTravSelected() && pSV->IsTravVisible());

	pSV->SetTravSelected(!bVisible); //toggle bullet between traverse and floated
	//pSV->SetRawTravVisible(!bVisible);
	pSV->SetTravVisible(bVisible);	 //attach/detach bullet
	SetCheck(IDC_TRAVSELECTED,FALSE);
	GetDocument()->RefreshMaps();
}

void CPlotView::OnTravToggle()
{
	pCV->TravToggle();
	pCV->PlotTraverse(0);
	GetDocument()->RefreshMapTraverses(TRV_SEL);
}

void CPlotView::OnUpdateTravToggle(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(pCV->m_iStrFlag>0 && (pCV->m_iStrFlag&(NET_STR_FLTMSKV|NET_STR_FLTMSKH))!=0);
}

void CPlotView::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
	if(nIDCtl==IDC_ROTATELEFT || nIDCtl==IDC_ROTATERIGHT) { 
			CDC* pDC = CDC::FromHandle(lpDIS->hDC);
		    CRect rect(&lpDIS->rcItem);
		    int select=(lpDIS->itemState&ODS_SELECTED)!=0;
		    int focus=(lpDIS->itemState&ODS_FOCUS)!=0;
		    int yOffset=select?ROT_BUTTON_Y:0;
		    int xOffset=(nIDCtl==IDC_ROTATERIGHT)?ROT_BUTTON_X:0;
		    pDC->StretchBlt(0,0,rect.Width(),rect.Height(),&m_dcRotBmp,
				xOffset,yOffset,ROT_BUTTON_X,ROT_BUTTON_Y,SRCCOPY);
		    if(focus && !select) {
			      rect.InflateRect(-3,-3);
			      pDC->DrawFocusRect(&rect);
		    }
	}
	else {
		ASSERT(FALSE);
	}
}

void CPlotView::OnEditFind()
{
	ClearTracker();
	CPltFindDlg dlg(this);

	if(dlg.DoModal()==IDOK) {
		//Locate() was successful --
		if(pCV->GoToComponent()>1) {
			pCV->PlotTraverse(0);
			VERIFY(!dbNTP.Go(CPrjDoc::m_iFindStation));
		}
		CPrjDoc::LocateStation();
	}
}

void CPlotView::OnFindPrevNext(int iNext)
{
	BOOL bFound=TRUE;
	int irec=0;

	ClearTracker();
	if(CPrjDoc::m_iFindStation) {
		ASSERT(CPrjDoc::m_iFindStation);
		if(CPrjDoc::m_iFindStationMax>1) {
			bFound=CPrjDoc::FindStation(hist_pltfind.GetFirstString(),CPltFindDlg::m_bMatchCase,iNext);
		}
		irec=CPrjDoc::m_iFindStation;
	}
	else {
		ASSERT(CPrjDoc::m_iFindVector);
		if(CPrjDoc::m_iFindVectorMax>1) {
			char names[SIZ_EDIT2NAMEBUF]; //Max len in dialog is 40
			strcpy(names,hist_netfind.GetFirstString());
			ASSERT(cfg_quotes==0);
			cfg_quotes=0;
			if(cfg_GetArgv(names,CFG_PARSE_ALL)<2 ||
				!CPrjDoc::FindVector(CNetFindDlg::m_bMatchCase,iNext)) {
				bFound=FALSE;
			}
		}
		irec=CPrjDoc::m_iFindVector;
	}

	if(bFound) {
		VERIFY(!dbNTP.Go(irec));
		if(!CPrjDoc::m_iFindStation && abs(CPrjDoc::m_iFindVectorIncr)<2) {
			pCV->GoToVector();
			pCV->PlotTraverse(0);
		}
		else if(pCV->GoToComponent()>1) {
			pCV->PlotTraverse(0);
		}

		if(!CPrjDoc::m_iFindStation) {
			if(!CPrjDoc::m_iFindVectorDir) irec--;
			VERIFY(!dbNTP.Go(irec));
			if(!pPLT->end_id) {
				if(!CPrjDoc::m_iFindVectorDir) VERIFY(!dbNTP.Next());
				else VERIFY(!dbNTP.Prev());
			}
		}
		else VERIFY(!dbNTP.Go(irec)); 
		CPrjDoc::LocateStation();
	}
	else {
		ASSERT(FALSE);
		CPrjDoc::m_iFindStation=CPrjDoc::m_iFindVector=0;
	}
}

void CPlotView::OnFindNext()
{
	OnFindPrevNext(1);
}

void CPlotView::OnUpdateFindNext(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(CPrjDoc::m_iFindStation!=0 /*|| CPrjDoc::m_iFindVector!=0*/);
}

void CPlotView::OnFindPrev() 
{
	OnFindPrevNext(-1);
}

void CPlotView::OnUpdateFindPrev(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(CPrjDoc::m_iFindStation!=0 && CPrjDoc::m_iFindStationMax>1);
	/* || CPrjDoc::m_iFindVector!=0 && CPrjDoc::m_iFindVectorMax>1);*/
}

void CPlotView::OnFilePrint()
{
	CPrjDoc::PlotPageGrid(0);
	if(GetPageView()->CheckSelected()) {
		((CWallsApp *)AfxGetApp())->SetLandscape(-2);
		CView::OnFilePrint();
	}
	else GetReView()->switchTab(TAB_PAGE);
}

void CPlotView::OnFilePrintPreview()
{
	CPrjDoc::PlotPageGrid(0);
	if(GetPageView()->CheckSelected()) {
		if(((CWallsApp *)AfxGetApp())->SetLandscape(-1)>0) CView::OnFilePrintPreview();
	}
	else GetReView()->switchTab(TAB_PAGE);
}

void CPlotView::OnEndPrintPreview(CDC* pDC,CPrintInfo* pInfo,POINT point,CPreviewView* pView)
{
	CView::OnEndPrintPreview(pDC,pInfo,point,pView);
}

void CPlotView::SetTravChecks(BOOL bTravSelected,BOOL bVisible)
{
	//Turns exactly one of them on, or both of them off --
	SetCheck(bTravSelected?IDC_TRAVSELECTED:IDC_TRAVFLOATED,bVisible);
	SetCheck(bTravSelected?IDC_TRAVFLOATED:IDC_TRAVSELECTED,FALSE); 
}

void CPlotView::OnFlagToggle()
{
	pSV->SetNewVisibility(TRUE);
}

void CPlotView::ToggleNoteFlag(UINT id,UINT flag)
{
	pSV->SetNewVisibility(TRUE);
	if(GetCheck(id)) m_vfSaved[0].bFlags|=flag;
	else m_vfSaved[0].bFlags&=~flag;
	m_vfSaved[0].bChanged=TRUE;
}

void CPlotView::OnExternNotes()
{
	ToggleNoteFlag(IDC_EXTERNNOTES,VF_VIEWNOTES);
}

void CPlotView::OnExternFlags()
{
	ToggleNoteFlag(IDC_EXTERNFLAGS,VF_VIEWFLAGS);
}

#ifdef _SAV_LBL
void CPlotView::OnExternLabels()
{
	ToggleNoteFlag(IDC_EXTERNLABELS, VF_VIEWLABEL);
}

void CPlotView::OnExternMarkers()
{
	ToggleNoteFlag(IDC_EXTERNMARKERS, VF_VIEWMARK);
}
#endif

void CPlotView::OnDisplayOptions()
{
	CMapDlg dlg(m_bPrinterOptions); //Start with last set worked on?
	dlg.DoModal();
	m_bPrinterOptions=dlg.m_pMF->bPrinter;
}


HBRUSH CPlotView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
    if(nCtlColor==CTLCOLOR_STATIC) {
      switch(pWnd->GetDlgCtrlID()) {
        case IDC_VIEW: nCtlColor=CTLCOLOR_EDIT;
        			   break;
        case IDC_WIDTH:
			pDC->SetTextColor(m_bTrackerActive?RGB_WHITE:RGB_DKRED);
			pDC->SetBkColor(::GetSysColor(m_bTrackerActive?COLOR_3DSHADOW:COLOR_3DFACE));
			return ::GetSysColorBrush(m_bTrackerActive?COLOR_3DSHADOW:COLOR_3DFACE);
      }
    }

	return CPanelView::OnCtlColor(pDC, pWnd, nCtlColor);
}

static BOOL WriteWMF(HANDLE hf,PLACEABLEMETAHEADER *plhdr)
{
	//Now modify SetWindowExt() function --
	METAHEADER *pHdr=(METAHEADER *)((BYTE *)plhdr+sizeof(PLACEABLEMETAHEADER));
	METARECORD *pRec=(METARECORD *)((BYTE *)pHdr+sizeof(METAHEADER));
	DWORD MaxRecord=0;
	METARECORD *pRecEnd=(METARECORD *)((WORD *)pHdr+pHdr->mtSize);
//	DWORD sizTotal=sizeof(METAHEADER);
	
	while(pRec<pRecEnd) {
		if(pRec->rdFunction==META_SETWINDOWEXT) {
			pRec->rdParm[0]=plhdr->Bottom;
			pRec->rdParm[1]=plhdr->Right;
		}
		if(pRec->rdFunction==META_ESCAPE) pHdr->mtSize-=pRec->rdSize;
		else {
		  //sizTotal+=2*pRec->rdSize;
		  if(pRec->rdSize>MaxRecord) MaxRecord=pRec->rdSize;
		}

		pRec=(METARECORD *)((WORD *)pRec+pRec->rdSize);
	}
	pHdr->mtMaxRecord=MaxRecord;

	//ASSERT(sizTotal==2*pHdr->mtSize);

	//Now write the file --
	void *pByte=plhdr;
	pRec=(METARECORD *)((BYTE *)pHdr+sizeof(METAHEADER));
	MaxRecord=0;
	BOOL bNoFail=TRUE;

	while(pRec<pRecEnd) {
		if(pRec->rdFunction==META_ESCAPE) {
			if(pByte) {
			  if(!(bNoFail=WriteFile(hf,pByte,(BYTE *)pRec-(BYTE *)pByte,&MaxRecord,NULL))) break;
			  pByte=NULL;
			}
		}
		else if(!pByte) pByte=pRec;
		pRec=(METARECORD *)((WORD *)pRec+pRec->rdSize);
	}
	if(pByte && bNoFail) bNoFail=WriteFile(hf,pByte,(BYTE *)pRec-(BYTE *)pByte,&MaxRecord,NULL);
	return bNoFail;
}

static BOOL SaveWinMetaFile(HENHMETAFILE hmet,HDC refDC,LPCSTR pathname)
{
	ENHMETAHEADER hdr;
	PLACEABLEMETAHEADER *plhdr=NULL;
	BOOL bRet=FALSE;

	UINT size=::GetWinMetaFileBits(hmet,0,NULL,MM_ANISOTROPIC,refDC);
	::GetEnhMetaFileHeader(hmet,sizeof(hdr),&hdr);
	if((plhdr=(PLACEABLEMETAHEADER *)malloc(size+sizeof(PLACEABLEMETAHEADER))) &&
		(size==::GetWinMetaFileBits(hmet,size,(BYTE *)plhdr+sizeof(PLACEABLEMETAHEADER),MM_TEXT,refDC))) {
		plhdr->Key=0x9AC6CDD7;
		plhdr->Handle=0;
		plhdr->Left=(SHORT)hdr.rclBounds.left;
		plhdr->Top=(SHORT)hdr.rclBounds.top;
		plhdr->Right=(SHORT)hdr.rclBounds.right;
		plhdr->Bottom=(SHORT)hdr.rclBounds.bottom;
		plhdr->Inch=CPlotView::m_uMetaRes;
		plhdr->Reserved=0;
		plhdr->Checksum=0;
		for(WORD *ptr=(WORD *)plhdr;ptr<(WORD *)&plhdr->Checksum;ptr++) {
		  plhdr->Checksum ^= *ptr;
		}

		//Now create WMF file --
		HANDLE hf;
		if((hf=::CreateFile(pathname,GENERIC_WRITE,0,NULL,
			CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL))!=INVALID_HANDLE_VALUE) {
			bRet=WriteWMF(hf,plhdr);
			::CloseHandle(hf);
		}
	}
	free(plhdr);
	return bRet;
}

static BOOL MyCreateEnhMetaFile(CMetaFileDC *mfDC,CDC *refDC,LPCSTR szFileName,  // Metafile filename
						LPCSTR szDesc,
                        DWORD dw01mmX,    // Width in .01mm
                        DWORD dw01mmY,    // Height in .01mm
                        DWORD dwDPI,
						BOOL bEMF)       // DPI (logical units)
{
   RECT     Rect;
   HDC      hScreenDC;
   int		viewX,viewY,winX,winY;
   double   PixelsX,PixelsY;

   // dw01mmX x dw01mmY in .01mm units
   SetRect( &Rect, 0, 0, dw01mmX, dw01mmY);

   // Get a Reference DC
   hScreenDC = GetDC(NULL);
   refDC->Attach(hScreenDC);

   // Get the physical characteristics of the reference DC
   PixelsX = (double)GetDeviceCaps( hScreenDC, HORZRES );
   PixelsY = (double)GetDeviceCaps( hScreenDC, VERTRES );
   winX=(int)(dw01mmX*dwDPI/2540.0);
   winY=(int)(dw01mmY*dwDPI/2540.0);
   viewX=(int)(dw01mmX*0.01*PixelsX/(double)GetDeviceCaps( hScreenDC, HORZSIZE ));
   viewY=(int)(dw01mmY*0.01*PixelsY/(double)GetDeviceCaps( hScreenDC, VERTSIZE ));

   //if(!bEMF) {
	   SetMapMode(refDC->m_hDC,MM_ANISOTROPIC);
	   SetWindowExtEx(refDC->m_hDC,winX,winY,NULL);
	   SetViewportExtEx(refDC->m_hDC,viewX,viewY,NULL);
   //}

   // Create the Metafile
   if(!mfDC->CreateEnhanced(refDC, szFileName, &Rect, szDesc)) {
	   ReleaseDC(NULL,refDC->Detach());
       return FALSE;
   }
/*
   if(bEMF) {
	   SetMapMode(mfDC->m_hDC,MM_ANISOTROPIC);
	   SetWindowExtEx(mfDC->m_hDC,winX,winY,NULL);
	   SetViewportExtEx(mfDC->m_hDC,viewX,viewY,NULL);
   }
*/
   mfDC->SetAttribDC(refDC->m_hDC);

   return TRUE;
}

void CPlotView::OnMapExport() 
{
	char buf[_MAX_PATH]; //At least 128
	CMapExpDlg exdlg;

	//Get default pathname --
	exdlg.m_pathname=GetDocument()->WorkPath(CPrjDoc::m_pReviewNode,CPrjDoc::TYP_WMF);
	exdlg.m_defpath=GetDocument()->WorkPath();

	if(exdlg.DoModal()!=IDOK) return;

	AfxGetApp()->m_pMainWnd->UpdateWindow();

	//Let's see if the directory needs to be created --
	if(!DirCheck((char *)(const char *)exdlg.m_pathname,TRUE)) return;

	AfxGetApp()->BeginWaitCursor();

	BOOL bEMF=!_stricmp(trx_Stpext(exdlg.m_pathname),"." NET_EMF_EXT);
	
	UINT width;

	//Let's determine a suitable dpi based on frame dimensions --
	width=(UINT)m_vfSaved[m_bProfile].fFrameWidth+1;
	if(width<(UINT)m_vfSaved[m_bProfile].fFrameHeight)
		width=(UINT)m_vfSaved[m_bProfile].fFrameHeight+1;

	m_uMetaRes=MAX_META_RES; /*=600, MAX_FRAME_PIXELS=32767*/

	while(m_uMetaRes*width>MAX_FRAME_PIXELS) m_uMetaRes-=20;
	if(m_uMetaRes<=0) {
		//Should not happen in this version (trapped in options dialog) --
		AfxMessageBox(IDS_ERR_METARES);
		return;
	}

	m_sizePrint.cx=(int)(m_vfSaved[m_bProfile].fFrameWidth*m_uMetaRes+0.5);
	m_sizePrint.cy=(int)(m_vfSaved[m_bProfile].fFrameHeight*m_uMetaRes+0.5);
	m_fPrintAspectRatio=1.0;

	LPCTSTR pathname=NULL,title=NULL;

	if(bEMF) {
	  pathname=exdlg.m_pathname;
	  title=buf;
      strcpy(buf,"Walls 2.0");
	  strcpy(buf+10,CPrjDoc::m_pReviewNode->Title());
	  buf[strlen(buf+10)+9+2]=0;
	}

	CDC refDC;
	CMetaFileDC mfDC;

	if(MyCreateEnhMetaFile(&mfDC,&refDC,pathname,title,
		(int)(2540.0*m_sizePrint.cx/m_uMetaRes+0.5),
		(int)(2540.0*m_sizePrint.cy/m_uMetaRes+0.5),
		m_uMetaRes,bEMF)) {

		m_pDCPrint=&mfDC; //For use by CPrjDoc::PlotFrame

		CPrjDoc::PlotFrame(M_PRINT + M_METAFILE +
		  GetCheck(IDC_EXTERNMARKERS)*M_MARKERS+
		  GetCheck(IDC_EXTERNLABELS)*M_NAMES+
		  GetCheck(IDC_EXTERNFLAGS)*M_FLAGS+
		  GetCheck(IDC_EXTERNNOTES)*M_NOTES+
		  GetCheck(IDC_WALLS)*M_LRUDS);

		HENHMETAFILE hmet=mfDC.CloseEnhanced();
		
		if(hmet) {
			if(!bEMF) bEMF=SaveWinMetaFile(hmet,refDC.m_hDC,exdlg.m_pathname);
			::DeleteEnhMetaFile(hmet);
		}
		else bEMF=FALSE;

		//Detach and release the reference DC (presumably a screen DC?) --

		::ReleaseDC(NULL,refDC.Detach());
	}
	else bEMF=FALSE;

	//dc.DeleteDC();

	AfxGetApp()->EndWaitCursor();

	CMsgBox(MB_ICONEXCLAMATION,bEMF?IDS_MAPEXP_OK:IDS_MAPEXP_BAD,_strupr(trx_Stpnam(exdlg.m_pathname)));
}

void CPlotView::DisableButton(UINT id)
{
	//This Message call apparently removes the black outline --
	//at least under Win95. We should probably check WinNT as well.
	//Only by experiment was this found to be necessary!
	GetDlgItem(id)->SendMessage(BM_SETSTYLE,BS_PUSHBUTTON,TRUE);
	Enable(id,FALSE);
}

void CPlotView::EnableRecall(BOOL bEnable)
{
	if(m_vfSaved[m_bProfile].bActive==bEnable) {
		//We are either activating or deactivating the saved view --
		m_vfSaved[m_bProfile].bChanged=TRUE;
	}

	//Assuming Default button set correctly, we won't allow saving the default
	if(!bEnable) {
	  DisableButton(IDC_SAVE);
	  DisableButton(IDC_RECALL);
	  //Either default or saved view must be active (only reason to disable Save/Recall) --
	  m_vfSaved[m_bProfile].bActive=
		  (GetDlgItem(IDC_DEFAULT)->IsWindowEnabled() && m_vfSaved[m_bProfile].iComponent!=0);
	}
	else {
	  //Don't allow saving of the default view --
	  if(GetDlgItem(IDC_DEFAULT)->IsWindowEnabled()) Enable(IDC_SAVE,bEnable);
	  else DisableButton(IDC_SAVE);
	  bEnable=(m_vfSaved[m_bProfile].iComponent!=0);
	  if(bEnable) Enable(IDC_RECALL,bEnable);
	  else DisableButton(IDC_RECALL);
	  //If we're enabling Recall OR no saved view exists, no saved view can be active --
	  m_vfSaved[m_bProfile].bActive=FALSE;
	}
}

void CPlotView::OnRecall() 
{
	ASSERT(GetDlgItem(IDC_RECALL)->IsWindowEnabled());
	ASSERT(m_iView==-2);
	ASSERT(m_vfSaved[m_bProfile].iComponent!=0);
	ClearTracker(FALSE); //No need to display panel width at this point

	m_pViewFormat=&m_vfSaved[m_bProfile];
	pCV->PlotTraverse(-2); //Will EnableRecall(TRUE)
	m_pViewFormat=NULL;
	EnableRecall(FALSE);
}

void CPlotView::OnSave() 
{
	ASSERT(GetDlgItem(IDC_SAVE)->IsWindowEnabled());
	ClearTracker(TRUE); //Need to redisplay panel width

	if(!GetDlgItem(IDC_DEFAULT)->IsWindowEnabled()) {
		ASSERT(FALSE);
		return;
	}
	/*
	if(!GetDlgItem(IDC_DEFAULT)->IsWindowEnabled())
		m_vfSaved[m_bProfile].iComponent=0;
	else 
		CPrjDoc::InitViewFormat(&m_vfSaved[m_bProfile]);
	*/

	CPrjDoc::InitViewFormat(&m_vfSaved[m_bProfile]);
	m_vfSaved[m_bProfile].bChanged=TRUE;

	//Sets m_vfSaved[m_bProfile].bActive=TRUE since both IDC_DEFAULT is enabled
	//and also m_vfSaved[m_bProfile].iComponent!=0 --
	EnableRecall(FALSE);
	//m_vfSaved[m_bProfile].bActive=TRUE;
}


void CPlotView::OnUpdateUTM(CCmdUI* pCmdUI)
{
	BOOL bEnable=m_bCursorInBmp;
    pCmdUI->Enable(bEnable);
	if(bEnable) {
		CString s;
		if(pCmdUI->m_nID==ID_INDICATOR_SEL) {
			CPrjDoc::m_pReviewNode->RefZoneStr(s);
		}
		else {
#ifndef VISTA_INC
			if(m_bMeasuring) GetDistanceFormat(s,m_bProfile,LEN_ISFEET());
			else
#endif
		    GetCoordinateFormat(s,dpCoordinate,m_bProfile,LEN_ISFEET());
		}
		pCmdUI->SetText(s);
	}
}

LRESULT CPlotView::OnMouseLeave(WPARAM wParam,LPARAM strText)
{
	/*
	if(m_bMeasuring) {
		RefreshMeasure();
		m_bMeasuring=FALSE;
#ifdef VISTA_INC
		DestroyTrackingTT();
#endif
	}
	m_bCursorInBmp=m_bTracking=FALSE;
	*/
	return FALSE;
}

void CPlotView::OnPltPanToHere() 
{
	DPOINT pt((double)(ptPan.x-m_ptBmp.x)/m_xscale,(double)(ptPan.y-m_ptBmp.y)/m_xscale);
    m_pPanPoint=&pt;
	pCV->PlotTraverse(-2);
	m_pPanPoint=NULL;
}

BOOL CPlotView::ZoomOutOK()
{
	if((1.0+(PERCENT_ZOOMOUT/100.0))*m_fPanelWidth>1000000.0) {
		AfxMessageBox(IDS_ERR_PAGEZOOMOUT);
		return FALSE;
	}
	return TRUE;
}

BOOL CPlotView::ZoomInOK()
{
	if((PERCENT_ZOOMOUT/100.0)*m_fPanelWidth<1.0) {
		AfxMessageBox(IDS_ERR_PAGEZOOM);
		return FALSE;
	}
	return TRUE;
}

void CPlotView::OnPanelZoomIn()
{
	if(!m_bTrackerActive) {
		if(!ZoomInOK()) return;
	}
	else if(!TrackerOK()) return;
	
	if(m_bTrackerActive || pCV->m_bSeg<1 || !pSV->IsTravSelected() ||
	   !pSV->IsTravVisible())
	  pCV->PlotTraverse(1);
	else {
	  pCV->m_bHomeTrav=TRUE;
	  pCV->PlotTraverse(0);
	  pCV->m_bHomeTrav=FALSE;
	}
}

void CPlotView::OnPanelZoomOut()
{
	if(ZoomOutOK()) 
		pCV->PlotTraverse(-1);
}

void CPlotView::OnPltZoomIn() 
{
	if(!ZoomInOK()) return;

	DPOINT pt((double)(ptPan.x-m_ptBmp.x)/m_xscale,(double)(ptPan.y-m_ptBmp.y)/m_xscale);
    m_pPanPoint=&pt;
	pCV->PlotTraverse(1);
	m_pPanPoint=NULL;
}

void CPlotView::OnPltZoomOut() 
{
	if(!ZoomOutOK()) return;

	DPOINT pt((double)(ptPan.x-m_ptBmp.x)/m_xscale,(double)(ptPan.y-m_ptBmp.y)/m_xscale);
    m_pPanPoint=&pt;
	pCV->PlotTraverse(-1);
	m_pPanPoint=NULL;
}

BOOL CPlotView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if(zDelta) {
		//ClearTracker(0);
		ptPan=m_rectBmp.CenterPoint();
		if(zDelta<0) {
			OnPltZoomOut();
		}
		else {
			OnPltZoomIn();
		}
		return TRUE;
	}
	return FALSE;
}

void CPlotView::OnUpdatePltRecall(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDlgItem(IDC_RECALL)->IsWindowEnabled());
}

void CPlotView::OnUpdatePltDefault(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDlgItem(IDC_DEFAULT)->IsWindowEnabled());
}

void CPlotView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if(m_bMeasuring) {
		RefreshMeasure();
		m_bMeasuring=FALSE;
#ifdef VISTA_INC
		DestroyTrackingTT();
#endif
	}
}

void CPlotView::OnMeasureDist() 
{
	bMeasureDist=!bMeasureDist;
}

void CPlotView::OnUpdateMeasureDist(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(bMeasureDist);
}

void CPlotView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if(m_bPanning) {
		m_bPanning=false;
		if(m_bPanned) {
			bPanValidated=TRUE;
			m_bPanned=false;
			return;
		}
	}
	CMenu menu;
	if(m_bCursorInBmp && !(nFlags&MK_CONTROL) && menu.LoadMenu(IDR_PLT_CONTEXT))
	{
		CPoint pt(point);
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		ClientToScreen(&point);
		int e=pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON|TPM_RETURNCMD,point.x,point.y,GetParentFrame());
		if(e) {
			ClearTracker();
			ptPan=pt;
			SendMessage(WM_COMMAND,e);
		}
		else {
			bPanValidated=FALSE;
		}
	}
}

BOOL CPlotView::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		CWnd *pWnd=GetFocus();
		if(pWnd) {
			UINT id=pWnd->GetDlgCtrlID();
			if(id==IDC_ZOOMIN) {
				PostMessage(WM_COMMAND,IDC_ZOOMIN);
				return 1;
			}
		}
	}
	return CFormView::PreTranslateMessage(pMsg);
}

void CPlotView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    if(m_bTrackerActive) {
		CRect rect=m_tracker.m_rect; //Does not include border and handles as does GetTrueRect()
		rect.NormalizeRect();
		if(rect.PtInRect(point)) {
		  OnPanelZoomIn();
		  return;
		}
    }
    if(m_bCursorInBmp) {
    	//Pan image --
		DPOINT pt((double)(point.x-m_ptBmp.x)/m_xscale,(double)(point.y-m_ptBmp.y)/m_xscale);
    	m_pPanPoint=&pt;
		m_bTrackerActive=FALSE; //Should already be the case
    	pCV->PlotTraverse(-2);
    	m_pPanPoint=NULL;
    }
}

void CPlotView::OnRButtonDown(UINT nFlags, CPoint point)
{
	if(m_bCursorInBmp) {
		if(!(nFlags&MK_CONTROL)) {
			m_bPanning=true;
			m_bPanned=false;
		}
		ClearTracker();
		bPanValidated=TRUE;
		ptPan=point;
    }
}

#ifdef VISTA_INC
void CPlotView::ShowTrackingTT(CPoint &point)
{
	CString s;
	GetDistanceFormat(s,m_bProfile,LEN_ISFEET(),false); //conversion possibly needed for CMapView windows only
	g_toolItem.lpszText = s.GetBuffer();
	::SendMessage(g_hwndTrackingTT,TTM_SETTOOLINFO, 0, (LPARAM)&g_toolItem);
	// Position the tooltip. The coordinates are adjusted so that the tooltip does not overlap the mouse pointer.
	ClientToScreen(&point);
	::SendMessage(g_hwndTrackingTT,TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(point.x + 10, point.y - 20));
}
#endif

void CPlotView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rectSave;
	int ht;
	
	if(m_bCursorInBmp) {
		if(m_bTrackerActive) {
			ht=m_tracker.HitTest(point);
			m_tracker.GetTrueRect(rectSave); //Includes sizing handles
			if(ht>=0) {
			    //Some part of the tracker was grabbed --
			    if(m_tracker.Track(this, point,TRUE)) {
					// The tracking was committed --
					InvalidateRect(&rectSave);
					m_tracker.GetTrueRect(rectSave);
					if(rectSave.Width()<m_sizeBmp.cx) InvalidateRect(&rectSave);
					else {
					  //We have expanded tracker to full panel size --
					  m_bTrackerActive=FALSE;
					  ht=2;  //Tracker deleted. No rubberband mode needed --
					}
				}
				else {
				    //The tracker change was cancelled with the escape key
				    //or right mouse button. Leave status unchanged --
				    ht=CRectTracker::hitMiddle;
				}
			}
			else {
			    //We clicked outside of tracker, cancelling it --
			    m_bTrackerActive=FALSE;
			    InvalidateRect(&rectSave);
			}
			if(ht>=0) {
			  if(ht==CRectTracker::hitMiddle) return;
			  ht=1;
			}
		}
		else ht=0;
		
		//At this point the status is:
		//ht =  2  - An existing tracker was deleted - do not enter rubberband mode
		//ht =  1  - An existing tracker changed in size - do not enter rubberband mode
		//ht =  0  - No tracker previously active - enter rubberband mode
		//ht = -1  - An existing tracker was deleted - enter rubberband mode
		
	    if(ht<=0) {
	        if(nFlags&MK_CONTROL) {
				bPanValidated=TRUE;
				ptPan=point;
		    }
			else if(bMeasureDist) {
				if(m_bMeasuring) {
					ASSERT(0);
					RefreshMeasure();
					m_bMeasuring=FALSE;
			        #ifdef VISTA_INC
					   DestroyTrackingTT();
                    #endif
				}
				if(!(nFlags&MK_RBUTTON)) {
					CPrjDoc::GetCoordinate(dpCoordinate,point,FALSE);
					dpCoordinateMeasure=dpCoordinate;
					ptMeasureStart=ptMeasureEnd=point;
					m_bMeasuring=true;
#ifdef VISTA_INC
					ASSERT(!g_hwndTrackingTT);
					if(!g_hwndTrackingTT) {
						g_hwndTrackingTT=CreateTrackingToolTip(m_hWnd);
					}
					if(g_hwndTrackingTT) {
						//position tooltip --
						ShowTrackingTT(point);
						//Activate the tooltip.
						::SendMessage(g_hwndTrackingTT,TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&g_toolItem);
					}
#endif
					return;
				}
			}
		    else if(m_tracker.TrackRubberBand(this,point,TRUE)) {
				// The tracking was committed --
				m_tracker.GetTrueRect(rectSave);
				if(rectSave.Width()<m_sizeBmp.cx) {
					m_bTrackerActive=rectSave.Width()>=MIN_TRACKERWIDTH;
					InvalidateRect(&rectSave);
					ht--;
				}
	        }
		}
		
		if(ht) {
			if(m_bTrackerActive) GetTrackerPos(NULL); //Initialize new width
			DisplayPanelWidth();
		}
		return;
	}
	CPanelView::OnLButtonDown(nFlags,point);
} 

void CPlotView::OnMouseMove(UINT nFlags, CPoint point)
{
	//TODO: Add your message handler code here and/or call default
	//CPanelView::OnMouseMove(nFlags, point);
	BOOL bCursorInBmp=m_rectBmp.PtInRect(point);
	if(bCursorInBmp!=m_bCursorInBmp) {
		if(bCursorInBmp) ::SetCursor(bMeasureDist?CMapView::m_hCursorMeasure:m_hCursorBmp);
		m_bCursorInBmp=bCursorInBmp;
	}
	
	if(!m_bCursorInBmp) {
		if(m_bMeasuring) {
			RefreshMeasure();
			m_bMeasuring=FALSE;
	#ifdef VISTA_INC
			DestroyTrackingTT();
	#endif
		}
		CPanelView::OnMouseMove(nFlags, point);
		return;
	}

	CPrjDoc::GetCoordinate(dpCoordinate, point, FALSE);

	if(m_bMeasuring) {

		if(ptMeasureEnd==point)
			return;

		RefreshMeasure();
		ptMeasureEnd=point;
		RefreshMeasure();


#ifdef VISTA_INC
		if(g_hwndTrackingTT) {
			ShowTrackingTT(point);
		}
#endif
		return;
	}

	if(m_bPanning && !(nFlags&MK_RBUTTON)) {
		m_bPanning=m_bPanned=false;
	}

	if(bPanValidated && (m_bPanning || (nFlags&(MK_CONTROL+MK_LBUTTON+MK_RBUTTON))>MK_CONTROL)) {
    	point.x-=ptPan.x;
    	point.y-=ptPan.y;
    	if(point.x || point.y) {
    	    if(m_bPanning || (nFlags&MK_LBUTTON)) {
    			//Pan image --
				DPOINT pt((double)(m_sizeBmp.cx/2.0-point.x)/m_xscale,(double)(m_sizeBmp.cy/2.0-point.y)/m_xscale);
	    	    ptPan.x+=point.x;
	    	    ptPan.y+=point.y;
		    	m_pPanPoint=&pt;
				//m_bTrackerActive=FALSE; //Should already be the case
		    	pCV->PlotTraverse(-2);
		    	m_pPanPoint=NULL;
	    		bPanValidated=FALSE;
				if(m_bPanning) m_bPanned=true;
		    }
		    else if(point.x) {
    			//Rotate image in one degree increments --
			    int i=(int)(fView+(180.0*point.x)/m_sizeBmp.cx);
			    if(i>=360) i-=360;
			    else if(i<0) i+=360;
			    ASSERT(i>=0 && i<360);
				if(m_iView!=i) {
				  m_iView=i;
		    	  ptPan.x+=point.x;
		    	  //ptPan.y+=point.y;
				  pCV->PlotTraverse(-2);
	    		  bPanValidated=FALSE;
	    		}
		    }
	    }
	}
}
