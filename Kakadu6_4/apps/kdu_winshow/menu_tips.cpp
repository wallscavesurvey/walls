/*****************************************************************************/
// File: menu_tips.cpp [scope = APPS/WINSHOW]
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
#include "stdafx.h"
#include "menu_tips.h"
#include "kdws_manager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/* ========================================================================= */
/*                                CPopupText                                 */
/* ========================================================================= */

IMPLEMENT_DYNAMIC(CPopupText,CWnd)
BEGIN_MESSAGE_MAP(CPopupText,CWnd)
  ON_WM_NCHITTEST()
  ON_WM_PAINT()
  ON_MESSAGE(WM_SETTEXT, OnSetText)
  ON_WM_TIMER()
END_MESSAGE_MAP()

/*****************************************************************************/
/*                           CPopupText::OnSetText                           */
/*****************************************************************************/

LRESULT CPopupText::OnSetText(WPARAM wp, LPARAM lp)
{
  CRect rc;
  GetWindowRect(&rc);
  CClientDC dc(this);
  CString string((LPCTSTR) lp);
  DrawText(dc,string,rc,DT_CALCRECT);
  if (rc.right > (rc.left+200))
    { // Resize the rectangle in multi-line mode.  To do this, we have to
      // add newline characters and then resize
      string += "\ntest";
      rc.right = rc.left + 200;
      DrawText(dc,string,rc,DT_CALCRECT | DT_WORDBREAK);
      int height_save = rc.Height();
      string += "\ntest2"; // Figure out the size of the extra newline
      DrawText(dc,string,rc,DT_CALCRECT | DT_WORDBREAK);
      int new_height = rc.Height();
      rc.bottom = rc.top + height_save - (new_height-height_save);
    }
  rc.InflateRect(m_szMargins);
  SetWindowPos(NULL, 0, 0, rc.Width(), rc.Height(),
    SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
  return Default();
}

/*****************************************************************************/
/*                            CPopupText::OnPaint                            */
/*****************************************************************************/

void CPopupText::OnPaint()
{
  CRect rc;
  GetClientRect(&rc);
  CString s;
  GetWindowText(s);
  CPaintDC dc(this);
  DrawText(dc,s,rc,DT_WORDBREAK);
}

/*****************************************************************************/
/*                           CPopupText::DrawText                            */
/*****************************************************************************/

void CPopupText::DrawText(CDC& dc, LPCTSTR lpText, CRect& rc, UINT flags)
{
  CBrush b(GetSysColor(COLOR_INFOBK)); // use tooltip bg color
  dc.FillRect(&rc, &b);
  dc.SetBkMode(TRANSPARENT);
  dc.SetTextColor(GetSysColor(COLOR_INFOTEXT)); // tooltip text color
  CFont* pOldFont = dc.SelectObject(&m_font);
  dc.DrawText(lpText, &rc, flags);
  dc.SelectObject(pOldFont);
}

/*****************************************************************************/
/*                       CPopupText::PreCreateWindow                         */
/*****************************************************************************/

BOOL CPopupText::PreCreateWindow(CREATESTRUCT& cs) 
{
  static CString sClassName;
  if (sClassName.IsEmpty())
    sClassName = AfxRegisterWndClass(0);
  cs.lpszClass = sClassName;
  cs.style = WS_POPUP|WS_BORDER;
  cs.dwExStyle |= WS_EX_TOOLWINDOW;
  return CWnd::PreCreateWindow(cs);
}

/* ========================================================================= */
/*                              CSubclassWndMap                              */
/* ========================================================================= */

/*****************************************************************************/
/* CALLBACK                       HookWndProc                                */
/*****************************************************************************/
  
LRESULT CALLBACK HookWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
  /* Subclassed window proc for message hooks. Replaces AfxWndProc (or whatever
     else was there before). */
{
#ifdef _USRDLL
  // If this is a DLL, need to set up MFC state
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif

  // Set up MFC message state just in case anyone wants it
  // This is just like AfxCallWindowProc, but we can't use that because
  // a CSubclassWnd is not a CWnd.
  //
  MSG& curMsg = AfxGetThreadState()->m_lastSentMsg;
  MSG  oldMsg = curMsg;   // save for nesting
  curMsg.hwnd		= hwnd;
  curMsg.message = msg;
  curMsg.wParam  = wp;
  curMsg.lParam  = lp;

  // Get hook object for this window. Get from hook map
  CSubclassWnd* pSubclassWnd = theHookMap.Lookup(hwnd);
  ASSERT(pSubclassWnd);

  LRESULT lr;
  if (msg==WM_NCDESTROY) {
    // Window is being destroyed: unhook all hooks (for this window)
    // and pass msg to orginal window proc
    //
    WNDPROC wndproc = pSubclassWnd->m_pOldWndProc;
    theHookMap.RemoveAll(hwnd);
    lr = ::CallWindowProc(wndproc, hwnd, msg, wp, lp);

  } else {
    // pass to msg hook
    lr = pSubclassWnd->WindowProc(msg, wp, lp);
  }
  curMsg = oldMsg;			// pop state
  return lr;
}

/*****************************************************************************/
/* STATIC                CSubclassWndMap::GetHookMap                         */
/*****************************************************************************/

CSubclassWndMap& CSubclassWndMap::GetHookMap()
{
  // By creating theMap here, C++ doesn't instantiate it until/unless
  // it's ever used! This is a good trick to use in C++, to
  // instantiate/initialize a static object the first time it's used.
  //
  static CSubclassWndMap theMap;
  return theMap;
}

/*****************************************************************************/
/*                          CSubclassWndMap::Add                             */
/*****************************************************************************/

#ifdef _WIN64
#  define KD_WNDPROC_TO_LONG_PTR(__p) ((LONG_PTR) __p)
#  define KD_LONG_PTR_TO_WNDPROC(__p) ((WNDPROC) __p)
#else // WIN32
#  define KD_WNDPROC_TO_LONG_PTR(__p) ((LONG)((__int64) __p))
#  define KD_LONG_PTR_TO_WNDPROC(__p) ((WNDPROC)((__int64) __p))
#endif

void CSubclassWndMap::Add(HWND hwnd, CSubclassWnd* pSubclassWnd)
{
  ASSERT(hwnd && ::IsWindow(hwnd));

  // Add to front of list
  pSubclassWnd->m_pNext = Lookup(hwnd);
  SetAt(hwnd, pSubclassWnd);
  
  if (pSubclassWnd->m_pNext==NULL) {
    // If this is the first hook added, subclass the window
    pSubclassWnd->m_pOldWndProc = KD_LONG_PTR_TO_WNDPROC(
      SetWindowLongPtr(hwnd, GWLP_WNDPROC,
                       KD_WNDPROC_TO_LONG_PTR(HookWndProc)));

  } else {
    // just copy wndproc from next hook
    pSubclassWnd->m_pOldWndProc = pSubclassWnd->m_pNext->m_pOldWndProc;
  }
  ASSERT(pSubclassWnd->m_pOldWndProc);
}

/*****************************************************************************/
/*                         CSubclassWndMap::Remove                           */
/*****************************************************************************/

void CSubclassWndMap::Remove(CSubclassWnd* pUnHook)
{
  HWND hwnd = pUnHook->m_hWnd;
  ASSERT(hwnd && ::IsWindow(hwnd));

  CSubclassWnd* pHook = Lookup(hwnd);
  ASSERT(pHook);
  if (pHook==pUnHook) {
    // hook to remove is the one in the hash table: replace w/next
    if (pHook->m_pNext)
      SetAt(hwnd, pHook->m_pNext);
    else {
      // This is the last hook for this window: restore wnd proc
      RemoveKey(hwnd);
      SetWindowLongPtr(hwnd,GWLP_WNDPROC,
                       KD_WNDPROC_TO_LONG_PTR(pHook->m_pOldWndProc));
    }
  } else {
    // Hook to remove is in the middle: just remove from linked list
    while (pHook->m_pNext!=pUnHook)
      pHook = pHook->m_pNext;
    ASSERT(pHook && pHook->m_pNext==pUnHook);
    pHook->m_pNext = pUnHook->m_pNext;
  }
}

/*****************************************************************************/
/*                       CSubclassWndMap::RemoveAll                          */
/*****************************************************************************/

void CSubclassWndMap::RemoveAll(HWND hwnd)
{
  CSubclassWnd* pSubclassWnd;
  while ((pSubclassWnd = Lookup(hwnd))!=NULL)
    pSubclassWnd->HookWindow((HWND)NULL); // Unhooks it
}

/*****************************************************************************/
/*                         CSubclassWndMap::Lookup                           */
/*****************************************************************************/

CSubclassWnd* CSubclassWndMap::Lookup(HWND hwnd)
{
  CSubclassWnd* pFound = NULL;
  if (!CMapPtrToPtr::Lookup(hwnd, (void*&)pFound))
    return NULL;
  ASSERT_KINDOF(CSubclassWnd, pFound);
  return pFound;
}


/* ========================================================================= */
/*                               CSubclassWnd                                */
/* ========================================================================= */

IMPLEMENT_DYNAMIC(CSubclassWnd, CWnd);

/*****************************************************************************/
/*                         CSubclassWnd::HookWindow                          */
/*****************************************************************************/

BOOL CSubclassWnd::HookWindow(HWND hwnd)
{
  ASSERT_VALID(this);
  if (hwnd)
    { // Hook the window
      ASSERT(m_hWnd==NULL);
      ASSERT(::IsWindow(hwnd));
      theHookMap.Add(hwnd, this);			// Add to map of hooks
    }
  else if (m_hWnd)
    {	// Unhook the window
      theHookMap.Remove(this);				// Remove from map
      m_pOldWndProc = NULL;
    }
  m_hWnd = hwnd;
  return TRUE;
}


/* ========================================================================= */
/*                             CMenuTipManager                               */
/* ========================================================================= */





/*****************************************************************************/
/*                   CMenuTipManager::GetResCommandPrompt                    */
/*****************************************************************************/

CString CMenuTipManager::GetResCommandPrompt(UINT nID)
{
  // load appropriate string
  CString s;
  if (s.LoadString(nID)) {
    LPTSTR lpsz = s.GetBuffer(255);
    // first newline terminates prompt
    lpsz = _tcschr(lpsz, '\n');
    if (lpsz != NULL)
      *lpsz = '\0';
    s.ReleaseBuffer();
  }
  return s;
}

/*****************************************************************************/
/*                   CMenuTipManager::GetResCommandPrompt                    */
/*****************************************************************************/

LRESULT CMenuTipManager::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
  if (msg==WM_MENUSELECT)
    OnMenuSelect(LOWORD(wp), HIWORD(wp), (HMENU)lp);
  else if (msg==WM_ENTERIDLE)
    OnEnterIdle((UINT) wp, (HWND)lp);
  return CSubclassWnd::WindowProc(msg, wp, lp);
}

/*****************************************************************************/
/*                       CMenuTipManager::OnMenuSelect                       */
/*****************************************************************************/

void CMenuTipManager::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hMenu)
{
  if (!m_wndTip.m_hWnd) {
    m_wndTip.Create(CPoint(0,0), CWnd::FromHandle(m_hWnd));
    m_wndTip.m_szMargins = CSize(4,2);
  }

  if ((nFlags & 0xFFFF)==0xFFFF) {
    m_wndTip.Cancel();	// cancel/hide tip
    m_bMouseSelect = FALSE;
    m_bSticky = FALSE;

  } else if (nFlags & MF_POPUP) {
    m_wndTip.Cancel();	// new popup: cancel
    m_bSticky = FALSE;

  } else if (nFlags & MF_SEPARATOR) {
    // separator: hide tip but remember sticky state
    m_bSticky = m_wndTip.IsWindowVisible();
    m_wndTip.Cancel();

  } else if (nItemID && hMenu) {
    // if tips already displayed, keep displayed
    m_bSticky = m_wndTip.IsWindowVisible() || m_bSticky;

    // remember if mouse used to invoke menu
    m_bMouseSelect = (nFlags & MF_MOUSESELECT)!=0;

    // get prompt and display tip (with or without timeout)
    CString prompt = OnGetCommandPrompt(nItemID);
    if (prompt.IsEmpty())
      m_wndTip.Cancel(); // no prompt: cancel tip

    else {
      CRect rc = GetMenuTipRect(hMenu, nItemID);
      m_wndTip.SetWindowPos(&CWnd::wndTopMost, rc.left, rc.top,
        rc.Width(), rc.Height(), SWP_NOACTIVATE);
      m_wndTip.SetWindowText(prompt);
      m_wndTip.ShowDelayed(m_bSticky ? 0 : m_iDelay);
    }
  }
}

/*****************************************************************************/
/*                        CMenuTipManager::OnEnterIdle                       */
/*****************************************************************************/

void CMenuTipManager::OnEnterIdle(UINT nWhy, HWND hwndWho)
{
  if (m_bMouseSelect && nWhy==MSGF_MENU) {
    CPoint pt;
    GetCursorPos(&pt);
    if (hwndWho != ::WindowFromPoint(pt))
      m_wndTip.Cancel();
  }
}

/*****************************************************************************/
/*                      CMenuTipManager::GetMenuTipRect                      */
/*****************************************************************************/

CRect CMenuTipManager::GetMenuTipRect(HMENU hmenu, UINT nID)
{
  CWnd* pWndMenu = GetRunningMenuWnd(); //CWnd::WindowFromPoint(pt);
  ASSERT(pWndMenu);

  CRect rcMenu;
  pWndMenu->GetWindowRect(rcMenu); // whole menu rect

  // add heights of menu items until i reach nID
  int count = ::GetMenuItemCount(hmenu);
  int cy = rcMenu.top + GetSystemMetrics(SM_CYEDGE)+1;
  for (int i=0; i<count; i++) {
    CRect rc;
    ::GetMenuItemRect(m_hWnd, hmenu, i, &rc);
    if (::GetMenuItemID(hmenu,i)==nID) {
      // found menu item: adjust rectangle to right and down
      rc += CPoint(rcMenu.right - rc.left, cy - rc.top);
      return rc;
    }
    cy += rc.Height(); // add height
  }
  return CRect(0,0,0,0);
}

/*****************************************************************************/
/* CALLBACK                       MyEnumProc                                 */
/*****************************************************************************/

static BOOL CALLBACK MyEnumProc(HWND hwnd, LPARAM lParam)
  /* Note that windows are enumerated in top-down Z-order, so the menu
     window should always be the first one found. */
{
  kdws_string class_name(32);
  GetClassName(hwnd,class_name,32);
  if (strcmp(class_name,"#32768")==0) { // special classname for menus
    *((HWND*)lParam) = hwnd; // found it
    return FALSE;
  }
  return TRUE;
}

/*****************************************************************************/
/*                    CMenuTipManager::GetRunningMenuWnd                     */
/*****************************************************************************/

CWnd* CMenuTipManager::GetRunningMenuWnd()
{
  HWND hwnd = NULL;
  EnumWindows(MyEnumProc,(LPARAM)&hwnd);
  return CWnd::FromHandle(hwnd);
}
