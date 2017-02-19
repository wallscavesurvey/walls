// ListCtrlEx.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "layersetsheet.h"
#include "ListCtrlEx.h"
//#include "EnableVisualStyles.h"

// CListCtrlEx

UINT urm_MOVE_ITEM = ::RegisterWindowMessage( _T( "_LISTCTRLEX_MOVE_ITEM" ) );

IMPLEMENT_DYNAMIC(CListCtrlEx, CListCtrl)
CListCtrlEx::CListCtrlEx()
: m_bDragging(false)
, m_bHdrInit(false)
, m_nTimer(NULL)
{
}

CListCtrlEx::~CListCtrlEx()
{
	if(m_nTimer) {
		VERIFY(::KillTimer(m_hWnd,m_nTimer));
		m_nTimer=NULL;
	}
}


BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
	//ON_NOTIFY(HDN_BEGINTRACKA, 0, OnHdrBeginTrack) //works
	//ON_NOTIFY(HDN_BEGINTRACKW, 0, OnHdrBeginTrack)
	//ON_NOTIFY(HDN_DIVIDERDBLCLICKA, 0, OnHdrDblClick) //fails
	//ON_NOTIFY(HDN_DIVIDERDBLCLICKA, 0, OnHdrDblClick)
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
    ON_WM_TIMER()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG,OnBeginDrag)
	ON_COMMAND(ID_MOVETOTOP, &CListCtrlEx::OnMovetoTop)
	ON_COMMAND(ID_MOVETOBOTTOM, &CListCtrlEx::OnMovetobottom)
	ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, OnTabletQuerySystemGestureStatus)
END_MESSAGE_MAP()


// CListCtrlEx message handlers
/*
void CListCtrlEx::OnHdrBeginTrack(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult=(((NMHEADER*)pNMHDR)->iItem==0)?1:0;
}
*/

BOOL CListCtrlEx::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
  LPNMHDR      pNMHdr   = reinterpret_cast<LPNMHDR>(lParam);
  CHeaderCtrl* pHdrCtrl = GetHeaderCtrl();

  ASSERT(pHdrCtrl);

  if (pNMHdr->hwndFrom == pHdrCtrl->m_hWnd) {

    LPNMHEADER pNMHeader = reinterpret_cast<LPNMHEADER>(pNMHdr);

	switch (pNMHdr->code) {
		case HDN_BEGINTRACKW:
		case HDN_BEGINTRACKA:
		case HDN_DIVIDERDBLCLICKW:
	    case HDN_DIVIDERDBLCLICKA:
		  if(pNMHeader->iItem==0) {
			  *pResult=1;
			  return TRUE;
		  }
	}
  }
  return CListCtrl::OnNotify(wParam,lParam,pResult);
}

void CListCtrlEx::OnBeginDrag (NMHDR* pnmhdr, LRESULT* pResult)
{
	if(m_bDragging) {
		ASSERT(0);
		return;
	}
	int items=GetItemCount();
	if(items<2) {
		return;
	}
	m_nDragIndex=((NM_LISTVIEW *)pnmhdr)->iItem;
	GetItemRect(GetTopIndex(),&m_crRange,LVIR_BOUNDS);
	m_nItemHeight=m_crRange.Height();
	int count=GetCountPerPage();
	if(count>items) count=items;
	m_crRange.bottom=m_crRange.top+count*m_nItemHeight;
	m_nDropIndex=m_nDragIndex;
	m_bDragging=true;
	SetCapture();
}

void CListCtrlEx::OnMouseMove(UINT nFlags, CPoint point) 
{
	if(m_bDragging) {
		ASSERT(m_nDragIndex>=0);

		int nIndex;

		if(m_crRange.PtInRect(point)) {
			if(m_nTimer) {
				VERIFY(::KillTimer(m_hWnd,m_nTimer));
				m_nTimer=NULL;
			}
			nIndex=GetTopIndex()+(point.y-m_crRange.top)/m_nItemHeight;
			DrawTheLines((nIndex<=m_nDragIndex)?nIndex:(nIndex+1));
		}
		else {
			if(point.x<m_crRange.left || point.x>=m_crRange.right) {
				m_bDragging=false;
				if(m_nTimer) {
					VERIFY(::KillTimer(m_hWnd,m_nTimer));
					m_nTimer=NULL;
				}
				DrawTheLines(0); //just clears when !m_bDragging
			}
			else {
				if(point.y>=m_crRange.bottom) {
					m_nTimerDir=1;
				}
				else {
					m_nTimerDir=-1;
				}
				if(!m_nTimer) {
					m_nTimer=::SetTimer(m_hWnd,1112,150,NULL);
					ASSERT(m_nTimer==1112);
				}
			}
		}
	}

	CListCtrl::OnMouseMove(nFlags, point);
}

void CListCtrlEx::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bDragging) {
		VERIFY(ReleaseCapture());
		m_bDragging=false;
		if(m_nTimer) {
			if(!::KillTimer(m_hWnd,m_nTimer)) {
				LastErrorBox();
			}
			m_nTimer=NULL;
		}
		DrawTheLines(-1); //just clears when !m_bDragging
		if(m_nDropIndex!=m_nDragIndex && m_nDropIndex!=m_nDragIndex+1) {
			CListCtrl::OnLButtonUp(nFlags, point);
			//remove and reinsert!
			//((CPageLayers *)GetParent())->MoveLayer(m_nDropIndex);
			//if(m_nDropIndex>m_nDragIndex) m_nDropIndex--; 
			GetParent()->SendMessage(urm_MOVE_ITEM,(WPARAM)m_nDragIndex,(LPARAM)m_nDropIndex);
			return;
		}
	}
	
	CListCtrl::OnLButtonUp(nFlags, point);
}

void CListCtrlEx::DrawTheLines(int nIndex)
{
   CDC *pDC = GetDC();
   CPen *pOldPen;
   CRect Rect;
   int y;

   if(m_nDropIndex!=m_nDragIndex && m_nDropIndex!=m_nDragIndex+1) {
		//Clear old line --
		if(m_nDropIndex<GetItemCount()) {
			GetItemRect(m_nDropIndex,&Rect,LVIR_BOUNDS);
			y=Rect.top;
		}
		else {
			GetItemRect(m_nDropIndex-1,&Rect,LVIR_BOUNDS);
			y=Rect.bottom;
		}
		CPen PenGray(PS_SOLID,1,::GetSysColor(COLOR_BTNFACE));
		pOldPen = (CPen *)pDC->SelectObject(&PenGray);
		pDC->MoveTo(Rect.left,y-1);
		pDC->LineTo(Rect.right-1,y-1);
		pDC->SelectStockObject(WHITE_PEN);
		pDC->MoveTo(Rect.left,y);
		pDC->LineTo(Rect.right-1,y);
		pDC->SelectObject(pOldPen);
   }

   if(!m_bDragging) {
	   ReleaseDC(pDC);
	   return;
   }

   m_nDropIndex=nIndex;

   if (m_nDropIndex!=m_nDragIndex && m_nDropIndex!=m_nDragIndex+1) {
		if(m_nDropIndex<GetItemCount()) {
			GetItemRect(m_nDropIndex,&Rect,LVIR_BOUNDS);
			y=Rect.top;
		}
		else {
			GetItemRect(m_nDropIndex-1,&Rect,LVIR_BOUNDS);
			y=Rect.bottom;
		}
		pOldPen = (CPen *)pDC->SelectStockObject(BLACK_PEN);
		pDC->MoveTo(Rect.left,y-1);
		pDC->LineTo(Rect.right-1,y-1);
		pDC->MoveTo(Rect.left,y);
		pDC->LineTo(Rect.right-1,y);
		pDC->SelectObject(pOldPen);
   }
   ReleaseDC(pDC);
}

void CListCtrlEx::OnTimer(UINT nIDEvent)
{
   if(nIDEvent==m_nTimer) {
	   if(m_bDragging) {
		  ASSERT(m_nDropIndex>=0 && m_nDropIndex<=GetItemCount());
		  int newPos=m_nDropIndex+m_nTimerDir;
		  if(newPos>=0 && newPos<=GetItemCount()) {
			int n=GetTopIndex();
			if(newPos<=n) {
				VERIFY(Scroll(CSize(0,-m_nItemHeight)));
			}
			else if(newPos>=n+GetCountPerPage()) {
				VERIFY(Scroll(CSize(0,m_nItemHeight)));
			}
			else newPos=-1;
			if(newPos>=0) {
				UpdateWindow();
				DrawTheLines(newPos);
			}
		  }
	   }
   }
   else 
	   CListCtrl::OnTimer(nIDEvent);
}

void CListCtrlEx::InitHdrCheck()
{
	if (m_bHdrInit) return;

	CHeaderCtrl* pHeadCtrl = GetHeaderCtrl();
	ASSERT(pHeadCtrl->GetSafeHwnd());

	VERIFY( m_checkHeadCtrl.SubclassWindow(pHeadCtrl->GetSafeHwnd()) );
	VERIFY( m_checkImgList.Create(IDB_CHECKBOXES, 16, 3, RGB(255,0,255)) );
	int i = m_checkImgList.GetImageCount();
	m_checkHeadCtrl.SetImageList(&m_checkImgList);
	
	HDITEM hdItem;
	hdItem.mask = HDI_IMAGE | HDI_FORMAT;
	VERIFY( m_checkHeadCtrl.GetItem(0, &hdItem) );
	hdItem.iImage = 1;
	hdItem.fmt |= HDF_IMAGE;
	
	VERIFY( m_checkHeadCtrl.SetItem(0, &hdItem) );

	m_bHdrInit = true;
}

void CListCtrlEx::CellHitTest(const CPoint& pt, int& nRow, int& nCol) const
{
	nRow = -1;
	nCol = -1;

	LVHITTESTINFO lvhti = {0};
	lvhti.pt = pt;
	nRow = ListView_SubItemHitTest(m_hWnd, &lvhti);	// SubItemHitTest is non-const
	nCol = lvhti.iSubItem;
	if (!(lvhti.flags & LVHT_ONITEMLABEL))
		nRow = -1;
}


void CListCtrlEx::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (point.x==-1 && point.y==-1)
	{
		return;
	}
	if(pWnd!=GetHeaderCtrl()) {
		CPoint pt(point);
		if (point.x==-1 && point.y==-1) return;
		ScreenToClient(&pt);
		int nRow, nCol;
		CellHitTest(pt, nRow, nCol);
		if(nRow!=-1) {
			CMenu menu;
			if(menu.LoadMenu(IDR_COLTABLE_CONTEXT)) {
				CMenu* pPopup = menu.GetSubMenu(0);
				ASSERT(pPopup != NULL);
				if(!nRow) {
					pPopup->EnableMenuItem(ID_MOVETOTOP,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				}
				else if(nRow+1>=GetItemCount()) {
					pPopup->EnableMenuItem(ID_MOVETOBOTTOM,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				}
				m_nDragIndex=nRow;
				VERIFY(pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this));
			}
		}
	}
}

void CListCtrlEx::OnMovetoTop()
{
	ASSERT(m_nDragIndex>0);

	//iDropItem=0;
	//Inside msg handler --
	//if(iDropItem>iDragIndex) iDropItem--; 
	//iDragIndex=old index, iDropItem=new index after deletion and reinsertion

	GetParent()->SendMessage(urm_MOVE_ITEM,(WPARAM)m_nDragIndex,0);
}

void CListCtrlEx::OnMovetobottom()
{
	ASSERT(m_nDragIndex+1<GetItemCount());

	//iDropItem=GetItemCount();
	//Inside msg handler --
	//if(iDropItem>iDragIndex) iDropItem--; 
	//iDragIndex=old index, iDropItem=new index after deletion and reinsertion

	GetParent()->SendMessage(urm_MOVE_ITEM,(WPARAM)m_nDragIndex,GetItemCount());
}

LRESULT CListCtrlEx::OnTabletQuerySystemGestureStatus(WPARAM,LPARAM)
{
   return 0;
}