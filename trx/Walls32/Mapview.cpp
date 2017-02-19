// mapview.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "segview.h"
#include "compview.h"
#include "mapframe.h"
#include "mapview.h"
#include "compile.h"
#include "dialogs.h"
#include "vnode.h"
#include "ResizeFrmDlg.h"
#include "ScaleWnd.h"
#include <commctrl.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapView

IMPLEMENT_DYNCREATE(CMapView, CView)

HCURSOR CMapView::m_hCursorPan=0;
HCURSOR CMapView::m_hCursorHand=0;
HCURSOR CMapView::m_hCursorCross=0;
HCURSOR CMapView::m_hCursorArrow=0;
HCURSOR CMapView::m_hCursorMeasure=0;
HCURSOR CMapView::m_hCursorIdentify=0;

CMapView::CMapView()
{
#ifdef VISTA_INC
	ASSERT(g_hwndTrackingTT==NULL);
#endif
/*
	m_bScrollBars=m_bScrolling=false;
*/
	m_bMeasuring=m_bInContext=m_bPanning=m_bPanned=false;
	m_ptBitmapOverlap.x=m_ptBitmapOverlap.y=0;
	m_bIdentify=FALSE;
	m_pVecNode=NULL;
	m_pScaleWnd=NULL;
}

CMapView::~CMapView()
{
	delete m_pScaleWnd;
}

BEGIN_MESSAGE_MAP(CMapView, CView)
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
	ON_COMMAND(ID_MAP_CENTER, OnMapCenter)
	ON_COMMAND(ID_MAP_LABELS, OnMapLabels)
	ON_COMMAND(ID_MAP_ORIGINAL, OnMapOriginal)
	ON_COMMAND(ID_MAP_ZOOMIN, OnMapZoomin)
	ON_UPDATE_COMMAND_UI(ID_MAP_ZOOMIN, OnUpdateMapZoomin)
	ON_COMMAND(ID_MAP_ZOOMOUT, OnMapZoomout)
	ON_UPDATE_COMMAND_UI(ID_MAP_ZOOMOUT, OnUpdateMapZoomout)
	ON_UPDATE_COMMAND_UI(ID_MAP_LABELS, OnUpdateMapLabels)
	ON_COMMAND(ID_MAP_MARKERS, OnMapMarkers)
	ON_UPDATE_COMMAND_UI(ID_MAP_MARKERS, OnUpdateMapMarkers)
	ON_COMMAND(ID_MAP_NOTES, OnMapNotes)
	ON_UPDATE_COMMAND_UI(ID_MAP_NOTES, OnUpdateMapNotes)
	ON_COMMAND(ID_MAP_FLAGS, OnMapFlags)
	ON_UPDATE_COMMAND_UI(ID_MAP_FLAGS, OnUpdateMapFlags)
	ON_COMMAND(ID_MAP_PASSAGE, OnMapPassage)
	ON_UPDATE_COMMAND_UI(ID_MAP_PASSAGE, OnUpdateMapPassage)
	ON_COMMAND(ID_MAP_VIEWHELP, OnMapViewhelp)
	ON_COMMAND(ID_MEASUREDIST, OnMeasureDist)
	ON_UPDATE_COMMAND_UI(ID_MEASUREDIST, OnUpdateMeasureDist)
	ON_COMMAND(ID_MAP_PREFIXES, OnMapPrefixes)
	ON_UPDATE_COMMAND_UI(ID_MAP_PREFIXES, OnUpdateMapPrefixes)
	ON_COMMAND(ID_MAP_VECTORS, OnMapVectors)
	ON_UPDATE_COMMAND_UI(ID_MAP_VECTORS, OnUpdateMapVectors)
	//ON_COMMAND(ID_MAP_SELECTED, OnMapSelected)
	//ON_UPDATE_COMMAND_UI(ID_MAP_SELECTED, OnUpdateMapSelected)
	ON_COMMAND(ID_MAP_ELEVATIONS, OnMapElevations)
	ON_UPDATE_COMMAND_UI(ID_MAP_ELEVATIONS, OnUpdateMapElevations)
	ON_COMMAND(ID_LOCATEINGEOM, OnLocateInGeom)
	ON_COMMAND(ID_GO_EDIT, OnGoEdit)
	ON_COMMAND(ID_VECTORINFO, OnVectorInfo)
	ON_COMMAND(ID_REVIEWSEG, OnViewSegment)
	ON_UPDATE_COMMAND_UI(ID_LOCATEINGEOM, OnUpdateLocateInGeom)
	ON_COMMAND(ID_RESIZEMAPFRAME, OnResizemapframe)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SEL,OnUpdateUTM)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_UTM,OnUpdateUTM)
	ON_COMMAND(ID_TRAV_TOGGLE, OnTravToggle)
	ON_UPDATE_COMMAND_UI(ID_TRAV_TOGGLE, OnUpdateTravToggle)
	ON_COMMAND(ID_SCALEUNITS_FEET, OnScaleUnitsToggle)
	ON_UPDATE_COMMAND_UI(ID_SCALEUNITS_FEET, OnUpdateScaleUnitsFeet)
	ON_COMMAND(ID_SCALEUNITS_METERS, OnScaleUnitsToggle)
	ON_UPDATE_COMMAND_UI(ID_SCALEUNITS_METERS, OnUpdateScaleUnitsMeters)

END_MESSAGE_MAP()

bool CMapView::IsReviewNode()
{
	if(!m_pFrame->m_pDoc || m_pFrame->m_pFrameNode!=CPrjDoc::m_pReviewNode || m_pFrame->m_dwReviewNetwork != dbNTN.Position()) {
		return false;
	}
	if((int)dbNTN.Position()!=pPV->m_recNTN) {
		ASSERT(0);
		return false;
	}
	return true;
}

BOOL CMapView::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.
	
	if(!CView::PreCreateWindow(cs)) return FALSE;
	cs.dwExStyle&=~WS_EX_CLIENTEDGE;
	return TRUE;	  
}

void CMapView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
    
    //Lets set the initial window size and position based on the size
    //of the bitmap. We actually do this by resizing the parent frame
    //which has not yet been activated or made visible. (This will be done
    //by ActivateFrame(), which is one of the last operations of the
    //template's OpenDocumentFile() or, in this case, InitialUpdateFrame()). 
	
	m_pFrame=(CMapFrame *)GetParentFrame();
	ASSERT(m_pFrame && m_pFrame->IsKindOf(RUNTIME_CLASS(CMapFrame)));
	
	//Let's center parent within it's own parent's client area, reducing the
    //initial size as necessary --
    CRect rect;
    (m_pFrame->GetParent())->GetClientRect(&rect);
    
    int width=m_pFrame->m_ptSize.x;
    int height=m_pFrame->m_ptSize.y;
    int left=rect.left; //==0
    int top=rect.top;	//==0
    
    //if(width>=rect.Width()) width=rect.Width(); else
    left+=(rect.Width()-width)/2;
    //if(height>=rect.Height()) height=rect.Height(); else
    top+=(rect.Height()-height)/2;

	m_bFeetUnits=m_pFrame->m_bFeetUnits;
	m_pScaleWnd = new CScaleWnd(m_bFeetUnits);
	if(!m_pScaleWnd->Create(this)) {
		delete m_pScaleWnd;
		m_pScaleWnd=NULL;
	}
    
	m_pFrame->SetWindowPos(NULL,left,top,width,height,SWP_HIDEWINDOW|SWP_NOACTIVATE);

	m_bProfile=m_pFrame->m_bProfile;
	m_bCursorInBmp=m_bTracking=FALSE;
	m_bIdentify=!bMeasureDist && (m_pFrame->m_pVnode || m_pFrame->m_pVnodeF);
	m_pVecNode=NULL;

}

void CMapView::OnDraw(CDC* pDC)
{
	if(m_bPanning || m_pFrame->m_bResizing) {
	    HBRUSH hBrush=pSV->m_hBrushBkg;
		CRect image(0,0,m_rectBmp.Width(),m_rectBmp.Height());
		image.OffsetRect(m_ptBitmapOverlap);
		CRect client;
		GetClientRect(&client);
		CRect overlap;
		if(!overlap.IntersectRect(&client,&image)) {
			::FillRect(pDC->m_hDC,client,hBrush);
		}
		else {
			//Optain a compatible DC for the bitmap --
			CDC dcBMP;
			dcBMP.CreateCompatibleDC(pDC);
			CBitmap *pBmpOld=dcBMP.SelectObject(m_pFrame->m_pBmp);
			::BitBlt(pDC->m_hDC, m_ptBitmapOverlap.x, m_ptBitmapOverlap.y,
				image.Width(), image.Height(), dcBMP, 0, 0, SRCCOPY);

			if(overlap!=client) {
				if(image.top>0) ::FillRect(pDC->m_hDC, CRect(0, 0, client.right, image.top), hBrush);
				if(image.left>0) ::FillRect(pDC->m_hDC, CRect(0, 0, image.left, client.bottom), hBrush);
				if(image.right<client.right) ::FillRect(pDC->m_hDC, CRect(image.right, 0, client.right, client.bottom), hBrush);
				if(image.bottom<client.bottom) ::FillRect(pDC->m_hDC, CRect(0, image.bottom, client.right, client.bottom), hBrush);
			}
		}
	    return;
	}

	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	CRect rectPlot;

	if(m_pVecNode && rectPlot.IntersectRect(&m_VecNodeRect,&rectClip)) ClearLocate();

	if(rectPlot.IntersectRect(&m_rectBmp,&rectClip)) {

	    //Optain a compatible DC for the bitmap --
	    CDC dcBMP;
	    dcBMP.CreateCompatibleDC(pDC);
	    CBitmap *pBmpOld=dcBMP.SelectObject(m_pFrame->m_pBmp);
	    
	    pDC->BitBlt(rectPlot.left,rectPlot.top,rectPlot.Width(),rectPlot.Height(),
	  	  &dcBMP,rectPlot.left,rectPlot.top,SRCCOPY);

	  	dcBMP.SelectObject(pBmpOld);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMapView message handlers

#ifdef VISTA_INC
void CMapView::ShowTrackingTT(CPoint &point)
{
	CString s;
	GetDistanceFormat(s,m_bProfile,m_bFeetUnits, m_pScaleWnd && (m_pScaleWnd->IsFeetUnits()!=m_bFeetUnits));
	g_toolItem.lpszText = s.GetBuffer();
	::SendMessage(g_hwndTrackingTT,TTM_SETTOOLINFO, 0, (LPARAM)&g_toolItem);
	// Position the tooltip. The coordinates are adjusted so that the tooltip does not overlap the mouse pointer.
	ClientToScreen(&point);
	::SendMessage(g_hwndTrackingTT,TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(point.x + 10, point.y - 20));
}
#endif

void CMapView::OnLButtonDown(UINT nFlags, CPoint point)
{
	ClearLocate();

	if(bMeasureDist) {
		if(m_bMeasuring) {
			ASSERT(0);
			RefreshMeasure();
			m_bMeasuring=FALSE;
			#ifdef VISTA_INC
				DestroyTrackingTT();
            #endif
		}
		if(!(nFlags&MK_RBUTTON)) {
			GetCoordinate(point);
			dpCoordinateMeasure=dpCoordinate;
			ptMeasureStart=ptMeasureEnd=point;
			m_bMeasuring=true;
#ifdef VISTA_INC
			ASSERT(!g_hwndTrackingTT);
			if(!g_hwndTrackingTT) {
				g_hwndTrackingTT=CreateTrackingToolTip(m_hWnd);
			}
			if(g_hwndTrackingTT) {
				//position tooltip --
				ShowTrackingTT(point);
				//Activate the tooltip.
				::SendMessage(g_hwndTrackingTT,TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&g_toolItem);
			}
#endif

		}
	}
	else if(IsReviewNode()) {
		CFixedTracker tracker;
		tracker.m_pr=&m_rectBmp;
		/*
		m_bScrolling=true;
		*/
		//Sets capture and waits for LButtonUp (may be part of a double-click) --
		if(tracker.TrackRubberBand(this, point, TRUE))
		{
			tracker.m_rect.NormalizeRect();
			int pxwidth=tracker.m_rect.Width();
			if(pxwidth>=MIN_TRACKERWIDTH) {
				if(pxwidth/m_pFrame->m_scale<0.1) AfxMessageBox(IDS_ERR_PAGEZOOM);
				else {
					NewView(tracker.m_rect);
				}
			}
		}
	}
	m_bPanned=m_bPanning=FALSE;
}

void CMapView::RefreshVecNode(BOOL bClear)
{
	CRect rect;

	ASSERT(m_pVecNode);

	if(bClear) {
		rect=m_VecNodeRect;
		m_pVecNode=NULL;
	}
	else {
		rect.SetRect(m_pVecNode->B_xy[0],m_pVecNode->B_xy[1],m_pVecNode->M_xy[0],m_pVecNode->M_xy[1]);
		rect.NormalizeRect();
		int w=rect.Width();
		if(!w && !rect.Height()) rect.InflateRect(VNODE_BOX_RADIUS,VNODE_BOX_RADIUS,VNODE_BOX_RADIUS,VNODE_BOX_RADIUS);
		else {
			if(w<2*VNODE_NEAR_LIMIT_SQRT) rect.InflateRect(VNODE_NEAR_LIMIT_SQRT-w/2,0,VNODE_NEAR_LIMIT_SQRT-w/2,0);
			w=rect.Height();
			if(w<2*VNODE_NEAR_LIMIT_SQRT) rect.InflateRect(0,VNODE_NEAR_LIMIT_SQRT-w/2,0,VNODE_NEAR_LIMIT_SQRT-w/2);
		}
		m_VecNodeRect=rect;
	}
	/*
	CPoint pt=GetDeviceScrollPosition();
	rect.OffsetRect(-pt.x,-pt.y);
	*/
	CClientDC dc(this);
	dc.DrawFocusRect(rect);
}

void CMapView::RefreshMeasure()
{
    CClientDC dc(this);
    dc.SetROP2(R2_NOT);
    dc.SetBkMode(TRANSPARENT);
    dc.MoveTo(ptMeasureStart);
	dc.LineTo(ptMeasureEnd);
}

void CMapView::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect client(0,0,m_sizeClient.cx, m_sizeClient.cy);
	m_bCursorInBmp=m_rectBmp.PtInRect(point);


	if((nFlags & MK_RBUTTON)) {
		ASSERT(!m_bInContext);
		if(m_bPanning) {
			if(!m_bCursorInBmp)	{
				PanExit();
				return;
			}

			if(point!=m_ptPan) {
				if(!m_bPanned) {
					if(!IsReviewNode()) {
						m_bPanning=false;
						return;
					}
					SetCapture(); //released on PanExit()
					m_bPanned=true;
					RefreshCursor();
				}
				//perform pan

			   	m_ptBitmapOverlap+=(point-m_ptPan);
				m_ptPan=point;
				Invalidate(FALSE);
			}
		}
		return;
	}
	
	if(!m_bCursorInBmp) return;

	GetCoordinate(point);
	
	if(m_bMeasuring) {

		if(ptMeasureEnd==point)
			return;

		RefreshMeasure();
		ptMeasureEnd=point;
		RefreshMeasure();

#ifdef VISTA_INC
		if(g_hwndTrackingTT) {
			ShowTrackingTT(point);
		}
#endif
	}
	else {
		if(m_bIdentify) {
			ASSERT(m_pFrame->m_pVnode || m_pFrame->m_pVnodeF);
			ClearLocate();
			m_pVecNode=m_pFrame->GetVecNode(point);
			DrawLocate();
		}
		if(!m_bTracking) {
			TrackMouse(m_hWnd);
			m_bTracking=TRUE;
		}
	}
}

void CMapView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(m_bMeasuring) {
		RefreshMeasure();
		m_bMeasuring=false;
#ifdef VISTA_INC
		DestroyTrackingTT();
#endif
	}
	/*
	m_bScrolling=false;
	*/
	//::ReleaseCapture();
}

void CMapView::RefreshCursor()
{
	HCURSOR hc;

	if(m_bPanning) hc=m_hCursorHand;
	else if(bMeasureDist) {
		ClearLocate();
		m_bIdentify=FALSE;
		hc=m_hCursorMeasure;
	}
	else if(IsReviewNode()) {
		m_bIdentify=(m_pFrame->m_pVnode || m_pFrame->m_pVnodeF);
		//hc=m_bIdentify?m_hCursorIdentify:m_hCursorCross;
		hc=m_hCursorIdentify;
	}
	else {
		ClearLocate();
		m_bIdentify=FALSE;
		hc=m_hCursorArrow;
	}

	VERIFY(::SetCursor(hc));
}

BOOL CMapView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	/*
	if(m_bScrollBars && GetKeyState(VK_CONTROL)<0) {
	    ::SetCursor(m_hCursorPan);
	}
	else
	*/
	RefreshCursor();
	return TRUE;

	//return CScrollView::OnSetCursor(pWnd, nHitTest, message);
}

void CMapView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if(m_pScaleWnd && m_pScaleWnd->m_width && cy!=m_pScaleWnd->m_bottom)
		SetScalePos();
}

LRESULT CMapView::OnExitSizeMove(WPARAM, LPARAM)
{
	//Refresh map, resizing bitmap if bitmap doesn't cover client --

	CRect client;
	GetClientRect(&client);
	m_sizeClient=CSize(client.right,client.bottom);

	if(client.right!=m_rectBmp.right || client.bottom!=m_rectBmp.bottom) {
		m_pFrame->SetDocMapFrame();
		//Generate new image --
		ASSERT(pCV && pCV->IsKindOf(RUNTIME_CLASS(CCompView)));
		m_pFrame->m_pCRect=&client;
		pPV->m_bExternUpdate=3;
		CPrjDoc::PlotFrame(m_pFrame->m_flags);
		m_bFeetUnits=m_pFrame->m_bFeetUnits;
		pPV->m_bExternUpdate=0;
		m_pVecNode=NULL;
	}
	return 0;
}

void CMapView::GetCoordinate(CPoint point)
{
	/*
	point+=GetDeviceScrollPosition();
	*/

	//coord.x=x*LEN_SCALE(cosA/xscale) - y*LEN_SCALE(sinA/xscale) + LEN_SCALE(xoff*cosA-yoff*sinA+midE);
	//coord.y=-x*LEN_SCALE(sinA/xscale) - y*LEN_SCALE(cosA/xscale) - LEN_SCALE(xoff*sinA+yoff*cosA-midN);

	point.y+=2;
	point.x+=2;

	if(m_bProfile) dpCoordinate.y=m_pFrame->m_cosAx+m_pFrame->m_midNx*(point.y);
	else {
		dpCoordinate.x = point.x*m_pFrame->m_cosAx -
			point.y*m_pFrame->m_sinAx + m_pFrame->m_midEx;
		dpCoordinate.y =-point.x*m_pFrame->m_sinAx -
			point.y*m_pFrame->m_cosAx - m_pFrame->m_midNx;
	}
}

void CMapView::OnUpdateUTM(CCmdUI* pCmdUI)
{
	BOOL bEnable=m_bCursorInBmp;
    pCmdUI->Enable(bEnable);
	if(bEnable) {
		CString s;
		if(pCmdUI->m_nID==ID_INDICATOR_SEL) {
			m_pFrame->m_pFrameNode->RefZoneStr(s);
		}
		else {
#ifndef VISTA_INC
			if(m_bMeasuring) GetDistanceFormat(s,m_bProfile,m_bFeetUnits);
			else
#endif
		    GetCoordinateFormat(s,dpCoordinate,m_bProfile,m_bFeetUnits);
		}
		pCmdUI->SetText(s); 
	}
}

LRESULT CMapView::OnMouseLeave(WPARAM wParam,LPARAM strText)
{
	m_bCursorInBmp=m_bTracking=FALSE;
	if(m_bMeasuring) {
		RefreshMeasure();
		m_bMeasuring=false;
#ifdef VISTA_INC
		DestroyTrackingTT();
#endif
	}

	if(!m_bInContext) ClearLocate();

	return FALSE;
}

void CMapView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(m_bCursorInBmp && (nFlags&MK_CONTROL)) {
		/*
		if(m_bScrollBars) ::SetCursor(m_hCursorPan);
		else
		*/
		RefreshCursor();
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMapView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(m_bCursorInBmp && (nFlags&MK_CONTROL)) RefreshCursor();
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CMapView::NewView(CRect &rect)
{
	//CPlotView::m_pMapFrame=m_pFrame;
	m_pFrame->SetDocMapFrame();
	//Generate new image --
	ASSERT(pCV && pCV->IsKindOf(RUNTIME_CLASS(CCompView)));
	m_pFrame->m_pCRect=&rect;
	pPV->m_bExternUpdate=2;
	CPrjDoc::PlotFrame(m_pFrame->m_flags);
	m_bFeetUnits=m_pFrame->m_bFeetUnits;
	pPV->m_bExternUpdate=0;
	m_pVecNode=NULL;
}

void CMapView::OnMapCenter() 
{
	m_ptPopup.x-=m_sizeClient.cx/2;
	m_ptPopup.y-=m_sizeClient.cy/2;
	NewView(CRect(m_ptPopup, m_sizeClient));
}

void CMapView::OnMapOriginal() 
{
	//Restore prevue map view --
	m_pFrame->SetDocMapFrame();
	pPV->OnUpdateFrame();
}

#define ZOOMINSCALE_DFLT 0.5
#define ZOOMOUTSCALE_DFLT 2.0

void CMapView::OnMapZoom(double scale)
{
	//create a rect with scale * size and with same center coordinates--
	int bw=m_sizeClient.cx;
	int incx=(int)(bw*scale/2);
	if(!incx) {
		ASSERT(0); //doesn't happen due to OnUpdate
		return;
	}
	int bh=m_sizeClient.cy;
	int incy=(int)(bh*scale/2);
	if(!incy) {
		ASSERT(0);
		return;
	}
	UINT tlx=bw/2-incx;
	UINT tly=bh/2-incy;
	NewView(CRect(tlx,tly,tlx+2*incx,tly+2*incy));
}

void CMapView::OnMapZoomin() 
{
	m_ptPopup.x-=m_sizeClient.cx/4;
	m_ptPopup.y-=m_sizeClient.cy/4;
	NewView(CRect(m_ptPopup.x,m_ptPopup.y,
		m_ptPopup.x+m_sizeClient.cx/2,m_ptPopup.y+m_sizeClient.cy/2));
}

void CMapView::OnUpdateMapZoomin(CCmdUI* pCmdUI) 
{
	double scale=m_pFrame->m_scale*0.5;
	pCmdUI->Enable((m_sizeClient.cx*0.5)/scale>=1.0 && (m_sizeClient.cy*0.5)/scale>=1.0);
}

void CMapView::OnMapZoomout() 
{
	m_ptPopup.x-=m_sizeClient.cx;
	m_ptPopup.y-=m_sizeClient.cy;
	NewView(CRect(m_ptPopup.x,m_ptPopup.y,
		m_ptPopup.x+2*m_sizeClient.cx,m_ptPopup.y+2*m_sizeClient.cy));
}

void CMapView::OnUpdateMapZoomout(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_sizeClient.cx/(m_pFrame->m_scale*0.5)<=1000000.0);
}

BOOL CMapView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if(zDelta) {

		if(!IsReviewNode()) {
			OnContextMenu(this, pt);
			return FALSE;
		}

		if(zDelta<0) {
			if(m_sizeClient.cx/(m_pFrame->m_scale*0.9)>1000000.0) {
				return TRUE;
			}
			OnMapZoom(1/0.9);
		}
		else {
			if((m_sizeClient.cx*0.9)/m_pFrame->m_scale<1.0) {
				return TRUE;
			}
			OnMapZoom(0.9);
		}
		return TRUE;
	}
	return FALSE;
}

void CMapView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if(IsReviewNode()) {
		/*
		m_ptPopup=point+GetDeviceScrollPosition();
		*/
		m_ptPopup=point;
		OnMapCenter();
	}
}

void CMapView::ToggleLayer(UINT flag)
{
	UINT uNames=(m_pFrame->m_flags&M_NAMES);

	if(flag==M_NAMES) {
		if((m_pFrame->m_flags&(M_NAMES+M_HEIGHTS+M_PREFIXES))==M_NAMES)
			m_pFrame->m_flags&=~M_NAMES;
		else {
			m_pFrame->m_flags&=~(M_HEIGHTS+M_PREFIXES);
			m_pFrame->m_flags|=M_NAMES;
		}
	}
	else if(flag&(M_HEIGHTS+M_PREFIXES)) {
		if(m_pFrame->m_flags&flag) m_pFrame->m_flags&=~(flag+M_NAMES);
		else {
			m_pFrame->m_flags&=~(M_HEIGHTS+M_PREFIXES);
			m_pFrame->m_flags|=(flag+M_NAMES);
		}
	}
	else {
		if(m_pFrame->m_flags&flag) m_pFrame->m_flags&=~flag;
		else m_pFrame->m_flags|=flag;
	}

	CMapFrame::m_bKeepVnode=m_pFrame->m_pVnode && !(flag&(M_NOVECTORS+M_LRUDS)) && !pSV->IsNewVisibility();

	if(uNames!=(m_pFrame->m_flags&M_NAMES) || (flag&(M_MARKERS+M_NOTES+M_FLAGS)))
		pSV->SetNewVisibility(TRUE);

	NewView(CRect(0,0,m_sizeClient.cx,m_sizeClient.cy));
	ASSERT(!CMapFrame::m_bKeepVnode);
	CMapFrame::m_bKeepVnode=FALSE;
}

void CMapView::RefreshView()
{
	//Called by all style change functions in CSegView --

	CMapFrame::m_bKeepVnode=!pSV->IsNewVisibility();

	CRect rect(0,0,m_sizeClient.cx,m_sizeClient.cy);
	m_pFrame->m_pCRect=&rect;
	m_pFrame->SetDocMapFrame();
	pPV->m_bExternUpdate=2; //no change in frame size
	CPrjDoc::PlotFrame(m_pFrame->m_flags);
	m_bFeetUnits=m_pFrame->m_bFeetUnits;
	pPV->m_bExternUpdate=0;
	m_pVecNode=NULL;
	ASSERT(!CMapFrame::m_bKeepVnode);
	CMapFrame::m_bKeepVnode=FALSE;
	VERIFY(SetWindowPos(&CWnd::wndTop, 0,0,0,0,
		SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE));
	GetParentFrame()->ActivateFrame();
}

void CMapView::OnMapLabels() 
{
	ToggleLayer(M_NAMES);
}

void CMapView::OnUpdateMapLabels(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((m_pFrame->m_flags&(M_NAMES+M_HEIGHTS+M_PREFIXES))==M_NAMES);
}

void CMapView::OnMapPrefixes() 
{
	ToggleLayer(M_PREFIXES);
}

void CMapView::OnUpdateMapPrefixes(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((m_pFrame->m_flags&M_PREFIXES)!=0);
}


void CMapView::OnMapElevations() 
{
	ToggleLayer(M_HEIGHTS);
}

void CMapView::OnUpdateMapElevations(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((m_pFrame->m_flags&M_HEIGHTS)!=0);
}

void CMapView::OnMapMarkers() 
{
	ToggleLayer(M_MARKERS);
}

void CMapView::OnUpdateMapMarkers(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((m_pFrame->m_flags&M_MARKERS)!=0);
}

void CMapView::OnMapNotes() 
{
	ToggleLayer(M_NOTES);
}

void CMapView::OnUpdateMapNotes(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((m_pFrame->m_flags&M_NOTES)!=0);
}

void CMapView::OnMapFlags() 
{
	ToggleLayer(M_FLAGS);
}

void CMapView::OnUpdateMapFlags(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((m_pFrame->m_flags&M_FLAGS)!=0);
}

void CMapView::OnMapVectors() 
{
	ToggleLayer(M_NOVECTORS);
}

void CMapView::OnUpdateMapVectors(CCmdUI* pCmdUI) 
{
	//BOOL bCheck=(m_pFrame->m_flags&M_NOVECTORS)==0;
	pCmdUI->SetCheck((m_pFrame->m_flags&M_NOVECTORS)==0);
	//pCmdUI->Enable(bCheck || pSV->IsLinesVisible());
}

void CMapView::OnMapPassage()
{
	ToggleLayer(M_LRUDS);
}

void CMapView::OnUpdateMapPassage(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck((m_pFrame->m_flags&M_LRUDS)!=0);
}

void CMapView::OnMapViewhelp() 
{
	CMapHelpDlg dlg(m_pFrame->m_pFrameNode->Title());
	dlg.DoModal();
}

void CMapView::OnMeasureDist() 
{
	if(bMeasureDist=!bMeasureDist) {
		ClearLocate();
		m_bIdentify=FALSE;
	}
}

void CMapView::OnUpdateMeasureDist(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(bMeasureDist);
}

void CMapView::OnLocateInGeom()
{
	if(!dbNTP.Go(m_iPltrec)) {
		ASSERT(pPLT->vec_id && pPLT->str_id);
		CPrjDoc::PositionReview();
	}
}

void CMapView::OnUpdateLocateInGeom(CCmdUI* pCmdUI) 
{
	ASSERT(m_iPltrec==(int)dbNTP.Position());
	pCmdUI->Enable(pPLT->str_id!=NULL);
}

void CMapView::OnGoEdit() 
{
	if(!dbNTP.Go(m_iPltrec)) {
		ASSERT(pPLT->vec_id);
		CPrjDoc::LoadVecFile((pPLT->vec_id+1)/2);
	}
}

void CMapView::OnVectorInfo() 
{
	//Assumes pPLT and pVEC is open and correctly positioned --
	CPrjDoc::VectorProperties();
}

void CMapView::OnViewSegment()
{
	//Assumes pPLT and pVEC is open and correctly positioned --
	CPrjDoc::ViewSegment();
}

void CMapView::OnResizemapframe()
{

	CRect rect;
	GetClientRect(&rect);

	CResizeFrmDlg dlg;

	CWindowDC dc(NULL);
	dlg.m_iPx_per_in=dc.GetDeviceCaps(LOGPIXELSX);
	ASSERT(dlg.m_iPx_per_in>0);
	dlg.m_iCurrentWidth=rect.right;
	dlg.m_dfltWidthInches=CPlotView::m_mfFrame.fFrameWidthInches;

	if(dlg.DoModal()==IDOK) {
		CPlotView::m_mfFrame.fFrameWidthInches=dlg.m_dfltWidthInches;
		if(dlg.m_bSave) {
			CPlotView::m_mfFrame.bChanged=TRUE;
		}
	}
}

BOOL CMapView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CMapView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu menu;

	if(m_pScaleWnd) {
		CRect scRect;
		m_pScaleWnd->GetWindowRect(&scRect);
		if(scRect.PtInRect(point)) {
			if(menu.LoadMenu(IDR_SCALE_CONTEXT)) {
				CMenu* pPopup = menu.GetSubMenu(0);
				ASSERT(pPopup != NULL);
				pPopup->CheckMenuItem(ID_SCALEUNITS_FEET, m_bFeetUnits?(MF_CHECKED|MF_BYCOMMAND):(MF_UNCHECKED|MF_BYCOMMAND));
				pPopup->CheckMenuItem(ID_SCALEUNITS_METERS, m_bFeetUnits?(MF_UNCHECKED|MF_BYCOMMAND):(MF_CHECKED|MF_BYCOMMAND));
				//ClientToScreen(&point);
				m_bInContext=true;
				VERIFY(pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, GetParentFrame()));
				m_bInContext=false;
			}
			return;
		}
	}

	UINT id=IDR_MAP_FRAMEINFO;
	if(IsReviewNode()) {
		if(m_bPanned) return;
		if(m_pVecNode && !dbNTP.Go(m_pVecNode->pltrec) && !dbNTV.Go((pPLT->vec_id+1)/2))
			id=IDR_MAP_IDENTIFY;
		else id=IDR_MAP_CONTEXT;
	}
	ASSERT(!m_pVecNode || m_bIdentify);

	if(menu.LoadMenu(id))
	{
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);

		if(id==IDR_MAP_IDENTIFY) {
			m_iPltrec=m_pVecNode->pltrec;
			CString sTitle("Edit ");
			CPrjDoc::AppendVecStationNames(sTitle);
			pPopup->ModifyMenu(0,MF_BYPOSITION|MF_STRING,ID_GO_EDIT,sTitle);
			//Doesn't work! Why???
			//int i=pPopup->EnableMenuItem(ID_LOCATEINGEOM,MF_BYCOMMAND|(pPLT->str_id?MF_ENABLED:MF_GRAYED));
		}
		m_ptPopupRaw=point;
		ScreenToClient(&m_ptPopupRaw);
		m_ptPopup=m_ptPopupRaw;

		m_bInContext=true;
		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,m_pFrame);
		m_bInContext=false;
	}
	ClearLocate();
}

void CMapView::OnRButtonDown(UINT nFlags, CPoint point)
{
	if(IsReviewNode()) {
		m_bPanning=true; m_bPanned=false;
		m_ptPan=point;
		m_ptBitmapOverlap.x=m_ptBitmapOverlap.y=0; //pixel offset of DIB within client
		RefreshCursor();
	}
	else CView::OnRButtonDown(nFlags, point);
}

void CMapView::OnRButtonUp(UINT nFlags, CPoint point)
{
	bool bPanned=false;
	if(m_bPanning) {
		bPanned=m_bPanned;
		PanExit();
	}
	if(!bPanned) CView::OnRButtonUp(nFlags, point);
}

void CMapView::PanExit()
{
	m_bPanning=false;
	if(m_bPanned) {
		m_bPanned=false;
		ReleaseCapture();
		NewView(CRect(-m_ptBitmapOverlap.x, -m_ptBitmapOverlap.y,
			m_sizeClient.cx-m_ptBitmapOverlap.x, m_sizeClient.cy-m_ptBitmapOverlap.y));
		m_ptBitmapOverlap.x=m_ptBitmapOverlap.y=0;

	}
	RefreshCursor();
}

void CMapView::SetScalePos()
{
	CRect client;
	GetClientRect(&client);
	m_pScaleWnd->SetWindowPos(NULL, 0, client.bottom-BAR_HEIGHT, m_pScaleWnd->m_width,
		BAR_HEIGHT, SWP_NOZORDER|SWP_SHOWWINDOW);
	m_pScaleWnd->m_bottom=client.bottom;
}

void CMapView::DrawScale()
{
	if(m_pScaleWnd) {
		m_pScaleWnd->Draw(1.0/m_pFrame->m_scale,m_pFrame->m_bProfile?-1.0:m_pFrame->m_fView);
		SetScalePos();
	}
}

void CMapView::OnScaleUnitsToggle()
{
	ASSERT(m_pScaleWnd);
	m_pScaleWnd->ToggleFeetUnits();
	DrawScale();
}

void CMapView::OnUpdateScaleUnitsFeet(CCmdUI *pCmdUI)
{
	if(m_pScaleWnd) {
		pCmdUI->Enable(1);
		pCmdUI->SetCheck(m_pScaleWnd->IsFeetUnits());
	}
	else pCmdUI->Enable(0);
}

void CMapView::OnUpdateScaleUnitsMeters(CCmdUI *pCmdUI)
{
	if(m_pScaleWnd) {
		pCmdUI->Enable(1);
		pCmdUI->SetCheck(!m_pScaleWnd->IsFeetUnits());
	}
	else pCmdUI->Enable(0);
}

void CMapView::OnTravToggle()
{
	pCV->TravToggle();
	GetDocument()->RefreshMapTraverses(TRV_SEL+TRV_PLT);
}

void CMapView::OnUpdateTravToggle(CCmdUI* pCmdUI)
{
     if(IsReviewNode() && pSV->IsTravVisible()) {
		 pCmdUI->Enable(pCV->m_iStrFlag>0 && (pCV->m_iStrFlag&(NET_STR_FLOATV|NET_STR_FLOATH))!=0);
		 return;
	 }
	 pCmdUI->Enable(FALSE);
}

void CMapView::OnUpdate(CView* pSender,LPARAM lHint,CObject* pHint) 
{
	switch(lHint) {
		case CPrjDoc::LHINT_CLOSEMAP:
			if(m_pFrame->m_pFrameNode==(CPrjListNode *)pHint) {
				m_pFrame->PostMessage(WM_CLOSE);
			}
			return;

		case CPrjDoc::LHINT_FLGCHECK:
		case CPrjDoc::LHINT_FLGREFRESH:
		    {
				if(!m_pFrame->m_pDoc || m_pFrame->m_pFrameNode!=CPrjDoc::m_pReviewNode ||
					!((UINT)pHint==M_TOGGLEFLAGS || ((UINT)pHint & m_pFrame->m_flags)!=0)) return;
				if(lHint==CPrjDoc::LHINT_FLGREFRESH) RefreshView();
				CPrjDoc::m_nMapsUpdated++;
				return;
		    }

        case CPrjDoc::LHINT_REFRESHNET:
			ASSERT(!m_pFrame->m_pDoc || m_pFrame->m_pDoc==CPrjDoc::m_pReviewDoc);
			#ifdef _DEBUG
			  DWORD netFrm=m_pFrame->m_dwReviewNetwork;
			  DWORD netNTN=dbNTN.Position();
			#endif
			if(m_pFrame->m_pDoc && m_pFrame->m_pFrameNode==CPrjDoc::m_pReviewNode &&
				    m_pFrame->m_dwReviewNetwork == dbNTN.Position())
		    {
				if(CPrjDoc::m_nMapsUpdated<0) {
					pCV->PlotTraverse(0); //Identify selected system and travserse for highlighting
					CPrjDoc::m_nMapsUpdated=0;
				}
				RefreshView();
				CPrjDoc::m_nMapsUpdated++;
			}
			return;
	}
}

void CMapView::OnActivateView(BOOL bActivate, CView* pActiveView, CView* pDeactiveView)
{
	CView::OnActivateView(bActivate, pActiveView, pDeactiveView);
	if(bActivate && m_pFrame->m_pDoc && IsReviewNode()) m_pFrame->SetDocMapFrame();
}
