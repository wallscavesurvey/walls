/*****************************************************************************/
// File: kdu_video_io.h [scope = APPS/COMPRESSED-IO]
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
   Defines classes derived from "kdu_compressed_source" and
"kdu_compressed_target" which may be used by video processing applications.
A pair of abstract base classes provide generic video management tools,
building on those of "kdu_compressed_source" and "kdu_compressed_target",
which may be implemented in a variety of ways.  A simple implementation
of these base classes is provided here for use with sequential, video
sequences, while a much more sophisticated implementation is provided
in "mj2.h" to support the Motion JPEG2000 file format.
******************************************************************************/

#ifndef KDU_VIDEO_IO_H
#define KDU_VIDEO_IO_H

#include <stdio.h> // Use C I/O functions for speed; can make a big difference
#include <string.h>
#include "kdu_file_io.h"

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
     kdu_error _name("E(kdu_video_io.h)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(kdu_video_io.h)",_id);
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


// Classes defined here:
class kdu_compressed_video_source;
class kdu_compressed_video_target;
class kdu_simple_video_source;
class kdu_simple_video_target;

/*****************************************************************************/
/*                              kdu_field_order                              */
/*****************************************************************************/

enum kdu_field_order {
    KDU_FIELDS_NONE,
    KDU_FIELDS_TOP_FIRST,
    KDU_FIELDS_TOP_SECOND
  };

/* ========================================================================= */
/*                         Abstract Base Classes                             */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdu_compressed_video_source                          */
/*****************************************************************************/

class kdu_compressed_video_source :
  public kdu_compressed_source {
  /* [BIND: reference]
     [SYNOPSIS]
     This abstract base class defines core services of interest to
     applications working with compressed video content.  Itself
     derived from `kdu_compressed_source', implementations of this class
     may be passed to `kdu_codestream::create', for the purpose of
     parsing and/or decompressing individual images from a compressed video
     source.
     [//]
     Kakadu's implementation of the Motion JPEG2000 file format offers
     an appropriately derived class (`mj2_video_source'), which implements the
     interfaces declared here.  For a much simpler implementation, or
     inspiration for implementing your own video source classes, you might
     consider the `kdu_simple_video_source' class.
  */
  public: // Member functions
    virtual kdu_uint32 get_timescale() { return 0; }
      /* [SYNOPSIS]
           If the video source provides no timing information, this function
           may return 0.  Otherwise, it returns the number of ticks per
           second, which defines the time scale used to describe frame
           periods.  See `get_frame_period'.
      */
    virtual kdu_field_order get_field_order() { return KDU_FIELDS_NONE; }
      /* [SYNOPSIS]
           Returns KDU_FIELDS_NONE if the video track contains progressive
           scan frames.  Some video sources may not be able to support anything
           other than progressive scan frames; however, it is convenient to
           provide support for interlaced formats directly from the abstract
           base class.
           [//]
           For interlaced video, the function returns one of
           KDU_FIELDS_TOP_FIRST or KDU_FIELDS_TOP_SECOND, meaning that the
           frames are interlaced with the first (respectively, second) field
           (in temporal sequence) holding the frame's top line.
      */
    virtual void set_field_mode(int which) { return; }
      /* [SYNOPSIS]
           This function may be called at any time, to specify which fields
           will be accessed by subsequent calls to `open_image'.  If the
           video is progressive (see `get_field_order'), this function has
           no effect.  Note that some video sources might not support anything
           other than progressive video, in which case the function will also
           do nothing.
         [ARG: which]
           Must be one of 0, 1 or 2.  If 0, calls to `open_image' open
           the first field of the next frame in sequence.  If 1, calls to
           `open_image' open the second field of the next frame in sequence.
           If 2, `open_image' opens each field of the frame in sequence.
      */
    virtual int get_num_frames() { return 0; }
      /* [SYNOPSIS]
           Returns the total number of frames which are available, or 0 if
           the value is not known.  Some video sources might not provide
           an indication of the total number of frames available in a global
           header, in which case they are at liberty to return 0 here.
      */
    virtual bool seek_to_frame(int frame_idx) { return false; }
      /* [SYNOPSIS]
           Call this function to set the index (starts from 0) of the frame
           to be opened by the next call to `open_image'.
         [RETURNS]
           False if the indicated frame does not exist, or frame seeking is
           not supported by the implementation.
      */
    virtual kdu_long get_duration() { return 0; }
      /* [SYNOPSIS]
           If the video source provides no timing information, or the full
           extent of the video is not readily deduced a priori, this function
           may return 0.  Otherwise, it returns the total duration of the
           video track, measured in the time scale (ticks per second)
           identified by the `get_timescale' function.
      */
    virtual int time_to_frame(kdu_long time_instant) { return -1; }
      /* [SYNOPSIS]
           If the video source provides no time indexing capabilities,
           this function may return -1.  Otherwise, it should return the
           index of the frame whose period includes the supplied
           `time_instant', measured in the time scale (ticks per second)
           identified by the `get_timescale' function.
           [//]
           If time indexing is available, but `time_instant' exceeds the
           duration of the video track, the function returns the index of
           the last available frame.  Similarly, if `time_instant' refers
           to a time prior to the start of the video sequence, the function
           should return 0 (the index of the first frame).
      */
    virtual kdu_long get_frame_instant() { return 0; }
      /* [SYNOPSIS]
           If the video source provides no timing information, this function
           may return 0.  Otherwise, it should return the starting time
           of the frame to which the currently open image belongs, measured
           in the time scale (ticks per second) identified by the
           `get_timescale' function.  If no image is currently open,
           the function returns the starting time of the next frame which
           will be opened by `open_image', or the duration of the video track
           if no new frames are available for opening.
           [//]
           Note that the return value should be unaffected by the field mode
           established by `set_field_mode'.  That is, the function returns
           frame starting times, not field starting times, when the video is
           interlaced.
      */
    virtual kdu_long get_frame_period() { return 0; }
      /* [SYNOPSIS]
           If the compressed video source provides no timing information,
           this function returns 0.  Otherwise, it returns the number of
           ticks associated with the frame to which the currently open image
           belongs.  If no image is currently open, the function returns the
           frame period associated with the frame to which the next open
           image would belong if `open_image' were called.  The number of
           ticks per second is identified by the `get_timescale' function.
           If the video is interlaced, there are two images (fields) in each
           frame period.
      */
    virtual int open_image() = 0;
      /* [SYNOPSIS]
           Call this function to open the next video image in sequence,
           providing access to its underlying JPEG2000 code-stream.  The
           sequence of images opened by this function depends upon whether
           the video is interlaced or progressive, and also on any previous
           calls to `set_field_mode'.  For progressive video, the function
           opens each frame in sequence.  If the field mode was set to 0 or
           1, the function also opens each frame of an interlaced video in
           sequence, supplying only the first or second field, respectively,
           of each frame.  If the video is interlaced and the field mode was
           set to 2, the function opens each field of each frame in turn, so
           that the frame index advances only on every second call to this
           function.
           [//]
           After calling this function, the present object may be passed into
           `kdu_codestream::create' for parsing and, optionally, decompression
           of the image's code-stream.  Once the `kdu_codestream' object is
           done (destroyed or re-created), the `close_image' function may be
           called to prepare the object for opening a subsequent image.
         [RETURNS]
           The frame index associated with the open image, or -1 if no further
           images can be opened.  Note that the frame index advances only
           once every two calls to this function, if the video is interlaced
           and the field mode (see `set_field_mode') is 2.  Note also, that
           `seek_to_frame' might be able to re-position the frame pointer
           before opening an image.
      */
    virtual void close_image() = 0;
      /* [SYNOPSIS]
           Each successful call to `open_image' must be bracketed by a call to
           `close_image'.  Does nothing if no image is currently open.
      */
  };

/*****************************************************************************/
/*                      kdu_compressed_video_target                          */
/*****************************************************************************/

class kdu_compressed_video_target :
  public kdu_compressed_target {
  /* [BIND: reference]
     [SYNOPSIS]
     This abstract base class defines core services of interest to
     applications which generate compressed video content.  Itself
     derived from `kdu_compressed_target', implementations of this class
     may be passed to `kdu_codestream::create', for the purpose of generating
     or transcoding individual images in a compressed video sequence.
     [//]
     Kakadu's implementation of the Motion JPEG2000 file format offers
     an appropriately derived class (`mj2_video_target'), which implements the
     interfaces declared here.  For a much simpler implementation, or
     inspiration for implementing your own video target classes, you might
     consider the `kdu_simple_video_target' class.
  */
  public: // Member functions
    virtual void open_image() = 0;
      /* [SYNOPSIS]
           Call this function to initiate the generation of a new image for
           the video sequence.  At the most basic level, video is considered
           to be a sequence of images.  In the case of interlaced video, a
           frame/field structure may be imposed where each frame consists of
           two fields and each field is considered a separate image.  However,
           some compressed video targets might not support interlaced video.
           [//]
           After calling this function, the present object may be passed into
           `kdu_codestream::create' to generate the JPEG2000 code-stream
           representing the open video image.  Once the code-stream has been
           fully generated (usually performed by `kdu_codestream::flush'),
           the image must be closed using `close_image'.  A new video image
           can then be opened.
      */
    virtual void close_image(kdu_codestream codestream) = 0;
      /* [SYNOPSIS]
           Each call to `open_image' must be bracketed by a call to
           `close_image'.  The caller must supply a non-empty `codestream'
           interface, which was used to generate the compressed data for
           the image just being closed.  Its member functions may be used to
           determine dimensional parameters for internal initialization
           and consistency checking.
      */
  };


/* ========================================================================= */
/*                          Simple Video Format                              */
/* ========================================================================= */

#define KDU_SIMPLE_VIDEO_MAGIC ((((kdu_uint32) 'M')<<24) |  \
                                (((kdu_uint32) 'J')<<16) |  \
                                (((kdu_uint32) 'C')<< 8) |  \
                                (((kdu_uint32) '2')<< 0))
#define KDU_SIMPLE_VIDEO_YCC ((kdu_uint32) 1)
#define KDU_SIMPLE_VIDEO_RGB ((kdu_uint32) 2)

/*****************************************************************************/
/*                         kdu_simple_video_source                           */
/*****************************************************************************/

class kdu_simple_video_source :
  public kdu_compressed_video_source {
  /* [BIND: reference] */
  public: // Member functions
    kdu_simple_video_source() { file = NULL; }
    kdu_simple_video_source(const char *fname, kdu_uint32 &flags)
      { file = NULL; open(fname,flags); }
      /* [SYNOPSIS] Convenience constructor, which also calls `open'. */
    ~kdu_simple_video_source() { close(); }
      /* [SYNOPSIS] Automatically calls `close'. */
    bool exists() { return (file != NULL); }
      /* [SYNOPSIS]
           Returns true if there is an open file associated with the object.
      */
    bool operator!() { return (file == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if there is an open file
           associated with the object.
      */
    void open(const char *fname, kdu_uint32 &flags)
      {
      /* [SYNOPSIS]
           Closes any currently open file and attempts to open a new one,
           generating an appropriate error (through `kdu_error') if the
           indicated file cannot be opened.  Sets `flags' to the 
           value of the flags word recovered from the file.  Currently, only
           two flags are defined, KDU_SIMPLE_VIDEO_YCC and
           KDU_SIMPLE_VIDEO_RGB.  If neither is present, the first image
           component is interpreted as a monochrome image for each video
           frame.
         [ARG: fname]
           Relative path name of file to be opened.
      */
        close();
        file = fopen(fname,"rb");
        if (file == NULL)
          { KDU_ERROR(e,0); e <<
              KDU_TXT("Unable to open compressed data file")
              << ", \"" << fname << "\"!";
          }
        kdu_uint32 magic;
        if (!(read_dword(magic) && read_dword(timescale) &&
              read_dword(frame_period) && read_dword(flags) &&
              (magic == KDU_SIMPLE_VIDEO_MAGIC)))
          { KDU_ERROR(e,1); e <<
              KDU_TXT("Input file")
              << ", \"" << fname << "\", " <<
              KDU_TXT("does not appear to have a valid format.");
          }
        file_pos = 16;
        image_open = false;
        completed_frames = 0; frame_instant = 0;
      }
    bool close()
      { /* [SYNOPSIS]
             It is safe to call this function, even if no file has been opened.
             This particular implementation of the `close' function always
             returns true.
        */
        if (file != NULL)
          fclose(file);
        file = NULL;
        return true;
      }
    kdu_uint32 get_timescale() { return timescale; }
      /* [SYNOPSIS]
           See `kdu_compressed_video_source::get_timescale' for an explanation.
      */
    kdu_long get_frame_instant() { return frame_instant; }
      /* [SYNOPSIS]
           See `kdu_compressed_video_source::get_frame_instant' for an
           explanation.
      */
    int open_image()
      { /* [SYNOPSIS]
             See `kdu_compressed_video_source::open_image' for an explanation.
        */
        assert(!image_open);
        kdu_uint32 image_length;
        if (!read_dword(image_length))
          return -1;
        image_open = true;
        file_pos += 4; start_pos = file_pos; lim_pos = start_pos+image_length;
        return completed_frames;
      }
    void close_image()
      { /* [SYNOPSIS]
             See `kdu_compressed_video_source::close_image' for an explanation.
        */
        assert(image_open); image_open = false;
        if (file_pos != lim_pos)
          { kdu_fseek(file,file_pos=lim_pos); }
        completed_frames++; frame_instant += frame_period;
      }
    kdu_long get_frame_period() { return frame_period; }
      /* [SYNOPSIS]
           See `kdu_compressed_video_source::get_frame_period' for an
           explanation.
      */
    int get_capabilities()
      { return KDU_SOURCE_CAP_SEQUENTIAL | KDU_SOURCE_CAP_SEEKABLE; }
      /* [SYNOPSIS]
           The returned capabilities word always includes the flag,
           `KDU_SOURCE_CAP_SEQUENTIAL' and `KDU_SOURCE_CAP_SEEKABLE'.
           See `kdu_compressed_source::get_capabilities'
           for an explanation of capabilities.
      */
    bool seek(kdu_long offset)
      { /* [SYNOPSIS] See `kdu_compressed_source::seek' for an explanation. */
        assert((file != NULL) && image_open);
        file_pos = offset;
        if (file_pos >= lim_pos)
          file_pos = lim_pos-1;
        if (file_pos < start_pos)
          file_pos = start_pos;
        kdu_fseek(file,file_pos);
        return true;
      }
    kdu_long get_pos()
      { /* [SYNOPSIS]
           See `kdu_compressed_source::get_pos' for an explanation. */
        return (file == NULL)?-1:file_pos;
      }
    int read(kdu_byte *buf, int num_bytes)
      { /* [SYNOPSIS] See `kdu_compressed_source::read' for an explanation. */
        assert((file != NULL) && image_open);
        int max_bytes = (int)(lim_pos-file_pos);
        if (num_bytes > max_bytes)
          num_bytes = max_bytes;
        num_bytes = (int) fread(buf,1,(size_t) num_bytes,file);
        file_pos += num_bytes;
        return num_bytes;
      }
  private: // Helper functions
    bool read_dword(kdu_uint32 &val)
      { int byte;
        val = (kdu_uint32) getc(file); val <<= 8;
        val += (kdu_uint32) getc(file); val <<= 8;
        val += (kdu_uint32) getc(file); val <<= 8;
        val += (kdu_uint32)(byte = getc(file));
        return (byte != EOF);
      }
  private: // Data
    FILE *file;
    kdu_uint32 frame_period, timescale;
    kdu_long frame_instant;
    int completed_frames;
    bool image_open; // True if  an image is currently open
    kdu_long file_pos; // Current position in file
    kdu_long start_pos; // Location in file of currently open image
    kdu_long lim_pos; // Location beyond end of currently open image
  };

/*****************************************************************************/
/*                         kdu_simple_video_target                           */
/*****************************************************************************/

class kdu_simple_video_target :
  public kdu_compressed_video_target {
  /* [BIND: reference] */
  public: // Member functions
    kdu_simple_video_target()
      { file=NULL; head=tail=NULL; }
    kdu_simple_video_target(const char *fname, kdu_uint32 timescale,
                            kdu_uint32 frame_period, kdu_uint32 flags)
      { file=NULL; open(fname,timescale,frame_period,flags); }
      /* [SYNOPSIS] Convenience constructor, which also calls `open'. */
    ~kdu_simple_video_target()
      { close();
        while ((tail=head) != NULL)
          { head=tail->next; delete tail; }
      }
    bool exists() { return (file != NULL); }
      /* [SYNOPSIS]
           Returns true if there is an open file associated with the object.
      */
    bool operator!() { return (file == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if there is an open file
           associated with the object.
      */
    void open(const char *fname, kdu_uint32 timescale, kdu_uint32 frame_period,
              kdu_uint32 flags)
      {
      /* [SYNOPSIS]
           Opens the indicated file for writing, generating an error message
           through `kdu_error', it this is not possible.  Writes a 16-byte
           header consisting of 4 integers, in big-endian byte order.  The
           first holds the magic string, "MJC2"; the second holds the time
           scale (clock ticks per second); the third holds a frame period
           (number of clock ticks between frame); and the fourth holds a
           flags word.  Currently, only two flags are defined,
           KDU_SIMPLE_VIDEO_YCC and KDU_SIMPLE_VIDEO_RGB.  If neither flag is
           set, the first component of each video image will be taken to
           represent a monochrome image (and that is all that can be assumed).
      */
        close();
        file = fopen(fname,"wb");
        if (file == NULL)
          { KDU_ERROR(e,2); e <<
              KDU_TXT("Unable to open compressed data file")
              << ", \"" << fname << "\"!";
          }
        write_dword(KDU_SIMPLE_VIDEO_MAGIC);
        write_dword(timescale);
        write_dword(frame_period);
        write_dword(flags);
        image_open=false;
      }
    bool close()
      { /* [SYNOPSIS]
             It is safe to call this function, even if no file has yet been
             opened.  This particular implementation of the `close' function
             always returns true.
        */
        if (file != NULL)
          fclose(file);
        file = NULL;
        return true;
      }
    void open_image()
      { /* [SYNOPSIS]
             See description of `kdu_compressed_video_target::open_image'.
        */
        assert(!image_open);
        image_open = true; image_len = 0; tail = NULL;
      }
    bool write(const kdu_byte *buf, int num_bytes)
      { /* [SYNOPSIS]
             See `kdu_compressed_video_target::write' for an explanation.
        */
        assert(image_open);
        image_len += num_bytes;
        while (num_bytes > 0)
          {
            if (head == NULL)
              head = new kd_stream_store;
            if (tail == NULL)
              { tail = head; tail->remaining += tail->len; tail->len = 0; }
            if (tail->remaining == 0)
              {
                if (tail->next == NULL)
                  tail->next = new kd_stream_store;
                tail = tail->next; tail->remaining += tail->len; tail->len = 0;
              }
            int xfer = (num_bytes<tail->remaining)?num_bytes:(tail->remaining);
            memcpy(tail->buf+tail->len,buf,(size_t) xfer);
            tail->remaining -= xfer; tail->len += xfer;
            num_bytes -= xfer; buf += xfer;
          }
        return true;
      }
    void close_image(kdu_codestream codestream)
      { /* [SYNOPSIS]
             See `kdu_compressed_video_target::close_image' for an explanation.
        */
        assert(image_open);
        write_dword((kdu_uint32) image_len);
        for (tail=head; image_len > 0; image_len -= tail->len, tail=tail->next)
          fwrite(tail->buf,1,(size_t)(tail->len),file);
        image_open = false;
      }
  private: // Helper functions
    void write_dword(kdu_uint32 val)
      {
        putc((kdu_byte)(val>>24),file); putc((kdu_byte)(val>>16),file);
        putc((kdu_byte)(val>> 8),file); putc((kdu_byte)(val>> 0),file);
      }
  // --------------------------------------------------------------------------
  private: // Declarations
      struct kd_stream_store {
          kd_stream_store()
            { len = 0; remaining = 8192; next = NULL; }
          int len, remaining;
          kdu_byte buf[8192];
          kd_stream_store *next;
        };
  // --------------------------------------------------------------------------
  private: // Data
    FILE *file;
    bool image_open;
    int image_len;
    kd_stream_store *head, *tail;
  };

#undef KDU_ERROR
#undef KDU_ERROR_DEV
#undef KDU_WARNING
#undef KDU_WARNING_DEV
#undef KDU_TXT

#endif // KDU_VIDEO_IO_H
