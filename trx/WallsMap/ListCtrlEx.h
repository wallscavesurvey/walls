#pragma once

#include "CheckHeadCtrl.h"

extern UINT urm_MOVE_ITEM;

// CListCtrlEx

class CListCtrlEx : public CListCtrl
{
	DECLARE_DYNAMIC(CListCtrlEx)

public:
	CListCtrlEx();
	virtual ~CListCtrlEx();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	void InitHdrCheck();

	CCheckHeadCtrl m_checkHeadCtrl;
	bool m_bHdrInit;

	void SetSelected(int i)
	{
		SetItemState(i,LVIS_SELECTED,LVIS_SELECTED);
		SetItemState(i,LVIS_FOCUSED,LVIS_FOCUSED);
	}
	
private:
	bool m_bDragging;
	bool m_bDropBelow;
	int m_nDropIndex;
	int m_nDragIndex;
	int m_nItemHeight;
	CRect m_crRange;

	CImageList	m_checkImgList;

	UINT_PTR m_nTimer;
	int m_nTimerDir;

	void DrawTheLines(int Index);
	void CellHitTest(const CPoint& pt, int& nRow, int& nCol) const;

protected:
	//virtual void PreSubclassWindow();
	//afx_msg void OnHdrBeginTrack(NMHDR* pNMHDR, LRESULT* pResult); 
	//afx_msg void OnHdrDblClick(NMHDR* pNMHDR, LRESULT* pResult); 
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnMovetoTop();
	afx_msg void OnMovetobottom();
	afx_msg LRESULT OnTabletQuerySystemGestureStatus(WPARAM,LPARAM);

	DECLARE_MESSAGE_MAP()
};


