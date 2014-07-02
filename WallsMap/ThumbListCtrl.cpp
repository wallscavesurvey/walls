//	CThumbListCtrl - Enhanced CListCtrl with thumbnail dragging and customized tool tips.
//

#include "stdafx.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "ImageViewDlg.h"
#include "ThumbListCtrl.h"
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <afxdisp.h>	// OLE stuff
#include <shlwapi.h>	// Shell functions (PathFindExtension() in this case)
#include <afxpriv.h>	// ANSI to/from Unicode conversion macros
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

CThumbListCtrl::CThumbListCtrl()
{
	//
	// Default drop mode
	//
#ifdef _USE_TT
	//m_pchTip = NULL;
	//m_pwchTip = NULL;
	//m_bToolTipCtrlCustomizeDone = false;
#endif
	m_bDragging=false;
	m_nTimer=NULL;
}

CThumbListCtrl::~CThumbListCtrl()
{
	if(m_nTimer) {
		VERIFY(::KillTimer(m_hWnd,m_nTimer));
		m_nTimer=NULL;
	}
#ifdef _USE_TT
	/*
	if(m_pchTip != NULL)
		delete m_pchTip;

	if(m_pwchTip != NULL)
		delete m_pwchTip;
	*/
#endif
}

BEGIN_MESSAGE_MAP(CThumbListCtrl, CListCtrl)
#ifdef _USE_TT
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
#endif
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG,OnBeginDrag)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////

void CThumbListCtrl::PreSubclassWindow() 
{
	CListCtrl::PreSubclassWindow();

	m_pDlg=(CImageViewDlg *)GetParent();

	// Add initialization code
	EnableToolTips(TRUE);
}

//Image tiles are 100 x 75, space between them is 43,
//Left edge of 1st tile has offset 21 within window.
#define IM_PAGEWIDTH 143
#define IM_PAGEHEIGHT 100
#define IM_TILEWIDTH 100
#define IM_TILEHEIGHT 75
#define IM_TILEXOFF 21
#define IM_TILEYOFF 2

void CThumbListCtrl::OnBeginDrag (NMHDR* pnmhdr, LRESULT* pResult)
{
	    
	ASSERT(!m_bDragging);
	if(m_bDragging || !m_pDlg->IsDragAllowed())
		return;

	int items=GetItemCount();
	if(items<2) return;
	m_nDragIndex=((NM_LISTVIEW *)pnmhdr)->iItem;
	m_nScrollOff=GetScrollPos(SB_HORZ); //0 or negative
	GetClientRect(&m_crRange); //client size 596 x 113 initially (top=left=0)
	//m_crRange.OffsetRect(m_nScrollOff,0);

	m_nDropIndex=m_nDragIndex;
	m_bDragging=true;
	m_bInitDrag=false;
	SetCapture();
}

void CThumbListCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if(m_bDragging) {
		ASSERT(m_nDragIndex>=0);

		if(m_crRange.PtInRect(point)) {
			if(m_nTimer) {
				VERIFY(::KillTimer(m_hWnd,m_nTimer));
				m_nTimer=NULL;
			}
			if(!m_bInitDrag || (point.x>=0 && m_nDragX!=point.x)) {
				DrawTheLines(point.x);
			}
			//DrawTheLines((nIndex<=m_nDragIndex)?nIndex:(nIndex+1));
		}
		else {
			if(point.y>=m_crRange.bottom || point.y<m_crRange.top) {
				m_bDragging=false;
				if(m_nTimer) {
					VERIFY(::KillTimer(m_hWnd,m_nTimer));
					m_nTimer=NULL;
				}
				DrawTheLines(-1);
				VERIFY(ReleaseCapture());
			}
			else {
				if(point.x>=m_crRange.right) {
					m_nTimerDir=1;
				}
				else {
					m_nTimerDir=-1;
				}
				if(!m_nTimer) {
					m_nTimer=::SetTimer(m_hWnd,1112,300,NULL);
					ASSERT(m_nTimer==1112);
				}
			}
		}
	}

	CListCtrl::OnMouseMove(nFlags, point);
}

static void DrawOutLine(CDC *pDC,CRect rect)
{
	pDC->DrawFocusRect(&rect);
	rect.InflateRect(-1,-1);
	pDC->DrawFocusRect(&rect);
}

void CThumbListCtrl::DrawBar(bool bBlack)
{
	CRect rect;
	int off=IM_PAGEWIDTH/2;
	if(m_nDropIndex<GetItemCount()) off=-off;

	GetItemRect((off<0)?m_nDropIndex:(m_nDropIndex-1),&rect,LVIR_ICON);
	if(!m_nDropIndex) {
		off+=16;
	}
	else if(off>0) {
		off-=16;
	}
	rect.OffsetRect(off,0);
#ifdef _DEBUG
	int jj=(rect.Width()/2-1);
	//ASSERT((rect.Width()/2-1)==57);
#endif
	rect.InflateRect(-(rect.Width()/2-1),-4); //-57,-4 (works fine)

	CDC *pDC=GetDC();
	pDC->FillSolidRect(&rect,bBlack?RGB(0,0,0):GetSysColor(COLOR_BTNFACE));
	ReleaseDC(pDC);

    m_bBarDrawn=bBlack;
}


void CThumbListCtrl::DrawTheLines(int pointX)
{
   CDC *pDC = GetDC();

   if(pointX<0) {
	   if(m_bInitDrag) {
		   DrawOutLine(pDC,m_crDrag);
		   if(m_bBarDrawn) DrawBar(false);
		   m_bInitDrag=false;
	   }
	   ReleaseDC(pDC);
	   return;
   }

   int nBot=GetItemCount();

   int nIndex=(pointX+m_nScrollOff)/IM_PAGEWIDTH;
   if(nIndex>=nBot) nIndex=nBot-1;

   if(!m_bInitDrag) {
	   m_bInitDrag=true;
	   GetItemRect(nIndex,&m_crDrag,LVIR_ICON );
 	   m_crDrag.InflateRect(-1,-1);
	   DrawOutLine(pDC,m_crDrag);
	   m_bBarDrawn=false;
  }
  else {
	   DrawOutLine(pDC,m_crDrag);

	   m_crDrag.OffsetRect(pointX-m_nDragX,0);
	   nIndex=(m_crDrag.CenterPoint().x+m_nScrollOff+IM_PAGEWIDTH/2)/IM_PAGEWIDTH;
	   if(nIndex>GetItemCount()) nIndex=GetItemCount();
	   if(nIndex!=m_nDropIndex) {
		   if(m_bBarDrawn) DrawBar(false);
		   m_nDropIndex=nIndex;
		   DrawBar(true);
	   }
 	   DrawOutLine(pDC,m_crDrag);
  }
   m_nDragX=pointX;
   ReleaseDC(pDC);
}

void CThumbListCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if(m_bDragging) {
		DrawTheLines(-1);

		VERIFY(ReleaseCapture());

		if(m_nTimer) {
			if(!::KillTimer(m_hWnd,m_nTimer)) {
				LastErrorBox();
			}
			m_nTimer=NULL;
		}
		if(m_nDropIndex!=m_nDragIndex && m_nDropIndex!=m_nDragIndex+1) {
			CListCtrl::OnLButtonUp(nFlags, point);
			m_pDlg->MoveSelected(m_nDragIndex,m_nDropIndex);
			m_bDragging=false;
			return;
		}
		m_bDragging=false;
	}
	
	CListCtrl::OnLButtonUp(nFlags, point);
}

void CThumbListCtrl::OnTimer(UINT nIDEvent)
{
   if(nIDEvent==m_nTimer) {
	   if(m_bDragging) {
		  ASSERT(m_nDropIndex>=0 && m_nDropIndex<=GetItemCount());
		  int newPos=m_nDropIndex+m_nTimerDir;
		  if(newPos>=0 && newPos<=GetItemCount()) {
			  DrawTheLines(-1);
			  Scroll(CSize(m_nTimerDir*IM_PAGEWIDTH,0));
			  UpdateWindow();
			  m_nScrollOff=GetScrollPos(SB_HORZ); //0 or negative
			  DrawTheLines(m_nDragX);
		  }
	   }
   }
   else 
	   CListCtrl::OnTimer(nIDEvent);
}

void CThumbListCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	if(m_bDragging) return;
	CListCtrl::OnRButtonDown(nFlags, point);
}
