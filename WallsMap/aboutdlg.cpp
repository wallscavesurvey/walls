// aboutdlg.cpp : implementation file
//

#include "stdafx.h"
#include "StdioFileEx.h"
//#include <dos.h>
#include "wallsmap.h"
#include "MemUsageDlg.h"
#include "aboutdlg.h"
//#include <direct.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_BN_CLICKED(IDC_MEMORY_USAGE, &CAboutDlg::OnBnClickedMemoryUsage)
	ON_BN_CLICKED(IDC_HELPCONTENTS, &CAboutDlg::OnBnClickedHelp)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAboutDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAboutDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog()
{
	m_textLink.SubclassDlgItem(IDC_NEWURL, this);
	CDialog::OnInitDialog();
	CString fmt, sVersion, urlName;
	CStdioFileEx file;
	if (file.Open(theApp.m_csUrlPath, CFile::modeRead | CFile::typeText)) {
		file.ReadString(urlName);
		file.ReadString(urlName);
		if (urlName.GetLength() > 11 && !memcmp("URL=http://", (LPCSTR)urlName, 11)) {
			m_textLink.SetLink((LPCSTR)urlName + 11);
			LPCSTR pExt = strstr(theApp.m_csUrlPath, "WallsMap-");
			if (pExt) {
				fmt.SetString(pExt + 9, trx_Stpext(pExt) - (pExt + 9));
				sVersion.Format(" (%s Edition)", (LPCSTR)fmt);
			}
		}
		file.Close();
	}
	fmt.LoadStringA(IDS_ABOUT_TITLE);
	urlName.Format(fmt, (LPCSTR)sVersion);
	GetDlgItem(IDC_ST_TITLE)->SetWindowTextA(urlName);
	CenterWindow();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAboutDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	EndDialog(IDOK);
}

void CAboutDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	EndDialog(IDOK);
}

void CAboutDlg::OnBnClickedMemoryUsage()
{
	CMemUsageDlg dlg;
	dlg.DoModal();
}

void CAboutDlg::OnBnClickedHelp()
{
	AfxGetApp()->WinHelp(1000);
	EndDialog(IDOK);
}


LRESULT CAboutDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	OnBnClickedHelp();
	return TRUE;
}
