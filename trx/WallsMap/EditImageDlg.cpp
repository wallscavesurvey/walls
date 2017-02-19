// EditImageDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "EditImageDlg.h"


// CEditImageDlg dialog

IMPLEMENT_DYNAMIC(CEditImageDlg, CDialog)

CEditImageDlg::CEditImageDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditImageDlg::IDD, pParent)
{
}

CEditImageDlg::~CEditImageDlg()
{
}

void CEditImageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PLACEMENT, m_placement);
	DDX_Text(pDX, IDC_CAPTION, m_csTitle);
	DDV_MaxChars(pDX, m_csTitle, 80);
	DDX_Text(pDX, IDC_PATHNAME, m_csPath);
	DDV_MaxChars(pDX, m_csPath, 250);

	if(pDX->m_bSaveAndValidate) {
		m_rtf.GetText(m_csDesc);
		m_csDesc.Trim();
	}
}

BEGIN_MESSAGE_MAP(CEditImageDlg, CDialog)
	//ON_BN_CLICKED(IDC_BROWSE, &CEditImageDlg::OnBnClickedBrowse)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_REGISTERED_MESSAGE(urm_LINKCLICKED, OnLinkClicked)
END_MESSAGE_MAP()


BOOL CEditImageDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(!m_bIsEditable) {
		SetRO(IDC_CAPTION);
	}
	else {
		VERIFY(m_ceTitle.SubclassDlgItem(IDC_CAPTION,this));
	}
	GetDlgItem(IDC_ST_SIZE)->SetWindowText(m_csSize);

	CRect rect;
	m_placement.GetWindowRect(rect);
	ScreenToClient(rect);
	m_rtf.Create(WS_TABSTOP | WS_CHILD | WS_VISIBLE, rect, this, 300, &CMainFrame::m_fEditFont, CMainFrame::m_clrTxt);

	//GetDlgItem(IDC_EDIT_PASTE)->EnableWindow(m_rtf.CanPaste() && ::IsClipboardFormatAvailable(CF_TEXT));
	m_rtf.SetWordWrap(TRUE);
	m_rtf.SetBackgroundColor(RGB(255, 255, 255));
	m_rtf.GetRichEditCtrl().LimitText(10*1024);
	m_rtf.m_pParentDlg=this;
	m_rtf.m_idContext=IDR_IMG_CONTEXT;

    m_rtf.SetReadOnly(!m_bIsEditable);
    m_rtf.ShowToolbar(0);
    m_rtf.SetText(m_csDesc,0); //This will init toolbar button

	m_rtf.GetRichEditCtrl().SetFocus();

	if(!m_bIsEditable) {
		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
		GetDlgItem(IDCANCEL)->SetFocus();
	}
	return FALSE;  // return TRUE unless you set the focus to a control
}

LRESULT CEditImageDlg::OnLinkClicked(WPARAM, LPARAM lParam)
{
	ENLINK *p_enlink=(ENLINK *)lParam;
	ASSERT(p_enlink && p_enlink->msg==WM_LBUTTONUP && p_enlink->chrg.cpMin<p_enlink->chrg.cpMax);

	if(p_enlink->chrg.cpMin<p_enlink->chrg.cpMax) {
		CString csLink;
		m_rtf.GetRichEditCtrl().GetTextRange(p_enlink->chrg.cpMin, p_enlink->chrg.cpMax, csLink);

		//second param is NULL rather than "open" in order to invoke the default action,
		//otherwise PDF files would return SE_ERR_NOASSOC
		if((int)ShellExecute(m_hWnd, NULL, csLink, NULL, NULL, SW_SHOW)>32)
			return TRUE;
			/*
			if(e<=32) {
			switch(e) {
			case ERROR_FILE_NOT_FOUND :
			e=100; break;
			case ERROR_PATH_NOT_FOUND :
			e=101; break;
			case ERROR_BAD_FORMAT :
			e=102; break;
			case SE_ERR_ACCESSDENIED :
			e=103; break;
			case SE_ERR_NOASSOC :
			e=104; break;
			case SE_ERR_SHARE :
			e=105; break;
			}
			goto _errmsg;
			}
			*/

		CMsgBox("Unable to open %s.\nThe url may be improperly formatted.", (LPCSTR)csLink);
	}

	return TRUE; //rtn not used
}


LRESULT CEditImageDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}
