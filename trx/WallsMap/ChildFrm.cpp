// ChildFrm.cpp : implementation of the CChildFrame class
//
#include "stdafx.h"
#include "WallsMap.h"

#include ".\childfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	ON_WM_NCLBUTTONDOWN()
	//ON_WM_NCLBUTTONUP()
	ON_WM_NCMOUSEMOVE()
END_MESSAGE_MAP()


// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
	m_bResizing=false;
}

CChildFrame::~CChildFrame()
{
}


BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	//cs.style &= ~FWS_ADDTOTITLE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return CMDIChildWnd::PreCreateWindow(cs);
}

// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

void CChildFrame::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	switch(nHitTest) {
		case HTRIGHT :
		case HTBOTTOM :
		case HTBOTTOMRIGHT :
		case HTBOTTOMLEFT :
		case HTTOPRIGHT :
		case HTTOP :
		case HTLEFT :
			m_bResizing=true;
			break;
	}

	CMDIChildWnd::OnNcLButtonDown(nHitTest, point);
}

void CChildFrame::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if(m_bResizing) {
		GetActiveView()->SendMessage(WM_EXITSIZEMOVE);
		m_bResizing=false;
	}
	CMDIChildWnd::OnNcMouseMove(nHitTest, point);
}
