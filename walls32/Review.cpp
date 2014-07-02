// review.cpp : implementation of the CReView class
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "review.h"
#include "compview.h"
#include "plotview.h"
#include "pageview.h"
#include "traview.h"
#include "segview.h"
#include "compile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReView

IMPLEMENT_DYNCREATE(CReView, CTabView)

BEGIN_MESSAGE_MAP(CReView, CTabView)
	//{{AFX_MSG_MAP(CReView)
	//}}AFX_MSG_MAP
	// Standard printing commands
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReView construction/destruction

FONT_SPEC CReView::font_spec;
BOOL CReView::m_bFontScaled=FALSE;
      
CReView::CReView()
{
}

CReView::~CReView()
{
}

BOOL CALLBACK SetControlFont(HWND hControl,LPARAM pSpec)
{
   int i=::GetDlgCtrlID(hControl);
   int *pID=((lpFONT_SPEC)pSpec)->pDlgFont_id;
   
   for(;*pID;pID++) if(*pID==i) {
	   ::SendMessage(hControl,WM_SETFONT,(WPARAM)((lpFONT_SPEC)pSpec)->hFont,FALSE);
	   break;
   }
   return TRUE;
}

void CReView::SetFontsBold(HWND hWnd,int *pCtl)
{
	font_spec.hFont=(HFONT)GetBoldFont()->GetSafeHandle();
	font_spec.pDlgFont_id=pCtl;
    VERIFY(::EnumChildWindows(hWnd,(WNDENUMPROC)SetControlFont,(LPARAM)(lpFONT_SPEC)&font_spec));
}

/////////////////////////////////////////////////////////////////////////////
// CReView diagnostics

#ifdef _DEBUG
void CReView::AssertValid() const
{
	CTabView::AssertValid();
}

void CReView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPrjDoc* CReView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPrjDoc)));
	return (CPrjDoc*) m_pDocument;
}
#endif //_DEBUG

HBRUSH CReView::m_hBrush;
HBRUSH CReView::m_hPltBrush;

CPen CReView::m_PenGrid;
CPen CReView::m_PenBorder;
CPen CReView::m_PenTravSel;
CPen CReView::m_PenTravRaw;
CPen CReView::m_PenFragRaw;
CPen CReView::m_PenTravFrag;
CPen CReView::m_PenNet;
CPen CReView::m_PenSys;
CPen CReView::m_PenSysSel;

PRJFONT CReView::m_ReviewFont;
static char BASED_CODE szReviewFont[] = "Font for Review Tables";

void CReView::Initialize()
{
    //Let's try calculating an appropriate default fixed-pitch font --
    
    CLogFont lf;
 
    lf.Init("Courier New",90,FW_REGULAR,FF_MODERN+FIXED_PITCH);
    
    m_ReviewFont.Load(szReviewFont,FALSE,&lf);
    //m_ReviewFont.LineHeight+=2;
	m_bFontScaled=FALSE;

	//m_hPltBrush=::CreateSolidBrush(RGB_LTGRAY);
	m_hPltBrush=(HBRUSH)::GetStockObject(LTGRAY_BRUSH);

	m_hBrush=::CreateSolidBrush(SRV_DLG_BACKCOLOR);
	
	m_PenGrid.CreatePen(PS_SOLID,0,GetSysColor(COLOR_BTNFACE));
	m_PenBorder.CreatePen(PS_SOLID,0,GetSysColor(COLOR_BTNSHADOW));
	
	m_PenNet.CreatePen(PS_SOLID,0,SRV_PEN_NETCOLOR);
	m_PenSys.CreatePen(PS_SOLID,0,SRV_PEN_SYSCOLOR);
	m_PenSysSel.CreatePen(PS_SOLID,0,SRV_PEN_SYSSELCOLOR);
	m_PenTravSel.CreatePen(PS_SOLID,0,SRV_PEN_TRAVSELCOLOR);
	m_PenTravRaw.CreatePen(PS_SOLID,0,SRV_PEN_TRAVRAWCOLOR);
	m_PenFragRaw.CreatePen(PS_SOLID,0,SRV_PEN_TRAVFRAGRAWCOLOR);
	m_PenTravFrag.CreatePen(PS_SOLID,0,SRV_PEN_TRAVFRAGCOLOR);
}

void CReView::ChangeFont()
{
	if(!m_ReviewFont.Change(CF_FIXEDPITCHONLY|CF_LIMITSIZE|CF_NOSIZESEL|CF_FORCEFONTEXIST|
	   CF_SCALABLEONLY|CF_NOOEMFONTS|CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT)) return;
    //m_ReviewFont.LineHeight+=2;
	m_bFontScaled=FALSE;
	if(pCV) {
	  pCV->UpdateFont();
	  pTV->UpdateFont();
	}
}

void CReView::Terminate()
{
	::DeleteObject(m_hBrush);
	::DeleteObject(m_hPltBrush);
    m_ReviewFont.Save();
}

void CReView::ResetContents()
{
    GetCompView()->ResetContents();
    GetTraView()->ResetContents();
    GetSegView()->ResetContents();
    GetPlotView()->ResetContents();
	GetPageView()->ResetContents();
}

void CReView::OnInitialUpdate()
{
    //Get rid of 3D border --
	ModifyStyleEx(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE,0);
    {
		CCompView* pCompView = (CCompView *)addTabView(RUNTIME_CLASS(CCompView),GetDocument(),
			"&Geometry");
		ASSERT(pCompView != NULL);
		pCompView->m_pTabView = this;
	}
	{
		CTraView* pTraView = (CTraView *)addTabView(RUNTIME_CLASS(CTraView),GetDocument(),
			"&Traverse");
		ASSERT(pTraView != NULL);
		pTraView->m_pTabView = this;
	}
	{
		CPlotView* pPlotView = (CPlotView *)addTabView(RUNTIME_CLASS(CPlotView),GetDocument(),
			"&Map");
		ASSERT(pPlotView != NULL);
		pPlotView->m_pTabView = this;
	}
	{
		CPageView* pPageView = (CPageView *)addTabView(RUNTIME_CLASS(CPageView),GetDocument(),
			"&Page Layout");
		ASSERT(pPageView != NULL);
		pPageView->m_pTabView = this;
	}
	{
		CSegView* pSegView = (CSegView *)addTabView(RUNTIME_CLASS(CSegView),GetDocument(),
			"&Segments");
		ASSERT(pSegView != NULL);
		pSegView->m_pTabView = this;
	}
	
	CTabView::OnInitialUpdate();
}

void CReView::switchTab(int viewIndex)
{
	static int bLandscape=0;

	if(m_tabArray.m_curTab==viewIndex) return;
	if(m_tabArray.m_curTab==TAB_PLOT) GetPlotView()->ClearTracker();
	else if(m_tabArray.m_curTab==TAB_SEG) {
		GetSegView()->SelectSegments();
	}

	switch(viewIndex) {

		case TAB_PAGE :
			  if(!GetSegView()->InitSegTree()) return;
			  if(!bLandscape) theApp.SetLandscape(2);
			  bLandscape=1;
			  GetCompView()->PlotTraverse(0);
			  CPrjDoc::PlotPageGrid(0);
			  GetPageView()->RefreshScale();
			  break;

		case TAB_PLOT :
				{
				  if(!GetSegView()->InitSegTree()) return;
				  pCV->PlotTraverse(0);
				  if(pCV->m_bHomeTrav) {
					GetSegView()->SetTravSelected(TRUE); //View selected vs floated
					GetSegView()->SetTravVisible(TRUE);  //Attach Traverse bullet
					GetPlotView()->EnableMarkers(); //Turn on Traverse and Markers checkboxes
				  }
				} 
				break;

		case TAB_SEG  :
			  if(!GetSegView()->InitSegTree()) return;
			  break;

		case TAB_TRAV :
			  if(!GetCompView()->SetTraverse()) return;
			  break;
	}
	
	CTabView::switchTab(viewIndex);
}
