// scene.cpp : main application file
// written in December 1994
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#include "stdafx.h"
#include "hgapp.h"
#include "pcutil/mfcutil.h"

#include "mainfrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern UINT id_MessageLoad;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIMainFrame)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIMainFrame)
	//{{AFX_MSG_MAP(CMainFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	ON_MESSAGE(WM_WRITE_STATUS, OnWriteStatus)
	ON_MESSAGE(WM_COPYDATA, OnCopyData)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP

	// Global help commands
	//ON_COMMAND(ID_HELP_INDEX, CMDIMainFrame::OnHelpIndex)
	//ON_COMMAND(ID_HELP_USING, CMDIMainFrame::OnHelpUsing)
	//ON_COMMAND(ID_HELP, CMDIMainFrame::OnHelp)
	//ON_COMMAND(ID_CONTEXT_HELP, CMDIMainFrame::OnContextHelp)
	//ON_COMMAND(ID_DEFAULT_HELP, CMDIMainFrame::OnHelpIndex)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars
	
// toolbar buttons - IDs are command buttons
static UINT BASED_CODE buttons[] =
{
	// same order as in the bitmap 'toolbar.bmp'
	ID_FILE_OPEN, 
		ID_SEPARATOR,
	ID_WINDOW_CASCADE,
	ID_WINDOW_TILE_VERT,
	ID_WINDOW_TILE_HORZ,
		ID_SEPARATOR,
	ID_VIEW_RESET,
		ID_SEPARATOR,
	ID_APP_ABOUT
};

static UINT BASED_CODE indicators[] =
{
  ID_SEPARATOR,           // status line indicator
  ID_SEPARATOR,           // page indicator
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

LONG CMainFrame::OnCopyData(WPARAM wParam,LPARAM lParam)
{
	COPYDATASTRUCT *cds=(COPYDATASTRUCT *)lParam;
	if(cds->dwData==15 && cds->cbData==MAX_PATH) {
		CDocument *pDoc=AfxGetApp()->OpenDocumentFile((LPCSTR)cds->lpData);
		return pDoc!=NULL;
	}
	return FALSE;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	extern LPCTSTR lpszUniqueClass;
    // Use the specific class name we established earlier
	cs.lpszClass = lpszUniqueClass;

	return CMDIFrameWnd::PreCreateWindow(cs);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadBitmap(IDR_MAINFRAME) ||
		!m_wndToolBar.SetButtons(buttons,
		  sizeof(buttons)/sizeof(UINT)))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

  m_wndStatusBar.SetPaneInfo(0, 0, SBPS_STRETCH, 0);

  UINT nId, nStyle;
  int  cxWidth;
    
  m_wndStatusBar.GetPaneInfo(1, nId, nStyle, cxWidth);
  m_wndStatusBar.SetPaneInfo(1, nId, nStyle, 170);


	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
#if (_MFC_VER > 0x0250)
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY | CBRS_FLOAT_MULTI);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	// TODO: Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
#endif

	return 0;
}

void CMainFrame::drawProgress(DisplayMethod method)
{
  if(!guiControls_ || !guiControls_->getProgIndicator())
    return;

  CRect cProgressBar, cPercent;
  CBrush bkBrush, progressBrush;

  CDC* pDC=m_wndStatusBar.GetDC();    

  m_wndStatusBar.GetItemRect( 1, &cProgressBar);

  cPercent=cProgressBar;

  cProgressBar.left+= 2;
  cProgressBar.right-=2;
  cProgressBar.top+=2;
  cProgressBar.bottom-=2;

  bkBrush.CreateSolidBrush(RGB(192,192,192));  // erase Background
  CBrush* oldBrush = pDC->SelectObject(&bkBrush);
  pDC->FillRect(&cProgressBar,&bkBrush);

  cProgressBar.right = cProgressBar.left + (long)((float)cProgressBar.Width() * guiControls_->getProgIndicator());
  progressBrush.CreateSolidBrush(RGB(128,255,128));
  CBrush* oldBrush2 = pDC->SelectObject(&progressBrush);
  pDC->FillRect(&cProgressBar,&progressBrush);

  char buffer[10];
  sprintf(buffer, "%d %%", (UINT)(guiControls_->getProgIndicator()*100));
  pDC->SetBkMode(TRANSPARENT);
  pDC->DrawText(buffer, -1, &cPercent, DT_CENTER);
  pDC->SelectObject(oldBrush2);
  progressBrush.DeleteObject();
      
  pDC->SelectObject(oldBrush);

  bkBrush.DeleteObject();
  m_wndStatusBar.ReleaseDC(pDC);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
LRESULT CMainFrame::OnWriteStatus(WPARAM wParam, LPARAM lParam)
{
    DisplayMethod method = (DisplayMethod)wParam;

    if (method == percentageMethod)
    {
        drawProgress(method);
        return 0L;
    }

    const char* text = 0;
    char buf[80];

    if (method == textMethod)
        text = (const char*)lParam;

    switch (method)
    {
        case textMethod :
            text = (const char*)lParam;
            break;

        case clearMethod :
            text = "";
            break;

        case valueMethod :
            sprintf(buf, "%d", (int)lParam);
            text = buf;
            break;
    }

    m_wndStatusBar.SetPaneText(1, text, TRUE);
    m_wndStatusBar.RedrawWindow(NULL,NULL,
        RDW_NOERASE | RDW_NOFRAME | RDW_NOINTERNALPAINT | RDW_UPDATENOW);

    return 0L;
} // OnWriteStatus
