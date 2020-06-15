#include "stdafx.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "ImageViewDlg.h"
#include "ThumbListCtrl.h"
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <afxdisp.h>	// OLE stuff
//#include <shlwapi.h>	// Shell functions (PathFindExtension() in this case)
//#include <afxpriv.h>	// ANSI to/from Unicode conversion macros

#ifdef _USE_TT

int CThumbListCtrl::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
	int index = HitTest(point);
	if (index < 0)
		return -1;

	CRect rect;
	GetItemRect(index, &rect, LVIR_BOUNDS);
	//GetItemRect(index, &rectLabel, LVIR_LABEL );

	pTI->hwnd = m_hWnd;
	pTI->uId = (UINT)((index << 10) + 1);
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	pTI->rect = rect;

	return pTI->uId;
}

//////////////////////////////////////////////////
// OnToolTipText()                              //
// Modify this function to change the text      //
// displayed in the Tool Tip.                   //
//////////////////////////////////////////////////
BOOL CThumbListCtrl::OnToolTipText(UINT /*id*/, NMHDR * pNMHDR, LRESULT * pResult)
{
	UINT nID = pNMHDR->idFrom;

	// check if this is the automatic tooltip of the control
	if (nID == 0)
		return TRUE;	// do not allow display of automatic tooltip,
						// or our tooltip will disappear
	//*pResult = 0; default

	//must be called each time when other tooltip contexts are active! 
	ToolTipCustomize(64, 32000, 32, NULL);

	LPSTR pTipText = m_pDlg->GetTooltipText(((nID - 1) >> 10) & 0x3fffff, pNMHDR->code);
	if (!pTipText) return TRUE; 	// we handled the message (no tooltip)
	if (pNMHDR->code == TTN_NEEDTEXTA) {
		TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
		pTTTA->hinst = 0;
		pTTTA->lpszText = pTipText;
	}
	else {
		ASSERT(pNMHDR->code == TTN_NEEDTEXTW);
		TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
		pTTTW->hinst = 0;
		pTTTW->lpszText = (LPWSTR)pTipText;
	}

#if 0
#ifndef _UNICODE
	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		/*
		if(m_pchTip != NULL)
		  delete m_pchTip;

		m_pchTip = new TCHAR[strTip.GetLength()+1];
		lstrcpyn(m_pchTip, strTip, strTip.GetLength());
		m_pchTip[strTip.GetLength()] = 0;
	   ((TOOLTIPTEXTA*)pNMHDR)->lpszText = (WCHAR*)m_pchTip;
		*/
		*((TOOLTIPTEXTA*)pNMHDR)->szText = 0; //necessary?
		((TOOLTIPTEXTA*)pNMHDR)->hinst = NULL; //lpszText is not a string resource id!
		((TOOLTIPTEXTA*)pNMHDR)->lpszText = (LPSTR)pTipText;
	}
	else
	{
		ASSERT(0); //test if handling this is required!
		/*
		if(m_pwchTip != NULL)
		  delete m_pwchTip;

		m_pwchTip = new WCHAR[strTip.GetLength()+1];
		_mbstowcsz(m_pwchTip, strTip, strTip.GetLength());
		m_pwchTip[strTip.GetLength()] = 0; // end of text
		((TOOLTIPTEXTW*)pNMHDR)->lpszText = (WCHAR*)m_pwchTip;
		*/
		return FALSE;
	}
#else
	if (pNMHDR->code == TTN_NEEDTEXTA)
		_wcstombsz(pTTTA->szText, strTip, 80);
	else
		lstrcpyn(pTTTW->szText, strTip, 80);
#endif
#endif
	return TRUE;    // message was handled
}
#endif //USE_TT
