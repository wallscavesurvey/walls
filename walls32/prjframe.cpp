// prjframe.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjframe.h"
#include "prjdoc.h"
#include "prjview.h"
#include "compview.h"
#include "compile.h"
#include "mapframe.h"
#include "itemprop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrjFrame

IMPLEMENT_DYNCREATE(CPrjFrame, CMDIChildWnd)

//#define SIZ_WINDOW_NAME 20
//#define SIZ_FILE_EXT 3  //must be fixed

BEGIN_MESSAGE_MAP(CPrjFrame, CMDIChildWnd)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_WM_ICONERASEBKGND()
	//{{AFX_MSG_MAP(CPrjFrame)
	ON_WM_GETMINMAXINFO()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SYSCOMMAND()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCRBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CPrjFrame::CPrjFrame()
{
}

/////////////////////////////////////////////////////////////////////////////
// CPrjFrame message handlers

BOOL CPrjFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.

	cs.style &= ~(LONG)FWS_ADDTOTITLE;
	cs.style &=~(LONG)WS_MAXIMIZEBOX;
	cs.dwExStyle&=~(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE);

	if(!CMDIChildWnd::PreCreateWindow(cs)) return FALSE;
    m_bIconTitle=FALSE;
	return TRUE;	  
}

void CPrjFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	// TODO: Add your message handler code here and/or call default
	
	CMDIChildWnd::OnGetMinMaxInfo(lpMMI);
    /*
	CPrjDoc *cd=(CPrjDoc *)GetActiveDocument();
	if(cd) {
      ASSERT(cd->IsKindOf(RUNTIME_CLASS(CPrjDoc)));
      lpMMI->ptMaxTrackSize.x=m_FrameSize.x;
      lpMMI->ptMaxTrackSize.y=m_FrameSize.y+cd->GetDataWidth();
    }
    */
}

void CPrjFrame::OnIconEraseBkgnd(CDC* pDC)
{
    CMainFrame::IconEraseBkgnd(pDC);
}

void CPrjFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	//Experimentation alone (double-clicking system menu) revealed
	//requirement to trap undocumented 0xF063 !!??
	if(nID==SC_CLOSE || nID==0xF063) {

		if(hPropHook && pPropView->GetParent()==this) pItemProp->EndItemProp(IDCANCEL);

		CPrjDoc *pDoc=(CPrjDoc *)GetActiveDocument();
		if(pCV && pDoc==CPrjDoc::m_pReviewDoc) {
		  if(pDoc->m_bComputing) return;
		  pCV->Close();
		}
		//Also close any remaining map frames --
		POSITION pos=pDoc->GetFirstViewPosition();
		while (pos != NULL) {
			CView* pView = pDoc->GetNextView(pos);
			ASSERT_VALID(pView);
			CFrameWnd* pFrame = pView->GetParentFrame();
			ASSERT_VALID(pFrame);
			if(pFrame->IsKindOf(RUNTIME_CLASS(CMapFrame))) {
			   ((CMapFrame *)pFrame)->m_pDoc=NULL;
			   pFrame->SendMessage(WM_SYSCOMMAND,SC_CLOSE);
			}
	    }
	}
	CMDIChildWnd::OnSysCommand(nID, lParam);
}

LRESULT CPrjFrame::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(19,HELP_CONTEXT);
	return TRUE;
}

BOOL CPrjFrame::ChkNcLButtonDown(UINT nHitTest)
{
	if(hPropHook) {
		if(!PropOK()) {
			TRACE0("OnNcLButtonDown (!PropOK)\n");
			return FALSE;
		}
		TRACE0("OnNcLButtonDown (PropOK, default)\n");
		Pause();
		if(pPropView->GetParent()!=this) {
			if(HTCLOSE!=nHitTest) {
				pItemProp->PostMessage(WM_PROPVIEWLB,
				(WPARAM)m_pPrjList->GetDocument(),(LPARAM)m_pPrjList->GetSelectedNode());
				Pause();
			}
		}
	}
	return TRUE;
}

void CPrjFrame::OnNcLButtonDown(UINT nHitTest, CPoint point) 
{
	if(ChkNcLButtonDown(nHitTest))
		CMDIChildWnd::OnNcLButtonDown(nHitTest, point);
}


void CPrjFrame::OnNcRButtonDown(UINT nHitTest, CPoint point) 
{
	if(ChkNcLButtonDown(nHitTest))
		CMDIChildWnd::OnNcRButtonDown(nHitTest, point);
}
