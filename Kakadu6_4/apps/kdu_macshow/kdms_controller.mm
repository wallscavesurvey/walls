/*****************************************************************************/
// File: kdms_controller.mm [scope = APPS/MACSHOW]
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
 Implements the `kdms_controller' Objective-C class.
 *****************************************************************************/
#import "kdms_controller.h"
#import "kdms_window.h";
#import "kdms_url_dialog.h"

/*****************************************************************************/
/* IMPLEMENTATION          kd_core_message_controller                        */
/*****************************************************************************/

@implementation kdms_core_message_controller

/*****************************************************************************/
/*                    kdms_core_message_controller::init                     */
/*****************************************************************************/

- (kdms_core_message_controller *)init:(kdu_message_queue *)msg_queue
{
  self->queue = msg_queue;
  return self;
}

/*****************************************************************************/
/*               kdms_core_message_controller::pop_messages                  */
/*****************************************************************************/

- (void)pop_messages
{
  while (queue->pop_message() != NULL);
}

@end // End of `kdms_core_message_controller' implementation
  

/*****************************************************************************/
/* CLASS                   kd_core_message_collector                         */
/*****************************************************************************/

class kd_core_message_collector : public kdu_message_queue {
  /* This object is used to implement error and warning message services
     which are registered globally for use by all threads which may
     use `kdu_error' or `kdu_warning'.  The object is based on the
     functionality offered by the versatile Kakadu core message handling
     class `kdu_message_queue', providing the required application specific
     override for `kdu_message_queue::pop_message' so that all message display
     are handled in the application's main thread. */
  public: // Member functions
    kd_core_message_collector(bool for_errors)
      {
        this->configure(16,true,for_errors,KDU_ERROR_EXCEPTION);
        this->in_modal_display = false;
        this->for_errors = for_errors;
        this->message_controller = [[kdms_core_message_controller alloc]
                                    init:this];
      }
    ~kd_core_message_collector()
      {
        [message_controller release];
      }
    const char *pop_message()
      {
        if (![NSThread isMainThread])
          {
            [message_controller
             performSelectorOnMainThread:@selector(pop_messages)
             withObject:nil
             waitUntilDone:NO];
            return NULL;
          }
        const char *text = NULL;
        if ((!in_modal_display) &&
            ((text=kdu_message_queue::pop_message()) != NULL))
          {
            in_modal_display = true;
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"OK"];
            [alert setMessageText:[NSString stringWithUTF8String:text]];
            if (for_errors)
              {
                [alert setAlertStyle:NSCriticalAlertStyle];
                [alert setInformativeText:
                  @"Error message from Kakadu core system"];
              }
            else
              {
                [alert setAlertStyle:NSWarningAlertStyle];
                [alert setInformativeText:
                  @"Warning message from Kakadu core system"];
              }
            [alert runModal];
            [alert release];
            in_modal_display = false;
          }
        return text;
      }
  private: // Data
    bool in_modal_display;
    bool for_errors;
    kdms_core_message_controller *message_controller;
  };

static kd_core_message_collector warn_collector(false);
static kd_core_message_collector err_collector(true);
static kdu_message_formatter warn_formatter(&warn_collector,50);
static kdu_message_formatter err_formatter(&err_collector,50);


/*===========================================================================*/
/*                            EXTERNAL FUNCTIONS                             */
/*===========================================================================*/

bool kdms_compare_file_pathnames(const char *name1, const char *name2)
{
  if ((name1 == NULL) || (name2 == NULL))
    return false;
  if (strcmp(name1,name2) == 0)
    return true;
  FSRef ref1, ref2;
  if ((FSPathMakeRef((const uint8 *) name1,&ref1,NULL) == 0) &&
      (FSPathMakeRef((const uint8 *) name2,&ref2,NULL) == 0))
    return (FSCompareFSRefs(&ref1,&ref2) == noErr);
  return false;
}

/*===========================================================================*/
/*                           kdms_frame_presenter                            */
/*===========================================================================*/

/*****************************************************************************/
/*                kdms_frame_presenter::kdms_frame_presenter                 */
/*****************************************************************************/

kdms_frame_presenter::kdms_frame_presenter(kdms_window_manager *manager,
                                           kdms_window *wnd,
                                           int min_drawing_interval)
{
  this->window_manager = manager;
  this->window = wnd;
  this->min_drawing_interval = min_drawing_interval;
  this->drawing_downcounter = 1; // Can draw immediately
  this->graphics_context = nil;
  drawing_mutex.create();
  painter = NULL;
  frame_to_paint = last_frame_painted = (1<<31);
  wake_app_once_painting_completes = false;
}

/*****************************************************************************/
/*                kdms_frame_presenter::~kdms_frame_presenter                */
/*****************************************************************************/

kdms_frame_presenter::~kdms_frame_presenter()
{
  assert(painter == NULL); // `disable' should have been called already
  drawing_mutex.destroy();
  if (graphics_context)
    [graphics_context release];
}

/*****************************************************************************/
/*                      kdms_frame_presenter::disable                        */
/*****************************************************************************/

bool kdms_frame_presenter::disable()
{
  wake_app_once_painting_completes = false; // Only the app thread can write
            // this flag, so we should cancel any previously installed request
            // for wakeup, before going any further.
  if (painter == NULL)
    return true; // Already disabled
  wake_app_once_painting_completes = true; // Set the flag to true, to ensure
        // safe behaviour in the event of race conditions.
  if (drawing_mutex.try_lock())
    {
      wake_app_once_painting_completes = false;
      painter = NULL;
      drawing_mutex.unlock();
    }
  return (painter == NULL);
}

/*****************************************************************************/
/*                       kdms_frame_presenter::reset                         */
/*****************************************************************************/

void kdms_frame_presenter::reset()
{
  drawing_mutex.lock();
  painter = NULL;
  frame_to_paint = last_frame_painted = (1<<31);
  wake_app_once_painting_completes = false;
  drawing_mutex.unlock();
}

/*****************************************************************************/
/*                 kdms_frame_presenter::draw_pending_frame                  */
/*****************************************************************************/

bool kdms_frame_presenter::draw_pending_frame(CFRunLoopRef main_app_run_loop)
{
  if (drawing_downcounter > 1)
    {
      drawing_downcounter--;
      return false;
    }
  kdms_renderer *target = painter;
  if ((target == NULL) || (last_frame_painted == frame_to_paint))
    {
      if (wake_app_once_painting_completes)
        CFRunLoopWakeUp(main_app_run_loop); // Previous wakeup might have
                                            // arrived before app went to sleep
      return false;
    }  
  if (!graphics_context)
    {
      graphics_context = [NSGraphicsContext graphicsContextWithWindow:window];
      [graphics_context retain];
    }
  drawing_mutex.lock();
  target = painter; // Check again to make sure we have something to paint
  if (target != NULL)
    {
      last_frame_painted = frame_to_paint;
      target->paint_view_from_presentation_thread(graphics_context);
      drawing_downcounter = min_drawing_interval;
      if (wake_app_once_painting_completes)
        CFRunLoopWakeUp(main_app_run_loop);
    }
  drawing_mutex.unlock();
  return true;
}

/*===========================================================================*/
/*                            kdms_client_notifier                           */
/*===========================================================================*/

/*****************************************************************************/
/*                        kdms_client_notifier::notify                       */
/*****************************************************************************/

void kdms_client_notifier::notify()
{
  if (have_unhandled_notification) return;
  have_unhandled_notification = true;
  [controller
   performSelectorOnMainThread:@selector(notificationFromJpipClient:)
   withObject:client_identifier
   waitUntilDone:NO];
}


/*===========================================================================*/
/*                            kdms_window_manager                            */
/*===========================================================================*/

/*****************************************************************************/
/*                kdms_window_manager::kdms_window_manager                   */
/*****************************************************************************/

kdms_window_manager::kdms_window_manager(kdms_controller *controller)
{
  this->controller = controller;
  windows = next_idle_window = NULL;
  next_window_identifier = 1;
  main_observer = NULL;
  timer = NULL;
  presentation_timer = NULL;
  main_app_run_loop = CFRunLoopGetCurrent();
  last_known_key_wnd = nil;
  broadcast_actions_once = false;
  broadcast_actions_indefinitely = false;
  app_should_terminate = false;
  
  next_window_row = 0;
  
  next_window_to_wake = NULL;
  will_check_best_window_to_wake = false;
  
  min_presentation_drawing_interval = 1;
  open_file_list = NULL;
  window_list_change_mutex.create();
  
  // Set up timer for use later on
  CFRunLoopTimerContext timer_ctxt;
  timer_ctxt.version = 0;
  timer_ctxt.info = this;
  timer_ctxt.retain = NULL;
  timer_ctxt.release = NULL;
  timer_ctxt.copyDescription = NULL;
  double distant_time = 60.0*60.0*24.0*365.0*10000; // 10,000 years
  double huge_interval = 60.0*60.0*24.0*365.0*100; // 100 years
  timer =
    CFRunLoopTimerCreate(NULL,distant_time,huge_interval,0,0,
                         timer_callback,&timer_ctxt);
  if (timer == NULL)
    { kdu_error e; e << "Cannot create run-loop timer object."; }
  CFRunLoopAddTimer(main_app_run_loop,timer,kCFRunLoopCommonModes);
  
  // Set up main run-loop observer to be called when the run-loop goes idle
  CFRunLoopObserverContext observer_ctxt;
  observer_ctxt.version = 0;
  observer_ctxt.info = this;
  observer_ctxt.retain = NULL;
  observer_ctxt.release = NULL;
  observer_ctxt.copyDescription = NULL;
  main_observer =
    CFRunLoopObserverCreate(NULL,kCFRunLoopBeforeWaiting,
                            YES,0,run_loop_callback,&observer_ctxt);
  if (main_observer == NULL)
    { kdu_error e; e << "Cannot create run-loop observer."; }
  CFRunLoopAddObserver(main_app_run_loop,main_observer,kCFRunLoopCommonModes);

  reset_placement_engine();
}

/*****************************************************************************/
/*           kdms_window_manager::configure_presentation_manager             */
/*****************************************************************************/

void kdms_window_manager::configure_presentation_manager()
{
  // Find the screen refresh rate
  int refresh_rate = 0;
  CFDictionaryRef mode = CGDisplayCurrentMode(CGMainDisplayID());
  if (mode != NULL)
    {
      CFNumberRef value = (CFNumberRef)
        CFDictionaryGetValue(mode,kCGDisplayRefreshRate);
      if (value != NULL)
        CFNumberGetValue(value,kCFNumberIntType,&refresh_rate);
    }
  if (refresh_rate == 0)
    refresh_rate = 60; // Can't find true value; assume standard LCD rate
  min_presentation_drawing_interval = (int)(refresh_rate/45);
                       // Allow for screen to be updated at least 45fps
  if (min_presentation_drawing_interval < 1)
    min_presentation_drawing_interval = 1;
  
  // Set up regular timer to schedule presentation times.
  CFRunLoopTimerContext timer_ctxt;
  timer_ctxt.version = 0;
  timer_ctxt.info = this;
  timer_ctxt.retain = NULL;
  timer_ctxt.release = NULL;
  timer_ctxt.copyDescription = NULL;
  double interval = 1.0 / (double) refresh_rate;
  double initial_time = CFAbsoluteTimeGetCurrent() + interval;
  presentation_timer =
    CFRunLoopTimerCreate(NULL,initial_time,interval,0,0,
                         presentation_timer_callback,&timer_ctxt);
  if (presentation_timer == NULL)
    { kdu_error e; e << "Cannot create presentation run-loop timer object."; }
  CFRunLoopAddTimer(CFRunLoopGetCurrent(),presentation_timer,
                    kCFRunLoopCommonModes);
}

/*****************************************************************************/
/*             kdms_window_manager::application_can_terminate                */
/*****************************************************************************/

bool kdms_window_manager::application_can_terminate()
{
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (![elt->wnd application_can_terminate])
      return false;
  return true;
}
  
/*****************************************************************************/
/*        kdms_window_manager::send_application_terminating_messages         */
/*****************************************************************************/

void kdms_window_manager::send_application_terminating_messages()
{
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    [elt->wnd application_terminating];
}

/*****************************************************************************/
/*               kdms_window_manager::reset_placement_engine                 */
/*****************************************************************************/

void kdms_window_manager::reset_placement_engine()
{
  cycle_origin = next_window_pos = kdu_coords(10,10);
  next_window_row = cycle_origin.y;  
}

/*****************************************************************************/
/*                   kdms_window_manager::place_window                       */
/*****************************************************************************/

bool kdms_window_manager::place_window(kdms_window *wnd, NSSize frame_size,
                                       bool do_not_place_again,
                                       bool placing_first_empty_window)
{
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      break;
  if (elt != NULL)
    {
      if (elt->window_placed && do_not_place_again)
        return false;
      if (!placing_first_empty_window)
        elt->window_placed = true;
    }
  NSRect rect = [[NSScreen mainScreen] visibleFrame];
  kdu_dims screen_rect;
  screen_rect.pos.x = (int) floor(rect.origin.x + 0.5);
  screen_rect.pos.y = (int) floor(rect.origin.y + 0.5);
  screen_rect.size.x = (int) floor(rect.size.width + 0.5);
  screen_rect.size.y = (int) floor(rect.size.height + 0.5);
  
  // Determine `frame_rect' as it sits within the `screen_rect', with the
  // origin at the top left hand corner of the screen rectangle.
  kdu_dims frame_rect;
  frame_rect.pos = next_window_pos;
  frame_rect.size.x = (int) floor(frame_size.width + 0.5);
  frame_rect.size.y = (int) floor(frame_size.height + 0.5);
  int h_delta = screen_rect.size.x - frame_rect.size.x; // Hor. placement slack
  if (frame_rect.pos.x > h_delta)
    { // Window does not fit horizontally
      if (frame_rect.pos.x == cycle_origin.x)
        { // This is the first attempt to place a window on this row
          frame_rect.pos.x = h_delta; // Shift left as little as possible
        }
      else
        { // Move down to the next window placement row
          frame_rect.pos.x = cycle_origin.x;
          frame_rect.pos.y = next_window_row;
          if (frame_rect.pos.x > h_delta)
            frame_rect.pos.x = h_delta; // Shift left as little as possible
        }
    }
  int v_delta = screen_rect.size.y-frame_rect.size.y; // Vert. placement slack
  if (frame_rect.pos.y > v_delta)
    { // Window does not fit vertically
      if (frame_rect.pos.y == cycle_origin.y)
        { // This is the first row of windows for the cycle
          frame_rect.pos.y = v_delta; // Shift window up as little as possible
        }
      else
        { // Start a new cycle
          cycle_origin.x += 50;
          cycle_origin.y += 50;
          while (cycle_origin.y > (screen_rect.size.y>>2))
            cycle_origin.y -= (screen_rect.size.y>>2);
          while (cycle_origin.x > (screen_rect.size.x>>2))
            cycle_origin.x -= (screen_rect.size.x>>2);
          frame_rect.pos.y = next_window_row = cycle_origin.y;
          frame_rect.pos.x = cycle_origin.x;
          if (frame_rect.pos.y > v_delta)
            frame_rect.pos.y = v_delta;
          if (frame_rect.pos.x > h_delta)
            frame_rect.pos.x = h_delta;
        }
    }

  if (frame_rect.pos.x < 0)
    frame_rect.pos.x = 0; // Just in case
  if (frame_rect.pos.y < 0)
    frame_rect.pos.y = 0; // Just in case
  
  // Determine the next window placement location
  if (!placing_first_empty_window)
    {
      next_window_pos = frame_rect.pos;
      next_window_pos.x += frame_rect.size.x + 10; // Leave a small gap.
      if ((v_delta =
           (frame_rect.pos.y+frame_rect.size.y+10)-next_window_row) > 0)
        next_window_row += v_delta;
    }
  
  // Actually place and display the window
  NSRect place_rect;
  place_rect.origin.x = frame_rect.pos.x;
  place_rect.origin.y = (screen_rect.pos.y+screen_rect.size.y) -
  (frame_rect.pos.y+frame_rect.size.y);
  place_rect.size.width = frame_rect.size.x;
  place_rect.size.height = frame_rect.size.y;
  [wnd setFrame:place_rect display:YES];
  
  return true;
}

/*****************************************************************************/
/*                     kdms_window_manager::add_window                       */
/*****************************************************************************/

void
  kdms_window_manager::add_window(kdms_window *wnd)
{
  kdms_window_list *elt, *tail=NULL;
  for (elt=windows; elt != NULL; tail=elt, elt=elt->next)
    if (elt->wnd == wnd)
      return;
  app_should_terminate = false;
  elt = new kdms_window_list;
  elt->wnd = wnd;
  elt->window_identifier = next_window_identifier++;
  elt->file_or_url_name = NULL; // Until we have one
  elt->wakeup_time = -1.0;
  elt->window_empty = true;
  elt->window_placed = false;
  elt->frame_presenter =
    new kdms_frame_presenter(this,wnd,min_presentation_drawing_interval);
  window_list_change_mutex.lock();
  elt->next = NULL;
  if ((elt->prev = tail) == NULL)
    windows = elt;
  else
    tail->next = elt;
  window_list_change_mutex.unlock();
}

/*****************************************************************************/
/*                    kdms_window_manager::remove_window                     */
/*****************************************************************************/

void
  kdms_window_manager::remove_window(kdms_window *wnd)
{
  if (wnd == last_known_key_wnd)
    last_known_key_wnd = nil;
  
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      break;
  assert(elt != NULL);
  if (elt == NULL)
    return;
  if (elt == next_idle_window)
    next_idle_window = NULL; // Start scanning list again for idle processing

  window_list_change_mutex.lock();

  if (elt->frame_presenter != NULL)
    {
      elt->frame_presenter->reset(); // Just in case
      delete elt->frame_presenter;
      elt->frame_presenter = NULL;
    }
  
  if (elt->prev == NULL)
    windows = elt->next;
  else
    elt->prev->next = elt->next;
  if (elt->next != NULL)
    elt->next->prev = elt->prev;
  window_list_change_mutex.unlock();
  
  delete elt;
  
  if (windows == NULL)
    { 
      reset_placement_engine();
      app_should_terminate = true;
    }
  if (elt == next_window_to_wake)
    {
      next_window_to_wake = NULL;
      install_next_scheduled_wakeup();
    }
}

/*****************************************************************************/
/*                    kdms_window_manager::get_access_idx                    */
/*****************************************************************************/

int kdms_window_manager::get_access_idx(kdms_window *wnd)
{
  kdms_window_list *elt;
  int idx = 0;
  for (elt=windows; (elt != NULL) && (elt->wnd != wnd); idx++, elt=elt->next);
  return (elt==NULL)?-1:idx;
}

/*****************************************************************************/
/*                    kdms_window_manager::access_window                     */
/*****************************************************************************/

kdms_window *kdms_window_manager::access_window(int idx)
{
  kdms_window_list *elt;
  for (elt=windows; (elt != NULL) && (idx > 0); idx--, elt=elt->next);
  return ((elt==NULL) | (idx != 0))?NULL:elt->wnd;
}

/*****************************************************************************/
/*               kdms_window_manager::get_window_identifier                  */
/*****************************************************************************/

int kdms_window_manager::get_window_identifier(kdms_window *wnd)
{
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      return elt->window_identifier;
  return 0;
}

/*****************************************************************************/
/*                kdms_window_manager::declare_window_empty                  */
/*****************************************************************************/

void kdms_window_manager::declare_window_empty(kdms_window *wnd, bool empty)
{
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      elt->window_empty = empty;
}

/*****************************************************************************/
/*                  kdms_window_manager::find_empty_window                   */
/*****************************************************************************/

kdms_window *kdms_window_manager::find_empty_window()
{
  kdms_window *result = nil;
  kdms_window_list *best_elt = NULL;
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->window_empty &&
        ((best_elt == NULL) ||
         (best_elt->window_identifier < elt->window_identifier)))
      {
        best_elt = elt;
        result = elt->wnd;
      }
  return result;
}

/*****************************************************************************/
/*               kdms_window_manager::get_next_action_window                 */
/*****************************************************************************/

kdms_window *kdms_window_manager::get_next_action_window(kdms_window *wnd)
{
  if (!(broadcast_actions_once || broadcast_actions_indefinitely))
    return nil;
  if (!last_known_key_wnd)
    {
      if ([wnd isKeyWindow])
        last_known_key_wnd = wnd;
      else
        { // Initiating menu action from non-key window?? Safest to stop here.
          broadcast_actions_once = false;
          return nil;
        }
    }

  kdms_window_list *elt = windows;
  for (; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      break;
  if (elt == NULL)
    { // Should not happen!  Safest to cancel any action broadcast
      last_known_key_wnd = nil;
      broadcast_actions_once = false;
      return nil;
    }
  elt = elt->next;
  if (elt == NULL)
    elt = windows;
  kdms_window *result = elt->wnd;
  
  if (result == last_known_key_wnd)
    { // We have been right around already
      last_known_key_wnd = nil;
      broadcast_actions_once = false;
      return nil;
    }
  
  return result;
}

/*****************************************************************************/
/*               kdms_window_manager::set_action_broadcasting                */
/*****************************************************************************/

void kdms_window_manager::set_action_broadcasting(bool broadcast_once,
                                                  bool broadcast_indefinitely)
{
  this->broadcast_actions_once = broadcast_once;
  this->broadcast_actions_indefinitely = broadcast_indefinitely;
  last_known_key_wnd = nil;
}

/*****************************************************************************/
/*           kdms_window_manager::broadcast_playclock_adjustment             */
/*****************************************************************************/

void kdms_window_manager::broadcast_playclock_adjustment(double delta)
{
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    [elt->wnd adjust_playclock:delta];
}

/*****************************************************************************/
/*                     kdms_window_manager::schedule_wakeup                  */
/*****************************************************************************/

void
  kdms_window_manager::schedule_wakeup(kdms_window *wnd,
                                       double wakeup_time)
{
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      break;
  if ((elt == NULL) || (elt->wakeup_time == wakeup_time))
    return;
  elt->wakeup_time = wakeup_time;
  if (will_check_best_window_to_wake)
    { // We will get around to considering this wakeup when we get back into
      // the ongoing call to `install_next_scheduled_wakeup'.
      assert(next_window_to_wake == NULL);
      return;
    }
  if (wakeup_time < 0.0)
    {
      if (next_window_to_wake == elt)
        {
          next_window_to_wake = NULL;
          install_next_scheduled_wakeup();
        }
    }
  else if ((next_window_to_wake == NULL) ||
           (next_window_to_wake == elt) ||
           (wakeup_time < next_window_to_wake->wakeup_time))
    {
      next_window_to_wake = elt;
      CFRunLoopTimerSetNextFireDate(timer,wakeup_time);
    }
}

/*****************************************************************************/
/*                  kdms_window_manager::get_frame_presenter                 */
/*****************************************************************************/

kdms_frame_presenter *
  kdms_window_manager::get_frame_presenter(kdms_window *wnd)
{
  kdms_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      return elt->frame_presenter;
  return NULL;
}

/*****************************************************************************/
/*               kdms_window_manager::retain_open_file_pathname              */
/*****************************************************************************/

const char *kdms_window_manager::retain_open_file_pathname(const char *path,
                                                           kdms_window *wnd)
{  
  kdms_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (kdms_compare_file_pathnames(scan->open_pathname,path))
      {
        scan->retain_count++;
        break;
      }
  if (scan == NULL)
    {
      scan = new kdms_open_file_record;
      scan->open_pathname = new char[strlen(path)+1];
      strcpy(scan->open_pathname,path);
      scan->next = open_file_list;
      open_file_list = scan;
      scan->retain_count = 1;
    }
  kdms_window_list *wnd_elt = windows;
  for (; wnd_elt != NULL; wnd_elt=wnd_elt->next)
    if (wnd_elt->wnd == wnd)
      wnd_elt->file_or_url_name = scan->open_pathname;
  return scan->open_pathname;
}

/*****************************************************************************/
/*              kdms_window_manager::release_open_file_pathname              */
/*****************************************************************************/

void kdms_window_manager::release_open_file_pathname(const char *path,
                                                     kdms_window *wnd)
{
  kdms_window_list *wnd_elt = windows;
  for (; wnd_elt != NULL; wnd_elt=wnd_elt->next)
    if (wnd_elt->wnd == wnd)
      wnd_elt->file_or_url_name = NULL;
  
  kdms_open_file_record *scan, *prev;
  for (prev=NULL, scan=open_file_list; scan!=NULL; prev=scan, scan=scan->next)
    if (kdms_compare_file_pathnames(scan->open_pathname,path))
      break;
  if (scan == NULL)
    return; // Nothing to release
  scan->retain_count--;
  if (scan->retain_count == 0)
    {
      if (prev == NULL)
        open_file_list = scan->next;
      else
        prev->next = scan->next;
      if (scan->save_pathname != NULL)
        {
          remove(scan->open_pathname);
          rename(scan->save_pathname,scan->open_pathname);
        }
      delete scan;
    }
}

/*****************************************************************************/
/*                 kdms_window_manager::retain_jpip_client                   */
/*****************************************************************************/

const char *
  kdms_window_manager::retain_jpip_client(const char *server,
                                          const char *request,
                                          const char *url,
                                          kdu_client * &client,
                                          int &request_queue_id,
                                          kdms_window *wnd)
{
  client = NULL; request_queue_id = -1; // Just in case something goes wrong
  if (url != NULL)
    server = request = NULL; // To be sure we don't mix two sources of info
  kdms_window_list *wnd_elt = windows;
  for (; wnd_elt != NULL; wnd_elt=wnd_elt->next)
    if (wnd_elt->wnd == wnd)
      {
        wnd_elt->file_or_url_name = NULL;
        break;
      }

  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  kdu_client_mode mode;
  switch ([defaults integerForKey:KDMS_KEY_JPIP_MODE]) {
    case ((NSInteger) KDU_CLIENT_MODE_AUTO):
      mode = KDU_CLIENT_MODE_AUTO; break;
    case ((NSInteger) KDU_CLIENT_MODE_INTERACTIVE):
      mode = KDU_CLIENT_MODE_INTERACTIVE; break;
    case ((NSInteger) KDU_CLIENT_MODE_NON_INTERACTIVE):
      mode = KDU_CLIENT_MODE_NON_INTERACTIVE; break;
    default: mode = KDU_CLIENT_MODE_INTERACTIVE; break;
  }
 
  kdms_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    {
      if (scan->jpip_client == NULL)
        continue;
      if (!scan->jpip_client->check_compatible_connection(server,request,
                                                          mode,url))
        continue;
      if (scan->jpip_client->is_one_time_request())
        request_queue_id = 0;
      else
        request_queue_id = scan->jpip_client->add_queue();
      if (request_queue_id < 0)
        continue; // Maybe the client connection was lost
      scan->retain_count++;
      break;
    }
  if (scan == NULL)
    {
      NSString *string;
      const char *proxy = NULL;
      const char *transport = "http";
      const char *cache_dir = NULL;
      if (((string = [defaults stringForKey:KDMS_KEY_JPIP_CACHE]) != nil) &&
          ([string length] > 0) &&
          ([defaults boolForKey:KDMS_KEY_JPIP_USE_CACHE] == YES))
        cache_dir = [string UTF8String];
      if (((string = [defaults stringForKey:KDMS_KEY_JPIP_PROXY]) != nil) &&
          ([string length] > 0) &&
          ([defaults boolForKey:KDMS_KEY_JPIP_USE_PROXY] == YES))
        proxy = [string UTF8String];
      if (((string = [defaults stringForKey:KDMS_KEY_JPIP_TRANSPORT])!=nil) &&
          ([string length] > 0))
        transport = [string UTF8String];
      
      scan = new kdms_open_file_record;
      scan->jpip_client = new kdu_client;
      try {
        if (url != NULL)
          {
            request_queue_id =
              scan->jpip_client->connect(NULL,proxy,NULL,transport,
                                         cache_dir,mode,url);
            scan->open_url = new char[strlen(url)+1];
            strcpy(scan->open_url,url);
          }
        else
          {
            request_queue_id =
              scan->jpip_client->connect(server,proxy,request,transport,
                                         cache_dir,mode,NULL);
            scan->open_url = new char[strlen(server)+strlen(request)+
                                      strlen("jpip://") + 2];
            strcpy(scan->open_url,"jpip://");
            strcat(scan->open_url,server);
            strcat(scan->open_url,"/");
            strcat(scan->open_url,request);
          }
      }
      catch (kdu_exception exc) {
        delete scan;       
        throw exc;
      }
      scan->client_identifier = [NSObject alloc];
      scan->client_notifier =
        new kdms_client_notifier(controller,scan->client_identifier);
      scan->jpip_client->install_notifier(scan->client_notifier);
      scan->next = open_file_list;
      open_file_list = scan;
      scan->retain_count = 1;
    }

  if (wnd_elt != NULL)
    wnd_elt->file_or_url_name = scan->open_url;
  client = scan->jpip_client;
  return scan->open_url;
}

/*****************************************************************************/
/*         kdms_window_manager::use_jpx_translator_with_jpip_client          */
/*****************************************************************************/

void
  kdms_window_manager::use_jpx_translator_with_jpip_client(kdu_client *client)
{
  kdms_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (scan->jpip_client == client)
      {
        if (scan->jpx_client_translator == NULL)
          {
            scan->jpx_client_translator = new kdu_clientx;
            client->install_context_translator(scan->jpx_client_translator);
          }
        break;
      }
}

/*****************************************************************************/
/*                  kdms_window_manager::release_jpip_client                 */
/*****************************************************************************/

void kdms_window_manager::release_jpip_client(kdu_client *client,
                                              kdms_window *wnd)
{
  kdms_window_list *wnd_elt = windows;
  for (; wnd_elt != NULL; wnd_elt=wnd_elt->next)
    if (wnd_elt->wnd == wnd)
      wnd_elt->file_or_url_name = NULL;
  
  kdms_open_file_record *scan, *prev;
  for (prev=NULL, scan=open_file_list; scan!=NULL; prev=scan, scan=scan->next)
    if (scan->jpip_client == client)
      break;
  if (scan == NULL)
    return; // Nothing to release
  scan->retain_count--;
  if (scan->retain_count == 0)
    {
      if (prev == NULL)
        open_file_list = scan->next;
      else
        prev->next = scan->next;
      assert(scan->save_pathname == NULL);
      scan->jpip_client->disconnect(false,2000,-1,true);
         // Forcibly disconnect everything after a 2 second timeout.
      scan->jpip_client->close();
      delete scan;
    }
}

/*****************************************************************************/
/*              kdms_window_manager::handle_client_notification              */
/*****************************************************************************/

void kdms_window_manager::handle_client_notification(id client_identifier)
{
  const char *open_url = NULL;
  kdms_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (scan->client_identifier == client_identifier)
      {
        open_url = scan->open_url;
        scan->client_notifier->handling_notification();
        break;
      }
  for (kdms_window_list *wnd=windows; wnd != NULL; wnd=wnd->next)
    if ((wnd->file_or_url_name == open_url) && (wnd->wnd != nil))
      [wnd->wnd client_notification];
}

/*****************************************************************************/
/*                kdms_window_manager::get_save_file_pathname                */
/*****************************************************************************/

const char *kdms_window_manager::get_save_file_pathname(const char *path)
{
  kdms_open_file_record *scan;
  for (scan=open_file_list; scan!=NULL; scan=scan->next)
    if (kdms_compare_file_pathnames(scan->open_pathname,path))
      {
        if (scan->save_pathname == NULL)
          {
            scan->save_pathname = new char[strlen(path)+2];
            strcpy(scan->save_pathname,path);
            strcat(scan->save_pathname,"~");
          }
        return scan->save_pathname;
      }
  return path;
}

/*****************************************************************************/
/*              kdms_window_manager::declare_save_file_invalid               */
/*****************************************************************************/

void kdms_window_manager::declare_save_file_invalid(const char *path)
{
  kdms_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (kdms_compare_file_pathnames(scan->open_pathname,path))
      return; // Safety measure to prevent deleting open file; shouldn't happen
    else if ((scan->save_pathname != NULL) &&
             kdms_compare_file_pathnames(scan->save_pathname,path))
      {
        delete[] scan->save_pathname;
        scan->save_pathname = NULL;
      }
  remove(path);
}

/*****************************************************************************/
/*              kdms_window_manager::get_open_file_retain_count              */
/*****************************************************************************/

int kdms_window_manager::get_open_file_retain_count(const char *path)
{
  kdms_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (kdms_compare_file_pathnames(scan->open_pathname,path))
      return scan->retain_count;
  return 0;
}
  
/*****************************************************************************/
/*               kdms_window_manager::check_open_file_replaced               */
/*****************************************************************************/

bool kdms_window_manager::check_open_file_replaced(const char *path)
{
  kdms_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if ((scan->save_pathname != NULL) &&
        kdms_compare_file_pathnames(scan->open_pathname,path))
      return true;
  return false;
}

/*****************************************************************************/
/*                     kdms_window_manager::timer_callback                   */
/*****************************************************************************/

void kdms_window_manager::timer_callback(CFRunLoopTimerRef timer, void *info)
{
  kdms_window_manager *manager = (kdms_window_manager *) info;
  kdms_window_list *elt = manager->next_window_to_wake;
  if ((elt == NULL) || (timer != manager->timer))
    return;
  double current_time = CFAbsoluteTimeGetCurrent();
  double scheduled_time = elt->wakeup_time;
  elt->wakeup_time = -1.0; // Cancel further wakeups
  manager->next_window_to_wake = NULL;
  manager->will_check_best_window_to_wake = true; // Prevents scheduling a new
      // timer event from within `schedule_wakeup' if it is called from within
      // the following `wakeup' call before getting a chance to choose the
      // best window to time, in the ensuing call to
      // `install_next_scheduled_wakeup'.
  if (scheduled_time >= 0.0)
    [elt->wnd wakeupScheduledFor:scheduled_time occurredAt:current_time];
  manager->install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*              kdms_window_manager::presentation_timer_callback             */
/*****************************************************************************/

void kdms_window_manager::presentation_timer_callback(CFRunLoopTimerRef timer,
                                                      void *info)
{
  kdms_window_manager *manager = (kdms_window_manager *) info;
  manager->window_list_change_mutex.lock();
  kdms_window_list *elt;
  for (elt=manager->windows; elt != NULL; elt=elt->next)
    { // Give every window a chance to present a frame in the presentation
      // thread.
      elt->frame_presenter->draw_pending_frame(manager->main_app_run_loop);
    }
  manager->window_list_change_mutex.unlock();
}

/*****************************************************************************/
/*            kdms_window_manager::install_next_scheduled_wakeup             */
/*****************************************************************************/

void
  kdms_window_manager::install_next_scheduled_wakeup()
{
  next_window_to_wake = NULL; // Just in case
  will_check_best_window_to_wake = true;
      // Avoids accidental installation of a new scheduled wakeup while we
      // are scanning for one.  This could happen if `schedule_wakeup' is
      // called from within a window's `wakeup' function.
  
  double current_time = 0.0; // Evaluate only if we need it
  kdms_window_list *elt, *earliest_elt = NULL;
  while (earliest_elt == NULL)
    {
      for (elt=windows; elt != NULL; elt=elt->next)
        if (elt->wakeup_time >= 0.0)
          {
            if (earliest_elt == NULL)
              {
                current_time = CFAbsoluteTimeGetCurrent();
                earliest_elt = elt;
              }
            else if (elt->wakeup_time < earliest_elt->wakeup_time)
              earliest_elt = elt;
          }
      if (earliest_elt == NULL)
        {
          will_check_best_window_to_wake = false;
          return; // Nothing left to wakeup
        }
      if (earliest_elt->wakeup_time <= current_time)
        {
          double scheduled_time = earliest_elt->wakeup_time;
          earliest_elt->wakeup_time = -1.0;
          if (scheduled_time >= 0.0)
            [earliest_elt->wnd
             wakeupScheduledFor:scheduled_time
             occurredAt:current_time];
          earliest_elt = NULL;
        }
    }

  // If we get here, we are ready to wait for `earliest_elt'
  next_window_to_wake = earliest_elt;
  CFRunLoopTimerSetNextFireDate(timer,earliest_elt->wakeup_time);
  will_check_best_window_to_wake = false;
}

/*****************************************************************************/
/*                    kdms_window_manager::run_loop_callback                 */
/*****************************************************************************/

void
  kdms_window_manager::run_loop_callback(CFRunLoopObserverRef observer,
                                         CFRunLoopActivity activity,
                                         void *info)
{
  kdms_window_manager *manager = (kdms_window_manager *) info;
  double start_time = CFAbsoluteTimeGetCurrent();
  while (1)
    {
      if (manager->should_app_terminate())
        {
          [NSApp terminate:nil];
          return;
        }
      if (manager->windows == NULL)
        return;
      kdms_window_list *elt = manager->next_idle_window;
      if (elt == NULL)
        elt = manager->next_idle_window = manager->windows;
      while (![elt->wnd on_idle])
        {
          elt = elt->next;
          if (elt == NULL)
            elt = manager->windows;
          if (elt == manager->next_idle_window)
            { // Been right around the entire window list and found no window
              // with any idle processing left to do.  Now we can let the
              // run loop sleep.
              return;
            }
        }

      // We just did some idle-time processing.  We need to return so
      // that pending user messages can be called, but if there are none,
      // we want to make sure that we will come back into this function to
      // do any remaining idle processing, rather than sleeping until a
      // user event occurs.  One way to do this is to send a dummy event
      // to the run-loop.  However, the method represented below also works
      // just fine and is simpler.
      manager->next_idle_window = elt->next;
      double cur_time = CFAbsoluteTimeGetCurrent();
      if (cur_time > (start_time+0.05))
        {
          CFRunLoopWakeUp(manager->main_app_run_loop);
          return;
        }
    }
}


/*===========================================================================*/
/* IMPLEMENTATION               kdms_controller                              */
/*===========================================================================*/

@implementation kdms_controller

/*****************************************************************************/
/*                     kdms_controller::awake_from_nib                       */
/*****************************************************************************/

- (void)awakeFromNib
{
  window_manager = NULL;
  kdu_customize_warnings(&warn_collector);
  kdu_customize_errors(&err_collector);
  
  // Create cursors
  cursors[0] = [NSCursor crosshairCursor];
  cursors[1] = [NSCursor openHandCursor];
  [cursors[0] retain]; // Just in case; not clear if they are auto-release
  [cursors[1] retain]; // Just in case; not clear if they are auto-release
  
  // Create window manager
  window_manager = new kdms_window_manager(self);
  
  // Start presentation thread
  [NSThread detachNewThreadSelector:@selector(presentationThreadEntry:)
                           toTarget:self
                         withObject:nil];
  
  // Create one empty window -- no harm in that
  kdms_window *wnd = [kdms_window alloc];
  [wnd initWithManager:window_manager andCursors:cursors];
  window_manager->place_window(wnd,[wnd frame].size,false,true);
  
  // Install a Get URL Handler to handle URL's with the "jpip://" protocol.
  NSAppleEventManager *event_manager =
    [NSAppleEventManager sharedAppleEventManager];
  [event_manager setEventHandler:self
                     andSelector:@selector(handleGetURLEvent:withReplyEvent:)
                  forEventClass:kInternetEventClass
                      andEventID:kAEGetURL];
}

/*****************************************************************************/
/*                 kdms_controller::presentationThreadEntry                  */
/*****************************************************************************/

- (void)presentationThreadEntry:(id)param
{
  NSAutoreleasePool *autorelease_pool = [[NSAutoreleasePool alloc] init];
  NSDate *endDate = [NSDate distantFuture];
  try {
    window_manager->configure_presentation_manager();
  } catch (...) {
    exit(-1); // Must have generated an error message during configuration;
              // for now, we'll just exit the entire process.  Could keep
              // running, and adjust things so that any video frames are
              // presented immediately, but there is no good reason why
              // an error should occur here.
  }
  do {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:endDate]; // Process a single event
  } while (1); /* Here is where we could check for an exit condition if we
                  need one in the future. */
  [autorelease_pool release];
}

/*****************************************************************************/
/*                        kdms_controller::openFile                          */
/*****************************************************************************/

- (BOOL)application:(NSApplication *)app openFile:(NSString *)filename
{
  kdms_window *wnd = window_manager->find_empty_window();
  if (wnd == nil)
    {
      wnd = [kdms_window alloc];
      [wnd initWithManager:window_manager andCursors:cursors];
    }
  [wnd open_file:filename];
  return YES;
}

/*****************************************************************************/
/*                        kdms_controller::openFiles                         */
/*****************************************************************************/

- (void)application:(NSApplication *)app openFiles:(NSArray *)filenames
{
  for (int n=0; n < filenames.count; n++)
    {
      kdms_window *wnd = window_manager->find_empty_window();
      if (wnd == nil)
        {
          wnd = [kdms_window alloc];
          [wnd initWithManager:window_manager andCursors:cursors];
        }
      NSString *filename = [filenames objectAtIndex:n];
      [wnd open_file:filename];
    }  
}

/*****************************************************************************/
/*                   kdms_controller::handleGetURLEvent                      */
/*****************************************************************************/

- (void)handleGetURLEvent:(NSAppleEventDescriptor *)event
           withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
  NSString *url_string = nil;
  url_string = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
  if (url_string == nil)
    return;
  kdms_window *wnd = window_manager->find_empty_window();
  if (wnd == nil)
    {
      wnd = [kdms_window alloc];
      [wnd initWithManager:window_manager andCursors:cursors];
    }
  [wnd open_url:url_string];
}

/*****************************************************************************/
/*               kdms_controller::notificationFromJpipClient                 */
/*****************************************************************************/

- (void) notificationFromJpipClient:(id)client_identifier
{
  window_manager->handle_client_notification(client_identifier);
}

/*****************************************************************************/
/*                      kdms_controller::menuWindowNew                       */
/*****************************************************************************/

- (IBAction) menuWindowNew:(NSMenuItem *)sender
{
  kdms_window *wnd = [kdms_window alloc];
  [wnd initWithManager:window_manager andCursors:cursors];
  window_manager->place_window(wnd,[wnd frame].size);
}

/*****************************************************************************/
/*                    kdms_controller::menuWindowArrange                     */
/*****************************************************************************/

- (IBAction) menuWindowArrange:(NSMenuItem *)sender
{
  if (window_manager == NULL)
    return;
  window_manager->reset_placement_engine();
  kdms_window *wnd;
  for (int w=0; (wnd = window_manager->access_window(w)) != NULL; w++)
    {
      NSRect wnd_frame = [wnd frame];
      window_manager->place_window(wnd,wnd_frame.size);
    }
}

/*****************************************************************************/
/*                kdms_controller::menuWindowBroadcastOnce                   */
/*****************************************************************************/

- (IBAction) menuWindowBroadcastOnce:(NSMenuItem *)sender
{
  if (window_manager != NULL)
    window_manager->set_action_broadcasting(true,false);
}

/*****************************************************************************/
/*             kdms_controller::menuWindowBroadcastIndefinitely              */
/*****************************************************************************/

- (IBAction) menuWindowBroadcastIndefinitely:(NSMenuItem *)sender
{
  if (window_manager != NULL)
    {
      bool indefinite = window_manager->is_broadcasting_actions_indefinitely();
      window_manager->set_action_broadcasting(false,!indefinite);
    }
}

/*****************************************************************************/
/*                   kdms_controller::menuFileOpenNewWindow                  */
/*****************************************************************************/

- (IBAction) menuFileOpenNewWindow:(NSMenuItem *)sender
{
  NSArray *file_types =
    [NSArray arrayWithObjects:@"jp2",@"jpx",@"jpf",@"mj2",@"j2c",@"j2k",nil];
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  [panel setPrompt:@"Open"];
  [panel setTitle:@"Choose file(s) to open, each in a new window"];
  [panel setAllowsMultipleSelection:YES];
  if ([panel runModalForTypes:file_types] == NSOKButton)
    { // Open one or more new windows with files
      NSArray *filenames = [panel filenames];
      for (int n=0; n < filenames.count; n++)
        {
          kdms_window *wnd = [kdms_window alloc];
          [wnd initWithManager:window_manager andCursors:cursors];
          NSString *filename = [[panel filenames] objectAtIndex:n];
          [wnd open_file:filename];
        }
    }
}

/*****************************************************************************/
/*                 kdms_controller::menuFileOpenUrlNewWindow                 */
/*****************************************************************************/

- (IBAction)menuFileOpenUrlNewWindow:(NSMenuItem *)sender
{
  kdms_url_dialog *dialog = [[kdms_url_dialog alloc] init];
  [dialog setTitle:@"Open JPIP URL in New Window"];
  NSString *url = [dialog run_modal];
  [dialog release];
  if (url != nil)
    {
      kdms_window *wnd = [kdms_window alloc];
      [wnd initWithManager:window_manager andCursors:cursors];
      [wnd open_url:url];      
    }
}
  
/*****************************************************************************/
/*                       kdms_controller::menuAppQuit                        */
/*****************************************************************************/

- (IBAction)menuAppQuit:(NSMenuItem *)sender
{
  if (window_manager != NULL)
    {
      if (!window_manager->application_can_terminate())
        return; // Not allowed to terminate at this point in time.
      window_manager->send_application_terminating_messages();
    }
  [NSApp terminate:sender];
}

/*****************************************************************************/
/*                     kdms_controller:validateMenuItem                      */
/*****************************************************************************/

- (BOOL) validateMenuItem:(NSMenuItem *)item
{
  if (window_manager == NULL)
    return NO; // Should not happen
  if ([item action] == @selector(menuWindowBroadcastIndefinitely:))
    {
      bool indefinite = window_manager->is_broadcasting_actions_indefinitely();
      [item setState:((indefinite)?YES:NO)];
      return YES;
    }
  if ([item action] == @selector(menuWindowArrange:))
    return (window_manager->access_window(0))?YES:NO;
  return YES;  
}

@end
