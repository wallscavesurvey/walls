// walls.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"
#include "walls.h"
#include "prjframe.h"
#include "prjdoc.h"
#include "prjview.h"
#include "txtfrm.h"
#include "lineview.h"
#include "wedview.h"
#include "revframe.h"
#include "review.h"
#include "mapframe.h"
#include "plotview.h"
#include "mapview.h"
#include "segview.h"
#include "ImpSefDlg.h"
#include "ImpCssDlg.h"
#include "aboutdlg.h"
#include "TipDlg.h"
#include "GpsInDlg.h"
#include <direct.h>
#include <winspool.h>
#include <new.h>

#ifdef _DEBUG
#undef _TEST_FOCUS
#ifdef _TEST_FOCUS
VOID CALLBACK HighlightTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
#endif
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define MAX_HELP_CONTEXT 26
#define CONTENTS_HELP_CONTEXT 1000

/////////////////////////////////////////////////////////////////////////////
// CWallsApp

BEGIN_MESSAGE_MAP(CWallsApp, CWinApp)
	ON_COMMAND(CG_IDS_TIPOFTHEDAY, ShowTipOfTheDay)
	//{{AFX_MSG_MAP(CWallsApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_PROJECT_NEW, OnProjectNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_IMPORT_SEF, OnImportSef)
	ON_COMMAND(ID_IMPORT_CSS, OnImportCss)
	ON_COMMAND(ID_GPS_DOWNLOAD, OnGpsDownload)
	//}}AFX_MSG_MAP
	// Global help commands
#ifdef _USEHTML
	ON_COMMAND(ID_HELP_INDEX, OnHelpContents)
#else
	ON_COMMAND(ID_HELP_INDEX, CWinApp::OnHelpIndex)
#endif
	ON_COMMAND(ID_HELP,OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CWinApp::OnHelpIndex)
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWallsApp::OnFilePrintSetup)
	ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, &CWallsApp::OnOpenRecentFile)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

//Publics available to documents and views --
CMultiDocTemplate * CWallsApp::m_pPrjTemplate;
CMultiDocTemplate * CWallsApp::m_pSrvTemplate;
CMultiDocTemplate * CWallsApp::m_pRevTemplate;
CMultiDocTemplate * CWallsApp::m_pMapTemplate;
/////////////////////////////////////////////////////////////////////////////
// The one and only CWallsApp object

CWallsApp NEAR theApp;
LPCTSTR lpszUniqueClass = _T("Walls32Class");
static BOOL bClassRegistered = FALSE;

#ifndef _WIN32
/////////////////////////////////////////////////////////////////////////////
// Workarounds for MS's buggy implementation of malloc(), etc., which 
// incorrectly calls the new handler set by MFC, thereby throwing an
// exception upon failure instead of returning NULL.
#ifndef _DEBUG 
// The debug allocators throw exceptions no matter what the "new handler" is --
void * operator new( size_t nSize )
{
  void* p = malloc( nSize );
  if ( p == NULL )
      AfxThrowMemoryException();
  return p;
}
#endif

static int NewHandler(size_t n)
{
	return 0;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// INI file implementation

static char BASED_CODE szIniFileSection[] = "Most Recent Project";
static char BASED_CODE szIniFileEntry[] = "File";

CWallsApp::CWallsApp()
{
	 m_bDDEFileOpen=FALSE;
	 m_hWalls2Mutex = CreateMutex(NULL, TRUE, _T("Walls2Mutex"));
	 m_bFirstInstance = GetLastError() != ERROR_ALREADY_EXISTS;
	 m_nID_RecentFile=0;
}

void CWallsApp::UpdateIniFileWithDocPath(const char* pszPathName)
{
	WriteProfileString(szIniFileSection, szIniFileEntry, pszPathName);
}

#if 0
//Doesn't work as described!
BOOL CWallsApp::GetPrinterPageInfo(BOOL *pbLandscape,int *hres,int *vres,int *pxperinX,int *pxperinY)
{
    // Get default printer settings.

   CDC dc;
   CPrintDialog dlg(FALSE);

   if(!dlg.GetDefaults()) return FALSE;

   *pbLandscape=(dlg.GetDevMode()->dmOrientation==DMORIENT_LANDSCAPE); /*Fails*/
   VERIFY(dc.Attach(dlg.GetPrinterDC()));
   *hres=dc.GetDeviceCaps(HORZRES);
   *pxperinX=dc.GetDeviceCaps(LOGPIXELSX);
   *vres=dc.GetDeviceCaps(VERTRES);
   *pxperinY=dc.GetDeviceCaps(LOGPIXELSY);
   //VERIFY(::GlobalUnlock(dlg.m_pd.hDevNames)); /*Fails*/
   //VERIFY(::GlobalUnlock(dlg.m_pd.hDevMode));
   //dc destructor will delete the DC
   return TRUE;
}
#endif

BOOL CWallsApp::GetPrinterPageInfo(BOOL *pbLandscape,int *hres,int *vres,int *pxperinX,int *pxperinY)
{
    // Get default printer settings.
    PRINTDLG   pd;

    pd.lStructSize = (DWORD) sizeof(PRINTDLG);
    if(m_hDevNames && GetPrinterDeviceDefaults(&pd)) {
        // Lock memory handle.
        DEVMODE FAR* pDevMode = (DEVMODE FAR*)::GlobalLock(m_hDevMode);
        LPDEVNAMES lpDevNames;
        LPTSTR lpszDriverName, lpszDeviceName, lpszPortName;

        if(pDevMode) {
           // Change printer settings in here.
		   *pbLandscape=(pDevMode->dmOrientation==DMORIENT_LANDSCAPE);
           // Unlock memory handle.
		   lpDevNames = (LPDEVNAMES)GlobalLock(pd.hDevNames);
		   lpszDriverName = (LPTSTR )lpDevNames + lpDevNames->wDriverOffset;
		   lpszDeviceName = (LPTSTR )lpDevNames + lpDevNames->wDeviceOffset;
		   lpszPortName   = (LPTSTR )lpDevNames + lpDevNames->wOutputOffset;

		   CDC dc;
		   VERIFY(dc.CreateDC(lpszDriverName,lpszDeviceName,lpszPortName,pDevMode));
		   *hres=dc.GetDeviceCaps(HORZRES);
		   *pxperinX=dc.GetDeviceCaps(LOGPIXELSX);
		   *vres=dc.GetDeviceCaps(VERTRES);
		   *pxperinY=dc.GetDeviceCaps(LOGPIXELSY);
		   ::GlobalUnlock(m_hDevNames);
       }
	   ::GlobalUnlock(m_hDevMode);
	   return pDevMode!=NULL;
    }
	return FALSE;
}

void CWallsApp::GetDefaultFrameSize(double &fWidth,double &fHeight,BOOL bPrinter)
{
	BOOL bLandscape;
	int hres,vres,pxperinX,pxperinY;

	if(!bPrinter) {
		fWidth=CPlotView::m_mfPrint.fFrameWidthInches;
		fHeight=CPlotView::m_mfPrint.fFrameHeightInches;
	}
	if(bPrinter || fWidth<0.1 || fHeight<0.1) { 
		if(!((CWallsApp *)AfxGetApp())->GetPrinterPageInfo(&bLandscape,&hres,&vres,&pxperinX,&pxperinY) ||
			pxperinX<=0 || pxperinY<=0 ||
			(fWidth=((double)hres/pxperinX))<=1.1 ||
			(fHeight=((double)vres/pxperinY))<=1.1) {
			fWidth=9.5;
			fHeight=7.5;
		}
		else {
			fWidth=((int)((fWidth-1.0)/0.5))/2.0;
			fHeight=((int)((fHeight-1.0)/0.5))/2.0;
		}
	}
}

int CWallsApp::SetLandscape(int bLandscape)
{
    // Get default printer settings.
    PRINTDLG   pd;
	BOOL bRet=FALSE;

    pd.lStructSize = (DWORD) sizeof(PRINTDLG);
    if (GetPrinterDeviceDefaults(&pd)) {
        // Lock memory handle.
        DEVMODE FAR* pDevMode = (DEVMODE FAR*)::GlobalLock(m_hDevMode);
        LPDEVNAMES lpDevNames;
        LPTSTR lpszDriverName, lpszDeviceName, lpszPortName;
        HANDLE hPrinter;

        if(pDevMode) {
           // Change printer settings in here.
		   if(bLandscape>=0) pDevMode->dmOrientation = bLandscape?DMORIENT_LANDSCAPE:DMORIENT_PORTRAIT;
           // Unlock memory handle.
		   lpDevNames = (LPDEVNAMES)GlobalLock(pd.hDevNames);
		   lpszDriverName = (LPTSTR )lpDevNames + lpDevNames->wDriverOffset;
		   lpszDeviceName = (LPTSTR )lpDevNames + lpDevNames->wDeviceOffset;
		   lpszPortName   = (LPTSTR )lpDevNames + lpDevNames->wOutputOffset;

		   VERIFY(::OpenPrinter(lpszDeviceName, &hPrinter, NULL));
		   bRet=(IDOK==::DocumentProperties(NULL,hPrinter,lpszDeviceName,pDevMode,
                           pDevMode, DM_IN_BUFFER|DM_OUT_BUFFER));

		   if(!bRet && bLandscape<2) {
			   CMsgBox(MB_ICONEXCLAMATION,(bLandscape==-2?IDS_ERR_BADPRINTER1:IDS_ERR_NOPRINTER1),lpszDeviceName);
			   bRet=-1;
		   }

		   // Sync the pDevMode.
		   // See SDK help for DocumentProperties for more info.
		   ::ClosePrinter(hPrinter);
		   ::GlobalUnlock(m_hDevNames);
		   ::GlobalUnlock(m_hDevMode);
       }
    }
    if(!bRet && bLandscape<2) AfxMessageBox(IDS_ERR_NOPRINTER);

	return bRet;
}

void CWallsApp::SendFileOpenCmd(CWnd *pWnd,LPCSTR pCmd)
{
	ASSERT(m_hWalls2Mutex);
	if(pCmd && pCmd[0] && m_hWalls2Mutex) {
		//Send WM_COPYDATA command --
		if(WaitForSingleObject(m_hWalls2Mutex,6000) != WAIT_TIMEOUT) {
			COPYDATASTRUCT cd;
			cd.dwData=WALLS_FILEOPEN;
			cd.cbData=strlen(pCmd)+1;
			cd.lpData=(LPVOID)pCmd;
			pWnd->SendMessage(WM_COPYDATA,NULL,(LPARAM)&cd);
			VERIFY(ReleaseMutex(m_hWalls2Mutex));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWallsApp initialization
BOOL CWallsApp::FirstInstance()
{
	CWnd *pWndPrev, *pWndChild;

	// Determine if another window with our class name exists...
	if(pWndPrev = CWnd::FindWindow(lpszUniqueClass,NULL))
	{
		pWndChild = pWndPrev->GetLastActivePopup(); // if so, does it have any popups?

		if(pWndPrev->IsIconic()) pWndPrev->ShowWindow(SW_RESTORE);      // If iconic, restore the main window

		pWndChild->SetForegroundWindow();         // Bring the main window or it's popup to
												  // the foreground

		// and we are done activating the previous one.
		SendFileOpenCmd(pWndPrev,m_lpCmdLine);
		return FALSE;                             
	}

	ASSERT(m_bFirstInstance && m_hWalls2Mutex);

    // Register our unique class name that we wish to use
    WNDCLASS wndcls;

    memset(&wndcls, 0, sizeof(WNDCLASS));   // start with NULL defaults

    wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
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

    return bClassRegistered = TRUE; // First instance. Proceed as normal.
}


BOOL CWallsApp::InitInstance()
{
    // If a previous instance of the application is already running,
    // then activate it and return FALSE from InitInstance to end the
    // execution of this instance.
    if(!FirstInstance()) return FALSE;

    // Standard initialization
    // SetDialogBkColor();        // set dialog background color to gray
    
	// Register the application's document templates. We register the .SRV
	// type first so that CWinApp::OpenDocumentFile() will open all files
	// except *.WPJ as text files (CLineDoc) rather than as projects (CPrjDoc).
	// This strategy depends on MFC's "best match" algorithm for selecting
	// the best template for a given file name --

    m_pSrvTemplate=new CMultiDocTemplate(IDR_SRVTYPE,
            RUNTIME_CLASS(CLineDoc),
            RUNTIME_CLASS(CTxtFrame),        // MDI child frame
            RUNTIME_CLASS(CWedView));

	AddDocTemplate(m_pSrvTemplate);
            
	m_pPrjTemplate=new CMultiDocTemplate(IDR_PRJTYPE,
			RUNTIME_CLASS(CPrjDoc),
            RUNTIME_CLASS(CPrjFrame),
			RUNTIME_CLASS(CPrjView));
			
	m_pPrjTemplate->m_hMenuShared = m_pSrvTemplate->m_hMenuShared;
				
	AddDocTemplate(m_pPrjTemplate);
	
	m_pRevTemplate=new CMultiDocTemplate(IDR_REVTYPE,
			RUNTIME_CLASS(CPrjDoc),
            RUNTIME_CLASS(CRevFrame),    
			RUNTIME_CLASS(CReView));
			
	m_pRevTemplate->m_hMenuShared = m_pSrvTemplate->m_hMenuShared;			
	AddDocTemplate(m_pRevTemplate);
	
	m_pMapTemplate=new CMultiDocTemplate(IDR_MAPTYPE,
			RUNTIME_CLASS(CPrjDoc),
            RUNTIME_CLASS(CMapFrame),    
			RUNTIME_CLASS(CMapView));
			
	m_pMapTemplate->m_hMenuShared = m_pSrvTemplate->m_hMenuShared;			
	AddDocTemplate(m_pMapTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	m_pMainWnd = pMainFrame;

	dwOsMajorVersion=GetOsMajorVersion();

	if(dwOsMajorVersion<5) {
	   CMsgBox(MB_ICONINFORMATION, "Sorry, this program requires a later version of Windows (XP or above).");
	   return FALSE;
	}

#ifdef _USEHTML
	m_bWinHelp=FALSE;
	//***7.1
	#if _MFC_VER >= 0x0700
		EnableHtmlHelp();
	#endif
	strcpy(trx_Stpext(m_pszHelpFilePath),".chm");
	if(_access(m_pszHelpFilePath,0)==-1) {
		CMsgBox(MB_ICONINFORMATION,"NOTE: Online help is unavailable since %s is missing from the Walls program folder.", trx_Stpnam(m_pszHelpFilePath));
	}
#endif
	
	{
	  char *p=_strdup(m_pszHelpFilePath);
	  strcpy(p+strlen(p)-3,"INI");
	  m_pszProfileName=p;
	}
 	
	SetRegistryKey(_T("NZ Applications"));
    LoadStdProfileSettings(6);
    CPrjView::Initialize();    // Load saved options for project windows
    CWedView::Initialize();    // Load saved options for edit windows
    CReView::Initialize();
    CPlotView::Initialize();
	CSegView::Initialize();
    CCenterColorDialog::LoadCustomColors();

	mag_InitModel();
    
	//SetLandscape(2);            //Set the initial printer mode to landscape
    
#ifndef _WIN32
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME)) return FALSE;
#else
	// By turning off the default MFC-defined FWS_ADDTOTITLE style,
	// the framework will use first string in the document template
	// STRINGTABLE resource instead of the document name.
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME,WS_OVERLAPPEDWINDOW)) return FALSE;
#endif	
	pMainFrame->RestoreWindow(m_nCmdShow); //Slow under XP!
	pMainFrame->UpdateWindow();

	// enable file manager drag/drop and DDE Execute open
	pMainFrame->DragAcceptFiles();
	EnableShellOpen();
	RegisterShellFileTypes();

	// simple command line parsing
#ifdef _DEBUG
	//CMsgBox(MB_ICONEXCLAMATION,"%02x|%02x|%s|*",m_lpCmdLine[0],m_lpCmdLine[1],m_lpCmdLine);
#endif

#ifdef _TEST_FOCUS
	// Check the focus ten times a second
	// Change hwndMain to your main application window
	// Note that this won't work if you have multiple UI threads
	::SetTimer(pMainFrame->m_hWnd, 1, 100, HighlightTimerProc);
#endif

	if (*m_lpCmdLine=='\0' || m_lpCmdLine[1]=='"')
	{
		//return TRUE;
		// Try opening file name saved in private INI file.
		CString strDocPath = GetProfileString(szIniFileSection, szIniFileEntry, NULL);
		if (!strDocPath.IsEmpty() && !_access(strDocPath,0) && SetCurrentDir(strDocPath)) {
			CDocument *pDoc=OpenDocumentFile(strDocPath);
			if(pDoc && !stricmp(trx_Stpext(strDocPath),PRJ_PROJECT_EXT)) {
				UpdateIniFileWithDocPath(pDoc->GetPathName());
			}
		}
	}
	else if ((*m_lpCmdLine == '-' || *m_lpCmdLine == '/') &&
		(m_lpCmdLine[1] == 'e' || m_lpCmdLine[1] == 'E'))
	{
		// program launched embedded - wait for DDE or OLE open
	}
	else
	{
		//CMsgBox(MB_ICONEXCLAMATION,"CMD: %s",m_lpCmdLine);
		// open an existing document
		RunCommand(WALLS_FILEOPEN,m_lpCmdLine);
	}

	// CG: This line inserted by 'Tip of the Day' component.
	ShowTipAtStartup();

	if(m_hWalls2Mutex) ReleaseMutex(m_hWalls2Mutex);

	return TRUE;
}

int CWallsApp::ExitInstance()
{
    if(bClassRegistered) {
		CReView::Terminate();
		CPlotView::Terminate();
		CCenterColorDialog::SaveCustomColors();
		CPrjView::Terminate(); //Save project settings
		CWedView::Terminate(); //Save survey settings
		m_pPrjTemplate->m_hMenuShared = NULL;

#ifdef _USEHTMLREG
		UnregisterHTMLHelp();
#endif
		::UnregisterClass(lpszUniqueClass,AfxGetInstanceHandle());
		if(m_hWalls2Mutex && m_bFirstInstance) {
			VERIFY(CloseHandle(m_hWalls2Mutex));
		}
	}
	return CWinApp::ExitInstance(); // Saves MRU and returns the value from PostQuitMessage
}

/////////////////////////////////////////////////////////////////////////////
// App command to run the about dialog

void CWallsApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWallsApp commands
void CWallsApp::OnFileOpen()
{
    if(!PropOK()) return;

	CString strFilter;
	#define NUM_FILTERS 3
	
	AddFilter(strFilter,IDS_PRJ_FILES);
	AddFilter(strFilter,IDS_SRV_FILES);
    AddFilter(strFilter,AFX_IDS_ALLFILTER);
    
	CString pathName;
	if(!DoPromptPathName(pathName,
	  OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,
	  NUM_FILTERS,strFilter,TRUE,
	  IDS_PRJ_FILEOPEN,
	  PRJ_PROJECT_EXTW)) return;

    m_pMainWnd->UpdateWindow();
	OpenDocumentFile(pathName);
}

void CWallsApp::OnProjectNew()
{
	if(!CloseItemProp()) return;
	m_pPrjTemplate->OpenDocumentFile(NULL);
}

void CWallsApp::OnImportSef()
{
	CImpSefDlg dlg;

	CLineView::m_nLineStart=0;
	if(dlg.DoModal()!=IDOK || dlg.m_PrjPath.IsEmpty()) {
		if(CLineView::m_nLineStart) {
			m_pSrvTemplate->OpenDocumentFile(dlg.m_SefPath);
			CLineView::m_nLineStart=0;
		}
	}
	else {
		m_pMainWnd->UpdateWindow();
		m_pPrjTemplate->OpenDocumentFile(dlg.m_PrjPath);
	}
}

void CWallsApp::OnImportCss()
{
	CImpCssDlg dlg;

	CLineView::m_nLineStart=0;
	if(dlg.DoModal()!=IDOK || dlg.m_WpjPath.IsEmpty()) {
		if(CLineView::m_nLineStart) {
			CLineDoc *pDoc=CLineDoc::GetLineDoc(dlg.m_CssPath);
			if(pDoc) {
				pDoc->DisplayLineViews();
			}
			else {
				theApp.OpenDocumentFile(dlg.m_CssPath);
			}
			CLineView::m_nLineStart=0;
		}
	}
	else {
		m_pMainWnd->UpdateWindow();
		m_pPrjTemplate->OpenDocumentFile(dlg.m_WpjPath);
	}
}

bool CWallsApp::IsProjectOpen(LPCSTR pathName)
{
	if(FindOpenProject(pathName)) {
		CMsgBox(MB_ICONINFORMATION, "%s\n\nThis project is already open. You must choose a different name or location.", (LPCSTR)pathName);
		return true;
	}
	return false;
}

void CWallsApp::OnGpsDownload() 
{
	CGpsInDlg dlg;
	if(IDOK==dlg.DoModal()) {
	  m_pMainWnd->UpdateWindow();
	  m_pPrjTemplate->OpenDocumentFile(dlg.m_Pathname);
	}
}

void CWallsApp::AddToRecentFileList(const char* pszPathName)
{
	if(!stricmp(trx_Stpext(pszPathName),PRJ_PROJECT_EXTW))
		CWinApp::AddToRecentFileList(pszPathName);
}

#if _MFC_VER >= 0x0700
void CWallsApp::WinHelpInternal(DWORD_PTR dwData, UINT nCmd /*= HELP_CONTEXT*/)
{
	// return global app help mode state to FALSE (backward compatibility)
	m_bHelpMode = FALSE;
	m_pMainWnd->PostMessage(WM_KICKIDLE); // trigger idle update
	WinHelp(dwData,nCmd);
}
#endif

void CWallsApp::WinHelp(DWORD dwData,UINT nCmd)
{
		if(dwData>MAX_HELP_CONTEXT && (dwData<TIP_MINIDHELP || dwData>TIP_MAXIDHELP)) dwData=CONTENTS_HELP_CONTEXT;
#ifdef _USEHTML
		if(!m_bWinHelp) {
			if(nCmd!=HELP_CONTEXT) dwData=CONTENTS_HELP_CONTEXT;
			//***7.1 HtmlHelp(/*m_pMainWnd->m_hWnd*/NULL,m_pszHelpFilePath,HH_HELP_CONTEXT,dwData);
			HtmlHelp(dwData,HH_HELP_CONTEXT);
		}
		else
#endif
		CWinApp::WinHelp(dwData,nCmd);
}

void CWallsApp::ShowTipAtStartup(void)
{
	// CG: This function added by 'Tip of the Day' component.

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (cmdInfo.m_bShowSplash)
	{
		CTipDlg dlg;
		if (dlg.m_bStartup)	dlg.DoModal();
	}

}

void CWallsApp::ShowTipOfTheDay(void)
{
	// CG: This function added by 'Tip of the Day' component.
	CTipDlg dlg;
	dlg.DoModal();
}

CPrjDoc * CWallsApp::GetOpenProject(void)
{
	CDocument *pDoc;
	POSITION pos = m_pPrjTemplate->GetFirstDocPosition();
	while(pos) {
		pDoc=m_pPrjTemplate->GetNextDoc(pos);
		if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc))) return (CPrjDoc *)pDoc;
	}
	return NULL;
}

CPrjDoc * CWallsApp::FindOpenProject(LPCSTR pathname)
{
	CPrjDoc *pDoc;
	POSITION pos = m_pPrjTemplate->GetFirstDocPosition();
	while(pos) {
		pDoc=(CPrjDoc *)m_pPrjTemplate->GetNextDoc(pos);
		if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc))) {
			if(!_stricmp(pDoc->GetPathName(),pathname)) return pDoc;
		}
	}
	return NULL;
}

void CWallsApp::OnFilePrintSetup() 
{
    int nResult,bAvail;

	CPrintDialog pd(TRUE);

_retry:
	nResult=DoPrintDialog(&pd);

	//Due to a bug in MFC we could have nResult==IDCANCEL even though
	//a dialog wasn't displayed! (Network printer unavailable.)
	//SetLandscape(-1) returns >0 if default printer is available.

	if((bAvail=SetLandscape(-1))>0) {
		if(nResult!=IDCANCEL && CPrjDoc::m_bPageActive) CPrjDoc::PlotPageGrid(4);
	}
	else if(bAvail) {
		m_hDevNames=NULL;
		goto _retry;
	}
}

#ifdef _USEHTML
void CWallsApp::OnHelpContents()
{
	if(m_bWinHelp) CWinApp::OnHelpIndex();
	else WinHelp(CONTENTS_HELP_CONTEXT,HELP_CONTEXT);
}
#endif

LRESULT CWallsApp::RunCommand(int fcn,LPCSTR pData)
{
	if(fcn==WALLS_FILEOPEN) {
		CDocument *pDoc=NULL;
		CString cmd(pData);
		cmd.Remove('"');
		LPCSTR p=strstr(cmd," -L");
		if(p) {
		  cmd.SetAt(p-(LPCSTR)cmd,0);
		  CLineView::m_nLineStart=atoi(p+=3);
		  CLineView::m_nCharStart=(p=strstr(p," -C"))?(atoi(p+3)-1):0;
		}
		CPrjDoc *pPrjDoc;
		CPrjListNode *pNode;
		if((pNode=CPrjDoc::FindFileNode(&pPrjDoc,cmd,TRUE)) &&
			!(m_bDDEFileOpen && pNode->IsOther())) {
			//Don't open a PRJ file as text via DDE if it's attached to the tree --
			pDoc=pPrjDoc->OpenSurveyFile(pNode);
		}
		else {
			SetCurrentDir(cmd);
			CLineDoc::m_bReadOnlyOpen=_access(cmd,6)!=0;
			pDoc=OpenWallsDocument(cmd);
			//pDoc=CWinApp::OpenDocumentFile(cmd);
			CLineDoc::m_bReadOnlyOpen=FALSE;
			if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CLineDoc))) {
				((CLineDoc *)pDoc)->DisplayLineViews();
			}
		}
		CLineView::m_nLineStart=0;
		return (LRESULT)pDoc;
	}
	return FALSE;
}

BOOL CWallsApp::OnDDECommand(LPTSTR lpszCommand)
{
	//CMsgBox(MB_ICONEXCLAMATION,"DDE: %s",lpszCommand);
	m_bDDEFileOpen=TRUE;
	BOOL bRet=CWinApp::OnDDECommand(lpszCommand);
	m_bDDEFileOpen=FALSE;
	return bRet;
}

CDocument* CWallsApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
	CDocument *pDoc=NULL;
	//CMsgBox(MB_ICONEXCLAMATION,"DDE: %s, OpenDoc: %s",m_bDDEFileOpen?"TRUE":"FALSE",lpszFileName);
	if(m_bDDEFileOpen) {
		pDoc=(CDocument *)RunCommand(WALLS_FILEOPEN,lpszFileName);
		m_bDDEFileOpen=FALSE;
	}
	else {
		CLineDoc::m_bReadOnlyOpen=_access(lpszFileName,6)!=0;
//		pDoc=CWinApp::OpenDocumentFile(lpszFileName);
		pDoc=OpenWallsDocument(lpszFileName);
		CLineDoc::m_bReadOnlyOpen=FALSE;
	}
	return pDoc;
}

BOOL CWallsApp::OnOpenRecentFile(UINT nID)
{
	m_nID_RecentFile=nID;
	nID=CWinApp::OnOpenRecentFile(nID);
	m_nID_RecentFile=0;
	return nID;
}

CDocument* CWallsApp::OpenWallsDocument(LPCTSTR lpszFileName)
{
	LPSTR p=trx_Stpext(lpszFileName);
	char pathbuf[_MAX_PATH];
	if(!stricmp(p,PRJ_PROJECT_EXT) && p-lpszFileName<_MAX_PATH-4) {
		  FILE *fp=NULL;
		  if(_access(lpszFileName,6)!=0) {
			  if(errno==ENOENT) {
				  CMsgBox(MB_ICONEXCLAMATION,"%s --\nThis file no longer exists. Its extension may have been renamed to .WPJ",lpszFileName);
			  }
			  else CMsgBox(MB_ICONEXCLAMATION,"%s --\nThis file is protected and can't be accessed.",lpszFileName);
			  return NULL;
		  }

		  if(!(fp=fopen(lpszFileName,"r")) || !fgets(pathbuf,16,fp)) {
			  ASSERT(0);
			  p=NULL;
		  }
		  else if(!strstr(pathbuf,";WALLS") && !strstr(pathbuf,".BOOK")) {
			  CMsgBox(MB_ICONEXCLAMATION,"%s --\n\nAlthough this file has the .PRJ extension used by earlier versions of Walls, it has the "
				  "wrong format for a project script (extension .WPJ). It may be a projection file used by GIS software.",lpszFileName);
			  p=NULL;
		  }
		  else {
			  p=NULL;
			  UINT id=CMsgBox(MB_OKCANCEL,"%s --\n\nNOTE: The required extension for project script files has been changed from .PRJ to .WPJ to "
				  "distinguish them from the projection files created in shapefile exports. Select OK to open "
				  "this project while automatically changing the script's extension to .WPJ.",lpszFileName);
			  if(id==IDOK) {
				  fclose(fp);
				  fp=NULL;
				  p=strcpy(pathbuf,lpszFileName);
				  strcpy(trx_Stpext(p),PRJ_PROJECT_EXTW);
				  if(rename(lpszFileName,p)) {
					  CMsgBox(MB_ICONEXCLAMATION,"Could not rename the file. Check if %s already exists.",trx_Stpnam(p));
					  p=NULL;
				  }
				  else {
					  //Note the PRJ file name would remain in the MRU list if that was its origin --
					  if(m_nID_RecentFile>=(UINT)ID_FILE_MRU_FILE1 && m_nID_RecentFile<(UINT)ID_FILE_MRU_FILE1+m_pRecentFileList->GetSize()) {
					    ASSERT(!strcmp(lpszFileName,(LPCTSTR)(*m_pRecentFileList)[m_nID_RecentFile-ID_FILE_MRU_FILE1]));
						m_pRecentFileList->Remove(m_nID_RecentFile-ID_FILE_MRU_FILE1);
						m_nID_RecentFile=0;
					  }
				  }
			  }
		  }
		  if(fp) fclose(fp);
		  return p?CWinApp::OpenDocumentFile(p):NULL;
	}
	else {
		CLineDoc *pDoc=(CLineDoc *)CWinApp::OpenDocumentFile(lpszFileName);
		if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CLineDoc)))
			pDoc->UpdateOpenFileTitle(trx_Stpnam(lpszFileName),"");
		return pDoc;
	}
}

#ifdef _TEST_FOCUS
LRESULT CALLBACK HighlightWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_NCHITTEST:
		return HTTRANSPARENT;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

VOID CALLBACK HighlightTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	// static locals are bad
	static bool initialised = false;
	static HWND hwndHighlight = 0;

	if(!initialised)
	{
		HINSTANCE hInstance = 0;

		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style          = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc    = HighlightWndProc;
		wcex.cbClsExtra     = 0;
		wcex.cbWndExtra     = 0;
		wcex.hInstance      = hInstance;
		wcex.hIcon          = 0;
		wcex.hCursor        = 0;
		wcex.hbrBackground  = (HBRUSH)(COLOR_HIGHLIGHTTEXT);
		wcex.lpszMenuName   = 0;
		wcex.lpszClassName  = "HighlightWindowClasss";
		wcex.hIconSm        = 0;

		ATOM atomHighlightClass = RegisterClassEx(&wcex);

		hwndHighlight = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
			(LPCSTR)atomHighlightClass, "", WS_POPUP,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

		// Set opacity to 200/255
		SetLayeredWindowAttributes(hwndHighlight, 0, 200, LWA_ALPHA);

		initialised = true;
	}

	static HWND hwndCurrentHighlight = 0;

	HWND hwndFocus = GetFocus();
	if(hwndFocus != hwndCurrentHighlight)
	{
		if(hwndFocus == 0)
		{
			ShowWindow(hwndHighlight, SW_HIDE);
		}
		else
		{
			RECT rect;
			GetWindowRect(hwndFocus, &rect);
			MoveWindow(hwndHighlight, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, false);
			ShowWindow(hwndHighlight, SW_SHOW);
		}
		hwndCurrentHighlight = hwndFocus;
	}
}
#endif
