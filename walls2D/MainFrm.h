// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__F1645577_B660_482D_AABA_F9A7AC0564A0__INCLUDED_)
#define AFX_MAINFRM_H__F1645577_B660_482D_AABA_F9A7AC0564A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{

protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

	// Attributes
public:
	// Holds status line text
	CString m_csStatText;
	CString m_csNoSVGMsg;

	// Operations
public:
	void HelpMsg();
	CString *NoSVGMsg();

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void StartAnimation(BOOL bTimer);
	void StopAnimation();
	void RestoreWindow(int nShow);

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CReBar      m_wndReBar;
	UINT		m_uTimer;

private:
	HRESULT ExecCmdTarget(const GUID *pguidCmdGroup, DWORD nCmdID);
	BOOL CMainFrame::SelectionPresent(void);

	// Generated message map functions
protected:
	//CAnimateCtrl m_wndAnimate;
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditFind();
	afx_msg void OnEditPaste();
	afx_msg void OnEditSelectAll();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnGoWalls();
	afx_msg void OnDestroy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnViewStop();
	afx_msg void OnUpdateViewStop(CCmdUI* pCmdUI);
	afx_msg void OnFilePrintPreview();
	afx_msg void OnFilePageSetup();
	afx_msg void OnCoordPage();
	afx_msg void OnUpdateCoordPage(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void DoNothing();

	afx_msg void OnDropDown(NMHDR* pNotifyStruct, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__F1645577_B660_482D_AABA_F9A7AC0564A0__INCLUDED_)
