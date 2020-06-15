/*****************************************************************************/
// File: synthesis.cpp [scope = CORESYS/TRANSFORMS]
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
   Implements the reverse DWT (subband/wavelet synthesis).  The implementation
uses lifting to reduce memory and processing, while keeping as much of the
implementation as possible common to both the reversible and the irreversible
processing paths.  The implementation is generic to the extent that it
supports any odd-length symmetric wavelet kernels -- although only 3 are
currently accepted by the "kdu_kernels" object.
******************************************************************************/

#include <assert.h>
#include <string.h>
#include "kdu_arch.h" // Include architecture-specific info for speed-ups.
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "kdu_kernels.h"
#include "synthesis_local.h"

// Set things up for the inclusion of assembler optimized routines
// for specific architectures.  The reason for this is to exploit
// the availability of SIMD type instructions on many modern processors.
#if ((defined KDU_PENTIUM_MSVC) && (defined _WIN64))
# undef KDU_PENTIUM_MSVC
# ifndef KDU_X86_INTRINSICS
#   define KDU_X86_INTRINSICS // Use portable intrinsics instead
# endif
#endif // KDU_PENTIUM_MSVC && _WIN64

#if (defined _WIN64) && !(defined KDU_NO_MMX64)
#  define KDU_NO_MMX64
#endif // _WIN64 && !KDU_NO_MMX64

#if defined KDU_X86_INTRINSICS
#  define KDU_SIMD_OPTIMIZATIONS
#  include "x86_dwt_local.h" // For 32-bit or 64-bit builds, GCC or .NET
#elif defined KDU_PENTIUM_MSVC
#  define KDU_SIMD_OPTIMIZATIONS
#  include "msvc_dwt_mmx_local.h" // Only for 32-bit builds, MSVC or .NET
#elif defined KDU_PENTIUM_GCC
#  define KDU_SIMD_OPTIMIZATIONS
#  include "gcc_dwt_mmx_local.h" // For 32-bit or 64-bit builds in GCC
#elif defined KDU_SPARCVIS_GCC
#  define KDU_SIMD_OPTIMIZATIONS
#  include "gcc_dwt_sparcvis_local.h" // Contains asm commands in-line
#elif defined KDU_ALTIVEC_GCC
#  define KDU_SIMD_OPTIMIZATIONS
#  include "gcc_dwt_altivec_local.h" // Contains Altivec PIM code in-line
#endif


/* ========================================================================= */
/*                               kdu_synthesis                               */
/* ========================================================================= */

/*****************************************************************************/
/*                        kdu_synthesis::kdu_synthesis                       */
/*****************************************************************************/

kdu_synthesis::kdu_synthesis(kdu_resolution resolution,
	kdu_sample_allocator *allocator,
	bool use_shorts, float normalization,
	kdu_thread_env *env, kdu_thread_queue *env_queue)
	// In the future, we may create separate, optimized objects for each kernel.
{
	kd_synthesis *obj = new kd_synthesis;
	state = obj;
	obj->init(resolution.access_node(), allocator,
		use_shorts, normalization, 0, env, env_queue);
}

/*****************************************************************************/
/*                        kdu_synthesis::kdu_synthesis                       */
/*****************************************************************************/

kdu_synthesis::kdu_synthesis(kdu_node node,
	kdu_sample_allocator *allocator,
	bool use_shorts, float normalization,
	int pull_offset, kdu_thread_env *env,
	kdu_thread_queue *env_queue)
	// In the future, we may create separate, optimized objects for each kernel.
{
	kd_synthesis *obj = new kd_synthesis;
	state = obj;
	obj->init(node, allocator, use_shorts, normalization, pull_offset, env, env_queue);
}


/* ========================================================================= */
/*                              kd_synthesis                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                           kd_synthesis::init                              */
/*****************************************************************************/

void
kd_synthesis::init(kdu_node node, kdu_sample_allocator *allocator,
	bool use_shorts, float normalization,
	int pull_offset, kdu_thread_env *env,
	kdu_thread_queue *env_queue)
{
	kdu_resolution res = node.access_resolution();
	this->reversible = res.get_reversible();
	this->use_shorts = use_shorts;
	this->initialized = false;
	this->free_lines = NULL;
	this->queues = NULL;
	this->vert_source_vlines = NULL;
	this->vert_source_bufs = NULL;
	this->vert_steps = this->hor_steps = NULL;
	this->num_vert_steps = this->num_hor_steps = 0;
	this->vert_symmetric_extension = this->hor_symmetric_extension = false;
	int kernel_id = node.get_kernel_id();
	int directions = node.get_directions();
	this->vertical_first = reversible &&
		((directions & (KDU_NODE_DECOMP_BOTH | KDU_NODE_TRANSPOSED)) ==
		(KDU_NODE_DECOMP_BOTH | KDU_NODE_TRANSPOSED));
	coeff_handle = NULL;
	icoeff_handle = NULL;
	source_buf_handle = NULL;
	line_handle = NULL;
	vert_step_handle = hor_step_handle = NULL;
	queue_handle = NULL;

	// Get output dimensions
	kdu_dims dims;  node.get_dims(dims);
	kdu_coords min, max;
	min = dims.pos; max = min + dims.size;  max.x--; max.y--;
	y_min_out = min.y;  y_max_out = max.y;
	x_min_out = min.x;  x_max_out = max.x;
	x_min_pull = x_min_out - pull_offset;
	x_min_buf = x_min_pull; // May need to change this later

	empty = !dims;
	unit_height = (y_min_out == y_max_out);
	unit_width = (x_min_out == x_max_out);

	if (empty)
		return;

	// Get basic structure
	kdu_node child[4] =
	{ node.access_child(LL_BAND), node.access_child(HL_BAND),
	 node.access_child(LH_BAND), node.access_child(HH_BAND) };

	assert(child[LL_BAND].exists()); // We should not be a leaf node here
	kdu_dims ll_dims;  child[LL_BAND].get_dims(ll_dims);

	vert_xform_exists = child[LH_BAND].exists();
	hor_xform_exists = child[HL_BAND].exists();

	const kdu_kernel_step_info *hor_step_info = NULL, *vert_step_info = NULL;

	int s, n;
	int max_vert_kernel_support = 0;
	int total_coeffs = 0;
	float vert_low_gain = 1.0F, vert_high_gain = 1.0F;
	float hor_low_gain = 1.0F, hor_high_gain = 1.0F;
	unit_height = false;
	if (vert_xform_exists)
	{
		int low_min, low_max, high_min, high_max, max_step_length;
		vert_step_info =
			node.get_kernel_info(num_vert_steps, vert_low_gain, vert_high_gain,
				vert_symmetric, vert_symmetric_extension,
				low_min, low_max, high_min, high_max, true);
		max_vert_kernel_support =
			((low_max > high_max) ? low_max : high_max) -
			((low_min < high_min) ? low_min : high_min);
		if (num_vert_steps <= 4)
			vert_steps = step_store;
		else
			vert_steps = vert_step_handle = new kd_lifting_step[num_vert_steps];
		for (max_step_length = 0, s = 0; s < num_vert_steps; s++)
		{
			vert_steps[s].support_min = (kdu_int16)
				vert_step_info[s].support_min;
			vert_steps[s].support_length = (kdu_byte)
				vert_step_info[s].support_length;
			vert_steps[s].downshift = (kdu_byte)vert_step_info[s].downshift;
			vert_steps[s].rounding_offset = (kdu_int16)
				vert_step_info[s].rounding_offset;
			vert_steps[s].kernel_id = (kdu_byte)kernel_id;
			vert_steps[s].reversible = reversible;
			if (max_step_length < (int)vert_step_info[s].support_length)
				max_step_length = vert_step_info[s].support_length;
			total_coeffs += vert_step_info[s].support_length;
		}

		if (max_step_length <= 4)
			vert_source_bufs = source_buf_store;
		else
			vert_source_bufs = source_buf_handle = new void *[2 * max_step_length];
		vert_source_vlines =
			(kd_vlift_line **)(vert_source_bufs + max_step_length);

		kdu_dims lh_dims;  child[LH_BAND].get_dims(lh_dims);
		y_min_in[0] = (ll_dims.pos.y << 1);
		y_max_in[0] = ((ll_dims.pos.y + ll_dims.size.y - 1) << 1);
		y_min_in[1] = (lh_dims.pos.y << 1) + 1;
		y_max_in[1] = ((lh_dims.pos.y + lh_dims.size.y - 1) << 1) + 1;
		if (lh_dims.size.y <= 0)
		{
			unit_height = 1;
			assert(y_min_in[0] == y_max_in[0]);
			y_min_in[1] = y_min_in[0] + 1;
			y_max_in[1] = y_min_in[1] - 2;
		}
		if (ll_dims.size.y <= 0)
		{
			unit_height = 1;
			assert(y_min_in[1] == y_max_in[1]);
			y_min_in[0] = y_min_in[1] + 1;
			y_max_in[0] = y_min_in[0] - 2;
		}
	}
	else
	{
		y_min_in[0] = y_min_out;  y_max_in[0] = y_max_out;
	}

	unit_width = false;
	if (hor_xform_exists)
	{
		int low_min, low_max, high_min, high_max;
		hor_step_info =
			node.get_kernel_info(num_hor_steps, hor_low_gain, hor_high_gain,
				hor_symmetric, hor_symmetric_extension,
				low_min, low_max, high_min, high_max, false);

		if (hor_step_info == vert_step_info)
			hor_steps = vert_steps;
		else if ((num_hor_steps + num_vert_steps) <= 4)
			hor_steps = step_store + num_vert_steps;
		else
			hor_steps = hor_step_handle = new kd_lifting_step[num_hor_steps];
		if (hor_steps != vert_steps)
			for (s = 0; s < num_hor_steps; s++)
			{
				hor_steps[s].support_min = (kdu_int16)
					hor_step_info[s].support_min;
				hor_steps[s].support_length = (kdu_byte)
					hor_step_info[s].support_length;
				hor_steps[s].downshift = (kdu_byte)hor_step_info[s].downshift;
				hor_steps[s].rounding_offset = (kdu_int16)
					hor_step_info[s].rounding_offset;
				hor_steps[s].kernel_id = (kdu_byte)kernel_id;
				hor_steps[s].reversible = reversible;
				total_coeffs += hor_step_info[s].support_length;
			}

		kdu_dims hl_dims;  child[HL_BAND].get_dims(hl_dims);
		int min_in[2] = { (ll_dims.pos.x << 1),
						 (hl_dims.pos.x << 1) + 1 };
		int max_in[2] = { ((ll_dims.pos.x + ll_dims.size.x - 1) << 1),
						 ((hl_dims.pos.x + hl_dims.size.x - 1) << 1) + 1 };
		if (hl_dims.size.x <= 0)
		{
			unit_width = true;
			assert(min_in[0] == max_in[0]);
			min_in[1] = min_in[0] + 1;
			max_in[1] = min_in[1] - 2;
		}
		if (ll_dims.size.x <= 0)
		{
			unit_width = true;
			assert(min_in[1] == max_in[1]);
			min_in[0] = min_in[1] + 1;
			max_in[0] = min_in[0] - 2;
		}
		x_min_in = (min_in[0] < min_in[1]) ? min_in[0] : min_in[1];
		x_max_in = (max_in[0] > max_in[1]) ? max_in[0] : max_in[1];
		while (x_min_in < x_min_buf)
			x_min_buf -= (use_shorts) ? 8 : 4;
		coset_off_in[0] = (kdu_byte)(((x_min_in + 1) >> 1) - ((x_min_buf + 1) >> 1));
		coset_off_in[1] = (kdu_byte)((x_min_in >> 1) - (x_min_buf >> 1));
		coset_width_in[0] = 1 + (x_max_in >> 1) - ((x_min_in + 1) >> 1);
		coset_width_in[1] = 1 + ((x_max_in - 1) >> 1) - (x_min_in >> 1);
		request_offset[0] = (kdu_byte)(ll_dims.pos.x - ((x_min_buf + 1) >> 1));
		request_offset[1] = (kdu_byte)(hl_dims.pos.x - (x_min_buf >> 1));
		request_width[0] = ll_dims.size.x;
		request_width[1] = hl_dims.size.x;

		int lfill[2], rfill[2];
		lfill[0] = ((int)request_offset[0]) - ((int)coset_off_in[0]);
		lfill[1] = ((int)request_offset[1]) - ((int)coset_off_in[1]);
		rfill[0] = (coset_off_in[0] + coset_width_in[0])
			- (request_offset[0] + request_width[0]);
		rfill[1] = (coset_off_in[1] + coset_width_in[1])
			- (request_offset[1] + request_width[1]);
		assert((lfill[0] >= 0) && (rfill[0] >= 0) &&
			(lfill[0] < 256) && (rfill[0] < 256) &&
			(lfill[1] >= 0) && (rfill[1] >= 0) &&
			(lfill[1] < 256) && (rfill[1] < 256));
		left_fill[0] = (kdu_byte)lfill[0];  right_fill[0] = (kdu_byte)rfill[0];
		left_fill[1] = (kdu_byte)lfill[1];  right_fill[1] = (kdu_byte)rfill[1];

		coset_off_out[1] = (kdu_byte)((x_min_out >> 1) - (x_min_buf >> 1));
		coset_off_out[0] = (kdu_byte)(((x_min_out + 1) >> 1) - ((x_min_buf + 1) >> 1));
		coset_width_out[0] = 1 + (x_max_out >> 1) - ((x_min_out + 1) >> 1);
		coset_width_out[1] = 1 + ((x_max_out - 1) >> 1) - (x_min_out >> 1);
	}
	else
	{
		x_min_in = x_min_out;  x_max_in = x_max_out;
		request_offset[0] = (kdu_byte)pull_offset;
		request_width[0] = x_max_out + 1 - x_min_out;
		request_offset[1] = 0; request_width[1] = 0;
		left_fill[0] = left_fill[1] = right_fill[0] = right_fill[1] = 0;
		coset_off_out[0] = (kdu_byte)(x_min_out - x_min_buf);
		coset_width_out[0] = 1 + x_max_out - x_min_out;
		coset_off_out[1] = 0;  coset_width_out[1] = 0;
	}

	// Next, allocate coefficient storage and initialize lifting steps
	float *coeff_next = coeff_store;
	int *icoeff_next = icoeff_store;
	if (total_coeffs > 8)
	{
		coeff_next = coeff_handle = new float[total_coeffs];
		icoeff_next = icoeff_handle = new int[total_coeffs];
	}
	if (vert_xform_exists)
	{
		const float *coefficients = node.get_kernel_coefficients(true);
		for (s = 0; s < num_vert_steps; s++)
		{
			kd_lifting_step *step = vert_steps + s;
			step->step_idx = s;

			step->coeffs = coeff_next;
			step->icoeffs = icoeff_next;
			step->add_shorts_first = false;
#ifdef KDU_SIMD_OPTIMIZATIONS
			if ((kernel_id == Ckernels_W9X7) && (s != 1) &&
				((kdu_mmx_level > 0) || kdu_sparcvis_exists || kdu_altivec_exists))
				step->add_shorts_first = true;
#endif // KDU_SIMD_OPTIMIZATIONS
			float fact, max_factor = 0.4F;
			for (n = 0; n < step->support_length; n++)
			{
				step->coeffs[n] = fact = coefficients[n];
				if (fact > max_factor)
					max_factor = fact;
				else if (fact < -max_factor)
					max_factor = -fact;
			}
			if (!reversible)
			{ // Find appropriate downshift and rounding_offset values
				for (step->downshift = 16; max_factor >= 0.499F; max_factor *= 0.5F)
					step->downshift--;
				if (step->downshift > 15)
					step->rounding_offset = KDU_INT16_MAX;
				else
					step->rounding_offset = (kdu_int16)(1 << (step->downshift - 1));
			}
			fact = (float)(1 << step->downshift);
			for (n = 0; n < step->support_length; n++)
				step->icoeffs[n] = (int)floor(0.5 + step->coeffs[n] * fact);
			coeff_next += step->support_length;
			icoeff_next += step->support_length;
			coefficients += step->support_length;

			step->extend = 0; // Not used in the vertical direction
		}
	}

	int max_extend = 0;
	if (hor_xform_exists)
	{
		const float *coefficients = node.get_kernel_coefficients(false);
		for (s = 0; s < num_hor_steps; s++)
		{
			kd_lifting_step *step = hor_steps + s;
			if (hor_steps != vert_steps)
			{
				step->step_idx = s;
				step->coeffs = coeff_next;
				step->icoeffs = icoeff_next;
				step->add_shorts_first = false;
#ifdef KDU_SIMD_OPTIMIZATIONS
				if ((kernel_id == Ckernels_W9X7) && (s != 1) &&
					((kdu_mmx_level > 0) || kdu_sparcvis_exists || kdu_altivec_exists))
					step->add_shorts_first = true;
#endif // KDU_SIMD_OPTIMIZATIONS
				float fact, max_factor = 0.4F;
				for (n = 0; n < step->support_length; n++)
				{
					step->coeffs[n] = fact = coefficients[n];
					if (fact > max_factor)
						max_factor = fact;
					else if (fact < -max_factor)
						max_factor = -fact;
				}
				if (!reversible)
				{ // Find appropriate downshift and rounding_offset values
					for (step->downshift = 16;
						max_factor >= 0.499F; max_factor *= 0.5F)
						step->downshift--;
					if (step->downshift > 15)
						step->rounding_offset = KDU_INT16_MAX;
					else
						step->rounding_offset = (kdu_int16)(1 << (step->downshift - 1));
				}
				fact = (float)(1 << step->downshift);
				for (n = 0; n < step->support_length; n++)
					step->icoeffs[n] = (int)floor(0.5 + step->coeffs[n] * fact);
				coeff_next += step->support_length;
				icoeff_next += step->support_length;
			}
			else
			{ // Horizontal and vertical lifting steps should be identical
				for (n = 0; n < step->support_length; n++)
					assert(step->coeffs[n] == coefficients[n]);
			}

			coefficients += step->support_length;

			int extend_left = -step->support_min;
			int extend_right = step->support_min - 1 + (int)step->support_length;
			if (x_min_in & 1)
				extend_left += (s & 1) ? (-1) : 1;
			if (!(x_max_in & 1))
				extend_right += (s & 1) ? 1 : (-1);
			int extend = (extend_left > extend_right) ? extend_left : extend_right;
			extend = (extend < 0) ? 0 : extend;
			if (extend > max_extend)
				max_extend = extend;
			assert(extend < 256);
			step->extend = (kdu_byte)extend;
		}
	}

	// Create line queues
	if ((num_vert_steps > 0) && !unit_height)
	{
		if (num_vert_steps <= 4)
			queues = queue_store;
		else
			queues = queue_handle = new kd_vlift_queue[num_vert_steps + 1];
		queues += 1; // So we can access first queue with index -1
	}

	// Create `step_next_row_pos' array
	step_next_row_pos = NULL;
	if (num_vert_steps > 0)
	{
		if (num_vert_steps <= 4)
			step_next_row_pos = next_row_pos_store;
		else
			step_next_row_pos = next_row_pos_handle = new int[num_vert_steps];
	}

	// Find allocation parameters for internal line buffers
	int buf_width[2] = { request_offset[0] + request_width[0],
						request_offset[1] + request_width[1] };
	int buf_neg_extent, buf_pos_extent;
	if (hor_xform_exists)
	{
		int spare_left = // Num unused initial samples in all cosets
			(coset_off_in[0] < coset_off_in[1]) ? coset_off_in[0] : coset_off_in[1];
		buf_neg_extent = max_extend - spare_left;
		if (buf_neg_extent < 0)
			buf_neg_extent = 0;
		buf_pos_extent = coset_off_in[0] + coset_width_in[0];
		if (buf_pos_extent < (coset_off_in[1] + coset_width_in[1]))
			buf_pos_extent = coset_off_in[1] + coset_width_in[1];
		buf_pos_extent += max_extend;
	}
	else
	{
		buf_neg_extent = 0;
		buf_pos_extent = buf_width[0];
	}

	// Simulate vertical lifting process to find number of line buffers required
	int required_line_buffers = 1; // Need this if no vertical transform
	if ((num_vert_steps > 0) && !unit_height)
		required_line_buffers = simulate_vertical_lifting(max_vert_kernel_support);

	if (required_line_buffers > 6)
		line_handle = new kd_vlift_line[required_line_buffers];
	for (free_lines = NULL, n = 0; n < required_line_buffers; n++)
	{
		if (line_handle == NULL)
		{
			line_store[n].next = free_lines;  free_lines = line_store + n;
		}
		else
		{
			line_handle[n].next = free_lines; free_lines = line_handle + n;
		}
		free_lines->pre_create(allocator, buf_width[0], buf_width[1], reversible,
			use_shorts, buf_neg_extent, buf_pos_extent,
			hor_xform_exists);
	}

	// Set up vertical counters and queues
	y_next_out = y_min_out;
	y_next_in[0] = y_min_in[0];  y_next_in[1] = y_min_in[1];
	if (queues != NULL)
	{ // Figure out reflection bounds
		int y_min = (y_min_in[0] < y_min_in[1]) ? y_min_in[0] : y_min_in[1];
		int y_max = (y_max_in[0] > y_max_in[1]) ? y_max_in[0] : y_max_in[1];
		for (s = -1; s < num_vert_steps; s++)
		{
			int local_y_min = (y_min_in[s & 1] > (y_min + 1)) ? (y_min_in[s & 1]) : y_min;
			int local_y_max = (y_max_in[s & 1] < (y_max - 1)) ? (y_max_in[s & 1]) : y_max;
			int max_source_request_idx = local_y_max - ((local_y_max ^ s) & 1);
			if (s >= 0)
				max_source_request_idx = y_max_in[s & 1] +
				2 * (vert_steps[s].support_min - 1 +
				(int)vert_steps[s].support_length);
			queues[s].init(local_y_min, local_y_max, s, vert_symmetric_extension,
				max_source_request_idx);
			if ((s >= 0) && (vert_steps[s].support_length <= 0))
				queues[s].source_done();
		}
	}
	for (s = 0; s < num_vert_steps; s++)
		step_next_row_pos[s] = y_min_in[1 - (s & 1)];

	// Now determine the normalizing downshift and subband nominal ranges.
	float child_ranges[4] =
	{ normalization, normalization, normalization, normalization };
	normalizing_upshift = 0;
	if (!reversible)
	{
		int ns;
		const float *vert_bibo_gains = node.get_bibo_gains(ns, true);
		assert(ns == num_vert_steps);
		const float *hor_bibo_gains = node.get_bibo_gains(ns, false);
		assert(ns == num_hor_steps);

		float bibo_max = normalization; // Not a true BIBO gain; this value just
										 // means leave normalization alone.

		// Find BIBO gains from vertical analysis steps (these are done first)
		if ((num_vert_steps > 0) && !unit_height)
		{
			child_ranges[LL_BAND] /= vert_low_gain;
			child_ranges[HL_BAND] /= vert_low_gain;
			child_ranges[LH_BAND] /= vert_high_gain;
			child_ranges[HH_BAND] /= vert_high_gain;

			float bibo_prev, bibo_in, bibo_out;

			// Find cumulative horizontal gain to input of node.
			bibo_prev = hor_bibo_gains[0] * normalization;
			for (bibo_in = bibo_prev, n = 0; n < num_vert_steps; n++,
				bibo_in = bibo_out)
			{
				bibo_out = bibo_prev * vert_bibo_gains[n + 1];
				if (bibo_out > bibo_max)
					bibo_max = bibo_out; // Fit representation to lifting outputs
				if (use_shorts && vert_steps[n].add_shorts_first)
				{
					bibo_in *= 2.0; // Avoid overflow during pre-accumulation
					if (bibo_in > bibo_max)
						bibo_max = bibo_in;
				}
			}
		}

		// Find BIBO gains from horizonal analysis steps
		if ((num_hor_steps > 0) && !unit_width)
		{
			child_ranges[LL_BAND] /= hor_low_gain;
			child_ranges[HL_BAND] /= hor_high_gain;
			child_ranges[LH_BAND] /= hor_low_gain;
			child_ranges[HH_BAND] /= hor_high_gain;

			float bibo_prev, bibo_in, bibo_out;

			// Find maximum gain at output of vertical decomposition, if any
			bibo_prev = vert_bibo_gains[num_vert_steps];
			if ((num_vert_steps > 0) &&
				(vert_bibo_gains[num_vert_steps - 1] > bibo_prev))
				bibo_prev = vert_bibo_gains[num_vert_steps - 1];
			bibo_prev *= normalization; // Account for fact that gains reported
					 // by `kdu_node::get_bibo_gains' assume `normalization'=1

			for (bibo_in = bibo_prev, n = 0; n < num_hor_steps; n++,
				bibo_in = bibo_out)
			{
				bibo_out = bibo_prev * hor_bibo_gains[n + 1];
				if (bibo_out > bibo_max)
					bibo_max = bibo_out; // Fit representation to lifting outputs
				if (use_shorts && hor_steps[n].add_shorts_first)
				{
					bibo_in *= 2.0; // Avoid overflow during pre-accumulation
					if (bibo_in > bibo_max)
						bibo_max = bibo_in;
				}
			}
		}

		if (use_shorts)
		{
			float overflow_limit = 1.0F * (float)(1 << (16 - KDU_FIX_POINT));
			// This is the largest numeric range which can be represented in
			// our signed 16-bit fixed-point representation without overflow.
			while (bibo_max > 0.95*overflow_limit)
			{ // Leave a little extra headroom to allow for approximations in
			  // the numerical BIBO gain calculations.
				normalizing_upshift++;
				for (int b = 0; b < 4; b++)
					child_ranges[b] *= 0.5F;
				bibo_max *= 0.5;
			}
		}
	}

	// Finally, create the subband interfaces recursively.
	for (n = 0; n < 4; n++)
	{
		if (!child[n])
			continue;
		int child_offset = request_offset[n & 1];
		if (!child[n].access_child(LL_BAND))
			subbands[n] = kdu_decoder(child[n].access_subband(), allocator,
				use_shorts, child_ranges[n], child_offset,
				env, env_queue);
		else
			subbands[n] = kdu_synthesis(child[n], allocator, use_shorts,
				child_ranges[n], child_offset,
				env, env_queue);
	}
}

/*****************************************************************************/
/*                      kd_synthesis::~kd_synthesis                          */
/*****************************************************************************/

kd_synthesis::~kd_synthesis()
{
	int n;
	for (n = 0; n < 4; n++)
		if (subbands[n].exists())
			subbands[n].destroy();
	if (coeff_handle != NULL)
		delete[] coeff_handle;
	if (icoeff_handle != NULL)
		delete[] icoeff_handle;
	if (source_buf_handle != NULL)
		delete[] source_buf_handle;
	if (line_handle != NULL)
		delete[] line_handle;
	if (vert_step_handle != NULL)
		delete[] vert_step_handle;
	if (hor_step_handle != NULL)
		delete[] hor_step_handle;
	if (queue_handle != NULL)
		delete[] queue_handle;
	if (next_row_pos_handle != NULL)
		delete[] next_row_pos_handle;
}

/*****************************************************************************/
/*                 kd_synthesis::simulate_vertical_lifting                   */
/*****************************************************************************/

int
kd_synthesis::simulate_vertical_lifting(int support_max)
{
	int s, s_max, src_idx, vsub_parity;
	int max_used_lines = 0, used_lines = 0;
	kd_lifting_step *step;

	// Find simple upper bounds to avoid wasting too much simulation effort
	int sim_y_max_out = y_max_out;
	int sim_y_max_in[2] = { y_max_in[0], y_max_in[1] };
	int delta = sim_y_max_out - (y_min_out + support_max + 2);
	if (delta > 0)
	{
		delta &= ~1; // Change height by multiple of 2
		sim_y_max_out -= delta;
		sim_y_max_in[0] -= delta;
		sim_y_max_in[1] -= delta;
	}

	// Set up counters for simulation
	y_next_out = y_min_out;
	y_next_in[0] = y_min_in[0];  y_next_in[1] = y_min_in[1];
	{
		int y_min = (y_min_in[0] < y_min_in[1]) ? y_min_in[0] : y_min_in[1];
		int y_max =
			(sim_y_max_in[0] > sim_y_max_in[1]) ? sim_y_max_in[0] : sim_y_max_in[1];
		for (s = -1; s < num_vert_steps; s++)
		{
			int local_y_min = (y_min_in[s & 1] > (y_min + 1)) ? (y_min_in[s & 1]) : y_min;
			int local_y_max =
				(sim_y_max_in[s & 1] < (y_max - 1)) ? (sim_y_max_in[s & 1]) : y_max;
			int max_source_request_idx = local_y_max - ((local_y_max ^ s) & 1);
			if (s >= 0)
				max_source_request_idx = sim_y_max_in[s & 1] +
				2 * (vert_steps[s].support_min - 1 +
				(int)vert_steps[s].support_length);
			queues[s].init(local_y_min, local_y_max, s, vert_symmetric_extension,
				max_source_request_idx);
			if ((s >= 0) && (vert_steps[s].support_length <= 0))
				queues[s].source_done();
		}
	}
	for (s = 0; s < num_vert_steps; s++)
		step_next_row_pos[s] = y_min_in[1 - (s & 1)];

	// Run the simulator
	for (; y_next_out <= sim_y_max_out; y_next_out++)
	{
		s_max = -1;
		bool have_result = false;
		do { // Loop until row `y_next_out' becomes available
			for (s = s_max; s >= 0; s--)
			{
				vsub_parity = 1 - (s & 1); // Parity of output line for this step
				if (s == num_vert_steps)
				{ // This happens when we need to load data into the last
				  // queue
					if (y_next_in[vsub_parity] <= sim_y_max_in[vsub_parity])
					{
						used_lines++; // Simulate getting new line from free list
						if (used_lines > max_used_lines)
							max_used_lines = used_lines;
						queues[s - 1].simulate_push_line(y_next_in[vsub_parity],
							used_lines);
						y_next_in[vsub_parity] += 2;
					}
					continue;
				}

				step = vert_steps + s;
				src_idx = (step_next_row_pos[s] ^ 1) + 2 * step->support_min;
				if (step_next_row_pos[s] > sim_y_max_in[vsub_parity])
					continue; // Lifting step is finished

				if (s < (num_vert_steps - 1))
				{
					if (!queues[s + 1].test_update(step_next_row_pos[s], false))
					{ // Need to progress up-stream synthesis step
						s_max = s + 2;
						break;
					}
				}
				if ((step->support_length > 0) &&
					!queues[s].simulate_access_source(src_idx,
						step->support_length,
						used_lines))
				{ // Can't access all source lines required by step
					s_max = s + 1;
					break;
				}
				if (s == (num_vert_steps - 1))
				{ // Read update row from subbands
					used_lines++; // Simulate getting input line from free list
								  // and re-using it as the output line
					if (used_lines > max_used_lines)
						max_used_lines = used_lines;
					assert(step_next_row_pos[s] == y_next_in[vsub_parity]);
					y_next_in[vsub_parity] += 2;
				}
				else
				{
					queues[s + 1].simulate_access_update(step_next_row_pos[s],
						used_lines);
					used_lines++; // Simulate getting output line from free list
					if (used_lines > max_used_lines)
						max_used_lines = used_lines;
				}

				queues[s - 1].simulate_push_line(step_next_row_pos[s], used_lines);

				step_next_row_pos[s] += 2;
			}

			if (s < 0)
			{ // Finished a round of vertical synthesis.  See if we have
			  // the required row yet
				s_max = 1 - (y_next_out & 1);
				have_result =
					queues[s_max - 1].simulate_access_update(y_next_out, used_lines);
				if (vertical_first && have_result &&
					(s_max > 0) && (used_lines == max_used_lines))
					max_used_lines++; // Need an extra one for horz synthesis
			}
		} while (!have_result);
	}

	return max_used_lines;
}

/*****************************************************************************/
/*                           kd_synthesis::start                             */
/*****************************************************************************/

void
kd_synthesis::start(kdu_thread_env *env)
{
	if (!initialized)
	{ // Finish creating all the buffers.
		kd_vlift_line *scan;
		if (env != NULL)
			env->acquire_lock(KD_THREADLOCK_ALLOCATOR);
		for (scan = free_lines; scan != NULL; scan = scan->next)
			scan->create();
		initialized = true;
		if (env != NULL)
			env->release_lock(KD_THREADLOCK_ALLOCATOR);
	}

	int n;
	for (n = 0; n < 4; n++)
		if (subbands[n].exists())
			subbands[n].start(env);
}

/*****************************************************************************/
/*                           kd_synthesis::pull                              */
/*****************************************************************************/

void
kd_synthesis::pull(kdu_line_buf &line, kdu_thread_env *env)
{
	if (empty)
		return;

	kd_vlift_line *vline_in, *vline_out = NULL;
	if (!initialized)
		start(env);

	assert(y_next_out <= y_max_out);

	int c, k;
	int vsub_parity = (vert_xform_exists) ? (y_next_out & 1) : 0; // Vert subband parity
	if ((num_vert_steps == 0) || unit_height)
	{ // No vertical synthesis
		vline_out = free_lines;
		horizontal_synthesis(vline_out, vsub_parity, env, false);
		if (unit_height && reversible && vsub_parity)
		{ // Reversible transform of a single odd-indexed sample -- halve vals
			if (!use_shorts)
			{ // Working with 32-bit data
				kdu_sample32 *dp;
				for (c = 0; c < 2; c++)
					for (dp = vline_out->cosets[c].get_buf32() + coset_off_out[c],
						k = coset_width_out[c]; k--; dp++)
						dp->ival >>= 1;
			}
			else
			{ // Working with 16-bit data
				kdu_sample16 *dp;
				for (c = 0; c < 2; c++)
					for (dp = vline_out->cosets[c].get_buf16() + coset_off_out[c],
						k = coset_width_out[c]; k--; dp++)
						dp->ival >>= 1;
			}
		}
	}
	else
	{ // Need to perform vertical synthesis
		kd_lifting_step *step;
		int src_idx, s, s_max = -1;
		do { // Loop until row `y_next_out' becomes available
			for (s = s_max; s >= 0; s--)
			{
				vsub_parity = 1 - (s & 1); // Parity of output line for this step
				if (s == num_vert_steps)
				{ // This happens when we need to load data into the last
				  // queue
					if (y_next_in[vsub_parity] <= y_max_in[vsub_parity])
					{
						vline_in = free_lines;  assert(vline_in != NULL);
						free_lines = vline_in->next;  vline_in->next = NULL;
						horizontal_synthesis(vline_in, vsub_parity, env, false);
						queues[s - 1].push_line(y_next_in[vsub_parity], vline_in,
							free_lines);
						y_next_in[vsub_parity] += 2;
					}
					continue;
				}

				step = vert_steps + s;
				src_idx = (step_next_row_pos[s] ^ 1) + 2 * step->support_min;
				vline_in = vline_out = NULL;
				if (step_next_row_pos[s] > y_max_in[vsub_parity])
					continue; // Lifting step is finished

				vline_in = NULL;
				if (s < (num_vert_steps - 1))
				{
					if (!queues[s + 1].test_update(step_next_row_pos[s], false))
					{ // Need to progress up-stream synthesis step
						s_max = s + 2;
						break;
					}
				}
				if ((step->support_length > 0) &&
					!queues[s].access_source(src_idx, step->support_length,
						vert_source_vlines, free_lines))
				{ // Can't access all source lines required by step
					s_max = s + 1;
					break;
				}
				if (s == (num_vert_steps - 1))
				{ // Read update row from subbands
					vline_in = free_lines;  assert(vline_in != NULL);
					assert(step_next_row_pos[s] == y_next_in[vsub_parity]);
					horizontal_synthesis(vline_in, vsub_parity, env, false);
					y_next_in[vsub_parity] += 2;
				}
				else
					vline_in =
					queues[s + 1].access_update(step_next_row_pos[s], free_lines);
				vline_out = free_lines;  assert(vline_out != NULL);
				free_lines = vline_out->next;  vline_out->next = NULL;
				queues[s - 1].push_line(step_next_row_pos[s], vline_out,
					free_lines);
				if (step->support_length <= 0)
				{
					if (use_shorts)
						for (c = 0; c < 2; c++)
							memcpy(vline_out->cosets[c].get_buf16(),
								vline_in->cosets[c].get_buf16(),
								(size_t)(vline_in->cosets[c].get_width() << 1));
					else
						for (c = 0; c < 2; c++)
							memcpy(vline_out->cosets[c].get_buf32(),
								vline_in->cosets[c].get_buf32(),
								(size_t)(vline_in->cosets[c].get_width() << 2));
				}
				else
				{
					int k;
					for (c = 0; c < 2; c++)
					{
						if (coset_width_out[c] == 0)
							continue;
						int width = coset_width_out[c];
						int off = coset_off_out[c];
						if (vertical_first)
						{ // Need to do vert lifting across the entire coset
							off = 0; width = vline_in->cosets[c].get_width();
						}
						if (use_shorts)
						{
							kdu_sample16 **bf = (kdu_sample16 **)vert_source_bufs;
							for (k = 0; k < (int)step->support_length; k++)
								bf[k] = vert_source_vlines[k]->cosets[c].get_buf16();
							perform_synthesis_lifting_step(step, bf,
								vline_in->cosets[c].get_buf16(),
								vline_out->cosets[c].get_buf16(),
								width, off);
						}
						else
						{
							kdu_sample32 **bf = (kdu_sample32 **)vert_source_bufs;
							for (k = 0; k < (int)step->support_length; k++)
								bf[k] = vert_source_vlines[k]->cosets[c].get_buf32();
							perform_synthesis_lifting_step(step, bf,
								vline_in->cosets[c].get_buf32(),
								vline_out->cosets[c].get_buf32(),
								width, off);
						}
					}
				}
				step_next_row_pos[s] += 2;
			}

			vline_out = NULL;
			if (s < 0)
			{ // Finished a round of vertical synthesis.  See if we have
			  // the required row yet
				s_max = 1 - (y_next_out & 1);
				vline_out = queues[s_max - 1].access_update(y_next_out, free_lines);
				if (vertical_first && (vline_out != NULL))
				{ // Peform horizontal synthesis
					if (s_max > 0)
					{ // May need to make a copy of `vline_out' for doing the
					  // horizontal synthesis
						kd_vlift_line *tmp = free_lines; assert(tmp != NULL);
						if (tmp != vline_out)
						{ // We do need to copy
							for (c = 0; c < 2; c++)
								if (use_shorts)
									memcpy(tmp->cosets[c].get_buf16(),
										vline_out->cosets[c].get_buf16(),
										(size_t)(tmp->cosets[c].get_width() << 1));
								else
									memcpy(tmp->cosets[c].get_buf32(),
										vline_out->cosets[c].get_buf32(),
										(size_t)(tmp->cosets[c].get_width() << 2));
							vline_out = tmp;
						}
					}
					horizontal_synthesis(vline_out, y_next_out & 1, env, true);
				}
			}
		} while (vline_out == NULL);
	}

	y_next_out++;

	/* Finally, copy newly generated samples into `line', interleaving the
	   even and odd sub-sequences, as required, and supplying any required
	   upshift. */
	c = x_min_pull & 1; // Index of first coset to be interleaved.
	k = (x_max_out + 2 - x_min_pull) >> 1; // May write one extra sample.
	int src_off = (x_min_pull - x_min_buf) >> 1;
	if (!hor_xform_exists)
	{ // Just transfer contents from coset[0]
		assert(src_off == 0);
		if (!use_shorts)
		{ // Working with 32-bit data
			kdu_sample32 *dp = line.get_buf32();
			kdu_sample32 *sp = vline_out->cosets[0].get_buf32();
			assert(normalizing_upshift == 0);
			for (; k--; sp += 2, dp += 2)
			{
				dp[0] = sp[0];  dp[1] = sp[1];
			}
		}
		else
		{ // Working with 16-bit data
			kdu_sample16 *dp = line.get_buf16();
			kdu_sample16 *sp = vline_out->cosets[0].get_buf16();
			if (normalizing_upshift == 0)
				for (; k--; sp += 2, dp += 2)
				{
					dp[0] = sp[0]; dp[1] = sp[1];
				}
			else
			{
				for (; k--; sp += 2, dp += 2)
				{
					dp[0].ival = (sp[0].ival << normalizing_upshift);
					dp[1].ival = (sp[1].ival << normalizing_upshift);
				}
			}
		}
	}
	else
	{ // Interleave even and odd cosets
		if (!use_shorts)
		{ // Working with 32-bit data
			kdu_sample32 *dp = line.get_buf32();
			kdu_sample32 *sp1 = vline_out->cosets[c].get_buf32() + src_off;
			kdu_sample32 *sp2 = vline_out->cosets[1 - c].get_buf32() + src_off;
			assert(normalizing_upshift == 0);
#ifdef KDU_SIMD_OPTIMIZATIONS
			if (!simd_interleave(&(sp1->ival), &(sp2->ival), &(dp->ival), k))
#endif // KDU_SIMD_OPTIMIZATIONS          if (normalizing_upshift == 0)
				for (; k--; sp1++, sp2++, dp += 2)
				{
					dp[0] = *sp1; dp[1] = *sp2;
				}
		}
		else
		{ // Working with 16-bit data
			kdu_sample16 *dp = line.get_buf16();
			kdu_sample16 *sp1 = vline_out->cosets[c].get_buf16() + src_off;
			kdu_sample16 *sp2 = vline_out->cosets[1 - c].get_buf16() + src_off;
			if (normalizing_upshift == 0)
			{
#ifdef KDU_SIMD_OPTIMIZATIONS
				if (!simd_interleave(&(sp1->ival), &(sp2->ival), &(dp->ival), k))
#endif // KDU_SIMD_OPTIMIZATIONS
					for (; k--; sp1++, sp2++, dp += 2)
					{
						dp[0] = *sp1; dp[1] = *sp2;
					}
			}
			else
			{
#ifdef KDU_SIMD_OPTIMIZATIONS
				if (!simd_upshifted_interleave(&(sp1->ival), &(sp2->ival),
					&(dp->ival), k, normalizing_upshift))
#endif // KDU_SIMD_OPTIMIZATIONS
					for (; k--; sp1++, sp2++, dp += 2)
					{
						dp[0].ival = (*sp1).ival << normalizing_upshift;
						dp[1].ival = (*sp2).ival << normalizing_upshift;
					}
			}
		}
	}
}

/*****************************************************************************/
/*                    kd_synthesis::horizontal_synthesis                     */
/*****************************************************************************/

void
kd_synthesis::horizontal_synthesis(kd_vlift_line *line, int vert_parity,
	kdu_thread_env *env, bool post_vertical)
{
	int s, c, k, width, x_off;

	if (!post_vertical)
	{ // Load the data in from subbands, performing any boundary fill
		for (c = 0; c < 2; c++)
			if (request_width[c] > 0)
			{
				subbands[2 * vert_parity + c].pull(line->cosets[c], env);
				if ((k = left_fill[c]) > 0)
				{
					if (use_shorts)
						for (kdu_sample16 *sp = line->cosets[c].get_buf16() +
							request_offset[c]; k > 0; k--, sp--)
							sp[-1] = sp[0];
					else
						for (kdu_sample32 *sp = line->cosets[c].get_buf32() +
							request_offset[c]; k > 0; k--, sp--)
							sp[-1] = sp[0];
				}
				if ((k = right_fill[c]) > 0)
				{
					if (use_shorts)
						for (kdu_sample16 *sp = line->cosets[c].get_buf16() +
							request_offset[c] + request_width[c]; k > 0; k--, sp++)
							sp[0] = sp[-1];
					else
						for (kdu_sample32 *sp = line->cosets[c].get_buf32() +
							request_offset[c] + request_width[c]; k > 0; k--, sp++)
							sp[0] = sp[-1];
				}
			}
		if (vertical_first)
			return; // Come back and do the horizontal transform later
	}

	// Check to see if we can skip the horizontal transform
	if (num_hor_steps == 0)
		return;
	if (unit_width)
	{
		if (reversible && (x_min_in & 1))
		{
			if (!use_shorts)
				(line->cosets[1].get_buf32())[coset_off_in[1]].ival >>= 1;
			else
				(line->cosets[1].get_buf16())[coset_off_in[1]].ival >>= 1;
		}
		return;
	}

	// Perform horizontal lifting steps
	for (s = num_hor_steps - 1; s >= 0; s--)
	{
		kd_lifting_step *step = hor_steps + s;
		if (step->support_length == 0)
			continue;
		c = 1 - (s & 1); // Coset which is updated by this lifting step
		width = coset_width_in[c];
		x_off = coset_off_in[c];
		if (use_shorts)
		{ // Processing 16-bit samples
			kdu_sample16 *sp = line->cosets[1 - c].get_buf16() + coset_off_in[1 - c];
			kdu_sample16 *dp = line->cosets[c].get_buf16() + x_off;
			kdu_sample16 *esp = sp + coset_width_in[1 - c] - 1; // End of source buffer
			if (!hor_symmetric_extension)
				for (k = 1; k <= step->extend; k++)
				{
					sp[-k] = sp[0];  esp[k] = esp[0];
				}
			else
				for (k = 1; k <= step->extend; k++)
				{ // Apply symmetric extension policy
					sp[-k] = sp[k - ((x_min_in^s) & 1)];
					esp[k] = esp[-k + ((x_max_in^s) & 1)];
				}
			if (x_min_in & 1)
				sp -= c + c - 1;
			sp += step->support_min;

#ifdef KDU_SIMD_OPTIMIZATIONS
			if ((step->kernel_id == (kdu_byte)Ckernels_W5X3) &&
				simd_W5X3_h_synth(&(sp[-x_off].ival), &(dp[-x_off].ival),
					width + x_off, step))
				continue;
			else if ((step->kernel_id == (kdu_byte)Ckernels_W9X7) &&
				simd_W9X7_h_synth(&(sp[-x_off].ival), &(dp[-x_off].ival),
					width + x_off, step))
				continue;
			else if (step->support_length <= 2)
			{
				if (simd_2tap_h_synth(&(sp[-x_off].ival), &(dp[-x_off].ival),
					width + x_off, step))
					continue;
			}
			else if (step->support_length <= 4)
			{
				if (simd_4tap_h_synth(&(sp[-x_off].ival), &(dp[-x_off].ival),
					width + x_off, step))
					continue;
			}
#endif // KDU_SIMD_OPTIMIZATIONS
			if ((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]))
			{ // Special implementation for symmetric least-dissimilar filters
				kdu_int32 downshift = step->downshift;
				kdu_int32 offset = step->rounding_offset;
				kdu_int32 val, i_lambda = step->icoeffs[0];
				if (i_lambda == 1)
					for (k = 0; k < width; k++)
					{
						val = sp[k].ival;  val += sp[k + 1].ival;
						dp[k].ival -= (kdu_int16)((offset + val) >> downshift);
					}
				else if (i_lambda == -1)
					for (k = 0; k < width; k++)
					{
						val = sp[k].ival;  val += sp[k + 1].ival;
						dp[k].ival -= (kdu_int16)((offset - val) >> downshift);
					}
				else
					for (k = 0; k < width; k++)
					{
						val = sp[k].ival;  val += sp[k + 1].ival;
						dp[k].ival -=
							(kdu_int16)((offset + i_lambda * val) >> downshift);
					}
			}
			else
			{ // Generic 16-bit implementation
				int t, support = step->support_length;
				kdu_int32 downshift = step->downshift;
				kdu_int32 offset = step->rounding_offset;
				int sum, *cpp = step->icoeffs;
				for (k = 0; k < width; k++, sp++)
				{
					for (sum = offset, t = 0; t < support; t++)
						sum += cpp[t] * sp[t].ival;
					dp[k].ival -= (kdu_int16)(sum >> downshift);
				}
			}
		}
		else
		{ // Processing 32-bit samples
			kdu_sample32 *sp = line->cosets[1 - c].get_buf32() + coset_off_in[1 - c];
			kdu_sample32 *dp = line->cosets[c].get_buf32() + x_off;
			kdu_sample32 *esp = sp + coset_width_in[1 - c] - 1; // End of source buffer
			if (!hor_symmetric_extension)
				for (k = 1; k <= step->extend; k++)
				{
					sp[-k] = sp[0];  esp[k] = esp[0];
				}
			else
				for (k = 1; k <= step->extend; k++)
				{ // Apply symmetric extension policy
					sp[-k] = sp[k - ((x_min_in^s) & 1)];
					esp[k] = esp[-k + ((x_max_in^s) & 1)];
				}
			if (x_min_in & 1)
				sp -= c + c - 1;
			sp += step->support_min;

#ifdef KDU_SIMD_OPTIMIZATIONS
			if ((step->kernel_id == (kdu_byte)Ckernels_W5X3) &&
				simd_W5X3_h_synth32(&(sp[-x_off].ival), &(dp[-x_off].ival),
					width + x_off, step))
				continue;
			else if (!reversible)
			{
				if (step->support_length <= 2)
				{
					if (simd_2tap_h_irrev32(&(sp[-x_off].fval),
						&(dp[-x_off].fval), width + x_off,
						step, true))
						continue;
				}
				else if (step->support_length <= 4)
				{
					if (simd_4tap_h_irrev32(&(sp[-x_off].fval),
						&(dp[-x_off].fval), width + x_off,
						step, true))
						continue;
				}
			}
#endif // KDU_SIMD_OPTIMIZATIONS
			if ((step->support_length == 2) && (step->coeffs[0] == step->coeffs[1]))
			{ // Special implementation for symmetric least-dissimilar filters
				if (!reversible)
				{
					float lambda = step->coeffs[0];
					for (k = 0; k < width; k++)
						dp[k].fval -= lambda * (sp[k].fval + sp[k + 1].fval);
				}
				else
				{
					kdu_int32 downshift = step->downshift;
					kdu_int32 offset = step->rounding_offset;
					kdu_int32 i_lambda = step->icoeffs[0];
					if (i_lambda == 1)
						for (k = 0; k < width; k++)
							dp[k].ival -=
							((offset + sp[k].ival + sp[k + 1].ival) >> downshift);
					else if (i_lambda == -1)
						for (k = 0; k < width; k++)
							dp[k].ival -=
							((offset - sp[k].ival - sp[k + 1].ival) >> downshift);
					else
						for (k = 0; k < width; k++)
							dp[k].ival -=
							((offset + i_lambda * (sp[k].ival + sp[k + 1].ival)) >> downshift);
				}
			}
			else
			{ // Generic 32-bit lifting step implementation
				int t, support = step->support_length;
				if (!reversible)
				{
					float sum, *cpp = step->coeffs;
					for (k = 0; k < width; k++, sp++)
					{
						for (sum = 0.0F, t = 0; t < support; t++)
							sum += cpp[t] * sp[t].fval;
						dp[k].fval -= sum;
					}
				}
				else
				{
					kdu_int32 downshift = step->downshift;
					kdu_int32 offset = step->rounding_offset;
					int sum, *cpp = step->icoeffs;
					for (k = 0; k < width; k++, sp++)
					{
						for (sum = offset, t = 0; t < support; t++)
							sum += cpp[t] * sp[t].ival;
						dp[k].ival -= (sum >> downshift);
					}
				}
			}
		}
	}
}


/* ========================================================================= */
/*                           EXTERNAL FUNCTIONS                              */
/* ========================================================================= */

/*****************************************************************************/
/* EXTERN            perform_synthesis_lifting_step (32-bit)                 */
/*****************************************************************************/

void
perform_synthesis_lifting_step(kd_lifting_step *step,
	kdu_sample32 *src_ptrs[],
	kdu_sample32 *in, kdu_sample32 *out,
	int width, int x_off)
{
	int k;

	if (width <= 0)
		return;
	while (x_off > 4)
	{
		x_off -= 4; in += 4; out += 4;
	}
	width += x_off;

#ifdef KDU_SIMD_OPTIMIZATIONS
	if (step->kernel_id == (kdu_byte)Ckernels_W5X3)
	{
		if (simd_W5X3_v_synth32(&(src_ptrs[0]->ival), &(src_ptrs[1]->ival),
			&(in->ival), &(out->ival), width, step))
			return;
	}
	else if (!step->reversible)
	{
		if (step->support_length <= 2)
		{
			if (simd_2tap_v_irrev32(&(src_ptrs[0]->fval),
				&(src_ptrs[step->support_length - 1]->fval),
				&(in->fval), &(out->fval), width, step, true))
				return;
		}
		else if (step->support_length <= 4)
		{
			if (simd_4tap_v_irrev32(&(src_ptrs[0]->fval), &(src_ptrs[1]->fval),
				&(src_ptrs[2]->fval),
				&(src_ptrs[step->support_length - 1]->fval),
				&(in->fval), &(out->fval), width, step, true))
				return;
		}
	}
#endif // KDU_SIMD_OPTIMIZATIONS
	if ((step->support_length == 2) && (step->coeffs[0] == step->coeffs[1]))
	{ // Special implementation for symmetric least-dissimilar filters
		kdu_sample32 *sp1 = src_ptrs[0], *sp2 = src_ptrs[1];
		if (!step->reversible)
		{
			float lambda = step->coeffs[0];
			for (k = x_off; k < width; k++)
				out[k].fval = in[k].fval - lambda * (sp1[k].fval + sp2[k].fval);
		}
		else
		{
			kdu_int32 downshift = step->downshift;
			kdu_int32 offset = step->rounding_offset;
			kdu_int32 i_lambda = step->icoeffs[0];
			if (i_lambda == 1)
				for (k = x_off; k < width; k++)
					out[k].ival = in[k].ival -
					((offset + sp1[k].ival + sp2[k].ival) >> downshift);
			else if (i_lambda == -1)
				for (k = x_off; k < width; k++)
					out[k].ival = in[k].ival -
					((offset - sp1[k].ival - sp2[k].ival) >> downshift);
			else
				for (k = x_off; k < width; k++)
					out[k].ival = in[k].ival -
					((offset + i_lambda * (sp1[k].ival + sp2[k].ival)) >> downshift);
		}
	}
	else
	{ // More general 32-bit processing
		int t;
		if (!step->reversible)
		{
			for (t = 0; t < step->support_length; t++, in = out)
			{
				kdu_sample32 *sp = src_ptrs[t];
				float lambda = step->coeffs[t];
				for (k = x_off; k < width; k++)
					out[k].fval = in[k].fval - lambda * sp[k].fval;
			}
		}
		else
		{
			kdu_int32 downshift = step->downshift;
			kdu_int32 offset = step->rounding_offset;
			int sum, *fpp, support = step->support_length;
			for (k = x_off; k < width; k++)
			{
				for (fpp = step->icoeffs, sum = offset, t = 0; t < support; t++)
					sum += fpp[t] * (src_ptrs[t])[k].ival;
				out[k].ival = in[k].ival - (sum >> downshift);
			}
		}
	}
}

/*****************************************************************************/
/* EXTERN            perform_synthesis_lifting_step (16-bit)                 */
/*****************************************************************************/

void
perform_synthesis_lifting_step(kd_lifting_step *step,
	kdu_sample16 *src_ptrs[],
	kdu_sample16 *in, kdu_sample16 *out,
	int width, int x_off)
{
	int k;

	if (width <= 0)
		return;
	while (x_off > 8)
	{
		x_off -= 8; in += 8; out += 8;
	}
	width += x_off;

#ifdef KDU_SIMD_OPTIMIZATIONS
	if ((step->kernel_id == (kdu_byte)Ckernels_W5X3) &&
		simd_W5X3_v_synth(&(src_ptrs[0]->ival), &(src_ptrs[1]->ival),
			&(in->ival), &(out->ival), width, step))
		return;
	else if ((step->kernel_id == (kdu_byte)Ckernels_W9X7) &&
		simd_W9X7_v_synth(&(src_ptrs[0]->ival), &(src_ptrs[1]->ival),
			&(in->ival), &(out->ival), width, step))
		return;
	else if (step->support_length <= 2)
	{
		if (simd_2tap_v_synth(&(src_ptrs[0]->ival),
			&(src_ptrs[step->support_length - 1]->ival),
			&(in->ival), &(out->ival), width, step))
			return;
	}
	else if (step->support_length <= 4)
	{
		if (simd_4tap_v_synth(&(src_ptrs[0]->ival), &(src_ptrs[1]->ival),
			&(src_ptrs[2]->ival),
			&(src_ptrs[step->support_length - 1]->ival),
			&(in->ival), &(out->ival), width, step))
			return;
	}
#endif // KDU_SIMD_OPTIMIZATIONS
	if ((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]))
	{
		kdu_sample16 *sp1 = src_ptrs[0], *sp2 = src_ptrs[1];
		kdu_int32 downshift = step->downshift;
		kdu_int32 offset = (1 << downshift) >> 1;
		kdu_int32 val, i_lambda = step->icoeffs[0];
		if (i_lambda == 1)
			for (k = x_off; k < width; k++)
			{
				val = sp1[k].ival;  val += sp2[k].ival;
				out[k].ival = in[k].ival - (kdu_int16)((offset + val) >> downshift);
			}
		else if (i_lambda == -1)
			for (k = x_off; k < width; k++)
			{
				val = sp1[k].ival;  val += sp2[k].ival;
				out[k].ival = in[k].ival - (kdu_int16)((offset - val) >> downshift);
			}
		else
			for (k = x_off; k < width; k++)
			{
				val = sp1[k].ival;  val += sp2[k].ival;  val *= i_lambda;
				out[k].ival = in[k].ival - (kdu_int16)((offset + val) >> downshift);
			}
	}
	else
	{ // More general 16-bit processing
		kdu_int32 downshift = step->downshift;
		kdu_int32 offset = step->rounding_offset;
		int t, sum, *fpp, support = step->support_length;
		for (k = x_off; k < width; k++)
		{
			for (fpp = step->icoeffs, sum = offset, t = 0; t < support; t++)
				sum += fpp[t] * (src_ptrs[t])[k].ival;
			out[k].ival = in[k].ival - (kdu_int16)(sum >> downshift);
		}
	}
}

