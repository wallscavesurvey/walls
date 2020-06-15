#pragma once

#ifndef _DIBWRAPPER_H
#include "DIBWrapper.h"
#endif

// CScaleWnd
enum { BAR_MARGIN = 10, BAR_HEIGHT = 24, BAR_WIDTH = 320 };

#define BAR_MAXWIDTH (BAR_WIDTH-2*BAR_MARGIN)

class CScaleWnd : public CStatic
{
	DECLARE_DYNAMIC(CScaleWnd)

public:
	CScaleWnd();
	virtual ~CScaleWnd();

	int m_bottom, m_width;

	virtual BOOL Create(CWnd *pWnd);
	void Draw(double mPerPx, bool bFeet);

	//static int CScaleWnd::DrawScaleBar(HBITMAP hbm,HFONT hFont,double mPerPx,bool bFeet);
	static int cro_DrawScaleBar(CDC *pDC, double mPerPx, long barlen, bool bFeet);
	static void CopyScaleBar(CDIBWrapper &dib, double mPerPx, double fRatio, bool bFeet);

private:
	HFONT m_hFont;
	bool m_bFeet;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
};


