// traview.h : header file for Traverse View of CReview tabbed dialog --
//

#ifndef __TRAVIEW_H
#define __TRAVIEW_H

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#ifndef PRJHIER_H
#include "prjhier.h"
#endif

#ifndef __HTBUTTON_H
#include "htbutton.h"
#endif

#ifndef __REVIEW_H
#include "review.h"
#endif

/////////////////////////////////////////////////////////////////////////////
class CTraView : public CPanelView
{
	DECLARE_DYNCREATE(CTraView)

public:
	enum { NUM_GRD_COLS = 3, GRD_LABEL_COL = 15, GRD_NAME_COL = 18, GRD_ORIG_COL = 25, GRD_CORR_COL = 21 };
	static int m_barCol[NUM_GRD_COLS];

	int m_recNTS;		//NTS record corresponding to current displayed data

protected:
	CHTButton m_buttonNext;
	CHTButton m_buttonPrev;
	BOOL m_bFeetUnits;

	CTraView();			// protected constructor used by dynamic creation

	CPrjDoc *GetDocument() { return (CPrjDoc *)CView::GetDocument(); }
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint);

	// Form Data
public:
	//{{AFX_DATA(CTraView)
	enum { IDD = IDD_TRAVDLG };
	// NOTE: the ClassWizard will add data members here
//}}AFX_DATA

private:
	void LabelUnits();

	// Operations
public:
	void ClearContents();
	void ResetContents();
	void SetF_UVE(char *buf, UINT idc);
	void SetTotals(double hlen, double vlen, double *cxyz, int sign);
	void EnableButtons(int iTrav);
	CReView *GetReView() { return (CReView *)m_pTabView; }
	CListBox *GetTravLB() { return pLB(IDC_VECLIST); }
	void UpdateFont();

	// Implementation
protected:
	virtual ~CTraView();
	afx_msg void OnNextTrav();
	afx_msg void OnPrevTrav();
	afx_msg void OnSysCommand(UINT nChar, LONG lParam);
	afx_msg void OnVectorDblClk();
	// Generated message map functions
	//{{AFX_MSG(CTraView)
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnUseFeet();
	afx_msg void OnMapExport();
	afx_msg void OnEditFind();
	afx_msg void OnFindNext();
	afx_msg void OnUpdateFindNext(CCmdUI* pCmdUI);
	afx_msg void OnFindPrev();
	afx_msg void OnUpdateFindPrev(CCmdUI* pCmdUI);
	afx_msg void OnLocateOnMap();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnVectoInfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif
