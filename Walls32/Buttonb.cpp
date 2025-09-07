// buttonb.cpp: implementation file for CButtonBar
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "walls.h"
#include "buttonb.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif 

BEGIN_MESSAGE_MAP(CButtonBar, CDialogBar)
	//{{AFX_MSG_MAP(CButtonBar)
	ON_REGISTERED_MESSAGE(WM_RETFOCUS, OnRetFocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

LRESULT CALLBACK ButtonWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == BM_SETSTYLE/* && (wParam&BS_DEFPUSHBUTTON)*/) {
		//Don't let the button display itself with a thick border --
		if (wParam&(WPARAM)BS_DEFPUSHBUTTON) {
			//The docs are unclear about what other style flags may have been set --
			wParam &= ~BS_DEFPUSHBUTTON;
		}
	}
	LRESULT lret = ::CallWindowProc((WNDPROC)CButtonBar::lpfnOldButtonProc, hWnd, msg, wParam, lParam);

	//Tell Windows to ignore all keyboard input while the control has the focus --
	if (msg == WM_GETDLGCODE) lret |= (DLGC_WANTALLKEYS);
	else if (msg == WM_LBUTTONUP) {
		//We have fixed it so that the only way the button gets input focus is
		//through the dialog manager when the left button is pressed. We
		//therefore return focus whenever the left button is released.
		//Apparently, this does not interfere with the sending of a BN_CLICKED
		//notification. The notification is not sent if the mouse has moved
		//outside the window, but the button still receives WM_LBUTTONUP.

		::SendMessage(::GetParent(hWnd), WM_RETFOCUS, FALSE, 0L);
	}
	return lret;
}

BOOL CALLBACK SubclassButton(HWND hButton, LPARAM bSubClass)
{
	if (bSubClass) {
		//If this control is the first pushbutton created, or if it has the
		//same original WINPROC as the first subclassed pushbutton, subclass it --
		if (!CButtonBar::lpfnOldButtonProc) {
			CButtonBar::lpfnOldButtonProc = ::SetWindowLongPtr(hButton, GWLP_WNDPROC, (LONG_PTR)ButtonWndProc);
		}

		//This message call apparently removes the black outline --
		//at least under Win95. We should probably check WinNT as well.
		//Only by experiment was this found to be necessary! 
		::SendMessage(hButton, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
	}
	else {
		ASSERT(FALSE);
	}

#ifdef _BOLD_LABELS
	HFONT hf = (HFONT)CButtonBar::m_font.GetSafeHandle();
	if (hf) ::SendMessage(hButton, WM_SETFONT, (WPARAM)hf, 0L);
#endif

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//CButtonBar members --

LONG_PTR CButtonBar::lpfnOldButtonProc = NULL;
#ifdef _BOLD_LABELS
CFont CButtonBar::m_font;
#endif

LRESULT CButtonBar::OnRetFocus(WPARAM bSubclass, LPARAM)
{
	if (bSubclass) {
		//Lets subclass the buttons --
		VERIFY(::EnumChildWindows(GetSafeHwnd(), (WNDENUMPROC)SubclassButton, TRUE));
	}
	else if (m_hWndFocus) {
		//Return focus to the desired window
		::SetFocus(m_hWndFocus);
	}

	return LRESULT(TRUE);
}

BOOL CButtonBar::Create(CWnd* pParentWnd, UINT nIDTemplate,
	UINT nStyle, UINT nID)
{
	if (!CDialogBar::Create(pParentWnd, nIDTemplate, nStyle, nID)) return FALSE;

#ifdef _BOLD_LABELS
	if (!m_font.GetSafeHandle()) {
		CLogFont lf;
		lf.Init("Arial", 80, FW_BOLD);
		VERIFY(m_font.CreatePointFontIndirect(&lf));
	}
#endif

	//Let's post a user-defined message to subclass the buttons --
	VERIFY(PostMessage(WM_RETFOCUS, TRUE));
	VERIFY(PostMessage(WM_RETFOCUS, FALSE));
	return TRUE;
}
