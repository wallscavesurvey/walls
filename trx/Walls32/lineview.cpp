// lineview.cpp : implementation of the CLineView class

#include "stdafx.h"
#include "walls.h"
#include "linedoc.h"
#include "lineview.h"
#include "wedview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CLineView,CView)

/////////////////////////////////////////////////////////////////////////////
// CLineView

BEGIN_MESSAGE_MAP(CLineView,CView)
    //{{AFX_MSG_MAP(CLineView)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_COMMAND(ID_EDIT_DELETE,OnEditDelete)
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLineView construction, initialization, and destruction


CBrush CLineView::m_cbBackground;
CBrush CLineView::m_cbHighlight;
CBrush CLineView::m_cbFlag;
COLORREF CLineView::m_TextColor;

LINENO CLineView::m_nLineStart=0;
//LINENO CLineView::m_nLineStart2=0;
int CLineView::m_nCharStart=0;

CLineView::CLineView()
{
	// Init everything to zero (except for base class, CView) --
	// AFX_ZERO_INIT_OBJECT(CView);
	m_nLineOffset=m_nLineTotal=m_nActiveLine=m_nSelectLine=m_nFlagLine=0;
	//m_nFlagLine2=0;
	m_bCaretVisible=m_bOwnCaret=false;
	m_caretX=m_caretY=0;
	m_rangeX=m_scrollX=m_nActiveChr=m_nSelectChr=m_bSelectMode=m_bInsideUpdate=0;
	m_iSelectX=m_iSelectY=m_lastCaretX=m_nMargin=m_nPageHorz=m_nPageVert=m_nLineHorz=0;
	m_nLineMinWidth=m_nLineMaxWidth=m_nLineHeight=m_nHdrHeight=0;
	m_hWndParent=NULL;
}

CLineView::~CLineView()
{
}

void CLineView::OnInitialUpdate()
{
//	CView::OnInitialUpdate();
    m_hWndParent=::GetParent(m_hWnd);
    m_nLineTotal=GetLineCount();
    ASSERT(m_nLineTotal);
    if(m_nLineStart) {
	  if(m_nLineStart>m_nLineTotal) {
			m_nLineStart=1;
			m_nCharStart=0;
			m_nFlagLine=0;
			//m_nFlagLine2=0;
	  }
	  else {
			m_nFlagLine=m_nLineStart;
			//m_nFlagLine2=m_nLineStart2;
	  }
      m_nSelectLine=m_nActiveLine=m_nLineStart;
      m_nLineOffset=m_nLineStart-START_PREFIX;
      if(m_nLineOffset<1) m_nLineOffset=1;
      m_nSelectChr=m_nActiveChr=GetPosition(m_nActiveLine,m_nCharStart,m_caretX);
      m_nLineStart=0;
	  //m_nLineStart2=0;
    }
    else {
      m_caretX=0;
      m_nSelectChr=m_nActiveChr=0;
      m_nFlagLine=0;
      m_nSelectLine=m_nLineOffset=m_nActiveLine=1;
    }
    m_bCaretVisible=true;
	m_bSelectMode=FALSE;
	CalculateLineMetrics();
	m_caretY=LineClientY(m_nActiveLine);
	UpdateBars();
}

void CLineView::SetFlagLine(LINENO line)
{
  if(m_nFlagLine!=line) {
    InvalidateLine(m_nFlagLine,0);
    m_nFlagLine=line;
    if(line) InvalidateLine(line,0);
  }
  /*
  if(m_nFlagLine2!=line2) {
    InvalidateLine(m_nFlagLine2,0);
    m_nFlagLine2=line2;
    if(line2) InvalidateLine(line2,0);
  }
  */
}

void CLineView::ScrollToError()
{
  if(m_nLineTotal>=m_nLineStart) {
	//BUG: Was m_nLineTotal>START_PREFIX!! (3/13/05)
	//if(m_nLineTotal>START_PREFIX) ScrollToDevicePosition(0,m_nLineStart-START_PREFIX);
    int top=m_nLineStart-START_PREFIX;
	ScrollToDevicePosition(0,(top>0)?top:1);
	SetPosition(m_nLineStart,m_nCharStart,POS_SCROLL);
	SetFlagLine(m_nLineStart); //,m_nLineStart2);
  }
  m_nLineStart=0;
  //m_nLineStart2=0;
}

void CLineView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
    //Shift viewport origin in negative direction of horizontal scroll --
    pDC->SetViewportOrg(pDC->IsPrinting()?0:-GetScrollPos(SB_HORZ),0);
	CView::OnPrepareDC(pDC, pInfo);     // For default Printing behavior
    SelectTextFont(pDC);
   	CalculateLineMetrics();
}

/////////////////////////////////////////////////////////////
// Helper functions

int CLineView::GetWindowVertScroll(LINENO nLines) const
{
    DWORD lshift=m_nLineHeight;

	if(nLines>0) {
      lshift*=nLines;
      return lshift>(DWORD)INT_MAX?INT_MIN:-(int)LOWORD(lshift);
	}
    lshift*=-nLines;
    return lshift>(DWORD)INT_MAX?INT_MAX:(int)LOWORD(lshift);
}

int CLineView::GetVertBarPos(LINENO lineOffset) const
{
    if(m_nLineTotal<=1) return 0;
    return (int)(((DWORD)(lineOffset-1)*SB_VERT_RANGE)/(m_nLineTotal-1));
}
	 
void CLineView::ScrollToDevicePosition(int xPos,LINENO lineOffset)
{
#ifdef DEBUG
	if(!lineOffset || lineOffset>m_nLineTotal) {
		ASSERT(FALSE);
	}
#endif
			  
    ASSERT(lineOffset && lineOffset<=m_nLineTotal);
    ASSERT(xPos<=m_rangeX && xPos>=0);

	CRect rect;
	GetEditRect(&rect);
    
	//This does not force an UpdateWindow(), but may send WM_PAINT --
    ScrollWindow(m_scrollX-xPos,GetWindowVertScroll(lineOffset-m_nLineOffset),0,&rect);
	//Update the scrollbars --		  
	SetScrollPos(SB_HORZ,m_scrollX=xPos);
    SetScrollPos(SB_VERT,GetVertBarPos(m_nLineOffset=lineOffset));
    m_caretY=LineClientY(m_nActiveLine);
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// Tie to scrollbars and CWnd behaviour

BOOL CLineView::GetTrueClientSize(CSize& sizeClient, CSize& sizeSb)
    // Compute size of client assuming the scrollbars, if present, were removed.
    // Also get the size (widths) of potential scrollbars.
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
		// Style-produced vert scrollbars may be present
	    sizeSb.cx = GetSystemMetrics(SM_CXVSCROLL);
		if (dwStyle & WS_BORDER) sizeSb.cx--;
		if (dwStyle & WS_VSCROLL) sizeClient.cx += sizeSb.cx;  // currently on - adjust now
	}
	
	if (GetScrollBarCtrl(SB_HORZ) == NULL)
	{
		// Style produced horz scrollbars may be present
	    sizeSb.cy = GetSystemMetrics(SM_CYHSCROLL);
		if (dwStyle & WS_BORDER) sizeSb.cy--;
		if (dwStyle & WS_HSCROLL) sizeClient.cy += sizeSb.cy;  // currently on - adjust now
	}

	// return TRUE if enough room
	return (sizeClient.cx>sizeSb.cx && sizeClient.cy>sizeSb.cy);
}

void CLineView::UpdateBars()
{
    // This function assumes that m_nLineTotal, m_nLineOffset
    // members have been set. Bars are drawn and positioned but the
    // client area is NOT scrolled or invalidated directly.
    // This function also initializes m_scrollX and xRangeX.
 
	// NOTE: turning on/off the scrollbars will cause 'OnSize' callbacks,
	// which in turn invoke this function. We must ignore those resizings.
	
	if (m_bInsideUpdate) return;         // Do not allow recursive calls

	// Lock out recursion
	m_bInsideUpdate = TRUE;

	
	ASSERT(m_nLineMaxWidth>0);
    ASSERT(m_nLineOffset && m_nLineTotal>=m_nLineOffset);

	CSize sizeClient; //Size of client w/o scrollbars
	CSize sizeSb;     //.cx,.cy widths of horiz and vert scrollbars, respectively

	if (!GetTrueClientSize(sizeClient, sizeSb))
	{
		// no room for both types of scroll bars 
		CRect rect;
		GetClientRect(&rect);
		if (rect.right > 0 && rect.bottom > 0)
		{
			// if entire client area is not invisible, assume we have
			// control over our scrollbars
			EnableScrollBarCtrl(SB_BOTH,FALSE);
		}
		m_bInsideUpdate = FALSE;
		m_scrollX=m_rangeX=0;
		return;
	}

	// enough room to add scrollbars --
	
    // Current scroll bar positions assuming bars are present --
    CPoint ptMove(GetScrollPos(SB_HORZ),GetVertBarPos(m_nLineOffset));

	int xRange = m_nLineMaxWidth-sizeClient.cx;
	BOOL bNeedHBars = xRange>0;
    BOOL bNeedHScroll = !bNeedHBars && ptMove.x>0;
    
	if (bNeedHBars) sizeClient.cy-=sizeSb.cy;      // need room for a scroll bar
    else ptMove.x=0;
      
    BOOL bNeedVBars=m_nLineOffset>1 ||
          ((DWORD)m_nLineTotal*m_nLineHeight)>(DWORD)sizeClient.cy;
	
	if (bNeedVBars) {
	  xRange+=sizeSb.cx;
	  sizeClient.cx-=sizeSb.cx;
	}
	else ptMove.y=0;                           // jump back to origin

	if (bNeedVBars && !bNeedHBars && xRange>0) {
	  bNeedHBars=TRUE;
	  bNeedHScroll=FALSE;
  	  sizeClient.cy-=sizeSb.cy;
	}
	
	if (bNeedHBars) {
	  //If current horiz scroll position will be past the limit, scroll to limit
	  if(ptMove.x>xRange) ptMove.x=xRange;
	}
	else {
	  //If removing bars, scroll to 0 first --
	  if(bNeedHScroll) ScrollWindow(SetScrollPos(SB_HORZ,0),0);
	  m_scrollX=0;
	}
	
	// now update the bars as appropriate
	EnableScrollBarCtrl(SB_HORZ,bNeedHBars);
	
	if (bNeedHBars) {
	  SetScrollRange(SB_HORZ,0,m_rangeX=xRange,FALSE);
	  SetScrollPos(SB_HORZ,m_scrollX=ptMove.x,TRUE);
      m_nPageHorz=(sizeClient.cx/m_nLineMinWidth)*m_nLineMinWidth;
      if(m_nPageHorz<m_nLineMinWidth) m_nPageHorz=m_nLineMinWidth;
	  m_nLineHorz=m_nPageHorz/5;
      if(m_nLineHorz<m_nLineMinWidth) m_nLineHorz=m_nLineMinWidth;
	}
	else m_rangeX=m_scrollX=m_nLineHorz=m_nPageHorz=0;

	EnableScrollBarCtrl(SB_VERT,bNeedVBars);
    if (bNeedVBars) {
      SetScrollRange(SB_VERT,0,SB_VERT_RANGE,FALSE);
	  SetScrollPos(SB_VERT,ptMove.y,TRUE);
	  m_nPageVert=m_nLineHeight*((sizeClient.cy-m_nHdrHeight)/m_nLineHeight-1);
	  if(m_nPageVert<m_nLineHeight) m_nPageVert=m_nLineHeight;
	}
	// Remove recursion lockout
	m_bInsideUpdate = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CLineView scrolling

void CLineView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType,cx,cy);
    //OnInitialUpdate() sets m_nLineTotal
    if(m_nLineTotal>0) UpdateBars();
}

void CLineView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int z=m_scrollX;
	
	ASSERT(pScrollBar==GetScrollBarCtrl(SB_HORZ));    // may be null
	ASSERT(m_scrollX==GetScrollPos(SB_HORZ));
	
	#ifdef _DEBUG
	  int zMax,zMin;
	  GetScrollRange(SB_HORZ, &zMin, &zMax);
	  ASSERT(zMin == 0 && zMax==m_rangeX && zMax>0);
    #endif
    
    if(m_rangeX<=0) return;
    
	switch (nSBCode)
	{
	case SB_TOP:
		z = 0;
		break;

	case SB_BOTTOM:
		z = m_rangeX;
		break;
		
	case SB_LINEUP:
		z -= m_nLineHorz;
		break;

	case SB_LINEDOWN:
		z += m_nLineHorz;
		break;

	case SB_PAGEUP:
		z -= m_nPageHorz;
		break;

	case SB_PAGEDOWN:
		z += m_nPageHorz;
		break;

	case SB_THUMBTRACK:
		z = nPos;
		break;

	default:        // ignore other notifications
		return;
	}

	if (z<0) z=0;
	else if (z>m_rangeX) z=m_rangeX;

	if (z!=m_scrollX)
	{
		ScrollWindow(m_scrollX-z,0);
		SetScrollPos(SB_HORZ,m_scrollX=z);
		UpdateWindow();
	}
}

void CLineView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
        LINENO newOffset;
	
	switch (nSBCode)
	{
	case SB_TOP:
            newOffset=1;
		break;

	case SB_BOTTOM:
        newOffset=m_nLineTotal;
		break;
		
	case SB_LINEUP:
            newOffset=m_nLineOffset>1?m_nLineOffset-1:1;
		break;

	case SB_LINEDOWN:
            newOffset=(m_nLineOffset<m_nLineTotal)?m_nLineOffset+1:m_nLineTotal;
		break;

	case SB_PAGEUP:
            newOffset=m_nLineOffset-(LINENO)(m_nPageVert/m_nLineHeight);
            if(newOffset<1) newOffset=1;
		break;

	case SB_PAGEDOWN:
            newOffset=m_nLineOffset+(LINENO)(m_nPageVert/m_nLineHeight);
            if(newOffset>m_nLineTotal) newOffset=m_nLineTotal;
		break;

	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:    //CScrollView uses SB_THUMBTRACK
                newOffset=(LINENO)(((DWORD)m_nLineTotal*nPos)/SB_VERT_RANGE);
                if(newOffset<1) newOffset=1;
		break;

	default:        // ignore other notifications
		return;
	}

    if(newOffset!=m_nLineOffset) ScrollToDevicePosition(m_scrollX,newOffset);
}

/////////////////////////////////////////////////////////////////////////////
// CLineView updating and drawing

void CLineView::RectToLineOffsets(const CRect& rect,
			int& nFirstLine, int& nLastLine,
			BOOL bIncludePartial)
{
    //Get 0-based window line numbers for lines either wholly or partially
    //covered by the rectangle rect. Note that the bottom border of the
    //rectangle is not considered to be covering a line.
	//
	//  if(bIncludePartialLines) {
	//    fst=top/h;
	//    lst=(bot-1)/h;
	//  }
	//  else {
	//    fst=(top+(h-1))/h;
	//    lst=((bot-1)-(h-1))/h;
	//  }
    //
    
	int nRounding = bIncludePartial? 0 : (m_nLineHeight-1);
	nFirstLine = (rect.top-m_nHdrHeight+nRounding)/m_nLineHeight;
	nLastLine = (rect.bottom-m_nHdrHeight-1-nRounding)/m_nLineHeight;
	
    if(m_nLineTotal<m_nLineOffset+(LINENO)nLastLine)
           nLastLine=(int)LOWORD(m_nLineTotal-m_nLineOffset);
}

void CLineView::GetEditRectLastLine(CRect *pRect,LINENO& nLastLine,BOOL bIncludePartial)
{
    int nLast;
    CRect rect;
	
    if(!pRect) pRect=&rect;
	GetEditRect(pRect);
	nLast=pRect->Height()-1;
	if(!bIncludePartial) nLast-=(m_nLineHeight-1);
	nLast/=m_nLineHeight;
	nLastLine=m_nLineOffset+nLast;
	if(nLastLine>m_nLineTotal) nLastLine=m_nLineTotal;
}
	
BOOL CLineView::GetSelectedString(LPSTR &pStart,int &nLen,BOOL bWholeWords/*=FALSE*/)
{
    //If the selection is either one complete line or part of a line,
    //obtain its pointer and length (excluding EOL). Return TRUE
    //if the length is nonzero.
    
	CLineRange range(m_nActiveLine,m_nActiveChr,m_nSelectLine,m_nSelectChr);
	if(CLineDoc::NormalizeRange(range) && range.line0==range.line1) {
	  pStart=(LPSTR)LinePtr(range.line0);
	  int linlen=*(LPBYTE)pStart;
	  pStart+=range.chr0+1;
	  if(range.chr1>linlen) range.chr1=linlen;
	  nLen=range.chr1-range.chr0;
	  if(nLen && bWholeWords) {
	    if((range.chr0 && !IsWordDelim(pStart[-1])) ||
	     (range.chr1<linlen && !IsWordDelim(pStart[nLen]))) return FALSE;
	  }  
	}
	else nLen=0;
	return nLen>0;
}

BOOL CLineView::GetSelectedChrs(LINENO nLine,int &nChr0,int &nChr1)
{
   if(!IsSelectionCleared()) {
     int chr0=m_nSelectChr;
     int chr1=m_nActiveChr;
     LINENO line0=m_nSelectLine;
     LINENO line1=m_nActiveLine;
     
     if(CLineDoc::NormalizeRange(line0,chr0,line1,chr1) &&
         nLine>=line0 && nLine<=line1) {
       nChr0=(nLine==line0)?chr0:0;
       nChr1=(nLine==line1)?chr1:INT_MAX;
       return nChr1>0;
     }
   }
   return FALSE; 
}

static BOOL IsStartOfWord(LPBYTE p)
{
  return CLineView::IsWS(*(p-1)) && !CLineView::IsWS(*p);
}

int CLineView::GetStartOfWord(LPBYTE pLine,int& nChr,int iDir)
{
  int len=*pLine++;
  int i;

  ASSERT(nChr>=0);
    
  if(nChr>len) nChr=len;
  if(iDir>0) {
    if(nChr>0) {
      for(i=nChr;i<len;i++) if(IsStartOfWord(pLine+i)) break;
    }
    else i=0; 
  }
  else {
    if(nChr<len) {
      for(i=nChr;i;i--) if(IsStartOfWord(pLine+i)) break;
    }
    else i=len;
  }
  i-=nChr;
  nChr+=i;
  return i;
}

#ifdef _ADJ_WHOLE
//Removed 7/8/95 because I found it more often annoying than useful --
void CLineView::AdjustForWholeWords(LINENO nLine,int& nChr,int& xPos)
{
   // Adjust for word granularity provided the new selection as defined by the
   // proposed new offset and horiz caret position (nLine,nChr,xPos) would
   // otherwise cover at least the first character of a word and some preceeding
   // whitespace. A "word" as we define it includes its trailing whitespace
   // (but not the EOL). This function also advances the trailing end of the
   // selection if this ending line would contain no selected data.
   
   LPBYTE pLine=LinePtr(nLine);
   int chrBase=m_nSelectChr;
   int iDir=(m_nSelectLine>nLine||(chrBase>nChr && m_nSelectLine==nLine))?-1:1;
   BOOL bMultiline=nLine!=m_nSelectLine;

   if(bMultiline && (nLine+iDir)==m_nSelectLine) {
     //Advance the trailing end of the block one position if nothing would be
     //selected in its "last" line. This does not require a screen refresh --
     if(iDir<0 && !chrBase ||
        iDir>0 && chrBase>=*LinePtr(m_nSelectLine)) {
        m_nSelectLine+=iDir;
        m_nSelectChr=chrBase=iDir<0?*LinePtr(m_nSelectLine):0;
        bMultiline=nLine!=m_nSelectLine;
     }
   }
    
   //We don't adjust for whole words if the selection would cover just part of
   //a single word, or if the selection has been nullified --
   
   if(!bMultiline) {
      int chr1,chr2;
      if(iDir>0) {
        chr1=chrBase+2;
        chr2=nChr;
      }
      else {
        chr1=nChr+2;
        chr2=chrBase;
      }
      for(;chr1<=chr2;chr1++) if(IsStartOfWord(pLine+chr1)) break;
      if(chr1>chr2) return;
   }
   
   //Adjust the leading edge as required. This also moves the proposed
   //caret position --                    
   if(nChr && GetStartOfWord(pLine,nChr,iDir)) nChr=GetPosition(nLine,nChr,xPos);
   
   //Adjust the trailing edge, which can cause a need for screen repainting
   //that would not be accomplished otherwise --
   if(bMultiline) pLine=LinePtr(nLine=m_nSelectLine);
   if(GetStartOfWord(pLine,chrBase,-iDir)) {
     InvalidateLine(nLine,chrBase>m_nSelectChr?m_nSelectChr:chrBase);
     m_nSelectChr=chrBase;
   }
}                   
#endif
        
void CLineView::ScrollToActivePosition()
{
    // Scroll to the insertion point (m_nActiveLine,m_nActiveChr)
    // so that it is within view.
    
	// Determine 0-based offset of last completely displayed line --
	CRect rect;
	LINENO nLastLine;
	
	GetEditRectLastLine(&rect,nLastLine,FALSE);
	
	// Calculate a new horizontal scroll position, xPos --
	
	int xPos=m_caretX-m_scrollX;  //client-X position of caret
	if(xPos>=rect.right) xPos=xPos-rect.right+m_nLineHorz-1;
	else if(xPos<0) xPos-=m_nLineHorz-1;
	else xPos=0;
	if(xPos) {
	  xPos=m_scrollX+m_nLineHorz*(xPos/m_nLineHorz);
	  if(xPos<0) xPos=0;
      else if(xPos>m_rangeX) xPos=m_rangeX;
    }
    else xPos=m_scrollX;
 
	// Also, if the revised or selected line is below those currently visible
	// in the window, we will scroll so the line is the last line fully visible.

    LINENO nNewOffset=m_nLineOffset;
 	if(m_nActiveLine<nNewOffset) nNewOffset=m_nActiveLine;
	else if(m_nActiveLine>nLastLine) nNewOffset+=m_nActiveLine-nLastLine;

	if(nNewOffset==m_nLineTotal && m_nLineTotal>1) nNewOffset--;
	   
    //Scroll as necessary (this sets m_scrollX, m_scrollY and m_nLineOffset) --
    if(xPos!=m_scrollX || nNewOffset!=m_nLineOffset) {
      ASSERT(xPos<=m_rangeX && xPos>=0);
 	  ScrollToDevicePosition(xPos,nNewOffset);
 	}
}

BOOL CLineView::SetPosition(LINENO nLine,int iPos,
				UINT iTyp/*=POS_CHR*/,BOOL bSelect/*=FALSE*/)
{
   // Set new active position (m_nActiveLine, m_nActiveChr,
   // m_caretX, and m_caretY) to <nLine,iPos> where iPos is the proposed line
   // character offset, or to the position nearest client-relative
   // X-position, iPos, if (iTyp&POS_NEAREST)!=0. If bSelect==TRUE the current
   // selection, if any, is extended with the new position, otherwise it is
   // cleared. If (iTyp&POS_ADJUST)!=0, adjust the new position so that the
   // selection covers whole words (WinWord style). Finally, unless
   // iTyp&POS_NOTRACK, the position is scrolled into view if necessary, which
   // in turn may force an UpdateWindow(). In any case, parts of the window
   // may get invalidated due to changes in the selection.

   int nChr;
   long xPos;
   
   if(nLine<1) nLine=1;
   else if(nLine>m_nLineTotal) nLine=m_nLineTotal;
   if(iTyp&POS_NEAREST) {
     //Determine <nChr,xPos> nearest client-relative X position, iPos --
     if((xPos=iPos)==-1) {
       if(!m_lastCaretX) m_lastCaretX=m_caretX;
       xPos=m_lastCaretX;
     }
     else {
       m_lastCaretX=0;
       xPos-=Margin();
       if(xPos<0) xPos=0;
     }
     GetNearestPosition(nLine,nChr,xPos);
   }
   else {
     //Determine line-relative xPos corresponding to char position iPos --
     nChr=GetPosition(nLine,iPos,xPos);
     m_lastCaretX=0;
   }
   
   //Adjust selection for word granularity (bSelect==TRUE) --
   if(iTyp&POS_ADJUST) {
     bSelect=TRUE;
     #ifdef _ADJ_WHOLE
     AdjustForWholeWords(nLine,nChr,xPos);
     #endif
   }
   
   if(nChr!=m_nActiveChr || xPos!=m_caretX || nLine!=m_nActiveLine ||
      (iTyp&POS_SCROLL) || !bSelect && !IsSelectionCleared()) {
      //POS_SCROLL implies either the font or m_nLineOffset may have changed --
      if(bSelect) {
        InvalidateSelection(nLine,nChr,m_nActiveLine,m_nActiveChr);
        m_bCaretVisible=(nLine==m_nSelectLine && nChr==m_nSelectChr);
      }
      else {
        if(!IsSelectionCleared()) 
          InvalidateSelection(m_nSelectLine,m_nSelectChr,m_nActiveLine,m_nActiveChr);
        m_nSelectLine=nLine;
        m_nSelectChr=nChr;
        m_bCaretVisible=true;
      }
      m_nActiveLine=nLine;
      m_nActiveChr=nChr;
      m_caretX=xPos;
	  if(m_nLineOffset>m_nLineTotal) m_nLineOffset=m_nLineTotal; /*** Missing prior 1/7/05 -- BUG FIXED 3/10/05 (was nLine)*/
      m_caretY=LineClientY(nLine); //depends on m_nLineOffset
      if(!(iTyp&POS_NOTRACK)) {
        //Scrolls so insertion point is in view and calls PositionCaret()
        ScrollToActivePosition();
      }
      PositionCaret();
      return TRUE;
    }
    return FALSE;
}

void CLineView::UpdateDoc(UINT uUpd,LPSTR pChar,int nLen)
{ 
    //Used only by derived class --
    CLineRange range(m_nSelectLine,m_nSelectChr,m_nActiveLine,m_nActiveChr);
    GetDocument()->Update(uUpd,pChar,nLen,range);
}
    
void CLineView::RefreshText(UINT upd,LINENO line0,int chr0,LINENO line1,int chr1)
{
  //A portion of the document has just been updated. The screen must be refreshed
  //to reflect an update of type upd. The insertion point must also be adjusted.
  //
  //Arcane Issue No.1239 --
  //There seems to be no standard way to handle text edits when different windows
  //into the same file are visible. For example, the MS Visual Workbench doesn't
  //adjust the file offsets defining the selections and insertion points for inactive
  //views when text above these insertion points are deleted. This is surely
  //undesirable since it changes both the caret position with respect to text and
  //the identity of any highlighted selections. Like WinWord, we will clear any
  //selections in inactive views while preserving the logical insertion points.
  //(If affected by a deletion, we move them to the next point following the
  //deletion.)
	
  LINENO nUpdLines=line1-line0;
  if(chr1==INT_MAX) nUpdLines++;
  
  //At this point the view's m_nActiveLine/Chr, m_nLineTotal, m_nLineOffset,
  //and window's invalidated region reflect the state before the deletion, replacement,
  //or insertion. We update these items in the following  block. Updating the
  //scroll bars, if required, and scrolling so the adjusted position is visible
  //in the active view is deferred to UPD_SCROLL below.
  
  if(upd&CLineDoc::UPD_INSERT) {
  
      //line0 and line1 are the first and last numbers of document lines
      //containing new data from this insertion. (A line's "data" includes the
      //implied EOL.)
      
      //nUpdLines==total number of lines (EOL's) inserted. chr1==INT_MAX
      //if the last char inserted was the implied EOL. For example, when
      //the user presses the RETURN key, we should have line0==line1 and
      //chr1==INT_MAX, with chr0 original chars remaining on line0 and
      //<original length>-chr0 chars placed before the original EOL on a
      //new line at line0+1.
      
      //Update line count before SetPosition() and InvalidateText() --
      m_nLineTotal+=nUpdLines;
      ASSERT(m_nLineTotal==GetLineCount());
      
      //Next, reposition insertion point if required --
      
      //When not globally replacing this is the local view's position --
      //ASSERT(!m_bOwnCaret || m_nActiveLine==line0 && m_nActiveChr==chr0 ||
      //  !chr0 && m_nActiveLine==line0-1);

      //Revise line number of first displayed line and invalidate the changed region.
      //Note that without "!m_bOwnCaret" in the commented out part the RETURN key would
      //not cause a scroll of the active window if the cursor was at the top left
      //corner of the window, in which case the only visual effect would be
      //a line number change on the status bar! We might want to avoid the scroll
      //only for inactive (!m_bOwnCaret) windows (if at all!). I prefer to simplify
      //the code --
      
      if(m_nLineOffset>line0)
        //|| !bOwnCaret && m_nLineOffset==line0 && chr0==0 && chr1==INT_MAX)
        //Only the line numbers would change --
        m_nLineOffset+=nUpdLines;
      else if(!nUpdLines)
        //Only one line is changed (it may be out of view) --
        InvalidateLine(line0,chr0);
      else
        //Invalidate all lines >=line0 that are in view -- 
        InvalidateText(line0,chr0,LINENO_MAX,INT_MAX);
        
      //Change the insertion point to follow the data it was originally
      //pointing to --
      if(m_nActiveLine>line0) m_nSelectLine=m_nActiveLine+=nUpdLines;
      else if(m_nActiveLine==line0 && m_nActiveChr>=chr0) {
          if(chr1<INT_MAX) SetPosition(line1,m_nActiveChr+chr1-chr0,POS_CHR|POS_NOTRACK);
          else {
            //If chr1==INT_MAX, the original data after the insertion point begins
            //on a new line. CLineDoc's memory version of the file always contains
            //a trailing EOL *before which* all insertions are made, hence line1+1
            //exists --
            SetPosition(line1+1,m_nActiveChr-chr0,POS_CHR|POS_NOTRACK);
          }
      }
  } //UPD_INSERT
  
  else if(upd&CLineDoc::UPD_DELETE) {
      
      //Revise line number of first displayed line --
      if(m_nLineOffset>line1) {
        //Only the line numbers would change --
        m_nLineOffset-=nUpdLines;
        if(line0==m_nLineOffset) InvalidateLine(line0,0); 
      }
      else {
        LINENO nLastLine;
        GetEditRectLastLine(NULL,nLastLine,TRUE);
        if(nLastLine>=line0) {
          //At least part of the deleted text was visible --
          if(line0<m_nLineOffset) m_nLineOffset=line0;
          if(nUpdLines) InvalidateText(line0,0,LINENO_MAX,INT_MAX);
          else InvalidateLine(line0,chr0);
        }
      }
      
      //For deletes, update line count *after* InvalidateText() --
      m_nLineTotal-=nUpdLines;
      ASSERT(m_nLineTotal==GetLineCount());
  
      //Next, reposition insertion point if required.
      if(m_nActiveLine>line1) {
         //Change active line number (reposition not needed) --
         m_nSelectLine=m_nActiveLine-=nUpdLines;
      }
      else if(m_nActiveLine!=line0 || m_nActiveChr>chr0) {
		//if(m_nActiveLine>line0 || m_nActiveLine==line0 && m_nActiveChr>chr0) {
        //(line0 <= m_nActiveLine <= line1) and (m_nActiveLine:Chr > line0:chr0)
        //Reposition insertion point --
        if(m_nActiveLine==line1 && m_nActiveChr>chr1)
          //A prefix of the active line was deleted --
          SetPosition(line0,chr0+m_nActiveChr-chr1,POS_CHR|POS_NOTRACK);
        else
          //Move position to start of deleted block --
          SetPosition(line0,chr0,POS_CHR|POS_NOTRACK);
      }
  }
  else
    //UPD_REPLACE --
    if(upd&CLineDoc::UPD_REPLACE) {
         //Normally we can assume we are dealing with single keystrokes in
         //overstrike mode: line0==line1, chr1=chr0+1. For now, to test our
         //understanding we treat the general case --
         
         InvalidateText(line0,chr0,line1,chr1);
         
         if(m_nActiveLine>=line0 && m_nActiveLine<=line1 &&
            !(m_nActiveLine==line0 && m_nActiveChr<chr0)) {
           //If the character at the current position is changed by the replacement,
           //move position to the end of the replacement -- same as a cut followed
           //by a paste. Note that SetPosition(m_nActiveLine,m_nActiveChr)
           //must be used to recalculate the caret position since we allow
           //proportional fonts --
           SetPosition(line1,
             (m_nActiveLine==line1 && chr1<m_nActiveChr)?m_nActiveChr:chr1,POS_CHR|POS_NOTRACK);
         }
    }
   
  if(upd&CLineDoc::UPD_SCROLL) {
    if(upd&CLineDoc::UPD_BARS) UpdateBars(); 
    	//SetScrollPos(SB_VERT,GetVertBarPos(m_nLineOffset));
    ScrollToActivePosition();
    PositionCaret();
  }
  else {
    ASSERT(upd&CLineDoc::UPD_DELETE);
  } 
}

void CLineView::OnUpdate(CView *,LPARAM lHint,CObject* pHint)
{
	// OnUpdate() is called by CDocument::UpdateAllViews() when
	// either the font or a range of text has changed.
	
    if(pHint != NULL)
	{
      ASSERT(pHint->IsKindOf(RUNTIME_CLASS(CLineHint)));
  
      if(PH(upd)&CLineDoc::UPD_CLEARSEL) {
        //Invoked by CLineDoc prior to any updating -- 
        //For the active view, we move the caret to the beginning of the affected range.
        if(m_bOwnCaret) SetPosition(PH(pRange)->line0,PH(pRange)->chr0,POS_CHR|POS_NOTRACK);
        else ClearSelection();
      }
      else
        RefreshText(PH(upd),PH(pRange)->line0,PH(pRange)->chr0,
          PH(pRange)->line1,PH(pRange)->chr1);
	}
	else if(lHint==LHINT_FONT)
	{
	  //Complete invalidation of window due to font change --
	  CalculateLineMetrics();
	  UpdateBars();
	  Invalidate();
	  //Caret position changed if font changed, but we retain selection.
	  SetPosition(m_nActiveLine,m_nActiveChr,POS_CHR|POS_SCROLL,!IsSelectionCleared());
	}
	//else no updating
}

void CLineView::OnDraw(CDC* pDC)
{
	// The window has been invalidated and needs to be repainted;
	// or a page needs to be printed (or previewed).
     
	// First, determine the range of lines that need to be displayed or printed.
	int nFirstOffset,nLastOffset;
	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	
	if(rectClip.top<m_nHdrHeight) {
	  OnDrawHdr(pDC);
	  if((rectClip.top=m_nHdrHeight)>=rectClip.bottom) return;
	}
	
    ASSERT(GetLineCount());
    
    //Determine 0-base ordinals of all or partially invalidated lines --
	RectToLineOffsets(rectClip,nFirstOffset,nLastOffset,TRUE);
	
    //The derived class will redraw the rectangles bounding individual
    //lines. NOTE: The background of rectClip has NOT been erased.
    //Any actual erasing here (or via WM_ERASEBKGND) can cause flicker.
    
	int clipBottom=rectClip.bottom;
    LINENO nLine=m_nLineOffset+(nFirstOffset-1);
	
	rectClip.top=m_nLineHeight*nFirstOffset+m_nHdrHeight;
	rectClip.bottom=rectClip.top;
	
	for(;nFirstOffset<=nLastOffset;nFirstOffset++)
	{
	  rectClip.bottom+=m_nLineHeight;
	  OnDrawLine(pDC,++nLine,rectClip);
	  rectClip.top+=m_nLineHeight;
	}
	
	// Erase background beneath the last line --
	if(rectClip.top<=clipBottom ) {
	  rectClip.bottom=clipBottom;
	  pDC->FillRect(rectClip,&m_cbBackground);
	}
}

void CLineView::InvalidateText(LINENO line0,int chr0,LINENO line1,int chr1)
{
    //Invalidate the rectangle covering the normalized range of text.
    
    //NOTE:For a single line we avoid repainting the left unchanged portion
    //(as determined by chr0) but not the right. Only with certain
    //fonts that are slow to display does this reduce flicker.
    
    //Convert line0 and line1 to zero-based window line numbers --
    if(line1<m_nLineOffset) return;
    if(line0<m_nLineOffset) {
      line0=m_nLineOffset;
      chr0=0;
    }
    if(chr0==chr1 && line0==line1) return;
    
    //Determine offset of the last displayed line with respect to the first --
	LINENO nLastLine;
	CRect rect;
	
	GetEditRectLastLine(&rect,nLastLine,TRUE);
    if(line0>nLastLine) return;
    
    if(line1>nLastLine) {
	  line1=nLastLine;
	  //In case we would want to adjust rect.right -- chr1=INT_MAX;
	}
    rect.top=((int)(line0-m_nLineOffset)*m_nLineHeight)+m_nHdrHeight;
    rect.bottom=((int)(line1-m_nLineOffset+1)*m_nLineHeight)+m_nHdrHeight;
    
	if(chr0>1 && line0==line1) {
	  //For the common case that only the right portion of a single line
	  //is being refreshed, we can avoid some repainting.
      chr0=GetPosition(line0,chr0-1,rect.left);
	  //Decrementing chr0 is necessary with the Arial (>10 pt) font at least.
	  //It appears that a cell can lose its last column of pixels when it is
	  //followed by 'w', whose cell is then widened by one pixel!
	  rect.left-=m_scrollX;
	}
	InvalidateRect(&rect);
}

void CLineView::ClearSelection()
{
   if(m_nSelectLine!=m_nActiveLine || m_nSelectChr!=m_nActiveChr) {
      InvalidateSelection(m_nSelectLine,m_nSelectChr,m_nActiveLine,m_nActiveChr);          
      m_nSelectLine=m_nActiveLine;
      m_nSelectChr=m_nActiveChr;
   }
   DisplayCaret(m_bCaretVisible=true);
}

BOOL CLineView::IncActivePosition(BOOL bSelect)
{
  LINENO nLine=m_nActiveLine;
  UINT chr=m_nActiveChr;
  
  if(chr>=(UINT)*LinePtr(nLine)) {
    if(nLine>=m_nLineTotal) {
      if(!bSelect) ClearSelection();
      return FALSE;
    }
    nLine++;
    chr=0;
  }
  else chr++;
  return SetPosition(nLine,chr,POS_CHR,bSelect);
}
      
BOOL CLineView::DecActivePosition(BOOL bSelect)
{
  LINENO nLine=m_nActiveLine;
  UINT chr=m_nActiveChr;
  
  if(!chr) {
    if(nLine<=1) {
      if(!bSelect) ClearSelection();
      return FALSE;
    }
    chr=(UINT)*LinePtr(--nLine);
  }
  else chr--;
  return SetPosition(nLine,chr,POS_CHR,bSelect);
}

/////////////////////////////////////////////////////////////////////////////
// CLineView commands
void CLineView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	BOOL bControl,bShift;
	UINT iTyp=POS_CHR;
	LINENO nLine;
	int iPos;
	
    if(IsMinimized()) goto dflt;
    
    StatusClear();
    
    SetFlagLine(0);

    bControl=GetKeyState(VK_CONTROL)<0;
    bShift=GetKeyState(VK_SHIFT)<0;
    
	switch (nChar)
	{
	    case VK_SHIFT:
	    case VK_CONTROL: return; 
	    
		case VK_HOME:
            if(bControl) {
              nLine=1;
              iPos=0;
            }
		    else {
		       //Toggle between chr 0 and first word --
		       nLine=m_nActiveLine;
		       iPos=0;
		       LPSTR pline=(LPSTR)LinePtr(nLine);
		       for(int len=*pline++;len && !::IsCharAlphaNumeric(*pline);len--,iPos++,pline++);
               if(iPos==m_nActiveChr) iPos=0;
		    }
			break;
			
		case VK_END:
            if(bControl) {
              nLine=m_nLineTotal;
              iPos=0;
            }
		    else {
		      nLine=m_nActiveLine;
		      iPos=*LinePtr(nLine);
		    }
			break;
			
		case VK_UP:
		    if(bControl && !bShift) {
			  OnVScroll(SB_LINEUP,0,NULL);
			  return;
		    }
		    nLine=m_nActiveLine-1;
		    iPos=-1;
		    iTyp=POS_NEAREST;
			break;
			
		case VK_DOWN:
		    if(bControl && !bShift) {
			  OnVScroll(SB_LINEDOWN,0,NULL);
			  return;
			}
		    nLine=m_nActiveLine+1;
		    iPos=-1;
		    iTyp=POS_NEAREST;
			break;
			
		case VK_RIGHT:
		    if(!bControl) {
		      IncActivePosition(bShift);
		      return;
		    }
		    goto _chkDerived;
			
		case VK_LEFT:
		    if(!bControl) {
		      DecActivePosition(bShift);
		      return;
		    }
		    goto _chkDerived;
			
		case VK_PRIOR:
		    if(!bControl || bShift) {
              OnVScroll(SB_PAGEUP,0,NULL);
			  nLine=m_nActiveLine-m_nPageVert/m_nLineHeight;
			  iPos=-1;
			  iTyp=POS_NEAREST;
			}
			else {
			  //Horz pageup --;
			  nLine=m_nActiveLine;
			  iPos=m_caretX>m_nPageHorz?m_caretX-m_nPageHorz:0;
			  iTyp=POS_NEAREST;
			}
			break;
			
		case VK_NEXT:
		    if(!bControl || bShift) {
              OnVScroll(SB_PAGEDOWN,0,NULL);
              nLine=m_nActiveLine+m_nPageVert/m_nLineHeight;
              iPos=-1;
              iTyp=POS_NEAREST;
			}
			else {
			  //Horz pagedown --;
			  nLine=m_nActiveLine;
			  iPos=m_caretX+m_nPageHorz;
			  iTyp=POS_NEAREST;
			}
			break;
			
	    case VK_BACK: if(IsSelectionCleared() && !DecActivePosition(FALSE)) return;
	                  nChar=VK_DELETE; //Allow derived class to delete
	    
		//Allow derived class to adjust position and/or update as necessary --	
	    default: goto _chkDerived;
	}
	SetPosition(nLine,iPos,iTyp,bShift);
	return;
	    
_chkDerived:
	if(ProcessVKey(nChar,bShift,bControl)) return;
	
dflt:
	CView::OnKeyDown(nChar,nRepCnt,nFlags);
}

void CLineView::OnLButtonDown(UINT nFlags, CPoint point)
{
	StatusClear();
	
    if(point.y>m_nHdrHeight) {
      SetFlagLine(0);
      BOOL bShift=(nFlags&MK_SHIFT)!=0;
      SetPosition(m_nLineOffset+(point.y-m_nHdrHeight)/m_nLineHeight,m_scrollX+point.x,
        bShift?(POS_NEAREST|POS_ADJUST):POS_NEAREST);
      m_iSelectX=m_iSelectY=0;
      m_bSelectMode=TRUE;
      SetTimer(0x64,0x64,NULL);
      SetCapture();
	}
}

void CLineView::OnLButtonUp(UINT nFlags, CPoint point)
{
    if(m_bSelectMode) {
      ReleaseCapture();
      KillTimer(0x64);
      m_iSelectX=m_iSelectY=0;
      m_bSelectMode=FALSE;
      SetPosition(m_nActiveLine,m_nActiveChr,POS_CHR|POS_ADJUST|POS_SCROLL);
    }
}

void CLineView::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_bSelectMode) {
	  CRect rect;
	  GetEditRect(&rect);
	  if(rect.PtInRect(point)) {
        SetPosition(m_nLineOffset+(point.y-m_nHdrHeight)/m_nLineHeight,
          m_scrollX+point.x,POS_NEAREST|POS_ADJUST);
	    m_iSelectX=m_iSelectY=0;
      }
      else {
        //Perform real-time updating of position in one dimension only.
        //Timer will handle the dimension that requires scrolling --
        m_iSelectY=point.y<rect.top?-1:(point.y>=rect.bottom?1:0);
        m_iSelectX=point.x<rect.left?-1:(point.x>=rect.right?1:0);
        if(!m_iSelectX) {
          //Cursor is above or below window --
          SetPosition(m_nActiveLine,m_scrollX+point.x,POS_NEAREST,TRUE);
        }
        else if(!m_iSelectY) {
          //Cursor is right or left of window --
          LINENO nLine=m_nLineOffset+(point.y-m_nHdrHeight)/m_nLineHeight;
		  if(nLine>m_nLineTotal) nLine=m_nLineTotal;
          SetPosition(nLine,(m_iSelectX>0)?*LinePtr(nLine):0,POS_CHR,TRUE);
          m_iSelectX=0;
        }
      }
    }
}

void CLineView::OnTimer(UINT nIDEvent)
{
    
	if(nIDEvent==0x64) {
      int lineInc=0;
      int xPos;
      UINT iTyp;

      if(m_iSelectY>0 && m_nActiveLine<m_nLineTotal) lineInc++;
      else if(m_iSelectY<0 && m_nActiveLine>1) lineInc--;
	  else m_iSelectY=0;

	  if(m_iSelectX) {
	    if(m_iSelectX<0) xPos=0;
		else xPos=*LinePtr(m_nActiveLine+lineInc);
		iTyp=POS_CHR;
	  }
	  else if(lineInc) {
	    xPos=m_caretX;
	    iTyp=POS_NEAREST;
	  }
	  
	  if(lineInc||m_iSelectX) {
	    if(!SetPosition(m_nActiveLine+lineInc,xPos,iTyp,TRUE)) m_iSelectY=m_iSelectX=0;
	  }
    }
}

void CLineView::OnEditDelete()
{
	ProcessVKey(VK_DELETE,FALSE,FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CLineView diagnostics

#ifdef _DEBUG
CLineDoc* CLineView::GetDocument() // non-debug version is inline
{
        ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CLineDoc)));
        return (CLineDoc*) m_pDocument;
}

void CLineView::Dump(CDumpContext& dc) const
{
	ASSERT_VALID(this);

	CView::Dump(dc);

    AFX_DUMP1(dc, "\nm_nLineTotal = ", m_nLineTotal);
    AFX_DUMP1(dc, "\nm_nLineOffset = ", m_nLineOffset);
	AFX_DUMP1(dc, "\nm_bInsideUpdate = ", m_bInsideUpdate);
	AFX_DUMP1(dc, "\nm_nLineMaxWidth = ", m_nLineMaxWidth);
	AFX_DUMP1(dc, "\nm_nHdrHeight = ", m_nHdrHeight);
}

#endif //_DEBUG

BOOL CLineView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if(zDelta<0) {
		OnVScroll(SB_LINEDOWN,0,NULL);
	}
	else {
		OnVScroll(SB_LINEUP,0,NULL);
	}
	return TRUE;
}

void CLineView::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!GetDocument()->m_bReadOnly);
}

void CLineView::OnEditSelectall()
{
	SetPosition(1,0,POS_CHR|POS_NOTRACK);
	SetPosition(m_nLineTotal,INT_MAX,POS_CHR|POS_NOTRACK,TRUE);
}
