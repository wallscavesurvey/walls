// tabarray.h : interface of the CTabArray class
// Written by Gerry High
// V1.2 4/13/94
//
/////////////////////////////////////////////////////////////////////////////
// Modifications for my use (DMcK) --
// Must define one or both symbols in each pair below:
//
// T_TABS:  enable top tab style
// LR_TABS: enable Left/Right tab styles
//
// W_TABS:  enable MS Word style tab outline
// C_TABS:  enable Chicago style tab outline (use with T_TABS only)

#define T_TABS
#undef  LR_TABS

#define W_TABS
#undef  C_TABS
/////////////////////////////////////////////////////////////////////////////

#ifndef __MFX_TABARRAY_H__
#define __MFX_TABARRAY_H__

/////////////////////////////////////////////////////////////////////////////
// CTabInfo

class CTabInfo : public CObject
{
public:
	CTabInfo()
	{
		m_tabWidth = 0;
		m_tabLabel = NULL;
		m_pView = NULL;
		m_mnemonic = (char)0;
		m_active=TRUE;
	}
	~CTabInfo()
	{
		if(m_tabLabel)
		{
			delete m_tabLabel;
		}
	}
	int 		m_tabWidth;	// width of tab
	char* 		m_tabLabel;	// label of tab 
	CWnd* 		m_pView;	// pointer to CWnd object
	char		m_mnemonic;	// character of mnemonic
	BOOL		m_active;	// is this tab active?
};

enum eLookAndFeel { LAF_CHICAGO,LAF_MSWORD};
enum eTabPosition { TABSONTOP, TABSONLEFT, TABSONLEFTBOT,TABSONRIGHT, TABSONRIGHTBOT, TABSONBOTTOM };

#define BKPEN()pDC->SelectObject(&blackPen)
#define LTPEN()pDC->SelectObject(&lightPen)
#define DKPEN()pDC->SelectObject(&darkPen)

/////////////////////////////////////////////////////////////////////////////
// CTabArray

class CTabArray : public CObArray
{
// Construction/Destruction
public:
	CTabArray();
	~CTabArray();

// Attributes
public:
	int m_curTab;				// index of current tab
	int m_margin;				// margin around tab "pages"
	int m_tabHeight;			// height of a tab (including margins, etc.)
	eTabPosition m_position;	// position of tabs
	int m_lfEscapement;			// font escapement (rotation)
	BOOL m_frameBorderOn;		// draw tab page frame?

	CFont* m_normalFont;		// font of non-current tab
	CFont* m_boldFont;			// font of current tab

// Operations
public:

#ifdef C_TABS
	void drawChicagoTabs(CWnd* pWnd, CDC *pDC);
#endif
#ifdef W_TABS
	void drawMSWordTabs(CWnd* pWnd, CDC *pDC);
#ifdef T_TABS
	void GetTabRect(RECT *pRect,int tabidx);
#endif
#endif

// Implementation
protected:
	CPen blackPen;
	CPen darkPen;
	CPen lightPen;
	CPen* pOldPen;

#ifdef T_TABS
	void drawMSWordTopTabs(CWnd* pWnd, CDC *pDC);
#endif
#ifdef LR_TABS
	void drawMSWordLeftTabs(CWnd* pWnd, CDC *pDC);
	void drawMSWordRightTabs(CWnd* pWnd, CDC *pDC);
#endif
};

#endif

/////////////////////////////////////////////////////////////////////////////
