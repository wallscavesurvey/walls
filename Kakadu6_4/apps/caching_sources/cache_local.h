/*****************************************************************************/
// File: cache_local.h [scope = APPS/CACHING_SOURCES]
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
  Contains local definitions used within "kdu_cache.h"
******************************************************************************/

#ifndef CACHE_LOCAL_H
#define CACHE_LOCAL_H

#include "kdu_cache.h"

// Defined here:
struct kd_cache_buf;
struct kd_cache_buf_group;
class kd_cache_buf_server;
struct kd_cache_hd;
class kd_cache_buf_io;
struct kd_var_cache_seg;
struct kd_class_cache;
struct kd_stream_cache;
struct kd_cache;

/*****************************************************************************/
/*                               kd_cache_buf                                */
/*****************************************************************************/

#define KD_CACHE_BUF_LEN 64 // Must be a multiple of 4, >= sizeof(kd_cache_hd)

struct kd_cache_buf {
    kdu_byte bytes[KD_CACHE_BUF_LEN];
    kd_cache_buf *next;
  };
  /* Variable length buffers in the cache are created as linked lists of
     these buffers.  When used in this way, the first 4 bytes of the first
     buffer in the list are used to store the total number of bytes in the
     entire buffer. */

/*****************************************************************************/
/*                            kd_cache_buf_group                             */
/*****************************************************************************/

#define KD_CACHE_BUF_GROUP_LEN 32

struct kd_cache_buf_group {
    kd_cache_buf bufs[KD_CACHE_BUF_GROUP_LEN];
    kd_cache_buf_group *next;
  };
  /* Rather than allocating `kd_cache_buf' structures on an individual basis,
     they are allocated in multiples (groups) of 32 at a time.  This reduces
     the number of heap allocation requests and the risk of excessive
     heap fragmentation. */

/*****************************************************************************/
/*                            kd_cache_buf_server                            */
/*****************************************************************************/

class kd_cache_buf_server {
  public: // Member functions
    kd_cache_buf_server()
      { groups = NULL; free_bufs = NULL;
        allocated_bytes=peak_allocated_bytes=0; }
    ~kd_cache_buf_server()
      {
        kd_cache_buf_group *tmp;
        while ((tmp=groups) != NULL)
          { groups = tmp->next; delete tmp; }
      }
    kd_cache_buf *get()
      {
        if (free_bufs == NULL)
          {
            kd_cache_buf_group *grp = new kd_cache_buf_group;
            grp->next = groups; groups = grp;
            for (int n=KD_CACHE_BUF_GROUP_LEN-1; n >= 0; n--)
              { grp->bufs[n].next = free_bufs; free_bufs = grp->bufs + n; }
          }
        kd_cache_buf *result = free_bufs;
        free_bufs = result->next;
        result->next = NULL;
        allocated_bytes += sizeof(kd_cache_buf);
        if (allocated_bytes > peak_allocated_bytes)
          peak_allocated_bytes = allocated_bytes;
        return result;
      }
    void release(kd_cache_buf *head)
      { // Releases a list of buffers headed by `head'.
        kd_cache_buf *tmp;
        while ((tmp=head) != NULL)
          { head = tmp->next; tmp->next = free_bufs; free_bufs = tmp; }
        allocated_bytes -= sizeof(kd_cache_buf);
      }
    int get_peak_allocated_bytes() { return peak_allocated_bytes; }
  private: // Data
    kd_cache_buf_group *groups; // List of buffer resources for deletion
    kd_cache_buf *free_bufs; // Buffers are allocated from a free list
    int allocated_bytes, peak_allocated_bytes;
  };

/*****************************************************************************/
/*                                kd_cache_hd                                */
/*****************************************************************************/

struct kd_cache_hd {
    kd_var_cache_seg *lru_seg; // Segment containing less recently used bin
    kd_var_cache_seg *mru_seg; // Segment containing more recently used bin
    kdu_byte lru_idx; // Index of next less recently used bin in its segment
    kdu_byte mru_idx; // Index of next more recently used bin in its segment
    bool have_final; // True if we have the final contribution to the data bin
    bool marked; // True if marked by application
    kdu_int32 initial_bytes;
    kd_cache_buf *hole_list; // NULL unless there are holes.
  };
  /* Notes:
        The first sizeof(kd_cache_hd) bytes of each non-empty data-bin in the
     cache hold the contents of this structure.  Specifically, they hold the
     pointers required to implement a doubly-linked list of cache elements,
     plus the information required to determine the range and/or ranges of
     bytes which are currently available for this data-bin.
        The `lru_seg' and `lru_idx' members are used to link to the next
     less recently used data-bin belonging to the same `kd_cache_class'
     object.  They will be NULL and 0, respectively, if this is already
     the least recently used data-bin in its class.
        Similarly, the `mru_seg' and `mru_idx' members are used to link
     to the most recently used data-bin in the `kd_cache_class' object,
     holding NULL and 0 if this is already the most recently used bin.
        The `have_final' member is true if the final byte of this data-bin
     has already been loaded into the cache -- that does not necessarily mean
     that all earlier bytes of the data-bin have already been loaded, though.
        The `marked' member is used to implement the functionality of the
     `kdu_cache::mark_databinn' function.  The marking state has no
     pre-defined interpretation.
        The `initial_bytes' member holds the number of initial bytes from
     the data-bin which have already been loaded into the cache.  This may
     possibly be followed by other ranges of bytes which are separated from
     the initial prefix by holes.
        The `hole_list' member points to a linked list of `kd_cache_buf'
     objects which are used to store information about holes in the
     data-bins contents.  This member will hold NULL if and only if all
     available bytes form a contiguous prefix of the data-bin's contents.
     Otherwise, the list of buffers to which this member points hold a
     sequence of 2K-1 4-byte integers, where K is the number of disjoint
     contiguous segments of data in the cache for this data-bin.  The last of
     these integers holds 0.  The remaining integers form K-1 pairs, which
     represent the start of each non-initial segment (relative to the start
     of the data-bin) and the location immediately beyond the end of the
     same segment. */

/*****************************************************************************/
/*                                kd_cache_buf_io                            */
/*****************************************************************************/

  /* This class allows convenient updating of the data in a list of cache
     buffers. */
class kd_cache_buf_io {
  public: // Member functions
    kd_cache_buf_io()
      { buf_server=NULL; buf=list=NULL; buf_pos=0; }
    kd_cache_buf_io(kd_cache_buf_server *server, kd_cache_buf *list=NULL,
                    int initial_offset=0)
      { init(server,list,initial_offset); }
    void init(kd_cache_buf_server *server, kd_cache_buf *list=NULL,
              int initial_offset=0)
      /* If `list' is NULL, a new list of buffers is created when the first
         attempt is made to write data. */
      {
        assert((initial_offset >= 0) && (initial_offset <= KD_CACHE_BUF_LEN));
        buf_server = server;
        this->buf = this->list = list;
        this->buf_pos = (list==NULL)?KD_CACHE_BUF_LEN:initial_offset;
      }
    kd_cache_buf *get_list() { return list; }
    void finish_list()
      { /* Writes a terminal 0 (32-bit word) if necessary. */
        if (list == NULL) return; // No terminal 0 required
        write_length(0);
      }
    void advance(int num_bytes)
      { /* Advances the current location `num_bytes' into the cached
           representation, adding new cache buffers to the end of the list
           if necessary, but not writing any data at all into old or new
           cache buffers. */
        while (num_bytes > 0)
          {
            if (buf_pos == KD_CACHE_BUF_LEN)
              {
                if (buf == NULL)
                  buf = list = buf_server->get();
                else if (buf->next == NULL)
                  buf = buf->next = buf_server->get();
                else
                  buf = buf->next;
                buf_pos = 0;
              }
            buf_pos += num_bytes; num_bytes = 0;
            if (buf_pos > KD_CACHE_BUF_LEN)
              { num_bytes = buf_pos-KD_CACHE_BUF_LEN; buf_pos -= num_bytes; }
          }
      }
    kdu_int32 read_length()
      { /* Reads a 4-byte length value in native byte order and returns it.
           It is an error (potentially fatal) to attempt to read past the
           end of the cache buffer list.  It is also an error to read from
           an unaligned address. */
        assert((buf_pos & 3) == 0);
        if (buf_pos == KD_CACHE_BUF_LEN)
          {
            assert((buf != NULL) && (buf->next != NULL));
            buf = buf->next;
            buf_pos = 0;
          }
        kdu_int32 *ptr = (kdu_int32 *)(buf->bytes + buf_pos);
        buf_pos += 4;
        return *ptr;
      }
    bool read_byte_range(kdu_int32 &start, kdu_int32 &lim)
      { /* Reads a new byte range consisting of `start' and `limt'>`start' if
           it can, returning false if `start' turns out to be zero. */
        if ((buf == NULL) || ((start = read_length()) == 0)) return false;
        lim = read_length();
        assert(lim > start);
        return true;
      }
    void write_length(kdu_int32 length)
      { /* Writes a 4 byte big-endian length value, augmenting the cache buffer
           list if necessary.  It is an error to write to an unaligned address
           within the buffer. */
        assert((buf_pos & 3) == 0);
        if (buf_pos == KD_CACHE_BUF_LEN)
          {
            if (buf == NULL)
              buf = list = buf_server->get();
            else if (buf->next == NULL)
              buf = buf->next = buf_server->get();
            else
              buf = buf->next;
            buf_pos = 0;
          }
        kdu_int32 *ptr = (kdu_int32 *)(buf->bytes + buf_pos);
        buf_pos += 4;
        *ptr = length;
      }
    void write_byte_range(kdu_int32 start, kdu_int32 lim)
      { // Writes two integers, the second of which must be strictly greater
        // than the first.
        assert(lim > start);
        write_length(start);
        write_length(lim);
      }
    void copy_from(const kdu_byte data[], int num_bytes)
      { /* Copies `num_bytes' from the `data' buffer into the cached
           representation, starting from the current location and extending
           the buffer list as necessary to accommodate the demand. */
        while (num_bytes > 0)
          {
            if (buf_pos == KD_CACHE_BUF_LEN)
              {
                if (buf == NULL)
                  buf = list = buf_server->get();
                else if (buf->next == NULL)
                  buf = buf->next = buf_server->get();
                else
                  buf = buf->next;
                buf_pos = 0;
              }
            int xfer_bytes = KD_CACHE_BUF_LEN-buf_pos;
            xfer_bytes = (xfer_bytes < num_bytes)?xfer_bytes:num_bytes;
            memcpy(buf->bytes+buf_pos,data,(size_t) xfer_bytes);
            data += xfer_bytes; num_bytes -= xfer_bytes; buf_pos += xfer_bytes;
          }
      }
    bool operator==(kd_cache_buf_io &rhs)
      { return (buf == rhs.buf) && (buf_pos == rhs.buf_pos); }
  public:
    kd_cache_buf_server *buf_server;
    kd_cache_buf *list; // Points to head of list
    kd_cache_buf *buf; // Points to current buffer in list
    int buf_pos;
  };

/*****************************************************************************/
/*                             kd_var_cache_seg                              */
/*****************************************************************************/

struct kd_var_cache_seg {
  public: // Member functions
    kd_var_cache_seg() { memset(this,0,sizeof(kd_var_cache_seg)); }
    ~kd_var_cache_seg();
      /* Recursive deletion function.  Does not explicitly free up
         `kd_cache_buf' resources, but these can be globally released
         by deleting the `kd_buf_server' object. */
  public: // Data
    kdu_long base_id; // In-class ID of first entry in `databins' array.
    int shift;
    union {
        kd_cache_buf *databins[128];
        kd_var_cache_seg *segs[128];
      };
  };
  /* This structure is the basis for a dynamically expandable set of cache
     entries.  Each data-bin has a unique identifier.  Ideally, we would
     allocate an array with one entry for each possible data-bin.  While this
     makes addressing them easy, it presupposes that the cache has a reliable
     way to determine the number of possible data-bins which could occur
     up front.  The alternative, implemented here, requires no such
     capability.  The data associated with any particular data-bin are
     recovered as follows:
        If `shift' is 0, the data-bin identifier must be in the range 0 to
     127 and a pointer to its cache buffers is recovered by indexing the
     `databins' array with the least significant 7 bits of the identifier.
     If `shift' is 7, the data-bin identifier must be in the range 0 to
     2^14-1; the most significant 7 bits of the 14-bit identifier are used
     to index one of the `kd_var_cache_seg' objects in the `segs' array; the
     least significant 7 bits are used to address the specific elements of
     the `databins' array for this object.  More generally, `shift' is a
     multiple of 7, and the data-bin's buffer list may be recovered by
     dereferencing databins[id], if shift=0, or else recursively applying
     the dereferencing algorithm to segs[id>>shift], using
     (id & ~((-1)<<shift)) as the identifier.
        If, in the process of applying the above dereferencing algorithm, it
     is discovered that the identifier does not currently exist in the cache,
     a new `kd_var_cache' object may need to be created, or a new tier may
     need to be added to the multi-tier dereferencing scheme.  Interestingly,
     the addition of new tiers is immune to race conditions which might
     be expected between readers and writers of the cache which run in
     different threads of execution. */

/*****************************************************************************/
/*                               kd_class_cache                              */
/*****************************************************************************/

struct kd_class_cache {
  public: // Member functions
    kd_class_cache()
      {
        class_code = KDU_UNDEFINED_DATABIN; // Needs to be set by owner
        root = NULL; transferred_bytes = 0;
        lru_seg = mru_seg = NULL; lru_idx = mru_idx = 0;
      }
    ~kd_class_cache()
      { reset(); }
    void reset()
      { /* The caller is responsible for deleting the `kd_buf_server' object
           to release the resources assigned to this class cache. */
        if (root != NULL)
          { delete root; root = NULL; }
        lru_seg = mru_seg = NULL;
        lru_idx = mru_idx = 0; transferred_bytes = 0;
      }
    void unlink_databin(kd_var_cache_seg *seg, kdu_byte idx);
      /* Unlinks the data bin at `seg->databins[idx]' from the
         MRU and LRU lists. */
    void add_mru(kd_var_cache_seg *seg, kdu_byte idx);
      /* Adds the data bin at `seg->databins[idx]' to the head of the MRU
         list and the tail of the LRU list.  If it is already linked into
         these lists, the caller is responsible for calling `unlink_databin'
         first. */
    void add_lru(kd_var_cache_seg *seg, kdu_byte idx);
      /* Adds the data bin at `seg->databins[idx]' to the head of the LRU
         list and the tail of the MRU list.  If it is already linked into
         these lists, the caller is responsible for calling `unlink_databin'
         first. */
  public: // Data
    int class_code; // e.g., `KDU_PRECINCT_DATABIN', ...
    kd_var_cache_seg *root;
    int transferred_bytes;
    kd_var_cache_seg *lru_seg;
    kdu_byte lru_idx;
    kd_var_cache_seg *mru_seg;
    kdu_byte mru_idx;
  };
  /* `root' holds NULL until some entries are added to this cache class.  Once
     that happens, `root' points to a single cache segment, which is the root
     of a sparse tree of cache segments representing the entire cache contents.
        `lru_seg' and `lru_idx' together identify the least recently used
     data bin in the cache -- `lru_seg' points to the cache segment in which
     it resides, and `lru_idx' holds the index (0 to 127) of the relevant
     entry in `lru_seg->databins'.
        `mru_seg' and `mru_idx' together identify the most recently used
     data bin in the cache -- `mru_seg' points to the cache segment in which
     it resides, and `mru_idx' holds the index (0 to 127) of the relevant
     entry in `mru_seg->databins'.
  */

/*****************************************************************************/
/*                              kd_stream_cache                              */
/*****************************************************************************/

struct kd_stream_cache {
  public: // Member functions
    kd_stream_cache(kdu_long codestream_id)
      {
        this->codestream_id = codestream_id;
        for (int cls=0; cls < KDU_NUM_DATABIN_CLASSES; cls++)
          classes[cls].class_code = cls;
        next = NULL;
      }
  public: // Data
    kdu_long codestream_id;
    kd_class_cache classes[KDU_NUM_DATABIN_CLASSES];
    kd_stream_cache *next; // Points to next stream in list
  };

/*****************************************************************************/
/*                                  kd_cache                                 */
/*****************************************************************************/

struct kd_cache {
  public: // Member functions
    kd_cache()
      {
        total_cache_segs = read_buf_pos = databin_pos = 0;
        read_head = NULL; read_start = NULL;
        streams = current_stream = NULL; read_buf = NULL; buf_server = NULL;
      }
  public: // Owned resources and cache state
    kd_cache_buf_server *buf_server;
    kd_stream_cache *streams; // Points to a list of code-stream-based records
    int total_cache_segs;
  public: // Read/scope state management
    kd_stream_cache *current_stream; // Points to the current code-stream
    kd_cache_buf *read_start; // Points to first buffer in active data-bin
    kd_cache_buf *read_buf; // Points to current buffer for the active data-bin
    int read_buf_pos; // Position of next byte to be read from `read_buf'
    int databin_pos; // Position of next byte to be read, within data-bin
    kd_cache_hd *read_head; // Use this to get length of current read context
  };

#endif // CACHE_LOCAL
