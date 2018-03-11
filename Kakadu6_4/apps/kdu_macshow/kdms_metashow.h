/*****************************************************************************/
// File: kdms_metashow.h [scope = APPS/MACSHOW]
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
   Defines the `kdms_metashow' class, which provides a window for dynamically
 displaying the metadata structure from JP2-family files.  The display is
 dynamic in the sense that it can be updated as more information becomes
 available in the data source (relevant only if it is a client cache).
 *****************************************************************************/
#import <Cocoa/Cocoa.h>
#import "kdms_controller.h"
#include "kdu_compressed.h"
#include "jpx.h"

class kdms_renderer;

// Defined here
struct kdms_metanode;
@class kdms_metashow_delegate;
@class kdms_metashow;

/******************************************************************************/
/*                               kdms_metanode                                */
/******************************************************************************/

struct kdms_metanode {
public: // Member functions
  kdms_metanode(kdms_metashow_delegate *owner, kdms_metanode *parent,
                NSBrowser *browser);
  ~kdms_metanode();
  bool update_structure(jp2_family_src *jp2_src, jpx_meta_manager meta_manager);
    /* Returns true if any changes are made to the metadata tree.  If so,
       the NSBrowser's validateVisibleColumns function should be called to
       reflect any changes in the browser window. */
  void create_browser_view();
    /* Called from within `update_structure', after all members have been
     initialized, other than `name' and related parameters which determine
     how the node is displayed in the browser. */
  bool is_visible_in_browser();
    /* Returns true if the present node is the node's parent represents
     a visible browser column, whose contents are its children. */
  void write_name_to_browser_cell(NSBrowserCell *cell);
    /* Writes the node's name and other attributes into the supplied `cell'. */
public: // Links
  kdms_metashow_delegate *owner;
  kdms_metanode *parent;
  NSBrowser *browser;
public: // Data derived from the JP2 family source
  bool has_no_box; // First node in parent's child list may be informative only
  bool is_deleted; // If node has been deleted in JPX metadata editor
  bool is_changed; // If node has been altered in JPX metadata editor
  bool ancestor_changed; // If any ancestor was altered in JPX metadata editor
  kdu_uint32 box_type; // 0 if this is the root node
  jp2_locator locator; // Use this to open a box at any point in the future
  kdu_long contents_bin;
  int box_header_length; // Use this to invoke `jpx_meta_manager::locate_node'
  jpx_metanode jpx_node; // Empty until we can resolve it (if ever)
  int codestream_idx; // -ve if does not refer to a code-stream
  int compositing_layer_idx; // -ve if does not refer to a compositing layer
  bool child_list_complete; // See below
  bool contents_complete; // See below
  kdms_metanode *children; // Points to a list of child nodes, if any
  kdms_metanode *last_child; // Last element in the `children' list
  kdms_metanode *next; // Points to next node at current level
public: // Data used to associate the node with browser cells and columns
  int browser_column; // Column in which children are displayed
  int browser_row; // Row displaying this node within its parent's column
  kdms_metanode *selected_child; // Non-NULL if a child is known to be selected
  bool browser_column_needs_update; // If `browser_column' needs to be updated
public: // Data which affects the way the node is presented in the browser
  char name[80]; // Name to show in browser column
  bool responds_to_double_click; // If so, display name as a hyperlink
  bool can_display_contents; // If contents can be displayed in text pane
  bool is_display_node; // If this is `kdms_metashow_delegate:display_node'
  bool is_leaf;
  kdu_long leaf_content_bytes; // 0 if not a leaf in the tree
};
/* Notes:
    The `child_list_complete' member is false if we cannot be sure that
 all children have been found yet.  In this case, the list of children
 may need to be grown next time `update' is called.  The fact that
 the list of children is complete does not mean that each child in
 the list is itself complete.
    The `contents_complete' member is true if the node is a super-box
 and its child list is complete, or if the node is a leaf in the tree
 and the corresponding JP2 box's contents are complete. */

/******************************************************************************/
/*                         kdms_metashow_delegate                             */
/******************************************************************************/

// Constants used with `get_text_attributes' below.
#define KDS_TEXT_ORIGINAL 0
#define KDS_TEXT_CHANGED 4
#define KDS_TEXT_DELETED 8
#define KDS_TEXT_ATTRIBUTE_LEAF 1
#define KDS_TEXT_ATTRIBUTE_LINK 2

@interface kdms_metashow_delegate : NSResponder {
  kdms_renderer *owner;
  NSBrowser *browser;
  NSTextView *info_text_panel; // Used to display box contents
  NSBox *info_text_frame; // Used to display a title for the info panel
  NSButton *edit_button;
  jp2_family_src *src;
  int num_codestreams;
  int num_jpch; // Number of codestream header boxes found
  int num_jplh; // Number of compositing layer header boxes found
  kdms_metanode *tree; // Local skeleton of meta-data tree
  NSDictionary *informative_text_attributes;
  NSDictionary *text_attributes[2*2*3]; // Indexed by leaf+link*2+edits*4, where
          // leaf = 0 for branch and 1 for leaf boxes
          // link = 0 for no link and 1 for a clickable link
          // edits = 0 for normal, 1 for changed and 2 for deleted.
  kdms_metanode *display_node; // Node whose contents are being displayed in
                               // the `info_text_panel' or else NULL.
  int display_node_content_bytes; // Nmber of bytes currently displayed;
                                  // becomes -1 if we have converted all the
                                  // unicode characters we can display.
}
-(void)initWithOwner:(kdms_renderer *)renderer;
-(void)setBrowser:(NSBrowser *)browser
        infoPanel:(NSTextView *)info_panel
        infoFrame:(NSBox *)info_frame
       editButton:(NSButton *)button;
-(void)reset; // Deletes the metadata tree and all associated information
-(void)dealloc;
-(void)activateWithSource:(jp2_family_src *)source;
      // After changing the source, call the `update' function below immediately
-(void)update_metadata;
-(void)select_matching_metanode:(jpx_metanode)jpx_node;
-(int)found_new_codestream_box; // Returns idx of the new codestream
-(int)found_new_codestream_header_box; // Returns idx of new codestream header
-(int)found_new_compositing_layer_box; // Returns idx of new compositing layer

-(NSDictionary *)get_informative_text_attributes; // To display info in cell
-(NSDictionary *)get_text_attributes:(int)style;
    // `style' is one of KDS_TEXT_ORIGINAL, KDS_TEXT_CHANGED, KDS_TEXT_DELETED
    // together with zero, one or both of the following flags:
    // KDS_TEXT_ATTRIBUTE_LEAF and KDS_TEXT_ATTRIBUTE_LINK.

-(void)update_info_text; // Updates `info_text_panel' and perhaps the title in
  // `info_text_frame', based on the `display_node' member, which may be
  // getting displayed for the first time, or updated with more content bytes
  // available.

- (void)configure_edit_button_for_node:(kdms_metanode *)node;
  // Adjusts the enabled state of the edit button, based on whether or not
  // `node' can be edited.  `node' can be NULL, in which case editing is of
  // course disabled.

-(void)browser:(NSBrowser *)sender willDisplayCell:(id)cell
         atRow:(NSInteger)row column:(NSInteger)column;
-(NSInteger)browser:(NSBrowser *)sender numberOfRowsInColumn:(NSInteger)column;
-(BOOL)browser:(NSBrowser *)sender isColumnValid:(NSInteger)column;
-(IBAction)user_double_click:(NSBrowser *)sender;
-(IBAction)user_click:(NSBrowser *)sender;
-(IBAction)edit_button_click:(NSButton *)sender;

@end // kdms_metashow_delegate

/******************************************************************************/
/*                              kdms_metashow                                 */
/******************************************************************************/

@interface kdms_metashow : NSWindow {
  kdms_renderer *owner;
  kdms_metashow_delegate *delegate;
  NSBrowser *browser;
  NSTextView *text_panel;
  NSButton *edit_button;
  NSBox *text_panel_frame;
  int identifier;
  const char *filename;
}

- (void) initWithOwner:(kdms_renderer *)renderer
              location:(NSPoint)preferred_location
         andIdentifier:(int)identifier;
    /* The `renderer' reference is provided so that user actions in the
     metashow window can affect what information is currently being
     rendered (e.g., which codestream or compositing layer is displayed).
       The `location' supplied here is the preferred location for the top
     left corner of the metashow window.
       The integer `identifier' supplied here is used to construct a
     title for the metashow window which associates it with the owner's
     window, in which imagery is being rendered.  This is the main window's
     numeric identifier, as assigned by the `kdms_window_manager'
     object and used to construct the title for the main window. */
- (void) dealloc;
- (void) activateWithSource:(jp2_family_src *)source
                   andFileName:(const char *)name;
   /* The file name supplied here is used to construct a title for the
    metashow window, along with the identifier supplied by the previous
    function.  The file name string is not copied, so the caller must be
    sure that the string's original memory buffer is preserved, so long
    as the metashow window is activated, in case the need arises to modify
    the title in any way. */
- (void) update_metadata;
- (void) deactivate;
- (void) select_matching_metanode:(jpx_metanode)jpx_node;
   /* This function attempts to locate `node' in the metadata tree and select
    that node in the browser. */
- (void) close; // Resets the `owner->metashow' to nil.
@end
