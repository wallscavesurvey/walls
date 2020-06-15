// HTBUTTON.H -- CButton class that does not retain focus --

#ifndef __HTBUTTON_H
#define __HTBUTTON_H

class CHTButton : public CButton
{
private:
	HWND m_hwndPrevFocus;
	static WNDPROC pfnSuper;

protected:
	virtual WNDPROC *GetSuperWndProcAddr();
	// Generated message map functions
	//{{AFX_MSG(CHTButton)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif


