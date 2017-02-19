// revframe.h : interface of the CRevFrame class
//
/////////////////////////////////////////////////////////////////////////////
// CRevFrame frame

class CRevFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CRevFrame)
protected:
	CRevFrame();			//protected constructor used by dynamic creation

// Attributes
public:
    POINT m_ptFrameSize;    //Set in CCompView::OnInitialUpdate()
	void CenterWindow();    //Called in CCompView::OnInitialUpdate()

// Implementation
protected:
	BOOL m_bIconTitle;
	virtual ~CRevFrame() {};
	virtual void ActivateFrame(int nCmdShow);
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	 
	afx_msg void OnIconEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnSetText(WPARAM wParam,LPARAM strText);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CRevFrame)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnEscapeKey();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
