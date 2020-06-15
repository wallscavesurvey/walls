// ColorPopup.cpp : implementation file
//
// Written by Chris Maunder (chrismaunder@codeguru.com)
// Extended by Alexander Bischofberger (bischofb@informatik.tu-muenchen.de)
// Copyright (c) 1998.
//
// Updated 30 May 1998 to allow any number of colours, and to
//                     make the appearance closer to Office 97. 
//                     Also added "Default" text area.         (CJM)
//
//         13 June 1998 Fixed change of focus bug (CJM)
//         30 June 1998 Fixed bug caused by focus bug fix (D'oh!!)
//                      Solution suggested by Paul Wilkerson.
//
// ColorPopup is a helper class for the colour picker control
// CColorPicker. Check out the header file or the accompanying 
// HTML doc file for details.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Expect bugs.
// 
// Please use and enjoy. Please let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into this
// file. 

#include "stdafx.h"
#include <math.h>
#include <afxpriv.h>
#include "ColorButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* Not used -- see below
#ifndef SM_CMONITORS
	#define SM_XVIRTUALSCREEN       76
	#define SM_YVIRTUALSCREEN       77
	#define SM_CXVIRTUALSCREEN      78
	#define SM_CYVIRTUALSCREEN      79
	#define SM_CMONITORS            80
	#define SM_SAMEDISPLAYFORMAT    81
#endif
*/

#define MORECOLORS_BOX_VALUE -3
#define DEFAULT_BOX_VALUE -2
#define CUSTOM_BOX_VALUE  -2
#define CUSTOM_CLR_LIMIT  -1000
#define INVALID_COLOUR    -1

#define MAX_COLOURS      100


ColorTableEntry CColorPopup::m_crColors[] =
{
	{ RGB(0x00, 0x00, 0x00),    _T("Black")             },
	{ RGB(0xA5, 0x2A, 0x00),    _T("Brown")             },
	{ RGB(0x00, 0x40, 0x40),    _T("Dark Olive Green")  },
	{ RGB(0x00, 0x55, 0x00),    _T("Dark Green")        },
	{ RGB(0x00, 0x00, 0x5E),    _T("Dark Teal")         },
	{ RGB(0x00, 0x00, 0x8B),    _T("Dark blue")         },
	{ RGB(0x4B, 0x00, 0x82),    _T("Indigo")            },
	{ RGB(0x28, 0x28, 0x28),    _T("Dark gray")         },

	{ RGB(0x8B, 0x00, 0x00),    _T("Dark red")          },
	{ RGB(0xFF, 0x68, 0x20),    _T("Orange")            },
	{ RGB(0x8B, 0x8B, 0x00),    _T("Dark yellow")       },
	{ RGB(0x00, 0x93, 0x00),    _T("Green")             },
	{ RGB(0x38, 0x8E, 0x8E),    _T("Teal")              },
	{ RGB(0x00, 0x00, 0xFF),    _T("Blue")              },
	{ RGB(0x7B, 0x7B, 0xC0),    _T("Blue-gray")         },
	{ RGB(0x66, 0x66, 0x66),    _T("Gray - 40")         },

	{ RGB(0xFF, 0x00, 0x00),    _T("Red")               },
	{ RGB(0xFF, 0xAD, 0x5B),    _T("Light orange")      },
	{ RGB(0x32, 0xCD, 0x32),    _T("Lime")              },
	{ RGB(0x3C, 0xB3, 0x71),    _T("Sea green")         },
	{ RGB(0x00, 0xFF, 0xFF),    _T("Cyan")              },
	{ RGB(0x7D, 0x9E, 0xC0),    _T("Light blue")        },
	{ RGB(0x80, 0x00, 0x80),    _T("Violet")            },
	{ RGB(0x7F, 0x7F, 0x7F),    _T("Gray - 50")         },

	{ RGB(0xFF, 0xC0, 0xCB),    _T("Pink")              },
	{ RGB(0xFF, 0xD7, 0x00),    _T("Gold")              },
	{ RGB(0xFF, 0xFF, 0x00),    _T("Yellow")            },
	{ RGB(0x00, 0xFF, 0x00),    _T("Bright green")      },
	{ RGB(0x40, 0xE0, 0xD0),    _T("Turquoise")         },
	{ RGB(0xC0, 0xFF, 0xFF),    _T("Skyblue")           },
	{ RGB(0x48, 0x00, 0x48),    _T("Plum")              },
	{ RGB(0xC0, 0xC0, 0xC0),    _T("Light gray")        },

	{ RGB(0xFF, 0xE4, 0xE1),    _T("Rose")              },
	{ RGB(0xD2, 0xB4, 0x8C),    _T("Tan")               },
	{ RGB(0xFF, 0xFF, 0xE0),    _T("Light yellow")      },
	{ RGB(0x98, 0xFB, 0x98),    _T("Pale green ")       },
	{ RGB(0xAF, 0xEE, 0xEE),    _T("Pale turquoise")    },
	{ RGB(0x68, 0x83, 0x8B),    _T("Pale blue")         },
	{ RGB(0xE6, 0xE6, 0xFA),    _T("Lavender")          },
	{ RGB(0xFF, 0xFF, 0xFF),    _T("White")             }
};

int CColorPopup::m_nNumColors = sizeof(m_crColors) / sizeof(ColorTableEntry);

/////////////////////////////////////////////////////////////////////////////
// CColorPopup

CColorPopup::CColorPopup()
{
	Initialise();
}

CColorPopup::CColorPopup(CPoint p, COLORREF crColor,
	CColorCombo* pParentWnd, LPCTSTR *pText, COLORREF *pClrElems, int nTextElems)
{
	Initialise();

	m_crColor = m_crInitialColor = crColor;
	m_pParent = pParentWnd;
	m_nTextElems = nTextElems;
	m_pText = pText;
	m_pClrElems = pClrElems;

	Create(p, crColor, pParentWnd);
}

void CColorPopup::Initialise()
{
	m_pTextRect = NULL;
	ASSERT(m_nNumColors <= MAX_COLOURS);
	if (m_nNumColors > MAX_COLOURS)
		m_nNumColors = MAX_COLOURS;

	m_nNumColumns = 0;
	m_nNumRows = 0;
	m_nBoxSize = 18;
	m_nMargin = ::GetSystemMetrics(SM_CXEDGE);
	m_nCurrentSel = INVALID_COLOUR;
	m_nChosenColorSel = INVALID_COLOUR;
	m_pParent = NULL;
	m_crColor = m_crInitialColor = RGB(0, 0, 0);

	// Idiot check: Make sure the colour square is at least 5 x 5;
	if (m_nBoxSize - 2 * m_nMargin - 2 < 5) m_nBoxSize = 5 + 2 * m_nMargin + 2;

	// Create the font
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0));
	m_Font.CreateFontIndirect(&(ncm.lfMessageFont));

	// Create the palette
	struct {
		LOGPALETTE    LogPalette;
		PALETTEENTRY  PalEntry[MAX_COLOURS];
	} pal;

	LOGPALETTE* pLogPalette = (LOGPALETTE*)&pal;
	pLogPalette->palVersion = 0x300;
	pLogPalette->palNumEntries = (WORD)m_nNumColors;

	for (int i = 0; i < m_nNumColors; i++)
	{
		pLogPalette->palPalEntry[i].peRed = GetRValue(m_crColors[i].crColor);
		pLogPalette->palPalEntry[i].peGreen = GetGValue(m_crColors[i].crColor);
		pLogPalette->palPalEntry[i].peBlue = GetBValue(m_crColors[i].crColor);
		pLogPalette->palPalEntry[i].peFlags = 0;
	}

	m_Palette.CreatePalette(pLogPalette);
}

CColorPopup::~CColorPopup()
{
	m_Font.DeleteObject();
	m_Palette.DeleteObject();
	delete[] m_pTextRect;
}

BOOL CColorPopup::Create(CPoint p, COLORREF crColor, CColorCombo* pParentWnd)
{
	ASSERT(pParentWnd && ::IsWindow(pParentWnd->GetSafeHwnd()));

	m_pParent = pParentWnd;
	m_crColor = m_crInitialColor = crColor;

	// Get the class name and create the window
	CString szClassName = AfxRegisterWndClass(CS_CLASSDC | CS_SAVEBITS | CS_HREDRAW | CS_VREDRAW,
		0,
		(HBRUSH)(COLOR_BTNFACE + 1),
		0);

	if (!CWnd::CreateEx(0, szClassName, _T(""), WS_VISIBLE | WS_POPUP,
		p.x, p.y, 100, 100, // size updated soon
		pParentWnd->GetSafeHwnd(), 0, NULL))
		return FALSE;

	m_pTextRect = new CRect[m_nTextElems];

	// Set the window size
	SetWindowSize();

	// Create the tooltips
	CreateToolTips();

	// Find which cell (if any) corresponds to the initial colour
	m_nChosenColorSel = GetIndexFromColor(crColor);
	//m_nCurrentSel=m_nChosenColorSel;

	// Capture all mouse events for the life of this window
	SetCapture();

	return TRUE;
}

BEGIN_MESSAGE_MAP(CColorPopup, CWnd)
	//{{AFX_MSG_MAP(CColorPopup)
	ON_WM_NCDESTROY()
	ON_WM_LBUTTONUP()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	ON_WM_KILLFOCUS()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorPopup message handlers

// For tooltips
BOOL CColorPopup::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);

	// Fix (Adrian Roman): Sometimes if the picker loses focus it is never destroyed
	if (GetCapture()->GetSafeHwnd() != m_hWnd)
		SetCapture();

	return CWnd::PreTranslateMessage(pMsg);
}

// If an arrow key is pressed, then move the selection
void CColorPopup::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int row = GetRow(m_nCurrentSel),
		col = GetColumn(m_nCurrentSel);

	if (row == -1) {
		ChangeSelection(m_nChosenColorSel, 2);
		goto _ret;
	}

	if (nChar == VK_DOWN)
	{
		if (row == DEFAULT_BOX_VALUE) {
			row = col = 0;
		}
		else if (row < DEFAULT_BOX_VALUE) {
			if (--row == CUSTOM_BOX_VALUE - m_nTextElems) {
				row = m_pText[0] ? DEFAULT_BOX_VALUE : 0;
				col = 0;
			}
		}
		else
		{
			row++;
			if (GetIndex(row, col) == INVALID_COLOUR)
			{
				if (m_pText[1]) row = MORECOLORS_BOX_VALUE;
				else if (m_pText[0]) row = DEFAULT_BOX_VALUE;
				else row = 0;
				col = 0;
			}
		}
	}

	else if (nChar == VK_UP)
	{
		if (row == DEFAULT_BOX_VALUE)
		{
			if (m_pText[1]) {
				row = CUSTOM_BOX_VALUE - m_nTextElems + 1;
				col = 1;
			}
			else
			{
				row = GetRow(m_nNumColors - 1);
				col = GetColumn(m_nNumColors - 1);
			}
		}
		else if (row < DEFAULT_BOX_VALUE) {
			if (++row > MORECOLORS_BOX_VALUE)
			{
				row = GetRow(m_nNumColors - 1);
				col = GetColumn(m_nNumColors - 1);
			}
			/* else col unchanged*/
		}
		else if (row > 0) row--;
		else /* row == 0 */
		{
			if (m_pText[0]) {
				row = DEFAULT_BOX_VALUE;
				col = 0;
			}
			else if (m_pText[1]) {
				row = CUSTOM_BOX_VALUE - m_nTextElems + 1;
				col = 1;
			}
			else
			{
				row = GetRow(m_nNumColors - 1);
				col = GetColumn(m_nNumColors - 1);
			}
		}
	}

	else if (nChar == VK_RIGHT)
	{
		if (row == DEFAULT_BOX_VALUE) row = col = 0;

		else if (row < DEFAULT_BOX_VALUE) {
			if (++col > 1) {
				col = 0;
				if (--row == CUSTOM_BOX_VALUE - m_nTextElems) row = m_pText[0] ? DEFAULT_BOX_VALUE : 0;
			}
		}
		else if (col < m_nNumColumns - 1) col++;
		else {
			col = 0;
			if (++row >= m_nNumRows) {
				if (m_pText[1])	row = MORECOLORS_BOX_VALUE;
				else if (m_pText[0])	row = DEFAULT_BOX_VALUE;
				else row = 0;
			}
		}
	}

	else if (nChar == VK_LEFT)
	{
		if (row == DEFAULT_BOX_VALUE)
		{
			if (m_pText[1]) {
				row = CUSTOM_BOX_VALUE - m_nTextElems + 1;
				col = 1;
			}
			else {
				row = GetRow(m_nNumColors - 1);
				col = GetColumn(m_nNumColors - 1);
			}
		}
		else if (row < DEFAULT_BOX_VALUE) {
			if (--col < 0) {
				col = 1;
				if (++row > MORECOLORS_BOX_VALUE) {
					row = GetRow(m_nNumColors - 1);
					col = GetColumn(m_nNumColors - 1);
				}
			}
		}
		else if (col > 0) col--;
		else { /* col == 0 */
			if (row > 0) {
				row--;
				col = m_nNumColumns - 1;
			}
			else {
				if (m_pText[0]) {
					row = DEFAULT_BOX_VALUE;
					col = 0;
				}
				else if (m_pText[1]) {
					row = CUSTOM_BOX_VALUE - m_nTextElems + 1;
					col = 1;
				}
				else {
					row = GetRow(m_nNumColors - 1);
					col = GetColumn(m_nNumColors - 1);
				}
			}
		}
	}
	else {
		if (nChar == VK_ESCAPE)
		{
			m_crColor = m_crInitialColor;
			EndSelection(CPN_SELENDCANCEL);
			return;
		}

		if (nChar == VK_RETURN || nChar == VK_SPACE)
		{
			EndSelection(CPN_SELENDOK);
			return;
		}

		goto _ret;
	}

	ChangeSelection(GetIndex(row, col), 2);
_ret:
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

// auto-deletion
void CColorPopup::OnNcDestroy()
{
	CWnd::OnNcDestroy();
	delete this;
}

void CColorPopup::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// Draw color cells
	for (int i = 0; i < m_nNumColors; i++) DrawCell(&dc, i);

	// Draw horizontal line
	CRect rect(m_pTextRect[1]);

	dc.FillSolidRect(rect.left + 2 * m_nMargin, rect.top,
		rect.Width() - 4 * m_nMargin, 1, ::GetSysColor(COLOR_3DSHADOW));
	dc.FillSolidRect(rect.left + 2 * m_nMargin, rect.top + 1,
		rect.Width() - 4 * m_nMargin, 1, ::GetSysColor(COLOR_3DHILIGHT));

	// Draw custom text
	for (int i = 0; i < m_nTextElems; i++) {
		if (m_pText[i]) {
			DrawTextCell(&dc, i, FALSE);
			if (i) DrawCell(&dc, CUSTOM_CLR_LIMIT + CUSTOM_BOX_VALUE - i);
		}
	}

	// Draw raised window edge (ex-window style WS_EX_WINDOWEDGE is sposed to do this,
	// but for some reason isn't
	GetClientRect(rect);
	dc.DrawEdge(rect, EDGE_RAISED, BF_RECT);
}

void CColorPopup::OnMouseMove(UINT nFlags, CPoint point)
{
	int nNewSelection = INVALID_COLOUR;

	// Translate points to be relative raised window edge
	point.x -= m_nMargin;
	point.y -= m_nMargin;

	for (int i = 0; i < m_nTextElems; i++) {
		if (m_pText[i] && m_pTextRect[i].PtInRect(point)) {
			if (!i || point.x <= m_pTextRect[i].left + m_nTextWidth) nNewSelection = CUSTOM_BOX_VALUE - i;
			else nNewSelection = CUSTOM_CLR_LIMIT + CUSTOM_BOX_VALUE - i;
			break;
		}
	}

	if (nNewSelection == INVALID_COLOUR) {
		// Take into account text box
		if (m_pText[0]) point.y -= m_pTextRect[0].Height();

		// Get the row and column
		if (point.y >= 0 && point.x >= 0) nNewSelection = GetIndex(point.y / m_nBoxSize, point.x / m_nBoxSize);

		// In range? If not, default and exit
		if (nNewSelection < 0 || nNewSelection >= m_nNumColors)
		{
			CWnd::OnMouseMove(nFlags, point);
			return;
		}
	}

	// OK - we have the row and column of the current selection (may be CUSTOM_BOX_VALUE)
	// Has the row/col selection changed? If yes, then redraw old and new cells.
	if (nNewSelection != m_nCurrentSel) ChangeSelection(nNewSelection, (nFlags&MK_LBUTTON) != 0);

	CWnd::OnMouseMove(nFlags, point);
}

// End selection on LButtonUp
void CColorPopup::OnLButtonUp(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonUp(nFlags, point);

	DWORD pos = GetMessagePos();
	point = CPoint(LOWORD(pos), HIWORD(pos));

	if (m_WindowRect.PtInRect(point))
		EndSelection(CPN_SELENDOK);
	else
		EndSelection(CPN_SELENDCANCEL);
}

void CColorPopup::OnLButtonDown(UINT nFlags, CPoint point)
{
	point.x -= m_nMargin;
	point.y -= m_nMargin;

	for (int i = 0; i < m_nTextElems; i++) {
		if (m_pText[i] && m_pTextRect[i].PtInRect(point)) {
			if (!i || point.x <= m_pTextRect[i].left + m_nTextWidth) {
				//ChangeSelection(CUSTOM_BOX_VALUE-i,TRUE);
				CClientDC dc(this);        // device context for drawing
				DrawTextCell(&dc, i, TRUE);
				break;
			}
		}
	}
	CWnd::OnLButtonDown(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// CColorPopup implementation

int CColorPopup::GetIndex(int row, int col) const
{
	if (row <= CUSTOM_BOX_VALUE && row > CUSTOM_BOX_VALUE - m_nTextElems) {
		ASSERT(m_pText[CUSTOM_BOX_VALUE - row]);
		if (col) row += CUSTOM_CLR_LIMIT;
		return row;
	}
	if (row < 0 || col < 0 || row >= m_nNumRows || col >= m_nNumColumns) return INVALID_COLOUR;
	if (row*m_nNumColumns + col >= m_nNumColors) return INVALID_COLOUR;
	return row * m_nNumColumns + col;
}

int CColorPopup::GetRow(int nIndex) const
{
	if (nIndex <= CUSTOM_BOX_VALUE) {
		if (nIndex < CUSTOM_CLR_LIMIT) nIndex -= CUSTOM_CLR_LIMIT;
		ASSERT(nIndex > CUSTOM_BOX_VALUE - m_nTextElems);
		if (m_pText[CUSTOM_BOX_VALUE - nIndex]) return nIndex;
	}
	else if (nIndex >= 0 && nIndex < m_nNumColors) return nIndex / m_nNumColumns;
	return INVALID_COLOUR;
}

int CColorPopup::GetColumn(int nIndex) const
{
	if (nIndex <= CUSTOM_BOX_VALUE) return (nIndex < CUSTOM_CLR_LIMIT) ? 1 : 0;
	if (nIndex >= 0 && nIndex < m_nNumColors) return nIndex % m_nNumColumns;
	return INVALID_COLOUR;
}

int CColorPopup::GetTableIndex(COLORREF crColor)
{
	int i;
	for (i = 0; i < m_nNumColors; i++) if (GetColor(i) == crColor) break;
	return i;
}

char * CColorPopup::GetColorName(char *buf, COLORREF crColor)
{
	int i;

	if (IS_CUSTOM(crColor)) {
		i = (crColor >> CUSTOM_COLOR_SHIFT) + 1;
		ASSERT(i > 1 && i < MAX_TEXT_ELEMS);
		strcpy(buf, dflt_pText[i]);
		ASSERT(strlen(buf) > 3);
		buf[strlen(buf) - 3] = 0;
	}
	else {
		i = GetTableIndex(crColor);
		if (i < m_nNumColors) strcpy(buf, GetColorName(i));
		else GetRGBStr(buf, crColor);
	}
	return buf;
}

int CColorPopup::GetIndexFromColor(COLORREF crColor)
{
	if (crColor == CLR_DEFAULT && m_pText[0])
	{
		return DEFAULT_BOX_VALUE;
	}

	if (IS_CUSTOM(crColor))
	{
		return MORECOLORS_BOX_VALUE + CUSTOM_CLR_LIMIT - (crColor >> CUSTOM_COLOR_SHIFT);
	}

	int i = GetTableIndex(crColor);
	if (i < m_nNumColors) return i;

	if (m_pText[1]) {
		return MORECOLORS_BOX_VALUE + CUSTOM_CLR_LIMIT;
	}

	ASSERT(FALSE);
	return INVALID_COLOUR;
}

// Gets the dimensions of the colour cell given by (row,col)
BOOL CColorPopup::GetCellRect(int nIndex, const LPRECT& rect)
{
	if (nIndex < 0 || nIndex >= m_nNumColors) {
		ASSERT(FALSE);
		return FALSE;
	}

	rect->left = GetColumn(nIndex) * m_nBoxSize + m_nMargin;
	rect->top = GetRow(nIndex) * m_nBoxSize + m_nMargin;

	// Move everything down if we are displaying a default text area
	if (m_pText[0]) rect->top += (m_nMargin + m_pTextRect[0].Height());

	rect->right = rect->left + m_nBoxSize;
	rect->bottom = rect->top + m_nBoxSize;

	return TRUE;
}

// Works out an appropriate size and position of this window
void CColorPopup::SetWindowSize()
{
	int i;
	CSize TextSize(0, 0);

	//Get the font and text size.
	CClientDC dc(this);
	CFont* pOldFont = (CFont*)dc.SelectObject(&m_Font);
	CSize size;

	for (i = 0; i < m_nTextElems; i++) {
		if (m_pText[i]) {
			size = dc.GetTextExtent(m_pText[i], strlen(m_pText[i]));
			if (size.cx > TextSize.cx) TextSize.cx = size.cx;
			if (size.cy > TextSize.cy) TextSize.cy = size.cy;
		}
	}

	dc.SelectObject(pOldFont);
	TextSize += CSize(2 * m_nMargin, 2 * m_nMargin);
	m_nTextWidth = TextSize.cx + 2 * m_nMargin;

	// Add even more space to draw the horizontal line
	TextSize.cy += 2 * m_nMargin + 2;

	// Get the number of columns and rows
	//m_nNumColumns = (int) sqrt((double)m_nNumColors);    // for a square window (yuk)
	m_nNumColumns = 8;
	m_nNumRows = m_nNumColors / m_nNumColumns;
	if (m_nNumColors % m_nNumColumns) m_nNumRows++;

	// Get the current window position, and set the new size
	CRect rect;
	GetWindowRect(rect);

	m_WindowRect.SetRect(rect.left, rect.top,
		rect.left + m_nNumColumns * m_nBoxSize + 2 * m_nMargin,
		rect.top + m_nNumRows * m_nBoxSize + 2 * m_nMargin);

	// if default text, then expand window if necessary, and set text width as
	// window width
	if (m_pText[0])
	{
		if (TextSize.cx > m_WindowRect.Width())
			m_WindowRect.right = m_WindowRect.left + TextSize.cx;
		TextSize.cx = m_WindowRect.Width() - 2 * m_nMargin;

		// Work out the text area
		m_pTextRect[0].SetRect(m_nMargin, m_nMargin,
			m_nMargin + TextSize.cx, 2 * m_nMargin + TextSize.cy);
		m_WindowRect.bottom += m_pTextRect[0].Height() + 2 * m_nMargin;
	}
	else m_pTextRect[0].SetRectEmpty();

	// if custom text, then expand window if necessary, and set text width as
	// window width

	if (TextSize.cx > m_WindowRect.Width())
		m_WindowRect.right = m_WindowRect.left + TextSize.cx;
	TextSize.cx = m_WindowRect.Width() - 2 * m_nMargin;

	int yCoord = m_WindowRect.Height();

	for (i = 1; i < m_nTextElems; i++) {
		if (m_pText[i])
		{
			m_pTextRect[i].SetRect(m_nMargin, yCoord,
				m_nMargin + TextSize.cx, yCoord + m_nMargin + TextSize.cy);
			yCoord += TextSize.cy;

		}
		else m_pTextRect[i].SetRectEmpty();
	}

	m_WindowRect.bottom = m_WindowRect.top + yCoord + 3 * m_nMargin;

	/*	Just avoid check of screen size -- not worth the complications given the
		possibility of multiple monitors!

		 // Need to check it'll fit on screen: Too far right?
		CSize ScreenSize;
		if(GetSystemMetrics(SM_CMONITORS)>1) {
			ScreenSize.cx=GetSystemMetrics(SM_CXVIRTUALSCREEN);
			ScreenSize.cy=GetSystemMetrics(SM_CYVIRTUALSCREEN); //works only for tallest screen!
		}
		else {
			ScreenSize.cx=GetSystemMetrics(SM_CXSCREEN);
			ScreenSize.cy=GetSystemMetrics(SM_CYSCREEN);
		}

		if (m_WindowRect.right > ScreenSize.cx)
			m_WindowRect.OffsetRect(-(m_WindowRect.right - ScreenSize.cx), 0);

		// Too far left?
		if (m_WindowRect.left < 0)
			m_WindowRect.OffsetRect( -m_WindowRect.left, 0);

		// Bottom falling out of screen?
		if (m_WindowRect.bottom > ScreenSize.cy)
		{
			CRect ParentRect;
			m_pParent->GetWindowRect(ParentRect);
			m_WindowRect.OffsetRect(0, -(ParentRect.Height() + m_WindowRect.Height()));
		}
	*/
	// Set the window size and position
	MoveWindow(m_WindowRect, TRUE);
}

void CColorPopup::CreateToolTips()
{
	// Create the tool tip
	if (!m_ToolTip.Create(this)) return;

	CRect rect;

	// Add a tool for each cell
	for (int i = 0; i < m_nNumColors; i++)
	{
		if (!GetCellRect(i, rect)) continue;
		m_ToolTip.AddTool(this, GetColorName(i), rect, 1);
	}
	if (m_pText[1]) {
		rect = m_pTextRect[1];
		rect.left += m_nTextWidth;
		m_ToolTip.AddTool(this, LPSTR_TEXTCALLBACK, &rect, 2);
	}
}

void CColorPopup::ChangeSelection(int nIndex, BOOL bDepressed)
{
	CClientDC dc(this);        // device context for drawing

	ASSERT(nIndex < m_nNumColors &&
		(nIndex<CUSTOM_CLR_LIMIT || nIndex>CUSTOM_BOX_VALUE - m_nTextElems));

	// Set Current selection as invalid and redraw old selection (this way
	// the old selection will be drawn unselected)
	if (m_nCurrentSel != nIndex && m_nCurrentSel != INVALID_COLOUR) {
		int OldSel = m_nCurrentSel;
		m_nCurrentSel = INVALID_COLOUR;
		DrawCell(&dc, OldSel);
	}

	// Set the current selection as row/col and draw (it will be drawn selected)
	m_nCurrentSel = nIndex;
	DrawCell(&dc, m_nCurrentSel, bDepressed);

	// Store the current colour
	if (nIndex <= CUSTOM_BOX_VALUE) {
		if (nIndex < CUSTOM_CLR_LIMIT) nIndex -= CUSTOM_CLR_LIMIT;
		nIndex = CUSTOM_BOX_VALUE - nIndex;
		ASSERT(nIndex >= 0 && nIndex < m_nTextElems);
		m_crColor = m_pClrElems[nIndex];
	}
	else m_crColor = GetColor(nIndex);
	if (m_pParent->GetTrackSelection()) m_pParent->SendMessage(CPN_SELCHANGE, (WPARAM)m_crColor, 0);
}

void CColorPopup::EndSelection(int nMessage)
{
	ReleaseCapture();

	if (nMessage == CPN_SELENDCANCEL) m_crColor = m_crInitialColor;
	else if (m_nCurrentSel <= MORECOLORS_BOX_VALUE && m_nCurrentSel > CUSTOM_CLR_LIMIT) {
		int idx = CUSTOM_BOX_VALUE - m_nCurrentSel;
		ASSERT(idx > 0 && idx < m_nTextElems);
		m_crColor = (m_pClrElems[idx] | GET_CUSTOM_COLOR_BIT);
	}

	m_pParent->PostMessage(nMessage, (WPARAM)m_crColor, 0);

	DestroyWindow();
}

void CColorPopup::DrawColorCell(CDC *pDC, CRect &rect, COLORREF clr, UINT flags)
{
	// Select and realize the palette
	CPalette* pOldPalette = NULL;
	if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
	{
		pOldPalette = pDC->SelectPalette(&m_Palette, FALSE);
		pDC->RealizePalette();
	}

	// fill background
	//if (flags==1)
	//    pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DHILIGHT));
	//else
	pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

	// Draw button
	if (flags & 2)
		pDC->DrawEdge(rect, BDR_RAISEDINNER, BF_RECT);
	else if (flags & 1)
		pDC->DrawEdge(rect, BDR_SUNKENOUTER, BF_RECT);

	CBrush brush(PALETTERGB(GetRValue(clr), GetGValue(clr), GetBValue(clr)));
	CPen   pen;
	pen.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));

	CBrush* pOldBrush = (CBrush*)pDC->SelectObject(&brush);
	CPen*   pOldPen = (CPen*)pDC->SelectObject(&pen);

	rect.DeflateRect(m_nMargin + 1, m_nMargin + 1);
	pDC->Rectangle(rect);

	if (IS_CUSTOM(clr)) {
		//Replace interior of rectangle with a custom pattern --
		CUSTOM_COLOR cc;
		cc.pDC = pDC;
		cc.pClr = &clr;
		cc.pRect = &rect;
		rect.DeflateRect(1, 1);
		((CColorCombo *)m_pParent)->DoCustomColor(&cc);
	}

	// restore DC and cleanup
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
	brush.DeleteObject();
	pen.DeleteObject();

	if (pOldPalette && pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
		pDC->SelectPalette(pOldPalette, FALSE);
}


void CColorPopup::DrawTextCell(CDC *pDC, int idx, BOOL bDepressed)
{
	CRect TextButtonRect = m_pTextRect[idx];
	TextButtonRect.top += 2 * m_nMargin;
	if (idx) TextButtonRect.right = TextButtonRect.left + m_nTextWidth;
	// Fill background
	pDC->FillSolidRect(TextButtonRect, ::GetSysColor(COLOR_3DFACE));

	TextButtonRect.DeflateRect(1, 1);

	if (!idx) {
		// Draw thin line around text
		CRect LineRect = TextButtonRect;
		//LineRect.DeflateRect(m_nMargin,m_nMargin);
		CPen pen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));
		CPen* pOldPen = pDC->SelectObject(&pen);
		pDC->SelectStockObject(NULL_BRUSH);
		pDC->Rectangle(LineRect);
		pDC->SelectObject(pOldPen);
	}

	// Draw button --
	if (bDepressed & 1) pDC->DrawEdge(TextButtonRect, BDR_SUNKENOUTER, BF_RECT);
	else if (bDepressed) pDC->DrawEdge(TextButtonRect, BDR_RAISEDINNER, BF_RECT);

	// Draw custom text
	LPCTSTR pText = m_pText[idx];
	CFont *pOldFont = (CFont*)pDC->SelectObject(&m_Font);
	pDC->SetBkMode(TRANSPARENT);
	if (!idx) idx = (DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	else idx = (DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	TextButtonRect.left += 2 * m_nMargin;
	pDC->DrawText(pText, strlen(pText), TextButtonRect, idx);
	pDC->SelectObject(pOldFont);
}

void CColorPopup::DrawCell(CDC* pDC, int nIndex, BOOL bDepressed)
{
	UINT flags = (m_nChosenColorSel == nIndex) + 2 * (m_nCurrentSel == nIndex);

	// For the custom color and text areas --
	if (nIndex <= CUSTOM_BOX_VALUE) {
		if (nIndex < CUSTOM_CLR_LIMIT) {
			int idx = CUSTOM_CLR_LIMIT + CUSTOM_BOX_VALUE - nIndex;

			ASSERT(idx > 0 && idx < m_nTextElems);
			ASSERT(m_pText[idx]);

			// The extent of the actual text button
			CRect TextButtonRect = m_pTextRect[idx];
			TextButtonRect.top += 2 * m_nMargin;
			TextButtonRect.left += m_nTextWidth + 2 * m_nMargin;
			//Use appropriate color: m_crInitialColor if in "More colors..."
			DrawColorCell(pDC, TextButtonRect, m_pClrElems[idx], flags);
		}
		else DrawTextCell(pDC, CUSTOM_BOX_VALUE - nIndex, bDepressed + 2 * (m_nCurrentSel == nIndex));
		return;
	}

	CRect rect;
	if (!GetCellRect(nIndex, rect)) return;
	DrawColorCell(pDC, rect, GetColor(nIndex), flags);
}

BOOL CColorPopup::OnQueryNewPalette()
{
	Invalidate();
	return CWnd::OnQueryNewPalette();
}

void CColorPopup::OnPaletteChanged(CWnd* pFocusWnd)
{
	CWnd::OnPaletteChanged(pFocusWnd);

	if (pFocusWnd->GetSafeHwnd() != GetSafeHwnd())
		Invalidate();
}

void CColorPopup::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	ReleaseCapture();
	//DestroyWindow(); - causes crash when Custom colour dialog appears.
}

// KillFocus problem fix suggested by Paul Wilkerson.
//***7.1 void CColorPopup::OnActivateApp(BOOL bActive, HTASK hTask)
void CColorPopup::OnActivateApp(BOOL bActive, DWORD hTask)
{
	CWnd::OnActivateApp(bActive, hTask);

	// If Deactivating App, cancel this selection
	if (!bActive) EndSelection(CPN_SELENDCANCEL);
}

void CColorPopup::GetRGBStr(char *str, COLORREF clr)
{
	wsprintf(str, "RGB( %02X,%02X,%02X )", GetRValue(clr), GetGValue(clr), GetBValue(clr));
}

BOOL CColorPopup::OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult)
{
	if (pNMHDR->idFrom == 2) {
		GetRGBStr(((TOOLTIPTEXT *)pNMHDR)->szText, m_pClrElems[1]);
		return TRUE;
	}
	return(FALSE);
}

