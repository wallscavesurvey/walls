/*****************************************************************************/
// File: kdms_properties.mm [scope = APPS/MACSHOW]
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
   Implements the `kdms_properties' class, which provides a dialog for
 displaying JPEG2000 codestream properties in a browser window.
 ******************************************************************************/
#import "kdms_properties.h"

/*===========================================================================*/
/*                             kdms_property_node                            */
/*===========================================================================*/

/*****************************************************************************/
/*                  kdms_property_node::kdms_property_node                   */
/*****************************************************************************/

kdms_property_node::kdms_property_node()
{
  next = NULL;
  node_text = NULL;
  num_children = 0;
  children = last_child = NULL;
  delimiter = '\n';
  max_chars = num_chars = 0;
  buffer = NULL;
}

/*****************************************************************************/
/*                  kdms_property_node::~kdms_property_node                  */
/*****************************************************************************/

kdms_property_node::~kdms_property_node()
{
  if (node_text != NULL)
    delete[] node_text;
  if (buffer != NULL)
    delete[] buffer;
  kdms_property_node *elt;
  while ((elt=children) != NULL)
    {
      children = elt->next;
      delete elt;
    }
}

/*****************************************************************************/
/*                        kdms_property_node::put_text                       */
/*****************************************************************************/

void kdms_property_node::put_text(const char *string)
{
  int new_chars = (int) strlen(string);
  if ((buffer == NULL) || ((num_chars+new_chars) > max_chars))
    {
      max_chars = (num_chars+new_chars)*2;
      char *buf = new char[max_chars+1];
      *buf = '\0';
      if (buffer != NULL)
        {
          strcpy(buf,buffer);
          delete[] buffer;
        }
      buffer = buf;
    }
  num_chars += new_chars;
  strcat(buffer,string);
  if (delimiter == '\0')
    return;
  const char *new_line_ptr = strchr(buffer,delimiter);
  if (new_line_ptr == NULL)
    return;
  
  // Add a child containing the new line of text and retain any
  // characters from the buffer which lie beyond the new-line character.
  int line_length = new_line_ptr - buffer;
  add_child(line_length);
  for (num_chars=0; buffer[line_length+num_chars+1] != '\0'; num_chars++)
    buffer[num_chars] = buffer[line_length+num_chars+1];
  buffer[num_chars] = '\0';
}

/*****************************************************************************/
/*                         kdms_property_node::flush                         */
/*****************************************************************************/

void kdms_property_node::flush(bool end_of_message)
{
  if (num_chars > 0)
    {
      add_child(num_chars);
      num_chars = 0;
      *buffer = '\0';
    }
}

/*****************************************************************************/
/*                       kdms_property_node::add_child                       */
/*****************************************************************************/

void kdms_property_node::add_child(int line_length)
{
  char *sp = buffer;
  while ((line_length > 0) && ((*sp == ' ') || (*sp == '\t')))
    { sp++; line_length--; }
  if (line_length == 0)
    return;
  kdms_property_node *node = new kdms_property_node;
  if (last_child == NULL)
    children = last_child = node;
  else
    last_child = last_child->next = node;
  num_children++;
  node->node_text = new char[line_length+1];
  strncpy(node->node_text,buffer,(size_t) line_length);
  node->node_text[line_length] = '\0';
}


/*===========================================================================*/
/*                          kdms_properties_delegate                         */
/*===========================================================================*/

@implementation kdms_properties_delegate

/*****************************************************************************/
/*                kdms_properties_delegate:initWithCodestream                */
/*****************************************************************************/

-(bool)initWithCodestream:(kdu_codestream)cs
{
  root = NULL;
  browser = nil;
  codestream_params = cs.access_siz();
  
  [super init];
  root = new kdms_property_node;
  
  try {
    // Start by textualizing any comments
    (*root) << "Code-stream comments\n";
    kdu_codestream_comment com;
    while ((com = cs.get_comment(com)).exists())
      {
        int length;
        const char *comtext = com.get_text();
        if (*comtext != '\0')
          {
            root->last_child->set_delimiter('\0');// No delimiters for comments
            (*root->last_child) << comtext;
          }
        else if ((length = com.get_data(NULL,0,INT_MAX)) > 0)
          (*root->last_child) << "<Binary comment with "
                              << length << " bytes>";
        root->last_child->flush();
      }
    
    // Textualize code-stream main header parameters
    (*root) << "Main header\n";
    codestream_params->textualize_attributes(*(root->last_child),-1,-1);
    
    // Textualize tile header parameters
    cs.apply_input_restrictions(0,0,0,0,NULL);
    cs.change_appearance(false,false,false);
    kdu_dims valid_tiles; cs.get_valid_tiles(valid_tiles);
    kdu_coords idx;
    for (idx.y=0; idx.y < valid_tiles.size.y; idx.y++)
      for (idx.x=0; idx.x < valid_tiles.size.x; idx.x++)
        {
          kdu_dims tile_dims;
          cs.get_tile_dims(valid_tiles.pos+idx,-1,tile_dims);
          int tnum = idx.x + idx.y*valid_tiles.size.x;
          (*root) << "Tile " << tnum << " at ("
                  << "y=" << tile_dims.pos.y
                  << ", x=" << tile_dims.pos.x
                  << ", height=" << tile_dims.size.y
                  << ", width=" << tile_dims.size.x << ")";
          root->flush();
          
          // Open and close tile in case the code-stream has not yet been
          // fully parsed -- this may generate an exception if there is a
          // parsing error.
          kdu_tile tile = cs.open_tile(valid_tiles.pos+idx);
          tile.close();
          
          codestream_params->textualize_attributes(*(root->last_child),
                                                   tnum,tnum);
        }
  }
  catch (kdu_exception exc) { // Error occurred while accessing codestream
    return false;
  }
  
  return true;
}

/*****************************************************************************/
/*                    kdms_properties_delegate:set_owner                     */
/*****************************************************************************/

-(void)set_browser:(NSBrowser *)the_browser
{
  browser = the_browser;
  [browser sendActionOn:NSLeftMouseDownMask];
  [browser setTarget:self];
  [browser setAction:@selector(user_click:)];
}

/*****************************************************************************/
/*           kdms_properties_delegate:browser:numberOfRowsInColumn           */
/*****************************************************************************/

-(NSInteger)browser:(NSBrowser *)sender numberOfRowsInColumn:(NSInteger)column
{
  if ((root == NULL) || (column < 0) || (column > 1))
    return 0;
  
  kdms_property_node *elt = root;
  if (column == 1)
    {
      int branch_idx = [sender selectedRowInColumn:0];
      if (branch_idx >= elt->num_children)
        return 0;
      for (elt=elt->children; branch_idx > 0; branch_idx--)
        elt = elt->next;
    }
  return elt->num_children;
}

/*****************************************************************************/
/*        kdms_properties_delegate:browser:willDisplayCell:atRow:column      */
/*****************************************************************************/

-(void)browser:(NSBrowser *)sender willDisplayCell:(id)cell
         atRow:(NSInteger)row column:(NSInteger)column
{
  if ((root == NULL) || (column < 0) || (column > 1))
    return;
  kdms_property_node *elt = root;
  if (column == 1)
    {
      int branch_idx = [sender selectedRowInColumn:0];
      if (branch_idx >= elt->num_children)
        return;
      for (elt=elt->children; branch_idx > 0; branch_idx--)
        elt = elt->next;
    }
  if (row >= elt->num_children)
    return;
  for (elt=elt->children; row > 0; row--)
    elt = elt->next;
  NSString *ns_text = [NSString stringWithUTF8String:elt->node_text];
  
  /* // The following code works in OSX10.5, but not 10.4.  It's not
     // really necessary, so to maximize compatibility, we comment it out.
  unichar delim_buf[1] = {0x204B};  // Paragraph symbol in system font
  NSString *delim = [NSString stringWithCharacters:delim_buf length:1];
  ns_text = [ns_text
             stringByReplacingOccurrencesOfString:@"\n" withString:delim];
  */
  [cell setStringValue:ns_text];
  [cell setLeaf:(column==1)?YES:NO];
}

/*****************************************************************************/
/*                     kdms_properties_delegate:dealloc                      */
/*****************************************************************************/

-(void)dealloc
{
  if (root != NULL)
    delete root;
  [super dealloc];
}

/*****************************************************************************/
/*                   kdms_properties_delegate:user_click                     */
/*****************************************************************************/

-(IBAction)user_click:(NSBrowser *)sender
{
  if ([sender selectedColumn] != 1)
    return;
  int branch_idx = [sender selectedRowInColumn:0];
  int item_idx = [sender selectedRowInColumn:1];
  kdms_property_node *elt = root;
  bool is_comment = (branch_idx == 0);
  if (branch_idx >= elt->num_children)
    return;
  for (elt=elt->children; branch_idx > 0; branch_idx--)
    elt = elt->next;
  if (item_idx >= elt->num_children)
    return;
  for (elt=elt->children; item_idx > 0; item_idx--)
    elt = elt->next;
  const char *selected_node_text = elt->node_text;
  
  // Now we have the text to display.  We will create a formatted string
  // with this text in it, and possibly also a description of the
  // codestream attribute.
  int formatted_line_length = 70;
  kdms_property_node tmp_node; // Use this to collect text to display
  tmp_node.set_delimiter('\0'); // prevent the creation of extra children
  kdu_message_formatter formatter(&tmp_node,formatted_line_length);
  formatter << selected_node_text;
  if ((codestream_params != NULL) && !is_comment)
    {
      const char *attribute_id;
      kdu_params *obj =
        codestream_params->find_string(elt->node_text,attribute_id);
      formatter << "\n\n";
      if (obj != NULL)
        obj->describe_attribute(attribute_id,formatter,true);
    }
  formatter.flush(true); // Access 
  assert(tmp_node.num_children == 1);
  const char *formatted_text = tmp_node.children->node_text;
  
  // Finally, with a text-view for the formatted text and run it modally.
  NSString *formatted_string = [NSString stringWithUTF8String:formatted_text];
  NSFont *fixed_font = [NSFont userFixedPitchFontOfSize:11.0F];
  NSDictionary *attribute_dictionary =
    [NSDictionary dictionaryWithObject:fixed_font forKey:NSFontAttributeName];
  NSAttributedString *attributed_string =
    [[NSAttributedString alloc] initWithString:formatted_string
                                    attributes:attribute_dictionary];
  
  NSString *wnd_title =
    (is_comment)?(@"Codestream comment"):(@"Parameter Attribute");
  kdms_message_window *wnd = [kdms_message_window alloc];
  [wnd initWithTitle:wnd_title andMessage:attributed_string];
  [wnd run_modal];
  [attributed_string release];
}

@end // kdms_properties_delegate

/*===========================================================================*/
/*                              kdms_properties                              */
/*===========================================================================*/

@implementation kdms_properties

/*****************************************************************************/
/*                   kdms_properties:initWithTopLeftPoint                    */
/*****************************************************************************/

- (bool)initWithTopLeftPoint:(NSPoint)point
              the_codestream:(kdu_codestream)cs
            codestream_index:(int)cs_idx
{
  is_running = false;
  
  // Create and initialize the browser delegate.
  delegate = [kdms_properties_delegate alloc];
  if (![delegate initWithCodestream:cs])
    return false;
  
  // Find an initial frame location and size for the window
  NSRect screen_rect = [[NSScreen mainScreen] frame];
  NSRect frame_rect = screen_rect;
  frame_rect.size.height = (float) ceil(frame_rect.size.height * 0.5);
  frame_rect.size.width = (float) ceil(frame_rect.size.width * 0.4);
  frame_rect.origin.x = (float) floor(point.x);
  frame_rect.origin.y = (float) floor(point.y - frame_rect.size.height);
  
  if (frame_rect.origin.x < screen_rect.origin.x)
    frame_rect.origin.x = screen_rect.origin.x;
  if (frame_rect.origin.y < screen_rect.origin.y)
    frame_rect.origin.y = screen_rect.origin.y;
  if ((frame_rect.origin.x+frame_rect.size.width) >
      (screen_rect.origin.x+screen_rect.size.width))
    frame_rect.origin.x = screen_rect.origin.x +
      (screen_rect.size.width - frame_rect.size.width);
  if ((frame_rect.origin.y+frame_rect.size.height) >
      (screen_rect.origin.y+screen_rect.size.height))
    frame_rect.origin.y = screen_rect.origin.y +
      (screen_rect.size.height - frame_rect.size.height);
  
  NSUInteger window_style = NSTitledWindowMask | NSClosableWindowMask |
                            NSResizableWindowMask;  
  NSRect content_rect = [NSWindow contentRectForFrameRect:frame_rect
                                                styleMask:window_style];

  // Create window with an appropriate title
  [super initWithContentRect:content_rect
                   styleMask:window_style
                     backing:NSBackingStoreBuffered defer:NO];
  [super setReleasedWhenClosed:NO];
  char title[80];
  sprintf(title,"Properties for code-stream %d",cs_idx);
  [self setTitle:[NSString stringWithUTF8String:title]];
  
  // Create browser
  NSBrowser *browser = [NSBrowser alloc];
  [browser initWithFrame:content_rect];
  [browser setMaxVisibleColumns:2];
  [browser setTitle:@"Tile/Global" ofColumn:0];
  [browser setHasHorizontalScroller:YES];
  [browser setColumnResizingType:NSBrowserAutoColumnResizing];
  [browser setDelegate:delegate];
  [self setContentView:browser];
  [delegate set_browser:browser];
  
  // Release resources held by controls or windows
  [self makeKeyAndOrderFront:self]; // Display the window
  [browser release]; // Held by the window already
  
  return true;
}

/*****************************************************************************/
/*                          kdms_properties:close                            */
/*****************************************************************************/

- (void)close
{
  if (is_running)
    [NSApp stopModal];
  [super close];
}

/*****************************************************************************/
/*                        kdms_properties:run_modal                          */
/*****************************************************************************/

- (void)run_modal
{
  if (is_running)
    return;
  is_running = true;
  [NSApp runModalForWindow:self];
  is_running = false;
}

/*****************************************************************************/
/*                         kdms_properties:dealloc                           */
/*****************************************************************************/

- (void)dealloc
{
  if (delegate != NULL)
    [delegate release];
  [super dealloc];
}

@end // kdms_properties


/*===========================================================================*/
/*                           kdms_message_window                             */
/*===========================================================================*/

@implementation kdms_message_window

/*****************************************************************************/
/*               kdms_message_window:initWithTitle:andMessage                */
/*****************************************************************************/

- (void)initWithTitle:(NSString *)title
           andMessage:(NSAttributedString *)message
{
  is_running = false;

  NSRect content_rect;
  content_rect.origin.x =
    content_rect.origin.y = 0.0F; // Doesn't matter, because window will be
      // centred when run modally; this always happens if we
      // don't explicitly bring it onto the screen first.
  content_rect.size = [message size];
  content_rect.size.width += 10.0F; // Avoid the risk of wrap-around
  content_rect.size.height += 10.0F; // Avoid the risk of overrun
  NSUInteger window_style = NSTitledWindowMask | NSClosableWindowMask;
  [super initWithContentRect:content_rect
                   styleMask:window_style
                     backing:NSBackingStoreBuffered defer:NO];
  [super setReleasedWhenClosed:NO];
  [super setTitle:title];

  NSColor *bg_colour =
    [NSColor colorWithDeviceRed:0.95F green:0.9F blue:0.85F alpha:1.0F];
  NSTextField *text_field = [NSTextField alloc];
  [text_field initWithFrame:content_rect];
  [text_field setEditable:NO];
  [text_field setDrawsBackground:YES];
  [text_field setBordered:NO];
  [text_field setBackgroundColor:bg_colour];
  NSTextFieldCell *text_field_cell = [text_field cell];
  [text_field_cell setAttributedStringValue:message];
  [super setContentView:text_field];
  [text_field release];
}

/*****************************************************************************/
/*                        kdms_message_window:close                          */
/*****************************************************************************/

- (void)close
{
  if (is_running)
    [NSApp stopModal];
  [super close];
}

/*****************************************************************************/
/*                      kdms_message_window:run_modal                        */
/*****************************************************************************/

- (void)run_modal
{
  if (is_running)
    return;
  is_running = true;
  [NSApp runModalForWindow:self];
  is_running = false;
}

@end // kdms_message_window


