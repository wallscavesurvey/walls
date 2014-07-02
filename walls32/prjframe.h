// prjframe.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrjFrame frame
class CPrjList;

#ifndef BUTTONB_H
#include "buttonb.h"
#endif

class CPrjFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CPrjFrame)
protected:
	CPrjFrame();			// protected constructor used by dynamic creation

// Attributes
public:
   //Set in CPrjView::OnInitialUpdate() to control resizing (not yet used)
   //POINT m_FrameSize;
   
   CButtonBar m_PrjDlgBar;
   CPrjList *m_pPrjList;
   
// Operations
private:
	BOOL ChkNcLButtonDown(UINT nHitTest);

// Implementation
protected:
	BOOL m_bIconTitle;
	virtual ~CPrjFrame() {};
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	 
	afx_msg void OnIconEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnSetText(WPARAM wParam,LPARAM strText);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CPrjFrame)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnNcRButtonDown(UINT nHitTest, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
