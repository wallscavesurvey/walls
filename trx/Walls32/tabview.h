// tabview.h : interface of the CTabView class
// Written by Gerry High
// V1.2 4/13/94
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __MFX_TABVIEW_H__
#define __MFX_TABVIEW_H__

#include "tabarray.h"

class CTabView : public CView             
{
	DECLARE_DYNCREATE(CTabView)
protected: // create from serialization only
	CTabView();

// Attributes
public:
	// OnUpdate hints
	enum
	{
		hintSwitchTo = 0x0A00,
		hintSwitchFrom
	};

// Operations
public:

// Implementation
public:
	virtual 		~CTabView();
	virtual void 	OnActivateView(BOOL bActivate, CView* pActiveView, CView* pDeactiveView);
	virtual void 	OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void 	OnInitialUpdate();
	int				GetTabIndex() {return m_tabArray.m_curTab;}

	virtual CView* 	addTabView(CRuntimeClass* viewClass,CDocument* document,
					char* tabLabel,	BOOL border = FALSE,BOOL show = FALSE,int tabWidth = 100);
	BOOL 			doSysCommand(UINT nID,LONG lParam);
	void			enableView(int viewIndex, BOOL bEnable = TRUE);
	void			setFrameBorderOn(BOOL on = TRUE);
	void			setLAF(eLookAndFeel LAF=LAF_MSWORD);
	void			setMargin(int margin=7);
	void			setTabHeight(int height=25);
	void			setTabPosition(eTabPosition tabPos=TABSONTOP);
	virtual void 	switchTab(int viewIndex);
    void 			ChangeDoc(CDocument *pDocNew);
    CFont *			GetNormalFont() {return m_tabArray.m_normalFont;}
    CFont *			GetBoldFont() {return m_tabArray.m_boldFont;}
    
protected:
	virtual CView* 	createTabView(CRuntimeClass* viewClass,CDocument* document,CWnd* parentWnd,	BOOL border,BOOL show);
	virtual void	createFonts();
	virtual void	destroyFonts();
	
	void			repositionViews();
#ifdef T_TABS
	BOOL			switchTopTab(CPoint point);
#endif
#ifdef LR_TABS
	BOOL			switchVerticalTab(CPoint point);
#endif

#ifdef _DEBUG
	virtual void 	AssertValid() const;
	virtual void 	Dump(CDumpContext& dc) const;
#endif

	// Implementation data
	int m_width; 				//view width
	int m_height;				//view height
	int m_nTabs;				//number of tabs
	CTabArray m_tabArray;		//array of CTabInfo objects
	CView* m_curView;			//current view
	eLookAndFeel m_lookAndFeel;	// Look of Tabs (either LAF_CHICAGO or LAF_MSWORD)
	
// Generated message map functions
protected:
	//{{AFX_MSG(CTabView)
	afx_msg BOOL 	OnEraseBkgnd(CDC* pDC);
	afx_msg void 	OnSize(UINT nType, int cx, int cy);
	afx_msg void 	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void 	OnSetFocus(CWnd* pOldWnd);
	afx_msg int  	OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif

/////////////////////////////////////////////////////////////////////////////
