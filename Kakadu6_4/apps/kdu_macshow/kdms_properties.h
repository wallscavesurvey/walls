/*****************************************************************************/
// File: kdms_properties.h [scope = APPS/MACSHOW]
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
   Defines the `kdms_properties' class, which provides a dialog for
 displaying JPEG2000 codestream properties in a browser window.
 *****************************************************************************/
#import <Cocoa/Cocoa.h>
#import "kdms_controller.h"
#include "kdu_compressed.h"

@class kdms_properties;

/*****************************************************************************/
/*                             kdms_property_node                            */
/*****************************************************************************/

struct kdms_property_node : kdu_message {
  /* This object is used to collect message text while textualizing
     `kdu_params' objects.  Each time a new line is collected, as evidenced
     by the appearance of `delimiter' character in the input stream or a
     `flush' call, the accumulated text, if any, is used to add a new child
     to the current node to store the accumulated line of text (without the
     delimiter character).  The delimiter character defaults to `'\n',
     but may be changed using the `set_delimiter' function. */
  kdms_property_node();
  ~kdms_property_node();
  void set_delimiter(char delimiter) { this->delimiter = delimiter; }
    /* Sets the delimiter character which is used to separate the text
       which is to be associated with distinct child nodes when received
       via the `kdu_message::put_text' function.  If `delimiter' is `\0',
       there is no delimiter and a child node will not be created until
       the next call to `flush'. */
public: // Data for this node
  kdms_property_node *next; // Points to next sibling
  char *node_text;
public: // Links to this node's children
  int num_children;
  kdms_property_node *children;
  kdms_property_node *last_child; // so we can easily add a new one 
private: // Functions used to implement `kdu_message' functionality
  void add_child(int line_length); // Adds a new child whose text comes
          // from the the first `line_length' characters of the `buffer'.
  void put_text(const char *string);
public:
  void flush(bool end_of_message=false);
private: // Data members used to implement the `kdu_message' functionality
  char delimiter;
  int num_chars, max_chars;
  char *buffer;
};

/*****************************************************************************/
/*                         kdms_properties_delegate                          */
/*****************************************************************************/

@interface kdms_properties_delegate : NSResponder {
  kdms_property_node *root; // There is only one root node, not a list.
  NSBrowser *browser;
  kdu_params *codestream_params;
}
-(bool)initWithCodestream:(kdu_codestream)cs;
  /* Extracts all the data required from the codestream, retaining no
   further reference to it or to any of the interfaces it offers.
   Returns false if an exception is generated while trying to open
   a tile. */
-(void)set_browser:(NSBrowser *)browser;
-(void)browser:(NSBrowser *)sender willDisplayCell:(id)cell
         atRow:(NSInteger)row column:(NSInteger)column;
-(NSInteger)browser:(NSBrowser *)sender numberOfRowsInColumn:(NSInteger)column;
-(void)dealloc;
-(IBAction)user_click:(NSBrowser *)sender;

@end // kdms_properties_delegate

/*****************************************************************************/
/*                              kdms_properties                              */
/*****************************************************************************/

@interface kdms_properties : NSWindow {
  kdms_properties_delegate *delegate;
  bool is_running;
}

- (bool)initWithTopLeftPoint:(NSPoint)point
              the_codestream:(kdu_codestream)codestream
            codestream_index:(int)idx;
    /* The `point' argument identifies a preferred location for the top-left
       corner of the codestream properties window.  Of course, if the window
       does not fit, the location must be adjusted.
         Extracts all the information required from the supplied codestream,
       after which he caller is free to use it in any way desired.
         Returns false if an exception occurs while trying to access some
       required information from the codestream.  In this case, the caller
       should close the source file, since it is corrupted.
     */
- (void)run_modal;
- (void)close; // Overrides NSWindow:close to stop the modal event loop
- (void)dealloc;

@end // kdms_properties

/*****************************************************************************/
/*                             kdms_message_window                           */
/*****************************************************************************/

@interface kdms_message_window : NSWindow {
  bool is_running;
}
- (void)initWithTitle:(NSString *)title
           andMessage:(NSAttributedString *)message;
- (void)run_modal;
- (void)close; // Overrides NSWindow:close to stop the modal event loop

@end // kdms_message_window


