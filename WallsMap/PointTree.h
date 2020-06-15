#pragma once


// CPointTree

class CPointTree : public CTreeCtrl
{
	DECLARE_DYNAMIC(CPointTree)

public:
	CPointTree();
	virtual ~CPointTree();
	HTREEITEM MoveItem(HTREEITEM hDragItem, HTREEITEM hDropItem);

private:
	CImageList*	m_pDragImage;
	bool		m_bLDragging;
	HTREEITEM	m_hitemDrag, m_hitemDrop;
	HCURSOR m_hcArrow, m_hcNo;
	bool m_bDraggingRoot;

	BOOL CheckSelect(CPoint &point);

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnTabletQuerySystemGestureStatus(WPARAM, LPARAM);
};


