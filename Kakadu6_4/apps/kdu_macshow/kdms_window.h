/*****************************************************************************/
// File: kdms_window.h [scope = APPS/MACSHOW]
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
   Defines the `kdms_window' and `kdms_document_view' Objective-C classes,
which manage most of the Cocoa interaction for an single image/video source.
The rendering aspects are handled by the `kdms_renderer' object.  There is
always a single `kdms_renderer' object for each `kdms_window'.
******************************************************************************/

#import <Cocoa/Cocoa.h>
#import "kdms_renderer.h"
#import "kdms_controller.h"

#define KDMS_FOCUS_MARGIN 2.0F // Added to boundaries of focus region, in
  // screen space, to determine a region which safely covers the
  // focus box and any bounding outline.

/*****************************************************************************/
/*                             kdms_document_view                            */
/*****************************************************************************/

@interface kdms_document_view : NSView {
  kdms_renderer *renderer;
  kdms_window *wnd;
  bool is_first_responder; // Otherwise, the catalog panel may be 1st responder
  bool is_focussing; // If a new focus box is being delineated
  bool is_dragging; // If a mouse-controlled panning operation is in progress
  bool shape_dragging; // If mouse down while editing ROI shape
  bool shape_scribbling; // If mouse down while scribbling an ROI path/boundary
  NSPoint mouse_start; // Point at which focus/editing commenced
  NSPoint mouse_last; // Point last reached dragging mouse during focus/editing
}
- (id)initWithFrame:(NSRect)frame;
- (void)set_renderer:(kdms_renderer *)renderer;
- (void)set_owner:(kdms_window *)owner;
- (BOOL)isOpaque;
- (void)drawRect:(NSRect)rect;
- (bool)is_first_responder;
     /* Allows the `renderer' object to discover whether or not the document
      view is the first responder within its window -- if not, and the catalog
      panel is active, it may be the first responder. */
- (void)start_focusbox_at:(NSPoint)point;
     /* Called when the left mouse button is pressed without shift.
      This initiates the definition of a new focus box. */
- (void)start_viewdrag_at:(NSPoint)point;
     /* Called when the left mouse button is pressed with the shift key.
      This initiates a panning operation which is driven by mouse drag
      events. */
- (void)cancel_focus_drag;
     /* Called whenever a key is pressed.  This cancels any focus box
      delineation or mouse drag based panning operations. */
- (bool)end_focus_drag_at:(NSPoint)point;
     /* Called when the left mouse button is released.  This ends any
      focus box delineation or mouse drag based panning operations.  Returns
      true if a focus or dragging operation was ended by the mouse release
      operation; otherwise, returns false, meaning that the user has
      depressed and released the mouse button without any intervening drag
      operation. */
- (void)mouse_drag_to:(NSPoint)point;
     /* Called when a mouse drag event occurs with the left mouse button
      down.  This affects any current focus box delineation or view
      panning operations initiated by the `start_focusbox_at' or
      `start_viewdrag_at' functions, if they have not been cancelled. */
- (NSRect)rectangleFromMouseStartToPoint:(NSPoint)point;
      /* Returns the rectangle formed between `mouse_start' and `point'. */
- (void)invalidate_mouse_box;
      /* Utility function called from within the various focus/drag functions
       above, in order o invalidate the rectangle defined by `mouse_start'
       through `mouse_last', so that it will be redrawn. */
- (void)handle_single_click:(NSPoint)point;
      /* Called when the left mouse button is released, without having dragged
       the mouse and without holding the shift key down. */
- (void)handle_double_click:(NSPoint)point;
      /* Called when the left mouse button is double-clicked. */
- (void)handle_right_click:(NSPoint)point;
      /* Called when a right-click or ctrl-left-click occurs inside the window.
         As with the other functions above, `point' is a location expressed
         relative to the present document view. */
- (void)handle_shape_editor_click:(NSPoint)point;
- (void)handle_shape_editor_drag:(NSPoint)point;
- (void)handle_shape_editor_move:(NSPoint)point;
      /* Perform selection/deselection/drag/move of a vertex in the shape
         editor, based on whether the mouse `point' (expressed relative to the
         current document view) lies within the nearest vertex painted by the
         shape editor. */
- (void)add_shape_scribble_point:(NSPoint)point;
      /* Add the current mouse point to the scribble points associated with
         the shape editor and redraw, as required. */
- (bool)snap_doc_point:(NSPoint)point to_shape:(kdu_coords *)shape_point;
      /* This function maps `point' from the document view to the
         integer coordinate system of the shape editor, if there is one -- if
         there is no current shape editor, the function immediately returns
         true.  The function additionally implements a policy for "snapping"
         the supplied `point' to nearby anchor points, region boundaries and
         editing guide lines in the shape editor, so as to facilitate
         interactive editing. */
- (void)repaint_shape_region:(kdu_dims)shape_region;
      /* Performs repainting of the region of the document view which is
         affected by `shape_region', as expressed on the codestream
         coordinate system used by an installed `jpx_roi_editor' shape
         editor. */
//-----------------------------------------------------------------------------
// User Event Override Functions
- (BOOL)acceptsFirstResponder;
- (BOOL)becomeFirstResponder;
- (BOOL)resignFirstResponder;
- (void)keyDown:(NSEvent *)key_event;
- (void)rightMouseDown:(NSEvent *)mouse_event;
- (void)mouseDown:(NSEvent *)mouse_event;
- (void)mouseDragged:(NSEvent *)mouse_event;
- (void)mouseUp:(NSEvent *)mouse_event;

@end

/*****************************************************************************/
/*                               kdms_window                                 */
/*****************************************************************************/

@interface kdms_window : NSWindow {
  kdms_window_manager *window_manager;
  kdms_renderer *renderer;
  kdms_document_view *doc_view;
  NSCursor **manager_cursors;
  NSCursor *doc_cursor_normal; // Obtained from the controller
  NSCursor *doc_cursor_dragging; // Obtained from the controller
  NSScrollView *scroll_view;
  NSView *status_bar; // Contains the status panes
  NSProgressIndicator *progress_indicator; // At top of status bar, if present
  NSFont *status_font; // Used for status panes, catalog paste bar & buttons
  NSTextField *status_panes[3]; // At bottom of status bar
  float status_height; // Height of status bar (panes+progress), in pixels
  int status_pane_lengths[3]; // Num chars currently in each status pane + 1
  char *status_pane_caches[3]; // Caches current string contents of each pane
  char status_pane_cache_buf[256]; // Provides storage for the above array
  NSScrollView *catalog_panel; // Created on demand for metadata catalog
  NSBox *catalog_tools; // Created on demand for metadata catalog
  NSTextField *catalog_paste_bar; // Created inside `catalog_tools'
  NSButton *catalog_buttons[3]; // History "back"/"fwd" and "peer" buttons
  float catalog_panel_width; // Positioned on the right of main scroll view
}

//-----------------------------------------------------------------------------
// Initialization/Termination Functions
- (void)initWithManager:(kdms_window_manager *)manager
             andCursors:(NSCursor **)cursors;
    /* Initializes a newly allocated window.  The `cursors' array holds
     pointers to two cursors (normal and dragging), which are shared
     by all windows. */
- (void)dealloc; // Removes us from the `controller's list
- (void)open_file:(NSString *)filename;
- (void)open_url:(NSString *)url;
- (bool)application_can_terminate;
     // Used by the window manager to see if the application should be allowed
     // to quit at this point in time -- typically, if there are unsaved
     // edits, the implementation will interrogate the user in order to
     // decide.
- (void)application_terminating;
- (void)close;

//-----------------------------------------------------------------------------
// Functions used to route information to the renderer
- (bool)on_idle; // Calls `renderer->on_idle'.
- (void)adjust_playclock:(double)delta; // Calls `renderer->adjust_playclock'
- (void)wakeupScheduledFor:(CFAbsoluteTime)scheduled_wakeup_time
        occurredAt:(CFAbsoluteTime)current_time;
- (void)client_notification;
- (void)cancel_pending_show; // See `kdms_renderer::cancel_pending_show'

//-----------------------------------------------------------------------------
// Utility Functions for managed objects
- (void)set_progress_bar:(double)value;
    // If a progress bar does not already exist, one is created, which may
    // cause some resizing of the document view.  In any event, the new
    // or existing progress bar's progress value is set to `value', which
    // is expected to lie in the range 0 to 100.
- (void)remove_progress_bar;
    // Make sure there is no progress bar being displayed.  This function
    // also removes the metadata catalog if there is one, for simplicity.
    // This should cause no problems, since the progress bar is only removed
    // from within `kdms_renderer::close_file' which also removes any
    // existing metadata catalog.
- (void)create_metadata_catalog;
    // Called from the main menu or from within `kdms_renderer' to create
    // the `catalog_panel' and associated objects to manage a metadata
    // catalog.  Also invokes `renderer->set_metadata_catalog_source'.
- (void)remove_metadata_catalog;
    // Called from the main menu or from within `kdms_renderer' to remove
    // the `catalog_panel' and associated objects.  Also invokes
    // `renderer->set_metadata_catalog_source' with a NULL argument.
- (void)set_status_strings:(const char **)three_strings;
    // Takes an array of three character strings, whose contents
    // are written to the left, centre and right status bar panels.
    // Use NULL or an empty string for a panel you want to leave empty
- (float)get_bottom_margin_height; // Return height of status bar
- (float)get_right_margin_width; // Return width of any active catalog panel.
- (void) set_drag_cursor:(BOOL)is_dragging;
    // If yes, a cursor is shown to indicate dragging of the view around.

//-----------------------------------------------------------------------------
// Menu Functions
- (IBAction) menuFileOpen:(NSMenuItem *)sender;
- (IBAction) menuFileOpenUrl:(NSMenuItem *)sender;
- (IBAction) menuFileDisconnectURL:(NSMenuItem *)sender;
- (IBAction) menuFileSave:(NSMenuItem *)sender;
- (IBAction) menuFileSaveAs:(NSMenuItem *)sender;
- (IBAction) menuFileSaveAsLinked:(NSMenuItem *)sender;
- (IBAction) menuFileSaveAsEmbedded:(NSMenuItem *)sender;
- (IBAction) menuFileSaveReload:(NSMenuItem *)sender;
- (IBAction) menuFileProperties:(NSMenuItem *)sender;

- (IBAction) menuViewHflip:(NSMenuItem *)sender;
- (IBAction) menuViewVflip:(NSMenuItem *)sender;
- (IBAction) menuViewRotate:(NSMenuItem *)sender;
- (IBAction) menuViewCounterRotate:(NSMenuItem *)sender;
- (IBAction) menuViewZoomIn:(NSMenuItem *)sender;
- (IBAction) menuViewZoomOut:(NSMenuItem *)sender;
- (IBAction) menuViewZoomInSlightly:(NSMenuItem *)sender;
- (IBAction) menuViewZoomOutSlightly:(NSMenuItem *)sender;
- (IBAction) menuViewWiden:(NSMenuItem *)sender;
- (IBAction) menuViewShrink:(NSMenuItem *)sender;
- (IBAction) menuViewRestore:(NSMenuItem *)sender;
- (IBAction) menuViewRefresh:(NSMenuItem *)sender;
- (IBAction) menuViewLayersLess:(NSMenuItem *)sender;
- (IBAction) menuViewLayersMore:(NSMenuItem *)sender;
- (IBAction) menuViewOptimizeScale:(NSMenuItem *)sender;
- (IBAction) menuViewPixelScaleX1:(NSMenuItem *)sender;
- (IBAction) menuViewPixelScaleX2:(NSMenuItem *)sender;

- (IBAction) menuFocusOff:(NSMenuItem *)sender;
- (IBAction) menuFocusHighlighting:(NSMenuItem *)sender;
- (IBAction) menuFocusWiden:(NSMenuItem *)sender;
- (IBAction) menuFocusShrink:(NSMenuItem *)sender;
- (IBAction) menuFocusLeft:(NSMenuItem *)sender;
- (IBAction) menuFocusRight:(NSMenuItem *)sender;
- (IBAction) menuFocusUp:(NSMenuItem *)sender;
- (IBAction) menuFocusDown:(NSMenuItem *)sender;

- (IBAction) menuModeFast:(NSMenuItem *)sender;
- (IBAction) menuModeFussy:(NSMenuItem *)sender;
- (IBAction) menuModeResilient:(NSMenuItem *)sender;
- (IBAction) menuModeResilientSOP:(NSMenuItem *)sender;
- (IBAction) menuModeSingleThreaded:(NSMenuItem *)sender;
- (IBAction) menuModeMultiThreaded:(NSMenuItem *)sender;
- (IBAction) menuModeMoreThreads:(NSMenuItem *)sender;
- (IBAction) menuModeLessThreads:(NSMenuItem *)sender;
- (IBAction) menuModeRecommendedThreads:(NSMenuItem *)sender;

- (IBAction) menuNavComponent1:(NSMenuItem *)sender;
- (IBAction) menuNavMultiComponent:(NSMenuItem *)sender;
- (IBAction) menuNavComponentNext:(NSMenuItem *)sender;
- (IBAction) menuNavComponentPrev:(NSMenuItem *)sender;
- (IBAction) menuNavImageNext:(NSMenuItem *)sender;
- (IBAction) menuNavImagePrev:(NSMenuItem *)sender;
- (IBAction) menuNavCompositingLayer:(NSMenuItem *)sender;
- (IBAction) menuNavTrackNext:(NSMenuItem *)sender;

- (IBAction) menuMetadataCatalog:(NSMenuItem *)sender;
- (IBAction) menuMetadataSwapFocus:(NSMenuItem *)sender;
- (IBAction) menuMetadataShow:(NSMenuItem *)sender;
- (IBAction) menuMetadataAdd:(NSMenuItem *)sender;
- (IBAction) menuMetadataEdit:(NSMenuItem *)sender;
- (IBAction) menuMetadataDelete:(NSMenuItem *)sender;
- (IBAction) menuMetadataFindOrphanedROI:(NSMenuItem *)sender;
- (IBAction) menuMetadataCopyLabel:(NSMenuItem *)sender;
- (IBAction) menuMetadataCopyLink:(NSMenuItem *)sender;
- (IBAction) menuMetadataCut:(NSMenuItem *)sender;
- (IBAction) menuMetadataPasteNew:(NSMenuItem *)sender;
- (IBAction) menuMetadataDuplicate:(NSMenuItem *)sender;
- (IBAction) menuMetadataUndo:(NSMenuItem *)sender;
- (IBAction) menuMetadataRedo:(NSMenuItem *)sender;
- (IBAction) menuOverlayEnable:(NSMenuItem *)sender;
- (IBAction) menuOverlayFlashing:(NSMenuItem *)sender;
- (IBAction) menuOverlayToggle:(NSMenuItem *)sender;
- (IBAction) menuOverlayRestrict:(NSMenuItem *)sender;
- (IBAction) menuOverlayLighter:(NSMenuItem *)sender;
- (IBAction) menuOverlayHeavier:(NSMenuItem *)sender;
- (IBAction) menuOverlayDoubleSize:(NSMenuItem *)sender;
- (IBAction) menuOverlayHalveSize:(NSMenuItem *)sender;

- (IBAction) menuPlayStartForward:(NSMenuItem *)sender;
- (IBAction) menuPlayStartBackward:(NSMenuItem *)sender;
- (IBAction) menuPlayStop:(NSMenuItem *)sender;
- (IBAction) menuPlayRepeat:(NSMenuItem *)sender;
- (IBAction) menuPlayNative:(NSMenuItem *)sender;
- (IBAction) menuPlayCustom:(NSMenuItem *)sender;
- (IBAction) menuPlayFrameRateUp:(NSMenuItem *)sender;
- (IBAction) menuPlayFrameRateDown:(NSMenuItem *)sender;

- (IBAction) menuStatusToggle:(NSMenuItem *)sender;

- (IBAction) menuWindowDuplicate:(NSMenuItem *)sender;
- (IBAction) menuWindowNext:(NSMenuItem *)sender;
- (bool) can_do_WindowNext;

- (BOOL) validateMenuItem:(NSMenuItem *)menuitem;

//-----------------------------------------------------------------------------
// Overrides
- (void) resignKeyWindow;
- (void) becomeKeyWindow;

//-----------------------------------------------------------------------------
// Notification Functions
- (void) scroll_view_BoundsDidChange:(NSNotification *)notification;

@end

/*****************************************************************************/
/*                            External Functions                             */
/*****************************************************************************/

extern int interrogate_user(const char *message, const char *option_0,
                            const char *option_1="", const char *option_2="",
                            const char *option_3="");
  /* Presents a warning message to the user with up to three options, returning
   the index of the selected option.  If `option_1' is an empty string, only
   the first option is presented to the user.  If `option_2' is emty, at most
   two options are presented to the user; and so on. */
