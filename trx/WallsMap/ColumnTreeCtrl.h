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
#pragma once

#include "stdafx.h"
#ifdef _USE_THEMES
#include "HeaderCtrlH.h"
#endif
#include "CustomTreeChildCtrl.h"

class CColumnTreeCtrl : public CStatic
{
public:
	DECLARE_DYNCREATE(CColumnTreeCtrl)
	
	/*
	 * Construction/destruction
	 */
	 
	CColumnTreeCtrl();
	virtual ~CColumnTreeCtrl();

	// explicit construction 
	//BOOL Create(DWORD dwStyle , const RECT& rect, CWnd* pParentWnd, UINT nID);

	virtual void PreSubclassWindow();

	/*
	 *  Operations
	 */
	bool HasSiblings(HTREEITEM hItem) {return m_Tree.HasSiblings(hItem);}
	HTREEITEM GetParentItem(HTREEITEM hItem) {return m_Tree.GetParentItem(hItem);}
	void ClearTree();
	int GetColumnWidth(int i) {return m_arrColWidths[i];}

	virtual void AssertValid( ) const;

	CCustomTreeChildCtrl& GetTreeCtrl() { return m_Tree; }
	CHeaderCtrl& GetHeaderCtrl() { return m_Header; }

	HTREEITEM InsertItem(LPVOID pData,LPCSTR label,int nIcon,UINT nState,LPCSTR *colstr,HTREEITEM hParent=TVI_ROOT,HTREEITEM hAfter=TVI_LAST);
	void SetCheck(HTREEITEM hItem,bool bCheck=true) {m_Tree.SetCheck(hItem,bCheck);}
	int InsertColumn(int nCol,LPCTSTR lpszColumnHeading, int nFormat=0, int nWidth=-1, int nSubItem=-1);
	void UpdateColumns();

	void SetFirstColumnMinWidth(UINT uMinWidth);
		
	HTREEITEM HitTest(CPoint pt, UINT* pFlags=NULL) const;
	HTREEITEM HitTest(CTVHITTESTINFO* pHitTestInfo) const;

protected:

	DECLARE_MESSAGE_MAP()

	enum ChildrenIDs { HeaderID = 1, TreeID = 2, HScrollID = 3, Header2ID = 4};
	
	CCustomTreeChildCtrl m_Tree;
	CScrollBar m_horScroll;

#ifdef _USE_THEMES
	CHeaderCtrlH m_Header;
	CHeaderCtrlH m_Header2;
#else
	CHeaderCtrl m_Header;
	CHeaderCtrl m_Header2;
#endif

	CFont *m_pHdrFont;
	
	int m_cyHeader;
	int m_cxTotal;
	int m_xPos;
	int m_xOffset;
	int m_uMinFirstColWidth;
	BOOL m_bHeaderChangesBlocked;

	enum{MAX_COLUMN_COUNT=16}; // change this value if you need more than 16 columns

	int m_arrColWidths[MAX_COLUMN_COUNT];
	DWORD m_arrColFormats[MAX_COLUMN_COUNT];
	
	virtual void Initialize();
	void RepositionControls();

	virtual void OnDraw(CDC* pDC) {}
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHeaderItemChanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHeaderItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCancelMode();
	afx_msg void OnEnable(BOOL bEnable);
	//afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg BOOL OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSelchanged(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg BOOL OnSelchanging(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
};