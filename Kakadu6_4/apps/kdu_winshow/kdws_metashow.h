/*****************************************************************************/
// File: kdws_metashow.h [scope = APPS/WINSHOW]
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
   Header file for "kdws_metashow.cpp".
******************************************************************************/
#ifndef KDWS_METASHOW_H
#define KDWS_METASHOW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "jp2.h"

// Defined here
struct kdws_metanode;
class kdws_metashow;

// Defined elsewhere
class kdws_renderer;

/*****************************************************************************/
/*                               kdws_metanode                               */
/*****************************************************************************/

struct kdws_metanode {
  public: // Member functions
    kdws_metanode(kdws_metashow *owner, kdws_metanode *parent);
    ~kdws_metanode();
    void update_display();
      /* Called only if `can_display' is true and this node is to be displayed
         in the edit box.  The `owner->display' member must already point
         to the current node. */
    bool update_structure(jp2_input_box *this_box,
                          bool container_complete,
                          jpx_meta_manager meta_manager);
      /* `this_box' will be NULL only at the root node; otherwise, it
         points to an open box representing the node's contents.  This box
         is used to open further sub-boxes if the `child_list_complete'
         member is false.  The `container_complete' argument should be set to
         true if the node's contents are known to be complete.  If
         `this_box' is NULL, the value will be true if information about the
         headers of all boxes at the file level is known.  Otherwise, this
         argument will be true if and only if `this_box'->is_complete() returns
         true.
            The function returns true if any existing nodes' display state
         changed (as a result of editing); in this case, the caller should
         invalidate the tree control, so as to be sure that any custom
         drawing changes get a chance to take effect. */
    void insert_into_view();
      /* Call this function after all members have been initialized, other
         than `name', `handle' and `can_display', to create a name for the
         node, determine whether or not it corresponds to a box type whose
         contents can be displayed as text, and insert it into the tree view
         control. */
  public: // Links
    kdws_metashow *owner;
    kdws_metanode *parent;
  public: // Data
    bool is_deleted; // If node has been deleted in JPX metadata editor
    bool is_changed; // If node has been altered in JPX metadata editor
    bool ancestor_changed; // If an ancestor was altered in JPX metadata editor
    kdu_uint32 box_type; // 0 if this is the root node
    jp2_locator locator; // Use this to open a box at any point in the future
    int box_header_length; // Use to invoke `jpx_meta_manager::locate_node'
    jpx_metanode jpx_node; // Empty until we can resolve it (if ever)
    int codestream_idx; // -ve if does not refer to a code-stream
    int compositing_layer_idx; // -ve if does not refer to a compositing layer
    bool child_list_complete; // See below
    bool contents_complete; // See below
    kdws_metanode *children; // Points to a list of child nodes, if any
    kdws_metanode *last_child; // Last element in the `children' list
    kdws_metanode *last_complete_child; // See below
    kdws_metanode *next; // Points to next node at current level
  public: // Interaction with the Tree View Window
    char name[80]; // Short text description for this node
    HTREEITEM handle;
    bool can_display; // False unless a leaf whose contents can be displayed
  };
  /* Notes:
         The `child_list_complete' member is false if we cannot be sure that
       all children have been found yet.  In this case, the list of children
       may need to be grown next time `update' is called.  The fact that
       the list of children is complete does not mean that each child in
       the list is itself complete.  This information may be determined by
       using the `last_complete_child' member.
         The `contents_complete' member is true if the node is a super-box
       and its child list is complete, or if the node is a leaf in the tree
       and the corresponding JP2 box's contents are complete.
         The `last_complete_child' member points to the last element in the
       `children' list whose `is_complete' function returns true.  If no
       children are known to have been completely constructed,
       `last_complete_child' will be NULL. */

/*****************************************************************************/
/*                              kdws_metashow                                */
/*****************************************************************************/

class kdws_metashow: public CDialog {
  //---------------------------------------------------------------------------
  public: // Member functions
	  kdws_metashow(kdws_renderer *renderer, POINT preferred_location,
                  int window_identifier, CWnd* pParent=NULL);
      /* The `renderer' reference is provided so that user actions in the
         metashow window can affect what information is currently being
         rendered (e.g., which codestream or compositing layer is displayed).
            The `location' supplied here is the preferred location for the top
         left corner of the metashow window.
            The integer `identifier' supplied here is used to construct a
         title for the metashow window which associates it with the owner's
         window, in which imagery is being rendered.  This is the main window's
         numeric identifier, as assigned by the application object and used
         object and used to construct the title for the main window. */
    ~kdws_metashow();
    void activate(jp2_family_src *src, const char *filename);
      /* Called from when a new file is opened.  This function augments the
         meta-data structure as much as it can, inserting it into the tree
         view, before returning.  If the ultimate data source is a dynamic
         cache, you may have to invoke `update_metadata' when new data
         arrives in the cache.
           The file name string is not copied, so the caller must be
         sure that the string's original memory buffer is preserved, so long
         as the metashow window is activated, in case the need arises to modify
         the title in any way. */
    void update_metadata();
      /* Updates the current tree and view to reflect any new data which may
         have arrived for the `jp2_family_src' object which was passed into
         `activate'.  This does nothing unless the ultimate source of data
         is a dynamic cache, whose contents may have expanded since the last
         call to `activate' or `update_metadata'.  Returns true if and only if
         the meta-data representation is known to be complete -- i.e., all
         tree nodes must be complete and there must be no more tree nodes
         whose headers are not yet known.  It is safe to call this function at
         any time, even if `activate' has not been called. */
    void deactivate();
      /* Called when a file is closed.  Safe to call this at any time, even
         if the object has not previously been activated.  The function
         clears the contents of the current tree and view. */
    void select_matching_metanode(jpx_metanode jpx_node);
      /* Cause the displayed tree-view to be expanded as required to display
         and select the identified node. */
    void close() { DestroyWindow(); }
  //---------------------------------------------------------------------------
  private: // Helper functions
    CTreeCtrl *get_tree_ctrl()
      {
        return (CTreeCtrl *) GetDlgItem(IDC_META_TREE);
      }
    CEdit *get_edit_ctrl()
      {
        return (CEdit *) GetDlgItem(IDC_META_TEXT);
      }
  //---------------------------------------------------------------------------
  private: // My Data
    friend struct kdws_metanode;
    kdws_renderer *renderer; // Non-NULL if we need to delete via application
    jp2_family_src *src; // Null if there is no active meta-data tree
    int num_codestreams; // Num contiguous codestream boxes found
    int num_jpch; // Number of codestream header boxes found
    int num_jplh; // Number of compositing layer header boxes found
    kdws_metanode *tree; // Local skeleton of the meta-data tree
    kdws_metanode *display; // NULL, or node being displayed in the edit box

    kdws_string *display_buf;
    int display_buf_len; // Size of `display_buf' array.
    int display_buf_pos; // Position of next character to add to `display_buf'
    int display_box_pos; // Position of next_display character in JP2 box;
                         // Becomes negative if the box is fully displayed.
    kdu_coords dialog_dims; // Height and width of dialog since last `OnSize'
    kdu_coords tree_dims; // Height & width of tree control since last `OnSize'
    kdu_coords edit_dims; // Height & width of edit control since last `OnSize'
    bool started;

    COLORREF changed_text_colour;
    COLORREF deleted_text_colour;
    CFont deleted_text_font;

    int window_identifier;
    const char *filename;
  //---------------------------------------------------------------------------
  public: // MFC stuff
  	enum { IDD = IDD_META_STRUCTURE };
	protected:
	  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	  virtual void PostNcDestroy();
    virtual void OnOK();
  // ----------------------------------------------------------------------------
  protected: // Message Handlers
    afx_msg void OnClose();
	  afx_msg void OnSelchangedMetaTree(NMHDR* pNMHDR, LRESULT* pResult);
	  afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void tree_ctrl_custom_draw(NMHDR *pNMHDR, LRESULT *pResult);
	  DECLARE_MESSAGE_MAP()
  };

#endif // KDWS_METASHOW_H
