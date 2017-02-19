// TabCombo.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "TabCombo.h"
#include <dlgs.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTabComboHist hist_prjfind(6),hist_prjrepl(6);

void OnSelChangeFindStr(CDialog *pDlg,BOOL bGlobal)
{
	//LPCSTR s=GetFindStringPtr();
	int idx=((CComboBox *)pDlg->GetDlgItem(IDC_FINDSTRING))->GetCurSel();
	ASSERT(idx>=0);
	CComboHistItem *pItem=hist_prjfind.GetItem(idx);
	if(pItem) {
		idx=pItem->m_wFlags;
		((CButton *)pDlg->GetDlgItem(bGlobal?IDC_MATCHWHOLEWORD:1040))->SetCheck((idx&F_MATCHWHOLEWORD)!=0);
		((CButton *)pDlg->GetDlgItem(bGlobal?IDC_MATCHCASE:1041))->SetCheck((idx&F_MATCHCASE)!=0);
		((CButton *)pDlg->GetDlgItem(IDC_USEREGEX))->SetCheck((idx&F_USEREGEX)!=0);
	}
}

void CTabComboHist::InsertAsFirst(CString &str,WORD flags)
{
	CComboHistItem *pLast=NULL,*pItem=m_pFirst;
	int cnt=0;
	
	while(pItem) {
		if(!str.Compare(pItem->m_Str)) {
			pItem->m_wFlags=flags; 
			if(!pLast) return;
			pLast->m_pNext=pItem->m_pNext;
			pItem->m_pNext=m_pFirst;
			m_pFirst=pItem;
			return;
		}
		if(++cnt==m_nMaxItems) {
			//We've compared with last item -- we must delete that item
			ASSERT(cnt==m_nItems);
			pLast->m_pNext=NULL;
			delete pItem;
			pItem=NULL;
			m_nItems--;
		}
		else {
			pLast=pItem;
			pItem=pItem->m_pNext;
		}
	}

	m_pFirst=new CComboHistItem(m_pFirst,str,flags);
	m_nItems++;
}

CComboHistItem * CTabComboHist::GetItem(int index)
{
	CComboHistItem *pItem=m_pFirst;
	while(index-- && pItem) pItem=pItem->m_pNext;
	return pItem;
}

// CTabCombo

CTabCombo::CTabCombo()
{
}

CTabCombo::~CTabCombo()
{
}

void CTabCombo::LoadHistory(CTabComboHist *pHist)
{
	int cnt=0;
	CComboHistItem *pItem=pHist->m_pFirst;
	while(pItem) {
		cnt++;
		VERIFY(CComboBox::AddString(pItem->m_Str)!=CB_ERR);
		pItem=pItem->m_pNext;
	}
	if(!cnt) VERIFY(CComboBox::AddString("")!=CB_ERR); //avoid confusing mouse-click behavior
	ASSERT(cnt==pHist->m_nItems);
}

BEGIN_MESSAGE_MAP(CTabCombo, CComboBox)
	//{{AFX_MSG_MAP(CTabCombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCombo message handlers


BOOL CTabCombo::PreTranslateMessage(MSG* pMsg) 
{
   if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_TAB && ::GetKeyState(VK_CONTROL)<0) {
	    ((CEdit *)GetWindow(GW_CHILD))->ReplaceSel("\t");
		return TRUE;
   }
   return CComboBox::PreTranslateMessage(pMsg);
}
