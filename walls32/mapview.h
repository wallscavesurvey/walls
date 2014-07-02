// mapview.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CMapView view

#ifndef __FTRACKER_H
#include "ftracker.h"
#endif

struct MAP_VECNODE;

class CMapView : public CScrollView
{
	DECLARE_DYNCREATE(CMapView)
protected:
   BOOL m_bPanning,m_bMeasuring,m_bScrollBars;
   int m_iPosX,m_iPosY,m_iPtrX,m_iPtrY,m_iPosMaxX,m_iPosMaxY;

   CMapView();			// protected constructor used by dynamic creation
   CRect m_rectBmp;  // Bitmap's client-relative rectangle in view
   BOOL m_bCursorInBmp,m_bTracking;
   BOOL m_bProfile,m_bFeetUnits;
   CMapFrame *m_pFrame;
   CPoint m_ptPopup,m_ptPopupRaw;

   void ClearLocate() { if(m_pVecNode) RefreshVecNode(TRUE);}
   void DrawLocate() { if(m_pVecNode) RefreshVecNode(FALSE);}

// Attributes
private:
	int m_iPltrec;
	BOOL m_bInContext;
    //CMapView *m_pNextView;
	void GetCoordinate(CPoint point);
	void NewView(CRect &rect);
	void ToggleLayer(UINT flag);
	void CenterBitmap();
	void RefreshCursor();
	void RefreshMeasure();

// Operations
public:
	static HCURSOR m_hCursorPan,m_hCursorCross,m_hCursorArrow,m_hCursorMeasure;
	static HCURSOR m_hCursorIdentify;
	static BOOL m_bMeasureDist;
	static CPoint m_ptMeasureStart,m_ptMeasureEnd;
	static DPOINT m_Coordinate,m_CoordinateMeasure;
	static void GetDistanceFormat(CString &s,BOOL bProfile,BOOL bFeetUnits);

	BOOL m_bIdentify;

	void RefreshView();

// Implementation
protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs);

	MAP_VECNODE *m_pVecNode;
	CRect m_VecNodeRect;
	void RefreshVecNode(BOOL bClear);
   
	virtual ~CMapView();
	virtual	void OnDraw(CDC* pDC);		// overridden to draw this view
	virtual	void OnInitialUpdate();		// first time after construct
	
	afx_msg void OnUpdateUTM(CCmdUI* pCmdUI);
    afx_msg LRESULT OnMouseLeave(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CMapView)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMapCenter();
	afx_msg void OnMapLabels();
	afx_msg void OnMapOriginal();
	afx_msg void OnMapZoomin();
	afx_msg void OnMapZoomout();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnUpdateMapLabels(CCmdUI* pCmdUI);
	afx_msg void OnMapMarkers();
	afx_msg void OnUpdateMapMarkers(CCmdUI* pCmdUI);
	afx_msg void OnMapNotes();
	afx_msg void OnUpdateMapNotes(CCmdUI* pCmdUI);
	afx_msg void OnMapFlags();
	afx_msg void OnUpdateMapFlags(CCmdUI* pCmdUI);
	afx_msg void OnMapPassage();
	afx_msg void OnUpdateMapPassage(CCmdUI* pCmdUI);
	afx_msg void OnMapViewhelp();
	afx_msg void OnUpdateMapZoomin(CCmdUI* pCmdUI);
	afx_msg void OnMeasureDist();
	afx_msg void OnUpdateMeasureDist(CCmdUI* pCmdUI);
	afx_msg void OnMapPrefixes();
	afx_msg void OnUpdateMapPrefixes(CCmdUI* pCmdUI);
	afx_msg void OnMapVectors();
	afx_msg void OnUpdateMapVectors(CCmdUI* pCmdUI);
	afx_msg void OnMapElevations();
	afx_msg void OnUpdateMapElevations(CCmdUI* pCmdUI);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLocateInGeom();
	afx_msg void OnGoEdit();
	afx_msg void OnVectorInfo();
	afx_msg void OnUpdateLocateInGeom(CCmdUI* pCmdUI);
	afx_msg void OnResizemapframe();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
