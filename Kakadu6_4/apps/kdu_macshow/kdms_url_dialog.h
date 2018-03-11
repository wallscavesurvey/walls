/*****************************************************************************/
// File: kdms_url_dialog.h [scope = APPS/MACSHOW]
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
    Defines the `kdms_url_dialog' Objective-C class, which implements a
 dialog for specifying a JPIP server, resource and other optional connection
 parameters.  This class goes hand in hand with the dialog window and
 associated controls, defined in the "url_dialog.nib" file.
******************************************************************************/

#import <Cocoa/Cocoa.h>
#include "kdu_client.h"

// Declared here:
@class kdms_url_dialog;


/*****************************************************************************/
/*                              kdms_url_dialog                              */
/*****************************************************************************/

@interface kdms_url_dialog : NSObject {
  IBOutlet NSWindow *dialog_window;
  IBOutlet NSTextField *server_field;
  IBOutlet NSTextField *request_field;
  IBOutlet NSTextField *proxy_field;
  IBOutlet NSTextField *cache_dir_field;
  IBOutlet NSPopUpButton *transport_popup;
  IBOutlet NSButton *interactive_button;
  IBOutlet NSButton *choose_cache_dir_button;
  IBOutlet NSButton *use_proxy_button;
  IBOutlet NSButton *use_cache_button;
  IBOutlet NSButton *open_button;
  IBOutlet NSButton *cancel_button;
  
  NSString *transport_method;
  NSString *server_name;
  NSString *proxy_name;
  NSString *cache_dir;
  NSString *request_string;
  kdu_client_mode mode;
  bool use_proxy;
  bool use_cache;
  
  NSUserDefaults *defaults;
  NSString *url; // non-nil if the open button was clicked
}

// Actions
- (IBAction) text_edited:(id)sender; // Sent if any text field was edited
- (IBAction) clicked_interactive:(id)sender;
- (IBAction) clicked_choose_cache:(id)sender;
- (IBAction) clicked_use_proxy:(id)sender;
- (IBAction) clicked_use_cache:(id)sender;
- (IBAction) clicked_open:(id)sender;
- (IBAction) clicked_cancel:(id)sender;

// External functions
- (kdms_url_dialog *) init; // Just resets member variables to zero
- (void) setTitle:(NSString *)title;
- (void) dealloc; // Deallocates any allocated strings
- (NSString *) run_modal;
   /* All the work is done here.  Initializes all text boxes and buttons based
      on information found using `NSUserDefaults'.  If the user
      selects the "Open" button, the function updates the defaults
      before returning a string which holds the complete URL,
      to be passed to `kdms_renderer::open_file' -- the returned string is
      already in the auto-release pool, so you don't need to explicitly
      release it.  Otherwise, if the user clicks "Cancel", the function
      returns nil.  The returned string has already had its components
      hex-hex encoded.  In particular, the host name has been hex-hex
      encoded, the resource component of the request string has been
      hex-hex encoded, and any query component of the request string has
      also been hex-hex encoded. */

@end
