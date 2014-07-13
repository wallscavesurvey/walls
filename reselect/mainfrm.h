// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__01954CBA_BD5C_4A37_8904_56302E742231__INCLUDED_)
#define AFX_MAINFRM_H__01954CBA_BD5C_4A37_8904_56302E742231__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
	
public: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

public:

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
	void SetAddress(LPCTSTR lpszUrl);
	void StartAnimation(BOOL bTimer);
	void StopAnimation();
	void InitAddressBar();
    void RestoreWindow(int nShow);
	afx_msg void OnGoTss();

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
	BOOL m_bShowAddress;
	CComboBoxEx m_wndAddress;
	CAnimateCtrl m_wndAnimate;

	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditFind();
	afx_msg void OnEditPaste();
	afx_msg void OnEditSelectAll();
	afx_msg void OnShowAddress();
	afx_msg void OnUpdateShowAddress(CCmdUI* pCmdUI);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void DoNothing();
	afx_msg void OnNewAddress();
	afx_msg void OnNewAddressEnter();
	afx_msg void OnDropDown(NMHDR* pNotifyStruct, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__01954CBA_BD5C_4A37_8904_56302E742231__INCLUDED_)
