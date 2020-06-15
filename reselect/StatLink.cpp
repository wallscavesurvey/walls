////////////////////////////////////////////////////////////////
// CStaticLink 1997 Microsoft Systems Journal. 
// If this program works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CStaticLink implements a static control that's a hyperlink
// to any file on your desktop or web. You can use it in dialog boxes
// to create hyperlinks to web sites. When clicked, opens the file/URL
//
#include "StdAfx.h"
#include "reselect.h"
#include "MainFrm.h"
#include "reselectVw.h"
#include "StatLink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CStaticLink, CStatic)

BEGIN_MESSAGE_MAP(CStaticLink, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
	ON_CONTROL_REFLECT(STN_CLICKED, OnClicked)
END_MESSAGE_MAP()

///////////////////
// Constructor sets default colors = blue/purple.
//
CStaticLink::CStaticLink()
{
	m_colorUnvisited = RGB(0, 0, 255);		 // blue
	m_colorVisited = RGB(128, 0, 128);	 // purple
	m_bVisited = FALSE;				 // not visited yet
}

//////////////////
// Handle reflected WM_CTLCOLOR to set custom control color.
// For a text control, use visited/unvisited colors and underline font.
// For non-text controls, do nothing. Also ensures SS_NOTIFY is on.
//
HBRUSH CStaticLink::CtlColor(CDC* pDC, UINT nCtlColor)
{
	ASSERT(nCtlColor == CTLCOLOR_STATIC);
	DWORD dwStyle = GetStyle();
	if (!(dwStyle & SS_NOTIFY)) {
		// Turn on notify flag to get mouse messages and STN_CLICKED.
		// Otherwise, I'll never get any mouse clicks!
		::SetWindowLong(m_hWnd, GWL_STYLE, dwStyle | SS_NOTIFY);
	}

	HBRUSH hbr = NULL;
	if ((dwStyle & 0xFF) <= SS_RIGHT) {

		// this is a text control: set up font and colors
		if (!(HFONT)m_font) {
			// first time init: create font
			LOGFONT lf;
			GetFont()->GetObject(sizeof(lf), &lf);
			lf.lfUnderline = TRUE;
			m_font.CreateFontIndirect(&lf);
		}

		// use underline font and visited/unvisited colors
		pDC->SelectObject(&m_font);
		pDC->SetTextColor(m_bVisited ? m_colorVisited : m_colorUnvisited);
		pDC->SetBkMode(TRANSPARENT);

		// return hollow brush to preserve parent background color
		// Changed *** DMcK *** (was hollow)
		hbr = (HBRUSH)::GetStockObject(LTGRAY_BRUSH);
	}
	return hbr;
}

/////////////////
// Handle mouse click: open URL/file.
//
void CStaticLink::OnClicked()
{
	((CMainFrame *)AfxGetMainWnd())->OnGoTss();
}
