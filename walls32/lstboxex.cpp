// lstboxex.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "lstboxex.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CListBoxEx,CListBox);

BEGIN_MESSAGE_MAP(CListBoxEx, CListBox)
	//{{AFX_MSG_MAP(CListBoxEx)
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_CANCELMODE()
	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CListBoxExResources::CListBoxExResources( 
	WORD bmId, WORD bmWidth, 
	WORD idCursNoDrop, 
	WORD idCursCopySingle, WORD idCursCopyMulti,
	WORD idCursMoveSingle, WORD idCursMoveMulti,
	COLORREF t1, COLORREF t2 )
{
	m_BitMapId     = bmId;
	
	m_ColorTransparent1 = t1;
	m_ColorTransparent2 = t2;

	// the bitmap width is for information only - the user of the CListBoxExResources object needs to
	// know the layout of the bitmap but the bitmap height is calculated from the height of the loaded 
	// bitmap assuming it is a two high bitmap
	m_BitmapWidth  = bmWidth;
	m_BitmapHeight = 0;
		
	CWinApp* app           = AfxGetApp();
	m_hCursorCopyMultiple  = app->LoadCursor( idCursCopyMulti );
	m_hCursorCopySingle    = app->LoadCursor( idCursCopySingle );
	m_hCursorMoveMultiple  = app->LoadCursor( idCursMoveMulti );
	m_hCursorMoveSingle    = app->LoadCursor( idCursMoveSingle );
	m_hCursorNoDrop        = app->LoadCursor( idCursNoDrop );

	ASSERT( m_hCursorCopyMultiple && m_hCursorCopySingle && m_hCursorMoveSingle && \
		m_hCursorMoveMultiple && m_hCursorNoDrop );
		
	GetSysColors();
	PrepareBitmaps( FALSE );
}

CListBoxExResources::~CListBoxExResources()
{
	UnprepareBitmaps();
}

void CListBoxExResources::UnprepareBitmaps()
{
	CBitmap* pBmp = (CBitmap*)CGdiObject::FromHandle(m_hOldBitmap);
	ASSERT(pBmp);
	VERIFY(m_dcFinal.SelectObject(pBmp));
	VERIFY(m_dcFinal.DeleteDC());
	VERIFY(m_BmpScreen.DeleteObject());
}

void CListBoxExResources::PrepareBitmaps( BOOL reinit )
{
	ASSERT( m_BitMapId );
	
	if( reinit )
		UnprepareBitmaps();
		
	CDC dcImage;
	CDC dcMasks;
		
	// create device contexts compatible with screen			
	VERIFY(dcImage.CreateCompatibleDC(0));
	VERIFY(dcMasks.CreateCompatibleDC(0));
	
	VERIFY(m_dcFinal.CreateCompatibleDC(0));

	CBitmap bitmap;
	VERIFY(bitmap.LoadBitmap(m_BitMapId));
	
	BITMAP bm;
	VERIFY(bitmap.GetObject(sizeof(BITMAP),&bm));
	
	const int bmWidth  = bm.bmWidth;
	// two high bitmap assumed
	const int bmHeight = bm.bmHeight/2;
	m_BitmapHeight = bmHeight;
	
	CBitmap* pOldImageBmp = dcImage.SelectObject(&bitmap);
	ASSERT(pOldImageBmp);
		
	CBitmap BmpMasks;
	VERIFY(BmpMasks.CreateBitmap( bmWidth,bmHeight*2,1,1,NULL ));
	
	CBitmap* pOldMaskBmp = (CBitmap*)dcMasks.SelectObject(&BmpMasks);
	ASSERT(pOldMaskBmp);
	
	// create the foreground and object masks		
	COLORREF crOldBk = dcImage.SetBkColor( m_ColorTransparent1 );
	dcMasks.BitBlt( 0, 0, bmWidth, bmHeight, &dcImage, 0, 0, SRCCOPY );
	dcImage.SetBkColor( m_ColorTransparent2 );
	dcMasks.BitBlt( 0, 0, bmWidth, bmHeight, &dcImage, 0, bmHeight, SRCAND );
	dcImage.SetBkColor( crOldBk );
	dcMasks.BitBlt( 0, bmHeight, bmWidth, bmHeight, &dcMasks, 0, 0, NOTSRCCOPY );

	// create DC to hold final image	
	VERIFY(m_BmpScreen.CreateCompatibleBitmap( &dcImage, bmWidth, bmHeight*2 ));
	CBitmap* pOldBmp = (CBitmap*)m_dcFinal.SelectObject(&m_BmpScreen);
	ASSERT(pOldBmp);
	m_hOldBitmap = pOldBmp->m_hObject;
			
	CBrush b1;
	VERIFY(b1.CreateSolidBrush( m_ColorHighlight ));
	CBrush b2;
	VERIFY(b2.CreateSolidBrush( m_ColorWindow ));
	
	m_dcFinal.FillRect( CRect(0,0,bmWidth,bmHeight), &b1 ); 
	m_dcFinal.FillRect( CRect(0,bmHeight,bmWidth,bmHeight*2), &b2 );
	
	// mask out the object pixels in the destination
	m_dcFinal.BitBlt(0,0,bmWidth,bmHeight,&dcMasks,0,0,SRCAND);
	// mask out the background pixels in the image
	dcImage.BitBlt(0,0,bmWidth,bmHeight,&dcMasks,0,bmHeight,SRCAND);
	// XOR the revised image into the destination
	m_dcFinal.BitBlt(0,0,bmWidth,bmHeight,&dcImage,0,0,SRCPAINT);

	// mask out the object pixels in the destination
	m_dcFinal.BitBlt(0,bmHeight,bmWidth,bmHeight,&dcMasks,0,0,SRCAND);
	// XOR the revised image into the destination
	m_dcFinal.BitBlt(0,bmHeight,bmWidth,bmHeight,&dcImage,0,0,SRCPAINT);

	VERIFY(dcMasks.SelectObject(pOldMaskBmp));
	VERIFY(dcImage.SelectObject(pOldImageBmp));
	
	// the result of all of this messing about is a bitmap identical with the
	// one loaded from the resources but with the lower row of bitmaps having 
	// their background changed from transparent1 to the window
	// background and the upper row having their background changed from transparent2 to the
	// highlight colour.  A derived CListBoxEx can BitBlt the relevant part
	// of the image into an item's device context for a transparent bitmap effect
	// which does not take any extra time over a normal BitBlt.
}

void CListBoxExResources::SysColorChanged()
{
	// reinitialise bitmaps and syscolors 
	// this should be called from the parent of the CListBoxExResources object
	// from the OnSysColorChange() function.
	GetSysColors();
	PrepareBitmaps( TRUE );
}

void CListBoxExResources::GetSysColors()
{
	//Use static colors. Unfortunately, windows pallete
	//changes can cause sky blue to be dithered!
	m_ColorWindow		 = RGB_LTGRAY; //lt gray
	m_ColorWindowText    = RGB_BLACK;
	m_ColorHighlight     = RGB_SKYBLUE; //sky blue
	m_ColorHighlightText = RGB_BLACK;

//	m_ColorWindow        = ::GetSysColor(COLOR_BTNFACE);
//	m_ColorHighlight     = ::GetSysColor(COLOR_HIGHLIGHT);
//	m_ColorWindowText    = ::GetSysColor(COLOR_BTNTEXT);
//	m_ColorHighlightText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);

}

/////////////////////////////////////////////////////////////////////////////
// CListBoxEx


CListBoxEx::CListBoxEx()
{
	m_bMultiple        = FALSE;
	m_bTracking        = FALSE;
	m_bRtTracking      = FALSE;
	m_bTracking1       = FALSE;
	m_bTrackingEnabled = TRUE;
	m_bDragSingle      = TRUE;
	m_hOldCursor       = NULL;
	m_hOldTrackCursor  = NULL;
	m_bCopy            = TRUE;
		
	m_lfHeight = 0;
	m_pResources = 0;
}

CListBoxEx::~CListBoxEx()
{
}

void CListBoxEx::AttachResources( const CListBoxExResources* r )
{
	if( r != m_pResources )
	{
		ASSERT(r);
		m_pResources = r;
		
		if( m_hWnd ) // if window created
			Invalidate(); 
	}
}

void CListBoxEx::MeasureItem( LPMEASUREITEMSTRUCT lpMIS )
{
	lpMIS->itemHeight = TextHeight();
}

void CListBoxEx::EnableDragDrop( BOOL enable_dd )
{
	if( !enable_dd )
		if( m_bTracking )
			CancelTracking();

	m_bTrackingEnabled = enable_dd;
}

int CListBoxEx::ItemFromPoint( const CPoint& point )
{
	// This method assumes fixed-height items.

    int index=point.y/TextHeight()+GetTopIndex();
    
    if(index<GetCount()) {
      CRect itemRect;
      GetItemRect(index,&itemRect);
      if(itemRect.PtInRect(point)) return index;
    }
    return LB_ERR;
}

/////////////////////////////////////////////////////////////////////////////
// CListBoxEx message handlers

void CListBoxEx::InitTracking(CPoint &point)
{
	ASSERT(!m_bTracking);
    if(!m_bTracking) {
	  int item=ItemFromPoint(point);
      ASSERT(item==LB_ERR || GetSel(item)>0);
	  if(item!=LB_ERR && GetSel(item)>0) {
	    m_MouseDownItem=item;
	    m_bDragSingle=m_bMultiple?(GetSelCount()==1):TRUE;
	    StartTracking(TRUE);
	  }
	}
}

void CListBoxEx::OnMouseMove(UINT nFlags, CPoint point)
{
	if(!m_bTracking) {
		CListBox::OnMouseMove(nFlags, point);
		return;
	}

	ASSERT(m_pResources);

	if(!m_bTracking1) {
	  // don't actually start tracking until mouse has moved outside limits - like file manager
	  if(ItemFromPoint(point)!=m_MouseDownItem) StartTracking(FALSE,nFlags);
	}
	else {
		CPoint screen(point);
		ClientToScreen(&screen);
		CWnd* dropWnd = WindowFromPoint(screen);
		
		//If we've moved out of the previous drop window, tell it to remove its focus
		//rectangle if it has drawn one --
		
		if(m_pDropWnd && m_pDropWnd!=dropWnd) {
		  (void)CanDrop(m_pDropWnd,NULL);
		  m_pDropWnd=NULL;
		}
		
		if(!dropWnd) return;

		// If valid window convert screen co-ords to dropWnd client co-ords
		CPoint dropClientPt(screen);
		dropWnd->ScreenToClient(&dropClientPt);
		
        //Send a message to the drop window. The drop window can draw a focus
        //rectangle if appropriate --
        
		if(!CanDrop(dropWnd,&dropClientPt))
		{
			// change to no-drop cursor if this has not already been done --
			if( !m_hOldTrackCursor )
			{
				HCURSOR hCursor = GetCursor(LbExNoDrop);
				if(!hCursor)
					hCursor = m_pResources->CursorNoDrop();
				ASSERT(hCursor);
				m_hOldTrackCursor = ::SetCursor(hCursor);
			}
		}
		else
		{
		    m_pDropWnd=dropWnd;
			// change drop cursor if this has not already been done --
			if( m_hOldTrackCursor )
			{
				::SetCursor(m_hOldTrackCursor);
				m_hOldTrackCursor = NULL;
			}
		}
	}

	if(!m_bTracking) CListBox::OnMouseMove(nFlags, point);
}

void CListBoxEx::StartTracking( BOOL start, WORD nFlags /*=0*/)
{
	ASSERT(m_pResources);
	
	if( m_bTrackingEnabled )
	{
		if(start)
		{
		    m_pDropWnd = NULL;
			m_bTracking = TRUE;
			SetTimer(0x128,0x80,NULL);
			SetCapture();
		}
		else
		{
			m_bTracking1 = TRUE;

			// default is copy
			HCURSOR hCursor = 0;
			//Kyle Marsh had this backward --			
			m_bCopy = (nFlags&MK_CONTROL)?TRUE:FALSE;

			// attempt to get custom cursor for this list box
			eLbExCursor type = (m_bCopy) ? 
				((m_bDragSingle) ? (LbExCopySingle) : (LbExCopyMultiple)) :
				((m_bDragSingle) ? (LbExMoveSingle) : (LbExMoveMultiple));
				
			hCursor = GetCursor(type);
			
			if(!hCursor)
			{
				// if no custom cursor use default from resource object
				hCursor = (!m_bCopy) ?
					((m_bDragSingle) ? (m_pResources->CursorMoveSingle()) : (m_pResources->CursorMoveMulti())):
					((m_bDragSingle) ? (m_pResources->CursorCopySingle()) : (m_pResources->CursorCopyMulti()));
			}

			ASSERT(hCursor);
			m_hOldCursor = ::SetCursor( hCursor );
		}
	}
}

void CListBoxEx::CancelTracking()
{
	if(m_bTracking || m_bTracking1)
	{
		if(m_pDropWnd) {
		  (void)CanDrop(m_pDropWnd,NULL);
		  m_pDropWnd=NULL;
	    }
		if(m_hOldTrackCursor) ::SetCursor(m_hOldCursor);
		ReleaseCapture();
		KillTimer(0x128);
		m_hOldTrackCursor = NULL;
		m_bTracking  = m_bRtTracking = m_bTracking1 = FALSE;
	}
}

void CListBoxEx::DroppedItemsAt( const CPoint& point)
{
	// Convert drop point to screen co-ords.
	CPoint screen(point);
	ClientToScreen(&screen);

	// Determine client window receiving drop.
	CWnd* dropWnd = WindowFromPoint(screen);
	if(!dropWnd) return;

	// If valid window convert screen co-ords to dropWnd client co-ords
	CPoint clientPt(screen);
	dropWnd->ScreenToClient(&clientPt);

	// Inform derived class that drop has occurred.
	// The derived class has to determine whether the drop is valid,
	// what to send to the client, and how to send it.
	// Kyle Marsh's version omitted m_bCopy!?
	
	DroppedItemsOnWindowAt(dropWnd,clientPt,m_bCopy);
}

void CListBoxEx::CheckTracking(const CPoint& point)
{
	if(m_bTracking1 || m_bTracking)
	{
	    BOOL dropped=m_bTracking1;
		CancelTracking();
		if(dropped) DroppedItemsAt(point);
	}
}

void CListBoxEx::OnLButtonUp(UINT nFlags,CPoint point)
{
	CheckTracking(point);
	CListBox::OnLButtonUp(nFlags, point);
}


void CListBoxEx::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if(m_bRtTracking) {
		m_bCopy=2;
		CheckTracking(point);
	}
	CListBox::OnRButtonUp(nFlags, point);
}

void CListBoxEx::OnRButtonDown(UINT nFlags, CPoint point)
{
	int index = ItemFromPoint(point);

	if(!m_bMultiple) return; // no effect in single selection

	int selState = GetSel(index);
	if(selState!=LB_ERR) SetSel( index, !selState );
}

void CListBoxEx::OnCancelMode()
{
	CancelTracking();
}

BOOL CListBoxEx::ChangeTextHeight(int cHeight)
{

 	if(!m_hWnd) return FALSE;
	SetTextHeight(cHeight);
	SetRedraw(FALSE);
	
	int nItems = GetCount();
	for(int i=0; i<nItems; i++ ) SetItemHeight(i,cHeight);
	
	SetRedraw(TRUE);
	Invalidate();
	return TRUE;
}

int CListBoxEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListBox::OnCreate(lpCreateStruct) == -1)
		return -1;
		
	m_bMultiple = (GetStyle() & (LBS_MULTIPLESEL|LBS_EXTENDEDSEL)) != 0;
	
	return 0;
}

void CListBoxEx::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(FALSE);
#if 0
	ASSERT(m_pResources); //need to attach resources before creation/adding items
	
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	// draw focus rectangle when no items in listbox
	if(lpDIS->itemID == (UINT)-1)
	{
		if(lpDIS->itemAction&ODA_FOCUS)
		{
			// rcItem.bottom seems to be 0 for variable height list boxes
			lpDIS->rcItem.bottom = TextHeight();
			pDC->DrawFocusRect( &lpDIS->rcItem );
		}
		return;
	}
	else
	{
		int selChange   = lpDIS->itemAction&ODA_SELECT;
		int focusChange = lpDIS->itemAction&ODA_FOCUS;
		int drawEntire  = lpDIS->itemAction&ODA_DRAWENTIRE;
			
		if(selChange || drawEntire)
		{
			BOOL sel = lpDIS->itemState & ODS_SELECTED;
	
			COLORREF hlite   = ((sel)?(m_pResources->ColorHighlight()):(m_pResources->ColorWindow()));
			COLORREF textcol = ((sel)?(m_pResources->ColorHighlightText()):(m_pResources->ColorWindowText()));

			pDC->SetBkColor(hlite);
			pDC->SetTextColor(textcol);
			// fill the retangle with the background colour the fast way.
			pDC->ExtTextOut( 0, 0, ETO_OPAQUE, &lpDIS->rcItem, NULL, 0, NULL );		   

 			CListBoxExDrawStruct ds( pDC, 
 				&lpDIS->rcItem, sel, 
 				(DWORD)lpDIS->itemData, lpDIS->itemID,
 				m_pResources );
			DrawItemEx( ds );
		}
		
		if( focusChange || (drawEntire && (lpDIS->itemState&ODS_FOCUS)) )
			pDC->DrawFocusRect(&lpDIS->rcItem);
	}
#endif
}

void CListBoxEx::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent==0x128 && m_bTracking && m_pDropWnd) {
		  (void)CanDrop(m_pDropWnd,NULL,TRUE);
	}
}
