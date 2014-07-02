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
#include <commctrl.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapView

IMPLEMENT_DYNCREATE(CMapView, CScrollView)

HCURSOR CMapView::m_hCursorPan=0;
HCURSOR CMapView::m_hCursorCross=0;
HCURSOR CMapView::m_hCursorArrow=0;
HCURSOR CMapView::m_hCursorMeasure=0;
HCURSOR CMapView::m_hCursorIdentify=0;
BOOL CMapView::m_bMeasureDist=FALSE;
CPoint CMapView::m_ptMeasureStart;
CPoint CMapView::m_ptMeasureEnd;
DPOINT CMapView::m_Coordinate;
DPOINT CMapView::m_CoordinateMeasure;

CMapView::CMapView()
{
	m_bPanning=m_bMeasuring=m_bInContext=FALSE;
	m_bScrollBars=m_bIdentify=FALSE;
	m_pVecNode=NULL;
}

CMapView::~CMapView()
{
}

BEGIN_MESSAGE_MAP(CMapView, CScrollView)
	//{{AFX_MSG_MAP(CMapView)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_COMMAND(ID_MAP_CENTER, OnMapCenter)
	ON_COMMAND(ID_MAP_LABELS, OnMapLabels)
	ON_COMMAND(ID_MAP_ORIGINAL, OnMapOriginal)
	ON_COMMAND(ID_MAP_ZOOMIN, OnMapZoomin)
	ON_COMMAND(ID_MAP_ZOOMOUT, OnMapZoomout)
	ON_WM_LBUTTONDBLCLK()
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
	ON_UPDATE_COMMAND_UI(ID_MAP_ZOOMIN, OnUpdateMapZoomin)
	ON_COMMAND(ID_MEASUREDIST, OnMeasureDist)
	ON_UPDATE_COMMAND_UI(ID_MEASUREDIST, OnUpdateMeasureDist)
	ON_COMMAND(ID_MAP_PREFIXES, OnMapPrefixes)
	ON_UPDATE_COMMAND_UI(ID_MAP_PREFIXES, OnUpdateMapPrefixes)
	ON_COMMAND(ID_MAP_VECTORS, OnMapVectors)
	ON_UPDATE_COMMAND_UI(ID_MAP_VECTORS, OnUpdateMapVectors)
	ON_COMMAND(ID_MAP_ELEVATIONS, OnMapElevations)
	ON_UPDATE_COMMAND_UI(ID_MAP_ELEVATIONS, OnUpdateMapElevations)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_LOCATEINGEOM, OnLocateInGeom)
	ON_COMMAND(ID_GO_EDIT, OnGoEdit)
	ON_COMMAND(ID_VECTORINFO, OnVectorInfo)
	ON_UPDATE_COMMAND_UI(ID_LOCATEINGEOM, OnUpdateLocateInGeom)
	ON_COMMAND(ID_RESIZEMAPFRAME, OnResizemapframe)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SEL,OnUpdateUTM)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_UTM,OnUpdateUTM)
END_MESSAGE_MAP()

BOOL CMapView::PreCreateWindow(CREATESTRUCT& cs)
{
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.
	
	if(!CScrollView::PreCreateWindow(cs)) return FALSE;
	cs.dwExStyle&=~WS_EX_CLIENTEDGE;
	return TRUE;	  
}

void CMapView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
    
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
    

    if(width>=rect.Width()) width=rect.Width();
    else left+=(rect.Width()-width)/2;
    if(height>=rect.Height()) height=rect.Height();
    else top+=(rect.Height()-height)/2;
    
	m_pFrame->SetWindowPos(NULL,left,top,width,height,SWP_HIDEWINDOW|SWP_NOACTIVATE);


	SetScrollSizes(MM_TEXT,m_pFrame->m_sizeBmp);
	m_rectBmp.SetRect(0,0,m_pFrame->m_sizeBmp.cx,m_pFrame->m_sizeBmp.cy);
	m_bProfile=m_pFrame->m_bProfile;
	m_bFeetUnits=m_pFrame->m_bFeetUnits;
	m_bCursorInBmp=m_bTracking=FALSE;
	m_bIdentify=!m_bMeasureDist && (m_pFrame->m_pVnode || m_pFrame->m_pVnodeF);
	m_pVecNode=NULL;

	//RecalcLayout
	//m_pFrame->SetWindowText(NULL); //Overridden version will use pNode->Title()
}

void CMapView::OnDraw(CDC* pDC)
{
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

void CMapView::CenterBitmap()
{
	//Assume we are positioned at some random scroll position.
	//We want to place center of bitmap at center of window.
	if(m_bScrollBars) {
	  int xs=0,ys=0;
	  SCROLLINFO info;
	  if(GetScrollInfo(SB_HORZ,&info)) {
		  ASSERT(info.nMin==0);
		  xs=(info.nMax-info.nPage)/2;
		  SetScrollPos(SB_HORZ,xs);
		  xs-=info.nPos;
	  }
	  if(GetScrollInfo(SB_VERT,&info)) {
		  ASSERT(info.nMin==0);
		  ys=(info.nMax-info.nPage)/2;
		  SetScrollPos(SB_VERT,ys);
		  ys-=info.nPos;
	  }
	  if(ys || xs) ScrollWindow(xs,ys);
	  GetCoordinate(m_ptPopupRaw);
	}
}

void CMapView::OnLButtonDown(UINT nFlags, CPoint point)
{
	ClearLocate();

	if(m_bScrollBars && (nFlags&MK_CONTROL)) {
	  //int min;
	  SCROLLINFO info;
	  //GetScrollRange(SB_HORZ,&min,&m_iPosMaxX);
	  GetScrollInfo(SB_HORZ,&info);
	  m_iPosX=info.nPos;
	  m_iPosMaxX=info.nMax-info.nPage+1;
	  //GetScrollRange(SB_VERT,&min,&m_iPosMaxY);
	  GetScrollInfo(SB_VERT,&info);
	  m_iPosY=info.nPos;
	  m_iPosMaxY=info.nMax-info.nPage+1;
	  if(m_iPosMaxX>0 && m_iPosMaxY>0) {
		  //m_iPosX=GetScrollPos(SB_HORZ);
		  //m_iPosY=GetScrollPos(SB_VERT);
		  m_iPtrX=point.x;
		  m_iPtrY=point.y;
		  m_bPanning=TRUE;
		  //SetCapture();
	  }
	}
	else if(m_bMeasureDist) {
		GetCoordinate(point);
		m_CoordinateMeasure=m_Coordinate;
		m_ptMeasureStart=m_ptMeasureEnd=point;
		m_bMeasuring=TRUE;
	}
	else if(m_pFrame->m_pFrameNode==CPrjDoc::m_pReviewNode) {
		CFixedTracker tracker;
		tracker.m_pr=&m_rectBmp;
		m_bPanning=TRUE;
		//Sets capture and waits for LButtonUp (may be part of a double-click) --
		if(tracker.TrackRubberBand(this, point, TRUE))
		{
			tracker.m_rect.NormalizeRect();
			int pxwidth=tracker.m_rect.Width();
			if(pxwidth>=MIN_TRACKERWIDTH) {
				if(pxwidth/m_pFrame->m_scale<0.1) AfxMessageBox(IDS_ERR_PAGEZOOM);
				else {
					tracker.m_rect.OffsetRect(GetDeviceScrollPosition());
					NewView(tracker.m_rect);
					CenterBitmap();
				}
			}
		}
		m_bPanning=FALSE;
	}
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
	CPoint pt=GetDeviceScrollPosition();
	rect.OffsetRect(-pt.x,-pt.y);
	CClientDC dc(this);
	dc.DrawFocusRect(rect);
}

void CMapView::RefreshMeasure()
{
    CClientDC dc(this);
    dc.SetROP2(R2_NOT);
    dc.SetBkMode(TRANSPARENT);
    dc.MoveTo(m_ptMeasureStart);
	dc.LineTo(m_ptMeasureEnd);
}

void CMapView::OnMouseMove(UINT nFlags, CPoint point)
{
	if((nFlags & MK_RBUTTON) || !(m_bCursorInBmp=m_rectBmp.PtInRect(point))) return;

	if(m_bPanning && (nFlags & MK_LBUTTON)) {
	  int posX,posY;

	  posX=m_iPosX+(m_iPtrX-point.x);
	  if(posX<0) posX=0;
	  else if(posX>m_iPosMaxX) posX=m_iPosMaxX;
	  
	  posY=m_iPosY+(m_iPtrY-point.y);
	  if(posY<0) posY=0;
	  else if(posY>m_iPosMaxY) posY=m_iPosMaxY;
	  
	  if(posX!=m_iPosX || posY!=m_iPosY) {
		  ScrollWindow(m_iPosX-posX,m_iPosY-posY); 
		  if(posX!=m_iPosX) SetScrollPos(SB_HORZ,m_iPosX=posX);
		  if(posY!=m_iPosY) SetScrollPos(SB_VERT,m_iPosY=posY);
	  }
	  
	  m_iPtrX=point.x;
	  m_iPtrY=point.y;
	  return;
	}

	GetCoordinate(point);
	
	if(m_bMeasuring) {
		RefreshMeasure();
		m_ptMeasureEnd=point;
		RefreshMeasure();
	}
	else {
		if(m_bIdentify) {
			ASSERT(m_pFrame->m_pVnode || m_pFrame->m_pVnodeF);
			ClearLocate();
			point+=GetDeviceScrollPosition();
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
		m_bMeasuring=FALSE;
	}
	m_bPanning=FALSE;
	//::ReleaseCapture();
}

void CMapView::RefreshCursor()
{
	HCURSOR hc;

	if(m_bMeasureDist) {
		ClearLocate();
		m_bIdentify=FALSE;
		hc=m_hCursorMeasure;
	}
	else if(m_pFrame->m_pFrameNode==CPrjDoc::m_pReviewNode) {
		m_bIdentify=(m_pFrame->m_pVnode || m_pFrame->m_pVnodeF);
		hc=m_bIdentify?m_hCursorIdentify:m_hCursorCross;
	}
	else {
		ClearLocate();
		m_bIdentify=FALSE;
		hc=m_hCursorArrow;
	}

	VERIFY(::SetCursor(hc));

	/*
	if(m_bIdentify) ::SetCursor(m_hCursorIdentify);
	else if(m_bMeasureDist) ::SetCursor(m_hCursorMeasure);
	else ::SetCursor((m_pFrame->m_pFrameNode==CPrjDoc::m_pReviewNode)?m_hCursorCross:m_hCursorArrow);
	*/
}

BOOL CMapView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if(m_bScrollBars && GetKeyState(VK_CONTROL)<0) {
	    ::SetCursor(m_hCursorPan);
	}
	else RefreshCursor();
	return TRUE;

	//return CScrollView::OnSetCursor(pWnd, nHitTest, message);
}

void CMapView::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);
	
	if(cx>0 && cy>0) {
		DWORD style=::GetWindowLong(m_hWnd, GWL_STYLE);
		m_bScrollBars=(style&WS_HSCROLL)!=0;
	}
}

void CMapView::GetCoordinate(CPoint point)
{
	point+=GetDeviceScrollPosition();

	//coord.x=x*LEN_SCALE(cosA/xscale) - y*LEN_SCALE(sinA/xscale) + LEN_SCALE(xoff*cosA-yoff*sinA+midE);
	//coord.y=-x*LEN_SCALE(sinA/xscale) - y*LEN_SCALE(cosA/xscale) - LEN_SCALE(xoff*sinA+yoff*cosA-midN);

	point.y+=2;
	point.x+=2;

	if(m_bProfile) m_Coordinate.y=m_pFrame->m_cosAx+m_pFrame->m_midNx*(point.y);
	else {
		m_Coordinate.x = point.x*m_pFrame->m_cosAx -
			point.y*m_pFrame->m_sinAx + m_pFrame->m_midEx;
		m_Coordinate.y =-point.x*m_pFrame->m_sinAx -
			point.y*m_pFrame->m_cosAx - m_pFrame->m_midNx;
	}
}

void CMapView::GetDistanceFormat(CString &s,BOOL bProfile,BOOL bFeetUnits)
{
	double dy=m_Coordinate.y-m_CoordinateMeasure.y;
	if(bProfile) s.Format("%10.1f %s vert",dy,bFeetUnits?"ft":"m"); 
	else {
		double dx=m_Coordinate.x-m_CoordinateMeasure.x;
		double az=atan2(dx,dy)*(180.0/3.141592653589793);
		if(az<0.0) az+=360.0;
		s.Format("%10.1f %s%10.1f deg",sqrt(dx*dx+dy*dy),
			bFeetUnits?"ft":"m",az);
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
			if(m_bMeasuring) GetDistanceFormat(s,m_bProfile,m_bFeetUnits);
			else GetCoordinateFormat(s,m_Coordinate,m_bProfile);
		}
		pCmdUI->SetText(s); 
	}
}

LRESULT CMapView::OnMouseLeave(WPARAM wParam,LPARAM strText)
{
	m_bCursorInBmp=m_bTracking=FALSE;
	if(m_bMeasuring) {
		RefreshMeasure();
		m_bMeasuring=FALSE;
	}

	if(!m_bInContext) ClearLocate();

	return FALSE;
}

void CMapView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(m_bCursorInBmp && (nFlags&MK_CONTROL)) {
		if(m_bScrollBars) ::SetCursor(m_hCursorPan);
		else RefreshCursor();
	}
	CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMapView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(m_bCursorInBmp && (nFlags&MK_CONTROL)) RefreshCursor();
	CScrollView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CMapView::NewView(CRect &rect)
{
	if(m_pFrame!=CPlotView::m_pMapFrame) {
		m_pFrame->RemoveThis();
		m_pFrame->m_pNext=CPlotView::m_pMapFrame;
		CPlotView::m_pMapFrame=m_pFrame;
	}
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
	m_ptPopup.x-=m_rectBmp.Width()/2;
	m_ptPopup.y-=m_rectBmp.Height()/2;
	NewView(CRect(m_ptPopup,m_pFrame->m_sizeBmp));
	CenterBitmap();
}

void CMapView::OnMapOriginal() 
{
	//Restore original view --
	m_pFrame->m_xoff=m_pFrame->m_org_xoff;
	m_pFrame->m_yoff=m_pFrame->m_org_yoff;
	m_pFrame->m_scale=m_pFrame->m_org_scale;
	NewView(CRect(0,0,m_pFrame->m_sizeBmp.cx,m_pFrame->m_sizeBmp.cy));
	CenterBitmap();
}

void CMapView::OnMapZoomin() 
{
	m_ptPopup.x-=m_rectBmp.Width()/4;
	m_ptPopup.y-=m_rectBmp.Height()/4;
	NewView(CRect(m_ptPopup.x,m_ptPopup.y,
		m_ptPopup.x+m_pFrame->m_sizeBmp.cx/2,m_ptPopup.y+m_pFrame->m_sizeBmp.cy/2));
	CenterBitmap();
}

void CMapView::OnUpdateMapZoomin(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pFrame->m_scale<(double)m_pFrame->m_sizeBmp.cx);
}

void CMapView::OnMapZoomout() 
{
	m_ptPopup.x-=m_rectBmp.Width();
	m_ptPopup.y-=m_rectBmp.Height();
	NewView(CRect(m_ptPopup.x,m_ptPopup.y,
		m_ptPopup.x+2*m_pFrame->m_sizeBmp.cx,m_ptPopup.y+2*m_pFrame->m_sizeBmp.cy));
	CenterBitmap();
}

void CMapView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if(m_pFrame->m_pFrameNode==CPrjDoc::m_pReviewNode) {
		m_ptPopup=point+GetDeviceScrollPosition();
		OnMapCenter();
	}
	else OnMapViewhelp();
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

	NewView(CRect(0,0,m_pFrame->m_sizeBmp.cx,m_pFrame->m_sizeBmp.cy));
	ASSERT(!CMapFrame::m_bKeepVnode);
	CMapFrame::m_bKeepVnode=FALSE;
}

void CMapView::RefreshView()
{
	CMapFrame::m_bKeepVnode=!pSV->IsNewVisibility();

	CRect rect(0,0,m_pFrame->m_sizeBmp.cx,m_pFrame->m_sizeBmp.cy);
	m_pFrame->m_pCRect=&rect;
	pPV->m_bExternUpdate=2;
	CPrjDoc::PlotFrame(m_pFrame->m_flags);
	m_bFeetUnits=m_pFrame->m_bFeetUnits;
	pPV->m_bExternUpdate=0;
	m_pVecNode=NULL;
	ASSERT(!CMapFrame::m_bKeepVnode);
	CMapFrame::m_bKeepVnode=FALSE;
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

void CMapView::OnMapPassage() 
{
	ToggleLayer(M_LRUDS);
}

void CMapView::OnMapVectors() 
{
	ToggleLayer(M_NOVECTORS);
}

void CMapView::OnUpdateMapVectors(CCmdUI* pCmdUI) 
{
	BOOL bCheck=(m_pFrame->m_flags&M_NOVECTORS)==0;
	pCmdUI->SetCheck(bCheck);
	pCmdUI->Enable(bCheck || pSV->IsLinesVisible());
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
	if(m_bMeasureDist=!m_bMeasureDist) {
		ClearLocate();
		m_bIdentify=FALSE;
	}
}

void CMapView::OnUpdateMeasureDist(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bMeasureDist);
}

void CMapView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu menu;
	UINT id=IDR_MAP_FRAMEINFO;
	if(m_pFrame->m_pFrameNode==CPrjDoc::m_pReviewNode) {
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
		m_ptPopup=m_ptPopupRaw+GetDeviceScrollPosition();
		m_bInContext=TRUE;
		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,m_pFrame);
		m_bInContext=FALSE;
	}
	ClearLocate();
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

void CMapView::OnResizemapframe()
{

	CWindowDC dc(NULL);
	int px_per_in=dc.GetDeviceCaps(LOGPIXELSX);
	ASSERT(px_per_in>0);
	double widthInches=(double)m_pFrame->m_sizeBmp.cx/px_per_in;

	CResizeFrmDlg dlg(&widthInches,px_per_in);
	if(dlg.DoModal()==IDOK) {
		if(dlg.m_bSave) {
			CPlotView::m_mfFrame.fFrameWidthInches=widthInches;
			CPlotView::m_mfFrame.bChanged=TRUE;
		}
		if(widthInches!=(double)m_pFrame->m_sizeBmp.cx/px_per_in) {
			px_per_in=(int)(widthInches*px_per_in);
			CRect rect(0,0,px_per_in,px_per_in);
			m_pFrame->m_pCRect=&rect;
			pPV->m_bExternUpdate=3;
			CPrjDoc::PlotFrame(m_pFrame->m_flags);
		}
	}
}
