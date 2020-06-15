// compview.h : header file
//

#ifndef __COMPVIEW_H

#define __COMPVIEW_H

#ifndef __NETLIB_H
#include "netlib.h"
#endif

#ifndef PRJHIER_H
#include "prjhier.h"
#endif 

#ifndef __REVIEW_H
#include "review.h"
#endif

#ifndef __SEGHIER_H
#include "seghier.h"
#endif

#ifndef __WALL_SHP_H
#include "wall_shp.h"
#endif

enum { TRV_ATT = 1, TRV_SEL = 2, TRV_PLT = 4, TRV_PASSAGE = 8, };

struct BASENAME {
	char baseName[NET_SIZNAME];
};

/////////////////////////////////////////////////////////////////////////////
class CCompView : public CPanelView
{
	DECLARE_DYNCREATE(CCompView)

	int m_nLastNetwork, m_nLastSystem, m_nLastTraverse;
	BOOL m_bLoopsShown;

public:
	int m_bIsol, m_bSys, m_bSeg; //0 or positive integer
	int m_nFileno;             //IDX_LIST index corresponding to current network component
	BOOL m_bHomeTrav;          //TRUE when zooming to a traverse in PlotTraverse(0)
	BOOL m_bZoomedTrav;        //TRUE if last operation was a zoom traverse
	CPanelView *m_pOtherPanel; //pPV if IncTrav or IncSys invoked from CPlotView

	int m_iStrFlag;            //"Float" flags of current string --

protected:
	CCompView();			// protected constructor used by dynamic creation

	CPrjDoc *GetDocument() { return (CPrjDoc *)CView::GetDocument(); }
	virtual void OnInitialUpdate();

	// Form Data
public:
	//{{AFX_DATA(CCompView)
	enum { IDD = IDD_COMPDLG };
	// NOTE: the ClassWizard will add data members here
//}}AFX_DATA

	static BYTE FAR *m_pNamTyp;
	static NTA_FLAGSTYLE *m_pFlagStyle;

	//For Walls outline drawing --
	static NTW_HEADER m_NTWhdr;
	static POINT *m_pNTWpnt;
	static NTW_POLYGON *m_pNTWply;

	int    m_iBaseNameCount;
	BOOL   m_bBaseName_filled;
	BASENAME *m_pBaseName;
	void GetBaseNames();

private:
	void ClearListBox(UINT idc);
	void SetSystem(int i);
	void ResetView();
	void UpdateFloat(int iSys, int iTrav);
	void OnFloat(int i);
	void DisableFloat();
	void ScaleReviewFont();
	void OnFindPrevNext(int iDir);

	// Operations
public:
	static void Terminate();
	static void Initialize();
	void GoToVector();
	BOOL GoToComponent();
	void UpdateFont();
	void Close() { OnFinished(); }
	void PostClose();
	void BringToTop();
	void ResetContents();
	void SetLoopCount(int nh, int nv);
	void SetTotals(UINT n, double d);
	BOOL SetTraverse(); //Invoked by CReView::switchTab(TAB_TRAV)
	void TravToggle();
	void PlotTraverse(int iZoom);
	CReView *GetReView() { return (CReView *)m_pTabView; }
	CTraView *GetTraView() { return pREV->GetTraView(); } //Used by CPrjDoc::SetTraverse()
	CPlotView *GetPlotView() { return pREV->GetPlotView(); } //Used by CPrjDoc::PlotTraverse()
	CSegView *GetSegView() { return pREV->GetSegView(); } //Used by CPrjDoc::PlotFrame()
	CPageView *GetPageView() { return pREV->GetPageView(); } //Used by CPrjDoc::PlotFrame()
	void Show(UINT idc, BOOL bShow)
	{
		GetDlgItem(idc)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
	}

	void ShowAbort(BOOL bShow);

	//Used in CSegView::OnDetails() --
	int NumNetworks()
	{
		return pLB(IDC_NETWORKS)->GetCount();
	}

	void GetNetworkBaseName(CString &name);

	//Messages generated during WALLNET4 adjustment --
	void StatMsg(char *msg);
	void AddString(char *msg, UINT idc);
	void SysTTL(char *msg);
	void LoopTTL(int nch, int ncv = 0);
	void NetMsg(NET_WORK *pNet, int bSurvey);
	void OnTravChg();
	BOOL OnSysChg();
	BOOL OnIsolChg();

	afx_msg void OnSysChgSel();
	afx_msg void OnIsolChgSel();
	afx_msg void OnTravChgSel();
	afx_msg void OnEditFind();
	afx_msg void OnFindNext();
	afx_msg void OnUpdateFindNext(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFindPrev(CCmdUI* pCmdUI);
	afx_msg void OnFindPrev();

	// Implementation
protected:
	virtual void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint);
	virtual ~CCompView();

	afx_msg void OnSysCommand(UINT nChar, LONG lParam);
	afx_msg void OnSurveyDblClk();
	afx_msg void OnTravDblClk();
	afx_msg void OnNetChg();

	// Generated message map functions
	afx_msg void OnFinished();
	afx_msg void OnCompleted();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnHomeTrav();
	afx_msg void OnFloatH();
	afx_msg void OnFloatV();
	afx_msg void OnMapExport();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnUpdateTravToggle(CCmdUI* pCmdUI);
	afx_msg void OnTravToggle();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif
