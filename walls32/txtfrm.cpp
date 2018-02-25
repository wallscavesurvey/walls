// txtfrm.cpp : implementation of the CTxtFrame class
//
#include "stdafx.h"
#include "walls.h"
#include "linedoc.h"
#include "txtfrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CTxtFrame,CMDIChildWnd)

BEGIN_MESSAGE_MAP(CTxtFrame, CMDIChildWnd)
	ON_WM_ICONERASEBKGND()
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()

BOOL CTxtFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name. 
	// cs.style &= ~(LONG)FWS_ADDTOTITLE;

	if(!CMDIChildWnd::PreCreateWindow(cs)) return FALSE;

	cs.style&=~(LONG)FWS_ADDTOTITLE;

    //if(cs.lpszName) cs.cx=CTxtView::InitialFrameWidth();
    cs.dwExStyle&=~(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE);
    m_bIconTitle=FALSE;
	return TRUE;	  
}

void CTxtFrame::OnIconEraseBkgnd(CDC* pDC)
{
    CMainFrame::IconEraseBkgnd(pDC);
}

void CTxtFrame::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos)
{
	CMDIChildWnd::OnWindowPosChanged(lpwndpos);

	m_bClosed=IsIconic()||IsZoomed();
	
	if(m_bClosed!=m_bIconTitle) {
	  //Refresh the caption? --
	  //CLineDoc *pDoc=(CLineDoc *)GetActiveDocument();
	  m_bIconTitle=m_bClosed;
	}
}
