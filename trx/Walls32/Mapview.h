// mapview.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CMapView view

#ifndef __FTRACKER_H
#include "ftracker.h"
#endif


struct MAP_VECNODE;
class CScaleWnd;

class CMapView : public CView
{
	DECLARE_DYNCREATE(CMapView)

private:
   bool m_bPanning,m_bPanned,m_bMeasuring; /*,m_bScrollBars,m_bScrolling;*/
   BOOL m_bCursorInBmp,m_bTracking;
   int m_iPosX,m_iPosY,m_iPtrX,m_iPtrY,m_iPosMaxX,m_iPosMaxY;
   CScaleWnd *m_pScaleWnd;

   CPoint m_ptBitmapOverlap,m_ptPan;

   CMapView();			// protected constructor used by dynamic creation
   CMapFrame *m_pFrame;
   CPoint m_ptPopup,m_ptPopupRaw;

   void ClearLocate() { if(m_pVecNode) RefreshVecNode(TRUE);}
   void DrawLocate() { if(m_pVecNode) RefreshVecNode(FALSE);}
   void SetScalePos();
  
// Attributes

	int m_iPltrec;
	BOOL m_bInContext;
    //CMapView *m_pNextView;
	void GetCoordinate(CPoint point);
	void NewView(CRect &rect);
	void RefreshCursor();
	void RefreshMeasure();
	bool IsReviewNode();
	void PanExit();
	void ToggleLayer(UINT flag);

#ifdef VISTA_INC
	void CMapView::ShowTrackingTT(CPoint &point);
#endif

// Operations
public:
	static HCURSOR m_hCursorPan,m_hCursorCross,m_hCursorArrow,m_hCursorMeasure;
	static HCURSOR m_hCursorIdentify,m_hCursorHand;

	BOOL m_bIdentify;
	BOOL m_bProfile;
	bool m_bFeetUnits;

	void DrawScale();
	void RefreshView();
	CSize m_sizeClient;
	CRect m_rectBmp;  // not same m_sizeClient during resize operation

	// Implementation
private:
	virtual ~CMapView();
    BOOL PreCreateWindow(CREATESTRUCT& cs);

	MAP_VECNODE *m_pVecNode;
	CRect m_VecNodeRect;
	void RefreshVecNode(BOOL bClear);
    void OnMapZoom(double scale);

	CPrjDoc *GetDocument() { return (CPrjDoc *)CView::GetDocument(); }

	virtual	void OnDraw(CDC* pDC);		// overridden to draw this view
	virtual	void OnInitialUpdate();		// first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActiveView, CView* pDeactiveView);
	//afx_msg void OnClose();
	afx_msg void OnUpdateUTM(CCmdUI* pCmdUI);
    afx_msg LRESULT OnMouseLeave(WPARAM wNone, LPARAM lParam);
	afx_msg LRESULT OnExitSizeMove(WPARAM, LPARAM);
	afx_msg void OnUpdateTravToggle(CCmdUI* pCmdUI);
	afx_msg void OnTravToggle();
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
	afx_msg void OnUpdateMapZoomin(CCmdUI* pCmdUI);
	afx_msg void OnMapZoomout();
	afx_msg void OnUpdateMapZoomout(CCmdUI* pCmdUI);
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
	afx_msg void OnMeasureDist();
	afx_msg void OnUpdateMeasureDist(CCmdUI* pCmdUI);
	afx_msg void OnScaleUnitsToggle();
	afx_msg void OnUpdateScaleUnitsFeet(CCmdUI *pCmdUI);
	afx_msg void OnUpdateScaleUnitsMeters(CCmdUI *pCmdUI);
	afx_msg void OnMapPrefixes();
	afx_msg void OnUpdateMapPrefixes(CCmdUI* pCmdUI);
	afx_msg void OnMapVectors();
	afx_msg void OnUpdateMapVectors(CCmdUI* pCmdUI);
	//afx_msg void OnMapSelected();
	//afx_msg void OnUpdateMapSelected(CCmdUI* pCmdUI);
	afx_msg void OnMapElevations();
	afx_msg void OnUpdateMapElevations(CCmdUI* pCmdUI);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLocateInGeom();
	afx_msg void OnGoEdit();
	afx_msg void OnVectorInfo();
	afx_msg void OnViewSegment();
	afx_msg void OnUpdateLocateInGeom(CCmdUI* pCmdUI);
	afx_msg void OnResizemapframe();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
