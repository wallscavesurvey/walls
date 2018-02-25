// WallsMapView.h : interface of the CWallsMapView class
//
#pragma once

#ifndef _MAPLAYER_H
#include "MapLayer.h"
#endif

#include "ScaleWnd.h"

#define _USE_CTR

#define MAX_MDISHIFT 5
#define INC_MDISHIFT 10
#define LEN_HISTORY 24

#define MIN_METERSPERPIXEL (1.0/1000)

class CMainFrame;
class CWallsMapDoc;

struct HIST_REC {
	HIST_REC() {}
	HIST_REC(const CFltPoint &ctr,double sc) : geoCtr(ctr),scale(sc) {}
	CFltPoint geoCtr;
	double scale;
};

template<typename TYPE> class CCircList {
public:
	CCircList(int len) : clist(len),pos_start(0),pos_current(-1),pos_end(0) {}
	void Clear() {pos_start=pos_current=pos_end=0;}
	BOOL IsNext() {return pos_current!=pos_end;}
	BOOL IsPrev() {return pos_current!=pos_start;}
	int inc_pos(int &pos)
	{
		if(++pos==clist.size()) pos=0;
		return pos;
	}
	int dec_pos(int &pos)
	{
		if(0==pos--) pos+=clist.size();
		return pos;
	}
	TYPE & GetNext()
	{
		ASSERT(IsNext());
		int pos=pos_current;
		return clist[inc_pos(pos)];
	}
	TYPE & GetPrev()
	{
		ASSERT(IsPrev());
		int pos=pos_current;
		return clist[dec_pos(pos)];
	}
	void Add(const TYPE &t)
	{
		clist[inc_pos(pos_current)]=t;
		if(pos_current && (pos_end=pos_current)==pos_start)
			inc_pos(pos_start);
	}
	std::vector<TYPE> clist;
	int pos_start,pos_current,pos_end;
};

class CWallsMapView : public CView
{
	friend class CDlgExportImg;

protected: // create from serialization only
	CWallsMapView();
	DECLARE_DYNCREATE(CWallsMapView)

// Attributes
public:
	CWallsMapDoc* GetDocument() const;
	static CWallsMapView *m_pViewDrag;
	static CWallsMapView *m_pSyncView;
	static bool m_bTestPoint;
	static bool m_bMarkCenter;

	bool m_bAutoCenterGPS,m_bTrackingGPS;

	bool m_bUseMarkPosition,m_bTrackRubber,m_bMeasuring;

	CFltPoint m_fMarkPosition;

	//Scroll wheel behavior --
	BOOL IsSelecting() {return GetDocument()->m_bSelecting;}
	void SetSelecting(BOOL bSel) {GetDocument()->m_bSelecting=bSel;}
	CBitmapRenderTarget m_RT;

private:
    CDIBWrapper m_DIB;
	CScaleWnd *m_pScaleWnd;
	double m_fScale; //Screen pixels per geounit (meters or degrees, or image px if not georeferenced)
	double m_fSignY;
	static int m_nMdiShift;
	CFltPoint m_fViewOrigin;
	CFltRect m_geoExtDIB;
	CPoint m_ptBitmapOverlap,m_ptDrag,m_ptMeasure;
	UINT m_xBorders,m_yBorders;
	UINT m_uAction;
	int m_iFullView;

	CPoint m_ptPopup;
	CFltPoint m_fptPopup;
	bool m_bDisplayTrack;
	BOOL m_bSync,m_bSyncCursor,m_bNoHist;
	bool m_bTestPointVisible,m_bTracking,m_bFirstViewDisplayed;
	CPoint m_ptSyncCursor;
	TOOLINFO m_ToolInfo;
	HWND m_wndTooltip;
	CPtNode m_ptNode;
	MAP_PTNODE *m_pMapPtNode;
	CCircList<HIST_REC> m_hist;

// Operations
public:
	static void SyncStop()	{
		if(CMainFrame::m_bSync) {
			GetMF()->UpdateViews(NULL,LHINT_SYNCSTOP);
			CMainFrame::m_bSync=FALSE;
			m_pSyncView=NULL;
		}
	}

	void RefreshDIB();

	void GeoPtToClientPt(const CFltPoint &fpt,CPoint &pt)
	{
		pt.x=_rnd((fpt.x-m_fViewOrigin.x)*m_fScale);
		pt.y=_rnd((fpt.y-m_fViewOrigin.y)*m_fScale*m_fSignY);
	}

	void ClientPtToGeoPt(const CPoint &pt,CFltPoint &fpt)
	{
		fpt.x=m_fViewOrigin.x+pt.x/m_fScale;
		fpt.y=m_fViewOrigin.y+pt.y/(m_fScale*m_fSignY);
	}

	void FitImage(BOOL bVisible);
#ifdef _USE_CTR
	void CenterOnPointDlg(CFltPoint *pfpt);
#endif
	void CenterOnGeoPoint(const CFltPoint &fpt,double fZoom);

	void FitShpExtent(CFltRect &geoExt);

	void GetViewExtent(CFltRect &fRect);
	double GetScale() {return m_fScale;} //m_fScale is pixels per geounit

	void OnUpdateStatusBar(CCmdUI* pCmdUI,int id);

	UINT GetScaleDenom()
	{
		return CLayerSet::GetScaleDenom(m_geoExtDIB,m_fScale,GetDocument()->IsProjected());
	}

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnActivateView(BOOL bActivate, CView* pActiveView, CView* pDeactiveView);

protected:
	virtual BOOL OnEraseBkgnd(CDC *pDC);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);

// Implementation
public:
	virtual ~CWallsMapView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	static void GetTooltip(MAP_PTNODE *pn,LPSTR text)
	{
		pn->pLayer->GetTooltip(pn->rec,text);
	}
	void ShowTooltip(CPoint point);
	void SetTooltipText();
	void DestroyTooltip();
	void SetScalePos();
	double MetersPerPixel(double scale);
	void DrawCross(const CPoint &pt);
	void MarkCenterPt();
	void MarkPosition();
	void MarkGridSelection();
	bool UpdateRestoredView();

	void DrawScale()
	{
		m_pScaleWnd->Draw(MetersPerPixel(m_fScale),GetDocument()->IsFeetUnits());
		SetScalePos();
	}

	void MarkSelected(CDC *pDC);
	void DrawOutlines(CDC *pDC);
	void DrawGPSTrackSymbol(CDC *pDC);

	bool RefreshPannedView();

	CFltPoint & GetClientCenter(CFltPoint &fpt)
	{
		//m_fScale is pixels per geounit
		CRect client;
		GetClientRect(&client);
		double fact=0.5/m_fScale;
		fpt.x=m_fViewOrigin.x+client.right*fact;
		fpt.y=m_fViewOrigin.y+client.bottom*fact*m_fSignY;
		return fpt;
	}

	void UpdateOnClientPt(const CPoint &pt)
	{
		CFltPoint fpt;
		ClientPtToGeoPt(pt,fpt);
		UpdateDIB(fpt,m_fScale);
	}

	void UpdateOnClientPt(const CPoint &pt,double scale)
	{
		CFltPoint fpt;
		ClientPtToGeoPt(pt,fpt);
		UpdateDIB(fpt,scale);
	}

	MAP_PTNODE *GetPtNode(int x,int y)
	{
		return m_ptNode.GetPtNode(x-m_ptBitmapOverlap.x,y-m_ptBitmapOverlap.y);
	}

	bool IsPointerValid() {
		return (m_uAction&(CWallsMapDoc::ACTION_PAN|CWallsMapDoc::ACTION_SELECT|CWallsMapDoc::ACTION_NOCOORD))==0;
	}

	double MaxImagePixelsPerGeoUnit()
	{
		return GetDocument()->MaxImagePixelsPerGeoUnit();
	}

	CImageLayer *GetTopImageLayer(const CFltPoint &ptGeo)
	{
		return GetDocument()->LayerSet().GetTopImageLayer(ptGeo);
	}

	CImageLayer *GetTopNteLayer(const CFltPoint &ptGeo)
	{
		CLayerSet &lSet=GetDocument()->LayerSet();
		return lSet.HasNteLayers()?lSet.GetTopNteLayer(ptGeo):NULL;
	}

	double ImagePixelsPerGeoUnit(const CFltPoint &ptGeo)
	{
		return GetDocument()->LayerSet().ImagePixelsPerGeoUnit(ptGeo);
	}

	double PopupPixelsPerGeoUnit()
	{
		CFltPoint ptGeo;
		ClientPtToGeoPt(m_ptPopup,ptGeo);
		return ImagePixelsPerGeoUnit(ptGeo);
	}

	bool IsScaleOK(double fScale)
	{
		//fScale = proposed screen pixels / geo_units, hence
		//fScale == 512*MaxImagePixelsPerGeoUnit() --> scrn_px == 512 * Img_px, or
		//fScale == 8/geo_perim --> fScale*geo_perim==8 px
		//(fZoom=scrn px/img_px)==1 --> fScale/(img_px/geo_units)==1 --> fScale==MaxImagePixelsPerGeoUnit()
		return fScale<=100000000.0 && fScale*GetDocument()->ExtentHalfPerim()>=8.0;
	}
	void DrawSyncCursor();
	void RefreshMeasure();
	double GetDragAz();
	void GetDragDistAz(CString &cs);
	BOOL ClearDIB(int width,int height);
	void UpdateDIB(const CFltPoint &ptGeoCtr,double newScale);
	void UpdateViewsAtPoint(const CPoint &point,LPARAM lHint);
	double GetPtSyncCursorFromHint(CObject *pHint);
	//void UpdateZoomPane();
	void UpdateTimePane();
	void GetFrameClient(CRect &rect);
	void InitFullViewWindow(); 
	void CenterOnPtPopup(double scale);
	void EditShape(int x,int cx,int y,int cy);
	void SelectEdited(BOOL bNew);
	//void MaximizeWindow();
	CMDIChildWnd * ComputeBorders();

protected:
	DECLARE_MESSAGE_MAP()

// Generated message map functions
public:
	virtual void OnInitialUpdate();
	//afx_msg void OnViewNozoom();
	//afx_msg void OnUpdateViewNozoom(CCmdUI *pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnContextNozoom();
	//afx_msg void OnViewFull();
	//afx_msg void OnUpdateViewFull(CCmdUI *pCmdUI);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnContextZoom12();
	afx_msg void OnUpdateContextZoom12(CCmdUI *pCmdUI);
	afx_msg void OnContextZoom25();
	afx_msg void OnUpdateContextZoom25(CCmdUI *pCmdUI);
	afx_msg void OnContextZoom50();
	afx_msg void OnUpdateContextZoom50(CCmdUI *pCmdUI);
	afx_msg void OnContextZoom625();
	afx_msg void OnUpdateContextZoom625(CCmdUI *pCmdUI);
	afx_msg void OnContextZoom3125();
	afx_msg void OnUpdateContextZoom3125(CCmdUI *pCmdUI);
	afx_msg void OnUpdateContextNozoom(CCmdUI *pCmdUI);
	afx_msg void OnContextZoomin();
	afx_msg void OnContextZoomout();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSyncOnpoint();
	afx_msg void OnUpdateSyncOnpoint(CCmdUI *pCmdUI);
	afx_msg void OnCenterOnPoint();
	afx_msg void OnUpdateCenterOnPosition(CCmdUI *pCmdUI);
	afx_msg void OnCenterOnPosition();
	afx_msg void OnFitImage();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnDestroy();
	afx_msg void OnContextZoomSpecify();
	afx_msg LRESULT OnMouseLeave(WPARAM,LPARAM);
	afx_msg LRESULT OnMouseHover(WPARAM,LPARAM);
	afx_msg LRESULT OnExitSizeMove (WPARAM, LPARAM);
	afx_msg void OnOptionsShowoutlines();
	afx_msg void OnUpdateOptionsShowoutlines(CCmdUI *pCmdUI);
	afx_msg void OnFitExtent();
	afx_msg void OnOptionsShowselected();
	afx_msg void OnUpdateOptionsShowselected(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFitImage(CCmdUI *pCmdUI);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnHideLabels();
	afx_msg void OnUpdateHideLabels(CCmdUI *pCmdUI);
	afx_msg void OnUpdateShowLabels(CCmdUI *pCmdUI);
	afx_msg void OnHideMarkers();
	afx_msg void OnUpdateHideMarkers(CCmdUI *pCmdUI);
	afx_msg void OnUpdateShowMarkers(CCmdUI *pCmdUI);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnGoBackward();
	afx_msg void OnUpdateGoBackward(CCmdUI *pCmdUI);
	afx_msg void OnGoForward();
	afx_msg void OnUpdateGoForward(CCmdUI *pCmdUI);
	afx_msg void OnToolMeasure();
	afx_msg void OnUpdateToolMeasure(CCmdUI *pCmdUI);
	afx_msg void OnOptionsBkcolor();
	afx_msg void OnUpdateAverageColors(CCmdUI *pCmdUI);
	afx_msg void OnAverageColors();
	afx_msg void OnUpdateSaveHistory(CCmdUI *pCmdUI);
	afx_msg void OnSaveHistory();
	afx_msg void OnUpdateSaveLayers(CCmdUI *pCmdUI);
	afx_msg void OnSaveLayers();
	afx_msg void OnUpdateRestoreView(CCmdUI *pCmdUI);
	afx_msg void OnRestoreView();
	afx_msg void OnEditShape();
	afx_msg void OnRelocateShape();
	afx_msg void OnFindlabel();
	afx_msg void OnUpdateFindlabel(CCmdUI *pCmdUI);
	afx_msg void OnAddLayer();
	afx_msg void OnAddShape(UINT id);
	afx_msg void OnUpdateAddLayer(CCmdUI *pCmdUI);
	afx_msg void OnSelectEdited();
	afx_msg void OnUpdateSelectEdited(CCmdUI *pCmdUI);
	afx_msg void OnToolSelect();
	afx_msg void OnUpdateToolSelect(CCmdUI *pCmdUI);
	afx_msg void OnSelectAdd();
	afx_msg void OnUpdateSelectAdd(CCmdUI *pCmdUI);
	afx_msg void OnExportPng();
	afx_msg void OnDatumToggle();
	afx_msg void OnElevUnitsToggle();
	afx_msg void OnScaleUnitsToggle();
	afx_msg void OnUpdateScaleUnitsFeet(CCmdUI *pCmdUI);
	afx_msg void OnUpdateScaleUnitsMeters(CCmdUI *pCmdUI);
	afx_msg void OnUpdateElevUnitsFeet(CCmdUI *pCmdUI);
	afx_msg void OnUpdateElevUnitsMeters(CCmdUI *pCmdUI);
	afx_msg void OnUpdateDatumToggle1(CCmdUI *pCmdUI);
	afx_msg void OnUpdateDatumToggle2(CCmdUI *pCmdUI);
	//afx_msg void OnOptionsGE();
	afx_msg void OnLaunchGE();
	afx_msg void OnLaunchWebMap();
	afx_msg void OnOptionsWebMap();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	//afx_msg void OnUpdateLaunchGe(CCmdUI *pCmdUI);
	afx_msg void OnImageOpacity();
	afx_msg LRESULT OnTabletQuerySystemGestureStatus(WPARAM,LPARAM);
	afx_msg void OnGpsTracking();
	afx_msg void OnUpdateGpsTracking(CCmdUI *pCmdUI);
	afx_msg void OnCtrOnGPSPoint();
	afx_msg void OnUpdateCtrOnGPSPoint(CCmdUI *pCmdUI);
	afx_msg void OnGpsDisplaytrack();
	afx_msg void OnUpdateGpsDisplaytrack(CCmdUI *pCmdUI);
	afx_msg void OnGpsEnablecentering();
	afx_msg void OnUpdateGpsEnablecentering(CCmdUI *pCmdUI);
	afx_msg LRESULT OnPropViewDoc(WPARAM wParam,LPARAM lParam);
};

#ifndef _DEBUG  // debug version in WallsMapView.cpp
inline CWallsMapDoc* CWallsMapView::GetDocument() const
   { return reinterpret_cast<CWallsMapDoc*>(m_pDocument); }
#endif


