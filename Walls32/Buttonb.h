// buttonb.h : header file for CButtonBar
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BUTTONB_H
#define BUTTONB_H
#undef _BOLD_LABELS

class CButtonBar : public CDialogBar
{
	//Set by SetFocusWindow() to the window requiring focus after
	//a button is clicked and released --
private:
	HWND m_hWndFocus;

	//We require all subclassed buttons in CButtonBar
	//objects in our application to be of the same kind --
public:
	static LONG_PTR lpfnOldButtonProc;
#ifdef _BOLD_LABELS
	static CFont m_font;
#endif     
public:
	CButtonBar() { m_hWndFocus = 0; }

	void SetFocusWindow(CWnd *pWnd) { m_hWndFocus = pWnd->GetSafeHwnd(); }

	//Override of CDialogBar::Create() --
	BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);

protected:
	//{{AFX_MSG(CButtonBar)
	afx_msg LRESULT OnRetFocus(WPARAM, LPARAM);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
#endif
