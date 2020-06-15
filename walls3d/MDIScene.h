#if !defined(AFX_MDISCENE_H__19B3B481_C5E3_11D0_B46B_444553540000__INCLUDED_)
#define AFX_MDISCENE_H__19B3B481_C5E3_11D0_B46B_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MDIScene.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMDISceneWnd frame

class CMDISceneWnd : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CMDISceneWnd)
protected:
	CMDISceneWnd();           // protected constructor used by dynamic creation

// Attributes
public:

	// Operations
public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMDISceneWnd)
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMDISceneWnd();

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MDISCENE_H__19B3B481_C5E3_11D0_B46B_444553540000__INCLUDED_)
