#if !defined(AFX_PAGEVIEW_H__65AD7FE1_C5DF_11D3_8DFD_00105A12A8D4__INCLUDED_)
#define AFX_PAGEVIEW_H__65AD7FE1_C5DF_11D3_8DFD_00105A12A8D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PageView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPageView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#ifndef __REVIEW_H
#include "review.h"
#endif

#define MAX_NUMSEL 500
#define PAGE_ZOOMOUT 150.0

class CPageView : public CPanelView
{
protected:
	CPageView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CPageView)

	CPrjDoc *GetDocument() { return (CPrjDoc *)CView::GetDocument(); }
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint);
	virtual void OnDraw(CDC* pDC);
	// Form Data
public:
	//{{AFX_DATA(CPageView)
	enum { IDD = IDD_PAGEGRID };
	CStatic	m_PagFrm;
	//}}AFX_DATA

// Attributes
public:
	static CRect m_rectBmp;  // Bitmap's client-relative rectangle in view
	CSize m_sizeBmp;    // Size (width and height) of bitmap in pixels
	CPoint m_ptBmp;     // Position for upper left corner of bitmap
	static CDC m_dcBmp; // Compatible Memory DC, with m_cBmp already selected

	static CPoint *m_selpt;
	static int *m_selseq;
	static UINT m_numsel;

	CReView *GetReView() { return (CReView *)m_pTabView; }

	CBitmap m_cBmpBkg;    // Bitmap containing map
	double m_xscale; //xscale (pixels per meter) computed in CPrjDoc::PlotpageGrid()
	double m_xoff, m_yoff, m_xpan, m_ypan;
	double m_pvxscale;     //Specifies view in preview map
	double m_pvxoff, m_pvyoff, m_pvsyoff, m_pvview;
	double m_fViewWidth, m_fViewHeight; //Printer view size in meters
	double m_fFrameWidth, m_fFrameHeight; //Used only for testing frame size changes in PlotPageGrid()
	double m_fPanelWidth, m_fPanelHeight;	  //Panel width and height in meters
	float m_fInchesX, m_fInchesY;
	int m_iCurSel;

protected:
	CBitmap m_cBmp;       // Bitmap with highlighted grid
	HBITMAP m_hBmpOld;    // Handle of old bitmap to save
	HBRUSH m_hBrushOld;   // Handle of old brush to save
	HPEN m_hPenOld;       // Handle of old pen to save
	BOOL m_bCursorInBmp, m_bTracking;

	// Operations
public:
	void ResetContents();
	int * SelectCell(int x, int y);
	int * SelectCell(CPoint& point) { return SelectCell(point.x, point.y); }
	void GetCell(CPoint *p, double x, double y);
	void GetCell(CPoint *p, DPOINT *pt) { GetCell(p, pt->x, pt->y); }
	void PageDimensionStr(char *buf, BOOL bInches);
	void RefreshPageInfo();
	void RefreshScale();
	BOOL CheckSelected();
	int  GetFrameID(char *buf, int len);
	UINT GetNumPages();
	void AdjustPageOffsets(double *pxoff, double *pyoff, UINT npage);

private:
	void DrawFrame(CDC *pDC, CRect *pClip);
	void GetLoc(DPOINT *pt, CPoint *p);
	void RefreshPages();

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CPageView)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CPageView();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	afx_msg void OnSysCommand(UINT nChar, LONG lParam);
	afx_msg void OnMapExport();
	afx_msg void OnUpdateUTM(CCmdUI* pCmdUI);
	afx_msg LRESULT OnMouseLeave(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CPageView)
	afx_msg void OnDestroy();
	afx_msg void OnPanelZoomIn();
	afx_msg void OnPanelZoomOut();
	afx_msg void OnPanelDefault();
	afx_msg void OnLandscape();
	afx_msg void OnFilePrintSetup();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnChgView();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnFilePrint();
	afx_msg void OnSelectAll();
	afx_msg void OnClearAll();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPageOffsets();
	afx_msg void OnDisplayOptions();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGEVIEW_H__65AD7FE1_C5DF_11D3_8DFD_00105A12A8D4__INCLUDED_)
