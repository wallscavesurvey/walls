// FindDlgTbl.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "FindDlgTbl.h"


// CFindDlgTbl

IMPLEMENT_DYNAMIC(CFindDlgTbl, CFindReplaceDialog)

CFindDlgTbl::CFindDlgTbl(LPCSTR pName,int iCol,BYTE fTyp) : m_bSelectAll(FALSE)
, m_pName(pName)
, m_iCol(iCol)
, m_fTyp(fTyp)
, m_bHasMemos(false)
, m_bSearchRTF(false)
{
}

CFindDlgTbl::~CFindDlgTbl()
{
}

BEGIN_MESSAGE_MAP(CFindDlgTbl, CFindReplaceDialog)
	ON_BN_CLICKED(IDOK,OnFindNext)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectAll)
	ON_EN_CHANGE(1152, OnEnChangeFindWhat)
	ON_CBN_SELCHANGE(IDC_LOOKIN, OnSelChangeLookIn)
END_MESSAGE_MAP()

// CFindDlgTbl message handlers

void CFindDlgTbl::EnableControls()
{
	int bShow=m_iCol?SW_SHOW:SW_HIDE;
	GetDlgItem(IDC_LOOKIN)->ShowWindow(bShow);
	GetDlgItem(IDC_ST_LOOKIN)->ShowWindow(bShow);
	//GetDlgItem(IDC_SEARCH_MEMOS)->ShowWindow(bShow);
	GetDlgItem(1040)->ShowWindow(bShow);
	GetDlgItem(1041)->ShowWindow(bShow);
	if(bShow==SW_HIDE) {
		GetDlgItem(1152)->SetWindowText("<Deleted Records>");
	}
	GetDlgItem(IDC_SEARCH_RTF)->ShowWindow((bShow && m_fTyp=='M')?SW_SHOW:SW_HIDE);
	((CEdit *)GetDlgItem(1152))->SetReadOnly(bShow==SW_HIDE);
}

BOOL CFindDlgTbl::OnInitDialog()
{
	CFindReplaceDialog::OnInitDialog();

	VERIFY(m_ce.SubclassDlgItem(1152,this)); //subclass "Find What" edit control
	VERIFY(m_cbLookIn.SubclassDlgItem(IDC_LOOKIN,this)); //subclass "Look in" edit control
	m_cbLookIn.AddString(m_pName);
	m_cbLookIn.AddString("Visible fields");
	m_cbLookIn.SetCurSel(0);

	EnableControls();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CFindDlgTbl::SetColumnData(LPCSTR pNam,int iCol,BYTE fTyp)
{
	m_pName=pNam;
	m_iCol=iCol;
	m_fTyp=fTyp;
	EnableControls();
	if(iCol) {
		m_cbLookIn.DeleteString(0);
		m_cbLookIn.InsertString(0,pNam);
		m_cbLookIn.SetCurSel(0);
		if(m_bHasMemos) {
			GetDlgItem(IDC_SEARCH_MEMOS)->ShowWindow(SW_HIDE);
		}
	}
}

void CFindDlgTbl::OnBnClickedSelectAll()
{
	//CWnd *p=CFindReplaceDialog::GetParent();
	m_bSelectAll=true;
	CFindReplaceDialog::SendMessage(WM_COMMAND,IDOK,0);
	m_bSelectAll=false;
}

void CFindDlgTbl::OnEnChangeFindWhat()
{
	CString text;
	GetDlgItem(1152)->GetWindowText(text);
	text.Trim();
	BOOL bEnable=text.GetLength()!=0;
	Enable(IDC_SELECTALL,bEnable);
	Enable(IDOK,bEnable);
}

void CFindDlgTbl::OnFindNext()
{
	//initialize extra control values 
	m_bSearchMemos=IsDlgButtonChecked(IDC_SEARCH_MEMOS)==BST_CHECKED;
	m_bSearchRTF=IsDlgButtonChecked(IDC_SEARCH_RTF)==BST_CHECKED;
	Default(); //invokes CDBGridDlg::OnFindDlgMessage
}

void CFindDlgTbl::OnSelChangeLookIn() 
{
	if(!m_bHasMemos) return;
	int i=m_cbLookIn.GetCurSel();
	GetDlgItem(IDC_SEARCH_MEMOS)->ShowWindow((i>0)?SW_SHOW:SW_HIDE);
	GetDlgItem(IDC_SEARCH_RTF)->ShowWindow((i>0 || m_fTyp=='M')?SW_SHOW:SW_HIDE);
}


