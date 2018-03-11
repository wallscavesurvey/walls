/*****************************************************************************/
// File: mj2_local.h [scope = APPS/JP2]
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
   Defines local classes used by the internal machinery behind the MJ2
(Motion JPEG2000) interfaces.  The relevant machinery is implemented in
"mj2.cpp".  You should respect the local nature of this header file and not
include it directly from an application or any other part of the Kakadu
system (not in the APPS/JP2 scope).
******************************************************************************/
#ifndef MJ2_LOCAL_H
#define MJ2_LOCAL_H

#include <assert.h>
#include "mj2.h"
#include "jp2_shared.h"

/* Note Carefully:
      If you want to be able to use the "kdu_text_extractor" tool to
   extract text from calls to `kdu_error' and `kdu_warning' so that it
   can be separately registered (possibly in a variety of different
   languages), you should carefully preserve the form of the definitions
   below, starting from #ifdef KDU_CUSTOM_TEXT and extending to the
   definitions of KDU_WARNING_DEV and KDU_ERROR_DEV.  All of these
   definitions are expected by the current, reasonably inflexible
   implementation of "kdu_text_extractor".
      The only things you should change when these definitions are ported to
   different source files are the strings found inside the `kdu_error'
   and `kdu_warning' constructors.  These strings may be arbitrarily
   defined, as far as "kdu_text_extractor" is concerned, except that they
   must not occupy more than one line of text.
      When defining these macros in header files, be sure to undefine
   them at the end of the header.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("E(mj2_local.h)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(mj2_local.h)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu File Format Support:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu File Format Support:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


// Defined here
class mj_sample_sizes;
class mj_chunk_offsets;
class mj_sample_chunks;
class mj_sample_times;
struct mj_time;
struct mj_track;
class mj_track_buf;
struct mj_video_read_state;
struct mj_video_write_state;
class mj_video_track;
class mj_movie;

/* ========================================================================= */
/*                                 Classes                                   */
/* ========================================================================= */

/*****************************************************************************/
/*                              mj_sample_sizes                              */
/*****************************************************************************/

class mj_sample_sizes {
  public: // Member functions
    mj_sample_sizes()
      {
        total_samples=default_size=current_sample_idx=0;
        head=current=tail=NULL;
      }
    ~mj_sample_sizes()
      {
        while ((tail=head) != NULL)
          { head=tail->next; delete tail; }
      }
    kdu_uint32 get_num_samples() { return total_samples; }
    void load_from_box(jp2_input_box *stsz);
      /* Loads the sample size information from an STSZ
         (Sample Table -> Sample Size) box, as defined by the Motion JPEG2000
         standard (ISO/IEC 15444-3).  The box has already been opened and
         its signature must be `mj2_sample_size_4cc'.  The function closes
         the supplied `stsz' box. */
    void save_to_box(jp2_output_box *super_box);
      /* Saves the sample size information to an STSZ
         (Sample Table -> Sample Size) box, as defined by the Motion JPEG2000
         standard (ISO/IEC 15444-3), creating the new box as a sub-box of the
         supplied `super_box'. */
    void append(kdu_uint32 size);
      /* Add a new sample, indicating its size. */
    void finalize();
      /* Call this function once all sample sizes have been sent to the
         object using `append'. */
    kdu_uint32 get_sample_size(kdu_uint32 sample_idx);
      /* Retrieve the size of the indicated sample. */
  private: // Declarations
      struct mj_sample_size_store {
          mj_sample_size_store()
            { num_elts=0; remaining_elts=1024; next=NULL; }
          kdu_uint32 num_elts;
          kdu_uint32 remaining_elts;
          kdu_uint32 elts[1024];
          mj_sample_size_store *next;
        };
  private: // Data
    kdu_uint32 total_samples;
    kdu_uint32 default_size; // If non-zero, all samples have the same size.
    kdu_uint32 current_sample_idx; // First sample index in `current'
    mj_sample_size_store *head;
    mj_sample_size_store *current; // For reading
    mj_sample_size_store *tail; // For writing
  };

/*****************************************************************************/
/*                             mj_chunk_offsets                              */
/*****************************************************************************/

class mj_chunk_offsets {
  public: // Member functions
    mj_chunk_offsets()
      {
        total_chunks = current_chunk_idx = 0; max_offset = 0;
        head = current = tail = NULL;
      }
    ~mj_chunk_offsets()
      {
        while ((tail=head) != NULL)
          { head=tail->next; delete tail; }
      }
    void load_from_box(jp2_input_box *stco);
      /* Loads the sample size information from an STCO or CO64
         (Sample Table -> Chunk Offset) box, as defined by the Motion
         JPEG2000 standard (ISO/IEC 15444-3).  The box has already been opened
         and its signature must be one of `mj2_chunk_offset_4cc' or
         `mj2_chunk_offset64_4cc'.  The function closes
         the supplied `stco' box. */
    void save_to_box(jp2_output_box *super_box);
      /* Saves the sample size information to an STCO or CO64
         (Sample Table -> Chunk Offset) box, as defined by the Motion JPEG2000
         standard (ISO/IEC 15444-3), creating the new box as a sub-box of the
         supplied `super_box'. */
    void append(kdu_long offset);
      /* Add a new chunk, indicating the offset of that chunk from the
         start of the containing file. */
    kdu_long get_chunk_offset(kdu_uint32 chunk_idx);
      /* Retrieve the location of the indicated chunk, relative to the
         start of the containing file. */
  private: // Declarations
      struct mj_chunk_offset_store {
          mj_chunk_offset_store()
            { num_elts=0; remaining_elts=1024; next=NULL; }
          kdu_uint32 num_elts;
          kdu_uint32 remaining_elts;
          kdu_long elts[1024];
          mj_chunk_offset_store *next;
        };
  private: // Data
    kdu_uint32 total_chunks;
    kdu_long max_offset;
    kdu_uint32 current_chunk_idx; // Index of first chunk in current record
    mj_chunk_offset_store *head;
    mj_chunk_offset_store *current; // For reading
    mj_chunk_offset_store *tail; // For writing
  };

/*****************************************************************************/
/*                             mj_sample_chunks                              */
/*****************************************************************************/

class mj_sample_chunks {
  public: // Member functions
    mj_sample_chunks()
      {
        current_chunk_idx = current_sample_idx = sample_counter = 0;
        head = current = tail = NULL;
      }
    ~mj_sample_chunks()
      {
        while ((tail=head) != NULL)
          { head=tail->next; delete tail; }
      }
    void load_from_box(jp2_input_box *stsc);
      /* Loads the sample size information from an STSC
         (Sample Table -> Sample to Chunk) box, as defined by the Motion
         JPEG2000 standard (ISO/IEC 15444-3).  The box has already been opened
         and its signature must be `mj2_sample_to_chunk_4cc'.  The function
         closes the supplied `stsc' box. */
    void save_to_box(jp2_output_box *super_box);
      /* Saves the sample size information to an STSC
         (Sample Table -> Sample to Chunk) box, as defined by the Motion
         JPEG2000 standard (ISO/IEC 15444-3), creating the new box as a
         sub-box of the supplied `super_box'. */
    void append_sample(kdu_uint32 chunk_idx);
      /* Notify the object that a new sample is being added, belonging to
         the indicated chunk.  Chunk indices start from 0 and should increase
         monotonically with the sample index. */
    void finalize();
      /* Call this function once all samples have been appended. */
    kdu_uint32 get_sample_chunk(kdu_uint32 sample_idx,
                                kdu_uint32 &sample_in_chunk);
      /* Returns the index of the chunk to which the indicated sample belongs
         and sets `sample_in_chunk' to the location of the sample within
         this chunk (0 for the first sample in the chunk). */
  private: // Declarations
      struct mj_samples_per_chunk {
          mj_samples_per_chunk()
            { num_samples=0; run=0; next=NULL; }
          kdu_uint32 num_samples;
          kdu_uint32 run; // If 0, the run goes on forever.
          mj_samples_per_chunk *next;
        };
  private: // Data
    kdu_uint32 current_chunk_idx; // Used for reading and writing
    kdu_uint32 current_sample_idx; // Only for reading
    kdu_uint32 sample_counter; // Counts samples in chunk; only for writing
    mj_samples_per_chunk *head;
    mj_samples_per_chunk *current; // For reading
    mj_samples_per_chunk *tail; // For writing
  };

/*****************************************************************************/
/*                              mj_sample_times                              */
/*****************************************************************************/

class mj_sample_times {
  public: // Member functions
    mj_sample_times()
      {
        total_samples=total_ticks=current_sample_idx=current_tick=0;
        head=current=tail=NULL;
      }
    ~mj_sample_times()
      {
        while ((tail=head) != NULL)
          { head=tail->next; delete tail; }
      }
    kdu_uint32 get_num_samples() { return total_samples; }
    void load_from_box(jp2_input_box *stts);
      /* Loads the sample size information from an STTS
         (Sample Table -> Time to Sample) box, as defined by the Motion
         JPEG2000 standard (ISO/IEC 15444-3).  The box has already been opened
         and its signature must be `mj2_time_to_sample_4cc'.  The function
         closes the supplied `stts' box. */
    void save_to_box(jp2_output_box *super_box);
      /* Saves the sample size information to an STTS
         (Sample Table -> Time to Sample) box, as defined by the Motion
         JPEG2000 standard (ISO/IEC 15444-3), creating the new box as a
         sub-box of the supplied `super_box'. */
    void append(kdu_uint32 sample_ticks);
      /* Adds a new sample, whose duration is `sample_ticks', measured in
         the time scale (number of ticks per second) of the containing
         track. */
    kdu_uint32 get_period();
      /* Returns the duration of the current sample, measured in the time
         scale of the containing track).  The current sample is the first
         sample in the track unless `seek_to_samle' or `seek_to_time'
         have been called. */
    kdu_uint32 seek_to_sample(kdu_uint32 sample_idx);
      /* After calling this function, the next call to `get_period' will return
         the duration of the sample indicated by `sample_idx'.  Sample indices
         start from 0.  The function returns the cumulative time occupied by
         all preceding samples in the track, measured in the track's time
         scale (ticks per second).  This return value is effectively the
         sample's start time. */
    kdu_uint32 seek_to_time(kdu_uint32 time);
      /* Similar to `seek_to_sample' except that the function seeks to the
         last sample whose start time is less than or equal to the indicated
         `time', measured in the time scale (ticks per second) of the
         containing track.  The function returns the index of the relevant
         sample, where sample indices start from 0. */
    kdu_uint32 get_duration() { return total_ticks; }
  private: // Declarations
      struct mj_ticks_per_sample {
          mj_ticks_per_sample()
            { num_ticks=0; run=0; next=NULL; }
          kdu_uint32 num_ticks;
          kdu_uint32 run;
          mj_ticks_per_sample *next;
        };
  private: // Data
    kdu_uint32 total_samples;
    kdu_uint32 total_ticks;
    kdu_uint32 current_sample_idx;  // For reading -- idx at start of `current'
    kdu_uint32 current_tick; // For reading -- time at start of `current'
    mj_ticks_per_sample *head;
    mj_ticks_per_sample *current; // For reading
    mj_ticks_per_sample *tail;
  };

/*****************************************************************************/
/*                                  mj_time                                  */
/*****************************************************************************/

struct mj_time {
  public: // Functions
    mj_time()
      { creation_time = modification_time = duration = 0; timescale = 1000; }
    bool need_long_version()
      {
#ifdef KDU_LONG64
        return (((creation_time | modification_time | duration)>>32) != 0);
#else
        return false;
#endif // KDU_LONG64
      }
    void set_defaults();
      /* Call this function to replace any zero-valued creation or modification
         times with appropriate defaults.  In practice, when either quantity
         is missing, the two quantities will be set to the same value and
         when both are missing they will be both be set to the current
         time, as indicated by the system clock. */
  public: // Data
#ifdef KDU_LONG64
    kdu_long creation_time; // Measured in seconds since midnight 1/1/1904
    kdu_long modification_time; // Measured in seconds since midnight 1/1/1904
    kdu_long duration; // Measured in units of an appropriate time scale
#else
    kdu_uint32 creation_time;
    kdu_uint32 modification_time;
    kdu_uint32 duration;
#endif
    kdu_uint32 timescale;
  };

/*****************************************************************************/
/*                                 mj_track                                  */
/*****************************************************************************/

struct mj_track {
  public: // Member functions
    mj_track(mj_movie *movie)
      {
        this->movie = movie; disabled = false;
        track_idx=0; next=NULL; video_track=NULL; num_samples=0;
        compositing_order = 0; presentation_volume=1.0;
        for (int i=0; i < 9; i++)
          transformation_matrix[i] = (i%4)?0.0:1.0;
      }
    ~mj_track();
    void flush();
      /* Call this function before writing the movie meta data.  Flushes out
         any partially assembled data for the track. */
    void load_from_box(jp2_input_box *trak);
      /* Loads all track information (everything except the compressed
         data itself) from the supplied TRAK box, whose format should
         conform to the Motion JPEG2000 standard (ISO/IEC 15444-3).  The
         `trak' box has already been opened and its signature must be
         `mj_track_4cc'.  The function closes the supplied `trak' box. */
    void save_to_box(jp2_output_box *super_box);
      /* Saves all track information (everything except the compressed
         data itself) to a Motion JPEG2000 TRAK box, creating this new
         box as a sub-box of the supplied `super_box'. */
  private: // Helper functions
    void read_track_header_box(jp2_input_box *box);
      /* Caller should check to see if the `disabled' flag is set. */
    void write_track_header_box(jp2_output_box *super_box);
    void read_media_header_box(jp2_input_box *box);
    void write_media_header_box(jp2_output_box *super_box);
    void read_media_handler_box(jp2_input_box *box);
    void write_media_handler_box(jp2_output_box *super_box);
    void read_data_reference_box(jp2_input_box *box);
      /* Sets the `disabled' flag if the track contains external data
         references. */
    void write_data_reference_box(jp2_output_box *super_box);
    void read_sample_description_box(jp2_input_box *box);
      /* Sets the `disabled' flag if the track contains multiple sample
         descriptions -- not currently supported. */
    void write_sample_description_box(jp2_output_box *super_box);
  public: // Data
    bool disabled; // True only if a disabled track's header is read.
    mj_movie *movie; // Movie to which the track belongs
    kdu_uint32 track_idx;
    mj_track *next; // Tracks are maintained in a linked list.
    kdu_uint32 handler_id; // Currently only `mj2_video_handler_4cc' supported
    mj_video_track *video_track; // NULL if this is not a video track

    mj_time track_times; /* Refers to the track, including edits, using the
                            movie's time scale. */
    mj_time media_times; /* Refers to the media itself, without edits, using
                            the track's time scale. */

    double presentation_volume;

    double presentation_width; // See below
    double presentation_height; // See below
    double transformation_matrix[9]; // See below

    kdu_int16 compositing_order;

    kdu_uint32 num_samples; // Total number of samples stored so far
    mj_sample_sizes sample_sizes; // Manages info in the sample size box
    mj_chunk_offsets chunk_offsets; // Manages info in the chunk offsets box
    mj_sample_chunks sample_chunks; // Manages info in the sample to chunk box
    mj_sample_times sample_times; // Manages info in the sample to time box
  };
  /* Notes:
           The `presentation_width' and `presentation_height' values are
        stored in the track header as 16.16 fixed-point quantities.  They
        need not take integer values.  Notionally, the video frames are
        scaled to fit these presentation dimensions and are then subjected
        to the transformation described by `transformation_matrix'.
           Each point (p,q) on the surface of the image, after scaling to
        the presentation dimensions mentioned above, is subjected to the
        following transformation:
            | m |   | a   c   x |   | p |
            | n | = | b   d   y | * | q |
            | z |   | u   v   w |   | 1 |
        where the transformed point lies at x' = m/z and y'=n/z.  However,
        the quantities u, v and w in the above matrix are required hold
        0, 0 and 1, respectively, so that the transformation can also be
        written as:
            | x' |   | a   c |   | p |     | x |
            |    | = |       | * |   |  +  |   |
            | y' |   | b   d |   | q |     | y |
        The quantities from the above matrix are stored in lexicographic
        order within the `transformation_matrix' array, although we note
        that the corresponding fixed-point quantities are stored in the
        MJ2 track header in column-wise order.  Prior to transformation,
        the presentation points (p,q) are considered to be anchored such
        that the upper-left corner of the image is at (0,0). */

/*****************************************************************************/
/*                              mj_chunk_buf                                 */
/*****************************************************************************/

class mj_chunk_buf {
  public: // Member functions
    mj_chunk_buf()
      { head = tail = current = mark = NULL;
        current_bytes_retrieved = mark_bytes_retrieved = 0; }
    ~mj_chunk_buf()
      {
        while ((tail=head) != NULL)
          { head=tail->next; delete tail; }
      }
    void clear()
      /* Prepare to write a new chunk. */
      {
        tail = current = mark = NULL;
        mark_bytes_retrieved = current_bytes_retrieved = 0;
      }
    void store(const kdu_byte *buf, int num_bytes);
      /* Appends the indicated number of bytes to the end of the buffer. */
    void write(jp2_output_box *box, int num_bytes);
      /* Retrieves the next `num_bytes' from the store, writing them to
         the supplied open output box (should have been opened as a
         contiguous code-stream box).  Sufficient bytes must be available to
         complete the operation. */
    void set_mark()
      { mark = current; mark_bytes_retrieved = current_bytes_retrieved; }
    void goto_mark()
      { current = mark; current_bytes_retrieved = mark_bytes_retrieved; }
      /* These functions may be used while retrieving data from the buffer.
         Calling `goto_mark' will cause subsequent data retrieval calls to
         recover data from the position at which `set_mark' was called.  If
         `set_mark' has not been called, `goto_mark' will return to the
         start of the buffered data. */
  private: // Declarations
      struct mj_chunk_store {
          mj_chunk_store()
            { num_elts=0; remaining_elts=100000; next = NULL; }
          int num_elts; // Number of valid entries in `elts'
          int remaining_elts; // Number of free entries in `elts'
          kdu_byte elts[100000];
          mj_chunk_store *next;
        };
  private: // Data
    mj_chunk_store *head;
    mj_chunk_store *tail; // Used for storing
    mj_chunk_store *current; // Used for retrieving
    int current_bytes_retrieved; // Num bytes already retrieved from `current'
    mj_chunk_store *mark; // Used by `set_mark' and `goto_mark'
    int mark_bytes_retrieved;
  };

/*****************************************************************************/
/*                            mj_video_read_state                           */
/*****************************************************************************/

struct mj_video_read_state {
  public: // Member functions
      mj_video_read_state()
        {
          source = NULL;  num_frames = 0;  fields_per_frame = 0;
          open_frame_idx = open_field_idx = 0;
          open_frame_instant = open_frame_period = 0;
          next_frame_idx = next_field_idx = 0;
          recent_chunk_idx = recent_frame_in_chunk = 0;
          recent_frame_pos = -1;  field_step = 1;
          reset_next_frame_info();
        }
      ~mj_video_read_state()
        { if (source != NULL) delete source; }
      void reset_next_frame_info()
        { // Called when `next_frame_idx' changes
          next_frame_instant = next_frame_period = 0;
          next_frame_pos = -1; next_field0_len = 0;
        }
      kdu_long find_frame_pos(mj_track *track, kdu_uint32 frame_idx);
        /* Fills in the `next_frame_pos' value, using the chunk state info
           to efficiently navigate the chunk size information. */
  public: // Track-wide information
    mj2_video_source *source; // Non-NULL if video track is readable.
    kdu_uint32 num_frames;
    kdu_uint32 fields_per_frame;
  public: // State variables which describe a currently open image
    kdu_uint32 open_frame_idx; // Frame index of currently open image
    kdu_uint32 open_field_idx; // Field index of currently open image
    kdu_uint32 open_frame_instant; // Frame start time for current open image
    kdu_uint32 open_frame_period; // Duration of frame containing open image
  public: // State variables which describe the next frame for `open_image'
    kdu_uint32 next_frame_idx; // Index of next frame to open (starts from 0)
    kdu_uint32 next_field_idx; // Index of next field to open (0 or 1)
    kdu_uint32 next_frame_instant; // Start time of next frame (0 if unknown)
    kdu_uint32 next_frame_period; // Duration of next frame (0 if unknown)
    kdu_long next_frame_pos; // Absolute location of next frame (-1 if unknown)
    kdu_long next_field0_len; // Length of 1st field codestream (0 if unknown)
    kdu_uint32 field_step; // step=1 unless interlaced with field mode 0 or 1,
                           // in which case step=2.
  public: // State variables used by `find_frame_pos()'.
    kdu_uint32 recent_chunk_idx; // Recently referenced chunk
    kdu_uint32 recent_frame_in_chunk; // Recent frame pos w.r.t chunk
    kdu_long recent_frame_pos; // Recent frame's absolute location in file
  public: // State variables which are valid only when a field is open
    jp2_input_box field_box; // Box containing current field's code-stream
  };

/*****************************************************************************/
/*                            mj_video_write_state                           */
/*****************************************************************************/

struct mj_video_write_state {
  public: // Member functions
    mj_video_write_state()
      {
        target = NULL;
        field_idx = chunk_frames = 0;
        fields_per_frame = max_chunk_frames = 1;
        chunk_time = frame_time = chunk_idx = 0;
      }
    ~mj_video_write_state()
      { if (target != NULL) delete target; }
    void set_max_chunk_frames(kdu_uint32 max_frames)
      {
        max_frames = (max_frames < 1)?1:max_frames;
        max_chunk_frames = (max_frames>32)?32:max_frames;
      }
    bool started()
      { // Returns true if one or more fields have been opened.
        return (chunk_idx || chunk_frames);
      }
    void flush_chunk(mj_track *track);
      /* Flushes the contents of the existing, buffered chunk, to the output
         device (discovered via `track'), resetting all current chunk
         counters and advancing the chunk index.  This function appends
         chunk and sample information to the `track->sample_times' and
         `track->chunk_offsets' objects.  If the final frame is incomplete,
         (only one of two interlaced fields written), the last field is
         simply duplicated. */
  public: // Data
    mj2_video_target *target; // Non-NULL if video track is writable
    kdu_uint32 field_sizes[64]; // Sizes of fields in current chunk
    kdu_uint32 field_idx; // Index into `field_sizes'
    kdu_uint32 fields_per_frame; // Either 1 or 2.
    kdu_uint32 chunk_frames; // Number of frames opened within current chunk
    kdu_uint32 max_chunk_frames; // Max frames allowed in any chunk
    kdu_uint32 chunk_time; // Num ticks for frames opened within currrent chunk
    kdu_uint32 frame_time; // Number of ticks in next frame to be opened
    kdu_uint32 chunk_idx; // Index of current chunk
    mj_chunk_buf chunk_buf;
  };

/*****************************************************************************/
/*                              mj_video_track                               */
/*****************************************************************************/

class mj_video_track {
  public: // Member functions
    mj_video_track(mj_track *track)
      {
        this->track=track; field_order=KDU_FIELDS_NONE;
        graphics_mode = 0; opcolour[0] = opcolour[1] = opcolour[2] = 0;
        frame_width = frame_height = 0; field_open = false;
        horizontal_resolution = vertical_resolution = 72.0F;
        first_codestream_id = -1;  num_codestreams = 0;
      }
    void flush()
      { // Flushes any partially written chunks out
        if (field_open)
          { KDU_ERROR_DEV(e,0); e <<
              KDU_TXT("Attempting to complete a Motion JPEG2000 file "
              "(writing) without first closing all open video fields.");
          }
        write_state.flush_chunk(track);
      }
    void read_media_header_box(jp2_input_box *vmhd);
      /* Reads an open VMHD (Video Media Header) box, closing the box when
         done. */
    void write_media_header_box(jp2_output_box *super_box);
      /* Creates and writes a VMHD (Video Media Header) box, as a sub-box
         of `super_box'. */
    void read_sample_entry_box(jp2_input_box *entry);
      /* Reads the visual sample entry box, within a Motion JPEG2000 sample
         description box.  On entry, the visual sample entry box has been
         opened and its type code must be `mj2_visual_sample_entry'.  The
         function closes the `entry' box when done.  If the entry is legal,
         but the current implementation will not be able to read this track,
         the function disables the current track.  The caller should check
         for this condition upon return, since there is no need to finish
         reading a disabled track's meta-data. */
    void write_sample_entry_box(jp2_output_box *super_box);
      /* Creates a new visual sample entry box within the sample description
         box identified by `super_box', writing the JP2 header, field ordering
         and any other information before closing the sample entry box and
         returning. */
    void initialize_read_state();
      /* This function is called from `mj_track::load_from_box' once all
         meta-data has been read from the Motion JPEG2000 TRAK box. */
    mj2_video_target *get_target()
      {
        if (write_state.target == NULL)
          { write_state.target = new mj2_video_target;
            write_state.target->state = this; }
        return write_state.target;
      }
    mj2_video_source *get_source()
      { return read_state.source; }
  private: // Data
    friend class mj2_video_target;
    friend class mj2_video_source;
    mj_track *track;
    kdu_int16 graphics_mode;
    kdu_int16 opcolour[3];
    kdu_int16 frame_width;
    kdu_int16 frame_height;
    double horizontal_resolution;
    double vertical_resolution;
    jp2_header header; // Manages info from JP2 header box
    bool field_open; // True if there is an open image
    mj_video_read_state read_state; // Manages state information for reading
    mj_video_write_state write_state; // Manages state information for writing
  public: // Public data
    kdu_field_order field_order; // Progressive or interlaced field order
    int num_codestreams; // num_frames * fields_per_frame
    int first_codestream_id; // Unique ID of first codestream; -ve until
                             // set by `mj2_source::count_codestreams'
  };

/*****************************************************************************/
/*                                 mj_movie                                  */
/*****************************************************************************/

class mj_movie {
  public: // Member functions
    mj_movie()
      {
        writing = false; tgt = NULL; src=NULL;
        tracks = NULL; playback_rate = 1.0; playback_volume = 1.0;
        for (int i=0; i < 9; i++)
          transformation_matrix[i] = (i%4)?0.0:1.0;
      }
    ~mj_movie();
    jp2_family_src *get_src()
      { // Used only when reading a movie.
        return src;
      }
    jp2_family_tgt *get_tgt()
      { // Used only when writing a movie.
        return tgt;
      }
    jp2_output_box *get_mdat()
      { // Accesses the `mdat' box for writing new frame chunks.
        return &mdat;
      }
  private: // Helper functions
    void read_movie_header_box(jp2_input_box *box);
    void write_movie_header_box(jp2_output_box *super_box);
  private: // Data
    friend class mj2_source;
    friend class mj2_target;
    friend class mj2_video_source;
    bool writing; // True if the object was created for writing.
    jp2_family_src *src; // Non-NULL if opened for reading
    jp2_family_tgt *tgt; // Non-NULL if opened for writing
    jp2_output_box mdat; // Remains open during code-stream generation.
    mj_track *tracks;
    mj_time movie_times;
    double playback_rate;
    double playback_volume;
    double transformation_matrix[9]; // See namesake in `mj_track'.
    kdu_dims movie_dims; // Empty region until figured out
  };

#undef KDU_ERROR
#undef KDU_ERROR_DEV
#undef KDU_WARNING
#undef KDU_WARNING_DEV
#undef KDU_TXT

#endif // MJ2_LOCAL_H
