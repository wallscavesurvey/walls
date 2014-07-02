// TabEdit.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "lineview.h"
#include "wedview.h"
#include "TabEdit.h"
#include <dlgs.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabEdit

CTabEdit::CTabEdit()
{
}

CTabEdit::~CTabEdit()
{
}


BEGIN_MESSAGE_MAP(CTabEdit, CEdit)
	//{{AFX_MSG_MAP(CTabEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabEdit message handlers


BOOL CTabEdit::PreTranslateMessage(MSG* pMsg) 
{
   if (pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_TAB && ::GetKeyState(VK_CONTROL)<0) {
	    ReplaceSel("\t");
		return TRUE;
   }
   return CEdit::PreTranslateMessage(pMsg);
}
/////////////////////////////////////////////////////////////////////////////
// CFindTabDlg dialog


CFindTabDlg::CFindTabDlg(BOOL bFindOnly)
	: CFindReplaceDialog()
{
	m_fr.lpTemplateName=MAKEINTRESOURCE(bFindOnly?1540:1541);
	//Modify default settings in FINDREPLACE structure --
	m_fr.Flags |= (DWORD)FR_SHOWHELP;

	m_bFindOnly=bFindOnly;

	//{{AFX_DATA_INIT(CFindTabDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BEGIN_MESSAGE_MAP(CFindTabDlg, CDialog)
	//{{AFX_MSG_MAP(CFindTabDlg)
	//}}AFX_MSG_MAP
	ON_CBN_EDITCHANGE(IDC_FINDSTRING, OnEditchangeFindstring)
	ON_CBN_SELCHANGE(IDC_FINDSTRING, OnSelchangeFindStr)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFindTabDlg message handlers

BOOL CFindTabDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_tabEdit.SubclassDlgItem(IDC_FINDSTRING,this);
	GetDlgItem(IDC_FINDSTRING)->SetWindowText(m_fr.lpstrFindWhat);
	m_tabEdit.LoadHistory(&hist_prjfind);

	if(!m_bFindOnly) {
		m_tabReplace.SubclassDlgItem(IDC_REPLSTRING,this);
		GetDlgItem(IDC_REPLSTRING)->SetWindowText(m_fr.lpstrReplaceWith);
		m_tabReplace.LoadHistory(&hist_prjrepl);

	}
	SetWndTitle(GetDlgItem(IDC_ST_NOTE),IDS_TAB_NOTE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CFindTabDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(230,HELP_CONTEXT);
	return TRUE;
}

void CFindTabDlg::OnEditchangeFindstring()
{
	LPCSTR s=GetFindStringPtr();
	GetDlgItem(IDOK)->EnableWindow(*s!=0);
	if(!m_bFindOnly) {
		GetDlgItem(1024)->EnableWindow(*s!=0);
		GetDlgItem(1025)->EnableWindow(*s!=0);
	}
}

void CFindTabDlg::OnSelchangeFindStr()
{
	OnSelChangeFindStr(this,FALSE); //bGlobal=FALSE
}
