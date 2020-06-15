// MyListCtrl1.cpp : implementation file
//

#include "stdafx.h"
#include "QuickList.h"

#ifdef _USE_THEMES
#include "EnableVisualStyles.h"
#endif

#ifdef QUICKLIST_USE_MEMO_BUTTON
#include "dbtfile.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool CQuickList::m_bUseHeaderEx = false;
bool CQuickList::m_bCtrl = false;

static int iHitTest = 0;

/////////////////////////////////////////////////////////////////////////////
// CQuickList

//Constructor
CQuickList::CListItemData::CListItemData()
{
	Reset();
}

//Reset settings.
void CQuickList::CListItemData::Reset()
{
	m_text.Empty();
	m_item = m_subitem = 0;
	m_isSelected = false;
	m_isHot = false;
	m_noSelection = false;
	m_cTyp = 0;

	m_allowEdit = m_editBorder = false;

	m_pXCMB = NULL;

#ifndef QUICKLIST_NOTEXTSTYLE 
	m_textStyle.m_textPosition = DT_VCENTER | DT_SINGLELINE | DT_LEFT | DT_END_ELLIPSIS;
	m_textStyle.m_bold = false;
	m_textStyle.m_italic = false;
	m_textStyle.m_hFont = NULL;
	m_textStyle.m_uFldWidth = 0;
	m_textStyle.m_wCharWidth = 0;
#endif

#ifndef QUICKLIST_NOIMAGE
	m_image.m_imageID = -1;
	m_image.m_imageList = NULL;
	m_image.m_noSelection = true;
	m_image.m_center = false;
	m_image.m_blend = ILD_BLEND50;
#endif

#ifndef QUICKLIST_NOBUTTON
	m_button.m_pText = NULL;
	m_button.m_draw = false;
	m_button.m_style = DFCS_BUTTONCHECK;
	m_button.m_noSelection = true;
	m_button.m_center = true;
	m_button.m_width = 1;
#endif

#ifndef QUICKLIST_NOCOLORS
	m_colors.m_backColor =
		m_colors.m_selectedBackColor =
		m_colors.m_selectedTextColor =
		m_colors.m_selectedBackColorNoFocus =
		m_colors.m_hotTextColor =
		m_colors.m_textColor = DEFAULTCOLOR;

#ifndef QUICKLIST_NONAVIGATION
	m_colors.m_navigatedTextColor =
		m_colors.m_navigatedBackColor = DEFAULTCOLOR;
#endif
#endif
}

//Constructor
CQuickList::CQuickList() :
	m_edit(NULL)
	, m_pAdjustWidth(NULL)
	, m_bHdrToolTip(false)
	, m_hdrToolTipRect(0, 0, 0, 0)
	, m_Margin(1.0)
	, m_pGridFont(NULL)
	, m_pCellFont(NULL)
	, m_editOnEnter(true)
	, m_editOnF2(true)
	, m_editOnWriting(true)
	, m_editEndOnLostFocus(true)
	, m_bWrap(false)
	, m_bUsingHeaderEx(false)
	, m_iGridMargin(6)
	, m_editLastEndKey(0)
	, m_lastItem(-1)
	, m_lastCol(-1)
	, m_bComboIsClicked(FALSE)
	, m_nComboItem(0)
	, m_nComboSubItem(0)
	, m_pListBox(NULL)
	, m_nComboInitialIndex(-1)
{
#ifndef QUICKLIST_NOKEYFIND
#ifndef QUICKLIST_NONAVIGATION
	m_keyFindColumn = KEYFIND_CURRENTCOLUMN;
#else
	m_keyFindColumn = 0;
#endif
#endif

#ifndef QUICKLIST_NONAVIGATION
	m_enableNavigation = true;
	m_navigationColumn = 0;
#endif


#ifndef QUICKLIST_NOTOOLTIP
	//m_bToolTipCtrlCustomizeDone=false;
	m_nTTInitial = 64;
	m_nTTAutopop = 3200;
	m_nTTReshow = 32; //for thumblist
#endif

	m_lastget = new CListItemData;

	InitSysColors();
}

//Destructor
CQuickList::~CQuickList()
{
	delete m_lastget;
	if (m_pCellFont) {
		VERIFY(m_pCellFont->DeleteObject());
		delete m_pCellFont; //OK!
	}
	if (m_pGridFont) {
		VERIFY(m_pGridFont->DeleteObject());
		delete m_pGridFont; //dmck - not OK! illegal mem access in destructor!
	}
}

BEGIN_MESSAGE_MAP(CQuickList, CListCtrl)
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_WM_KILLFOCUS()
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
	ON_WM_CHAR()

	//ON_NOTIFY_REFLECT(LVN_ITEMCHANGING, OnLvnItemChanging) 
	ON_NOTIFY_REFLECT(LVN_HOTTRACK, OnLvnHotTrack)
	ON_NOTIFY_REFLECT_EX(NM_CLICK, OnClickEx)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, OnDblClickEx)

	ON_NOTIFY_EX(HDN_DIVIDERDBLCLICKA, 0, OnHeaderDividerDblClick)
	ON_NOTIFY_EX(HDN_DIVIDERDBLCLICKW, 0, OnHeaderDividerDblClick)
	//ON_NOTIFY_EX(HDN_BEGINTRACKA, 0, OnHeaderBeginResize)
	//ON_NOTIFY_EX(HDN_BEGINTRACKW, 0, OnHeaderBeginResize)
	ON_NOTIFY_EX(HDN_BEGINDRAG, 0, OnHeaderBeginDrag)
	ON_NOTIFY_EX(HDN_ENDDRAG, 0, OnHeaderEndDrag)

#ifndef QUICKLIST_NOKEYFIND
	ON_NOTIFY_REFLECT_EX(LVN_ODFINDITEM, OnOdfinditem)
#endif

#ifndef QUICKLIST_NOTOOLTIP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
#endif

	ON_REGISTERED_MESSAGE(WM_XCOMBOLIST_VK_ESCAPE, OnComboEscape)
	ON_REGISTERED_MESSAGE(WM_XCOMBOLIST_LBUTTONUP, OnComboLButtonUp)
	ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, OnTabletQuerySystemGestureStatus)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQuickList message handlers

//Custom draw
void CQuickList::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	//  *pResult = 0;	return;

	  // If this is the beginning of the control's paint cycle, request
	  // notifications for each item.

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		// This is the pre-paint stage for an item.  We need to make another
		// request to be notified during the post-paint stage.
		*pResult = CDRF_NOTIFYSUBITEMDRAW;;
	}

	else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage)
	{
		CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);

		DrawItem(static_cast<int> (pLVCD->nmcd.dwItemSpec),
			pLVCD->iSubItem, pDC);
		*pResult = CDRF_SKIPDEFAULT;	// We've painted everything.
	}
}

/*
	About GetTextColor and GetBackColor:
		Here you see why you shouldn't try to optimze your code :-)
  */

#ifndef COLOR_HOTLIGHT
#define COLOR_HOTLIGHT 26
#endif

  //Get which text color to use
COLORREF CQuickList::GetTextColor(const CQuickList::CListItemData& id, const bool forceNoSelection, const bool forceSelection)
{
	bool navigated = false;
#ifndef QUICKLIST_NONAVIGATION
	if (id.GetSubItem() == m_navigationColumn && m_enableNavigation)
		navigated = true;
#endif

	if ((!forceNoSelection && id.IsSelected()) //IsSelected(id.m_item, id.m_subitem)) 
		|| forceSelection)
	{
#ifndef QUICKLIST_NOCOLORS
#ifndef QUICKLIST_NONAVIGATION
		if (navigated && GetFocus() == this)
		{
			if (id.m_colors.m_navigatedTextColor != DEFAULTCOLOR)
				return id.m_colors.m_navigatedTextColor;
			else
				return ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		}
#endif

		if (id.m_colors.m_selectedTextColor != DEFAULTCOLOR)
			return id.m_colors.m_selectedTextColor;
		else
		{
			if (GetFocus() == this)
				return ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			else
				return ::GetSysColor(COLOR_WINDOWTEXT);
		}
#else

		if (GetFocus() == this)
			return ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		else
			return ::GetSysColor(COLOR_WINDOWTEXT);

#endif
	}

	//Is item hot?
	if (GetHotItem() == id.GetItem())
	{
#ifndef QUICKLIST_NOCOLORS
		if (id.m_colors.m_hotTextColor != DEFAULTCOLOR)
			return id.m_colors.m_hotTextColor;
		else
#endif
			return ::GetSysColor(COLOR_HOTLIGHT);
	}
	else
	{
#ifndef QUICKLIST_NOCOLORS
		if (id.m_colors.m_textColor != DEFAULTCOLOR)
			return id.m_colors.m_textColor;
		else
#endif
			return ::GetSysColor(COLOR_WINDOWTEXT);
	}


}

//Get which background color to use
COLORREF CQuickList::GetBackColor(const CQuickList::CListItemData& id, const bool forceNoSelection)
{
	bool navigated = false;
#ifndef QUICKLIST_NONAVIGATION
	if (id.GetSubItem() == m_navigationColumn && m_enableNavigation)
		navigated = true;
#endif

	bool hasFocus = (GetFocus() == this);

	//Is selected
	if (!forceNoSelection && id.IsSelected()) //IsSelected(id.m_item, id.m_subitem))
	{
		//Return selected color
#ifndef QUICKLIST_NOCOLORS
#ifndef QUICKLIST_NONAVIGATION
		if (navigated && GetFocus() == this)
		{
			if (id.m_colors.m_navigatedBackColor != DEFAULTCOLOR)
				return id.m_colors.m_navigatedBackColor;
			else
				return ::GetSysColor(COLOR_HOTLIGHT);
		}
#endif

		if ((hasFocus && id.m_colors.m_selectedBackColor != DEFAULTCOLOR) ||
			(!hasFocus && id.m_colors.m_selectedBackColorNoFocus != DEFAULTCOLOR))
		{
			if (hasFocus)
				return id.m_colors.m_selectedBackColor;
			else
				return id.m_colors.m_selectedBackColorNoFocus;
		}
		else
		{
			if (!IsColumnNavigationOn() && id.IsHot())
				return ::GetSysColor(COLOR_HOTLIGHT);

			if (hasFocus)
				return ::GetSysColor(COLOR_HIGHLIGHT);
			else
				return ::GetSysColor(COLOR_BTNFACE);
		}
#else
	//Simple window colors
		if (navigated && GetFocus() == this)
			return ::GetSysColor(COLOR_HOTLIGHT);
		else if (!IsColumnNavigationOn() && id.IsHot())
			return ::GetSysColor(COLOR_HOTLIGHT);
		else
		{
			if (GetFocus() == this)
				return ::GetSysColor(COLOR_HIGHLIGHT);
			else
				return ::GetSysColor(COLOR_BTNFACE);
		}
#endif

	}

#ifndef QUICKLIST_NOCOLORS
	if (id.m_colors.m_backColor != DEFAULTCOLOR)
		return id.m_colors.m_backColor;
	else
#endif
		return ::GetSysColor(COLOR_WINDOW);
}

//Is item selected?
bool CQuickList::IsSelected(const int item, const int subitem)
{
	if (GetItemState(item, LVIS_SELECTED))
	{
		if (subitem == 0 ||	//First column or...	 
			FullRowSelect()	//full row select?
			)
		{
			// has focus?
			if (m_hWnd != ::GetFocus())
			{
				if (GetStyle() & LVS_SHOWSELALWAYS)
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			return true;
		}
	}

	return false;
}

//On painting
void CQuickList::OnPaint()
{
	Default();

#ifndef QUICKLIST_NOEMPTYMESSAGE	
	//Draw empty list message
	if (GetItemCount() <= 0 && !m_emptyMessage.IsEmpty())
	{
		CDC* pDC = GetDC();
		int nSavedDC = pDC->SaveDC();

		CRect rc;
		GetWindowRect(&rc);
		ScreenToClient(&rc);
		CHeaderCtrl* pHC = GetHeaderCtrl();
		if (pHC != NULL)
		{
			CRect rcH;
			pHC->GetItemRect(0, &rcH);
			rc.top += rcH.bottom;
		}
		rc.top += 10;

		COLORREF crText = CListCtrl::GetTextColor();

		pDC->SetTextColor(crText);
		int oldMode = pDC->SetBkMode(TRANSPARENT);

		pDC->SelectStockObject(ANSI_VAR_FONT);
		pDC->DrawText(m_emptyMessage, -1, rc, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);
		pDC->SetBkMode(oldMode);
		pDC->RestoreDC(nSavedDC);
		ReleaseDC(pDC);
	}
#endif


}

//Get rect of subitem.
BOOL CQuickList::GetSubItemRect(int nItem,
	int nSubItem,
	int nArea,
	CRect& rect)
{
	ASSERT(nItem >= 0);
	ASSERT(nItem < GetItemCount());
	if ((nItem < 0) || nItem >= GetItemCount())
		return FALSE;
	ASSERT(nSubItem >= 0);
	ASSERT(nSubItem < GetHeaderCtrl()->GetItemCount());
	if ((nSubItem < 0) || nSubItem >= GetHeaderCtrl()->GetItemCount())
		return FALSE;

	BOOL bRC = CListCtrl::GetSubItemRect(nItem, nSubItem, nArea, rect);

	// if nSubItem == 0, the rect returned by CListCtrl::GetSubItemRect
	// is the entire row. To solve this we use the left and right position
	// of the column header, which should be the same.
	if (nSubItem == 0)
	{
		//If the horisontal scroll is used, we need to handle the offset.
		int offset = rect.left;

		CRect firstColumnRect;
		GetHeaderCtrl()->GetItemRect(0, &firstColumnRect);
		rect.left = firstColumnRect.left + offset;
		rect.right = firstColumnRect.right + offset;
	}

	return bRC;
}

//Get text rect
//Return false if none
bool CQuickList::GetTextRect(const int item, const int subitem, CRect& rect)
{
	CQuickList::CListItemData& id = GetItemData(item, subitem);

	return GetTextRect(id, rect);
}

//Get text rect
//Return false if none
bool CQuickList::GetTextRect(CQuickList::CListItemData& id, CRect& rect)
{
	if (!id.m_textStyle.m_uFldWidth && id.m_button.m_draw) {
		//Table memo button or checkbox --
		if (!id.m_button.m_pText) return false;
		//prefix shown in table --
	}

	GetSubItemRect(id.GetItem(), id.GetSubItem(), LVIR_BOUNDS, rect);

	if (id.m_button.m_draw) return true;

	if (!id.m_editBorder && id.m_textStyle.m_uFldWidth) {
		int width = id.m_textStyle.m_uFldWidth*id.m_textStyle.m_wCharWidth + 2 * m_iGridMargin;
		if (rect.Width() > width) rect.right = rect.left + width;
	}

#ifndef QUICKLIST_NOIMAGE
	{
		//Move if image
		CRect temp;
		if (GetImageRect(id, temp, false))
			rect.left = temp.right;
	}
}

#endif
return true;
}

//Get check box rect
//Return false if none
bool CQuickList::GetCheckboxRect(const int item, const int subitem, CRect& rect, bool checkOnly)
{
	CQuickList::CListItemData& id = GetItemData(item, subitem);

	return id.m_button.m_draw && GetCheckboxRect(id, rect, checkOnly);
}

//Get check box rect
//Return false if none
bool CQuickList::GetCheckboxRect(CQuickList::CListItemData& id, CRect& rect, bool checkOnly)
{
#ifdef QUICKLIST_NOBUTTON
	UNREFERENCED_PARAMETER(id);
	UNREFERENCED_PARAMETER(rect);

	return false;
#else
	ASSERT(id.m_button.m_draw);

	//if(!id.m_button.m_draw) return false;

	GetSubItemRect(id.GetItem(), id.GetSubItem(), LVIR_BOUNDS, rect);

	int height = rect.Height();

	if (id.m_button.m_style&DFCS_BUTTONPUSH) {
		if (!checkOnly) return true;

		ASSERT(id.m_button.m_width > 1);

		int width = MemoButtonWidth(height);

		if (!id.m_button.m_center)
		{
			if (id.m_button.m_noSelection) {
				//form view --
				rect.left += MemoButtonOffset(height); //(height-3)
				rect.right = rect.left + width;
			}
			else {
				//table view -- buttons on left
				width = ExpandedMemoButtonWidth(height);
				if (rect.Width() <= width + 6) return false;
				rect.left += 6;
				rect.right = rect.left + width;
				rect.InflateRect(-1, -1);
			}
		}
		else {
			// table view -- buttons in center
			int x = rect.CenterPoint().x - width / 2;
			if (x < rect.left) {
				return false;
			}
			rect.left = x;
			rect.right = x + width;
			rect.InflateRect(-1, -1);
		}
	}
	else {
		//checkbox --
		if (id.m_button.m_center)
		{
			if (!checkOnly) return true;
			//Table view --
			CRect full(rect);
			rect.left = full.CenterPoint().x - (height) / 2;
			rect.right = full.CenterPoint().x + (height + 1) / 2;
			rect.InflateRect(-1, -1);
		}
		else {
			if (!checkOnly) {
				rect.right = rect.left + MemoButtonOffset(height) + height;
				return true;
			}
			rect.left += MemoButtonOffset(height);
			rect.right = rect.left + height;	// width = height
		}

	}
	return true;
#endif
}


#ifndef QUICKLIST_NOIMAGE
//Get image rect
//Return false if none
bool CQuickList::GetImageRect(const int item, const int subitem, CRect& rect, bool imageOnly)
{
	CQuickList::CListItemData& id = GetItemData(item, subitem);

	return GetImageRect(id, rect, imageOnly);
}
//Get image rect
//Return false if none
bool CQuickList::GetImageRect(CQuickList::CListItemData& id, CRect& rect, bool imageOnly)
{
	if (id.m_image.m_imageID == -1)
		return false;

	GetSubItemRect(id.GetItem(), id.GetSubItem(), LVIR_BOUNDS, rect);

	//If there is an image box we has to move the rect
	int leftMarging = 1;
	{
		CRect chk;
		if (GetCheckboxRect(id, chk, false))
		{
			leftMarging = 0;
			rect.left = chk.right;
		}
	}

	//Draw image?
	if (id.m_image.m_imageList)
	{
		int nImage = id.m_image.m_imageID;

		if (nImage < 0)
			return false;

		SIZE sizeImage;
		sizeImage.cx = sizeImage.cy = 0;
		IMAGEINFO info;

		if (rect.Width() > 0 && id.m_image.m_imageList->GetImageInfo(nImage, &info))
		{
			//if imageOnly we only return the rect of the image.
			//otherwise we also include area under and below image.
			sizeImage.cx = info.rcImage.right - info.rcImage.left;
			sizeImage.cy = info.rcImage.bottom - info.rcImage.top;

			//Real size
			SIZE size;
			size.cx = rect.Width() < sizeImage.cx ? rect.Width() : sizeImage.cx;
			size.cy = 1 + rect.Height() < sizeImage.cy ? rect.Height() : sizeImage.cy;

			POINT point;
			point.y = rect.CenterPoint().y - (sizeImage.cy / 2);
			//Center image?
			if (id.m_image.m_center)
				point.x = rect.CenterPoint().x - (sizeImage.cx / 2);
			else
				point.x = rect.left;

			point.x += leftMarging;

			//Out of area?
			if (point.y < rect.top)
				point.y = rect.top;

			//
			if (!imageOnly)
			{
				rect.right = point.x + sizeImage.cx;
			}
			else
				//Make rect of point and size, and return this
				rect = CRect(point, size);

			return true;
		}
	}

	return false;
}
#endif

//Draw an item in pDC
void CQuickList::DrawItem(int item, int subitem, CDC* pDC)
{
	if (iHitTest > 0) return;
	CListItemData id = GetItemData(item, subitem);

	//Draw button?
#ifndef QUICKLIST_NOBUTTON
	if (id.m_button.m_draw) {
		ASSERT(!id.m_pXCMB);
		DrawButton(id, pDC);
	}
#endif

	//Draw image?
#ifndef QUICKLIST_NOIMAGE
	if (id.m_image.m_imageID != -1)
		DrawImage(id, pDC);
#endif

	//if(id.m_textStyle.m_uFldWidth)
	DrawText(id, pDC);

	if (id.m_pXCMB && (bIgnoreEditTS || !(id.m_pXCMB->wFlgs&XC_LISTNOEDIT)))
	{
		DrawArrowBox(id, pDC);
	}
}

#ifndef QUICKLIST_NOIMAGE
//Draw image
void CQuickList::DrawImage(CQuickList::CListItemData& id,
	CDC* pDC)
{
	CRect imgRect;

	if (GetImageRect(id, imgRect, false))
	{
		CImageList* pImageList = id.m_image.m_imageList;

		COLORREF blendColor = GetBackColor(id);

		// save image list background color
		COLORREF rgb = pImageList->GetBkColor();

		// set image list background color
		pImageList->SetBkColor(GetBackColor(id, id.m_image.m_noSelection || id.m_noSelection));

		//Fill background
		FillSolidRect(pDC,
			imgRect,
			GetBackColor(id, id.m_image.m_noSelection || id.m_noSelection)
		);

		GetImageRect(id, imgRect, true);

		if (id.m_noSelection ||
			!id.IsSelected() ||
			id.m_image.m_blend == 0 ||
			blendColor == TRANSPARENTCOLOR)
		{
			pImageList->DrawIndirect(pDC,
				id.m_image.m_imageID,
				imgRect.TopLeft(),
				imgRect.Size(),
				CPoint(0, 0));
		}
		else
		{
			pImageList->DrawIndirect(pDC,
				id.m_image.m_imageID,
				imgRect.TopLeft(),
				imgRect.Size(),
				CPoint(0, 0),
				ILD_NORMAL | id.m_image.m_blend,
				SRCCOPY,
				CLR_DEFAULT,
				blendColor);
		}
		pImageList->SetBkColor(rgb);
	}

	return;

}
#endif

void CQuickList::DrawButtonText(CQuickList::CListItemData& id, CRect &rect, CDC* pDC)
{
	CFont *pOldFont = pDC->SelectObject(GetCellFont());
	rect.left += ExpandedMemoButtonWidth(rect.Height()) + m_iGridMargin + 4;
	if (rect.right - rect.left <= 10) return;
	pDC->SetTextColor(RGB(128, 128, 128));
	pDC->SetBkMode(TRANSPARENT);
	pDC->DrawText(id.m_button.m_pText, &rect, DT_VCENTER | DT_SINGLELINE | DT_LEFT | DT_END_ELLIPSIS);
	pDC->SelectObject(pOldFont);
}

//Draw text
void CQuickList::DrawText(CQuickList::CListItemData& id, CDC* pDC)
{
	ASSERT(pDC);

	CRect rect;
	if (!GetTextRect(id, rect)) return;

	int fullRight = rect.right;

	if (id.m_button.m_draw && (id.m_button.m_style&DFCS_BUTTONPUSH)) {
		if (!id.m_button.m_pText) {
			if (id.m_textStyle.m_uFldWidth == 1) {
				rect.left += (MemoButtonOffset(rect.Height()) + MemoButtonWidth(rect.Height()));
				FillSolidRect(pDC, rect, ::GetSysColor(COLOR_BTNFACE));
			}
			return;
		}

		if (!id.m_textStyle.m_uFldWidth) {
			//table button, text on right --
			DrawButtonText(id, rect, pDC);
			return;
		}

		rect.right = rect.left + MemoButtonOffset(rect.Height());
	}

	if (id.m_text.IsEmpty())
	{
		//Just clean --
		FillSolidRect(pDC,
			rect,
			GetBackColor(id, !FullRowSelect() || id.m_noSelection));
	}
	else
	{
		UINT textFormat;

#ifdef QUICKLIST_NOTEXTSTYLE 
		textFormat = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | GetTextJustify(id.GetSubItem());
#else
		CFont *pOldFont = NULL;

		textFormat = id.m_textStyle.m_textPosition;

		CFont *font = GetCellFont();

		if (id.m_textStyle.m_hFont) {
			pOldFont = pDC->SelectObject(CFont::FromHandle(id.m_textStyle.m_hFont));
		}
		// check if bold specified for subitem
		else {
			pOldFont = pDC->SelectObject(font);
		}
#endif
		{

			pDC->SetTextColor(GetTextColor(id, id.m_noSelection));

			COLORREF backColor = GetBackColor(id, id.m_noSelection);

			FillSolidRect(pDC,
				rect,
				GetBackColor(id, !FullRowSelect() || id.m_noSelection)
			);

			if (backColor == TRANSPARENTCOLOR)
				pDC->SetBkMode(TRANSPARENT);
			else
			{
				pDC->SetBkMode(OPAQUE);
				pDC->SetBkColor(backColor);
			}

			CString text = id.m_text;
			//If we don't do this, character after & will be underlined.
			text.Replace(_T("&"), _T("&&"));

			if (textFormat&DT_RIGHT) {
				ASSERT(!id.m_button.m_draw);
				rect.right -= m_iGridMargin;
			}
			else if (!(textFormat&DT_CENTER)) {
				rect.left += m_iGridMargin;
			}

			pDC->DrawText(text, &rect, textFormat);

#ifdef QUICKLIST_USE_MEMO_BUTTON
			if (id.m_button.m_draw && id.m_button.m_pText && id.m_textStyle.m_uFldWidth == 1) {
				text = id.m_button.m_pText;
				text.Replace(_T("&"), _T("&&"));
				rect.left = rect.right + MemoButtonWidth(rect.Height());
				rect.right = fullRight;
				FillSolidRect(pDC, rect, ::GetSysColor(COLOR_BTNFACE));
				rect.right = rect.left + pDC->GetTextExtent("A").cx*LEN_MEMO_HDR;
				pDC->SetTextColor(RGB(128, 128, 128));
				pDC->SetBkMode(TRANSPARENT);
				rect.left += m_iGridMargin;
				pDC->DrawText(text, &rect, textFormat);
			}
#endif

		}
#ifndef QUICKLIST_NOTEXTSTYLE 
		if (pOldFont)
			pDC->SelectObject(pOldFont);
#endif
	}
}


#ifndef QUICKLIST_NOBUTTON
//Draw button (check box/radio button)
void CQuickList::DrawButton(CQuickList::CListItemData& id,
	CDC* pDC)
{
	ASSERT(pDC);

	CRect chkboxrect;

	GetCheckboxRect(id, chkboxrect, false);

	//Clear background
	FillSolidRect(pDC,
		chkboxrect,
		GetBackColor(id, id.m_button.m_noSelection || id.m_noSelection)
	);

	if (!GetCheckboxRect(id, chkboxrect, true)) return;

	pDC->DrawFrameControl(&chkboxrect, DFC_BUTTON, id.m_button.m_style);

	if ((id.m_button.m_style&(DFCS_INACTIVE | DFCS_BUTTONPUSH)) == DFCS_BUTTONPUSH) {
		UINT uAlign = pDC->SetTextAlign(TA_CENTER | TA_BASELINE);
		int iMode = pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(::GetSysColor(COLOR_BTNSHADOW));
		pDC->TextOut(chkboxrect.CenterPoint().x, chkboxrect.CenterPoint().y + 1, "...", 3);
		pDC->SetBkMode(iMode);
		pDC->SetTextAlign(uAlign);
	}
}
#endif

//Called when the list needs information about items to draw them
//The reason this function is implemented is that it help the list a little
//bit to calculate item width more correctly
void CQuickList::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

	//Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	//Which item number?
	int itemid = pItem->iItem;

	//Do the list need text information?
	if ((pItem->mask & LVIF_IMAGE) || (pItem->mask & LVIF_TEXT))
	{
		CQuickList::CListItemData &id = GetItemData(itemid, pItem->iSubItem);

		if ((pItem->mask & LVIF_TEXT))
		{
			//Copy the text to the LV_ITEM structure
			//Maximum number of characters is in pItem->cchTextMax
			lstrcpyn(pItem->pszText, id.m_text, pItem->cchTextMax);
		}

		//Do the list need image information?	
		if (pItem->mask & LVIF_IMAGE)
		{
#ifndef QUICKLIST_NOIMAGE
			//Set which image to use
			pItem->iImage = id.m_image.m_imageID;
#endif

			//Show check box?
#ifndef QUICKLIST_NOBUTTON
			if (id.m_button.m_draw)
			{
				//To enable check box, we have to enable state mask...
				pItem->mask |= LVIF_STATE;
				pItem->stateMask = LVIS_STATEIMAGEMASK;

				if ((id.m_button.m_style & DFCS_CHECKED) != 0)
				{
					//Turn check box on..
					pItem->state = INDEXTOSTATEIMAGEMASK(2);
				}
				else
				{
					//Turn check box off
					pItem->state = INDEXTOSTATEIMAGEMASK(1);
				}
			}
#endif
		}
	}

	*pResult = 0;
}

//Get information about an item
CQuickList::CListItemData& CQuickList::GetItemData(const int item, const int subitem)
{

	ASSERT(item >= 0);
	ASSERT(item < GetItemCount());
	ASSERT(subitem >= 0);

	//Static, so we doesn't new to create a new every time
	CListItemData& id = *m_lastget;
	id.Reset();

	id.m_item = item;
	id.m_subitem = subitem;
	id.m_isSelected = IsSelected(item, subitem); //GetItemState(item, LVIS_SELECTED)!=0;
	id.m_isHot = (GetHotItem() == id.m_item);

#ifndef QUICKLIST_NOIMAGE
	id.m_image.m_imageList = GetImageList(LVSIL_SMALL);

	if (!FullRowSelect())
		id.m_image.m_noSelection = true;
#endif

#ifndef QUICKLIST_NOTEXTSTYLE 
	//Same align as in column
	id.m_textStyle.m_textPosition = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | GetTextJustify(subitem);
#endif


	::SendMessage(GetParent()->GetSafeHwnd(),
		WM_QUICKLIST_GETLISTITEMDATA,
		(WPARAM)GetSafeHwnd(),
		(LPARAM)&id);

	return id;
}

//Return wich alignment an column has (DT_LEFT, DT_CENTER or DT_RIGHT).
UINT CQuickList::GetTextJustify(const int headerpos)
{
	HDITEM hditem;
	hditem.mask = HDI_FORMAT;
	GetHeaderCtrl()->GetItem(headerpos, &hditem);

	int nFmt = hditem.fmt & HDF_JUSTIFYMASK;

	if (nFmt == HDF_CENTER)
		return  DT_CENTER;
	else if (nFmt == HDF_LEFT)
		return  DT_LEFT;
	else
		return  DT_RIGHT;
}

#ifndef QUICKLIST_NOKEYFIND
//Called when user writes in the list to find an item
BOOL CQuickList::OnOdfinditem(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (m_keyFindColumn == KEYFIND_DISABLED)
	{
		//When we return FALSE this message will be handled 
		//by the parent. Just the way we want it!
		return FALSE;
	}

	int column = m_keyFindColumn;

#ifndef QUICKLIST_NONAVIGATION
	//Use current column?
	if (column == KEYFIND_CURRENTCOLUMN)
		column = GetNavigationColumn();
#endif

	ASSERT(column >= 0);

	NMLVFINDITEM* pFindInfo = (NMLVFINDITEM*)pNMHDR;

	// Set the default return value to -1
	// That means we didn't find any match.
	*pResult = -1;

	//This is the string we search for
	CString searchstr = pFindInfo->lvfi.psz;

	//	TRACE(_T("Find: %s\n"), searchstr);

	int startPos = pFindInfo->iStart;
	//Is startPos outside the list (happens if last item is selected)
	if (startPos >= GetItemCount())
		startPos = 0;

	int currentPos = startPos;

	//Let's search...
	do
	{
		//Do this word begins with all characters in searchstr?
		if (_tcsnicmp(GetItemData(currentPos, column).m_text,
			searchstr,
			searchstr.GetLength()
		) == 0)
		{
			//Select this item and stop search.
			*pResult = currentPos;
			break;
		}

		//Go to next item
		currentPos++;

		//Need to restart at top?
		if (currentPos >= GetItemCount())
			currentPos = 0;

		//Stop if back to start
	} while (currentPos != startPos);

	return TRUE;

}
#endif

//Test where a point is on an item
bool CQuickList::HitTest(const POINT& point, int& item, int& subitem, bool* onCheck, bool* onImage)
{
	LVHITTESTINFO test;
	test.pt = point;
	test.flags = 0;
	test.iItem = test.iSubItem = -1;

	iHitTest = 1;
	iHitTest = ListView_SubItemHitTest(m_hWnd, &test);
	if (iHitTest == -1) return false;
	iHitTest = 0;

	item = test.iItem;
	subitem = test.iSubItem;

#ifndef QUICKLIST_NOBUTTON
	//Make check box hit test?
	if (onCheck != NULL)
	{
		*onCheck = false;
		CListItemData& id = GetItemData(test.iItem, test.iSubItem);

		//Has active check box?
		if (id.m_button.m_draw && !(id.m_button.m_style&DFCS_INACTIVE)) {
			CRect checkrect;
			if (GetCheckboxRect(id, checkrect, true) && checkrect.PtInRect(point))
				*onCheck = true;
		}
	}
#endif

#ifndef QUICKLIST_NOIMAGE
	//Make image hit test?
	if (onImage != NULL)
	{
		*onImage = false;
		CRect imgrect;

		//Has image box?
		if (GetImageRect(test.iItem, test.iSubItem, imgrect, true))
		{
			if (imgrect.PtInRect(point)) *onImage = true;
		}
	}
#endif

	return true;
}

#ifndef QUICKLIST_NOIMAGE
//Redraw images
void CQuickList::RedrawImages(const int top, const int bottom, const int column, BOOL erase)
{
	RedrawSubitems(top, bottom, column, 2, erase);
}
#endif

//Redraw some of a subitem
void CQuickList::RedrawSubitems(int top, int bottom, int column, int part, BOOL erase)
{
	if (GetItemCount() <= 0)
		return;

	if (!MakeInside(top, bottom))
		//No need to redraw items.
		return;

	CRect t, b;

	if (part == 0) //Redraw subitem completely
	{
		GetSubItemRect(top, column, LVIR_BOUNDS, t);
		GetSubItemRect(bottom, column, LVIR_BOUNDS, b);
	}
	else if (part == 1)//Redraw check boxs
	{
		if (!GetCheckboxRect(top, column, t, true)) return;
		GetCheckboxRect(bottom, column, b, true);
	}
	else if (part == 2)//Redraw images
	{
#ifndef QUICKLIST_NOIMAGE
		GetImageRect(top, column, t, true);
		GetImageRect(bottom, column, b, true);
#endif
	}
	else //Redraw text
	{
		GetTextRect(top, column, t);
		GetTextRect(bottom, column, b);
	}

	//Mix them
	t.UnionRect(&t, &b);

	InvalidateRect(&t, erase);

}

//Make items inside visible area
bool CQuickList::MakeInside(int& top, int &bottom)
{
	int min = GetTopIndex(), max = min + GetCountPerPage();

	if (max >= GetItemCount())
		max = GetItemCount() - 1;

	//Is both outside visible area?
	if (top < min && bottom < min)
		return false;
	if (top > max && bottom > max)
		return false;

	if (top < min)
		top = min;

	if (bottom > max)
		bottom = max;

	return true;
}

//Start editing an sub item
void CQuickList::EditSubItem(int Item, int Column, POINT *pt)
{
	// Make sure that nCol is valid
	{
		//CHeaderCtrl* pHeader = (CHeaderCtrl*) GetDlgItem(0);
		CHeaderCtrl* pHeader = GetHeaderCtrl();
		ASSERT(pHeader == (CHeaderCtrl*)GetDlgItem(0));
		int nColumnCount = pHeader->GetItemCount();
		if (!Column || Column >= nColumnCount || GetColumnWidth(Column) < 5)
			return;
	}

	CListItemData id = GetItemData(Item, Column); //can't use reference here due to Create!

	if (id.m_button.m_draw && !(id.m_button.m_style&DFCS_BUTTONPUSH))
		return; //handles double-click outside of checkbox

	//Prompt for editor's name and/or open memo --
	if (id.m_allowEdit) {
		LV_DISPINFO dispinfo;
		dispinfo.hdr.hwndFrom = m_hWnd;
		dispinfo.hdr.idFrom = GetDlgCtrlID();
		dispinfo.hdr.code = LVN_BEGINLABELEDIT;
		dispinfo.item.iItem = Item;
		dispinfo.item.iSubItem = Column;
		if (::SendMessage(GetParent()->GetSafeHwnd(), WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)&dispinfo))
			return;
	}

	if (id.m_button.m_draw) {
		return;
	}

	CRect Rect;
	//Same as GetItemRect (Item, &Rect, LVIR_BOUNDS) when id.m_editBorder
	GetTextRect(id, Rect);

	// Now scroll if we need to expose the column
	CRect ClientRect;
	GetClientRect(&ClientRect);
	if (Rect.right > ClientRect.right) {
		Rect.right = ClientRect.right;
		if (Rect.Width() < 40) return;
	}

	if (id.m_pXCMB && id.m_allowEdit) {
		if (!bIgnoreEditTS && (id.m_pXCMB->wFlgs&XC_LISTNOEDIT) || !pt || pt->x > Rect.right - Rect.Height()) {
			SelectComboItem(id, Rect); //dropdown list edit
			return;
		}
		if (pt->x == INT_MIN) pt = NULL; //return key
		Rect.right -= Rect.Height() - 3;
	}

	HFONT hFont = id.m_textStyle.m_hFont;
	if (!hFont) hFont = *GetCellFont();

	UINT cx, cy;
	if (id.m_editBorder) {
		cx = 4; cy = 0;
	}
	else {
		Rect.bottom--;
		Rect.left += m_iGridMargin;
	}

	DWORD style = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
	if (!id.m_allowEdit) {
		style |= ES_READONLY;
	}

	if (id.m_editBorder) style |= WS_BORDER;
	{
		UINT justify = GetTextJustify(Column);
		if (justify == DT_LEFT) style |= ES_LEFT;
		else if (justify == DT_CENTER) {
			style |= ES_CENTER;
		}
		else {
			//ASSERT(style & WS_BORDER);
			style |= DT_RIGHT;
			cx = 0;
			cy = 4;
			if (wOSVersion < 0x0600) cy--;
		}
	}

	if (m_edit != NULL)
	{
		TRACE(_T("Warning: Edit box was visible when an item was started to edit.\n"));
		StopEdit();
	}

	m_edit = new CQuickEdit(this, Item, Column, hFont, id.m_text, id.m_cTyp, m_editEndOnLostFocus);

	VERIFY(m_edit->Create(style, Rect, this, 1001));

	if (id.m_editBorder) {
		m_edit->SetMargins(cx, cy);
	}

	m_edit->LimitText(id.m_textStyle.m_uFldWidth);

	if (!id.m_cTyp) m_edit->SetSel(0, -1);
	else {
		if (pt && Rect.left < pt->x) {
			int iStart = LOWORD(m_edit->CharFromPos(CPoint(pt->x - Rect.left, 0)));
			if (iStart > 0) {
				m_edit->SetSel(iStart, iStart);
			}
		}
	}
}

//Called from edit box when editing is stopped
//Calling parent to save the text
void CQuickList::OnEndEdit(const int item, const int subitem, LPCSTR text, UINT endkey)
{
	//m_edit normally not yet destroyed, but will be after this fcn returns.
	//NULL if selecting dropdown item!

	if (m_edit) {
		m_editLastSel = m_edit->GetSel();
		m_editLastEndKey = endkey;
		if (m_edit->IsReadOnly())
			return;
		m_edit = NULL;
	}

	if (endkey == VK_ESCAPE) {
		RedrawItems(item, item);
		return;
	}

	//SetFocus();

	LV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDIT;

	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = item;
	dispinfo.item.iSubItem = subitem;
	dispinfo.item.pszText = (LPSTR)text;
	dispinfo.item.cchTextMax = text ? strlen(text) : 0;

	//RedrawText(item, item, subitem);
	RedrawItems(item, item);

	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
}

//When mouse wheel is used
BOOL CQuickList::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	StopEdit();
	return CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

//When vertical scroll
void CQuickList::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	StopEdit();
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

//When horisontial scorll
void CQuickList::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	StopEdit();

	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

//Stop editing.
void CQuickList::StopEdit(bool cancel)
{
	if (m_edit != NULL)
	{
		TRACE(_T("Stop editing\n"));
		//m_edit->DestroyWindow();
		m_edit->StopEdit(cancel);
	}
}

void CQuickList::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_lastItem = m_lastCol = -1;
	CListCtrl::OnRButtonDown(nFlags, point);
}

void CQuickList::OnLButtonDown(UINT nFlags, CPoint point)
{
	LVHITTESTINFO test;
	test.pt = point;
	test.flags = 0;
	test.iItem = test.iSubItem = -1;

	if (ListView_SubItemHitTest(m_hWnd, &test) != -1) {
		m_bCtrl = (nFlags&MK_CONTROL) != 0;
		int item = test.iItem;
		int subitem = test.iSubItem;
		CListItemData &id = GetItemData(item, subitem);

		ASSERT(!id.m_pXCMB || id.m_isSelected); //if no breaks, fix below

		bool bStopList = (m_pListBox != NULL);
		if (bStopList) {
			StopListBoxEdit(NULL);
			if (m_nComboItem == item && m_nComboSubItem == subitem)
				return;
		}

		bool bButton = id.m_button.m_draw && (id.m_button.m_style&DFCS_BUTTONPUSH) != 0;

		if (!bButton && id.m_button.m_draw) {
			m_lastItem = m_lastCol = -1;
		}
		else {

			if (bButton || m_lastItem == item && m_lastCol == subitem || id.m_pXCMB && id.m_isSelected)
			{
				if (bButton) {
					CRect rect;
					GetCheckboxRect(id, rect, true);

					if (m_lastItem != item || m_lastCol != subitem) {
						CListCtrl::OnLButtonDown(nFlags, point);
						m_lastItem = item;
						m_lastCol = subitem;
					}
					if (!rect.PtInRect(point)) {
						m_bCtrl = false;
						return;
					}
				}
				EditSubItem(item, subitem, &point);
				m_lastItem = m_lastCol = -1;
				m_bCtrl = false;
				return;
			}
			m_lastItem = item;
			m_lastCol = subitem;
		}
	}
	else {
		if (m_pListBox)
			StopListBoxEdit(NULL);
		else
			m_lastItem = m_lastCol = -1;
	}
	CListCtrl::OnLButtonDown(nFlags, point);
	m_bCtrl = false;
}

//When key is pressed
void CQuickList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{

#ifndef QUICKLIST_NONAVIGATION
	if (m_enableNavigation)
	{
		int incCol = 0;
		bool ret = false;

		switch (nChar)
		{
		case VK_LEFT:
		{
			//TryNavigate(m_navigationColumn-1);			
			incCol--;
			break;
		}
		case VK_RIGHT:
		{
			//TryNavigate(m_navigationColumn+1);
			incCol++;
			break;
		}

		case VK_DOWN:
		{
			if (m_bWrap && GetItemInFocus() + 1 >= GetItemCount()) {
				SetSelected(0);
				EnsureVisible(0, 0);
				ret = true;
			}
			break;
		}

		case VK_UP:
		{
			if (m_bWrap && !GetItemInFocus()) {
				SetSelected(GetItemCount() - 1);
				EnsureVisible(GetItemCount() - 1, 0);
				ret = true;
			}
			break;
		}

		case VK_SPACE:
		case VK_RETURN:
		{
			if (m_editOnEnter)
			{
				int item = GetItemInFocus();
				CListItemData id = GetItemData(item, m_navigationColumn);
				if (id.m_allowEdit && (!id.m_button.m_draw || (id.m_button.m_style&DFCS_BUTTONPUSH))) {
					if (nChar == VK_RETURN && id.m_pXCMB) {
						CPoint pt(INT_MIN, 0); //edit box instead of dropdown list
						EditSubItem(item, m_navigationColumn, &pt);
					}
					else
						EditSubItem(item, m_navigationColumn);
				}
				else if (id.m_button.m_draw) {
					//id.m_allowEdit implies checkbox
					if (id.m_allowEdit || (id.m_button.m_style&(DFCS_BUTTONPUSH | DFCS_INACTIVE)) == DFCS_BUTTONPUSH) {
						CListHitInfo hit;
						hit.m_item = item;
						hit.m_subitem = m_navigationColumn;
						hit.m_onButton = true;
						::SendMessage(GetParent()->GetSafeHwnd(),
							WM_QUICKLIST_CLICK,
							(WPARAM)GetSafeHwnd(),
							(LPARAM)&hit);
						if (id.m_allowEdit) {
							::PostMessage(m_hWnd, WM_KEYDOWN, VK_DOWN, 0);
						}

					}
				}
				return;
			}
			break;
		}

		case VK_F2:
		{
			if (m_editOnF2)
			{
				EditSubItem(GetItemInFocus(), m_navigationColumn);
				return;
			}
			break;
		}

		default:
			ret = false; //support page keys!
		}

		//Navigation column changed?
		if (incCol)
		{
			int nCols = GetHeaderCtrl()->GetItemCount();
			VEC_INT v_order(nCols);
			VERIFY(GetColumnOrderArray(&v_order[0], nCols));
			//find the actual table column --
			int nCol = 0;
			for (; nCol < nCols; nCol++) {
				if (m_navigationColumn == v_order[nCol]) {
					nCol += incCol;
					break;
				}
			}
			if (nCol < 0 || nCol >= nCols)
				return;

			nCols = m_navigationColumn; //save old navigation column

			TryNavigate(v_order[nCol]);

			//Redraw sub items (focus rect is changed)
			int focus = GetItemInFocus();
			RedrawSubitems(focus,
				focus,
				nCols);

			RedrawSubitems(focus,
				focus,
				m_navigationColumn);
			return;
		}

		if (ret) return;

	}
#endif

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CQuickList::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (m_editOnWriting &&
		nChar != VK_RETURN &&
		nChar != VK_SPACE &&
		nChar != VK_BACK &&
		nRepCnt >= 0 &&
		nChar >= 32 //Ignore ctrl+a, b and similar
		)
	{
		//TRACE(_T("Edit by writing\n"));
		//CString text( (TCHAR) nChar, nRepCnt==0?1:nRepCnt );

		EditSubItem(GetItemInFocus(), m_navigationColumn);
		return;
	}

	CListCtrl::OnChar(nChar, nRepCnt, nFlags);
}

//Try to navigate to a new column
void CQuickList::TryNavigate(const int newcol)
{
#ifndef QUICKLIST_NONAVIGATION
	if (!m_enableNavigation)
		return;

	if (newcol < 0 || newcol >= GetHeaderCtrl()->GetItemCount())
		return;

	CQuickList::CNavigationTest test;
	test.m_previousColumn = m_navigationColumn;
	test.m_newColumn = newcol;
	test.m_allowChange = true;

	//Ask parent if it's ok to change column
	::SendMessage(GetParent()->GetSafeHwnd(),
		WM_QUICKLIST_NAVIGATIONTEST,
		(WPARAM)GetSafeHwnd(),
		(LPARAM)&test);

	if (test.m_allowChange)
		SetNavigationColumn(test.m_newColumn);
#else
	UNREFERENCED_PARAMETER(newcol);
#endif
}

//Set navigation column
void CQuickList::SetNavigationColumn(const int column)
{
#ifndef QUICKLIST_NONAVIGATION
	//MakeColumnVisible(column);

	int oldCol = m_navigationColumn;

	int topVisible = 0, bottomVisible = GetItemCount() - 1;
	MakeInside(topVisible, bottomVisible);

	//Redraw the visible items that are selected
	for (int item = topVisible; item <= bottomVisible; item++)
	{
		if (IsSelected(item, 0))
		{
			m_navigationColumn = -1;
			RedrawSubitems(item, item, oldCol);
			m_navigationColumn = column;
			RedrawSubitems(item, item, m_navigationColumn);
		}
	}

	m_navigationColumn = column;
#else
	UNREFERENCED_PARAMETER(column);
#endif
}

//Pretranslate message
BOOL CQuickList::PreTranslateMessage(MSG* pMsg)
{
#ifndef QUICKLIST_NONAVIGATION
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam) {
			//case VK_UP:
			//case VK_DOWN:
		case VK_RETURN:
		{
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;
		}
		}
	}
#endif

	if (m_bHdrToolTip && (pMsg->message == WM_LBUTTONDOWN ||
		pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEMOVE)) {
		m_hdrToolTip.RelayEvent(pMsg);
		if (pMsg->message == WM_MOUSEMOVE && !m_hdrToolTipRect.IsRectNull()) {
			CPoint point(pMsg->pt);
			ScreenToClient(&point);
			if (!m_hdrToolTipRect.PtInRect(point)) {
				//TRACE(_T("m_hdrToolTip.Pop() - pt.x=%d,pt.y=%d rect.lf=%d,rect.rt=%d\n"),
					//point.x,point.y,m_hdrToolTipRect.left,m_hdrToolTipRect.right);

				m_hdrToolTip.Pop(); //kill tooltip header does not do it
			}
		}
	}

	return CListCtrl::PreTranslateMessage(pMsg);
}

//Make a column visible
void CQuickList::MakeColumnVisible(int nCol)
{
	if (nCol < 0)
		return;

	// Get the column offset
	int colwidth, offset = 0;
	{
		VEC_INT v_order(GetHeaderCtrl()->GetItemCount());
		VERIFY(GetColumnOrderArray(&v_order[0], v_order.size()));
		ASSERT(nCol < (int)v_order.size());
		it_int it = v_order.begin();
		for (; *it != nCol; it++) {
			if (it == v_order.end()) {
				ASSERT(0);
				return;
			}
			offset += GetColumnWidth(*it);
		}
		colwidth = GetColumnWidth(*it);
	}

	CRect rect;
	GetItemRect(0, &rect, LVIR_BOUNDS);
	// Now scroll if we need to show the column
	CRect rcClient;
	GetClientRect(&rcClient);
	if (offset + rect.left < 0 || offset + colwidth + rect.left > rcClient.right)
	{
		CSize size(offset + rect.left, 0);
		Scroll(size);
		Invalidate(FALSE);
		UpdateWindow();
	}
}


#ifndef QUICKLIST_NOTOOLTIP

/*
BOOL CQuickList::PtInHeader(const CPoint &point) const
{
		CRect rectHdr;
		CListCtrl::GetSubItemRect(0,0,LVIR_BOUNDS,rectHdr);
		//rectHdr == client coords of part visible in row 0 plus scroll distance
		int offset=rectHdr.left; //<= 0
		m_HeaderCtrl.GetClientRect(rectHdr); //hdr client rect in hdr coords
		rectHdr.right+=offset; //rectHdr now visible portion of header in client coords
		return rectHdr.PtInRect(point);
}
*/

//On tool tip test
int CQuickList::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
	//The list could get focus in XP. We don't like that
	//if we are editing 
	if (m_edit != NULL)
		return -1;

	LVHITTESTINFO lvhitTestInfo;

	lvhitTestInfo.pt = point;
	int nItem = ListView_SubItemHitTest(this->m_hWnd, &lvhitTestInfo);

	int nSubItem = lvhitTestInfo.iSubItem;
	UINT nFlags = lvhitTestInfo.flags;

	if (nItem < 0) {
		//BOOL bInHdr=PtInHeader(point);
		if (!m_bHdrToolTip || nSubItem <= 0 || point.y > m_hdrHeight) return -1;
		nFlags = LVHT_ONITEMLABEL;
		nItem = 0;
	}

	// nFlags is 0 if the SubItemHitTest fails
	// Therefore, 0 & <anything> will equal false
	if (nFlags & (LVHT_ONITEMLABEL | LVHT_ONITEMICON | LVHT_ONITEMSTATEICON))
	{
		// If it did fall on a list item,
		// and it was also hit one of the
		// item specific subitems we wish to show tool tips for

		// get the client (area occupied by this control
		RECT rcClient;
		GetClientRect(&rcClient);

		// fill in the TOOLINFO structure
		pTI->hwnd = m_hWnd;
		pTI->uId = (UINT)(nItem * 1000 + nSubItem + 1);
		pTI->lpszText = LPSTR_TEXTCALLBACK;
		pTI->rect = rcClient;

		//TRACE(_T("OnToolHitTest rtn: %d\n"),pTI->uId);

		return pTI->uId;	// By returning a unique value per listItem,
							// we ensure that when the mouse moves over another
							// list item, the tooltip will change
	}
	else
	{
		//Otherwise, we aren't interested, so let the message propagate
		return -1;
	}
}

//On tooltip text
BOOL CQuickList::OnToolTipText(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	UINT nID = pNMHDR->idFrom;

	// check if this is the automatic tooltip of the control
	if (nID == 0)
		return TRUE;	// do not allow display of automatic tooltip,
						// or our tooltip will disappear

	*pResult = 0;

	// get the mouse position
	const MSG* pMessage;
	pMessage = GetCurrentMessage();
	ASSERT(pMessage);
	CPoint pt;
	pt = pMessage->pt;		// get the point from the message
	ScreenToClient(&pt);	// convert the point's coords to be relative to this control

	if (m_bHdrToolTip) {
		if (pt.y < m_hdrHeight)
			pt.y = m_hdrHeight;
		else {
			return TRUE;
		}
	}

	// see if the point falls onto a list item
	m_hdrToolTipRect.SetRect(0, 0, 0, 0);
	int nItem = -1, nSubItem = -1;

	if (!HitTest(pt, nItem, nSubItem)) {
		return TRUE;
	}

	ASSERT(nItem >= 0);

	if (!m_bHdrToolTip && nSubItem)
		return TRUE;

	//if((nSubItem==0)==!bFieldlist) //fieldlist:200 else 64
		//return TRUE;

	//Evidently, this must be called each time when other tooltip contexts are active! 
	ToolTipCustomize(m_nTTInitial, m_nTTAutopop, m_nTTReshow, m_bHdrToolTip ? &m_hdrToolTip : NULL);

	if (m_bHdrToolTip && pt.y == m_hdrHeight) {
		CListCtrl::GetSubItemRect(0, 0, LVIR_BOUNDS, m_hdrToolTipRect);
		//m_hdrToolTipRect == client coords of part visible in row 0 plus scroll distance
		int xoff = m_hdrToolTipRect.left; //<= 0
		//now get rect in clent coord of subitem --
		m_HeaderCtrl.GetItemRect(nSubItem, m_hdrToolTipRect);
		m_hdrToolTipRect.left += xoff; m_hdrToolTipRect.right += xoff;
		if (nSubItem <= 0) {
			return TRUE;
		}
		nItem = -1;
	}

	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	BOOL bW = (pNMHDR->code != TTN_NEEDTEXTA);

	QUICKLIST_TTINFO ttinfo(nItem, nSubItem, bW, bW ? (LPVOID)&pTTTW->szText : (LPVOID)&pTTTA->szText);
	//TRACE("sending GetToolTip(nItem=%d,..)\n",nItem);
	if (::SendMessage(GetParent()->GetSafeHwnd(),
		WM_QUICKLIST_GETTOOLTIP,
		(WPARAM)GetSafeHwnd(),
		(LPARAM)&ttinfo))
	{
		if (!ttinfo.pvText80) {
			ASSERT(0);
			return TRUE;
		}

		if (!bW) {
			if ((LPVOID)pTTTA->szText != ttinfo.pvText80) {
				pTTTA->hinst = 0;
				pTTTA->lpszText = (LPSTR)ttinfo.pvText80;
			}
		}
		else {
			ASSERT(pNMHDR->code == TTN_NEEDTEXTW);
			if ((LPVOID)pTTTW->szText != ttinfo.pvText80) {
				pTTTW->hinst = 0;
				pTTTW->lpszText = (LPWSTR)ttinfo.pvText80;
			}
		}
	}
	return TRUE;
}
#endif

//On click 
BOOL CQuickList::OnClickEx(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	//Try to change navigation column when user clicks on an item
	CListHitInfo hit;

	if (HitTest(pNMListView->ptAction, hit))
	{
		//First, try to navigate to the column
		TryNavigate(hit.m_subitem);
		::SendMessage(GetParent()->GetSafeHwnd(),
			WM_QUICKLIST_CLICK,
			(WPARAM)GetSafeHwnd(),
			(LPARAM)&hit);
	}

	*pResult = 0;

	return FALSE;
}

//On double click
BOOL CQuickList::OnDblClickEx(NMHDR* pNMHDR, LRESULT*) //pResult)
{
	NMLISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	CQuickList::CListHitInfo hit;

	if (HitTest(pNMListView->ptAction, hit))
	{
		TryNavigate(hit.m_subitem);
		if (!::SendMessage(GetParent()->GetSafeHwnd(),
			WM_QUICKLIST_DBLCLICK,
			(WPARAM)GetSafeHwnd(),
			(LPARAM)&hit)) {

			if (!hit.m_onButton)
				EditSubItem(hit.m_item, hit.m_subitem, &pNMListView->ptAction);
		}
	}
	return FALSE;
}

void CQuickList::OnMouseMove(UINT nFlags, CPoint point)
{
	/*	If you run CQuickList on a WindowXP system
		and using a manifest file and editing an item
		and moving the mouse pointer over the list,
		some list item will be redrawn over the edit
		box. The solution to this is to handle the
		WM_MOUSEMOVE message and make sure the
		base class isn't called.
	*/

	//Don't call base class!!!
	//CListCtrl::OnMouseMove(nFlags, point);
}

void CQuickList::OnLvnHotTrack(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	/*	If you run CQuickList on a WindowXP system
		and using a manifest file, the hot item
		will be changed. This is not good.
		To solve this, we handle the LVN_HOTTRACK message
		and return -1.

		Sense OnMouseMove is also handled this function
		will never be called, but it's still good to have.
	*/
	*pResult = -1;
}

#if 0
//Never called
void CQuickList::OnLvnItemChanging(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	/*	If you run CQuickList on a WindowXP system
		and using a manifest file, the hot item
		will be changed. This is not good.
		To solve this, we handle the LVN_HOTTRACK message
		and return -1.

		Sense OnMouseMove is also handled this function
		will never be called, but it's still good to have.
	*/
	*pResult = -1;
}
#endif

void CQuickList::PreSubclassWindow()
{
#ifdef _DEBUG
	//Doesn't have LVS_OWNERDATA?
	if (!(GetStyle() & LVS_OWNERDATA))
	{
		::MessageBox(NULL, _T("ERROR : You must set the LVS_OWNERDATA style for your CQuickList in the resources"), _T("Error"), MB_OK);
	}

	//Have owner draw fixed?
	if (GetStyle() & LVS_OWNERDRAWFIXED)
	{
		//ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0 );
		//::MessageBox(NULL, _T("ERROR : You should NOT set the LVS_OWNERDRAWFIXED style for your CQuickList in the resources"), _T("Error"), MB_OK);
	}

	//Not in report view?
	DWORD style = GetStyle()&LVS_TYPEMASK;

	if (style != LVS_REPORT)
	{
		::MessageBox(NULL, _T("You should only use CQuickList in report view. Please change!"), _T("Why not report view?"), MB_OK);
	}
#endif

	if (m_bUseHeaderEx) {
		SubclassHeader();
		m_bUseHeaderEx = false;
		//  m_nTTAutopop=3200;
		//	m_nTTInitial=64;
		//  m_nTTReshow=32; //for thumblist

		if (m_bHdrToolTip) {
			m_hdrToolTip.Create(this);
			m_hdrToolTip.SetMaxTipWidth(500);
			m_hdrToolTip.SetDelayTime(TTDT_AUTOPOP, m_nTTAutopop);
			m_hdrToolTip.SetDelayTime(TTDT_INITIAL, m_nTTInitial + 200);
			m_hdrToolTip.SetDelayTime(TTDT_RESHOW, m_nTTReshow);
			m_hdrToolTip.AddTool(&m_HeaderCtrl);
		}
	}
	CListCtrl::PreSubclassWindow();
}

#if 0
//To handle the "right click on column header" message
BOOL CQuickList::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Credits!
	// http://www.codeguru.com/Cpp/controls/listview/columns/article.php/c985

	HD_NOTIFY	*pHDN = (HD_NOTIFY*)lParam;

	LPNMHDR pNH = (LPNMHDR)lParam;
	// wParam is zero for Header ctrl
	if (wParam == 0 && pNH->code == NM_RCLICK)
	{
		// Right button was clicked on header
		CPoint pt(GetMessagePos());
		CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
		pHeader->ScreenToClient(&pt);

		CHeaderRightClick cr;
		cr.m_column = -1;
		cr.m_mousePos = pt;

		// Determine the column index
		CRect rcCol;
		for (int i = 0; Header_GetItemRect(pHeader->m_hWnd, i, &rcCol); i++)
		{
			if (rcCol.PtInRect(pt))
			{
				cr.m_column = i;
				break;
			}
		}

		//Make screen cordinates
		ClientToScreen(&cr.m_mousePos);

		//Notify parent
		::SendMessage(GetParent()->GetSafeHwnd(),
			WM_QUICKLIST_HEADERRIGHTCLICK,
			(WPARAM)GetSafeHwnd(),
			(LPARAM)&cr);
	}


	return CListCtrl::OnNotify(wParam, lParam, pResult);
}
#endif

void CQuickList::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	/*
	if(m_pListBox && m_pListBox!=pNewWnd) {
		StopListBoxEdit(NULL);
	}
	*/
	m_lastItem = m_lastCol = -1;
}

//------------------------------------------------------------------------
//! Takes the current font and increases the font with the given margin
//! multiplier. Increases the row-height but keeps the cell font intact.
//! Gives more room for the grid-cell editors and their border.
//------------------------------------------------------------------------
void CQuickList::InitFonts(double margin)
{
	m_Margin = margin;

	LOGFONT lf = { 0 };
	VERIFY(GetFont()->GetLogFont(&lf) != 0);

	if (m_pCellFont) {
		VERIFY(m_pCellFont->DeleteObject());
		delete m_pCellFont;
		m_pCellFont = NULL;
	}
	m_pCellFont = new CFont;
	VERIFY(m_pCellFont->CreateFontIndirect(&lf));

	if (wOSVersion < 0x0600) {
		lf.lfHeight = (int)(lf.lfHeight * m_Margin);
		lf.lfWidth = (int)(lf.lfWidth * m_Margin);
		if (m_pGridFont) {
			VERIFY(m_pGridFont->DeleteObject());
			delete m_pGridFont;
		}
		m_pGridFont = new CFont();
		VERIFY(m_pGridFont->CreateFontIndirect(&lf));
	}

	if (GetToolTips() != NULL && GetToolTips()->m_hWnd != NULL)
		GetToolTips()->SetFont(m_pCellFont);
}

void CQuickList::EnableHeaderEx()
{
	//Must be called after adding columns --
	if (m_bUsingHeaderEx) {
		ASSERT(m_pCellFont);
		//ASSERT(m_pGridFont==m_HeaderCtrl.GetFont());
		HDITEM hdItem;
		hdItem.mask = HDI_FORMAT;
		for (int i = 0; i < m_HeaderCtrl.GetItemCount(); i++) {
			m_HeaderCtrl.GetItem(i, &hdItem);
			hdItem.fmt |= HDF_OWNERDRAW;
			VERIFY(m_HeaderCtrl.SetItem(i, &hdItem));
		}
		m_HeaderCtrl.m_pCellFont = m_pCellFont;
	}
}

void CQuickList::InitHeaderStyle()
{
#ifdef _USE_THEMES
	if (m_bUsingHeaderEx) {
		ASSERT(m_HeaderCtrl.m_hWnd);
		if (!m_pCellFont) InitFonts(1.2); //m_pCellfont same as list, m_pGridFont larger
		if (wOSVersion < 0x0600) {
			CListCtrl::SetFont(m_pGridFont); //necessary for XP gridlines to work!
			m_HeaderCtrl.SetFont(m_pGridFont);
		}
		else {
			ASSERT(!m_pGridFont);
			m_HeaderCtrl.SetFont(m_pCellFont);
		}
	}
	else {
		ASSERT(!m_HeaderCtrl.m_hWnd);
		if (m_pCellFont) {
			GetHeaderCtrl()->SetFont(m_pCellFont);
		}
	}
#else
	ASSERT(m_bUsingHeaderEx && m_HeaderCtrl.m_hWnd);
	if (!m_pCellFont) InitFonts(1.2); //m_pCellfont same as list, m_pGridFont larger
#if 1
	CListCtrl::SetFont(m_pCellFont);
	m_HeaderCtrl.SetFont(m_pCellFont); //necessary for XP gridlines to work!
#else
	CListCtrl::SetFont(m_pGridFont);
	m_HeaderCtrl.SetFont(m_pGridFont); //necessary for XP gridlines to work!
#endif
#endif
	if (m_bHdrToolTip) {
		CRect rect;
		m_HeaderCtrl.GetClientRect(rect);
		m_hdrHeight = rect.bottom - 1;
	}
}

void CQuickList::SetSortArrow(int colIndex, bool ascending)
{
	if (m_bUsingHeaderEx) {
		m_HeaderCtrl.SetSortImage(colIndex, ascending);
	}
#ifdef _USE_THEMES
	else if (wOSVersion >= 0x0600) //dmck
	{
		CHeaderCtrl *phdr = GetHeaderCtrl();
		for (int i = phdr->GetItemCount() - 1; i >= 0; i--)
		{
			HDITEM hditem = { 0 };
			hditem.mask = HDI_FORMAT;
			VERIFY(phdr->GetItem(i, &hditem));
			hditem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
			if (i == colIndex)
			{
				hditem.fmt |= (ascending ? HDF_SORTDOWN : HDF_SORTUP);
			}
			VERIFY(phdr->SetItem(i, &hditem));
		}
	}
#endif
}

// CdbgridCtrl message handlers
void CQuickList::OnContextMenu(CWnd* pWnd, CPoint point)
{
	//necessary?
	//if( GetFocus() != this ) SetFocus();	// Force focus to finish editing

	if (point.x == -1 && point.y == -1)
	{
		return;
		// OBS! point is initialized to (-1,-1) if using SHIFT+F10 or VK_APPS
		//OnContextMenuKeyboard(pWnd, point);
	}
	::SendMessage(GetParent()->GetSafeHwnd(),
		WM_QUICKLIST_CONTEXTMENU,
		(WPARAM)pWnd,
		(LPARAM)&point);
}

/*
BOOL CQuickList::OnHeaderBeginResize(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	//if( GetFocus() != this ) SetFocus();	// Force focus to finish editing

	// Check that column is allowed to be resized
	NMHEADER* pNMH = (NMHEADER*)pNMHDR;
	int nCol = (int)pNMH->iItem;

	//dmck - doesn't prevent cursor behavior
	if (nCol<0)
	{
		*pResult = TRUE;	// Block resize
		return TRUE;		// Block event
	}

	return FALSE;
}
*/

BOOL CQuickList::OnHeaderBeginDrag(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	NMHEADER* pNMH = (NMHEADER*)pNMHDR;
	//if( GetFocus() != this ) SetFocus();	// Force focus to finish editing

	if (pNMH->iItem < 1) {
		//don't handle label col automatically?
		return *pResult = TRUE;
	}
	return FALSE;
}

BOOL CQuickList::OnHeaderEndDrag(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	NMHEADER* pNMH = (NMHEADER*)pNMHDR;

	//pNMH->iItem - item being dropped, not its column position
	//pNMH->pitem->iOrder - new column position

	if (pNMH->pitem->mask & HDI_ORDER)
	{
		if (pNMH->pitem->iOrder < 1 || pNMH->iItem < 1) {
			*pResult = TRUE;
			return TRUE;
		}
		/*
		// Correct iOrder so it is just after the last hidden column
		int nColCount = GetHeaderCtrl()->GetItemCount();
		int* pOrderArray = new int[nColCount];
		VERIFY( GetColumnOrderArray(pOrderArray, nColCount) );

		for(int i = 0; i < nColCount ; ++i)
		{
			if (IsColumnVisible(pOrderArray[i]))
			{
				pNMH->pitem->iOrder = max(pNMH->pitem->iOrder,i);
				break;
			}
		}
		delete [] pOrderArray;
		*/
	}
	return FALSE;
}

BOOL CQuickList::OnHeaderDividerDblClick(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	//if( GetFocus() != this ) SetFocus();	// Force focus to finish editing
	ResizeColumn(((NMHEADER*)pNMHDR)->iItem);

	return TRUE;	// Don't let parent handle the event
}

void CQuickList::ResizeColumn(int nCol)
{
	if (nCol < GetHeaderCtrl()->GetItemCount() - 1) {
		SetRedraw(0);
		if (wOSVersion < 0x0600 && m_pCellFont) {
			ASSERT(m_pGridFont == GetFont());
			SetFont(m_pCellFont);
			//GetHeaderCtrl()->SetFont(m_pCellFont); unnecessary, assumes header font the same!
		}
		SetColumnWidth(nCol, LVSCW_AUTOSIZE_USEHEADER);
		if (nCol && m_pAdjustWidth) {
			m_pAdjustWidth(GetParent(), nCol);
			RedrawFocusItem(); //fix painting glitch!
		}
		if (wOSVersion < 0x0600 && m_pGridFont) {
			SetFont(m_pGridFont);
		}
		SetRedraw(1);
	}
	else SetColumnWidth(nCol, m_iLastColWidth);
}

void CQuickList::OnDestroy()
{
	//unsubclass edit and list box before destruction
	if (m_HeaderCtrl.GetSafeHwnd() != NULL) {
		ASSERT(m_bUsingHeaderEx);
		m_HeaderCtrl.UnsubclassWindow();
		ASSERT(!m_HeaderCtrl.m_hWnd);
	}
	CListCtrl::OnDestroy();
}

void CQuickList::SubclassHeader()
{
#ifdef _USE_THEMESXXX
	//#ifdef _USE_THEMES
		//Called in PreSubclassWindow() and CDBGridDlg::OnThemeChanged
	bool bUsingVisualStyles = EnableVisualStyles(m_hWnd, true);
	if (wOSVersion >= 0x0600 && bUsingVisualStyles) {
		if (m_bUsingHeaderEx) {
			//Moving back to Vista theme from classic theme --
			m_HeaderCtrl.UnsubclassWindow();
			ASSERT(!m_HeaderCtrl.m_hWnd);
			m_bUsingHeaderEx = false;
			//GetHeaderCtrl()->SetFont(m_pCellFont);
			//GetHeaderCtrl()->RedrawWindow();
			//GetHeaderCtrl()->Invalidate();
			//GetHeaderCtrl()->UpdateWindow();
			//bChg=true;
		}
	}
	else
#else
	bool bUsingVisualStyles = EnableVisualStyles(m_hWnd, true);
#ifdef _DEBUG
	if (!bUsingVisualStyles)
	{
		CMsgBox("Not using visual styles!");
	}
#endif
	//ASSERT(bUsingVisualStyles); //false for XP
#endif	
	if (!m_bUsingHeaderEx) {
		//Moving to classic theme (custom header) OR starting with it from PreSubclassWindow() --
		ASSERT(!m_HeaderCtrl.m_hWnd);
		VERIFY(m_HeaderCtrl.SubclassWindow(GetHeaderCtrl()->m_hWnd));
		m_bUsingHeaderEx = true;
		//m_HeaderCtrl.SetFont(m_pGridFont);
		//m_HeaderCtrl.RedrawWindow();
		//m_HeaderCtrl.Invalidate();
		//m_HeaderCtrl.UpdateWindow();
		//bChg=true;
	}
}

void CQuickList::CellHitTest(const CPoint& pt, int& nRow, int& nCol) const
{
	nRow = -1;
	nCol = -1;

	LVHITTESTINFO lvhti = { 0 };
	lvhti.pt = pt;
	nRow = ListView_SubItemHitTest(m_hWnd, &lvhti);	// SubItemHitTest is non-const
	nCol = lvhti.iSubItem;
	if (!(lvhti.flags & LVHT_ONITEMLABEL))
		nRow = -1;
}

LRESULT CQuickList::OnTabletQuerySystemGestureStatus(WPARAM, LPARAM)
{
	return 0;
}
