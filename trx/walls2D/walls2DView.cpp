// walls2DView.cpp : implementation of the CWalls2DView class
//
#include "stdafx.h"
#include "walls2D.h"
#include "MainFrm.h"
#include <mshtml.h>
#include <io.h>
#include <trx_str.h>

#include "walls2DDoc.h"
#include "walls2DView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum e_nav {F_IsPresent=-1,F_IsCurrent=-2,F_GoLastZoom=-3,F_GoHome=-4};

static BOOL bIE_Status=0; //0 - Adobe install required, 1 - Adove install OK, 2 - IE9 SVG internal
static bool bIE9_avail=false;
static bool bAdobe_avail=false;

CString CWalls2DView::m_strLastSVG="";
LPSTR CWalls2DView::m_pszTempHTM_Format=NULL;

static LPCSTR layer[]={
	"w2d_Grid",
	"w2d_Survey",
	"w2d_Detail",
	"w2d_Walls",
	"w2d_Labels",
	"w2d_Flags",
	"w2d_Notes"
};

enum e_layer {L_GRID,L_SURVEY,L_DETAIL,L_WALLS,L_LABELS,L_FLAGS,L_NOTES};

/////////////////////////////////////////////////////////////////////////////
// CWalls2DView

IMPLEMENT_DYNCREATE(CWalls2DView, CHtmlView)

BEGIN_MESSAGE_MAP(CWalls2DView, CHtmlView)
	//{{AFX_MSG_MAP(CWalls2DView)
	ON_COMMAND(ID_GO_BACK, OnGoBack)
	ON_COMMAND(ID_GO_FORWARD, OnGoForward)
	ON_COMMAND(ID_VIEW_STOP, OnViewStop)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_SHOW_VECTORS, OnShowVectors)
	ON_UPDATE_COMMAND_UI(ID_SHOW_VECTORS, OnUpdateShowVectors)
	ON_COMMAND(ID_SHOW_GRID, OnShowGrid)
	ON_UPDATE_COMMAND_UI(ID_SHOW_GRID, OnUpdateShowGrid)
	ON_COMMAND(ID_SHOW_STATIONS, OnShowStations)
	ON_UPDATE_COMMAND_UI(ID_SHOW_STATIONS, OnUpdateShowStations)
	ON_COMMAND(ID_SHOW_ADDITIONS, OnShowAdditions)
	ON_UPDATE_COMMAND_UI(ID_SHOW_ADDITIONS, OnUpdateShowAdditions)
	ON_WM_DESTROY()
	ON_COMMAND(ID_SHOW_WALLS, OnShowWalls)
	ON_UPDATE_COMMAND_UI(ID_SHOW_WALLS, OnUpdateShowWalls)
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_UPDATE_COMMAND_UI(ID_VIEW_REFRESH, OnUpdateViewRefresh)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STOP, OnUpdateViewStop)
	ON_COMMAND(ID_VIEW_HOME, OnViewHome)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOME, OnUpdateViewHome)
	ON_COMMAND(ID_SHOW_FLAGS, OnShowFlags)
	ON_UPDATE_COMMAND_UI(ID_SHOW_FLAGS, OnUpdateShowFlags)
	ON_COMMAND(ID_SHOW_NOTES, OnShowNotes)
	ON_UPDATE_COMMAND_UI(ID_SHOW_NOTES, OnUpdateShowNotes)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_UTM,OnUpdateUTM)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWalls2DView construction/destruction

CWalls2DView::CWalls2DView()
{
	m_bWrappedSVG=FALSE;
}

CWalls2DView::~CWalls2DView()
{
}

BOOL CWalls2DView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.lpszClass = AfxRegisterWndClass(0);
	return CHtmlView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CWalls2DView drawing

void CWalls2DView::OnDraw(CDC* pDC)
{
	//CWalls2DDoc* pDoc = GetDocument();
	//ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CWalls2DView diagnostics

#ifdef _DEBUG
void CWalls2DView::AssertValid() const
{
	CHtmlView::AssertValid();
}

void CWalls2DView::Dump(CDumpContext& dc) const
{
	CHtmlView::Dump(dc);
}

CWalls2DDoc* CWalls2DView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWalls2DDoc)));
	return (CWalls2DDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWalls2DView message handlers
void CWalls2DView::OnGoBack() 
{
	//GoBack();
}

void CWalls2DView::OnViewRefresh() 
{
	if(!m_bWrappedSVG) Refresh();	
}

void CWalls2DView::OnUpdateViewRefresh(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_bWrappedSVG);
}

void CWalls2DView::OnGoForward() 
{
	//GoForward();
}

void CWalls2DView::OnViewStop() 
{
	//This is actually a Go Back to Initial Zoomed View operation --
	ShowLayer("",F_GoLastZoom);
	theApp.m_bPanning=FALSE;
}

void CWalls2DView::OnUpdateViewStop(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bWrappedSVG && theApp.m_bPanning);
}


void CWalls2DView::OnViewHome() 
{
	ShowLayer("",F_GoHome);
	theApp.m_bPanning=theApp.m_bNotHomed=FALSE;
}

void CWalls2DView::OnUpdateViewHome(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bWrappedSVG && (theApp.m_bNotHomed || theApp.m_bPanning));
}

BOOL CWalls2DView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
// create the view window itself
	m_pCreateContext = pContext;
	if (!CView::Create(lpszClassName, lpszWindowName,
				dwStyle, rect, pParentWnd,  nID, pContext))
	{
		return FALSE;
	}

	// assure that control containment is on

	//AfxEnableControlContainer(); ***Removal necessary -- this resets default COccManager!!!

	RECT rectClient;
	GetClientRect(&rectClient);

	// create the control window
	// AFX_IDW_PANE_FIRST is a safe but arbitrary ID
	if (!m_wndBrowser.CreateControl(CLSID_WebBrowser, lpszWindowName,
				WS_VISIBLE | WS_CHILD, rectClient, this, AFX_IDW_PANE_FIRST))
	{
		DestroyWindow();
		return FALSE;
	}

	LPUNKNOWN lpUnk = m_wndBrowser.GetControlUnknown();
	HRESULT hr = lpUnk->QueryInterface(IID_IWebBrowser2, (void**) &m_pBrowserApp);
	if (!SUCCEEDED(hr))
	{
		m_pBrowserApp=NULL;
		m_wndBrowser.DestroyWindow();
		DestroyWindow();
		return FALSE;
	}

	return TRUE;
}

int CWalls2DView::ShowLayer(LPCSTR szLayer,int iShow)
{
  static BYTE parms[] = VTS_BSTR VTS_I2;
  short bRet=0;
  if(m_bWrappedSVG) {
	  m_DispDriver.InvokeHelper(m_dispid,DISPATCH_METHOD,
		  VT_I2,&bRet,parms,szLayer,iShow);
  }
  return bRet;
}

BOOL CWalls2DView::ShowStatus(int typ,BOOL *pbShow)
{
	int i=ShowLayer(layer[typ],F_IsPresent);
	*pbShow=(i==2);
	return i>0;
}

BOOL CWalls2DView::EnableLayers()
{
	BOOL bAvail=ShowLayer("",F_IsCurrent);
	if(!bAvail) AfxMessageBox(IDS_ERR_NOSVG3);

	m_bEnableGrid=bAvail && ShowStatus(L_GRID,&m_bShowGrid);
	m_bEnableWalls=bAvail && ShowStatus(L_WALLS,&m_bShowWalls);
	m_bEnableVectors=bAvail && ShowStatus(L_SURVEY,&m_bShowVectors);
	m_bEnableFlags=bAvail && ShowStatus(L_FLAGS,&m_bShowFlags);
	m_bEnableLabels=bAvail && ShowStatus(L_LABELS,&m_bShowLabels);
	m_bEnableNotes=bAvail && ShowStatus(L_NOTES,&m_bShowNotes);
	m_bEnableAdditions=bAvail && ShowStatus(L_DETAIL,&m_bShowAdditions);
	return bAvail;
}

void CWalls2DView::OnShowWalls() 
{
	m_bShowWalls=(m_bShowWalls==FALSE);
	ShowLayer(layer[L_WALLS],m_bShowWalls);
}

void CWalls2DView::OnUpdateShowWalls(CCmdUI* pCmdUI) 
{
	if(m_bWrappedSVG && m_bEnableWalls) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bShowWalls);
	}
	else {
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CWalls2DView::OnShowAdditions() 
{
	m_bShowAdditions=(m_bShowAdditions==FALSE);
	ShowLayer(layer[L_DETAIL],m_bShowAdditions);
}

void CWalls2DView::OnUpdateShowAdditions(CCmdUI* pCmdUI) 
{
	if(m_bWrappedSVG && m_bEnableAdditions) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bShowAdditions);
	}
	else {
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CWalls2DView::OnShowVectors() 
{
	m_bShowVectors=(m_bShowVectors==FALSE);
	ShowLayer(layer[L_SURVEY],m_bShowVectors);
}

void CWalls2DView::OnUpdateShowVectors(CCmdUI* pCmdUI) 
{
	if(m_bWrappedSVG && m_bEnableVectors) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bShowVectors);
	}
	else {
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CWalls2DView::OnShowGrid() 
{
	m_bShowGrid=(m_bShowGrid==FALSE);
	ShowLayer(layer[L_GRID],m_bShowGrid);
}

void CWalls2DView::OnUpdateShowGrid(CCmdUI* pCmdUI) 
{
	if(m_bWrappedSVG && m_bEnableGrid) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bShowGrid);
	}
	else {
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CWalls2DView::OnShowStations() 
{
	m_bShowLabels=(m_bShowLabels==FALSE);
	ShowLayer(layer[L_LABELS],m_bShowLabels);
}

void CWalls2DView::OnUpdateShowStations(CCmdUI* pCmdUI) 
{
	if(m_bWrappedSVG && m_bEnableLabels) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bShowLabels);
	}
	else {
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CWalls2DView::OnShowFlags() 
{
	m_bShowFlags=(m_bShowFlags==FALSE);
	ShowLayer(layer[L_FLAGS],m_bShowFlags);
}

void CWalls2DView::OnUpdateShowFlags(CCmdUI* pCmdUI) 
{
	if(m_bWrappedSVG && m_bEnableFlags) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bShowFlags);
	}
	else {
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CWalls2DView::OnShowNotes() 
{
	m_bShowNotes=(m_bShowNotes==FALSE);
	ShowLayer(layer[L_NOTES],m_bShowNotes);
}

void CWalls2DView::OnUpdateShowNotes(CCmdUI* pCmdUI) 
{
	if(m_bWrappedSVG && m_bEnableNotes) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bShowNotes);
	}
	else {
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CWalls2DView::OnUpdateUTM(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_bWrappedSVG);
	if(m_bWrappedSVG) {
		CString s;
		s.Format(theApp.m_bCoordPage?"Pts: %8.1f %8.1f":"%11.1f E%12.1f N",theApp.m_coord.x,theApp.m_coord.y);
		pCmdUI->SetText(s); 
	}
}

void CWalls2DView::OnDestroy() 
{
	if(m_bWrappedSVG) {
		m_DispDriver.DetachDispatch();
		m_bWrappedSVG=FALSE;
	}
	/* *******
	if(m_pBrowserApp) {
		m_pBrowserApp->Release();
		m_pBrowserApp = NULL;
	}
	//*/
	CHtmlView::OnDestroy();
}

static bool CheckIE()
{

	//Warn if IE9 has affected ASV install --
	//static LPCSTR ASV_clsid="{377B5106-3B4E-4A2D-8520-8767590CAC86}"; //ASV 3.03
	//{30590066-98b5-11cf-bb82-00aa00bdce0b} -- after IE9 install
	bIE9_avail=false;
	bAdobe_avail=false;

	HKEY hkey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Internet Explorer",0,KEY_READ,&hkey) != ERROR_SUCCESS)
		return false;

	char version[32];
	DWORD sz_clsid=32; //includes space for null terminator

	/* skip test for IE9 for now --
	if(RegQueryValueEx(hkey,"Version",NULL,NULL,(LPBYTE)version,&sz_clsid)==ERROR_SUCCESS &&
		!(*version!='9' && version[1]=='.'))
		bIE9_avail=true;
	*/

	RegCloseKey(hkey);

	if(RegOpenKeyEx(HKEY_CLASSES_ROOT,
			"Mime\\Database\\Content Type\\image/vnd.adobe.svg+xml",0,KEY_READ,&hkey)!=ERROR_SUCCESS)
		return bIE9_avail;

	{
		char clsid[64];
		sz_clsid=64;
		bAdobe_avail=RegQueryValueEx(hkey,"CLSID",NULL,NULL,(LPBYTE)clsid,&sz_clsid)==ERROR_SUCCESS;
		RegCloseKey(hkey);

		if(bAdobe_avail && RegOpenKeyEx(HKEY_CLASSES_ROOT,"Mime\\Database\\Content Type\\image/svg+xml",0,KEY_READ,&hkey)!=ERROR_SUCCESS) {
			bAdobe_avail=false;
			return bIE9_avail;
		}

		if(bAdobe_avail && bIE9_avail) {
			char clsid2[64];
			DWORD sz_clsid2=64;
			bAdobe_avail=RegQueryValueEx(hkey,"CLSID",NULL,NULL,(LPBYTE)clsid2,&sz_clsid2)==ERROR_SUCCESS;
			RegCloseKey(hkey);
			if(bAdobe_avail && strcmp(clsid2,clsid)) {
				bAdobe_avail=false;
				LPSTR p=strchr(version,'.');
				if(p && (p=strchr(p+1,'.'))) *p=0;
				CString msg;
				msg.Format("It appears that you've upgraded to a new version of Internet Explorer (%s+) "
					"since installing the Adobe SVG Viewer. To use Walls2D with Adobe's viewer you'll need to reinstall it "
					"by running SVGView.exe. You can obtain the latter via a link in the About Box.\n\n"
					"NOTE: When running SVGView.exe, you may get a false warning about installing over a "
					"newer version of the viewer.",version,p);

				AfxMessageBox(msg);
			}
		}
	}

	return bIE9_avail || bAdobe_avail;
}

void CWalls2DView::Initialize()
{
	CheckIE();
	bIE_Status=bIE9_avail?2:bAdobe_avail; //make optional later

	HRSRC hrsrc=FindResource(NULL,(bIE_Status>1)?"WALLS2D_IE9.HTM":"WALLS2D.HTM",RT_HTML);
	m_pszTempHTM_Format=NULL;
	if(hrsrc) {
		UINT len=SizeofResource(NULL,hrsrc);
		HGLOBAL hr=LoadResource(NULL,hrsrc);
		LPVOID lpres=LockResource(hr);
		if(hr && (m_pszTempHTM_Format=(LPSTR)malloc(len+1))) {
			memcpy(m_pszTempHTM_Format,lpres,len);
			m_pszTempHTM_Format[len]=0;
		}
	}
}

void CWalls2DView::Terminate()
{
	free(m_pszTempHTM_Format);
}

BOOL CWalls2DView::WriteTempHTM(LPCSTR svgpath)
{
#ifdef _DEBUG
	CString s(strstr(m_pszTempHTM_Format,"src="));
#endif
	BOOL bRet=FALSE;
	FILE *fp=fopen(theApp.m_pszTempHTM,"wb");
	if(fp) {
		if(fprintf(fp,m_pszTempHTM_Format,svgpath)>0) bRet=TRUE;
		if(fclose(fp)) bRet=FALSE;
	}
	return bRet;
}

BOOL CWalls2DView::OpenSVG(LPCSTR str)
{
	if(!memicmp(trx_Stpext(str),".svg",4)) {
		if(_stricmp(str,m_strLastSVG)) {
			if(!WriteTempHTM(str)) {
				AfxMessageBox("Unable to create HTML wrapper for SVG.");
				return FALSE;
			}
			m_strLastSVG=str;
		}
		Navigate2((LPCTSTR)theApp.m_pszTempHTM,NULL,NULL);
		return TRUE;
	}
	return FALSE;
}

void CWalls2DView::OnFileOpen() 
{
	CString str;

	str.LoadString(IDS_FILETYPES);

	CFileDialog fileDlg(TRUE, NULL, NULL,OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST,str);

	if(fileDlg.DoModal() == IDOK) {
		str=fileDlg.GetPathName();
		if(!OpenSVG(str)) {
			Navigate2(str, 0, NULL);
		}
	}
}

void CWalls2DView::OnInitialUpdate() 
{
 	if(!bIE_Status) {
		Navigate2("about:blank");
		return;
	}
	//m_strLastSVG="";
	LPCSTR pPath=theApp.GetRecentFile();
	theApp.m_pDocument=m_pDocument;

	if(!pPath) {
		pPath=theApp.m_pszDfltSVG;
		if(_access(pPath,4)==-1) {
			AfxMessageBox("The default image file, walls2D.svgz, is missing from the Walls2D program folder.");
			Navigate2("about:blank");
			return;
		}
	}

	if(!OpenSVG(pPath)) {
		Navigate2(pPath,0,NULL);
	}
}

void CWalls2DView::OnBeforeNavigate2(LPCTSTR lpszURL, DWORD nFlags,
	 LPCTSTR lpszTargetFrameName, CByteArray& baPostedData,
	 LPCTSTR lpszHeaders, BOOL* pbCancel) 
{
	theApp.m_csTitle.Empty();
}

void CWalls2DView::OnTitleChange(LPCTSTR lpszText)
{
}

#ifdef _DEBUG
void CWalls2DView::OnDownloadBegin()
{
	//CHtmlView::OnDownloadBegin(); //empty
	int i=0;
}
void CWalls2DView::OnDownloadComplete()
{
	// user will override to handle this notification
	int i=0;
}
#endif

void CWalls2DView::OnDocumentComplete(LPCTSTR lpszUrl)
{
	if(m_bWrappedSVG) {
		m_bWrappedSVG=FALSE;
		m_DispDriver.DetachDispatch();
	}

	m_bEnableWalls=m_bEnableVectors=m_bEnableGrid=m_bEnableLabels=m_bEnableAdditions=FALSE;

	if(!_stricmp(lpszUrl,theApp.m_pszTempHTM)) {

		lpszUrl=m_strLastSVG;

		IDispatch * pDisp = GetHtmlDocument();

		if(pDisp) {
		  HRESULT hr;
		  IHTMLDocument2* spDoc;
		  hr=pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&spDoc);
		  pDisp->Release();
		  if(SUCCEEDED(hr)) {
			IHTMLWindow2 *pWin;
			hr=spDoc->get_Script((IDispatch **)&pWin);
			spDoc->Release();
			if(SUCCEEDED(hr)) {
				static OLECHAR *p=L"w2dShow";
				hr = pWin->GetIDsOfNames(IID_NULL,&p,1,LOCALE_SYSTEM_DEFAULT,&m_dispid);
				if (SUCCEEDED(hr)) {
					m_DispDriver.AttachDispatch(pWin);
					m_bWrappedSVG=TRUE;
					theApp.m_coord.x=theApp.m_coord.y=0.0;
					theApp.m_bPanning=theApp.m_bNotHomed=FALSE;
					theApp.AddToRecentFileList(m_strLastSVG);
					if(EnableLayers()) {
						((CMainFrame*)AfxGetMainWnd())->HelpMsg();
						// this will change the main frame's title bar
						lpszUrl=trx_Stpnam(m_strLastSVG);
					}
					else {
						lpszUrl="";
						((CMainFrame*)AfxGetMainWnd())->NoSVGMsg();
						((CWalls2DApp *)AfxGetApp())->OnAppAbout();
						Navigate2("about:blank");
					}
				}
				pWin->Release();
			}
		  }
		}
	}
	if(theApp.m_csTitle.IsEmpty()) m_pDocument->SetTitle(lpszUrl);
}

BOOL CWalls2DView::CreateControlSite(COleControlContainer*, 
   COleControlSite** ppSite, UINT /* nID */, REFCLSID /* clsid */)
{
   ASSERT( ppSite != NULL );
   *ppSite = NULL;  // Use the default site
   return TRUE;
}
