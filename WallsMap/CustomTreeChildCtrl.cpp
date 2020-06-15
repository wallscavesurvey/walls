#include "stdafx.h"
#include "resource.h"
#include "CustomTreeChildCtrl.h"
//#include "CreateCheckboxImageList.h"

//#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//=============================================================================
// REGISTERED XHTMLTREE MESSAGES
//=============================================================================
UINT WM_XHTMLTREE_CHECKBOX_CLICKED = ::RegisterWindowMessage(_T("WM_XHTMLTREE_CHECKBOX_CLICKED"));
UINT WM_XHTMLTREE_END_DRAG = ::RegisterWindowMessage(_T("WM_XHTMLTREE_END_DRAG"));
UINT WM_XHTMLTREE_ITEM_RCLICKED = ::RegisterWindowMessage(_T("WM_XHTMLTREE_ITEM_RCLICKED"));
UINT WM_XHTMLTREE_ITEM_DBLCLCK = ::RegisterWindowMessage(_T("WM_XHTMLTREE_ITEM_DBLCLCK"));
UINT WM_XHTMLTREE_SELCHANGE = ::RegisterWindowMessage(_T("WM_XHTMLTREE_SELCHANGE"));
//const UINT HOT_TIMER			= 1;
const UINT LBUTTONDOWN_TIMER = 2;
const UINT AUTOEXPAND_TIMER = 6;
const UINT AUTOSCROLL_TIMER = 7;

const DWORD MIN_HOVER_TIME = 1500;		// 1.5 seconds
const int SCROLL_ZONE = 16;		// pixels for scrolling

//--------------------------------------------------------------------------------
// CCustomTreeChildCtrl Implementation
//--------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CCustomTreeChildCtrl, CTreeCtrl)

CCustomTreeChildCtrl::CCustomTreeChildCtrl()
	: m_bDestroyingTree(FALSE),
	m_bButtonDown(false),
	m_bDragging(false),
	m_tmAutoExpand(NULL),
	m_tmAutoScroll(NULL),
	m_iScrollDir(0),
	m_hDragItem(0),
	m_pHeader(0),
	m_hDropItem(0),
	m_hPreviousSel(0),
	m_bDropBelow(false),
	m_dwDropHoverTime(0)
	//m_hNoDropCursor(0),
	//m_hDropMoveCursor(0),
	//m_hPreviousCursor(0),
{
	//m_hPreviousCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);

}

CCustomTreeChildCtrl::~CCustomTreeChildCtrl()
{

	if (m_StateImage.GetSafeHandle())
		m_StateImage.DeleteImageList();

	//if (m_hNoDropCursor) DestroyCursor(m_hNoDropCursor);
	//if (m_hDropMoveCursor) DestroyCursor(m_hDropMoveCursor);

}

BEGIN_MESSAGE_MAP(CCustomTreeChildCtrl, CTreeCtrl)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//ON_WM_RBUTTONUP()
	//ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, OnTabletQuerySystemGestureStatus)
END_MESSAGE_MAP()

//=============================================================================
BOOL CCustomTreeChildCtrl::CreateCheckboxImages()
{
	if (m_StateImage.GetSafeHandle()) m_StateImage.DeleteImageList();
	//CBitmap bm;
	VERIFY(m_StateImage.Create(IDB_CHECKBOX4, 16, 1, RGB(255, 0, 255))); //no mask needed
	//VERIFY(m_StateImage.Create(16, 16, ILC_COLOR4, 4, 1)); //no mask needed
	//VERIFY(bm.LoadBitmap(IDB_CHECKBOX4));
	//m_StateImage.Add(&bm,RGB(255,0,255));
	SetImageList(&m_StateImage, TVSIL_STATE);
#if 0 && defined(_DEBUG)
	CImageList* pIL = pIL = GetImageList(TVSIL_STATE);
	ASSERT(pIL->GetImageCount() == 4);
	IMAGEINFO info;
	VERIFY(pIL->GetImageInfo(0, &info));
	CBitmap *pbm = CBitmap::FromHandle(info.hbmImage);
	ASSERT(pbm);
	BITMAP bmap;
	VERIFY(pbm->GetBitmap(&bmap));
	CDC *pDC = GetDC();
	VERIFY(SaveBitmapToFile(pbm, pDC, "IL_InitialW7.bmp"));
	VERIFY(ReleaseDC(pDC));
#endif
	return TRUE;
}

//=============================================================================
void CCustomTreeChildCtrl::Initialize()
//=============================================================================
{
	//Turning style off prevents assignment of correct image list under WinXP only!!
	//ModifyStyle(0,TVS_CHECKBOXES);
	SetImageList(NULL, TVSIL_STATE);

	if (m_StateImage.GetSafeHandle())
		m_StateImage.DeleteImageList();

	CreateCheckboxImages();
	m_bDestroyingTree = FALSE;
}

//---------------------------------------------------------------------------
// Message Handlers
//---------------------------------------------------------------------------

void CCustomTreeChildCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	UINT uFlags = 0;
	HTREEITEM hItem = HitTest(point, &uFlags);

	if (hItem) {
		if (TVHT_ONITEMSTATEICON&uFlags) {
			SetCheck(hItem, GetCheckedState(hItem) < CHECKED); //toggle checked state (TRISTATE vs CHECKED depends on context)
			return;
		}
		SelectItem(hItem);
		m_bButtonDown = true;
		return;
	}
	CTreeCtrl::OnLButtonDown(nFlags, point);
}

void CCustomTreeChildCtrl::MoveItem(HTREEITEM hDrag, HTREEITEM hDrop, bool bBelow)
{
	//PTL p=GetPTL(hDrop);
	HTREEITEM hNewParent = GetParentItem(hDrop);
	if (bBelow) {
		if (IsOpenFolder(hDrop)) {
			if (hDrag == GetChildItem(hDrop))
				return;
			hNewParent = hDrop;
			hDrop = TVI_FIRST;
		}
	}
	else {
		hDrop = GetPrevSiblingItem(hDrop);
		if (!hDrop) {
			hDrop = TVI_FIRST;
		}
	}
	m_bDragging = true; //suppress calls to parent during checkbox operations in CopyBranch
	hDrop = CopyBranch(hDrag, hNewParent, hDrop);
	m_bDragging = false;
	HTREEITEM hOldParent = GetParentItem(hDrag);
	DeleteBranch(hDrag);	// this was a move, not a copy
	if (hOldParent && !GetChildItem(hOldParent))
		SetItemImage(hOldParent, 1, 1);
	SendRegisteredMessage(WM_XHTMLTREE_END_DRAG, hDrop); //Parent will select hDrop after repairing m_layerset and reinitializing ptl->m_hItem
}

void CCustomTreeChildCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bButtonDown = false;
	if (m_bDragging) {
		VERIFY(ReleaseCapture());
		m_bDragging = false;
		KillExpandTimer();
		KillScrollTimer();
		if (m_hLineItem)
			DrawTheLines(0); //just clears when !m_bDragging

		if (m_hDropItem) {
			MoveItem(m_hDragItem, m_hDropItem, m_bDropBelow);
			m_hDropItem = 0;
		}
	}
	CTreeCtrl::OnLButtonUp(nFlags, point);
}

bool CCustomTreeChildCtrl::IsAncestor(HTREEITEM hAncestor, HTREEITEM hChild)
{
	do {
		if (hChild == hAncestor) return true;
	} while (hChild = GetParentItem(hChild));
	return false;
}

int CCustomTreeChildCtrl::GetDepthBranch(HTREEITEM hItem)
{
	//return number of subfolder levels contained in this folder --
	//0 if no children, 1 if only non-folder or empty-folder children, or 1+depth of deepest subfoldfer
	int depthMax = 0;
	hItem = GetChildItem(hItem);
	while (hItem) {
		int depth = 1 + GetDepthBranch(hItem);
		if (depth > depthMax) depthMax = depth;
		hItem = GetNextSiblingItem(hItem);
	}
	return depthMax;
}
int CCustomTreeChildCtrl::GetScrollDir(CPoint point)
{
	CRect rect;
	GetClientRect(&rect);
	if (point.x<rect.left || point.x>rect.right) return 0;
	if (point.y < rect.top) return -1;
	else if (point.y > rect.bottom) return 1;
	return 0;
}

HTREEITEM CCustomTreeChildCtrl::GetLastClientItem0()
{
	int n = GetVisibleCount();
	HTREEITEM h0 = GetFirstVisibleItem();
	while (--n && (h0 = GetNextVisibleItem(h0)));
	ASSERT(h0);
	return h0;
}

HTREEITEM CCustomTreeChildCtrl::GetLastClientItem()
{
	int n = GetVisibleCount();
	HTREEITEM h = GetFirstVisibleItem();
	ASSERT(h);
	for (; n > 1; n--) {
		if (IsExpanded(h)) h = GetChildItem(h);
		else {
			HTREEITEM h0 = h;
			while (!(h = GetNextSiblingItem(h0))) {
				if (!(h0 = GetParentItem(h0))) break;
			}
		}
	}
	ASSERT(h);
	return h;
}

HTREEITEM CCustomTreeChildCtrl::GetDropParent(HTREEITEM &hDropTarget, CPoint point)
{
	m_idir = 0;
	if (!hDropTarget) {
		if (!(m_idir = GetScrollDir(point))) {
			KillScrollTimer();
			return 0;
		}
		if (m_idir == m_iScrollDir) {
			ASSERT(m_tmAutoScroll);
			return 0; //handled in timer fcn
		}
		if (m_idir < 0) {
			hDropTarget = GetFirstVisibleItem(); //first item visibile in window
			ASSERT(hDropTarget);
			if (hDropTarget == m_hDragItem) return 0;
			//Moving above first visible item in client window --
			if (hDropTarget == GetRootItem()) m_idir = 0;
			hDropTarget = 0;
			return TVI_FIRST;
		}
		if (!m_hDropItem) {
			hDropTarget = GetLastVisibleItem(); //last unexpanded item in tree
			m_idir = 0;
		}
		else {
			hDropTarget = GetNextVisibleItem(m_hDropItem);
			if (hDropTarget) {
				LPCSTR p = (LPCSTR)GetItemData(hDropTarget);
				p = 0;
				EnsureVisible(hDropTarget);
			}
			if (!hDropTarget) {
				hDropTarget = m_hDropItem;
				m_idir = 0;
			}
		}
		LPCSTR p = (LPCSTR)GetItemData(hDropTarget);
		if (hDropTarget == m_hDragItem) return 0;
	}

	if (IsAncestor(m_hDragItem, hDropTarget))
		return 0; //cant't drag to own child

	HTREEITEM hNewParent = GetParentItem(hDropTarget);
	if (!hNewParent) hNewParent = TVI_ROOT;
	int nImage = GetImageIndex(hDropTarget);

	if (nImage < 2) {
		//Dragging to a folder --
		//if(GetChildItem(hDropTarget)==m_hDragItem)
			//return 0; //would not change tree
		if (m_iDragItemDepth + GetNumAncestors(hDropTarget) > MAX_TREEDEPTH)
			return IsExpanded(hDropTarget) ? 0 : hNewParent;
		return IsExpanded(hDropTarget) ? hDropTarget : hNewParent;
	}
	else if (m_iDragItemDepth + GetNumAncestors(hDropTarget) > MAX_TREEDEPTH + 1) return 0;

	return (GetNextSiblingItem(hDropTarget) == m_hDragItem) ? 0 : hNewParent;
}

void CCustomTreeChildCtrl::DragToPoint(HTREEITEM hItem, CPoint point)
{
	if (m_tmAutoExpand) { KillTimer(m_tmAutoExpand); m_tmAutoExpand = NULL; }

	HTREEITEM hParent = GetDropParent(hItem, point);
	if (hParent)
	{
		bool bNewBelow = (hItem != 0);
		if (!hItem) hItem = GetFirstVisibleItem();
		if (m_hDropItem != hItem || m_bDropBelow != bNewBelow)
		{
			m_hDropItem = hItem;
			m_bDropBelow = bNewBelow;
			m_dwDropHoverTime = GetTickCount();
			EnsureVisible(hItem);
			//UpdateWindow();
			DrawTheLines(hItem, bNewBelow);
			if (m_tmAutoExpand) { KillTimer(m_tmAutoExpand); m_tmAutoExpand = NULL; }
		}
		else if (ItemHasChildren(hItem) && m_iDragItemDepth + GetNumAncestors(hItem) <= MAX_TREEDEPTH) {
			// still over same item
			m_dwDropHoverTime = GetTickCount();
			if (!m_tmAutoExpand) {
				m_tmAutoExpand = SetTimer(AUTOEXPAND_TIMER, 100, NULL);
			}
		}
		if (m_idir) {
			m_tmAutoScroll = SetTimer(AUTOSCROLL_TIMER, 100, NULL);
			m_iScrollDir = m_idir; //scroll up
		}
	}
	else // not over an item
	{
		DrawTheLines(0);	// remove insert mark
		m_hDropItem = 0;
	}
}

void CCustomTreeChildCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// hItem will be non-zero if the cursor is anywhere over a valid item
	HTREEITEM hItem;
	UINT flags = 0;
	if (m_bDragging) {
		hItem = HitTest(point, &flags);
		DragToPoint(hItem, point);
		return;
	}
	if (m_bButtonDown) { //was nFlags&MK_LBUTTON
		m_bButtonDown = false;
		hItem = HitTest(point, &flags);
		if (hItem && !(TVHT_ONITEMSTATEICON&flags)) {
			BeginDrag(hItem);
			return;
		}
	}
	CTreeCtrl::OnMouseMove(nFlags, point);
}

//=============================================================================
HTREEITEM CCustomTreeChildCtrl::IsOverItem(LPPOINT lpPoint /*= NULL*/)
//=============================================================================
{
	CPoint point;
	if (lpPoint)
	{
		point = *lpPoint;
	}
	else
	{
		::GetCursorPos(&point);
		ScreenToClient(&point);
	}
	UINT flags = 0;
	HTREEITEM hItem = HitTest(point, &flags);

	return hItem;
}

void CCustomTreeChildCtrl::OnPaint()
{
	CRect rcClient;
	GetClientRect(&rcClient);

	CPaintDC dc(this);
	CDC dcMem;
	CBitmap bmpMem;

	// use temporary bitmap to avoid flickering
	dcMem.CreateCompatibleDC(&dc);
	if (bmpMem.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height()))
	{
		CBitmap* pOldBmp = dcMem.SelectObject(&bmpMem);

		// paint the window onto the memory bitmap

		CWnd::DefWindowProc(WM_PAINT, (WPARAM)dcMem.m_hDC, 0);

		// copy it to the window's DC
		dc.BitBlt(0, 0, rcClient.right, rcClient.bottom, &dcMem, 0, 0, SRCCOPY);

		dcMem.SelectObject(pOldBmp);

		bmpMem.DeleteObject();
	}
	dcMem.DeleteDC();

}

BOOL CCustomTreeChildCtrl::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;	// do nothing
}

BOOL CCustomTreeChildCtrl::AreAncestorsEnabled(HTREEITEM hItem)
{
	bool bTri = false;
	while (hItem = GetParentItem(hItem)) {
		UINT s = GetCheckedState(hItem);
		if (s != TRISTATE) return (s == CHECKED) ? (bTri ? 2 : 1) : 0;
		bTri = true;
	}
	return bTri ? 2 : 1;
}

bool CCustomTreeChildCtrl::EnableChildren(HTREEITEM hItem)
{
	//If any children would be made visible by checking this item,
	//switch them from TRISTATE to CHECKED
	//and return true. Do not, however, check hItem. Let caller do this!
	bool bRet = false;
	if (IsFolder(hItem)) {
		HTREEITEM hChild = GetChildItem(hItem);
		while (hChild) {
			UINT s = GetCheckedState(hChild);
			ASSERT(s == UNCHECKED || s == TRISTATE);
			if (s == TRISTATE && EnableChildren(hChild)) {
				SetCheckedState(hChild, CHECKED);
				bRet = true;
			}
			hChild = GetNextSiblingItem(hChild);
		}
	}
	else bRet = true;
	return bRet;
}

void CCustomTreeChildCtrl::DisableChildren(HTREEITEM hItem)
{
	//Make any CHECKED child items TRISTATE -- caller handles hItem
	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild) {
		if (CHECKED == GetCheckedState(hChild)) {
			DisableChildren(hChild);
			SetCheckedState(hChild, TRISTATE);
		}
		hChild = GetNextSiblingItem(hChild);
	}
}

void CCustomTreeChildCtrl::DisableAncestors(HTREEITEM hItem)
{
	//Make any CHECKED ancestors TRISTATE if unchecking this item
	//causes them to have no checked children --
	HTREEITEM hParent = GetParentItem(hItem);
	if (hParent && CHECKED == GetCheckedState(hParent)) {
		bool bSetTristate = true;
		HTREEITEM hChild = GetChildItem(hParent);
		while (hChild) {
			if (hChild != hItem && CHECKED == GetCheckedState(hChild)) {
				bSetTristate = false;
				break;
			}
			hChild = GetNextSiblingItem(hChild);
		}
		if (bSetTristate) {
			DisableAncestors(hParent);
			SetCheckedState(hParent, TRISTATE);
		}
	}
}

//=============================================================================
void CCustomTreeChildCtrl::SetCheck(HTREEITEM hItem, bool fCheck /*= true*/)
{
	ASSERT(hItem);

	UINT nState = fCheck ? CHECKED : UNCHECKED;

	if (fCheck) {
		BOOL bEnabled = AreAncestorsEnabled(hItem);
		if (bEnabled) {
			//Ancestors are all CHECKED or TRISTATE --
			//If no children would be made visible, switch nState to TRISTATE and leave ancestors untouched.
			//Otherwise, EnableChildren() will switch the newly visible children from TRISTATE to CHECKED and return true.
			if (!EnableChildren(hItem)) {
				nState = TRISTATE;
			}
			else if (bEnabled == 2) {
				//Some immediate ancestors are TRISTATE --
				//Switch them to CHECKED since at least some children are now checked
				HTREEITEM h = hItem;
				while (h = GetParentItem(h)) {
					if (TRISTATE == GetCheckedState(h))
						SetCheckedState(h, CHECKED);
					else break;
				}
			}
		}
		else {
			nState = TRISTATE;
		}
	}
	else {
		//Changing CHECKED or TRISTATE state to UNCHECKED --
		if (TRISTATE != GetCheckedState(hItem)) {
			//No effect on tree if changing TRISTATE to UNCHECKED
			//Changing CHECKED to UNCHECKED --
			if (IsFolder(hItem)) {
				//Some child items must be CHECKED, so make them TRISTATE via a recursive fcn --
				DisableChildren(hItem);
			}
			//Make any CHECKED ancestors TRISTATE if unchecking this item
			//causes them to have no checked children --
			DisableAncestors(hItem);
		}
	}

	if (!m_bDragging)
		SendRegisteredMessage(WM_XHTMLTREE_CHECKBOX_CLICKED, hItem, (LPARAM)nState);

	SetCheckedState(hItem, nState);
}

//=============================================================================
// GetNextItem - Get next item in sequence (as if tree was completely expanded)
//   Returns - The item immediately below the reference item
HTREEITEM CCustomTreeChildCtrl::GetNextItem(HTREEITEM hItem)
//=============================================================================
{
	HTREEITEM hItemNext = NULL;
	ASSERT(hItem);
	if (hItem) {
		if (!(hItemNext = GetChildItem(hItem))) {
			while (!(hItemNext = GetNextSiblingItem(hItem))) {
				if (!(hItem = GetParentItem(hItem)))
					return NULL;
			}
		}
	}
	return hItemNext;
}

//=============================================================================
// GetNextItem  - Get previous item as if outline was completely expanded
HTREEITEM CCustomTreeChildCtrl::GetPrevItem(HTREEITEM hItem)
{
	HTREEITEM hItemPrev;
	hItemPrev = GetPrevSiblingItem(hItem);
	if (hItemPrev == NULL)
		hItemPrev = GetParentItem(hItem);
	else
		hItemPrev = GetLastItem(hItemPrev);
	return hItemPrev;
}

//=============================================================================
// GetLastItem  - Gets last item in the branch
HTREEITEM CCustomTreeChildCtrl::GetLastItem(HTREEITEM hItem)
//=============================================================================
{
	// Last child of the last child of the last child ...
	if (!hItem) {
		for (HTREEITEM h = GetRootItem(); h; hItem = h, h = GetNextSiblingItem(h));
	}
	if (hItem) {
		while (ItemHasChildren(hItem)) {
			HTREEITEM hItemNext = GetChildItem(hItem);
			while (hItemNext) {
				hItem = hItemNext;
				hItemNext = GetNextSiblingItem(hItemNext);
			}
		}
	}
	return hItem;
}

BOOL CCustomTreeChildCtrl::PreTranslateMessage(MSG* pMsg)
//=============================================================================
{
	// allow edit control to receive messages, if 
	// label is being edited
	if (GetEditControl() &&
		((pMsg->message == WM_CHAR) ||
		(pMsg->message == WM_KEYDOWN) ||
			GetKeyState(VK_CONTROL)))
	{
		::TranslateMessage(pMsg);
		::DispatchMessage(pMsg);
		return TRUE;
	}


	//=========================================================================
	// WM_CHAR
	//=========================================================================
	if (pMsg->message == WM_CHAR)
	{
		if ((pMsg->wParam == VK_SPACE))
		{
			HTREEITEM hItem = GetSelectedItem();

			if (hItem)		//+++1.6
			{
				SetCheck(hItem, GetCheckedState(hItem) < CHECKED);
			}
			return TRUE;
		}
	}

	//=========================================================================
	// WM_KEYDOWN
	//=========================================================================
	if (pMsg->message == WM_KEYDOWN)
	{
		//=====================================================================
		// VK_ESCAPE while dragging
		//=====================================================================
		if (m_bDragging && pMsg->wParam == VK_ESCAPE)
		{
			VERIFY(ReleaseCapture());
			m_bDragging = m_bButtonDown = false;
			KillExpandTimer();
			KillScrollTimer();
			DrawTheLines(0); //just clears when !m_bDragging
			return TRUE;
		}
	}
	return CTreeCtrl::PreTranslateMessage(pMsg);
}

void CCustomTreeChildCtrl::ExpandItem(HTREEITEM hItem, bool bExpand)
{
	if (IsExpanded(hItem) != bExpand) {
		Expand(hItem, bExpand ? TVE_EXPAND : TVE_COLLAPSE);
		//bExpand == index in image list (uselected and selected), closed/open folder icon
		SetItemImage(hItem, bExpand, bExpand);
	}
}

void CCustomTreeChildCtrl::ExpandBranch(HTREEITEM hItem, bool bExpand)
{
	if (hItem && ItemHasChildren(hItem)) {
		ExpandItem(hItem, bExpand);
		if (hItem = GetChildItem(hItem)) {
			do ExpandBranch(hItem, bExpand);
			while (hItem = GetNextSiblingItem(hItem));
		}
	}
}

int CCustomTreeChildCtrl::GetImageIndex(HTREEITEM hItem)
{
	TVITEM tv;
	tv.hItem = hItem;
	tv.mask = TVIF_IMAGE;
	VERIFY(GetItem(&tv));
	return tv.iImage;
}

//=============================================================================
// OnTimer - check if mouse has left, turn off underlining and hot state
void CCustomTreeChildCtrl::OnTimer(UINT nIDEvent)
//=============================================================================
{
	if (nIDEvent == m_tmAutoScroll) {
		if (m_bDragging) {
			CPoint point;
			::GetCursorPos(&point);
			ScreenToClient(&point);
			m_iScrollDir = GetScrollDir(point);
			if (m_iScrollDir > 0) {
				HTREEITEM hItem = m_hDropItem ? GetNextVisibleItem(m_hDropItem) : 0;
				if (hItem) {
					DrawTheLines(0);
					SendMessage(WM_VSCROLL, SB_LINEDOWN);
					DragToPoint(hItem, point);
					return;
				}
			}
			else if (m_iScrollDir < 0) {
				HTREEITEM hItem = GetFirstVisibleItem();
				if (hItem && hItem != GetRootItem()) {
					hItem = GetPrevVisibleItem(hItem);
					DrawTheLines(0);
					SendMessage(WM_VSCROLL, SB_LINEUP);
					DragToPoint(hItem, point);
					return;
				}
			}
		}
		KillScrollTimer();
		return;
	}

	if (nIDEvent == m_tmAutoExpand) {
		CPoint point;
		::GetCursorPos(&point);
		ScreenToClient(&point);
		if (m_bDragging && IsOverItem(&point) == m_hDropItem) {
			if ((GetTickCount() - m_dwDropHoverTime) > MIN_HOVER_TIME) {
				// min hover time has passed, expand node if it has children
				DrawTheLines(0);
				ExpandItem(m_hDropItem, !IsExpanded(m_hDropItem));
				UpdateWindow();
				DrawTheLines(m_hDropItem, true);	// put insert mark after item
				m_dwDropHoverTime = GetTickCount();
			}
		}
		else {
			KillTimer(nIDEvent);
			m_tmAutoExpand = NULL;
		}
		return;
	}
	CTreeCtrl::OnTimer(nIDEvent);
}

//=============================================================================
void CCustomTreeChildCtrl::OnDestroy()
//=============================================================================
{
	CTreeCtrl::OnDestroy();
}

/*
//=============================================================================
void CCustomTreeChildCtrl::SetDragCursor()
//=============================================================================
{
	HCURSOR hCursor = m_hDropMoveCursor;

	if (hCursor)
		SetCursor(hCursor);
	else
		SetCursor(m_hPreviousCursor);
}
*/

//=============================================================================
void CCustomTreeChildCtrl::DrawTheLines(HTREEITEM hItem, bool bBelow/*=true*/)
{
	CDC *pDC = GetDC();
	CPen *pOldPen;
	CRect Rect;
	int y;

	if (m_hLineItem) {
		//Clear old line --
		GetItemRect(m_hLineItem, &Rect, 0);
		if (m_bLineBelow) {
			y = Rect.bottom;
			CPen PenGray(PS_SOLID, 1, ::GetSysColor(COLOR_BTNFACE));
			pOldPen = (CPen *)pDC->SelectObject(&PenGray);
			pDC->MoveTo(Rect.left, y - 1);
			pDC->LineTo(Rect.right - 1, y - 1);
			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(Rect.left, y);
			pDC->LineTo(Rect.right - 1, y);
			pDC->SelectObject(pOldPen);
		}
		else {
			y = Rect.top - 1;
			if (m_hLineItem == GetFirstVisibleItem()) y += 2;
			pOldPen = (CPen *)pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(Rect.left, y - 1);
			pDC->LineTo(Rect.right - 1, y - 1);
			pDC->MoveTo(Rect.left, y);
			pDC->LineTo(Rect.right - 1, y);
			pDC->SelectObject(pOldPen);
		}
	}

	if (!hItem || !m_bDragging) {
		m_hLineItem = 0;
		ReleaseDC(pDC);
		return;
	}

	m_hLineItem = hItem;
	m_bLineBelow = bBelow;
	GetItemRect(m_hLineItem, &Rect, 0);
	if (m_bLineBelow) {
		y = Rect.bottom;
	}
	else {
		y = Rect.top - 1;
		if (m_hLineItem == GetFirstVisibleItem()) y += 2;
	}
	pOldPen = (CPen *)pDC->SelectStockObject(BLACK_PEN);
	pDC->MoveTo(Rect.left, y - 1);
	pDC->LineTo(Rect.right - 1, y - 1);
	pDC->MoveTo(Rect.left, y);
	pDC->LineTo(Rect.right - 1, y);
	pDC->SelectObject(pOldPen);
	ReleaseDC(pDC);
}

//void CCustomTreeChildCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) 
void CCustomTreeChildCtrl::BeginDrag(HTREEITEM hItem)
{
	ASSERT(!m_bDragging);

	//HTREEITEM hItem = ((NMTREEVIEW*)pNMHDR)->itemNew.hItem;
	//ASSERT(hItem);
	if (!GetNextSiblingItem(hItem) && !GetPrevSiblingItem(hItem) && !GetParentItem(hItem))
		return;

	if (hItem != GetSelectedItem())
		SelectItem(hItem);

	m_bDragging = true;
	m_hDragItem = hItem;
	m_iDragItemDepth = GetDepthBranch(hItem);
	//SetTimer(LBUTTONDOWN_TIMER, 100, NULL);

	m_dwDropHoverTime = GetTickCount();
	m_hDropItem = NULL;
	m_hLineItem = NULL;

	SetCapture();
}


//=============================================================================
// CopyItem   - Copies an item to a new location
// Returns    - Handle of the new item
// hItem      - Item to be copied
// hNewParent - Handle of the parent for new item
// hAfter     - Item after which the new item should be created
HTREEITEM CCustomTreeChildCtrl::CopyItem(HTREEITEM hItem,
	HTREEITEM hNewParent,
	HTREEITEM hAfter /*= TVI_LAST*/)
	//=============================================================================
{
	HTREEITEM hNewItem = NULL;
	ASSERT(hItem);

	TVINSERTSTRUCT tvis;
	CString strText = _T("");

	// get information of the source item
	tvis.item.hItem = hItem;
	tvis.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_PARAM | TVIF_STATE |
		TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	GetItem(&tvis.item);
	strText = GetItemText(hItem);

	tvis.item.cchTextMax = strText.GetLength();
	tvis.item.pszText = strText.LockBuffer();

	// insert the item at proper location
	tvis.hParent = hNewParent;
	tvis.hInsertAfter = hAfter;
	tvis.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_STATE;

	hNewItem = InsertItem(&tvis);

	strText.UnlockBuffer();

	return hNewItem;
}

//=============================================================================
// CopyBranch - Moves all items in a branch to a new location
// Returns    - The new branch node
// hBranch    - The node that starts the branch
// hNewParent - Handle of the parent for new branch
// hAfter     - Item after which the new branch should be created
HTREEITEM CCustomTreeChildCtrl::CopyBranch(HTREEITEM hBranch,
	HTREEITEM hNewParent,
	HTREEITEM hAfter /*= TVI_LAST*/)
	//=============================================================================
{
	UINT nState = GetCheckedState(hBranch);
	SetCheck(hBranch, false);
	HTREEITEM hNewItem = CopyItem(hBranch, hNewParent, hAfter);
	HTREEITEM hChild = GetChildItem(hBranch);
	while (hChild)
	{
		// recursively transfer all the items
		CopyBranch(hChild, hNewItem);
		hChild = GetNextSiblingItem(hChild);
	}
	//if(nState>UNCHECKED) SetCheck(hNewItem,true); //omits unchecked box, vs11 version
	SetCheck(hNewItem, nState > UNCHECKED);
	if (IsExpanded(hBranch)) {
		Expand(hNewItem, TVE_EXPAND);
	}
	return hNewItem;
}

//=============================================================================
void CCustomTreeChildCtrl::DeleteBranch(HTREEITEM hItem)
//=============================================================================
{
	ASSERT(hItem);
	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild)
	{
		// recursively delete all the items
		HTREEITEM hNext = GetNextSiblingItem(hChild);
		DeleteBranch(hChild);
		hChild = hNext;
	}
	CTreeCtrl::DeleteItem(hItem);
}

#if 0
//=============================================================================
BOOL CCustomTreeChildCtrl::OnDblclk(NMHDR* /*pNMHDR*/, LRESULT* pResult)
//=============================================================================
{
	BOOL rc = 0;	// allow parent to handle

	CPoint point;
	::GetCursorPos(&point);
	ScreenToClient(&point);

	UINT flags = 0;
	HTREEITEM hItem = HitTest(point, &flags);

	if (hItem && !(flags == TVHT_ONITEMSTATEICON))
	{
		if (!IsFolder(hItem))
		{
			SendRegisteredMessage(WM_XHTMLTREE_ITEM_DBLCLCK, hItem, (LPARAM)GetItemData(hItem));
		}
	}

	*pResult = rc;
	return rc;
}
#endif

//=============================================================================
BOOL CCustomTreeChildCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
//=============================================================================
{
	BOOL rc = FALSE;	// allow parent to handle

	HTREEITEM hItem = ((NMTREEVIEW*)pNMHDR)->itemNew.hItem;

	if (m_bDestroyingTree || hItem == m_hPreviousSel)
		rc = TRUE;

	else {
		m_hPreviousSel = hItem;
		SendRegisteredMessage(WM_XHTMLTREE_SELCHANGE, hItem, (LPARAM)GetItemData(hItem));
	}

	*pResult = rc;
	return rc;
}

//=============================================================================
BOOL CCustomTreeChildCtrl::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
//=============================================================================
{
	BOOL rc = m_bDestroyingTree;	// allow parent to handle
	*pResult = rc;
	return rc;
}

//=============================================================================
HTREEITEM CCustomTreeChildCtrl::InsertItem(LPTVINSERTSTRUCT lpInsertStruct)
//=============================================================================
{
	TVINSERTSTRUCT tvis;
	memcpy(&tvis, lpInsertStruct, sizeof(TVINSERTSTRUCT));

	CString strText = tvis.item.pszText;
	tvis.item.pszText = strText.LockBuffer();

	HTREEITEM hItem = CTreeCtrl::InsertItem(&tvis);
	ASSERT(hItem);

	strText.UnlockBuffer();

	return hItem;
}

//=============================================================================
HTREEITEM CCustomTreeChildCtrl::InsertItem(LPCTSTR lpszItem,
	int nImage,
	UINT state,
	HTREEITEM hParent /*= TVI_ROOT*/,
	HTREEITEM hInsertAfter /*= TVI_LAST*/)
	//=============================================================================
{
	TVINSERTSTRUCT tvis = { 0 };

	tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvis.item.pszText = (LPTSTR)lpszItem;
	tvis.item.iImage = nImage;
	tvis.item.iSelectedImage = nImage;
	tvis.item.state = INDEXTOSTATEIMAGEMASK(state);
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;

	tvis.hParent = hParent;
	tvis.hInsertAfter = hInsertAfter;

	return InsertItem(&tvis);
}

void CCustomTreeChildCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	UINT flags = 0;
	HTREEITEM hItem = HitTest(point, &flags);
	if (flags & TVHT_ONITEMSTATEICON) {
#if 0 //def _DEBUG
		int n = GetVisibleCount(); //completely visible items
		HTREEITEM hLast = GetLastVisibleItem();
		static BOOL test = 1;
		SetInsertMark(test ? hLast : 0, test ? 1 : 0);
		test = !test;
#endif
		return;
	}
	if (hItem) {
		if (IsFolder(hItem)) {
			if (ItemHasChildren(hItem)) {
				Expand(hItem, TVE_TOGGLE);
				//nFlags=TVHT_ONITEMLABEL;
				//CTreeCtrl::OnLButtonDblClk(nFlags,point); //doesn't work
				int i = IsExpanded(hItem) ? 1 : 0;
				SetItemImage(hItem, i, i);
			}
			else {
				ASSERT(GetImageIndex(hItem) == 1); //empty folder always has open folder icon
				return;
			}
		}
		SendRegisteredMessage(WM_XHTMLTREE_ITEM_DBLCLCK, hItem, GetItemData(hItem));
	}
}

BOOL CCustomTreeChildCtrl::OnRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	CPoint point;
	::GetCursorPos(&point);
	CPoint ptClient(point);
	ScreenToClient(&ptClient);

	UINT flags = 0;
	HTREEITEM hItem = HitTest(ptClient, &flags);

	if (hItem && !(flags&TVHT_ONITEMSTATEICON))
	{
		//if(flags&TVHT_ONITEMLABEL)
		PostRegisteredMessage(WM_XHTMLTREE_ITEM_RCLICKED, hItem, (LPARAM)MAKELONG(point.x, point.y));
		SelectItem(hItem);
	}
	return (*pResult = 1);

}

void CCustomTreeChildCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	UINT flags = 0;
	HTREEITEM hItem = HitTest(point, &flags);
	if (!hItem || !(flags & TVHT_ONITEMSTATEICON))
		CTreeCtrl::OnRButtonDown(nFlags, point);
}

LRESULT CCustomTreeChildCtrl::OnTabletQuerySystemGestureStatus(WPARAM, LPARAM)
{
	return 0;
}
