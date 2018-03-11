/*****************************************************************************/
// File: kdms_catalog.mm [scope = APPS/MACSHOW]
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
 Implements the `kdms_catalog' Objective-C class.
******************************************************************************/
#import "kdms_metadata_editor.h"
#import "kdms_window.h"
#import "kdms_catalog.h"

/*===========================================================================*/
/*                             Internal Functions                            */
/*===========================================================================*/

/*****************************************************************************/
/* INLINE                    get_entry_for_metanode                          */
/*****************************************************************************/

static inline kdms_catalog_entry *
  get_entry_for_metanode(jpx_metanode metanode)
{
  return (kdms_catalog_entry *) metanode.get_state_ref();
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
     they can be reinserted in the course of `kdms_catalog::update_metadata'.
     This function is invoked from within `kdms_catalog_node::extract_entry'
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
      kdms_catalog_entry *entry = get_entry_for_metanode(child);
      if (entry != NULL)
        {
          kdms_catalog_node *node = entry->container;
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
/*                            kdms_catalog_item                              */
/*===========================================================================*/

@implementation kdms_catalog_item

/*****************************************************************************/
/*                      kdms_catalog_item:initWithNode                       */
/*****************************************************************************/

-(kdms_catalog_item *)initWithNode:(kdms_catalog_node *)nd
{
  self->node = nd;
  return self;
}

/*****************************************************************************/
/*                       kdms_catalog_item:deactivate                        */
/*****************************************************************************/

-(void)deactivate
{
  self->node = NULL;
}

/*****************************************************************************/
/*                        kdms_catalog_item:get_node                         */
/*****************************************************************************/

-(kdms_catalog_node *)get_node
{
  return node;
}

@end // kdms_catalog_item


/*===========================================================================*/
/*                           kdms_catalog_child_info                         */
/*===========================================================================*/

/*****************************************************************************/
/*             kdms_catalog_child_info::~kdms_catalog_child_info             */
/*****************************************************************************/

kdms_catalog_child_info::~kdms_catalog_child_info()
{
  kdms_catalog_node *child;
  while ((child = children) != NULL)
    {
      children = child->next_sibling;
      delete child;
    }
}

/*****************************************************************************/
/*       kdms_catalog_child_info::investigate_all_prefix_possibilities       */
/*****************************************************************************/

void kdms_catalog_child_info::investigate_all_prefix_possibilities()
{
  if (!check_new_prefix_possibilities)
    return;
  kdms_catalog_node *scan;
  for (scan=children; scan != NULL; scan=scan->next_sibling)
    {
      kdms_catalog_entry *entry = scan->cur_entry;
      if (!(scan->flags & KDMS_CATFLAG_HAS_METADATA))
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
/*                kdms_catalog_child_info::create_prefix_node                */
/*****************************************************************************/

bool kdms_catalog_child_info::create_prefix_node()
{
  if (children == NULL)
    return false;
  
  kdms_catalog_node *container = children->parent;
  kdms_catalog_entry *container_entry = children->parent_entry;
  assert(((container_entry==NULL) && (this==container->child_info)) ||
         ((container_entry!=NULL) && (this==&(container_entry->child_info))));

  // Find the longest prefix which matches at least 5 nodes (with longer
  // labels), such that there are other nodes which do not match this
  // prefix.
  int max_prefix_len=0, max_prefix_matches=0;
  kdms_catalog_node *max_prefix_start=NULL;
  int prefix_len, min_prefix_len=1;
  if (container->flags & KDMS_CATFLAG_PREFIX_NODE)
    min_prefix_len = container->label_length+1;
  kdms_catalog_node *scan, *next;
  int min_matches_for_next_prefix_len=5;
  if (num_children > 20)
    min_matches_for_next_prefix_len = 2;
  else
    for (scan=children; scan != NULL; scan=scan->next_sibling)
      if ((scan->label_length == min_prefix_len) &&
          (scan->flags & KDMS_CATFLAG_PREFIX_NODE))
        { min_matches_for_next_prefix_len = 2; break; }
  for (prefix_len=min_prefix_len;
       prefix_len <= KDMS_CATALOG_PREFIX_LEN;
       prefix_len++)
    { // See if we can find children which have a common prefix with length
      // `prefix_len' and are worth collapsing.
      kdms_catalog_node *start=NULL;
      int num_matches = 0;
      int min_matches = min_matches_for_next_prefix_len;
      bool have_something_else = false;
      for (scan=children; scan != NULL; scan=scan->next_sibling)
        {
          if (scan->label_length <= prefix_len)
            { have_something_else = true; continue; }
          if ((scan->label_length == (prefix_len+1)) &&
              (scan->flags & KDMS_CATFLAG_PREFIX_NODE))
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
  kdms_catalog_node *new_prefix =
    new kdms_catalog_node(max_prefix_start->flags & KDMS_CATFLAG_TYPE_MASK);
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
/*                              kdms_catalog_node                            */
/*===========================================================================*/

/*****************************************************************************/
/*                    kdms_catalog_node::kdms_catalog_node                   */
/*****************************************************************************/

kdms_catalog_node::kdms_catalog_node(int catalog_type)
{
  
  item = nil;
  object_value = nil;
  flags = ((kdu_uint16) catalog_type) & KDMS_CATFLAG_TYPE_MASK;
  assert(flags == (kdu_uint16) catalog_type);
  label_hash = 0;
  label_length = 0;
  next_sibling = prev_sibling = parent = NULL;
  parent_entry = NULL;
  link_type = JPX_METANODE_LINK_NONE;
  child_info = NULL;
  cur_entry = NULL;
  item = [[kdms_catalog_item alloc] initWithNode:this];
}

/*****************************************************************************/
/*                    kdms_catalog_node::~kdms_catalog_node                  */
/*****************************************************************************/

kdms_catalog_node::~kdms_catalog_node()
{
  if ((flags & KDMS_CATFLAG_HAS_METADATA) && (cur_entry != NULL))
    { // Delete all metadata entries
      while (cur_entry->prev != NULL)
        cur_entry = cur_entry->prev;
      kdms_catalog_entry *tmp;
      while ((tmp=cur_entry) != NULL)
        {
          cur_entry = tmp->next;
          delete tmp;
        }
    }
  else if ((!(flags & KDMS_CATFLAG_HAS_METADATA)) && (child_info != NULL))
    {
      delete child_info;
      child_info = NULL;
    }
  if (item != nil)
    {
      [item deactivate];
      [item autorelease]; // Ensures `item' exists until outlineview is stable
      item = nil;
    }
  if (object_value != nil)
    {
      [object_value autorelease];
      object_value = nil;
    }
}

/*****************************************************************************/
/*                      kdms_catalog_node::init (root)                       */
/*****************************************************************************/

void kdms_catalog_node::init()
{
  assert(((flags & ~KDMS_CATFLAG_TYPE_MASK) == 0) &&
         (label_length == 0) &&
         (child_info == NULL) && (parent == NULL));
  child_info = new kdms_catalog_child_info;
}

/*****************************************************************************/
/*                    kdms_catalog_node::init (metadata)                     */
/*****************************************************************************/

void kdms_catalog_node::init(jpx_metanode metanode)
{
  assert(((flags & ~KDMS_CATFLAG_TYPE_MASK) == 0) &&
         (cur_entry == NULL) && (label_length == 0));
  flags |= KDMS_CATFLAG_HAS_METADATA;
  jpx_metanode_link_type lnk_type;
  jpx_metanode lnk_tgt;
  const char *lbl =
    get_label_and_link_info_for_metanode(metanode,lnk_type,lnk_tgt);
  cur_entry = new kdms_catalog_entry;
  cur_entry->metanode = metanode;
  kdu_uint32 box_types[2] = {jp2_label_4cc,jp2_cross_reference_4cc};
  cur_entry->incomplete = !metanode.check_descendants_complete(2,box_types);
  cur_entry->container = this;
  change_link_info(lnk_type); // Must do this after `cur_entry' setup
  change_label(lbl); // Must do this after `change_link_info'
  metanode.set_state_ref(cur_entry);
}

/*****************************************************************************/
/*                   kdms_catalog_node::init (prefix node)                   */
/*****************************************************************************/

void kdms_catalog_node::init(kdms_catalog_node *ref, int prefix_len)
{
  assert(((flags & ~KDMS_CATFLAG_TYPE_MASK) == 0) &&
         (child_info == NULL) && (label_length == 0));
  child_info = new kdms_catalog_child_info;
  flags |= KDMS_CATFLAG_PREFIX_NODE;
  link_type = JPX_METANODE_LINK_NONE;
  if (prefix_len > KDMS_CATALOG_PREFIX_LEN)
    prefix_len = KDMS_CATALOG_PREFIX_LEN; // Should not happen
  label_length = prefix_len;
  int c;
  for (c=0; c < prefix_len; c++)
    label_prefix[c] = ref->label_prefix[c];
  label_prefix[c] = 0;
}

/*****************************************************************************/
/*                     kdms_catalog_node::change_label                       */
/*****************************************************************************/

void kdms_catalog_node::change_label(const char *new_lbl)
{
  assert(!(flags & KDMS_CATFLAG_PREFIX_NODE));
  kdu_uint32 new_lbl_hash = generate_label_hash(new_lbl);
  bool something_changed = (label_length != 0) && (new_lbl_hash != label_hash);
  this->label_hash = new_lbl_hash;
  
  // Now generate the unicode prefix
  label_length = (kdu_uint16)
    kdms_utf8_to_unicode(new_lbl,label_prefix,KDMS_CATALOG_PREFIX_LEN);
  
  // Now convert to upper case
  NSString *tmp = [[NSString alloc] initWithCharacters:label_prefix
                                                length:label_length];
  [[tmp uppercaseString] getCharacters:label_prefix];
  [tmp release];
  if (something_changed)
    note_label_changed();
}

/*****************************************************************************/
/*                       kdms_catalog_node::get_label                        */
/*****************************************************************************/

const char *kdms_catalog_node::get_label()
{
  const char *result = NULL;
  if (flags & KDMS_CATFLAG_HAS_METADATA)
    {
      jpx_metanode_link_type ltype;
      jpx_metanode link_target;
      if (cur_entry != NULL)
        result = get_label_and_link_info_for_metanode(cur_entry->metanode,
                                                      ltype,link_target);
    }
  else
    { // Treat as a root node -- shouldn't use for prefix children
      int catalog_type = (flags & KDMS_CATFLAG_TYPE_MASK);
      if (catalog_type == KDMS_CATALOG_TYPE_INDEX)
        result = "Index Labels";
      else if (catalog_type == KDMS_CATALOG_TYPE_ENTITIES)
        result = "Image-Entity Labels";
      else if (catalog_type == KDMS_CATALOG_TYPE_REGIONS)
        result = "Region-Specific Labels";
    }
  if (result == NULL)
    result = "???";
  return result;
}

/*****************************************************************************/
/*                    kdms_catalog_node::change_link_info                    */
/*****************************************************************************/

void kdms_catalog_node::change_link_info(jpx_metanode_link_type lnk_type)
{
  assert(flags & KDMS_CATFLAG_HAS_METADATA);
  kdu_uint16 special_flags = 0;
  if (lnk_type == JPX_GROUPING_LINK)
    special_flags = (KDMS_CATFLAG_ANONYMOUS_ENTRIES |
                     KDMS_CATFLAG_COLLAPSED_DESCENDANTS); 
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
        special_flags = KDMS_CATFLAG_ANONYMOUS_ENTRIES;
    }
  
  bool something_changed = ((label_length != 0) &&
                            ((lnk_type != this->link_type) ||
                             (special_flags |=
                              (flags & (KDMS_CATFLAG_COLLAPSED_DESCENDANTS |
                                        KDMS_CATFLAG_ANONYMOUS_ENTRIES)))));
  this->link_type = lnk_type;
  this->flags &= ~(KDMS_CATFLAG_COLLAPSED_DESCENDANTS |
                   KDMS_CATFLAG_ANONYMOUS_ENTRIES);
  this->flags |= special_flags;
  if (something_changed)
    this->note_label_changed();
}

/*****************************************************************************/
/*                         kdms_catalog_node::insert                         */
/*****************************************************************************/

kdms_catalog_node *
  kdms_catalog_node::insert(kdms_catalog_node *child,
                            kdms_catalog_entry *entry)
{
  assert((child->parent == NULL) && (child->parent_entry == NULL) &&
         (child->next_sibling == NULL) && (child->prev_sibling == NULL));
  assert((entry == NULL) || (entry->container == this));
  if (flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS)
    assert(entry == this->cur_entry);
  
  kdms_catalog_child_info *info = NULL;
  if (flags & KDMS_CATFLAG_HAS_METADATA)
    {
      assert((entry != NULL) && (entry->container == this));
      info = &(entry->child_info);
    }
  else
    {
      assert(entry == NULL);
      info = child_info;
    }
  
  kdms_catalog_node *scan, *prev;
  if (child->flags & (KDMS_CATFLAG_NEEDS_EXPAND | KDMS_CATFLAG_NEEDS_COLLAPSE |
                      KDMS_CATFLAG_NEEDS_RELOAD |
                      KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION))
    { // New parent hierarchy all needs attention in the catalog view
      for (scan=this;
           ((scan != NULL) &&
            !(scan->flags & KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION));
           scan=scan->parent)
       scan->flags |= KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
    }

  for (prev=NULL, scan=info->children;
       scan != NULL; prev=scan, scan=scan->next_sibling)
    {
      if ((scan->flags & KDMS_CATFLAG_PREFIX_NODE) &&
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
/*                  kdms_catalog_node::import_entry_list                     */
/*****************************************************************************/

void kdms_catalog_node::import_entry_list(kdms_catalog_node *src)
{
  // Back up to the start of the `src' entry list
  while (src->cur_entry->prev != NULL)
    src->cur_entry = src->cur_entry->prev;
  
  // Find start of the `cur_entry' list
  kdms_catalog_entry *list = this->cur_entry;
  while (list->prev != NULL)
    list = list->prev;
  
  kdms_catalog_entry *entry;
  while ((entry = src->cur_entry) != NULL)
    {
      if (flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS)
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
      kdms_catalog_entry *scan, *prev;
      if (flags & (KDMS_CATFLAG_COLLAPSED_DESCENDANTS |
                   KDMS_CATFLAG_ANONYMOUS_ENTRIES))
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
/*                         kdms_catalog_node::unlink                         */
/*****************************************************************************/

void kdms_catalog_node::unlink()
{
  if (parent == NULL)
    return; // Already unlinked
  kdms_catalog_child_info *parent_info=parent->child_info;
  if (parent->flags & KDMS_CATFLAG_HAS_METADATA)
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
  
  if ((parent->flags & KDMS_CATFLAG_PREFIX_NODE) &&
      (parent_info->num_children < 2) && (parent->parent != NULL))
    { // Collapse parent prefix node -- this may be recursive
      kdms_catalog_node *last_child = parent_info->children;
      if (last_child != NULL)
        {
          kdms_catalog_node *new_parent = parent->parent;
          kdms_catalog_entry *new_parent_entry = parent->parent_entry;
          kdms_catalog_child_info *new_parent_info = new_parent->child_info;
          if (new_parent->flags & KDMS_CATFLAG_HAS_METADATA)
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
/*                     kdms_catalog_node::extract_entry                      */
/*****************************************************************************/

kdms_catalog_node *
  kdms_catalog_node::extract_entry(kdms_catalog_entry *entry)
{
  assert(entry->container == this);

  bool was_first_entry = (entry->prev == NULL);
  bool was_collapsed = (flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS) &&
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
  kdms_catalog_node *new_container =
    new kdms_catalog_node(this->flags & KDMS_CATFLAG_TYPE_MASK);
  new_container->flags = this->flags;
  new_container->cur_entry = entry;
  entry->next = entry->prev = NULL;
  new_container->change_link_info(this->link_type);
  new_container->label_hash = this->label_hash;
  new_container->label_length = this->label_length;
  memcpy(new_container->label_prefix,this->label_prefix,
         sizeof(kdu_uint16)*(KDMS_CATALOG_PREFIX_LEN+1));
  entry->container = new_container;
  
  // Now adjust the descendants
  kdms_catalog_node *child;
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
/*                 kdms_catalog_node::note_children_decreased                */
/*****************************************************************************/

void
  kdms_catalog_node::note_children_decreased(kdms_catalog_child_info *info)
{
  bool needs_attention = false;
  if (!(flags & KDMS_CATFLAG_HAS_METADATA))
    {
      assert(info == this->child_info);
    }
  else if (info != &(cur_entry->child_info))
    {
      assert(!(flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS));
      return;
    }

  bool can_expand = (info->num_children > 0);
  if (flags & KDMS_CATFLAG_IS_EXPANDED)
    {
      needs_attention = true;
      flags |= (KDMS_CATFLAG_NEEDS_COLLAPSE | KDMS_CATFLAG_NEEDS_RELOAD);
      if (can_expand)
        flags |= KDMS_CATFLAG_NEEDS_EXPAND;
    }
  
  if ((!can_expand) && ((parent == NULL) ||
                        (parent->flags & KDMS_CATFLAG_IS_EXPANDED)))
    { // Expandability has changed
      needs_attention = true;
      flags |= KDMS_CATFLAG_NEEDS_RELOAD;
    }
  if (needs_attention)
    for (kdms_catalog_node *scan=parent;
         ((scan != NULL) &&
          !(scan->flags & KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION));
         scan=scan->parent)
      scan->flags |= KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
}

/*****************************************************************************/
/*                 kdms_catalog_node::note_children_increased                */
/*****************************************************************************/

void
  kdms_catalog_node::note_children_increased(kdms_catalog_child_info *info)
{
  if (info->num_children > 2)
    { 
      kdms_catalog_node *node_scan = this;
      kdms_catalog_child_info *info_scan=info;
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

  if (!(flags & KDMS_CATFLAG_HAS_METADATA))
    {
      assert(info == this->child_info);
    }
  else if (info != &(cur_entry->child_info))
    {
      assert(!(flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS));
      return;
    }
  if (flags & KDMS_CATFLAG_IS_EXPANDED)
    flags |= (KDMS_CATFLAG_NEEDS_EXPAND | KDMS_CATFLAG_NEEDS_COLLAPSE |
              KDMS_CATFLAG_NEEDS_RELOAD);
  else if ((info->num_children == 1) &&
           ((parent == NULL) || (parent->flags & KDMS_CATFLAG_IS_EXPANDED)))
    flags |= KDMS_CATFLAG_NEEDS_RELOAD;
  else
    return;
  for (kdms_catalog_node *scan=parent;
       (scan != NULL) &&
       !(scan->flags & KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION);
       scan=scan->parent)
    scan->flags |= KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
}

/*****************************************************************************/
/*                   kdms_catalog_node::note_label_changed                   */
/*****************************************************************************/

void
  kdms_catalog_node::note_label_changed()
{
  if ((parent == NULL) || (parent->flags & KDMS_CATFLAG_IS_EXPANDED))
    {
      flags |= KDMS_CATFLAG_NEEDS_RELOAD;
      for (kdms_catalog_node *scan=parent;
           (scan != NULL) &&
           !(scan->flags & KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION);
           scan=scan->parent)
        scan->flags |= KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
    }
  this->flags |= KDMS_CATFLAG_LABEL_CHANGED;
}

/*****************************************************************************/
/*                 kdms_catalog_node::reflect_changes_part1                  */
/*****************************************************************************/

void
  kdms_catalog_node::reflect_changes_part1(kdms_catalog *catalog)
{
  if (flags & KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION)
    {
      kdms_catalog_node *child;
      if (flags & KDMS_CATFLAG_HAS_METADATA)
        child = cur_entry->child_info.children;
      else
        child = child_info->children;
      for (; child != NULL; child=child->next_sibling)
        child->reflect_changes_part1(catalog);
    }
  if (flags & KDMS_CATFLAG_NEEDS_COLLAPSE)
    [catalog collapseItem:item];
  flags &= ~KDMS_CATFLAG_NEEDS_COLLAPSE;
}

/*****************************************************************************/
/*                 kdms_catalog_node::reflect_changes_part2                  */
/*****************************************************************************/

void
  kdms_catalog_node::reflect_changes_part2(kdms_catalog *catalog)
{
  if (flags & KDMS_CATFLAG_NEEDS_RELOAD)
    [catalog reloadItem:item];
  if (flags & KDMS_CATFLAG_NEEDS_EXPAND)
    [catalog expandItem:item];
  flags &= ~(KDMS_CATFLAG_NEEDS_RELOAD | KDMS_CATFLAG_NEEDS_EXPAND);
  if (flags & KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION)
    {
      kdms_catalog_node *child;
      if (flags & KDMS_CATFLAG_HAS_METADATA)
        child = cur_entry->child_info.children;
      else
        child = child_info->children;
      for (; child != NULL; child=child->next_sibling)
        child->reflect_changes_part2(catalog);
    }
  flags &= ~KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
}

/*****************************************************************************/
/*                        kdms_catalog_node::compare                         */
/*****************************************************************************/

int kdms_catalog_node::compare(kdms_catalog_node *ref, int num_chars)
{
  bool full_compare = ((num_chars == 0) &&
                       (flags & KDMS_CATFLAG_HAS_METADATA) &&
                       (ref->flags & KDMS_CATFLAG_HAS_METADATA));
  if ((num_chars <= 0) || (num_chars > KDMS_CATALOG_PREFIX_LEN))
    num_chars = KDMS_CATALOG_PREFIX_LEN;
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
/*             kdms_catalog_node::find_best_collapse_candidate               */
/*****************************************************************************/

#define KDMS_MAX_AUTO_COLLAPSE_HISTORY 20

kdms_catalog_node *
  kdms_catalog_node::find_best_collapse_candidate(int depth,
                                        kdms_catalog_selection *history,
                                        int &most_recent_use)
{
  if (depth == 0)
    return NULL;
  kdms_catalog_node *node;
  if (flags & KDMS_CATFLAG_HAS_METADATA)
    node = cur_entry->child_info.children;
  else
    node = child_info->children;
  kdms_catalog_node *best_node = NULL;
  for (; (node != NULL) && (most_recent_use < KDMS_MAX_AUTO_COLLAPSE_HISTORY);
       node = node->next_sibling)
    {
      if (node->flags & KDMS_CATFLAG_NEEDS_EXPAND)
        continue;
      if (!(node->flags & KDMS_CATFLAG_IS_EXPANDED))
        continue;
      if (depth > 1)
        { // Need to go further down
          kdms_catalog_node *candidate =
            node->find_best_collapse_candidate(depth-1,history,
                                               most_recent_use);
          if (candidate != NULL)
            best_node = candidate;
        }
      else
        { // We are at the right level; consider candidates here
          if (node->flags & KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION)
            continue;
          kdms_catalog_selection *hscan = history;
          int selection_age = 0;
          for (; (hscan != NULL) && (hscan->prev != NULL) &&
                 (selection_age < KDMS_MAX_AUTO_COLLAPSE_HISTORY);
               selection_age++, hscan=hscan->prev)
            { // See if `hscan' uses `node'
              kdms_catalog_entry *entry =
                get_entry_for_metanode(hscan->metanode);
              kdms_catalog_node *user;
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
/*          kdms_catalog_node::needs_client_requests_if_expanded             */
/*****************************************************************************/

bool kdms_catalog_node::needs_client_requests_if_expanded()
{
  if (parent == NULL)
    return true;
  if (flags & KDMS_CATFLAG_HAS_METADATA)
    {
      kdms_catalog_entry *entry = cur_entry;
      for (; entry != NULL; entry=entry->next)
        {
          if (entry->incomplete)
            return true;
          if (!(flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS))
            break;
        }
    }
  kdms_catalog_child_info *info = child_info;
  if (flags & KDMS_CATFLAG_HAS_METADATA)
    info = &(cur_entry->child_info);
  kdms_catalog_node *child;
  jpx_metanode_link_type child_link_type;
  for (child=info->children; child != NULL; child=child->next_sibling)
    if ((child->flags & KDMS_CATFLAG_HAS_METADATA) &&
        (child->link_type != JPX_METANODE_LINK_NONE) &&
        !child->cur_entry->metanode.get_link(child_link_type))
      return true;
  return false;
}

/*****************************************************************************/
/*               kdms_catalog_node::generate_client_requests                 */
/*****************************************************************************/

int kdms_catalog_node::generate_client_requests(kdu_window *client_window)
{
  int num_metareqs = 0;
  kdu_uint32 box_types[4] = {jp2_label_4cc,jp2_cross_reference_4cc,
                             jp2_number_list_4cc,jp2_roi_description_4cc};
  kdu_uint32 recurse_types[2] = {jp2_number_list_4cc,jp2_roi_description_4cc};
  if ((flags & KDMS_CATFLAG_HAS_METADATA) && (cur_entry != NULL))
    {
      kdms_catalog_entry *scan;
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
      if (this->flags & KDMS_CATFLAG_IS_EXPANDED)
        for (scan=cur_entry; scan != NULL; scan=scan->next)
          {
            num_metareqs +=
              scan->metanode.generate_metareq(client_window,4,box_types,
                                              2,recurse_types,true);
            if (!(flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS))
              break;
          }
    }

  if (this->flags & KDMS_CATFLAG_IS_EXPANDED)
    {
      kdms_catalog_node *child;
      if (flags & KDMS_CATFLAG_HAS_METADATA)
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
/*                               kdms_catalog                                */
/*===========================================================================*/

@implementation kdms_catalog

/*****************************************************************************/
/*                       kdms_catalog:initWithFrame:...                      */
/*****************************************************************************/

-(kdms_catalog *) initWithFrame:(NSRect)frame_rect
                       pasteBar:(NSTextField *)the_paste_bar
                     backButton:(NSButton *)the_back_button
                      fwdButton:(NSButton *)the_fwd_button
                     peerButton:(NSButton *)the_peer_button
                    andRenderer:(kdms_renderer *)the_renderer
{
  self->renderer = the_renderer;
  self->paste_bar = the_paste_bar;
  self->back_button = the_back_button;
  self->fwd_button = the_fwd_button;
  self->peer_button = the_peer_button;
  need_metanode_touch = true;
  performing_collapse_expand = false;
  need_client_update = false;
  for (int n=0; n < KDMS_CATALOG_NUM_TYPES; n++)
    root_nodes[n] = NULL;
  root_attributes = nil;
  prefix_attributes = nil;
  link_attributes = nil;
  grouping_link_attributes = nil;
  alternate_parent_link_attributes = nil;
  history = NULL;
  pasted_label = NULL;
  pasted_link = NULL;
  pasted_node_to_cut = NULL;
  [super initWithFrame:frame_rect];
  
  // Configure the outline view
  NSTableColumn *outline_column = [NSTableColumn alloc];
  [outline_column initWithIdentifier:@"Outline Column"];
  [[outline_column headerCell] setStringValue:@"Metadata Catalog"];
  [outline_column setEditable:NO];
  
  [self addTableColumn:outline_column];
  [self setOutlineTableColumn:outline_column];
  [self sizeLastColumnToFit];
  [outline_column setWidth:800.0F];
  [outline_column release]; // Retained by us
  [self setSelectionHighlightStyle:
   NSTableViewSelectionHighlightStyleSourceList];
  
  // Create root string attributes dictionary
  CGFloat font_size = [NSFont
                       systemFontSizeForControlSize:NSRegularControlSize];
  root_attributes =
    [NSDictionary dictionaryWithObjectsAndKeys:
     [NSColor brownColor], NSForegroundColorAttributeName,
     [NSFont boldSystemFontOfSize:font_size], NSFontAttributeName,
     nil];
  [root_attributes retain];

  prefix_attributes =
    [NSDictionary dictionaryWithObjectsAndKeys:
     [NSColor grayColor], NSForegroundColorAttributeName,
     nil];
  [prefix_attributes retain];
  
  // Create link string attributes dictionary
  link_attributes =
    [NSDictionary dictionaryWithObjectsAndKeys:
     [NSColor blueColor], NSForegroundColorAttributeName,
     [NSNumber numberWithInt:NSUnderlineStyleSingle],
     NSUnderlineStyleAttributeName,
     nil];
  [link_attributes retain];
  
  // Create grouping link string attributes dictionary
  grouping_link_attributes =
    [NSDictionary dictionaryWithObjectsAndKeys:
     [NSColor orangeColor], NSForegroundColorAttributeName,
     [NSNumber numberWithInt:NSUnderlineStyleSingle],
     NSUnderlineStyleAttributeName,
     nil];
  [grouping_link_attributes retain];
  
  // Create alternate parent link string attributes dictionary
  alternate_parent_link_attributes =
    [NSDictionary dictionaryWithObjectsAndKeys:
     [NSColor purpleColor], NSForegroundColorAttributeName,
     [NSNumber numberWithInt:NSUnderlineStyleSingle],
     NSUnderlineStyleAttributeName,
     nil];
  [alternate_parent_link_attributes retain];
  
  // Now create the root nodes
  for (int r=0; r < KDMS_CATALOG_NUM_TYPES; r++)
    {
      root_nodes[r] = new kdms_catalog_node(r);
      root_nodes[r]->init();
    }
  
  // Inter-link the windows and responders
  [self setDataSource:self];
  [self setTarget:self];
  [self setDoubleAction:@selector(user_double_click:)];
  [self setDelegate:self];
  [back_button setTarget:self];
  [back_button setAction:@selector(back_button_click:)];
  [fwd_button setTarget:self];
  [fwd_button setAction:@selector(fwd_button_click:)];
  [peer_button setTarget:self];
  [peer_button setAction:@selector(peer_button_click:)];
  return self;
}

/*****************************************************************************/
/*                       kdms_catalog:update_metadata                        */
/*****************************************************************************/

-(void)update_metadata
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
  bool root_node_empty[KDMS_CATALOG_NUM_TYPES];
  for (n=0; n < KDMS_CATALOG_NUM_TYPES; n++)
    root_node_empty[n] = ((root_nodes[n] != NULL) &&
                           (root_nodes[n]->child_info->num_children == 0));
  
  // Remember properties of the current selection, so we can restore it later.
  NSInteger orig_row_idx = [self selectedRow];
  kdms_catalog_item *orig_selected_item = nil;
  NSInteger orig_first_visible_row = 0;
  jpx_metanode orig_selected_metanode;
  if (orig_row_idx >= 0)
    {
      orig_selected_item = (kdms_catalog_item *) [self itemAtRow:orig_row_idx];
      NSRange visible_range = [self getVisibleRows];
      orig_first_visible_row = (NSInteger) visible_range.location;
      kdms_catalog_node *node = NULL;
      if ((orig_row_idx < orig_first_visible_row) ||
          (orig_row_idx >= (orig_first_visible_row+visible_range.length)))
        orig_selected_item = nil; // Item is not visible, so don't preserve
      else
        node = [orig_selected_item get_node];
      if ((node != NULL) && (node->flags & KDMS_CATFLAG_HAS_METADATA))
        {
          assert(node->cur_entry != NULL);
          orig_selected_metanode = node->cur_entry->metanode;
        }
    }

  // Now update the metadata
  jpx_metanode metanode;
  while ((metanode = meta_manager.get_touched_nodes()).exists())
    {
      if ((pasted_node_to_cut != NULL) && (metanode == *pasted_node_to_cut))
        [self paste_label:NULL]; // Clear the paste bar
      
      jpx_metanode link_target;
      jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
      const char *label =
        get_label_and_link_info_for_metanode(metanode,link_type,link_target);
      kdms_catalog_entry *entry = get_entry_for_metanode(metanode);
      kdms_catalog_node *node = (entry==NULL)?NULL:(entry->container);
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
      kdms_catalog_entry *new_parent_entry = NULL;
      kdms_catalog_node *new_parent = NULL;
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
            new_parent = root_nodes[KDMS_CATALOG_TYPE_REGIONS];
          else if (have_numlist)
            new_parent = root_nodes[KDMS_CATALOG_TYPE_ENTITIES];
          else
            new_parent = root_nodes[KDMS_CATALOG_TYPE_INDEX];
        }
      if (new_parent->flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS)
        { // Put everything under the parent's first entry
          assert(new_parent->cur_entry->prev == NULL);
          new_parent_entry = new_parent->cur_entry;
        }

      if (node != NULL)
        { // See if things have changed
          if (entry->incomplete)
            {
              if (node->flags & KDMS_CATFLAG_IS_EXPANDED)
                need_client_update = true;
              kdu_uint32 box_types[2]={jp2_label_4cc,jp2_cross_reference_4cc};
              if (metanode.check_descendants_complete(2,box_types))
                {
                  entry->incomplete = false; // Now complete
                  if (node->flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS)
                    {
                      kdms_catalog_entry *scan = node->cur_entry;
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
              kdms_catalog_entry *escan;
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
          node = new kdms_catalog_node(new_parent->flags &
                                       KDMS_CATFLAG_TYPE_MASK);
          node->init(metanode);
          assert(node->link_type == link_type);
          node = new_parent->insert(node,new_parent_entry);
        }
      
      // See if we have an incomplete link
      if ((link_type != JPX_METANODE_LINK_NONE) && (!link_target) &&
          (node->parent != NULL) &&
          (node->parent->flags & KDMS_CATFLAG_IS_EXPANDED))
        need_client_update = true;
    }
  
  performing_collapse_expand = true;
  for (n=0; n < KDMS_CATALOG_NUM_TYPES; n++)
    if (root_nodes[n] != NULL)
      {
        if (root_nodes[n]->child_info->check_new_prefix_possibilities)
          root_nodes[n]->child_info->investigate_all_prefix_possibilities();
        if (root_node_empty[n] &&
            (root_nodes[n]->child_info->num_children > 0))
          root_nodes[n]->flags |= KDMS_CATFLAG_NEEDS_EXPAND;
        root_nodes[n]->reflect_changes_part1(self);
        root_nodes[n]->reflect_changes_part2(self);
      }
  performing_collapse_expand = false;

  // Now restore the selection and the location of the selected row, if it
  // has not disappeared
  if (orig_selected_item != nil)
    {
      NSInteger new_row_idx = [self rowForItem:orig_selected_item];
      while ((new_row_idx < 0) && (orig_selected_item != nil))
        {
          // First see if the item is hidden
          kdms_catalog_node *node = [orig_selected_item get_node];
          if ((node != NULL) && (node->parent != NULL))
            {
              [self expand_context_for_node:node and_entry:NULL
                               with_cleanup:false];
              new_row_idx = [self rowForItem:orig_selected_item];
            }
          if (new_row_idx >= 0)
            break;

          // Next see if the data has been moved to a different catalog item
          orig_selected_item = nil; // No longer valid
          if (orig_selected_metanode.exists())
            { // Use metanode to find the new item with the same information
              kdms_catalog_entry *entry =
                get_entry_for_metanode(orig_selected_metanode);
              if (entry != NULL)
                {
                  orig_selected_item = entry->container->item;
                  new_row_idx = [self rowForItem:orig_selected_item];
                }
              orig_selected_metanode = jpx_metanode(); // Don't use again
            }
        }
      
      if (new_row_idx >= 0)
        { // Select the row, then scroll as required to keep location in view
          [self scrollRow:new_row_idx
            toVisibleLocation:(orig_row_idx - orig_first_visible_row)];
          NSIndexSet *rows = [NSIndexSet indexSetWithIndex:new_row_idx];
          [self selectRowIndexes:rows byExtendingSelection:NO];
        }
    }

  if (pasted_link != NULL)
    [self paste_link:(*pasted_link)]; // Repaste the link forces an update
  
  if (need_client_update)
    {
      need_client_update = false;
      if (renderer != NULL)
        renderer->update_client_metadata_of_interest();
    }
}

/*****************************************************************************/
/*                         kdms_catalog:deactivate                           */
/*****************************************************************************/

-(void)deactivate
{
  for (int n=0; n < KDMS_CATALOG_NUM_TYPES; n++)
    if (root_nodes[n] != NULL)
      {
        id root_item = root_nodes[n]->item;
        [root_item retain];
        delete root_nodes[n];
        root_nodes[n] = NULL;
        [root_item release];
      }
  self->renderer = NULL;
  [self paste_label:""];
  if (history != NULL)
    while (history->prev != NULL)
      history = history->prev;
  kdms_catalog_selection *hist;
  while ((hist = history) != NULL)
    {
      history = hist->next;
      delete hist;
    }
}

/*****************************************************************************/
/*                          kdms_catalog:dealloc                             */
/*****************************************************************************/

-(void)dealloc
{
  for (int n=0; n < KDMS_CATALOG_NUM_TYPES; n++)
    if ((root_nodes[n] != NULL) || (pasted_link != NULL) ||
        (pasted_node_to_cut != NULL))
      {
        kdu_error e; e << "You have neglected to invoke "
        "`kdms_catalog:deactivate' prior to releasing the `kdms_catalog' "
        "object.  As a result, a deferred deletion of this object may be "
        "occurring, running the risk of accessing invalid `jpx_metanode's.";
      }
  [root_attributes release];
  [prefix_attributes release];
  [link_attributes release];
  [grouping_link_attributes release];
  [alternate_parent_link_attributes release];
  if (pasted_label != NULL)
    delete[] pasted_label;
  [super dealloc];
}

/*****************************************************************************/
/*                  kdms_catalog:getFancyLabel:forLinkType                   */
/*****************************************************************************/

-(NSAttributedString *)getFancyLabel:(NSString *)label
                         forLinkType:(jpx_metanode_link_type)link_type
{
  NSAttributedString *result = [NSAttributedString alloc];
  if (link_type == JPX_METANODE_LINK_NONE)
    return [result initWithString:label attributes:root_attributes];
  else if (link_type == JPX_GROUPING_LINK)
    return [result initWithString:label attributes:grouping_link_attributes];
  else if (link_type == JPX_ALTERNATE_PARENT_LINK)
    return [result initWithString:[NSString stringWithFormat:@"%C%@",
                                   0x2196,label]
                       attributes:alternate_parent_link_attributes];
  else
    return [result initWithString:[NSString stringWithFormat:@"%C%@",
                                   0x2198,label]
                       attributes:link_attributes];
}

/*****************************************************************************/
/*                   kdms_catalog:get_selected_metanode                      */
/*****************************************************************************/

-(jpx_metanode)get_selected_metanode
{
  jpx_metanode result;
  kdms_catalog_node *node = [self get_selected_node];
  if ((node != NULL) && (node->flags & KDMS_CATFLAG_HAS_METADATA))
    {
      assert(node->cur_entry != NULL);
      result = node->cur_entry->metanode;
    }
  return result;
}

/*****************************************************************************/
/*                kdms_catalog:get_unique_selected_metanode                  */
/*****************************************************************************/

-(jpx_metanode)get_unique_selected_metanode
{
  jpx_metanode result;
  kdms_catalog_node *node = [self get_selected_node];
  if ((node != NULL) && (node->flags & KDMS_CATFLAG_HAS_METADATA) &&
      (node->cur_entry != NULL) && (node->cur_entry->prev == NULL) &&
      (node->cur_entry->next == NULL))
    result = node->cur_entry->metanode;
  return result;  
}

/*****************************************************************************/
/*         kdms_catalog:find_best_catalogued_descendant:and_distance         */
/*****************************************************************************/

-(jpx_metanode)find_best_catalogued_descendant:(jpx_metanode)node
                                  and_distance:(int &)distance_val
                             excluding_regions:(bool)no_regions
{
  jpx_metanode ref_node;
  if (history != NULL)
    ref_node = history->metanode;
  kdms_catalog_entry *entry;
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
              (tmp =
               [self find_best_catalogued_descendant:child
                                        and_distance:dist
                                   excluding_regions:no_regions]).exists())
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
                  (tmp =
                   [self find_best_catalogued_descendant:child
                                            and_distance:dist
                                       excluding_regions:no_regions]).exists())
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
              (tmp =
               [self find_best_catalogued_descendant:child
                                        and_distance:dist
                                   excluding_regions:no_regions]).exists() &&
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
/*                  kdms_catalog:select_matching_metanode                    */
/*****************************************************************************/

-(bool)select_matching_metanode:(jpx_metanode)metanode
{
  if (!metanode)
    return false;
  kdms_catalog_entry *entry = get_entry_for_metanode(metanode);
  if (entry == NULL)
    { 
      int dist = -1;
      metanode = [self find_best_catalogued_descendant:metanode
                                          and_distance:dist
                                     excluding_regions:false];
      if (metanode.exists())
        entry = get_entry_for_metanode(metanode);
    }
  if (entry == NULL)
    return false;
  kdms_catalog_node *node = entry->container;
  [self expand_context_for_node:node
                      and_entry:entry
                   with_cleanup:true];
  
  // Now select the node
  NSInteger idx = [self rowForItem:node->item];
  if (idx == [self selectedRow])
    renderer->flash_imagery_for_metanode(metanode);
  else if (idx >= 0)
    {
      NSInteger prev_sel_idx = [self selectedRow];
      NSIndexSet *rows = [NSIndexSet indexSetWithIndex:idx];
      [self selectRowIndexes:rows byExtendingSelection:NO];
      if (idx != prev_sel_idx)
        [self scrollRow:idx toVisibleLocation:-1];
    }
  return true;
}

/*****************************************************************************/
/*                        kdms_catalog:paste_label                           */
/*****************************************************************************/

-(void)paste_label:(const char *)label
{
  if (pasted_label != NULL)
    { delete[] pasted_label; pasted_label = NULL; }
  if (pasted_link != NULL)
    { delete pasted_link; pasted_link = NULL; }
  if (pasted_node_to_cut != NULL)
    { delete pasted_node_to_cut; pasted_node_to_cut = NULL; }
  if ((label == NULL) || (*label == '\0'))
    {
      [[paste_bar cell] setTitle:@""];
      [[paste_bar cell] setPlaceholderAttributedString:nil];
    }
  else
    {
      [[paste_bar cell] setTitle:[NSString stringWithUTF8String:label]];
      pasted_label = new char[strlen(label)+1];
      strcpy(pasted_label,label);
    }
}

/*****************************************************************************/
/*                        kdms_catalog:paste_link                            */
/*****************************************************************************/

-(void)paste_link:(jpx_metanode)link_target
{
  [self paste_label:NULL]; // Clears the paste bar
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
  
  NSString *paste_string = nil;
  const char *label = NULL;
  int num_streams, num_layers;
  bool rres;
  int i_param;
  void *addr_param;
  kdu_uint32 box_type = link_target.get_box_type();
  if ((label = link_target.get_label()) != NULL)
    paste_string = [NSString stringWithUTF8String:label];
  else if (link_target.get_num_regions() > 0)
    { // Represent the ROI node through its bounding box
      kdu_dims bounding_box = link_target.get_bounding_box();
      paste_string = [NSString stringWithFormat:@"<ROI: BB=%dx%d @ (%d,%d)>",
                      bounding_box.size.x,bounding_box.size.y,
                      bounding_box.pos.x,bounding_box.pos.y];
    }
  else if (link_target.get_numlist_info(num_streams,num_layers,rres))
    { // Represent the numlist node through its associations
      int n;
      const int *streams = link_target.get_numlist_codestreams();
      const int *layers = link_target.get_numlist_layers();
      bool need_comma = false;
      if (rres)
        { paste_string = @"<R"; need_comma = true; }
      else
        paste_string = @"<";
      for (n=0; n < num_streams; n++, need_comma=true)
        if (need_comma)
          paste_string = [paste_string stringByAppendingFormat:@",C%d",
                          streams[n]];
        else
          paste_string = [paste_string stringByAppendingFormat:@"C%d",
                          streams[n]];
      for (n=0; n < num_layers; n++, need_comma=true)
        if (need_comma)
          paste_string = [paste_string stringByAppendingFormat:@",L%d",
                          layers[n]];
        else
          paste_string = [paste_string stringByAppendingFormat:@"L%d",
                          layers[n]];
      paste_string = [paste_string stringByAppendingString:@">"];
    }
  else if (link_target.get_delayed(i_param,addr_param))
    paste_string = @"<in file pending save>";
  else if (box_type == jp2_xml_4cc)
    paste_string = @"<XML box>";
  else if (box_type == jp2_iprights_4cc)
    paste_string = @"<IP Rights box>";
  else
    {
      unichar signature[6] =
      { ((unichar) '<'), (unichar)((box_type>>24) & 0x00FF),
        (unichar)((box_type>>16) & 0x00FF), (unichar)((box_type>>8) & 0x00FF),
        (unichar)(box_type & 0x00FF), ((unichar) '>') };
      for (int c=1; c < 5; c++)
        if ((signature[c] < ((unichar)'A')) || (signature[c] > ((unichar)'z')))
          signature[c] = ((unichar)'_');
      paste_string = [NSString stringWithCharacters:signature length:6];
    }

  NSAttributedString *rich_string =
    [[NSAttributedString alloc] initWithString:paste_string
                                    attributes:link_attributes];
  [[paste_bar cell] setPlaceholderAttributedString:rich_string];
  [rich_string release];
  pasted_link = new jpx_metanode;
  *pasted_link = link_target;
}

/*****************************************************************************/
/*                    kdms_catalog:paste_node_to_cut                         */
/*****************************************************************************/

-(void)paste_node_to_cut:(jpx_metanode)cut_target
{
  [self paste_label:NULL]; // Clears the paste bar
  if ((!cut_target) || cut_target.is_deleted())
    return;
  const char *label = cut_target.get_label();
  if (label == NULL)
    return;
  
  NSString *paste_string = [NSString stringWithUTF8String:label];
  NSDictionary *cut_attributes =
    [NSDictionary dictionaryWithObjectsAndKeys:
     [NSNumber numberWithInt: NSUnderlineStyleSingle|NSUnderlinePatternSolid],
     NSStrikethroughStyleAttributeName, nil];
  NSAttributedString *rich_string =
    [[NSAttributedString alloc] initWithString:paste_string
                                    attributes:cut_attributes];
  [[paste_bar cell] setPlaceholderAttributedString:rich_string];
  [rich_string release];
  pasted_node_to_cut = new jpx_metanode;
  *pasted_node_to_cut = cut_target;
}

/*****************************************************************************/
/*                     kdms_catalog:get_pasted_label                         */
/*****************************************************************************/

-(const char *)get_pasted_label
{
  return pasted_label;
}

/*****************************************************************************/
/*                     kdms_catalog:get_pasted_link                          */
/*****************************************************************************/

-(jpx_metanode)get_pasted_link
{
  if ((pasted_link != NULL) && pasted_link->is_deleted())
    [self paste_label:NULL]; // Clears the paste bar
  if (pasted_link == NULL)
    return jpx_metanode();
  else
    return *pasted_link;
}

/*****************************************************************************/
/*                  kdms_catalog:get_pasted_node_to_cut                      */
/*****************************************************************************/

-(jpx_metanode)get_pasted_node_to_cut
{
  if ((pasted_node_to_cut != NULL) && pasted_node_to_cut->is_deleted())
    [self paste_label:NULL]; // Clears the paste bar
  if (pasted_node_to_cut == NULL)
    return jpx_metanode();
  else
    return *pasted_node_to_cut;
}

/*****************************************************************************/
/*                 kdms_catalog:generate_metadata_requests                   */
/*****************************************************************************/

-(int)generate_client_requests:(kdu_window *)client_window
{
  int num_metareqs = 0;
  jpx_meta_manager meta_manager = renderer->get_meta_manager();
  if (!meta_manager.exists())
    return 0; // Nothing to do
  
  for (int r=0; r < KDMS_CATALOG_NUM_TYPES; r++)
    {
      kdms_catalog_node *root = root_nodes[r];
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
/*                     kdms_catalog:user_double_click                        */
/*****************************************************************************/

-(IBAction)user_double_click:(NSBrowser *)sender
{
  jpx_metanode metanode = [self get_selected_metanode];
  if (!metanode)
    return;
  [self process_double_click:metanode];
}

/*****************************************************************************/
/*                    kdms_catalog:process_double_click                      */
/*****************************************************************************/

-(void)process_double_click:(jpx_metanode)metanode
{
  if (!metanode)
    return;
  kdms_catalog_entry *orig_entry = get_entry_for_metanode(metanode);
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
          [self select_matching_metanode:link_target];
        }
    }
  else
    link_target = metanode;
  if (renderer != NULL)
    { // Show imagery for the most derived node
      kdms_catalog_entry *target_entry = get_entry_for_metanode(link_target);
      kdu_uint16 target_type=0, orig_type=0;
      int ncs, nl; bool rres;
      if (target_entry != NULL)
        target_type = target_entry->container->flags & KDMS_CATFLAG_TYPE_MASK;
      else if (link_target.get_regions() != NULL)
        target_type = KDMS_CATALOG_TYPE_REGIONS;
      else if (link_target.get_numlist_info(ncs,nl,rres))
        target_type = KDMS_CATALOG_TYPE_ENTITIES;
      if (orig_entry != NULL)
        orig_type = orig_entry->container->flags & KDMS_CATFLAG_TYPE_MASK;
      if (orig_type > target_type)
        { 
          renderer->show_imagery_for_metanode(metanode);
          renderer->flash_imagery_for_metanode(metanode);
        }
      else if (target_type != KDMS_CATALOG_TYPE_INDEX)
        { 
          renderer->show_imagery_for_metanode(link_target);
          renderer->flash_imagery_for_metanode(link_target);
        }
    }
}

/*****************************************************************************/
/*                     kdms_catalog:back_button_click                        */
/*****************************************************************************/

-(IBAction)back_button_click:(NSButton *)sender
{
  if (history == NULL)
    return;
  jpx_metanode metanode = [self get_selected_metanode];
  if (metanode == history->metanode)
    {
      if (history->prev != NULL)
        {
          history = history->prev;
          [self select_matching_metanode:history->metanode];
        }
    }
  else
    [self select_matching_metanode:history->metanode];
}

/*****************************************************************************/
/*                     kdms_catalog:fwd_button_click                         */
/*****************************************************************************/

-(IBAction)fwd_button_click:(NSButton *)sender
{
  if (history == NULL)
    return;
  jpx_metanode metanode = [self get_selected_metanode];
  if (metanode == history->metanode)
    {
      if (history->next != NULL)
        {
          history = history->next;
          [self select_matching_metanode:history->metanode];
        }
    }
  else
    [self select_matching_metanode:history->metanode];
}

/*****************************************************************************/
/*                    kdms_catalog:peer_button_click                         */
/*****************************************************************************/

-(IBAction)peer_button_click:(NSButton *)sender
{
  jpx_metanode metanode = [self get_selected_metanode];
  jpx_metanode peer = find_next_peer(metanode);
  if (!peer)
    [peer_button setEnabled:NO]; // Button should never have been enabled
  else
    [self select_matching_metanode:peer];
}

/*****************************************************************************/
/*                kdms_catalog:outlineViewSelectionDidChange                 */
/*****************************************************************************/

-(void) outlineViewSelectionDidChange:(NSNotification *)notification
{
  if (renderer == NULL)
    return; // We have already deactivated ourself
  jpx_metanode metanode;
  kdms_catalog_node *node = [self get_selected_node];
  if (node != NULL)
    {
      if ((node == root_nodes[KDMS_CATALOG_TYPE_INDEX]) ||
          (node == root_nodes[KDMS_CATALOG_TYPE_ENTITIES]))
        { // Changes priority associated with certain metadata requests
          if (self->performing_collapse_expand)
            self->need_client_update = true;
          else if (renderer != NULL)
            renderer->update_client_metadata_of_interest();
        }
      if ((node->flags & KDMS_CATFLAG_HAS_METADATA) &&
          (node->cur_entry != NULL))
        metanode = node->cur_entry->metanode;
    }
  [peer_button setEnabled:(find_next_peer(metanode).exists())?YES:NO];
  if (renderer != NULL)
    renderer->flash_imagery_for_metanode(metanode);
  if (!metanode)
    { // Moved away from a meaningful selection
      [back_button setEnabled:(history != NULL)?YES:NO];
      [fwd_button setEnabled:(history != NULL)?YES:NO];
      return;
    }
  if ((history != NULL) && (history->metanode == metanode))
    { // Moved to current selection; may be a result of navigation
      [back_button setEnabled:(history->prev != NULL)?YES:NO];
      [fwd_button setEnabled:(history->next != NULL)?YES:NO];
      return;
    }
  kdms_catalog_selection *elt;
  if (history != NULL)
    { // New element selected; delete all future nodes in browsing history
      while ((elt = history->next) != NULL)
        {
          history->next = elt->next;
          delete elt;
        }
      [fwd_button setEnabled:NO];
    }
  elt = new kdms_catalog_selection;
  elt->metanode = metanode;
  elt->next = NULL;
  if ((elt->prev = history) != NULL)
    elt->prev->next = elt;
  history = elt;
  [back_button setEnabled:(history->prev !=NULL)?YES:NO];
}

/*****************************************************************************/
/*                  kdms_catalog:outlineViewItemDidExpand                    */
/*****************************************************************************/

-(void) outlineViewItemDidExpand:(NSNotification *)notification
{
  kdms_catalog_node *node = NULL;
  kdms_catalog_item *item = (kdms_catalog_item *)
  [[notification userInfo] objectForKey:@"NSObject"];
  if (item != nil)
    node = [item get_node];
  if (node == NULL)
    return;
  node->flags |= KDMS_CATFLAG_IS_EXPANDED;
  if (node->needs_client_requests_if_expanded() && (renderer != NULL))
    {
      if (self->performing_collapse_expand)
        self->need_client_update = true;
      else
        renderer->update_client_metadata_of_interest();
    }  
}

/*****************************************************************************/
/*                 kdms_catalog:outlineViewItemDidCollapse                   */
/*****************************************************************************/

-(void) outlineViewItemDidCollapse:(NSNotification *)notification
{
  kdms_catalog_node *node = NULL;
  kdms_catalog_item *item = (kdms_catalog_item *)
    [[notification userInfo] objectForKey:@"NSObject"];
  if (item != nil)
    node = [item get_node];
  if (node == NULL)
    return;
  node->flags &= ~KDMS_CATFLAG_IS_EXPANDED;
  kdms_catalog_child_info *info = node->child_info;
  if (node->flags & KDMS_CATFLAG_HAS_METADATA)
    info = &(node->cur_entry->child_info);
  if (info != NULL)
    {
      kdms_catalog_node *child;
      for (child=info->children; child != NULL; child=child->next_sibling)
        if (child->object_value != nil)
          {
            [child->object_value release];
            child->object_value = nil;
          }
    }
  if (node->needs_client_requests_if_expanded() && (renderer != NULL))
    {
      if (self->performing_collapse_expand)
        self->need_client_update = true;
      else
        renderer->update_client_metadata_of_interest();
    }
}

/*****************************************************************************/
/*                          kdms_catalog:keyDown                             */
/*****************************************************************************/

-(void)keyDown:(NSEvent *)key_event
{
  NSUInteger modifier_flags = [key_event modifierFlags];
  NSString *the_key = [key_event charactersIgnoringModifiers];
  if ([the_key length] > 0)
    {
      unichar key_char = [the_key characterAtIndex:0];
      bool is_enter=false, is_backspace=false;
      bool is_downarrow=false, is_uparrow=false;
      if (modifier_flags & NSNumericPadKeyMask)
        {
          if (key_char == NSUpArrowFunctionKey)
            is_uparrow = true;
          else if (key_char == NSDownArrowFunctionKey)
            is_downarrow = true;
        }
      else
        {
          if ((key_char == NSEnterCharacter) ||
              (key_char == NSCarriageReturnCharacter))
            is_enter = true;
          else if ((key_char == NSBackspaceCharacter) ||
                   (key_char == NSDeleteCharacter))
            is_backspace = true;
        }
      kdms_catalog_node *node = [self get_selected_node];
      if ((node != NULL) &&
          (node->flags & KDMS_CATFLAG_HAS_METADATA) &&
          (node->cur_entry != NULL))
        {
          if (is_enter)
            {
              [self process_double_click:node->cur_entry->metanode];
              return;
            }
          else if (is_backspace)
            {
              if (renderer != NULL)
                renderer->menu_MetadataDelete();
              return;
            }
          else if (!(node->flags & KDMS_CATFLAG_ANONYMOUS_ENTRIES))
            {
              if (is_uparrow && (node->cur_entry->prev != NULL))
                {
                  [self select_matching_metanode:
                   node->cur_entry->prev->metanode];
                  return;
                }
              else if (is_downarrow && (node->cur_entry->next != NULL))
                {
                  [self select_matching_metanode:
                   node->cur_entry->next->metanode];
                  return;
                }
            }
        }
    }
  [super keyDown:key_event];
}

/*****************************************************************************/
/*                         kdms_catalog:mouseDown                            */
/*****************************************************************************/

-(void)mouseDown:(NSEvent *)mouse_event
{ // Convert control-left click into right-click
  [[self window] makeFirstResponder:self];
  if ([mouse_event modifierFlags] & NSControlKeyMask)
    [self rightMouseDown:mouse_event];
  else
    [super mouseDown:mouse_event];
}

/*****************************************************************************/
/*                        kdms_catalog:rightMouseDown                        */
/*****************************************************************************/

-(void)rightMouseDown:(NSEvent *)mouse_event
{
  [[self window] makeFirstResponder:self];
  NSPoint window_point = [mouse_event locationInWindow];
  NSRect rect;
  rect.origin = [self convertPoint:window_point fromView:nil];
  rect.origin.x -= 1;
  rect.origin.y -= 1;
  rect.size.width = rect.size.height = 2;
  NSRange rows = [self rowsInRect:rect];
  if (rows.length == 1)
    {
      kdms_catalog_item *item = (kdms_catalog_item *)
        [self itemAtRow:rows.location];
      if (item != nil)
        {
          kdms_catalog_node *node = [item get_node];
          if ((node != NULL) && (node->flags & KDMS_CATFLAG_HAS_METADATA) &&
              !(node->flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS))
            {
              assert(node->cur_entry != NULL);
              if (renderer != NULL)
                renderer->edit_metanode(node->cur_entry->metanode,true);
            }
        }
    }
}

/*****************************************************************************/
/*                kdms_catalog:outlineView:toolTipforCell:...                */
/*****************************************************************************/

-(NSString *)outlineView:(NSOutlineView *)ov 
          toolTipForCell:(NSCell *)cell
                    rect:(NSRectPointer)rect
             tableColumn:(NSTableColumn *)tc
                    item:(id)item
           mouseLocation:(NSPoint)mouseLocation
{
  if (item == nil)
    return nil;
  if (item == root_nodes[KDMS_CATALOG_TYPE_INDEX]->item)
    return @"Catalogs labels and links which are not associated with "
            "specific codestreams, compositing layers, or the rendered "
            "result -- i.e., metadata which is not grouped under a JPX "
            "number-list (nlst) box.\n"
            "   It is reasonable to regard these as "
            "global index information.  Links from a global index into "
            "image entities or regions can be particularly useful.";
  else if (item == root_nodes[KDMS_CATALOG_TYPE_ENTITIES]->item)
    return @"Catalogs labels and links which are associated with "
            "specific codestreams, compositing layers, or the rendered "
            "result, but not with specific image regions.\n"
            "   Double-click items in this catalog (or hit enter) to move "
            "the image view to display the most closely related content which "
            "involves the codestreams and/or compositing layers associated "
            "with that item.";
  else if (item == root_nodes[KDMS_CATALOG_TYPE_REGIONS]->item)
    return @"Catalogs labels and links which are associated with "
            "specific spatial regions.\n"
            "   Double-click items in this catalog (or hit enter) to move the "
            "image view to display the relevant imagery and also adjust the "
            "focus box to surround the region of interest.";
  
  kdms_catalog_node *node = [item get_node];
  if ((node != NULL) && (node->flags & KDMS_CATFLAG_HAS_METADATA))
    {
      if (node->link_type != JPX_METANODE_LINK_NONE)
        return @"Double-click links (or hit enter) to move the selection to "
                "the link target, if possible.\n"
                "   Right/ctrl-click to edit\n"
                "   Arrow keys move selection between and within items\n"
                "   Del/Backspace to delete (if editor is closed)\n"
                "   \xe2\x8c\x98R to limit region-of-interest overlays to "
                "those which are semantically connected to this link only";
      else if ((node->flags & KDMS_CATFLAG_TYPE_MASK) ==
               KDMS_CATALOG_TYPE_INDEX)
        return @"Right/ctrl-click to open the editor at the link source\n"
                "   Arrow keys move selection between and within items\n"
                "   Del/Backspace to delete the link (if editor is closed)\n"
                "   \xe2\x8c\x98R to limit region-of-interest overlays to "
                "those which are semantically connected to this item only.";  
      else
        return @"Double-click (or hit enter) to dispay the relevant imagery "
                "and adjust the focus box to surround the region of "
                "interest.\n"
                "   Right/ctrl-click to open the editor at the link source\n"
                "   Arrow keys move selection between and within items\n"
                "   Del/Backspace to delete the link (if editor is closed)\n"
                "   \xe2\x8c\x98R to limit region-of-interest overlays to "
                "those which are semantically connected to this item only.";
    }

  return nil;
}

/*****************************************************************************/
/*                 kdms_catalog:outlineView:child:ofItem                     */
/*****************************************************************************/

-(id)outlineView:(NSOutlineView *)view child:(NSInteger)idx ofItem:(id)item
{
  if (renderer == NULL)
    return nil;
  if (item == nil)
    {
      if ((idx < 0) || (idx >= KDMS_CATALOG_NUM_TYPES) ||
          (root_nodes[idx]==NULL))
        return nil;
      return root_nodes[idx]->item;
    }
  kdms_catalog_node *node = [item get_node];
  if (node == NULL)
    return nil;
  kdms_catalog_child_info *info;
  if (!(node->flags & KDMS_CATFLAG_HAS_METADATA))
    info = node->child_info;
  else
    info = &(node->cur_entry->child_info);
  
  if ((idx < 0) || (idx >= info->num_children))
    return nil;
  kdms_catalog_node *child;
  for (child=info->children; idx > 0; idx--)
    child=child->next_sibling;
  return child->item;
}

/*****************************************************************************/
/*               kdms_catalog:outlineView:isItemExpandable                   */
/*****************************************************************************/

-(BOOL)outlineView:(NSOutlineView *)view isItemExpandable:(id)item
{
  if (renderer == NULL)
    return NO;
  if (item == nil)
    return TRUE;
  kdms_catalog_node *node = [item get_node];
  if (node == NULL)
    return NO;
  bool result = false;
  if (!(node->flags & KDMS_CATFLAG_HAS_METADATA))
    result = (node->child_info->num_children > 0);
  else if (node->cur_entry->child_info.num_children > 0)
    result = true;
  else if (!(node->flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS))
    result = node->cur_entry->incomplete;
  else
    { // Need to see if any of the collapsed entries are incomplete
      kdms_catalog_entry *scan;
      for (scan=node->cur_entry; scan != NULL; scan=scan->next)
        if (scan->incomplete)
          { result = true; break; }
    }
  return (result)?YES:NO;
}

/*****************************************************************************/
/*            kdms_catalog:outlineView:numberOfChildrenOfItem                */
/*****************************************************************************/

-(NSInteger) outlineView:(NSOutlineView *)view numberOfChildrenOfItem:(id)item
{
  if (renderer == NULL)
    return 0;
  if (item == nil)
    return KDMS_CATALOG_NUM_TYPES;
  kdms_catalog_node *node = [item get_node];
  if (node == NULL)
    return 0;
  int result=0;
  if (!(node->flags & KDMS_CATFLAG_HAS_METADATA))
    result = node->child_info->num_children;
  else
    result = node->cur_entry->child_info.num_children;
  return (NSInteger) result;
}

/*****************************************************************************/
/*        kdms_catalog:outlineView:objectValueForTableColumn:byItem          */
/*****************************************************************************/

-(id)outlineView:(NSOutlineView *)view
     objectValueForTableColumn:(NSTableColumn *)table_column byItem:(id)item
{
  if (renderer == NULL)
    return nil;
  kdms_catalog_node *node = [item get_node];
  if (node == NULL)
    return nil;
  if (node->object_value != nil)
    {
      if (!(node->flags & KDMS_CATFLAG_LABEL_CHANGED))
        return node->object_value;
      [node->object_value release];
      node->object_value = nil; 
    }
  node->flags &= ~KDMS_CATFLAG_LABEL_CHANGED;
  
  NSString *label_string = nil;
  if (node->flags & KDMS_CATFLAG_HAS_METADATA)
    {
      int entry_idx=0;
      kdms_catalog_entry *entry;
      for (entry=node->cur_entry; entry->prev != NULL; entry=entry->prev)
        entry_idx++;
      int entry_count = entry_idx+1;
      for (entry=node->cur_entry; entry->next != NULL; entry=entry->next)
        entry_count++;
      kdu_uint16 unicode_buf[100];
      int unicode_length =
        kdms_utf8_to_unicode(node->get_label(),unicode_buf,99);
      if (unicode_length == 99)
        unicode_buf[96]=unicode_buf[97]=unicode_buf[98] = (kdu_uint16) '.';
      label_string = [[NSString alloc] initWithCharacters:unicode_buf
                                                   length:unicode_length];
      if (entry_count > 1)
        {
          if (node->flags & KDMS_CATFLAG_ANONYMOUS_ENTRIES)
            label_string = [[NSString alloc] initWithFormat:@"(%d) %@",
                            entry_count,label_string];
          else
            label_string = [[NSString alloc] initWithFormat:@"%d%C(%d) %@",
                            entry_idx+1,0x21A4,entry_count,label_string];
        }
    }
  else if (node->flags & KDMS_CATFLAG_PREFIX_NODE)
    label_string = [[NSString alloc] initWithFormat:@"%S...",
                    node->label_prefix];
  else // Should be a root label
    label_string = [[NSString alloc] initWithUTF8String:node->get_label()];
  
  if (node->flags & KDMS_CATFLAG_PREFIX_NODE)
    node->object_value =
      [[NSAttributedString alloc] initWithString:label_string
                                      attributes:prefix_attributes];
  else if ((node->flags & KDMS_CATFLAG_HAS_METADATA) &&
           (node->link_type == JPX_METANODE_LINK_NONE))
    node->object_value = label_string;
  else
    {
      node->object_value = [self getFancyLabel:label_string
                                   forLinkType:node->link_type];
      [label_string release];
    }
  return node->object_value;
}

/*****************************************************************************/
/*                     kdms_catalog:get_selected_node                        */
/*****************************************************************************/

-(kdms_catalog_node *)get_selected_node
{
  NSInteger idx = [self selectedRow];
  if (idx >= 0)
    {
      kdms_catalog_item *item = (kdms_catalog_item *) [self itemAtRow:idx];
      if (item != nil)
        return [item get_node];
    }
  return NULL;
}

/*****************************************************************************/
/*        kdms_catalog:expand_contex_for_node:and_entry:with_cleanup         */
/*****************************************************************************/

-(void)expand_context_for_node:(kdms_catalog_node *)node
                     and_entry:(kdms_catalog_entry *)entry
                  with_cleanup:(bool)cleanup
{
  if (entry == NULL)
    entry = node->cur_entry;
  assert(entry->container == node);

  // Find which nodes need to be expanded
  bool needs_attention=false, already_expanded=false;
  int depth_to_first_expand = -10000; // Ridiculously small value
  kdms_catalog_node *scan, *root=NULL;
  for (scan=node; scan != NULL; depth_to_first_expand++,
       root=scan, entry=scan->parent_entry, scan=scan->parent)
    {
      if (needs_attention)
        scan->flags |= KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
      if (entry != scan->cur_entry)
        {
          if (scan->flags & KDMS_CATFLAG_COLLAPSED_DESCENDANTS)
            {
              assert(scan->cur_entry->prev == NULL);
              entry = scan->cur_entry;
            }
          else
            {
              scan->cur_entry = entry;
              if (!(scan->flags & KDMS_CATFLAG_ANONYMOUS_ENTRIES))
                {
                  scan->flags |= (KDMS_CATFLAG_NEEDS_RELOAD |
                                  KDMS_CATFLAG_LABEL_CHANGED);
                  needs_attention = true;
                }
              if (scan->flags & KDMS_CATFLAG_IS_EXPANDED)
                {
                  scan->flags |= KDMS_CATFLAG_NEEDS_COLLAPSE;
                  needs_attention = true;
                }
              if (scan != node)
                {
                  scan->flags |= KDMS_CATFLAG_NEEDS_EXPAND;
                  needs_attention = true;
                  depth_to_first_expand = 0;
                }
            }
        }
      else if (already_expanded || (scan->flags & KDMS_CATFLAG_IS_EXPANDED))
        already_expanded = true;
      else if (scan != node)
        {
          scan->flags |= KDMS_CATFLAG_NEEDS_EXPAND;
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
              kdms_catalog_node *best =
                root->find_best_collapse_candidate(d,history,most_recent_use);
              if (best != NULL)
                {
                  best->flags |= KDMS_CATFLAG_NEEDS_COLLAPSE;
                  for (best=best->parent; best != NULL; best=best->parent)
                    {
                      if (!(best->flags &
                        (KDMS_CATFLAG_NEEDS_EXPAND |
                         KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION)))
                        best->flags |= KDMS_CATFLAG_NEEDS_COLLAPSE;
                      best->flags |= KDMS_CATFLAG_DESCENDANT_NEEDS_ATTENTION;
                    }
                  break;
                }
            }
        }
      performing_collapse_expand = true;
      root->reflect_changes_part1(self);
      root->reflect_changes_part2(self);
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
/*                         kdms_catalog:getVisibleRows                       */
/*****************************************************************************/

-(NSRange)getVisibleRows
{
  NSRect rect = [[self enclosingScrollView] documentVisibleRect];
  return [self rowsInRect:rect];
}

/*****************************************************************************/
/*                   kdms_catalog:scrollRow:toVisibleLocation                */
/*****************************************************************************/

-(void)scrollRow:(NSInteger)row_idx toVisibleLocation:(NSInteger)pos
{
  if (row_idx < 0)
    return; // Illegal row index
  NSRange cur_range = [self getVisibleRows];
  NSInteger cur_first_row = (NSInteger)(cur_range.location-1);
  NSInteger cur_last_row = cur_first_row + (NSInteger)(cur_range.length-1);
  NSInteger scroll_to_idx = -1;
  if ((pos >= 0) && (pos < cur_range.length))
    { // Known row position
      NSInteger new_first_row = row_idx - pos;
      NSInteger new_last_row = new_first_row+(NSInteger)(cur_range.length-1);
      if (new_first_row < cur_first_row)
        scroll_to_idx = new_first_row;
      else if (new_last_row > cur_last_row)
        scroll_to_idx = new_last_row;
      else
        scroll_to_idx = row_idx;
    }
  else
    { // Try to make the row appear in the central third of the display
      NSInteger middle_first_row = (2*cur_first_row+cur_last_row)/3;
      NSInteger middle_last_row = (cur_first_row+2*cur_last_row)/3;
      if (row_idx < middle_first_row)
        { // Scroll up
          scroll_to_idx = cur_first_row + (row_idx-middle_first_row);
          if (scroll_to_idx < 0)
            scroll_to_idx = 0;
        }
      else if (row_idx > middle_last_row)
        { // Scroll down
          scroll_to_idx = cur_last_row + (row_idx-middle_last_row);
        }
    }
  NSInteger num_rows = [self numberOfRows];
  if (scroll_to_idx >= num_rows)
    scroll_to_idx = num_rows-1;
  if (scroll_to_idx >= 0)
    [self scrollRowToVisible:scroll_to_idx];
}

@end // kdms_catalog
