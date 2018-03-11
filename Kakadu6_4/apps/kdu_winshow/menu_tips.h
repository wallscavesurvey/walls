/*****************************************************************************/
// File: menu_tips.h [scope = APPS/WINSHOW]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/*****************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/*****************************************************************************/
// Licensee: Mr David McKenzie
// License number: 00590
// The licensee has been granted a NON-COMMERCIAL license to the contents of
// this source file.  A brief summary of this license appears below.  This
// summary is not to be relied upon in preference to the full text of the
// license agreement, accepted at purchase of the license.
// 1. The Licensee has the right to install and use the Kakadu software and
//    to develop Applications for the Licensee's own use.
// 2. The Licensee has the right to Deploy Applications built using the
//    Kakadu software to Third Parties, so long as such Deployment does not
//    result in any direct or indirect financial return to the Licensee or
//    any other Third Party, which further supplies or otherwise uses such
//    Applications.
// 3. The Licensee has the right to distribute Reusable Code (including
//    source code and dynamically or statically linked libraries) to a Third
//    Party, provided the Third Party possesses a license to use the Kakadu
//    software, and provided such distribution does not result in any direct
//    or indirect financial return to the Licensee.
/******************************************************************************
Description:
   Adds tool-tips to menu items without the need for a status bar.  Adapted
from a 2003 article by Paul DiLascia in MSDN magazine so as to make the
complex set of menu items in "kdu_show" more user friendly under Windows.
******************************************************************************/
#ifndef MENU_TIPS_H
#define MENU_TIPS_H

// Defined here:
class CNonClientMetrics;
class CPopupText;
class CSubclassWndMap;
class CSubclassWnd;
class CMenuTipManager;

/*****************************************************************************/
/*                           CNonClientMetrics                               */
/*****************************************************************************/

class CNonClientMetrics : public NONCLIENTMETRICS {
public:
  CNonClientMetrics() {
    cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,this,0);
  }
};

/*****************************************************************************/
/*                              CPopupText                                   */
/*****************************************************************************/

class CPopupText : public CWnd {
public: // Member functions
  CPopupText()
    {
      CNonClientMetrics ncm;
      m_font.CreateFontIndirect(&ncm.lfMenuFont);
      m_szMargins = CSize(4,4);
    }
  BOOL Create(CPoint pt, CWnd* pParentWnd, UINT nID=0)
    {
      return CreateEx(0,NULL,NULL,WS_POPUP|WS_VISIBLE,
                      CRect(pt,CSize(0,0)),pParentWnd,nID);
    }
  void ShowDelayed(UINT msec)
    { /* Show after a short delay -- no delay means show right away. */
      if (msec==0)
        OnTimer(1);
      else
        SetTimer(1, msec, NULL);
    }
  void Cancel()
    { /* Cancel text; kills the timer and hides the window. */
        if (m_hWnd)
          { KillTimer(1); ShowWindow(SW_HIDE); }
    }
protected:
  virtual void PostNcDestroy() {}
     /* Avoids attempts to delete the window -- typically created on stack. */
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    /* Registers the window class, if necessary. */
  void DrawText(CDC& dc, LPCTSTR lpText, CRect& rc, UINT flags);
  afx_msg LRESULT OnSetText(WPARAM wp, LPARAM lp);
  afx_msg void OnPaint();
  afx_msg void OnTimer(UINT_PTR nIDEvent)
    {
      ShowWindow(SW_SHOWNA);  Invalidate();
      UpdateWindow();         KillTimer(1);
    }
  DECLARE_DYNAMIC(CPopupText);
  DECLARE_MESSAGE_MAP();
public: // Data
  CSize m_szMargins;  // extra space around text: change if you like
  CFont m_font;       // font: change if you like
};

/*****************************************************************************/
/*                            CSubclassWndMap                                */
/*****************************************************************************/

class CSubclassWndMap : private CMapPtrToPtr {
public:
  CSubclassWndMap() {}
  static CSubclassWndMap& GetHookMap();
    // Returns the one and only global hook map
  void Add(HWND hwnd, CSubclassWnd* pSubclassWnd);
    // Adds a hook to the global hook map; i.e., associate hook with window
  void Remove(CSubclassWnd* pSubclassWnd);
    // Removes hook from the list associated with the global map
  void RemoveAll(HWND hwnd);
  CSubclassWnd* Lookup(HWND hwnd);
    // Find first hook associate with window
};

#define	theHookMap	(CSubclassWndMap::GetHookMap())
    // So the hook map isn't instantiated until someone actually requests it

/*****************************************************************************/
/*                             CSubclassWnd                                  */
/*****************************************************************************/

class CSubclassWnd : public CObject {
  /* The message hook map is derived from CMapPtrToPtr, which associates
     a pointer with another pointer. It maps an HWND to a CSubclassWnd, in the
     same way MFC's internal maps map HWND's to CWnd's. The first CSubclassWnd
     attached to a window is stored in the map; all other CSubclassWnd's for
     that window are then chained via CSubclassWnd::m_pNext. */
public:
  CSubclassWnd()
    { m_pNext = NULL;	m_pOldWndProc = NULL;	m_hWnd  = NULL; }
  ~CSubclassWnd()
    {	if (m_hWnd) HookWindow((HWND) NULL); }
  BOOL	HookWindow(HWND  hwnd);
    /* Subclass a window.  This installs a new window proc that directs
       messages to the CSubclassWnd.  pWnd=NULL to remove -- done
       automatically on WM_NCDESTROY) */
  BOOL	HookWindow(CWnd* pWnd)	{ return HookWindow(pWnd->GetSafeHwnd()); }
  void	Unhook()   { HookWindow((HWND)NULL); }
  BOOL	IsHooked() { return m_hWnd!=NULL; }
  virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp)
    {	ASSERT(m_pOldWndProc);
      if (m_pNext) return m_pNext->WindowProc(msg,wp,lp);
      return ::CallWindowProc(m_pOldWndProc,m_hWnd,msg,wp,lp);
    }
    /* Window proc-like virtual function which specific CSubclassWnds will
       override to do stuff. Default passes the message to the next hook; 
       the last hook passes the message to the original window.  You MUST call
       this at the end of your WindowProc if you want the real window to get
       the message. This is just like CWnd::WindowProc, except that
       a CSubclassWnd is not a window. */
  LRESULT Default()
    {	// MFC stores current MSG in thread state
      MSG& curMsg = AfxGetThreadState()->m_lastSentMsg;
      return CSubclassWnd::WindowProc(curMsg.message, // Avoid inf. recursion
                                      curMsg.wParam,curMsg.lParam);
    }
    /* Like base class WindowProc, but with no args, so individual
       message handlers can do the default thing. Like `CWnd::Default'.
       Call this at the end of handler functions. */
#ifdef _DEBUG
  virtual void AssertValid() const
    { CObject::AssertValid();
      ASSERT(m_hWnd==NULL || ::IsWindow(m_hWnd));
      if (m_hWnd)
        for (CSubclassWnd* p=theHookMap.Lookup(m_hWnd); p; p=p->m_pNext)
          {
            if (p==this) break;
            ASSERT(p); // should have found it!
          }
    }
  virtual void Dump(CDumpContext& dc) const {	CObject::Dump(dc); }
#endif
protected:
  friend LRESULT CALLBACK HookWndProc(HWND, UINT, WPARAM, LPARAM);
  friend class CSubclassWndMap;
  HWND				m_hWnd;				// the window hooked
  WNDPROC			m_pOldWndProc;		// ..and original window proc
  CSubclassWnd*	m_pNext;				// next in chain of hooks for this window

  DECLARE_DYNAMIC(CSubclassWnd);
};

/*****************************************************************************/
/*                            CMenuTipManager                                */
/*****************************************************************************/

class CMenuTipManager : public CSubclassWnd {
public: // Member functions
  CMenuTipManager() : m_iDelay(1000), m_bSticky(FALSE) { }
  ~CMenuTipManager() { }
public: // Main installation function
  void Install(CWnd* pWnd) { HookWindow(pWnd); }
public: // Handlers to call from the main window
  void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hMenu);
  void OnEnterIdle(UINT nWhy, HWND hwndWho);
    /* Need to handle WM_ENTERIDLE to cancel the tip if the user moved
       the mouse off the popup menu. For main menus, Windows will send a
       WM_MENUSELECT message for the parent menu when this happens, but for
       context menus there's no other way to know the user moved the mouse
       off the menu. */
private: // Helper functions
  static CWnd* GetRunningMenuWnd();
  static void  GetRunningMenuRect(CRect& rcMenu);
  CRect GetMenuTipRect(HMENU hmenu, UINT nID);
  static CString GetResCommandPrompt(UINT nID);
    // Override CSubclassWnd::WindowProc to hook messages on behalf of
  virtual CString OnGetCommandPrompt(UINT nID)
    { return GetResCommandPrompt(nID); }
  virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);
public: // Public data
  int m_iDelay;				// tooltip delay: you can change
protected: // Private data
  CPopupText m_wndTip;		// home-grown "tooltip"
  BOOL m_bMouseSelect;		// whether menu invoked by mouse
  BOOL m_bSticky;			// after first tip appears, show rest immediately
};

#endif // MENU_TIPS_H
