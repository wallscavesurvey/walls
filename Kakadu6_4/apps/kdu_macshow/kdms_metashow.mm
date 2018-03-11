/*****************************************************************************/
// File: kdms_metashow.mm [scope = APPS/MACSHOW]
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
 Implements the `kdms_metashow' class, which provides a window for dynamically
 displaying the metadata structure from JP2-family files.  The display is
 dynamic in the sense that it can be updated as more information becomes
 available in the data source (relevant only if it is a client cache).
 *****************************************************************************/
#import "kdms_metashow.h"
#include "kdms_window.h"

#define KDMS_MAX_DISPLAY_CHARS 4096

/*===========================================================================*/
/*                             INTERNAL FUNCTIONS                            */
/*===========================================================================*/

/*****************************************************************************/
/* STATIC              find_selected_node_in_browser_column                  */
/*****************************************************************************/

static kdms_metanode *
  find_selected_node_in_browser_column(kdms_metanode *root, NSBrowser *browser,
                                       NSInteger column)
/* This function is used by quite a number of the `kdms_metanode_delegate'
 object's functions to find the node the metadata tree whose children
 comprise a given browser column.  It does this by scanning the tree,
 starting from the root and identifying the selected branches in each
 browser column.  The function keeps track of which child is the selected
 branch in each node via the `kdms_metanode::selected_child' member.  It
 uses this member to improve the efficiency of the scan, where possible;
 it also makes whatever changes are required to the `selected_child'
 path, based upon the selected branch information returned by the browser. */
{
  kdms_metanode *scan, *node=root;
  while ((node != NULL) && (node->browser_column < column))
    {
      int branch_idx = [browser selectedRowInColumn:node->browser_column];
      if ((node->selected_child != NULL) &&
          (node->selected_child->browser_row != branch_idx))
        { // Selection is changing; erase all selections from this point
          kdms_metanode *tmp=node;
          while ((scan=tmp->selected_child) != NULL)
            {
              tmp->selected_child = NULL;
              tmp = scan;
            }
        }
      if (node->selected_child == NULL)
        {
          for (scan=node->children; scan != NULL; scan=scan->next)
            if (scan->browser_row == branch_idx)
              break;
          node->selected_child = scan;
        }
      node = node->selected_child;
    }
  return node;  
}

/*****************************************************************************/
/* STATIC                  find_matching_metanode                            */
/*****************************************************************************/

static kdms_metanode *
  find_matching_metanode(kdms_metanode *container, jpx_metanode jpx_node)
  /* Recursively searches through the metadata tree to see if we can find
   a node which references the supplied `jpx_node' object.  In the case of
   multiple matches, the deepest match in the tree will be found, which is
   generally the most useful behaviour. */
{
  kdms_metanode *scan, *result;
  for (scan=container->children; scan != NULL; scan=scan->next)
    if ((result = find_matching_metanode(scan,jpx_node)) != NULL)
      return result;
  return (container->jpx_node == jpx_node)?container:NULL;
}

/*****************************************************************************/
/* STATIC                   set_selections_to_node                           */
/*****************************************************************************/

static void
  set_selections_to_node(kdms_metanode *node, NSBrowser *browser)
  /* Recursively walks back up the tree from `node' to the root of the
   metadata tree and then walks back down again, setting the selected cell
   in each column so that the node is reached. */
{
  if (node->parent == NULL)
    return; // Nothing to select
  set_selections_to_node(node->parent,browser);
  [browser selectRow:node->browser_row inColumn:node->parent->browser_column];
}

/*****************************************************************************/
/* STATIC INLINE                make_4cc_char                                */
/*****************************************************************************/

static inline char make_4cc_char(char ch)
  /* Ensures that 4CC codes can be printed. */
{
  if (ch == ' ')
    return '_';
  if ((ch < 0x20) || (ch & 0x80))
    return '.';
  return ch;
}

/*****************************************************************************/
/* STATIC INLINE              make_printable_char                            */
/*****************************************************************************/

static inline unichar make_printable_char(char ch)
  /* Might do some more interesting substitutions in the future. */
{
  if ((ch < 0x20) || (ch & 0x80))
    return '.';
  else
    return ch;
}

/*****************************************************************************/
/* STATIC INLINE                   make_hex                                  */
/*****************************************************************************/

static inline char make_hex(kdu_byte val)
{
  val &= 0x0F;
  if (val < 10)
    return (char) ('0'+val);
  else
    return (char) ('A'+val-10);
}


/*===========================================================================*/
/*                               kdms_metanode                               */
/*===========================================================================*/

/*****************************************************************************/
/*                       kdms_metanode::kdms_metanode                        */
/*****************************************************************************/

kdms_metanode::kdms_metanode(kdms_metashow_delegate *owner,
                             kdms_metanode *parent,
                             NSBrowser *browser)
{
  this->owner = owner;
  this->parent = parent;
  this->browser = browser;
  this->has_no_box = true;
  this->is_deleted = false;
  this->is_changed = false;
  this->ancestor_changed = false;
  box_type = 0;
  box_header_length = 0;
  codestream_idx = compositing_layer_idx = -1;
  child_list_complete = contents_complete = false;
  children = last_child = next = NULL;
  if (parent == NULL)
    browser_column = 0;
  else
    browser_column = parent->browser_column + 1;
  if ((parent == NULL) || (parent->last_child == NULL))
    browser_row = 0;
  else
    browser_row = parent->last_child->browser_row + 1;
  selected_child = NULL;
  browser_column_needs_update = true;
  name[0] = '\0';
  responds_to_double_click = false;
  can_display_contents = false;
  is_display_node = false;
  is_leaf = false;
  leaf_content_bytes = 0;
}

/*****************************************************************************/
/*                      kdms_metanode::~kdms_metanode                        */
/*****************************************************************************/

kdms_metanode::~kdms_metanode()
{
  while ((last_child=children) != NULL)
    {
      children = last_child->next;
      delete last_child;
    }
}

/*****************************************************************************/
/*                     kdms_metanode::update_structure                       */
/*****************************************************************************/

bool
  kdms_metanode::update_structure(jp2_family_src *jp2_src,
                                  jpx_meta_manager meta_manager)
{
  jp2_input_box container_box; // Box which contains this node's children
  bool contents_were_complete = contents_complete;
  kdu_long previous_leaf_content_bytes = leaf_content_bytes;
  if (!contents_complete)
    { // Need to open `container_box', if there is one
      if (parent == NULL)
        contents_complete = jp2_src->is_top_level_complete();
      else
        {
          container_box.open(jp2_src,locator);
          if (container_box.exists())
            {
              contents_complete = container_box.is_complete();
              if (is_leaf)
                leaf_content_bytes = container_box.get_remaining_bytes();
            }
        }
    }
  else
    assert(child_list_complete);
  
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

  kdms_metanode *child;
  bool added_something = false;
  
  if (!child_list_complete)
    { // Add onto the child list, if we can
      jp2_input_box box;
      
      if (parent == NULL)
        {
          if ((last_child == NULL) || last_child->has_no_box)
            box.open(jp2_src);
          else
            {
              box.open(jp2_src,last_child->locator);
              if (box.exists())
                {
                  box.close();
                  box.open_next();
                }
            }
        }
      else if (container_box.exists())
        { // Open child sub-boxes in sequence, until we get to the end of the
          // known set of children
          box.open(&container_box);
          for (child=children; (child!=NULL) && box.exists();
               child=child->next)
            {
              if (child->has_no_box)
                continue;
              box.close();
              box.open_next();
            }
        }
      while (box.exists())
        { // Have a new child
          added_something = true;
          if (box.get_remaining_bytes() < 0)
            contents_complete = true; // Rubber length box must be last
          child = new kdms_metanode(owner,this,browser);
          if (last_child == NULL)
            children = last_child = child;
          else
            last_child = last_child->next = child;
          child->has_no_box = false;
          child->box_type = box.get_box_type();
          child->locator = box.get_locator();
          child->contents_bin = box.get_contents_locator().get_databin_id();
          child->box_header_length = box.get_box_header_length();
          child->child_list_complete =
            child->is_leaf = !jp2_is_superbox(child->box_type);
          child->create_browser_view();
          box.close();
          box.open_next();
        }
      if (contents_complete)
        child_list_complete = true;
    }
  
  bool any_changes = false;
  if (added_something || (contents_complete && !contents_were_complete))
    { // May need to redisplay this browser column
      any_changes = true;
      browser_column_needs_update = true;
    }
  if (((leaf_content_bytes > previous_leaf_content_bytes) ||
       editing_state_changed) &&
      (!parent->browser_column_needs_update) &&
      is_visible_in_browser())
    { // Make adjustments to just this node's appearance
      NSBrowserCell *cell = [browser loadedCellAtRow:browser_row
                                              column:parent->browser_column];
      if (cell != nil)
        write_name_to_browser_cell(cell);
      [browser setNeedsDisplay:YES];
    }
  if (container_box.exists())
    container_box.close();
  if (is_display_node &&
      ((leaf_content_bytes > previous_leaf_content_bytes) ||
       jpx_node_just_found || editing_state_changed))
    [owner update_info_text];
      
  // Now scan through the children, trying to complete them
  for (child=children; child != NULL; child=child->next)
    if ((!child->has_no_box) && child->update_structure(jp2_src,
                                                        meta_manager))
      any_changes = true;
  
  return any_changes;
}

/*****************************************************************************/
/*                    kdms_metanode::create_browser_view                     */
/*****************************************************************************/

void
  kdms_metanode::create_browser_view()
{
  if (box_type == 0)
    strcpy(name,"????"); // Should not be possible
  else
    {
      name[0] = make_4cc_char((char)(box_type>>24));
      name[1] = make_4cc_char((char)(box_type>>16));
      name[2] = make_4cc_char((char)(box_type>>8));
      name[3] = make_4cc_char((char)(box_type>>0));
      name[4] = '\0';
    }
  
  if ((box_type == jp2_codestream_4cc) && (parent != NULL) &&
      (parent->parent == NULL))
    codestream_idx = [owner found_new_codestream_box];
  else if (box_type == jp2_codestream_header_4cc)
    codestream_idx = [owner found_new_codestream_header_box];
  else if (box_type == jp2_compositing_layer_hdr_4cc)
    compositing_layer_idx = [owner found_new_compositing_layer_box];
  
  char *cp = name + strlen(name);
  sprintf(cp," @%d",(int)locator.get_file_pos());
  cp += strlen(cp);
  if (locator.get_databin_id() >= 0)
    {
      sprintf(cp," {H%d;B%d}",(int)locator.get_databin_id(),(int)contents_bin);
      cp += strlen(cp);
    }
  
  if (box_type == jp2_file_type_4cc)
    {
      can_display_contents = true;
    }
  else if ((box_type == jp2_file_type_4cc) || (box_type == jp2_label_4cc) ||
           (box_type == jp2_xml_4cc) || (box_type == jp2_iprights_4cc))
    {
      can_display_contents = true;
    }
  else if (box_type == jp2_roi_description_4cc)
    {
      can_display_contents = true;
      responds_to_double_click = true;
    }
  else if ((!jp2_is_superbox(box_type)) && (box_type != jp2_codestream_4cc) &&
           (parent != NULL))
    {
      can_display_contents = true;
    }
  else if (codestream_idx >= 0)
    {
      sprintf(cp," [stream %d]",codestream_idx+1);
      responds_to_double_click = true;
    }
  else if (compositing_layer_idx >= 0)
    {
      sprintf(cp," [layer %d]",compositing_layer_idx+1);
      responds_to_double_click = true;
    }
  else if (box_type == jp2_composition_4cc)
    {
      strcpy(cp," [composition]");
      responds_to_double_click = true;
    }
  if (responds_to_double_click)
    {
      assert(children == NULL);
      children = last_child = new kdms_metanode(owner,this,browser);
      children->child_list_complete = true;
      strcpy(children->name,"(dbl clk link to show)");
    }
}

/*****************************************************************************/
/*                   kdms_metanode::is_visible_in_browser                    */
/*****************************************************************************/

bool kdms_metanode::is_visible_in_browser()
{
  if (parent == NULL)
    return false; // We don't actually display the root node as a browser cell
  if ((parent->parent != NULL) &&
      (parent->parent->selected_child != parent))
    return false;
  int first_visible_column = [browser firstVisibleColumn];
  int last_visible_column = [browser lastVisibleColumn];
  return ((parent->browser_column >= first_visible_column) &&
          (parent->browser_column <= last_visible_column));
}

/*****************************************************************************/
/*                 kdms_metanode::write_name_to_browser_cell                 */
/*****************************************************************************/

void kdms_metanode::write_name_to_browser_cell(NSBrowserCell *cell)
{
  [cell setLeaf:(child_list_complete && (children==NULL))?YES:NO];

  NSString *ns_text = [NSString stringWithUTF8String:name];
  if (is_leaf && (leaf_content_bytes >= 0))
    {
      if (contents_complete)
        ns_text = [ns_text stringByAppendingFormat:@" (%qi bytes)",
                   (long long) leaf_content_bytes];
      else
        ns_text = [ns_text stringByAppendingFormat:@" (%qi+ bytes)",
                   (long long) leaf_content_bytes];
    }
  NSDictionary *attributes = nil;
  if (has_no_box)
    attributes = [owner get_informative_text_attributes];
  else
    {
      int text_style = KDS_TEXT_ORIGINAL;
      if (is_changed || ancestor_changed)
        text_style = KDS_TEXT_CHANGED;
      if (is_deleted)
        text_style = KDS_TEXT_DELETED;
      text_style |= (is_leaf)?KDS_TEXT_ATTRIBUTE_LEAF:0;
      text_style |= (responds_to_double_click)?KDS_TEXT_ATTRIBUTE_LINK:0;
      attributes = [owner get_text_attributes:text_style];
    }
  if (attributes)
    {
      NSAttributedString *attributed_string =
        [[NSAttributedString alloc] initWithString:ns_text
                                        attributes:attributes];
      [cell setAttributedStringValue:attributed_string];
      [attributed_string release];
    }
  else
    [cell setStringValue:ns_text];
}


/*===========================================================================*/
/*                          kdms_metashow_delegate                           */
/*===========================================================================*/

@implementation kdms_metashow_delegate

/*****************************************************************************/
/*                   kdms_metashow_delegate:initWithOwner                    */
/*****************************************************************************/

- (void) initWithOwner:(kdms_renderer *) renderer
{
  owner = renderer;
  browser = nil;
  info_text_panel = nil;
  info_text_frame = nil;
  edit_button = nil;
  src = NULL;
  num_codestreams = 0;
  num_jpch = 0;
  num_jplh = 0;
  tree = NULL;
  display_node = NULL;
  display_node_content_bytes = 0;
  informative_text_attributes = nil;
  for (int a=0; a < 12; a++)
    text_attributes[a] = nil;
  id objects[6], keys[6];
  
  // Start by creating informative text dictionary
  keys[0] = NSForegroundColorAttributeName;
  objects[0] = [NSColor redColor];
  informative_text_attributes =
    [NSDictionary dictionaryWithObjects:objects forKeys:keys count:1];
  [informative_text_attributes retain];
  
  // Now create the other text dictionaries
  int edits, link, leaf;
  for (edits=0; edits < 12; edits+=4)
    for (link=0; link < 4; link+=2)
      for (leaf=0; leaf < 2; leaf++)
        {
          int nkeys = 0;
          if (link)
            {
              keys[nkeys] = NSUnderlineStyleAttributeName;
              objects[nkeys] = [NSNumber numberWithInt:NSUnderlineStyleSingle];
              nkeys++;
              keys[nkeys] = NSForegroundColorAttributeName;
              objects[nkeys] = [NSColor blueColor];
              nkeys++;
            }
          if (leaf)
            {
              keys[nkeys] = NSFontAttributeName;
              objects[nkeys] = [NSFont boldSystemFontOfSize:
                                [NSFont systemFontSize]];
              nkeys++;
            }
          if (edits == KDS_TEXT_DELETED)
            {
              keys[nkeys] = NSStrikethroughStyleAttributeName;
              objects[nkeys] = [NSNumber numberWithInt:
                                NSUnderlineStyleSingle |
                                NSUnderlinePatternSolid];
              nkeys++;
              keys[nkeys] = NSStrikethroughColorAttributeName;
              objects[nkeys] = [NSColor redColor];
              nkeys++;              
            }
          else if (edits == KDS_TEXT_CHANGED)
            {
              keys[nkeys] = NSStrikethroughStyleAttributeName;
              objects[nkeys] = [NSNumber numberWithInt:
                                NSUnderlineStyleSingle |
                                NSUnderlinePatternDot];
              nkeys++;
              keys[nkeys] = NSBackgroundColorAttributeName;
              objects[nkeys] = [NSColor colorWithCalibratedRed:1.0
                                                         green:0.9
                                                          blue:0.8
                                                         alpha:1.0];
              nkeys++;              
              keys[nkeys] = NSForegroundColorAttributeName;
              objects[nkeys] = [NSColor colorWithCalibratedRed:0.3
                                                         green:0.2
                                                          blue:0.1
                                                         alpha:1.0];
              nkeys++;              
            }
          else
            assert(edits == KDS_TEXT_ORIGINAL);
          assert(nkeys <= 6);
          int att_idx = edits | link | leaf;
          assert(att_idx < 12);
          if (nkeys > 0)
            {
               
              text_attributes[att_idx] =
                [NSDictionary dictionaryWithObjects:objects
                                            forKeys:keys
                                              count:nkeys];
              [text_attributes[att_idx] retain];
            }
        }
}

/*****************************************************************************/
/*    kdms_metashow_delegate:setBrowser:infoPanel:infoFrame:editButton       */
/*****************************************************************************/

- (void) setBrowser:(NSBrowser *)the_browser
          infoPanel:(NSTextView *)info_panel
          infoFrame:(NSBox *)info_frame
         editButton:(NSButton *)button
{
  self->browser = the_browser;
  self->info_text_panel = info_panel;
  self->info_text_frame = info_frame;
  self->edit_button = button;
  [browser setTarget:self];
  [browser sendActionOn:NSLeftMouseDownMask];
  [browser setAction:@selector(user_click:)];
  [browser setDoubleAction:@selector(user_double_click:)];
}

/*****************************************************************************/
/*                        kdms_metashow_delegate:reset                       */
/*****************************************************************************/

- (void) reset
{
  if (display_node != NULL)
    {
      if (info_text_panel)
        [info_text_panel setString:@""];
      if (info_text_frame)
        [info_text_frame setTitle:@""];
      [edit_button setEnabled:NO];
      display_node = NULL;
      display_node_content_bytes = 0;
    }
  if (tree != NULL)
    delete tree;
  tree = NULL;
  src = NULL;
  num_codestreams = 0;
  num_jpch = 0;
  num_jplh = 0;
  if (browser)
    [browser reloadColumn:0];
}

/*****************************************************************************/
/*                       kdms_metashow_delegate:dealloc                      */
/*****************************************************************************/

- (void) dealloc
{
  if (tree != NULL)
    delete tree;
  tree = NULL;
  if (informative_text_attributes)
    [informative_text_attributes release];
  for (int n=0; n < 12; n++)
    if (text_attributes[n])
      [text_attributes[n] release];
  [super dealloc];
}

/*****************************************************************************/
/*                kdms_metashow_delegate:activateWithSource                  */
/*****************************************************************************/

- (void) activateWithSource:(jp2_family_src *)source
{
  if ((self->src != NULL) && (self->src != source))
    [self reset];
  self->src = source;
  tree = new kdms_metanode(self,NULL,browser);
}

/*****************************************************************************/
/*                  kdms_metashow_delegate:update_metadata                   */
/*****************************************************************************/

- (void) update_metadata
{
  if ((src == NULL) || (tree == NULL))
    return;
  jpx_meta_manager meta_manager = owner->get_meta_manager();
  if (meta_manager.exists())
    meta_manager.load_matches(-1,NULL,-1,NULL);
  bool any_change = false;
  try {
    any_change = tree->update_structure(src,meta_manager);
  } catch (kdu_exception) { }
  if (any_change)
    {
      [browser validateVisibleColumns];
      [browser setNeedsDisplay:YES]; // Required if window does not have focus
    }
}

/*****************************************************************************/
/*             kdms_metashow_delegate:select_matching_metanode               */
/*****************************************************************************/

- (void) select_matching_metanode:(jpx_metanode)jpx_node
{
  kdms_metanode *node = find_matching_metanode(tree,jpx_node);
  if (node == NULL)
    return;
  set_selections_to_node(node,browser);
  [self user_click:browser];
  [browser setNeedsDisplay:YES];
}

/*****************************************************************************/
/*             kdms_metashow_delegate:found_new_codestream_box               */
/*****************************************************************************/

- (int) found_new_codestream_box
{
  return num_codestreams++;
}

/*****************************************************************************/
/*          kdms_metashow_delegate:found_new_codestream_header_box           */
/*****************************************************************************/

- (int) found_new_codestream_header_box
{
  return num_jpch++;
}

/*****************************************************************************/
/*          kdms_metashow_delegate:found_new_compositing_layer_box           */
/*****************************************************************************/

- (int) found_new_compositing_layer_box
{
  return num_jplh++;
}

/*****************************************************************************/
/*          kdms_metashow_delegate:get_informative_text_attributes           */
/*****************************************************************************/

- (NSDictionary *) get_informative_text_attributes
{
  return informative_text_attributes;
}

/*****************************************************************************/
/*               kdms_metashow_delegate:get_text_attributes                  */
/*****************************************************************************/

- (NSDictionary *) get_text_attributes:(int)style
{
  if ((style < 0) || (style >= 12))
    return nil;
  return text_attributes[style];
}

/*****************************************************************************/
/*            kdms_metashow_delegate:configure_edit_button_for_node          */
/*****************************************************************************/

-(void) configure_edit_button_for_node:(kdms_metanode *)node
{
  if ((node == NULL) || (!node->jpx_node) ||
      (node->jpx_node.is_deleted()) ||
      (node->box_type == jp2_free_4cc))
    [edit_button setEnabled:NO];
  else
    [edit_button setEnabled:YES];
}

/*****************************************************************************/
/*                   kdms_metashow_delegate:update_info_text                 */
/*****************************************************************************/

- (void) update_info_text
{
  if (display_node == NULL)
    return;
  [self configure_edit_button_for_node:display_node];

  if (display_node->box_type == jp2_file_type_4cc)
    { // Read entire box and generate representation from scratch
      if (display_node_content_bytes == 0)
        [info_text_frame setTitle:@"JP2 File-Type Box (expanded)"];
      NSString *text = [NSString string];
      jp2_input_box box;
      box.open(src,display_node->locator);
      
      int num_brands = 1;
      kdu_uint32 brand=0, minor_version=0;
      box.read(brand);
      box.read(minor_version);
      text = [text stringByAppendingFormat:
              @"Brand = \"%c%c%c%c\"\n"
              "Minor version = %d\n"
              "Compatibility list:",
              make_4cc_char((char)(brand>>24)),
              make_4cc_char((char)(brand>>16)),
              make_4cc_char((char)(brand>>8)),
              make_4cc_char((char)(brand>>0)),
              minor_version];
      while (box.read(brand) && (num_brands < 100)) // Avoid memory issues
        {
          num_brands++;
          text = [text stringByAppendingFormat:
                   @"\n    \"%c%c%c%c\"",
                   make_4cc_char((char)(brand>>24)),
                   make_4cc_char((char)(brand>>16)),
                   make_4cc_char((char)(brand>>8)),
                   make_4cc_char((char)(brand>>0))];
        }
      box.close();
      display_node_content_bytes = display_node->leaf_content_bytes;
      [info_text_panel setString:text];
    }
  else if ((display_node->box_type == jp2_label_4cc) ||
           (display_node->box_type == jp2_xml_4cc) ||
           (display_node->box_type == jp2_iprights_4cc))
    { // Write box as UTF8 text
      if (display_node_content_bytes < 0)
        return;
      if (display_node_content_bytes == 0)
        [info_text_frame setTitle:
         [NSString stringWithFormat:@"\"%s\" Box (as text)",
          display_node->name]];
      jp2_input_box box;
      box.open(src,display_node->locator);
      char utf8_buf[3*KDMS_MAX_DISPLAY_CHARS+1];
      kdu_uint16 unicode_buf[KDMS_MAX_DISPLAY_CHARS+1];
      display_node_content_bytes =
        box.read((kdu_byte *)utf8_buf,3*KDMS_MAX_DISPLAY_CHARS);
      utf8_buf[display_node_content_bytes] = '\0';
      box.close();
      int num_unichars =
        kdms_utf8_to_unicode(utf8_buf,unicode_buf,KDMS_MAX_DISPLAY_CHARS);
      if (num_unichars == KDMS_MAX_DISPLAY_CHARS)
        { // Filled the display buffer
          display_node_content_bytes = -1;
          unicode_buf[num_unichars-1] = ((kdu_uint16) '.');
          unicode_buf[num_unichars-2] = ((kdu_uint16) '.');
          unicode_buf[num_unichars-3] = ((kdu_uint16) '.');
          unicode_buf[num_unichars-3] = ((kdu_uint16) '.');
        }
      [info_text_panel setString:[NSString stringWithCharacters:unicode_buf
                                                         length:num_unichars]];
    }
  else
    { // Print hex dump of contents
      if (display_node_content_bytes == 0)
        [info_text_frame setTitle:
         [NSString stringWithFormat:@"\"%s\" Box (hex dump)",
          display_node->name]];
      int max_chars_left = KDMS_MAX_DISPLAY_CHARS - display_node_content_bytes;
      if (max_chars_left < 0)
        return;
      
      jp2_input_box box;
      box.open(src,display_node->locator);
      box.seek(display_node_content_bytes);
      char buf[256];
      char display_buf[256*4+128+1];
      while (max_chars_left>=0)
        {
          int max_chars = max_chars_left;
          if (max_chars > 256)
            max_chars = 256;
          int num_chars = box.read((kdu_byte *)buf,max_chars);
          if (!display_node->contents_complete)
            num_chars &= ~7; // Round down to multiple of 8 bytes = 1 line
          if (num_chars == 0)
            break;
          int display_chars = 0;
          int n, row, col;
          for (n=0, row=0; n < num_chars; row++)
            {
              for (col=0; (n < num_chars) && (col < 8); col++, n++)
                {
                  display_buf[display_chars++] = make_hex(buf[n]>>4);
                  display_buf[display_chars++] = make_hex(buf[n]);
                  display_buf[display_chars++] = ' ';
                }
              for (int p=col; p < 8; p++)
                {
                  display_buf[display_chars++] = ' ';
                  display_buf[display_chars++] = ' ';
                  display_buf[display_chars++] = ' ';
                }
              display_buf[display_chars++] = '|';
              display_buf[display_chars++] = ' ';
              for (n-=col; col > 0; col--, n++)
                display_buf[display_chars++] = make_printable_char(buf[n]);
              display_buf[display_chars++] = '\n';
            }
          display_buf[display_chars] = '\0';
          
          NSString *text = [NSString stringWithUTF8String:display_buf];
          display_node_content_bytes += num_chars;
          max_chars_left -= num_chars;
          if ((max_chars_left <= 0) &&
              (display_node_content_bytes < display_node->leaf_content_bytes))
            {
              text = [text stringByAppendingString:@"...."];
              display_node_content_bytes += 4; // So we don't come back here
              max_chars_left -= 4;
            }
          [info_text_panel setString:
           [[info_text_panel string] stringByAppendingString:text]];
        }
      box.close();
    }
}

/*****************************************************************************/
/*            kdms_metashow_delegate:browser:numberOfRowsInColumn            */
/*****************************************************************************/

-(NSInteger)browser:(NSBrowser *)sender numberOfRowsInColumn:(NSInteger)column
{
  kdms_metanode *node =
    find_selected_node_in_browser_column(tree,sender,column);
  if (node == NULL)
    return 0;
  node->browser_column_needs_update = false;
  
  int result = 0;
  if (node->children != NULL)
    { // Result is the number of children in `node', possibly with an extra ...
      result = node->last_child->browser_row + 1;
      if (!node->child_list_complete)
        result++;
    }  
  return result;
}

/*****************************************************************************/
/*       kdms_metashow_delegate:browser:willDisplayCell:atRow:column         */
/*****************************************************************************/

-(void)browser:(NSBrowser *)sender willDisplayCell:(id)cell
         atRow:(NSInteger)row column:(NSInteger)column
{
  kdms_metanode *node =
    find_selected_node_in_browser_column(tree,sender,column);
  if (node == NULL)
    return;
  for (node=node->children; node != NULL; node=node->next)
    if (node->browser_row == row)
      break;
  
  if (node == NULL)
    {
      [cell setLeaf:YES];
      [cell setStringValue:@"..."];
    }
  else
    node->write_name_to_browser_cell(cell);
}

/*****************************************************************************/
/*             kdms_metashow_delegate:browser:isColumnValid                  */
/*****************************************************************************/

-(BOOL)browser:(NSBrowser *)sender isColumnValid:(NSInteger)column
{
  kdms_metanode *node =
    find_selected_node_in_browser_column(tree,sender,column);
  if (node == NULL)
    return YES; // Should not happen
  return (node->browser_column_needs_update)?NO:YES;
}

/*****************************************************************************/
/*                     kdms_metashow_delegate:user_click                     */
/*****************************************************************************/

-(IBAction)user_click:(NSBrowser *)sender
{
  int column = [sender selectedColumn];
  kdms_metanode *node =
    find_selected_node_in_browser_column(tree,sender,column);
  if (node == NULL)
    return;
  int branch_idx = [sender selectedRowInColumn:column];
  for (node=node->children; node != NULL; node=node->next)
    if (node->browser_row == branch_idx)
      break;
  if (node == NULL)
    return;
  [self configure_edit_button_for_node:node];
  if (node == display_node)
    return;
  if (display_node != NULL)
    {
      [info_text_panel setString:@""];
      [info_text_frame setTitle:@""];
      display_node->is_display_node = false;
      display_node = NULL;
      display_node_content_bytes = 0;
    }
  if (!node->can_display_contents)
    return;
  display_node = node;
  node->is_display_node = true;
  [self update_info_text];
}

/*****************************************************************************/
/*                 kdms_metashow_delegate:user_double_click                  */
/*****************************************************************************/

-(IBAction)user_double_click:(NSBrowser *)sender
{
  int column = [sender selectedColumn];
  kdms_metanode *node =
    find_selected_node_in_browser_column(tree,sender,column);
  if (node == NULL)
    return;
  int branch_idx = [sender selectedRowInColumn:column];
  for (node=node->children; node != NULL; node=node->next)
    if (node->browser_row == branch_idx)
      break;
  if ((node == NULL) || (owner == NULL) || !node->responds_to_double_click)
    return;
  if (node->box_type == jp2_roi_description_4cc)
    owner->show_imagery_for_metanode(node->jpx_node);
  else if (node->codestream_idx >= 0)
    owner->set_codestream(node->codestream_idx);
  else if (node->compositing_layer_idx >= 0)
    owner->set_compositing_layer(node->compositing_layer_idx);
  else
    owner->menu_NavMultiComponent();
  [owner->window orderWindow:NSWindowBelow
   relativeTo:[[browser window] windowNumber]];
}

/*****************************************************************************/
/*                 kdms_metashow_delegate:edit_button_click                  */
/*****************************************************************************/

-(IBAction)edit_button_click:(NSButton *)sender
{
  int column = [browser selectedColumn];
  kdms_metanode *node =
    find_selected_node_in_browser_column(tree,browser,column);
  if (node == NULL)
    return;
  int branch_idx = [browser selectedRowInColumn:column];
  for (node=node->children; node != NULL; node=node->next)
    if (node->browser_row == branch_idx)
      break;
  if ((node == NULL) || (!node->jpx_node.exists()) ||
      node->jpx_node.is_deleted() || (node->box_type == jp2_free_4cc))
    { // Should not have been allowed
      [edit_button setEnabled:NO];
      return;
    }
  owner->edit_metanode(node->jpx_node);
}

@end // kdms_metashow_delegate


/*===========================================================================*/
/*                               kdms_metashow                               */
/*===========================================================================*/

@implementation kdms_metashow

/*****************************************************************************/
/*            kdms_metashow:initWithOwner:location:andIdentifier             */
/*****************************************************************************/

- (void)initWithOwner:(kdms_renderer *)renderer
             location:(NSPoint)preferred_location
        andIdentifier:(int)the_identifier
{
  owner = renderer;
  delegate = nil;
  browser = nil;
  edit_button = nil;
  text_panel = nil;
  text_panel_frame = nil;
  self->identifier = the_identifier;
  self->filename = NULL;
  
  // Create the window
  NSRect screen_rect = [[NSScreen mainScreen] frame];
  NSRect frame_rect = screen_rect;
  frame_rect.size.height = (float) ceil(frame_rect.size.height * 0.3);
  frame_rect.size.width = (float) ceil(frame_rect.size.width * 0.5);
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
  
  NSUInteger window_style = NSTitledWindowMask | NSClosableWindowMask |
                            NSResizableWindowMask;
  NSRect content_rect = [NSWindow contentRectForFrameRect:frame_rect
                                                styleMask:window_style];
  [super initWithContentRect:content_rect
                   styleMask:window_style
                     backing:NSBackingStoreBuffered defer:NO];
  [super setReleasedWhenClosed:NO];
  char title[40];
  sprintf(title,"Metashow %d",identifier);
  [self setTitle:[NSString stringWithUTF8String:title]];
  [self setFrame:frame_rect display:NO];
  
  NSView *super_view = [self contentView];
  content_rect = [super_view frame]; // Makes the origin relative to the window
  
  // Create browser and position it at the left edge of the super_view
  NSRect browser_rect = content_rect;
  browser_rect.size.width = (float) floor(0.55*content_rect.size.width);
  browser = [NSBrowser alloc];
  [browser initWithFrame:browser_rect];
  [browser setMaxVisibleColumns:2];
  [browser setTitle:@"Top Level Boxes" ofColumn:0];
  [browser setHasHorizontalScroller:YES];
  [browser setColumnResizingType:NSBrowserAutoColumnResizing];
  [browser setAutoresizingMask:NSViewMaxXMargin | NSViewWidthSizable |
   NSViewHeightSizable | NSViewMinYMargin | NSViewMaxYMargin];
  [super_view addSubview:browser];
  
  // Create a button which we can use for metadata editing
  NSRect button_rect = content_rect;
  button_rect.origin.y = 2;
  button_rect.size.height = 19;
  button_rect.origin.x = browser_rect.origin.x+browser_rect.size.width;
  button_rect.size.width -= browser_rect.size.width;
  button_rect.origin.x += button_rect.size.width*0.2F;
  button_rect.size.width *= 0.6F;
  edit_button = [NSButton alloc];
  [edit_button initWithFrame:button_rect];
  [edit_button setButtonType:NSMomentaryLightButton];
  [edit_button setAutoresizingMask:NSViewMinXMargin | NSViewWidthSizable];
  [edit_button setTitle:@"Open in Metadata Editor"];
  [edit_button setBordered:YES];
  [edit_button setBezelStyle:NSRoundedBezelStyle];
  [edit_button setEnabled:NO];
  [super_view addSubview:edit_button];
  
  // Adjust the button rectangle so represent a border around the button so
  // that the text panel is positioned correctly.
  button_rect.size.height += 2*button_rect.origin.y;
  button_rect.origin.y = 0;
    
  // Create a titled box to act as a frame for the information display panel.
  text_panel_frame = [NSBox alloc];
  NSRect text_panel_frame_rect = content_rect;
  text_panel_frame_rect.origin.x=browser_rect.origin.x+browser_rect.size.width;
  text_panel_frame_rect.size.width -= browser_rect.size.width;
  text_panel_frame_rect.origin.y=button_rect.origin.y+button_rect.size.height;
  text_panel_frame_rect.size.height -= button_rect.size.height;
  [text_panel_frame initWithFrame:text_panel_frame_rect];
  [text_panel_frame setTitlePosition:NSAboveTop];
  [text_panel_frame setTitleFont:
   [NSFont systemFontOfSize:[NSFont systemFontSize]]];
  [text_panel_frame setBorderType:NSLineBorder];
  [text_panel_frame setAutoresizingMask:NSViewMinXMargin | NSViewWidthSizable |
   NSViewHeightSizable | NSViewMaxYMargin];
  [super_view addSubview:text_panel_frame];

  // Create scroll view for information display panel
  NSRect text_panel_rect = [[text_panel_frame contentView] frame];
  text_panel_rect.origin.y = 0.0F;
  text_panel_rect.origin.x = 0.0F;
  NSScrollView *scroll_view = [NSScrollView alloc];
  [scroll_view initWithFrame:text_panel_rect];
  [scroll_view setBorderType:NSGrooveBorder];
  [scroll_view setHasVerticalScroller:YES];
  [scroll_view setHasHorizontalScroller:NO];
  [scroll_view setAutoresizingMask:
   NSViewMaxXMargin | NSViewWidthSizable | NSViewHeightSizable];
  [[text_panel_frame contentView] addSubview:scroll_view];
  
  // Create information display panel
  NSColor *text_panel_background_colour =
  [NSColor colorWithDeviceRed:0.95F green:0.9F blue:1.0F alpha:1.0F];
  NSRect text_panel_content_rect;
  text_panel_content_rect.origin.x = text_panel_content_rect.origin.y = 0.0F;
  text_panel_content_rect.size = [scroll_view contentSize];
  text_panel = [NSTextView alloc];
  [text_panel initWithFrame:text_panel_content_rect];
  [text_panel setMaxSize:NSMakeSize(FLT_MAX,FLT_MAX)];
  [text_panel setVerticallyResizable:YES];
  [text_panel setHorizontallyResizable:NO];
  [text_panel setAutoresizingMask:NSViewWidthSizable];
  [text_panel setFont:
   [NSFont userFixedPitchFontOfSize:
    [NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
  [[text_panel textContainer]
   setContainerSize:NSMakeSize(text_panel_content_rect.size.width,FLT_MAX)];
  [[text_panel textContainer] setWidthTracksTextView:YES];
  [text_panel setEditable:NO];
  [text_panel setDrawsBackground:YES];
  [text_panel setBackgroundColor:text_panel_background_colour];
  [scroll_view setDocumentView:text_panel];
  
  // Create browser delegate
  delegate = [kdms_metashow_delegate alloc];
  [delegate initWithOwner:renderer];
  [browser setDelegate:delegate];
  [delegate setBrowser:browser
              infoPanel:text_panel
             infoFrame:text_panel_frame
            editButton:edit_button];
  [edit_button setTarget:delegate];
  [edit_button setAction:@selector(edit_button_click:)];
  
  // Release resources held by controls or windows
  [self makeKeyAndOrderFront:self]; // Display the window
  [browser release]; // Held by the `super_view' already
  [edit_button release]; // Held by the `super_view' already
  [text_panel_frame release]; // Held by the `super_view' already
  [scroll_view release]; // Held by the `text_panel_frame' already
  [text_panel release]; // Held by `scroll_view' already
}

/*****************************************************************************/
/*                         kdms_metashow:dealloc                             */
/*****************************************************************************/

- (void) dealloc
{
  if (delegate != nil)
    [delegate release];
  [super dealloc];
}

/*****************************************************************************/
/*               kdms_metashow:activateWithSource:andFileName                */
/*****************************************************************************/

- (void) activateWithSource:(jp2_family_src *)source
                   andFileName:(const char *)name
{
  self->filename = name;
  char *title = new char[strlen(filename)+40];
  sprintf(title,"Metashow %d: %s",identifier,filename);
  [self setTitle:[NSString stringWithUTF8String:title]];  
  delete[] title;
  if (delegate != nil)
    {
      [delegate activateWithSource:source];
      [delegate update_metadata];
    }
}

/*****************************************************************************/
/*                       kdms_metashow:update_metadata                       */
/*****************************************************************************/

- (void) update_metadata
{
  if (delegate != nil)
    [delegate update_metadata];
}

/*****************************************************************************/
/*                          kdms_metashow:deactivate                         */
/*****************************************************************************/

- (void) deactivate
{
  [delegate reset];
}

/*****************************************************************************/
/*                   kdms_metashow:select_matching_metanode                  */
/*****************************************************************************/

- (void) select_matching_metanode:(jpx_metanode)jpx_node
{
  [delegate select_matching_metanode:jpx_node];
}

/*****************************************************************************/
/*                             kdms_metashow:close                           */
/*****************************************************************************/

- (void) close
{
  if (owner != NULL)
    owner->metashow = nil;
  [super close];
}

@end // kdms_metashow

