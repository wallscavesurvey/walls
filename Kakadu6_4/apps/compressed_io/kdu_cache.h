/*****************************************************************************/
// File: kdu_cache.h [scope = APPS/COMPRESSED_IO]
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
  Describes a platform independent caching compressed data source.  A
complete implementation for the client in an interactive client-server
application can be derived from this class and requires relatively little
additional effort.  The complete client must incorporate networking elements.
******************************************************************************/

#ifndef KDU_CACHE_H
#define KDU_CACHE_H

#include <assert.h>
#include <string.h>
#include "kdu_elementary.h"
#include "kdu_compressed.h"

// Defined here
class kdu_cache;

// Defined elsewhere
struct kd_cache;

/*****************************************************************************/
/* ENUM                    Data-bin Class Identifiers                        */
/*****************************************************************************/

#define KDU_PRECINCT_DATABIN    0 // Used for precinct-oriented streams
#define KDU_TILE_HEADER_DATABIN 1 // One for each tile in the code-stream
#define KDU_TILE_DATABIN        2 // Used for tile-part oriented streams
#define KDU_MAIN_HEADER_DATABIN 3 // Code-stream main header; only ID=0 allowed
#define KDU_META_DATABIN        4 // Used for meta-data and file structure info
#define KDU_UNDEFINED_DATABIN   5

#define KDU_NUM_DATABIN_CLASSES KDU_UNDEFINED_DATABIN

/*****************************************************************************/
/*                                 kdu_cache                                 */
/*****************************************************************************/

class kdu_cache : public kdu_compressed_source {
  /* [BIND: reference]
     [SYNOPSIS]
       Implements a caching compressed data source, i.e., one which offers
       the `KDU_SOURCE_CAP_CACHED' capability explained in connection with
       `kdu_compressed_source::get_capabilities'.
       [//]
       The object has two types of interfaces: those used to transfer new
       data into the cache (e.g., data received incrementally over a
       network connection); and those used to retrieve the cached data.  By
       implementing the `acquire_lock' and `release_lock' functions in
       a derived class, these two sets of functions may be safely invoked
       from different threads in a multi-threading environment, allowing
       cache updates to occur asynchronously with cached data access.
       [//]
       When a caching data source is supplied to `kdu_codestream::create'
       the image quality obtained when rendering the code-stream data
       generally improves as more and more compressed data is transferred
       into the cache using the `add_to_databin' function.  Note carefully
       that the object must not be passed into `kdu_codestream::create'
       at least until the main header data-bin has been completed.  This
       may be verified by calling `get_databin_length', with a
       data-bin class of `KDU_MAIN_HEADER_DATABIN'.  Before passing the
       object to `kdu_codestream::create' you must call `set_read_scope',
       passing in these same data-bin class and in-class identifiers
       (`KDU_MAIN_HEADER_DATABIN' and 0, respectively).
       [//]
       Even more functionality may be achieved if you read from a caching
       data source indirectly via a `jp2_source' object.  To do this, you
       must first create and open a `jp2_family_src' object, passing the
       `kdu_cache' object in as the `jp2_family_src' object's data source.
       You can then open JP2 boxes by passing the `jp2_family_src' object
       in the call to `jp2_input_box::open' or to `jp2_source::open'.  In
       fact, you may open multiple boxes simultaneously on the same
       `jp2_family_src' object (even from multiple threads, if you are
       careful to implement the synchronization objects offered by the
       `jp2_family_src' class.  Using the `kdu_cache' object as a source
       for JP2 box and JP2 source reading, provides you with additional
       functionality specifically designed for interacting with the meta-data
       and with multiple code-streams via the JPIP protocol and allows high
       level applications to be largely oblivious as to whether the ultimate
       source of information is local or remote.
       [//]
       Some effort is invested in the implementation of this object to
       ensure that truly massive compressed images (many Gigabytes in size)
       may be cached efficiently.  In particular, both header data and
       compressed precinct data are managed using a special sparse cache
       structure, which supports both efficient random access and dynamic
       growth.
       [//]
       The present object maintains its own LRU (Least Recently Used) lists
       which can be used to implement cache management policies when clients
       have insufficient memory to buffer compressed data indefinitely.  The
       LRU list may be traversed in either direction (most to least recently
       used data-bins, or vice-versa).  Elements may also be promoted or
       demoted within the list by making appropriate function calls.
  */
  public: // Lifecycle member functions
    KDU_AUX_EXPORT kdu_cache();
    KDU_AUX_EXPORT virtual ~kdu_cache();
    KDU_AUX_EXPORT void attach_to(kdu_cache *existing);
      /* [SYNOPSIS]
           This function may be called to attach the cache object to another
           existing cache object.  Once attached, all attempts to access
           the contents of the cache will actually access the contents of
           the `existing' cache.  While attached, you may not use the
           `add_to_databin' function or any member function which modifies
           the cache contents, or their auxiliary information (list order
           and mark status), since the object has no cache elements
           of its  own.  In this mode, the `existing' object's `acquire_lock'
           and `release_lock' function will be used instead of the present
           object's synchronization functions, wherever synchronization is
           required.
           [//]
           To detach the cache, you must call `close', after which the
           object will be able to perform as an autonomous cache.
           [//]
           Automatically closes the present cache object if it has been
           used prior to calling this function.
      */
    KDU_AUX_EXPORT virtual bool close();
      /* [SYNOPSIS]
           It is always safe to call this function.  It discards all cached
           data-bins and any associated structures, after which you can start
           using the object again with a clean slate.  Immediately after
           this call, the `get_databin_length' function will return 0 in
           response to all queries.
         [RETURNS]
           Always returns true.
      */
  public: // Functions used to manage cached data
    KDU_AUX_EXPORT virtual void
      add_to_databin(int databin_class, kdu_long codestream_id,
                     kdu_long databin_id, const kdu_byte data[],
                     int offset, int num_bytes, bool is_final,
                     bool add_as_most_recent=true,
                     bool mark_if_augmented=false);
      /* [SYNOPSIS]
           Augments the cached representation of the indicated data-bin.  The
           data-bin is identified by a class code, `databin_class', a
           code-stream ID (normally 0, unless the cache is used to manage
           multiple code-streams simultaneously) and an in-class identifier,
           `databin_id'.  Currently, the classes defined by this
           object are `KDU_META_DATABIN', `KDU_MAIN_HEADER_DATABIN',
           `KDU_TILE_HEADER_DATABIN', `KDU_PRECINCT_DATABIN' and
           `KDU_TILE_DATABIN'.
           [>>] The `KDU_META_DATABIN' class is used with JP2-family file
                formats to communicate metadata and file structure
                information.  Amongst other things, this metadata provides
                the framework within which the content of code-stream
                specific data-bins are to be interpreted.  For this data-bin
                class, the value of the `codestream_id' is ignored.  Raw
                code-streams have exactly one metadata-bin (the one with
                `databin_id'=0) which must be empty -- it must be signalled
                as finalized after transferring 0 bytes of content data.
           [>>] For the `KDU_MAIN_HEADER_DATABIN' class, the in-class
                identifier must be equal to 0, while the code-stream
                identifier indicates the code-stream whose main header is
                being augmented.
           [>>] For the `KDU_TILE_HEADER_DATABIN' class, the in-class
                identifier (`databin_id') holds the tile number, starting
                from 0 for the tile in the upper left hand corner of the
                image.
           [>>] For the `KDU_PRECINCT_DATABIN' class, the in-class identifier
                has the interpretation described in connection with
                `kdu_compressed_source::set_precinct_scope'.
           [>>] For the `KDU_TILE_DATABIN' class, the in-class identifier has
                the same interpretation as for `KDU_TILE_HEADER_DATABIN'.
                Generally, an interactive communication involves either the
                `KDU_TILE_HEADER_DATABIN' and `KDU_PRECINCT_DATABIN' classes,
                or the `KDU_TILE_DATABIN' class, but not both.
           [//]
           New data may be added to the cache in any desired order, which is
           why the absolute location (`offset') and length (`num_bytes') of
           the new data are explicitly specified in the call.  If the new
           data range overlaps with data which already exists in the cache,
           the present function may either overwrite or leave the overlapping
           data bytes as they were.  Both interpretations are assumed to
           produce the same result, since the nominal contents of any
           given data-bin are not permitted to change over time.
         [ARG: is_final]
           If true, the cache is being informed that the supplied bytes
           represent the terminal portion of the data-bin.  If there are no
           holes, the cache representation is complete.  If there are holes,
           the cache representation of the data-bin will be complete once
           these holes have been filled in.  At that time, the
           `get_databin_length' function will set an `is_complete' argument
           to true.
         [ARG: add_as_most_recent]
           If true, the data-bin to which data is added is automatically
           promoted to the head of the "most recently used" list.  Otherwise,
           the data-bin's current position in the list is not affected.  If
           a new data-bin entry needs to be created in the cache and the
           `add_as_most_recent' argument is false, the new entry is placed
           at the tail of the "most recently used" list, not its head.
         [ARG: mark_if_augmented]
           If true, the data-bin's marking flag will be set (future calls to
           `mark_databin' will return true) if and only if the present
           call augments the cached contents of the indicated data-bin.  This
           is useful when adding data obtained from a cache file generated
           during a previous browsing session.  The marked state can then
           be used to keep track of data-bins whose state is not fully known
           by a server.
      */
    KDU_AUX_EXPORT virtual int
      get_databin_length(int databin_class, kdu_long codestream_id,
                         kdu_long databin_id, bool *is_complete=NULL);
      /* [SYNOPSIS]
           Returns the total number of initial bytes which have been cached
           for a particular data-bin.  See `add_to_databin' for an explanation
           of data-bin classes and identifiers.
         [RETURNS]
           If the data-bin has multiple contiguous ranges of bytes, with
           intervening holes, the function returns the length only of the
           contiguous range of bytes which commences at byte 0.
         [ARG: is_complete]
           If non-NULL, this argument is used to return a value indicating
           whether or not the cached representation of the indicated data-bin
           is complete.  For this, the representation must have non holes
           and the `is_final' flag must have been asserted in a previous call
           to `add_to_databin'.
      */
    KDU_AUX_EXPORT virtual void
      promote_databin(int databin_class, kdu_long codestream_id,
                      kdu_long databin_id);
       /* [SYNOPSIS]
            If the indicated data-bin does not have a representation in the
            cache, this function does nothing.  Otherwise, it promotes the
            data-bin to the head of the most recently used list (tail of the
            least recently used list).  See `add_to_databin' for an
            explanation of data-bin classes and identifiers.
       */
    KDU_AUX_EXPORT virtual void
      demote_databin(int databin_class, kdu_long codestream_id,
                     kdu_long databin_id);
       /* [SYNOPSIS]
            If the indicated data-bin does not have a representation in the
            cache, this function does nothing.  Otherwise, it demotes the
            data-bin to the tail of the most recently used list (head of the
            least recently used list).    See `add_to_databin' for an
            explanation of data-bin classes and identifiers.
       */
    KDU_AUX_EXPORT virtual kdu_long
      get_next_codestream(kdu_long stream_id);
      /* [SYNOPSIS]
           Returns the next code-stream for which one or more data-bins
           are available in the cache, after the code-stream identified by
           `stream_id'.  If `stream_id'=-1, the function returns the index
           of the first such code-stream.  If there are no more code-streams
           for which data-bins are available, the function return -1.
      */
    KDU_AUX_EXPORT virtual kdu_long
      get_next_lru_databin(int databin_class, kdu_long codestream_id,
                           kdu_long databin_id, bool only_if_marked=false);
       /* [SYNOPSIS]
            Gets the next less recently used data-bin, within the indicated
            data-bin class and code-stream, after the data-bin identified by
            `databin_id'.  If `only_if_marked' is true, the function skips
            over any data-bins which do not have the marking bit set.
            [//]
            If `databin_id' is negative, or does not refer to an existing
            data-bin with any representation in the cache, the function
            returns the identifier of the MOST recently used data-bin in
            the indicated class.
            [//]
            If there are no data-bins less recently used than that identified
            by `databin_id', the function returns -1.
            [//]
            See `add_to_databin' for an explanation of data-bin classes
            and identifiers.
       */
    KDU_AUX_EXPORT virtual kdu_long
      get_next_mru_databin(int databin_class, kdu_long codestream_id,
                           kdu_long databin_id, bool only_if_marked=false);
       /* [SYNOPSIS]
            Gets the next more recently used data-bin, within the indicated
            data-bin class and code-stream, after the data-bin identified by
            `databin_id'.  If `only_if_marked' is true, the function skips
            over any data-bins which do not have the marking bit set.
            [//]
            If `databin_id' is negative, or does not refer to an existing
            data-bin with any representation in the cache, the function
            returns the identifier of the LEAST recently used data-bin in the
            indicated class.
            [//]
            If there are no data-bins more recently used than that identified
            by `databin_id', the function returns -1.
            [//]
            See `add_to_databin' for an explanation of data-bin classes
            and identifiers.
       */
    KDU_AUX_EXPORT virtual bool
      mark_databin(int databin_class, kdu_long codestream_id,
                   kdu_long databin_id, bool mark_state);
      /* [SYNOPSIS]
           This function provides a convenient service which may be used
           by client applications or derived objects for marking data-bins.
           The interpretation of a mark is not defined here, but the
           service is expected to be used to record whether or not the
           server has already been informed regarding the existing contents
           of the data-bin.  Note carefully that data-bins which do not
           already exist will NOT be created in order to be marked.
           [//]
           See `add_to_databin' for an explanation of data-bin classes
           and identifiers.
         [RETURNS]
           True if the data-bin was already marked; false otherwise.
           Data-bins which are not present in the cache are considered
           unmarked.
         [ARG: mark_state]
           If true, the function causes the data-bin to be marked; otherwise,
           the function causes the data-bin to become unmarked.
      */
    KDU_AUX_EXPORT virtual void clear_all_marks();
      /* [SYNOPSIS]
           Returns all data-bins to the unmarked state, so that `mark_databin'
           will return false.
      */
    KDU_AUX_EXPORT virtual void set_all_marks();
      /* [SYNOPSIS]
           Causes all data-bins for which the cache contains some data to
           be marked, so that `mark_databin' will return true.
      */
  public: // Functions used to read from the cache
    KDU_AUX_EXPORT virtual int
      get_databin_prefix(int databin_class, kdu_long codestream_id,
                         kdu_long databin_id, kdu_byte buf[], int max_bytes);
      /* [SYNOPSIS]
           This function may be used to read the contents of a data-bin.
           It represents a useful alternative to the sequence of
           `set_read_scope' and `read' calls.  The latter require internal
           state management, and so can only be used by one thread at a time.
           The present function, however, involves no state.
           [//]
           See `add_to_databin' for an explanation of data-bin classes
           and identifiers.
         [RETURNS]
           The number of bytes actually written into `buf', or -1 if
           `return_negative_if_waiting' is true and the data-bin is not yet
           complete, and the return value would otherwise be less than
           `max_bytes'.
         [ARG: max_bytes]
           Maximum number of bytes to read into `buf'.
      */
    KDU_AUX_EXPORT virtual void
      set_read_scope(int databin_class, kdu_long codestream_id,
                     kdu_long databin_id);
      /* [SYNOPSIS]
           This function must be called at least once after the object is
           created or closed, before calling `read'.  It identifies the
           particular data-bin from which reading will proceed.  Reading
           always starts from the beginning of the relevant data-bin.  Note,
           however, that the `set_tileheader_scope' and `set_precinct_scope'
           functions may both alter the read scope (they actually do so
           by calling this function).  See `add_to_databin' for an explanation
           of data-bin classes.
           [//]
           It is not safe to call this function while any calls to the
           codestream management machinery accessed via `kdu_codestream'
           are in progress, since they may try to read from the caching data
           source themselves.  For this reason, you will usually want to
           invoke this function from the same thread of execution as that
           used to access the code-stream and perform decompression tasks.
           Use of this function from within a `jp2_input_box' or `jp2_source'
           object is safe, even in multi-threaded applications (so long as
           the synchronization members of `kdu_family_src' are implemented
           by the application).
           [//]
           Before passing a `kdu_cache' object directly to
           `kdu_codestream::create', you should first use the present function
           to set the read-scope to `databin_class'=`KDU_MAIN_HEADER_DATABIN'
           and `databin_id'=0.
      */
    KDU_AUX_EXPORT virtual bool set_tileheader_scope(int tnum, int num_tiles);
      /* [SYNOPSIS]
           See `kdu_compressed_source::set_tileheader_scope' for an
           explanation.  Note that this function essentially just calls
           `set_read_scope', passing in `databin_class'=`KDU_HEADER_DATABIN'
           and `databin_id'=`tnum', while using the same code-stream
           identifier which was involved in the most recent call to
           `set_read_scope'.  The function always returns true.
      */
    KDU_AUX_EXPORT virtual bool set_precinct_scope(kdu_long unique_id);
      /* [SYNOPSIS]
           See `kdu_compressed_source::set_precinct_scope' for an
           explanation.  Note that this function essentially just calls
           `set_read_scope', passing in `databin_class'=`KDU_PRECINCT_DATABIN'
           and `databin_id'=`unique_id', while using the same code-stream
           identifier which was involved in the most recent call to
           `set_read_scope'.  The present function always returns true.
      */
    KDU_AUX_EXPORT virtual int read(kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           This function implements the functionality required of
           `kdu_compressed_source::read'.  It reads data sequentially from
           the data-bin indicated in the most recent call to
           `set_read_scope' -- note that `set_read_scope' may have been
           invoked indirectly from `set_tileheader_scope' or
           `set_precinct_sope'.  If `set_read_scope' has not yet been called,
           since the object was created or since the last call to `close',
           this function returns 0 immediately.
         [RETURNS]
           The number of bytes actually read and written into the supplied
           `buf' buffer.  This value will never exceed `num_bytes', but may
           be less than `num_bytes' if the end of the data-bin is encountered.
           After the end of the data-bin has been reached, subsequent calls
           to `read' will invariably return 0.
      */
  public: // Other base function overrides
    virtual int get_capabilities()
      { return KDU_SOURCE_CAP_CACHED | KDU_SOURCE_CAP_SEEKABLE; }
      /* [SYNOPSIS]
           This object offers the `KDU_SOURCE_CAP_CACHED' and
           `KDU_SOURCE_CAP_SEEKABLE' capabilities.  It does not support
           sequential reading of the code-stream.  See
           `kdu_compressed_source::get_capabilities' for definitions of
           the various capabilities which compressed data sources may
           advertise.
      */
    KDU_AUX_EXPORT virtual bool seek(kdu_long offset);
      /* [SYNOPSIS]
           Implements `kdu_compressed_source::seek'.  As explained there,
           seeking within a cached source moves the read pointer relative
           to the context of the current data-bin only.  An `offset' of 0
           refers to the start of the current data-bin, which may be the
           code-stream main header, a tile header, or a precinct.
      */
    KDU_AUX_EXPORT virtual kdu_long get_pos();
      /* [SYNOPSIS]
           Implements `kdu_compressed_source::get_pos', returning the location
           of the read pointer, relative to the start of the current context
           (current data-bin).  See `seek' for more information.
      */
  public: // Synchronization
    virtual void acquire_lock() { mutex.lock(); }
      /* [BIND: callback]
         [SYNOPSIS]
           This function, together with `release_lock', is used to guard
           access to the cached data.  From Kakadu v6.2, this function
           actually locks an internal mutex, so there is no longer any
           need to override the virtual function in a descendant class
           which requires multi-threaded access -- of course, there is no
           harm in doing so.  You should not normally need to call this
           function explicitly.
      */
    virtual void release_lock() { mutex.unlock(); }
      /* [BIND: callback]
         [SYNOPSIS]
           See comments appearing with the `acquire_lock' function.
      */
  public: // Statistics reporting functions
    KDU_AUX_EXPORT kdu_long get_peak_cache_memory();
      /* [SYNOPSIS]
           Returns the total amount of memory used to cache compressed data.
           This includes the cached bytes, as well as the additional state
           information required to maintain the state of the cache.  It
           includes the storage used to cache JPEG2000 packet data as well as
           headers.
      */
    KDU_AUX_EXPORT kdu_long get_transferred_bytes(int databin_class);
      /* [SYNOPSIS]
           This function returns the total number of bytes which have been
           transferred into the object within the indicated data-bin class.
           See `add_to_databin' for an explanation of data-bin classes.
      */
  private: // Data
    kd_cache *state; // Hides the object's real storage.
    kdu_cache *attached; // Set by `attach_to'.
    kdu_mutex mutex;
  };

#endif // KDU_CACHE_H
