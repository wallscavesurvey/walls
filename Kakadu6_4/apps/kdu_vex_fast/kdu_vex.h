/*****************************************************************************/
// File: kdu_vex.h [scope = APPS/VEX_FAST]
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
   Class declarations for the platform independent multi-threaded video
decompression pipeline used by the "kdu_vex_fast" demo app.
******************************************************************************/

#ifndef KDU_VEX_H
#define KDU_VEX_H

#include "kdu_threads.h"
#include "kdu_elementary.h"
#include "kdu_arch.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "mj2.h"

// Declared here:
class vex_frame_memory_allocator;
struct vex_frame;
class vex_processor;
class vex_engine;
class vex_frame_queue;

/*****************************************************************************/
/*                         vex_frame_memory_allocator                        */
/*****************************************************************************/

#define VEX_FRAME_FORMAT_CONTIG 0
#define VEX_FRAME_FORMAT_XRGB 1

class vex_frame_memory_allocator {
  public: // Member functions
    virtual ~vex_frame_memory_allocator() { return; }
    virtual void *allocate_mem(int num_components, kdu_dims *component_dims,
                               size_t sample_bytes, int format,
                               kdu_byte * &buffer, size_t &frame_bytes,
                               size_t &stride, bool &fully_aligned) = 0;
      /* This function is used to allocate the surface buffer memory for
         `vex_frame' objects.  It returns an opaque handle, which must be
         passed back into the `destroy_mem' function in order to properly
         destroy the resource.  The handle might be the base of an allocated
         block of memory or a much richer object, known only to the
         internal implementation of the `vex_frame_memory_allocator'-derived
         object.
         [//]
         The amount of memory which must be allocated is determined by
         the component sizes found in the `component_dims' array -- this array
         has `num_components' entries.  The structure of the writable
         memory surface returned via the `buffer' argument is determined by
         the `format' argument.  Currently, only two formats are defined,
         as follows:
         [>>] If `format' = `VEX_FRAME_MEMORY_CONTIG', image components
              appear as contiguous raster blocks of memory, one after the
              other, without any intervening gaps, either between rows or
              between components.  Each sample is allocated `sample_bytes'
              bytes of memory and the pointer returned via `buffer' references
              the first sample of the first image component.  In this case,
              the value returned via `frame_bytes' is the sum of the areas
              of each image component, multiplied by `sample_bytes'.  Also,
              in this case the value returned via `stride' is 0 and the
              value returned via `fully_aligned' is true only if
              the first sample of every line in every image component has
              a 16-byte aligned location in the memory buffer.
         [>>] If `format' = `VEX_FRAME_MEMORY_XRGB', there may be no more
              than 4 image components, all of which must have identical
              dimensions; moreover, `sample_bytes' must be equal to 4,
              meaning that the separation between consecutive samples in
              any given component is expected to be 4 bytes, not that each
              sample has 4 bytes all to itself -- if any of these conditions
              fail, the function generates an internal error through
              `kdu_error', so the caller must be careful not to hold any
              mutex at the time.  An attempt is made to ensure that the
              returned `buffer' is 16-byte aligned and that the `stride'
              (bytes between consecutive rows) is also a multiple of 16 bytes.
              The `fully_aligned' variable is set to true if and only if
              these conditions are satisfied.  For this format, the value
              returned via `frame_bytes' represents the accessible number
              of bytes in the `buffer', but is unlikely to be important.
          [//]
          Note that the function never returns NULL.  It either succeeds,
          or else it generates an error through `kdu_error'. */
    virtual void destroy_mem(void *handle) = 0;
      /* Call this function to destroy the frame memory allocated via
         `allocate_mem'. */
  };

/*****************************************************************************/
/*                                 vex_frame                                 */
/*****************************************************************************/

struct vex_frame {
  public: // Member functions
    vex_frame()
      { buffer=NULL; buffer_handle=NULL; allocator=NULL; engine=NULL;
        fully_aligned=aborted=waiting=false; next=NULL; }
    ~vex_frame()
      {
        if (codestream.exists()) codestream.destroy();
        if ((buffer_handle != NULL)  && (allocator != NULL))
          allocator->destroy_mem(buffer_handle);
      }
  public: // Data
    jp2_input_box box; // Holds the compressed data source for the frame
    kdu_codestream codestream; // Created by the queue manager
    int precision;
    bool is_signed;
    size_t sample_bytes; // Bytes for each decompressed output sample
    size_t frame_bytes; // Bytes for the entire frame (concatenated components)
    size_t stride; // Non-zero only if a single interleaved buffer is used
    kdu_byte *buffer; // Array with `frame_bytes' entries; 16-byte aligned
    bool fully_aligned; // If all image rows start on 16-byte boundaries
    void *buffer_handle; // Pass to `allocator->destroy_mem' for cleanup
    vex_frame_memory_allocator *allocator;  // Object that allocated buffer.
    vex_engine *engine; // Non-NULL if the frame is being decompressed
    bool aborted; // True if frame was sent to `vex_frame_queue::frame_aborted'
    bool waiting; // True if the frame is waiting to be assigned to an engine
  private: // Data
    friend class vex_frame_queue;
    vex_frame *next;
  };
  /* Notes:
       This object maintains three types of resources: 1) the compressed
     data for the frame, pre-loaded into `box' by the management thread
     during calls to `vex_frame_queue::get_processed_frame'; 2) an
     open codestream (created by the management thread after loading the
     `box' contents during `vex_frame_queue::get_processed_frame'); and
     3) a frame buffer, into which the decompressed samples are written
     by one or another of the `vex_engine' frame processing engines.
        Two organizations for the `buffer' are currently supported.  If
     `stride'=0, the `buffer' contains all samples of the first image
     component in raster order, followed by all samples of the second
     image component, and so on.  This organization is suitable for
     directly dumping the decompressed frame to a VIX file -- it is exactly
     the same order used by the "kdu_v_expand" application and consumed
     by "kdu_v_compress".
        If `stride' > 0, the `buffer' has an interleaved representation,
     with successive lines separated by `stride' bytes.  In this case,
     all image components must have the same dimensions, and all are
     represented with just one byte per sample.  The `sample_bytes' member
     will always be equal to 4 in this case and the `buffer' will always
     have an interleaved ARGB organization, with the B sample appearing
     first, followed by G, R and then A (alpha), within each pixel.
     The value of `stride' is selected so as to be a multiple of 32 bytes,
     so that all buffer lines can be 32-byte aligned.  Unused samples at
     the end of each line are undefined.  This type of representation is
     selected only if the `want_display' argument to `vex_frame_queue::init'
     is true. */

/*****************************************************************************/
/*                                vex_processor                              */
/*****************************************************************************/

class vex_processor {
  public: // Member functions
    vex_processor() { comp_info=NULL; reset(); }
    ~vex_processor() { reset(); }
    void reset();
      /* Restores the processor to its initial state so that `process_frame'
         can be called for the first time. */
    void start_frame(vex_frame *frame, kdu_thread_env *env=NULL,
                     int double_buffering_height=0,
                     kdu_int32 truncation=-1);
      /* Call this function to start processing the indicated frame.
         Processing itself occurs by calling `process_frame' until it
         returns true.  It is worth noting that some internal processing
         of the frame may already have occurred if a non-NULL `next_frame'
         argument was supplied in a previous call to `process_frame'.
         [//]
         If `truncation' >= 0, it is used to modify the factor which the
         object passes to `kdu_codestream::set_block_truncation'.  As noted
         in the description of that function, block truncation enables a
         reduction in decompression CPU time at the expense of image quality.
         On construction, the `vex_processor' object sets its internal
         `block_truncation_factor' to 0 (no truncation).  This internal
         factor is passed to each new codestream's `set_block_truncation'
         function.  On each call to `start_frame' or `process_frame', the
         internal `block_truncation_factor' member can be altered by a
         non-negative `truncation' argument, in which case the modified
         value is also passed to the `kdu_codestream::set_block_truncation'
         functions of all codestreams which the object is currently using
         (there may be up to two, if the engine is multi-threaded).  This
         is intended to provide the fastest reasonable response to any
         required change in the decompression processing speed. */
    bool process_frame(int max_lines_at_once=256, vex_frame *next_frame=NULL,
                       int truncation=-1);
      /* Decompresses the codestream belonging to the `frame' supplied in
         the most recent call to `start_frame', writing results to the
         frame's internal buffer.  The function processes at most
         `max_lines_at_once' image component lines from any single tile before
         returning; it returns false unless the entire frame has been
         processed.
         [//]
         The `next_frame' argument is of interest only in multi-threading
         environments.  In particular, it provides a means to keep processing
         resources active even when we come to the end of processing a
         current codestream.  In the current implementation, the initial
         block decoding jobs for the `next_frame' are automatically
         activated when processing of the current `codestream' leaves the
         system under-utilized, in the sense defined by
         `kdu_thread_entity::add_queues'.
         [//]
         The main reason for limiting the amount of data processed in
         each call to `process_frame' is to provide a means to take
         advantage of a `next_frame' which does not become available
         until some time after the current frame has started.  This means
         that the first few calls to this function might provide a NULL
         `next_frame' argument, while subsequent calls provide an actual
         `next_frame'.  This is OK, so long as you continue to supply the
         same `next_frame' argument until the frame is completely processed.
         Moreover, any subsequent call to `start_frame' must supply the
         same frame as that supplied by any non-NULL `next_frame' argument
         here.
         [//]
         For an explanation of the `truncation' argument, see the comments
         appearing with the `start_frame' function. */
  private: // Declarations
    struct vex_comp_info { // Stores fixed component-wide information
        kdu_dims comp_dims; // Dimensions and location of component
        int count_val; // Vertical sub-sampling factor for component
        size_t comp_boff; // From start of frame buffer to 1st row in component
      };
    struct vex_tcomp {
        kdu_coords comp_size; // Obtained from `vex_comp_info::comp_dims'
        kdu_coords tile_size; // Size of current tile-component
        size_t boff; // From start of frame buffer to next row in tile-comp
        size_t next_tile_boff; // `boff' value for 1st row in next tile
        int precision; // # bits from tile-component stored in VIX sample MSB's
        int counter;
        int tile_rows_left; // Rows which have not yet been processed
        bool using_shorts;  // True if engine uses 16-bit samples for this comp
        bool simd_xfer_shorts_bytes; // These 2 members are used to accelerate
        int simd_xfer_downshift;     // testing for the conditions under which
                                     // the fast SIMD transfer code can be used
      };
    struct vex_tile {
      public: // Members
        vex_tile() { frame=NULL; components=NULL; env_queue=NULL; next=NULL; }
        ~vex_tile() { reset(); }
        void reset()
          {
            frame = NULL;
            if (components != NULL)
              { delete[] components; components=NULL; }
            valid_indices = kdu_dims(); idx = kdu_coords();
            if (engine.exists()) engine.destroy();
            if (ifc.exists()) ifc.close();
            env_queue = NULL;
            next = NULL;
          }
        bool is_active() { return engine.exists(); }
      public: // Data
        vex_frame *frame;
        kdu_dims valid_indices;
        kdu_coords idx; // Relative index, within `valid_indices' range
        bool last_in_frame; // True if this is the last tile in the frame
        kdu_tile ifc;
        kdu_multi_synthesis engine;
        vex_tcomp *components; // One for each component produced by `engine'
        kdu_thread_queue *env_queue;
        vex_tile *next; // Points to the next tile in a circular buffer
        int simd_downshift; // Single downshift for all components if possible
        bool simd_mono_shorts_to_argb8;
        bool simd_rgb_shorts_to_argb8; // True if 3 comps can be transferred
                              // and interleaved together into an ARGB buffer.
      };
  private: // Helper functions
    void init_tile(vex_tile *tile, vex_frame *frame);
      /* Initializes the `tile' object to reference the first valid tile
         of the supplied frame's codestream, updating or even creating all
         internal members as appropriate.  The function even creates the
         internal `comp_info' array if it does not currently exist.  The
         `tile' object must not be active when this function is called,
         meaning that `vex_tile::is_active' must return false. */
    bool init_tile(vex_tile *tile, vex_tile *ref_tile);
      /* This second form of the `init_tile' function initializes the object
         referenced by `tile' to use the codestream which is currently
         associated with `ref_tile', except that the next tile within
         that codestream is opened.  If there is no such tile, the function
         returns false, leaving the object in the inactive state (see
         `vex_tile::is_active').  Note that the `ref_tile' object can be
         either active or inactive, so long as it has a valid `frame' member.
         In fact, if `ref_tile' is inactive, it can refer to the same object
         as `tile'. */
    void close_tile(vex_tile *tile);
      /* This function does nothing unless `tile' is currently active
         (see `vex_tile::is_active').  In that case, the function destroys
         the internal `vex_tile::engine' object and closes any open codestream
         tile.  It leaves the `vex_tile::codestream' and `vex_tile::idx'
         members alone, however, so that they can be used to find and open
         the next tile in the same codestream, if one exists.
            This function is not invoked from the present object's destructor,
         since it executes the `kdu_thread::terminate' function in
         multi-threaded environments.  This is safe only if the application
         has not itself invoked `kdu_thread::terminate'. */
  private: // Data
    kdu_thread_env *env; // Used only for multi-threaded processing
    vex_frame *frame;
    bool double_buffering; // Derived from `double_buffering_height'
    int processing_stripe_height; // Derived from `double_buffering_height'
    int num_components;
    vex_comp_info *comp_info; // Fixed information for each component
    vex_tile tiles[2];
    vex_tile *current_tile; // Points to 1 of the objects in the `tiles' array
    vex_tile *next_tile; // Used only for multi-threaded processing (see below)
    kdu_int32 block_truncation_factor;
    kdu_long env_batch_idx; // Only for multi-threaded processing (see below)
  };
  /* Notes:
        For multi-threaded applications (i.e., when `env' is non-NULL), the
     implementation manages up to two `vex_tile' objects.  The `current_tile'
     pointer refers to the one from which data is currently being decompressed
     into the frame buffer.  The `next_tile' pointer refers to either the next
     tile in the current frame, or the first tile in the next codestream; if
     non-NULL, its `engine' has been started, which means that there are
     code-block decoding tasks which can be performed in the background by
     Kakadu's thread scheduler.  However, these jobs will not be performed
     until the system becomes under-utilized, in the sense described by the
     `kdu_thread_entity::add_queue' function.  This is managed through a
     `env_batch_idx' counter, which increments each time a new tile queue is
     created.
  */

/*****************************************************************************/
/*                                vex_engine                                 */
/*****************************************************************************/

class vex_engine {
  public: // Member functions
    vex_engine()
      {
        queue = NULL; blocked=false;
        graceful_shutdown_requested = immediate_shutdown_requested = false;
        num_threads = 0; cpu_affinity = 0; master_thread_name[0] = '\0';
      }
      /* [SYNOPSIS]
           Remember to call `startup' to start the engine running.
      */
    ~vex_engine() { shutdown(false); }
      /* [SYNOPSIS]
         Calls `shutdown', in case it has not already been called and
         then destroys the object's resources.
      */
    void startup(vex_frame_queue *queue, int engine_idx,
                 int num_threads, kdu_long cpu_affinity,
                 int thread_concurrency);
      /* [SYNOPSIS]
         Use this function to start the engine's master thread and any
         additional threads which it should control.
         [//]
         The `num_threads' argument indicates the number of threads to assign
         to the engine; if the value is 1, Kakadu's single threaded processing
         environment will be used, rather than the multi-threaded environment
         with only one thread.  This is OK, since disjoint processing engines
         running on different threads have different codestreams and do not
         directly exercise any shared I/O operations.
         [//]
         The `cpu_affinity' argument determines which CPU's the engine's
         threads can be scheduled onto.
         [//]
         The `thread_concurrency' argument is relevant only for operating
         systems which do not automatically allocate all CPUs to each
         running process -- Solaris is one example of this.  The value of
         the `thread_concurrency' argument should be the same for all
         constructed engines; it should be set to the number of CPUs that
         you would like to be accessible to the process.  In most cases,
         this should simply be equal to the total number of frame processing
         threads, accumulated over all frame processing engines, or the
         number of known system CPUs, whichever is larger -- since knowledge
         of CPUs can be unreliable.
      */
    void shutdown(bool graceful);
      /* [SYNOPSIS]
         This function should be called from the queue management thread --
         the same one which called `startup'.  It requests an orderly
         termination of the engine's master thread, along with any additional
         threads which it manages, at the earliest convenience.
         [//]
         If `graceful' is true, the engine will shut down only after
         completing the processing of any outstanding frames and returning
         them to the frame queue via `vex_frame_queue::frame_processed'
         (or `vex_frame_queue::frame_aborted' if there was an internal frame
         error).
         [//]
         If `graceful' is false, the engine will stop processing as soon as
         possible and it will not return any partially compressed frames
         to the frame queue -- there is no need for this, since the frame
         queue keeps a record of all frames it has ever created, for
         destruction purposes.  If the engine is found to be blocked on
         a call to `vex_frame_queue::get_frame_to_process', the
         `vex_frame_queue::terminate' function is called, which ensures that
         this engine (and all others) will be immediately unblocked and
         that calls to `vex_frame_queue::get_frame_to_process' will return
         false.  For this reason, ungraceful shutdown is intended only for
         the case in which we want to terminate all engines.
         [//]
         Once an engine has been shutdown, it can be started again using the
         `startup' function, although it is unclear how useful this
         capability actually is.
      */
  private: // Member functions
    friend kdu_thread_startproc_result
           KDU_THREAD_STARTPROC_CALL_CONVENTION engine_startproc(void *);
    void run_single_threaded();
    void run_multi_threaded();
      /* The engine's master thread runs in one of these two functions. */
  private: // Data
    vex_processor processor;
    vex_frame_queue *queue;
    bool blocked; // True during blocking call to `queue->get_frame_to_process'
    bool graceful_shutdown_requested;
    bool immediate_shutdown_requested;
    int num_threads; // Number of threads controlled by this engine
    int thread_concurrency; // See `startup'
    kdu_long cpu_affinity; // Affinity set for this engine
    char master_thread_name[81]; // Makes debugging a bit easier
    kdu_thread master_thread; // Master thread for this engine
  };

/*****************************************************************************/
/*                              vex_frame_queue                              */
/*****************************************************************************/

class vex_frame_queue : public vex_frame_memory_allocator {
  public: // Member functions
    vex_frame_queue()
      { source=NULL; component_subs=NULL; component_dims=NULL;
        active_head = active_tail = processed_head = processed_tail = NULL;
        recycled_frames = NULL; discard_levels=0; is_signed=false;
        precision=num_components=0; sample_bytes=0; frame_format=0;
        repeat_count=next_frame_to_open=0; block_truncation_factor=0;
        num_active_frames = max_active_frames = num_waiting_frames = 0;
        exhausted = terminated = exception_raised = false;
        last_exception_code = KDU_NULL_EXCEPTION; }
      /* [SYNOPSIS]
         Before actually using the object, you need to call `init'.
      */
    ~vex_frame_queue();
      /* [SYNOPSIS]
         Be sure to shutdown (or destroy) all `vex_engine' objects which might
         be using the queue, before destroying the frame queue object
         itself.
      */
    bool init(mj2_video_source *source, int discard_levels, int max_components,
              int repeat_factor, int max_active_frames, bool want_display);
      /* [SYNOPSIS]
         Prepares the queue for use.  The function actually opens the first
         frame in the source to learn its dimensions and other precision
         information.  On return, the `source' is positioned to the first
         frame again, in case the caller wishes to open frames in a naive
         way.  You can query the various interface functions below to
         discover the dimensional information discovered during
         initialization.
         [//]
         The `repeat_factor' argument indicates the number of
         times the complete sequence will be repeated over and over again
         before the object reports that the source is exhausted.
         [//]
         The `max_active_frames' argument indicates the number of frames which
         the object is prepared to keep active at any given time, where an
         active frame is one whose compressed data stream has been read into
         memory, but whose processed contents have not yet been retrieved
         via the `get_processed_frame' function.  Normally, you would want to
         make `max_active_frames' somewhat larger than the number of frame
         processing engines you have.
         [//]
         If `want_display' is true, `vex_frame' objects created by this object
         will have a component-interleaved format, with non-zero
         `vex_frame::stride' and at most 4 image components, regardless
         of `max_components'.  Otherwise, the components will appear one
         after the other in each frame buffer, which is the most suitable
         format for writing the output to a VIX file.
         [//]
         The function returns false if no frames can be opened from the
         supplied video source.
      */
    int get_num_components()
      { assert(source != NULL); return num_components; }
    bool get_signed()
      { assert(source != NULL); return is_signed; }
    int get_precision()
      { assert(source != NULL); return precision; }
    kdu_dims get_frame_dims()
      { assert(source != NULL); return frame_dims; }
    kdu_coords get_component_subsampling(int c)
      { assert((source != NULL) && (c >= 0) && (c < num_components));
        return component_subs[c]; }
    kdu_dims get_component_dims(int c)
      { assert((source != NULL) && (c >= 0) && (c < num_components));
        return component_dims[c]; }
    void set_truncation_factor(kdu_int32 factor)
      { this->block_truncation_factor=factor; }
      /* [SYNOPSIS]
         You may use this function to dynamically adjust the level of
         code-block truncation, if any, for trading computation time
         against quality.  If `factor' is 0, all code-block data will be
         decompressed as-is.  For larger values, some coding passes may be
         trimmed away to accelerate decompression at the expense of
         quality.  You may call this function at any time, but it may take
         some time for its effect to propagate down to the threads which
         are ultimately performing block decoding tasks.  Nevertheless,
         changes here can usefully affect the speed/quality tradeoff even
         for frames whose decompression is in progress, so long as the
         frames are not too small.
         [//]
         For more information on the interpretation of the `factor' argument,
         consult the comments appearing with the core function,
         `kdu_codestream::set_block_truncation'.
      */
    kdu_int32 get_truncation_factor() { return block_truncation_factor; }
      /* [SYNOPSIS]
         Used by `vex_engine' to find the latest block truncation factor to
         pass into its calls to `vex_processor::start_frame' and
         `vex_processor::process_frame'.
      */
    vex_frame *get_processed_frame(vex_frame_memory_allocator *allocator);
      /* [SYNOPSIS]
         It is not safe to invoke this function from more than one thread
         at once.  It is expected that you have a single queue management
         thread, which calls this function.
         [//]
         This function is where the queue manager does most of its processing.
         The function first checks to see whether any new active frames can
         be added to the queue without exceeding the `max_active_frames' limit.
         If so, their compressed data is loaded into memory and their
         codestream interface is created here.  Once no more active frames
         can be created, the function waits (if necessary) until the frame
         at the head of the active queue has been fully processed.  At that
         point, the function moves the head of the active queue onto the
         `processed_frames' queue (for safe keeping in case of an unexpected
         exception) and returns a pointer to it.
         [//]
         If new frames must be created, this is done using the supplied
         `allocator' object's `allocate_mem' function, which allows for the
         creation of custom display memory surfaces, if desired.  The
         present object can always be used as the `allocator' if all you
         need is to allocate memory from the general heap.  If a different
         `allocator' object is used, you must be sure not to destroy it
         until after the present object has been destroyed.
         [//]
         Note that this is a blocking call; it returns NULL only once
         all frames in the video sequence have been processed.
         [//]
         The caller may choose to write the contents of a returned frame to
         disk, blast them to a display adapter, or simply ignore them.  In
         any case, the retrieved frame should be returned to the present
         object by a subsequent call to `recycle_processed_frame', so that
         its resources can be re-used.  However, there is no obligation to
         do this immediately.  This allows the caller to maintain multiple
         processed frames if required.
         [//]
         If one of the frame processing engines calls `frame_aborted', the
         present function may throw an exception, as explained in connection
         with that function.  However, even if this happens, the object is
         not left in an unpredictable state.  It is possible to resume
         processing in the frame queue, since the exception condition will
         automatically be cleared.  In this case, the aborted frame will
         eventually be retrieved by calls to the present function -- it will
         be marked as aborted.  If the failure was caused originally by a
         memory allocation failure (i.e., `std::bad_alloc' was caught), the
         internal representation will hold `KDU_MEMORY_EXCEPTION' and the
         exception will be rethrown here as `std::bad_alloc' once more.
         Thus, exceptions thrown by this function can be expected to be of
         type `kdu_exception' or `std::bad_alloc'.
      */
    void recycle_processed_frame(vex_frame *frame);
      /* [SYNOPSIS]
         As explained above, each call to `get_processed_frame' should
         ultimately be matched by one to this function.  However, in the
         event that an unexpected condition causes premature termination of
         the normal processing sequence, it is not strictly required to
         recycle outstanding frames.  This is because an internal record
         is kept of all frames retrieved via `get_processed_frame' so that
         they can be deleted by the `vex_frame_queue' destructor.
         [//]
         Frames must be recycled in the same order in which they were
         originally retrieved via the `get_processed_frames' function.
      */
    vex_frame *get_frame_to_process(vex_engine *engine, bool blocking);
      /* [SYNOPSIS]
         This function is where each frame processing engine comes to get
         a frame to process.  The function may block the caller if there
         are no active frames which are not already being processed.  It is,
         however, possible to avoid blocking by specifying `blocking'=false.
         This is useless for single-threaded frame processing engines, but
         multi-threaded frame processing engines typically request one frame
         ahead, so that they can add make sure there is always work to keep
         all threads active.  Failure to access a frame ahead of time need
         not be a problem, since they can always call again later while
         getting on with processing the frame which they have in hand.  If
         `blocking' is true, the function returns NULL only once the video
         sequence has been fully processed or allocated to other engines.
         [//]
         Each successful call to this function should be matched by one to
         `frame_processed', unless some unexpected exception occurs which
         interrupts the normal processing sequence and results in destruction
         of the `vex_frame_queue' object.
      */
    void frame_processed(vex_frame *frame);
      /* [SYNOPSIS]
         Each call to `get_frame_to_process' must be matched by one to this
         function, once the frame has been fully processed. */
    void frame_aborted(vex_frame *frame, kdu_exception exception_code);
      /* [SYNOPSIS]
         This function is called from a processing engine if an exception
         condition occurred while processing the frame -- the condition will
         most likely already have generated a call to `kdu_error', although
         memory allocation problems caught as `std::bad_alloc' are converted
         to `KDU_MEMORY_EXCEPTION' so that their nature is not lost.  The
         function causes an exception to be thrown in the queue management
         thread at the earliest convenience, using the supplied
         `exception_code'.  In practice, this exception is thrown on the
         next call to `get_processed_frame', if one is not already in
         process.
      */
    void terminate();
      /* [SYNOPSIS]
         This function may be called directly from the top level application,
         or indirectly via `vex_engine::shutdown'.  The latter occurs if a
         call to `vex_engine::shutdown' with `graceful'=false found the
         engine to be blocked.  In either case, this function ultimately
         unblocks any threads which are blocked on `get_frame_to_process'
         and ensures that these and all future functions return NULL.
      */
    virtual void *
      allocate_mem(int num_components, kdu_dims *component_dims,
                   size_t sample_bytes, int format,
                   kdu_byte * &buffer, size_t &frame_bytes,
                   size_t &stride, bool &fully_aligned);
      /* [SYNOPSIS]
         Implements the `vex_frame_memory_allocator::allocate_mem'
         function, putting all memory on the heap.
      */
    virtual void
      destroy_mem(void *handle); 
      /* [SYNOPSIS]
         Implements the `vex_frame_memory_allocator::destroy_mem'
         function.
      */
  private: // Data
    mj2_video_source *source;
    kdu_int32 block_truncation_factor;
    int discard_levels;
    int num_components;
    bool is_signed;
    int precision;
    kdu_dims frame_dims; // Apply sub-sampling factors to get component dims
    kdu_coords *component_subs; // Subsampling factors for each component
    kdu_dims *component_dims; // Dimensions and locations of each component

    int repeat_count; // When this reaches 0, we stop reading the source
    int next_frame_to_open; // Index supplied to `source->seek_to_frame'
    int max_active_frames;
    int num_active_frames; // Number of frames in the `active_frames' list
    int num_waiting_frames; // Number of active frames with `waiting'=true
    vex_frame *active_head; // List of frames whose box is open for processing
    vex_frame *active_tail; // Append new frames ready for processing here
    vex_frame *processed_head;
    vex_frame *processed_tail;
    vex_frame *recycled_frames;
    size_t sample_bytes; // For `vex_frame_memory_allocator::allocate_mem'.
    int frame_format; // VEX_FRAME_FORMAT_CONTIG or VEX_FRAME_FORMAT_XRGB
    bool exhausted; // True if all source frames have been opened
    bool terminated; // True if queue should appear empty to engines
    bool exception_raised;
    kdu_exception last_exception_code;
    kdu_mutex mutex; // Used to guard access to avoid state access conflicts
    kdu_event active_ready; // Signalled when an active frame has been
       // processed -- this may wake up the management thread in a call to
       // `get_processed_frame'.  This is an auto-reset event, since only
       // one thread can use `get_processed_frame'.
    kdu_event active_waiting; // Set if any active frame has `waiting'=true;
                        // manually reset, once there are no more such frames.
  };

#endif // KDU_VEX_H
