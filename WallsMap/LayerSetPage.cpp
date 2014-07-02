// LayerSetPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "Layersetsheet.h"

/////////////////////////////////////////////////////////////////////////////
// CLayerSetPage property page

BEGIN_MESSAGE_MAP(CLayerSetPage, CPropertyPage)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_ERASEBKGND()
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

BOOL CLayerSetPage::OnInitDialog() 
{
	BOOL bret = CPropertyPage::OnInitDialog();

	CRect	cltRect;

	GetClientRect(&cltRect);
	m_cltR0 = cltRect;
	ClientToScreen(&m_cltR0);
	m_cltRect = m_cltR0;

	m_bNeedInit = FALSE;

	return bret;
}

void CLayerSetPage::OnSize(UINT nType,int cx,int cy)
{
   CPropertyPage::OnSize(nType,cx,cy);

   if (m_bNeedInit)
      return;

	std::vector<CItemCtrl>::iterator it;
	int	 nCount;

	if((nCount = m_Items.size()) > 0) {
		CRect  cltRect;
		GetClientRect(&cltRect);
		ClientToScreen(cltRect);

		HDWP   hDWP;
		int    sizeType = WST_NONE;		

		switch (Sheet()->m_nDelaySide) {
		case WMSZ_BOTTOM:
			sizeType = WST_BOTTOM;
			break;
		case WMSZ_BOTTOMLEFT:
			sizeType = WST_BOTTOM|WST_LEFT;
			break;
		case WMSZ_BOTTOMRIGHT:
			sizeType = WST_BOTTOM|WST_RIGHT;
			break;
		case WMSZ_LEFT:
			sizeType = WST_LEFT;
			break;
		case WMSZ_RIGHT:
			sizeType = WST_RIGHT;
			break;
		case WMSZ_TOP:
			sizeType = WST_TOP;
			break;
		case WMSZ_TOPLEFT:
			sizeType = WST_TOP|WST_LEFT;
			break;
		case WMSZ_TOPRIGHT:
			sizeType = WST_TOP|WST_RIGHT;
			break;
		default:
			break;
		}

		if (sizeType != WST_NONE) {
			hDWP = ::BeginDeferWindowPos(nCount);

			for (it = m_Items.begin(); it != m_Items.end(); it++)
				hDWP = it->OnSize(hDWP, sizeType, &cltRect, &m_cltRect, &m_cltR0, this);

			::EndDeferWindowPos(hDWP);
		}

		m_cltRect = cltRect;
	}
}

void CLayerSetPage::OnGetMinMaxInfo(MINMAXINFO *pmmi)
{
	//if ((HWND)m_wndSizeIcon == NULL)
		//return;

	pmmi->ptMinTrackSize.x = m_xMin-250;
	pmmi->ptMinTrackSize.y = m_yMin;

	if (m_xSt == CST_NONE)
		pmmi->ptMaxTrackSize.x = pmmi->ptMaxSize.x = m_xMin;
	if (m_ySt == CST_NONE)
		pmmi->ptMaxTrackSize.y = pmmi->ptMaxSize.y = m_yMin;
}

BOOL CLayerSetPage::OnEraseBkgnd(CDC *pDC)
{
	if (!(GetStyle() & WS_CLIPCHILDREN)) {
		std::vector<CItemCtrl>::const_iterator it;

		for(it = m_Items.begin();it != m_Items.end(); it++) {
			// skip over the size icon if it's been hidden
			//if(it->m_nID == m_idSizeIcon && !m_wndSizeIcon.IsWindowVisible())
				//continue;

			if(it->m_bFlickerFree && ::IsWindowVisible(GetDlgItem(it->m_nID)->GetSafeHwnd())) {
				pDC->ExcludeClipRect(&it->m_wRect);
			}
		}
	}

	CDialog::OnEraseBkgnd(pDC);
	return FALSE;
}

void CLayerSetPage::AddControl( UINT nID,int xl, int xr, int yt, int yb, int bFlickerFree,
								double xRatio, double cxRatio, double yRatio, double cyRatio )
{
	CItemCtrl	item;
	CRect		cltRect;

	GetDlgItem(nID)->GetWindowRect(&item.m_wRect);
	ScreenToClient(&item.m_wRect);

	item.m_nID = nID;
	item.m_stxLeft = xl;
	item.m_stxRight = xr;
	item.m_styTop = yt;
	item.m_styBottom = yb;
	item.m_bFlickerFree = !!(bFlickerFree & 0x01);
	item.m_bInvalidate = !!(bFlickerFree & 0x02);
	item.m_xRatio = xRatio;
	item.m_cxRatio = cxRatio;
	item.m_yRatio = yRatio;
	item.m_cyRatio = cyRatio;

	GetClientRect(&cltRect);
	if (xl == CST_RELATIVE || xl == CST_ZOOM || xr == CST_RELATIVE || xr == CST_ZOOM)
		item.m_xRatio = (item.m_wRect.left + item.m_wRect.right) / 2.0 / cltRect.Width();

	if (yt == CST_RELATIVE || yt == CST_ZOOM || yb == CST_RELATIVE || yb == CST_ZOOM)
		item.m_yRatio = (item.m_wRect.bottom + item.m_wRect.top ) / 2.0 / cltRect.Height();

	std::vector<CItemCtrl>::iterator it;
	int  nCount;

	if((nCount = m_Items.size()) > 0) {
		for (it = m_Items.begin(); it != m_Items.end(); it++) {
			if (it->m_nID == item.m_nID) {
				*it = item;
				return;
			}
		}
	}

	m_Items.push_back(item);

}

#if 0
BOOL CLayerSetPage::OnKillActive() 
{
	//m_idLastFocus=checkid(GetFocus()->GetDlgCtrlID(),getids());
	//return UpdateData(TRUE) && (!hPropHook || ValidateData());
	return CPropertyPage::OnKillActive();
}
#endif

BOOL CLayerSetPage::OnSetActive()
{
	BOOL bRet=CPropertyPage::OnSetActive();
	if(bRet) {
		PostMessage(WM_RETFOCUS);
	}
	return bRet;
}
