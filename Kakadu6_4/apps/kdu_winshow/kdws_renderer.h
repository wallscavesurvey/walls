/*****************************************************************************/
// File: kdws_renderer.h [scope = APPS/WINSHOW]
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
the image window.  The `kdws_renderer' object is created by (and conceptually
owned by) `kdws_frame_window'.  Menu commands and other user events received
by those objects are mostly forwarded to the renderer.
******************************************************************************/
#ifndef KDWS_RENDERER_H
#define KDWS_RENDERER_H

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#if !(defined _MSC_VER && (_MSC_VER >= 1300)) // If not .NET
#  define DWORD_PTR DWORD
#  define UINT_PTR kdu_uint32
#  define INT_PTR kdu_int32
#endif // Not .NET

#include "resource.h"       // main symbols
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_file_io.h"
#include "jpx.h"
#include "kdu_client.h"
#include "kdu_clientx.h"
#include "kdu_region_compositor.h"

// Declared here:
class kdws_file;
struct kdws_file_editor;
class kdws_file_services;
class kdws_notifier;
class kd_bitmap_buf;
class kd_bitmap_compositor;
class kdws_playmode_stats;
struct kdws_rel_dims;
struct kdws_rel_focus;
class kdws_renderer;

// Declared elsewhere
class kdws_string;
class kdws_manager;
class kdws_frame_presenter;
class kdws_frame_window;
class kdws_image_view;
class kdws_properties;
class kdws_url_dialog;
class kdws_metashow;
class kdws_metadata_editor;
class kdws_catalog;

#define KDWS_OVERLAY_BORDER 4
#define KDWS_NUM_OVERLAY_PARAMS 18 // Size of the array initializer below
#define KDWS_OVERLAY_PARAMS \
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

extern int kdws_utf8_to_unicode(const char *utf8_string,
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
/*                                 kdws_file                                 */
/*****************************************************************************/

class kdws_file {
private: // Private life-cycle functions
  friend class kdws_file_services;
  kdws_file(kdws_file_services *owner); // Links onto tail of `owner' list
  ~kdws_file(); // Unlinks from `owner' first.
public: // Member functions
  int get_id() { return unique_id; }
  /* Gets the unique identifier for this file, which can be used with
   `kdws_file_services::find_file'. */
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
  kdws_file_services *owner;
  kdws_file *next, *prev; // Used to build a doubly-linked list
  char *pathname;
  bool is_temporary; // If true, the file is deleted once `retain_count'=0.
};

/*****************************************************************************/
/*                             kdws_file_editor                              */
/*****************************************************************************/

struct kdws_file_editor {
  kdws_file_editor()
    {
      doc_suffix = app_pathname = app_name = NULL;
      next = NULL;
    }
  ~kdws_file_editor()
    {
      if (doc_suffix != NULL) delete[] doc_suffix;
      if (app_pathname != NULL) delete[] app_pathname;
      if (app_name != NULL) delete[] app_name;
    }
  char *doc_suffix; // File extension for matching document types
  char *app_pathname; // Either the application pathname or else the first
                      // part of a command to run the application.
  char *app_name; // Extracted from `app_pathname'.
  kdws_file_editor *next;
};

/*****************************************************************************/
/*                            kdws_file_services                             */
/*****************************************************************************/

class kdws_file_services {
public: // Member functions
  kdws_file_services(const char *source_pathname);
    /* If `source_pathname' is non-NULL, all temporary files are created by
     removing its file suffix, if any, and then appending to the name to
     make it unique.  This is the way you should create the object when
     editing an existing file.  Otherwise, if `source_pathname' is NULL,
     a base pathname is created internally using the `tmpnam' function. */
  ~kdws_file_services();
  bool find_new_base_pathname();
    /* This function sets the base pathname for temporary files based upon
     the ANSI tmpnam() function.  It erases any existing base pathname and
     generates a new one which is derived from the name returned by tmpnam()
     after first testing that the file returned by tmpnam() can be created.
     If the file cannot be created, the function returns false.  The function
     is normally called only internally from within `confirm_base_pathname'. */
  kdws_file *retain_tmp_file(const char *suffix);
    /* Generates a unique filename.  Does not try to create the file at this
     point, but may do so later.  In any case, the file will be deleted once
     it is no longer needed.  The returned object has a retain count of 1.
     Once you release the object, it will be destroyed, and the temporary
     file deleted.  */
  kdws_file *retain_known_file(const char *pathname);
    /* As for the last function, but the returned object represents a
     non-temporary file (i.e., one that will not be deleted when it is no
     longer required).  Generally, this is an existing file supplied by the
     user.  You should supply the full pathname to the file.  The returned
     object has a retain count of at least 1.  If the same file is already
     retained for some other purpose, the retain count may be greater. */
  kdws_file *find_file(int identity);
    /* This function is used to recover a file whose ID was obtained
     previously using `kdws_file::get_id'.  It provides a robust mechanism
     for accessing files whose identity is stored indirectly via an integer
     and managed by the `jpx_meta_manager' interface. */
  kdws_file_editor *get_editor_for_doc_type(const char *doc_suffix, int which);
    /* This function is used to access an internal list of editors which
     can be used for files (documents) with the indicated suffix (extension).
     The function returns NULL if `which' is greater than or equal to the
     number of matching editors for such files which are known.  If the
     function has no stored editors for the file, it attempts to find some,
     using operating-system services.  You can also add editors using the
     `add_editor_for_doc_type' function. */
  kdws_file_editor *add_editor_for_doc_type(const char *doc_suffix,
                                            const char *app_pathname);
    /* Use this function if you want to add custom editors for files
     (documents) with a given suffix (extension), or if you want to move an
     existing editor to the head of the list. */
private:
  void add_editor_progid_for_doc_type(const char *doc_suffix,
                                      kdws_string &progid);
    /* Used internally to add editors based on their ProgID's, as discovered
       by scanning the registry under HKEY_CLASSES_ROOT/.<suffix>. */
  void confirm_base_pathname(); // Called when `retain_tmp_file' is first used
private: // Data
  friend class kdws_file;
  kdws_file *head, *tail; // Linked list of files
  int next_unique_id;
  char *base_pathname; // For temporary files
  bool base_pathname_confirmed; // True once we know we can write to above
  kdws_file_editor *editors; // Head of the list; new editors go to the head
};

/*****************************************************************************/
/*                               kd_bitmap_buf                               */
/*****************************************************************************/

class kd_bitmap_buf : public kdu_compositor_buf {
  public: // Member functions and overrides
    kd_bitmap_buf()
      { bitmap=NULL; next=NULL; }
    virtual ~kd_bitmap_buf()
      {
        if (bitmap != NULL) DeleteObject(bitmap);
        buf = NULL; // Remember to do this whenever overriding the base class
      }
    void create_bitmap(kdu_coords size);
      /* If this function finds that a bitmap buffer is already available,
         it returns without doing anything. */
    HBITMAP access_bitmap(kdu_uint32 * &buf, int &row_gap)
      { buf=this->buf; row_gap=this->row_gap; return bitmap; }
    void flush_from_cache();
      /* This function flushes all cache-lines associated with the buffer
         to external memory in preparation for block transfer to the
         display.  This turns out to resolve some problems with the Windows
         BitBlt and StretchBlt operations during video display, in which
         occasional scanlines can contain quite different data to their
         surrounding ones.  The reason for this is not entirely clear, but
         my best guess is that when some of the imagery that needs to be
         blitted is in main memory and some resides within the CPU cache, the
         transfer to graphics memory is performed out of order; this can
         interact badly with the screen tearing associated with transferring
         new data to graphics memory in a manner which is not synchronized
         with the vertical frame blanking period.  Later, we will migrate
         the kdu_show application to use a tearing-free transfer technique
         for video rendering, but for the moment this simple function resolves
         the most annoying artefacts associated with screen tearing. */
  protected: // Additional data
    friend class kd_bitmap_compositor;
    HBITMAP bitmap; // NULL if not using a bitmap
    BITMAPINFO bitmap_info; // Description for `bitmap' if non-NULL
    kdu_coords size;
    kd_bitmap_buf *next;
  };

/*****************************************************************************/
/*                            kd_bitmap_compositor                           */
/*****************************************************************************/

class kd_bitmap_compositor : public kdu_region_compositor {
  /* This class augments the platorm independent `kdu_region_compositor'
     class with display buffer management logic which allocates system
     bitmaps.  You can take this as a template for designing richer
     buffer management systems -- e.g. ones which utilize display hardware
     through DirectDraw. */
  public: // Member functions
    kd_bitmap_compositor(kdu_thread_env *env=NULL,
                         kdu_thread_queue *env_queue=NULL)
                         : kdu_region_compositor(env,env_queue)
      { active_list = free_list = NULL; }
    ~kd_bitmap_compositor()
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
    kd_bitmap_buf *get_composition_bitmap(kdu_dims &region)
      { return (kd_bitmap_buf *) get_composition_buffer(region); }
      /* Use this instead of `kdu_compositor::get_composition_buffer'. */
  private: // Data
    kd_bitmap_buf *active_list;
    kd_bitmap_buf *free_list;
  };

/*****************************************************************************/
/*                            kdws_playmode_stats                            */
/*****************************************************************************/

class kdws_playmode_stats {
public: // Member functions
  kdws_playmode_stats() { reset(); }
  void reset() // Called by `kdws_renderer::start_playmode'
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
/*                              kdws_rel_dims                                */
/*****************************************************************************/

struct kdws_rel_dims {
  /* Used to represent the centre and dimensions of a rectangular region,
     relative to a containing rectangle whose identity is implied by the
     context. */
  kdws_rel_dims() { reset(); }
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
/*                              kdws_rel_focus                               */
/*****************************************************************************/

struct kdws_rel_focus {
public: // Member functions
  kdws_rel_focus() { reset(); }
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
  kdws_rel_dims region; // See below
  kdws_rel_dims layer_region; // See below
  kdws_rel_dims codestream_region; // See below
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
/*                                kdws_renderer                              */
/*****************************************************************************/

class kdws_renderer: public CCmdTarget
{
public: // Public member functions for use by other objects
  kdws_renderer(kdws_frame_window *owner,
                kdws_image_view *image_view,
                kdws_manager *manager);
  ~kdws_renderer();
  void set_key_window_status(bool is_key_window);
    /* Called when the associated frame window gains or relinquishes key
       status within the application */
  void close_file();
  void open_as_duplicate(kdws_renderer *src);
    /* Called to open the file/URL which is currently opened by `src' within
       the current window.  This function indirecly invokes `open_file'. */
  void open_file(const char *filename_or_url);
    /* This function enters with `filename_or_url'=NULL if it is being called
       to complete an overlapped opening operation.  This happens with the
       'kdu_client' compressed data source, which must first be connected
       to a server and then, after some time has passed, becomes ready
       to actually open a code-stream or JP2-family data source. */
  void perform_essential_cleanup_steps();
    /* Called immediately from the destructor and from `close_file' so as
       to ensure that important file saving/deleting steps are performed. */
  void suspend_processing(bool suspend) { processing_suspended = suspend; }
    /* Call this function to suspend or resume idle time processing of
       compressed image data.  This is useful when drawing onto the display
       surface, since idle time processing may cause its own calls to
       `paint_region'. */
  void set_view_size(kdu_coords size);
    /* This function should be called whenever the client view window's
       dimensions may have changed.  The function should be called at least
       whenever the WM_SIZE message is received by the client view window.
       The current object expects to receive this message before it can begin
       processing a newly opened image.  If `size' defines a larger region
       than the currently loaded image, the actual `view_dims' region used
       for rendering purposes will be smaller than the client view window's
       dimensions. */
  void set_hscroll_pos(int pos, bool relative_to_last=false);
    /* Identifies a new left hand boundary position for the current view.
       If `relative_to_last' is true, the new position is expressed relative
       to the existing left hand view boundary.  Otherwise, the new position
       is expressed relative to the image origin. */
  void set_vscroll_pos(int pos, bool relative_to_last=false);
    /* Identifies a new upper boundary position for the current view.
       If `relative_to_last' is true, the new position is expressed relative
       to the existing left hand view boundary.  Otherwise, the new position
       is expressed relative to the image origin. */
  void set_scroll_pos(int xpos, int ypos, bool relative_to_last=false);
    /* This is a combined scrolling function that allows scrolling in two
       directions simultaneously */
  void scroll_to_view_anchors();
    /* Adjust scrolling position so that the previously calculated view
       anchor is at the centre of the visible region, if possible. */
  RECT convert_region_to_image_view_rect(kdu_dims region)
    { RECT rect; rect.left=rect.right=rect.left=rect.bottom=0;
      if (!(region &= view_dims).is_empty())
        {
          region.pos -= view_dims.pos;
          rect.left = region.pos.x*pixel_scale;
          rect.top = region.pos.y*pixel_scale;
          rect.right = rect.left + region.size.x*pixel_scale;
          rect.bottom = rect.top + region.size.y*pixel_scale;
        }
      return rect;
    }
  void paint_region(CDC *dc, kdu_dims region, bool is_absolute=true);
    /* Paints a portion of the viewable region to the supplied device
       context.  The supplied `region' is expressed in the same absolute
       coordinate system as `view_dims', unless `is_absolute' is false, in
       which case `region.pos' identifies the location of the upper left
       hand corner of the region, relative to the current view.  If the
       region is not completely contained inside the current view region, or
       no file has yet been loaded, the background white colour is painted into
       the entire region first, and then the portion which intersects with the
       current view region is painted.  This allows for the possibility that
       the client view region might be larger than the rendering view region
       (e.g., because the image is too small to occupy the minimum window
       size).
          If `dc' is NULL, the function automatically acquires the relevant
       device context and releases it before returning. */
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
  void paint_view_from_presentation_thread()
    { lock_view();
      if (buffer != NULL)
        { paint_region(NULL,view_dims,true); buffer->flush_from_cache(); }
      unlock_view(); }
    /* This function is invoked from the presentation thread when the
       `buffer' needs to be drawn.  Painting is performed via `paint_region,
       but taking care to lock the image view. */
  void  display_status();
    /* Displays information in the status bar.  The particular type of
       information displayed depends upon the value of the `status_id'
       member variable. */
  bool on_idle();
    /* This function is called when the run-loop is about to go idle.
       This is where we perform our decompression and render processing.
       The function should return true if a subsequent call to the function
       would be able to perform additional processing.  Otherwise, it
       should return false.  The function is invoked indirectly from
       manager->OnIdle(), via `kdws_frame_window::on_idle'. */
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
  void set_focus_box(kdu_dims new_box);
    /* Called from the image_view object when the user finishes entering a
       new focus box.  Note that the coordinates of `new_box' are expressed
       relative to the view port. */
  void set_codestream(int codestream_idx);
    /* Called from `kdws_metashow' when the user clicks on a code-stream box,
       this function switches to single component mode to display the
       image components of the indicated code-stream. */
  void set_compositing_layer(int layer_idx);
    /* Called from `kd_metashow' when the user clicks on a compositing layer
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
public: // Public member functions which are used for metadata
  void set_metadata_catalog_source(kdws_catalog *source);
    /* If `source' is NULL, this function releases any existing
       `catalog_source' object.  Otherwise, the function installs the new
       object as the internal `catalog_source' member and invokes the
       `source->update_metadata' function to parse JPX metadata into the
       newly created catalog. */
  void reveal_metadata_at_point(kdu_coords point, bool dbl_click=false);
    /* If the metadata catalog is open, this function attempts
       to find and select an associated label within the metadata catalog.
       Most of the work is done by `kdws_catalog:select_matching_metanode'.
       If the `metashow' window is open, the function also attempts to select
       the appropriate metadata within the metashow window, using the
       `kdws_metashow:select_matching_metanode' function.
       [//]
       If `dbl_click' is true, this function is being called in response to
       a mouse double-click operation, which causes the item to be
       passed to `show_imagery_for_metanode'. */
  void edit_metadata_at_point(kdu_coords *point=NULL);
    /* If `point' is NULL, the function obtains the location at which to
       edit the metadata from the current mouse position.  Otherwise, `point'
       is interpreted relative to the upper-left hand corner of the image
       view window, except that any prevaling `pixel_scale' has already
       been taken into account. */
  void edit_metanode(jpx_metanode node, bool include_roi_or_numlist=false);
    /* Opens the metadata editor to edit the supplied node and any of its
       descendants.  This function may be called from the `kdws_metashow'
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
  bool show_imagery_for_metanode(jpx_metanode node);
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
       being viewed -- so long as it is compatible. */
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
       which in turn calls `kdws_catalog:generate_metadata_requests'. */
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
  void resize_stripmap(int min_width);
    /* This function is called whenever the current width of the strip map
       buffer is too small. */
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
       If `match_all_layers' is true, the function tries to find a frame
       which contains all compositing layers identified in the `layers'
       array.  If there are none, you might call the function a second
       time with `match_all_layers' false.  This option is ignored unless
       `num_regions' and `n_streams' both equal to 0.     
       [//]
       If necessary, the function loops back to the first frame and continues
       searching until all frames have been examined.  If a match is found, the
       relevant frame index is returned.  Otherwise, the function
       returns -1.
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
  kdu_dims get_new_focus_box();
    /* Performs most of the work of the `update_focus_box' and `set_focus_box'
       functions. */
  void update_focus_box();
    /* Called whenever a change in the viewport may require the focus box
       to be adjusted.  The focus box must always be a subset of the
       viewport. */
  void outline_focus_box(CDC *dc, kdu_dims relative_region);
    /* This function is automatically called from within `paint_region' if
       the focus box is enabled.  It pens an outline around the current focus
       box.  The `relative_region' member holds the region which is being
       updated by `paint_region', expressed relative to the top left hand
       corner of the view, without incorporating the prevailing `pixel_scale'.
       That is, the true region being updated is obtained by multiplying
       the coordinates by `pixel_scale'.  The implementation can intersect
       the `dc' object's clipping rectangle with the `pixel_scale' scaled
       version of `relative_region'; it can also decide whether or not to
       draw anything based on the value of `relative_region'. */
// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------
private: // Functions related to file saving
  void save_as(bool preserve_codestream_links, bool force_codestream_links);
  bool save_over(); // Saves over the existing file; returns false if failed.
  void save_as_jpx(const char *filename, bool preserve_codestream_links,
                   bool force_codestream_links);
  void save_as_jp2(const char *filename);
  void save_as_raw(const char *filename);
  void copy_codestream(kdu_compressed_target *tgt, kdu_codestream src);

// ----------------------------------------------------------------------------
public: // Public data
  kdws_frame_window *window;
  kdws_image_view *image_view; // Null until the frame is created
  kdws_manager *manager;
  kdws_metashow *metashow; // NULL if not active
  kdws_catalog *catalog_source; // NULL if metadata catalog is not visible
  kdws_metadata_editor *editor; // NULL if metadata editor is not active
  bool catalog_closed; // True if catalog was explicitly closed
  int window_identifier;
  bool have_unsaved_edits;
// ----------------------------------------------------------------------------
private: // File and URL names retained by the `window_manager'
  bool processing_call_to_open_file; // Prevents recursive entry to `open_file'
  const char *open_file_pathname; // File we are reading; NULL for a URL
  const char *open_file_urlname; // JPIP URL we are reading; NULL if not JPIP
  const char *open_filename; // Points to filename within path or URL
  char *last_save_pathname; // Last file to which user saved anything
  bool save_without_asking; // If the user does not want to be asked in future
  kdws_renderer *duplication_src; // Non-NULL inside `open_as_duplicate'
// ----------------------------------------------------------------------------
private: // Compressed data source and rendering machinery
  bool allow_seeking;
  int error_level; // 0 = fast, 1 = fussy, 2 = resilient, 3 = resilient_sop
  kdu_simple_file_source file_in;
  kdu_cache cache_in; // If open, the cache is bound to a JPIP client
  jp2_threadsafe_family_src jp2_family_in;
  jpx_source jpx_in;
  mj2_source mj2_in;
  jpx_composition composition_rules; // Composition/rendering info, if available
  kd_bitmap_compositor *compositor;
  kdu_thread_env thread_env; // Multi-threaded environment
  int num_threads; // If 0 use single-threaded machinery with `thread_env'=NULL
  int num_recommended_threads; // If 0, recommend single-threaded machinery
  bool in_idle; // Indicates we are inside the `OnIdle' function
  bool processing_suspended;
  double defer_process_regions_deadline; // Used in non-playback mode to
    // determine whether `compositor->process' should be called with the
    // `KDU_COMPOSIT_DEFER_REGION' flag.  This deadline is used to
    // encourage processing and region aggregation over short time scales.
// ----------------------------------------------------------------------------
private: // JPIP state information
  kdu_client *jpip_client; // Currently NULL or points to `single_client'
  kdu_window client_roi; // Last posted/in-progress client-server window.
  kdu_window tmp_roi; // Temp work space to avoid excessive new/delete usage
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
  kdws_file_services *file_server; // Created on-demand for metadata editing
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
  kdws_frame_presenter *frame_presenter; // Paints frames from separate thread
  kdws_playmode_stats playmode_stats;
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
  CPen focus_pen_inside;
  CPen focus_pen_outside;
  kdu_dims focus_box; // Uses same coordinate system as `view_dims'
  kdws_rel_focus rel_focus;
// ----------------------------------------------------------------------------
private: // Overlay parameters
  bool overlays_enabled;
  int overlay_flashing_state; // 0=none; 1=getting brigher; -1=getting darker
  float overlay_nominal_intensity; // Basic value we get active intensity from
  int overlay_size_threshold; // Min composited size for an overlay to be shown
  jpx_metanode overlay_restriction;
  jpx_metanode last_selected_overlay_restriction;
  kdu_uint32 overlay_params[KDWS_NUM_OVERLAY_PARAMS];
  int overlay_highlight_state; // 0 if not flashing imagery for metanode
  jpx_metanode highlight_metanode;
// ----------------------------------------------------------------------------
private: // Image buffer and painting resources
  kd_bitmap_buf *buffer; // Get from `compositor' object; NULL if invalid
  CDC compatible_dc; // Compatible DC for use with BitBlt function
  kdu_coords strip_extent; // Dimensions of strip buffer
  HBITMAP stripmap; // Manages strip buffer for indirect painting
  BITMAPINFO stripmap_info; // Used in calls to CreateDIBSection.
  kdu_uint32 *stripmap_buffer; // Buffer managed by `stripmap' object.
  kdu_byte foreground_lut[256];
  kdu_byte background_lut[256];
// ----------------------------------------------------------------------------
public: // File Menu Handlers
	afx_msg void menu_FileDisconnect();
	afx_msg void can_do_FileDisconnect(CCmdUI* pCmdUI);
	afx_msg void menu_FileSaveAs();
	afx_msg void can_do_FileSaveAs(CCmdUI* pCmdUI);
  afx_msg void menu_FileSaveAsLinked();
  afx_msg void can_do_FileSaveAsLinked(CCmdUI *pCmdUI);
  afx_msg void menu_FileSaveAsEmbedded();
  afx_msg void can_do_FileSaveAsEmbedded(CCmdUI *pCmdUI);
  afx_msg void menu_FileSaveReload();
  afx_msg void can_do_FileSaveReload(CCmdUI *pCmdUI);
	afx_msg void menu_FileProperties();
	afx_msg void can_do_FileProperties(CCmdUI* pCmdUI);
// ----------------------------------------------------------------------------
public: // View Menu Handlers (orientation and zoom)
	afx_msg void menu_ViewHflip();
	afx_msg void can_do_ViewHflip(CCmdUI* pCmdUI);
	afx_msg void menu_ViewVflip();
	afx_msg void can_do_ViewVflip(CCmdUI* pCmdUI);
	afx_msg void menu_ViewRotate();
	afx_msg void can_do_ViewRotate(CCmdUI* pCmdUI);
	afx_msg void menu_ViewCounterRotate();
	afx_msg void can_do_ViewCounterRotate(CCmdUI* pCmdUI);
	afx_msg void menu_ViewZoomOut();
	afx_msg void can_do_ViewZoomOut(CCmdUI* pCmdUI);
	afx_msg void menu_ViewZoomIn();
	afx_msg void can_do_ViewZoomIn(CCmdUI* pCmdUI);
	afx_msg void menu_ViewZoomOutSlightly();
	afx_msg void menu_ViewZoomInSlightly();
  afx_msg void menu_ViewOptimizescale();
  afx_msg void can_do_ViewOptimizescale(CCmdUI *pCmdUI);
	afx_msg void menu_ViewRestore();
	afx_msg void can_do_ViewRestore(CCmdUI* pCmdUI);
	afx_msg void menu_ViewRefresh();
	afx_msg void can_do_ViewRefresh(CCmdUI* pCmdUI);
// ----------------------------------------------------------------------------
public: // View Menu Handlers (quality layers)
  afx_msg void menu_LayersLess();
	afx_msg void can_do_LayersLess(CCmdUI* pCmdUI);
	afx_msg void menu_LayersMore();
	afx_msg void can_do_LayersMore(CCmdUI* pCmdUI);
// ----------------------------------------------------------------------------
public: // View Menu Handlers (status display)
	afx_msg void menu_StatusToggle();
	afx_msg void can_do_StatusToggle(CCmdUI* pCmdUI);
// ----------------------------------------------------------------------------
public: // View Menu Handlers (image window)
	afx_msg void menu_ViewWiden();
	afx_msg void can_do_ViewWiden(CCmdUI* pCmdUI);
	afx_msg void menu_ViewShrink();
	afx_msg void can_do_ViewShrink(CCmdUI* pCmdUI);
  afx_msg void menu_ViewScaleX2();
  afx_msg void menu_ViewScaleX1();
// ----------------------------------------------------------------------------
public: // View Menu Handlers (navigation)
	afx_msg void menu_NavComponent1();
	afx_msg void can_do_NavComponent1(CCmdUI* pCmdUI);
	afx_msg void menu_NavComponentNext();
	afx_msg void can_do_NavComponentNext(CCmdUI* pCmdUI);
	afx_msg void menu_NavComponentPrev();
	afx_msg void can_do_NavComponentPrev(CCmdUI* pCmdUI);
  afx_msg void menu_NavCompositingLayer();
	afx_msg void can_do_NavCompositingLayer(CCmdUI* pCmdUI);
	afx_msg void menu_NavMultiComponent();
	afx_msg void can_do_NavMultiComponent(CCmdUI* pCmdUI);
  afx_msg void menu_NavTrackNext();
  afx_msg void can_do_NavTrackNext(CCmdUI *pCmdUI);
	afx_msg void menu_NavImageNext();
	afx_msg void can_do_NavImageNext(CCmdUI* pCmdUI);
	afx_msg void menu_NavImagePrev();
	afx_msg void can_do_NavImagePrev(CCmdUI* pCmdUI);
// ----------------------------------------------------------------------------
public: // Focus Menu Handlers
	afx_msg void menu_FocusOff();
	afx_msg void can_do_FocusOff(CCmdUI* pCmdUI);
	afx_msg void menu_FocusHighlighting();
	afx_msg void can_do_FocusHighlighting(CCmdUI* pCmdUI);
	afx_msg void menu_FocusWiden();
	afx_msg void menu_FocusShrink();
	afx_msg void menu_FocusLeft();
	afx_msg void menu_FocusRight();
	afx_msg void menu_FocusUp();
	afx_msg void menu_FocusDown();
// ----------------------------------------------------------------------------
public: // Mode Menu Handlers
 	afx_msg void menu_ModeFast();
	afx_msg void can_do_ModeFast(CCmdUI* pCmdUI);
	afx_msg void menu_ModeFussy();
	afx_msg void can_do_ModeFussy(CCmdUI* pCmdUI);
	afx_msg void menu_ModeResilient();
	afx_msg void can_do_ModeResilient(CCmdUI* pCmdUI);
	afx_msg void menu_ModeResilientSop();
	afx_msg void can_do_ModeResilientSop(CCmdUI* pCmdUI);
	afx_msg void menu_ModeSeekable();
	afx_msg void can_do_ModeSeekable(CCmdUI* pCmdUI);
  afx_msg void menu_ModeSinglethreaded();
  afx_msg void can_do_ModeSinglethreaded(CCmdUI *pCmdUI);
  afx_msg void menu_ModeMultithreaded();
  afx_msg void can_do_ModeMultithreaded(CCmdUI *pCmdUI);
  afx_msg void menu_ModeMorethreads();
  afx_msg void can_do_ModeMorethreads(CCmdUI *pCmdUI);
  afx_msg void menu_ModeLessthreads();
  afx_msg void can_do_ModeLessthreads(CCmdUI *pCmdUI);
  afx_msg void menu_ModeRecommendedThreads();
  afx_msg void can_do_ModeRecommendedThreads(CCmdUI *pCmdUI);
// ----------------------------------------------------------------------------
public: // Metadata Menu Handlers
  afx_msg void menu_MetadataCatalog();
  afx_msg void can_do_MetadataCatalog(CCmdUI* pCmdUI)
    { pCmdUI->SetCheck(catalog_source != NULL);
      pCmdUI->Enable((catalog_source != NULL) || jpx_in.exists()); }
  afx_msg void menu_MetadataSwapFocus();
  afx_msg void can_do_MetadataSwapFocus(CCmdUI *pCmdUI)
    { pCmdUI->Enable((catalog_source != NULL) || jpx_in.exists()); }
  afx_msg void menu_MetadataShow();
	afx_msg void can_do_MetadataShow(CCmdUI* pCmdUI)
    { pCmdUI->SetCheck(metashow != NULL);
      pCmdUI->Enable(true); }
  afx_msg void menu_MetadataAdd();
  afx_msg void can_do_MetadataAdd(CCmdUI* pCmdUI)
    { pCmdUI->Enable((compositor != NULL) && jpx_in.exists() &&
                     ((jpip_client == NULL) || !jpip_client->is_alive(-1))); }
  afx_msg void menu_MetadataEdit();
  afx_msg void can_do_MetadataEdit(CCmdUI* pCmdUI)
    { pCmdUI->Enable((compositor != NULL) && jpx_in.exists()); }
  afx_msg void menu_MetadataDelete();
  afx_msg void can_do_MetadataDelete(CCmdUI* pCmdUI);
  afx_msg void menu_MetadataCopyLabel();
  afx_msg void can_do_MetadataCopyLabel(CCmdUI* pCmdUI);
  afx_msg void menu_MetadataCopyLink();
  afx_msg void can_do_MetadataCopyLink(CCmdUI* pCmdUI)
    { pCmdUI->Enable(((compositor != NULL) && jpx_in.exists())); }
  afx_msg void menu_MetadataCut();
  afx_msg void can_do_MetadataCut(CCmdUI* pCmdUI);
  afx_msg void menu_MetadataPasteNew();
  afx_msg void can_do_MetadataPasteNew(CCmdUI* pCmdUI);
  afx_msg void menu_OverlayEnable();
  afx_msg void can_do_OverlayEnable(CCmdUI* pCmdUI)
    { pCmdUI->SetCheck((compositor != NULL) && jpx_in.exists() &&
                       overlays_enabled);
      pCmdUI->Enable((compositor != NULL) && jpx_in.exists()); } 
  afx_msg void menu_OverlayFlashing();
  afx_msg void can_do_OverlayFlashing(CCmdUI* pCmdUI)
    { pCmdUI->SetCheck((compositor != NULL) && jpx_in.exists() &&
                       overlay_flashing_state && overlays_enabled);
      pCmdUI->Enable((compositor != NULL) && jpx_in.exists() &&
                     overlays_enabled && !in_playmode); }  
  afx_msg void menu_OverlayToggle();
  afx_msg void can_do_OverlayToggle(CCmdUI* pCmdUI)
    { pCmdUI->Enable((compositor != NULL) && jpx_in.exists()); }
  afx_msg void menu_OverlayRestrict();
  afx_msg void can_do_OverlayRestrict(CCmdUI* pCmdUI);
  afx_msg void menu_OverlayLighter();
  afx_msg void can_do_OverlayLighter(CCmdUI* pCmdUI)
    { pCmdUI->Enable((compositor != NULL) && jpx_in.exists() &&
                     overlays_enabled); }
  afx_msg void menu_OverlayHeavier();
  afx_msg void can_do_OverlayHeavier(CCmdUI* pCmdUI)
    { pCmdUI->Enable((compositor != NULL) && jpx_in.exists() &&
                     overlays_enabled); }
  afx_msg void menu_OverlayDoubleSize();
  afx_msg void can_do_OverlayDoubleSize(CCmdUI* pCmdUI)
    { pCmdUI->Enable((compositor != NULL) && jpx_in.exists() &&
                     overlays_enabled); }
  afx_msg void menu_OverlayHalveSize();
  afx_msg void can_do_OverlayHalveSize(CCmdUI* pCmdUI)
    { pCmdUI->Enable((compositor != NULL) && jpx_in.exists() &&
                     overlays_enabled && (overlay_size_threshold > 1)); }
// ----------------------------------------------------------------------------
public: // Play Menu Handlers
  bool source_supports_playmode()
    { return (compositor != NULL) &&
      (mj2_in.exists() || jpx_in.exists()) &&
      (num_frames != 0) && (num_frames != 1); }
  afx_msg void menu_PlayStartForward() { start_playmode(false); }
	afx_msg void can_do_PlayStartForward(CCmdUI* pCmdUI)
    { pCmdUI->Enable(source_supports_playmode() &&
                     (single_component_idx < 0) &&
                     ((single_layer_idx < 0) || !jpx_in.exists()) &&
                      (playmode_reverse || !in_playmode)); }
  afx_msg void menu_PlayStartBackward() { start_playmode(true); }
	afx_msg void can_do_PlayStartBackward(CCmdUI* pCmdUI)
    { pCmdUI->Enable(source_supports_playmode() &&
                     (single_component_idx < 0) &&
                     ((single_layer_idx < 0) || !jpx_in.exists()) &&      
                      !(playmode_reverse && in_playmode)); }
  afx_msg void menu_PlayStop() { stop_playmode(); }
	afx_msg void can_do_PlayStop(CCmdUI* pCmdUI)
    { pCmdUI->Enable(in_playmode); }
  afx_msg void menu_PlayRepeat() { playmode_repeat = !playmode_repeat; }
  afx_msg void can_do_PlayRepeat(CCmdUI* pCmdUI)
    { pCmdUI->SetCheck(playmode_repeat);
      pCmdUI->Enable(source_supports_playmode()); }
  afx_msg void menu_PlayNative();
  afx_msg void can_do_PlayNative(CCmdUI* pCmdUI);
  afx_msg void menu_PlayCustom();
  afx_msg void can_do_PlayCustom(CCmdUI* pCmdUI);  
  afx_msg void menu_PlayFrameRateUp();
  afx_msg void can_do_PlayFrameRateUp(CCmdUI* pCmdUI)
    { pCmdUI->Enable(source_supports_playmode()); }
  afx_msg void menu_PlayFrameRateDown();
  afx_msg void can_do_PlayFrameRateDown(CCmdUI* pCmdUI)
    { pCmdUI->Enable(source_supports_playmode()); }

// ----------------------------------------------------------------------------
public: // Window Menu Handlers -- this one not actually a message handler
	bool can_do_WindowDuplicate()
    { return (compositor != NULL); }

	DECLARE_MESSAGE_MAP()
};

#endif // KDU_SHOW_H
