/*****************************************************************************/
// File: kdws_metadata_editor.cpp [scope = APPS/WINSHOW]
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
   Implementation of the metadata display dialog in the "kdu_show" application.
******************************************************************************/
#include "stdafx.h"
#include "kdws_renderer.h"
#include "kdws_metadata_editor.h"
#include "kdws_window.h"
#include "kdws_metashow.h"
#include "kdws_catalog.h"
#include "kdws_manager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*===========================================================================*/
/*                           INTERNAL FUNCTIONS                              */
/*===========================================================================*/

/*****************************************************************************/
/* STATIC                     get_nonneg_integer                             */
/*****************************************************************************/

static void get_nonneg_integer(CEdit *field, int &val)
{
  kdws_string string(80);
  int label_length = field->LineLength();
  if (label_length > 80)
    label_length = 80;
  field->GetLine(0,string,label_length);
  const char *str = string;
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

static bool check_integer_in_range(kdws_metadata_editor *dlg,
                                   CEdit *field, int min_val,
                                   int max_val)
{
  kdws_string string(80);
  int label_length = field->LineLength();
  if (label_length > 80)
    label_length = 80;
  field->GetLine(0,string,label_length);
  const char *str = string;
  int val = 0;
  for (; *str != '\0'; str++)
    if (!isdigit(*str))
      break;
    else
      val = (val*10) + (int)(*str - '0');
  if ((*str == '\0') && (val >= min_val) && (val <= max_val))
    return true;
  sprintf(string,"Text field requires integer in range %d to %d",
          min_val,max_val);
  dlg->interrogate_user(string,"Kdu_show: Dialog entry error",
                  MB_OK|MB_ICONERROR);
  dlg->GotoDlgCtrl(field);
  return false;
}

/*****************************************************************************/
/* STATIC                         copy_file                                  */
/*****************************************************************************/

static void
  copy_file(kdws_file *existing_file, kdws_file *new_file)
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
}

/*****************************************************************************/
/* STATIC                      choose_editor                                 */
/*****************************************************************************/

static kdws_file_editor *
  choose_editor(kdws_file_services *file_server, const char *doc_suffix)
{
 kdws_string pathname(MAX_PATH);
  OPENFILENAME ofn;
  memset(&ofn,0,sizeof(ofn));  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFilter = _T("Executable file\0*.exe\0\0");
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = pathname;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = _T("Choose editor application");
  ofn.lpstrInitialDir = _T("c:\\Program Files");
  ofn.Flags = OFN_FILEMUSTEXIST;
  if (!GetOpenFileName(&ofn))
    return NULL;
  return file_server->add_editor_for_doc_type(doc_suffix,pathname);
}

/*****************************************************************************/
/* STATIC                 choose_open_file_pathname                          */
/*****************************************************************************/

static bool
  choose_open_file_pathname(const char *suffix, kdws_file *existing_file,
                            char pathname[], int max_pathname_chars)
  /* Allows the user to select an existing file, with the indicated suffix
     (extension) in the finder.  If `existing_file' is non-NULL, the function
     uses the existing file's pathname to decide which directory to start the
     finder in.  If the user selects a file, the function returns true,
     writing the file pathname into `pathname'. */
{
  OPENFILENAME ofn;
  memset(&ofn,0,sizeof(ofn));  ofn.lStructSize = sizeof(ofn);
  char initial_dir_buf[MAX_PATH+1];
  const char *initial_dir = NULL;
  if (existing_file != NULL)
    {
      initial_dir = initial_dir_buf;
      strncpy(initial_dir_buf,existing_file->get_pathname(),MAX_PATH);
      initial_dir_buf[MAX_PATH] = '\0';
      char *cp = initial_dir_buf + strlen(initial_dir) - 1;
      for (; (cp > initial_dir_buf) && (*cp != '/') && (*cp != '\\') &&
             (cp[-1] != ':'); cp--);
      if (cp > initial_dir_buf)
        *cp = '\0';
    }
  ofn.hwndOwner = NULL;
  ofn.lpstrFilter = _T("*.*\0*.*\0\0");
  kdws_string suffix_ext(20);
  if (strlen(suffix) < 8)
    {
      strcpy(suffix_ext,suffix);
      ofn.lpstrFilter = suffix_ext;
    }
  kdws_string pathname_string(max_pathname_chars);
  kdws_string initial_dir_string(initial_dir);
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = pathname_string;
  ofn.nMaxFile = max_pathname_chars;
  ofn.lpstrTitle = _T("Choose file for box contents");
  ofn.lpstrInitialDir = initial_dir_string;
  ofn.Flags = OFN_FILEMUSTEXIST;
  if (!GetOpenFileName(&ofn))
    return false;
  strcpy(pathname,pathname_string);
  return true;
}

/*****************************************************************************/
/* STATIC                 choose_save_file_pathname                          */
/*****************************************************************************/

static bool
  choose_save_file_pathname(const char *suffix, kdws_file *existing_file,
                            char pathname[], int max_pathname_chars)
  /* Same as `choose_open_file_pathname' except that you are choosing a
     file to save to, rather than an existing file to use. */
{
  OPENFILENAME ofn;
  memset(&ofn,0,sizeof(ofn));  ofn.lStructSize = sizeof(ofn);
  char initial_dir_buf[MAX_PATH+1];
  const char *initial_dir = NULL;
  if (existing_file != NULL)
    {
      initial_dir = initial_dir_buf;
      strncpy(initial_dir_buf,existing_file->get_pathname(),MAX_PATH);
      initial_dir_buf[MAX_PATH] = '\0';
      char *cp = initial_dir_buf + strlen(initial_dir) - 1;
      for (; (cp > initial_dir_buf) && (*cp != '/') && (*cp != '\\') &&
             (cp[-1] != ':'); cp--);
      if (cp > initial_dir_buf)
        *cp = '\0';
    }
  ofn.hwndOwner = NULL;
  kdws_string suffix_ext(20);
  ofn.lpstrFilter = _T("*.*\0*.*\0\0");
  if (strlen(suffix) < 8)
    {
      strcpy(suffix_ext,suffix);
      ofn.lpstrFilter = suffix_ext;
    }
  kdws_string pathname_string(max_pathname_chars);
  kdws_string initial_dir_string(initial_dir);
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = pathname_string;
  ofn.nMaxFile = max_pathname_chars;
  ofn.lpstrTitle = _T("Choose file to receive box contents");
  ofn.lpstrInitialDir = initial_dir_string;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
  if (!GetSaveFileName(&ofn))
    return false;
  strcpy(pathname,pathname_string);
  return true;
}

/*****************************************************************************/
/* STATIC                 get_file_retained_by_node                          */
/*****************************************************************************/

static kdws_file *
  get_file_retained_by_node(jpx_metanode node, kdws_file_services *file_server)
{
  int i_param;
  void *addr_param;
  kdws_file *result = NULL;
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
                                    kdws_file_services *file_server,
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
      kdws_file *file = get_file_retained_by_node(dst_child,file_server);
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
/*                           kdws_matching_metalist                          */
/*===========================================================================*/

/*****************************************************************************/
/*                  kdws_matching_metalist::find_container                   */
/*****************************************************************************/

kdws_matching_metanode *
  kdws_matching_metalist::find_container(jpx_metanode node)
{
  for (; node.exists(); node=node.get_parent())
    for (kdws_matching_metanode *scan=head; scan != NULL; scan=scan->next)
      if (scan->node == node)
        return scan;
  return NULL;
}

/*****************************************************************************/
/*                    kdws_matching_metalist::find_member                    */
/*****************************************************************************/

kdws_matching_metanode *
  kdws_matching_metalist::find_member(jpx_metanode node)
{
  for (kdws_matching_metanode *scan=head; scan != NULL; scan=scan->next)
    {
      jpx_metanode scan_node = scan->node;
      for (; scan_node.exists(); scan_node=scan_node.get_parent())
        if (scan_node == node)
          return scan;
    }
  return NULL;
}

/*****************************************************************************/
/*                   kdws_matching_metalist::delete_item                     */
/*****************************************************************************/

void kdws_matching_metalist::delete_item(kdws_matching_metanode *item)
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
/*                    kdws_matching_metalist::append_node                    */
/*****************************************************************************/

kdws_matching_metanode *kdws_matching_metalist::append_node(jpx_metanode node)
{
  kdws_matching_metanode *elt = new kdws_matching_metanode;
  elt->node = node;
  elt->next = NULL;
  if ((elt->prev = tail) == NULL)
    head = tail = elt;
  else
    tail = tail->next = elt;
  return elt;
}

/*****************************************************************************/
/*   kdws_matching_metalist::append_node_expanding_numlists_and_free_boxes   */
/*****************************************************************************/

void
  kdws_matching_metalist::append_node_expanding_numlists_and_free_boxes(
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
/*             kdws_matching_metalist::append_image_wide_nodes               */
/*****************************************************************************/

void kdws_matching_metalist::append_image_wide_nodes(jpx_meta_manager manager,
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
  if ((layer_idx >= 0) || (stream_idx >= 0))
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
  kdws_matching_metanode *scan, *next;
  for (scan=head; scan != NULL; scan=next)
    {
      next = scan->next;
      
      jpx_metanode ancestor;
      for (ancestor=scan->node;
           ancestor.exists(); ancestor=ancestor.get_parent())
        {
          kdws_matching_metanode *test;
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
/*                          kdws_metanode_edit_state                         */
/*===========================================================================*/

/*****************************************************************************/
/*           kdws_metanode_edit_state::kdws_metanode_edit_state              */
/*****************************************************************************/

kdws_metanode_edit_state::kdws_metanode_edit_state(jpx_meta_manager manager,
                                              kdws_file_services *file_server)
{
  this->meta_manager = manager;
  metalist = &internal_metalist;
  edit_item = NULL;
  cur_node_sibling_idx = 0;
  next_sibling_idx = prev_sibling_idx = -1;
  this->file_server = file_server;
}

/*****************************************************************************/
/*                 kdws_metanode_edit_state::move_to_parent                  */
/*****************************************************************************/

void kdws_metanode_edit_state::move_to_parent()
{
  if ((cur_node == root_node) || !cur_node.exists())
    return;
  cur_node = cur_node.get_parent();
  cur_node_sibling_idx = next_sibling_idx = prev_sibling_idx = -1;
  if (cur_node != root_node)
    find_sibling_indices();
}

/*****************************************************************************/
/*                  kdws_metanode_edit_state::move_to_next                   */
/*****************************************************************************/

void kdws_metanode_edit_state::move_to_next()
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
/*                  kdws_metanode_edit_state::move_to_prev                   */
/*****************************************************************************/

void kdws_metanode_edit_state::move_to_prev()
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
/*              kdws_metanode_edit_state::move_to_descendants                */
/*****************************************************************************/

void kdws_metanode_edit_state::move_to_descendants()
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
/*                 kdws_metanode_edit_state::move_to_node                    */
/*****************************************************************************/

void kdws_metanode_edit_state::move_to_node(jpx_metanode node)
{
  kdws_matching_metanode *container = metalist->find_container(node);
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
/*                kdws_metanode_edit_state::delete_cur_node                  */
/*****************************************************************************/

void
  kdws_metanode_edit_state::delete_cur_node(kdws_renderer *renderer)
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
      kdws_file *file = file_server->find_file(i_param);
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
/*        kdws_metanode_edit_state::validate_metalist_and_root_node          */
/*****************************************************************************/

void kdws_metanode_edit_state::validate_metalist_and_root_node()
{
  kdws_matching_metanode *item, *next_item;
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
/*                 kdws_metanode_edit_state::add_child_node                  */
/*****************************************************************************/

void
  kdws_metanode_edit_state::add_child_node(kdws_renderer *renderer)
{
  assert(cur_node.exists());
  jpx_metanode new_node;
  try {
    new_node = cur_node.add_label("<new label>");
  }
  catch (int) {
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
/*                kdws_metanode_edit_state::add_parent_node                  */
/*****************************************************************************/

void
  kdws_metanode_edit_state::add_parent_node(kdws_renderer *renderer)
{
  assert(cur_node.exists());
  jpx_metanode parent = cur_node.get_parent();
  jpx_metanode new_parent;
  try {
    new_parent = parent.add_label("<new parent>");
    cur_node.change_parent(new_parent);
  }
  catch (int) {
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
/*              kdws_metanode_edit_state::find_sibling_indices               */
/*****************************************************************************/

void kdws_metanode_edit_state::find_sibling_indices()
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
/*                        kdws_metadata_dialog_state                         */
/*===========================================================================*/

/*****************************************************************************/
/*         kdws_metadata_dialog_state::kdws_metadata_dialog_state            */
/*****************************************************************************/

kdws_metadata_dialog_state::kdws_metadata_dialog_state(
                                         kdws_metadata_editor *dlg,
                                         int num_codestreams,
                                         int num_layers,
                                         kdws_box_template *available_types,
                                         kdws_file_services *file_server,
                                         bool allow_edits)
{
  this->dlg = dlg;
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
  num_roi_regions = 0;
  roi_is_elliptical = roi_is_encoded = false;
  this->available_types = available_types;
  memset(offered_types,0,sizeof(int *) * KDWS_NUM_BOX_TEMPLATES);
  num_offered_types = 1;
  offered_types[0] = available_types + KDWS_LABEL_TEMPLATE;
  selected_type = offered_types[0];
  label_string = NULL;
  this->file_server = file_server;
  external_file = NULL;
  external_file_replaces_contents = false;
}

/*****************************************************************************/
/*         kdws_metadata_dialog_state::~kdws_metadata_dialog_state           */
/*****************************************************************************/

kdws_metadata_dialog_state::~kdws_metadata_dialog_state()
{
  if (selected_codestreams != NULL)
    delete[] selected_codestreams;
  if (selected_layers != NULL)
    delete[] selected_layers;
  if (external_file != NULL)
    external_file->release();
  if (label_string != NULL)
    delete[] label_string;
}

/*****************************************************************************/
/*              kdws_metadata_dialog_state::compare_associations             */
/*****************************************************************************/

bool kdws_metadata_dialog_state::compare_associations(
                                          kdws_metadata_dialog_state *ref)
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
    return ((num_roi_regions == ref->num_roi_regions) &&
            (roi_bounding_box == ref->roi_bounding_box) &&
            (roi_is_elliptical == ref->roi_is_elliptical) &&
            (roi_is_encoded == ref->roi_is_encoded));
}

/*****************************************************************************/
/*                kdws_metadata_dialog_state::compare_contents               */
/*****************************************************************************/

bool kdws_metadata_dialog_state::compare_contents(
                                          kdws_metadata_dialog_state *ref)
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
/*        kdws_metadata_dialog_state::eliminate_duplicate_codestreams        */
/*****************************************************************************/

void kdws_metadata_dialog_state::eliminate_duplicate_codestreams()
{
  int n, m;
  for (n=0; n < num_selected_codestreams; n++)
    {
      for (m=0; m < n; m++)
        if (selected_codestreams[m] == selected_codestreams[n])
          break;
      if (m < n)
        { // entry n is a duplicate codestream
          num_selected_codestreams--;
          for (m=n; m < num_selected_codestreams; m++)
            selected_codestreams[m] = selected_codestreams[m+1];
          n--; // So main loop visits all locations
        }
    }
}

/*****************************************************************************/
/*           kdws_metadata_dialog_state::eliminate_duplicate_layers          */
/*****************************************************************************/

void kdws_metadata_dialog_state::eliminate_duplicate_layers()
{
  int n, m;
  for (n=0; n < num_selected_layers; n++)
    {
      for (m=0; m < n; m++)
        if (selected_layers[m] == selected_layers[n])
          break;
      if (m < n)
        { // entry n is a duplicate codestream
          num_selected_layers--;
          for (m=n; m < num_selected_layers; m++)
            selected_layers[m] = selected_layers[m+1];
          n--; // So main loop visits all locations
        }
    }
}

/*****************************************************************************/
/*            kdws_metadata_dialog_state::add_selected_codestream            */
/*****************************************************************************/

int kdws_metadata_dialog_state::add_selected_codestream(int idx)
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
/*              kdws_metadata_dialog_state::add_selected_layer               */
/*****************************************************************************/

int kdws_metadata_dialog_state::add_selected_layer(int idx)
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
/*            kdws_metadata_dialog_state::configure_with_metanode            */
/*****************************************************************************/

void kdws_metadata_dialog_state::configure_with_metanode(jpx_metanode node)
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
      offered_types[0] = available_types + KDWS_LABEL_TEMPLATE;
      offered_types[1] = available_types + KDWS_XML_TEMPLATE;
      offered_types[2] = available_types + KDWS_IPR_TEMPLATE;
    }
  else if (box_type == jp2_xml_4cc)
    {
      num_offered_types = 3;
      offered_types[1] = available_types + KDWS_LABEL_TEMPLATE;
      offered_types[0] = available_types + KDWS_XML_TEMPLATE;
      offered_types[2] = available_types + KDWS_IPR_TEMPLATE;
    }
  else if (box_type == jp2_iprights_4cc)
    {
      num_offered_types = 3;
      offered_types[2] = available_types + KDWS_LABEL_TEMPLATE;
      offered_types[1] = available_types + KDWS_XML_TEMPLATE;
      offered_types[0] = available_types + KDWS_IPR_TEMPLATE;
    }
  else
    {
      num_offered_types = 1;
      offered_types[0] = available_types + KDWS_CUSTOM_TEMPLATE;
      available_types[KDWS_CUSTOM_TEMPLATE].init(box_type,NULL,NULL);
    }
  if (!can_edit_node)
    num_offered_types = 1; // Do not offer other box-types
  selected_type = offered_types[0];
  set_label_string(node.get_label());
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
  num_roi_regions = 0;
  roi_bounding_box = kdu_dims();
  roi_is_encoded = false;
  roi_is_elliptical = false;
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
      else if (whole_image &&
               ((num_roi_regions = scan.get_num_regions()) > 0))
        {
          whole_image = false;
          roi_bounding_box = scan.get_bounding_box();
          const jpx_roi *regions = scan.get_regions();
          for (n=0; n < num_roi_regions; n++)
            {
              if (regions[n].is_elliptical)
                roi_is_elliptical = true;
              if (regions[n].is_encoded)
                roi_is_encoded = true;
            }
        }
    }
}

/*****************************************************************************/
/*                     kdws_metadata_dialog_state::copy                      */
/*****************************************************************************/

void kdws_metadata_dialog_state::copy(kdws_metadata_dialog_state *src)
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
  roi_bounding_box = src->roi_bounding_box;
  roi_is_elliptical = src->roi_is_elliptical;
  roi_is_encoded = src->roi_is_encoded;
  num_roi_regions = src->num_roi_regions;
  num_offered_types = src->num_offered_types;
  selected_type = src->selected_type;
  external_file_replaces_contents = src->external_file_replaces_contents;
  this->set_external_file(src->external_file);
  set_label_string(src->label_string);
  for (int n=0; n < num_offered_types; n++)
    offered_types[n] = src->offered_types[n];
}

/*****************************************************************************/
/*            kdws_metadata_dialog_state::save_to_external_file              */
/*****************************************************************************/

bool kdws_metadata_dialog_state::save_to_external_file(
                                               kdws_metanode_edit_state *edit)
{
  if (external_file == NULL)
    set_external_file(file_server->retain_tmp_file(selected_type->file_suffix),
                      true);
  if ((selected_type->box_type == jp2_label_4cc) && (label_string != NULL))
    { // Save the label string
      FILE *file = fopen(external_file->get_pathname(),"w");
      if (file != NULL)
        {
          fwrite(label_string,1,strlen(label_string),file);
          fclose(file);
          return true;
        }
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
/*           kdws_metadata_dialog_state::internalize_label_node              */
/*****************************************************************************/

bool kdws_metadata_dialog_state::internalize_label_node()
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
      dlg->interrogate_user(
                 "External file contains more than 4096 characters!  "
                 "Managing internal labels of this size can become "
                 "impractical -- you should consider using an XML "
                 "box for large amounts of text.",
                 "Kdu_show: Metadata entry error",
                 MB_OK | MB_ICONERROR);
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
/*                 kdws_metadata_dialog_state::save_metanode                 */
/*****************************************************************************/

bool
  kdws_metadata_dialog_state::save_metanode(kdws_metanode_edit_state *edit,
                                            kdws_metadata_dialog_state *ref,
                                            kdws_renderer *renderer)
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
          (dlg->interrogate_user(
                      "The changes you have made require the metadata "
                      "you have edited to be associated with a new "
                      "region of interest and/or a new set of image "
                      "entities (codestreams, layers or rendered "
                      "result).  In the process, you may lose existing "
                      "associations with other metadata in the node's "
                      "ancestry.  Consider making these changes to the "
                      "node's parent instead (click \"revert\" and "
                      "then \"Up to parent\" to do this).",
                      "Kdu_show: May lose existing associations",
                      MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL))
        return false;
    }
  
  if (changed_associations || !edit->cur_node.exists())
    { // Need to create a new set of associations
      jpx_roi bounding_region;
      bounding_region.region = roi_bounding_box;
      bounding_region.is_elliptical = roi_is_elliptical;
      bounding_region.is_encoded = false;
      bounding_region.coding_priority = 0;
      if ((num_roi_regions > 0) && (num_selected_codestreams == 0))
        {
          dlg->interrogate_user(
                     "The region of interest (ROI) you have selected "
                     "must be associated with at least one codestream "
                     "in order to have meaning.  The ROI dimensions "
                     "are necessarily interpreted with respect to the "
                     "coordinate system of a codestream.  To proceed, "
                     "you should either add an associated codestream "
                     "or else click the \"Whole Image\" button.",
                     "Kdu_show: Add an associated codestream",
                     MB_OK | MB_ICONERROR);
          return false;
        }
        
      const jpx_roi *roi_regions = NULL;
      if ((num_roi_regions == 1) && !roi_is_encoded)
        roi_regions = &bounding_region;
      else if (num_roi_regions > 0)
        { // Get region list from original node
          jpx_metanode scan;
          for (scan=edit->cur_node; scan.exists(); scan=scan.get_parent())
            if (scan.get_num_regions() == num_roi_regions)
              {
                roi_regions = scan.get_regions();
                break;
              }
          if (roi_regions == NULL)
            {
              assert(0);
              num_roi_regions = 1; // So nothing breaks
              roi_is_encoded = false;
              roi_regions = &bounding_region;
            }
        }
      eliminate_duplicate_codestreams();
      eliminate_duplicate_layers();
      jpx_metanode container =
        edit->meta_manager.insert_node(num_selected_codestreams,
                                       selected_codestreams,
                                       num_selected_layers,
                                       selected_layers,
                                       rendered_result,
                                       num_roi_regions,roi_regions);
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
          int i_param;
          void *addr_param;
          kdws_file *existing_file = NULL;
          if (node.get_delayed(i_param,addr_param) &&
              (addr_param == (void *)file_server))
            existing_file = file_server->find_file(i_param);
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

      kdws_matching_metanode *item;
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
      kdws_file *existing_file = NULL;
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


/* ========================================================================= */
/*                          kdws_metadata_editor                             */
/* ========================================================================= */

/*****************************************************************************/
/*                kdws_metadata_editor::kdws_metadata_editor                 */
/*****************************************************************************/

kdws_metadata_editor::kdws_metadata_editor(jpx_source *source,
                                           kdws_file_services *file_services,
                                           bool can_edit,
                                           kdws_renderer *renderer,
                                           POINT preferred_location,
                                           int window_identifier,
                                           CWnd* pParent)
{
  this->dialog_created = false;
  this->mapping_state_to_controls = false;

  this->source = source;
  this->allow_edits = can_edit;
  this->file_server = file_services;
  this->renderer = renderer;

  num_codestreams = 1;
  num_layers = 1;
  edit_state = NULL;
  state = NULL;
  initial_state = NULL;
  owned_metalist = NULL;
  
  available_types[KDWS_LABEL_TEMPLATE].init(jp2_label_4cc,"txt",
                                            "<new label>");
  available_types[KDWS_XML_TEMPLATE].init(jp2_xml_4cc,"xml",
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r");
  available_types[KDWS_IPR_TEMPLATE].init(jp2_iprights_4cc,"xml",
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r");

  if (!this->Create(kdws_metadata_editor::IDD,pParent))
    { kdu_warning w;
      w << "Unable to load/create the metdata editor dialog.";
      return;
    }
  dialog_created = true;

  RECT frame_rect; GetWindowRect(&frame_rect);
  int screen_width = GetSystemMetrics(SM_CXSCREEN);
  int screen_height = GetSystemMetrics(SM_CYSCREEN);
  int x_off = preferred_location.x - frame_rect.left;
  int y_off = preferred_location.y - frame_rect.top;
  if ((frame_rect.right+x_off) > screen_width)
    x_off = screen_width - frame_rect.right;
  if ((frame_rect.bottom+y_off) > screen_height)
    y_off = screen_height - frame_rect.bottom;
  if ((frame_rect.left+x_off) < 0)
    x_off = -frame_rect.left;
  if ((frame_rect.top+y_off) < 0)
    y_off = -frame_rect.top;
  frame_rect.left += x_off;  frame_rect.right  += x_off;
  frame_rect.top  += y_off;  frame_rect.bottom += y_off;
  SetWindowPos(NULL,frame_rect.left,frame_rect.top,0,0,
               SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);

  char title[40];
  sprintf(title,"Metadata editor %d",window_identifier);
  SetWindowText(kdws_string(title));
  EnableToolTips(TRUE);
}

/*****************************************************************************/
/*              kdws_metadata_editor::~kdws_metadata_editor                  */
/*****************************************************************************/

kdws_metadata_editor::~kdws_metadata_editor()
{
  if (edit_state != NULL)
    delete edit_state;
  if (state != NULL)
    delete state;
  if (initial_state != NULL)
    delete initial_state;
  if (owned_metalist != NULL)
    delete owned_metalist;
}

/*****************************************************************************/
/*                    kdws_metadata_editor:DestroyWindow                     */
/*****************************************************************************/

BOOL kdws_metadata_editor::DestroyWindow()
{
  if ((renderer != NULL) && (renderer->editor == this))
    renderer->editor = NULL;
  renderer = NULL;
  BOOL result = CDialog::DestroyWindow();
  delete this;
  return result;
}

/*****************************************************************************/
/*                     kdws_metadata_editor:preconfigure                     */
/*****************************************************************************/

void kdws_metadata_editor::preconfigure()
{
  source->count_codestreams(num_codestreams);
  if (num_codestreams < 1)
    num_codestreams = 1;
  source->count_compositing_layers(num_layers);
  if (num_layers < 1)
    num_layers = 1;

  if (edit_state != NULL)
    delete edit_state;
  edit_state = new
    kdws_metanode_edit_state(source->access_meta_manager(),
                             file_server);
  if (state != NULL)
    delete state;
  state = new
    kdws_metadata_dialog_state(this,num_codestreams,num_layers,
                               available_types,file_server,allow_edits);  
  if (initial_state != NULL)
    delete initial_state;
  initial_state = new
    kdws_metadata_dialog_state(this,num_codestreams,num_layers,
                               available_types,file_server,allow_edits);
  
  if (owned_metalist != NULL)
    delete owned_metalist;
  owned_metalist = NULL;
  
  map_state_to_controls();
}

/*****************************************************************************/
/*                 kdws_metadata_editor::configure (metalist)                */
/*****************************************************************************/

void
  kdws_metadata_editor::configure(kdws_matching_metalist *metalist)
{
  if (!dialog_created)
    return;
  SetWindowPos(&wndTop,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
  if ((state != NULL) && (initial_state != NULL) &&
      !(state->compare(initial_state) ||
        (interrogate_user("You have made some changes; are you sure you "
                          "want to change the editing context without "
                          "applying these changes?",
                          "Kdu_show: About to lose changes",
                          MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }

  this->preconfigure();
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
  map_state_to_controls();
}

/*****************************************************************************/
/*                    kdws_metadata_editor:setActiveNode                     */
/*****************************************************************************/

void kdws_metadata_editor::setActiveNode(jpx_metanode node)
{
  if (owned_metalist == NULL)
    return;
  edit_state->move_to_node(node);
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  map_state_to_controls();
}

/*****************************************************************************/
/*            kdws_metadata_editor::configure (initial region)               */
/*****************************************************************************/

void
  kdws_metadata_editor::configure(kdu_dims roi, int codestream_idx,
                                  int layer_idx,
                                  bool associate_with_rendered_result)
{
  if (!dialog_created)
    return;
  SetWindowPos(&wndTop,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
  if ((state != NULL) && (initial_state != NULL) &&
      !(state->compare(initial_state) ||
        (interrogate_user("You have made some changes; are you sure you "
                          "want to change the editing context without "
                          "applying these changes?",
                          "Kdu_show: About to lose changes",
                          MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }

  this->preconfigure();
  state->num_selected_codestreams = state->num_selected_layers = 0;
  if (codestream_idx >= 0)
    state->add_selected_codestream(codestream_idx);
  if (layer_idx >= 0)
    state->add_selected_layer(layer_idx);
  state->rendered_result = associate_with_rendered_result;
  if (state->whole_image = roi.is_empty())
    {
      state->roi_bounding_box = kdu_dims();
      state->num_roi_regions = 0;
      state->num_offered_types = 3;
      state->offered_types[0] = available_types + KDWS_LABEL_TEMPLATE;
      state->offered_types[1] = available_types + KDWS_XML_TEMPLATE;
      state->offered_types[2] = available_types + KDWS_IPR_TEMPLATE;
      state->selected_type = state->offered_types[0];
      state->set_label_string("<new label>");
    }
  else
    {
      state->roi_bounding_box = roi;
      state->num_roi_regions = 1;
      state->num_offered_types = 1;
      state->offered_types[0] = available_types + KDWS_CUSTOM_TEMPLATE;
      available_types[KDWS_CUSTOM_TEMPLATE].init(jp2_roi_description_4cc,
                                                 NULL,NULL);
      state->selected_type = state->offered_types[0];
      state->set_label_string(NULL);
    }
  state->set_external_file(NULL);
  state->roi_is_elliptical = false;
  state->roi_is_encoded = false;
  initial_state->copy(state);
  map_state_to_controls();
}

/*****************************************************************************/
/*                 kdws_metadata_editor::interrogate_user                    */
/*****************************************************************************/

int kdws_metadata_editor::interrogate_user(const char *text,
                                           const char *caption, UINT flags)
{
  kdws_string text_string(text);
  kdws_string caption_string(caption);
  return this->MessageBox(text_string,caption_string,flags);
}

/*****************************************************************************/
/*               kdws_metadata_editor::map_state_to_controls                 */
/*****************************************************************************/

void kdws_metadata_editor::map_state_to_controls()
{
  kdws_string string(80);
  int c, sel;

  mapping_state_to_controls = true;

  // Set data values
  sel = list_codestreams()->GetCurSel();
  list_codestreams()->ResetContent();
  for (c=0; c < state->num_selected_codestreams; c++)
    {
      sprintf(string,"Codestream %d",state->selected_codestreams[c]);
      list_codestreams()->AddString(string);
    }
  if ((sel != LB_ERR) && (sel < c))
    list_codestreams()->SetCurSel(sel);
  
  sel = list_layers()->GetCurSel();
  list_layers()->ResetContent();
  for (c=0; c < state->num_selected_layers; c++)
    {
      sprintf(string,"Layer %d",state->selected_layers[c]);
      list_layers()->AddString(string);
    }
  if ((sel != LB_ERR) && (sel < c))
    list_layers()->SetCurSel(sel);
  rendered_result_button()->SetCheck(
    (state->rendered_result)?BST_CHECKED:BST_UNCHECKED);

  roi_rectangular_button()->SetCheck(
    (state->roi_is_elliptical)?BST_UNCHECKED:BST_CHECKED);
  roi_elliptical_button()->SetCheck(
    (state->roi_is_elliptical)?BST_CHECKED:BST_UNCHECKED);
  roi_encoded_button()->SetCheck(
    (state->roi_is_encoded)?BST_CHECKED:BST_UNCHECKED);
  roi_multiregion_button()->SetCheck(
   (state->num_roi_regions>1)?BST_CHECKED:BST_UNCHECKED);
  whole_image_button()->SetCheck(
    (state->whole_image)?BST_CHECKED:BST_UNCHECKED);
  sprintf(string,"%d",state->roi_bounding_box.pos.x);
  x_pos_field()->SetWindowText(string);
  sprintf(string,"%d",state->roi_bounding_box.pos.y);
  y_pos_field()->SetWindowText(string);
  sprintf(string,"%d",state->roi_bounding_box.size.x);
  width_field()->SetWindowText(string);
  sprintf(string,"%d",state->roi_bounding_box.size.y);
  height_field()->SetWindowText(string);

  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
  if (state->is_link)
    {
      is_link_button()->SetCheck(BST_CHECKED);
      if (edit_state->cur_node.exists())
        edit_state->cur_node.get_link(link_type);
    }
  else
    is_link_button()->SetCheck(BST_UNCHECKED);
  if (link_type == JPX_GROUPING_LINK)
    is_link_button()->SetWindowText(_T("Link node (grouping)"));
  else if (link_type == JPX_ALTERNATE_CHILD_LINK)
    is_link_button()->SetWindowText(_T("Link node (alt child)"));
  else if (link_type == JPX_ALTERNATE_PARENT_LINK)
    is_link_button()->SetWindowText(_T("Link node (alt parent)"));
  else
    is_link_button()->SetWindowText(_T("Not a link"));

  int t;
  box_type_popup()->ResetContent();
  for (t=0; t < state->num_offered_types; t++)
    box_type_popup()->AddString(
      kdws_string(state->offered_types[t]->box_type_string));
  for (t=0; t < state->num_offered_types; t++)
    if (state->offered_types[t] == state->selected_type)
      box_type_popup()->SetCurSel(t);
  
  box_editor_popup()->ResetContent();
  box_editor_popup()->AddString(_T("Choose in Finder"));
  kdws_file_editor *editor;
  for (t=0; (editor=file_server->get_editor_for_doc_type(
                           state->selected_type->file_suffix,t)) != NULL; t++)
    box_editor_popup()->AddString(kdws_string(editor->app_name));
  if (t > 0)
    box_editor_popup()->SetCurSel(1);
  
  kdws_file *external_file = state->get_external_file();
  if (external_file != NULL)
    external_file_field()->SetWindowText(
      kdws_string(external_file->get_pathname()));
  else
    external_file_field()->SetWindowText(_T(""));
  temporary_file_button()->SetCheck(
   ((external_file!=NULL) &&
    external_file->get_temporary())?BST_CHECKED:BST_UNCHECKED);
  label_field()->SetWindowText(kdws_string(state->get_label_string()));

  bool is_roid = (state->selected_type->box_type == jp2_roi_description_4cc);

  // Determine which controls should be enabled
  add_stream_button()->EnableWindow(
    (state->can_edit_node && !state->roi_is_encoded)?TRUE:FALSE);
  remove_stream_button()->EnableWindow(
    (state->can_edit_node && (!state->roi_is_encoded) &&
     (state->num_selected_codestreams > 0))?TRUE:FALSE);
  codestream_stepper()->EnableWindow(
    (state->can_edit_node && !state->roi_is_encoded)?TRUE:FALSE);

  add_layer_button()->EnableWindow((state->can_edit_node)?TRUE:FALSE);
  remove_layer_button()->EnableWindow(
    (state->can_edit_node && (state->num_selected_layers > 0))?TRUE:FALSE);
  compositing_layer_stepper()->EnableWindow((state->can_edit_node)?TRUE:FALSE);

  rendered_result_button()->EnableWindow((state->can_edit_node)?TRUE:FALSE);

  bool roi_enable = state->can_edit_node &&
   !(state->whole_image || state->roi_is_encoded);
  roi_multiregion_button()->EnableWindow(
    ((roi_enable && (state->num_roi_regions>1))?TRUE:FALSE));
  roi_enable = roi_enable && (state->num_roi_regions <= 1);
  x_pos_field()->SetReadOnly((!roi_enable)?TRUE:FALSE);
  y_pos_field()->SetReadOnly((!roi_enable)?TRUE:FALSE);
  width_field()->SetReadOnly((!roi_enable)?TRUE:FALSE);
  height_field()->SetReadOnly((!roi_enable)?TRUE:FALSE);
  roi_rectangular_button()->EnableWindow((roi_enable)?TRUE:FALSE);
  roi_elliptical_button()->EnableWindow((roi_enable)?TRUE:FALSE);
  whole_image_button()->EnableWindow(
    (state->can_edit_node && (!state->roi_is_encoded) && is_roid)?TRUE:FALSE);
  roi_encoded_button()->EnableWindow(
    (state->can_edit_node && state->roi_is_encoded)?TRUE:FALSE);
  
  jp2_family_src *existing_src;
  bool can_save_to_file =
    ((state->get_external_file() != NULL) ||
     ((state->selected_type->box_type == jp2_label_4cc) &&
      (state->get_label_string() != NULL) &&
      !(state->is_link || state->is_cross_reference)) ||
     (edit_state->cur_node.exists() &&
      !edit_state->cur_node.get_existing(existing_src).is_null()));

  is_link_button()->EnableWindow(FALSE);
  box_type_popup()->EnableWindow(
    (state->can_edit_node && (state->num_offered_types > 1))?TRUE:FALSE);
  box_editor_popup()->EnableWindow(
    (state->can_edit_node && !is_roid)?TRUE:FALSE);
  save_to_file_button()->EnableWindow((can_save_to_file)?TRUE:FALSE);
  external_file_field()->SetReadOnly(TRUE);
  choose_file_button()->EnableWindow(
    (state->can_edit_node && !is_roid)?TRUE:FALSE);
  edit_file_button()->EnableWindow(
    (state->can_edit_node && !is_roid)?TRUE:FALSE);
  internalize_label_button()->EnableWindow(
    ((external_file != NULL) &&
     (state->selected_type->box_type==jp2_label_4cc))?TRUE:FALSE);

  copy_link_button()->EnableWindow(
   (edit_state->cur_node.exists() && allow_edits &&
    (state->is_link || !state->is_cross_reference))?TRUE:FALSE);
  paste_button()->EnableWindow(
    (allow_edits && renderer->catalog_source && !is_roid)?TRUE:FALSE);
  copy_descendants_button()->EnableWindow(
   (edit_state->cur_node.exists() && allow_edits &&
    (renderer->catalog_source != NULL))?TRUE:FALSE);

  label_field()->SetReadOnly(
    (state->can_edit_node &&
     (state->selected_type->box_type==jp2_label_4cc) &&
     (external_file == NULL))?FALSE:TRUE);

  apply_button()->EnableWindow((allow_edits)?TRUE:FALSE);
  delete_button()->EnableWindow((allow_edits)?TRUE:FALSE);
  if (state->is_link)
    delete_button()->SetWindowText(_T("Delete Link"));
  else
    delete_button()->SetWindowText(_T("Delete"));
  exit_button()->EnableWindow(TRUE);

  if (edit_state->cur_node == edit_state->root_node)
    {
      next_button()->EnableWindow(
       ((edit_state->edit_item != NULL) &&
        (edit_state->edit_item->next != NULL))?TRUE:FALSE);
      prev_button()->EnableWindow(
       ((edit_state->edit_item != NULL) &&
        (edit_state->edit_item->prev != NULL))?TRUE:FALSE);
      parent_button()->EnableWindow(FALSE);
    }
  else
    {
      next_button()->EnableWindow(
        (edit_state->next_sibling_idx>=0)?TRUE:FALSE);
      prev_button()->EnableWindow(
        (edit_state->prev_sibling_idx>=0)?TRUE:FALSE);
      parent_button()->EnableWindow(TRUE);
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
  descendants_button()->EnableWindow((have_non_numlist_children)?TRUE:FALSE);
  add_child_button()->EnableWindow((allow_edits)?TRUE:FALSE);
  add_parent_button()->EnableWindow((allow_edits && !is_roid)?TRUE:FALSE);
  metashow_button()->EnableWindow((edit_state->cur_node.exists() &&
                                   !state->is_link)?TRUE:FALSE);
  catalog_button()->EnableWindow(
    (edit_state->cur_node.exists() &&
     (initial_state->get_label_string() != NULL))?TRUE:FALSE);

  update_apply_buttons();

  mapping_state_to_controls = false;
}

/*****************************************************************************/
/*              kdws_metadata_editor::map_controls_to_state                  */
/*****************************************************************************/

void kdws_metadata_editor::map_controls_to_state()
{
  /* The following members have already been set in response to IBAction
   messages: `selected_codestreams', `selected_layers', `whole_image',
   `num_roi_regions', `roi_is_encoded', `external_file' and `selected_type'.
   Here we find the remaining member values by querying the current state
   of the dialog controls. */
  if (mapping_state_to_controls)
    return;

  state->rendered_result = (rendered_result_button()->GetCheck()==BST_CHECKED);
  get_nonneg_integer(x_pos_field(),state->roi_bounding_box.pos.x);
  get_nonneg_integer(y_pos_field(),state->roi_bounding_box.pos.y);
  get_nonneg_integer(width_field(),state->roi_bounding_box.size.x);
  get_nonneg_integer(height_field(),state->roi_bounding_box.size.y);

  if ((state->selected_type->box_type == jp2_label_4cc) &&
      (state->get_external_file() == NULL))
    {
      int label_length = label_field()->LineLength();
      kdws_string str(label_length+8);
      label_field()->GetLine(0,str,label_length);
      state->set_label_string(str);
    }
  else
    state->set_label_string(NULL);
}

/*****************************************************************************/
/*               kdws_metadata_editor::update_apply_buttons                  */
/*****************************************************************************/

void kdws_metadata_editor::update_apply_buttons()
{
  bool something_to_apply =
     !(edit_state->cur_node.exists() && state->compare(initial_state));
  apply_button()->EnableWindow((something_to_apply)?TRUE:FALSE);
  apply_and_exit_button()->EnableWindow((something_to_apply)?TRUE:FALSE);
  if (!edit_state->cur_node.exists())
    {
      apply_button()->SetWindowText(_T("Create w/ label"));
      apply_and_exit_button()->SetWindowText(_T("Create + Exit"));
    }
  else
    {
      apply_button()->SetWindowText(_T("Apply"));
      apply_and_exit_button()->SetWindowText(_T("Apply + Exit"));
    }
}

/*****************************************************************************/
/*                  MESSAGE MAP for kdws_metadata_editor                     */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_metadata_editor, CDialog)
  ON_WM_CTLCOLOR()
  ON_NOTIFY(TTN_GETDISPINFO, 0, get_tooltip_dispinfo)

	ON_NOTIFY(UDN_DELTAPOS, IDC_META_SPIN_STREAM, OnDeltaposMetaSpinStream)
	ON_NOTIFY(UDN_DELTAPOS, IDC_META_SPIN_LAYER, OnDeltaposMetaSpinLayer)
	ON_BN_CLICKED(IDC_META_REMOVE_STREAM, OnMetaRemoveStream)
	ON_BN_CLICKED(IDC_META_ADD_STREAM, OnMetaAddStream)
	ON_BN_CLICKED(IDC_META_REMOVE_LAYER, OnMetaRemoveLayer)
	ON_BN_CLICKED(IDC_META_ADD_LAYER, OnMetaAddLayer)
	ON_BN_CLICKED(IDC_META_MULTI_REGION, setSingleRegion)
	ON_BN_CLICKED(IDC_META_ENCODED, clearROIEncoded)
	ON_BN_CLICKED(IDC_META_RECTANGULAR, setROIRectangular)
	ON_BN_CLICKED(IDC_META_ELLIPTICAL, setROIElliptical)
	ON_BN_CLICKED(IDC_META_WHOLE_IMAGE, setWholeImage)

	ON_CBN_SELCHANGE(IDC_META_BOX_TYPE, changeBoxType)
	ON_BN_CLICKED(IDC_META_SAVE_TO_FILE, saveToFile)
	ON_BN_CLICKED(IDC_META_CHOOSE_FILE, chooseFile)
	ON_BN_CLICKED(IDC_META_EDIT_FILE, editFile)
	ON_CBN_SELCHANGE(IDC_META_BOX_EDITOR, changeEditor)
	ON_BN_CLICKED(IDC_META_INTERNALIZE_LABEL, internalizeLabel)

	ON_BN_CLICKED(IDC_META_COPY_LINK, clickedCopyLink)
	ON_BN_CLICKED(IDC_META_PASTE, clickedPaste)
	ON_BN_CLICKED(IDC_META_COPY_DESCENDANTS, clickedCopyDescendants)

	ON_BN_CLICKED(IDC_META_CANCEL, clickedExit)
	ON_BN_CLICKED(IDC_META_DELETE, clickedDelete)
	ON_BN_CLICKED(IDC_META_APPLY, clickedApply)
	ON_BN_CLICKED(IDC_META_APPLY_AND_EXIT, clickedApplyAndExit)
	ON_BN_CLICKED(IDC_META_NEXT, clickedNext)
	ON_BN_CLICKED(IDC_META_PREV, clickedPrev)
	ON_BN_CLICKED(IDC_META_PARENT, clickedParent)
	ON_BN_CLICKED(IDC_META_DESCENDANTS, clickedDescendants)
	ON_BN_CLICKED(IDC_META_ADD_CHILD, clickedAddChild)
	ON_BN_CLICKED(IDC_META_ADD_PARENT, clickedAddParent)

	ON_BN_CLICKED(IDC_META_METASHOW, findInMetashow)
	ON_BN_CLICKED(IDC_META_CATALOG, findInCatalog)

  ON_BN_CLICKED(IDC_META_REVERT, clickedRevert)
	ON_EN_CHANGE(IDC_META_XPOS, otherAction)
	ON_EN_CHANGE(IDC_META_WIDTH, otherAction)
	ON_EN_CHANGE(IDC_META_YPOS, otherAction)
	ON_EN_CHANGE(IDC_META_HEIGHT, otherAction)
	ON_EN_CHANGE(IDC_META_LABEL, otherAction)
  ON_BN_CLICKED(IDC_META_RENDERED, otherAction)
END_MESSAGE_MAP()

/*****************************************************************************/
/*                     kdws_metadata_editor::OnCtlColor                      */
/*****************************************************************************/

HBRUSH kdws_metadata_editor::OnCtlColor(CDC *dc, CWnd *wnd, UINT control_type)
{
  // Call the base implementation first, to get a default brush
  HBRUSH brush = CDialog::OnCtlColor(dc,wnd,control_type);
  int id = wnd->GetDlgCtrlID();
  if ((id == IDC_META_XPOS) || (id == IDC_META_YPOS) ||
      (id == IDC_META_WIDTH) || (id == IDC_META_HEIGHT) ||
      (id == IDC_META_LABEL) || (id == IDC_META_IS_LINK))
    {
      COLORREF link_status_text_colour = RGB(0,0,0);
      if ((state != NULL) && state->is_link)
        link_status_text_colour = RGB(0,0,255);
      dc->SetTextColor(link_status_text_colour);
    }
  return brush;
}

/*****************************************************************************/
/*                kdws_metadata_editor::get_tooltip_dispinfo                 */
/*****************************************************************************/

void kdws_metadata_editor::get_tooltip_dispinfo(NMHDR* pNMHDR,
                                                LRESULT *pResult)
{
  NMTTDISPINFO *pTTT = (NMTTDISPINFO *) pNMHDR;
  UINT_PTR nID =pNMHDR->idFrom;
  if (pTTT->uFlags & TTF_IDISHWND)
    {
        // idFrom is actually the HWND of the tool
        nID = ::GetDlgCtrlID((HWND)nID);
        if(nID)
          {
            int num_chars =
              ::LoadString(AfxGetResourceHandle(),
                           (UINT) nID,this->tooltip_buffer,
                           KD_METAEDIT_MAX_TOOLTIP_CHARS);
            if (num_chars > 0)
              {
                pTTT->lpszText = this->tooltip_buffer;
                pTTT->hinst = NULL;
              }
            ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,300);
          }
    }
}

/*****************************************************************************/
/*         kdws_metadata_editor:[ACTIONS for Association Editing]            */
/*****************************************************************************/

void
  kdws_metadata_editor::OnMetaRemoveStream() 
{
  if (state->roi_is_encoded || (state->num_selected_codestreams == 0) ||
      !state->can_edit_node)
    return;
  int n, idx = list_codestreams()->GetCurSel();
  if ((idx == LB_ERR) || (idx >= state->num_selected_codestreams))
    idx = state->num_selected_codestreams-1;
  map_controls_to_state();
  for (n=idx+1; n < state->num_selected_codestreams; n++)
    state->selected_codestreams[n-1] = state->selected_codestreams[n];
  state->num_selected_codestreams--;
  map_state_to_controls();
}

void
  kdws_metadata_editor::OnMetaAddStream() 
{
  if (state->roi_is_encoded || !state->can_edit_node)
    return;
  map_controls_to_state();
  int n, idx = state->num_selected_codestreams;
  if (idx >= num_codestreams)
    { // Find a duplicated codestream and select it
      for (idx=num_codestreams-1; idx > 0; idx--)
        {
          for (n=0; n < idx; n++)
            if (state->selected_codestreams[n] ==
                state->selected_codestreams[idx])
              break;
          if (n < idx)
            break;
        }
      if (idx <= 0)
        return;
    }
  else
    state->num_selected_codestreams++;
  state->selected_codestreams[idx] = 0;
  map_state_to_controls();
  list_codestreams()->SetCurSel(idx);
}

void
  kdws_metadata_editor::OnDeltaposMetaSpinStream(NMHDR* pNMHDR,
                                                 LRESULT* pResult) 
{
  if (state->roi_is_encoded || !state->can_edit_node)
    return;
  NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
  if (state->num_selected_codestreams == 0)
    {
      OnMetaAddStream();
      return;
    }
  int idx = list_codestreams()->GetCurSel();
  if ((idx == LB_ERR) || (idx >= state->num_selected_codestreams))
    idx = state->num_selected_codestreams-1;
  map_controls_to_state();
  state->selected_codestreams[idx] -= pNMUpDown->iDelta;
  if (state->selected_codestreams[idx] < 0)
    state->selected_codestreams[idx] = 0;
  else if (state->selected_codestreams[idx] >= num_codestreams)
    state->selected_codestreams[idx] = num_codestreams-1;
  map_state_to_controls();
  *pResult = 0;
}

void
  kdws_metadata_editor::OnMetaRemoveLayer() 
{
  if ((state->num_selected_layers == 0) || !state->can_edit_node)
    return;
  int n, idx = list_layers()->GetCurSel();
  if ((idx == LB_ERR) || (idx >= state->num_selected_layers))
    idx = state->num_selected_layers-1;
  map_controls_to_state();
  for (n=idx+1; n < state->num_selected_layers; n++)
    state->selected_layers[n-1] = state->selected_layers[n];
  state->num_selected_layers--;
  map_state_to_controls();
}

void
  kdws_metadata_editor::OnMetaAddLayer() 
{
  if (!state->can_edit_node)
    return;
  map_controls_to_state();
  int n, idx = state->num_selected_layers;
  if (idx >= num_layers)
    { // Find a duplicated layer and select it
      for (idx=num_layers-1; idx > 0; idx--)
        {
          for (n=0; n < idx; n++)
            if (state->selected_layers[n] == state->selected_layers[idx])
              break;
          if (n < idx)
            break;
        }
      if (idx <= 0)
        return;
    }
  else
    state->num_selected_layers++;
  state->selected_layers[idx] = 0;
  map_state_to_controls();
  list_layers()->SetCurSel(idx);
}

void
  kdws_metadata_editor::OnDeltaposMetaSpinLayer(NMHDR* pNMHDR,
                                                LRESULT* pResult) 
{
  if (!state->can_edit_node)
    return;
  NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
  if (state->num_selected_layers == 0)
    {
      OnMetaAddLayer();
      return;
    }
  int idx = list_layers()->GetCurSel();
  if ((idx == LB_ERR) || (idx >= state->num_selected_layers))
    idx = state->num_selected_layers-1;
  map_controls_to_state();
  state->selected_layers[idx] -= pNMUpDown->iDelta;
  if (state->selected_layers[idx] < 0)
    state->selected_layers[idx] = 0;
  else if (state->selected_layers[idx] >= num_layers)
    state->selected_layers[idx] = num_layers-1;
  map_state_to_controls();
  *pResult = 0;
}

void kdws_metadata_editor::setWholeImage() 
{
  if (state->roi_is_encoded || (!state->can_edit_node) ||
      (state->selected_type->box_type == jp2_roi_description_4cc))
    return;
  state->whole_image = (whole_image_button()->GetCheck() == BST_CHECKED);
  if (state->whole_image)
    state->num_roi_regions = 0;
  else if (state->num_roi_regions == 0)
    state->num_roi_regions = 1;
  map_controls_to_state();
  map_state_to_controls();
}

void kdws_metadata_editor::clearROIEncoded() 
{
  if (!(state->can_edit_node && state->roi_is_encoded))
    return;
  if (interrogate_user("The node you are editing is associated with a region "
                       "of interest that is explicitly encoded with a higher "
                       "priority in the codestream.  Removing the encoded "
                       "status will allow you to edit the region shape and "
                       "codestream associations but lose the connection "
                       "with codestream encoding properties; moreover if "
                       "this is a multi-region node, it will be converted "
                       "to a single region ROI description.  Click OK if you "
                       "still wish to proceed.",
                       "Kdu_show: Operation has side effects",
                       MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
    return;
  state->roi_is_encoded = false;
  state->num_roi_regions = 1;
  map_controls_to_state();
  map_state_to_controls();	
}

void kdws_metadata_editor::setSingleRegion() 
{
  if (state->whole_image || state->roi_is_encoded || !state->can_edit_node)
    return;
  if (interrogate_user("The node you are editing is associated with a "
                       "multi-region ROI description.  Removing the "
                       "multi-region status will replace the description "
                       "with a single region, initially based on the "
                       "original description's bounding box.  This allows "
                       "you to explicitly edit the region and shape.  Click "
                       "OK if you still wish to proceed.",
                       "Kdu_show: Operation has side effects",
                       MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
    return;
  state->num_roi_regions = 1;
  map_controls_to_state();
  map_state_to_controls();
}

void kdws_metadata_editor::setROIRectangular() 
{
  if (state->whole_image || state->roi_is_encoded || !state->can_edit_node)
    return;
  state->roi_is_elliptical = false;
  update_apply_buttons();
}

void kdws_metadata_editor::setROIElliptical() 
{
  if (state->whole_image || state->roi_is_encoded || !state->can_edit_node)
    return;
  state->roi_is_elliptical = true;
  update_apply_buttons();
}

/*****************************************************************************/
/*           kdws_metadata_editor:[ACTIONS for Content Editing]              */
/*****************************************************************************/

void kdws_metadata_editor::changeBoxType() 
{
  int idx = box_type_popup()->GetCurSel();
  if ((idx < 0) || (idx >= state->num_offered_types) ||
      (state->selected_type == state->offered_types[idx]) ||
      !state->can_edit_node)
    return;
  map_controls_to_state();
  state->selected_type = state->offered_types[idx];
  state->set_external_file(NULL);
  if (state->selected_type->box_type == jp2_label_4cc)
    state->set_label_string("<new label>");
  else
    {
      state->set_label_string(NULL);
      kdws_file *file =
        file_server->retain_tmp_file(state->selected_type->file_suffix);
      file->create_if_necessary(state->selected_type->initializer);
      state->set_external_file(file,true);
    }
  state->set_external_file_replaces_contents();
      // Does nothing if no external file
  map_state_to_controls();
}

void kdws_metadata_editor::saveToFile() 
{
  kdws_file *existing_file = state->get_external_file();
  char filename[MAX_PATH+1];
  if (!choose_save_file_pathname(state->selected_type->file_suffix,
                                 existing_file,filename,MAX_PATH))
    return;
  kdws_file *new_file = file_server->retain_known_file(filename);
  map_controls_to_state();
  if (existing_file != NULL)
    copy_file(existing_file,new_file);
  state->set_external_file(new_file);
  if ((existing_file == NULL) && !state->save_to_external_file(edit_state))
    new_file->create_if_necessary(state->selected_type->initializer);
  map_state_to_controls();
}

void kdws_metadata_editor::chooseFile() 
{
  if (!state->can_edit_node)
    return;
  map_controls_to_state();
  char filename[MAX_PATH+1];
  if (choose_open_file_pathname(state->selected_type->file_suffix,
                                state->get_external_file(),
                                filename,MAX_PATH))
    {
      kdws_file *file = file_server->retain_known_file(filename);
      file->create_if_necessary(state->selected_type->initializer);
      state->set_external_file(file,true);
      state->set_external_file_replaces_contents();
      state->set_label_string(NULL);
    }
  map_state_to_controls();
}

void kdws_metadata_editor::editFile() 
{
  if ((!state->can_edit_node) || (state->selected_type == NULL))
    return;
  map_controls_to_state();
  kdws_file *external_file = state->get_external_file();
  if (external_file == NULL)
    {
      state->save_to_external_file(edit_state);
      external_file = state->get_external_file();
    }
  if (external_file == NULL)
    { // Should not happen
      MessageBeep(MB_ICONEXCLAMATION); return;
    }
  external_file->create_if_necessary(state->selected_type->initializer);
  
  const char *file_suffix = state->selected_type->file_suffix;
  kdws_file_editor *editor=NULL;
  int idx = box_editor_popup()->GetCurSel();
  if (idx > 0)
    editor = file_server->get_editor_for_doc_type(file_suffix,idx-1);
  if (editor != NULL) // Put it at the head of the list of known editors
    file_server->add_editor_for_doc_type(file_suffix,editor->app_pathname);
  else
    editor = choose_editor(file_server,file_suffix);
  if (editor == NULL)
    MessageBeep(MB_ICONEXCLAMATION);
  else
    {
      kdws_string command_line((int)(strlen(editor->app_pathname) + 6 +
                                     strlen(external_file->get_pathname())));
      if (editor->app_pathname[0] == '\"')
        sprintf(command_line,"%s \"%s\"",editor->app_pathname,
                external_file->get_pathname());
      else
        sprintf(command_line,"\"%s\" \"%s\"",editor->app_pathname,
                external_file->get_pathname());
      STARTUPINFO startup_info;
      memset(&startup_info,0,sizeof(startup_info));
      startup_info.cb = sizeof(startup_info);
      PROCESS_INFORMATION process_info;
      memset(&process_info,0,sizeof(process_info));
      if (CreateProcess(NULL,command_line,NULL,NULL,FALSE,
                        NORMAL_PRIORITY_CLASS,NULL,NULL,&startup_info,
                        &process_info))
        state->set_external_file_replaces_contents();
      else
        MessageBeep(MB_ICONEXCLAMATION);
    }
  map_state_to_controls();
}

void kdws_metadata_editor::changeEditor() 
{
  if ((!state->can_edit_node) || (state->selected_type == NULL))
    return;
  map_controls_to_state();
  kdws_file_editor *editor = NULL;
  const char *file_suffix = state->selected_type->file_suffix;
  int idx = box_editor_popup()->GetCurSel();
  if (idx > 0)
    editor = file_server->get_editor_for_doc_type(file_suffix,idx-1);
  if (editor == NULL)
    editor = choose_editor(file_server,file_suffix);
  if (editor != NULL)
    file_server->add_editor_for_doc_type(file_suffix,editor->app_pathname);
  map_state_to_controls();
}

void kdws_metadata_editor::internalizeLabel()
{
  map_controls_to_state();
  if (!state->internalize_label_node())
    {
      MessageBeep(MB_ICONEXCLAMATION);
      return;
    }
  map_state_to_controls();
}

/*****************************************************************************/
/*       kdws_metadata_editor:[ACTIONS which work with the Pastebar]         */
/*****************************************************************************/

void kdws_metadata_editor::clickedCopyLink()
{
  if ((!edit_state->cur_node.exists()) || (!state->can_edit_node) ||
      (state->is_cross_reference && !state->is_link))
    return;
  jpx_metanode link_target;
  jpx_metanode_link_type link_type;
  if (state->is_link)
    link_target = edit_state->cur_node.get_link(link_type);
  else
    link_target = edit_state->cur_node;
  if (renderer->catalog_source == NULL)
    renderer->window->create_metadata_catalog();
  if (link_target.exists() && (renderer->catalog_source != NULL))
    renderer->catalog_source->paste_link(link_target);
}

void kdws_metadata_editor::clickedPaste()
{
  if ((renderer->catalog_source == NULL) ||
      (state->selected_type->box_type == jp2_roi_description_4cc) ||
      !allow_edits)
    return;
  const char *pasted_label=renderer->catalog_source->get_pasted_label();
  jpx_metanode pasted_link=renderer->catalog_source->get_pasted_link();
  jpx_metanode node_to_cut=renderer->catalog_source->get_pasted_node_to_cut();
  if ((pasted_label == NULL) && (!pasted_link) && (!node_to_cut))
    {
      interrogate_user("You need to copy/link/cut something to the catalog "
                       "paste bar before you can paste over the current node "
                       "in the editor.",
                       "Kdu_show: Put something in the paste bar",
                       MB_OK | MB_ICONERROR);
      return;
    }
  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
  if (pasted_link.exists())
    {
      int response =
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
                         "node within which the link is found.\n"
                         "  Click \"YES\" for an Alternate Child link, \"NO\" "
                         "for the other choices.\n",
                         "Kdu_show: Alternate Child link?",
                         MB_YESNOCANCEL | MB_ICONQUESTION);
      if (response == IDCANCEL)
        return;
      if (response == IDYES)
        link_type = JPX_ALTERNATE_CHILD_LINK;
      else if ((response =
                interrogate_user("Would you like an Alternate Parent link?  "
                                 "If you click \"NO\", a Grouping link will "
                                 "be created.",
                                 "Kdu_show: Alternate Parent link?",
                                 MB_YESNOCANCEL | MB_ICONQUESTION)) ==
                                 IDCANCEL)
        return;
      else if (response == IDYES)
        link_type = JPX_ALTERNATE_PARENT_LINK;
      else
        link_type = JPX_GROUPING_LINK;
  }

  map_controls_to_state();
  if (!edit_state->cur_node)
    { // Save the node first, so we can restrict our attention to replacing it
      try {
        if (!state->save_metanode(edit_state,initial_state,renderer))
          {
            MessageBeep(MB_ICONEXCLAMATION);
            update_apply_buttons();
            return;
          }
      }
      catch (int)
      {
        preconfigure();
        return;
      }
    }

  kdws_file *file=get_file_retained_by_node(edit_state->cur_node,file_server);
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
  catch (int)
  {
    renderer->metadata_changed(true);
    map_state_to_controls();
    return;
  }
  renderer->metadata_changed(true);
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  map_state_to_controls();
}

void kdws_metadata_editor::clickedCopyDescendants()
{
  jpx_metanode pasted_link = renderer->catalog_source->get_pasted_link();
  if (!pasted_link)
    {
     interrogate_user("You need to copy a link to the catalog pastebar "
                      "before you can copy the present node's descendants "
                      "across as descendants of the target of that link.",
                      "Kdu_show: Put something in the paste bar",
                      MB_OK | MB_ICONERROR);
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
                     "Kdu_show: Copy everything?",
                     MB_YESNOCANCEL | MB_ICONQUESTION);
  if (user_choice == IDCANCEL)
    return;
  bool copy_linkers = (user_choice == IDYES);
  jpx_metanode root_metanode = edit_state->meta_manager.access_root();
  edit_state->meta_manager.reset_copy_locators(root_metanode,true);
  copy_descendants_with_file_retain(edit_state->cur_node,pasted_link,
                                    file_server,copy_linkers);
  edit_state->meta_manager.reset_copy_locators(root_metanode,true,true);
  renderer->metadata_changed(true);
}


/*****************************************************************************/
/*        kdws_metadata_editor:[ACTIONS for Navigation and Control]          */
/*****************************************************************************/

void kdws_metadata_editor::clickedExit() 
{
  map_controls_to_state();
  if (!edit_state->cur_node.exists())
    {
      if (interrogate_user("Are you sure you want to exit without adding "
                           "any metadata?  To do so, click the \"Apply\" "
                           "button before exiting.",
                           "Kdu_show: Exit anyway?",
                           MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
        {
          update_apply_buttons();
          return;
        }
    }
  else if (!(state->compare(initial_state) ||
             (interrogate_user("You have made some changes; are you sure you "
                               "want to exit without applying these changes "
                               "by clicking \"Apply\"?",
                               "Kdu_show: Exit anyway?",
                               MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }
  close();
}

void kdws_metadata_editor::clickedDelete() 
{
  if (edit_state->cur_node.exists())
    { // Otherwise we haven't added anything yet, so there's nothing to delete
      if (interrogate_user("Are you sure you want to delete this node and "
                           "all its descendants?",
                           "Kdu_show: Really delete?",
                           MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
        {
          update_apply_buttons();
          return;
        }
      try {
        edit_state->delete_cur_node(renderer);
      }
      catch (int) {
        preconfigure();
        return;
      }
    }
  if (!edit_state->cur_node.exists())
    close();
  else
    {
      state->configure_with_metanode(edit_state->cur_node);
      initial_state->copy(state);
      map_state_to_controls();      
    }
}

void kdws_metadata_editor::clickedApply() 
{
  if ((!state->roi_bounding_box.is_empty()) &&
      !(check_integer_in_range(this,x_pos_field(),0,INT_MAX) &&
        check_integer_in_range(this,y_pos_field(),0,INT_MAX) &&
        check_integer_in_range(this,width_field(),0,INT_MAX) &&
        check_integer_in_range(this,height_field(),0,INT_MAX)))
    return;
  map_controls_to_state();
  try {
    bool create_label = !edit_state->cur_node.exists();
    if (!state->save_metanode(edit_state,initial_state,renderer))
      {
        update_apply_buttons();
        return;
      }
    if (create_label)
      {
        edit_state->add_child_node(renderer);
        state->configure_with_metanode(edit_state->cur_node);
      }
  }
  catch (int) {
    preconfigure();
    return;
  }
  initial_state->copy(state); // Now everthing starts again from scratch
  map_state_to_controls(); // May alter the navigation options
}

void kdws_metadata_editor::clickedApplyAndExit() 
{
  if ((!state->roi_bounding_box.is_empty()) &&
      !(check_integer_in_range(this,x_pos_field(),0,INT_MAX) &&
        check_integer_in_range(this,y_pos_field(),0,INT_MAX) &&
        check_integer_in_range(this,width_field(),0,INT_MAX) &&
        check_integer_in_range(this,height_field(),0,INT_MAX)))
    return;
  map_controls_to_state();
  try {
    if (!state->save_metanode(edit_state,initial_state,renderer))
      {
        update_apply_buttons();
        return;
      }
  }
  catch (int) { }
  close();
}

void kdws_metadata_editor::clickedNext() 
{
  map_controls_to_state();
  if (!(state->compare(initial_state) ||
        (interrogate_user("Changes you have made will be lost unless you "
                          "click \"Apply\" before navigating to another node "
                          "in the metadata tree.",
                          "Kdu_show: About to lose changes",
                          MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }
  edit_state->move_to_next();
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  map_state_to_controls();
}

void kdws_metadata_editor::clickedPrev() 
{
  map_controls_to_state();
  if (!(state->compare(initial_state) ||
        (interrogate_user("Changes you have made will be lost unless you "
                          "click \"Apply\" before navigating to another node "
                          "in the metadata tree.",
                          "Kdu_show: About to lose changes",
                          MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }
  edit_state->move_to_prev();
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  map_state_to_controls();  
}

void kdws_metadata_editor::clickedParent() 
{
  map_controls_to_state();
  if (!(state->compare(initial_state) ||
        (interrogate_user("Changes you have made will be lost unless you "
                          "click \"Apply\" before navigating to another node "
                          "in the metadata tree.",
                          "Kdu_show: About to lose changes",
                          MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }
  edit_state->move_to_parent();
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  map_state_to_controls();  
}

void kdws_metadata_editor::clickedDescendants() 
{
  map_controls_to_state();
  if (!(state->compare(initial_state) ||
        (interrogate_user("Changes you have made will be lost unless you "
                          "click \"Apply\" before navigating to another node "
                          "in the metadata tree.",
                          "Kdu_show: About to lose changes",
                          MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }
  edit_state->move_to_descendants();
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  map_state_to_controls();  
}

void kdws_metadata_editor::clickedAddChild() 
{
  map_controls_to_state();
  if (!edit_state->cur_node.exists())
    {
      interrogate_user("You need to click \"Apply\" to add the primary node "
                       "to the metadata tree before adding a child.",
                       "Kdu_show: Apply changes first",
                       MB_OK | MB_ICONERROR);
      {
        update_apply_buttons();
        return;
      }
    }
  if (!(state->compare(initial_state) ||
        (interrogate_user("Changes you have made will be lost unless you "
                          "click \"Apply\" before navigating to another node "
                          "in the metadata tree.",
                          "Kdu_show: About to lose changes",
                          MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }
  edit_state->add_child_node(renderer);
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  map_state_to_controls();  
}

void kdws_metadata_editor::clickedAddParent()
{
  if ((state->selected_type->box_type == jp2_roi_description_4cc) ||
      !allow_edits)
    return;
  map_controls_to_state();
  if (!edit_state->cur_node.exists())
    {
      interrogate_user("You need to click \"Apply\" to add the primary node "
                       "to the metadata tree before inserting a parent.",
                       "Kdu_show: Apply changes first",
                       MB_OK | MB_ICONERROR);
      {
        update_apply_buttons();
        return;
      }
    }
  if (!(state->compare(initial_state) ||
        (interrogate_user("Changes you have made will be lost unless you "
                          "click \"Apply\" before inserting a new parent "
                          "into the metadata tree.",
                          "Kdu_show: About to lose changes",
                          MB_OKCANCEL | MB_ICONWARNING) == IDOK)))
    {
      update_apply_buttons();
      return;
    }
  edit_state->add_parent_node(renderer);
  state->configure_with_metanode(edit_state->cur_node);
  initial_state->copy(state);
  map_state_to_controls();
}

void kdws_metadata_editor::findInMetashow() 
{
  if (!edit_state->cur_node)
    return;
  if (renderer->metashow == NULL)
    renderer->menu_MetadataShow();
  if (renderer->metashow != NULL)
    renderer->metashow->select_matching_metanode(edit_state->cur_node); 
}

void kdws_metadata_editor::findInCatalog()
{
  if (!edit_state->cur_node)
    return;
  if (renderer->catalog_source == NULL)
    renderer->window->create_metadata_catalog();
  jpx_metanode node=edit_state->cur_node;
  if (renderer->catalog_source)
    renderer->catalog_source->select_matching_metanode(node);
}

/*****************************************************************************/
/*                 kdws_metadata_editor:[Generic ACTIONS]                    */
/*****************************************************************************/

void kdws_metadata_editor::clickedRevert() 
{
  state->copy(initial_state);
  map_state_to_controls();
}

void kdws_metadata_editor::otherAction() 
{
  map_controls_to_state();
  update_apply_buttons();
}
