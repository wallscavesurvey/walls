#include "stdafx.h"
#include "QuickList.h"

///////////////////////////////////////////////////////////////////////////////
// DrawArrowBox
void CQuickList::DrawArrowBox(CQuickList::CListItemData& id,CDC *pDC)
{
	CRect ArrowRect;

	VERIFY(GetTextRect(id,ArrowRect));

	//rect.top--;
	ArrowRect.bottom--;	// bottom edge is white, so this doesn't matter
	ArrowRect.left++;		// leave margin in case row is highlighted
	ArrowRect.right++;
	ArrowRect.left = ArrowRect.right - ArrowRect.Height();
	ArrowRect.DeflateRect(1, 1);

	// draw arrow box --
	CPen *pOldPen = (CPen *)pDC->SelectStockObject(BLACK_PEN);
	pDC->DrawEdge(&ArrowRect, EDGE_RAISED, BF_RECT);
	ArrowRect.DeflateRect(2, 2);
	pDC->FillSolidRect(ArrowRect, m_crBtnFace);

	// draw the 5 x 3 downarrow using blackpen
	//int x = ArrowRect.left + 1;
	//int y = ArrowRect.top + 2;
	int x = ArrowRect.left + (ArrowRect.Width()-5)/2;
	int y = ArrowRect.top + (ArrowRect.Height()-3)/2;
	int k = 5;
	for (int i = 0; i < 3; i++)
	{
		pDC->MoveTo(x, y);
		pDC->LineTo(x+k, y);
		x++;
		y++;
		k -= 2;
	}
	pDC->SelectObject(pOldPen);
	//pDC->SelectObject(pOldFont); 
	//pDC->SelectObject(pOldBrush); 
}

void CQuickList::SelectComboItem(CListItemData &id,CRect &rect)
{
	int nItem=id.GetItem();
	int nSubItem=id.GetSubItem();
	ASSERT(GetItemState(nItem, LVIS_SELECTED));

	if(m_pListBox) {
		if(m_nComboItem==nItem && m_nComboSubItem==nSubItem)
			return;
		StopListBoxEdit(NULL);
	}

	// create and populate the combo's listbox
	m_pListBox = new CXComboList(this);
	ASSERT(m_pListBox);

	if (!m_pListBox) return;
	m_nComboItem = nItem;
	m_nComboSubItem = nSubItem;

	int ht=rect.Height();
	rect.top += ht;
	rect.right-=1;

	int maxht=nItem=id.m_pXCMB->sa.size();

	if(maxht>XC_MAX_LISTHEIGHT) maxht=XC_MAX_LISTHEIGHT;

	rect.bottom = rect.top + maxht * ht; //will resize later
	ClientToScreen(&rect);

	CString szClassName = AfxRegisterWndClass(CS_CLASSDC|CS_SAVEBITS,
											  LoadCursor(NULL, IDC_ARROW));

	if(!m_pListBox->CreateEx(0, szClassName, _T(""),
						WS_POPUP | WS_VISIBLE /*| WS_VSCROLL*/ | WS_BORDER,
						rect,
						this, 0, NULL))
	{
		delete m_pListBox;
		m_pListBox=NULL;
		return;
	}

	m_pListBox->SetFont(id.m_textStyle.m_hFont?CFont::FromHandle(id.m_textStyle.m_hFont):GetCellFont(),FALSE);

	// create the vertical scrollbar
	bool bScroll=(nItem>XC_MAX_LISTHEIGHT);
	if(bScroll) {
		VERIFY(m_pListBox->m_wndSBVert.Create(WS_VISIBLE|WS_CHILD|SBS_VERT,
			CRect(0,0,0,0),m_pListBox, 0));
		m_pListBox->m_wndSBVert.SetFont(m_pListBox->GetFont(), FALSE);
	}

	int index = -1;
	for(VEC_CSTR::iterator it=id.m_pXCMB->sa.begin();it!=id.m_pXCMB->sa.end();it++) {
		if(index<0 && !(*it).Compare(id.m_text)) index=it-id.m_pXCMB->sa.begin();
		m_pListBox->AddString(*it);
	}
	m_nComboInitialIndex=index;
	if(index<0) index=0;
	m_pListBox->SetCurSel(index);
	m_pListBox->SetActive(bScroll?11:0,maxht);

}

///////////////////////////////////////////////////////////////////////////////
// OnSysColorChange

void CQuickList::InitSysColors()
{
	m_cr3DFace              = ::GetSysColor(COLOR_3DFACE);
	m_cr3DHighLight         = ::GetSysColor(COLOR_3DHIGHLIGHT);
	m_cr3DShadow            = ::GetSysColor(COLOR_3DSHADOW);
	m_crBtnFace             = ::GetSysColor(COLOR_BTNFACE);
	m_crBtnShadow           = ::GetSysColor(COLOR_BTNSHADOW);
	m_crBtnText             = ::GetSysColor(COLOR_BTNTEXT);
	m_crGrayText            = ::GetSysColor(COLOR_GRAYTEXT);
	m_crHighLight           = ::GetSysColor(COLOR_HIGHLIGHT);
	m_crHighLightText       = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_crWindow              = ::GetSysColor(COLOR_WINDOW);
	m_crWindowText          = ::GetSysColor(COLOR_WINDOWTEXT);
}

void CQuickList::OnSysColorChange()
{
	TRACE(_T("in CQuickList::OnSysColorChange\n"));

	CListCtrl::OnSysColorChange();
	InitSysColors();
}

void CQuickList::StopListBoxEdit(LPCSTR pText)
{
	ASSERT(m_pListBox);
	OnEndEdit(m_nComboItem,m_nComboSubItem,pText,0);
	m_pListBox->DestroyWindow();
	delete m_pListBox;
	m_pListBox = NULL;
	if(pText) {
		m_lastItem=m_nComboItem;
		m_lastCol=m_nComboSubItem;
	}
}

///////////////////////////////////////////////////////////////////////////////
// OnComboEscape
LRESULT CQuickList::OnComboEscape(WPARAM, LPARAM)
{
	if(m_pListBox)
		StopListBoxEdit(NULL);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnComboLButtonUp
LRESULT CQuickList::OnComboLButtonUp(WPARAM, LPARAM)
{
	if(m_pListBox)
	{
		CString str;
		LPCSTR pText=NULL;
		int i = m_pListBox->GetCurSel();
		if (i!=LB_ERR && i!=m_nComboInitialIndex) {
			m_pListBox->GetText(i,str);
			pText=str;
		}
		StopListBoxEdit(pText);
	}
	return 0;
}
