/*****************************************************************************/
// File: vex_display.h [scope = APPS/VEX_FAST]
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
   Local class declarations for "vex_display.cpp".
******************************************************************************/

#ifndef VEX_DISPLAY_H
#define VEX_DISPLAY_H

// System includes
#include <assert.h>
// Kakadu core includes
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_vex.h"

#ifdef KDU_DX9
#  include <d3d9.h> // If this include statement gives you compile errors, you
                    // can undefine "KDU_DX9".  Alternatively, you can install
                    // the DirectX 9 SDK or a later version.

/*****************************************************************************/
/*                           vex_display_surface                             */
/*****************************************************************************/

struct vex_display_surface {
    kdu_coords alloc_size;
    kdu_dims frame_region; // Region occupied by the frame
    D3DLOCKED_RECT locked_rect;
    IDirect3DSurface9 *surface;
    vex_display_surface *next;
  };
  /* This structure is used to manage individual OffScreenPlain surface
     buffers in the SYSTEMMEM pool.  Locked instances of this structure form
     the handles returned by `vex_display::allocate_mem'.  Unlocked
     instances of the structure form an internal queue of frames waiting
     for display within `vex_display'.  It is possible that the
     `visible_rect' region is not anchored at the origin of the surface,
     if for some reason the surface memory buffer was not 16-byte aligned.
     When locked, `locked_rect.pBits' will be non-NULL and the entire
     surface is available for reading and/or writing. */

/*****************************************************************************/
/*                           vex_display (DirectX)                           */
/*****************************************************************************/

class vex_display : public vex_frame_memory_allocator {
  public: // Member functions
    vex_display() { direct3d=NULL; window_class=NULL; window=NULL;
                    device=NULL; hinst=NULL; free_surfaces=NULL;
                    ready_head = ready_tail = NULL;
                    display_shutdown_requested=false; }
    ~vex_display() { shutdown(); }
    bool exists() { return (direct3d != NULL); }
    const char *init(kdu_coords frame_size, bool full_screen, float fps,
                     int total_frame_buffers);
      /* Starts Direct3D and creates a window surface for drawing, which is
         accommodates the video frame if possible.  Returns a non-NULL
         explanation string if the operation could not be completed, after
         which `exists' will continue to return false.  The
         `total_frame_buffers' argument represents the maximum number of
         frame memory surfaces which will ever be created by this object.
         In practice, these are all created up front from SYSTEMMEM pool
         memory.  This total number of frame buffers includes all frames
         maintained by the `vex_frame_queue' object, since they are
         obtained via the `allocate_mem' function; it also includes all
         frame buffers which may be waiting internally for display.  The
         value of this argument must be at least as large as the number
         required by the `vex_frame_queue', or else calls to
         `allocate_mem' may fail.  In practice, you should make it larger
         than the number of frames managed by `vex_frame_queue' so as
         to avoid delaying the management thread while GPU operations
         are in progress. */ 
    void shutdown();
      /* Terminates the Direct3D interface and restores the screen to its
         previous mode.  Once frame surfaces have been allocated using
         `allocate_mem', this function should not be called until they have
         all been returned. */
    bool push_frame(vex_frame *frame);
      /* Returns false if a user-quit request has been detected, or
         something else has gone wrong. */
    virtual void *
      allocate_mem(int num_components, kdu_dims *component_dims,
                   size_t sample_bytes, int format,
                   kdu_byte * &buffer, size_t &frame_bytes,
                   size_t &stride, bool &fully_aligned);
        /* Implements the `vex_frame_memory_allocator::allocate_mem'
           function, putting all memory on the heap.  If `component_dims' is
           NULL, this function is being invoked internally, so no checking
           of frame dimensions is required. */
    virtual void destroy_mem(void *handle); 
        /* Implements the `vex_frame_memory_allocator::destroy_mem'
           function. */
  private: // Helper functions
    friend kdu_thread_startproc_result
           KDU_THREAD_STARTPROC_CALL_CONVENTION display_startproc(void *);
    void run_display_loop();
      /* Presents new frames at the required rate (if possible) and monitors
         the windows message queue for the `q' or `ESC' characters. */
    void set_backbuffer_rects();
      /* Uses `frame_dims' and `display_dims' to configure the `paint_rect'
         `num_dirty_rects' and `dirty_rects' members for rendering to the
         backbuffer.  This function also adjusts `display_dims.pos' if
         necessary, to ensure that the display region is fully covered by
         frame data, if possible. */
    bool flush_message_queue();
      /* Executes `PeekMessage' until all window messages are cleared.  Passes
         all messages onto the DefWindowProc function, but catches key down
         events.  Arrow keys are used to produce
         `pending_pan_up/down/left/right' messages, while other keys all
         other keys are interpreted as user-kill instructions -- in the
         latter case the function returns false. */
  private: // Data
    float target_frame_period;
    IDirect3D9 *direct3d;
    HINSTANCE hinst;
    ATOM window_class;
    HWND window;
    IDirect3DDevice9 *device;
    kdu_dims frame_dims; // `pos' is always (0,0)
    kdu_dims display_dims; // `pos' holds top-left display pixel's location in
           // the complete video frame; `size' holds display client dimensions
    RECT paint_rect; // Passed to `UpdateSurface'
    int num_dirty_rects; // Used in `Clear' calls to clear unused portions
    RECT dirty_rects[2]; // of the backbuffer, since it is discardable.
    int pending_pan_up;    // These are used to inform the display thread that
    int pending_pan_down;  // an arrow key has been pressed and it should
    int pending_pan_left;  // adjust the `display_dims.pos' member accordingly
    int pending_pan_right; // and run `set_backbuffer_rects' again.
    vex_display_surface *free_surfaces;
    vex_display_surface *ready_head, *ready_tail;
    kdu_mutex mutex;
    kdu_thread display_thread;
    kdu_event surface_free_event; // Used to signal `free_surfaces' != NULL
    kdu_event surface_ready_event; // Used to signal `ready_head' != NULL
    bool display_shutdown_requested;
  };

#else // !KDU_DX9

/*****************************************************************************/
/*                         vex_display (No DirectX)                          */
/*****************************************************************************/

class vex_display : public vex_frame_memory_allocator {
  public: // Member functions
    bool exists() { return false; }
    const char *init(kdu_coords frame_size, bool full_screen, float fps,
                     int total_frame_buffers)
      { return "Unable to actually render decompressed "
        "frames to the display, due to lack of DirectX support.  This "
        "application has not been compiled against DirectX."; }
    void shutdown() { return; }
    bool push_frame(vex_frame *frame) { return false; }
    virtual void *
      allocate_mem(int num_components, kdu_dims *component_dims,
                   size_t sample_bytes, int format,
                   kdu_byte * &buffer, size_t &frame_bytes,
                   size_t &stride, bool &fully_aligned)
      { return NULL; }
    virtual void destroy_mem(void *handle) { return; }
  };
#endif // !KDU_DX9

#endif // VEX_DISPLAY_H
