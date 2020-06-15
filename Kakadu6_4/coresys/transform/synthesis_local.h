/*****************************************************************************/
// File: synthesis_local.h [scope = CORESYS/TRANSFORMS]
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
   Local definitions used by "synthesis.cpp".  These are not to be included
from any other scope.
******************************************************************************/

#ifndef SYNTHESIS_LOCAL_H
#define SYNTHESIS_LOCAL_H

#include <assert.h>
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "transform_local.h"

// Defined here

class kd_synthesis;

/*****************************************************************************/
/*                               kd_synthesis                                */
/*****************************************************************************/

class kd_synthesis : public kdu_pull_ifc_base {
public: // Member functions
	kd_synthesis()
	{
		vert_source_vlines = NULL; vert_source_bufs = NULL;
		free_lines = NULL; vert_steps = hor_steps = NULL; queues = NULL;
		coeff_handle = NULL; icoeff_handle = NULL; source_buf_handle = NULL;
		line_handle = NULL; vert_step_handle = hor_step_handle = NULL;
		queue_handle = NULL; next_row_pos_handle = NULL;
	}
	void init(kdu_node node, kdu_sample_allocator *allocator,
		bool use_shorts, float normalization, int pull_offset,
		kdu_thread_env *env, kdu_thread_queue *env_queue);
protected: // These functions implement their namesakes in the base class
	virtual ~kd_synthesis();
	virtual void start(kdu_thread_env *env);
	virtual void pull(kdu_line_buf &line, kdu_thread_env *env);
protected: // Internal implementation functions
	int simulate_vertical_lifting(int support_max);
	/* See the corresponding function in `kd_analysis'. */
	void perform_vertical_lifting_step(kd_lifting_step *step,
		kdu_sample16 **src_ptrs,
		kd_vlift_line *vline_in,
		kd_vlift_line *vline_out);
	/* This function implements a single vertical lifting step, using
	   16-bit sample values.  Each line consists of up to two cosets,
	   whose dimensions and offsets are maintained in `coset_width_out'
	   and `coset_off_out', respectively.  The cosets of each source
	   line may be found in the `vert_source_vlines' buffer.  The `src_ptrs'
	   buffer has the same dimension and is used by the function to
	   temporarily record pointers to the actual source lines associated
	   with any given coset. */
	void perform_vertical_lifting_step(kd_lifting_step *step,
		kdu_sample32 **src_ptrs,
		kd_vlift_line *vline_in,
		kd_vlift_line *vline_out);
	/* Same as the first form of the function, except for working with
	   32-bit sample values. */
	void horizontal_synthesis(kd_vlift_line *line, int vert_parity,
		kdu_thread_env *env, bool post_vertical);
	/* Performs horizontal synthesis, pulling source subband lines from
	   their respective subband interfaces.  Uses `vert_parity' to
	   determine whether the line belongs to a vertically low- or
	   high-pass subband.  If horizontal decomposition is not involved,
	   this function simply retrieves the required line directly from
	   the relevant subband interface.
		  If `post_vertical' is true, this function is being called after
	   vertical synthesis.  In that case, no attempt is made to load subband
	   samples from the children nodes.  This can only happen if the
	   `vertical_first' flag is true, in which case calls to this function
	   with `post_vertical' false will not do any horizontal subband
	   transformation; they will only load the subband samples from child
	   nodes. */
protected: // Data
	kdu_pull_ifc subbands[4]; // Interfaces for acquiring subband samples
	bool reversible;
	bool initialized; // True after the first `pull' call.
	bool vert_xform_exists; // True if vert splitting performed at this node
	bool hor_xform_exists; // True if horiz. splitting performed at this node
	int num_vert_steps; // Number of lifting steps in the vertical direction
	int num_hor_steps; // Number of lifting steps in the horizontal direction
	kd_vlift_line **vert_source_vlines; // Sized for max vertical step support
	void **vert_source_bufs; // Sized for max vertical step support
	kd_vlift_line *free_lines; // Head of free list used by `queues'
	kd_lifting_step *vert_steps; // `num_vert_steps' entries
	kd_lifting_step *hor_steps; // `num_hor_steps' entries
	bool vert_symmetric; // If vertical kernel belongs to WS symmetric class
	bool hor_symmetric; // If horizontal kernel belongs to WS symmetric class
	bool vert_symmetric_extension; // If false, vert extension is 0-order hold
	bool hor_symmetric_extension; // If false, hor extension is 0-order hold
	int normalizing_upshift; // Amount to post-shift irreversible outputs.
	kd_vlift_queue *queues; // valid indices run from -1 to `num_vert_steps'-1
	int *step_next_row_pos; // Holds `num_vert_steps' entries
	int y_min_out, y_next_out, y_max_out;
	int y_min_in[2], y_next_in[2], y_max_in[2]; // One for each subband
	int x_min_buf; // Coordinate associated with start of all internal buffers
	int x_min_in; // Coordinate of first nominal input sample for hor. lifting
	int x_max_in; // Coordinate of last nominal input sample for hor. lifting
	int coset_width_in[2]; // From `x_min_in' to `x_max_in', in coset domain
	kdu_byte coset_off_in[2]; // From `x_min_buf' to `x_min_in' in coset domain
	kdu_byte request_offset[2]; // Offset supplied when requesting subband data
	int request_width[2]; // Number of samples pulled from subbands
	kdu_byte left_fill[2]; // See below
	kdu_byte right_fill[2]; // See below
	int x_min_out; // Coordinate of first required output sample
	int x_max_out; // Coordinate of last required output sample
	int coset_width_out[2]; // From `x_min_out' to `x_max_out', in coset domain
	int x_min_pull; // Coordinate associated with start of line in `pull' calls
	kdu_byte coset_off_out[2]; // `x_min_buf' to `x_min_out' in coset domain
	bool use_shorts; // All `pull' calls will request 16-bit samples
	bool unit_height;
	bool unit_width;
	bool empty;
	bool vertical_first; // If true, we will perform vertical synthesis first
private: // Handles to dynamically allocated arrays
	float *coeff_handle; // Array from which step `coeffs' arrays are allocated
	int *icoeff_handle; // Array from which step `icoeffs' arrays are allocated
	void **source_buf_handle; // Handle to array holding source buf/vline ptrs
	kd_vlift_line *line_handle; // Handle to array of allocated line buffers
	kd_lifting_step *vert_step_handle; // Handle to array containing vert steps
	kd_lifting_step *hor_step_handle; // Handle to array containing horiz steps
	kd_vlift_queue *queue_handle; // Handle to array of step input queues
	int *next_row_pos_handle; // Used to allocate `step_next_row_pos'
private: // Fixed versions of above arrays, for use with smaller kernels
	float coeff_store[8]; // Up to 8 lifting filter taps
	int icoeff_store[8]; // Up to 8 lifting filter taps
	void *source_buf_store[8]; // Up to 4 src lines in each vert lifting step
	kd_vlift_line line_store[6]; // Up to 6 line state buffers
	kd_lifting_step step_store[4]; // Up to 4 unique lifting steps
	kd_vlift_queue queue_store[5]; // Queues for up to 4 lifting steps
	int next_row_pos_store[4]; // Up to 4 row positions can be stored
};
/* Notes:
	  By and large, this object closely mirrors the `kd_analysis' object
   which performs the corresponding transform analysis operations.  The
   key difference is that additional member variables must be created
   here to manage the possibility that synthesis is restricted to a
   region of interest.  This complicates things for two main reasons:
   1) the synthesized region must generally be constructed from a larger
   number of subband samples, due to the spreading effect of the wavelet
   filters; and 2) in the face of such spreading, maintaining word
   alignment for all of the interacting line buffers becomes more
   challenging -- alignment is required for efficient implementation on
   SIMD architectures.  The solution to these problems which is adopted
   here is documented below.  Note that this solution differs significantly
   from that found in Kakadu version 4.5 and earlier, in part because
   subsequent versions need to support arbitrary transform kernels.
	  For vertical processing, we maintain separate ranges and current
   line counters for the output rows (returned in response to calls to
   `pull') and for each of the vertical input subbands.  If there is
   no vertical transform, `y_min_in', `y_next_in' and `y_max_in' are
   not used, since `y_min_out', `y_next_out' and `y_max_out' apply to
   both the lines returned in response to `pull' and the lines fetched
   from our own descendant subbands.
	  For horizontal processing, the situation is greatly complicated by
   alignment requirements.  For this reason, valid data within any given
   line might commence beyond the nominal start of the line.  Each line
   buffer has a nominal range of samples whose indices run from 0 to
   W-1, where W is the value returned by `kdu_line_buf::get_width'.
   Of course, the line can be (and is) allocated in such a way as to allow
   access to a prescribed number of samples before the nominal start of
   the line and a prescribed number of samples beyond the nominal end
   of the line.  The sample corresponding to index 0 is guaranteed to be
   aligned on a 16-byte boundary.  This alignment is beyond our control,
   being managed by the `kdu_sample_allocator' object; however, we want
   to ensure that all vertical and horizontal processing can be performed
   on aligned boundaries within the buffers.
	  The output lines produced by complete DWT synthesis represent a
   range of sample locations on the current node's canvas coordinate
   system, ranging from `x_min_out' to `x_max_out'.  Due to alignment
   constraints, the `x_min_out' location might not coincide with the
   first nominal sample in the line buffer.  For this reason, we maintain
   a separate record of the absolute location associated with the nominal
   start of the line buffer.  For all internal processing, this location
   is identified by the `x_min_buf' member, where `x_min_buf' <= `x_min_out'.
   For the `line' buffer supplied in calls to `pull', the location of
   the first sample in the line buffer is fixed by the parent object
   which calls us -- it is derived from the `pull_offset' argument
   supplied to the constructor.  We maintain this value in the `x_min_pull'
   member.  Since we are free to select `x_min_buf' we make sure that
   `x_min_pull' - `x_min_buf' is a multiple of 16 bytes, so that data
   transfers from internal to external line buffers can be fully aligned
   on 16-byte boundaries.
	  The `x_min_in' and `x_max_in' coordinates are expressed in the
   same coordinate system as `x_min_out' and `x_max_out'.  In particular,
   these are the tightest bounds which contain the locations of all
   relevant horizontal subband samples (low-pass subband samples in the
   even locations and high-pass samples in the odd locations).  These
   subband samples are actually kept in separate buffers, known as the
   coset buffers.  If the coset buffers were interleaved (even coset in
   the even locations, odd coset in the odd locations), the location
   corresponding to the nominal start of the interleaved buffer would
   be `x_min_buf'.  This association allows us to deduce the offset
   from the start of each coset buffer to the first sample which belongs
   to the `x_min_in' to `x_max_in' range.  These offsets are maintained
   in the `coset_off_in' member.  The number of samples within each
   coset which lie within the `x_min_in' to `x_max_in' range in the
   interleaved domain is maintained by `coset_width_in'.  Horizontal
   lifting is based upon these samples and any required boundary
   extension is also applied to these samples.
	  The `request_offset' member identifies the offset from the
   nominal start of each coset buffer to the first subband sample which
   is pulled from the relevant subband interface.  The number of
   samples which are pulled from horizontal low- and high-pass subband
   interfaces in each line is given by the `request_width' member.
   For symmetric least-dissimilar wavelet filters, these `request_offset'
   and `request_width' parameters define exactly the same region as
   `x_min_in' and `x_max_in'.  In general, however, there may be some
   samples in the `x_min_in' through `x_max_in' range which are not
   filled by subband `pull' calls.  These samples should not ultimately
   have any bearing on the data returned via the present object's `pull'
   interface.  However, it is possible that a restricted region of interest
   is not correctly traced through to the subbands where asymmetric
   transform kernels are used with symmetric boundary extension.  For
   this reason, and because floating point access to uninitialized data
   could cause exceptions, we always fill any missing samples in the
   `x_min_in' through `x_max_in' range by replication, prior to horizontal
   lifting.  The number of these fill samples required on the left boundary
   of each coset is given by `left_fill', while the number of fill samples
   required on the right boundary is given by `right_fill'. */

#endif // SYNTHESIS_LOCAL_H
