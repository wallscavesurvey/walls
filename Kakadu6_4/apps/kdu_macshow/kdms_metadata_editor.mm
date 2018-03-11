/*****************************************************************************/
// File: kdms_metadata_editor.mm [scope = APPS/MACSHOW]
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
    Implements the `kdms_metadata_editor' Objective-C class.
******************************************************************************/
#include <sys/xattr.h>
#import "kdms_metadata_editor.h"
#import "kdms_catalog.h"
#import "kdms_window.h"

/*===========================================================================*/
/*                           INTERNAL FUNCTIONS                              */
/*===========================================================================*/

/*****************************************************************************/
/* STATIC                     get_nonneg_integer                             */
/*****************************************************************************/

static void get_nonneg_integer(NSTextField *field, int &val)
{
  NSString *string = [[field cell] stringValue];
  const char *str = [string UTF8String];
  val = 0;
  for (; *str != '\0'; str++)
    if (!isdigit(*str))
      break;
    else
      val = (val*10) + (int)(*str - '0');
}

/*****************************************************************************/
/* STATIC                   check_integer_in_range                           */
/*****************************************************************************/

static bool check_integer_in_range(NSTextField *field, int min_val,
                                   int max_val)
{
  NSString *string = [[field cell] stringValue];
  const char *str = [string UTF8String];
  for (; *str != '\0'; str++)
    if (!isdigit(*str))
      break;
  if (*str == '\0')
    {
      int val = [string intValue];
      if ((val >= min_val) && (val <= max_val))
        return true;
    }
  [field selectText:field];
  NSAlert *alert = [[NSAlert alloc] init];
  [alert addButtonWithTitle:@"OK"];
  [alert setMessageText:
   [NSString stringWithFormat:@"Text field requires integer in range %d to %d",
    min_val,max_val]];
  [alert setAlertStyle:NSCriticalAlertStyle];
  [alert runModal];
  [alert release];  
  return false;
}

/*****************************************************************************/
/* STATIC                     make_array_of_items                            */
/*****************************************************************************/

NSArray *make_array_of_items(int num_indices, int *indices)
  /* Returns an auto-released array whose elements are string representations
   of the supplied `indices'.  If `indices' is NULL, the array's titles are
   the integers in the range 1 to `num_indices'. */
{
  NSString **strings = new NSString *[num_indices];
  for (int n=0; n < num_indices; n++)
    strings[n] = [NSString stringWithFormat:@"%d",
                  ((indices==NULL)?(n+1):indices[n])];
  NSArray *result = [NSArray arrayWithObjects:strings count:num_indices];
  delete[] strings;
  return result;
}

/*****************************************************************************/
/* STATIC                         copy_file                                  */
/*****************************************************************************/

static void
  copy_file(kdms_file *existing_file, kdms_file *new_file)
{
  if (existing_file == new_file)
    return;
  kdu_byte buf[512];
  size_t xfer_bytes;
  FILE *src = fopen(existing_file->get_pathname(),"rb");
  if (src == NULL)
    return;
  FILE *dst = fopen(new_file->get_pathname(),"wb");
  if (dst != NULL)
    {
      while ((xfer_bytes = fread(buf,1,512,src)) > 0)
        fwrite(buf,1,xfer_bytes,dst);
      fclose(dst);
    }
  fclose(src);
  size_t attr_buf_size = getxattr(existing_file->get_pathname(),
                                  "com.apple.TextEncoding",NULL,0,0,0);
  if (attr_buf_size != 0)
    {
      char *attr_buf = new char[attr_buf_size+1];
      getxattr(existing_file->get_pathname(),
               "com.apple.TextEncoding",attr_buf,attr_buf_size,0,0);
      setxattr(new_file->get_pathname(),
               "com.apple.TextEncoding",attr_buf,attr_buf_size,0,0);
      delete[] attr_buf;
    }
}

/*****************************************************************************/
/* STATIC                   choose_editor_in_finder                          */
/*****************************************************************************/

static kdms_file_editor *
  choose_editor_in_finder(kdms_file_services *file_server,
                          const char *doc_suffix)
{
  NSArray *file_types = [NSArray arrayWithObject:@"app"];
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  [panel setAllowsMultipleSelection:NO];
  if (([panel runModalForTypes:file_types] == NSOKButton) &&
      ([panel filenames].count > 0))
    {
      const char *filename = [[[panel filenames] objectAtIndex:0] UTF8String];
      return file_server->add_editor_for_doc_type(doc_suffix,filename);
    }
  return NULL;
}

/*****************************************************************************/
/* STATIC                 choose_open_file_in_finder                         */
/*****************************************************************************/

static const char *
  choose_open_file_in_finder(const char *suffix, kdms_file *existing_file)
  /* Allows the user to select an existing file, with the indicated suffix
   (extension) in the finder.  If `existing_file' is non-NULL, the function
   uses the existing file's pathname to decide which directory to start the
   finder in.  If the user selects a file, the function returns a non-NULL
   pathname, which can be safely used until the run-loop's auto-release
   pool is visited. */
{
  NSArray *file_types = [NSArray arrayWithObject:
                         [NSString stringWithUTF8String:suffix]];
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  [panel setAllowsMultipleSelection:NO];
  NSString *directory = nil;
  if (existing_file != NULL)
    {
      const char *sep, *pathname = existing_file->get_pathname();
      if ((pathname != NULL) && ((sep=strrchr(pathname,'/')) != NULL))
        {                                
          directory = [NSString stringWithUTF8String:pathname];
          directory = [directory substringToIndex:(NSUInteger)(sep-pathname)];
        }
    }
  if ([panel runModalForDirectory:directory
                             file:nil types:file_types] != NSOKButton)
    return NULL;
  if ([panel filenames].count <= 0)
    return NULL;
  return [[[panel filenames] objectAtIndex:0] UTF8String];
}

/*****************************************************************************/
/* STATIC                 choose_save_file_in_finder                         */
/*****************************************************************************/

static const char *
  choose_save_file_in_finder(const char *suffix, kdms_file *existing_file)
  /* Same as `choose_open_file_in_finder' except that you are choosing a
   file to save to, rather than an existing file to use. */
{
  NSSavePanel *panel = [NSSavePanel savePanel];
  [panel setTitle:@"Save the metadata box's contents to a file"];
  [panel setCanCreateDirectories:YES];
  [panel setExtensionHidden:NO];
  [panel setCanSelectHiddenExtension:YES];
  [panel setRequiredFileType:[NSString stringWithUTF8String:suffix]];
  [panel setAllowsOtherFileTypes:NO];

  NSString *directory = nil;
  if (existing_file != NULL)
    {
      const char *sep, *pathname = existing_file->get_pathname();
      if ((pathname != NULL) && ((sep=strrchr(pathname,'/')) != NULL))
        {                                
          directory = [NSString stringWithUTF8String:pathname];
          directory = [directory substringToIndex:(NSUInteger)(sep-pathname)];
        }
    }
  if ([panel runModalForDirectory:directory file:@""] != NSOKButton)
    return NULL;
  if ([panel filename] == nil)
    return NULL;
  return [[panel filename] UTF8String];
}

/*****************************************************************************/
/* STATIC                 get_file_retained_by_node                          */
/*****************************************************************************/

static kdms_file *
  get_file_retained_by_node(jpx_metanode node, kdms_file_services *file_server)
{
  int i_param;
  void *addr_param;
  kdms_file *result = NULL;
  if (node.get_delayed(i_param,addr_param) &&
      (addr_param == (void *)file_server))
    result = file_server->find_file(i_param);
  return result;
}

/*****************************************************************************/
/* STATIC             copy_descendants_with_file_retain                      */
/*****************************************************************************/

static void
  copy_descendants_with_file_retain(jpx_metanode src, jpx_metanode dst,
                                    kdms_file_services *file_server,
                                    bool copy_linkers)
{
  int c, num_descendants; src.count_descendants(num_descendants);

  for (c=0; c < num_descendants; c++)
    {
      jpx_metanode src_child = src.get_descendant(c);
      if (!src_child)
        continue;
      jpx_metanode dst_child = dst.add_copy(src_child,false,true);
      if (!dst_child)
        continue;
      kdms_file *file = get_file_retained_by_node(dst_child,file_server);
      if (file != NULL)
        file->retain(); // Retain a second time on behalf of the copy
      copy_descendants_with_file_retain(src_child,dst_child,file_server,
                                        false);
    }
  
  jpx_metanode src_linker;
  while (copy_linkers && (src_linker = src.enum_linkers(src_linker)).exists())
    {
      jpx_metanode_link_type link_type;
      jpx_metanode link_target = src_linker.get_link(link_type);
      assert(link_target == src);
      jpx_metanode parent = src_linker.get_parent();
      if (!parent)
        continue; // Should not be possible
      jpx_metanode dst_linker = parent.add_link(dst,link_type);
      copy_descendants_with_file_retain(src_linker,dst_linker,file_server,
                                        false);
    }
}


/*===========================================================================*/
/*                           kdms_matching_metalist                          */
/*===========================================================================*/

/*****************************************************************************/
/*                  kdms_matching_metalist::find_container                   */
/*****************************************************************************/

kdms_matching_metanode *
  kdms_matching_metalist::find_container(jpx_metanode node)
{
  for (; node.exists(); node=node.get_parent())
    for (kdms_matching_metanode *scan=head; scan != NULL; scan=scan->next)
      if (scan->node == node)
        return scan;
  return NULL;
}

/*****************************************************************************/
/*                    kdms_matching_metalist::find_member                    */
/*****************************************************************************/

kdms_matching_metanode *
  kdms_matching_metalist::find_member(jpx_metanode node)
{
  for (kdms_matching_metanode *scan=head; scan != NULL; scan=scan->next)
    {
      jpx_metanode scan_node = scan->node;
      for (; scan_node.exists(); scan_node=scan_node.get_parent())
        if (scan_node == node)
          return scan;
    }
  return NULL;
}

/*****************************************************************************/
/*                   kdms_matching_metalist::delete_item                     */
/*****************************************************************************/

void kdms_matching_metalist::delete_item(kdms_matching_metanode *item)
{
  if (item->prev == NULL)
    {
      assert(item == head);
      head = item->next;
    }
  else
    item->prev->next = item->next;
  if (item->next == NULL)
    {
      assert(item == tail);
      tail = item->prev;
    }
  else
    item->next->prev = item->prev;
  delete item;
}

/*****************************************************************************/
/*                    kdms_matching_metalist::append_node                    */
/*****************************************************************************/

kdms_matching_metanode *kdms_matching_metalist::append_node(jpx_metanode node)
{
  kdms_matching_metanode *elt = new kdms_matching_metanode;
  elt->node = node;
  elt->next = NULL;
  if ((elt->prev = tail) == NULL)
    head = tail = elt;
  else
    tail = tail->next = elt;
  return elt;
}

/*****************************************************************************/
/*   kdms_matching_metalist::append_node_expanding_numlists_and_free_boxes   */
/*****************************************************************************/

void
  kdms_matching_metalist::append_node_expanding_numlists_and_free_boxes(
                                                            jpx_metanode node)
{
  int num_cs, num_layers;
  bool applies_to_rendered_result;
  if (node.get_numlist_info(num_cs,num_layers,applies_to_rendered_result) ||
      (node.get_box_type() == jp2_free_4cc))
    {
      jpx_metanode child;
      for (int c=0; (child=node.get_descendant(c)).exists(); c++)
        append_node_expanding_numlists_and_free_boxes(child);
    }
  else
    append_node(node);
}

/*****************************************************************************/
/*             kdms_matching_metalist::append_image_wide_nodes               */
/*****************************************************************************/

void kdms_matching_metalist::append_image_wide_nodes(jpx_meta_manager manager,
                                                     int layer_idx,
                                                     int stream_idx)
{
  jpx_metanode node;
  if (layer_idx >= 0)
    {
      node = jpx_metanode();
      while ((node = manager.enumerate_matches(node,-1,layer_idx,
                                               false,kdu_dims(),0,
                                               true,true)).exists())
        append_node_expanding_numlists_and_free_boxes(node);
    }
  if (stream_idx >= 0)
    {
      node = jpx_metanode();
      while ((node = manager.enumerate_matches(node,stream_idx,layer_idx,
                                               false,kdu_dims(),0,
                                               true,true)).exists())
        append_node_expanding_numlists_and_free_boxes(node);      
    }
  
  node = jpx_metanode();
  while ((node = manager.enumerate_matches(node,-1,-1,true,kdu_dims(),
                                           0,true)).exists())
    append_node_expanding_numlists_and_free_boxes(node);
  
  node = jpx_metanode();
  while ((node = manager.enumerate_matches(node,-1,-1,false,kdu_dims(),
                                           0,true)).exists())
    append_node_expanding_numlists_and_free_boxes(node);
  
  // Now go through and eliminate any nodes which are descended from or
  // equal to others in the list.
  kdms_matching_metanode *scan, *next;
  for (scan=head; scan != NULL; scan=next)
    {
      next = scan->next;
      
      jpx_metanode ancestor;
      for (ancestor=scan->node;
           ancestor.exists(); ancestor=ancestor.get_parent())
        {
          kdms_matching_metanode *test;
          for (test=head; test != NULL; test=test->next)
            if ((test != scan) && (test->node == ancestor))
              break;
          if (test != NULL)
            break;
        }
      if (ancestor.exists())
        delete_item(scan); // One of `scan's ancestors is already in the list
      
    }
}


/*===========================================================================*/
/*                          kdms_metanode_edit_state                         */
/*===========================================================================*/

/*****************************************************************************/
/*           kdms_metanode_edit_state::kdms_metanode_edit_state              */
/*****************************************************************************/

kdms_metanode_edit_state::kdms_metanode_edit_state(jpx_meta_manager manager,
                                              kdms_file_services *file_server)
{
  this->meta_manager = manager;
  metalist = &internal_metalist;
  edit_item = NULL;
  cur_node_sibling_idx = 0;
  next_sibling_idx = prev_sibling_idx = -1;
  this->file_server = file_server;
}

/*****************************************************************************/
/*                 kdms_metanode_edit_state::move_to_parent                  */
/*****************************************************************************/

void kdms_metanode_edit_state::move_to_parent()
{
  if ((cur_node == root_node) || !cur_node.exists())
    return;
  cur_node = cur_node.get_parent();
  cur_node_sibling_idx = next_sibling_idx = prev_sibling_idx = -1;
  if (cur_node != root_node)
    find_sibling_indices();
}

/*****************************************************************************/
/*                  kdms_metanode_edit_state::move_to_next                   */
/*****************************************************************************/

void kdms_metanode_edit_state::move_to_next()
{
  if (!cur_node.exists())
    return;
  if (cur_node == root_node)
    {
      if (edit_item->next == NULL)
        return;
      edit_item = edit_item->next;
      root_node = cur_node = edit_item->node;
    }
  else
    {
      if (next_sibling_idx < 0)
        return;
      cur_node = cur_node.get_parent().get_descendant(next_sibling_idx);
      prev_sibling_idx = cur_node_sibling_idx;
      cur_node_sibling_idx = next_sibling_idx;
      next_sibling_idx = -1; // Mark this one as unknown
      find_sibling_indices();
    }
}

/*****************************************************************************/
/*                  kdms_metanode_edit_state::move_to_prev                   */
/*****************************************************************************/

void kdms_metanode_edit_state::move_to_prev()
{
  if (!cur_node.exists())
    return;
  if (cur_node == root_node)
    {
      if (edit_item->prev == NULL)
        return;
      edit_item = edit_item->prev;
      root_node = cur_node = edit_item->node;
    }
  else
    {
      if (prev_sibling_idx < 0)
        return;
      cur_node = cur_node.get_parent().get_descendant(prev_sibling_idx);
      next_sibling_idx = cur_node_sibling_idx;
      cur_node_sibling_idx = prev_sibling_idx;
      prev_sibling_idx = -1; // Mark this one as unknown
      find_sibling_indices();
    }
}

/*****************************************************************************/
/*              kdms_metanode_edit_state::move_to_descendants                */
/*****************************************************************************/

void kdms_metanode_edit_state::move_to_descendants()
{
  if (!cur_node.exists())
    return;
  jpx_metanode child;
  int c;
  for (c=0; (child = cur_node.get_descendant(c)).exists(); c++)
    {
      bool rres;
      int num_cs, num_l;
      if (!child.get_numlist_info(num_cs,num_l,rres))
        break;
    }
  if (!child.exists())
    return;
  cur_node = child;
  cur_node_sibling_idx = c;
  prev_sibling_idx = -1;
  next_sibling_idx = -1;
  find_sibling_indices();
}

/*****************************************************************************/
/*                 kdms_metanode_edit_state::move_to_node                    */
/*****************************************************************************/

void kdms_metanode_edit_state::move_to_node(jpx_metanode node)
{
  kdms_matching_metanode *container = metalist->find_container(node);
  if (container == NULL)
    return; // Do nothing
  edit_item = container;
  root_node = edit_item->node;
  cur_node = node;
  cur_node_sibling_idx = -1;
  prev_sibling_idx = -1;
  next_sibling_idx = -1;
  if (node != root_node)
    find_sibling_indices();
}

/*****************************************************************************/
/*                kdms_metanode_edit_state::delete_cur_node                  */
/*****************************************************************************/

void
  kdms_metanode_edit_state::delete_cur_node(kdms_renderer *renderer)
{
  jpx_metanode node_to_delete = cur_node;
  jpx_metanode next_node;
  if (cur_node != root_node)
    { // Deleting a descendant of a top-level node.
      jpx_metanode parent = cur_node.get_parent();
      if (prev_sibling_idx >= 0)
        next_node = parent.get_descendant(prev_sibling_idx);
      else if (next_sibling_idx >= 0)
        next_node = parent.get_descendant(next_sibling_idx);
      else
        next_node = parent;
      cur_node_sibling_idx = prev_sibling_idx = next_sibling_idx = -1;
    }
  
  cur_node = jpx_metanode(); // Temporarily get rid of it
  
  int i_param;
  void *addr_param;
  if (node_to_delete.get_delayed(i_param,addr_param) &&
      (addr_param == (void *)file_server))
    {
      kdms_file *file = file_server->find_file(i_param);
      if (file != NULL)
        file->release();
    }
  node_to_delete.delete_node();
  validate_metalist_and_root_node();
  if ((!next_node) || next_node.is_deleted())
    {
      cur_node = root_node;
      cur_node_sibling_idx = prev_sibling_idx = next_sibling_idx = -1;
    }
  else
    cur_node = next_node;  
  if (cur_node != root_node)
    find_sibling_indices();
  renderer->metadata_changed(false);
}

/*****************************************************************************/
/*        kdms_metanode_edit_state::validate_metalist_and_root_node          */
/*****************************************************************************/

void kdms_metanode_edit_state::validate_metalist_and_root_node()
{
  kdms_matching_metanode *item, *next_item;
  for (item=metalist->get_head(); item != NULL; item=next_item)
    {
      next_item = item->next;
      if (!item->node.is_deleted())
        continue;
      if (item == edit_item)
        {
          edit_item = (edit_item->prev!=NULL)?edit_item->prev:edit_item->next;
          root_node = (edit_item!=NULL)?edit_item->node:jpx_metanode();
          cur_node_sibling_idx = prev_sibling_idx = next_sibling_idx = -1;
        }
      metalist->delete_item(item);
    }
}

/*****************************************************************************/
/*                 kdms_metanode_edit_state::add_child_node                  */
/*****************************************************************************/

void
  kdms_metanode_edit_state::add_child_node(kdms_renderer *renderer)
{
  assert(cur_node.exists());
  jpx_metanode new_node;
  try {
    new_node = cur_node.add_label("<new label>");
  }
  catch (kdu_exception) {
    return;
  }
  if (new_node.exists())
    {
      cur_node = new_node;
      cur_node_sibling_idx = prev_sibling_idx = next_sibling_idx = -1;
      find_sibling_indices();
      renderer->metadata_changed(true);
    }
}

/*****************************************************************************/
/*                kdms_metanode_edit_state::add_parent_node                  */
/*****************************************************************************/

void
  kdms_metanode_edit_state::add_parent_node(kdms_renderer *renderer)
{
  assert(cur_node.exists());
  jpx_metanode parent = cur_node.get_parent();
  jpx_metanode new_parent;
  try {
    new_parent = parent.add_label("<new parent>");
    cur_node.change_parent(new_parent);
  }
  catch (kdu_exception) {
    renderer->metadata_changed(true); // Just in case one op succeeded above
    return;
  }
  if (cur_node == root_node)
    {
      root_node = new_parent;
      if (edit_item != NULL)
        edit_item->node = new_parent;
    }
  cur_node = new_parent;
  cur_node_sibling_idx = prev_sibling_idx = next_sibling_idx = -1;
  if (cur_node != root_node)
    find_sibling_indices();
  renderer->metadata_changed(true);
}

/*****************************************************************************/
/*              kdms_metanode_edit_state::find_sibling_indices               */
/*****************************************************************************/

void kdms_metanode_edit_state::find_sibling_indices()
{
  assert(cur_node != root_node);
  jpx_metanode parent = cur_node.get_parent();
  jpx_metanode sibling;
  int num_cs, num_l;
  bool rres;
  if (cur_node_sibling_idx < 0)
    {
      for (cur_node_sibling_idx=0;
           (sibling=parent.get_descendant(cur_node_sibling_idx)).exists();
           cur_node_sibling_idx++)
        if (sibling == cur_node)
          break;
        else if (!sibling.get_numlist_info(num_cs,num_l,rres))
          prev_sibling_idx = cur_node_sibling_idx;
      assert(sibling == cur_node);
    }
  else if (prev_sibling_idx < 0)
    {
      for (prev_sibling_idx=cur_node_sibling_idx-1;
           prev_sibling_idx >= 0;
           prev_sibling_idx--)
        {
          sibling = parent.get_descendant(prev_sibling_idx);
          if (!sibling.get_numlist_info(num_cs,num_l,rres))
            break;
        }
    }
  if (next_sibling_idx < 0)
    {
      for (next_sibling_idx=cur_node_sibling_idx+1;
           (sibling=parent.get_descendant(next_sibling_idx)).exists();
           next_sibling_idx++)
        if (!sibling.get_numlist_info(num_cs,num_l,rres))
          break;
      if (!sibling.exists())
        next_sibling_idx = -1;
    }
}


/*===========================================================================*/
/*                        kdms_metadata_dialog_state                         */
/*===========================================================================*/

/*****************************************************************************/
/*         kdms_metadata_dialog_state::kdms_metadata_dialog_state            */
/*****************************************************************************/

kdms_metadata_dialog_state::kdms_metadata_dialog_state(int num_codestreams,
                                         int num_layers,
                                         kdms_box_template *available_types,
                                         kdms_file_services *file_server,
                                         bool allow_edits)
{
  this->num_codestreams = num_codestreams;
  this->num_layers = num_layers;
  num_selected_codestreams = num_selected_layers = 0;
  selected_codestreams = new int[num_codestreams];
  selected_layers = new int[num_layers];
  this->allow_edits = allow_edits;
  is_link = is_cross_reference = false;
  can_edit_node = allow_edits;
  rendered_result = false;
  whole_image = true;
  this->available_types = available_types;
  memset(offered_types,0,sizeof(int *) * KDMS_NUM_BOX_TEMPLATES);
  num_offered_types = 1;
  offered_types[0] = available_types + KDMS_LABEL_TEMPLATE;
  selected_type = offered_types[0];
  label_string = NULL;
  this->file_server = file_server;
  external_file = NULL;
  external_file_replaces_contents = false;
}

/*****************************************************************************/
/*         kdms_metadata_dialog_state::~kdms_metadata_dialog_state           */
/*****************************************************************************/

kdms_metadata_dialog_state::~kdms_metadata_dialog_state()
{
  if (selected_codestreams != NULL)
    delete[] selected_codestreams;
  if (selected_layers != NULL)
    delete[] selected_layers;
  if (external_file != NULL)
    external_file->release();
}

/*****************************************************************************/
/*              kdms_metadata_dialog_state::compare_associations             */
/*****************************************************************************/

bool kdms_metadata_dialog_state::compare_associations(
                                          kdms_metadata_dialog_state *ref)
{
  int n;
  if (num_selected_codestreams != ref->num_selected_codestreams)
    return false;
  for (n=0; n < num_selected_codestreams; n++)
    if (selected_codestreams[n] != ref->selected_codestreams[n])
      return false;
  if (num_selected_layers != ref->num_selected_layers)
    return false;
  for (n=0; n < num_selected_layers; n++)
    if (selected_layers[n] != ref->selected_layers[n])
      return false;
  if (rendered_result != ref->rendered_result)
    return false;
  if (whole_image != ref->whole_image)
    return false;
  if (whole_image)
    return true;
  else
    return (roi_editor == ref->roi_editor);
}

/*****************************************************************************/
/*                kdms_metadata_dialog_state::compare_contents               */
/*****************************************************************************/

bool kdms_metadata_dialog_state::compare_contents(
                                          kdms_metadata_dialog_state *ref)
{
  if (selected_type != ref->selected_type)
    return false;
  if (is_link != ref->is_link)
    return false;
  if (external_file_replaces_contents || ref->external_file_replaces_contents)
    return false;
  if (((label_string != NULL) || (ref->label_string != NULL)) &&
      ((label_string == NULL) || (ref->label_string == NULL) ||
       (strcmp(label_string,ref->label_string) != 0)))
    return false;
  return true;
}

/*****************************************************************************/
/*            kdms_metadata_dialog_state::add_selected_codestream            */
/*****************************************************************************/

int kdms_metadata_dialog_state::add_selected_codestream(int idx)
{
  if ((idx < 0) || (idx >= num_codestreams))
    return -1;
  int m, n;
  for (n=0; n < num_selected_codestreams; n++)
    if (selected_codestreams[n] == idx)
      return false; // Nothing to do; already exists in the list
    else
      if (selected_codestreams[n] > idx)
        break;
  for (m=n; m < num_selected_codestreams; m++)
    selected_codestreams[m+1] = selected_codestreams[m];
  selected_codestreams[n] = idx;
  num_selected_codestreams++;
  return n;
}

/*****************************************************************************/
/*              kdms_metadata_dialog_state::add_selected_layer               */
/*****************************************************************************/

int kdms_metadata_dialog_state::add_selected_layer(int idx)
{
  if ((idx < 0) || (idx >= num_layers))
    return -1;
  int m, n;
  for (n=0; n < num_selected_layers; n++)
    if (selected_layers[n] == idx)
      return false; // Nothing to do; already exists in the list
    else
      if (selected_layers[n] > idx)
        break;
  for (m=n; m < num_selected_layers; m++)
    selected_layers[m+1] = selected_layers[m];
  selected_layers[n] = idx;
  num_selected_layers++;
  return n;
}

/*****************************************************************************/
/*            kdms_metadata_dialog_state::configure_with_metanode            */
/*****************************************************************************/

void kdms_metadata_dialog_state::configure_with_metanode(jpx_metanode node)
{
  kdu_uint32 box_type = node.get_box_type();
  can_edit_node = allow_edits;
  if (box_type == jp2_cross_reference_4cc)
    can_edit_node = false;
  is_link = is_cross_reference = false;
  jpx_fragment_list frag_list;
  if (node.get_cross_reference(frag_list) != 0)
    is_cross_reference = true;
  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
  jpx_metanode link_target = node.get_link(link_type);
  if (link_target.exists())
    {
      is_link = true;
      can_edit_node = false;
      node = link_target;
      box_type = node.get_box_type();
    }
  
  if (box_type == jp2_label_4cc)
    {
      num_offered_types = 3;
      offered_types[0] = available_types + KDMS_LABEL_TEMPLATE;
      offered_types[1] = available_types + KDMS_XML_TEMPLATE;
      offered_types[2] = available_types + KDMS_IPR_TEMPLATE;
    }
  else if (box_type == jp2_xml_4cc)
    {
      num_offered_types = 3;
      offered_types[1] = available_types + KDMS_LABEL_TEMPLATE;
      offered_types[0] = available_types + KDMS_XML_TEMPLATE;
      offered_types[2] = available_types + KDMS_IPR_TEMPLATE;
    }
  else if (box_type == jp2_iprights_4cc)
    {
      num_offered_types = 3;
      offered_types[2] = available_types + KDMS_LABEL_TEMPLATE;
      offered_types[1] = available_types + KDMS_XML_TEMPLATE;
      offered_types[0] = available_types + KDMS_IPR_TEMPLATE;
    }
  else
    {
      num_offered_types = 1;
      offered_types[0] = available_types + KDMS_CUSTOM_TEMPLATE;
      available_types[KDMS_CUSTOM_TEMPLATE].init(box_type,NULL,NULL);
    }
  if (!can_edit_node)
    num_offered_types = 1; // Do not offer other box-types
  selected_type = offered_types[0];
  label_string = node.get_label();
  external_file_replaces_contents = false;
  set_external_file(NULL);
  {
    int i_param;
    void *addr_param;
    if (node.get_delayed(i_param,addr_param) &&
        (addr_param == (void *)file_server))
      set_external_file(file_server->find_file(i_param));
  }
  
  whole_image = true;
  roi_editor.reset();
  rendered_result = false;
  num_selected_codestreams = num_selected_layers = 0;
  
  jpx_metanode scan;
  for (scan=node; scan.exists(); scan=scan.get_parent())
    {
      bool rres;
      int n, num_cs, num_l;
      if (scan.get_numlist_info(num_cs,num_l,rres))
        {
          const int *streams = scan.get_numlist_codestreams();
          for (n=0; n < num_cs; n++)
            add_selected_codestream(streams[n]);
          const int *layers = scan.get_numlist_layers();
          for (n=0; n < num_l; n++)
            add_selected_layer(layers[n]);
          if (rres)
            rendered_result = true;
        }
      else if (whole_image && (scan.get_num_regions() > 0))
        {
          whole_image = false;
          roi_editor.init(scan.get_regions(),scan.get_num_regions());
        }
    }
}

/*****************************************************************************/
/*                     kdms_metadata_dialog_state::copy                      */
/*****************************************************************************/

void kdms_metadata_dialog_state::copy(kdms_metadata_dialog_state *src)
{
  assert((num_codestreams == src->num_codestreams) &&
         (num_layers == src->num_layers));
  num_selected_codestreams = src->num_selected_codestreams;
  num_selected_layers = src->num_selected_layers;
  memcpy(selected_codestreams,src->selected_codestreams,
         (size_t)(sizeof(int)*num_selected_codestreams));
  memcpy(selected_layers,src->selected_layers,
         (size_t)(sizeof(int)*num_selected_layers));
  rendered_result = src->rendered_result;
  allow_edits = src->allow_edits;
  can_edit_node = src->can_edit_node;
  is_link = src->is_link;
  is_cross_reference = src->is_cross_reference;
  whole_image = src->whole_image;
  roi_editor.copy_from(src->roi_editor);
  num_offered_types = src->num_offered_types;
  selected_type = src->selected_type;
  external_file_replaces_contents = src->external_file_replaces_contents;
  this->set_external_file(src->external_file);
  label_string = src->label_string;
  for (int n=0; n < num_offered_types; n++)
    offered_types[n] = src->offered_types[n];
}

/*****************************************************************************/
/*            kdms_metadata_dialog_state::save_to_external_file              */
/*****************************************************************************/

bool kdms_metadata_dialog_state::save_to_external_file(
                                               kdms_metanode_edit_state *edit)
{
  if (external_file == NULL)
    set_external_file(file_server->retain_tmp_file(selected_type->file_suffix),
                      true);
  if ((selected_type->box_type == jp2_label_4cc) && (label_string != NULL))
    { // Save the label string
      NSString *ns_label = [NSString stringWithUTF8String:label_string];
      NSString *ns_path = [NSString stringWithUTF8String:
                           external_file->get_pathname()];
      [ns_label writeToFile:ns_path atomically:NO
                   encoding:NSUTF8StringEncoding error:NULL];
    }
  else if (edit->cur_node.exists())
    {
      jp2_input_box box;
      if (edit->cur_node.open_existing(box))
        {
          const char *fopen_mode = "w";
          if (selected_type->initializer == NULL)
            fopen_mode = "wb"; // Write unrecognized boxes as binary
          FILE *file = fopen(external_file->get_pathname(),fopen_mode);
          if (file != NULL)
            {
              kdu_byte buf[512];
              int xfer_bytes;
              while ((xfer_bytes = box.read(buf,512)) > 0)
                fwrite(buf,1,(size_t)xfer_bytes,file);
              fclose(file);
              return true;
            }
        }
    }
  return false;
}

/*****************************************************************************/
/*           kdms_metadata_dialog_state::internalize_label_node              */
/*****************************************************************************/

bool kdms_metadata_dialog_state::internalize_label_node()
{
  if ((external_file == NULL) ||
      (selected_type->box_type != jp2_label_4cc))
    return false;
  
  const char *pathname = external_file->get_pathname();
  FILE *fp = fopen(pathname,"r");
  if (fp == NULL)
    return false;
  
  char *tmp_label = new char[4097];
  size_t num_chars = fread(tmp_label,1,4097,fp);
  fclose(fp);
  if (num_chars > 4096)
    {
      interrogate_user("External file contains more than 4096 characters!  "
                       "Managing internal labels of this size can become "
                       "impractical -- you should consider using an XML "
                       "box for large amounts of text.","OK");
      delete[] tmp_label;
      return false;
    }
  tmp_label[num_chars] = '\0';
  if (label_string != NULL)
    { delete[] label_string; label_string = NULL; }
  label_string = tmp_label;
  set_external_file(NULL);
  external_file_replaces_contents = false;
  return true;
}

/*****************************************************************************/
/*                 kdms_metadata_dialog_state::save_metanode                 */
/*****************************************************************************/

bool
  kdms_metadata_dialog_state::save_metanode(kdms_metanode_edit_state *edit,
                                            kdms_metadata_dialog_state *ref,
                                            kdms_renderer *renderer)
{
  if (!can_edit_node)
    return false;
  bool changed_associations = !compare_associations(ref);
  if (changed_associations && edit->cur_node.exists())
    {
      bool rres;
      int num_cs, num_l;
      jpx_metanode parent = edit->cur_node.get_parent();
      while (parent.exists() &&
             (parent.get_numlist_info(num_cs,num_l,rres) ||
              (parent.get_box_type() == jp2_free_4cc)))
        parent = parent.get_parent();
      if (parent.exists() && (parent.get_box_type() != 0) &&
          (edit->cur_node.get_num_regions() == 0) &&
          !interrogate_user("The changes you have made require the metadata "
                            "you have edited to be associated with a new "
                            "region of interest and/or a new set of image "
                            "entities (codestreams, layers or rendered "
                            "result).  In the process, you may lose existing "
                            "associations with other metadata in the node's "
                            "ancestry.  Consider making these changes to the "
                            "node's parent instead (click \"revert\" and "
                            "then \"Up to parent\" to do this).",
                            "Cancel Apply","Proceed with Apply"))
        return false;
    }
  
  if (changed_associations || !edit->cur_node.exists())
    { // Need to create a new set of associations
      const jpx_roi *roi_regions = NULL;
      int roi_num_regions=0;
      if (!whole_image)
        roi_regions = roi_editor.get_regions(roi_num_regions);
      if ((roi_regions != NULL) && (num_selected_codestreams == 0))
        {
          interrogate_user("The region of interest (ROI) you have selected "
                           "must be associated with at least one codestream "
                           "in order to have meaning.  The ROI dimensions "
                           "are necessarily interpreted with respect to the "
                           "coordinate system of a codestream.  To proceed, "
                           "you should either add an associated codestream "
                           "or else click the \"Whole Image\" button.",
                           "OK","");
          return false;
        }
        
      jpx_metanode container =
        edit->meta_manager.insert_node(num_selected_codestreams,
                                       selected_codestreams,
                                       num_selected_layers,
                                       selected_layers,
                                       rendered_result,
                                       roi_num_regions,roi_regions);
      jpx_metanode node = container;
      bool new_data_only = true;
      if (selected_type->box_type == jp2_roi_description_4cc)
        { // Move all the previous node's descendants to the new container;
          // also redirect all links to the previous node to point to the new
          // container.
          if (edit->cur_node.exists())
            {
              new_data_only = false;
              jpx_metanode child;
              while ((child = edit->cur_node.get_descendant(0)).exists())
                child.change_parent(container);
              jpx_metanode linker;
              while ((linker = edit->cur_node.enum_linkers(linker)).exists())
                {
                  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
                  jpx_metanode link_target = linker.get_link(link_type);
                  assert(link_target == edit->cur_node);
                  linker.change_to_link(container,link_type);
                }
              if (edit->cur_node == edit->root_node)
                { // About to delete the node referenced by `edit_item'
                  edit->metalist->delete_item(edit->edit_item);
                  edit->edit_item = NULL;
                  edit->root_node = jpx_metanode();
                }
              edit->cur_node.delete_node();
              edit->validate_metalist_and_root_node();
            }
        }
      else if (edit->cur_node.exists())
        { // Move current node to new parent
          new_data_only = false;
          node = edit->cur_node;
          kdms_file *existing_file =
          get_file_retained_by_node(node,file_server);
          node.change_parent(container);
          if (external_file_replaces_contents)
            {
              node.change_to_delayed(selected_type->box_type,
                                     external_file->get_id(),file_server);
              external_file->retain(); // Retain on behalf of the node.
              if (existing_file != NULL)
                existing_file->release();
              external_file_replaces_contents = false;
            }
          else if (label_string != NULL)
            {
              assert(selected_type->box_type == jp2_label_4cc);
              node.change_to_label(label_string);
              if (existing_file != NULL)
                existing_file->release();
            }
          else
            assert(selected_type->box_type == node.get_box_type());
        }
      else
        {
          if (external_file_replaces_contents)
            {
              node = container.add_delayed(selected_type->box_type,
                                           external_file->get_id(),
                                           file_server);
              external_file->retain(); // Retain on behalf of the node.
              external_file_replaces_contents = false;
            }
          else if (label_string != NULL)
            {
              assert(selected_type->box_type == jp2_label_4cc);
              node = container.add_label(label_string);
            }
          else
            assert(0); // Cannot currently add other box types
        }

      kdms_matching_metanode *item;
      if (roi_regions != NULL)
        { // Add `container' to the metalist as a new top-level item, unless
          // it already exists.
          item = edit->metalist->find_container(container);
          if (item == NULL)
            { // There is no item already in the metalist which holds
              // `container'.  This the most likely case.
              if ((item = edit->metalist->find_member(container)) != NULL)
                { // There is a top-level item in the metalist which is
                  // contained within the new container
                  edit->metalist->delete_item(item);
                }
              item = edit->metalist->append_node(container);
            }
        }
      else
        { // Add `node' to the metalist as a new top-level item.  It will not
          // already exist.
          item = edit->metalist->append_node(node);
        }
      edit->edit_item = item;
      edit->root_node = item->node;
      edit->cur_node = node;
      edit->cur_node_sibling_idx = -1;
      edit->next_sibling_idx = edit->prev_sibling_idx = -1;
      if (edit->cur_node != edit->root_node)
        edit->find_sibling_indices();
      renderer->metadata_changed(new_data_only);
    }
  else if (!compare_contents(ref))
    { // Just changing the box contents or box-type of an existing box
      jpx_metanode node = edit->cur_node;
      int i_param;
      void *addr_param;
      kdms_file *existing_file = NULL;
      if (node.get_delayed(i_param,addr_param) &&
          (addr_param == (void *)file_server))
        existing_file = file_server->find_file(i_param);
      if (external_file_replaces_contents)
        {
          node.change_to_delayed(selected_type->box_type,
                                 external_file->get_id(),file_server);
          external_file->retain(); // Retain on behalf of the node.
          if (existing_file != NULL)
            existing_file->release();
          external_file_replaces_contents = false;
        }
      else if (label_string != NULL)
        {
          assert(selected_type->box_type == jp2_label_4cc);
          node.change_to_label(label_string);
          if (existing_file != NULL)
            existing_file->release();
        }
      else
        return false;      
      renderer->metadata_changed(false);
    }
  
  this->configure_with_metanode(edit->cur_node);
      // This is not always necessary, but generally a good idea.  For
      // example, if we replaced the label string, we need to use this
      // function to get a valid pointer to memory that we know has not
      // been deallocated.
  return true;
}


/*===========================================================================*/
/*                          kdms_metadata_editor                             */
/*===========================================================================*/

@implementation kdms_metadata_editor

/*****************************************************************************/
/*    kdms_metadata_editor:initWithSource:fileServices:editable:owner        */
/*                               location:andIdentifier                      */
/*****************************************************************************/

- (void) initWithSource:(jpx_source *)the_source
           fileServices:(kdms_file_services *)file_services
               editable:(bool)can_edit
                  owner:(kdms_renderer *)the_renderer
               location:(NSPoint)preferred_location
           andIdentifier:(int)identifier
{
  self->source = the_source;
  self->editing_shape = false;
  self->shape_editing_mode = JPX_EDITOR_VERTEX_MODE;
  self->allow_edits = can_edit;
  self->file_server = file_services;
  self->renderer = the_renderer;
  
  num_codestreams = 1;
  num_layers = 1;
  edit_state = NULL;
  state = NULL;
  initial_state = NULL;
  owned_metalist = NULL;
  
  available_types[KDMS_LABEL_TEMPLATE].init(jp2_label_4cc,"txt",
                                            "<new label>");
  available_types[KDMS_XML_TEMPLATE].init(jp2_xml_4cc,"xml",
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r");
  available_types[KDMS_IPR_TEMPLATE].init(jp2_iprights_4cc,"xml",
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r");

  dialog_window = nil;
  x_pos_field = y_pos_field = width_field = height_field = nil;
  single_rectangle_button = nil;
  single_ellipse_button = nil;
  roi_encoded_button = nil;
  whole_image_button = nil;
  
  roi_shape_editor_button = nil;
  roi_mode_popup = nil;
  roi_complexity_level = nil;
  roi_complexity_value = nil;
  roi_add_region_button = nil;
  roi_elliptical_button = nil;
  roi_delete_region_button = nil;
  roi_path_width_field = nil;
  roi_set_path_button = nil;
  roi_fill_path_button = nil;
  roi_undo_button = roi_redo_button = nil;
  roi_split_vertices_button = nil;
  roi_scribble_button = nil;
  roi_scribble_convert_button = nil;
  roi_scribble_replaces_button = nil;
  roi_scribble_accuracy_slider = nil;
  
  codestream_popup = compositing_layer_popup = nil;
  codestream_field = compositing_layer_field = nil;
  codestream_add_button = compositing_layer_add_button = nil;
  codestream_remove_button = compositing_layer_remove_button = nil;
  codestream_stepper = compositing_layer_stepper = nil;
  rendered_result_button = nil;
  
  is_link_button = nil;
  link_type_label = nil;
  box_type_popup = nil;
  box_editor_popup = nil;
  label_field = nil;
  external_file_field = nil;
  temporary_file_button = nil;
  save_to_file_button = nil;
  edit_file_button = nil;
  choose_file_button = nil;
  internalize_label_button = nil;
  
  copy_link_button = nil;
  paste_button = nil;
  copy_descendants_button = nil;
  
  apply_button = apply_and_exit_button = delete_button = exit_button = nil;
  next_button = prev_button = parent_button = nil;
  descendants_button = add_parent_button = add_child_button = nil;
  metashow_button = catalog_button = nil;
  if (![NSBundle loadNibNamed:@"metadata_editor" owner:self])
    { kdu_warning w;
      w << "Could not load NIB file, \"metadata_editor.nib\".";
      return;
    }
  
  char title[40];
  sprintf(title,"Metadata editor %d",identifier);
  [dialog_window setTitle:[NSString stringWithUTF8String:title]];
  
  NSRect screen_rect = [[NSScreen mainScreen] frame];
  NSRect frame_rect = [dialog_window frame];
  frame_rect.origin.x = preferred_location.x;
  frame_rect.origin.y = preferred_location.y - frame_rect.size.height;
  float min_x = screen_rect.origin.x;
  float lim_x = min_x + screen_rect.size.width;
  float min_y = screen_rect.origin.y;
  float lim_y = min_y + screen_rect.size.height;
  if ((frame_rect.origin.x + frame_rect.size.width) > lim_x)
    frame_rect.origin.x = lim_x - frame_rect.size.width;
  else if (frame_rect.origin.x < min_x)
    frame_rect.origin.x = min_x;
  if ((frame_rect.origin.y + frame_rect.size.height) > lim_y)
    frame_rect.origin.y = lim_y - frame_rect.size.height;
  else if (frame_rect.origin.y < min_y)
    frame_rect.origin.y = min_y;
  [dialog_window setFrame:frame_rect display:YES];
  hide_shape_paths = true;
  shape_scribbling_active = false;
  fixed_shape_path_thickness = 0;
  [roi_mode_popup selectItemAtIndex:0];
  [roi_elliptical_button setState:NSOffState];
}

/*****************************************************************************/
/*                        kdms_metadata_editor:close                         */
/*****************************************************************************/

- (void) close
{
  [self stop_shape_editor];
  if ((renderer != NULL) && (renderer->editor == self))
    renderer->editor = nil;
  [self release];
}

/*****************************************************************************/
/*                       kdms_metadata_editor:dealloc                        */
/*****************************************************************************/

- (void) dealloc
{
  assert(!editing_shape);
  if (dialog_window != nil)
    {
      [dialog_window close];
      [dialog_window release]; // Releases everything
    }
  if (edit_state != NULL)
    delete edit_state;
  if (state != NULL)
    delete state;
  if (initial_state != NULL)
    delete initial_state;
  if (owned_metalist != NULL)
    delete owned_metalist;
  [super dealloc];
}

/*****************************************************************************/
/*                     kdms_metadata_editor:preconfigure                     */
/*****************************************************************************/

- (void) preconfigure
{
  assert(!editing_shape);
  source->count_codestreams(num_codestreams);
  if (num_codestreams < 1)
    num_codestreams = 1;
  source->count_compositing_layers(num_layers);
  if (num_layers < 1)
    num_layers = 1;

  if (edit_state != NULL)
    delete edit_state;
  edit_state = new kdms_metanode_edit_state(source->access_meta_manager(),
                                            file_server);
  if (state != NULL)
    delete state;
  state = new kdms_metadata_dialog_state(num_codestreams,num_layers,
                                         available_types,file_server,
                                         allow_edits);  
  if (initial_state != NULL)
    delete initial_state;
  initial_state = new kdms_metadata_dialog_state(num_codestreams,num_layers,
                                                 available_types,file_server,
                                                 allow_edits);
  
  if (owned_metalist != NULL)
    delete owned_metalist;
  owned_metalist = NULL;
  
  [codestream_stepper setMinValue:0];
  [codestream_stepper setMaxValue:(num_codestreams-1)];
  [compositing_layer_stepper setMinValue:0];
  [compositing_layer_stepper setMaxValue:(num_layers-1)];
  
  [roi_mode_popup removeAllItems];
  [roi_mode_popup insertItemWithTitle:@"Vertex mode" atIndex:0];
  [roi_mode_popup insertItemWithTitle:@"Skeleton mode" atIndex:1];
  [roi_mode_popup insertItemWithTitle:@"Path mode" atIndex:2];
  
  [self map_state_to_controls];
}

/*****************************************************************************/
/*                 kdms_metadata_editor:configureWithEditList                */
/*****************************************************************************/

- (void) configureWithEditList:(kdms_matching_metalist *)metalist
{
  if (editing_shape)
    { NSBeep(); return; }
  if (!dialog_window)
    return;
  [dialog_window makeKeyAndOrderFront:self];
  if ((state != NULL) && (initial_state != NULL) &&
      !(state->compare(initial_state) ||
        interrogate_user("You have made some changes; are you sure you "
                         "want to change the editing context without "
                         "applying these changes?","Cancel","Proceed")))
    {
      [self update_apply_buttons];
      return;
    }

  [self preconfigure];
  owned_metalist = metalist;
  
  edit_state->metalist = metalist;
  edit_state->edit_item = metalist->get_head();
  if (edit_state->edit_item == NULL)
    {
      assert(0);
      return;
    }
  edit_state->cur_node = edit_state->root_node = edit_state->edit_item->node;
  edit_state->cur_node_sibling_idx = 0;
  edit_state->next_sibling_idx = edit_state->prev_sibling_idx = -1;
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];
}

/*****************************************************************************/
/*                    kdms_metadata_editor:setActiveNode                     */
/*****************************************************************************/

- (void) setActiveNode:(jpx_metanode)node
{
  if (editing_shape)
    { NSBeep(); return; }
  if (owned_metalist == NULL)
    return;
  edit_state->move_to_node(node);
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];
}

/*****************************************************************************/
/*        kdms_metadata_editor:configureWithInitialRegion                    */
/*                            :stream:layer:appliesToRenderedResult          */
/*****************************************************************************/

- (void) configureWithInitialRegion:(kdu_dims)roi
                         stream:(int)codestream_idx
                              layer:(int)layer_idx
            appliesToRenderedResult:(bool)associate_with_rendered_result
{
  if (editing_shape)
    { NSBeep(); return; }
  if (!dialog_window)
    return;
  [dialog_window makeKeyAndOrderFront:self];
  if ((state != NULL) && (initial_state != NULL) &&
      !(state->compare(initial_state) ||
        interrogate_user("You have made some changes; are you sure you "
                         "want to change the editing context without "
                         "applying these changes?","Cancel","Proceed")))
    {
      [self update_apply_buttons];
      return;
    }
  
  [self preconfigure];
  
  state->num_selected_codestreams = state->num_selected_layers = 0;
  if (codestream_idx >= 0)
    state->add_selected_codestream(codestream_idx);
  if (layer_idx >= 0)
    state->add_selected_layer(layer_idx);
  state->rendered_result = associate_with_rendered_result;
  state->whole_image = roi.is_empty();
  state->roi_editor.reset();
  if (state->whole_image)
    {
      state->num_offered_types = 3;
      state->offered_types[0] = available_types + KDMS_LABEL_TEMPLATE;
      state->offered_types[1] = available_types + KDMS_XML_TEMPLATE;
      state->offered_types[2] = available_types + KDMS_IPR_TEMPLATE;
      state->selected_type = state->offered_types[0];
      state->label_string = "<new label>";
    }
  else
    {
      jpx_roi region; region.init_rectangle(roi);
      state->roi_editor.init(&region,1);
      state->num_offered_types = 1;
      state->offered_types[0] = available_types + KDMS_CUSTOM_TEMPLATE;
      available_types[KDMS_CUSTOM_TEMPLATE].init(jp2_roi_description_4cc,
                                                 NULL,NULL);
      state->selected_type = state->offered_types[0];
      state->label_string = NULL;
    }
  state->set_external_file(NULL);
  initial_state->copy(state);
  [self map_state_to_controls];
}

/*****************************************************************************/
/*                kdms_metadata_editor:map_state_to_controls                 */
/*****************************************************************************/

- (void) map_state_to_controls
{
  // Set colours to highlight link status
  NSColor *link_status =
    (state->is_link)?[NSColor blueColor]:[NSColor textColor];
  [x_pos_field setTextColor:link_status];
  [y_pos_field setTextColor:link_status];
  [width_field setTextColor:link_status];
  [height_field setTextColor:link_status];
  [label_field setTextColor:link_status];
  
  // Set data values
  int c;
  [codestream_popup removeAllItems];
  if (state->num_selected_codestreams > 0)
    [codestream_popup addItemsWithTitles:
     make_array_of_items(state->num_selected_codestreams,
                         state->selected_codestreams)];
  [[codestream_field cell] setIntValue:0];
  [[codestream_stepper cell] setIntValue:0];
  
  [compositing_layer_popup removeAllItems];
  if (state->num_selected_layers > 0)
    [compositing_layer_popup addItemsWithTitles:
     make_array_of_items(state->num_selected_layers,state->selected_layers)];
  [[compositing_layer_field cell] setIntValue:0];
  [[compositing_layer_stepper cell] setIntValue:0];
  
  [rendered_result_button
   setState:(state->rendered_result)?NSOnState:NSOffState];
  
  [whole_image_button setState:(state->whole_image)?NSOnState:NSOffState];
  kdu_dims bb;
  if (state->roi_editor.get_bounding_box(bb))
    {
      [[x_pos_field cell] setStringValue:[NSString stringWithFormat:@"%d",
                                          bb.pos.x]];
      [[y_pos_field cell] setStringValue:[NSString stringWithFormat:@"%d",
                                          bb.pos.y]];
      [[width_field cell] setStringValue:[NSString stringWithFormat:@"%d",
                                          bb.size.x]];
      [[height_field cell] setStringValue:[NSString stringWithFormat:@"%d",
                                           bb.size.y]];
    }
  else
    {
      [[x_pos_field cell] setStringValue:@""];
      [[y_pos_field cell] setStringValue:@""];
      [[width_field cell] setStringValue:@""];
      [[height_field cell] setStringValue:@""];      
    }
  if (state->roi_editor.is_simple())
    {
      int nregns;
      const jpx_roi *roi = state->roi_editor.get_regions(nregns);
      [single_rectangle_button setState:
       (roi->is_elliptical)?NSOffState:NSOnState];
      [single_ellipse_button setState:
       (roi->is_elliptical)?NSOnState:NSOffState];
    }
  else
    { 
      [single_rectangle_button setState:NSOffState];
      [single_ellipse_button setState:NSOffState];
    }
  [roi_encoded_button setState:
   (state->roi_editor.contains_encoded_regions())?NSOnState:NSOffState];
  
  if (editing_shape)
    [roi_shape_editor_button setTitle:@"Cancel"];
  else if (!edit_state->cur_node.exists())
    [roi_shape_editor_button setTitle:@"Create & edit"];
  else if (!state->compare(initial_state))
    [roi_shape_editor_button setTitle:@"Apply & edit"];
  else
    [roi_shape_editor_button setTitle:@"Edit"];
  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
  if (state->is_link)
    {
      [is_link_button setState:NSOnState];
      if (edit_state->cur_node.exists())
        edit_state->cur_node.get_link(link_type);
    }
  else
    [is_link_button setState:NSOffState];
  if (link_type == JPX_GROUPING_LINK)
    [[link_type_label cell] setTitle:@"Link node (grouping)"];
  else if (link_type == JPX_ALTERNATE_CHILD_LINK)
    [[link_type_label cell] setTitle:@"Link node (alt child)"];
  else if (link_type == JPX_ALTERNATE_PARENT_LINK)
    [[link_type_label cell] setTitle:@"Link node (alt parent)"];
  else
    [[link_type_label cell] setTitle:@"Not a link"];

  [box_type_popup removeAllItems];
  int t;
  for (t=0; t < state->num_offered_types; t++)
    [box_type_popup addItemWithTitle:
     [NSString stringWithUTF8String:state->offered_types[t]->box_type_string]];
  for (t=0; t < state->num_offered_types; t++)
    if (state->offered_types[t] == state->selected_type)
        [box_type_popup selectItemAtIndex:t];
  
  [box_editor_popup removeAllItems];
  [box_editor_popup addItemWithTitle:@"Choose in Finder"];
  kdms_file_editor *editor;
  for (t=0; (editor=file_server->get_editor_for_doc_type(
                           state->selected_type->file_suffix,t)) != NULL; t++)
   [box_editor_popup addItemWithTitle:
    [NSString stringWithUTF8String:editor->app_name]];
  if (t > 0)
    [box_editor_popup selectItemAtIndex:1];
  
  kdms_file *external_file = state->get_external_file();
  if (external_file != NULL)
    [external_file_field setStringValue:
     [NSString stringWithUTF8String:external_file->get_pathname()]];
  else
    [external_file_field setStringValue:@""];
  [temporary_file_button setState:
   ((external_file!=NULL) &&
    external_file->get_temporary())?NSOnState:NSOffState];
  
  if (state->label_string != NULL)
    [[label_field cell] setStringValue:
     [NSString stringWithUTF8String:state->label_string]];
  else
    [[label_field cell] setStringValue:@""];

  bool is_roid = (state->selected_type->box_type == jp2_roi_description_4cc);
  bool have_encoded_regions = state->roi_editor.contains_encoded_regions();
  
  // Determine which controls should be enabled
  bool can_edit = state->can_edit_node && !editing_shape;
  
  [codestream_popup setEnabled:((can_edit && !have_encoded_regions)?YES:NO)];
  [codestream_field setEnabled:((can_edit && !have_encoded_regions)?YES:NO)];
  [codestream_add_button setEnabled:
   ((can_edit && !have_encoded_regions)?YES:NO)];
  [codestream_remove_button setEnabled:
   ((can_edit && (!have_encoded_regions) &&
     (state->num_selected_codestreams > 0))?YES:NO)];
  [codestream_stepper setEnabled:((can_edit && !have_encoded_regions)?YES:NO)];

  [compositing_layer_popup setEnabled:((can_edit)?YES:NO)];
  [compositing_layer_field setEnabled:((can_edit)?YES:NO)];
  [compositing_layer_add_button setEnabled:((can_edit)?YES:NO)];
  [compositing_layer_remove_button setEnabled:
   ((can_edit && (state->num_selected_layers > 0))?YES:NO)];
  [compositing_layer_stepper setEnabled:((can_edit)?YES:NO)];

  [rendered_result_button setEnabled:((can_edit)?YES:NO)];
  
  bool roi_enable = can_edit &&
   (edit_state->cur_node == edit_state->root_node) &&
   !(state->whole_image || have_encoded_regions);
  [roi_shape_editor_button setEnabled:((roi_enable || editing_shape)?YES:NO)];
  [x_pos_field setEnabled:NO];
  [y_pos_field setEnabled:NO];
  [width_field setEnabled:NO];
  [height_field setEnabled:NO];
  [single_rectangle_button setEnabled:((roi_enable && !editing_shape)?YES:NO)];
  [single_ellipse_button setEnabled:((roi_enable && !editing_shape)?YES:NO)];
  [whole_image_button setEnabled:
   ((can_edit && (!have_encoded_regions) &&
     (!editing_shape) && is_roid)?YES:NO)];
  [roi_encoded_button setEnabled:
   ((can_edit && (!editing_shape) && have_encoded_regions)?YES:NO)];
  [roi_mode_popup setEnabled:((roi_enable || editing_shape)?YES:NO)];
  
  jp2_family_src *existing_src;
  bool can_save_to_file =
    ((state->get_external_file() != NULL) ||
     ((state->selected_type->box_type == jp2_label_4cc) &&
      (state->label_string != NULL) &&
      !(state->is_link || state->is_cross_reference)) ||
     (edit_state->cur_node.exists() &&
      !edit_state->cur_node.get_existing(existing_src).is_null()));
  
  [is_link_button setEnabled:NO];
  [box_type_popup setEnabled:
   ((can_edit && (state->num_offered_types > 1))?YES:NO)];
  [box_editor_popup setEnabled:((can_edit && !is_roid)?YES:NO)];
  [save_to_file_button setEnabled:can_save_to_file?YES:NO];
  [external_file_field setEnabled:NO];
  [choose_file_button setEnabled:((can_edit && !is_roid)?YES:NO)];
  [edit_file_button setEnabled:((can_edit && !is_roid)?YES:NO)];

  [internalize_label_button setEnabled:
   ((external_file != NULL) && can_edit &&
     (state->selected_type->box_type==jp2_label_4cc))?YES:NO];
  
  [copy_link_button setEnabled:
   (edit_state->cur_node.exists() && allow_edits && (!editing_shape) &&
    (state->is_link || !state->is_cross_reference))?YES:NO];
  [paste_button setEnabled:
   (allow_edits && (!editing_shape) &&
    renderer->catalog_source && !is_roid)?YES:NO];
  [copy_descendants_button setEnabled:
   (edit_state->cur_node.exists() && allow_edits && (!editing_shape) &&
    (renderer->catalog_source != nil))?YES:NO];  
  
  [label_field setEnabled:
   ((state->can_edit_node && (state->selected_type->box_type==jp2_label_4cc) &&
    (external_file == NULL))?YES:NO)];

  NSColor *active_text_colour = [NSColor blackColor];
  NSColor *inactive_text_colour = [NSColor grayColor];
  [apply_button setEnabled:(allow_edits)?YES:NO];
  [delete_button setEnabled:(allow_edits)?YES:NO];
  if (state->is_link)
    [delete_button setTitle:@"Delete Link"];
  else
    [delete_button setTitle:@"Delete"];
  [exit_button setEnabled:YES];
  
  if (edit_state->cur_node == edit_state->root_node)
    {
      [next_button setEnabled:
       ((edit_state->edit_item != NULL) &&
        (edit_state->edit_item->next != NULL) &&
        !editing_shape)?YES:NO];
      [next_button_label setTextColor:
       ((edit_state->edit_item != NULL) &&
        (edit_state->edit_item->next != NULL))?
                   active_text_colour:inactive_text_colour];
      [prev_button setEnabled:
       ((edit_state->edit_item != NULL) &&
        (edit_state->edit_item->prev != NULL) &&
        !editing_shape)?YES:NO];
      [prev_button_label setTextColor:
       ((edit_state->edit_item != NULL) &&
        (edit_state->edit_item->prev != NULL))?
                   active_text_colour:inactive_text_colour];
      [parent_button setEnabled:NO];
      [parent_button_label setTextColor:inactive_text_colour];
    }
  else
    {
      [next_button setEnabled:
       ((edit_state->next_sibling_idx>=0) && !editing_shape)?YES:NO];
      [next_button_label setTextColor:
       (edit_state->next_sibling_idx>=0)?
                   active_text_colour:inactive_text_colour];
      [prev_button setEnabled:
       ((edit_state->prev_sibling_idx>=0) && !editing_shape)?YES:NO];
      [prev_button_label setTextColor:
       (edit_state->prev_sibling_idx>=0)?
                   active_text_colour:inactive_text_colour];
      [parent_button setEnabled:((!editing_shape)?YES:NO)];
      [parent_button_label setTextColor:active_text_colour];
    }
  bool have_non_numlist_children = false;
  if (edit_state->cur_node.exists())
    {
      bool rres; // dummy variable used in check for numlist nodes
      int num_cs, num_l; // dummy variables used in check for numlist nodes
      jpx_metanode child;
      for (c=0; (child=edit_state->cur_node.get_descendant(c)).exists(); c++)
        if (!child.get_numlist_info(num_cs,num_l,rres))
          {
            have_non_numlist_children = true;
            break;
          }
    }
  [descendants_button setEnabled:
   (have_non_numlist_children && !editing_shape)?YES:NO];
  [descendants_button_label setTextColor:
   (have_non_numlist_children)?active_text_colour:inactive_text_colour];
  [add_child_button setEnabled:(allow_edits && !editing_shape)?YES:NO];
  [add_parent_button setEnabled:(allow_edits &&
                                 !(is_roid || editing_shape))?YES:NO];
  [metashow_button setEnabled:(edit_state->cur_node.exists() &&
                               !state->is_link)?YES:NO];
  [catalog_button setEnabled:(edit_state->cur_node.exists() &&
                              (initial_state->label_string != NULL))?YES:NO];
  [self update_apply_buttons];
  [self update_shape_editor_controls];
}

/*****************************************************************************/
/*                kdms_metadata_editor:map_controls_to_state                 */
/*****************************************************************************/

- (void) map_controls_to_state
{
  /* The following members have already been set in response to IBAction
   messages: `selected_codestreams', `selected_layers', `whole_image',
   `external_file' and `selected_type'.
   Here we find the remaining member values by querying the current state
   of the dialog controls. */
  state->rendered_result = ([rendered_result_button state]==NSOnState);
  if ((state->selected_type->box_type == jp2_label_4cc) &&
      (state->get_external_file() == NULL))
    state->label_string = [[[label_field cell] stringValue] UTF8String];
  else
    state->label_string = NULL;
}

/*****************************************************************************/
/*                kdms_metadata_editor:update_apply_buttons                  */
/*****************************************************************************/

- (void) update_apply_buttons
{
  bool something_to_apply = editing_shape ||
     !(edit_state->cur_node.exists() && state->compare(initial_state));
  [apply_button setEnabled:(something_to_apply)?YES:NO];
  [apply_and_exit_button setEnabled:(something_to_apply)?YES:NO];
  if (!edit_state->cur_node.exists())
    {
      [apply_button setTitle:@"Create w/ label"];
      [apply_and_exit_button setTitle:@"Create & Exit"];
    }
  else
    {
      [apply_button setTitle:@"Apply"];
      [apply_and_exit_button setTitle:@"Apply & Exit"];
    }
}

/*****************************************************************************/
/*           kdms_metadata_editor:update_shape_editor_controls               */
/*****************************************************************************/

- (void) update_shape_editor_controls
{
  kdu_coords point;
  int num_point_instances = 0;
  bool have_selection =
    (edit_state->active_shape_editor.get_selection(point,
                                                   num_point_instances) >= 0);
  int num_regions=0;
  edit_state->active_shape_editor.get_regions(num_regions);
  int max_undo=0; bool can_redo=false;
  edit_state->active_shape_editor.get_history_info(max_undo,can_redo);
  
  [roi_complexity_level setEnabled:(editing_shape)?YES:NO];
  [roi_complexity_value setEnabled:(editing_shape)?YES:NO];
  if (editing_shape)
    {
      double complexity = 100.0 *
        edit_state->active_shape_editor.measure_complexity();
      [[roi_complexity_level cell] setDoubleValue:complexity];
      [[roi_complexity_value cell] setStringValue:
       [NSString stringWithFormat:@"%4.1f%%",complexity]];
    }
  else
    { 
      [[roi_complexity_level cell] setDoubleValue:0.0];
      [[roi_complexity_value cell] setStringValue:@""];
    }
  [roi_add_region_button setEnabled:(editing_shape && have_selection)?YES:NO];
  [roi_elliptical_button setEnabled:(editing_shape)?YES:NO];
  if (shape_editing_mode == JPX_EDITOR_PATH_MODE)
    [roi_elliptical_button setTitle:@"+junctions"];
  else
    [roi_elliptical_button setTitle:@"Ellipses"];
  [roi_delete_region_button setEnabled:
   (editing_shape && have_selection && (num_regions > 1))?YES:NO];
  [roi_split_vertices_button setEnabled:
   (editing_shape && have_selection && (num_point_instances > 1))?YES:NO]; 
  bool path_mode = (shape_editing_mode == JPX_EDITOR_PATH_MODE);
  [roi_path_width_field setEnabled:
   (editing_shape && path_mode && (fixed_shape_path_thickness==0))?YES:NO];
  [roi_set_path_button setEnabled:(editing_shape && path_mode)?YES:NO];
  bool have_closed_path_to_fill = false;
  if (editing_shape && path_mode)
    {
      kdu_uint32 path_flags[8]={0,0,0,0,0,0,0,0}; kdu_byte members[256];
      kdu_coords start_pt, end_pt;
      while (edit_state->active_shape_editor.enum_paths(path_flags,members,
                                                        start_pt,end_pt) > 0)
        if (start_pt == end_pt)
          { have_closed_path_to_fill = true; break; }
    }
  [roi_fill_path_button setEnabled:(have_closed_path_to_fill)?YES:NO];
  if (fixed_shape_path_thickness == 0)
    [roi_set_path_button setTitle:@"Fix path width"];
  else
    [roi_set_path_button setTitle:@"Release path width"];
  [roi_undo_button setEnabled:(editing_shape && (max_undo > 0))?YES:NO];
  [roi_redo_button setEnabled:(editing_shape && can_redo)?YES:NO];

  int num_scribble_points = 0;
  if (shape_scribbling_active)
    edit_state->active_shape_editor.get_scribble_points(num_scribble_points);
  [roi_scribble_button setEnabled:(editing_shape)?YES:NO];
  [roi_scribble_replaces_button setEnabled:(editing_shape)?YES:NO];
  [roi_scribble_convert_button setEnabled:
   (editing_shape && (num_scribble_points > 0))?YES:NO];
  [roi_scribble_accuracy_slider setEnabled:
   (editing_shape && (num_scribble_points > 0))?YES:NO];
  [roi_scribble_button setTitle:
   (shape_scribbling_active)?@"Stop scribble":@"Start scribble"];
  [roi_scribble_convert_button setTitle:
   (path_mode)?@"Scribble \xe2\x86\x92 path":@"Scribble \xe2\x86\x92 fill"];
}

/*****************************************************************************/
/*                  kdms_metadata_editor:stop_shape_editor                   */
/*****************************************************************************/

- (void) stop_shape_editor
{
  shape_scribbling_active = false;
  if (!editing_shape)
    return;
  if (renderer != NULL)
    {
      renderer->set_shape_editor(NULL);
      renderer->set_focus_box(kdu_dims());
    }
  editing_shape = false;
}
  
/*****************************************************************************/
/*                kdms_metadata_editor:query_save_shape_edits                */
/*****************************************************************************/

- (bool) query_save_shape_edits
{
  if (!editing_shape)
    true;
  if (edit_state->active_shape_editor.measure_complexity() > 1.0)
    {
      if (!interrogate_user("The edited shape configuration exceeds the "
                            "number of region elements which can be saved "
                            "in a JPX ROI Description box.  The region "
                            "will not be rendered completely.  You should "
                            "delete some quadrilateral/elliptical regions "
                            "or simplify them to horizontally/vertically "
                            "aligned rectangles or ellipses (these require "
                            "fewer elements to represent).",
                            "Keep editing","Discard edits"))
        return false;
    }
  else
    state->roi_editor.copy_from(edit_state->active_shape_editor);
  [self stop_shape_editor];
  return true;
}

/*****************************************************************************/
/*                kdms_metadata_editor:control:isValidObject                 */
/*****************************************************************************/

- (BOOL) control:(NSControl *)control isValidObject:(id)object
{
  if (control == label_field)
    return YES;
  const char *string = [[[control cell] stringValue] UTF8String];
  for (; (string != NULL) && (*string != '\0'); string++)
    if (!isdigit(*string))
      {
        NSBeep();
        return NO;
      }
  return YES;
}

/*****************************************************************************/
/*         kdms_metadata_editor:[ACTIONS for Association Editing]            */
/*****************************************************************************/

- (IBAction) removeCodestream:(id)sender
{
  if (state->roi_editor.contains_encoded_regions() || editing_shape ||
      !state->can_edit_node)
    return;
  int n, idx = [codestream_popup indexOfSelectedItem];
  if ((idx < 0) || (idx >= state->num_selected_codestreams))
    return; // Could happen if the list is already empty
  [self map_controls_to_state];
  for (n=idx+1; n < state->num_selected_codestreams; n++)
    state->selected_codestreams[n-1] = state->selected_codestreams[n];
  state->num_selected_codestreams--;
  [self map_state_to_controls];
}

- (IBAction) addCodestream:(id)sender
{
  if (state->roi_editor.contains_encoded_regions() || editing_shape ||
      !state->can_edit_node)
    return;
  if (!check_integer_in_range(codestream_field,0,num_codestreams-1))
    return;
  [self map_controls_to_state];
  int idx = [[codestream_field cell] intValue];
  [[codestream_stepper cell] setIntValue:idx];
  [[codestream_field cell] setStringValue:@""];
  if ((idx = state->add_selected_codestream(idx)) < 0)
    return;
  [self map_state_to_controls];
}

- (IBAction) stepCodestream:(id)sender
{
  if (state->roi_editor.contains_encoded_regions() || editing_shape ||
      !state->can_edit_node)
    return;
  int idx = [[codestream_stepper cell] intValue];
  if ((idx >= 0) && (idx < num_codestreams))
    [[codestream_field cell] setIntValue:idx];
}

- (IBAction) showCodestream:(id)sender
{
  if (!check_integer_in_range(codestream_field,0,num_codestreams-1))
    return;
  int idx = [[codestream_field cell] intValue];
  [[codestream_stepper cell] setIntValue:idx];
  renderer->set_codestream(idx);  
}

- (IBAction) removeCompositingLayer:(id)sender
{
  if (editing_shape || !state->can_edit_node)
    return;
  int n, idx = [compositing_layer_popup indexOfSelectedItem];
  if ((idx < 0) || (idx >= state->num_selected_layers))
    return; // Could happen if the list is already empty
  [self map_controls_to_state];
  for (n=idx+1; n < state->num_selected_layers; n++)
    state->selected_layers[n-1] = state->selected_layers[n];
  state->num_selected_layers--;
  [self map_state_to_controls];
}

- (IBAction) addCompositingLayer:(id)sender
{
  if (editing_shape || !state->can_edit_node)
    return;
  if (!check_integer_in_range(compositing_layer_field,0,num_layers-1))
    return;
  [self map_controls_to_state];
  int idx = [[compositing_layer_field cell] intValue];
  [[compositing_layer_stepper cell] setIntValue:idx];  
  [[compositing_layer_field cell] setStringValue:@""];
  if ((idx = state->add_selected_layer(idx)) < 0)
    return;
  [self map_state_to_controls];
}

- (IBAction) stepCompositingLayer:(id)sender
{
  if (editing_shape || !state->can_edit_node)
    return;
  int idx = [[compositing_layer_stepper cell] intValue];
  if ((idx >= 0) && (idx < num_layers))
    [[compositing_layer_field cell] setIntValue:idx];
}

- (IBAction) showCompositingLayer:(id)sender
{
  if (!check_integer_in_range(compositing_layer_field,0,num_layers-1))
    return;
  int idx = [[compositing_layer_field cell] intValue];
  [[compositing_layer_stepper cell] setIntValue:idx];
  renderer->set_compositing_layer(idx);
}

- (IBAction) setWholeImage:(id)sender
{
  if (state->roi_editor.contains_encoded_regions() ||
      (!state->can_edit_node) || editing_shape ||
      state->roi_editor.is_empty() ||
      (state->selected_type->box_type != jp2_roi_description_4cc))
    return;
  state->whole_image = ([whole_image_button state] == NSOnState);
  [self map_controls_to_state];
  [self map_state_to_controls];
}

- (IBAction) clearROIEncoded:(id)sender
{
  if (editing_shape ||
      !(state->can_edit_node && state->roi_editor.contains_encoded_regions()))
    return;
  if (!interrogate_user("The node you are editing is associated with regions "
                        "of interest that are explicitly encoded with higher "
                        "priority in the codestream.  Removing the encoded "
                        "status will allow you to edit the region shape and "
                        "codestream associations but lose the connection "
                        "with codestream encoding properties.  Are you sure "
                        "you want to proceed?","Cancel","Proceed"))
    return;
  int n, num_regions=0;
  const jpx_roi *regions = state->roi_editor.get_regions(num_regions);
  for (n=0; n < num_regions; n++)
    if (regions[n].is_encoded)
      {
        jpx_roi mod_region = regions[n];
        mod_region.is_encoded = false;
        state->roi_editor.modify_region(n,mod_region);
      }
  [self map_controls_to_state];
  [self map_state_to_controls];
}

- (IBAction) setSingleRectangle:(id)sender
{
  [self map_controls_to_state];
  if (editing_shape || state->roi_editor.contains_encoded_regions() ||
      !state->can_edit_node)
    return;
  int nregns=0;
  const jpx_roi *roi = state->roi_editor.get_regions(nregns);
  if ((roi != NULL) && ([single_rectangle_button state] == NSOnState))
    { 
      jpx_roi regn = *roi;
      regn.is_elliptical = false; regn.flags &= ~JPX_QUADRILATERAL_ROI;
      state->roi_editor.init(&regn,1);
    }
  [self map_state_to_controls];
}

- (IBAction) setSingleEllipse:(id)sender
{
  [self map_controls_to_state];
  if (editing_shape || state->roi_editor.contains_encoded_regions() ||
      !state->can_edit_node)
    return;
  int nregns=0;
  const jpx_roi *roi = state->roi_editor.get_regions(nregns);
  if ((roi != NULL) && ([single_ellipse_button state] == NSOnState))
    { 
      jpx_roi regn = *roi;
      regn.is_elliptical = true; regn.flags &= ~JPX_QUADRILATERAL_ROI;
      state->roi_editor.init(&regn,1);
    }
  [self map_state_to_controls];
}

/*****************************************************************************/
/*          kdms_metadata_editor:[ACTIONS for ROI shape editing]             */
/*****************************************************************************/

- (IBAction) editROIShape:(id)sender
{
  if (!state->can_edit_node)
    return;
  [self map_controls_to_state];
  kdu_istream_ref istream_ref;
  if (editing_shape)
    [self stop_shape_editor];
  else
    { 
      renderer->stop_playmode();
      if (!edit_state->cur_node.exists())
        { // Must create the box first
          if (!state->save_metanode(edit_state,initial_state,renderer))
            {
              [self update_apply_buttons];
              return;
            }
          edit_state->add_child_node(renderer);
          edit_state->move_to_parent();
          state->configure_with_metanode(edit_state->cur_node);
          initial_state->copy(state);
        }
      else if (!state->compare(initial_state))
        { // Must apply changes first
          if (!state->save_metanode(edit_state,initial_state,renderer))
            {
              [self update_apply_buttons];
              return;
            }
          state->configure_with_metanode(edit_state->cur_node);
          initial_state->copy(state);
        }      
      edit_state->active_shape_editor.copy_from(state->roi_editor);
      edit_state->active_shape_editor.set_max_undo_history(4096);
      NSInteger idx = [roi_mode_popup indexOfSelectedItem];
      if (idx == 0)
        shape_editing_mode = JPX_EDITOR_VERTEX_MODE;
      else if (idx == 1)
        shape_editing_mode = JPX_EDITOR_SKELETON_MODE;
      else if (idx == 2)
        shape_editing_mode = JPX_EDITOR_PATH_MODE;
      hide_shape_paths = (shape_editing_mode != JPX_EDITOR_PATH_MODE);
      shape_scribbling_active = false;
      fixed_shape_path_thickness = 0;
      edit_state->active_shape_editor.set_mode(shape_editing_mode);
      if (renderer->show_imagery_for_metanode(edit_state->cur_node,
                                              &istream_ref) &&
          istream_ref.exists() &&
          renderer->set_shape_editor(&(edit_state->active_shape_editor),
                                     istream_ref,hide_shape_paths,
                                     shape_scribbling_active))
        editing_shape = true;
      else if (!istream_ref)
        interrogate_user("Cannot start find (and display) the imagery for "
                         "this ROI description node, which is an essential "
                         "preliminary for launching the shape editor.  "
                         "Perhaps the codestream associations are invalid??",
                         "OK");
      else
        interrogate_user("Cannot start the shape editor for some reason.  "
                         "Most likely this ROI description node is too "
                         "complex for the current implementation of the "
                         "shape editor.","OK");
    }
  [self map_state_to_controls];
}

- (IBAction) changeROIMode:(id)sender
{
  if (!editing_shape)
    return;
  NSInteger idx = [roi_mode_popup indexOfSelectedItem];
  jpx_roi_editor_mode mode = shape_editing_mode;
  if (idx == 0)
    mode = JPX_EDITOR_VERTEX_MODE;
  else if (idx == 1)
    mode = JPX_EDITOR_SKELETON_MODE;
  else if (idx == 2)
    mode = JPX_EDITOR_PATH_MODE;
  if (mode == shape_editing_mode)
    return;
  
  hide_shape_paths = (mode != JPX_EDITOR_PATH_MODE);
  fixed_shape_path_thickness = 0;
  shape_editing_mode = mode;
  kdu_dims update_dims =
    edit_state->active_shape_editor.set_mode(shape_editing_mode);
  if (!update_dims.is_empty())
    renderer->update_shape_editor(update_dims,hide_shape_paths,
                                  fixed_shape_path_thickness,
                                  shape_scribbling_active);
  [self update_shape_editor_controls];
}

- (IBAction) addROIRegion:(id)sender
{
  if (!editing_shape)
    return;
  bool ellipses = ([roi_elliptical_button state] == NSOnState);
  kdu_dims visible_frame = renderer->get_visible_shape_region();
  kdu_dims update_dims =
    edit_state->active_shape_editor.add_region(ellipses,visible_frame);
  if (!update_dims.is_empty())
    renderer->update_shape_editor(update_dims,hide_shape_paths,
                                  fixed_shape_path_thickness,
                                  shape_scribbling_active);
  [self update_shape_editor_controls];
}

- (IBAction) deleteROIRegion:(id)sender
{
  if (!editing_shape)
    return;
  kdu_dims update_dims =
    edit_state->active_shape_editor.delete_selected_region();
  if (!update_dims.is_empty())
    { 
      update_dims.augment(renderer->shape_editor_adjust_path_thickness());
      renderer->update_shape_editor(update_dims,hide_shape_paths,
                                    fixed_shape_path_thickness,
                                    shape_scribbling_active);
    }
  [self update_shape_editor_controls];
}

- (IBAction) splitROIAnchor:(id)sender
{
  if (!editing_shape)
    return;
  kdu_dims update_dims =
    edit_state->active_shape_editor.split_selected_anchor();
  if (!update_dims.is_empty())
    { 
      update_dims.augment(renderer->shape_editor_adjust_path_thickness());
      renderer->update_shape_editor(update_dims,hide_shape_paths,
                                    fixed_shape_path_thickness,
                                    shape_scribbling_active);
    }
  [self update_shape_editor_controls];
}

- (IBAction) setROIPathWidth:(id)sender
{
  if (!editing_shape)
    return;
  if (fixed_shape_path_thickness > 0)
    { // Must be releasing the path thickness
      fixed_shape_path_thickness = 0;
      renderer->update_shape_editor(kdu_dims(),hide_shape_paths,0,
                                    shape_scribbling_active);
      [self update_shape_editor_controls];
      return;
    }
  if (!check_integer_in_range(roi_path_width_field,1,255))
    return;
  int thickness = [[roi_path_width_field cell] intValue];
  if (thickness < 1) thickness = 1;
  if (thickness > 255) thickness = 255;
  bool success = false;
  kdu_dims update_dims =
    edit_state->active_shape_editor.set_path_thickness(thickness,success);
  fixed_shape_path_thickness = (success)?thickness:0;
  if (!update_dims.is_empty())
    renderer->update_shape_editor(update_dims,hide_shape_paths,
                                  fixed_shape_path_thickness,
                                  shape_scribbling_active);
  if (!success)
    NSBeep();
  [self update_shape_editor_controls];
}

- (IBAction) fillROIPath:(id)sender
{
  if (!editing_shape)
    return;
  bool success = false;
  kdu_dims update_dims =
    edit_state->active_shape_editor.fill_closed_paths(success);
  if (!update_dims.is_empty())
    { 
      shape_editing_mode = JPX_EDITOR_VERTEX_MODE;
      hide_shape_paths = true;
      fixed_shape_path_thickness = 0;
      update_dims.augment(
          edit_state->active_shape_editor.set_mode(shape_editing_mode));
      renderer->update_shape_editor(update_dims,hide_shape_paths,
                                    fixed_shape_path_thickness,
                                    shape_scribbling_active);
      [roi_mode_popup selectItemAtIndex:0];
    }
  if (!success)
    NSBeep();
  [self update_shape_editor_controls];
}

- (IBAction) undoROIEdit:(id)sender
{
  if (!editing_shape)
    return;
  kdu_dims update_dims = edit_state->active_shape_editor.undo();
  if (shape_scribbling_active)
    { 
      int num_points=0;
      edit_state->active_shape_editor.get_scribble_points(num_points);
      if (num_points == 0)
        shape_scribbling_active = false;
    }
  if (!update_dims.is_empty())
    renderer->update_shape_editor(update_dims,hide_shape_paths,
                                  fixed_shape_path_thickness,
                                  shape_scribbling_active);
  [self update_shape_editor_controls];
}

- (IBAction) redoROIEdit:(id)sender
{
  if (!editing_shape)
    return;
  kdu_dims update_dims = edit_state->active_shape_editor.redo();
  if (shape_scribbling_active)
    { 
      int num_points=0;
      edit_state->active_shape_editor.get_scribble_points(num_points);
      if (num_points == 0)
        shape_scribbling_active = false;
    }
  if (!update_dims.is_empty())
    renderer->update_shape_editor(update_dims,hide_shape_paths,
                                  fixed_shape_path_thickness,
                                  shape_scribbling_active);
  [self update_shape_editor_controls];
}

- (IBAction) startScribble:(id)sender
{
  if (!editing_shape)
    return;
  kdu_dims update_dims;
  if (shape_scribbling_active)
    { 
      update_dims = edit_state->active_shape_editor.clear_scribble_points();
      shape_scribbling_active = false;
    }
  else
    shape_scribbling_active = true;
  renderer->update_shape_editor(update_dims,hide_shape_paths,
                                fixed_shape_path_thickness,
                                shape_scribbling_active);
  [self update_shape_editor_controls];
}

- (IBAction) convertScribble:(id)sender
{
  if (!(editing_shape && shape_scribbling_active))
    return;
  int num_scribble_points;
  edit_state->active_shape_editor.get_scribble_points(num_scribble_points);
  if (num_scribble_points < 1)
    return;
  bool replace = ([roi_scribble_replaces_button state] == NSOnState);
  int flags = JPX_EDITOR_FLAG_FILL | JPX_EDITOR_FLAG_ELLIPSES;
  if (shape_editing_mode == JPX_EDITOR_PATH_MODE)
    { 
      bool ellipses = ([roi_elliptical_button state] == NSOnState);
      flags = (ellipses)?JPX_EDITOR_FLAG_ELLIPSES:0;
    }
  double acc = [[roi_scribble_accuracy_slider cell] doubleValue];
  kdu_dims update_dims =
    edit_state->active_shape_editor.convert_scribble_path(replace,flags,acc);
  if (update_dims.is_empty())
    interrogate_user("Converted scribble path's complexity will be too high.  "
                     "Adjust the conversion accuracy to a lower value using "
                     "the slider provided.","OK");
  else
    { 
      update_dims.augment(renderer->shape_editor_adjust_path_thickness());
      renderer->update_shape_editor(update_dims,hide_shape_paths,
                                    fixed_shape_path_thickness,
                                    shape_scribbling_active);
    }
  [self update_shape_editor_controls];
}

/*****************************************************************************/
/*           kdms_metadata_editor:[ACTIONS for Content Editing]              */
/*****************************************************************************/

- (IBAction) changeBoxType:(id)sender
{
  int idx = [box_type_popup indexOfSelectedItem];
  if ((idx < 0) || (idx >= state->num_offered_types) ||
      (state->selected_type == state->offered_types[idx]) ||
      editing_shape || !state->can_edit_node)
    return;
  [self map_controls_to_state];
  state->selected_type = state->offered_types[idx];
  state->set_external_file(NULL);
  if (state->selected_type->box_type == jp2_label_4cc)
    state->label_string = "<new label>";
  else
    {
      state->label_string = NULL;
      kdms_file *file =
        file_server->retain_tmp_file(state->selected_type->file_suffix);
      file->create_if_necessary(state->selected_type->initializer);
      state->set_external_file(file,true);
    }
  state->set_external_file_replaces_contents();
      // Does nothing if no external file
  [self map_state_to_controls];
}

- (IBAction) saveToFile:(id)sender
{
  if (state->is_link)
    return;
  kdms_file *existing_file = state->get_external_file();
  const char *filename =
    choose_save_file_in_finder(state->selected_type->file_suffix,
                               existing_file);
  if (filename == NULL)
    return;
  kdms_file *new_file = file_server->retain_known_file(filename);
  [self map_controls_to_state];
  if (existing_file != NULL)
    copy_file(existing_file,new_file);
  state->set_external_file(new_file);
  if ((existing_file == NULL) && !state->save_to_external_file(edit_state))
    new_file->create_if_necessary(state->selected_type->initializer);
  [self map_state_to_controls];
}

- (IBAction) chooseFile:(id)sender
{
  if (editing_shape || !state->can_edit_node)
    return;
  [self map_controls_to_state];
  const char *filename =
    choose_open_file_in_finder(state->selected_type->file_suffix,
                               state->get_external_file());
  if (filename != NULL)
    {
      kdms_file *file = file_server->retain_known_file(filename);
      file->create_if_necessary(state->selected_type->initializer);
      state->set_external_file(file,true);
      state->set_external_file_replaces_contents();
      state->label_string = NULL;
    }
  [self map_state_to_controls];
}

- (IBAction) editFile:(id)sender
{
  if (editing_shape || (!state->can_edit_node) ||
      (state->selected_type == NULL))
    return;
  [self map_controls_to_state];
  kdms_file *external_file = state->get_external_file();
  if (external_file == NULL)
    {
      state->save_to_external_file(edit_state);
      external_file = state->get_external_file();
    }
  if (external_file == NULL)
    { // Should not happen
      NSBeep(); return;
    }
  external_file->create_if_necessary(state->selected_type->initializer);
  
  const char *file_suffix = state->selected_type->file_suffix;
  kdms_file_editor *editor=NULL;
  int idx = [box_editor_popup indexOfSelectedItem];
  if (idx > 0)
    editor = file_server->get_editor_for_doc_type(file_suffix,idx-1);
  if (editor != NULL) // Put it at the head of the list of known editors
    file_server->add_editor_for_doc_type(file_suffix,editor->app_pathname);
  else
    editor = choose_editor_in_finder(file_server,file_suffix);
  if (editor == NULL)
    NSBeep();
  else
    {
      FSRef item_ref;
      OSStatus os_status =
        FSPathMakeRef((UInt8 *)external_file->get_pathname(),&item_ref,NULL);
      if (os_status == 0)
        {
          LSLaunchFSRefSpec launch_spec;
          launch_spec.appRef = &(editor->fs_ref);
          launch_spec.numDocs = 1;
          launch_spec.itemRefs = &item_ref;
          launch_spec.passThruParams = NULL;
          launch_spec.launchFlags = kLSLaunchDefaults;
          launch_spec.asyncRefCon = NULL;
          os_status = LSOpenFromRefSpec(&launch_spec,NULL);
        }
      if (os_status == 0)
        state->set_external_file_replaces_contents();
      else
        NSBeep();
    }
  [self map_state_to_controls];
}

- (IBAction) changeEditor:(id)sender
{
  if (editing_shape || (!state->can_edit_node) ||
      (state->selected_type == NULL))
    return;
  [self map_controls_to_state];
  kdms_file_editor *editor = NULL;
  const char *file_suffix = state->selected_type->file_suffix;
  int idx = [box_editor_popup indexOfSelectedItem];
  if (idx > 0)
    editor = file_server->get_editor_for_doc_type(file_suffix,idx-1);
  if (editor == NULL)
    editor = choose_editor_in_finder(file_server,file_suffix);
  if (editor != NULL)
    file_server->add_editor_for_doc_type(file_suffix,editor->app_pathname);
  [self map_state_to_controls];
}

- (IBAction) internalizeLabel:(id)sender
{
  [self map_controls_to_state];
  if (editing_shape || !state->internalize_label_node())
    {
      NSBeep();
      return;
    }
  [self map_state_to_controls];      
}

/*****************************************************************************/
/*       kdms_metadata_editor:[ACTIONS which work with the Pastebar]         */
/*****************************************************************************/

- (IBAction) clickedCopyLink:(id)sender
{
  if (editing_shape || (!edit_state->cur_node.exists()) || (!allow_edits) ||
      (state->is_cross_reference && !state->is_link))
    return;
  jpx_metanode link_target;
  jpx_metanode_link_type link_type;
  if (state->is_link)
    link_target = edit_state->cur_node.get_link(link_type);
  else
    link_target = edit_state->cur_node;
  if (renderer->catalog_source == nil)
    [renderer->window create_metadata_catalog];
  if (link_target.exists() && renderer->catalog_source)
    [renderer->catalog_source paste_link:link_target];
}

- (IBAction) clickedPaste:(id)sender
{
  if (editing_shape || (renderer->catalog_source == nil) ||
      (state->selected_type->box_type == jp2_roi_description_4cc) ||
      !allow_edits)
    return;
  const char *pasted_label = [renderer->catalog_source get_pasted_label];
  jpx_metanode pasted_link = [renderer->catalog_source get_pasted_link];
  jpx_metanode node_to_cut = [renderer->catalog_source get_pasted_node_to_cut];
  if ((pasted_label == NULL) && (!pasted_link) && (!node_to_cut))
    {
      interrogate_user("You need to copy/link/cut something to the catalog "
                       "paste bar before you can paste over the current node "
                       "in the editor.","OK");
      return;
    }
  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
  if (pasted_link.exists())
    {
      int user_response =
        interrogate_user("What type of link would you like to add?\n"
                         "  \"Grouping links\" can have their own "
                         "descendants, all of which can be understood as "
                         "members of the same group.\n"
                         "  \"Alternate child\" is the most natural type of "
                         "link -- it suggests that the target of the link "
                         "and all its descendants could be considered "
                         "descendants of the node within which the link is "
                         "found.\n"
                         "  \"Alternate parent\" suggests that the target "
                         "of the link could be considered a parent of the "
                         "node within which the link is found.\n",
                         "Grouping","Alternate child","Alternate parent",
                         "Cancel");
      switch (user_response) {
        case 0: link_type = JPX_GROUPING_LINK; break;
        case 1: link_type = JPX_ALTERNATE_CHILD_LINK; break;
        case 2: link_type = JPX_ALTERNATE_PARENT_LINK; break;
        default: return;
      }
    }
  [self map_controls_to_state];  
  if (!edit_state->cur_node)
    { // Save the node first, so we can restrict our attention to replacing it
      try {
        if (!state->save_metanode(edit_state,initial_state,renderer))
          {
            NSBeep();
            [self update_apply_buttons];
            return;
          }
      }
      catch (kdu_exception)
      {
        [self preconfigure];
        return;
      }
    }

  kdms_file *file=get_file_retained_by_node(edit_state->cur_node,file_server);
  if (file != NULL)
    file->release();
  try {
    if (pasted_label != NULL)
      edit_state->cur_node.change_to_label(pasted_label);
    else if (pasted_link.exists())
      edit_state->cur_node.change_to_link(pasted_link,link_type);
    else if (node_to_cut.exists())
      {
        if (!node_to_cut.change_parent(edit_state->cur_node.get_parent()))
          { kdu_error e; e << "Cut and paste operation seems to be trying "
            "to move a node (along with its descendants) into its own "
            "descendant hierarchy!"; }
        if (edit_state->cur_node == edit_state->root_node)
          {
            edit_state->root_node = node_to_cut;
            if (edit_state->edit_item != NULL)
              edit_state->edit_item->node = node_to_cut;
          }
        edit_state->cur_node.delete_node();
        edit_state->cur_node = node_to_cut;
        edit_state->validate_metalist_and_root_node();
      }
  }
  catch (kdu_exception)
  {
    renderer->metadata_changed(true);
    [self map_state_to_controls];
    return;
  }
  renderer->metadata_changed(true);
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];
}

- (IBAction) clickedCopyDescendants:(id)sender
{
  jpx_metanode pasted_link = [renderer->catalog_source get_pasted_link];
  if (!pasted_link)
    {
      interrogate_user("You need to copy a link to the catalog pastebar "
                       "before you can copy the present node's descendants "
                       "across as descendants of the target of that link.",
                       "OK");
      return;
    }
  int user_choice =
    interrogate_user("You are about to copy all descendants of the current "
                     "node across as descendants of the node targeted by "
                     "the link in the catalog pastebar.  As an added feature, "
                     "you can opt to also make copies of all link nodes (and "
                     "their descendants) which currently point to the current "
                     "node -- that is, you can treat links to the current "
                     "node as if they were descendants.  Doing this now "
                     "will help to ensure that all copied links point "
                     "correctly to the relevant copies of their original "
                     "targets.",
                     "Copy all","Descendants only","Cancel");
  if (user_choice == 2)
    return;
  bool copy_linkers = (user_choice == 0);
  jpx_metanode root_metanode = edit_state->meta_manager.access_root();
  edit_state->meta_manager.reset_copy_locators(root_metanode,true);
  copy_descendants_with_file_retain(edit_state->cur_node,pasted_link,
                                    file_server,copy_linkers);
  edit_state->meta_manager.reset_copy_locators(root_metanode,true,true);
  renderer->metadata_changed(true);
}

/*****************************************************************************/
/*        kdms_metadata_editor:[ACTIONS for Navigation and Control]          */
/*****************************************************************************/

- (IBAction) clickedExit:(id)sender
{
  [self map_controls_to_state];
  if (editing_shape)
    {
      if (!interrogate_user("Exiting the metadata editor will cancel your "
                            "open ROI shape editor.","Cancel","Exit"))
        {
          [self update_apply_buttons];
          return;
        }
      [self stop_shape_editor];
    }
  if (!edit_state->cur_node.exists())
    {
      if (!interrogate_user("Are you sure you want to exit without adding "
                            "any metadata?  To do so, click the \"Apply\" "
                            "button before exiting.","Cancel","Exit"))
        {
          [self update_apply_buttons];
          return;
        }
    }
  else if (!(state->compare(initial_state) ||
             interrogate_user("You have made some changes; are you sure you "
                              "want to exit without applying these changes by "
                              "clicking \"Apply\"?","Cancel","Exit")))
    {
      [self update_apply_buttons];
      return;
    }
  [self close];
}

- (IBAction) clickedDelete:(id)sender
{
  if (editing_shape)
    {
      if (!interrogate_user("Deleting this node will cancel your "
                            "open ROI shape editor.","Cancel","Exit"))
        return;
      [self stop_shape_editor];
    }
  if (edit_state->cur_node.exists())
    { // Otherwise we haven't added anything yet, so there's nothing to delete
      if (interrogate_user("Are you sure you want to delete this node and "
                           "all its descendants?","Delete","Cancel"))
        {
          [self update_apply_buttons];
          return;
        }
      try {
        edit_state->delete_cur_node(renderer);
      }
      catch (kdu_exception) {
        [self preconfigure];
        return;
      }
    }
  if (!edit_state->cur_node.exists())
    [self close]; // Nothing left to edit
  else
    {
      state->configure_with_metanode(edit_state->cur_node);
      initial_state->copy(state);
      [self map_state_to_controls];      
    }
}

- (IBAction) clickedApply:(id)sender
{
  if (editing_shape && ![self query_save_shape_edits])
    return;
  [self map_controls_to_state];
  try {
    bool create_label = !edit_state->cur_node.exists();
    if (!state->save_metanode(edit_state,initial_state,renderer))
      {
        [self update_apply_buttons];
        return;
      }
    if (create_label)
      {
        edit_state->add_child_node(renderer);
        state->configure_with_metanode(edit_state->cur_node);
      }
  }
  catch (kdu_exception)
    {
      [self preconfigure];
      return;
    }
  initial_state->copy(state); // Now everthing starts again from scratch
  [self map_state_to_controls]; // May alter the navigation options
}

- (IBAction) clickedApplyAndExit:(id)sender
{
  if (editing_shape && ![self query_save_shape_edits])
    return;
  [self map_controls_to_state];
  try {
    if (!state->save_metanode(edit_state,initial_state,renderer))
      {
        [self update_apply_buttons];
        return;
      }
  }
  catch (kdu_exception) { }
  [self close];
}

- (IBAction) clickedNext:(id)sender
{
  if (editing_shape)
    return;
  [self map_controls_to_state];
  if (!(state->compare(initial_state) ||
        interrogate_user("Changes you have made will be lost unless you click "
                         "\"Apply\" before navigating to another node in the "
                         "metadata tree.","Cancel","Continue")))
    {
      [self update_apply_buttons];
      return;
    }
  edit_state->move_to_next();
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];
}

- (IBAction) clickedPrev:(id)sender
{
  if (editing_shape)
    return;
  [self map_controls_to_state];
  if (!(state->compare(initial_state) ||
        interrogate_user("Changes you have made will be lost unless you click "
                         "\"Apply\" before navigating to another node in the "
                         "metadata tree.","Cancel","Continue")))
    {
      [self update_apply_buttons];
      return;
    }
  edit_state->move_to_prev();
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];  
}

- (IBAction) clickedParent:(id)sender
{
  if (editing_shape)
    return;
  [self map_controls_to_state];
  if (!(state->compare(initial_state) ||
        interrogate_user("Changes you have made will be lost unless you click "
                         "\"Apply\" before navigating to another node in the "
                         "metadata tree.","Cancel","Continue")))
    {
      [self update_apply_buttons];
      return;
    }
  edit_state->move_to_parent();
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];  
}

- (IBAction) clickedDescendants:(id)sender
{
  if (editing_shape)
    return;
  [self map_controls_to_state];
  if (!(state->compare(initial_state) ||
        interrogate_user("Changes you have made will be lost unless you click "
                         "\"Apply\" before navigating to another node in the "
                         "metadata tree.","Cancel","Continue")))
    {
      [self update_apply_buttons];
      return;
    }
  edit_state->move_to_descendants();
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];
}

- (IBAction) clickedAddChild:(id)sender
{
  if (editing_shape || !allow_edits)
    return;
  [self map_controls_to_state];
  if (!edit_state->cur_node.exists())
    {
      interrogate_user("You need to click \"Create\" to add the primary node "
                       "to the metadata tree before adding a child.",
                       "OK","");
      {
        [self update_apply_buttons];
        return;
      }
    }
  if (!(state->compare(initial_state) ||
        interrogate_user("Changes you have made will be lost unless you click "
                         "\"Apply\" or \"Create\" before adding a new child "
                         "node to the metadata tree.","Cancel","Continue")))
    {
      [self update_apply_buttons];
      return;
    }
  edit_state->add_child_node(renderer);
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];
}

- (IBAction) clickedAddParent:(id)sender
{
  if ((state->selected_type->box_type == jp2_roi_description_4cc) ||
      editing_shape || !allow_edits)
    return;
  [self map_controls_to_state];
  if (!edit_state->cur_node.exists())
    {
      interrogate_user("You need to click \"Create\" to add the current node "
                       "to the metadata tree before inserting a parent.","OK");
      {
        [self update_apply_buttons];
        return;
      }
    }
  if (!(state->compare(initial_state) ||
        interrogate_user("Changes you have made will be lost unless you click "
                         "\"Apply\" or \"Create\" before inserting a new "
                         "parent into the metadata tree.",
                         "Cancel","Continue")))
    {
      [self update_apply_buttons];
      return;
    }
  edit_state->add_parent_node(renderer);
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  [self map_state_to_controls];
}

- (IBAction) findInMetashow:(id)sender
{
  if (!edit_state->cur_node)
    return;
  if (!renderer->metashow)
    renderer->menu_MetadataShow();
  if (renderer->metashow)
    [renderer->metashow select_matching_metanode:edit_state->cur_node]; 
}

- (IBAction) findInCatalog:(id)sender
{
  if (!edit_state->cur_node)
    return;
  if (!renderer->catalog_source)
    renderer->menu_MetadataCatalog();
  jpx_metanode node=edit_state->cur_node;
  if (renderer->catalog_source)
    [renderer->catalog_source select_matching_metanode:node]; 
}

/*****************************************************************************/
/*                 kdms_metadata_editor:[Generic ACTIONS]                    */
/*****************************************************************************/

- (IBAction) revertToInitialState:(id)sender
{
  if (editing_shape)
    {
      if (!interrogate_user("Reverting to the original state will cancel your "
                            "open ROI shape editor.","Cancel","Exit"))
        return;
      [self stop_shape_editor];
    }
  state->copy(initial_state);
  [self map_state_to_controls];
}

- (IBAction) otherAction:(id)sender
{
  [self map_controls_to_state];
  [self update_apply_buttons];
}

/*****************************************************************************/
/*                kdms_metadata_editor:[Delegation messages]                 */
/*****************************************************************************/

- (IBAction) controlTextDidChange:(NSNotification *)notification
{
  [self map_controls_to_state];
  [self update_apply_buttons];
}

@end
