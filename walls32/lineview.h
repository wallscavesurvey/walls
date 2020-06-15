// lineview.h : interface of the CLineView class
//
// This class implements the behavior of a scrolling view that presents
// multiple lines of fixed-height data.
// Define _ADJ_WHOLE to enable "whole-word" selection behavior.
/////////////////////////////////////////////////////////////////////////////
#undef _ADJ_WHOLE

#ifndef LINEDOC_H
#include "linedoc.h"
#endif

class CLineView : public CView
{
	DECLARE_DYNAMIC(CLineView)

public:
	enum { LHINT_REFRESH = 0, LHINT_FONT = 1 };
	enum { SB_VERT_RANGE = 1000 };
	enum { START_PREFIX = 3 };
	enum { POS_CHR = 0, POS_NEAREST = 1, POS_ADJUST = 2, POS_NOTRACK = 4, POS_SCROLL = 8 };
	CLineView();
	CLineDoc* GetDocument();

	static LINENO m_nLineStart;
	//static LINENO m_nLineStart2;
	static int m_nCharStart;
	LINENO  m_nLineOffset;    // current line offset of first displayed line
	LINENO  m_nLineTotal;     // total lines in document

protected:
	bool  m_bCaretVisible;    // FALSE if data are selected (?)
	bool  m_bOwnCaret;        // TRUE for the active view
	long  m_caretX;           // current line-relative caret X-position
	long  m_caretY;           // current client-relative caret Y-position
	int	  m_rangeX;           // current horizontal scrolling range
	int   m_scrollX;          // current horiz scroll offset

	int     m_nActiveChr;     // Current char offset of caret in active line
	LINENO  m_nActiveLine;    // Maintain an active line for use by the derived class
	int     m_nSelectChr;     // Base line of current selection
	LINENO  m_nSelectLine;    // Base char of current selection
	LINENO  m_nFlagLine;
	//LINENO	m_nFlagLine2;
	BOOL  m_bSelectMode;
	BOOL  m_bInsideUpdate;    // internal state for OnSize callback
	int   m_iSelectX;
	int   m_iSelectY;
	int   m_lastCaretX;       //Used only by SetPosition()
	int   m_nMargin;          //X-offset in window of first line char
	int   m_nPageHorz;
	int   m_nPageVert;
	int   m_nLineHorz;
	int   m_nLineMinWidth;    // minimum size for m_nLineHorz and m_nPageHorz
	int   m_nLineMaxWidth;    // width of longest line in current device units
	int   m_nLineHeight;      // height of line in current device untis
	int   m_nHdrHeight;       // height of top-of window header
	HWND  m_hWndParent;

	static CBrush m_cbBackground;
	static CBrush m_cbHighlight;
	static CBrush m_cbFlag;
	static COLORREF m_TextColor;

	// Operations --
public:

	void GetEditRectLastLine(CRect *pRect, LINENO& nLastLine, BOOL bIncludePartial);
	void ScrollToError(); //Called by CPrjView::Compile()
	void SetFlagLine(LINENO line); //Used by CWedView::OnGotoline()

	//Used by CPrjView::OnCompile() to position caret at error --
	BOOL SetPosition(LINENO nLine, int iPos, UINT iTyp = POS_CHR, BOOL bSelect = FALSE);

	void UpdateBars();      // Enable/disable, adjust and display scrollbars

	static BOOL IsWS(BYTE c) { return c == '\t' || c == ' '; }
	static int GetStartOfWord(LPBYTE pLine, int& nChr, int iDir);

	//Private helpers --
private:
	void GetEditRect(CRect *pRect)
	{
		GetClientRect(pRect);
		pRect->top += m_nHdrHeight;
	}
	void RectToLineOffsets(const CRect& rect, int& nFirstOff, int& nLastOff, BOOL bIncludePartial);
	static BOOL NormalizeViewRange(LINENO& nLine0, int& nChr0, LINENO& nLine1, int& nChr1);
	void RefreshText(UINT upd, LINENO line0, int chr0, LINENO line1, int chr1);
	void InvalidateText(LINENO nLine0, int nChr0, LINENO nLine1, int nChr1);
	void InvalidateSelection(LINENO nLine0, int nChr0, LINENO nLine1, int nChr1)
	{
		if (CLineDoc::NormalizeRange(nLine0, nChr0, nLine1, nChr1))
			InvalidateText(nLine0, nChr0, nLine1, nChr1);
	}
	BOOL IncActivePosition(BOOL bSelect);
	BOOL DecActivePosition(BOOL bSelect);

protected:
	void UpdateDoc(UINT uUpd, LPSTR pChar = NULL, int nLen = 0); //Used by derived class
	int  Margin() { return m_nMargin; }
	LPBYTE LinePtr(LINENO nLine) { return GetDocument()->LinePtr(nLine); }
	int	 LineLen(LINENO nLine) { return (int)*LinePtr(nLine); }
	BOOL IsMinimized() { return ::IsIconic(m_hWndParent); }
	//Device units (no validation) --
	void ScrollToDevicePosition(int xPos, LINENO lineOffset);
	void ScrollToActivePosition();
	LINENO GetLineCount() { return GetDocument()->GetLineCount(); }
	BOOL CLineView::IsSelectionCleared()
	{
		return m_nSelectChr == m_nActiveChr && m_nSelectLine == m_nActiveLine;
	}
#ifdef _ADJ_WHOLE
	void AdjustForWholeWords(LINENO nLine, int& nChr, int& xPos);
#endif
	BOOL GetSelectedChrs(LINENO nLine, int &chr1, int &chr2);
	BOOL GetSelectedString(LPSTR &pStart, int &nLen, BOOL bWholeWords = FALSE);
	void ClearSelection();
	void InvalidateLine(LINENO nLine, int nChr)
	{
		InvalidateText(nLine, nChr, nLine, nChr + 1);
	}
	BOOL GetTrueClientSize(CSize& sizeClient, CSize& sizeSb); // size with no bars
	int  GetWindowVertScroll(LINENO nLines) const;
	int  GetVertBarPos(LINENO lineOffset) const;
	void CalculateLineMetrics() {
		GetLineMetrics(m_nLineMinWidth, m_nLineMaxWidth, m_nLineHeight, m_nHdrHeight, m_nMargin);
	}
	int LineClientY(LINENO nLine) {
		//assumes line is visible --
		return (int)((nLine - m_nLineOffset)*m_nLineHeight) + m_nHdrHeight;
	}

	// Overridables
protected:
	//Optional overrides --

	//Required overrides (defaults can be implemented if needed) --
	virtual CFont * SelectTextFont(CDC* pDC) = 0;
	virtual void GetLineMetrics(int& nLineMinWidth, int& nLineMaxWidth, int& nLineHeight,
		int& nHdrHeight, int& nMargin) = 0;
	virtual void OnDrawLine(CDC* pDC, LINENO nLine, CRect rect) = 0;
	virtual void PositionCaret() = 0;
	virtual void DisplayCaret(bool bOn) = 0;
	virtual int GetPosition(LINENO nLine, int nChr, long& xPos) = 0;
	virtual void GetNearestPosition(LINENO nLine, int& nChr, long& xPos) = 0;
	virtual void OnDrawHdr(CDC* pDC) = 0;
	virtual BOOL ProcessVKey(UINT nChar, BOOL bShift, BOOL bControl) = 0;

	//Standard MFC Override --
	void OnInitialUpdate();

	// Overrides of CView
	virtual void OnUpdate(CView* pSender, LPARAM lHint = 0L, CObject* pHint = NULL);
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	virtual ~CLineView();
#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
#endif //_DEBUG

	// Generated message map functions
protected:
	//{{AFX_MSG(CLineView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnEditDelete();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg void OnEditSelectall();

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in lineview.cpp
inline CLineDoc* CLineView::GetDocument()
{
	return (CLineDoc*)m_pDocument;
}
#endif
