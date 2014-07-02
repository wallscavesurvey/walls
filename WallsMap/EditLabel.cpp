// ShowShapeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "QuickEdit.h"
#include "EditLabel.h"

//Global functions --

LPCSTR CheckAccent(UINT wParam,BYTE &bAccent)
{
	static BYTE rep[2];

	switch(wParam) {

		case VK_SHIFT: {
			if(bAccent) return NULL;
			break;
		}

		case 0xde: { //'
			if(GetKeyState(VK_CONTROL)<0) {
				bAccent=0xde;
				return (LPCSTR)rep;
			}
			break;
		}

		case 0xc0: { //~
			if(GetKeyState(VK_CONTROL)<0 && GetKeyState(VK_SHIFT)<0) {
				bAccent=0xc0;
				return (LPCSTR)rep;
			}
			break;
		}

		case 0x36: { //^
			if(GetKeyState(VK_CONTROL)<0 && GetKeyState(VK_SHIFT)<0) {
				bAccent=0x36;
				return (LPCSTR)rep;
			}
			break;
		}

		case 0xba: { //:
			if(GetKeyState(VK_CONTROL)<0 && GetKeyState(VK_SHIFT)<0) {
				bAccent=0xba;
				return (LPCSTR)rep;
			}
			break;
		}
		case 0x32: { //@
			if(GetKeyState(VK_CONTROL)<0 && GetKeyState(VK_SHIFT)<0) {
				bAccent=0x32;
				return (LPCSTR)rep;
			}
		}
	}

	if(bAccent) {
		*rep=0;
		bool bShft=(GetKeyState(VK_SHIFT)<0);
		switch(bAccent) {
			case 0xde: //'
				switch(wParam) {
					case 0x41 : *rep=bShft?0xc1:0xe1; break; //a
					case 0x45 : *rep=bShft?0xc9:0xe9; break; //e
					case 0x49 : *rep=bShft?0xcd:0xed; break; //i
					case 0x4F : *rep=bShft?0xd3:0xf3; break; //o
					case 0x55 : *rep=bShft?0xda:0xfa; break; //u
				}
				break;

			case 0x36: //^
				switch(wParam) {
					case 0x41 : *rep=bShft?0xc2:0xe2; break; //a
					case 0x45 : *rep=bShft?0xca:0xea; break; //e
					case 0x49 : *rep=bShft?0xce:0xee; break; //i
					case 0x4F : *rep=bShft?0xd4:0xf4; break; //o
					case 0x55 : *rep=bShft?0xdb:0xfb; break; //u
				}
				break;
	
			case 0xc0: //~
				if(wParam==0x4E) *rep=bShft?0xd1:0xf1;
				break;

			case 0xba: //:
				if(wParam==0x55) *rep=bShft?0xdc:0xfc;
				break;

			case 0x32: //@
				if(wParam==0x43) *rep=0xa9; //copyright
				break;
		}

		bAccent=0;
		if(*rep)
			return (LPCSTR)rep;
	}
	return NULL;
}

void SetLowerNoAccents(LPSTR s)
{
	//For removing for search:  ( '    '    '    '    '      ~    :      ^'   ^'   ^'   ^'   ^'     ^~   ^:    )
	static BYTE accents[] =     {0xe1,0xe9,0xed,0xf3,0xfa,  0xf1,0xfc,  0xc1,0xc9,0xcd,0xd3,0xda,  0xd1,0xdc, 0};
	static char non_accents[] = {'a' ,'e' ,'i' ,'o' ,'u' ,  'n' ,'u' ,  'a' ,'e' ,'i' ,'o' ,'u' ,  'n' ,'u' , 0};

	for (LPBYTE cp = (LPBYTE)s ; *cp ; ++cp ) {
		if(*cp > 'Z') {
			if(*cp>0xc0 && (s=strchr((LPSTR)accents,*cp))!=NULL)
				*cp=non_accents[s-(LPSTR)accents];
		}
		else if(*cp>='A') {
			*cp += 'a' - 'A';
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// class CEditLabel --

BEGIN_MESSAGE_MAP(CEditLabel, CEdit)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CEditLabel::OnContextMenu(CWnd* pWnd, CPoint point)
{
	SetFocus();
	CMenu menu;
	menu.CreatePopupMenu();
	int n=0;

	BOOL bReadOnly = GetStyle() & ES_READONLY;
	DWORD flags = CanUndo() && !bReadOnly ? 0 : MF_GRAYED;
	menu.InsertMenu(n++, MF_BYPOSITION|flags, EM_UNDO, MES_UNDO);

	DWORD sel = GetSel();
	flags = LOWORD(sel) == HIWORD(sel) ? MF_GRAYED : 0;
	menu.InsertMenu(n++, MF_BYPOSITION|flags, WM_COPY, MES_COPY);

	flags = (flags == MF_GRAYED || bReadOnly) ? MF_GRAYED : 0;
	menu.InsertMenu(n++, MF_BYPOSITION|flags, WM_CUT, MES_CUT);
	menu.InsertMenu(n++, MF_BYPOSITION|flags, WM_CLEAR, MES_DELETE);

	flags = IsClipboardFormatAvailable(CF_TEXT) && !bReadOnly ? 0 : MF_GRAYED;
	menu.InsertMenu(n++, MF_BYPOSITION|flags, WM_PASTE, MES_PASTE);

	//menu.InsertMenu(n++, MF_BYPOSITION|MF_SEPARATOR);

	int len = GetWindowTextLength();
	flags = (!len || (LOWORD(sel) == 0 && HIWORD(sel) == len)) ? MF_GRAYED : 0;
	menu.InsertMenu(n++, MF_BYPOSITION|flags, ME_SELECTALL, MES_SELECTALL);

	if(m_cmd) {
		menu.InsertMenu(n++, MF_BYPOSITION|MF_SEPARATOR);
		menu.InsertMenu(n++, MF_BYPOSITION,m_cmd,m_pmes);
	}

	if (point.x == -1 || point.y == -1)	{
		CRect rc;
		GetClientRect(&rc);
		point = rc.CenterPoint();
		ClientToScreen(&point);
	}

	int nCmd = menu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RETURNCMD|TPM_RIGHTBUTTON, point.x, point.y, this);

	menu.DestroyMenu();

	if (nCmd < 0) return;

	if(m_pParent && nCmd==m_cmd) {
		m_pParent->PostMessage(nCmd);
		return;
	}

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

BOOL CEditLabel::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message==WM_KEYDOWN) {
		LPCSTR p=CheckAccent(pMsg->wParam,m_bAccent);
		if(p) {
			if(!m_bAccent) ReplaceSel(p);
			return TRUE;
		}
		if(m_cmd && pMsg->wParam==VK_RETURN) {
			m_pParent->PostMessage(m_cmd,2);
			return TRUE;
		}
	}
	return CEdit::PreTranslateMessage(pMsg);
}
