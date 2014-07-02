// dbrcview.cpp : implementation of the CDBRecView class
//
#include "stdafx.h"
#include "dbrcdoc.h"
#include "dbfheader.h"
#include "dbrcview.h"
#include <stdlib.h>
#include <limits.h> // for INT_MAX
//#include <ctl3d.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CDBRecView,CView)

/////////////////////////////////////////////////////////////////////////////
// CDBRecView

BEGIN_MESSAGE_MAP(CDBRecView,CView)
	//{{AFX_MSG_MAP(CDBRecView)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBRecView construction, initialization, and destruction

CDBRecView::CDBRecView()
{
	// Init everything to zero
	AFX_ZERO_INIT_OBJECT(CView);

	m_lineOffset=m_lineTotal=0L;
	m_nPrevSelectedRow = 0;
	m_phdr=NULL;
}

CDBRecView::~CDBRecView()
{
	if(m_phdr) delete m_phdr;
}

void CDBRecView::ResizeHdr(int xshift,int xrange)
{
	WINDOWPOS wp;
	HD_LAYOUT hdlay;
	CRect rect;
	hdlay.prc=&rect;
	hdlay.pwpos=&wp;
	GetClientRect(&rect);
	VERIFY(m_phdr->Layout(&hdlay));
	VERIFY(::SetWindowPos(m_phdr->m_hWnd,wp.hwndInsertAfter,
		wp.x-xshift,wp.y,rect.Width()+xrange,wp.cy,wp.flags | SWP_SHOWWINDOW));
	m_nHdrHeight=wp.cy+1;
	//m_phdr->Invalidate();
}

void CDBRecView::OnInitialUpdate()
{
//	Ctl3dSubclassDlg(m_hWnd,CTL3D_ALL);
//	CView::OnInitialUpdate();

	m_nPrevRowCount=GetRowCount();
	m_nPrevSelectedRow=GetActiveRow();

	SetLineRange(m_nPrevSelectedRow-1L,m_nPrevRowCount);
	{
	  CClientDC dc(this);     
	  SelectTextFont(&dc);
	  GetRowDimensions(&dc,m_nRowWidth,m_nRowHeight,m_nHdrCols,m_nRowLabelWidth);
	}

	CSize sizeTotal(m_nRowWidth,1000);

	// The vertical per-page scrolling distance will be based on the
	// client area's vertical dimension and row height. It is automatically
	// maintained by the default behavior of CDBRecView.
	
	CSize sizePage(m_nRowWidth/5,0);

	// The vertical per-line scrolling distance is used here to specify
	// the height of the row, which must be positive.
	
	CSize sizeLine(m_nRowWidth/20, m_nRowHeight);
	
	m_totalDev = sizeTotal;
	m_pageDev = sizePage;
	m_lineDev = sizeLine;

	// now adjust device specific sizes
	if(m_totalDev.cy <= 0) m_totalDev.cy=1000;
	ASSERT(m_totalDev.cx>0);
	if (m_pageDev.cx == 0) m_pageDev.cx = m_totalDev.cx / 10;
	if (m_lineDev.cx == 0) m_lineDev.cx = m_pageDev.cx / 10;

	CRect rect(0,0,0,0);
	m_phdr=new CDBFHeader();
	VERIFY(m_phdr->Create(/*HDS_DRAGDROP|HDS_BUTTONS|*/CCS_TOP|WS_CHILD,rect,this,2));

	{
		CFont *pFont=CFont::FromHandle((HFONT)GetStockObject(ANSI_VAR_FONT));
		m_phdr->SetFont(pFont);
	}

	{
		HD_ITEM hd;

		hd.mask=HDI_FORMAT|HDI_TEXT|HDI_WIDTH;
		//hd.fmt=HDF_LEFT|HDF_STRING;

		for(int i=0;i<m_nHdrCols;i++) {
		  GetFieldData(i,hd.pszText,hd.fmt,hd.cxy);
		  hd.cchTextMax=strlen(hd.pszText);
		  m_phdr->InsertItem(i,&hd);
		}
	}

	UpdateBars();
	m_phdr->EnableWindow(FALSE);
}

void CDBRecView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
#ifdef _DEBUG
	ASSERT_VALID(pDC);
#endif //_DEBUG
    // by default shift viewport origin in negative direction of horizontal scroll --
    pDC->SetViewportOrg(pDC->IsPrinting()?0:-GetScrollPos(SB_HORZ),0);

	CView::OnPrepareDC(pDC, pInfo);     // For default Printing behavior
    SelectTextFont(pDC);
	GetRowDimensions(pDC,m_nRowWidth,m_nRowHeight,m_nHdrCols,m_nRowLabelWidth);
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions

int CDBRecView::GetWindowVertScroll(LONG nRows) const
{
	int shift=m_lineDev.cy;

	if(nRows>0) {
	  nRows*=shift;
	  return (DWORD)nRows>(DWORD)INT_MAX?INT_MIN:-(int)nRows;
	}
	nRows*=-shift;
	return (DWORD)nRows>(DWORD)INT_MAX?INT_MAX:(int)nRows;
}

void CDBRecView::SetLineRange(LONG lineOffset,LONG lineTotal)
{
    if(lineOffset<0L) lineOffset=0;
    ASSERT(lineTotal>=lineOffset);
    m_lineOffset=lineOffset;
    m_lineTotal=lineTotal;
}
	 
int CDBRecView::GetVertBarPos(LONG lineOffset) const
{
    if(!m_lineTotal) return 0;
 	return (int)LOWORD((lineOffset*m_totalDev.cy)/m_lineTotal);
}
	 
void CDBRecView::ScrollToRow(LONG lineOffset)
{
	ASSERT(lineOffset>=0L && lineOffset<=m_lineTotal);
	
	if(m_lineOffset!=lineOffset) {
		SetScrollPos(SB_VERT,GetVertBarPos(lineOffset));
		CRect rect;
		GetClientRect(&rect);
		rect.top+=m_nHdrHeight;
		ScrollWindow(0,GetWindowVertScroll(lineOffset-m_lineOffset),&rect,&rect);
		m_lineOffset=lineOffset;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Other helpers
void CDBRecView::FillOutsideRect(CDC* pDC, CBrush* pBrush)
{
	ASSERT_VALID(pDC);
	ASSERT_VALID(pBrush);
	
	// Fill Rect outside the image
	CRect rect;
	GetClientRect(rect);
	ASSERT(rect.left == 0 && rect.top == 0);
	rect.top+=m_nHdrHeight;
	rect.left=m_totalDev.cx;
	if(!rect.IsRectEmpty())
		pDC->FillRect(rect, pBrush);    // vertical strip along the side
		 
	ASSERT(m_lineDev.cy>0);	
    if((m_lineTotal-m_lineOffset)<=(LONG)((rect.bottom-rect.top)/m_lineDev.cy))
    {
	  rect.left = 0;
	  rect.right = m_totalDev.cx;
	  rect.top = m_nHdrHeight+(int)LOWORD(m_lineTotal-m_lineOffset)*m_lineDev.cy;
	  if(!rect.IsRectEmpty())
		pDC->FillRect(rect, pBrush);    // horizontal strip along the bottom
    }
}                                                                        

/////////////////////////////////////////////////////////////////////////////
// Tie to scrollbars and CWnd behaviour

BOOL CDBRecView::GetTrueClientSize(CSize& sizeClient, CSize& sizeSb)
    // Compute size of client assuming there were no scrollbars
    // and also the size (widths) of potential scrollbars.
	// Return TRUE if there is enough room to add both scrollbars.
{
	CRect rect;
	GetClientRect(&rect);
	ASSERT(rect.top == 0 && rect.left == 0);
	sizeClient.cx = rect.right;
	sizeClient.cy = rect.bottom;
	sizeSb.cx = sizeSb.cy = 0;
	DWORD dwStyle = GetStyle();

	// first calculate the size of a potential scrollbar.
	// scrollbar controls do NOT affect client area.
	// scrollbar controls do not get turned on/off.
	
	if (GetScrollBarCtrl(SB_VERT) == NULL)
	{
		// vert scrollbars will impact client area of this window
	    sizeSb.cx = GetSystemMetrics(SM_CXVSCROLL);
		if (dwStyle & WS_BORDER) sizeSb.cx--;
		if (dwStyle & WS_VSCROLL)
			sizeClient.cx += sizeSb.cx;  // currently on - adjust now
	}
	if (GetScrollBarCtrl(SB_HORZ) == NULL)
	{
		// horz scrollbars will impact client area of this window
	    sizeSb.cy = GetSystemMetrics(SM_CYHSCROLL);
		if (dwStyle & WS_BORDER) sizeSb.cy--;
		if (dwStyle & WS_HSCROLL) sizeClient.cy += sizeSb.cy;  // currently on - adjust now
	}

	// return TRUE if enough room
	return (sizeClient.cx > sizeSb.cx && sizeClient.cy > sizeSb.cy);
}

void CDBRecView::UpdateBars()
{
    // This function assumes that m_lineTotal, m_lineOffset, and m_totalDev
    // members have been set. Bars are drawn and positioned but the
    // client area is NOT scrolled or invalidated directly.
 
	// NOTE: turning on/off the scrollbars will cause 'OnSize' callbacks,
	// which in turn invoke this function. We must ignore those resizings.
	
	if (m_bInsideUpdate) return;         // Do not allow recursive calls

	// Lock out recursion
	m_bInsideUpdate = TRUE;

	
	ASSERT(m_totalDev.cx >= 0 && m_totalDev.cy>0);
	ASSERT(m_lineOffset>=0L && m_lineTotal>=m_lineOffset);

	CSize sizeClient; //Size of client w/o scrollbars
	CSize sizeSb;     //.cx,.cy widths of horiz and vert scrollbars, respectively

	if (!GetTrueClientSize(sizeClient, sizeSb))
	{
		// no room for scroll bars (common for zero sized elements)
		CRect rect;
		GetClientRect(&rect);
		if (rect.right > 0 && rect.bottom > 0)
		{
			// if entire client area is not invisible, assume we have
			// control over our scrollbars
			EnableScrollBarCtrl(SB_BOTH, FALSE);
		}
		m_bInsideUpdate = FALSE;
		return;
	}

	// enough room to add scrollbars --
	
    // Current scroll bar positions assuming bars are present --
   	CPoint ptMove(GetScrollPos(SB_HORZ),GetVertBarPos(m_lineOffset));

	int xRange = m_totalDev.cx - sizeClient.cx;  // > 0 => need Horiz scrollbar
	BOOL bNeedH = xRange > 0;
    BOOL bNeedHScroll = !bNeedH && ptMove.x>0;
    
	if (bNeedH) sizeClient.cy -= sizeSb.cy;      // need room for a scroll bar
    else ptMove.x=0;
      
	BOOL bNeedV = m_lineOffset > 0L || (m_lineTotal*m_lineDev.cy)>(LONG)sizeClient.cy;
	
	if (bNeedV) xRange += sizeSb.cx;
	else ptMove.y = 0;                           // jump back to origin

	if (bNeedV && !bNeedH && xRange > 0) {
	  bNeedH = TRUE;
	  bNeedHScroll=FALSE;
  	  sizeClient.cy -= sizeSb.cy;
	}
	
	// If current horiz scroll position will be past the limit, scroll to limit
	if (xRange>0 && ptMove.x>xRange) {
		ptMove.x = xRange;
	}
	else if(bNeedHScroll) {
		ScrollWindow(SetScrollPos(SB_HORZ,0),0);
	}


	// now update the bars as appropriate
	EnableScrollBarCtrl(SB_HORZ, bNeedH);
	if (bNeedH) {
	  SetScrollRange(SB_HORZ,0,xRange,FALSE);
	  SetScrollPos(SB_HORZ,ptMove.x,TRUE);
	  ResizeHdr(ptMove.x,xRange);
	}
    else ResizeHdr(ptMove.x,0);

	EnableScrollBarCtrl(SB_VERT, bNeedV);
    if (bNeedV) {
      SetScrollRange(SB_VERT,0,m_totalDev.cy,FALSE);
	  SetScrollPos(SB_VERT,ptMove.y,TRUE);
	  m_pageDev.cy=m_lineDev.cy*((sizeClient.cy-m_nHdrHeight)/m_lineDev.cy-1);
	  if(m_pageDev.cy<=0) m_pageDev.cy=m_lineDev.cy;
	}

	// Remove recursion lockout
	m_bInsideUpdate = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CDBRecView scrolling

void CDBRecView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType,cx,cy);
	if(m_phdr && cx>0 && cy>0) {
	  UpdateBars();
	}
}

void CDBRecView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int zOrig, z;   // z = x or y depending on 'nBar'
	int zMin, zMax;
	
	ASSERT(pScrollBar == GetScrollBarCtrl(SB_HORZ));    // may be null
	zOrig = z = GetScrollPos(SB_HORZ);
	GetScrollRange(SB_HORZ, &zMin, &zMax);
	ASSERT(zMin == 0);
	if (zMax <= 0)
	{
		TRACE0("Warning: no scroll range - ignoring scroll message\n");
		ASSERT(z == 0);     // must be at top
		return;
	}

	switch (nSBCode)
	{
	case SB_TOP:
		z = 0;
		break;

	case SB_BOTTOM:
		z = zMax;
		break;
		
	case SB_LINEUP:
		z -= m_lineDev.cx;
		break;

	case SB_LINEDOWN:
		z += m_lineDev.cx;
		break;

	case SB_PAGEUP:
		z -= m_pageDev.cx;
		break;

	case SB_PAGEDOWN:
		z += m_pageDev.cx;
		break;

	case SB_THUMBTRACK:
		z = nPos;
		break;

	default:        // ignore other notifications
		return;
	}

	if (z < 0) z = 0;
	else if (z > zMax) z = zMax;

	if (z != zOrig)
	{
		ScrollWindow(-(z-zOrig),0);
		SetScrollPos(SB_HORZ,z);
		UpdateWindow();
	}
}

void CDBRecView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int barPosOrig;
	LONG newOffset;
	
	ASSERT(pScrollBar == GetScrollBarCtrl(SB_VERT));    // may be null
	barPosOrig = GetScrollPos(SB_VERT);

	switch (nSBCode)
	{
	case SB_TOP:
	    newOffset=0L;
		break;

	case SB_BOTTOM:
        newOffset=m_lineTotal;
		break;
		
	case SB_LINEUP:
	    newOffset=m_lineOffset?m_lineOffset-1L:0L;
		break;

	case SB_LINEDOWN:
	    newOffset=m_lineOffset<m_lineTotal?m_lineOffset+1L:m_lineOffset;
		break;

	case SB_PAGEUP:
	    newOffset=m_lineOffset-(LONG)(m_pageDev.cy/m_lineDev.cy);
	    if(newOffset<0L) newOffset=0L;
		break;

	case SB_PAGEDOWN:
	    newOffset=m_lineOffset+(LONG)(m_pageDev.cy/m_lineDev.cy);
	    if(newOffset>m_lineTotal) newOffset=m_lineTotal;
		break;

	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:    //CScrollView uses SB_THUMBTRACK
		newOffset=(nPos*m_lineTotal)/m_totalDev.cy;
		break;

	default:        // ignore other notifications
		return;
	}

	if(newOffset!=m_lineOffset || GetVertBarPos(newOffset)!=barPosOrig)
	{
	    ScrollToRow(newOffset);
		//UpdateWindow();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDBRecView updating and drawing

CRect CDBRecView::RowWndOffsetToRect(int rowWndOffset, CDBRecHint *pHint)
{
    //Return device rectangle for a row to be invalidated --
    
	CRect rectClient;
	GetClientRect(&rectClient);
	
	rectClient.top=m_nHdrHeight+rowWndOffset*m_nRowHeight;
	rectClient.bottom=rectClient.top+m_nRowHeight;
	//Invalidate row label only if just a selection change --
	if(!pHint->m_recUpdated) rectClient.right=m_nRowLabelWidth;
	return rectClient;
}

void CDBRecView::RectToRowWndOffsets(const CRect& rect,
			int& nFirstRow, int& nLastRow,
			BOOL bIncludePartiallyShownRows)
{
	int nRounding = bIncludePartiallyShownRows? 0 : (m_nRowHeight - 1);
	
	nFirstRow = (rect.top-m_nHdrHeight+nRounding)/m_nRowHeight;
	nLastRow = (rect.bottom-m_nHdrHeight-nRounding)/m_nRowHeight;
	if(m_lineTotal<=m_lineOffset+(LONG)nLastRow)
	   nLastRow=(int)LOWORD(m_lineTotal-m_lineOffset)-1;
}

void CDBRecView::UpdateRow(LONG nInvalidRow,CDBRecHint *pHint)
{
	
	// If the number of rows has changed, adjust the scrollbars.
	
	LONG nRowCount=GetRowCount();
	if(nRowCount!=m_lineTotal) {
		m_lineTotal=nRowCount;
		UpdateBars();
    }
    
	// Determine the range of the rows that are currently fully visible
	// in the window.
	
	int nFirstRow, nLastRow;
	CRect rectClient;
	GetClientRect(&rectClient);
	rectClient.top+=m_nHdrHeight;
	RectToRowWndOffsets(rectClient,nFirstRow,nLastRow, FALSE); //nFirstRow=0
 
	// If the newly selected row is below those currently visible
	// in the window.  Scroll so the newly selected row is visible.

	int nInvalidOffset;
 	if(nInvalidRow <= m_lineOffset) {
 	   ScrollToRow(nInvalidRow-1);
 	   nInvalidOffset=0;
 	}
	else if(nInvalidRow > m_lineOffset+nLastRow+1) {
 	   ScrollToRow(nInvalidRow-(nLastRow-nFirstRow)-1);
 	   nInvalidOffset=nLastRow;
 	}
 	else nInvalidOffset=(int)LOWORD(nInvalidRow-m_lineOffset)-1;

    // Invalidate rectangle of the updated row --
	{
	   CRect rectInvalid=RowWndOffsetToRect(nInvalidOffset,pHint);
	   InvalidateRect(&rectInvalid);
	}

	// If the selected row has changed, give the derived class an opportunity
	// to repaint the previously selected row, perhaps to un-highlight it.

	LONG nSelectedRow=GetActiveRow();
	if (m_nPrevSelectedRow!=nSelectedRow)
	{
		if(m_lineOffset<m_nPrevSelectedRow &&
		   m_lineOffset+LONG(nLastRow+1)>=m_nPrevSelectedRow)
		{
		  CRect rectOldSelection =
		    RowWndOffsetToRect((int)LOWORD(m_nPrevSelectedRow-m_lineOffset-1),pHint);
		  InvalidateRect(&rectOldSelection);
		}
		m_nPrevSelectedRow=nSelectedRow;
	}
}

void CDBRecView::OnUpdate(CView*,LPARAM lHint,CObject* pHint)
{
	// OnUpdate() is called whenever the document has changed and,
	// therefore, the view needs to redisplay some or all of itself.

	if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CDBRecHint)))
	{
		UpdateRow((LONG)lHint,(CDBRecHint *)pHint);
	}
	else
	{
		Invalidate();
	}
}

void CDBRecView::OnDraw(CDC* pDC)
{
	if (GetRowCount()==0) return;

	// The window has been invalidated and needs to be repainted;
	// or a page needs to be printed (or previewed).
	// First, determine the range of rows that need to be displayed or
	// printed.

	int nFirstOffset, nLastOffset;
	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	pDC->LPtoDP(&rectClip);
	if(rectClip.top<m_nHdrHeight) {
	  if((rectClip.top=m_nHdrHeight)>=rectClip.bottom) return;
	}

	// Draw each row in the invalidated region of the window,
	// or on the printed (previewed) page.
	RectToRowWndOffsets(rectClip, nFirstOffset, nLastOffset, TRUE);
	LONG nActiveRow=GetActiveRow();
	LONG nRow=m_lineOffset+nFirstOffset;
	for(int y=m_nRowHeight*nFirstOffset+m_nHdrHeight;
	      nFirstOffset<=nLastOffset;
	      nFirstOffset++,y+=m_nRowHeight)
	    {
	    nRow++;
		OnDrawRow(pDC,nRow,y,nRow==nActiveRow);
		}
}

/////////////////////////////////////////////////////////////////////////////
// CDBRecView commands

void CDBRecView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
		case VK_HOME:
			ActivateRow(1L);
			break;
		case VK_END:
			ActivateRow(m_lineTotal);
			break;
		case VK_UP:
			ActivateNextRow(FALSE);
			break;
		case VK_DOWN:
			ActivateNextRow(TRUE);
			break;
		case VK_PRIOR:
			OnVScroll(SB_PAGEUP,
				0,  // nPos not used for PAGEUP
				GetScrollBarCtrl(SB_VERT));
			break;
		case VK_NEXT:
			OnVScroll(SB_PAGEDOWN,
				0,  // nPos not used for PAGEDOWN
				GetScrollBarCtrl(SB_VERT));
			break;
		default:
			CView::OnKeyDown(nChar, nRepCnt, nFlags);
	}
}

BOOL CDBRecView::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
}

void CDBRecView::OnLButtonDown(UINT, CPoint point)
{
    if(point.y>m_nHdrHeight) {
      LONG nRow=m_lineOffset+(1+(point.y-m_nHdrHeight)/m_nRowHeight);
	  if(nRow <= m_lineTotal) ActivateRow(nRow);
	}
}


BOOL CDBRecView::OnNotify(WPARAM wParam,LPARAM lParam,LRESULT* pResult)
{
	HD_NOTIFY *pHdr=(HD_NOTIFY *)lParam;
	int idx;

	ASSERT(pHdr->hdr.idFrom==2);
	switch(pHdr->hdr.code) {
		case HDN_BEGINTRACK		:	{
									  idx=pHdr->iItem;
								      break;
									}
		case HDN_DIVIDERDBLCLICK:	{
									  //ASSERT(FALSE);
								      break;
									}
		case HDN_ENDTRACK		:	{
									  //ASSERT(FALSE);
								      break;
									}
		case HDN_ITEMCHANGED	:	{
									  //ASSERT(FALSE);
								      break;
									}
		case HDN_ITEMCHANGING	:	{
									  //ASSERT(FALSE);
								      break;
									}
		case HDN_ITEMCLICK		:	{
									  //ASSERT(FALSE);
								      break;
									}
		case HDN_TRACK			:	{
									  //ASSERT(FALSE);
								      break;
									}
	}

	return CView::OnNotify(wParam,lParam,pResult);
}

/////////////////////////////////////////////////////////////////////////////
// CDBRecView diagnostics

#ifdef _DEBUG
CDBRecDoc* CDBRecView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDBRecDoc)));
	return (CDBRecDoc*) m_pDocument;
}

void CDBRecView::Dump(CDumpContext& dc) const
{
	ASSERT_VALID(this);

	CView::Dump(dc);

	AFX_DUMP1(dc, "\nm_totalDev = ", m_totalDev);
	AFX_DUMP1(dc, "\nm_lineTotal = ", m_lineTotal);
	AFX_DUMP1(dc, "\nm_lineOffset = ", m_lineOffset);
	AFX_DUMP1(dc, "\nm_pageDev = ", m_pageDev);
	AFX_DUMP1(dc, "\nm_lineDev = ", m_lineDev);
	AFX_DUMP1(dc, "\nm_bInsideUpdate = ", m_bInsideUpdate);
	AFX_DUMP1(dc, "\nm_nPrevSelectedRow = ", m_nPrevSelectedRow);
	AFX_DUMP1(dc, "\nm_nPrevRowCount = ", m_nPrevRowCount);
	AFX_DUMP1(dc, "\nm_nRowWidth = ", m_nRowWidth);
	AFX_DUMP1(dc, "\nm_nHdrHeight = ", m_nHdrHeight);
}

#endif //_DEBUG

