// 3dconfig.cpp : implementation file

#include "stdafx.h"
#include "walls.h"
#include "3dconfig.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// C3DConfigDlg dialog

C3DConfigDlg::C3DConfigDlg(BOOL b3D, CWnd* pParent /*=NULL*/)
	: CDialog(C3DConfigDlg::IDD, pParent)
{
	m_b3D = b3D;
	//{{AFX_DATA_INIT(C3DConfigDlg)
	//}}AFX_DATA_INIT
}

void C3DConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C3DConfigDlg)
	DDX_Text(pDX, IDC_3DPATH, m_3DPath);
	DDV_MaxChars(pDX, m_3DPath, 250);
	DDX_Text(pDX, IDC_3DVIEWER, m_3DViewer);
	DDV_MaxChars(pDX, m_3DViewer, 80);
	DDX_Check(pDX, IDC_3DSTATUS, m_3DStatus);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(C3DConfigDlg, CDialog)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//{{AFX_MSG_MAP(C3DConfigDlg)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// C3DConfigDlg message handlers

void C3DConfigDlg::OnBrowse()
{
	CString strFilter;

	if (!AddFilter(strFilter, IDS_EXE_FILES)) return;

	if (DoPromptPathName(m_3DViewer,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		1, strFilter, TRUE,
		IDS_3DBROWSE,
		".exe")) {
		GetDlgItem(IDC_3DVIEWER)->SetWindowText(m_3DViewer);
	}
}

BOOL C3DConfigDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWndTitle(this, 0, m_b3D ? "3D" : "SVG");
	SetWndTitle(GetDlgItem(IDC_ST_3DCONFIG), IDS_3DCONFIG3,
		m_b3D ? "VRML" : "SVG", m_b3D ? "WRL" : "SVG", m_b3D ? "WRL" : "SVG");

	CenterWindow();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

LRESULT C3DConfigDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(13, HELP_CONTEXT);
	return TRUE;
}

