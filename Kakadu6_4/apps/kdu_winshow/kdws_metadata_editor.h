/*****************************************************************************/
// File: kdws_metadata_editor.h [scope = APPS/WINSHOW]
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
   Header file for "kdws_metadata_editor.cpp".
******************************************************************************/
#ifndef KDWS_METADATA_EDITOR_H
#define KDWS_METADATA_EDITOR_H

#include "kdws_renderer.h"

// Declared here:
struct kdws_matching_metanode;
class kdws_matching_metalist;
struct kdws_box_template;
class kdws_metanode_edit_state;
class kdws_metadata_dialog_state;
class kdws_metadata_editor;

#define KD_METAEDIT_MAX_TOOLTIP_CHARS 4096

/*****************************************************************************/
/*                           kdws_matching_metanode                          */
/*****************************************************************************/

struct kdws_matching_metanode {
  jpx_metanode node;
  kdws_matching_metanode *next;
  kdws_matching_metanode *prev;
};

/*****************************************************************************/
/*                           kdws_matching_metalist                          */
/*****************************************************************************/

class kdws_matching_metalist {
  /* This class is used to build a list of top-level jpx_metanodes which
   match the current cursor point for the `kdws_renderer::menu_MetadataEdit'
   function.  A top-level matching jpx_metanode is one which is not a
   descendant of another matching node.  The list does not include any
   numlist nodes, since numlist nodes have no meaning in and of themselves,
   so it makes no sense to present them to an editor.  To facilitate this,
   the `append_node_expanding_numlists' function is provided. */
public: // Member functions
  kdws_matching_metalist() { head = tail = NULL; }
  ~kdws_matching_metalist()
    { while ((tail=head) != NULL) { head=tail->next; delete tail; } }
  kdws_matching_metanode *find_container(jpx_metanode node);
    /* Searches for an existing element in the list, which is either equal
     to `node' or an acestor of `node'.  Returns NULL if none can be found. */
  kdws_matching_metanode *find_member(jpx_metanode node);
    /* Searches for an existing element in the list, which is either equal
     to `node' or a descendant of `node'.  Returns NULL if none can be
     found. */
  void delete_item(kdws_matching_metanode *entry);
    /* Deletes `entry' from the list managed by the object. */
  kdws_matching_metanode *append_node(jpx_metanode node);
    /* Creates a new `kdws_matching_metanode' item to hold `node' and
     appends it to the tail of the internal list, returning a pointer to
     the created item.  This function does no check to see whether or not
     an item already exists on the list which represents `node' or one
     of its ancestors. */
  void append_node_expanding_numlists_and_free_boxes(jpx_metanode node);
    /* This function does the same as `append_node', unless `node'
     happens to be a numlist or a free box, in which case its immediate
     children are added to the list.  The function is recursive, to cater for
     the possibility that a numlist or free node contains a numlist or
     free child. */
  void append_image_wide_nodes(jpx_meta_manager manager,
                               int layer_idx, int stream_idx);
    /* This function traverses the collection of image-wide (i.e., not
     ROI-specific) metadata nodes which can be associated with a
     given compositing layer and/or codestream.  If `layer_idx' >= 0,
     metadata which is associated specifically with the compositing layer is
     examined first.  Then, if `stream_idx' >= 0, metadata associated with
     the indicated codestream is examined next.  Finally, metadata which is
     not associated with any specific compositing layer or codestream is 
     examined.  Following this scanning order, the function adds only
     top-level numlists (i.e., those which are not descendants of
     matching nodes), expanding matching numlist nodes, as explained in
     connection with the `append_node_expanding_numlists' function. */ 
  kdws_matching_metanode *get_head() { return head; }
private: // Data
  kdws_matching_metanode *head, *tail;
};

/*****************************************************************************/
/*                             kdws_box_template                             */
/*****************************************************************************/

struct kdws_box_template {
  kdu_uint32 box_type;
  char box_type_string[5]; // 4-character code in printable form
  const char *file_suffix; // Always a non-empty string
  const char *initializer; // NULL for binary external file representations
public: // Convenience initializer
  void init(kdu_uint32 box_type, const char *suffix, const char *initializer)
    {
      this->box_type=box_type; this->initializer=initializer;
      box_type_string[0] = make_4cc_char((char)(box_type>>24));
      box_type_string[1] = make_4cc_char((char)(box_type>>16));
      box_type_string[2] = make_4cc_char((char)(box_type>>8));
      box_type_string[3] = make_4cc_char((char)(box_type>>0));
      box_type_string[4] = '\0';
      this->file_suffix = (suffix==NULL)?box_type_string:suffix;
    }
private: // Convenience function
  char make_4cc_char(char ch)
    {
      if (ch == ' ')
        return '_';
      if ((ch < 0x20) || (ch & 0x80))
        return '.';
      return ch;
    }
};

// Indices into an array of templates:
#define KDWS_LABEL_TEMPLATE 0
#define KDWS_XML_TEMPLATE 1
#define KDWS_IPR_TEMPLATE 2
#define KDWS_CUSTOM_TEMPLATE 3 // Used if original box type none of above
#define KDWS_NUM_BOX_TEMPLATES (KDWS_CUSTOM_TEMPLATE+1)

/*****************************************************************************/
/*                          kdws_metanode_edit_state                         */
/*****************************************************************************/

class kdws_metanode_edit_state {
public: // Member functions
  kdws_metanode_edit_state(jpx_meta_manager manager,
                           kdws_file_services *file_server);
  void move_to_parent();
  void move_to_next();
  void move_to_prev();
  void move_to_descendants();
  void move_to_node(jpx_metanode node);
  void delete_cur_node(kdws_renderer *renderer);
    /* Deletes the `cur_node' and the JPX data source and makes all required
     adjustments in order to obtain new valid `cur_node', `root_node' and
     `edit_item' values, if possible.  If there is no relevant new node,
     the function returns with `cur_node' empty.  The `renderer' object
     is provided so that its `metadata_changed' function can be called once
     the node has been deleted. */
  void add_child_node(kdws_renderer *renderer);
    /* Adds a descendant to the current node and adjusts the `cur_node'
     member to reference the new child.  The `renderer' object is
     provided so that its `metadata_changed' function can be called once
     the child has been added.  The new descendant is set to be a label
     node, with an initial text string equal to "<new label>". */
  void add_parent_node(kdws_renderer *renderer);
    /* Similar to `add_child_node' except that a new parent of the current
     node is added, having initial label text "<new parent>". */
  void find_sibling_indices();
    /* Before calling this function, `cur_node' and `root_node' should exist
     and reference different metadata nodes, and the unknown sibling indices
     should be set to -ve values.  These are `cur_node_sibling_idx',
     `next_sibling_idx' and `prev_sibling_idx'.  The function uses the
     known values, if any, to help it find the unknown values. */
  void validate_metalist_and_root_node();
    /* This function is called after a node has been deleted.  It deals with
     the fact that the deletion of one node may cause other nodes to be
     deleted; not just descendants, but also nodes which link to the deleted
     node, their descendants and so forth.  The function removes dead nodes
     from the edit list and adjusts `edit_item' and `root_node' accordingly.
     It does not touch `cur_node' since that will be manipulated by the
     caller, who is in the process of deleting `cur_node'. */
public: // Data
  jpx_meta_manager meta_manager; // Provided by the constructor
  kdws_matching_metalist *metalist; // Points to `internal_metalist' if not
            // opened to edit existing metadata.
  kdws_matching_metanode *edit_item; // Item from edit list; either this node
            // is being edited or else one of its descendants is being edited
            // NULL if we are adding metadata from scratch.
  jpx_metanode root_node; // Equals `edit_item->node' if editing; starts out
            // empty if adding metadata from scratch.
  jpx_metanode cur_node; // Either `root_node' or one of its descendants
  int cur_node_sibling_idx; // Which child `cur_node' is of its parent
  int next_sibling_idx; // Idx of the next non-numlist child of our parent
  int prev_sibling_idx; // Idx of the previous non-numlist child of our parent
    // The above two members are used only if `cur_node' is not the root node.
    // A -ve value means there is no next (resp. prev) non-numlist sibling.
private: // Private data
  kdws_matching_metalist internal_metalist;
  kdws_file_services *file_server; // Provided by the constructor
};

/*****************************************************************************/
/*                        kdws_metadata_dialog_state                         */
/*****************************************************************************/

class kdws_metadata_dialog_state {
public: // Functions
  kdws_metadata_dialog_state(kdws_metadata_editor *dlg,
                             int num_codestreams, int num_layers,
                             kdws_box_template *available_types,
                             kdws_file_services *file_server,
                             bool allow_edits);
  ~kdws_metadata_dialog_state();
  bool compare_associations(kdws_metadata_dialog_state *ref);
    /* Returns true if the image-entity or ROI associations of this
     object are the same as those of the `ref' object. */
  bool compare_contents(kdws_metadata_dialog_state *ref);
    /* Returns true if the box-type, label-string and any other node content
     attributes are the same as those of the `ref' object. */
  bool compare(kdws_metadata_dialog_state *ref)
    { return compare_associations(ref) && compare_contents(ref); }
  void eliminate_duplicate_codestreams();
  void eliminate_duplicate_layers();
  void set_label_string(const char *string)
    {
      if (label_string != NULL)
        { delete[] label_string; label_string = NULL; }
      if (string == NULL) return;
      while ((*string == '\n') || (*string == ' ') ||
           (*string == '\r') || (*string == '\t'))
        string++;
      label_string = new char[strlen(string)+1];
      strcpy(label_string,string);
    }
  const char *get_label_string()
    { return (label_string != NULL)?label_string:""; }
  int add_selected_codestream(int idx);
    /* Augments the `selected_codestreams' list.  Returns -ve if `idx' was
     already in the list. Otherwise returns the entry in the
     `selected_codestream' array which contains the new codestream index. */
  int add_selected_layer(int idx);
    /* Augments the `selected_layers' list.  Returns -ve if `idx' was already
     in the list.  Otherwise returns the entry in the `selected_layers'
     array which contains the new compositing layer index. */
  void configure_with_metanode(jpx_metanode node);
    /* Sets up the internal member values based on the information in
     the supplied `node'. */
  void copy(kdws_metadata_dialog_state *src);
    /* Copy contents from `src'. */
  kdws_file *get_external_file() { return external_file; }
  void set_external_file(kdws_file *file, bool new_file_just_retained=false)
    { // Manages the release/retain calls required to ensure that external
      // file resources will be properly cleaned up when not required.  The
      // `new_file_just_retained' argument should be true only if the file
      // has just been obtained using `kdws_file_services::retain...'.
      if ((this->external_file == file) && !new_file_just_retained) return;
      if (external_file != NULL) external_file->release();
      external_file = file;
      if ((file != NULL) && !new_file_just_retained)
        file->retain();
    }
  void set_external_file_replaces_contents()
    { if (external_file != NULL) external_file_replaces_contents = true; }
  bool save_to_external_file(kdws_metanode_edit_state *edit);
    /* Creates a temporary external file if necessary.  Saves the contents
     of the label string, or `edit->cur_node' to the file.  Returns false
     if nothing could be saved for some reason (e.g., the file was locked
     or there was nothing to save because a current node does not exist or
     already has an associated file. */
  bool internalize_label_node();
    /* Reads back the label text from an external file.  Returns false
     if this cannot be done or the current node does not have an associated
     external file.  If it can be done, the file is released
     and the current node is replaced with an internalized label node,
     rather than a delayed label node and the function returns true. */
  bool save_metanode(kdws_metanode_edit_state *edit_state,
                     kdws_metadata_dialog_state *initial_state,
                     kdws_renderer *renderer);
    /* Uses the information contained in the object's members to create
     a new metadata node.  The new metanode becomes the new value of
     `edit_state->cur_node' and, if appropriate, `edit_state->root_node'.
        If `edit_state->cur_node' exists on entry, that node changed to
     the new node; this may require deletion of the existing node or moving
     it to a different parent.
        The `initial_state' object is compared against the current object
     to determine whether or not there are changes that need to be saved.
        If the numlist or ROI associations have changed, the
     `jpx_meta_manager::insert_node' function is used to create a new parent
     to hold the node which is being created (or to create the new node itself
     if it is an ROI description box node).  When this happens, the new node
     (or its parent) is appended to the edit list as a new top-level matching
     metanode and the `edit_state' members are all adjusted to reflect the
     new structure.
        The function returns false if the process could not be performed
     because some inappropriate condition was detected (and flagged to the
     user).  In this case, no changes are made.  Otherwise, the function
     returns true, even if there are no changes to save.
        The `renderer' object's `metadata_changed' function is invoked
     if any changes are made in the metadata structure.  If metadata is
     added only, the function is invoked with its `new_data_only'
     argument equal to true. */
public: // Data members describing associations for the node
  kdws_metadata_editor *dlg;
  int num_codestreams; // Provided by the constructor
  int num_layers; // Provided by the constructor
  int num_selected_codestreams;
  int *selected_codestreams; // Indices of selected codestreams
  int num_selected_layers;
  int *selected_layers; // Indices of selected compositing layers
  bool allow_edits; // Equals `kdws_metadata_editor:allow_edits'
  bool is_link; // If the node is a link -- can't edit actual node contents
  bool is_cross_reference;
  bool can_edit_node; // Links and cross-reference boxes cannot be edited
  bool rendered_result;
  bool whole_image;
  int num_roi_regions;       // Number of regions, in case of complex ROI
  kdu_dims roi_bounding_box; // Bounding box for all regions
  bool roi_is_elliptical;    // True if one or more regions are elliptical
  bool roi_is_encoded;       // True if one or more regions are encoded
public: // Data members describing box contents for the node
  kdws_box_template *available_types; // Array provided by the constructor
  kdws_box_template *offered_types[KDWS_NUM_BOX_TEMPLATES]; // Array of
                             // pointers into the `available_types' array
  int num_offered_types; // Number of options offered to the user
  kdws_box_template *selected_type; // Matches cur selection from offered list
private: // We keep the following member private, since we need to be sure
         // to manage the retain/release calls correctly
  char *label_string; // Non-NULL if node holds an internally stored label.
         // Note that this member and `external_file' can both be non-NULL if
         // the external file is just a copy of the label string; in this case,
         // `external_file_replaces_contents' will be false of course.
  kdws_file *external_file; // If this reference is non-NULL, the present
         // object has retained it.  It may be obtained from an existing
         // metanode, in which case the same referece will be found in
         // `kdws_metanode_edit_state'.  Alternatively, it may have been
         // freshly created to hold user edits.
  bool external_file_replaces_contents; // True if `external_file'
         // is anything other than a copy of the existing box
         // contents (i.e., if edited, or if file is provided by user).
  kdws_file_services *file_server; // Provided by the constructor
};

/*****************************************************************************/
/*                           kdws_metadata_editor                            */
/*****************************************************************************/

class kdws_metadata_editor : public CDialog {
  public: // Functions called before DoModal
	  kdws_metadata_editor(jpx_source *manager,
                         kdws_file_services *file_services,
                         bool can_edit,
                         kdws_renderer *renderer,
                         POINT preferred_location,
                         int window_identifier,
                         CWnd *pParent);
      /* Starts the dialog window, but you need to call one of the
         `configure...' functions to do useful editing.  After that you can
         call `configure' as often as you like before closing the editor.
         [//]
         Note that editing activities may have various side-effects related
         to the `renderer' object.
         [//]
         The `preferred_location' represents a preferred location for the
         upper left hand corner of the window.  If the window cannot fit at
         that location, adjustments are of course made. */
    ~kdws_metadata_editor();
    void close() { DestroyWindow(); }
    void configure(kdws_matching_metalist *metalist);
      /* To use this function, you should be sure that `metalist->get_head()'
         returns a non-empty list of matching metadata nodes. */
    void configure(kdu_dims roi, int codestream_idx, int layer_idx,
                   bool associate_with_renderered_result);
      /* Use this function to associate new metadata with a region. */
    void setActiveNode(jpx_metanode node);
      /* You may call this function after `configureWithEditList' in order to
         move the active node being edited to `node', but this will be ignored
         unless `node' is a descendant of one of the nodes in the `metalist'
         supplied to `configure(metalist)'.  The function has the same
         effect as user navigation to `node'. */
  int interrogate_user(const char *text, const char *caption,
                       UINT flags);
    /* This function essentially invokes `MessageBox' with the supplied flags
       (logical OR of one of MB_ICONERROR, MB_ICONWARNING or MB_ICONQUESTION
       with one of MB_OK, MB_OKCANCEL, MB_YESNO, MB_YESNOCANCEL, etc.),
       with the same return value as MessageBox.  The supplied strings are
       treated as UTF8 and converted to Unicode, as required. */
  private: // Base overrides
    BOOL DestroyWindow();
    void OnCancel() { clickedExit(); }
  private: // Internal functions
    void preconfigure();
    void map_state_to_controls(); // Copies `state' to dialog controls
    void map_controls_to_state(); // Copies dialog control info to `state'
    void update_apply_buttons(); // Enables/disables the "Apply ..." buttons,
                      // based on whether or not there is anything to apply.
  //---------------------------------------------------------------------------
  private: // My data
    bool dialog_created;
    bool mapping_state_to_controls; // Prevent recursive mapping stimulated
                                    // by window messages.
    bool allow_edits; // If editing is allowed
    kdws_box_template available_types[KDWS_NUM_BOX_TEMPLATES];

    int num_codestreams;
    int num_layers;

    jpx_source *source;
    kdws_file_services *file_server;
    kdws_renderer *renderer;


    kdws_matching_metalist *owned_metalist; // Passed by `configure'
    kdws_metanode_edit_state *edit_state;
    kdws_metadata_dialog_state *state;
    kdws_metadata_dialog_state *initial_state;
       // Provision of two copies of the state allows one to keep track of
       // the original node parameters so we can see what has changed.

    _TCHAR tooltip_buffer[KD_METAEDIT_MAX_TOOLTIP_CHARS+1];

  //---------------------------------------------------------------------------
  private: // Control access
    CEdit *x_pos_field()
      { return (CEdit *) GetDlgItem(IDC_META_XPOS); }
    CEdit *y_pos_field()
      { return (CEdit *) GetDlgItem(IDC_META_YPOS); }
    CEdit *width_field()
      { return (CEdit *) GetDlgItem(IDC_META_WIDTH); }
    CEdit *height_field()
      { return (CEdit *) GetDlgItem(IDC_META_HEIGHT); }
    CButton *roi_elliptical_button()
      { return (CButton *) GetDlgItem(IDC_META_ELLIPTICAL); }
    CButton *roi_rectangular_button()
      { return (CButton *) GetDlgItem(IDC_META_RECTANGULAR); }
    CButton *roi_multiregion_button()
      { return (CButton *) GetDlgItem(IDC_META_MULTI_REGION); }
    CButton *roi_encoded_button() 
      { return (CButton *) GetDlgItem(IDC_META_ENCODED); }
    CButton *whole_image_button()
      { return (CButton *) GetDlgItem(IDC_META_WHOLE_IMAGE); }

    CListBox *list_codestreams()
      { return (CListBox *) GetDlgItem(IDC_META_STREAMS); }
    CListBox *list_layers()
      { return (CListBox *) GetDlgItem(IDC_META_LAYERS); }
    CButton *rendered_result_button()
      { return (CButton *) GetDlgItem(IDC_META_RENDERED); }
    CButton *remove_stream_button()
      { return (CButton *) GetDlgItem(IDC_META_REMOVE_STREAM); }
    CButton *remove_layer_button()
      { return (CButton *) GetDlgItem(IDC_META_REMOVE_LAYER); }
    CButton *add_stream_button()
      { return (CButton *) GetDlgItem(IDC_META_ADD_STREAM); }
    CButton *add_layer_button()
      { return (CButton *) GetDlgItem(IDC_META_ADD_LAYER); }
    CSpinButtonCtrl *codestream_stepper()
      { return (CSpinButtonCtrl *) GetDlgItem(IDC_META_SPIN_STREAM); }
    CSpinButtonCtrl *compositing_layer_stepper()
      { return (CSpinButtonCtrl *) GetDlgItem(IDC_META_SPIN_LAYER); }

    CButton *is_link_button()
      { return (CButton *) GetDlgItem(IDC_META_IS_LINK); }
    CComboBox *box_type_popup()
      { return (CComboBox *) GetDlgItem(IDC_META_BOX_TYPE); }
    CComboBox *box_editor_popup()
      { return (CComboBox *) GetDlgItem(IDC_META_BOX_EDITOR); }
    CEdit *label_field()
      { return (CEdit *) GetDlgItem(IDC_META_LABEL); }
    CEdit *external_file_field()
      { return (CEdit *) GetDlgItem(IDC_META_EXTERNAL_FILE); }
    CButton *temporary_file_button()
      { return (CButton *) GetDlgItem(IDC_META_TEMPORARY_FILE); }
    CButton *save_to_file_button()
      { return (CButton *) GetDlgItem(IDC_META_SAVE_TO_FILE); }
    CButton *edit_file_button()
      { return (CButton *) GetDlgItem(IDC_META_EDIT_FILE); }
    CButton *choose_file_button()
      { return (CButton *) GetDlgItem(IDC_META_CHOOSE_FILE); }
    CButton *internalize_label_button()
      { return (CButton *) GetDlgItem(IDC_META_INTERNALIZE_LABEL); }

    CButton *copy_link_button()
      { return (CButton *) GetDlgItem(IDC_META_COPY_LINK); }
    CButton *paste_button()
      { return (CButton *) GetDlgItem(IDC_META_PASTE); }
    CButton *copy_descendants_button()
      { return (CButton *) GetDlgItem(IDC_META_COPY_DESCENDANTS); }

    CButton *apply_button()
      { return (CButton *) GetDlgItem(IDC_META_APPLY); }
    CButton *apply_and_exit_button()
      { return (CButton *) GetDlgItem(IDC_META_APPLY_AND_EXIT); }
    CButton *delete_button()
      { return (CButton *) GetDlgItem(IDC_META_DELETE); }
    CButton *exit_button()
      { return (CButton *) GetDlgItem(IDC_META_CANCEL); }
    CButton *revert_button()
      { return (CButton *) GetDlgItem(IDC_META_REVERT); }

    CButton *next_button()
      { return (CButton *) GetDlgItem(IDC_META_NEXT); }
    CButton *prev_button()
      { return (CButton *) GetDlgItem(IDC_META_PREV); }
    CButton *parent_button()
      { return (CButton *) GetDlgItem(IDC_META_PARENT); }
    CButton *descendants_button()
      { return (CButton *) GetDlgItem(IDC_META_DESCENDANTS); }
    CButton *add_child_button()
      { return (CButton *) GetDlgItem(IDC_META_ADD_CHILD); }
    CButton *add_parent_button()
      { return (CButton *) GetDlgItem(IDC_META_ADD_PARENT); }

    CButton *metashow_button()
      { return (CButton *) GetDlgItem(IDC_META_METASHOW); }
    CButton *catalog_button()
      { return (CButton *) GetDlgItem(IDC_META_CATALOG); }
  //---------------------------------------------------------------------------
  public:
	  enum { IDD = IDD_METAEDIT };
  protected: // Handlers for enhanced user experience
    afx_msg HBRUSH OnCtlColor(CDC *dc, CWnd *wnd, UINT control_type);
    afx_msg void get_tooltip_dispinfo(NMHDR* pNMHDR, LRESULT *pResult);

  protected: // Actions which are used to alter box associations
	  afx_msg void OnDeltaposMetaSpinStream(NMHDR* pNMHDR, LRESULT* pResult);
	  afx_msg void OnDeltaposMetaSpinLayer(NMHDR* pNMHDR, LRESULT* pResult);
	  afx_msg void OnMetaRemoveStream();
	  afx_msg void OnMetaAddStream();
	  afx_msg void OnMetaRemoveLayer();
	  afx_msg void OnMetaAddLayer();
	  afx_msg void setSingleRegion();
	  afx_msg void clearROIEncoded();
	  afx_msg void setROIRectangular();
	  afx_msg void setROIElliptical();
	  afx_msg void setWholeImage();
  protected: // Actions which are used to alter box contents
	  afx_msg void changeBoxType();
	  afx_msg void saveToFile();
	  afx_msg void chooseFile();
	  afx_msg void editFile();
	  afx_msg void changeEditor();
    afx_msg void internalizeLabel();
  protected: // Actions which work with the catalog pastebar
    afx_msg void clickedCopyLink();
    afx_msg void clickedPaste();
    afx_msg void clickedCopyDescendants();
  protected: // Actions used to navigate and control the editing session
	  afx_msg void clickedExit();
	  afx_msg void clickedDelete();
	  afx_msg void clickedApply();
	  afx_msg void clickedApplyAndExit();
	  afx_msg void clickedNext();
	  afx_msg void clickedPrev();
	  afx_msg void clickedParent();
	  afx_msg void clickedDescendants();
	  afx_msg void clickedAddChild();
	  afx_msg void clickedAddParent();
	  afx_msg void findInMetashow();
    afx_msg void findInCatalog();
  protected: // Generic actions
    afx_msg void clickedRevert();
	  afx_msg void otherAction();
  protected: // Set up the message map
	  DECLARE_MESSAGE_MAP()
};

#endif // KD_METADATA_EDITOR_H
