// CheckHeadCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "CheckHeadCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCheckHeadCtrl

CCheckHeadCtrl::CCheckHeadCtrl()
{
}

CCheckHeadCtrl::~CCheckHeadCtrl()
{
}


BEGIN_MESSAGE_MAP(CCheckHeadCtrl, CHeaderCtrl)
	ON_NOTIFY_REFLECT(HDN_ITEMCLICK, OnItemClicked)
	//ON_NOTIFY_REFLECT_EX(HDN_ITEMCHANGING, OnItemchanging)
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCheckHeadCtrl message handlers
void CCheckHeadCtrl::OnItemClicked(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMHEADER* pNMHead = (NMHEADER*)pNMHDR;
	*pResult = 0;

	int nItem = pNMHead->iItem;
	if (0 != nItem)
		return;

	HDITEM hdItem;
	hdItem.mask = HDI_IMAGE;
	VERIFY( GetItem(nItem, &hdItem) );

	if (hdItem.iImage == 1)
		hdItem.iImage = 2;
	else
		hdItem.iImage = 1;

	VERIFY( SetItem(nItem, &hdItem) );
	
	BOOL bl = hdItem.iImage == 2 ? TRUE : FALSE;
	CListCtrl* pListCtrl = (CListCtrl*)GetParent();
	int nCount = pListCtrl->GetItemCount();	
	for(nItem = 0; nItem < nCount; nItem++)
	{
		ListView_SetCheckState(pListCtrl->GetSafeHwnd(), nItem, bl);
	}
}

BOOL CCheckHeadCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return TRUE;
//	return CHeaderCtrl::OnSetCursor(pWnd, nHitTest, message);
}

void CCheckHeadCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
//	CHeaderCtrl::OnLButtonDblClk(nFlags, point);
}

void CCheckHeadCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	HD_ITEM hi;
	hi.mask = HDI_WIDTH;	// We want the column width.
	GetItem(0, &hi);
	if(point.x<hi.cxy)
		CHeaderCtrl::OnLButtonDblClk(nFlags, point);
}

#if 0
BOOL CCheckHeadCtrl::OnItemchanging(NMHDR* pNMHDR, LRESULT* pResult) 
{
	HD_NOTIFY *phdn = (HD_NOTIFY*) pNMHDR;
	*pResult=1;
	return TRUE;

	/*
	bool bFrozen = false;

	if(m_bEnableFreeze)

	{

		int nItem = phdn->iItem;

		bFrozen = (NULL != m_lstFrozenCols.Find(nItem));

	}

	*pResult = (bFrozen ? 1 : 0); // keep user from resizing certain columns
	*/

}
#endif