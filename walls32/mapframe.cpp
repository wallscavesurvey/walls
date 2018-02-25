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
	m_bResizing=false;
	m_dwReviewNetwork=0;
	m_pFrameNode=NULL;
	m_pDoc=NULL;
}

CMapFrame::~CMapFrame()
{
	delete m_pBmp;
	delete m_pVnode;
	delete m_pVnodeF;
}

BEGIN_MESSAGE_MAP(CMapFrame, CMDIChildWnd)
	ON_WM_CLOSE()
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_WM_ICONERASEBKGND()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCMOUSEMOVE()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

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

    if(m_pBmp && m_pFrameNode!=CPrjDoc::m_pReviewNode)
		lpMMI->ptMaxTrackSize=lpMMI->ptMinTrackSize=m_ptSize;
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

void CMapFrame::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	switch(nHitTest) {
	case HTRIGHT:
	case HTBOTTOM:
	case HTBOTTOMRIGHT:
	case HTBOTTOMLEFT:
	case HTTOPRIGHT:
	case HTTOP:
	case HTLEFT:
		m_bResizing=true;
		break;
	}

	CMDIChildWnd::OnNcLButtonDown(nHitTest, point);
}

void CMapFrame::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if(m_bResizing) {
		m_pView->SendMessage(WM_EXITSIZEMOVE);
		m_bResizing=false;
	}
	CMDIChildWnd::OnNcMouseMove(nHitTest, point);
}
void CMapFrame::SetTransform(UINT flags)
{
	m_flags=flags;
	CPrjDoc::InitTransform(this); //also sets m_bProfile to bProfile
	m_bFeetUnits=LEN_ISFEET();
	CMapView *pView=(CMapView *)GetActiveView();
	if(pView) {
		pView->m_bProfile=bProfile;
		pView->m_bFeetUnits=m_bFeetUnits;
		pView->m_bIdentify=(m_pVnode || m_pVnodeF);
	}
}

void CMapFrame::ShowTitle()
{
	ASSERT(m_bFeetUnits==LEN_ISFEET());
	char buf[256];
	char view[6];
	strcpy(view, GetFloatStr(m_fView, 1));

	CString csBase;
	pCV->GetNetworkBaseName(csBase);
	_snprintf(buf, 256, "%s%s  -  %s: %s\xB0  Width: %s %s", m_pFrameNode->Title(), (LPCSTR)csBase,
		m_bProfile?"Profile":"Plan", view, GetFloatStr(LEN_SCALE(m_sizeBmp.cx/m_scale), 1), LEN_UNITSTR());
	SetWindowText(buf);
}

void CMapFrame::InitializeFrame(CBitmap *pBmp, CSize size, double xoff, double yoff, double scale)
{
	ASSERT(!pPV->m_bExternUpdate || pPV->m_bExternUpdate>1);

	m_pBmp=pBmp;
	m_sizeBmp=size;
	m_pView=(CMapView *)GetActiveView();
	m_pView->m_rectBmp.SetRect(0, 0, size.cx, size.cy);
	CRect cr;
	m_pView->GetClientRect(&cr);
	m_pView->m_sizeClient=CSize(cr.right, cr.bottom);

	//This frame can be zoomed if m_pFrameNode==CPrjDoc::m_pReviewNode --
	m_pFrameNode=CPrjDoc::m_pReviewNode;
	m_dwReviewNetwork=dbNTN.Position();
}

void CMapFrame::SetFrameSize(CSize &sz)
{
	//To control resizing of this frame --
	m_ptSize.x=sz.cx+2*::GetSystemMetrics(SM_CXFRAME); //== 720+2*4
	m_ptSize.y=sz.cy+2*::GetSystemMetrics(SM_CYFRAME)+
		::GetSystemMetrics(SM_CYCAPTION); //== 516+2*4+19 == 543
}

void CMapFrame::RemoveThis()
{
	if(!m_pDoc || !m_pDoc->m_pMapFrame) {
		return;
	}

	#ifdef _DEBUG
	int n1=m_pDoc->GetFrameCnt();
	ASSERT(n1>=1);
	#endif

	CMapFrame *pPrev=NULL;
	CMapFrame *pThis=m_pDoc->m_pMapFrame;
	while(pThis && pThis!=this) {
		pPrev=pThis;
		pThis=pThis->m_pNext;
	}
	if(pThis) {
		if(pPrev) pPrev->m_pNext=m_pNext;
		else {
			ASSERT(this==m_pDoc->m_pMapFrame);
			m_pDoc->m_pMapFrame=m_pNext;
		}
	}
    #ifdef _DEBUG
	int n2=m_pDoc->GetFrameCnt();
	ASSERT(n2==n1-1);
	#endif
}

/*
void CMapFrame::RemoveThis()
{
	//Delete this frame from CPlotView's linked list of frames that could 
	ASSERT(m_pDoc);
	if(m_pDoc && m_pDoc->m_pMapFrame) {

		CMapFrame *pPrev=NULL;
		CMapFrame *pThis=m_pDoc->m_pMapFrame;

		while(pThis && pThis!=this) {
			pPrev=pThis;
			pThis=pThis->m_pNext;
		}
		//If pThis==NULL, this frame belongs to a set of orphaned frames, which is OK --
		if(pThis) {
			if(pPrev) pPrev->m_pNext=m_pNext;
			else m_pDoc->m_pMapFrame=m_pNext;
		}
	}
}
*/

void CMapFrame::SetDocMapFrame()
{
	//Place this frame at top of linked list with m_pDoc->m_pMapFrame==this --
	ASSERT(m_pDoc);
	if(m_pDoc->m_pMapFrame!=this) {
		RemoveThis();
		m_pNext=m_pDoc->m_pMapFrame;
		m_pDoc->m_pMapFrame=this;
	}
}

void CMapFrame::OnClose()
{
	if(m_pDoc) {
	    ASSERT(CPrjDoc::m_pReviewDoc!=m_pDoc || m_pDoc->m_pMapFrame);
		RemoveThis();
		if(m_pFrameNode==CPrjDoc::m_pReviewNode && !m_pDoc->FrameNodeExists(m_pFrameNode)) {
			ASSERT(pPV->IsEnabled(IDC_UPDATEFRAME));
			pPV->Enable(IDC_UPDATEFRAME, FALSE);
		}
		m_pDoc=NULL;
	}
	CMDIChildWnd::OnClose();
}
