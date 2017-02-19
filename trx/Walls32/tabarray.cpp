// CTabArray
// written by Gerry High
// 74750,2456 CIS

#include "stdafx.h"
#include "tabarray.h"

#include <ctype.h>
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabArray

const int TAB_TABHEIGHT = 19;
const int TAB_MARGIN = 7;

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CTabArray::CTabArray()
{
	m_curTab = -1;
	m_tabHeight = TAB_TABHEIGHT;
	m_position = TABSONTOP;
	m_margin = TAB_MARGIN;
	m_frameBorderOn = TRUE;

	blackPen.CreatePen(PS_SOLID,0,GetSysColor(COLOR_WINDOWFRAME));
	darkPen.CreatePen(PS_SOLID,0,GetSysColor(COLOR_BTNSHADOW));
	lightPen.CreatePen(PS_SOLID,0,GetSysColor(COLOR_BTNHIGHLIGHT));

	m_boldFont = NULL;
	m_normalFont = NULL;
}

CTabArray::~CTabArray()
{
	// free up tab info
	CObject* pTab;
	int j = GetSize();
	for(int i = 0; i < j; i++)
	{
		if ((pTab = GetAt(0)) != NULL)
		{
			RemoveAt(0);
			delete pTab;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTabArray drawing

#ifdef C_TABS
void CTabArray::drawChicagoTabs(CWnd* pWnd, CDC *pDC)
{   
	ASSERT(m_boldFont);
	ASSERT(m_normalFont);

	CPen* pOldPen = pDC->SelectObject(&darkPen);
	CFont* pOldFont = pDC->SelectObject(m_boldFont);
    int nOldMode = pDC->SetBkMode(TRANSPARENT);

    COLORREF oldTextColor = pDC->SetTextColor(GetSysColor(COLOR_BTNTEXT));
	int x, y, width, height;
	CRect rect;
	pDC->GetClipBox(&rect);

	x = m_margin;
	y = m_margin + m_tabHeight;

	CRect rcClient;
	pWnd->GetClientRect(rcClient);

	width = rcClient.Width() - 2 * m_margin;
	height = rcClient.Height() - 2 * m_margin;

	//
	//draw 3 outer lines of bounding rect
	//	
	DKPEN();
 	pDC->MoveTo(x,y);
 	pDC->LineTo(x,y+height-1-m_tabHeight);
	pDC->LineTo(x+width-1,y+height-1-m_tabHeight);
	pDC->LineTo(x+width-1,y-2);

	BKPEN();
	pDC->MoveTo(x,y+height-m_tabHeight);
 	pDC->LineTo(x+width,y+height-m_tabHeight);
 	pDC->LineTo(x+width,y-1);

	LTPEN(); 	
	pDC->MoveTo(x+1,y);
	pDC->LineTo(x+1,y+height-2-m_tabHeight);

	//
	// now draw each tab
	//
	x = m_margin;
	y = m_margin;
	for(int i=0; i < GetSize(); i++)
	{
		CTabInfo* pTab = (CTabInfo*)GetAt(i);
		if(i == m_curTab)
		{
			DKPEN();
 			pDC->MoveTo(x,y+1);
 			pDC->LineTo(x,y+3+m_tabHeight-3);

			LTPEN();
	 		pDC->MoveTo(x+1,y+1);
 			pDC->LineTo(x+1,y+3+m_tabHeight-3);
		
			pDC->MoveTo(x+2,y);	//1 pixel spot
 			pDC->LineTo(x+3,y+1);
		
	 		pDC->MoveTo(x+3,y-1);
			pDC->LineTo(x+pTab->m_tabWidth-2,y-1);

			DKPEN();
	 		pDC->MoveTo(x+3,y-2);
			pDC->LineTo(x+pTab->m_tabWidth-2,y-2);
			pDC->LineTo(x+pTab->m_tabWidth-2,y+m_tabHeight);
		
	 		BKPEN();
			pDC->MoveTo(x+pTab->m_tabWidth-1,y+1);
			pDC->LineTo(x+pTab->m_tabWidth-1,y+m_tabHeight);

			LTPEN();
			if(i != 0)
			{
	 			pDC->MoveTo(x-1,y+3+m_tabHeight-4);
	 			pDC->LineTo(m_margin+1,y+3+m_tabHeight-4);
			}
			pDC->MoveTo(x+pTab->m_tabWidth,y+3+m_tabHeight-4);
			pDC->LineTo(m_margin + width,y+3+m_tabHeight-4);

 			DKPEN();
	 		pDC->MoveTo(x+pTab->m_tabWidth,y+3+m_tabHeight-5);
			pDC->LineTo(m_margin  +width,y+3+m_tabHeight-5);
			pDC->SelectObject(m_boldFont);
		}
		else
		{
			DKPEN();
		 	pDC->MoveTo(x,y+3);
			pDC->LineTo(x,y+3+m_tabHeight-3);

			LTPEN();
	 		pDC->MoveTo(x+1,y+3);
 			pDC->LineTo(x+1,y+3+m_tabHeight-3);
		
	 		pDC->MoveTo(x+2,y+2);	//1 pixel spot
			pDC->LineTo(x+3,y+3);
		
			pDC->MoveTo(x+3,y+1);
	 		pDC->LineTo(x+pTab->m_tabWidth-2,y+1);
		
			DKPEN();
			pDC->MoveTo(x+3,y);
 			pDC->LineTo(x+pTab->m_tabWidth-2,y);
 			pDC->LineTo(x+pTab->m_tabWidth-2,y+m_tabHeight);
		
	 		BKPEN();
			pDC->MoveTo(x+pTab->m_tabWidth-1,y+3);
			pDC->LineTo(x+pTab->m_tabWidth-1,y+m_tabHeight);
			pDC->SelectObject(m_normalFont);
		}

		CRect rect(x, y + 2, x+pTab->m_tabWidth, y + m_tabHeight);
	    pDC->SetTextColor(GetSysColor(pTab->m_active ? COLOR_BTNTEXT:COLOR_GRAYTEXT));
		pDC->DrawText(pTab->m_tabLabel,lstrlen(pTab->m_tabLabel),&rect,
				DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 		x += pTab->m_tabWidth;
		
	}

	pDC->SetTextColor(oldTextColor);

	pDC->SetBkMode(nOldMode);
	pDC->SelectObject(pOldFont);
	pDC->SelectObject(pOldPen);
}
#endif

#ifdef W_TABS
void CTabArray::drawMSWordTabs(CWnd* pWnd, CDC *pDC)
{
	ASSERT(m_boldFont);
	ASSERT(m_normalFont);

	CPen* pOldPen = pDC->SelectObject(&darkPen);
	CFont* pOldFont = pDC->SelectObject(m_boldFont);
    int nOldMode = pDC->SetBkMode(TRANSPARENT);

#ifdef LR_TABS
	switch (m_position)
	{
		case TABSONLEFT:
		case TABSONLEFTBOT:
			drawMSWordLeftTabs(pWnd, pDC);
			break;

		case TABSONRIGHT:
		case TABSONRIGHTBOT:
			drawMSWordRightTabs(pWnd, pDC);
			break;
		default:
#endif
#ifdef T_TABS
			drawMSWordTopTabs(pWnd, pDC);
#endif
#ifdef LR_TABS
			break;
	}
#endif

	pDC->SetBkMode(nOldMode);
	pDC->SelectObject(pOldFont);
	pDC->SelectObject(pOldPen);
}

#ifdef T_TABS

void CTabArray::GetTabRect(RECT *pRect,int tabidx)
{
    int x=m_margin;
    CTabInfo *pTab;
    
    for(int i=0;i<=tabidx;i++) {
	  pTab = (CTabInfo*) GetAt(i);
	  if(i<tabidx) x += pTab->m_tabWidth;
	}
	pRect->left=x;
	pRect->top=m_margin+2; 
	pRect->right=x+pTab->m_tabWidth;
	pRect->bottom=m_margin+m_tabHeight;
}

void CTabArray::drawMSWordTopTabs(CWnd* pWnd, CDC *pDC)
{
    COLORREF oldTextColor = pDC->SetTextColor(GetSysColor(COLOR_BTNTEXT));
	int x, y, width, height;
	CRect rect;
	pDC->GetClipBox(&rect);

	x = m_margin;
	y = m_margin + m_tabHeight;

	CRect rcClient;
	pWnd->GetClientRect(rcClient);

	width = rcClient.Width() - 2 * m_margin;
	height = rcClient.Height() - 2 * m_margin;

	// draw 3 outer lines of bounding rect
	//
	BKPEN();
	pDC->MoveTo(x				, y);
	pDC->LineTo(x				, y + height - m_tabHeight);
	pDC->LineTo(x + width		, y + height - m_tabHeight);
	pDC->LineTo(x + width		, y - 1);

	LTPEN();
	pDC->MoveTo(x + 1			, y);
	pDC->LineTo(x + 1			, y + height - m_tabHeight);
	pDC->MoveTo(x + 2			, y);
	pDC->LineTo(x + 2			, y + height - 1 - m_tabHeight);

	DKPEN();
	pDC->MoveTo(x + 2			, y + height - 1 - m_tabHeight);
	pDC->LineTo(x + width - 1	, y + height - 1 - m_tabHeight);
	pDC->LineTo(x + width - 1	, y);
	pDC->MoveTo(x + 3			, y + height - 2 - m_tabHeight);
	pDC->LineTo(x + width - 2	, y + height - 2 - m_tabHeight);
	pDC->LineTo(x + width - 2	, y + 1);

	// now draw each tab
	//
	x = m_margin;
	y = m_margin;

	for (int i = 0; i < GetSize(); i++)
	{
		CTabInfo *pTab = (CTabInfo*) GetAt(i);

		if (i == m_curTab)
		{
			// Outline
			BKPEN();
			pDC->MoveTo(m_margin				, y + m_tabHeight);
			pDC->LineTo(x						, y + m_tabHeight);	// 1
			pDC->LineTo(x						, y + 4);			// 2
			pDC->LineTo(x + 4					, y);				// 3
			pDC->LineTo(x + pTab->m_tabWidth - 4, y);				// 4
			pDC->LineTo(x + pTab->m_tabWidth	, y + 4);			// 5
			pDC->LineTo(x + pTab->m_tabWidth	, y + m_tabHeight);	// 6
			pDC->LineTo(m_margin + width		, y + m_tabHeight);	// 7

			// Hilite
			for (int j = 0; j < 2; j++)
			{
				LTPEN();
				pDC->MoveTo(m_margin + 1			, y + m_tabHeight + 1 + j);
				pDC->LineTo(x + 1 + j				, y + m_tabHeight + 1 + j);	// 1
				pDC->LineTo(x + 1 + j				, y + 4 + j);				// 2
				pDC->LineTo(x + 4 + j				, y + 1 + j);				// 3
				pDC->LineTo(x + pTab->m_tabWidth - 3, y + 1 + j);				// 4
            	
				pDC->MoveTo(x + pTab->m_tabWidth - 2, y + m_tabHeight + 1 + j);	
				pDC->LineTo(m_margin + width - j	, y + m_tabHeight + 1 + j);	// 7
    	
				if (j)
				{
					pDC->MoveTo(x + 1 + j, y + 4 + j - 1);				// fill corner gap
					pDC->LineTo(x + 4 + j, y + 1 + j - 1);				//
				}

				// Shadow
				DKPEN();
				if(j)
				{
					pDC->MoveTo(x + pTab->m_tabWidth - 4, y + 2);
					pDC->LineTo(x + pTab->m_tabWidth - 1, y + 5);
				}

				pDC->MoveTo(x + pTab->m_tabWidth - 3 - j, y + 2 + j);
				pDC->LineTo(x + pTab->m_tabWidth - 1 - j, y + 4 + j);				// 5
				pDC->LineTo(x + pTab->m_tabWidth - 1 - j, y + m_tabHeight + 2 + j);	// 6
			}
        	
			// Font
			pDC->SelectObject(m_boldFont);
		}
		else
		{
			// Outline
			BKPEN();
			pDC->MoveTo(x						, y + m_tabHeight);
			pDC->LineTo(x						, y + 4);			// 1
			pDC->LineTo(x + 4					, y);				// 2
			pDC->LineTo(x + pTab->m_tabWidth - 4, y);				// 3
			pDC->LineTo(x + pTab->m_tabWidth	, y + 4);			// 4
			pDC->LineTo(x + pTab->m_tabWidth	, y + m_tabHeight);	// 5
    	
			// Hilite
			LTPEN();
			pDC->MoveTo(x + 1					, y + m_tabHeight - 1);
			pDC->LineTo(x + 1					, y + 4);			// 1
			pDC->LineTo(x + 4					, y + 1);			// 2
			pDC->LineTo(x + pTab->m_tabWidth - 3, y + 1);			// 3
	
			// Shadow
			DKPEN();
			pDC->MoveTo(x + pTab->m_tabWidth - 3, y + 2);
			pDC->LineTo(x + pTab->m_tabWidth - 1, y + 4);			// 4
			pDC->LineTo(x + pTab->m_tabWidth - 1, y + m_tabHeight);	// 5
        	
			// Font
			pDC->SelectObject(m_normalFont);
		}

	    pDC->SetTextColor(GetSysColor(pTab->m_active ? COLOR_BTNTEXT:COLOR_GRAYTEXT));
		CRect rect(x, y + 2, x + pTab->m_tabWidth, y + m_tabHeight);
		pDC->DrawText(pTab->m_tabLabel, lstrlen(pTab->m_tabLabel), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    	
		x += pTab->m_tabWidth;
	}
	pDC->SetTextColor(oldTextColor);
}
#endif

#ifdef LR_TABS
void CTabArray::drawMSWordLeftTabs(CWnd* pWnd, CDC *pDC)
{
    COLORREF oldTextColor = pDC->SetTextColor(GetSysColor(COLOR_BTNTEXT));
	int x, y, width, height;
	CRect rect;
	pDC->GetClipBox(&rect);

	CRect rcClient;
	pWnd->GetClientRect(rcClient);

	width = rcClient.Width() - 2 * m_margin;
	height = rcClient.Height() - 2 * m_margin;

	x = m_margin;
	y = m_margin + height;

	if (m_frameBorderOn)
	{
		// draw 3 outer lines of bounding rect
		//
		BKPEN();
		pDC->MoveTo(x + m_tabHeight				, y);
		pDC->LineTo(x + width					, y);
		pDC->LineTo(x + width					, y - height);
		pDC->LineTo(x + m_tabHeight-1			, y - height);

		DKPEN();
		pDC->MoveTo(x + m_tabHeight				, y - 1);
		pDC->LineTo(x + width					, y - 1);
		pDC->MoveTo(x + m_tabHeight				, y - 2);
		pDC->LineTo(x + width - 1				, y - 2);

		pDC->MoveTo(x + width - 1				, y - 1);
		pDC->LineTo(x + width - 1				, y - height);
		pDC->MoveTo(x + width - 2				, y - 1);
		pDC->LineTo(x + width - 2				, y - height + 1);

		LTPEN();
		pDC->MoveTo(x + width - 2				, y - height + 1);
		pDC->LineTo(x + m_tabHeight + 1			, y - height + 1);
		pDC->MoveTo(x + width - 3				, y - height + 2);
		pDC->LineTo(x + m_tabHeight + 2			, y - height + 2);
	}

	if(m_position == TABSONLEFT)
	{
		y = m_margin;
	}
	// now draw each tab
	//
	for (int i = 0; i < GetSize(); i++)
	{
		CTabInfo *pTab = (CTabInfo*)GetAt(i);
		if(m_position == TABSONLEFT && i == 0)
		{
			y += pTab->m_tabWidth;
		}

		if (i == m_curTab)
		{
			// Outline
			BKPEN();
			pDC->MoveTo(x + m_tabHeight			, m_margin + height);
			pDC->LineTo(x + m_tabHeight			, y);						// 1
			pDC->LineTo(x + 4					, y);						// 2
			pDC->LineTo(x						, y - 4);					// 3
			pDC->LineTo(x						, y - pTab->m_tabWidth + 4);// 4
			pDC->LineTo(x + 4					, y - pTab->m_tabWidth);	// 5
			pDC->LineTo(x + m_tabHeight			, y - pTab->m_tabWidth);	// 6
			pDC->LineTo(x + m_tabHeight		 	, m_margin);				// 7

			// Hilite
			for (int j = 0; j < 2; j++)
			{
				LTPEN();
				pDC->MoveTo(x + m_tabHeight + 1 + j	, m_margin + height - j - 1);
				pDC->LineTo(x + m_tabHeight + 1 + j	, y - j - 1);				// 1
				DKPEN();
				pDC->LineTo(x + 4 + j				, y - j - 1);				// 2
				pDC->LineTo(x + 1 + j				, y - j - 4);				// 3
				if(j)
				{
					pDC->MoveTo(x + 4, y - 2);
					pDC->LineTo(x + 1, y - 5);
					pDC->MoveTo(x + 2, y - 5);
				}
				LTPEN();
				pDC->LineTo(x + 1 + j				, y - pTab->m_tabWidth + 3);// 4
            	
				pDC->MoveTo(x + m_tabHeight + 1 + j	, y - pTab->m_tabWidth +1 + j);
				pDC->LineTo(x + m_tabHeight + 1 + j	, m_margin + j);			// 7
    	
				if (j)
				{
					pDC->MoveTo(x + 1		, y - pTab->m_tabWidth + 5);		// fill corner gap
					pDC->LineTo(x + 5		, y - pTab->m_tabWidth + 1);		//
				}

				pDC->MoveTo(x + 1 + j,y - pTab->m_tabWidth + 4 + j);
				pDC->LineTo(x + 4 + j,y - pTab->m_tabWidth + 1 + j);					// 5
				pDC->LineTo(x + m_tabHeight + 2 + j, y - pTab->m_tabWidth + 1 + j);	// 6
			}
        	
			// Font
			pDC->SelectObject(m_boldFont);
		}
		else
		{
			// Outline
			BKPEN();
			pDC->MoveTo(x + m_tabHeight			, y);
			pDC->LineTo(x + 4 					, y);							// 1
			pDC->LineTo(x						, y - 4);						// 2
			pDC->LineTo(x						, y - pTab->m_tabWidth + 4);	// 3
			pDC->LineTo(x + 4					, y - pTab->m_tabWidth);		// 4
			pDC->LineTo(x + m_tabHeight			, y - pTab->m_tabWidth);		// 5
    	
			// Hilite
			DKPEN();
			pDC->MoveTo(x + m_tabHeight	- 1		, y - 1);
			pDC->LineTo(x + 4					, y - 1);						// 1
			pDC->LineTo(x + 1					, y - 4);						// 2
			LTPEN();
			pDC->LineTo(x + 1					, y - pTab->m_tabWidth + 3);  	// 3
	
			// Shadow
			pDC->MoveTo(x + 2					, y - pTab->m_tabWidth + 3);
			pDC->LineTo(x + 4					, y - pTab->m_tabWidth + 1); 	// 4
			pDC->LineTo(x  + m_tabHeight		, y - pTab->m_tabWidth + 1);	// 5
        	
			// Font
			pDC->SelectObject(m_normalFont);
		}

		// DrawText inside clipping rect doesn't seem to work for vertical orientation.
		// TextOut takes the (x,y) coordinates given, and rotates the string about that point.
		// So for our 90 degree rotation, (x,y) becomes the bottom left.
		CSize size = pDC->GetTextExtent(pTab->m_tabLabel, strlen(pTab->m_tabLabel));
		int left = x + (m_tabHeight-size.cy)/2 + 1;
		int bottom  = y - (pTab->m_tabWidth - size.cx)/2;
		int oldMode = pDC->SetBkMode(TRANSPARENT);

	    pDC->SetTextColor(GetSysColor(pTab->m_active ? COLOR_BTNTEXT:COLOR_GRAYTEXT));
		pDC->TextOut(left, bottom, pTab->m_tabLabel, strlen(pTab->m_tabLabel));
		pDC->SetBkMode(oldMode);

		if(m_position == TABSONLEFT)
			y += pTab->m_tabWidth;
		else
			y -= pTab->m_tabWidth;
	}
	pDC->SetTextColor(oldTextColor);
}

void CTabArray::drawMSWordRightTabs(CWnd* pWnd, CDC *pDC)
{
    COLORREF oldTextColor = pDC->SetTextColor(GetSysColor(COLOR_BTNTEXT));
	int x, y, width, height;
	CRect rect;
	pDC->GetClipBox(&rect);

	CRect rcClient;
	pWnd->GetClientRect(rcClient);

	width = rcClient.Width() - 2 * m_margin;
	height = rcClient.Height() - 2 * m_margin;

	x = m_margin + width;
	y = m_margin + height;
	if (m_frameBorderOn)
	{
		// draw 3 outer lines of bounding rect
		//
		BKPEN();
		pDC->MoveTo(x - m_tabHeight				, y);
		pDC->LineTo(x - width					, y);
		pDC->LineTo(x - width     				, y - height);
		pDC->LineTo(x - m_tabHeight - 1			, y - height);

		DKPEN();
		pDC->MoveTo(x - m_tabHeight				, y - 1);
		pDC->LineTo(x - width					, y - 1);
		pDC->MoveTo(x - m_tabHeight				, y - 2);
		pDC->LineTo(x - width + 1				, y - 2);

		pDC->MoveTo(x - width + 1				, y);
		pDC->LineTo(x - width + 1				, y - height);
		pDC->MoveTo(x - width + 2				, y);
		pDC->LineTo(x - width + 2				, y - height + 1);

		LTPEN();
		pDC->MoveTo(x - width + 1				, y - height + 1);
		pDC->LineTo(x - m_tabHeight - 2			, y - height + 1);
		pDC->MoveTo(x - width + 1				, y - height + 2);
		pDC->LineTo(x - m_tabHeight - 3			, y - height + 2);
	}

	if(m_position == TABSONRIGHT)
	{
		y = m_margin;
	}
	x = m_margin + width - m_tabHeight;
	// now draw each tab
	//
	for (int i = 0; i < GetSize(); i++)
	{
		CTabInfo *pTab = (CTabInfo*)GetAt(i);
		if(m_position == TABSONRIGHT && i == 0)
		{
			y += pTab->m_tabWidth;
		}

		if (i == m_curTab)
		{
			// Outline
			BKPEN();
			pDC->MoveTo(x              			, m_margin + height);
			pDC->LineTo(x 						, y);						// 1
			pDC->LineTo(x + m_tabHeight - 4		, y);						// 2
			pDC->LineTo(x + m_tabHeight			, y - 4);					// 3
			pDC->LineTo(x + m_tabHeight			, y - pTab->m_tabWidth + 4);// 4
			pDC->LineTo(x + m_tabHeight - 4		, y - pTab->m_tabWidth);	// 5
			pDC->LineTo(x 						, y - pTab->m_tabWidth);	// 6
			pDC->LineTo(x 		 				, m_margin);				// 7


			// Hilite
			for (int j = 0; j < 2; j++)
			{
				LTPEN();
				pDC->MoveTo(x - 1 - j				, m_margin + height - j - 1);
				pDC->LineTo(x - 1 - j				, y - j - 1);				// 1
				DKPEN();
				pDC->LineTo(x + m_tabHeight - 4 - j	, y - j - 1);				// 2
				pDC->LineTo(x + m_tabHeight - 1 - j	, y - 4 - j);				// 3
				LTPEN();
				pDC->LineTo(x + m_tabHeight - 1 - j	, y - pTab->m_tabWidth + 3);// 4
            	
				pDC->MoveTo(x - 1 - j				, y - pTab->m_tabWidth +1 + j);
				pDC->LineTo(x - 1 - j				, m_margin + j);			// 7
    	
				if (j)
				{
					pDC->MoveTo(x + m_tabHeight - 3 - j	, y - 1 - j);				// fill corner gap
					pDC->LineTo(x + m_tabHeight - j		, y - 4 - j);				//
				}

				pDC->MoveTo(x + m_tabHeight - 2, 	y - pTab->m_tabWidth + 3 + j);
				pDC->LineTo(x + m_tabHeight - 4, 	y - pTab->m_tabWidth + 1 + j);					// 5
				pDC->LineTo(x - 2 - j, y - pTab->m_tabWidth + 1 + j);	// 6
			}
        	
			// Font
			pDC->SelectObject(m_boldFont);
		}
		else
		{
			// Outline
			BKPEN();
			pDC->MoveTo(x						, y);
			pDC->LineTo(x + m_tabHeight - 4 	, y);							// 1
			pDC->LineTo(x + m_tabHeight			, y - 4);						// 2
			pDC->LineTo(x + m_tabHeight			, y - pTab->m_tabWidth + 4);	// 3
			pDC->LineTo(x + m_tabHeight - 4		, y - pTab->m_tabWidth);		// 4
			pDC->LineTo(x 						, y - pTab->m_tabWidth);		// 5
    	
			// Hilite
			DKPEN();
			pDC->MoveTo(x + 1					, y - 1);
			pDC->LineTo(x + m_tabHeight - 5		, y - 1);						// 1
			pDC->LineTo(x + m_tabHeight - 1		, y - 5);						// 2
			LTPEN();
			pDC->LineTo(x + m_tabHeight - 1		, y - pTab->m_tabWidth + 5);  	// 3
	
			// Shadow
			LTPEN();
			pDC->MoveTo(x + m_tabHeight - 2		, y - pTab->m_tabWidth + 3);
			pDC->LineTo(x + m_tabHeight - 4		, y - pTab->m_tabWidth + 1);  	// 4
			pDC->LineTo(x 						, y - pTab->m_tabWidth + 1);	// 5
        	
			// Font
			pDC->SelectObject(m_normalFont);
		}

		// DrawText inside clipping rect doesn't seem to work for vertical orientation.
		// TextOut takes the (x,y) coordinates given, and rotates the string about that point.
		// So for our 90 degree rotation, (x,y) becomes the bottom left.
		CSize size = pDC->GetTextExtent(pTab->m_tabLabel, strlen(pTab->m_tabLabel));
		int left = x + m_tabHeight - (m_tabHeight-size.cy)/2 + 1;
		int top  = y - pTab->m_tabWidth + (pTab->m_tabWidth - size.cx)/2;
		int oldMode = pDC->SetBkMode(TRANSPARENT);

	    pDC->SetTextColor(GetSysColor(pTab->m_active ? COLOR_BTNTEXT:COLOR_GRAYTEXT));
		pDC->TextOut(left, top, pTab->m_tabLabel, strlen(pTab->m_tabLabel));
		pDC->SetBkMode(oldMode);

		if(m_position == TABSONRIGHT)
			y += pTab->m_tabWidth;
		else
			y -= pTab->m_tabWidth;
	}
	pDC->SetTextColor(oldTextColor);
}
#endif
#endif

