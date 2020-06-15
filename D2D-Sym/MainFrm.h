
// MainFrm.h : interface of the CMainFrame class
//

#pragma once

class CMainFrame : public CFrameWnd
{

protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

	// Attributes
public:

	// Operations
public:

	// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	bool IsCreated() { return m_bCreated; }


	void SetFPS(LPCSTR pMode, double fps)
	{
		ASSERT(m_bCreated);
		CString s;
		if (fps) s.Format("%.1f fps", fps);
		m_wndStatusBar.SetPaneText(2, s);
		m_wndStatusBar.SetPaneText(1, pMode);
	}

protected:  // control bar embedded members
	CToolBar          m_wndToolBar;
	CStatusBar        m_wndStatusBar;
	bool m_bCreated;

	// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnUpdateFPS(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMode(CCmdUI *pCmdUI);
	DECLARE_MESSAGE_MAP()
};


