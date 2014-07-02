// DisabledCombo.cpp : implementation file
//

#include "stdafx.h"
#include "DisCombo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const UINT nMessage=::RegisterWindowMessage("ComboSelEndOK");

/////////////////////////////////////////////////////////////////////////////
// CDisabledCombo

CDisabledCombo::CDisabledCombo()
{
	m_ListBox.SetParent(this);
}

CDisabledCombo::~CDisabledCombo()
{
}

// default implementation
BOOL CDisabledCombo::IsItemEnabled(UINT nIndex) const
{
	//if (nIndex>=(DWORD)GetCount()) return TRUE;	// whatever

	//DWORD uData=GetItemData(nIndex);

	return FALSE;
}


BEGIN_MESSAGE_MAP(CDisabledCombo, CComboBox)
	//{{AFX_MSG_MAP(CDisabledCombo)
	ON_WM_CHARTOITEM()
	ON_CONTROL_REFLECT(CBN_SELENDOK, OnSelendok)
	//}}AFX_MSG_MAP
	ON_CONTROL_REFLECT(CBN_DROPDOWN, RecalcDropWidth)
	ON_MESSAGE(WM_CTLCOLORLISTBOX, OnCtlColor)
	ON_REGISTERED_MESSAGE(nMessage, OnRealSelEndOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisabledCombo message handlers

void CDisabledCombo::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	ASSERT((GetStyle()&(CBS_OWNERDRAWFIXED|CBS_HASSTRINGS))==(CBS_OWNERDRAWFIXED|CBS_HASSTRINGS));
	CComboBox::PreSubclassWindow();
}

void CDisabledCombo::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	// TODO: Add your code to draw the specified item
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);

	if (((LONG)(lpDrawItemStruct->itemID) >= 0) &&
		(lpDrawItemStruct->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)))
	{
		BOOL fDisabled = !IsWindowEnabled () || !IsItemEnabled(lpDrawItemStruct->itemID);

		COLORREF newTextColor = fDisabled ?
			RGB(0x80, 0x80, 0x80) : GetSysColor (COLOR_WINDOWTEXT);  // light gray

		COLORREF oldTextColor = pDC->SetTextColor (newTextColor);

		COLORREF newBkColor = GetSysColor (COLOR_WINDOW);
		COLORREF oldBkColor = pDC->SetBkColor (newBkColor);

		if (newTextColor == newBkColor)
			newTextColor = RGB(0xC0, 0xC0, 0xC0);   // dark gray

		if (!fDisabled && ((lpDrawItemStruct->itemState & ODS_SELECTED) != 0))
		{
			pDC->SetTextColor (GetSysColor (COLOR_HIGHLIGHTTEXT));
			pDC->SetBkColor (GetSysColor (COLOR_HIGHLIGHT));
		}

		CString strText;
		GetLBText(lpDrawItemStruct->itemID, strText);

		const RECT &rc=lpDrawItemStruct->rcItem;

		pDC->ExtTextOut(rc.left + 2,
				  rc.top + 2,// + max(0, (cyItem - m_cyText) / 2),
				  ETO_OPAQUE, &rc,
				  strText, strText.GetLength (), NULL);

		pDC->SetTextColor (oldTextColor);
		pDC->SetBkColor (oldBkColor);
	}
	else if ((LONG)(lpDrawItemStruct->itemID)<0)	// drawing edit text
	{
		COLORREF newTextColor = GetSysColor (COLOR_WINDOWTEXT);  // light gray
		COLORREF oldTextColor = pDC->SetTextColor (newTextColor);

		COLORREF newBkColor = GetSysColor (COLOR_WINDOW);
		COLORREF oldBkColor = pDC->SetBkColor (newBkColor);

		if ((lpDrawItemStruct->itemState & ODS_SELECTED) != 0)
		{
			pDC->SetTextColor (GetSysColor (COLOR_HIGHLIGHTTEXT));
			pDC->SetBkColor (GetSysColor (COLOR_HIGHLIGHT));
		}

		CString strText;
		GetWindowText(strText);
		const RECT &rc=lpDrawItemStruct->rcItem;

		pDC->ExtTextOut(rc.left + 2,
				  rc.top + 2,// + max(0, (cyItem - m_cyText) / 2),
				  ETO_OPAQUE, &rc,
				  strText, strText.GetLength (), NULL);

		pDC->SetTextColor (oldTextColor);
		pDC->SetBkColor (oldBkColor);
	}

	if ((lpDrawItemStruct->itemAction & ODA_FOCUS) != 0)
		pDC->DrawFocusRect(&lpDrawItemStruct->rcItem);
	
}

void CDisabledCombo::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	// TODO: Add your code to determine the size of specified item
	UNREFERENCED_PARAMETER(lpMeasureItemStruct);
}

int CDisabledCombo::OnCharToItem(UINT nChar, CListBox* pListBox, UINT nIndex) 
{
	// TODO: Add your message handler code here and/or call default
	
	int ret=CComboBox::OnCharToItem(nChar, pListBox, nIndex);
	if (ret>=0 && !IsItemEnabled(ret))
		return -2;
	else
		return ret;
}

void CDisabledCombo::OnSelendok() 
{
	// TODO: Add your control notification handler code here
	GetWindowText(m_strSavedText);
	PostMessage(nMessage);	
}

LRESULT CDisabledCombo::OnRealSelEndOK(WPARAM,LPARAM)
{
	CString currentText;
	GetWindowText(currentText);

	int index=FindStringExact(-1,currentText);
	if (index>=0 && !IsItemEnabled(index))
	{
		SetWindowText(m_strSavedText);
		GetParent()->SendMessage(WM_COMMAND,MAKELONG(GetWindowLong(m_hWnd,GWL_ID),CBN_SELCHANGE),(LPARAM)m_hWnd);
	}

	return 0;
}

LRESULT CDisabledCombo::OnCtlColor(WPARAM,LPARAM lParam)
{
	if (m_ListBox.m_hWnd==NULL && lParam!=0 && lParam!=(LPARAM)m_hWnd)
		m_ListBox.SubclassWindow((HWND)lParam);

	return Default();
}


void CDisabledCombo::PostNcDestroy() 
{
	// TODO: Add your specialized code here and/or call the base class
//	m_ListBox.UnsubclassWindow();
	m_ListBox.Detach();
	
	CComboBox::PostNcDestroy();
}

void CDisabledCombo::RecalcDropWidth()
{
    // Reset the dropped width
    int nNumEntries = GetCount();
    int nWidth = 0;
    CString str;

    CClientDC dc(this);
    int nSave = dc.SaveDC();
    dc.SelectObject(GetFont());

    int nScrollWidth = ::GetSystemMetrics(SM_CXVSCROLL);
    for (int i = 0; i < nNumEntries; i++)
    {
        GetLBText(i, str);
        int nLength = dc.GetTextExtent(str).cx + nScrollWidth;
        nWidth = max(nWidth, nLength);
    }
    
    // Add margin space to the calculations
    nWidth += dc.GetTextExtent("0").cx;

    dc.RestoreDC(nSave);
    SetDroppedWidth(nWidth);
}

/////////////////////////////////////////////////////////////////////////////
// CListBoxInsideComboBox

CListBoxInsideComboBox::CListBoxInsideComboBox()
{
	m_Parent=NULL;
}

CListBoxInsideComboBox::~CListBoxInsideComboBox()
{
}


BEGIN_MESSAGE_MAP(CListBoxInsideComboBox, CWnd)
	//{{AFX_MSG_MAP(CListBoxInsideComboBox)
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CListBoxInsideComboBox message handlers

void CListBoxInsideComboBox::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CRect rect;	GetClientRect(rect);

	if (rect.PtInRect(point))
	{
		BOOL outside=FALSE;
		int index=((CListBox *)this)->ItemFromPoint(point,outside);
		if (!outside && !m_Parent->IsItemEnabled(index))
			return;	// don't click there
	}
	
	CWnd::OnLButtonUp(nFlags, point);
}


void CListBoxInsideComboBox::SetParent(CDisabledCombo *ptr)
{
	m_Parent=ptr;
}
