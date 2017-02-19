// PageView.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "compview.h"
#include "PageOffDlg.h"
#include "PageView.h"
#include "segview.h"
#include "compile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPageView

IMPLEMENT_DYNCREATE(CPageView, CPanelView)

CRect CPageView::m_rectBmp;
CDC CPageView::m_dcBmp;
CPoint * CPageView::m_selpt;
int * CPageView::m_selseq;
UINT CPageView::m_numsel;

static CPoint ptPan;
static BOOL bPanValidated;

CPageView::CPageView()
	: CPanelView(CPageView::IDD)
{
	//{{AFX_DATA_INIT(CPageView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPageView::~CPageView()
{
	if(m_selpt) {
		free(m_selpt);
		free(m_selseq);
		m_numsel=0;
		m_selpt=NULL;
		m_selseq=NULL;
	}
}

void CPageView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageView)
	DDX_Control(pDX, IDC_PAGFRM, m_PagFrm);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPageView, CPanelView)
	ON_WM_SYSCOMMAND()
	ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
	//{{AFX_MSG_MAP(CPageView)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_ZOOMIN, OnPanelZoomIn)
	ON_BN_CLICKED(IDC_ZOOMOUT, OnPanelZoomOut)
	ON_BN_CLICKED(IDC_DEFAULT, OnPanelDefault)
	ON_BN_CLICKED(IDC_LANDSCAPE, OnLandscape)
	ON_BN_CLICKED(ID_FILE_PRINT_SETUP, OnFilePrintSetup)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_BN_CLICKED(IDC_CHGVIEW, OnChgView)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_BN_CLICKED(IDC_SELECTALL, OnSelectAll)
	ON_BN_CLICKED(IDC_CLEARALL, OnClearAll)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_PAGEOFFSETS, OnPageOffsets)
	ON_COMMAND(IDC_DISPLAYOPTIONS, OnDisplayOptions)
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_UTM,OnUpdateUTM)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SEL,OnUpdateUTM)
	ON_COMMAND(ID_MAP_EXPORT, OnMapExport)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageView diagnostics

#ifdef _DEBUG
void CPageView::AssertValid() const
{
	CFormView::AssertValid();
}

void CPageView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPageView message handlers
void CPageView::OnSysCommand(UINT nChar,LONG lParam)
{
	//Handle keystrokes here --
	
	CPanelView::OnSysCommand(nChar,lParam);
}

void CPageView::OnInitialUpdate()
{
    static int dlgfont_id[]={IDC_ST_PANEL,IDC_ST_PRINTER,
		/*IDC_ST_FRAMESIZE,IDC_ST_MAPSCALE,IDC_ST_PRINTAREA,IDC_PAGES,*/0};
      
    CPanelView::OnInitialUpdate();
    
    //Set fonts of most controls to a more pleasing non-bold type. NOTE: Changing the
    //font in the dialog template would adversely affect button sizes, etc. Also, non-bold
    //type styles apparently can't be specified in the template!
    
    GetReView()->SetFontsBold(GetSafeHwnd(),dlgfont_id);
    
    m_PagFrm.GetClientRect(&m_rectBmp);

    m_sizeBmp.cx = m_rectBmp.right-2; //Subtract 2 since we will InflateRect(-1,-1)
    m_sizeBmp.cy = m_rectBmp.bottom-2;

    // Convert to screen coordinates using static as base,
    // then to DIALOG (instead of static) client coords 
    // using dialog as base
    m_PagFrm.ClientToScreen(&m_rectBmp);
    ScreenToClient(&m_rectBmp);
    m_rectBmp.InflateRect(-1,-1);
        
    m_ptBmp.x = m_rectBmp.left;
    m_ptBmp.y = m_rectBmp.top;
    
    // Get temporary DC for dialog - Will be released in dc destructor
    CClientDC dc(this);
    
    //Create CBitmap for plot --
    // m_dcBmp.SelectObject will not work with planes=1, bits=4 -- Why not?
    //UINT planes=(UINT)GetDeviceCaps(dc.m_hDC, PLANES);  //=1
    //UINT bits=(UINT)GetDeviceCaps(dc.m_hDC, BITSPIXEL); //=8 (this works)
	//VERIFY(m_cBmp.CreateBitmap(m_sizeBmp.cx,m_sizeBmp.cy,planes,bits,NULL));
	                   
    //Might as well use this --
    //Allocates 160128+32 bytes of GPI private (assuming 1024x756x256) --
    //Allocates 313120+32 bytes of GPI private (assuming 1024x756x64K) --
    if(!(m_cBmp.CreateCompatibleBitmap(&dc,m_sizeBmp.cx,m_sizeBmp.cy))) {
		ASSERT(FALSE);
	}
    
    //For efficient traverse highlighting, we need another similar bitmap --
    VERIFY(m_cBmpBkg.CreateCompatibleBitmap(&dc,m_sizeBmp.cx,m_sizeBmp.cy));
    
    //Our strategy in CPrjDoc::PlotTraverse() --
    //  1) Render m_cBmpBkg with network component and highlighted system
    //     assuming view and/or system has changed.
    //  2) SRCCOPY m_cBmpBkg onto m_cBmp
    //  3) Render m_cBmp with highlighted traverse
	    
    // Create compatible memory DC to hold m_cBmp --
    VERIFY(m_dcBmp.CreateCompatibleDC(&dc));
    VERIFY(m_hBmpOld=(HBITMAP)::SelectObject(m_dcBmp.m_hDC,m_cBmp.GetSafeHandle()));
    VERIFY(m_hBrushOld=(HBRUSH)::SelectObject(m_dcBmp.m_hDC,CReView::m_hPltBrush));
    VERIFY(m_hPenOld=(HPEN)::SelectObject(m_dcBmp.m_hDC,CReView::m_PenNet.GetSafeHandle()));

    //m_hCursorBmp=AfxGetApp()->LoadStandardCursor(IDC_CROSS);
	
	//m_tracker.m_nStyle=CRectTracker::resizeInside|CRectTracker::solidLine;
    ResetContents();
}

void CPageView::ResetContents()
{
	m_bCursorInBmp=m_bTracking=FALSE;
	m_dcBmp.PatBlt(0,0,m_sizeBmp.cx,m_sizeBmp.cy,PATCOPY);
	InvalidateRect(&m_rectBmp,FALSE);
	CPrjDoc::m_bPageActive=FALSE;
	GetDlgItem(IDC_PAGES)->SetWindowText("1");
	m_numsel=0;
}

void CPageView::DrawFrame(CDC *pDC,CRect *pClip)
{
	CRect rect=m_rectBmp;
	rect.InflateRect(1,1);
    if(!pClip->IntersectRect(pClip,&rect)) return;

  //Draw shadowed frame --
	CPen *oldpen=(CPen *)pDC->SelectObject(&CReView::m_PenNet);
	//if(pClip->left<=rect.left) {
	  pDC->MoveTo(rect.left,rect.top);
	  pDC->LineTo(rect.left,rect.bottom);
	//}
	//if(pClip->top<=rect.top) {
	  pDC->MoveTo(rect.left,rect.top);
	  pDC->LineTo(rect.right,rect.top);
	//}

	pDC->SelectStockObject(WHITE_PEN);
	//if(pClip->bottom>=rect.bottom) {
	  pDC->MoveTo(rect.left,rect.bottom);
	  pDC->LineTo(rect.right,rect.bottom);
	//}
	//if(pClip->right>=rect.right) {
	  pDC->MoveTo(rect.right,rect.top);
	  pDC->LineTo(rect.right,rect.bottom);
	//}
	pDC->SelectObject(oldpen);
}

void CPageView::OnDraw(CDC* pDC)
{
	// The window has been invalidated and needs to be repainted;
	
	CPanelView::OnDraw(pDC);
     
	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	
	CRect rectPlot;
	if(rectPlot.IntersectRect(&m_rectBmp,&rectClip)) {
	    bPanValidated=TRUE;
	    pDC->BitBlt(rectPlot.left,rectPlot.top,rectPlot.Width(),rectPlot.Height(),
	  	  &m_dcBmp,rectPlot.left-m_rectBmp.left,rectPlot.top-m_rectBmp.top,SRCCOPY);
	  	//if(m_bTrackerActive) m_tracker.Draw(pDC);
		if(rectPlot!=rectClip) DrawFrame(pDC,&rectClip);
	}
	else DrawFrame(pDC,&rectClip);
}

void CPageView::OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint)
{
	//No updating from CPrjDoc for now.
	//We must override this to prevent other UpdateAllView() broadcasts
	//from invalidating the entire view (?)
}


void CPageView::OnDestroy() 
{
	CPanelView::OnDestroy();
	
	//NOTE: We use API calls here to avoid MFC's exception-prone
	//FromHandle() calls --
	
    VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(),m_hBmpOld));
    VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(),m_hPenOld));
    VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(),m_hBrushOld));
    
    VERIFY(m_dcBmp.DeleteDC());
    
    // ~CBitmap() should free memory for the bitmap --
    // m_cBmp.DeleteObject();
}

void CPageView::OnPanelZoomIn() 
{
	if(m_fPanelWidth/(1.0+PERCENT_ZOOMOUT/100.0)<0.1) AfxMessageBox(IDS_ERR_PAGEZOOM);
	else CPrjDoc::PlotPageGrid(1);
}

void CPageView::OnPanelZoomOut() 
{
	double pxscale=m_sizeBmp.cx/(m_fPanelWidth*(1.0+PERCENT_ZOOMOUT/100.0));
	if(m_fViewWidth*pxscale>=2.0 && m_fViewHeight*pxscale>=2.0) CPrjDoc::PlotPageGrid(-1);
}

void CPageView::OnPanelDefault() 
{
	CPrjDoc::PlotPageGrid(-2);
	RefreshPages();
}

void CPageView::OnLandscape() 
{
	((CWallsApp *)AfxGetApp())->SetLandscape(IsDlgButtonChecked(IDC_LANDSCAPE));
	CPrjDoc::PlotPageGrid(4);
}

void CPageView::OnFilePrintSetup() 
{
	((CWallsApp *)AfxGetApp())->OnFilePrintSetup();
}

void CPageView::GetLoc(DPOINT *pt,CPoint *p)
{
	pt->x=(p->x-m_ptBmp.x)/m_xscale+m_xoff+m_xpan;
	pt->y=(p->y-m_ptBmp.y)/m_xscale+m_yoff+m_ypan;
}

void CPageView::GetCell(CPoint *p,double x,double y)
{
	x-=m_pvxoff;
	y-=m_pvyoff;
	if(x<0.0) x-=m_fViewWidth;
	if(y<0.0) y-=m_fViewHeight;
	p->x=(int)(x/m_fViewWidth);
	p->y=(int)(y/m_fViewHeight);
}

static int FAR PASCAL comp_sel_fcn(int i)
{
	int t=CPageView::m_selpt[CPageView::m_numsel].y;
	if(i<0) i=-i;
	i--;
	if(CPageView::m_selpt[i].y==t) {
	    t=CPageView::m_selpt[CPageView::m_numsel].x;
		if(t==CPageView::m_selpt[i].x) return 0;
		return (t>CPageView::m_selpt[i].x)?1:-1;
	}
	return (t>CPageView::m_selpt[i].y)?1:-1;
}

int * CPageView::SelectCell(int x,int y)
{
	if(!m_selpt) {
		if(m_numsel || m_selseq) {
			ASSERT(FALSE);
		}
		if(!(m_selpt=(CPoint *)malloc((MAX_NUMSEL+1)*sizeof(CPoint)))) return NULL;
		if(!(m_selseq=(int *)malloc(MAX_NUMSEL*sizeof(DWORD)))) {
			free(m_selpt); m_selpt=NULL;
			return NULL;
		}
	}
	if(!m_numsel) trx_Bininit((DWORD *)m_selseq,(DWORD *)&m_numsel,comp_sel_fcn);
	m_selpt[m_numsel].x=x;
	m_selpt[m_numsel].y=y;

	if(m_numsel>=MAX_NUMSEL && !trx_Blookup(TRX_DUP_NONE)) {
		AfxMessageBox(IDS_ERR_MAXPAGES);
		return NULL;
	}
	return (int *)trx_Binsert(m_numsel+1,TRX_DUP_NONE);
}

void CPageView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if(m_rectBmp.PtInRect(point)) {
		DPOINT pt;
		GetLoc(&pt,&point);
		GetCell(&point,&pt);
		int *pseq=SelectCell(point);
		if(pseq) {
			if(trx_binMatch) *pseq=-*pseq;
			CPrjDoc::PlotPageGrid(0);
			RefreshPages();
		}
	}
}

void CPageView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if(m_rectBmp.PtInRect(point)) {
		//Pan image --
		DPOINT pt;
		GetLoc(&pt,&point);
		GetCell(&point,&pt);
		int *pseq=SelectCell(point);
		if(pseq && trx_binMatch && *pseq<0) *pseq=-*pseq;
		pt.x=m_fViewWidth*(point.x+0.5);
		pt.y=m_fViewHeight*(point.y+0.5);
		//(x,y) is now frame center wrt printer page's top left corner
		pt.y-=(m_fViewHeight-(pPV->m_sizeBmp.cy*m_fViewWidth)/pPV->m_sizeBmp.cx)/2.0;
		//(x,y) is now frame center wrt preview map's top left corner
		pPV->m_pPanPoint=&pt;
		pCV->PlotTraverse(-2);
		pPV->m_pPanPoint=NULL;
		m_xpan=m_ypan=0.0;
	    for(int j,i=0;i<(int)m_numsel;i++) {
		  j=abs(m_selseq[i])-1;
		  m_selpt[j].x-=point.x;
		  m_selpt[j].y-=point.y;
		}
		CPrjDoc::PlotPageGrid(2);
		RefreshPages();
	}
}

void CPageView::OnChgView() 
{
	pPV->OnChgView();
	RefreshPages();
}

void CPageView::OnFilePrintPreview() 
{
	pPV->OnFilePrintPreview();
}

void CPageView::OnFilePrint() 
{
	pPV->OnFilePrint();
}

void CPageView::OnSelectAll() 
{
	CPrjDoc::SelectAllPages();
	CPrjDoc::PlotPageGrid(0);
	RefreshPages();
}

void CPageView::OnClearAll() 
{
	m_numsel=0;	
	CPrjDoc::PlotPageGrid(0);
	RefreshPages();
}

UINT CPageView::GetNumPages()
{
	UINT i,n=0;
	for(i=0;i<m_numsel;i++) if(m_selseq[i]>0) n++;
	return n;
}

BOOL CPageView::CheckSelected()
{
	if(!GetNumPages()) {
		AfxMessageBox(IDS_ERR_NOPAGES);
		return FALSE;
	}
	return TRUE;
}

int CPageView::GetFrameID(char *buf,int len)
{
	return _snprintf(buf,len,"Frame %d:%d ",m_selpt[m_iCurSel].y,m_selpt[m_iCurSel].x);
}

void CPageView::AdjustPageOffsets(double *pxoff,double *pyoff,UINT npage)
{
	UINT i,n=0;
	for(i=0;i<m_numsel;i++)
		if(m_selseq[i]>0 && ++n==npage) {
			m_iCurSel=m_selseq[i]-1;
			*pxoff+=m_selpt[m_iCurSel].x*m_fViewWidth;
			*pyoff+=m_selpt[m_iCurSel].y*m_fViewHeight;
			break;
		}
}

void CPageView::RefreshPages()
{
	GetDlgItem(IDC_PAGES)->SetWindowText(GetIntStr(GetNumPages()));
}

void CPageView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if(m_rectBmp.PtInRect(point) && (nFlags&MK_CONTROL)) {
		bPanValidated=TRUE;
		ptPan=point;
		return;
    }
	CPanelView::OnLButtonDown(nFlags, point);
}

void CPageView::PageDimensionStr(char *buf,BOOL bInches)
{
	DimensionStr(buf,(double)m_fInchesX,(double)m_fInchesY,bInches);
}

void CPageView::RefreshPageInfo()
{
	BOOL bLandscape;
	int m_hres,m_vres,m_pxperinX,m_pxperinY;
	char buf[60];

	if(((CWallsApp *)AfxGetApp())->GetPrinterPageInfo(&bLandscape,&m_hres,&m_vres,&m_pxperinX,&m_pxperinY)) {
		m_fInchesX=(float)((double)m_hres/m_pxperinX);
		m_fInchesY=(float)((double)m_vres/m_pxperinY);
	}
	else {
		m_fInchesX=(float)CPlotView::m_mfPrint.fFrameWidthInches;
		m_fInchesY=(float)CPlotView::m_mfPrint.fFrameHeightInches;
		if(m_fInchesX<=1.1 || m_fInchesY<=1.1) {
			m_fInchesX=9.5;
			m_fInchesY=7.5;
		}
		bLandscape=TRUE;
	}
    ((CButton *)GetDlgItem(IDC_LANDSCAPE))->SetCheck(bLandscape);

	PageDimensionStr(buf,pPV->m_bInches);
	GetDlgItem(IDC_ST_PRINTAREA)->SetWindowText(buf);
}

void CPageView::RefreshScale()
{
	char buf[60];

	double wframe=m_fFrameWidth;
	double wview=pPV->m_fPanelWidth;
	BOOL bInches=pPV->m_bInches;
	BOOL bFeet=LEN_ISFEET();

	DimensionStr(buf,wframe,m_fFrameHeight,bInches);
	GetDlgItem(IDC_ST_FRAMESIZE)->SetWindowText(buf);

	if(!bInches) wframe*=2.54;
	if(bFeet) wview/=0.3048;

	_snprintf(buf,60,"1 %s = %.2f %s",bInches?"in":"cm",
		wview/wframe,bFeet?"ft":"m");
	GetDlgItem(IDC_ST_MAPSCALE)->SetWindowText(buf);
}


void CPageView::OnPageOffsets() 
{
	VIEWFORMAT *pVF=&pPV->m_vfSaved[pPV->m_bProfile];
	CPageOffDlg dlg(pVF,(double)m_fInchesX,(double)m_fInchesY);

	if(dlg.DoModal()==IDOK && pVF->bChanged) {
		CPrjDoc::PlotPageGrid(4);
	}
}

void CPageView::OnMapExport()
{
	pPV->OnMapExport();
}

void CPageView::OnDisplayOptions() 
{
	((CMainFrame *)AfxGetMainWnd())->SetOptions(1);
}

void CPageView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	//CPanelView::OnMouseMove(nFlags, point);
	BOOL bCursorInBmp=m_rectBmp.PtInRect(point);

	if(bCursorInBmp!=m_bCursorInBmp) {
	  if(bCursorInBmp) ::SetCursor(pPV->m_hCursorBmp);
	  m_bCursorInBmp=bCursorInBmp;
	}

	if(!bCursorInBmp) return;
	
	if((nFlags&(MK_CONTROL+MK_LBUTTON))>MK_CONTROL) {
		if(bPanValidated) {
    		point.x-=ptPan.x;
    		point.y-=ptPan.y;
    		if(point.x || point.y) {
    			//Pan image --
				m_xpan-=point.x/m_xscale;
				m_ypan-=point.y/m_xscale;
	    		ptPan.x+=point.x;
	    		ptPan.y+=point.y;
				CPrjDoc::PlotPageGrid(-3);
	    		bPanValidated=FALSE;
			}
		}
		return;
	}

	CPrjDoc::GetCoordinate(dpCoordinate,point,TRUE);
	if(!m_bTracking) {
		TrackMouse(m_hWnd);
		m_bTracking=TRUE;
	}
}

BOOL CPageView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: Add your message handler code here and/or call default
	
	if(this==pWnd && nHitTest==HTCLIENT && m_bCursorInBmp) {
           ::SetCursor(pPV->m_hCursorBmp);
	       return TRUE;
	}
	return CPanelView::OnSetCursor(pWnd, nHitTest, message);
}

void CPageView::OnUpdateUTM(CCmdUI* pCmdUI)
{
	BOOL bEnable=m_bCursorInBmp;
    pCmdUI->Enable(bEnable);
	if(bEnable) {
		CString s;
		if(pCmdUI->m_nID==ID_INDICATOR_SEL) {
			CPrjDoc::m_pReviewNode->RefZoneStr(s);
		}
		else {
			GetCoordinateFormat(s,dpCoordinate,pPV->m_bProfile,LEN_ISFEET());
			pCmdUI->SetText(s);
		}
	}
}

LRESULT CPageView::OnMouseLeave(WPARAM wParam,LPARAM strText)
{
	m_bCursorInBmp=m_bTracking=FALSE;
	return FALSE;
}

BOOL CPageView::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		CWnd *pWnd=GetFocus();
		if(pWnd) {
			UINT id=pWnd->GetDlgCtrlID();
			if(id==IDC_ZOOMIN) {
				PostMessage(WM_COMMAND,IDC_ZOOMIN);
				return 1;
			}
		}
	}
	return CFormView::PreTranslateMessage(pMsg);
}
