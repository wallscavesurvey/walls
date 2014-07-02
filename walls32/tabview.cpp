// CTabView:
// written by Gerry High
// 74750,2456 CIS
// for more info see the tabview.wri document

#include "stdafx.h"
#include "tabview.h"

#include <ctype.h>
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabView

IMPLEMENT_DYNCREATE(CTabView, CView)

BEGIN_MESSAGE_MAP(CTabView, CView)
	//{{AFX_MSG_MAP(CTabView)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_MOUSEACTIVATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabView construction/destruction

CTabView::CTabView()
{
	m_curView = NULL;
	m_nTabs = 0;
	m_lookAndFeel = LAF_MSWORD;
}

CTabView::~CTabView()
{
	destroyFonts();
}

/////////////////////////////////////////////////////////////////////////////
// CTabView configuration helper methods

void  CTabView::setLAF(eLookAndFeel LAF)
{
#ifdef C_TABS
	if(LAF == m_lookAndFeel)
		return;
		
	m_lookAndFeel = LAF;

	if(IsWindowVisible())
		InvalidateRect(NULL);
#endif
}
void CTabView::setMargin(int margin)
{
	if(margin == m_tabArray.m_margin) return;
		
	m_tabArray.m_margin = margin;

	repositionViews();
	if(IsWindowVisible()) InvalidateRect(NULL);
}

void CTabView::setTabHeight(int height)
{
	if(height == m_tabArray.m_tabHeight)
		return;
		
	m_tabArray.m_tabHeight = height;

	repositionViews();
	if(IsWindowVisible()) InvalidateRect(NULL);
}

void CTabView::setTabPosition(eTabPosition position)
{
	int i,j;

	if(position == m_tabArray.m_position)
		return;
			
#ifdef LR_TABS
	switch (position)
	{
		case TABSONLEFT:
		case TABSONLEFTBOT:
		case TABSONRIGHT:
		case TABSONRIGHTBOT:
			m_tabArray.m_position = position;

			for(j=0;j<m_nTabs;j++)
			{
				CTabInfo* pTab = (CTabInfo*)m_tabArray[j];
				int sLen = strlen(pTab->m_tabLabel);
				for(i=0;i<sLen;i++)
				{
					if(pTab->m_tabLabel[i] == '&' && i != (sLen -1))
					{
						memmove(&pTab->m_tabLabel[i],&pTab->m_tabLabel[i+1],sLen - i);
						break;
					}
				}
			}
			break;
		default:
#endif

#ifdef T_TABS
			m_tabArray.m_position = TABSONTOP;

			for(j=0;j<m_nTabs;j++)
			{
				CTabInfo* pTab = (CTabInfo*)m_tabArray[j];
				int sLen = strlen(pTab->m_tabLabel);
				for(i=0;i<sLen;i++)
				{
					if((char)(DWORD)CharUpper((LPSTR)pTab->m_tabLabel[i]) == pTab->m_mnemonic
					 	&& i != (sLen -1))
					{
						memmove(&pTab->m_tabLabel[i+1],&pTab->m_tabLabel[i],sLen - i);
                        pTab->m_tabLabel[i] = '&';
                        break;
					}
				}
			}
#endif
#ifdef LR_TABS
			break;
	}
#endif

	if(m_nTabs != 0)
	{
		createFonts();
		repositionViews();
		if(IsWindowVisible())
			InvalidateRect(NULL);
	}
}

void CTabView::setFrameBorderOn(BOOL on)
{
	if(m_tabArray.m_frameBorderOn == on)
		return;
		
	m_tabArray.m_frameBorderOn = on;

	if(IsWindowVisible())
		InvalidateRect(NULL);
}

void CTabView::repositionViews()
{
	for(int i=0;i<m_nTabs;i++)
	{
		CTabInfo* pTab = (CTabInfo*)m_tabArray[i];
#ifdef LR_TABS
		switch (m_tabArray.m_position)
		{
			case TABSONLEFT:
			case TABSONLEFTBOT:
				pTab->m_pView->SetWindowPos(NULL,
											m_tabArray.m_margin + 5 + m_tabArray.m_tabHeight,m_tabArray.m_margin + 5,
											m_width - 2 * (m_tabArray.m_margin + 5) - m_tabArray.m_tabHeight,
											m_height - 2 * (m_tabArray.m_margin + 5),
											SWP_NOZORDER);
				break;

			case TABSONRIGHT:
			case TABSONRIGHTBOT:
				pTab->m_pView->SetWindowPos(NULL,
											m_tabArray.m_margin + 5,
											m_tabArray.m_margin + 5,
											m_width - 2 * (m_tabArray.m_margin + 5) - m_tabArray.m_tabHeight,
											m_height - 2 * (m_tabArray.m_margin + 5),
											SWP_NOZORDER);
				break;
				
			default:
#endif
#ifdef T_TABS
				pTab->m_pView->SetWindowPos(NULL,
											m_tabArray.m_margin + 5,
											m_tabArray.m_margin + 5 + m_tabArray.m_tabHeight,
											m_width - 2 * (m_tabArray.m_margin + 5),
											m_height - 2 * (m_tabArray.m_margin + 5) - m_tabArray.m_tabHeight,
											SWP_NOZORDER);
#endif
#ifdef LR_TABS
				break;
		}
#endif
	}
}

void CTabView::createFonts()
{
	destroyFonts();
		
	CClientDC dc(this);
	int nEscapement = 0;

#ifdef LR_TABS	
	switch (m_tabArray.m_position)
	{
		case TABSONLEFT:
		case TABSONLEFTBOT:
		case TABSONRIGHT:
		case TABSONRIGHTBOT:
			if(m_tabArray.m_position == TABSONLEFT || m_tabArray.m_position == TABSONLEFTBOT)
				nEscapement = 900;
			else
				nEscapement = -900;
			break;
	}
#endif

	LOGFONT lf;

	memset(&lf, '\0', sizeof(LOGFONT));

	lf.lfHeight = -MulDiv(8, dc.GetDeviceCaps(LOGPIXELSY), 72);
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfClipPrecision = CLIP_LH_ANGLES | CLIP_STROKE_PRECIS;
	strcpy(lf.lfFaceName, "Arial");

#ifdef LR_TABS
	if (nEscapement == 0)
	{
		lf.lfPitchAndFamily = FF_SWISS ;   
	}
	else
	{
		lf.lfPitchAndFamily = FF_SWISS | 0x04;
		lf.lfEscapement = nEscapement;
		lf.lfOrientation = nEscapement;
	}
#else
    lf.lfPitchAndFamily = FF_SWISS ;
#endif

	// Create the normal font
	lf.lfWeight = FW_NORMAL;

	m_tabArray.m_normalFont = new CFont;
	m_tabArray.m_normalFont->CreateFontIndirect(&lf);

	// Create the bold font
	lf.lfWeight = FW_BOLD;

    m_tabArray.m_boldFont = new CFont;
	m_tabArray.m_boldFont->CreateFontIndirect(&lf);
}

void CTabView::destroyFonts()
{
	if(m_tabArray.m_boldFont)
		delete m_tabArray.m_boldFont;
	if(m_tabArray.m_normalFont)
		delete m_tabArray.m_normalFont;
}

/////////////////////////////////////////////////////////////////////////////
// CTabView message handlers

void CTabView::OnActivateView(BOOL bActivate, CView* pActiveView, CView* pDeactiveView)
{
	CView::OnActivateView(bActivate, pActiveView, pDeactiveView);

	if (bActivate && m_nTabs && m_tabArray.m_curTab == -1)
	{
		switchTab(0);
	}
}

BOOL CTabView::OnEraseBkgnd(CDC* pDC)
{
	CBrush backBrush(GetSysColor(COLOR_BTNFACE));
	CBrush* pOldBrush = pDC->SelectObject(&backBrush);
	
	CRect rect;
	pDC->GetClipBox(&rect);	//erase the area needed
	pDC->PatBlt(rect.left,rect.top,rect.Width(),rect.Height(),PATCOPY);
	pDC->SelectObject(pOldBrush);
	return TRUE;
}

void CTabView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	m_width = cx;
	m_height = cy;
	
	for(int i=0;i<m_nTabs;i++)
	{
		CTabInfo* pTab = (CTabInfo*)m_tabArray[i];

#ifdef LR_TABS
		switch (m_tabArray.m_position)
		{
			case TABSONLEFT:
			case TABSONLEFTBOT:
			case TABSONRIGHT:
			case TABSONRIGHTBOT:
				pTab->m_pView->SetWindowPos(NULL,
											0,
											0,
											cx - 2 * (m_tabArray.m_margin + 5) - m_tabArray.m_tabHeight,
											cy - 2 * (m_tabArray.m_margin + 5),
											SWP_NOMOVE|SWP_NOZORDER);
				break;

			default:
#endif
#ifdef T_TABS
				pTab->m_pView->SetWindowPos(NULL,
											0,
											0,
											cx - 2 * (m_tabArray.m_margin + 5),
											cy - 2 * (m_tabArray.m_margin + 5) - m_tabArray.m_tabHeight,
											SWP_NOMOVE|SWP_NOZORDER);
#endif
#ifdef LR_TABS

				break;
		}
#endif
	}
}

void CTabView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CView::OnLButtonDown(nFlags, point);

#ifdef LR_TABS
	switch (m_tabArray.m_position)
	{
		case TABSONLEFT:
		case TABSONLEFTBOT:
		case TABSONRIGHT:
		case TABSONRIGHTBOT:
			if ( switchVerticalTab(point) )
				return;

		default:
#endif
#ifdef T_TABS
			if ( switchTopTab(point) )
				return;
#endif
#ifdef LR_TABS
	}
#endif
	GetParentFrame()->SetActiveView(m_curView);
}

#ifdef LR_TABS
BOOL CTabView::switchVerticalTab(CPoint point)
{
	CRect rect;
	int x = (m_tabArray.m_position == TABSONLEFT || m_tabArray.m_position == TABSONLEFTBOT) ? m_tabArray.m_margin : 
		(m_width - m_tabArray.m_margin - m_tabArray.m_tabHeight);
	int y = (m_tabArray.m_position == TABSONLEFT || m_tabArray.m_position == TABSONRIGHT) ? m_tabArray.m_margin:(m_height - m_tabArray.m_margin);
	rect.left = x;
	rect.right = rect.left + m_tabArray.m_tabHeight;
	
	for(int i = 0; i < m_nTabs; i++)
	{
		CTabInfo* pTab = (CTabInfo*)m_tabArray[i];
		if(m_tabArray.m_position == TABSONLEFT || m_tabArray.m_position == TABSONRIGHT)
		{
			rect.top 	= y;
			rect.bottom = y + pTab->m_tabWidth;
		}
		else
		{
			rect.bottom = y;
			rect.top = rect.bottom - pTab->m_tabWidth;
		}
		if(rect.PtInRect(point) && pTab->m_active)
		{
			switchTab(i);
			return TRUE;
		}
		if(m_tabArray.m_position == TABSONLEFT || m_tabArray.m_position == TABSONRIGHT)
			y += pTab->m_tabWidth;
		else
			y -= pTab->m_tabWidth;
	}
	return(FALSE);
}
#endif

#ifdef T_TABS
BOOL CTabView::switchTopTab(CPoint point)
{
	CRect rect;
	int x = m_tabArray.m_margin, y = m_tabArray.m_margin;

	rect.top = y;
	rect.bottom = y + m_tabArray.m_tabHeight;

	for(int i = 0; i < m_nTabs; i++)
	{
		CTabInfo* pTab = (CTabInfo*)m_tabArray[i];
		rect.left = x;
		rect.right = rect.left + pTab->m_tabWidth;
		if(rect.PtInRect(point) && pTab->m_active)
		{
			switchTab(i);
			return TRUE;
		}
		x += pTab->m_tabWidth;
	}
	return(FALSE);
}
#endif

void CTabView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	createFonts();
	
	CFrameWnd* pFrame = GetParentFrame();
	pFrame->RecalcLayout();
	m_tabArray.m_curTab = -1;
}

CView* CTabView::addTabView(
	CRuntimeClass* viewClass,
	CDocument* document,
	char* tabLabel,
	BOOL border,
	BOOL show,
	int tabWidth
	)
{
	// Setup defaults

	if(!tabLabel)
		return NULL;

	CTabInfo* pTab = new CTabInfo;
	if(!pTab)
		return NULL;
		
	m_tabArray.m_curTab++;
	pTab->m_pView = createTabView(viewClass,document,this,border,show);
	if(!pTab->m_pView)
	{
		delete pTab;
		m_tabArray.m_curTab--;
		return NULL;
	}
	pTab->m_tabWidth = tabWidth;
	int sLen = strlen(tabLabel);
	pTab->m_tabLabel = (char *)new char[sLen + 1 ];
	strcpy(pTab->m_tabLabel,tabLabel);
	for(int i=0;i<sLen;i++)
	{
		if(tabLabel[i] == '&' && i != (sLen -1))
		{
			pTab->m_mnemonic = (char)(DWORD)CharUpper((LPSTR)tabLabel[i+1]);
#ifdef LR_TABS
			if(m_tabArray.m_position == TABSONLEFT ||
			   m_tabArray.m_position == TABSONLEFTBOT)
				memmove(&tabLabel[i],&tabLabel[i+1],sLen - i-1);
#endif
			break;
		}
	}
	
	m_tabArray.Add(pTab);
	m_nTabs++;

	return (CView*)pTab->m_pView;
}

CView* CTabView::createTabView(
	CRuntimeClass* pViewClass,
	CDocument* pDoc,
	CWnd*	parentWnd,
	BOOL border,
	BOOL show
	)
{
	CRect viewRect;
	
#ifdef LR_TABS
	switch (m_tabArray.m_position)
	{
		case TABSONLEFT:
		case TABSONLEFTBOT:
			viewRect.SetRect(m_tabArray.m_margin + 5 + m_tabArray.m_tabHeight,
							 m_tabArray.m_margin + 5,
							 m_width - (m_tabArray.m_margin + 5),
							 m_height - (m_tabArray.m_margin + 5));
			break;

		case TABSONRIGHT:
		case TABSONRIGHTBOT:
			viewRect.SetRect(m_tabArray.m_margin + 5,
							 m_tabArray.m_margin + 5,
							 m_width - (m_tabArray.m_margin + 5),
							 m_height- (m_tabArray.m_margin + 5));
			break;

		default:
#endif
#ifdef T_TABS
			viewRect.SetRect(m_tabArray.m_margin + 5,
							 m_tabArray.m_margin + 5 + m_tabArray.m_tabHeight,
							 m_width - (m_tabArray.m_margin + 5),
							 m_height- (m_tabArray.m_margin + 5));
#endif
#ifdef LR_TABS
			break;
	}
#endif

	CView* pView = (CView*)pViewClass->CreateObject();
    ASSERT(pView != NULL);
    
  	CCreateContext context;
   	context.m_pNewViewClass		= pViewClass;
   	context.m_pCurrentDoc		= pDoc;
   	context.m_pNewDocTemplate	= NULL;
   	context.m_pLastView			= NULL;
   	context.m_pCurrentFrame		= GetParentFrame();
    if (!pView->Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,viewRect,
		this, AFX_IDW_PANE_FIRST+1+m_tabArray.m_curTab, &context))
	{
	    	TRACE0("Warning: couldn't create view for frame\n");
	    	return NULL;
	}

	if(show)
	{
		pView->SetWindowPos(NULL, 0, 0, 0, 0,
		                    SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		GetParentFrame()->SetActiveView(pView);
	}
	else
		pView->SetWindowPos(NULL, 0, 0, 0, 0,
		                    SWP_HIDEWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);

	long style = ::GetWindowLong(pView->GetSafeHwnd(), GWL_STYLE);
	if(!border)
		style &= ~WS_BORDER;
	else
		style |= WS_BORDER;
	::SetWindowLong(pView->GetSafeHwnd(), GWL_STYLE, style);
    //Get rid of 3D border --
	pView->ModifyStyleEx(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE,0);

	return pView;
}

void CTabView::switchTab(int viewIndex)
{
	if(viewIndex < 0 || viewIndex >= m_nTabs)
		return;
	CTabInfo* pTab = (CTabInfo*)m_tabArray[viewIndex];

	if(m_curView != pTab->m_pView)
	{
		if (m_curView)
		{
			m_curView->GetDocument()->UpdateAllViews(this, CTabView::hintSwitchFrom, m_curView);
			m_curView->ShowWindow(SW_HIDE);
		}

		if (pTab->m_pView)
		{
			CView* pView = (CView*)pTab->m_pView;
			pView->GetDocument()->UpdateAllViews(this, CTabView::hintSwitchTo, pView);
			pView->ShowWindow(SW_SHOW);
		}

		m_curView = (CView*)pTab->m_pView;
		m_tabArray.m_curTab = viewIndex;
		InvalidateRect(NULL);
	}
	GetParentFrame()->SetActiveView(m_curView);
}

void CTabView::enableView(int viewIndex, BOOL bEnable)
{
	CTabInfo* pTab = (CTabInfo*)m_tabArray[viewIndex];
	if(viewIndex < 0 || viewIndex >= m_nTabs)
		return;

//can't disable current view
	if(m_curView != pTab->m_pView && bEnable != pTab->m_active)
	{
		pTab->m_active = bEnable;
		if(IsWindowVisible()) {
		  RECT rect;
		  m_tabArray.GetTabRect(&rect,viewIndex);
		  InvalidateRect(&rect);
		}
	}
}

BOOL CTabView::doSysCommand(UINT nID,LONG lParam)
{
	WPARAM wCmdType = nID & 0xFFF0;

	if(wCmdType==SC_KEYMENU && !HIWORD(lParam)) {
		char mnemonic = (char)(DWORD)CharUpper((LPSTR)lParam);
		for(int i = 0; i < m_nTabs; i++) {
			CTabInfo* pTab = (CTabInfo*)m_tabArray[i];
			if(mnemonic == pTab->m_mnemonic && pTab->m_active)
			{
				switchTab(i);
				return 1;
			}
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CTabView drawing

void CTabView::OnDraw(CDC* pDC)
{
#ifdef C_TABS
#ifdef LR_TABS 
	if(m_lookAndFeel == LAF_CHICAGO && m_tabArray.m_position == TABSONLEFT)
	{
		TRACE0("Warning: left tabs not supported for LAF = CHICAGO");
		m_lookAndFeel = LAF_MSWORD;
	}

	if(m_lookAndFeel == LAF_CHICAGO)
	{	
		m_tabArray.drawChicagoTabs(this, pDC);
	}
	else // MS WORD STYLE LOOK--compliments of David Hollifield
#endif
#endif
	{
		m_tabArray.drawMSWordTabs(this, pDC);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTabView diagnostics

#ifdef _DEBUG
void CTabView::AssertValid() const
{
	CView::AssertValid();
}

void CTabView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

void CTabView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	
	if (m_curView)
	{
		// Set the keyboard focus to this view
		m_curView->SetFocus();
		GetParentFrame()->SetActiveView(m_curView);
		return;
	}
}

int CTabView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	return MA_ACTIVATE;
}

void CTabView::ChangeDoc(CDocument *pDocNew)
{
    CDocument *pDoc=GetDocument();
    for(int viewIndex=m_nTabs;viewIndex--;) {
    	CView* pView = (CView *)((CTabInfo*)m_tabArray[viewIndex])->m_pView;
    	pDoc->RemoveView(pView);
		pDocNew->AddView(pView);
    }
    pDoc->RemoveView(this);
	pDocNew->AddView(this);
}