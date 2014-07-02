// ScaleWnd.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "WallsMap.h"
#include "scalewnd.h"
#include <math.h>

IMPLEMENT_DYNAMIC(CScaleWnd, CStatic)
CScaleWnd::CScaleWnd() : m_hFont(0)
{
}

CScaleWnd::~CScaleWnd()
{
	if(m_hFont) ::DeleteObject(m_hFont);
}

BOOL CScaleWnd::Create(CWnd *pWnd)
{
	CString bmName;
	bmName.Format(_T("#%d"),IDB_SCALEBAR);
	if(!CStatic::Create(bmName,WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_CENTERIMAGE,CRect(0,0,0,0),pWnd)) return FALSE;
	CLogFont lf;
	char buf[48];
	strcpy(buf,"-9,0,0,0,700,0,0,0,0,3,2,1,34,\"Arial\"");
	lf.GetFontFromStr(buf);
	m_hFont=lf.Create();
	return TRUE;
}

BEGIN_MESSAGE_MAP(CScaleWnd, CStatic)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CScaleWnd message handlers


BOOL CScaleWnd::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	//return CStatic::OnEraseBkgnd(pDC);
	return TRUE;
}

void CScaleWnd::Draw(double mPerPx,bool bFeet)
{
	if(bFeet) {
		mPerPx*=(1/0.3048);
	}

	if((long)(mPerPx*BAR_MAXWIDTH)<1) {
		//don't draw bar --
		m_width=0;
		SetWindowPos(NULL,0,0,0,0,
			SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_HIDEWINDOW);
		return;
	}

	CDC dc;
	dc.CreateCompatibleDC(NULL);
	HBITMAP hbmOld = (HBITMAP)::SelectObject(dc.m_hDC,GetBitmap());
	HFONT hFontOld=(HFONT)::SelectObject(dc.m_hDC,m_hFont);
	HBRUSH hbrOld = (HBRUSH)::SelectObject(dc.m_hDC,::GetStockObject(LTGRAY_BRUSH));
	VERIFY(dc.PatBlt(0,0,BAR_WIDTH,BAR_HEIGHT,PATCOPY));

	m_width=cro_DrawScaleBar(&dc,mPerPx,BAR_WIDTH,bFeet);

	VERIFY(::SelectObject(dc.m_hDC,hbrOld));
	VERIFY(::SelectObject(dc.m_hDC,hFontOld));
	VERIFY(::SelectObject(dc.m_hDC,hbmOld));

	Invalidate(); //otherwise remnant left when bar is enlarged!
}


static bool bImage=false;
void CScaleWnd::CopyScaleBar(CDIBWrapper &dib,double mPerPx,double fRatio,bool bFeet)
{
	if(bFeet) {
		mPerPx*=(1/0.3048);
	}

	int barht=(int)(fRatio*BAR_HEIGHT+0.5);
	int barln=(int)(fRatio*BAR_WIDTH+0.5);

	CDIBWrapper dibBar;
	if(!dibBar.Init(barln,barht)) return;
	dibBar.Fill((HBRUSH)::GetStockObject(LTGRAY_BRUSH));

	CDC *pdcBar=dibBar.GetMemoryDC();
	if(pdcBar) {
		char buf[64];
		sprintf(buf,"-%u,0,0,0,700,0,0,0,0,3,2,1,34,\"Arial\"",(int)(9*fRatio+0.5));
		CLogFont lf;
		lf.GetFontFromStr(buf);
		HFONT hFontOld=(HFONT)::SelectObject(pdcBar->m_hDC,lf.Create());
		if(hFontOld) {

			bImage=true;
			barln=cro_DrawScaleBar(pdcBar,mPerPx,barln,bFeet);
			bImage=false;

			::BitBlt(dib.GetMemoryDC()->m_hDC,0,dib.GetHeight()-barht,barln,barht,pdcBar->m_hDC,0,0,SRCCOPY);
			::DeleteObject(::SelectObject(pdcBar->m_hDC,hFontOld));
		}
	}
}

static void __inline d2d_DL(ID2D1RenderTarget *prt, float x1, float y1, float x2, float y2, ID2D1Brush *pbr, float w)
{
	prt->DrawLine(CD2DPointF(x1, y1), CD2DPointF(x2, y2), pbr, w);
}

int CScaleWnd::cro_DrawScaleBar(CDC *pDC,double mPerPx,long barlen,bool bFeet)
{
	// bFeet == feet units?
	// mPerPx == bFeet?(ft per pixel):(meters per pixel)

	float bm_ratio=(float)barlen/BAR_WIDTH;
	float bar_maxw=bm_ratio*BAR_MAXWIDTH;
	long meters=(long)(mPerPx*bar_maxw); //Max unit length of scale bar
	bool bMiles=false;
	if(bFeet) {
		if(meters>=5280) {
			bMiles = true;
			mPerPx *= 1000.0 / 5280; // == (miles/1000) per pixel
			meters = (long)(mPerPx*bar_maxw);
		}
	}

	int p = 1; //power of 10 multiplier
	if (meters>10) {
		p = (int)pow(10.0, (int)log10(mPerPx*bar_maxw));
		meters /= p;
	}
	if (meters>5) meters = 5;
	meters *= p; //final bar length in meters or (miles/1000)
	barlen = (long)(meters / mPerPx + 0.5); //final bar length in pixels
	float w = bm_ratio*BAR_MARGIN;
	if (!bImage) w += (bar_maxw - barlen) / 2;

	float lw = (float)(int)(bm_ratio * 2 + 0.5f);

	ID2D1DCRenderTarget *pRT = NULL;
	ID2D1SolidColorBrush *pBR = NULL;
	CRect brect;
	pDC->GetClipBox(&brect);
	if (0 > d2d_pFactory->CreateDCRenderTarget(&d2d_props, &pRT) || 0 > pRT->BindDC(pDC->m_hDC, &brect) ||
		0 > pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBR))
	{
		SafeRelease(&pRT);
		ASSERT(0);
		return 0;
	}
	pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	pRT->BeginDraw();
	d2d_DL(pRT, w, bm_ratio*13.5f, w, bm_ratio*19.5f,pBR,lw);
	d2d_DL(pRT, w, bm_ratio*19.5f, w + barlen, bm_ratio*19.5f, pBR, lw);
	d2d_DL(pRT, w + barlen, bm_ratio*19.5f, w + barlen, bm_ratio*13.5f, pBR, lw);

	if (meters>1 && meters != 1000) {
		lw=(float)(int)(bm_ratio + 0.5f);
		p = meters / p;
		if (p == 1) p = 10;
		//draw p-1 intermediate ticks --
		float delta = barlen / (float)p;
		for (int i = 1; i<p; i++) {
			int m = (int)(i*delta + 0.5f);
			d2d_DL(pRT, w + m, bm_ratio*16.5f, w + m, bm_ratio*19.5f, pBR, lw);
		}
	}

	VERIFY(0>=pRT->EndDraw());
	pBR->Release();
	pRT->Release();

	CString label;
	if(meters>=1000 && (!bFeet || bMiles)) {
		ASSERT((meters%1000)==0);
		label.Format("%u %s",meters/1000,bFeet?"mi":"km");
	}
	else {
		label.Format("%u %s",meters,bFeet?"ft":"m");
	}

	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextAlign(TA_NOUPDATECP|TA_BOTTOM|TA_RIGHT);
	VERIFY(pDC->TextOut((int)(w+bm_ratio),(int)((bm_ratio*13.5)+0.5),"0",1));
	VERIFY(pDC->TextOut((int)(w+barlen+bm_ratio*4),(int)((bm_ratio*13.5)+0.5),label));

	return barlen+(int)(2*BAR_MARGIN*bm_ratio+0.5);
}

/*
int CScaleWnd::DrawScaleBar(HBITMAP hbm,HFONT hFont,double mPerPx,bool bFeet)
{
	CDC dc;
	dc.CreateCompatibleDC(NULL);
	HBITMAP hbmOld = (HBITMAP)::SelectObject(dc.m_hDC,hbm);
	HBRUSH hbrOld = (HBRUSH)::SelectObject(dc.m_hDC,::GetStockObject(LTGRAY_BRUSH));
	//HBRUSH hbrOld = (HBRUSH)::SelectObject(dc.m_hDC,::GetStockObject(WHITE_BRUSH));
	HFONT hFontOld=(HFONT)::SelectObject(dc.m_hDC,hFont);

	VERIFY(dc.PatBlt(0,0,BAR_WIDTH,BAR_HEIGHT,PATCOPY));
	//Invalidate(); //otherwise remnant left when bar is inlarged!

	// bFeet == feet units?
	// mPerPx == bFeet?(ft per pixel):(meters per pixel)

	long meters=(long)(mPerPx*BAR_MAXWIDTH); //Max unit length of scale bar
	bool bMiles=false;
	if(bFeet) {
		if(meters>=5280) {
			bMiles=true;
			mPerPx*=1000.0/5280; // == (miles/1000) per pixel
			meters=(long)(mPerPx*BAR_MAXWIDTH);
		}
	}

	int p=1; //power of 10 multiplier
	if(meters>10) {
		p=(int)pow(10.0,(int)log10(mPerPx*BAR_MAXWIDTH));
		meters/=p;
	}
	if(meters>5) meters=5;

	meters*=p; //final bar length in meters or (miles/1000)

	int barlen=(long)(meters/mPerPx+0.5); //final bar length in pixels

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextAlign(TA_NOUPDATECP|TA_BOTTOM|TA_RIGHT);

	int w=(BAR_MAXWIDTH-barlen)/2+BAR_MARGIN;

	dc.MoveTo(w,14);
	dc.LineTo(w,20);
	dc.LineTo(w+barlen,20);
	dc.LineTo(w+barlen,14-1);
	dc.MoveTo(w+1,14);
	dc.LineTo(w+1,20-1);
	dc.LineTo(w-1+barlen,20-1);
	dc.LineTo(w-1+barlen,14-1);

	CString label;
	if(meters>=1000 && (!bFeet || bMiles)) {
		ASSERT((meters%1000)==0);
		label.Format("%u %s",meters/1000,bFeet?"mi":"km");
	}
	else {
		label.Format("%u %s",meters,bFeet?"ft":"m");
	}
	VERIFY(dc.TextOut(w+3,14-1,"0",1));
	VERIFY(dc.TextOut(w+barlen+5,14-1,label));

	if(meters>1 && meters!=1000) {
		p=meters/p;
		if(p==1) p=10;
		//draw p-1 intermediate ticks --
		double delta=barlen/(double)p;
		for(int i=1;i<p;i++) {
			meters=(int)(i*delta+0.5);
			dc.MoveTo(w+meters,16);
			dc.LineTo(w+meters,20);
		}
	}

	VERIFY(::SelectObject(dc.m_hDC,hFontOld));
	VERIFY(::SelectObject(dc.m_hDC,hbrOld));
	VERIFY(::SelectObject(dc.m_hDC,hbmOld));

	return barlen;
}
*/

