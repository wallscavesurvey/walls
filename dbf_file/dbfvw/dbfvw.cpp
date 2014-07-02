// dbfvw.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "dbfvw.h"
#include "utility.h"
#include <direct.h>
#include <trx_str.h>
#include <string.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDbfvwApp

BEGIN_MESSAGE_MAP(CDbfvwApp, CWinApp)
        //{{AFX_MSG_MAP(CDbfvwApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	//}}AFX_MSG_MAP
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDbfvwApp construction

CDbfvwApp::CDbfvwApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDbfvwApp object

CDbfvwApp theApp;
/////////////////////////////////////////////////////////////////////////////
// INI file implementation
static char BASED_CODE szIniFileSection[] = "Most Recent DBF File";
static char BASED_CODE szIniFileEntry[] = "File";
static char *pExtDBF[4]={".NTV",".NTS",".NTN",".NTP"};

void CDbfvwApp::UpdateIniFileWithDocPath(const char* pszPathName)
{
    char *p=_NCHR(trx_Stpext((LPSTR)pszPathName));
	for(int i=1;i<=3;i++) if(!stricmp(p,pExtDBF[i])) return;
	WriteProfileString(szIniFileSection, szIniFileEntry, pszPathName);
}

/////////////////////////////////////////////////////////////////////////////
// CDbfvwApp initialization

BOOL CDbfvwApp::InitInstance()
{
	// Standard initialization --
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	LoadStdProfileSettings(6);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	// serve as the connection between documents, frame windows and views.
	
    AddDocTemplate(new CMultiDocTemplate(IDR_DBFTYPE,
                        RUNTIME_CLASS(CDbfDoc),
                        RUNTIME_CLASS(CDbfFrame),        // standard MDI child frame
                        RUNTIME_CLASS(CDbfView)));
			
	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// enable file manager drag/drop and DDE Execute open
	pMainFrame->DragAcceptFiles();
	EnableShellOpen();
	RegisterShellFileTypes();

	// Parse command line for standard shell commands, DDE, file open
	//CCommandLineInfo cmdInfo;
	//ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line
	//if (!ProcessShellCommand(cmdInfo))return FALSE;

	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	// simple command line parsing
	if (m_lpCmdLine[0] == '\0')
	{
		// Try opening file name saved in private INI file.
		CString strDocPath = GetProfileString(szIniFileSection, szIniFileEntry, NULL);
		if (!strDocPath.IsEmpty()) {
		   CDBRecDoc::m_bInitialOpen=TRUE;
		   OpenDatabase(strDocPath.GetBuffer(0));
		   CDBRecDoc::m_bInitialOpen=FALSE;
		}
	}
	else if ((m_lpCmdLine[0] == '-' || m_lpCmdLine[0] == '/') &&
		(m_lpCmdLine[1] == 'e' || m_lpCmdLine[1] == 'E'))
	{
		// program launched embedded - wait for DDE or OLE open
	}
	else
	{
		// open an existing document
		OpenDocumentFile(m_lpCmdLine);
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

// App command to run the dialog
void CDbfvwApp::OnAppAbout()
{
	CDialog aboutDlg(IDD_ABOUTBOX);
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CDbfvwApp commands

static void NEAR PASCAL AddFilter(CString &filter,int nIDS)
{
	CString newFilter;
	
	VERIFY(newFilter.LoadString(nIDS));
    filter += newFilter;
    filter += (char)'\0';
	
    char *pext=strchr(newFilter.GetBuffer(0),'(');
    ASSERT(pext!=NULL);
    filter += ++pext;
    
    //filter.SetAt(filter.GetLength()-1,'\0') causes assertion failure!
    //Then why doesn't filter+='\0' fail also?
    
    filter.GetBufferSetLength(filter.GetLength()-1); //delete trailing ')'
    filter += (char)'\0';
}    

BOOL CDbfvwApp::DoPromptOpenFile(CString& pathName,DWORD lFlags,
   int numFilters,CString &strFilter)
{
	CFileDialog dlgFile(TRUE);

	CString title;
	VERIFY(title.LoadString(AFX_IDS_OPENFILE));
	
	dlgFile.m_ofn.Flags |= lFlags;

	dlgFile.m_ofn.nMaxCustFilter+=numFilters;
	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.hwndOwner = AfxGetApp()->m_pMainWnd->GetSafeHwnd();
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = pathName.GetBuffer(MAX_PATH);

	BOOL bRet = dlgFile.DoModal() == IDOK ? TRUE : FALSE;
	pathName.ReleaseBuffer();
	return bRet;
}

void CDbfvwApp::OnFileOpen()
{
	CString strFilter;
	#define NUM_FILTERS 2
	
	AddFilter(strFilter,IDS_NETWORK_FILES);
    AddFilter(strFilter,AFX_IDS_ALLFILTER);
    
	CString pathName;
	if(!DoPromptOpenFile(pathName,OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,
	  NUM_FILTERS,strFilter)) return;

	OpenDatabase(pathName.GetBuffer(0));
	
}

CDocument *CDbfvwApp::OpenDatabase(char *pszPathName)
{
	CDocument *pDoc=CWinApp::OpenDocumentFile(pszPathName);
	if(pDoc) {
	  char *p=trx_Stpnam(pszPathName)-1;
	  if(p>pszPathName && *p=='\\') {
		*p=0;
		_chdir(pszPathName);
		*p='\\';
	  }
	  p=trx_Stpext(p+1);
	  if(!stricmp(p,pExtDBF[0])) {
		BOOL bLower=(p[1]=='n');
	    for(int i=1;i<=3;i++) {
		   strcpy(p,pExtDBF[i]);
		   if(bLower) _strlwr(p);
	       CWinApp::OpenDocumentFile(pszPathName);
		}
	  }
	}
	return pDoc;
}

CDocument *CDbfvwApp::OpenDocumentFile(LPCSTR pszPathName)
{
	char buf[MAX_PATH];
	strcpy(buf,pszPathName);
	return OpenDatabase(buf);
}

void CDbfvwApp::AddToRecentFileList(const char* pszPathName)
{
    char *p=trx_Stpext((LPSTR)pszPathName);
	if(toupper(p[1])!='N' || toupper(p[2])!='T' || toupper(p[3])=='V')
	  CWinApp::AddToRecentFileList(pszPathName);
}
