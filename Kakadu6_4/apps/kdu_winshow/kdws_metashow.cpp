/*****************************************************************************/
// File: kdws_metashow.cpp [scope = APPS/WINSHOW]
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
#include "kdws_metashow.h"
#include "kdws_manager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/* ========================================================================= */
/*                          INTERNAL FUNCTIONS                               */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                   make_printable_char                              */
/*****************************************************************************/

static inline char
  make_printable_char(char ch)
{
  if ((ch < 0x20) || (ch > 0x7F))
    ch = 0x7F;
  return ch;
}

/*****************************************************************************/
/* STATIC                  find_matching_metanode                            */
/*****************************************************************************/

static kdws_metanode *
  find_matching_metanode(kdws_metanode *container, jpx_metanode jpx_node)
  /* Recursively searches through the metadata tree to see if we can find
   a node which references the supplied `jpx_node' object.  In the case of
   multiple matches, the deepest match in the tree will be found, which is
   generally the most useful behaviour. */
{
  kdws_metanode *scan, *result;
  for (scan=container->children; scan != NULL; scan=scan->next)
    if ((result = find_matching_metanode(scan,jpx_node)) != NULL)
      return result;
  return (container->jpx_node == jpx_node)?container:NULL;
}


/* ========================================================================= */
/*                             kdws_metanode                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                     kdws_metanode::kdws_metanode                          */
/*****************************************************************************/

kdws_metanode::kdws_metanode(kdws_metashow *owner, kdws_metanode *parent)
{
  this->owner = owner;
  this->parent = parent;
  this->is_deleted = false;
  this->is_changed = false;
  this->ancestor_changed = false;
  box_type = 0;
  box_header_length = 0;
  codestream_idx = compositing_layer_idx = -1;
  child_list_complete = contents_complete = false;
  children = last_child = last_complete_child = next = NULL;
  name[0] = '\0';
  handle = NULL;
  can_display = false;
}

/*****************************************************************************/
/*                       kdws_metanode::~kdws_metanode                       */
/*****************************************************************************/

kdws_metanode::~kdws_metanode()
{
  while ((last_child=children) != NULL)
    {
      children = last_child->next;
      delete last_child;
    }
}

/*****************************************************************************/
/*                      kdws_metanode::update_display                        */
/*****************************************************************************/

void
  kdws_metanode::update_display()
{
  assert(can_display && (owner->display==this) && (owner->display_box_pos>=0));
  CEdit *edit = owner->get_edit_ctrl();
  CTreeCtrl *ctrl = owner->get_tree_ctrl();

  jp2_input_box box;
  box.open(owner->src,locator);
  bool box_is_complete = box.exists() && box.is_complete();
  if (!box_is_complete)
    {
      contents_complete = false;
      owner->get_tree_ctrl()->SetItemState(handle,0,TVIS_BOLD);
    }
  if (!box.exists())
    {
      edit->SetWindowText(_T("..."));
      return;
    }
  if (box_is_complete && !contents_complete)
    {
      contents_complete = true;
      owner->get_tree_ctrl()->SetItemState(handle,TVIS_BOLD,TVIS_BOLD);
    }

  int buf_len = owner->display_buf_len;
  char *tp, *buf = *(owner->display_buf);
  if (box_type == jp2_file_type_4cc)
    {
      if (!contents_complete)
        {
          edit->SetWindowText(_T("..."));
          return;
        }

      kdu_uint32 brand=0, minor_version=0;
      box.read(brand);
      box.read(minor_version);
      sprintf(buf,"Brand = \"");
      tp = buf + strlen(buf);
      *(tp++) = make_printable_char((char)(brand>>24));
      *(tp++) = make_printable_char((char)(brand>>16));
      *(tp++) = make_printable_char((char)(brand>>8));
      *(tp++) = make_printable_char((char)(brand>>0));
      sprintf(tp,"\"\r\nMinor version = %d\r\nCompatibility list:\r\n    ",
              minor_version);
      tp += strlen(tp);
      bool first_in_list = true;
      while (box.read(brand) && ((tp-buf) < (buf_len-10)))
        {
          if (first_in_list)
            first_in_list = false;
          else
            { *(tp++) = ','; *(tp++) = ' '; }
          *(tp++) = '\"';
          *(tp++) = make_printable_char((char)(brand>>24));
          *(tp++) = make_printable_char((char)(brand>>16));
          *(tp++) = make_printable_char((char)(brand>>8));
          *(tp++) = make_printable_char((char)(brand>>0));
          *(tp++) = '\"';
          *tp = '\0';
        }
      owner->display_box_pos = -1; // Indicates that the display is complete
      edit->SetWindowText(*(owner->display_buf));
    }
  else if ((box_type == jp2_label_4cc) ||
           (box_type == jp2_xml_4cc) || (box_type == jp2_iprights_4cc))
    { // Displaying plain text directly
      box.seek(owner->display_box_pos);
      int num_bytes;
      char text[256];
      char *bp = buf +  owner->display_buf_pos;
      while ((num_bytes = box.read((kdu_byte *) text,256)) > 0)
        {
          owner->display_box_pos += num_bytes;
          for (tp=text; num_bytes > 0; tp++, num_bytes--)
            {
              if ((*tp == '\0') ||
                  ((*tp == '\n') && ((bp==buf) || (bp[-1] != '\r'))))
                { *(bp++) = '\r'; *(bp++) = '\n'; }
              else if ((bp != buf) && (bp[-1] == '\r') && (*tp != '\n'))
                { *(bp++) = '\n'; *(bp++) = *tp; }
              else
                *(bp++) = *tp;
            }
          owner->display_buf_pos = (int)(bp - buf);
          if ((owner->display_buf_len - owner->display_buf_pos) <= 520)
            {
              owner->display_box_pos = -1; // So we don't try to add more text
              strcpy(bp,"......\r\n\r\n<truncated to save memory>");
              bp += strlen(bp);
              break;
            }
        }
      *bp = '\0';
      if (contents_complete)
        owner->display_box_pos = -1; // Indicates that display is complete
      else if (owner->display_box_pos >= 0)
        strcpy(bp,"......");
      edit->SetWindowText(*(owner->display_buf));
    }
  else
    { // Print hex dump of box contents
      box.seek(owner->display_box_pos);
      int n, num_bytes;
      kdu_byte *dp, data[8];
      char val, *bp = buf +  owner->display_buf_pos;
      if (owner->display_buf_pos == 0)
        {
          strcpy(buf,
                 "   Hex Dump of Binary Contents:\r\n"
                 "----------------------------------\r\n");
          bp += strlen(bp);
          owner->display_buf_pos = (int)(bp - buf);
        }
      while ((num_bytes = box.read(data,8)) > 0)
        {
          if ((num_bytes < 8) && !contents_complete)
            break; // Wait until we have at least one line of data to write
          owner->display_box_pos += num_bytes;
          for (dp=data, n=0; n < num_bytes; n++, dp++)
            {
              val = (char)((*dp) >> 4);
              *(bp++) = (val < 10)?('0'+val):('A'+val-10);
              val = (char)((*dp) & 0x0F);
              *(bp++) = (val < 10)?('0'+val):('A'+val-10);
              *(bp++) = ' ';
            }
          for (; n < 8; n++)
            { *(bp++) = '-'; *(bp++) = '-'; *(bp++) = ' '; }
          *(bp++) = '|'; *(bp++) = ' ';
          for (tp=(char *) data, n=0; n < num_bytes; n++, tp++)
            *(bp++) = make_printable_char(*tp);
          *(bp++) = '\r';
          *(bp++) = '\n';
          owner->display_buf_pos = (int)(bp - buf);
          if ((owner->display_buf_len - owner->display_buf_pos) <= 520)
            {
              owner->display_box_pos = -1; // So we don't try to add more text
              strcpy(bp,"......\r\n\r\n<truncated to save memory>");
              bp += strlen(bp);
              break;
            }
        }
      *bp = '\0';
      if (contents_complete)
        owner->display_box_pos = -1; // Indicates that display is complete
      else if (owner->display_box_pos >= 0)
        strcpy(bp,"......");
      edit->SetWindowText(*(owner->display_buf));
    }
}

/*****************************************************************************/
/*                    kdws_metanode::update_structure                        */
/*****************************************************************************/

bool
  kdws_metanode::update_structure(jp2_input_box *this_box,
                                bool container_complete,
                                jpx_meta_manager meta_manager)
{
  bool editing_state_changed = false;
  bool jpx_node_just_found = false;
  if (meta_manager.exists())
    {
      if (!jpx_node.exists())
        {
          jpx_node =
            meta_manager.locate_node(locator.get_file_pos()+box_header_length);
          if (jpx_node.exists())
            jpx_node_just_found = true;
        }
      if (jpx_node.exists() && (!is_changed) && jpx_node.is_changed())
        is_changed = editing_state_changed = true;
      if (jpx_node.exists() && (!ancestor_changed) &&
          jpx_node.ancestor_changed())
        ancestor_changed = editing_state_changed = true;
      if (jpx_node.exists() && (!is_deleted) && jpx_node.is_deleted())
        is_deleted = editing_state_changed = true;
    }

  kdws_metanode *child;
  bool added_something = false;

  if (!child_list_complete)
    { // Add onto the child list, if we can
      jp2_input_box box;
      if (this_box == NULL)
        {
          if (last_child == NULL)
            box.open(owner->src);
          else
            {
              box.open(owner->src,last_child->locator);
              if (box.exists())
                {
                  box.close();
                  box.open_next();
                }
            }
        }
      else
        { // Open sub-boxes in sequence, until we get to the end
          box.open(this_box);
          for (child=children; (child!=NULL) && box.exists(); child=child->next)
            {
              box.close();
              box.open_next();
            }
        }
      while (box.exists())
        { // Have a new child
          added_something = true;
          if (box.get_remaining_bytes() < 0)
            container_complete = true; // Rubber length box must be last
          child = new kdws_metanode(owner,this);
          if (last_child == NULL)
            children = last_child = child;
          else
            last_child = last_child->next = child;
          child->box_type = box.get_box_type();
          child->locator = box.get_locator();
          child->box_header_length = box.get_box_header_length();
          child->child_list_complete = !jp2_is_superbox(child->box_type);
          child->insert_into_view();
          box.close();
          box.open_next();
        }

      if (added_something)
        owner->get_tree_ctrl()->Expand(handle,TVE_EXPAND);
    }

  if (container_complete && !contents_complete)
    { // Change the appearance of the tree node
      child_list_complete = contents_complete = true;
      owner->get_tree_ctrl()->SetItemState(handle,TVIS_BOLD,TVIS_BOLD);
      CEdit *edit = owner->get_tree_ctrl()->GetEditControl();
    }

  // Now scan through any incomplete children, trying to complete them
  if (last_complete_child != NULL)
    child = last_complete_child->next;
  else
    child = children;
  for (; child != NULL; child=child->next)
    {
      jp2_input_box box;
      box.open(owner->src,child->locator);
      if (child->update_structure(&box,box.is_complete(),meta_manager))
        editing_state_changed = true;
      box.close();
    }
  return editing_state_changed;
}

/*****************************************************************************/
/*                     kdws_metanode::insert_into_view                       */
/*****************************************************************************/

void
  kdws_metanode::insert_into_view()
{
  if (box_type == 0)
    strcpy(name,"<root>");
  else
    {
      name[0] = (char)(box_type>>24);
      name[1] = (char)(box_type>>16);
      name[2] = (char)(box_type>>8);
      name[3] = (char)(box_type>>0);
      name[4] = '\0';
    }

  if ((box_type == jp2_codestream_4cc) && (parent != NULL) &&
      (parent->parent == NULL))
    codestream_idx = owner->num_codestreams++;
  else if (box_type == jp2_codestream_header_4cc)
    codestream_idx = owner->num_jpch++;
  else if (box_type == jp2_compositing_layer_hdr_4cc)
    compositing_layer_idx = owner->num_jplh++;

  char *cp = name + strlen(name);

  if (box_type == jp2_file_type_4cc)
    {
      can_display = true;
      strcpy(cp," [expand]");
    }
  else if ((box_type == jp2_file_type_4cc) || (box_type == jp2_label_4cc) ||
      (box_type == jp2_xml_4cc) || (box_type == jp2_iprights_4cc))
    {
      can_display = true;
      strcpy(cp," [show text]");
    }
  else if ((!jp2_is_superbox(box_type)) && (box_type != jp2_codestream_4cc) &&
           (parent != NULL))
    {
      can_display = true;
      strcpy(cp," [show hex]");
    }
  else if (codestream_idx >= 0)
    sprintf(cp," [show stream %d]",codestream_idx+1);
  else if (compositing_layer_idx >= 0)
    sprintf(cp," [show layer %d]",compositing_layer_idx+1);
  else if (box_type == jp2_composition_4cc)
    strcpy(cp," [show composition]");

  CTreeCtrl *ctrl = owner->get_tree_ctrl();
  handle = ctrl->InsertItem(kdws_string(name),
                            (parent==NULL)?TVI_ROOT:(parent->handle));
  ctrl->SetItemData(handle,(DWORD_PTR) this);
}


/* ========================================================================= */
/*                             kdws_metashow                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdws_metashow::kdws_metashow                         */
/*****************************************************************************/

kdws_metashow::kdws_metashow(kdws_renderer *renderer,
                             POINT preferred_location,
                             int window_identifier, CWnd* pParent)
	: CDialog()
{
  started = false; // Avoid executing code in message handlers for a while
  this->renderer = renderer;
  src = NULL;
  tree = display = NULL;
  this->window_identifier = window_identifier;
  this->filename = NULL;

  // Create the window
  Create(kdws_metashow::IDD, pParent);
  get_tree_ctrl()->SetBkColor(0x00A0FFFF);
  ShowWindow(SW_SHOW);

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

  display_buf_len = get_edit_ctrl()->GetLimitText();
  if (display_buf_len < 1024)
    display_buf_len = 1024;
  display_buf = new kdws_string(display_buf_len);
  display_buf_pos = display_box_pos = 0;
  display = NULL;

  num_codestreams = num_jplh = num_jpch = 0;

  RECT rect;
  GetClientRect(&rect);
  dialog_dims.x = rect.right - rect.left;
  dialog_dims.y = rect.bottom - rect.top;
  get_tree_ctrl()->GetWindowRect(&rect);
  tree_dims.x = rect.right - rect.left;
  tree_dims.y = rect.bottom - rect.top;
  get_edit_ctrl()->GetWindowRect(&rect);
  edit_dims.x = rect.right - rect.left;
  edit_dims.y = rect.bottom - rect.top;
  started = true; // Enable code in message handlers

  changed_text_colour = RGB(0,255,0);
  deleted_text_colour = RGB(255,0,0);
  LOGFONT font_attributes;
  get_tree_ctrl()->GetFont()->GetLogFont(&font_attributes);
  font_attributes.lfStrikeOut = TRUE;
  deleted_text_font.CreateFontIndirect(&font_attributes);

  kdws_string title(40);
  sprintf(title,"Metashow %d",window_identifier);
  SetWindowText(title);
}

/*****************************************************************************/
/*                      kdws_metashow::~kdws_metashow                        */
/*****************************************************************************/

kdws_metashow::~kdws_metashow()
{
  if (tree != NULL)
    delete tree;
  if (display_buf != NULL)
    delete display_buf;
  deleted_text_font.DeleteObject();
}

/*****************************************************************************/
/*                         kdws_metashow::activate                           */
/*****************************************************************************/

void
  kdws_metashow::activate(jp2_family_src *src,
                          const char *filename)
{
  deactivate();
  this->src = src;
  tree = new kdws_metanode(this,NULL);
  tree->insert_into_view();
  update_metadata();
  this->filename = filename;
  kdws_string title(40 + ((int) strlen(filename)));
  sprintf(title,"Metashow %d: %s",window_identifier,filename);
  SetWindowText(title);
}

/*****************************************************************************/
/*                     kdws_metashow::update_metadata                        */
/*****************************************************************************/

void
  kdws_metashow::update_metadata()
{
  if ((src == NULL) || (tree == NULL))
    return;
  jpx_meta_manager meta_manager = renderer->get_meta_manager();
  if (meta_manager.exists())
    meta_manager.load_matches(-1,NULL,-1,NULL);
  bool editing_state_changed =
    tree->update_structure(NULL,src->is_top_level_complete(),meta_manager);
  if (editing_state_changed)
    get_tree_ctrl()->Invalidate();
  if ((display != NULL) && (display_box_pos >= 0))
    display->update_display();
}

/*****************************************************************************/
/*                        kdws_metashow::deactivate                          */
/*****************************************************************************/

void
  kdws_metashow::deactivate()
{
  get_tree_ctrl()->DeleteAllItems();
  get_edit_ctrl()->SetWindowText(_T(""));
  if (tree != NULL)
    delete tree;
  tree = display = NULL;
  src = NULL;
  num_codestreams = num_jpch = num_jplh = 0;
}

/*****************************************************************************/
/*                  kdws_metashow::select_matching_metanode                  */
/*****************************************************************************/

void
  kdws_metashow::select_matching_metanode(jpx_metanode jpx_node)
{
  kdws_metanode *node = find_matching_metanode(tree,jpx_node);
  if (node != NULL)
    {
      get_tree_ctrl()->EnsureVisible(node->handle);
      get_tree_ctrl()->SelectItem(node->handle);
    }
}

/*****************************************************************************/
/*                     kdws_metashow::DoDataExchange                         */
/*****************************************************************************/

void kdws_metashow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

/*****************************************************************************/
/*                     kdws_metashow::PostNcDestroy                          */
/*****************************************************************************/

void kdws_metashow::PostNcDestroy() 
{
  CDialog::PostNcDestroy();
  if (renderer != NULL)
    {
      assert(renderer->metashow == this);
      renderer->metashow = NULL;
      renderer = NULL;
    }
  delete this;
}

/*****************************************************************************/
/*                       MESSAGE MAP for kdws_metashow                       */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_metashow, CDialog)
	ON_WM_CLOSE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_META_TREE, OnSelchangedMetaTree)
	ON_WM_SIZE()
  ON_NOTIFY(NM_CUSTOMDRAW, IDC_META_TREE, tree_ctrl_custom_draw)
END_MESSAGE_MAP()

/*****************************************************************************/
/*                          kdws_metashow::OnClose                           */
/*****************************************************************************/

void kdws_metashow::OnClose() 
{
  DestroyWindow();	
}

/*****************************************************************************/
/*                           kdws_metashow::OnOK                             */
/*****************************************************************************/

void kdws_metashow::OnOK() 
{
  DestroyWindow();	
}

/*****************************************************************************/
/*                    kdws_metashow::OnSelchangedMetaTree                    */
/*****************************************************************************/

void kdws_metashow::OnSelchangedMetaTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

  kdws_metanode *node = (kdws_metanode *) pNMTreeView->itemNew.lParam;
  *pResult = 0;
  if (node == NULL)
    return;
  if (display != NULL)
    { // Clear the display
      display_box_pos = -1;
      display_buf_pos = 0;
      display = NULL;
      get_edit_ctrl()->SetWindowText(_T(""));
    }
  if (node->can_display)
    { // Display the contents of this node
      display = node;
      display_buf_pos = display_box_pos = 0;
      node->update_display();
    }
  if (node->codestream_idx >= 0)
    renderer->set_codestream(node->codestream_idx);
  else if (node->compositing_layer_idx >= 0)
    renderer->set_compositing_layer(node->compositing_layer_idx);
  else if (node->box_type == jp2_composition_4cc)
    renderer->menu_NavMultiComponent();
}

/*****************************************************************************/
/*                          kdws_metashow::OnSize                            */
/*****************************************************************************/

void kdws_metashow::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!started)
    return;

  kdu_coords new_dims;
  new_dims.x = cx; new_dims.y = cy;
  if (dialog_dims != new_dims)
    {
      kdu_coords change = new_dims - dialog_dims;
      tree_dims.y += change.y;
      edit_dims.y += change.y;
      edit_dims.x += change.x;
      if ((edit_dims.x >= 50) && (edit_dims.y >= 50))
        get_edit_ctrl()->SetWindowPos(NULL,0,0,edit_dims.x,edit_dims.y,
                                      SWP_NOMOVE | SWP_NOOWNERZORDER);
      if ((tree_dims.x >= 50) && (tree_dims.y >= 50))
        get_tree_ctrl()->SetWindowPos(NULL,0,0,tree_dims.x,tree_dims.y,
                                      SWP_NOMOVE | SWP_NOOWNERZORDER);
    }
  dialog_dims = new_dims;
}

/*****************************************************************************/
/*                  kdws_metashow::tree_ctrl_custom_draw                     */
/*****************************************************************************/

void kdws_metashow::tree_ctrl_custom_draw(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NMTVCUSTOMDRAW *cdraw = (NMTVCUSTOMDRAW *) pNMHDR;
  *pResult = CDRF_DODEFAULT;
  if (cdraw->nmcd.dwDrawStage == CDDS_PREPAINT)
    *pResult = CDRF_NOTIFYITEMDRAW;
  else if (cdraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
      kdws_metanode *node = (kdws_metanode *) cdraw->nmcd.lItemlParam;
      if (node == NULL)
        return;
      if (node->is_changed || node->ancestor_changed)
        cdraw->clrText = changed_text_colour;
      else if (node->is_deleted)
        {
          cdraw->clrText = deleted_text_colour;
          ::SelectObject(cdraw->nmcd.hdc,deleted_text_font);
          *pResult = CDRF_NEWFONT;
        }
    }
}
