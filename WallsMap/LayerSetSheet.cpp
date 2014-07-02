// LayerSetPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "LayerSetSheet.h"
#include <direct.h>

const UINT WM_PROPVIEWDOC = ::RegisterWindowMessage("PROPVIEWDOC");
const UINT WM_RETFOCUS = ::RegisterWindowMessage("RETFOCUS");

BOOL hPropHook;
CLayerSetSheet *pLayerSheet;

// CLayerSetSheet

IMPLEMENT_DYNAMIC(CLayerSetSheet, CPropertySheet)

//int CLayerSetSheet::m_nSavedCX=0;
//int CLayerSetSheet::m_nSavedCY=0;
int CLayerSetSheet::m_nSavedX=0;
int CLayerSetSheet::m_nSavedY=0;
int CLayerSetSheet::m_nSavedCX=0;
int CLayerSetSheet::m_nSavedCY=0;

CLayerSetSheet::CLayerSetSheet(CWallsMapDoc *pDoc, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet("", pParentWnd, iSelectPage),m_pDoc(pDoc)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	//SetDocChanged(); //New Document
	pLayerSheet=this;
	hPropHook=FALSE;
	m_bNeedInit=TRUE;
	m_nDelaySide = 0;
	//m_bDraggingIcon=false;
	m_nMinCX=m_nMinCY=0;
	AddPage(&m_pageLayers);
	AddPage(&m_pageDetails);
}

CLayerSetSheet::~CLayerSetSheet()
{
	ASSERT(!hPropHook);
	pLayerSheet=NULL;
}

BEGIN_MESSAGE_MAP(CLayerSetSheet, CPropertySheet)
	ON_REGISTERED_MESSAGE(WM_PROPVIEWDOC,OnPropViewDoc)
	ON_MESSAGE (WM_RESIZEPAGE, OnResizePage)
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CLayerSetSheet::OnInitDialog()
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	SetPropTitle();

	CRect r;  GetWindowRect(&r);

	//For appearances sake --
	SetWindowPos(NULL,0,0,r.Width(),r.Height()+5,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

	/*
	if(m_wndSizeIcon.Create(this)) {
		CRect cltRect;
		GetClientRect(&cltRect);
		m_wndSizeIcon.SetPos(cltRect);
	}
	*/

	// Init m_nMinCX/Y
	m_nMinCX = r.Width()-250;
	m_nMinCY = r.Height();

	CLayerSetPage* pPage = (CLayerSetPage*)GetActivePage ();
	ASSERT (pPage);
	// store page size in m_PageRect
	pPage->GetWindowRect (&m_PageRect);
	ScreenToClient (&m_PageRect);
	GetClientRect(&m_rCrt);

	// After this point we allow resize code to kick in
	m_bNeedInit = FALSE;

	//Resize window to last saved position --
	m_nDelaySide=WMSZ_BOTTOMRIGHT;
	int cx=r.Width(),cy=r.Height();
	if(!m_nSavedCX) {
		m_nSavedX=r.left;
		m_nSavedY=r.top;
		m_nSavedCX=cx;
		m_nSavedCY=cy;
	}
	if(m_pDoc->LayersWidth()) {
		cx=m_pDoc->LayersWidth();
		cy=m_pDoc->LayersHeight();
	}
	//else cx+=40; //default width adjustment
	MoveWindow(m_nSavedX,m_nSavedY,cx,cy,0);

	hPropHook=TRUE;

	m_dropTarget.Register(this);

	return bResult;
}

BOOL CLayerSetSheet::Create(CWnd* pParentWnd,DWORD dwStyle/* = (DWORD)-1*/,DWORD dwExStyle/* = 0*/)
{
	ASSERT(dwStyle==-1);

	//Replace WS_MODALFRAME default --
	dwStyle = WS_THICKFRAME | DS_3DLOOK | WS_SYSMENU | DS_MODALFRAME |	DS_SETFONT | WS_POPUP | WS_VISIBLE | WS_CAPTION;

	return CPropertySheet::Create(pParentWnd,dwStyle,dwExStyle);
}


void CLayerSetSheet::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
   CPropertySheet::OnGetMinMaxInfo(lpMMI);
   lpMMI->ptMinTrackSize.x = m_nMinCX;
   lpMMI->ptMinTrackSize.y = m_nMinCY;
}

void CLayerSetSheet::OnSizing(UINT nSide, LPRECT lpRect)
{
	CPropertySheet::OnSizing(nSide, lpRect);
	m_nDelaySide = nSide;
}

void CLayerSetSheet::OnSize(UINT nType, int cx, int cy)
{
	// Handle WM_SIZE events by resizing the tab control and by 
	//    moving all the buttons on the property sheet.
   CPropertySheet::OnSize(nType, cx, cy);

   if (m_bNeedInit)
      return;

   CTabCtrl *pTab = GetTabControl();
   ASSERT(NULL != pTab && IsWindow(pTab->m_hWnd));
    
   int dx = cx - m_rCrt.Width();
   int dy = cy - m_rCrt.Height();
   GetClientRect(&m_rCrt);

   /*
   if((HWND)m_wndSizeIcon) {
	   m_wndSizeIcon.SetPos(m_rCrt);
   }
   */

   HDWP hDWP = ::BeginDeferWindowPos(5);

   CRect r1; 
   pTab->GetClientRect(&r1); 
   r1.right += dx; r1.bottom += dy;
   m_PageRect.right+=dx; m_PageRect.bottom+=dy;

   ::DeferWindowPos(hDWP, pTab->m_hWnd, NULL,
                    0, 0, r1.Width(), r1.Height(),
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);

/*
   // Move all buttons with the lower right sides
   for (CWnd *pChild = GetWindow(GW_CHILD);
        pChild != NULL;
        pChild = pChild->GetWindow(GW_HWNDNEXT))
   {
      if (pChild->SendMessage(WM_GETDLGCODE) & DLGC_BUTTON)
      {
		 //ASSERT(FALSE);
         pChild->GetWindowRect(&r1);
		 ScreenToClient(&r1); 
         r1.top += dy; r1.bottom += dy; r1.left+= dx; r1.right += dx;
         ::DeferWindowPos(hDWP, pChild->m_hWnd, NULL,
                          r1.left, r1.top, 0, 0,
                          SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
      }
      // Resize everything else...
      else
      {
         pChild->GetClientRect(&r1); 
		 r1.right += dx; r1.bottom += dy;
		 ::DeferWindowPos(hDWP, pChild->m_hWnd, NULL, 0, 0, r1.Width(), r1.Height(),SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
      }
   }
*/

   ::EndDeferWindowPos(hDWP);
   PostMessage (WM_RESIZEPAGE);
}


LONG CLayerSetSheet::OnResizePage(UINT, LONG)
{
    // resize the page using m_PageRect which was set in OnInitDialog()
    CLayerSetPage* pPage = (CLayerSetPage*)GetActivePage ();
    ASSERT (pPage);
    pPage->MoveWindow (&m_PageRect);
	m_nDelaySide = 0;

    return 0;
}

BOOL CLayerSetSheet::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    NMHDR* pnmh = (LPNMHDR) lParam;

    // the sheet resizes the page whenever it is activated
    // so we need to resize it to what we want
	if (TCN_SELCHANGE == pnmh->code) {
        // user-defined message needs to be posted because page must
        // be resized after TCN_SELCHANGE has been processed
		m_nDelaySide=WMSZ_BOTTOMRIGHT;
		PostMessage (WM_RESIZEPAGE);
	}

    return CPropertySheet::OnNotify(wParam, lParam, pResult);
}

// CLayerSetSheet message handlers

void CLayerSetSheet::PostNcDestroy() 
{
	if(pLayerSheet) {
		hPropHook=FALSE;
		delete(this);
	}
}

LRESULT CLayerSetSheet::OnPropViewDoc(WPARAM wParam,LPARAM typ)
{
	CWallsMapDoc *pDoc=(CWallsMapDoc *)wParam;

	switch(typ) {
		case LHINT_REFRESH:
			{
				pDoc->RefreshViews();
				break;
			}
		case LHINT_FITIMAGE:
			{
				//Zoom to layer
				POSITION pos = pDoc->GetFirstViewPosition();
				CWallsMapView *pView=(CWallsMapView *)pDoc->GetNextView(pos);
				if(pView)
					pView->FitImage(LHINT_FITIMAGE);
				break;
			}
		case LHINT_SYMBOLOGY:
			{
				ASSERT(pDoc==m_pDoc);
				((CPageLayers *)GetPage(0))->OnBnClickedSymbology();
				break;
			}
		case LHINT_NEWDOC:
			{
				//New document --
				ASSERT(m_pDoc);
				SaveWindowSize();
				m_pDoc=pDoc;
				RestoreWindowSize();
				SetPropTitle();
				int page=m_pDoc->GetPropStatus().nTab;
				SetActivePage(page);
				if(!page)
					((CLayerSetPage *)GetPage(0))->UpdateLayerData(pDoc);
				break;
			}
		case LHINT_NEWLAYER:
			{
				ASSERT(pDoc==m_pDoc);
				if(GetActivePage()) SetActivePage(0);
				((CLayerSetPage *)GetPage(0))->UpdateLayerData(pDoc);
				break;
			}
		default: ASSERT(0);
	}

	return TRUE;
}

void CLayerSetSheet::RestoreWindowSize()
{
	//Resize window to last saved position --
	m_nDelaySide=WMSZ_BOTTOMRIGHT;
	int cx=m_nSavedCX,cy=m_nSavedCY;
	if(m_pDoc->LayersWidth()) {
		cx=m_pDoc->LayersWidth();
		cy=m_pDoc->LayersHeight();
	}
	CRect r;  GetWindowRect(&r);
	MoveWindow(r.left,r.top,cx,cy,1);
}

void CLayerSetSheet::SaveWindowSize()
{
	CRect r;  GetWindowRect(&r);
	m_nSavedX = r.left;
	m_nSavedY = r.top;
	CPageLayers *p=(CPageLayers *)GetPage(0);
	if(p->m_pDoc==m_pDoc) {
		bool bChg=p->SaveColumnWidths();
		CPropStatus &prop=m_pDoc->GetPropStatus();
		int cx=r.Width(),cy=r.Height();
		if(cx!=prop.nWinWidth || cy!=prop.nWinHeight) {
			prop.nWinWidth=cx; prop.nWinHeight=cy;
			bChg=true;
		}
		if(bChg) {
			//m_pDoc->m_bLayersUpdated=true;
			if(m_pDoc->IsSavingLayers()) {
				m_pDoc->m_bLayersUpdated=true;
				m_pDoc->SetChanged(1); //no prompt to change just for this
			}
		}
	}
}

void CLayerSetSheet::OnDestroy()
{
	SaveWindowSize();
	CPropertySheet::OnDestroy();
}
