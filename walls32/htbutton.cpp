#include "stdafx.h"
#include "htbutton.h"

BEGIN_MESSAGE_MAP(CHTButton, CButton)
	//{{AFX_MSG_MAP(CHTButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CHTButton::OnLButtonDown(UINT nFlags, CPoint pt)
{
	// the CWnd* returned by CWnd::GetFocus may be temporary,
	// so don't save it.  Save the HWND instead.
	m_hwndPrevFocus = CWnd::GetFocus()->GetSafeHwnd();

	CButton::OnLButtonDown(nFlags, pt);
}

void CHTButton::OnLButtonUp(UINT nFlags, CPoint pt)
{
	CButton::OnLButtonUp(nFlags, pt);

	::SetFocus(m_hwndPrevFocus);
}

WNDPROC  CHTButton::pfnSuper = NULL;

WNDPROC* CHTButton::GetSuperWndProcAddr()
{
	return &pfnSuper;
}

