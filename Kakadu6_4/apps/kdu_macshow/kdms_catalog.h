/*****************************************************************************/
// File: kdms_catalog.h [scope = APPS/MACSHOW]
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
   Defines the `kdms_catalog' class, which provides a NSOutlineViewDataSource
 object for the metadata catalog outline view available for JPX files.  The
 object keeps track of labels as they become available, as well as changes
 in the label metadata brought about by editing operations.  It builds
 organized collections of labels, which can be used to navigate to associated
 image entities and/or image regions.
 *****************************************************************************/
#import <Cocoa/Cocoa.h>
#import "kdms_controller.h"
#include "kdu_compressed.h"
#include "jpx.h"
#include "kdu_client_window.h"

// Defined here:
struct kdms_catalog_selection;
struct kdms_catalog_entry;
struct kdms_catalog_child_info;
@class kdms_catalog_item;
struct kdms_catalog_node;
@class kdms_catalog;

// Defined elsewhere:
class kdms_renderer;

#define KDMS_CATALOG_PREFIX_LEN 27 // Best if 1 less than a multiple of 4

#define KDMS_CATALOG_TYPE_INDEX 0
#define KDMS_CATALOG_TYPE_ENTITIES 1
#define KDMS_CATALOG_TYPE_REGIONS 2
#define KDMS_CATALOG_NUM_TYPES 3

// The constants below are used for `kdms_catalog_node::flags'; note that
// the least significant 4 bits of the flags word are reserved for storing
// the catalog type, which is one of the `KDMS_CATALOG_TYPE_xxx' values.
#define KDMS_CATFLAG_TYPE_MASK                   ((kdu_uint16) 0x000F)
#define KDMS_CATFLAG_IS_EXPANDED                 ((kdu_uint16) 0x0010)
#define KDMS_CATFLAG_NEEDS_EXPAND                ((kdu_uint16) 0x0020)
#define KDMS_CATFLAG_NEEDS_COLLAPSE              ((kdu_uint16) 0x0040)
#define KDMS_CATFLAG_NEEDS_RELOAD                ((kdu_uint16) 0x0080)
#define KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION  ((kdu_uint16) 0x0100)
#define KDMS_CATFLAG_PREFIX_NODE                 ((kdu_uint16) 0x0200)
#define KDMS_CATFLAG_HAS_METADATA                ((kdu_uint16) 0x0400)
#define KDMS_CATFLAG_ANONYMOUS_ENTRIES           ((kdu_uint16) 0x0800)
#define KDMS_CATFLAG_COLLAPSED_DESCENDANTS       ((kdu_uint16) 0x1000)
#define KDMS_CATFLAG_LABEL_CHANGED               ((kdu_uint16) 0x2000)


/*****************************************************************************/
/*                         kdms_catalog_selection                            */
/*****************************************************************************/

struct kdms_catalog_selection {
  jpx_metanode metanode;
  kdms_catalog_selection *prev;
  kdms_catalog_selection *next;
};
  /* This structure is used to build a list of metanodes which have been
   selected in the past, so that the history can be retraced. */

/*****************************************************************************/
/*                           kdms_catalog_item                               */
/*****************************************************************************/

@interface kdms_catalog_item : NSObject {
  kdms_catalog_node *node;
}
-(kdms_catalog_item *)initWithNode:(kdms_catalog_node *)node;
-(void)deactivate; // Called when deleting the `kdms_catalog_node'
-(kdms_catalog_node *)get_node; // Returns NULL if `deactivate' has been called

@end // kdms_catalog_item

/*****************************************************************************/
/*                         kdms_catalog_child_info                           */
/*****************************************************************************/

struct kdms_catalog_child_info {
public: // Functions
  kdms_catalog_child_info() { init(); }
  ~kdms_catalog_child_info();
    /* Deletes all the children. */
  void init()
    { num_children=0; children=NULL; check_new_prefix_possibilities=false; }
  void investigate_all_prefix_possibilities();
    /* Recursively checks for new prefix node possibilities, based on the
       `check_new_prefix_possibilities' flag in this node and any of its
       descendants, clearing the flag as it goes. */
  bool create_prefix_node();
    /* This function checks to see whether some of our `children' can be
       collapsed underneath a common "prefix node".  If true, the collapsing
       is performed and the function returns true, but additional opportunities
       may still exist to collapses further nodes under a common prefix. */
public: // Data
  int num_children;
  bool check_new_prefix_possibilities;
  kdms_catalog_node *children;
};
/* This structure is used to record information about the immediate children
   of a metanode (where found inside a `kdms_catalog_entry' object,
   or a catalog node (where found inside a `kdms_catalog_node' object which
   has an empty `kdms_catalog_entry' list.  The latter case applies only
   to "prefix nodes", as described under `kdms_catalog_node'.
     If `check_new_prefix_possibilities' is true, it is possible that this
   object or one of its descendants has an increased number of children,
   some of which might now be able to be collapsed under common prefix
   nodes. */

/*****************************************************************************/
/*                           kdms_catalog_entry                              */
/*****************************************************************************/

struct kdms_catalog_entry {
public: // Functions
  kdms_catalog_entry()
    { incomplete=false; container = NULL; next = prev = NULL; }
  ~kdms_catalog_entry()
    {
      if (metanode.exists())
        metanode.set_state_ref(NULL);
    }
public: // Data
  jpx_metanode metanode; // Can be empty -- see below
  bool incomplete; // See below
  kdms_catalog_node *container;
  kdms_catalog_entry *next;
  kdms_catalog_entry *prev;
  kdms_catalog_child_info child_info; // Information about immediate children
};
/* One instance of this structure is used for each specific metanode which
   can be represented in the catalog.  Where multiple metanodes have the
   same label, which would be displayed in the same way, they
   are all collected together in a doubly-linked list of catlog entries,
   connected via the `next' and `prev' members.  The containing
   `kdms_catalog_node' object points into a single entry in this list via
   its `cur_entry' member.  This single entry is the one to which the
   current visible catalog entry relates; it might not be the head of the
   list.
      The `incomplete' flag is true if any of the following are true:
   1) The number of children for `metanode' is not yet known;
   2) One or more known children cannot yet be recovered via
      `metanode.get_descendant'.
   These conditions can arise only in the event that the ultimate source
   of data is a dynamic cache.
*/

/*****************************************************************************/
/*                           kdms_catalog_node                               */
/*****************************************************************************/

struct kdms_catalog_node {
public: // Member functions
  kdms_catalog_node(int catalog_type);
    /* This function leaves the node in a state which requires further
       intitialization using one of the `init' functions below.  The
       `catalog_type' supplied here must be one of the `KDMS_CATALOG_TYPE_xxx'
       values defined at the start of this header file.  All non-root nodes
       must have the same catalog type as their parent. */
  ~kdms_catalog_node();
    /* Deletes the node and all its children.  Also clears the application
       state reference associated with any non-empty `metanode' interfaces. */
  void init();
    /* Use this for root nodes only. */
  void init(jpx_metanode metanode);
    /* Initialize as a metadata node.  `metanode.set_state_ref' is invoked to
       store a reference back to the `kdms_catalog_entry' object which is
       created to manage the metanode. */
  void init(kdms_catalog_node *ref, int prefix_length);
    /* Initialize as a prefix node, taking the prefix from `ref'. */
  void change_label(const char *new_label);
    /* Call this function to change (if required) the label associated
       with the node.  The function fills in the `label_prefix' array and
       also generates a hash code for the label; it does not actually make
       a copy of the label string.   The function is used internally to set
       the label for the first time (when `label_length'=0).  It is illegal
       to invoke this function on a prefix node.  Changing an existing label
       will result in `note_label_changed' being called. */
  const char *get_label();
    /* Convenience function to retrieve the full UTF-8 label that should
       be used for this node -- it is generated on the fly and not allocated
       any memory here; you should not consider the returned string to be
       persistent.  This function works for all nodes except prefix nodes,
       whose labels are obtained directly from the `label_prefix' array as
       unicode text. */
  void change_link_info(jpx_metanode_link_type link_type);
    /* Call this function to change (if required) the link type
       associated with this node.  When there is no link target, you
       should supply `link_type'=JPX_METANODE_LINK_NONE.  If `label_length'
       is still 0, this function is being used internally to set the link
       information of a node being initialized for the first time.  Otherwise,
       if the link information changes in a way which affects the way a
       catalog item should be displayed, the function invokes
       `note_label_changed'.  Note that this function also sets up the
       `KDMS_CATFLAG_ANONYMOUS_ENTRIES' and
       `KDMS_CATFLAG_COLLAPSED_DESCENDANTS' flags based
       upon link and parent relationships. */
  kdms_catalog_node *insert(kdms_catalog_node *child,
                            kdms_catalog_entry *entry);
    /* Inserts `child' as a descendant of the current node, which may be
     a root node, or any other node.
       If you are inserting into a root node or a prefix node, `entry'
     will be NULL and the new node will be inserted within the children
     managed by `child_info'.  Otherwise, the new node is inserted within the
     children managed by `entry->child_info' and `entry' should
     be one of the entries in the list managed by the current node -- the
     `cur_entry' member points into this list, but not necessarily to `entry'.
     If the current node has `KDMS_CATFLAG_COLLAPSED_DESCENDANTS' set, the
     `entry' argument MUST be the first entry in the current node's entry
     list.
       The function's main features are: 1) it inserts the child in
     alphabetical order, with respect to the upper-case converted unicode label
     representation stored in `label_prefix'; 2) it recursively follows
     prefix node branches in order to insert the new child at the correct
     location; 3) if `child' contains the same label and link type as an
     existing neighbour, the function appends the `cur_entry'
     list from `child' to the entry list of the existing neighbour and
     deletes `child'; and 4) `note_children_increased' is invoked as required.
        In the event that `child' is deleted, in accordance with the third
     feature identified above, the function returns a pointer to the
     node into which the entry list was moved.  Moreover, in this case,
     the function invokes the `note_label_changed' function, to ensure that
     modified labels are loaded by the catalog view. */
  void import_entry_list(kdms_catalog_node *src);
    /* This function is central to the formation of multi-entry catalog nodes
     within `insert'.  It takes all entries from `src' and moves them
     across to the current node, deleting `src' when done and invoking
     `note_label_changed' to reflect the fact that the label displayed in
     the catalog generally depends upon the number of entries. */
  void unlink();
    /* Unlinks the node from its parent and siblings, leaving its
     sibling and `parent' pointers all equal to NULL.  It can
     subsequently be inserted into another node if you like.  This function
     automatically invokes the parent node's `note_children_decreased'
     function.  If the node's parent is a prefix node which is left with
     at most 1 child as a result of this function, the prefix node is
     removed, collapsing any of its descendants back into its own parent.
     For this reason, you should avoid holding onto a reference to the
     current node's parent, unless it is a true metadata node or
     root node. */
  kdms_catalog_node *extract_entry(kdms_catalog_entry *entry);
    /* Extracts `entry' from the current object's list of entries, creating
     a new container for it, which contains just this entry, and returning a
     pointer to the new container.  This function may leave the current
     object empty -- i.e., no entries; however, you would not normally
     bother using this function in that case.
       For nodes with the `KDMS_CATFLAG_COLLAPSED_DESCENDANTS' flag set,
     it is extremely complex to figure out how to extract children which
     belong to `entry' from the collapsed list.  Therefore, when this
     happens, the current function takes a short cut by unlinking all
     descendants of `entry->metanode' and adding them to the touched list
     so that they will eventually be reinserted into the returned
     extracted list, if that is appropriate. */
  void note_children_increased(kdms_catalog_child_info *info);
    /* This function is called if an update operation is known to have
     caused the number of children associated with a node to increase.
     The `info' argument identifies the collection of children whose
     number has increased.  This must be identical to the current object's
     `child_info' member if it is a root or prefix node.  For metadata nodes,
     it must be the `kdms_catalog_entry::child_info' member of one of the
     node's entries -- the one whose number of children has increased.
     The function determines whether or not there will be an impact on the
     node's appearance in the catalog and updates the relevant `needs...'
     flags accordingly. */
  void note_children_decreased(kdms_catalog_child_info *info);
    /* This function is called if an update operation is known to have
     caused the number of children associated with a node to decrease.
     The `info' argument identifies the collection of children whose
     number has decreased.  This must be identical to the current object's
     `child_info' member if it is a root or prefix node.  For metadata nodes,
     it must be the `kdms_catalog_entry::child_info' member of one of the
     node's entries -- the one whose number of children has decreased.
     The function determines whether or not there will be an impact on the
     node's appearance in the catalog and updates the relevant `needs...'
     flags accordingly.  If expandability of the node changes, it must be
     reloaded. */
  void note_label_changed();
    /* This function is called if the label associated with a node has
       changed.  This schedules a reload of the node's parent.  It also
       marks the node with `KDMS_CATFLAG_LABEL_CHANGED' which causes the
       `kdms_catalog:outlineView:objectValueForTableColumn:byItem' function
       to generate a new object. */
  void reflect_changes_part1(kdms_catalog *catalog);
    /* This function is used together with `reflect_changes_part2' to
       process the changes recorded by the above `note...' functions.  This
       allows the effects of many changes to be efficiently reflected in a
       much smaller number of operations.  The function recursively scans
       through the nodes which need attention, collapsing nodes which
       are marked with the `KDMS_CATFLAG_NEEDS_COLLAPSE' flag, from the
       bottom of the tree upwards. */
  void reflect_changes_part2(kdms_catalog *catalog);
    /* This function recursively scans through the nodes which need attention,
       expanding and reloading nodes on the way down, as required. */
  int compare(kdms_catalog_node *ref, int num_chars=0);
    /* Compares the current and `ref' nodes to determine the relative
       order in which they should appear within the catalog.  If
       `num_chars' is greater than 0, only the `label_prefix' array is used
       for comparison and then only the first `num_chars' characters of the
       label are used.  If `num_chars' is 0, the function initially uses the
       `label_prefix' array; if the nodes are identical with respect to the
       label prefix, it uses the full text of each label; if they are still
       indistinguishable, it uses the link type.  Finally, if the two nodes
       are still indistinguishable and have a link type of
       `JPX_GROUPING_LINK_TYPE', the function compares their link targets. */
  kdms_catalog_node *
    find_best_collapse_candidate(int depth,
                                 kdms_catalog_selection *history,
                                 int &most_recent_use);
    /* This function is used to automatically collapse branches in the catalog
       which have not been used for some time.  The function searches for any
       node at depth `depth' in the hierarchy, relative to the current node
       (a depth of 1 refers to immediate descendants) which is currently
       expanded and does not have `KDMS_CATFLAG_NEEDS_EXPAND' or
       `KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION' set.  There may be multiple
       nodes of this form, so they are compared with the `history' list to
       determine the node which has least recently been used.  If no node has
       yet been discovered, the `most_recent_use' argument will be negative.
       Upon return, if a collapsable node is encountered, that node is
       returned and `first_history_use' is set to the distance back into
       the history list (starting from 0 as the head of the `history' list)
       at which the first use of any of the node in question's descendants
       is found -- i.e., the first point in the history which depends on the
       node being expanded. We do not search back further than
       `KDMS_MAX_AUTO_COLLAPSE_HISTORY' elements in the past -- if no
       user of the node is found during the search, `first_history_usage'
       is set to `KDMS_MAX_AUTO_COLLAPSE_HISTORY' and that node is
       automatically the best match. */
  bool needs_client_requests_if_expanded();
    /* This function determines whether JPIP client requests ought to be
       generated to fetch new metadata if this node were expanded.  The
       function does not care whether or not the node or its descendants
       are currently expanded.  The answer is YES if any of the following
       hold: a) this is a root node; b) `KDMS_CATFLAG_HAS_METADATA' is set and
       `cur_entry->incomplete' is true; c) `KDMS_CATFLAG_COLLAPSED_DESCENDANTS'
       is set and any of the entries in the list headed by `cur_entry' has its
       `incomplete' flag true; d) any of the node's children is a link node
       whose link target has not yet been resolved. */
  int generate_client_requests(kdu_window *client_window);
    /* This recursive function does most of the work of
       `kdms_catalog:generate_client_requests'.  It recursively visits all
       nodes in the catalog which are expanded, along with nodes whose
       parents are expanded.  It generates appropriate requests for both
       links and descendants of expanded nodes, returning the total number
       of calls issued to `client_window->add_metareq'. */
public: // Data which relates to the visible catalog
  kdms_catalog_item *item; // Required to integrate with NSOutlineView.
  id object_value; // Object retained for the outline view  
  kdu_uint16 flags;
  kdu_uint16 label_length; // Num unicode chars in `label_prefix'
  kdu_uint32 label_hash;
  kdu_uint16 label_prefix[KDMS_CATALOG_PREFIX_LEN+1];
    // null-terminated uppercase version of the label, truncated to at most
    // `KDMS_CATLOG_PREFIX_LEN' characters; used to order entries.
  kdms_catalog_node *next_sibling; // Doubly-linked list of siblings
  kdms_catalog_node *prev_sibling;  
  kdms_catalog_node *parent;
  kdms_catalog_entry *parent_entry; // Entry in `parent' whose
          // `kdms_catalog_child_info' list contains us as a child; note that
          // the `parent_entry->metanode' member might not be our parent if
          // the parent is a node with collapsed descendants -- see below.
public: // Data which relates to the actual represented metadata
  jpx_metanode_link_type link_type; // All entries must have same link type
  kdms_catalog_child_info *child_info; // For roots and prefix nodes only
  kdms_catalog_entry *cur_entry; // For nodes which have metadata
};
  /* Notes:
      This object represents a single item which can be displayed within
   the catalog.  There may be multiple metadata entries within this item,
   but only one of them (identified by `cur_entry') can actually be presented
   at any given time.  The user can navigate between them using the arrow
   keys or indirectly via actions which invoke
   `kdms_catalog:select_matching_metanode'.
      For efficiency reasons, the initial prefix of up to
   KDMS_CATALOG_PREFIX_LEN characters of the label string are converted to
   upper case and stored in the `label_prefix' array when the object is
   constructed.  This is used for searching and other related operations.
   The full label text, on the other hand, is generated on the fly when
   required, so as to keep the catalog nodes as light weight as possible.
      There are three types of catalog nodes: 1) root nodes; 2) prefix
   nodes; and 3) metadata nodes.  Metadata nodes keep a list of one or
   more `kdms_catalog_entry' objects, each of which represents a single
   `jpx_metanode' where all entries in the list have the same label, for
   one reason or another.  Prefix nodes display a common prefix in the
   catalog -- the prefix itself is the upper-case string found in
   `label_prefix' and its length is found in `label_length'.  All children
   of a prefix node have labels which, after conversion to upper-case,
   share the prefix found in the prefix node.
      The `KDMS_CATFLAG_ANONYMOUS_ENTRIES' and
   `KDMS_CATFLAG_COLLAPSED_DESCENDANTS' flags
   deserve some further explanation.  These flags are meaningful only if
   `KDMS_CATFLAG_HAS_METADATA' is set.
      `KDMS_CATFLAG_ANONYMOUS_ENTRIES' is set if the entries represent
   links with potentially different parents -- this is judged to be the
   case for any node whose entries have parents that are not in the
   catalog or whose parents are grouping links.  For anonymous entry nodes,
   there is no way to meaningfully order multiple entries, nor is there
   much value in the user navigating between the entries, because they
   are all just links to a single target.  The `cur_entry' member may
   still point to the most recently selected entry, however, which can
   be useful for editing purposes.
      `KDMS_CATFLAG_COLLAPSED_DESCENDANTS' is set only for grouping link
   nodes.  In this case, the children of all entries are sorted into the
   `kdms_catalog_entry::child_info' list of the first entry associated with
   the node; moreover, in this case, `cur_entry' is not permitted to point
   to anything other than this first entry in the list.  The other entries
   exist only to keep track of the metanodes which are associated with this
   catalog node.  If `KDMS_CATFLAG_COLLAPSED_DESCENDANTS' is set,
   `KDMS_CATFLAG_ANONYMOUS_ENTRIES' is also necessarily set.
 */

/*****************************************************************************/
/*                              kdms_catalog                                 */
/*****************************************************************************/


@interface kdms_catalog : NSOutlineView {
  kdms_renderer *renderer;
  NSButton *back_button;
  NSButton *fwd_button;
  NSButton *peer_button;
  NSTextField *paste_bar;
  NSDictionary *root_attributes;
  NSDictionary *prefix_attributes;
  NSDictionary *link_attributes;
  NSDictionary *grouping_link_attributes;
  NSDictionary *alternate_parent_link_attributes;
  bool need_metanode_touch; // Before `meta_manager' recovered for first time
  bool performing_collapse_expand; // Defers client updates during op sequence
  bool need_client_update; // If need for update found while above flag true
  kdms_catalog_node *root_nodes[KDMS_CATALOG_NUM_TYPES];
  kdms_catalog_selection *history; // Points to the current selection in the
                                   // doubly linked selection history list.
  jpx_metanode *pasted_link;        // Have to store these on heap, because
  jpx_metanode *pasted_node_to_cut; // they have C++ constructors.
  char *pasted_label;
}
-(kdms_catalog *)initWithFrame:(NSRect)frame_rect
                      pasteBar:(NSTextField *)paste_bar
                    backButton:(NSButton *)back_button
                     fwdButton:(NSButton *)fwd_button
                    peerButton:(NSButton *)peer_button
                   andRenderer:(kdms_renderer *)renderer;
-(void)update_metadata;
  /* Scans all newly available/changed/deleted metadata nodes and updates the
     internal structure accordingly.  The metadata manager is obtained each
     time time by calling `kdms_renderer::get_meta_manager', just in case the
     relevant `jpx_source' object becomes invalid.  If this is found
     to be the first time a valid metadata manager is retrieved in this way,
     the `jpx_metanode::touch' function is invoked on the root node of the
     metadata tree, so as to ensure that all nodes in the tree can be accessed
     by `jpx_meta_manager::get_touched_nodes'. */
-(void)deactivate;
  /* This message must be sent while the `renderer' object and its embedded
     `jpx_source' object still exist, since only at that point is it actually
     safe to delete the internal `kdms_catalog_node' objects and detach
     them from `jpx_metanode's managed inside the `jpx_source'.  If we wait
     until `dealloc' is invoked to do this, the `jpx_in' object may already
     have been destroyed, leading to invalid memory accesses.   The problem
     with relying upon `dealloc' is that it may not be invoked until the
     main run-loop examines the auto-release pool, so we cannot force it to
     occur prior to destruction of the `jpx_source' machinery into which it
     is linked.  In practice, this function clears the outline view and then
     deletes all catalog nodes. */
-(void)dealloc;
-(NSAttributedString *)getFancyLabel:(NSString *)label
                         forLinkType:(jpx_metanode_link_type)link_type;
  /* Creates a new attributed string formatted to represent `label' as a link
   of the indicated type or, if `link_type' is `JPX_METANODE_LINK_NONE' as a
   root label. */
-(jpx_metanode)get_selected_metanode;
  /* Returns the node which is currently selected, if any. */
-(jpx_metanode)get_unique_selected_metanode;
  /* Same as above, but returns an empty interface if the item which is
   selected represents more than one actual metadata nodes.  This happens,
   for example if a JPX_GROUPING_LINK node is selected. */
-(jpx_metanode)find_best_catalogued_descendant:(jpx_metanode)node
                                  and_distance:(int &)distance
                             excluding_regions:(bool)exclude_regions;
  /* Searches the descendants of `node' until one is found which is in the
     catalog, passing through grouping link nodes (such nodes will not
     themselves be returned by this function).  If the search yields multiple
     descendants which are all in the catalog, it employs a heuristic to
     determine the best matching sibling, based on its "distance" within the
     metadata graph to the most recently selected metanode in the catalog
     (if any).  The distance between two nodes in the metadata graph is
     measured by the total number of edges that must be traversed to walk up
     from one node to the closest common parent and back down to the other
     node.  If there are link nodes in the graph, the function also examines
     paths which traverse such links.
     [//]
     If the function returns a non-empty `jpx_metanode' interface, it also
     sets `distance' to the determined distance between the returned
     metanode and the most recently selected metanode in the catalog, plus
     the distance between the returned metanode and `node'.  If the returned
     metanode is itself the most recently selected metanode,
     the returned distance is equal to the depth of the returned node
     relative to `node' -- this value is always non-negative and 0 only
     if `node' is itself the most recently selected metanode.  If the
     returned metanode is not itself the most recently selected metanode,
     the returned distance is augmented by the distance in the metadata
     graph, between the returned metanode and the most recently selected
     metanode.
     [//]
     If `exclude_regions' is true, the function does not explore past any
     region-of-interest nodes. */
-(bool)select_matching_metanode:(jpx_metanode)metanode;
  /* If `node' is in the metadata catalog already, the function selects
     it.  Otherwise, the function effectively invokes
     `find_best_catalogued_descendant' on `node' first and then selects the
     metadata node determined by that function.  If any such selection
     changes the current selection at all, the function also adjusts
     the scroll view so that the newly selected row appears in the central
     third of the display, to the extent that this is possible; this is
     achieved by invoking [self scrollRow:... toVisibleLocation:-1].
     The function returns true if a match was found; false otherwise. */
-(void)paste_label:(const char *)label;
-(void)paste_link:(jpx_metanode)link_target;
-(void)paste_node_to_cut:(jpx_metanode)node_to_cut;
  /* The above functions are used to copy a label, or remember a link or
   node to cut, in the pastebar, which can be later used to construct
   new metadata.  The pastebar can store a linkm a copied label or a the
   identity of a node which is to be cut and moved to a new location.  Links
   are displayed in a manner which depends on the type of node which is
   linked, and this information is updated automatically if the linked
   metanode's content changes.  In any case, copied labels are presented
   in regular text within the pastebar, while links are presented in blue
   text with underlining. */
-(const char *)get_pasted_label;
-(jpx_metanode)get_pasted_link;
-(jpx_metanode)get_pasted_node_to_cut;
  /* The above functions retrieve information from the pastebar,
   if available. */
-(int)generate_client_requests:(kdu_window *)client_window;
  /* This important function is invoked from within
     `kdms_renderer::update_client_window_of_interest'. It examines all
     items in the catalog which are expanded and determines which of them
     are incomplete.  For these, it arranges for a suitable metadata request
     to be included in the `client_window' object.  The function includes
     metadata requests for any metanode whose children are expanded in the
     catalog, but for which the number of children is not yet known or one
     or more children cannot yet be parsed.  The function also includes
     metadata requests for the target of any link node in an expanded
     context.  Finally, the function generates high level requests for
     various types of metadata to fill the various categories of catalog
     information.  Returns the total number of calls issued to
     `client_window->add_metareq'. */

// Methods used to catch user events
-(IBAction)user_double_click:(NSBrowser *)sender;
-(IBAction)back_button_click:(NSButton *)sender;
-(IBAction)fwd_button_click:(NSButton *)sender;
-(IBAction)peer_button_click:(NSButton *)sender;
-(void)outlineViewSelectionDidChange:(NSNotification *)notification;
  /* Delegate method allows us to catch and record selected nodes. */
-(void)outlineViewItemDidExpand:(NSNotification *)notification;
-(void)outlineViewItemDidCollapse:(NSNotification *)notification;
-(void)keyDown:(NSEvent *)the_event;
  /* Catch up/down arrow navigation keys and reinterpret them for
     multi-entry catalog items. */
-(void)rightMouseDown:(NSEvent *)the_event;
  /* Catch right-click events and use them to open up the editor. */
-(NSString *)outlineView:(NSOutlineView *)ov
          toolTipForCell:(NSCell *)cell
                    rect:(NSRectPointer)rect
             tableColumn:(NSTableColumn *)tc
                    item:(id)item
           mouseLocation:(NSPoint)mouseLocation;

// Methods used to implement the NSOutlineViewDataSource informal protocol
-(id)outlineView:(NSOutlineView *)outlineView
           child:(NSInteger)index
          ofItem:(id)item;
-(BOOL)outlineView:(NSOutlineView *)outlineView
       isItemExpandable:(id)item;
-(NSInteger)outlineView:(NSOutlineView *)outlineView
            numberOfChildrenOfItem:(id)item;
-(id)outlineView:(NSOutlineView *)view
     objectValueForTableColumn:(NSTableColumn *)tableColumn
          byItem:(id)item;

// Helper functions
-(kdms_catalog_node *)get_selected_node;
  // Returns the catalog node which is currently selected, or else NULL.
  // Used to implement other selection-related functions.
-(void)expand_context_for_node:(kdms_catalog_node *)node
                     and_entry:(kdms_catalog_entry *)entry
                  with_cleanup:(bool)cleanup;
  // This function is used by `select_matching_metanode' and elsewhere to
  // ensure that the identified `node' appears within an expanded context
  // in the catalog.  If `entry' is non-NULL, it must belong to the
  // `node->cur_entry' list, and the function makes sure that this node
  // appears in the relevant catalog row, rather than any other entry for
  // the node.  If you supply a NULL `entry' argument, the function leaves
  // any `cur_entry' as is (if there is any).  If `cleanup' is true, the
  // function endeavours to collapse less recently used branches in the
  // metadata hierarchy so that the catalog view does not become too
  // cluttered through repeated expansions.
-(NSRange)getVisibleRows;
  // Returns the range of rows currently visible within the enclosing
  // scroll view
-(void)scrollRow:(NSInteger)row_idx toVisibleLocation:(NSInteger)pos;
  /* Adjusts the scroll position so that the row at absolute location `row_idx'
     appears at location `pos' relative to the first visible row in the
     scroll view.  If `pos' lies outside the bounds of the scroll view (e.g.,
     if `pos' is negative), scrolling is performed to make the row appear in
     the central third of the scroll view, or as close as possible to this
     region. */
-(void)process_double_click:(jpx_metanode)metanode;
  /* Does most of the work of `user_double_click' so the behaviour can be
     invoked in response to other actions, such as an Enter key being
     depressed while a node is selected with the catalog having keyboard
     focus. */

@end // kdms_catalog

