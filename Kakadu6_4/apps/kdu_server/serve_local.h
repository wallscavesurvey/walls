/*****************************************************************************/
// File: serve_local.h [scope = APPS/SERVER]
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
   Header file providing internal implementation structures and classes for
the platform independent core services of the Kakadu server application.
******************************************************************************/

#ifndef SERVE_LOCAL_H
#define SERVE_LOCAL_H

#include <assert.h>
#include "kdu_serve.h"

// Important constants which affect behaviour of `kdu_serve'
#define KD_MAX_ACTIVE_CODESTREAMS 4
#define KD_MAX_WINDOW_CODESTREAMS 256
#define KD_MAX_ATTACHED_CODESTREAMS 6
#define KD_MAX_WINDOW_SAMPLES 1000000 // Total samples in active precinct bins

// Defined here:
class kd_chunk_output;
struct kd_binref;
struct kd_active_binref;
struct kd_chunk_binref;
struct kd_meta;
struct kd_stream;
struct kd_tile;
struct kd_tile_comp;
struct kd_resolution;
struct kd_precinct_block;
struct kd_precinct;
struct kd_active_precinct;
struct kd_window_context;
struct kd_sequencing_state;
struct kd_codestream_window;
struct kd_serve;
class kd_chunk_server;
class kd_binref_server;
class kd_active_precinct_server;
class kd_pblock_server;


/*****************************************************************************/
/*                               kd_chunk_output                             */
/*****************************************************************************/

class kd_chunk_output : public kdu_output {
  /* This class, provides `kdu_output' derived services for
     transferring compressed data and headers to a list of `kds_chunk'
     objects. */
  public: // Member functions
    kd_chunk_output()
      { chunk = NULL; }
    void open(kds_chunk *first_chunk, int skip_bytes,
              kd_chunk_server *extra_server=NULL)
      { /* After this function returns and prior to the `close' call,
           calls to the base object's `put' member function will augment the
           data already in `first_chunk', as indicated by the value of its
           `chunk_bytes' field on entry, skipping the indicated number of
           initial bytes supplied to `put'.  If additional chunks are
           required, the function will first try to follow the `next' links
           from `first_chunk', in each case augmenting the bytes which may
           already be stored in that buffer.  If no more buffers remain in
           the list attached to `first_chunk', further output data will
           simply be discarded. */
        this->chunk = first_chunk;
        this->skip_bytes = skip_bytes;
      }
    kds_chunk *close()
      /* Returns a pointer to the last chunk to which any information
         was written. */
      {
        flush_buf();
        kds_chunk *result=chunk; chunk = NULL;
        return result;
      }
  protected: // Implementation of base virtual member function
    virtual void flush_buf();
  private: // Data
    kds_chunk *chunk;
    int skip_bytes;
  };

/*****************************************************************************/
/*                                 kd_binref                                 */
/*****************************************************************************/

struct kd_binref {
  public: // Data
    kd_stream *stream; // See below
    kd_tile *tile; // See below
    kd_meta *meta; // If data-bin refers to a metadata-bin
    kd_precinct *precinct; // If data-bin refers to a precinct data-bin
    kd_binref *next;
  };
  /* Notes:
        This structure is derived into one of `kd_active_binref' or
     `kd_chunk_binref'.  In each case, it is used to refer to a single
     data-bin.
        The `stream' member must be non-NULL for any codestream-related
     data-bin -- i.e., anything other than a meta data-bin.  If `stream'
     is non-NULL, but `tile' and `precinct' are both NULL, the data-bin in
     question is the codestream's main header data-bin.  Note that a
     `kd_stream' object may not be destroyed (nor may any of its `kd_tile' or
     `kd_precinct' objects) so long as a `kd_binref' is referencing it.  To
     ensure that this is the case, each `kd_stream' maintains a count of the
     number of `kd_binref' objects which reference it.
        The `tile' member must be non-NULL for any tile or precinct data-bin.
     If `precinct' is NULL, but `tile' is non-NULL, the data-bin is a tile
     header data-bin.
        The `precinct' member points to the precinct associated with
     a precinct data-bin.  If the precinct happens to be active, it can
     be discovered by de-referencing `precinct->active'.  If
     `stream->tiles' is NULL, the `precinct' member must not be de-referenced.
  */

/*****************************************************************************/
/*                              kd_active_binref                             */
/*****************************************************************************/

struct kd_active_binref : public kd_binref {
  public: // Functions
    kd_active_binref *get_next() { return (kd_active_binref *) next; }
  public: // Data
    int log_relevance; // 256*log_2(relevance frac) -- but see below
    union {
      int num_active_bytes; // Only for meta data-bin references -- see below
      int num_active_layers; // Only for precinct data-bin references
    };
  };
  /* Notes:
        This structure is used to build a list of active data-bins for each
     window context (see `kd_window_context'), identifying the data-bins for
     which information can be generated by
     `kd_context_window::generate_increments'.
        The `log_relevance' value is used to determine a priority for
     sequencing data from this data-bin during calls to
     `kd_window_context::generate_increments'.  For precinct data-bins, this
     value represents 256 * the log of the precinct's relevance fraction,
     which is essentially the fraction of the precinct's total samples which
     overlap with the client's window of interest -- the value is always <= 0.
     For tile and codestream header data-bins, this value is always equal
     to 65535, which ensures that these data-bins always get sequenced
     first, no mattter what.  For meta data-bins, the value is derived from
     `kd_meta::sequence', together with the absolute priority of the metadata
     portion of the relevant window request.  If the metadata is requested
     with absolute priority, it is assigned a `log_relevance' value of 65535.
     Otherwise, the value may be smaller.  When determining incremental
     contributions for precinct data-bins, the
     `kd_window_context::simulate_increments' function adds `log_relevance'
     to the relevant `kd_stream::active_layer_log_slope' entry before
     comparing with the current value of `kd_window_context::active_threshold'.
     However, for non-precinct data-bin references, the `log_relevance' value
     is compared directly with `kd_window_context::active_threshold'.
        The `num_active_bytes' member refers to the total number of bytes for
     the data-bin (starting from the first byte of the data-bin) which are
     relevant to the window context -- this value is of interest only for
     meta data-bins.
        The `num_active_layers' member refers to the total number of quality
     layers (or packets) for a precinct data-bin, which are relevant to the
     codestream window which generated this data-bin reference.
  */

/*****************************************************************************/
/*                               kd_chunk_binref                             */
/*****************************************************************************/

struct kd_chunk_binref : public kd_binref {
  public: // Functions
    kd_chunk_binref *get_next() { return (kd_chunk_binref *) next; }
  public: // Data
    int prev_bytes; // Number of bytes previously associated with data-bin
    kdu_uint16 prev_packets; // Relevant only for precinct data-bin references
    kdu_uint16 prev_packet_bytes; // Also only for precinct data-bin references
  };
  /* Notes:
        This structure is used to build a list of delivered data-bins for
     each `kds_chunk' object which contains a generated chunk of data to be
     delivered to a client.  This list can be used to restore the cache model
     to a suitable state in the event that the chunk of data is abandoned
     in the future.
        `prev_bytes' refers to the number of bytes from the data-bin which
     were in the client cache model prior to the generation of increments
     which resulted in the construction of that data chunk.  If an abandoned
     chunk is returned to `kdu_serve::release_chunks', the
     cache model for any data-bins identified in that chunk can be updated
     with the aid of the `prev_bytes' value.
        Specifically, if only `stream' is non-NULL,
     `stream->dispatched_header_bytes' is set to the value of `prev_bytes'
     unless `stream_dispatched_header_bytes' already has an even smaller
     value.  Similarly, if `tile' is non-NULL but `precinct' is NULL,
     `tile->dispatched_header_bytes' is set to `prev_bytes', unless it
     is already smaller.  If `meta' is non-NULL, `meta->dispatched_bytes' is
     reduced to `prev_bytes' unless it is already smaller.  For precinct
     data-bins, the `prev_bytes', `prev_packets' and `prev_packet_bytes'
     members together communicate the state of the precinct data-bin prior
     to the increments represented by the data chunk in which this reference
     is found.
  */

/*****************************************************************************/
/*                                 kd_meta                                   */
/*****************************************************************************/

struct kd_meta {
  public: // Member functions
    kd_meta()
      { metagroup=NULL; is_modeled=0;
        next_in_bin=prev_in_bin=phld=parent=active_next=next_modeled=NULL; }
    ~kd_meta()
      { // Recursively deletes the sub-trees.
        kd_meta *mp;
        while ((mp=phld) != NULL)
          { phld = mp->next_in_bin; delete mp; }
      }
    int init(const kds_metagroup *metagroup, kd_meta *parent, kdu_long bin_id,
             int bin_offset, int tree_depth);
      /* Recursive function, initializes a newly constructed `kd_meta'
         object to associate itself with the supplied `metagroup' and
         then recursively creates the placeholder list if `metagroup->phld'
         is non-NULL.  Returns the total number of whole meta data-bins
         associated with the `kds_metagroup' object and all its descendants
         (via `phld' links).
            The `bin_offset' argument holds the offset from the start of the
         data-bin until the first byte of the current metagroup.  The
         `tree_depth' argument holds 0 if `bin_id'=0 and larger values for
         each successively deeper data-bin in the hierachy.  `tree_depth'
         also corresponds to the depth of the top level boxes in the
         metadata-bin within the JP2 box hierarchy. */
    void resolve_links(kd_meta *tree);
      /* Recursive function passes through the current node and all of its
         descendants, looking for nodes which may need a non-NULL
         `link_target' member -- i.e., nodes whose `metagroup->link_fpos'
         value is strictly positive.  For each such node, a search is
         conducted for the most appropriate link target by examining the
         entire `tree'. */
    bool find_active_scope_and_sequence(kdu_window &window,
                                  kd_codestream_window *first_window,
                                  kd_codestream_window *lim_window,
                                  int max_sequence=KDS_METASCOPE_MAX_SEQUENCE,
                                  int *accumulating_byte_limit=NULL,
                                  int recursion_depth=0);
      /* Tests the scope of the current object against the supplied `window',
         setting the `in_scope' and `sequence' members accordingly.  If
         `window.metareq' is non-NULL, the function scans the list of metadata
         requests to determine whether or not a metadata group should be
         considered in scope and, if so, how much of it is in scope.
         Otherwise, everything which can be considered relevant to the
         window region is considered in scope.  When evaluating the scope,
         `first_window' is considered to be the head of a list of active
         codestream windows for some window context, which is traversed up
         until, but not including, the point at which `lim_window' has been
         encountered -- if `lim_window' is NULL, the entire list is traversed
         from `first_window'.
            If the `phld' member is non-NULL, the function recursively passes
         through the metagroup list associated with the placeholder,
         evaluating the scope and sequence of those elements.  If any
         are in scope, the present object must also be brought into scope
         and its sequence priority adjusted to be at least as low (low
         sequence means high priority) as that of the in-scope placeholder
         elements.  Once an in-scope element is found, the function also
         scans backwards through the list of elements which belong to the
         same data-bin, setting them all to be in scope, with sequence
         numbers no larger than that found for the current element.  This
         is important since data-bins are delivered in linear fashion.
            The `max_sequence' argument limits the maximum value for the
         `sequence' member of this node.  This is set to a non-trivial value
         if the containing box is requested in a manner which also requires
         content from its descendants.
            If `accumulated_byte_limit' is non-NULL, at least some bytes
         from the current node are requested.  The number of bytes from this
         node's box contents is given by *`accumulated_byte_limit' and the
         current function must decrement *`accumulated_byte_limit' by the
         number of bytes it is actually able to contribute (including bytes
         contributed by its descendants).
            If `recursion_depth' is >= `tree_depth', the headers of all
         boxes found at the top level of this node are required.  When
         the function is invoked on the root of the metadata tree, the
         value `recursion_depth'=0 ensures that all box headers found within
         meta-databin 0 are forced in scope -- something required by the
         JPIP standard.  The value of `recursion_depth' is increased to the
         `dynamic_depth' of any recursive metareq instruction which brings
         the current node into scope -- and this increased recursion depth
         is passed down to our descendants.
            The function returns true if it places any metagroup in scope
         which has a non-NULL `link_target' member.  This means that once
         the recursive function has completed, we will have to invoke
         `add_active_link_targets_to_scope' prior to invoking the
         `include_active_groups' function. */
    void add_active_link_targets_to_scope();
      /* Recursive function which examines the current node and all its
         descendants which are `in_scope', looking for any which have a
         non-NULL `link_target'.  Upon discovering a non-NULL link target,
         the function moves the `link_target' metagroup into scope, along
         with its containing metagroups and previous sibling metagroups.
         In the process of doing this, we can be sure that we will not
         add any metagroup into scope which itself has a non-NULL `link_target'
         (that would require iterative re-application of this function).  The
         reason we can be sure of not doing this is that non-NULL
         `link_target' members are only found in metagroups which hold the
         contents of cross-reference boxes, but no other cross-reference
         box may point into a cross-reference box, and the contents of each
         cross-reference box are assigned their own data-bin so that there
         is no need to serve up any cross-reference box's contents in order
         to satisfy the prefix condition for metadata-bin delivery. */
    void include_active_groups(kd_meta *start);
      /* Checks to see if the object is in scope (as discovered by
         `find_active_scope_and_sequence' or
         `add_active_link_targets_to_scope').  If so, it is added into
         the active list at a place starting no earlier than that identified
         by `start', which is necessarily non-NULL.  Since `start' must be
         non-NULL, it is possible that the caller needs to make `start'
         reference the current object.  The implementation needs to be
         aware of this possibility to avoid creating self-referential loops.
            The function recursively passes through the metagroups attached
         to a non-NULL `phld' link, including them also in the active list,
         so long as they are in scope.  Each time elements are included in
         the active list, they are included in such a way as to ensure that
         sequence numbers are non-decreasing as the list is traversed.
         The active list is connected via the `active_next' members.
            It is imperative that you call this function after calls to
         `find_active_scope_and_sequence' and (optionally)
         `add_active_link_targets_to_scope' so that the `in_scope'
         flags for all `kd_meta' nodes in the hierarchy can be left in the
         false state for next time. */
  public: // Data which is valid from all window contexts
    kdu_long bin_id; // metadata-bin identifier
    int tree_depth; // Depth of top-level boxes in the bin within box hierarchy
    const kds_metagroup *metagroup;
    kd_meta *next_in_bin; // Parallels `kd_metagroup::next'
    kd_meta *prev_in_bin; // Points to previous element in the same data-bin
    kd_meta *phld; // Same as `kds_metagroup::phld' for this parallel structure
    kd_meta *parent; // `kd_meta' object in whose `phld' box we fall, if any
    kd_meta *link_target; // See below
    kdu_int32 bin_offset; // Offset from start of data-bin to this group
    kdu_int32 num_bytes; // Same as `metagroup->length'
    kdu_int32 dispatched_bytes; // Num bytes already sent out
    kdu_int32 sim_bytes; // See `kd_window_context::simulate_increments'
    kdu_byte can_complete; // If the end of this object ends its data-bin
    kdu_byte dispatch_complete; // If data-bin complete in client cache
    kdu_byte sim_complete; // See `kd_window_context::simulate_increments'
    kdu_byte is_modeled; // See below
  public: // Data which is computed for a single window context
    bool in_scope; // True if the metagroup lies within current window's scope
    int max_content_bytes; // See below
    int sequence; // Sequence priority if `in_scope' is true
    int active_length; // Max bytes to be sent, based on `max_content_bytes'
  public: // Dynamic links
    kd_meta *next_modeled; // See below
    kd_meta *active_next; // For linking active metadata list in `kdu_serve'
  };
  /* Notes:
        The `max_content_bytes' member identifies an upper bound on the
     number of content bytes which are of interest to the client from
     any JP2 box represented by this object.  In practice, the server
     will attempt to deliver all bytes up to and including the header of
     the last JP2 box represented by the `kds_metagroup' object, plus
     up to `max_content_bytes' additional bytes, coming from the contents
     of the last JP2 box in the group.  The `kds_metagroup' object
     essentially controls the granularity with which metadata may be
     selectively disseminated.  Note that `max_content_bytes' is adjusted
     along with `in_scope' and `sequence', whenever the
     `find_active_scope_and_sequence' member function is called.
        The `link_target' member refers to a `kd_meta' object whose metagroup
     is to be served (all contents, but descendants are not required) if any
     content bytes of the present metagroup are served.  This member is NULL
     unless the `metagroup->link_fpos' value from which it derives is
     non-negative.  Specifically, a search is conducted for the lowest
     level metagroup in the hierarchy, which contains the header of a
     box in whose contents the `metagroup->link_pos' member is found.  The
     entire contents of that group must be delivered to be sure of providing
     enough information for the client to recover the headers of all boxes
     which contain the linked location.
        To assist in the implementation of temporary cache models for
     processing of stateless requests, the `is_modeled' and `next_modeled'
     members are provided.  The `is_modeled' member becomes non-zero when
     the `kd_meta' object is first referenced by a `kd_active_binref' entry.
     At that point also, the `is_modeled' flag is set to true for all
     `kd_meta' objects which belong to the same meta data-bin, and the
     last `kd_meta' object in the data-bin (the one with `can_complete'
     non-zero) is linked into the `kd_serve::modeled_metabins' list via its
     `next_modeled' links.  Each time a new stateless request is about to be
     processed (or one has been fully processed), the entire list of modeled
     `kd_meta' objects is disassembled, resetting the `is_modeled' flags to 0
     and also resetting the cache model values to reflect an empty cache model.
     The `is_modeled' status also allows the internal machinery to determine
     whether cache model manipulation instructions have already been applied
     to a `kd_meta' object so that they need not be applied again.  This is
     important, because `kd_meta' objects might be visited multiple times
     during successive calls to `kd_window_context::sequence_active_bins' and
     we only want to apply cache model manipulation instructions once -- this
     is easy to do for atomic instructions, but stateless requests may use
     non-atomic instructions which apply to a range of data-bins and cannot
     generally be erased as they are applied.
  */

/*****************************************************************************/
/*                                 kd_stream                                 */
/*****************************************************************************/

struct kd_stream {
  public: // Member functions
    kd_stream(kd_serve *owner);
    ~kd_stream();
    void initialize(kdu_serve_target *target, int stream_id,
                    bool ignore_relevance_info);
      /* This function recovers all the summary information for the stream
         which can be obtained without actually opening any `kdu_codestream'
         interfaces, leaving the object in the "collapsed" and "unattached"
         state, with a NULL `tiles' member. */
    void construct_interfaces();
      /* This function constructs the `interchange' and `structure'
         codestreams, constructing the `tiles' array (i.e., a cache model) if
         it does not already exist.  The function assumes that the `source'
         codestream interface exists.  A lot of the construction work may
         be deferred until a subsequent call to `init_tile'. */
    void deconstruct_interfaces(bool leave_expanded);
      /* This function closes any open tile interfaces and then destroys
         the main `interchange' and `structure' codestreams.  If
         `leave_expanded' is false, the function also removes the entire
         `tiles' array, leaving the stream without any cache model information
         apart from that associated with the main codestream header.  The
         function is used to move the stream into the "Lightweight Expanded"
         state or the "Collapsed" state.  The function must not be invoked
         on a stream which is in the active state or is associated with a
         codestream window.  The function must also not be invoked on a stream
         which is in the "attached" state -- that is, the `source' interface
         must already be empty. */
    void ensure_expanded();
      /* Invoked from various places, this function ensures that the object
         is in the "expanded" state -- it does nothing if the stream is
         already expanded. */
    kd_tile *access_tile(kdu_coords idx);
      /* Ensures that the stream is at least in the "expanded" state and
         returns a pointer to the tile whose absolute index is `idx'.  If
         the tile is being accessed for the first time, this function makes
         sure at least that tile header information is imported into the
         `interchange' codestream so that the header size can be computed,
         allowing meaningful interaction with cache model state for the
         tile header data-bin.  For efficiency reasons, the function leaves
         any opened codestream interfaces open, so if you are finished
         using them, you should call `close_tile', even if you do not
         first invoke `open_tile'. */
    void open_tile(kd_tile *tp, bool attach_to_source,
                   kdu_dims *structure_region);
      /* If `structure_region' is non-NULL, the function opens the
         `kdu_tile::structure' interface after first creating (if
         necessary) the `kd_stream::structure' codestream and invoking
         `kdu_codestream::apply_input_restrictions' to limit access to
         just the `structure_region' region of interest.  The object maintains
         an internal record of the `structure_region' which was last passed
         to `apply_input_restrictions' so that the function need only be
         invoked when something changes.  With `structure_region' non-NULL,
         neither the `kd_tile::interchange' nor `kd_tile::source' interfaces
         are explicitly opened by this function.
            Otherwise, the function opens the `kd_tile::interchange' interface
         and the `source' interface (if `attach_to_source' is true) as
         required.  These operations may require the stream to be attached
         to the underlying `kdu_serve_target' object and/or the
         `interchange' codestream to be created for the first time.
            Rather than directly closing any of the `kdu_tile' interfaces
         opened by this function, you should invoke `close_tile' as soon
         as you are finished with the tile within the context in which you
         opened it.  As an alternative, you may invoke `close_attached_tiles'
         to close all tiles which have an open `kd_tile::source' interface.
         These operations ensure that tile interfaces will be closed
         if possible, but left open if there are other users of those
         interfaces.  In particular, if any `kd_active_binref' is holding a
         reference to the tile or any of its precincts (via a
         `kd_active_precinct' object), the interchange interface to the tile
         will not be closed -- but this will happen automatically when all
         such active data-bin references have been released.
      */
    void close_tile(kd_tile *tp);
      /* This function does not destroy the `kd_tile' object itself, but
         closes whatever open `kdu_tile' interfaces are no longer needed.
         It is best to match each call to `open_tile' with a call
         to the returned tile's `close' function -- although this is not
         strictly necessary.  Do not invoke `kdu_tile::close' directly on
         `interchange' or `source' interfaces in particular, since these
         require book keeping to be performed.
            You should also invoke this function after `access_tile', if you
         do not intend to invoke `open_tile' immediately, since the initial
         call to `access_tile' may open tile interfaces and leave them open.
            For convenience, the `open_tile' function keeps track of tiles
         which are opened with the `attach_to_source' option, so you can
         close them all in one go with a call to `close_attached_tiles'.  This
         is particularly important, because tiles which are attached to the
         `source' codestream are interfaces to a locked shared resource in
         the `kdu_serve_target'; they all need to be closed before the
         resources can be unlocked for other threads to access. */
    void close_attached_tiles();
      /* As mentioned above, this function effectively invokes `close_tile'
         on all tiles which have been opened with the `attach_to_source'
         option. */
    void adjust_completed_tiles(kd_tile *tp, bool was_complete);
      /* This function is called if something has happened which may affect
         whether or not `tp' has been completely dispatched, as identified
         by `tp->completed_precincts' and `tp->dispatched_header_complete'.
         If `was_complete' is false, the function checks to see if the tile
         is now complete -- if so, the number of completed tiles for the
         associated `kd_stream' is adjusted and
         `kd_serve::adjust_completed_streams' is called to propagate the
         impact of changes in tile completeness to stream completeness. */
    void erase_cache_model();
      /* This function is used in stateless operation to efficiently erase
         any cache model associated with the stream, either prior to
         responding to a new stateless request, or after we have finished
         responding to a current stateless request (at least as far as this
         codestream is concerned).  The function resets the `is_modeled',
         `dispatched_header_bytes' and `dispatched_header_complete' members
         and then visits all the tiles found on the `modeled_tiles' list,
         removing all cache model information from them.  Unlike
         `deconstruct_interfaces', this function does not actually clean up
         interchange codestreams, since they may be used in a subsequent
         stateless request. */
    void process_header_model_instructions(int tnum, int i_buf[], int i_cnt);
      /* This function processes cache model manipulation instructions which
         relate to the codestream main header (`tnum' < 0) or a tile header
         data-bin.  If necessary, the tile is instantiated prior to processing
         of the cache model instructions; this also applies to the stream
         itself, if it is in a collapsed state.  Cache model instructions have
         been pre-digested by `kdu_window_model' and converted into a sequence
         of `i_cnt' integers, where `i_cnt' will be either 1 or 2.  The
         integers themselves are found in `i_buf'.  Each integer represents
         either and additive or subtractive cache model operation, as explained
         in connection with `kdu_window_model::get_header_instruction'. */
    void process_precinct_model_instructions(int tnum, int cnum, int rnum,
                                             kdu_long pnum, int i_buf[],
                                             int i_cnt, bool additive);
      /* Similar to the above, except this function processes precinct
         model manipulation instructions.  Most precinct model instructions
         are handled efficiently by `kd_window_context::generate_active_bins'
         but those which cannot be handled there may need to be handled
         through this function.  The function needs to be prepared for the
         fact that the stream may be collapsed or the relevant tile may
         not yet have been initialized.  Neither of these operations might
         be required if the model manipulation instructions are purely
         subtractive.  In view of this, the `additive' argument provides
         advance information about whether the `i_buf' array contains any
         additive cache model manipulation instructions. */
  private: // Helper functions
    void init_tile(kd_tile *tp, kdu_coords idx);
      /* This function is called if `comps' is NULL or `have_params' is false.
         In the former case, the tile has no cache model and neither the
         `interchange' nor `structure' interfaces have ever been opened, so
         `have_params' is also necessarily false.  It is possible that
         `comps' is non-NULL, but `have_params' is false, in which case we
         have a cache model which was preserved even after the interchange
         codestreams were destroyed.   This function is only ever called from
         within `open_tile'.  The `idx' argument holds the absolute tile
         indices, as supplied to `open_tile'. */
  public: // Global identification information
    kd_serve *serve;
    int stream_id;
  public: // Summary information recovered from `kdu_serve_target' without
          // opening any actual codestream
    kdu_dims image_dims; // Dimensions of full image
    kdu_dims tile_partition; // pos=tile origin on canvas; size=tile size
    int num_components; // Number of codestream image components
    kdu_coords *component_subs; // Codestream image component sub-sampling
    int num_output_components; // Number of output image components
    kdu_coords *output_component_subs; // Output image component sub-sampling
    int total_tiles;
    kdu_dims tile_indices; // Range of indices used for accessing tiles
    kdu_long pid_gap; // Product of `num_components' and `total_tiles'
    int max_discard_levels; // <=32; same as min DWT levels over all tile-comps
    int max_quality_layers; // Max quality layers, over all tile-comps
    int num_layer_slopes; // Number of entries in the `layer_log_slopes' array
    int *layer_log_slopes; // See below
    const int *active_layer_log_slopes; // See below
  public: // Summary dynamic state information
    int dispatched_header_bytes; // Initial main header bytes transferred.
    int sim_header_bytes; // See `kd_window_context::simulate_increments'
    kdu_byte dispatched_header_complete;// If data-bin complete in client cache
    kdu_byte sim_header_complete; // See `simulate_increments'
    int completed_tiles; // Total tiles whose contents are fully transferred
    bool is_modeled; // See below
    bool is_locked; // If codestream locked, except in `generate_increments'
    kdu_dims last_structure_region; // See description of `open_tile'
  public: // Referencing information
    kd_codestream_window *windows; // List of codestream windows using us
    int num_active_contexts; // Num window contexts in which stream is active
    int num_chunk_binrefs; // Number of references from `kds_chunk::bins' lists
  public: // Codestream interfaces
    kdu_codestream source; // Empty if not attached to `serve_target'
    kdu_codestream interchange; // Used for transcoding -- see below
    kdu_codestream structure; // Used for determining data-bins -- see below
  public: // Parameters which are valid only in the "expanded" state
    int main_header_bytes; // Size of the main header
    kd_tile *tiles; // Array of tiles, in raster order -- NULL if collapsed
  public: // Links
    kd_tile *attached_tiles; // Doubly-linked list of tiles with open `source'
    kd_tile *modeled_tiles; // List of tiles with `kd_tile::is_modeled' true
    kd_stream *next, *prev; // Used to build doubly-linked lists
  };
  /* Notes:
        `kd_stream' maintains various kinds of information for a single
     codestream.  In particular, it maintains the cache model information
     for the codestream, global summary information about the codestream,
     an interchange `kdu_codestream' object for dynamic construction of
     header and precinct data-bins, a second interchange codestream
     (`structure') for converting Window of Interest (WOI) coordinates into
     corresponding data-bins, and a reference (`source') to the
     `kdu_codestream' object managed by the ultimate `kdu_serve_target'.
     Not all of this information need be present, depending on the current
     state of existence.  Streams have the following states of existence:
     [>>] Collapsed -- in this state, summary information about the codestream
          has been recovered directly from `kdu_serve_target' without actually
          opening any codestream itself.  There is no cache model information,
          no `interchange' or `structure' codestream, and no association with
          any shared resources offered via the `kdu_serve_target' object.
          Collapsed codestreams cost very little to keep around, while
          preserving some basic information, which can be used to estimate
          the memory impact associated with a client request.  A collapsed
          codestream has a NULL `tiles' array.
     [>>] Expanded but Lightweight -- in this state, the `tiles' array is
          populated and capable of storing cache model information for the
          codestream.  However, none of the `kdu_codestream' interfaces is
          valid and there are no active representations of any of the
          codestream entities.  None of the `serve_target's resources
          are occupied in this mode.
     [>>] Expanded and Mediumweight -- in this state, the `tiles' array is
          populated and the `structure' and `interchange' codestreams are
          also available; these may occupy significant additional resources,
          depending on the internal representation of the interchange
          codestreams.  However, the `source' interface is empty, so there is
          no consumption of shared resources that are managed by the
          `serve_target'.
     [>>] Attached but Unlocked -- in this state, the `source' interface
          is non-empty, meaning that the object is attached to live
          `kdu_codestream' machinery which is managed by the `serve_target'.
          This may be expensive, so it's a good idea to avoid keeping too
          many attached codestreams.
     [>>] Locked -- in this state, the `source' interface is locked for
          exclusive access by the current working thread.  In this mode, the
          individual tiles can open their `kd_tile::source' interfaces and
          drag data into their interchange codestreams for the generation of
          data increments.
     [//]
     The `is_modeled' member is set to true when this stream is first accessed
     in a way which involves cache modeling of any form.  In practice, it
     is set only by `kd_codestream_window::sequence_active_bins' and reset
     only by `erase_cache_model'.
     [//]
     In addition to the above states of existence, a stream may be in the
     "active" state (corresponding to `num_active_contexts' > 0).  Throughout
     this documentation, the term "active" is reserved for objects for which
     data increments may be generated.  If a `kd_stream' is to be actively
     used, it will have to be attached to the `kdu_serve_target'.  However,
     it is possible to transition in and out of the active state without
     actually having any increments generated.  For this reason, a `kd_stream'
     which is in the active state must be expanded (i.e., must have a non-NULL
     `tiles' array) but need not immediately have a valid `source' interface.
     Conversely, a `kd_stream' which becomes inactive may remain attached
     for some time, so as to avoid deleting and renewing resources in the
     `serve_target' when the stream next needs to be actively used.
     [//]
     The reference counters are used to determine whether or not a `kd_stream'
     object may change its level of existence.  `num_window_refs' counts the
     number of `kd_codestream_window' objects which reference the stream.  If
     this value drops to zero, the stream may return to the collapsed state if
     desired.  `num_active_contexts' counts the number of `kd_window_context'
     objects which currently have the stream listed in their
     `active_streams' array.  If this count drops to zero, the stream may
     enter the Mediumweight or even Lightweight expanded state if desired,
     although it may be costly to re-attach the codestream to the
     `serve_target' -- so it should be detached only if resources are running
     low.  The `num_chunk_binrefs' member counts the number of `kd_binref'
     objects found in the `kds_chunk::bins' list managed by data chunks, which
     reference the stream.  The stream may not be totally destroyed unless all
     three counters reach zero.
     [//]
     `layer_log_slopes' is NULL if the codestream contains no information
     about the distortion-length slope thresholds which were used to create
     its quality layers.  Otherwise, this member points to an array with
     `num_layer_slopes' entries.  The value of `layer_log_slopes'[i] is
     equal to 256*(256-64+log_2(`slope')) where `slope' is the distortion
     length slope threshold associated with quality layer i.  Specifically,
     the slope threshold corresponds to the threshold applied to the
     ratio Delta_D / Delta_L, where Delta_D measures change in total
     squared error distortion for samples normalized to a nominal range of
     1.0, while Delta_L measures change in code-length in bytes.
     Note that the information in this array may be augmented with
     "dummy" slopes for hypothetical extra quality layers.  In particular,
     we generally need at least 1 more log slope value than
     `max_quality_layers'.  The values in this array are non-increasing
     and there are guaranteed to be at least `max_quality_layers'+1 of them.
     [//]
     The `active_layer_log_slopes' member either points to `layer_log_slopes'
     or to `kd_window_context::dummy_layer_log_slopes'.  In any event, it is
     configured to hold a non-NULL value by the function
     `kd_window_context::install_active_layer_log_slopes' which is invoked
     within `kd_window_context::simulate_increments'.  This allows the
     active slopes to potentially change each time the set of active streams
     changes -- important, because if some of the active streams have their
     own layer-log-slope information, while others do not, the complete set
     must use the `kd_window_context::dummy_layer_log_slopes' array for
     consistency.
  */

/*****************************************************************************/
/*                                  kd_tile                                  */
/*****************************************************************************/

struct kd_tile {
  public: // Functions
    kd_tile()
      {
        have_params=false; comps=NULL;
        num_layers=header_bytes=dispatched_header_bytes=sim_header_bytes=0;
        dispatched_header_complete=sim_header_complete=is_modeled=uses_ycc=0;
        total_precincts=completed_precincts=0;
        num_active_precincts=num_tile_binrefs=0;
        next_attached = prev_attached = next_modeled = NULL;
        modeled_resolutions = NULL;
      }
  public: // Data
    bool have_params; // If initialized since interchange codestream creation
    kdu_coords t_idx; // Absolute coordinates of tile in source code-stream.
    kd_stream *stream; // Code-stream to which the tile belongs
    int tnum; // Code-stream tile index, starting from 0 for top-left tile
    kdu_tile source; // Interface into the `stream->source' codestream
    kdu_tile interchange; // Interface to the `stream->interchange' codestream
    kdu_tile structure; // Interface into the `stream->structure' codestream
    kd_tile_comp *comps; // Array of tile-comps, one for each component
    int num_layers; // Number of quality layers in the tile.
    int header_bytes; // Size of the tile header
    int dispatched_header_bytes; // Tile header bytes transferred already.
    int sim_header_bytes; // See `kd_window_context::simulate_increments'
    kdu_byte dispatched_header_complete;// If data-bin complete in client cache
    kdu_byte sim_header_complete; // See `simulate_increments'
    kdu_byte is_modeled; // See below
    kdu_byte uses_ycc; // Non-zero if tile uses a YCC component transform
    int total_precincts; // Total precincts in this tile
    int completed_precincts; // Number of precincts completely transferred
    int num_active_precincts; // Number of active precincts for this tile
    int num_tile_binrefs; // Number of non-precinct `kd_active_binref's
    kd_tile *next_attached; // Used to build a doubly-linked list of all
    kd_tile *prev_attached; // tiles currently with an open `source' interface
    kd_tile *next_modeled; // Used to link all tiles with `is_modeled' != 0
    kd_resolution *modeled_resolutions; // See below
  };
  /* Notes:
          This object serves three purposes: 1) it maintains basic structural
       information about the tile which is required for sequencing data-bin
       contributions; 2) it maintains potential interfaces to the original
       source of tile data (`source'), the interchange codestream which is
       used to prepare data, and the structure codestream which is used to
       convert window regions into associated data-bins -- any or all of
       these interfaces may be empty; and 3) it maintains cache
       model information for the tile -- this is done through the `comps'
       array and its own subordinate arrays.
          The `num_active_precincts' and `num_tile_binrefs' members maintain
       counts of the number of dynamic references which are currently held
       to this object.  The former counts the total number of
       `kd_active_precinct' objects which are modeling precincts associated
       with this tile, while the latter counts the number of `kd_active_binref'
       objects whose `tile' member points to this tile and which do not
       reference precincts.  If either of these counts is non-zero, the
       `interchange' interface should not be closed, because it is imminently
       needed or else managing potentially open `kdu_precinct' interfaces.
       On the other hand, when both these counters do go to zero
       (detected inside `kd_stream::release_active_binrefs'), the tile's
       `close' function will be called automatically, to clean up and
       recycle resources as soon as possible.
          The `is_modeled' and `next_modeled' members are used to build a
       list of all tiles in the containing `stream' which have potentially
       had cache model information associated with them.  This is of interest
       only for the stateless request processing mode, in which the cache
       model must be erased between requests.  In practice, we mark tiles as
       modeled when they are first accessed by
       `kd_window_context::sequence_active_bins' and we mark them as unmodeled
       inside `kd_stream::erase_cache_model'.  If `is_modeled' is non-zero,
       the `modeled_resolutions' member points to a list of all
       member `kd_resolution' objects for which any precinct cache models
       have been created.  The `kd_stream::erase_cache_model' erases all of
       these as well.
  */

/*****************************************************************************/
/*                                 kd_tile_comp                              */
/*****************************************************************************/

struct kd_tile_comp {
  public: // Functions
    kd_tile_comp() { res = NULL; }
  public: // Data
    int c_idx; // Component index in source tile.
    kd_tile *tile;
    int num_resolutions; // Total number of resolution levels available
    kd_resolution *res; // Array of resolutions, starting from lowest (LL)
  };
  /* Notes:
        See `kd_tile' for an explanation of how this object is used.
  */
/*****************************************************************************/
/*                                 kd_precinct                               */
/*****************************************************************************/

struct kd_precinct { // Server state: 8-bytes per precinct, if 32-bit ptrs
    kd_precinct() { cached_bytes = cached_packets = 0; active = NULL; }
    kdu_uint16 cached_bytes; // Bytes already cached by client.
    kdu_uint16 cached_packets; // Packets already available at client.
    kd_active_precinct *active;
  };
  /* Notes:
        This structure is the fundamental element in the cache model managed
     by the `kdu_serve' object.  It maintains information about the
     number of bytes of packet data which have already been transferred
     using `kd_context_window::generate_increments', as well as the number of
     packets which these bytes represent.
        Although undesirable, it can happen that the server application
     transfers only a fractional number of packets.  In this case, the
     `cached_packets' member holds the number of complete packets which have
     been transferred, while the `cached_bytes' member indicates
     the actual total number of bytes which the client is believed to have
     cached (or to be about to cache), including partial packets.  Partially
     cached packets may arise when abandoned messages are returned to the
     `kdu_serve::release_chunks' function, or when a non-trivial
     `max_data_bytes' argument is supplied to
     `kd_context_window::generate_increments'.
        Since we are modeling the state of the client cache here, we need to
     bear in mind that there are three quantities of interest: 1) the number
     of bytes in the client's cache; 2) the number of whole packets in the
     client's cache; and 3) a binary flag indicating whether the client's
     cache is aware that the precinct's representation is complete.  Under
     some circumstances, it may happen that the client has received all packets
     in the precinct and its cache is aware of this, but the client may not
     yet know that its representation for the precinct is complete.  In this
     case, we still need to send a message to the client to inform it that
     its cache representation for the precinct is complete.
        In view of the above, we can now provide a more concrete description of
     the `cached_packets' and `cached_bytes' members.  If the client is not
     known to have anything in its cache for the precinct, both values are
     zero.  Otherwise, if one of `cached_packets' or `cached_bytes' is
     known, while the other is unknown, the unknown value should be set to
     0xFFFF.  Finally, if the precinct data-bin is know to be fully cached
     at the client, `cached_packets' will hold the same value as the
     corresponding tile's `kd_tile::num_layers' member, while `cached_bytes'
     can hold any value other than 0xFFFF.
        The `active' member holds NULL unless this precinct is currently
     active, in which case it points to a `kd_active_precinct' structure
     which maintains the state of the active precinct. */

/*****************************************************************************/
/*                              kd_precinct_block                            */
/*****************************************************************************/

struct kd_precinct_block {
    union {
      kd_precinct_block *free_next; // Used to build the free list
      kd_precinct_block **active_next; // Used to build the active list
    };
    kd_precinct precincts[1]; // Actually a larger array, allocated as required
  };

/*****************************************************************************/
/*                                kd_resolution                              */
/*****************************************************************************/

struct kd_resolution {
  public: // Functions
    kd_resolution()
      { pblocks=active_pblocks=NULL; is_modeled=false; next_modeled=NULL; }
    bool access_pblock(kdu_coords p_idx, kdu_dims &pblk_dims,
                       bool only_if_top_of_block, bool only_if_left_of_block,
                       bool only_if_new)
      { /* This function is used to assist in applying cache model
           instructions to whole blocks of precincts at once, working together
           with `process_model_instructions'.  The function looks for the
           precinct block which contains the precinct whose absolute index is
           given by `p_idx'.  If one does not exist, it is created.  The
           size of the precinct block and the location of its top-left corner
           (expressed in multiples of precincts), relative to the first
           precinct in the resolution, are returned via `pblk_dims'.  If
           `only_if_top_of_block' is true, the function returns false without
           doing anything if `p_idx' does not identify a precinct on the top
           edge of the block.  Similarly, if `only_if_left_of_block' is true,
           `p_idx' must correspond to a precinct on the left edge of its
           block.  If `only_if_new' is true, the function returns false
           without doing anything, unless the precinct block needs to be
           created for the first time by this call. */
        p_idx -= precinct_indices.pos; // Convert to a relative index
        if (only_if_top_of_block && ((p_idx.y & (pblock_size-1)) != 0))
          return false;
        if (only_if_left_of_block && ((p_idx.x & (pblock_size-1)) != 0))
          return false;
        int b_x = p_idx.x >> log_pblock_size;
        int b_y = p_idx.y >> log_pblock_size;
        kd_precinct_block **pb = pblocks + b_x + b_y*num_pblocks.x;
        if (*pb == NULL)
          { 
            (*pb = get_new_pblock())->active_next = active_pblocks;
            active_pblocks = pb;
          }
        else if (only_if_new)
          return false;
        pblk_dims.pos.x = b_x<<log_pblock_size;
        pblk_dims.pos.y = b_y<<log_pblock_size;
        pblk_dims.size = precinct_indices.size-pblk_dims.pos;
        if (pblk_dims.size.x > pblock_size) pblk_dims.size.x = pblock_size;
        if (pblk_dims.size.y > pblock_size) pblk_dims.size.y = pblock_size;
        return true;
      }
    kd_precinct *access_precinct(kdu_coords p_idx, bool create_if_missing=true)
      { /* Return pointer to cache entry for the indicated precinct (absolute
           index).  If `create_if_missing' is true, the function will create
           the containing precinct block, if it does not already exist.
           Otherwise, the function will return NULL in this case. */
        p_idx -= precinct_indices.pos;
        int b_x = p_idx.x >> log_pblock_size;
        int b_y = p_idx.y >> log_pblock_size;
        kd_precinct_block **pb = pblocks + b_x + b_y*num_pblocks.x;
        if (*pb == NULL)
          { 
            if (!create_if_missing) return NULL;
            (*pb = get_new_pblock())->active_next = active_pblocks;
            active_pblocks = pb;
          }
        p_idx.x-=b_x<<log_pblock_size; p_idx.y-=b_y<<log_pblock_size;
        return (*pb)->precincts + p_idx.x + (p_idx.y<<log_pblock_size);
      }
    kd_precinct_block *get_new_pblock();
      /* Fetches a new precinct block, of the relevant size, from the
         `kd_serve' object's common store. */
    void free_active_pblocks();
      /* Recycles all active precinct blocks back to the `kd_serve' object's
         common store. */
    void process_pblock_model_instructions(kdu_dims region, int i_buf[]);
      /* This function applies the precinct data-bin cache model instructions
         found in `i_buf' to the collection of precinct data-bins whose
         indices are represented by `region'.  In practice, the entire region
         corresponds to at most one precinct block, although the `region.size'
         member may indicate dimensions smaller than `pblock_size' because
         the precinct block goes over the boundaries of the resolution's
         canvas.  The `i_buf' array contains 2 entries for each location in
         `region'.  The interpretation of these 2 entries is given with the
         description of `kdu_window_model::get_precinct_block'.  The precinct
         block must have been instantiated already, typically by a call to
         `access_pblock'. */
    void process_model_instructions(kd_precinct *prec, int i_buf[], int i_cnt);
      /* This function is the workhorse for `process_pblock_model_instructions'
         but can also be called directly to process cache model manipulation
         instructions for a single precinct. `i_cnt' identifies the maximum
         number of instructions in `i_buf' which need to be considered.
         There are at most 2 instructions, the first of which provides a
         lower bound for the precinct data-bin (bin complete if the value is
         -ve), while the second provides an exclusive upper bound for the
         precinct data-bin (unless 0).  The non-negative values in this
         array are 2*L if L is a byte limit and 2*L+1 if L is a quality layer
         limit. */
  public: // Data
    int r_idx; // Resolution index in containing tile-component.
    kd_tile_comp *tile_comp;
    kdu_dims precinct_indices; // Range of indices for accessing precincts
    kdu_long pid_base; // Absolute bin ID of top-left precinct in resolution
    int tl_samples; // Total code-block samples in top-left precinct
    int top_samples; // Total code-block samples in top-centre precinct
    int tr_samples; // Total code-block samples in top-right precinct
    int left_samples; // Total code-block samples in left-centre precinct
    int centre_samples; // Total code-block samples in centre precinct
    int right_samples; // Total code-block samples in right-centre precinct
    int bl_samples; // Total code-block samples in bottom-left precinct
    int bottom_samples; // Total code-block samples in bottom-centre precinct
    int br_samples; // Total code-block samples in bottom-right precinct
    int pblock_size; // Precinct cache divided into blocks (SxS precincts each)
    int log_pblock_size; // log_2(S), where S=`pblock_size'
    kdu_coords num_pblocks; // Dimensions of the array of precinct cache blocks
    kd_precinct_block **pblocks; // One entry for each precinct cache block.
    kd_precinct_block **active_pblocks; // See below
    bool is_modeled; // See below
    kd_resolution *next_modeled; // See below
  };
  /* Notes:
          The fundamental cache element is that of a precinct.  However, there
       can be an enormous number of precincts in an image (well over a
       million) if it is truly huge.  Moreover, the majority of these might
       all be in a single resolution of a single tile-component.  Since
       relatively few of these will ever be touched by a client, memory
       can be conserved by providing a block-based structure for accessing
       the precinct cache elements.  This structure is described and
       managed by the last few member variables of the present object, starting
       from the `pblock_size' member.
          The `pblock_size' member indicates the nominal width and the height
       of each precinct cache block.  Thus, each cache block manages an
       array of `pblock_size' x `pblock_size' precincts.
          The `log_pblock_size' member holds log_2(`pblock_size'), where
       `pblock_size' is guaranteed to be an exact power of 2.
          `num_pblocks' indicates the number of precinct cache blocks in
       each direction.
          `pblocks' holds one pointer for each cache block.  The pointers are
       organized in raster fashion, with `num_pblocks.x' pointers on each
       row.  Each element of the array holds either NULL or a pointer to
       a `kd_precinct_block' object, whose entries are themselves organized
       in raster scan fashion, measuring `num_pblocks' by `num_pblocks'.
          `active_pblocks' manages a list of precinct blocks which are
       currently referenced from within the `pblocks' array.  If non-NULL,
       this member points to the location within `pblocks' at which the
       reference to the first active precinct block on the list may be found.
       The list is connected indirectly via `kd_precinct_block::active_next',
       which is NULL at the end of the list, or else points to the location
       in `pblocks' which references the next active precinct block.  This
       indirect list referencing structure allows the `pblock' references to
       be reset to NULL as the list is being cleaned up.
          `next_modeled' is used to build a list of all `kd_resolution'
       objects within a `kd_tile' for which `is_modeled' is true.  These
       are the objects which can have a non-NULL `active_pblocks' member.
  */

/*****************************************************************************/
/*                            kd_active_precinct                             */
/*****************************************************************************/

struct kd_active_precinct {
    kdu_precinct interchange;
    kd_precinct *cache;
    kdu_coords p_idx; // Absolute coordinates of precinct in its resolution
    kd_resolution *res; // Resolution level to which precinct belongs
    kdu_byte current_complete; // Non-zero if data-bin known to be complete
    kdu_byte sim_complete; // See below
    kdu_uint16 current_packets, current_bytes, current_packet_bytes;
    kdu_uint16 sim_packets, sim_bytes, sim_packet_bytes;
    kdu_uint16 next_layer;
    kdu_uint16 max_packets; // Num layers in the tile to which precinct belongs
    kdu_uint16 num_samples; // Area of the precinct
    int num_active_binrefs; // Num `kd_active_binref's referencing this object
    int max_id_length; // See below
    int hdr_threshold; // See below
    kd_active_precinct *next;
    kd_active_precinct *prev;
  };
  /* Notes:
        This structure plays a central role in managing the active state of
     the server.  Whereas a `kd_precinct' structure might be maintained
     for every precinct in the image (although we would hope to have a lot
     of empty precinct blocks if the image is very large) the current
     structure is used only for precincts which are considered "active".
     Active precincts are those which are currently available for processing
     by `kd_context_window::generate_increments'.
        In general, each window context contributes its own set of active
     precincts, some of which may be in common with other window contexts.
     Given this commonality, a single doubly-linked list of all active
     precincts is maintained by the `kd_serve' object, and pointers into
     this list are maintained by individual lists of `kd_binref' objects
     which are private to each window context.  The `num_active_binrefs' member
     keeps track of the number of window contexts from which a single
     `kd_active_precinct' object is referenced.  When this value drops to
     0, the entry is removed from the shared doubly-linked list of active
     precincts.
        The `current_bytes' and `current_packets' members record the
     total number of bytes and the number of complete packets which would be
     cached by the client if the chunks generated by
     `kd_context_window::generate_increments' were completely transferred to
     the client.  The value of `current_bytes' may exceed the number of bytes
     associated with `current_packets', if a packet has been partially
     transferred.  The `current_packet_bytes' member records (if known) the
     number of bytes associated with `current_packets' whole packets.  This
     value is often identical to `current_bytes' but may be less.
        The `sim_bytes', `sim_packets' and `sim_packet_bytes' members play
     the same role as `current_bytes', `current_packets' and
     `current_packet_bytes', except that they are calculated by
     `kd_context_window::simulate_increments' and later moved into their
     `current...' associates when the data-bin increments are actually
     dispatched.
        The `current_complete' and `sim_complete' members are used
     to manage the fact that a precinct for which all bytes have been sent
     to the client is not necessarily known by the client to be completed.
     A precinct is considered to be "complete" if we have sent all
     quality layers to the client ourselves, or we have received additive
     cache model statements from the client which explicitly indicate that
     the precinct data-bin is complete.  The `sim_complete' member
     is non-zero if `kd_window_context::simulate_increments' has determined
     that sufficient information should be dispatched such that the entire
     precinct data-bin will have been sent.  Even if no actual bytes of the
     data-bin remain to be sent, if `sim_complete' is non-zero, but
     `current_complete' is zero, we need to include a message for this
     precinct in the generated data chunks. These members will only take
     values of 0 and 1.
        If `interchange' is an empty interface or either of
     `current_packets' or `current_bytes' is equal to 0xFFFF, the above
     members are not generally valid.  This happens if content has only
     recently been transferred from the `cache' object, or if cache
     model instructions have left the object in a not completely-known
     state.  In such events, `kd_window_context::load_active_precinct'
     must be called at the point when the information is actually required
     (always happens inside `kd_window_context::simulate_increments').  That
     function sets all the `sim_...' variables equal to their `current_...'
     namesakes.
        The `next_layer' member holds the index of the next quality layer
     (equivalently, the index of the next packet) to be considered for
     inclusion in a collection of data being prepared by `generate_increments'.
     `next_layer' will commonly be equal to `current_packets', but may be
     larger, if `kd_window_context::simulate_increments' has encountered one
     or more packets too small for inclusion by themselves.
        The `max_id_length' member is used to remember the calculated maximum
     length of the data-bin ID field in any message header used to communicate
     data for this precinct.  It only needs to be calculated when an
     active precinct is first considered for inclusion by
     `kd_window_context::generate_increments'.
        The `hdr_threshold' member is used in simulating the maximum number
     of message header bytes associated with data for this precinct.  The
     value records the maximum value that `current_bytes' can take on before
     a new message header might have to be generated.  It is initialized to
     -1 inside `kd_window_context::simulate_packet_generation' before each
     simulation attempt.  The purpose of this member is to limit the frequency
     with which message header length simulation must be performed.
  */

/*****************************************************************************/
/*                            kd_window_context                              */
/*****************************************************************************/

struct kd_window_context {
  public: // Member functions
    kd_window_context(kd_serve *owner, int context_id);
    ~kd_window_context();
      /* This function will clean up all the intermediate mess that may be
         left behind in the event that an exception is thrown deep down in
         the machinery.  As a result, you can catch exceptions at quite
         a high level. */
    void process_window_changes();
      /* Called from within `kdu_serve::generate_increments' if the
         `window_changed' member is true.  If `window_imagery_changed' is
         true, the function modifies the window and associated parameters to
         make it legal, installs a new set of codestream windows in the
         `new_stream_windows' list and then sets `window_changed' to
         false; the transition to the `new_stream_windows' is not actually
         completed until the next call to `sequence_active_bins', which
         detects the `update_window_streams' flag left behind by this
         function.
            If `window_imagery_changed' is false, the new window of interest
         differs from the existing one only in regard to the metadata which
         is of interest.  In this case, the function resets `window_changed'
         and sets the `update_window_metadata' flag.  The active metadata-bins
         of interest will be be recomputed on the next call to
         `sequence_active_bins', which detects the `update_window_metadata'
         flag.  However, no new codestream windows need be created in this
         case and the `update_window_streams' flag is left false.  This
         case should not occur in stateless mode, or if there are cache
         model instructions or changes in the request processing preferences,
         since all of these things generally require complete resequencing
         of both imagery and metadata bins.
            In either case, the function returns with `window_changed' and
         `window_imagery_changed' both false and with exactly one of
         `update_window_streams' or `update_window_metadata' true. */
    void sequence_active_bins();
      /* This is the function which manages the set of active data-bins for
         which increments may be generated at any given time.  Increments
         can be generated for the active data-bins in quality progressive
         fashion, even respecting relevance fractions.  However, the larger
         the set of active data-bins, the more memory resources are generally
         consumed.  Moreover, hard limits generally exist on the number of
         codestreams which can be simultaneously active.  For these reasons,
         this function may need to be called multiple times.
         [//]
         If `update_window_streams' is true on entry, the function is
         being called for the first time after a change in the requested
         window of interest for this context.  In this case, the function
         initializes the sweeping machinery, taking special care to preserve
         common elements between the current and previous request windows.
         [//]
         If `update_window_metadata' is true on entry, the function is
         being called for the first time after a change in the metadata
         aspects of the requested window of interest, but without any change
         in the imagery aspects.  In this case, the function leaves the
         sweep machinery and all active codestream data-bins intact, but
         regenerates the active metadata-bins.
         [//]
         Otherwise, the function is being called to advance the set of
         active data-bins.  In this process, the function cycles through the
         set of all codestream windows progressively.  Each time it passes
         the first codestream window, the function decreases the number of
         extra discard levels (if non-zero) or else increases the
         `active_bin_rate_threshold'.
         [//]
         If `sweep_next' and `meta_sweep_next' are both NULL, all data-bins
         associated with the current window request which are not yet fully
         dispatched are all on the `active_bins' list.  This condition may
         become true after calling this function.  If the condition is true
         on entry, the function removes all active streams and active data-bins
         but there is still the possibility that abandoned data-chunks will
         cause one or both of these members to become non-NULL so that
         future sequencing calls can add active data-bins.
      */
    int simulate_increments(int suggested_total_bytes,
                            int max_total_bytes, bool align,
                            bool using_extended_message_headers);
      /* This function is called from within `generate_increments' to
         simulate the generation of data-bin increments for data-bins
         referenced from the `active_bins' list.  The function advances
         `active_bin_ptr' and `active_threshold', recording the results of
         the simulation process in `kd_meta::sim_bytes' and
         `kd_meta::sim_complete' (for metadata binrefs),
         `kd_stream::sim_header_bytes' and `kd_stream::sim_header_complete'
         (for codestream header binrefs), `kd_tile::sim_header_bytes'
         and `kd_tile::sim_header_complete' (for tile header binrefs) and
         `kd_active_precinct::sim_bytes', `kd_active_precinct::sim_packets',
         `kd_active_precinct::sim_packet_bytes' and
         `kd_active_precinct::sim_complete' (for precinct binrefs).
           The `suggested_total_bytes' and `max_total_bytes' limits provide
         loose and tight bounds, respectively, on the total number of message
         bytes (including headers) that should be associated with the
         simulated increments.  In practice, this function can only use a
         conservative upper bound on the number of header bytes.  A bound on
         the length of a single message header is obtained using the
         `serve' object's `id_encoder', together with the
         `using_extended_message_headers' argument.  The function recognizes
         that multiple messages may be required, but it
         assumes that at least `max_chunk_body_bytes'/4 message body bytes
         will be transferred in the first message of any byte range which is
         split across multiple chunks. The caller must be careful to follow
         this principle so as to avoid accidentally violating the
         `max_total_bytes' limit when data is delivered.
           The function returns the simulated number of message bytes.  In the
         event that simulation stopped as a result of reaching the hard limit
         associated with the `max_total_bytes' argument, the function returns
         a value exactly equal to `max_total_bytes', so that the caller
         knows what happened. */
    kds_chunk *generate_increments(int suggested_data_bytes,
                                   int &max_data_bytes, bool align,
                                   bool use_extended_message_headers,
                                   bool decouple_chunks);
      /* Does most of the work of `kdu_serve::generate_increments'. */
    void process_outstanding_model_instructions(kd_stream *stream);
      /* This function processes any outstanding atomic cache model
         instructions which relate to the indicated `kd_stream' object.
         If `stream' is NULL, all outstanding atomic cache model
         instructions are processed.  If `current_model_instructions' is
         NULL, there are no relevant instructions to process, so nothing
         is done.  This function is called under various circumstances, which
         require cache model alignment.  Non-NULL values for `stream' are
         provided to allow for the processing of outstanding model manipulation
         instructions as soon as we know that we will not be accessing a
         given codestream again -- we do not currently use this option anywhere
         but it could be useful for reducing the amount of storage associated
         with `kdu_window_model' when codestreams are processed sequentially
         (probably relevant only for video). */
  //---------------------------------------------------------------------------
  private: // Utility functions
    bool add_active_stream_ref(kd_stream *stream);
      /* This function is called when a `kd_codestream_window' object needs
         active access to its `kd_stream' object.  The function first checks
         that this is possible, given the `KD_MAX_ACTIVE_CODESTREAMS' quota
         and the state of the `codestream_seq_pref' flag (if sequential
         codestreams are preferred, only one active codestream will be allowed
         at a time).  If allowed, the function updates `num_active_streams',
         `active_streams' and `num_active_stream_refs' accordingly,
         returning true.  Otherwise, the function returns false, meaning that
         the quota would be exceeded by making the stream active.  Note
         that the function automatically takes care of notifying the
         `kd_serve' object about newly active streams, so that their internal
         reference counters can be updated and they can be moved onto the
         `kd_serve' object's cross-context active list if required. */
    void remove_active_stream_ref(kd_stream *stream);
      /* This function is called when a `kd_codestream_window' object no
         longer needs active access to its `kd_stream' object.  The function
         reduces the relevant reference count in `num_active_stream_refs'
         and, if zero, informs the `kd_serve' object that the window context
         no longer requires active access to the codestream.
            This function may be called with `stream'=NULL, in which case
         all active `kd_stream' objects are released back to the `kd_serve'
         object. */
    int add_stream(int stream_idx);
      /* This function is used when inserting new window codestreams within
         `process_window_changes'.  It scans the `stream_indices' array to
         see if a codestream with the same index already exists.  If not,
         a new entry is inserted -- they are ordered in increasing (or
         decreasing) order, depending on whether or not `window_prefs'
         indicates reverse delivery of codestreams.  If the number of window
         streams would exceed `KD_MAX_WINDOW_CODESTREAMS', the function does
         nothing, returning -1.  Otherwise, the location of `stream_idx'
         within the `stream_indices' array is returned.  This function is
         used together with `insert_new_stream_window'. */
    void insert_new_stream_window(kd_codestream_window *win,
                                  int stream_entry);
      /* Inserts `win' into the list headed by `new_stream_windows'.
         The `stream_entry' argument is the return value from a
         previous call to `add_stream'.  It identifies the location of the
         corresponding codestream index within the `stream_indices' array.
         The function also updates the `first_stream_windows' and
         `last_stream_windows' arrays at location `stream_entry' and ensures
         that the stream windows are linked together in order of increasing
         (or decreasing) codestream index, depending on whether or not
         `window_prefs' indicates reverse delivery of codestreams. */
    void restart_increment_generation()
      { /* Called if something changes which may require the active data-bins
           to be scanned again from scratch for subsequent generation of
           increments. */
        active_bin_ptr = active_bins;  scan_first_layer = true;
        scanned_precinct_bytes = scanned_precinct_samples = 0;
        scanned_incomplete_metabins = scanned_incomplete_codestream_bins = 0;
        next_active_threshold = INT_MIN;
        active_threshold = INT_MAX; // Actually ingored with `scan_first_layer'
        active_bins_completed = active_metabins_completed = false;
        active_bin_rate_reached = false;
      }
    void install_active_layer_log_slopes();
    void lock_active_streams();
    void unlock_active_streams();
    void load_active_precinct(kd_active_precinct *active,
                              kd_tile_comp * &tc, kdu_tile_comp &tci,
                              kdu_tile_comp &source_tci,
                              kd_resolution * &rp, kdu_resolution &rpi,
                              kdu_resolution &source_rpi);
      /* This function is called from within `simulate_increments' if an
         active precinct's `interchange' interface is found to be empty, or
         if either of the `active->current_packets' or `active->current_bytes'
         members is found to be 0xFFFF at the time when the relevant values
         are needed, or if `active->current_packet_bytes' is found to be zero
         while `active->current_packets' is non-zero.  These are all conditions
         which may occur due to partial availability of information during
         the processing of cache model manipulation instructions, or because
         cache model information was collapsed to the minimal representation
         embodied by `kd_precinct'.  If required, the function opens the
         relevant interface and loads it up with content from the `source'
         codestream.  The function then figures out values for all of the
         cache model related state variables, based on the values of
         `current_packets' and `current_bytes', at most one of which should be
         0xFFFF.  These activities may require the locking of active
         codestreams, which should be automatically unlocked before
         `simulate_increments' returns.  The last 6 arguments hold state
         variables which keep track of the resolution and tile-component
         (along with appropriate codestream interfaces) which were last used
         by this function, so that new ones need not be opened unnecessarily.
            This function will never be called if the precinct data-bin was
         known to be complete in the client cache, so it is safe here to
         reset `current_complete' to 0.  The function also sets all
         `sim_...' variables equal to their `current_...' counterparts
         before returning. */
    int estimate_msg_hdr_cost(int &hdr_threshold, int range_start,
                              int range_lim, int extended_num_packets,
                              int max_id_length);
      /* This function is used by `simulate_increments' to estimate
         the number of new message header bytes which may be required if the
         byte range from `range_start' to `range_lim' within a data-bin
         must be included in outgoing data chunks.  If `hdr_threshold' is -ve,
         the function calculates the header cost directly and sets
         `hdr_threshold' equal to the maximum value of `range_lim' for which
         the estimated header cost is valid.  If `hdr_threshold' is
         non-negative on entry, the function calculates the number of new
         message header bytes required and updates `hdr_threshold'.
         The `extended_num_packets' argument is 0 if extended message headers
         are not required; otherwise, it is the number of packets which must
         be signalled via an extended precinct data-bin message header.
           The algorithm implemented here assumes that the threshold for
         possibly splitting a byte range into multiple messages is
         B/4 message body bytes, where B equals `max_chunk_body_bytes'-- as
         explained in connection with the `simulate_increments' function.
         Thus, the first `hdr_threshold' value will be `range_start' + B/4.
         Each successive `hdr_threshold' value is B-L bytes greater, where L
         is the calculated worst case message header size, allowing for
         chunk boundaries which fall anywhere from the previous
         `hdr_threshold' to the new `hdr_threshold'-1. The splitting threshold
         applies to range bytes, not range bytes + header bytes.  This
         splitting policy must be followed `generate_xxx_increments'
         functions.
       */
    int generate_meta_increment(kd_meta *meta, kds_chunk * &chunk_tail,
                                bool decouple_chunks);
    int generate_header_increment(kd_stream *stream, kd_tile *tp,
                                  kds_chunk * &chunk_tail,
                                  bool decouple_chunks);
    int generate_precinct_increment(kd_active_precinct *precinct,
                                    kd_stream *stream, kd_tile *tp,
                                    kds_chunk * &chunk_tail,
                                    bool decouple_chunks,
                                    bool use_extended_headers);
      /* The above three functions generate actual incremental contributions
         from data-bins, pushing their results to the list of data chunks whose
         tail is found at `chunk_tail' and extending the list as required.
         These functions all move the relevant `sim_...' member variables into
         the corresponding `current_...' or `dispatched_...' member variables
         to reflect the fact that simulated increments have been actually
         dispatched.  The functions do not update any tile, codestream or
         global completion counters -- this is something that is left to the
         caller to preserve clarity in the code.  The functions all return
         the number of actual chunk data bytes generated.  Note that all of
         these functions must follow the message range splitting policy
         outlined in connection with `estimate_msg_hdr_cost'. */
  //---------------------------------------------------------------------------
  public: // Who am I
    int context_id;
    kd_serve *serve;
    kd_window_context *next;
  public: // Codestream window management
    kd_codestream_window *stream_windows;
    kd_codestream_window *new_stream_windows;
    kd_codestream_window *old_stream_windows; // See below
    kd_codestream_window *first_active_stream_window;
    kd_codestream_window *last_active_stream_window;
    bool active_streams_locked;
    int num_active_streams;
    kd_stream *active_streams[KD_MAX_ACTIVE_CODESTREAMS];
    int num_active_stream_refs[KD_MAX_ACTIVE_CODESTREAMS];
    int num_streams; // Number of distinct codestreams belonging to the window
    int stream_indices[KD_MAX_WINDOW_CODESTREAMS]; // In increasing order
    kd_codestream_window *first_stream_windows[KD_MAX_WINDOW_CODESTREAMS];
    kd_codestream_window *last_stream_windows[KD_MAX_WINDOW_CODESTREAMS];
  public: // Information about the requested window of interest & preferences
    kdu_window_prefs window_prefs; // Consulted when considering a new WOI
    kdu_window current_window; // Returned by `kdu_serve::get_window'.
    kdu_window last_translated_window; // See below
    bool window_changed; // If `kdu_serve::set_window' changed `current_window'
    bool window_imagery_changed; // See `process_window_changes'
    bool update_window_streams; // See `process_window_changes'
    bool update_window_metadata; // See `process_window_changes'
    kdu_window_model model_instructions;// Accumulates cache model instructions
    kdu_window_model *current_model_instructions; // NULL if no instructions
    bool codestream_seq_pref; // If we want codestreams delivered sequentially
  public: // Information used to manage the collection of active data-bin refs
    kd_active_binref *active_bins; // Starts with metadata binrefs
    kd_active_binref *last_active_metabin; // For separating codestream binrefs
    kd_active_binref *old_active_bins; // See below
    kd_active_binref *new_active_bins; // See below
    int sweep_extra_discard_levels; // Decrements during each sweep until 0
    kd_codestream_window *sweep_start;
    kd_codestream_window *sweep_next; // Next stream to use in sequencing
    kd_codestream_window *meta_sweep_next; // See below
  public: // Information used to manage increment generation
    kd_active_binref *active_bin_ptr; // Next bin for generating increments
    int active_threshold; // See below
    int next_active_threshold; // See below
    bool scan_first_layer; // See below
    kdu_long scanned_precinct_bytes; // See below
    kdu_long scanned_precinct_samples; // See below
    int scanned_incomplete_metabins; // See below
    int scanned_incomplete_codestream_bins; // See below
    int num_dummy_layer_slopes;
    int *dummy_layer_log_slopes; // For streams with no slope info
  public: // State information used to establish completion
    bool active_bins_completed; // All required data sent from `active_bins'
    bool active_metabins_completed;// Required metadata sent from `active_bins'
    bool active_bin_rate_reached; // Bit-rate for active_bins reached threshold
    double active_bin_rate_threshold; // Bit-rate threshold (-ve if unlimited)
    kds_chunk *extra_data_head; // Holds the outstanding extra data pushed in
    kds_chunk *extra_data_tail; // by `kdu_serve::push_extra_data'
    int extra_data_bytes; // Number of data bytes in the above list.
  private: // Private data
    kdu_range_set context_set; // Temporary storage for requested contexts
    kdu_range_set stream_set; // Temporary storage for requested codestreams
    kd_chunk_output out;
  };
  /* Notes:
        A window context corresponds to a single JPIP request channel.  Each
     window context manages its own window of interest, for which data is
     generated in epochs.  The window of interest is expressed, in turn, via
     `kd_codestream_window' objects, each of which represents a single
     rectangular window into a particular set of image components
     of a single codestream at a particular resolution.  We refer to these as
     "codestream windows" (or "stream windows", for short).
        The term "active" refers to the collection of codestream elements
     which are actively available to the `generate_increments' function.
        Each window context has two limits which are used to restrict the
     amount of codestream resources it can consume at any given time.  The
     first limit is the number of codestreams that we are prepared
     to consider in a given window request from the client.  This limit is
     given by `KD_MAX_WINDOW_CODESTREAMS'.  The second limit is the number
     of codestreams which can be simultaneously active -- the active
     codestreams must be simultaneously attached and locked during calls
     to the `generate_increments' function, so they can be costly to keep
     around; this limit is given by `KD_MAX_ACTIVE_CODESTREAMS'.
        The `stream_windows' member points to a list of all codestream windows
     that are associated with the window of interest that the context is
     currently managing.  There are guaranteed to be at most
     `KD_MAX_WINDOW_CODESTREAMS' distinct `kd_stream' objects referenced
     from this list.  The `new_stream_windows' and `old_stream_windows'
     members are used to manage the transition between an old window of
     interest and a new one.  During this transition, the complete set of
     codestream windows which represents the new window of interest is first
     collected in the `new_stream_windows' list.  The next call to
     `sequence_active_bins' then moves these new stream windows into the
     current stream windows list, releasing the old ones.  As an
     intermediate step, `sequence_active_bins' temporarily stores the list of
     old stream windows using the `old_stream_windows' member, so that it can
     be released properly in the event that an exception occurs during the
     call to `sequence_active_bins'.
        `first_active_stream_window' points to the first codestream window
     within the `stream_windows' list which is currently active, meaning that
     it is contributing active data-bins for the generation of increments.
     Similarly, `last_active_stream_window' points to the last active stream
     window.  Scanning the list from `first_active_stream_window' until
     `last_active_stream_window' (with wrap-around at the end of the list)
     will visit all active stream windows.  Out of these, however, there
     may be some which are not actually contributing any active data-bins
     and so do not contribute to the `KD_MAX_ACTIVE_CODESTREAMS' quota on
     active `kd_stream' objects.
        The above-mentioned quota on active `kd_stream' objects is managed by
     the `num_active_streams', `active_streams' and `num_active_stream_refs'
     members, via the `remove_active_stream_ref' and `add_active_stream_ref'
     functions.  Specifically, the `active_streams' array keeps track of
     `num_active_streams' pointers to `kd_stream' objects which are
     actively in use by this window context, while the `num_active_stream_refs'
     array keeps track of the number of `kd_codestream_window' objects which
     are using these active streams.  Each `kd_codestream_window' object
     which is contributing active data-bin references contributes to the
     count associated with the relevant active `kd_stream' entry here.
        The `stream_indices', `first_stream_windows' and `last_stream_windows'
     arrays are managed by the `process_window_changes' function -- via calls
     to `add_stream' and `insert_new_stream_window'.  New window requests are
     converted into a sequence of codestream windows, which are initially
     stored at `new_stream_windows' (until the call to `sequence_active_bins').
     To ensure that there are at most `KD_MAX_WINDOW_CODESTREAMS' distinct
     codestreams associated with these stream windows, and to keep the stream
     windows organized in increasing order of their associated codestream
     indices, we store the distinct codestream indices used within the
     `stream_indices' array, while using the `first_stream_windows' and
     `last_stream_windows' arrays to store pointers to the first and last
     codestream windows with each codestream index, as they appear within the
     `stream_windows' list.  The number of used entries in these arrays is
     given by `num_streams', which grows each time a distinct codestream
     is encountered.  Note that if `new_stream_windows' is non-NULL, the
     `stream_indices', `first_stream_windows' and `last_stream_windows'
     members actually refer to the new collection of codesteam windows,
     rather than the existing one managed by the `stream_windows' list -- this
     is a transient condition created by the processing of window of interest
     changes without any intervening generation of increments.
        The `sweep_start' and `sweep_next' members maintain the state of
     the sequencing machine.  Specifically, `sweep_next' points to the next
     `kd_codestream_window' from which active data-bins can
     potentially be sequenced by calling its `sequence_active_bins' function.
     The sweep machinery starts at `sweep_start', which is usually the same
     as `stream_windows', but may be different if the window-of-interest
     was changed to one which overlapped with a previous one and we found it
     beneficial to start the sweep machinery for the new window of interest
     at the place we were previously up to.
        The `meta_sweep_next' member plays a similar role to `sweep_next',
     except that it is used for sequencing active metadata-bin references.
     In the current implementation, this member is either equal to
     `stream_windows' or else NULL, since any call to `sequence_active_bins'
     which finds `meta_sweep_next' non-NULL automatically generates a complete
     set of metadata-bin references for the request, taking all codestream
     windows into account simultaneously.  Better incremental handling of
     metadata for requests with a large number of codestream windows is left
     as something for the future.  For the moment, we take the stated approach
     because it prevents us from having to make many redundant passes through
     the metadata tree.
        Once `sweep_next' and `meta_sweep_next' both become NULL, no further
     sequencing of active data-bins is required and indeed the next call to
     `sequence_active_bins' will leave the `active_bins' list entirely empty.
     However, one or both of these conditions may be reversed by the handling
     of abandoned data chunks.
        `last_translated_window' holds a copy of the window, as passed into
     `kdu_serve::set_window', which was last translated via
     `process_window_changes'.  If `window_changed' is false, this window
     differs from `current_window' only in regard to changes which have
     been made by the server, either in translating codestream contexts,
     or in restricting the region of interest so as to conserve resources.
     If `window_changed' is true, the `last_translated_window' and
     `current_window' members may be used to identify any changes between
     the previous and current requests.
        The `active_bins' member points to a linked list of active data-bin
     references, in which all metadata-bins appear at the start, followed
     by references to stream-header, tile-header and precinct data-bins.  The
     last metadata-bin reference is identified by `last_active_metabin', which
     allows the codestream data-bin references to cleanly separated from the
     metadata-bin references.
        The `old_active_bins' member is used by `sequence_active_bins' to store
     a temporary list of previous active codestream data-bin references, which
     we don't want to release until after new active data-bin references have
     been fully generated (so we don't have to reconstruct active precinct
     resources which are in common between successive sets of active
     data-bins).  By storing the old list here, we ensure that it can be
     properly released in the event that an exception is caught during the
     sequencing of new active data-bins.  The same reasoning lies behind the
     `new_active_bins' list which stores new active codestream data-bin
     references which we cannot add to `active_bins' until they have all
     been generated, but we don't want the list to be lost in the event that
     an exception is thrown.
        `active_bin_ptr' holds a pointer to the next element of the
     `active_bins' list which is to be considered by the
     `simulate_packet_generation' function.
        `active_threshold' holds the threshold which is currently being used by
     `simulate_packet_generation' to determine the next packet contribution
     to be included in the batch of increments being prepared by
     `generate_increments'.  At each element in the active precinct
     list, `active_threshold' is compared against `layer_log_slopes'[i] +
     delta where `i' is the value of the element's `next_layer' member,
     delta is the value of the element's `log_relevance' member, and
     `layer_log_slopes' is the `kd_codestream_window::layer_log_slopes' array
     for the element's code-stream.  If `active_threshold' is less than or
     equal to this value, the number of packets for that precinct is
     considered for augmentation.
        `next_active_threshold' is the value which should be assigned to
     `active_threshold' once `active_ptr' reaches the end of the `active_bins'.
     It is computed incrementally, while the active list is being processed,
     in such a way as to minimize the number of processing passes, while
     maintaining the desired ordering properties.  Specifically, while scanning
     through the list of active precincts, the value of `next_active_threshold'
     is set to the smallest value which ensures that no more than 1 layer
     will be included from any precinct during the next scan.
       `scan_first_layer' is set to true whenever the `active_bins' list
     is changed.  When this member is true, the next scan through the active
     bin list inside `simulate_increments' will ignore the `active_threshold'
     value and consider the first layer of every precinct as a candidate for
     inclusion in the increments being prepared by `generate_increments',
     unless that layer has already been included.  The reason for treating
     the first layer differently is that its log-slope-threshold value is
     not a reliable indication of the actual distortion-length slopes of the
     underlying code-block coding passes; the slopes may have been arbitrarily
     steeper than this threshold.  As a result, the R-D optimal layer
     resequencing algorithm may fail to allocate sufficient weight to
     lower resolution precincts, which often have reduced log-relevance
     due to the fact that their region of influence is larger than the
     window region.  After the first scan through the active precinct list,
     `scan_first_layer' reverts to false, but it may be set to true again
     if the cache model for one of the active precincts is downgraded.
        `scanned_precinct_bytes' is set to 0 at the start of each scan
     through the active bin list inside `simulate_increments', and then
     used to add up the `kd_active_precinct::sim_bytes' values associated
     with all encountered precinct data-bin references.  Similarly,
     `scanned_precinct_samples' is used to accumulate the total number of
     precinct samples for which code bytes are accumulated by
     `scanned_precinct_bytes'.  This allows the `simulate_increments'
     function to determine whether sufficient data is available for a
     partial collection of active streams and precincts to move on to a new
     set.
        `scanned_incomplete_metabins' and `scanned_incomplete_codestream_bins'
     are used to count the number of outstanding meta data-bins and
     codestream-related data-bins on the `active_bins' list, for which
     data still needs to be dispatched.  As with the other `scanned_...'
     members, these counters are set to 0 when `active_ptr'=`active_bins'
     and their values are checked at the end of a complete scan through the
     set of active data-bins.
  */

/*****************************************************************************/
/*                            kd_window_sequencer                            */
/*****************************************************************************/

struct kd_window_sequencer {
  public: // Functions
    kd_window_sequencer() { reset(); }
    void reset() { t_idx.x=t_idx.y=0; cn=0; r_idx=0; p_idx.x=p_idx.y=0; }
    bool operator==(const kd_window_sequencer &rhs) const
      { 
        return ((t_idx==rhs.t_idx) && (cn==rhs.cn) &&
                (r_idx==rhs.r_idx) && (p_idx==rhs.p_idx));
      }
  public: // Data
    kdu_coords t_idx; // Relative index of next tile to sequence from window
    int cn; // See below
    int r_idx; // Resolution index (0 is LL band) of next data-bin to sequence
    kdu_coords p_idx; // Relative index of next precinct to sequence
  };
  /* Notes:
        This structure is used to record the state associated with a single
     `kd_codstream_window', when sequencing data-bins using the machinery
     offered by `kd_codestream_window::sequence_active_bins'.
        The interpretation of the above member variables generally requires
     the relevant `kd_stream' object to be in the "expanded" state so that the
     structural parameters of every tile are known.  The `t_idx' and `p_idx'
     parameters hold coordinates which start from (0,0), measured relative
     to the set of tiles (resp. precincts) which are relevant to the
     associated `kd_codestream_window'.
        The `cn' member is an index into an array of codestream image
     components which cannot be formed without accessing the relevant tile
     in the interchange codestream managed by an expanded `kd_stream' object;
     it depends on the coding parameters associated with the tile, together
     with the image components of interest to the `kd_codestream_window'
     whose data-bins are to be sequenced.  Nevertheless, the counter starts
     out at 0 and advances from there, as with `r_idx'. */

/*****************************************************************************/
/*                           kd_codestream_window                            */
/*****************************************************************************/

struct kd_codestream_window {
  public: // Member functions
    kd_codestream_window(kd_serve *owner);
    ~kd_codestream_window();
    void initialize(kd_stream *stream, kd_window_context *context,
                    kdu_coords resolution, kdu_dims region,
                    int round_direction, int max_required_layers,
                    kdu_range_set &component_ranges,
                    int num_context_components,
                    const int *context_component_indices);
      /* This function is called from within `kd_serve::get_codestream_window'
         to associates the object with the given `stream', along with the
         supplied `window_context', configuring the `window_discard_levels',
         `window_max_layers, `window_region', `window_tiles',
         `component_ranges' and `context_component_indices' members.
         Note that a non-positive value for `max_required_layers' is
         interpreted as no limit at all.
         [//]
         The function may also be called from within
         `kd_window_context::process_window_changes' to adjust some of the
         above parameters.  This case is easily distinguished from the first
         by the fact that the `stream' and `context' members are already
         set correctly, as opposed to holding NULL.  Making the distinction
         is important because in the first case, the function automatically
         adds the codestream window to the `stream->windows' list.
         [//]
         Note that this function does not require the `stream' object to be
         in the "attached" or even "expanded" state; no attempt will be made
         to access tiles or their parameters.  As a result, the codestream
         window can be initialized without having to actually occupy shared
         resources in the `kdu_serve_target' object.
         [//]
         The `component_ranges' argument identifies the set of
         codestream components which might belong to the window.  If this is
         an empty set, the internal `component_ranges' member is
         initialized to represent all codestream image components.  If
         `num_context_components' <= 0, there are no further restrictions
         on the set of relevant codestream components.  Otherwise,
         `context_component_indices' must point to an array with
         `num_context_components' entries, which identify the set of
         output image components which are of interest -- these have
         been derived from a codestream context.  For a discussion of
         the differences between output and codestream image components,
         see the `kdu_codestream::apply_input_restrictions' function.
         For our present purposes here, it is sufficient to realize that
         each output component may be formed from multiple codestream
         components and that the mapping between output components and
         codestream components may vary from tile-to-tile.
      */
    int get_max_extra_discard_levels();
      /* Returns the number of additional discard levels (beyond the installed
         `window_discard_levels' value) which can have an effect on the
         behaviour of the `sequence_active_bins' function. */
    kdu_long get_window_samples(int extra_discard_levels);
      /* This function returns an estimate of the total number of samples
         associated with the codestream window, assuming that the
         `window_discard_levels' member is augmented by `extra_discard_levels'.
         This estimate is based purely on information generated by
         `initialize' and does not require the associated `kd_stream' object
         to be "attached" or even "expanded".  In particular, we cannot
         explicitly determine the properties of multi-component transforms,
         which can be tile-dependent, so we cannot reliably infer the number
         of codestream image components required to form output image
         components.
         [//]
         The function allows a `kd_window_context' object to determine a
         (possibly) reduced resolution at which all of its codestream windows
         can sequence their data-bins onto the active list without exceeding
         the memory constraint associated with the `KD_MAX_WINDOW_SAMPLES'
         constant. */
    bool contains(kd_codestream_window *rhs);
      /* Returns true if the current codestream window completely contains
         the `rhs' codestream window.  This is used to eliminate redundant
         codestream windows when processing a window-of-interest request
         within `kd_window_context'. */
    bool sequence_active_bins(kd_active_binref * &head,
                              kd_active_binref *res_tails[],
                              int extra_discard_levels, kdu_long max_samples,
                              kdu_window_model *model_instructions);
      /* This function implements the central algorithm used to sequence
         codestream-related data-bins onto the `active_bins' list maintained
         by a window context (see `kd_window_context').  It is important that
         even very large windows of interest, or windows of interest which
         span a massive number of codestreams, can be served in a manner
         which does not consume too much memory.  To do this,
         `kd_window_context::sequence_active_bins' issues calls to its
         `kd_codestream_window' objects' `sequence_active_bins' function in
         a sequence of stages, which may involve multiple sweeps through the
         content.  During any given sweep, a reduced resolution version of the
         codestream window may be used (as indicated by `extra_discard_levels')
         so that a remote client can obtain a reduced resolution representation
         quickly, in the event that the server cannot deliver everything
         together in truly quality-progressive fashion.
         [//]
         If the internal `sequencing_active' flag is false, sequencing of
         active data-bins starts from scratch; otherwise, it starts from
         where sequencing last left off.  The present function leaves
         the `sequencing_active' flag equal to true if and only if the
         process of sequencing data-bins has been started, but not yet
         completed for the current codestream window.
         [//]
         On entry, `head' point to the start of the relevant
         `kd_context_window' object's `active_bins' list, while the
         `res_tails' array contains a collection of pointers into this list,
         each of which identifies the last active binref for a given resolution
         level.  Specifically, `res_tails' is expected to hold 33 entries
         (no more and no less), all of which are initialized to NULL when
         `head' is NULL.  These entries are used to sort the elements in the
         generated `active_bins' list according to their resolution, in a
         manner which does not incur much processing overhead.  The contents
         of this array are temporary, but each non-NULL pointer must point into
         the single linked list headed by `head'.
         [//]
         The function updates `head' and the `res_tails' entries as active
         data-bins are added, returning true if anything was added to the
         list headed by `head'.  A false return means that:
              All remaining data-bins associated with this codestream window
              (starting from any pre-existing sequencing state, and accounting
              for the `extra_discard_levels' value) have already
              been fully dispatched, at least to the limit identified by
              `window_max_layers'.
         [//]
         If the function does return true, the object's `is_active' flag is
         set to true.  However, the function does not ever reset this flag.
         The `kd_window_context::sequence_active_bins' function resets
         the `is_active' flags of all previously active codestream windows
         before sequencing a fresh set of active data-bins, but the present
         function could be used differently, in which case it might necessarily
         be correct to reset the flag prior to each call to this function.
         This is why the function does not reset the flag itself.
         [//]
         The `max_samples' argument is used to limit the number of precinct
         data-bins which are sequenced before this function returns.  If
         this value is <= 0, no limit is applied.  Otherwise, the limit
         becomes exhausted when the number of samples associated with
         precint databins exceeds `max_samples'.
         [//]
         Note that when this function is called, the `stream' object is
         guaranteed to be in the "active" state.  This does not necessarily
         mean that it has been "attached" or that a cache model even exists
         yet, but it does mean that these things are allowed to happen as
         the need arises -- the number of active codestreams is limited in
         such a way that resources should exist to support the attaching and
         locking of all active codestreams simultaneously if required.
         [//]
         If `model_instructions' is non-NULL, the function processes any
         cache model instructions which apply to the content being sequenced.
         As explained with `kdu_window_model', this causes the processed
         atomic cache model instructions to be removed so that they will not
         accidentally be processed multiple times when `sequence_active_bins'
         is invoked again.  We do, however, need to be careful to process
         non-atomic cache model instructions multiple times since these
         cannot be removed from the `model_instructions' object in a reliable
         way.  Fortunately (this is by design), non-atomic cache model
         instructions are permitted only in the stateless mode and we can
         tell whether or not we are visiting a data-bin for the first time
         in stateless mode, because the entire cache model is guaranteed to
         be empty before we start processing a stateless request.
      */
    void sync_sequencer(kd_window_sequencer &seq, int extra_discard_levels);
      /* This function is invoked when a new window of interest leads to the
         creation of a new set of codestream windows with overlap with an
         older codestream window which was actively contributing data-bins
         to the increment generation process.  The older codestream window's
         `sequencer_start' member is passed in here as `seq'.  If the state
         of `seq' is compatible with the current codestream window, taking
         `extra_discard_levels' into account, `sequencer' and `sequencer_start'
         are both set to start at `seq' and `sequencing_active' is set to true
         so that the next call to `sequence_active_bins' takes off from this
         point. */
  private: // Helper functions
    int *get_scratch_components(int num_needed)
      {
        if (num_needed > max_scratch_components)
          {
            max_scratch_components += num_needed;
            if (scratch_components != NULL)
              delete[] scratch_components;
            scratch_components = new int[max_scratch_components];
          }
        return scratch_components;
      }
    int *get_scratch_pblock_model_buf(kdu_coords &size)
      { /* Returned array can hold at least 2*size.x*size.y entries. */
        int area = size.x*size.y;
        if (area > max_model_pblock_area)
          { 
            if (model_pblock_buf != NULL) delete[] model_pblock_buf;
            max_model_pblock_area = 2*area; // Avoid frequent re-allocation
            model_pblock_buf = NULL;
            model_pblock_buf = new int[2*max_model_pblock_area];
          }
        return model_pblock_buf;
      }
  public: // Members installed by `initialize'
    kd_serve *serve;
    kd_stream *stream;
    kd_window_context *context;
    kdu_range_set component_ranges; // See `initialize'
    bool codestream_comps_unrestricted; // If `component_ranges' consists of
                                        // all codestream image components
    int num_codestream_components;
    int max_codestream_components;
    int *codestream_components; // Expansion of `component_ranges'
    int num_context_components;
    int max_context_components; // Num elements in `context_component_indices'
    int *context_components;
    int window_discard_levels; // Divide canvas dims by 2^d to get dimensions
    int window_max_layers; // Max quality layers required to satisfy WOI
    kdu_dims window_region; // view dims mapped to the high-res canvas
    kdu_dims window_tiles; // Indices of tiles which cover `window_region'
  public: // Members relating codestream window to a codestream context request
    int context_type; // If != 0, window is translated from codestream context
    int context_idx; // Index of context, if `context' is non-NULL
    int member_idx; // Codestream member within context, if `context' non-NULL
    int remapping_ids[2]; // Copied from `kdu_sampled_range::remapping_ids'
  public: // State variables which change while sequencing active data-bins
    bool is_active; // If current active bins were generated by this object
    bool sequencing_active; // If `sequencer' is part way through
    kd_window_sequencer sequencer; // Current sequencing state
    kd_window_sequencer sequencer_start; // Starting point for last call to
        // `sequence_active_bins' -- reliable only if `is_active' is true
    bool content_incomplete; // See below
    bool fully_dispatched; // If all required content has been fully delivered
  public: // Links
    kd_codestream_window *next; // For managing stream windows of `context'
    kd_codestream_window *stream_next; // For managing a doubly-linked list
    kd_codestream_window *stream_prev; // of all stream windows using `stream'
  private: // Scratch space for calculating tile-components in use
    int max_scratch_components;
    int *scratch_components;
    int max_model_pblock_area;
    int *model_pblock_buf;
  };
  /* Notes:
        Each codestream window contributes some set of codestream elements
     (precinct data-bins and header data-bins) to its containing window
     context (as given by the `context' member).  To provide for requests
     which cover very large windows of interest, or very many codestreams, we
     cannot generally load all such data-bins into memory at once and run the
     layer progressive (or virtual layer progressive) scheduling algorithm
     over them.  Instead, we must be prepared to schedule codestream elements
     into the active data-bin set managed by `kd_window_context' progressively
     and then generate increments in layer progressive fashion from just these
     active data-bins.  There are thus two additional elements in the machinery
     beyond layer progressive generation of data-bin increments which must be
     realized: 1) machinery to sequence data-bins into the active working set;
     and 2) machinery to determine how much data should be generated for the
     active set before we advance the sequencing machinery.  None of this is
     all that simple, but very important for managing memory consumption and
     data access costs in the server.
        The `sequencing_active' member indicates whether or not the
     sequencing machinery has been started and has not yet finished passing
     through all relevant data-bins in calls to `generate_active_bins'.
        The `content_incomplete' flag plays an important role in avoiding
     unnecessary accesses to codestream content.  When the sequencing state
     machinery is started from scratch, this flag is reset to false, unless
     `sequence_active_bins' is invoked with a non-zero `extra_discard_levels'
     argument, in which case it is set to true.  Once active data-bins have
     been processed, if it turns out that their contents were not completely
     dispatched by `kd_window_context::generate_increments', the codestream
     windows from which they came have their `content_incomplete' flags set
     to true.  If the active data-bin sequencing machinery moves past a
     particular codestream window and its `content_complete' flag is still
     set to zero, the `fully_dispatched' flag can be set to true.
  */

/*****************************************************************************/
/*                                 kd_serve                                  */
/*****************************************************************************/

struct kd_serve {
  public: // Member functions
    kd_serve(kdu_serve *owner);
    ~kd_serve();
    void initialize(kdu_serve_target *target, int max_chunk_size,
                    int chunk_prefix_bytes, bool ignore_relevance_info,
                    kds_id_encoder *id_encoder);
      /* Implements `kdu_serve::initialize'. */
  //---------------------------------------------------------------------------
  public: // Utility functions
    kd_active_binref *
      create_active_binref(kd_stream *stream, kd_tile *tp);
    kd_active_binref *
      create_active_binref(kd_tile *tp, kd_resolution *rp, kd_precinct *prec,
                           kdu_coords p_idx);
      /* The above two functions create active data-bin references.  The first
         function is for codestream main and tile header data-bin references.
         The second is for precinct references, providing the information
         required to instantiate a `kd_active_precinct' object if required.
         The function manages all required reference counting in a manner
         which is complemented by `release_active_binrefs', so that resources
         can be kept to a minimum within the server. */
    bool create_active_meta_binrefs(kd_active_binref * &head,
                                    kd_active_binref * &tail,
                                    kdu_window &window,
                                    kd_codestream_window *first_window,
                                    kd_codestream_window *lim_window,
                                    kdu_window_model *model_instructions);
      /* This function invokes `kd_meta::find_active_scope_and_sequence' with
         supplied parameters and then allocates the set of active data-bin
         references required to reference the relevant content, appending
         them to the list of active data-bin references, whose head
         and tail are given by `head' and `tail'.  Note that `first_window'
         (inclusive) and `lim_window' (non-inclusive) identify the range of
         codestream windows which are to be checked for matching
         imagery-specific metadata.
         [//]
         Like the `kd_codestream_window::sequence_active_bins' function, this
         function deliberately excludes references to metadata-bins which
         have already been fully dispatched, which may result in it adding
         nothing at all to the calling `kd_window_context' object's
         `active_bins' list (the list headed by `head').
         [//]
         Also like `kd_codestream_window::sequence_active_bins', this function
         processes relevant cache model manipulation instructions on the fly,
         as found in `model_instructions'.
         [//]
         The function returns true if and only if at least one meta data-bin
         reference is added. */
    void release_active_binrefs(kd_active_binref *list);
      /* This convenient function simultaneously releases the entire `list'
         of active binrefs and also reduces the reference counter associated
         with referenced `kd_active_precinct' objects, releasing these
         active precincts if required and, potentially, closing the any tile
         to which they belong, within the `interchange' codestream. */
    kd_chunk_binref *create_chunk_binref(kd_stream *str, kd_tile *tp,
                                         kd_precinct *prec, kd_meta *meta,
                                         kds_chunk *chunk);
      /* This convenient function creates a new chunk binref, initializes
         its member variables and attaches it to the `chunk->bins' list,
         returning a pointer to the chunk binref so that the caller can
         modify its initially zero-valued `prev_bytes', `prev_packets'
         and/or `prev_packet_bytes' members.  The function also increments
         the `str->num_chunk_binrefs' counter if appropriate. */
    kd_stream *get_stream(int stream_id, bool create_if_missing);
      /* This function looks for the codestream with the indicated index,
         returning a pointer to it if it exists or needs to be created from
         scratch -- NULL if it did not exist and `create_if_missing' is false.
         If a new stream is created, this function leaves it in the
         collapsed state, sitting on the `collapsed_unused_streams' list.
         The caller may need to do something about this
         later by invoking `attach_stream', which will immediately promote
         it to the `attached_unused_streams' list. */
    kd_codestream_window *
      get_codestream_window(int stream_id, kd_window_context *context,
                            kdu_coords resolution, kdu_dims region,
                            int round_direction, int max_required_layers,
                            kdu_range_set &component_ranges,
                            int num_context_components=0,
                            const int *context_component_indices=NULL);
      /* Allocates a new `kd_codestream_window' (or obtains one from the
         recycling pool) and initializes it to reference the indicated
         `stream_id' with the other arguments supplying window parameters.
         The interpretation of the various arguments is explained with
         `kd_codestream_window::initialize'.  This function is used by
         a window context when processing a new window of interest.
         Internally, the function manipulates the list of `kd_stream' objects,
         updating their reference counters and so forth. */
    void release_codestream_windows(kd_codestream_window *list);
      /* This function releases an entire list of codestream windows, returning
         them to the internal recycling list, adjusting the reference counters
         for their referenced `kd_stream' objects, and potentially moving the
         streams between internal lists of active, inactive and unused
         `kd_stream' objects.  The `kd_stream' objects retain their
         cache models and attachment status for the moment, but this may
         change if the resources need to be reclaimed in the future. */
    void add_active_context_for_stream(kd_stream *stream);
      /* This function is called from within `kd_window_context' when it
         makes a stream active.  The condition may only be brief, so we
         are careful to avoid consuming much resources when simply making a
         stream active.  The `stream->num_active_contexts' counter is
         incremented and, if it was previously zero, the stream is moved from
         the inactive list to the active list.  The function does not
         directly cause the stream to be attached to the `kdu_serve_target'
         object, but that will happen automatically when access to the
         source is required. */
    void remove_active_context_for_stream(kd_stream *stream);
      /* This function is called from within `kd_window_context' when a
         stream which was previously active within the window context is no
         longer active in that context.  The `stream->num_active_contexts'
         counter is reduced and if this leaves the counter equal to 0, the
         stream is moved to the inactive list, internally.  The function
         does not directly cause the stream to be detached from the
         `kdu_serve_target', but that may happen automatically at a later
         point if active streams need to be attached and there are insufficient
         resources to keep all inactive or unused streams attached. */
    void attach_stream(kd_stream *stream);
      /* Causes `stream' to be attached to the `kdu_serve_target' and, if
         necessary, a cache model to be created for it.  As a precurser,
         we may need to first detach some other unused (or inactive)
         codestream.  A codestream will normally be in the active state
         before this function is called, but the function may need to be
         called just to get to the point where we can process cache model
         manipulation statements. */
    void detach_stream(kd_stream *stream);
      /* Causes `stream' to be detached from the `kdu_serve_target', leaving
         its `source' interface empty.  Also updates the internal mechanism
         for keeping track of the total number of attached streams. */
    void lock_codestreams(int num_streams, kd_stream *streams[]);
    void unlock_codestreams(int num_streams, kd_stream *streams[]);
      /* The above two functions provide convenient mechanisms to lock and
         unlock an array of `num_streams' codestreams in the `kdu_serve_target'
         object, for exclusive access from this thread.  The
         `lock_codestreams' function leaves all `kd_stream' objects'
         `is_locked' flag set to true, while the `unlock_codestream' leaves
         this flag set to false.  It is an error to call `lock_codestreams'
         if any of the streams is already locked -- although this condition
         is checked for you.  It is also an error to call `lock_codestreams'
         without first unlocking all previously locked codestreams.  Finally,
         it is an error to lock or unlock any more than
         `KD_MAX_ACTIVE_CODESTREAMS' codestreams at once.  All of these
         conditions are checked. */
    void adjust_completed_streams(kd_stream *stream, bool was_complete);
      /* This function is called if something has happened which may affect
         whether or not `stream' has been completely dispatched, as identified
         by `stream->completed_tiles' and `stream->dispatched_header_complete'.
         If `was_complete' is false, the function checks to see if the stream
         is now complete -- if so, the `num_completed_codestreams' member is
         incremented.  If `was_complete' is true, the function checks to see
         if the stream is now incomplete -- if so, `num_completed_codestreams'
         is decremented.  The function is invoked whenever a stream may
         have become newly complete or newly incomplete as a result of
         the generation of data increments, the handling of abandoned data
         chunks, or the processing of cache model manipulation instructions. */
    void adjust_completed_metabins(kd_meta *meta, bool was_complete);
      /* Same as `adjust_completed_streams', but this function keeps track of
         the number of completed datadata-bins.  Note that this function will
         do nothing at all if `meta->can_complete' is zero, so you could avoid
         even calling it in that case.  The function is invoked whenever
         a meta data-bin might have become newly complete or newly incomplete
         as a result of the generation of data increments, the handling
         of abandoned data chunks, or the processing of cache model
         manipulation instructions. */
    kd_meta *find_metabin(kdu_long bin_id, kd_meta *root=NULL);
      /* Recursive function to recover the first `kd_meta' object belonging
         to the meta data-bin with the indicated absolute identifier.  This
         function relies upon `bin_id's being assigned in a recursive manner
         within the `kdu_serve_target', so that the all data-bins which
         are used to represent the contents of a box have ID's which are
         less than or equal to those of all data-bins used to represent the
         contents of its next sibling. */
    void process_metabin_model_instructions(kd_meta *meta, int i_buf[],
                                            int i_cnt);
      /* Processes the `i_cnt' digested cache model manipulation instructions
         found in `i_buf', applying them to the data-bin to which `meta'
         belongs.  For an explanation of the values found in `i_buf', refer to
         `kdu_window_model::get_meta_instructions' -- there are at most two
         entries.  Note that this function needs to extend the effect of
         the cache model instructions to the entire data-bin, which may
         include multiple `kd_meta' objects, connected via their `next_in_bin'
         and `prev_in_bin' members.  Note also that additive instructions may
         cause `meta->is_modeled' to be set (done to all objects in the
         data-bin, adding the last one to the `modeled_metabins' list), while
         all both additive and subtractive instructions may cause calls to
         `adjust_completed_metabins'. */
    void erase_metadata_cache_model();
      /* This function erases all cache model information from the metadata
         tree.  It does so by visiting only the `kd_meta' objects for which
         cache model information might exist; these are the objects on the
         list headed by `modeled_metabins' and connected via the
         `kd_meta::next_modeled' links.  The function plays an important
         role in the processing of stateless requests. */
  //---------------------------------------------------------------------------
  private: // Helper functions
    void move_stream_to_list(kd_stream *stream, kd_stream * &list);
      /* This function is used to move streams between the various lists
         managed by this object, for book-keeping purposes.  The `stream'
         is detached from whatever list it currently sits on and then
         attached to the list headed by `list'. */
  //---------------------------------------------------------------------------
  public: // Links and embedded services
    kdu_serve *owner;
    kdu_serve_target *target;
    kd_window_context *contexts; // List of window contexts
    kd_chunk_server *chunk_server;
    kd_active_precinct_server *active_precinct_server;
    kd_binref_server *binref_server;
    kd_pblock_server *pblock_server;
    kds_id_encoder default_id_encoder;
    kds_id_encoder *id_encoder;
    int max_chunk_body_bytes;
  public: // Options and preferences
    bool ignore_relevance_info;
    bool is_stateless; // Determined on first call to `kdu_serve::set_window'
    bool is_stateful; // Determined on first call to `kdu_serve::set_window'
    bool stateless_image_done; // See below
  public: // Members used to manage metadata
    kd_meta *metatree; // Metadata is global
    kd_meta *modeled_metabins; // See `kd_meta::next_modeled'
    int total_metabins;
    int num_completed_metabins;
  public: // Members used to manage codestream resources
    int total_codestreams; // Total number of codestreams offered by `target'
    int num_completed_codestreams; // Num codestreams fully dispatched
    kd_stream **stream_refs; // Array with `total_codestreams' entries
    kd_stream *collapsed_unused_streams; // Collapsed, not attached, not used
    kd_stream *lightweight_unused_streams; // Expanded, but no codestream ifc's
    kd_stream *mediumweight_unused_streams; // Interchange interfaces only
    kd_stream *attached_unused_streams; // Attached to `target' => source ifc's
    kd_stream *inactive_streams; // Used by window context, but not active
    kd_stream *active_streams; // Used by window context and active
    int num_attached_streams;
    int num_expanded_streams;
    int num_collapsed_streams;
  private: // Internal resources
    kd_codestream_window *free_codestream_windows;
    kd_active_precinct *active_precincts;
    int num_locked_stream_indices; // Used by `(un)lock_codestreams'
    int locked_stream_indices[KD_MAX_ACTIVE_CODESTREAMS];
  };
  /* Notes:
       Resource consumption in the `kdu_serve_target' object and in the
     management of cache models, depends on the state in which `kd_stream'
     objects are maintained.  As noted in connection with that object, a
     `kd_stream' can be "collapsed", "expanded but not attached",
     "attached but not locked", and "locked".  A stream can also be active,
     meaning that the need for it to be attached (and later locked) is
     anticipated, but the stream need not be attached until the need actually
     arises.
       The `stream_refs' array maintains one entry for each possible codestream
     in the `target' object.  In the future, it may be necessary to provide
     an hierarchical structure to manage a massive number of codestreams, but
     as it is one could expect to handle hours of video with one codestream
     per frame.  Many entries in the `stream_refs' array may be NULL, so we
     also reference the non-NULL entries through various lists, as described
     below.
       `collapsed_unused_streams' is a list of the `kd_stream' objects which
     are collapsed (i.e., having no `tiles' array).  These are necessarily
     not referenced from within any `kd_codestream_window' and not attached to
     the serve `target'.
       `lightweight_unused_streams' is a list of the `kd_stream' objects which
     are expanded, yet do not yet have any codestream interfaces
     (`interchange', `structure' or `source').  These are necessarily not
     referenced from within any `kd_codestream_window' or attached to the
     serve `target', yet they are able to retain cache model information.
       `mediumweight_unused_streams' is a list of the `kd_stream' objects
     which are expanded and have `interchange' and `structure' codestream
     interfaces, yet are not referenced from any `kd_codestream_window' and are
     not attached to the serve `target'.
       `attached_unused_streams' is a list of the `kd_stream' objects which
     are expanded and have a full set of `source', `interchange' and
     `structure' codestream interfaces, being attached to the serve `target',
     while not currently being referenced by any `kd_codestream_window'.
       `inactive_streams' is a list of all `kd_stream' objects which are
     currently referenced from within at least one `kd_codestream_window',
     but are not yet active.  These are necessarily expanded, but they might
     not yet have any codestream interfaces.
       `active_streams' is a list of all `kd_stream' objects which are
     currently active.  These are necessarily referenced from at least one
     `kd_codestream_window' and have at least `interchange' and `structure'
     codestream interfaces.  They will generally need to be attached to
     the serve `target' if this has not been done already.
        The `num_attached_streams' and `num_expanded_streams' members keep
     track of the total number of codestreams which are attached to the
     `kdu_serve_target' object and the total number of codestreams which
     have non-NULL `tiles' arrays, respectively.  Each of these members
     counts codestreams which may appear on more than one of the above
     lists.  The `num_collapsed_streams' counts only those
     codestreams which are collapsed (these should all be on the
     `collapsed_unused_streams' list).  Together,
     `num_collapsed_steams' and `num_expanded_streams' account for all
     `kd_stream' objects which are currently in existence.
        The object operates only in stateless or stateful mode.  The
     `is_stateless' argument found in the first call to `kdu_serve::set_window'
     determines which mode is being used and it is illegal to mix request
     types.  In the stateless mode, the `stateless_image_done' flag is used
     to record whether or not the entire target's contents have been
     dispatched by `generate_increments' in response to the most recent
     window of interest request.  This flag is reset whenever a new window
     of interest arrives, and set to true if `num_completed_codestreams' is
     found to be equal to `total_codestreams' and `num_completed_metabins'
     is found to be equal to `total_metabins' at the point when
     `kd_window_context::generate_increments' finishes generating all
     content for its window of interest.
  */

/*****************************************************************************/
/*                               kd_chunk_server                             */
/*****************************************************************************/

class kd_chunk_server {
  public: // Member functions
    kd_chunk_server(int chunk_size)
      {
        this->chunk_size = chunk_size; chunk_prefix_bytes = 0;
        chunk_groups = NULL; free_chunks = NULL;
      }
    ~kd_chunk_server();
    void set_chunk_prefix_bytes(int prefix_bytes)
      { /* Call this function to change the number of prefix bytes which are
           reserved for the application in each subsequent `kds_chunk' object
           returned by `get_chunk'.  The default prefix size is 0, but the
           value may be changed at any point. */
        assert(prefix_bytes < chunk_size);
        chunk_prefix_bytes = prefix_bytes;
      }
    kds_chunk *get_chunk();
      /* The returned chunk structure has most members initialized to 0,
         except for `max_bytes', `prefix_bytes' and `num_bytes'.  The value
         of `num_bytes' is initialized to `prefix_bytes'. */
    void release_chunk(kds_chunk *chunk);
      /* Releases a single chunk. */
  private: // Definitions
      struct kd_chunk_group {
          kd_chunk_group *next;
        };
  private: // Data
    int chunk_size;
    int chunk_prefix_bytes;
    kd_chunk_group *chunk_groups;
    kds_chunk *free_chunks;
  };

/*****************************************************************************/
/*                         kd_active_precinct_server                         */
/*****************************************************************************/

class kd_active_precinct_server {
  public: // Member functions
    kd_active_precinct_server()
      { groups = NULL; free_list = NULL; }
    ~kd_active_precinct_server();
    kd_active_precinct *get_precinct();
      /* The returned record has all fields initialized to zero. */
    void release_precinct(kd_active_precinct *elt)
      { elt->next=free_list; free_list=elt; }
  private: // Definitions
      struct kd_active_precinct_group {
          kd_active_precinct_group *next;
          kd_active_precinct precincts[32];
        };
  private: // Data
    kd_active_precinct_group *groups;
    kd_active_precinct *free_list; // List of recycled precincts
  };

/*****************************************************************************/
/*                             kd_binref_server                              */
/*****************************************************************************/

class kd_binref_server {
  public: // Member functions
    kd_binref_server()
      { groups = NULL; free_list = NULL; }
    ~kd_binref_server();
    kd_active_binref *get_active_binref()
      { return (kd_active_binref *) get_binref(); }
    kd_chunk_binref *get_chunk_binref()
      { return (kd_chunk_binref *) get_binref(); }
      /* The returned records are not initialized. */
    void release_binrefs(kd_binref *list)
      { // Releases an entire list at once, connected via the `next' pointers.
        kd_binref *elt;
        while ((elt=list) != NULL)
          { list = elt->next; elt->next=free_list; free_list=elt; }
      }
  private: // Helper functions
    kd_binref *get_binref();
  private: // Definitions
    union kd_any_binref {
      kd_active_binref active_binref;
      kd_chunk_binref chunk_binref;
    };
    struct kd_binref_group {
      kd_binref_group *next;
      kd_any_binref binrefs[32];
    };
  private: // Data
    kd_binref_group *groups;
    kd_binref *free_list; // List of recycled bin refs
  };

/*****************************************************************************/
/*                             kd_pblock_server                              */
/*****************************************************************************/

class kd_pblock_server {
  public: // Member functions
    kd_pblock_server() { lim_log_pblock_size=0; free_lists=NULL; }
    ~kd_pblock_server();
    kd_precinct_block *get_pblock(int log_pblock_size);
      /* Returns a precinct block, whose values are initialized to 0, which
         is capable of supporting 2^(2*`log_pblock_size') precincts
         cache models. */
    void release_pblock(kd_precinct_block *pb, int log_pblock_size);
      /* Releases the object allocated using `get_pblock'. */
  private: // Data
    int lim_log_pblock_size;
    kd_precinct_block **free_lists; // `lim_log_pblock_size' lists
  };

#endif // SERVE_LOCAL_H
