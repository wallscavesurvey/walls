//***************************************************************************
//
// AUTHOR:  James White (feel free to remove or otherwise mangle any part)
//
//***************************************************************************
#include "stdafx.h"
#include "ColorButton.h"

//***********************************************************************
//**                         MFC Debug Symbols                         **
//***********************************************************************
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//***********************************************************************
//**                            DDX Method                            **
//***********************************************************************

void AFXAPI DDX_ColorCombo(CDataExchange *pDX, int nIDC, COLORREF& crColor)
{
	HWND hWndCtrl = pDX->PrepareCtrl(nIDC);
	ASSERT(hWndCtrl != NULL);

	CColorCombo* pColorCombo = (CColorCombo*)CWnd::FromHandle(hWndCtrl);
	if (pDX->m_bSaveAndValidate)
	{
		crColor = pColorCombo->Color;
	}
	else // initializing
	{
		pColorCombo->Color = crColor;
	}
}

//***********************************************************************
//**                             Constants                             **
//***********************************************************************
const int g_ciArrowSizeX = 4;
const int g_ciArrowSizeY = 2;

LPCTSTR dflt_pText[MAX_TEXT_ELEMS] = { NULL,"More colors...","Color by depth...",
							"Color by date...","Color by stats..." };

//***********************************************************************
//**                            MFC Macros                            **
//***********************************************************************
IMPLEMENT_DYNCREATE(CColorCombo, CButton)

//***********************************************************************
// Method:	CColorCombo::CColorCombo(void)
// Notes:	Default Constructor.
//***********************************************************************
CColorCombo::CColorCombo(void) :
	_Inherited(),
	m_Color(0x00C0C0C0),
	m_pText(dflt_pText),
	m_nTextElems(2),
	m_bPopupActive(FALSE),
	m_bTrackSelection(FALSE),
	m_pToolTips(NULL)
{
	m_ClrElems[0] = 0x00C0C0C0;
	m_ClrElems[1] = 0x00FFFFFF;
}

//***********************************************************************
// Method:	CColorCombo::~CColorCombo(void)
// Notes:	Destructor.
//***********************************************************************
CColorCombo::~CColorCombo(void)
{
}

//***********************************************************************
// Method:	CColorCombo::GetColor()
// Notes:	None.
//***********************************************************************
COLORREF CColorCombo::GetColor(void) const
{
	return m_Color;
}


//***********************************************************************
// Method:	CColorCombo::SetColor()
// Notes:	None.
//***********************************************************************
void CColorCombo::SetColor(COLORREF Color)
{
	m_Color = Color;

	if (m_nTextElems < 2) {
		ASSERT(FALSE);
	}
	for (int i = 1; i < m_nTextElems; i++) {
		m_ClrElems[i] = ((i - 1) << CUSTOM_COLOR_SHIFT) | (Color & 0xFFFFFF);
	}

	if (::IsWindow(m_hWnd)) RedrawWindow();
}


//***********************************************************************
// Method:	CColorCombo::GetDefaultColor()
// Notes:	None.
//***********************************************************************
COLORREF CColorCombo::GetDefaultColor(void) const
{
	return m_ClrElems[0];
}

//***********************************************************************
// Method:	CColorCombo::SetDefaultColor()
// Notes:	None.
//***********************************************************************
void CColorCombo::SetDefaultColor(COLORREF Color)
{
	m_ClrElems[0] = Color;
}

//***********************************************************************
// Method:	CColorCombo::SetCustomText()
// Notes:	None.
//***********************************************************************
void CColorCombo::SetCustomText(int nTextElems, LPCTSTR *pText /*=dflt_pText*/)
{
	ASSERT(nTextElems <= MAX_TEXT_ELEMS);
	m_nTextElems = nTextElems;
	m_pText = pText;
}

//***********************************************************************
// Method:	CColorCombo::SetTrackSelection()
// Notes:	None.
//***********************************************************************
void CColorCombo::SetTrackSelection(BOOL bTrack)
{
	m_bTrackSelection = bTrack;
}

//***********************************************************************
// Method:	CColorCombo::GetTrackSelection()
// Notes:	None.
//***********************************************************************
BOOL CColorCombo::GetTrackSelection(void) const
{
	return m_bTrackSelection;
}

//***********************************************************************
//**                         CButton Overrides                         **
//***********************************************************************
void CColorCombo::PreSubclassWindow()
{
	ModifyStyle(0, BS_OWNERDRAW);

	_Inherited::PreSubclassWindow();
}

//***********************************************************************
//**                         Message Handlers                         **
//***********************************************************************
BEGIN_MESSAGE_MAP(CColorCombo, CButton)
	//{{AFX_MSG_MAP(CColorCombo)
	ON_CONTROL_REFLECT_EX(BN_CLICKED, OnClicked)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
	ON_MESSAGE(CPN_SELENDOK, OnSelEndOK)
	ON_MESSAGE(CPN_SELENDCANCEL, OnSelEndCancel)
	ON_MESSAGE(CPN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()


//***********************************************************************
// Method:	CColorCombo::OnSelEndOK()
// Notes:	None.
//***********************************************************************
LONG CColorCombo::OnSelEndOK(UINT wParam, LONG /*lParam*/)
{
	BOOL bColorChanged;

	m_bPopupActive = FALSE;
	if (m_pToolTips) m_pToolTips->Activate(FALSE);

	if (IS_GET_CUSTOM(wParam)) {
		CUSTOM_COLOR cc;
		cc.pDC = NULL;
		wParam &= ~GET_CUSTOM_COLOR_BIT;
		cc.pClr = (COLORREF *)&wParam;
		if (!DoCustomColor(&cc)) return OnSelEndCancel((UINT)m_Color, 0);
		bColorChanged = TRUE;
	}
	else bColorChanged = (COLORREF)wParam != m_Color;


	if (bColorChanged) {
		SetColor((COLORREF)wParam);
		Invalidate();
	}

	CWnd *pParent = GetParent();

	if (pParent)
	{
		pParent->SendMessage(CPN_CLOSEUP, wParam, (LPARAM)GetDlgCtrlID());
		if (bColorChanged) pParent->SendMessage(CPN_SELCHANGE, wParam, (LPARAM)GetDlgCtrlID());
		else if (!m_bTrackSelection) pParent->SendMessage(CPN_SELENDCANCEL, m_Color, (LPARAM)GetDlgCtrlID());
		if (m_bTrackSelection) pParent->SendMessage(CPN_SELENDOK, wParam, (LPARAM)GetDlgCtrlID());
	}

	return TRUE;
}


//***********************************************************************
// Method:	CColorCombo::OnSelEndCancel()
// Notes:	None.
//***********************************************************************
LONG CColorCombo::OnSelEndCancel(UINT wParam, LONG /*lParam*/)
{
	m_bPopupActive = FALSE;
	if (m_pToolTips) m_pToolTips->Activate(FALSE);

	Color = (COLORREF)wParam;

	CWnd *pParent = GetParent();

	if (pParent)
	{
		if (m_pToolTips) m_pToolTips->Activate(FALSE);
		pParent->SendMessage(CPN_CLOSEUP, wParam, (LPARAM)GetDlgCtrlID());
		pParent->SendMessage(CPN_SELENDCANCEL, wParam, (LPARAM)GetDlgCtrlID());
	}

	return TRUE;
}


//***********************************************************************
// Method:	CColorCombo::OnSelChange()
// Notes:	None.
//***********************************************************************
LONG CColorCombo::OnSelChange(UINT wParam, LONG /*lParam*/)
{
	if (m_bTrackSelection) {
		Color = (COLORREF)wParam;
		CWnd *pParent = GetParent();
		if (pParent) pParent->SendMessage(CPN_SELCHANGE, wParam, (LPARAM)GetDlgCtrlID());
	}

	return TRUE;
}

//***********************************************************************
// Method:	CColorCombo::OnCreate()
// Notes:	None.
//***********************************************************************
int CColorCombo::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CButton::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

//***********************************************************************
// Method:	CColorCombo::OnClicked()
// Notes:	None.
//***********************************************************************
BOOL CColorCombo::OnClicked()
{
	m_bPopupActive = TRUE;

	CRect rDraw;
	GetWindowRect(rDraw);

	new CColorPopup(CPoint(rDraw.left, rDraw.bottom),		// Point to display popup
		m_Color,								// Selected colour
		this,									// parent
		m_pText,
		m_ClrElems,
		m_nTextElems);

	CWnd *pParent = GetParent();

	if (pParent)
		pParent->SendMessage(CPN_DROPDOWN, (LPARAM)m_Color, (LPARAM)GetDlgCtrlID());

	return TRUE;
}



//***********************************************************************
// Method:	CColorCombo::DrawItem()
// Notes:	None.
//***********************************************************************
void CColorCombo::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	ASSERT(lpDrawItemStruct);

	CDC*    pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	UINT    state = lpDrawItemStruct->itemState;
	CRect   rDraw = lpDrawItemStruct->rcItem;
	CRect	rArrow;

	if (m_bPopupActive)
		state |= ODS_SELECTED | ODS_FOCUS;

	//******************************************************
	//**                  Draw Outer Edge
	//******************************************************
	UINT uFrameState = DFCS_BUTTONPUSH | DFCS_ADJUSTRECT;

	if (state & ODS_SELECTED)
		uFrameState |= DFCS_PUSHED;

	if (state & ODS_DISABLED)
		uFrameState |= DFCS_INACTIVE;

	pDC->DrawFrameControl(&rDraw,
		DFC_BUTTON,
		uFrameState);


	if (state & ODS_SELECTED)
		rDraw.OffsetRect(1, 1);

	//******************************************************
	//**                     Draw Focus
	//******************************************************
	if (state & ODS_FOCUS)
	{
		RECT rFocus = { rDraw.left,
					   rDraw.top,
					   rDraw.right - 1,
					   rDraw.bottom };

		pDC->DrawFocusRect(&rFocus);
	}

	rDraw.DeflateRect(::GetSystemMetrics(SM_CXEDGE),
		::GetSystemMetrics(SM_CYEDGE));

	//******************************************************
	//**                     Draw Arrow
	//******************************************************
	rArrow.left = rDraw.right - g_ciArrowSizeX - ::GetSystemMetrics(SM_CXEDGE) / 2;
	rArrow.right = rArrow.left + g_ciArrowSizeX;
	rArrow.top = (rDraw.bottom + rDraw.top) / 2 - g_ciArrowSizeY / 2;
	rArrow.bottom = (rDraw.bottom + rDraw.top) / 2 + g_ciArrowSizeY / 2;

	DrawArrow(pDC,
		&rArrow,
		0,
		(state & ODS_DISABLED)
		? ::GetSysColor(COLOR_GRAYTEXT)
		: RGB(0, 0, 0));


	rDraw.right = rArrow.left - ::GetSystemMetrics(SM_CXEDGE) / 2;

	//******************************************************
	//**                   Draw Separator
	//******************************************************
	pDC->DrawEdge(&rDraw,
		EDGE_ETCHED,
		BF_RIGHT);

	rDraw.right -= (::GetSystemMetrics(SM_CXEDGE) * 2) + 1;

	//******************************************************
	//**                     Draw Color
	//******************************************************
	if ((state & ODS_DISABLED) == 0)
	{
		if (IS_CUSTOM(m_Color)) {
			CUSTOM_COLOR cc;
			cc.pDC = pDC;
			cc.pClr = &m_Color;
			cc.pRect = &rDraw;
			DoCustomColor(&cc);
		}
		else pDC->FillSolidRect(&rDraw, m_Color);

		::FrameRect(pDC->m_hDC,
			&rDraw,
			(HBRUSH)::GetStockObject(BLACK_BRUSH));
	}
}


//***********************************************************************
//**                          Static Methods                          **
//***********************************************************************

//***********************************************************************
// Method:	CColorCombo::DrawArrow()
// Notes:	None.
//***********************************************************************
void CColorCombo::DrawArrow(CDC* pDC,
	RECT* pRect,
	int iDirection,
	COLORREF clrArrow /*= RGB(0,0,0)*/)
{
	POINT ptsArrow[3];

	switch (iDirection)
	{
	case 0: // Down
	{
		ptsArrow[0].x = pRect->left;
		ptsArrow[0].y = pRect->top;
		ptsArrow[1].x = pRect->right;
		ptsArrow[1].y = pRect->top;
		ptsArrow[2].x = (pRect->left + pRect->right) / 2;
		ptsArrow[2].y = pRect->bottom;
		break;
	}

	case 1: // Up
	{
		ptsArrow[0].x = pRect->left;
		ptsArrow[0].y = pRect->bottom;
		ptsArrow[1].x = pRect->right;
		ptsArrow[1].y = pRect->bottom;
		ptsArrow[2].x = (pRect->left + pRect->right) / 2;
		ptsArrow[2].y = pRect->top;
		break;
	}

	case 2: // Left
	{
		ptsArrow[0].x = pRect->right;
		ptsArrow[0].y = pRect->top;
		ptsArrow[1].x = pRect->right;
		ptsArrow[1].y = pRect->bottom;
		ptsArrow[2].x = pRect->left;
		ptsArrow[2].y = (pRect->top + pRect->bottom) / 2;
		break;
	}

	case 3: // Right
	{
		ptsArrow[0].x = pRect->left;
		ptsArrow[0].y = pRect->top;
		ptsArrow[1].x = pRect->left;
		ptsArrow[1].y = pRect->bottom;
		ptsArrow[2].x = pRect->right;
		ptsArrow[2].y = (pRect->top + pRect->bottom) / 2;
		break;
	}
	}

	CBrush brsArrow(clrArrow);
	CPen penArrow(PS_SOLID, 1, clrArrow);

	CBrush* pOldBrush = pDC->SelectObject(&brsArrow);
	CPen*   pOldPen = pDC->SelectObject(&penArrow);

	pDC->SetPolyFillMode(WINDING);
	pDC->Polygon(ptsArrow, 3);

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

BOOL CColorCombo::DoCustomColor(CUSTOM_COLOR *pCC)
{
	int bRet;
	CWnd *pParent = GetParent();
	if (!pParent) {
		ASSERT(FALSE);
		return FALSE;
	}
	if (!pCC->pDC) {
		if (IS_CUSTOM(*pCC->pClr)) {
			bRet = pParent->SendMessage(CPN_GETCUSTOM, (WPARAM)pCC->pClr, (LPARAM)GetDlgCtrlID());
		}
		else {
			CColorDialog dlg(*pCC->pClr, CC_FULLOPEN | CC_ANYCOLOR, this);
			bRet = (IDOK == dlg.DoModal());
			if (bRet) *pCC->pClr = dlg.GetColor();
		}
	}
	else {
		ASSERT(IS_CUSTOM(*pCC->pClr));
		bRet = pParent->SendMessage(CPN_DRAWCUSTOM, (LPARAM)pCC);
	}
	return bRet;
}

char * CColorCombo::GetColorName(char *buf)
{
	return CColorPopup::GetColorName(buf, m_Color);
}

void CColorCombo::AddToolTip(CToolTipCtrl *pTTCtrl)
{
	m_pToolTips = pTTCtrl;
	CRect rect;
	GetWindowRect(&rect);
	ScreenToClient(&rect);
	pTTCtrl->AddTool(this, LPSTR_TEXTCALLBACK, &rect, 3);
}
BOOL CColorCombo::PreTranslateMessage(MSG* pMsg)
{
	if (m_pToolTips) {
		switch (pMsg->message)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			m_pToolTips->Activate(TRUE);
			m_pToolTips->RelayEvent(pMsg);
		}
	}
	return CButton::PreTranslateMessage(pMsg);
}
BOOL CColorCombo::OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT *)
{
	if (pNMHDR->idFrom == 3) {
		GetColorName(((TOOLTIPTEXT *)pNMHDR)->szText);
		return(TRUE);
	}
	return FALSE;
}
