// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "walls2D.h"

#include <wininet.h>
#include <mshtml.h>
#include "MainFrm.h"
#include "walls2DDoc.h"
#include "walls2DView.h"

// Include files needed WebBrowser Command Group ID
#include <initguid.h>
#include "WBCmdGroup.h"
#include "windowpl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_WM_TIMER()
	ON_COMMAND(ID_GO_WALLS, OnGoWalls)
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_VIEW_STOP, OnViewStop)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STOP, OnUpdateViewStop)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
	ON_COMMAND(ID_FILE_PAGE_SETUP, OnFilePageSetup)
	ON_COMMAND(ID_COORD_PAGE, OnCoordPage)
	ON_UPDATE_COMMAND_UI(ID_COORD_PAGE, OnUpdateCoordPage)
	//}}AFX_MSG_MAP
	ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, OnDropDown)
	ON_COMMAND(ID_FONT_DROPDOWN, DoNothing)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_UTM
	//ID_INDICATOR_CAPS,
	//ID_INDICATOR_NUM,
	//ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CImageList img;
	CString str;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndReBar.Create(this))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndToolBar.CreateEx(this))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	// set up toolbar properties
	m_wndToolBar.GetToolBarCtrl().SetButtonWidth(50, 150);
	m_wndToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

	img.Create(IDB_HOTTOOLBAR, 22, 0, RGB(255, 0, 255));
	m_wndToolBar.GetToolBarCtrl().SetHotImageList(&img);
	img.Detach();
	img.Create(IDB_COLDTOOLBAR, 22, 0, RGB(255, 0, 255));
	m_wndToolBar.GetToolBarCtrl().SetImageList(&img);
	img.Detach();
	m_wndToolBar.ModifyStyle(0, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT);

	m_wndToolBar.SetButtons(NULL, 16);

	// set up each toolbar button
	m_wndToolBar.SetButtonInfo(0, ID_FILE_OPEN, TBSTYLE_BUTTON, 3);
	m_wndToolBar.SetButtonText(0, "Open");

	m_wndToolBar.SetButtonInfo(1, ID_FILE_PRINT, TBSTYLE_BUTTON, 7);
	m_wndToolBar.SetButtonText(1, "Print");

	m_wndToolBar.SetButtonInfo(2, 0, TBBS_SEPARATOR, 20);

	m_wndToolBar.SetButtonInfo(3, ID_VIEW_HOME, TBSTYLE_BUTTON, 2);
	m_wndToolBar.SetButtonText(3, "Home");

	m_wndToolBar.SetButtonInfo(4, ID_VIEW_STOP, TBSTYLE_BUTTON, 0);
	m_wndToolBar.SetButtonText(4, "Back");

	m_wndToolBar.SetButtonInfo(5, 0, TBBS_SEPARATOR, 20);

	m_wndToolBar.SetButtonInfo(6, ID_SHOW_WALLS, TBSTYLE_BUTTON, 8);
	m_wndToolBar.SetButtonText(6, "Walls");

	m_wndToolBar.SetButtonInfo(7, ID_SHOW_ADDITIONS, TBSTYLE_BUTTON, 11);
	m_wndToolBar.SetButtonText(7, "Detail");

	m_wndToolBar.SetButtonInfo(8, 0, TBBS_SEPARATOR, 20);

	m_wndToolBar.SetButtonInfo(9, ID_SHOW_VECTORS, TBSTYLE_BUTTON, 5);
	m_wndToolBar.SetButtonText(9, "Survey");

	m_wndToolBar.SetButtonInfo(10, ID_SHOW_STATIONS, TBSTYLE_BUTTON, 4);
	m_wndToolBar.SetButtonText(10, "Labels");

	m_wndToolBar.SetButtonInfo(11, ID_SHOW_FLAGS, TBSTYLE_BUTTON, 12);
	m_wndToolBar.SetButtonText(11, "Flags");

	m_wndToolBar.SetButtonInfo(12, ID_SHOW_NOTES, TBSTYLE_BUTTON, 13);
	m_wndToolBar.SetButtonText(12, "Notes");

	m_wndToolBar.SetButtonInfo(13, ID_SHOW_GRID, TBSTYLE_BUTTON, 6);
	m_wndToolBar.SetButtonText(13, "Grid");

	m_wndToolBar.SetButtonInfo(14, 0, TBBS_SEPARATOR, 20);

	m_wndToolBar.SetButtonInfo(15, ID_APP_ABOUT, TBSTYLE_BUTTON, 10);
	m_wndToolBar.SetButtonText(15, "About");

	CRect rectToolBar;

	// set up toolbar button sizes
	m_wndToolBar.GetItemRect(0, &rectToolBar);
	m_wndToolBar.SetSizes(rectToolBar.Size(), CSize(30, 20));

	// create the animation control
	//m_wndAnimate.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 10, 10), this, AFX_IDW_TOOLBAR + 2);
	//m_wndAnimate.Open(IDR_MFCAVI);

	// add the toolbar, and animation to the rebar
	m_wndReBar.AddBar(&m_wndToolBar);
	//m_wndReBar.AddBar(&m_wndAnimate, NULL, NULL, RBBS_FIXEDSIZE | RBBS_FIXEDBMP);

	// set up min/max sizes and ideal sizes for pieces of the rebar
	// set up min/max sizes and ideal sizes for pieces of the rebar
	REBARBANDINFO rbbi;

	rbbi.cbSize = sizeof(rbbi);
	rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_SIZE;
	rbbi.cxMinChild = rectToolBar.Width();
	rbbi.cyMinChild = rectToolBar.Height();
	rbbi.cx = rbbi.cxIdeal = rectToolBar.Width() * 9;
	m_wndReBar.GetReBarCtrl().SetBandInfo(0, &rbbi);
	rbbi.cxMinChild = 0;

	//CRect rectAddress;

	//rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
	//m_wndAddress.GetEditCtrl()->GetWindowRect(&rectAddress);
	//rbbi.cyMinChild = rectAddress.Height() + 10;
	//rbbi.cxIdeal = 200;
	//m_wndReBar.GetReBarCtrl().SetBandInfo(1, &rbbi);

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_FIXED);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
			sizeof(indicators) / sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_csStatText.LoadString(AFX_IDS_IDLEMESSAGE);

	//PostMessage(WM_COMMAND,ID_VIEW_STATUS_BAR);

	ModifyStyle(FWS_PREFIXTITLE, 0);

	return 0;
}

void CMainFrame::HelpMsg()
{
	m_wndStatusBar.SetPaneText(0, m_csStatText, TRUE);
}

CString * CMainFrame::NoSVGMsg()
{
	if (m_csNoSVGMsg.IsEmpty()) m_csNoSVGMsg.LoadString(IDS_ERR_NOSVG);
	m_wndStatusBar.SetPaneText(0, m_csNoSVGMsg, TRUE);
	return &m_csNoSVGMsg;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(0);
	return CFrameWnd::PreCreateWindow(cs);
}

////////////////////////////////////////////////////////////
// This method will call the IOleCommandTarget::Exec
// method to invoke the command target id specified
// for the command group specified.
////////////////////////////////////////////////////////////
HRESULT CMainFrame::ExecCmdTarget(const GUID *pguidCmdGroup, DWORD nCmdID)
{
	LPDISPATCH lpDispatch = NULL;
	LPOLECOMMANDTARGET lpOleCommandTarget = NULL;
	HRESULT hr = E_FAIL;

	lpDispatch = ((CWalls2DView*)GetActiveView())->GetHtmlDocument();
	ASSERT(lpDispatch);

	if (lpDispatch)
	{
		// Get an pointer for the IOleCommandTarget interface.
		hr = lpDispatch->QueryInterface(IID_IOleCommandTarget, (void**)&lpOleCommandTarget);
		ASSERT(lpOleCommandTarget);

		lpDispatch->Release();

		if (SUCCEEDED(hr))
		{
			// Invoke the given command id for the WebBrowser control
			hr = lpOleCommandTarget->Exec(pguidCmdGroup, nCmdID, 0, NULL, NULL);
			lpOleCommandTarget->Release();
		}
	}

	return hr;
}

BOOL CMainFrame::SelectionPresent(void)
{
	HRESULT hr;
	BOOL bret = FALSE;

	IDispatch * pDocDisp = ((CWalls2DView *)GetActiveView())->GetHtmlDocument();

	if (pDocDisp != NULL) {
		IHTMLDocument2* pDoc;
		hr = pDocDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc);
		if (SUCCEEDED(hr))
		{
			IHTMLSelectionObject *pSel;
			hr = pDoc->get_selection(&pSel);
			if (SUCCEEDED(hr)) {
				BSTR bs;
				hr = pSel->get_type(&bs);
				if (SUCCEEDED(hr) && CString(bs) == "Text") bret = TRUE;
				pSel->Release();
			}
			pDoc->Release();
		}
		pDocDisp->Release();
	}

	return bret;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::StopAnimation()
{
	//m_wndAnimate.Stop();
	//m_wndAnimate.Seek(0);
}

void CMainFrame::StartAnimation(BOOL bTimer)
{
	// Start the animation.  This is called when the browser begins to
	// navigate to a new location
	if (bTimer) m_uTimer = SetTimer(0x128, 2000, NULL);
	//m_wndAnimate.Play(0, -1, -1);
}

void CMainFrame::DoNothing()
{
	// this is here only so that the toolbar buttons for the dropdown menus
	// will have a callback, and thus will not be disabled.
}

void CMainFrame::OnDropDown(NMHDR* pNotifyStruct, LRESULT* pResult)
{
	// this function handles the dropdown menus from the toolbar
	NMTOOLBAR* pNMToolBar = (NMTOOLBAR*)pNotifyStruct;
	CRect rect;

	// translate the current toolbar item rectangle into screen coordinates
	// so that we'll know where to pop up the menu
	m_wndToolBar.GetToolBarCtrl().GetRect(pNMToolBar->iItem, &rect);
	rect.top = rect.bottom;
	::ClientToScreen(pNMToolBar->hdr.hwndFrom, &rect.TopLeft());
	if (pNMToolBar->iItem == ID_FONT_DROPDOWN)
	{
		CMenu menu;
		CMenu* pPopup;

		// the font popup is stored in a resource
		menu.LoadMenu(IDR_FONT_POPUP);
		pPopup = menu.GetSubMenu(0);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, rect.left, rect.top + 1, AfxGetMainWnd());
	}
	*pResult = TBDDRET_DEFAULT;
}

void CMainFrame::OnEditCopy()
{
	ExecCmdTarget(NULL, OLECMDID_COPY);
}

void CMainFrame::OnEditFind()
{
	// Invoke the find dialog box
	ExecCmdTarget(&CGID_IWebBrowser, CWBCmdGroup::HTMLID_FIND);
}

void CMainFrame::OnEditSelectAll()
{
	// Select all items in the WebBrowser document
	ExecCmdTarget(NULL, OLECMDID_SELECTALL);
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == m_uTimer) {
		KillTimer(nIDEvent);
		m_uTimer = 0;
		StopAnimation();
	}
	else CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnGoWalls()
{
	theApp.Navigate2Links(IDC_WALLS_URL);
}

void CMainFrame::RestoreWindow(int nShow)
{
	CWindowPlacement wp;
	if (!wp.Restore(this)) ShowWindow(nShow);
}

void CMainFrame::OnDestroy()
{
	CWindowPlacement wp;
	wp.Save(this);
	CFrameWnd::OnDestroy();
}

void CMainFrame::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(SelectionPresent());
}

void CMainFrame::OnViewStop()
{
	// TODO: Add your command handler code here

}

void CMainFrame::OnUpdateViewStop(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

}

void CMainFrame::OnFilePrintPreview()
{
	ExecCmdTarget(NULL, OLECMDID_PRINTPREVIEW);
}

void CMainFrame::OnFilePageSetup()
{
	ExecCmdTarget(NULL, OLECMDID_PAGESETUP);
}

void CMainFrame::OnCoordPage()
{
	CWalls2DApp::m_bCoordPage = !CWalls2DApp::m_bCoordPage;
}

void CMainFrame::OnUpdateCoordPage(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(CWalls2DApp::m_bCoordPage);
}
