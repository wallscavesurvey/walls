#include "stdafx.h"
//#include "HeaderCtrlEx.h"
#include "QuickList.h"

#if 0
#ifdef _DEBUG
		char debug_msg[80];
#endif
#endif

void CHeaderCtrlEx::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) // override
{
	ASSERT(lpDrawItemStruct->CtlType == ODT_HEADER);
	HDITEM hdi;
	TCHAR lpBuffer[256];
	hdi.mask = HDI_TEXT;
	hdi.pszText = lpBuffer;
	hdi.cchTextMax = 256;
	GetItem(lpDrawItemStruct->itemID, &hdi);

	CDC dc;

	dc.Attach(lpDrawItemStruct->hDC);

	int nSavedDC = dc.SaveDC();
	CRect rect(lpDrawItemStruct->rcItem);

	// Set clipping region to limit drawing within column
	/*
	{
		CRgn rgn;
		rgn.CreateRectRgnIndirect( &rect );
		dc.SelectObject( &rgn );
		rgn.DeleteObject();
	}
	*/

#if 0
#ifdef _DEBUG
		sprintf(debug_msg,"rect.top=%u rect.bottom=%u",rect.top,rect.bottom);
#endif
#endif

	// Draw the button frame --
	dc.DrawFrameControl(&rect, DFC_BUTTON, DFCS_BUTTONPUSH);

	// Adjust the rect if the mouse button is pressed on it
	if( lpDrawItemStruct->itemState == ODS_SELECTED )
	{
		rect.left++;
		rect.top += 2;
		rect.right++;
	}

	//Draw the text --
	CFont *pFontOld;
	if(m_pCellFont)	pFontOld=dc.SelectObject(m_pCellFont);

	rect.bottom-=3;
	dc.DrawText(lpBuffer, -1, &rect,
		DT_SINGLELINE | DT_CENTER | DT_BOTTOM | DT_END_ELLIPSIS); // | DT_NOCLIP);
	rect.bottom+=3;

	if(m_pCellFont) dc.SelectObject(pFontOld);

	// Draw the Sort arrow
	if(lpDrawItemStruct->itemID == (UINT)m_nSortCol)
	{
		rect=lpDrawItemStruct->rcItem;
		int x0=rect.left+rect.Width()/2;
		int y0=rect.top+2;
		CBrush brush;
		brush.CreateStockObject(BLACK_BRUSH);
		CBrush *pOldBrush = dc.SelectObject(&brush);
		if (m_bSortAsc)
		{
			// Arrow pointing down
			CPoint Pt[3];
			Pt[0] = CPoint(x0+3, y0);	// Right
			Pt[1] = CPoint(x0-3, y0);	// Left
			Pt[2] = CPoint(x0, y0+3);	// Bottom
			dc.Polygon(Pt, 3);
		}
		else
		{
			// Arrow pointing up
			CPoint Pt[3];
			Pt[0] = CPoint(x0,  y0);	// Top
			Pt[1] = CPoint(x0-3,  y0+3);	// Left
			Pt[2] = CPoint(x0+3, y0+3);	// Right
			dc.Polygon(Pt, 3);
		}
		dc.SelectObject(pOldBrush);
	}

	// Restore dc
	dc.RestoreDC( nSavedDC );

	// Detach the dc before returning
	dc.Detach();
}

int CHeaderCtrlEx::SetSortImage( int nCol, bool bAsc )
{
	int nPrevCol = m_nSortCol;

	m_nSortCol = nCol;
	m_bSortAsc = bAsc;

	// Invalidate header control so that it gets redrawn
	Invalidate();
	return nPrevCol;
}

BEGIN_MESSAGE_MAP(CHeaderCtrlEx, CHeaderCtrl)
	ON_WM_LBUTTONUP()
	ON_MESSAGE(HDM_LAYOUT, OnLayout)
	/*
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	*/
	ON_MESSAGE(WM_TABLET_QUERYSYSTEMGESTURESTATUS, OnTabletQuerySystemGestureStatus)
END_MESSAGE_MAP()

LRESULT CHeaderCtrlEx::OnLayout( WPARAM wParam, LPARAM lParam )
{
	LRESULT lResult = CHeaderCtrl::DefWindowProc(HDM_LAYOUT, 0, lParam);

	if(wOSVersion>=0x0600) {
		HD_LAYOUT &hdl = *( HD_LAYOUT * ) lParam;
		RECT *prc = hdl.prc;
		WINDOWPOS *pwpos = hdl.pwpos;

		int nHeight = pwpos->cy-4;

		pwpos->cy = nHeight;
		prc->top = nHeight;
	}

	return lResult;
}

void CHeaderCtrlEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CHeaderCtrl::OnLButtonUp(nFlags, point);
}

LRESULT CHeaderCtrlEx::OnTabletQuerySystemGestureStatus(WPARAM,LPARAM)
{
   return 0;
}

/*
BOOL CHeaderCtrlEx::PreTranslateMessage(MSG* pMsg) 
{
	//MSG msg=*pMsg;
	BOOL bRet = CHeaderCtrl::PreTranslateMessage(pMsg);
	if(pMsg->message== WM_MOUSEMOVE) {
		CWnd *pParent=GetParent();
		ASSERT(pParent);
		if(pParent) {
			CWnd *pdlg=pParent->GetParent();
			ASSERT(pdlg);
			if(pdlg!=GetActiveWindow()) {
				pParent->PostMessage(pMsg); //need to construct wParam, etc
			}
		}
	}
	return bRet;
}
*/

/*
int CHeaderCtrlEx::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
	return ((CQuickList *)GetParent())->OnToolHitTest(point, pTI);
}

BOOL CHeaderCtrlEx::OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT *pResult)
{
	return ((CQuickList *)GetParent())->OnToolTipText(id, pNMHDR, pResult);
}
*/