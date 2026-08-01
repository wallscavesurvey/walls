// ResourceButton.cpp : implementation file
//

#include "stdafx.h"
#include "ResourceButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourceButton

CResourceButton::~CResourceButton()
{
}

BEGIN_MESSAGE_MAP(CResourceButton, baseCResourceButton)
	//{{AFX_MSG_MAP(CResourceButton)
		//ON_WM_MOUSEMOVE()
		//ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
		//ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceButton message handlers
/*
BOOL CResourceButton::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	return baseCResourceButton::PreTranslateMessage(pMsg);
}
*/
void CResourceButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// TODO: Add your code to draw the specified item

	CDC * pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	CDC * pMemDC = new CDC;
	pMemDC->CreateCompatibleDC(pDC);

	CBitmap * pOldBitmap;
	pOldBitmap = pMemDC->SelectObject(&m_Bitmap);

	int offset = (m_index*m_styleCount);
	if (IsWindowEnabled()) {
		if (lpDrawItemStruct->itemState & ODS_SELECTED)
		{
			pDC->BitBlt(0, 0, m_ButtonSize.cx, m_ButtonSize.cy, pMemDC, (offset + 1)*m_ButtonSize.cx, 0, SRCCOPY);
		}
		else {
			pDC->BitBlt(0, 0, m_ButtonSize.cx, m_ButtonSize.cy, pMemDC, offset*m_ButtonSize.cx, 0, SRCCOPY);
		}
	}
	else if (m_styleCount >= 3) {
		pDC->BitBlt(0, 0, m_ButtonSize.cx, m_ButtonSize.cy, pMemDC, (offset + 2)*m_ButtonSize.cx, 0, SRCCOPY);
	}

	// clean up
	pMemDC->SelectObject(pOldBitmap);
	delete pMemDC;
}


BOOL CResourceButton::LoadBitmap(UINT bitmapid, int nCount, int nStyles)
{
	m_Bitmap.Attach(::LoadImage(::AfxGetInstanceHandle(), MAKEINTRESOURCE(bitmapid), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS));
	BITMAP	bitmapbits;
	m_Bitmap.GetBitmap(&bitmapbits);
	m_ButtonSize.cy = bitmapbits.bmHeight;
	m_ButtonSize.cx = bitmapbits.bmWidth / ((m_styleCount = nStyles)*(m_imageCount = nCount));
	SetWindowPos(NULL, 0, 0, m_ButtonSize.cx, m_ButtonSize.cy, SWP_NOMOVE | SWP_NOOWNERZORDER);
	return TRUE;
}

/*
void CResourceButton::OnMouseMove(UINT nFlags, CPoint point)
{
	//	TODO: Add your message handler code here and/or call default

	if (m_bTracking == FALSE)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE|TME_HOVER;
		tme.dwHoverTime = 1;
		m_bTracking = _TrackMouseEvent(&tme);
	}
	baseCResourceButton::OnMouseMove(nFlags, point);
}

LRESULT CResourceButton::OnMouseHover(WPARAM wparam, LPARAM lparam)
{
	// TODO: Add your message handler code here and/or call default
	//m_bHover = TRUE;
	//Invalidate();
	return 0;
}


LRESULT CResourceButton::OnMouseLeave(WPARAM wparam, LPARAM lparam)
{
	//m_bTracking = FALSE;
	//m_bHover=FALSE;
	//Invalidate();
	return 0;
}
*/
