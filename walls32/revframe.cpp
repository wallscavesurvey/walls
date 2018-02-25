// revframe.cpp : implementation of the CRevFrame class
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "compview.h"
#include "revframe.h"
#include "review.h"
#include "Compile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRevFrame

IMPLEMENT_DYNCREATE(CRevFrame, CMDIChildWnd)

//#define SIZ_WINDOW_NAME 20
//#define SIZ_FILE_EXT 3  //must be fixed

BEGIN_MESSAGE_MAP(CRevFrame, CMDIChildWnd)
    ON_MESSAGE(WM_SETTEXT,OnSetText)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_WM_ICONERASEBKGND()
	//{{AFX_MSG_MAP(CRevFrame)
	ON_WM_GETMINMAXINFO()
	ON_WM_WINDOWPOSCHANGED()
	ON_COMMAND(ID_ESCAPEKEY, OnEscapeKey)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CRevFrame::CRevFrame()
{
	m_ptFrameSize.x=0;
}

/////////////////////////////////////////////////////////////////////////////
// CRevFrame message handlers

BOOL CRevFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.
	
	cs.style&=~(LONG)FWS_ADDTOTITLE;
    cs.style&=~(LONG)WS_MAXIMIZEBOX;
    cs.style&=~(LONG)WS_THICKFRAME;
 
	if(!CMDIChildWnd::PreCreateWindow(cs)) return FALSE;
    m_bIconTitle=FALSE;
	return TRUE;	  
}

void CRevFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	//CMDIChildWnd::OnGetMinMaxInfo(lpMMI);
	if(m_ptFrameSize.x)
      lpMMI->ptMaxTrackSize=lpMMI->ptMinTrackSize=m_ptFrameSize;
}

void CRevFrame::OnIconEraseBkgnd(CDC* pDC)
{
    CMainFrame::IconEraseBkgnd(pDC);
}

void CRevFrame::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos)
{
	CMDIChildWnd::OnWindowPosChanged(lpwndpos);
	if(m_bIconTitle!=IsIconic()) SetWindowText("");
}

LRESULT CRevFrame::OnSetText(WPARAM wParam,LPARAM strText)
{
  if(!pCV->m_bBaseName_filled) {
	  return TRUE;
  }
  m_bIconTitle=IsIconic();
  CPrjListNode *pNode=CPrjDoc::m_pReviewNode;
  LPSTR pStr=(LPSTR)(m_bIconTitle?pNode->Name():pNode->Title());
  CString csRevTitle(CPrjDoc::m_pReviewDoc->Title());
  CString csBaseName;
  pCV->GetNetworkBaseName(csBaseName);
  if(!pNode->IsRoot() || !csBaseName.IsEmpty()) {
	  csRevTitle.AppendFormat(" - %s%s",pStr,csBaseName);
  }
  return DefWindowProc(WM_SETTEXT,wParam,(LPARAM)(LPCSTR)csRevTitle);
}

void CRevFrame::CenterWindow()
{
    // Center window in parent's client area. Note that CWnd::CenterWindow()
    // works only for non-child windows, whose treatment by SetWindowPos() is
    // different (it assumes client coorinates for child windows) --
    
	// rcParent is the rectange we should center ourself in
	CRect rc;
	GetParent()->GetClientRect(&rc);
	GetParent()->ClientToScreen(&rc);
	
	// find ideal center point in screen coordinates --
	int xMid = (rc.left + rc.right) / 2;
	int yMid = (rc.top + rc.bottom) / 2;

	// determine this window's upper left corner --
	GetWindowRect(&rc);
	CPoint corner(xMid - rc.Width() / 2, yMid - rc.Height() / 2);
	GetParent()->ScreenToClient(&corner);
	if(corner.x<0) corner.x=0;
	if(corner.y<0) corner.y=0;
	
	SetWindowPos(NULL, corner.x, corner.y, -1, -1, SWP_NOSIZE | SWP_NOZORDER);
}

void CRevFrame::OnEscapeKey()
{
	PostMessage(WM_SYSCOMMAND,SC_CLOSE);
}

LRESULT CRevFrame::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	int nTab=((CPanelView *)GetActiveView())->m_pTabView->GetTabIndex();
	AfxGetApp()->WinHelp(nTab+1,HELP_CONTEXT);
	return TRUE;
}

BOOL bJustActivated=FALSE;
void CRevFrame::ActivateFrame(int nCmdShow)
{   bJustActivated =TRUE;
	CMDIChildWnd::ActivateFrame(CPrjDoc::m_bReviewing?SW_HIDE:nCmdShow);
}
