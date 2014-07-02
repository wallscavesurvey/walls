// CompareMemo.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "CompareMemo.h"
#include "afxdialogex.h"


// CCompareMemo dialog

IMPLEMENT_DYNAMIC(CCompareMemo, CDialog)

CCompareMemo::CCompareMemo(CWnd* pParent /*=NULL*/)
	: CDialog(CCompareMemo::IDD, pParent)
{

}

CCompareMemo::~CCompareMemo()
{
}

UINT CCompareMemo::CheckPaths()
{
	if(m_PathTarget.IsEmpty() || CheckAccess(m_PathTarget, 4)) {
		AfxMessageBox("The file to compare this memo against is unspecified or unavailable.");
		return IDC_PATH_TARGET;
	}
	if(m_PathSource.IsEmpty() || !_access(m_PathTarget,0) && CheckAccess(m_PathTarget, 4)) {
		AfxMessageBox("The file needed for storing this memo is unspecified or can't be written.");
		return IDC_PATH_SOURCE;
	}
	return 0;
}

void CCompareMemo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATH_SOURCE, m_PathSource);
	DDV_MaxChars(pDX, m_PathSource, 260);
	DDX_Text(pDX, IDC_PATH_TARGET, m_PathTarget);
	DDV_MaxChars(pDX, m_PathTarget, 260);

	if(pDX->m_bSaveAndValidate) {
		UINT id=CheckPaths();
		if(id) {
			pDX->m_idLastControl=id;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
	}
}

BEGIN_MESSAGE_MAP(CCompareMemo, CDialog)
	ON_BN_CLICKED(IDC_BROWSE_SRC, &CCompareMemo::OnClickedBrowseSrc)
	ON_BN_CLICKED(IDC_BROWSE_TGT, &CCompareMemo::OnClickedBrowseTgt)
END_MESSAGE_MAP()

// CCompareMemo message handlers

void CCompareMemo::OnClickedBrowseSrc()
{
	CString strFilter;
	if(!AddFilter(strFilter,m_bRTF?IDS_RTF_FILES:IDS_TXT_FILES)) return;
	if(m_bRTF && !AddFilter(strFilter,IDS_TXT_FILES)) return;
	//if(!AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;

	if(!DoPromptPathName(m_PathSource,OFN_HIDEREADONLY,
		m_bRTF?2:1,strFilter,FALSE,IDS_SAVETOFILE,m_bRTF?".rtf":".txt")) return;

	GetDlgItem(IDC_PATH_SOURCE)->SetWindowText(m_PathSource);
}

void CCompareMemo::OnClickedBrowseTgt()
{
	CString strFilter;
	if(!AddFilter(strFilter,m_bRTF?IDS_RTF_FILES:IDS_TXT_FILES)) return;
	if(!AddFilter(strFilter,m_bRTF?IDS_TXT_FILES:IDS_RTF_FILES)) return;
	if(!AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;

	if(!DoPromptPathName(m_PathTarget, OFN_FILEMUSTEXIST,3,strFilter,
		TRUE, IDS_COMPARE_OPEN, m_bRTF?".rtf":".txt")) return;

	GetDlgItem(IDC_PATH_TARGET)->SetWindowText(m_PathTarget);
}


BOOL CCompareMemo::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString title;
	title.Format("Use %s to Compare Text", trx_Stpnam(GetMF()->m_pComparePrg));
	SetWindowText(title);
	GetDlgItem(IDC_ST_MSGHDR)->SetWindowText(
		"This will launch your compare program with the contents of two files displayed side-by-side. "
		"The left file will contain the text of the current memo. The right file will be one you select, "
		"by default the last file you created using the editor's \"Save to file\" menu option.");

	return TRUE;  // return TRUE unless you set the focus to a control

}
