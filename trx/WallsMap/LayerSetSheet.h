#pragma once

#include "ResizeDlg.h"
#include "FileDropTarget.h"
#include "ColumnTreeCtrl.h"
#include "PageDetails.h"
#include "PageLayers.h"

extern BOOL hPropHook;
extern CLayerSetSheet *pLayerSheet;
extern const UINT WM_PROPVIEWDOC;
extern const UINT WM_RETFOCUS;

//===========================================
// CLayerSetSheet

#define WM_RESIZEPAGE WM_USER + 111

class CLayerSetSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CLayerSetSheet)

public:
	CLayerSetSheet(CWallsMapDoc *pDoc, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CLayerSetSheet();

	CWallsMapDoc *GetDoc() {return m_pDoc;}
	void SetPropTitle() {SetWindowText(m_pDoc->GetTitle());}
	void RefreshViews() {PostMessage(WM_PROPVIEWDOC,(WPARAM)m_pDoc,(LPARAM)LHINT_REFRESH);}
	void UpdateDibViews() {PostMessage(WM_PROPVIEWDOC,(WPARAM)m_pDoc,(LPARAM)LHINT_UPDATEDIB);}
	void RepaintViews() {m_pDoc->RepaintViews();}
	void SetNewLayer() {m_pageDetails.SetNewLayer();}

	UINT m_nDelaySide;

protected:
	DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnPropViewDoc(WPARAM wParam,LPARAM);

	virtual void PostNcDestroy();

	BOOL   m_bNeedInit;
	CRect  m_rCrt;
	int    m_nMinCX,m_nMinCY;
	//static int m_nSavedCX,m_nSavedCY;
	static int m_nSavedX,m_nSavedY,m_nSavedCX,m_nSavedCY;
	RECT   m_PageRect;

public:
	virtual BOOL OnInitDialog();
	virtual BOOL Create(CWnd* pParentWnd,DWORD dwStyle=(DWORD)-1,DWORD dwExStyle = 0);
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	void RefreshListItem(PTL ptl) {m_pageLayers.RefreshListItem(ptl);}
	void RedrawList() {m_pageLayers.RedrawList();}
	void SaveWindowSize();
	void RestoreWindowSize();
	void SelectDocTreePosition()
	{
		m_pageLayers.SelectDocTreePosition();
	}
	void SelectLayer(int i)
	{
		m_pDoc->SetSelectedLayerPos(i);
		m_pageLayers.SelectDocTreePosition();
	}

private:
	CWallsMapDoc *m_pDoc;
	CPageDetails m_pageDetails;
	CPageLayers m_pageLayers;
	CFileDropTarget m_dropTarget;

public:
    afx_msg LRESULT OnResizePage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnSizing(UINT nSide, LPRECT lpRect);
	afx_msg void OnDestroy();
};

__inline CLayerSetSheet * CLayerSetPage::Sheet()
{
	return STATIC_DOWNCAST(CLayerSetSheet,GetParent());
}
__inline CWallsMapDoc * CLayerSetPage::GetDoc()
{
	return Sheet()->GetDoc();
}


