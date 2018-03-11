/*****************************************************************************/
// File: kdms_url_dialog.mm [scope = APPS/MACSHOW]
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
    Implements the `kdms_url_dialog' Objective-C class.
******************************************************************************/
#include "kdu_utils.h"
#include "kdcs_comms.h"
#import "kdms_url_dialog.h"
#import "kdms_controller.h"

/*===========================================================================*/
/*                             kdms_url_dialog                               */
/*===========================================================================*/

@implementation kdms_url_dialog

/*****************************************************************************/
/*                          kdms_url_dialog:init                             */
/*****************************************************************************/

- (kdms_url_dialog *) init
{
  dialog_window = nil;
  server_field = nil;
  request_field = nil;
  proxy_field = nil;
  cache_dir_field = nil;
  transport_popup = nil;
  interactive_button = nil;
  choose_cache_dir_button = nil;
  use_proxy_button = nil;
  use_cache_button = nil;
  open_button = nil;
  cancel_button = nil;
  if (![NSBundle loadNibNamed:@"url_dialog" owner:self])
    { kdu_warning w;
      w << "Could not load NIB file, \"url_dialog.nib\"."; }
  
  return self;
}

/*****************************************************************************/
/*                        kdms_url_dialog:setTitle                           */
/*****************************************************************************/

- (void) setTitle:(NSString *)title
{
  [dialog_window setTitle:title];
}

/*****************************************************************************/
/*                        kdms_url_dialog:run_modal                          */
/*****************************************************************************/

- (NSString *) run_modal
{
  if (!dialog_window)
    return nil;
  transport_method = nil;
  server_name = proxy_name = nil;
  cache_dir = nil;
  request_string = nil;
  use_proxy = use_cache = false;

  defaults = [NSUserDefaults standardUserDefaults];
  switch ([defaults integerForKey:KDMS_KEY_JPIP_MODE]) {
    case ((NSInteger) KDU_CLIENT_MODE_AUTO):
      mode = KDU_CLIENT_MODE_AUTO; break;
    case ((NSInteger) KDU_CLIENT_MODE_INTERACTIVE):
      mode = KDU_CLIENT_MODE_INTERACTIVE; break;
    case ((NSInteger) KDU_CLIENT_MODE_NON_INTERACTIVE):
      mode = KDU_CLIENT_MODE_NON_INTERACTIVE; break;
    default: mode = KDU_CLIENT_MODE_INTERACTIVE; break;
  }
  if (!(server_name = [defaults stringForKey:KDMS_KEY_JPIP_SERVER]))
    server_name = @"";
  if (!(proxy_name = [defaults stringForKey:KDMS_KEY_JPIP_PROXY]))
    proxy_name = @"";
  if (!(cache_dir = [defaults stringForKey:KDMS_KEY_JPIP_CACHE]))
    cache_dir = @"";
  if (!(request_string = [defaults stringForKey:KDMS_KEY_JPIP_REQUEST]))
    request_string = @"";
  if (!(transport_method = [defaults stringForKey:KDMS_KEY_JPIP_TRANSPORT]))
    transport_method = @"http";

  bool have_proxy_name = ([proxy_name length] > 0);
  bool have_cache_dir = ([cache_dir length] > 0);
  bool have_request_string = ([request_string length] > 0);
  bool have_server_name = ([server_name length] > 0);
  
  use_cache = (have_cache_dir &&
               ([defaults boolForKey:KDMS_KEY_JPIP_USE_CACHE] == YES));
  use_proxy = (have_proxy_name &&
               ([defaults boolForKey:KDMS_KEY_JPIP_USE_PROXY] == YES));
  
  [transport_popup removeAllItems];
  [transport_popup addItemWithTitle:@"http"];
  [transport_popup addItemWithTitle:@"http-tcp"];
  [transport_popup addItemWithTitle:@"none"];
  [transport_popup selectItemWithTitle:transport_method];
  
  [[server_field cell] setStringValue:server_name];
  [[proxy_field cell] setStringValue:proxy_name];
  [[request_field cell] setStringValue:request_string];
  [[cache_dir_field cell] setStringValue:cache_dir];
  [interactive_button setState:
   (mode==KDU_CLIENT_MODE_INTERACTIVE)?NSOnState:NSOffState];
  [use_cache_button setState:(use_cache)?NSOnState:NSOffState];
  [use_proxy_button setState:(use_proxy)?NSOnState:NSOffState];
  [use_cache_button setEnabled:have_cache_dir?YES:NO];
  [use_proxy_button setEnabled:have_proxy_name?YES:NO];
  [interactive_button setEnabled:YES];
  [open_button setEnabled:(have_server_name && have_request_string)?YES:NO];
  [cancel_button setEnabled:YES];
  [cache_dir_field setEditable:NO];
  [choose_cache_dir_button setEnabled:YES];
  [dialog_window setDefaultButtonCell:[open_button cell]];
  
  if (!have_server_name)
    [server_field selectText:self];
  else
    [request_field selectText:self];
  
  url = nil;
  [NSApp runModalForWindow:dialog_window];
  if (url != nil)
    [url autorelease];
  return url;
}

/*****************************************************************************/
/*                         kdms_url_dialog:dealloc                           */
/*****************************************************************************/

- (void) dealloc
{
  if (dialog_window != nil)
    {
      [dialog_window close];
      [dialog_window release]; // Releases everything
    }
  [super dealloc];
}

/*****************************************************************************/
/*                         kdms_url_dialog:[ACTIONS]                         */
/*****************************************************************************/

- (IBAction) clicked_interactive:(id)sender
{
  if ([interactive_button state] == NSOnState)
    mode = KDU_CLIENT_MODE_INTERACTIVE;
  else
    mode = KDU_CLIENT_MODE_AUTO;
}

- (IBAction) clicked_use_proxy:(id)sender
{
  proxy_name = [[server_field cell] stringValue];
  if ([proxy_name length] == 0)
    { // Should not have been able to click the button
      use_proxy = false;
      [use_proxy_button setState:NSOffState];
      [use_proxy_button setEnabled:NO];
    }
  else
    use_proxy = ([use_proxy_button state] == NSOnState);
}

- (IBAction) clicked_use_cache:(id)sender
{
  if ([cache_dir length] == 0)
    { // Should not have been able to click the button
      use_cache = false;
      [use_cache_button setState:NSOffState];
      [use_cache_button setEnabled:NO];
    }
  else
    use_cache = ([use_cache_button state] == NSOnState);
}

- (IBAction) clicked_choose_cache:(id)sender
{
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  [panel setTitle:@"Choose JPIP cache directory"];
  [panel setPrompt:@"Choose"];
  [panel setAllowsMultipleSelection:NO];
  [panel setCanChooseFiles:NO];
  [panel setCanChooseDirectories:YES];
  NSString *initial_directory=nil;
  NSString *initial_file=nil;
  if ([cache_dir length] > 0)
    {
      initial_file = [cache_dir lastPathComponent];
      initial_directory = [cache_dir stringByDeletingLastPathComponent];
    }
  if ([panel runModalForDirectory:initial_directory
                            file:initial_file
                            types:nil] == NSOKButton)
    {
      cache_dir = [[panel filenames] objectAtIndex:0];
      [[cache_dir_field cell] setStringValue:cache_dir];
      bool have_cache_dir = ([cache_dir length] > 0);
      use_cache = have_cache_dir;
      [use_cache_button setEnabled:(have_cache_dir)?YES:NO];
      [use_cache_button setState:(use_cache)?NSOnState:NSOffState];
    }
}

- (IBAction) text_edited:(id)sender
{
  bool had_proxy_name = ([proxy_name length] > 0);
  request_string = [[request_field cell] stringValue];
  server_name = [[server_field cell] stringValue];
  proxy_name = [[proxy_field cell] stringValue];
  bool have_request_string = ([request_string length] > 0);
  bool have_server_name = ([server_name length] > 0);
  bool have_proxy_name = ([proxy_name length] > 0);
  [open_button setEnabled:(have_request_string && have_server_name)?YES:NO];
  if (!have_proxy_name)
    use_proxy = false;
  else if (!had_proxy_name)
    use_proxy = true;
  [use_proxy_button setState:(use_proxy)?NSOnState:NSOffState];
  [use_proxy_button setEnabled:(have_proxy_name)?YES:NO];
}

- (IBAction) clicked_cancel:(id)sender
{
  url = nil; // Should be nil already
  [NSApp stopModal];
}

- (IBAction) clicked_open:(id)sender
{
  transport_method = [transport_popup titleOfSelectedItem];
  request_string = [[request_field cell] stringValue];
  server_name = [[server_field cell] stringValue];
  proxy_name = [[proxy_field cell] stringValue];
  bool have_request_string = ([request_string length] > 0);
  bool have_server_name = ([server_name length] > 0);
  if (!(have_request_string && have_server_name))
    return;
  
  [defaults setBool:((use_cache)?YES:NO) forKey:KDMS_KEY_JPIP_USE_CACHE];
  [defaults setBool:((use_proxy)?YES:NO) forKey:KDMS_KEY_JPIP_USE_PROXY];
  [defaults setObject:request_string forKey:KDMS_KEY_JPIP_REQUEST];
  [defaults setObject:server_name forKey:KDMS_KEY_JPIP_SERVER];
  [defaults setObject:proxy_name forKey:KDMS_KEY_JPIP_PROXY];
  [defaults setObject:cache_dir forKey:KDMS_KEY_JPIP_CACHE];
  [defaults setObject:transport_method forKey:KDMS_KEY_JPIP_TRANSPORT];
  [defaults setInteger:((NSInteger) mode) forKey:KDMS_KEY_JPIP_MODE];
  
  const char *name = [server_name UTF8String];
  char *tmp_name = new char[1+strlen(name)];
  const char *port_suffix = strrchr(name,':');
  if ((name[0] == '[') && (port_suffix != NULL))
    { // Check whether this is really a port suffix or not
      port_suffix = strchr(name,']');
      if ((port_suffix != NULL) && (port_suffix[1] == ':'))
        port_suffix++;
      else if ((port_suffix != NULL) && (port_suffix[1] == '\0'))
        port_suffix = NULL;
    }
  if (port_suffix != NULL)
    { // Copy everything before the port suffix to `tmp_name', making it `name'
      strcpy(tmp_name,name);
      tmp_name[port_suffix-name] = '\0';
      name = tmp_name;
    }
  else
    port_suffix = "";
  char *enc_server = new char[3+kdu_hex_hex_encode(name,NULL,NULL,"&?=")];
  if ((name[0] == '[') && (name[strlen(name)-1] == ']'))
    strcpy(enc_server,name);
  else if ((name[0] != '[') &&
           kdcs_sockaddr::test_ip_literal(name,KDCS_ADDR_FLAG_IPV6_ONLY))
    { // Add brackets to IPv6 literals
      enc_server[0] = '[';
      strcpy(enc_server+1,name);
      strcat(enc_server,"]");
    }
  else
    kdu_hex_hex_encode(name,enc_server,NULL,"&?=");
  
  name = [request_string UTF8String];
  const char *query = strrchr(name,'?');
  if ((query != NULL) && (strchr(query,'=') == NULL))
    query = NULL; // Must be a `?' inside the requested filename itself
  char *enc_resource = new char[1+kdu_hex_hex_encode(name,NULL,query,"&?=")];
  kdu_hex_hex_encode(name,enc_resource,query,"&?=");
  if (query == NULL)
    url = [[NSString alloc] initWithFormat:@"jpip://%s%s/%s",
           enc_server,port_suffix,enc_resource];
  else
    {
      char *enc_query = new char[1+kdu_hex_hex_encode(query+1,NULL)];
      kdu_hex_hex_encode(query+1,enc_query);
      url = [[NSString alloc] initWithFormat:@"jpip://%s%s/%s?%s",
             enc_server,port_suffix,enc_resource,enc_query];
      delete[] enc_query;
    }
  delete[] tmp_name;
  delete[] enc_server;
  delete[] enc_resource;
  [NSApp stopModal];
}

@end
