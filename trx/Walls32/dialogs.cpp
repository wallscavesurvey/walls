// dialogs.cpp : Miscellaneous small dialogs
//

#include "stdafx.h"
#include "walls.h"
#include "dialogs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
CLineNoDlg::CLineNoDlg(CWnd* pParent,LINENO nLineNo,LINENO nMaxLineNo)
	: CDialog(CLineNoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLineNoDlg)
	m_nLineNo=nLineNo;
	m_nMaxLineNo=nMaxLineNo;
	//}}AFX_DATA_INIT
}

void CLineNoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLineNoDlg)
	DDX_Text(pDX, IDC_EDIT1, m_nLineNo);
	DDV_MinMaxUInt(pDX, m_nLineNo, 1, m_nMaxLineNo);
	//}}AFX_DATA_MAP
}

BOOL CLineNoDlg::OnInitDialog()
{ 
	CDialog::OnInitDialog(); 
	CenterWindow();
	//::MessageBeep(MB_ICONEXCLAMATION);
	return TRUE;
}

BEGIN_MESSAGE_MAP(CLineNoDlg, CDialog)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
//ApplyToAll Dialog used in CSegView --

//BOOL CApplyDlg::m_bNoPromptStatus=FALSE;

CApplyDlg::CApplyDlg(UINT nID,BOOL *pbNoPrompt,LPCSTR pTitle,LPCSTR pMsg,CWnd* pParent /*=NULL*/)
	: m_pbNoPrompt(pbNoPrompt), m_pTitle(pTitle), m_pMsg(pMsg), CDialog(nID, pParent)
{
	m_bNoPrompt = FALSE;
}

BEGIN_MESSAGE_MAP(CApplyDlg, CDialog)
END_MESSAGE_MAP()

void CApplyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_APPLY_PROMPT, m_bNoPrompt);
	
	if(pDX->m_bSaveAndValidate) *m_pbNoPrompt=m_bNoPrompt;
} 

BOOL CApplyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetWindowText(m_pTitle);
	if(m_pMsg) GetDlgItem(IDC_ST_MESSAGE)->SetWindowText(m_pMsg);
	CenterWindow();

	return TRUE;  // return TRUE  unless you set the focus to a control
}
/////////////////////////////////////////////////////////////////////////////
// CErrorDlg dialog

CErrorDlg::CErrorDlg(LPCSTR msg,CWnd* pParent /*=NULL*/,LPCSTR pTitle /*=NULL*/)
	: CDialog(CErrorDlg::IDD, pParent)
{
	m_msg=msg;
	m_title=pTitle;
	//{{AFX_DATA_INIT(CErrorDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


BEGIN_MESSAGE_MAP(CErrorDlg, CDialog)
	ON_REGISTERED_MESSAGE(WM_RETFOCUS,OnRetFocus)
	//{{AFX_MSG_MAP(CErrorDlg)
	ON_BN_CLICKED(IDC_BUTTONOK, OnButtonOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg message handlers

static WNDPROC lpfnErrButtonProc=NULL;

LRESULT CALLBACK ButtonErrProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    if(msg==WM_KILLFOCUS) {
		::PostMessage(::GetParent(hWnd),WM_RETFOCUS,0,0);
    }
    return ::CallWindowProc(lpfnErrButtonProc,hWnd,msg,wParam,lParam);
}

BOOL CErrorDlg::OnInitDialog() 
{
	if(m_title) SetWindowText(m_title);
	GetDlgItem(IDC_ST_ERRMSG)->SetWindowText(m_msg);
	VERIFY(lpfnErrButtonProc=(WNDPROC)::SetWindowLong(GetDlgItem(IDC_BUTTONOK)->m_hWnd,
		GWL_WNDPROC,(DWORD)ButtonErrProc));
	MessageBeep(MB_OK);
	CDialog::OnInitDialog();
	CenterWindow();
	return TRUE;
}

LRESULT CErrorDlg::OnRetFocus(WPARAM bSubclass,LPARAM)
{
  EndDialog(0);  
  return TRUE;
}


void CErrorDlg::OnButtonOK() 
{
  EndDialog(0);
}
/////////////////////////////////////////////////////////////////////////////
// CMoveDlg dialog


BEGIN_MESSAGE_MAP(CMoveDlg, CDialog)
	//{{AFX_MSG_MAP(CMoveDlg)
	ON_BN_CLICKED(IDC_LINKTOFILES, OnLinkToFiles)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoveDlg message handlers

void CMoveDlg::OnLinkToFiles() 
{
	EndDialog(IDNO);
}

BOOL CMoveDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if(m_bCopy) GetDlgItem(IDOK)->SetWindowText("Copy Files");
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CMapHelpDlg dialog


CMapHelpDlg::CMapHelpDlg(LPCSTR title,CWnd* pParent /*=NULL*/)
	: CDialog(CMapHelpDlg::IDD, pParent)
{
	m_Title=title;
	//{{AFX_DATA_INIT(CMapHelpDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BEGIN_MESSAGE_MAP(CMapHelpDlg, CDialog)
	//{{AFX_MSG_MAP(CMapHelpDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapHelpDlg message handlers

BOOL CMapHelpDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetWindowText(m_Title);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CFileStatDlg dialog


CFileStatDlg::CFileStatDlg(LPCSTR pPathName,CWnd* pParent /*=NULL*/)
	: CDialog(CFileStatDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFileStatDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pPathName=pPathName;
}


BEGIN_MESSAGE_MAP(CFileStatDlg, CDialog)
	//{{AFX_MSG_MAP(CFileStatDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileStatDlg message handlers

BOOL CFileStatDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	long filesize;
	SYSTEMTIME *ptm;
	BOOL bReadOnly=FALSE;

 	GetDlgItem(IDC_ST_LOCATION)->SetWindowText(m_pPathName);
	if(ptm=GetLocalFileTime(m_pPathName,&filesize,&bReadOnly)) {
	   GetDlgItem(IDC_ST_UPDATED)->SetWindowText(GetTimeStr(ptm,&filesize));
	   GetDlgItem(IDC_READONLY)->ShowWindow(bReadOnly?SW_SHOW:SW_HIDE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CNotLoopDlg dialog


CNotLoopDlg::CNotLoopDlg(LPCSTR pMsg,LPCSTR pButtonMsg,CWnd* pParent /*=NULL*/)
	: CDialog(CNotLoopDlg::IDD, pParent)
{
	m_pButtonMsg=pButtonMsg;
	m_pMsg=pMsg;
	//{{AFX_DATA_INIT(CNotLoopDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BEGIN_MESSAGE_MAP(CNotLoopDlg, CDialog)
	//{{AFX_MSG_MAP(CNotLoopDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNotLoopDlg message handlers

BOOL CNotLoopDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GetDlgItem(IDC_ST_MESSAGE)->SetWindowText(m_pMsg);
	GetDlgItem(IDCANCEL)->SetWindowText(m_pButtonMsg);
	::MessageBeep(MB_ICONASTERISK);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg dialog

COpenExDlg::COpenExDlg(CString &msg,CWnd* pParent /*=NULL*/)
	: CDialog(COpenExDlg::IDD, pParent)
	, m_msg(msg)
{
	//{{AFX_DATA_INIT(COpenExDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BEGIN_MESSAGE_MAP(COpenExDlg, CDialog)
	//{{AFX_MSG_MAP(COpenExDlg)
	ON_BN_CLICKED(IDC_OPENEXISTING, OnOpenExisting)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL COpenExDlg::OnInitDialog() 
{
	GetDlgItem(IDC_ST_PATHNAME)->SetWindowText(m_msg);
	CDialog::OnInitDialog();
	//CenterWindow();
	return TRUE;
}

void COpenExDlg::OnOpenExisting() 
{
  EndDialog(IDC_OPENEXISTING);
}
// Dialogs.cpp : implementation file
//

// CDeleteDlg dialog

IMPLEMENT_DYNAMIC(CDeleteDlg, CDialog)
CDeleteDlg::CDeleteDlg(LPCSTR pName,CWnd* pParent /*=NULL*/)
	: CDialog(CDeleteDlg::IDD, pParent),m_pName(pName)
{
}

CDeleteDlg::~CDeleteDlg()
{
}

void CDeleteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDeleteDlg, CDialog)
	ON_BN_CLICKED(ID_DELETE_FILES, OnBnClickedDeleteFiles)
END_MESSAGE_MAP()


// CDeleteDlg message handlers

void CDeleteDlg::OnBnClickedDeleteFiles()
{
	EndDialog(IDYES);
}

BOOL CDeleteDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWndTitle(this,0,m_pName);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
