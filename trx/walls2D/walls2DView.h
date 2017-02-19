// walls2DView.h : interface of the CWalls2DView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WALLS2DVIEW_H__10BB15DB_7CE5_4808_BC73_37B617220343__INCLUDED_)
#define AFX_WALLS2DVIEW_H__10BB15DB_7CE5_4808_BC73_37B617220343__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWalls2DView : public CHtmlView
{
protected: // create from serialization only
	CWalls2DView();
	DECLARE_DYNCREATE(CWalls2DView)

// Attributes
private:
	static LPSTR m_pszTempHTM_Format;
	static CString m_strLastSVG;
	BOOL m_bShowWalls;
	BOOL m_bShowVectors;
	BOOL m_bShowGrid;
	BOOL m_bShowLabels;
	BOOL m_bShowFlags;
	BOOL m_bShowNotes;
	BOOL m_bShowAdditions;
	BOOL m_bEnableGrid;
	BOOL m_bEnableAdditions;
	BOOL m_bEnableWalls;
	BOOL m_bEnableVectors;
	BOOL m_bEnableLabels;
	BOOL m_bEnableFlags;
	BOOL m_bEnableNotes;
	COleDispatchDriver m_DispDriver;
	BOOL m_bWrappedSVG;
	DISPID m_dispid;

public:
	CWalls2DDoc* GetDocument();
	BOOL OpenSVG(LPCSTR svgpath);

// Operations
public:
	static void Initialize();
	static void Terminate();
	afx_msg void OnUpdateUTM(CCmdUI* pCmdUI);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWalls2DView)
	public:
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL


	BOOL WriteTempHTM(LPCSTR svgpath);
	BOOL EnableLayers();
	int ShowLayer(LPCSTR szLayer,int iShow);
	BOOL ShowStatus(int typ,BOOL *pbShow);
	void OnTitleChange(LPCTSTR lpszText);

#ifdef _DEBUG
	virtual void OnDownloadBegin();
	virtual void OnDownloadComplete();
#endif
	virtual void OnDocumentComplete(LPCTSTR lpszUrl);
	virtual void OnBeforeNavigate2(LPCTSTR lpszURL, DWORD nFlags,
		LPCTSTR lpszTargetFrameName, CByteArray& baPostedData,
		LPCTSTR lpszHeaders, BOOL* pbCancel);

// Implementation
public:
	virtual ~CWalls2DView();

	virtual BOOL CreateControlSite(COleControlContainer* pContainer, 
	   COleControlSite** ppSite, UINT nID, REFCLSID clsid);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CWalls2DView)
	afx_msg void OnGoBack();
	afx_msg void OnGoForward();
	afx_msg void OnViewStop();
	afx_msg void OnFileOpen();
	afx_msg void OnShowVectors();
	afx_msg void OnUpdateShowVectors(CCmdUI* pCmdUI);
	afx_msg void OnShowGrid();
	afx_msg void OnUpdateShowGrid(CCmdUI* pCmdUI);
	afx_msg void OnShowStations();
	afx_msg void OnUpdateShowStations(CCmdUI* pCmdUI);
	afx_msg void OnShowAdditions();
	afx_msg void OnUpdateShowAdditions(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnShowWalls();
	afx_msg void OnUpdateShowWalls(CCmdUI* pCmdUI);
	afx_msg void OnViewRefresh();
	afx_msg void OnUpdateViewRefresh(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStop(CCmdUI* pCmdUI);
	afx_msg void OnViewHome();
	afx_msg void OnUpdateViewHome(CCmdUI* pCmdUI);
	afx_msg void OnShowFlags();
	afx_msg void OnUpdateShowFlags(CCmdUI* pCmdUI);
	afx_msg void OnShowNotes();
	afx_msg void OnUpdateShowNotes(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in walls2DView.cpp
inline CWalls2DDoc* CWalls2DView::GetDocument()
   { return (CWalls2DDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WALLS2DVIEW_H__10BB15DB_7CE5_4808_BC73_37B617220343__INCLUDED_)
