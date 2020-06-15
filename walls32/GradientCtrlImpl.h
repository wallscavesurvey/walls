// GradientCtrlImpl.h: interface for the CGradientCtrlImpl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRADIENTCTRLIMPL_H__C28028E9_A49A_498F_8BC4_77DE5D266D04__INCLUDED_)
#define AFX_GRADIENTCTRLIMPL_H__C28028E9_A49A_498F_8BC4_77DE5D266D04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Gradient.h"

class CGradientCtrl;

class CGradientCtrlImpl
{
public:
	CGradientCtrlImpl(CGradientCtrl *owner);
protected:
	virtual ~CGradientCtrlImpl();

	void GetGradientFrame();

	void Draw(CDC *dc);
	void DrawGradient(CDC *dc);
	void DrawHistogram(CDC *dc);
	void DrawPegs(CDC *dc);
	void DrawSelPeg(CDC *dc, int peg);
	void DrawSelPeg(CDC *dc, CPoint point, int direction);
	void DrawPeg(CDC *dc, CPoint point, COLORREF color, int direction);
	void DrawEndPegs(CDC *dc);

	int PointFromPos(double pos);
	double PosFromPoint(int point);
	int GetPegIndent(int index);
	int PtInPeg(CPoint point);

	void GetPegRect(int index, CRect *rect, bool right);

	void ParseToolTipLine(CString &tiptext, CPeg peg);
	void ShowTooltip(CPoint point, CString text);
	CString ExtractLine(CString source, int line);
	void SetTooltipText(CString text);
	void DestroyTooltip();
	void SynchronizeTooltips();
	BOOL GetNearbyValueStr(int x, CString &valstr);

	bool IsVertical();
	int GetDrawWidth();

	HWND m_wndToolTip;
	TOOLINFO m_ToolInfo;
	CGradientCtrl *m_Owner;
	CToolTipCtrl m_ToolTipCtrl;
	int m_RectCount, m_HistWidth, m_HistLength;
	CRect m_GradFrame;
	float *m_pHistValue;
	WORD *m_pHistArray;
	BOOL m_LeftDownSide, m_RightUpSide;

	CPeg m_Null;

	friend class CGradientCtrl;
};

#endif // !defined(AFX_GRADIENTCTRLIMPL_H__C28028E9_A49A_498F_8BC4_77DE5D266D04__INCLUDED_)
