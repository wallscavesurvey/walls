/* ==========================================================================
	File :			RuleRichEditCtrl.cpp

	Class :			CRulerRichEditCtrl

	Author :		Johan Rosengren, Abstrakt Mekanik AB
					Iain Clarke

	Date :			2004-04-17

	Purpose :		"CRulerRichEditCtrl" is a "CWnd" derived class containing an
					embedded RTF-control, a ruler-control with dragable tab-
					positions and a formatting toolbar. The class can be used
					to - for example - add a complete mini-editor to a modal
					or modeless dialog box.

	Description :	The class mainly handles mouse messages. The mouse
					messages are sent from the ruler control, and are
					button down, where the a check is made for the cursor
					located on one of the tab-markers, mouse move, where an
					XORed line is drawn across the RTF-control and button up,
					where a new tab position is set. The class also handles
					the toolbar buttons, setting styles as
					appropriate for the selected text.

	Usage :			Add a "CRulerRichEditCtrl"-member to the parent class.
					Call Create to create the control. "GetRichEditCtrl" can
					be used to access the embedded RTF-control. Remember to
					call "AfxInitRichEdit(2)"!

					The contents can be saved to disk by calling "Save", and
					loaded from disk by calling "Load". The two functions
					will automatically display a file dialog if the file
					name parameter of the calls are left empty.

					"GetRTF" and "SetRTF" can be used to get and set the
					contents of the embedded RTF-control as RTF
					respectively.

					The ruler measures can be displayed as inches or
					centimeters, by calling "SetMode". "GetMode" will get the
					current mode.

   ========================================================================*/

#include "stdafx.h"
#include "MainFrm.h"
#include "shplayer.h"
#include "logfont.h"
#include "resource.h"
#include "ids.h"

#include <algorithm>
#include "StdGrfx.h"
#include "TextFile.h"
   //includes rulerricheditctrl.h --
#include "dbteditdlg.h"
#include "TableFillDlg.h"
#include "EditImageDlg.h"
extern const UINT WM_SWITCH_EDITMODE;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Registered messages for ruler/CRulerRichEditCtrl communication

UINT urm_RULERACTION = ::RegisterWindowMessage(_T("_RULERRICHEDITCTRL_RULER_TRACK_"));
UINT urm_GETSCROLLPOS = ::RegisterWindowMessage(_T("_RULERRICHEDITCTRL_GET_SCROLL_POS_"));
UINT urm_SETCURRENTFONTNAME = ::RegisterWindowMessage(_T("_RULERRICHEDITCTRL_SET_CURRENT_FONT_NAME"));
UINT urm_SETCURRENTFONTSIZE = ::RegisterWindowMessage(_T("_RULERRICHEDITCTRL_SET_CURRENT_FONT_SIZE"));
UINT urm_SETCURRENTFONTCOLOR = ::RegisterWindowMessage(_T("_RULERRICHEDITCTRL_SET_CURRENT_FONT_COLOR"));
UINT urm_DATACHANGED = ::RegisterWindowMessage(_T("_RULERRICHEDITCTRL_DATA_CHANGED"));
UINT urm_LINKCLICKED = ::RegisterWindowMessage(_T("_RULERRICHEDITCTRL_LINK_CLICKED"));

/////////////////////////////////////////////////////////////////////////////
// Stream callback functions
// Callbacks to the Save and Load functions.

static DWORD CALLBACK StreamOut(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{

	// Setting up temp buffer
	char*	buff;
	buff = new char[cb + 1];
	buff[cb] = (char)0;
	strncpy(buff, (LPCSTR)pbBuff, cb);
	int max = strlen(buff);

	CString* str = (CString*)dwCookie;

#ifdef _UNICODE

	// We want to convert the buff to wide chars
	int length = ::MultiByteToWideChar(CP_UTF8, 0, buff, max, NULL, 0);
	if (length)
	{
		TCHAR* wBuff = new TCHAR[length + 1];
		::MultiByteToWideChar(CP_UTF8, 0, buff, max, wBuff, length + 1);
		wBuff[length] = 0;
		*str += wBuff;
		delete[] wBuff;
	}

#else

	*str += buff;

#endif

	delete[] buff;
	*pcb = max;

	return 0;

}

static DWORD CALLBACK StreamIn(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{

	CString* str = ((CString*)dwCookie);

#ifdef _UNICODE

	// Unicode is only supported for SF_TEXT, so we need
	// to convert
	LPCTSTR ptr = str->GetBuffer((*str).GetLength());
	int length = ::WideCharToMultiByte(CP_UTF8, 0, ptr, -1, NULL, 0, NULL, NULL);
	int max = min(cb, length);
	if (length)
	{
		char* buff = new char[length];
		::WideCharToMultiByte(CP_UTF8, 0, ptr, -1, buff, length + 1, NULL, NULL);
		strncpy((LPSTR)pbBuff, buff, max);
		delete[] buff;
	}
	str->ReleaseBuffer();

#else

	int max = min(cb, (*str).GetLength());
	strncpy((LPSTR)pbBuff, (*str), max);

#endif

	(*str) = (*str).Right((*str).GetLength() - max);

	*pcb = max;

	return 0;

}

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl

CRulerRichEditCtrl::CRulerRichEditCtrl() : m_pen(PS_DOT, 0, RGB(0, 0, 0))
/* ============================================================
	Function :		CRulerRichEditCtrl::CRulerRichEditCtrl
	Description :	constructor
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :

   ============================================================*/
{

	m_readOnly = m_bFormatRTF = m_showToolbar = m_showRuler = FALSE;
	m_margin = m_offset = m_rulerPosition = 0;
	m_movingtab = -1;
	m_pFindDlg = NULL;
	m_richEditModule = LoadLibrary(_T("Riched20.dll"));
}

CRulerRichEditCtrl::~CRulerRichEditCtrl()
/* ============================================================
	Function :		CRulerRichEditCtrl::~CRulerRichEditCtrl
	Description :	destructor
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :

   ============================================================*/
{

	if (m_richEditModule)
		FreeLibrary(m_richEditModule);

	//ASSERT(IsBadReadPtr(m_pFindDlg,sizeof(CRulerFindDlg)));
	m_pen.DeleteObject();
}


BOOL CRulerRichEditCtrl::Create(DWORD dwStyle, const RECT &rect, CWnd* pParentWnd,
	UINT nID, const CLogFont *pFont, COLORREF clrTxt)
	/* ============================================================
		Function :		CRulerRichEditCtrl::Create
		Description :	Creates the control and sub controls.
		Access :		Public

		Return :		BOOL				-	"TRUE" if created OK.
		Parameters :	DWORD dwStyle		-	Style of the control,
												normally "WS_CHILD"
												and "WS_VISIBLE".
						const RECT &rect	-	Placement rectangle.
						CWnd* pParentWnd	-	Parent window.
						UINT idContext      -   Context menu id
						UINT nID			-	Control ID

		Usage :			Call to create the control.

	   ============================================================*/
{

	BOOL result = CWnd::Create(NULL, _T(""), dwStyle, rect, pParentWnd, nID);
	if (result)
	{

		result = FALSE;
		// Save screen resolution for
		// later on.
		CClientDC dc(this);
		m_pxPerInch = dc.GetDeviceCaps(LOGPIXELSX);

		// Create sub-controls
		if (CreateRTFControl(pFont, clrTxt))
		{
			CreateMargins();
			return TRUE;
		}
	}

	return result;

}

BOOL CRulerRichEditCtrl::CreateToolbar()
{
	CRect rect;
	GetClientRect(rect);

	CRect toolbarRect(0, 0, rect.right, TOOLBAR_HEIGHT);
	return m_toolbar.Create(this, toolbarRect);
}

BOOL CRulerRichEditCtrl::CreateRuler()
{

	CRect rect;
	GetClientRect(rect);

	CRect rulerRect(0, TOOLBAR_HEIGHT, rect.right, TOOLBAR_HEIGHT + RULER_HEIGHT);
	return m_ruler.Create(rulerRect, this, RULER_CONTROL);
}

BOOL CRulerRichEditCtrl::CreateRTFControl(const CLogFont *pFont, COLORREF clrTxt)
/* ============================================================
	Function :		CRulerRichEditCtrl::CreateRTFControl
	Description :	Creates the embedded RTF-control.
	Access :		Private

	Return :		BOOL				-	"TRUE" if created ok.

	Usage :			Called during control creation

   ============================================================*/
{
	BOOL result = FALSE;

	CRect rect;
	GetClientRect(rect);

	//int top = TOOLBAR_HEIGHT + RULER_HEIGHT;
	int top = 0;
	CRect rtfRect(0, top, rect.right, rect.bottom);
	DWORD style = ES_NOHIDESEL | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_WANTRETURN | ES_MULTILINE;
	//if( autohscroll )	style |= ES_AUTOHSCROLL;

	if (m_rtf.Create(style, rtfRect, this))
	{

		// Setting up default tab stops
		ParaFormat para(PFM_TABSTOPS);
		para.cTabCount = MAX_TAB_STOPS;
		for (int t = 0; t < MAX_TAB_STOPS; t++)
			para.rgxTabs[t] = 640 * (t + 1);

		m_rtf.SetParaFormat(para);

		// Setting default character format
		CharFormat	cf;
		//cf.dwMask = CFM_SIZE | CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
		cf.dwMask = CFM_SIZE | CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_COLOR;

		cf.crTextColor = clrTxt;
		//cf.crBackColor=RGB(0x38,0x8E,0x8E); //only text, not window

		//Compute cf.yHeight = number of twips, or 1/1440 inches (original default was 200)
		//m_pxPerInch == CDC::GetDeviceCaps(LOGPIXELSY) 

		ASSERT(pFont->lf.lfHeight < 0);
		cf.yHeight = 20 * (LONG)((-pFont->lf.lfHeight*72.0) / m_pxPerInch + 0.5);

		cf.dwEffects = 0;
		if (pFont->lf.lfWeight == FW_BOLD) cf.dwEffects |= CFE_BOLD;
		if (pFont->lf.lfItalic) cf.dwEffects |= CFE_ITALIC;
		//if(..) cf.dwEffects|=CFE_AUTOCOLOR;

		lstrcpy(cf.szFaceName, pFont->lf.lfFaceName);

		m_rtf.SendMessage(EM_SETCHARFORMAT, 0, (LPARAM)&cf);

		// Set the internal tabs array
		SetTabStops((LPLONG)(para.rgxTabs), MAX_TAB_STOPS);

		m_rtf.SendMessage(EM_AUTOURLDETECT, TRUE, 0);

		m_rtf.SetEventMask(m_rtf.GetEventMask() | ENM_SELCHANGE | ENM_SCROLL | ENM_CHANGE | ENM_LINK);
		SetReadOnly(GetReadOnly());


		result = TRUE;
	}

	return result;

}

void CRulerRichEditCtrl::CreateMargins()
/* ============================================================
	Function :		CRulerRichEditCtrl::CreateMargins
	Description :	Sets the margins for the subcontrols and
					the RTF-control edit rect.
	Access :		Private

	Return :		void
	Parameters :	none

	Usage :			Called during control creation.

   ============================================================*/
{

	// Set up edit rect margins
	int scmargin = 4;
	CRect rc;
	m_rtf.GetClientRect(rc);

	rc.top = scmargin;
	rc.left = scmargin * 2;
	rc.right -= scmargin * 2;

	m_rtf.SetRect(rc);

	// Get the diff between the window- and client 
	// rect of the RTF-control. This gives the actual 
	// size of the RTF-control border.
	CRect	r1;
	CRect	r2;

	m_rtf.GetWindowRect(r1);
	m_rtf.GetClientRect(r2);
	m_rtf.ClientToScreen(r2);

	// Create the margin for the toolbar 
	// controls and the ruler.
	m_margin = scmargin * 2 + r2.left - r1.left;
	m_ruler.SetMargin(m_margin);

}

BEGIN_MESSAGE_MAP(CRulerRichEditCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_MESSAGE(WM_GETTEXT, OnGetText)
	ON_MESSAGE(WM_GETTEXTLENGTH, OnGetTextLength)
	ON_BN_CLICKED(BUTTON_FONT, OnButtonFont)
	//ON_BN_CLICKED(BUTTON_COLOR, OnButtonColor)
	ON_BN_CLICKED(BUTTON_BOLD, OnButtonBold)
	ON_BN_CLICKED(BUTTON_ITALIC, OnButtonItalic)
	ON_BN_CLICKED(BUTTON_UNDERLINE, OnButtonUnderline)
	ON_BN_CLICKED(BUTTON_LEFTALIGN, OnButtonLeftAlign)
	ON_BN_CLICKED(BUTTON_CENTERALIGN, OnButtonCenterAlign)
	ON_BN_CLICKED(BUTTON_RIGHTALIGN, OnButtonRightAlign)
	ON_BN_CLICKED(BUTTON_INDENT, OnButtonIndent)
	ON_BN_CLICKED(BUTTON_OUTDENT, OnButtonOutdent)
	ON_BN_CLICKED(BUTTON_BULLET, OnButtonBullet)
	ON_WM_SETFOCUS()

	ON_REGISTERED_MESSAGE(urm_RULERACTION, OnTrackRuler)
	ON_REGISTERED_MESSAGE(urm_GETSCROLLPOS, OnGetScrollPos)
	ON_REGISTERED_MESSAGE(urm_SETCURRENTFONTNAME, OnSetCurrentFontName)
	ON_REGISTERED_MESSAGE(urm_SETCURRENTFONTSIZE, OnSetCurrentFontSize)
	ON_REGISTERED_MESSAGE(urm_SETCURRENTFONTCOLOR, OnSetCurrentFontColor)
	ON_REGISTERED_MESSAGE(urm_DATACHANGED, OnDataChanged)
	ON_REGISTERED_MESSAGE(WM_FINDDLGMESSAGE, OnFindDlgMessage)

	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectAll)
	ON_COMMAND(ID_SHOWRULER, OnShowRuler)
	//ON_COMMAND(ID_SHOWTOOLBAR,OnShowToolbar)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FORMAT_PLAIN, OnFormatRTF)
	ON_COMMAND(ID_FORMAT_RTF, OnFormatRTF)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
	ON_COMMAND(ID_INS_LINKTOFILE, OnLinkToFile)
	ON_COMMAND(ID_SAVETOFILE, OnSaveToFile)
	//ON_COMMAND(ID_INS_LINKFORVIEWER,OnLinkForViewer)
	ON_COMMAND(ID__COMPAREWITHFILE, OnCompareWithFile)
	ON_COMMAND(ID__REPLACEWITHFILE, OnReplaceWithFile)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl message handlers

void CRulerRichEditCtrl::OnPaint()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnPaint
	Description :	Paints the ruler.
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	CPaintDC mainDC(this);
	UpdateTabStops();

}

BOOL CRulerRichEditCtrl::OnEraseBkgnd(CDC* /*pDC*/)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnEraseBkgnd
	Description :	Returns "TRUE" to avoid flicker.
	Access :		Protected

	Return :		BOOL		-	Always "TRUE",
	Parameters :	CDC* pDC	-	Not used

	Usage :			Called from MFC.

   ============================================================*/
{

	return TRUE;

}

BOOL CRulerRichEditCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnNotify
	Description :	Called as the RTF-control is updated or
					the selection changes.
	Access :		Protected

	Return :		BOOL				-	From base class
	Parameters :	WPARAM wParam		-	Control ID
					LPARAM lParam		-	Not interested
					LRESULT* pResult	-	Not interested

	Usage :			Called from MFC. We must check the control
					every time the selection changes or the
					contents are changed, as the cursor might
					have entered a new paragraph, with new tab
					and/or font settings.

   ============================================================*/
{

	if (wParam == RTF_CONTROL && m_showToolbar)
	{

		// Update the toolbar
		UpdateToolbarButtons();

		// Update ruler
		CRect rect;
		GetClientRect(rect);
		rect.top = TOOLBAR_HEIGHT;
		rect.bottom = rect.top + RULER_HEIGHT;

		RedrawWindow(rect);
	}

	return CWnd::OnNotify(wParam, lParam, pResult);

}

void CRulerRichEditCtrl::OnSize(UINT nType, int cx, int cy)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSize
	Description :	We resize the embedded RTF-control.
	Access :		Protected

	Return :		void
	Parameters :	UINT nType	-	Not interested
					int cx		-	New width
					int cy		-	New height

	Usage :			Called from MFC.

   ============================================================*/
{

	CWnd::OnSize(nType, cx, cy);

	if (m_rtf.m_hWnd)
	{

		LayoutControls(cx, cy);

	}

}

void CRulerRichEditCtrl::OnSetFocus(CWnd* pOldWnd)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetFocus
	Description :	We handle over the focus to the embedded
					RTF-control.
	Access :		Protected

	Return :		void
	Parameters :	CWnd* pOldWnd	-	Not used

	Usage :			Called from MFC.

   ============================================================*/
{

	CWnd::OnSetFocus(pOldWnd);
	m_rtf.SetFocus();

}

LRESULT CRulerRichEditCtrl::OnGetScrollPos(WPARAM, LPARAM)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnGetScrollPos
	Description :	The function handles the registered message
					"urm_GETSCROLLPOS", that is sent from the
					ruler to get the current scroll position
					of the embedded RTF-control.
	Access :		Protected

	Return :		LRESULT		-	Current scroll pos
	Parameters :	WPARAM mode	-	Not used
					LPARAM pt	-	Not used

	Usage :			Called from MFC

   ============================================================*/
{

	return m_rtf.GetScrollPos(SB_HORZ);

}

LRESULT CRulerRichEditCtrl::OnTrackRuler(WPARAM mode, LPARAM pt)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnTrackRuler
	Description :	The function handles the registered message
					"urm_RULERACTION", that is sent from the
					mouse handling mappings in the ruler control.
					The function handles dragging of tabulator
					points in the ruler.
	Access :		Protected

	Return :		LRESULT		-	Not used
	Parameters :	WPARAM mode	-	The type of mouse operation,
									"DOWN", "MOVE" or "UP"
					LPARAM pt	-	Cursor point for the cursor.

	Usage :			Called from MFC.

   ============================================================*/
{

	CPoint* point = (CPoint*)pt;
	int toolbarHeight = 0;
	if (m_showToolbar)
		toolbarHeight = TOOLBAR_HEIGHT;

	switch (mode)
	{
	case DOWN:
		// The left mouse button is clicked
	{
		// Check if we clicked on a tab-marker.
		int pos = m_rtf.GetScrollPos(SB_HORZ);
		m_movingtab = -1;
		CRect hitRect;
		int y = RULER_HEIGHT - 9;
		for (int t = 0; t < MAX_TAB_STOPS; t++)
		{
			int x = m_tabs[t] + m_margin - pos;
			hitRect.SetRect(x - 2, y - 1, x + 3, y + 3);
			if (hitRect.PtInRect(*point))
			{
				// Yes, we did.
				m_movingtab = t;

				// Calc offset, as the hit area is wider than
				// the 1 pixel tab line
				m_offset = point->x - (m_tabs[t] + m_margin - pos);
			}
		}

		if (m_movingtab != -1)
		{

			// We did click in a tab marker.
			// Start dragging

			// Find initial ruler position
			m_rulerPosition = point->x - m_offset;
			CRect rect;
			GetClientRect(rect);

			// Draw a new ruler line
			CClientDC dc(this);
			dc.SelectObject(&m_pen);
			dc.SetROP2(R2_XORPEN);

			dc.MoveTo(m_rulerPosition, toolbarHeight + 3);
			dc.LineTo(m_rulerPosition, rect.Height());

			dc.SelectStockObject(BLACK_PEN);

		}
	}
	break;

	case MOVE:
		// The mouse is moved
	{
		if (m_movingtab != -1)
		{
			CRect rect;
			GetClientRect(rect);
			CClientDC dc(this);

			// Erase previous line
			dc.SelectObject(&m_pen);
			dc.SetROP2(R2_XORPEN);

			dc.MoveTo(m_rulerPosition, toolbarHeight + 3);
			dc.LineTo(m_rulerPosition, rect.Height());

			// Set up new line
			// Calc min and max. We can not place this marker 
			// before the previous or after the next. Neither 
			// can we move the marker outside the ruler.
			int pos = m_rtf.GetScrollPos(SB_HORZ);
			int min = m_margin + m_offset;
			if (m_movingtab > 0)
				min = (m_tabs[m_movingtab - 1] + m_margin - pos) + 3 + m_offset;

			int max = rect.Width() - 5 + m_offset;
			if (m_movingtab < m_tabs.GetUpperBound())
				max = (m_tabs[m_movingtab + 1] + m_margin - pos) - 3 + m_offset;
			max = min(max, rect.Width() - 5 + m_offset);

			// Set new positions
			m_rulerPosition = max(min, point->x - m_offset);
			m_rulerPosition = min(m_rulerPosition, max);

			// Draw the new line
			dc.MoveTo(m_rulerPosition, toolbarHeight + 3);
			dc.LineTo(m_rulerPosition, rect.Height());

			dc.SelectStockObject(BLACK_PEN);

		}
	}
	break;

	case UP:
		// The mouse button is released
	{
		if (m_movingtab != -1)
		{
			CRect rect;
			GetClientRect(rect);
			CClientDC dc(this);
			dc.SelectObject(&m_pen);
			dc.SetROP2(R2_XORPEN);
			dc.MoveTo(m_rulerPosition, toolbarHeight + 3);
			dc.LineTo(m_rulerPosition, rect.Height());

			// Set new value for tab position
			int pos = m_rtf.GetScrollPos(SB_HORZ);
			m_tabs[m_movingtab] = m_rulerPosition - m_margin + pos - m_offset;

			// Get the current tabstops, as we
			// must set all tabs in one operation
			ParaFormat para(PFM_TABSTOPS);
			para.cTabCount = MAX_TAB_STOPS;
			m_rtf.GetParaFormat(para);

			// Convert current position to twips
			double twip = (double)m_pxPerInch / 1440;
			int tabpos = m_tabs[m_movingtab];
			tabpos = (int)((double)tabpos / twip + .5);
			para.rgxTabs[m_movingtab] = tabpos;

			// Set tabs to control
			m_rtf.SetParaFormat(para);

			// Erase the ruler
			//m_ruler.RedrawWindow();
			//m_rtf.RedrawWindow();
			// Erase previous line


			m_movingtab = -1;
			m_rtf.SetFocus();

		}
	}
	break;
	}

	return 0;

}

LRESULT CRulerRichEditCtrl::OnSetText(WPARAM wParam, LPARAM lParam)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetText
	Description :	The function handles the "WM_SETTEXT"
					message. The handler sets the text in the
					RTF-control
	Access :		Protected

	Return :		LRESULT			-	From the control
	Parameters :	WPARAM wParam	-	Passed on
					LPARAM lParam	-	Passed on

	Usage :			Called from MFC.

   ============================================================*/
{

	return m_rtf.SendMessage(WM_SETTEXT, wParam, lParam);

}

LRESULT CRulerRichEditCtrl::OnGetText(WPARAM wParam, LPARAM lParam)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnGetText
	Description :	The function handles the "WM_GETTEXT"
					message. The handler gets the text from the
					RTF-control
	Access :		Protected

	Return :		LRESULT			-	From the control
	Parameters :	WPARAM wParam	-	Passed on
					LPARAM lParam	-	Passed on

	Usage :			Called from MFC.

   ============================================================*/
{

	return m_rtf.SendMessage(WM_GETTEXT, wParam, lParam);

}

LRESULT CRulerRichEditCtrl::OnGetTextLength(WPARAM, LPARAM)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnGetTextLength
	Description :	The function handles the "WM_GETTEXTLENGTH"
					message. The handler gets the length of
					the text in the RTF-control
	Access :		Protected

	Return :		LRESULT			-	From the control
	Parameters :	WPARAM wParam	-	Passed on
					LPARAM lParam	-	Passed on

	Usage :			Called from MFC.

   ============================================================*/
{

	return m_rtf.GetTextLengthEx(GTL_PRECISE);

}

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl public implementation

CString CRulerRichEditCtrl::GetRTF()
/* ============================================================
	Function :		CRulerRichEditCtrl::GetRTF
	Description :	Returns the contents of the control as RTF.
	Access :		Public

	Return :		CString	-	The RTF-contents of the control.
	Parameters :	none

	Usage :			Call this function to get a char buffer
					with the contents of the embedded RTF-
					control.

   ============================================================*/
{
	CString str;
	GetRTF(&str);
	return str;
}

void CRulerRichEditCtrl::GetRTF(CString *pStr)
{
	EDITSTREAM	es;
	es.dwError = 0;
	es.dwCookie = (DWORD)pStr;
	es.pfnCallback = StreamOut;
	m_rtf.StreamOut(SF_RTF, es);
}

void CRulerRichEditCtrl::SetRTF(const CString& rtf)
/* ============================================================
	Function :		CRulerRichEditCtrl::SetRTF
	Description :	Set the contents of the embedded RTF-
					control from rtf.
	Access :		Public

	Return :		void
	Parameters :	const CString& rtf	-	The rtf-contents to
											set.

	Usage :			Call this function to set the RTF-contents
					of the control.

   ============================================================*/
{
	EDITSTREAM	es;
	es.dwCookie = (DWORD)&rtf;
	es.pfnCallback = StreamIn;
	m_rtf.StreamIn(SF_RTF, es);
}

BOOL CRulerRichEditCtrl::Save(CString& filename)
/* ============================================================
	Function :		CRulerRichEditCtrl::Save
	Description :	Saves the contents to the file filename.
					If filename is empty, a file dialog will
					be displayed and the selected name will be
					returned in the "CString".
	Access :		Public

	Return :		BOOL				-	"TRUE" if the file
											was saved.
	Parameters :	CString& filename	-	The file name to save
											to. Can be empty.

	Usage :			Call to save the contents of the embedded
					RTF-control do a file.

   ============================================================*/
{

	BOOL result = TRUE;

	CString* str = new CString;

	EDITSTREAM	es;
	es.dwCookie = (DWORD)str;
	es.pfnCallback = StreamOut;
	m_rtf.StreamOut(SF_RTF, es);

	CTextFile f(_T("rtf"));
	result = f.WriteTextFile(filename, *str);

	delete str;
	return result;

}

BOOL CRulerRichEditCtrl::Load(CString& filename)
/* ============================================================
	Function :		CRulerRichEditCtrl::Load
	Description :	Loads the embedded RTF-control with the
					contents from the file filename.
					If filename is empty, a file dialog will
					be displayed and the selected name will be
					returned in the "CString".
	Access :		Public

	Return :		BOOL				-	"TRUE" if the file
											was loaded.
	Parameters :	CString& filename	-	File name to load
											from. Can be empty.

	Usage :			Call to load an RTF-file to the control.

   ============================================================*/
{

	BOOL result = TRUE;

	CString* str = new CString;
	CTextFile f(_T("rtf"));
	result = f.ReadTextFile(filename, *str);
	if (result)
	{
		EDITSTREAM	es;
		es.dwCookie = (DWORD)str;
		es.pfnCallback = StreamIn;
		m_rtf.StreamIn(SF_RTF, es);
	}

	delete str;
	return result;

}


/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl toolbar button handlers

void CRulerRichEditCtrl::OnButtonFont()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonFont
	Description :	Button handler for the Font button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC

   ============================================================*/
{
	DoFont();
}

#if 0
void CRulerRichEditCtrl::OnButtonColor()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonColor
	Description :	Button handler for the Color button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{
	DoColor();
}
#endif

void CRulerRichEditCtrl::OnButtonBold()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonBold
	Description :	Button handler for the Bold button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoBold();

}

void CRulerRichEditCtrl::OnButtonItalic()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonItalic
	Description :	Button handler for the Italic button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoItalic();

}

void CRulerRichEditCtrl::OnButtonUnderline()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonUnderline
	Description :	Button handler for the Underline button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoUnderline();

}

void CRulerRichEditCtrl::OnButtonLeftAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonLeftAlign
	Description :	Button handler for the Left aligned button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoLeftAlign();

}

void CRulerRichEditCtrl::OnButtonCenterAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonCenterAlign
	Description :	Button handler for the Center button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoCenterAlign();

}

void CRulerRichEditCtrl::OnButtonRightAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonRightAlign
	Description :	Button handler for the Right-aligned button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoRightAlign();

}

void CRulerRichEditCtrl::OnButtonIndent()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonIndent
	Description :	Button handler for the Indent button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoIndent();

}

void CRulerRichEditCtrl::OnButtonOutdent()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonOutdent
	Description :	Button handler for the outdent button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoOutdent();

}

void CRulerRichEditCtrl::OnButtonBullet()
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonBullet
	Description :	Button handler for the Bullet button
	Access :		Protected

	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoBullet();

}

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl private helpers

void CRulerRichEditCtrl::SetTabStops(LPLONG tabs, int size)
/* ============================================================
	Function :		CRulerRichEditCtrl::SetTabStops
	Description :	Set the tab stops in the internal tab stop
					list from the RTF-control, converting the
					twip values to physical pixels.
	Access :		Private

	Return :		void
	Parameters :	LPLONG tabs	-	A pointer to an array of
									"LONG" twip values.
					int size	-	The size of "tabs"

	Usage :			Call to set the tab list.

   ============================================================*/
{

	m_tabs.RemoveAll();

	double twip = (double)m_pxPerInch / 1440;
	for (int t = 0; t < size; t++)
	{
		// Convert from twips to pixels
		int tabpos = *(tabs + t);
		tabpos = (int)((double)tabpos * twip + .5);
		m_tabs.Add(tabpos);

	}

	m_ruler.SetTabStops(m_tabs);
}

void CRulerRichEditCtrl::UpdateTabStops()
/* ============================================================
	Function :		CRulerRichEditCtrl::UpdateTabStops
	Description :	Sets the tabs in the internal tab stop
					list, converting the twip physical (pixel)
					position to twip values.
	Access :		Private

	Return :		void
	Parameters :	none

	Usage :			Call to refresh the tab list from the RTF-
					control. Called from the "OnPaint" handler.

   ============================================================*/
{

	ParaFormat para(PFM_TABSTOPS);
	m_rtf.GetParaFormat(para);
	SetTabStops((LPLONG)(para.rgxTabs), MAX_TAB_STOPS);

}

void CRulerRichEditCtrl::UpdateToolbarButtons()
/* ============================================================
	Function :		CRulerRichEditCtrl::UpdateToolbarButtons
	Description :	Updates the toolbar button, by getting
					formatting information from the currently
					selected text in the embedded RTF-control.
	Access :		Private

	Return :		void
	Parameters :	none

	Usage :			Call as the selection changes in the
					RTF-control

   ============================================================*/
{

	if (m_toolbar.m_hWnd)
	{
		CToolBarCtrl &ctb = m_toolbar.GetToolBarCtrl();
		CharFormat	cf;
		cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
		m_rtf.SendMessage(EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		ParaFormat para(PFM_ALIGNMENT | PFM_NUMBERING);
		m_rtf.GetParaFormat(para);

		// Style
		VERIFY(ctb.SetState(BUTTON_BOLD, TBSTATE_ENABLED | ((cf.dwEffects & CFE_BOLD) ? TBSTATE_CHECKED : 0)));
		ctb.SetState(BUTTON_ITALIC, TBSTATE_ENABLED | ((cf.dwEffects & CFE_ITALIC) ? TBSTATE_CHECKED : 0));
		ctb.SetState(BUTTON_UNDERLINE, TBSTATE_ENABLED | ((cf.dwEffects & CFM_UNDERLINE) ? TBSTATE_CHECKED : 0));

		ctb.SetState(BUTTON_LEFTALIGN, TBSTATE_ENABLED | (para.wAlignment == PFA_LEFT ? TBSTATE_CHECKED : 0));
		ctb.SetState(BUTTON_CENTERALIGN, TBSTATE_ENABLED | (para.wAlignment == PFA_CENTER ? TBSTATE_CHECKED : 0));
		ctb.SetState(BUTTON_RIGHTALIGN, TBSTATE_ENABLED | (para.wAlignment == PFA_RIGHT ? TBSTATE_CHECKED : 0));

		ctb.SetState(BUTTON_BULLET, TBSTATE_ENABLED | (para.wNumbering ? TBSTATE_CHECKED : 0));

		if (cf.dwMask & CFM_FACE)
			m_toolbar.SetFontName(CString(cf.szFaceName));

		if (cf.dwMask & CFM_SIZE)
			m_toolbar.SetFontSize(cf.yHeight / 20);

		if (cf.dwMask & CFM_COLOR)
			m_toolbar.SetFontColor(cf.crTextColor);

	}

}

void CRulerRichEditCtrl::SetEffect(int mask, int effect)
/* ============================================================
	Function :		CRulerRichEditCtrl::SetEffect
	Description :	Sets the effect (bold, italic and/or
					underline) for the currently selected text
					in the embedded RTF-control.
	Access :		Private

	Return :		void
	Parameters :	int mask	-	What effects are valid. See
									the documentation for
									"CHARFORMAT".
					int effect	-	What effects to set. See the
									documentation for "CHARFORMAT".

	Usage :			Called internally from button handlers

   ============================================================*/
{

	CharFormat cf;
	cf.dwMask = mask;
	cf.dwEffects = effect;

	m_rtf.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
	m_rtf.SetFocus();

}

void CRulerRichEditCtrl::SetAlignment(int alignment)
/* ============================================================
	Function :		CRulerRichEditCtrl::SetAlignment
	Description :	Sets the alignment for the currently
					selected text in the embedded RTF-control.
	Access :		Private

	Return :		void
	Parameters :	int alignment	-	Alignment to set. See
										documentation for
										"PARAFORMAT"

	Usage :			Called internally from button handlers

   ============================================================*/
{

	ParaFormat	para(PFM_ALIGNMENT);
	para.wAlignment = (WORD)alignment;

	m_rtf.SetParaFormat(para);
	UpdateToolbarButtons();
	m_rtf.SetFocus();

}

// Virtual interface
void CRulerRichEditCtrl::DoFont()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoFont
	Description :	Externally accessible member to set the
					font of the control
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to set the font of the selected text.

   ============================================================*/
{

	// Get the current font
	//LOGFONT	lf;
	//ZeroMemory( &lf, sizeof( LOGFONT ) );
	CharFormat	cf;
	m_rtf.SendMessage(EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

	/*
	int height;

	// Creating a LOGFONT from the current font settings

	// Font
	if( cf.dwMask & CFM_FACE )
		lstrcpy( lf.lfFaceName, cf.szFaceName );

	if( cf.dwMask & CFM_SIZE )
	{
		double twip = ( double )m_pxPerInch / 1440;
		height = cf.yHeight;
		height = -( int ) ( ( double ) height * twip +.5 );
		lf.lfHeight = height;

	}

	// Effects
	if( cf.dwMask & CFM_BOLD )
	{
		if( cf.dwEffects & CFE_BOLD )
			lf.lfWeight = FW_BOLD;
		else
			lf.lfWeight = FW_NORMAL;
	}

	if( cf.dwMask & CFM_ITALIC )
		if( cf.dwEffects & CFE_ITALIC )
			lf.lfItalic = TRUE;

	if( cf.dwMask & CFM_UNDERLINE )
		if( cf.dwEffects & CFE_UNDERLINE )
			lf.lfUnderline = TRUE;

	CFontDialog	dlg( &lf );
*/

// Show font dialog

	CFontDialog	dlg(cf); //also sets dlg.m_cf.Flags|=CF_EFFECTS

	if (dlg.DoModal() == IDOK)
	{
		// Apply new font
		cf.yHeight = dlg.GetSize() * 2;
		lstrcpy(cf.szFaceName, dlg.GetFaceName());

		cf.dwMask = CFM_FACE | CFM_SIZE;
		cf.dwEffects = 0;

		if (dlg.IsBold())
		{

			cf.dwMask |= CFM_BOLD;
			cf.dwEffects |= CFE_BOLD;

		}

		if (dlg.IsItalic())
		{

			cf.dwMask |= CFM_ITALIC;
			cf.dwEffects |= CFE_ITALIC;

		}

		if (dlg.IsUnderline())
		{

			cf.dwMask |= CFM_UNDERLINE;
			cf.dwEffects |= CFE_UNDERLINE;

		}

		m_rtf.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

	}

	m_rtf.SetFocus();

}

void CRulerRichEditCtrl::SetCurrentFontName(const CString& font)
/* ============================================================
	Function :		CRulerRichEditCtrl::SetCurrentFontName
	Description :	Changes the font of the selected text in
					the editor to "font".
	Access :		Public

	Return :		void
	Parameters :	const CString& font	-	Font name of font
											to change to.

	Usage :			Call to set the font of the selected text
					in the editor.

   ============================================================*/
{
	CharFormat	cf;
	cf.dwMask = CFM_FACE;

	lstrcpy(cf.szFaceName, font);

	m_rtf.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void CRulerRichEditCtrl::SetCurrentFontSize(int size)
/* ============================================================
	Function :		CRulerRichEditCtrl::SetCurrentFontSize
	Description :	Changes the size of the selected text in
					the editor to "size" (measured in
					typographical points).
	Access :		Public

	Return :		void
	Parameters :	int size	-	New size in typographical
									points

	Usage :			Call to change the size of the selected
					text.

   ============================================================*/
{
	CharFormat	cf;
	cf.dwMask = CFM_SIZE;
	cf.yHeight = size * 20;

	m_rtf.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void CRulerRichEditCtrl::SetCurrentFontColor(COLORREF color)
/* ============================================================
	Function :		CRulerRichEditCtrl::SetCurrentFontSize
	Description :	Changes the color of the selected text in
					the editor to "color".
	Access :		Public

	Return :		void
	Parameters :	COLORREF color	-	New color

	Usage :			Call to change the color of the selected
					text.

   ============================================================*/
{

	CharFormat	cf;
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = color;

	m_rtf.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

}

LRESULT CRulerRichEditCtrl::OnSetCurrentFontName(WPARAM font, LPARAM)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetCurrentFontName
	Description :	Handler for the registered message
					"urm_SETCURRENTFONTNAME", called when the
					font name is changed from the toolbar.
	Access :		Protected

	Return :		LRESULT		-	Not used
	Parameters :	WPARAM font	-	Pointer to the new font name
					LPARAM		-	Not used

	Usage :			Called from MFC

   ============================================================*/
{

	CString fnt((LPCTSTR)font);
	SetCurrentFontName(fnt);
	m_rtf.SetFocus();
	return 0;

}

LRESULT CRulerRichEditCtrl::OnSetCurrentFontSize(WPARAM, LPARAM size)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetCurrentFontSize
	Description :	Handler for the registered message
					"urm_SETCURRENTFONTSIZE", called when the
					font size is changed from the toolbar.
	Access :		Protected

	Return :		LRESULT		-	Not used
	Parameters :	WPARAM		-	Not used
					LPARAM size	-	New font size in typographical
									points of the selected text

	Usage :			Called from MFC

   ============================================================*/
{

	SetCurrentFontSize(size);
	m_rtf.SetFocus();
	return 0;

}

LRESULT CRulerRichEditCtrl::OnSetCurrentFontColor(WPARAM, LPARAM color)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetCurrentFontColor
	Description :	Handler for the registered message
					"urm_SETCURRENTFONTCOLOR", called when the
					font color is changed from the toolbar.
	Access :		Protected

	Return :		LRESULT		-	Not used
	Parameters :	WPARAM		-	Not used
					LPARAM		-	New color of the selected
									text

	Usage :			Called from MFC

   ============================================================*/
{

	SetCurrentFontColor((COLORREF)color);
	m_rtf.SetFocus();
	return 0;
}

void CRulerRichEditCtrl::DoColor()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoColor
	Description :	Externally accessible member to set the
					color of the selected text
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to set the color of the selected text.

   ============================================================*/
{

	// Get the current color
	COLORREF	clr(RGB(0, 0, 0));
	CharFormat	cf;
	m_rtf.SendMessage(EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
	if (cf.dwMask & CFM_COLOR)
		clr = cf.crTextColor;

	// Display color selection dialog
	CColorDialog dlg(clr);
	if (dlg.DoModal() == IDOK)
	{

		// Apply new color
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0;
		cf.crTextColor = dlg.GetColor();

		m_rtf.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

	}

	m_rtf.SetFocus();

}

void CRulerRichEditCtrl::DoBold()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoBold
	Description :	Externally accessible member to set/unset
					the selected text to/from bold
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to toggle the selected text to/from
					bold.

   ============================================================*/
{

	m_toolbar.GetToolBarCtrl().CheckButton(BUTTON_BOLD, !m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_BOLD));

	int effect = 0;
	if (m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_BOLD))
		effect = CFE_BOLD;

	SetEffect(CFM_BOLD, effect);

}

void CRulerRichEditCtrl::DoItalic()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoItalic
	Description :	Externally accessible member to set/unset
					the selected text to/from italic
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to toggle the selected text to/from
					italic.

   ============================================================*/
{

	m_toolbar.GetToolBarCtrl().CheckButton(BUTTON_ITALIC, !m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_ITALIC));

	int effect = 0;
	if (m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_ITALIC))
		effect = CFE_ITALIC;

	SetEffect(CFM_ITALIC, effect);

}

void CRulerRichEditCtrl::DoUnderline()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoUnderline
	Description :	Externally accessible member to set/unset
					the selected text to/from underline
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to toggle the selected text to/from
					underlined.

   ============================================================*/
{

	m_toolbar.GetToolBarCtrl().CheckButton(BUTTON_UNDERLINE, !m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_UNDERLINE));

	int effect = 0;
	if (m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_UNDERLINE))
		effect = CFE_UNDERLINE;

	SetEffect(CFM_UNDERLINE, effect);

}

void CRulerRichEditCtrl::DoLeftAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoLeftAlign
	Description :	Externally accessible member to set the
					selected text to left aligned.
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to left-align the selected text

   ============================================================*/
{

	if (!m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_LEFTALIGN))
		SetAlignment(PFA_LEFT);

}

void CRulerRichEditCtrl::DoCenterAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoCenterAlign
	Description :	Externally accessible member to set the
					selected text to center aligned
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to center-align the selected text

   ============================================================*/
{

	if (!m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_CENTERALIGN))
		SetAlignment(PFA_CENTER);

}

void CRulerRichEditCtrl::DoRightAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoRightAlign
	Description :	Externally accessible member to set the
					selected text to right aligned
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to right-align the selected text

   ============================================================*/
{

	if (!m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_RIGHTALIGN))
		SetAlignment(PFA_RIGHT);

}

void CRulerRichEditCtrl::DoIndent()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoIndent
	Description :	Externally accessible member to indent the
					selected text to the next tab position
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to indent the selected text

   ============================================================*/
{

	// Get current indent
	ParaFormat	para(PFM_STARTINDENT | PFM_TABSTOPS);
	m_rtf.GetParaFormat(para);
	int newindent = para.dxStartIndent;

	// Find next larger tab
	for (int t = MAX_TAB_STOPS - 1; t >= 0; t--)
	{

		if (para.rgxTabs[t] > para.dxStartIndent)
			newindent = para.rgxTabs[t];

	}

	if (newindent != para.dxStartIndent)
	{

		// Set indent to this value
		para.dwMask = PFM_STARTINDENT | PFM_OFFSET;
		para.dxStartIndent = newindent;
		para.dxOffset = 0; //newindent;

		m_rtf.SetParaFormat(para);

	}

	m_rtf.SetFocus();

}

void CRulerRichEditCtrl::DoOutdent()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoOutdent
	Description :	Externally accessible member to outdent the
					selected text to the previous tab position
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to outdent the selected text

   ============================================================*/
{

	// Get the current indent, if any
	ParaFormat	para(PFM_STARTINDENT | PFM_TABSTOPS);
	m_rtf.GetParaFormat(para);
	int newindent = 0;

	// Find closest smaller tab
	for (int t = 0; t < MAX_TAB_STOPS; t++)
		if (para.rgxTabs[t] < para.dxStartIndent)
			newindent = para.rgxTabs[t];

	// Set indent to this value or 0 if none
	para.dwMask = PFM_STARTINDENT | PFM_OFFSET;
	para.dxStartIndent = newindent;
	para.dxOffset = 0; //newindent;

	m_rtf.SetParaFormat(para);
	m_rtf.SetFocus();


}

void CRulerRichEditCtrl::DoBullet()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoBullet
	Description :	Externally accessible member to set the
					selected text to bulleted
	Access :		Public

	Return :		void
	Parameters :	none

	Usage :			Call to set the selected text to bulleted.

   ============================================================*/
{

	m_toolbar.GetToolBarCtrl().CheckButton(BUTTON_BULLET, !m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_BULLET));

	ParaFormat	para(PFM_NUMBERING);
	if (m_toolbar.GetToolBarCtrl().IsButtonChecked(BUTTON_BULLET))
		para.wNumbering = PFN_BULLET;
	else
		para.wNumbering = 0;

	m_rtf.SetParaFormat(para);
	m_rtf.SetFocus();

}

void CRulerRichEditCtrl::ShowToolbar(BOOL show)
/* ============================================================
	Function :		CRulerRichEditCtrl::ShowToolbar
	Description :	Shows or hides the toolbar
	Access :		Public

	Return :		void
	Parameters :	BOOL show	-	"TRUE" to show

	Usage :			Call to show or hide the toolbar subcontrol

   ============================================================*/
{

	if (m_hWnd && show != m_showToolbar)
	{
		if (show) {
			if (!m_toolbar.m_hWnd) {
				if (!CreateToolbar()) {
					ASSERT(0);
					return;
				}
			}
			m_toolbar.GetToolBarCtrl().ShowWindow(SW_SHOW);
		}
		else {
			ASSERT(m_toolbar.m_hWnd);
			m_toolbar.GetToolBarCtrl().ShowWindow(SW_HIDE);
		}

		CRect rect;
		GetClientRect(rect);
		m_showToolbar = show;
		LayoutControls(rect.Width(), rect.Height());
	}

}

void CRulerRichEditCtrl::ShowRuler(BOOL show)
/* ============================================================
	Function :		CRulerRichEditCtrl::ShowRuler
	Description :	Shows or hides the ruler
	Access :		Public

	Return :		void
	Parameters :	BOOL show	-	"TRUE" to show

	Usage :			Call to show or hide the ruler subcontrol

   ============================================================*/
{

	if (m_hWnd && show != m_showRuler)
	{
		if (show) {
			if (!m_ruler.m_hWnd) {
				if (!CreateRuler()) {
					ASSERT(0);
					return;
				}
			}
			m_ruler.ShowWindow(SW_SHOW);
		}
		else {
			ASSERT(m_ruler.m_hWnd);
			m_ruler.ShowWindow(SW_HIDE);
		}

		CRect rect;
		GetClientRect(rect);
		m_showRuler = show;
		LayoutControls(rect.Width(), rect.Height());
	}
}

void CRulerRichEditCtrl::LayoutControls(int width, int height)
/* ============================================================
	Function :		CRulerRichEditCtrl::LayoutControls
	Description :	Lays out the sub-controls depending on
					visibility.
	Access :		Private

	Return :		void
	Parameters :	int width	-	Width of control
					int height	-	Height of control

	Usage :			Called internally to lay out the controls

   ============================================================*/
{

	int toolbarHeight = 0;
	if (m_showToolbar)
		toolbarHeight = TOOLBAR_HEIGHT;
	int rulerHeight = 0;
	if (m_showRuler)
		rulerHeight = RULER_HEIGHT;

	if (m_toolbar.m_hWnd) m_toolbar.GetToolBarCtrl().MoveWindow(0, 0, width, toolbarHeight);
	if (m_ruler.m_hWnd) m_ruler.MoveWindow(0, toolbarHeight, width, rulerHeight);

	CRect rect(0, toolbarHeight + rulerHeight, width, height);
	m_rtf.MoveWindow(rect);

}

BOOL CRulerRichEditCtrl::IsToolbarVisible() const
/* ============================================================
	Function :		CRulerRichEditCtrl::IsToolbarVisible
	Description :	Returns if the toolbar is visible or not
	Access :		Public

	Return :		BOOL	-	"TRUE" if visible
	Parameters :	none

	Usage :			Call to get the visibility of the toolbar

   ============================================================*/
{

	return m_showToolbar;

}

BOOL CRulerRichEditCtrl::IsRulerVisible() const
/* ============================================================
	Function :		CRulerRichEditCtrl::IsRulerVisible
	Description :	Returns if the ruler is visible or not
	Access :		Public

	Return :		BOOL	-	"TRUE" if visible
	Parameters :	none

	Usage :			Call to get the visibility of the ruler

   ============================================================*/
{

	return m_showRuler;

}

void CRulerRichEditCtrl::SetReadOnly(BOOL readOnly)
/* ============================================================
	Function :		CRulerRichEditCtrl::SetReadOnly
	Description :	Sets the control to read only or not.
	Access :		Public

	Return :		void
	Parameters :	BOOL readOnly	-	New read only state

	Usage :			Call to set the read only state of the
					control

   ============================================================*/
{

	if (m_rtf.m_hWnd)
		m_rtf.SetReadOnly(readOnly);

	m_readOnly = readOnly;

}

void CRulerRichEditCtrl::OnContextMenu(CWnd* pWnd, CPoint pt)
{
	CRect rect;
	m_rtf.GetWindowRect(&rect);
	if (!rect.PtInRect(pt)) return;
	CMenu menu;
	if (!menu.LoadMenu(m_idContext)) return;
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	if (m_idContext != IDR_RTF_CONTEXT) {
		if (m_readOnly)
			pPopup->EnableMenuItem(ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

		if (m_readOnly || !m_rtf.CanPaste())
			pPopup->EnableMenuItem(ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

		if (!m_rtf.CanUndo())
			pPopup->EnableMenuItem(ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		if (!m_rtf.CanRedo())
			pPopup->EnableMenuItem(ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

		if (m_idContext == IDR_FILL_CONTEXT) {
			//will never br readonly at this point --
			if (((CTableFillDlg *)(m_pParentDlg))->m_bMemo) {
				pPopup->CheckMenuItem(ID_FORMAT_PLAIN, (!m_bFormatRTF) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
				pPopup->CheckMenuItem(ID_FORMAT_RTF, m_bFormatRTF ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
			}
			else {
				pPopup->DeleteMenu(ID_FORMAT_PLAIN, MF_BYCOMMAND);
				pPopup->DeleteMenu(ID_FORMAT_RTF, MF_BYCOMMAND);
				pPopup->DeleteMenu(pPopup->GetMenuItemCount() - 1, MF_BYPOSITION);
			}
		}
	}
	else {
		if (m_readOnly || !m_rtf.CanPaste())
			pPopup->EnableMenuItem(ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

		if (m_pFindDlg) {
			pPopup->EnableMenuItem(ID_EDIT_FIND, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			pPopup->EnableMenuItem(ID_EDIT_REPLACE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}

		if (!CMainFrame::m_pComparePrg || CDbtEditDlg::m_csLastSaved.IsEmpty())
			pPopup->EnableMenuItem(ID__COMPAREWITHFILE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

		if (m_readOnly) {
			pPopup->EnableMenuItem(ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			pPopup->DeleteMenu(ID_EDIT_REPLACE, MF_BYCOMMAND);

			pPopup->DeleteMenu(ID_SHOWRULER, MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_SHOWTOOLBAR, MF_BYCOMMAND);
			VERIFY(pPopup->DeleteMenu(pPopup->GetMenuItemCount() - 1, MF_BYPOSITION)); //text format
			VERIFY(pPopup->DeleteMenu(pPopup->GetMenuItemCount() - 1, MF_BYPOSITION)); //separator...
			VERIFY(pPopup->DeleteMenu(pPopup->GetMenuItemCount() - 1, MF_BYPOSITION)); //insert file link...
			VERIFY(pPopup->DeleteMenu(pPopup->GetMenuItemCount() - 1, MF_BYPOSITION)); //Replace with file...
			pPopup->DeleteMenu(0, MF_BYPOSITION); //undo
			pPopup->DeleteMenu(0, MF_BYPOSITION); //redo
			pPopup->DeleteMenu(0, MF_BYPOSITION); //separator
		}
		else {
			if (!m_rtf.CanUndo())
				pPopup->EnableMenuItem(ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			if (!m_rtf.CanRedo())
				pPopup->EnableMenuItem(ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			pPopup->CheckMenuItem(ID_FORMAT_PLAIN, (!m_bFormatRTF) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
			pPopup->CheckMenuItem(ID_FORMAT_RTF, m_bFormatRTF ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
			if (m_bFormatRTF) {
				pPopup->CheckMenuItem(ID_SHOWTOOLBAR, m_showToolbar ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
				pPopup->CheckMenuItem(ID_SHOWRULER, m_showRuler ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
			}
			else {
				pPopup->DeleteMenu(ID_SHOWTOOLBAR, MF_BYCOMMAND);
				pPopup->DeleteMenu(ID_SHOWRULER, MF_BYCOMMAND);
				//pPopup->DeleteMenu(pPopup->GetMenuItemCount()-1,MF_BYPOSITION);
			}
		}
	}
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
}

void CRulerRichEditCtrl::OnEditCut()
{
	m_rtf.Cut();
}

void CRulerRichEditCtrl::OnEditCopy()
{
	m_rtf.Copy();
}

void CRulerRichEditCtrl::OnEditPaste()
{
	ASSERT(CanPaste());

#ifdef _DEBUG
	VERIFY(::OpenClipboard(m_hWnd));
	char name[80];
	UINT len, format = 0;
	while (format = EnumClipboardFormats(format)) {
		if (format == CF_TEXT) {
			strcpy(name, "Text");
		}
		else if (format >= 0xC000 && format <= 0xFFFF) {
			len = ::GetClipboardFormatName(format, name, 79);
		}
		else {
			//switch(format) {
			strcpy(name, "Other");
		}
		len = 0;
	}
	VERIFY(::CloseClipboard());
#endif

	// if(!m_bFormatRTF && !GetTextLength() && ::IsClipboardFormatAvailable(uCF_RTF)) {
	if (!m_bFormatRTF && m_idContext != IDR_IMG_CONTEXT) {
		if ((m_idContext != IDR_FILL_CONTEXT || ((CTableFillDlg *)m_pParentDlg)->m_bMemo) && ::IsClipboardFormatAvailable(uCF_RTF)) {
			UINT id = AfxMessageBox(IDS_RTF_PASTE, MB_YESNOCANCEL);
			if (id == IDCANCEL) return;
			if (id == IDYES) {
				if (GetTextLength()) {
					CHARRANGE cr;
					m_rtf.GetSel(cr);
					OnFormatRTF();
					m_rtf.SetSel(cr);
				}
				else
					OnFormatRTF();
			}
		}
	}
	m_rtf.Paste();
}

void CRulerRichEditCtrl::OnEditSelectAll()
{
	m_rtf.SetSel(0, -1);
}

void CRulerRichEditCtrl::OnEditUndo()
{
	ASSERT(m_rtf.CanUndo());
	m_rtf.Undo();
}

void CRulerRichEditCtrl::OnEditRedo()
{
	ASSERT(m_rtf.CanRedo());
	m_rtf.Redo();
}

void CRulerRichEditCtrl::OnShowRuler()
{
	ASSERT(!m_readOnly);
	ShowRuler(m_showRuler == FALSE);
}

/*
void CRulerRichEditCtrl::OnShowToolbar()
{
   ASSERT(!m_readOnly);
   ShowToolbar(m_showToolbar==FALSE);
}
*/

// obtained basic code from CView::OnFilePrint () in the VIEWPRNT.CPP file
// and CRichEditView::OnPrint(CDC* pDC, CPrintInfo* pInfo) in the
// VIEWRICH.CPP file
void CRulerRichEditCtrl::OnFilePrint()
{
	BOOL bError = FALSE;
	DOCINFO docInfo;
	FORMATRANGE fr;
	LONG nextLoc;
	UINT nEndPage;
	UINT nStartPage;

	CDC printDC;
	CPrintInfo printInfo;
	CSize size;

	ASSERT(printInfo.m_pPD);

	// set flags
	printInfo.m_pPD->m_pd.Flags |= PD_ALLPAGES | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_NOSELECTION;

	printInfo.m_nNumPreviewPages = AfxGetApp()->m_nNumPreviewPages;
	VERIFY(printInfo.m_strPageDesc.LoadString(AFX_IDS_PREVIEWPAGEDESC));

	// get page numbers for setup dialog
	printInfo.m_pPD->m_pd.nFromPage = (WORD)printInfo.GetMinPage();
	printInfo.m_pPD->m_pd.nToPage = (WORD)printInfo.GetMaxPage();

	// if user hit cancel, don't print
	if (AfxGetApp()->DoPrintDialog(printInfo.m_pPD) != IDOK)	return;

	// if we failed to create a printing device context, don't print
	if (printInfo.m_pPD->m_pd.hDC == NULL)	return;

	// set up document info and start the document printing process
	memset(&docInfo, 0, sizeof(DOCINFO));
	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.lpszDocName = "Rich Edit Control"; // set job name
	docInfo.lpszOutput = NULL; // don't save to file

	// setup the printing DC
	printDC.Attach(printInfo.m_pPD->m_pd.hDC); // attach printer dc
	printDC.m_bPrinting = TRUE;

	// start document printing process
	if (printDC.StartDoc(&docInfo) == SP_ERROR) {
		// cleanup and show error message
		printDC.Detach(); // will be cleaned up by CPrintInfo destructor
		AfxMessageBox(AFX_IDP_FAILED_TO_START_PRINT);
		return;
	}

	// Guarantee values are in the valid range
	nEndPage = printInfo.GetToPage();
	nStartPage = printInfo.GetFromPage();

	if (nEndPage < printInfo.GetMinPage())
		nEndPage = printInfo.GetMinPage();
	if (nEndPage > printInfo.GetMaxPage())
		nEndPage = printInfo.GetMaxPage();

	if (nStartPage < printInfo.GetMinPage())
		nStartPage = printInfo.GetMinPage();
	if (nStartPage > printInfo.GetMaxPage())
		nStartPage = printInfo.GetMaxPage();

	int nStep = (nEndPage >= nStartPage) ? 1 : -1;
	nEndPage = (nEndPage == 0xffff) ? 0xffff : nEndPage + nStep;

	// set map mode to MM_TEXT
	printDC.SetMapMode(MM_TEXT);

	// offset by printing offset
	printDC.SetViewportOrg(-printDC.GetDeviceCaps(PHYSICALOFFSETX), -printDC.GetDeviceCaps(PHYSICALOFFSETY));

	// adjust DC because richedit doesn't do things like MFC
	if ((printDC.GetDeviceCaps(TECHNOLOGY) != DT_METAFILE) && (printDC.m_hAttribDC != NULL)) {
		ScaleWindowExtEx(printDC.GetSafeHdc(),
			printDC.GetDeviceCaps(LOGPIXELSX),
			printDC.GetDeviceCaps(LOGPIXELSX),
			printDC.GetDeviceCaps(LOGPIXELSY),
			printDC.GetDeviceCaps(LOGPIXELSY), NULL);

	}

	// obtain paper size in twips
	size.cx = MulDiv(printDC.GetDeviceCaps(PHYSICALWIDTH), 1440, printDC.GetDeviceCaps(LOGPIXELSX));
	size.cy = MulDiv(printDC.GetDeviceCaps(PHYSICALHEIGHT), 1440, printDC.GetDeviceCaps(LOGPIXELSY));

	// set paper rectangle
	fr.rcPage = CRect(CPoint(0, 0), size);

	// set page rectangle to have .5 in margins (720 twips)
	CRect rect = fr.rcPage;
	rect.DeflateRect(720, 720);
	fr.rc = rect;

	// set device contexts to draw to
	fr.hdcTarget = printDC.m_hAttribDC;
	fr.hdc = printDC.GetSafeHdc();

	// set char range
	fr.chrg.cpMin = 0;
	/*
		I have discovered that the print method in this article doesn't work with Rich Edit Controls 2.0.
		The reason being that GetTextLength() doesn't return the number of characters as it should,
		instead it returns the number of bytes that the control contains! This causes FormatRange to
		return an incorrect value for the final iteration of the for-loop that determines the number of
		pages in the final print out.

		The solution is to use GetTextLengthEx() instead. Using the flag GetTextLengthEx(GTL_PRECISE)
		worked for me.
	*/
	//m_rtf.GetWindowTextLength() failed with too large of a count (DMcK)

	fr.chrg.cpMax = m_rtf.GetTextLengthEx(GTL_PRECISE);

	// loop thru as many pages as necessary for the control
	long lastLoc = 0;
	do {
		printDC.StartPage();
		nextLoc = m_rtf.FormatRange(&fr, TRUE);
		printDC.EndPage();
		if (lastLoc >= nextLoc) break;
		fr.chrg.cpMin = lastLoc = nextLoc;
	} while ((nextLoc > 0) && (nextLoc < fr.chrg.cpMax));

	// flush buffer
	m_rtf.FormatRange(NULL, FALSE);

	// cleanup document printing process
	if (!bError) printDC.EndDoc();
	else printDC.AbortDoc();

	printDC.Detach(); // will be cleaned up by CPrintInfo destructor
}

void CRulerRichEditCtrl::SetText(LPCSTR pText, BOOL bFormatRTF)
{
	//Initial data insertion --
	ASSERT(uCF_RTF);

	m_rtf.SetWindowText("");
	if (bFormatRTF) {
		//rtf codes are present --
		ASSERT(!*pText || *pText == '{');
		VERIFY(!m_rtf.SetTextMode(TM_RICHTEXT));
		//m_rtf.SetTextMode(TM_RICHTEXT);
		SetRTF(CString(pText));
		m_bFormatRTF = TRUE;
	}
	else {
		m_bFormatRTF = m_showToolbar;
		if (!m_bFormatRTF) {
			//m_rtf.SetWindowText("");
			VERIFY(!m_rtf.SetTextMode(TM_PLAINTEXT));
		}
		if (*pText) m_rtf.SetWindowText(pText);
	}

	if (!m_readOnly) {
		long len = GetTextLength() + 32 * 1024;
		if (m_rtf.GetLimitText() < len)
			m_rtf.LimitText(len); //for now
	}

	UpdateToolbarButtons();
	m_rtf.SetModify(FALSE);
	//Enable/disable insert links button, etc. --
	GetParent()->SendMessage(urm_DATACHANGED, (WPARAM)m_bFormatRTF);
}

void CRulerRichEditCtrl::GetText(CString &s)
{
	if (m_bFormatRTF) {
		GetRTF(&s);
	}
	else m_rtf.GetWindowText(s);
}

void CRulerRichEditCtrl::OnFormatRTF()
{
	if (m_bFormatRTF && GetTextLength()) {
		if (IDOK != AfxMessageBox(IDS_RTF_CONVERT2PLAIN, MB_OKCANCEL)) return;
	}

	CString s;
	m_rtf.GetWindowText(s);
	m_rtf.SetWindowText(""); //really necessary, apparently!
	if (m_bFormatRTF) {
		VERIFY(!m_rtf.SetTextMode(TM_PLAINTEXT));
		if (!s.IsEmpty())
			m_rtf.SetWindowText(s);
		ShowRuler(FALSE);
		ShowToolbar(FALSE);
		m_bFormatRTF = FALSE;
		m_rtf.SetModify();
	}
	else {
		VERIFY(!m_rtf.SetTextMode(TM_RICHTEXT));
		if (!s.IsEmpty())
			m_rtf.SetWindowText(s);
		ShowToolbar(TRUE);
		m_bFormatRTF = TRUE;
	}
	GetParent()->SendMessage(urm_DATACHANGED, (WPARAM)m_bFormatRTF);
}

LRESULT CRulerRichEditCtrl::OnDataChanged(WPARAM, LPARAM)
{
	if (m_showRuler) {
		CRect rect;
		m_rtf.GetClientRect(rect);
		rect.top = m_showToolbar ? TOOLBAR_HEIGHT : 0;
		rect.bottom = rect.top + RULER_HEIGHT;
		RedrawWindow(rect);
	}
	return TRUE; //rtn not used
}

void CRulerRichEditCtrl::OnEditFind()
{
	ASSERT(!m_pFindDlg);
	CRulerFindDlg *pDlg = new CRulerFindDlg();
	CHARRANGE cr;
	m_rtf.GetSel(cr);
	int lenSel = cr.cpMax - cr.cpMin;
	if (lenSel > 0 && lenSel < 80) m_csLastFind = m_rtf.GetSelText();
	if (pDlg->Create(TRUE, (LPCSTR)m_csLastFind, NULL, FR_DOWN, this)) {
		m_pFindDlg = pDlg;
		VERIFY(pDlg->m_ce.SubclassDlgItem(1152, pDlg)); //subclass "Find" edit control
	}
	ASSERT(m_pFindDlg);
}

void CRulerRichEditCtrl::OnEditReplace()
{
	ASSERT(!m_pFindDlg);
	CRulerFindDlg *pDlg = new CRulerFindDlg();
	CHARRANGE cr;
	m_rtf.GetSel(cr);
	int lenSel = cr.cpMax - cr.cpMin;
	if (lenSel > 0 && lenSel < 80) {
		m_csLastFind = m_rtf.GetSelText();
		m_csLastRepl.Empty();
	}
	if (pDlg->Create(FALSE, m_csLastFind, m_csLastRepl, FR_DOWN, this)) {
		m_pFindDlg = pDlg;
		VERIFY(pDlg->m_ce.SubclassDlgItem(1152, pDlg)); //subclass "Find" edit control
		VERIFY(pDlg->m_cr.SubclassDlgItem(1153, pDlg)); //subclass "Find" edit control
	}
	ASSERT(m_pFindDlg);

}

LRESULT CRulerRichEditCtrl::OnFindDlgMessage(WPARAM wParam, LPARAM lParam)
{
	ASSERT(m_pFindDlg && m_pFindDlg == (CRulerFindDlg *)CFindReplaceDialog::GetNotifier(lParam));

	// If the FR_DIALOGTERM flag is set,
	// invalidate the handle identifying the dialog box.
	if (m_pFindDlg->IsTerminating())
	{
		//m_pFindDlg->DestroyWindow(); //??
		m_pFindDlg = NULL;
		return 0;
	}

	DWORD dwflags = m_pFindDlg->m_fr.Flags;
	//unnecessary --
	//if(m_pFindDlg->FindNext()) dwflags|=FR_FINDNEXT;
	//if(m_pFindDlg->ReplaceCurrent()) dwflags|=FR_REPLACE;
	//if(m_pFindDlg->ReplaceAll()) dwflags|=FR_REPLACEALL;
	//if(m_pFindDlg->MatchCase()) dwflags|=FR_MATCHCASE;
	//if(m_pFindDlg->MatchWholeWord()) dwflags|=FR_WHOLEWORD;
	//if(m_pFindDlg->SearchDown()) dwflags|=FR_DOWN;

	if (dwflags&(FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL))
	{
		FINDTEXTEX ft;
		CHARRANGE crSel;
		m_rtf.GetSel(crSel);

		bool bNextSearch = false;
		CString csFind = m_pFindDlg->GetFindString();
		ft.lpstrText = (LPCSTR)csFind;

		CString csRepl;

		if (crSel.cpMax > crSel.cpMin) {
			CString csSel = m_rtf.GetSelText();
			if ((dwflags&FR_MATCHCASE) && !csFind.Compare(csSel) ||
				!(dwflags&FR_MATCHCASE) && !csFind.CompareNoCase(csSel)) {
				bNextSearch = true; //target (csFind) matches current selection
				if (dwflags&(FR_REPLACE | FR_REPLACEALL)) {
					csRepl = m_pFindDlg->GetReplaceString();
					m_rtf.ReplaceSel(csRepl, TRUE); //allow undo
					m_rtf.GetSel(crSel);
				}
			}
		}

		if (dwflags&FR_DOWN) {
			ft.chrg.cpMin = bNextSearch ? (crSel.cpMin + 1) : crSel.cpMin;
			ft.chrg.cpMax = -1;

			if (dwflags&FR_REPLACEALL) {
				UINT nReplaced = 1;
				if (!bNextSearch) {
					nReplaced--;
					csRepl = m_pFindDlg->GetReplaceString();
				}

				int iStart = ft.chrg.cpMin;
				int idif = csRepl.GetLength() - csFind.GetLength();

				while (true) {
					if (m_rtf.FindText((dwflags&(FR_MATCHCASE | FR_WHOLEWORD)) | FR_DOWN, &ft) != -1) {
						m_rtf.SetSel(ft.chrgText);
						m_rtf.ReplaceSel(csRepl, TRUE); //allow undo
						nReplaced++;
						m_rtf.GetSel(crSel);
						ft.chrg.cpMin = crSel.cpMin;
						//ft.chrg.cpMax=(iStart>=0)?-1:-iStart;
						if (iStart >= 0) ft.chrg.cpMax = -1;
						else {
							iStart -= idif;
							if (iStart <= 0) break;
							ft.chrg.cpMax = -iStart;
						}
					}
					else {
						if (iStart <= 0) break;
						ft.chrg.cpMin = 0;
						ft.chrg.cpMax = iStart;
						iStart = -iStart;
					}
				}
				CMsgBox("%u replacements.", nReplaced);
				m_rtf.SetFocus();
				return 0;
			}
		}
		else {
			ASSERT(!(dwflags&(FR_REPLACE | FR_REPLACEALL)));
			ft.chrg.cpMax = 0;
			ft.chrg.cpMin = crSel.cpMax ? (crSel.cpMax - 1) : GetTextLength();
		}

	_retry:
		if (m_rtf.FindText(dwflags, &ft) != -1) {
			m_rtf.SetSel(ft.chrgText);
		}
		else {
			if (bNextSearch) {
				MessageBeep(MB_OK);
				//The selection matches the string searched for, but there are no
				//additional matches in the current search direction.
				//Wrap around automatically and find at least this selection--
				if (dwflags&FR_DOWN) {
					ft.chrg.cpMin = 0;
					ft.chrg.cpMax = crSel.cpMax;
				}
				else {
					ft.chrg.cpMin = GetTextLength();
					ft.chrg.cpMax = crSel.cpMin;
				}
				bNextSearch = false; //shouldn't be necessary
				goto _retry;
			}
			AfxMessageBox("The specified text was not found.", MB_ICONQUESTION);
		}
		m_rtf.SetFocus();
	}

	return 0;
}
void CRulerRichEditCtrl::OnLinkToFile()
{
	((CDbtEditDlg *)GetParent())->OnLinkToFile();
}
void CRulerRichEditCtrl::OnSaveToFile()
{
	((CDbtEditDlg *)GetParent())->OnSaveToFile();
}

void CRulerRichEditCtrl::OnCompareWithFile()
{
	((CDbtEditDlg *)GetParent())->OnCompareWithFile();
}

void CRulerRichEditCtrl::OnReplaceWithFile()
{
	((CDbtEditDlg *)GetParent())->OnReplaceWithFile();
}

