// CompareFcnDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "CompareFcnDlg.h"
#include "afxdialogex.h"


// CCompareFcnDlg dialog

IMPLEMENT_DYNAMIC(CCompareFcnDlg, CDialog)

CCompareFcnDlg::CCompareFcnDlg(LPCSTR pPath, LPCSTR pArgs, CWnd* pParent /*=NULL*/)
	: CDialog(CCompareFcnDlg::IDD, pParent)
	, m_PathName(pPath)
	, m_Arguments(pArgs)
{

}

CCompareFcnDlg::~CCompareFcnDlg()
{
}

void CCompareFcnDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHNAME, m_PathName);
	DDV_MaxChars(pDX, m_PathName, 260);
	DDX_Text(pDX, IDC_ARGUMENTS, m_Arguments);
	DDV_MaxChars(pDX, m_Arguments, 60);
	if(pDX->m_bSaveAndValidate && !m_PathName.IsEmpty() && (m_PathName[1]!=':' || _access(m_PathName, 0))) {
		AfxMessageBox("Program's pathname incomplete or not found.");
		pDX->m_idLastControl=IDC_PATHNAME;
		pDX->Fail();
	}
}


BEGIN_MESSAGE_MAP(CCompareFcnDlg, CDialog)
	ON_BN_CLICKED(IDBROWSE, &CCompareFcnDlg::OnClickedBrowse)
END_MESSAGE_MAP()


BOOL CCompareFcnDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_ST_MSGHDR)->SetWindowText(
		"This program is launched when option \"Compare with last saved\" is selected from the text editor's menu. "
		"It should accept two pathnames as command line arguments. %1 will be the file containing the current memo's text "
		"and %2 will be the file most recently created using the editor's \"Save to file\" menu option.\n\n"
		"For example, enter /s /u \"%1\" \"%2\" for command arguments if the program is WinMerge (WinMergeU.exe).");

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CCompareFcnDlg::OnClickedBrowse()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_EXE_FILES)) return;

	TCHAR pf[MAX_PATH];
	CString path;
	if(SHGetSpecialFolderPath(0, pf, CSIDL_PROGRAM_FILES, FALSE)) {
		strcat(pf, "\\");
		path=pf;
	}
	else if(!m_PathName.IsEmpty()) {
		LPCSTR p=trx_Stpnam(m_PathName);
		path.SetString(p,trx_Stpnam(p)-p);
	}

	if(!DoPromptPathName(path, OFN_FILEMUSTEXIST,3,strFilter,
		TRUE, IDS_COMPARE_FCN, ".exe")) return;

	GetDlgItem(IDC_PATHNAME)->SetWindowText(m_PathName=path);
}
