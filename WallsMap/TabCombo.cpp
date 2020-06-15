// TabCombo.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "EditLabel.h"
#include "TabCombo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void OnSelChangeFindStr(CDialog *pDlg, CTabComboHist *phist_find)
{
	int idx = ((CComboBox *)pDlg->GetDlgItem(IDC_FIND_COMBO))->GetCurSel();
	ASSERT(idx >= 0);
	CComboHistItem *pItem = phist_find->GetItem(idx);
	if (pItem) {
		idx = pItem->m_wFlags;
		((CButton *)pDlg->GetDlgItem(IDC_SEL_WHOLEWORDS))->SetCheck((idx&F_MATCHWHOLEWORD) != 0);
		((CButton *)pDlg->GetDlgItem(IDC_SEL_MATCHCASE))->SetCheck((idx&F_MATCHCASE) != 0);
		//((CButton *)pDlg->GetDlgItem(IDC_USEREGEX))->SetCheck((idx&F_USEREGEX)!=0);
	}
}

bool CTabComboHist::InsertAsLast(LPCSTR pstr, WORD flags)
{
	//no string comparisons needed
	if (m_nItems >= m_nMaxItems) return false;
	CComboHistItem *pPrev = NULL, *pNext = m_pFirst;
	while (pNext) {
		pPrev = pNext; pNext = pNext->m_pNext;
	}
	pNext = new CComboHistItem(NULL, pstr, flags);
	if (pPrev) pPrev->m_pNext = pNext;
	else m_pFirst = pNext;
	m_nItems++;
	return true;
}

void CTabComboHist::InsertAsFirst(LPCSTR pstr, WORD flags)
{
	CComboHistItem *pLast = NULL, *pItem = m_pFirst;
	int cnt = 0;

	while (pItem) {
		if (!strcmp(pItem->m_Str, pstr)) {
			pItem->m_wFlags = flags;
			if (!pLast) return;
			pLast->m_pNext = pItem->m_pNext;
			pItem->m_pNext = m_pFirst;
			m_pFirst = pItem;
			return;
		}
		if (++cnt == m_nMaxItems) {
			//We've compared with last item -- we must delete that item
			ASSERT(cnt == m_nItems);
			pLast->m_pNext = NULL;
			delete pItem;
			pItem = NULL;
			m_nItems--;
		}
		else {
			pLast = pItem;
			pItem = pItem->m_pNext;
		}
	}

	m_pFirst = new CComboHistItem(m_pFirst, pstr, flags);
	m_nItems++;
}

CComboHistItem * CTabComboHist::GetItem(int index)
{
	CComboHistItem *pItem = m_pFirst;
	while (index-- && pItem) pItem = pItem->m_pNext;
	return pItem;
}

//==========================================================================
// CTabCombo
CTabCombo::~CTabCombo()
{
}

HBRUSH CTabCombo::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_EDIT)
	{
		//Edit control
		if (m_editLabel.GetSafeHwnd() == NULL)
			m_editLabel.SubclassWindow(pWnd->GetSafeHwnd());
	}
	/*
	else if (nCtlColor == CTLCOLOR_LISTBOX)
	{
		//ListBox control
		if (m_listbox.GetSafeHwnd() == NULL)
			m_listbox.SubclassWindow(pWnd->GetSafeHwnd());
	}
	*/
	HBRUSH hbr = CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);
	return hbr;
}

void CTabCombo::OnDestroy()
{
	if (m_editLabel.GetSafeHwnd() != NULL)
		m_editLabel.UnsubclassWindow();
	//if (m_listbox.GetSafeHwnd() != NULL)
		//m_listbox.UnsubclassWindow();
	CComboBox::OnDestroy();
}

void CTabCombo::LoadHistory(CTabComboHist *pHist)
{
	int cnt = 0;
	if (pHist) {
		CComboHistItem *pItem = pHist->m_pFirst;
		while (pItem) {
			cnt++;
			VERIFY(CComboBox::AddString(pItem->m_Str) != CB_ERR);
			pItem = pItem->m_pNext;
		}
		ASSERT(cnt == pHist->m_nItems);
	}
	if (!cnt) VERIFY(CComboBox::AddString("") != CB_ERR); //avoid confusing mouse-click behavior
}

void CTabCombo::ClearListItems()
{
	for (int i = GetCount() - 1; i >= 0; i--)
	{
		DeleteString(i);
	}

	//Get rectangles
	CRect rctComboBox, rctDropDown;
	//Combo rect
	GetClientRect(&rctComboBox);
	//DropDownList rect
	GetDroppedControlRect(&rctDropDown);

	//Get Item height
	int itemHeight = GetItemHeight(-1);
	//Converts coordinates
	GetParent()->ScreenToClient(&rctDropDown);
	//Set height
	rctDropDown.bottom = rctDropDown.top + rctComboBox.Height() + itemHeight;
	//apply changes
	MoveWindow(&rctDropDown);

	m_editLabel.SetCustomCmd(0); //no need to clear again
}

BEGIN_MESSAGE_MAP(CTabCombo, CComboBox)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()
