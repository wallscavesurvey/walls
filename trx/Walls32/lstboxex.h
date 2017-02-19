#if !defined(LSTBOXEX_H)
#define LSTBOXEX_H

// lstboxex.h : header file
//

class CListBoxExResources
{
private:
	HCURSOR m_hCursorMoveMultiple;
	HCURSOR m_hCursorMoveSingle;
	HCURSOR m_hCursorCopyMultiple;
	HCURSOR m_hCursorCopySingle;
	HCURSOR m_hCursorNoDrop;
private:
	COLORREF m_ColorWindow;
	COLORREF m_ColorHighlight;
	COLORREF m_ColorWindowText;
	COLORREF m_ColorHighlightText;
	
	COLORREF m_ColorTransparent1;
	COLORREF m_ColorTransparent2;

	CDC      m_dcFinal;
	HGDIOBJ  m_hOldBitmap;
	CBitmap  m_BmpScreen;
	WORD     m_BitMapId;
	WORD     m_BitmapHeight;
	WORD     m_BitmapWidth;
private:
	void GetSysColors();
	void PrepareBitmaps( BOOL );
	void UnprepareBitmaps();
	void UnloadResources();
	void LoadResources();
public:
	void SysColorChanged();
	
	const CDC&   DcBitMap()         const { return m_dcFinal; } 
	
	WORD         BitmapHeight()     const { return m_BitmapHeight; }
	WORD         BitmapWidth()      const { return m_BitmapWidth; }
	
	HCURSOR      CursorNoDrop()     const { return m_hCursorNoDrop; }
	HCURSOR      CursorCopySingle() const { return m_hCursorCopySingle; }
	HCURSOR      CursorCopyMulti()  const { return m_hCursorCopyMultiple; }
	HCURSOR      CursorMoveSingle() const { return m_hCursorMoveSingle; }
	HCURSOR      CursorMoveMulti()  const { return m_hCursorMoveMultiple; }

	COLORREF ColorWindow()          const { return m_ColorWindow; }
	COLORREF ColorHighlight()       const { return m_ColorHighlight; }
	COLORREF ColorWindowText()      const { return m_ColorWindowText; }
	COLORREF ColorHighlightText()   const { return m_ColorHighlightText; }
	
	CListBoxExResources
	(
		WORD bitmapId, WORD, WORD, WORD, WORD, WORD, WORD, COLORREF=RGB_GREEN, COLORREF=RGB_PURPLE
	);
	
	~CListBoxExResources();	
};

class CListBoxExDrawStruct
{
public:
	const CListBoxExResources *m_pResources;
	CDC*  m_pDC;
	CRect m_Rect;
	BOOL  m_Sel;
	DWORD m_ItemData;
	WORD  m_ItemIndex;
		
	CListBoxExDrawStruct(
		CDC* pdc, LPRECT pRect, BOOL sel, DWORD item, WORD itemIndex, const CListBoxExResources *pres )
	{
		m_pDC        = pdc;
		m_Sel        = sel;
		m_ItemData   = item;
		m_ItemIndex  = itemIndex;
		m_pResources = pres;
		m_Rect.CopyRect(pRect);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CListBoxEx window

class CListBoxEx : public CListBox
{
protected:
	int     m_lfHeight;
	BOOL    m_bTracking;
	BOOL	m_bRtTracking;
	BOOL    m_bTracking1;
	BOOL    m_bTrackingEnabled;
	BOOL    m_bMultiple;
	BOOL    m_bDragSingle;
	BOOL    m_bCopy;
	int     m_MouseDownItem;
	CWnd *  m_pDropWnd;
		
	HCURSOR m_hOldCursor;
	HCURSOR m_hOldTrackCursor;
protected:	
	const CListBoxExResources* m_pResources;
// Construction
public:
	CListBoxEx();
	void AttachResources( const CListBoxExResources* );
// Attributes
public:
	enum eLbExCursor { LbExMoveSingle, LbExMoveMultiple, LbExCopySingle, LbExCopyMultiple, LbExNoDrop };

	int TextHeight() const {return m_lfHeight;}

// Operations
public:
	BOOL ChangeTextHeight(int cHeight);
	void SetTextHeight(int cHeight) {m_lfHeight=cHeight;}
	void InitTracking(CPoint &point);
	
// Implementation
public:
	virtual ~CListBoxEx();
	// Generated message map functions
protected:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	
protected:
	// must override this to provide drawing of item
	virtual void DrawItemEx( CListBoxExDrawStruct& ) = 0;

	// you can override these for drag/drop server functionality --
	
	//Called from CListBoxEx::OnMouseMove() --
	virtual BOOL CanDrop(
	   CWnd* DropWnd,
	   CPoint* DropWndClientPt,
	   BOOL bTimer=FALSE) {return FALSE;}
	//Called from private CListBoxEx::DroppedItemsAt() --
	virtual void DroppedItemsOnWindowAt(
	   CWnd* DropWnd,
	   const CPoint& DropWndClientPt,
	   BOOL bCopy=FALSE){}
	// override this in order to provide custom drag/drop cursors for your list box
	virtual HCURSOR GetCursor(eLbExCursor) const {return HCURSOR(NULL);}
	
private:
	void StartTracking( BOOL, WORD=0 ); 
	void DroppedItemsAt( const CPoint& );
	void CheckTracking(const CPoint& point);
protected:
	void CancelTracking();
public:
	int  ItemFromPoint( const CPoint& );
	void EnableDragDrop( BOOL );
protected:
	//{{AFX_MSG(CListBoxEx)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCancelMode();
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct); 
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	 
	DECLARE_DYNAMIC(CListBoxEx)
};

/////////////////////////////////////////////////////////////////////////////

#endif // LSTBOXEX_H
