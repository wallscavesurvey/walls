// mapframe.h : header file
//
#if !defined(PRJHIER_H)
#include "prjhier.h"
#endif

class CVNode;
struct MAP_VECNODE;

class CMapFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CMapFrame)
protected:
	CMapFrame();			// protected constructor used by dynamic creation

// Attributes
public:
   CBitmap *m_pBmp;			// Initialized in CPrjDoc::PlotFrame(). Destructor must delete it!
   CSize m_sizeBmp;         // Size of bitmap in pixels
   UINT m_flags;
   CPoint m_ptSize;
   CMapFrame *m_pNext;
   CRect *m_pCRect;			// Used when zooming or panning from view itself
   CVNode *m_pVnode,*m_pVnodeF;
   static BOOL m_bKeepVnode;
   BOOL m_bProfile,m_bFeetUnits;
   CPrjListNode *m_pFrameNode;
   double m_cosAx,m_sinAx,m_midEx,m_midNx,m_scale,m_xoff,m_yoff,m_fView;
   double m_sinA,m_cosA,m_midE,m_midN,m_midU;
   double m_org_xoff,m_org_yoff,m_org_scale;
   
// Operations
public:
void InitializeFrame(CBitmap *pBmp,CSize size,double xoff,double yoff,double scale);
void SetTitle(const char *pTitle,UINT flags);
void RemoveThis();

MAP_VECNODE *GetVecNode(CPoint point);

//static void DetachFrameSet();

// Implementation
protected:
	virtual ~CMapFrame();
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	 
	afx_msg void OnIconEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	
	// Generated message map functions
	//{{AFX_MSG(CMapFrame)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
