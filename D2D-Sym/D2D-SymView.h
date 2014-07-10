
// D2D-SymView.h : interface of the CD2DSymView class
//

#pragma once

// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "D2D-Sym.h"
#endif
#include "SymDlg.h"
#include "MainFrm.h"
class CD2DSymDoc;

#define _USE_FPS

#ifdef _USE_FPS
#define FRAME_TIMER_INTERVAL 6 //milliseconds (for invalidating view)
#define FRAME_LABEL_INTERVAL 20 //display fps after this many timer intervals
#endif

//Set non-zero to use rounded line caps and joins -- in D2D actally faster than default (flat)
#define _USE_STROKESTYLE 1

#define _GDI_DEFAULT //undefine to remove GDI version when GDI is selected (68 fps)

//only one of following can be true, gdi used if _GDI_DEFAULT defined and both false
#define _START_D2D true
#define _START_CAIRO !_START_D2D

#define CAIRO_BLIT 0 //if !=0, draw to image surface, then draw image to win32 surface (slower with Radeon)

//with _START_SYMSIZE 20 (triangles), _START_VECSIZE 5, vec_count=200 -- Vaio laptop!!
#define _USE_CAIRO_POLY 1 //if 0, 15 fps -- if 1, 52 fps (200 vecs, sz 10) faster!
#define _USE_D2D_POLY 0   //if 0, 65 fps -- if 1, 52 fps (200 vecs, sz 10) slower! (worse with >200 vecs)
//On Win8 desktop, both poly, cairo=50, d2d=65 (10000 vecs, d2d=44 (50 if vecsize=1)

#define _START_BACKSYM FALSE
#define _START_SYMSIZE 20
#define _START_VECSIZE 3
#define _START_ANTIALIASED TRUE

#define VEC_COUNT_IDX 2
const int vec_count[]={10,50,100,200,500,1000,2000,5000,7000,10000};

//fps for antialiased 1-pixel lines (aliased in paren)--
//_D2D_SW_RENDERING defined --
//m_nEndpoints=5000 - None: (66.8 @ 0%), GDI: (60.2 @ 25%), Cairo: 2.4 @ 25% (23.6 @ 25%), D2D: 6.5 @ 92% (24.8 @ 71%)

//_D2D_SW_RENDERING undefined --
//m_nEndpoints=5000 - None: (66.8 @ 0%), GDI: (60.2 @ 25%), Cairo: 2.4 @ 25% (23.6 @ 25%), D2D: 1.5 @ 22% (54.5 @ 16%)
//

template<class Interface>
inline void
SafeRelease(
    Interface **ppInterfaceToRelease
    )
{
    if (*ppInterfaceToRelease != NULL)
    {
        (*ppInterfaceToRelease)->Release();

        (*ppInterfaceToRelease) = NULL;
    }
}

struct CPOINTF {
   CPOINTF(float fx,float fy) : x(fx),y(fy) {}
   float x;
   float y;
};

typedef std::vector<CPOINTF> VECPOINTF;
typedef VECPOINTF::iterator VECPOINTF_IT;

typedef std::vector<CD2DPointF> VEC2DPOINTF;
typedef VEC2DPOINTF::iterator VEC2DPOINTF_IT;

class CD2DSymView : public CView
{
protected: // create from serialization only
	CD2DSymView();
	DECLARE_DYNCREATE(CD2DSymView)

// Attributes
public:
	CD2DSymDoc* GetDocument() const;
	CMainFrame * GetMF() {return (CMainFrame *)theApp.m_pMainWnd;}

// Operations
public:
	void UpdateSymbols(CSymDlg &dlg);
	void UpdateOpacity(UINT id,int iPos);
	void UpdateColor(UINT id,COLORREF clr);
	void UpdateSymSize(int i);

	void Start_fps_timer();

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual BOOL DestroyWindow();

protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

#ifdef _USE_FPS
	afx_msg void OnTimer(UINT nIDEvent);
#endif

#ifdef _USE_CAIRO
	afx_msg void OnDrawCairo() ;
#endif
	afx_msg void OnUpdateDrawCairo(CCmdUI* pCmdUI) ;

	afx_msg void OnDrawD2D();
	afx_msg void OnUpdateDrawD2D(CCmdUI *pCmdUI);

	afx_msg void OnDrawGDI() ;
	afx_msg void OnUpdateDrawGDI(CCmdUI* pCmdUI);

	afx_msg void OnDrawAntialias();
	afx_msg void OnUpdateDrawAntialias(CCmdUI* pCmdUI);
	afx_msg void OnSymbology();
	afx_msg void OnAppAbout();
	afx_msg LRESULT OnDisplayChange(WPARAM wParam,LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnD2dsw();
	afx_msg void OnUpdateD2dsw(CCmdUI *pCmdUI);

// Implementation
private:
	VECPOINTF m_vPT;
	VEC2DPOINTF m_vPTsc;
	CRect m_CRsc;
	void FixModeStr();
	void DrawDataCairo(CDC* pDC);
	void DrawDataGDI(CDC *dc,CRect &rect);

	bool InitializeD2D();
	void DrawDataD2D(CDC &dc);

	bool InitVecArray(int idx);
	int m_iVecCountIdx;

	bool m_bD2D,m_bD2Dsw,m_bCairo;
	int m_right,m_bottom;
	CString m_csMode;
	HBITMAP m_hbm;
	
	//Declare D2D resources --
public:
	static ID2D1Factory *m_pFactory;

private:
	ID2D1DCRenderTarget* m_pRT;
	ID2D1SolidColorBrush *m_pLineBrush;
	ID2D1StrokeStyle *m_pStrokeStyleSolidRoundwRoundCap;

#ifdef _USE_FPS
	bool m_bUpdating;
	UINT m_nFrames;
	UINT m_nTimerCount;
#endif

public:
	virtual ~CD2DSymView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in D2D_SymView.cpp
inline CD2DSymDoc* CD2DSymView::GetDocument() const
   { return reinterpret_cast<CD2DSymDoc*>(m_pDocument); }
#endif

