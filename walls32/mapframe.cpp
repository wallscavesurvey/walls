// mapframe.cpp : implementation file

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "mapframe.h"
#include "compview.h"
#include "segview.h"
#include "compile.h"
#include "vnode.h"
#include "mapview.h"
#include "afxpriv.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapFrame

IMPLEMENT_DYNCREATE(CMapFrame, CMDIChildWnd)

BOOL CMapFrame::m_bKeepVnode=FALSE;

CMapFrame::CMapFrame()
{
	m_pBmp=NULL;
	m_pNext=NULL;
	m_pVnode=m_pVnodeF=NULL;
}

void CMapFrame::SetTitle(const char *pTitle,UINT flags)
{
	char buf[256];
	char view[6];

	m_flags=flags;
	if(!pSV->IsLinesVisible()) m_flags|=M_NOVECTORS;

	CPrjDoc::InitTransform(this);
	strcpy(view,GetFloatStr(m_fView,1));
	_snprintf(buf,256,"%s  -  %s: %s\xB0  Width: %s %s",pTitle,
	  m_bProfile?"Profile":"Plan",view,GetFloatStr(LEN_SCALE(m_sizeBmp.cx/m_scale),1),LEN_UNITSTR());    
	m_bFeetUnits=LEN_ISFEET();
	SetWindowText(buf);
	CMapView *pView=(CMapView *)GetActiveView();
	if(pView) pView->m_bIdentify=(m_pVnode || m_pVnodeF);
}

void CMapFrame::InitializeFrame(CBitmap *pBmp,CSize size,double xoff,double yoff,double scale)
{
    //Things that may be useful -- only repainting needed for now --
	
	//Prepainted bitmap (deleted in ~CMapFrame) --
	m_pBmp=pBmp;
	
	//To use with SetScrollSizes() in view class --
	m_sizeBmp=size;

	//To restore original view --
	m_org_xoff=xoff;
	m_org_yoff=yoff;
	m_org_scale=scale;

	//This frame can be zoomed if m_pFrameNode==CPrjDoc::m_pReviewNode --
	m_pFrameNode=CPrjDoc::m_pReviewNode;
	
	//To control resizing of this frame --
	
    m_ptSize.x=size.cx+2*::GetSystemMetrics(SM_CXFRAME); //== 720+2*4
	m_ptSize.y=size.cy+2*::GetSystemMetrics(SM_CYFRAME)+
		::GetSystemMetrics(SM_CYCAPTION); //== 516+2*4+19 == 543
}

void CMapFrame::RemoveThis()
{
	//Delete this frame from CPlotView's linked list of frames that could 
	//possibly be updated from CSegView::OnExternUpdate() --
	if(CPlotView::m_pMapFrame) {
	  
	  CMapFrame *pPrev=NULL;
	  CMapFrame *pThis=CPlotView::m_pMapFrame;
	  
	  while(pThis && pThis!=this) {
	    pPrev=pThis;
	    pThis=pThis->m_pNext;
	  }
	  //If pThis==NULL, this frame belongs to a set of orphaned frames, which is OK --
	  if(pThis) {
	    if(pPrev) pPrev->m_pNext=m_pNext;
	    else CPlotView::m_pMapFrame=m_pNext;
	  }
	}
}
/*
void CMapFrame::DetachFrameSet()
{
	//Orphan all frames to prevent zooming from any of them.
	//This is invoked when a new frame size is selected in MapDlg.cpp.
	CMapFrame *pNext=CPlotView::m_pMapFrame;
	while(pNext) {
		pNext->m_pFrameNode=(CPrjListNode *)(-1);
		pNext=pNext->m_pNext;
	}
	CPlotView::m_pMapFrame=NULL;
}
*/
CMapFrame::~CMapFrame()
{
	delete m_pBmp;
	delete m_pVnode;
	delete m_pVnodeF;

	RemoveThis();
	
	if(CPlotView::m_pMapFrame==NULL) {
		if(pCV) pCV->EnableUpdateButtons();
	}
}

BEGIN_MESSAGE_MAP(CMapFrame, CMDIChildWnd)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_WM_ICONERASEBKGND()
	//{{AFX_MSG_MAP(CMapFrame)
	ON_WM_GETMINMAXINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapFrame message handlers

BOOL CMapFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.
	
	cs.style&=~(LONG)FWS_ADDTOTITLE;
    cs.style&=~(LONG)WS_MAXIMIZEBOX;
    //cs.style&=~(LONG)WS_CHILD;
	if(!CMDIChildWnd::PreCreateWindow(cs)) return FALSE;
	cs.dwExStyle&=~WS_EX_CLIENTEDGE;
	return TRUE;	  
}

void CMapFrame::OnIconEraseBkgnd(CDC* pDC)
{
    CMainFrame::IconEraseBkgnd(pDC);
}

void CMapFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	CMDIChildWnd::OnGetMinMaxInfo(lpMMI);
    if(m_pBmp) lpMMI->ptMaxTrackSize=m_ptSize;
}

MAP_VECNODE *CMapFrame::GetVecNode(CPoint point)
{
	MAP_VECNODE *pVN=NULL;

	if(m_pVnodeF && (pVN=m_pVnodeF->GetVecNode(point.x,point.y))) return pVN;
	if(m_pVnode) pVN=m_pVnode->GetVecNode(point.x,point.y);
	return pVN;
}

LRESULT CMapFrame::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(112,HELP_CONTEXT);
	return TRUE;
}
