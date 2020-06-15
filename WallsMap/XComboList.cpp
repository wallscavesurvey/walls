// XComboList.cpp
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// This software is released into the public domain.
// You are free to use it in any way you like.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XComboList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT NEAR WM_XCOMBOLIST_VK_ESCAPE = ::RegisterWindowMessage(_T("WM_XCOMBOLIST_VK_ESCAPE"));
UINT NEAR WM_XCOMBOLIST_LBUTTONUP = ::RegisterWindowMessage(_T("WM_XCOMBOLIST_LBUTTONUP"));

BEGIN_MESSAGE_MAP(CFocusedListBox, CListBox)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CFocusedListBox::OnKillFocus(CWnd *pNewWnd)
{
	CListBox::OnKillFocus(pNewWnd);
	ASSERT(((CXComboList *)GetParent())->m_bActive);
	((CXComboList *)GetParent())->m_pParent->PostMessage(WM_XCOMBOLIST_VK_ESCAPE, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CXComboList, CWnd)
	//{{AFX_MSG_MAP(CXComboList)
	ON_WM_LBUTTONDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_CREATE()
	ON_WM_VSCROLL()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// OnKillFocus
void CXComboList::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	if (m_bActive) {
		m_pParent->PostMessage(WM_XCOMBOLIST_VK_ESCAPE, 0, 0);
	}
}

CXComboList::CXComboList(CWnd *pParent)
{
	m_pParent = pParent;
	ASSERT(m_pParent);
	m_bFirstTime = TRUE;
	m_bActive = false;
}

CXComboList::~CXComboList()
{
}

///////////////////////////////////////////////////////////////////////////////
// SetActive
void CXComboList::SetActive(int nScrollBarWidth, int nItemsVisible)
{
	if (!::IsWindow(m_ListBox.m_hWnd))
		return;

	m_ListBox.SetFocus(); //needed to select items in listbox

	if (!m_bFirstTime) return;
	m_bFirstTime = FALSE;

	CRect rect;
	GetWindowRect(&rect);

	// set listbox size according to item height
	int nItemHeight = m_ListBox.GetItemHeight(0);

	CRect lbrect;
	GetClientRect(&lbrect);
	lbrect.top += 1;
	lbrect.bottom = lbrect.top + /*(rect.Height()/ nItemHeight)*/ nItemsVisible * nItemHeight;
	lbrect.left += 1;
	lbrect.right -= nScrollBarWidth;

	//int nItemsInView = (lbrect.Height()) / nItemHeight;

	// set size of listbox wrapper (from size of listbox)
	rect.bottom = rect.top + lbrect.Height() + 4;
	MoveWindow(&rect);
	m_ListBox.MoveWindow(&lbrect);
	m_ListBox.BringWindowToTop();

	int nCurSel = m_ListBox.GetCurSel();
	// set top index, to force selected item to be in view
	m_ListBox.SetTopIndex(nCurSel > 0 ? nCurSel - 1 : 0);

	if (nScrollBarWidth) {
		// set size and position for vertical scroll bar
		CRect sbrect;
		sbrect = lbrect;
		sbrect.left = lbrect.right;
		sbrect.right += nScrollBarWidth;
		m_wndSBVert.MoveWindow(&sbrect);

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		m_wndSBVert.GetScrollInfo(&si);

		// set info for scrollbar
		si.nMin = 0;
		si.nMax = m_ListBox.GetCount();
		if (si.nMax < 0)
			si.nMax = 1;
		si.nPage = nItemsVisible;

		if (nCurSel == LB_ERR || nCurSel < 0)
			nCurSel = 0;
		si.nPos = nCurSel;

		if (si.nPos < 0)
			si.nPos = 0;
		m_wndSBVert.SetScrollInfo(&si);
		m_wndSBVert.SetScrollPos(si.nPos, TRUE);
	}

	m_bActive = true;

	//RedrawWindow();
	SetTimer(1, 60, NULL);
}

///////////////////////////////////////////////////////////////////////////////
// GetScrollBarCtrl
CScrollBar* CXComboList::GetScrollBarCtrl(int nBar)
{
	UNUSED_ALWAYS(nBar);
	return &m_wndSBVert;
}

///////////////////////////////////////////////////////////////////////////////
// OnLButtonDown
void CXComboList::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_pParent->PostMessageA(WM_XCOMBOLIST_LBUTTONUP, 0, 0);
	CWnd::OnLButtonUp(nFlags, point);
}

///////////////////////////////////////////////////////////////////////////////
// PreTranslateMessage
BOOL CXComboList::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
	{
		///////////////////////////////////////////////////////////////////
		// we need to trap all cursor keys & alpha keys to reposition the 
		// scroll bar
		///////////////////////////////////////////////////////////////////

		//TRACE("   WM_KEYDOWN\n");

		SCROLLINFO si =
		{
			sizeof(SCROLLINFO),
			SIF_ALL | SIF_DISABLENOSCROLL,
		};
		m_wndSBVert.GetScrollInfo(&si);
		BOOL bSetScrollInfo = FALSE;
		int nIndex = 0;
		if (::IsWindow(m_ListBox.m_hWnd))
			nIndex = m_ListBox.GetCurSel();
		if (nIndex == LB_ERR || nIndex < 0)
			nIndex = 0;

		// use index from listbox, because scroll position cannot be relied
		// upon here

		switch (pMsg->wParam)
		{
		case VK_RETURN:
			m_pParent->PostMessageA(WM_XCOMBOLIST_LBUTTONUP, 0, 0);
			break;

		case VK_SPACE:
		case VK_ESCAPE:
			m_pParent->PostMessageA(WM_XCOMBOLIST_VK_ESCAPE, 0, 0);
			break;

			// handle scrolling messages
		case VK_DOWN:
			si.nPos = nIndex + 1;
			bSetScrollInfo = TRUE;
			break;

		case VK_END:
			si.nPos = si.nMax;
			bSetScrollInfo = TRUE;
			break;

		case VK_HOME:
			si.nPos = 0;
			bSetScrollInfo = TRUE;
			break;

		case VK_NEXT:			// PAGE DOWN
			si.nPos = nIndex + (si.nPage - 1);
			bSetScrollInfo = TRUE;
			break;

		case VK_PRIOR:			// PAGE UP
			si.nPos = nIndex - (si.nPage - 1);
			bSetScrollInfo = TRUE;
			break;

		case VK_UP:
			si.nPos = nIndex - 1;
			bSetScrollInfo = TRUE;
			break;

		default:
			if (pMsg->wParam >= 0x41/*VK_A*/ && pMsg->wParam <= 0x5A/*VK_Z*/)
			{
				// this was an alpha key - try to find listbox index
				CString strAlpha;
				strAlpha = (_TCHAR)pMsg->wParam;
				int nIndex2 = 0;
				if (::IsWindow(m_ListBox.m_hWnd))
					nIndex2 = m_ListBox.FindString(nIndex, strAlpha);
				if (nIndex2 != LB_ERR)
				{
					si.nPos = nIndex2;
					bSetScrollInfo = TRUE;
				}
			}
			break;
		}

		if (bSetScrollInfo)
		{
			// update scrollbar
			if (si.nPos < 0)
				si.nPos = 0;
			if (si.nPos > si.nMax)
				si.nPos = si.nMax;
			m_wndSBVert.SetScrollInfo(&si);
		}

		break;
	}

	case WM_LBUTTONUP:
		m_pParent->PostMessageA(WM_XCOMBOLIST_LBUTTONUP, 0, 0);
		break;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

///////////////////////////////////////////////////////////////////////////////
// OnCreate
int CXComboList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// create the listbox that we're wrapping
	VERIFY(m_ListBox.Create(WS_VISIBLE | WS_CHILD | LBS_NOINTEGRALHEIGHT/*|WS_BORDER*/,
		CRect(0, 0, 0, 0), this, 0));

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnVScroll
void CXComboList::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar*)
{
	if (!::IsWindow(m_ListBox.m_hWnd))
		return;

	// forward scroll message to listbox
	const MSG* pMsg = GetCurrentMessage();
	m_ListBox.SendMessage(WM_VSCROLL, pMsg->wParam, pMsg->lParam);

	SCROLLINFO si =
	{
		sizeof(SCROLLINFO),
		SIF_ALL | SIF_DISABLENOSCROLL,
	};
	m_wndSBVert.GetScrollInfo(&si);

	switch (nSBCode)
	{
	case SB_BOTTOM:			// scroll to bottom
		si.nPos = si.nMax;
		break;
	case SB_TOP:			// scroll to top
		si.nPos = 0;
		break;
	case SB_PAGEDOWN:		// scroll one page down
		si.nPos += si.nPage;
		break;
	case SB_PAGEUP:			// scroll one page up
		si.nPos -= si.nPage;
		break;
	case SB_LINEDOWN:		// scroll one line up
		si.nPos += 1;
		break;
	case SB_LINEUP:			// scroll one line up
		si.nPos -= 1;
		break;
	case SB_THUMBTRACK:		// drag scroll box to specified position. The 
							// current position is provided in nPos
	case SB_THUMBPOSITION:	// scroll to the absolute position. The current 
							// position is provided in nPos
		si.nPos = nPos;
		break;
	case SB_ENDSCROLL:		// end scroll
		return;
	default:
		break;
	}

	if (si.nPos < 0)
		si.nPos = 0;
	if (si.nPos > si.nMax)
		si.nPos = si.nMax;
	m_wndSBVert.SetScrollInfo(&si);
}

///////////////////////////////////////////////////////////////////////////////
// OnDestroy
void CXComboList::OnDestroy()
{
	KillTimer(1);

	if (::IsWindow(m_ListBox.m_hWnd))
		m_ListBox.DestroyWindow();


	CWnd::OnDestroy();
}

///////////////////////////////////////////////////////////////////////////////
// OnTimer
void CXComboList::OnTimer(UINT nIDEvent)
{
	UNUSED_ALWAYS(nIDEvent);

	if (!::IsWindow(m_ListBox.m_hWnd))
		return;

	// get current mouse position
	POINT point;
	::GetCursorPos(&point);
	ScreenToClient(&point);

	BOOL bOutside;
	int nIndex = m_ListBox.ItemFromPoint(point, bOutside);
	//TRACE("   nIndex=%d  bOutside=%d\n", nIndex, bOutside);

	if (!bOutside)
	{
		int nCurSel = m_ListBox.GetCurSel();

		if (nIndex != nCurSel)
			if (nIndex >= 0 && nIndex < m_ListBox.GetCount())
				m_ListBox.SetCurSel(nIndex);
	}
}
