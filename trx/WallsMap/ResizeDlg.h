// A flicker-free resizing dialog class
// Copyright (c) 1999 Andy Brown <andy@mirage.dabsol.co.uk>
// You may do whatever you like with this file, I just don't care.

/* ResizeDlg.h
 *
 * Derived from 1999 Andy Brown <andy@mirage.dabsol.co.uk> by Robert Python Apr 2004 BJ
 * goto http://www.codeproject.com/dialog/resizabledialog.asp for Andy Brown's
 * article "Flicker-Free Resizing Dialog".
 *
 * 1, Enhanced by Robert Python (RobertPython@163.com, www.panapax.com) Apr 2004.
 *    In this class, add the following features:
 *    (1) controls reposition and/or resize direction(left-right or top-bottom) sensitive;
 *    (2) add two extra reposition/resize type: ZOOM and DELTA_ZOOM, which is useful in there
 *        are more than one controls need reposition/resize at the same direction.
 *
**/
 
#ifndef __RESIZEDIALOG__H
#define __RESIZEDIALOG__H
#pragma once
#include <vector>
//#include "ResizeIcon.h"

// How window size ...
#define WST_NONE		0x00					// No size changed
#define WST_LEFT		0x01					// size to left
#define WST_TOP			0x02					// size to top
#define WST_RIGHT		0x04					// size to right
#define WST_BOTTOM		0x08					// size to bottom
#define WST_TOPLEFT		(WST_TOP|WST_LEFT)		// size to top & left
#define WST_TOPRIGHT	(WST_TOP|WST_RIGHT)		// size to top & right
#define WST_BOTTOMRIGHT	(WST_BOTTOM|WST_RIGHT)	// size to bottom & right
#define WST_BOTTOMLEFT	(WST_BOTTOM|WST_LEFT)	// size to bottom & right

enum {				// possible Control reSize Type
	CST_NONE = 0,
	CST_RESIZE,		// NOMOVE + SIZE, add all delta-size of dlg to control
	CST_REPOS,		// MOVE(absolutely) + NOSIZE, move control's pos by delta-size
	CST_RELATIVE,	// MOVE(proportional)  + NOSIZE, keep control always at a relative pos
	CST_ZOOM,		// MOVE + SIZE (both are automatically proportional)
	CST_DELTA_ZOOM	// MOVE(proportional, set manually) + SIZE(proportional, set manuall)
};

// contained class to hold item state
//
class CItemCtrl
{
public:
	UINT	m_nID;
	UINT	m_stxLeft  	   : 4;			// when left resizing ...
	UINT	m_stxRight     : 4;			// when right resizing ...
	UINT	m_styTop   	   : 4;			// when top resizing ...
	UINT	m_styBottom    : 4;			// when bottom resizing ...
	UINT	m_bFlickerFree : 1;
	UINT	m_bInvalidate  : 1;			// Invalidate ctrl's rect(eg. no-automatical update for static when resize+move)
	UINT	m_r0		   : 14;
	CRect	m_wRect;
	double	m_xRatio, m_cxRatio;
	double	m_yRatio, m_cyRatio;

protected:
	void Assign(const CItemCtrl& src);

public:
	CItemCtrl();
	CItemCtrl(const CItemCtrl& src);

	HDWP OnSize(HDWP hdwp, int sizeType, CRect *pnCltRect, CRect *poCltRect, CRect *pR0, CWnd *pDlg);

	CItemCtrl& operator=(const CItemCtrl& src);
};

class CResizeDlg : public CDialog
{
	DECLARE_DYNAMIC(CResizeDlg)

public:
	CResizeDlg(UINT nID,CWnd *pParentWnd = NULL);
	virtual ~CResizeDlg();

protected:
	DECLARE_MESSAGE_MAP()

public:
	std::vector<CItemCtrl>	m_Items;           // array of controlled items
	CRect					m_cltRect, m_cltR0;
	int						m_xMin, m_yMin, m_xMax, m_yMax;
	int						m_xSt,  m_ySt;
	UINT					m_nDelaySide;
	CPoint					m_ptDragIcon;
	bool					m_bDraggingIcon;

protected:
	void 					AddControl( UINT nID, int xl, int xr, int yt, int yb, int bFlickerFree = 0, 
									    double xRatio = -1.0, double cxRatio = -1.0,
									    double yRatio = -1.0, double cyRatio = -1.0 );
	void 					AllowSizing(int xst, int yst);
	virtual BOOL			OnInitDialog();

	//afx_msg void			OnLButtonDown(UINT nFlags, CPoint point);
	//afx_msg void			OnLButtonUp(UINT nFlags, CPoint point);
	//afx_msg void			OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void			OnSizing(UINT nSide, LPRECT lpRect);
	afx_msg void			OnSize(UINT nType, int cx, int cy);
	afx_msg void			OnGetMinMaxInfo(MINMAXINFO *pmmi);
	afx_msg BOOL			OnEraseBkgnd(CDC* pDC);

public:
	int						UpdateControlRect(UINT nID, CRect *pnr);
	void					Reposition(const CRect &rect,UINT uFlags=0);
};

#endif //	__RESIZEDIALOG__H
