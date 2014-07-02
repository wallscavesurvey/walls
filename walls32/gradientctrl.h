#if !defined(AFX_GRADIENTCTRL_H__BA457C5B_AA8B_4E1F_8399_57B9E3C1A406__INCLUDED_)
#define AFX_GRADIENTCTRL_H__BA457C5B_AA8B_4E1F_8399_57B9E3C1A406__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GradientCtrl.h : header file
//

#include "Gradient.h"
#include "GradientCtrlImpl.h"

#define GCW_AUTO -1

#define GC_SELCHANGE			1
#define GC_PEGMOVE				2
#define GC_PEGMOVED				3
#define GC_PEGREMOVED			4
#define GC_CREATEPEG			5
#define GC_EDITPEG				6
#define GC_CHANGE				7

struct PegNMHDR
{
	NMHDR nmhdr;
	CPeg peg;
	int index;
};

struct PegCreateNMHDR
{
	NMHDR nmhdr;
	double position;
	COLORREF color;
};

class CGradientCtrlImpl;

/////////////////////////////////////////////////////////////////////////////
// CGradientCtrl window

class CGradientCtrl : public CWnd
{
// Construction
public:
	CGradientCtrl();
	BOOL Create(const RECT& rect, CWnd* pParentWnd, UINT nID);
	
	enum Orientation
	{
		ForceHorizontal,
		ForceVertical,
		Auto
	};

// Attributes
public:
	int GetHistWidth() const {return m_Impl->m_HistWidth;}
	int GetHistLength() const {return m_Impl->m_HistLength;}
	float *GetHistValue() {return m_Impl->m_pHistValue;}
	int GetGradientWidth() const {return m_Width;}
	void SetGradientWidth(int iWidth)
	{
		m_Width = iWidth; m_Impl->GetGradientFrame();
	}
	int GetSelIndex() const {return m_Selected;}
	int SetSelIndex(int iSel);
	const CPeg GetSelPeg() const;
	CGradient& GetGradient() {return m_Gradient;}
	void SetGradient(CGradient src) {m_Gradient = src;}
	void ShowTooltips(BOOL bShow = true);
	Orientation GetOrientation() const {return m_Orientation;}
	void SetOrientation(Orientation orientation) {m_Orientation = orientation;}
	BOOL GetPegSide(BOOL rightup) const;
	void SetPegSide(BOOL setrightup, BOOL enable);
	void SetTooltipFormat(const CString format);
	CString GetTooltipFormat() const;

// Operations
public:
	int MoveSelected(double newpos, BOOL bUpdate);
	COLORREF SetColorSelected(COLORREF crNewColor, BOOL bUpdate);

// Internals
protected:
	BOOL RegisterWindowClass();
	void GetPegRgn(CRgn *rgn);
	void SendBasicNotification(UINT code, CPeg peg, int index);
	void SendSelectedMessage(UINT code) {SendBasicNotification(code,GetSelPeg(),m_Selected);}
	void CreatePeg(double pos,COLORREF color);

	CGradient m_Gradient;
	int m_Width;

	int m_Selected;
	CPoint m_MouseDown;

	BOOL m_bShowToolTip;
	CString m_ToolTipFormat;

	enum Orientation m_Orientation;

	CGradientCtrlImpl *m_Impl;

	friend class CGradientCtrlImpl;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGradientCtrl)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGradientCtrl();

	// Generated message map functions
protected:
	afx_msg LRESULT OnMouseLeave(WPARAM wParam,LPARAM strText);
	//{{AFX_MSG(CGradientCtrl)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRADIENTCTRL_H__BA457C5B_AA8B_4E1F_8399_57B9E3C1A406__INCLUDED_)
