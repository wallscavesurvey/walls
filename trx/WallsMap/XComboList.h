// XComboList.h
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// This software is released into the public domain.
// You are free to use it in any way you like.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XCOMBOLIST_H
#define XCOMBOLIST_H

#ifndef _UTILITY_H
#include "utility.h"
#endif

extern UINT NEAR WM_XCOMBOLIST_VK_ESCAPE;
extern UINT NEAR WM_XCOMBOLIST_LBUTTONUP;


extern UINT NEAR WM_XLISTCTRL_CHECKBOX_CLICKED;

#define XC_MAX_LISTHEIGHT 10
#define XC_RES_LISTITEMS 20

struct XCOMBODATA
{
	// Static info for formatting a specific shapefile's attributes
	XCOMBODATA() : nInitStr(0), wFlgs(0), nFld(0) {}

	VEC_CSTR		sa;						// strings for combo listbox (size at least 2)
	UINT			nFld;					// DBF field number
	WORD			nInitStr;				// index in sa[] of initialization string
	WORD			wFlgs;					// flags (see below)
};

struct XDESCDATA
{
	XDESCDATA() : nFld(0) {}
	XDESCDATA(UINT fld,LPCSTR pDesc) : nFld(fld), sDesc(pDesc) {}
	UINT nFld;
	CString sDesc;
};
typedef std::vector<XDESCDATA> VEC_XDESC;
typedef VEC_XDESC *PVEC_XDESC;

enum {XC_READONLY=1,XC_INITSTR=2,XC_INITPOLY=4,XC_INITELEV=8,XC_HASLIST=16,
	XC_LISTNOEDIT=32,XC_METERS=64,XC_NOPROMPT=128}; //supported flags

typedef XCOMBODATA *PXCOMBODATA;
typedef std::vector<PXCOMBODATA> VEC_PXCOMBO; //avoid deletes?
typedef std::vector<XCOMBODATA> VEC_XCOMBO;
typedef VEC_XCOMBO *PVEC_XCOMBO;
typedef VEC_XCOMBO::iterator it_vec_xcombo;

class CFocusedListBox : public CListBox
{
 //necessary for determining when to close dropdown listbox --
protected:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CXComboList window

class CXComboList : public CWnd
{
// Construction
public:
	CXComboList(CWnd *pParent);
	virtual ~CXComboList();

// Attributes
public:

// Operations
public:
	void SetActive(int nScrollBarWidth,int nItemsVisible);

	int AddString(LPCTSTR lpszItem)
	{
		return m_ListBox.AddString(lpszItem);
	}
	int GetCount()
	{
		return m_ListBox.GetCount();
	}
	void GetText(int nIndex, CString& rString)
	{
		m_ListBox.GetText(nIndex, rString);
	}
	int FindStringExact(int nIndexStart, LPCTSTR lpszFind)
	{
		return m_ListBox.FindStringExact(nIndexStart, lpszFind);
	}
	int SetCurSel(int nSelect)
	{
		return m_ListBox.SetCurSel(nSelect);
	}
	int GetCurSel()
	{
		return m_ListBox.GetCurSel();
	}
	void SetFont(CFont* pFont, BOOL bRedraw = TRUE)
	{
		m_ListBox.SetFont(pFont, bRedraw);
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXComboList)
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual CScrollBar* GetScrollBarCtrl(int nBar);
	//}}AFX_VIRTUAL

	CScrollBar	m_wndSBVert;
	CWnd *		m_pParent;
	bool		m_bActive;

// Implementation
protected:
	CFocusedListBox	m_ListBox;
	BOOL		m_bFirstTime;

	// Generated message map functions
protected:
	//{{AFX_MSG(CXComboList)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //XCOMBOLIST_H
