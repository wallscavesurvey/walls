// reselect.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "reselect.h"

#include "MainFrm.h"
#include "reselectDoc.h"
#include "reselectVw.h"

//#include <..\src\occimpl.h>
#include <afxocc.h>

#include <direct.h>
#include "custsite.h"
#include "statlink.h"

#define TSS_URL "http://www.utexas.edu/tmm/sponsored_sites/tss/"
#define TEXBIB_URL "http://www.utexas.edu/tmm/sponsored_sites/tss/TexBib/"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReselectApp

BEGIN_MESSAGE_MAP(CReselectApp, CWinApp)
	//{{AFX_MSG_MAP(CReselectApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	//ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	//ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	//ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReselectApp construction

CReselectApp::CReselectApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

CReselectApp::~CReselectApp()
{
	if ( m_pDispOM != NULL)
	{
		delete m_pDispOM;
	}
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CReselectApp object

CReselectApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CReselectApp initialization

BOOL CReselectApp::InitInstance()
{
	/*
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
	*/

	// Create a custom control manager class so we can overide the site
	CCustomOccManager *pMgr = new CCustomOccManager;

	// Create an IDispatch class for extending the Dynamic HTML Object Model 
	m_pDispOM = new CImpIDispatch;

	// Set our control containment up but using our control container 
	// management class instead of MFC's default
	AfxEnableControlContainer(pMgr);

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("NZ Applications"));

	//For now, don't maintain MRU file list --
	LoadStdProfileSettings(0);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	m_pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CReselectDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CReselectVw));
	AddDocTemplate(m_pDocTemplate);

	{
	  char *p=_strdup(m_pszHelpFilePath);
	  strcpy(p+strlen(p)-3,"INI");
	  m_pszProfileName=p;
	}
 	
	if(!_getcwd(m_szDBFilePath,_MAX_PATH)) strcpy(m_szDBFilePath,m_pszProfileName);
	else if(m_szDBFilePath[strlen(m_szDBFilePath)-1]!='\\') strcat(m_szDBFilePath,"\\");

	CReselectVw::Initialize();
	m_link=GetProfileString("Url","TSS",NULL);
	if(m_link.IsEmpty()) m_link=TSS_URL;

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	m_nCmdShow=SW_HIDE;
	if (!ProcessShellCommand(cmdInfo)) return FALSE;

	((CMainFrame *)m_pMainWnd)->RestoreWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CUpLnkDlg : public CDialog
{
// Construction
public:
	CUpLnkDlg(LPCSTR pURL, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUpLnkDlg)
	enum { IDD = IDD_UPDATELINK };
	CString m_NewURL;
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUpLnkDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CUpLnkDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CUpLnkDlg dialog

CUpLnkDlg::CUpLnkDlg(LPCSTR pURL,CWnd* pParent /*=NULL*/)
	: CDialog(CUpLnkDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUpLnkDlg)
	m_NewURL = pURL;
	//}}AFX_DATA_INIT
}

void CUpLnkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUpLnkDlg)
	DDX_Text(pDX, IDC_NEWURL, m_NewURL);
	DDV_MaxChars(pDX, m_NewURL, 128);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CUpLnkDlg, CDialog)
	//{{AFX_MSG_MAP(CUpLnkDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpLnkDlg message handlers

BOOL CUpLnkDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	CEdit *ce=(CEdit *)GetDlgItem(IDC_NEWURL);
	ce->SetSel(0,-1);
	ce->SetFocus();
	return FALSE;  // return TRUE  unless you set the focus to a control
}

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CStaticLink m_textLink;		// static text
	//{{AFX_MSG(CAboutDlg)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL OnInitDialog();
	afx_msg void OnLinkUpdate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_BN_CLICKED(IDC_LINKUPDATE, OnLinkUpdate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CReselectApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CReselectApp message handlers


int CReselectApp::ExitInstance() 
{
	CReselectVw::Terminate();
	return CWinApp::ExitInstance();
}

void CAboutDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	EndDialog(IDOK);
}

void CAboutDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	EndDialog(IDOK);
}

BOOL CAboutDlg::OnInitDialog() 
{
	GetDlgItem(IDC_ST_TEXDATE)->SetWindowText(theApp.m_texbib_date);
	GetDlgItem(IDC_ST_NWDATE)->SetWindowText(theApp.m_nwbb_date);
	m_textLink.SubclassDlgItem(IDC_TSS_URL,this);
	CDialog::OnInitDialog();
	CenterWindow();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAboutDlg::OnLinkUpdate() 
{
	CUpLnkDlg dlg(theApp.m_link);

	if(IDOK==dlg.DoModal()) {
		theApp.m_link=dlg.m_NewURL;
		VERIFY(AfxGetApp()->WriteProfileString("Url","TSS",dlg.m_NewURL));
	}
}

BOOL CAboutDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	BOOL bRet=CDialog::OnCommand(wParam, lParam);
	if(bRet && (WORD)wParam==IDC_TSS_URL) EndDialog(IDOK);
	return bRet;
}

void CReselectApp::ExecuteUrl(LPCSTR url)
{
	DWORD flags;
	if(InternetGetConnectedState(&flags,0)) {
		//bool b=TestURL(url); //doesn't fail even if file is missing
		//ASSERT(b);
		VERIFY((UINT)ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL)>32);
	}
	else AfxMessageBox(IDS_INET_NOTCONNECTED);
}

/*
bool CReselectApp::TestURL(LPCSTR url)
{
	char szBuffer[80];

	CInternetSession session;

	CInternetFile* file=NULL;

	try
	{
		file=(CInternetFile *)session.OpenURL(url);
	}
	catch(...)
	{
		if(file) file->Close();
		file = NULL;
	}

	bool bRet=file!=NULL;
    if(bRet) {
		DWORD dwLen = sizeof(szBuffer);
		bRet=HttpQueryInfo(file, HTTP_QUERY_STATUS_CODE,szBuffer, &dwLen, NULL)!=0;
		if (bRet && atoi(szBuffer) < 300 )
		{
			bRet=false;
		}
		file->Close();
	}
	session.Close(); 
	return bRet;
}
*/