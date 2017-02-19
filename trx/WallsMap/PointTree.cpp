// PointTree.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "showshapedlg.h"

// CPointTree

IMPLEMENT_DYNAMIC(CPointTree, CTreeCtrl)

CPointTree::CPointTree()
{
	m_pDragImage=NULL;
	m_bLDragging=false;
	m_hcArrow=::LoadCursor(NULL, IDC_ARROW);
	m_hcNo=::LoadCursor(NULL, IDC_NO);
}

CPointTree::~CPointTree()
{
	if(m_pDragImage) delete m_pDragImage;
}


BEGIN_MESSAGE_MAP(CPointTree, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, OnTabletQuerySystemGestureStatus)
END_MESSAGE_MAP()


// CPointTree message handlers

void CPointTree::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_bLDragging) {
		ASSERT(m_pDragImage);
		CPoint pt (point);
		ClientToScreen (&pt);

		// move the drag image
		VERIFY(m_pDragImage->DragMove(pt));

		// unlock window updates
		VERIFY (m_pDragImage->DragShowNolock (FALSE));

		// get the CWnd pointer of the window that is under the mouse cursor
		if(WindowFromPoint(pt)==this) {
			UINT uFlags;
			// get the item that is below cursor
			HTREEITEM h=HitTest(point,&uFlags);
			// highlight it
			if(h!=m_hitemDrop) {
				SelectDropTarget(m_hitemDrop=NULL);
				if(h && (TVHT_ONITEM & uFlags) &&
						(!m_bDraggingRoot || !GetParentItem(h)) && app_pShowDlg->IsSelDroppable(h)) {
					SelectDropTarget(m_hitemDrop=h);
				}
			}
			::SetCursor(m_hitemDrop?m_hcArrow:m_hcNo);
		}
		else {
			// turn off drag hilite for tree control
			if (m_hitemDrop)
			{
				SelectDropTarget (NULL);
				m_hitemDrop = NULL;
			}
			::SetCursor(m_hcNo);
		}
		// lock window updates
		VERIFY(m_pDragImage->DragShowNolock(TRUE));
	}
	else {
		::SetCursor(m_hcArrow);
	}
	CTreeCtrl::OnMouseMove(nFlags,point);
}

void CPointTree::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	ASSERT(GetSelectedItem()==pNMTreeView->itemNew.hItem);

	// Item user started dragging ...
	*pResult = 0;
	if(!app_pShowDlg->IsSelMovable()) return;

	m_hitemDrag = pNMTreeView->itemNew.hItem;
	m_bDraggingRoot=!GetParentItem(m_hitemDrag);

	// So user cant drag root node
	//if (GetParentItem(m_hitemDrag) == NULL) return ; 

	m_hitemDrop = NULL;

	if(m_pDragImage) {
		delete m_pDragImage;
		m_pDragImage=NULL;
	}

	CRect rect;
	GetItemRect(m_hitemDrag, rect, TRUE);
	rect.top = rect.left = 0;

	// Create bitmap
	CClientDC	dc(this);
	CDC 		memDC;

	if(!memDC.CreateCompatibleDC(&dc))
		return;

	CBitmap bitmap;
	if(!bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height()))
		return;

	CBitmap* pOldMemDCBitmap = memDC.SelectObject( &bitmap );
	CFont* pOldFont = memDC.SelectObject(GetFont());

	memDC.FillSolidRect(&rect, RGB(255, 255, 255)); // Here green is used as mask color
	memDC.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
	memDC.TextOut(rect.left,rect.top,GetItemText(m_hitemDrag));

	memDC.SelectObject( pOldFont );
	memDC.SelectObject( pOldMemDCBitmap );

	// Create CImageList
	m_pDragImage = new CImageList;
	VERIFY(m_pDragImage->Create(rect.Width(), rect.Height(),ILC_COLOR | ILC_MASK, 0, 1));
	m_pDragImage->Add(&bitmap, RGB(255, 255, 255)); // Here green is used as mask color

	m_bLDragging = true;
	m_pDragImage->BeginDrag(0, CPoint(-15,-15));
	POINT pt = pNMTreeView->ptDrag;
	ClientToScreen( &pt );
	m_pDragImage->DragEnter(NULL, pt);
	SetCapture();
}

void CPointTree::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bLDragging)
	{
		// end dragging
		ASSERT(m_pDragImage);
		ASSERT(m_hitemDrag==GetSelectedItem());
		VERIFY (m_pDragImage->DragLeave(GetDesktopWindow()));	
		m_pDragImage->EndDrag();
		// stop intercepting all mouse messages
		VERIFY(::ReleaseCapture());
		m_bLDragging = false;

		CPoint pt (point);
		ClientToScreen (&pt);
		// get the CWnd pointer of the window that is under the mouse cursor
		if((CWnd *)this==WindowFromPoint(pt)) {
			// unhilite the drop target
			SelectDropTarget (NULL);
		}
		delete m_pDragImage;
		m_pDragImage=NULL;
		if(m_hitemDrop && m_hitemDrag!=m_hitemDrop) {
			ASSERT(m_hitemDrag==GetSelectedItem());
			app_pShowDlg->OnDropTreeItem(m_hitemDrop);
		}
	}
	CTreeCtrl::OnLButtonUp(nFlags, point);
}

BOOL CPointTree::CheckSelect(CPoint &point)
{
	if(!m_bLDragging) {
		// get the item that is below cursor
		HTREEITEM h=HitTest(point);
		if(h && h!=GetSelectedItem()) {
			if(app_pShowDlg->CancelSelChange()) {
				return FALSE;
			}
			SelectItem(h);
		}
	}
	return TRUE;
}

void CPointTree::OnLButtonDown(UINT nFlags, CPoint point)
{
	if(CheckSelect(point))
		CTreeCtrl::OnLButtonDown(nFlags, point);
}

void CPointTree::OnRButtonDown(UINT nFlags, CPoint point)
{
	if(CheckSelect(point))
		CTreeCtrl::OnRButtonDown(nFlags, point);
}

HTREEITEM CPointTree::MoveItem(HTREEITEM hDragItem,HTREEITEM hDropItem)
{

	TV_INSERTSTRUCT tvis;

	tvis.item.hItem	= hDragItem;
	tvis.item.mask = TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_STATE|TVIF_PARAM;

	// get item that was dragged
	VERIFY(GetItem(&tvis.item));

	HTREEITEM hDropRoot=GetParentItem(hDropItem);
	if(!hDropRoot) {
		hDropRoot=hDropItem;
		tvis.hInsertAfter=TVI_FIRST;
	}
	else {
		tvis.hInsertAfter=hDropItem;
	}
	tvis.hParent=hDropRoot;
	tvis.item.pszText=LPSTR_TEXTCALLBACK;
	tvis.item.mask = TVIF_DI_SETITEM|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_TEXT|TVIF_STATE|TVIF_PARAM;

	VERIFY(hDropItem=InsertItem(&tvis));
	// delete the original item (move operation)
	VERIFY(DeleteItem(hDragItem));
	return hDropItem;
}

LRESULT CPointTree::OnTabletQuerySystemGestureStatus(WPARAM,LPARAM)
{
   return 0;
}
