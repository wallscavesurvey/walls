/*****************************************************************************/
// File: kdws_window.cpp [scope = APPS/WINSHOW]
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
   Implementation of the objects declared in "kdws_window.h"
******************************************************************************/

#include "stdafx.h"
#include <Cderr.h>
#include "kdws_manager.h"
#include "kdws_window.h"
#include "kdws_renderer.h"
#include "kdws_url_dialog.h"
#include "menu_tips.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/* ========================================================================= */
/*                              kdws_image_view                              */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdws_image_view::kdws_image_view                     */
/*****************************************************************************/

kdws_image_view::kdws_image_view()
{
  manager = NULL;
  window = NULL;
  renderer = NULL;
  pixel_scale = last_pixel_scale = 1;
  max_view_size = kdu_coords(10000,10000);
  max_view_size_known = false;
  sizing = false;
  last_size = kdu_coords(0,0);
  scroll_step = kdu_coords(1,1);
  scroll_page = scroll_step;
  scroll_end = kdu_coords(100000,100000);
  screen_size.x = GetSystemMetrics(SM_CXSCREEN);
  screen_size.y = GetSystemMetrics(SM_CYSCREEN);

  focusing = false;
  panning = false;
  have_keyboard_focus = false;

  HINSTANCE hinst = AfxGetInstanceHandle();
  normal_cursor = LoadCursor(NULL,IDC_CROSS);
  focusing_cursor = LoadCursor(NULL,IDC_CROSS);
  panning_cursor = LoadCursor(NULL,IDC_SIZEALL);
}

/*****************************************************************************/
/*                     kdws_image_view::set_max_view_size                    */
/*****************************************************************************/

void
  kdws_image_view::set_max_view_size(kdu_coords size, int pscale,
                                     bool first_time)
{
  max_view_size = size;
  pixel_scale = pscale;
  max_view_size.x*=pixel_scale;
  max_view_size.y*=pixel_scale;
  max_view_size_known = true;
  if ((last_size.x > 0) && (last_size.y > 0))
    { /* Otherwise, the windows message loop is bound to send us a
         WM_SIZE message some time soon, which will call the
         following function out of the 'OnSize' member function. */
      check_and_report_size(first_time);
    }
  last_pixel_scale = pixel_scale;
}

/*****************************************************************************/
/*                   kdws_image_view::check_and_report_size                  */
/*****************************************************************************/

void
  kdws_image_view::check_and_report_size(bool first_time)
{
  if (sizing)
    return; // We are already in the process of negotiating new dimensions.
  if ((!first_time) && max_view_size_known &&
      ((last_size.x | last_size.y) == 0))
    first_time = true;

  kdu_coords inner_size, outer_size, border_size;
  RECT rect;
  GetClientRect(&rect);
  inner_size.x = rect.right-rect.left;
  inner_size.y = rect.bottom-rect.top;
  kdu_coords target_size = inner_size;
  if (pixel_scale != last_pixel_scale)
    {
      target_size.x = (target_size.x * pixel_scale) / last_pixel_scale;
      target_size.y = (target_size.y * pixel_scale) / last_pixel_scale;
      last_pixel_scale = pixel_scale;
    }
  if (target_size.x > max_view_size.x)
    target_size.x = max_view_size.x;
  if (target_size.y > max_view_size.y)
    target_size.y = max_view_size.y;
  if (first_time)
    { // Can make window larger, if desired
      target_size = max_view_size;
      if (target_size.x > screen_size.x)
        target_size.x = screen_size.x;
      if (target_size.y > screen_size.y)
        target_size.y = screen_size.y;
    }
  target_size.x -= target_size.x % pixel_scale;
  target_size.y -= target_size.y % pixel_scale;

  if (inner_size != target_size)
    { // Need to resize windows -- ugh.
      sizing = true; // Guard against unwanted recursion.
      int iteration = 0;
      do {
          iteration++;
          // First determine the difference between the image view's client
          // area and the frame window's outer boundary.  This total border
          // area may change during resizing, which is the reason for the
          // loop.
          window->GetWindowRect(&rect);
          outer_size.x = rect.right-rect.left;
          outer_size.y = rect.bottom-rect.top;
          
          border_size = outer_size - inner_size;
          if (iteration > 4)
            {
              target_size = outer_size - border_size;
              if (target_size.x < pixel_scale)
                target_size.x = pixel_scale;
              if (target_size.y < pixel_scale)
                target_size.y = pixel_scale;
            }
          outer_size = target_size + border_size; // Assuming no border change
          iteration++;
          if (outer_size.x > screen_size.x)
            outer_size.x = screen_size.x;
          if (outer_size.y > screen_size.y)
            outer_size.y = screen_size.y;
          target_size = outer_size - border_size;
          window->SetWindowPos(NULL,0,0,outer_size.x,outer_size.y,
                               SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
          GetClientRect(&rect);
          inner_size.x = rect.right-rect.left;
          inner_size.y = rect.bottom-rect.top;
        } while ((inner_size.x != target_size.x) ||
                 (inner_size.y != target_size.y));
      target_size = inner_size; // If we could not achieve our original target,
      sizing = false;           // the image view will have to be a bit larger.
    }

  last_size = inner_size;
  if (first_time)
    {
      sizing = true;
      manager->place_window(window,outer_size,true);
      sizing = false;
    }
  if (renderer != NULL)
    {
      kdu_coords tmp = last_size;
      tmp.x = 1 + ((tmp.x-1)/pixel_scale);
      tmp.y = 1 + ((tmp.y-1)/pixel_scale);
      renderer->set_view_size(tmp);
    }
}


/*****************************************************************************/
/*                     kdws_image_view::PreCreateWindow                      */
/*****************************************************************************/

BOOL kdws_image_view::PreCreateWindow(CREATESTRUCT& cs) 
{
  if (!CWnd::PreCreateWindow(cs))
    return FALSE;

  cs.style &= ~WS_BORDER;
  cs.lpszClass = 
    AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
                        ::LoadCursor(NULL, IDC_CROSS),
                        HBRUSH(COLOR_WINDOW+1), NULL);
  return TRUE;
}

/*****************************************************************************/
/*                         kdws_image_view::OnPaint                          */
/*****************************************************************************/

void kdws_image_view::OnPaint() 
{
  if (renderer == NULL)
    return;
  renderer->lock_view();
  PAINTSTRUCT paint_struct;
  kdu_dims region;
  CDC *dc = BeginPaint(&paint_struct);
  region.pos.x = paint_struct.rcPaint.left / pixel_scale;
  region.pos.y = paint_struct.rcPaint.top / pixel_scale;
  region.size.x = (paint_struct.rcPaint.right+pixel_scale-1)/pixel_scale
                - region.pos.x;
  region.size.y = (paint_struct.rcPaint.bottom+pixel_scale-1)/pixel_scale
                - region.pos.y;
  if (renderer != NULL)
    renderer->paint_region(dc,region,false);
  EndPaint(&paint_struct);
  renderer->unlock_view();
}

/*****************************************************************************/
/*                        kdws_image_view::OnHScroll                         */
/*****************************************************************************/

void kdws_image_view::OnHScroll(UINT nSBCode, UINT nPos,
                                CScrollBar* pScrollBar) 
{
  if (renderer == NULL)
    return;
  if (nSBCode == SB_THUMBPOSITION)
    {
      SCROLLINFO sc_info;
      GetScrollInfo(SB_HORZ,&sc_info);
      renderer->set_hscroll_pos(sc_info.nTrackPos);
    }
  else if (nSBCode == SB_LINELEFT)
    renderer->set_hscroll_pos(-scroll_step.x,true);
  else if (nSBCode == SB_LINERIGHT)
    renderer->set_hscroll_pos(scroll_step.x,true);
  else if (nSBCode == SB_PAGELEFT)
    renderer->set_hscroll_pos(-scroll_page.x,true);
  else if (nSBCode == SB_PAGERIGHT)
    renderer->set_hscroll_pos(scroll_page.x,true);
  else if (nSBCode == SB_LEFT)
    renderer->set_hscroll_pos(0);
  else if (nSBCode == SB_RIGHT)
    renderer->set_hscroll_pos(scroll_end.x);
}

/*****************************************************************************/
/*                         kdws_image_view::OnVScroll                        */
/*****************************************************************************/

void kdws_image_view::OnVScroll(UINT nSBCode, UINT nPos,
                                CScrollBar* pScrollBar) 
{
  if (renderer == NULL)
    return;
  if (nSBCode == SB_THUMBPOSITION)
    {
      SCROLLINFO sc_info;
      GetScrollInfo(SB_VERT,&sc_info);
      renderer->set_vscroll_pos(sc_info.nTrackPos);
    }
  else if (nSBCode == SB_LINEUP)
    renderer->set_vscroll_pos(-scroll_step.y,true);
  else if (nSBCode == SB_LINEDOWN)
    renderer->set_vscroll_pos(scroll_step.y,true);
  else if (nSBCode == SB_PAGEUP)
    renderer->set_vscroll_pos(-scroll_page.y,true);
  else if (nSBCode == SB_PAGEDOWN)
    renderer->set_vscroll_pos(scroll_page.y,true);
  else if (nSBCode == SB_TOP)
    renderer->set_vscroll_pos(0);
  else if (nSBCode == SB_BOTTOM)
    renderer->set_vscroll_pos(scroll_end.y);
}

/*****************************************************************************/
/*                       kdws_image_view::OnKeyDown                          */
/*****************************************************************************/

void kdws_image_view::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  if (focusing)
    cancel_focus_drag();
  if (renderer != NULL)
    { // Look out for keys which are not bound to the menu -- currently the
      // only keys of this form are those with obvious scrolling functions and
      // the escape key.
      renderer->cancel_pending_show();
      if (nChar == VK_LEFT)
        renderer->set_hscroll_pos(-scroll_step.x,true);
      else if (nChar == VK_RIGHT)
        renderer->set_hscroll_pos(scroll_step.x,true);
      else if (nChar == VK_UP)
        renderer->set_vscroll_pos(-scroll_step.y,true);
      else if (nChar == VK_DOWN)
        renderer->set_vscroll_pos(scroll_step.y,true);
      else if (nChar == VK_PRIOR)
        renderer->set_vscroll_pos(-scroll_page.y,true);
      else if (nChar == VK_NEXT)
        renderer->set_vscroll_pos(scroll_page.y,true);
      else if (nChar == VK_RETURN)
        renderer->menu_NavImageNext();
      else if (nChar == VK_BACK)
        renderer->menu_NavImagePrev();
      else if (nChar == VK_CONTROL)
        {
          if (panning)
            {
              SetCursor(normal_cursor);
              panning = false;
            }
        }
    }
  CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

/*****************************************************************************/
/*                       kdws_image_view::OnLButtonDown                      */
/*****************************************************************************/

void
  kdws_image_view::OnLButtonDown(UINT nFlags, CPoint point) 
{
  if (renderer != NULL)
    renderer->cancel_pending_show();
  last_mouse_point_scaled.x = (pixel_scale>1)?(point.x/pixel_scale):point.x;
  last_mouse_point_scaled.y = (pixel_scale>1)?(point.y/pixel_scale):point.y;
  if ((nFlags & MK_CONTROL) != 0)
    {
      if (renderer != NULL)
        renderer->edit_metadata_at_point(&last_mouse_point_scaled);
    }
  else if ((nFlags & MK_SHIFT) != 0)
    {
      panning = true;
      mouse_pressed_point = point;
    }
  else
    { // Start focusing operation
      focusing = true;
      focus_start.x = point.x;
      focus_start.y = point.y;
      focus_end = focus_start;
      focus_rect.left = point.x;
      focus_rect.right = point.x+1;
      focus_rect.top = point.y;
      focus_rect.bottom = point.y+1;
      SIZE border; border.cx = border.cy = 2;
      CDC *dc = GetDC();
      dc->DrawDragRect(&focus_rect,border,NULL,border);
      ReleaseDC(dc);
      if (renderer != NULL)
        renderer->suspend_processing(true);
      SetCapture();
      SetCursor(focusing_cursor);
    }
  CWnd::OnLButtonDown(nFlags, point);
}

/*****************************************************************************/
/*                      kdws_image_view::OnRButtonDown                       */
/*****************************************************************************/

void kdws_image_view::OnRButtonDown(UINT nFlags, CPoint point) 
{
  if (renderer != NULL)
    renderer->cancel_pending_show();
  last_mouse_point_scaled.x = (pixel_scale>1)?(point.x/pixel_scale):point.x;
  last_mouse_point_scaled.y = (pixel_scale>1)?(point.y/pixel_scale):point.y;
  if (focusing)
    cancel_focus_drag();
  if (renderer != NULL)
    renderer->edit_metadata_at_point(&last_mouse_point_scaled);
}

/*****************************************************************************/
/*                        kdws_image_view::OnLButtonUp                       */
/*****************************************************************************/

void
  kdws_image_view::OnLButtonUp(UINT nFlags, CPoint point) 
{
  if (renderer != NULL)
    renderer->cancel_pending_show();
  last_mouse_point_scaled.x = (pixel_scale>1)?(point.x/pixel_scale):point.x;
  last_mouse_point_scaled.y = (pixel_scale>1)?(point.y/pixel_scale):point.y;
  if (panning)
    {
      SetCursor(normal_cursor);
      panning = false;
    }
  else if (focusing)
    { // End focusing operation
      SIZE border; border.cx = border.cy = 2;
      SIZE zero_border; zero_border.cx = zero_border.cy = 0;
      CDC *dc = GetDC();
      dc->DrawDragRect(&focus_rect,zero_border,&focus_rect,border);
      ReleaseDC(dc);
      focusing = false;
      if (renderer != NULL)
        {
          kdu_dims new_box;
          new_box.pos = focus_start; new_box.size = focus_end - focus_start;
          if (new_box.size.x < 0)
            { new_box.pos.x = focus_end.x; new_box.size.x = -new_box.size.x; }
          if (new_box.size.y < 0)
            { new_box.pos.y = focus_end.y; new_box.size.y = -new_box.size.y; }
          new_box.pos.x = new_box.pos.x / pixel_scale;
          new_box.pos.y = new_box.pos.y / pixel_scale;
          new_box.size.x = (new_box.size.x+pixel_scale-1)/pixel_scale;
          new_box.size.y = (new_box.size.y+pixel_scale-1)/pixel_scale;
          if (new_box.area() > 15)
            renderer->set_focus_box(new_box);
          else
            renderer->reveal_metadata_at_point(last_mouse_point_scaled);
          renderer->suspend_processing(false);
        }
      ReleaseCapture();
      SetCursor(normal_cursor);
    }
  CWnd::OnLButtonUp(nFlags, point);
}

/*****************************************************************************/
/*                    kdws_image_view::OnLButtonDblClk                       */
/*****************************************************************************/

void
  kdws_image_view::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
  if (renderer != NULL)
    renderer->cancel_pending_show();
  if (focusing)
    cancel_focus_drag();
  last_mouse_point_scaled.x = (pixel_scale>1)?(point.x/pixel_scale):point.x;
  last_mouse_point_scaled.y = (pixel_scale>1)?(point.y/pixel_scale):point.y;
  renderer->reveal_metadata_at_point(last_mouse_point_scaled,true);
}

/*****************************************************************************/
/*                     kdws_image_view::cancel_focus_drag                    */
/*****************************************************************************/

void
  kdws_image_view::cancel_focus_drag()
{
  if (!focusing)
    return;
  SIZE border; border.cx = border.cy = 2;
  SIZE zero_border; zero_border.cx = zero_border.cy = 0;
  CDC *dc = GetDC();
  dc->DrawDragRect(&focus_rect,zero_border,&focus_rect,border);
  ReleaseDC(dc);
  focusing = false;
  renderer->suspend_processing(false);
  ReleaseCapture();
  SetCursor(normal_cursor);
}

/*****************************************************************************/
/*                       kdws_image_view::OnMouseMove                        */
/*****************************************************************************/

void
  kdws_image_view::OnMouseMove(UINT nFlags, CPoint point) 
{
  last_mouse_point_scaled.x = (pixel_scale>1)?(point.x/pixel_scale):point.x;
  last_mouse_point_scaled.y = (pixel_scale>1)?(point.y/pixel_scale):point.y;
  if (nFlags & MK_CONTROL)
    {
      if (focusing)
        cancel_focus_drag();
      if (panning)
        {
          SetCursor(normal_cursor);
          panning = false;
        }
    }
  else if (panning)
    {
      SetCursor(panning_cursor);
      if (renderer != NULL)
        {
          renderer->set_scroll_pos(
               (2*(mouse_pressed_point.x-point.x)+pixel_scale-1)/pixel_scale,
               (2*(mouse_pressed_point.y-point.y)+pixel_scale-1)/pixel_scale,
               true);
          mouse_pressed_point = point;
        }
    }
  else if (focusing)
    {
      focus_end.x = point.x;
      focus_end.y = point.y;
      RECT new_rect;
      if (focus_start.x < focus_end.x)
        { new_rect.left = focus_start.x; new_rect.right = focus_end.x; }
      else
        { new_rect.left = focus_end.x; new_rect.right = focus_start.x; }
      if (focus_start.y < focus_end.y)
        { new_rect.top = focus_start.y; new_rect.bottom = focus_end.y; }
      else
        { new_rect.top = focus_end.y; new_rect.bottom = focus_start.y; }
      new_rect.right++; new_rect.bottom++;
      SIZE border; border.cx = border.cy = 2;
      CDC *dc = GetDC();
      dc->DrawDragRect(&new_rect,border,&focus_rect,border);
      ReleaseDC(dc);
      focus_rect = new_rect;
    }
}

/*****************************************************************************/
/*                     kdws_image_view::OnMouseActivate                      */
/*****************************************************************************/

int
  kdws_image_view::OnMouseActivate(CWnd* pDesktopWnd,
                                   UINT nHitTest, UINT message) 
{
  if (this != GetFocus())
    { // Mouse click is activating window; prevent further mouse messages
      CWnd::OnMouseActivate(pDesktopWnd,nHitTest,message);
      return MA_ACTIVATEANDEAT;
    }
  else
    return CWnd::OnMouseActivate(pDesktopWnd,nHitTest,message);
}

/*****************************************************************************/
/*                     kdws_image_view::OnMouseWheel                         */
/*****************************************************************************/

BOOL
  kdws_image_view::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
  if (renderer == NULL)
    return FALSE;
  if ( nFlags & MK_CONTROL )  // Zoom
    if ( zDelta > 0 )
      renderer->menu_ViewZoomIn();
    else
      renderer->menu_ViewZoomOut();
  else                        // Scroll
    if (nFlags & MK_SHIFT)          // Horizontal
      renderer->set_hscroll_pos(zDelta/10,true);
    else                            // Vertical
      renderer->set_vscroll_pos(zDelta/10,true);
  return TRUE;
}

/*****************************************************************************/
/*                       kdws_image_view::OnSetFocus                         */
/*****************************************************************************/

void kdws_image_view::OnSetFocus(CWnd *pOldWnd)
{
  CWnd::OnSetFocus(pOldWnd);
  this->have_keyboard_focus = true;
}

/*****************************************************************************/
/*                      kdws_image_view::OnKillFocus                         */
/*****************************************************************************/

void
  kdws_image_view::OnKillFocus(CWnd* pNewWnd) 
{
  CWnd ::OnKillFocus(pNewWnd);
  this->have_keyboard_focus = false;
  if (focusing)
    cancel_focus_drag();
  if (panning)
    {
      SetCursor(normal_cursor);
      panning = false;
    }
}

/*****************************************************************************/
/*                    Message Handlers for kdws_image_view                   */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_image_view,CWnd)
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEACTIVATE()
	ON_WM_LBUTTONUP()
  ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYUP()
  ON_WM_MOUSEWHEEL()
  ON_WM_SETFOCUS()
  ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

/* ========================================================================= */
/*                           kdws_frame_window                               */
/* ========================================================================= */

/*****************************************************************************/
/*                  kdws_frame_window::kdws_frame_window                     */
/*****************************************************************************/

kdws_frame_window::kdws_frame_window()
{
  manager = NULL;
  renderer = NULL;
  menu_tip_manager = NULL;
  added_to_manager = false;
  image_view.set_frame_window(this);
  progress_height = 0;
  status_height = 0;
  status_text_height = 0;
  catalog_panel_width = 0;
  for (int p=0; p < 3; p++)
    {
      status_pane_lengths[p] = 0;
      status_pane_caches[p] = NULL;
    }
  status_pane_colour = RGB(255,255,230);
  status_text_colour = RGB(100,60,0);
  status_brush.CreateSolidBrush(status_pane_colour);
  CFont *sys_font = CFont::FromHandle((HFONT) GetStockObject(SYSTEM_FONT));
  LOGFONT font_attributes; sys_font->GetLogFont(&font_attributes);
  font_attributes.lfWeight = FW_NORMAL;
  font_attributes.lfPitchAndFamily = VARIABLE_PITCH;
  font_attributes.lfWidth = 0;
  font_attributes.lfFaceName[0] = '\0';
  status_font.CreateFontIndirect(&font_attributes);
}

/*****************************************************************************/
/*                         kdws_frame_window::init                           */
/*****************************************************************************/

bool kdws_frame_window::init(kdws_manager *manager)
{
  this->manager = manager;
  try {
    if (!this->LoadFrame(IDR_MAINFRAME,
                         WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                         WS_THICKFRAME | WS_MINIMIZEBOX,
                         NULL,NULL))
      { kdu_error e; e <<
          "Failed to create frame window: perhaps the "
          "menu resource could not be found or some resource limit has "
          "been exceeded???";
      }
  }
  catch (int) {
    return false;
  };
  manager->add_window(this);
  added_to_manager = true;
  renderer = new kdws_renderer(this,&image_view,manager);
  image_view.set_manager_and_renderer(manager,renderer);
  renderer->close_file(); // Causes the window title to be updated
  return true;
}

/*****************************************************************************/
/*                  kdws_frame_window::~kdws_frame_window                    */
/*****************************************************************************/

kdws_frame_window::~kdws_frame_window()
{
  if ((manager != NULL) && added_to_manager)
    {
      manager->remove_window(this);
      manager = NULL;
    }
  image_view.set_manager_and_renderer(NULL,NULL);
  if (renderer != NULL)
    { delete renderer; renderer = NULL; }
  status_brush.DeleteObject();
  status_font.DeleteObject();
  if (menu_tip_manager != NULL)
    delete menu_tip_manager;
}

/*****************************************************************************/
/*                     kdws_frame_window::PostNcDestroy                      */
/*****************************************************************************/

void kdws_frame_window::PostNcDestroy() 
{
  if ((manager != NULL) && added_to_manager)
    {
      manager->remove_window(this);
      manager = NULL;
    }
  catalog_view.deactivate();
  CFrameWnd::PostNcDestroy(); // this should destroy the window
}

/*****************************************************************************/
/*                        kdws_frame_window::OnCmdMsg                        */
/*****************************************************************************/

BOOL kdws_frame_window::OnCmdMsg(UINT nID, int nCode, void *pExtra,
                                 AFX_CMDHANDLERINFO *pHandlerInfo)
{
  if ((renderer != NULL) &&
      ((nCode == CN_COMMAND)  || (nCode == CN_UPDATE_COMMAND_UI)) &&
      renderer->OnCmdMsg(nID,nCode,pExtra,pHandlerInfo))
    {
      kdws_frame_window *next_wnd=NULL;
      if (nCode == CN_UPDATE_COMMAND_UI)
        renderer->cancel_pending_show();
      else if ((next_wnd = manager->get_next_action_window(this)) != NULL)
        next_wnd->OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
      return TRUE;
    }
  return CFrameWnd::OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

/*****************************************************************************/
/*                kdws_frame_window::application_can_terminate               */
/*****************************************************************************/

bool kdws_frame_window::application_can_terminate()
{
  if (renderer == NULL)
    return true;
  if ((renderer->editor != NULL) &&
      (interrogate_user("You have an open metadata editor for one or more "
                        "windows that will be closed when this application "
                        "terminates. Are you sure you want to quit the "
                        "application?",
                        "Kdu_show: Will close associated windows",
                        MB_OKCANCEL | MB_ICONWARNING) != IDOK))
    return false;
  if (renderer->have_unsaved_edits &&
      (interrogate_user("You have unsaved edits in one or more windows that "
                        "will be closed when this application terminates.  "
                        "Are you sure you want to quit the application?",
                        "Kdu_show: About to lose edits",
                        MB_OKCANCEL | MB_ICONWARNING) != IDOK))
    return false;
  return true;
}

/*****************************************************************************/
/*                kdws_frame_window::application_terminating                 */
/*****************************************************************************/

void kdws_frame_window::application_terminating()
{
  catalog_view.deactivate();
  if (renderer != NULL)
    renderer->perform_essential_cleanup_steps();
}

/*****************************************************************************/
/*                    kdws_frame_window::PreCreateWindow                     */
/*****************************************************************************/

BOOL kdws_frame_window::PreCreateWindow(CREATESTRUCT &cs)
{
  if( !CFrameWnd::PreCreateWindow(cs) )
    return FALSE;
  cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
  cs.lpszClass = AfxRegisterWndClass(0);
  return TRUE;
}


/*****************************************************************************/
/*                    kdws_frame_window::OnCreateClient                      */
/*****************************************************************************/

BOOL kdws_frame_window::OnCreateClient(CREATESTRUCT *cs, CCreateContext *ctxt)
{
  CDC *dc = this->GetDC();
  CFont *existing_font = dc->SelectObject(&status_font);
  TEXTMETRIC font_metrics;
  dc->GetTextMetrics(&font_metrics);
  status_text_height = font_metrics.tmHeight + 4;
  dc->SelectObject(existing_font);
  this->ReleaseDC(dc);

  UINT wnd_id = AFX_IDW_PANE_FIRST;
  if (!image_view.Create(NULL, NULL,
                         AFX_WS_DEFAULT_VIEW | WS_HSCROLL | WS_VSCROLL,
                         CRect(0,0,0,0),this,wnd_id++,NULL))
    return FALSE;
  if(!progress_indicator.Create(AFX_WS_DEFAULT_VIEW | PBS_SMOOTH,
                          CRect(0,0,0,0),this,wnd_id++))
    return FALSE;
  for (int p=0; p < 3; p++)
    {
      DWORD style = SS_SUNKEN | SS_LEFT | SS_ENDELLIPSIS;
      if (!status_panes[p].Create(NULL,style,CRect(0,0,0,0),this,wnd_id++))
        return FALSE;
      status_panes[p].SetFont(&status_font,FALSE);
      status_panes[p].SetWindowText(_T(""));
    }
  catalog_view.set_tools_font_and_height(&status_font,status_text_height-4);
  if (!catalog_view.Create(NULL,SS_SUNKEN | SS_CENTER | SS_ENDELLIPSIS |
                           SS_OWNERDRAW, // We draw a focus-dependent border
                           CRect(0,0,0,0),this,wnd_id++))
    return FALSE;

  progress_height = 0;
  status_height = status_text_height;
  catalog_panel_width = 0;

  assert(manager != NULL);

  kdu_coords initial_size = image_view.get_screen_size();
  initial_size.x = (initial_size.x * 3) / 10;
  initial_size.y = (initial_size.y * 3) / 10;
  this->SetWindowPos(NULL,0,0,initial_size.x,initial_size.y,
                     SWP_SHOWWINDOW | SWP_NOZORDER);
  ShowWindow(SW_SHOW);
  this->DragAcceptFiles(TRUE);
  return TRUE;
}

/*****************************************************************************/
/*                    kdws_frame_window::interrogate_user                    */
/*****************************************************************************/

int kdws_frame_window::interrogate_user(const char *text, const char *caption,
                                        UINT flags)
{
  kdws_string text_string(text);
  kdws_string caption_string(caption);
  return this->MessageBox(text_string,caption_string,flags);
}

/*****************************************************************************/
/*                       kdws_frame_window::open_file                        */
/*****************************************************************************/

void kdws_frame_window::open_file(const char *filename)
{
  if (renderer == NULL)
    return;
  if (renderer->have_unsaved_edits &&
      (interrogate_user("You have edited the existing file but not saved "
                        "these edits ... perhaps you saved the file in a "
                        "format which cannot hold all the metadata ("
                        "use JPX).  Do you still want to close the existing "
                        "file to open a new one?",
                        "Kdu_show: Close without saving?",
                        MB_OKCANCEL | MB_ICONWARNING) != IDOK))
      return;
  if (manager->check_open_file_replaced(filename) &&
      (interrogate_user("A file you have elected to open is out of date, "
                        "meaning that another open window in the application "
                        "has saved over the file but not yet released its "
                        "access, so that the saved copy can replace the "
                        "original.  Do you still wish to open the old copy?",
                        "Kdu_show: Open old copy?",
                        MB_OKCANCEL | MB_ICONWARNING) != IDOK))
    return;
  renderer->open_file(filename);
}

/*****************************************************************************/
/*                       kdws_frame_window::open_url                        */
/*****************************************************************************/

void kdws_frame_window::open_url(const char *url)
{
  if (renderer == NULL)
    return;
  if (renderer->have_unsaved_edits &&
      (interrogate_user("You have edited the existing file but not saved "
                        "these edits ... perhaps you saved the file in a "
                        "format which cannot hold all the metadata ("
                        "use JPX).  Do you still want to close the existing "
                        "file to open a new one?",
                        "Kdu_show: Close without saving?",
                        MB_OKCANCEL | MB_ICONWARNING) != IDOK))
      return;
  renderer->open_file(url);
}

/*****************************************************************************/
/*                  kdws_frame_window::set_status_strings                    */
/*****************************************************************************/

void kdws_frame_window::set_status_strings(const char **three_strings)
{
  int p; // Panel index
  
  // Start by finding the status panes which have changed, and allocating
  // space to cache the new status strings.
  int total_chars = 0; // Including NULL chars.
  bool pane_updated[3] = {false,false,false};
  for (p=0; p < 3; p++)
    {
      const char *string = three_strings[p];
      if (string == NULL)
        string = "";
      int len = (int) strlen(string)+1;
      pane_updated[p] = ((len != status_pane_lengths[p]) ||
                         (strcmp(string,status_pane_caches[p]) != 0));
      if ((total_chars + len) <= 256)
        { // Can cache this string
          status_pane_lengths[p] = len;
          status_pane_caches[p] = status_pane_cache_buf + total_chars;
          total_chars += len;
        }
      else
        { // Cannot cache this string
          status_pane_lengths[p] = 0;
          status_pane_caches[p] = NULL;
        }          
    }
  
  // Now update any status panes whose text has changed and also cache the
  // new text if we can, for next time.  Caching allows us to avoid updating
  // too many status panes, which can speed things up in video applications,
  // for example.
  for (p=0; p < 3; p++)
    {
      if (!pane_updated[p])
        continue;
      const char *string = three_strings[p];
      if (string == NULL)
        string = "";
      if (status_pane_caches[p] != NULL)
        strcpy(status_pane_caches[p],string);
      CString cstring(string);
      status_panes[p].SetWindowText(cstring);
    }
}

/*****************************************************************************/
/*                   kdws_frame_window::set_progress_bar                     */
/*****************************************************************************/

void kdws_frame_window::set_progress_bar(double value)
{
  if (progress_height == 0)
    {
      progress_height = status_text_height >> 1;
      status_height = status_text_height + progress_height;
      regenerate_layout();
    }
  if (value < 0.0)
    value = 0.0;
  else if (value > 100.0)
    value = 100.0;
  progress_indicator.SetPos((int)(value+0.5));
}

/*****************************************************************************/
/*                 kdws_frame_window::remove_progress_bar                    */
/*****************************************************************************/

void kdws_frame_window::remove_progress_bar()
{
  progress_height = 0;
  status_height = status_text_height;
  regenerate_layout();
}

/*****************************************************************************/
/*               kdws_frame_window::create_metadata_catalog                  */
/*****************************************************************************/

void kdws_frame_window::create_metadata_catalog()
{
  if (renderer == NULL)
    return;
  if (catalog_view.is_active())
    { // Catalog already exists
      assert(catalog_panel_width > 0);
      renderer->set_metadata_catalog_source(&catalog_view);
      return;
    }
  catalog_panel_width = 280;
  RECT rect;  GetWindowRect(&rect);
  rect.right += catalog_panel_width;
  int screen_width = image_view.get_screen_size().y;
  if (rect.right > screen_width)
    { // At least some of the catalog will be invisible
      int width = rect.right-rect.left;
      if (width >= screen_width)
        rect.right = rect.left + screen_width;
      int delta = (rect.right-screen_width) >> 1;
      rect.left -= delta;
      rect.right -= delta;
    }
  SetWindowPos(NULL,0,0,rect.right-rect.left,rect.bottom-rect.top,
               SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);
  catalog_view.Invalidate(TRUE);
  catalog_view.activate(renderer);
  renderer->set_metadata_catalog_source(&catalog_view);
}

/*****************************************************************************/
/*               kdws_frame_window::remove_metadata_catalog                  */
/*****************************************************************************/

void kdws_frame_window::remove_metadata_catalog()
{
  catalog_view.deactivate();
  renderer->set_metadata_catalog_source(NULL);
  catalog_panel_width = 0;
  regenerate_layout(); // May force the window to shrink
}

/*****************************************************************************/
/*                   kdws_frame_window::regenerate_layout                    */
/*****************************************************************************/

void kdws_frame_window::regenerate_layout() 
{
  // Start by finding the layout rectangles for all constituent window
  RECT frame_rect;
  GetClientRect(&frame_rect);
  assert((frame_rect.top == 0) && (frame_rect.left == 0));

  bool change_catalog_width = false;
  if (catalog_panel_width > 0)
    {
      int old_width = catalog_panel_width;
      catalog_panel_width = 280;
      int frame_width = frame_rect.right-frame_rect.left;
      if (catalog_panel_width > (frame_width>>1))
        catalog_panel_width = frame_width>>1;
      if (catalog_panel_width < 1)
        catalog_panel_width = 1; // Make sure it doesn't go to zero.
      change_catalog_width = (catalog_panel_width != old_width);
    }


  RECT image_rect = frame_rect;
  image_rect.bottom -= status_height;
  if (image_rect.bottom < 1)
    image_rect.bottom = 1;
  image_rect.right -= catalog_panel_width;
  if (image_rect.right < 1)
    image_rect.right = 1;

  RECT status_rect = image_rect;
  status_rect.top = image_rect.bottom;
  status_rect.bottom = frame_rect.bottom;

  RECT catalog_rect = frame_rect;
  catalog_rect.left = image_rect.right;

  image_view.SetWindowPos(NULL,image_rect.left,image_rect.top,
                          image_rect.right-image_rect.left,
                          image_rect.bottom-image_rect.top,
                          SWP_NOZORDER|SWP_SHOWWINDOW);

  if (catalog_panel_width > 0)
    {
      catalog_view.SetWindowPos(NULL,catalog_rect.left,catalog_rect.top,
                                catalog_rect.right-catalog_rect.left,
                                catalog_rect.bottom-catalog_rect.top,
                                SWP_NOZORDER|SWP_SHOWWINDOW);
      if (change_catalog_width)
        catalog_view.Invalidate(TRUE);
    }
  else
    catalog_view.SetWindowPos(NULL,0,0,0,0,SWP_NOZORDER|SWP_HIDEWINDOW);

  if (progress_height > 0)
    {
      RECT prog_rect = status_rect;
      prog_rect.bottom = prog_rect.top + progress_height;
      status_rect.top = prog_rect.bottom;
      progress_indicator.SetWindowPos(NULL,prog_rect.left,prog_rect.top,
                                      prog_rect.right-prog_rect.left,
                                      prog_rect.bottom-prog_rect.top,
                                      SWP_NOZORDER|SWP_SHOWWINDOW);
      progress_indicator.Invalidate();
    }
  else
    progress_indicator.SetWindowPos(NULL,0,0,0,0,SWP_NOZORDER|SWP_HIDEWINDOW);

  int p;
  RECT pane_rects[3];
  for (p=0; p < 3; p++)
    pane_rects[p] = status_rect;
  int width = status_rect.right-status_rect.left;
  int outer_width = (width * 3) / 10;
  int inner_width = width - 2*outer_width;
  pane_rects[0].right = pane_rects[1].left = pane_rects[0].left + outer_width;
  pane_rects[1].right = pane_rects[2].left = pane_rects[1].left + inner_width;
  for (p=0; p < 3; p++)
    {
      status_panes[p].SetWindowPos(NULL,pane_rects[p].left,pane_rects[p].top,
                                   pane_rects[p].right-pane_rects[p].left,
                                   pane_rects[p].bottom-pane_rects[p].top,
                                   SWP_NOZORDER|SWP_SHOWWINDOW);
      status_panes[p].Invalidate();
    }

  image_view.check_and_report_size(false);
}

/*****************************************************************************/
/*                  Message Handlers for kdws_frame_window                   */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_frame_window, CFrameWnd)
  ON_WM_CREATE()
	ON_WM_SIZE()
  ON_WM_CTLCOLOR()
	ON_WM_DROPFILES()
  ON_WM_CLOSE()
  ON_WM_ACTIVATE()
  ON_WM_PARENTNOTIFY()
	ON_COMMAND(ID_FILE_OPEN, menu_FileOpen)
  ON_COMMAND(ID_FILE_OPENURL, menu_FileOpenURL)
  ON_COMMAND(ID_WINDOW_DUPLICATE, menu_WindowDuplicate)
  ON_UPDATE_COMMAND_UI(ID_WINDOW_DUPLICATE, can_do_WindowDuplicate)
  ON_COMMAND(ID_WINDOW_NEXT, menu_WindowNext)
  ON_UPDATE_COMMAND_UI(ID_WINDOW_NEXT, can_do_WindowNext)
  ON_COMMAND(ID_WINDOW_CLOSE, OnClose)
END_MESSAGE_MAP()

/*****************************************************************************/
/*                       kdws_frame_window::OnCreate                         */
/*****************************************************************************/

int kdws_frame_window::OnCreate(CREATESTRUCT *create_struct)
{
  int result = CFrameWnd::OnCreate(create_struct);
  if (result != 0)
    return result;
  menu_tip_manager = new CMenuTipManager;
	menu_tip_manager->Install(this); // Comment out to make menu tooltips go away
	return 0;
}

/*****************************************************************************/
/*                      kdws_frame_window::OnCtlColor                        */
/*****************************************************************************/

HBRUSH kdws_frame_window::OnCtlColor(CDC *dc, CWnd *wnd, UINT control_type)
{
  // Call the base implementation first, to get a default brush
  HBRUSH brush = CFrameWnd::OnCtlColor(dc,wnd,control_type);
  for (int p=0; p < 3; p++)
    if (wnd == (status_panes+p))
      {
        dc->SetTextColor(status_text_colour);
        dc->SetBkColor(status_pane_colour);
        return status_brush;
      }
  return brush;
}

/*****************************************************************************/
/*                     kdws_frame_window::OnParentNotify                     */
/*****************************************************************************/

void kdws_frame_window::OnParentNotify(UINT message, LPARAM lParam)
{
  kdu_uint16 low_word = (kdu_uint16) message;
  if ((low_word == WM_LBUTTONDOWN) || (low_word == WM_RBUTTONDOWN) ||
      (low_word == WM_MBUTTONDOWN))
    {
      POINT point;
      point.x = (int)(lParam & 0x0000FFFF);
      point.y = (int)((lParam>>16) & 0x0000FFFF);
      this->ClientToScreen(&point); // Gets mouse location in screen coords
      RECT rect; image_view.GetWindowRect(&rect); // `rect' is in screen coords
      if ((point.x >= rect.left) && (point.x < rect.right) &&
          (point.y >= rect.top) && (point.y < rect.bottom) &&
          GetFocus() != &image_view)
        image_view.SetFocus();
    }
}

/*****************************************************************************/
/*                       kdws_frame_window::OnDropFiles                      */
/*****************************************************************************/

void kdws_frame_window::OnDropFiles(HDROP hDropInfo) 
{
  kdws_string fname(1023);
  DragQueryFile(hDropInfo,0,fname,1023);
  DragFinish(hDropInfo);
  if (renderer != NULL)
    renderer->open_file(fname);
}

/*****************************************************************************/
/*                        kdws_frame_window::OnClose                         */
/*****************************************************************************/

void kdws_frame_window::OnClose()
{
  if (renderer != NULL)
    {
      if ((renderer->editor != NULL) &&
          (interrogate_user("You have an open metadata editor, which will "
                            "be closed if you continue.  Do you still want "
                            "to close the window?",
                            "Kdu_show: Will close associated windows",
                            MB_OKCANCEL | MB_ICONWARNING) != IDOK))
        return;
      if (renderer->have_unsaved_edits &&
          (interrogate_user("You have edited this file but not saved these "
                            "edits ... perhaps you saved the file in a "
                            "format which cannot hold all the metadata ("
                            "use JPX).  Do you still want to close?",
                            "Kdu_show: About to lose edits",
                            MB_OKCANCEL | MB_ICONWARNING) != IDOK))
        return;
    }
  catalog_view.deactivate(); // Do this before destroying windows
  if ((manager != NULL) && added_to_manager)
    { // Doing this here allows the application's main window to be changed
      // so that the application doesn't accidentally get closed down
      // prematurely by the framework.
      manager->remove_window(this);
      manager = NULL;
    }
  this->DestroyWindow();
}

/*****************************************************************************/
/*                       kdws_frame_window::OnActivate                       */
/*****************************************************************************/

void kdws_frame_window::OnActivate(UINT nState, CWnd *pWndOther,
                                   BOOL bMinimized)
{
  CFrameWnd::OnActivate(nState,pWndOther,bMinimized);
  if (renderer != NULL)
    renderer->set_key_window_status(nState != WA_INACTIVE);
  if (nState != WA_INACTIVE)
    image_view.SetFocus();
  if (this->catalog_panel_width > 0)
    catalog_view.reveal_focus();
}

/*****************************************************************************/
/*                      kdws_frame_window::menu_FileOpen                     */
/*****************************************************************************/

void kdws_frame_window::menu_FileOpen()
{
  int filename_buf_len = 4096; // Allows for a lot of files (multi-select)
  kdws_string filename_buf(filename_buf_len);
  kdws_string initial_dir(MAX_PATH);
  kdws_settings *settings = manager->access_persistent_settings();
  OPENFILENAME ofn;
  memset(&ofn,0,sizeof(ofn)); ofn.lStructSize = sizeof(ofn);
  strcpy(initial_dir,settings->get_open_save_dir());
  if (initial_dir.is_empty())
    GetCurrentDirectory(MAX_PATH,initial_dir);

  ofn.hwndOwner = GetSafeHwnd();
  ofn.lpstrFilter =
    _T("JP2-family file ")
    _T("(*.jp2, *.jpx, *.jpf, *.mj2)\0*.jp2;*.jpx;*.jpf;*.mj2\0")
    _T("JP2 files (*.jp2)\0*.jp2\0")
    _T("JPX files (*.jpx, *.jpf)\0*.jpx;*.jpf\0")
    _T("MJ2 files (*.mj2)\0*.mj2\0")
    _T("Uwrapped code-streams (*.j2c, *.j2k)\0*.j2c;*.j2k\0")
    _T("All files (*.*)\0*.*\0\0");
  ofn.nFilterIndex = settings->get_open_idx();
  if ((ofn.nFilterIndex > 5) || (ofn.nFilterIndex < 1))
    ofn.nFilterIndex = 1;
  ofn.lpstrFile = filename_buf;
  ofn.nMaxFile = filename_buf_len;
  ofn.lpstrTitle = _T("Open file (or multiple files)");
  ofn.lpstrInitialDir = initial_dir;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
  if (!GetOpenFileName(&ofn))
    {
      if (CommDlgExtendedError() == FNERR_BUFFERTOOSMALL)
        interrogate_user("Too many files selected -- exceeded allocated "
                         "buffer!",
                         "Kdu_show: file selection error",
                         MB_OK | MB_ICONERROR);
      return;
    }

  // Get the path prefix and update `settings' for next time
  int prefix_len = (int) ofn.nFileOffset;
  initial_dir.clear();
  if (prefix_len > 0)
    { // Get the initial part of the pathname
      if (prefix_len > MAX_PATH)
        {
          interrogate_user("Strange error produced by open file dialog: the "
                           "path prefix of a file to open seems to exceed the "
                           "system-wide maximum path length??",
                           "Kdu_show: file selection error",
                           MB_OK | MB_ICONERROR);
          return;
        }
      initial_dir.clear();
      memcpy((WCHAR *) initial_dir,(const WCHAR *) filename_buf,
             sizeof(WCHAR)*(size_t)prefix_len);
      prefix_len = initial_dir.strlen(); // In case nulls were inserted
      if (prefix_len > 0)
        {
          WCHAR *last_cp = initial_dir; last_cp += prefix_len-1;
          if ((*last_cp == L'/') || (*last_cp == L'\\'))
            *last_cp = L'\0';
          settings->set_open_save_dir(initial_dir);
        }
    }
  settings->set_open_idx(ofn.nFilterIndex);

  // Now finally open the file(s)
  const char *src = ((const char *) filename_buf) + ofn.nFileOffset;
  char *dst = ((char *) filename_buf) + prefix_len;
  if ((*dst == '\0') && (src > dst))
    *(dst++) = '\\';
  bool first_file = true;
  while (*src != '\0')
    {
      // Transfer filename part of the string down to `dst'
      assert(src >= dst);
      char *dp = dst;
      while (*src != '\0')
        *(dp++) = *(src++);
      *dp = '\0';
      kdws_frame_window *target_wnd = this;
      if (!first_file)
        {
          target_wnd = new kdws_frame_window();
          if (!target_wnd->init(manager))
            {
              delete target_wnd;
              break;
            }
        }
      target_wnd->open_file(filename_buf);

      first_file = false;
      if (filename_buf.is_valid_pointer(src+1))
        src++; // So we can pick up extra files, if they exist
    }
}

/*****************************************************************************/
/*                     kdws_frame_window::menu_FileOpenURL                   */
/*****************************************************************************/

void kdws_frame_window::menu_FileOpenURL()
{
  kdws_settings *settings = manager->access_persistent_settings();
  kdws_url_dialog url_dialog(settings,this);
  if (url_dialog.DoModal() == IDOK)
    this->open_url(url_dialog.get_url());
}

/*****************************************************************************/
/*                   kdws_frame_window:menu_WindowDuplicate                  */
/*****************************************************************************/

void kdws_frame_window::menu_WindowDuplicate()
{
  if ((renderer != NULL) && renderer->can_do_WindowDuplicate())
    {
      kdws_frame_window *wnd = new kdws_frame_window();
      if (!wnd->init(manager))
        {
          delete wnd;
          return;
        }
      wnd->renderer->open_as_duplicate(this->renderer);
    }
}

/*****************************************************************************/
/*                      kdws_frame_window:menu_WindowNext                    */
/*****************************************************************************/

void kdws_frame_window::menu_WindowNext()
{
  int idx = manager->get_access_idx(this);
  if (idx < 0) return;
  kdws_frame_window *target_wnd = manager->access_window(idx+1);
  if (target_wnd == NULL)
    target_wnd = manager->access_window(0);
  if (target_wnd == NULL)
    return;
  target_wnd->SetActiveWindow();
}

/*****************************************************************************/
/*                     kdws_frame_window:can_do_WindowNext                   */
/*****************************************************************************/

void kdws_frame_window::can_do_WindowNext(CCmdUI* pCmdUI)
{
  int idx = manager->get_access_idx(this);
  pCmdUI->Enable((idx > 0) ||
                 ((idx == 0) && (manager->access_window(1) != NULL)));
}
