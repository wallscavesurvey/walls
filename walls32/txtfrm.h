// txtfrm.h : interface of the CTxtFrame class
//
// We override CMDIChildWnd to customize the MDI child's title bar.
// By default the title bar shows the document name.  But we want
// it to instead show the text defined as the first string in
// the document template STRINGTABLE resource.

/////////////////////////////////////////////////////////////////////////////

class CTxtFrame : public CMDIChildWnd
{
    DECLARE_DYNCREATE(CTxtFrame)
public:
    BOOL m_bClosed;
protected:
    BOOL m_bIconTitle;
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg void OnIconEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnSetText(WPARAM wParam,LPARAM strText);

	// Generated message map functions
	//{{AFX_MSG(CTxtFrame)
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

