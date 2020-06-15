#if !defined(AFX_QUICKLIST_H__92CE873E_DD8F_11D8_B14D_002018574596__INCLUDED_)
#define AFX_QUICKLIST_H__92CE873E_DD8F_11D8_B14D_002018574596__INCLUDED_

#pragma once

extern WORD wOSVersion;

#include "HeaderCtrlEx.h"
#include "XComboList.h"
/////////////////////////////////////////////////////////////////////////////
// CQuickList window

/*
	CQuickList is an owner draw virtual list. Useful to create
	a complex large list without using a lot of memory.

	Version 1.01 - 22 Jan 2006
	Fixed problem when LVS_EX_HEADERDRAGDROP was used.


	Version 1.0 - 10 Sep 2004
	Solved the "hot item" problem in Windows XP. Added support
	for themes. Added message when user right clicks on the
	column header.


	Version 0.9 - 28 Aug 2004
	First version, stable but with some minor problems.

	Get the latest version at http://www.codeproject.com/listctrl/quicklist.asp

	/PEK


	Credits
	Hans Dietrich, "XListCtrl - A custom-draw list control with subitem formatting"
	http://www.codeproject.com/listctrl/xlistctrl.asp

	Michael Dunn, "Neat Stuff to do in List Controls Using Custom Draw"
	http://www.codeproject.com/listctrl/lvcustomdraw.asp

	Allan Nielsen, "SuperGrid - Yet Another listview control"
	http://www.codeproject.com/listctrl/supergrid.asp

	Lee Nowotny, "Easy Navigation Through an Editable List View"
	http://www.codeproject.com/listctrl/listeditor.asp

	Craig Henderson, "Time to Complete Progress Control"
	http://www.codeproject.com/miscctrl/progresstimetocomplete.asp (Deleted ***DMcK)

  */
  /*
  You could define one or several of the following, if
  you don't use the specific feature:

  #define	QUICKLIST_NOIMAGE			//No image support
  #define	QUICKLIST_NOBUTTON			//No button support
  #define	QUICKLIST_NOTEXTSTYLE		//No text style support (bold, italic, position)
  #define	QUICKLIST_NOEMPTYMESSAGE	//No support for empty message
  #define	QUICKLIST_NOPROGRESSBAR		//No progress bar support
  #define QUICKLIST_NOKEYFIND			//No support for LVN_ODFINDITEM message (but you might implement this by yourself)
  #define QUICKLIST_NONAVIGATION		//No support for column navigation
  #define QUICKLIST_NOEDIT			//No support for editing items
  #define	QUICKLIST_NOCOLORS			//Default colors will be used
  #define	QUICKLIST_NOTOOLTIP			//No support for tool tips.
  */

#define	QUICKLIST_USE_MEMO_BUTTON

#define	QUICKLIST_NOPROGRESSBAR		//No progress bar support
#define	QUICKLIST_NOEMPTYMESSAGE	//No support for empty message
#define QUICKLIST_NOKEYFIND			//No support for LVN_ODFINDITEM message (but you might implement this by yourself)
#define	QUICKLIST_NOIMAGE			//No image support
  //#define	QUICKLIST_NOTOOLTIP			//No support for tool tips.

  //This message is sent then the list needs data
  //WPARAM: Handle to list	LPARAM: Pointer to CListItemData
#define WM_QUICKLIST_GETLISTITEMDATA	(WM_USER + 1979)

//This is sent when navigation column is changing
//WPARAM: Handle to list	LPARAM: Pointer to CNavigationTest
#define WM_QUICKLIST_NAVIGATIONTEST		(WM_USER + 1980)

//This is when user click on the list
//WPARAM: Handle to list	LPARAM: Pointer to CListHitInfo
#define WM_QUICKLIST_CLICK				(WM_USER + 1981)

#define WM_QUICKLIST_DBLCLICK			(WM_USER + 1982)

//This is sent when edit box is losing focus, and when
//edit box doesn't closed automaticly when losing focus
//WPARAM: Handle to edit box LPARAM: 0
#define WM_QUICKLIST_EDITINGLOSTFOCUS	(WM_USER + 1983)

#define WM_QUICKLIST_ENDEDIT	(WM_USER + 1984)
#define WM_QUICKLIST_EDITCHANGE	(WM_USER + 1985)
#define WM_QUICKLIST_BEGINEDIT	(WM_USER + 1986)
#define WM_QUICKLIST_NAVIGATETO	(WM_USER + 1987)
#define WM_QUICKLIST_CONTEXTMENU (WM_USER + 1988)

#ifndef QUICKLIST_NOTOOLTIP
#define WM_QUICKLIST_GETTOOLTIP	(WM_USER + 1989)
struct QUICKLIST_TTINFO {
	QUICKLIST_TTINFO(int row, int col, BOOL bw, LPVOID pvText)
		: nRow(row)
		, nCol(col)
		, bW(bw)
		, pvText80(pvText) {}
	int nRow;
	int nCol;
	BOOL bW;
	LPVOID pvText80;
};
#endif

#define KEYFIND_CURRENTCOLUMN -1
#define KEYFIND_DISABLED	  -2

//Use these color if you want to draw with the default color or transparent
#define DEFAULTCOLOR		CLR_DEFAULT
#define TRANSPARENTCOLOR	CLR_NONE

#ifndef VEC_INT
typedef std::vector<int> VEC_INT;
typedef VEC_INT::iterator it_int;
#endif

#include "QuickEdit.h"

typedef void(*XC_ADJUST_CB)(CWnd *pWnd, int nFld);

class CQuickList : public CListCtrl
{
	// Construction
public:
	CQuickList();
	virtual ~CQuickList();

	//See definitions after this class
	class CListItemData;
	class CNavigationTest;

	class CListHitInfo
	{
	public:
		CListHitInfo() : m_item(-1), m_subitem(-1), m_onImage(false), m_onButton(false) {}
		int m_item;
		int m_subitem;
		bool m_onImage;
		bool m_onButton;
	};

	class CHeaderRightClick;

	// Attributes
public:
	CQuickEdit *m_edit; //Make accessible from dialog

	int m_iGridMargin;
	bool m_bWrap;
	int m_iLastColWidth;

	int ExpandedMemoButtonWidth(int rectheight) { return rectheight + 4; }
	int MemoButtonWidth(int rectheight) { return 2 * (rectheight - 2); }
	int MemoButtonOffset(int rectheight) { return rectheight - 3; }

	void SetAdjustWidthCallback(XC_ADJUST_CB pcb)
	{
		m_pAdjustWidth = pcb;
	}
	bool m_bHdrToolTip;

private:
	static bool m_bUseHeaderEx;
	CHeaderCtrlEx m_HeaderCtrl;
	bool m_bUsingHeaderEx;

	int m_hdrHeight;
	CRect m_hdrToolTipRect;
	CToolTipCtrl m_hdrToolTip;
	//BOOL PtInHeader(const CPoint &point) const;

	int m_lastItem, m_lastCol;

	XC_ADJUST_CB m_pAdjustWidth;

public:
	static void UseHeaderEx()
	{
		m_bUseHeaderEx = true; //static
	}
	void SubclassHeader();
	void InitHeaderStyle();
	void EnableHeaderEx();
	bool IsUsingHeaderEx() { return m_bUsingHeaderEx; } //Vaiable set in PreSubclassWindow()
	void InitFonts(double margin = 1.2);
	void ClearLastKeyedItem() { m_lastItem = m_lastCol = -1; }
	void CellHitTest(const CPoint& pt, int& nRow, int& nCol) const;

	// Overrides
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void PreSubclassWindow();
	//virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetSortArrow(int colIndex, bool ascending);
	void ResizeColumn(int nCol);

	CFont* GetCellFont()
	{
		return m_pCellFont ? m_pCellFont : GetFont();
	}

	//Get which column that is activated. -1 if none
	int GetNavigationColumn() const
	{
#ifndef QUICKLIST_NONAVIGATION
		return m_navigationColumn;
#else
		return -1;
#endif
	}

	//Enable/disable column navigation
	void EnableColumnNavigation(const bool enable)
	{
#ifndef QUICKLIST_NONAVIGATION
		m_enableNavigation = enable;
#else
		UNREFERENCED_PARAMETER(enable);
#endif
	}


	//Is column navigation on?
	bool IsColumnNavigationOn() const
	{
#ifndef QUICKLIST_NONAVIGATION
		return m_enableNavigation;
#else
		return false;
#endif
	}

	void RedrawFocusItem()
	{
		int ifocus = GetItemInFocus();
		RedrawItems(ifocus, ifocus);
	}

	void DrawArrowBox(CQuickList::CListItemData& id, CDC *pDC);
	void SelectComboItem(CListItemData &id, CRect &rect);
	void InitSysColors();
	void StopListBoxEdit(LPCSTR pText);
	bool HasFocus()
	{
		return GetFocus() == this;
	}
	bool IsActive()
	{
		return m_edit || m_pListBox || HasFocus();
	}

	//Get the rect of specific items
	bool GetCheckboxRect(const int item, const int subitem, CRect& rect, bool checkOnly);
	bool GetCheckboxRect(CQuickList::CListItemData& id, CRect& rect, bool checkOnly);

#ifndef QUICKLIST_NOIMAGE
	bool GetImageRect(const int item, const int subitem, CRect& rect, bool imageOnly);
	bool GetImageRect(CQuickList::CListItemData& id, CRect& rect, bool imageOnly);
	//Redraw images in a column
	void RedrawImages(const int topitem, const int bottomitem, const int column, BOOL erase = FALSE);
#endif

	bool GetTextRect(const int item, const int subitem, CRect& rect);
	bool GetTextRect(CQuickList::CListItemData& id, CRect& rect);


	//Set which column the navigation column is
	void SetNavigationColumn(const int column);

	//Set start edit events ---
	void SetEditOnWriting(const bool edit)
	{
		//Edit when user start to write
		m_editOnWriting = edit;
	}

	void SetEditOnEnter(const bool edit)
	{
		//Edit when enter is pressed
		m_editOnEnter = edit;
	}

	void SetEditOnF2(const bool edit)
	{
		//Edit when F2 is pressed
		m_editOnF2 = edit;
	}

	//Edit when edit box is losing focus?
	bool GetEndEditOnLostFocus() const
	{
		return m_editEndOnLostFocus;
	}

	//Return which key was pressed when last editing was finished.
	//Possible values: 0, (no key) VK_RETURN or VK_ESCAPE
	UINT GetLastEndEditKey() const
	{
		return m_editLastEndKey;
	}

	//Get edit box. May be NULL.
	CQuickEdit* GetEditBox() const
	{
		return m_edit;
	}

	int GetHeaderHeight()
	{
		if (m_bUsingHeaderEx) {
			HDITEM hdItem;
			hdItem.mask = HDI_HEIGHT;
			m_HeaderCtrl.GetItem(0, &hdItem);
			return hdItem.cxy + 2;
		}
		else {
			CRect rect;
			GetHeaderCtrl()->GetWindowRect(&rect);
			return rect.Height();
		}
	}

	//Return true if point is on an item. "item" and "subitem" shows which item the point is on.
	//If you want to now if the point is on a check box and/or image use the last two parameters.
	bool HitTest(const POINT& point, int& item, int& subitem, bool* onCheck = NULL, bool* onImage = NULL);
	bool HitTest(const POINT& point, CListHitInfo &info)
	{
		return HitTest(point, info.m_item, info.m_subitem, &info.m_onButton, &info.m_onImage);
	}

	//Set which column to search in when message LVN_ODFINDITEM is recieved
	void SetKeyfindColumn(const int col = KEYFIND_DISABLED)
	{
#ifndef QUICKLIST_NOKEYFIND
		m_keyFindColumn = col;
#else
		UNREFERENCED_PARAMETER(col);
#endif
	}

	//Redraw check boxes
	void RedrawCheckBoxs(const int top, const int bottom, const int column, BOOL erase = FALSE)
	{
		RedrawSubitems(top, bottom, column, 1, erase);
	}

	//Redraw items in a column
	void RedrawSubitems(const int top, const int bottom, const int column, BOOL erase = FALSE)
	{
		RedrawSubitems(top, bottom, column, 0, erase);
	}

	//Redraw text in a column
	void RedrawText(const int top, const int bottom, const int column, BOOL erase = FALSE)
	{
		RedrawSubitems(top, bottom, column, 3, erase);
	}

	//Start/stop editing
	void EditSubItem(int Item, int Column, POINT *pt = NULL);
	void StopEdit(bool cancel = false);

	//Edit box is closing when losing focus. You can change this.
	//If the edit box should close when losing focus, the parent will
	//recieve WM_QUICKLIST_EDITINGLOSTFOCUS when the box is losing focus.
	void SetEndEditOnLostFocus(bool autoend = true)
	{
		m_editEndOnLostFocus = autoend;

		if (m_edit != NULL)
			m_edit->SetEndOnLostFocus(m_editEndOnLostFocus);
	}

	void SetSelected(int i)
	{
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
		SetItemState(i, LVIS_FOCUSED, LVIS_FOCUSED);
	}

	//  This doesn't work (the returned image is instable/transparent): 
	// CImageList* CreateDragImageEx(const int nItem, const int nSubItem);

#ifndef QUICKLIST_NOEMPTYMESSAGE
	//Set the message that will be shown if the list is empty
	void SetEmptyMessage(const CString &message)
	{
		m_emptyMessage = message;
	}
	void SetEmptyMessage(UINT ID)
	{
		CString temp;
		temp.LoadString(ID);

		SetEmptyMessage(temp);
	}
#endif

	//Return true if an item is selected. NOTE: subitems (>0) is never selected if not full row selection is activated.
	bool IsSelected(const int item, const int subitem);

	//Return the only item with the LVNI_FOCUSED state
	int GetItemInFocus()
	{
		return GetNextItem(-1, LVNI_FOCUSED);
	}

	//Make a column visible
	void MakeColumnVisible(int nCol);

	static bool m_bCtrl;

	void SetTooltipDelays(int nTTInitial, int nTTAutopop, int nTTReshow)
	{
		m_nTTInitial = nTTInitial; m_nTTAutopop = nTTAutopop; m_nTTReshow = nTTReshow;
	}

	// Generated message map functions
protected:

	//afx_msg void OnLvnItemChanging(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnHotTrack(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnDestroy();

	afx_msg void OnPaint();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnClickEx(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnDblClickEx(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//afx_msg BOOL OnHeaderBeginResize(UINT id, NMHDR* pNmhdr, LRESULT* pResult);
	afx_msg BOOL OnHeaderBeginDrag(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHeaderEndDrag(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHeaderDividerDblClick(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSysColorChange();
	afx_msg LRESULT OnComboEscape(WPARAM, LPARAM);
	afx_msg LRESULT OnComboLButtonUp(WPARAM, LPARAM);
	afx_msg LRESULT OnTabletQuerySystemGestureStatus(WPARAM, LPARAM);
#ifndef QUICKLIST_NOKEYFIND
	//Is called when user pressing on keys to find an item. 
	afx_msg BOOL OnOdfinditem(NMHDR* pNMHDR, LRESULT* pResult);
	int m_keyFindColumn;
#endif

#ifndef QUICKLIST_NOTOOLTIP
	int m_nTTInitial, m_nTTAutopop, m_nTTReshow;
	//bool m_bToolTipCtrlCustomizeDone;
public:
	afx_msg BOOL OnToolTipText(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
	virtual int OnToolHitTest(CPoint point, TOOLINFO * pTI) const;
protected:
#endif

	void OnEndEdit(const int item, const int subitem, LPCSTR text, UINT endkey);

	friend CQuickEdit;

	bool m_editOnEnter;
	bool m_editOnF2;
	bool m_editOnWriting;
	bool m_editEndOnLostFocus;

	// Maintaining margin
public:
	CFont* m_pGridFont;
	CFont* m_pCellFont;
	double m_Margin;

protected:

	UINT m_editLastEndKey;
	DWORD m_editLastSel;

#ifndef QUICKLIST_NONAVIGATION
	bool m_enableNavigation;
	int m_navigationColumn;
#endif

	//#ifndef QUICKLIST_NOXCOMBOLIST
	COLORREF		m_cr3DFace;
	COLORREF		m_cr3DHighLight;
	COLORREF		m_cr3DShadow;
	COLORREF		m_crBtnFace;
	COLORREF		m_crBtnShadow;
	COLORREF		m_crBtnText;
	COLORREF		m_crGrayText;
	COLORREF		m_crHighLight;
	COLORREF		m_crHighLightText;
	COLORREF		m_crWindow;
	COLORREF		m_crWindowText;

	BOOL			m_bComboIsClicked;
	int				m_nComboItem;
	int				m_nComboSubItem;
	CRect			m_rectComboButton;
	CXComboList *	m_pListBox;
	int				m_nComboInitialIndex;
	//#endif

		//Fill a solid rect, unless transparent color
	static void FillSolidRect(CDC *pDC, const RECT& rect, const COLORREF color)
	{
		if (color != TRANSPARENTCOLOR)
			pDC->FillSolidRect(&rect, color);
	}

	//Try to navigato to a column. Ask parent first
	void TryNavigate(const int newcol);

	//Draw functions
	void DrawItem(int item, int subitem, CDC* pDC);

#ifndef QUICKLIST_NOIMAGE
	void DrawImage(CQuickList::CListItemData& id,
		CDC* pDC);
#endif
	void DrawButtonText(CQuickList::CListItemData& id,
		CRect &rect, CDC* pDC);

	void DrawText(CQuickList::CListItemData& id,
		CDC* pDC);

	void DrawButton(CQuickList::CListItemData& id,
		CDC* pDC);


	//Redraw some of a subitem
	void RedrawSubitems(int topitem, int bottomitem, int column, int part, BOOL erase);

	//Return true if full row select is activated
	bool FullRowSelect()
	{
		return (GetExtendedStyle() & LVS_EX_FULLROWSELECT) != 0;
	}


	//Get the color to use to draw text or background.
	COLORREF GetTextColor(const CQuickList::CListItemData& id, const bool forceNoSelection = false, const bool forceSelection = false);
	COLORREF GetBackColor(const CQuickList::CListItemData& id, const bool forceNoSelection = false);


	//Get the rect where a specific item should be drawn 
	BOOL GetSubItemRect(int iItem, int iSubItem, int nArea, CRect& rect);


#ifndef QUICKLIST_NOEMPTYMESSAGE	
	//The message that is drawn if list is empty
	CString m_emptyMessage;
#endif

	//Returns information about a specific item (calling parent to get information)
	virtual CQuickList::CListItemData& GetItemData(const int item, const int subitem);

	//Used by GetItemData. Don't use this directly
	CListItemData* m_lastget;

	//Returns DT_CENTER, DT_LEFT or DT_RIGHT
	UINT GetTextJustify(const int header);

	//Make sure that top and bottom is inside visible area
	//Return false in everything is outside visible area
	bool MakeInside(int& top, int &bottom);

	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////
//This structure is used to get information about an item.
class CQuickList::CListItemData
{
public:
	CListItemData();

	//Get which item/row
	int CListItemData::GetItem() const
	{
		return m_item;
	}

	//Get which subitem/column
	int CListItemData::GetSubItem() const
	{
		return m_subitem;
	}

	//Is item selected?
	bool CListItemData::IsSelected() const
	{
		return m_isSelected;
	}

	//Is item hot?
	bool CListItemData::IsHot() const
	{
		return m_isHot;
	}

	//The item text
	CString m_text;
	void	*m_pRec;

	//Set this to true if you don't want to draw a selection mark
	//even if this item is selected.
	//Default value: false
	bool m_noSelection;

	//Set this to true if the item is available for editing
	//Default value: false
	char m_cTyp;		//field type
	bool m_allowEdit;
	bool m_editBorder;

#ifndef QUICKLIST_NOTEXTSTYLE 
	//Information about which text style that should be used.
	struct CListTextStyle
	{
		//Default value: false
		bool m_bold;

		//Default value: false
		bool m_italic;

		WORD m_wCharWidth; //used to determine max edit box size
		//Font - nonzero if DC override
		HFONT m_hFont;		//used if not 0
		UINT m_uFldWidth;   //max characters in edit box

		//Default value: 
		// DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS 
		//See CDC:DrawText in MSDN
		UINT m_textPosition;
	} m_textStyle;
#endif

#ifndef QUICKLIST_NOIMAGE
	//Information about the image
	struct CListImage
	{
		//The image position in the image list.
		//-1 if no image.
		//Default value: -1
		int m_imageID;

		//The image list where the image is.
		//Default value: A pointer to the image list in the list
		//control that is used small images (LVSIL_SMALL)
		CImageList* m_imageList;

		//Set true if you don't want to draw selection mark if
		//the item is selection
		//Default value: true
		bool m_noSelection;

		//Center the image. Useful if no text.
		//Default value: false;
		bool m_center;

		//Blend if the image is selected. Use ILD_BLEND25 or 
		//ILD_BLEND50, or 0 if you don't want to use this feature.
		//Default value: ILD_BLEND25
		int m_blend;

	} m_image;
#endif

#ifndef QUICKLIST_NOBUTTON
	//Information about the button
	struct CListButton
	{
		//The style to use to draw the control.
		//Default value: DFCS_BUTTONCHECK
		//Use DFCS_CHECKED to draw the check mark.
		//Use DFCS_BUTTONRADIO for radio button, DFCS_BUTTONPUSH
		//for push button.
		//See CDC::DrawFrameControl for details.
		int m_style;
		LPCSTR m_pText;

		//If you want to draw a button, set this to true
		//Default value: false
		bool m_draw;

		//Center the check box is the column. Useful if no text
		//Default value: true
		bool m_center;
		BYTE m_width; //width of gap on left

		//Set this to true if you don't want to draw selection
		//mark under the control.
		//Default value: true
		bool m_noSelection;

	} m_button;
#endif

#ifndef QUICKLIST_NOCOLORS
	//Information about the colors to use
	struct CListColors
	{
		//Default value for all: DEFAULTCOLOR
		COLORREF m_textColor;
		COLORREF m_backColor;
		COLORREF m_hotTextColor;
		COLORREF m_selectedTextColor;
		COLORREF m_selectedBackColor;
		COLORREF m_selectedBackColorNoFocus;

		//These colors are used to draw selected items 
		//in the "navigation column"
#ifndef QUICKLIST_NONAVIGATION
		COLORREF m_navigatedTextColor;
		COLORREF m_navigatedBackColor;
#endif
	} m_colors;
#endif

	PXCOMBODATA m_pXCMB;

private:
	friend CQuickList;

	void Reset();
	int m_item;
	int m_subitem;
	bool m_isSelected;
	bool m_isHot;
};


class CQuickList::CNavigationTest
{
public:
	int m_previousColumn;
	int m_newColumn;
	bool m_allowChange;
};

class CQuickList::CHeaderRightClick
{
public:
	int m_column;
	CPoint m_mousePos;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUICKLIST_H__92CE873E_DD8F_11D8_B14D_002018574596__INCLUDED_)
