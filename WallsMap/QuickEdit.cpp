// EditCell.cpp : implementation file
//

#include "stdafx.h"
#include "EditLabel.h"
//#include ".\quickedit.h"
#include "quicklist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQuickEdit

CQuickEdit::CQuickEdit(CQuickList* pCtrl, int iItem, int iSubItem,
	HFONT hFont, CString sInitText, BYTE cTyp, bool endonlostfocus)
{
	pListCtrl = pCtrl;
	Item = iItem;
	SubItem = iSubItem;
	InitText = sInitText;
	m_hFont = hFont;
	m_isClosing = m_bChanged = m_bReadOnly = false;
	m_bAccent = 0;
	if ((m_cTyp = cTyp) == 'L') {
		strcpy(m_pYN, "YN ");
		strcpy(m_pTF, "TF ");
		m_pLogChar = m_pYN;
		m_pLogStr[1] = 0;
		m_iLogChar = 2;
	}
	m_endOnLostFocus = endonlostfocus;
}

CQuickEdit::~CQuickEdit()
{
}

BEGIN_MESSAGE_MAP(CQuickEdit, CEdit)
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_WM_CREATE()
	ON_WM_GETDLGCODE()
	ON_WM_LBUTTONDOWN()
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	ON_WM_CONTEXTMENU()
	//ON_WM_LBUTTONDBLCLK()
	//ON_WM_KEYUP()
	//ON_WM_KEYDOWN()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQuickEdit message handlers

void CQuickEdit::SetEndOnLostFocus(bool autoend)
{
	m_endOnLostFocus = autoend;
}

BOOL CQuickEdit::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		{
			LPCSTR p = CheckAccent(pMsg->wParam, m_bAccent);
			if (p) {
				if (!m_bAccent) ReplaceSel(p);
				return TRUE;
			}
		}
		switch (pMsg->wParam) {
		case VK_RETURN:
		case VK_DELETE:
		case VK_ESCAPE:
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;		    	// DO NOT process further

		case VK_UP:
		case VK_DOWN:
			StopEdit(pMsg->wParam);
			return TRUE;
		}
	}

	return CEdit::PreTranslateMessage(pMsg);
}

void CQuickEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	//End edit?
	if (!m_isClosing)
	{
		if (m_endOnLostFocus)
		{
			StopEdit((UINT)-1);
		}
	}
}

void CQuickEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//BOOL Shift = GetKeyState (VK_SHIFT) < 0;
	if (nChar == VK_ESCAPE || nChar == VK_RETURN) {
		StopEdit(nChar);
		return;
	}
	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

int CQuickEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Set the proper font
	CFont* Font = m_hFont ? CFont::FromHandle(m_hFont) : GetParent()->GetFont();
	SetFont(Font);

	SetWindowText(InitText);
	SetFocus();
	//SetSel (0, -1);
	return 0;
}

UINT CQuickEdit::OnGetDlgCode()
{
	return CEdit::OnGetDlgCode() | DLGC_WANTARROWS; // | DLGC_WANTTAB;
}

bool CQuickEdit::IsChanged()
{
	CString text;
	GetWindowText(text);
	return text.Compare(InitText) != 0;
}

//Stop editing
void CQuickEdit::StopEdit(UINT endkey)
{
	m_isClosing = true;
	//	SetListText();

		//Don't call SetListText to set the text. The reason is that
		//the object may be destoyed before DestroyWindow() in this function
		//is called.

	ASSERT(pListCtrl);

	if (pListCtrl) {
		LPCSTR pText = NULL;
		CString Text;

		if (GetModify() && IsChanged()) {
			GetWindowText(Text);
			pText = Text;
		}

		//pListCtrl->m_edit = NULL;
		pListCtrl->OnEndEdit(Item, SubItem, pText, endkey);
	}

	DestroyWindow();

	delete this;
}

/* Default behavior --
void CQuickEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	char buf[256];
	GetWindowText(buf,256);
	LPCSTR p0=buf+LOWORD(CharFromPos(point)); //number of characters behind the cursor
	LPCSTR p1=p0;

	while(p0>buf && !isspace((BYTE)*p0)) --p0;

	while(*p1 && !isspace((BYTE)*p1)) ++p1;
	while(isspace((BYTE)*p1)) ++p1;
	SetSel(p0-buf,p1-buf);
}
*/

void CQuickEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_cTyp == 'L') {
		if (++m_iLogChar > 2) m_iLogChar = 0;
		*m_pLogStr = m_pLogChar[m_iLogChar];
		SetSel(0, 1);
		ReplaceSel(m_pLogStr);
		SetSel(1, 1);
		return;
	}
	CEdit::OnLButtonDown(nFlags, point);
}

void CQuickEdit::OnChange()
{
	if (IsChanged() != m_bChanged) {
		m_bChanged = !m_bChanged;
		pListCtrl->GetParent()->SendMessage(WM_QUICKLIST_EDITCHANGE, (WPARAM)m_bChanged);
	}
}

void CQuickEdit::OnContextMenu(CWnd* pWnd, CPoint point)
{
	SetFocus();
	CMenu menu;
	menu.CreatePopupMenu();

	BOOL bReadOnly = GetStyle() & ES_READONLY;
	DWORD flags = CanUndo() && !bReadOnly ? 0 : MF_GRAYED;
	menu.InsertMenu(0, MF_BYPOSITION | flags, EM_UNDO, MES_UNDO);

	DWORD sel = GetSel();
	flags = LOWORD(sel) == HIWORD(sel) ? MF_GRAYED : 0;
	menu.InsertMenu(1, MF_BYPOSITION | flags, WM_COPY, MES_COPY);

	flags = (flags == MF_GRAYED || bReadOnly) ? MF_GRAYED : 0;
	menu.InsertMenu(2, MF_BYPOSITION | flags, WM_CUT, MES_CUT);
	menu.InsertMenu(3, MF_BYPOSITION | flags, WM_CLEAR, MES_DELETE);

	flags = IsClipboardFormatAvailable(CF_TEXT) && !bReadOnly ? 0 : MF_GRAYED;
	menu.InsertMenu(4, MF_BYPOSITION | flags, WM_PASTE, MES_PASTE);

	menu.InsertMenu(5, MF_BYPOSITION | MF_SEPARATOR);

	int len = GetWindowTextLength();
	flags = (!len || (LOWORD(sel) == 0 && HIWORD(sel) == len)) ? MF_GRAYED : 0;
	menu.InsertMenu(6, MF_BYPOSITION | flags, ME_SELECTALL, MES_SELECTALL);

	if (point.x == -1 || point.y == -1) {
		CRect rc;
		GetClientRect(&rc);
		point = rc.CenterPoint();
		ClientToScreen(&point);
	}

	int nCmd = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, this);

	menu.DestroyMenu();

	if (nCmd < 0) return;

	switch (nCmd) {
	case EM_UNDO:
	case WM_CUT:
	case WM_COPY:
	case WM_CLEAR:
	case WM_PASTE:
		SendMessage(nCmd);
		break;
	case ME_SELECTALL:
		SendMessage(EM_SETSEL, 0, -1);
		break;
	default:
		SendMessage(WM_COMMAND, nCmd);
	}
}


