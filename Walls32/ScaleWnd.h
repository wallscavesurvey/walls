#pragma once

#define BAR_HEIGHT 22

class CScaleWnd : public CStatic
{
	DECLARE_DYNAMIC(CScaleWnd)

public:
	CScaleWnd(bool bFeet) : m_bFeet(bFeet),m_hFont(NULL),m_hbm(NULL),m_width(0),m_hdcAA(NULL),m_hbmAA(NULL),m_fView(-1) {};
	virtual ~CScaleWnd();

	int m_bottom,m_width;

	virtual BOOL Create(CWnd *pWnd);
	void Draw(double mPerPx,double fView);

	void ToggleFeetUnits() {m_bFeet=!m_bFeet;}
	bool IsFeetUnits() {return m_bFeet;}

private:
	HBITMAP m_hbm,m_hbmAA, m_hbmAAOld;
	HDC m_hdcAA;
	HFONT m_hFont;
	double m_fView;
	bool m_bFeet;

	BOOL CreateAAImage(double fView);
	void DrawNorthArrow(HDC hdc, double fView);
	int DrawScaleBar(HDC hdc, double mPerPx, double fView);

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


