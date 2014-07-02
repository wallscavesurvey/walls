// review.h : interface of the CReView class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __REVIEW_H
#define __REVIEW_H

#ifndef __PRJFONT_H
 #include "prjfont.h"
#endif
#ifndef __PANELVW_H
#include "panelvw.h"
#endif

#define pLB(id) ((CListBox *)GetDlgItem(id))
#define pREV ((CReView *)m_pTabView)

enum eTABS {TAB_COMP,TAB_TRAV,TAB_PLOT,TAB_PAGE,TAB_SEG};

/////////////////////////////////////////////////////////////////////////////

struct FONT_SPEC {	//Used by CReview::SetFontsBold()
       HFONT hFont;
       int *pDlgFont_id;
};
typedef FONT_SPEC FAR *lpFONT_SPEC;

class CPlotView;
class CPageView;
class CTraView;
class CCompView;
class CSegView;

class CReView : public CTabView
{
protected: // create from serialization only
	CReView();
	DECLARE_DYNCREATE(CReView)

// Attributes
public:
    static PRJFONT m_ReviewFont;
	static HBRUSH m_hBrush;  //Brush for dialog panels - set in Initialize()
	static HBRUSH m_hPltBrush;  //Brush for map panel - set in Initialize()
	static CPen m_PenGrid;   //Pen for CTraView's grid lines
	static CPen m_PenBorder; //Pen for CTraView's label borders
	
	static CPen m_PenNet;    //Pen for unadjusted vectors
	static CPen m_PenSys;    //Pen for unselected loop systems
	static CPen m_PenTravSel;//Pen for selected traverse 
	static CPen m_PenTravRaw;//Pen for unadjusted floating traverse
	static CPen m_PenFragRaw;//Pen for unadjusted floating chain fragment
	static CPen m_PenTravFrag;//Pen for traverse fragment
	static CPen m_PenSysSel; //Pen for selected system
	static BOOL m_bFontScaled;
	
private:
    static FONT_SPEC font_spec; //Used by SetFontsBold()	

// Operations
public:
	CPrjDoc* GetDocument();
	
	void ResetContents(); //Clears contents of all CPanelViews
	
    CPlotView *GetPlotView()
    {
		return (CPlotView *)((CTabInfo*)m_tabArray[TAB_PLOT])->m_pView;
    }
    CPageView *GetPageView()
    {
		return (CPageView *)((CTabInfo*)m_tabArray[TAB_PAGE])->m_pView;
    }
    CTraView *GetTraView()
    {
		return (CTraView *)((CTabInfo*)m_tabArray[TAB_TRAV])->m_pView;
    }
    CCompView *GetCompView()
    {
		return (CCompView *)((CTabInfo*)m_tabArray[TAB_COMP])->m_pView;
    }
    CSegView *GetSegView()
    {
		return (CSegView *)((CTabInfo*)m_tabArray[TAB_SEG])->m_pView;
    }
	static void Initialize();
    static void Terminate();
	static void ChangeFont();
	//Used by CPlotView and CSegView(?) to revise fonts of dialog controls --
    void SetFontsBold(HWND hWnd,int *pCtl);

// Implementation
public:
	virtual ~CReView();
	virtual void OnInitialUpdate();
	virtual void switchTab(int viewIndex);
	
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CReView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG	// debug version in Review.cpp
inline CPrjDoc* CReView::GetDocument()
   { return (CPrjDoc*) m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
#endif
