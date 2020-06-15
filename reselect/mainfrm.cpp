// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "reselect.h"
#include <wininet.h>

#include <mshtml.h>
#include "MainFrm.h"
#include "reselectDoc.h"
#include "reselectVw.h"

// Include files needed WebBrowser Command Group ID
#include <initguid.h>
#include "WBCmdGroup.h"
#include "windowpl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WID_ADDRESS 52575

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_COMMAND(ID_SHOW_ADDRESS, OnShowAddress)
	ON_UPDATE_COMMAND_UI(ID_SHOW_ADDRESS, OnUpdateShowAddress)
	ON_WM_TIMER()
	ON_COMMAND(ID_GO_TSS, OnGoTss)
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	//}}AFX_MSG_MAP
	ON_CBN_SELENDOK(AFX_IDW_TOOLBAR + 1, OnNewAddress)
	ON_COMMAND(IDOK, OnNewAddressEnter)
	ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, OnDropDown)
	ON_COMMAND(ID_FONT_DROPDOWN, DoNothing)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
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

	m_wndToolBar.SetButtons(NULL, 13);

	// set up each toolbar button
	m_wndToolBar.SetButtonInfo(0, ID_GO_BACK, TBSTYLE_BUTTON, 0);
	str.LoadString(IDS_BACK);
	m_wndToolBar.SetButtonText(0, str);

	m_wndToolBar.SetButtonInfo(1, ID_GO_FORWARD, TBSTYLE_BUTTON, 1);
	str.LoadString(IDS_FORWARD);
	m_wndToolBar.SetButtonText(1, str);

	m_wndToolBar.SetButtonInfo(2, ID_VIEW_STOP, TBSTYLE_BUTTON, 2);
	str.LoadString(IDS_STOP);
	m_wndToolBar.SetButtonText(2, str);

	m_wndToolBar.SetButtonInfo(3, ID_VIEW_REFRESH, TBSTYLE_BUTTON, 3);
	str.LoadString(IDS_REFRESH);
	m_wndToolBar.SetButtonText(3, str);

	m_wndToolBar.SetButtonInfo(4, ID_FILE_PRINT, TBSTYLE_BUTTON, 7);
	str.LoadString(IDS_PRINT);
	m_wndToolBar.SetButtonText(4, str);

	m_wndToolBar.SetButtonInfo(5, ID_FONT_DROPDOWN, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN, 8);
	str.LoadString(IDS_FONT);
	m_wndToolBar.SetButtonText(5, str);

	m_wndToolBar.SetButtonInfo(6, ID_EDIT_FIND, TBSTYLE_BUTTON, 9);
	m_wndToolBar.SetButtonText(6, "Find");

	m_wndToolBar.SetButtonInfo(7, 0, TBBS_SEPARATOR, 20);

	m_wndToolBar.SetButtonInfo(8, ID_GO_SEARCH_TEXBIB, TBSTYLE_BUTTON, 12);
	str.LoadString(IDS_SEARCH_TEXBIB);
	m_wndToolBar.SetButtonText(8, str);

	m_wndToolBar.SetButtonInfo(9, ID_GO_SEARCH_NWBB, TBSTYLE_BUTTON, 5);
	str.LoadString(IDS_SEARCH_NWBB);
	m_wndToolBar.SetButtonText(9, str);

	m_wndToolBar.SetButtonInfo(10, 0, TBBS_SEPARATOR, 20);

	m_wndToolBar.SetButtonInfo(11, ID_GO_TSS, TBSTYLE_BUTTON, 13);
	str.LoadString(IDS_GO_TSS);
	m_wndToolBar.SetButtonText(11, str);

	m_wndToolBar.SetButtonInfo(12, ID_APP_ABOUT, TBSTYLE_BUTTON, 10);
	m_wndToolBar.SetButtonText(12, "About");

	CRect rectToolBar;

	// set up toolbar button sizes
	m_wndToolBar.GetItemRect(0, &rectToolBar);
	m_wndToolBar.SetSizes(rectToolBar.Size(), CSize(30, 20));

	// create a combo box for the address bar
	if (!m_wndAddress.Create(CBS_DROPDOWN | WS_CHILD, CRect(0, 0, 200, 120), this, AFX_IDW_TOOLBAR + 1))
	{
		TRACE0("Failed to create combobox\n");
		return -1;      // fail to create
	}

	// create the animation control
	m_wndAnimate.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 10, 10), this, AFX_IDW_TOOLBAR + 2);
	m_wndAnimate.Open(IDR_MFCAVI);

	// add the toolbar, animation, and address bar to the rebar
	m_wndReBar.AddBar(&m_wndToolBar);
	m_wndReBar.AddBar(&m_wndAnimate, NULL, NULL, RBBS_FIXEDSIZE | RBBS_FIXEDBMP);
	str.LoadString(IDS_ADDRESS);
	m_wndReBar.AddBar(&m_wndAddress, str, NULL, RBBS_FIXEDBMP | RBBS_BREAK);

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

	CRect rectAddress;

	rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
	m_wndAddress.GetEditCtrl()->GetWindowRect(&rectAddress);
	rbbi.cyMinChild = rectAddress.Height() + 10;
	rbbi.cxIdeal = 200;
	m_wndReBar.GetReBarCtrl().SetBandInfo(2, &rbbi);

	//InitAddressBar();
	rbbi.wID = WID_ADDRESS;
	rbbi.fMask = RBBIM_ID;
	m_wndReBar.GetReBarCtrl().SetBandInfo(2, &rbbi);
	m_bShowAddress = TRUE;
	OnShowAddress();

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_FIXED);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
			sizeof(indicators) / sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	PostMessage(WM_COMMAND, ID_VIEW_STATUS_BAR);

	return 0;
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

	lpDispatch = ((CReselectVw*)GetActiveView())->GetHtmlDocument();
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

	IDispatch * pDocDisp = ((CReselectVw*)GetActiveView())->GetHtmlDocument();

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

void CMainFrame::SetAddress(LPCTSTR lpszUrl)
{
	// This is called when the browser has completely loaded the new location,
	// so make sure the text in the address bar is up to date and stop the
	// animation.
	m_wndAddress.SetWindowText(lpszUrl);
	if (!m_uTimer) StopAnimation();
}

void CMainFrame::StopAnimation()
{
	m_wndAnimate.Stop();
	m_wndAnimate.Seek(0);
}

void CMainFrame::StartAnimation(BOOL bTimer)
{
	// Start the animation.  This is called when the browser begins to
	// navigate to a new location
	if (bTimer) m_uTimer = SetTimer(0x128, 2000, NULL);
	m_wndAnimate.Play(0, -1, -1);
}

void CMainFrame::OnNewAddress()
{
	// gets called when an item in the Address combo box is selected
	// just navigate to the newly selected location.
	CString str;

	m_wndAddress.GetLBText(m_wndAddress.GetCurSel(), str);
	((CReselectVw *)GetActiveView())->Navigate2(str, 0, NULL);
}

void CMainFrame::OnNewAddressEnter()
{
	// gets called when an item is entered manually into the edit box portion
	// of the Address combo box.
	// navigate to the newly selected location and also add this address to the
	// list of addresses in the combo box.
	CString str;

	m_wndAddress.GetEditCtrl()->GetWindowText(str);
	((CReselectVw *)GetActiveView())->Navigate2(str, 0, NULL);

	COMBOBOXEXITEM item;

	item.mask = CBEIF_TEXT;
	item.iItem = -1;
	item.pszText = (LPTSTR)(LPCTSTR)str;
	m_wndAddress.InsertItem(&item);
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

void CMainFrame::OnShowAddress()
{
	int i = m_wndReBar.GetReBarCtrl().IDToIndex(WID_ADDRESS);

	m_bShowAddress = (m_bShowAddress == 0);
	m_wndReBar.GetReBarCtrl().ShowBand(i, m_bShowAddress);
}

void CMainFrame::OnUpdateShowAddress(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowAddress);
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

void CMainFrame::OnGoTss()
{
	theApp.ExecuteUrl(theApp.m_link);
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
