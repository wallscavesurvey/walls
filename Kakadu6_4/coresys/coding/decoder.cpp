/*****************************************************************************/
// File: decoder.cpp [scope = CORESYS/CODING]
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
   Implements the functionality offered by the "kdu_decoder" object defined
in "kdu_sample_processing.h".  Includes ROI adjustments, dequantization,
subband sample buffering and geometric appearance transformations.
******************************************************************************/

#include <string.h>
#include <assert.h>
#include "kdu_arch.h" // Include architecture-specific info for speed-ups.
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "kdu_block_coding.h"
#include "kdu_kernels.h"

/* Note Carefully:
	  If you want to be able to use the "kdu_text_extractor" tool to
   extract text from calls to `kdu_error' and `kdu_warning' so that it
   can be separately registered (possibly in a variety of different
   languages), you should carefully preserve the form of the definitions
   below, starting from #ifdef KDU_CUSTOM_TEXT and extending to the
   definitions of KDU_WARNING_DEV and KDU_ERROR_DEV.  All of these
   definitions are expected by the current, reasonably inflexible
   implementation of "kdu_text_extractor".
	  The only things you should change when these definitions are ported to
   different source files are the strings found inside the `kdu_error'
   and `kdu_warning' constructors.  These strings may be arbitrarily
   defined, as far as "kdu_text_extractor" is concerned, except that they
   must not occupy more than one line of text.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
   kdu_error _name("E(decoder.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
   kdu_warning _name("W(decoder.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) kdu_error _name("Kakadu Core Error:\n");
#  define KDU_WARNING(_name,_id) kdu_warning _name("Kakadu Core Warning:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
// Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers

#if ((defined KDU_PENTIUM_MSVC) && (defined _WIN64))
# undef KDU_PENTIUM_MSVC
# ifndef KDU_X86_INTRINSICS
#   define KDU_X86_INTRINSICS // Use portable intrinsics instead
# endif
#endif // KDU_PENTIUM_MSVC && _WIN64

#ifndef KDU_NO_SSE
#  if defined KDU_X86_INTRINSICS
#    define KDU_SIMD_OPTIMIZATIONS
#    include "x86_coder_local.h"
#  elif defined KDU_PENTIUM_MSVC
#    define KDU_SIMD_OPTIMIZATIONS
#    include "msvc_coder_mmx_local.h"
#  elif defined KDU_PENTIUM_GCC
#    define KDU_SIMD_OPTIMIZATIONS
#    include "gcc_coder_mmx_local.h"
#  endif
#endif // !KDU_NO_SSE

/* ========================================================================= */
/*                   Local Class and Structure Definitions                   */
/* ========================================================================= */

/*****************************************************************************/
/*                                kd_decoder                                 */
/*****************************************************************************/

class kd_decoder : public kdu_pull_ifc_base, public kdu_worker {
public: // Member functions
	kd_decoder()
	{
		allocator = NULL; initialized = false; queue = NULL;
		lines16 = NULL; lines32 = NULL;
	}
	void init(kdu_subband band, kdu_sample_allocator *allocator,
		bool use_shorts, float normalization, int pull_offset,
		kdu_thread_env *env, kdu_thread_queue *queue);
protected: // These functions implement their namesakes in the base class
	virtual ~kd_decoder();
	virtual void start(kdu_thread_env *env);
	virtual void pull(kdu_line_buf &line, kdu_thread_env *env);
private: // Internal implementation
	virtual void do_job(kdu_thread_entity *ent, int job_idx);
	/* Each row of code-blocks is partitioned into `num_jobs_per_row' jobs,
	   and the code-block row together with the range of code-block indices
	   within that row which are to be processed by the job are deduced
	   from `job_idx' alone.  The `block_indices' array is considered
	   static and the `job_idx' is used to locate the block indices which
	   are to be processed. */
	void adjust_roi_background(kdu_block *block);
	/* Shifts up background samples after a block has been decoded. */
private: // Data -- Note: member variables chosen for minimum footprint.
	kdu_block_decoder block_decoder;
	kdu_subband band;
	kdu_int16 K_max; // Max magnitude bit-planes, excluding ROI upshift
	kdu_int16 K_max_prime; // Max magnitude bit-planes, including ROI upshift.
	bool reversible;
	bool initialized; // True once `start' has been called
	kdu_byte alignment_offset; // See below
	kdu_byte num_jobs_per_row; // Num parallel block decoding jobs per row
	float delta; // For irreversible compression only.
	kdu_dims block_indices; // Range of block indices.
	int subband_rows, subband_cols;
	kdu_uint16 secondary_seq; // See below
	kdu_int16 first_block_width;
	kdu_int16 first_block_height;
	kdu_int16 nominal_block_width;
	kdu_int16 nominal_block_height;
	kdu_int16 buffer_height; // > `nominal_block_height' if double buffered
	kdu_int16 next_buffered_line; // index of next line returned by `pull'
	kdu_int16 buffered_line_lim; // 1+idx of the last line available to `pull'
	int next_pull_block_row; // Row of code-blocks `pull' will use next
	kdu_sample_allocator *allocator;
	kdu_sample16 **lines16; // NULL or array of `buffer_height' pointers
	kdu_sample32 **lines32; // NULL or array of `buffer_height' pointers
	int pull_offset;
	kdu_thread_queue *queue; // Used only for multi-threaded processing
};
/* Notes:
	  Only one of the `lines16' and `lines32' members may be non-NULL,
   depending on whether the transform will be pulling 16- or 32-bit sample
   values from us.  The relevant array is pre-created during construction and
   actually allocated from the single block of memory associated with the
   `allocator' object during the first call to `pull' or `start'.
	  The `lines16' or `lines32' buffers may support either single
   buffering or double buffering.  In the former case,
   `buffer_height' <= `nominal_block_height' and only one buffer "bank"
   is used (see below).  Double buffering is implicitly identified by
   the case `buffer_height' > `nominal_block_height', regardless of
   the height of the first row of code-blocks in a subband.  Double
   buffering is of interest only in conjunction with multi-threading
   (i.e., a non-NULL `queue').  Since double buffering is costly, we
   do not apply it to all subbands.  A typical strategy is to double
   buffer only one subband in each resolution level.
	  The `secondary_seq' member is passed to `kdu_thread_env::add_jobs'.
   It is non-zero if and only if we are using double buffering.  In that
   case, the value is set to 1 + the resolution level to which this
   subband belongs (LL subband corresponds to resolution level 0).
	  The line buffers are organized into either one or two
   "banks"; since the single buffering case is obvious, we consider here
   only the double buffered scenario, in which there are two banks,
   identified as Bank 0 and Bank 1.  Bank 0 always starts at
   line 0, while bank 1 starts at line `nominal_block_height'.
	  The `do_job' function uses its `job_idx' argument to figure out
   which bank its code-block jobs belong to, together with the
   code-blocks to be processed within that bank, noting that each
   code-block row is partitioned into `num_jobs_per_row' jobs.  The
   particular code-block indices of relevance can be computed from the
   `block_indices' range (this does not evolve over time), together with
   the job index.  In this way, the only dynamic quantity which is
   used by `do_job' to locate and dimension its source data is the job
   index itself.  It is worth emphasizing that the `subband_rows' counter
   is neither accessed nor updated by the `do_job' function.
	  The `next_buffered_line' and `buffered_line_lim' members identify
   the row (within `lines16' or `lines32', as appropriate) to be used by
   the next call to `pull', along with an exclusive upper bound on this
   index.  The interval [`next_buffered_line',`buffered_line_lim')
   inevitably corresponds to a subset of one bank.  Once
   `next_buffered_line' reaches `buffered_line_lim', we have exhausted
   one bank of decoded subband samples.  When a call to `pull' produces
   this condition, the function does not return until it has launched
   new block decoding jobs.  When a call to `pull' encouters this condition
   on entry, it must first wait for a new bank to be generated, by calling
   `kdu_thread_entity::process_jobs'.
	  The `alignment_offset' member maintains state information between
   the constructor and the point at which the line buffers are actually
   initialized.  The allocator provides line buffers which are aligned
   on 16-byte boundaries, but what we want for optimal memory transactions
   is for the code-block boundaries to be 16-byte aligned.  The
   `alignment_offset' value gives the number of samples at the start of
   each allocated line buffer which should be skipped in order to ensure
   that code-block boundaries are 16-byte aligned.  In the event that the
   subband does not contain any horizontal code-block boundaries, this
   offset will be 0. */


   /* ========================================================================= */
   /*                                kdu_decoder                                */
   /* ========================================================================= */

   /*****************************************************************************/
   /*                          kdu_decoder::kdu_decoder                         */
   /*****************************************************************************/

kdu_decoder::kdu_decoder(kdu_subband band, kdu_sample_allocator *allocator,
	bool use_shorts, float normalization,
	int pull_offset, kdu_thread_env *env,
	kdu_thread_queue *env_queue)
	// In the future, we may create separate, optimized objects for each kernel.
{
	kd_decoder *dec = new kd_decoder;
	state = dec;
	dec->init(band, allocator, use_shorts, normalization, pull_offset, env, env_queue);
}

/* ========================================================================= */
/*                               kd_decoder                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                            kd_decoder::init                               */
/*****************************************************************************/

void
kd_decoder::init(kdu_subband band, kdu_sample_allocator *allocator,
	bool use_shorts, float normalization, int pull_offset,
	kdu_thread_env *env, kdu_thread_queue *env_queue)
{
	assert((this->allocator == NULL) && (this->queue == NULL));
	this->band = band;

	K_max = (kdu_int16)band.get_K_max();
	K_max_prime = (kdu_int16)band.get_K_max_prime();
	assert(K_max_prime >= K_max);
	reversible = band.get_reversible();
	initialized = false;
	delta = band.get_delta() * normalization;

	kdu_dims dims;
	band.get_dims(dims);
	kdu_coords nominal_block_size, first_block_size;
	band.get_block_size(nominal_block_size, first_block_size);
	band.get_valid_blocks(block_indices);

	subband_cols = dims.size.x;
	subband_rows = dims.size.y;
	first_block_width = (kdu_int16)first_block_size.x;
	first_block_height = (kdu_int16)first_block_size.y;
	nominal_block_width = (kdu_int16)nominal_block_size.x;
	nominal_block_height = (kdu_int16)nominal_block_size.y;

	if ((env != NULL) && (subband_rows > 0) && (subband_cols > 0))
		queue = env->add_queue(this, env_queue, "block decoder");

	// Determine how to partition a row of code-blocks into decoding jobs
	num_jobs_per_row = 1;
	if ((queue != NULL) && (env->get_num_threads() > 1))
	{ // See if we should split up the code-blocks on a row
		kdu_long block_row_samples = subband_cols;
		if (subband_rows < (int)nominal_block_height)
			block_row_samples *= subband_rows;
		else
			block_row_samples *= nominal_block_height;
		int num_per_row = (int)(block_row_samples / 8192);
		assert(num_per_row <= block_indices.size.x);
		if (num_per_row > 32)
			num_per_row = 32;
		else if (num_per_row < 1)
			num_per_row = 1;
		num_jobs_per_row = (kdu_byte)num_per_row;
	}

	// Determine whether or not to use double buffering, and hence buffer height
	secondary_seq = 0; // Changed further down if we settle on double buffering
	buffer_height = nominal_block_height;
	if (subband_rows <= (int)buffer_height)
		buffer_height = (kdu_int16)subband_rows;
	else if ((queue != NULL) && (env->get_num_threads() > 1))
	{ // We might want to use double buffering in this case.
		int ideal_double_buffered_bands = // ceil(9/num_jobs_per_row)
			1 + (8 / num_jobs_per_row);
		if (band.get_band_idx() <= ideal_double_buffered_bands)
		{ // Decided to use double buffering for this subband.  Note that,
		  // except for the LL subband, band indices start at 1.
			if ((subband_rows - first_block_height) < (int)nominal_block_height)
				buffer_height += (kdu_int16)(subband_rows - first_block_height);
			else
				buffer_height += nominal_block_height;
			secondary_seq = (kdu_uint16)
				(64 - band.access_resolution().get_dwt_level());
		}
	}

	// Set up counters
	next_buffered_line = 0;
	buffered_line_lim = 0; // No available buffered lines right now
	next_pull_block_row = 0;

	// Establish buffer configuration
	alignment_offset = 0;
	if (first_block_size.x < subband_cols)
		alignment_offset = (kdu_byte)
		((use_shorts) ? ((-first_block_size.x) & 7) : ((-first_block_size.x) & 3));

	lines16 = NULL;
	lines32 = NULL;
	this->allocator = NULL;
	if (!dims)
	{
		subband_rows = 0; return;
	}
	this->allocator = allocator;
	allocator->pre_alloc(use_shorts, 0, subband_cols + 3 + ((int)alignment_offset),
		buffer_height);
	// Note the extra 3 samples, for hassle free quad-word transfers
	// and the alignment offset so that code-block boundaries can be
	// 16-byte aligned.
	if (use_shorts)
		lines16 = new kdu_sample16 *[buffer_height];
	else
		lines32 = new kdu_sample32 *[buffer_height];
	this->pull_offset = pull_offset;
}

/*****************************************************************************/
/*                          kd_decoder::~kd_decoder                          */
/*****************************************************************************/

kd_decoder::~kd_decoder()
{
	if (lines16 != NULL)
		delete[] lines16;
	if (lines32 != NULL)
		delete[] lines32;
}

/*****************************************************************************/
/*                             kd_decoder::start                             */
/*****************************************************************************/

void
kd_decoder::start(kdu_thread_env *env)
{
	if (initialized || (subband_cols == 0) || (subband_rows == 0))
	{
		initialized = true;
		return;
	}

	if (env != NULL)
		env->acquire_lock(KD_THREADLOCK_ALLOCATOR);
	int n;
	if (lines16 != NULL)
		for (n = 0; n < buffer_height; n++)
			lines16[n] = allocator->alloc16(0, subband_cols + 3 + (int)alignment_offset)
			+ alignment_offset;
	else
		for (n = 0; n < buffer_height; n++)
			lines32[n] = allocator->alloc32(0, subband_cols + 3 + (int)alignment_offset)
			+ alignment_offset;
	initialized = true;
	if (env != NULL)
	{
		env->release_lock(KD_THREADLOCK_ALLOCATOR);
		bool finalize_queue = (subband_rows <= ((int)first_block_height));
		env->add_jobs(queue, num_jobs_per_row, finalize_queue);
		// Launch only primary jobs here
	}
}

/*****************************************************************************/
/*                              kd_decoder::pull                             */
/*****************************************************************************/

void
kd_decoder::pull(kdu_line_buf &line, kdu_thread_env *env)
{
	assert((queue == NULL) || (env != NULL));
	if (line.get_width() <= pull_offset)
		return;

	if (!initialized)
		start(env);

	assert(subband_rows > 0);
	if (next_buffered_line == buffered_line_lim)
	{ // Perform block decoding (or wait until it is done) as required
		next_buffered_line = 0;
		if (queue != NULL)
		{
			if (secondary_seq != 0)
			{ // Double buffering in use
				assert(buffer_height > nominal_block_height);
				if (next_pull_block_row == 0)
				{ // `start' created jobs for bank 0; add bank 1 jobs here.
					bool finalize = (subband_rows <= (int)
						(first_block_height + nominal_block_height));
					env->add_jobs(queue, num_jobs_per_row, finalize, secondary_seq);
				}
				else if (buffered_line_lim <= nominal_block_height)
					next_buffered_line = nominal_block_height;
			}
			env->process_jobs(queue); // Waits for primary jobs.
		}
		else
		{
			assert(num_jobs_per_row == 1);
			do_job(env, next_pull_block_row);
		}
		if (next_pull_block_row == 0)
			buffered_line_lim = first_block_height;
		else if (subband_rows < (int)nominal_block_height)
			buffered_line_lim = next_buffered_line + (kdu_int16)subband_rows;
		else
			buffered_line_lim = next_buffered_line + nominal_block_height;
		assert(buffered_line_lim <= buffer_height);
		next_pull_block_row++;
	}

	// Transfer data
	assert(line.get_width() == (subband_cols + pull_offset));
	if (lines32 != NULL)
		memcpy(line.get_buf32() + pull_offset, lines32[next_buffered_line],
		(size_t)(subband_cols << 2));
	else
		memcpy(line.get_buf16() + pull_offset, lines16[next_buffered_line],
		(size_t)(subband_cols << 1));

	// Finally, update the line buffer management state.
	subband_rows--;
	next_buffered_line++;
	if ((queue == NULL) || (next_buffered_line < buffered_line_lim))
		return;

	// We have finished with this memory bank; can use it to launch more jobs
	if (subband_rows <= 0)
		return; // Should never actually be < 0.
	if (secondary_seq != 0)
	{ // Using double buffering within this subband
		assert(buffer_height > nominal_block_height);
		if (subband_rows <= (int)nominal_block_height)
		{ // There are no more block rows to decode, but we need to promote
		  // any existing secondary jobs to the primary service state
			env->add_jobs(queue, 0, true);
		}
		else
		{ // Add a new set of block decoding jobs in the secondary service
		  // state, promoting the jobs we added last time to the primary state.
			bool finalize = (subband_rows <= (int)(nominal_block_height << 1));
			env->add_jobs(queue, num_jobs_per_row, finalize, secondary_seq);
		}
	}
	else
	{ // Single-buffered; just add jobs in the primary service state.
		bool finalize = (subband_rows <= (int)(nominal_block_height));
		env->add_jobs(queue, num_jobs_per_row, finalize);
	}
}

/*****************************************************************************/
/*                             kd_decoder::do_job                            */
/*****************************************************************************/

void
kd_decoder::do_job(kdu_thread_entity *ent, int job_idx)
{
	kdu_thread_env *env = (kdu_thread_env *)ent;
	int jobs_per_row = num_jobs_per_row;
	int blocks_remaining = block_indices.size.x;
	int offset = 0; // Horizontal offset into line buffers
	kdu_coords idx = block_indices.pos; // Index of first block to process
	kdu_sample32 **buf32 = lines32; // First buffer line to process (32-bit)
	kdu_sample16 **buf16 = lines16; // First buffer line to process (16-bit)

	// Start by using `job_idx' to find where we are
	int decoder_block_row = job_idx / jobs_per_row;
	if ((decoder_block_row & 1) && (buffer_height > nominal_block_height))
	{ // Select memory bank 1
		if (buf32 != NULL) buf32 += nominal_block_height;
		if (buf16 != NULL) buf16 += nominal_block_height;
	}
	idx.y += decoder_block_row;
	assert(idx.y < (block_indices.pos.y + block_indices.size.y));
	if (jobs_per_row > 1)
	{
		int job_idx_in_row = job_idx - (decoder_block_row*jobs_per_row);
		int skip_blocks = (blocks_remaining*job_idx_in_row) / jobs_per_row;
		blocks_remaining =
			((blocks_remaining*(job_idx_in_row + 1)) / jobs_per_row) - skip_blocks;
		assert(blocks_remaining > 0);
		if (skip_blocks > 0)
		{
			idx.x += skip_blocks;
			offset += first_block_width +
				(skip_blocks - 1) * (int)nominal_block_width;
		}
	}

	// Now scan through the blocks to process
	kdu_coords xfer_size;
	kdu_block *block;

	for (; blocks_remaining > 0; blocks_remaining--,
		idx.x++, offset += xfer_size.x)
	{
		block = band.open_block(idx, NULL, env);
		if (block->num_passes > 0)
			block_decoder.decode(block);
		xfer_size = block->region.size;
		if (block->transpose)
			xfer_size.transpose();
		assert((xfer_size.x + offset) <= subband_cols);
		int m, n;
		if (block->num_passes == 0)
		{ /* Fill block region with 0's.  Note the unrolled loops and the
			 fact that there is no need to worry if the code zeros out as
			 many as 3 samples beyond the end of each block line, since the
			 buffers were allocated with a bit of extra space at the end. */
			if (buf32 != NULL)
			{ // 32-bit samples
				kdu_sample32 *dp;
				for (m = 0; m < xfer_size.y; m++)
					if (reversible)
						for (dp = buf32[m] + offset, n = xfer_size.x; n > 0; n -= 4, dp += 4)
						{
							dp[0].ival = 0; dp[1].ival = 0; dp[2].ival = 0; dp[3].ival = 0;
						}
					else
						for (dp = buf32[m] + offset, n = xfer_size.x; n > 0; n -= 4, dp += 4)
						{
							dp[0].fval = 0.0F; dp[1].fval = 0.0F;
							dp[2].fval = 0.0F; dp[3].fval = 0.0F;
						}
			}
			else
			{ // 16-bit samples
				kdu_sample16 *dp;
				for (m = 0; m < xfer_size.y; m++)
					for (dp = buf16[m] + offset, n = xfer_size.x; n > 0; n -= 4, dp += 4)
					{
						dp[0].ival = 0; dp[1].ival = 0; dp[2].ival = 0; dp[3].ival = 0;
					}
			}
		}
		else
		{ // Need to dequantize and/or convert quantization indices.
			if (K_max_prime > K_max)
				adjust_roi_background(block);
			int row_gap = block->size.x;
			kdu_int32 *spp = block->sample_buffer + block->region.pos.y*row_gap;
#ifdef KDU_SIMD_OPTIMIZATIONS
			if ((block->region.pos.x == 0) && (row_gap == xfer_size.x) &&
				((xfer_size.x == 32) || (xfer_size.x == 64)) &&
				(!(block->vflip | block->hflip | block->transpose)) &&
				(buf16 != NULL) &&
				simd_xfer_decoded_block(spp, buf16, offset, xfer_size.x,
					xfer_size.y, reversible, K_max, delta))
			{
				band.close_block(block, env);
				continue;
			}
#endif // KDU_SIMD_OPTIMIZATIONS
			spp += block->region.pos.x;
			kdu_int32 *sp;
			int m_start = 0, m_inc = 1, n_start = offset, n_inc = 1;
			if (block->vflip)
			{
				m_start += xfer_size.y - 1; m_inc = -1;
			}
			if (block->hflip)
			{
				n_start += xfer_size.x - 1; n_inc = -1;
			}
			if (buf32 != NULL)
			{ // Need to generate 32-bit dequantized values.
				kdu_sample32 *dp, **dpp = buf32 + m_start;
				if (reversible)
				{ // Output is 32-bit absolute integers.
					kdu_int32 val;
					kdu_int32 downshift = 31 - K_max;
					if (downshift < 0)
					{
						KDU_ERROR(e, 0); e <<
							KDU_TXT("Insufficient implementation "
								"precision available for true reversible processing!");
					}
					if (!block->transpose)
						for (m = xfer_size.y; m--; dpp += m_inc, spp += row_gap)
							for (dp = (*dpp) + n_start, sp = spp,
								n = xfer_size.x; n--; sp++, dp += n_inc)
							{
								val = *sp;
								if (val < 0)
									dp->ival = -((val & KDU_INT32_MAX) >> downshift);
								else
									dp->ival = val >> downshift;
							}
					else
						for (m = xfer_size.y; m--; dpp += m_inc, spp++)
							for (dp = (*dpp) + n_start, sp = spp,
								n = xfer_size.x; n--; sp += row_gap, dp += n_inc)
							{
								val = *sp;
								if (val < 0)
									dp->ival = -((val & KDU_INT32_MAX) >> downshift);
								else
									dp->ival = val >> downshift;
							}
				}
				else
				{ // Output is true floating point values.
					kdu_int32 val;
					float scale = delta;
					if (K_max <= 31)
						scale /= (float)(1 << (31 - K_max));
					else
						scale *= (float)(1 << (K_max - 31)); // Can't decode all planes
					if (!block->transpose)
						for (m = xfer_size.y; m--; dpp += m_inc, spp += row_gap)
							for (dp = (*dpp) + n_start, sp = spp,
								n = xfer_size.x; n--; sp++, dp += n_inc)
							{
								val = *sp;
								if (val < 0)
									val = -(val & KDU_INT32_MAX);
								dp->fval = scale * val;
							}
					else
						for (m = xfer_size.y; m--; dpp += m_inc, spp++)
							for (dp = (*dpp) + n_start, sp = spp,
								n = xfer_size.x; n--; sp += row_gap, dp += n_inc)
							{
								val = *sp;
								if (val < 0)
									val = -(val & KDU_INT32_MAX);
								dp->fval = scale * val;
							}
				}
			}
			else
			{ // Need to produce 16-bit dequantized values.
				kdu_sample16 *dp, **dpp = buf16 + m_start;
				if (reversible)
				{ // Output is 16-bit absolute integers.
					kdu_int32 val;
					kdu_int32 downshift = 31 - K_max;
					assert(downshift >= 0); // Otherwise should be using 32 bits.
					if (!block->transpose)
						for (m = xfer_size.y; m--; dpp += m_inc, spp += row_gap)
							for (dp = (*dpp) + n_start, sp = spp,
								n = xfer_size.x; n--; sp++, dp += n_inc)
							{
								val = *sp;
								if (val < 0)
									dp->ival = (kdu_int16)
									-((val & KDU_INT32_MAX) >> downshift);
								else
									dp->ival = (kdu_int16)(val >> downshift);
							}
					else
						for (m = xfer_size.y; m--; dpp += m_inc, spp++)
							for (dp = (*dpp) + n_start, sp = spp,
								n = xfer_size.x; n--; sp += row_gap, dp += n_inc)
							{
								val = *sp;
								if (val < 0)
									dp->ival = (kdu_int16)
									-((val & KDU_INT32_MAX) >> downshift);
								else
									dp->ival = (kdu_int16)(val >> downshift);
							}
				}
				else
				{ // Output is 16-bit fixed point values.
					float fscale = delta * (float)(1 << KDU_FIX_POINT);
					if (K_max <= 31)
						fscale /= (float)(1 << (31 - K_max));
					else
						fscale *= (float)(1 << (K_max - 31));
					fscale *= (float)(1 << 16) * (float)(1 << 16);
					kdu_int32 val, scale = ((kdu_int32)(fscale + 0.5F));
					if (!block->transpose)
						for (m = xfer_size.y; m--; dpp += m_inc, spp += row_gap)
							for (dp = (*dpp) + n_start, sp = spp,
								n = xfer_size.x; n--; sp++, dp += n_inc)
							{
								val = *sp;
								if (val < 0)
									val = -(val & KDU_INT32_MAX);
								val = (val + (1 << 15)) >> 16; val *= scale;
								dp->ival = (kdu_int16)((val + (1 << 15)) >> 16);
							}
					else
						for (m = xfer_size.y; m--; dpp += m_inc, spp++)
							for (dp = (*dpp) + n_start, sp = spp,
								n = xfer_size.x; n--; sp += row_gap, dp += n_inc)
							{
								val = *sp;
								if (val < 0)
									val = -(val & KDU_INT32_MAX);
								val = (val + (1 << 15)) >> 16; val *= scale;
								dp->ival = (kdu_int16)((val + (1 << 15)) >> 16);
							}
				}
			}
		}
		band.close_block(block, env);
	}
}

/*****************************************************************************/
/*                     kd_decoder::adjust_roi_background                     */
/*****************************************************************************/

void
kd_decoder::adjust_roi_background(kdu_block *block)
{
	kdu_int32 upshift = K_max_prime - K_max;
	kdu_int32 mask = (-1) << (31 - K_max); mask &= KDU_INT32_MAX;
	kdu_int32 *sp = block->sample_buffer;
	kdu_int32 val;
	int num_samples = ((block->size.y + 3) >> 2) * (block->size.x << 2);

	for (int n = num_samples; n--; sp++)
		if ((((val = *sp) & mask) == 0) && (val != 0))
		{
			if (val < 0)
				*sp = (val << upshift) | KDU_INT32_MIN;
			else
				*sp <<= upshift;
		}
}
