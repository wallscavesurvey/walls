// WallsMap.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
//#include <vld.h>
#include "afxpriv.h" //for CRecentFileList
#include "WallsMap.h"
//#include "Wallmag.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "GPSPortSettingDlg.h"
#include "GdalLayer.h"
#include "NtiLayer.h"
#include "ShpLayer.h"
#include "aboutdlg.h"
#include <io.h>

//D2D globals --

D2D1_RENDER_TARGET_PROPERTIES d2d_props = D2D1::RenderTargetProperties(
	D2D1_RENDER_TARGET_TYPE_DEFAULT,  //hardware if avaiable (not for WIC targets)
	D2D1::PixelFormat(
	DXGI_FORMAT_B8G8R8A8_UNORM,
	D2D1_ALPHA_MODE_IGNORE), //supports cleartype text (can also be D2D1_ALPHA_MODE_PREMULTIPLIED for GDI compat)
	0.0f,
	0.0f,
	//D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE, //necessary?
	D2D1_RENDER_TARGET_USAGE_NONE,
	D2D1_FEATURE_LEVEL_DEFAULT
	);

ID2D1StrokeStyle *d2d_pStrokeStyleRnd = NULL;
ID2D1Factory* d2d_pFactory = NULL;

// CWallsMapApp

BEGIN_MESSAGE_MAP(CWallsMapApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	// ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	// ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16,OnOpenRecentFile)
END_MESSAGE_MAP()


// CWallsMapApp construction

LPCTSTR lpszUniqueClass = _T("WallsMapClass");

CWallsMapApp::CWallsMapApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

//Publics available to documents and views --
CMultiDocTemplate * CWallsMapApp::m_pDocTemplate;

// The one and only CWallsMapApp object
CWallsMapApp theApp;
BOOL CWallsMapApp::m_bBackground=FALSE;
BOOL CWallsMapApp::m_bNoAddRecent=FALSE;

// CWallsMapApp initialization

#ifdef _CHK_COMMANDLINE
void CWallsMapApp::ParseCommandLine0(CCommandLineInfo& rCmdInfo)
{
	for (int i = 1; i < __argc; i++)
	{
		LPCTSTR pszParam = __targv[i];
		//CMsgBox("Param %d: %s",i,pszParam);
		BOOL bFlag = FALSE;
		BOOL bLast = ((i + 1) == __argc);
		if (pszParam[0] == '-' || pszParam[0] == '/')
		{
			// remove flag specifier
			++pszParam;
			if(!strcmp(pszParam,"TS_OPT")) {
				//CMsgBox("OK!");
				bTS_OPT=true;
				continue;
			}
			bFlag = TRUE;
		}
		rCmdInfo.ParseParam(pszParam, bFlag, bLast);
	}
	//CMsgBox("rCmdInfo.m_strFileName: %s",(LPCSTR)rCmdInfo.m_strFileName);
}
#endif

static const TCHAR szRecentMapSection[] = "Recent Map";
static const TCHAR szRecentMapFile[] = "File";

void CWallsMapApp::UpdateRecentMapFile(const char* pszPathName)
{
	WriteProfileString(szRecentMapSection, szRecentMapFile, pszPathName);
}

void CWallsMapApp::WinHelpInternal(DWORD_PTR dwData, UINT nCmd)
{
	if(dwData>=HID_BASE_RESOURCE)
		return;

	CWnd* pMainWnd = AfxGetMainWnd();
	ENSURE_VALID(pMainWnd);

	// return global app help mode state to FALSE (backward compatibility)
	m_bHelpMode = FALSE;
	pMainWnd->PostMessage(WM_KICKIDLE); // trigger idle update
	pMainWnd->WinHelpInternal(dwData, nCmd);
}

void CWallsMapApp::WinHelp(DWORD dwData,UINT nCmd)
{
	//HtmlHelp(nCmd == HELP_CONTEXT ? dwData : 0,HH_HELP_CONTEXT);
	if(dwData==0) return;
	ASSERT(nCmd == HELP_CONTEXT);
	HtmlHelp(dwData,HH_HELP_CONTEXT);
}

BOOL CWallsMapApp::RegisterWallsMapClass()
{
    WNDCLASS wndcls;

    memset(&wndcls, 0, sizeof(WNDCLASS));   // start with NULL defaults

    wndcls.style = CS_DBLCLKS; //| CS_HREDRAW | CS_VREDRAW; //avoids severe flicker of the view when resizing!!!!! (2012-09-30)
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
	return TRUE;
}


static CWnd *IsFileInUse(LPCSTR pszFile)
{
	USES_CONVERSION; //required for T2COLE macro
	CWnd *pWnd=NULL;

	IRunningObjectTable *prot;
	HRESULT hr = GetRunningObjectTable(0, &prot);
	if (SUCCEEDED(hr))
	{
		IMoniker *pmkFile;
		hr = CreateFileMoniker(T2COLE(pszFile), &pmkFile);
		if (SUCCEEDED(hr))
		{
			IEnumMoniker *penumMk;
			hr = prot->EnumRunning(&penumMk);
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;

				ULONG celt;
				IMoniker *pmk;
				while (FAILED(hr) && (penumMk->Next(1, &pmk, &celt) == S_OK))
				{
					DWORD dwType;
					if (SUCCEEDED(pmk->IsSystemMoniker(&dwType)) && 
						(dwType == MKSYS_FILEMONIKER))
					{
						// Is this a moniker prefix?
						IMoniker *pmkPrefix;
						if (SUCCEEDED(pmkFile->CommonPrefixWith(pmk, &pmkPrefix)))
						{
							if (S_OK == pmkFile->IsEqual(pmkPrefix))
							{
								// Get the IFileIsInUse instance
								IUnknown *punk;
								if (prot->GetObject(pmk, &punk) == S_OK)
								{
								    IFileIsInUse *pfiu;
									hr = punk->QueryInterface(IID_PPV_ARGS(&pfiu));
									punk->Release();
									if(SUCCEEDED(hr)) {
										LPWSTR pszName;
										hr=pfiu->GetAppName(&pszName);
										if(SUCCEEDED(hr)) {
											CString sApp(OLE2T(pszName));
											if(sApp.GetLength()>=8 && !memcmp((LPCSTR)sApp,"WallsMap",8)) {
												DWORD dwCap=0;
												hr=pfiu->GetCapabilities(&dwCap);
												if(SUCCEEDED(hr) && (dwCap&OF_CAP_CANSWITCHTO)) {
													HWND hwnd;
													if(S_OK==pfiu->GetSwitchToHWND(&hwnd)) {
														pWnd=CWnd::FromHandle(hwnd);
													}
												}
											}
										}
										pfiu->Release();
										pmkPrefix->Release();
										pmk->Release();
										break;
									}
								}
							}
							pmkPrefix->Release();
						}
					}
					pmk->Release();
				}
				penumMk->Release();
			}
			pmkFile->Release();
		}
		prot->Release();
	}
	return pWnd;
}

BOOL CWallsMapApp::InitInstance()
{
#ifdef _DEBUG
#if 0
	//Find cause of memory leak --
   _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); //not required?
    //Look at block numbers of memory leaks in output window --
    //heap allocation no. 1962 wasn't released on last run, so set a breakpoint
    //in heap manager at that allocation.
    //_CrtSetBreakAlloc(1962); //1028-byte pixman context never freed, so leave alone
    //_CrtSetBreakAlloc(2567); //24 bytes -- gdal cpl warn/error msg memory leaks (fixed)
    //etc.
#endif
#endif

	VERIFY(RegisterWallsMapClass());
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	VERIFY(AfxInitRichEdit2());

	VERIFY(S_OK==CoInitializeEx(NULL,COINIT_APARTMENTTHREADED));

	// Initialize OLE 2.0 libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_AFXOLEINIT_FAILED);
		return FALSE;
	}

	if (wOSVersion<0x600 || !EnableD2DSupport()) {
		CMsgBox("Sorry, Windows Vista/7/8+ with Direct2D support is required to run this build of WallsMap.");
		return FALSE;
	}

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;

	#ifdef _CHK_COMMANDLINE
		ParseCommandLine0(cmdInfo);
	#else
		ParseCommandLine(cmdInfo);
	#endif

	if(!cmdInfo.m_strFileName.IsEmpty() && !_stricmp(trx_Stpext(cmdInfo.m_strFileName),NTL_EXT)) {
	    //If file is already open in a WallsMap instance, activate that instance and return --
		CWnd *pWnd=IsFileInUse(cmdInfo.m_strFileName);
		if(pWnd) {
			if(pWnd->IsIconic()) pWnd->ShowWindow(SW_RESTORE);      // If iconic, restore the main window
			pWnd->SetForegroundWindow(); // Bring the main window or it's popup to the foreground
			return FALSE;
		}
	}

	ASSERT(!d2d_pStrokeStyleRnd && !d2d_pFactory);
	d2d_pFactory = AfxGetD2DState()->GetDirect2dFactory();

	if (d2d_pFactory->CreateStrokeStyle(
		D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_ROUND, // default is flat, which would create broken line joins when using DrawLine()
		D2D1_CAP_STYLE_ROUND,
		D2D1_CAP_STYLE_FLAT,  // not relevant since solid
		D2D1_LINE_JOIN_ROUND, // not relevant if using DrawLine()
		2.0f,
		D2D1_DASH_STYLE_SOLID,
		0.0f),
		NULL,
		0,
		&d2d_pStrokeStyleRnd) < 0)
	{
		ASSERT(0); return FALSE;
	}

	CWinApp::InitInstance(); //does nothing in this case

    // Change extension for help file
	{
		CString strHelpFile = m_pszHelpFilePath;
		strHelpFile.Replace(".HLP", ".chm");
		free((void*)m_pszHelpFilePath);
		m_pszHelpFilePath = _tcsdup(strHelpFile);
		m_eHelpType = afxHTMLHelp; //New requirement as indicated be wincore.cpp assert!
	}
	//CMsgBox("version=%x",wOSVersion);

	/*
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
	*/

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization

	SetRegistryKey(_T("NZ Applications"));
	LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)
	CMainFrame::LoadPreferences();

	CWallsMapDoc::m_bOpenAsEditable=(CMainFrame::IsPref(PRF_OPEN_EDITABLE));

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	m_pDocTemplate = new CMultiDocTemplate(IDR_WallsMapTYPE,
		RUNTIME_CLASS(CWallsMapDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CWallsMapView));

	if (!m_pDocTemplate) return FALSE;
	AddDocTemplate(m_pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	pMainFrame->ClearStatusPanes();

	CShpLayer::Init();
	CNtiLayer::Init();

	//m_hCursorHand=LoadCursor(MAKEINTRESOURCE(AFX_IDC_TRACK4WAY)); //fails!??
	m_hCursorZoomIn=LoadCursor(IDC_CURSOR_ZOOMIN);
	m_hCursorZoomOut=LoadCursor(IDC_CURSOR_ZOOMOUT);
	m_hCursorHand=LoadCursor(IDC_CURSOR_HAND);
	m_hCursorSelect=LoadCursor(IDC_CURSOR_SELECT);
	m_hCursorSelectAdd=LoadCursor(IDC_CURSOR_SELECTADD);
	m_hCursorMeasure=LoadCursor(IDC_MEASURE);
	m_hCursorCross=LoadStandardCursor(IDC_CROSS);
	m_hCursorArrow=LoadStandardCursor(IDC_ARROW);

	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd

	// The main window has been initialized, so show and update it
	((CMainFrame *)m_pMainWnd)->RestoreWindow(m_nCmdShow);
	//pMainFrame->ShowWindow(m_nCmdShow);

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.

	if(CMainFrame::IsPref(PRF_OPEN_LAST) && cmdInfo.m_strFileName.IsEmpty())
	{
		CString strDocPath = GetProfileString(szRecentMapSection,szRecentMapFile,NULL);
		if (!strDocPath.IsEmpty() && !_access(strDocPath,0)) {
			if(_stricmp(trx_Stpext(strDocPath),NTL_EXT) || !IsFileInUse(strDocPath))
				OpenDocumentFile(strDocPath);
		}
	}
	else {
		if(!ProcessShellCommand(cmdInfo)) {
			//return FALSE;
		}
	}
	pMainFrame->UpdateWindow();
	// enable file manager drag/drop and DDE Execute open
	//pMainFrame->DragAcceptFiles(TRUE);
	return TRUE;
}

int CWallsMapApp::ExitInstance()
{
	CNtiLayer::UnInit();
	CGdalLayer::UnInit();
	CShpLayer::UnInit();
	CMainFrame::SavePreferences();
	CoUninitialize();
#if 0
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
#endif
	return CWinApp::ExitInstance(); // Saves MRU and returns the value from PostQuitMessage
}

// App command to run the dialog
void CWallsMapApp::OnAppAbout()
{
#ifdef NTI_TIMING
	CString s;
	s.Format("Tile decoding timer -\nSelect YES to enable, NO to disable, CANCEL to clear.\n"
		"Note: Timer is currently %sabled.",nti_inf_timing?"en":"dis");
	if(nti_tm_inf_cnt) {
		double secs=GetPerformanceSecs(&nti_tm_inf);
		s.AppendFormat("\n\nDecoding time: %.3f (%u tiles), secs/tile: %.4f",secs,nti_tm_inf_cnt,secs/nti_tm_inf_cnt);
	}
	UINT id=AfxMessageBox(s,MB_YESNOCANCEL);
	if(id==IDCANCEL) {
		nti_tm_inf=0;
		nti_tm_inf_cnt=0;
		CAboutDlg aboutDlg;
		aboutDlg.DoModal();
	}
	else {
		nti_inf_timing=(id==IDYES);
	}

#elif POLY_TIMIMG
	static bool nti_timing=false;
	UINT id=CMsgBox(MB_YESNOCANCEL,"Timer - Select YES to enable, NO to disable, CANCEL to show and clear.\n"
		"Note: Timer is currently %sabled.",nti_timing?"en":"dis");
	if(id==IDCANCEL) {
		CMsgBox("Timings --\nOpening: %.3f\nCopyPoly: %.3f\nFileIO part: %.3f\nClipPoly: %.3f\nInsidePoly: %.3f\nDrawPoly: %.3f"
			"\nMax PolyPts: %d",
			NTI_SECS(0),NTI_SECS(1),NTI_SECS(2),NTI_SECS(3),NTI_SECS(4),NTI_SECS(5),nti_tmSz);
		CLR_NTI_ALL();
		nti_tmSz=0;
	}
	else if(id==IDYES) {
		nti_timing=true;
		nti_tmSz=0;
	}
	else nti_timing=false;
#elif defined(LYR_TIMING)
	static bool nti_timing=false;
	UINT id=CMsgBox(MB_YESNOCANCEL,"Timer - Select YES to enable layer timing, NO to disable, CANCEL to show and clear.\n"
		"Note: Timer is currently %sabled.",bLYRTiming?"en":"dis");
	if(id==IDCANCEL) {
		CMsgBox("Timings --\nOverAll Rendering: %.3f\nCulling: %.3f\nLayers skipped: %u",NTI_SECS(1),NTI_SECS(2),nLayersSkipped);
		CLR_NTI_TM(1);CLR_NTI_TM(2);
		nLayersSkipped=0;
	}
	else if(id==IDYES) {
		bLYRTiming=true;
	}
	else bLYRTiming=false;

#elif defined(_USE_TM)
	//unsigned __int64 sum;
	//SUM_NTI_ALL(sum);
	//nti_tm[8] = sum;
	//CMsgBox("Times: Polylines=%.2f Polygons=%.2f TOTAL: %.2f",NTI_SECS(0), NTI_SECS(6), NTI_SECS(8));
	CMsgBox("Times: Go=%.2f AppendR=%.2f AppendB=%.2f FldLoop=%.2f Ext=%.2f memLoop=%.2f memFlush=%.2f TOTAL: %.2f",
		NTI_SECS(0), NTI_SECS(1), NTI_SECS(2), NTI_SECS(3), NTI_SECS(4), NTI_SECS(5), NTI_SECS(6), NTI_SECS(8));
	CLR_NTI_ALL();
#else
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
#endif
}

BOOL CWallsMapApp::OnOpenRecentFile(UINT nID)
{
	//Modified version of same fcn in appui.cpp. Notifies user of absent MRU file
	ASSERT_VALID(this);
	ASSERT(m_pRecentFileList != NULL);

	ASSERT(nID >= ID_FILE_MRU_FILE1);
	ASSERT(nID < ID_FILE_MRU_FILE1 + (UINT)m_pRecentFileList->GetSize());
	int nIndex = nID - ID_FILE_MRU_FILE1;
	ASSERT((*m_pRecentFileList)[nIndex].GetLength() != 0);

	TRACE(traceAppMsg, 0, _T("MRU: open file (%d) '%s'.\n"), (nIndex) + 1,
			(LPCTSTR)(*m_pRecentFileList)[nIndex]);

	LPCTSTR pPath=(*m_pRecentFileList)[nIndex];

	//Check existence and read permission first --
	if (_access(pPath,4)==-1) {
		CMsgBox(MB_ICONINFORMATION,"%s is no longer available and will be removed from list.",(*m_pRecentFileList)[nIndex]);
		m_pRecentFileList->Remove(nIndex);
	}
	else OpenDocumentFile(pPath);

	return TRUE;
}

void CWallsMapApp::AddToRecentFileList(const char* pszPathName)
{
	if(!m_bNoAddRecent)
		CWinApp::AddToRecentFileList(pszPathName);
}