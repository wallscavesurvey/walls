// A flicker-free resizing dialog class
// Copyright (c) 1999 Andy Brown <andy@mirage.dabsol.co.uk>
// You may do whatever you like with this file, I just don't care.

/* ResizeDlg.cpp
 *
 * Derived from 1999 Andy Brown <andy@mirage.dabsol.co.uk> by Robert Python Apr 2004 BJ
 * goto http://www.codeproject.com/dialog/resizabledialog.asp for Andy Brown's
 * article "Flicker-Free Resizing Dialog".
 *
 * 1, Enhanced by Robert Python (RobertPython@163.com, support@panapax.com, www.panapax.com) Apr 2004.
 *    In this class, add the following features:
 *    (1) controls reposition and/or resize direction(left-right or top-bottom) sensitive;
 *    (2) add two extra reposition/resize type: ZOOM and DELTA_ZOOM, which is useful in there
 *        are more than one controls need reposition/resize at the same direction.
 *
**/ 
#include "stdafx.h"
#include "ResizeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CItemCtrl::CItemCtrl()
{
	m_nID = 0;
	m_stxLeft = CST_NONE;
	m_stxRight = CST_NONE;
	m_styTop = CST_NONE;
	m_styBottom = CST_NONE;
	m_bFlickerFree = 1;
	m_xRatio = m_cxRatio = 1.0;
	m_yRatio = m_cyRatio = 1.0;
}

CItemCtrl::CItemCtrl(const CItemCtrl& src)
{
	Assign(src);
}

CItemCtrl& CItemCtrl::operator=(const CItemCtrl& src)
{
	Assign(src);
	return *this;
}

void CItemCtrl::Assign(const CItemCtrl& src)
{
	m_nID = src.m_nID;
	m_stxLeft = src.m_stxLeft;
	m_stxRight = src.m_stxRight;
	m_styTop = src.m_styTop;
	m_styBottom = src.m_styBottom;
	m_bFlickerFree = src.m_bFlickerFree;
	m_bInvalidate = src.m_bInvalidate;
	m_wRect = src.m_wRect;
	m_xRatio = src.m_xRatio;
	m_cxRatio = src.m_cxRatio;
	m_yRatio = src.m_yRatio;
	m_cyRatio = src.m_cyRatio;
}

HDWP CItemCtrl::OnSize(HDWP hDWP,int sizeType, CRect *pnRect, CRect *poRect, CRect *pR0, CWnd *pDlg)
{
	CRect ctrlRect = m_wRect, curRect;
	CWnd *pWnd = pDlg->GetDlgItem(m_nID);
	int deltaX = pnRect->Width() - poRect->Width();
	int deltaY = pnRect->Height()- poRect->Height();
	int deltaX0 = pnRect->Width() - pR0->Width();
	int deltaY0 = pnRect->Height()- pR0->Height();
	int newCx, newCy;
	int st, bxUpdate = 1, byUpdate = 1;

	// anyway, get the ctrl's real pos & size
	pWnd->GetWindowRect(&curRect);
	pDlg->ScreenToClient(&curRect);

	// process left-horizontal option
	st = CST_NONE;
	if ((sizeType & WST_LEFT) && m_stxLeft != CST_NONE) {
		ASSERT((sizeType & WST_RIGHT) == 0);

		st = m_stxLeft;
	}
	else if ((sizeType & WST_RIGHT) && m_stxRight != CST_NONE) {
		ASSERT((sizeType & WST_LEFT) == 0);

		st = m_stxRight;
	}

	switch(st) {
		case CST_NONE:
			if (m_stxLeft == CST_ZOOM || m_stxRight == CST_ZOOM ||
				m_stxLeft == CST_DELTA_ZOOM || m_stxRight == CST_DELTA_ZOOM)
			{
				ctrlRect.left = curRect.left;
				ctrlRect.right = curRect.right;
				bxUpdate = 0;
			}
			break;

		case CST_RESIZE:
			ctrlRect.right += deltaX;
			break;

		case CST_REPOS:
			ctrlRect.left += deltaX;
			ctrlRect.right += deltaX;
			break;

		case CST_RELATIVE:
			newCx = ctrlRect.Width();
			ctrlRect.left = (int)((double)m_xRatio * 1.0 * pnRect->Width() - newCx / 2.0);
			ctrlRect.right = ctrlRect.left + newCx; /* (int)((double)m_xRatio * 1.0 * pnRect->Width() + newCx / 2.0); */
			break;

		case CST_ZOOM:
			ctrlRect.left = (int)(1.0 * ctrlRect.left * (double)pnRect->Width() / pR0->Width());
			ctrlRect.right = (int)(1.0 * ctrlRect.right * (double)pnRect->Width() / pR0->Width());
			bxUpdate = 0;
			break;

		case CST_DELTA_ZOOM:
			newCx = ctrlRect.Width();
			ctrlRect.right = (int)(ctrlRect.left * 1.0 + deltaX0 * 1.0 * m_xRatio + newCx * 1.0 + deltaX0 * m_cxRatio);
			ctrlRect.left += (int)(deltaX0 * 1.0 * m_xRatio);
			bxUpdate = 0;
			break;

		default:
			break;
	}

	// process vertical option
	st = CST_NONE;
	if ((sizeType & WST_TOP) && (m_styTop != CST_NONE)) {
		ASSERT((sizeType & WST_BOTTOM) == 0);
		st = m_styTop;
	}
	else if ((sizeType & WST_BOTTOM) && (m_styBottom != CST_NONE)) {
		ASSERT((sizeType & WST_TOP) == 0);
		st = m_styBottom;
	}

	switch (st) {
		case CST_NONE:
			if (m_styTop == CST_ZOOM || m_styTop == CST_ZOOM ||
			m_styBottom == CST_DELTA_ZOOM || m_styBottom == CST_DELTA_ZOOM)
			{
				ctrlRect.top = curRect.top;
				ctrlRect.bottom = curRect.bottom;
				byUpdate = 0;
			}
			break;

		case CST_RESIZE:
			ctrlRect.bottom += deltaY;
			break;

		case CST_REPOS:
			ctrlRect.top += deltaY;
			ctrlRect.bottom += deltaY;
			break;

		case CST_RELATIVE:
			newCy = ctrlRect.Width();
			ctrlRect.top = (int)((double)m_yRatio * 1.0 * pnRect->Width() - newCy / 2.0);
			ctrlRect.bottom = ctrlRect.top + newCy; /* (int)((double)m_yRatio * 1.0 * pnRect->Width() + newCy / 2.0); */

		case CST_ZOOM:
			ctrlRect.top = (int)(1.0 * ctrlRect.top * (double)pnRect->Height() / pR0->Height());
			ctrlRect.bottom = (int)(1.0 * ctrlRect.bottom * (double)pnRect->Height() / pR0->Height());
			byUpdate = 0;
			break;

		case CST_DELTA_ZOOM:
			newCy = ctrlRect.Height();
			ctrlRect.bottom = (int)(ctrlRect.top * 1.0 + deltaY0 * 1.0 * m_yRatio + newCy * 1.0 + deltaY0 * m_cyRatio);
			ctrlRect.top += (int)(deltaY0 * 1.0 * m_yRatio);
			byUpdate = 0;
			break;

		default:
			break;
	}

	if (bxUpdate) {
		curRect.left = m_wRect.left;
		curRect.right = m_wRect.right;
	}
	if (byUpdate) {
		curRect.top = m_wRect.top;
		curRect.bottom = m_wRect.bottom;
	}

	if (ctrlRect != curRect) {
		if (m_bInvalidate) {
			pWnd->Invalidate(0);
		}

		hDWP = ::DeferWindowPos(hDWP, (HWND)*pWnd, NULL, ctrlRect.left, ctrlRect.top, ctrlRect.Width(), ctrlRect.Height(), SWP_NOZORDER);

		#if 1 /* why No effect!!!! */
		if (m_bInvalidate) {
			pDlg->InvalidateRect(&curRect);
			pDlg->UpdateWindow();
		}
		#endif /* No effect???? */

		if (bxUpdate) {
			m_wRect.left = ctrlRect.left;
			m_wRect.right = ctrlRect.right;
		}
		if (byUpdate) {
			m_wRect.top = ctrlRect.top;
			m_wRect.bottom = ctrlRect.bottom;
		}
	}

	return hDWP;
}

IMPLEMENT_DYNAMIC(CResizeDlg, CDialog)
BEGIN_MESSAGE_MAP(CResizeDlg, CDialog)
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

CResizeDlg::CResizeDlg(const UINT resID,CWnd *pParent)
	  : CDialog(resID,pParent)
{
	m_xSt = m_ySt = CST_RESIZE;
	m_xMin = m_yMin = 32;
	m_xMax = m_yMax=0;
	m_nDelaySide = 0;
	m_bDraggingIcon=false;
}

CResizeDlg::~CResizeDlg()
{
	// TODO:
}

BOOL CResizeDlg::OnInitDialog() 
{
	BOOL bret = CDialog::OnInitDialog();

	GetClientRect(&m_cltR0);
	ClientToScreen(&m_cltR0);
	m_cltRect = m_cltR0;
	CRect wRect;
	GetWindowRect(&wRect);
	m_xMin = wRect.Width();		// default x limit
	m_yMin = wRect.Height();	// default y limit

	return bret;
}

void CResizeDlg::OnSizing(UINT nSide, LPRECT lpRect)
{
	/*  wParam for WM_SIZING message  */
	// WMSZ_LEFT           1
	// WMSZ_RIGHT          2
	// WMSZ_TOP            3
	// WMSZ_TOPLEFT        4
	// WMSZ_TOPRIGHT       5
	// WMSZ_BOTTOM         6
	// WMSZ_BOTTOMLEFT     7
	// WMSZ_BOTTOMRIGHT    8

	CDialog::OnSizing(nSide, lpRect);

	m_nDelaySide = nSide;
}

void CResizeDlg::OnSize(UINT nType,int cx,int cy)
{

	//SIZE_RESTORED       0
	//SIZE_MINIMIZED      1
	//SIZE_MAXIMIZED      2
	//SIZE_MAXSHOW        3
	//SIZE_MAXHIDE        4

	ASSERT(nType==SIZE_RESTORED);

	CDialog::OnSize(nType,cx,cy);

	std::vector<CItemCtrl>::iterator it;
	int	 nCount=m_Items.size();

	if(nCount > 0) {
		CRect  cltRect;
		GetClientRect(&cltRect);
		ClientToScreen(cltRect);

		HDWP   hDWP;
		int    sizeType = WST_NONE;		

		switch (m_nDelaySide) {
		case WMSZ_BOTTOM:
			sizeType = WST_BOTTOM;
			break;
		case WMSZ_BOTTOMLEFT:
			sizeType = WST_BOTTOM|WST_LEFT;
			break;
		case WMSZ_BOTTOMRIGHT:
			sizeType = WST_BOTTOM|WST_RIGHT;
			break;
		case WMSZ_LEFT:
			sizeType = WST_LEFT;
			break;
		case WMSZ_RIGHT:
			sizeType = WST_RIGHT;
			break;
		case WMSZ_TOP:
			sizeType = WST_TOP;
			break;
		case WMSZ_TOPLEFT:
			sizeType = WST_TOP|WST_LEFT;
			break;
		case WMSZ_TOPRIGHT:
			sizeType = WST_TOP|WST_RIGHT;
			break;
		default:
			break;
		}

		if (sizeType != WST_NONE) {
			hDWP = ::BeginDeferWindowPos(nCount);

			for (it = m_Items.begin(); it != m_Items.end(); it++)
				hDWP = it->OnSize(hDWP, sizeType, &cltRect, &m_cltRect, &m_cltR0, this);

			::EndDeferWindowPos(hDWP);
		}

		m_cltRect = cltRect;
	}

	m_nDelaySide = 0;
}

void CResizeDlg::OnGetMinMaxInfo(MINMAXINFO *pmmi)
{
	pmmi->ptMinTrackSize.x = m_xMin;
	pmmi->ptMinTrackSize.y = m_yMin;

	if (m_xMax || m_xSt == CST_NONE)
		pmmi->ptMaxTrackSize.x = pmmi->ptMaxSize.x = (m_xMax?m_xMax:m_xMin);

	if (m_yMax || m_ySt == CST_NONE)
		pmmi->ptMaxTrackSize.y = pmmi->ptMaxSize.y = (m_yMax?m_yMax:m_yMin);
}

BOOL CResizeDlg::OnEraseBkgnd(CDC *pDC)
{
	if (!(GetStyle() & WS_CLIPCHILDREN)) {
		std::vector<CItemCtrl>::const_iterator it;

		for(it = m_Items.begin();it != m_Items.end(); it++) {

			if(it->m_bFlickerFree) {
				if(::IsWindowVisible(GetDlgItem(it->m_nID)->GetSafeHwnd()))
					pDC->ExcludeClipRect(&it->m_wRect);
			}
		}
	}

	CDialog::OnEraseBkgnd(pDC);
	return FALSE;
}

void CResizeDlg::AddControl( UINT nID,int xl, int xr, int yt, int yb, int bFlickerFree,
								double xRatio, double cxRatio, double yRatio, double cyRatio )
{
	CItemCtrl	item;
	CRect		cltRect;

	GetDlgItem(nID)->GetWindowRect(&item.m_wRect);
	ScreenToClient(&item.m_wRect);

	item.m_nID = nID;
	item.m_stxLeft = xl;
	item.m_stxRight = xr;
	item.m_styTop = yt;
	item.m_styBottom = yb;
	item.m_bFlickerFree = !!(bFlickerFree & 0x01);
	item.m_bInvalidate = !!(bFlickerFree & 0x02);
	item.m_xRatio = xRatio;
	item.m_cxRatio = cxRatio;
	item.m_yRatio = yRatio;
	item.m_cyRatio = cyRatio;

	GetClientRect(&cltRect);
	if (xl == CST_RELATIVE || xl == CST_ZOOM || xr == CST_RELATIVE || xr == CST_ZOOM)
		item.m_xRatio = (item.m_wRect.left + item.m_wRect.right) / 2.0 / cltRect.Width();

	if (yt == CST_RELATIVE || yt == CST_ZOOM || yb == CST_RELATIVE || yb == CST_ZOOM)
		item.m_yRatio = (item.m_wRect.bottom + item.m_wRect.top ) / 2.0 / cltRect.Height();

	std::vector<CItemCtrl>::iterator it;
	int  nCount;

	if((nCount = m_Items.size()) > 0) {
		for (it = m_Items.begin(); it != m_Items.end(); it++) {
			if (it->m_nID == item.m_nID) {
				*it = item;
				return;
			}
		}
	}

	m_Items.push_back(item);

}

void CResizeDlg::AllowSizing(int xst, int yst)
{
	m_xSt = xst;
	m_ySt = yst;
}

int CResizeDlg::UpdateControlRect(UINT nID, CRect *pnr)
{
	std::vector<CItemCtrl>::iterator it;
	int  nCount;

	if ((nCount = m_Items.size()) > 0) {
		for (it = m_Items.begin(); it != m_Items.end(); it++) {
			if (it->m_nID == nID) {
				if (pnr != NULL) {
					it->m_wRect = *pnr;
				}
				else {
					GetDlgItem(nID)->GetWindowRect(&it->m_wRect);
					ScreenToClient(&it->m_wRect);
				}

				return 0;
			}
		}
	}

	return -1;
}

void CResizeDlg::Reposition(const CRect &rect,UINT uFlags /*=0*/)
{
	m_nDelaySide=(uFlags&SWP_NOSIZE)?0:WMSZ_BOTTOMRIGHT;
	VERIFY(SetWindowPos(0,rect.left,rect.top,rect.Width(),rect.Height(),
		uFlags|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW));
}
