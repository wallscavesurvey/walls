/*****************************************************************************/
// File: compressed_local.h [scope = CORESYS/COMPRESSED]
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
   Local definitions employed by the machinery behind the interfaces defined
in "kdu_compressed.h".  These definitions should not be included from any
other scope.  They are for use only in implementing the compressed data
management machinery.
   As a general rule, we use the prefix "kd_" for internal classes and
structures, for which a "kdu_" prefixed interface class exists in the
common scope header file, "kdu_compressed.h".
******************************************************************************/

#ifndef COMPRESSED_LOCAL_H
#define COMPRESSED_LOCAL_H

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_arch.h"

// The following classes and structures are all defined here:

struct kd_code_buffer;
class kd_buf_server;
class kd_thread_buf_server;

class kd_compressed_output;
class kd_input;
class kd_compressed_input;
class kd_pph_input;

class kd_marker;
class kd_pp_markers;
struct kd_tpart_pointer;
struct kd_tile_ref;
class kd_tlm_generator;
class kd_tpart_pointer_server;
class kd_precinct_pointer_server;

class kd_compressed_stats;
class kd_codestream_comment;
class kd_packet_sequencer;

class kd_header_in;
class kd_header_out;
class kd_block;

class kd_reslength_checker;
struct kd_global_rescomp;
struct kd_comp_info;
struct kd_output_comp_info;
struct kd_mct_ss_model;
struct kd_mct_stage;
struct kd_mct_block;

struct kd_codestream;
struct kd_tile;
struct kd_tile_comp;
struct kd_node;
struct kd_subband;
struct kd_resolution;

struct kd_precinct_band;
struct kd_precinct;
class kd_precinct_size_class;
class kd_precinct_server;
class kd_precinct_ref;

struct kd_thread_block_state;
class kd_thread_env;


/* ========================================================================= */
/*                         Input Exception Codes                             */
/* ========================================================================= */

#define KDU_EXCEPTION_PRECISION ((kdu_uint16) 13)
    /* Thrown when a packet specifies quantities too large to represent
       within the constraints defined by the current implementation.  This
       is almost certainly due to an erroneously constructed code-stream
       and will be treated as such if the resilient mode is enabled. */
#define KDU_EXCEPTION_ILLEGAL_LAYER ((kdu_uint16) 21)
    /* Thrown when the inclusion tag tree attains an illegal state while
       decoding a packet header.  Illegal states can occur only when one
       or more packets from the same precinct have had their empty packet
       bit set to 0 (i.e. the packet was skipped).  Since no code-block can
       contribute to such packets, the index of the layer in which the
       block is first included cannot be equal to the index of a packet marked
       as empty.  If the inclusion tag tree decodes such a layer index, an
       error must have occurred and this exception is thrown. */
#define KDU_EXCEPTION_ILLEGAL_MISSING_MSBS ((kdu_uint16) 74)
    /* Thrown when the inclusion tag tree identifies a number of missing
       MSB's which is larger than 74.  The number of magnitude bit-planes in
       the representation of any subband sample can never exceed 37 in the
       JPEG2000 standard.  This may be augmented by U, the ROI up-shift
       factor; however, there is no practical value in selecting U larger
       than the number of magnitude bit-planes in any subband.  Thus, 74
       is a reasonable upper bound on the number of missing magnitude
       bit-planes which can be signalled -- the bound is mandated in profiles
       1 and 2 and would be mandated in the standard as a whole were it not
       for an oversight. */

  /* Notes:
        The above exceptions may be thrown during packet header decoding.  In
     addition, if an illegal marker code is encountered while reading any
     portion of a packet, an exception of type "kdu_uint16" will also be
     thrown, having the value of the illegal marker code.  Marker exception
     throwing may not occur unless either the "fussy" or the "resilient"
     mode is enabled, since watching for in-packet markers is a little
     time-consuming. */

/* ========================================================================= */
/*                           External Functions                              */
/* ========================================================================= */

extern void
  print_marker_code(kdu_uint16 code, kdu_message &out);

/* ========================================================================= */
/*                        Class/Struct Definitions                           */
/* ========================================================================= */

/*****************************************************************************/
/*                              kd_code_buffer                               */
/*****************************************************************************/

#define KD_CODE_BUFFER_LEN (KDU_CODE_BUFFER_ALIGN-2-(int) sizeof(void *))
    // Note: the codestream generation code currently relies upon the fact
    // that the code-buffer length is even.  This allows us to read/write
    // aligned short integers at the front of the variable length data
    // segment in each code-block, so as to store coding pass slope and
    // length information, while still being able to easily figure out
    // where the code-bytes actually start.  For more on this, see the
    // way in which `kd_block::body_bytes_offset' is first calculated in
    // `kd_block::write_packet_header'.

struct kd_code_buffer { // Chunks code bytes to minimize heap transactions
    kd_code_buffer *next;
    kdu_int16 state; // See below
    kdu_byte buf[KD_CODE_BUFFER_LEN];
  };
  /* Notes:
       This structure is intended to be aligned on a 32- or 64-byte
     boundary, depending on the value of `KD_CODE_BUFFER_ALIGN'.  Moreover, to
     avoid fragmentation of "virtual" cache pages, each buffer is equipped with
     a `state' member, which is used to reorder buffers on the free list
     in such a way as to keep cache pages together, wherever possible.  To
     see how this is achieved, without significant complexity or the
     addition of extra space-wasting pointers to the buffer's memory block,
     we must first describe the interpretation of the `state' member:
        * The least significant 7 bits hold the position of this buffer
          within its virtual cache page.  There are `KD_CODE_BUFFERS_PER_PAGE'
          buffers within each page, so the position index must lie in the
          range 0 to `KD_CODE_BUFFERS_PER_PAGE'-1.  Evidently, then, there
          can be no more than 128 code buffers in a virtual cache page, but
          a practical limit may be much smaller than this.
        * The most significant bit holds one if and only if the buffer
          is allocated; equivalently, it holds 0 if the buffer is free.
        We now describe how the `state' member is used when requesting and
     releasing buffers from the `kd_buf_server' object.  When a buffer is
     requested, the head of the `kd_buf_server' object's free list is
     inspected.  The earliest available buffer within the corresponding
     page is then allocated; if this happens to be the one which is directly
     linked into the free list, the last buffer in the page which is still
     free will be used as the new linking buffer.  This helps to minimize
     transactions with the linked list itself, which tends to reduce access
     to memory which might be shared between multiple threads.  Before
     returning with the newly allocated buffer, we set its most significant
     `state' bit to 1.
       When a buffer is released, we first check whether any buffer in the
     same page is already free; if so, we need only set the MSB of the new
     buffer's `state' member to 0.  Otherwise, we must also enter the
     buffer explicitly on the free list. */

/*****************************************************************************/
/*                               kd_buf_server                               */
/*****************************************************************************/

class kd_buf_server {
  public: // Member functions
    kd_buf_server()
      {
        alloc = NULL; free_head = NULL;
        total_pages = num_allocated_pages = peak_allocated_pages = 0;
        structure_bytes = peak_structure_bytes = 0;
        cache_threshold_bytes = 0; num_users=0;
        attached_thread_buf_servers = NULL;
      }
    ~kd_buf_server();
       /* First call `can_destroy' to see if you can destroy it.  We assume
          that the caller has NOT locked the `KD_THREADLOCK_GENERAL' mutex. */
    bool can_destroy()
      { return (num_users == 0); }
    void attach()
      { num_users++; }
    void detach()
      { assert(num_users > 0); num_users--; }
    kd_code_buffer *get();
      /* Returns a new code buffer, with its `next' pointer set to NULL.
         In multi-threading environments, this function should generally be
         called with the `KD_THREADLOCK_GENERAL' mutex locked. */
    void release(kd_code_buffer *buf);
      /* Recycles a previously acquired code buffer.  In multi-threading
         environments, this function should generally be called with the
         `KD_THREADLOCK_GENERAL' mutex locked. */
    kd_code_buffer *get_page_block(int num_pages, kd_code_buffer * &tail);
      /* Returns an ordered linked list of buffers, representing `num_pages'
         contiguous virtual pages, where each virtual page consists of
         `KD_CODE_BUFFERS_PER_PAGE' buffers.  The idea is that virtual pages
         should not be split across L2 cache lines so that virtual pages can
         be manipulated without encountering problems with cache coherency
         overhead in multi-processor environments.  On return, the value of
         `tail' is set to point to the tail of the list.  In multi-threading
         environments, this function should be called with the
         `KD_THREADLOCK_GENERAL' mutex locked. */
    kdu_long get_current_buf_bytes()
      { /* Returns the current number of bytes allocated by the server for
           bit-stream buffers. */
        return (kdu_long)(num_allocated_pages * KD_CODE_BUFFER_LEN *
                          KDU_CODE_BUFFERS_PER_PAGE);
      }
    kdu_long get_peak_buf_bytes()
      { /* Returns the maximum number of bytes ever actually allocated by
           the server for bit-stream buffers  */
        return (kdu_long)(peak_allocated_pages * KD_CODE_BUFFER_LEN *
                          KDU_CODE_BUFFERS_PER_PAGE);
      }
    kdu_long get_current_structure_bytes()
      { return structure_bytes; }
    kdu_long get_peak_structure_bytes()
      { return peak_structure_bytes; }
    void augment_structure_bytes(kdu_long increment)
      { // Use to adjust the number of structural bytes either up or down
        structure_bytes += increment; assert(structure_bytes >= 0);
        if (structure_bytes > peak_structure_bytes)
          peak_structure_bytes = structure_bytes;
      }
    kdu_long augment_cache_threshold(kdu_long increment)
      { // Use to adjust the cache threshold either up or down
        return (cache_threshold_bytes += increment);
      }
    bool cache_threshold_exceeded()
      { return (cache_threshold_bytes <
                (structure_bytes + num_allocated_pages *
                 (KD_CODE_BUFFER_LEN*KDU_CODE_BUFFERS_PER_PAGE))); }
      /* This function is called from the `kd_precinct_server::get' function
         before it serves up a new precinct container, whenever one or more
         precincts are currently on its inactive list.  The function returns
         true if the cache threshold has been exceeded, meaning that one or
         more inactive precincts should be closed (unloaded from
         memory, along with all their associated code buffers).  For more
         on this, consult the comments appearing below the declaration of
         the `kd_precinct_server' class.
            The function is also called whenever a new tile-part is started,
         to determine whether there are unloadable tiles which should be
         unloaded to bring memory consumption below the cache threshold. */
    void attach_thread_buf_server(kd_thread_buf_server *tbs);
    void detach_thread_buf_server(kd_thread_buf_server *tbs);
      /* The above two functions may be called only with the
         `KD_THREADLOCK_GENERAL' mutex already locked by the caller.  They
         are used only in multi-threading applications. */
  private: // Definitions
      struct kd_code_alloc {
          kd_code_alloc *next;
          kdu_byte block[1]; // Actual structure is allocated to be a much
                             // larger memory block, from which aligned
                             // virtual cache pages are extracted.
        };
  private: // Helper functions
    void alloc_pages();
      /* This function is called when there is insufficient remaining
         storage on the free list. */
  private: // Data
    kd_code_alloc *alloc; // Linked list of allocated memory blocks
    kd_code_buffer *free_head;
    kdu_long total_pages;
    kdu_long num_allocated_pages; // Number of fully allocated pages
    kdu_long peak_allocated_pages;
    kdu_long structure_bytes; // Bytes in tile/comp/subband/precinct structures
    kdu_long peak_structure_bytes;
    kdu_long cache_threshold_bytes;
    int num_users; // Number of users currently sharing the object.
    kd_thread_buf_server *attached_thread_buf_servers;
  };

/*****************************************************************************/
/*                           kd_thread_buf_server                            */
/*****************************************************************************/

#define KD_THREAD_PEAK_STORE_FAC 4

class kd_thread_buf_server {
  public: // Member functions
    void init(kd_thread_env *env) { this->env=env; reset(); }
    void set_codestream_buf_server(kd_buf_server *buf_server);
      /* This function is used to nominate a particular codestream's
         `kd_buf_server' object as the ultimate source of all future
         buffers served by the present object.  If we are already
         attached to a different codestream `buf_server', we must
         first detach, releasing any buffers in our local store to
         the codestream `buf_server' from which they came.  These
         operations are all guarded by the KD_THREADLOCK_GENERAL mutex,
         but the present function is always invoked from a context in
         which the mutex is not locked.  If `buf_server' is NULL, the
         function simply detaches us from any codestream `buf_server'
         to which we might presently be attached. */
    bool is_attached() { return (ultimate_buf_server != NULL); }
      /* Returns true if the last call to `set_codestream_buf_server'
         specified a non-NULL codestream `buf_server'. */
    void detach_while_holding_general_lock()
      { /* Called by `discard_all', so we don't have to unlock and then
           relock the `KD_THREADLOCK_GENERAL' mutex. */
        if (ultimate_buf_server != NULL)
          ultimate_buf_server->detach_thread_buf_server(this);
      }
    kd_code_buffer *get()
      { /* Behaves like `kd_buf_server::get', but allocates initially
           from the thread's local buffer store, getting extra data from the
           codestream specified in the most recent call to
           `set_codestream_buf_server' where required.  This function must
           not be called while holding any lock, since it may need to
           temporarily instantiate a lock to get extra storage from the
           underlying codestream. */
        if (head == NULL) augment_local_store(false);
        kd_code_buffer *result = head;
        if ((head = result->next) == NULL)
          tail = NULL;
        result->next = NULL;
        num_available_buffers--;
        return result;
      }
    void adjust_peak_buffers();
      /* This function is called from within `kd_thread_env::flush' to
         adjust the value of `peak_buffers', which records the maximum
         number of buffers we would like to make available locally
         in this thread buffer server.  The calculation uses the fact that
         `ref_available_buffers'-`num_available_buffers' equals the number
         of buffers which have been obtained via `get', since the last call
         to this function.  This quantity is multiplied by a factor
         `KD_THREAD_PEAK_STORE_FAC' to obtain a reasonable value for
         `peak_buffer', the value of which is adjusted only ever upwards. */
    void augment_local_store(bool general_lock_held);
      /* This function is used to allocate new buffers from the underlying
         codestream, specified in the last call to `set_codestream'.
         If `adjust_peak_buffers' has never been called, the
         function allocates `KD_THREAD_PEAK_STORE_FAC' times the number of
         buffers which have already been served via `get'.  Otherwise, it
         allocates exactly `peak_buffers' new buffers.
            If `general_lock_held' is false, the `KD_THREADLOCK_GENERAL'
         lock is not currently held and must be acquired prior to actually
         allocating any new buffers (and released again before returning,
         of course).  The function is called in this way only when a call
         to `get' finds that the local store of buffers has been
         completely exhausted.
            The function is called with `general_lock_held' equal to true
         from within `kd_thread_env::flush' to take advantage of the fact
         that the general lock is already held for other reasons -- i.e.,
         to piggy-back the activity of refilling the local buffer store
         onto other activities which required the lock. */
  private: // Helper functions
    friend class kd_buf_server;
    void reset()
      { // Called by `kd_buf_server::detach_thread_buf_server' after removing
        // us from its attached thread buffer list.  This function is also
        // used to do most of the work of our own constructor.
        ultimate_buf_server = NULL;
        next_attached = prev_attached = NULL;
        ref_available_buffers = 0;
        peak_buffers = 0;
        num_available_buffers = 0;
        head = tail = NULL;
      }
  private: // Data
    kd_thread_env *env;
    int peak_buffers; // See the `adjust_peak_buffers' function.
    int num_available_buffers; // Number of buffers on the `head' list.
    int ref_available_buffers; // Adjusted so that `ref_available_buffers' -
         // `num_available_buffers' is always equal to the number of buffers
         // allocated via `get' since the last call to `adjust_peak_buffers'.
    kd_code_buffer *head;
    kd_code_buffer *tail;
    kd_buf_server *ultimate_buf_server; // All buffers are allocated from here
    kd_thread_buf_server *next_attached; // Used to build a doubly linked list
    kd_thread_buf_server *prev_attached; // of objects which are all attached
                     // to the same `ultimate_buf_server'.
  };

/*****************************************************************************/
/*                             kd_compressed_output                          */
/*****************************************************************************/

class kd_compressed_output : public kdu_output {
  public: // Member functions
    kd_compressed_output(kdu_compressed_target *target)
      { this->target=target; flushed_bytes = 0; }
    virtual ~kd_compressed_output()
      { flush_buf(); }
    kdu_compressed_target *access_tgt()
      { /* You may call this function to access the underlying `target'
           object, but you should never manipulate that object if you
           have written anything to the present object, without re-accessing
           it. */
           flush_buf();  return target;
      }
    kdu_long get_bytes_written()
      { return (flushed_bytes + (next_buf-buffer)); }
    void flush()
      { flush_buf(); }
  protected: // Virtual functions which implement required services.
    virtual void flush_buf()
      {
        if (next_buf > buffer)
          target->write(buffer,(int)(next_buf-buffer));
        flushed_bytes += next_buf - buffer;
        next_buf = buffer;
      }
  private: // Data
    kdu_compressed_target *target;
    kdu_long flushed_bytes;
  };

/*****************************************************************************/
/*                                kd_input                                   */
/*****************************************************************************/

#define KD_IBUF_SIZE 512
#define KD_IBUF_PUTBACK 6 // Maximum number of bytes you can put back.

class kd_input {
  /* This abstract base class must be derived to construct meaningful
     input devices.  Currently, we have two derived classes in mind: one
     for reading from the code-stream and one for recovering packet
     headers from PPM or PPT markers.  Since there may be many low level
     byte-oriented transactions, we emphasize efficiency for such
     transactions. */
  public: // Member functions
    kd_input()
      { first_unread = first_unwritten = buffer+KD_IBUF_PUTBACK;
        exhausted = fully_buffered = throw_markers = false; }
    virtual ~kd_input() { return; }
    bool is_fully_buffered() { return fully_buffered; }
    void enable_marker_throwing(bool reject_all=false)
      { /* If `reject_all' is false, subsequent reads will throw an exception
           if a bona fide SOP or SOT marker is found. In this case, the
           function will check to make sure that the length field is correct,
           putting the length field and marker code bytes back to the source
           before throwing the exception, so that the catch statement can read
           them again.
              If `reject_all' is true, subsequent reads will throw an
           exception if any marker code in the range FF90 to FFFF is found.
           Again, the marker code itself is put back onto the stream before
           throwing the exception.
              Marker throwing is disabled before throwing an exception for
           either of the above reasons.
              Note that code-stream reading may be slowed down
           when this marker throwing is enabled.  For this reason, the
           capability is best used only when error resilience or error
           detection capabilities are required. */
        this->reject_all = reject_all; throw_markers = true; have_FF = false;
      }
    bool disable_marker_throwing()
      { /* Disable marker exception throwing.  Returns true unless the
           last byte which was read was an FF, in which case the function
           returns false.  This allows the caller to catch markers which
           were partially read.  Any code-stream segment which is not
           permitted to contain a marker code in the range FF90 through FFFF
           is also not permitted to terminate with an FF. */
        if (!throw_markers) return true;
        throw_markers = false; // Must not touch `reject_all' here.
        if (exhausted) have_FF = false;
        return !have_FF;
      }
    void terminate_prematurely()
      { /* This function may be called in non-resilient mode if an EOC marker
           is encountered before the stream physically ends. */
        exhausted = true;
      }
    bool failed()
      { /* Returns true if any of the input functions, `get', `read' or
           `ignore' failed to completely fulfill its objective due to
           exhaustion of the relevant source. */
        return exhausted;
      }
    bool get(kdu_byte &byte) // throws (kdu_uint16 code): unexpected marker
      { /* Access a single byte, returning false if and only if the source is
           exhausted, in which case future calls to `failed' return true. */
        if (exhausted || ((first_unread==first_unwritten) && !load_buf()))
          return false; // If `load_buf' fails it sets `exhausted' for us.
        byte = *(first_unread++);
        if (throw_markers)
          {
            if (have_FF && (byte > 0x8F))
              process_unexpected_marker(byte);
            have_FF = (byte==0xFF);
          }
        return true;
      }
    void putback(kdu_byte byte)
      { /* You may put back more than 1 byte, but you may not call this
           function if a previous read has ever failed.  It is also illegal
           to call this function while marker throwing is enabled. */
        assert(!exhausted);
        assert(!throw_markers);
        first_unread--;
        if (!fully_buffered)
          {
            assert(first_unread >= buffer);
            *first_unread = byte;
          }
      }
    void putback(kdu_uint16 code)
      { /* This function is designed to improve the readability of code
           which puts marker codes back to the input object from which they
           were read. */
        assert(!exhausted);
        assert(!throw_markers);
        first_unread-=2;
        if (!fully_buffered)
          {
            assert(first_unread >= buffer);
            first_unread[0] = (kdu_byte)(code>>8);
            first_unread[1] = (kdu_byte) code;
          }
      }
    int pseudo_read(kdu_byte * &addr, int count)
      { /* This function should be called only for `fully_buffered' sources.
           It behaves like `read', except that it does not actually read
           any bytes.  Instead, it puts the address of the block of data
           that would have been read into the variable referenced by the
           `addr' argument. */
        assert(fully_buffered);
        addr = first_unread;
        if (throw_markers)
          return read(addr,count); // Slower path, looks for markers
        first_unread += count;
        if (first_unread > first_unwritten)
          {
            count -= (int)(first_unread - first_unwritten);
            first_unread = first_unwritten; exhausted = true;
          }
        return count;
      }
    int read(kdu_byte *buf, int count);
      /* More efficient than `get' when the number of bytes to be read is
         more than 2 or 3.  Returns the number of bytes actually read.  If
         less than `count', future calls to `failed' will return true.  Note
         that this function is less efficient if marker throwing is enabled. */
    int read(kd_code_buffer * &cbuf, kdu_byte &buf_pos,
             kd_buf_server *buf_server, int length);
      /* Same as the above function, except that the requested bytes are
         written to a linked list of `kd_code_buffer' buffers, headed by
         `cbuf', where the `buf_pos' indicates the next free byte in the
         `cbuf' buffer.  The function automatically grows the linked list
         of buffers, as required, using `buf_server->get()'.  Upon return,
         the `cbuf' and `buf_pos' variables are advanced to refer to the
         most recently written `kd_code_buffer' object and the already
         written prefix of that object, respectively.  The function
         returns the actual number of bytes transferred in this way, which
         is equal to `length' unless the ultimate source of data ran dry. */
    virtual kdu_long ignore(kdu_long count);
      /* Skips over the indicated number of bytes, returning the number of
         bytes actually skipped.  If less than `count', future calls to
         `failed' will return true.  The function may be overridden by
         derived classes which support seeking. */
  protected: // Data and functions used for buffer management.
    virtual bool load_buf() = 0;
      /* Returns false and sets `exhausted' to true if the source is unable
         to provide any more data.  Otherwise, augments the internal buffer
         and returns true.  Buffer manipulation must conform to the following
         conventions.  The `first_unread' pointer should be set to
         `buffer'+KD_IBUF_PUTBACK and then bytes should be loaded into the
         remaining KD_IBUF_SIZE bytes of the `buffer' array. The
         `first_unwritten' pointer should then be set to point just beyond
         the last byte actually written into the buffer.  The only exception
         to the above conventions is for fully buffered sources, in which
         the entire data source lies in a contiguous external memory buffer.
         In this case, the internal `buffer' is not used; the first call to
         `load_buf' sets `first_unread' to point to the start of the external
         memory buffer and `first_unwritten' to point just beyond the external
         memory buffer.  In this case, the `fully_buffered' flag should be
         set to true, which signals to the `putback' functions that it is
         sufficient for them to adjust the `first_unread' pointer, without
         actually writing anything. */
    kdu_byte buffer[KD_IBUF_SIZE+KD_IBUF_PUTBACK];
    kdu_byte *first_unread; // Points to next byte to retrieve from buffer.
    kdu_byte *first_unwritten; // Points beyond last available byte in buffer
    bool fully_buffered;
    bool exhausted;
    bool throw_markers; // If true, must look for unexpected markers.
  private: // Functions
    void process_unexpected_marker(kdu_byte last_byte);
      /* This function is called when marker throwing is enabled and a
         marker code in the range FF90 through FFFF has been found.  The
         least significant byte of the marker code is supplied as the
         `last_byte' argument.  The function determines whether to throw
         the exception or not.  If not, reading continues unaffected. */
  private: // Data
    bool have_FF; // Valid only with `throw_markers'. Means last byte was FF.
    bool reject_all; // If `false' marker throwing is only for SOP's and SOT's
  };
  
/*****************************************************************************/
/*                             kd_compressed_input                           */
/*****************************************************************************/

class kd_compressed_input : public kd_input {
  public: // Member functions
    kd_compressed_input(kdu_compressed_source *source);
    int get_capabilities()
      { return source->get_capabilities(); }
    bool set_tileheader_scope(int tnum, int num_tiles);
      /* Generates an error through `kdu_error' if the embedded source does
         not support the `KDU_SOURCE_CAP_CACHED' capability.  This function
         simply calls the source's own `set_tileheader_scope' function and
         clears the internal state variables (especially, the `exhausted'
         member) so that subsequent reads will recover bytes from the indicated
         tile's header.  For more information, refer to the comments appearing
         with `kdu_compressed_source::set_tileheader_scope'. */
    kdu_long get_offset()
      { /* Returns the address of the next byte which will be returned by the
           base object's `get' function.  If seeking is enabled, the source
           may be repositioned to this point by supplying this address as the
           `unique_address' argument to `seek'.  Even if seeking is not
           enabled, `get_offset' will return the correct address and this may
           be used to determine appropriate arguments for the `ignore' member
           function declared below.  May not be called if the source is in
           anything other than code-stream scope. */
        assert(!special_scope);
        return cur_offset + last_loaded_bytes - (first_unwritten-first_unread);
      }
    void seek(kdu_long unique_address);
      /* If `unique_address' is non-negative, it is treated as a code-stream
         offset and the function seeks to a point `unique_address' bytes past
         the beginning of the code-stream (SOC marker), using the embedded
         `kdu_compressed_source' object's `seek' function.  In this case,
         a terminal error is generated if the source does not
         offer the KDU_SOURCE_CAP_SEEKABLE capability.
            If `unique_address' is negative, the function treats
         -(1+`unique_address') as the unique identifier associated with a
         cached precinct, supplying this identifier to the embedded
         `kdu_compressed_source' object's `set_precinct_scope' function.  In
         this case, a terminal error is generated if the source does not
         offer the KDU_SOURCE_CAP_CACHEABLE capability.
            In either case, the function may clear or set the "exhausted"
         state, so that the base object's `failed' member function may return
         false, having previously returned true, or vice-versa. */
    kdu_long ignore(kdu_long count); // Overrides `kd_input::ignore'.
    void set_max_bytes(kdu_long limit);
      /* No effect except when reading in code-stream scope. */
    kdu_long get_bytes_read();
      /* Returns the smallest initial prefix of the source stream which
         contains all of the bytes consumed from `kd_input'.  Note that more
         bytes may have been read from the compressed data source and
         buffered internally, without having been actually used.
            For cached sources (those advertising the KDU_SOURCE_CAP_CACHED
         capability), the function's return value will be 0, unless we
         are still reading the main header. */
    kdu_long get_suspended_bytes();
      /* Returns the total number of bytes which have been read in the
         suspended state.  The returned value may be subtracted from
         the value returned by `get_bytes_read' to determine the total
         number of bytes which have been read in a non-suspended state.
         Byte limits apply only to non-suspended reads. */
    void set_suspend(bool state)
      {
        if (special_scope) return;
        if ((suspend_ptr == NULL) && state)
          { // Enter suspended mode
            suspend_ptr = first_unread;
            if (alt_first_unwritten != NULL)
              {
                assert(alt_first_unwritten > first_unwritten);
                last_loaded_bytes += alt_first_unwritten - first_unwritten;
                first_unwritten = alt_first_unwritten;
                alt_first_unwritten = NULL;
              }
          }
        else if ((suspend_ptr != NULL) && !state)
          { // Leave suspended mode
            suspended_bytes += first_unread-suspend_ptr;  suspend_ptr = NULL;
            kdu_long limit = suspended_bytes + max_bytes_allowed - cur_offset;
            if (limit < last_loaded_bytes)
              { // We have already loaded the buffer with too many bytes
                alt_first_unwritten = first_unwritten;
                first_unwritten -= (last_loaded_bytes - limit);
                last_loaded_bytes = limit;
                if (first_unwritten < first_unread)
                  { // We have already read past the end.
                    exhausted = true;
                    suspended_bytes -= (first_unread-first_unwritten);
                    first_unwritten = first_unread;
                    alt_first_unwritten = NULL;
                  }
              }
          }
      }
  protected: // Virtual functions which implement required services.
    virtual bool load_buf();
  private: // Data
    kdu_compressed_source *source;
    kdu_long cur_offset; // Offset from start of code-stream
    kdu_long max_bytes_allowed; // Source may have fewer bytes than this
    kdu_long max_address_read; // Address of last byte read from base object.
    kdu_long suspended_bytes; // Bytes read while in suspended mode.
    kdu_long last_loaded_bytes; // Num bytes added by last call to `load_buf'
    kdu_byte *suspend_ptr; // Points into the buffer
    kdu_byte *alt_first_unwritten; // See below
    bool special_scope; // True for anything other than code-stream scope.
  };
  /* Notes:
        The `cur_offset' member holds the offset from the start of the
     code-stream to the first nominal byte in the input buffer; i.e., the
     byte at kd_input::buffer + KD_IBUF_PUTBACK.  The actual number of bytes
     which have been read from `source' is larger by the amount
     `kd_input::first_unwritten' - (`kd_input::buffer'+KD_IBUF_PUTBACK).
        The object may be placed in a suspended mode while reading data
     which belongs to code-stream packets which lie outside the current
     resolution, components or layers of interest.  This allows the
     application to set a byte limit which applies only to the relevant
     data which would be left if the irrelevant data were first parsed
     out of the code-stream.  The object is in suspended mode if and only
     if `suspend_ptr' is non-NULL.  In this case, `suspend_ptr' holds the
     value of `kd_input::first_unread' immediately prior to the point at
     which the object entered the suspended mode.  The total number of
     bytes consumed while in the suspended mode is maintained by the
     `suspended_bytes' member, but use the function, `get_suspended_bytes'
     to evaluate a reliable count of the number of suspended bytes, which
     takes into account activity within the `kd_input' object of which
     the derived object might not be aware.
        The `alt_first_unwritten' member is used to keep track of bytes in
     the buffer which may be inaccessible while in the regular (non-suspended)
     mode, which may need to be made accessible while the object is in the
     suspended mode.  This member holds NULL until exiting the suspended mode
     requires `first_unwritten' to be truncated to account for a byte limit.
     At that point, it is set to the value of the `first_unwritten' member
     prior to truncation.  When the object enters the suspended mode, any
     non-NULL `alt_first_unwritten' value is copied to `first_unwritten'. */

/*****************************************************************************/
/*                               kd_pph_input                                */
/*****************************************************************************/

class kd_pph_input : public kd_input {
  /* Alternate input for packet header bytes, which is used in conjunction
     with the PPM and PPT marker segments. */
  public: // Member functions
    kd_pph_input(kd_buf_server *server)
      { buf_server = server; first_buf = read_buf = write_buf = NULL; }
    virtual ~kd_pph_input();
    void add_bytes(kdu_byte *data, int num_bytes);
      /* This function is called when unpacking a PPM or PPT marker, to
         augment the current object. */
  protected: // Virtual functions which implement required services.
    virtual bool load_buf();
  private: // Data
    kd_code_buffer *first_buf;
    kd_code_buffer *read_buf, *write_buf;
    int read_pos, write_pos;
    kd_buf_server *buf_server;
  };
  /* Notes:
        `first_buf' points to the first in a linked list of buffers used to
     temporarily store PPM or PPT marker segment bytes.
        `read_buf' points to the element in the linked list of code buffers
     which is currently being read; `read_pos' points to the next element of
     the current buffer which will be read. This may be equal to
     KD_CODE_BUFFER_LEN, in which case the next element in the code buffer
     list must be accessed when the next read attempt occurs.
        `write_buf' points to the last element in the linked list of code
     buffers and `write_pos' identifies the first unwritten byte in this
     element.  `write_pos' may equal KD_CODE_BUFFER_LEN, in which case a
     new element will need to be added before further data can be written.
        `buf_server' points to the object which is used to add and release
     code buffer elements.  This is a shared resource. */

/*****************************************************************************/
/*                                kd_marker                                  */
/*****************************************************************************/

class kd_marker {
  /* Objects of this class are used to read and store code-stream marker
     segments. The most efficient way to use the class is to construct a
     single serving object which is used to read the markers one by one and
     then copy any markers which need to be preserved into new instances of
     the class, using the copy constructor.  This concentrates wasted buffer
     space in the one server marker which actually reads from the input
     code-stream. */
  public: // Member functions
    kd_marker(kd_input *input, kd_codestream *cs)
      { this->source = input; this->codestream = cs;
        code = 0; length = 0; max_length = 0; buf = NULL;
        encountered_skip_code = false; }
    kd_marker(const kd_marker &orig);
      /* Copies any marker segment stored in `orig'. Note that the `source'
         pointer is not copied, meaning that the `read' member function may
         not be invoked on the copy. */
    ~kd_marker() { if (buf != NULL) delete[] buf; }
    kd_marker *move(kd_input *input, kd_codestream *cs)
      { this->source = input; this->codestream = cs; return this; }
        /* This function is used when a `kd_marker' object must be moved
         to a different codestream, or to work with a different source of
         data. */
    void print_current_code(kdu_message &out)
      { /* Prints a text string identifying the current marker code. */
        print_marker_code(this->code,out);
      }
    bool read(bool exclude_stuff_bytes=false, bool skip_to_marker=false);
      /* Reads a new marker (or marker segment) from the `kd_input' object
         supplied during construction.
             Returns true if successful.  Causes of failure may be: 1) the
         code-stream source is exhausted; 2) a valid marker code was not found;
         or 3) an SOP or SOT marker code was found, but followed by an invalid
         length field.  This third cause of failure is important since it
         prevents the function from attempting to consume an invalid SOP or
         SOT marker segment, in the process of which it might consume any
         number of useful packets, having their own SOP markers.
              A valid marker code must commence with an FF.  If
         `exclude_stuff_bytes' is true then the second byte of a valid marker
         code must be strictly greater than 0x8F.  Otherwise, the second byte
         is arbitrary.  If `skip_to_marker' is true, the function consumes
         bytes indefinitely, until the source is exhausted or a valid marker
         code is found.  In this case, the function returns false only if the
         source is exhausted. Otherwise, the function expects to find a valid
         marker code immediately.
             As a convenience feature, the function automatically skips over
         any EOC marker it encounters.  This has two advantages: 1) we can
         detect the end of the code-stream without having to explicitly check
         for an EOC marker; 2) we should not be overly troubled by EOC
         markers which arise due to corruption of the code-stream (this has
         quite a high likelihood of occurring if the code-stream is subject
         to corruption).
             In the event that the source is not exhausted but a valid
         marker code is not found, the function puts back any bytes it consumed
         before returning false.  This allows the caller to continue reading
         from the point where the function was first called. */
    kdu_uint16 get_code() { return code; }
      /* Returns 0 if no actual marker is available yet. */
    int get_length() { return length; }
      /* Returns length of marker segment (not including the length specifier).
         Returns 0 if no actual marker available, or if the marker is a
         delimiter (no segment). */
    kdu_byte *get_bytes() { return buf; }
      /* Returns pointer to marker segment bytes immediately following the
         length specifier. */
    void clear() { code = 0; length = 0; encountered_skip_code = false; }
      /* Erase internal state so that not marker is advertised.  This is
         useful if the marker has already been inspected and found to be
         not useful. */
  private: // Data
    kd_input *source; // If NULL, `read' may not be used.
    kd_codestream *codestream;
    kdu_uint16 code;
    int length;
    int max_length; // Number of bytes which the buffer can store.
    kdu_byte *buf;
    bool encountered_skip_code;
  };

/*****************************************************************************/
/*                                 kd_pp_markers                             */
/*****************************************************************************/

class kd_pp_markers {
  public: // Member functions
    kd_pp_markers()
      { list = NULL; }
    ~kd_pp_markers();
    void add_marker(kd_marker &copy_source); 
      /* Copies the `kd_marker' object into a new `kd_pp_marker_list'
         element, inserting it into the evolving list on the basis of its
         Zppm or Zppt index.  Markers need not be added in order. */
    void transfer_tpart(kd_pph_input *pph_input);
      /* Transfers an entire tile-part of packed packet header data from
         the internal marker list to the `pph_input' object, which may
         then be used as a source object for packet header reading.  If the
         object is managing a list of PPM markers, the function expects to
         find the total number of bytes associated with the next tile-part
         embedded in the current marker segment.  Otherwise, all marker
         segments are dumped into the `pph_input' object.  Marker segments
         whose data have been consumed are deleted automatically. */
    void ignore_tpart();
      /* Same as `transfer_tpart' except that the data are simply discarded.
         This is useful when discarding a tile for which PPM marker information
         has been provided. */
  private: // Definitions

      class kd_pp_marker_list : public kd_marker {
        public: // Member functions
          kd_pp_marker_list(kd_marker &copy_source) : kd_marker(copy_source)
            { next = NULL; }
        public: // Data
          kd_pp_marker_list *next;
          int znum;
          int bytes_read;
        };

  private: // Convenience functions
    void advance_list();
  private: // Data
    bool is_ppm; // Otherwise, list contains PPT markers.
    kd_pp_marker_list *list;
  };

/*****************************************************************************/
/*                            kd_tpart_pointer                               */
/*****************************************************************************/

struct kd_tpart_pointer {
    kdu_long address; // Offset relative to the start of the code-stream.
    kd_tpart_pointer *next; // For building linked lists.
  };

/*****************************************************************************/
/*                               kd_tile_ref                                 */
/*****************************************************************************/

struct kd_tile_ref {
    kd_tpart_pointer *tpart_head;
    kd_tpart_pointer *tpart_tail;
    kd_tile *tile;
  };
  /* Notes:
       This structure is used to build an array of references to each tile
       in the code-stream.  The array is allocated when the codestream is
       constructed, but the actual `kd_tile' objects are only instantiated
       when needed and destroyed as soon as possible.
          The `tpart_head' and `tpart_tail' members manage a list of
       addresses to tile-parts of the tile.  The list may be constructed
       by reading TLM marker segments in the main code-stream header, or it
       may be constructed dynamically as tile-parts are encountered within
       the code-stream, enabling them to be re-opened at a later point if
       the compressed data-source supports seeking.  The list should be
       empty if the source does not support seeking.
          Once we are sure that all tile-part addresses have been found
       for the tile, the `tpart_tail' member is set to NULL.
          Once a tile has been opened and destroyed in non-persistent mode,
       the `tile' pointer value is set to KD_EXPIRED_TILE, marking it
       as unusable. */

/*****************************************************************************/
/*                              kd_tlm_generator                             */
/*****************************************************************************/

class kd_tlm_generator {
  public: // Member functions
    kd_tlm_generator()
      {
        num_tiles = max_tparts = num_elts = 0; elts = NULL;
        tile_data_bytes=0;
      }
    ~kd_tlm_generator() { if (elts != NULL) delete[] elts; }
    bool init(int num_tiles, int max_tparts, int tnum_bytes, int tplen_bytes);
      /* Call this function after construction or `clear' in order to
         initialize the object in preparation for generating TLM marker
         segments.
            If the number of tiles or tile-parts identified here
         cannot be accommodated by a legal set of TLM marker segments,
         having the indicated precisions for tile-part numbers (`tnum_bytes')
         and tile-part lengths (`tplen_bytes'), the function returns false
         and `exists' will continue to return false.
            Otherwise, the object is initialized and subsequent calls to
         `exists' will return true.  Note that the number of tiles given
         here is the total number in the entire code-stream.  This might
         be more than the number of tiles being compressed in the
         current fragment, in which case the `elts' array will not be
         full by the time `write_tlms' is called. */
    bool exists() { return (num_tiles > 0); }
    bool operator!() { return (num_tiles == 0); }
    void clear()
      {
        num_tiles = max_tparts = num_elts = tnum_prec = tplen_prec = 0;
        record_bytes = 0; tile_data_bytes = 0;
        if (elts != NULL)
          { delete[] elts; elts = NULL; }
      }
    int get_max_tparts() { return max_tparts; }
      /* Returns the parameter recorded with the `ORGgen_tlm' parameter
         attribute, which triggered the creation of this object.  The
         value returned here is guaranteed to lie in the range 1 to 255. */
    int get_tlm_bytes() { return tlm_bytes; }
      /* Returns the total number of bytes occupied by TLM marker segments.
         This value is computed immediately by the object's constructor; it
         does not depend on the actual tile-part length values added later
         via `add_tpart_length'. */
    void write_dummy_tlms(kd_compressed_output *out);
      /* Writes a properly sized set of TLM marker segments to the `out'
         object, except that all tile-part lengths are set equal to 0.  It
         is most important that the TLM marker segments appear at the very
         end of the main codestream header, since a pointer to the start
         of the TLM data will later be generated on the basis of the
         amount of tile-data which has been written, plus the size of the
         TLM data. */
    void add_tpart_length(int tnum, kdu_long length);
      /* Call this function as the tile-part length information becomes
         available while generating the code-stream.  The actual marker
         TLM marker segments can be written only once all tile-part lengths
         have been added. */
    void write_tlms(kdu_compressed_target *tgt,
                    int prev_tiles_written, kdu_long prev_tile_bytes_written);
      /* This function writes the final TLM marker segments.  The `tgt'
         object's `start_rewrite' and `end_rewrite' functions are used to
         reposition the output stream over the previously written dummy
         TLM marker segments and rewrite them with valid tile-part length
         values.  The `prev_tiles_written' and `prev_tile_bytes_written'
         arguments are identical to the values originally supplied to
         `kdu_codestream::create'.  They are used to determine the
         location of the TLM marker information which must be written,
         since some of it may have previously been written when compressing
         different codestream fragments. */
  private: // Structures
      struct kd_tlm_elt {
          kdu_uint16 tnum;
          kdu_uint32 length;
        };
  private: // Data
    int num_tiles;
    int max_tparts;
    int tnum_prec; // Precision (number of bytes) used for tile numbers
    int tplen_prec; // Precision (number of bytes) used for tile-part lengths
    int record_bytes; // Bytes for each tile-part record in TLM marker segments
    int tlm_bytes; // Total number of bytes occupied by TLM marker segments
    int num_elts; // num_tiles * max_tparts
    int elt_ctr; // Pointer to next unfilled entry in `elts'
    kdu_long tile_data_bytes; // sum of all the tile-part lengths added so far
    kd_tlm_elt *elts; // One element for each tile-part, in sequence.
  };

/*****************************************************************************/
/*                         kd_tpart_pointer_server                           */
/*****************************************************************************/

class kd_tpart_pointer_server {
  public: // Member functions
    kd_tpart_pointer_server()
      {
        groups = NULL; free_list = NULL; tlm_markers = NULL;
        translated_tlm_markers = false;
      }
    ~kd_tpart_pointer_server();
    bool using_tlm_info() { return translated_tlm_markers; }
      /* Returns true if TLM data was used to construct tile-part
         address lists in the most recent call to `translate_markers'. */
    void add_tlm_marker(kd_marker &marker);
      /* Call this function whenever a TLM marker segment is encountered,
         while parsing the main header. */
    void translate_markers(kdu_long first_sot_address,
                           int num_tiles, kd_tile_ref *tile_refs);
      /* Call this function when the main header has been completely read,
         passing in the seek position associated with the first byte of
         the first SOT marker in the code-stream.  This is the location of
         the SOT marker, relative to the start of the code-stream.  It is
         used to convert tile-part lengths into absolute addresses for
         each tile-part.  If no TLM marker segments have been read, the
         function does nothing.  Otherwise, it fills in the tile-part
         address list in each of the `num_tiles' entries of the
         `tile_refs' array.  If the available TLM information is
         insufficient to fully initialize the addresses of all tile-parts
         of each tile, the function issues a warning (since is is strictly
         illegal to have only partial TLM information). */
    void add_tpart(kd_tile_ref *tile_ref, kdu_long sot_address);
      /* Call this function when a new tile-part is encountered in a
         persistent seekable compressed data source, where the tile-part
         address was not already in the list of known tile-part addresses
         for the tile.  This allows the tile to be loaded or reloaded at
         a later time without reparsing the entire code-stream. */
  private: // Definitions

#     define KD_POINTER_GROUP_SIZE 32
      struct kd_pointer_group {
          kd_tpart_pointer elements[KD_POINTER_GROUP_SIZE];
          kd_pointer_group *next;
        };
        /* The purpose of this structure is to avoid an unnecessarily large
           number of calls to `new' and `delete'. */

      class kd_tlm_marker_list : public kd_marker {
        public: // Member functions
          kd_tlm_marker_list(kd_marker &copy_source) : kd_marker(copy_source)
            { next = NULL; }
        public: // Data
          kd_tlm_marker_list *next;
          int znum; // TLM sequence number used for concatenating info.
        };
        /* Objects of this class are used to store TLM marker segments in
           a list, so that they can be correctly ordered and their contents
           extracted. */

  private: // Data
    kd_tlm_marker_list *tlm_markers; // List of TLM marker segments.
    kd_pointer_group *groups; // List of pointer record groups allocated.
    kd_tpart_pointer *free_list; // List of empty pointer records not yet used
    bool translated_tlm_markers;
  };
  /* Notes:
       This object serves two roles: 1) it collects, resequences and
       translates TLM marker segments from the code-stream's main header;
       and 2) it manages the allocation of tile-part address lists, so as
       to avoid excessive memory fragmentation.  The storage for these
       lists is deallocated only when the present object is destroyed. */

/*****************************************************************************/
/*                          kd_precinct_pointer_server                       */
/*****************************************************************************/

class kd_precinct_pointer_server {
  public: // Member functions
    kd_precinct_pointer_server()
      { buf_server = NULL; }
    ~kd_precinct_pointer_server()
      { disable(); }
    void restart()
      { disable(); } // Called if the code-stream is restarted.
    bool is_active()
      { return (buf_server != NULL); }
    void initialize(kd_buf_server *buf_server)
      /* Call this function when constructing the tile to which the
         object is attached, unless the code-stream is being read from
         a non-seekable source, in which case you should leave the object
         uninitialized and none of the other functions will have any effect. */
      { assert(this->buf_server == NULL); this->buf_server = buf_server;
        head = tail = NULL; head_pos = tail_pos = available_addresses = 0;
        next_address = 0; tpart_bytes_left = 0; num_layers = 0;
        something_served = final_tpart_with_unknown_length = false;
        next_znum = 0; packets_left_in_precinct=0; }
    void add_plt_marker(kd_marker &marker, kdu_params *cod_params,
                        kdu_params *poc_params);
      /* Call this function whenever a PLT marker segment is encountered,
         while parsing a tile-part header.  The `cod_params' and `poc_params'
         arguments must point to the relevant tile's COD and POC parameter
         objects.  The function uses these to determine whether or not the
         pointer information can be used.  In particular, it is used only
         if the current progression order sequences all packets of any
         given precinct together and if the POC object indicates that there
         will be no progression order changes.  These conditions are checked
         again after each tile-part header has been loaded (see the
         `start_tpart_body' member function).  The reason for the conditions
         is that at most one seek address can be maintained for any given
         precinct. */
    void start_tpart_body(kdu_long start_address, kdu_uint32 tpart_body_length,
                          kdu_params *cod_params, kdu_params *poc_params,
                          bool packed_headers,
                          bool final_tpart_with_unknown_length);
      /* This function must be called before addresses can be recovered for
         precincts whose packets appear in the current tile-part.  The
         `start_address' and `tpart_body_length' arguments identify the
         absolute seeking address of the first byte in the body of the
         tile-part and the total number of body bytes in the tile-part.
         These will be used to determine which packets belong to the
         tile-part.  If this is the last tile-part of the tile and its
         length is unknown, the `final_tpart_with_unknown_length' argument
         should be true, and `tpart_body_length' is ignored.
            This function may determine that precinct address information
         cannot be served up for the current tile, for one of a number of
         This is generally because the packets of each precinct are found
         not to be contiguous within the tile-part, or because the presence
         of POC information renders this a likely problem.  If the
         number of quality layers is found to have changed from the point
         when packet length information was first converted into precinct
         lengths and addresses, we must also conclude that address information
         is not available.  A further reason for discarding packet length
         information is if packet headers have been packed into marker
         segments, since this requires that the packets be processed in order
         and once one by one.  If any such determination is made before any
         precinct addresses have been served up, the object is simply
         disabled so that calls to `get_precinct_address' will return 0.
         Otherwise, a terminal error is generated.  In this event, the
         code-stream might need to be opened again with seeking disabled
         so that the present object will never be initialized. */
    kdu_long get_precinct_address()
      { return ((buf_server==NULL)?((kdu_long) 0):pop_address()); }
      /* Returns the address of the next precinct (in the sequence determined
         by the current progression order) in the tile.  This address may be
         used to seek to the relevant location when reading the precinct's
         packets.
            The function returns 0 if seeking is not possible, if
         packet length information cannot be found in the code-stream,
         or if packet length information is not usable for one of a number
         of reasons.  As explained above, packet length information is usable
         only if the packets of a precinct all appear consecutively so that
         each precinct is characterized by only one seeking address.
            The function returns -1 if seeking is possible, but the first
         packet of the precinct belongs to a tile-part which has not yet been
         opened.  In this event, the `kd_tile::read_tile_part_header' member
         function will have to be called until the information becomes
         available. */
  private: // Helper functions
    kdu_long pop_address();
      /* Does all the work of `get_precinct_address'. */
    void disable()
      { // Call to discard contents and cease recording new PLT info.
        if (buf_server == NULL) return;
        while ((tail=head) != NULL)
          { head = tail->next; buf_server->release(tail); }
        buf_server = NULL;
      }
    void initialize_recording()
      { // Call before parsing the first PLT marker segment.
        assert((buf_server != NULL) && (head == NULL));
        head = tail = buf_server->get();
      }
    void record_byte(kdu_byte val)
      { // Call to record a byte of encoded precinct length info.
        assert(tail != NULL);
        if (tail_pos == KD_CODE_BUFFER_LEN)
          { tail=tail->next=buf_server->get(); tail_pos = 0; }
        tail->buf[tail_pos++] = val;
      }
    kdu_byte retrieve_byte()
      { // Call to retrieve a byte of encoded precinct length info.
        assert((head != tail) || (head_pos < tail_pos));
        if (head_pos == KD_CODE_BUFFER_LEN)
          { kd_code_buffer *tmp = head; head=head->next; head_pos = 0;
            buf_server->release(tmp); }
        return head->buf[head_pos++];
      }
  private: // Data
    kd_buf_server *buf_server;
    kd_code_buffer *head, *tail;
    int head_pos; // First valid byte in the `head' buffer (for retrieving)
    int tail_pos; // First unused byte in the `tail' buffer (for recording)
    int available_addresses; // Precinct addresses recorded, but not retrieved
    kdu_long next_address; // Address of the next precinct in current tile-part
    kdu_uint32 tpart_bytes_left; // Bytes left in the current tile-part
    bool final_tpart_with_unknown_length; // If true, previous member ignored
    int num_layers; // Packets/precinct assumption used to parse PLT info
    bool something_served; // False until the first valid address is served
    kdu_byte next_znum; // PLT markers must appear in sequence
    kdu_long precinct_length; // Accumulator for lengths of packets
    int packets_left_in_precinct;
  };

/*****************************************************************************/
/*                             kd_compressed_stats                           */
/*****************************************************************************/

class kd_compressed_stats {
  /* An object of this class is used to monitor statistics of the compression
     process.  One application of these statistics is the provision of feedback
     to the block encoder concerning a conservative lower bound to the
     distortion-length slope threshold which will be found by the PCRD-opt
     rate allocation algorithm when the image has been fully compressed.  This
     allows the encoder to skip coding passes which are almost certain to be
     discarded.
        The current very simple implementation makes the naive assumption
     that the compressibility of all subband samples is the same, regardless
     of the subband or resolution level.  This is OK if the number of samples
     which have been processed for each subband is proportional to the
     size of the subband, since then the average compressibility is a good
     measure to use in predicting rate control properties.  In practice,
     though, delay through the wavelet transform means that the percentage
     of samples seen for higher resolution subbands is generally higher than
     that seen for lower frequency subbands, until everything has been
     compressed. Since the higher frequency subbands are almost invariably
     more compressible, the predicted compressibility of the source is
     estimated too high and so the predicted rate-distortion slope threshold
     passes considerably more bits than the final rate allocation will.  This
     renders the prediction excessively conservative for most images.  A
     good fix for this would be to keep track of the percentage of subband
     samples which have been compressed in each resolution level and use this
     information to form a more reliable predictor.  This improvement might
     be included in a future version. */
  public: // Member functions
    kd_compressed_stats() { init(); }
    kd_compressed_stats(kdu_long total_samples, kdu_long target_bytes,
                        bool enable_trimming)
      {
        init();
        this->total_samples = total_samples;
        next_trim = (total_samples+7) >> 3;
        conservative_extra_samples = 4096 + (total_samples>>4);
        target_rate =
          (total_samples==0)?1.0:(((double) target_bytes)/total_samples);
        this->trimming_enabled = enable_trimming;
      }
    void init()
      {
        total_samples=num_coded_samples=conservative_extra_samples=next_trim=0;
        target_rate = 0.0;
        min_quant_slope = 4095; max_quant_slope = 0;
        block_slope_threshold = remaining_slope_threshold = 0;
        for (int n=0; n < 4096; n++)
          quant_slope_rates[n] = 0;
        trimming_enabled = false;
      }
    bool is_empty() { return (num_coded_samples == 0); }
      /* Returns true if some information has been entered via the
         `update_stats' function. */
    bool update_stats(kdu_block *block)
      { /* Invoked by "kdu_subband::close_block".  If the function returns
           true, it is recommended that the compressed data be trimmed back
           to a size consistent with the target compressed length at this
           point.  Remember to invoke `update_quant_slope_thresholds' once
           this function returns. */
        num_coded_samples += block->size.x*block->size.y;
        int quant_slope, length = 0;
        for (int n=0; n < block->num_passes; n++)
          {
            length += block->pass_lengths[n];
            if (block->pass_slopes[n] == 0)
              continue;
            quant_slope = block->pass_slopes[n] >> 4;
            if (quant_slope < min_quant_slope) min_quant_slope = quant_slope;
            if (quant_slope > max_quant_slope) max_quant_slope = quant_slope;
            quant_slope_rates[quant_slope] += length;
            length = 0;
          }
        if (trimming_enabled && (num_coded_samples > next_trim))
          { next_trim += (total_samples+7)>>4; return true; }
        return false;
      }
    bool update_stats(kd_compressed_stats &src)
      { /* Similar to the first version of the `update_stats' function, except
           that the information is copied from the `src' object, returning it
           to the empty state. */
        num_coded_samples += src.num_coded_samples;  src.num_coded_samples = 0;
        if (src.min_quant_slope < this->min_quant_slope)
          this->min_quant_slope = src.min_quant_slope;
        if (src.max_quant_slope > this->max_quant_slope)
          this->max_quant_slope = src.max_quant_slope;
        for (int n=src.min_quant_slope; n <= src.max_quant_slope; n++)
          { quant_slope_rates[n] += src.quant_slope_rates[n];
            src.quant_slope_rates[n] = 0; }
        src.min_quant_slope = 4095;  src.max_quant_slope = 0;
        if (trimming_enabled && (num_coded_samples > next_trim))
          { next_trim += (total_samples+7)>>4; return true; }
        return false;
      }
    void update_quant_slope_thresholds()
      { /* This function should be called after `update_stats' to ensure that
           the most up-to-date slope threshold is available for return via
           `get_conservative_slope_threshold'.  If desired, multiple calls
           to `update_stats' may precede the call to this function, which
           may improve efficiency.  The reason for computing the slope
           threshold here, rather than in `get_conservative_slope_threshold'
           is that the latter function may be invoked asynchronously by any
           number of threads of execution, whereas the present function is
           always invoked while holding the `KD_THREADLOCK_STATS' mutex
           (in multi-threaded applications) so that other threads cannot
           modify the statistics while this function is in progress. */
        int n;  kdu_long max_bytes, cumulative_bytes;
        // Adjust `block_slope_threshold'
        max_bytes = num_coded_samples + conservative_extra_samples;
        max_bytes = 1 + (kdu_long)(max_bytes * target_rate);
        for (cumulative_bytes=0, n=max_quant_slope; n >= min_quant_slope; n--)
          if ((cumulative_bytes += quant_slope_rates[n]) >= max_bytes)
            break;
        block_slope_threshold = n;
        // Adjust `remaining_slope_threshold'
        max_bytes = total_samples;
        max_bytes = 1 + (kdu_long)(max_bytes * target_rate);
        for (cumulative_bytes=0, n=max_quant_slope; n >= min_quant_slope; n--)
          if ((cumulative_bytes += quant_slope_rates[n]) >= max_bytes)
            break;
        remaining_slope_threshold = n;
      }
    kdu_uint16 get_conservative_slope_threshold(bool assume_all_coded=false)
      { /* The slope threshold generated by PCRD-opt is unlikely to be
           smaller than that returned here -- remember that smaller thresholds
           mean that more compressed data are included in the code-stream.
           If `assume_all_coded' is true, the slope threshold is based on the
           assumption that no bytes will be generated for any further samples.
           Otherwise, the assumption is that future samples will have the
           same average compressibility as those already coded.  Note that
           since compressors tend to output a higher proportion of high
           frequency subband samples than low frequency subband samples at
           any point up to the end of the image, the actual compressibility
           of future samples can be expected to be lower on average, making
           our estimate more conservative. */
        int val =
          (assume_all_coded)?remaining_slope_threshold:block_slope_threshold;
        val = (val<<4)-1;
        return (kdu_uint16)((val>0)?val:1);
      }
  private: // Data
    double target_rate; // Expressed in bytes per sample, not bits per sample
    kdu_long total_samples; // Total number of subband sample in entire image
    kdu_long next_trim; // Num samples at which memory should next be trimmed
    kdu_long conservative_extra_samples; // Add to coded samples to be safe
    kdu_long num_coded_samples; // Number of samples already coded
    kdu_long quant_slope_rates[4096]; // See below
    int min_quant_slope, max_quant_slope;
    int block_slope_threshold; // See below
    int remaining_slope_threshold; // See below
    bool trimming_enabled;
  };
  /* Notes:
        The `quant_slope_rates' array holds the total number of coded
     bytes which belong to each of a set of 4096 quantized distortion-length
     slope bins.  The original distortion-length slopes have a 16-bit unsigned
     logarithmic representation (see discussion of the "pass_slopes" member
     array in the "kdu_block" structure).  The index of the bin to which a
     given slope, lambda, belongs is given by
         bin = floor(lambda/16).
     Rounding down means that 16*bin is the smallest slope consistent with the
     bin so that summing the entries from bin to 4095 yields the maximum number
     of code-block bytes which could be contributed to the compressed image
     if the PCRD-opt algorithm selected a slope threshold of 16*bin.  Note,
     however, that bin 0 represents only those slopes in the range 1 to 15,
     since 0 is not a valid slope threshold.
        The `block_slope_threshold' member essentially gives the current
     conservative slope threshold to be used during block coding -- this
     is the slope we want when calling `get_conservative_slope_threshold'
     with the `assume_all_coded' argument set to false.  More specifically,
     `block_slope_threshold' is the smallest index into the `quant_slope_rates'
     array such that the cumulative number of bytes represented by that
     entry and all higher entries is strictly less than the target rate
     multiplied by the number of samples coded so far (plus some
     conservative extra samples).  The corresponding slope threshold is
     obtained by multiplying this value by 16 and subtracting 1.
        The `remaining_slope_threshold' member plays the same role as
     `block_slope_threshold', except that it is based upon the total number
     of samples in the entire image, rather than just the total number of
     samples coded so far.  In this way, it essentially provides a
     conservative distortion-length slope threshold, which can safely be
     used to trim already generated coding passes from their respective
     code-blocks.  This value is used to generate the return value from
     `get_conservative_slope_threshold' when its `assume_all_coded' argument
     is set to true. */

/*****************************************************************************/
/*                            kd_codestream_comment                          */
/*****************************************************************************/

class kd_codestream_comment {
  public: // Member functions
    kd_codestream_comment()
      { readonly=false; is_text=is_binary=false;
        max_bytes=num_bytes=0; buf=NULL; next=NULL; }
    ~kd_codestream_comment()
      { if (buf != NULL) delete[] buf; }
    void init(int length, kdu_byte *data, bool is_text);
      /* This function is used only when parsing comment markers from a
         code-stream; if `is_text' is true, the `data' buffer contains
         `length' characters, starting from the first byte in the body of the
          comment marker; the array of characters may or may not be terminated
          with a null character -- if not, a null character is automatically
          appended to the internal representation.  This function leaves
          the object in the `read-only' state.
      */
    int write_marker(kdu_output *out, int force_length=0);
      /* If `out' is NULL, the marker segment is ony simulated.  In
         any event, the function returns the total number of bytes consumed
         by the comment marker segment, from the marker code to the
         last segment byte.
            If `force_length' is not 0, the marker segment will be forced
         to occupy an exact number of bytes; this is only appropriate for
         text comments, since they can be padded beyond the end of a null
         terminator.  If necessary, the text body of the marker segment will
         be truncated, or padded to the specified length.  A non-zero
         `force_length' value may not be less than 6, which is the length
         of the comment marker header.
            This function leaves the object in the `read-only' state.
      */
  private: // Data
    friend class kdu_codestream_comment;
    bool readonly; // True if you are not allowed to write to the text buffer
    bool is_text; // If true, `buf' is a null-terminated UTF-8 string
    bool is_binary; // If true, `buf' holds binary data
    int max_bytes; // `buf' can hold this many chars only
    int num_bytes; // Number of bytes in `buf' -- includes null terminator
    kdu_byte *buf; // Holds comment text.
  public: // Links
    kd_codestream_comment *next;
  };

/*****************************************************************************/
/*                             kd_packet_sequencer                           */
/*****************************************************************************/

class kd_packet_sequencer {
  public: // Member functions
    kd_packet_sequencer(kd_tile *tile)
      {
        this->tile = tile;
        init();
      }
    void init();
      /* Called when the containing tile is re-initialized for processing a
         new code-stream with exactly the same structural properties as the
         previous one. */
    void save_state();
      /* Saves all quantities associated with packet sequencing, both within
         this object and in the associated tile and its constituent precincts,
         so that they can be later restored to resume sequencing from where
         we left off. */
    void restore_state();
      /* Restores the state saved by the `save_state' member function. */
    kd_precinct_ref *next_in_sequence(kd_resolution* &res, kdu_coords &idx);
      /* This function returns a pointer to the `kd_precinct_ref' object
         which manages the precinct associated with the next packet in the
         relevant tile's packet sequence (see below for the possibility of
         a NULL return value).  The function also returns a
         pointer to the `kd_resolution' object which manages the returned
         precinct and the horizontal and vertical coordinates of the
         precinct within that resolution.  The precinct itself may be
         opened, if required, by passing `res' and `idx' directly into the
         `kd_precinct_ref::open' function -- see description of that
         function for more information.
            When desequencing packets from an input code-stream, the caller
         should first invoke the returned reference's
         `kd_precinct_ref::is_desequenced' member function to determine
         whether or not the present function has done all of the required
         desequencing for the precinct itself.  This happens if the
         precinct is directly addressable via a seek address recovered from
         the relevant tile's `kd_precinct_pointer_server' object.  If
         `kd_precinct_ref::is_desequenced' returns false, the caller should
         proceed to open the precinct with `kd_precinct_ref::open' and
         parse the next packet using `kd_precinct::desequence_packet'.  This
         has the effect of updating the precinct state so that the next
         call to `next_in_sequence' can move on to the next packet in
         sequence.
            When sequencing packets for an output code-stream, the caller
         should usually open the precinct and then call
         `kd_precinct::write_packet'.  Again, this enables the next call
         to `next_in_sequence' to move on to the next packet in sequence.
            If the caller does not actually take steps to desequence the
         next input packet or to write the next output packet, subsequent
         calls to this function will return exactly with exactly the same
         precinct reference.  This allows the caller to delay its packet
         parsing or generation activities.
             For both input and output codestreams, this function may return
         NULL.   In either case, a NULL return value means that
         no further packets can be sequenced based on the information
         available for the current tile-part.  For output codestreams, a
         NULL return value means that the caller must write another tile-part
         (or simulate such an activity).  After such an activity, the
         next call to this function will have access to the packet sequencing
         specifications in a new POC marker segment (if this were not possible
         the function would have generated a terminal error instead of
         returning NULL).  For input codestreams, a NULL return value means
         that the caller must advance to a new tile-part.  There are two
         possible reasons for this: 1) a new POC marker segment may be
         required to obtain sequencing specifications for the next packet;
         or 2) the `kd_precinct_pointer_server::get_precinct_address' function
         might have returned -1, meaning that precincts are addressable, but
         the first packet of the next precinct belongs to the tile's next
         tile-part.
             The function also returns NULL if all packets have been
         transferred to or from the code-stream. This event may be detected
         by checking the tile's `num_sequenced_packets' field; the value
         of that field is advanced by `kd_precinct::desequence_packet' when
         it is called or from within the present function if the precinct
         is addressable (this is the condition under which the returned
         reference's `kd_precinct_ref::is_desequenced' returns true
         immediately). */
  private: // Member functions
    bool next_progression();
      /* Sets up the next progression (or the very first progression).
         Returns false, if no further progressions are available without
         starting a new tile-part. */
    kd_precinct_ref *next_in_lrcp(kd_resolution* &res, kdu_coords &idx);
    kd_precinct_ref *next_in_rlcp(kd_resolution* &res, kdu_coords &idx);
    kd_precinct_ref *next_in_rpcl(kd_resolution* &res, kdu_coords &idx);
    kd_precinct_ref *next_in_pcrl(kd_resolution* &res, kdu_coords &idx);
    kd_precinct_ref *next_in_cprl(kd_resolution* &res, kdu_coords &idx);
       // These functions do most of the work of `next_in_sequence'.  They
       // return NULL if there are no more packets to sequence within the
       // current progression order.
  private: // Declarations
      struct kd_packet_sequencer_state {
        public: // Current sequencing bounds
          int order; // One of the orders defined in the scope of `cod_params'
          int res_min, comp_min; // Lower bounds (inclusive) for loops
          int layer_lim, res_lim, comp_lim; // Exclusive upper bounds for loops
        public: // Current looping state.
          int layer_idx; // Current layer index
          int comp_idx; // Current component index
          int res_idx; // Current resolution level index
          kdu_coords pos; // Precinct position indices in current tile-comp-res
        public: // Spatial sequencing parameters.
          kdu_coords grid_min; // Common grid start for spatial loops
          kdu_coords grid_inc; // Common grid increments for spatial loops
          kdu_coords grid_loc; // Common grid location for spatial loops
        public: // POC access parameters
          kdu_params *poc; // NULL unless POC-based sequencing
          int next_poc_record; // Index of next order record in current POC
        };
  private: // Data
    kd_tile *tile;
    int max_dwt_levels; // Over all components.
    bool common_grids; // True if all components have power-of-2 sub-sampling.
    kdu_coords grid_lim; // Point just beyond tile on the canvas
    bool state_saved;
    kd_packet_sequencer_state state;
    kd_packet_sequencer_state saved_state; // Used by `save_state' member.
  };

/*****************************************************************************/
/*                          kd_reslength_checker                             */
/*****************************************************************************/

class kd_reslength_checker {
  public: // Member functions
    kd_reslength_checker() { memset(this,0,sizeof(*this)); }
    ~kd_reslength_checker() { if (specs != NULL) delete[] specs; }
    bool init(cod_params *cod);
      /* Initializes the contents of the object based on any `Creslength'
         parameter attribute found within `cod'.  In practice, `cod'
         is the relevant global, tile-specific, component-specific, or
         tile-component specific object retrieved by a call to
         `kdu_params::access_unique' with the appropriate tile and component
         indices; it may well be NULL, in which case the object is initialize
         with no constraints.  The function returns true if and only if
         an appropriate `Creslength' attribute was found. */
    void set_layer(int layer_idx);
      /* Use this function to walk thorugh the quality layers.  The object's
         `max_bytes' and `num_bytes' arrays maintain state for only a current
         quality layer.  If `layer_idx' is non-zero, it must be either the
         same or one more than the value of `layer_idx' which was last
         supplied to the function, so that the `prev_layer_bytes' array
         can be used to correctly initialize the `num_bytes' entries
         internally. */
    bool check_packet(kdu_long packet_bytes, int depth)
      /* Updates internal records of the number of bytes associated with
         the given resolution (highest resolution corresponds to `depth'=0)
         and all higher resolutions (smaller depths).  Checks for violation
         of constraints, returning false if any constraints is violated. */
      {
        assert((depth >= 0) && (depth <= 32));
        if ((current_layer_idx < 0) || (specs == NULL)) return true;
        kdu_long lim;
        for (int d=0; (d <= depth) && ((lim=max_bytes[d]) > 0); d++)
          if ((num_bytes[d] += packet_bytes) > lim) return false;
        return true;
      }
  private: // Member variables
    int num_specs;
    kdu_long *specs;
    int current_layer_idx; // -1 if no call to `set_layer' yet.
    kdu_long max_bytes[33]; // Limit for this resolution
    kdu_long num_bytes[33]; // Accumulated number of bytes; see below
    kdu_long prev_layer_bytes[33];
  };
  /* This object manages a single collection of resolution length constraints,
     as provided via the `Creslength' coding parameter attribute.  There
     are potentially (1+T)*(1+C) of these objects in a codestream, where T
     is the number of tiles and C is the number of image components.  These
     correspond to one global form, C component-specific forms, T
     tile-specific forms and C*T tile-component-specific forms.
        `num_specs' and `specs' together hold the parameter values actually
     extracted from the relevant `Creslength' attribute, via the `init'
     function.  These together represent a collection of limits on the
     number of bytes at one or more resolutions, potentially for more than
     one quality layer.  Quality layers are separated by 0's in the
     `specs' array.
        `max_bytes' holds one limit for each potential resolution, starting
     from the full resolution and working down.  The entries in this array
     are derived from the `specs' array, for the quality layer whose index
     is passed to `set_layer'.
        `num_bytes[d]' holds the cumulative number of bytes for all packets
     whose lengths have been passed to `check_packet' so far, for packets
     at depth d or deeper.  This value is compared with `max_bytes[d]' to
     determine whether the resolution length constraint at depth d has been
     violated.
        `prev_layer_bytes' is used to initialize the `num_bytes' entries
     at the start of each new quality layer, based on the total number of
     bytes accumulated by `check_packet' in the previous layer, if any. */

/*****************************************************************************/
/*                           kd_global_rescomp                               */
/*****************************************************************************/

struct kd_global_rescomp {
  public: // Member functions
    kd_global_rescomp()
      {
        codestream = NULL; depth = comp_idx = 0;
        total_area = area_used_by_tiles = area_covered_by_tiles = 0;
        remaining_area = ready_area = 0;
        ready_fraction = reciprocal_fraction = -1.0;
        first_ready = last_ready = NULL;
      }
    ~kd_global_rescomp() { close_all(); }
    void close_all();
      /* Closes all precincts currently stored on the ready list. */
    void initialize(kd_codestream *codestream, int depth, int comp_idx);
      /* Call this function upon creation or restart of an output
         `kdu_codestream' object.  Releases any existing precincts and
         initializes all counters. */
    void notify_tile_status(kdu_dims tile_dims, bool uses_this_resolution);
      /* Call this function when each tile is opened, to indicate whether or
         not this resolution level exists within each component of that tile.
         This updates internal counters used for incremental rate control and
         slope prediction. */
    void add_ready_precinct(kd_precinct *precinct);
    void close_ready_precinct(kd_precinct *precinct);
      /* Extracts the precinct from the list and releases all of its resources.
         This function is called from `write_packet'. */
  public: // Data
    kd_codestream *codestream;
    int depth, comp_idx;
    kdu_long total_area;
    kdu_long area_used_by_tiles;
    kdu_long area_covered_by_tiles;
    kdu_long remaining_area;
    kd_precinct *first_ready;
    kd_precinct *last_ready;
    kdu_long ready_area;
    double ready_fraction;
    double reciprocal_fraction; // 1/ready_fraction.
  };
  /* Notes:
        `depth' is 0 for the highest resolution level, increasing from there.
        `total_area' is the total area of this component-resolution.
        `area_used_by_tiles' is the area associated with the portion of this
     component-resolution covered by tiles which have already been opened and
     have precincts which contribute to the component-resolution.
        `area_covered_by_tiles' is the area associated with the portion of this
     component-resolution covered by tiles which have already been opened,
     regardless of whether those tiles have precincts which contribute to the
     component-resolution or not.  The ratio between `area_used_by_tiles' and
     `area_covered_by_tiles' is used to estimate the proportion of the
     remaining area which is likely to contribute precincts.
        `remaining_area' is the maximum area for which precincts must still
     be written to the code-stream.  This is equal to `total_area' minus
     `area_unused_by_tiles' minus the total area associated with all precincts
     in this component-resolution for which at least one packet has already
     been written out to the code-stream.
        `ready_area' is the area associated with all precincts in the
     ready list headed by `first_ready'.  The ready precincts have been fully
     generated, but none of their packets have yet been written to the
     code-stream.  Whenever it is done, rate allocation is performed on these
     precincts.
        `ready_fraction' holds a quantity in the range 0 to 1, representing
     the expected fraction of the remaining area which is represented by the
     ready precincts.  A negative value means that this quantity has not
     yet been computed, or it needs to be recomputed.  The value is computed
     from
        F = `ready_area' / A
     where A is given by
        A = (`remaining_area'-T)
          + T * (`area_used_by_tiles'/`area_covered_by_tiles')
     and
        T = `total_area' - `area_covered_by_tiles'
  */

/*****************************************************************************/
/*                              kd_comp_info                                 */
/*****************************************************************************/

struct kd_comp_info {
  public: // Data
    kdu_coords sub_sampling; // Component sub-sampling factors.
    float crg_x, crg_y; // Component registration offsets.
    int precision; // Original component bit-depths.
    bool is_signed; // Signed/unsigned nature of original components
    kdu_byte hor_depth[33];
    kdu_byte vert_depth[33];
    int apparent_idx; // See below
    kd_comp_info *from_apparent; // See below
  };
  /* Notes:
       This structure holds codestream-wide information for a single image
       component.
          The `hor_depth' and `vert_depth' arrays contain information which
       might be affected by DFS marker segments in the codestream's main
       header.  The values of `hor_depth'[d] and `vert_depth'[d] identify
       the total number of horizontal and vertical low-pass filtering and
       downsampling stages which are involved in creating the resolution
       level which is `d' levels below the original full image resolution.
       Neither value may exceed d.  Thus, `hor_depth'[0] and `vert_depth'[0]
       are guaranteed to equal zero -- these entries exist only to make the
       implementation more regular.  Codestreams conforming to JPEG2000
       Part-1 must have `hor_depth'[d] = `vert_depth'[d] = d for all d.
       Part-2 codestreams, however, may have custom downsampling factor
       styles (represented by DFS marker segments in the main header).
          The `apparent_idx' and `from_apparent' members are provided
       because the `kdu_codestream' interface (and its descendants) might
       be limited to exposing only a subset of the actual image components
       which are available.  This subset is selected via
       `kdu_codestream::apply_input_restrictions'.  The `apparent_idx'
       member identifies the index (starting from 0) of the present
       codestream image component, as it appears to the external API.  This
       value will be 0 if the component is not visible across the API.  The
       `from_apparent' member is used to translate apparent component
       indices into true codestream component indices.  Specifically, the
       `kd_codestream::comp_info[c].from_apparent' member points to the
       particular entry in the `kd_codestream::comp_info' array which
       represents the true codestream image component whose apparent index is
       c.  This is NULL if c >= `kd_codestream::num_apparent_components'. */

/*****************************************************************************/
/*                           kd_output_comp_info                             */
/*****************************************************************************/

struct kd_output_comp_info {
  public: // Member functions
    kd_output_comp_info()
      {
        precision=0; is_signed=false; subsampling_ref = NULL;
        apparent_idx=-1; from_apparent=0;
        block=NULL; block_comp_idx=0; apparent_block_comp_idx=0;
        is_of_interest=false; ss_tmp=0.0F;
      }
  public: // Data
    int precision;
    bool is_signed;
    kd_comp_info *subsampling_ref;
    int apparent_idx;
    int from_apparent;
    kd_mct_block *block;
    int block_comp_idx;
    int apparent_block_comp_idx;
    bool is_of_interest; // See below
    float ss_tmp; // Temporary storage used in sensitivity calculations;
                  // see `kd_mct_block::analyze_sensitivity'.
  };
  /* Notes:
        This structure is used to describe the image components produced at
     the output of any given MCT transform (synthesis) stage.  It is
     also used to describe the image components at the output of the
     multi-component transform from a global perspective.  The `is_signed'
     and `precision' members are meaningful only where this record is
     found at the output of the last MCT stage in the inverse multi-component
     transform, or in the global `output_comp_info' array managed by
     `kd_codestream'.
        If the inverse multi-component transform does not produce any data for
     this output component, its `subsampling_ref' member is NULL.  Otherwise
     `subsampling_ref' points to the `kd_comp_info' structure which describes
     any of the codestream image components which is involved in
     reconstructing this output component -- it does not matter which one.
     The purpose of this reference is to facilitate the determination of the
     dimensions and sub-sampling attributes of the relevant output component.
     All codestream image components involved in the reconstruction of
     any given MCT output component must have identical dimensions, so it
     is sufficient to reference only one of them.
        The `apparent_idx' and `from_apparent' members work in exactly the
     same way as their namesakes in `kd_comp_info' to facilitate translation
     between true image component indices and apparent image component
     indices, as presented to the application.  The only difference is
     that `from_apparent' is the index (rather than the address) of the
     true output image component which appears to have the index of the
     current component.  The apparent component indices are influenced by
     calls to the second form of the `kdu_codestream::change_appearance'
     function.
        The last four members are used only when the object is found in
     the `output_comp_info' array of a `kd_mct_stage' object.  In this case,
     `block' points to the MCT transform block which is used to produce this
     component within the relevant stage, while `block_comp_idx' indicates
     which of the output components from that transform stage corresponds
     to the current component.  The `apparent_block_comp_idx' member
     identifies the apparent index of this output component within the
     referenced transform block, which may be smaller than `block_comp_idx'
     if some earlier output components from the transform block are not
     apparent.  This value is guaranteed to lie between 0 and
     `block->num_apparent_outputs'-1.  `is_of_interest' is set to false
     if and only if `apparent_idx' is negative, except in the final
     transform stage, where it is also set to false if the corresponding
     component is not of interest to the application, as identified by
     calls to `kdu_tile::set_components_of_interest'.
        If the object is found in `kd_codestream::output_comp_info',
     the `block' member will be NULL and `block_comp_idx',
     `apparent_block_idx' and `is_of_interest' have no meaning. */

/*****************************************************************************/
/*                            kd_mct_ss_model                                */
/*****************************************************************************/

struct kd_mct_ss_model {
  public: // Member functions
    kd_mct_ss_model()  { ss_vals = ss_handle = NULL; }
    ~kd_mct_ss_model() { if (ss_handle != NULL) delete[] ss_handle; }
  public: // Data
    kdu_int16 range_min;
    kdu_int16 range_len;
    float *ss_vals;
    float *ss_handle;
  };
  /* See the discussion of `kd_mct_block::ss_models' for an explanation of
     this structure. */

/*****************************************************************************/
/*                              kd_mct_block                                 */
/*****************************************************************************/

struct kd_mct_block {
  public: // Member functions
    kd_mct_block() { memset(this,0,sizeof(*this)); }
    ~kd_mct_block()
      {
        if (input_indices != NULL) delete[] input_indices;
        if (inputs_required != NULL) delete[] inputs_required;
        if (output_indices != NULL) delete[] output_indices;
        if (dwt_step_info != NULL) delete[] dwt_step_info;
        if (dwt_coefficients != NULL) delete[] dwt_coefficients;
        if (scratch != NULL) delete[] scratch;
        if (ss_models != NULL) delete[] ss_models;
      }
    void analyze_sensitivity(int which_input, float input_weight,
                             int &min_output_idx, int &max_output_idx,
                             bool restrict_to_interest);
      /* This function plays a central role in the calculation of energy
         weights.  It determines the contribution of the block input
         component identified by `which_input' to the set of stage output
         components for the MCT stage in which the block resides,
         multiplying the contribution by `input_weight' and adding it into
         the `kd_output_comp_info::ss_tmp' members of all affected stage
         outputs.  The `which_input' argument must lie in the range 0 to
         `num_inputs'-1, where `num_inputs' is the number of inputs for
         this block.
            The `min_output_idx' and `max_output_idx' arguments
         play a key role in minimizing the complexity of sensitivity
         calculations for multi-component transforms which involve large
         numbers of image components.  On entry, these identify the range
         of stage output components whose `kd_output_comp_info::ss_tmp'
         members have already been affected by sensitivity calculations
         for other transform blocks in the same stage; if this is the first
         stage, the `min_output_idx' value supplied on entry should be
         strictly greater than the `max_output_idx' value, to indicate that
         the range of affected stage outputs is empty so far.  On exit,
         the `min_output_idx' and `max_output_idx' arguments represent the
         augmented range of affected outputs.  Whenever this range grows,
         the function first initialized the affected `ss_tmp' members to 0,
         so there is no need to explicitly initialize all `ss_tmp' members
         in the `kd_mct_stage::output_comp_info' array.  This can also save
         quite a bit of wasted effort when there are a very large number
         of components.
            If `restrict_to_interest' is true, the sensitivity analysis is
         restricted to include only those output components for which
         `kd_output_comp_info::is_of_interest' is true. */
  private: // Helper functions
    void create_matrix_ss_model();
    void create_rxform_ss_model();
    void create_old_rxform_ss_model();
    void create_dependency_ss_model();
    void create_dwt_ss_model();
  public: // Data
    kd_mct_stage *stage;
    int num_inputs;
    int num_required_inputs;
    int *input_indices;
    bool *inputs_required;
    int num_outputs;
    int num_apparent_outputs;
    int *output_indices;
    kd_mct_ss_model *ss_models;
    bool is_reversible;
    bool is_null_transform;
    kdu_params *offset_params;
    kdu_params *matrix_params;
    kdu_params *old_mat_params; // For compatibility with versions before v6.0
    kdu_params *triang_params;
    int dwt_num_steps, dwt_num_levels, dwt_canvas_origin;
    bool dwt_symmetric, dwt_symmetric_extension;
    kdu_kernel_step_info *dwt_step_info;
    float *dwt_coefficients;
    int dwt_low_synth_min, dwt_low_synth_max;
    int dwt_high_synth_min, dwt_high_synth_max;
    float dwt_synth_gains[2]; // Subband gains applied before synthesis lifting
    float *scratch; // Contains `num_inputs' entries if allocated
  };
  /* Notes:
        This structure manages a single transform block within a stage of the
     multi-component transform.  Most types of transform blocks have an
     identical number of input and output components, but irreversible
     decorrelation transforms can have more or less output components than
     input components.
        The `input_indices' array provides the indices of the stage input
     components which are used by this block.  These must lie in the range
     0 to `stage->num_input_components'-1.  Moreover, if this is not the
     first stage, these same indices may be used to address the
     `stage->prev_stage->output_comp_info' array, to discover how the input
     components are produced.
        The output_indices' array provides the indices of the stage output
     components which are produced by this block.  These indices may be
     used to address the `stage->output_comp_info' array to discover whether
     each output component is apparent or not, amongst other things.
        The `ss_models' array is created and manipulated by calls to the
     `analyze_sensitivity' function.  If non-NULL, the array contains
     `num_inputs' entries, each of which identifies the linearized
     contribution of a unit change in the corresponding input to the
     block's output components.  For reasons of efficiency, each input
     identifies the range of block output indices which it affects, via the
     corresponding `kd_mct_ss_model::range_min' and
     `kd_mct_ss_model::range_len' members.  The contribution of each input
     to the affected block outputs is given by its `kd_mct_ss_model::ss_vals'
     array, with length `kd_mct_ss_model::range_len'.  This array is created
     (set to a non-NULL value) only on demand.  Note also that the
     `kd_mct_ss_model::ss_vals' arrays can be allocated in a variety of
     ways and can be shared between different input models, wherever this
     allows for improved efficiency.  This is achieved by separately
     keeping track of actual allocated memory blocks via the
     `kd_mct_ss_model::ss_handle' members.
        Each transform block notionally has an offset vector, whose
     elements are to be added to the output component samples as the final
     step in the transform process.  If `offset_params' is non-NULL, the
     offsets are derived from its `Mvector' attribute.  Otherwise, the
     offsets are all 0.
        If `matrix_params', `triang_params' and `dwt_step_info' are all NULL,
     the transform block does nothing else.  We refer to blocks of this form
     as "NULL blocks".  NULL blocks map their i'th input component to their
     i'th output component, with the possible addition of an offset from
     `offset_params'.  NULL blocks are useful primarily as a tool for
     discarding certain input components, since only the first
     `num_outputs' components find their way through the block.  They are
     also useful for introducing permutations and offsets.  The JPEG2000
     standard, Part 2, refers to NULL transform blocks in this same sense,
     although the text in the standard is potentially unclear.
        For NULL transform blocks, the `is_reversible' flag will be false.
     For all other types of transform blocks, the `is_reversible' flag plays
     an important role in determining how the transformation is implemented.
        Apart from NULL transform blocks, there are four other types of
     transforms:
     [>>] Irreversible decorrelation transforms are described by a matrix,
          whose coefficients are provided by the `Mmatrix' attribute
          managed by `matrix_params'.  This type of transform is used if
          `matrix_params' is non-NULL and `is_reversible' is false.
     [>>] Reversible decorrelation transforms are described by a matrix
          of SERM factorization coefficients.  In this case, `is_reversible'
          is true and the value of `num_outputs' must equal `num_inputs'.
          Codestreams generated prior to v6.0 will have `old_mat_xforms'
          non-NULL, in which case the corresponding matrix coefficients
          have an organization with `num_outputs' rows and `num_outputs'+1
          columns, where the columns represent successive lifting steps.
          Codestreams generated from v6.0 onwards have `matrix_xforms'
          non-NULL, in which case the matrix coefficients have an organization
          with `num_outputs'+1 rows and `num_outputs' columns, where the
          rows represent successive lifting steps.  This matrix organization
          is transposed in answering calls to `kdu_tile::get_mct_rxform_info'.
     [>>] Reversible or irreversible dependency transform.  In this case,
          the `triang_params' member is non-NULL and its `Mtriang'
          attribute is used to access the transform coefficients.
     [>>] Reversible or irreversible DWT (Discrete Wavelet Transform).  In
          this case, `dwt_step_info' is non-NULL.  The DWT transform kernel
          is described by the `dwt_step_info', `dwt_num_steps',
          `dwt_symmetric', `dwt_symmetric_extension' and `dwt_coefficients'
          members.  The number of DWT levels to be applied is described by
          the `dwt_num_levels' member, and the canvas origin which defines
          the coordinate system for the DWT is given by `dwt_canvas_origin'.
          The regions of support occupied by each of the synthesis filters
          are identified by the `dwt_low_synth_min' through
          `dwt_high_synth_max' members.
     
        All transform coefficients and offsets are represented using the
     normalization conventions of the JPEG2000 marker segments from which
     they are derived.  This convention means that the supplied coefficients
     would produce the correct output image component samples, at their
     declared bit-depths, if applied to codestream image components which
     are also expressed at their declared bit-depths. */

/*****************************************************************************/
/*                              kd_mct_stage                                 */
/*****************************************************************************/

struct kd_mct_stage {
  public : // Static member function
    static void
      create_stages(kd_mct_stage * &result_head,
                    kd_mct_stage * &result_tail,
                    kdu_params *params_root, int tile_num,
                    int num_codestream_components,
                    kd_comp_info *codestream_component_info,
                    int num_output_components,
                    kd_output_comp_info *output_component_info);
      /* This function is used to create a linked list of `kd_mct_stage'
         objects, which describe the inverse multi-component transform for
         a particular tile, identified by the `tile_num' argument.  The list
         of MCT stages is created from the descriptions found in the
         coding parameter sub-system, whose root is given by `params_root'.
         The remaining arguments describe the input and output image
         components associated with the transform.  The contents of these
         arrays are not modified, but they are used to contribute to the
         initialization of the stage contents.  Note that the
         `kd_output_comp_info::ref_subsampling' member of the
         `output_component_info' array is ignored by this function.  The
         caller may later use the corresponding member of the internal
         `output_comp_info' member array to check for dimensional
         consistency with the corresponding global value.
            The head and tail of the created list are both returned via
         `result_head' and `result_tail'.  A number of conditions could
         cause this function to fail through `kdu_error'.  In many
         applications this causes an exception to be thrown, after
         which any partially created list of transform stages should be
         cleaned up.  The easiest way to do this is for the function to
         grow the list inside an object which will persist beyond any
         such exception -- in practice, this is the `kd_codestream' or
         `kd_tile' object, both of which maintain head/tail member
         variables for MCT stage lists.
            If `tile_num' is negative, this function is being used to create
         the `global_mct_head' & `global_mct_tail' members of
         `kd_codestream' based only on information in the main codestream
         header.  Strictly speaking, it is possible that insufficient
         marker segments might be found in the main header to do this
         successfully in a legal Part 2 codestream.  However, without
         this information it is not possible to know the dimensions of
         the output components ahead of time, which is an accidental oversight
         by the committee which defined JPEG2000 Part2.  For this reason,
         we shall treat such codestreams as illegal, but we shall not
         insist that any of the transform coefficients (linear coefficients
         or offsets) be available from the main codestream header; only
         the structure is required (MCO and MCT marker segments).  In
         particular, this means that the present function leaves all but
         the first 6 member fields of each `kd_mct_block' equal to 0
         (i.e., as if each transform block was a null transform) when
         invoked with a negative `tile_num' argument. */
  public: // Member functions
    kd_mct_stage() { memset(this,0,sizeof(*this)); }
    ~kd_mct_stage()
      {
        if (input_required_indices != NULL)
          delete[] input_required_indices;
        if (output_comp_info != NULL) delete[] output_comp_info;
        if (blocks != NULL) delete[] blocks;
      }
    void apply_output_restrictions(kd_output_comp_info *output_comp_info,
                                   int num_components_of_interest=0,
                                   const int *components_of_interest=NULL);
      /* This function should be directly invoked on the last stage in a
         list of `kd_mct_stage' objects (i.e., on the object whose
         `next_stage' member is NULL).  It converts the restrictions on
         the set of required output components, as identified by the
         `kd_output_comp_info::apparent_idx' values in the supplied
         `output_comp_info' argument (this should be the global
         `kd_codestream::output_comp_info' array), into a set of required
         input components for each stage.  Specifically, it starts
         by reflecting the output component requirements to the final stage
         in the list.  It then works backward through the stages until it
         arrives at the first stage, at which point the set of required
         codestream components is known.  In each stage, the function sets
         the `input_required_indices' array entries, the `apparent_idx',
         `from_apparent' and `apparent_block_comp_idx' members of each
         `output_comp_info' array, the `kd_mct_stage::num_apparent_outputs'
         and the `kd_mct_block::num_apparent_outputs' values.
            The `num_components' of interest and `components_of_interest'
         arguments have the same interpretation as their namesakes in the
         `kdu_tile::set_components_of_interest' function. */
  public: // Data
    int num_inputs;
    int num_required_inputs;
    int *input_required_indices;
    int num_outputs;
    int num_apparent_outputs;
    kd_output_comp_info *output_comp_info;
    int num_blocks;
    kd_mct_block *blocks;
    kd_mct_stage *prev_stage; // Closer to the codestream image components
    kd_mct_stage *next_stage; // Closer to the output image components
  };
  /* Notes:
       Each stage of the multi-component transform has a well-defined
     contiguous set of input and output components, even if not all of the
     components in this range are fully used or defined.  The stages appear
     in the order required for inverse transformation (synthesis) during
     decompression.  For this reason, the first `num_inputs'
     member for the first MCT stage is guaranteed to be equal to the
     number of codestream image components.
        In order to satisfy the constraints described by IS 15444-2,
     the `num_inputs' value of any one stage must not be less
     than the `num_outputs' value of the previous stage.  Also,
     every one of the `num_inputs' must be "touched" by one or
     another MCT block.  However, not all of the `num_outputs'
     need to be explicitly generated by any MCT block.  Those which are
     not are known as NULL components -- they are defined to be identically
     equal to 0.
        Once the list of stages has been created, the `num_outputs' value
     from one stage is augmented (if necessary) to the `num_inputs' value
     of the next stage.  This is done only to simplify the way in which
     stages are manipulated.
        The `input_required_indices' array contains one entry for each
     of the input components.  The k'th entry is non-negative if and only
     if input component k is "required", meaning that it contributes to at
     least one of the output components which are apparent and of interest
     to the application, as identified by the
     `kd_output_comp_info::is_of_interest' values in the
     `output_comp_info' array.  Moreover, non-negative indices in the
     `input_required_indices' array identify the ordinal position
     occupied by the corresponding component, in the set of all required
     components.  For non-initial stages, this ordinal position is the same
     as the apparent index of the previous stage's output component.
        It may not always be easy (or expedient) to determine
     precisely whether an input component actually contributes to the
     output produced by a transform block, so it is possible that the
     set of input components which are marked as "required" contains
     some components which could potentially be ignored.  Nevertheless,
     reasonable efforts are made to limit the set of required input
     components, based on the set of required output components.
        The `num_required_inputs' member holds the total number of
     non-negative entries in the `input_required_indices' array.
        The `prev_stage' member is NULL if this is the first stage in
     the inverse (synthesis) multi-component transform, meaning that the
     stage's input components are the codestream image components.
        The `next_stage' member is NULL if this is the last stage in the
     inverse (synthesis) multi-component transform, meaning that the stage's
     output components are the final output image components produced by
     the decompression process. */

/*****************************************************************************/
/*                              kd_codestream                                */
/*****************************************************************************/

struct kd_codestream { // State structure for the "kdu_codestream" interface
  public: // Member functions
    kd_codestream()
      { // We will be relying on all fields starting out as 0.
        memset(this,0,sizeof(*this));
        max_unloadable_tiles = 64; // A reasonable default value
      }
    ~kd_codestream();
    void construct_common();
      /* Called from within `kdu_codestream::create' after some initial
         construction work to set the thing up for input or output. */
    void construct_output_comp_info();
      /* Called if `output_comp_info' is NULL, but before (or if
         necessary inside) `finalize_construction'.  During reading, this
         function may not be called until all main header information
         has been parsed, since the information in `siz' is not
         complete until both the SIZ and CBD marker segments have been
         parsed.  For codestreams created for writing or for interchange,
         this function is called from within `construct_common'. */
    void finalize_construction();
      /* For input codestreams, this function is called immediately after
         the main header has been read.  For output codestreams, the
         call is deferred so as to give the caller an opportunity to
         modify coding parameters and call `kdu_params::finalize_all'.  It
         is invoked when information from those coding parameters is
         first required, which will be no later than the first call to
         `kdu_codestream::open_tile'. */
    void restrict_to_fragment(kdu_dims fragment_region,
                              int fragment_tiles_generated,
                              kdu_long fragment_tile_bytes_generated);
      /* This function is called immediately after `construct_common'
         if the `kdu_codestream::create' function supplied a non-NULL
         `fragment_region'. */
    void restart();
      /* Called from within `kdu_codestream::restart' to perform common
         re-initialization steps for an existing code-stream. */
    kd_tile *create_tile(kdu_coords idx);
      /* Creates and completely initializes a new tile, installing it in the
         internal array of tile references and returning a pointer to the
         new tile.  The pointer returned may be equal to KD_EXPIRED_TILE if
         the tile was found not to belong to the current region of interest.
         Note that this function might reclaim previously released tiles
         from a cache of released typical tiles, managed by the
         `typical_tile_cache' member.  The `idx' argument holds the absolute
         index of the tile, which must lie within the range represented by
         the `tile_indices' region member. */
    void trim_compressed_data();
      /* This function may be called periodically to trim away the storage
         associated with compressed data bytes which will never be included
         in the final compressed representation.  The function can do nothing
         unless `kdu_codestream::set_max_bytes' has been called to establish
         a target compressed length. */
    void calculate_min_header_cost();
      /* Estimates the minimum total number of header bytes required by
         mandatory code-stream headers, recording the results in the member
         variables, `min_fixed_header_size' and `min_layer_header_size'.
         The minimum header cost for the first k quality layers is equal to
         `min_fixed_header_size' + `min_layer_header_size'*k.  The initial
         cost includes the main header, any tile-part headers and the EOC
         marker.  The per-layer cost includes 1 byte for every precinct, plus
         an additional 2 bytes per precinct if EPH markers are used and an
         additional 6 bytes per precinct if SOP marker segments are used.  The
         costs reported here deliberately exclude the cost of any optional PLT
         or TLM pointer marker segments and may underestimate the cost of
         tile-part headers under some conditions. */
    kdu_long simulate_output(kdu_long &header_bytes,
                             int layer_idx, kdu_uint16 slope_threshold,
                             bool finalize_layer, bool last_layer,
                             kdu_long max_bytes=KDU_LONG_HUGE,
                             kdu_long *sloppy_bytes=NULL);
      /* This function plays a critical role in rate control and preparing
         for code-stream generation.  It may be used in applications which
         generate the entire code-stream once all precinct data has been
         generated, but it may also be used in applications which generate
         the code-stream incrementally, periodically sizing and generating
         whatever portion of the code-stream can be written, given the
         code-blocks which have been encoded so far.
            To understand the operation of this function, it is helpful to
         know that whenever a precinct's code-blocks have been encoded, the
         precinct is entered onto a "ready" list.  When the present function
         is called, it simulates the process of generating final code-stream
         output for a single quality layer of all such "ready" precincts.
         If the "ready" precincts are not sufficient to generate the
         full code-stream, their size is automatically scaled, using an
         algorithm which attempts to estimate the number of bytes which
         would be occupied by all packets belonging to the indicated layer
         which have not yet been written to the code-stream.  For this reason,
         the `max_bytes' value supplied to this function should represent
         the maximum number of bytes which are allowed for all packets
         belonging to the indicated layer, minus the number of bytes which
         have already been written for packets belonging to that layer.
            The simulation process is driven by the supplied distortion-length
         slope threshold.  A slope threshold of 0 yields the largest possible
         output size and 0xFFFF yielding the smallest possible output size
         (all code-block bytes discarded).
            The number of bytes returned includes the extra cost of tile and
         packet headers.  The number of such header bytes is also separately
         returned via the `header_bytes' argument.  The function deliberately
         does not count the cost of PLT or TLM marker segments.  Also,
         certain combinations of the ORGtparts and Porder code-stream
         parameter attributes can lead to minor inaccuracies in the simulated
         lengths due to difficulties in anticipating the number of tile-parts
         which will need to be written.  Both the return value and the
         `header_bytes' value are scaled to represent the expected cost over
         all packets of the indicated layer which have not yet been written
         to the code-stream.
            The `finalize_layer' argument should be true if this is the last
         call to this function with the current layer index.  This causes the
         packet header state information to be saved to a place from which
         it will be restored in each simulation attempt for the next layer.
         Also, it is mandatory to finalize the layers during simulation
         before actually generating packet data.
            The `last_layer' argument should be true if this is the final
         layer for which any information will be generated.  If any tile
         contains more layers, their contribution will be included at this
         point as empty packets (i.e., the empty packet bit is set to 0).
         Any such empty packet is never assigned an SOP marker.
            The `max_bytes' argument may be used to supply an upper bound on
         the total number of bytes which are allowed to be generated here.
         This allows the simulation process to stop as soon as this limit
         is exceeded.  In this case, the returned byte count may not be a
         true reflection of the full cost of generating the layer, but it
         is guaranteed to exceed the `max_bytes' limit.
            When the function is called with `finalize_layer' true, the
         `max_bytes' limit must not, under any circumstances be exceeded.
         The assumption is that the caller has already done the work of
         figuring out a slope threshold which will satisfy the relevant
         rate constraint -- if not, `max_bytes' can be set to
         KDU_LONG_HUGE (the default).  Note that KDU_LONG_HUGE is a
         very large "kdu_long" value which can be represented exactly as a
         double precision floating point quantity; it may be less than
         KDU_LONG_MAX.
            A non-NULL `sloppy_bytes' argument may be supplied only when
         finalizing the last layer, once all code-blocks in the entire
         code-stream have been generated.  In this case, the caller must be
         certain that a slope threshold of `slope_threshold'+1 can satisfy
         the limit imposed by `max_bytes'.  The function first runs the low
         level packet-oriented simulation tools with this value of
         `slope_threshold'+1 and then runs them again (this time finalizing)
         with `slope_threshold'.  On the second run, any additional coding
         passes which would be contributed due to the reduced threshold are
         trimmed away from the relevant code-blocks (they cannot be recovered
         later on) until the number of additional bytes is less than the value
         of *`sloppy_bytes'.  The *`sloppy_bytes' is progressively decremented,
         so that upon return the caller may determine the total number of
         available bytes which could not be used up.  The caller should then
         generate packet data using the supplied slope threshold.
            This `sloppy_bytes' option slows down the rate control process and
         is recommended primarily when you can expect quite a few code-block
         passes to have the same length-distortion slope value.  This happens
         during transcoding, where the only slope information available comes
         from the indices of the quality layers to which the coding passes
         originally contributed.  It may also happen if the block encoder
         does not collect a rich set of length-distortion statistics, simply
         estimating a rough prioritization instead. */
    void pcrd_opt(bool trim_to_rate, double tolerance);
      /* Runs the PCRD-opt algorithm to find the slope thresholds for each
         of the `num_sized_layers' quality layers for which an entry appears
         in the `layer_targets', `layer_sizes' and `layer_thresholds' arrays.
         Upon return, the `layer_thresholds' array holds the distortion-length
         slope thresholds for each quality layer, which are found to achieve
         the cumulative layer sizes specified in the `layer_targets' array, as
         closely as possible.
            Non-zero entries in the `layer_targets' array are not altered by
         this function.  Any entry which is found to be zero is replaced by
         a suitable non-zero target size, based on the following principles:
         1) Cumulative layer target sizes are to be spaced logarithmically.
            This policy allows target sizes to be assigned to any layer which
            falls between layers having known target sizes.
         2) If the last quality layer has an unknown size, all remaining
            coded data will be included in that layer and its entry in the
            `layer_targets' array will be set equal to the actual cumulative
            number of bytes allocated to all quality layers.
         3) If one or more initial layer have an unknown target size, their
            target sizes will be assigned logarithmically following the
            convention that each layer should increase the cumulative size
            by a factor of sqrt(2).
               The function is designed to work under both single flush and
         incremental flushing conditions.  When incremental flushing is used,
         the function will be called multiple times.  Each call determines a
         suitable set of slope thresholds for incremental code-stream, after
         which the caller performs the incremental code-stream generation.
         This latter activity updates entries in the `layer_sizes' array
         which keeps track of the total size of each quality layer, excluding
         optional pointer information.  The `layer_targets' values refer to
         the cumulative size at each layer, spread over the entire code-stream.
         The present function finds slope thresholds which, if applied to
         all code-blocks not yet written to the code-stream, would be expected
         to produce a number of bytes equal to the relevant `layer_targets'
         value minus the corresponding `layer_sizes' value.
               The `trim_to_rate' argument has the same interpretation as it
         does in the `kdu_codestream::flush' function.  You should not use this
         option unless all code-blocks have already been encoded, meaning that
         this is the last call to this function prior to complete generation
         of the code-stream.
               The `tolerance' argument also has the same interpretation as it
         does in the `kdu_codestream::flush' function.  There is no restriction
         on its use.
    */
    bool generate_codestream(int max_layers);
      /* Generates as much of the final code-stream as possible, given the
         packet progression order which has been selected and the precincts
         which have actually been finalized.  Returns true if the code-stream
         has been completely generated after this function finishes its
         work; in this case, the EOC marker is also written.
         Returns false otherwise.
            The contents of each quality layer are determined from the
         distortion-length slope thresholds recorded in the `layer_thresholds'
         member array.  The number of available thresholds must not be less
         than `max_layers'.  If `max_layers' is less than the number of actual
         quality layers in any tile, the additional quality layers will all
         be assigned empty packets.
            The "prcr_opt" member function will normally be used first to find
         distortion-length slope thresholds for the various quality layers.
         In any event, some simulation must have been performed to set up the
         packet size information required to determine tile-part header sizes,
         which are required for writing SOT marker segments.  This function
         always creates at least one new tile-part for every tile.  If
         requested, PLT marker segments are written into the tile-part headers
         to identify the lengths of packets generated in this call. */
    void set_reserved_layer_info_bytes(int num_layers);
      /* Sets aside a certain number of bytes for writing a comment marker
         containing a text description of the size and distortion-length slope
         associated with each layer.  The actual marker must be written by
         a subsequent call to `gen_layer_info_comment'. */
    void gen_layer_info_comment(int num_layers, kdu_long *layer_bytes,
                                kdu_uint16 *thresholds);
      /* Writes the comment marker identified above in connection with
         `set_reserved_layer_info_bytes'. */
    void unload_tiles_to_cache_threshold();
      /* Releases unloadable tiles until the memory cache and/or unloadable
         tile count threshold is reached.  The function first tries to
         release those unloadable tiles which do not lie in the current
         region of interest.  If one or both thresholds are still exceeded,
         the function continues to release unloadable tiles, even if they
         do belong to the current region of interest. */
  private: // Helper functions
    void read_main_header();
      /* Only for input objects.  Reads all main header marker segments
         (starting just after the SIZ marker segment) up to and
         including the first SOT marker.  This function is shared
         by both `construct_common' and `restart'. */
    void freeze_comments();
      /* Called right before any generation or simulation tasks for code-stream
         generation. */
  public: // Links to Other Objects
    kdu_message *textualize_out; // NULL unless params to be textualized

  public: // Owned Resources
    kd_compressed_input *in; // NULL unless an input codestream object.
    kd_compressed_output *out; // NULL unless an output codestream object.
    siz_params *siz;
    kd_marker *marker; // General marker reading service. NULL if `in' is NULL
    kd_pp_markers *ppm_markers; // NULL unless PPM markers are used.
    kd_buf_server *buf_server; // Serves resources to buffer compressed data
    kdu_block *block; // Shared resource, used by non-threaded applications
    kd_compressed_stats *stats; // NULL unless rate prediction is being used.
    kd_tpart_pointer_server *tpart_ptr_server; // NULL if not keeping addresses
    kd_precinct_server *precinct_server; // Always active
    kd_codestream_comment *comhead, *comtail; // Linked list of comments
    kd_tlm_generator tlm_generator; // Not initialized until the first
                 // call to `generate_codestream', and then only if required.
    kd_mct_stage *global_mct_head; // Provided for robust cleanup if error
    kd_mct_stage *global_mct_tail; // occurs while constructing MCT stage list.

  public: // Dimensions and Parameters
    int profile; // 0 thru 5.  Adjusted if a violation is detected.
    bool uses_mct; // If the `Sextensions_MCT' flag is present in SIZ.
    int num_components; // Number of codestream image components.
    int num_apparent_components; // Codestream components visible acrosss API
    int num_output_components; // Same as `num_components' unless MCT is used
    int num_apparent_output_components; // =`num_apparent_components' if no MCT
    kdu_component_access_mode component_access_mode; // See below
    kdu_dims canvas; // Holds the image origin and dimensions; this is
                     // affected by fragment regions
    kdu_dims tile_partition; // Holds the tiling origin and dimensions; this
                             // is not affected by fragment regions
    kdu_coords tile_span; // Tiles across and down the entire codestream
    kdu_dims tile_indices; // Range of tile indices in `tile_refs' array
    int discard_levels, min_dwt_levels, max_apparent_layers;
    int max_tile_layers; // Max layers in any tile opened so far.
    int num_open_tiles; // Number of tiles opened, but not yet closed.
    kdu_dims region; // Current region of interest; often identical to `canvas'
    bool cannot_flip; // If true, codestream not compatible with view flipping
    bool initial_fragment; // These are both true, except possibly when
    bool final_fragment; // generating fragmented codestreams
    double fragment_area_fraction; // Fraction of image in current fragment
    int prev_tiles_written;           // These two parameters are zero, except
    kdu_long prev_tile_bytes_written; // when `initial_fragment' is false.

  public: // Arrays
    kd_comp_info *comp_info; // Array with `num_components' members
    kd_output_comp_info *output_comp_info; // `num_output_components' members
    kd_tile_ref *tile_refs; // One for each tile, in raster order

  public: // Information used for code-stream generation and rate control
    kd_tile *tiles_in_progress_head; // Head & tail for list of tiles which
    kd_tile *tiles_in_progress_tail; // have been opened, but not yet flushed
    kd_global_rescomp *global_rescomps; // Manages precincts ready for output
    int num_incomplete_tiles; // Number of tiles for which one or more
                              // packets have yet to be generated.
    int num_sized_layers; // Number of elements in each of the following arrays
    kdu_long *layer_sizes; // Bytes written to each layer; non-cumulative
    kdu_long *layer_targets; // Target sizes for each layer; cumulative
      // maintain state between multiple invocations of `kdu_codestream::flush'
    kdu_uint16 *layer_thresholds; // Used by `kdu_codestream::flush' to store
      // slope thresholds between code-stream sizing and generation.
    kdu_long min_fixed_header_size; // See description of the function,
    kdu_long min_layer_header_size; // `calculate_min_header_cost'.
    bool reslength_constraints_used; // If reslength constraints used anywhere 
    bool reslength_constraints_violated; // If any reslength constraint failed
    bool reslength_warning_issued; // If using together with incremental flush
    kd_reslength_checker *reslength_checkers; // 1+`num_components' entries;
        // first entry holds global reslength checker; then one per component.
  
  public: // Flags and other State Variables
    bool allow_restart; // True if `kdu_codestream::restart' is allowed
    bool transpose, vflip, hflip; // Geometric manipulation parameters
    bool resilient; // If true, error resilience is maximized.
    bool expect_ubiquitous_sops; // See declaration of "set_resilient" function
    bool fussy; // If true, sensitivity to correctness is maximized.
    bool interchange; // True if `in' and `out' are both NULL.
    bool simulate_parsing_while_counting_bytes; // See `set_max_bytes'
    bool persistent; // If `interchange' is true or if an input object's
                     // `kdu_codestream::set_persistent' function was called
    bool cached; // True if compressed source advertises KDU_SOURCE_CAP_CACHED
    bool in_memory_source; // True if `in' offers `KDU_SOURCE_CAP_IN_MEMORY'
    bool tiles_accessed; // Becomes false after first tile access
    bool construction_finalized; // True once `finalize_construction' is called
    bool comments_frozen; // True after header sizing has started
    bool header_generated; // True once main header has been written.
    kdu_int32 block_truncation_factor;// Used only by `kdu_subband::open_block'
    kdu_long header_length; // Num bytes written to main header
    int reserved_layer_info_bytes; // Bytes reserved for layer info COM segment
    kd_tile *active_tile; // NULL unless we are in the middle of reading a tile
    kdu_long next_sot_address; // For seeking to the next unread tile-part.
    int next_tnum; // Negative, except while scanning 1'st tparts in Profile-0
    int num_completed_tparts; // Number of tile-parts actually read or written.
    kdu_uint16 min_slope_threshold; // 0 until `set_min_slope_threshold' called
    clock_t start_time; // Used by `report_cpu_time'.

  public: // Tile cache management
    kd_tile *unloadable_tiles_head;
    kd_tile *unloadable_tiles_tail; // Tiles enter list from the tail
    kd_tile *unloadable_tile_scan; // Points to next tile in the list, to be
        // checked by `unload_tiles_to_cache_threshold' to see if it belongs
        // to the current region of interest.  Becomes NULL if all tiles in
        // the unloadable list belong to the region of interest.
    int num_unloadable_tiles; // Number of tiles in unloadable list
    int max_unloadable_tiles; // Limits the length of the unloadable tiles
        // list to avoid long searches for tiles which belong to the current
        // region of interest.
    kd_tile *typical_tile_cache; // List of released typical tiles, all of
                                 // which have `kd_tile::tile_ref'=NULL.
  };
  /* Notes:
         The `num_tparts_used' count does not include tiles which we skipped
     over since they had no intersection with the region of interest.
         The `active_tile' member is used during code-stream parsing to keep
     track of the tile whose tile-part is being actively parsed.  If `cached'
     is true, the compressed data is managed by a cache and there is no
     parsing as such.  In this case, `active_tile' is always NULL and
     `tpart_ptr_server' should remain NULL.
         The `global_rescomps' array contains 33*num_components entries.
     The entry at location d*num_components+c is used to manage precincts
     which are ready for rate-control simulation, belonging to component c
     at resolution depth d.  Depth 0 corresponds to the highest resolution
     level of each tile of component c.
         Information is maintained for two different types of image
     components here.  Most of the machinery is concerned with so-called
     "codestream image components".  These are the ones which are subjected
     to spatial DWT, quantization and block coding processes.  Consequently,
     we commonly refer to these simply as the components.  The image
     components, however, are often subjected to subsequent inverse
     colour or multi-component transformation processes in order to
     reconstruct a final image.  We refer to the components (or planes) of
     the final reconstructed image as "output image components".  The
     output components are described by the `num_output_components',
     `num_apparent_output_components' and `output_comp_info' members.
         As discussed extensively in the comments appearing with the second
     form of the `kdu_codestream::apply_input_restrictions' function, the
     set of visible (or apparent) output components may be different to
     the set of visible (or apparent) codestream components.  In fact, only
     one of these two sets of components may be restricted at any given
     time, depending on the state of the `component_access_mode' member.
        If `component_access_mode' is set to `KDU_WANT_CODESTREAM_COMPONENTS',
     the set of codestream components may be restricted or permuted via the
     `num_apparent_components' member, working in conjunction with
     `kd_comp_info::apparent_idx' and `kd_comp_info::from_apparent'.  In this
     case, however, information about output components will not be made
     available over the API -- output components will be treated as synonymous
     with codestream components.
        If `component_access_mode' is set to `KDU_WANT_OUTPUT_COMPONENTS', the
     set of output components may be restricted or permuted via the
     `num_apparent_output_components' member, working in conjunction with
     `kd_output_comp_info::apparent_idx' and
     `kd_output_comp_info::from_apparent'.  In this case, all codestream
     components are visible across the API in their original form, except
     that applications will not be able to access a non-empty
     `kdu_tile_comp' interface for any tile-component which is not
     involved in the reconstruction of any visible output components within
     the relevant tile (this may vary from tile to tile). */

/*****************************************************************************/
/*                                kd_tile                                    */
/*****************************************************************************/

#define KD_EXPIRED_TILE ((kd_tile *)(-1))

struct kd_tile { // State structure for the "kdu_tile" interface
  public: // Member functions
    kd_tile(kd_codestream *codestream, kd_tile_ref *tref,
            kdu_coords idx, kdu_dims tile_dims);
      /* Sets up only enough information for the tile to locate itself
         within the codestream canvas.  `idx' holds the absolute index of
         the tile, but this must lie within the range identified by
         `codestream->tile_indices'. */
    ~kd_tile();
      /* Note that this function also detaches the tile object from the
         codestream object which references it, replacing the pointer with
         the special illegal value, KD_EXPIRED_TILE. */
    void release();
      /* This function serves a similar role to the destructor, except that
         it reserves the option to move the object onto a list of cached
         "typical" tiles managed by the `kd_codestream' object.  This is
         done if the tile has its `is_typical' member set to true and if the
         codestream is open for reading.  The cached typical tile's structure
         can be reused in the future, without rebuilding it from scratch,
         subject to appropriate conditions.  If the function chooses not to
         move the tile onto the typical list, the destructor will be invoked,
         so this is a self-efacing function. */
    void initialize();
      /* Called immediately after a successful construction.  Builds all
         subordinate structures and fills in all fields.   If the codestream
         object has the persistent mode (applies to persistent input objects
         and all interchange objects), the region of interest members
         in the tile and all of its subordinate structures will be set to
         empty regions. Otherwise, the `set_elements_of_interest' function
         will be called immediately. */
    void recycle(kd_tile_ref *tref, kdu_coords idx, kdu_dims tile_dims);
      /* This function is similar to `initialize', but works on a previously
         constructed (typical) tile, rather than a tile which has been
         freshly created using the constructor.  If possible, the function
         will reuse the previously constructed structures (this is generally
         possible if the new tile is also a typical one).  Otherwise, it
         will delete those structures and invoke `initialize' to start over
         again.  The function is called only from `kd_codestream::create_tile'
         and only on tiles which were previously added to the typical tile
         cache by `kd_tile::release'. */
    void restart();
      /* Called from within `kdu_codestream::restart', so as to recycle all
         resources as early as possible and reset state variables in
         preparation for re-use.  Sets the `needs_reinit' flag, since
         the `reinitialize' function must be called to complete the restart
         operation -- this is deferred until the tile is first accessed
         (opened, or accessed for parsing). */
    void reinitialize();
      /* Called when a tile is first accessed in any way, after a previous
         call to `restart' (i.e., after a code-stream restart). */
    void open();
      /* Called by `kdu_codestream::open_tile' after the tile has been
         constructed and initialized (if necessary).  The function marks
         the tile as open until it is subsequently closed.  If the codestream
         object has the persistent mode (applies to persistent and all
         interchange objects), the tile's region of interest fields will
         all be set up here. */
    bool read_tile_part_header();
      /* Returns false, if the code-stream source was exhausted before a
         new tile-part header could be completely read (up to and including
         the SOD marker).  Except in this event, the current tile becomes
         the codestream object's active tile upon return. */
    bool finished_reading();
      /* This function may be called as soon as we can be sure that no more
         information can be read for the tile.  When this happens, the tile
         is marked as `exhausted' and a search is conducted to see if we can
         destroy any precincts whose existence serves only to allow the parsing
         of packet headers.
            It can happen that the tile deletes itself inside this function,
         if the tile's resources are no longer needed by the application.  In
         this case, the function returns true, indicating to the caller that
         no further attempt should be made to reference the tile.  In all
         other circumstances, the function returns false. */
    kdu_long generate_tile_part(int max_layers, kdu_uint16 slope_thresholds[]);
      /* If all packets for the tile have already been transferred, the
         function does nothing and returns 0.  Otherwise, it generates the
         next tile-part header and all packets for that tile-part, using the
         packet sequencer to determine both the order of packets and the
         number of packets which belong to this tile-part.  The number
         of packets generated may be limited by the number of precincts
         for which code-block data is currently available.  The function
         returns the total number of bytes in the tile-part, including all
         marker codes and segments.
            The `max_layers' argument indicates the number of quality layers
         for which rate allocation information is available in the
         `slope_thresholds' array.  If the tile contains additional layers,
         the corresponding packets are assigned the special empty packet code,
         occupying only a single byte.
            If this function completes the generation of all the tile data,
         it may release the tile itself, unless its continued existence is
         otherwise required (e.g., it may be open for access by the
         application).  Also, if the maximum number of tile-parts advertised
         by a `codestream->tlm_generator' object exceeds the number of
         tile-parts actually generated when the tile data has all been
         written, the function writes extra empty tile-parts as required.
         The empty tile-parts are each 14 bytes long, but the function
         returns only the size of the last non-empty tile-part which was
         generated. */
    void remove_from_in_progress_list();
      /* This function is called after the last tile-part of a tile has been
         generated.  It is also called when destroying or restarting a tile,
         just to be on the safe side, although it should have been removed
         from the in-progress list previously.  See the notes below for a
         discussion of "in-progress" tiles. */
    void adjust_unloadability()
      { // Called when any of the conditions described in the notes below
        // associated with tile unloadability might have changed.
        if ((!codestream->persistent) ||
            ((codestream->tpart_ptr_server==NULL) && !codestream->cached))
          { assert(!is_unloadable); return; }
        if (is_open || (this==codestream->active_tile))
          { // Should not be on the unloadable list
            if (is_unloadable)
              withdraw_from_unloadable_list();
          }
        else
          { // Should be on the unloadable list
            if (!is_unloadable)
              add_to_unloadable_list();
          }
      }
  private: // Helper functions
    friend class kdu_tile;
    void set_elements_of_interest();
      /* Called from within `kd_tile::open' and/or `kd_tile::initialize'
         to mark each component, resolution, subband, precinct and
         precinct-band with the current region of interest, resolution of
         interest, components of interest and layers of interest.  If the
         codestream mode is "persistent" (applies to input objects and all
         interchange objects), this function is delayed until the tile is
         opened (an event which may occur any number of times, with different
         regions, resolutions, components and layers of interest).  Otherwise,
         the function is called only once, when the tile is initialized.  This
         is important, since non-persistent input objects' resources will be
         cleaned up as soon as they are known not to be
         required.  To do this, region of interest information must be made
         available during parsing, which may occur prior to the point at which
         a tile is actually opened. */
    void withdraw_from_unloadable_list();
      /* Removes a tile, whose `is_unloadable' member is true, from the
         unloadable tiles list managed by `kd_codestream', leaving
         `is_unloadable' equal to false. */
    void add_to_unloadable_list();
      /* Adds a tile, whose `is_unloadable' member is false, to the
         unloadable tiles list managed by `kd_codestream', leaving
         `is_unloadable' equal to true. */
    float find_multicomponent_energy_gain(int comp_idx,
                                          bool restrict_to_interest);
      /* Finds the energy gain factor associated with the contribution of
         the indicated codestream image component to the output image
         components.  For this purpose, an impulsive distortion in codestream
         component `comp_idx' is considered, having magnitude equal to the
         component's nominal range.  The impact of this impulsive distortion
         is evaluated at the outputs of any multi-component transform, or
         Part-1 colour transform.  The resulting output distortion is
         expressed relative to each output component's nominal range and
         the sum of these squared normalized quantities is taken and returned.
         If `restrict_to_interest' is true, output distortion contributions
         are ignored if they are not currently of interest to the
         application. */
  public: // Links and Identification
    kd_codestream *codestream;
    int t_num; // Absolute tile-number, as used by parameter sub-system
    kdu_coords t_idx; // Absolute 2D tile-index
    kd_tile_ref *tile_ref; // Points to entry in `kd_codestream::tile_refs'
                           // which points to us, or NULL if we are not there.
    kd_tpart_pointer *tpart_ptrs; // Pointers for seekable sources
    kd_tile *in_progress_next, *in_progress_prev; // See below
    kd_tile *unloadable_next, *unloadable_prev; // See below
    kd_tile *typical_next; // Used to build a list of released typical tiles.
                           // Tiles on this list have `tile_ref'=NULL

  public: // Owned Resources
    kd_pp_markers *ppt_markers; // NULL except possibly in tpart header reading
    kd_pph_input *packed_headers; // NULL unless tile has packed packet headers
    kd_packet_sequencer *sequencer;
    kd_precinct_pointer_server precinct_pointer_server;
    kd_reslength_checker *reslength_checkers; // 1+`num_components' entries;
      // same structure as `kd_codestream::reslength_checkers' but for
      // tile-specific reslength constraints.
  
  public: // Dimensions and Parameters
    bool is_typical; // All typical tiles have the same structure & transform
    bool fully_typical; // All fully typical tiles have identical coding params
    int num_components; // Number of tile-components
    int num_layers;
    int num_apparent_layers;
    int max_relevant_layers; // See below
    kdu_long total_precincts; // Used only during code-stream generation
    kdu_long max_relevant_packets; // See below
    kdu_dims dims; // Region occupied by the tile on the canvas
    kdu_dims region; // Region of interest within tile.
    kdu_coords coding_origin;

  public: // Arrays and Lists
    kd_mct_stage *mct_head; // Linked list of MCT stages, starting from the one
    kd_mct_stage *mct_tail; // whose input components are the tile-components
    kd_tile_comp *comps;
    kdu_long structure_bytes; // Total bytes contained in all embedded
        // structures, excepting only precincts and code-block bit-stream
        // buffers.  Whenever this value changes, the changes should also be
        // reflected in the `codestream->buf_server' object, so that the total
        // memory load associated with tile data structures can be included in
        // cache threshold calculations.  This enables precinct and tile
        // unloading decisions to be made in a wholistic way, based on
        // total memory consumption.

  public: // Flags and other State Variables
    bool use_sop, use_eph, use_ycc;
    bool empty_shell; // True if tile header was not available when opened.
    bool is_in_progress; // See `in_progress_next' and `in_progress_prev'
    bool is_open;
    bool is_unloadable; // See below
    bool is_addressable; // See below
    bool closed; // Once closed, the tile may no longer be accessed.
                 // Persistent tiles are never marked as closed.
    bool initialized; // Fields below here are valid only once initialized
    bool needs_reinit; // Set inside `restart' to indicate the need for
                       // reinitialization when the tile is first accessed.
    bool insert_plt_segments; // If true, PLT marker segs introduced
    bool resolution_tparts; // Start new tile-part at resolution boundaries
    bool component_tparts; // Start new tile-part at component boundaries
    bool layer_tparts; // Start new tile-part at layer boundaries
    int num_tparts; // Zero if number of tile-parts unknown.
    int next_tpart; // Index of next tile-part to be read or written.
    int sequenced_relevant_packets; // See below
    int next_input_packet_num;
    bool skipping_to_sop; // See notes below.
    int next_sop_sequence_num; // Valid only if `skipping_to_sop' is true.
    int saved_num_sequenced_packets;// For "kdu_packet_sequencer::save_state"
    bool exhausted; // True if no more tile-parts are available for this tile.
  };
  /* Notes:
        The `max_relevant_layers' member indicates the maximum number of
     layers from the current tile which can ever be relevant to the
     application.  It holds the same value as `num_layers', unless the
     code-stream was created for reading and is non-persistent, in which
     case it holds the value of `num_apparent_layers'.  This member is used
     to avoid parsing any more packets than we have to when reading
     non-persistent code-streams.
        The `max_relevant_packets' member is equal to the total number of
     packets in the tile, unless the code-stream was created for reading,
     is non-persistent, and `kdu_codestream::apply_input_restrictions' has
     been used to reduce the spatial region of interest, or to identify one
     or more resolutions, components or layers as irrelevant to the
     application.  In that case it holds the product of the number of the
     `max_relevant_layers' value and the number of precincts which are
     actually relevant to the identify spatial region, resolution and
     components.
        Similarly, `sequenced_relevant_packets' counts the number of packets
     which have been sequenced (either written or read, as appropriate) so
     far, skipping only those packets which are deemed to be irrelevant to
     the application.  As for `max_relevant_packets', all packets are
     considered relevant if the code-stream was not created for reading, or
     if the code-stream is persistent, regardless of any calls to
     `kdu_codestream::apply_input_restrictions'.  When this value reaches
     the value of `max_relevant_packets', there is no need to parse any
     further contents from the tile.
        If `skipping_to_sop' is true, packets are automatically discarded
     (this is a pseudo-transfer which leaves the packet empty) until one is
     reached whose packet number agrees with the value of
     `next_sop_sequence_num'.  To assist this operation, the
     `next_input_packet_num' counter keeps track of the zero-based index
     of the next packet to be parsed.  Note that we need a separate counter,
     in addition to `sequenced_relevant_packets', since not all packets
     which are parsed need necessarily be relevant to the application.
        A tile is said to be "unloadable" once the following conditions have
     been satisfied:
     1) the codestream is in persistent mode;
     2) the compressed data source is seekable or a cache;
     3) this tile is not currently open;
     4) this tile is not the one referenced by `kd_codestream::active';
     Such tiles are enterered on a list managed by the `unloadable_next'
     and `unloadable_prev' members, and have their `is_unloadable' flag set
     to true.  When an unloadable tile is deleted, the corresponding
     `kd_tile_ref::tile' entry is set to NULL rather than `KD_EXPIRED_TILE'.
     This ensures that the tile can be re-parsed from the compressed data
     source if required again at a later point.  Tiles on the unloadable
     list may be deleted automatically, according to a tile memory management
     policy.
        A tile is said to be "addressable" if the following conditions hold:
     1) the codestream is in input (reading) mode;
     2) the compressed data source is not a cache;
     3) the compressed data source is seekable; and
     4) the current tile-part contains usable precinct pointer information.
     Such tiles are identified by the `is_addressable' flag.  This flag is
     set on each call to `read_tile_part_header'.  All that actually
     matters is that we correctly identify when the codestream has an
     active tile which is not addressable; in that case, the tile's
     packets must be desequenced up until the end of the tile-part before
     any seeking on the compressed data source can be allowed.
        A tile is said to be "in-progress" if the following conditions
     apply:
     1) the codestream was created for output;
     2) the tile has already been opened -- it may or may not have been closed;
     3) the tile's contents have not yet been fully flushed to the codestream.
     In-progress tiles are entered onto a doubly-linked list managed by the
     `in_progress_next' and `in_progress_prev' pointers.  The head and
     tail of this list are managed by `kd_codestream::tiles_in_progress_head'
     and `kd_codestream::tiles_in_progress_tail'. */ 

/*****************************************************************************/
/*                              kd_tile_comp                                 */
/*****************************************************************************/

struct kd_tile_comp { // State structure for the "kdu_tile_comp" interface
  public: // Member functions
    kd_tile_comp()
      { // We will be relying on all fields starting out as 0.
        memset(this,0,sizeof(*this));
        G_tc = G_tc_restricted = -1.0F;
      }
    ~kd_tile_comp();
    void initialize_kernel_parameters(int atk_idx, kdu_kernels &kernels);
      /* On entry, the `kernel_id' member is valid.  On exit, the
         other `kernel_xxx' parameters are valid.  The `atk_idx' value is
         used only if `kernel_id' is equal to `Ckernels_ATK'.  The
         function leaves `kernels' initialized with the relevant transform
         kernel, for further use by the caller if desired. */
    void reset_layer_stats()
      {
        if (layer_stats != NULL)
          {
            int num_entries = ((1+dwt_levels)*tile->num_layers)<<1;
            memset(layer_stats,0,(size_t)(sizeof(kdu_long)*num_entries));
          }
      }
  public: // Links and Identification
    kd_codestream *codestream;
    kd_tile *tile;
    kd_comp_info *comp_info; // Ptr to relevant `codestream->comp_info' entry
    int cnum;

  public: // Dimensions and Parameters
    kdu_coords sub_sampling;
    kdu_dims dims;
    kdu_dims region;
    int dwt_levels, apparent_dwt_levels;
    bool reversible;
    int kernel_id; // Ckernels_W5X3, Ckernels_W9X7 or Ckernels_ATK
    bool kernel_symmetric, kernel_symmetric_extension;
    int kernel_num_steps;
    kdu_kernel_step_info *kernel_step_info, *kernel_step_info_flipped;
    float *kernel_coefficients, *kernel_coefficients_flipped;
    float kernel_low_scale; // Amount to scale low-pass lifting analysis output
    float kernel_high_scale; // Amount to scale hi-pass lifting analysis output
    int low_support_min, low_support_max; // low-pass synthesis impulse support
    int high_support_min, high_support_max; // high-pass synth impulse support
    float G_tc; // Global multi-component energy gain term; -ve if not known
    float G_tc_restricted; // Multi-component energy gain term based only on
            // those output components currently of interest; -ve if not known
    int recommended_extra_bits;
    kdu_coords blk; // Nominal code-block dimensions.
    int modes; // Block coder modes.  Flags defined in scope `cod_params'

  public: // Arrays
    kd_resolution *resolutions; // Contains 1+`dwt_levels' entries.
    kdu_long *layer_stats; // Contains `num_layers' pairs per resolution,
      // starting from the lowest resolution (`res_level'=0); each pair holds
      // number of parsed packets for the layer-resolution, followed by
      // number of parsed packet bytes for the layer-resolution.  Available
      // only for input codestreams; populated by `kd_precinct::read_packet'.

  public: // Flags and Other State Variables
    bool enabled; // See below
    bool is_of_interest; // See below
    kdu_coords grid_min; // These fields are used by "kd_packet_sequencer"
    kdu_coords grid_inc; // for spatially oriented progressions.
    kdu_coords saved_grid_min; // For "kd_packet_sequencer::save_state"
    kdu_coords saved_grid_inc; // For "kd_packet_sequencer::save_state"
  };
  /* Notes:
        `recommended_extra_bits' may be added to the sample bit-depth
     (precision) for this component to determine an appropriate number
     of bits for internal representations associated with the data
     processing path.
        In the reversible path, the value is set to the maximum bit-depth
     expansion factor, as reported in Table 17.4 of the book by Taubman and
     Marcellin, adding an extra 1 bit if the RCT has been used -- note that it
     is not desirable for luminance and chrominance components to have
     different numeric representations, since that would significantly
     complicate the colour transform procedure.
        In the irreversible path, the value is set to 7, which means
     that 16-bit representations are judged sufficient for 9 bit
     imagery or less.  Of course, the impact of selecting a reduced
     representation precision is only reduced accuracy in the
     irreversible processing path.
        The `enabled' flag is used to efficiently identify the tile-components
     which are enabled.  Remember that a tile-component may be disabled,
     either because the corresponding codestream image component is not
     apparent (only when the `kd_codestream::component_access_mode' is
     `KDU_WANT_CODESTREAM_COMPONENTS'), or because the tile-component
     is not involved in the construction of an apparent output image
     component (when the `kd_codestream::component_access_mode' is
     `KDU_WANT_OUTPUT_COMPONENTS').  In the latter case, all codestream
     image components are apparent, but disabled ones cannot be
     accessed -- this is important, since in the latter case the set
     of enabled components may vary from tile to tile.
        The `is_of_interest' flag is used to store the identify of components
     which are marked as being of interest, by calls to
     `kdu_tile::set_components_of_interest'.  This is done only when the
     component access mode is `KDU_WANT_CODESTREAM_COMPONENTS' or the
     `mct_head' member is NULL.  Otherwise, the components of interest are
     passed directly to the `kd_mct_stage::apply_output_restrictions'
     function.  Note that the `is_of_interest' flags do not generally
     belong to the component in which they are stored.  They are just
     copies of the information supplied to
     `kdu_tile::set_components_of_interest', without any attempt to apply
     any component index permutations which might be relevant. */

/*****************************************************************************/
/*                            kd_precinct_ref                                */
/*****************************************************************************/

  /* Objects of this class provide the vehicle through which precincts
     references are instantiated and removed from the array of precinct
     references maintained by the `kd_resolution' object.  It is important
     that the state information be small.  In particular, it has the same
     size as `kdu_long', which should never be smaller than the size of
     a memory pointer, but may be larger -- see the declaration of
     `kdu_long' in "elementary.h".  For precincts whose seek addresses are
     deduced from PLT marker segments in the code-stream, or precincts which
     are maintained by a caching data source, or precincts which are used by
     codestream objects created for interchange, this object provides
     facilities for recycling a disused precinct's resources while keeping its
     unique address around for rapid reloading in the event that it
     is required at a later point. */

class kd_precinct_ref {
  public: // Member functions
    kd_precinct_ref()
      { state = 0; }
    ~kd_precinct_ref()
      { if (!((state == 0) || (state & 1))) close(); }
    bool parsed_and_unloaded()
      { return ((state & 3) == 3) && (state != 3); }
      /* Returns true if the precinct is addressable (i.e., dynamically
         loadable) and it has been parsed and subsequently unloaded from
         memory. */
    kd_precinct *deref()
      { return (state & 1)?NULL:((kd_precinct *) _kdu_long_to_addr(state)); }
      /* Returns NULL unless the reference identifies a valid `kd_precinct'
         object, which is already loaded in memory. */
    kd_precinct *active_deref();
      /* Returns NULL unless the reference identifies a valid `kd_precinct'
         object, which is already loaded in memory and is in the active
         state. */
    kd_precinct *open(kd_resolution *res, kdu_coords pos_idx,
                      bool need_activate);
      /* Attempts to open the precinct at the indicated position within the
         array of precincts maintained by the indicated resolution object.
         `pos_idx' holds the horizontal and vertical position of the
         precinct (starting from 0) within the array of precincts for
         the given resolution.  That is, `pos_idx' holds a relative, not
         an absolute precinct index.
            If the precinct is known to be unloadable (this happens if the
         precinct has previously been opened and has then been permanently
         closed), the function returns NULL.
            If the precinct currently exists in memory (the `state' variable
         contains a valid pointer to the precinct), the function returns a
         pointer to that precinct; however, if `need_activate' is true
         and the precincts's `released' flag is true, `kd_precinct::activate'
         is called before returning the `kd_precinct' pointer.  When the
         codestream object was created for input or for interchange, the
         `need_activate' argument should be set to true only when the
         precinct is being opened in order to access code-block data.  In
         that case, the precinct must belong to an open tile and lie within
         the application's current region of interest, meaning that it will
         be released again when the tile is closed.  For codestream objects
         created for output, the argument should have no effect, since
         output precincts are not actually released.
            Otherwise, the function obtains a new `kd_precinct' object from
         the code-stream's precinct server and calls its `initialize'
         function.  If the `state' variable held the precinct's unique
         address value on entry, that address is written into the newly
         instantiated `kd_precinct' object's `unique_address' field, and
         the P bit (see notes on `state' below) is copied to the precinct's
         KD_PFLAG_PARSED flag.  If compressed data comes from a cached source,
         or the codestream object was created for interchange (no compressed
         data sources or targets), the newly created precinct is automatically
         marked as addressable, with its `unique_address' set to the
         precinct's unique identifier, as described in the comments appearing
         with the declaration of `kd_precinct::unique_address'.  For
         precincts which can be addressed using a seek address (for seekable
         compressed data sources), the seek address is installed using
         `set_address' from within the `kd_packet_sequencer::next_in_sequence'
         function.
            It is worth noting that this function may cause other precincts
         to be unloaded from memory, in order to conserve memory resources.
         If this happens, only precincts which have been released and
         which do not intersect with the current region of interest will be
         unloaded. */
    void close();
      /* Closes the active `kd_precinct' object whose pointer is in
         `state', calling its `closing' member function and recycling its
         storage.  If the precinct is marked as addressable, its
         `unique_address' member and KD_PFLAG_PARSED flag are recorded in
         `state' using the convention outlined in the notes below, allowing
         the precinct to be re-opened at a later point.
            In many cases, you are better off calling the `release' member
         function instead of `close' (see below).
            Note that this function may be safely invoked even when the
         precinct is not currently active -- it does nothing unless `state'
         holds a valid precinct address. */
    void close_and_reset();
      /* This function is used only by `kdu_tile::parse_all_relevant_packets'
         and then only if it is asked to parse everything from scratch.  The
         behaviour is similar to calling `close', except that the internal
         `state' record is left with its P flag reset, so that when the
         precinct is reloaded, its packet statistics will be entered into
         the `kd_tile_comp::layer_stats' array -- the caller invariably
         resets this array. */
    void release();
      /* This function should be called once a precinct is no longer
         required, in preference to the `close' function.  If the system is
         running low on resources, the function may unload the precinct from
         memory to recycle its resources.  Alternatively, the function is at
         liberty to leave the precinct in memory, moving it to a list of
         inactive precincts which are subject to unloading at some point in
         the future, when resources may be running low.  Non-addressable or
         cached precincts are unloaded immediately, since they are
         guaranteed never to be re-opened once `release' has been called. */
    void clear();
      /* This function recycles the storage associated with any existing
         precinct and sets the internal `state' variable to 0.  It is called
         when the code-stream management machinery is restarted. */
    bool is_desequenced();
      /* This function is intended for use within
         `kd_packet_sequencer::next_in_sequence', for checking whether
         or not the precinct which is returned by that function needs
         to have its contents read from a compressed data source in
         order to desequence a new packet.  Opening precincts can be a time
         consuming task, especially when there are an enormous number of
         precincts (this can happen when working with huge untiled images,
         especially in geospatial applications).  This function takes
         advantage of the fact that there is no need to actually open a
         precinct which is not already loaded into memory.  The fact that this
         function is called first by the packet desequencer, allows a precinct
         which has already been fully generated to be closed immediately
         during packet generation mode.
            The function works as follows.  If the precinct is already loaded
         in memory, its `sequenced' flag is accessed directly.   Otherwise,
         if the reference `state' variable identifies a valid unique
         address for the precinct, it is known to be addressable and
         therefore must have already been fully desequenced.  Also, if the
         `state' variable indicates that the precinct is unloadable, or that
         it has been closed in packet generation mode, it is interpreted as
         sequenced, in the sense that the application cannot sequence further
         packets for the precinct.  Otherwise, the `state' variable must hold
         0 (see below), from which we deduce that the precinct has not yet
         been sequenced. */
    bool set_address(kd_resolution *res, kdu_coords pos_idx,
                     kdu_long seek_address);
      /* Called from within `kd_packet_sequencer::next_in_sequence', this
         function installs a seek address for a precinct whose packets are
         contiguous within the code-stream and have valid pointer information
         stored in PLT marker segments.  Subsequent calls to `is_sequenced'
         will return true.  The precinct itself may either be open or closed
         when this function is called.  The `pos_idx' argument is provided
         for computing the relevance of this precinct to the application.  It
         holds a relative precinct index, having the same interpretation as
         that passed to `kd_precinct::initialize' and `kdu_precinct_ref::open'.
            It can happen that this function causes the tile, and hence the
         precinct reference, to be destroyed.  This happens if the call to
         this function renders the resolution or tile completely parsed,
         and it is known that the application has no need for any further
         access to the tile.  In this case, and only in this case, the
         function returns false, meaning that the caller must not make
         any further attempt to access the tile or any of its resources. */
  private: // Utility functions
    kd_precinct *instantiate_precinct(kd_resolution *res, kdu_coords pos_idx);
      /* Called from `open' if the `kd_precinct' object is not in memory at
         the time and can legitimately be created at this point. */
  private: // Data
    kdu_long state;
  };
  /* Notes:
        `state' holds 0 if no information is yet available concerning the
     precinct associated with this reference.
        If the precinct is currently active in memory, `state' holds the
     address in memory (pointer) of the `kd_precinct' structure.
        If the precinct is known to be addressable, `state' holds an odd
     integer, equal to 4*A + 2*P + 1, where A is the precinct's
     `unique_address' value and P is a flag, equal to 1 if the precinct
     has been loaded and parsed previously.
        The `unique_address' value has the same definition as the
     corresponding data member in `kd_precinct'.
        This P flag plays the same role as the KD_PFLAG_PARSED bit in
     `kd_precinct::flags'.  It is used to avoid collecting layer size
     statistics twice for the same precinct which could otherwise happen
     when precincts get swapped in and out of memory on demand.
        A value of `state'=1 means that the precinct can be re-instantiated,
     but that it is known to contain no packets whatsoever.
        If a previously active precinct's resources have been recycled and
     re-loading is not possible, `state' is assigned the special value of 3. */

/*****************************************************************************/
/*                               kd_leaf_node                                */
/*****************************************************************************/

struct kd_leaf_node {
    public: // Data
      kd_node *parent; // Every leaf node has a non-leaf parent node
      kd_resolution *resolution; // Always points to the owning resolution
      kdu_dims dims; // Dimensions & location of the image at this node
      kdu_dims region; // See below
      kdu_byte branch_x, branch_y; // See below
      bool is_leaf; // False if this object is the base of `kd_node'
  };
  /* Notes:
        This structure is the base of the richer `kd_node' structure.  The
     key difference is that leaf nodes have no children, so we do not waste
     storage recording pointers to them.
        The `branch_x' and `branch_y' members describe how this node is
     derived from its parent node, if any.  `branch_x' holds 0 if horizontal
     low-pass filtering and decimation were used to obtain the node from its
     parent; it holds 1 if horizontal high-pass filtering and decimation were
     used; it holds 2 if no filtering or decimation were involved in the
     horizontal direction.  `branch_y' plays a similar role in describing the
     vertical filtering and decimation processes.  If `parent' is NULL,
     the `branch_x' and `branch_y' members have no meaning.
        This structure manages two different, yet related regions
     (location, plus dimensions) which deserve some explanation here.
     * The `dims' member serves the usual role of identifying the location and
       dimensions of the image, as it appears on entry into this node,
       during forward transformation (analysis).  For primary nodes,
       these are the dimensions and location of the tile-component-resolution
       within its canvas.
     * `region' identifies a subset of the `dims' region, containing
       all of the node's samples which are involved in the synthesis of the
       current region of interest, as specified in the most recent call to
       `kdu_codestream::apply_input_restrictions'. */

/*****************************************************************************/
/*                                 kd_node                                   */
/*****************************************************************************/

struct kd_node : public kd_leaf_node {
    public: // Member functions
      void adjust_cover(kdu_dims child_cover,
                        int child_branch_x, int child_branch_y);
        /* This function is used to find the `region_cover' members.  In
           a downward sweep through the decomposition branches associated
           with a given resolution level, the `region_cover' members are all
           set to empty regions.  We then assign each leaf node (subband)
           a region cover (not actually recorded in the leaf node) equal to
           its region of interest and work back up the tree from the
           leaves to the primary node of the resolution in question, calling
           this function to grow the region cover so as to encompass the
           smallest region which covers each of its children. */
    public: // Data
      kdu_dims region_cover; // See below
      kdu_dims prec_dims; // Temp storage for calculating precinct dimensions
      kd_leaf_node *children[4]; // Indexed by `LL_BAND' through `HH_BAND'
      kdu_byte num_hor_steps, num_vert_steps; // See below
      kdu_uint16 num_descendant_nodes;
      kdu_uint16 num_descendant_leaves;
      float *bibo_gains; // See below
  };
  /* Notes:
        This object is used to construct descriptions of all non-leaf
     nodes in the DWT decomposition structure.  Each node in the structure
     may be decomposed horizontally, vertically or both.  If it is decomposed
     in the horizontal direction, `children[HL_BAND]' will be non-NULL.  If it
     is decomposed in the vertical direction, `children[LH_BAND]' will be
     non-NULL.  If it is decomposed in both directions, `children[HH_BAND]'
     will also be non-NULL.  If it is decomposed at all, `children[LL_BAND]'
     will always be non-NULL.
        The decomposition structure consists of a collection of primary
     nodes, each of which is embedded within a `kd_resolution' object.
     In the case of the simple Mallat structure, each primary node has
     4 children, with the LL child embedded within the next lower
     resolution object and the remaining children embedded within
     `kd_subband' objects.  In wavelet packet decomposition structures,
     detail subbands produced by a primary node may, themselves, be
     decomposed further into subbands.
        The actual object pointed to by each non-NULL member of the `children'
     array may need to be cast to type `kd_node', rather than `kd_leaf_node'
     if the `kd_leaf_node::is_leaf' member is false.
        The `children[LL_BAND]' entry of a primary node always points to
     the node which is embedded within the next lower resolution
     `kd_resolution' object, except at the lowest resolution of all.
     The lowest `kd_resolution' object's embedded node has only one
     child, with `children[LL_BAND]' pointing to the single (LL) subband
     belonging to that resolution.
        The `num_hor_steps' and `num_vert_steps' members identify the number
     of horizontal and vertical lifting steps associated with the horizontal
     and vertical transform operations applied at this node to produce its
     children.  One or both of these may be 0, either because the transform
     is trivial (lazy wavelet) or because the node is split only in the
     horizontal direction, only in the vertical direction, or is not split
     at all.
        The `num_descendant_nodes' member holds a count of the total number
     of nodes (leaf nodes and non-leaf nodes) which may be reached via the
     `children' array.  The `num_descendant_leaves' member holds a count of
     the number of these descendant nodes which are leaves.  Neither of these
     counts includes the current node.  Clearly there is no need to
     maintain this information with leaf nodes, since the counts would both
     be 0 in this case.
        The `bibo_gains' member point to an array with
     2+`num_hor_steps'+`num_vert_steps' entries.  The first 1+`num_hor_steps'
     entries describe BIBO gains for the horizontal transform performed
     at this node.  The remaining `+`num_vert_steps' entries describe BIBO
     gains for the vertical transform performed at this node.  Within each
     of these 2 sets of BIBO gains the first entry holds the BIBO gain from
     the original input image to the image which is presented at the input
     to this node.  Subsequent entries hold the BIBO gains from the original
     input image to the image which is presented at the output of each
     successive analysis lifting step.  Al BIBO gains are assessed relative to
     the nominal transform normalization, in which reversible lifting steps
     are applied as-is, while irreversible lifting steps are followed by the
     relevant `kd_tile_comp::low_scale' and `kd_tile_comp::high_scale' values.
        The interpretation of `region_cover' is similar to that of
     `kd_leaf_node::region', with a subtle, yet important twist.  In order to
     reconstruct each sample in `region', it is necessary to decode a
     particular set of samples from the subbands which are associated with
     this node.  Each of these subband samples has a mapping onto the
     coordinate system associated with the node, following the usual rules of
     the canvas coordinate system.  `res_cover' holds the smallest region
     which contains all of these mapped subband sample locations, except that
     the LL subband branch is included only if this is not a primary node.
     This ensures that the primary node embedded inside each `kd_resolution'
     object will have a `region_cover' which spans the nominal locations of
     all subband samples of interest, which are represented by codestream
     packets associated with that resolution. */

/*****************************************************************************/
/*                                kd_subband                                 */
/*****************************************************************************/

struct kd_subband : public kd_leaf_node {
    // State structure for the "kdu_subband" interface
  public: // Identification
    kdu_int16 descriptor; // See `cod_params::expand_decomp_bands'
    kdu_byte orientation; // One of LL_BAND, HL_BAND, LH_BAND or HH_BAND.
                     // This is the orientation of the primary subband from
                     // which we are derived within the same resolution level.
    kdu_byte sequence_idx; // Index within the `kd_resolution::subbands' array
                     // This is also the order in which subbands contribute
                     // code-blocks to codestream packets.
    kdu_byte transpose_sequence_idx; // Index of the real subband which appears
                      // to have this `sequence_idx' value when
                      // `kdu_codestream::change_appearance' has been used to
                      // transpose the codestream's appearance.
  public: // Dimensions and Parameters
    kdu_byte epsilon; // Ranging parameter.
    kdu_byte K_max; // Maximum magnitude bit-planes, not including ROI upshift
    kdu_byte K_max_prime; // Max magnitude bit-planes, including ROI upshift
    float delta; // Step size, normalized for image data range of -0.5 to 0.5
    float G_b; // Subband energy gain factor.
    float W_b; // Subband amplitude weight; needs squaring to get energy weight
    float roi_weight; // Extra amplitude weight for ROI foreground blocks.
    kdu_dims block_partition; // Holds the coding origin and dimensions.
    kdu_dims block_indices; // Holds the range of valid block indices.
    kdu_dims region_indices;  // Same as `block_indices', but restricted to
                              // blocks which overlap the region of interest.
    kdu_coords blocks_per_precinct; // precinct size / block size
    kdu_coords log2_blocks_per_precinct; // log2 of precinct size/block size
  };
  /* Notes:
        It may come as a surprise to notice that this object has no
     direct connection to code-blocks.  To access a code-block, the object
     first determines the precinct to which it belongs, creating that
     precinct if necessary; this, in turn, creates its precinct-bands and
     the associated code-blocks.  In this way, no memory whatsoever need be
     dedicated to code-blocks which lie in unaccessed precincts. */

/*****************************************************************************/
/*                              kd_resolution                                */
/*****************************************************************************/

struct kd_resolution { // State structure for the "kdu_resolution" interface
  public: // Member functions
    kd_resolution()
      {
        codestream = NULL; tile_comp = NULL; rescomp = NULL;
        precinct_refs = NULL; subbands = subband_handle = NULL; num_subbands=0;
        num_intermediate_nodes = 0; intermediate_nodes = NULL;
        can_flip = true;  node.bibo_gains = NULL;
      }
    ~kd_resolution()
      {
        if (node.bibo_gains != NULL) delete[] node.bibo_gains;
        for (int n=0; n < (int) num_intermediate_nodes; n++)
          if (intermediate_nodes[n].bibo_gains != NULL)
            delete[] intermediate_nodes[n].bibo_gains;
        if (precinct_refs != NULL) delete[] precinct_refs;
        if (subband_handle != NULL) delete[] subband_handle;
        if (intermediate_nodes != NULL) delete[] intermediate_nodes;
      }
    void build_decomposition_structure(kdu_params *coc, kdu_kernels &kernels);
      /* This function creates the `subbands' and `intermediate_nodes'
         arrays and fills in all the relevant pointers.  It uses `kernels'
         to create and fill in the `kdu_node::hor_bibo_gains' and
         `kdu_node::vert_bibo_gains' arrays, where required. */
    void complete_initialization();
      /* Called from within `kd_tile::initialize' to finish the initialization
         process, after all of the key parameters have been set.  Currently,
         the only task performed here is the initialization of the
         `max_blocks_per_precinct' member. */
  public: // Links and Identification
    kd_codestream *codestream;
    kd_tile_comp *tile_comp;
    kd_global_rescomp *rescomp;
    kdu_byte res_level; // Runs from 0 (LL band) to `num_dwt_levels'
    kdu_byte dwt_level; // Runs from `num_dwt_levels' (res level 0 and 1) to 1
    kdu_byte hor_depth; // From relevant entry of `kd_comp_info::hor_depth'
    kdu_byte vert_depth; // From relevant entry of `kd_comp_info::vert_depth'

  public: // Embedded node
    kd_node node; // Each resolution is a primary node in the decomposition

  public: // Dimensions and Parameters
    kdu_dims precinct_partition; // Holds the coding origin and dimensions.
    kdu_dims precinct_indices; // Holds the range of valid precinct indices.
    kdu_dims region_indices; // Holds range of indices for precincts whose
                             // code-blocks contribute to region of interest.
  public: // Derived quantities
    int max_blocks_per_precinct; // See discussion under `kd_precinct_band'
    bool propagate_roi; // True if ROI masks to be propagated to descendants

    bool can_flip; // Set to false if this level contains subband decomposition
                   // styles which are incompatible with view flipping.

  public: // Arrays
    kdu_byte num_subbands; // Excludes the LL band unless `res_level'=0
    kdu_byte num_intermediate_nodes; // Enough for all non-leaf nodes descended
                                 // from the primary node in this object.
    kd_node *intermediate_nodes; // Array with `num_intermediate_nodes' entries
    kd_precinct_ref *precinct_refs;
    kd_subband *subbands; // Points to `subband_store' or `subband_handle'.
    kd_subband *subband_handle; // For dynamically allocated subbands
    kd_subband subband_store[3]; // Avoid dynamic allocation for common case

  public: // Flags and Other State Variables
    kdu_coords current_sequencer_pos; // Used by `kd_packet_sequencer'.
    kdu_coords saved_current_sequencer_pos; // For saving the sequencer state
  };
  /* Notes:
        The `current_sequencer_pos' coordinates are used to sequence
     spatially oriented packet progressions.  They maintain the
     indices (starting from [0,0], rather than `precinct_indices.pos')
     of the precinct which is currently being sequenced.
        The `region_indices' member holds the smallest range of precinct
     indices, such that the code-blocks contained within these precincts
     are sufficient to reconstruct all samples inside `node.region'.
     Equivalently, the range in `precinct_indices' yields the set of
     precincts which intersect with `node.region_cover'.
        It is worth noting that the area of intersection between each
     precinct's region on the resolution and the `node.region_cover' region
     is used to compute the information returned by
     `kdu_resolution::get_precinct_relevance'. */

/*****************************************************************************/
/*                             kd_header_in                                  */
/*****************************************************************************/

class kd_header_in {
  /* Manages the reading of packet header bytes from the code-stream, along
     with bit stuffing and unpacking. */
  public: // Member functions
    kd_header_in(kd_input *source)
      { this->source = source; bits_left=0; byte=0; header_bytes=0; }
    int get_bit() // throws its own "this" pointer if the source is exhausted.
      {
        if (bits_left==0)
          {
            bits_left = (byte==0xFF)?7:8;
            if (!source->get(byte))
              { bits_left = 0; throw this; }
            header_bytes++;
          }
        bits_left--;
        return (byte >> bits_left) & 1;
      }
    kdu_uint32 get_bits(int num_bits) // thorws(kd_header_in *)
      { kdu_uint32 result = 0;
        while (num_bits > 0)
          {
            if (bits_left==0)
              {
                bits_left = (byte==0xFF)?7:8;
                if (!source->get(byte))
                  { bits_left = 0; throw this; }
                header_bytes++;
              }
            int xfer_bits = (num_bits<bits_left)?num_bits:bits_left;
            bits_left -= xfer_bits;
            num_bits -= xfer_bits;
            result <<= xfer_bits;
            result |= ((byte >> bits_left) & ~(0xFF << xfer_bits));
          }
        return result;
      }
    int finish()
      { // Call this when the header is all read, to consume any stuffing byte
        // Returns total header length
        if ((bits_left == 0) && (byte == 0xFF))
          {
            bits_left = 7;
            if (!source->get(byte))
              throw this;
            header_bytes++;
          }
        return header_bytes;
      }
  private: // Data
    kd_input *source; // Provides the input mechanism.
    kdu_byte byte;
    int bits_left;
    int header_bytes;
  };

/*****************************************************************************/
/*                                kd_header_out                              */
/*****************************************************************************/

class kd_header_out {
  /* Manages the simulated and real output of packet header bytes. */
  public: // Member functions
    kd_header_out(kdu_output *out=NULL) // Null for simulating header
      { byte = 0; bits_left = 8; completed_bytes = 0; this->out = out; }
    void put_bit(int bit)
      { // Output a header bit
        assert(bit == (bit & 1));
        if (bits_left == 0)
          { if (out != NULL) out->put(byte);
            completed_bytes++;
            bits_left = (byte==0xFF)?7:8;
            byte = 0; }
        byte += byte + bit;
        bits_left--;
      }
    void put_bits(kdu_int32 val, int num_bits)
      { // Output the least significant `num_bits' of `val'
        while (num_bits > 0)
          put_bit((val >> (--num_bits)) & 1);
      }
    int finish()
      { // Returns the total number of bytes consumed by the packet header.
        if (bits_left < 8)
          {
            byte <<= bits_left;
            if (out != NULL)
              out->put(byte);
            completed_bytes++;
            if (byte == 0xFF)
              { // Need the stuffing byte.
                if (out != NULL)
                  out->put((kdu_byte) 0);
                completed_bytes++;
              }
          }
        return completed_bytes;
      }
  private: // Data
    kdu_byte byte;
    int bits_left;
    int completed_bytes;
    kdu_output *out;
  };

/*****************************************************************************/
/*                                 kd_block                                  */
/*****************************************************************************/

class kd_block {
  /* For a very small size (only 24 bytes on 32-bit machines), this class
     defines a lot of member functions!. */
  // --------------------------------------------------------------------------
  public: // Lifecycle member functions
    void cleanup(kd_buf_server *buf_server)
      {
        while ((current_buf=first_buf) != NULL)
          { first_buf=current_buf->next;  buf_server->release(current_buf); }
        set_discard(); // Discard from any packets parsed in the future
      }
    kd_code_buffer *cleanup()
      { // As above, but returns the linked list of buffers
        kd_code_buffer *result = first_buf;  first_buf = current_buf = NULL;
        set_discard();
        return result;
      }

  // --------------------------------------------------------------------------
  public: // Packet manipulation member functions
    int parse_packet_header(kd_header_in &head, kd_buf_server *buf_server,
                            int layer_idx); // Throws (kdu_uint16 code)
      /* Parses bits in the packet header corresponding to the current
         block for the current layer (this must be the next layer for which
         information has not yet been parsed).  The function returns the
         number of new body bytes contributed by the code-block in this layer
         and also stores this value internally to be used in subsequent calls
         to the `read_body_bytes' member function.
            The `kd_header_in' object may throw an exception if the packet
         data terminates unexpectedly.  Moreover, to assist in locating and
         possibly recovering from errors, the present function may throw a
         16-bit unsigned integer exception code, which is either the value of
         an illegal marker code encountered in the packet header, or one of
         the special exception codes, KDU_EXCEPTION_PRECISION or
         KDU_EXCEPTION_ILLEGAL_LAYER. */
    void read_body_bytes(kd_input *source, kd_buf_server *buf_server,
                         bool in_memory_source)
      { /* This function should be called after parsing the header of a
           packet.  It reads the code-bytes contributed by the code-block to
           this packet.  If `in_memory_source' is true, only the address of
           the code-bytes is recorded, and then only if the length of the
           code-byte segment is non-zero. */
        if (this->temp_length > 0)
          {
            if (num_passes == 255)
              { // Read and discard the code-bytes.
                assert(first_buf == NULL);  source->ignore(this->temp_length);
              }
            else if (in_memory_source)
              {
                kdu_byte *addr;
                this->num_bytes += source->pseudo_read(addr,this->temp_length);
                put_address(addr,buf_server);
              }
            else
              this->num_bytes +=
                source->read(current_buf,buf_pos,buf_server,this->temp_length);
            this->temp_length = 0;
          }
      }
    void retrieve_data(kdu_block *block, int max_layers, int discard_passes,
                       bool in_memory_source);
      /* Retrieves compressed code-bytes, number of coding passes and codeword
         segment lengths from the current code-block and stores them in the
         `block' object.  All fields which are common for compression and
         decompression have already been filled out by the caller.  The
         byte buffer is allocated so as to be large enough to allow a
         2 byte marker code to be appended to the compressed data,
         if desired -- see declaration of "kdu_subband::open_block".
            The `max_layers' argument identifies the maximum number of
         quality layers for which code-block contributions are to be
         retrieved.
            The `discard_passes' argument indicates the number of additional
         coding passes to be removed, if any.
            If `in_memory_source' is true, the layer code bytes are not
         stored in the `kd_code_buffer' list embedded inside `block'.  Instead,
         the `kd_code_buffer' list stores only the header information for
         each layer, along with the address of the code bytes, which reside
         in the memory block managed by an in-memory compressed data source
         (one which offers the `KDU_SOURCE_CAP_IN_MEMORY') capability.  In
         this case, we retrieve the address and then perform a simple memory
         block copy operation to efficiently retrieve the code bytes for
         each layer. */
    void store_data(kdu_block *block, kd_buf_server *buf_server);
      /* Stores compressed code-bytes, pass length and pass slope information
         from the `block' object into the current code-block.  This function
         is invoked by "kdu_subband::close_block" when the containing
         codestream object was not constructed for input.  Also reflects
         the `msbs_w' value into higher nodes in the tag tree. */
    void store_data(kdu_block *block, kd_thread_buf_server *thread_buf_server);
      /* Same as the above function, but uses `kd_thread_buf_server::get'
         rather than `kd_buf_server::get' to get new code buffers.  This
         version is used to save code-block state in multi-threaded
         environments so as to minimize cache coherency bottlenecks and the
         need to lock shared resources. */
    void transfer_data(kd_block &src)
      { /* Like `store_data', but gets results from another block.  Exploits
           the layout of the object to copy only whole pointers or whole
           32-bit words, even where these represent multiple fields. */
        current_buf = first_buf = src.first_buf;
        *((kdu_int32 *) &buf_pos) = *((kdu_int32 *) &src.buf_pos);
        src.current_buf = src.first_buf = NULL;
      }
    bool trim_data(kdu_uint16 slope_threshold, kd_buf_server *buf_server);
      /* This function serves two purposes: it helps reduce the amount of
         compressed data which must be buffered in memory while compressing
         an image; and it allows individual block contributions to be
         discarded during packet construction in order to tightly fit the
         generated bit-rate to the target compressed bit-rate.
            The function sets all coding pass slopes which are less than or
         equal to the supplied slope threshold to 0 and then discards all
         code bytes which could not possibly be included in the final
         code-stream, as a result of this modification.
            If anything was discarded, the function returns true.  Otherwise,
         it returns false. */
    int start_packet(int layer_idx, kdu_uint16 slope_threshold);
      /* Call this function only during rate allocation or packet generation.
         Before the first call to this function, you must reset the tag tree
         (see the "reset_output_tree" member function).  If you are calling
         the function multiple times with the same layer for rate allocation
         purposes, you must use "save_output_tree" and "restore_output_tree"
         as described below.
            The function leaves behind information about the number of new
         coding passes and the number of new code bytes which would be
         contributed by this code-block if the supplied threshold were actually
         used.  The final contributing coding pass must have a non-zero
         distortion-length slope which is strictly greater than the slope
         threshold.  It also updates the tag tree structure to reflect any
         new knowledge about the first layer to which this code-block would
         contribute.  This information is picked up by "write_packet_header".
            The function's return value is the total number of new code-bytes
         contributed by the block.  This allows the PCRD-opt algorithm to
         determine roughly the right slope threshold before committing to the
         work of simulating packet header generation. */
    void write_packet_header(kd_header_out &head, int layer_idx,
                             bool simulate);
      /* Call this function only after a `start_packet' call.  It writes
         (or simulates the writing of) packet header information corresponding
         to the code-block inclusion information set up by `start_packet'.
         If `simulate' is true, some internal state adjustments are delayed
         until the next call to "save_output_tree", so multiple calls to
         "start_packet" and "write_packet_header" may be issued with the same
         layer index to determine an optimal slope threshold.  Otherwise,
         if `simulate' is false, the internal state information is fully
         updated and the "save_output_tree" and "restore_output_tree" functions
         need not and should not be called. */
    void write_body_bytes(kdu_output *dest);
      /* This function should be called after a non-simulated
         "write_packet_header" call to output the newly included body bytes.
         The actual number of bytes to be written was left behind by
         the preceding "start_packet" call in the `temp_length' field. */
  // --------------------------------------------------------------------------
  public: // Static functions used to operate on tag-trees built from kd_block
    static kd_block *build_tree(kdu_coords size, kdu_byte * &mem_block);
      /* Builds a full tag tree returning a pointer to the upper left hand
         corner leaf node.  Apart from the `up_down' navigation pointers,
         all fields are initialized to 0.  This is appropriate for tag tree
         decoding, but encoders will need to call "reset_output_tree" and
         use "start_packet" to install appropriate node values.
            The tag tree is not allocated from the heap.  Instead, it is
         allocated from an existing memory block, identified by the `mem_block'
         argument.  This memory block is assumed to be large enough to hold
         the array of `kd_block' structures.  Upon return, the `mem_block'
         value is advanced by an amount equal to the size of the tag tree
         array. */
    static void reset_output_tree(kd_block *tree, kdu_coords size);
      /* Call this function immediately before simulating or actually
         generating any packet data for the relevant code-blocks.  Once
         the PCRD-opt algorithm has completed, you should call this function
         a second time to prepare for actual packet output.  The function
         initializes the threshold state variables, `layer_w_bar' and
         `msbs_w_bar' to 0, and the value state variables, `layer_w' and
         `msbs_w' to their maximum representable values.  The succession of
         "start_packet" calls will update the value variables `layer_w'
         and `msbs_w' in such a way that the tag tree coding process runs
         correctly in each subsequent call to "write_packet_header". */
    static void save_output_tree(kd_block *tree, kdu_coords size);
      /* Call this only during rate allocation, before moving on to a new
         layer.  The function saves the current state (`msbs_wbar' and
         `layer_wbar') of each node in the tag tree.  For leaf nodes, it also
         commits the number of new coding passes found in the last call to
         "start_packet". */
    static void restore_output_tree(kd_block *tree, kdu_coords size);
      /* Call this during rate allocation before supplying a new slope
         threshold for the same layer index, in a new "start_packet" call.
         The function restores the `msbs_w_bar' and `layer_w_bar' values
         previously saved by "save_output_tree". */
  // --------------------------------------------------------------------------
  public: // State identification functions
    void set_discard()
      { /* Use this function only with input-oriented operations.  It informs
           the object that none of the code-blocks associated with this
           block will ever be accessed, so that it is free to skip over them
           when reading a packet.  This can save a lot of memory if you have
           a defined region of interest for decompression. */
        num_passes = 255;
      }
    bool empty()
      { // Returns true if the block has no code-bytes yet.
        return first_buf == NULL;
      }
    void set_modes(int modes)
      {
        assert(modes == (modes & 0xFF));
        this->modes = (kdu_byte) modes;
      }
  // --------------------------------------------------------------------------
  private: // Convenience functions for transferring data to/from code buffers.
    void start_buffering(kd_buf_server *buf_server) // Call before `put_byte'
      { assert(first_buf == NULL);
        first_buf = current_buf = buf_server->get(); buf_pos = 0; }
    void start_buffering(kd_thread_buf_server *buf_server)
      { assert(first_buf == NULL);
        first_buf = current_buf = buf_server->get(); buf_pos = 0; }
    void put_byte(kdu_byte val, kd_buf_server *buf_server)
      {
        assert(current_buf != NULL);
        if (buf_pos==KD_CODE_BUFFER_LEN)
          { buf_pos=0; current_buf=current_buf->next=buf_server->get(); }
        current_buf->buf[buf_pos++] = val;
      }
    void put_word(int val, kd_buf_server *buf_server)
      {
        buf_pos += (buf_pos & 1); // Align the buffer pointer
        if (buf_pos > (KD_CODE_BUFFER_LEN-2))
          { buf_pos=0; current_buf=current_buf->next=buf_server->get(); }
        ((kdu_uint16 *)(current_buf->buf+buf_pos))[0] = (kdu_uint16) val;
        buf_pos += 2;
      }
    void put_address(kdu_byte *addr, kd_buf_server *buf_server)
      {
        buf_pos += (-buf_pos)&(KDU_POINTER_BYTES-1); // Align the buf pointer
        if (buf_pos > (KD_CODE_BUFFER_LEN-KDU_POINTER_BYTES))
          { buf_pos=0; current_buf=current_buf->next=buf_server->get(); }

        ((kdu_byte **)(current_buf->buf+buf_pos))[0] = addr;
        buf_pos += KDU_POINTER_BYTES;
      }
    void put_byte(kdu_byte val, kd_thread_buf_server *buf_server)
      {
        assert(current_buf != NULL);
        if (buf_pos==KD_CODE_BUFFER_LEN)
          { buf_pos=0; current_buf=current_buf->next=buf_server->get(); }
        current_buf->buf[buf_pos++] = val;
      }
    void put_word(int val, kd_thread_buf_server *buf_server)
      {
        buf_pos += (buf_pos & 1); // Align the buffer pointer
        if (buf_pos > (KD_CODE_BUFFER_LEN-2))
          { buf_pos=0; current_buf=current_buf->next=buf_server->get(); }
        ((kdu_uint16 *)(current_buf->buf+buf_pos))[0] = (kdu_uint16) val;
        buf_pos += 2;
      }
    void put_address(kdu_byte *addr, kd_thread_buf_server *buf_server)
      {
        buf_pos += (-buf_pos)&(KDU_POINTER_BYTES-1); // Align the buf pointer
        if (buf_pos > (KD_CODE_BUFFER_LEN-KDU_POINTER_BYTES))
          { buf_pos=0; current_buf=current_buf->next=buf_server->get(); }

        ((kdu_byte **)(current_buf->buf+buf_pos))[0] = addr;
        buf_pos += KDU_POINTER_BYTES;
      }
    kdu_byte get_byte()
      {
        if (buf_pos==KD_CODE_BUFFER_LEN)
          { buf_pos=0; current_buf=current_buf->next;
            assert(current_buf != NULL); }
        return current_buf->buf[buf_pos++];
      }
    int get_word()
      {
        buf_pos += (buf_pos & 1); // Align the buffer pointer
        buf_pos += 2; // Advance the buffer pointer
        if (buf_pos > KD_CODE_BUFFER_LEN)
          { buf_pos=2; current_buf=current_buf->next;
            assert(current_buf != NULL); }
        return ((kdu_uint16 *)(current_buf->buf+buf_pos))[-1];
      }
    kdu_byte *get_address()
      {
        buf_pos += (-buf_pos)&(KDU_POINTER_BYTES-1); // Align the buf pointer
        buf_pos += KDU_POINTER_BYTES; // Advance the buffer pointer
        if (buf_pos > KD_CODE_BUFFER_LEN)
          { buf_pos=KDU_POINTER_BYTES; current_buf=current_buf->next;
            assert(current_buf != NULL); }
        return ((kdu_byte **)(current_buf->buf+buf_pos))[-1];
      }
    void skip_word()
      {
        buf_pos += 2 + (buf_pos & 1); // Align and advance the buffer pointer
        if (buf_pos > KD_CODE_BUFFER_LEN)
          { buf_pos=2; current_buf=current_buf->next;
            assert(current_buf != NULL); }
      }
    void skip_address()
      {
        buf_pos += (-buf_pos)&(KDU_POINTER_BYTES-1); // Align the buf pointer
        buf_pos += KDU_POINTER_BYTES; // Advance the buffer pointer
        if (buf_pos > KD_CODE_BUFFER_LEN)
          { buf_pos=KDU_POINTER_BYTES; current_buf=current_buf->next;
            assert(current_buf != NULL); }
      }
  // --------------------------------------------------------------------------
  private: // Data (lots of unions to make it as small as possible!)
    union {
      kd_code_buffer *first_buf;
      kdu_uint16 save_layer_w; // Used for saving non-leaf node state
      };
    union {
      kd_code_buffer *current_buf;
      kdu_uint16 save_layer_wbar; // Used for saving_non-leaf node state
      };
    union {
      kdu_byte buf_pos; // Location of next read/write byte in `current_buf'.
      kdu_byte save_msbs_wbar; // Used for saving non-leaf node state
      };
    kdu_byte msbs_w;
    kdu_byte num_passes;
    kdu_byte pass_idx;
    union {
      kdu_uint16 layer_w; // Used only when `pass_idx'=0
      kdu_uint16 body_bytes_offset; // Used in packet output if `pass_idx' > 0
      kdu_byte save_beta; // Used during output simulation if `pass_idx' > 0
      };
    union {
      kdu_uint16 layer_wbar;
      kdu_uint16 num_bytes; // Used only for input operations
      kdu_byte pending_new_passes; // Used only for output operations
      };
    union {
      kdu_byte msbs_wbar;
      kdu_byte beta;
      };
    kdu_byte modes;
    kdu_uint16 temp_length; // Temporary storage of length quantities.
    kd_block *up_down;
  };
  /* Notes:
        This structure has been designed to provide sufficient state
     information for each code-block to perform packet header parsing as
     well as block encoding and decoding.  For coding purposes, the information
     represented by the structure is generally transferred to or from a
     much less tightly packed representation for speed of execution of the
     block coding routines.  As it stands, the total size of this structure
     should be 24 bytes on a 32-bit architecture. An array of code-blocks is
     maintained by each precinct-band.  The array contains one entry for
     every block in the relevant precinct-band, but also contains additional
     entries to be used as tag-tree nodes.  The total number of such extra
     nodes is about 1/3 the number of code-blocks and should not be a
     significant cause of concern for overall memory consumption. There is
     also substantial benefit to be had by locating the tag tree nodes in
     the same contiguous memory block.  Notice that the structure contains
     no reference back to the precinct-band to which it belongs.  When
     necessary, this reference must be supplied separately to interested
     functions. One consequence of this memory saving step is that the
     object's destructor cannot do anything useful.  It only checks to make
     sure that the explicit `cleanup' function has already been called.
        If a block is to be discarded (since it does not intersect with a
     user-specified region of interest), the `num_passes' field will hold
     the otherwise impossible value, 255.
        The `first_buf', `current_buf' and `buf_pos' fields constitute the
     compressed data buffer management mechanism, which is used to store all
     quantities whose length is unpredictable a priori. In particular, a
     linked list of `kd_code_buffer' structures is provided, which grows on
     demand through access to the codestream object's embedded `buffering'
     object.  Data is stored in these buffers in the following formats:
           * Input Format -- [<layer #> (<#bytes> <#passes> ...)] data [<...
             That is, for each layer to which the code-block makes a non-empty
             contribution, we record first the layer number (2 bytes) and
             then one or more #bytes/#passes pairs.  #bytes is represented
             using 2 bytes, while #passes is represented using only 1 byte.
             There is one such 3 byte pair for each layer and one additional
             3 byte pair for each codeword segment terminating pass which
             is not the final pass contributed in the layer.  In this way,
             sufficient information is provided to recover all termination
             lengths, as well as to build layer associations.  The most
             significant bit of the #bytes field is used as a marker to
             indicate whether or not this is the last such field prior to
             the appearance of the code bytes contributed by that layer.  A
             1 means that there are more fields to go before the code bytes
             appear.
           * In-memory Format --[<layer #>(<#bytes> <#passes>...)] <addr> [<...
             This format is used as an alternative to the Input Format above,
             in the special case when the compressed data source supports the
             `KDU_SOURCE_CAP_IN_MEMORY' capability, so that
             `kd_codestream::in_memory_source' is true.  In this case, the
             "data" field is replaced by the address where the data can be
             found in the memory block managed by the compressed data source.
             Moreover, if the number of code-bytes associated with a layer
             is zero, the <addr> field is missing.
           * Output Format -- [<RD slope> <#bytes>] ... [<RD ...] data
             That is, all of the header information for all coding passes
             appears up front, followed by all of the code bytes associated
             with those coding passes.  The reason for putting all the
             header information up front is that it facilitates the
             identification of appropriate layer contributions by the
             rate control algorithm.  Each coding pass has a 4 byte header,
             with 2 bytes for the distortion-length slope value and 2 bytes
             for the number of code-bytes for the pass.
     The particular format which is used to store data depends upon the
     member functions which are used, since these implicitly determine
     whether the codestream object is being used for input or output.
        The `modes' field holds the block coder mode fields.  Some of these
     are required even for correct packet header parsing, regardless of whether
     the block is to be actually decoded.
        `msbs_w' and `msbs_wbar' are the state information required
     for tag-tree coding of the number of missing MSB's for a code-block
     which is about to make its first non-empty contribution to the
     code-stream. Similarly, `layer_w' and `layer_wbar' hold the state
     information required for tag-tree coding of the layer index in which a
     code-block first contributes to the code-stream.  The operation of
     tag-tree coding and the `w' and `wbar' terminology are explained in
     Section 8.4 of the book by Taubman and Marcellin.
        The `beta' field shares storage with `msbs_wbar'.  This is not
     required until the number of missing MSB's has been encoded or decoded,
     at which point we initialize `beta' to 3 (see the JPEG2000 standard or
     Section 12.5.4 of the book by Taubman and Marcellin).  A useful
     consequence of this behaviour is that `beta' may be tested at any
     point of the packet header encoding or decoding process to determine
     whether or not the code-block has yet contributed to the code-stream
     -- the value will be non-zero if and only if it has done so.
        The `num_bytes' field shares storage with `layer_wbar'.  It
     identifies the total number of bytes included by the code-block in
     all packets read or written so far.  The value is taken to be 0 if
     the code-block has not yet contributed to any packets (`beta'=0),
     during which time the `layer_wbar' state variable is in use.
        The `up_down' field is used to efficiently navigate the tag-tree
     structure.  When the tag tree is not actually being navigated, the
     field points up to the current node's parent in the tag tree.  The
     root mode is readily identified by the fact that it has a NULL `up_down'
     pointer. During navigation, the tag-tree coder first walks from a
     leaf node up to the root, modifying the `up_down' pointers as it goes
     so that each successive parent's `up_down' field points bacl down to the
     child from which the parent was reached.  The coder then walks back down
     from the root to the original leaf node, restoring the `up_down' fields to
     their original `up' state. */

/*****************************************************************************/
/*                             kd_precinct_band                              */
/*****************************************************************************/

struct kd_precinct_band {
  /* Used to store state information for an element of the precinct partition
     within a single subband.  Once we get down to precincts and ultimately
     code-blocks, we have to be careful not to waste too much memory, since
     an image may have a very large number of these.  Moreover, it may be
     necessary to maintain tag-trees and other state information even for
     precincts which do not lie within our region of interest, so that the
     relevant packet headers can be parsed -- otherwise, we may have no way
     of knowing how to skip over packets which are not interesting to us. */
    kd_subband *subband;
    kdu_dims block_indices; /* Range of code-block indices for this precinct.
        This is a subset of the `block_indices' specified by the `subband'. */
    kd_block *blocks;
  };
  /* Notes:
     In order to make the representation of this object as efficient as
     possible, the two tag trees and all additional information required to
     correctly parse packet headers is maintained within the code-block
     array itself.  The `blocks' array consists of one element for each
     code-block, organized in raster fashion, plus additional elements to
     represent higher nodes in the tag-tree structure.  See the comments
     associated with the definition of `kd_block' for more information on
     this.
        In practice, the `blocks' array is not allocated separately on the
     heap.  Instead, the block of memory allocated to hold the `kd_precinct'
     structure is augmented by an amount sufficient to hold all of the
     code-blocks for all of the subbands in the precinct.  This is
     accomplished by the custom `kd_precinct::new' operator, which is
     informed of the maximum number of code-blocks required by any precinct
     within the containing `kd_resolution' object.  This value is maintained
     by the `kd_resolution::max_blocks_per_precinct" data member.  The
     `blocks' fields are initialized to the NULL state by the
     `kd_precinct::initialize' member function and are later set to point
     into appropriate locations in the precinct's memory block during
     tag-tree construction. */

/*****************************************************************************/
/*                                kd_precinct                                */
/*****************************************************************************/

  /* Precincts and all of their subordinate entities (precinct-bands and
     code-blocks) are maintained by a code-stream wide precinct server
     object, which allows for efficient allocation and recycling of the
     memory resources. The constructor is invoked from within the server,
     while the `initialize' and `release' functions are invoked from
     within the `kd_precinct_ref' class which manages references to
     precincts from within the `kd_resolution' object. */

// Interpretation of the following flags is given with the `flags' member:
#define KD_PFLAG_GENERATING  0x0001
#define KD_PFLAG_CORRUPTED   0x0002
#define KD_PFLAG_DESEQUENCED 0x0004
#define KD_PFLAG_ADDRESSABLE 0x0008
#define KD_PFLAG_RELEASED    0x0010
#define KD_PFLAG_INACTIVE    0x0020
#define KD_PFLAG_RELEVANT    0x0040
#define KD_PFLAG_SIGNIFICANT 0x0080
#define KD_PFLAG_PARSED      0x0100

struct kd_precinct {
  public: // Member functions
    void initialize(kd_resolution *resolution, kdu_coords pos_idx);
      /* Called from within `kd_precinct_ref::open' to configure a newly
         allocated (or recycled) object.  `pos_idx' holds the horizontal and
         vertical position of the precinct (starting from 0) within the array
         of precincts for the relevant resolution level.  Thus, `pos_idx'
         holds a relative index, rather than an absolute index. */
    void activate();
      /* This function is called from within `kd_precinct_ref::open' to
         re-activate a previously released precinct, which has not actually
         been unloaded from memory.  Substantially less work is required in
         this case, and the function must be careful to respect the current
         contents of the precinct's code-blocks.  The function sets the
         `num_required_layers' field for this activation (may change between
         successive `kd_tile::open' calls) and also sets the
         `num_outstanding_blocks' field to the total number of code-blocks
         which lie within the current region of interest. */
    void closing();
      /* Called from within `kd_precinct_ref::close' immediately prior to
         recycling the storage associated with the precinct, its precinct-bands
         and their code-blocks.  This function should release any resources
         which are specific to this particular precinct.  These resources
         include any non-NULL `layer_bytes' array and any code-byte buffers.
         Note that the `kd_precinct_ref::close' function may be called
         indirectly while opening a different precinct, if the current
         precinct has already been released.  This enables open calls to
         deal with resource limitations by unloading precincts which are not
         currently active. */
    void release()
      /* Used only for input codestream objects, this function is called once
         all code-blocks within the current region of interest have been
         consumed.  The function sets `num_outstanding_blocks' to 0, although
         in most cases this function will be called because
         `num_outstanding_blocks' has just become 0 (only exception is when
         a tile is being closed).  Also sets `released' to true.  If the
         KD_PFLAG_DESEQUENCED flag is false, the function returns without
         modifying the precinct in any other way.
            The `kd_precinct_ref::release' function is called to formally
         release the precinct's resources (this might not unload them from
         memory) if the codestream mode is non-persistent or if the precinct
         is addressable.  In the latter case, the precinct's contents can
         be reloaded from the compressed source if necessary.
            This function shall not be used with codestream objects
         opened for output or for interchange (no compressed data source and
         no compressed data target). */
      {
        assert(resolution->codestream->in != NULL);
        num_outstanding_blocks = 0;
        flags |= KD_PFLAG_RELEASED;
          // released = true;
        if ((flags & KD_PFLAG_ADDRESSABLE) ||
            ((flags & KD_PFLAG_DESEQUENCED) &&
                // if (addressable ||
                //     (desequenced &&
             !resolution->codestream->persistent))
          ref->release(); // May unload us from memory!
      }
    void finished_desequencing()
      /* Called once we can be sure that no more packets will be available
         for this precinct.  In practice, the function is called when the
         last packet has been desequenced or when the containing tile has
         been completely read.  If no packets at all have been desequenced for
         the precinct when this function is called (must have been invoked
         from within `kd_tile::finished_reading'), the precinct is marked
         as addressable, with an address of 0.  This is because it is safe
         to unload such a precinct and then reload it again regardless of
         the services offered by the compressed data source -- there is no
         data actually available from the compressed source for the
         precinct anyway, so reloading is a no-op.
            It should be noted that this function may unload the precinct
         from memory if it is determined that all code-blocks in the
         region of interest (there may be none) have already been consumed. */
      {
        if (flags & KD_PFLAG_DESEQUENCED)
          return;
        flags |= KD_PFLAG_DESEQUENCED;
          // desequenced = true;
        if (next_layer_idx == 0)
          {
            flags |= KD_PFLAG_ADDRESSABLE;
              // addressable = true;
            unique_address = 0;
          }
        if (num_outstanding_blocks == 0)
          release();
      }
    bool desequence_packet()
      /* This function should be called when a non-NULL `kd_precinct_ref'
         pointer is returned by `kd_packet_sequencer::next_in_sequence' and
         its `kd_precinct_ref::is_sequenced' function returns false.
         The function invokes `read_packet' and, if successful, increments
         the precinct's `next_layer_idx' field and the containing tile's
         `num_sequenced_packets' field.
            The function returns false if `read_packet' returns false,
         in which case, more tile-parts must be read (if there are any) to
         recover the information required to desequence the packet.
            Before returning true, the function checks to see whether all
         packets for the tile have been sequenced.  If so, the tile's
         `finished_reading' function is called. */
      {
        kd_tile *tile = resolution->tile_comp->tile;
        assert(!(flags & (KD_PFLAG_DESEQUENCED | KD_PFLAG_ADDRESSABLE)));
        if (!read_packet()) return false;
        next_layer_idx++;
        if (next_layer_idx == tile->num_layers)
          finished_desequencing();
        tile->next_input_packet_num++;
        if ((flags & KD_PFLAG_RELEVANT) &&
            (next_layer_idx <= tile->max_relevant_layers))
          {
            tile->sequenced_relevant_packets++;
            if (tile->sequenced_relevant_packets==tile->max_relevant_packets)
              tile->finished_reading();
          }
        return true;
      }
    void load_required_packets()
      /* If the precinct is addressable, this function uses its
         `unique_address' to position the compressed data source at the
         beginning of the precinct's first packet; it then loads all required
         packets from the compressed source.  Otherwise, the function does
         nothing at all -- reading is presumed to have occurred during packet
         desequencing. */
      {
        if ((!(flags & KD_PFLAG_ADDRESSABLE)) ||
            (unique_address == 0) || (num_packets_read != 0))
          return;
        assert((flags & KD_PFLAG_DESEQUENCED) &&
               (next_layer_idx >= required_layers));
        resolution->codestream->in->seek(unique_address);
        while (num_packets_read < required_layers)
          if (!read_packet())
            break;
      }
    kdu_long simulate_packet(kdu_long &header_bytes,
                             int layer_idx, kdu_uint16 slope_threshold,
                             bool finalize_layer, bool last_layer,
                             kdu_long max_bytes=KDU_LONG_HUGE,
                             bool trim_to_limit=false);
      /* Implements the functionality of `kd_codestream::simulate_output'
         for a single precinct.  Note that the total number of packet bytes
         is left behind in the internal `layer_bytes' array.  This allows the
         determination of tile-part length fields later on when we come to
         construct the code-stream.  The function returns this total number
         of bytes.  It also returns the number of packet header bytes via
         `header_bytes' (the function return value includes the cost of these
         header bytes).
            If the `max_bytes' limit is exceeded, the function may return
         prematurely, without having completed the simulation.  This is
         unacceptable when finalizing a layer, so you must be sure that
         `max_bytes' is sufficient in that case.
            If `trim_to_limit' is true, `finalize_layer' and `last_layer'
         must also both be true.  The function then trims away all coding
         passes from the precinct's code-blocks which are associated with
         slope values of `slope_threshold'+1 or less, until the limit
         associated with `max_bytes' is satisfied.  This functionality is
         used to implement the `sloppy_bytes' capabilities advertised by the
         `kd_codestream::simulate_output' function. */
    kdu_long write_packet(kdu_uint16 slope_threshold, bool empty_packet=false);
      /* Writes out the next packet for this precinct, using the supplied
         distortion-length slope threshold.  Increments the `next_layer_idx'
         field as well as the containing tile's `num_sequenced_packets'
         field when done.  If `empty_packet' is true, an empty packet should
         be issued, having a length of 1 byte, plus the length of any required
         EPH marker (no SOP marker will be attached to empty packets).  The
         function returns the total number of bytes output for the packet. */
  private: // Member functions
    bool read_packet();
      /* Reads a new layer of this precinct sequentially from the
         compressed data source, incrementing the `num_packets_read' member.
         Note that packet reading may be interleaved with acesses to the
         precinct's code-blocks for various reasons.  For example, we
         may initially open a tile with only a limited number of required
         layers, using its code-block data to decompress an approximate
         representation of the image.  We may then find that the additional
         packets need to be loaded in order to advance to other precincts
         in the code-stream.
            The function returns false if an SOT marker (or an EOC marker) is
         encountered.  Note that when an SOT marker is encountered, its
         segment is fully read into the codestream marker service and the
         precinct's tile must be active (otherwise an error message will be
         generated).  Before returning false in this way, the function sets
         the `active_tile' pointer in the codestream object to NULL.  The
         caller may determine the cause of failure by invoking the codestream
         marker service's `get_code' function, which will return one of SOT,
         EOC or 0, where 0 occurs only if the code-stream has been
         exhausted. */
    bool handle_corrupt_packet();
      /* This function implements alternate processing for the `read_packet'
         function in the event that a packet is found to be corrupt.  It is
         called whenever packet corruption is detected, including when the
         precinct is known to have had previously corrupt packets. The function
         manipulates the containing tile's `skipping_to_sop' and
         `next_sop_sequence_num' fields, advancing to the next SOP or SOT
         marker if necessary (the function may be called after detecting an
         SOP marker already, whose sequence number is not as expected).  If
         an SOT marker is encountered, the function renders the current tile
         inactive and returns false (this is the behaviour expected of
         `read_packet').  Otherwise, it updates the precinct and tile state to
         identify the packet as having been read. */
  public: // Public Links and Identification
    kd_resolution *resolution;
    kd_precinct_ref *ref; // Element of `resolution->precinct_refs' array

  public: // Mode flags used for decompression and/or interchange
    int flags; // The following flags are defined.
       // KD_PFLAG_GENERATING -- after 1'st call to `kd_precinct::get_packets'
       // KD_PFLAG_CORRUPTED -- if error occurred during packet reading
       // KD_PFLAG_DESEQUENCED -- if all packets have been desequenced
       // KD_PFLAG_ADDRESSABLE -- if seekable, cached or used for interchange
       // KD_PFLAG_RELEASED -- if `activate' must be called to revive state
       // KD_PFLAG_INACTIVE -- if resides on `kd_precict_server's inactive list
       // KD_PFLAG_RELEVANT -- always true if codestream is for non-persistent
       //     input; otherwise, true only if precinct belongs to the relevant
       //     set of precincts of a relevant tile-component-resolution
       // KD_PFLAG_SIGNIFICANT -- if any code-block bytes have been generated
       //     by `kdu_precinct::size_packets' or `kdu_precinct::get_packets'
       // KD_PFLAG_PARSED -- if the precinct was parsed previously and then
       //     unloaded; prevents multiple accumulation of packet length stats
  public: // State variables
    int required_layers; /* Actual number of layers for output codestream;
                max apparent layers for input or interchange codestream. */
    int next_layer_idx; // Next layer to be sequenced or generated.
    union { // First field used for input; second interchange; third for output
        int num_packets_read; // Incremented when packets read or interchanged
        int cumulative_bytes; // Size of all packets generated so far.
        int saved_next_layer_idx; // Used by "kd_packet_sequencer::save_state"
      };

  public: // Activation state variables
    int num_outstanding_blocks; // See notes below.
    union { // First field used only in reading; second only in writing.
        kdu_long unique_address; // See below
        kdu_long *packet_bytes; // Lengths generated during simulated output
      };
  public: // code-block containers
    kd_precinct_band *subbands; // In practice, this member points into the
       // same block of memory which is allocated to hold the precinct so
       // as to encourage memory locality and allow for manipulation of all
       // aspects of a precinct, including all of its code-blocks, via a
       // single heap block.  Note that the number of entries in this array
       // is given by `resolution->num_subbands'.

  public: // See below for an explanation of these links
    kd_precinct *next; // Next in free, inactive or generated list
    kd_precinct *prev; // Previous in inactive or generated list
    kd_precinct_size_class *size_class; // For correctly releasing storage
  };
  /* Notes:
        `unique_address' is valid if and only if the precinct is marked as
     addressable, meaning that the precinct has a unique address from which it
     may be loaded (for input codestream objects) or identified (for
     interchange codestream objects).  If addressable, the `unique_address'
     will be 0 if and only if the precinct is known to be empty, meaning that
     reloading the precinct does not require any actual data transfers from a
     compressed data source.
        Except as described above, when used with seekable compressed data
     sources, the `unique_address' member holds a positive quantity which
     identifies an offset into the code-stream at which the precinct's
     packets may be found.  In the case of cached sources (those advertising
     the KDU_SOURCE_CAP_CACHED capability) or codestream objects created for
     interchange (no compressed data source or target), the `unique_address'
     will be negative and -(1+`unique_address') is the precinct's unique
     identifier, as passed to the `kdu_compressed_source::set_precinct_scope'
     function, or returned by the `kdu_precinct::get_unique_id' function.
        The `num_outstanding_blocks' member is used to monitor the number of
     code-blocks from the application's region of interest which have still
     to be consumed (input codestreams) or the number of code-blocks which
     have still to be written in order to complete the precinct (output
     and interchange codestreams).
        The `next' and `prev' links may be used to manage the precinct's
     location in one of three different lists.  The first list is the
     singly-linked free list maintained by `kd_precinct_size_class', from
     which the `kd_precinct_server' object re-issues previously released
     resources.
        The second list on which this structure may find itself is the
     doubly-linked list of currently inactive precincts, which is managed by
     the `kd_precinct_server' object.  Precincts which find themselves on this
     list have been released, but their resources have not yet been recycled.
     They may be successfully re-activated, if their resources have not been
     re-issued to another precinct.  Precincts on this list may be identified
     by the `inactive' flag.  Every inactive precinct necessarily also has
     its `released' member set to true, but the converse is not necessarily
     true.  This is because precincts which have been partially parsed
     in a context for which they were not directly relevant, will be marked
     as released, even though they are not inactive.
        The third list on which this structure may find itself is the
     doubly-linked list of generated precincts managed by the
     `kd_global_rescomp' structure.  This list contains all precincts
     from a given resolution of a given image component, across all tiles,
     which have been fully generated, but not yet flushed out to the
     code-stream.   The list is used to manage incremental rate allocation
     when processing large images.  Elements on this list have not yet
     been closed. */

/*****************************************************************************/
/*                         kd_precinct_size_class                            */
/*****************************************************************************/

  /* For an explanation of the role played by precinct size classes, see
     the discussion appearing below the declaration of `kd_precinct_server'. */

class kd_precinct_size_class {
  public: // Member functions
    kd_precinct_size_class(int max_blocks, int num_subbands,
                           kd_precinct_server *server,
                           kd_buf_server *buf_server)
      {
        this->max_blocks = max_blocks;
        this->num_subbands = num_subbands;
        this->server = server;  this->buf_server = buf_server;
        total_precincts = 0; free_list = NULL; next = NULL;
        alloc_bytes = (int) sizeof(kd_precinct);
        alloc_bytes += (-alloc_bytes) & 7; // Round up to 8-byte boundary
        alloc_bytes += num_subbands * (int) sizeof(kd_precinct_band);
        alloc_bytes += (-alloc_bytes) & 7; // Round up to 8-byte boundary
        alloc_bytes += 4 + max_blocks * (int) sizeof(kd_block);
      }
    ~kd_precinct_size_class()
      { kd_precinct *tmp;
        while ((tmp=free_list) != NULL)
          { free_list = tmp->next; free(tmp); total_precincts--; }
        assert(total_precincts == 0);
      }
    kd_precinct *get()
      {
        if (free_list == NULL)
          augment_free_list();
        kd_precinct *result = free_list; free_list = free_list->next;
        result->next = result->prev = NULL;
        buf_server->augment_structure_bytes(alloc_bytes);
        return result;
      }
    void release(kd_precinct *precinct)
      {
        if (precinct->flags & KD_PFLAG_INACTIVE)
          withdraw_from_inactive_list(precinct);
        precinct->next = free_list;
        free_list = precinct;
        buf_server->augment_structure_bytes(-alloc_bytes);
      }
    void move_to_inactive_list(kd_precinct *precinct);
    void withdraw_from_inactive_list(kd_precinct *precinct);
      /* Used to move precincts onto or off the inactive list maintained
         by the `kd_precinct_server' object to which the present object
         belongs. */
  private: // Helper function
    void augment_free_list();
  private: // Data
    friend class kd_precinct_server;
    kd_precinct_server *server;
    kd_buf_server *buf_server;
    int max_blocks;
    int num_subbands;
    int alloc_bytes; // Num bytes to allocate per precinct.
    kdu_long total_precincts; // Number of precincts allocated with this size
    kd_precinct *free_list; // List of precincts which have been released
    kd_precinct_size_class *next; // Next size class
  };

/*****************************************************************************/
/*                           kd_precinct_server                              */
/*****************************************************************************/

class kd_precinct_server {
  public: // Member functions
    kd_precinct_server(kd_buf_server *buf_server)
      { size_classes = NULL; total_allocated_bytes = 0;
        inactive_head = inactive_tail = NULL; this->buf_server = buf_server; }
    ~kd_precinct_server()
      {
        kd_precinct_size_class *tmp;
        while ((tmp=size_classes) != NULL)
          { size_classes = tmp->next; delete tmp; }
      }
    kd_precinct *get(int max_blocks, int num_subbands);
    kdu_long get_total_allocated_bytes()
      { return total_allocated_bytes; }
  private: // Data
    friend class kd_precinct_size_class;
    kd_precinct_size_class *size_classes; // List of different size classes.
    kdu_long total_allocated_bytes;
    kd_precinct *inactive_head, *inactive_tail;
    kd_buf_server *buf_server; // Used to manage cacheing, as described below
  };
  /* Notes:
        This object is used to serve up empty precincts and to recycle their
     storage when they are no longer required.  Precinct memory blocks are
     organized into size classes, based on the maximum number of code-blocks
     and precinct-bands in the precinct, where these quantities are consistent
     for all precincts associated with any given `kd_resolution' object.  In
     many cases, it can happen that all precincts from the entire image have
     exactly the same size class.  This maximizes the potential for recycling
     precinct storage, since precincts may only be allocated from and
     recycled back to their own size class.  The reason for this dependence on
     size classes is that the block of memory associated with each precinct
     is large enough to accommodate all `max_blocks' code-blocks which might
     be required to populate the `kd_precinct_band::blocks' arrays and all
     precinct-bands which are required to populate the `kd_precinct::subbands'
     array.  This gets us out of having to maintain separate mechanisms
     to serve up code-block storage as a recyclable resource.  It also helps
     with memory localization.
        The object also manages a cacheing service for precincts and their
     data.  When precincts are released using `kd_precinct_ref::release',
     they are appended to a list of inactive precincts headed by
     `inactive_head' and concluded by `inactive_tail'.  The list is doubly
     linked via the `kd_precinct::next' and `kd_precinct::prev' members.  When
     a previously released precinct is opened using `kd_precinct_ref::open',
     it is removed from this list and its `kd_precinct::activate' function is
     called.  Note that the KD_PFLAG_RELEASED flag is not a reliable
     indicator of whether or not a precinct is on the inactive list, since
     precincts which have been released using `kd_precinct::release', but
     have not yet been completely desequenced (all packets have not yet been
     parsed from or located in the code-stream), will also have this flag
     set, but will not be on an inactive list.  The KD_PFLAG_INACTIVE
     flag should be used to determine whether or not a precinct is on the
     inactive list.
        When `kd_precinct_server::get' function is called, it queries the
     codestream object's `kd_buf_server' service to determine whether or not
     a cache threshold has been exceeded.  If so, it closes down precincts
     on the inactive list until the `kd_buf_server' object reports that
     sufficient resources are in hand. */

/*****************************************************************************/
/*                          kd_thread_block_state                            */
/*****************************************************************************/

struct kd_thread_block_state {
    kd_block block;
    kd_precinct *target_precinct;
    kd_block *target_block; // see below
  };
  /* An array of these structures is used to maintain information about
     processed code-blocks in thread-local storage, reflecting this information
     only every so often to the `kdu_codestream' machinery, so as to minimize
     thread blocking and cache coherency management overhead.  Each object of
     this class represents a single code-block, identified via the
     `target_precinct' and `target_block' members.
        If `target_precinct' is NULL, the record is empty.  Non-empty records
     may appear anywhere within the `outstanding_blocks' array in
     `kd_thread_env'. */

/*****************************************************************************/
/*                              kd_thread_env                                */
/*****************************************************************************/

#define KD_THREAD_MAX_OUTSTANDING_BLOCKS 8

class kd_thread_env {
  public: // Member functions
    kd_thread_env(kdu_thread_env *owner)
      {
        this->owner=owner;  buf_server.init(this);
        num_outstanding_blocks=0;  stats_codestream = NULL;
        memset(outstanding_blocks,0,
               sizeof(kd_thread_block_state)*KD_THREAD_MAX_OUTSTANDING_BLOCKS);
      }
    kd_thread_block_state *get_block_state(kd_codestream *cs,
                                           kd_precinct *precinct,
                                           kd_block *block)
      {
        if (num_outstanding_blocks == KD_THREAD_MAX_OUTSTANDING_BLOCKS)
          flush(true);
        kd_thread_block_state *scan=outstanding_blocks;
        for (int n=0; n < KD_THREAD_MAX_OUTSTANDING_BLOCKS; n++, scan++)
          if (scan->target_precinct == NULL)
            {
              num_outstanding_blocks++;
              scan->target_precinct=precinct;  scan->target_block=block;
              owner->have_outstanding_blocks = true;
              return scan;
            }
        return NULL; // Should not be possible!!
      }
    void update_stats(kdu_block *blk, kd_codestream *cs)
      { /* Called during encoding after each code-block of data has been
           generated, whenever coding statistics are being kept by the
           relevant codestream.  This function updates the internal coding
           statistics record managed by the `stats' member, which is
           reconciled against the underlying codestream only at appropriate
           intervals, so as to minimize thread blocking conditions. */
        if ((stats_codestream != cs) && (stats_codestream != NULL))
          {
            reconcile_stats();
            stats_codestream = cs;
          }
        stats.update_stats(blk);
      }
    void reconcile_stats();
      /* This function is used to reconcile the temporary information in
         the `stats' member with the coding statistics manager in the
         `kd_codestream' object referenced by the `stats_codestream' member.
         After reconciliation, the `stats' member will be empty and the
         `stats_codestream' member is reset to NULL so that we are free to
         accumulate statistics for a different codestream in the future.
         This function is invoked from `flush' and also if a call to
         `update_stats' specifies a different codestream to that for which
         coding statistics are currently being recorded in the `stats'
         member. */
    void flush(bool force_flush);
      /* Clears the `outstanding_blocks' queue and reconciles any outstanding
         `stats' information.  This function should not be called while
         holding any lock, since it typically needs to acquire its own lock or
         locks.
            If `force_flush' is false, the function may return without
         flushing anything, if it finds that the necessary mutex is already
         held by another thread.  This helps to keep threads alive for as long
         as possible.  A thread will typically try to flush its block queue
         each time it is augmented, once it starts getting close to full.
         A forced flush is required only once the queue becomes completely
         full -- this is performed automatically from within
         `get_block_state'. */
    void discard_all();
      /* This function is called from `kdu_thread_env::do_sync' or
         `kdu_thread_env::on_finished' if an exception has been handled
         by any thread in the working group.  It discards all processed
         code-blocks, returning any associated storage to the internal
         queue, being careful to do so in such a way as to avoid throwing
         further exceptions while acquiring locks. */
  private: // Padding
    kdu_byte _leadin[KDU_MAX_L2_CACHE_LINE];
  public: // Links
    kdu_thread_env *owner;
  public: // Public resources
    kdu_block block; // Thread-local code-block storage
    kd_compressed_stats stats; // Used to temporarily record statistics
    kd_codestream *stats_codestream; // Codestream to which statistics belong
    kd_thread_buf_server buf_server;
    int num_outstanding_blocks;
  private: // Private resources
    kd_thread_block_state outstanding_blocks[KD_THREAD_MAX_OUTSTANDING_BLOCKS];
  private: // Padding
    kdu_byte _trailer[KDU_MAX_L2_CACHE_LINE];
  };


/* ========================================================================= */
/*         Inlined Member Functions Delayed to Satisfy Dependencies          */
/* ========================================================================= */

inline void
  kd_thread_buf_server::set_codestream_buf_server(kd_buf_server *buf_server)
{
  if (buf_server != this->ultimate_buf_server)
    {
      env->owner->acquire_lock(KD_THREADLOCK_GENERAL);
      if (ultimate_buf_server != NULL)
        ultimate_buf_server->detach_thread_buf_server(this);
      if (buf_server != NULL)
        buf_server->attach_thread_buf_server(this);
      assert(buf_server == ultimate_buf_server);
      env->owner->release_lock(KD_THREADLOCK_GENERAL);
    }
}

inline bool
  kd_precinct_ref::is_desequenced()
{
  register kdu_long state_val = state;
  return ((state_val != 0) &&
          ((state_val & 1) ||
           (((kd_precinct *) _kdu_long_to_addr(state_val))->flags &
            KD_PFLAG_DESEQUENCED)));
}

inline void
  kd_precinct_ref::clear()
{
  if (state && !(state & 1))
    {
      kd_precinct *precinct = (kd_precinct *) _kdu_long_to_addr(state);
      precinct->ref = NULL; // Satisfy the test in `kd_precinct::closing'
      precinct->closing();
      precinct->size_class->release(precinct);
    }
  state = 0;
}

inline void
  kd_precinct_ref::close_and_reset()
{
  if (state & 1)
    {
      if (state != 3)
        state &= ~((kdu_long) 2);
    }
  else if (state != 0)
    {
      kd_precinct *prec = (kd_precinct *) _kdu_long_to_addr(state);
      if ((prec->flags & KD_PFLAG_PARSED)||(prec->num_packets_read > 0))
        {
          close();
          state &= ~((kdu_long) 2);
        }
    }
}

inline void
  kd_precinct_ref::release()
{
  kd_precinct *precinct = (kd_precinct *) _kdu_long_to_addr(state);
  assert((state != 0) && !(state & 1));
  if (precinct->flags & KD_PFLAG_INACTIVE)
    return; // Already on the inactive list.
  precinct->flags |= KD_PFLAG_RELEASED;
    // precinct->released = true; // In case not already set.
  if ((precinct->flags & KD_PFLAG_ADDRESSABLE) &&
      (!precinct->resolution->codestream->cached) &&
      ((precinct->num_packets_read == 0) ||
       (precinct->num_packets_read == precinct->next_layer_idx)))
    precinct->size_class->move_to_inactive_list(precinct);
  else
    close();
}

inline kd_precinct *
  kd_precinct_ref::open(kd_resolution *res, kdu_coords pos_idx,
                        bool need_activate)
{
  if (state == 3)
    return NULL; // Precinct is not currently loaded
  if ((state == 0) || (state & 1))
    return instantiate_precinct(res,pos_idx);
  kd_precinct *result = (kd_precinct *) _kdu_long_to_addr(state);
  if (result->flags & KD_PFLAG_INACTIVE)
    { // Must be on the inactive list
      result->size_class->withdraw_from_inactive_list(result);
      result->activate();
    }
  else if (need_activate && (result->flags & KD_PFLAG_RELEASED))
    result->activate();
  return result;
}

inline kd_precinct *
  kd_precinct_ref::active_deref()
{ // This function is often invoked without mutex protection.  It is used to
  // determine whether a valid precinct reference exists already or one must
  // be created using `kd_precinct_ref::open', which should be protected in
  // a multi-threading environment.  To avoid possible race conditions, we
  // inform the compiler that the `state' variable should be treated as
  // volatile here.
  volatile kdu_long *state_ref = &state;
  register kdu_long state_val = *state_ref;
  if ((state_val & 1) || (state_val == 0)) return NULL;
  kd_precinct *result = (kd_precinct *) _kdu_long_to_addr(state_val);
  if (result->flags & (KD_PFLAG_INACTIVE|KD_PFLAG_RELEASED)) return NULL;
  return (*state_ref == _addr_to_kdu_long(result))?result:NULL;
      // Need to be sure that another thread did not activate the precinct
      // object for use with another `kd_precinct_ref' reference, causing
      // the flags test above to succeed, even though the precinct had been
      // in the released state when we accessed the value of `state' on
      // the second line.  The fact that `state_ref' is volatile, forces the
      // compiler to generate code which reloads it to properly confirm that
      // `result' is the precinct which belongs to us.
}


/* ========================================================================= */
/*                              External Functions                           */
/* ========================================================================= */

extern void
  kd_create_dwt_description(int kernel_id, int atk_idx, kdu_params *root,
                            int tnum, bool &reversible, bool &symmetric,
                            bool &symmetric_extension, int &num_steps,
                            kdu_kernel_step_info * &step_info,
                            float * &coefficients);
  /* This function is used in two places to expand the description of an
     arbitrary DWT kernel, based on information recovered from the codestream
     parameter sub-system.  Inputs to the function are the `kernel_id'
     (one of `Ckernels_W5X3', `Ckernels_W9X7' or `Ckernels_ATK'), the
     `atk_idx' (for non-Part1 kernel's identified as `Ckernels_ATK'), and
     the `root' of the codestream parameter sub-system.  The function
     determines whether or not the kernel describes a reversible transform,
     whether the kernel filters are whole-sample-symmetric, whether the
     boundary extension policy is symmetric extension or replication, the
     description of each lifting step, and the set of all lifting step
     coefficients, returning them via the last 6 arguments.  The function
     creates two arrays, returning them via the `step_info' and
     `coefficients' arguments.  In the event that the function fails through
     `kdu_error', most applications will define the behaviour to be that of
     throwing an exception.  In this case, it is important that these
     arguments reference members of a persistent structure which can be
     cleaned up when the exception is handled.  As for the structure of the
     arrays, it is sufficient to note that they can be passed directly to
     the `kdu_kernels::init' function. */

#endif // COMPRESSED_LOCAL_H
