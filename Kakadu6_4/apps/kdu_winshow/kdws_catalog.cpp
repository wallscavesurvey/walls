/*****************************************************************************/
// File: kdws_catalog.cpp [scope = APPS/WINSHOW]
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
   Implements the `kdws_catalog' class, which provides a metadata catalog
 for JPX files.  The object keeps track of labels as they become available,
 as well as changes in the label metadata brought about by editing operations.
 It builds organized collections of labels, which can be used to navigate to
 associated image entities and/or image regions.
 *****************************************************************************/
#include "stdafx.h"
#include "kdu_utils.h"
#include "kdws_catalog.h"
#include "kdws_renderer.h"
#include "kdws_window.h"
#include "kdws_manager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define KD_CATALOG_TREE_CTRL_ID     (AFX_IDW_PANE_FIRST+100)
#define KD_CATALOG_BACK_BUTTON_ID   (AFX_IDW_PANE_FIRST+101)
#define KD_CATALOG_FWD_BUTTON_ID    (AFX_IDW_PANE_FIRST+102)
#define KD_CATALOG_PEER_BUTTON_ID   (AFX_IDW_PANE_FIRST+103)
#define KD_CATALOG_PASTEBAR_CTRL_ID (AFX_IDW_PANE_FIRST+104)


/*===========================================================================*/
/*                           INTERNAL FUNCTIONS                              */
/*===========================================================================*/

/*****************************************************************************/
/* INLINE                    get_entry_for_metanode                          */
/*****************************************************************************/

static inline kdws_catalog_entry *
  get_entry_for_metanode(jpx_metanode metanode)
{
  return (kdws_catalog_entry *) metanode.get_state_ref();
}

/*****************************************************************************/
/* STATIC             get_label_and_link_info_for_metanode                   */
/*****************************************************************************/

static const char *
  get_label_and_link_info_for_metanode(jpx_metanode metanode,
                                       jpx_metanode_link_type &link_type,
                                       jpx_metanode &link_target)
{
  link_type = JPX_METANODE_LINK_NONE;
  link_target = metanode.get_link(link_type);
  const char *label = NULL;
  int i_param;
  void *addr_param;
  if (link_target.exists())
    {
      if ((label = link_target.get_label()) == NULL)
        {
          bool rres;
          int num_cs, num_l;
          if (link_target.get_num_regions() > 0)
            label = "<region (ROI)>";
          else if (link_target.get_numlist_info(num_cs,num_l,rres))
            label = "<image entities>";
          else if (link_target.get_delayed(i_param,addr_param) &&
                   (link_target.get_box_type() == jp2_label_4cc))
            label = "<label in file pending save>";
        }
    }
  else if (link_type != JPX_METANODE_LINK_NONE)
    label = "<link pending...>";
  else if (((label = metanode.get_label()) == NULL) &&
           metanode.get_delayed(i_param,addr_param) &&
           (metanode.get_box_type() == jp2_label_4cc))
    label = "<label in file pending save>";
  
  return label;
}

/*****************************************************************************/
/* STATIC                    generate_label_hash                             */
/*****************************************************************************/

static kdu_uint32 generate_label_hash(const char *label)
{
  kdu_uint32 result = 0;
  while (*label != '\0')
    {
      result ^= (result << 3);
      result += (result << 7);
      result ^= (kdu_uint32)(*(label++));
    }
  return result;
}

/*****************************************************************************/
/* STATIC          remove_and_touch_descendants_of_metanode                  */
/*****************************************************************************/

static void
  remove_and_touch_descendants_of_metanode(jpx_metanode metanode)
  /* Recursively scans all descendants of `metanode', extracting and
     unlinking them from the catalog structure, but touching them so that
     they can be reinserted in the course of `kdws_catalog::update_metadata'.
     This function is invoked from within `kdws_catalog_node::extract_entry'
     when asked to extract an entry from a multi-entry node with the
     `has_collapsed_descendants' flag set to true. */
{
  int c, num_descendants; metanode.count_descendants(num_descendants);
  for (c=0; c < num_descendants; c++)
    {
      jpx_metanode child = metanode.get_descendant(c);
      if (!child)
        continue;
      remove_and_touch_descendants_of_metanode(child);
      kdws_catalog_entry *entry = get_entry_for_metanode(child);
      if (entry != NULL)
        {
          kdws_catalog_node *node = entry->container;
          if ((entry->prev != NULL) || (entry->next != NULL))
            node = node->extract_entry(entry);
          assert(entry == node->cur_entry);
          node->unlink();
          delete node;
          child.touch();
        }
    }
}

/*****************************************************************************/
/* STATIC                 measure_metanode_distance                          */
/*****************************************************************************/

static int
  measure_metanode_distance(jpx_metanode src, jpx_metanode tgt,
                            bool allow_src_links, bool allow_tgt_links,
                            int min_distance=INT_MAX)
  /* Measures similarity between the graphs terminating at `src' and `tgt',
     searching paths which traverse up through each node's ancestry to the
     point at which they no longer reside within the catalog, allowing for
     the traversal of at most one link from the `src' tree if `allow_src_links'
     is true and at most one link from the `tgt' tree if `allow_tgt_links' is
     true.  On entry, the `min_distance' argument holds the minimum path
     distance found so far, allowing the search to be terminated early to save
     effort. */
{
  if (!tgt)
    return INT_MAX;
  
  // Start by comparing `src' with each node in the ancestry of `tgt'
  int distance;
  jpx_metanode scan;
  for (scan=tgt, distance=0;
       scan.exists() && (scan.get_state_ref() != NULL) &&
       (distance < min_distance);
       distance++, scan=scan.get_parent())
    if (scan == src)
      { min_distance = distance; break; }

  // Now compare `src' with any links found in each node of the `tgt' hierarchy
  jpx_metanode_link_type link_type;
  if (allow_tgt_links)
    {
      for (scan=tgt, distance=1;
           scan.exists() && (scan.get_state_ref() != NULL) &&
           (distance < min_distance);
           scan=scan.get_parent())
        {
          jpx_metanode node = scan.get_link(link_type);
          if (node.exists() && (node.get_state_ref() != NULL))
            {
              int d = distance +
                measure_metanode_distance(src,node,allow_src_links,false,
                                          min_distance-distance);
              if (d < min_distance)
                min_distance = d;
            }
          distance++; // Add 1 to branch count to follow links from descendants
          if (distance < min_distance)
            {
              int c, num_descendants; scan.count_descendants(num_descendants);
              for (c=0; c < num_descendants; c++)
                if ((node=scan.get_descendant(c)).exists() &&
                    (node=node.get_link(link_type)).exists() &&
                    (node.get_state_ref() != NULL))
                  {
                    int d = distance +
                      measure_metanode_distance(src,node,allow_src_links,false,
                                                min_distance-distance);
                    if (d < min_distance)
                      min_distance = d;
                  }
            }
        }
    }
  
  // Now consider comparisons with ancestors of `src'
  if ((scan=src.get_parent()).exists() && (scan.get_state_ref() != NULL))
    {
      int d = 1 + measure_metanode_distance(scan,tgt,allow_src_links,
                                            allow_tgt_links,min_distance-1);
      if (d < min_distance)
        min_distance = d;
    }
  
  // Finally consider comparisons with links from `src'
  if (allow_src_links)
    {
      jpx_metanode node = src.get_link(link_type);
      if (node.exists() && (node.get_state_ref() != NULL))
        {
          int d = 1 +
            measure_metanode_distance(node,tgt,false,allow_tgt_links,
                                      min_distance-1);
          if (d < min_distance)
            min_distance = d;
        }
      if (2 < min_distance)
        {
          int c, num_descendants; scan.count_descendants(num_descendants);
          for (c=0; c < num_descendants; c++)
            if ((node=src.get_descendant(c)).exists() &&
                (node=node.get_link(link_type)).exists() &&
                (node.get_state_ref() != NULL))
              {
                int d = 2 +
                  measure_metanode_distance(node,tgt,false,allow_tgt_links,
                                            min_distance-2);
                if (d < min_distance)
                  min_distance = d;
              }
        }
    }
  return min_distance;
}

/*****************************************************************************/
/* STATIC                 look_for_related_child_link                        */
/*****************************************************************************/

static jpx_metanode
  look_for_related_child_link(jpx_metanode target, jpx_metanode origin)
  /* This function is called when we are about to follow a link from `origin'
     to `target' so as to select `target'.  The function checks to see if
     `target' has an immediate descendant which links back to any
     node in the ancestry of `origin'.  If so, that descendant is returned;
     otherwise, the function simply returns `target'. */
{
  int c, num_descendants; target.count_descendants(num_descendants);
  for (c=0; c < num_descendants; c++)
    {
      jpx_metanode child_tgt, child = target.get_descendant(c);
      jpx_metanode_link_type link_type;
      if (child.exists() &&
          (child_tgt = child.get_link(link_type)).exists() &&
          origin.find_path_to(child_tgt,0,JPX_PATH_TO_DIRECT,0,
                              NULL,NULL).exists())
        return child;
    }
  return target;
}

/*****************************************************************************/
/* STATIC                       find_next_peer                               */
/*****************************************************************************/

static jpx_metanode find_next_peer(jpx_metanode metanode)
  /* Looks for the next sibling of `metanode' which has a state_ref (i.e., is
     in the catalog), cycling back to the first sibling if necessary.  Returns
     an empty interface if there is none -- certainly this happens if
     `metanode' is an empty interface. */
{
  jpx_metanode parent;
  if (metanode.exists())
    parent = metanode.get_parent();
  jpx_metanode peer;
  if (parent.exists())
    {
      bool found_self = false;
      int c, num_descendants; parent.count_descendants(num_descendants);
      for (c=0; c < num_descendants; c++)
        {
          jpx_metanode child = parent.get_descendant(c);
          if (get_entry_for_metanode(child) == NULL)
            continue;
          if (child == metanode)
            found_self = true;
          else if (!peer)
            peer = child;
          else if (found_self)
            { peer = child; break; }
        }
    }
  return peer;
}

/*===========================================================================*/
/*                           kdws_catalog_child_info                         */
/*===========================================================================*/

/*****************************************************************************/
/*             kdws_catalog_child_info::~kdws_catalog_child_info             */
/*****************************************************************************/

kdws_catalog_child_info::~kdws_catalog_child_info()
{
  kdws_catalog_node *child;
  while ((child = children) != NULL)
    {
      children = child->next_sibling;
      delete child;
    }
}

/*****************************************************************************/
/*       kdws_catalog_child_info::investigate_all_prefix_possibilities       */
/*****************************************************************************/

void kdws_catalog_child_info::investigate_all_prefix_possibilities()
{
  if (!check_new_prefix_possibilities)
    return;
  kdws_catalog_node *scan;
  for (scan=children; scan != NULL; scan=scan->next_sibling)
    {
      kdws_catalog_entry *entry = scan->cur_entry;
      if (!(scan->flags & KDWS_CATFLAG_HAS_METADATA))
        {
          assert(scan->child_info != NULL);
          if (scan->child_info->check_new_prefix_possibilities)
            scan->child_info->investigate_all_prefix_possibilities();
        }
      else if (entry != NULL)
        {
          while (entry->prev != NULL)
            entry = entry->prev;
          for (; entry != NULL; entry=entry->next)
            if (entry->child_info.check_new_prefix_possibilities)
              entry->child_info.investigate_all_prefix_possibilities();
        }
    }
  while (create_prefix_node());
  check_new_prefix_possibilities = false;
}

/*****************************************************************************/
/*                kdws_catalog_child_info::create_prefix_node                */
/*****************************************************************************/

bool kdws_catalog_child_info::create_prefix_node()
{
  if (children == NULL)
    return false;
  
  kdws_catalog_node *container = children->parent;
  kdws_catalog_entry *container_entry = children->parent_entry;
  assert(((container_entry==NULL) && (this==container->child_info)) ||
         ((container_entry!=NULL) && (this==&(container_entry->child_info))));  

  // Find the longest prefix which matches at least 5 nodes (with longer
  // labels), such that there are other nodes which do not match this
  // prefix.
  int max_prefix_len=0, max_prefix_matches=0;
  kdws_catalog_node *max_prefix_start=NULL;
  int prefix_len, min_prefix_len=1;
  if (container->flags & KDWS_CATFLAG_PREFIX_NODE)
    min_prefix_len = container->label_length+1;
  kdws_catalog_node *scan, *next;
  int min_matches_for_next_prefix_len=5;
  if (num_children > 20)
    min_matches_for_next_prefix_len = 2;
  else
    for (scan=children; scan != NULL; scan=scan->next_sibling)
      if ((scan->label_length == min_prefix_len) &&
          (scan->flags & KDWS_CATFLAG_PREFIX_NODE))
        { min_matches_for_next_prefix_len = 2; break; }
  for (prefix_len=min_prefix_len;
       prefix_len <= KDWS_CATALOG_PREFIX_LEN;
       prefix_len++)
    { // See if we can find children which have a common prefix with length
      // `prefix_len' and are worth collapsing.
      kdws_catalog_node *start=NULL;
      int num_matches = 0;
      int min_matches = min_matches_for_next_prefix_len;
      bool have_something_else = false;
      for (scan=children; scan != NULL; scan=scan->next_sibling)
        {
          if (scan->label_length <= prefix_len)
            { have_something_else = true; continue; }
          if ((scan->label_length == (prefix_len+1)) &&
              (scan->flags & KDWS_CATFLAG_PREFIX_NODE))
            min_matches_for_next_prefix_len = 2; // We have a prefix node whose
                  // length is equal to the next prefix length to be tested, so
                  // we will encourage the formation of additional prefix nodes
                  // with that same length.
          if (start != NULL)
            {
              if (scan->compare(start,prefix_len) == 0)
                { num_matches++; continue; }
              else
                {
                  have_something_else = true;
                  if (num_matches >= min_matches)
                    break;
                }
            }
          start = scan;
          num_matches = 1;
        }
      if ((num_matches >= min_matches) && have_something_else)
        {
          max_prefix_len = prefix_len;
          max_prefix_matches = num_matches;
          max_prefix_start = start;
        }
      else if (num_matches < 2)
        break;
    }
  if (max_prefix_start == NULL)
    return false; // Nothing to do

  // At this point we need to create the prefix node
  prefix_len = max_prefix_len;
  kdws_catalog_node *new_prefix =
    new kdws_catalog_node(max_prefix_start->flags & KDWS_CATFLAG_TYPE_MASK);
  new_prefix->init(max_prefix_start,prefix_len);
  new_prefix->parent = container;
  new_prefix->parent_entry = container_entry;
  if ((new_prefix->prev_sibling = max_prefix_start->prev_sibling) == NULL)
    this->children = new_prefix;
  else
    new_prefix->prev_sibling->next_sibling = new_prefix;
  new_prefix->next_sibling = max_prefix_start;
  this->num_children++;
  max_prefix_start->prev_sibling = new_prefix;
  
  // Finally, move all the matching children under the new prefix node
  for (scan=max_prefix_start; scan != NULL; scan=next)
    {
      next = scan->next_sibling;
      if (scan->label_length <= prefix_len)
        continue;
      if (scan->compare(max_prefix_start,prefix_len) != 0)
        break;
      scan->unlink();
      new_prefix->insert(scan,NULL);
    }
  new_prefix->child_info->check_new_prefix_possibilities = false;
     // Don't need to check these, because the prefix is known to be as
     // large as it possibly can be -- anyway, we were already in the process
     // of checking prefix possibilities when we came to creating this prefix.
  return true;
}


/*===========================================================================*/
/*                              kdws_catalog_node                            */
/*===========================================================================*/

/*****************************************************************************/
/*                    kdws_catalog_node::kdws_catalog_node                   */
/*****************************************************************************/

kdws_catalog_node::kdws_catalog_node(int catalog_type)
{
  
  item = NULL;
  flags = ((kdu_uint16) catalog_type) & KDWS_CATFLAG_TYPE_MASK;
  assert(flags == (kdu_uint16) catalog_type);
  label_hash = 0;
  label_length = 0;
  next_sibling = prev_sibling = parent = NULL;
  parent_entry = NULL;
  link_type = JPX_METANODE_LINK_NONE;
  child_info = NULL;
  cur_entry = NULL;
}

/*****************************************************************************/
/*                    kdws_catalog_node::~kdws_catalog_node                  */
/*****************************************************************************/

kdws_catalog_node::~kdws_catalog_node()
{
  if ((flags & KDWS_CATFLAG_HAS_METADATA) && (cur_entry != NULL))
    { // Delete all metadata entries
      while (cur_entry->prev != NULL)
        cur_entry = cur_entry->prev;
      kdws_catalog_entry *tmp;
      while ((tmp=cur_entry) != NULL)
        {
          cur_entry = tmp->next;
          delete tmp;
        }
    }
  else if ((!(flags & KDWS_CATFLAG_HAS_METADATA)) && (child_info != NULL))
    {
      delete child_info;
      child_info = NULL;
    }
  if (item != NULL)
    {
      item->release_catalog_node(this);
      item = NULL;
    }
}

/*****************************************************************************/
/*                      kdws_catalog_node::init (root)                       */
/*****************************************************************************/

void kdws_catalog_node::init()
{
  assert(((flags & ~KDWS_CATFLAG_TYPE_MASK) == 0) &&
         (label_length == 0) &&
         (child_info == NULL) && (parent == NULL));
  child_info = new kdws_catalog_child_info;
}

/*****************************************************************************/
/*                    kdws_catalog_node::init (metadata)                     */
/*****************************************************************************/

void kdws_catalog_node::init(jpx_metanode metanode)
{
  assert(((flags & ~KDWS_CATFLAG_TYPE_MASK) == 0) &&
         (cur_entry == NULL) && (label_length == 0));
  flags |= KDWS_CATFLAG_HAS_METADATA;
  jpx_metanode_link_type lnk_type;
  jpx_metanode lnk_tgt;
  const char *lbl =
    get_label_and_link_info_for_metanode(metanode,lnk_type,lnk_tgt);
  cur_entry = new kdws_catalog_entry;
  cur_entry->metanode = metanode;
  kdu_uint32 box_types[2] = {jp2_label_4cc,jp2_cross_reference_4cc};
  cur_entry->incomplete = !metanode.check_descendants_complete(2,box_types);
  cur_entry->container = this;
  change_link_info(lnk_type); // Must do this after `cur_entry' setup
  change_label(lbl); // Must do this after `change_link_info'
  metanode.set_state_ref(cur_entry);
}

/*****************************************************************************/
/*                   kdws_catalog_node::init (prefix child)                  */
/*****************************************************************************/

void kdws_catalog_node::init(kdws_catalog_node *ref, int prefix_len)
{
  assert(((flags & ~KDWS_CATFLAG_TYPE_MASK) == 0) &&
         (child_info == NULL) && (label_length == 0));
  child_info = new kdws_catalog_child_info;
  flags |= KDWS_CATFLAG_PREFIX_NODE;
  link_type = JPX_METANODE_LINK_NONE;
  if (prefix_len > KDWS_CATALOG_PREFIX_LEN)
    prefix_len = KDWS_CATALOG_PREFIX_LEN; // Should not happen
  label_length = prefix_len;
  int c;
  for (c=0; c < prefix_len; c++)
    label_prefix[c] = ref->label_prefix[c];
  label_prefix[c] = 0;
}

/*****************************************************************************/
/*                     kdws_catalog_node::change_label                       */
/*****************************************************************************/

void kdws_catalog_node::change_label(const char *new_lbl)
{
  assert(!(flags & KDWS_CATFLAG_PREFIX_NODE));
  kdu_uint32 new_lbl_hash = generate_label_hash(new_lbl);
  bool something_changed = (label_length != 0) && (new_lbl_hash != label_hash);
  this->label_hash = new_lbl_hash;
  
  // Now generate the unicode prefix
  label_length = (kdu_uint16)
    kdws_utf8_to_unicode(new_lbl,label_prefix,KDWS_CATALOG_PREFIX_LEN);
  
  // Now convert to upper case
  CharUpperBuffW((WCHAR *) label_prefix,label_length);

  if (something_changed)
    note_label_changed();
}

/*****************************************************************************/
/*                       kdws_catalog_node::get_label                        */
/*****************************************************************************/

const char *kdws_catalog_node::get_label()
{
  const char *result = NULL;
  if (flags & KDWS_CATFLAG_HAS_METADATA)
    {
      jpx_metanode_link_type ltype;
      jpx_metanode link_target;
      if (cur_entry != NULL)
        result = get_label_and_link_info_for_metanode(cur_entry->metanode,
                                                      ltype,link_target);
    }
  else
    { // Treat as a root node -- shouldn't use for prefix children
      int catalog_type = (flags & KDWS_CATFLAG_TYPE_MASK);
      if (catalog_type == KDWS_CATALOG_TYPE_INDEX)
        result = "Index Labels";
      else if (catalog_type == KDWS_CATALOG_TYPE_ENTITIES)
        result = "Image-Entity Labels";
      else if (catalog_type == KDWS_CATALOG_TYPE_REGIONS)
        result = "Region-Specific Labels";
    }
  if (result == NULL)
    result = "???";
  return result;
}

/*****************************************************************************/
/*                    kdws_catalog_node::change_link_info                    */
/*****************************************************************************/

void kdws_catalog_node::change_link_info(jpx_metanode_link_type lnk_type)
{
  assert(flags & KDWS_CATFLAG_HAS_METADATA);
  kdu_uint16 special_flags = 0;
  if (lnk_type == JPX_GROUPING_LINK)
    special_flags = (KDWS_CATFLAG_ANONYMOUS_ENTRIES |
                     KDWS_CATFLAG_COLLAPSED_DESCENDANTS); 
  else if (lnk_type != JPX_METANODE_LINK_NONE)
    { // See if we need to be anonymous
      assert(cur_entry != NULL);
      jpx_metanode par = cur_entry->metanode.get_parent();
      jpx_metanode par_link_tgt;
      jpx_metanode_link_type par_link_type;
      if ((!par.exists()) ||
          (get_label_and_link_info_for_metanode(par,par_link_type,
                                                par_link_tgt) == NULL) ||
          (par_link_type == JPX_GROUPING_LINK))
        special_flags = KDWS_CATFLAG_ANONYMOUS_ENTRIES;
    }
  
  bool something_changed = ((label_length != 0) &&
                            ((lnk_type != this->link_type) ||
                             (special_flags |=
                              (flags & (KDWS_CATFLAG_COLLAPSED_DESCENDANTS |
                                        KDWS_CATFLAG_ANONYMOUS_ENTRIES)))));
  this->link_type = lnk_type;
  this->flags &= ~(KDWS_CATFLAG_COLLAPSED_DESCENDANTS |
                   KDWS_CATFLAG_ANONYMOUS_ENTRIES);
  this->flags |= special_flags;
  if (something_changed)
    this->note_label_changed();
}

/*****************************************************************************/
/*                         kdws_catalog_node::insert                         */
/*****************************************************************************/

kdws_catalog_node *
  kdws_catalog_node::insert(kdws_catalog_node *child,
                            kdws_catalog_entry *entry)
{
  assert((child->parent == NULL) && (child->parent_entry == NULL) &&
         (child->next_sibling == NULL) && (child->prev_sibling == NULL));
  assert((entry == NULL) || (entry->container == this));
  if (flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS)
    assert(entry == this->cur_entry);
  
  kdws_catalog_child_info *info = NULL;
  if (flags & KDWS_CATFLAG_HAS_METADATA)
    {
      assert((entry != NULL) && (entry->container == this));
      info = &(entry->child_info);
    }
  else
    {
      assert(entry == NULL);
      info = child_info;
    }
  
  kdws_catalog_node *scan, *prev;
  if (child->flags & (KDWS_CATFLAG_NEEDS_EXPAND | KDWS_CATFLAG_NEEDS_COLLAPSE |
                      KDWS_CATFLAG_NEEDS_RELOAD |
                      KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION))
    { // New parent hierarchy all needs attention in the catalog view
      for (scan=this;
           ((scan != NULL) &&
            !(scan->flags & KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION));
           scan=scan->parent)
       scan->flags |= KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
    }

  for (prev=NULL, scan=info->children;
       scan != NULL; prev=scan, scan=scan->next_sibling)
    {
      if ((scan->flags & KDWS_CATFLAG_PREFIX_NODE) &&
          (child->compare(scan,scan->label_length) == 0))
        {
          if (child->label_length == scan->label_length)
            break; // Insert ourselves right before `scan'
          else
            return scan->insert(child,NULL);
        }
      int rel_order = child->compare(scan);
      if (rel_order < 0)
        break; // We come strictly before `scan' in alphabetic order
      else if (rel_order == 0)
        { // We have the same label as `scan'
          scan->import_entry_list(child); // This also deletes `child'
          return scan;
        }
    }
    
  if ((child->prev_sibling = prev) == NULL)
    info->children = child;
  else
    prev->next_sibling = child;
  if ((child->next_sibling = scan) != NULL)
    scan->prev_sibling = child;
  child->parent = this;
  child->parent_entry = entry;
  info->num_children++;
  note_children_increased(info);
  return child;
}

/*****************************************************************************/
/*                  kdws_catalog_node::import_entry_list                     */
/*****************************************************************************/

void kdws_catalog_node::import_entry_list(kdws_catalog_node *src)
{
  // Back up to the start of the `src' entry list
  while (src->cur_entry->prev != NULL)
    src->cur_entry = src->cur_entry->prev;
  
  // Find start of the `cur_entry' list
  kdws_catalog_entry *list = this->cur_entry;
  while (list->prev != NULL)
    list = list->prev;
  
  kdws_catalog_entry *entry;
  while ((entry = src->cur_entry) != NULL)
    {
      if (flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS)
        { // Cannot easily move descendants of `entry' across to the head
          // of the `list' because they might be embedded within prefix
          // children.  Simplest thing is to remove all the descendants of
          // `entry->metanode' from the catalog first and add them to the
          // metadata manager's touched list so that they get re-inserted
          // later on.
          remove_and_touch_descendants_of_metanode(entry->metanode);
          assert(entry->child_info.children == NULL);
        }
      
      // Remove `entry' from its `src' container
      if ((src->cur_entry = entry->next) != NULL)
        src->cur_entry->prev = NULL;
      entry->next = entry->prev = NULL;
      
      // Decide where to insert `entry' within the current list
      kdws_catalog_entry *scan, *prev;
      if (flags & (KDWS_CATFLAG_COLLAPSED_DESCENDANTS |
                   KDWS_CATFLAG_ANONYMOUS_ENTRIES))
        { // Insert immediately after the head of the list
          prev = list; scan = list->next;
        }
      else
        {
          kdu_long idx = entry->metanode.get_sequence_index();
          for (scan=list, prev=NULL; scan != NULL; prev=scan, scan=scan->next)
            if (scan->metanode.get_sequence_index() > idx)
              break; // We should be inserted immediately before `scan'
        }
      if ((entry->next = scan) != NULL)
        scan->prev = entry;
      if ((entry->prev = prev) != NULL)
        prev->next = entry;
      entry->container = this;
    }
  
  this->note_label_changed(); // Because there are more entries
  delete src;
}

/*****************************************************************************/
/*                         kdws_catalog_node::unlink                         */
/*****************************************************************************/

void kdws_catalog_node::unlink()
{
  if (parent == NULL)
    return; // Already unlinked
  kdws_catalog_child_info *parent_info=parent->child_info;
  if (parent->flags & KDWS_CATFLAG_HAS_METADATA)
    {
      assert(parent_entry != NULL);
      parent_info = &(parent_entry->child_info);
    }  
  
  if (prev_sibling == NULL)
    {
      assert(this == parent_info->children);
      parent_info->children = next_sibling;
    }
  else
    prev_sibling->next_sibling = next_sibling;
  if (next_sibling != NULL)
    next_sibling->prev_sibling = prev_sibling;
  parent_info->num_children--;
  parent->note_children_decreased(parent_info);

  if ((parent->flags & KDWS_CATFLAG_PREFIX_NODE) &&
      (parent_info->num_children < 2) && (parent->parent != NULL))
    { // Collapse parent prefix node -- this may be recursive
      kdws_catalog_node *last_child = parent_info->children;
      if (last_child != NULL)
        {
          kdws_catalog_node *new_parent = parent->parent;
          kdws_catalog_entry *new_parent_entry = parent->parent_entry;
          kdws_catalog_child_info *new_parent_info = new_parent->child_info;
          if (new_parent->flags & KDWS_CATFLAG_HAS_METADATA)
            {
              assert(new_parent_entry != NULL);
              new_parent_info = &(new_parent_entry->child_info);
            }
          last_child->prev_sibling = parent;
          if ((last_child->next_sibling = parent->next_sibling) != NULL)
            last_child->next_sibling->prev_sibling = last_child;
          parent->next_sibling = last_child;
          new_parent_info->num_children++;
          last_child->parent = new_parent;
          last_child->parent_entry = new_parent_entry;
          parent_info->children = NULL;
          parent_info->num_children = 0;
        }
      assert(parent_info->children == NULL);
      parent->unlink();
      delete parent;
    }

  prev_sibling = next_sibling = parent = NULL;
  parent_entry = NULL;
}

/*****************************************************************************/
/*                     kdws_catalog_node::extract_entry                      */
/*****************************************************************************/

kdws_catalog_node *
  kdws_catalog_node::extract_entry(kdws_catalog_entry *entry)
{
  assert(entry->container == this);

  bool was_first_entry = (entry->prev == NULL);
  bool was_collapsed = (flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS) &&
    ((entry->prev != NULL) || (entry->next != NULL));
  if (was_collapsed)
    remove_and_touch_descendants_of_metanode(entry->metanode);

  // First unlink `entry' from the list of entries for this node
  if (entry->prev != NULL)
    entry->prev->next = entry->next;
  if (entry->next != NULL)
    entry->next->prev = entry->prev;
  if ((entry == cur_entry) && ((cur_entry = entry->prev) == NULL))
    cur_entry = entry->next;
  if (cur_entry != NULL)
    this->note_label_changed(); // Otherwise the caller should unlink us
  
  // Now create a new container
  kdws_catalog_node *new_container =
    new kdws_catalog_node(this->flags & KDWS_CATFLAG_TYPE_MASK);
  new_container->flags = this->flags;
  new_container->cur_entry = entry;
  entry->next = entry->prev = NULL;
  new_container->change_link_info(this->link_type);
  new_container->label_hash = this->label_hash;
  new_container->label_length = this->label_length;
  memcpy(new_container->label_prefix,this->label_prefix,
         sizeof(kdu_uint16)*(KDWS_CATALOG_PREFIX_LEN+1));
  entry->container = new_container;
  
  // Now adjust the descendants
  kdws_catalog_node *child;
  if (!was_collapsed)
    { // Sufficient to adjust the `parent' members of each child
      for (child=entry->child_info.children;
           child != NULL; child=child->next_sibling)
        child->parent = new_container;
    }
  else if (was_first_entry)
    { // All children were previously collapsed under `entry'.  Have to
      // move them all back across to `cur_entry' in the current node.
      assert(cur_entry->child_info.children == NULL);
      cur_entry->child_info = entry->child_info;
      entry->child_info.init();
      for (child=cur_entry->child_info.children;
           child != NULL; child=child->next_sibling)
        {
          assert(child->parent_entry == entry);
          child->parent_entry = cur_entry;
        }
    }
  else
    { // All children are still collapsed under the `cur_entry' in the
      // current object.  Shouldn't have to do anything.
      assert(entry->child_info.children == NULL);
      entry->child_info.init(); // Just to be sure
    }
  
  return new_container;
}

/*****************************************************************************/
/*                 kdws_catalog_node::note_children_decreased                */
/*****************************************************************************/

void
  kdws_catalog_node::note_children_decreased(kdws_catalog_child_info *info)
{
  bool needs_attention = false;
  if (!(flags & KDWS_CATFLAG_HAS_METADATA))
    {
      assert(info == this->child_info);
    }
  else if (info != &(cur_entry->child_info))
    {
      assert(!(flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS));
      return;
    }

  bool can_expand = (info->num_children > 0);
  if (flags & KDWS_CATFLAG_IS_EXPANDED)
    {
      needs_attention = true;
      flags |= KDWS_CATFLAG_NEEDS_COLLAPSE;
      if (can_expand)
        flags |= KDWS_CATFLAG_NEEDS_EXPAND;
    }
  
  if ((!can_expand) && ((parent == NULL) ||
                        (parent->flags & KDWS_CATFLAG_IS_EXPANDED)))
    { // Expandability has changed
      needs_attention = true;
      flags |= KDWS_CATFLAG_NEEDS_RELOAD;
    }
  if (needs_attention)
    for (kdws_catalog_node *scan=parent;
         ((scan != NULL) &&
          !(scan->flags & KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION));
         scan=scan->parent)
      scan->flags |= KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
}

/*****************************************************************************/
/*                 kdws_catalog_node::note_children_increased                */
/*****************************************************************************/

void
  kdws_catalog_node::note_children_increased(kdws_catalog_child_info *info)
{
  if (info->num_children > 2)
    { 
      kdws_catalog_node *node_scan = this;
      kdws_catalog_child_info *info_scan=info;
      while (!info_scan->check_new_prefix_possibilities)
        {
          info_scan->check_new_prefix_possibilities = true;
          if (node_scan->parent_entry != NULL)
            info_scan = &(node_scan->parent_entry->child_info);
          else if (node_scan->parent == NULL)
            break;
          else
            info_scan = node_scan->parent->child_info;
          node_scan = node_scan->parent;
        }
    }

  if (!(flags & KDWS_CATFLAG_HAS_METADATA))
    {
      assert(info == this->child_info);
    }
  else if (info != &(cur_entry->child_info))
    {
      assert(!(flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS));
      return;
    }
  bool needs_attention = false;
  if (flags & KDWS_CATFLAG_IS_EXPANDED)
    {
      flags |= (KDWS_CATFLAG_NEEDS_EXPAND | KDWS_CATFLAG_NEEDS_COLLAPSE |
                KDWS_CATFLAG_NEEDS_RELOAD);
      needs_attention = true;
    }
  else if (flags & KDWS_CATFLAG_WANTS_EXPAND)
    {
      if ((parent != NULL) && (parent->flags & KDWS_CATFLAG_IS_EXPANDED))
        {
          flags |= (KDWS_CATFLAG_NEEDS_EXPAND | KDWS_CATFLAG_NEEDS_RELOAD);
          needs_attention = true;
        }
      else
        flags &= ~KDWS_CATFLAG_WANTS_EXPAND; // Flag should not have been set
    }
  if ((info->num_children == 1) &&
      ((parent == NULL) || (parent->flags & KDWS_CATFLAG_IS_EXPANDED)))
    {
      flags |= KDWS_CATFLAG_NEEDS_RELOAD;
      needs_attention = true;
    }
  if (!needs_attention)
    return;
  for (kdws_catalog_node *scan=parent;
       (scan != NULL) &&
       !(scan->flags & KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION);
       scan=scan->parent)
    scan->flags |= KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
}

/*****************************************************************************/
/*                   kdws_catalog_node::note_label_changed                   */
/*****************************************************************************/

void
  kdws_catalog_node::note_label_changed()
{
  if ((parent == NULL) || (parent->flags & KDWS_CATFLAG_IS_EXPANDED))
    {
      flags |= KDWS_CATFLAG_NEEDS_RELOAD;
      for (kdws_catalog_node *scan=parent;
           (scan != NULL) &&
           !(scan->flags & KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION);
           scan=scan->parent)
        scan->flags |= KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
    }
}

/*****************************************************************************/
/*                    kdws_catalog_node::is_expandable                       */
/*****************************************************************************/

bool kdws_catalog_node::is_expandable()
{
  bool result = false;
  if (!(flags & KDWS_CATFLAG_HAS_METADATA))
    result = (child_info->num_children > 0);
  else if (cur_entry->child_info.num_children > 0)
    result = true;
  else if (!(flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS))
    result = cur_entry->incomplete;
  else
    { // Need to see if any of the collapsed entries are incomplete
      kdws_catalog_entry *scan;
      for (scan=cur_entry; scan != NULL; scan=scan->next)
        if (scan->incomplete)
          { result = true; break; }
    }
  return result;
}

/*****************************************************************************/
/*                   kdws_catalog_node::reload_tree_item                     */
/*****************************************************************************/

void
  kdws_catalog_node::reload_tree_item(kdws_catalog *catalog)
{
  kdu_uint16 unicode_buf[257];
  int num_unichars = 0;
  int max_unichars = 256;
  if (catalog->tree_supports_true_unichars)
    unicode_buf[num_unichars++] = 0xFEFF; // Byte Order Mark

  const char *label_string = NULL;
  if (flags & KDWS_CATFLAG_HAS_METADATA)
    {
      int entry_idx = 0;
      kdws_catalog_entry *entry;
      for (entry=cur_entry; entry->prev != NULL; entry=entry->prev)
        entry_idx++;
      int entry_count = entry_idx+1;
      for (entry=cur_entry; entry->next != NULL; entry=entry->next)
        entry_count++;
      label_string = get_label();
      if (entry_count > 1)
        {
          char prefix_buf[20];
          if (flags & KDWS_CATFLAG_ANONYMOUS_ENTRIES)
            {
              sprintf(prefix_buf,"(%d) ",entry_count);
              num_unichars +=
                kdws_utf8_to_unicode(prefix_buf,unicode_buf+num_unichars,200);
            }
          else
            {
              sprintf(prefix_buf,"%d",entry_idx+1);
              num_unichars +=
                kdws_utf8_to_unicode(prefix_buf,unicode_buf+num_unichars,200);
              if (catalog->tree_supports_true_unichars)
                unicode_buf[num_unichars++] = 0x21A4;
              sprintf(prefix_buf,"(%d) ",entry_count);
              num_unichars +=
                kdws_utf8_to_unicode(prefix_buf,unicode_buf+num_unichars,200);
            }
        }
      if (catalog->tree_supports_true_unichars)
        {
          if (link_type == JPX_ALTERNATE_PARENT_LINK)
            unicode_buf[num_unichars++] = 0x2196;
          else if (link_type == JPX_ALTERNATE_CHILD_LINK)
            unicode_buf[num_unichars++] = 0x2198;
        }
    }
  else if (flags & KDWS_CATFLAG_PREFIX_NODE)
    {
      for (kdu_uint16 c=0; c < label_length; c++)
        unicode_buf[num_unichars++] = label_prefix[c];
      unicode_buf[num_unichars++] = '.';
      unicode_buf[num_unichars++] = '.';
      unicode_buf[num_unichars++] = '.';
    }
  else
    { // Should be root node
      label_string = get_label();
    }
  if (label_string != NULL)
    num_unichars +=
      kdws_utf8_to_unicode(label_string,unicode_buf+num_unichars,
                           max_unichars-num_unichars);
  unicode_buf[num_unichars] = 0;

  TVINSERTSTRUCTW params;
  memset(&params,0,sizeof(params));
  params.item.mask = TVIF_TEXT | TVIF_CHILDREN;
  params.item.cChildren = (this->is_expandable())?1:0;
  params.item.pszText = (WCHAR *) unicode_buf;
  if (item == NULL)
    {
      item = new kdws_catalog_item(this);
      params.item.mask |= TVIF_PARAM;
      params.item.lParam = (LPARAM) item;
      if (parent == NULL)
        params.hParent = TVI_ROOT;
      else
        params.hParent = parent->item->get_tree_item();
      if (next_sibling == NULL)
        params.hInsertAfter = TVI_LAST;
      else if (prev_sibling == NULL)
        params.hInsertAfter = TVI_FIRST;
      else
        params.hInsertAfter = prev_sibling->item->get_tree_item();
      HTREEITEM tree_item_handle = (HTREEITEM)
        catalog->tree_ctrl.SendMessage(TVM_INSERTITEMW,0,(LPARAM)&params);
      item->set_tree_item(tree_item_handle);
    }
  else
    {
      params.item.mask |= TVIF_HANDLE;
      params.item.hItem = item->get_tree_item();
      catalog->tree_ctrl.SendMessage(TVM_SETITEMW,0,(LPARAM)&(params.item));
    }

  flags &= ~KDWS_CATFLAG_NEEDS_RELOAD;
}

/*****************************************************************************/
/*                 kdws_catalog_node::reflect_changes_part1                  */
/*****************************************************************************/

void
  kdws_catalog_node::reflect_changes_part1(kdws_catalog *catalog)
{
  if (flags & KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION)
    {
      kdws_catalog_node *child;
      if (flags & KDWS_CATFLAG_HAS_METADATA)
        child = cur_entry->child_info.children;
      else
        child = child_info->children;
      bool cancel_reloads =
        (((flags & KDWS_CATFLAG_NEEDS_COLLAPSE) ||
          (!(flags & KDWS_CATFLAG_IS_EXPANDED))) &&
         !(flags & KDWS_CATFLAG_NEEDS_EXPAND));
      for (; child != NULL; child=child->next_sibling)
        {
          if (cancel_reloads)
            child->flags &= ~KDWS_CATFLAG_NEEDS_RELOAD;
          child->reflect_changes_part1(catalog);
        }
    }
  if (flags & KDWS_CATFLAG_NEEDS_COLLAPSE)
    {
      flags &= ~KDWS_CATFLAG_WANTS_EXPAND;
      catalog->tree_ctrl.Expand(item->get_tree_item(),
                                TVE_COLLAPSE|TVE_COLLAPSERESET);
      if (flags & KDWS_CATFLAG_IS_EXPANDED)
        { // The TVN_ITEMEXPANDED notification was not sent -- often happens,
          // contrary to all Windows API documentation I can find.
          catalog->perform_collapse_processing_for_node(this);
        }
    }
  flags &= ~KDWS_CATFLAG_NEEDS_COLLAPSE;
}

/*****************************************************************************/
/*                 kdws_catalog_node::reflect_changes_part2                  */
/*****************************************************************************/

void
  kdws_catalog_node::reflect_changes_part2(kdws_catalog *catalog)
{
  if (flags & KDWS_CATFLAG_NEEDS_RELOAD)
    {
      if ((parent == NULL) || (parent->flags & KDWS_CATFLAG_IS_EXPANDED))
        this->reload_tree_item(catalog); // Above test performed just in case
      flags &= ~KDWS_CATFLAG_NEEDS_RELOAD;
    }
  if (flags & KDWS_CATFLAG_NEEDS_EXPAND)
    {
      if (item == NULL)
        reload_tree_item(catalog);
      catalog->tree_ctrl.Expand(item->get_tree_item(),TVE_EXPAND);
      if (!(flags & KDWS_CATFLAG_IS_EXPANDED))
        { // Just in case the TVN_ITEMEXPANDED notification was not sent
          catalog->assign_tree_items_to_expanding_node_children(this);
          catalog->perform_expand_processing_for_node(this);
          assert(flags & KDWS_CATFLAG_IS_EXPANDED);
        }
      flags &= ~KDWS_CATFLAG_NEEDS_EXPAND;
    }
  if (flags & KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION)
    {
      kdws_catalog_node *child;
      if (flags & KDWS_CATFLAG_HAS_METADATA)
        child = cur_entry->child_info.children;
      else
        child = child_info->children;
      for (; child != NULL; child=child->next_sibling)
        child->reflect_changes_part2(catalog);
    }
  flags &= ~KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
}

/*****************************************************************************/
/*                        kdws_catalog_node::compare                         */
/*****************************************************************************/

int kdws_catalog_node::compare(kdws_catalog_node *ref, int num_chars)
{
  bool full_compare = ((num_chars == 0) &&
                       (flags & KDWS_CATFLAG_HAS_METADATA) &&
                       (ref->flags & KDWS_CATFLAG_HAS_METADATA));
  if ((num_chars <= 0) || (num_chars > KDWS_CATALOG_PREFIX_LEN))
    num_chars = KDWS_CATALOG_PREFIX_LEN;
  for (int n=0; n < num_chars; n++)
    if (label_prefix[n] < ref->label_prefix[n])
      return -1;
    else if (label_prefix[n] > ref->label_prefix[n])
      return 1;
    else if (label_prefix[n] == 0)
      break;
  
  if (!full_compare)
    return 0;
  const char *label = this->get_label();
  const char *ref_label = ref->get_label();
  int rel_order = (int) strcmp(label,ref_label);
  if (rel_order != 0)
    return rel_order;
  if (link_type != ref->link_type)
    return (link_type < ref->link_type)?-1:1;
  if (link_type == JPX_GROUPING_LINK)
    {
      jpx_metanode_link_type tmp;
      jpx_metanode link_target = cur_entry->metanode.get_link(tmp);
      jpx_metanode ref_link_target = ref->cur_entry->metanode.get_link(tmp);
      if (link_target != link_target)
        {
          if (!link_target)
            return -1;
          if (!ref_link_target)
            return 1;
          kdu_long seq_idx = link_target.get_sequence_index();
          kdu_long ref_seq_idx = ref_link_target.get_sequence_index();
          return (seq_idx < ref_seq_idx)?-1:1;
        }
    }
  return 0;
}

/*****************************************************************************/
/*             kdws_catalog_node::find_best_collapse_candidate               */
/*****************************************************************************/

#define KDWS_MAX_AUTO_COLLAPSE_HISTORY 20

kdws_catalog_node *
  kdws_catalog_node::find_best_collapse_candidate(int depth,
                                        kdws_catalog_selection *history,
                                        int &most_recent_use)
{
  if (depth == 0)
    return NULL;
  kdws_catalog_node *node;
  if (flags & KDWS_CATFLAG_HAS_METADATA)
    node = cur_entry->child_info.children;
  else
    node = child_info->children;
  kdws_catalog_node *best_node = NULL;
  for (; (node != NULL) && (most_recent_use < KDWS_MAX_AUTO_COLLAPSE_HISTORY);
       node = node->next_sibling)
    {
      if (node->flags & KDWS_CATFLAG_NEEDS_EXPAND)
        continue;
      if (!(node->flags & KDWS_CATFLAG_IS_EXPANDED))
        continue;
      if (depth > 1)
        { // Need to go further down
          kdws_catalog_node *candidate =
            node->find_best_collapse_candidate(depth-1,history,
                                               most_recent_use);
          if (candidate != NULL)
            best_node = candidate;
        }
      else
        { // We are at the right level; consider candidates here
          if (node->flags & KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION)
            continue;
          kdws_catalog_selection *hscan = history;
          int selection_age = 0;
          for (; (hscan != NULL) && (hscan->prev != NULL) &&
                 (selection_age < KDWS_MAX_AUTO_COLLAPSE_HISTORY);
               selection_age++, hscan=hscan->prev)
            { // See if `hscan' uses `node'
              kdws_catalog_entry *entry =
                get_entry_for_metanode(hscan->metanode);
              kdws_catalog_node *user;
              if ((entry == NULL) || ((user=entry->container) == NULL))
                continue;
              for (user=user->parent; user != NULL; user=user->parent)
                if (user == node)
                  break;
              if (user != NULL)
                break; // `hscan' depends on `node' being expanded
            }
          if (selection_age > most_recent_use)
            {
              best_node = node;
              most_recent_use = selection_age;
            }
        }
    }
  return best_node;  
}

/*****************************************************************************/
/*          kdws_catalog_node::needs_client_requests_if_expanded             */
/*****************************************************************************/

bool kdws_catalog_node::needs_client_requests_if_expanded()
{
  if (parent == NULL)
    return true;
  if (flags & KDWS_CATFLAG_HAS_METADATA)
    {
      kdws_catalog_entry *entry = cur_entry;
      for (; entry != NULL; entry=entry->next)
        {
          if (entry->incomplete)
            return true;
          if (!(flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS))
            break;
        }
    }
  kdws_catalog_child_info *info = child_info;
  if (flags & KDWS_CATFLAG_HAS_METADATA)
    info = &(cur_entry->child_info);
  kdws_catalog_node *child;
  jpx_metanode_link_type child_link_type;
  for (child=info->children; child != NULL; child=child->next_sibling)
    if ((child->flags & KDWS_CATFLAG_HAS_METADATA) &&
        (child->link_type != JPX_METANODE_LINK_NONE) &&
        !child->cur_entry->metanode.get_link(child_link_type))
      return true;
  return false;
}

/*****************************************************************************/
/*               kdws_catalog_node::generate_client_requests                 */
/*****************************************************************************/

int kdws_catalog_node::generate_client_requests(kdu_window *client_window)
{
  int num_metareqs = 0;
  kdu_uint32 box_types[4] = {jp2_label_4cc,jp2_cross_reference_4cc,
                             jp2_number_list_4cc,jp2_roi_description_4cc};
  kdu_uint32 recurse_types[2] = {jp2_number_list_4cc,jp2_roi_description_4cc};
  if ((flags & KDWS_CATFLAG_HAS_METADATA) && (cur_entry != NULL))
    {
      kdws_catalog_entry *scan;
      if (link_type != JPX_METANODE_LINK_NONE)
        {
          for (scan=cur_entry; scan != NULL; scan=scan->prev)
            num_metareqs +=
              scan->metanode.generate_link_metareq(client_window,4,box_types,
                                                   2,recurse_types,true);
          for (scan=cur_entry->next; scan != NULL; scan=scan->next)
            num_metareqs +=
              scan->metanode.generate_link_metareq(client_window,4,box_types,
                                                   2,recurse_types,true);
        }
      if (this->flags & (KDWS_CATFLAG_IS_EXPANDED|KDWS_CATFLAG_WANTS_EXPAND))
        for (scan=cur_entry; scan != NULL; scan=scan->next)
          {
            num_metareqs +=
              scan->metanode.generate_metareq(client_window,4,box_types,
                                              2,recurse_types,true);
            if (!(flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS))
              break;
          }
    }

  if (this->flags & KDWS_CATFLAG_IS_EXPANDED)
    {
      kdws_catalog_node *child;
      if (flags & KDWS_CATFLAG_HAS_METADATA)
        child = cur_entry->child_info.children;
      else
        child = child_info->children;
      for (; child != NULL; child=child->next_sibling)
        num_metareqs +=
          child->generate_client_requests(client_window);
    }
  
  return num_metareqs;
}


/*===========================================================================*/
/*                             kdws_tree_ctrl                                */
/*===========================================================================*/

/*****************************************************************************/
/*              kdws_tree_ctrl::find_item_by_walking_downwards               */
/*****************************************************************************/

HTREEITEM
  kdws_tree_ctrl::find_item_by_walking_downwards(HTREEITEM start,
                                                 int distance)
{
  HTREEITEM scan;
  for (; distance > 0; distance--)
    { // try to advance `start' one row downwards
      scan = GetNextSiblingItem(start);
      if (scan == NULL)
        {
          HTREEITEM parent=start;
          while ((parent = GetParentItem(parent)) != NULL)
            if ((scan = GetNextSiblingItem(parent)) != NULL)
              break;
          if (scan == NULL)
            break; // No more items to be found
          else
            { // The parent's next sibling is the next row
              start = scan;
              continue;
            }
        }
      start = scan;
      while ((scan = GetChildItem(start)) != NULL)
        start = scan;
    }
  return start;
}

/*****************************************************************************/
/*               kdws_tree_ctrl::find_item_by_walking_upwards                */
/*****************************************************************************/

HTREEITEM
  kdws_tree_ctrl::find_item_by_walking_upwards(HTREEITEM start,
                                               int distance)
{
  HTREEITEM scan;
  for (; distance > 0; distance--)
    { // try to advance `start' one row upwards
      scan = GetPrevSiblingItem(start);
      if (scan == NULL)
        {
          scan = GetParentItem(start);
          if (scan == NULL)
            break; // No more items to be found
          else
            { // The parent is the previous row
              start = scan;
              continue;
            }
        }
      start = scan;
      while ((scan=GetChildItem(start)) != NULL)
        { // Move `start' to its last child
          start = scan;
          while ((scan=GetNextSiblingItem(start)) != NULL)
            start = scan;
        }
    }
  return start;
}

/*****************************************************************************/
/*                    kdws_tree_ctrl::get_visible_location                   */
/*****************************************************************************/

int kdws_tree_ctrl::get_visible_location(HTREEITEM item)
{
  int idx = 0;
  HTREEITEM scan = GetFirstVisibleItem();
  int visible_rows = GetVisibleCount();
  for (; (scan != NULL) && (visible_rows > 0);
       scan=GetNextVisibleItem(scan), idx++, visible_rows--)
    if (scan == item)
      return idx;
  return -1;
}

/*****************************************************************************/
/*             kdws_tree_ctrl::scroll_item_to_visible_location               */
/*****************************************************************************/

void kdws_tree_ctrl::scroll_item_to_visible_location(HTREEITEM item, int pos)
{
  if (item == NULL)
    return;
  int visible_pos = get_visible_location(item);
  if (visible_pos < 0)
    {
      EnsureVisible(item);
      visible_pos = get_visible_location(item);
      if (visible_pos < 0)
        return; // Should not be possible
    }
  int visible_rows = GetVisibleCount();
  int target_pos = pos;
  if ((pos < 0) || (pos >= visible_rows))
    { // Choose closest position in the central third of the display
      int middle_start = visible_rows/3;
      int middle_lim = (2*visible_rows)/3;
      if (visible_pos < middle_start)
        target_pos = middle_start;
      else if (visible_pos >= middle_lim)
        target_pos = middle_lim;
      else
        target_pos = visible_pos;
    }
  if (target_pos < visible_pos)
    {
      HTREEITEM new_last_visible_item =
        find_item_by_walking_downwards(item,(visible_rows-1-target_pos));
      EnsureVisible(new_last_visible_item);
    }
  else if (target_pos > visible_pos)
    {
      HTREEITEM new_first_visible_item =
        find_item_by_walking_upwards(item,target_pos);
      EnsureVisible(new_first_visible_item);
    }
}

/*****************************************************************************/
/*                    kdws_tree_ctrl::PreSubclassWindow                      */
/*****************************************************************************/

void kdws_tree_ctrl::PreSubclassWindow()
{
  CTreeCtrl::PreSubclassWindow();
  EnableToolTips(TRUE);
}

/*****************************************************************************/
/*                      kdws_tree_ctrl::OnToolHitTest                        */
/*****************************************************************************/

INT_PTR kdws_tree_ctrl::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
  RECT rect;
  UINT nFlags;
  HTREEITEM hitem = HitTest(point,&nFlags);
  if(nFlags & TVHT_ONITEMLABEL)
    {
      GetItemRect( hitem, &rect, TRUE );
      pTI->hwnd = m_hWnd;
      pTI->uId = (UINT_PTR)hitem;
      pTI->lpszText = LPSTR_TEXTCALLBACK;
      pTI->rect = rect;
      return pTI->uId;
    }
  return -1;
}


/*****************************************************************************/
/*                   Message Handlers for kdws_tree_ctrl                     */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_tree_ctrl, CTreeCtrl)
  ON_WM_SETFOCUS()
  ON_WM_KILLFOCUS()
  ON_WM_KEYDOWN()
  ON_WM_KEYUP()
  ON_WM_CHAR()
  ON_WM_LBUTTONDBLCLK()
  ON_WM_LBUTTONDOWN()
  ON_WM_RBUTTONDOWN()
  ON_NOTIFY_EX_RANGE(TTN_GETDISPINFOA, 0, 0xFFFF, get_tooltip_dispinfo)
  ON_NOTIFY_EX_RANGE(TTN_GETDISPINFOW, 0, 0xFFFF, get_tooltip_dispinfo)
END_MESSAGE_MAP()

/*****************************************************************************/
/*                       kdws_tree_ctrl::OnSetFocus                          */
/*****************************************************************************/

void kdws_tree_ctrl::OnSetFocus(CWnd *pOldWnd)
{
  has_focus = true;
  catalog->reveal_focus();
}

/*****************************************************************************/
/*                       kdws_tree_ctrl::OnKillFocus                         */
/*****************************************************************************/

void kdws_tree_ctrl::OnKillFocus(CWnd *pNewWnd)
{
  has_focus = false;
  catalog->reveal_focus();
}

/*****************************************************************************/
/*                        kdws_tree_ctrl::OnKeyDown                          */
/*****************************************************************************/

void kdws_tree_ctrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  handled_char = 0;
  if (catalog->user_key_down(nChar))
    handled_char = nChar;
  else
    CTreeCtrl::OnKeyDown(nChar,nRepCnt,nFlags);
}

/*****************************************************************************/
/*                         kdws_tree_ctrl::OnKeyUp                           */
/*****************************************************************************/

void kdws_tree_ctrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  if (nChar == handled_char)
    handled_char = 0;
  else
    CTreeCtrl::OnKeyUp(nChar,nRepCnt,nFlags);
}

/*****************************************************************************/
/*                         kdws_tree_ctrl::OnChar                            */
/*****************************************************************************/

void kdws_tree_ctrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  if (nChar != handled_char)
    CTreeCtrl::OnKeyUp(nChar,nRepCnt,nFlags);
}

/*****************************************************************************/
/*                     kdws_tree_ctrl::OnLButtonDblClk                       */
/*****************************************************************************/

void kdws_tree_ctrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
  HTREEITEM tree_item = this->HitTest(point,NULL);
  if ((tree_item != NULL) && (tree_item == this->GetSelectedItem()))
    catalog->user_double_click();
}

/*****************************************************************************/
/*                      kdws_tree_ctrl::OnLButtonDown                        */
/*****************************************************************************/

void kdws_tree_ctrl::OnLButtonDown(UINT nFlags, CPoint point)
{
  if (!(nFlags & MK_CONTROL))
    {
      CTreeCtrl::OnLButtonDown(nFlags,point);
      return;
    }
  OnRButtonDown(nFlags,point);
}

/*****************************************************************************/
/*                      kdws_tree_ctrl::OnRButtonDown                        */
/*****************************************************************************/

void kdws_tree_ctrl::OnRButtonDown(UINT nFlags, CPoint point)
{
  HTREEITEM tree_item = this->HitTest(point,NULL);
  if (tree_item != NULL)
    {
      kdws_catalog_item *item = (kdws_catalog_item *) GetItemData(tree_item);
      if ((item == NULL) || (item->get_tree_item() != tree_item))
        return;
      catalog->user_right_click(item->get_node());
    }
}

/*****************************************************************************/
/*                  kdws_tree_ctrl::get_tooltip_dispinfo                     */
/*****************************************************************************/

BOOL kdws_tree_ctrl::get_tooltip_dispinfo(UINT id, NMHDR* pNMHDR,
                                          LRESULT *pResult)
{
  UINT_PTR nID = pNMHDR->idFrom;
  NMTTDISPINFOA *pTTTA = (NMTTDISPINFOA *) pNMHDR;
  NMTTDISPINFOW *pTTTW = (NMTTDISPINFOW *) pNMHDR;
  bool use_wide = pNMHDR->code == TTN_GETDISPINFOW;
  bool use_ansii = pNMHDR->code == TTN_GETDISPINFOA;
  if (!(use_ansii || use_wide))
    return FALSE;

  // Avoid processing messages coming from the tree-control's built-in tooltip
  if (use_ansii && (pTTTA->uFlags & TTF_IDISHWND) && (m_hWnd == (HWND) nID))
    return FALSE;
  if (use_wide && (pTTTW->uFlags & TTF_IDISHWND) && (m_hWnd == (HWND) nID))
    return FALSE;

  // Get the tree item
  const MSG* pMessage;
  CPoint pt;
  pMessage = GetCurrentMessage(); // get mouse pos
  ASSERT ( pMessage );
  pt = pMessage->pt;
  ScreenToClient( &pt );
  UINT nFlags;
  HTREEITEM tree_item = HitTest( pt, &nFlags );
  if (tree_item == NULL)
    return FALSE;
  kdws_catalog_item *item = (kdws_catalog_item *) GetItemData(tree_item);
  if (item == NULL)
    return FALSE;
  assert(tree_item == item->get_tree_item());
  kdws_catalog_node *node = item->get_node();

  // Now set the text
  if (use_ansii)
    {
      pTTTA->hinst = NULL;
      pTTTA->lpszText = NULL;
            if ((node->parent == NULL) &&
          ((node->flags & 0x000F) == KDWS_CATALOG_TYPE_INDEX))
        pTTTA->lpszText =
          "Catalogs labels and links which are not associated with "
          "specific codestreams, compositing layers, or the rendered "
          "result -- i.e., metadata which is not grouped under a JPX "
          "number-list (nlst) box.\n"
          "   It is reasonable to regard these as "
          "global index information.  Links from a global index into "
          "image entities or regions can be particularly useful.";
      else if ((node->parent == NULL) &&
               ((node->flags & 0x000F) == KDWS_CATALOG_TYPE_ENTITIES))
        pTTTA->lpszText =
          "Catalogs labels and links which are associated with "
          "specific codestreams, compositing layers, or the rendered "
          "result, but not with specific image regions.\n"
          "   Double-click items in this catalog (or press ENTER) to move "
          "the image view to display the most closely related content which "
          "involves the codestreams and/or compositing layers associated "
          "with that item.";
      else if ((node->parent == NULL) &&
               ((node->flags & 0x000F) == KDWS_CATALOG_TYPE_REGIONS))
        pTTTA->lpszText =
          "Catalogs labels and links which are associated with "
          "specific spatial regions.\n"
          "   Double-click items in this catalog (or press ENTER) to move the "
          "image view to display the relevant imagery and also adjust the "
          "focus box to surround the region of interest.";
      else if (node->flags & KDWS_CATFLAG_HAS_METADATA)
        {
          if (node->link_type != JPX_METANODE_LINK_NONE)
            pTTTA->lpszText =
            "Double-click links (or press ENTER) to move the selection to "
              "the link target, if possible.\n"
              "   Right/ctrl-click to edit\n"
              "   Arrow keys move selection between and within items\n"
              "   Del/Backspace to delete (if editor is closed)\n"
              "   ctrl-R to limit region-of-interest overlays to "
              "those which are semantically connected to this link only";
          else
            pTTTA->lpszText =
              "Double-click (or press ENTER) to dispay the relevant imagery "
              "and adjust the focus box to surround the region of "
              "interest.\n"
              "   Right/ctrl-click to open the editor at the link source\n"
              "   Arrow keys move selection between and within items\n"
              "   Del/Backspace to delete the link (if editor is closed)\n"
              "   ctrl-R to limit region-of-interest overlays to "
              "those which are semantically connected to this item only.";
        }
    }
  else if (use_wide)
    {
      pTTTW->hinst = NULL;
      pTTTW->lpszText = NULL;
      if ((node->parent == NULL) &&
          ((node->flags & 0x000F) == KDWS_CATALOG_TYPE_INDEX))
        pTTTW->lpszText =
          L"Catalogs labels and links which are not associated with "
          L"specific codestreams, compositing layers, or the rendered "
          L"result -- i.e., metadata which is not grouped under a JPX "
          L"number-list (nlst) box.\n"
          L"   It is reasonable to regard these as "
          L"global index information.  Links from a global index into "
          L"image entities or regions can be particularly useful.";
      else if ((node->parent == NULL) &&
               ((node->flags & 0x000F) == KDWS_CATALOG_TYPE_ENTITIES))
        pTTTW->lpszText =
          L"Catalogs labels and links which are associated with "
          L"specific codestreams, compositing layers, or the rendered "
          L"result, but not with specific image regions.\n"
          L"   Double-click items in this catalog (or press ENTER) to move "
          L"the image view to display the most closely related content which "
          L"involves the codestreams and/or compositing layers associated "
          L"with that item.";
      else if ((node->parent == NULL) &&
               ((node->flags & 0x000F) == KDWS_CATALOG_TYPE_REGIONS))
        pTTTW->lpszText =
          L"Catalogs labels and links which are associated with "
          L"specific spatial regions.\n"
          L"   Double-click items in this catalog (or press ENTER) to move the "
          L"image view to display the relevant imagery and also adjust the "
          L"focus box to surround the region of interest.";
      else if (node->flags & KDWS_CATFLAG_HAS_METADATA)
        {
          if (node->link_type != JPX_METANODE_LINK_NONE)
            pTTTW->lpszText =
              L"Double-click links (or press ENTER) to move the selection to "
              L"the link target, if possible.\n"
              L"   Right/ctrl-click to edit\n"
              L"   Arrow keys move selection between and within items\n"
              L"   Del/Backspace to delete (if editor is closed)\n"
              L"   ctrl-R to limit region-of-interest overlays to "
              L"those which are semantically connected to this link only";
          else
            pTTTW->lpszText =
              L"Double-click (or press ENTER) to dispay the relevant imagery "
              L"and adjust the focus box to surround the region of "
              L"interest.\n"
              L"   Right/ctrl-click to open the editor at the link source\n"
              L"   Arrow keys move selection between and within items\n"
              L"   Del/Backspace to delete the link (if editor is closed)\n"
              L"   ctrl-R to limit region-of-interest overlays to "
              L"those which are semantically connected to this item only.";
        }
    }
  ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,700);
  *pResult = 0;
  return TRUE;
}


/*===========================================================================*/
/*                              kdws_catalog                                 */
/*===========================================================================*/

/*****************************************************************************/
/*                       kdws_catalog::kdws_catalog                          */
/*****************************************************************************/

kdws_catalog::kdws_catalog()
{
  last_drawn_with_focus_ring = false;
  tools_font = NULL;
  tools_font_height = 0;
  renderer = NULL;
  need_metanode_touch = true;
  performing_collapse_expand = false;
  need_client_update = false;
  for (int n=0; n < KDWS_CATALOG_NUM_TYPES; n++)
    root_nodes[n] = NULL;
  history = NULL;
  pasted_link = jpx_metanode();
  pasted_node_to_cut = jpx_metanode();
  pasted_label = NULL;
  tree_ctrl.catalog = this;
}

/*****************************************************************************/
/*                          kdws_catalog::Create                             */
/*****************************************************************************/

BOOL kdws_catalog::Create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect,
                          CWnd* pParentWnd, UINT nID)
{
  // Create custom colours for drawing tree item text
  normal_text_colour = RGB(0,0,0);
  root_text_colour = RGB(153,51,0); // Brown
  prefix_text_colour = RGB(100,100,100); // Grey
  grouping_link_text_colour = RGB(255,102,0); // Orange
  alternate_child_link_text_colour = RGB(0,0,255); // Blue
  alternate_parent_link_text_colour = RGB(204,0,102); // Purple

  // Create background brush and frame pens
  focus_frame_pen.CreatePen(PS_SOLID,3,RGB(0,0,255));
  nonfocus_frame_pen.CreatePen(PS_SOLID,3,RGB(200,200,200));
  background_brush.CreateSolidBrush(RGB(255,255,230));

  // Create custom colours for drawing tree item backrounds when selected
  normal_backsel_colour = RGB(250,192,144);
  root_backsel_colour = RGB(278,161,199);
  prefix_backsel_colour = RGB(200,200,200);
  grouping_link_backsel_colour = RGB(147,205,221);
  alternate_child_link_backsel_colour = RGB(194,214,154);
  alternate_parent_link_backsel_colour = RGB(197,190,151);

  // Create custom fonts for drawing tree items
  LOGFONT font_attributes; tools_font->GetLogFont(&font_attributes);
  font_attributes.lfWeight = FW_NORMAL;
  font_attributes.lfItalic = FALSE;
  normal_item_font.CreateFontIndirect(&font_attributes);
  prefix_item_font.CreateFontIndirect(&font_attributes);
  font_attributes.lfWeight = FW_HEAVY;
  root_item_font.CreateFontIndirect(&font_attributes);
  font_attributes.lfWeight = FW_NORMAL;
  font_attributes.lfUnderline = TRUE;
  grouping_link_font.CreateFontIndirect(&font_attributes);
  alternate_child_link_font.CreateFontIndirect(&font_attributes);
  alternate_parent_link_font.CreateFontIndirect(&font_attributes);
  font_attributes.lfUnderline = FALSE;
  font_attributes.lfStrikeOut = TRUE;
  cut_text_font.CreateFontIndirect(&font_attributes);

  // Create the controls
  if (!CStatic::Create(_T("Catalog paste bar/tools"),dwStyle,
                       rect,pParentWnd,nID))
    return FALSE;
  this->SetFont(tools_font,FALSE);

  if (!paste_bar.Create(NULL,SS_SUNKEN|SS_LEFT|SS_ENDELLIPSIS,
                        CRect(0,0,0,0),this,KD_CATALOG_PASTEBAR_CTRL_ID))
    return FALSE;

  paste_bar.SetFont(tools_font,FALSE);

  if (!back_button.Create(_T("Back"),BS_PUSHBUTTON|BS_CENTER|BS_VCENTER,
                          CRect(0,0,0,0),this,KD_CATALOG_BACK_BUTTON_ID))
    return FALSE;
  if (!fwd_button.Create(_T("Fwd"),BS_PUSHBUTTON|BS_CENTER|BS_VCENTER,
                         CRect(0,0,0,0),this,KD_CATALOG_FWD_BUTTON_ID))
    return FALSE;
  if (!peer_button.Create(_T("Peer"),BS_PUSHBUTTON|BS_CENTER|BS_VCENTER,
                          CRect(0,0,0,0),this,KD_CATALOG_PEER_BUTTON_ID))
    return FALSE;
  back_button.SetFont(tools_font,FALSE);
  fwd_button.SetFont(tools_font,FALSE);
  peer_button.SetFont(tools_font,FALSE);

  if (!tree_ctrl.Create(TVS_HASBUTTONS | TVS_FULLROWSELECT | WS_BORDER |
                        TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
                        CRect(0,0,0,0),this,KD_CATALOG_TREE_CTRL_ID))
    return FALSE;
  tree_ctrl.SetBkColor(RGB(255,255,230));
  tree_ctrl.SendMessage(CCM_SETVERSION,5,0); // Ensures sizing for item fonts
  tree_supports_true_unichars = false;
  OSVERSIONINFO os_version;
  memset(&os_version,0,sizeof(os_version));
  os_version.dwOSVersionInfoSize = sizeof(os_version);
  GetVersionEx(&os_version);
  if (os_version.dwMajorVersion >= 6)
    tree_supports_true_unichars = true;
  return TRUE;
}

/*****************************************************************************/
/*                         kdws_catalog::DrawItem                            */
/*****************************************************************************/

void kdws_catalog::DrawItem(DRAWITEMSTRUCT *draw_struct)
{
  CDC dc;
  dc.Attach(draw_struct->hDC);
  RECT rect = draw_struct->rcItem;
  CBrush *old_brush = dc.SelectObject(&background_brush);
  CPen *frame_pen = &nonfocus_frame_pen;
  last_drawn_with_focus_ring = tree_ctrl.has_focus;
  if (last_drawn_with_focus_ring)
    frame_pen = &focus_frame_pen;
  CPen *old_pen = dc.SelectObject(frame_pen);
  dc.Rectangle(&rect);
  dc.SelectObject(old_pen);
  dc.SelectObject(old_brush);
  int old_bk_mode = dc.SetBkMode(TRANSPARENT);
  COLORREF old_text_colour = dc.SetTextColor(normal_text_colour);
  rect.bottom = rect.top + tools_height;
  rect.top += 3;
  rect.left +=3;
  rect.right -= 3;
  dc.DrawEdge(&rect,EDGE_BUMP,BF_RECT);
  rect.top += 2;
  CString title;
  GetWindowText(title);
  dc.DrawText(title,&rect,DT_CENTER);
  dc.SetTextColor(old_text_colour);
  dc.SetBkMode(old_bk_mode);
  dc.Detach();
}

/*****************************************************************************/
/*                        kdws_catalog::reveal_focus                         */
/*****************************************************************************/

void kdws_catalog::reveal_focus()
{
  bool need_focus_ring = tree_ctrl.has_focus;
  if (need_focus_ring != last_drawn_with_focus_ring)
    this->Invalidate(FALSE);
}

/*****************************************************************************/
/*                        kdws_catalog::~kdws_catalog                        */
/*****************************************************************************/

kdws_catalog::~kdws_catalog()
{
  for (int n=0; n < KDWS_CATALOG_NUM_TYPES; n++)
    if ((root_nodes[n] != NULL) || pasted_link.exists() ||
        pasted_node_to_cut.exists())
      {
        kdu_error e; e << "You have neglected to invoke "
        "`kdws_catalog:deactivate' prior to deleting the `kdws_catalog' "
        "object.  As a result, a deferred deletion of this object may be "
        "occurring, running the risk of accessing invalid `jpx_metanode's.";
      }
  if (pasted_label != NULL)
    delete[] pasted_label;
  normal_item_font.DeleteObject();
  root_item_font.DeleteObject();
  prefix_item_font.DeleteObject();
  grouping_link_font.DeleteObject();
  alternate_child_link_font.DeleteObject();
  alternate_parent_link_font.DeleteObject();
  cut_text_font.DeleteObject();
  focus_frame_pen.DeleteObject();
  nonfocus_frame_pen.DeleteObject();
  background_brush.DeleteObject();
}

/*****************************************************************************/
/*                         kdws_catalog::activate                            */
/*****************************************************************************/

void kdws_catalog::activate(kdws_renderer *renderer)
{
  this->renderer = renderer;
  need_metanode_touch = true;
  performing_collapse_expand = false;
  need_client_update = false;
  assert(history == NULL);
  pasted_link = jpx_metanode();
  pasted_node_to_cut = jpx_metanode();
  
  // Now create the root nodes; these are the only ones for which we directly
  // insert tree control items.  All the rest are inserted (or deleted)
  // dynamically, as items are expanded or collapsed.  Even here, we do not
  // actually insert the item's label, since all labels are generated
  // dynamically, on demand, inside the `get_tree_item_dispinfo'
  // notification handler.
  HTREEITEM last_tree_item = NULL;
  for (int r=0; r < KDWS_CATALOG_NUM_TYPES; r++)
    {
      kdws_catalog_node *node = new kdws_catalog_node(r);
      node->init();
      root_nodes[r] = node;
      node->reload_tree_item(this);
    }
}

/*****************************************************************************/
/*                        kdws_catalog::deactivate                           */
/*****************************************************************************/

void kdws_catalog::deactivate()
{
  if (renderer == NULL)
    return;
  renderer = NULL;
  this->paste_label("");
  if (history != NULL)
    while (history->prev != NULL)
      history = history->prev;
  kdws_catalog_selection *hist;
  while ((hist = history) != NULL)
    {
      history = hist->next;
      delete hist;
    }

  for (int n=0; n < KDWS_CATALOG_NUM_TYPES; n++)
    if (root_nodes[n] != NULL)
      {
        delete root_nodes[n]; // Detaches everything from the tree control
        root_nodes[n] = NULL;
      }
  tree_ctrl.DeleteAllItems();
}

/*****************************************************************************/
/*                      kdws_catalog::update_metadata                        */
/*****************************************************************************/

void kdws_catalog::update_metadata()
{
  jpx_meta_manager meta_manager = renderer->get_meta_manager();
  if (!meta_manager.exists())
    return; // Nothing to do
  if (need_metanode_touch)
    { // Starting up for the first time
      meta_manager.access_root().touch();
      need_metanode_touch = false;
      need_client_update = true;
    }
  meta_manager.load_matches(-1,NULL,-1,NULL);
  
  int n;
  bool root_node_empty[KDWS_CATALOG_NUM_TYPES];
  for (n=0; n < KDWS_CATALOG_NUM_TYPES; n++)
    root_node_empty[n] = ((root_nodes[n] != NULL) &&
                           (root_nodes[n]->child_info->num_children == 0));
  
  // Remember properties of the current selection, so we can restore it later.
  kdws_catalog_node *orig_selected_node = this->get_selected_node();
  jpx_metanode orig_selected_metanode;
  int orig_selected_pos = -1;
  if ((orig_selected_node != NULL) && (orig_selected_node->item != NULL))
    {
      orig_selected_pos = tree_ctrl.get_visible_location(
                     orig_selected_node->item->get_tree_item());
      if (orig_selected_node->flags & KDWS_CATFLAG_HAS_METADATA)
        {
          assert(orig_selected_node->cur_entry != NULL);
          orig_selected_metanode = orig_selected_node->cur_entry->metanode;
        }
    }
  orig_selected_node = NULL; // So we don't accidentally access it

  // Now update the metadata
  jpx_metanode metanode;
  while ((metanode = meta_manager.get_touched_nodes()).exists())
    {
      if (metanode == pasted_node_to_cut)
        this->paste_label(NULL); // Clear the paste bar
      
      jpx_metanode link_target;
      jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
      const char *label =
        get_label_and_link_info_for_metanode(metanode,link_type,link_target);
      kdws_catalog_entry *entry = get_entry_for_metanode(metanode);
      kdws_catalog_node *node = (entry==NULL)?NULL:(entry->container);
      if (metanode.is_deleted() || (label == NULL))
        {
          kdu_uint32 touched_box_type = metanode.get_box_type();
          if ((touched_box_type == jp2_number_list_4cc) ||
              (touched_box_type == jp2_roi_description_4cc))
            need_client_update = true; // May need metareqs for descendants
          if (entry != NULL)
            {
              if ((entry->prev != NULL) || (entry->next != NULL))
                node = node->extract_entry(entry);
              assert(entry == node->cur_entry);
              node->unlink();
              delete node;
            }
          continue;
        }
      
      // Find the correct parent for `metanode'
      kdws_catalog_entry *new_parent_entry = NULL;
      kdws_catalog_node *new_parent = NULL;
      bool have_numlist=false, have_roi=false;
      jpx_metanode scan = metanode;
      while ((scan = scan.get_parent()).exists())
        {
          if ((new_parent_entry = get_entry_for_metanode(scan)) != NULL)
            break;
          bool rres;
          int num_l, num_cs;
          if (scan.get_numlist_info(num_cs,num_l,rres) &&
              ((num_cs > 0) || (num_l > 0) || rres))
            have_numlist = true;
          else if (scan.get_num_regions() > 0)
            have_roi = true;
        }
      if (new_parent_entry != NULL)
        new_parent = new_parent_entry->container;
      else
        {
          if (have_roi)
            new_parent = root_nodes[KDWS_CATALOG_TYPE_REGIONS];
          else if (have_numlist)
            new_parent = root_nodes[KDWS_CATALOG_TYPE_ENTITIES];
          else
            new_parent = root_nodes[KDWS_CATALOG_TYPE_INDEX];
        }
      if (new_parent->flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS)
        { // Put everything under the parent's first entry
          assert(new_parent->cur_entry->prev == NULL);
          new_parent_entry = new_parent->cur_entry;
        }

      if (node != NULL)
        { // See if things have changed
          if (entry->incomplete)
            {
              if (node->flags &
                  (KDWS_CATFLAG_IS_EXPANDED | KDWS_CATFLAG_WANTS_EXPAND))
                need_client_update = true;
              kdu_uint32 box_types[2]={jp2_label_4cc,jp2_cross_reference_4cc};
              if (metanode.check_descendants_complete(2,box_types))
                {
                  entry->incomplete = false; // Now complete
                  if (node->flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS)
                    {
                      kdws_catalog_entry *scan = node->cur_entry;
                      if (scan->child_info.num_children == 0)
                        for (; scan != NULL; scan=scan->next)
                          if (scan->incomplete)
                            break;
                      if (scan == NULL)
                        node->note_label_changed(); // There won't be children
                    }
                  else if ((entry == node->cur_entry) &&
                           (entry->child_info.num_children == 0))
                    node->note_label_changed(); // There won't be children
                }
            }

          bool need_reinsert = false;
          bool grouping_link_changed = false;
          if (node->link_type == JPX_GROUPING_LINK)
            {
              jpx_metanode_link_type tmp_link_type;
              kdws_catalog_entry *escan;
              for (escan=node->cur_entry; escan != NULL; escan=escan->next)
                if (link_target != escan->metanode.get_link(tmp_link_type))
                  { grouping_link_changed=true; break; }
            }
          if (grouping_link_changed || (link_type != node->link_type) ||
              (node->label_hash != generate_label_hash(label)))
            {
              need_reinsert = true; // Because something has changed
              if ((entry->prev != NULL) || (entry->next != NULL))
                node = node->extract_entry(entry);
              node->change_label(label);
              node->change_link_info(link_type);
            }
          else if ((node->parent != new_parent) ||
                   (node->parent_entry != new_parent_entry))
            { // Change of parenting relationship
              need_reinsert = true;
              if ((entry->prev != NULL) || (entry->next != NULL))
                node = node->extract_entry(entry);
              node->change_link_info(link_type);
                  // May modify `has_collapsed_descendants'
            }
          if (need_reinsert)
            {
              if (node->parent != NULL)
                node->unlink(); // Best to remove
              node = new_parent->insert(node,new_parent_entry);
            }
        }
      else
        { // Create from scratch
          node = new kdws_catalog_node(new_parent->flags &
                                       KDWS_CATFLAG_TYPE_MASK);
          node->init(metanode);
          assert(node->link_type == link_type);
          node = new_parent->insert(node,new_parent_entry);
        }
      
      // See if we have an incomplete link
      if ((link_type != JPX_METANODE_LINK_NONE) && (!link_target) &&
          (node->parent != NULL) &&
          (node->parent->flags &
           (KDWS_CATFLAG_IS_EXPANDED | KDWS_CATFLAG_WANTS_EXPAND)))
        need_client_update = true;
    }
  
  performing_collapse_expand = true;
  for (n=0; n < KDWS_CATALOG_NUM_TYPES; n++)
    if (root_nodes[n] != NULL)
      {
        if (root_nodes[n]->child_info->check_new_prefix_possibilities)
            root_nodes[n]->child_info->investigate_all_prefix_possibilities();
        if (root_node_empty[n] &&
            (root_nodes[n]->child_info->num_children > 0))
          root_nodes[n]->flags |= KDWS_CATFLAG_NEEDS_EXPAND;
        root_nodes[n]->reflect_changes_part1(this);
        root_nodes[n]->reflect_changes_part2(this);
      }
  performing_collapse_expand = false;

  // Now restore the selection and the location of the selected row, if it
  // has not disappeared
  if (orig_selected_metanode.exists() && (orig_selected_pos >= 0))
    {
      kdws_catalog_entry *orig_entry =
        get_entry_for_metanode(orig_selected_metanode);
      if (orig_entry != NULL)
        orig_selected_node = orig_entry->container;
      if ((orig_selected_node != NULL) && (orig_selected_node->parent != NULL))
        {
          expand_context_for_node(orig_selected_node,orig_entry,false);

          HTREEITEM tree_item = NULL;
          if (orig_selected_node->item != NULL)
            tree_item = orig_selected_node->item->get_tree_item();
          if (tree_item != NULL)
            {
              tree_ctrl.Select(tree_item,TVGN_CARET);
              tree_ctrl.scroll_item_to_visible_location(tree_item,
                                                        orig_selected_pos);
            }
        }
    }

  if (pasted_link.exists())
    this->paste_link(pasted_link); // Repaste the link forces an update
  if (need_client_update)
    {
      need_client_update = false;
      if (renderer != NULL)
        renderer->update_client_metadata_of_interest();
    }
}

/*****************************************************************************/
/*                  kdws_catalog::get_selected_metanode                      */
/*****************************************************************************/

jpx_metanode kdws_catalog::get_selected_metanode()
{
  jpx_metanode result;
  kdws_catalog_node *node = this->get_selected_node();
  if ((node != NULL) && (node->flags & KDWS_CATFLAG_HAS_METADATA))
    {
      assert(node->cur_entry != NULL);
      result = node->cur_entry->metanode;
    }
  return result;
}

/*****************************************************************************/
/*               kdws_catalog::get_unique_selected_metanode                  */
/*****************************************************************************/

jpx_metanode kdws_catalog::get_unique_selected_metanode()
{
  jpx_metanode result;
  kdws_catalog_node *node = this->get_selected_node();
  if ((node != NULL) && (node->flags & KDWS_CATFLAG_HAS_METADATA) &&
      (node->cur_entry != NULL) && (node->cur_entry->prev == NULL) &&
      (node->cur_entry->next == NULL))
    result = node->cur_entry->metanode;
  return result;  
}

/*****************************************************************************/
/*         kdws_catalog::find_best_catalogued_descendant:and_distance        */
/*****************************************************************************/

jpx_metanode
  kdws_catalog::find_best_catalogued_descendant(jpx_metanode node,
                                                int &distance_val,
                                                bool no_regions)
{
  jpx_metanode ref_node;
  if (history != NULL)
    ref_node = history->metanode;
  kdws_catalog_entry *entry;
  jpx_metanode tmp, child, result;
  
  // First search for children which are in the catalog themselves.
  int dist, min_distance=INT_MAX;
  int c, num_descendants; node.count_descendants(num_descendants);
  for (c=0; (c < num_descendants) && (min_distance > 0); c++)
    { 
      child = node.get_descendant(c);
      if (child.exists() && ((child.get_regions()==NULL) || !no_regions) &&
          (entry = get_entry_for_metanode(child)) != NULL)
        { 
          if ((entry->container->link_type == JPX_GROUPING_LINK) &&
              (tmp = find_best_catalogued_descendant(child,dist,
                                                     no_regions)).exists())
            child = tmp;
          else
            dist = measure_metanode_distance(child,ref_node,true,true);
          if ((dist < min_distance) || !result)
            { min_distance = dist; result = child; }
        }
    }
  
  if (!result)
    { // Now search for children which are direct links into the catalog
      for (c=0; c < num_descendants; c++)
        { 
          node.get_descendant(c);
          jpx_metanode_link_type link_type;
          if (child.exists() && (child = child.get_link(link_type)).exists() &&
              ((child.get_regions() == NULL) || !no_regions) &&
              ((entry = get_entry_for_metanode(child)) != NULL))
            {  
              if ((entry->container->link_type == JPX_GROUPING_LINK) &&
                  (tmp = find_best_catalogued_descendant(child,dist,
                                                         no_regions)).exists())
                child = tmp;
              else
                dist = measure_metanode_distance(child,ref_node,false,true);
              if ((dist < min_distance) || !result)
                { min_distance = dist; result = child; }
            }
        }
    }
  
  if (!result)
    { // Recursively visit descendants
      for (c=0; c < num_descendants; c++)
        { 
          child = node.get_descendant(c);
          if (child.exists() &&
              ((child.get_regions() == NULL) || !no_regions) &&
              (tmp = find_best_catalogued_descendant(child,dist,
                                                     no_regions)).exists() &&
              ((dist < min_distance) || !result))
            { min_distance = dist; result = child; }
        }
    }
  
  if (result.exists())
    { 
      distance_val = min_distance;
      if (distance_val < INT_MAX)
        distance_val++;
    }
  return result;
}

/*****************************************************************************/
/*                 kdws_catalog::select_matching_metanode                    */
/*****************************************************************************/

bool kdws_catalog::select_matching_metanode(jpx_metanode metanode)
{
  if (!metanode)
    return false;
  kdws_catalog_entry *entry = get_entry_for_metanode(metanode);
  if (entry == NULL)
    { 
      int dist = -1;
      metanode = find_best_catalogued_descendant(metanode,dist,false);
      if (metanode.exists())
        entry = get_entry_for_metanode(metanode);
    }
  if (entry == NULL)
    return false;
  
  kdws_catalog_node *node = entry->container;
  expand_context_for_node(node,entry,true);
  
  // Now select the node
  HTREEITEM prev_item = tree_ctrl.GetSelectedItem();
  HTREEITEM new_item = node->item->get_tree_item();
  if (new_item == prev_item)
    renderer->flash_imagery_for_metanode(metanode);
  tree_ctrl.Select(new_item,TVGN_CARET);
  if (prev_item == new_item)
    tree_ctrl.scroll_item_to_visible_location(new_item,
                     tree_ctrl.get_visible_location(prev_item));
  else
    tree_ctrl.scroll_item_to_visible_location(new_item,-1);
  return true;
}

/*****************************************************************************/
/*                        kdws_catalog::paste_label                          */
/*****************************************************************************/

void kdws_catalog::paste_label(const char *label)
{
  if (pasted_label != NULL)
    { delete[] pasted_label; pasted_label = NULL; }
  pasted_link = jpx_metanode();
  pasted_node_to_cut = jpx_metanode();
  paste_bar.SetFont(&normal_item_font);
  if ((label == NULL) || (*label == '\0'))
    paste_bar.SetWindowText(_T(""));
  else
    {
      pasted_label = new char[strlen(label)+1];
      strcpy(pasted_label,label);
      paste_bar.SetWindowText(kdws_string(pasted_label));
    }
}

/*****************************************************************************/
/*                       kdws_catalog::paste_link                            */
/*****************************************************************************/

void kdws_catalog::paste_link(jpx_metanode link_target)
{
  paste_label(NULL); // Clears the paste bar
  if (!link_target)
    return;
  jpx_metanode_link_type link_type;
  if (link_target.get_link(link_type).exists())
    link_target = link_target.get_link(link_type); // `link' is already a link
  jpx_fragment_list frag_list;
  if ((!link_target) || (link_target.get_link(link_type) != NULL) ||
      (link_target.get_cross_reference(frag_list) != 0))
    return;
  if (link_target.is_deleted())
    return;
  
  char paste_string_buf[80];
  const char *paste_string = NULL;
  const char *label = NULL;
  int num_streams, num_layers;
  bool rres;
  int i_param;
  void *addr_param;
  kdu_uint32 box_type = link_target.get_box_type();
  if ((label = link_target.get_label()) != NULL)
    paste_string = label;
  else if (link_target.get_num_regions() > 0)
    { // Represent the ROI node through its bounding box
      kdu_dims bounding_box = link_target.get_bounding_box();
      sprintf(paste_string_buf,"<ROI: BB=%dx%d @ (%d,%d)>",
              bounding_box.size.x,bounding_box.size.y,
              bounding_box.pos.x,bounding_box.pos.y);
      paste_string = paste_string_buf;
    }
  else if (link_target.get_numlist_info(num_streams,num_layers,rres))
    { // Represent the numlist node through its associations
      int n;
      const int *streams = link_target.get_numlist_codestreams();
      const int *layers = link_target.get_numlist_layers();
      bool need_comma = false;
      paste_string = paste_string_buf;
      if (rres)
        { strcpy(paste_string_buf,"<R"); need_comma = true; }
      else
        strcpy(paste_string_buf,"<");
      for (n=0; n < num_streams; n++, need_comma=true)
        {
          if (need_comma)
            strcat(paste_string_buf,",");
          sprintf(paste_string_buf+strlen(paste_string_buf),"C%d",streams[n]);
        }
      for (n=0; n < num_layers; n++, need_comma=true)
        {
          if (need_comma)
            strcat(paste_string_buf,",");
          sprintf(paste_string_buf+strlen(paste_string_buf),",L%d",layers[n]);
        }
      strcat(paste_string_buf,">");
    }
  else if (link_target.get_delayed(i_param,addr_param))
    paste_string = "<in file pending save>";
  else if (box_type == jp2_xml_4cc)
    paste_string = "<XML box>";
  else if (box_type == jp2_iprights_4cc)
    paste_string = "<IP Rights box>";
  else
    {
      paste_string = paste_string_buf;
      paste_string_buf[0] = '<';
      paste_string_buf[1] = (char)((box_type>>24) & 0x00FF);
      paste_string_buf[2] = (char)((box_type>>16) & 0x00FF);
      paste_string_buf[3] = (char)((box_type>>8) & 0x00FF);
      paste_string_buf[4] = (char)((box_type>>0) & 0x00FF);
      paste_string_buf[5] = '>';
      paste_string_buf[6] = '\0';
      for (int c=1; c < 5; c++)
        if ((paste_string_buf[c] < 'A') || (paste_string_buf[c] > 'z'))
          paste_string_buf[c] = '_';
    }
  pasted_link = link_target;
  paste_bar.SetFont(&alternate_child_link_font);
  paste_bar.SetWindowText(kdws_string(paste_string));
}

/*****************************************************************************/
/*                    kdws_catalog::paste_node_to_cut                        */
/*****************************************************************************/

void kdws_catalog::paste_node_to_cut(jpx_metanode cut_target)
{
  paste_label(NULL); // Clears the paste bar
  if ((!cut_target) || cut_target.is_deleted())
    return;
  const char *label = cut_target.get_label();
  if (label == NULL)
    return;
  pasted_node_to_cut = cut_target;
  paste_bar.SetFont(&cut_text_font);
  paste_bar.SetWindowText(kdws_string(label));
}

/*****************************************************************************/
/*                     kdws_catalog::get_pasted_label                        */
/*****************************************************************************/

const char * kdws_catalog::get_pasted_label()
{
  return pasted_label;
}

/*****************************************************************************/
/*                     kdws_catalog::get_pasted_link                         */
/*****************************************************************************/

jpx_metanode kdws_catalog::get_pasted_link()
{
  if (pasted_link.exists() && pasted_link.is_deleted())
    paste_label(NULL); // Clears the paste bar
  return pasted_link;
}

/*****************************************************************************/
/*                  kdws_catalog::get_pasted_node_to_cut                     */
/*****************************************************************************/

jpx_metanode kdws_catalog::get_pasted_node_to_cut()
{
  if (pasted_node_to_cut.exists() && pasted_node_to_cut.is_deleted())
    paste_label(NULL); // Clears the paste bar
  return pasted_node_to_cut;
}

/*****************************************************************************/
/*                kdws_catalog::generate_metadata_requests                   */
/*****************************************************************************/

int kdws_catalog::generate_client_requests(kdu_window *client_window)
{
  int num_metareqs = 0;
  jpx_meta_manager meta_manager = renderer->get_meta_manager();
  if (!meta_manager.exists())
    return 0; // Nothing to do
  
  for (int r=0; r < KDWS_CATALOG_NUM_TYPES; r++)
    {
      kdws_catalog_node *root = root_nodes[r];
      num_metareqs += root->generate_client_requests(client_window);
    }

  jpx_metanode node = meta_manager.access_root();
  kdu_uint32 box_types[4] = {jp2_label_4cc,jp2_cross_reference_4cc,
                             jp2_number_list_4cc,jp2_roi_description_4cc};
  kdu_uint32 descend_types[2] = {jp2_number_list_4cc,jp2_roi_description_4cc};
  num_metareqs +=
    node.generate_metareq(client_window,4,box_types,2,descend_types,true);
  return num_metareqs;
}

/*****************************************************************************/
/*                      kdws_tree_ctrl::user_key_down                        */
/*****************************************************************************/

bool kdws_catalog::user_key_down(UINT nChar)
{
  bool is_enter=false, is_backspace=false;
  bool is_uparrow=false, is_downarrow=false;
  if (nChar == VK_RETURN)
    is_enter = true;
  else if (nChar == VK_BACK)
    is_backspace = true;
  else if (nChar == VK_UP)
    is_uparrow = true;
  else if (nChar == VK_DOWN)
    is_downarrow=true;
  if (is_enter || is_backspace || is_uparrow || is_downarrow)
    {
      kdws_catalog_node *node = get_selected_node();
      if ((node != NULL) &&
          (node->flags & KDWS_CATFLAG_HAS_METADATA) &&
          (node->cur_entry != NULL))
        {
          if (is_enter)
            {
              process_double_click(node->cur_entry->metanode);
              return true;
            }
          else if (is_backspace)
            {
              if (renderer != NULL)
                renderer->menu_MetadataDelete();
              return true;
            }
          else if (!(node->flags & KDWS_CATFLAG_ANONYMOUS_ENTRIES))
            {
              if (is_uparrow && (node->cur_entry->prev != NULL))
                {
                  select_matching_metanode(node->cur_entry->prev->metanode);
                  return true;
                }
              else if (is_downarrow && (node->cur_entry->next != NULL))
                {
                  select_matching_metanode(node->cur_entry->next->metanode);
                  return true;
                }
            }
        }
    }
  return false;
}

/*****************************************************************************/
/*                    kdws_catalog::user_double_click                        */
/*****************************************************************************/

void kdws_catalog::user_double_click()
{
  jpx_metanode metanode = this->get_selected_metanode();
  if (!metanode)
    return;
  process_double_click(metanode);
}

/*****************************************************************************/
/*                   kdws_catalog::process_double_click                      */
/*****************************************************************************/

void kdws_catalog::process_double_click(jpx_metanode metanode)
{
  if (!metanode)
    return;
  kdws_catalog_entry *orig_entry = get_entry_for_metanode(metanode);
  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
  jpx_metanode link_target = metanode.get_link(link_type);
  if (link_target.exists())
    {
      if (link_target.get_state_ref() != NULL)
        { // Select the link target in the catalog instead -- but first
          // see if the link target has an immediate descendant which links
          // to a node from which we are descended.  If so, this will make
          // for better interaction.
          link_target = look_for_related_child_link(link_target,metanode);
          this->select_matching_metanode(link_target);
        }
    }
  else
    link_target = metanode;
  if (renderer != NULL)
    { // Show imagery for the most derived node
      kdws_catalog_entry *target_entry = get_entry_for_metanode(link_target);
      kdu_uint16 target_type=0, orig_type=0;
      int ncs, nl; bool rres;
      if (target_entry != NULL)
        target_type = target_entry->container->flags & KDWS_CATFLAG_TYPE_MASK;
      else if (link_target.get_regions() != NULL)
        target_type = KDWS_CATALOG_TYPE_REGIONS;
      else if (link_target.get_numlist_info(ncs,nl,rres))
        target_type = KDWS_CATALOG_TYPE_ENTITIES;      
      if (orig_entry != NULL)
        orig_type = orig_entry->container->flags & KDWS_CATFLAG_TYPE_MASK;
      if (orig_type > target_type)
        {
          renderer->show_imagery_for_metanode(metanode);
          renderer->flash_imagery_for_metanode(metanode);
      }
      else if (target_type != KDWS_CATALOG_TYPE_INDEX)
        {
          renderer->show_imagery_for_metanode(link_target);
          renderer->flash_imagery_for_metanode(link_target);
        }
    }
}

/*****************************************************************************/
/*                     kdws_catalog::user_right_click                        */
/*****************************************************************************/

void kdws_catalog::user_right_click(kdws_catalog_node *node)
{
  if ((node != NULL) && (node->flags & KDWS_CATFLAG_HAS_METADATA) &&
      !(node->flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS))
    {
      assert(node->cur_entry != NULL);
      if (renderer != NULL)
        renderer->edit_metanode(node->cur_entry->metanode,true);
    }
}

/*****************************************************************************/
/*                     kdws_catalog::get_selected_node                       */
/*****************************************************************************/

kdws_catalog_node *
  kdws_catalog::get_selected_node()
{
  HTREEITEM tree_item = tree_ctrl.GetSelectedItem();
  if (tree_item == NULL)
    return NULL;
  kdws_catalog_item *item = (kdws_catalog_item *)
    tree_ctrl.GetItemData(tree_item);
  if (item == NULL)
    return NULL;
  return item->get_node();
}

/*****************************************************************************/
/*                   kdws_catalog::expand_contex_for_node                    */
/*****************************************************************************/

void
  kdws_catalog::expand_context_for_node(kdws_catalog_node *node,
                                        kdws_catalog_entry *entry,
                                        bool cleanup)
{
  if (entry == NULL)
    entry = node->cur_entry;
  assert(entry->container == node);

  // Find which nodes need to be expanded
  bool needs_attention=false, already_expanded=false;
  int depth_to_first_expand = -10000; // Ridiculously small value
  kdws_catalog_node *scan, *root=NULL;
  for (scan=node; scan != NULL; depth_to_first_expand++,
       root=scan, entry=scan->parent_entry, scan=scan->parent)
    {
      if (needs_attention)
        scan->flags |= KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
      if (entry != scan->cur_entry)
        {
          if (scan->flags & KDWS_CATFLAG_COLLAPSED_DESCENDANTS)
            {
              assert(scan->cur_entry->prev == NULL);
              entry = scan->cur_entry;
            }
          else
            {
              scan->cur_entry = entry;
              if (!(scan->flags & KDWS_CATFLAG_ANONYMOUS_ENTRIES))
                {
                  scan->flags |= (KDWS_CATFLAG_NEEDS_RELOAD |
                                  KDWS_CATFLAG_LABEL_CHANGED);
                  needs_attention = true;
                }
              if (scan->flags & KDWS_CATFLAG_IS_EXPANDED)
                {
                  scan->flags |= KDWS_CATFLAG_NEEDS_COLLAPSE;
                  needs_attention = true;
                }
              if (scan != node)
                {
                  scan->flags |= KDWS_CATFLAG_NEEDS_EXPAND;
                  needs_attention = true;
                  depth_to_first_expand = 0;
                }
            }
        }
      else if (already_expanded || (scan->flags & KDWS_CATFLAG_IS_EXPANDED))
        already_expanded = true;
      else
        {
          scan->flags |= KDWS_CATFLAG_NEEDS_EXPAND;
          needs_attention = true;
          depth_to_first_expand = 0;
        }
    }

  if (needs_attention)
    {
      if ((depth_to_first_expand > 0) && cleanup)
        { // See if we can collapse one not-recently used branch at this
          // depth or less to keep clutter down.
          for (int d=depth_to_first_expand; d >= 1; d--)
            {
              int most_recent_use=-1;
              kdws_catalog_node *best =
                root->find_best_collapse_candidate(d,history,most_recent_use);
              if (best != NULL)
                {
                  best->flags |= KDWS_CATFLAG_NEEDS_COLLAPSE;
                  for (best=best->parent; best != NULL; best=best->parent)
                    {
                      if (!(best->flags &
                        (KDWS_CATFLAG_NEEDS_EXPAND |
                         KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION)))
                        best->flags |= KDWS_CATFLAG_NEEDS_COLLAPSE;
                      best->flags |= KDWS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
                    }
                  break;
                }
            }
        }
      performing_collapse_expand = true;
      root->reflect_changes_part1(this);
      root->reflect_changes_part2(this);
      performing_collapse_expand = false;
      if (need_client_update)
        {
          need_client_update = false;
          if (renderer != NULL)
            renderer->update_client_metadata_of_interest();
        }
    }
}

/*****************************************************************************/
/*        kdws_catalog::assign_tree_items_to_expanding_node_children         */
/*****************************************************************************/

void
  kdws_catalog::assign_tree_items_to_expanding_node_children(
                                                    kdws_catalog_node *node)
{
  kdws_catalog_child_info *info;
  if (!(node->flags & KDWS_CATFLAG_HAS_METADATA))
    info = node->child_info;
  else
    info = &(node->cur_entry->child_info);
  kdws_catalog_node *child = info->children;
  if (child == NULL)
    {
      node->flags |= KDWS_CATFLAG_WANTS_EXPAND;
      if (node->needs_client_requests_if_expanded() && (renderer != NULL))
        {
          if (this->performing_collapse_expand)
            this->need_client_update = true;
          else
            renderer->update_client_metadata_of_interest();
        }
    }
  else
    for (; child != NULL; child=child->next_sibling)
      if (child->item == NULL)
        child->reload_tree_item(this);
}

/*****************************************************************************/
/*             kdws_catalog::perform_expand_processing_for_node              */
/*****************************************************************************/

void
  kdws_catalog::perform_expand_processing_for_node(kdws_catalog_node *node)
{
  node->flags |= KDWS_CATFLAG_IS_EXPANDED;
  if (node->needs_client_requests_if_expanded() && (renderer != NULL))
    {
      if (this->performing_collapse_expand)
        this->need_client_update = true;
      else
        renderer->update_client_metadata_of_interest();
    }
}

/*****************************************************************************/
/*            kdws_catalog::perform_collapse_processing_for_node             */
/*****************************************************************************/

void
  kdws_catalog::perform_collapse_processing_for_node(kdws_catalog_node *node)
{
  HTREEITEM tree_item = node->item->get_tree_item();
  HTREEITEM child_item = tree_ctrl.GetChildItem(tree_item);
  while (child_item != NULL)
    {
      HTREEITEM next_item = tree_ctrl.GetNextSiblingItem(child_item);
      tree_ctrl.DeleteItem(child_item);
      child_item = next_item;
    }

  node->flags &= ~(KDWS_CATFLAG_IS_EXPANDED | KDWS_CATFLAG_WANTS_EXPAND);
  if (node->needs_client_requests_if_expanded() && (renderer != NULL))
    {
      if (this->performing_collapse_expand)
        this->need_client_update = true;
      else
        renderer->update_client_metadata_of_interest();
    }
}


/*****************************************************************************/
/*                    Message Handlers for kdws_catalog                      */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_catalog, CStatic)
ON_WM_SIZE()
ON_WM_CTLCOLOR()
ON_NOTIFY(NM_CUSTOMDRAW,KD_CATALOG_TREE_CTRL_ID,tree_ctrl_custom_draw)
ON_NOTIFY(TVN_DELETEITEM,KD_CATALOG_TREE_CTRL_ID,tree_item_delete)
ON_NOTIFY(TVN_ITEMEXPANDING,KD_CATALOG_TREE_CTRL_ID,item_expanding)
ON_NOTIFY(TVN_ITEMEXPANDED,KD_CATALOG_TREE_CTRL_ID,item_expanded)
ON_NOTIFY(TVN_SELCHANGED,KD_CATALOG_TREE_CTRL_ID,item_selection_changed)
ON_BN_CLICKED(KD_CATALOG_BACK_BUTTON_ID, back_button_click)
ON_BN_CLICKED(KD_CATALOG_FWD_BUTTON_ID,  fwd_button_click)
ON_BN_CLICKED(KD_CATALOG_PEER_BUTTON_ID, peer_button_click)
END_MESSAGE_MAP()

/*****************************************************************************/
/*                          kdws_catalog::OnSize                             */
/*****************************************************************************/

void kdws_catalog::OnSize(UINT nType, int cx, int cy)
{
  if ((cx <= 0) || (cy <= 0))
    return;

  this->Invalidate();
  RECT panel_rect; this->GetClientRect(&panel_rect);
  RECT pastebar_rect = panel_rect;
  pastebar_rect.left += 10;
  pastebar_rect.right -= 10;
  pastebar_rect.top += tools_font_height+8;
  pastebar_rect.bottom = pastebar_rect.top + tools_font_height+4;
  if (pastebar_rect.right > pastebar_rect.left)
    {
      paste_bar.SetWindowPos(NULL,pastebar_rect.left,pastebar_rect.top,
                             pastebar_rect.right-pastebar_rect.left,
                             pastebar_rect.bottom-pastebar_rect.top,
                             SWP_SHOWWINDOW | SWP_NOZORDER);
      paste_bar.Invalidate();
    }
  else
    paste_bar.SetWindowPos(NULL,0,0,0,0,SWP_HIDEWINDOW|SWP_NOZORDER);

  RECT button_rect = pastebar_rect;
  button_rect.top = pastebar_rect.bottom + 4;
  button_rect.bottom = button_rect.top + tools_font_height + 4;
  if (button_rect.right > (button_rect.left+29))
    {
      int b_width=(button_rect.right-button_rect.left-8) / 3;
      back_button.SetWindowPos(NULL,button_rect.left,button_rect.top,
                               b_width,button_rect.bottom-button_rect.top,
                               SWP_SHOWWINDOW | SWP_NOZORDER);
      button_rect.left += b_width + 4;
      b_width = (button_rect.right-button_rect.left-4) / 2;
      fwd_button.SetWindowPos(NULL,button_rect.left,button_rect.top,
                              b_width,button_rect.bottom-button_rect.top,
                              SWP_SHOWWINDOW | SWP_NOZORDER);
      button_rect.left = button_rect.right-b_width;
      peer_button.SetWindowPos(NULL,button_rect.left,button_rect.top,
                               b_width,button_rect.bottom-button_rect.top,
                               SWP_SHOWWINDOW | SWP_NOZORDER);
      back_button.Invalidate();
      fwd_button.Invalidate();
      peer_button.Invalidate();
    }
  else
    {
      back_button.SetWindowPos(NULL,0,0,0,0,SWP_HIDEWINDOW|SWP_NOZORDER);
      fwd_button.SetWindowPos(NULL,0,0,0,0,SWP_HIDEWINDOW|SWP_NOZORDER);
      peer_button.SetWindowPos(NULL,0,0,0,0,SWP_HIDEWINDOW|SWP_NOZORDER);
    }

  RECT tree_rect = panel_rect;
  tree_rect.top += tools_height + 2;
  tree_rect.bottom -= 3;
  tree_rect.left += 3;
  tree_rect.right -= 3;
  if ((tree_rect.bottom > tree_rect.top) && (tree_rect.right > tree_rect.left))
    {
      tree_ctrl.SetWindowPos(NULL,tree_rect.left,tree_rect.top,
                             tree_rect.right-tree_rect.left,
                             tree_rect.bottom-tree_rect.top,
                             SWP_SHOWWINDOW | SWP_NOZORDER);
      tree_ctrl.Invalidate();
    }
  else
    tree_ctrl.SetWindowPos(NULL,0,0,0,0,SWP_HIDEWINDOW|SWP_NOZORDER);
}

/*****************************************************************************/
/*                         kdws_catalog::OnCtlColor                          */
/*****************************************************************************/

HBRUSH kdws_catalog::OnCtlColor(CDC *dc, CWnd *wnd, UINT control_type)
{
  // Call the base implementation first, to get a default brush
  HBRUSH brush = CStatic::OnCtlColor(dc,wnd,control_type);
  if (wnd == &paste_bar)
    {
      if (pasted_link.exists())
        dc->SetTextColor(alternate_child_link_text_colour);
      else
        dc->SetTextColor(normal_text_colour);
    }
  return brush;
}

/*****************************************************************************/
/*                   kdws_catalog::tree_ctrl_custom_draw                     */
/*****************************************************************************/

void kdws_catalog::tree_ctrl_custom_draw(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NMTVCUSTOMDRAW *cdraw = (NMTVCUSTOMDRAW *) pNMHDR;
  *pResult = CDRF_DODEFAULT;
  if (cdraw->nmcd.dwDrawStage == CDDS_PREPAINT)
    *pResult = CDRF_NOTIFYITEMDRAW;
  else if (cdraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
      kdws_catalog_item *item = (kdws_catalog_item *) cdraw->nmcd.lItemlParam;
      if (item == NULL)
        return;
      kdws_catalog_node *node = item->get_node();
      if (node == NULL)
        return;
      bool is_selected = ((cdraw->nmcd.uItemState & CDIS_SELECTED) != 0);
      if (node->parent == NULL)
        { // Draw text with root node attributes
          if (is_selected)
            cdraw->clrTextBk = root_backsel_colour;
          cdraw->clrText = root_text_colour;
          ::SelectObject(cdraw->nmcd.hdc,root_item_font);
        }
      else if (node->flags & KDWS_CATFLAG_PREFIX_NODE)
        {
          if (is_selected)
            cdraw->clrTextBk = prefix_backsel_colour;
          cdraw->clrText = prefix_text_colour;
          ::SelectObject(cdraw->nmcd.hdc,prefix_item_font);
        }
      else if (node->link_type == JPX_GROUPING_LINK)
        { // Draw text with grouping link attributes
          if (is_selected)
            cdraw->clrTextBk = grouping_link_backsel_colour;
          cdraw->clrText = grouping_link_text_colour;
          ::SelectObject(cdraw->nmcd.hdc,grouping_link_font);
        }
      else if (node->link_type == JPX_ALTERNATE_PARENT_LINK)
        { // Draw text with alternate parent link attributes
          if (is_selected)
            cdraw->clrTextBk = alternate_parent_link_backsel_colour;
          cdraw->clrText = alternate_parent_link_text_colour;
          ::SelectObject(cdraw->nmcd.hdc,alternate_parent_link_font);
        }
      else if (node->link_type == JPX_ALTERNATE_CHILD_LINK)
        { // Draw text with alternate child link attributes
          if (is_selected)
            cdraw->clrTextBk = alternate_child_link_backsel_colour;
          cdraw->clrText = alternate_child_link_text_colour;
          ::SelectObject(cdraw->nmcd.hdc,alternate_child_link_font);
        }
      else
        { // Draw text with normal label attributes
          if (is_selected)
            cdraw->clrTextBk = normal_backsel_colour;
          cdraw->clrText = normal_text_colour;
          ::SelectObject(cdraw->nmcd.hdc,normal_item_font);
        }
      *pResult = CDRF_NEWFONT;
    }
}

/*****************************************************************************/
/*                      kdws_catalog::tree_item_delete                       */
/*****************************************************************************/

void kdws_catalog::tree_item_delete(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NMTREEVIEW *params = (NMTREEVIEW *) pNMHDR;
  TVITEM *tvitem = &(params->itemOld);
  kdws_catalog_item *item = (kdws_catalog_item *) tvitem->lParam;
  if (item == NULL)
    return;
  HTREEITEM tree_item = tvitem->hItem;
  if (tree_item == NULL)
    return;
  kdws_catalog_node *node = item->get_node();
  if (node != NULL)
    node->flags &= ~(KDWS_CATFLAG_IS_EXPANDED | KDWS_CATFLAG_WANTS_EXPAND);
        // We need this, because when a user collapses a node, we might not get
        // notification (via `item_expanded' of any subordinate nodes which
        // also get collapsed.  As a result, we can have a node which has
        // been deleted from the tree, but still has the IS_EXPANDED flag set,
        // which corrupts the implementation in other places.
  item->release_tree_item(tree_item);
}

/*****************************************************************************/
/*                        kdws_catalog::item_expanding                       */
/*****************************************************************************/

void kdws_catalog::item_expanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NMTREEVIEW *params = (NMTREEVIEW *) pNMHDR;
  TVITEM *tvitem = &(params->itemNew);
  kdws_catalog_item *item = (kdws_catalog_item *) tvitem->lParam;
  if (item == NULL)
    return;
  HTREEITEM tree_item = tvitem->hItem;
  if (tree_item == NULL)
    return;
  assert(tree_item == item->get_tree_item());
  kdws_catalog_node *node = item->get_node();
  if (node == NULL)
    return;
  if (params->action & TVE_EXPAND)
    assign_tree_items_to_expanding_node_children(node);
}

/*****************************************************************************/
/*                        kdws_catalog::item_expanded                        */
/*****************************************************************************/

void kdws_catalog::item_expanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NMTREEVIEW *params = (NMTREEVIEW *) pNMHDR;
  TVITEM *tvitem = &(params->itemNew);
  kdws_catalog_item *item = (kdws_catalog_item *) tvitem->lParam;
  if (item == NULL)
    return;
  HTREEITEM tree_item = tvitem->hItem;
  if (tree_item == NULL)
    return;
  assert(tree_item == item->get_tree_item());
  kdws_catalog_node *node = item->get_node();
  if (node == NULL)
    return;
  if (params->action & TVE_EXPAND)
    perform_expand_processing_for_node(node);
  else if (params->action & TVE_COLLAPSE)
    perform_collapse_processing_for_node(node);
}

/*****************************************************************************/
/*                  kdws_catalog::item_selection_changed                     */
/*****************************************************************************/

void kdws_catalog::item_selection_changed(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NMTREEVIEW *info = (NMTREEVIEW *) pNMHDR;
  kdws_catalog_node *node = NULL;
  TVITEM *tvitem = &(info->itemNew);
  if (tvitem != NULL)
    {
      kdws_catalog_item *item = (kdws_catalog_item *) tvitem->lParam;
      HTREEITEM tree_item = tvitem->hItem;
      if ((item != NULL) && (tree_item != NULL))
        {
          assert(tree_item == item->get_tree_item());
          node = item->get_node();
        }
    }
  jpx_metanode metanode;
  if (node != NULL)
    {
      if ((node == root_nodes[KDWS_CATALOG_TYPE_INDEX]) ||
          (node == root_nodes[KDWS_CATALOG_TYPE_ENTITIES]))
        { // Changes priority associated with certain metadata requests
          if (performing_collapse_expand)
            need_client_update = true;
          else
            renderer->update_client_metadata_of_interest();
        }
      if ((node->flags & KDWS_CATFLAG_HAS_METADATA) &&
          (node->cur_entry != NULL))
        metanode = node->cur_entry->metanode;
    }
  peer_button.EnableWindow(find_next_peer(metanode).exists());
  if (renderer != NULL)
    renderer->flash_imagery_for_metanode(metanode);
  if (!metanode)
    { // Moved away from a meaningful selection
      back_button.EnableWindow(history != NULL);
      fwd_button.EnableWindow(history != NULL);
      return;
    }
  if ((history != NULL) && (history->metanode == metanode))
    { // Moved to current selection; may be a result of navigation
      back_button.EnableWindow(history->prev != NULL);
      fwd_button.EnableWindow(history->next != NULL);
      return;
    }
  kdws_catalog_selection *elt;
  if (history != NULL)
    { // New element selected; delete all future nodes in browsing history
      while ((elt = history->next) != NULL)
        {
          history->next = elt->next;
          delete elt;
        }
      fwd_button.EnableWindow(FALSE);
    }
  elt = new kdws_catalog_selection;
  elt->metanode = metanode;
  elt->next = NULL;
  if ((elt->prev = history) != NULL)
    elt->prev->next = elt;
  history = elt;
  back_button.EnableWindow(history->prev != NULL);
}

/*****************************************************************************/
/*                      kdws_catalog::back_button_click                      */
/*****************************************************************************/

void kdws_catalog::back_button_click()
{
  if (history == NULL)
    return;
  jpx_metanode metanode = get_selected_metanode();
  if (metanode == history->metanode)
    {
      if (history->prev != NULL)
        {
          history = history->prev;
          select_matching_metanode(history->metanode);
        }
    }
  else
    select_matching_metanode(history->metanode);
  tree_ctrl.SetFocus();
}

/*****************************************************************************/
/*                       kdws_catalog::fwd_button_click                      */
/*****************************************************************************/

void kdws_catalog::fwd_button_click()
{
  if (history == NULL)
    return;
  jpx_metanode metanode = get_selected_metanode();
  if (metanode == history->metanode)
    {
      if (history->next != NULL)
        {
          history = history->next;
          select_matching_metanode(history->metanode);
        }
    }
  else
    select_matching_metanode(history->metanode);
  tree_ctrl.SetFocus();
}

/*****************************************************************************/
/*                      kdws_catalog::peer_button_click                      */
/*****************************************************************************/

void kdws_catalog::peer_button_click()
{
  jpx_metanode metanode = get_selected_metanode();
  jpx_metanode peer = find_next_peer(metanode);
  if (!peer)
    peer_button.EnableWindow(FALSE); // Button should never have been enabled
  else
    select_matching_metanode(peer);
  tree_ctrl.SetFocus();
}
