/*****************************************************************************/
// File: kdws_window.h [scope = APPS/WINSHOW]
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
   Class definitions for the window-related objects used by the "kdu_show"
application.
******************************************************************************/
#ifndef KDWS_WINDOW_H
#define KDWS_WINDOW_H

#include "kdu_compressed.h"
#include "kdws_renderer.h"
#include "kdws_catalog.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Defined here:
class kdws_splitter_window;
class kdws_image_view;
class kdws_frame_window;

// Defined elsewhere:
class kdws_renderer;
class kdws_catalog;
class kdws_manager;
class CMenuTipManager;

/*****************************************************************************/
/*                             kdws_image_view                               */
/*****************************************************************************/

class kdws_image_view: public CWnd {
public: // Access methods for use by the frame window
  kdws_image_view();
  void set_manager_and_renderer(kdws_manager *manager, kdws_renderer *renderer)
    { this->manager = manager; this->renderer = renderer; }
  void set_frame_window(kdws_frame_window *frame_window)
    { this->window=frame_window; }
  kdu_coords get_screen_size() { return screen_size; }
  void set_max_view_size(kdu_coords size, int pixel_scale,
                         bool first_time=false);
    /* Called by the manager object whenever the image dimensions
       change for one reason or another.  This is used to prevent the
       window from growing larger than the dimensions of the image (unless
       the image is too tiny).  During periods when no image is loaded, the
       maximum view size should be set sufficiently large that it does not
       restrict window resizing.  The `size' value refers to the number
       of image pixels which are available for display.  Each of these
       occupies `pixel_scale' by `pixel_scale' display pixels.
          If `first_time' is true, the image dimensions have only just been
       determined for the first time.  In this case, the function is allowed
       to grow the current size of the window and it also invokes
       `kdws_manager::place_window'. */
  void set_scroll_metrics(kdu_coords step, kdu_coords page, kdu_coords end)
    { /* This function is called by the manager to set or adjust the
         interpretation of incremental scrolling commands.  In particular,
         these parameters are adjusted whenever the window size changes.
            The `step' coordinates identify the amount to scroll by when an
         arrow key is used, or the scrolling arrows are clicked.
            The `page' coordinates identify the amount to scroll by when the
         page up/down keys are used or the empty scroll bar region is clicked.
            The `end' coordinates identify the maximum scrolling position. */
      scroll_step = step;
      scroll_page = page;
      scroll_end = end;
    }
  void check_and_report_size(bool first_time);
    /* Called when the window size may have changed or may need to be
       adjusted to conform to maximum dimensions established by the
       image -- e.g., called from inside the frame's OnSize message handler,
       or when the image dimensions may have changed. */
  void cancel_focus_drag();
    /* Called if the user cancels an in-progress focus box definition
       operation (mouse click down, drag, mouse click again to finish)
       by pressing the escape key, or any other method which may be
       provided. */
  kdu_coords get_last_mouse_point() { return last_mouse_point_scaled; }
    /* Returns the last known location of the mouse, within the image view,
       relative to the upper left hand corner of the image view, factoring
       out the effect of any prevailing `pixel_scale' value. */
  bool is_first_responder() { return this->have_keyboard_focus; }
    /* Returns true if the image view window currently has the keyboard
       focus.  This may affect the interpretation of various commands. */
// ----------------------------------------------------------------------------
private: // Data
  kdws_manager *manager;
  kdws_renderer *renderer;
  kdws_frame_window *window;
  int pixel_scale; // Display pixels per image pixel
  int last_pixel_scale; // So we can resize display in `check_and_report_size'
  kdu_coords screen_size; // Dimensions of the display
  kdu_coords max_view_size; // Max size for client area (in display pixels)
  bool max_view_size_known; // If the `renderer' has called `set_max_view_size'
  kdu_coords last_size;
  bool sizing; // Flag to prevent recursive calls to resizing functions.
  kdu_coords scroll_step, scroll_page, scroll_end;
  kdu_coords last_mouse_point_scaled; // Last location, divided by pixel scale

  bool focusing; // True only when defining focus box region with mouse clicks
  kdu_coords focus_start; // Coordinates of start point in focusing operation
  kdu_coords focus_end; // Coordinates of end point in focusing operation
  RECT focus_rect; // Most recent focus box during focusing.

  bool panning;
  CPoint mouse_pressed_point;
  bool have_keyboard_focus;

  HCURSOR normal_cursor;
  HCURSOR focusing_cursor;
  HCURSOR panning_cursor;
// ----------------------------------------------------------------------------
protected: // MFC overrides
	void OnPaint();
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
// ----------------------------------------------------------------------------
protected: // Message map functions
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
  afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
  afx_msg void OnKillFocus(CWnd *pNewWnd);
  afx_msg void OnSetFocus(CWnd *pOldWnd);
	DECLARE_MESSAGE_MAP()
};

/*****************************************************************************/
/*                            kdws_frame_window                              */
/*****************************************************************************/

class kdws_frame_window: public CFrameWnd {
// ----------------------------------------------------------------------------
public: // Member functions
	kdws_frame_window();
  virtual ~kdws_frame_window();
  bool init(kdws_manager *app);
    /* Call this right after the constructor, to load the frame window and
       its menu. */
  int interrogate_user(const char *text, const char *caption,
                       UINT flags);
    /* This function essentially invokes `MessageBox' with the supplied flags
       (logical OR of one of MB_ICONERROR, MB_ICONWARNING or MB_ICONQUESTION
       with one of MB_OK, MB_OKCANCEL, MB_YESNO, MB_YESNOCANCEL, etc.),
       with the same return value as MessageBox.  The supplied strings are
       treated as UTF8 and converted to Unicode, as required. */
  void open_file(const char *filename);
    /* Invokes `renderer->open_file' after first performing some validation
       checks. */
  void open_url(const char *url);
    /* Invokes `renderer->open_file' after first performing some validation
       checks. */
  void client_notification()
    { if (renderer != NULL) renderer->client_notification(); }
    /* Passes along messages posted by a JPIP client. */
  void adjust_playclock(double delta)
    { if (renderer != NULL) renderer->adjust_playclock(delta); }
    /* Passes on adjustments delivered to all windows via the manager. */
  void wakeup(double scheduled_time, double current_time)
    { if (renderer != NULL) renderer->wakeup(scheduled_time,current_time); }
    /* Passes on scheduled wakeup events generated by the manager. */
  bool on_idle() { return (renderer != NULL) && renderer->on_idle(); }
    /* Gives the renderer a chance to do idle-time processing. */
  void paint_view_from_presentation_thread()
    { if (renderer != NULL) renderer->paint_view_from_presentation_thread(); }
    /* Called if the entire image view is to be repainted from within the
       presentation thread -- happens only in playmode.  This situation is
       a little bit delicate, because we need to ensure that painting to
       the image view occurs on only one thread at a time. */
  bool application_can_terminate();
    /* Used by the application to see if it should be allowed to quit at
       this point in time -- typically, if there are unsaved edits, the
       implementation will interrogate the user in order to decide. */
  void application_terminating();
    /* Sent by the application before closing all windows. */
  bool is_key_window() { return (this == GetActiveWindow()); }
    /* Returns true if this frame window has key status.  By that we mean
       that the frame window is the active window on the display. */
  void set_status_strings(const char **three_strings);
    /* Takes an array of three character strings, whose contents
       are written to the left, centre and right status bar panels.
       Use NULL or an empty string for a panel you want to leave empty. */
  void set_progress_bar(double value);
    /* If a progress bar does not already exist, one is created, which may
       cause some resizing of the document view.  In any event, the new
       or existing progress bar's progress value is set to `value', which
       is expected to lie in the range 0 to 100. */
  void remove_progress_bar();
    /* Make sure there is no progress bar being displayed.  This function
       also removes the metadata catalog if there is one, for simplicity.
       This should cause no problems, since the progress bar is only removed
       from within `kdws_renderer::close_file' which also removes any
       existing metadata catalog. */
  void create_metadata_catalog();
    /* Called from the main menu or from within `kdws_renderer' to create
       the `catalog_panel' and associated objects to manage a metadata
       catalog.  Also invokes `renderer->set_metadata_catalog_source'. */
  void remove_metadata_catalog();
    /* Called from the main menu or from within `kdws_renderer' to remove
       the `catalog_panel' and associated objects.  Also invokes
       `renderer->set_metadata_catalog_source' with a NULL argument. */
// ----------------------------------------------------------------------------
protected: // MFC overrides
  virtual BOOL OnCreateClient(CREATESTRUCT *cs, CCreateContext *ctxt); 
	virtual BOOL PreCreateWindow(CREATESTRUCT &cs);
  virtual void PostNcDestroy();
  virtual BOOL OnCmdMsg(UINT nID, int nCode, void *pExtra,
                        AFX_CMDHANDLERINFO *pHandlerInfo);
    // Used to route commands to the renderer
// ----------------------------------------------------------------------------
private: // Internal helper functions
  void regenerate_layout();
    /* Call this whenever anything might have changed the layout of views
       and controls within the frame window. */
// ----------------------------------------------------------------------------
private: // Client windows
  kdws_image_view image_view;
  CProgressCtrl progress_indicator; // At top of status bar, if present
  CStatic status_panes[3]; // At bottom of status bar
  CMenuTipManager *menu_tip_manager;
  kdws_catalog catalog_view;
private: // Fonts, colours, brushes
  COLORREF status_pane_colour;
  COLORREF status_text_colour;
  COLORREF catalog_panel_colour;
  CBrush status_brush;
  CFont status_font;
private: // State information
  kdws_manager *manager;
  kdws_renderer *renderer;
  bool added_to_manager; // True once `manager->add_window' called
  int progress_height; // 0 if there is no progress bar
  int status_height; // Height of status bar (panes + progress), in pixels
  int status_text_height; // Height of the font + 4
  int catalog_panel_width; // 0 if no catalog
  int status_pane_lengths[3]; // Num chars currently in each status pane + 1
  char *status_pane_caches[3]; // Caches current string contents of each pane
  char status_pane_cache_buf[256]; // Provides storage for the above array
// ----------------------------------------------------------------------------
public: // Diagnostic functions
#ifdef _DEBUG
  virtual void AssertValid() const
  {
    CFrameWnd::AssertValid();
  }
  virtual void Dump(CDumpContext& dc) const
  {
    CFrameWnd::Dump(dc);
  }
#endif
// ----------------------------------------------------------------------------
protected: // Message map functions
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnClose();
  afx_msg void OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized);
  afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy)
    { if ((cx > 0) && (cy > 0)) regenerate_layout(); }
	afx_msg void OnDropFiles(HDROP hDropInfo);
  afx_msg HBRUSH OnCtlColor(CDC *dc, CWnd *wnd, UINT control_type);
  afx_msg void menu_FileOpen();
  afx_msg void menu_FileOpenURL();
  afx_msg void menu_WindowDuplicate();
  afx_msg void can_do_WindowDuplicate(CCmdUI* pCmdUI)
    { pCmdUI->Enable((renderer != NULL) &&
                     renderer->can_do_WindowDuplicate()); }
  afx_msg void menu_WindowNext();
  afx_msg void can_do_WindowNext(CCmdUI* pCmdUI);
  DECLARE_MESSAGE_MAP()
};

#endif // KDWS_WINDOW_H
