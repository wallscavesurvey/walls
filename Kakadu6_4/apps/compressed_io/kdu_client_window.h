/*****************************************************************************/
// File: kdu_client_window.h [scope = APPS/CLIENT-SERVER]
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
  Describes the interface for the `kdu_window' class which is used to describe
generic windows into a JPEG2000 image (or file with multiple images), for
use with JPIP client and server components.
******************************************************************************/
#ifndef KDU_CLIENT_WINDOW_H
#define KDU_CLIENT_WINDOW_H

#include "kdu_compressed.h"
#include "jp2.h"

// Defined here:
struct kdu_sampled_range;
class  kdu_range_set;
struct kdu_metareq;
struct kdu_window;
struct kdu_window_prefs;
class kdu_window_model;

// The following are used for the `kdu_sampled_range::context_type' member
#define KDU_JPIP_CONTEXT_NONE       ((int) 0)
#define KDU_JPIP_CONTEXT_JPXL       ((int) 1)
#define KDU_JPIP_CONTEXT_MJ2T       ((int) 2)
#define KDU_JPIP_CONTEXT_TRANSLATED ((int) -1)

// The following flags are used in the `kdu_metareq::qualifier' member
#define KDU_MRQ_ALL     ((int) 1)
#define KDU_MRQ_GLOBAL  ((int) 2)
#define KDU_MRQ_STREAM  ((int) 4)
#define KDU_MRQ_WINDOW  ((int) 8)
#define KDU_MRQ_DEFAULT ((int) 14) // GLOBAL | STREAM | WINDOW
#define KDU_MRQ_ANY     ((int) 15)

// Mutually exclusive WINDOW-PREF flags and their mask
#define KDU_WINDOW_PREF_FULL           ((int) 0x00000001)
#define KDU_WINDOW_PREF_PROGRESSIVE    ((int) 0x00000002)
#define KDU_WINDOW_PREF_MASK           ((int) 0x00000003)

// Mutually exclusive CONCISENESS-PREF flags and their mask
#define KDU_CONCISENESS_PREF_CONCISE   ((int) 0x00000010)
#define KDU_CONCISENESS_PREF_LOOSE     ((int) 0x00000020)
#define KDU_CONCISENESS_PREF_MASK      ((int) 0x00000030)

// Mutually exclusive PLACEHOLDER-PREF flags and their mask
#define KDU_PLACEHOLDER_PREF_INCR      ((int) 0x00000100)
#define KDU_PLACEHOLDER_PREF_EQUIV     ((int) 0x00000200)
#define KDU_PLACEHOLDER_PREF_ORIG      ((int) 0x00000400)
#define KDU_PLACEHOLDER_PREF_MASK      ((int) 0x00000700)

// Mutually exclusive CODESTREAM-SEQ-PREF flags and their mask
#define KDU_CODESEQ_PREF_FWD           ((int) 0x00001000)
#define KDU_CODESEQ_PREF_BWD           ((int) 0x00002000)
#define KDU_CODESEQ_PREF_ILVD          ((int) 0x00004000)
#define KDU_CODESEQ_PREF_MASK          ((int) 0x00007000)

// Preference flags which have separate parameters in `kdu_window_pref'
#define KDU_MAX_BANDWIDTH_PREF         ((int) 0x00010000)
#define KDU_BANDWIDTH_SLICE_PREF       ((int) 0x00020000)
#define KDU_COLOUR_METH_PREF           ((int) 0x00040000)
#define KDU_CONTRAST_SENSITIVITY_PREF  ((int) 0x00080000)

// Flags for `kdu_window_model'
#define KDU_WINDOW_MODEL_COMPLETE        ((int) 0x00000001)
#define KDU_WINDOW_MODEL_LAYERS          ((int) 0x00000002)
#define KDU_WINDOW_MODEL_SUBTRACTIVE     ((int) 0x00000004)


/*****************************************************************************/
/*                            kdu_sampled_range                              */
/*****************************************************************************/

struct kdu_sampled_range {
  /* [BIND: copy]
     [SYNOPSIS]
       This structure is used to keep track of a range of values which may
       have been sub-sampled.  In the context of the `kdu_window' object,
       it provides an efficient mechanism for expressing lists of code-stream
       indices or image component indices.
       [//]
       The range includes all values of the form `from' + k*`step' which
       are no larger than `to', where k is a non-negative integer.
       [//]
       In addition to ranges, the object may also be used to keep track of
       geometric qualifiers and expansions (in terms of member codestreams)
       for ranges of higher level objects such as JPX compositing layers.
  */
  //---------------------------------------------------------------------------
  public: // Member functions
    kdu_sampled_range()
      {
        remapping_ids[0]=remapping_ids[1]=-1; context_type=0; expansion=NULL;
        from=0; to=-1; step=1;
      }
      /* [SYNOPSIS]
           Default constructor, creates an empty range with `to' < `from'.
      */
    kdu_sampled_range(const kdu_sampled_range &src)
      { *this = src; this->expansion = NULL; }
      /* [SYNOPSIS]
           Copy constructor -- note that any range `expansion' information is
           lost.  Expansion information is used only by `kdu_window' and
           must be retrieved using `kdu_window::access_context_expansion'.
      */
    kdu_sampled_range(int val)
      {
        remapping_ids[0]=remapping_ids[1]=-1; context_type=0; expansion=NULL;
        from=to=val; step=1;
      }
      /* [SYNOPSIS]
           Constructs an empty range which represents a single value, `val'.
      */
    kdu_sampled_range(int from, int to)
      {
        remapping_ids[0]=remapping_ids[1]=-1; context_type=0; expansion=NULL;
        this->from=from; this->to=to; step=1; assert(to >= from);
      }
      /* [SYNOPSIS]
           Constructs a simple range (not sub-sampled) from `from' to `to',
           inclusive.  The range is empty if `to' < `from'.
      */
    kdu_sampled_range(int from, int to, int step)
      {
        remapping_ids[0]=remapping_ids[1]=-1; context_type=0; expansion=NULL;
        this->from=from; this->to=to; this->step=step;
        assert((to >= from) && (step >= 1));
      }
      /* [SYNOPSIS]
           Constructs the most general sub-sampled range.  `step' must be
           strictly greater than 0.  The range is empty if `to' < `from'.
      */
    bool is_empty() const { return (to < from); }
      /* [SYNOPSIS]
           Returns true if the range is empty, meaning that its `to' member
           is smaller than its `from' member.
      */
    bool operator!() { return (to < from); }
      /* [SYNOPSIS]
           Same as `is_empty'.
      */
    int get_from() const { return from; }
      /* [SYNOPSIS] Gets the value of the public `from' member.  Useful for
         non-native language bindings such as Java. */
    int get_to() const { return to; }
      /* [SYNOPSIS] Gets the value of the public `to' member.  Useful for
         non-native language bindings such as Java. */
    int get_step() const { return step; }
      /* [SYNOPSIS] Gets the value of the public `step' member.  Useful for
         non-native language bindings such as Java. */
    int get_remapping_id(int which) const
      { return ((which>=0)&&(which<2))?(remapping_ids[which]):0; }
      /* [SYNOPSIS] Retrieves entries in the public `remapping_ids' member
         array. If `which' is anything other than 0 or 1, the function
         simply returns 0. */
    int get_context_type() const { return context_type; }
      /* [SYNOPSIS] Gets the value of the public `context_type' member.
         Useful for non-native language bindings such as Java. */
    void set_from(int from) { this->from=from; }
      /* [SYNOPSIS] Sets the value of the public `from' member.  Useful for
         non-native language bindings such as Java. */
    void set_to(int to) { this->to=to; }
      /* [SYNOPSIS] Sets the value of the public `to' member.  Useful for
         non-native language bindings such as Java. */
    void set_step(int step) { this->step=step; }
      /* [SYNOPSIS] Sets the value of the public `step' member.  Useful for
         non-native language bindings such as Java. */
    void set_remapping_id(int which, int id_val)
      { if ((which >= 0) && (which < 2)) remapping_ids[which]=id_val; }
      /* [SYNOPSIS] Sets entries in the public `remapping_ids' member
         array. If `which' is anything other than 0 or 1, the function
         does nothing. */
    void set_context_type(int ctp) { this->context_type=ctp; }
      /* [SYNOPSIS] Sets the value of the public `context_type' member.
         Useful for non-native language bindings such as Java. */
  public: // Data
    int from;
      /* [SYNOPSIS] Inclusive, non-negative lower bound of the range. */
    int to;
      /* [SYNOPSIS] Inclusive, non-negative upper bound of the range. */
    int step;
      /* [SYNOPSIS] Strictly positive step value. */
    int remapping_ids[2];
      /* [SYNOPSIS]
           Two parameters which may be used to signal the mapping of
           the view-window resolution and region onto the coordinates of
           particular codestreams.  The interpretation of these parameters
           depends on the `context_type' member.  They shall be ignored
           if `context_type' is 0.
      */
    int context_type;
      /* [SYNOPSIS]
           This member is 0 (`KDU_JPIP_CONTEXT_NONE') unless the present
           object describes a range of image entities (e.g. JPX compositing
           layers or MJ2 video tracks) within a codestream context, or a
           range of codestream indices which were obtained by translating
           such a context.  Currently, the only legal non-zero values are
           those defined by the following macros:
           [>>] `KDU_JPIP_CONTEXT_JPXL' --
                In this case, the indices in the sampled range correspond
                to JPX compositing layer indices, starting from 0 for the
                first compositing layer in the data source.  Ranges with this
                context type may be used by the `kdu_window::contexts' range
                set.  The `remapping_ids' correspond (in order) to the
                index (from 0) of an instruction set box, and the index
                (from 0) of an instruction within that instruction set
                box, which together define the mapping of view-window
                coordinates from a complete composited image to the
                individual codestream reference grids.  If
                `remapping_ids'[0] is negative, the view-window
                coordinates are mapped from the compositing layer registration
                grid (rather than a complete composited image) to the
                individual codestream reference grids.  For a full set of
                equations describing this remapping process, see the JPIP
                standard (FCD-2).
           [>>] `KDU_JPIP_CONTEXT_MJ2T' -- In this case, the indices in the
                sampled range correspond to MJ2 video tracks, starting from
                1 for the first track (a quirk of MJ2 is that tracks are
                numbered from 1 rather than 0).  Ranges with this context type
                may be used by the `kdu_window::contexts' range set.  In this
                case, `remapping_ids'[0] is used to control view window
                transformations, while `remapping_ids'[1] is used to control
                temporal transformations.  A negative value for
                `remapping_ids'[0] means that there shall be no view window
                remapping; a value of 0 means that remapping is
                to be performed at the track level; and a value of 1 means
                that remapping is to be performed at the movie level.  A
                negative value for `remapping_ids'[1] means that the first
                codestream in the track is to be taken as the start of the
                range of codestreams which belong to the context.  A value
                of 0 means that the context starts with the first
                codestream in the track which was captured at the time
                when the request is processed (the JPIP "now" time).  The
                value of `remapping_ids'[1] may be ignored if video is
                not being captured live.
           [>>] `KDU_JPIP_CONTEXT_TRANSLATED' -- In this case, the indices in
                the sampled range correspond to codestreams which have been
                implicitly included within the `kdu_window::codestreams'
                range set as a result of translating a codestream context
                found in the `kdu_window::contexts' range set.  Such entries
                may be generated by a JPIP server.  The `remapping_ids'[0]
                value, in this case, holds the index (starting from 0) of
                the context range, found within `kdu_window::contexts' whose
                translation yielded the codestreams identified by the
                present range.  `remapping_ids'[1] identifies which element
                of the referenced context range was translated.  A value
                of 0 means that the first element in the referenced context
                range was translated to yield the range of codestreams
                identified here.  A value of 1 means that the second element
                in the context range was translated, and so forth.  As an
                example, suppose the second sampled range in
                `kdu_window::contexts' holds the range "5-8:2" with
                context-type `KDU_JPIP_CONTEXT_JPXL', then if
                the present range has `remapping_ids'[0]=1 and
                `remapping_ids'[1]=1, the range represents codestreams
                (and their associated view-windows) which were translated
                from the JPX compositing layer with index 7.
      */
    kdu_range_set *expansion;
      /* [SYNOPSIS]
           If non-NULL, this member points to a `kdu_range_set' which
           provides an expansion of the codestream identifiers associated
           with a context.  The field may be non-NULL only if `context_type'
           is non-zero and refers to a codestream context (e.g., JPX
           compositing layers or MJ2 video tracks).  Moveover, the individual
           `kdu_sampled_range' objects within the referenced
           `expansion' must have zero-valued `context_type' members.
           [//]
           The object referenced by `expansion' is not considered to be
           owned by the present object, and it will not be destroyed when
           the present object is destroyed.  It must be managed separately.
           In particular, `kdu_window' provides for the management of
           codestream context expansions.
           [//]
           In JPIP client-server communication, expansions play a slightly
           different role to codestream ranges with the
           `KDU_JPIP_CONTEXT_TRANSLATED' context type.  Specifically, when
           a client request includes one or more codestream contexts,
           managed by `kdu_window::contexts', the server responds by
           sending back the requested codestream contexts (or as many as
           it could translate), expanding each into a set of codestream
           ranges.  The server also sends back information about the actual
           codestreams for which it will serve data, which are generally
           a subset of those requested, directly or indirectly via
           codestream contexts.  This information is managed by the
           `kdu_window::codestreams' range set.  Ranges in that set have
           the special `KDU_JPIP_CONTEXT_TRANSLATED' context type if and
           only if they represent data which is being served as a result
           of translating a codestream context.  Otherwise they have a
           context type of 0.  The distinction between codestreams which
           are being served as a result of context translation and
           codestreams which are being served as a result of a direct
           codestream request can be very important.  It allows the
           client and server to figure out whether or not a new request
           is the same, a superset of, or a subset of a previous request,
           possibly modified by the server, for which a response is in
           progress.
      */
  };

/*****************************************************************************/
/*                              kdu_range_set                                */
/*****************************************************************************/

class kdu_range_set {
  /* [BIND: reference]
     [SYNOPSIS]
       This structure is used to keep track of a set of indices.  The set is
       represented as a union of (potentially sub-sampled) ranges, i.e. as
       a union of `kdu_sampled_range's.  The lower bounds of the ranges
       are arranged in order, from smallest to largest, but the ranges
       might still overlap.  Each sampled range may have an additional
       geometric mapping qualifier.  Ranges with different geometric mapping
       qualifiers will not be merged together into a single range, even
       if they do overlap.
  */
  //---------------------------------------------------------------------------
  public: // Member functions
    kdu_range_set() { ranges=NULL; num_ranges=max_ranges=0; next=NULL; }
    kdu_range_set(const kdu_range_set &src)
      { assert(0); }
      // For the moment, it is simplest to just disallow copy constructors.
      // We want to encourage range sets to be trasferred by reference across
      // function boundaries.
    KDU_AUX_EXPORT ~kdu_range_set();
    kdu_range_set &operator=(const kdu_range_set &src)
      { copy_from(src); return *this; }
      /* [SYNOPSIS] Invokes `copy_from' -- does not copy expansions. */
    KDU_AUX_EXPORT void copy_from(const kdu_range_set &src);
      /* [SYNOPSIS]
           Effectively initializes the current object and then adds each of
           the ranges in the `src' object.  Does not copy expansions -- i.e.,
           leaves the `kdu_sampled_range::expansions' member to NULL in
           each copied range.
      */
    bool is_empty() const { return (num_ranges==0); }
      /* [SYNOPSIS]
           Returns true if the set is empty -- no indices are present.
      */
    bool operator!() const { return (num_ranges==0); }
      /* [SYNOPSIS]
           Same as `is_empty'.
      */
    KDU_AUX_EXPORT bool contains(const kdu_range_set &rhs,
                                 bool empty_set_defaults_to_zero=false) const;
      /* [SYNOPSIS]
           Returns true if this set contains all the indices which belong to
           the `rhs' object.  Expansions (see `kdu_sampled_range::expansion')
           are not checked to determine containment, since it is assumed that
           any expansion of an image entity into its codestreams will always
           be the same.  Likewise, ranges with a
           `kdu_sampled_range::context_type' value of
           `KDU_JPIP_CONTEXT_TYPE_TRANSLATED' are also deliberately ignored,
           since they represent server translations of information which is
           otherwise being signalled.
           [//]
           If `empty_set_defaults_to_zero' is true, an empty range set is
           treated as if it contained the single index 0, for the purpose
           of comparison.
       */
    bool equals(const kdu_range_set &rhs,
                bool empty_set_defaults_to_zero=false) const
      { return (this->contains(rhs) && rhs.contains(*this)); }
      /* [SYNOPSIS]
           Returns true if this object holds exactly the same set of
           indices as the `rhs' object.  As for the `contains' function,
           expansions are not checked, and ranges with a
           `kdu_sampled_range::context_type' value of
           `KDU_JPIP_CONTEXT_TYPE_TRANSLATED' are also deliberately ignored,
           since they represent server translations of information which is
           otherwise being signalled.
           [//]
           If `empty_set_defaults_to_zero' is true, an empty range set is
           treated as if it contained the single index 0, for the purpose
           of comparison.
      */
    bool operator==(const kdu_range_set &rhs) const
      { return this->equals(rhs); }
      /* [SYNOPSIS] Same as `equals'. */
    bool operator!=(const kdu_range_set &rhs) const
      { return !this->equals(rhs); }
      /* [SYNOPSIS] Opposite of `equals'. */
    void init() { num_ranges = 0; }
      /* [SYNOPSIS]
           Initializes the object to the empty set, but does not destroy
           previously allocated internal resources.
      */
    KDU_AUX_EXPORT void
      add(kdu_sampled_range range, bool allow_merging=true);
      /* [SYNOPSIS]
           Add a new (possibly sub-sampled) range of indices to those already
           in the set.  The function may simply add this range onto the end
           of the list of ranges, but it might also find a more efficient
           way to represent the indices with the same, or a smaller number of
           ranges.  If the new range fills in a hole, for example, the
           number of distinct ranges in the list might be reduced.  The
           function is not required to find the most efficient representation
           for the set of indices represented by all calls to this
           function since the last call to `init', since finding the
           most efficient representation is a non-trivial problem,
           and probably not worth the effort.
           [//]
           The function adjusts the `to' members of the various ranges
           to ensure that they take the smallest possible values which
           are consistent with the set of indices represented by the
           range.  This means that the `to' member will actually be equal
           to the last index in the range.
           [//]
           Where the `range.context_type' member is non-zero, the range will
           be merged only with other sampled ranges having exactly the same
           context type and exactly the same remapping identifiers
           (see `kdu_sampled_range::context_type' and
           `kdu_sampled_range::remapping_ids').
         [ARG: allow_merging]
           If false, the range will not be merged with any others; it will
           simply be appended to the end of the existing set of ranges.  This
           is particularly useful when constructing expansions of requested
           codestream contexts, since it is most useful for the client to
           receive separate expansions for each individually requested
           codestream context range, without having them merged.
      */
    void add(int val) { add(kdu_sampled_range(val)); }
      /* [SYNOPSIS]
           Adds a single index to the set.  Equivalent to calling the first
           form of the `add' function with a range which contains just the
           one element, `val', with a `context_type' value of 0.
      */
    void add(int from, int to) { add(kdu_sampled_range(from,to)); }
      /* [SYNOPSIS]
           Adds a contiguous range of indices to the set.  Equivalent to
           calling the first form of the `add' function with a
           `kdu_sampled_range' object whose `kdu_sampled_range::from' and
           `kdu_sampled_range::to' members are identical to the `from' and
           `to' arguments, with `kdu_sampled_range::step' set to 1 and
           `kdu_sampled_range::context_type' set to 0.
      */
    int get_num_ranges() const { return num_ranges; }
      /* [SYNOPSIS]
           Returns the number of distinct ranges in the set.
      */
    kdu_sampled_range get_range(int n) const
      {
        if ((n >= 0) && (n < num_ranges))
          return ranges[n];
        return kdu_sampled_range(); // Empty range
      }
      /* [SYNOPSIS]
           Returns the `n'th element from the internal list of
           (potentially sub-sampled) index ranges whose union represents
           the set of all indices in the set.  `n' must be
           non-negative.  If it is equal to or larger than the number of
           ranges in the list, the function returns an empty range, having
           its `to' member smaller than its `from' member.
           [//]
           Note that this function returns a copy of the internal range,
           which is devoid of any context expansion -- i.e.,
           `kdu_sampled_range::context_expansion' is NULL.
      */
    kdu_sampled_range *access_range(int n)
      {
        if ((n >= 0) && (n < num_ranges))
          return ranges+n;
        else
          return NULL;
      }
      /* [SYNOPSIS]
           Same as `get_range' except that it returns a pointer to the
           internal `kdu_sampled_range' object, rather than a copy of its
           contents.  Returns NULL if `n' is not a valid range index.
      */
    bool test(int index) const
      {
        for (int n=0; n < num_ranges; n++)
          if ((ranges[n].from >= 0) && (ranges[n].from <= index) &&
              (ranges[n].to >= index) &&
              ((ranges[n].step == 1) ||
               (((index-ranges[n].from) % ranges[n].step) == 0)))
            return true;
        return false;
      }
      /* [SYNOPSIS]
           Performs membership testing.  Returns true if `index' belongs
           to the set, meaning that it can be found within one of the
           ranges whose union represents the set.
      */
    KDU_AUX_EXPORT int expand(int *buf, int accept_min, int accept_max);
      /* [SYNOPSIS]
           Writes all unique indices which fall within the ranges defined in this
           set, while simultaneously lying within the interval
           [`accept_min',`accept_max'] into the supplied buffer, returning the
           number of written indices.  It is your responsibility to ensure that
           `buf' is large enough to accept all required indices, which can easily
           be done by ensuring at least that `buf' can hold
           `accept_max'+1-`accept_min' entries.
      */
  private: // Data
    friend struct kdu_window;
    int max_ranges; // Size of the `ranges' array
    int num_ranges; // Number of valid elements in the `ranges' array
    kdu_sampled_range *ranges;
  public: // Data
    kdu_range_set *next; // May be used to build linked lists of range sets
  };

/*****************************************************************************/
/*                               kdu_metareq                                 */
/*****************************************************************************/

struct kdu_metareq {
  /* [BIND: reference]
     [SYNOPSIS]
       This structure is used to describe metadata in which the client is
       interested.  It is used to build a linked list of descriptors which
       provide a complete description of the client's metadata interests,
       apart from any metadata which the server should deem to be mandatory
       for the client to correctly interpret requested imagery.  The order
       of elements in the list is immaterial to the list's interpretation.
  */
  //---------------------------------------------------------------------------
  public: // Data
    kdu_uint32 box_type;
      /* [SYNOPSIS] 
           JP2-family box 4 character code, or else 0 (wildcard).
      */
    int qualifier;
      /* [SYNOPSIS]
           Provides qualification of the conditions under which a box
           matching the `box_type' value should be considered to be of
           interest to the client.  This member takes the logical OR of
           one or more of the following flags: `KDU_MRQ_ALL', `KDU_MRQ_GLOBAL',
           `KDU_MRQ_STREAM' or `KDU_MRQ_WINDOW', corresponding to JPIP
           "metareq" request field qualifiers of "/a", "/g", "/s" and "/w",
           respectively.  The word should not be equal to 0.  Each of the flags
           represents a context, consisting of a collection of boxes which
           might be of interest.  The boxes which are of interest to the client
           are restricted to those boxes which belong to one of the contexts
           for which a flag is present.
           [>>] `KDU_MRQ_WINDOW' identifies a context which consists of all
                boxes which are known to be directly associated with a
                specific (non-global) spatial region of one or more
                codestreams, so long as the spatial region and other aspects
                of the metadata intersect with the requested view window.
           [>>] `KDU_MRQ_STREAM' identifies a context which consists of all
                boxes which are known to be associated with one or more
                codestreams or codestream contexts associated with the
                view window, but not with any specific spatial region of
                any codestream.
           [>>] `KDU_MRQ_GLOBAL' identifies a context which consists of all
                boxes which are relevant to the requested view window, but
                do not fall within either of the above two categories.
           [>>] `KDU_MRQ_ALL' identifies a context consisting of all JP2
                boxes in the entire target.
           [//]
           Note that the first three contexts mentioned above are mutually
           exclusive, yet their union is generally only a subset of the
           `KDU_MRQ_ALL' context.  The union of the first 3 flags is
           conveniently given by the macro, `KDU_MRQ_DEFAULT', since it is
           the default qualifier for JPIP `metareq' requests which do not
           explicitly identify a box context.
      */
    bool priority;
      /* [SYNOPSIS]
           True if the boxes described by this record are to have higher
           priority than image data when delivered by the server.  Otherwise,
           they have lower priority than the image data described by the
           request window.
      */
    int byte_limit;
      /* [SYNOPSIS]
           Indicates the maximum number of bytes from the contents of any
           JP2 box conforming to the new `kdu_metareq' record, which are
           of interest to the client.  May not be negative.  Guaranteed to
           be 0 if `recurse' is true.
      */
    bool recurse;
      /* [SYNOPSIS]
           Indicates that the client is also interested in all descendants
           (via sub-boxes) of any box which matches the `box_type' and
           other attributes specified by the various members of this structure.
           If true, the `byte_limit' member will be 0, meaning that the
           client is assumed to be interested only in the header of boxes
           which match the `box_type' and the descendants of such boxes.
      */
    kdu_long root_bin_id;
      /* [SYNOPSIS]           
           Indicates the metadata-bin identifier of the data-bin which is
           to be used as the root when processing this record.  If non-zero,
           the scope of the record is reduced to the set of JP2 boxes which
           are found in the indicated data-bin or are linked from it via
           placeholders.
      */
    int max_depth;
      /* [SYNOPSIS]
           Provides a limit on the maximum depth to which the server is
           expected to descend within the metadata tree to find JP2 boxes
           conforming to the other attributes of this record.  A value of 0
           means that the client is interested only in boxes which are found
           at the top level of the data-bin identified by `root_bin_id'.
           A value of 1 means that the client is additionally interested
           in sub-boxes of boxes which are found at the top level of the
           data-bin identified by `root_bin_id'; and so forth.
      */
    kdu_metareq *next;
      /* [SYNOPSIS]
           Link to the next element of the list, or else NULL.
      */
    int dynamic_depth;
      /* [SYNOPSIS]
           The interpretation of this member is not formally defined here.  It
           is provided to allow metadata analyzers to save temporary state
           information.  In the current implementation of `kdu_serve', this
           member is negative until the `root_bin_id' data-bin is encountered
           while traversing the metadata hierarchy.  At that point,
           `dynamic_depth' is set to the tree depth down to which this
           metadata request applies -- i.e., `max_depth' + tree_depth, where
           tree_depth is the depth in the tree associated with top-level boxes
           found within the box whose contents have been replaced by the
           data-bin.
      */
  //---------------------------------------------------------------------------
  public: // Member functions
    bool operator==(const kdu_metareq &rhs) const
      { /* [SYNOPSIS] Returns true only if all members are identical to those
           of `rhs'. */
        return (box_type == rhs.box_type) && (priority == rhs.priority) &&
               (qualifier == rhs.qualifier) &&
               (byte_limit == rhs.byte_limit) && (recurse == rhs.recurse) &&
               (root_bin_id == rhs.root_bin_id) &&
               (max_depth == rhs.max_depth);
      }
    bool equals(const kdu_metareq &rhs) const
      { return (*this == rhs); }
      /* [SYNOPSIS] Same as `operator==', returning true only if all members
         of this structure are identical to those of `rhs'. */
    kdu_uint32 get_box_type() const
      { return box_type; }
      /* [SYNOPSIS] Returns the value of the public `box_type' member.
         Useful for non-native language bindings such as Java. */
    int get_qualifier() const
      { return qualifier; }
      /* [SYNOPSIS] Returns the value of the public `qualifier' member.
         Useful for non-native language bindings such as Java.  Note that
         the return value is a union of one or more of the flags:
         `KDU_MRQ_ALL', `KDU_MRQ_GLOBAL', `KDU_MRQ_STREAM' and
         `KDU_MRQ_WINDOW'. */
    bool get_priority() const
      { return priority; }
      /* [SYNOPSIS] Returns the value of the public `priority' member.
         Useful for non-native language bindings such as Java. */
    int get_byte_limit() const
      { return byte_limit; }
      /* [SYNOPSIS] Returns the value of the public `byte_limit' member.
         Useful for non-native language bindings such as Java. */
    bool get_recurse() const
      { return recurse; }
      /* [SYNOPSIS] Returns the value of the public `recurse' member.
         Useful for non-native language bindings such as Java. */
    kdu_long get_root_bin_id() const
      { return root_bin_id; }
      /* [SYNOPSIS] Returns the value of the public `root_bin_id' member.
         Useful for non-native language bindings such as Java. */
    int get_max_depth() const
      { return max_depth; }
      /* [SYNOPSIS] Returns the value of the public `max_depth' member.
         Useful for non-native language bindings such as Java. */
    kdu_metareq *get_next() const
      { return next; }
      /* [SYNOPSIS] Returns the value of the public `next' member.
         Useful for non-native language bindings such as Java. */
  };

/*****************************************************************************/
/*                                kdu_window                                 */
/*****************************************************************************/

struct kdu_window {
  /* [BIND: reference]
     [SYNOPSIS]
       This structure defines the elements which can be used to identify
       a spatial region of interest, an image resolution of interest and
       image components of interest, in a manner which does not depend
       upon the coordinate system used by any particular JPEG2000
       code-stream.  Together, we refer to these parameters as identifying
       a "window" into the full image.
  */
  //---------------------------------------------------------------------------
  public: // Member functions
    KDU_AUX_EXPORT kdu_window();
    kdu_window(const kdu_window &src)
      { assert(0); }
      // For the moment, it is simplest to just prevent the use of copy
      // constructors.
    kdu_window &operator=(const kdu_window &rhs)
      { copy_from(rhs); return *this; }
      /* [SYNOPSIS]
           Simply invokes `copy_from' with the `copy_expansions' option set
           to false.
      */
    KDU_AUX_EXPORT ~kdu_window();
    KDU_AUX_EXPORT void init();
      /* [SYNOPSIS]
           Sets all dimensions to zero.  Sets the `max_layers' value to 0
           (default value, meaning no restriction).  Sets the collections
           of components and codestreams to empty.  Sets `round_direction'
           to -1 (round-down is the default).  Sets the `metareq' list to
           empty (NULL).
      */
    KDU_AUX_EXPORT bool is_empty() const;
      /* [SYNOPSIS]
           Returns true if `init' has just been called, or any changes made
           to the object since the last call to `init' should have no impact
           on the response expected from a JPIP server.  Any non-trivial value
           installed for the `resolution', `region', `components',
           `codestreams', `contexts' or `max_layers' members, or any `metareq'
           entries which have been added, will cause this function to return
           false.
      */
    KDU_AUX_EXPORT void copy_from(const kdu_window &src,
                                  bool copy_expansions=false);
      /* [SYNOPSIS]
           Copies the window represented by `src'.  If `copy_expansions' is
           true, the context expansions (see `kdu_sampled_range::expansion')
           within the `contexts' range set are also copied.  Otherwise,
           expansions are not copied, and `kdu_sampled_range::expansion' is
           set to NULL for each copied element of the `contexts' object.
      */
    KDU_AUX_EXPORT void copy_metareq_from(const kdu_window &src);
      /* [SYNOPSIS]
           Similar to `copy_from' but copies only the metadata request
           information.  Specifically, this function removes any existing
           `metareq' list from the current object, copies the `src.metareq'
           list and copies the `src.metadata_only' flag.
      */
    bool metareq_contains(const kdu_window &rhs) const
      {
        kdu_metareq *rq1, *rq2;
        for (rq1=rhs.metareq; rq1 != NULL; rq1=rq1->next)
          {
            for (rq2=metareq; rq2 != NULL; rq2=rq2->next)
              if (*rq2 == *rq1) break;
            if (rq2 == NULL)
              return false;
          }
        return true;
      }
      /* [SYNOPSIS]
           Compares the `metareq' lists associated with the current and
           `rhs' objects, returning true if every metadata request in `rhs'
           matches one in the current object.
      */
    KDU_AUX_EXPORT bool imagery_contains(const kdu_window &rhs) const;
      /* [SYNOPSIS]
           Returns true if `rhs' describes a window whose image window
           geometry, image components, image resolution, image region,
           code-streams, code-stream contexts and quality layers are all
           contained within the window described by the current object.
           Does not check expansions of code-stream contexts (see
           `kdu_sampled_range::expansion'), since the expansion for each
           range in the `contexts' set should be unique.
           [//]
           Note that `metareq' lists are deliberately excluded from the
           containment test, since they may be separately investigated using
           `metareq_contains'.  However, the function does consider the
           `metadata_only' member.  In particular, if `metadata_only'
           is true, the current object is not considered to contain the `rhs'
           object if `rhs.metadata_only' is false.
      */
    bool contains(const kdu_window &rhs) const
      { return (imagery_contains(rhs) && metareq_contains(rhs)); }
      /* [SYNOPSIS]
           Returns true if all aspects of the `rhs' window are contained
           within the current one.  This means that `imagery_contains' and
           `metareq_contains' must both return true when invoked on `rhs'.
      */
    KDU_AUX_EXPORT bool imagery_equals(const kdu_window &rhs) const;
      /* [SYNOPSIS]
           Returns true if `rhs' represents exactly the same window as the
           current object, with respect to imagery aspects of the
           window-of-interest.  The `metareq' lists are deliberately ignored,
           but the `metadata_only' member must agree between the current
           object and `rhs'.
      */
    bool equals(const kdu_window &rhs) const
      { 
        return (imagery_equals(rhs) && metareq_contains(rhs) &&
                rhs.contains(*this));
      }
      /* [SYNOPSIS]
           Returns true if `rhs' represents exactly the same window
           as the current object.  Does not check expansions (see
           `kdu_sampled_range::expansion'), since the expansion for each
           range in the `contexts' set should be unique.
      */
  //---------------------------------------------------------------------------
  public: // Data and interfaces for function-only bindings
    kdu_coords resolution;
      /* [SYNOPSIS]
           Describes the preferred image resolution in terms of the width and
           the height of the full image.  In the simplest case where no
           geometric mapping is required, the request is serviced by
           discarding r highest resolution levels, where r is chosen so that
           `resolution.x' and `resolution.y' are as close as possible to
           ceil(Xsiz/2^r)-ceil(Xoff/2^r) and ceil(Ysiz/2^r)-ceil(Yoff/2^r),
           where the sense of "close" is determined by the `round_direction'
           member.  Xsiz and Ysiz, Xoff and Yoff are the dimensions recorded
           in the codestream's SIZ marker segment.
           [//]
           Where geometric mapping information is provided via codestream
           `contexts', additional transformations are applied to match the
           resolution of each codestream to the resolution of the
           composited result which is identified by the present member.
           [//]
           If both `resolution.x' and `resolution.y' are 0, no compressed
           image data is being requested.  Such a request requires the server
           to send only the essential marker segments from main
           codestream headers, plus any metadata which is required for correct
           rendering.  Such a request might be sent by a client when it first
           connects to the server, but would not normally be issued
           explicitly at the application levels.
      */
    kdu_coords get_resolution() const
      { return resolution; }
      /* [SYNOPSIS] Returns a copy of the public `resolution' member.
         Useful for non-native language bindings such as Java. */
    void set_resolution(kdu_coords resolution)
      { this->resolution=resolution; }
      /* [SYNOPSIS] Sets the public `resolution' member.
         Useful for non-native language bindings such as Java. */
  //---------------------------------------------------------------------------
    int round_direction;
      /* [SYNOPSIS]
           Indicates the preferred direction for converting the `resolution'
           size to the size of an actual available image resolution in
           each codestream.  The available resolutions for each codestream
           are found by starting with the image size on the codestream's
           high resolution canvas, and dividing down by multiples of 2,
           following the usual rounding conventions of the JPEG2000
           canvas coordinate system.  Thus, the available image dimensions
           correspond essentially to the codestream image dimensions obtained
           by discarding a whole number of highest resolution levels,
           except that the number of discarded DWT levels is not necessarily
           limited by the number of DWT levels used in each relevant
           tile component.
           [>>] A value of 0 means that rounding is to the nearest available
                codestream image resolution, in the sense of area.
                Specifically, the actual codestream image resolution's
                dimensions, X and Y, should minimize |XY - xy| where x and y
                are the values of `resolution.x' and `resolution.y'.
           [>>] A positive value means that the requested resolution should
                be rounded up to the nearest actual available codestream
                resolution.  That is, the actual selected dimensions, X and Y,
                must satisfy X >= `resolution.x' and Y >= `resolution.y',
                if possible.
           [>>] A negative value means that the requested resolution should
                be rounded down to the nearest actual available codestream
                resolution.  That is, the actual selected dimensions, X and Y,
                must satisfy X <= `resolution.x' and Y <= `resolution.y', if
                possible.  Note: this is the default value set by `init'.
      */
    int get_round_direction() const
      { return round_direction; }
      /* [SYNOPSIS] Returns the value of the public `round_direction' member.
         Useful for non-native language bindings such as Java. */
    void set_round_direction(int direction)
      { round_direction = direction; }
      /* [SYNOPSIS] Sets the value of the public `round_direction' member.
         Useful for non-native language bindings such as Java. */
  //---------------------------------------------------------------------------
    kdu_dims region;
      /* [SYNOPSIS]
           Describes a spatial region, expressed relative to the image
           resolution defined by the `resolution' member.
      */
    kdu_dims get_region() const { return region; }
      /* [SYNOPSIS] Returns a copy of the public `region' member.
         Useful for non-native language bindings such as Java. */
    void set_region(kdu_dims region) { this->region=region; }
      /* [SYNOPSIS] Sets the public `region' member.
         Useful for non-native language bindings such as Java. */
  //---------------------------------------------------------------------------
    kdu_range_set components;
      /* [SYNOPSIS]
           Set of image components which belong to the window.  If this member
           is empty (`components.is_empty' returns true), the window includes
           all possible image components.  You should avoid adding
           ranges to a `components' member whose `kdu_sampled_range::step'
           member is anything other than 1.
      */
    kdu_range_set *access_components()
      { return &components; }
      /* [SYNOPSIS]
           Returns a pointer (reference) to the public `components' member.
           This is useful for non-native language bindings such as Java.
      */
  //---------------------------------------------------------------------------
    kdu_range_set codestreams;
      /* [SYNOPSIS]
           Set of codestreams which belong to the window.  If this member
           is empty, no particular restriction is imposed on the components
           being accessed.  If no information is provided by any other
           members (e.g., `compositing_layers') from which codestreams
           of interest may be deduced, the default behaviour depends on the
           type of image being served.
      */
    kdu_range_set *access_codestreams()
      { return &codestreams; }
      /* [SYNOPSIS]
           Returns a pointer (reference) to the public `codestreams' member.
           This is useful for non-native language bindings such as Java.
      */
  //---------------------------------------------------------------------------
    kdu_range_set contexts;
      /* [SYNOPSIS]
           This member is used to identify a collection of codestream
           contexts which a server is expected to expand into
           a collection of codestreams.  Currently, there are only two
           defined context types: JPX compositing layers; and MJ2
           tracks.  Accordingly, each `kdu_sampled_range' element in the
           `contexts' object must have a non-zero
           `kdu_sampled_range::context_type' member, which must match one of
           the macros `KDU_JPIP_CONTEXT_JPXL' or `KDU_JPIP_CONTEXT_MJ2T'.
      */
    kdu_range_set *access_contexts()
      { return &contexts; }
      /* [SYNOPSIS]
           Returns a pointer (reference) to the public `contexts'
           member.  This is useful for non-native language bindings such
           as Java.
      */
    KDU_AUX_EXPORT kdu_range_set *create_context_expansion(int which);
      /* [SYNOPSIS]
           This function creates a new internally managed `kdu_range_set'
           object and associates it with a particular `kdu_sampled_range'
           element within the `contexts' object.  That element is the one
           with index `which'.  It may be recovered by invoking
           `contexts.get_range' with `which' as the argument.  Once the
           function returns, the caller may add the indices of the codestreams
           associated with the relevant contexts using `kdu_range_set::add'.
      */
    kdu_range_set *access_context_expansion(int which)
      { kdu_sampled_range *rg = contexts.access_range(which);
        return (rg == NULL)?NULL:rg->expansion; }
    KDU_AUX_EXPORT const char *parse_context(const char *string);
      /* [SYNOPSIS]
           Parses a single JPIP context-range specifier from the supplied
           string, returning a pointer to any unparsed segment of the
           string.  Since JPIP "context" request fields and response headers
           may contain multiple context-range expressions, separated by
           commas (for requests) or semi-colons (for response headers), the
           function will often return a pointer to one of these separators.
           The caller decides how to invoke this function again where a
           list of context-range expressions is available.  If an error
           occurs, the function will return with a pointer to the character
           at which the error was detected, adding nothing to the internal
           `contexts' range set.  If an unrecognized context string is
           encountered, the function skips over it, returning a pointer to
           the character immediately following the unrecognized context-range
           expression.  The rules for JPIP context-range expressions, allows
           the function to skip unrecognized expressions effectively.
           [//]
           The context-range expression which is parsed, may contain a
           codestream expansion (separated by "=" in the string).  If so,
           the expansion is processed and recorded via the
           `create_context_expansion' function.
           [//]
           Upon successful completion, a recognized context-range expression
           will result in exactly one range being appended to the end of the
           `contexts' member (without merging or reordering), and possibly
           an associated expansion being generated.
           [//]
           JPIP "context" requests may contain non-URI-legal characters.  This
           function expects that these characters appear in raw form, not
           hex-hex encoded.  It is the caller's responsibility to make sure
           that any required hex-hex decoding has been performed.  In general,
           it is advisable to hex-hex decode each request field from a JPIP
           query string separately.  The `kdu_hex_hex_decode' function is
           well suited to this task, providing in-place piecewise hex-hex
           decoding.
      */
  //---------------------------------------------------------------------------
    int max_layers;
      /* [SYNOPSIS]
           Identifies the maximum number of code-stream quality layers
           which are to be considered as belonging to the window.  If this is
           zero, or any value larger than the actual number of layers in
           the code-stream, the window is considered to embody all available
           quality layers.
      */
    int get_max_layers() const
      { return max_layers; }
      /* [SYNOPSIS] Returns the value of the public `max_layers' member.
         Useful for non-native language bindings such as Java. */
    void set_max_layers(int val)
      { max_layers = val; }
      /* [SYNOPSIS] Sets the value of the public `max_layers' member.
         Useful for non-native language bindings such as Java. */
  //---------------------------------------------------------------------------
    bool metadata_only;
      /* [SYNOPSIS]
           If this flag is true, the client is not interested in receiving
           anything other than the metadata which would be returned in
           response to the requested window.  No compressed imagery, or
           even image headers should be sent.  Note that setting this member
           to true may have no effect unless a non-empty `metareq' list is
           also provided (e.g. by calls to `add_metareq').
      */
    bool get_metadata_only() const
      { return metadata_only; }
      /* [SYNOPSIS] Returns the value of the public `metadata_only' member.
         Useful for non-native language bindings such as Java. */
    void set_metadata_only(bool val)
      { metadata_only = val; }
      /* [SYNOPSIS] Sets the value of the public `metadata_only' member.
         Useful for non-native language bindings such as Java.  Note that
         the `init_metareq' and `parse_metareq' function also set the
         value of the `metadata_only' member.  In particular, `init_metareq'
         sets it to false, while `parse_metareq' sets it to true if a
         "!!" token is encountered in the parsed text. */
    kdu_metareq *metareq;
      /* [SYNOPSIS]
           Points to a linked list of `kdu_metareq' objects.  If NULL, the
           client is interested in all metadata the server deems to be
           important, delivered in the order the server chooses.  If non-NULL
           only those items identified by this list are of interest to the
           client, unless items are omitted which the server deems to be
           essential for the client to correctly interpret the requested
           imagery.
           [//]
           Do not add to this list manually.  Instead, use the `add_metareq'
           function to build a list of `kdu_metareq' objects.  Note that
           the `init' function resets the `metareq' list to empty (NULL).
      */
    kdu_metareq *get_metareq(int index) const
      { kdu_metareq *req=metareq;
        for (; (req != NULL) && (index > 0); req=req->next, index--);
        return req;
      }
      /* [SYNOPSIS]
           Returns NULL if the number of entries on the `metareq' list is
           equal to or less than `index'.  Otherwise, the function returns
           the particular entry on the `metareq' list which is indexed by
           the `index' value.  Useful for non-native language bindings such
           as Java.
      */
    void init_metareq()
      { kdu_metareq *req;
        while ((req=metareq) != NULL)
          { metareq=req->next; req->next=free_metareqs; free_metareqs=req; }
        metadata_only = false;
        have_metareq_all = have_metareq_global =
          have_metareq_stream = have_metareq_window = false;
      }
      /* [SYNOPSIS]
           Initializes the `metareq' list to the empty state and sets
           the `metadata_only' member to false.  This function
           is automatically called by `init', but it can be useful to
           initialize only the `metareq' list and nothing else.
      */
    KDU_AUX_EXPORT void
      add_metareq(kdu_uint32 box_type, int qualifier=KDU_MRQ_DEFAULT,
                  bool priority=false, int byte_limit=INT_MAX,
                  bool recurse=false, kdu_long root_bin_id=0,
                  int max_depth=INT_MAX)
      {
        if ((byte_limit < 0) || recurse) byte_limit = 0;
        if (root_bin_id < 0) root_bin_id = 0;
        if (max_depth < 0) max_depth = 0;
        kdu_metareq *req = free_metareqs;
        if (req == NULL) req = new kdu_metareq;
        else free_metareqs = req->next;
        req->next = metareq;  metareq = req;
        req->box_type = box_type; req->qualifier = qualifier;
        req->priority = priority; req->byte_limit = byte_limit;
        req->recurse = recurse; req->root_bin_id = root_bin_id;
        req->max_depth = max_depth;
        if (qualifier & KDU_MRQ_ALL) have_metareq_all = true;
        if (qualifier & KDU_MRQ_GLOBAL) have_metareq_global = true;
        if (qualifier & KDU_MRQ_STREAM) have_metareq_stream = true;
        if (qualifier & KDU_MRQ_WINDOW) have_metareq_window = true;        
      }
      /* [SYNOPSIS]
           Use this function to add a new entry to the `metareq' list.
           The order of elements in the list is not important.  Each element
           in the list describes a particular `box_type' of interest.  This
           should either be a valid JP2-family box signature (4 character
           code) or 0; in the latter case, all box types are implied.  The
           interpretations of the other arguments are described below.
         [ARG: box_type]
           JP2-family box 4 character code, or else 0 (wildcard).
         [ARG: qualifier]
           Must be a union of one or more of the flags, `KDU_MRQ_ALL',
           `KDU_MRQ_GLOBAL', `KDU_MRQ_STREAM' and `KDU_MRQ_WINDOW'.  At least
           one of these flags must be present.  The default value,
           `KDU_MRQ_DEFAULT' is equivalent to the union of `KDU_MRQ_GLOBAL',
           `KDU_MRQ_STREAM' and `KDU_MRQ_WINDOW'.  For a detailed explanation
           of the role played by these qualifiers, see the comments appearing
           with the `kdu_metareq::qualifier' member.
         [ARG: byte_limit]
           Indicates the maximum number of bytes from the contents of any
           JP2 box conforming to the new `kdu_metareq' record, which are
           of interest to the client.  May not be negative.  Note that
           this argument is ignored if `recurse' is true.
         [ARG: priority]
           True if the boxes described by the new `kdu_metareq' structure
           are to be assigned higher priority than image data when the
           server determines the order in which to transmit information which
           is relevant to the window request.  If false, these boxes will
           have lower priority than image data (unless the priority is
           boosted by another record in the `metareq' list).  If no
           image data is to be returned (see `metadata_only'), the `priority'
           value may still have the effect of causing some high priority
           metadata to be sent before low priority metadata.
         [ARG: recurse]
           Indicates that the client is also interested in all descendants
           (via sub-boxes) of any box which matches the `box_type' and
           other attributes specified by the various arguments.  If true,
           the `byte_limit' argument will be ignored, but treated as 0,
           meaning that the client is assumed to be interested only in the
           header of boxes which match the `box_type' and the descendants
           of such boxes.
         [ARG: root_bin_id]
           Indicates the metadata-bin identifier of the data-bin which is
           to be used as the root when processing the new `kdu_metareq'
           record.  If non-zero, the scope of the record is reduced to
           the set of JP2 boxes which are found in the indicated data-bin
           or are linked from it via placeholders.
         [ARG: max_depth]
           Provides a limit on the maximum depth to which the server is
           expected to descend within the metadata tree to find JP2 boxes
           conforming to the other attributes of this record.  A value of 0
           means that the client is interested only in boxes which are found
           at the top level of the data-bin identified by `root_bin_id'.
           A value of 1 means that the client is additionally interested
           in sub-boxes of boxes which are found at the top level of the
           data-bin identified by `root_bin_id'; and so forth.
      */
    KDU_AUX_EXPORT const char *parse_metareq(const char *string);
      /* [SYNOPSIS]
           Parse the contents of a JPIP "metareq=" request field, adding
           elements to the `metareq' list as we go.  This function does not
           reset the `metareq' list before it starts; instead, it simply
           appends the results obtained by parsing the supplied `string' to
           any existing list.  Call `init_metareq' first if you wish to
           start with an empty list.
           [//]
           Note that this function will set `metadata_only' to true if it
           encounters the "!!" token.  If this token is encountered, it
           must appear at the end of the string.
           [//]
           JPIP "metareq" requests may contain non-URI-legal characters.  This
           function expects that these characters appear in raw form, not
           hex-hex encoded.  It is the caller's responsibility to make sure
           that any required hex-hex decoding has been performed.  In general,
           it is advisable to hex-hex decode each request field from a JPIP
           query string separately.  The `kdu_hex_hex_decode' function is
           well suited to this task, providing in-place piecewise hex-hex
           decoding.
           [//]
           Box-type codes within a "metareq" request field are extracted
           using the `kdu_parse_type_code' function, which translates octal
           expressions that may have been used to represent non-alphanumeric
           bytes in the type code.
         [RETURNS]
           NULL if the `string' is successfully parsed.  Otherwise, returns
           a pointer to the first point in the supplied `string' at which
           a parsing error occurred.
      */
    bool have_metareq_all;
      /* [SYNOPSIS]
           Provided to facilitate efficient searching, this member is true
           if any only if the `metareq' list contains at least one entry
           whose `kdu_metareq::qualifier' member contains the `KDU_MRQ_ALL'
           flag.
      */
    bool have_metareq_global;
      /* [SYNOPSIS]
           Provided to facilitate efficient searching, this member is true
           if any only if the `metareq' list contains at least one entry
           whose `kdu_metareq::qualifier' member contains the `KDU_MRQ_GLOBAL'
           flag.
      */
    bool have_metareq_stream;
      /* [SYNOPSIS]
           Provided to facilitate efficient searching, this member is true
           if any only if the `metareq' list contains at least one entry
           whose `kdu_metareq::qualifier' member contains the `KDU_MRQ_STREAM'
           flag.
      */
    bool have_metareq_window;
      /* [SYNOPSIS]
           Provided to facilitate efficient searching, this member is true
           if any only if the `metareq' list contains at least one entry
           whose `kdu_metareq::qualifier' member contains the `KDU_MRQ_WINDOW'
           flag.
      */
  private:
    kdu_metareq *free_metareqs;
    kdu_range_set *expansions; // List of objects for use as context expansions
    kdu_range_set *last_used_expansion; // Points to last element of the above
                            // list to be allocated by `add_context_expansion'.
  };

/*****************************************************************************/
/*                            kdu_window_prefs                               */
/*****************************************************************************/

struct kdu_window_prefs {
  /* [BIND: reference]
     [SYNOPSIS]
       This structure describes a complete collection of "Related Pref sets"
       as defined by the JPIP standard (IS15444-9).  These are used to
       express client preferences to a server.  Preferences might no be
       respected by a server, but a client can identify one or more preferences
       as mandatory, forcing the server to generate an error response if it
       cannot support the preference.  The support offered by Kakadu's server
       components is likely to expand as time goes by, but the present object
       represents the complete set of preferences which a client can request.
       [//]
       The object describes client preferences through a set of three flag
       words, `preferred', `required' and `denied', together with auxiliary
       parameters which are needed to qualify some preferences.
       [//]
       The idea is that a JPIP client will communicate preferred
       (i.e., non-mandatory) and/or required (i.e., mandatory) preferences
       to the server and keep track of any required preferences which the
       server denies, so that it does not ask again.  Preferences are
       maintained within JPIP sessions, so there is no need to ask again
       within the same session, but a client may change its preferences from
       time to time.  By comparing an existing and new `kdu_window_pref'
       object, it is possible for a client to figure out what new information
       needs to be included in a JPIP "pref" request and to update the
       existing `kdu_window_pref' object based upon the replies received.
       [//]
       The name `kdu_window_pref' is used for this object, because the
       preferences which it represents affect the way in which a server
       responds to window-of-interest requests -- this are encapsulated by
       `kdu_window'.
  */
  //---------------------------------------------------------------------------
  public: // Member functions
    kdu_window_prefs()
      { csf_sensitivities=NULL; init(); }
    kdu_window_prefs(const kdu_window_prefs &src)
      { // Copy constructor to prevent accidental transfer of owned arrays
        csf_sensitivities=NULL; copy_from(src);
      }
    ~kdu_window_prefs() { init(); }
    kdu_window_prefs &operator=(const kdu_window_prefs &rhs)
      { copy_from(rhs); return *this; }
      /* [SYNOPSIS] Invokes `copy_from'. */
    KDU_AUX_EXPORT void init();
      /* [SYNOPSIS]
           Resets all flag bits in the `preferred', `required' and `denied'
           words to zero.  Also resets all auxiliary parameter values and
           deallocates any arrays.
      */
    void copy_from(const kdu_window_prefs &src)
      { init(); update(src); }
      /* [SYNOPSIS]
           Copies `src' to the current object in a safe way.
      */
    KDU_AUX_EXPORT int update(const kdu_window_prefs &src);
      /* [SYNOPSIS]
           This function replaces the entire contents of a "related-pref-set"
           with any information supplied for that same "related-pref-set" in
           `src'.  If `src' has nothing to say about some "related-pref-set",
           the corresponding information in the current object is left
           untouched.
           [//]
           The `denied' member is updated a little differently.  All
           flags in the current object's `denied' member are simply OR'd
           together with those in `src.denied' -- the idea is that if
           something has been denied once, it always will be.
         [RETURNS]
           The return value is a collection of flag bits which together
           identify the "related-pref-set"s which were updated by this
           function.  Specifically, if there is any change in the
           `preferred' or `required' preference flags, all flag bits for
           preference options which belong to the same "related-pref-set"
           are set within the returned integer.  You can use this information
           to determine whether any update occurred and, if so, what
           preferences might need to be communicated between a client and a
           server.
      */
    KDU_AUX_EXPORT const char *parse_prefs(const char *string);
      /* [SYNOPSIS]
           Parse the contents of a JPIP "pref=" request field, modifying the
           object's contents accordingly.  The object is automatically
           initialized before parsing, so this function cannot itself
           perform any update of an existence preference state record --
           that is the role of the `update' function.
           [//]
           Every successfully parsed preference modifies either the
           `preferred' or the `required' word, along with any relevant
           parameters for the related-pref-set in question.
         [RETURNS]
           NULL if the `string' is successfully parsed.  Otherwise, returns
           a pointer to the first point in the supplied `string' at which
           a parsing error occurred.
      */
    KDU_AUX_EXPORT int write_prefs(char buf[], int related_pref_sets=-1);
      /* [SYNOPSIS]
           Writes the body of a JPIP "pref=" request to the supplied buffer
           (unless it `buf' is NULL), returning the number of characters
           which were written (or would be written if `buf' were non-NULL),
           not including the null-terminator.  Note, it is possible that
           the written text may contain URI-non-legal characters (depending
           on how the "pref" field may or may not be extended in the future).
           As a result, you are advised to hex-hex encode the text produced
           by this function when writing HTTP GET statements -- the
           `kdu_hex_hex_encode' function is well suited to this purpose.
           [//]
           You can also use this function to write the body of a "JPIP-pref:"
           return header, by erasing `preferred' and moving `denied' to
           `required' before invoking the function.
         [ARG: related_pref_sets]
           This argument has the same interpretation as the return value
           from the `update' function.  Indeed, the primary intent is that
           that function's return value be passed in here, so that a client
           need only express preferences which have been changed.  This is
           because JPIP preferences are considered to be persistent within
           sessions.  The argument consists of a collection of flag bits.
           If the flag associated with any option in a "related-pref-set" is
           set, the preference information for all options in that
           "related-pref-set" are written to `buf' (unless it is NULL of
           course).  Setting this argument to -1 (the default) ensures that
           all preference information is written without exception.
      */
     KDU_AUX_EXPORT bool set_pref(int pref_flag, bool make_required=false);
       /* [SYNOPSIS]
            This function provides all the support you need for configuring
            simple preferences -- those which do not require auxiliary
            parameters.  It is much better to use this function than to
            directly manipulate the `preferred' and `required' members.
            The function takes care to ensure that mutually exclusive
            preferences are avoided.  For example, if `pref_flag' is equal to
            `KDU_WINDOW_PREF_FULL', and `KDU_WINDOW_PREF_PROGRESSIVE' flag will
            be cleared from the `preferred' and `required' members.  The
            function also ensures that each preference appears only in one of
            the `preferred' and `required' members.  If you specify
            `make_required'=true, the preference is added to the `required'
            member and cleared from the `preferred' member.  Similarly, if
            `make_required' is false, the preference is added to the
            `preferred' member and cleared from the `required' member.
            [//]
            As a general rule, you should try to avoid requiring preferences
            as JPIP servers are required to issue an error response if they
            do not support a required preference and this could adversely
            interfere with ongoing communication.  For this reason, the
            `make_required' argument default to false and is best left that
            way, except where you really know what you are doing.
          [RETURNS]
            The function returns false, without making any changes, if the
            `pref_flag' argument does not correspond to any known simple
            preference -- one which does not require parameters.
       */
     void set_max_bandwidth(kdu_long max_bw, bool make_required=false)
       { 
         int *set_flags = (make_required)?&required:&preferred;
         int *reset_flags = (make_required)?&preferred:&required;
         (*set_flags) |= KDU_MAX_BANDWIDTH_PREF;
         (*reset_flags) &= ~KDU_MAX_BANDWIDTH_PREF;
         max_bandwidth = max_bw;
       }
       /* [SYNOPSIS]
            Use this function to set up a global maximum bandwidth preference,
            rather than explicitly setting the object's data members.  The
            `max_bw' member is expressed in Bits per Second.  As with
            `set_pref', this function ensures that only one of `preferred' or
            `required' signals the preference. However, you are advised
            against making the preference required.
       */
     void set_bandwidth_slice(kdu_uint32 bw_slice, bool make_required=false)
       { 
         int *set_flags = (make_required)?&required:&preferred;
         int *reset_flags = (make_required)?&preferred:&required;
         (*set_flags) |= KDU_BANDWIDTH_SLICE_PREF;
         (*reset_flags) &= ~KDU_BANDWIDTH_SLICE_PREF;
         bandwidth_slice = bw_slice;
       }
       /* [SYNOPSIS]
            Use this function to set up a bandwidth slice preference,
            rather than explicitly setting the object's data members.  The
            `bw_slice' member identifies a relative slice of the total
            available bandwidth to be assigned to a specific JPIP channel.
            As with `set_pref', this function ensures that only one of
            `preferred' or `required' signals the preference. However, you
            are advised against making the preference required.
        */
     int get_colour_description_priority(jp2_colour_space space,
                                         int prec, kdu_byte approx)
       { /* [SYNOPSIS]
              Implements the colour description prioritization algorithm
              described in conjunction with `colour_meth_pref_limits'.  If
              you have several colour descritions, you can supply the
              values returned by `jp2_colour::get_space',
              `jp2_colour::get_precedence' and
              `jp2_colour::get_approximation_level' to this function and
              select the colour description whose returned priority value
              is smallest.
         */
         int limit = colour_meth_pref_limits[0]; // Enumerated space
         if ((space == JP2_iccLUM_SPACE) || (space == JP2_iccRGB_SPACE))
           limit = colour_meth_pref_limits[1]; // Restricted ICC space
         else if (space == JP2_iccANY_SPACE)
           limit = colour_meth_pref_limits[2]; // Unresricted ICC space
         else if (space == JP2_vendor_SPACE)
           limit = colour_meth_pref_limits[3]; // Vendor specific space
         int priority = ((int) approx) - prec; // Unmodified priority
         if ((approx == 0) && (limit == 0))
           priority = 3000; // Not mentioned in request and unknown approx
         else if (limit == 0)
           priority += 2000; // Not mentioned, but known approx 
         else if (limit == 0)
           priority += 1000; // Mentioned, but unknown approx
         else if (((int) approx) <= limit)
           priority = 1 - prec;
         return priority;
       }
  //---------------------------------------------------------------------------
  public: // Data and interfaces for function-only bindings
    int preferred;
      /* [SYNOPSIS]
           This word contains a collection of flag bits, corresponding to
           each of the preference choices which can be specified by a
           client.  Some of these preference choices require additional
           parameters, which are recorded in the object's other members,
           but the additional parameters are relevant only if the corresponding
           flag bit is found.  There are three flag words maintained by
           this object: `preferred', `required' and `denied'.  Any given flag
           bit should be set in at most one of these words.
           [>>] If the flag bit is set in the `preferred' word, the
                associated server behaviour is being requested by the client
                as a "non-required" preference.
           [>>] If the flag bit is set in the `required' word, the associated
                server behaviour is being requested by the client as a
                "required" preference.  This is a potential point of confusion
                with JPIP, which is cleared up in Draft Ammendment 4.
                Requiring a preference does not necessarily ask for it more
                strongly (that would probably have no meaning anyway).
                Instead, it provides a means for the client to discover whether
                the server supports a preferred behaviour.  More specifically,
                if the server implements an algorithm to address the behaviour
                associated with a required preference, it can honour a required
                preference request; otherwise, it should issue an error in
                response to the preference request, along with a JPIP-pref
                response header.  Clients should exercise caution in
                requiring preferences, because unsupported preferences must
                be met with an error response and errors may possibly cause
                a server or client to terminate any ongoing communication
                session, depending upon the implementation.
           [>>] If the flag bit is set in the `denied' word, the associated
                behaviour has been requested by the client, but denied by
                the server through the issuance of an error response with
                meaningful "JPIP-pref" response headers.  Once a behaviour
                has been denied, it is unlikely that a future request for
                the same behaviour (as a required preference) will yield a
                better response, so clients should probably keep track of
                denied behaviours using the `denied' word.
           [//]
           The specific defined flag bits are explained below, in relation
           to the JPIP standard:
           [>>] `KDU_WINDOW_PREF_PROGRESSIVE' is a default behaviour.  It
                means that the server is free to reduce the requested
                window of interest, in size or resolution, as it sees fit,
                in order to avoid excessive resource consumption, while
                striving to deliver a quality progressive stream of data
                over the (possibly adjusted) window of interest.  Any changes
                the server makes must be communicated explicitly to the client
                via JPIP response headers, and such changes must implement
                a consistent policy, meaning that if the client issues
                a subsequent request for the modified window of interest, the
                server should not make any further adjustments.  If this
                preference is required by a client, the client wants to know
                whether the server provides an algorithm which attempts to
                sequence the requested contents across the entire serviced
                window of interest, such that the contents are served
                with some sort of uniform progression in quality -- interpreted
                in whatever form is most natural or realizable.
           [>>] `KDU_WINDOW_PREF_FULL' is mutually exclusive with
                `KDU_WINDOW_PREF_PROGRESSIVE'.  It means that the client would
                prefer to receive requested windows of interest in full,
                even if this means that the server has to deliver the contents
                in a sequence which is not quality progressive in order to
                minimize the memory it must dedicate to serving the client --
                this may result in a poor experience for interactive users,
                but may benefit off-line agents which are only interested in
                downloading the content of interest, in full.  It makes sense
                to require this preference if you need to know whether
                the server can give you everything.
           [>>] `KDU_CONCISENESSS_PREF_CONCISE' is mutually exclusive with
                `KDU_CONCISENESS_PREF_LOOSE'.  It means that the client would
                like to receive data relating only to the requested window
                of interest, meaning in particular that it does not want the
                server to anticipate the client's need for additional data,
                not explicitly requested. It makes very little sense for a
                client to require this behaviour -- remember that requiring
                a preference does not make a preference request any stronger;
                it only requires the server to generate an error if it does
                not provide an algorithm to implement the preference; and
                there is nothing specific to implement here.
           [>>] `KDU_CONCISENESS_PREF_LOOSE' is mutually exclusive with
                `KDU_CONCISENESS_PREF_CONCISE'.  It means that the client is
                happy to allow the server to provide additional data, deemed
                to be appropriate to the request, even if it is not
                specifically related to the request as stated -- this is
                something that a smart server might be able to do, for
                example.  It makes very little sense for a client to require
                this behaviour, because there is no well-defined algorithm
                whose support the client could expect to learn about by
                requiring the preference.  In fact, the only reasonable way
                to use this preference is to reverse any effect associated
                with a previous request for the `KDU_CONCISENESS_PREF_CONCISE'
                preference.
           [>>] `KDU_PLACEHOLDER_PREF_INCR' is a default behaviour.  It means
                that, in the event that a box has multiple representations
                induced by placeholders in the metadata, the client would
                prefer to receive incremental codestream data-bins, if
                available.  Failing that, the client would prefer to receive
                a streaming equivalent box, if available.  Otherwise, the
                server should send original contents.  It is worth noting that
                which representations are actually available for any given box
                in the metadata depends upon the way placeholders are used,
                which may well be the server's decision.  However, once a
                placeholder structure has been selected, the JPIP server is
                obliged to not alter it, when serving the same resource with
                the same unique JPIP target identifier.  For this reason, you
                should not interpret this preference as a request for the
                server to create layouts which involve incremental codestreams.
                The interpretation is that whenever the layout involves
                multiple representations (the client will discover this
                sooner or later), the ones the client prefers are incremental
                codestreams.  Requiring this preference is a bit strange, but
                it would mean that a client wants to know whether a server
                can deliver incremental codestreams whenever they are
                available -- it has no implications for whether incremental
                codestreams should be available in the logical target, nor
                should the determination of whether the preference is
                supported be based on whether or not incremental codestreams
                appear within a specific logical target.
           [>>] `KDU_PLACEHOLDER_PREF_EQUIV' is mutually exclusive with
                `KDU_PLACEHOLDER_PREF_INCR' and `KDU_PLACEHOLDER_PREF_ORIG'.
                It means that the client would prefer to receive a streaming
                equivalent box, if available.  Failing that, the client would
                prefer to receive the original box, if available.  If neither
                are available, the client must be content with incremental
                codestreams -- these are one of the most interesting and
                useful features of JPIP anyway.  Requiring this preference
                is not to be interpreted as requiring that the server deliver
                or dynamically consruct a logical target which contains
                streaming equivalent placeholders for boxes.  If this
                preference is required, the server should declare that the
                preference is not supported, issuing an error, unless it
                provides a means to return streaming equivalent boxes wherever
                they happen to be present in any logical target.
           [>>] `KDU_PLACEHOLDER_PREF_ORIG' is mutually exclusive with
                `KDU_PLACEHOLDER_PREF_INCR' and `KDU_PLACEHOLDER_PREF_EQUIV'.
                It means that the client would prefer to receive the original
                box, if available.  Failing that, the client would prefer to
                receive a streaming equivalent box, if available.  If this
                preference is required, the client wants to know that the
                server is capable of returning original box contents for all
                boxes in the file, regardless of whether they have other
                representations through placeholders.
           [>>] `KDU_CODESEQ_PREF_FWD' is mutually exclusive with
                `KDU_CODESEQ_PREF_BWD' and `KDU_CODESEQ_PREF_ILVD'.  It means
                that the client would like to receive data from multiple
                requested codestreams sequentially, in increasing order
                of codestream index -- this is probably the default when
                serving video.  If this preference is required, the client
                wants to know whether the server implementation is capable
                of deliverying codestreams one by one for all logical targets,
                not just one associated with the request in which the
                preference is requested.
           [>>] `KDU_CODESEQ_PREF_BWD' is mutually exclusive with
                `KDU_CODESEQ_PREF_FWD' and `KDU_CODESEQ_PREF_ILVD'.  It means
                that the client would like to receive data from multiple
                requested codestreams sequentially, in decreasing order
                of codestream index.  If this preference is required, the
                client wants to know whether the server implementation is
                capable of delivering codestreams one by one in reverse order,
                for all logical targets, not just the one associated with
                the request in which the preference is requested.
           [>>] `KDU_CODESEQ_PREF_ILVD'is mutually exclusive with
                `KDU_CODESEQ_PREF_FWD' and `KDU_CODESEQ_PREF_BWD'.  It means
                that the client would like to receive data from multiple
                requested codestreams in an interleaved fashion so that they
                all improve progressively in quality, in a sense determined
                by the server -- this is probably the default when serving
                codestreams which contribute to a single composited JPX frame.
                If this preference is required, the client wanst to know
                whether the server is capable of delivering requests for
                multiple codestreams in such a way that their representation
                is progressively transmitted in an approximately uniform way.
           [>>] `KDU_MAX_BANDWIDTH_PREF' indicates that the client would like
                the server to limit the total bandwidth associated with its
                response data.  Unlike all other preferences, this one applies
                to all JPIP channels which might be present in a JPIP session.
                Every other preference is stored (and potentially respected)
                separately for each JPIP channel, where the server is
                granting multiple channels.  The actual bandwidth limit is
                provided by the `max_bandwidth' member.  If this preference
                is required, the client wants to know if the server supports
                global bandwidth limiting.
           [>>] `KDU_BANDWIDTH_SLICE_PREF' indicates that the client would like
                the server to partition the bandwidth it assigns to different
                JPIP channels within the same session in a manner which is
                proportional to each channel's slice value.  Slice preferences
                may be sent to set each channel's slice, but if this is not
                done for one or more channels, the default slice is 1.  The
                slice value associated with this flag is given by the
                `bandwidth_slice' member.  If this preference is required,
                the client wants to know whether the server supports relative
                distribution of available bandwidth across channels,
                regardless of whether multiple channels are currently open --
                if the server does not even support multiple channels, it
                does not support bandwidth slicing, by definition, so it
                should issue an error in response to a client requiring
                this preference.
           [>>] `KDU_COLOUR_METH_PREF' refers to colour space preferences
                which are qualified by the `colour_meth_pref_limits' array,
                described below.  If this preference is required, it means
                that a client wants to know whether the server implements
                the algorithm for prioritizing the delivery of colour
                description boxes, regardless of whether the current logical
                target contains multiple alternate colour description boxes
                or not.
           [>>] `KDU_CONTRAST_SENSITIVITY_PREF' indicates that the client
                would prefer the server to deliver data in a manner which is
                tuned to the contrast sensitivity function described by the
                `num_csf_angles', `max_sensitivities_per_csf_angle' and
                `csf_sensitivities' members.  If this preference is required,
                it means that the client wants to know whether the server
                implements an algorithm that can utilize any supplied
                constrast sensitivity information, regardless of whether
                the delivery of the current view-window would be affected
                by such an algorithm.
      */
    int required;
      /* [SYNOPSIS]
           See `preferred' for an explanation.
      */
    int denied;
      /* [SYNOPSIS]
           See `preferred' for an explanation.
      */
    kdu_long max_bandwidth;
      /* [SYNOPSIS]
           Should be non-negative.  See the discussion of
           `KDU_MAX_BANDWIDTH_PREF' above under `preferred'.  Bandwidth
           is expressed in Bits per Second.  Values may be rounded to
           low precision multiples of 1kb/s, 1Mb/s, 1Gb/s or 1Tb/s when
           actually delivering this preference in a request to the server.
      */
    kdu_uint32 bandwidth_slice;
      /* [SYNOPSIS]
           Should be non-negative.  See the discussion of
           `KDU_BANDWIDTH_SLICE_PREF' above under `preferred'.
      */
    kdu_byte colour_meth_pref_limits[4];
      /* [SYNOPSIS]
           Meaningful only if the `KDU_COLOUR_METH_PREF' flag occurs in
           `preferred' or `required'.
           [//]
           The four members of this array are used to identify colour
           method preferences, corresponding to "enumerated" colour spaces,
           "restricted ICC colour spaces", "unrestricted ICC colour spaces"
           and "vendor specific colour spaces", in that order.  Each entry
           in the array holds a LIMIT value on the APPROX field for colour
           spaces encounered within the relevant class of colour spaces.
           Meaningful values for the LIMIT lie in the range 0 to 4 and 255.
           0 means that the corresponding class of colour spaces is omitted
           from the preferences request.  255 means that the corresponding
           class of Values was identified by a preference request with no
           explicit "limit" value -- the interpretation is the same as 4
           (or any larger number) in all other respects.
           [//]
           The recommended algorithm for the server to follow in deciding
           which of multiple colour spaces to send for any compositing layer
           of interest is as follows -- note, however that most servers will
           more than likely just send all colour spaces, regardless, and this
           is perfectly legal.
           [>>] The server should generate a PRIORITY_i value for each
                available colour space box (indexed by i) that is available
                for the compositing layer in question, choosing the colour
                space with lowest PRIORITY_i value to send in preference to
                colour spaces with higher PRIORITY_i value, excepting those
                colour spaces which the server happens to know the client
                cannot support (presumably communicated via a JPIP
                capabilities request field, although capabilities requests
                are based on the JPX feature box, which has lost most of its
                teeth due to unworkable definitions in the original version
                of IS15444-2).
           [>>] PRIORITY_i = APPROX_i - PREC_i if APPROX_i > LIMIT (for the
                relevant type of colour space).
           [>>] PRIORITY_i = 1 - PREC_i if APPROX_i <= LIMIT (for the
                relevant type of colour spaces).
           [>>] If APPROX_i is equal to 0, a large value (512) is added to
                the PRIORITY_i value calculated for the i'th colour
                description box so that no colour description box which
                does not offer approximation information will be selected
                in preference with one which is does, assuming its colour
                space class is mentioned in the preference request.
           [>>] If LIMIT=0, a large value (1024) is added to the PRIORITY_i
                values of all colour space boxes belonging to the
                corresponding colour space class (one that is not mentioned in
                the preference request) so that no colour description box
                belonging to an unmentioned class will be selected in
                preference to one belonging to a mentioned class.
           [//]
           Note that the above are signed quantities; the JPIP standard uses
           an offset of 256 to make them unsigned, but this only complicates
           the description.  Note also that high values for PREC_i correspond
           to more desirable colour descriptions.
           [//]
           Evidently, if LIMIT <= 1, all colour space boxes of the relevant
           type are assigned PRIORITY_i = APPROX_i - PREC_i, since no colour
           space is supposed to have APPROX_i smaller than 1.  Also, if
           LIMIT >= 4, all priorities are obtained using the second expression
           above.  In generaly, specifying a larger LIMIT value means that
           the client application is less concerned about the accuracy of
           the colour space representation, caring only about the PREC_i
           value (precedence).
           [//]
           Now unfortunately, although IS15444-2 states that APPROX values in
           JPX files should never be 0, declaring that all JPX file writers
           should have better understanding of the approximation levels of
           their colour space than JP2 file writers, this does is not
           practically workable.  A very common scenario is that a JPX file
           is written to encapsulate JP2 data, together with some important
           features of JPX, such as region-of-interest metadata, for example.
           In this case, it would be wrong for the creator of such content to
           modify the original APPROX value of the colour space from 0 to
           something else, since that would be sheer guesswork.  In our
           opinion, therefore, any server which processes colour method
           preferences should be able to cope with the APPROX_i=0 case.
           The most natural approach would be for the server to consider
           all colour space boxes with APPROX_i=0 to have lower priority
           than any colour space box with a non-zero APPROX_i value.
           [//]
           A client might choose to specify a larger LIMIT if it makes
           significant numerical approximations in its implementation of
           colour space transformations, so that it matters little how
           accurately the original colour space parameters were
           expressed.  Rendering tools based upon Kakadu's
           `jp2_colour_converter' class should probably specify 3 or 4 for
           the LIMIT, since this class makes a number of approximations in
           order to transform colour with reasonable throughput -- in the
           future, it is possible that more accurate variants will be offered.
           However, in practice, we do not currently recommend that clients
           bother posting any colour method preferences, since the server
           is unlikely to pay any attention to them.
      */
    int num_csf_angles;
      /* [SYNOPSIS]
           Meaningful only if the `KDU_CONTRAST_SENSITIVITY_PREF' flag occurs
           in `preferred' or `required'.  Indicates the number of angles
           for which sensitivity information is provided in the
           `csf_sensitivities' array.
      */
    int max_sensitivities_per_csf_angle;
      /* [SYNOPSIS]
           Meaningful only if the `KDU_CONTRAST_SENSITIVITY_PREF' flag occurs
           in `preferred' or `required'.  Indicates the maximum number of
           contrast sensitivity values recorded along any angle.  If some
           angle has fewer sensitivities, the corresponding entries in the
           `csf_sensitivities' array should be -ve.
      */
    float *csf_sensitivities;
      /* [SYNOPSIS]
           Meaningful only if the `KDU_CONTRAST_SENSITIVITY_PREF' flag occurs
           in `preferred' or `required'.  If non-NULL, this array is
           expected to hold `num_csf_angles' rows of
           (2+`max_sensitivities_per_csf_angle') entries, each, organized row
           by row.  Each row corresponds to a particular angle.  The first
           two entries in each row are the angle, expressed in degrees
           away from horizontal, and the sample density d, which should be
           a positive quantity smaller than 1.  The n'th sensitivity,
           starting from n=0, represents the sensitivity at radial frequency
           pi * d^n, where pi is the Nyquist frequency.  The first negative
           sensitivity value, if any, on any given row terminates the list
           of sensitivity values for that angle.
           [//]
           The server is free to interpolate contrast sensitivity values you
           provide in any manner it desires, to arrive at an effective
           modulation transfer function MTF(omega1,omega2) for the assumed
           viewer, where omega1 and omega2 reach pi at the horizontal and
           vertical Nyquist frequencies associated with the requested frame
           size.  What this means is that whatever resolution data the server
           chooses to send, the reconstructed image (unrestricted by any
           region of interest) is assumed to be scaled (up or down) so
           that it fits inside the requested frame size, and the MTF is
           expressed with respect to this (potentially) scaled image.
           Unlike the other preferences, this one might need to be
           altered regularly (if used at all), since an interactive user
           may frequently change the frame size (effectively to zoom in
           and out).
           [//]
           It may be that a server ignores all but one angle, for
           simplicity especially if the angles are non-uniformly spaced or
           have different sampling densities.
      */
  };

/*****************************************************************************/
/*                            kdu_window_model                               */
/*****************************************************************************/

class kdu_window_model {
  /* [BIND: reference]
     [SYNOPSIS]
     This object is used to manage cache model manipulation instructions,
     as passed to `kdu_serve::set_window'.  The object supports all
     possible cache model instructions that can be expressed within the
     JPIP standard (IS15444-9), while also allowing the server to defer
     its processing of those instructions in such a way as to minimize
     resource consumption.
     [//]
     JPIP allows for cache model instructions which can be unbounded in
     scope (e.g., large or even open ranges of codestreams, tiles, components,
     etc.); however, this type of instruction can be used only with stateless
     requests, which means that the instructions need only be applied to the
     content which is actually going to be served.  More generally, Kakadu's
     server implementation defers processing of all cache model instructions
     until the point where the relevant content is about to be served.  This
     allows late instantiation of the resources required to properly
     interpret the cache model instructions and also ensures that all possible
     styles of cache modeling instruction can be handled (and handled
     efficiently).  At the time of this writing, we are not aware of any
     other vendor who can handle the full range of instructions permitted
     by the standard.
     [//]
     For session-based requests, it is possible that some cache model
     instructions refer to content which is not actually associated with an
     imagery request.  However, these instructions are necessarily atomic
     (one codestream, one tile, one component, one resolution, one precinct).
     As the server visits content of interest to serve to the client, it
     processes atomic and non-atomic cache model instructions which are
     relevant to the content.  The present object automatically discards
     atomic instructions as they are accessed, so that once all content
     relevant to a window of interest has been visited, the server can
     complete any session-based cache modeling obligations simply by
     accessing and processing any residual atomic instructions (in most
     cases, there should be none).
     [//]
     In view of the above, the present object provides two ways to access
     cache model instructions: instructions which retrieve and discard a
     single atomic instruction; and functions which apply atomic and
     non-atomic instructions to a set of data-bins which are of interest to
     the caller, discarding the atomic ones.  With this introduction, it
     should be possible to understand the function interfaces provided
     below.
   */
  public: // Member functions
    kdu_window_model()
      { 
        contexts = add_context = free_contexts = NULL;
        free_instructions = meta_head = meta_tail = NULL;
        stateless = background_full = false;
      }
    ~kdu_window_model()
      { clear();
        kdwm_stream_context *sc;  kdwm_instruction *ip;
        while ((sc = free_contexts) != NULL)
          { free_contexts=sc->next; delete sc; }
        while ((ip = free_instructions) != NULL)
          { free_instructions=ip->next; delete ip; }
      }
    kdu_window_model(const kdu_window_model &src)
      { // Copy constructor to prevent accidental transfer of owned arrays
        contexts = add_context = free_contexts = NULL;
        meta_head = meta_tail = free_instructions = NULL;
        copy_from(src);
      }
    kdu_window_model &operator=(const kdu_window_model &rhs)
      { this->copy_from(rhs); return *this; }
      /* [SYNOPSIS] Invokes `copy_from'. */
    void copy_from(const kdu_window_model &src)
      { 
        clear();
        stateless=src.stateless; background_full=src.background_full;
        if (src.add_context != NULL)
          set_codestream_context(src.add_context->smin,src.add_context->smax);
        append(src);
      }
    KDU_AUX_EXPORT void clear();
      /* [SYNOPSIS]
           Removes any existing cache model instructions.  After this
           call, you must invoke the second form of `init' before any call to
           `add_instruction', so as to set up an initial codestream
           context for adding new cache model instructions.
      */
    void init(bool stateless)
      { 
        clear(); assert((contexts == NULL) && (meta_head == NULL));
        this->stateless=stateless; this->background_full=background_full;
      }
      /* [SYNOPSIS]
           Almost the same as calling `clear', except that the function
           also sets the object's `stateless' attribute.  Use this
           function if you wish to return the object to an empty state,
           correctly reporting its statelessness via `is_stateless', but
           you do not yet need to add any cache model instructions.  Use
           the second form of `init' if you need to add cache model
           instructions.  Either of the `init' functions may be called as
           often as you like.
      */
    void init(bool stateless, bool background_full, int default_stream_idx)
      { 
        clear(); assert((contexts == NULL) && (meta_head == NULL));
        this->stateless=stateless; this->background_full=background_full;
        set_codestream_context(default_stream_idx,default_stream_idx);
      }
      /* [SYNOPSIS]
           Prepare the object to receive cache model instructions via the
           `set_codestream_context' and `add_instruction' functions.  This
           function discards (actually, recycles) any existing cache
           model instructions.  The function also provides information which
           determines how the cache model instructions should be interpreted
           by subsequent calls to the various `get_...' functions.
         [ARG: stateless]
           If true, the cache model instructions found here are intended to
           be used with stateless processing of window of interest requests.
           This means that only those instructions which refer to content
           that will actually be served (in accordance with a prevailing
           window of interest) need be considered.  A server should delete
           any existing cache model contents before proceeding and after
           processing is complete.  Server cache models will generally be
           used for all stateless processing or for a single stateful
           session.
           [//]
           In the stateless mode, the `get_precinct_instructions' will always
           return 0, while the `get_header_instructions' and
           `get_meta_instructions' functions will return 0 unless invoked
           with specific stream/bin ID's.
         [ARG: background_full]
           If true, the object contains only subtractive cache model
           instructions, which are applied to a cache model which is
           effectively considered to identify all data-bins as fully cached
           prior to application of the model instructions.  This option can
           be used only in the `stateless' mode.  When used, the behaviour
           of the various `get_...' instructions is modified to automatically
           signal completed data-bins, before signalling any subtractive
           contributions.  Moreover, in this mode, all calls to
           `add_instruction' are considered to specify subtractive
           cache model instructions, regardless of whether the
           `KDU_WINDOW_MODEL_SUBTRACTIVE' flag is present or not.
         [ARG: default_stream_idx]
           This argument sets up the opening codestream context.  All cache
           model instructions added via `add_instruction' will be considered
           to apply to this codestream until a subsequent call to
           `set_codestream_context'.
      */
    bool is_stateless() const
      { return stateless; }
      /* [SYNOPSIS]
           Returns true if the cache model is to be used to process a stateless
           request.  This means that all pre-existing cache model content
           is to be discarded and only those cache model instructions which
           relate to requested content need be considered.
      */
    bool is_empty() const
      { 
        if (background_full || (meta_head != NULL))
          return false;
        for (kdwm_stream_context *sc=contexts; sc != NULL; sc=sc->next)
          if (sc->head != NULL)
            return false;
        return true;
      }
      /* [SYNOPSIS]
           Returns true if the object has no impact upon any cache model.
      */
    KDU_AUX_EXPORT void append(const kdu_window_model &src);
      /* [SYNOPSIS]
           Appends all cache model instructions from `src' to those
           found in the current object.
      */
    KDU_AUX_EXPORT void set_codestream_context(int stream_min, int stream_max);
      /* [SYNOPSIS]
           Use this function to set the range of codestreams (i.e., the
           codestream context) to which cache model instructions added
           using the `add_instruction' functions apply.  Prior to the first
           call to this function, all instructions added by `add_instruction'
           apply to the codestream identified by the second form of the
           `init' function's `default_stream_idx' argument.
           [//]
           If `stream_min' is negative, the codestream context spans all
           codestreams and `stream_max' is ignored.  Otherwise, `stream_max'
           must be at least as large as `stream_min'.  If
           `stream_min' < `stream_max' or `stream_min' < 0, cache model
           instructions added within the codestream context are not atomic.
      */
    KDU_AUX_EXPORT void add_instruction(int databin_class, kdu_long bin_id,
                                        int flags, int limit);
      /* [SYNOPSIS]
           Use this function to add an "explicit" cache model instruction to
           the internal record.  Explicit instructions identify the relevant
           data-bin by its absolute `bin_id', together with its
           `databin_class'.  You must call the second form of the `init'
           function before adding any cache model instructions.
         [ARG: databin_class]
           Should be one of `KDU_MAIN_HEADER_DATABIN',
           `KDU_TILE_HEADER_DATABIN', `KDU_PRECINCT_DATABIN',
           `KDU_TILE_DATABIN' or `KDU_META_DATABIN'.
         [ARG: bin_id]
           Unique identifier of the data-bin within its class.  All valid
           data-bin ID's are non-negative, but you can specify a negative
           value here, meaning ALL data-bins within the class (and current
           codestream context).  The -ve value is equivalent to "*" in
           the JPIP request syntax.  Note that instructions added using this
           function are atomic only if `bin_id' is non-negative and the
           codestream context consists of a single codestream (see
           `set_codestream_context').
         [ARG: flags]
           Logical OR of zero or more of the following options:
           [>>] `KDU_WINDOW_MODEL_LAYERS' -- if this flag is present, the
                `limit' argument is to be interpreted as a quality layer limit,
                as opposed to a byte limit.  This flag is ignored unless the
                `databin_class' is `KDU_PRECINCT_DATABIN'.
           [>>] `KDU_WINDOW_MODEL_SUBTRACTIVE' -- if this flag is present, the
                `limit' argument expresses an upper bound on the
                cache contents, as opposed to a lower bound.
         [ARG: limit]
           Either an inclusive upper bound on the number of quality layers or
           bytes for the indicated data-bin, or an inclusive lower bound,
           depending on the presence or absence of the
           `KDU_WINDOW_MODEL_LAYERS' and `KDU_WINDOW_MODEL_SUBTRACTIVE'
           flags.
           [//]
           For additive instructions (those without the
           `KDU_WINDOW_MODEL_SUBTRACTIVE') flag, a -ve `limit' value may be
           used to declare that the client's cache model is complete for
           the data-bin(s) in question.
      */
    KDU_AUX_EXPORT void
      add_instruction(int tmin, int tmax, int cmin, int cmax,
                      int rmin, int rmax, kdu_long pmin, kdu_long pmax,
                      int flags, int limit);
      /* [SYNOPSIS]
           Similar to the first form of the `add_instruction' function,
           for the case of an "implicit" cache model instructions.  Implicit
           cache model instructions identify precinct data-bins implicitly
           through their tile, component, resolution and position
           coordinates, as opposed to their absolute data-bin indices.
           Implicit cache model instructions apply only to precinct
           data-bins; moreover, the `limit' value, if any, applies only to
           the number of available quality layers (not the number of bytes).
           [//]
           An atomic cache model instruction must have `tmin'=`tmax',
           `cmin'=`cmax', `rmin'=`rmax' and `pmin'=`pmax' all non-negative,
           with a codestream context which consists of a single codestream
           only -- see `set_codestream_context'.
         [ARG: tmin]
           Together with `tmax', this argument identifies the range of tiles
           to which the instruction applies.  If `tmax' != `tmin' or
           `tmin' < 0, the instruction is not atomic.  If `tmin'
           is negative, the instruction applies to all tiles and `tmax' is
           ignored.
           [//]
           If 0 <= `tmin' < `tmax', the instruction refers to a rectangular
           block of tiles, whose upper left hand tile has tile number `tmin'
           and whose lower right hand tile has tile number `tmax'.  Note
           that this range does not generally include all tiles whose
           tile numbers lie in the range `tmin' to `tmax'.
         [ARG: tmax]
           Must be no smaller than `tmin'.  See `tmin' for more info.
         [ARG: cmin]
           Together with `cmax', this argument identifies the range of
           codestream image component indices to which the instruction applies.
           If `cmax' != `cmin' or `cmin' < 0, the instruction
           is not atomic.  If `cmin' is negative, the instruction applies to
           all image components and `cmax' is ignored.
         [ARG: cmax]
           Must be no smaller than `cmin'.  See `cmin' for more info.
         [ARG: rmin]
           Together with `rmax', this argument identifies the range of
           tile-component resolutions to which the instruction applies.
           `rmin'=0 corresponds to the lower resolution of the relevant
           tile-component(s) -- i.e., the LL band.  If `rmax' != `rmin' or
           `rmin' < 0, the instruction is not atomic.  If `rmin' is negative,
           the instruction applies to all resolutions of the relevant
           tile-component(s) and `rmax' is ignored.
         [ARG: rmax]
           Must be no smaller than `rmin'.  See `rmin' for more info.
         [ARG: pmin]
           Together with `pmax', this argument identifies the range of
           precincts to which the instruction applies.  If `pmax' != `pmin'
           or `pmin' < 0, the instruction is not atomic.  If `pmin' is
           negative, the instruction applies to all precincts of the
           relevant tile-component-resolution(s) and `pmax' is ignored.
           [//]
           Precinct indices here are expressed relative to the top-left
           precinct in the tile-component-resolution to which they belong,
           following a raster scan order, with the number of precincts on
           each row determined by the underlying codestream structure.  If
           0 <= `pmin' < `pmax', the instruction applies to a rectangular
           block of precints, with `pmin' identifying the top-left precinct
           in the block and `pmax' identifying the bottom-right precinct
           in the block.  This is the same principle which is used to
           interpret differing `tmin' and `tmax' values.
         [ARG: flags]
           This argument has exactly the same interpretation as in the
           first form of the `add_instruction' function, except that the
           `KDU_WINDOW_MODEL_LAYERS' flag is required if `limit' is to
           have any meaning -- this is because implicit cache model
           instructions may provide quality layer limits, but not byte limits.
         [ARG: limit]
           Either an inclusive upper bound on the number of quality layers
           for the indicated data-bin, or an inclusive lower bound, depending
           on the presence or absence of the `KDU_WINDOW_MODEL_SUBTRACTIVE'
           flag.
           [//]
           For additive instructions (those without the
           `KDU_WINDOW_MODEL_SUBTRACTIVE') flag, a -ve `limit' value may be
           used to declare that the client's cache model is complete for
           the data-bin(s) in question.
      */
    KDU_AUX_EXPORT int
      get_meta_instructions(kdu_long &bin_id, int buf[]);
      /* [SYNOPSIS]
           This instruction retrieves digested cache modeling instructions
           for exactly one meta data-bin.  If `bin_id' holds a non-negative
           value on entry, information is retrieved for that data-bin only.
           Otherwise, information is retrieved for any meta data-bin.
           In the first case, the information retrieved may come from atomic
           or non-atomic cache model instructions, while in the second case
           only atomic cache model instructions are considered.  In either
           case, matching atomic cache model instructions are discarded by
           the function so that they will not be matched again.
           [//]
           Note that in the stateless mode (see `init'), this function
           returns 0 unless `bin_id' holds a non-negative value on entry.
           That is, the function only returns information for explicitly
           identified meta data-bins.
         [RETURNS]
           0 if no matching cache model instructions were found; otherwise,
           the function returns the number of values written to `buf', which
           will never exceed 2.
         [ARG: buf]
           Array with at least two entries.  The first entry, P, provides
           positive statements about the state of the client cache for this
           data-bin, while the second entry, N, provides negative statements
           about the state of the client cache for this data-bin.  Together,
           these quantitites declare that the number of bytes B in the client
           cache for this data-bin satisfy P <= B < N.  Values of P < 0 mean
           that there the data-bin is complete in the client's cache (a bit
           like P = infinity).  If N = 0, there is no upper bound (a bit like
           N = infinity).  If the function returns less than 2, N is taken to
           be 0 (no upper bound), while if the function returns 0, P is taken
           to be 0 (no lower bound).  For convenience, the function always
           initializes `buf' with zeros, so that the caller can ignore the
           function's return value if desired.
      */
    KDU_AUX_EXPORT int get_first_atomic_stream();
      /* [SYNOPSIS]
           Returns the index of the codestream associated with the first
           as-yet unprocessed atomic cache model instruction.  If there are
           none, the function returns -1.
      */
    KDU_AUX_EXPORT int
      get_header_instructions(int stream_idx, int tnum, int buf[]);
      /* [SYNOPSIS]
           This function plays the same role for codestream header data-bins
           as `get_meta_instructions' does for meta data-bins.  The function
           searches for cache model instructions (atomic and non-atomic) which
           apply to the indicated code-stream main or tile header data-bin
           (main header if `tnum' is -ve), writing the results of such a
           search to `buf' and returning the number of written entries.
         [RETURNS]
           0 if no matching cache model instructions were found; otherwise,
           the function returns the number of values written to `buf', which
           will never exceed 2.
         [ARG: stream_idx]
           Identifies the codestream of interest; must be non-negative.
         [ARG: tnum]
           Used to specify a specific tile number (or -1 for the codestream
           main header).
         [ARG: buf]
           Array with at least 2 entries, to which data is written by the
           function.  The interpretation of these values (N and P) is
           identical to that described for `get_meta_instructions'.
      */
    KDU_AUX_EXPORT int
      get_header_instructions(int stream_idx, int *tnum, int buf[]);
      /* [SYNOPSIS]
           Same as first form of `get_header_instructions', except that this
           version of the function searches for the first atomic cache
           model instruction(s) which refer to main or tile header data-bins
           within the indicated codestream, returning the tile index (or -1 for
           a main header data-bin) via `tnum'.
           [//]
           The caller will typically use this function only to retrieve
           information about codestream header data-bins which are not directly
           relevant to a current window request, during session-based
           processing.
           [//]
           As with all the other `get_...' functions, matching atomic
           cache model instructions are automatically discarded after their
           information has been retrieved by this function.
           [//]
           Note that in the stateless mode (see `init'), this function
           always returns 0.  This is because the function is not used to
           retrieve information about explicitly identified data-bins.  That
           is the role of the first form of the function.
      */
    KDU_AUX_EXPORT int
      get_precinct_instructions(int stream_idx, int &tnum, int &cnum,
                                int &rnum, kdu_long &pnum, int buf[]);
      /* [SYNOPSIS]
           This function is similar to the second form of
           `get_header_instructions' in that it can only be used to
           fetch information expressed by atomic cache model instructions.
           The caller will typically use this function only to retrieve
           information about precinct data-bins which are not directly
           relevant to a current window request, during session-based
           processing.  The function always fills out values for `tnum',
           `cnum', `rnum' and `pnum' based on the atomic cache model
           instruction it finds, if any.  If an explicit cache model
           instruction is found, `tnum', `cnum' and `rnum' will be set to -1
           and `pnum' will be set to the relevant precinct's absolute
           data-bin ID.  On the other hand, if an implicit cache model
           instruction is found, `tnum', `cnum', `rnum' and `pnum' will all
           be set to non-negative values, identifying the tile, component,
           resolution and relative precinct number for the matching
           instruction -- the should generally be able to translate this
           information into an absolute precinct data-bin ID.
           [//]
           As with all the other `get_...' functions, matching atomic
           cache model instructions are automatically discarded after their
           information has been retrieved by this function.
           [//]
           Note that in the stateless mode (see `init'), this function
           always returns 0.  This is because the function is not used to
           retrieve information about explicitly identified data-bins.  That
           is the role of the `get_precinct_block' function.
         [RETURNS]
           0 if no matching cache model instructions were found; otherwise,
           the function returns the number of values written to `buf', which
           will never exceed 2.
         [ARG: stream_idx]
           Identifies the codestream of interest; must be non-negative.
         [ARG: tnum]
           Used to return the tile number found in matching implicit atomic
           cache model instructions, or -1 if the matching instruction is
           explicit.
         [ARG: cnum]
           Used to return the component index found in matching implicit atomic
           cache model instructions, or -1 if the matching instruction is
           explicit.
         [ARG: rnum]
           Used to return the resolution index found in matching implicit
           atomic cache model instructions, or -1 if the matching instruction
           is explicit.
         [ARG: pnum]
           Used to return the absolute precinct data-bin ID for matching
           explicit atomic cache model instructions, or the relative
           precinct number if the matching instruction is implicit.
         [ARG: buf]
           Array with at least 2 entries, to which data is written by the
           function.  The interpretation of these values is similar to that
           described for `get_meta_instructions', except that limits may
           refer to the number of bytes or the number of quality layers.
           [>>] The first value provides additive statements
                about the client's cache for the relevant data-bin (a -ve
                value means the cache is full, 0 means nothing, 2*P means
                there are at least P bytes, and 2*P+1 means there are at
                least P quality layers).
           [>>] The second value in each pair provides subtractive statements
                about the client's cache for the relevant data-bin (0 means
                nothing, 2*N means there are less than N bytes, and
                2*N+1 means there are less than N quality layers).  Note that
                values or 2 and 3 both mean that the cache is empty, but we
                will only use the value 2 for this.  Values of 1 and 3 will
                not occur.
      */
    KDU_AUX_EXPORT bool
      get_precinct_block(int stream_idx, int tnum, int cnum, int rnum,
                         int t_across, int p_across, kdu_long id_base,
                         kdu_long id_gap, kdu_dims region, int buf[]);
      /* [SYNOPSIS]
           This is the function which should be used to retrieve cache
           model instructions for precinct data-bins as they become
           active during the server's sweep through a window of interest.
           The function retrieves information provided by both atomic and
           non-atomic cache model instructions, discarding the atomic ones.
           The function retrieves information for an entire block of
           precincts within a single resolution, or a single tile-component,
           within a single code-stream, writing the information to the
           `buf' array, which holds 2 entries for each precinct in the block.
           These 2 entries are sufficient to identify the effects of additive
           and subtractive cache model instructions, which provide byte
           limits and/or quality layer limits.  The various arguments provide
           sufficient information for the function to interpret both explicit
           and implicit cache model instructions in a uniform manner.
         [RETURNS]
           True if any precinct cache model instructions matched the supplied
           block of precinct data-bins.
         [ARG: stream_idx]
           Identifies the code-stream of interest; must be non-negative.
         [ARG: tnum]
           Identifies the tile of interest; must be non-negative.
         [ARG: cnum]
           Identifies the image component of interest; must be non-negative.
         [ARG: rnum]
           Identifies the resolution of interest; must be non-negative.
         [ARG: t_across]
           Supplies the number of tiles across the width of the codestream
           (i.e., the number of tile columns).  Vertically adjacent tiles have
           tile numbers separated by `t_across'.  This value is required to
           correctly interpret tile ranges which may occur in implicit cache
           model instructions in the stateless mode.
         [ARG: p_across]
           Supplies the number of precincts across the width of the given
           tile-component-resolution (i.e., the number of precinct columns).
           Vertically adjacent precincts have data-bin ID's separated by
           `p_across'*`id_gap'.
         [ARG: id_base]
           Supplies the absolute precinct data-bin ID associated with the
           top-left precinct within the given tile-component-resolution.
         [ARG: id_gap]
           Supplies the separation between successive precinct data-bin ID's
           associated with the given tile-component-resolution.  In practice,
           this is a constant for all precincts within a code-stream, being
           equal to the number of codestream image components, multiplied by
           the number of tiles in the codestream.
         [ARG: region]
           Identifies the first precinct within the block, together with
           the width and height of the block.  Specifically, within the
           given tile-component-resolution, there are `region.pos.y' precincts
           above and `region.pos.x' precincts to the left of the top-left
           precinct in the block, while the block consists of
           `region.size.x' horizontally adjacent precincts by
           `region.size.y' vertically adjacent precincts.
         [ARG: buf]
           Array with 2 entries for each precinct, organized in raster scan
           order with successive rows of precincts separated by
           2*`region.size.x' array entries.  The interpretation of each pair
           of values is identical to that of the 2 values returned by
           `get_precinct_instructions'.  In particular:
           [>>] The first value in each pair provides additive statements
                about the client's cache for the relevant data-bin (a -ve
                value means the cache is full, 0 means nothing, 2*P means
                there are at least P bytes, and 2*P+1 means there are at
                least P quality layers).
           [>>] The second value in each pair provides subtractive statements
                about the client's cache for the relevant data-bin (0 means
                nothing, 2*N means there are less than N bytes, and
                2*N+1 means there are less than N quality layers).  Note that
                values or 2 and 3 both mean that the cache is empty, but
                will only use the value 2 for this.
           [//]
           It can happen that cache model statements for some precinct
           collectively provide information on the number of bytes and
           quality layers for a precinct.  To avoid ambiguity, we take the
           following steps.
           [>>] A subtractive instruction concerning bytes obliterates any
                additive instruction concerning layers.  Similarly, a
                subtractive instruction concerning layers obliterates any
                additive instruction concerning bytes.
           [>>] If there are two subtractive instructions, one concerning bytes
                and the other concerning layers, they are collapsed into the
                conservative "cache empty" statement (i.e., 2*N=2).
           [>>] If there are two additive instructions, one concerning bytes
                and the other concerning layers, neither of which is
                obliterated (because there are no subtractive instructions
                specific to layers or bytes), we retain the most recent
                additive instruction.
      */
  private: // Structures
      struct kdwm_instruction {
          kdu_byte atomic; // Precomputed "atomic" status
          kdu_byte subtractive; // Non-zero if `limit' is an upper bound
          kdu_byte absolute_bin_id; // If using `bin_id' and `databin_class'
          int limit; // Inclusive limit (precincts stored as twice limit + 1)
          union {
            int tmin;
            int databin_class;
          };
          int tmax;
          kdu_int16 cmin, cmax;
          kdu_int16 rmin, rmax;
          union {
            kdu_long bin_id;
            kdu_long pmin;
          };
          kdu_long pmax;
          kdwm_instruction *next;
      };
      struct kdwm_stream_context {
        public: // Functions
          bool is_atomic()
            { return ((smin==smax) && (smin >= 0)); }
        public: // Data
          int smin, smax; // Range of codestream ID's or -1
          kdwm_instruction *head, *tail; // In the same order they were added
          kdwm_stream_context *next;
      };
  private: // Data
    bool stateless;
    bool background_full;
    kdwm_stream_context *contexts; // Ordered by `smin'
    kdwm_stream_context *add_context; // Current context for `add_instruction'
    kdwm_instruction *meta_head, *meta_tail; // For instructions with no stream
    kdwm_instruction *free_instructions; // Recycling list
    kdwm_stream_context *free_contexts; // Recycling list
  };

/*****************************************************************************/
/* INLINE                   kdu_write_type_code                              */
/*****************************************************************************/

static inline const char *
  kdu_write_type_code(kdu_uint32 type_code, char buf[])
  { /* [SYNOPSIS]
         This function records `type_code' in `buf' as a four-character-code,
         with octal encoding of non-alphanumeric characters.  More
         specifically, for each of the four bytes in `type_code' (starting
         from the most significant byte), if the byte has the same numeric
         value as an alphanumeric ASCII character, it is written to `buf'
         directly as that character; if the byte has the value 0x20 (ASCII
         space), it is written as a "_" (this is because spaces are very
         common in JP2 box type codes); otherwise, it is written using an
         octal expression of the form "\ddd".  The function is used to
         record box type-codes when forming JPIP "metareq" expressions, but
         may have other applications.
       [RETURNS]
         The ASCII string representation of `type_code', following the
         substitution conventions outlined above.  The return value is actually
         just a pointer to `buf'.
       [ARG: buf]
         Must be able to hold at least 17 characters (allows for all 4 bytes
         to be octal encoded, plus the null terminator).
    */
    kdu_byte val; const char *result = buf;
    for (int i=4; i > 0; i--, type_code<<=8)
      if ((val=(kdu_byte)(type_code>>24)) == 0x20)
        *(buf++) = '_';
      else if (((val >= (kdu_byte)'A') && (val <= (kdu_byte)'Z')) ||
               ((val >= (kdu_byte)'a') && (val <= (kdu_byte)'z')) ||
               ((val >= (kdu_byte)'0') && (val <= (kdu_byte)'9')))
        *(buf++) = (char) val;
      else
        { *(buf++)='\\';            *(buf++)=(char)('0'+((val>>6)&3));
          *(buf++)=(char)('0'+((val>>3)&7));  *(buf++)=(char)('0'+(val&7)); }
    *buf = '\0';
    return result;
  }

/*****************************************************************************/
/* INLINE                   kdu_parse_type_code                              */
/*****************************************************************************/

static inline kdu_uint32
  kdu_parse_type_code(const char *string, int &num_chars)
  { /* [SYNOPSIS]
         This function parses a four byte type-code, which has been written
         following the conventions outlined for `kdu_write_type_code'.
         Specifically, "_" is converted to 0x20, octal expressions ("\ddd")
         are converted to the corresponding 8-bit numeric value, and all
         other characters are considered to directly represent a byte of the
         four-character-code.  The parsing process accepts more general
         strings than those which can be generated by `kdu_write_type_code'.
         In particular, it accepts type codes whose bytes may have been
         directly represented with non-alphanumeric characters, so long as
         this does not create ambiguity.
       [RETURNS]
         The parsed type-code.  The function never fails or throws exceptions,
         regardless of how the `string' is formatted.  If `string'
         terminates prematurely, zeros will be substituted for the missing
         bytes.  This leaves errors to be detected at the next point in
         the parsing of a larger string within which type-codes are embedded.
       [ARG: num_chars]
         Used to return the number of characters from `string' which were
         parsed to generate the type-code; does not include any null
         terminator.
    */
    char ch; const char *sp=string;  kdu_uint32 result=0;
    for (int i=4; i > 0; i--, sp++)
      {
        result <<= 8;
        if ((ch = *sp) == '_') result += 0x20;
        else if ((ch == '\\') && ((sp[1] >= '0') && (sp[1] < '4')) &&
                 ((sp[2] >= '0') && (sp[2] < '8')) &&
                 ((sp[3] >= '0') && (sp[3] < '8')))
          {
            result += (((kdu_uint32)(sp[1]-'0'))<<6) +
              (((kdu_uint32)(sp[2]-'0'))<<3) + ((kdu_uint32)(sp[3]-'0'));
            sp += 3;
          }
        else if (ch != '\0')
          result += (kdu_uint32) ch;
        else
          sp--; // Back up so we don't walk over the null terminator
      }
    num_chars = (int)(sp-string);
    return result;
  }

#endif // KDU_CLIENT_WINDOW_H
