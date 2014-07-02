// walls2D.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <afxpriv.h>
#include "walls2D.h"

#include "MainFrm.h"
#include "walls2DDoc.h"
#include "walls2DView.h"

//#include <ocdb.h>
//#include <occimpl.h>
#include <afxocc.h>
#include <direct.h>
#include "custsite.h"
#include "idispimp.h"
#include "statlink.h"
#include <io.h>

#include <trx_str.h>

#define WALLS_URL "http://www.utexas.edu/depts/tnhc/.www/tss/Walls/tsswalls.htm"

#define ADOBE_URL "http://www.adobe.com/svg/viewer/install/main.html"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWalls2DApp

BEGIN_MESSAGE_MAP(CWalls2DApp, CWinApp)
	//{{AFX_MSG_MAP(CWalls2DApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	//ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	//ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWalls2DApp construction

CWalls2DApp::CWalls2DApp()
{
	m_pszDfltSVG=m_pszTempHTM=NULL;
	m_pDispOM=NULL;
}

CWalls2DApp::~CWalls2DApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWalls2DApp object

CWalls2DApp theApp;

BOOL CWalls2DApp::m_bCoordPage=FALSE;
BOOL CWalls2DApp::m_bPanning=FALSE;
BOOL CWalls2DApp::m_bNotHomed=FALSE;
CString CWalls2DApp::m_csTitle="";
CDocument *CWalls2DApp::m_pDocument=NULL;
/////////////////////////////////////////////////////////////////////////////
// CWalls2DApp initialization
BOOL CWalls2DApp::InitInstance()
{
	// Create a custom control manager class so we can overide the site
	CCustomOccManager *pMgr = new CCustomOccManager;

	// Create an IDispatch class for extending the Dynamic HTML Object Model 
	m_pDispOM = new CImpIDispatch;

	// Set our control containment up but using our control container 
	// management class instead of MFC's default
	AfxEnableControlContainer(pMgr);

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("NZ Applications"));

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	m_pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CWalls2DDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CWalls2DView));
	AddDocTemplate(m_pDocTemplate);

	{
	  char buf[MAX_PATH];
	  DWORD len=::GetTempPath(MAX_PATH,buf);
	  len=GetLongPathName(buf,buf,MAX_PATH);
	  if(len>MAX_PATH) {
		  ASSERT(0);
		  len=0; //shouldn't happen
	  }
	  strcpy(buf+len,"walls2d.htm"); //writeable temporary file!
	  m_pszTempHTM=_strdup(buf);

	  len = ::GetModuleFileName(m_hInstance, buf, MAX_PATH);
	  ASSERT( len && len != MAX_PATH );
	  LPSTR p=buf;
	  while(p=strstr(p,"\\.\\")) {
		  strcpy(p,p+2);
		  p++;
	  }
	  len=trx_Stpext(buf)-buf;
	  strcpy(buf+len,".svgz");
	  m_pszDfltSVG=_strdup(buf);

	  strcpy(buf+len,".ini");
	  m_pszProfileName=_strdup(buf); //shouldn have to free this
	}
 	
	LoadStdProfileSettings(6);  // Load standard INI file options (including MRU)

	CWalls2DView::Initialize();

	m_wallsLink=GetProfileString("Url","Walls",NULL);
	if(m_wallsLink.IsEmpty()) m_wallsLink=WALLS_URL;

	m_adobeLink=GetProfileString("Url","Adobe",NULL);
	if(m_adobeLink.IsEmpty()) m_adobeLink=ADOBE_URL;

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
	CUpLnkDlg(LPCSTR pWallsURL,LPCSTR pAdobeURL, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUpLnkDlg)
	enum { IDD = IDD_UPDATELINK };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CString m_WallsURL,m_AdobeURL;

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

CUpLnkDlg::CUpLnkDlg(LPCSTR pWallsURL,LPCSTR pAdobeURL, CWnd* pParent /*=NULL*/)
	: CDialog(CUpLnkDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUpLnkDlg)
	//}}AFX_DATA_INIT
	m_WallsURL = pWallsURL;
	m_AdobeURL = pAdobeURL;
}

void CUpLnkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUpLnkDlg)
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_WALLS_URL, m_WallsURL);
	DDV_MaxChars(pDX, m_WallsURL, 128);
	DDX_Text(pDX, IDC_ADOBE_URL, m_AdobeURL);
	DDV_MaxChars(pDX, m_AdobeURL, 128);
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
	CEdit *ce=(CEdit *)GetDlgItem(IDC_ADOBE_URL);
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
	CStaticLink m_wallsLink;		// static text
	CStaticLink m_adobeLink;	// static text
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
void CWalls2DApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWalls2DApp message handlers

int CWalls2DApp::ExitInstance() 
{
	CWalls2DView::Terminate();
	free(m_pszDfltSVG);
	free(m_pszTempHTM);
	delete m_pDispOM;
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
	m_wallsLink.SubclassDlgItem(IDC_WALLS_URL,this);
	m_adobeLink.SubclassDlgItem(IDC_ADOBE_URL,this);
	CDialog::OnInitDialog();

	CString str;
	str.LoadString(IDS_ABOUT_TEXT);
	GetDlgItem(IDC_WALLS2D_INFO)->SetWindowText(str);

	CenterWindow();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAboutDlg::OnLinkUpdate() 
{
	CUpLnkDlg dlg(theApp.m_wallsLink,theApp.m_adobeLink);

	if(IDOK==dlg.DoModal()) {
		theApp.m_wallsLink=dlg.m_WallsURL;
		VERIFY(AfxGetApp()->WriteProfileString("Url","Walls",dlg.m_WallsURL));
		theApp.m_adobeLink=dlg.m_AdobeURL;
		VERIFY(AfxGetApp()->WriteProfileString("Url","Adobe",dlg.m_AdobeURL));
	}
}

BOOL CAboutDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	BOOL bRet=CDialog::OnCommand(wParam, lParam);
	if(bRet && (WORD)wParam==IDC_WALLS_URL) EndDialog(IDOK);
	return bRet;
}

LPCSTR CWalls2DApp::GetRecentFile(UINT nIndex)
{
	CString & cPath=(*m_pRecentFileList)[nIndex];
	return cPath.IsEmpty()?NULL:(LPCSTR)cPath;
}

LPCSTR CWalls2DApp::GetRecentFile()
{
	LPCSTR pPath=GetRecentFile(0);
	while(pPath && _access(pPath,4)==-1) {
		m_pRecentFileList->Remove(0);
		pPath=GetRecentFile(0);
	}
	return pPath;
}

BOOL CWalls2DApp::OnOpenRecentFile(UINT nID)
{
	//ASSERT_VALID(this);
	//ASSERT(m_pRecentFileList != NULL);

	//ASSERT(nID >= ID_FILE_MRU_FILE1);
	//ASSERT(nID < ID_FILE_MRU_FILE1 + (UINT)m_pRecentFileList->GetSize());

	int nIndex = nID - ID_FILE_MRU_FILE1;
	LPCSTR pPath=GetRecentFile(nIndex);

	ASSERT(pPath);

	if(_access(pPath,4)==-1) {
		 AfxMessageBox("The selected recent file is no longer accessible.");
		 m_pRecentFileList->Remove(nIndex);
		 return TRUE;
	}

	CWalls2DView *pView=(CWalls2DView *)((CMainFrame*)AfxGetMainWnd())->GetActiveView();

	if(pView) {
		if(!pView->OpenSVG(pPath)) {
			m_pRecentFileList->Remove(nIndex);
		}
	}

	return TRUE;
}

void CWalls2DApp::Navigate2Links(UINT id)
{
	LPCSTR pLink;

	if(id==IDC_ADOBE_URL) pLink=(LPCSTR)m_adobeLink;
	else pLink=(LPCSTR)m_wallsLink;

	if(!*pLink) return;

	// Call ShellExecute to run the file.
	// For an URL, this means opening it in the browser.
	//
	HINSTANCE h = ShellExecute(NULL, "open", pLink, NULL, NULL, SW_SHOWNORMAL);
	if ((UINT)h <= 32) MessageBeep(0);	// unable to execute file!
}

