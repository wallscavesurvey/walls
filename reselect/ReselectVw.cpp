// ReselectVw.cpp : implementation file
//

#include "stdafx.h"
#include "reselect.h"
#include "MainFrm.h"

#include "ReselectDoc.h"
#include "ReselectVw.h"
#include "resellib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static char *nwbb_htm="nwbb.htm";
static char *texbib_htm="texbib.htm";

char *CReselectVw::m_pDatabase=NULL;
BOOL CReselectVw::m_bEnableNwbb=FALSE;
BOOL CReselectVw::m_bEnableTexbib=FALSE;

static char argvbuf[SIZ_ARGVBUF];

/////////////////////////////////////////////////////////////////////////////
// CReselectVw

IMPLEMENT_DYNCREATE(CReselectVw, CHtmlView)

BEGIN_MESSAGE_MAP(CReselectVw, CHtmlView)
	//{{AFX_MSG_MAP(CReselectVw)
	ON_COMMAND(ID_GO_BACK, OnGoBack)
	ON_COMMAND(ID_GO_FORWARD, OnGoForward)
	ON_COMMAND(ID_VIEW_STOP, OnViewStop)
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_VIEW_FONTS_LARGE, OnViewFontsLarge)
	ON_COMMAND(ID_VIEW_FONTS_LARGEST, OnViewFontsLargest)
	ON_COMMAND(ID_VIEW_FONTS_MEDIUM, OnViewFontsMedium)
	ON_COMMAND(ID_VIEW_FONTS_SMALL, OnViewFontsSmall)
	ON_COMMAND(ID_VIEW_FONTS_SMALLEST, OnViewFontsSmallest)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_GO_SEARCH_NWBB, OnGoSearchNwbb)
	ON_COMMAND(ID_GO_SEARCH_TEXBIB, OnGoSearchTexbib)
	ON_UPDATE_COMMAND_UI(ID_GO_SEARCH_NWBB, OnUpdateGoSearchNwbb)
	ON_UPDATE_COMMAND_UI(ID_GO_SEARCH_TEXBIB, OnUpdateGoSearchTexbib)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CReselectVw::CReselectVw()
{
	//m_baPostedData.SetSize(64,64);

	//{{AFX_DATA_INIT(CReselectVw)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CReselectVw::~CReselectVw()
{
}

void CReselectVw::DoDataExchange(CDataExchange* pDX)
{
	CHtmlView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReselectVw)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BOOL CReselectVw::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	cs.lpszClass = AfxRegisterWndClass(0);
	return CHtmlView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CReselectVw drawing

void CReselectVw::OnDraw(CDC* pDC) 
{
	CReselectDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

/////////////////////////////////////////////////////////////////////////////
// CReselectVw diagnostics

#ifdef _DEBUG
void CReselectVw::AssertValid() const
{
	CHtmlView::AssertValid();
}

void CReselectVw::Dump(CDumpContext& dc) const
{
	CHtmlView::Dump(dc);
}

CReselectDoc* CReselectVw::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CReselectDoc)));
	return (CReselectDoc *)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CReselectVw message handlers

CString CReselectVw::ResourceToURL(LPCTSTR lpszURL)
{
	// This functions shows how to convert an URL to point
	// within the application
	// I only use it once to get the startup page

	CString m_strURL;
	HINSTANCE hInstance = AfxGetResourceHandle();
	ASSERT(hInstance != NULL);
	
	LPTSTR lpszModule = new TCHAR[_MAX_PATH];
	
	if (GetModuleFileName(hInstance, lpszModule, _MAX_PATH))
	{
		m_strURL.Format(_T("res://%s/%s"), lpszModule, lpszURL);
	}
	
	delete []lpszModule;

	return m_strURL;
}

static BOOL GetFileTimeStr(char *time,const char *pName)
{
    struct _stat status;
	struct tm *ptm;

	strcpy(argvbuf,theApp.m_szDBFilePath);
	strcpy(trx_Stpnam(argvbuf),pName);
	argvbuf[strlen(argvbuf)-1]='x';
	if(_stat(argvbuf,&status)) {
		*time=0;
		return FALSE;
	}
	ptm=localtime(&status.st_mtime);
	_snprintf(time,31,"Version\n%2u/%02u/%02u",ptm->tm_mon+1,ptm->tm_mday,(ptm->tm_year%100));
	return TRUE;
}

static char *section="Startup";

void CReselectVw::Initialize()
{
	m_bEnableTexbib=GetFileTimeStr(theApp.m_texbib_date,texbib_htm);
	m_bEnableNwbb=GetFileTimeStr(theApp.m_nwbb_date,nwbb_htm);
	m_pDatabase=texbib_htm;
	*theApp.m_texbib_keybuf=*theApp.m_nwbb_keybuf=0;
	*theApp.m_texbib_nambuf=*theApp.m_nwbb_nambuf=0;

	theApp.m_texbib_bFlags=theApp.m_nwbb_bFlags=0; //Must be <=7
	{
		int i;
		if(m_bEnableTexbib) {
			i=theApp.GetProfileInt(section,"TXkey",RESEL_SHOWNO_MASK);
			if((UINT)i<=RESEL_MASK) theApp.m_texbib_bFlags=i;
		}
		if(m_bEnableNwbb) {
			i=theApp.GetProfileInt(section,"NWkey",RESEL_SHOWNO_MASK);
			if((UINT)i<=RESEL_MASK) theApp.m_nwbb_bFlags=i;
		}
	}
	if(m_bEnableTexbib && m_bEnableNwbb) {
		CString str;
		str=theApp.GetProfileString(section,"Database");
		if(!str.IsEmpty() && str[0]=='n') m_pDatabase=nwbb_htm;
	}
	else if(!m_bEnableTexbib && m_bEnableNwbb) m_pDatabase=nwbb_htm;
}

void CReselectVw::Terminate()
{
	theApp.WriteProfileString(section,"Database",m_pDatabase);
	if(m_bEnableTexbib) theApp.WriteProfileInt(section,"TXkey",RESEL_MASK&theApp.m_texbib_bFlags);
	if(m_bEnableNwbb) theApp.WriteProfileInt(section,"NWkey",RESEL_MASK&theApp.m_nwbb_bFlags);
}

void CReselectVw::OnInitialUpdate() 
{
	m_strNwbb = ResourceToURL(nwbb_htm);
	m_strTexbib = ResourceToURL(texbib_htm);
	Navigate2((m_pDatabase==nwbb_htm)?m_strNwbb:m_strTexbib, NULL, NULL);
}

static char *get_argv(char *pbuf,char *p)
{
	int c;
	char hexbuf[4];

	while(*p && *p!='&') {
		switch(*p) {
		  case '%' : hexbuf[0]=*++p;
					 if(*p) {
						 hexbuf[1]=*++p;
						 hexbuf[2]=0;
						 if(*p) p++;
						 sscanf(hexbuf,"%x",&c);
						 *pbuf++=c;
					 }
					 break;

		  case '+' : while(*++p=='+');
					 *pbuf++=' ';
					 break;

		  default:   *pbuf++=*p++;
		}
	}
	*pbuf++=0;
	return pbuf;
}

static int GetPostedArgv(char **argv,const char *strBytes,LPCTSTR lpszURL)
{
	LPCSTR p;
	LPSTR pargv;
	BOOL bQuoting=FALSE;
	BOOL bTexbib;


	strcpy(argvbuf,theApp.m_szDBFilePath);
    
	if((p=strrchr(lpszURL,'/'))) p++;
	else p=texbib_htm;

	bTexbib=(*p!='n');

	strcpy(trx_Stpnam(argvbuf),p);
	   
	argv[0]=argvbuf;
	argv[3]=0;

	pargv=argvbuf+strlen(argvbuf)+1;

	argv[1]=pargv;
	if((p=strstr(strBytes,"T1="))) pargv=get_argv(pargv,(LPSTR)p+3);
	else *pargv++=0;

	argv[2]=pargv;
	if((p=strstr(strBytes,"T2="))) pargv=get_argv(pargv,(LPSTR)p+3);
	else *pargv++=0;

	int flags=0;

	if((p=strstr(strBytes,"RK="))) {
		if(p[3]=='1') flags=1;
		else if(p[3]=='2') flags=2;
	}

	if((p=strstr(strBytes,"RA="))) {
		if(p[3]=='1') flags|=RESEL_OR_AUTHOR_MASK;
	}

	if((p=strstr(strBytes,"C1="))) {
		flags|=RESEL_SHOWNO_MASK;
	}

	if((p=strstr(strBytes,"C2="))) {
		flags|=RESEL_SORT_MASK;
	}

	if(!bTexbib) {
		theApp.m_nwbb_bFlags=flags;
		strcpy(theApp.m_nwbb_keybuf,argv[1]);
		strcpy(theApp.m_nwbb_nambuf,argv[2]);
		flags|=RESEL_BLUE_MASK;
	}
	else {
		theApp.m_texbib_bFlags=flags;
		strcpy(theApp.m_texbib_keybuf,argv[1]);
		strcpy(theApp.m_texbib_nambuf,argv[2]);
	}
	return flags;
}

void CReselectVw::OnBeforeNavigate2(LPCTSTR lpszURL, DWORD nFlags,
	 LPCTSTR lpszTargetFrameName, CByteArray& baPostedData,
	 LPCTSTR lpszHeaders, BOOL* pbCancel) 
{
	// Are we on the initial navigation thrown by the 
	// OnInitialUpdate?  If not, process the POST.

	BOOL bTimer=FALSE;
	static BOOL bLastError=FALSE;

	if(!memcmp(lpszURL,"res:",4)) {

		ASSERT(!bLastError || (*lpszHeaders==NULL));

		if(!*lpszHeaders || bLastError) {
			bTimer=TRUE;
			m_pDatabase=(*trx_Stpnam(lpszURL)=='n')?nwbb_htm:texbib_htm;
			bLastError=FALSE;
		}
		else {
			// Build a message box from the submitted data

			/*
			CString strMessage, strBytes;
			strMessage = _T("");

			// Get general info from the submitted form
			strMessage.Format(
			_T("Browse To:\n%s\n\nFlags: %d\nTarget Frame:\n%s\n\nHeaders:\n%s\n"),
			lpszURL, nFlags,	lpszTargetFrameName, lpszHeaders);
			*/

			// Get the POST data --
			CString strBytes;
			int len=baPostedData.GetSize();

			if(len) {
				strBytes = _T("");
				for(int i = 0;i< len;i++) {
					strBytes += (char)baPostedData[i];
				}

				//m_baPostedData.RemoveAll();
				//m_baPostedData.Append(baPostedData);

				//re_Errstr();

				re_SetFlags(GetPostedArgv(m_Argv,strBytes,lpszURL));

				if(re_Select(3,m_Argv)) {
					AfxMessageBox(re_Errstr());
					//PostMessage((*m_pDatabase=='n')?ID_GO_SEARCH_NWBB:ID_GO_SEARCH_TEXBIB);
					Navigate2((*m_pDatabase=='n')?m_strNwbb:m_strTexbib,NULL,NULL);
					bLastError=TRUE;
				}
				else {
					Navigate2(m_Argv[0],0,0);
				}
				*pbCancel=TRUE;
			}
		}
	}
	else {
		bLastError=FALSE;
		if(!memicmp(lpszURL,"http",4)) {
			*pbCancel=TRUE;
			theApp.ExecuteUrl(lpszURL);
			return;
		}
	}

	//CHtmlView::OnBeforeNavigate2(lpszURL, nFlags,	lpszTargetFrameName, baPostedData, lpszHeaders, pbCancel);
	// start the animation so that is plays while the new page is being loaded
    if(memcmp(lpszURL,"mailto:",7))
		((CMainFrame*)GetParentFrame())->StartAnimation(bTimer);
}

void CReselectVw::OnTitleChange(LPCTSTR lpszText)
{
	// this will change the main frame's title bar
	if (m_pDocument != NULL)
		m_pDocument->SetTitle(lpszText);
}

void CReselectVw::OnDocumentComplete(LPCTSTR lpszUrl)
{
	// make sure the main frame has the new URL.  This call also stops the animation
	((CMainFrame*)GetParentFrame())->SetAddress(lpszUrl);
}

void CReselectVw::OnGoBack() 
{
	GoBack();
}

void CReselectVw::OnGoForward() 
{
	GoForward();
}

void CReselectVw::OnViewStop() 
{
	Stop();
	((CMainFrame*)GetParentFrame())->StopAnimation();
}

void CReselectVw::OnViewRefresh() 
{
	Refresh();
}

void CReselectVw::OnViewFontsLarge() 
{
	COleVariant vaZoomFactor(3l);

	ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
		   &vaZoomFactor, NULL);
}

void CReselectVw::OnViewFontsLargest() 
{
	COleVariant vaZoomFactor(4l);

	ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
		   &vaZoomFactor, NULL);
}

void CReselectVw::OnViewFontsMedium() 
{
	COleVariant vaZoomFactor(2l);

	ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
		   &vaZoomFactor, NULL);
}

void CReselectVw::OnViewFontsSmall() 
{
	COleVariant vaZoomFactor(1l);

	ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
		   &vaZoomFactor, NULL);
}

void CReselectVw::OnViewFontsSmallest() 
{
	COleVariant vaZoomFactor(0l);

	ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
		   &vaZoomFactor, NULL);
}

void CReselectVw::OnFileOpen() 
{
	CString str;

	str.LoadString(IDS_FILETYPES);

	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, str);

	if(fileDlg.DoModal() == IDOK)
		Navigate2(fileDlg.GetPathName(), 0, NULL);
}


void CReselectVw::OnGoSearchNwbb() 
{
	Navigate2(m_strNwbb,NULL,NULL);
}

void CReselectVw::OnGoSearchTexbib() 
{
	Navigate2(m_strTexbib,NULL,NULL);
}

void CReselectVw::OnUpdateGoSearchNwbb(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bEnableNwbb);
	
}

void CReselectVw::OnUpdateGoSearchTexbib(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bEnableTexbib);
}

BOOL CReselectVw::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
// create the view window itself
	m_pCreateContext = pContext;
	if (!CView::Create(lpszClassName, lpszWindowName,
				dwStyle, rect, pParentWnd,  nID, pContext))
	{
		return FALSE;
	}

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
		m_pBrowserApp = NULL;
		m_wndBrowser.DestroyWindow();
		DestroyWindow();
		return FALSE;
	}

	return TRUE;
} 

BOOL CReselectVw::CreateControlSite(COleControlContainer*, 
   COleControlSite** ppSite, UINT /* nID */, REFCLSID /* clsid */)
{
   ASSERT( ppSite != NULL );
   *ppSite = NULL;  // Use the default site
   return TRUE;
}
