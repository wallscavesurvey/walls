// MDIScene.cpp : implementation file
//

#include "stdafx.h"
//#include "scene.h"
#include "MDIScene.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMDISceneWnd

IMPLEMENT_DYNCREATE(CMDISceneWnd, CMDIChildWnd)

CMDISceneWnd::CMDISceneWnd()
{
}

CMDISceneWnd::~CMDISceneWnd()
{
}


BEGIN_MESSAGE_MAP(CMDISceneWnd, CMDIChildWnd)
	//{{AFX_MSG_MAP(CMDISceneWnd)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMDISceneWnd message handlers

BOOL CMDISceneWnd::PreCreateWindow(CREATESTRUCT& cs) 
{

    if (!CMDIChildWnd::PreCreateWindow (cs))
        return FALSE;

    cs.style &= ~FWS_ADDTOTITLE;
    return TRUE;
}

BOOL CMDISceneWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
	//return CView::OnEraseBkgnd(pDC);
}

