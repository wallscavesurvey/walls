/*****************************************************************************/
// File: analysis_local.h [scope = CORESYS/TRANSFORMS]
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
   Local definitions used by "analysis.cpp".  These are not to be included
from any other scope.
******************************************************************************/

#ifndef ANALYSIS_LOCAL_H
#define ANALYSIS_LOCAL_H

#include <assert.h>
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "transform_local.h"
#include "kdu_roi_processing.h"

// Defined here

class kd_analysis;

/*****************************************************************************/
/*                                kd_analysis                                */
/*****************************************************************************/

class kd_analysis : public kdu_push_ifc_base {
public: // Member functions
	kd_analysis()
	{
		vert_source_vlines = NULL; vert_source_bufs = NULL;
		free_lines = NULL; vert_steps = hor_steps = NULL; queues = NULL;
		coeff_handle = NULL; icoeff_handle = NULL; source_buf_handle = NULL;
		line_handle = NULL; vert_step_handle = hor_step_handle = NULL;
		queue_handle = NULL; next_row_pos_handle = NULL;
	}
	void init(kdu_node node, kdu_sample_allocator *allocator,
		bool use_shorts, float normalization,
		kdu_roi_node *roi, kdu_thread_env *env,
		kdu_thread_queue *queue);
protected: // These functions implement their namesakes in the base class
	virtual ~kd_analysis();
	virtual void push(kdu_line_buf &line, kdu_thread_env *env);
private: // Internal implementation functions
	int simulate_vertical_lifting(int support_max);
	/* This function simulates the vertical lifting steps so as to determine
	   the minimum number of line buffers which must be allocated.  The
	   simulation is based on the actual range of image rows for which
	   results must be calculated, except that the upper bound on this
	   range is reduced by an even integer, if necessary, so that the
	   range does not significantly exceed `support_max' -- there is
	   no point in simulating the vertical processing away from boundaries
	   where the queueing system is in equilibrium.  The function
	   will only be called if `num_vert_steps' is non-zero, and
	   `unit_height' is false. */
	void perform_vertical_lifting_step(kd_lifting_step *step,
		kdu_sample16 **src_ptrs,
		kd_vlift_line *vline_in,
		kd_vlift_line *vline_out);
	/* This function implements a single vertical lifting step, using
	   16-bit sample values. */
	void perform_vertical_lifting_step(kd_lifting_step *step,
		kdu_sample32 **src_ptrs,
		kd_vlift_line *vline_in,
		kd_vlift_line *vline_out);
	/* This function implements a single vertical lifting step, using
	   32-bit sample values. */
	void horizontal_analysis(kd_vlift_line *line, int vert_parity,
		kdu_thread_env *env, bool pre_vertical);
	/* Performs horizontal analysis, pushing the resulting horizontal subband
	   lines on to their respective subband interfaces.  Uses `vert_parity'
	   to determine whether the line belongs to a vertically low- or
	   high-pass subband.  If horizontal decomposition is not involved, this
	   function simply passes the supplied line directly out to the relevant
	   subband interface.
		  If `pre_vertical' is true, this function is being called before
	   vertical analysis.  In that case, no attempt is made to push the
	   generated horizontal subband samples to the child nodes.  This can
	   only happen if the `horizontal_first' flag is true, in which case
	   calls to this function with `pre_vertical' false will not do any
	   horizontal subband transformation; they will only push the subband
	   samples to the relevant child nodes. */
private: // Data
	kdu_push_ifc subbands[4];  // Interfaces for delivering subband samples
	bool reversible;
	bool use_shorts; // Using 16-bit (shorts) or 32-bit representations?
	bool initialized; // Final initialization happens in the first `push' call
	bool vert_xform_exists; // True if vert splitting performed at this node
	bool hor_xform_exists; // True if horiz. splitting performed at this node
	bool horizontal_first; // If true, we perform horizontal analysis first
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
	int normalizing_downshift; // Amount to pre-shift irreversible inputs.
	kd_vlift_queue *queues; // valid indices run from -1 to `num_vert_steps'-1
	int *step_next_row_pos; // Holds `num_vert_steps'+1 entries
	int y_min, y_max; // Range of row numbers
	int y_next; // Next input row number
	int x_min, x_max;
	int coset_width[2]; // Width of horizontal low- and high-pass bands
	bool unit_height; // Sequences of length 1 must be processed differently
	bool unit_width;
	bool empty; // True only if the resolution has no area.
	kdu_roi_level roi_level; // Keep this around only to allow destruction
							 // of ROI processing objects when done.
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
	float coeff_store[8]; // Up to 8 unique lifting filter taps
	int icoeff_store[8]; // Up to 8 unique lifting filter taps
	void *source_buf_store[8]; // Up to 4 src lines in each vert lifting step
	kd_vlift_line line_store[6]; // Up to 6 line state buffers
	kd_lifting_step step_store[4]; // Up to 4 unique lifting steps
	kd_vlift_queue queue_store[5]; // Queues for up to 4 lifting steps
	int next_row_pos_store[5]; // Up to 5 row positions can be stored
};
/* Notes:
	  This object is responsible for performing analysis operations for a
   single node in the DWT tree.  Vertical analysis is performed first,
   followed by horizontal analysis.  The implementation is incremental and
   aims to minimize memory consumption.  New lines of samples for the
   relevant node are pushed one by one via the `push' function.  This
   function invokes the vertical analysis logic, which in turn pushes any
   newly generated vertical subband lines to the `horizontal_analysis'
   function and thence on to the `hor_low' and `hor_high' interfaces.
	  `y_next' holds the coordinate (true canvas coordinates) of the
   next input row to be supplied via the `push' function.  The first and
   last valid input rows have coordinates `y_min' and `y_max'.
	  'x_min' and `x_max' identify the true canvas coordinates of the first
   and last sample in each input line.
	  To minimize memory fragmentation, all line buffers used by this
   object are allocated together from adjacent locations in a single memory
   block provided by the `kdu_sample_allocator' object.  The memory block
   is sized during construction and actual allocation is completed in the
   first `push' call, which is identified by the `initialized' flag being
   false. */

#endif // ANALYSIS_LOCAL_H
