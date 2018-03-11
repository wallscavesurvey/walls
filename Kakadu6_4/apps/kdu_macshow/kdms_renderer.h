/*****************************************************************************/
// File: kdms_renderer.h [scope = APPS/MACSHOW]
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
   Defines the main image management and rendering object associated with
any given window's view.  The `kdms_renderer' object works hand in hand
with `kdms_window' and its document view, `kdms_document_view'.  In fact,
menu commands and other user events received by those objects are mostly
forwarded directly to the `kdms_renderer' object.
******************************************************************************/

#import <Cocoa/Cocoa.h>
#import "kdms_controller.h"
#import "kdms_metashow.h"
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_file_io.h"
#include "jpx.h"
#include "kdu_region_compositor.h"

// External declarations
@class kdms_window;
@class kdms_document_view;
@class kdms_catalog;
@class kdms_metadata_editor;

// Declared here:
class kdms_file;
struct kdms_file_editor;
class kdms_file_services;
class kdms_compositor_buf;
class kdms_region_compositor;
class kdms_data_provider;
class kdms_playmode_stats;
struct kdms_rel_dims;
struct kdms_rel_focus;
class kdms_renderer;

#define KDMS_OVERLAY_BORDER 4
#define KDMS_NUM_OVERLAY_PARAMS 18 // Size of the array initializer below
#define KDMS_OVERLAY_PARAMS \
   {4, 0x20FFFF00, 0x600000FF, 0xA00000FF, 0xA0FFFF00, 0x60FFFF00, 10, \
    2, 0x200000FF, 0x600000FF, 0x80FFFF00, 6, \
    1, 0x300000FF, 0x60FFFF00, 3, \
    0, 0x400000FF }
  // Above macro defines initializers for the `overlay_params' array,
  // passed to `kdu_region_compositor::configure_overlays'.  The meaning of
  // the parameters is provided along with the definition of this function.
  // Briefly, though, each row above defines a border size B, an interior
  // ARGB colour, B ARGB border colours running from the interior to exterior
  // edge of the border, and a minimum "width" for the overall region defined
  // by the ROI description box which is being painted.  If the minimum
  // width is not met, the overlay painting logic falls through to the next
  // row of parameters, which generally (but not necessarily) involves a
  // smaller border size. The last line corresponds to the case in which no
  // border is painted at all, so there is only an interior colour.

enum kds_status_id {
  KDS_STATUS_LAYER_RES=0,
  KDS_STATUS_DIMS=1,
  KDS_STATUS_MEM=2,
  KDS_STATUS_CACHE=3,
  KDS_STATUS_PLAYSTATS=4
};

/*****************************************************************************/
/*                             EXTERNAL FUNCTIONS                            */
/*****************************************************************************/

extern int kdms_utf8_to_unicode(const char *utf8_string,
                                kdu_uint16 unichar_buf[], int max_chars);
  /* This simple function expands a null-terminated UTF-8 string as a
     null-terminated 16-bit unicode string, storing the result in
     `unicode_buf'.  At most `max_chars' are converted, not including the
     terminating null character, so `unichar_buf' must be capable of holding
     at least `max_chars'+1 entries.  The function returns the number of
     unicode characters actually converted. Note that UTF-8 can represents
     character codes which with more than 16 bits.  Where this happens, the
     function simply discards all more significant bits.  There is probably
     a more intelligent and uniform way of handling such cases, but they
     should virtually never arise in practice. */

/*****************************************************************************/
/*                                 kdms_file                                 */
/*****************************************************************************/

class kdms_file {
private: // Private life-cycle functions
  friend class kdms_file_services;
  kdms_file(kdms_file_services *owner); // Links onto tail of `owner' list
  ~kdms_file(); // Unlinks from `owner' first.
public: // Member functions
  int get_id() { return unique_id; }
  /* Gets the unique identifier for this file, which can be used with
   `kdms_file_services::find_file'. */
  void release();
  /* Decrements the retain count.  Once the retain count reaches 0, the
   object is deleted, and the file might also be deleted if it was a
   temporary file. */
  void retain()
    { retain_count++; }
  const char *get_pathname() { return pathname; }
  bool get_temporary() { return is_temporary; }
  bool create_if_necessary(const char *initializer);
  /* Creates the file and sets some initial contents based on the supplied
   `initializer' string, if any.  Returns false if the file cannot be
   created, or it already exists.  If the `initializer' string is NULL,
   any file created here is opened as a binary file; otherwise, it is
   created to hold text. */
private: // Data
  int retain_count;
  int unique_id;
  kdms_file_services *owner;
  kdms_file *next, *prev; // Used to build a doubly-linked list
  char *pathname;
  bool is_temporary; // If true, the file is deleted once `retain_count'=0.
};

/*****************************************************************************/
/*                             kdms_file_editor                              */
/*****************************************************************************/

struct kdms_file_editor {
  kdms_file_editor()
    {
      doc_suffix = app_pathname = NULL;
      app_name = NULL;
      next = NULL;
    }
  ~kdms_file_editor()
    {
      if (doc_suffix != NULL) delete[] doc_suffix;
      if (app_pathname != NULL) delete[] app_pathname;
    }
  char *doc_suffix; // File extension for matching document types
  char *app_pathname; // Full pathname to the application bundle
  const char *app_name; // Points to the application name within full pathname
  FSRef fs_ref; // File-system reference for the editor
  kdms_file_editor *next;
};

/*****************************************************************************/
/*                            kdms_file_services                             */
/*****************************************************************************/

class kdms_file_services {
public: // Member functions
  kdms_file_services(const char *source_pathname);
    /* If `source_pathname' is non-NULL, all temporary files are created by
     removing its file suffix, if any, and then appending to the name to
     make it unique.  This is the way you should create the object when
     editing an existing file.  Otherwise, if `source_pathname' is NULL,
     a base pathname is created internally using the `tmpnam' function. */
  ~kdms_file_services();
  bool find_new_base_pathname();
    /* This function sets the base pathname for temporary files based upon
     the ANSI tmpnam() function.  It erases any existing base pathname and
     generates a new one which is derived from the name returned by tmpnam()
     after first testing that the file returned by tmpnam() can be created.
     If the file cannot be created, the function returns false.  The function
     is normally called only internally from within `confirm_base_pathname'. */
  kdms_file *retain_tmp_file(const char *suffix);
    /* Generates a unique filename.  Does not try to create the file at this
     point, but may do so later.  In any case, the file will be deleted once
     it is no longer needed.  The returned object has a retain count of 1.
     Once you release the object, it will be destroyed, and the temporary
     file deleted.  */
  kdms_file *retain_known_file(const char *pathname);
    /* As for the last function, but the returned object represents a
     non-temporary file (i.e., one that will not be deleted when it is no
     longer required).  Generally, this is an existing file supplied by the
     user.  You should supply the full pathname to the file.  The returned
     object has a retain count of at least 1.  If the same file is already
     retained for some other purpose, the retain count may be greater. */
  kdms_file *find_file(int identity);
    /* This function is used to recover a file whose ID was obtained
     previously using `kdms_file::get_id'.  It provides a robust mechanism
     for accessing files whose identity is stored indirectly via an integer
     and managed by the `jpx_meta_manager' interface. */
  kdms_file_editor *get_editor_for_doc_type(const char *doc_suffix, int which);
    /* This function is used to access an internal list of editors which
     can be used for files (documents) with the indicated suffix (extension).
     The function returns NULL if `which' is greater than or equal to the
     number of matching editors for such files which are known.  If the
     function has no stored editors for the file, it attempts to find some,
     using operating-system services.  You can also add editors using the
     `add_editor_for_doc_type' function. */
  kdms_file_editor *add_editor_for_doc_type(const char *doc_suffix,
                                            const char *app_pathname);
    /* Use this function if you want to add custom editors for files
     (documents) with a given suffix (extension), or if you want to move an
     existing editor to the head of the list. */
private:
  void confirm_base_pathname(); // Called when `retain_tmp_file' is first used
private: // Data
  friend class kdms_file;
  kdms_file *head, *tail; // Linked list of files
  int next_unique_id;
  char *base_pathname; // For temporary files
  bool base_pathname_confirmed; // True once we know we can write to above
  kdms_file_editor *editors; // Head of the list; new editors go to the head
};

/*****************************************************************************/
/*                           kdms_compositor_buf                             */
/*****************************************************************************/

class kdms_compositor_buf : public kdu_compositor_buf {
public: // Member functions and overrides
  kdms_compositor_buf()
    { handle=NULL; next=NULL; }
  virtual ~kdms_compositor_buf()
    {
      if (handle != NULL) delete[] handle;
      buf = NULL; // Remember to do this whenever overriding the base class
    }
protected: // Additional data
  friend class kdms_region_compositor;
  kdu_uint32 *handle;
  kdu_coords size;
  kdms_compositor_buf *next;
};

/*****************************************************************************/
/*                          kdms_region_compositor                           */
/*****************************************************************************/

class kdms_region_compositor : public kdu_region_compositor {
  /* This class augments the `kdu_region_compositor' class with display
   buffer management logic which allocates image buffers whose lines are
   all aligned on 16-byte boundaries.  It also recycles buffers, to
   minimize excessive allocation and reallocation of large blocks of
   memory.  This can potentially improve cache utilization in video
   applications.  You can take this class as a template for designing richer
   buffer management systems -- e.g. ones which might utilize display
   hardware explicitly. */
public: // Member functions
  kdms_region_compositor(kdu_thread_env *env=NULL,
                         kdu_thread_queue *env_queue=NULL)
                 : kdu_region_compositor(env,env_queue)
    { active_list = free_list = NULL; }
  ~kdms_region_compositor()
    {
      pre_destroy(); // Destroy all buffers before base destructor called
      assert(active_list == NULL);
      while ((active_list=free_list) != NULL)
        { free_list=active_list->next; delete active_list; }
    }
  virtual kdu_compositor_buf *
    allocate_buffer(kdu_coords min_size, kdu_coords &actual_size,
                    bool read_access_required);
  virtual void delete_buffer(kdu_compositor_buf *buffer);
private: // Data
  kdms_compositor_buf *active_list;
  kdms_compositor_buf *free_list;
};

/*****************************************************************************/
/*                            kdms_data_provider                             */
/*****************************************************************************/

class kdms_data_provider {
public: // Member functions
  kdms_data_provider();
  ~kdms_data_provider();
  CGDataProviderRef init(kdu_coords size, kdu_uint32 *buf, int row_gap,
                         bool display_with_focus=false,
                         kdu_dims focus_box=kdu_dims());
    /* This function sets up the data provider for painting imagery in
       response to calls to its `get_bytes_callback' function.  The region
       to be painted has dimensions given by `size' and the data is buffered
       in the array given by `buf', whose successive image rows are
       separated by `row_gap'.  The start of the buffer is aligned with
       the upper left corner of the region to be painted, as supplied in
       an outer call to `CGImageCreate' (see `kdms_renderer::paint_region').
       The caller should attempt to adjust the region to be painted so
       that `buf' is aligned on a 16-byte boundary.  This maximizes the
       opportunity for efficient SIMD implementations of the process which
       transfers data to the GPU when `get_bytes_callback' is called.
         The internal implementation selects between a number of transfer
       strategies, depending on the machine architecture, word alignment
       and whether or not a focus box is required.
         The `focus_box' identifies a region (possibly empty) relative to
       the top-left corner of the region to be painted, where a focus box
       is to be highlighted.  Focus box highlighting is performed by passing
       the imagery inside and outside the focus box through two different
       intensity tone curves, so that the focussing region appears
       brighter.  This behaviour occurs whenever the `display_with_focus'
       argument is true. The supplied `focus_box' need not be contained
       within (or even intersect) the region being painted. */
private: // The provider callback function
  static size_t get_bytes_callback(void *info, void *buffer,
                                   size_t offset, size_t count);
private: // Data
  off_t tgt_size; // Total number of dwords offered by the provider
  int tgt_width, tgt_height; // Derived from the `init's `size' argument
  int buf_row_gap; // Copy of `init's `row_gap' argument
  kdu_uint32 *buf; // Copy of `init's `buf' argument
  bool display_with_focus; // Indicates that tone curves are required
  int rows_above_focus; // Rows above focus box when focussing
  int focus_rows; // Rows inside focus box when focussing
  int rows_below_focus; // Rows below focus box when focussing
  int cols_left_of_focus; // Columns to the left of focus box when focussing
  int focus_cols; // Columns inside focus box when focussing
  int cols_right_of_focus; // Columns to the right of focus box when focussing
  CGDataProviderRef provider_ref;
  kdu_byte foreground_lut[256]; // Foreground tone curve for use when focussing
  kdu_byte background_lut[256]; // Background tone curve for use when focussing
};

/*****************************************************************************/
/*                            kdms_playmode_stats                            */
/*****************************************************************************/

class kdms_playmode_stats {
public: // Member functions
  kdms_playmode_stats() { reset(); }
  void reset() // Called by `kdms_renderer::start_playmode'
    {
      last_frame_idx = -1;
      num_updates_since_reset = 0;
      cur_time_at_last_update = 0.0;
      mean_time_between_frames = 1.0; // Any value which can be inverted
    }
  void update(int frame_idx, double start_time, double end_time,
              double cur_time);
    // Called each time a rendered frame is pushed onto the playback queue
  double get_avg_frame_rate()
    {
      if ((num_updates_since_reset<2) || (mean_time_between_frames <= 0.0001))
        return -1.0;
      else
        return 1.0 / mean_time_between_frames;
    }
private: // Data
  int num_updates_since_reset; // Number of calls to `update' since `reset'
  int last_frame_idx; // Allows us to ignore duplicate calls to `update'
  double cur_time_at_last_update; // Value of `cur_time' in last `update' call
  double mean_time_between_frames;
};

/*****************************************************************************/
/*                              kdms_rel_dims                                */
/*****************************************************************************/

struct kdms_rel_dims {
  /* Used to represent the centre and dimensions of a rectangular region,
     relative to a containing rectangle whose identity is implied by the
     context. */
  kdms_rel_dims() { reset(); }
  void reset() { centre_x=centre_y=0.5; size_x=size_y=1.0; }
    /* Initializes the region to one which exactly covers its container. */
  bool set(kdu_dims region, kdu_dims container);
    /* Initializes the `region' relative to `container'.  Returns true if
       the `region' is a subset of or equal to `container'.  Otherwise, the
       function clips the `region' to the `container' but returns false. */
  bool set(kdu_dims region, kdu_coords container_size)
    { kdu_dims cont; cont.size=container_size; return set(region,cont); }
  kdu_dims apply_to_container(kdu_dims container);
    /* Expands the relative parameters into an absolute region, relative to
       the supplied `container'. */
  bool is_partial()
    { return ((size_x != 1.0) || (size_y != 1.0) ||
              (centre_x != 0.5) || (centre_y != 0.5)); }
    /* Returns true if the containing rectangle is only partially covered by
       the region. */
  void transpose()
    { double tmp; tmp=centre_x; centre_x=centre_y; centre_y=tmp;
      tmp=size_x; size_x=size_y; size_y=tmp; }
    /* Transpose the region. */
  void vflip() { centre_y=1.0-centre_y; }
    /* Flip the region vertically. */
  void hflip() { centre_x=1.0-centre_x; }
    /* Flip the region horizontally. */
  double centre_x, centre_y;
  double size_x, size_y;
};

/*****************************************************************************/
/*                              kdms_rel_focus                               */
/*****************************************************************************/

struct kdms_rel_focus {
public: // Member functions
  kdms_rel_focus() { reset(); }
  void reset()
    { codestream_idx=layer_idx=frame_idx=-1;
      region.reset(); layer_region.reset(); codestream_region.reset(); }
  bool is_valid() { return ((codestream_idx&layer_idx&frame_idx) >= 0); }
    /* Returns true if any of the `codestream_idx', `layer_idx' or `frame_idx'
       members is non-negative. */
  bool operator!() { return !is_valid(); }
    /* Returns true if `is_valid' is false. */
  void hflip()
    { region.hflip(); layer_region.hflip(); codestream_region.hflip(); }
  void vflip()
    { region.vflip(); layer_region.vflip(); codestream_region.vflip(); }
  void transpose()
    { region.transpose(); layer_region.transpose();
      codestream_region.transpose(); }
  bool get_centre_if_valid(double &centre_x, double &centre_y)
    { if (!is_valid()) return false;
      centre_x=region.centre_x; centre_y=region.centre_y; return true; }
public: // Data
  int codestream_idx; // -ve unless focus fully contained within a codestream
  int layer_idx; // -ve unless focus fully contained in a compositing layer
  int frame_idx; // -ve unless focus defined on a JPX or MJ2 frame
  kdms_rel_dims region; // See below
  kdms_rel_dims layer_region; // See below
  kdms_rel_dims codestream_region; // See below
};
  /* Notes:
       The `region' member holds the location and dimensions of the focus
     box relative to the full image, whatever that might be.  If `frame_idx'
     is non-negative, the full image is a composed JPX frame or an MJ2 frame.
     Otherwise, if `layer_idx' is non-negative, the full image is a
     single compositing layer.  Otherwise, if `codestream_idx' must be
     non-negative and the full image is a single component from a single
     codestream.  If none of the image entity indices is non-negative, the
     `region', `layer_region' and `codestream_region' members are all invalid.
        The `layer_region' member holds the location and dimensions of the
     focus region relative to the compositing layer identified by `layer_idx',
     if it is non-negative.
        The `codestream_region' member holds the location and dimensions of
     the focus region relative to the codestream identified by
     `codestream_idx', if it is non-negative. */

/*****************************************************************************/
/*                               kdms_renderer                               */
/*****************************************************************************/

class kdms_renderer {
public: // Main public member functions
  kdms_renderer(kdms_window *owner,
                kdms_document_view *doc_view,
                NSScrollView *scroll_view,
                kdms_window_manager *window_manager);
  ~kdms_renderer();
  void set_key_window_status(bool is_key_window);
    /* Called when the associated window gains or relinquishes key
       status. */
  void perform_essential_cleanup_steps();
    /* This function performs only those cleanup steps which are required to
       correctly manage non-memory resources (generally, temporary files).  It
       is invoked if the application is about to terminate without giving
       the destructor an opportunity to execute. */
  void close_file();
  void open_as_duplicate(kdms_renderer *src);
    /* Called to open the file/URL which is currently opened by `src' within
       the current window.  This function indirecly invokes `open_file'. */
  void open_file(const char *filename_or_url);
    /* This function enters with `filename_or_url'=NULL if it is being called
       to complete an overlapped opening operation.  This happens with the
       `kdu_client' compressed data source, which must first be connected
       to a server and then, after some time has passed, becomes ready
       to actually open a code-stream or JP2-family data source. */
  void view_dims_changed();
    /* This function is typically invoked automatically when the
       `doc_view' object's bounds or frame rectangle change, e.g., due
       to resizing or scrolling. */
  NSRect convert_region_to_doc_view(kdu_dims region);
    /* Converts a `region', defined in the same coordinate system as
       `view_dims', `buffer_dims' and `image_dims', to a region within
       the `doc_view' coordinate system, which runs from (0,0) at
       its lower left hand corner to (image_dims.size.x*pixel_scale-1,
       image_dims.size.y*pixel_scale-1) at its upper right hand corner. */
  kdu_dims convert_region_from_doc_view(NSRect doc_rect);
    /* Does the reverse of `convert_region_to_doc_view', except that the
       returned region is expanded as required to ensure that it covers
       the `doc_rect' rectangle. */
  NSPoint convert_point_to_doc_view(kdu_coords point);
    /* Converts a `point', defined in the same coordinate system as
       `view_dims', `buffer_dims' and `image_dims', to a loction within
       the `doc_view' coordinate system, which runs from (0,0) at its
       lower left hand corner to (image_dims.size.x*pixel_scale-1,
       image_dims.size.y*pixel_scale-1) at its upper right hand corner. */
  kdu_coords convert_point_from_doc_view(NSPoint point);
    /* Does the reverse of `convert_point_to_doc_view'. */
  bool paint_region(kdu_dims region, CGContextRef graphics_context,
                    NSPoint *presentation_offset=NULL);
      /* This function does all the actual work of drawing imagery
       to the window.  The `region' is expressed in the same coordinate
       system as `view_dims' and `image_dims'.  Returns false if nothing
       can be painted, due to the absence of any rendering machinery
       at the present time.  In this case, the caller should take action
       to erase the region itself.
          If this function is called from `paint_view_from_presentation_thread'
       its `presentation_offset' argument will be non-NULL.  In this
       case, the *`presentation_offset' value is added from the
       locations of the region which is painted, to account for the fact
       that the graphics context is based on the window geometry rather
       than the document view geometry. */
  void lock_view()
    { if (in_playmode) view_mutex_locked=view_mutex.lock(); }
  void unlock_view()
    { if (view_mutex_locked) { view_mutex_locked=false; view_mutex.unlock();} }
    /* These functions are called to take out an exclusive lock on the
       image view for the purpose of painting.  As a general rule, anything
       which might affect the behaviour of `paint_region' should be bracketed
       by calls to `lock_view' and `unlock_view'.  This includes calls to
       `paint_region' themselves, as well as anything which alters
       `view_dims', `focus_box' or `buffer'. */
  void paint_view_from_presentation_thread(NSGraphicsContext *gc);
    /* This function is used to paint directly to the `doc_view' object
       from the presentation thread.  Calls to this function arrive from the
       `kdms_frame_presenter' object when it needs to present a frame.
       The `lock_view' and `unlock_view' functions are used to ensure
       that the underlying resources (`view_dims' and `buffer' in particular)
       disappear or are changed while we are painting the view. */
  void display_status();
    /* Displays information in the status bar.  The particular type of
       information displayed depends upon the value of the `status_id'
       member variable.  If `playmode_frame_idx' is non-negative, it holds the
       index of the frame currently being presented in playmode, as opposed to
       `frame_idx' which holds the index of the frame being (or most recently)
       rendered. */
  bool on_idle();
    /* This function is called when the run-loop is about to go idle.
       This is where we perform our decompression and render processing.
       The function should return true if a subsequent call to the function
       would be able to perform additional processing.  Otherwise, it
       should return false.  The function is invoked from our run-loop
       observer, which sits inside the `kdms_controller' object. */
  bool refresh_display(bool new_imagery_only=false);
    /* Called from `initialize_regions' and also from any point where the
       image needs to be decompressed again.  If `new_imagery_only', the
       function does nothing unless a compositing layer or pending MJ2 frame
       change becomes available for the first time as a result of newly
       available codestream content.  The function can be called immediately
       with this option if any new data arrives in the cache from a JPIP
       server -- without the `new_imagery_only' argument being true, the
       function should be called at most episodically, to avoid wasting too
       much processing time.  The function returns true if and only if
       something was refreshed by a call to `compositor->refresh'. */
  void wakeup(double scheduled_wakeup_time, double current_time);
    /* This function is called if a wakeup was previously scheduled, once
       the scheduled time has arrived.  For convenience, the function is
       passed the original scheduled wakeup time and the current time. */
  void client_notification();
    /* This function is called if the JPIP client used by this object has
       changed the cache contents or the status message. */
  void set_codestream(int codestream_idx);
    /* Called from `kdms_metashow' when the user clicks on a code-stream box,
       this function switches to single component mode to display the
       image components of the indicated code-stream. */
  void set_compositing_layer(int layer_idx);
    /* Called from `kdms_metashow' when the user clicks on a compositing layer
       header box, this function switches to single layer mode to display the
       indicated compositing layer. */
  void set_compositing_layer(kdu_coords point);
    /* This form of the function determines the compositing layer index
       based on the supplied location.  It searches for the top-most
       compositing layer which falls underneath the current mouse position
       (given by `point', relative to the upper left corner of the view
       window).  If none is found, no action is taken. */
  bool set_frame(int frame_idx);
    /* Called from `kd_playcontrol' and also internally, this function is
       used to change the frame within an MJ2 track or a JPX animation.
       It invokes `change_frame_idx' but also configures the processing
       machinery to decompress the new frame.  Values which are currently
       illegal will be corrected automatically.  The function returns true
       if it produced a change in the current frame. */
  void stop_playmode();
    /* Wraps up all the functionality to move out of video playback mode. */
  void start_playmode(bool reverse);
    /* Wraps up all the functionality to commence video playback mode.
       The way in which video is played depends upon the existing state of
       the following parameters: `use_native_timing', `custom_fps',
       `rate_multiplier' and `playmode_repeat', each of which may be configured
       using menu commands.  The `reverse' argument indicates whether the
       video playback mode will be entered for playing video in the forward
       or the reverse direction. */
  void adjust_playclock(double delta)
    { if (in_playmode) playclock_offset += delta; }
    /* This function is called by the window manager whenever any video
       playback window finds that it is getting behind and needs to make
       adjustments to its playback clock, so that future playback will not
       try too aggressively to make up for lost time.  The `delta' value is
       added to the value of `playclock_offset', if we are in playmode.  If
       not, it doesn't matter.  Doing things in this way allows the playclock
       adjustments to be broadcast to all windows so that they can remain
       roughly in sync. */
// ----------------------------------------------------------------------------
public: // Public member functions which change the appearance
  bool get_max_scroll_view_size(NSSize &size);
    /* This function returns false and does nothing unless there is a valid
       rendering configuration installed.  If there is, the function returns
       true after setting `size' to the maximum size that the scroll-view's
       frame can take on. */
  void scroll_relative_to_view(float h_scroll, float v_scroll);
    /* This function causes the displayed region to scroll by the indicated
       amounts, expressed as a fraction of the dimensions of the currently
       visible region of the image. */
  void scroll_absolute(float h_scroll, float v_scroll, bool use_doc_coords);
    /* This function causes the displayed region o scroll by the indicated
       amount in the horizontal and vertical directions.  If `use_doc_coords'
       is false, the scrolling displacement is expressed relative to the
       canvas coordinate system used by `view_dims' and `image_dims'.
       Otherwise the scrolling displacement is expressed relative to the
       coordinate system used by `doc_view', with `scroll_y' representing
       an upward rather than downward scrolling amount. */
  void set_focus_box(kdu_dims new_box);
    /* Called from the `doc_view' object when the user finishes entering a
       new focus box.  Note that the coordinates of `new_box' have already
       been converted to the same coordinate system as `view_dims',
       `buffer_dims' and `image_dims', using `convert_region_from_doc_view'. */
  bool need_focus_outline(kdu_dims *focus_box);
    /* This function returns true if a focus box needs to be outlined by
       the `kdms_document_view::drawRect' function which is calling it,
       setting the current focus regin (in the `view_dims' coordinate system)
       into the supplied `focus_box' object. */
// ----------------------------------------------------------------------------
public: // Public member functions which are used for metadata
  void set_metadata_catalog_source(kdms_catalog *source);
    /* If `source' is nil, this function releases any existing
       `catalog_source' object.  Otherwise, the function installs the new
       object as the internal `catalog_source' member and invokes the
       [source update_metadata] function to parse JPX metadata into the
       newly created catalog. */
  void reveal_metadata_at_point(kdu_coords point, bool dbl_click=false);
    /* If the metadata catalog is open, this function attempts
       to find and select an associated label within the metadata catalog.
       Most of the work is done by `kdms_catalog:select_matching_metanode'.
       If the `metashow' window is open, the function also attempts to select
       the appropriate metadata within the metashow window, using the
       `kdms_metashow:select_matching_metanode' function.
       [//]
       If `dbl_click' is true, this function is being called in response to
       a mouse double-click operation, which causes the node to be
       passed to `show_imagery_for_metanode'. */
  void edit_metadata_at_point(kdu_coords *point=NULL);
    /* If `point' is NULL, the function obtains the location at which to
       edit the metadata from the current mouse position.  Otherwise, `point'
       is interpreted with respect to the same coordinate system as
       `view_dims', `buffer_dims' and `image_dims'. */
  void edit_metanode(jpx_metanode node, bool include_roi_or_numlist=false);
    /* Opens the metadata editor to edit the supplied node and any of its
       descendants.  This function may be called from the `kdms_metashow'
       object if the user clicks the edit button.  If the node is not
       directly editable, the function searches its descendants for a
       collection of top-level editable objects.  If the `node' itself is
       directly editable and `include_roi_or_numlist' is true, the function
       searches up through the node's ancestors until it comes to an ROI
       node or a numlist node.  If it comes to an ROI node first, the ROI
       node forms the root of the edit list; if it comes to a numlist node
       first, the numlist node's descendant from which `node' descends
       forms the root of the edit list; otherwise, `node' is the root
       of the edit list.  In all cases, editing starts at `node' rather
       than any ancestor node which is set as the root of the edit list.
       However, the user may navigate up to any ancestors which may exist
       within the edit list. */
  bool flash_imagery_for_metanode(jpx_metanode node);
    /* This function does not alter the current view (unlike
       `show_imagery_for_metanode'), but if a region of interest (or
       perhaps a specific codestream or compositing layer) identified by
       the supplied `node' is visible within the current view and does not
       occupy the entire view, it is temporarily highlighted.  This is done
       only if overlays are enabled, in which case the overlay is temporarily
       limited to `node' and the overlay brightness and dwell periods are
       controlled by a highlight state machine -- once the machine exits,
       regular overlay painting is restored. */
  bool show_imagery_for_metanode(jpx_metanode node,
                                 kdu_istream_ref *istream_ref=NULL);
    /* Changes the current view so as to best reveal the imagery associated
       with the supplied metadata node.  The function examines the node and
       its ancestors to find the first ROI (region of interest) node and the
       first numlist node it can.  If there is no numlist, but there is an ROI
       node, the ROI is associated with the first codestream.  If the numlist
       node's image entities do no intersect with what is being displayed
       or none of an ROI node's associated codestreams are being displayed, the
       display is changed to include at least one of the relevant image
       entities.  If there is an ROI node, the imagery is panned and zoomed
       in order to make the region clearly visible and the focus box is
       adjusted to fit around the ROI node's bounding box.
          During interactive JPIP browsing, it can happen that the image
       information required to complete a call to this function are not yet
       available in the cache.  When this happens, the function does as much
       as it can but leaves behind a record of the `node' argument in the
       `pending_show_metanode' member variable, so that the function can be
       automatically called again when new data arrives in the cache.  Such
       pending show events can potentially disrupt future user navigation, so
       the `cancel_pending_show' function is provided, to be called in the
       event of user interaction which might invalidate a pending attempt to
       change the display.  That function is invoked under just about any
       user interaction events which could affect the main image view.
          If `node' happens to be an empty interface, the function does
       nothing.  On the other hand, if the most recent call to this function
       supplied exactly the same `node' and if multiple frames (in frame
       composition mode) or multiple compositing layers (in single compositing
       layer rendering mode) or multiple codestreams (in single codestream
       rendering mode) can be considered to be associated with the `node',
       this function advances the view to the next associated one.  To enable
       this behaviour, the function saves the identity of the `node' supplied
       most recently to this funcion in the `last_show_metanode' variable.
          The function returns false if it was unable to find compatible
       imagery, perhaps because the relevant imagery has not yet become
       available in a dynamic cache, as explained above.  Otherwise, the
       function returns true, even if no change was made in the image
       being viewed -- so long as it is compatible.
          The `istream_ref' argument can be used to obtain the identity of
       the imagery stream (istream) associated with a successful outcome of
       the function -- the value at `istream_ref' may actually be modified
       even if the function returns false, but in that case the value
       should be ignored.  If `node' identified a compositing layer,
       the corresponding ilayer's primary istream is returned.  If `node'
       identified a specific codestream, the corresponding istream is
       returned.  If `node' identified a specific region of interest, the
       istream against which the region is registered is returned here.
       If none of the above cases hold, `istream_ref' is set to a null
       reference.  The `istream_ref' argument is particularly useful in
       view of the fact that the `node' may be associated with many
       codestreams or compositing layers and that each individual
       codestream or compositing layer may be associated with multiple
       imagery layers in the current composition, so it can be difficult
       or even impossible otherwise for the caller to figure out which
       imagery layer was the one used by a successful call to this
       function to base the displayed content. */
  void cancel_pending_show();
    /* This function cancels any internal record of a pending call to
       `show_imagery_for_metanode' which is to be re-attempted when the
       cache contents are augmented by a JPIP client.  The function is
       invoked prior to any user interaction events which might affect the
       rendering state -- actually, any menu command which might be
       routed to the window and any key or mouse events in the main image
       view.  The only operations which do not cancel pending show events
       are those which take place exclusively within the catalog panel
       and/or a metashow window. */
  jpx_meta_manager get_meta_manager()
    { return (!jpx_in)?jpx_meta_manager():jpx_in.access_meta_manager(); }
    /* Returns an empty interface unless there is an open JPX file.  This
       function is used by the `metashow' tool to cross-reference meta data
       which it directly parses with that which is currently maintained by
       the JPX meta-manager, possibly having been edited. */
  void metadata_changed(bool new_data_only);
    /* This function is called from within the metadata editor when changes
       are made to the metadata structure, which might need to be reflected in
       the overlay display and/or an open metashow window.  If `new_data_only'
       is true, no changes have been made to existing metadata. */
  void update_client_metadata_of_interest()
    { if (jpip_client != NULL) update_client_window_of_interest(); }
    /* This function is called by the `catalog_source' if any change occurs
       which may result in the need to change the metadata currently requested
       from the client.  The function calls `update_client_window_of_interest',
       which in turn calls `kdms_catalog:generate_metadata_requests'. */
// ----------------------------------------------------------------------------
public: // Public member functions which are used for shape editing
  bool set_shape_editor(jpx_roi_editor *ed,
                        kdu_istream_ref ed_istream=kdu_istream_ref(),
                        bool hide_path_segments=true,
                        int fixed_path_thickness=0,
                        bool scribble_mode=false);
    /* Installs or removes a shape editor.  Used by the
       `kdms_metadata_editor' object.  Simultaneously sets up the
       configuration state parameters for shape editing -- see
       `update_shape_editor' for more on these. */
  void update_shape_editor(kdu_dims region, bool hide_path_segments,
                           int fixed_path_thickness, bool scribble_mode);
    /* Causes the `region' being edited by an active shape editor to
       be repainted where `region' is expressed using the coordinate system of
       the shape editor itself; also potentially changes the `hide_shape_path',
       `fixed_shape_path_thickness' and `shape_scribbling_mode' members.  The
       first of these state variables determines whether or not `get_shape_path'
       can return non-zero.  If this value is changed, or if `region' is empty,
       the entire shape editor region is redrawn, regardless of the `region'
       argument.  If `fixed_path_thickness' is non-zero, future calls to the
       `shape_editor_adjust_path_thickness' function will adjust the path
       thickness -- this operation occurs each time an editing operation
       changes the region of interest.  If `scribble_mode' is true, mouse
       movements with the left button down are caught and used to generate
       calls to the shape editor's `jpx_roi_editor::add_scribble_point'
       function. */
  kdu_dims shape_editor_adjust_path_thickness()
    {
      bool sc; // Success value not used
      if ((shape_editor == NULL) || (fixed_shape_path_thickness <= 0))
        return(kdu_dims());
      return shape_editor->set_path_thickness(fixed_shape_path_thickness,sc);
    }
    /* This function automatically invokes `set_path_thickness' if and only
       if a fixed shape path thickness is currently installed, as determined
       by recent calls to `set_shape_editor' or `update_shape_editor'. */
  jpx_roi_editor *get_shape_editor() { return shape_editor; }
  bool get_shape_scribble_mode() { return shape_scribbling_mode; }
  jpx_roi_editor *
    map_shape_point_from_doc_view(NSPoint point, kdu_coords &mapped_point)
    {
      kdu_coords pt = convert_point_from_doc_view(point);
      kdu_dims regn; regn.pos=pt; regn.size.x=regn.size.y=1;
      if ((shape_editor == NULL) || (compositor == NULL) ||
          !compositor->map_region(regn,shape_istream))
        return NULL;
      mapped_point = regn.pos; return shape_editor;
    }
    /* Maps a `point' on the `doc_view' coordinate system to the high
       resolution codestream canvas coordinate system of the codestream
       associated with `shape_istream', adjusted so that it expresses
       the location relative to the start of the codestream's image region
       (i.e., following the convention of ROI description boxes).
       If `shape_istream' does not correspond to a valid istream reference
       into the current composition, or there is no currently active
       shape editor, the function returns NULL.  Otherwise, the function
       returns a pointer to the current shape editor, setting `mapped_point'
       to the relevant codestream location. */
  jpx_roi_editor *
    map_shape_point_to_doc_view(kdu_coords point, NSPoint &mapped_point)
    {
      kdu_dims regn; regn.pos=point; regn.size.x=regn.size.y=1;
      if ((shape_editor == NULL) || (compositor == NULL) ||
          (regn=compositor->inverse_map_region(regn,shape_istream)).is_empty())
        return NULL;
      mapped_point = convert_point_to_doc_view(regn.pos); return shape_editor;
    }
    /* Performs the reverse mapping to `map_shape_point_from_doc_view'. */
  kdu_dims get_visible_shape_region()
    {
      kdu_dims regn=view_dims;
      if ((shape_editor == NULL) || (compositor == NULL) ||
          !compositor->map_region(regn,shape_istream))
        regn.size.x=regn.size.y=0;
      return regn;
    }
    /* Maps the current `view_dims' region to the high resolution
       codestream canvas coordinate system of the codestream associated
       with `shape_istream', adjusted so that it expresses the location
       relative to the start of the codestream's image region (i.e., following
       the convention of ROI description boxes). */
  bool nudge_selected_shape_points(int delta_x, int delta_y,
                                   kdu_dims &update_region);
    /* This function returns false unless there is an active shape editor
       in which an anchor point is selected.  In that case, the function
       moves any currently selected anchor point `delta_x' samples to the
       right and `delta_y' samples down, where the samples in question here
       represent one pixel at the current rendering scale; it also sets
       `update_region' to the region reported by
       `jpx_roi_editor::move_selected_anchor'. */
  int get_shape_anchor(NSPoint &point, int which, bool sel_only, bool dragged);
    /* This function can be used to walk through the vertices which need
       to be displayed by the ROI shape editor.  The function essentially
       just invokes `shape_editor->get_anchor', converting the vertex
       location to the `doc_view' coordinate system.  If there is no active
       shape editor, or `which' is greater than or equal to the number of
       vertices to be painted, the function returns 0; otherwise, the return
       value is identical to the value returned by the corresponding call to
       `shape_editor->get_anchor'. */
  int get_shape_edge(NSPoint &from, NSPoint &to, int which,
                     bool sel_only, bool dragged);
    /* Same as `get_shape_anchor' but for edges. */
  int get_shape_curve(NSPoint &centre, NSSize &extent, double &alpha,
                      int which, bool sel_only, bool dragged);
    /* Similar to `get_shape_edge', but returns ellipses.  The ellipse is
       to be formed by starting with a cardinally aligned ellipse with
       centre at `centre', half-height `size.height' and half-width
       `size.width'.  This initial ellipse is to be skewed horizontally
       using the `alpha' parameter, meaning that a boundary point at
       location (x,y) is to be shifted to location
       (x+alpha*(y-centre.y), y). */
  int get_shape_path(NSPoint &from, NSPoint &to, int which);
    /* Same as `get_shape_edge', but returns path segments rather than
       region edges, by invoking the `shape_editor's `get_path_segment'
       function. */
  bool get_scribble_point(NSPoint &pt, int which)
    { kdu_coords shape_pt;
      return ((shape_editor != NULL) &&
              shape_editor->get_scribble_point(shape_pt,which) &&
              map_shape_point_to_doc_view(shape_pt,pt));
    }
    /* Similar to `get_shape_edge', but returns successive scribble points
       from the shape editor.  Returns false if there is none which the index
       `which'. */
// ----------------------------------------------------------------------------
private: // Private implementation functions
  void place_duplicate_window();
    /* This function is called from `open_file' right before the call to
       `initialize_regions' if a window is being duplicated.  It sizes and
       places the current window (using `window_manager->place_window') such
       that it holds the same contents as those identified by the `view_dims'
       member of `duplication_src'.  If the `duplication_src' is using a
       focus box, the placement size and view anchors are adjusted so that
       the new window will include only a small border around the focus
       region. */
  void invalidate_configuration();
    /* Call this function if the compositing configuration parameters
       (`single_component_idx', `single_codestream_idx', `single_layer_idx')
       are changed in any way.  The function removes all compositing layers
       from the current configuration installed into the `compositor' object.
       It sets the `configuration_complete' flag to false, the `bitmap'
       member to NULL, the image dimensions to 0, and posts a new client
       request to the `kdu_client' object if required.   After calling this
       function, you should generally call `initialize_regions'. */
  void initialize_regions(bool perform_full_window_init);
    /* Called by `open_file' after first opening an image (or JPIP client
       session) and also whenever the composition is changed (e.g., a change
       in the JPX compositing layers which are to be used, or a change to
       single-component mode rendering).  If `perform_full_window_init' is
       true, the function diescards any existing information about the
       view dimensions -- this is important in many settings. */
  void configure_overlays();
    /* Invokes `compositor->configure_overlays' with the appropriate
       parameters for the current state. */
  void size_window_for_image_dims(bool can_enlarge_window);
    /* This function is called when `image_dims' may have changed,
       including when `image_dims' is set for the first time.  It adjusts
       the size of the `doc_view' contents rectangle to reflect the new image
       dimensions.  If `can_enlarge_window' is true, it tries to adjust the
       size of the containing window, so that the image can be fully displayed.
       Otherwise, it acts only to reduce the window size, if the window is
       too large for the image that needs to be displayed. This process
       should result in the issuing of a notification message that the bounds
       and/or frame of the `doc_view' object have changed, which ultimately
       results in a call to the `view_dims_changed' function, but we invoke
       that function explicitly from here anyway, just to be sure that
       `view_dims' gets set. */
  float increase_scale(float from_scale, bool slightly);
    /* This function uses `kdu_region_compositor::find_optimal_scale' to
       find a suitable scale for rendering the current focus or view which
       is at least 1.5 times larger (1.1 times if `slightly' is true) than
       `from_scale' and ideally around twice (1.2 times if `slightly is true)
       `from_scale'. */
  float decrease_scale(float to_scale, bool slightly);
    /* Same as `increase_scale' but decreases the scale by at least 1.5
       times (1.1 times if `slightly' is true) `from_scale' and ideally
       around twice (1.2 times if `slightly' is true) `from_scale'. */
  void change_frame_idx(int new_frame_idx);
    /* This function is used to manipulate the `frame', `frame_idx',
       `frame_iteration' and `frame_instant' members.  It does not do anything
       else. */
  int find_compatible_frame_idx(int n_streams, const int *streams,
                                int n_layers, const int *layers,
                                int num_regions, const jpx_roi *regions,
                                int &compatible_codestream_idx,
                                int &compatible_layer_idx,
                                bool match_all_layers,
                                bool advance_to_next);
    /* This function is used when searching for a composited frame which
       matches a JPX metadata node, in the sense defined by the
       `show_imagery_for_metanode' function.
       [//]
       Starting from the current frame (if any), it scans through the frames
       in the JPX source, looking for one which matches at least one of the
       codestreams in `streams' (if there are any) and at least one of the
       compositing layers in `layers' (if there are any).  Moreover, if the
       `regions' array is non-empty, at least one of the regions must be
       visible, with respect to at least one of the codestreams in the
       `streams' array.  In any case, if a match is found and the `streams'
       array is non-empty, the matching codestream is written to the
       `compatible_codestream_idx' argument; otherwise, this argument is left
       untouched.  Similarly, if a match is found, the index of the matching
       compositing layer (the layer in the frame within which the match is
       found) is written to `compatible_layer_idx'.
       [//]
       If necessary, the function loops back to the first frame and continues
       searching until all frames have been examined.  If a match is found, the
       relevant frame index is returned.  Otherwise, the function
       returns -1.
       [//]
       If `match_all_layers' is true, the function tries to find a frame
       which contains all compositing layers identified in the `layers'
       array.  If there are none, you might call the function a second
       time with `match_all_layers' false.  This option is ignored unless
       `num_regions' and `n_streams' both equal 0.
       [//]
       If `advance_to_next' is true, the function tries to find the next
       frame (in cyclic sequence) beyond the current one. */
  bool adjust_rel_focus();
    /* This function uses the various members of `rel_focus' to determine
       whether or not the relative focus information determined when
       `rel_focus' was last configured needs to be adjusted to match the
       current image which is configured wihtin the `compositor'.  If
       adjustments do need to be made, function returns true.  The function
       is invoked at the end of the `initialize_regions' cycle once the
       new compositor configuration has been determined. */
  void calculate_rel_view_and_focus_params();
    /* Call this function to determine the centre of the current view relative
       to the entire image and to fill out the `rel_focus' structure with
       a complete description of the current focus box, if any.
       This information is used by subsequent calls to `set_view_size' and
       `get_new_focus_box' calls to position the view port and focus box.
       The most common use of this function is to preserve (or modify) the
       key properties  of the view port and focus box when the image is
       zoomed, rotated, flipped, or specialized to a different image
       component.  */
  void scroll_to_view_anchors();
    /* Adjust scrolling position so that the previously calculated view
       anchor is at the centre of the visible region, if possible. */
  void update_focus_box(bool view_may_be_scrolling=false);
    /* Called whenever a change in the focus box may be required.  This
       function calls `get_new_focus_box' and then repaints the region of
       the screen affected by any focus changes.  If the function is called
       in response to a change in the visible region, it is possible that
       a scrolling activity is in progress.  This should be signalled by
       setting the `view_may_be_scrolling' argument to true, since then any
       change in the focus box will require the entire view to be repainted.
       This is because the scrolling action moves portions of the surface
       and we cannot be sure in which order the focus box repainting and
       data moving will occur, since it is determined by the Cocoa
       framework. */
  kdu_dims get_new_focus_box();
    /* Performs most of the work of the `update_focus_box' and `set_focus_box'
       functions. */
//-----------------------------------------------------------------------------
private: // Functions specifically provided for JPIP interactive browsing  
  bool check_initial_request_compatibility();
    /* Checks the server response from an initial JPIP request (if the
       response is available yet) to determine if it is compatible with the
       mode in which we are currently trying to open the image.  If so, the
       function returns true.  Otherwise, the function returns false after
       first downgrading the image opening mode.  If we are currently trying
       to open an animation frame, we downgrade the operation to one in which
       we are trying to open a single compositing layer.  If this is also
       incompatible, we may downgrade the opening mode to one in which we
       are trying to open only a single image component of a single
       codestream.  This function is used if the client is created for
       non-interactive communication, or if the initial request used to
       connect to the server contains a non-trivial window of interest. */
  void update_client_window_of_interest();
    /* Called if the compressed data source is a "kdu_client"
       object, whenever the viewing region and/or resolution may
       have changed.  Also called from `update_client_metadata_of_interest'.
       This function computes and passes information about the client's
       window of interest and/or metadata of interest to the remote server. */  
  void change_client_focus(kdu_coords actual_resolution,
                           kdu_dims actual_region);
    /* Called if the compressed data source is a `kdu_client' object,
       whenever the server's reply to a region of interest request indicates
       that it is processing a different region to that which was requested
       by the client.  The changes are reflected by redrawing the focus
       box, as necessary, while being careful not to issue a request for a
       new region of interest from the server in the process. */
//-----------------------------------------------------------------------------
private: // Functions used to schedule events in time
  void schedule_frame_end_wakeup(double when)
    { next_frame_end_time = when; install_next_scheduled_wakeup(); }
  void schedule_overlay_flash_wakeup(double when);
  void schedule_render_refresh_wakeup(double when);
  void schedule_status_update_wakeup(double when);
    /* Each of the above functions is used to schedule a wakeup event for
       some time-based functionality.  The new time replaces any wakeup
       time currently scheduled for the relevant event (render-refresh,
       overlay-flash, frame-end or status-update, as appropriate).
       The `install_next_schedule_wakeup' function is invoked to actually
       schedule (or reschedule) a `wakeup' call for the earliest
       outstanding wakeup time, after any adjustments made by this function.
       Apart from frame-end wakeups (used in playback mode), the other wakeup
       times do not need to be very precise.  The relevant functions adjust
       these wakeup times if possible so as to minimize unnecessary wakeup
       calls. */
  void install_next_scheduled_wakeup();
    /* Service function used by the above functions and in other places, to
       compare the various wakeup times, find the most imminent one, and
       schedule a new `wakeup' call if necessary. */
  double get_current_time() { return CFAbsoluteTimeGetCurrent(); }
//-----------------------------------------------------------------------------
private: // Functions related to video playback
  void adjust_frame_queue_presentation_and_wakeup(double current_time);
    /* This function is called from `on_idle' and `wakeup' in playmode, to
       make whatever adjustments are required to the compositor's frame queue.
          It first determines how many frames can be safely popped from the
       head of the queue, so that there will always be at least one complete
       frame left for the frame presentation thread to present to the window.
       These popped frames must all have expired.  If there is a newly
       generated frame (not yet pushed onto the queue), we may be able to pop
       all frames which are currently on the queue.
          It then attempts to pop the determined number of expired frames and
       push any newly generated frame onto the queue.  This might not be
       possible if the frame presenter is in the process of presenting a frame,
       since we cannot disturb the head of the frame queue in that case.
          If the above steps result in a new frame on the head of the queue,
       the function informs the `frame_presenter' of this new frame.  If we
       need to wait until the frame at the head of the queue expires before we
       can do any more rendering of new frames (because the queue is full), the
       function schedules a wakeup call for the frame at the head of the
       queue's expiry date.
          This function also takes responsibility for stopping playmode, in
       the event that the last frame in the sequence has been pushed onto the
       queue and all frames have passed their expiry date. */
  double get_absolute_frame_end_time()
    { double rel_time;
      if (use_native_timing)
        rel_time = native_playback_multiplier *
          ((playmode_reverse)?(-frame_start):frame_end);
      else
        rel_time = custom_frame_interval *
          ((playmode_reverse)?(-frame_idx):(frame_idx+1));
      return rel_time + playclock_offset + playclock_base;
    }
    /* This is a utilty function to ensure that we always provide a consistent
       conversion from the track-relative frame times identified by the
       member variables `frame_start' and `frame_end' to absolute frame end
       times, as used during video playback.  The conversion depends upon the
       current playback settings (forward, reverse, frame rate multipliers,
       etc. */
  double get_absolute_frame_start_time()
    { double rel_time;
      if (use_native_timing)
        rel_time = native_playback_multiplier *
          ((playmode_reverse)?(-frame_end):frame_start);
      else
        rel_time = custom_frame_interval *
          ((playmode_reverse)?(-frame_idx-1):frame_idx);
      return rel_time + playclock_offset + playclock_base;
    }
//-----------------------------------------------------------------------------
private: // Functions related to file saving  
  bool save_over(); // Saves over the existing file; returns false if failed.
  void save_as_jpx(const char *filename, bool preserve_codestream_links,
                   bool force_codestream_links);
  void save_as_jp2(const char *filename);
  void save_as_raw(const char *filename);
  void copy_codestream(kdu_compressed_target *tgt, kdu_codestream src);
//-----------------------------------------------------------------------------
// Member Variables
public: // Reference to window and document view objects.
  kdms_window *window;
  kdms_document_view *doc_view;
  NSScrollView *scroll_view;
  kdms_window_manager *window_manager; // For wakeup calls
  kdms_metashow *metashow; // nil if metashow window not created yet
  kdms_catalog *catalog_source; // nil if metadata catalog is not visible
  kdms_metadata_editor *editor; // nil if metadata editor is not active
  bool catalog_closed; // True only if catalog was explicitly closed
  int window_identifier; // Unique identifier, displayed as part of title
  bool have_unsaved_edits;
// ----------------------------------------------------------------------------
private: // File and URL names retained by the `window_manager'
  bool processing_call_to_open_file; // Prevents recursive entry to `open_file'
  const char *open_file_pathname; // File we are reading; NULL for a URL
  const char *open_file_urlname; // JPIP URL we are reading; NULL if not JPIP
  const char *open_filename; // From `open_file_pathname' or `jpip_client'
  char *last_save_pathname; // Last file to which user saved anything 
  bool save_without_asking; // If the user does not want to be asked in future
  kdms_renderer *duplication_src; // Non-NULL inside `open_as_duplicate'
// ----------------------------------------------------------------------------
private: // Compressed data source and rendering machinery
  bool allow_seeking;
  int error_level; // 0 = fast, 1 = fussy, 2 = resilient, 3 = resilient_sop
  kdu_simple_file_source file_in;
  kdu_cache cache_in; // If open, the cache is bound to a JPIP client
  jp2_threadsafe_family_src jp2_family_in;
  jpx_source jpx_in;
  mj2_source mj2_in;
  jpx_composition composition_rules; // Composition/rendering info if available
  kdms_region_compositor *compositor;
  kdu_thread_env thread_env; // Multi-threaded environment
  int num_threads; // If 0 use single-threaded machinery with `thread_env'=NULL
  int num_recommended_threads; // If 0, recommend single-threaded machinery
  bool processing_suspended;
  double defer_process_regions_deadline; // Used in non-playback mode to
    // determine whether `compositor->process' should be called with the
    // `KDU_COMPOSIT_DEFER_REGION' flag.  This deadline is used to
    // encourage processing and region aggregation over short time scales.
// ----------------------------------------------------------------------------
private: // JPIP state information
  kdu_client *jpip_client; // Non-NULL if browsing via JPIP
  kdu_window client_roi;
  kdu_window tmp_roi;
  kdu_window_prefs client_prefs;
  int client_request_queue_id;
  bool one_time_jpip_request; // True if `jpip_client'!=NULL & one-time-request
  bool respect_initial_jpip_request; // True if we have a non-trivial initial
                  // request and the configuration is not yet valid
  bool enable_region_posting; // True if focus changes can be sent to server
  bool catalog_has_novel_metareqs; // If `catalog_source' posted metareqs
  kdu_long jpip_client_received_queue_bytes; // Used to check for new data for
  kdu_long jpip_client_received_total_bytes; // request queue and whole client
  bool jpip_client_received_bytes_since_last_refresh; // New data since refresh
  double jpip_refresh_interval; // Measured in seconds; depends on key status
// ----------------------------------------------------------------------------
private: // JPIP state information used only for `display_status'
  const char *jpip_client_status; // Status text last displayed
  kdu_long jpip_interval_start_bytes; // Bytes at start of rate interval
  double jpip_interval_start_time; // Active seconds at start of rate interval
  double jpip_client_data_rate; // Filtered local rate (bytes/sec) for queue
// ----------------------------------------------------------------------------
private: // External file state management (for editing and saving changes)
  kdms_file_services *file_server; // Created on-demand for metadata editing
// ----------------------------------------------------------------------------
private: // Modes and status which may change while the image is open
  int max_display_layers;
  bool transpose, vflip, hflip; // Current geometry flags.
  float min_rendering_scale; // Negative if unknown
  float max_rendering_scale; // Negative if unknown
  float rendering_scale; // Supplied to `kdu_region_compositor::set_scale'
  int single_component_idx; // Negative when performing a composite rendering
  int max_components; // -ve if not known
  int single_codestream_idx; // Used with `single_component'
  int max_codestreams; // -ve if not known
  int single_layer_idx; // For rendering one layer at a time; -ve if not in use
  int max_compositing_layer_idx; // -ve if not known
  int frame_idx; // Index of current frame
  double frame_start; // Frame start time (secs) w.r.t. track/animation start
  double frame_end; // Frame end time (secs) w.r.t. track/animation start
  jx_frame *frame; // NULL, if unable to open the above frame yet
  int frame_iteration; // Iteration within `frame' object, if repeated
  int num_frames; // -ve if not known
  jpx_frame_expander frame_expander; // Used to find frame membership
  bool configuration_complete; // If required compositing layers are installed
  bool fit_scale_to_window; // Used when opening file for first time
  kds_status_id status_id; // Info currently displayed in status window
  jpx_metanode pending_show_metanode; // See `show_imagery_for_metanode'
  jpx_metanode last_show_metanode; // See `show_imagery_for_metanode'
// ----------------------------------------------------------------------------
private: // State information for video/animation playback services
  kdu_mutex view_mutex; // Serialize access from main & presentation threads
  bool view_mutex_locked;
  bool in_playmode;
  bool playmode_repeat;
  bool playmode_reverse; // For playing backwards
  bool pushed_last_frame;
  bool use_native_timing;
  float custom_frame_interval; // Frame period if `use_native_timing' is false
  float native_playback_multiplier; // Amount to scale the native frame times
  double playclock_base; // These 2 quantities (offset+base) are added
  double playclock_offset; // to relative frame times.  We keep a separate base
                   // to subtract from display times to find `kdu_long'
                   // time-stamps even if `kdu_long' is only a 32-bit integer.
  int playmode_frame_queue_size; // Max size of jitter-absorbtion queue
  int playmode_frame_idx; // For `display_status'; -1 if no frame presented
  int num_expired_frames_on_queue; // Num frames waiting to be popped
  kdms_frame_presenter *frame_presenter; // Paints frames from separate thread
  kdms_playmode_stats playmode_stats;
// ----------------------------------------------------------------------------
private: // State information for scheduled activities
  double last_render_completion_time; // Used to find earliest re-render time
  double last_status_update_time; // Set only in playmode & JPIP cache contexts
  double next_render_refresh_time; // Time for next re-rendering to occur
  double next_overlay_flash_time; // Time for next overlay flash event (or -ve)
  double next_frame_end_time; // Expiry date of playmode display frame (or -ve)
  double next_status_update_time; // -ve if status updates not required
  double next_scheduled_wakeup_time; // -ve if no wakeup is scheduled
// ----------------------------------------------------------------------------
private: // Image and viewport dimensions
  int pixel_scale; // Number of display pels per image pel
  kdu_dims image_dims; // Dimensions & location of complete composed image
  kdu_dims buffer_dims; // Uses same coordinate system as `image_dims'
  kdu_dims view_dims; // Uses same coordinate system as `image_dims'
  double view_centre_x, view_centre_y;
  bool view_centre_known;
// ----------------------------------------------------------------------------
private: // Focus parameters
  bool enable_focus_box; // If disabled, focus box is identical to view port.
  bool highlight_focus; // If true, focus region is highlighted.
  kdu_dims focus_box; // Uses same coordinate system as `view_dims'.
  kdms_rel_focus rel_focus;
// ----------------------------------------------------------------------------
private: // Overlay parameters
  bool overlays_enabled;
  int overlay_flashing_state; // 0 means not flashing
  float overlay_nominal_intensity; // Basic value we get active intensity from
  int overlay_size_threshold; // Min composited size for an overlay to be shown
  jpx_metanode overlay_restriction;
  jpx_metanode last_selected_overlay_restriction;
  kdu_uint32 overlay_params[KDMS_NUM_OVERLAY_PARAMS];
  int overlay_highlight_state; // 0 if not flashing imagery for metanode
  jpx_metanode highlight_metanode;
// ----------------------------------------------------------------------------
private: // Image buffer and painting resources
  kdu_compositor_buf *buffer; // Get from `compositor' object; NULL if invalid
  CGColorSpaceRef generic_rgb_space;
  kdms_data_provider app_paint_data_provider; // For app `paint_region' calls
  kdms_data_provider presentation_data_provider; // For presentation thread
                                                 // calls to `paint_region'
  jpx_roi_editor *shape_editor;  // If these members are non-null shape editing
  kdu_istream_ref shape_istream; // is in progress; image display is dimmed and
                                 // `kdms_window' uses `get_shape_anchor' and
                                 // `get_shape_edge' to draw the shape.
  bool hide_shape_path; // If true, `get_shape_path' always returns 0.
  bool shape_scribbling_mode;
  int fixed_shape_path_thickness; // If >0, path thickness set after each move
//-----------------------------------------------------------------------------
// File Menu Handlers
public:
  void menu_FileDisconnectURL();
  bool can_do_FileDisconnectURL(NSMenuItem *item)
    { return ((jpip_client != NULL) &&
              jpip_client->is_alive(client_request_queue_id)); }
  void menu_FileSave();
  bool can_do_FileSave(NSMenuItem *item)
    { return ((compositor != NULL) && have_unsaved_edits &&
              (open_file_pathname != NULL) && !mj2_in.exists()); }
  void menu_FileSaveAs(bool preserve_codestream_links,
                       bool force_codestream_links);
  bool can_do_FileSaveAs(NSMenuItem *item)
    { return (compositor != NULL) && !in_playmode; }
  void menu_FileSaveReload();
  bool can_do_FileSaveReload(NSMenuItem *item)
    { return ((compositor != NULL) && have_unsaved_edits &&
              (open_file_pathname != NULL) && (!mj2_in.exists()) &&
              !in_playmode); }
  void menu_FileProperties();
  bool can_do_FileProperties(NSMenuItem *item)
    { return (compositor != NULL); }

public:
//-----------------------------------------------------------------------------
// View Menu Handlers
public:
  void menu_ViewHflip();
	bool can_do_ViewHflip(NSMenuItem *item)
    { return (buffer != NULL); }
  void menu_ViewVflip();
	bool can_do_ViewVflip(NSMenuItem *item)
    { return (buffer != NULL); }
  void menu_ViewRotate();
	bool can_do_ViewRotate(NSMenuItem *item)
    { return (buffer != NULL); }
  void menu_ViewCounterRotate();
	bool can_do_ViewCounterRotate(NSMenuItem *item)
    { return (buffer != NULL); }
  void menu_ViewZoomIn();
	bool can_do_ViewZoomIn(NSMenuItem *item)
    { return (buffer != NULL); }
  void menu_ViewZoomOut();
	bool can_do_ViewZoomOut(NSMenuItem *item)
    { return (buffer != NULL); }
  void menu_ViewZoomInSlightly();
	bool can_do_ViewZoomInSlightly(NSMenuItem *item)
    { return (buffer != NULL); }
  void menu_ViewZoomOutSlightly();
	bool can_do_ViewZoomOutSlightly(NSMenuItem *item)
    { return (buffer != NULL); }
  void menu_ViewWiden();
	bool can_do_ViewWiden(NSMenuItem *item)
    { return (buffer == NULL) ||
             ((view_dims.size.x < image_dims.size.x) ||
              (view_dims.size.y < image_dims.size.y)); }
  void menu_ViewShrink();
	bool can_do_ViewShrink(NSMenuItem *item)
    { return true; }
  void menu_ViewRestore();
	bool can_do_ViewRestore(NSMenuItem *item)
    { return (buffer != NULL) && (transpose || vflip || hflip); }
  void menu_ViewRefresh();
	bool can_do_ViewRefresh(NSMenuItem *item)
    { return (compositor != NULL); }
  void menu_ViewLayersLess();
	bool can_do_ViewLayersLess(NSMenuItem *item)
    { return (compositor != NULL) && (max_display_layers > 1); }
  void menu_ViewLayersMore();
	bool can_do_ViewLayersMore(NSMenuItem *item)
    { return (compositor != NULL) &&
             (max_display_layers <
              compositor->get_max_available_quality_layers()); }
  void menu_ViewOptimizeScale();
	bool can_do_ViewOptimizeScale(NSMenuItem *item)
    { return (compositor != NULL) && configuration_complete; }
  void menu_ViewPixelScaleX1();
	bool can_do_ViewPixelScaleX1(NSMenuItem *item)
    { [item setState:((pixel_scale==1)?NSOnState:NSOffState)];
      return true; }
  void menu_ViewPixelScaleX2();
	bool can_do_ViewPixelScaleX2(NSMenuItem *item)
    { [item setState:((pixel_scale==2)?NSOnState:NSOffState)];
      return true; }
  
  //---------------------------------------------------------------------------
  // Focus Menu Handlers
public:
  void menu_FocusOff();
	bool can_do_FocusOff(NSMenuItem *item)
    { return enable_focus_box; }
  void menu_FocusHighlighting();
	bool can_do_FocusHighlighting(NSMenuItem *item)
    { [item setState:(highlight_focus?NSOnState:NSOffState)];
      return true; }
  void menu_FocusWiden();
	bool can_do_FocusWiden(NSMenuItem *item)
    { return enable_focus_box; }
  void menu_FocusShrink();
	bool can_do_FocusShrink(NSMenuItem *item)
    { return true; }
  void menu_FocusLeft();
	bool can_do_FocusLeft(NSMenuItem *item)
    { return enable_focus_box && (focus_box.pos.x > image_dims.pos.x); }
  void menu_FocusRight();
	bool can_do_FocusRight(NSMenuItem *item)
    { return enable_focus_box &&
      ((focus_box.pos.x+focus_box.size.x) <
       (image_dims.pos.x+image_dims.size.x)); }
  void menu_FocusUp();
	bool can_do_FocusUp(NSMenuItem *item)
    { return enable_focus_box && (focus_box.pos.y > image_dims.pos.y); }
  void menu_FocusDown();
	bool can_do_FocusDown(NSMenuItem *item)
    { return enable_focus_box &&
      ((focus_box.pos.y+focus_box.size.y) <
       (image_dims.pos.y+image_dims.size.y)); }
  
  //---------------------------------------------------------------------------
  // Mode Menu Handlers
public:
  void menu_ModeFast();
	bool can_do_ModeFast(NSMenuItem *item)
    { [item setState:((error_level==0)?NSOnState:NSOffState)];
      return true; }
  void menu_ModeFussy();
	bool can_do_ModeFussy(NSMenuItem *item)
    { [item setState:((error_level==1)?NSOnState:NSOffState)];
      return true; }
  void menu_ModeResilient();
	bool can_do_ModeResilient(NSMenuItem *item)
    { [item setState:((error_level==2)?NSOnState:NSOffState)];
      return true; }
  void menu_ModeResilientSOP();
	bool can_do_ModeResilientSOP(NSMenuItem *item)
    { [item setState:((error_level==3)?NSOnState:NSOffState)];
      return true; }
  void menu_ModeSingleThreaded();
	bool can_do_ModeSingleThreaded(NSMenuItem *item)
    { [item setState:((num_threads==0)?NSOnState:NSOffState)];
      return true; }
  void menu_ModeMultiThreaded();
	bool can_do_ModeMultiThreaded(NSMenuItem *item); // See .mm file
  void menu_ModeMoreThreads();
	bool can_do_ModeMoreThreads(NSMenuItem *item)
    { return true; }
  void menu_ModeLessThreads();
	bool can_do_ModeLessThreads(NSMenuItem *item)
    { return (num_threads > 0); }
  void menu_ModeRecommendedThreads();
	bool can_do_ModeRecommendedThreads(NSMenuItem *item); // See .mm file
  
  //---------------------------------------------------------------------------
  // Navigation Menu Handlers
public:  
  void menu_NavComponent1();
	bool can_do_NavComponent1(NSMenuItem *item)
    { [item setState:(((compositor != NULL) &&
                       (single_component_idx==0))?NSOnState:NSOffState)];
      return (compositor != NULL) && (single_component_idx != 0); }
  void menu_NavMultiComponent();
	bool can_do_NavMultiComponent(NSMenuItem *item)
    {
      [item setState:(((compositor != NULL) &&
                       (single_component_idx < 0) &&
                       ((single_layer_idx < 0) || (num_frames==0) || !jpx_in))?
                      NSOnState:NSOffState)];
      return (compositor != NULL) &&
             ((single_component_idx >= 0) ||
              ((single_layer_idx >= 0) && (num_frames != 0) &&
               jpx_in.exists()));
    }
  void menu_NavComponentNext();
	bool can_do_NavComponentNext(NSMenuItem *item)
  { return (compositor != NULL) &&
           ((max_components < 0) ||
            (single_component_idx < (max_components-1))); }
  void menu_NavComponentPrev();
	bool can_do_NavComponentPrev(NSMenuItem *item)
    { return (compositor != NULL) && (single_component_idx != 0); }
  void menu_NavImageNext();
	bool can_do_NavImageNext(NSMenuItem *item); // Lengthy decision in .mm file
  void menu_NavImagePrev();
	bool can_do_NavImagePrev(NSMenuItem *item); // Lengthy decision in .mm file
  void menu_NavCompositingLayer();
	bool can_do_NavCompositingLayer(NSMenuItem *item)
    { [item setState:(((compositor != NULL) &&
                       (single_layer_idx >= 0))?NSOnState:NSOffState)];
      return (compositor != NULL) && (single_layer_idx < 0); }
  void menu_NavTrackNext();
	bool can_do_NavTrackNext(NSMenuItem *item)
    { return (compositor != NULL) && mj2_in.exists(); }

  //---------------------------------------------------------------------------
  // Metadata Menu Handlers
public:
  void menu_MetadataCatalog();
  bool can_do_MetadataCatalog(NSMenuItem *item)
    { [item setState:((catalog_source)?NSOnState:NSOffState)];
      return (catalog_source || jpx_in.exists()); }
  void menu_MetadataSwapFocus();
  bool can_do_MetadataSwapFocus(NSMenuItem *item)
    { return (catalog_source || jpx_in.exists()); }
  void menu_MetadataShow();
	bool can_do_MetadataShow(NSMenuItem *item)
    { [item setState:((metashow != nil)?NSOnState:NSOffState)];
      return true; }
  void menu_MetadataAdd();
  bool can_do_MetadataAdd(NSMenuItem *item)
    { return ((compositor != NULL) && jpx_in.exists() &&
              ((jpip_client == NULL) || !jpip_client->is_alive(-1))); }
  void menu_MetadataEdit();
  bool can_do_MetadataEdit(NSMenuItem *item)
    { return (compositor != NULL) && jpx_in.exists(); }
  void menu_MetadataDelete();
  bool can_do_MetadataDelete(NSMenuItem *item);
  void menu_MetadataFindOrphanedROI();
  bool can_do_MetadataFindOrphanedROI(NSMenuItem *item)
    { return (compositor != NULL) && (catalog_source != nil); }
  void menu_MetadataCopyLabel();
  bool can_do_MetadataCopyLabel(NSMenuItem *item);
  void menu_MetadataCopyLink();
  bool can_do_MetadataCopyLink(NSMenuItem *item)
    { return ((compositor != NULL) && jpx_in.exists()); }
  void menu_MetadataCut();
  bool can_do_MetadataCut(NSMenuItem *item);
  void menu_MetadataPasteNew();
  bool can_do_MetadataPasteNew(NSMenuItem *item);
  void menu_MetadataDuplicate();
  bool can_do_MetadataDuplicate(NSMenuItem *item);
  void menu_MetadataUndo();
  bool can_do_MetadataUndo(NSMenuItem *item)
    { int undo_elts; bool can_redo; if (shape_editor == NULL) return false;
      shape_editor->get_history_info(undo_elts,can_redo);
      return (undo_elts > 0); }
  void menu_MetadataRedo();
  bool can_do_MetadataRedo(NSMenuItem *item)
    { int undo_elts; bool can_redo; if (shape_editor == NULL) return false;
      shape_editor->get_history_info(undo_elts,can_redo); return can_redo; }
  void menu_OverlayEnable();
  bool can_do_OverlayEnable(NSMenuItem *item)
    { [item setState:(((compositor != NULL) && jpx_in.exists() &&
                       overlays_enabled)?NSOnState:NSOffState)];
      return (compositor != NULL) && jpx_in.exists(); } 
  void menu_OverlayFlashing();
  bool can_do_OverlayFlashing(NSMenuItem *item)
    { [item setState:(((compositor != NULL) && jpx_in.exists() &&
                       overlay_flashing_state &&
                       overlays_enabled)?NSOnState:NSOffState)];
      return (compositor != NULL) && jpx_in.exists() && overlays_enabled &&
             !in_playmode; }
  void menu_OverlayToggle();
  bool can_do_OverlayToggle(NSMenuItem *item)
    { return (compositor != NULL) && jpx_in.exists(); }
  void menu_OverlayRestrict();
  bool can_do_OverlayRestrict(NSMenuItem *item);
  void menu_OverlayLighter();
  bool can_do_OverlayLighter(NSMenuItem *item)
    { return (compositor != NULL) && jpx_in.exists() && overlays_enabled; }
  void menu_OverlayHeavier();
  bool can_do_OverlayHeavier(NSMenuItem *item)
    { return (compositor != NULL) && jpx_in.exists() && overlays_enabled; }
  void menu_OverlayDoubleSize();
  bool can_do_OverlayDoubleSize(NSMenuItem *item)
    { return (compositor != NULL) && jpx_in.exists() && overlays_enabled; }
  void menu_OverlayHalveSize();
  bool can_do_OverlayHalveSize(NSMenuItem *item)
    { return (compositor != NULL) && jpx_in.exists() && overlays_enabled &&
      (overlay_size_threshold > 1); }
  
  //---------------------------------------------------------------------------
  // Play Menu Handlers
public:
  bool source_supports_playmode()
    { return (compositor != NULL) &&
      (mj2_in.exists() || jpx_in.exists()) &&
      (num_frames != 0) && (num_frames != 1); }
  void menu_PlayStartForward() { start_playmode(false); }
	bool can_do_PlayStartForward(NSMenuItem *item)
    { return source_supports_playmode() &&
      (single_component_idx < 0) &&
      ((single_layer_idx < 0) || !jpx_in.exists()) &&
      (playmode_reverse || !in_playmode); }
  void menu_PlayStartBackward() { start_playmode(true); }
	bool can_do_PlayStartBackward(NSMenuItem *item)
    { return source_supports_playmode() &&
      (single_component_idx < 0) &&
      ((single_layer_idx < 0) || !jpx_in.exists()) &&      
      !(playmode_reverse && in_playmode); }
  void menu_PlayStop() { stop_playmode(); }
	bool can_do_PlayStop(NSMenuItem *item)
    { return in_playmode; }
  void menu_PlayRepeat() { playmode_repeat = !playmode_repeat; }
  bool can_do_PlayRepeat(NSMenuItem *item)
    { [item setState:((playmode_repeat)?NSOnState:NSOffState)];
      return source_supports_playmode(); }
  void menu_PlayNative();
  bool can_do_PlayNative(NSMenuItem *item);
  void menu_PlayCustom();
  bool can_do_PlayCustom(NSMenuItem *item);  
  void menu_PlayFrameRateUp();
  bool can_do_PlayFrameRateUp(NSMenuItem *item) 
    { return source_supports_playmode(); }
  void menu_PlayFrameRateDown();
  bool can_do_PlayFrameRateDown(NSMenuItem *item)
    { return source_supports_playmode(); }
  
  //---------------------------------------------------------------------------
  // Status Menu Handlers
public:  
  void menu_StatusToggle();
	bool can_do_StatusToggle(NSMenuItem *item)
    { return (compositor != NULL); }

  //---------------------------------------------------------------------------
  // Window Menu Handlers
public:  
	bool can_do_WindowDuplicate(NSMenuItem *item)
  { return (compositor != NULL); }  
};
