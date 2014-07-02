/*****************************************************************************
* CColumnTreeCtrl
* Version: 1.1 
* Date: February 18, 2008
* Author: Oleg A. Krivtsov
* E-mail: olegkrivtsov@mail.ru
* Based on ideas implemented in Michal Mecinski's CColumnTreeCtrl class 
* (see copyright note below).
*
*****************************************************************************/

/*********************************************************
* Multi-Column Tree View
* Version: 1.1
* Date: October 22, 2003
* Author: Michal Mecinski
* E-mail: mimec@mimec.w.pl
* WWW: http://www.mimec.w.pl
*
* You may freely use and modify this code, but don't remove
* this copyright note.
*
* There is no warranty of any kind, express or implied, for this class.
* The author does not take the responsibility for any damage
* resulting from the use of it.
*
* Let me know if you find this code useful, and
* send me any modifications and bug reports.
*
* Copyright (C) 2003 by Michal Mecinski
*********************************************************/

#include "stdafx.h"
#include "ColumnTreeCtrl.h"

#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// IE 5.0 or higher required
#ifndef TVS_NOHSCROLL
	#error CColumnTreeCtrl requires IE 5.0 or higher; _WIN32_IE should be greater than 0x500.
#endif

#define COLUMN_MARGIN		1		// 1 pixel between coumn edge and text bounding rectangle

#define DEFMINFIRSTCOLWIDTH 100	// here we need to avoid zero-width first column	

IMPLEMENT_DYNCREATE(CColumnTreeCtrl, CStatic)

BEGIN_MESSAGE_MAP(CColumnTreeCtrl, CStatic)
	ON_WM_SETFOCUS()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_ENABLE()
	ON_NOTIFY_REFLECT_EX(NM_RCLICK, OnRClick)
	//ON_NOTIFY_REFLECT_EX(NM_DBLCLK, OnDblclk)
	ON_NOTIFY_REFLECT_EX(TVN_SELCHANGED, OnSelchanged)
	ON_NOTIFY_REFLECT_EX(TVN_SELCHANGING, OnSelchanging)
END_MESSAGE_MAP()

CColumnTreeCtrl::CColumnTreeCtrl()
{
	// initialize members
	m_uMinFirstColWidth = DEFMINFIRSTCOLWIDTH;
	m_bHeaderChangesBlocked = FALSE;
	m_xOffset = 0;
	m_pHdrFont=NULL;
}

CColumnTreeCtrl::~CColumnTreeCtrl()
{
	delete m_pHdrFont;
}

void CColumnTreeCtrl::PreSubclassWindow()
{
	Initialize();
}

void CColumnTreeCtrl::AssertValid( ) const
{
	// validate control state
	ASSERT( m_Tree.m_hWnd ); 
	ASSERT( m_Header.m_hWnd );
	ASSERT( m_Header2.m_hWnd );
}

void CColumnTreeCtrl::Initialize()
{
	if (m_Tree.m_hWnd) 
		return; // do not initialize twice

	CRect rcClient;
	GetClientRect(&rcClient);
	
	// create tree and header controls as children
	m_Tree.Create(WS_CHILD | WS_VISIBLE  | TVS_NOHSCROLL | TVS_NOTOOLTIPS, CRect(), this, TreeID);
	m_Tree.ModifyStyle(TVS_DISABLEDRAGDROP, 0); //why?
	//m_Tree.ModifyStyle(0, WS_EX_CLIENTEDGE | WS_EX_NOPARENTNOTIFY | TVS_EX_DOUBLEBUFFER);
	//m_Header.Create(WS_CHILD | HDS_BUTTONS | WS_VISIBLE | HDS_FULLDRAG  , rcClient, this, HeaderID);
	m_Header.Create(WS_CHILD | WS_VISIBLE | HDS_FULLDRAG  , rcClient, this, HeaderID);
	m_Header2.Create(WS_CHILD , rcClient, this, Header2ID);
	m_Tree.m_pHeader=&m_Header; //allow m_Tree access to header
	// create horisontal scroll bar
	m_horScroll.Create(SBS_HORZ|WS_CHILD|SBS_BOTTOMALIGN,rcClient,this,HScrollID);

    // set correct font for the header
	m_Header.SetFont(m_Tree.GetFont());
	m_Header2.SetFont(m_Tree.GetFont());

	// get header layout
	WINDOWPOS wp;
	HDLAYOUT hdlayout;
	hdlayout.prc = &rcClient;
	hdlayout.pwpos = &wp;
	m_Header.Layout(&hdlayout);
	m_cyHeader = hdlayout.pwpos->cy;
	m_xOffset = 4;
	m_xPos = 0;

	UpdateColumns();
}

/*
void CColumnTreeCtrl::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	m_Tree.SendMessage(WM_SETTINGCHANGE);
	m_horScroll.SendMessage(WM_SETTINGCHANGE);

	// set correct font for the header
	CRect rcClient;
	GetClientRect(&rcClient);
	
	//CFont* pFont = m_Tree.GetFont();
	//m_Header.SetFont(pFont);
	//m_Header2.SetFont(pFont);
	
	m_Header.SendMessage(WM_SETTINGCHANGE);
	m_Header2.SendMessage(WM_SETTINGCHANGE);
	
	m_Header.SetFont(CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT)));
	m_Header2.SetFont(CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT)));

	// get header layout
	WINDOWPOS wp;
	HDLAYOUT hdlayout;
	hdlayout.prc = &rcClient;
	hdlayout.pwpos = &wp;
	m_Header.Layout(&hdlayout);
	m_cyHeader = hdlayout.pwpos->cy;

	RepositionControls();
}
*/

void CColumnTreeCtrl::OnPaint()
{
	// do not draw entire background to avoid flickering
	// just fill right-bottom rectangle when it is visible
	
	CPaintDC dc(this);
	
	CRect rcClient;
	GetClientRect(&rcClient);
	
	int cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
	int cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
	CBrush brush;
	brush.CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
	
	CRect rc;

	// determine if vertical scroll bar is visible
	SCROLLINFO scrinfo;
	scrinfo.cbSize = sizeof(scrinfo);
	m_Tree.GetScrollInfo(SB_VERT,&scrinfo,SIF_ALL);
	BOOL bVScrollVisible = scrinfo.nMin!=scrinfo.nMax?TRUE:FALSE;
	
	if(bVScrollVisible)
	{
		// fill the right-bottom rectangle
		rc.SetRect(rcClient.right-cxVScroll, rcClient.bottom-cyHScroll,
				rcClient.right, rcClient.bottom);
		dc.FillRect(rc,&brush);
	}

	
}

BOOL CColumnTreeCtrl::OnEraseBkgnd(CDC* pDC)
{
	// do nothing, all work is done in OnPaint()
	return TRUE;
	
}

void CColumnTreeCtrl::OnSize(UINT nType, int cx, int cy)
{
	RepositionControls();
}


void CColumnTreeCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO scrinfo;
	scrinfo.cbSize = sizeof(scrinfo);
	
	m_Tree.GetScrollInfo(SB_VERT,&scrinfo,SIF_ALL);
	
	BOOL bVScrollVisible = scrinfo.nMin!=scrinfo.nMax?TRUE:FALSE;
	
	// determine full header width
	int cxTotal = m_cxTotal+(bVScrollVisible?GetSystemMetrics(SM_CXVSCROLL):0);
	
	CRect rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.Width();

	int xLast = m_xPos;

	switch (nSBCode)
	{
	case SB_LINELEFT:
		m_xPos -= 15;
		break;
	case SB_LINERIGHT:
		m_xPos += 15;
		break;
	case SB_PAGELEFT:
		m_xPos -= cx;
		break;
	case SB_PAGERIGHT:
		m_xPos += cx;
		break;
	case SB_LEFT:
		m_xPos = 0;
		break;
	case SB_RIGHT:
		m_xPos = cxTotal - cx;
		break;
	case SB_THUMBTRACK:
		m_xPos = nPos;
		break;
	}

	if (m_xPos < 0)
		m_xPos = 0;
	else if (m_xPos > cxTotal - cx)
		m_xPos = cxTotal - cx;

	if (xLast == m_xPos)
		return;

	m_Tree.m_nOffsetX = m_xPos;

	SetScrollPos(SB_HORZ, m_xPos);
	CWnd::OnHScroll(nSBCode,nPos,pScrollBar);
	RepositionControls();
	
}

void CColumnTreeCtrl::OnHeaderItemChanging(NMHDR* pNMHDR, LRESULT* pResult)
{
	// do not allow user to set zero width to the first column.
	// the minimum width is defined by m_uMinFirstColWidth;

	if(m_bHeaderChangesBlocked)
	{
		// do not allow change header size when moving it
		// but do not prevent changes the next time the header will be changed
		m_bHeaderChangesBlocked = FALSE;
		*pResult = TRUE; // prevent changes
		return;
	}

	*pResult = FALSE;

	LPNMHEADER pnm = (LPNMHEADER)pNMHDR;
	if(pnm->iItem==0)
	{
		CRect rc;
		m_Header.GetItemRect(0,&rc);
		if(pnm->pitem->cxy<m_uMinFirstColWidth)
		{
			// do not allow sizing of the first column 
			pnm->pitem->cxy=m_uMinFirstColWidth+1;
			*pResult = TRUE; // prevent changes
		}
		return;
	}
	
}

void CColumnTreeCtrl::OnHeaderItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	UpdateColumns();

	m_Tree.Invalidate();
}


void CColumnTreeCtrl::OnTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	// We use custom drawing to correctly display subitems.
	// Standard drawing code is used only for displaying icons and dotted lines
	// The rest of work is done here.

	NMCUSTOMDRAW* pNMCustomDraw = (NMCUSTOMDRAW*)pNMHDR;
	NMTVCUSTOMDRAW* pNMTVCustomDraw = (NMTVCUSTOMDRAW*)pNMHDR;

	switch (pNMCustomDraw->dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT:
		*pResult = CDRF_DODEFAULT | CDRF_NOTIFYPOSTPAINT;
		break;

	case CDDS_ITEMPOSTPAINT:
		{
			HTREEITEM hItem = (HTREEITEM)pNMCustomDraw->dwItemSpec;
			UINT state=pNMCustomDraw->uItemState;
			CRect rcItem = pNMCustomDraw->rc;

			if (rcItem.IsRectEmpty())
			{
				// nothing to paint
				*pResult = CDRF_DODEFAULT;
				break;
			}

			CDC dc;
			dc.Attach(pNMCustomDraw->hdc);

			CRect rcLabel;
			m_Tree.GetItemRect(hItem, &rcLabel, TRUE); //text only

			COLORREF crTextBk = pNMTVCustomDraw->clrTextBk;
			if(!state && hItem==m_Tree.GetSelectedItem()) 
				crTextBk=GetSysColor(COLOR_BTNFACE);
			COLORREF clrText = pNMTVCustomDraw->clrText;
			COLORREF crWnd = GetSysColor((IsWindowEnabled()?COLOR_WINDOW:COLOR_BTNFACE));
			
			int nColsCnt = m_Header.GetItemCount();

			// clear the original label rectangle
			dc.FillSolidRect(&rcLabel, crWnd);

			CString strText = m_Tree.GetItemText(hItem);
			CString strSub;
			AfxExtractSubString(strSub, strText, 0, '\t');

			// calculate main label's size
			CRect rcText(0,0,0,0);
			dc.DrawText(strSub, &rcText, DT_NOPREFIX | DT_CALCRECT);
			rcLabel.right = min(rcLabel.left + rcText.right + 4, m_arrColWidths[0] - 4);

			ASSERT(!(m_Tree.GetStyle()&TVS_FULLROWSELECT));

			if (rcLabel.Width() < 0)
				crTextBk = crWnd;
			if (crTextBk != crWnd)	// draw label's background
			{
				ASSERT(!state || (state&(CDIS_SELECTED|CDIS_FOCUS)));
				//must be a selected row --
				CRect rcSelect =  rcLabel;
				rcSelect.right = rcItem.right;

				dc.FillSolidRect(&rcSelect, crTextBk);

				// draw focus rectangle if necessary
				//if (state & CDIS_FOCUS)	dc.DrawFocusRect(&rcSelect);

			}
			
			// draw main label

			CFont* pOldFont = NULL;
			if(m_Tree.GetStyle()&TVS_TRACKSELECT && pNMCustomDraw->uItemState && CDIS_HOT)
			{
				LOGFONT lf;
				pOldFont = m_Tree.GetFont();
				pOldFont->GetLogFont(&lf);
				lf.lfUnderline = 1;
				CFont newFont;
				newFont.CreateFontIndirect(&lf);
				dc.SelectObject(newFont);
			}

			rcText = rcLabel;
			rcText.DeflateRect(2, 1);
			dc.SetTextColor(clrText);
			dc.DrawText(strSub, &rcText, DT_VCENTER | DT_SINGLELINE | 
				DT_NOPREFIX | DT_END_ELLIPSIS);

			int xOffset = m_arrColWidths[0];
			dc.SetBkMode(TRANSPARENT);

			//if(IsWindowEnabled())	
				//dc.SetTextColor(::GetSysColor(COLOR_MENUTEXT));
			
			// set not underlined text for subitems
			if(pOldFont) 
				dc.SelectObject(pOldFont);

			// draw other columns text
			for (int i=1; i<nColsCnt; i++)
			{
				if (AfxExtractSubString(strSub, strText, i, '\t'))
				{
					rcText = rcLabel;
					rcText.left = xOffset+ COLUMN_MARGIN;
					rcText.right = xOffset + m_arrColWidths[i]-COLUMN_MARGIN;
					rcText.DeflateRect(m_xOffset, 1, 2, 1);
					if(rcText.left<0 || rcText.right<0 || rcText.left>=rcText.right)
					{
						xOffset += m_arrColWidths[i];
						continue;
					}
					DWORD dwFormat = m_arrColFormats[i]&HDF_RIGHT?
						DT_RIGHT:(m_arrColFormats[i]&HDF_CENTER?DT_CENTER:DT_LEFT);
					

					dc.DrawText(strSub, &rcText, DT_SINGLELINE |DT_VCENTER 
						| DT_NOPREFIX | DT_END_ELLIPSIS | dwFormat);
				}
				xOffset += m_arrColWidths[i];
			}

			// draw vertical lines...
			xOffset = 0;
			for (int i=0; i<nColsCnt; i++)
			{
				xOffset += m_arrColWidths[i];
				rcItem.right = xOffset-1;
				dc.DrawEdge(&rcItem, BDR_SUNKENINNER, BF_RIGHT);
			}
			// ...and the horizontal ones
			dc.DrawEdge(&rcItem, BDR_SUNKENINNER, BF_BOTTOM);
			if(pOldFont) dc.SelectObject(pOldFont);
			dc.Detach();
		}
		*pResult = CDRF_DODEFAULT;
		break;

	default:
		*pResult = CDRF_DODEFAULT;
	}
}

void CColumnTreeCtrl::ClearTree()
{
	m_Tree.DeleteAllItems();
	int nCount = m_Header.GetItemCount();
	ASSERT(nCount==m_Header2.GetItemCount());
	for (int i=0;i < nCount;i++) {
		VERIFY(m_Header.DeleteItem(0));
		VERIFY(m_Header2.DeleteItem(0));
	}
}

void CColumnTreeCtrl::UpdateColumns()
{
	m_cxTotal = 0;

	HDITEM hditem;
	hditem.mask = HDI_WIDTH;
	int nCnt = m_Header.GetItemCount();
	
	ASSERT(nCnt<=MAX_COLUMN_COUNT);
	
	// get column widths from the header control
	for (int i=0; i<nCnt; i++)
	{
		if (m_Header.GetItem(i, &hditem))
		{
			m_cxTotal += m_arrColWidths[i] = hditem.cxy;
			if (i==0)
				m_Tree.m_nFirstColumnWidth = hditem.cxy;
		}
	}
	m_bHeaderChangesBlocked = TRUE;
	RepositionControls();
}


void CColumnTreeCtrl::RepositionControls()
{
	// reposition child controls
	if (m_Tree.m_hWnd)
	{
	
		CRect rcClient;
		GetClientRect(&rcClient);
		int cx = rcClient.Width();
		int cy = rcClient.Height();

		// get dimensions of scroll bars
		int cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
		int cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
	
		// determine if vertical scroll bar is visible
		SCROLLINFO scrinfo;
		scrinfo.cbSize = sizeof(scrinfo);
		m_Tree.GetScrollInfo(SB_VERT,&scrinfo,SIF_ALL);
		BOOL bVScrollVisible = scrinfo.nMin!=scrinfo.nMax?TRUE:FALSE;
	
		// determine full header width
		int cxTotal = m_cxTotal+(bVScrollVisible?cxVScroll:0);
	
		if (m_xPos > cxTotal - cx) m_xPos = cxTotal - cx;
		if (m_xPos < 0)	m_xPos = 0;
	
		scrinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		scrinfo.nPage = cx;
		scrinfo.nMin = 0;
		scrinfo.nMax = cxTotal;
		scrinfo.nPos = m_xPos;
		m_horScroll.SetScrollInfo(&scrinfo);

		CRect rcTree;
		m_Tree.GetClientRect(&rcTree);
		
		// move to a negative offset if scrolled horizontally
		int x = 0;
		if (cx < cxTotal)
		{
			x = m_horScroll.GetScrollPos();
			cx += x;
		}
	
		// show horisontal scroll only if total header width is greater
		// than the client rect width and cleint rect height is big enough
		BOOL bHScrollVisible = rcClient.Width()<cxTotal 
			&& rcClient.Height()>=cyHScroll+m_cyHeader;
			
		m_horScroll.ShowWindow(bHScrollVisible?SW_SHOW:SW_HIDE);
	
		m_Header.MoveWindow(-x, 0, cx  - (bVScrollVisible?cxVScroll:0), m_cyHeader);
		
		m_Header2.MoveWindow(rcClient.Width()-cxVScroll, 0, cxVScroll, m_cyHeader);

		m_Tree.MoveWindow(-x, m_cyHeader, cx, cy-m_cyHeader-(bHScrollVisible?cyHScroll:0));
		
		m_horScroll.MoveWindow(0, rcClient.Height()-cyHScroll,
			rcClient.Width() - (bVScrollVisible?cxVScroll:0), cyHScroll);

		
		// show the second header at the top-right corner 
		// only when vertical scroll bar is visible
		m_Header2.ShowWindow(bVScrollVisible?SW_SHOW:SW_HIDE);

		RedrawWindow();
	}
}

int CColumnTreeCtrl::InsertColumn(int nCol, LPCTSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem)
{
	// update the header control in upper-right corner
	// to make it look the same way as main header
	// Call UpdateColumns() after all columns are inserted!

	HDITEM hditem;
	hditem.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
	hditem.fmt = nFormat;
	hditem.cxy = nWidth;
	hditem.pszText = (LPTSTR)lpszColumnHeading;
	m_arrColFormats[nCol] = nFormat;
	int indx = m_Header.InsertItem(nCol, &hditem);

	if(m_Header.GetItemCount()>0) 
	{
		// if the main header contains items, 
		// insert an item to m_Header2
		hditem.pszText = _T("");
		hditem.cxy = GetSystemMetrics(SM_CXVSCROLL)+5;
		m_Header2.InsertItem(0,&hditem);
	};
	
	return indx;
}

void CColumnTreeCtrl::SetFirstColumnMinWidth(UINT uMinWidth)
{
	// set minimum width value for the first column
	m_uMinFirstColWidth = uMinWidth;
}

// Call this function to determine the location of the specified point 
// relative to the client area of a tree view control.
HTREEITEM CColumnTreeCtrl::HitTest(CPoint pt, UINT* pFlags) const
{
	CTVHITTESTINFO htinfo = {pt, 0, NULL, 0};
	HTREEITEM hItem = HitTest(&htinfo);
	if(pFlags)
	{
		*pFlags = htinfo.flags;
	}
	return hItem;
}

// Call this function to determine the location of the specified point 
// relative to the client area of a tree view control.
HTREEITEM CColumnTreeCtrl::HitTest(CTVHITTESTINFO* pHitTestInfo) const
{
	// We should use our own HitTest() method, because our custom tree
	// has different layout than the original CTreeCtrl.

	UINT uFlags = 0;
	HTREEITEM hItem = NULL;
	CRect rcItem, rcClient;
	
	CPoint point = pHitTestInfo->pt;
	point.x += m_xPos;
	point.y -= m_cyHeader;

	hItem = m_Tree.HitTest(point, &uFlags);

	// Fill the CTVHITTESTINFO structure
	pHitTestInfo->hItem = hItem;
	pHitTestInfo->flags = uFlags;
	pHitTestInfo->iSubItem = 0;
		
	if (! (uFlags&TVHT_ONITEMLABEL || uFlags&TVHT_ONITEMRIGHT) )
		return hItem;

	// Additional check for item's label.
	// Determine correct subitem's index.

	int i;
	int x = 0;
	for(i=0; i<m_Header.GetItemCount(); i++)
	{
		if(point.x>=x && point.x<=x+m_arrColWidths[i])
		{
			pHitTestInfo->iSubItem = i;
			pHitTestInfo->flags = TVHT_ONITEMLABEL;
			return hItem;
		}
		x += m_arrColWidths[i];
	}	
	
	pHitTestInfo->hItem = NULL;
	pHitTestInfo->flags = TVHT_NOWHERE;
	return NULL;
}


BOOL CColumnTreeCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	// we need to forward all notifications to the parent window,
    // so use OnNotify() to handle all notifications in one step
	
	LPNMHDR pHdr = (LPNMHDR)lParam;
	
	// there are several notifications we need to precess
		
	if(pHdr->code==NM_CUSTOMDRAW && pHdr->idFrom == TreeID)
	{
		OnTreeCustomDraw(pHdr,pResult);
		return TRUE; // do not forward
	}
	
	if(pHdr->code==HDN_ITEMCHANGING && pHdr->idFrom == HeaderID)
	{
		OnHeaderItemChanging(pHdr,pResult);
		return TRUE; // do not forward
	}
	
	if(pHdr->code==HDN_ITEMCHANGED && pHdr->idFrom == HeaderID)
	{
		OnHeaderItemChanged(pHdr,pResult);
		return TRUE; // do not forward
	}

	if(pHdr->code==TVN_ITEMEXPANDED && pHdr->idFrom == TreeID)
	{
		RepositionControls(); // ... and forward
	}

	// forward notifications from children to the control owner
	pHdr->hwndFrom = GetSafeHwnd();
	pHdr->idFrom = GetWindowLong(GetSafeHwnd(),GWL_ID);
	return (BOOL)GetParent()->SendMessage(WM_NOTIFY,wParam,lParam);
		
}

void CColumnTreeCtrl::OnCancelMode()
{
	m_Header.SendMessage(WM_CANCELMODE);
	m_Header2.SendMessage(WM_CANCELMODE);
	m_Tree.SendMessage(WM_CANCELMODE);
	m_horScroll.SendMessage(WM_CANCELMODE);
	
}
void CColumnTreeCtrl::OnEnable(BOOL bEnable)
{
	m_Header.EnableWindow(bEnable);
	m_Header2.EnableWindow(bEnable);
	m_Tree.EnableWindow(bEnable);
	m_horScroll.EnableWindow(bEnable);
}

HTREEITEM CColumnTreeCtrl::InsertItem(LPVOID pData,LPCSTR label,int nIcon,UINT nState,LPCSTR *colstr,HTREEITEM hParent /*=TVI_ROOT*/,HTREEITEM hAfter /*=TVI_LAST*/)
{
	int cols=m_Header.GetItemCount();
	CString s(label);
	for(int i=1;i<cols;i++) {
		s+='\t';
		s+=*colstr++;
	}
	HTREEITEM hTree = m_Tree.InsertItem(s,nIcon,nState,hParent,hAfter);
	m_Tree.SetItemData(hTree,(DWORD_PTR)pData);
	return hTree;
}

BOOL CColumnTreeCtrl::OnRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	return m_Tree.OnRClick(pNMHDR, pResult);
}

/*
BOOL CColumnTreeCtrl::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	return m_Tree.OnDblclk(pNMHDR, pResult);
}
*/
BOOL CColumnTreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	return m_Tree.OnSelchanged(pNMHDR, pResult);
}
BOOL CColumnTreeCtrl::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
	return m_Tree.OnSelchanging(pNMHDR, pResult);
}

void CColumnTreeCtrl::OnSetFocus( CWnd* pOldWnd ) 
{
	//CWnd::OnSetFocus( pOldWnd );
	m_Tree.SetFocus();
}
