// ScaleWnd.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PrjFont.h"
//#include "DibWrapper.h"
#include "scalewnd.h"
#include <math.h>

IMPLEMENT_DYNAMIC(CScaleWnd, CStatic)

// CScaleWnd
#define BAR_MARGIN 10
#define BAR_WIDTH 320
#define BAR_NAWIDTH 24

#define BAR_MAXWIDTH (BAR_WIDTH-2*BAR_MARGIN)
#define BAR_TOTALWIDTH (BAR_NAWIDTH+BAR_WIDTH)
#define AA_SCALE 4

CScaleWnd::~CScaleWnd()
{
	if(m_hFont) ::DeleteObject(m_hFont);
	if(m_hbm)  ::DeleteObject(m_hbm);
	if(m_hbmAA) ::DeleteObject(m_hbmAA);
}

BOOL CScaleWnd::Create(CWnd *pWnd)
{
	//CString bmName;
	//bmName.Format(_T("#%d"),IDB_SCALEBAR1);
	if(!CStatic::Create(NULL, WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_CENTERIMAGE,CRect(0,0,0,0),pWnd)) {
	//if(!(m_hWnd=CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, "STATIC","", WS_POPUP | WS_VISIBLE ,0, 0, 300, 24, NULL, NULL, AfxGetInstanceHandle(), NULL))) {
		ASSERT(_TraceLastError());
		return FALSE;
	}
	VERIFY(m_hbm=::CreateCompatibleBitmap(::GetDC(NULL), BAR_TOTALWIDTH, BAR_HEIGHT));
	if(!m_hbm) return FALSE;
	SetBitmap(m_hbm);

#ifdef _DEBUG
	BITMAP bm = {0};
	GetObject(GetBitmap(), sizeof(bm), &bm);
	ASSERT(BAR_TOTALWIDTH==bm.bmWidth);
	ASSERT(BAR_HEIGHT==bm.bmHeight);
#endif
	CLogFont lf;
	char buf[48];
	strcpy(buf,"-9,0,0,0,700,0,0,0,0,3,2,1,34,\"Arial\"");
	lf.GetFontFromStr(buf);
	VERIFY(m_hFont=::CreateFontIndirect(&lf));
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

void CScaleWnd::DrawNorthArrow(HDC hdc,double fView)
{
//Create at4x size --

	HPEN hOldPen = (HPEN)::GetCurrentObject(hdc, OBJ_PEN);
	HBRUSH hOldBrush = (HBRUSH)::GetCurrentObject(hdc, OBJ_BRUSH);
	HPEN hGrayPen  = ::CreatePen(PS_SOLID, AA_SCALE*1, RGB(0x80, 0x80, 0x80));
	HPEN hBlackPen = ::CreatePen(PS_SOLID, AA_SCALE*1, RGB(0x00, 0x00, 0x00));
	HBRUSH hLtBrush = ::CreateSolidBrush(0x00DDDDDD);

	int radius=AA_SCALE * 10;
	int cx=AA_SCALE * (BAR_HEIGHT/2); //BAR_HEIGHT==22
	int cy=AA_SCALE * (BAR_HEIGHT/2+1);
	int arrowx=cx-(int)(radius*sin(fView*U_DEGREES)+0.5);
	int arrowy=cy-(int)(radius*cos(fView*U_DEGREES)+0.5);

	::SelectObject(hdc, hGrayPen);
	::SelectObject(hdc, hLtBrush); // (HBRUSH)GetStockObject(WHITE_BRUSH));
	::Ellipse(hdc, cx-radius, cy-radius, cx+radius, cy+radius);
	::SelectObject(hdc, (HBRUSH)GetStockObject(HOLLOW_BRUSH));
	::Ellipse(hdc, cx-AA_SCALE, cy-AA_SCALE, cx+AA_SCALE, cy+AA_SCALE);
	::MoveToEx(hdc, cx, cy, NULL);
	::LineTo(hdc, arrowx, arrowy);

	::SelectObject(hdc, hBlackPen);
	::Ellipse(hdc, arrowx-AA_SCALE, arrowy-AA_SCALE, arrowx+AA_SCALE, arrowy+AA_SCALE);

	::SelectObject(hdc, hOldPen);
	::SelectObject(hdc, hOldBrush);
	::DeleteObject(hGrayPen);
	::DeleteObject(hBlackPen);
	::DeleteObject(hLtBrush);
}

BOOL CScaleWnd::CreateAAImage(double fView)
{
	if(m_hbmAA) ::DeleteObject(m_hbmAA);
	HDC hDC = ::GetDC(NULL);
	HBITMAP hTempBitmap = ::CreateCompatibleBitmap(hDC, AA_SCALE*BAR_HEIGHT, AA_SCALE*BAR_HEIGHT);
	if(!hTempBitmap) {
		::ReleaseDC(NULL, hDC);
		return FALSE;
	}
	m_hbmAA = ::CreateCompatibleBitmap(hDC, BAR_HEIGHT, BAR_HEIGHT);
	if(!m_hbmAA) {
		::DeleteObject(hTempBitmap);
		::ReleaseDC(NULL, hDC);
		return FALSE;
	}

	// Create anti-alias DC and bitmap
	m_hdcAA = ::CreateCompatibleDC(hDC);
	m_hbmAAOld = (HBITMAP)::SelectObject(m_hdcAA, m_hbmAA);

	// Calculate memory requested
	DWORD dwMemory = (AA_SCALE*BAR_HEIGHT) * (AA_SCALE*BAR_HEIGHT) * 4;

	// Create temporary DC and large bitmap
	HDC hTempDC = ::CreateCompatibleDC(hDC);
	HBITMAP hOldTempBitmap = (HBITMAP)::SelectObject(hTempDC, hTempBitmap);
	HBRUSH hOldTempBrush = (HBRUSH) ::SelectObject(hTempDC, (HBRUSH)::GetStockObject(LTGRAY_BRUSH));
	::PatBlt(hTempDC,0, 0, (AA_SCALE*BAR_HEIGHT), (AA_SCALE*BAR_HEIGHT), PATCOPY);

	::ReleaseDC(NULL, hDC);

	// Create drawing
	DrawNorthArrow(hTempDC,fView);

	// Copy temporary DC to anti-aliazed DC
	int oldStretchBltMode = ::SetStretchBltMode(m_hdcAA, HALFTONE);
	::StretchBlt(m_hdcAA, 0, 0, BAR_HEIGHT, BAR_HEIGHT, hTempDC, 0, 0, AA_SCALE*BAR_HEIGHT, AA_SCALE*BAR_HEIGHT, SRCCOPY);
	::SetStretchBltMode(m_hdcAA, oldStretchBltMode);

	// Destroy temporary DC and bitmap
	if(hTempDC)
	{
		::SelectObject(hTempDC, hOldTempBitmap);
		::SelectObject(hTempDC, hOldTempBrush);
		::DeleteDC(hTempDC);
		::DeleteObject(hTempBitmap);
	}
	return TRUE;
}

int CScaleWnd::DrawScaleBar(HDC hdc,double mPerPx,double fView)
{
	long meters=(long)(mPerPx*BAR_MAXWIDTH); //Max unit length of scale bar
	bool bMiles=false;
	if(m_bFeet) {
		if(meters>=5280) {
			bMiles = true;
			mPerPx *= 1000.0 / 5280; // == (miles/1000) per pixel
			meters = (long)(mPerPx*BAR_MAXWIDTH);
		}
	}

	int p = 1; //power of 10 multiplier
	if(meters>10) {
		p = (int)pow(10.0, (int)log10(mPerPx*BAR_MAXWIDTH));
		meters /= p;
	}
	if(meters>5) meters = 5;
	meters *= p; //final bar length in meters or (miles/1000)
	long barlen = (long)(meters / mPerPx + 0.5); //final bar length in pixels
	long naw=(fView>=0)?BAR_NAWIDTH:0;
	long off=(BAR_TOTALWIDTH-barlen-2*BAR_MARGIN-naw)/2;
	//off=2*(off/2);
	long w = BAR_MARGIN + naw + off;

	::SetBkMode(hdc,TRANSPARENT);
	::SetTextAlign(hdc,TA_NOUPDATECP|TA_BOTTOM|TA_RIGHT);

	#define BAR_TOP 13

	::MoveToEx(hdc,w, BAR_TOP+1,NULL);
	::LineTo(hdc,w, BAR_TOP+7);
	::LineTo(hdc,w+barlen, BAR_TOP+7);
	::LineTo(hdc,w+barlen, BAR_TOP);
	::MoveToEx(hdc,w+1, BAR_TOP+1,NULL);
	::LineTo(hdc,w+1, BAR_TOP+6);
	::LineTo(hdc,w-1+barlen, BAR_TOP+6);
	::LineTo(hdc,w-1+barlen, BAR_TOP);

	CString label;
	if(meters>=1000 && (!m_bFeet || bMiles)) {
		ASSERT((meters%1000)==0);
		label.Format("%u %s", meters/1000, m_bFeet?"mi":"km");
	}
	else {
		label.Format("%u %s", meters, m_bFeet?"ft":"m");
	}
	VERIFY(::TextOut(hdc,w+3, BAR_TOP, "0", 1));
	VERIFY(::TextOut(hdc,w+barlen+5, BAR_TOP, label,label.GetLength()));

	if(meters>1 && meters!=1000) {
		p=meters/p;
		if(p==1) p=10;
		//draw p-1 intermediate ticks --
		double delta=barlen/(double)p;
		for(int i=1; i<p; i++) {
			meters=(int)(i*delta+0.5);
			::MoveToEx(hdc,w+meters, BAR_TOP+3,NULL);
			::LineTo(hdc,w+meters, BAR_TOP+7);
		}
	}

	if(naw) {
		if(!m_hbmAA || m_fView!=fView) {
			//Create bitmap with antialiased circled north arrow
			if(m_hbmAA) {
				::DeleteObject(m_hbmAA);
				m_hbmAA=NULL;
			}
			if(!CreateAAImage(fView)) return 0;
			m_fView=fView;
		}
		else {
			ASSERT(m_hdcAA==NULL);
			HDC hDC=::GetDC(NULL);
			m_hdcAA = ::CreateCompatibleDC(hDC);
			m_hbmAAOld = (HBITMAP)::SelectObject(m_hdcAA, m_hbmAA);
			::ReleaseDC(NULL,hDC);
		}
		//Copy arrow bitmap to bar --
		VERIFY(BitBlt(hdc,w-BAR_HEIGHT-BAR_MARGIN/2, 0, BAR_HEIGHT, BAR_HEIGHT, m_hdcAA, 0, 0, SRCCOPY));
		::SelectObject(m_hdcAA, m_hbmAAOld);
		::DeleteDC(m_hdcAA);
		m_hdcAA=NULL;
	}

	return barlen+2*BAR_MARGIN+naw; //width of window
}

void CScaleWnd::Draw(double mPerPx, double fView)
{
	if(m_bFeet) {
		mPerPx*=(1/0.3048);
	}

	if((long)(mPerPx*BAR_MAXWIDTH)<1) {
		//don't draw bar --
		m_width=0;
		SetWindowPos(NULL, 0, 0, 0, 0,
			SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_HIDEWINDOW);
		return;
	}

	CDC dc;
	dc.CreateCompatibleDC(NULL);

	HBITMAP hbmOld = (HBITMAP)::SelectObject(dc.m_hDC, m_hbm);
	HFONT hFontOld=(HFONT)::SelectObject(dc.m_hDC, m_hFont);
	HBRUSH hbrOld = (HBRUSH)::SelectObject(dc.m_hDC, ::GetStockObject(LTGRAY_BRUSH));
	VERIFY(dc.PatBlt(0, 0, BAR_TOTALWIDTH, BAR_HEIGHT, PATCOPY));

	m_width=DrawScaleBar(dc.m_hDC, mPerPx, fView);

	VERIFY(::SelectObject(dc.m_hDC, hbrOld));
	VERIFY(::SelectObject(dc.m_hDC, hFontOld));
	VERIFY(::SelectObject(dc.m_hDC, hbmOld));

	Invalidate(); //otherwise remnant left when bar is enlarged!
}
