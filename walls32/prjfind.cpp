// prjfind.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjfind.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrjFindDlg dialog


CPrjFindDlg::CPrjFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPrjFindDlg::IDD, pParent)
{
	m_wFlags=hist_prjfind.GetFirstFlags();
	//{{AFX_DATA_INIT(CPrjFindDlg)
	m_FindString=hist_prjfind.GetFirstString();
	m_bMatchCase=(m_wFlags&F_MATCHCASE)!=0;
	m_bMatchWholeWord=(m_wFlags&F_MATCHWHOLEWORD)!=0;
	m_bSearchDetached=(m_wFlags&F_SEARCHDETACHED)!=0;
	m_bUseRegEx=(m_wFlags&F_USEREGEX)!=0;
	//}}AFX_DATA_INIT
	m_nNumChecked=0;
}

void CPrjFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrjFindDlg)
	DDX_Text(pDX, IDC_FINDSTRING, m_FindString);
	DDV_MaxChars(pDX, m_FindString, 80);
	DDX_Check(pDX, IDC_MATCHCASE, m_bMatchCase);
	DDX_Check(pDX, IDC_MATCHWHOLEWORD, m_bMatchWholeWord);
	DDX_Check(pDX, IDC_SEARCHDETACHED, m_bSearchDetached);
	DDX_Check(pDX, IDC_USEREGEX, m_bUseRegEx);
	//}}AFX_DATA_MAP
	
	if(pDX->m_bSaveAndValidate) {
		if(!m_bUseRegEx && m_bMatchWholeWord)
			m_FindString.Trim();
		if(!m_FindString.IsEmpty()) {
			m_wFlags=F_MATCHCASE * m_bMatchCase +
	  			F_MATCHWHOLEWORD * m_bMatchWholeWord +
	  			F_SEARCHDETACHED * m_bSearchDetached +
				F_USEREGEX * m_bUseRegEx;
			if(m_bUseRegEx) {
				m_bMatchWholeWord=FALSE;
				if(!CheckRegEx(m_FindString,m_wFlags)) {
					pDX->m_idLastControl=IDC_FINDSTRING;
					//***7.1 pDX->m_hWndLastControl=GetDlgItem(IDC_FINDSTRING)->m_hWnd;
					pDX->m_bEditLastControl=TRUE;
					pDX->Fail();
					return;
				}
			}
			hist_prjfind.InsertAsFirst(m_FindString,m_wFlags);
		}
	}
}

BEGIN_MESSAGE_MAP(CPrjFindDlg, CDialog)
	//{{AFX_MSG_MAP(CPrjFindDlg)
	ON_BN_CLICKED(IDC_CLEARCHKS, OnClearChks)
	ON_CBN_SELCHANGE(IDC_FINDSTRING, OnSelchangeFindstring)
	ON_CBN_EDITCHANGE(IDC_FINDSTRING, OnEditchangeFindstring)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPrjFindDlg message handlers

BOOL CPrjFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	m_tabEdit.SubclassDlgItem(IDC_FINDSTRING,this);
	m_tabEdit.LoadHistory(&hist_prjfind);
	SetWndTitle(GetDlgItem(IDC_ST_NOTE),IDS_TAB_NOTE);
	GetDlgItem(IDC_NUMCHECKED)->SetWindowText(GetIntStr(m_nNumChecked));
	GetDlgItem(IDOK)->EnableWindow(!m_FindString.IsEmpty());
	
	CenterWindow();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPrjFindDlg::OnClearChks()
{
	EndDialog(IDC_CLEARCHKS);
}

void CPrjFindDlg::OnSelchangeFindstring() 
{
	OnSelChangeFindStr(this,TRUE); //bGlobal=TRUE
}

void CPrjFindDlg::OnEditchangeFindstring() 
{
	// TODO: Add your control notification handler code here
	CString s;
	GetDlgItem(IDC_FINDSTRING)->GetWindowText(s);
	GetDlgItem(IDOK)->EnableWindow(!s.IsEmpty());
}

LRESULT CPrjFindDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(230,HELP_CONTEXT);
	return TRUE;
}


