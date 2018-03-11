/*****************************************************************************/
// File: kdms_controller.h [scope = APPS/MACSHOW]
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
   Defines the main controller object for the interactive JPEG2000 viewer,
"kdu_macshow".
******************************************************************************/

#import <Cocoa/Cocoa.h>
#include "kdu_compressed.h"
#include "kdu_client.h"
#include "kdu_clientx.h"

#if (__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1050)
#  define NSInteger int
#  define NSUInteger unsigned int
#  define uint8 unsigned char
#  define CGFloat float
#endif // __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__

// Defined elsewhere:
@class kdms_window;
class kdms_renderer;

// Defined here:
class kdms_frame_presenter;
class kdms_client_notifier;
struct kdms_window_list;
struct kdms_open_file_record;
class kdms_window_manager;
@class kdms_controller;
@class kdms_core_message_controller;

/*****************************************************************************/
/*          Macros Representing Keys in the User Defaults Database           */
/*****************************************************************************/

#define KDMS_KEY_JPIP_TRANSPORT @"JPIP-CLIENT-TRANSPORT"
#define KDMS_KEY_JPIP_SERVER    @"JPIP-CLIENT-SERVER"
#define KDMS_KEY_JPIP_PROXY     @"JPIP-CLIENT-PROXY"
#define KDMS_KEY_JPIP_REQUEST   @"JPIP-CLIENT-REQUEST"
#define KDMS_KEY_JPIP_CACHE     @"JPIP-CLIENT-CACHE-DIRECTORY"
#define KDMS_KEY_JPIP_USE_PROXY @"JPIP-CLIENT-USE_PROXY"
#define KDMS_KEY_JPIP_USE_CACHE @"JPIP-CLIENT-USE-CACHE"
#define KDMS_KEY_JPIP_MODE      @"JPIP-CLIENT-MODE"

/*****************************************************************************/
/*                              EXTERNAL FUNCTIONS                           */
/*****************************************************************************/

extern bool kdms_compare_file_pathnames(const char *name1, const char *name2);
  /* Returns true if `name1' and `name2' refer to the same file.  If a simple
   string comparison returns false, the function converts both names to
   file system references, if possible, and performs the comparison on
   the references.  This helps minimize the risk of overwriting an existing
   file which the application is using. */

/*****************************************************************************/
/* CLASS                         kdms_frame_presenter                        */
/*****************************************************************************/

class kdms_frame_presenter {
      /* There is a unique frame presenter for each window managed by the
       `kdms_window_manager' object. */
public: // Member functions used by the `window_manager'
  kdms_frame_presenter(kdms_window_manager *window_manager, kdms_window *wnd,
                       int min_drawing_interval);
    /* The `min_drawing_interval' argument dictates the maximum rate at which
     the view will be painted.  The `draw_pending_frame' function is called
     regularly from a timed loop in the presentation thread.  The function
     will not actually paint any frame data to the screen if it has
     previously done so in any of the I-1 previous calls, where I is the
     value of `min_drawing_interval'. */
  ~kdms_frame_presenter();
  bool draw_pending_frame(CFRunLoopRef main_app_run_loop);
    /* Called from the presentation thread's run-loop at a regular rate.
     This function draws any frame previously registered by `present_frame',
     returning true if anything is drawn.  If requested, the main application's
     run-loop is sent a wakeup message after frame drawing completes. */
public: // Member functions used by the application thread
  void present_frame(kdms_renderer *painter, int frame_idx)
  {
    wake_app_once_painting_completes=false; // Only the app can write this flag
    this->painter = painter;
    this->frame_to_paint = frame_idx;
  }
    /* This function indicates that a frame (with the indicated index) is
     available for painting.  If the frame has already been painted, this call
     has no effect.  Otherwise, the presentation thread invokes
     the `painter->paint_view_from_presentation_thread' function at the
     next reasonable opportunity, considering reasonable maximum painting
     rates and the screen refresh rate. */
  bool disable();
    /* This function attempts to cancel any outstanding frame presentation
     request, so that the head of the `kdu_region_compositor' object's
     composition queue can be manipulated by calls to
     `kdu_region_compositor::pop_composition_buffer'.
     If the presentation thread is in the process of painting a frame,
     the function returns false immediately, without blocking.  In this case,
     it is not yet safe for the caller to manipulate the composition
     queue, but the object promises to wake up the main application thread
     once the presentation process is complete, so that the caller's
     `kdms_window::on_idle' function will eventually be called and get
     a chance to call the present function again. */
  void reset();
    /* Similar to `disable', except that this function blocks the caller
     until frame presentation can be firmly cancelled.  Moreover, once this
     is done, the `last_frame_painted' member is reset to a ridiculous value,
     so that any subsequent calls to `present_frame' will not be blocked from
     presenting the frame they provide. */
private: // Data
  kdms_window_manager *window_manager;
  kdms_window *window;
  NSGraphicsContext *graphics_context;
  kdu_mutex drawing_mutex; // Locked while the frame is being drawn
private: // State variables written only by the `app' thread
  kdms_renderer *painter; // If NULL, there is nothing to paint 
  bool wake_app_once_painting_completes; // If `disable' call failed
  int frame_to_paint; // Index of the frame waiting to be painted
private: // State variables written only by the presentation thread
  int min_drawing_interval; // Number of times `draw_pending_frame' must be
           // called after a view is painted, before it will be painted again.
  int drawing_downcounter; // Implements counter for `min_drawing_interval'
  int last_frame_painted; // Used to avoid painting a frame multiple times
};

/*****************************************************************************/
/*                             kdms_client_notifier                          */
/*****************************************************************************/

class kdms_client_notifier : public kdu_client_notifier {
  public: // Member functions
    kdms_client_notifier(kdms_controller *controller,
                         id client_identifier)
      {
        this->controller = controller;
        this->client_identifier = client_identifier;
        have_unhandled_notification = false;
      }
    void notify();
    void handling_notification()
      { // Called from main thread when about to notify application windows
        have_unhandled_notification = false;
      }
  private: // Data
    kdms_controller *controller;
    id client_identifier;
    bool have_unhandled_notification; // used to minimize queued notifications
};

/*****************************************************************************/
/* STRUCT                         kdms_window_list                           */
/*****************************************************************************/

struct kdms_window_list {
  kdms_window *wnd;
  int window_identifier; // See `kdms_window_manager::get_window_identifier'
  const char *file_or_url_name; // Used as an identifier or title
  double wakeup_time; // -ve if no wakeup is scheduled
  kdms_frame_presenter *frame_presenter;
  bool window_empty;
  bool window_placed;
  kdms_window_list *next;
  kdms_window_list *prev;
};

/*****************************************************************************/
/* STRUCT                     kdms_open_file_record                          */
/*****************************************************************************/

struct kdms_open_file_record {
  kdms_open_file_record()
    {
      open_pathname = open_url = save_pathname = NULL; retain_count = 0;
      jpip_client = NULL; jpx_client_translator = NULL;
      client_notifier = NULL; client_identifier = nil;
      next = NULL;
    }
  ~kdms_open_file_record()
    {
      if (open_pathname != NULL) delete[] open_pathname;
      if (save_pathname != NULL) delete[] save_pathname;
      if (open_url != NULL) delete[] open_url;
      if (jpip_client != NULL)
        {
          jpip_client->close(); // So we can remove the context translator
          jpip_client->install_context_translator(NULL);
        }
      if (jpx_client_translator != NULL) delete jpx_client_translator;
      if (jpip_client != NULL) delete jpip_client;
      if (client_notifier != NULL) delete client_notifier;
      if (client_identifier) [client_identifier release];
    }
  int retain_count;
  char *open_pathname; // Non-NULL if this record represents a local file
  char *open_url; // Non-NULL if this record represents a URL served via JPIP
  char *save_pathname; // Non-NULL if there is a valid saved file which needs
                       // to replace the existing file before closing.
  kdu_client *jpip_client; // Non-NULL if and only if `open_url' is non-NULL
  kdu_clientx *jpx_client_translator;
  kdms_client_notifier *client_notifier;
  id client_identifier;
  kdms_open_file_record *next;
};

/*****************************************************************************/
/* CLASS                        kdms_window_manager                          */
/*****************************************************************************/

class kdms_window_manager {
public: // Startup/shutdown member functions
  kdms_window_manager(kdms_controller *controller);
    /* Note: it is not safe to delete this object explicitly; since the
       `menuAppQuit' message may be received in the controller while
       `run_loop_callback' is testing for user events which need
       to be interleaved with decompression processing.  Thus,
       the `kdms_controller::menuAppQuit' function should terminate
       the application without explicitly deleting the window manager,
       leaving the operating system to clean it up. */
  void configure_presentation_manager();
    /* Called from the presentation thread, right after it is launched,
       before its run-loop is entered.  This gives the object a chance to
       install a timer and callback function to manage period frame
       presentation events. */
  bool application_can_terminate();
    /* Sends `application_can_terminate' messages to each window in turn
       until a false return is obtained, in which case the function returns
       false.  If all windows are happy to terminate, the function returns
       true. */
  void send_application_terminating_messages();
    /* Sends `application_terminating' messages to all windows. */
// ---------------------------------------------------------------------------
public: // Window list manipulation functions
  void add_window(kdms_window *wnd);
  void remove_window(kdms_window *wnd);
  bool should_app_terminate() { return app_should_terminate; }
    /* Returns true if the window list has become empty.  Called when the
       app is idle so that all outstanding processing has already been done. */
  int get_access_idx(kdms_window *wnd);
    /* Returns the position of the supplied window within the list of
       all active windows -- if this index is passed to `access_window',
       the same window will be returned.  Returns -1 if, for some reason,
       the window turns out not to be in the list -- may have been removed
       with `remove_window' or never added by `add_window'. */
  kdms_window *access_window(int idx);
    /* Retrieve the idx'th window in the list, starting from idx=0. */
  int get_window_identifier(kdms_window *wnd);
    /* Retrieves the integer identifier which is associated with the
       indicated window (0 if the window cannot be found).  The identifier
       is currently set equal to the number of `add_window' calls which
       occurred prior to and including the one which added this window. */
  bool place_window(kdms_window *wnd, NSSize frame_size,
                    bool do_not_place_again=false,
                    bool placing_first_empty_window=false);
    /* Place the window at a good location.  If `do_not_place_again' is true
       and the window has been placed before, the function returns false,
       doing nothing.  Otherwise, the function always returns true.  If
       `placing_first_empty_window' is true, the function places the window
       but does not update any internal state, so the window can be placed
       again when something is actually opened; this is sure to leave the
       window in the same position where possible, which is the most
       desirable scenario. */
  void reset_placement_engine();
    /* Resets the placement engine so that new window placement operations
       will start again from the top-left corner of the screen. */
  void declare_window_empty(kdms_window *wnd, bool is_empty);
    /* Called with `is_empty'=false when the window's
       `kdms_renderer::open_file' function is used to open a new file/URL.
       Called with `is_empty'=true when the window's
       `kdms_renderer::close_file' function is used to close a file/URL.
       Windows which are empty can be re-used by controller-wide
       operations which would otherwise create a new window. */
  kdms_window *find_empty_window();
    /* Returns NULL if there are no empty windows. */
// ---------------------------------------------------------------------------
public: // Menu action broadcasting functions
  kdms_window *get_next_action_window(kdms_window *caller);
    /* Called from within window-specific menu action handlers to determine
       the next window, if any, to which the menu action should be passed.
       The function returns nil if there is none (the normal situation).  The
       function may be called recursively.  It knows how to prevent indefinite
       recursion by identifying the key window (the one which should have
       received the menu action call in the first place.  If there is no
       key window when the function is called and the caller is not the
       key window, the function always returns nil for safety. */
  void set_action_broadcasting(bool broadcast_once,
                               bool broadcast_indefinitely);
    /* This function is used to configure the behavior of calls to
       `get_next_action_window'.  If both arguments are false, the latter
       function will always return nil.  If `broadcast_once' is false, the
       `get_next_action_window' function will return each window in turn for
       one single cycle.  If `broadcast_indefinitely' is true, the function
       will work to broadcast all menu actions to all windows. */
  bool is_broadcasting_actions_indefinitely()
    { return broadcast_actions_indefinitely; }
// ---------------------------------------------------------------------------
public: // Timer scheduling functions
  void schedule_wakeup(kdms_window *wnd, double time);
    /* Schedules a wakeup call for the supplied window at the indicated
       time.  A `wakeupScheduledFor:occurredAt' message will be sent to `wnd'
       at this time (or shortly after) passing the scheduled `time', together
       with the time at which the wakeup message is actually sent.  At most
       one wakeup time may be maintained for each window, so this function
       may change any previously installed wakeup time.  All wakeup times are
       managed internally to this object by a single run-loop timer object,
       so as to minimize overhead and encourage synchronization of frame
       playout times where there are multiple windows.
          If the `time' has already passed, this function will not invoke
       `[wnd wakeup]' immediately.  This is a safety measure to prevent
       unbounded recursion in case `schedule_wakeup' is invoked from within
       the `wakeup' function itself (a common occurrence).  Instead, the
       `wakeup' call will be made once the thread's run-loop gets control
       back again and invokes the `timer_callback' function.
          If the `time' argument is -ve, this function simply cancels
       any pending wakeup call for the window. */
  kdms_frame_presenter *get_frame_presenter(kdms_window *wnd);
    /* Returns the frame presenter object associated with the window, for
       use in presenting live video frames efficiently, in the background
       presentation thread. */
  void broadcast_playclock_adjustment(double delta);
    /* Called from any window in playback mode, which is getting behind its
       desired playback rate.  This function makes adjustments to all window's
       play clocks so that they can remain roughly in sync. */
// ---------------------------------------------------------------------------
public: // Management of files, URL's and JPIP clients
  const char *retain_open_file_pathname(const char *file_pathname,
                                        kdms_window *wnd);
    /* This function declares that a window (identified by `wnd') is about
       to open a file whose name is supplied as `file_pathname'.  If the file
       is already opened by another window, its retain count is incremented.
       Otherwise, a new internal record of the file pathname is made.  In any
       case, the returned pointer corresponds to the internal file pathname
       buffer managed by this object, which saves the caller from having to
       copy the file to its own persistent storage. */
  void release_open_file_pathname(const char *file_pathname,
                                  kdms_window *wnd);
    /* Releases a file previously retained via `retain_open_file_pathname'.
       If a temporary file has previously been used to save over an existing
       open file, and the retain count reaches 0, this function deletes the
       original file and replaces it with the temporary file.  The `wnd'
       argument identifies the window which is releasing the file. */
  const char *get_save_file_pathname(const char *file_pathname);
    /* This function is used to avoid overwriting open files when trying
       to save to an existing file.  The pathname of the file you want to
       save to is supplied as the argument.  The function either returns
       that same pathname (without copying it to an internal buffer) or else
       it returns a temporary pathname that should be use instead, remembering
       to move the temporary file into the original file once its retain
       count reaches zero, as described above in connection with the
       `release_open_file_pathname' function. */
  void declare_save_file_invalid(const char *file_pathname);
    /* This function is called if an attempt to save failed.  You supply
       the same pathname supplied originally by `get_save_file_pathname' (even
       if that was just the pathname you passed into the function).  The file
       is deleted and, if necessary, any internal reminder to copy that file
       over the original once the retain count reaches zero is removed. */
  int get_open_file_retain_count(const char *file_pathname);
    /* Returns the file's retain count. */
  bool check_open_file_replaced(const char *file_pathname);
    /* Returns false if the supplied file pathname already has an alternate
       save pathname, which will be used to replace the file once its retain
       count reaches zero, as explained in connection with the
       `release_open_file_pathname' function. */
  const char *retain_jpip_client(const char *server, const char *request,
                                 const char *url, kdu_client * &client,
                                 int &request_queue_id, kdms_window *wnd);
    /* This function is used when a window (identified by `wnd') needs to
       open a JPIP connection to an image on a remote server.  The first thing
       to understand is that when multiple windows want to access the same
       remote image, it is much more efficient for them to share a single cache
       and a single `kdu_client' object, opening multiple request queues
       within the client.  Where possible, this translates into the client
       opening multiple parallel JPIP channels to the server.  To facilitate
       this, the `kdu_client' object is not created within the window (or its
       associated `kdms_renderer' object), but within the present function.
       If a client already exists which has an alive connection to the server
       for the same resource, the function returns a reference to the existing
       `client' to which it has opened a new request queue, whose id is
       returned via `request_queue_id' -- this is the value supplied in calls
       to `kdu_client::post_window' and `kdu_client::disconnect' amongst other
       member functions.
       [//]
       In the special case of a one-time request, the function allows any
       number of windows to associate themselves with the client, returning
       a `request_queue_id' value of 0 in every case, as if each of them
       were the one which originally called `kdu_client::connect'.  This
       is fine because none of them are allowed to alter the window of
       interest for clients opened with a one-time request.
       [//]
       The remote image can be identified either through a non-NULL
       `url' string, or through non-NULL `server' and `request' strings.
       In the former case, the server name and request component's of the
       URL are separated by the function.  In either case, the function
       returns a reference to an internally created URL string, which could
       be used as the `url' in a future call to this function to retain the
       same client again, opening another request queue on it.  The returned
       string is usually also employed as the window's title.  This string
       remains valid until the retain count for the JPIP client reaches zero
       (through matching calls to `release_jpip_client'), at which point the
       client is closed and deleted.
       [//]
       The present function also arranges for a `kdu_client_notifier'
       object to be created and associated with the client, which in turn
       arranges for a `client_notification' message to be sent to `wnd'
       whenever the client has something to notify the application about.
       If multiple windows are sharing the same client, they will all receive
       notification messages.  The `client_notification' messages are
       delivered (asynchonously) within the application's main thread, even
       through they are generated from within the client's separate
       network management thread.
       [//]
       Note that this function may indirectly generate an error through
       `kdu_error' if there is something wrong with the `server',
       `request' or `url' arguments, so the caller needs to be prepared to
       catch the resulting integer exception.
    */
  void use_jpx_translator_with_jpip_client(kdu_client *client);
    /* This function is called if you discover that the resource being
       fetched using this client represents a JPX image resource.  The
       function installs a `kdu_clientx' client-translator for the client
       if one is not already installed. */
  void release_jpip_client(kdu_client *client, kdms_window *wnd);
    /* Releases access to a JPIP client (`kdu_client' object) obtained by
       a previous call to `retain_jpip_client'.  The caller should have
       already invoked `kdu_client::disconnect' (usually with a long timeout
       and without waiting).  The present function invokes the
       `kdu_client::disconnect' function again if and only if the number
       of windows using the client drops to 0; in this case a much smaller
       timeout is used to forcibly disconnect everything if the server is
       too slow; the function also waits for disconnection (or timeout)
       to complete. */
  void handle_client_notification(id client_identifier);
    /* Invoked by `kdms_controller:notificationFromJpipClient' when it
       handles the message initiated by `kdms_client_notifier::notify'
       within the main thread's run-loop. */
// ---------------------------------------------------------------------------
public: // Static callback functions
  static void
    run_loop_callback(CFRunLoopObserverRef observer,
                      CFRunLoopActivity activity, void *info);
  static void
    timer_callback(CFRunLoopTimerRef timer, void *info);
  static void
    presentation_timer_callback(CFRunLoopTimerRef timer, void *info);
// ---------------------------------------------------------------------------
private: // Helper functions
  void install_next_scheduled_wakeup();
    /* Scans the window list to find the next window which requires a wakeup
       call.  If the time has already passed, executes its wakeup function
       immediately and continue to scan; otherwise, sets the timer for future
       wakeup.  This function attempts to execute any pending wakeup calls
       in order. */
// ---------------------------------------------------------------------------
private: // Links
  kdms_controller *controller;
private: // Window management
  kdms_window_list *windows; // Window identifiers strictly increasing in list
  int next_window_identifier;
  kdms_window_list *next_idle_window; /* Points to next window to scan for
     idle-time processing.  NULL if we should start scanning from the
     start of the list next time `run_loop_callback' is called. */
  kdms_window *last_known_key_wnd; // nil if none is known to be key
  bool broadcast_actions_once;
  bool broadcast_actions_indefinitely;
  bool app_should_terminate; // Set if window list becomes empty
private: // Auto-placement information; these quantities are expressed in
         // screen coordinates, using the "vertical starts at top" convention
  kdu_coords next_window_pos; // For next window to be placed on current row
  int next_window_row; // Start of next row of windows
  kdu_coords cycle_origin; // Origin of current placement cycle
private: // Information for timed wakeups
  kdms_window_list *next_window_to_wake;
  bool will_check_best_window_to_wake; // This flag is set while in (or about
         // to call `install_next_scheduled_wakeup'.  In this case, a call
         // to `schedule_wakeup' should not try to determine the next window
         // to wake up by itself.
  CFRunLoopTimerRef timer;
private: // Run-loop observer
  CFRunLoopObserverRef main_observer;
private: // Data required to manage the presentation thread
  CFRunLoopRef main_app_run_loop; // So frame presenters can wake the main app
  CFRunLoopTimerRef presentation_timer;
  int min_presentation_drawing_interval;
  kdu_mutex window_list_change_mutex; // Locked by the main thread before
         // changing the window list.  Locked by the presentation thread
         // before scanning the window list for windows whose frame presenter
         // needs to be serviced.
private: // Data required to safely manage open files in the face of saving
  kdms_open_file_record *open_file_list;
};

/*****************************************************************************/
/* INTERFACE                      kdms_controller                            */
/*****************************************************************************/

@interface kdms_controller : NSObject
{
  NSCursor *cursors[2];
  kdms_window_manager *window_manager;
}

// ----------------------------------------------------------------------------
// Startup member functions
- (void)awakeFromNib;
- (void)presentationThreadEntry:(id)param;

// ----------------------------------------------------------------------------
// Functions used to handle Apple events (typically from launch services)
- (BOOL)application:(NSApplication *)app openFile:(NSString *)filename;
- (void)application:(NSApplication *)app openFiles:(NSArray *)filenames;
- (void)handleGetURLEvent:(NSAppleEventDescriptor *)event
           withReplyEvent:(NSAppleEventDescriptor *)replyEvent;

// ----------------------------------------------------------------------------
// Messages performed in the main thread on behalf of other threads
- (void)notificationFromJpipClient:(id)client_identifier;

// ----------------------------------------------------------------------------
// Menu functions
- (IBAction)menuWindowNew:(NSMenuItem *)sender;
- (IBAction)menuWindowArrange:(NSMenuItem *)sender;
- (IBAction)menuWindowBroadcastOnce:(NSMenuItem *)sender;
- (IBAction)menuWindowBroadcastIndefinitely:(NSMenuItem *)sender;
- (IBAction)menuFileOpenNewWindow:(NSMenuItem *)sender;
- (IBAction)menuFileOpenUrlNewWindow:(NSMenuItem *)sender;
- (IBAction)menuAppQuit:(NSMenuItem *)sender;

- (BOOL) validateMenuItem:(NSMenuItem *)menuitem;
 
@end

/*****************************************************************************/
/* INTERFACE                kdms_core_message_controller                     */
/*****************************************************************************/

@interface kdms_core_message_controller : NSObject
{
  kdu_message_queue *queue;
}
- (kdms_core_message_controller *)init:(kdu_message_queue *)msg_queue;
- (void)pop_messages;
@end
