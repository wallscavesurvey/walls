#if !defined(AFX_RESELECTVW_H__18720010_76AA_44DA_B362_8DA156A06596__INCLUDED_)
#define AFX_RESELECTVW_H__18720010_76AA_44DA_B362_8DA156A06596__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReselectVw.h : header file
//

#define MAX_ARGV 5
#define SIZ_ARGVBUF 128

/////////////////////////////////////////////////////////////////////////////
// CReselectVw html view

class CReselectDoc;

class CReselectVw : public CHtmlView
{
protected:
	CReselectVw();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CReselectVw)

// html Data
public:
	CReselectDoc* GetDocument();
	//{{AFX_DATA(CReselectVw)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
private:
	//CByteArray m_baPostedData;
	CString m_strTexbib;
	CString m_strNwbb;
	char *m_Argv[MAX_ARGV];
	static BOOL m_bEnableTexbib,m_bEnableNwbb;
	static char *m_pDatabase;

// Operations
public:
	CString ResourceToURL(LPCTSTR lpszURL);
	static void Initialize();
	static void Terminate();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReselectVw)
	public:
	virtual void OnInitialUpdate();
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	void OnTitleChange(LPCTSTR lpszText);
	void OnDocumentComplete(LPCTSTR lpszUrl);
	void OnBeforeNavigate2(LPCTSTR lpszURL, DWORD nFlags,
		LPCTSTR lpszTargetFrameName, CByteArray& baPostedData,
		LPCTSTR lpszHeaders, BOOL* pbCancel);


// Implementation
protected:
	virtual ~CReselectVw();
	virtual BOOL CreateControlSite(COleControlContainer* pContainer, 
	   COleControlSite** ppSite, UINT nID, REFCLSID clsid);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CReselectVw)
	afx_msg void OnGoBack();
	afx_msg void OnGoForward();
	afx_msg void OnViewStop();
	afx_msg void OnViewRefresh();
	afx_msg void OnViewFontsLarge();
	afx_msg void OnViewFontsLargest();
	afx_msg void OnViewFontsMedium();
	afx_msg void OnViewFontsSmall();
	afx_msg void OnViewFontsSmallest();
	afx_msg void OnFileOpen();
	afx_msg void OnGoSearchNwbb();
	afx_msg void OnGoSearchTexbib();
	afx_msg void OnUpdateGoSearchNwbb(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGoSearchTexbib(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in mfcieView.cpp
inline CReselectDoc* CReselectVw::GetDocument()
   { return (CReselectDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESELECTVW_H__18720010_76AA_44DA_B362_8DA156A06596__INCLUDED_)
