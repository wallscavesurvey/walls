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
    ON_MESSAGE(WM_SETTEXT,OnSetText)
	ON_WM_ICONERASEBKGND()
	//{{AFX_MSG_MAP(CTxtFrame)
	ON_WM_WINDOWPOSCHANGED()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CTxtFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name. 
	// cs.style &= ~(LONG)FWS_ADDTOTITLE;

	if(!CMDIChildWnd::PreCreateWindow(cs)) return FALSE;
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
	
	if(m_bClosed!=m_bIconTitle && ((CLineDoc *)GetActiveDocument())->m_pszFrameTitle) {
	  //Refresh the caption (see OnSetText() below) --
	  char strTxt[256];
	  GetWindowText(strTxt,256); //All we need here is the "*:nn" suffix
	  SetWindowText(strTxt);
	}
	m_bIconTitle=m_bClosed;
	
#if 0
	if(!m_bClosed) {
	  //We have changed position but we are NOT in a minimized or maximized state.
	  //If the window's title bar covers more than 50% of the title bar of
	  //an existing window of the same document class, we will make sure that
	  //the existing window is minimized --
	  CRect thisRect;
	  {
	      WINDOWPLACEMENT wp;
		  wp.length=sizeof(wp);
		  GetWindowPlacement(&wp);
	  	  thisRect=wp.rcNormalPosition;
	  }
	  CDocument *pDoc=GetActiveDocument();
	  ASSERT(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CLineDoc)));
	  if(pDoc) {
		  POSITION pos=CMainFrame::GetFirstMdiFrame(pDoc);
		  CRect rect,irect;
		  m_bClosed=TRUE; //Skip this window --
		  int barHeight=GetSystemMetrics(SM_CYSIZE)+GetSystemMetrics(SM_CYFRAME);
		  while(pos) {
		    CTxtFrame *pTxtFrame=(CTxtFrame *)CMainFrame::GetNextMdiFrame(pos,&rect,NULL);
		    if(!pTxtFrame->m_bClosed) {
		      rect.bottom=rect.top+barHeight;
		      if(irect.IntersectRect(&rect,&thisRect) &&
		        2*irect.Width()*irect.Height()>rect.Width()*rect.Height())
		        pTxtFrame->PostMessage(WM_SYSCOMMAND,SC_MINIMIZE);
		    } 
		  }
		  m_bClosed=FALSE;
	  }
	}
#endif
}

LRESULT CTxtFrame::OnSetText(WPARAM wParam,LPARAM strText)
{
  LPSTR pStr=(LPSTR)strText;

  //If NOT iconized we want to substitute the custom title in
  //CLineDoc::m_pszFrameTitle. Otherwise, we want to use the text in
  //CLineDoc::m_pszIconTitle, which we assume to be the "iconic" version.
  //In either case the update flag and/or window count must be properly
  //appended. We assume this suffix is already in strText, since we
  //have left the framework's FWS_ADDTOTITLE processing enabled.
      
  //A non-null custom frame title enables this feature --
  CLineDoc *pDoc=(CLineDoc *)GetActiveDocument();
  if(pDoc->m_pszFrameTitle) {
    m_bIconTitle=IsIconic()||IsZoomed();
 	  
	LPSTR p=strrchr(pStr,'*');
	if(!p && m_nWindow>0) p=strrchr(pStr,':');
	if(m_bIconTitle) {
	  pStr=pDoc->m_pszIconTitle;
	  pStr[pDoc->m_lenIconTitle]=0;
	}
	else {
	  pStr=pDoc->m_pszFrameTitle;
	  pStr[pDoc->m_lenFrameTitle]=0;
	}
	if(p) strcat(pStr,p);
  }
  
  return DefWindowProc(WM_SETTEXT,wParam,(LPARAM)pStr);
} 
