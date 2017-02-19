// scenedoc.cpp : implementation file
// written in December 1994
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#include "stdafx.h"
#include "hgapp.h"
#include "mainfrm.h"
#include "StatLink.h"

//#include "scene.h"
//#include "scviewer.h"
#include "scenedoc.h"
#include "scenevw.h"
#include "MDIScene.h"
#include "dde.h"
#include <ddeml.h>


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSceneApp

BEGIN_MESSAGE_MAP(CSceneApp, CApplication)
	//{{AFX_MSG_MAP(CSceneApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSceneApp construction

CSceneApp::CSceneApp()
{
  // for making integration easier we put code to an extra class...
  //scViewer_=new CSceneViewer;
}

CSceneApp::~CSceneApp()
{
  // so we created it, now its time to delete!
 // delete scViewer_;
}


/////////////////////////////////////////////////////////////////////////////
// The one and only CSceneApp object

CSceneApp theApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.
static const GUID BASED_CODE clsid =
	{ 0x0002180f, 0x0, 0x0, { 0xc0, 0xf0, 0x0, 0x4c, 0x37, 0x48, 0x0, 0x46 } };

/////////////////////////////////////////////////////////////////////////////
// CSceneApp initialization

bool CSceneApp::m_bCmdLineOpen=false;

BOOL CSceneApp::InitInstance()
{
	if(!FirstInstance()) return FALSE;

	// Initialize OLE libraries
/*	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}*/

  // initialize DDE
  /*m_pfnDDECallback=(PFNCALLBACK)MakeProcInstance((FARPROC) DDECallback,hInstance);

  int iErr=DdeInitialize(&m_iDDEInst,pfnDDECallback,CBF_FAIL_ALLSVRXACTIONS,0l);
  if(iErr!=DMLERR_NO_ERROR)
  {
    AfxMessageBox("Failed to initialize as DDE client.",MB_OK|MB_ICONSTOP);
    TRACE("Failed to initialize as DDE client. Error %d",iErr);
  }*/

  SetPrivateProfileName();

  if(!CApplication::InitInstance())
    return(FALSE);

	VERIFY(S_OK==CoInitializeEx(NULL,COINIT_APARTMENTTHREADED));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	LoadStdProfileSettings(6);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_SCENEVRMLTYPE,
		RUNTIME_CLASS(CSceneDoc),
		RUNTIME_CLASS(CMDISceneWnd),          // VRML child frame
		RUNTIME_CLASS(CSceneView));
	AddDocTemplate(pDocTemplate);

/*
  // the second one for the second input filter
	pDocTemplate = new CMultiDocTemplate(
		IDR_SCENESDFTYPE,
		RUNTIME_CLASS(CSceneDoc),
		RUNTIME_CLASS(CMDISceneWnd),          // SDF child frame
		RUNTIME_CLASS(CSceneView));
	AddDocTemplate(pDocTemplate);
*/

/*	// Connect the COleTemplateServer to the document template.
	//  The COleTemplateServer creates new documents on behalf
	//  of requesting OLE containers by using information
	//  specified in the document template.
	m_server.ConnectTemplate(clsid, pDocTemplate, FALSE);

	// Register all OLE server factories as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleTemplateServer::RegisterAll();
	// Note: MDI applications register all server objects without regard
	//   to the /Embedding or /Automation on the command line.
	// create main MDI Frame window*/

	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

/*	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes();

	// Parse the command line to see if launched as OLE server
	if (RunEmbedded() || RunAutomated())
	{
		// application was run with /Embedding or /Automation.  Don't show the
		//  main window in this case.
		return TRUE;
	}

	// When a server application is launched stand-alone, it is a good idea
	//  to update the system registry in case it has been damaged.
	m_server.UpdateRegistry(OAT_INPLACE_SERVER);

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();*/

  // In the Mac version we want the app frame maximized!
	if (m_lpCmdLine[0] != '\0')	OpenDocumentFile(m_lpCmdLine);

	 // call the encapsulated init code of CPSViewer
	// scViewer_->InitInstance();


	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

#ifdef _MAC
BOOL CSceneApp::CreateInitialDocument()
{
	// just do nothing here!
	//return CWinApp::CreateInitialDocument();
  return TRUE;
}
#endif

// Keep track of whether the class was registered so we can
// unregister it upon exit
static BOOL bClassRegistered = FALSE;
UINT id_MessageLoad = 0;
LPCTSTR lpszUniqueClass = _T("WALLS3DClass");

int CSceneApp::ExitInstance() 
{
  if(bClassRegistered)
      ::UnregisterClass(lpszUniqueClass,AfxGetInstanceHandle());
  // call the encapsulated exit code of CPSViewer
  //int scRet = scViewer_->ExitInstance();

  return CApplication::ExitInstance();

  //return scRet;
}


BOOL CSceneApp::FirstInstance()
{
	CWnd *pWndPrev, *pWndChild;
  
	if(!(id_MessageLoad=::RegisterWindowMessage("WALLS3D_MESSAGE")))
	{
      TRACE("Message Registration Failed\n");
      return FALSE;
	}

	// Determine if another window with our class name exists...
	if (pWndPrev = CWnd::FindWindow(lpszUniqueClass,NULL))
	{
		pWndChild = pWndPrev->GetLastActivePopup(); // if so, does it have any popups?

		if (pWndPrev->IsIconic()) 
		   pWndPrev->ShowWindow(SW_RESTORE);      // If iconic, restore the main window

		pWndChild->SetForegroundWindow();         // Bring the main window or it's popup to
												  // the foreground
		if(*m_lpCmdLine) {
			//AfxMessageBox(m_lpCmdLine);
			char path[MAX_PATH]; //for full path
			strncpy(path,m_lpCmdLine,MAX_PATH);
			path[MAX_PATH-1]=0;
			//int len=::GetFullPathName(m_lpCmdLine,MAX_PATH,path,NULL); //doesn't work if path is complete!!!
			//AfxMessageBox(path);
			COPYDATASTRUCT cds;
			cds.cbData=MAX_PATH;
			cds.dwData=15;
			cds.lpData=&path;
			if(!::SendMessage(pWndPrev->m_hWnd,WM_COPYDATA,0,(LPARAM)&cds)) {
				//ASSERT(FALSE);
				//target instance couldn't open file and displayed msg to that effect
			}
		}
		// and we are done activating the previous one.
		return FALSE;                             
	}
    // Register our unique class name that we wish to use
    WNDCLASS wndcls;

    memset(&wndcls, 0, sizeof(WNDCLASS));   // start with NULL defaults

    wndcls.style = 0; //CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;  //avoid flashing child windows!!!
    wndcls.lpfnWndProc = ::DefWindowProc;
    wndcls.hInstance = AfxGetInstanceHandle();
    wndcls.hIcon = LoadIcon(IDR_MAINFRAME); // or load a different icon
    wndcls.hCursor = LoadCursor( IDC_ARROW );
    wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wndcls.lpszMenuName = NULL;

    // Specify our own class name for using FindWindow later
    wndcls.lpszClassName = lpszUniqueClass;

    // Register new class and exit if it fails
    if(!AfxRegisterClass(&wndcls))
    {
      TRACE("Class Registration Failed\n");
      return FALSE;
    }
	
    bClassRegistered = TRUE;

    return TRUE;                              // First instance. Proceed as normal.
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

// Implementation
protected:
	CStaticLink m_textLink;		// static text
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
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
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	m_textLink.SubclassDlgItem(IDC_NEWURL,this);
	CDialog::OnInitDialog();
	m_textLink.SetLink("www.texasspeleologicalsurvey.org/software/walls/tsswalls.php");
	CenterWindow();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// App command to run the dialog
void CSceneApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CSceneApp commands

// 17.5.1995
// calls the actual view OnIdle function
BOOL CSceneApp::OnIdle(LONG lCount) 
{
	if(CWinApp::OnIdle(lCount))
	  return(TRUE);

  // call active view OnIdle()
  CMDIChildWnd* pMDIChildWnd=((CMDIFrameWnd*)m_pMainWnd)->MDIGetActive();
  if(pMDIChildWnd==NULL)
    // no active MDI child frame
    return(FALSE); 
  CSceneView* pView=(CSceneView*)pMDIChildWnd->GetActiveView();
  if(pView)
    return(pView->OnIdle(lCount));
  else
    return(FALSE);
}

//
// This bit of code catches control-c's, it cleans up everything.
//

int cleanupDone = 0;

void cleanup(int i)
{
    if (cleanupDone)
        _exit(i);
    cleanupDone = 1;

    theApp.ExitInstance();

    if (i)
       TRACE("Program stopped (caught signal %d).\n",i);
    exit(i);
}


extern "C" void _cleanup(int i)
{
    cleanup(i);
}


void CSceneApp::WinHelp(DWORD dwData, UINT nCmd) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CApplication::WinHelp(dwData, nCmd);
}

CDocument * CSceneApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
	CDocument *pDoc=CApplication::OpenDocumentFile(lpszFileName);
    if(pDoc && !m_bCmdLineOpen)((CMDIFrameWnd *)AfxGetMainWnd())->MDITile(MDITILE_VERTICAL); 
	m_bCmdLineOpen=false;
	return pDoc;
}
