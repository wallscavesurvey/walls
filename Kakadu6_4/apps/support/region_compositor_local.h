/******************************************************************************/
// File: region_compositor_local.h [scope = APPS/SUPPORT]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/******************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/******************************************************************************/
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
/*******************************************************************************
Description:
  Local definitions used in the implementation of `kdu_region_compositor'.
*******************************************************************************/
#ifndef REGION_COMPOSITOR_LOCAL_H
#define REGION_COMPOSITOR_LOCAL_H

#include "kdu_region_decompressor.h"
#include "kdu_region_compositor.h"

// Objects declared here
struct kdrc_queue;
struct kdrc_roinode;
struct kdrc_overlay_expression;
struct kdrc_overlay_segment;
class kdrc_overlay;
struct kdrc_refresh_elt;
class kdrc_refresh;
struct kdrc_codestream;
class kdrc_stream;
class kdrc_layer;

/******************************************************************************/
/*                                 kdrc_queue                                 */
/******************************************************************************/

struct kdrc_queue {
  public: // Member functions
    kdrc_queue()
      { composition_buffer=NULL; id=0; stamp=0; next=NULL; }
    ~kdrc_queue()
      { assert(composition_buffer == NULL); }
  public: // Data
    kdu_compositor_buf *composition_buffer;
    int id;
    kdu_long stamp;
    kdrc_queue *next;
  };

/******************************************************************************/
/*                                kdrc_roinode                                */
/******************************************************************************/

struct kdrc_roinode {
  public: // Member functions
    kdrc_roinode()
      { sort_area=0; cs_width=0; hidden=false; next=NULL; }
  public: // Data
    jpx_metanode node;
    kdu_dims bounding_box; // Bounding box, mapped to compositing grid,
      // augmented by the maximum painting border and clipped to segment region
    kdu_long sort_area; // Used to sort ROI nodes within each segment
    int cs_width; // Width given by `node.get_width' -- measured on cs canvas
    bool hidden; // True if this node is to be hidden
    kdrc_roinode *next;
  };
  /* Notes:
       This structure is used to manage lists of metadata nodes which have
     been found to be relevant to the current buffer surface.  Such lists
     are managed by `kdrc_overlay'. */

/******************************************************************************/
/*                          kdrc_overlay_expression                           */
/******************************************************************************/

struct kdrc_overlay_expression {
  public: // Functions
    kdrc_overlay_expression() { next_in_product = next_in_sum = NULL; }
    ~kdrc_overlay_expression()
      {
        kdrc_overlay_expression *term;
        while ((term=next_in_product) != NULL)
          { next_in_product = term->next_in_product; delete term; }
        if (next_in_sum)
          delete next_in_sum;
      }
    kdrc_overlay_expression *add_sum_term()
      { /* Augments the list of terms to be evaluated and OR'd together,
           returning a pointer to the new head of the sum-list. */
        kdrc_overlay_expression *term = new kdrc_overlay_expression;
        term->next_in_sum = this;
        return term;
      }
    bool add_product_term(jpx_metanode node, int effect);
      /* Adds `node' into the chain of expressions to be AND'd together by this
         object (the head of the sum-list) when it tests dependencies.  If
         `effect' is non-negative, the node must be related to the ROI being
         tested, for the term to evaluate to true.  Otherwise, the node must
         NOT be related to the ROI being tested, for the term to evaluate to
         true. Returns true if the new `node' has any effect upon the existing
         expression. */
    bool evaluate(jpx_metanode roi_node);
      /* Evaluates the complete sum-of-products expression. */
    bool equals(jpx_metanode node)
      { return ((next_in_product == NULL) && (next_in_sum == NULL) &&
                (positive_node == node) && (!negative_node)); }
  private: // We think of `kdrc_overlay_expres
    jpx_metanode positive_node;
    jpx_metanode negative_node;
    kdrc_overlay_expression *next_in_product; // Next term to be AND'd
    kdrc_overlay_expression *next_in_sum; // Next term to be OR'd
  };

/******************************************************************************/
/*                           kdrc_overlay_segment                             */
/******************************************************************************/

struct kdrc_overlay_segment {
  public: // Functions
    kdrc_overlay_segment()
      { head=first_unpainted=NULL; next=NULL; needs_process=false; }
    void reset(kdrc_roinode * &free_nodes)
      { // Put all the ROI nodes back on the free list
        while ((first_unpainted=head) != NULL)
          { head=first_unpainted->next; first_unpainted->next = free_nodes;
            free_nodes = first_unpainted; }
        needs_process = false;
      }
  public: // Data
    kdu_dims region; // Region occupied by segment on compositing grid
    kdrc_roinode *head; // Points to largest node which intersects the segment
    kdrc_roinode *first_unpainted; // Next node in the list to paint
    bool needs_process; // See below
    kdrc_overlay_segment *next; // Used to build a linked list; see below
  };
  /* Notes:
       Overlay information is segmented into fixed size segments, which are
     aligned absolutely on the compositing reference grid, so that segments
     occupy corresponding positions across all overlay buffers in a composition.
       Each such segment corresponds to a single reference in the
     `kdrc_overlay::seg_refs' array, and each such reference points to a
     linked list of `kdrc_overlay_segment' objects.
       At present, each reference in the `kdrc_overlay::seg_refs' array
     points to a single `kdrc_overlay_segment'.  However, in the future, we
     may allow for lists of `kdrc_overlay_segment' records, each spanning a
     smaller region.  The implementation is partially prepared for this
     eventuality.
       If `first_unpainted' is NULL, the segment's overlay information has
     been entirely painted.
       The `needs_process' member is true if the segment has not yet been
     processed by `kdrc_overlay::process'.  A segment which has a non-NULL
     `first_unpainted' member necessarily has `needs_process' equal to true;
     however, we also mark segments with `needs_process' equal to true if
     they have been recently erased within `kdrc_overlay::update_config',
     since the erased state is only communicated to the compositing logic
     in response to the information returned by `kdrc_overlay::process'. */

/******************************************************************************/
/*                          kdrc_trapezoid_follower                           */
/******************************************************************************/

struct kdrc_trapezoid_follower {
  public: // Member functions
    void init(kdu_coords left_top, kdu_coords left_bottom,
              kdu_coords right_top, kdu_coords right_bottom,
              int min_y, int max_y);
    void limit_max_y(int new_limit);
    void limit_min_y(int new_limit);
    int get_y_min() { return y_min; }
    bool get_next_line(double &x1_val, double &x2_val)
      { 
        x1_val=x1; x2_val=x2;
        if (y > y_end) return false;
        if ((++y) == y_end)
          { x1 += end_inc1; x2 += end_inc2; }
        else
          { x1 += inc1; x2 += inc2; inc1 = next_inc1; inc2 = next_inc2; }
        return true;
      }
  private: // Data
    int y_min; // First `y' coordinate (row index)
    int y; // Current `y' coordinate (row index)
    int y_end; // Final `y' coordinate
    double inc1, inc2; // Amount to increment x1 and x2 each time y increments
    double next_inc1, next_inc2;
    double end_inc1, end_inc2;
    double x1, x2; // x-coordinates of the left (x1) and right (x2) edges at `y'
  };
  /* Notes: This object is used by the `kdu_region_compositor::paint_overlays'
     function to paint quadrilaterals.  Specifically, the function is used to
     paint the lines between two bounding edges -- a left edge and a right
     edge.  This means that the function paints trapezoids, whose parallel
     edges are horizontal.
       The idea is that trapezoids will be painted row by row and this
     function is used to discover the left and right edges of each row.  To
     avoid gaps due to edges with high gradients, `x1' and `x2' boundaries
     are based on 3 quantities: x_c is the horizontal coordinate at the
     centre of the line; x_t is the horizontal coordinate at the top of the
     line; and x_b is the horizontal coordinate at the bottom of the line.
     Then the left edge is set to x1 = min{x1_c,x1_t+0.5,x1_b+0.5}, while
     the right edge is set to x2 = max{x2_c,x2_t-0.5,x2_b-0.5}.  This
     policy needs to be modified slightly at the vertices which define the
     edges of line segments, so as to exclude the contribution of
     x_t at the top and x_b at the bottom vertex.  To accommodate this,
     the `init' function sets up `inc1' and `inc2' to hold the amount by
     which `x1' and `x2' must be changed on the next line only; to be
     replaced by `next_inc1' and `next_inc2' on all subsequent lines, except
     the last (`y'=`y_end') for which `end_inc1' and `end_inc2' are to be
     used. */

/******************************************************************************/
/*                               kdrc_overlay                                 */
/******************************************************************************/

class kdrc_overlay {
  public: // Member functions
    kdrc_overlay(jpx_meta_manager meta_manager, int codestream_idx,
                 int log2_segment_size);
      /* Constructed from within `kdrc_stream::init' where a JPX data source
         is available.  Otherwise, the `kdrc_stream' object will have no
         overlay.  The `log2_segment_size' parameter here controls the size
         of segments into which the overlay metadata is partitioned.  These
         segments have a consistent size across all overlays in a composition.
         For more information, see the member variable of the same name. */
    ~kdrc_overlay();
    void set_geometry(kdu_coords image_offset, kdu_coords subsampling,
                      bool transpose, bool vflip,
                      bool hflip, kdu_coords expansion_numerator,
                      kdu_coords expansion_denominator,
                      kdu_coords compositing_offset,
                      int layer_idx);
      /* This function is called from the owning `kdrc_stream' object whenever
         the scale or orientation changes.  It provides the parameters
         required to map regions on the compositing surface to and from
         regions on the codestream's high resolution canvas, expressed relative
         to the image origin on that canvas.
         [//]
         The arguments may be interpreted in terms of a mapping from the
         coordinates used by ROI description boxes to the corresponding
         coordinates on the compositing reference grid.  Let `R' denote
         an object of the `kdu_dims' class, corresponding to an ROI
         description region.  The following steps will map `R' to a
         region on the compositing reference grid:
            1) Add `image_offset' to `R'.pos.  This translates the region
               to one which is correctly registered against the codestream's
               high resolution canvas.
            2) Divide the starting (inclusive) and ending (exclusive)
               coordinates of the region by `subsampling', rounding all
               coordinates up.
            3) Apply `kdu_dims::to_apparent', supplying the `transpose',
               `vflip' and `hflip' parameters.
            4) Multiply the coordinates by `expansion_numerator' over
               `expansion_denominator'.
            5) Subtract `compositing_offset'.
         [//]
         The final argument supplies the index of the compositing layer
         associated with the owning `kdrc_stream' object -- may be -1 if
         the stream is being used to display raw codestream data only.  This
         is useful in determining which regions of interest should be
         rendered on the overlay.
      */
    bool set_buffer_surface(kdu_compositor_buf *buffer,
                            kdu_dims compositing_region,
                            bool look_for_new_metadata,
                            kdrc_overlay_expression *dependencies,
                            bool mark_all_segs_for_processing);
      /* Called only from within `kdrc_layer', after `set_geometry' has
         been called from `kdrc_stream'.  This function removes all
         inappropriate metadata from the current list and updates internal
         state to reflect the portion of the overlay buffer which has still
         to be painted.  The function returns true if any metadata falls
         within the `compositing_region'.
            The function does not actually paint any metadata overlays.  It
         just schedules them to be painted in subsequent calls to `process'. */
    void update_config(int min_display_size,
                       kdrc_overlay_expression *dependencies,
                       const kdu_uint32 *aux_params, int num_aux_params,
                       bool dependencies_changed, bool aux_params_changed,
                       bool blending_factor_changed);
      /* If `painting_param' differs from the last used painting parameter,
         all metadata entries are marked as unpainted, so that subsequent calls
         to `process' will paint them with the new parameter.  If
         `min_display_size' has changed, some new overlay entries may be added
         or some may be removed.  The `dependencies' argument determines which,
         if any, of the overlay entries need to be hidden.  If
         `blending_factor_changed' is true, the blending factor has been
         changed, which means that every segment which contains metadata
         should have its `needs_process' flag set to true. */
    bool process(kdu_dims &seg_region);
      /* Call this function to process any unpainted overlay data onto the
         buffer installed by the last call to `set_buffer_surface'.  The
         function returns false if nothing more can be processed, either
         because there is no overlay buffer, or all metadata found in the last
         call to `set_buffer_surface' has been painted.  If the function does
         paint something, it paints an entire segment, setting `seg_region'
         equal to the region occupied by the complete segment and returning
         true.  The reason for always returning complete segment regions is
         that it allows the refresh manager and delayed compositing logic
         to assume that incomplete segments which lie on top of a recently
         updated region will be able to drive the compositing process at
         a later date. */
    bool get_unprocessed_overlay_seg(kdu_coords idx, kdu_dims &seg_region)
      {
        kdrc_overlay_segment *seg;
        idx.x -= seg_indices.pos.x;  idx.y -= seg_indices.pos.y;
        return ((seg_refs != NULL) && (idx.x >= 0) && (idx.y >= 0) &&
                (idx.x < seg_indices.size.x) && (idx.y < seg_indices.size.y) &&
                ((seg=seg_refs[idx.x+idx.y*seg_indices.size.x]) != NULL) &&
                seg->needs_process);
      }
      /* Does the work of `kdrc_layer::get_unprocessed_overlay_seg'. */
    void count_nodes(int &total_nodes, int &hidden_nodes);
      /* Does the work of `kdrc_layer::get_overlay_info'. */
    jpx_metanode search(kdu_coords point);
      /* Does the work of `kdrc_layer::search_overlay', returning a non-empty
         interface if a match is found. */
    void activate(kdu_region_compositor *compositor, int max_border_size);
      /* Called from `kdrc_layer' when it is initiated or activated. */
    void deactivate();
      /* Called from `kdrc_layer' when it is deactivated. The function may
         also be called immediately prior to `activate' to initiate a
         complete refresh of the buffer surface. */
  private: // Helper functions
    void map_from_compositing_grid(kdu_dims &region);
    void map_to_compositing_grid(kdu_dims &region);
    void recycle_seg_list(kdrc_overlay_segment *seg)
      { kdrc_overlay_segment *tmp;
        while ((tmp=seg) != NULL)
          { seg=tmp->next; tmp->reset(free_nodes);
            tmp->next=free_segs; free_segs=tmp; }
      }
    kdrc_roinode *node_alloc()
      {
        if (free_nodes == NULL)
          free_nodes = new kdrc_roinode;
        kdrc_roinode *nd=free_nodes; free_nodes=nd->next; nd->next=NULL;
        return nd;
      }
    kdrc_overlay_segment *seg_alloc(kdu_dims region)
      {
        kdrc_overlay_segment *seg=free_segs;
        if (seg == NULL)
          seg = new kdrc_overlay_segment;
        else
          free_segs = seg->next;
        seg->region = region; seg->next = NULL; seg->needs_process = false;
        return seg;
      }
  private: // Identification members
    kdu_region_compositor *compositor; // NULL unless activated
    jpx_meta_manager meta_manager;
    kdu_overlay_params params;
    int min_composited_size; // Does not include allowance for border
  private: // Geometry members passed in using `set_geometry'
    kdu_coords image_offset;
    kdu_coords subsampling;
    bool transpose, vflip, hflip;
    kdu_coords expansion_numerator;
    kdu_coords expansion_denominator;
    kdu_coords compositing_offset;
    int min_size;
  private: // Buffering members
    kdu_dims buffer_region; // Expressed in compositing domain
    kdu_compositor_buf *buffer;
  private: // Metadata list
    kdrc_roinode *free_nodes; // Efficient means for recycling node resources
    kdrc_overlay_segment *free_segs; // Efficient means of recycling segments
    kdu_dims seg_indices; // Range of absolute indices for active segments
    int log2_seg_size; // Nominal segment size is 2^{`log2_seg_size'}
    int num_segs; // `seg_indices.size.x' * `seg_indices.size.y'
    int first_unprocessed_seg; // `num_segs' if all processed 
    kdrc_overlay_segment **seg_refs; // Raster organized array, size `num_segs'
  };
  /* Notes:
       This class is used to manage overlay information which is generated
     from metadata found in a JPX source.  Each `kdrc_overlay' object is
     owned by exactly one `kdrc_stream' object.  The `kdrc_stream' object
     provides updates on the surface geometry as required.  However, it is
     `kdrc_layer' which actually activates and exercises the object.  Where
     an overlay surface buffer is required, it has exactly the same dimensions
     as the compositing layer's own composition buffer.
       The present object does not actually paint onto its overlay buffer
     directly.  Instead, it invokes virtual functions of the `compositor'
     object to do this.  This allows applications to simply override the
     overlay painting process, providing graphical effects which are most
     appropriate for exploiting application-specific metadata. */

/******************************************************************************/
/*                              kdrc_codestream                               */
/******************************************************************************/

struct kdrc_codestream {
  public: // Member functions
    kdrc_codestream(bool persistent, int cache_threshold)
      {
        head = NULL;  in_use=false;
        this->persistent = persistent;  this->cache_threshold = cache_threshold;
      }
    ~kdrc_codestream()
      {
        assert(head == NULL);
        if (ifc.exists())
          ifc.destroy();
      }
    void init(kdu_compressed_source *source);
      /* Open the JPEG2000 code-stream from a raw compressed data source. */
    void init(jpx_codestream_source stream);
      /* Open the JPEG2000 code-stream embedded within a JPX/JP2 data source. */
    void init(mj2_video_source *track, int frame_idx, int field_idx);
      /* Open the JPEG2000 code-stream embedded within an MJ2 video source. */
    bool restart(mj2_video_source *track, int frame_idx, int field_idx);
      /* Invokes `kdu_codestream::restart' if possible, rather than creating
         a new codestream engine.  If this is not possible, the function
         returns false. */
    void attach(kdrc_stream *user);
      /* After initialization, you must attach at least one `kdrc_stream'
         user.  Users are automatically attached at the head of the doubly
         linked list of common users.  If you later wish to move a user
         around, call `move_to_head' or `move_to_tail'. */
    void detach(kdrc_stream *user);
      /* Once all attached `kdrc_stream' users have detached, the present
         object will automatically be destroyed. */
    void move_to_head(kdrc_stream *user);
      /* Move this user to the head of the doubly linked list of users which
         share this code-stream. */
    void move_to_tail(kdrc_stream *user);
      /* Move this user to the tail of the doubly linked list of users which
         share this code-stream. */
  public: // Data
    bool persistent;
    int cache_threshold;
    jpx_input_box source_box; // Used when opening codestream inside JP2/JPX/MJ2
    kdu_codestream ifc;
    bool in_use; // If any user has an active `kdu_region_decompressor' engine
    kdu_dims canvas_dims; // Size of image on the high-resolution canvas
  public: // Links
    kdrc_stream *head; // Head of doubly-linked list of `kdrc_stream' objects
                       // which share this resource.
  };
  /* This structure is used to share a single `kdu_codestream' code-stream
     manager's machinery, state and buffering resources amongst potentially
     multiple compositing layers.  This allows the `kdrc_stream' object
     to be associated with exactly one compositing layer.  Note that at
     most one of the users may have an active `kdu_region_decompressor'
     object which has been started, but not yet finished. */

/******************************************************************************/
/*                                kdrc_stream                                 */
/******************************************************************************/

class kdrc_stream {
  public: // Member functions
    kdrc_stream(kdu_region_compositor *owner, bool persistent,
                int cache_threshold, kdu_thread_env *env,
                kdu_thread_queue *env_queue);
    ~kdrc_stream();
    void init(kdrc_codestream *new_cs, kdrc_stream *sibling);
      /* Use this form of the `init' function to render imagery from a
         codestream directly without file format colour/channel/palette
         information (even if it was available in the original data
         source).  In this case, exactly one of `new_cs' or `sibling'
         should be non-NULL.  If `sibling' is non-NULL, it must
         point to an existing `kdrc_stream' object which uses the same
         code-stream.  In this case, the existing `kdrc_codestream' object
         will be shared, rather than created afresh.  Otherwise, the
         `kdrc_codestream' object should have been created by the caller
         and passed in here as `new_cs'. */
    void init(jpx_codestream_source stream, jpx_layer_source layer,
              jpx_source *jpx_src, bool alpha_only, kdrc_stream *sibling,
              int log2_overlay_segment_size);
      /* Use this form of the `init' function to open a codestream within
         a JPX/JP2 compatible data source and associate all relevant
         rendering information.  The function will configure the object to
         produce both colour channels and an alpha channel (if one is
         available).  May generate an error through `kdu_error' if there is
         something wrong with the data source, or if the required colour
         rendering will not be possible.  The interpretation of `sibling'
         is discussed above, with the first form of the `init' function. */
    void init(mj2_video_source *track, int frame_idx, int field_idx,
              kdrc_stream *sibling);
      /* Use this form of the `init' function to open a codestream within
         an MJ2 video track and associate all relevant rendering information.
         The function configures the object to produce both colour channels and
         an alpha channel (if one is available).  May generate an error
         through `kdu_error' if there is something wrong with the data
         source, or if the required colour rendering will not be possible.
         The interpretation of `sibling' is discussed above, with the first
         form of the `init' function. */
    void set_error_level(int error_level);
      /* Call this function at any time after `init'.
         The `error_level' identifies the level of error resilience
         with which the code-stream is to be parsed.  A value of 0 means use
         the `fast' mode; 1 means use the `fussy' mode; 2 means use the
         `resilient' mode; and 3 means use the `resilient_sop' mode.   */
    void set_max_quality_layers(int max_quality_layers)
      { this->max_display_layers = max_quality_layers; }
      /* Note that this function has no effect until a new rendering cycle
         is started.  To ensure that the new rendering cycle starts
         immediately, you should call `invalidate_surface'. */
    int set_mode(int single_component,
                 kdu_component_access_mode access_mode);
      /* Use this to put the object into or take it out of single component
         mode.  If `single_component' < 0, the object will be taken out of
         single component mode.  Otherwise, it will be put into single component
         mode.
            In single component mode, `access_mode' indicates how the image
         component is to be interpreted.  This argument is disregarded if
         `single_component' is negative, since then we are always interested
         in output components, rather than codestream components.
            After this call, you must invoke `set_scale' to install a valid
         scale.
            If `single_component' is -ve, the function always returns -1.
         Otherwise, it returns the actual index of the component to be
         decompressed, which may be different to `single_component' if
         that value was too large. */
    void set_thread_env(kdu_thread_env *env, kdu_thread_queue *env_queue);
      /* Called if `kdu_region_compositor::set_thread_env' changes something. */
    void change_frame(int frame_idx);
      /* Use this function to change the codestream which is actually
         rendered, for streams which were initialized with an `mj2_video_source'
         object.  This is equivalent to first destroying the existing stream
         and then recreating and initializing it with the new frame index,
         but the same field and interlacing parameters which were originally
         supplied to `init'.  It is not necessary to call `set_scale'
         or `set_buffer_surface' again, if a valid scale was already
         installed. */
    int get_num_channels() { return mapping.num_channels; }
    int get_num_colours() { return mapping.num_colour_channels; }
    int get_components_in_use(int *indices);
      /* Returns the total number of image components which are in use,
         writing their indices into the supplied array, which must have
         space to accommodate at least 4 entries to be guaranteed that
         overwriting will not occur.  If the `component_access_mode' is
         `KDU_WANT_CODESTREAM_COMPONENTS' the function returns a -ve
         integer, -N, where N is the number of image components in use. */
    bool is_alpha_premultiplied() { return alpha_is_premultiplied; }
    int get_primary_component_idx() { return active_component; }
    kdrc_overlay *get_overlay() { return overlay; }
    kdu_dims set_scale(kdu_dims full_source_dims, kdu_dims full_target_dims,
                       kdu_coords source_sampling,
                       kdu_coords source_denominator,
                       bool transpose, bool vflip, bool hflip, float scale,
                       int &invalid_scale_code);
      /* Called after initialization and whenever the image is zoomed,
         rotated, etc.  After a call to this function, `set_buffer_surface'
         must also be called.
           `full_target_dims' identifies the location and size of the full
         image region as it would appear on the compositing grid at
         full size, without any transposition, rotation or scaling.  If the
         size of this region is 0, it will automatically be assigned
         the same size as `full_source_dims', after first assigning a default
         size for `full_source_dims' (if it is zero on entry) in the manner
         described below.
            After an initially empty `full_target_dims' is assigned the same
         size as `full_source_dims', the location of the default target
         region is assigned based upon the values found in
         `full_target_dims.pos' on entry.  Specifically, if
         `full_target_dims.pos.x' is zero on entry, the default assigned target
         region is also anchored at x=0; otherwise it is anchored at
         1-`full_target_dims.size.x' so that the right edge of the assigned
         target region is aligned at x=0.  The same policy is used to align
         the vertical edge of an assigned target region with y=0 at the top
         or bottom, depending on whether `full_target_dims.pos.y' was zero
         or not on entry.  This alignment policy is required to ensure that
         the default assigned target region would have its top-left corner
         anchored at the origin if interpreted after the application of
         "imagery layer"-specific orientation flags passed to
         `kdu_region_compositor::add_ilayer' and related functions.
            `full_source_dims' identifies the location and size of the complete
         source region to be used in generating the target image region.  This
         region is expressed relative to the source image (i.e., current
         code-stream image) upper left hand corner, but at a scale in which
         each code-stream canvas grid point occupies
         `source_sampling.x'/`source_denominator.x' horizontal source grid
         points and `source_sampling.y'/`source_denominator.y' vertical source
         grid points.  These sampling factors are normally both equal to 1,
         unless the code-stream is used by a JPX compositing layer whose
         embedded code-stream registration box specifies non-trivial sampling
         factors.
            If `full_source_dims' has zero size, it will automatically be
         set to the full canvas dimensions of the current source image,
         scaled up by the sampling factors given by `source_sampling' and
         `source_denominator'.  For MJ2 sources, the `full_source_dims'
         argument is always treated as if it were empty on entry.
            Note carefully that none of the `full_source_dims',
         `full_target_dims', `source_sampling' or `source_denominator'
         arguments are sensitive to the effects of image re-orientation and
         dynamic scaling, so their values will be the same each time this
         function is called, regardless of the values taken by the remaining
         arguments.
           In the special case where the object is in single component mode
         or it was initialized with a raw codestream (as opposed to a JPX
         stream interface or an MJ2 track), the source dimensions (as
         operated on by `full_source_dims', `source_sampling' and
         `source_denominator') are interpreted using the resolution of the
         first colour channel's image component, as opposed to the
         high resolution codestream canvas -- these are the same only if
         the first colour channel has sub-sampling factors of (1,1).
           `vflip, `hflip' and `transpose' identify additional geometric
         transformations which are to be applied on the compositing grid.
           `scale' represents the amount by which the full target dimensions
         are to be stretched or shrunk down during the rendering process.
           The function attempts to approximate the required scaling
         transformations by a reasonable combination of resolution level
         discarding and resampling.  It returns the actual location and
         dimensions of the region which can be painted on the compositing
         grid, accounting for all geometric changes associated with the
         last four arguments.
           If the requested scale is too small, too large, or the required
         flipping cannot be performed, the function returns an empty region,
         identifying the specific problem by setting one or more flags in
         the `invalid_scale_code' argument.  */
    void find_supported_scales(float &min_scale, float &max_scale,
                               kdu_dims full_source_dims,
                               kdu_dims full_target_dims,
                               kdu_coords source_sampling,
                               kdu_coords source_denominator);
      /* On entry, `min_scale' and `max_scale' hold what is know so far about
         the minimum and maximum scales which can be supported by the current
         configuration; a negative value means that nothing is known.  The
         last four parameters are supplied only to check whether or not
         conditions have changed since supported scale information for this
         stream was last deduced; the supported scales are actually only
         calculated inside `set_scale'; if conditions have changed, this
         function will have no impact on the supplied `min_scale' and
         `max_scale' values. */
    float find_optimal_scale(float anchor_scale, float min_scale,
                             float max_scale, bool avoid_subsampling,
                             kdu_dims full_source_dims,
                             kdu_dims full_target_dims,
                             kdu_coords source_sampling,
                             kdu_coords source_denominator);
      /* Does the work of `kdu_region_compositor::find_optimal_scale'. */
    void get_component_scale_factors(kdu_dims full_source_dims,
                                     kdu_dims full_target_dims,
                                     kdu_coords source_sampling,
                                     kdu_coords source_denominator,
                                     double &scale_x, double &scale_y);
      /* Does the work of `kdu_region_compositor::get_component_info'. */
    bool find_min_max_jpip_woi_scales(double min_scale[], double max_scale[]);
      /* Returns false without doing anything unless the object has a valid
         scale installed.  If it does, the function returns true after
         modifying the supplied `min_scale' and `max_scale' values so as
         to ensure that a compatible JPIP request, which scales the total
         composition dimensions accordingly when forming its "fsiz" request
         field, will include just the resolution levels which are required
         from the underlying codestream.
           More specifically, each of the two arguments is a 2-element array
         whose values correspond to a JPIP "fsiz" request which specifies
         "round-to-nearest" (first entry) and "round-up" (second entry),
         respectively.  A minimum scale value for the current codestream is
         computed as the smallest amount by which the current total image
         composition dimensions may be scaled without causing any of the
         codestream resolution levels which are in use to be discarded by a
         server.  Similarly, a maximum scale value is computed as the largest
         amount by which the current total image composition dimensions may be
         scaled without causing redundant information to be sent by the server
         for the current codestream.  These values are computed for each of
         the rounding cases and used to adjust the values of `min_scale' and
         `max_scale' supplied on entry, if necessary -- `min_scale' may be
         adjusted upward, while `max_scale' values may be adjusted downward.
         When the function is applied to all codestreams in a composition, it
         is possible that the `min_scale' and `max_scale' values are
         incompatible, in which case the minimum scale will be selected
         and some redundant data may be sent. */
    bool is_scale_valid() { return have_valid_scale; }
      /* Returns false if `set_scale' has not yet been called, or if the
         scale set in the last call to `set_scale' turns out to be unattainable.
         This might not be caught until a later call to `process' in which
         a tile-component is encountered whose number of resolution levels
         is too small to support the small rendering size requested. */
    kdu_dims find_composited_region(bool apply_cropping);
      /* If `apply_cropping' is true, this function just returns the region
         occupied by the stream on the compositing surface.  Otherwise, it
         returns the region which would be occupied by the stream on the
         compositing surface if it had not been cropped down. */
    void get_packet_stats(kdu_dims region, kdu_long &precinct_samples,
                          kdu_long &packet_samples,
                          kdu_long &max_packet_samples);
      /* Used to implement `kdu_region_compositor::get_codestream_packets',
         this function scans the precincts which contribute to the
         reconstruction of `region' (expressed on the composition canvas),
         returning: 1) the sum of their contributing areas via
         `precinct_samples' (this is figured out using
         `kdu_resolution::get_precinct_area'); 2) the sum of their
         contributing packets (from `kdu_resolution::get_precinct_packets')
         scaled by the corresponding precinct areas; and 3) the sum of
         the maximum possible number of contributing packets (from
         `kdu_tile::get_num_layers') scaled by the corresponding precinct
         areas. */
    void set_buffer_surface(kdu_compositor_buf *buffer,
                            kdu_dims compositing_region,
                            bool start_from_scratch);
      /* Called when the buffered region is panned, or when the buffer must
         be resized.  Note that `compositing_region' expresses the region
         represented by the buffer on the compositing grid, which must be
         offset by `compositing_offset' in order to find the region on the
         codestream's rendering grid.  If `start_from_scratch' is true, no
         previously generated data can be re-used, so generation of content
         for the new buffer surface must start from scratch. */
    kdu_dims map_region(kdu_dims region, bool require_intersection);
      /* Does the work of `kdu_region_compositor::map_region'.  If the
         region does not intersect with this ilayer and `require_intersection'
         is true, the function returns an empty region.  The function also
         returns an empty region is a valid scale is not installed.  Otherwise,
         it returns the coordinates of the region, mapped to the codestream's
         high resolution canvas, but adjusted so that the position is
         expressed relative to the upper left hand corner of the image
         region on the codestream canvas. */
    kdu_dims inverse_map_region(kdu_dims region);
      /* Does the work of `kdu_region_compositor::inverse_map_region'.  The
         supplied region is expressed on the codestream's high resolution
         canvas, but adjusted so that the position is expressed relative to
         the upper left hand corner of the image region on the codestream
         canvas.  The returned region is expressed in the composition
         coordinate system.  If the supplied `region' is empty, the region
         is considered to correspond to the entire image region on the
         codestream's high resolution grid. */
    bool process(int suggested_increment, kdu_dims &new_region,
                 int &invalid_scale_code);
      /* Gives this object the opportunity to process some more imagery,
         writing the results to the buffer surface supplied via
         `set_buffer_surface'.  The function may need to configure a new
         region in process first.  Upon return, it will generally update the
         `fraction_left' member to reflect the actual fraction of the imagery
         which has still to be generated.  This allows the caller to
         allocate processing resources to those code-streams which are
         furthest behind in their processing.
            The function returns with `new_region' set to the new rectangular
         region which has just been processed, expressed with respect to
         the compositing grid.  If this region is non-empty, the caller may
         attempt to update the final composited image based upon the new
         information.
            The function may return false only if the number of DWT levels
         in some tile-component is found to be too small to accommodate the
         current scale factor, or any required flipping cannot be performed.
         In either case, the cause of the problem is identified via the
         `invalid_scale_code' argument.  The returned code contains the
         logical OR of one or more of the single-bit flags,
         `KDU_COMPOSITOR_SCALE_TOO_SMALL' and `KDU_COMPOSITOR_CANNOT_FLIP'.
            The function may catch and rethrow an integer exception generated
         by a `kdu_error' handler, if some other processing error occurs. */
    void stop_processing()
      {
        if (processing)
          { decompressor.finish(); processing=codestream->in_use=false; }
      }
    bool can_process_now()
      { // Returns true if processing is not complete, and no other user of
        // the same code-stream resource is currently processing
        return (!is_complete) && (processing || !codestream->in_use);
      }
    void invalidate_surface();
      /* Called after `set_scale' to terminate all processing and invalidate
         all processed data.  Also called from `kdu_region_compositor::refresh'
         to force regeneration of the buffer surface. */
    int find_non_pending_rects(kdu_dims region, kdu_dims rects[]);
      /* Used by `kdrc_refresh::adjust' and
         `kdu_region_compositor::find_completed_rects', this function checks
         to see whether all or part of the supplied `region' will be
         regenerated as a result of future calls to this object's `process'
         function.  There may be up to 6 rectangular subsets of `region'
         which will not be regenerated in this way; these are written to
         the `rects' array and the number of such rectangles is the function's
         return value.  Accordingly, `rects' should be large enough to
         accommodate up to 6 entries. */
  private: // Helper functions
    void configure_subsampling();
      /* This function is called from within `init' and `set_mode' to
         fill out the `active_subsampling' array.  This array holds up to
         33 entries, corresponding to each possible number of discarded
         resolution levels d from 0 to `max_discard_levels'.  The value of
         `max_discard_levels' cannot possibly exceed 32.
            If `single_component' >= 0, both arrays hold the sub-sampling
         factors for this one component.  Otherwise, `active_subsampling'
         holds the subsampling factors for the `reference_component'. */
#define KDRC_PRIORITY_SHIFT 8
#define KDRC_PRIORITY_MAX (1<<KDRC_PRIORITY_SHIFT)
    void update_completion_status()
      { /* Updates the `is_complete' and `priority' members based upon the areas
           of the `active_region', `valid_region', `incomplete_region' and
           `region_in_process' members. */
        if ((!active_region) || (active_region == valid_region))
          { is_complete = true; priority = 0; return; }
        is_complete = false;
        kdu_long active = active_region.area();
        kdu_long valid = valid_region.area();
        if (processing)
          valid += (region_in_process.area() - incomplete_region.area());
        if (active == 0)
          priority = 0;
        else
          priority = (int)(((active-valid)<<KDRC_PRIORITY_SHIFT)/active);
      }
  private: // Information passed into constructor or `init'
    kdu_region_compositor *owner;
    bool persistent;
    bool alpha_only;
    bool alpha_is_premultiplied;
    int cache_threshold;
    mj2_video_source *mj2_track; // NULL if not initialized for video
    int mj2_frame_idx; // 0 if `mj2_track'=NULL
    int mj2_field_idx; // 0 if `mj2_track'=NULL
  private: // Multi-threading state
    kdu_thread_env *env;
    kdu_thread_queue *env_queue;
  private: // Members which are unaffected by zooming, panning or rotation
    kdrc_overlay *overlay; // Created if `init' is supplied a JPX data source
    int max_display_layers; // Max layers to be used in decompression
    kdu_channel_mapping mapping;
    kdu_region_decompressor decompressor;
    int single_component; // -ve unless in single-component mode
    int reference_component; // Index of first colour component in `mapping'
    int active_component; // single_component (if >= 0) or reference_component
    kdu_component_access_mode component_access_mode; // This member will equal
       // `KDU_WANT_OUTPUT_COMPONENTS' except possibly in single component mode
    kdu_coords active_subsampling[33]; // See `configure_subsampling'
    int max_discard_levels; // May change as we process more tiles
    bool can_flip; // May change as we process more tiles
  private: // Members which are affected by zooming and rotation
    bool have_valid_scale; // False until first call to `set_scale'
    bool transpose, vflip, hflip; // Identifies current rotation settings
    float scale; // Identifies the current scale
    kdu_dims full_source_dims; // Value supplied in last call to `set_scale'
    kdu_dims full_target_dims; // Value supplied in last call to `set_scale'
    kdu_coords source_sampling; // Value supplied in last call to `set_scale'
    kdu_coords source_denominator; // Value supplied in last call to `set_scale'
    int discard_levels; // Number of levels to discard
    kdu_coords expansion_numerator;   // Numerator and denominator for rational
    kdu_coords expansion_denominator; // reference component expansion factors
    kdu_dims rendering_dims; // Full source image region w.r.t. rendering grid
    kdu_coords compositing_offset; // Rendering origin - compositing origin
    float max_supported_scale; // -1 if not known; these parameters are updated
    float min_supported_scale; // each time the source/target params change
  private: // Members which relate to panning of the buffered surface
    kdu_coords buffer_origin; // Expressed on rendering grid
    kdu_dims active_region; // `rendering_dims' intersected with buffer region
    kdu_compositor_buf *buffer; // Points to first pixel at `buffer_origin'
    bool little_endian; // If machine word order is little-endian
  private: // Members used to keep track of incremental processing
    kdu_dims valid_region; // Subset of `active_region' known to have valid data
    bool processing;
    kdu_dims region_in_process; // Full region being processed (rendering grid)
    kdu_dims incomplete_region; // Remaining portion of `region_in_process'
    kdu_dims partially_complete_region; // Sub-rect of `region_in_process' which
               // covers regions produced by `kdu_region_decompressor::process'.
  public: // Public identification information
    kdu_istream_ref istream_ref;
    int codestream_idx; // Index of the code-stream within the source
    int colour_init_src; // See below
  public: // Public links and progress info
    kdrc_layer *layer; // Streams are always attached to a containing layer
    int priority; // Helps compositor decide which stream needs more processing
    bool is_complete; // When we have finished processing the active region
    bool is_active; // If and only if `layer' is on `owner's active layer list
    kdrc_stream *next; // Next in `kdu_compositor::streams' list.
    kdrc_codestream *codestream;
    kdrc_stream *next_codestream_user; // Next object using same code-stream
    kdrc_stream *prev_codestream_user; // Previous object using same code-stream
  };
  /* Notes:
       This object provides the internal representation for what is known
     as an "istream" within the documentation of `kdu_region_compositor'.
     Multiple istreams can share a single codestream, which is managed via
     the `codestream' interface.  Every "istream" necessarily belongs to
     exactly one "ilayer", which is represented internally by `kdrc_layer'.
       All istreams belong to a single `streams' list managed by the
     `owner' object.  Every istream is also attached to exactly one
     `kdrc_layer' object (i.e., `layer' is never NULL).  Those which are
     attached to active layers have their `is_active' member true, while
     those attached to inactive layers have their `is_active' member false.
       To understand the way this object works, you must first understand that
     there are several coordinate systems at work simultaneously:
     1) There is one global "compositing grid".  Most coordinates passed
        across this object's functions are expressed in terms of the global
        compositing grid.
     2) Each codestream has its own "rendering grid" whose scale matches that
        of the compositing grid, but the origin of the codestream's rendering
        grid is displaced relative to that of the compositing grid by the
        amount stored in the `compositing_offset' member.  Most internal
        coordinates are expressed in terms of the rendering grid.  The
        interpretation of the rendering grid is exactly the same as that
        adopted by the `kdu_region_decompressor' object.
     3) Each codestream has its own high-resolution canvas against which
        all component samples are registered.  The dimensions of the canvas
        are fixed, while those of the compositing and rendering grids change
        as a function of zoom and rotation parameters.  The rendering
        grid is related to the codestream canvas as follows.  We first find
        the dimensions of the reference component after any discarded
        resolution levels have been taken into account; these dimensions
        are related to those of the high-res canvas by sub-sampling by the
        relevant component sub-sampling factors and the number of discarded
        DWT levels.  We apply any required rotation/flipping to these
        dimensions and then expand by the factors stored in the `expansion'
        member.
       The `rendering_dims' region represents the portion of the code-stream
     image (expressed on the rendering grid) which is to be used for
     compositing.  In most cases, this will be the entire image, but
     some compositing rules use a cropped version of the image.
       The `active_region' represents the portion of the `rendering_dims'
     region which lies on the current buffer surface.  This is also
     expressed with respect to the rendering grid.
       The `valid_region' member identifies the subset of `active_region'
     for which image samples are known to have been generated.
       If this object was originally initialized using an MJ2 or JPX file
     format specification (i.e., not for raw codestream access), the
     `colour_init_src' member is set to identify the JPX compositing
     layer index or the MJ2 track index (zero based) from which the
     colour description, channel bindings and related information were
     retrieved during initialization.  Otherwise, this member is set to -1.
     It is possible for an object to be initialized with valid file format
     specifications (i.e., non-negative `colour_init_src') but subsequently
     to be put into single component mode via `set_mode'.  The object can be
     returned to its original use for disciplined colour rendering, so long
     as the `colour_init_src' member is consistent with the intended purpose.
  */

/******************************************************************************/
/*                                 kdrc_layer                                 */
/******************************************************************************/

class kdrc_layer {
  public: // Member functions
    kdrc_layer(kdu_region_compositor *owner);
    ~kdrc_layer();
    void init(jpx_layer_source layer, kdu_dims full_source_dims,
              kdu_dims full_target_dims, bool transpose, bool vflip,
              bool hflip);
      /* Does all the work of `kdu_region_compositor::add_compositing_layer'.
         May generate an error through `kdu_error' if the layer cannot be
         rendered.
            If one or more of the required codestream main headers is not
         yet available, the function does not actually create any
         codestreams.  In this case, the layer will be entered on the
         active list, but the `get_stream' function will return NULL regardless
         of its argument.  Once more information becomes available in a
         dynamic cache, you may be able to complete the initialization
         process by invoking `reinit'.
            The `full_source_dims' argument identifies the portion of this
         layer which is to be used for compositing.  This region is expressed
         on the layer's own reference grid, relative to the upper left hand
         corner of the layer image on that grid.  The layer reference grid
         is often identical to the high resolution canvas of its code-stream
         (or code-streams).  More generally, however, the layer reference
         grid is related to the code-stream canvas grids by the information
         returned via `jpx_layer_source::get_codestream_registration'.
            The `full_target_dims' argument identifies the region of the
         composited image onto which the source region should be composited
         (scaling may be involved).  These coordinates are expressed relative
         to the compositing reference grid which would be used if the
         image were rendered with a scale of 1 and without any rotation,
         flipping or transposition. */
    void init(mj2_video_source *track, int frame_idx, int field_handling,
              kdu_dims full_source_dims, kdu_dims full_target_dims,
              bool transpose, bool vflip, bool hflip);
      /* Use this version of the `init' function to add imagery layers
         which are tracks from an MJ2 data source.  The `frame_idx' and
         `field_handling' arguments have the same interpretation as their
         namesakes, passed into `add_compositing_layer'.
            The `full_target_dims' argument should already satisfy the
         conditions expected by `kdrc_stream::set_scale', meaning that it
         has been adjusted to reflect the target image region prior to the
         effects of `transpose', `vflip' and `hflip' and if it is an empty
         region, `full_target_dims.pos' has been configured to identify
         which edge (top/bottom, left/right) of a default target region
         should be aligned at the origin. */
    void init(int codestream_idx, int component_idx,
              kdu_component_access_mode codestream_access_mode,
              kdu_dims full_source_dims, kdu_dims full_target_dims,
              bool transpose, bool vflip, bool hflip);
      /* Use this version of the `init' function to add imagery layers
         which use a codestream directly.  If `component_idx' < 0, the
         we want to use the codestream as an image (potentially with
         multiple image components); otherwise, we want to render only
         a single image component. */
    bool reinit()
      {
        if (streams[0] == NULL)
          {
            if (jpx_layer.exists())
              init(jpx_layer,full_source_dims,full_target_dims,
                   init_transpose,init_vflip,init_hflip);
            else if (mj2_track != NULL)
              init(mj2_track,mj2_frame_idx,mj2_field_handling,full_source_dims,
                   full_target_dims,init_transpose,init_vflip,init_hflip);
            else if (direct_codestream_idx >= 0)
              init(direct_codestream_idx,direct_component_idx,
                   direct_access_mode,full_source_dims,
                   full_target_dims,init_transpose,init_vflip,init_hflip);
            else
              assert(0);
          }
        return (streams[0] != NULL);
      }
      /* Call this function if the original call to `init' did not complete
         due to one or more missing codestream main headers (should only
         happen with MJ2). */
    void disconnect_stream(kdrc_stream *obj)
      {
        for (int s=0; s < num_streams; s++)
          if (streams[s] == obj)
            { streams[s] = NULL; obj->layer = NULL; }
      }
    bool change_frame(int frame_idx, bool all_or_nothing);
      /* This function may be called to change only the frame associated with
         an object initialized using an MJ2 video source.  If the required
         codestream(s) can be opened the relevant calls to
         `kdrc_stream::change_frame' are made.  Otherwise, the
         `mj2_pending_frame_change' flag is set, deferring the actual frame
         change until the next call to this function or to `set_scale' or
         `kdu_region_compositor::refresh'.  If `streams[0]' is NULL on
         entry, the object was never completely initialized, so this function
         simply invokes `reinit', after setting up the correct frame index.
         The function returns true if it succeeds in making `frame_idx' the
         new frame index to be rendered within this layer, even if this
         required no actual change in the frame index.  If
         `all_or_nothing' is true, the function does not actually invoke
         the `kdrc_stream::change_frame' function on any codestream until
         it is sure that all required codestreams can be opened. */
    bool change_frame()
      { // Try 1st form of `change_frame' again, if change is still pending
        if (!mj2_pending_frame_change) return false;
        return change_frame(mj2_frame_idx,false);
      }
    void activate(kdu_dims full_source_dims, kdu_dims full_target_dims,
                  bool init_transpose, bool init_vflip, bool init_hflip,
                  int frame_idx, int field_handling);
      /* Call this function when moving an existing layer onto the active
         list.  It is assumed that the caller will be marking the entire
         composition as invalid so that a new set of calls to `set_scale'
         and `set_buffer_surface' will be invoked before any processing
         takes place. */
    void deactivate();
      /* Call this function when moving a layer from the active list to the
         inactive list.  Amongst other things, it marks each attached stream
         as inactive. */
    kdu_dims set_scale(bool transpose, bool vflip, bool hflip, float scale,
                       int &invalid_scale_code);
      /* Sets the orientation and scale at which the image is to be composed.
         The interpretation of these parameters is identical to that described
         in connection with `kdu_region_compositor::set_scale'.
            The function returns the full region which will be occupied by this
         layer on the compositing grid.  If this region is empty, the
         `scale' parameter may have been too small or the requested flipping
         might not be possible.  These conditions are identified by setting
         one or more flags in the `invalid_scale_code' argument.  Note that
         the scale may subsequently be found to be too small, or flipping
         may be found to be impossible while actually processing the imagery.
         This is because it may not be until then that a tile-component
         with insufficient DWT levels is encountered.
            If the call to `init' did not succeed, due to the lack of
         availability of codestream headers, the present function simply
         returns an empty region, with `invalid_scale_code' set to 0 */
    void find_supported_scales(float &min_scale, float &max_scale);
      /* Adjusts `min_scale' and `max_scale' as required to reflect any
         impact the current layer has on the minimum and maximum supported
         scale values.  On entry, a -ve value means that nothing is known
         about the relevant scale bound.  The information returned by this
         function depends upon a previous invocation of `set_scale'; if
         bounds have not yet been encountered in the process of executing
         `set_scale', they will not be reflected in the returned
         information. */
    float find_optimal_scale(float anchor_scale, float min_scale,
                             float max_scale, bool avoid_subsampling);
      /* Used to implement `kdu_region_compositor::find_optimal_scale'. */
    void get_component_scale_factors(kdrc_stream *stream,
                                     double &scale_x, double &scale_y);
      /* Used to implement `kdu_region_compositor::get_codestream_info'. */
    void set_buffer_surface(kdu_dims region, kdu_dims visible_region,
                            kdu_compositor_buf *compositing_buffer,
                            bool can_skip_surface_init,
                            kdrc_overlay_expression *overlay_dependencies);
      /* Called when the buffered region is panned, or when the buffer must
         be resized.  The `region' argument identifies the region occupied
         by the composited image buffer on the compositing grid.  If
         the present layer covers only a subset of the composited image, the
         layer's internal buffer surface may be set to a represent a smaller
         region.  The `visible_region' argument identifies the subset of the
         buffer `region' which is not completely covered by opaque
         compositing layers which are closer to the foreground than the
         current layer.  At most this region must actually be rendered for
         correct composition.  The `compositing_buffer' argument should be NULL
         if the present object's own rendering buffer is to be used as the
         composited image.  This happens if there is only one layer and it does
         not use alpha blending.  In this case, `compositing_row_gap' is
         ignored.
            The `can_skip_surface_init' argument indicates whether or
         not a newly allocated layer buffer (or layer buffer region) needs to
         be initialized -- generally this is required only for incremental
         rendering, where as-yet-unrendered portions of the image buffer might
         need to be displayed.  This argument is ignored if `compositing_buffer'
         is non-NULL. */
    void get_buffer_region(kdu_dims &region)
      { region = buffer_region; }
    kdu_compositor_buf *get_layer_buffer() { return buffer; }
      /* Used by `kdu_region_compositor' to get the rendering buffer of a
         single layer which does not require alpha compositing, so this buffer
         can be returned by `kdu_region_compositor::get_composition_buffer'
         instead of a separate compositing buffer.  This saves the work of
         copying everything to a separate buffer. */
    kdu_compositor_buf *take_layer_buffer(bool can_skip_surface_init);
      /* Used by `kdu_region_compositor::push_composition_buffer' if the
         surface buffer happens to be owned by a single compositing layer.
         This function takes ownership of the layer's buffer, forcing it to
         allocate a new one and invalidate its stream surfaces.  The function
         must not be called if there is a global compositing buffer -- an
         internal check is performed to be sure.  If `can_skip_surface_init'
         is true, initialization of the newly allocated surface buffer can
         be skipped.  This is possible if the layer buffer is being taken
         in order to push it onto the composited buffer queue, rather than
         turning it into a global composition buffer. */
    void change_compositing_buffer(kdu_compositor_buf *compositing_buffer)
      {
        assert(this->compositing_buffer != NULL);
        this->compositing_buffer = compositing_buffer;
      }
      /* Used by `kdu_region_compositor::push_composition_buffer' to reflect
         changes in the address of a global composition buffer into the
         present object.  The actual buffer surface region should not have
         changed since the last call to `set_buffer_surface'. */
    kdu_long measure_visible_area(kdu_dims region,
                                  bool assume_visible_through_alpha);
      /* This function measures the area of the subset of `region' within
         which the current compositing layer is visible.  If
         `assume_visible_through_alpha' is true, the layer is assumed to
         be hidden only by foreground layers which have no alpha channel.
         Otherwise, it is assumed to be hidden by any foregound data,
         regardless of alpha blending. */
    void get_visible_packet_stats(kdrc_stream *stream, kdu_dims region,
                                  kdu_long &precinct_samples,
                                  kdu_long &packet_samples,
                                  kdu_long &max_packet_samples);
      /* Used to implement `kdu_region_compositor::get_codestream_packets',
         this function accumulates contributions to its last three arguments
         from calls to `stream->get_packet_stats', based on those rectangular
         subsets of `region' which are considered to be visible.  These
         subsets are found by recursive application of the function, starting
         from the imagery layer which contains `stream'. */
    bool map_region(kdu_dims &region, kdrc_stream * &stream);
      /* Does the work of `kdu_region_compositor::map_region', invoking the
         `kdrc_stream::map_region' function on each relevant istream,
         performing the mapping and returning the matching istream via `stream'
         if successful. */
    float get_opacity(kdu_coords point);
      /* Returns the opacity of the current layer at location `point', the
         opacity being normalized to the range 0 to 1.  A value of 0 is
         returned if `point' does not belong to the region associated with
         this layer.  A value of 1 is returned if `point' does lie within
         the composited region associated with the layer and the layer is
         opaque.  For layers which are not opaque, the alpha channel of the
         layer is used to determine the opacity.  If the layer has not yet
         been fully rendered, so that the alpha channel at `point' is not
         yet known, the function will generally return 1.0, except
         possibly if the compositor has been configured to not initialize
         buffer surface -- e.g., when animating a video stream. */
    void get_overlay_info(int &total_roi_nodes, int &hidden_roi_nodes);
      /* Does the work of `kdu_region_compositor::get_overlay_info', augmenting
         the two supplied variables, as appropriate. */
    jpx_metanode search_overlay(kdu_coords point, kdrc_stream * &stream,
                                bool &is_opaque);
      /* Does the work of `kdu_region_compositor::search_overlays'.  Returns
         a non-empty interface if a matching metadata node is found.  Sets
         `is_opaque' to true if the current layer is opaque and contains
         `point'.  In this case, there is no point in searching for matches
         in lower layers. */
    void do_composition(kdu_dims region, kdu_int16 blending_factor_x128,
                        kdu_uint32 *erase_background);
      /* Called whenever new data is generated on any layer surface.  If
         `erase_background' is non-NULL, the `region' on the compositing
         buffer is first filled with the `erase_background' value, prior to
         compositing. */
    void configure_overlay(bool enable, int min_display_size,
                           int max_border_size,
                           kdrc_overlay_expression *dependencies,
                           const kdu_uint32 *aux_params,
                           int num_aux_params,
                           bool dependencies_changed,
                           bool aux_params_changed,
                           bool blending_factor_changed);
      /* Does the work of `kdu_compositor::configure_overlays'. */
    void update_overlay(bool start_from_scratch,
                        kdrc_overlay_expression *dependencies);
      /* Called when the overlay information might have changed for some
         reason.  Does nothing unless overlays are in use.
            If `start_from_scratch' is true, the function erases the
         overlay buffer and reads and paints all the overlay information
         from scratch.  If `start_from_scratch' is false, all existing overlay
         information is considered to remain valid and the function checks
         only to see if it needs to add to the overlay.
            One notable side effect of this function is that the arrival of
         new metadata may cause an overlay buffer to be created and then in
         turn may cause a separate composition buffer to be established in
         `kdu_compositor'.  If this happens, however, the new compositing
         buffer will be identical to the buffer which was last returned
         by `kdu_compositor::get_composition_buffer', so the application
         need not be aware of the change. */
    bool process_overlay(kdu_dims &new_region);
      /* Called from `kdu_compositor::process', this function returns true
         if any changes are made to the overlay surface, returning the
         affected region via `new_region'.  If a recent call to `update_overlay'
         specified the `start_from_scratch' and `delay_painting' options,
         this function will returns with `new_region' set to the region occupied
         by the entire buffer surface, since some previously painted metadata
         may need to be erased in this case. */
    bool get_unprocessed_overlay_seg(kdu_coords seg_idx, kdu_dims &seg_region)
      {
        if ((overlay == NULL) || !have_overlay_info) return false;
        return overlay->get_unprocessed_overlay_seg(seg_idx,seg_region);
      }
      /* This function returns false unless `seg_idx' holds the absolute index
         of an overlay segment, which is still waiting to be processed.  In
         the latter case, the function returns true and sets `seg_region'
         equal to the region which will be returned by `process_overlay' when
         the segment is eventually processed. */
    int get_num_streams() { return num_streams; }
    kdrc_stream *get_stream(int which)
      {
        if ((which < 0) || (which >= num_streams)) return NULL;
        return streams[which];
      }
    bool get_init_transpose() { return init_transpose; }
    bool get_init_vflip() { return init_vflip; }
    bool get_init_hflip() { return init_hflip; }
  private: // Helper functions
    void do_float_composition(kdu_dims region, float overlay_fact,
                              float *erase_background);
      /* This function does the work of `do_composition' for the case in which
         a floating point representation is being used.  The `overlay_fact'
         value is the direct linear multiplier applied to overlay alpha
         channel values.  The `erase_background' argument is either NULL, or
         an array with four background values, each in the range 0 to 1.0,
         corresponding to alpha, red, green and then blue. */
  private: // Members which are unaffected by global zoom, pan or rotation
    kdu_region_compositor *owner;
    jpx_layer_source jpx_layer; // Copied during the first form of `init'
    mj2_video_source *mj2_track; // Copied during the second form of `init'
    bool mj2_pending_frame_change; // See below
    bool init_transpose; // Copy of flag passed into `init'
    bool init_vflip; // Copy of flag passed into `init'
    bool init_hflip; // Copy of flag passed into `init'
    kdu_dims full_source_dims; // Copy of region passed into `init'
    kdu_dims full_target_dims; // Copy of region passed into `init'
    int num_streams; // Number of istreams used by this ilayer  
    kdrc_stream *streams[2]; // At most 2 streams per ilayer currently
    kdu_coords stream_sampling[2];    // JPX sources, the second one may be used
    kdu_coords stream_denominator[2]; // if alpha is in a separate codestream.
  private: // Members which are affected by zooming and rotation
    bool have_valid_scale; // If `set_scale' has been called successfully
    bool transpose, vflip, hflip; // Identifies current orientation settings
    float scale; // Identifies the current scale
    kdu_dims layer_region; // Region occupied by layer on compositing surface
  private: // Members which relate to panning of the layer's buffered surface
    kdu_compositor_buf *buffer; // Buffer to which codestream data is rendered
    kdu_dims buffer_region; // Visible region occupied by above buffer
    kdu_coords buffer_size; // Actual size of buffer
  private: // Members which are related to overlays
    kdrc_overlay *overlay;
    kdu_compositor_buf *overlay_buffer;
    kdu_coords overlay_buffer_size;
    int max_overlay_border; // Value supplied to `configure_overlay'
  private: // Members which relate to compositing
    kdu_compositor_buf *compositing_buffer; // Null if no compositing required
    kdu_dims compositing_buffer_region; // Region occupied by above buffer
  public: // Public identification information
    kdu_ilayer_ref ilayer_ref;
    int colour_init_src; // Same interpretation as in `kdrc_stream'
    int direct_codestream_idx; // -1 if not in direct codestream mode
    int direct_component_idx; // ignored if not in direct codestream mode
    kdu_component_access_mode direct_access_mode;
    int mj2_frame_idx; // Valid only if `mj2_track' is non-NULL
    int mj2_field_handling; // Copy of value passed to second form of `init'
  public: // Public links and other information
    bool have_alpha_channel; // True if either codestream will produce alpha
    bool alpha_is_premultiplied; // Meaningful only if `have_alpha_channel' true
    bool have_overlay_info; // True if there is non-trivial overlay info
    kdrc_layer *next; // active/inactive layer lists managed by `kdu_compositor'
    kdrc_layer *prev; // active list is doubly-linked
  };
  /* Notes:
          This object provides the internal information for what is known
       as an "imagery layer" (or "ilayer") to users of the
       `kdu_region_compositor' object.
          Each ilayer is served by at most two "istreams" (represented by
       `kdrc_stream' objects).  The `main_stream' object manages the colour
       channels and, if possible, alpha information.  The `aux_stream' object
       manages alpha information, if it is not already available from the
       `main_stream' object.  In the future, we may provide for up to four
       separate codestreams (one per colour channel) but this would require
       colour transformations to be implemented here, rather than in the
       underlying `kdu_region_decompressor' workhorse, which supports only
       one codestream.
          Each code-stream may have its own sub-sampling factors relating
       its canvas coordinates to those of a layer reference grid.  In fact,
       these sampling factors may be rational valued, being equal to the
       ratio between the `main_stream_sampling' and `main_stream_denominator'
       coordinates, or the `aux_stream_sampling' and `aux_stream_denominator'
       coordinates, as appropriate.  In many cases, however, the layer
       reference grid is identical to the high resolution canvas of one or
       both code-streams.  The sub-sampling factors indicate the number of
       layer grid points between each pair of canvas grid points.
          The `full_source_dims' member identifies the portion of the source
       image which is actually to be composited.  The coordinates
       are expressed, independent of any rotation or zooming factors, relative
       to the upper left hand corner of the image, as it appears on the layer
       grid (see above for the relationship between the layer grid and the
       high resolution canvas associated with each codestream).
          The `full_target_dims' member identifies the region of the composited
       image onto which the above source region is to be composited.  This
       region is expressed relative to the compositing grid which would be
       used if there were no scaling and no rotation.
          The `mj2_pending_frame_change' flag is true if the frame(s)
       represented by the current `streams' object(s) are not the correct ones.
       When a frame change is requested, but the required frames are not
       currently available from an MJ2 video source (not yet available in the
       dynamic cache), we leave the streams pointing to the codestreams
       associated with a previous frame so as not to interrupt video playback.
       However, each time `set_scale' or `kdu_region_compositor::refresh' is
       called, an attempt will be made to open the correct frame if it has
       since become available.  This is done using the `change_frame' function.
          The `layer_region' member identifies the full extent of the
       composited image region associated with this ilayer, expressed
       relative to the compositing grid, at the current scale and accounting
       for any rotation, flipping or transposition.
          The `layer_buffer_region' is always a subset of `layer_region'.  It
       identifies the region on the compositing grid which is currently
       buffered within the `layer_buffer'.
          The `compositing_buffer_region' member is ignored unless
       `compositing_buffer' is non-NULL.  In this case, imagery from this
       layer is to be composited onto the `compositing_buffer' when the
       `perform_composition' function is called.  The region occupied by the
       compositing buffer is also expressed relative to the compositing grid,
       but may be larger (no smaller) than the current `layer_buffer_region'.
       This is because some layers may occupy only a portion of the composited
       image. */

/******************************************************************************/
/*                             kdrc_refresh_elt                               */
/******************************************************************************/

struct kdrc_refresh_elt {
    kdu_dims region;
    kdrc_refresh_elt *next;
  };

/******************************************************************************/
/*                               kdrc_refresh                                 */
/******************************************************************************/

class kdrc_refresh {
  public: // Member functions
    kdrc_refresh() { free_elts = list = NULL; }
    ~kdrc_refresh()
      {
        kdrc_refresh_elt *scan;
        while ((scan=list) != NULL)
          { list = scan->next; delete scan; }
        while ((scan=free_elts) != NULL)
          { free_elts = scan->next; delete scan; }
      }
    bool is_empty() { return (list == NULL); }
    void add_region(kdu_dims region);
      /* Add a new region which needs to be refreshed.  You may add as many
         as you like; they may later be eliminated during calls to the
         `adjust' function. */
    bool pop_region(kdu_dims &region)
      { // Called from within `kdu_region_compositor::process'.
        kdrc_refresh_elt *result = list;
        if (result == NULL) return false;
        list = result->next; result->next = free_elts; free_elts = result;
        region = result->region;
        return true;
      }
    bool pop_largest_region(kdu_dims &region);
      /* Same as `pop_region' but pulls of the largest one. */
    bool pop_and_aggregate(kdu_dims &aggregated_region,
                           kdu_dims &new_region, kdu_long &aggregated_area,
                           float aggregation_threshold);
      /* This function is similar to `pop_region', except that it only pops
         regions which intersect with or adjoin `aggregated_region'. The
         `aggregated_area' argument holds the total area associated with
         aggregated regions from the refresh manager.  This value should not
         become smaller than `aggregated_region' * `aggregation_threshold'.
         If aggregating a region would violate this condition, the function
         returns false.
           For this function to work, `aggregated_region' should be non-empty
         and `aggregated_area' should be non-zero on entry -- that is, you
         should generally have invoked `pop_region' already to determine
         an initial region from which to start aggregating.
           Both the `aggregated_region' and `aggregated_area' arguments are
         updated prior to returning true. */
    void adjust(kdu_dims buffer_region);
      /* Called when the buffer surface changes.  Regions scheduled for
         refresh are intersected with the new buffer region, and removed if
         this leaves them empty. */
    void adjust(kdrc_stream *stream);
      /* After any change in the buffer surface, this function is passed each
         active stream in turn.  It passes through each of the elements on its
         refresh list, as it stood on entry, removing those elements and
         passing them to `stream->find_processed_rects', which determines the
         rectangular regions which need to be passed back to `add_region'.
         After passing through all the active streams in this way, we can be
         sure that the remaining regions will not be composited as a result
         of decompression processing; they must be explicitly refreshed. */
    void reset();
      /* Same as calling `adjust' with an empty region, but a little more
         efficient and more intuitive. */
  private: // Data
    kdrc_refresh_elt *free_elts;
    kdrc_refresh_elt *list; // List of regions which need refreshing
  };
  /* Notes:
       This object is used by `kdu_compositor' to keep track of regions which
       need to be re-composited and/or communicated to the application via
       `kdu_region_compositor::process', where this will not
       happen otherwise.  Regions managed by the refresh manager might not
       always need to undergo composition, but in most cases they do, so it
       is simplest to always compose them as they are popped.
          Regions may need to be added to the refresh manager where there is no
       overlaying layer imagery, which would normally drive the generation of
       processed regions to be returned by `kdu_region_compositor::process'.
       Regions may also need to be added to the refresh manager if a processed
       region gets decomposed into smaller segments, some of which are
       incomplete because they are covered by imagery which has not yet been
       processed; in this case, the completed portion may correspond to
       multiple rectangular regions, only one of which can be returned via
       `kdu_region_compositor::process' at a time, so the others are
       temporarily stored in the refresh manager. */
      

#endif // REGION_COMPOSITOR_LOCAL_H
