
// D2D-SymView.cpp : implementation of the CD2DSymView class
//

#include "stdafx.h"
#include "MainFrm.h"
#include "D2D-SymDoc.h"
#include "D2D-SymView.h"
#include "AbtBox.h"
#define _USE_TIMER
#include <GetElapsedSecs.h>
//#include "MemUsageDlg.h"
#include "PixelData.h"
#include <d2d1helper.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static CPixelData pxd;

// CD2DSymView
ID2D1Factory *CD2DSymView::m_pFactory = NULL;

IMPLEMENT_DYNCREATE(CD2DSymView, CView)

BEGIN_MESSAGE_MAP(CD2DSymView, CView)
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_WM_ERASEBKGND()
#ifdef _USE_FPS
	ON_WM_TIMER()
#endif

#ifdef _USE_CAIRO
	ON_COMMAND(ID_DRAW_CAIRO, OnDrawCairo)
#endif
	ON_UPDATE_COMMAND_UI(ID_DRAW_CAIRO, OnUpdateDrawCairo)

	ON_COMMAND(ID_DRAW_D2D, OnDrawD2D)
	ON_UPDATE_COMMAND_UI(ID_DRAW_D2D, OnUpdateDrawD2D)

	ON_COMMAND(ID_DRAW_GDI, OnDrawGDI)
	ON_UPDATE_COMMAND_UI(ID_DRAW_GDI, OnUpdateDrawGDI)
	ON_COMMAND(ID_DRAW_ANTIALIAS, OnDrawAntialias)
	ON_UPDATE_COMMAND_UI(ID_DRAW_ANTIALIAS, OnUpdateDrawAntialias)

	ON_COMMAND(ID_SYMBOLOGY, OnSymbology)
	ON_WM_SIZE()
	ON_COMMAND(ID_D2DSW, OnD2dsw)
	ON_UPDATE_COMMAND_UI(ID_D2DSW, OnUpdateD2dsw)
END_MESSAGE_MAP()

// CD2DSymView construction/destruction

CD2DSymView::CD2DSymView()
{
	pxd.iSymTyp = SYM_TRIANGLE;
	pxd.iSymSize = _START_SYMSIZE;
	pxd.fLineSize = 2.0;
	pxd.fVecSize = _START_VECSIZE;
	pxd.clrSym = RGB(64, 224, 208);
	pxd.clrLine = RGB(0, 0, 0);
	pxd.clrVec = RGB(255, 255, 255);
	pxd.clrBack = RGB(128, 128, 128);
	pxd.iOpacitySym = 100;
	pxd.iOpacityVec = 100;
	pxd.bAntialias = _START_ANTIALIASED;
	pxd.bDotCenter = FALSE;
	pxd.bBackSym = _START_BACKSYM;

	m_hbm = 0;

#ifdef _USE_FPS
	m_nFrames = m_nTimerCount = 0;
	m_bUpdating = false;
#endif

	//only one of following can be true, gdi used if both false
	m_bD2D = _START_D2D;
	m_bCairo = _START_CAIRO;
	ASSERT(!m_bD2D || !m_bCairo);

	m_bD2Dsw = false;
	m_pLineBrush = NULL;
	m_pStrokeStyleSolidRoundwRoundCap = NULL;
}

CD2DSymView::~CD2DSymView()
{
	SafeRelease(&m_pRT);
	SafeRelease(&m_pLineBrush);
	SafeRelease(&m_pStrokeStyleSolidRoundwRoundCap);
}

// CD2DSymView diagnostics

#ifdef _DEBUG
void CD2DSymView::AssertValid() const
{
	CView::AssertValid();
}

void CD2DSymView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CD2DSymDoc* CD2DSymView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CD2DSymDoc)));
	return (CD2DSymDoc*)m_pDocument;
}
#endif //_DEBUG

void CD2DSymView::FixModeStr()
{
	m_csMode.Format("%s", m_bCairo ? "Cairo" : (m_bD2D ? (m_bD2Dsw ? "D2D-SW" : "D2D-HW") : "GDI"));
	GetMF()->SetFPS(m_csMode, 0);
}

BOOL CD2DSymView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CD2DSymView drawing

void CD2DSymView::DrawDataGDI(CDC *dc, CRect &rect)
{
	CRect rc(rect);
	int bw = rect.Width() / 10;
	int bh = rect.Height() / 10;
	int w = 8 * bw;
	int h = 8 * bh;

	HPEN pen = (HPEN)::SelectObject(dc->m_hDC, ::GetStockObject(WHITE_PEN));

	VECPOINTF_IT it = m_vPT.begin();

	dc->MoveTo(bw + (int)(it->x*w + 0.5), bh + (int)(it->y*h + 0.5));

	for (it++; it != m_vPT.end(); it++) { //100 it = 33.8 fps
		dc->LineTo(bw + (int)(it->x*w + 0.5), bh + (int)(it->y*h + 0.5));
	}

	::SelectObject(dc->m_hDC, pen);
}

static void get_sym_offsets(double &symOffX, double &symOffY, int typ)
{
	switch (typ) {
	case SYM_SQUARE:
	case SYM_CIRCLE:
		symOffX = symOffY = 0.5;
		break;
	case SYM_TRIANGLE:
	{
		symOffX = 0.5;
		symOffY = 0.7113248654;  //1-tan(30)/2
	}
	break;
	}
	return;
}

#ifdef _USE_CAIRO

static cairo_surface_t *make_symbol(int typ, int sz, double lw, COLORREF sClr, COLORREF lClr, BOOL bAntialias, BOOL dotCtr)
{
	//note: lw = line width is relative to symbol size (pixels)
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, sz, sz);
	cairo_t *cr = cairo_create(surface);
	if (!bAntialias)
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

	cairo_scale(cr, sz, sz);

	lw /= sz;

	switch (typ) {
	case SYM_SQUARE:
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_rectangle(cr, lw / 2, lw / 2, 1 - lw, 1 - lw);
		break;

	case SYM_CIRCLE:
		cairo_arc(cr, 0.5, 0.5, 0.5 - lw / 2, 0, M_PI * 2);
		break;

	case SYM_TRIANGLE:
	{
		cairo_move_to(cr, lw*0.8660254038, 1 - lw / 2); //lw/(2*tan(30)
		cairo_line_to(cr, 0.5, 0.1330745962 + lw);  //1-sin(60) + lw
		cairo_line_to(cr, 1 - lw * 0.8660254038, 1 - lw / 2);
		cairo_close_path(cr);
	}
	break;
	}

	cairo_set_source_rgba(cr, GetR_f(sClr), GetG_f(sClr), GetB_f(sClr), 1); //first perform fill
	if (lw > 0.0) {
		cairo_fill_preserve(cr);
		cairo_set_source_rgba(cr, GetR_f(lClr), GetG_f(lClr), GetB_f(lClr), 1);
		cairo_set_line_width(cr, lw);
		cairo_stroke(cr);
	}
	else cairo_fill(cr);

	if (dotCtr) {
		double x0, y0;
		get_sym_offsets(x0, y0, typ);

		cairo_set_line_width(cr, (dotCtr == 1) ? (2.0 / sz) : (2.0 / 20));
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_antialias(cr, bAntialias ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
		//cairo_set_antialias(cr,(bAntialias && (sz&1))?CAIRO_ANTIALIAS_DEFAULT:CAIRO_ANTIALIAS_NONE); //no difference
		cairo_move_to(cr, x0, y0);
		cairo_line_to(cr, x0, y0);
		cairo_stroke(cr);
	}
	cairo_destroy(cr);
	return surface;
}

void CD2DSymView::DrawDataCairo(CDC* pDC)
{

	CRect rc;
	GetClientRect(rc);

	float bw = (float)(rc.Width() / 10); //border width
	float bh = (float)(rc.Height() / 10);
	float w = 8 * bw;
	float h = 8 * bh;

#if CAIRO_BLIT
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rc.right, rc.bottom);
#else
	cairo_surface_t *surface = cairo_win32_surface_create(pDC->GetSafeHdc());
#endif

	cairo_t *cr = cairo_create(surface);
	cairo_surface_t *symbol = NULL;

	cairo_set_antialias(cr, pxd.bAntialias ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);

	int iSize = pxd.iSymSize;

	//draw large version of symbol at center --
	if (iSize > 0 && pxd.bBackSym) {
		float fMult = 20 * 6.0f / iSize;

		symbol = make_symbol(pxd.iSymTyp, (int)(iSize*fMult * 2), pxd.fLineSize*fMult * 2,
			pxd.clrSym, pxd.clrLine, pxd.bAntialias, 2 * pxd.bDotCenter); //scale normal dot center by iSize/6

		cairo_set_source_surface(cr, symbol, (int)(bw + w / 2 - fMult * iSize), (int)(bh + h / 2 - fMult * iSize));
		cairo_paint_with_alpha(cr, 0.5);
		cairo_surface_destroy(symbol);

		//draw symbol bounding box using default pen --
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_rectangle(cr, (int)(bw + w / 2 - iSize * fMult), (int)(bh + h / 2 - iSize * fMult),
			(int)(iSize*fMult * 2), (int)(iSize*fMult * 2));
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
		if (pxd.bAntialias) cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
	}

	if (pxd.fVecSize > 0.0 && pxd.iOpacityVec > 0.0) {
		if (rc != m_CRsc || m_vPT.size() != m_vPTsc.size()) {
			m_vPTsc.clear();
			for (VECPOINTF_IT it = m_vPT.begin(); it != m_vPT.end(); it++) {
				m_vPTsc.push_back(CD2DPointF(bw + it->x*w, bh + it->y*h));
			}
			m_CRsc = rc;
		}
		cairo_set_source_rgba(cr, GetR_f(pxd.clrVec), GetG_f(pxd.clrVec), GetB_f(pxd.clrVec), pxd.iOpacityVec / 100.0);
		cairo_set_line_width(cr, pxd.fVecSize);
#if _USE_STROKESTYLE
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
#endif

#if _USE_CAIRO_POLY
		VEC2DPOINTF_IT it = m_vPTsc.begin();
		cairo_move_to(cr, it->x, it->y);

		for (it++; it != m_vPTsc.end(); it++) {
			cairo_line_to(cr, it->x, it->y);
		}
		cairo_stroke(cr);
#else
		//Much slower --
		VEC2DPOINTF_IT it0 = m_vPTsc.begin();
		for (VEC2DPOINTF_IT it = it0 + 1; it != m_vPTsc.end(); it++) {
			cairo_move_to(cr, it0->x, it0->y);
			cairo_line_to(cr, it->x, it->y);
			cairo_stroke(cr);
			it0 = it;
		}
#endif
	}

	int i = 0;

	if (iSize > 1 && pxd.iOpacitySym > 0.0) {
		double symOffX, symOffY;

		symbol = make_symbol(pxd.iSymTyp, iSize, pxd.fLineSize, pxd.clrSym, pxd.clrLine, pxd.bAntialias, pxd.bDotCenter);

		i = cairo_surface_get_reference_count(symbol);

		get_sym_offsets(symOffX, symOffY, pxd.iSymTyp);
		//place symbols at nearest pixel boundary for
		//best results when using square symbols with antialiasing with Cairo --
		symOffX = iSize * symOffX - bw - 0.5;
		symOffY = iSize * symOffY - bh - 0.5;
		double opacity = pxd.iOpacitySym / 100.0;

		for (VECPOINTF_IT it = m_vPT.begin(); it != m_vPT.end(); it++) {
			cairo_set_source_surface(cr, symbol, (int)(it->x*w - symOffX), (int)(it->y*h - symOffY));
			cairo_paint_with_alpha(cr, opacity);
		}
		cairo_surface_destroy(symbol);
	}

	cairo_destroy(cr);

#if CAIRO_BLIT
	cairo_surface_t *surf32 = cairo_win32_surface_create(pDC->GetSafeHdc());
	cairo_t *cr32 = cairo_create(surf32);

	cairo_set_source_surface(cr32, surface, 0, 0);
	cairo_paint(cr32);
	cairo_destroy(cr32);
	cairo_surface_destroy(surf32);
#endif

	cairo_surface_destroy(surface);
}
#endif

//===================================================================================
bool CD2DSymView::InitializeD2D()
{
	if (m_pFactory) return true;
	if (!(m_pFactory = AfxGetD2DState()->GetDirect2dFactory()))
		return false;

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		m_bD2Dsw ? D2D1_RENDER_TARGET_TYPE_SOFTWARE : D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(
			DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE_IGNORE),
		0,
		0,
		D2D1_RENDER_TARGET_USAGE_NONE,
		D2D1_FEATURE_LEVEL_DEFAULT
	);

	if (!SUCCEEDED(m_pFactory->CreateDCRenderTarget(&props, &m_pRT))) return false;

	if (!SUCCEEDED(m_pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
		&m_pLineBrush))) return FALSE;

#if _USE_STROKESTYLE
	if (!SUCCEEDED(m_pFactory->CreateStrokeStyle(
		D2D1::StrokeStyleProperties(
			D2D1_CAP_STYLE_ROUND, // default is flat, which would create broken line joins when using DrawLine()
			D2D1_CAP_STYLE_ROUND,
			D2D1_CAP_STYLE_FLAT,  // not relevant since solid
			D2D1_LINE_JOIN_ROUND, // not relevant if using DrawLine()
			2.0f,
			D2D1_DASH_STYLE_SOLID,
			0.0f),
		NULL,
		0,
		&m_pStrokeStyleSolidRoundwRoundCap))) return false;
#endif
	return true;
}

static ID2D1Bitmap *D2D_make_symbol(ID2D1RenderTarget *prt, int typ, float sz, double lw, COLORREF sClr, COLORREF lClr,
	BOOL bAntialias, ID2D1StrokeStyle *pDotStyle)
{
	D2D1_SIZE_F szF = D2D1::SizeF(sz, sz);

	ID2D1BitmapRenderTarget *pRT;

	if (prt->CreateCompatibleRenderTarget(&szF, NULL, NULL, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &pRT) < 0) {
		return NULL;
	}

	ID2D1SolidColorBrush *pBR;
	if (0 > pRT->CreateSolidColorBrush(RGBtoD2D(sClr), &pBR)) {
		pRT->Release();
		return NULL;
	}
	pRT->SetAntialiasMode((bAntialias && (typ != SYM_SQUARE)) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

	pRT->BeginDraw();
	float s = (float)sz;
	pRT->SetTransform(D2D1::Matrix3x2F::Scale(s, s));
	float w = (float)lw / s; //don't scale line width

	pRT->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));

	switch (typ) {
	case SYM_SQUARE:
		pRT->FillRectangle(D2D1::RectF(0.f, 0.f, 1.f, 1.f), pBR);
		pBR->SetColor(RGBtoD2D(lClr));
		pRT->DrawRectangle(D2D1::RectF(w / 2, w / 2, 1 - w / 2, 1 - w / 2), pBR, w);
		break;

	case SYM_CIRCLE:
	{
		D2D1_ELLIPSE circle = D2D1::Ellipse(D2D1::Point2F(0.5f, 0.5f), 0.5f - w / 2, 0.5f - w / 2);
		pRT->FillEllipse(circle, pBR);
		pBR->SetColor(RGBtoD2D(lClr));
		pRT->DrawEllipse(circle, pBR, w);
	}
	break;

	case SYM_TRIANGLE:
	{
		ID2D1PathGeometry *pPath = NULL;
		if (CD2DSymView::m_pFactory->CreatePathGeometry(&pPath) < 0) break;

		ID2D1GeometrySink *pSink = NULL;
		if (pPath->Open(&pSink) < 0) {
			pPath->Release();
			break;
		}
		//pSink->SetFillMode(D2D1_FILL_MODE_ALTERNATE); //default, not relevant anyway

		pSink->BeginFigure(D2D1::Point2F(w*0.8660254038f, 1 - w / 2), D2D1_FIGURE_BEGIN_FILLED);
		pSink->AddLine(D2D1::Point2F(0.5, 0.1330745962f + w));
		pSink->AddLine(D2D1::Point2F(1 - w * 0.8660254038f, 1 - w / 2));
		pSink->EndFigure(D2D1_FIGURE_END_CLOSED);

		pSink->Close();
		pSink->Release();

		pRT->FillGeometry(pPath, pBR);
		pBR->SetColor(RGBtoD2D(lClr));
		pRT->DrawGeometry(pPath, pBR, w);
		pPath->Release();
	}
	break;
	}

	if (pDotStyle) {
		double x0, y0;
		get_sym_offsets(x0, y0, typ);
		CD2DPointF ctr(0.5f, (float)y0);
		pRT->DrawLine(ctr, ctr, pBR, 2 / s, pDotStyle);
	}

	pBR->Release();
	if (pRT->EndDraw() < 0) {
		ASSERT(0);
		pRT->Release();
		return NULL;
	}
	ID2D1Bitmap *pBM;
	if (pRT->GetBitmap(&pBM) < 0) pBM = NULL;
	pRT->Release();
	return pBM;
}

void CD2DSymView::DrawDataD2D(CDC &dc)
{
	if (!m_pFactory && !InitializeD2D()) {
		return;
	}

	CRect rc;
	GetClientRect(rc);
	float bw = (float)(rc.Width() / 10); //border width
	float bh = (float)(rc.Height() / 10);
	float w = 8 * bw;
	float h = 8 * bh;

	// Bind the DC to the DC render target.
	if (0 > m_pRT->BindDC(dc.m_hDC, rc))
		return;

	m_pRT->BeginDraw();

	m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());

	if (pxd.fVecSize > 0.0 && pxd.iOpacityVec > 0) {
		if (rc != m_CRsc || m_vPT.size() != m_vPTsc.size()) {
			m_vPTsc.clear();
			for (VECPOINTF_IT it = m_vPT.begin(); it != m_vPT.end(); it++) {
				m_vPTsc.push_back(CD2DPointF(bw + it->x*w, bh + it->y*h));
			}
			m_CRsc = rc;
		}
		float lw = (float)pxd.fVecSize;
		m_pLineBrush->SetColor(RGBAtoD2D(pxd.clrVec, pxd.iOpacityVec / 100.f));
		m_pRT->SetAntialiasMode(pxd.bAntialias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

#if _USE_D2D_POLY
		//much slower (depends on video driver) but overlaps are visible with brush opacity < 1
		ID2D1PathGeometry *pPath = NULL;
		VERIFY(0 <= m_pFactory->CreatePathGeometry(&pPath));

		ID2D1GeometrySink *pSink = NULL;

		if (0 <= pPath->Open(&pSink)) {
			//pSink->SetFillMode(D2D1_FILL_MODE_ALTERNATE); //dflt alternate, relevant for FillGeometry only

			VEC2DPOINTF_IT it = m_vPTsc.begin();
			pSink->BeginFigure(*it++, D2D1_FIGURE_BEGIN_HOLLOW); //2nd param relevant for this figure for FillGeometry
#if 0
			for (; it != m_vPTsc.end(); it++) {
				pSink->AddLine(*it);
			}
#else
			//No change is speed: 3.2s Vaio no sym 2000 vecs sz 1
			pSink->AddLines(&(*it), m_vPTsc.size() - 1);
#endif
			pSink->EndFigure(D2D1_FIGURE_END_OPEN); //so as to detect endpoints

			pSink->Close();
			pSink->Release();
			m_pRT->DrawGeometry(pPath, m_pLineBrush, lw, m_pStrokeStyleSolidRoundwRoundCap);
			//m_pRT->FillGeometry(pPath, m_pLineBrush);
		}
		SafeRelease(&pPath);

#else
		VEC2DPOINTF_IT it0 = m_vPTsc.begin();
		for (VEC2DPOINTF_IT it = it0 + 1; it != m_vPTsc.end(); it++) {
			m_pRT->DrawLine(*it0, *it, m_pLineBrush, lw, m_pStrokeStyleSolidRoundwRoundCap);
			it0 = it;
		}
#endif
	}

	if (pxd.iSymSize && pxd.iOpacitySym > 0) {
		//m_RT.Flush(); //would be slower
		float sz = (float)pxd.iSymSize;
		ID2D1Bitmap *pSymbol = D2D_make_symbol(m_pRT, pxd.iSymTyp, sz, pxd.fLineSize,
			pxd.clrSym, pxd.clrLine, pxd.bAntialias, pxd.bDotCenter ? m_pStrokeStyleSolidRoundwRoundCap : NULL);
		if (pSymbol) {
			double symOffX, symOffY;
			get_sym_offsets(symOffX, symOffY, pxd.iSymTyp);
			//place symbols at nearest pixel boundary for
			//best results when using square symbols
			symOffX = sz * symOffX - bw - 0.5;
			symOffY = sz * symOffY - bh - 0.5;

			m_pRT->SetAntialiasMode((pxd.bAntialias && (pxd.iSymTyp != SYM_SQUARE)) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

			float opacity = pxd.iOpacitySym / 100.0f;

			for (VECPOINTF_IT it = m_vPT.begin(); it != m_vPT.end(); it++) {
				m_pRT->SetTransform(D2D1::Matrix3x2F::Translation((float)(int)(it->x*w - symOffX), (float)(int)(it->y*h - symOffY)));
				m_pRT->DrawBitmap(pSymbol, D2D1::RectF(0.f, 0.f, sz, sz), opacity);
			}
		}
		SafeRelease(&pSymbol);
	}

	if (0 > m_pRT->EndDraw()) {
		SafeRelease(&m_pRT);
		ASSERT(0);
	}
}

void CD2DSymView::OnDraw(CDC* pDC)
{

#ifdef _USE_FPS
	if (!m_bUpdating && !m_nFrames) {
		m_nTimerCount = 0;
		START_TIMER();
	}
#endif

	CDC dc;
	HBITMAP hbmold;
	CRect bounds;

	GetClientRect(&bounds);

	if (!m_hbm || bounds.right != m_right || bounds.bottom != m_bottom) {
		if (m_hbm) ::DeleteObject(m_hbm);
		m_hbm = ::CreateCompatibleBitmap(pDC->GetSafeHdc(), m_right = bounds.right, m_bottom = bounds.bottom);
		ASSERT(m_hbm);
	}

	VERIFY(dc.CreateCompatibleDC(pDC));
	hbmold = (HBITMAP)::SelectObject(dc.GetSafeHdc(), m_hbm);
	ASSERT(hbmold);

	dc.FillSolidRect(&bounds, ::GetSysColor(COLOR_BTNSHADOW)); //clears the bitmap

	//Perform some initial GDI painting --
	{
		CRect rc(bounds);
		rc.InflateRect(-rc.Width() / 10, -rc.Height() / 10);
		CBrush cbr, *cbrold;
		cbr.CreateSolidBrush(pxd.clrBack);
		cbrold = dc.SelectObject(&cbr);
		HPEN pen = (HPEN)::SelectObject(dc.GetSafeHdc(), ::GetStockObject(BLACK_PEN));
		dc.Rectangle(rc);
		::SelectObject(dc.GetSafeHdc(), pen);
		dc.SelectObject(cbrold);
	}

	if (m_iVecCountIdx >= 0) {
#ifdef _USE_CAIRO
		if (m_bCairo) {
			DrawDataCairo(&dc);
		}
		else
#endif

			if (m_bD2D) {
				DrawDataD2D(dc);
			}
			else

#ifdef _GDI_DEFAULT
				DrawDataGDI(&dc, bounds);
#else
		{}
#endif
	}

	//Finally, update the screen --
	VERIFY(pDC->BitBlt(bounds.left, bounds.top, bounds.Width(), bounds.Height(),
		&dc, bounds.left, bounds.top, SRCCOPY));

	VERIFY(m_hbm == ::SelectObject(dc.GetSafeHdc(), hbmold));

#ifdef _USE_FPS
	m_nFrames++;
	if (!m_bUpdating && m_nTimerCount >= FRAME_LABEL_INTERVAL) {
		STOP_TIMER();
		GetMF()->SetFPS(m_csMode, (double)m_nFrames / TIMER_SECS());
		m_nFrames = m_nTimerCount = 0;
	}
#endif
}

#ifdef _USE_FPS
void CD2DSymView::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == 1) {
		m_nTimerCount++;
		Invalidate();
	}
}
#endif


BOOL CD2DSymView::DestroyWindow()
{
#ifdef _USE_FPS
	KillTimer(1);
#endif
	return CView::DestroyWindow();
}

BOOL CD2DSymView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

bool CD2DSymView::InitVecArray(int idx)
{
	m_vPT.swap(VECPOINTF());
	try {
		m_vPT.reserve(idx = vec_count[m_iVecCountIdx = idx]);
		srand((unsigned)time(NULL));
		while (idx--) {
			float x = (float)rand() / RAND_MAX;
			float y = (float)rand() / RAND_MAX;
			m_vPT.push_back(CPOINTF(x, y));
		}
	}
	catch (...) {
		AfxMessageBox("Failed to allocate m_vPT array");
		m_iVecCountIdx = -1;
		return false;
	}
	return true;
}

void CD2DSymView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	InitVecArray(VEC_COUNT_IDX);

	Start_fps_timer();
}

void CD2DSymView::UpdateSymbols(CSymDlg &dlg)
{
	if (m_bCairo != (dlg.m_bGDI == 1) || m_bD2D != (dlg.m_bGDI == 2)) {
		m_bCairo = dlg.m_bGDI == 1;
		m_bD2D = dlg.m_bGDI == 2;
		FixModeStr();
	}
	if (dlg.m_iVecCountIdx != m_iVecCountIdx) {
		InitVecArray(dlg.m_iVecCountIdx);
	}
	pxd.iSymTyp = dlg.m_iSymTyp;
	pxd.clrSym = dlg.m_BtnSym.GetColor();
	pxd.clrLine = dlg.m_BtnLine.GetColor();
	pxd.clrVec = dlg.m_BtnVec.GetColor();
	pxd.clrBack = dlg.m_BtnBack.GetColor();
	pxd.bDotCenter = dlg.m_bDotCenter;
	pxd.iSymSize = (dlg.m_iSymSize > 1) ? dlg.m_iSymSize : 0;
	pxd.fLineSize = dlg.m_fLineSize;
	pxd.fVecSize = dlg.m_fVecSize;
	pxd.iOpacitySym = dlg.m_iOpacitySym;
	pxd.iOpacityVec = dlg.m_iOpacityVec;
	pxd.bAntialias = dlg.m_bAntialias;
	pxd.bBackSym = dlg.m_bBackSym;
	Invalidate(0);
}

void CD2DSymView::OnSymbology()
{
	int iSavSym, iSavVec, iSavTyp, iSavSize, iSavGDI;
	COLORREF clrSavSym, clrSavLine, clrSavVec, clrSavBack;
	BOOL bSavDotCenter, bSavAntialias, bBackSym;
	double fSavLineSize, fSavVecSize;

	//should fix to use structure?

#ifdef _USE_FPS
	m_bUpdating = true;
	KillTimer(1);
#endif

	CSymDlg dlg(this);
	dlg.m_iVecCountIdx = m_iVecCountIdx;
	dlg.m_bGDI = iSavGDI = (m_bCairo ? 1 : (m_bD2D ? 2 : 0));
	dlg.m_iSymTyp = iSavTyp = pxd.iSymTyp;
	dlg.m_clrSym = clrSavSym = pxd.clrSym;
	dlg.m_clrLine = clrSavLine = pxd.clrLine;
	dlg.m_clrVec = clrSavVec = pxd.clrVec;
	dlg.m_clrBack = clrSavBack = pxd.clrBack;
	dlg.m_bDotCenter = bSavDotCenter = pxd.bDotCenter;
	dlg.m_iSymSize = iSavSize = pxd.iSymSize;
	dlg.m_iOpacitySym = iSavSym = pxd.iOpacitySym;
	dlg.m_iOpacityVec = iSavVec = pxd.iOpacityVec;
	dlg.m_bAntialias = bSavAntialias = pxd.bAntialias;
	dlg.m_fLineSize = fSavLineSize = pxd.fLineSize;
	dlg.m_fVecSize = fSavVecSize = pxd.fVecSize;
	dlg.m_bBackSym = bBackSym = pxd.bBackSym;

	if (IDOK == dlg.DoModal()) {
		UpdateSymbols(dlg);
	}
	else {
		m_bCairo = iSavGDI == 1;
		m_bD2D = iSavGDI == 2;
		pxd.iSymTyp = iSavTyp;
		pxd.iSymSize = iSavSize;
		pxd.iOpacitySym = iSavSym;
		pxd.iOpacityVec = iSavVec;
		pxd.clrSym = clrSavSym;
		pxd.clrLine = clrSavLine;
		pxd.clrVec = clrSavVec;
		pxd.clrBack = clrSavBack;
		pxd.bDotCenter = bSavDotCenter;
		pxd.bAntialias = bSavAntialias;
		pxd.bBackSym = bBackSym;
		pxd.fLineSize = fSavLineSize;
		pxd.fVecSize = fSavVecSize;
	}
#ifdef _USE_FPS
	Start_fps_timer();
#endif
}

void CD2DSymView::UpdateOpacity(UINT id, int iPos)
{
	if (id == IDC_ST_OPACITY_SYM) pxd.iOpacitySym = iPos;
	else pxd.iOpacityVec = iPos;
	Invalidate(0);
}

void CD2DSymView::UpdateSymSize(int i)
{
	if (pxd.iSymSize != i) {
		pxd.iSymSize = i;
		Invalidate(0);
	}
}

void CD2DSymView::UpdateColor(UINT id, COLORREF clr)
{
	switch (id) {
	case IDC_COLOR_SYM: pxd.clrSym = clr; break;
	case IDC_COLOR_LINE: pxd.clrLine = clr; break;
	case IDC_COLOR_VEC: pxd.clrVec = clr; break;
	case IDC_COLOR_BACK: pxd.clrBack = clr; break;
	}
	Invalidate(0);
}

void CD2DSymView::OnAppAbout()
{
#ifdef _USE_FPS
	m_bUpdating = true;
	KillTimer(1);
#endif
	CAbtBox aboutDlg;
	aboutDlg.DoModal();

	Start_fps_timer();
}

void CD2DSymView::OnDrawGDI()
{
	m_bCairo = m_bD2D = false;
	Start_fps_timer();
}

void CD2DSymView::OnUpdateDrawGDI(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!m_bCairo && !m_bD2D);
}


#ifdef _USE_CAIRO
void CD2DSymView::OnDrawCairo()
{
	m_bCairo = !m_bCairo;
	m_bD2D = false; //switch to gdi if not turning on cairo
	Start_fps_timer();
}
#endif

void CD2DSymView::OnUpdateDrawCairo(CCmdUI* pCmdUI)
{
	ASSERT(!m_bCairo || !m_bD2D);
#ifdef _USE_CAIRO
	pCmdUI->SetCheck(m_bCairo);
#else
	pCmdUI->Enable(FALSE);
	pCmdUI->SetCheck(FALSE);
#endif
}

void CD2DSymView::OnDrawD2D()
{
	m_bD2D = !m_bD2D;
	m_bCairo = false; //switch to gdi if not turning on D2D
	Start_fps_timer();
}

void CD2DSymView::OnUpdateDrawD2D(CCmdUI *pCmdUI)
{
	ASSERT(!m_bCairo || !m_bD2D);
	pCmdUI->SetCheck(m_bD2D);
}

void CD2DSymView::OnDrawAntialias()
{
	pxd.bAntialias = !pxd.bAntialias;
	Start_fps_timer();
}

void CD2DSymView::OnUpdateDrawAntialias(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bCairo || m_bD2D);
	pCmdUI->SetCheck(pxd.bAntialias);
}

LRESULT CD2DSymView::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
	Invalidate(0);
	return 0; //not used
}

void CD2DSymView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if (theApp.m_pMainWnd) {
		//Under Windows 8 this fcn called twice with theApp.m_pMainWnd==NULL!
		ASSERT(GetMF()->IsCreated());
		Start_fps_timer();
	}
}

void CD2DSymView::OnD2dsw()
{
	m_bD2Dsw = !m_bD2Dsw;
	Start_fps_timer();
}

void CD2DSymView::OnUpdateD2dsw(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bD2D);
	pCmdUI->SetCheck(m_bD2Dsw);
}

void CD2DSymView::Start_fps_timer()
{
	FixModeStr();
#ifdef _USE_FPS
	m_nFrames = m_nTimerCount = 0;
	m_bUpdating = false;
	SetTimer(1, FRAME_TIMER_INTERVAL, NULL); //same as 10 for 100+ iter
#endif
	Invalidate(0);
};
