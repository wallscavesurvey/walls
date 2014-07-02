#pragma once

#include "stdafx.h"
#include "MapLayer.h"

extern UINT WM_XHTMLTREE_CHECKBOX_CLICKED;
extern UINT WM_XHTMLTREE_END_DRAG;
extern UINT WM_XHTMLTREE_ITEM_RCLICKED;
extern UINT WM_XHTMLTREE_SELCHANGE;
extern UINT WM_XHTMLTREE_ITEM_DBLCLCK;
extern UINT WM_XHTMLTREE_SELCHANGE;

#define	COLOR_NONE ((DWORD)-1)

#define MAX_TREEDEPTH 6 //allow 6 subfolders, 8 levels

enum CheckedState {UNCHECKED=1, CHECKED=2, TRISTATE=3};

typedef struct _CTVHITTESTINFO { 
  POINT pt; 
  UINT flags; 
  HTREEITEM hItem; 
  int iSubItem;
} CTVHITTESTINFO;

class CCustomTreeChildCtrl : public CTreeCtrl
{
	friend class CColumnTreeCtrl;

	DECLARE_DYNAMIC(CCustomTreeChildCtrl)

public:
	CCustomTreeChildCtrl();
	virtual ~CCustomTreeChildCtrl();

	void Initialize();
	BOOL CreateCheckboxImages();
	BOOL GetBit(DWORD bits, DWORD bitmask) { return bits & bitmask; }
	
	CCustomTreeChildCtrl&	SetDragOps(DWORD dwOps)	{ m_dwDragOps = dwOps; return *this; }
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	bool		IsFolder(HTREEITEM hItem) {return GetImageIndex(hItem)<2;}
	bool		IsOpenFolder(HTREEITEM hItem) {return GetImageIndex(hItem)==1;}
	bool		HasSiblings(HTREEITEM hItem) {return GetNextSiblingItem(hItem) || GetPrevSiblingItem(hItem);}
	bool		IsAncestor(HTREEITEM hAncestor,HTREEITEM hChild);
	int			GetDepthBranch(HTREEITEM hItem);
	
	void		ExpandBranch(HTREEITEM hItem,bool bExpand);
	void		ExpandItem(HTREEITEM hItem,bool bExpand);

	int			GetImageIndex(HTREEITEM hItem);
	void		DeleteBranch(HTREEITEM hItem);

	HTREEITEM	GetNextItem(HTREEITEM hItem);
				// do not hide CTreeCtrl version
	HTREEITEM	GetNextItem(HTREEITEM hItem, UINT nCode)	
				{ return CTreeCtrl::GetNextItem(hItem, nCode); }
	HTREEITEM	GetPrevItem(HTREEITEM hItem);
	HTREEITEM	GetLastItem(HTREEITEM hItem=NULL);
	
	void		SetCheck(HTREEITEM hItem, bool fCheck = true);
	BOOL		AreAncestorsEnabled(HTREEITEM hItem);
	bool		EnableChildren(HTREEITEM hItem);
	void		DisableChildren(HTREEITEM hItem);
	void		DisableAncestors(HTREEITEM hItem);

	PTL GetPTL(HTREEITEM hItem)
	{
		return (PTL)GetItemData(hItem);
	}

	int	GetNumAncestors(HTREEITEM hItem)
	{
		int n=0;
		while(hItem=GetParentItem(hItem)) n++;
		return n;
	}

	UINT GetCheckedState(HTREEITEM hItem)
	{
		return GetItemState(hItem, TVIS_STATEIMAGEMASK)>>12;
	}

	bool IsExpanded(HTREEITEM hItem)
	{
		return ItemHasChildren(hItem) && 0!=(GetItemState(hItem,TVIS_EXPANDED) & TVIS_EXPANDED);
	}
	HTREEITEM GetLastClientItem();
	HTREEITEM GetLastClientItem0();

	void MoveItem(HTREEITEM hDrag,HTREEITEM hDrop,bool bBelow);

	HTREEITEM InsertItem(LPTVINSERTSTRUCT lpInsertStruct);
	HTREEITEM InsertItem(LPCTSTR lpszItem, int nImage,UINT state, 
					HTREEITEM hParent = TVI_ROOT, 
					HTREEITEM hInsertAfter = TVI_LAST);

	void SetCheckedState(HTREEITEM hItem,UINT nState)
	{
		SetItemState(hItem,INDEXTOSTATEIMAGEMASK(nState),TVIS_STATEIMAGEMASK);
	}
	BOOL m_bDestroyingTree;

private:
	DECLARE_MESSAGE_MAP()

	CImageList		m_StateImage;
	int				m_nHorzPos;				// initial horz scroll position - saved

	int m_nFirstColumnWidth; // the width of the first column 
	int m_nOffsetX;      	 // offset of this window inside the parent

	LRESULT SendRegisteredMessage(UINT nMessage,HTREEITEM hItem,LPARAM lParam=0)
	{
		CWnd *pWnd=GetParent();
		if(pWnd && (pWnd=pWnd->GetParent())) return pWnd->SendMessage(nMessage, (WPARAM)hItem, lParam);
		return 0;
	}
	LRESULT PostRegisteredMessage(UINT nMessage,HTREEITEM hItem,LPARAM lParam=0)
	{
		CWnd *pWnd=GetParent();
		if(pWnd && (pWnd=pWnd->GetParent())) return pWnd->PostMessage(nMessage, (WPARAM)hItem, lParam);
		return 0;
	}

	//virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	CHeaderCtrl *m_pHeader;

	bool			m_bButtonDown;
	bool			m_bDragging;
	DWORD			m_dwDragOps;			// drag features
	//HCURSOR			m_hNoDropCursor;		// no-drop cursor handle
	//HCURSOR			m_hDropMoveCursor;		// drop (move) cursor handle
	//HCURSOR			m_hPreviousCursor;

	HTREEITEM		m_hDragItem;
	int				m_iDragItemDepth;
	HTREEITEM		m_hDropItem,m_hLineItem;
	bool			m_bLineBelow,m_bDropBelow;
	UINT_PTR		m_tmAutoExpand;
	UINT_PTR		m_tmAutoScroll;
	int				m_iScrollDir,m_idir;

	DWORD			m_dwDropHoverTime;		// number of ticks over a drop item
	//#endif XHTMLDRAGDROP

	HTREEITEM		m_hPreviousSel;

	int			GetScrollDir(CPoint point);
	BOOL		IsCtrlDown();
	//void		AdjustEditRect();
	
//#ifdef // XHTMLDRAGDROP

	HTREEITEM GetDropParent(HTREEITEM &hItem,CPoint point);

	void KillScrollTimer()
	{
		if(m_iScrollDir) {
			m_iScrollDir=0;
			KillTimer(m_tmAutoScroll);
			m_tmAutoScroll=NULL;
		}
	}
	void KillExpandTimer()
	{
		if(m_tmAutoExpand) {
			KillTimer(m_tmAutoExpand);
			m_tmAutoExpand=NULL;
		}
	}

	void DragToPoint(HTREEITEM hItem,CPoint point);

	void DrawTheLines(HTREEITEM hItem,bool bBelow=true);

	HTREEITEM	IsOverItem(LPPOINT lpPoint = NULL);

	HTREEITEM	CopyItem(HTREEITEM hBranch, 
						   HTREEITEM hNewParent,
						   HTREEITEM hAfter = TVI_LAST);
	HTREEITEM	CopyBranch(HTREEITEM hItem, 
						 HTREEITEM hNewParent,
						 HTREEITEM hAfter = TVI_LAST);

	void BeginDrag(HTREEITEM hItem);

//#endif // XHTMLDRAGDROP

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg BOOL OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSelchanged(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg BOOL OnSelchanging(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnTabletQuerySystemGestureStatus(WPARAM,LPARAM);
};
