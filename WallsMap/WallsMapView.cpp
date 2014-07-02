// WallsMapView.cpp : implementation of the CWallsMapView class
//

#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "ftracker.h"
#include "PercentDlg.h"
#include "backclrdlg.h"
#include "MapLayer.h"
#include "NtiLayer.h"
#include "WallsMapDoc.h"
#include "wallsmapview.h"
#include "LayerSetSheet.h"
#include "ShowShapeDlg.h"
#include "AdvSearchDlg.h"
#include "DlgExportImg.h"
#include "kmldef.h"
#include "DlgOptionsGE.h"
#include "SelEditedDlg.h"
#include "UpdatedDlg.h"
#include "DlgSymbolsImg.h"
#include "CenterViewDlg.h"
#include "GPSPortSettingDlg.h"
#include <georef.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CWallsMapView

IMPLEMENT_DYNCREATE(CWallsMapView, CView)

BEGIN_MESSAGE_MAP(CWallsMapView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_WM_ERASEBKGND()
	//ON_COMMAND(ID_VIEW_NOZOOM, OnViewNozoom)
	//ON_UPDATE_COMMAND_UI(ID_VIEW_NOZOOM, OnUpdateViewNozoom)
	ON_WM_SETFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
	ON_MESSAGE(WM_MOUSEHOVER,OnMouseHover)
	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	ON_WM_SETCURSOR()
	ON_COMMAND(ID_CONTEXT_ZOOM6_25, OnContextZoom625)
	ON_UPDATE_COMMAND_UI(ID_CONTEXT_ZOOM6_25, OnUpdateContextZoom625)
	ON_COMMAND(ID_CONTEXT_ZOOM12, OnContextZoom12)
	ON_UPDATE_COMMAND_UI(ID_CONTEXT_ZOOM12, OnUpdateContextZoom12)
	ON_COMMAND(ID_CONTEXT_ZOOM25, OnContextZoom25)
	ON_UPDATE_COMMAND_UI(ID_CONTEXT_ZOOM25, OnUpdateContextZoom25)
	ON_COMMAND(ID_CONTEXT_ZOOM50, OnContextZoom50)
	ON_UPDATE_COMMAND_UI(ID_CONTEXT_ZOOM50, OnUpdateContextZoom50)
	ON_COMMAND(ID_CONTEXT_ZOOM3_125, OnContextZoom3125)
	ON_UPDATE_COMMAND_UI(ID_CONTEXT_ZOOM3_125, OnUpdateContextZoom3125)
	ON_COMMAND(ID_CONTEXT_NOZOOM, OnContextNozoom)
	ON_UPDATE_COMMAND_UI(ID_CONTEXT_NOZOOM, OnUpdateContextNozoom)
	ON_COMMAND(ID_CONTEXT_ZOOMIN, OnContextZoomin)
	ON_COMMAND(ID_CONTEXT_ZOOMOUT, OnContextZoomout)
	ON_WM_SIZE()
	ON_COMMAND(ID_CTR_ONPOSITION, OnCenterOnPosition)
	ON_UPDATE_COMMAND_UI(ID_CTR_ONPOSITION,OnUpdateCenterOnPosition )
	ON_COMMAND(ID_SYNC_ONPOINT, OnSyncOnpoint)
	ON_UPDATE_COMMAND_UI(ID_SYNC_ONPOINT, OnUpdateSyncOnpoint)
	ON_COMMAND(ID_CTR_ONPOINT, OnCenterOnPoint)
	ON_COMMAND(ID_FIT_IMAGE, OnFitImage)
	ON_UPDATE_COMMAND_UI(ID_FIT_IMAGE, OnUpdateFitImage)
	ON_WM_MOUSEWHEEL()
	ON_WM_DESTROY()
	ON_COMMAND(ID_CONTEXT_ZOOM_SPECIFY, OnContextZoomSpecify)
	ON_COMMAND(ID_OPTIONS_SHOWOUTLINES, OnOptionsShowoutlines)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SHOWOUTLINES, OnUpdateOptionsShowoutlines)
	ON_COMMAND(ID_FIT_EXTENT, OnFitExtent)
	ON_COMMAND(ID_OPTIONS_SHOWSELECTED, OnOptionsShowselected)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SHOWSELECTED, OnUpdateOptionsShowselected)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_HIDELABELS, OnHideLabels)
	ON_UPDATE_COMMAND_UI(ID_HIDELABELS, OnUpdateHideLabels)
	ON_COMMAND(ID_HIDEMARKERS, OnHideMarkers)
	ON_UPDATE_COMMAND_UI(ID_HIDEMARKERS, OnUpdateHideMarkers)
	ON_COMMAND(ID_TOOLMEASURE, OnToolMeasure)
	ON_UPDATE_COMMAND_UI(ID_TOOLMEASURE, OnUpdateToolMeasure)

	ON_COMMAND(ID_SHOW_LABELS, OnHideLabels)
	ON_UPDATE_COMMAND_UI(ID_SHOW_LABELS, OnUpdateShowLabels)
	ON_COMMAND(ID_SHOW_MARKERS, OnHideMarkers)
	ON_UPDATE_COMMAND_UI(ID_SHOW_MARKERS, OnUpdateShowMarkers)
	ON_COMMAND(ID_AVERAGE_COLORS,OnAverageColors)
	ON_UPDATE_COMMAND_UI(ID_AVERAGE_COLORS, OnUpdateAverageColors)
	ON_COMMAND(ID_SAVE_HISTORY,OnSaveHistory)
	ON_UPDATE_COMMAND_UI(ID_SAVE_HISTORY, OnUpdateSaveHistory)
	ON_COMMAND(ID_SAVE_LAYERWINDOW,OnSaveLayers)
	ON_UPDATE_COMMAND_UI(ID_SAVE_LAYERWINDOW, OnUpdateSaveLayers)
	ON_COMMAND(ID_RESTOREVIEW,OnRestoreView)
	ON_UPDATE_COMMAND_UI(ID_RESTOREVIEW, OnUpdateRestoreView)
	ON_COMMAND(ID_ZOOM_BACK, OnGoBackward)
	ON_UPDATE_COMMAND_UI(ID_ZOOM_BACK, OnUpdateGoBackward)
	ON_COMMAND(ID_ZOOM_FORWARD, OnGoForward)
	ON_UPDATE_COMMAND_UI(ID_ZOOM_FORWARD, OnUpdateGoForward)

	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_OPTIONS_BKCOLOR, OnOptionsBkcolor)
	ON_COMMAND(ID_EDIT_SHAPE, OnEditShape)
	ON_COMMAND(ID_RELOCATE_SHAPE, OnRelocateShape)
	ON_COMMAND_RANGE(ID_APPENDLAYER_0,ID_APPENDLAYER_N,OnAddShape)
	ON_COMMAND(ID_FINDLABEL, OnFindlabel)
	ON_UPDATE_COMMAND_UI(ID_FINDLABEL, OnUpdateFindlabel)
	ON_COMMAND(ID_ADDLAYER, OnAddLayer)
	ON_UPDATE_COMMAND_UI(ID_ADDLAYER,OnUpdateAddLayer)

	ON_COMMAND(ID_SELECT_EDITED, OnSelectEdited)
	ON_UPDATE_COMMAND_UI(ID_SELECT_EDITED, OnUpdateSelectEdited)
	ON_COMMAND(ID_TOOL_SELECT, OnToolSelect)
	ON_UPDATE_COMMAND_UI(ID_TOOL_SELECT,OnUpdateToolSelect)
	ON_COMMAND(ID_SELECT_ADD, OnSelectAdd)
	ON_UPDATE_COMMAND_UI(ID_SELECT_ADD,OnUpdateSelectAdd)
	ON_COMMAND(ID_EXPORT_PNG, OnExportPng)

	ON_COMMAND(ID_SCALEUNITS_FEET, OnScaleUnitsToggle)
	ON_UPDATE_COMMAND_UI(ID_SCALEUNITS_FEET, OnUpdateScaleUnitsFeet)
	ON_COMMAND(ID_SCALEUNITS_METERS, OnScaleUnitsToggle)
	ON_UPDATE_COMMAND_UI(ID_SCALEUNITS_METERS, OnUpdateScaleUnitsMeters)

	ON_COMMAND(ID_ELEVUNITS_FEET, OnElevUnitsToggle)
	ON_UPDATE_COMMAND_UI(ID_ELEVUNITS_FEET, OnUpdateElevUnitsFeet)
	ON_COMMAND(ID_ELEVUNITS_METERS, OnElevUnitsToggle)
	ON_UPDATE_COMMAND_UI(ID_ELEVUNITS_METERS, OnUpdateElevUnitsMeters)

	ON_COMMAND(ID_DATUMTOGGLE_1, OnDatumToggle)
	ON_UPDATE_COMMAND_UI(ID_DATUMTOGGLE_1, OnUpdateDatumToggle1)
	ON_COMMAND(ID_DATUMTOGGLE_2, OnDatumToggle)
	ON_UPDATE_COMMAND_UI(ID_DATUMTOGGLE_2, OnUpdateDatumToggle2)

	ON_COMMAND(ID_LAUNCH_WEBMAP, OnLaunchWebMap)
	ON_COMMAND(ID_OPTIONS_WEBMAP, OnOptionsWebMap)
	ON_COMMAND(ID_LAUNCH_GE, OnLaunchGE)
	//ON_COMMAND(ID_OPTIONS_GE, OnOptionsGE)
	//ON_UPDATE_COMMAND_UI(ID_LAUNCH_GE, OnUpdateLaunchGe)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_COMMAND(ID_IMAGE_OPACITY,OnImageOpacity)
	ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, OnTabletQuerySystemGestureStatus)

	ON_COMMAND(ID_GPS_TRACKING, OnGpsTracking)
	ON_UPDATE_COMMAND_UI(ID_GPS_TRACKING, OnUpdateGpsTracking)
	ON_UPDATE_COMMAND_UI(ID_GPS_CENTER,OnUpdateCtrOnGPSPoint)
	ON_COMMAND(ID_GPS_CENTER,OnCtrOnGPSPoint)
	ON_COMMAND(ID_GPS_DISPLAYTRACK, &CWallsMapView::OnGpsDisplaytrack)
	ON_UPDATE_COMMAND_UI(ID_GPS_DISPLAYTRACK, &CWallsMapView::OnUpdateGpsDisplaytrack)
	ON_COMMAND(ID_GPS_ENABLECENTERING, &CWallsMapView::OnGpsEnablecentering)
	ON_UPDATE_COMMAND_UI(ID_GPS_ENABLECENTERING, &CWallsMapView::OnUpdateGpsEnablecentering)

	ON_REGISTERED_MESSAGE(WM_PROPVIEWDOC,OnPropViewDoc)
	END_MESSAGE_MAP()

int CWallsMapView::m_nMdiShift=0;
bool CWallsMapView::m_bMarkCenter=false;
CWallsMapView *CWallsMapView::m_pViewDrag=NULL;
CWallsMapView *CWallsMapView::m_pSyncView=NULL;
bool CWallsMapView::m_bTestPoint=false;

static GEODATA GD;

static int nad27=GF_NAD27();
static int wgs84=GF_WGS84();

// CWallsMapView construction/destruction

CWallsMapView::CWallsMapView() : m_hist(LEN_HISTORY)
{
   m_fScale = 0.0; //for testing
   m_ptBitmapOverlap.x=m_ptBitmapOverlap.y=0;
   m_uAction = CWallsMapDoc::ACTION_NOCOORD;
   m_iFullView = -1;
   m_pViewDrag=NULL;
   m_wndTooltip=NULL;
   m_pMapPtNode=NULL;
   m_pScaleWnd=NULL;
   m_bTracking=m_bFirstViewDisplayed=m_bUseMarkPosition=m_bTrackRubber=m_bMeasuring=false;
   m_bAutoCenterGPS=m_bTrackingGPS=false;
   m_bDisplayTrack=true;
   m_bSync=m_bSyncCursor=m_bNoHist=FALSE;
}

CWallsMapView::~CWallsMapView()
{
	if(m_pScaleWnd) {
		ASSERT(FALSE);
		delete m_pScaleWnd;
	}
	m_DIB.DeleteObject();
	DestroyTooltip();
	m_pViewDrag=NULL;
}

BOOL CWallsMapView::PreCreateWindow(CREATESTRUCT& cs)
{
	// Add the WS_CLIPCHILDREN style to avoid repaint problems:
	cs.style |= WS_CLIPCHILDREN;

	return CView::PreCreateWindow(cs);
}

// CWallsMapView printing

BOOL CWallsMapView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CWallsMapView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CWallsMapView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

// CWallsMapView diagnostics

#ifdef _DEBUG
void CWallsMapView::AssertValid() const
{
	CView::AssertValid();
}

void CWallsMapView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWallsMapDoc* CWallsMapView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWallsMapDoc)));
	return (CWallsMapDoc*)m_pDocument;
}
#endif //_DEBUG

////////////////////////////////////////////////////////////////////////
// CWallsMapView message handlers and overloads

// CWallsMapView drawing

BOOL CWallsMapView::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
}

void CWallsMapView::DrawOutlines(CDC *pDC)
{
	CRect client;
	GetClientRect(&client);

	CFltRect geoExtClient;
	ClientPtToGeoPt(client.TopLeft(),geoExtClient.tl);
	ClientPtToGeoPt(client.BottomRight(),geoExtClient.br);
	GetDocument()->LayerSet().DrawOutlines(pDC,geoExtClient,m_fScale);
}

void CWallsMapView::DrawCross(const CPoint &pt)
{
    CClientDC dc(this);
    dc.SetROP2(R2_NOT);
    dc.SetBkMode(TRANSPARENT);
    dc.MoveTo(pt.x-15,pt.y);
	dc.LineTo(pt.x+15,pt.y);
    dc.MoveTo(pt.x,pt.y-15);
	dc.LineTo(pt.x,pt.y+15);
}

void CWallsMapView::MarkCenterPt()
{
	CRect client;
	GetClientRect(&client);
	CPoint pt=client.CenterPoint();
	pt.y++;
	DrawCross(pt);
}

void CWallsMapView::MarkPosition()
{
	CRect client;
	GetClientRect(&client);

	CFltRect geoExtClient;
	ClientPtToGeoPt(client.TopLeft(),geoExtClient.tl);
	ClientPtToGeoPt(client.BottomRight(),geoExtClient.br);
  	CPoint pt(_rnd(m_fScale*(m_fMarkPosition.x-geoExtClient.l)),_rnd(m_fScale*(geoExtClient.t-m_fMarkPosition.y)));
	if(client.PtInRect(pt)) {
		DrawCross(pt);
	}
}

void CWallsMapView::DrawGPSTrackSymbol(CDC *pDC)
{
	CRect client;
	GetClientRect(&client);
	CWallsMapDoc *pDoc=GetDocument();
	ASSERT(!vptGPS.empty());

	CFltRect geoExtClient;
	ClientPtToGeoPt(client.TopLeft(),geoExtClient.tl);
	ClientPtToGeoPt(client.BottomRight(),geoExtClient.br);

	bool bDrawLn=m_bDisplayTrack && vptGPS.size()>1;

	CFltPoint fpt(vptGPS.back());
	pDoc->GetConvertedGPSPoint(fpt);

  	CPoint ptSym(_rnd(m_fScale*(fpt.x-geoExtClient.l)),_rnd(m_fScale*(geoExtClient.t-fpt.y)));

	if(m_bAutoCenterGPS) {
		WORD flg=(CGPSPortSettingDlg::m_wFlags&GPS_FLG_AUTOCENTER);
		double pc=(flg==0)?0.25:((flg==1)?0.15:0.05);
		CRect r(client);
		r.InflateRect(-(int)(pc*r.Width()),-(int)(pc*r.Height()));
		if(!r.PtInRect(ptSym)) {
			PostMessage(WM_PROPVIEWDOC);
			return;
		}
	}

	CRect rectSym(client);
	rectSym.InflateRect(50,50,50,50);

	bool bDrawPt=rectSym.PtInRect(ptSym)==TRUE;
	UINT len=vptGPS.size();
	if(bDrawPt || bDrawLn) {
		ASSERT(pGPSDlg);
		ID2D1DCRenderTarget *prt=NULL;
		if (0 > d2d_pFactory->CreateDCRenderTarget(&d2d_props, &prt) || 0 > prt->BindDC(pDC->m_hDC, client)) {
			SafeRelease(&prt);
			ASSERT(0);
			return;
		}
		
		if(bDrawLn) {
			ID2D1SolidColorBrush *pbr=NULL;
			if (0 > prt->CreateSolidColorBrush(RGBAtoD2D(pGPSDlg->m_mstyle_t.crMrk, pGPSDlg->m_mstyle_t.iOpacitySym/100.0f), &pbr)) {
				prt->Release();
				return;
			}
			prt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			prt->BeginDraw();

			int ibuf = 4 * client.Width();
			CRect rectTrack(client);
			rectTrack.InflateRect(ibuf,ibuf,ibuf,ibuf);
			bool bNeedConvert=!pDoc->IsGeoWgs84();
			bool bLastPt=false; //later, save point to allow clipping!(?)
			float w = pGPSDlg->m_mstyle_t.fLineWidth;
			CD2DPointF lastPtF;

			for(it_gpsfix it=vptGPS.begin();it!=vptGPS.end();it++) {
				fpt=*it;
				if(bNeedConvert) pDoc->GetConvertedGPSPoint(fpt);
				CPoint pt(_rnd(m_fScale*(fpt.x-geoExtClient.l)),_rnd(m_fScale*(geoExtClient.t-fpt.y)));
				if(rectTrack.PtInRect(pt)) {
					CD2DPointF ptF(pt);
					if(bLastPt) {
						prt->DrawLine(lastPtF, ptF, pbr, w, d2d_pStrokeStyleRnd);
					}
					if (bLastPt = it->flag == 0) {
						lastPtF = ptF;
					}
				}
				else {
					bLastPt=false;
				}
			}
			VERIFY(0 <= prt->EndDraw());
			pbr->Release();
		}

		if(bDrawPt) {
			prt->BeginDraw();
			CPlaceMark::PlotStyleSymbol(prt, pGPSDlg->m_mstyle_s, ptSym);
			CPlaceMark::PlotStyleSymbol(prt, pGPSDlg->m_mstyle_h, ptSym);
			prt->EndDraw();
		}

		prt->Release();
	}
}

void CWallsMapView::MarkGridSelection()
{
	CRect client;
	GetClientRect(&client);

	CFltRect geoExtClient;
	ClientPtToGeoPt(client.TopLeft(),geoExtClient.tl);
	ClientPtToGeoPt(client.BottomRight(),geoExtClient.br);
    CClientDC dc(this);
    dc.SetROP2(R2_NOT);
    dc.SetBkMode(TRANSPARENT);

	for(VEC_DWORD::iterator it=CShpLayer::m_vGridRecs.begin();it!=CShpLayer::m_vGridRecs.end();it++) {
		CFltPoint fpt(CShpLayer::m_pGridShp->m_fpt[*it-1]);
		CPoint pt(_rnd(m_fScale*(fpt.x-geoExtClient.l)),_rnd(m_fScale*(geoExtClient.t-fpt.y)));
		dc.MoveTo(pt.x-15,pt.y);
		dc.LineTo(pt.x+15,pt.y);
		dc.MoveTo(pt.x,pt.y-15);
		dc.LineTo(pt.x,pt.y+15);
	}
}

void CWallsMapView::MarkSelected(CDC *pDC)
{
	CWallsMapDoc *pDoc=GetDocument();
	if(!pDoc->m_vec_shprec.size()) return;

	bool bInitMrk=false;
	ID2D1DCRenderTarget *prt;
	CPlaceMark pm_s;
	CPoint ptHL(INT_MIN,0);
	CRect client;
	GetClientRect(&client);

	CFltRect geoExtClient;
	ClientPtToGeoPt(client.TopLeft(),geoExtClient.tl);
	ClientPtToGeoPt(client.BottomRight(),geoExtClient.br);

	for (it_shprec it = pDoc->m_vec_shprec.begin(); it != pDoc->m_vec_shprec.end(); it++) {
		ASSERT(it->pShp->ShpType()==CShpLayer::SHP_POINT);

		//gflags breakpoint triggered here when cancelling new point!
		
		if(!it->rec || !it->pShp->IsVisible() || !it->pShp->ExtentOverlap(geoExtClient) ||
			(it->pShp->m_pdbfile->vdbe[it->rec-1]&SHP_EDITDEL)) continue;

		CFltPoint fpt(it->pShp->m_fpt[it->rec-1]);
		if(fpt.x<geoExtClient.r && fpt.x>geoExtClient.l && fpt.y>geoExtClient.b && fpt.y<geoExtClient.t) {
			if(!bInitMrk) {
				if(0 > d2d_pFactory->CreateDCRenderTarget(&d2d_props,&prt) ||
					0 > prt->BindDC(*pDC, client)) {
					SafeRelease(&prt);
					ASSERT(0);
					break;
				}
				pm_s.Create(prt, CShpLayer::m_mstyle_s);
				bInitMrk = true;
				prt->BeginDraw();
			}

			CPoint pt((int)(m_fScale*(fpt.x-m_geoExtDIB.l))+m_ptBitmapOverlap.x,
				      (int)(m_fScale*(m_geoExtDIB.t-fpt.y))+m_ptBitmapOverlap.y);

			if(app_pShowDlg->IsRecHighlighted(it->pShp,it->rec))
				ptHL=pt;

			pm_s.Plot(prt, pt);
		}
	}
	if (bInitMrk) {
	 	if(ptHL.x != INT_MIN) {
			CPlaceMark::PlotStyleSymbol(prt, CShpLayer::m_mstyle_h, ptHL);
		}
		VERIFY(0 <= prt->EndDraw());
		SafeRelease(&prt);
	}
}

void CWallsMapView::OnDraw(CDC* pDC)
{
	HBRUSH hBrush = GetDocument()->m_hbrBack;
	CRect client;

	GetClientRect(&client);

	if(!m_DIB.GetHandle()) {
		::FillRect(pDC->m_hDC,client,hBrush);
		m_bSyncCursor=FALSE;
	}
	else {

		CRect image(0,0,m_DIB.GetWidth(),m_DIB.GetHeight());
		image.OffsetRect(m_ptBitmapOverlap);
		CRect overlap;
		if(!overlap.IntersectRect(&client,&image)) {
			::FillRect(pDC->m_hDC,client,hBrush);
		}
		else {
			if(overlap!=client) {
				if(image.top>0) ::FillRect(pDC->m_hDC,CRect(0,0,client.right,image.top),hBrush);
				if(image.left>0) ::FillRect(pDC->m_hDC,CRect(0,0,image.left,client.bottom),hBrush);
				if(image.right<client.right) ::FillRect(pDC->m_hDC,CRect(image.right,0,client.right,client.bottom),hBrush);
				if(image.bottom<client.bottom) ::FillRect(pDC->m_hDC,CRect(0,image.bottom,client.right,client.bottom),hBrush);
			}
			m_DIB.Draw(pDC->m_hDC,m_ptBitmapOverlap);
		}
		m_bSyncCursor=FALSE;
	}

	if(!(m_uAction&CWallsMapDoc::ACTION_PANNED)) {

		if(GetDocument()->m_bDrawOutlines) {
			DrawOutlines(pDC);
		}

		if(app_pShowDlg && GetDocument()==app_pShowDlg->m_pDoc && GetDocument()->m_bMarkSelected) {
			MarkSelected(pDC);
		}

		if(m_bTrackingGPS)
		{
			DrawGPSTrackSymbol(pDC);
		}

		if(m_bMarkCenter) {
			MarkCenterPt();
			m_bMarkCenter=false;
		}

		if(m_bUseMarkPosition) {
			MarkPosition();
		}

		if(CShpLayer::m_pGridShp) {
			if(CShpLayer::m_pGridShp->ShpType()!=CShpLayer::SHP_POINT)
				MarkCenterPt();
			else MarkGridSelection();
			CShpLayer::m_pGridShp=NULL;
			VEC_DWORD().swap(CShpLayer::m_vGridRecs);
		}

		if(m_bSync && m_pSyncView && m_pSyncView!=this) {
			DrawSyncCursor();
		}
	}
}

BOOL CWallsMapView::ClearDIB(int width,int height)
{
	ASSERT(width && height);

	if(!m_DIB.GetHandle() || m_DIB.GetWidth()!=width || m_DIB.GetHeight()!=height) {
		BITMAPINFOHEADER bmi;
		memset(&bmi,0,sizeof(bmi));
		bmi.biSize = sizeof(BITMAPINFOHEADER);
		bmi.biWidth = width;
		bmi.biHeight = -height;
		bmi.biPlanes = 1;
		bmi.biSizeImage = height*((3*width+3)&~3);
		bmi.biBitCount = 24;
		bmi.biCompression = BI_RGB;
		if(!m_DIB.InitBitmap((LPBITMAPINFO)&bmi)) return FALSE;
	}
	return m_DIB.Fill(GetDocument()->m_hbrBack);
}

void CWallsMapView::RefreshDIB()
{
	m_bNoHist=TRUE;
	CFltPoint fpt;
	UpdateDIB(GetClientCenter(fpt),m_fScale);
	m_bNoHist=FALSE;
}

void CWallsMapView::GetViewExtent(CFltRect &fRect)
{
	//Called when exporting
	CRect rect;
	GetClientRect(&rect);
	ClientPtToGeoPt(rect.TopLeft(),fRect.tl);
	ClientPtToGeoPt(rect.BottomRight(),fRect.br);
}

bool CWallsMapView::UpdateRestoredView()
{
	//restore last view by revising newScale and fptCenter.
	//compute scale such that the displayed vertical extent is the same
	ASSERT(GetDocument()->IsTransformed());
	CRestorePosition *pRP=GetDocument()->GetRestorePosition();
	if(pRP && pRP->uStatus>=1 && pRP->geoHeight>0.0 && GetDocument()->IsProjected()!=(fabs(pRP->ptGeo.x)<180 && fabs(pRP->ptGeo.y)<90)) {
		CRect crView;
		GetClientRect(&crView);
		//oldrange = pRP->height/pRP->scale = crView.Height()/newScale
		UpdateDIB(pRP->ptGeo,crView.Height()/pRP->geoHeight);
		return m_bFirstViewDisplayed=true;
	}
	m_bFirstViewDisplayed=true; //at least insure next view is saved
	return false;
}

void CWallsMapView::UpdateDIB(const CFltPoint &fptGeoCtr,double newScale)
{
	// Upon input:
	// fptGeoCtr	- desired new center in current client
	// newScale		- desired new scale (screen pixels / geo-units)
	// m_fSignY		- +1 or -1 (if georeferenced)

	// Upon return:
	// m_geoExtDIB			- Geo extent of updated DIB
	// m_ptBitmapOverlap	- pixel offset of DIB within client
	// m_fViewOrigin		- geo-coordinates of upper left corner of client
	// m_fScale				- set to newScale


	if(!m_bNoHist)
			m_hist.Add(HIST_REC(fptGeoCtr,newScale));

	CRect client;
	GetClientRect(&client);

	m_geoExtDIB.l=m_fViewOrigin.x=fptGeoCtr.x-client.right/(2.0*newScale);
	m_geoExtDIB.r=m_fViewOrigin.x+client.right/newScale;

	/*
	if(m_fSignY>0.0) {
		m_geoExtDIB.t=m_fViewOrigin.y=fptGeoCtr.y-(client.bottom/(2.0*newScale));
		m_geoExtDIB.b=m_fViewOrigin.y+(client.bottom/newScale);
	}
	else {
		m_geoExtDIB.t=m_fViewOrigin.y=fptGeoCtr.y+(client.bottom/(2.0*newScale));
		m_geoExtDIB.b=m_fViewOrigin.y-(client.bottom/newScale);
	}
	*/

	m_geoExtDIB.t=m_fViewOrigin.y=fptGeoCtr.y-m_fSignY*(client.bottom/(2.0*newScale));
	m_geoExtDIB.b=m_fViewOrigin.y+m_fSignY*(client.bottom/newScale);

	UINT w=0,h=0;
	CFltRect extDoc;
	GetDocument()->GetExtent(&extDoc,TRUE);

	if(m_geoExtDIB.IntersectWith(extDoc)) {
		if(GetDocument()->HasShpLayers()) {
			//Avoid truncation of labels
			m_geoExtDIB.r+=128/newScale;
			m_geoExtDIB.b-=128/newScale;
		}
		w=(UINT)(m_geoExtDIB.Width()*newScale+0.5);
		h=(UINT)(m_geoExtDIB.Height()*newScale+0.5);
	}

	m_ptBitmapOverlap.x=m_ptBitmapOverlap.y=0;
	CShpLayer::m_pPtNode=&m_ptNode;

	if(!w || !h) {
		m_DIB.DeleteObject();
		m_iFullView=0;
		m_ptNode.Init(0,0,0);
	}
	else {
		//Initialize new destination bitmap --
		ClearDIB(w,h);
		m_ptBitmapOverlap.x=(int)((m_geoExtDIB.l-m_fViewOrigin.x)*newScale);
		m_ptBitmapOverlap.y=(int)(m_fSignY*(m_geoExtDIB.t-m_fViewOrigin.y)*newScale);

		//Might as well make scale exactly correct for extent --
		//m_geoExtDIB.r=m_geoExtDIB.l+w/newScale;
		//m_geoExtDIB.b=m_geoExtDIB.t+m_fSignY*(h/newScale);

		//CopyToDIB() will initialize m_pPtNode for tooltip hit testing --

		//CopyToDIB returns:
		//			0			- No image data is in the rectangular region of the bitmap.
		//			1			- Only a portion of the image is contained in the region.
		//			2			- All of the image is contained in the region.

		m_iFullView=GetDocument()->CopyToDIB(&m_DIB,m_geoExtDIB,newScale);
	}

	ASSERT(newScale>0.0);

	if(m_pScaleWnd && (m_fScale!=newScale || !GetDocument()->IsProjected())) {
		m_fScale=newScale;;
		DrawScale();
	}
	else m_fScale=newScale;

	//Point m_ptBitmapOverlap will change during pan operation --
	//UpdateZoomPane();

	Invalidate(FALSE); //don't erase background

	//We've just repainted the view. No cursor should be visible --
	m_bSyncCursor=FALSE;

	if(m_bSync && m_pSyncView==this) {
		//Other views will now center at the geographic point
		//corresponding to this window's new center --
		CSyncHint syncHint(&fptGeoCtr,newScale);
		GetMF()->UpdateViews(this,LHINT_SYNCSTART,&syncHint);
	}
	m_uAction&=CWallsMapDoc::ACTION_NOCOORD;

	if((GetDocument()->m_uEnableFlags&NTL_FLG_RESTOREVIEW) && m_bFirstViewDisplayed) {
		//restore last view by revising newScale and fptCenter.
		//compute scale such that the displayed vertical extent is the same
		CRestorePosition *pRP=GetDocument()->GetRestorePosition();
		if(pRP) {
			pRP->geoHeight=client.Height()/newScale;
			ClientPtToGeoPt(client.CenterPoint(),pRP->ptGeo);
			pRP->uStatus=2;
			GetDocument()->SetChanged(1); //save without prompting upon close
		}
	}
}

void CWallsMapView::InitFullViewWindow()
{

	//Called only from OnInitialUpdate() --

	CWallsMapDoc* pDoc = GetDocument();
	CChildFrame * pParentFr=(CChildFrame *)GetParentFrame();

	bool bTrans=pDoc->IsTransformed();
	bool bTile=(bTrans && CMainFrame::IsPref(PRF_OPEN_MAXIMIZED));

	CRect crView;

	pParentFr->GetParentFrame()->GetClientRect(&crView);
	int i=GetMF()->ToolBarHt();
	if(i<2) i=2;
	crView.InflateRect(0,0,-4,-(i+GetMF()->StatusBarHt()+2));

	if(!bTile) {
		//Reduce size for cascade-style open --
		crView.InflateRect(-INC_MDISHIFT,-INC_MDISHIFT,-INC_MDISHIFT*MAX_MDISHIFT,-INC_MDISHIFT*MAX_MDISHIFT);
		crView.OffsetRect(m_nMdiShift*INC_MDISHIFT,m_nMdiShift*INC_MDISHIFT);
		if(++m_nMdiShift>MAX_MDISHIFT) m_nMdiShift=0;
	}

	//crView is now the MDI frame's client area we are making available for
	//image display.

	CFltPoint geoSize;
	double fScale;  //<window width>/<geographical extent or image pixels>
	bool bFitWidth=false;
	bool bCheckedVisible=false;

	UINT xSizeMax=crView.Width()-m_xBorders;
	UINT ySizeMax=crView.Height()-m_yBorders;
	ASSERT(xSizeMax>0 && ySizeMax>0);

	if(!(GetDocument()->m_uEnableFlags&NTL_FLG_RESTOREVIEW)) {

		pDoc->GetExtent(&m_geoExtDIB,TRUE); //Get extent assuming enabled (checked) layers will be shown

_recheck:

		geoSize=m_geoExtDIB.Size();

		if(geoSize.x*ySizeMax>=geoSize.y*xSizeMax) {
			bFitWidth=true;
			fScale=xSizeMax/geoSize.x;
		}
		else {
			fScale=ySizeMax/geoSize.y;
		}

		//fScale = desired screen pixels per geo_unit

		double fZoom=fScale/MaxImagePixelsPerGeoUnit(); //fZoom = screen pixels / image pixels
		if(fZoom>1.0) {
			fZoom=1.0;
			fScale=MaxImagePixelsPerGeoUnit();
		}

		{
			CFltRect fRect(m_geoExtDIB);
			pDoc->LayerSet().GetScaledExtent(m_geoExtDIB,fScale);
			if(memcmp(&fRect,&m_geoExtDIB,sizeof(CFltRect)))
				goto _recheck;
		}

		UINT x=(UINT)(fScale*geoSize.x+0.5);
		UINT y=(UINT)(fScale*geoSize.y+0.5);
		if(!x) x++;
		if(!y) y++;

		if(bTile) {
			//frame will be visible while loading data!
			if(bFitWidth) {
				xSizeMax=x+m_xBorders;
				ySizeMax+=m_yBorders;
			}
			else {
				xSizeMax+=m_xBorders;
				ySizeMax=y+m_yBorders;
			}
		}
		else {
			xSizeMax=x+m_xBorders;
			ySizeMax=y+m_yBorders;
		}
	}

	pParentFr->SetWindowPos(0,crView.left,crView.top,xSizeMax,ySizeMax,SWP_NOZORDER);

	if(bTile) {
		::PostMessage(GetMF()->m_hWndMDIClient,WM_MDITILE,MDITILE_VERTICAL,0);
	}
	else {
		if(GetDocument()->m_uEnableFlags&NTL_FLG_RESTOREVIEW) {
			ASSERT(!m_bFirstViewDisplayed);
			if(UpdateRestoredView()) 
				return;
		}
		UpdateDIB(m_geoExtDIB.Center(),fScale);
	}
}

void CWallsMapView::SetScalePos()
{
	CRect client;
	GetClientRect(&client);
	m_pScaleWnd->SetWindowPos(NULL,0,client.bottom-BAR_HEIGHT,m_pScaleWnd->m_width,
		BAR_HEIGHT,SWP_NOZORDER|SWP_SHOWWINDOW);
	m_pScaleWnd->m_bottom=client.bottom;
}

double CWallsMapView::MetersPerPixel(double scale)
{
	if(!GetDocument()->IsProjected()) {
		double m=(m_geoExtDIB.t+m_geoExtDIB.b)/2;
		return (MetricDistance(m,0,m,1)*1000)/scale;
	}
	return 1.0/scale;
}

void CWallsMapView::MaximizeWindow()
{
	WINDOWPLACEMENT wp;
	CMDIChildWnd *pParentFr=(CMDIChildWnd *)GetParentFrame();
	pParentFr->GetWindowPlacement(&wp);
	ASSERT(wp.length==sizeof(wp));
	wp.showCmd=SW_SHOWMAXIMIZED;
	VERIFY(pParentFr->SetWindowPlacement(&wp));
}

CMDIChildWnd * CWallsMapView::ComputeBorders()
{
	CMDIChildWnd *pParentFr=(CMDIChildWnd *)GetParentFrame();
	CRect crView,crViewCl;
	pParentFr->GetWindowRect(&crView);
	pParentFr->GetClientRect(&crViewCl);
	pParentFr->ClientToScreen(&crViewCl);
	m_xBorders=crView.Width()-crViewCl.Width();
	m_yBorders=crView.Height()-crViewCl.Height();
	ASSERT(m_xBorders>=0 && m_yBorders>=0);
	m_xBorders+=4;
	m_yBorders+=4;
	return pParentFr;
}

void CWallsMapView::OnInitialUpdate()
{
	GetMF()->ClearStatusPanes();

	if(GetDocument()->IsTransformed()) {
		m_fSignY=-1.0;
		// CStatic initialization --
		m_pScaleWnd = new CScaleWnd;
		VERIFY(m_pScaleWnd->Create(this));
	}
	else m_fSignY=1.0; 


	CMDIChildWnd *pParentFr=ComputeBorders();

	InitFullViewWindow();
	m_bTrackingGPS=pGPSDlg && pGPSDlg->HavePos() && GetDocument()->IsGeoRefSupported();
}

/*
void CWallsMapView::UpdateZoomPane()
{
	double zoom=m_fScale/ImagePixelsPerGeoUnit(NULL);
	if(zoom>0.0)
		GetMF()->SetStatusPane(4,"%s%%",GetPercentStr(zoom*100));
	else GetMF()->ClearStatusPane(4);
}

void CWallsMapView::OnViewNozoom()
{
	CRect client;
	GetClientRect(&client);
	m_ptPopup.x=client.right/2;
	m_ptPopup.y=client.bottom/2;
	OnContextNozoom();
}

void CWallsMapView::OnUpdateViewNozoom(CCmdUI *pCmdUI)
{
	double zoom=m_fScale/ImagePixelsPerGeoUnit(NULL);
	pCmdUI->Enable(zoom>0.0 && zoom!=1.0);

}
*/

void CWallsMapView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_pViewDrag=this;
}


void CWallsMapView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	ASSERT(!(m_uAction&CWallsMapDoc::ACTION_PAN));
	CWallsMapDoc *pDoc=GetDocument();
	if(IsSelecting()) {
		SetSelecting(0);
		pDoc->m_uActionFlags=0;
	}
	else {
		if(pDoc->m_uActionFlags==CWallsMapDoc::ACTION_MEASURE)
			pDoc->OnToolMeasure();
		else pDoc->OnViewPan();
	}
}

void CWallsMapView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	if(m_bSync) {
		if(m_bSyncCursor) DrawSyncCursor();
		GetMF()->UpdateViews(this,LHINT_SYNCHIDECURSOR);
		//We'll restore cursors in OnLButtonUp() --
		m_pSyncView=this;
	}

	if(IsSelecting()) {
			if(GetDocument()->HasShpPtLayers()) {
				CRectTracker tracker;
				//m_uAction=flag;
				//Sets capture and waits for LButtonUp (may be part of a double-click) --
				m_bTrackRubber=true;
				if(tracker.TrackRubberBand(this, point, TRUE)) {
					m_bTrackRubber=false;
					tracker.m_rect.NormalizeRect();
					if(tracker.m_rect.Width()>8 || tracker.m_rect.Height()>8) {
						CRect client;
						GetClientRect(&client);
						if(client.IntersectRect(&client,&tracker.m_rect)) {
							CPoint pt=client.CenterPoint();
							EditShape(pt.x,(client.Width()+1)/2,pt.y,(client.Height()+1)/2);
						}
					}
				}
				else m_bTrackRubber=false;
			}
	}
	else {
		UINT flag=GetDocument()->m_uActionFlags;

		switch(flag) {
			case 0:
			case CWallsMapDoc::ACTION_ZOOMIN:
			case CWallsMapDoc::ACTION_ZOOMOUT:
				{
					CFixedTracker tracker;
					CRect client;
					GetClientRect(&client);
					tracker.m_pr=&client;
					m_uAction=flag;
					//Sets capture and waits for LButtonUp (may be part of a double-click) --
					m_bTrackRubber=true;
					if(tracker.TrackRubberBand(this, point, TRUE)) {
						m_bTrackRubber=false;
						tracker.m_rect.NormalizeRect();
						int pxwidth=tracker.m_rect.Width();
						if(pxwidth>=client.right) return;
						double fNewScale;
						if(pxwidth>=MIN_TRACKERWIDTH) {
							fNewScale=(!flag || flag==CWallsMapDoc::ACTION_ZOOMIN)?
								((m_fScale*client.right)/pxwidth):((m_fScale*pxwidth)/client.right);

							if(IsScaleOK(fNewScale)) {
								UpdateOnClientPt(tracker.m_rect.CenterPoint(),fNewScale);
							}
						}
					}
					else {
						m_bTrackRubber=false;
						if(flag && tracker.m_rect.IsRectEmpty()) {
							m_ptPopup=tracker.m_rect.CenterPoint();
							ASSERT(client.PtInRect(m_ptPopup));

							if((!flag || flag==CWallsMapDoc::ACTION_ZOOMIN)) OnContextZoomin();
							else OnContextZoomout();
						}
					}
					m_uAction=0;
				}
				break;

			case CWallsMapDoc::ACTION_MEASURE:
				m_ptMeasure=point;
				m_bMeasuring=true;

			case CWallsMapDoc::ACTION_PAN:
				m_uAction=flag;
				m_ptDrag=point;
				//TrackMouse(m_hWnd);
				break;
		}
	}
}

bool CWallsMapView::RefreshPannedView()
{
	bool bRet=false;

	m_fViewOrigin.x=m_geoExtDIB.l-m_ptBitmapOverlap.x/m_fScale;
	m_fViewOrigin.y=m_geoExtDIB.t-m_ptBitmapOverlap.y/(m_fSignY*m_fScale);

	CFltRect geoExtClient;
	CRect client;
	GetClientRect(&client);

	//ClientPtToGeoPt() uses m_fViewOrigin --
	ClientPtToGeoPt(client.TopLeft(),geoExtClient.tl);
	ClientPtToGeoPt(client.BottomRight(),geoExtClient.br);

	CFltRect geoExtDoc;
	GetDocument()->GetExtent(&geoExtDoc,TRUE);

	if(geoExtClient.IntersectWith(geoExtDoc)) {
		//The client contains some part of document (should always be the case when panning) --
		if(!m_geoExtDIB.ContainsRect(geoExtClient)) {
			UpdateOnClientPt(client.CenterPoint());
			bRet=true;
		}
		else Invalidate(FALSE);
	}
	return bRet;
}

void CWallsMapView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bMeasuring=false;
	if(m_bSync) m_pSyncView=this;
	if(m_uAction&CWallsMapDoc::ACTION_PAN) {
		if(point!=m_ptDrag) {
			m_ptBitmapOverlap+=(point-m_ptDrag);
			m_uAction|=CWallsMapDoc::ACTION_PANNED;
		}
		//if m_iFullView==2, the bitmap contains the entire image,
		//so no updating is needed when panning --
		if((m_uAction&CWallsMapDoc::ACTION_PANNED)) {
			//Refresh bitmap only if required --
			RefreshPannedView();
			if(m_bSync) {
				//Redrawing of other views may be necessary even if this view doesn't require it--
				GetMF()->UpdateViews(this,LHINT_POSTTILE);
				ASSERT(m_pSyncView==this);
			}
		}
	}
	else if(m_uAction&CWallsMapDoc::ACTION_MEASURE) {
		RefreshMeasure();
	}

	if(m_bSync) {
		UpdateViewsAtPoint(point,LHINT_SYNCSHOWCURSOR);
	}
	m_uAction=0;
}

void CWallsMapView::RefreshMeasure()
{
    CClientDC dc(this);
    dc.SetROP2(R2_NOT);
    dc.SetBkMode(TRANSPARENT);
    dc.MoveTo(m_ptDrag);
	dc.LineTo(m_ptMeasure);
}

void CWallsMapView::OnUpdateStatusBar(CCmdUI* pCmdUI,int id)
{
	CString cs;
	CFltPoint ptGeo;

	switch(id) {
		case STAT_ZOOM:
			if(IsPointerValid()) {
				//Get zoom percentage for image beneath cursor --
				CFltPoint ptGeo;
				ClientPtToGeoPt(m_ptDrag,ptGeo);
				double zoom=m_fScale/ImagePixelsPerGeoUnit(ptGeo);
				if(zoom>0.0) cs.Format("%s%%",GetPercentStr(zoom*100));
			}
			break;

		case STAT_SCALE:
			{
				CWallsMapDoc *pDoc=GetDocument();
				if(!GetDocument()->IsTransformed()) break;

				cs.Format("1:%u",GetScaleDenom());
			}
			break;

		case STAT_DATUM:
			if(m_uAction&(CWallsMapDoc::ACTION_PAN|CWallsMapDoc::ACTION_SELECT)) return;
			if(GetDocument()->IsDatumToggled()) GetDocument()->GetOtherDatumName(cs);
			else GetDocument()->GetDatumName(cs);
			break;

		case STAT_SIZE:
			if(IsPointerValid()) {
				if(m_uAction&CWallsMapDoc::ACTION_MEASURE) {
					int dy=m_ptDrag.y-m_ptMeasure.y,dx=m_ptMeasure.x-m_ptDrag.x;
					double d=atan2((double)dx,(double)dy)*(180/M_PI);
					if(d<0.0) d+=360.0;
					cs.Format("Az: %.1f°",d);
					break;
				}
				CFltPoint ptGeo;
				ClientPtToGeoPt(m_ptDrag,ptGeo);
				CImageLayer *pLayer=GetTopNteLayer(ptGeo);
				if(pLayer) {
					int elev=((CNtiLayer *)pLayer)->GetApproxElev(ptGeo);
					if(elev!=CNTERecord::NODATA) {
						if(GetDocument()->IsElevUnitsFeet())
							cs.Format("Elev: %d ft",elev);
						else
							cs.Format("Elev: %.1f m",elev*0.3048);
					}
				}
				else if(pLayer=GetTopImageLayer(ptGeo)) {
					cs.Format("%dx%dx%d",pLayer->Width(),pLayer->Height(),pLayer->BitsPerSample()*pLayer->Bands());
				}
			}
			break;

		case STAT_DEG:
		case STAT_XY:
			{
				if(!IsPointerValid()) break;

				CWallsMapDoc *pDoc=GetDocument();
				bool bTrans=pDoc->IsTransformed();
				CFltPoint ptGeo0;
				ClientPtToGeoPt(m_ptDrag,ptGeo0);

				if(m_uAction&CWallsMapDoc::ACTION_MEASURE) {
					if(id!=STAT_DEG || bTrans && (!pDoc->IsGeoRefSupported() || !pDoc->IsValidGeoPt(ptGeo0))) break;
					double dist;
					CFltPoint ptGeo;
					ClientPtToGeoPt(m_ptMeasure,ptGeo);
					if(!bTrans || pDoc->IsProjected()) {
						ptGeo0.x-=ptGeo.x;
						ptGeo0.y-=ptGeo.y;
						dist=sqrt(ptGeo0.x*ptGeo0.x+ptGeo0.y*ptGeo0.y);
					}
					else {
						dist=1000*MetricDistance(ptGeo0.y,ptGeo0.x,ptGeo.y,ptGeo.x);
					}
					if(!bTrans) cs.Format("Dist: %.0f pixels",dist);
					else {
						if(pDoc->IsFeetUnits()) {
							dist*=(1/0.3048);
							if(dist>5280.0) cs.Format("Dist: %.1f miles",dist/5280);
							else cs.Format("Dist: %.1f feet",dist);
						}
						else {
							if(dist>=10000) cs.Format("Dist: %.1f km",dist/1000);
							else cs.Format("Dist: %.1f meters",dist);
						}
					}
				}
				else {
					if(bTrans && pDoc->IsGeoRefSupported()) {
						if(!pDoc->IsValidGeoPt(ptGeo0)) break;
						if(id==STAT_XY) {
							int zone=pDoc->IsDatumToggled()?pDoc->GetUTMOtherDatum(ptGeo0):pDoc->GetUTMCoordinates(ptGeo0);
							if(zone)
								cs.Format("UTM  %.1f E  %.1f N  %u%c",ptGeo0.x,ptGeo0.y,abs(zone),(zone>0)?'N':'S');
						}
						else {
							if(pDoc->IsDatumToggled()) pDoc->LayerSet().GetDEGOtherDatum(ptGeo0);
							else pDoc->GetDEGCoordinates(ptGeo0);
							if(fabs(ptGeo0.y)<90.0 && fabs(ptGeo0.x)<=180.0)
								cs.Format("%.5f%c %c  %.5f%c %c",fabs(ptGeo0.y),0xB0,
									(ptGeo0.y<0)?'S':'N',fabs(ptGeo0.x),0xB0,(ptGeo0.x<0)?'W':'E');
						}
					}
					else if(id==STAT_DEG)
						cs.Format("%.1f x %.1f",ptGeo0.x,ptGeo0.y);
				}
			}
			break;
	}
	pCmdUI->SetText(cs);
}

void CWallsMapView::OnMouseMove(UINT nFlags, CPoint point)
{
	// point units are client pixels --

	m_uAction&=~CWallsMapDoc::ACTION_NOCOORD;

	if(!m_bTracking) {
		TrackMouse(m_hWnd);
		m_bTracking=true;
	}

	if(m_uAction&CWallsMapDoc::ACTION_SELECT) {
		if(m_wndTooltip)
			DestroyTooltip();
		return;
	}

	m_pViewDrag=this;
	if(m_uAction&CWallsMapDoc::ACTION_PAN) {
		if(point!=m_ptDrag) {
			CPoint ptDiff(point-m_ptDrag);
			m_ptBitmapOverlap+=(ptDiff);
			m_ptDrag=point;
			m_uAction|=CWallsMapDoc::ACTION_PANNED;
			Invalidate(FALSE);
			if(m_bSync) {
				ASSERT(m_pSyncView==this);
				CSyncHint hint((CFltPoint *)&ptDiff,0);
				GetMF()->UpdateViews(this,LHINT_SYNCPAN,&hint);
			}
		}
		if(!(nFlags&MK_LBUTTON)) {
			OnLButtonUp(nFlags,point);
		}
		return;
	}

	if(m_uAction&CWallsMapDoc::ACTION_MEASURE) {
		ASSERT(m_bMeasuring);
		if(m_wndTooltip)
			DestroyTooltip();
		RefreshMeasure();
		m_ptMeasure=point;
		RefreshMeasure();
	}
	else {
		m_ptDrag=point;
		if(m_bSync) {
			if(m_pSyncView!=this) {
				m_pSyncView=this;
				//TrackMouse(m_hWnd);
			}
			else
				UpdateViewsAtPoint(point,LHINT_SYNCSHOWCURSOR);
		}
		if(GetDocument()->HasShpPtLayers()) {
			if(m_wndTooltip) {
				ASSERT(m_pMapPtNode);
				MAP_PTNODE *pn=GetPtNode(point.x,point.y);
				if(!pn) DestroyTooltip();
				else if(pn!=m_pMapPtNode) {
					m_pMapPtNode=pn;
					SetTooltipText();
				}
			}
			else {
				//Wait for a hover --
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_HOVER;
				tme.hwndTrack = m_hWnd;
				tme.dwHoverTime = 200;/*HOVER_DEFAULT; ==400 */
				_TrackMouseEvent(&tme);
			}
		}
	}
}

LRESULT CWallsMapView::OnMouseLeave(WPARAM wParam,LPARAM strText)
{
	m_uAction|=CWallsMapDoc::ACTION_NOCOORD;

	m_bTracking=false;

	if(m_wndTooltip) {
		DestroyTooltip();
	}
	if(m_bSync) {
		if(m_bSyncCursor) DrawSyncCursor();
		GetMF()->UpdateViews(this,LHINT_SYNCHIDECURSOR);
		//We'll restore cursors in OnLButtonUp() --
		m_pSyncView=NULL;
	}

	if(m_uAction&(CWallsMapDoc::ACTION_MEASURE|CWallsMapDoc::ACTION_PAN)) {
		OnLButtonUp(0,m_ptDrag);
	}
	m_pViewDrag=NULL;
	return FALSE;
}

LRESULT CWallsMapView::OnMouseHover(WPARAM wParam,LPARAM lParam)
{
	if(m_uAction&(CWallsMapDoc::ACTION_MEASURE|CWallsMapDoc::ACTION_SELECT))
		return FALSE;
	int xPos = (int)(short)LOWORD(lParam);
	int yPos = (int)(short)HIWORD(lParam);
	ASSERT(!m_wndTooltip);
	if(m_wndTooltip) DestroyTooltip();
	if(m_pMapPtNode=GetPtNode(xPos,yPos)) {
		ShowTooltip(CPoint(xPos,yPos+20));
	}
	return FALSE;
}

LRESULT CWallsMapView::OnExitSizeMove( WPARAM , LPARAM )
{
	if(m_iFullView==0 || m_iFullView==1) {
		//CSize szTotal=GetTotalSize();
		CSize szTotal=m_DIB.GetSize();

		CRect client;
		GetClientRect(&client);
		if(client.right>szTotal.cx || client.bottom>szTotal.cy) {
			//Window size increase --
			CWallsMapDoc *pDoc=GetDocument();
			CFltPoint ptGeo;
			ClientPtToGeoPt(CPoint(client.right,client.bottom),ptGeo);
			if((ptGeo.x<pDoc->Extent().r && client.right>szTotal.cx) ||
					(m_fSignY*(ptGeo.y-pDoc->Extent().b)<0.0 && client.bottom>szTotal.cy)) {
				UpdateOnClientPt(client.CenterPoint());
			}
		}
	}
    return 0;
}

BOOL CWallsMapView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    HCURSOR hc;
	if(IsSelecting()) {
		hc=(IsSelecting()==2)?theApp.m_hCursorSelectAdd:theApp.m_hCursorSelect;
	}
	else {
		UINT flag=GetDocument()->m_uActionFlags;
		switch(flag) {
			case CWallsMapDoc::ACTION_ZOOMIN: hc=theApp.m_hCursorZoomIn; break;
			case CWallsMapDoc::ACTION_ZOOMOUT: hc=theApp.m_hCursorZoomOut; break;
			case CWallsMapDoc::ACTION_PAN: hc=theApp.m_hCursorHand; break;
			case CWallsMapDoc::ACTION_MEASURE: hc=theApp.m_hCursorMeasure; break;
			//case CWallsMapDoc::ACTION_SELECT: hc=theApp.m_hCursorSelect; break;
			//default: hc=theApp.m_hCursorCross; break;
			default: hc=theApp.m_hCursorArrow; break;
		}
	}
	::SetCursor(hc);
	return TRUE;
}

void CWallsMapView::CenterOnPtPopup(double scale)
{
	ASSERT(scale>=0.0);
	if(scale<=0.0) return;
	if(m_bSync)	m_pSyncView=this;
	UpdateOnClientPt(m_ptPopup,scale);
	//if(m_bSync)	TrackMouse(m_hWnd);
}

#ifdef _USE_CTR
void CWallsMapView::CenterOnPointDlg(CFltPoint *pfpt)
{
	CFltPoint fpt;
	if(!pfpt) {
		//called from toolbar
		CRect client;
		GetClientRect(&client);
		ClientPtToGeoPt(client.CenterPoint(),fpt);
	}
	else fpt=*pfpt;
	CCenterViewDlg dlg(fpt,this);
	int id=dlg.DoModal();
	if(id!=IDOK) return;
	double f=m_fScale;
	if(dlg.m_bZoom) {
		if(IsScaleOK(f*4)) f*=4;
		else if(IsScaleOK(f*2)) f*=2;
	}

	if(dlg.m_bMark) {
		m_bUseMarkPosition=true;
		m_fMarkPosition=dlg.m_fpt;
	}
	else {
		m_bUseMarkPosition=m_bMarkCenter=false;
	}

	UpdateDIB(dlg.m_fpt,f);
}
#endif

void CWallsMapView::OnCenterOnPoint()
{
#ifdef _USE_CTR
	CFltPoint fpt;
	ClientPtToGeoPt(m_ptPopup,fpt);
	CenterOnPointDlg(&fpt);
#else
	CRect client;
	GetClientRect(&client);

	m_ptBitmapOverlap-=(m_ptPopup-client.CenterPoint());

	if(m_bSync) m_pSyncView=this;

	RefreshPannedView();
#endif
}

void CWallsMapView::OnUpdateCenterOnPosition(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->IsTransformed()==true);
}

void CWallsMapView::OnCenterOnPosition()
{
	CFltPoint fpt;
	if(m_bUseMarkPosition) fpt=m_fMarkPosition;
	else {
		CRect client;
		GetClientRect(&client);
		ClientPtToGeoPt(client.CenterPoint(),fpt);
	}
	CenterOnPointDlg(&fpt);
}

void CWallsMapView::OnContextZoomSpecify()
{
	double zoom=m_fScale/PopupPixelsPerGeoUnit();
	if(zoom<0.0) {
		ASSERT(0);
		return;
	}
	CPercentDlg dlg(zoom*100.0);
	if(dlg.DoModal()==IDOK) {
		double fScale=(dlg.m_fPercent/100.0)*m_fScale/zoom;
		//Note: fScale = fZoom * MaxImagePixelsPerGeoUnit() = (screen pixels / geo-units)
		//Size of document in screen pixels will be fZoom * MaxImagePixelsPerGeoUnit() * (geo_width+geo_height).
		//Let's require this size to be at least 8 for now --
		if(IsScaleOK(fScale))
			CenterOnPtPopup(fScale);
	}
}

#if 0
BOOL CWallsMapView::IsScaleOK(double fScale)
{
	//fScale = proposed screen pixels / geo_units, hence
	//fScale == 512*MaxImagePixelsPerGeoUnit() --> scrn_px == 512 * Img_px, or
	//fScale == 8/geo_perim --> fScale*geo_perim==8 px
	//(fZoom=scrn px/img_px)==1 --> fScale/(img_px/geo_units)==1 --> fScale==MaxImagePixelsPerGeoUnit()
	/*
	double m=MaxImagePixelsPerGeoUnit();
	CMapLayer *pLayer=GetDocument()->LayerSet().m_pLayerMaxImagePixelsPerGeoUnit;
	if(pLayer->LayerType()==TYP_SHP) {
		m=pLayer->IsProjected()?1000.0:100000000.0;
	}
	else m*=512;
	return fScale<=m && fScale*GetDocument()->ExtentHalfPerim()>=8.0;
	*/
	return fScale<=100000000.0 && fScale*GetDocument()->ExtentHalfPerim()>=8.0;
}
#endif

void CWallsMapView::OnContextNozoom()
{
	CenterOnPtPopup(PopupPixelsPerGeoUnit());
}

void CWallsMapView::OnUpdateContextNozoom(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_fScale==PopupPixelsPerGeoUnit());
}

void CWallsMapView::OnContextZoom50()
{
	CenterOnPtPopup(PopupPixelsPerGeoUnit()/2);
}

void CWallsMapView::OnUpdateContextZoom50(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_fScale==PopupPixelsPerGeoUnit()/2);
}

void CWallsMapView::OnContextZoom25()
{
	CenterOnPtPopup(PopupPixelsPerGeoUnit()/4);
}

void CWallsMapView::OnUpdateContextZoom25(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_fScale==PopupPixelsPerGeoUnit()/4);
}

void CWallsMapView::OnContextZoom12()
{
	CenterOnPtPopup(PopupPixelsPerGeoUnit()/8);
}

void CWallsMapView::OnUpdateContextZoom12(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_fScale==PopupPixelsPerGeoUnit()/8);
}

void CWallsMapView::OnContextZoom625()
{
	CenterOnPtPopup(PopupPixelsPerGeoUnit()/16);
}

void CWallsMapView::OnUpdateContextZoom625(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_fScale==PopupPixelsPerGeoUnit()/16);
}

void CWallsMapView::OnContextZoom3125()
{
	CenterOnPtPopup(PopupPixelsPerGeoUnit()/32);
}

void CWallsMapView::OnUpdateContextZoom3125(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_fScale==PopupPixelsPerGeoUnit()/32);
}

void CWallsMapView::OnContextZoomin()
{
	if(IsScaleOK(2*m_fScale))
		CenterOnPtPopup(2*m_fScale);
}

void CWallsMapView::OnContextZoomout()
{
	if(IsScaleOK(m_fScale/2))
		CenterOnPtPopup(m_fScale/2);
}

double CWallsMapView::GetPtSyncCursorFromHint(CObject *pHint)
{
	const CLayerSet &ls_sync=m_pSyncView->GetDocument()->LayerSet();
	const CLayerSet &ls=GetDocument()->LayerSet();
	CFltPoint fpt=*((CSyncHint *)pHint)->fpt;
	double scale=((CSyncHint *)pHint)->fupx; //Requested (screen pixels / geo-units)
	if(ls.IsTransformed()) {
		//If any datum is nonzero (meaning specified), it must be WGS84 (1) or NAD27 (2).
		ASSERT(ls_sync.m_iNad<3 && ls.m_iNad<3);
		//If both are zero, WGS84 is assumed, else if one is zero it's assumed to be same as other --
		int src_datum=ls_sync.m_iNad?(ls_sync.m_iNad==1?geo_WGS84_datum:geo_NAD27_datum):0;
		int dst_datum=ls.m_iNad?(ls.m_iNad==1?geo_WGS84_datum:geo_NAD27_datum):src_datum;
		if(!dst_datum) dst_datum=src_datum=geo_WGS84_datum;
		else if(!src_datum) src_datum=dst_datum;

		if(ls.IsProjected()) {
			//This document is UTM --
			if(!ls_sync.IsProjected()) {

				ASSERT(ls.m_iZone);

				//Convert Lat/Long scale to UTM scale --
				//Requested scale is currently (screen pixels/degrees), so multiply by (degrees/meters) --
				scale/=MetersPerDegreeLat(fpt.y);
				//Convert Lat/Long to this window's datum if required --
				if(dst_datum!=src_datum) {
					//Convert from WGS84 to NAD27 if src_datum==geo_WGS84_datum --
					geo_FromToWGS84(src_datum==geo_WGS84_datum,&fpt.y,&fpt.x,
						(src_datum==geo_WGS84_datum)?dst_datum:src_datum);
				}

				//Convert Lat/Lon to UTM, while forcing zone to ls.m_iZone --
				int zone=ls.m_iZone;
				if(!geo_LatLon2UTM(fpt,&zone,dst_datum))
					return -1;
			}
			else {
				int src_zone=ls_sync.m_iZone;
				int dst_zone=ls.m_iZone;
				ASSERT(src_zone+dst_zone!=0 && abs(src_zone)<=60 && abs(dst_zone)<=60);
				if(!src_zone) src_zone=dst_zone;
				else if(!dst_zone) dst_zone=src_zone;
				if(src_datum!=dst_datum || src_zone!=dst_zone) {
					//Switch to Lat/Lon required first --
					geo_UTM2LatLon(fpt,src_zone,src_datum);
					if(src_datum!=dst_datum) {
						geo_FromToWGS84(src_datum==geo_WGS84_datum,&fpt.y,&fpt.x,
							(src_datum==geo_WGS84_datum)?dst_datum:src_datum);
					}
					//Force to dst_zone (since dst_zone!=0) --
					if(!geo_LatLon2UTM(fpt,&dst_zone,dst_datum))
						return -1;
				}
			}
		}
		else {
			//This document is Lat/Long --
			if(ls_sync.IsProjected()) {
				ASSERT(ls_sync.m_iZone);
				//Convert UTM to Lat/Long --
				geo_UTM2LatLon(fpt,ls_sync.m_iZone,src_datum);
				//and switch datums if required --
				if(src_datum!=dst_datum) {
					geo_FromToWGS84(src_datum==geo_WGS84_datum,&fpt.y,&fpt.x,
						(src_datum==geo_WGS84_datum)?dst_datum:src_datum);
				}
			    //Convert UTM scale to Lat/Long scale --
				scale*=MetersPerDegreeLat(fpt.y);
			}
			else {
				//Both documents are Lat/Long --
				if(src_datum!=dst_datum) {
					geo_FromToWGS84(src_datum==geo_WGS84_datum,&fpt.y,&fpt.x,
						(src_datum==geo_WGS84_datum)?dst_datum:src_datum);
				}
			}
		}
	}
	GeoPtToClientPt(fpt,m_ptSyncCursor);
	return scale;
}

void CWallsMapView::OnUpdate(CView *pView,LPARAM lHint,CObject* pHint)
{
	switch(lHint) {

		case LHINT_CONVERT :
			{
				const CConvertHint &cvt=(const CConvertHint &)*pHint;
				//Convert view's origin and scale in the same way as a point and length in cvt.m_pShp
				//would be converted to cvt.m_iNad and cvt.m_iZone --
				CRect client;
				GetClientRect(&client);
				CFltPoint ptGeoCenter;
				ClientPtToGeoPt(client.CenterPoint(),ptGeoCenter);
				//m_fScale is pixels per geounit (meters or degrees)
				ASSERT(cvt.m_pShp->IsProjected()==(cvt.m_pShp->Zone()!=0));
				if(cvt.m_pShp->IsProjected()!=(cvt.m_iZone!=0)) {
					if(cvt.m_iZone) {
						//Converting from Lat/Long to UTM (degrees to meters)
						//m_fScale is pixels/degrees, so divide by meters/degrees
						m_fScale /= (MetricDistance(ptGeoCenter.y,0,ptGeoCenter.y,1)*1000); //m_fScale = px per meters
						cvt.m_pShp->ConvertPointsTo(&ptGeoCenter,cvt.m_iNad,cvt.m_iZone,1);
					}
					else {
						//Converting from UTM to Lat/Long (meters to degrees)
						//m_fScale is pixels/meters, so multiply by meters/degrees --
						cvt.m_pShp->ConvertPointsTo(&ptGeoCenter,cvt.m_iNad,cvt.m_iZone,1);
						m_fScale *= (MetricDistance(ptGeoCenter.y,0,ptGeoCenter.y,1)*1000); //m_fScale = px per degrees
					}
				}
				else {
					cvt.m_pShp->ConvertPointsTo(&ptGeoCenter,cvt.m_iNad,cvt.m_iZone,1);
				}
				//Now calculate m_fViewOrigin, which is geo position of top left corner of client area
				//assuming ptGeoCenter is geo position of client center --
				m_fViewOrigin.x=ptGeoCenter.x-(client.Width()/2)/m_fScale;
				m_fViewOrigin.y=ptGeoCenter.y+(client.Height()/2)/m_fScale;

				//Clear the view history!
				m_hist.Clear();
			}
			break;

		case LHINT_REFRESH :
			if(!m_bTestPoint || m_bTestPointVisible) {
				RefreshDIB();
			}

		case LHINT_REPAINT :
			Invalidate(FALSE);
			break;

		case LHINT_ENABLEGPS :
			{
				bool bEnable=pGPSDlg && pGPSDlg->HavePos();
				if(bEnable!=m_bTrackingGPS) {
					m_bTrackingGPS=bEnable;
					Invalidate(FALSE);
				}
			}
			break;

		case LHINT_GPSTRACKPT :
			if(m_bTrackingGPS && !m_bTrackRubber && !m_bMeasuring) Invalidate(FALSE);
			break;

		case LHINT_TESTPOINT :
			{
				CFltRect viewExt;
				GetViewExtent(viewExt);
				if(viewExt.IsPtInside(*((CSyncHint *)pHint)->fpt)) {
					m_bTestPointVisible=m_bTestPoint=true;
				}
				else m_bTestPointVisible=false;
			}
			break;

		case LHINT_FITIMAGE :
			OnFitImage();
			break;

		case LHINT_UPDATEDIB :
			RefreshDIB();
			Invalidate(FALSE);
			break;

		case LHINT_PRETILE :
			((CChildFrame *)GetParent())->m_bResizing=true;
			if(m_DIB.GetHandle())
				GetClientCenter(m_fptPopup);
			break;

		case LHINT_POSTTILE :
			if(((CChildFrame *)GetParent())->m_bResizing) {
				((CChildFrame *)GetParent())->m_bResizing=false;
				if(!m_DIB.GetHandle()) {
					ASSERT(m_iFullView==-1);
					FitImage(TRUE);
				}
				else {
					GeoPtToClientPt(m_fptPopup,m_ptPopup);
					//OnCenterOnPoint();
					CRect client;
					GetClientRect(&client);

					m_ptBitmapOverlap-=(m_ptPopup-client.CenterPoint());

					if(m_bSync) m_pSyncView=this;

					RefreshPannedView();
				}
				if(m_bSync) {
					if(m_bSyncCursor) DrawSyncCursor();
					m_pSyncView=NULL;
				}
				/*
				if(m_bSync && m_pSyncView==this) {
					if(m_bSyncCursor) DrawSyncCursor();
					GetMF()->UpdateViews(this,LHINT_SYNCHIDECURSOR);
					m_pSyncView=NULL;
				}
				*/
			}
			else {
				RefreshPannedView();
				//m_uAction=CWallsMapDoc::ACTION_PANNED;
				//Invalidate(FALSE);
			}
			break;

		case LHINT_UPDATESIZE :
			PostMessage(WM_EXITSIZEMOVE);
			break;

		case LHINT_SYNCSTART :
			{
				ASSERT(pView && pView!=this);
				CWallsMapDoc *pDoc=GetDocument();
				if(!pDoc->IsSyncCompatible(((CWallsMapView *)pView)->GetDocument())) break;

				ASSERT(m_pSyncView && m_pSyncView!=this && m_pSyncView==(CWallsMapView *)pView);
				//ASSERT(!m_bSync);

				//The new controlling view, pView, has just been painted.
				//We are requesting this view to be repainted with a new center.
				//When the window is redrawn (in OnDraw) a cursor will be also
				//be displayed --

				//m_fScale is this view's current scale, (screen pixels/ geo_units).
				//ps->fupx is the requested scale. ps->fpt is the requested
				//geographic point to center on. We must express that in current client
				//coordinates --

				double scale=GetPtSyncCursorFromHint(pHint);
				if(scale>0) {
					m_bSync=TRUE;

					/*if(m_fScale==scale) { //Haven't tested this -- not really worth it:
						m_ptBitmapOverlap-=(m_ptSyncCursor-GetClientCenter());
						RefreshPannedView();
					}
					else */
					UpdateOnClientPt(m_ptSyncCursor,scale);
				}
			}
			break;

		case LHINT_SYNCSTOP :
			if(m_bSync) {
				ASSERT(!m_bSyncCursor || m_pSyncView!=this); 
				if(m_bSyncCursor) DrawSyncCursor();
				m_bSync=FALSE;
			}
			break;

		case LHINT_SYNCSHOWCURSOR :
			if(m_bSync) {
				if(m_bSyncCursor) DrawSyncCursor();
				if(m_pSyncView) {
					ASSERT(m_pSyncView!=this && m_pSyncView==(CWallsMapView *)pView);
					if(GetPtSyncCursorFromHint(pHint)>=0) {
						DrawSyncCursor();
					}
				}
			}
			break;

		case LHINT_SYNCHIDECURSOR :
			if(m_bSync && m_bSyncCursor) DrawSyncCursor();
			break;

		case LHINT_SYNCPAN :
			if(m_bSync) {
				m_ptBitmapOverlap+=*(CPoint *)(((CSyncHint *)pHint)->fpt);
				m_uAction=CWallsMapDoc::ACTION_PANNED;
				Invalidate(FALSE);
			}
			break;

		default	: CView::OnUpdate(pView,lHint,pHint);
	}
}

void CWallsMapView::UpdateViewsAtPoint(const CPoint &point,LPARAM lHint)
{
	CFltPoint fpt;
	ClientPtToGeoPt(point,fpt);
	CSyncHint hint(&fpt,0);
	GetMF()->UpdateViews(this,lHint,&hint);
}

void CWallsMapView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if(m_pScaleWnd && cy!=m_pScaleWnd->m_bottom)
		SetScalePos();

	if(m_iFullView>=0 && !((CChildFrame *)GetParent())->m_bResizing) {

		// TODO: Add your message handler code here
		if(nType==SIZE_RESTORED && m_iFullView<2) {
			PostMessage(WM_EXITSIZEMOVE);
		}
	}
}

void CWallsMapView::OnUpdateSyncOnpoint(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bSync || !CMainFrame::m_bSync && GetMF()->NumSyncCompatible(GetDocument())>0);
	pCmdUI->SetCheck(m_bSync);
}

void CWallsMapView::OnSyncOnpoint()
{
	if(m_bSync) SyncStop();
	else {
		ASSERT(!CMainFrame::m_bSync && !m_pSyncView);
		CMainFrame::m_bSync=m_bSync=TRUE;
		m_pSyncView=this;
		UpdateOnClientPt(m_ptPopup);
		//TrackMouse(m_hWnd);
	}
}

void CWallsMapView::FitImage(BOOL bVisible)
{
 	//bVisible==2 : zoom to specific layer

	if((GetDocument()->m_uEnableFlags&NTL_FLG_RESTOREVIEW) && !m_bFirstViewDisplayed) {
		if(UpdateRestoredView()) 
			return;
	}
	CRect client;
	GetClientRect(&client);
	double fScale;  //<window width>/<geographical extent or image pixels>
	CFltPoint geoSize;
	CFltRect geoExt;
	CLayerSet &lset=GetDocument()->LayerSet();
	
	lset.GetExtent(&geoExt,bVisible);

_refit:

	geoSize=geoExt.Size();

	if(geoSize.x*client.bottom>=geoSize.y*client.right) {
		//Fit width
		fScale=client.right/geoSize.x;
	}
	else {
		fScale=client.bottom/geoSize.y;
	}

	if(bVisible!=2 && fScale>MaxImagePixelsPerGeoUnit())
		fScale=MaxImagePixelsPerGeoUnit();

	if(bVisible==1) {
		CFltRect fRect(geoExt);
		lset.GetScaledExtent(geoExt,fScale);
		if(memcmp(&fRect,&geoExt,sizeof(CFltRect)))
			goto _refit;
	}

	UpdateDIB(geoExt.Center(),fScale);
}

void CWallsMapView::OnFitImage()
{
	FitImage(TRUE);
}

void CWallsMapView::OnFitExtent()
{
	FitImage(FALSE);
}

void CWallsMapView::OnUpdateFitImage(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->FirstVisibleIndex()>=0);
}

void CWallsMapView::DrawSyncCursor()
{
	CRect client;
	GetClientRect(&client);
	CClientDC dc(this);
	dc.SetROP2(R2_NOT);
    dc.SetBkMode(TRANSPARENT);
    dc.MoveTo(0,m_ptSyncCursor.y);
	dc.LineTo(client.right,m_ptSyncCursor.y);
    dc.MoveTo(m_ptSyncCursor.x,0);
	dc.LineTo(m_ptSyncCursor.x,client.bottom);
	m_bSyncCursor=!m_bSyncCursor;
}

BOOL CWallsMapView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	if(zDelta) {
		if(m_bSync && m_pSyncView && m_pSyncView!=this) {
			m_pSyncView->GetParentFrame()->ActivateFrame();
			return m_pSyncView->OnMouseWheel(nFlags,zDelta,pt);
		}
		CRect client;
		GetClientRect(&client);

		if(CMainFrame::IsPref(PRF_ZOOM_FROMCURSOR)) {
			ScreenToClient(&pt);
			if(!client.PtInRect(pt)) return FALSE;
			m_ptPopup=pt;
		}
		else {
			m_ptPopup=client.CenterPoint();
		}

		if((zDelta<0 && !CMainFrame::IsPref(PRF_ZOOM_FORWARDOUT)) || (zDelta>0 && CMainFrame::IsPref(PRF_ZOOM_FORWARDOUT)))
			OnContextZoomout();
		else
			OnContextZoomin();

		return TRUE;
	}
	return FALSE;
}

void CWallsMapView::OnDestroy()
{
	if(m_bSync) SyncStop();
	if(m_pScaleWnd) {
		m_pScaleWnd->DestroyWindow();
		delete m_pScaleWnd;
		m_pScaleWnd=NULL;
	}
}

void CWallsMapView::OnOptionsShowoutlines()
{
	CWallsMapDoc *pDoc=GetDocument();
	ASSERT(pDoc->IsTransformed());
	pDoc->m_bDrawOutlines=(pDoc->m_bDrawOutlines==1)?0:1;
	pDoc->RepaintViews();
}

void CWallsMapView::OnUpdateOptionsShowoutlines(CCmdUI *pCmdUI)
{
	BOOL bEnable=GetDocument()->IsTransformed();
	pCmdUI->Enable(bEnable);
	pCmdUI->SetCheck(bEnable && GetDocument()->m_bDrawOutlines==1);
}


void CWallsMapView::OnOptionsShowselected()
{
	CWallsMapDoc *pDoc=GetDocument();
	ASSERT(pDoc->IsTransformed());
	pDoc->m_bDrawOutlines=(pDoc->m_bDrawOutlines==2)?0:2;
	pDoc->RepaintViews();
}

void CWallsMapView::OnUpdateOptionsShowselected(CCmdUI *pCmdUI)
{
	BOOL bEnable=GetDocument()->IsTransformed();
	pCmdUI->Enable(bEnable);
	pCmdUI->SetCheck(bEnable && GetDocument()->m_bDrawOutlines==2);
}

void CWallsMapView::OnActivateView(BOOL bActivate, CView* pActiveView, CView* pDeactiveView)
{
	CView::OnActivateView(bActivate, pActiveView, pDeactiveView);

	m_pViewDrag=this;
	if (bActivate && hPropHook && GetDocument()!=pLayerSheet->GetDoc())
	{
		pLayerSheet->SendMessage(WM_PROPVIEWDOC,(WPARAM)GetDocument(),(LPARAM)LHINT_NEWDOC);
	}
}

void CWallsMapView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CView::OnRButtonDown(nFlags, point);
}

static void DelMenuItem(CMenu *popup,int pos,int cnt)
{
	while(cnt--) popup->DeleteMenu(pos,MF_BYPOSITION);
}

void CWallsMapView::OnRButtonUp(UINT nFlags, CPoint point)
{
	if(m_bSync) {
		if(m_bSyncCursor) DrawSyncCursor();
		GetMF()->UpdateViews(this,LHINT_SYNCHIDECURSOR);
		//We'll restore cursors in OnRButtonUp() --
		m_pSyncView=this;
	}

	DestroyTooltip();

	//NOTE: point inits are client pixels --
	CMenu menu;
	HMENU hPopup=NULL;

	if(GetDocument()->IsTransformed()) {
		CRect scRect;
		m_pScaleWnd->GetWindowRect(&scRect);
		ScreenToClient(&scRect);
		if(scRect.PtInRect(point)) {
			if(menu.LoadMenu(IDR_SCALE_CONTEXT)) {
				CMenu* pPopup = menu.GetSubMenu(0);
				ASSERT(pPopup != NULL);
				pPopup->CheckMenuItem(ID_SCALEUNITS_FEET,GetDocument()->IsFeetUnits()?(MF_CHECKED|MF_BYCOMMAND):(MF_UNCHECKED|MF_BYCOMMAND));
				pPopup->CheckMenuItem(ID_SCALEUNITS_METERS,GetDocument()->IsFeetUnits()?(MF_UNCHECKED|MF_BYCOMMAND):(MF_CHECKED|MF_BYCOMMAND));
				ClientToScreen(&point);
				VERIFY(pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,GetParentFrame()));
				m_bTracking=false;
				if(m_bSync)	m_pSyncView=this;
			}
			return;
		}
	}

	m_pMapPtNode=GetPtNode(point.x,point.y);

	if(menu.LoadMenu(IDR_VIEW_CONTEXT))
	{
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);

		CFltPoint ptGeo;
		ClientPtToGeoPt(point,ptGeo);
		bool bTrans=GetDocument()->IsTransformed();
		bool bValid=bTrans && GetDocument()->IsValidGeoPt(ptGeo);

		if(!pGPSDlg || !pGPSDlg->HavePos()) {
			DelMenuItem(pPopup,12,5);
		}

		if(!bValid || !GetDocument()->IsGeoRefSupported()) {
			//delete divider, GE, and web mapping section
			DelMenuItem(pPopup,9,3);
		}
		else {
			CString s;
			if(GetMF()->GetWebMapName(s)) {
				pPopup->ModifyMenu(ID_LAUNCH_WEBMAP,MF_BYCOMMAND|MF_STRING,ID_LAUNCH_WEBMAP,s);
			}
			if(!GE_IsInstalled()) {
				pPopup->DeleteMenu(ID_LAUNCH_GE,MF_BYCOMMAND);
			}
		}

		if(ImagePixelsPerGeoUnit(ptGeo)<=0.0) {
			pPopup->DeleteMenu(2,MF_BYPOSITION); //zoom to level...
			pPopup->DeleteMenu(ID_CONTEXT_NOZOOM,MF_BYCOMMAND); //zoom 1:1
			pPopup->DeleteMenu(ID_IMAGE_OPACITY,MF_BYCOMMAND);
		}
		else if(!bValid) {
			pPopup->DeleteMenu(ID_IMAGE_OPACITY,MF_BYCOMMAND);
		}

		//else pPopup->DeleteMenu(ID_OPTIONS_BKCOLOR,MF_BYCOMMAND);

		int npos=0;

		if(bValid && m_pMapPtNode) {
			char text[SHP_TOOLTIP_SIZ];
			GetTooltip(m_pMapPtNode,text);
			CString msg;
			msg.Format("Select points near %s",text[0]?text:"cursor");
			pPopup->InsertMenu(0,MF_BYPOSITION,MF_SEPARATOR);
			pPopup->InsertMenu(0,MF_BYPOSITION,ID_EDIT_SHAPE,(LPCSTR)msg); //will relain the first item
			npos=1;
		}

		int iTyp=(app_pShowDlg && app_pShowDlg->m_pDoc==GetDocument() &&
				app_pShowDlg->SelectedLayer() && app_pShowDlg->SelectedLayer()->IsShpEditable());

		if(bValid && (iTyp && app_pShowDlg->IsAddingRec() || GetDocument()->GetAppendableLayers(NULL))) {
			CString s;

			if(iTyp) { //appendable layer or item is selected --
				iTyp=app_pShowDlg->GetRelocateStr(s);
				//iTyp=1(add pt to root),2(add pt similar to sel),3(relocate sel),or 4(relocate new point)
				if(!npos)
					pPopup->InsertMenu(0,MF_BYPOSITION,MF_SEPARATOR);
				pPopup->InsertMenu(npos++,MF_BYPOSITION,(iTyp>2)?ID_RELOCATE_SHAPE:ID_APPENDLAYER_N,s);
				if(iTyp==3)
					pPopup->InsertMenu(npos++,MF_BYPOSITION,ID_APPENDLAYER_N,"Add similar point");
			}

			if(!iTyp || iTyp!=4 &&
					!(iTyp==1 && CLayerSet::vAppendLayers.size()==1)/* == !already prompted */) {
				if(!npos)
					pPopup->InsertMenu(0,MF_BYPOSITION,MF_SEPARATOR);
				if(CLayerSet::vAppendLayers.size()==1) {
					s.Format("Add new point to %s",CLayerSet::vAppendLayers[0]->Title());
					pPopup->InsertMenu(npos++,MF_BYPOSITION,ID_APPENDLAYER_0,s);
				}
				else {
					hPopup=CreatePopupMenu();
					if(hPopup) {
						CShpLayer *pLayer=(iTyp==1)?app_pShowDlg->SelectedLayer():NULL;
						UINT id=ID_APPENDLAYER_0;
						for(VEC_TREELAYER::const_iterator p=CLayerSet::vAppendLayers.begin();p!=CLayerSet::vAppendLayers.end();p++,id++) {
							if(*p!=pLayer)
								VERIFY(AppendMenu(hPopup,MF_STRING,id,(LPCTSTR)(*p)->Title()));
						}
						pPopup->InsertMenu(npos++,MF_BYPOSITION|MF_POPUP,(UINT_PTR)hPopup,"Add new point to layer...");
					}
				}
			}
		}

		if(!bValid) {
			pPopup->DeleteMenu(ID_CTR_ONPOINT,MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_SYNC_ONPOINT,MF_BYCOMMAND);
		}

		if(!GetDocument()->ShpLayersVisible()) {
			pPopup->DeleteMenu(ID_HIDEMARKERS,MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_HIDELABELS,MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_TOOL_SELECT,MF_BYCOMMAND);
		}
		else {
			if(!GetDocument()->HasShpPtLayers()) {
				pPopup->DeleteMenu(ID_TOOL_SELECT,MF_BYCOMMAND);
			}
			if(GetDocument()->MarkersHidden()) {
				pPopup->ModifyMenu(ID_HIDEMARKERS,MF_BYCOMMAND,ID_SHOW_MARKERS,"Show markers");
			}
			if(GetDocument()->LabelsHidden()) {
				pPopup->ModifyMenu(ID_HIDELABELS,MF_BYCOMMAND,ID_SHOW_LABELS,"Show labels");
			}
		}
		m_ptPopup=point;
		ClientToScreen(&point);
		VERIFY(pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,GetParentFrame()));
		m_bTracking=false;
		//if(m_bSync)	TrackMouse(m_hWnd);
		if(m_bSync)	m_pSyncView=this;
	}

	if(hPopup) DestroyMenu(hPopup);

	//CView::OnRButtonUp(nFlags, point);
}

void CWallsMapView::OnHideLabels()
{
	GetDocument()->ToggleLabels();
}

void CWallsMapView::OnUpdateHideLabels(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->LabelLayersVisible());
}

void CWallsMapView::OnUpdateShowLabels(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->LabelLayersVisible());
	pCmdUI->SetCheck(!GetDocument()->LabelsHidden());
}

void CWallsMapView::OnHideMarkers()
{
	GetDocument()->ToggleMarkers();
}

void CWallsMapView::OnUpdateHideMarkers(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->MarkerLayersVisible());
}

void CWallsMapView::OnUpdateShowMarkers(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->MarkerLayersVisible());
	pCmdUI->SetCheck(!GetDocument()->MarkersHidden());
}

void CWallsMapView::ShowTooltip(CPoint point)
{
	//The following fails for WINVER=0x0501 !!

	//unsigned int uid = 0;       // for ti initialization

	char text[SHP_TOOLTIP_SIZ];

	ASSERT(!m_wndTooltip);

	// CREATE A TOOLTIP WINDOW
	m_wndTooltip = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,		
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, NULL, NULL);

	ASSERT(m_wndTooltip);
	if(!m_wndTooltip) return;
	// INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE
	GetTooltip(m_pMapPtNode,text);
	memset(&m_ToolInfo,0,sizeof(TOOLINFO));
	m_ToolInfo.cbSize = sizeof(TOOLINFO)
		//***HACK TO ALLOW TOOLTIP DISPLAY WHEN _WIN32_WINNT>=0x501 !!!! Seems necessary only here. Hopefully this is harmless.
		-sizeof(LPVOID);
	m_ToolInfo.uFlags = TTF_TRACK; // | TTF_ABSOLUTE doesn't help with 0x501;
	//m_ToolInfo.hwnd = NULL;
	//m_ToolInfo.hinst = NULL;
	//m_ToolInfo.uId = uid;
	m_ToolInfo.lpszText = text;		
	// ToolTip control will cover the whole window
	//m_ToolInfo.rect.left = 0;
	//m_ToolInfo.rect.top = 0;
	//m_ToolInfo.rect.right = 0;
	//m_ToolInfo.rect.bottom = 0;

#if 0
	//part pf TOOLINFO structure --
	#if (_WIN32_IE >= 0x0300)
		LPARAM lParam;
	#endif
	#if (_WIN32_WINNT >= 0x0501)
		void *lpReserved;
	#endif
#endif

	// SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW
	VERIFY(::SendMessage(m_wndTooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &m_ToolInfo));
	VERIFY(TrackMouse(m_hWnd));
	m_bTracking=true;
	ClientToScreen(&point);

	::SendMessage(m_wndTooltip, TTM_TRACKPOSITION, 0, (LPARAM)(DWORD) MAKELONG(point.x, point.y));

	VERIFY(::SendMessage(m_wndTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)(LPTOOLINFO) &m_ToolInfo));
}

void CWallsMapView::SetTooltipText()
{	
	if(m_wndTooltip)
	{
		char text[SHP_TOOLTIP_SIZ];
		GetTooltip(m_pMapPtNode,text);
		m_ToolInfo.lpszText = text;	
		::SendMessage(m_wndTooltip, TTM_SETTOOLINFO, 0, (LPARAM)(LPTOOLINFO) &m_ToolInfo);   	
	}
}

void CWallsMapView::DestroyTooltip()
{
	if(m_wndTooltip) {
		::DestroyWindow(m_wndTooltip);
		m_wndTooltip = NULL;
		m_pMapPtNode = NULL;
	}
}

void CWallsMapView::OnGoBackward()
{
	HIST_REC &rec=m_hist.GetPrev();
	m_bNoHist=TRUE;
	if(m_bSync)	m_pSyncView=this;
	UpdateDIB(rec.geoCtr,rec.scale);
	m_hist.dec_pos(m_hist.pos_current);
	//if(m_bSync)	TrackMouse(m_hWnd);
	m_bNoHist=FALSE;
}

void CWallsMapView::OnUpdateGoBackward(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_hist.IsPrev());
}

void CWallsMapView::OnGoForward()
{
	int end=m_hist.pos_end;
	HIST_REC &rec=m_hist.GetNext();
	m_bNoHist=TRUE;
	if(m_bSync)	m_pSyncView=this;
	UpdateDIB(rec.geoCtr,rec.scale);
	m_hist.inc_pos(m_hist.pos_current);
	//if(m_bSync)	TrackMouse(m_hWnd);
	m_bNoHist=FALSE;
}

void CWallsMapView::OnUpdateGoForward(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_hist.IsNext());
}

void CWallsMapView::OnToolMeasure()
{
	GetDocument()->OnToolMeasure();
}
void CWallsMapView::OnUpdateToolMeasure(CCmdUI *pCmdUI)
{
	GetDocument()->OnUpdateViewPan(pCmdUI);
}

void CWallsMapView::OnOptionsBkcolor()
{
	CWallsMapDoc *pDoc=GetDocument();
	CBackClrDlg dlg(pDoc->GetTitle(),pDoc->m_clrBack);
	if(dlg.DoModal()==IDOK && pDoc->m_clrBack!=dlg.m_cr) {
		::DeleteObject(pDoc->m_hbrBack);
		pDoc->m_clrBack=dlg.m_cr;
		if(dlg.m_cr==RGB(0,0,0) || dlg.m_cr==RGB(255,255,255))
			pDoc->m_hbrBack=(HBRUSH)::GetStockObject(dlg.m_cr?WHITE_BRUSH:BLACK_BRUSH);
		else
			pDoc->m_hbrBack=::CreateSolidBrush(dlg.m_cr);
		pDoc->RefreshViews();
		pDoc->SetChanged();
	}
}

void CWallsMapView::OnAverageColors()
{
	GetDocument()->m_uEnableFlags^=NTL_FLG_AVERAGE;
	GetDocument()->SetChanged();
	GetMF()->UpdateViews(NULL,LHINT_UPDATEDIB);
}

void CWallsMapView::OnUpdateAverageColors(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck((GetDocument()->m_uEnableFlags&NTL_FLG_AVERAGE)!=0);
}

void CWallsMapView::OnSaveHistory()
{
	GetDocument()->m_uEnableFlags^=NTL_FLG_SAVEHISTORY;
	GetDocument()->SetChanged();
}

void CWallsMapView::OnUpdateSaveHistory(CCmdUI *pCmdUI)
{
	BOOL bNTL=GetDocument()->m_bExtensionNTL!=0;
	pCmdUI->Enable(bNTL);
	pCmdUI->SetCheck(bNTL && (GetDocument()->m_uEnableFlags&NTL_FLG_SAVEHISTORY)!=0);
}

void CWallsMapView::OnSaveLayers()
{
	GetDocument()->m_uEnableFlags^=NTL_FLG_SAVELAYERS;
	GetDocument()->SetChanged();
}

void CWallsMapView::OnUpdateSaveLayers(CCmdUI *pCmdUI)
{
	BOOL bNTL=GetDocument()->m_bExtensionNTL!=0;
	pCmdUI->Enable(bNTL);
	pCmdUI->SetCheck(bNTL && (GetDocument()->m_uEnableFlags&NTL_FLG_SAVELAYERS)!=0);
}

void CWallsMapView::OnRestoreView()
{
	GetDocument()->m_uEnableFlags^=NTL_FLG_RESTOREVIEW;
	if(GetDocument()->m_uEnableFlags&NTL_FLG_RESTOREVIEW) {
		CRestorePosition *pRP=GetDocument()->GetRestorePosition();
		if(pRP) {
			CRect crView;
			GetClientRect(&crView);
			//oldrange = pRP->height/pRP->scale = crView.Height()/newScale
			ClientPtToGeoPt(crView.CenterPoint(),pRP->ptGeo);
			pRP->geoHeight=crView.Height()/m_fScale;
			pRP->uStatus=2;
			m_bFirstViewDisplayed=true;
		}
	}
	else {
		GetDocument()->DeleteRestorePosition();
		m_bFirstViewDisplayed=false;
	}
	GetDocument()->SetChanged();
}

void CWallsMapView::OnUpdateRestoreView(CCmdUI *pCmdUI)
{
	BOOL bNTL=GetDocument()->m_bExtensionNTL!=0;
	pCmdUI->Enable(bNTL);
	pCmdUI->SetCheck(bNTL && (GetDocument()->m_uEnableFlags&NTL_FLG_RESTOREVIEW)!=0);
}

void CWallsMapView::EditShape(int x,int cx,int y,int cy)
{
	if(!CheckShowDlg()) return;
	//if(app_pShowDlg && app_pShowDlg->CancelSelChange()) return;
	CWallsMapDoc *pDoc=GetDocument();

	bool bMerging=IsSelecting()==2 && app_pShowDlg && app_pShowDlg->m_pDoc==pDoc &&
		app_pShowDlg->NumSelected()>0;

	VEC_PTNODE vec_pn;
	VEC_SHPREC &vec_shprec=CLayerSet::vec_shprec_srch;
	ASSERT(!vec_shprec.size());

	int nPts=m_ptNode.GetVecPtNode(vec_pn,x-m_ptBitmapOverlap.x,cx,y-m_ptBitmapOverlap.y,cy,bMerging);

	//Build new vector of CShpLayer pointers and record numbers for this document --

	if(nPts) {
		if(bMerging)
			nPts+=app_pShowDlg->NumSelected();

		if(pDoc->SelectionLimited(nPts)) {
			return;
		}
		pDoc->InitLayerIndices();
	}
	else if(bMerging) return;

	if(bMerging) {
		vec_shprec.reserve(nPts);
		pDoc->StoreSelectedRecs();
	}
	else {
		ASSERT(!vec_shprec.size());
		vec_shprec.reserve(nPts);
		if(nPts>1)
			CShpLayer::Sort_Vec_ptNode(vec_pn);
	}

	//Now add the new selected points --
	for(VEC_PTNODE::iterator it=vec_pn.begin();it!=vec_pn.end();it++) {
		//ASSERT(!(*it)->pLayer->IsRecSelected((*it)->rec)); no longer true
		vec_shprec.push_back(SHPREC((*it)->pLayer,(*it)->rec));
	}

	if(bMerging && vec_shprec.size()>1) {
		CShpLayer::Sort_Vec_ShpRec(vec_shprec);
	}

	ASSERT(nPts==vec_shprec.size());

	if(pDoc->ReplaceVecShprec()) {
		//also resets selected record flags
		if(!app_pShowDlg)
			VERIFY(app_pShowDlg==new CShowShapeDlg(pDoc));
		else app_pShowDlg->ReInit(pDoc);
	}
	else {
		ASSERT(0);
	}
}

void CWallsMapView::OnEditShape()
{
	ASSERT(m_pMapPtNode && !m_wndTooltip);
	EditShape(m_ptPopup.x,VNODE_NEAR_LIMIT_SQRT,m_ptPopup.y,VNODE_NEAR_LIMIT_SQRT);
}

void CWallsMapView::OnRelocateShape()
{
	ASSERT(app_pShowDlg);
	CFltPoint fpt;
	ClientPtToGeoPt(m_ptPopup,fpt);
	app_pShowDlg->Relocate(fpt);
}

void CWallsMapView::OnFindlabel()
{
	if(!CheckShowDlg()) return;

	//if(app_pShowDlg && app_pShowDlg->CancelSelChange()) return;

	CWallsMapDoc *pDoc=GetDocument();

	ASSERT(pDoc->HasSearchableLayers());

	CAdvSearchDlg dlg(pDoc,AfxGetMainWnd());

	if(IDOK==dlg.DoModal()) {

		//pDoc->ReplaceVecShprec();

		if(!app_pShowDlg) {
			VERIFY(app_pShowDlg==new CShowShapeDlg(pDoc));
		}
		else {
			app_pShowDlg->ReInit(pDoc);
		}
	}
}

void CWallsMapView::OnUpdateFindlabel(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->HasSearchableLayers());
}

void CWallsMapView::OnAddLayer()
{
	GetDocument()->AddFileLayers();
}

void CWallsMapView::OnUpdateAddLayer(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->IsTransformed());
}

void CWallsMapView::OnSelectEdited()
{
	if(!CheckShowDlg()) return;

	//if(app_pShowDlg && app_pShowDlg->CancelSelChange()) return;

	CSelEditedDlg dlg(this);
	if(dlg.DoModal()==IDCANCEL) return;

	if(!app_pShowDlg)
		VERIFY(app_pShowDlg==new CShowShapeDlg(GetDocument()));
	else app_pShowDlg->ReInit(GetDocument());
}

void CWallsMapView::OnUpdateSelectEdited(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDocument()->EditedShapesVisible());
}

void CWallsMapView::OnAddShape(UINT id)
{
	if(!CheckShowDlg()) return; //calls app_pShowDlg->CancelSelChange()

	ASSERT(app_pShowDlg || id<ID_APPENDLAYER_N);

	CShpLayer *pLayer=(id<ID_APPENDLAYER_N)?(CShpLayer *)CLayerSet::vAppendLayers[id-ID_APPENDLAYER_0]:app_pShowDlg->SelectedLayer();
	ASSERT(pLayer);

	CWallsMapDoc *pDoc=GetDocument();
	CFltPoint fpt;
	ClientPtToGeoPt(m_ptPopup,fpt);

	if(pLayer->m_iZoneOrg) {
		int zone=pDoc->LayerSet().m_iZone;
		int nad=pDoc->LayerSet().m_iNad;
		if(zone) {
			CFltPoint fgeo(fpt);
			if(!GetConvertedPoint(fgeo,nad,0,nad,zone)) {
				AfxMessageBox("Point is outside the map's valid range.");
				return;
			}
			zone=GetZone(fgeo);
		}
		else zone=GetZone(fpt);
		if(abs(abs(zone)-abs(pLayer->m_iZoneOrg))>1) {
			pLayer->OutOfZoneMsg(zone);
			return;
		}
	}

	if(!(pLayer)->SaveAllowed(2)) return;
	
	if(id<ID_APPENDLAYER_N) {
		//we've selected from the alternate submenu --
		if(!app_pShowDlg)
			VERIFY(app_pShowDlg==new CShowShapeDlg(pDoc));
		else {
			if(pDoc==app_pShowDlg->m_pDoc) {
				pDoc->ClearVecShprec(); //definitely necessary
			}
			app_pShowDlg->ReInit(pDoc);
		}
		app_pShowDlg->AddShape(fpt,pLayer);
	}
	else {
		//Add similar point --
		ASSERT(app_pShowDlg->m_pDoc==pDoc);
		ASSERT(pLayer->IsAppendable());
		app_pShowDlg->AddShape(fpt,0);
	}
}

void CWallsMapView::OnToolSelect()
{
	GetDocument()->OnToolSelect();
}

void CWallsMapView::OnUpdateToolSelect(CCmdUI *pCmdUI)
{
	GetDocument()->OnUpdateToolSelect(pCmdUI);
}

void CWallsMapView::OnSelectAdd()
{
	GetDocument()->OnSelectAdd();
}

void CWallsMapView::OnUpdateSelectAdd(CCmdUI *pCmdUI)
{
	GetDocument()->OnUpdateSelectAdd(pCmdUI);
}

void CWallsMapView::OnExportPng()
{
	CDlgExportImg dlg(this);
	dlg.DoModal();
}

void CWallsMapView::OnScaleUnitsToggle()
{
	GetDocument()->ToggleFeetUnits();
	DrawScale();
}

void CWallsMapView::OnElevUnitsToggle()
{
	GetDocument()->ToggleElevUnits();
}

void CWallsMapView::OnDatumToggle()
{
	GetDocument()->ToggleDatum();
}

void CWallsMapView::OnUpdateScaleUnitsFeet(CCmdUI *pCmdUI)
{
	if(GetDocument()->IsTransformed()) {
		pCmdUI->Enable(1);
		pCmdUI->SetCheck(GetDocument()->IsFeetUnits());
	}
	else pCmdUI->Enable(0);
}

void CWallsMapView::OnUpdateScaleUnitsMeters(CCmdUI *pCmdUI)
{
	if(GetDocument()->IsTransformed()) {
		pCmdUI->Enable(1);
		pCmdUI->SetCheck(!GetDocument()->IsFeetUnits());
	}
	else pCmdUI->Enable(0);
}

void CWallsMapView::OnUpdateElevUnitsFeet(CCmdUI *pCmdUI)
{
	if(GetDocument()->HasNteLayers()) {
		pCmdUI->Enable(1);
		pCmdUI->SetCheck(GetDocument()->IsElevUnitsFeet());
	}
	else pCmdUI->Enable(0);
}

void CWallsMapView::OnUpdateElevUnitsMeters(CCmdUI *pCmdUI)
{
	if(GetDocument()->HasNteLayers()) {
		pCmdUI->Enable(1);
		pCmdUI->SetCheck(!GetDocument()->IsElevUnitsFeet());
	}
	else pCmdUI->Enable(0);
}

void CWallsMapView::OnUpdateDatumToggle1(CCmdUI *pCmdUI)
{
	if(GetDocument()->IsGeoRefSupported()) {
		pCmdUI->Enable(1);
		int iNad=GetDocument()->LayerSet().m_iNad;
		pCmdUI->SetCheck(GetDocument()->IsDatumToggled()?(iNad==2):(iNad==1));
	}
	else pCmdUI->Enable(0);
}

void CWallsMapView::OnUpdateDatumToggle2(CCmdUI *pCmdUI)
{
	if(GetDocument()->IsGeoRefSupported()) {
		pCmdUI->Enable(1);
		int iNad=GetDocument()->LayerSet().m_iNad;
		pCmdUI->SetCheck(GetDocument()->IsDatumToggled()?(iNad==1):(iNad==2));
	}
	else pCmdUI->Enable(0);
}

void CWallsMapView::OnLaunchWebMap()
{
	ASSERT(GetDocument()->IsGeoRefSupported() && GetDocument()->IsTransformed());
	CFltPoint fpt;
	ClientPtToGeoPt(m_ptPopup,fpt);
	GetDocument()->LayerSet().PrepareLaunchPt(fpt);
	GetMF()->Launch_WebMap(fpt,"WallsMap_Pt");
}

void CWallsMapView::OnOptionsWebMap()
{
	CString s(CMainFrame::m_csWebMapFormat);
	int idx=CMainFrame::m_idxWebMapFormat;
	BOOL bRet=GetMF()->GetWebMapFormatURL(TRUE);
	if(bRet) {
		OnLaunchWebMap();
		if(bRet>1) {
			CMainFrame::m_csWebMapFormat=s;
			CMainFrame::m_idxWebMapFormat=idx;
		}
	}
}

void CWallsMapView::OnLaunchGE()
{
	ASSERT(GetDocument()->IsGeoRefSupported() && GetDocument()->IsTransformed());
	CString kmlPath;
	if(!GetKmlPath(kmlPath,"WallsMap_Pt")) return;
	CFltPoint fpt;
	ClientPtToGeoPt(m_ptPopup,fpt);
	GetDocument()->LayerSet().PrepareLaunchPt(fpt);
	GE_POINT pt("WallsMap_Pt",0,fpt.y,fpt.x);
	GE_Launch(NULL,kmlPath,&pt,1,1); //fly direct
}

void CWallsMapView::CenterOnGeoPoint(const CFltPoint &fpt,double fZoom)
{
	fZoom*=m_fScale;
	double m=MetersPerPixel(fZoom); //fZoom too large == m too small
	if(m<MIN_METERSPERPIXEL) {
		fZoom*=m/MIN_METERSPERPIXEL;
	}
	UpdateDIB(fpt,fZoom);
}

void CWallsMapView::FitShpExtent(CFltRect &geoExt)
{
	CRect client;
	GetClientRect(&client);
	ASSERT(client.right>0 && client.bottom>0);

	CFltPoint geoSize=geoExt.Size();

	if(geoSize.x==0 && geoSize.y==0) {
		CenterOnGeoPoint(geoExt.Center(),4.0);
	}
	else {
		double fScale;
		if(geoSize.x*client.bottom>geoSize.y*client.right) {
			//Fit width
			fScale=client.right/geoSize.x;
		}
		else {
			fScale=client.bottom/geoSize.y;
		}
		CenterOnGeoPoint(geoExt.Center(),(fScale*0.85)/m_fScale);
	}
}

LRESULT CWallsMapView::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	/*
	virtual void CWinApp::WinHelp(DWORD_PTR dwData,UINT nCmd = HELP_CONTEXT);
	*/

	AfxGetApp()->WinHelp(1000);
	return TRUE;
}

void CWallsMapView::OnImageOpacity()
{
	CFltPoint ptGeo;
	ClientPtToGeoPt(m_ptPopup,ptGeo);
	CImageLayer *pImg=GetTopImageLayer(ptGeo);
	ASSERT(pImg);
	if(!pImg) return;
	CWallsMapDoc *pDoc=GetDocument();
	CDlgSymbolsImg *pDlg=CImageLayer::m_pDlgSymbolsImg;

	//ignore layer window status -- code below fails if positioned at folder
#if 0
	ASSERT(!hPropHook || pDoc==pLayerSheet->GetDoc());
	if(pDoc->SelectedLayerPtr()!=(CMapLayer *)pImg) {
		int id=pDoc->LayerSet().LayerPos(pImg);
		if(!id--) {
			ASSERT(0);
			return;
		}
		if(hPropHook && pDoc==pLayerSheet->GetDoc()) {
			pLayerSheet->SetActivePage(0);
			pLayerSheet->SelectLayer(id);
		}
		else pDoc->SetSelectedLayerPos(id);
	}
	if(hPropHook) {
		pLayerSheet->PostMessage(WM_PROPVIEWDOC,(WPARAM)0,(LPARAM)LHINT_SYMBOLOGY);
	}
	else {
#endif
	if(pDlg) {
		if(pDlg->m_pLayer!=pImg) pDlg->NewLayer(pImg);
		pDlg->BringWindowToTop();
	}
	else pDlg=new CDlgSymbolsImg(pImg);
}

LRESULT CWallsMapView::OnTabletQuerySystemGestureStatus(WPARAM,LPARAM)
{
   return 0;
}

LRESULT CWallsMapView::OnPropViewDoc(WPARAM wParam,LPARAM lParam)
{
	OnCtrOnGPSPoint();
	return TRUE;
}

void CWallsMapView::OnGpsTracking()
{
	// toggle tracking ON/OFF
	ASSERT(pGPSDlg);
	m_bTrackingGPS=!m_bTrackingGPS;
	Invalidate(FALSE); //UpdateAllViews(NULL,LHINT_REPAINT);
}

void CWallsMapView::OnUpdateGpsTracking(CCmdUI *pCmdUI)
{
	BOOL bEnable=GetDocument()->IsGeoRefSupported() && pGPSDlg && pGPSDlg->HavePos();
	pCmdUI->SetCheck(bEnable && m_bTrackingGPS);
	pCmdUI->Enable(bEnable);
}

void CWallsMapView::OnUpdateCtrOnGPSPoint(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(pGPSDlg && pGPSDlg->HavePos() && m_bTrackingGPS);
}

void CWallsMapView::OnCtrOnGPSPoint()
{
   if(m_bTrackingGPS && !vptGPS.empty()) {
	   CFltPoint fp(vptGPS.back());
	   GetDocument()->GetConvertedGPSPoint(fp);
	   CenterOnGeoPoint(fp,1.0);
   }
}


void CWallsMapView::OnGpsDisplaytrack()
{
	m_bDisplayTrack=!m_bDisplayTrack;
	Invalidate(FALSE);
}


void CWallsMapView::OnUpdateGpsDisplaytrack(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bDisplayTrack==true);
	pCmdUI->Enable(pGPSDlg && m_bTrackingGPS && pGPSDlg->IsRecording());
}


void CWallsMapView::OnGpsEnablecentering()
{
	m_bAutoCenterGPS=!m_bAutoCenterGPS;
}


void CWallsMapView::OnUpdateGpsEnablecentering(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bAutoCenterGPS==true);
	pCmdUI->Enable(pGPSDlg && m_bTrackingGPS);
}
