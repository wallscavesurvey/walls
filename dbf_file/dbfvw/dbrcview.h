// dbrcview.h : interface of the CDBRecView class
//

// This class implements the behavior of a scrolling view that presents
// multiple rows of fixed-height data.  A row view is similar to an
// owner-draw listbox in its visual behavior; but unlike listboxes,
// a row view has all of the benefits of a view (as well as scroll view),
// including perhaps most importantly printing and print preview.
/////////////////////////////////////////////////////////////////////////////

class CDBFHeader;

class CDBRecView : public CView
{
	DECLARE_DYNAMIC(CDBRecView)
	
public:
	CDBRecView();
	CDBRecDoc* GetDocument();
	
protected:
	BOOL  m_bInsideUpdate;    // internal state for OnSize callback
	CSize m_totalDev;         // total size in device units
	CSize m_pageDev;          // per page scroll size in device units 
	CSize m_lineDev;          // per line scroll size in device units
	LONG  m_lineOffset;       // current line offset of first displayed line
	LONG  m_lineTotal;        // total scrolling lines in document
	
	LONG  m_nPrevSelectedRow; // index of the most recently selected row
	LONG  m_nPrevRowCount;
	int   m_nRowWidth;        // width of row in current device units
	int   m_nRowLabelWidth;   // width of row label in current device units
	int   m_nRowHeight;       // height of row in current device untis
	int   m_nHdrHeight;       // height of top-of window header
	int	  m_nHdrCols;         // Column header items including blank one for rec no.

// Operations --
public:

    // Call the following in OnInitialUpdate() and whenever the document's size
    // changes. It does no screen updating, but prepares for the next UpdateBars.
    void SetLineRange(LONG lineOffset,LONG lineTotal);
    void SetLineTotal(LONG lineTotal);
	void UpdateBars();      // Enable/disable, adjust and display scrollbars
 
    // The following functions invalidate but do not force screen update --
    void ScrollToRow(LONG lineOffset);

	void FillOutsideRect(CDC* pDC, CBrush* pBrush);

protected:
	LONG GetActiveRow(){return GetDocument()->GetActiveRecNo();}
	LONG GetRowCount() {return GetDocument()->GetRecordCount();}
	void ActivateNextRow(BOOL bNext) {GetDocument()->ActivateNext(bNext);}
	void ActivateRow(LONG nRow) {GetDocument()->Activate(nRow);}
	CRect RowWndOffsetToRect(int rowWndOffset,CDBRecHint *pHint);
	void RectToRowWndOffsets(const CRect& rectLP, 
			int& nFirstOff, int& nLastOff, BOOL bIncludePartiallyShownRows);
	BOOL GetTrueClientSize(CSize& sizeClient, CSize& sizeSb); // size with no bars
    int  GetWindowVertScroll(LONG nRows) const;
    int  GetVertBarPos(LONG lineOffset) const;
	void ResizeHdr(int xshift,int xrange);

// Overridables
protected:
	//Header notification messages --
	virtual BOOL OnNotify(WPARAM wParam,LPARAM lParam,LRESULT* pResult);

	  // called by derived class's OnUpdate --
	virtual void UpdateRow(LONG nInvalidRow,CDBRecHint *pHint);

    virtual void SelectTextFont(CDC* pDC) {pDC->SelectStockObject(ANSI_FIXED_FONT);}
	virtual void GetRowDimensions(CDC* pDC,int& nRowWidth,
	             int& nRowHeight,int& nHdrCols,int& nRowLabelWidth) = 0;
	virtual void OnDrawRow(CDC* pDC, LONG nRow, int y, BOOL bSelected) = 0;
	virtual void GetFieldData(int idx,LPSTR &pText,int &fmt,int &len)=0;

	// standard overrides of MFC classes
	void OnInitialUpdate();
	
// Overrides of CView

	virtual void OnUpdate(CView* pSender,LPARAM lHint=0L,CObject* pHint=NULL);
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	virtual ~CDBRecView();

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
#endif //_DEBUG

// Generated message map functions
protected:
	CDBFHeader *m_phdr;
	//{{AFX_MSG(CDBRecView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG	// debug version in dbrcview.cpp
inline CDBRecDoc* CDBRecView::GetDocument()
   { return (CDBRecDoc*) m_pDocument; }
#endif
