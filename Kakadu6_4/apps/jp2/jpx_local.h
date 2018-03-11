/*****************************************************************************/
// File: jpx_local.h [scope = APPS/JP2]
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
   Defines local classes used by the internal JPX file format processing
machinery implemented in "jpx.cpp".  The internal object classes defined here
generally parallel the externally visible interface object classes defined
in the compressed-io header file, "jpx.h".  You should respect the local
nature of this header file and not include it directly from an application
or any other part of the Kakadu system (not in the APPS/JP2 scope).
******************************************************************************/
#ifndef JPX_LOCAL_H
#define JPX_LOCAL_H

#include <assert.h>
#include "jpx.h"
#include "jp2_shared.h"

// Defined here:
struct jx_instruction;
struct jx_frame;
struct jx_numlist;
struct jx_trapezoid;
struct jx_regions;
struct jx_scribble_converter;
struct jx_path_filler;
struct jx_metaref;
struct jx_crossref;
struct jx_metaread;
struct jx_metawrite;
struct jx_metanode;
struct jx_metagroup;
struct jx_roigroup;
struct jx_metaloc;
struct jx_metaloc_block;
class jx_metaloc_manager;
struct jx_meta_manager;
struct jx_frag;

class jx_fragment_list;
class jx_compatibility;
class jx_composition;
class jx_registration;
class jx_codestream_source;
class jx_layer_source;
class jx_source;
class jx_codestream_target;
class jx_layer_target;
class jx_target;


/*****************************************************************************/
/*                                  jx_frag                                  */
/*****************************************************************************/

struct jx_frag {
  kdu_long offset;
  kdu_long length;
  int url_idx;
};

/*****************************************************************************/
/*                              jx_fragment_list                             */
/*****************************************************************************/

class jx_fragment_list {
  public: // Member functions
    jx_fragment_list() { max_frags = 0; frags = NULL; reset(); }
    ~jx_fragment_list()
      { if (frags != NULL) delete[] frags; }
    void reset() { num_frags = 0; total_length = 0; }
    bool is_empty() { return (num_frags==0); }
    bool init(jp2_input_box *flst_box, bool allow_errors=true);
      /* Initializes the object from the information recorded in a fragment
         list box.  If `allow_errors' is false, the function simply returns
         false should any problems are encountered reading the fragment list
         box.  Otherwise and error is generated through `kdu_error'.  In any
         case, the function returns true if successful. */
    kdu_long get_link_region(kdu_long &len);
      /* If the fragments represented by this object represent a single
         region within the same file as the fragment list box itself, the
         function returns the location of the first byte in that region and
         sets `len' equal to the number of bytes in the region.  This is
         a useful pre-requisite for detecting links within the metadata
         structure.  Otherwise the function returns -1. */
    void set_link_region(kdu_long pos, kdu_long len)
      {
        if (frags == NULL) { max_frags = 1; frags = new jx_frag; }
        num_frags = 1; total_length = len;
        frags[0].offset = pos; frags[0].length=len; frags[0].url_idx = 0;
      }
      /* Sets the fragment list to contain a single fragment from the current
         file, starting at `pos' and covering `len' bytes.  Any pre-existing
         contents are erased. */
    void save_box(jp2_output_box *super_box);
      /* Writes the fragment list box within the supplied `super_box',
         even if it is empty. */
    void finalize(jp2_data_references data_references);
      /* Uses the supplied object to verify that all data references from
         fragments in the fragment list point to valid URL's. */
  private: // data
    friend class jpx_fragment_list;
    int max_frags;
    int num_frags;
    jx_frag *frags;
    kdu_long total_length;
  };

/*****************************************************************************/
/*                             jx_compatibility                              */
/*****************************************************************************/

class jx_compatibility {
  public: // Member functions
    jx_compatibility()
      {
        is_jp2 = false; is_jpxb_compatible = is_jp2_compatible = true;
        have_rreq_box = true; // By default, we will write one
        no_extensions = true; no_opacity = true; no_fragments = true;
        no_scaling = true; single_stream_layers = true;
        max_standard_features=num_standard_features=0; standard_features=NULL;
        max_vendor_features=num_vendor_features=0; vendor_features=NULL;
        for (int i=0; i < 8; i++)
          fully_understand_mask[i] = decode_completely_mask[i] = 0;
      }
    ~jx_compatibility()
      {
        if (standard_features != NULL) delete[] standard_features;
        if (vendor_features != NULL) delete[] vendor_features;
      }
    void copy_from(jx_compatibility *src);
    bool init_ftyp(jp2_input_box *ftyp_box);
      /* Parses the file-type box.  Returns false, if not compatible with
         either JP2 or JPX.  Resets `have_rreq_box' to false. */
    void init_rreq(jp2_input_box *rreq_box);
      /* Parse reader requirements box, setting `have_rreq_box' to true. */
    void set_not_jp2_compatible() { is_jp2_compatible = false; }
      /* Called while checking compatibility information prior to writing a
         JPX header, if any reason is found to believe that the file will
         not be JP2 compatible.  The file will, by default, be marked as
         JP2 compatible. */
    void set_not_jpxb_compatible() { is_jpxb_compatible = false; }
      /* Called while checking compatibility information prior to writing a
         JPX header, if any reason is found to believe that the file will
         not be compatible with JPX  baseline.  The file will, by default, be
         marked as JP2 compatible. */
    void have_non_part1_codestream() { no_extensions = false; }
    void add_standard_feature(kdu_uint16 feature_id, bool add_to_both=true);
      /* Used by `jpx_target' to add all features which it discovers
         automatically.  Unless a "fully understand" sub-expression for the
         feature already exists, the function adds a new sub-expression for
         just this feature, marking it as required to fully understand the
         file.  If `add_to_both' is true, the same thing is done for
         "decode completely" expressions. */
    void save_boxes(jx_target *owner);
      /* Writes the file-type and reader requirements boxes.  The
         `owner->open_top_box' function is used to open the necessary boxes. */

  private: // Internal structures
      struct jx_feature {
          jx_feature() { memset(this,0,sizeof(*this)); }
          kdu_uint16 feature_id;
          bool supported; // Feature supported by application
          kdu_uint32 fully_understand[8]; // Temporary holding area for writing
          kdu_uint32 decode_completely[8];// Temporary holding area for writing
          kdu_uint32 mask[8]; // Holds the sub-expression masks
        };
      struct jx_vendor_feature {
          jx_vendor_feature() { memset(this,0,sizeof(*this)); }
          kdu_byte uuid[16];
          bool supported; // Feature supported by application
          kdu_uint32 fully_understand[8]; // Temporary holding area for writing
          kdu_uint32 decode_completely[8];// Temporary holding area for writing
          kdu_uint32 mask[8]; // Holds the sub-expression masks
        };
  private: // Data
    friend class jpx_compatibility;
    bool is_jp2, is_jp2_compatible, is_jpxb_compatible, have_rreq_box;
    bool no_extensions, no_opacity, no_fragments, no_scaling;
    bool single_stream_layers;
    int max_standard_features;
    int num_standard_features;
    jx_feature *standard_features;
    int max_vendor_features;
    int num_vendor_features;
    kdu_uint32 fully_understand_mask[8]; // Mask of used sub-expression indices
    kdu_uint32 decode_completely_mask[8];// Mask of used sub-expression indices
    jx_vendor_feature *vendor_features;
    jp2_output_box out; // Stable resource for use with `open_top_box'
  };

/*****************************************************************************/
/*                              jx_instruction                               */
/*****************************************************************************/

struct jx_instruction {
  public: // Member functions
    jx_instruction()
      {
        layer_idx=increment=next_reuse=0; iset_idx = inum_idx = -1;
        visible=first_use=false; next=prev=NULL;
      }
  public: // Data
    int layer_idx;
    int increment; // Amount to add to `layer_idx' when repeating frame
    int next_reuse; // If non-zero, `increment' must be 0
    bool visible; // If false, the instruction serves only as a placeholder
    bool first_use; // Used by `save_box': marks first instruction to use layer
    int iset_idx; // Index of `inst' box from which instruction was parsed
    int inum_idx; // Index of the instruction within the `inst' box.
    kdu_dims source_dims;
    kdu_dims target_dims;
    jx_instruction *next; // Next instruction in doubly linked list
    jx_instruction *prev; // Previous instruction in doubly linked list
  };
  /* Notes:
       While reading a composition box, we first recover the `next_use' info
       and then use this to deduce `layer_idx' and `increment'.  While writing,
       we first create the `layer_idx' and `increment' fields and use this to
       deduce `next_use'. */

/*****************************************************************************/
/*                                 jx_frame                                  */
/*****************************************************************************/

struct jx_frame {
  public: // Member functions
    jx_frame()
      {
        duration=num_instructions=repeat_count=increment=0;
        persistent=false; head=tail=NULL;
        last_persistent_frame=next=prev=NULL;
      }
    ~jx_frame()
      { reset(); }
    void reset()
      {
        num_instructions = 0;
        while ((tail=head) != NULL)
          { head = tail->next; delete tail; }
      }
    jx_instruction *add_instruction(bool is_visible)
      {
        num_instructions++;
        if (tail == NULL)
          head = tail = new jx_instruction;
        else
          {
            tail->next = new jx_instruction;
            tail->next->prev = tail;
            tail = tail->next;
          }
        tail->visible = is_visible;
        return tail;
      }
  public: // Data
    int duration; // Milliseconds until next frame is played
    int repeat_count; // 0 if frame is played once
    int increment; // Common increment for all not-reused layers
    int num_instructions; // Number of instructions in this frame alone
    bool persistent; // If the last instruction in the frame is persistent
    jx_instruction *head, *tail; // Instructions for this frame alone
    jx_frame *last_persistent_frame; // Backward chain of persistent frames
    jx_frame *next; // Next frame in doubly linked list
    jx_frame *prev; // Previous frame in doubly linked list
  };

/*****************************************************************************/
/*                              jx_composition                               */
/*****************************************************************************/

class jx_composition {
  public: // Member functions
    jx_composition()
      {
        max_lookahead=last_frame_max_lookahead=0; loop_count=1;
        is_complete=false; head=tail=last_persistent_frame=NULL;
        num_parsed_iset_boxes = 0;
      }
    ~jx_composition()
      {
        while ((tail=head) != NULL)
          { head = tail->next; delete tail; }
      }
    void donate_composition_box(jp2_input_box &comp_box, jx_source *owner);
      /* Donates the composition box when it is encountered at the top
         level of a JPX data source, so that the internal machinery can
         take over the parsing of this box, which might not be commpleted
         until a later point (see `finish').  This function is called
         automatically from within `owner->parse_next_top_level_box'. */
    bool finish(jx_source *owner);
      /* If the object has not yet been completely parsed, this function
         attempts to parse it, calling `owner->parse_next_top_level_box'
         if necessary.  Returns true if the object has been completely
         parsed. */
    void finalize(jx_target *owner);
      /* You must call this before `save_box' and before
         `adjust_compatibility'.  This function expands all repeating
         frames out into non-repeated frames, out to the limit of the
         available set of compositing layers -- cannot be too unreasonable.
         It then introduces invisible layers as required to ensure that the
         expanded frames can be represented using only the `next_use'
         links.  When the frames are written by `save_box', we look for
         repeated instruction sets to collapse the representation back down
         again. */
    void adjust_compatibility(jx_compatibility *compatibility,
                              jx_target *owner);
      /* Called before writing a composition box, this function
         gives the object an opportunity to add to the list of features
         which will be included in the reader requirements box. */
    void save_box(jx_target *owner);
      /* Writes the composition box. */
  private: // Helper function
    void add_frame();
      /* Adds a new frame to the list, setting the `last_persistent_frame'
         pointers appropriately. */
    bool parse_instruction(bool have_target_pos, bool have_taret_size,
                           bool have_life_persist, bool have_source_region,
                           kdu_uint32 tick);
      /* Called from within `finish', this function parses a single
         instruction within the instruction set box, returing true if it
         found one and false if the instruction set box is complete.  The
         function draws its data from the `sub_in' member object, but does
         not close that box once it is complete.  The four flags reflect
         the information contained in the `ityp' field of the instruction
         set box. */
    void assign_layer_indices();
      /* This function is called from within `finish' once an initial set
         of frames has been created.  It uses the recorded `next_reuse'
         fields, walking through the sequence of frames and instructions
         to assign actual compositing layer indices to each instruction. */
    void remove_invisible_instructions();
      /* Called from within `finish' after `assign_layer_indices' has been
         called, this function removes all the instructions which are marked
         as invisible.  These were kept after parsing the composition box
         only so as to allow the layer indices to be properly recovered. */
  private: // Data
    friend class jpx_composition;
    bool is_complete; // If `finish' has previously returned true.
    jp2_output_box comp_out; // For use with `jx_target::open_top_box'
    jp2_input_box comp_in;
    jp2_input_box sub_in;
    int num_parsed_iset_boxes;
    int loop_count; // Global repeat count.
    kdu_coords size; // Size of compositing surface
    jx_frame *head, *tail; // Head and tail of frame list
    jx_frame *last_persistent_frame;
    int max_lookahead, last_frame_max_lookahead; // See below
  };
  /* Notes:
       `max_lookahead' and `last_frame_max_lookahead' are used only while
       parsing a `composition' box to
       determine when we can convert repeating instruction set boxes into
       repeating `jx_frame' objects.  It represents the distance between
       the most recently parsed instruction and the latest instruction to
       which any parsed instruction's `next_use' field refers. */

/*****************************************************************************/
/*                               jx_numlist                                  */
/*****************************************************************************/

struct jx_numlist {
  public: // Member functions
    jx_numlist() { memset(this,0,sizeof(*this)); }
    ~jx_numlist();
    void add_codestream(int idx); // Resizes the array as necessary
    void add_compositing_layer(int idx); // Resizes the array as necessary
    void write(jp2_output_box &box); // Write contents into the upon box.
  public: // Data
    int num_codestreams;
    int max_codestreams; // Max entries in `codestream_indices' array
    int single_codestream_idx;
    int *codestream_indices; // Points to `single_codestream_idx' if only one
    int num_compositing_layers;
    int max_compositing_layers; // Max entries in `compositing_layers' array
    int single_layer_idx;
    int *layer_indices; // Points to `single_layer_idx' if only one
    bool rendered_result;
  };

/*****************************************************************************/
/*                              jx_trapezoid                                 */
/*****************************************************************************/

struct jx_trapezoid {
  void init(kdu_coords l1, kdu_coords l2, kdu_coords r1, kdu_coords r2)
  { // left edge l1->l2 (l1 on top); right edge r1->r2 (r1 on top)
    left1=l1; left2=l2; right1=r1; right2=r2;
    min_y = (l1.y>r1.y)?l1.y:r1.y;
    max_y = (l2.y<r2.y)?l2.y:r2.y;
  }
  int min_y, max_y;
  kdu_coords left1, left2, right1, right2;
};

/*****************************************************************************/
/*                               jx_regions                                  */
/*****************************************************************************/

struct jx_regions {
  public: // Member functions
    jx_regions()
      { num_regions=max_regions=0; regions=NULL; max_width=0; }
    ~jx_regions()
      { if ((regions!=NULL) && (regions!=&bounding_region)) delete[] regions; }
    void set_num_regions(int num); // Resizes the arrays, as necessary
    void write(jp2_output_box &box);
      /* Write contents into the supplied open box. */
    bool read(jp2_input_box &box);
      /* Read open ROI description box, returning false if an error occurs. */
  private:
    static bool
      promote_roi_to_general_quadrilateral(jpx_roi &roi, kdu_dims inner_rect,
                                           int A, int B, int C, int D);
        /* Converts an existing rectangular region (`roi') into a general
           quadrilateral, based upon the supplied `inner_rect' (must be
           contained within `roi.region' and the four bit-fields A, B, C and D,
           whose interpretation is defined in the standard.  Returns false if
           an error occurs. */
    static int
      encode_general_quadrilateral(jpx_roi &roi, kdu_dims &inner_rect);
        /* Finds the `inner_rect' and the Rtyp code code required to
           represent the general quadrilateral in `roi', in addition to
           the quadrilateral's bounding box in `roi.region'.  Note that
           `roi.region' is assumed to have been correctly filled out to
           hold the tightest bounding box for the vertices in
           `roi.vertices'.  Moreover, `roi.vertices' are assumed to
           have the required sequence (clockwise) and order (top-most vertex
           first).  The function returns the complete Rtyp code. */
  public: // Data
    int num_regions;
    int max_regions;
    jpx_roi bounding_region;
    jpx_roi *regions; // Points to `bounding_region' if only one
    int max_width; // Width of widest region, from `jpx_roi::measure_axes'
  };

/*****************************************************************************/
/*                         jx_scribble_segment                               */
/*****************************************************************************/

struct jx_scribble_segment {
  public: // Member functions
    void set_associated_points(int seg_start, int seg_length)
      { 
        first_seg_point = seg_start % num_scribble_points;
        num_seg_points = seg_length;
      }
    const kdu_coords *get_point(int n)
      { 
        if ((n < 0) || (n >= num_seg_points)) return NULL;
        n += first_seg_point;
        while (n >= num_scribble_points) n -= num_scribble_points;
        return scribble_points + n;
      }
  public: // Data
    const kdu_coords *scribble_points; // Points back to owner's array
    int num_scribble_points; // Size of owner's scribble points array
    int first_seg_point; // Index into the `scribble_points' array
    int num_seg_points; // Number of scribble points in the segment
    bool is_line;
    bool is_ellipse;
    kdu_coords seg_start, seg_end;
    kdu_coords ellipse_centre, ellipse_size, ellipse_skew;
    jx_scribble_segment *next;
    jx_scribble_segment *prev;
  };
  /* Notes:
       This structure is used by `jx_scribble_converter' to represent an
     approximated scribble path.  Segments are connected either linearly
     or cyclically, via the `next' and `prev' members.  That is, `next' and
     `prev' form a doubly-linked list in which the first segment's `prev'
     member points to the last segment (if cyclic) or NULL, while the last
     segment' `next' member points to the first segment (if cyclic) or NULL.
        Each segment has an associated set of scribble points, which come from
     the shared array managed by the owning `jx_scribble_converter' object.
     For convenience, the location of this array is stored here as
     `scribble_points' and the number of available points is stored as
     `num_scribble_points'.  The points which are associated with the current
     segment have indices starting from `first_seg_point' and running
     consecutively for a total of `num_seg_points' points.  It is possible
     that these points have indices which wrap around back to the first entry
     in the `scribble_points' array, so the `get_point' function is provided
     for convenience.  You can be sure that each segment has a disjoint
     collection of associated points.
        A segment whose `is_line' and `is_ellipse' members are both false
     is being used to hold a range of scribble points which have not yet been
     fitted.  Otherwise, exactly one of `is_line' or `is_ellipse' is true.
        If `is_line' is true, `seg_start' and `seg_end' hold the end-points
     of the line segment which is approximating the associated scribble points.
     In the simplest case, `seg_start' and `seg_end' correspond to actual
     scribble points -- one of them will be associated with an adjacent
     segment, since scribble points are disjointly associated with segments.
     More generally, though, one or both of `seg_start' and `seg_end' may be
     modified so that the line segment is tangent to an adjacent elliptical
     segment.
        If `is_ellipse' is true, the ellipse is described by the
     `ellipse_centre', `ellipse_extent' and `ellipse_skew' parameters, which
     have the same interpretation as that used by the `jpx_roi::init_ellipse'
     function.  The `axis_extents' and `tan_theta' members provide an
     alternate interpretation of the ellipse, as returned by the second
     form of the `jpx_roi::get_ellipse' function.  Ellipses are obtained
     initially by fitting the set of associated scribble points -- an
     eigenvalue/eigenvector problem.  During this process, the set of
     associated scribble points is adjusted so as to obtain an ellipse which
     meets the various constraints.  Specifically, we grow the set of
     scribble points associated with the segment until all non-associated
     scribble points are found to be outside the ellipse.  We then assess the
     fitting constraints for the associated points and continue to extend
     the set of associated points until these fitting constraints are
     violated.  Once an ellipse is found, the set of associated points is
     trimmed back (if required) so as to ensure that the first and last
     associated points lie outside or on the ellipse boundary.  These first
     and last associated scribble points are projected to the ellipse and
     assigned to the `seg_start' and `seg_end' members.  Later, line
     segments can be connected to the ellipse at these end points.  However,
     for a smoother boundary, we can later explore further trimming away
     associated scribble points from the ellipse and projecting the first
     and last remaining scribble points to obtain new `seg_start' and
     `seg_end' values -- we can keep doing this until the attaching line
     segment becomes tangential to the ellipse (or nearly so).
        Note that an elliptical segment may not be adjacent to another
     elliptical segment, apart from itself -- this degenerate case
     corresponds to the entire scribble path being approximated by a single
     elliptical segment. */

/*****************************************************************************/
/*                        jx_scribble_converter                              */
/*****************************************************************************/

#define JXSC_MAX_VERTICES 512
struct jx_scribble_converter {
  public: // Member functions
    jx_scribble_converter()
      { num_scribble_points=num_boundary_vertices=0; segments=NULL; }
    ~jx_scribble_converter() {};
    void init(const kdu_coords *points, int num_points, bool want_fill);
      /* Initializes he `scribble_points' and `num_scribble_points' members
         and creates a single segment associated with all the points. */
    bool find_polygon_edges(kdu_long max_sq_dist);
      /* This function is invoked by `jpx_roi_editor::convert_scribble_path'
         to decompose segments of uncommitted sribble points into a piecewise
         linear approximation.  The following rules apply:
         1) The `seg_start' member of any segment must be identical to the
            `seg_last' member of a previous segment, if one exists.
         2) If there is no previous segment `seg_start' must be identical to
            the first associated scribble point.  Similarly, if there is no
            next segment, `seg_end' must be identical to the last associated
            scribble point.
         3) The squared Euclidean distance between a line segment and any
            of its associated scribble points may not exceed `max_sq_dist'.
         4) No line segment may intersect with any other segment, be it
            a line segment or an elliptical segment.
         5) No line segment may cross the original scribble path anywhere
            other than within the scrible points with which it is associated.
         The function returns false if the above conditions cannot be
         satisfied for some reason. */
    bool find_boundary_vertices();
      /* Fills out the `boundary_vertices' and `num_boundary_vertices' members
         based on the available `segments'.  This is mainly to facilitate
         access to the polygon portion of a region boundary by other objects
         so they don't have to process the segments directly.  Returns false
         if something goes wrong. */
  private: // Helper functions
    jx_scribble_segment *get_free_seg()
      { 
        jx_scribble_segment *result = free_segments;
        if (result != NULL)
          { free_segments=result->next; result->next=result->prev=NULL;
            result->is_line = result->is_ellipse = false; }
        return result;
      }
  public: // Data
    int num_scribble_points; // Used to store the original scribble points
    kdu_coords scribble_points[JX_ROI_SCRIBBLE_POINTS];
    jx_scribble_segment *segments;
    jx_scribble_segment *free_segments;
    jx_scribble_segment seg_store[512]; // Used to store the actual segments
    int num_boundary_vertices;
    kdu_coords boundary_vertices[513]; // See `find_boundary_vertices'.
  };

/*****************************************************************************/
/*                             jx_path_filler                                */
/*****************************************************************************/

#define JXPF_MAX_REGIONS 512
#define JXPF_MAX_VERTICES (4*JXPF_MAX_REGIONS)
#define JXPF_INTERNAL_EDGE JXPF_MAX_VERTICES

struct jx_path_filler {
  public: // Member functions
    jx_path_filler() { memset(this,0,sizeof(*this)); }
    bool init(jpx_roi_editor *editor, int num_path_members,
              kdu_byte *path_members, kdu_coords path_start);
      /* Initialize directly with info from `editor->enum_paths'.
         Note that only the quadrilateral regions are kept.  These are
         initially modified based on path segment information recovered via
         `editor->get_path_segment_for_region', so that all exterior edges
         are bounded by vertices.  The constructor assumes that the path
         is closed, so that it returns back to `path_start'.  Returns false
         if the path is not a closed loop, or if the path intersects itself,
         amongst other pathalogical conditions. */
    bool init(const kdu_coords *src_vertices, int num_src_vertices);
      /* This second form of the `init' function is used by the
         `jpx_roi_editor::convert_scribble_path' function to initialize
         the path filler with the vertices of a polygon.  The last vertex
         in the list must be identical to the first, or else the function
         will return false.  The function also returns false if the polygonal
         boundary intersects itself.  After this version of the `init'
         function, you would generally invoke `process' immediately.  The
         `original_path_members' array is reset to zero by this function
         and left there. */
    bool contains(const jx_path_filler *src) const;
      /* Returns true if the outer boundary of the polygon described by
         `src' is entirely contained within the inner boundary of the
         polygon described by the current object. */
    bool intersects(const jx_path_filler *src) const;
      /* Returns true if any of the path bricks associated with `src'
         intersect with the current object.  This function is usually of
         greatest interest when invoked immediately after the current
         object's `init' function. */
    void import_internal_boundary(const jx_path_filler *src);
      /* Imports all the regions from `src' into the current object,
         exchanging the roles played by external edges and unpaired internal
         edges.  The two paths should pass the `contains' test before this
         function is called. */
    void get_original_path_members(kdu_uint32 flags[]) const
      { for (int n=0; n < 8; n++) flags[n] |= original_path_members[n]; }
      /* `flags' is an 8-element array of bit-fields (256 bit-fields in all).
         The function sets bits which correspond to the original indices of
         the quadrilateral regions supplied to the constructor via the
         `path_members' argument. */
    bool process();
      /* Runs the various phases of the polygon filling algorithm. */
  private: // Helper functions
    int examine_path(const kdu_coords *src_vertices, int num_src_vertices);
      /* This function takes a list of vertices which run consecutively
         around a (supposedly) closed path, such that the last vertex is
         identical to the first vertex.  The function first checks that the
         path is indeed closed and that no edge intersects with any other
         edge.  If either of these conditions is violated, the function
         returns 0.  The function then determines whether the path is
         traversed clockwise or anti-clockwise, returning a +ve value for
         clockwise and -ve value for anti-clockwise.  Both of the `init'
         functions rely upon this function. */
    bool check_integrity();
      /* Useful debugging tool.  This function checks that all edges which
         reference other edges are referenced back by those same edges. */
    int count_internal_edges();
      /* Counts the number of internal edges which are not shared with any
         other region and have non-zero length.  The goal of the `process'
         function is to reduce this value to 0. */
    bool join();
      /* Attempts to share as many internal edges as possible -- once all
         internal edges are shared there are no holes inside the polygonal
         path being filled.  Makes only one pass through the regions to see
         what edges can be shared, returning true if anything was changed.
         If so, further sharing might still be possible. */
    bool simplify();
      /* Attempts to reduce the number of distinct quadrilateral regions
         by merging regions which are already sharing at least one edge.
         In order to do this, the common vertices may need to be adjusted,
         which is allowed so long as the edges of other quadrilaterals are
         not crossed in the process.  Makes only one pass through the regions
         to see what regions can be eliminated, returning true if anything
         was changed.  If so, further simplification might still be
         possible. */
    bool fill_interior();
      /* Attempts to reduce the number of interior edges by adding new
         regions.  Three connected interior edges might be reduced to 1 by
         adding an interior quadrilateral.  Similarly, two connected
         interior edges might be reduced to 1 by adding an interior triangle.
         The function adds at most one region, returning true if it does so,
         in which case further calls to this function might be able to
         further fill the interior.  However, after each successful call to
         this function, you should invoke `join'. */
    bool check_vertex_changes_for_edge(int edge_idx, const kdu_coords *v0,
                                       const kdu_coords *v1,
                                       int initial_edge_idx=-1) const;
      /* This recursive function evaluates whether or not the end-points of
         the edge corresponding to entry `edge_idx' in the `edges' array
         can legitimately be changed such that its end-points are moved
         to the coordinates found at `v0' and `v1'.  The supplied end-points
         are correctly ordered, within the region with index n=`edge_idx'>>2.
         That is, v0 holds the new value for vertex `edge_idx'-4*n and
         v1 holds the new value for vertex (`edge_idx-4*n+1) mod 4.  The
         function returns true immediately if v0 and v1 hold the same values
         which are currently found at these vertices.  Failing this, the
         function returns false if a vertex which must be moved is one of
         the end-points of an external edge, or if the move causes region
         n to be an invalid quadrilateral (i.e., opposite edges cross), or
         if the move causes any of the edges attached to the moved vertices
         to cross any external edge of the polygon at all.  If all the above
         tests succeed, the function recursively explores the impact on
         neighbouring regions which would be affected by the change through
         shared edges.  The function assumes that the line segment joining
         v0 to v1 already lies entirely within the polygon that is being
         filled, so this condition does not have to be checked -- saves
         redundant boundary scans.
            The `initial_edge_idx' argument should be -ve when the function
         is called from any place other than itself.  This signals that both
         sides of the edge itself may need to be checked.  Subsequent recursive
         calls from within itself, have `initial_edge_idx' set to the index
         of the first `edge_idx' value with which the function was called. */
    void apply_vertex_changes_for_edge(int edge_idx, const kdu_coords *v0,
                                       const kdu_coords *v1);
      /* This recursive function changes the end-points of the indicated edge
         to `v0' and `v1', respectively, following shared edges to other
         regions which may be sharing the vertices which are changing.  If
         the change causes any edge length to go to zero, the corresponding
         `region_edges' entry will be changed to point back to itself, so
         that triangle vertices are never treated as shared edges. */
    bool check_boundary_violation(const kdu_coords *v0,
                                  const kdu_coords *v1) const;
      /* Scans all the external edges of the polygon being filled, to check
         whether or not the line segment connecting `v0' to `v1' crosses one
         of these edges.  If so, the function returns true. */
    bool check_boundary_violation(const jpx_roi &roi) const;
      /* Scans all the external edges of the polygon being filled, to check
         whether or not any of them pass through or are contained within
         any of the edges of the quadrilateral supplied by `roi'. */
    bool is_region_triangular(int idx, int edges[]);
      /* This function returns false unless the region identified by `idx'
         represents a triangle, in which case it returns true, and sets the
         entries of the `edges' array to the indices of the three edges
         of the triangle, as found within the `region_edges' array.  These
         are drawn from 4*idx+p where p runs from 0 to 3, eliminating the
         edge whose length is zero.  The edges run clockwise around the
         triangle. */
    bool remove_degenerate_region(int idx);
      /* This function returns false without doing anything, unless the
         region identified by `idx' is degenerate, in one of the following
         senses: a) Two pairs of adjacent vertices are identical (a line);
         b) Opposite vertices (v0 and v2 or v1 and v3) are identical, meanng
         the edges fold back on themselves.  If either of these conditions
         hold, the region is removed and the function returns true.  Note
         that to do this properly, the function must reconnect edges around
         the removed degenerate region. */
    void remove_edge_references_to_region(int idx);
      /* Removes all references from other regions to any of the edges of
         region `idx'.  This allows the region to be repurposed or
         removed. */
    void remove_region(int idx);
      /* Removes the region identified by `idx', reconnecting the various
         array references around it.  It is an error to call this function
         if the region is still referenced by any other region, except
         possibly from an edge of zero length. */
    bool add_quadrilateral(int shared_edge1, int shared_edge2,
                           int shared_edge3);
      /* Tries to add a new quadrilateral region which will share existing
         internal edges identified by the two arguments.  These existing edges
         run clockwise around the outside of the new quadrilateral.  The
         function returns false if there is insufficient storage to add the
         new quadrilateral, if the quadrilateral formed by completing the
         fourth edge would be invalid (e.g., runs anti-clockwise, or has
         edges which cross), or if the quadrilateral would violate the
         boundary constraints -- see `check_boundary_violation'. */
    bool add_triangle(int shared_edge1, int shared_edge2);
      /* Tries to add a new triangular region which will share existing
         internal edges identified by the two arguments.  These existing edges
         run clockwise around the outside of the new triangle.  The
         function returns false if there is insufficient storage to add the
         new triangle, if the triangle formed by completing the third edge
         would be invalid (e.g., runs anti-clockwise, or has
         edges which cross), or if the triangle would violate the boundary
         constraints -- see `check_boundary_violation'. */
  public: // Data members used only by the first form of `init', which assumes
          // a maximum of 256 initial path bricks.
    kdu_uint32 original_path_members[8]; // 256 flag bits
    int num_tmp_path_vertices; // Used to store vertices of the path polygon
    kdu_coords tmp_path_vertices[257]; // for analysis by `examine_path'.
  public: // Data members used by both forms of `init' and the other functions
    int num_regions;
    kdu_coords region_vertices[JXPF_MAX_VERTICES]; // 4 vertices per region
    int region_edges[JXPF_MAX_VERTICES]; // See below
    jx_path_filler *container; // Used to determine holes for other paths
    jx_path_filler *next;
  };
  /* Notes:
        This object is used to implement the functionality of
     `jpx_roi_editor::fill_closed_paths', as well as
     `jpx_roi_editor::convert_scribble_path'.
        The `region_vertices' member keeps track of the 4 vertices associated
     with each region.  These appear at entries 4n through 4n+3, where n is
     the region index, in the range 0 to `num_regions'-1.
        The  `region_edges' array has one entry for each quadrilateral edge.
     Specifically edge e from region n corresponds to entry 4n+e where
     edge e runs between vertex e and vertex (e+1) mod 4.  The value of
     this entry is -ve if the edge is external to the polygon being filled.
     The value is JXPF_INTERNAL_EDGE if the edge is internal to the polygon
     being filled, but not shared with any other region.  Otherwise, the
     entry is equal to 4p+q where p and q are the index of another region
     and its edge, respectively. */

/*****************************************************************************/
/*                               jx_metaref                                  */
/*****************************************************************************/

struct jx_metaref {
  public: // Member functions
    jx_metaref() { src=NULL; i_param=0; addr_param=NULL; }
  public: // Data
    jp2_family_src *src; // Non-NULL for existing metadata in a JPX data source
    jp2_locator src_loc; // Used to identify location in a non-NULL `src'
    int i_param; // Integer parameter for delayed output box references
    void *addr_param; // Address parameter for delayed output box references
  };

/*****************************************************************************/
/*                              jx_crossref                                  */
/*****************************************************************************/

struct jx_crossref {
  public: // Member functions
    jx_crossref()
      { owner=NULL; box_type=0; metaloc=NULL;
        link=NULL; link_type=JPX_METANODE_LINK_NONE; next_link=NULL; }
    ~jx_crossref() { unlink(); }
    void append_to_list(jx_crossref *head)
      { /* Adds ourselves to the end of a list headed by `head'. */
        while (head->next_link != NULL) head = head->next_link;
        head->next_link = this;  this->next_link = NULL;
      }
    void link_found();
      /* Called once `metaloc' points to a valid target.  If the
         `link_type' member is already set (i.e., not `JPX_METANODE_LINK_NONE')
         it is not changed here.  Otherwise, `link_type' is discovered using
         the `JX_METANODE_FIRST_SUBBOX' flag of the owner and target nodes. */
    void unlink();
      /* Removes the structure from any list to which it belongs, headed
         (directly or indirectly) by `metaloc' or `link', and connected by
         `next_link'.  Leaves all of those members NULL.  This should have
         been done before the destructor is called. */
  public: // Data
    jx_metanode *owner;
    kdu_uint32 box_type;
    jx_fragment_list frag_list;
    jx_metaloc *metaloc; // Non-NULL only while waiting to resolve `link'
    jx_metanode *link;
    jpx_metanode_link_type link_type;
    jx_crossref *next_link; // For building a list of links to a metanode
  };
  /* Notes:
       When reading a cross-reference box, an instance of this structure is
     created to hold the fragment list and box-type.  If the cross-reference
     box has a box-type which is of interest to the meta-manager and its
     fragment list represents a single contiguous range of bytes from the
     same file, it might represent a link.  To investigate further,
     `jx_metaloc_manager::get_locator' is invoked with the location of the
     first byte in the fragment, and the `create_if_necessary' argument set
     to true.  The `metaloc' argument is set to the returned reference, which
     has one of the following forms:
     a) `metaloc' references the target of the link, in which case
        `link' is set equal to `metaloc->target' and `metaloc' is set to NULL.
     b) `metaloc' is newly allocated, in which case the `metaloc->target'
        is set to `owner'.
     c) `metaloc->target' points to the `owner' of a `jx_crossref' object
        whose `jx_crossref::link' member is NULL and whose
        `jx_crossref::metaloc' member is identical to `metaloc'.  This means
        that case (b) occurred previously, so we add ourselves to the list of
        nodes waiting for the true target to become available, building this
        list via the `next_link' member.
       Once the `link' member is non-NULL, the `metaloc' member is set to
     NULL and the `link' object's `linked_from' member will point to the
     head of a list of `jx_crossref' objects which contain links to it, the
     `next_link' member being used to build this list.  Should the `link'
     object be deleted during an editing operation, all the linking nodes
     are also deleted.
       When saving metadata to a file, only those cross-reference
     nodes which contain links can be saved; the others are skipped.  A two
     pass approach is generally required.  In the first pass, the locations
     and lengths of every box to be saved are determined and the
     `jx_metanode::linked_from' lists are traversed to record this
     information in the linking nodes' `frag_list' box as a single fragment.
     In the second pass, all the boxes (including links) can actually be
     written. */

/*****************************************************************************/
/*                              jx_metaread                                  */
/*****************************************************************************/

struct jx_metaread {
  public: // Member functions
    jx_metaread()
      { codestream_source=NULL; layer_source=NULL;
        asoc_databin_id=box_databin_id=-1;
        asoc_nesting_level=box_nesting_level=0; }
  public: // Data
    jp2_input_box asoc; // Used to manage an open asoc box.
    jp2_input_box box; // Used to manage a single box, or a sub-box of an asoc
    jx_codestream_source *codestream_source; // See below
    jx_layer_source *layer_source; // See below
    kdu_long asoc_databin_id; // -1 unless using a dynamic cache; see below
    kdu_long box_databin_id; // -1 unless using a dynamic cache; see below
    int asoc_nesting_level; // See below
    int box_nesting_level; // See below
  };
  /* Notes:
       The `codestream_source' or `layer_source' member may be non-NULL
     if the associated `jx_metanode' object was created while reading a
     codestream or compositing layer header box, to manage metadata found
     therein.  So long as either of these members is non-NULL, the
     `asoc' and `box' objects will be empty, and
     `jx_codestream_source::finish' and `jx_layer_source::finish' should
     be called whenever more metadata is requested by the application.
     Once all the imagery headers have been recovered from within a
     codestream header box, or a compositing layer header box, these
     pointers are reset to NULL, and the relevant header box is
     transplanted into `asoc', from which subsequent metadata boxes may
     be read.
       The last four members are used to provide additional information about
     any metadata-bin to which `asoc' and/or `box' belong.
     The `asoc_databin_id' and `asoc_nesting_level' members relate to the
     location of the header of the asoc box, if any.  This information is
     used by `jpx_metanode::generate_metareq' to generate data-bin-relative
     metadata requests.
        If there is no asoc box, `asoc_databin_id' will be -ve.  If the
     asoc box is found at the top level of its databin, it will have a
     nesting level of 0.  If it is found at the top level within the contents
     of a superbox, which is at the top level of the databin, the nesting
     level will be 1; and so forth.
        The `box_databin_id' is always non-negative if the data is sourced
     from a dynamic cache.  If `box' is (or will be) found at the top level
     of its databin, it will have a nesting level of 0; and so forth. */

/*****************************************************************************/
/*                              jx_metawrite                                 */
/*****************************************************************************/

struct jx_metawrite {
  public: // Functions
    jx_metawrite()
      {
        context=NULL; active_descendant=NULL;
        asoc_offset = 0; content_offset = 0; content_length = node_length = 0;
      }
  public: // Data
    jp2_output_box asoc; // Used to manage an open asoc box.
    jp2_output_box box; // Used to manage a single box, or a sub-box of an asoc
    jx_metagroup *context; // Context in which box was last marked for writing
    jx_metanode *active_descendant; // See description of `jx_metanode::write'
  public: // The following members are computed when boxes are closed
    int asoc_offset; // Header length of containing `asoc' box
    int content_offset; // Starting position of `box' contents relative to
          // the start of this node.  This is the sum of `asoc_offset' and the
          // `box' header length, where `asoc_offset' is 0 if `box' is not
          // the first sub-box of an asoc box.
    kdu_long content_length; // Num content bytes in `box'
    kdu_long node_length; // Includes descendants and box headers
  };
  
/*****************************************************************************/
/*                               jx_metanode                                 */
/*****************************************************************************/
// The following constants are used by the `jx_metanode::flags' member:
#define JX_METANODE_IS_COMPLETE       ((kdu_uint16) 0x0001)
  // Set once parent's `num_completed_descendants' updated
#define JX_METANODE_BOX_COMPLETE      ((kdu_uint16) 0x0002)
  // If the node's box is completely available (only for reading)
#define JX_METANODE_DESCENDANTS_KNOWN ((kdu_uint16) 0x0004)
  // If num descendants is known (only for reading)
#define JX_METANODE_EXISTING          ((kdu_uint16) 0x0008)
  // If the node is obtained by reading an existing box
#define JX_METANODE_WRITTEN           ((kdu_uint16) 0x0010)
  // If the node has already been written (and `write_state' deleted)
#define JX_METANODE_DELETED           ((kdu_uint16) 0x0020)
  // If the node has been moved to the deleted list
#define JX_METANODE_CONTENTS_CHANGED  ((kdu_uint16) 0x0040)
  // If a pre-existing node's contents have been changed
#define JX_METANODE_ANCESTOR_CHANGED    ((kdu_uint16) 0x0080)
  // If a pre-existing node's parent has been changed
#define JX_METANODE_FIRST_SUBBOX        ((kdu_uint16) 0x0100)
  // If this node is the first sub-box of an asoc box; used only when reading.
#define JX_METANODE_LOOP_DETECTION      ((kdu_uint16) 0x1000)
  // Used to detect loops within the `jx_meanode::find_path_to' function

struct jx_metanode {
  private:
    friend struct jx_meta_manager;
    ~jx_metanode(); // This function should be called only from the
                    // meta-manager's own destructor, while cleaning up
                    // the `deleted_nodes' list.  Nodes should first be
                    // moved onto this list from the metadata tree, using
                    // the `safe_delete' function.
  public: // Member functions
    jx_metanode(jx_meta_manager *manager)
      { memset(this,0,sizeof(*this)); this->manager = manager; }
    void safe_delete();
      /* This function is used to remove a node from the metadata tree,
         unlinking it from its parent and metagroup and recursively
         doing the same for all its children.  Rather than actually
         deleting the node, or its embedded data, however, the node is
         moved onto the meta-manager's list of deleted nodes.  This allows
         persistet references even after removal from the metadata tree. */
    void unlink_parent();
      /* Does most of the work of `safe_delete'.  Unlinks from parent, and
         updates the `num_descendants' and `num_completed_descendants'
         members. */
    void append_to_touched_list(bool recursive);
      /* This function is used to append a node to the manager's touched list,
         being careful to first remove it from the list, if it is already
         there.  If `recursive' is true, the function recursively appends
         the node's descendants also to the touched list.  The function also
         includes the JX_METANODE_ANCESTOR_CHANGED flag to the node if its
         parent has any of the change flags set. The function does nothing
         if the node does not have the JX_METANODE_BOX_COMPLETE flag set. */
    void place_on_touched_list();
      /* This function is used in place of `append_to_touched_list' when
         either of the two auxiliary conditions described under
         `jpx_meta_manager::get_touched_nodes' occurs -- these conditions
         relate to information about the immediate descendants of the node
         becoming available (number of children or availability of a child to
         `jpx_metanode::get_descendant').  An application may need to be
         informed of these changes where a dynamic cache is growing in
         unexpected ways, but it does not need to be guaranteed that nodes
         added to the touched list for either of these reasons alone will
         appear before any descendants on the touched list.  For this reason,
         the present function does not move the node if it is already on
         the touched list. */
    void append_child(jx_metanode *child);
      /* Does most of the work of `add_descendant'.  Updates the
         `num_descendants' and `num_completed_descendants' member. */
    jx_metanode *add_descendant();
      /* Adds a new node to the tail of this node's descendant list. */
    jx_metanode *
      add_numlist(int num_codestreams, const int *codestream_indices,
                  int num_compositing_layers, const int *layer_indices,
                  bool applies_to_rendered_result);
      /* Does most of the work of `jpx_metanode::add_numlist', but is
         also used to create a numlist node to hold metadata found in
         compositing layer header boxes while reading.  For this reason, the
         function does not set the `is_complete' member of the new box to
         true.  However, it does set `box_complete' to true. */
    void donate_input_box(jp2_input_box &src, int databin_nesting_level);
      /* This function may be called from `jx_source::parse_next_top_box' when
         a top-level metadata or association box is encountered in a JPX
         data source.  It may also be called from
         within `jx_codestream_source::finish' or `jx_layer_source::finish'
         when a metadata or association box is encountered inside a
         codestream or compositing layer header box.
            The `src' box need not be complete.  Its state is donated to the
         internal `asoc' or `box' member of a `jx_metaread' object which is
         created and assigned to the `read_state' member.  The function then
         attempts to parse the box and, if it is an assocation box, its
         various sub-boxes, by calling `finish_reading'.  In the process, new
         descendant nodes may be created resulting in recursive calls to
         the present function.
            The `databin_nesting_level' identifies the nesting level of the
         box within which `src' is found, relative to its databin, following
         the definition given for `jx_metaread::databin_nesting_level'.
         The value of this argument is irrelevant if this box is not sourced
         from a dynamic cache. */
    bool finish_reading();
      /* This function is first called from within `donate_input_box', but
         may need to be called again if the ultimate data source is fueled
         by a dynamic cache whose contents are not yet sufficiently complete
         to parse all required boxes.  The function continues to have an
         effect until both the `box_complete' and `descendants_known'
         members become true. */
    void update_completed_descendants();
      /* This function is called only while reading existing boxes from a
         JP2/JPX data source, once a parent node's `num_completed_descendants'
         member can be incremented.  This may cause that node's parent to
         to be adjusted as well in a recursive fashion. */
    void load_recursive();
      /* Recursively passes through the metadata tree, trying to finalize as
         many nodes as possible. */
    jx_metanode *find_path_to(jx_metanode *tgt, int descending_flags,
                              int ascending_flags, int num_exclusion_types,
                              const kdu_uint32 *exclusion_box_types,
                              const int *exclusion_flags, bool unify_groups);
      /* Recursive function to implement the `jpx_metanode::find_path_to'
         function for all but the simplest case in which we ignore
         links between nodes in the metadata graph. */
    bool check_path_exclusion(int num_exclusion_types,
                              const kdu_uint32 *exclusion_box_types,
                              const int *exclusion_flags);
      /* This function is used by `find_path_to' to check exclusion conditions.
         Returns true if the current node is to be excluded. */
    void clear_write_state(bool clear_frag_lists);
      /* Recursively passes through the node and all its descendants, resetting
         the `JX_METANODE_WRITTEN' flag and removing any `write_state' which
         might exist for some reason.  This is done at the start of each write
         phase visited by the `jpx_target::write_metadata' function.  There
         may be up to two write phases if a simulation phase is required to
         find locations for the fragments written inside cross-reference
         boxes.  The `clear_frag_lists' argument should be true when preparing
         for the first write phase -- it resets the contents of any
         `crossref->frag_list' and `cross_ref->box_type' members so that they
         can be filled in during the simulated write phase. */
    bool mark_for_writing(jx_metagroup *context);
      /* This function plays an important role in preparing the metadata
         tree for writing.  It figures out which boxes might have to be
         written multiple times and when they should be written in order
         to create a copy of the scale-space structure which is used
         internally within the file hierarchy.  This ultimately allows
         the file's metadata to be efficiently delivered via JPIP.
            The function recursively descends through all the descendants
         of the current node; if one is found which has not previously
         been written (or marked) with any context, it is marked with the
         context supplied here.  If the function succeeds in marking any
         descendant, or if the present node has not previously been
         marked with any context, the function marks the present node with
         the `context' and returns true.  Otherwise, it returns false. */
    jp2_output_box *write(jp2_output_box *super_box, jx_target *target,
                          jx_metagroup *context, int *i_param,
                          void **addr_param);
      /* This function recursively passes through the metadata tree, writing
         all nodes whose current write context has been marked to match
         the supplied `context' argument.  If any node is marked, its parent
         is also marked, so we can simply terminate the recursion as soon as
         a marked node is encountered.  The function opens internal
         super-boxes as necessary and keeps track of an `active_descendant'
         inside each node.  This allows the function to be called again, if
         it returned prematurely, to resume writing where it left off.  The
         function returns prematurely if it encounters any node whose box
         contents must be written by the application.  In this case, it
         sets the *`i_param' and *`addr_param' values appropriately and returns
         with a pointer to the open box whose contents are to be written by
         the application.  If `super_box' is NULL, the first box is opened
         as a top level box within the `target' object.  Otherwise, `target'
         is not used. */
     kdu_long discover_output_box_locations(kdu_long start_pos,
                                            jx_metagroup *context);
       /* This recursive function descends through the current node and its
          descendants, skipping over boxes whose `write_state' is NULL or has
          a context which does not match `context'.  The boxes which it
          actually visits are assumed to have been written (or simulated),
          with correct values for the `write_state->content_offset',
          `write_state->content_length' and `write_state->node_length'
          members.  The function accumulates the relevant lengths so as to
          discover the absolute location of every box which was written.
          This information is used to complete the `crossref->frag_list'
          information for cross-reference nodes whose `crossref->box_type'
          member is still 0 (meaning that no location has yet been found).
            Along the way, the function deletes the `write_state' of boxes it
          encounters.
            For convenience, the function returns the location of the next
          box following any boxes which were discovered here. */
  public: // Data
    jx_meta_manager *manager;
    kdu_uint32 box_type;
    kdu_uint16 flags; // JX_METANODE_xxx where xxx includes: IS_COMPLETE,
       // BOX_COMPLETE, DESCENDANTS_KNOWN, EXISTING, WRITTEN, DELETED,
       // CONTENTS_CHANGED and ANCESTOR_CHANGED
    kdu_byte rep_id; // One of the constants defined below
#define JX_NULL_NODE     ((kdu_byte) 0) // `rep_id' 0 until reading complete
#define JX_REF_NODE      ((kdu_byte) 1)
#define JX_NUMLIST_NODE  ((kdu_byte) 2)
#define JX_ROI_NODE      ((kdu_byte) 3)
#define JX_LABEL_NODE    ((kdu_byte) 4)
#define JX_CROSSREF_NODE ((kdu_byte) 5)
    union {
        jx_metaref *ref; // NULL unless box identified as delayed or existing
        jx_numlist *numlist; // NULL unless this is a Number List node
        jx_regions *regions; // NULL unless this is an ROI Description node
        char *label; // NULL unless this is a label node
        jx_crossref *crossref; // NULL unless this is a cross-reference node
      };
    union {
        jx_metaread *read_state; // NULL unless we have an incomplete open box
        jx_metawrite *write_state; // Non-NULL only after writing started
      };
    kdu_byte data[16]; // Depends on box-type; UUID boxes store UUID code here
    int num_descendants;
    int num_completed_descendants; // See below
    int sequence_index; // Obtained when first added
    void *app_state_ref; // Used to record application-specific state reference
    jx_metanode *parent; // Null if this is the root
    jx_metanode *head; // Head of sibling list; NULL if this is a leaf node
    jx_metanode *tail; // Tail of sibling list; NULL if this is a leaf node
    jx_metanode *next_sibling; // Used to link descendants of intermediate node
    jx_metanode *prev_sibling; // List is doubly linked
    jx_crossref *linked_from; // List of all cross-references which link to us
    jx_metagroup *metagroup; // Points to meta-manager group to which we belong
    jx_metanode *next_link; // Used to link into meta-manager groups
    jx_metanode *prev_link; // Also doubly linked
    jx_metanode *next_touched; // Used to link into touched node list
    jx_metanode *prev_touched; // Also doubly linked
  };
  /* Notes:
        The `num_descendants' member counts the number of elements in the
     list headed by `head', all of which are connected via their
     `next_sibling' and `prev_sibling' members.
        The `num_completed_descendants' member holds the number of descendants
     which have been completely parsed, meaning that the box header and
     all of its sub-boxes have been completely parsed. */

/*****************************************************************************/
/*                              jx_metagroup                                 */
/*****************************************************************************/

struct jx_metagroup {
  public: // Member functions
    void init()
      { // Can't do this in constructor, since we want to use object in union
        head = tail = NULL; roigroup = NULL;
      }
    void link(jx_metanode *node);
    void unlink(jx_metanode *node);
    bool mark_for_writing();
      /* Marks all nodes belonging to this metagroup for writing, including
         all of their descendants, and all parents on which they depend.
         Returns true if anything has been marked as a result of this call. */
  public: // Data
    jx_metanode *head;
    jx_metanode *tail;
    jx_roigroup *roigroup; // Non-NULL only if metagroup belongs to a larger
       // grouping of ROI description nodes; this member is set only when a
       // new ROI description node is linked into the group via
       // `jx_meta_manager::link'.
  };

/*****************************************************************************/
/*                               jx_roigroup                                 */
/*****************************************************************************/

#define JX_FINEST_SCALE_THRESHOLD 8
#define JX_ROIMETAGROUP_SIZE 128 // Size of region occupied by each metagroup
#define JX_ROIGROUP_SIZE 8 // Dimension of array of sub-groups or metagroups.
struct jx_roigroup {
  public: // Member functions
    jx_roigroup(jx_meta_manager *manager, int scale_idx, int level_idx)
      {
        this->manager = manager;
        this->parent = NULL;
        this->level = level_idx;
        this->scale_idx = scale_idx;
        elt_size.x = elt_size.y = JX_ROIMETAGROUP_SIZE<<scale_idx;
        for (; level_idx > 0; level_idx--)
          elt_size.x = elt_size.y *= JX_ROIGROUP_SIZE;
        group_dims.size.x = group_dims.size.y = elt_size.x * JX_ROIGROUP_SIZE;
        memset(metagroups,0,
               sizeof(jx_metagroup)*JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE);
      }
    ~jx_roigroup();
    void delete_child(kdu_coords loc);
      /* This function deletes the indicated element of the `sub_groups' or
         `metagroups' array, as appropriate, and recursively propagates the
         deletion operation to its own parent if this leaves it empty. */
  public: // Data
    jx_meta_manager *manager;
    jx_roigroup *parent; // NULL if this is the root for some scale
    int level; // 0 for leaf nodes in spatial partition tree for this scale
    int scale_idx; // Position within the `jx_meta_manager::roi_scales' array
    kdu_dims group_dims; // Region occupied by entire group
    kdu_coords elt_size; // Size of region occupied by each list/sub-group
    jp2_output_box active_out; // Used to write nested association boxes
    union {
        jx_roigroup *sub_groups[JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE];
        jx_metagroup metagroups[JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE];
      };
  };
  /* Notes:
       This object is used to efficiently manage spatially sensitive metadata
       in large images.  The metagroups referenced here are those which manage
       lists of nodes which hold ROI description boxes.  The ROI groups
       form a collection of spatial hierarchies, where each hierarchy has
       a corresponding scale.  The scale is determined by the maximum
       dimension of the region occupied by the relevant ROI description box.
          The ROI groups for which `level'=0, are at the leaves of the
       hierarchical structure associated with their scale.  For these
       groups, the `metagroups' array is used to reference the individual
       metadata groupings.  The array's elements represent a grid which
       is JX_ROIGROUP_SIZE elements wide by JX_ROIGROUP_SIZE elements high,
       arranged in raster order.  The dimensions of the region occupied by
       each metagroup are given by `elt_size'.  The location and dimensions
       of the entire array of metagroups are given by `group_dims'.
          If `level'>0, the `sub_groups' member contains an array of
       pointers to other metagroups.  The array is `JX_ROIGROUP_SIZE'
       metagroups wide by `JX_ROIGROUP_SIZE' metagroups high and
       `elt_size' measures the size of each such group. */

/*****************************************************************************/
/*                                jx_metaloc                                 */
/*****************************************************************************/
// Constants:
#define JX_METALOC_IS_BLOCK ((jx_metanode *) _kdu_long_to_addr(2))
 
struct jx_metaloc {
  public: // Convenience functions
    jx_metaloc() { loc = 0;  target = NULL; }
    bool is_block() { return (target == JX_METALOC_IS_BLOCK); }
    kdu_long get_loc() { return loc; }
  private: // Don't allow others to change `loc' accidentally
    friend class jx_metaloc_manager;
    kdu_long loc;
  public: // Data
    jx_metanode *target; // Can carry special values (see below)
  };
  /* Notes:
       This object is used in conjuction with `jx_metaloc_block' and
     `jx_metaloc_manager' to manage location information for metanodes.
     Location information is kept for metanodes when they are obtained
     by reading from a file or a cache.  Location information is also
     kept when a metanode is created by copying from another metanode,
     which may belong to a different meta-manager.  Location information
     allows metanodes to be resolved from locations, which is particularly
     important when resolving links contained within cross-reference boxes.
       A location is identified by the `loc' member, as follows:
     -> For boxes found in a file, the `loc' member holds the location of
        the first byte of the box contents within the file.
     -> For boxes found in a cache, the `loc' member holds the location of
        the first byte of the box contents within its original file, as
        reconstructed from the relevant placeholder boxes.  This information
        can be obtained by adding the length of the box header to the value
        returned by `jp2_input_box::get_locator().get_file_pos()'.
     -> For metanodes obtained by copying other metanodes, the `loc'
        member holds the address of the relevant `jx_metanode' object,
        cast as an integer (usually a 64-bit integer, unless kdu_long is
        defined to be 32-bits, which is highly unlikely).
       For metanodes which are associated with an asoc box read from a file
     or cache, there are two separate locators: one for the first byte of
     the contents of the asoc box; and one for the first byte of the
     contents of its first sub-box (the one which gives the metanode its
     type).
       In some cases, the `target' of a locator may not be known.
     This happens when a link (represented by a cross-reference box) is
     encountered and the metanode (if copied) or box (if read) to which it
     refers is not yet available.  In this case, a `jx_metaloc' locator
     is added as the target of the link with a NULL `target' member, to be
     filled in once the associated metanode is discovered and associated
     with the locator.
       The `target' member can also take on the special, value of
     `JX_METALOC_IS_BLOCK', which is neither a valid address, nor NULL.
     In this case, the `primary' and `secondary' members refer to the first
     location represented by an ordered block of locators or sub-blocks.
     See `jx_metaloc_block' for an explanation of how an efficiently
     searchable and expandable tree of meta-locators is built using this
     mechanism. */
 
/*****************************************************************************/
/*                             jx_metaloc_block                              */
/*****************************************************************************/
// Constants:
#define JX_METALOC_BLOCK_DIM 16 // Must be at least 2 (at least binary tree)

struct jx_metaloc_block : jx_metaloc {
    jx_metaloc_block()
      { memset(this,0,sizeof(*this)); target=JX_METALOC_IS_BLOCK; }
    jx_metaloc_block *parent; // NULL if this is the root container
    int active_elts; // Number of elements from `elts' array which are used
    jx_metaloc *elts[JX_METALOC_BLOCK_DIM]; // Pointers to locators or blocks
  };
  /* Notes:
       This object provides the mechanism to build an efficient structure for
     searching and dynamically updating meta-data locators.  The structure
     is a tree, in which each node has up to JX_METALOC_BLOCK_DIM branches.
     The actual number of branches is recorded in the `active_elts' member
     and the addresses of the branches are stored in the `elts' array.  Each
     branch in the `elts' array may point either to a `jx_metaloc' object
     (a leaf in the tree) or another `jx_metaloc_block' object (i.e.,
     a node with further branches).  You can readily rearrange the branches
     in a block to insert new locations or blocks of locations.  The
     allocation and deallocation of `jx_metaloc' and `jx_metaloc_block'
     objects is handled by `jx_metaloc_manager' in a manner which avoids
     excessive memory fragmentation or excessive calls to new and delete.
     To determine whether an element referenced by the `elts' array can
     be cast to type `jx_metaloc_block', you have only to test its type via
     the `jx_metaloc::is_block()' convenience member function. */ 

/*****************************************************************************/
/*                           jx_metaloc_manager                              */
/*****************************************************************************/

class jx_metaloc_manager {
  public: // Member functions
    jx_metaloc_manager();
    ~jx_metaloc_manager(); // Deallocates `locator_heap' and `block_heap' lists
    jx_metaloc *get_locator(kdu_long file_pos, bool create_if_necessary);
    jx_metaloc *get_copy_locator(jx_metanode *node, bool create_if_necessary)
      { return get_locator(_addr_to_kdu_long(node),create_if_necessary); }
      /* The above functions search for an existing locator first.  If none
         is found, the function either returns NULL or creates a new locator,
         depending on the `create_if_necessary' argument.  The first function
         identifies the location of the first byte of the relevant box
         contents within its original file (even if it is actually obtained
         from a cache).  The second function identifies an existing
         `jx_metanode' object as the source of a copy operation.  The
         returned object, if non-NULL, is guaranteed to be a leaf in the
         tree (i.e., not of type `jx_metaloc_block'). */
  private: // Internal types
#   define JX_METALOC_ALLOC_DIM 64 // Num locators/blocks to allocate at once
    struct jx_metaloc_alloc {
        jx_metaloc_alloc() { free_elts = JX_METALOC_ALLOC_DIM; }
        int free_elts; // Number of locators not yet allocated
        jx_metaloc locators[JX_METALOC_ALLOC_DIM];
        jx_metaloc_alloc *next;
      };
    struct jx_metaloc_block_alloc {
        jx_metaloc_block_alloc() { free_elts = JX_METALOC_ALLOC_DIM; }
        int free_elts; // Number of locators not yet allocated
        jx_metaloc_block blocks[JX_METALOC_ALLOC_DIM];
        jx_metaloc_block_alloc *next;
      };
  private: // Functions
    jx_metaloc *allocate_locator()
      {
        jx_metaloc_alloc *heap = locator_heap;
        if ((heap == NULL) || (heap->free_elts == 0))
          {
            heap = new jx_metaloc_alloc;
            heap->next = locator_heap;  locator_heap = heap;
          }
        return (heap->locators + (--heap->free_elts));
      }
    jx_metaloc_block *allocate_block()
      {
        jx_metaloc_block_alloc *heap = block_heap;
        if ((heap == NULL) || (heap->free_elts == 0))
          {
            heap = new jx_metaloc_block_alloc;
            heap->next = block_heap;  block_heap = heap;
          }
        return (heap->blocks + (--heap->free_elts));
      }
    void insert_into_metaloc_block(jx_metaloc_block *container,
                                   jx_metaloc *elt, int idx_of_predecessor);
      /* This function inserts `elt' into the location immediately
       following `idx_of_predecessor' (may be -ve if `elt' is supposed to
       be inserted into the first position of the containing metaloc block).
       If the container is not already full, this is readily accomplished
       by shuffling the existing elements (if required).  If the container is
       full, the function invokes itself recursively to insert a new
       metaloc block into the container's parent and redistribute elements
       between the current container and the new one, so as to encourage the
       formation of balanced trees.  The recursive application of this
       function may result in the entire tree becoming deeper, with a new
       root node. */
  private: // Data
    jx_metaloc_block *root; // Top of the search tree
    jx_metaloc_alloc *locator_heap;
    jx_metaloc_block_alloc *block_heap;
  };

/*****************************************************************************/
/*                             jx_meta_manager                               */
/*****************************************************************************/

struct jx_meta_manager {
  public: // Functions
    jx_meta_manager();
    ~jx_meta_manager();
    bool test_box_filter(kdu_uint32 box_type);
      /* Returns true if the indicated box type passes the parsing filter
         test -- as set up by the `jpx_meta_manager::set_box_filter'
         function. */
    void link(jx_metanode *node);
      /* This function does all the work of building the `jx_metagroup' and
         `jx_roigroup' structure, and linking the new node in, unless it does
         not belong to any metagroup. */
    jp2_output_box *write_metadata(int *i_param, void **addr_param);
      /* Implements `jpx_target::write_metadata'. */
  public: // Data
    jx_metaloc_manager metaloc_manager; // Location services for metadata
    jp2_family_src *ultimate_src;
    jx_source *source;
    jx_target *target; // Becomes non-NULL only once writing has commenced
    jx_metanode *tree; // Always exists; destroying this one node, kills tree
    int next_sequence_index; // Used to assign unique indices when nodes added
    jx_metagroup unassociated_nodes;
    jx_metagroup numlist_nodes;
    jx_roigroup *roi_scales[32]; // First element corresponds to finest scale
    jx_metanode *last_added_node; // Last node added to any metagroup.
    jp2_output_box active_out;
    jx_metagroup *active_metagroup; // Group which is being written
    kdu_long active_metagroup_out_pos; // Location of first byte in metagroup
    int max_filter_box_types; // Num elements in the `filter_box_types' array
    int num_filter_box_types;
    kdu_uint32 *filter_box_types;
    jx_metanode *deleted_nodes; // When nodes are deleted, we unlink them from
        // any metagroups and from their parent node, if any, then move
        // them onto this list, connected via their `next_sibling' members.
        // They are not actually destroyed until the meta-manager itself is
        // destroyed.  This increases the robustness of applications which
        // manipulate meta-data and (more importantly) allows us to resolve
        // links and file addresses even if the original node has disappeared.
    jx_metanode *touched_head, *touched_tail; // Used to manage a doubly-linked
        // list of touched nodes, as described in connection with the function
        // `jpx_meta_manager::get_touched_nodes'.
    bool have_link_nodes; // True if `crossref->link' is non-NULL in any node
    bool simulate_write; // True if calls to `write_metadata' are only
       // simulating the write operation, so as to discover the file locations
       // required to write links as cross-reference boxes.
};

/*****************************************************************************/
/*                             jx_registration                               */
/*****************************************************************************/

class jx_registration {
  public: // Member functions
    jx_registration()
      { max_codestreams = num_codestreams = 0; codestreams = NULL; }
    ~jx_registration()
      { if (codestreams != NULL) delete[] codestreams; }
    bool is_initialized() { return (num_codestreams > 0); }
      /* Returns true if `init' has been called. */
    void init(jp2_input_box *creg_box);
      /* Parses a codestream registration box. */
    void finalize(int channel_id);
      /* Use this form during reading.  If no codestream registration box was
         found, this function creates the default installation, with one
         codestream whose ID is identical to the compositing layer index.
         Note that this function does not set the `layer_size' member.  That
         member may need to be set later. */
    void finalize(j2_channels *channels, int layer_idx);
      /* Use this form during writing.  Uses the `j2_channels' object to
         determine which codestreams have been used.  Any which have not
         been assigned registration parameters are aligned with the
         compositing layer registration grid directly.  Note that this
         member does not set the `layer_size' member.  That member needs to
         be set by the caller, before `save_box' is invoked. */
    void save_box(jp2_output_box *super_box);
      /* Writes the codestream registration (creg) box. */
  private: // Internal structure
      struct jx_layer_stream {
          int codestream_id;
          kdu_coords alignment;
          kdu_coords sampling;
        };
  private: // Data
    friend class jx_layer_source;
    friend class jx_layer_target;
    friend class jpx_layer_source;
    friend class jpx_layer_target;
    friend class jpx_source;
    int max_codestreams;
    int num_codestreams;
    jx_layer_stream *codestreams;
    kdu_coords denominator;
    kdu_coords final_layer_size; // (0,0) until calculated
  };

/*****************************************************************************/
/*                          jx_codestream_source                             */
/*****************************************************************************/

class jx_codestream_source {
  public: // Member functions
    jx_codestream_source(jx_source *owner, jp2_family_src *src, int id,
                         bool restrict_to_jp2)
      {
        this->owner=owner; this->ultimate_src=src; this->id=id;
        this->restrict_to_jp2 = restrict_to_jp2;
        metadata_finished = have_bpcc = compatibility_finalized = false;
        stream_opened = stream_main_header_found = false;
        fragment_list = NULL; metanode = NULL;
      }
    ~jx_codestream_source()
      { if (fragment_list != NULL) delete fragment_list; }
    void donate_chdr_box(jp2_input_box &src);
      /* This function is called from the master `jx_source' object if it
         encounters a codestream header (chdr) box while parsing the top-level
         of the JPX data source.  The `src' box need not be complete.  Its
         state is donated to the internal `chdr' object and `src' is closed
         so that the caller can open the next box after the codestream header
         box at the top-level of the JPX data source.  The function attempts
         to parse all relevant sub-boxes of the codestream header box, but
         some data might not be available yet.  In this case, the attempt
         to parse sub-boxes will resume each time the `finish' function is
         called. */
    void donate_codestream_box(jp2_input_box &src);
      /* This function is called from the master `jx_source' object if it
         encounters a contiguous codestream (jp2c) or a fragment table (ftbl)
         box while parsing the top-level of the JPX data source.  The `src'
         box need not be complete.  Its state is transferred to the internal
         `stream_box', after which `src' is closed so that parsing can
         resume from the next top-level box in the JPX data source.  In the
         case of a contiguous codestream, this function does not actually
         read the box, but it does make the box available to the application
         via the `jpx_codestream_source::open_stream' function.  If that
         function is called before any further top level boxes are read, the
         underlying `jp2_family_src' object will not need to backtrack, so
         seeking will not be required.  If the box is closed, subsequent
         calls to `jpx_codestream_source::open_stream' will be implemented
         by using the `stream_loc' and `ultimate_src' members.  In the
         case of a fragment table, the function attempts to read the
         embedded fragment list, again so as to minimize the
         need for back tracking later on.  However, if the fragment list
         cannot yet be read, later attempts will be made to read it in
         response to `jpx_codestream_source::stream_ready',
         `jpx_codestream_source::access_fragment_list' or
         `jpx_codestream_source::open_stream' calls. */
    bool have_all_boxes()
      { return (metadata_finished && !stream_loc.is_null()); }
      /* Returns true if the object is no longer waiting for any JP2-family
         boxes to become available.  This allows the caller to determine
         whether the cause of a false return from `finish' is due only to
         the unavailability of the code-stream main header (still waiting
         for it to arrive from a remote server, for example). */
    bool finish(bool need_embedded_codestream);
      /* This function may be called at any time, but is generally called
         whenever `jpx_source::access_codestream' is used, to see whether or
         not all the required information for a codestream has been recovered.
            If the codestream header box has not been encountered, or defaults
         may be relevant and the default JP2 header box has not yet been
         completely parsed, the function calls appropriate member functions
         of the owning `jx_source' object in order to parse the relevant
         boxes, if possible.  If this fails to produce a complete
         representation, the function returns false, meaning that further
         parsing will be required at a later point.
            If `need_embedded_codestream' is true, the function also tries
         to read top level boxes until the contiguous codestream box or
         fragment table box for this codestream has been encountered, returning
         false if it cannot find one.
            Once the above conditions are satisfied, the function returns
         true. */
    bool parse_fragment_list();
      /* Returns true if a complete fragment list can be parsed from
         the fragment table in `stream_box', in which case `stream_box' is
         closed and re-opened using `jpx_input_box::open' to read from the
         fragments.  Otherwise, the function returns false. */
    bool is_stream_ready()
      {
        if (stream_box.get_box_type() == jp2_fragment_table_4cc)
          return parse_fragment_list();
        if ((!stream_main_header_found) && stream_box.exists() &&
            ((!stream_box.has_caching_source()) ||
             stream_box.set_codestream_scope(id)))
          stream_main_header_found = true;
        return stream_main_header_found;
      }
      /* Returns true if the codestream box has been found and
         the codestream main header is also available. */
    j2_component_map *get_component_map() { return &component_map; }
      /* Used by `jx_layer_source' to finish a compositing layer. */
    // ------------------------------------------------------------------------
  private: // Data
    friend class jpx_codestream_source;
    jx_source *owner;
    jp2_family_src *ultimate_src;
    int id;
    bool metadata_finished; // True if chdr box finished or non-existent
    bool stream_main_header_found; // May be false with caching data source
    bool stream_opened; // True if `jpx_codestream_source::stream_open' called
    bool restrict_to_jp2; // If true and `id'=0, ignore codestream header box
    jp2_input_box chdr; // Used to manage an open codestream header box
    jp2_input_box sub_box; // For parsing sub-boxes
    j2_dimensions dimensions;
    bool have_bpcc; // If we have seen a bpcc box in the JP2 header
    bool compatibility_finalized;
    j2_palette palette;
    j2_component_map component_map;
    jp2_locator header_loc; // If a codestream header is encountered
    jx_metanode *metanode; // Used to keep track of where we are sending any
                       // auxiliary metadata encountered in the header, until
                       // we have recovered all the image related boxes.
    jp2_locator stream_loc; // Non-empty once the jp2c or ftbl box is found
    jpx_input_box stream_box; // To read jp2c, parse ftbl, and translate flst
    jx_fragment_list *fragment_list;
  };

/*****************************************************************************/
/*                            jx_layer_source                                */
/*****************************************************************************/

class jx_layer_source {
  public: // Member functions
    jx_layer_source(jx_source *owner, int id)
      {
        this->owner=owner; this->id=id;
        finished = stream_headers_available = false;
        metanode = NULL;
      }
    ~jx_layer_source()
      { j2_colour *scan;
        while ((scan=colour.next) != NULL)
          { colour.next = scan->next; delete scan; }
      }
    void donate_jplh_box(jp2_input_box &src);
      /* This function is called from the master `jx_source' object if it
         encounters a compositing layer header (jplh) box while parsing the
         top-level of the JPX data source.  The `src' box need not be
         complete.  Its state is donated to the internal `jplh' object and
         `src' is closed so that the caller can open the next box after the
         compositing layer header box at the top-level of the JPX data source.
         The function attempts to parse all relevant sub-boxes of the
         compositing layer header box, but some data might not be available
         yet.  In this case, the attempt to parse sub-boxes will resume each
         time the `finish' function is called. */
    bool finish();
      /* This function may be called at any time, but is generally called
         whenever `jpx_source::access_layer' is used, to see whether or
         not a `jpx_layer_source' interface can be attached.  If the
         compositing layer header box has not been encountered, or the
         default JP2 header box has not yet been completely parsed, the
         function calls appropriate member functions of the owning `jx_source'
         object in order to parse the relevant boxes, if possible.  It also
         tries to finish all codestreams on which the compositing layer
         header box depends.  If these steps fail to produce a complete
         representation, the function returns false, meaning that a
         `jpx_layer_source' interface cannot yet be offered to the
         application.  Otherwise, the function returns true. */
    bool all_streams_ready()
      { return (stream_headers_available || check_stream_headers()); }
      /* Returns true if all codestreams associated with this compositing
         layer have their main codestream headers available. */
    jx_registration *access_registration() { return &registration; }
      /* Used to query the codestreams which are associated with a
         compositing layer, even if it cannot yet be opened via
         `jpx_source::access_layer'. */
  // --------------------------------------------------------------------------
  private: // Helper functions
    bool check_stream_headers();
      /* Checks to see if all codestream main headers are available, returning
         true and setting `stream_headers_available' if they are. */
  private: // Data
    friend class jpx_layer_source;
    jx_source *owner;
    int id;
    bool finished; // True if `finish' has returned true in the past
    bool stream_headers_available; // If `all_streams_ready' has returned true
    jp2_input_box jplh; // Used to manage an open compositing layer header box
    jp2_input_box cgrp; // Used to manage an open colour group box
    jp2_input_box sub_box; // Used to manage sub-boxes of the above boxes
    jp2_locator header_loc; // If a compositing layer header is encountered
    j2_resolution resolution;
    j2_channels channels;
    j2_colour colour; // First element in linked list of boxes
    jx_registration registration;
    jx_metanode *metanode; // Used to keep track of where we are sending any
                       // auxiliary metadata encountered in the header, until
                       // we have recovered all the image related boxes.
  };

/*****************************************************************************/
/*                                jx_source                                  */
/*****************************************************************************/

class jx_source {
  public: // Member functions
    jx_source(jp2_family_src *src);
    ~jx_source();
    bool is_top_level_complete() { return top_level_complete; }
      /* Returns true if all top-level boxes in the JPX data source have been
         seen. */
    bool are_layer_header_boxes_complete()
      { return top_level_complete || restrict_to_jp2; }
      /* True if we will not parse any more compositing layer header boxes,
         either because we have seen all top-level boxes in the JPX data
         source, or because this is a JP2 file, meaning that we will ignore
         all layer header boxes. */
    bool parse_next_top_level_box(bool already_open=false);
      /* Call this function to encourage the object to parse a new top-level
         box from the JPX data source.  If no new box can be parsed, the
         function returns false.  The function is normally called from
         within `jx_codestream_source::finish' or `jx_layer_source::finish'.
         This function protects itself against recursive calls, which can
         arise where a box is passed to `jx_codestream_source::donate_chdr'
         or `jx_layer_source::donate_jplh' which in turn calls the present
         function to seek for further boxes.  If the function is called
         while another call to it is in progress, it will return false to
         the second call.  If `already_open' is true, the `top_box' object
         must already be open.  This currently happens only when we overshoot
         the mandatory header while inside `jpx_source::open'. */
    bool finish_jp2_header_box();
      /* Call this function to parse as much as possible of the JP2 header
         box, without actually opening a new top level box.  If a JP2
         header box has not already been found, or if the thing has already
         been completely parsed, nothing will be done.  The function returns
         true if there is the JP2 header box has been completely parsed
         by the time this function returns.  Otherwise, it returns false. */
    int get_num_codestreams() { return num_codestreams; }
    int get_num_compositing_layers() { return num_layers; }
    jx_codestream_source *add_codestream();
      /* Augments the `codestreams' array, if necesssary, creates a new
         `jx_codestream_source' object and inserts it into the array with
         `num_codestreams' as its index, then increments `num_codestreams'. */
    jx_layer_source *add_layer();
      /* Augments the `layers' array, if necesssary, creates a new
         `jx_layer_source' object and inserts it into the array with
         `num_layers' as its index, then increments `num_layers'. */
    jx_codestream_source *get_codestream(int codestream_id)
      {
        while (codestream_id>=num_codestreams)
          if (!parse_next_top_level_box())
            return NULL;
        return codestreams[codestream_id];
      }
      /* Returns a pointer to the internal `jx_codestream_source' whose
         position in the file matches he supplied `codestream_id'.  If the
         codestream has not yet been encountered, `parse_next_top_level_box'
         is used to advance to the point where the codestream is encountered
         or the end of the data source is reached, or
         `parse_next_top_level_box' returns false for other reasons (e.g.,
         incomplete dynamic cache, or a recursive call).  In any event, if
         no codestream is found, the function returns NULL.  The function
         does not attempt to call `jx_codestream_source::finish' itself,
         but the caller may do this. */
    jx_layer_source *get_compositing_layer(int layer_id)
      {
        while (layer_id >= num_layers)
          if (!parse_next_top_level_box())
            return NULL;
        return layers[layer_id];
      }
      /* Same as `get_codestream', but for compositing layers. */
    j2_dimensions *get_default_dimensions()
      {
        if (jp2h_box_complete || finish_jp2_header_box())
          return &default_dimensions;
        else return NULL;
      }
      /* Returns NULL if a JP2 header box has not yet been fully parsed, but
         there is a possibility that one will be encountered or fully parsed
         in the future.  Otherwise, it returns a pointer to a non-finalized
         `j2_dimensions' object; if that object's `is_initialized' function
         reports false, there will be no default dimension information for
         codestreams.  This function will not attempt to parse a new top level
         box from the data source, but it will attempt to finish parsing a
         JP2 header box if one has already been found. */
    j2_palette *get_default_palette()
      {
        if (jp2h_box_complete || finish_jp2_header_box())
          return &default_palette;
        else return NULL;
      }
      /* Returns NULL if a JP2 header box has not yet been parsed to the
         point where a default palette box has been recovered, but
         there is a possibility that one will be encountered or fully parsed
         in the future.  Otherwise, it returns a pointer to a non-finalized
         `j2_palette' object; if that object's `is_initialized' function
         reports false, there will be no default palette.  This function will
         not attempt to parse a new top level box from the data source, but
         it will attempt to finish parsing a JP2 header box if one has
         already been found. */
    j2_component_map *get_default_component_map()
      {
        if (jp2h_box_complete || finish_jp2_header_box())
          return &default_component_map;
        else return NULL;
      }
      /* Same as `get_default_palette' but for the default component mapping
         box in a JP2 header box, if any. */
    j2_channels *get_default_channels()
      {
        if (jp2h_box_complete || finish_jp2_header_box())
          return &default_channels;
        else return NULL;
      }
      /* Same as `get_default_palette' but for the default channel definitions
         box in a JP2 header box, if any. */
    j2_resolution *get_default_resolution()
      {
        if (jp2h_box_complete || finish_jp2_header_box())
          return &default_resolution;
        else return NULL;
      }
      /* Same as `get_default_palette' but for the default resolution
         box in a JP2 header box, if any. */
    j2_colour *get_default_colour()
      {
        if (jp2h_box_complete || finish_jp2_header_box())
          return &default_colour;
        else return NULL;
      }
      /* Returns NULL if a JP2 header box has not yet been fully parsed, but
         there is a possibility that one will be encountered or fully parsed
         in the future.  Otherwise, it returns a pointer to a linked list of
         `j2_colour' objects -- one for each colour description box
         encountered in the JP2 header box.  If the linked list contains
         only one element, it is possible that that element's `is_initialized'
         function will report false, in which case the JP2 header box either
         does not exist or does not contain any colour descriptions.  This
         function will not attempt to parse a new top level box from the data
         source, but it will attempt to finish parsing a JP2 header box if
         one has already been found. */
    jx_metanode *get_metatree()
      { return meta_manager.tree; }
    bool test_box_filter(kdu_uint32 box_type)
      { return meta_manager.test_box_filter(box_type); }
    jp2_data_references get_data_references()
      { return jp2_data_references(&data_references); }
  private: // Data
    friend class jpx_source;
    jp2_family_src *ultimate_src;
    int ultimate_src_id; // Used to validate identity of ultimate source
    bool have_signature; // If we have seen the signature box
    bool have_file_type; // If we have seen the file-type box
    bool have_reader_requirements; // If we have seen reader requirements box
    bool is_completely_open; // If `jpx_source::open' has returned true
    bool restrict_to_jp2; // See below
    bool in_parse_next_top_level_box; // If the function is in progress.
    int num_codestreams; // Number found so far
    int num_layers; // Number of compositing layers found so far
    jp2_input_box top_box; // Used to walk through top level boxes
    bool top_level_complete; // If we have seen all top level boxes
    jp2_input_box jp2h_box; // Used to parse JP2 header box
    bool jp2h_box_found; // Once we have found the JP2 header box
    jp2_input_box sub_box; // Used to parse JP2 header sub-boxes
    bool jp2h_box_complete; // Once we have finished parsing the JP2 header box
    j2_dimensions default_dimensions;
    bool have_default_bpcc; // If we have seen a bpcc box in the JP2 header
    j2_data_references data_references;
    bool have_dtbl_box; // If `dtbl' box has been encountered
    jp2_input_box dtbl_box; // Holds open `dtbl_box' until fully parsed
    j2_palette default_palette;
    j2_component_map default_component_map;
    j2_channels default_channels;
    j2_colour default_colour; // Head of colour box list
    j2_resolution default_resolution;
    int max_codestreams; // Size of the allocated `codestreams' array
    jx_codestream_source **codestreams; // An array of pointers
    int max_layers; // Size of the allocated `layers' array
    jx_layer_source **layers; // An array of pointers
    int num_chdr_encountered; // Num codestream header boxes encountered
    int num_jp2c_or_ftbl_encountered; // Num JP2C or FTPL boxes encountered
    jx_compatibility compatibility;
    jx_composition composition;
    jx_meta_manager meta_manager;
  };
  /* Notes:
       `restrict_to_jp2' is true if the data source is JP2, not just
     JP2 compatible.  In this case, compositing layer header boxes will be
     ignored, and the first codestream header box, if any, will also be
     ignored.
  */

/*****************************************************************************/
/*                          jx_codestream_target                             */
/*****************************************************************************/

class jx_codestream_target {
  public: // Member functions
    jx_codestream_target(jx_target *owner, int id)
      {
        this->owner = owner; this->id = id;
        codestream_opened = finalized = chdr_written = have_breakpoint = false;
        i_param = 0; addr_param = NULL; next = NULL;
      }
    void finalize();
      /* This function is called from the owning `jx_target' object inside
         `write_header'.  You must call this function prior to
         `jx_layer_target::finalize' and prior to `write_chdr'. */
    bool is_chdr_complete() { return chdr_written; }
      /* Returns true if `write_chdr' has completed successfully already. */
    bool is_complete()
      { return (chdr_written && codestream_opened && !jp2c.exists()); }
      /* Returns true only once the contiguous codestream has been completely
         written. */
    j2_component_map *get_component_map()
      { assert(finalized); return &component_map; }
      /* Used by `jx_layer_target' to finish a compositing layer.  This is
         generally done prior to the point at which `write_chdr' or
         `copy_defaults' is called. */
    jp2_output_box *write_chdr(int *i_param=NULL, void **addr_param=NULL);
      /* Writes a top-level codestream header box.  Before writing each
         of the sub-boxes, the function uses the `jx_target' object to see
         if any defaults are available which exactly match the sub-box which is
         about to be written.  If so, the sub-box need not be written.  In
         any event, though, a codestream header box is always written, even
         if it turns out to be empty.
            If a breakpoint was installed, this function returns prematurely
         with a pointer to the codestream box itself, setting *`i_param'
         and *`addr_param' (if non-NULL) equal to the breakpoint's integer
         and address parameters, respectively.  In this case, the function
         will need to be called again. */
    void copy_defaults(j2_dimensions &default_dims, j2_palette &default_plt,
                       j2_component_map &default_map);
      /* Copies any initialized `dimensions', `palette' and `component_map'
         objects into the file-wide defaults supplied via this function's
         arguments. */
    void adjust_compatibility(jx_compatibility *compatibility);
      /* Called before writing a reader requirements box, this function
         gives the object an opportunity to add to the list of features
         which will be included in the reader requirements box and to
         signal that the file will not be JP2 compatible if that is found
         to be the case. */
  private: // Data
    friend class jpx_codestream_target;
    jx_target *owner;
    int id;
    bool codestream_opened; // If `jpx_codestream_target::open_stream' used.
    bool finalized; // If `finalize' has been called
    bool chdr_written; // If codestream header box has been written
    bool have_breakpoint; // If `jpx_codestream_target::set_breakpoint' used
    int i_param; // Set by `jpx_codestream_target::set_breakpoint'
    void *addr_param; // Set by `jpx_codestream_target::set_breakpoint'
    jx_fragment_list fragment_list;
    j2_dimensions dimensions;
    j2_palette palette;
    j2_component_map component_map;
    jp2_output_box chdr; // Maintain state between calls to `write_chdr'
    jp2_output_box jp2c; // Used to write the contiguous codestream box.
  public: // Links for including this object in a list
    jx_codestream_target *next; // Next compositing layer in sequence
  };

/*****************************************************************************/
/*                             jx_layer_target                               */
/*****************************************************************************/

class jx_layer_target {
  public: // Member functions
    jx_layer_target(jx_target *owner, int id)
      {
        this->owner = owner; this->id = id;
        finalized = jplh_written = have_breakpoint = need_creg = false;
        i_param = 0; addr_param = NULL; last_colour = NULL; next = NULL;
      }
    ~jx_layer_target()
      { j2_colour *scan;
        while ((scan=colour.next) != NULL)
          { colour.next = scan->next; delete scan; }
      }
    bool finalize();
      /* This function is called from the owning `jx_target' object inside
         `write_header'.  You must call this function prior to
         `write_jplh'.  The function returns true if a component registration
         box is required for this compositing layer.  It is important to
         know this, since if any compositing layer header box contains a
         component registration box, they all must. */
    bool is_jplh_complete() { return jplh_written; }
      /* Returns true if `write_jplh' has completed successfully already. */
    jp2_output_box *write_jplh(bool write_creg_box, int *i_param=NULL,
                               void **addr_param=NULL);
      /* Writes a top-level compositing layer header box.  Before writing each
         of the sub-boxes, the function uses the `jx_target' object to see
         if any defaults are available which exactly match the sub-box which is
         about to be written.  If so, the sub-box need not be written.  In
         any event, though, a compositing layer header box is always written,
         even if it turns out to be empty.
            Component registration boxes are written if and only if the
         `write_creg_box' argument is true.  This is because, if any layer
         header box contains a component registration box, they all must.
            If a breakpoint was installed, this function returns prematurely
         with a pointer to the codestream box itself, setting *`i_param'
         and *`addr_param' (if non-NULL) equal to the breakpoint's integer
         and address parameters, respectively.  In this case, the function
         will need to be called again. */
    void copy_defaults(j2_resolution &default_res,
                       j2_channels &default_channels,
                       j2_colour &default_colour);
      /* Copies any initialized `resolution', `channels' and `colour'
         objects into the file-wide defaults supplied via this function's
         arguments. */
    void adjust_compatibility(jx_compatibility *compatibility);
      /* Called before writing a reader requirements box, this function
         gives the object an opportunity to add to the list of features
         which will be included in the reader requirements box and to
         signal that the file will not be JP2 compatible if that is found
         to be the case. */
    bool uses_codestream(int idx);
      /* Returns true if a codestream with the indicated index is used by
         the current compositing layer.  This is used to implement the
         functionality expected of `jpx_target::write_headers'. */
  private: // Data
    friend class jpx_layer_target;
    jx_target *owner;
    int id;
    bool finalized; // If `finalize' has been called
    bool need_creg; // If this object's `registration' member is non-trivial
    bool jplh_written; // If the compositing layer header box has been written
    bool have_breakpoint; // If `jpx_layer_target::set_breakpoint' used
    int i_param; // Set by `jpx_layer_target::set_breakpoint'
    void *addr_param; // Set by `jpx_layer_target::set_breakpoint'
    j2_resolution resolution;
    j2_channels channels;
    j2_colour colour; // First element in linked list of boxes
    j2_colour *last_colour; // Last element added to linked list (or NULL)
    jx_registration registration;
    jp2_output_box jplh; // Maintain state between calls to `write_jplh'
  public: // Links for including this object in a list
    jx_layer_target *next; // Next compositing layer in sequence
  };

/*****************************************************************************/
/*                                jx_target                                  */
/*****************************************************************************/

class jx_target {
  public: // Member functions
    jx_target(jp2_family_tgt *tgt);
    ~jx_target();
    kdu_long open_top_box(jp2_output_box *box, kdu_uint32 box_type,
                          bool simulate_write=false);
      /* Opens a new top-level output box, but keeps an internal record of
         the open box to make sure that two top-level boxes are not open
         at the same time.  If an open top level box already exists, the
         present function will generate an error.  All boxes supplied as
         arguments must be persistent for the life of the `jx_target' object
         so that it can always check whether or not they are still open.
         To escape from this condition, you may call the function with a NULL
         `box' argument when the top-level box is closed; in this case, you
         are free to destroy the box which was previously opened.
           If `simulate_write' is true, the box will be opened in such a way
         that written data disappears, rather than contributing to the
         target file.  This is used by an initial simulated metadata writing
         phase when link nodes exist, so that these link nodes can discover the
         locations and lengths of their target box contents.
           The function returns the location of the first byte of the box to
         be written, regardless of whether `simulate_write' is true or false.
         This is useful during simulation, since it can be used to compute
         the locations of all constituent boxes, once they are closed. */
    kdu_long get_file_pos()
      {
        if (simulated_tgt != NULL)
          return simulated_tgt->get_bytes_written();
        return ultimate_tgt->get_bytes_written();        
      }
      /* Retrieves the location of the real or simulated output file pointer.
         If `open_top_box' has just been called, the return value will be
         identical to the value returned by that function.  In fact, the
         value returned here may remain the same until the most recently
         opened top-level box is closed, because output JP2 boxes generally
         buffer their contents until they are closed, whereup their header
         can be correctly written.  The only exception is if a top-level box
         is declared to have rubber-length, or if the
         `jp2_output_box::write_header_last' function is invoked, in which
         case the box will write its contents immediately, advancing the
         output file pointer. */
    jx_codestream_target *get_codestream(int codestream_id)
      {
        jx_codestream_target *sp=codestreams;
        for (; (codestream_id>0) && (sp!=NULL); codestream_id--, sp=sp->next);
        return sp;
      }
      /* Returns a pointer to the indicated codestream, if one exists, or else
         NULL.  Used by `jx_layer_target' to build associations between
         compositing layers and their codestreams. */
    bool can_write_codestreams()
      { return main_header_complete && !headers_in_progress; }
    int get_num_layers() { return num_layers; }
    j2_dimensions *get_default_dimensions()
      { return &default_dimensions; }
    j2_palette *get_default_palette()
      { return &default_palette; }
    j2_component_map *get_default_component_map()
      { return &default_component_map; }
    j2_channels *get_default_channels()
      { return &default_channels; }
    j2_colour *get_default_colour()
      { return &default_colour; }
    j2_resolution *get_default_resolution()
      { return &default_resolution; }
    jp2_data_references get_data_references()
      { return jp2_data_references(&data_references); }
  private: // Data
    friend class jpx_target;
    jp2_family_tgt *ultimate_tgt;
    jp2_family_tgt *simulated_tgt; // Created on-demand by `open_top_box'
    jp2_output_box *last_opened_top_box;
    jp2_output_box box; // Stable resource to use with `open_top_box'
    int last_codestream_threshold; // From last call to `write_headers'
    bool need_creg_boxes; // If any layer needs creg, they all do
    bool headers_in_progress; // Last call to `write_headers' returned non-NULL
    bool main_header_complete; // After mandatory headers written
    int num_codestreams; // Number added so far
    int num_layers; // Number added so far
    j2_data_references data_references;
    j2_dimensions default_dimensions;
    j2_palette default_palette;
    j2_component_map default_component_map;
    j2_channels default_channels;
    j2_colour default_colour; // Head of colour box list
    j2_resolution default_resolution;
    jx_codestream_target *codestreams; // Head of a linked list
    jx_codestream_target *last_codestream; // Tail of above list
    jx_layer_target *layers; // Head of a linked list
    jx_layer_target *last_layer; // Tail of above list
    jx_compatibility compatibility;
    jx_composition composition;
    jx_meta_manager meta_manager;
  };

#endif // JPX_LOCAL_H
