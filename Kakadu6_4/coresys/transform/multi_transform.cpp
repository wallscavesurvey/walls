/*****************************************************************************/
// File: multi_transform.cpp [scope = CORESYS/TRANSFORMS]
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
   Implements `kdu_multi_analysis' and `kdu_multi_synthesis'.
******************************************************************************/

#include <assert.h>
#include <string.h>
#include <math.h>
#include "kdu_arch.h" // Include architecture info for any speedups
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "kdu_roi_processing.h"
#include "multi_transform_local.h"

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
   kdu_error _name("E(multi_transform.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
   kdu_warning _name("W(multi_transform.cpp)",_id);
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


/* ========================================================================= */
/*                               kd_multi_line                               */
/* ========================================================================= */

/*****************************************************************************/
/*                            kd_multi_line::reset                           */
/*****************************************************************************/

void
kd_multi_line::reset(int rev_off, float irrev_off)
{
	kdu_sample32 *dp32 = line.get_buf32();
	kdu_sample16 *dp16 = line.get_buf16();
	assert(line.is_absolute() == reversible);
	if (reversible)
	{
		if (dp16 != NULL)
		{ // Reversible 16-bit integers
			if (rev_off == 0)
				memset(dp16, 0, (size_t)(size.x << 1));
			else
				for (int n = size.x; n > 0; n--)
					(dp16++)->ival = (kdu_int16)rev_off;
		}
		else if (dp32 != NULL)
		{ // Reversible 32-bit integers
			if (rev_off == 0)
				memset(dp32, 0, (size_t)(size.x << 2));
			else
				for (int n = size.x; n > 0; n--)
					(dp32++)->ival = rev_off;
		}
		else
			assert(0);
	}
	else
	{
		if (dp16 != NULL)
		{ // Irreversible fixed-point words
			if (irrev_off == 0.0F)
				memset(dp16, 0, (size_t)(size.x << 1));
			else
			{
				kdu_int16 off = (kdu_int16)
					floor(0.5 + irrev_off * (1 << KDU_FIX_POINT));
				for (int n = size.x; n > 0; n--)
					(dp16++)->ival = off;
			}
		}
		else
		{ // Irreversible floating-point words
			for (int n = size.x; n > 0; n--)
				(dp32++)->fval = irrev_off;
		}
	}
}

/*****************************************************************************/
/*                         kd_multi_line::apply_offset                       */
/*****************************************************************************/

void
kd_multi_line::apply_offset(int rev_off, float irrev_off)
{
	int n;
	kdu_sample32 *dp32 = line.get_buf32();
	kdu_sample16 *dp16 = line.get_buf16();
	if (reversible)
	{
		if (rev_off == 0)
			return;
		if (dp32 != NULL)
		{
			for (n = size.x; n > 0; n--, dp32++)
				dp32->ival += rev_off;
		}
		else
		{
			for (n = size.x; n > 0; n--, dp16++)
				dp16->ival += (kdu_int16)rev_off;
		}
	}
	else
	{
		if (irrev_off == 0.0F)
			return;
		if (dp32 != NULL)
		{
			for (n = size.x; n > 0; n--, dp32++)
				dp32->fval += irrev_off;
		}
		else
		{
			kdu_int16 off = (kdu_int16)
				floor(0.5 + irrev_off * (1 << KDU_FIX_POINT));
			for (n = size.x; n > 0; n--, dp16++)
				dp16->ival += off;
		}
	}
}

/*****************************************************************************/
/*                            kd_multi_line::copy                            */
/*****************************************************************************/

void
kd_multi_line::copy(kd_multi_line *src, int rev_offset, float irrev_offset)
{
	int n;
	assert(src->size.x == size.x);
	kdu_sample32 *dp32 = line.get_buf32();
	kdu_sample16 *dp16 = line.get_buf16();
	kdu_sample32 *sp32 = src->line.get_buf32();
	kdu_sample16 *sp16 = src->line.get_buf16();
	if (reversible)
	{
		assert(src->reversible);
		if (dp32 != NULL)
		{
			for (n = size.x; n > 0; n--, dp32++, sp32++)
				dp32->ival = sp32->ival + rev_offset;
		}
		else
		{
			for (n = size.x; n > 0; n--, dp16++, sp16++)
				dp16->ival = sp16->ival + rev_offset;
		}
	}
	else
	{
		if (dp32 != NULL)
		{
			if (src->reversible)
			{ // Convert to irreversible representation on the way through
				float factor = 1.0F / (float)(1 << bit_depth);
				for (n = size.x; n > 0; n--, dp32++, sp32++)
					dp32->fval = sp32->ival*factor + irrev_offset;
			}
			else
			{
				if (src->bit_depth == this->bit_depth)
				{
					for (n = size.x; n > 0; n--, dp32++, sp32++)
						dp32->fval = sp32->fval + irrev_offset;
				}
				else
				{
					float factor = (1 << src->bit_depth) / (float)(1 << bit_depth);
					for (n = size.x; n > 0; n--, dp32++, sp32++)
						dp32->fval = sp32->fval*factor + irrev_offset;
				}
			}
		}
		else
		{
			kdu_int16 off16 = (kdu_int16)
				floor(0.5 + irrev_offset * (1 << KDU_FIX_POINT));
			int upshift = ((src->reversible) ? KDU_FIX_POINT : (src->bit_depth))
				- this->bit_depth;
			if (upshift == 0)
			{
				for (n = size.x; n > 0; n--, dp16++, sp16++)
					dp16->ival = sp16->ival + off16;
			}
			else if (upshift > 0)
			{
				for (n = size.x; n > 0; n--, dp16++, sp16++)
					dp16->ival = (sp16->ival << upshift) + off16;
			}
			else
			{
				int downshift = -upshift;
				int off32 = (((int)off16) << downshift) + (1 << (downshift - 1));
				for (n = size.x; n > 0; n--, dp16++, sp16++)
					dp16->ival = (kdu_int16)((off32 + sp16->ival) >> downshift);
			}
		}
	}
}


/* ========================================================================= */
/*                             kd_multi_transform                            */
/* ========================================================================= */

/*****************************************************************************/
/*                   kd_multi_transform::~kd_multi_transform                 */
/*****************************************************************************/

kd_multi_transform::~kd_multi_transform()
{
	while ((block_tail = xform_blocks) != NULL)
	{
		xform_blocks = block_tail->next;
		delete block_tail;
	}
	while ((output_collection = codestream_collection) != NULL)
	{
		codestream_collection = output_collection->next;
		delete output_collection;
	}
	if (constant_output_lines != NULL)
		delete[] constant_output_lines;
	if (codestream_components != NULL)
		delete[] codestream_components;
	if (scratch_ints != NULL)
		delete[] scratch_ints;
	if (scratch_floats != NULL)
		delete[] scratch_floats;
}

/*****************************************************************************/
/*                        kd_multi_transform::construct                      */
/*****************************************************************************/

void
kd_multi_transform::construct(kdu_codestream codestream, kdu_tile tile,
	bool force_precise, bool skip_ycc, bool want_fastest,
	int processing_stripe_height, kdu_thread_env *env,
	kdu_thread_queue *env_queue, bool double_buffering)
{
	this->use_ycc = (!skip_ycc) && tile.get_ycc();

	int n;
	int num_stage_inputs, num_stage_outputs;
	int num_block_inputs, num_block_outputs;

	// Start by configuring the codestream input component collection
	kd_multi_collection *collection = new kd_multi_collection;
	codestream_collection = output_collection = collection;
	if (!tile.get_mct_block_info(0, 0, num_stage_inputs, num_stage_outputs,
		num_block_inputs, num_block_outputs))
		assert(0); // Codestream machinery always defines at least one (dummy)
				   // stage, for simplicity;
	collection->num_components = num_stage_inputs;
	collection->components = new kd_multi_line *[num_stage_inputs];
	codestream_components = new kd_multi_component[num_stage_inputs];
	if (env != NULL)
	{
		this->env_queue = env_queue;
		for (n = 0; n < num_stage_inputs; n++)
			codestream_components[n].env_queue =
			env->add_queue(codestream_components + n, env_queue,
				"component processor");
	}
	int *tmp_comp_indices = get_scratch_ints(num_stage_inputs);
	tile.get_mct_block_info(0, 0, num_stage_inputs, num_stage_outputs,
		num_block_inputs, num_block_outputs,
		NULL, NULL, NULL, NULL, tmp_comp_indices);
	for (n = 0; n < num_stage_inputs; n++)
	{
		kd_multi_component *comp = codestream_components + n;
		comp->comp_idx = tmp_comp_indices[n];
		collection->components[n] = &(comp->line);
		comp->line.collection_idx = n;
		kdu_tile_comp tc =
			tile.access_component(comp->comp_idx);
		if (tc.get_reversible())
			comp->line.reversible = true;
		else
			comp->line.need_irreversible = true;

		if (force_precise)
			comp->line.need_precise = true;
		else if (tc.get_bit_depth(true) <= 16)
			comp->line.need_precise = false;
		else if (comp->line.reversible)
			comp->line.need_precise = true;
		else
			comp->line.need_precise = !want_fastest;

		comp->line.bit_depth = tc.get_bit_depth(false);
		kdu_resolution res = tc.access_resolution();
		kdu_dims dims; res.get_dims(dims);
		comp->line.size = dims.size;
		comp->double_buffering =
			((env == NULL) || (env->get_num_threads() < 2) ||
			(num_stage_inputs < 2) || !dims) ? false : double_buffering;
		comp->max_available_rows = comp->num_buffer_rows = processing_stripe_height;
		if (comp->double_buffering)
			comp->num_buffer_rows *= 2;
		else if (env != NULL)
			env->add_jobs(comp->env_queue, 0, true); // Make it clear that no jobs
												   // will be added to this queue.
		if (comp->num_buffer_rows > comp->line.size.y)
		{
			comp->num_buffer_rows = comp->line.size.y;
			comp->max_available_rows = comp->num_buffer_rows;
			if (comp->double_buffering)
				comp->max_available_rows = (comp->num_buffer_rows + 1) >> 1;
		}
		comp->buffer = new kdu_line_buf[comp->num_buffer_rows];
	}

	// Now configure the remaining component collections and transform blocks.
	int s, b;
	kd_multi_block *block;
	for (s = 0; tile.get_mct_block_info(s, 0, num_stage_inputs, num_stage_outputs,
		num_block_inputs, num_block_outputs); s++)
	{
		assert(collection->num_components == num_stage_inputs);
		collection = new kd_multi_collection;
		collection->prev = output_collection;
		output_collection = output_collection->next = collection;
		collection->num_components = num_stage_outputs;
		collection->components = new kd_multi_line *[num_stage_outputs];
		for (n = 0; n < num_stage_outputs; n++)
			collection->components[n] = NULL;

		b = 0;
		do {
			bool block_is_reversible, symmetric, symmetric_ext;
			int num_levels, canvas_min, canvas_lim, num_steps;
			const float *dwt_coeffs;

			if (tile.get_mct_matrix_info(s, b))
				block = new kd_multi_matrix_block;
			else if (tile.get_mct_rxform_info(s, b))
				block = new kd_multi_rxform_block;
			else if (tile.get_mct_dependency_info(s, b, block_is_reversible))
				block = new kd_multi_dependency_block(block_is_reversible);
			else if (tile.get_mct_dwt_info(s, b, block_is_reversible,
				num_levels, canvas_min, canvas_lim,
				num_steps, symmetric, symmetric_ext,
				dwt_coeffs) != NULL)
				block = new kd_multi_dwt_block();
			else
				block = new kd_multi_null_block();

			block->prev = block_tail;
			if (block_tail == NULL)
				xform_blocks = block_tail = block;
			else
				block_tail = block_tail->next = block;
			block->initialize(s, b, tile, num_block_inputs, num_block_outputs,
				collection->prev, collection, this);
		} while (tile.get_mct_block_info(s, ++b, num_stage_inputs,
			num_stage_outputs, num_block_inputs, num_block_outputs));
	}

	// Find out how many constant output lines there are -- these need to be
	// assigned line resources from the `constant_output_lines' array
	int cscan, num_constant_output_lines = 0;
	for (n = 0; n < output_collection->num_components; n++)
		if (output_collection->components[n] == NULL)
			num_constant_output_lines++;
	if (num_constant_output_lines > 0)
		constant_output_lines = new kd_multi_line[num_constant_output_lines];

	// Set output component sizes and bit-depths.
	for (cscan = 0, n = 0; n < output_collection->num_components; n++)
	{
		kd_multi_line *line = output_collection->components[n];
		if (line == NULL)
		{
			line = output_collection->components[n] =
				constant_output_lines + (cscan++);
			line->need_precise = force_precise;
			line->need_irreversible = true; // Use irreversible as the default
			line->is_constant = true;
		}
		line->num_consumers++;
		kdu_dims dims;
		codestream.get_tile_dims(tile.get_tile_idx(), n, dims, true);
		line->size = dims.size;
		line->bit_depth = codestream.get_bit_depth(n, true);
	}

	// Now pass through the network as often as required, until the
	// reversibility, bit-depth and `need_precise' flags of all blocks and
	// lines becomes stable.  This could take a few iterations, if there are
	// null lines.
	while (propagate_knowledge(force_precise, false));

	// Now normalize the coefficients based on available bit-depth information
	for (block = xform_blocks; block != NULL; block = block->next)
	{
		for (n = 0; n < block->num_components; n++)
		{
			kd_multi_line *line = block->components + n;
			if ((line->bit_depth == 0) || (line->irrev_offset == 0.0F))
				continue; // Normalization is not possible or not required
			line->irrev_offset *= 1.0F / ((float)(1 << line->bit_depth));
		}
		block->normalize_coefficients();
	}

	// Re-run the `propagate_knowledge' function in case coefficient
	// normalization encountered a problem which caused it to adjust the
	// `need_precise' flag for one or more lines.
	while (propagate_knowledge(false, false));
	while (propagate_knowledge(false, true));

	// Now adjust offsets to make sure that output components are signed.
	for (n = 0; n < output_collection->num_components; n++)
	{
		kd_multi_line *line = output_collection->components[n];
		if (!codestream.get_signed(n, true))
		{
			line->rev_offset -= ((1 << line->bit_depth) >> 1);
			line->irrev_offset -= 0.5F;
		}
	}

	if (num_constant_output_lines > 0)
	{ // See if we can combine some of the constant output lines
		for (n = 0; n < output_collection->num_components; n++)
		{
			kd_multi_line *line = output_collection->components[n];
			if ((line > constant_output_lines) &&
				(line < (constant_output_lines + num_constant_output_lines)))
			{ // Candidate for replacement
				assert(line->is_constant && !line->reversible);
				kd_multi_line *ref = constant_output_lines;
				for (; ref < line; ref++)
					if ((ref->size.x == line->size.x) &&
						(ref->irrev_offset == line->irrev_offset))
					{
						output_collection->components[n] = ref;
						break;
					}
			}
		}
	}

	// Finally, see if we can bypass copying in null transforms.
	for (block = xform_blocks; block != NULL; block = block->next)
	{
		for (n = 0; n < block->num_dependencies; n++)
		{
			while ((block->dependencies[n] != NULL) &&
				(block->dependencies[n]->bypass != NULL))
				block->dependencies[n] = block->dependencies[n]->bypass;
		}
		if (block->is_null_transform)
			for (n = 0; n < block->num_components; n++)
			{
				kd_multi_line *tgt_line = block->components + n;
				if (tgt_line->is_constant)
					continue;
				kd_multi_line *src_line = block->dependencies[n];
				assert((n < block->num_dependencies) && (src_line != NULL));
				bool have_offset = (tgt_line->reversible) ?
					(tgt_line->rev_offset != 0) : (tgt_line->irrev_offset != 0.0F);
				assert(src_line->bypass == NULL);
				assert(tgt_line->reversible == src_line->reversible);
				if (have_offset && (src_line->num_consumers > 1))
					continue; // Bypassing could cause conflicts
				if ((!src_line->reversible) &&
					(src_line->bit_depth != tgt_line->bit_depth))
					continue; // Bypassing will cause bit-depth inconsistencies
				tgt_line->bypass = src_line;
				src_line->rev_offset += tgt_line->rev_offset;
				src_line->irrev_offset += tgt_line->irrev_offset;
				src_line->num_consumers += tgt_line->num_consumers - 1;
			}
	}
	for (collection = codestream_collection->next;
		collection != NULL; collection = collection->next)
		for (n = 0; n < collection->num_components; n++)
		{
			while ((collection->components[n] != NULL) &&
				(collection->components[n]->bypass != NULL))
				collection->components[n] = collection->components[n]->bypass;
		}
}

/*****************************************************************************/
/*                  kd_multi_transform::propagate_knowledge                  */
/*****************************************************************************/

bool
kd_multi_transform::propagate_knowledge(bool force_precise,
	bool force_hanging_constants_to_reversible)
{
	int n;
	kd_multi_block *block;
	bool any_change = false;
	bool found_size_inconsistency = false;
	bool found_bit_depth_inconsistency = false;

	if (use_ycc)
	{ // Ensure consistency amongst the first 3 components in the codestream
	  // collection.
		assert(codestream_collection->num_components >= 3);
		kd_multi_line **lines = codestream_collection->components;
		bool reversible = false, irreversible = false, precise = force_precise;
		for (n = 0; n < 3; n++)
		{
			if (lines[n]->reversible)
				reversible = true;
			if (lines[n]->need_irreversible)
				irreversible = true;
			if (lines[n]->need_precise)
				precise = true;
		}
		for (n = 0; n < 3; n++)
		{
			lines[n]->reversible = reversible;
			lines[n]->need_irreversible = irreversible;
			lines[n]->need_precise = precise;
			if (lines[n]->size != lines[0]->size)
				found_size_inconsistency = true;
		}
	}

	for (block = xform_blocks;
		(block != NULL) && !found_size_inconsistency;
		block = block->next)
	{
		if (block->is_null_transform)
		{ // These pass components through individually; they have no size
		  // of their own.
			assert(block->num_dependencies <= block->num_components);
			// The `kd_multi_null_block::initialize' function should have
			// made this condition hold.
			for (n = 0; n < block->num_components; n++)
			{
				kd_multi_line *line = block->components + n;
				if (force_precise)
					line->need_precise = true;
				if (!line->is_constant)
				{
					kd_multi_line *dep = block->dependencies[n];
					if (dep->need_precise != line->need_precise)
					{
						any_change = true;
						line->need_precise = dep->need_precise = true;
					}
					if (dep->need_irreversible != line->need_irreversible)
					{
						any_change = true;
						line->need_irreversible = dep->need_irreversible = true;
					}
					if (dep->reversible != line->reversible)
					{
						any_change = true;
						line->reversible = dep->reversible = true;
					}
					if (dep->size != line->size)
					{
						any_change = true;
						if ((dep->size.x == 0) && (dep->size.y == 0))
							dep->size = line->size;
						else if ((line->size.x == 0) && (line->size.y == 0))
							line->size = dep->size;
						else
							found_size_inconsistency = true;
					}
					if (dep->bit_depth != line->bit_depth)
					{
						any_change = true;
						if (dep->bit_depth == 0)
							dep->bit_depth = line->bit_depth;
						else if (line->bit_depth == 0)
							line->bit_depth = dep->bit_depth;
						else
							found_bit_depth_inconsistency = true;
					}
				}
				else if (force_hanging_constants_to_reversible &&
					(!line->need_irreversible) && (!line->reversible))
					line->reversible = any_change = true;
			}
		}
		else
		{ // Regular block transform; hi/lo precision and size of all
		  // components manipulated by the transform must be consistent.
			bool xform_needs_precise = force_precise;
			bool have_xform_size = false;
			kdu_coords xform_size;
			bool have_unknown_input_bit_depth = false;
			bool have_unknown_output_bit_depth = false;
			kd_multi_line *line;
			for (n = 0; n < block->num_dependencies; n++)
			{
				if ((line = block->dependencies[n]) == NULL)
					continue;
				if (line->need_precise)
					xform_needs_precise = true;
				if ((line->size != xform_size) && !have_xform_size)
				{
					xform_size = line->size;  have_xform_size = true;
				}
				if (line->bit_depth == 0)
					have_unknown_input_bit_depth = true;
			}
			for (n = 0; n < block->num_components; n++)
			{
				line = block->components + n;
				if (line->need_precise)
					xform_needs_precise = true;
				if ((line->size != xform_size) && !have_xform_size)
				{
					xform_size = line->size;  have_xform_size = true;
				}
				if (line->bit_depth == 0)
					have_unknown_output_bit_depth = true;
			}
			for (n = 0; n < block->num_dependencies; n++)
			{
				if ((line = block->dependencies[n]) == NULL)
					continue;
				if (line->need_precise != xform_needs_precise)
				{
					any_change = true; line->need_precise = true;
				}
				if (line->size != xform_size)
				{
					assert(have_xform_size);
					any_change = true;
					if ((line->size.x == 0) && (line->size.y == 0))
						line->size = xform_size;
					else
						found_size_inconsistency = true;
				}
			}
			for (n = 0; n < block->num_components; n++)
			{
				line = block->components + n;
				if (line->need_precise != xform_needs_precise)
				{
					any_change = true; line->need_precise = true;
				}
				if (line->size != xform_size)
				{
					assert(have_xform_size);
					any_change = true;
					if ((line->size.x == 0) && (line->size.y == 0))
						line->size = xform_size;
					else
						found_size_inconsistency = true;
				}
			}
			if (block->propagate_bit_depths(have_unknown_input_bit_depth,
				have_unknown_output_bit_depth))
				any_change = true;
		}
	}

	if (found_size_inconsistency)
	{
		KDU_ERROR(e, 0x25080500); e <<
			KDU_TXT("Cannot implement multi-component transform.  It seems that "
				"image components which must be processed by a common transform "
				"block (or decorrelating colour transform) have incompatible "
				"dimensions.  This error may also be detected if the sub-sampling "
				"factors associated with an MCT output image component vary from "
				"tile to tile or if relative component size change from "
				"resolution level to resolution level (due to incompatible Part-2 "
				"downsampling factor style usage).  While these latter conditions "
				"might not be strictly illegal, they are clearly foolish.");
	}

	if (found_bit_depth_inconsistency)
	{
		KDU_ERROR(e, 0x22080500); e <<
			KDU_TXT("Part-2 codestream declares a codestream component "
				"to have a different bit-depth (Sprecision) to the output "
				"component (Mprecision) with which it is directly associated.  "
				"While this is allowed, it makes very little sense, and Kakadu "
				"will not perform the required multiple scaling for irreversibly "
				"transformed components.");
	}

	return any_change;
}

/*****************************************************************************/
/*                    kd_multi_transform::create_resources                   */
/*****************************************************************************/

void
kd_multi_transform::create_resources()
{
	int n;
	kd_multi_block *block;

	bool reversibility_consistent = true;
	for (n = 0; n < codestream_collection->num_components; n++)
	{
		kd_multi_component *comp = codestream_components + n;
		assert(codestream_collection->components[n] == &(comp->line));
		if (comp->line.reversible != !comp->line.need_irreversible)
			reversibility_consistent = false;
		for (int k = 0; k < comp->num_buffer_rows; k++)
			comp->buffer[k].pre_create(&allocator, comp->line.size.x,
				comp->line.reversible,
				!comp->line.need_precise);
	}
	for (block = xform_blocks; block != NULL; block = block->next)
		for (n = 0; n < block->num_components; n++)
		{
			kd_multi_line *line = block->components + n;
			if (line->reversible != !line->need_irreversible)
				reversibility_consistent = false;
			if ((line->bypass == NULL) && (line->line.check_status() == 0))
				line->line.pre_create(&allocator, line->size.x,
					line->reversible, !line->need_precise);
		}
	for (n = 0; n < output_collection->num_components; n++)
	{
		kd_multi_line *line = output_collection->components[n];
		if (line->reversible != !line->need_irreversible)
			reversibility_consistent = false;
		if ((line->bypass == NULL) && (line->block == NULL) &&
			(line->collection_idx < 0) && (line->line.check_status() == 0))
			line->line.pre_create(&allocator, line->size.x,
				line->reversible, !line->need_precise);
	}

	if (!reversibility_consistent)
	{
		KDU_ERROR(e, 0x23080501); e <<
			KDU_TXT("Cannot implement multi-component transform.  It seems "
				"that one or more transform steps require image samples to be "
				"treated as reversible, where other steps require the same image "
				"samples to be treated as irreversible.  This is illegal in Part-1 "
				"of the JPEG2000 standard.  Although Part-2 is not clear on the "
				"matter, Kakadu's implementation insists only that irreversibly "
				"compressed samples not be subjected to reversible multi-component "
				"transform processing during decompression -- this is eminently "
				"reasonable, since exact reversible processing of data which is "
				"not already exactly defined, makes no sense.  The reverse case, "
				"in which reversibly compressed data is processed using an "
				"irreversible multi-component transform, can make sense, "
				"particularly where there are multiple ways to render the same "
				"original reversibly compressed codestream components to MCT "
				"outputs.");
	}

	allocator.finalize();

	for (n = 0; n < codestream_collection->num_components; n++)
	{
		kd_multi_component *comp = codestream_components + n;
		for (int k = 0; k < comp->num_buffer_rows; k++)
			comp->buffer[k].create();
	}
	for (block = xform_blocks; block != NULL; block = block->next)
		for (n = 0; n < block->num_components; n++)
		{
			kd_multi_line *line = block->components + n;
			if ((line->bypass == NULL) && (line->line.check_status() < 0))
			{
				line->line.create();
				if (line->is_constant)
					line->initialize();
			}
		}
	for (n = 0; n < output_collection->num_components; n++)
	{
		kd_multi_line *line = output_collection->components[n];
		if ((line->bypass == NULL) && (line->block == NULL) &&
			(line->collection_idx < 0) && (line->line.check_status() < 0))
		{
			line->line.create();
			assert(line->is_constant);
			line->initialize();
		}
	}
}

/*****************************************************************************/
/*                    kd_multi_transform::get_scratch_ints                   */
/*****************************************************************************/

int *
kd_multi_transform::get_scratch_ints(int num_elts)
{
	if (max_scratch_ints < num_elts)
	{
		int new_max_scratch_ints = max_scratch_ints + num_elts;
		int *new_scratch_ints = new int[new_max_scratch_ints];
		if (scratch_ints != NULL)
			delete[] scratch_ints;
		scratch_ints = new_scratch_ints;
		max_scratch_ints = new_max_scratch_ints;
	}
	return scratch_ints;
}

/*****************************************************************************/
/*                   kd_multi_transform::get_scratch_floats                  */
/*****************************************************************************/

float *
kd_multi_transform::get_scratch_floats(int num_elts)
{
	if (max_scratch_floats < num_elts)
	{
		int new_max_scratch_floats = max_scratch_floats + num_elts;
		float *new_scratch_floats = new float[new_max_scratch_floats];
		if (scratch_floats != NULL)
			delete[] scratch_floats;
		scratch_floats = new_scratch_floats;
		max_scratch_floats = new_max_scratch_floats;
	}
	return scratch_floats;
}


/* ========================================================================= */
/*                             kdu_multi_synthesis                           */
/* ========================================================================= */

/*****************************************************************************/
/*                         kdu_multi_synthesis::create                       */
/*****************************************************************************/

kdu_long
kdu_multi_synthesis::create(kdu_codestream codestream, kdu_tile tile,
	bool force_precise, bool skip_ycc, bool want_fastest,
	int processing_stripe_height, kdu_thread_env *env,
	kdu_thread_queue *env_queue, bool double_buffering)
{
	kd_multi_synthesis *obj = new kd_multi_synthesis;
	state = obj;
	if (processing_stripe_height < 1)
		processing_stripe_height = 1;
	return obj->create(codestream, tile, force_precise, skip_ycc, want_fastest,
		processing_stripe_height, env, env_queue,
		double_buffering);
}


/* ========================================================================= */
/*                             kd_multi_synthesis                            */
/* ========================================================================= */

/*****************************************************************************/
/*                  kd_multi_synthesis::~kd_multi_synthesis                  */
/*****************************************************************************/

kd_multi_synthesis::~kd_multi_synthesis()
{
	if (output_row_counters != NULL)
		delete[] output_row_counters;
}

/*****************************************************************************/
/*                    kd_multi_synthesis::terminate_queues                   */
/*****************************************************************************/

void
kd_multi_synthesis::terminate_queues(kdu_thread_env *env)
{
	if (env != NULL)
	{
		assert((env_queue != NULL) &&
			(codestream_components[0].env_queue != NULL));
		env->terminate(env_queue, true);
	}
}

/*****************************************************************************/
/*                         kd_multi_synthesis::create                        */
/*****************************************************************************/

kdu_long
kd_multi_synthesis::create(kdu_codestream codestream, kdu_tile tile,
	bool force_precise, bool skip_ycc, bool want_fastest,
	int processing_stripe_height, kdu_thread_env *env,
	kdu_thread_queue *env_queue, bool double_buffering)
{
	construct(codestream, tile, force_precise, skip_ycc, want_fastest,
		processing_stripe_height, env, env_queue, double_buffering);

	// Create the engines and line buffers
	int n;
	for (n = 0; n < codestream_collection->num_components; n++)
	{
		kd_multi_component *comp = codestream_components + n;
		kdu_thread_queue *queue = (env == NULL) ? NULL : comp->env_queue;
		kdu_tile_comp tc = tile.access_component(comp->comp_idx);
		kdu_resolution res = tc.access_resolution();
		if (res.which() == 0)
			comp->pull_ifc = kdu_decoder(res.access_subband(LL_BAND), &allocator,
				!(comp->line.need_precise), 1.0F,
				0, env, queue);
		else
			comp->pull_ifc = kdu_synthesis(res, &allocator,
				!(comp->line.need_precise), 1.0F,
				env, queue);
	}

	create_resources();

	output_row_counters = new int[output_collection->num_components];
	for (n = 0; n < output_collection->num_components; n++)
		output_row_counters[n] = 0;

	kdu_long result = allocator.get_size();
	for (n = 0; n < codestream_collection->num_components; n++)
	{

		kd_multi_component *comp = codestream_components + n;
		comp->pull_ifc.start(env);
		if (comp->double_buffering)
		{
			comp->first_row_in_progress = 0;
			comp->num_rows_in_progress = comp->max_available_rows;
			int remaining_lines = comp->line.size.y - comp->num_rows_in_progress;
			assert(remaining_lines >= 0);
			if (comp->num_rows_in_progress > 0)
				env->add_jobs(comp->env_queue, 1, (remaining_lines == 0));
		}
	}
	return result;
}

/*****************************************************************************/
/*                        kd_multi_synthesis::get_size                       */
/*****************************************************************************/

kdu_coords
kd_multi_synthesis::get_size(int comp_idx)
{
	assert((comp_idx >= 0) && (comp_idx < output_collection->num_components));
	return output_collection->components[comp_idx]->size;
}

/*****************************************************************************/
/*                        kd_multi_synthesis::get_line                       */
/*****************************************************************************/

kdu_line_buf *
kd_multi_synthesis::get_line(int comp_idx, kdu_thread_env *env)
{
	assert((comp_idx >= 0) && (comp_idx < output_collection->num_components));
	kd_multi_line *line =
		output_collection->components[comp_idx];
	kdu_line_buf *result =
		get_line(line, output_row_counters[comp_idx], env);
	if (result != NULL)
		output_row_counters[comp_idx]++;

	return result;
}

/*****************************************************************************/
/*                        kd_multi_synthesis::get_line                       */
/*****************************************************************************/

kdu_line_buf *
kd_multi_synthesis::get_line(kd_multi_line *line, int tgt_row_idx,
	kdu_thread_env *env)
{
	assert(line->bypass == NULL);
	if (line->is_constant)
		return &(line->line);
	if (line->row_idx == tgt_row_idx)
	{ // The line we want has already been generated
		assert(line->outstanding_consumers > 0);
		line->outstanding_consumers--;
		if (line->block != NULL)
			line->block->outstanding_consumers--;
		return &(line->line);
	}

	// If we get here, we need to generate the requested row
	assert(line->row_idx == (tgt_row_idx - 1));
	if (line->outstanding_consumers > 0)
		return NULL; // Have to wait until previous line is completely consumed

	int n;
	kd_multi_block *block = line->block;
	if ((block != NULL) && block->is_null_transform)
	{
		n = (int)(line - block->components);
		assert((n >= 0) && (n < block->num_dependencies));
		kd_multi_line *dep = block->dependencies[n];  assert(dep != NULL);
		if (get_line(dep, tgt_row_idx, env) == NULL)
			return NULL;
		line->row_idx = tgt_row_idx;
		line->outstanding_consumers = line->num_consumers;
		line->copy(dep, line->rev_offset, line->irrev_offset);
	}
	else if (block != NULL)
	{ // The line is generated by a transform block; see if it is ready to
	  // advance.
		if (block->outstanding_consumers > 0)
			return NULL;
		kd_multi_line *scan;
		while (block->num_available_dependencies < block->num_dependencies)
		{
			scan = block->dependencies[block->num_available_dependencies];
			if ((scan == NULL) || scan->is_constant)
			{
				block->num_available_dependencies++; continue;
			}
			if (get_line(scan, tgt_row_idx, env) == NULL)
				return NULL;
			scan->outstanding_consumers++; // Don't actually consume the row yet
			if (scan->block != NULL)
				scan->block->outstanding_consumers++;
			block->num_available_dependencies++;
		}

		// See if we can write to all the outputs of the transform
		for (n = 0; n < block->num_components; n++)
		{
			scan = block->components + n;
			if (scan->outstanding_consumers > 0)
				return false;
		}

		// If we get here, we can run the transform
		block->perform_transform();
		for (n = 0; n < block->num_dependencies; n++)
			if ((scan = block->dependencies[n]) != NULL)
			{
				assert(scan->row_idx == tgt_row_idx);
				scan->outstanding_consumers--;
				if (scan->block != NULL)
					scan->block->outstanding_consumers--;
			}
		for (n = 0; n < block->num_components; n++)
		{
			scan = block->components + n;
			assert((scan->outstanding_consumers == 0) &&
				(scan->row_idx == (tgt_row_idx - 1)));
			scan->row_idx = tgt_row_idx;
			scan->outstanding_consumers = scan->num_consumers;
			block->outstanding_consumers += scan->outstanding_consumers;
		}
		block->num_available_dependencies = 0;
	}
	else
	{
		n = line->collection_idx;  assert(n >= 0);
		assert(codestream_collection->components[n] == line);
		bool perform_ycc = use_ycc && (n < 3);
		if (perform_ycc)
		{ // We will have to process all 3 lines & perform inverse RCT/ICT
			for (n = 0; n < 3; n++)
			{
				kd_multi_line *ycc_line = codestream_collection->components[n];
				assert(ycc_line->row_idx == (tgt_row_idx - 1));
				if (ycc_line->outstanding_consumers > 0)
					return NULL; // Cannot run inverse RCT/ICT yet
			}
			n = 0; // Start from first one
		}
		do {
			kd_multi_component *comp = codestream_components + n;
			if (comp->num_available_rows == 0)
			{
				if (comp->double_buffering)
				{ // Need to wait for stripe rows in progress
					assert((env != NULL) &&
						(comp->num_rows_in_progress > 0));
					env->process_jobs(comp->env_queue);
					comp->next_available_row = comp->first_row_in_progress;
					comp->num_available_rows = comp->num_rows_in_progress;
					if (comp->next_available_row > 0)
						comp->first_row_in_progress = 0;
					else
						comp->first_row_in_progress = comp->num_available_rows;
					comp->num_rows_in_progress =
						comp->line.size.y - tgt_row_idx - comp->num_available_rows;
					bool final_stripe = true;
					if (comp->num_rows_in_progress > comp->max_available_rows)
					{
						comp->num_rows_in_progress = comp->max_available_rows;
						final_stripe = false;
					}
					if (comp->num_rows_in_progress > 0)
						env->add_jobs(comp->env_queue, 1, final_stripe);
				}
				else
				{ // Need to generate more available stripe rows
					assert(((comp->env_queue == NULL) && (env == NULL)) ||
						((comp->env_queue != NULL) && (env != NULL)));
					comp->next_available_row = comp->first_row_in_progress = 0;
					comp->num_rows_in_progress = comp->line.size.y - tgt_row_idx;
					if (comp->num_rows_in_progress > comp->max_available_rows)
						comp->num_rows_in_progress = comp->max_available_rows;
					comp->do_job(env, 0);
					comp->num_available_rows = comp->num_rows_in_progress;
					comp->num_rows_in_progress = 0;
				}
			}
			comp->line.line = comp->buffer[comp->next_available_row++];
			comp->num_available_rows--;
		} while (perform_ycc && ((++n) < 3));
		if (perform_ycc)
		{
			kd_multi_line **ycc_lines = codestream_collection->components;
			kdu_convert_ycc_to_rgb(ycc_lines[0]->line,
				ycc_lines[1]->line,
				ycc_lines[2]->line);
			for (n = 0; n < 3; n++)
			{
				kd_multi_line *scan = ycc_lines[n];
				scan->apply_offset(scan->rev_offset, scan->irrev_offset);
				scan->row_idx++;
				scan->outstanding_consumers = scan->num_consumers;
			}
		}
		else
		{
			line->apply_offset(line->rev_offset, line->irrev_offset);
			line->row_idx++;
			line->outstanding_consumers = line->num_consumers;
		}
	}

	// If we get here, we should have been successful
	assert((line->row_idx == tgt_row_idx) && (line->outstanding_consumers > 0));
	line->outstanding_consumers--;
	if (line->block != NULL)
		line->block->outstanding_consumers--;
	return &(line->line);
}

/*****************************************************************************/
/*                   kd_multi_synthesis::is_line_precise                     */
/*****************************************************************************/

bool
kd_multi_synthesis::is_line_precise(int comp_idx)
{
	if ((output_collection == NULL) ||
		(comp_idx < 0) || (comp_idx >= output_collection->num_components))
		return false;
	kd_multi_line *line = output_collection->components[comp_idx];
	return line->need_precise;
}

/*****************************************************************************/
/*                   kd_multi_synthesis::is_line_absolute                    */
/*****************************************************************************/

bool
kd_multi_synthesis::is_line_absolute(int comp_idx)
{
	if ((output_collection == NULL) ||
		(comp_idx < 0) || (comp_idx >= output_collection->num_components))
		return false;
	kd_multi_line *line = output_collection->components[comp_idx];
	return line->reversible;
}


/* ========================================================================= */
/*                             kdu_multi_analysis                            */
/* ========================================================================= */

/*****************************************************************************/
/*                         kdu_multi_analysis::create                        */
/*****************************************************************************/

kdu_long
kdu_multi_analysis::create(kdu_codestream codestream, kdu_tile tile,
	bool force_precise, kdu_roi_image *roi, bool want_fastest,
	int processing_stripe_height, kdu_thread_env *env,
	kdu_thread_queue *env_queue, bool double_buffering)
{
	kd_multi_analysis *obj = new kd_multi_analysis;
	state = obj;
	if (processing_stripe_height < 1)
		processing_stripe_height = 1;
	return obj->create(codestream, tile, force_precise, roi, want_fastest,
		processing_stripe_height, env, env_queue,
		double_buffering);
}


/* ========================================================================= */
/*                              kd_multi_analysis                            */
/* ========================================================================= */

/*****************************************************************************/
/*                   kd_multi_analysis::~kd_multi_analysis                   */
/*****************************************************************************/

kd_multi_analysis::~kd_multi_analysis()
{
	if (source_row_counters != NULL)
		delete[] source_row_counters;
}

/*****************************************************************************/
/*                    kd_multi_analysis::terminate_queues                    */
/*****************************************************************************/

void
kd_multi_analysis::terminate_queues(kdu_thread_env *env)
{
	if (env != NULL)
	{
		assert((env_queue != NULL) &&
			(codestream_components[0].env_queue != NULL));
		env->terminate(env_queue, true);
	}
}

/*****************************************************************************/
/*                          kd_multi_analysis::create                        */
/*****************************************************************************/

kdu_long
kd_multi_analysis::create(kdu_codestream codestream, kdu_tile tile,
	bool force_precise, kdu_roi_image *roi_image,
	bool want_fastest, int processing_stripe_height,
	kdu_thread_env *env, kdu_thread_queue *env_queue,
	bool double_buffering)
{
	construct(codestream, tile, force_precise, false, want_fastest,
		processing_stripe_height, env, env_queue, double_buffering);
	prepare_network_for_inversion();

	// Create the engines and line buffers
	int n;
	for (n = 0; n < codestream_collection->num_components; n++)
	{
		kd_multi_component *comp = codestream_components + n;
		kdu_thread_queue *queue = (env == NULL) ? NULL : comp->env_queue;
		kdu_tile_comp tc = tile.access_component(comp->comp_idx);
		kdu_resolution res = tc.access_resolution();
		kdu_dims tile_dims;  res.get_dims(tile_dims);
		kdu_roi_node *roi_node = NULL;
		if (roi_image != NULL)
			roi_node = roi_image->acquire_node(comp->comp_idx, tile_dims);
		if (res.which() == 0)
			comp->push_ifc = kdu_encoder(res.access_subband(LL_BAND), &allocator,
				!(comp->line.need_precise), 1.0F, roi_node,
				env, queue);
		else
			comp->push_ifc = kdu_analysis(res, &allocator,
				!(comp->line.need_precise), 1.0F, roi_node,
				env, queue);
	}

	create_resources();

	source_row_counters = new int[output_collection->num_components];
	for (n = 0; n < output_collection->num_components; n++)
		source_row_counters[n] = 0;
	for (n = 0; n < codestream_collection->num_components; n++)
	{
		kd_multi_component *comp = codestream_components + n;
		comp->num_available_rows = comp->max_available_rows;
		assert(comp->num_available_rows <= comp->line.size.y);
		if (comp->num_available_rows > 0)
		{
			comp->line.line = comp->buffer[0];
			comp->next_available_row = 1;
			comp->num_available_rows--;
		}
	}

	return allocator.get_size();
}

/*****************************************************************************/
/*                         kd_multi_analysis::get_size                       */
/*****************************************************************************/

kdu_coords
kd_multi_analysis::get_size(int comp_idx)
{
	assert((comp_idx >= 0) && (comp_idx < output_collection->num_components));
	return output_collection->components[comp_idx]->size;
}

/*****************************************************************************/
/*             kd_multi_analysis::prepare_network_for_inversion              */
/*****************************************************************************/

void
kd_multi_analysis::prepare_network_for_inversion()
{
	int n;
	kd_multi_block *block;
	const char *expl, *failure_explanation = NULL;

	for (block = block_tail; block != NULL; block = block->prev)
	{
		if (block->is_null_transform)
		{
			for (n = 0; n < block->num_dependencies; n++)
			{
				assert(n < block->num_components);
				if ((block->components[n].num_consumers == 0) &&
					(block->dependencies[n] != NULL))
				{
					block->dependencies[n]->num_consumers--;
					block->dependencies[n] = NULL;
				}
			}
		}
		else if ((expl = block->prepare_for_inversion()) != NULL)
		{
			failure_explanation = expl;
			for (n = 0; n < block->num_components; n++)
				block->components[n].is_constant = true;
			// This notifies downstream blocks that there is no need to
			// write to this component.
			for (n = 0; n < block->num_dependencies; n++)
				if (block->dependencies[n] != NULL)
				{
					block->dependencies[n]->num_consumers--;
					block->dependencies[n] = NULL;
				}
		}
	}

	// Make a forward sweep through the blocks, removing any dependencies which
	// are now constant
	for (block = xform_blocks; block != NULL; block = block->next)
	{
		for (n = 0; n < block->num_dependencies; n++)
		{
			kd_multi_line *line = block->dependencies[n];
			if ((line == NULL) || !line->is_constant)
				continue;
			block->dependencies[n] = NULL;
			line->num_consumers--;
			if (block->is_null_transform)
				block->components[n].is_constant = true;
		}
	}

	// Now check to see if we will be able to generate all codestream components
	for (n = 0; n < codestream_collection->num_components; n++)
		if (codestream_collection->components[n]->num_consumers <= 0)
		{
			KDU_ERROR(e, 0x26080501); e <<
				KDU_TXT("Cannot perform forward multi-component transform based "
					"on the source image components supplied.  The multi-component "
					"transform is defined from the perspective of decompression "
					"(i.e., synthesis, or inverse transformation).  Not all of the "
					"defined transform blocks may be invertible.  Also, if the "
					"defined transform blocks do not use all codestream components "
					"to produce final output image components during decompression, "
					"it will not be possible to work back from the final image "
					"components to codestream components which can be subjected to "
					"spatial wavelet transformation and coding.  One of these "
					"conditions has been encountered with the configuration you are "
					"targeting during compression.");
			if (failure_explanation != NULL)
				e << KDU_TXT("  The following additional explanation is "
					"available ---- ") << failure_explanation;
		}

	// Finally, make sure that there is only one consumer for each output
	// row (means one consumer for each source row from the compressor's
	// perspective).
	for (n = 0; n < output_collection->num_components; n++)
	{
		kd_multi_line *line = output_collection->components[n];
		for (block = block_tail;
			(block != NULL) && (line->num_consumers > 1);
			block = block->prev)
			for (int k = 0; k < block->num_dependencies; k++)
				if (block->dependencies[k] == output_collection->components[n])
				{
					block->dependencies[k] = NULL;
					output_collection->components[n]->num_consumers--;
					break;
				}
	}
}

/*****************************************************************************/
/*                     kd_multi_analysis::exchange_line                      */
/*****************************************************************************/

kdu_line_buf *
kd_multi_analysis::exchange_line(int comp_idx, kdu_line_buf *written,
	kdu_thread_env *env)
{
	assert((comp_idx >= 0) && (comp_idx < output_collection->num_components));
	int row_idx = source_row_counters[comp_idx];
	kd_multi_line *line = output_collection->components[comp_idx];
	if (row_idx >= line->size.y)
		return NULL;
	assert(line->num_consumers == 1);
	if (written != NULL)
	{
		assert((written == &(line->line)) && !line->waiting_for_inversion);
		line->apply_offset(-line->rev_offset, -line->irrev_offset);
		advance_line(line, row_idx, env);
		source_row_counters[comp_idx] = ++row_idx;
	}
	assert(line->row_idx == (row_idx - 1));
	if (line->waiting_for_inversion)
		return NULL;
	else
		return &(line->line);
}

/*****************************************************************************/
/*                      kd_multi_analysis::advance_line                      */
/*****************************************************************************/

void
kd_multi_analysis::advance_line(kd_multi_line *line, int new_row_idx,
	kdu_thread_env *env)
{
	assert(line->num_consumers == 1);       // Must be true by the time this
	assert(line->row_idx == (new_row_idx - 1)); // function is invoked.
	line->row_idx = new_row_idx;
	line->waiting_for_inversion = false; // Unless proven otherwise
	if (line->is_constant)
		return; // Nothing more to do

	int n;
	kd_multi_block *block = line->block;
	if ((block != NULL) && block->is_null_transform)
	{ // Pass through to the corresponding dependency line, if any
		n = (int)(line - block->components);
		assert((n >= 0) && (n < block->num_dependencies));
		kd_multi_line *dep = block->dependencies[n];
		if ((dep != NULL) && (dep->row_idx >= new_row_idx))
		{ // This dependency has been written already
			assert(dep->num_consumers > 1);
			dep->num_consumers--;
			block->dependencies[n] = dep = NULL;
		}
		if (dep == NULL)
			return; // Nothing to do; this dependency need not be written
		assert((dep->num_consumers > 0) && !dep->is_constant);
		dep->copy(line, -dep->rev_offset, -dep->irrev_offset);
		advance_line(dep, new_row_idx, env);
	}
	else if (block != NULL)
	{ // See if we can invert the transform block
		line->waiting_for_inversion = true;
		block->outstanding_consumers--;
		if (block->outstanding_consumers > 0)
			return; // Can't process the block yet
		kd_multi_line *dep;
		while (block->num_available_dependencies < block->num_dependencies)
		{
			dep = block->dependencies[block->num_available_dependencies];
			if ((dep != NULL) && dep->is_constant)
			{ // Should not happen, but just in case
				block->dependencies[block->num_available_dependencies] = dep = NULL;
			}
			if ((dep != NULL) && (dep->row_idx < new_row_idx) &&
				dep->waiting_for_inversion)
				return; // Can't write inversion results yet
			block->num_available_dependencies++;
		}

		// If we get here, we can run the transform, but first remove any
		// dependencies which have already been written
		for (n = 0; n < block->num_dependencies; n++)
			if (((dep = block->dependencies[n]) != NULL) &&
				(dep->row_idx >= new_row_idx))
			{
				assert(dep->num_consumers > 1);
				dep->num_consumers--;
				block->dependencies[n] = NULL;
			}

		block->perform_inverse();

		for (n = 0; n < block->num_dependencies; n++)
			if ((dep = block->dependencies[n]) != NULL)
				advance_line(dep, new_row_idx, env);
		for (n = 0; n < block->num_components; n++)
		{
			block->components[n].waiting_for_inversion = false;
			if (block->components[n].num_consumers > 0)
			{
				assert(block->components[n].num_consumers == 1);
				block->outstanding_consumers++;
			}
		}
		block->num_available_dependencies = 0;
	}
	else
	{ // Push the line into the relevant codestream component engine
		n = line->collection_idx;  assert(n >= 0);
		assert(codestream_collection->components[n] == line);
		bool process_ycc = use_ycc && (n < 3);
		if (process_ycc)
		{
			line->waiting_for_inversion = true;
			kd_multi_line **ycc_lines = codestream_collection->components;
			for (n = 0; n < 3; n++)
				if (ycc_lines[n]->row_idx < new_row_idx)
					return; // Cannot run RCT/ICT yet
			kdu_convert_rgb_to_ycc(ycc_lines[0]->line,
				ycc_lines[1]->line,
				ycc_lines[2]->line);
			for (n = 0; n < 3; n++)
			{
				assert(ycc_lines[n]->waiting_for_inversion);
				ycc_lines[n]->waiting_for_inversion = false;
			}
			n = 0;
		}
		do {
			kd_multi_component *comp = codestream_components + n;
			if (comp->num_available_rows == 0)
			{ // We have generated all rows
				int num_rows_left = comp->line.size.y - (new_row_idx + 1);
				bool last_stripe = (num_rows_left == 0);
				if (comp->double_buffering)
				{ // Convert available rows into an active job in progress
					assert(env != NULL);
					int first_available_row = 0;
					if (comp->num_rows_in_progress)
					{ // Do outstanding work
						env->process_jobs(comp->env_queue);
						if (comp->first_row_in_progress == 0)
							first_available_row = comp->num_rows_in_progress;
					}
					comp->first_row_in_progress = first_available_row;
					comp->num_rows_in_progress =
						comp->next_available_row - first_available_row;
					if (first_available_row > 0)
						comp->next_available_row = 0;
					env->add_jobs(comp->env_queue, 1, last_stripe);
				}
				else
				{
					assert(((comp->env_queue == NULL) && (env == NULL)) ||
						((comp->env_queue != NULL) && (env != NULL)));
					comp->num_rows_in_progress = comp->next_available_row;
					comp->next_available_row = comp->first_row_in_progress = 0;
					comp->do_job(env, 0);
					comp->num_rows_in_progress = 0;
				}
				comp->num_available_rows =
					(num_rows_left < comp->max_available_rows) ?
					num_rows_left : (comp->max_available_rows);
			}
			if (comp->num_available_rows > 0)
			{
				comp->line.line = comp->buffer[comp->next_available_row++];
				comp->num_available_rows--;
			}
		} while (process_ycc && ((++n) < 3));
	}

	// If we get here, we have used the new line
	assert(!line->waiting_for_inversion);
}

/*****************************************************************************/
/*                    kd_multi_analysis::is_line_precise                     */
/*****************************************************************************/

bool
kd_multi_analysis::is_line_precise(int comp_idx)
{
	if ((output_collection == NULL) ||
		(comp_idx < 0) || (comp_idx >= output_collection->num_components))
		return false;
	kd_multi_line *line = output_collection->components[comp_idx];
	return line->need_precise;
}

/*****************************************************************************/
/*                   kd_multi_analysis::is_line_absolute                     */
/*****************************************************************************/

bool
kd_multi_analysis::is_line_absolute(int comp_idx)
{
	if ((output_collection == NULL) ||
		(comp_idx < 0) || (comp_idx >= output_collection->num_components))
		return false;
	kd_multi_line *line = output_collection->components[comp_idx];
	return line->reversible;
}


/* ========================================================================= */
/*                            kd_multi_null_block                            */
/* ========================================================================= */

/*****************************************************************************/
/*                       kd_multi_null_block::initialize                     */
/*****************************************************************************/

void
kd_multi_null_block::initialize(int stage_idx, int block_idx,
	kdu_tile tile, int num_block_inputs, int num_block_outputs,
	kd_multi_collection *input_collection,
	kd_multi_collection *output_collection,
	kd_multi_transform *owner)
{
	int *scratch =
		owner->get_scratch_ints(num_block_inputs + 2 * num_block_outputs);
	int *input_indices = scratch;
	int *output_indices = input_indices + num_block_inputs;
	int *rev_offsets = output_indices + num_block_outputs;
	float *irrev_offsets = owner->get_scratch_floats(num_block_outputs);
	int n, num_stage_inputs, num_stage_outputs;
	tile.get_mct_block_info(stage_idx, block_idx, num_stage_inputs,
		num_stage_outputs, num_block_inputs, num_block_outputs,
		input_indices, output_indices, irrev_offsets,
		rev_offsets);
	assert((num_stage_inputs == input_collection->num_components) &&
		(num_stage_outputs == output_collection->num_components));

	num_components = num_block_outputs;
	components = new kd_multi_line[num_components];
	num_dependencies = num_block_inputs;
	if (num_dependencies > num_components)
		num_dependencies = num_components; // The rest are just being discarded
	dependencies = new kd_multi_line *[num_dependencies];

	for (n = 0; n < num_dependencies; n++)
		dependencies[n] = input_collection->components[input_indices[n]];
	for (n = 0; n < num_components; n++)
	{
		kd_multi_line *line = components + n;
		line->block = this;
		output_collection->components[output_indices[n]] = line;
		if (n >= num_dependencies)
			line->is_constant = true;
		else
		{
			kd_multi_line *dep = dependencies[n];
			line->need_irreversible = dep->need_irreversible;
			line->reversible = dep->reversible;
			if (dep->is_constant)
			{
				line->is_constant = true;
				line->rev_offset = dep->rev_offset;
				line->irrev_offset = dep->irrev_offset;
				dependencies[n] = dep = NULL;
			}
			else
				dep->num_consumers++;
		}
		line->rev_offset += rev_offsets[n];
		line->irrev_offset += irrev_offsets[n];
	}
}


/* ========================================================================= */
/*                           kd_multi_matrix_block                           */
/* ========================================================================= */

/*****************************************************************************/
/*                      kd_multi_matrix_block::initialize                    */
/*****************************************************************************/

void
kd_multi_matrix_block::initialize(int stage_idx, int block_idx,
	kdu_tile tile, int num_block_inputs, int num_block_outputs,
	kd_multi_collection *input_collection,
	kd_multi_collection *output_collection,
	kd_multi_transform *owner)
{
	int *scratch =
		owner->get_scratch_ints(num_block_inputs + num_block_outputs);
	int *input_indices = scratch;
	int *output_indices = input_indices + num_block_inputs;
	float *irrev_offsets = owner->get_scratch_floats(num_block_outputs);
	int n, num_stage_inputs, num_stage_outputs;
	tile.get_mct_block_info(stage_idx, block_idx, num_stage_inputs,
		num_stage_outputs, num_block_inputs, num_block_outputs,
		input_indices, output_indices, irrev_offsets, NULL);
	assert((num_stage_inputs == input_collection->num_components) &&
		(num_stage_outputs == output_collection->num_components));

	num_components = num_block_outputs;
	components = new kd_multi_line[num_components];
	num_dependencies = num_block_inputs;
	dependencies = new kd_multi_line *[num_dependencies];
	memset(dependencies, 0, sizeof(kd_multi_line *)*(size_t)num_dependencies);
	coefficients = new float[num_block_inputs*num_block_outputs];
	tile.get_mct_matrix_info(stage_idx, block_idx, coefficients);

	for (n = 0; n < num_dependencies; n++)
	{
		dependencies[n] = input_collection->components[input_indices[n]];
		if (dependencies[n] == NULL)
			continue;
		dependencies[n]->num_consumers++;
	}

	for (n = 0; n < num_components; n++)
	{
		kd_multi_line *line = components + n;
		line->block = this;
		output_collection->components[output_indices[n]] = line;
		line->need_irreversible = true;
		line->irrev_offset = irrev_offsets[n];
	}

	// Before returning, see if we can pre-compute the impact of any constant
	// input lines.
	for (n = 0; n < num_dependencies; n++)
		if (dependencies[n]->is_constant)
		{
			int m;
			float off = dependencies[n]->irrev_offset;
			for (m = 0; m < num_components; m++)
			{
				kd_multi_line *line = components + m;
				line->irrev_offset += off * coefficients[m*num_dependencies + n];
			}
			dependencies[n]->num_consumers--;
			dependencies[n] = NULL;
		}
}

/*****************************************************************************/
/*               kd_multi_matrix_block::normalize_coefficients               */
/*****************************************************************************/

void
kd_multi_matrix_block::normalize_coefficients()
{
	int n, m;
	bool need_precise = false;

	for (n = 0; n < num_dependencies; n++)
	{
		kd_multi_line *line = dependencies[n];
		if (line == NULL)
			continue;
		assert(!line->is_constant);
		if (line->bit_depth == 0)
		{ // Unknown dynamic range; can't chance fixed-point overflow
			need_precise = true;  continue;
		}
		if (line->need_precise)
			need_precise = true;
		float factor = (float)(1 << line->bit_depth);
		for (m = 0; m < num_components; m++)
			coefficients[m*num_dependencies + n] *= factor;
	}
	for (m = 0; m < num_components; m++)
	{
		kd_multi_line *line = components + m;
		if (line->bit_depth == 0)
		{ // Unknown dynamic range; can't chance fixed-point overflow
			need_precise = true;  continue;
		}
		if (line->need_precise)
			need_precise = line->need_precise;
		float factor = 1.0F / (float)(1 << line->bit_depth);
		for (n = 0; n < num_dependencies; n++)
			coefficients[m*num_dependencies + n] *= factor;
	}
	if (need_precise)
	{
		for (n = 0; n < num_dependencies; n++)
			if (dependencies[n] != NULL)
				dependencies[n]->need_precise = true;
		for (m = 0; m < num_components; m++)
			components[m].need_precise = true;
	}
}

/*****************************************************************************/
/*                  kd_multi_matrix_block::perform_transform                 */
/*****************************************************************************/

void
kd_multi_matrix_block::perform_transform()
{
	int m, n;
	for (m = 0; m < num_components; m++)
	{
		kd_multi_line *line = components + m;
		int w, width = line->line.get_width();
		if (line->line.get_buf32() != NULL)
		{ // Floating point implementation
			float factor = line->irrev_offset;
			kdu_sample32 *dst = line->line.get_buf32();
			for (w = 0; w < width; w++)
				dst[w].fval = factor;
			for (n = 0; n < num_dependencies; n++)
			{
				factor = coefficients[m*num_dependencies + n];
				if ((dependencies[n] == NULL) || (factor == 0.0F))
					continue;
				kdu_sample32 *src = dependencies[n]->line.get_buf32();
				if (!dependencies[n]->reversible)
					for (w = 0; w < width; w++)
						dst[w].fval += factor * src[w].fval;
				else
				{ // Need to convert source samples from ints to floats on
				  // the fly.
					int src_bit_depth = dependencies[n]->bit_depth;
					if (src_bit_depth > 0)
						factor *= 1.0F / (1 << src_bit_depth);
					for (w = 0; w < width; w++)
						dst[w].fval += factor * (float)src[w].ival;
				}
			}
		}
		else
		{
			if (short_coefficients == NULL)
				create_short_coefficients(width);
			kdu_sample16 *dst = line->line.get_buf16();
			kdu_int32 offset, *acc = short_accumulator;
			memset(acc, 0, (size_t)(width << 2));
			for (n = 0; n < num_dependencies; n++)
			{
				kdu_int16 factor = short_coefficients[m*num_dependencies + n];
				if ((dependencies[n] == NULL) || (factor == 0))
					continue;
				kdu_sample16 *src = dependencies[n]->line.get_buf16();
				if (!dependencies[n]->reversible)
					for (w = 0; w < width; w++)
						acc[w] += ((kdu_int32)src[w].ival) * factor;
				else
				{ // Need to convert source samples from reversible short ints
				  // to fixed-point short ints on the fly.
					int src_bit_depth = dependencies[n]->bit_depth;
					assert(src_bit_depth > 0); // Else should be using 32-bits
					int upshift = KDU_FIX_POINT - src_bit_depth;
					if (upshift < 0)
					{ // This case is actually extremely unlikely, since high
					  // bit-depth reversible data will generally be
					  // represented using 32-bit integers, not shorts.  For
					  // this reason, we don't worry about accuracy; just
					  // scale down the value of `factor'
						factor = (factor + (1 << (-upshift - 1))) >> (-upshift);
						upshift = 0;
					}
					for (w = 0; w < width; w++)
						acc[w] += (((kdu_int32)src[w].ival) << upshift) * factor;
				}
			}
			kdu_int32 downshift = this->short_downshift;
			offset = (kdu_int32)
				floor(0.5 + line->irrev_offset * (1 << KDU_FIX_POINT));
			offset <<= downshift;
			offset += (1 << downshift) >> 1;
			for (w = 0; w < width; w++)
				dst[w].ival = (kdu_int16)((acc[w] + offset) >> downshift);
		}
	}
}

/*****************************************************************************/
/*                kd_multi_matrix_block::prepare_for_inversion               */
/*****************************************************************************/

const char *
kd_multi_matrix_block::prepare_for_inversion()
{
	if (inverse_coefficients != NULL)
		return NULL;

	int n, m, k;
	int available_inputs = num_dependencies;
	int available_outputs = 0;
	for (n = 0; n < num_components; n++)
		if (components[n].num_consumers > 0)
			available_outputs++;
	this->outstanding_consumers = available_outputs;
	if (available_outputs < available_inputs)
		return "Encountered underdetermined system while trying to invert a "
		"multi-component transform block so as to convert MCT output "
		"components into codestream components during compression.";
	for (n = 0; n < num_dependencies; n++)
		if ((dependencies[n] != NULL) && dependencies[n]->reversible)
			return "Encountered an irreversible decorrelation transform block "
			"which operates on reversible codestream sample data.  "
			"While we allow such transforms to be processed during "
			"decompression, it is unreasonable to generate reversibly "
			"compressed component samples using an irreversible inverse "
			"multi-component transform during compression.  Kakadu will "
			"not invert this transform during compression.  This can "
			"prevent the compression process from proceeding if there are "
			"no other paths back from the MCT output components to the "
			"codestream components.";

	inverse_coefficients = new float[num_dependencies*num_components];

	assert(work == NULL);
	work = new double[3 * available_inputs*available_outputs +
		2 * available_inputs*available_inputs];
	double *A = work;
	double *pinv_A = A + available_inputs * available_outputs;
	double *G = pinv_A + available_inputs * available_outputs;
	double *AtA = G + available_inputs * available_outputs;
	double *inv_G = AtA + available_inputs * available_inputs;

	// Find matrix A, being the forward transformation from the set of
	// available inputs to the set of available outputs.  Note that A may
	// have more rows than columns.
	double *dp = A;
	for (m = 0; m < num_components; m++)
	{
		if (components[m].num_consumers < 1)
			continue;
		for (n = 0; n < num_dependencies; n++)
			*(dp++) = coefficients[m*num_dependencies + n];
	}

	// Next, evaluate A^t * A.  We will eventually find the pseudo-inverse
	// of the system y = Ax by setting A^t*y = (A^t*A)*x so that
	// pinv_A = inv(A^t*A) * A^t.
	double max_energy = 0.0;
	dp = AtA;
	for (m = 0; m < available_inputs; m++)
		for (n = 0; n < available_inputs; n++)
		{
			double sum = 0.0;
			for (k = 0; k < available_outputs; k++)
				sum += A[m + k * available_inputs] * A[n + k * available_inputs];
			*(dp++) = sum;
			if ((m == n) && (sum > max_energy))
				max_energy = sum;
		}

	// Now invert the AtA matrix.  This matrix is guaranteed to be at least
	// positive semi-definite.  Thus, we may use Cholesky factorization
	// to perform the inversion, while at the same time providing a robust
	// means to test for ill-conditioning.  The Cholesky factorization
	// represents AtA as G*G^t where G is a lower triangular matrix.  Thus,
	//   | g00  0   0   0   0   0  ...|     |g00 g10 g20 ...|
	//   | g10 g11  0   0   0   0  ...|  *  | 0  g11 g21 ...| = AtA
	//   | g20 g21 g22  0   0   0  ...|     | 0   0  g22 ...|
	//   | ... ... ... ... ... ... ...|     |... ... ... ...|
	for (m = 0; m < available_inputs; m++)
	{
		double sum = AtA[m + m * available_inputs];
		for (n = 0; n < m; n++)
			sum -= G[m*available_inputs + n] * G[m*available_inputs + n];
		if (sum < (max_energy*0.0000000000001))
		{
			delete[] work;
			work = NULL;
			return "Near singular irreversible decorrelation transform block "
				"encountered in multi-component transform description.  This "
				"can prevent the compression process from proceeding if "
				"there are no other paths back from the MCT output "
				"components to the codestream components.";
		}
		G[m*available_inputs + m] = sqrt(sum);
		double factor = 1.0 / G[m*available_inputs + m];
		for (n = 0; n < m; n++)
			G[n*available_inputs + m] = 0.0; // Rest of column above diagonal
		for (n = m + 1; n < available_inputs; n++)
		{
			sum = AtA[n*available_inputs + m]; // Column m from AtA
			for (k = 0; k < m; k++)
				sum -= G[n*available_inputs + k] * G[m*available_inputs + k];
			G[n*available_inputs + m] = sum * factor;
		}
	}

	// Now invert G
	for (n = 0; n < available_inputs; n++)
	{ // Scan through the columns of inv_G; it is also lower triangular
		for (m = 0; m < n; m++)
			inv_G[m*available_inputs + n] = 0.0;
		inv_G[n*available_inputs + n] = 1.0 / G[n*available_inputs + n];
		for (m = n + 1; m < available_inputs; m++)
		{
			double sum = 0.0;
			for (k = 0; k < m; k++)
				sum += inv_G[k*available_inputs + n] * G[m*available_inputs + k];
			inv_G[m*available_inputs + n] = -sum / G[m*available_inputs + m];
		}
	}

	// Now find pinv_A = inv(G)^t * inv(G) * A^t -- equivalently,
	// pinv_A^t = A * inv(G)^t * inv(G)

	// Start by overwriting G with A * inv(G)^t
	for (m = 0; m < available_outputs; m++)
		for (n = 0; n < available_inputs; n++)
		{
			double sum = 0.0;
			for (k = 0; k < available_inputs; k++)
				sum += A[m*available_inputs + k] * inv_G[n*available_inputs + k];
			G[m*available_inputs + n] = sum;
		}

	// Now multiply by inv_G and write the transposed result to pinv_A
	for (m = 0; m < available_outputs; m++)
		for (n = 0; n < available_inputs; n++)
		{
			double sum = 0.0;
			for (k = 0; k < available_inputs; k++)
				sum += G[m*available_inputs + k] * inv_G[k*available_inputs + n];
			pinv_A[n*available_outputs + m] = sum;
		}

	// Finally, write the `inverse_coefficients' array
	double *sp = pinv_A;
	for (m = 0; m < num_dependencies; m++)
		for (n = 0; n < num_components; n++)
			inverse_coefficients[m*num_components + n] =
			(components[n].num_consumers < 1) ? 0.0F : ((float) *(sp++));

	delete[] work;
	work = NULL;
	return NULL;
}

/*****************************************************************************/
/*                   kd_multi_matrix_block::perform_inverse                  */
/*****************************************************************************/

void
kd_multi_matrix_block::perform_inverse()
{
	assert(inverse_coefficients != NULL);

	int m, n;
	for (m = 0; m < num_dependencies; m++)
	{
		kd_multi_line *line = dependencies[m];
		if (line == NULL)
			continue;

		int w, width = line->line.get_width();
		if (line->line.get_buf32() != NULL)
		{ // Floating point implementation
			float factor = -line->irrev_offset;
			kdu_sample32 *dst = line->line.get_buf32();
			for (w = 0; w < width; w++)
				dst[w].fval = factor;
			for (n = 0; n < num_components; n++)
			{
				if (components[n].num_consumers < 1)
					continue;
				kdu_sample32 *src = components[n].line.get_buf32();
				factor = inverse_coefficients[m*num_components + n];
				for (w = 0; w < width; w++)
					dst[w].fval += factor * src[w].fval;
			}
		}
		else
		{
			if (short_coefficients == NULL)
				create_short_inverse_coefficients(width);
			kdu_sample16 *dst = line->line.get_buf16();
			kdu_int32 *acc = short_accumulator;
			memset(acc, 0, (size_t)(width << 2));
			for (n = 0; n < num_components; n++)
			{
				if (components[n].num_consumers < 1)
					continue;
				kdu_sample16 *src = components[n].line.get_buf16();
				kdu_int16 factor = short_coefficients[m*num_components + n];
				for (w = 0; w < width; w++)
					acc[w] += ((kdu_int32)src[w].ival) * factor;
			}
			kdu_int32 downshift = this->short_downshift;
			kdu_int32 offset = (kdu_int32)
				floor(0.5 + line->irrev_offset * (1 << KDU_FIX_POINT));
			offset = ((-offset) << downshift) + ((1 << downshift) >> 1);
			for (w = 0; w < width; w++)
				dst[w].ival = (kdu_int16)((acc[w] + offset) >> downshift);
		}
	}
}

/*****************************************************************************/
/*              kd_multi_matrix_block::create_short_coefficients             */
/*****************************************************************************/

void
kd_multi_matrix_block::create_short_coefficients(int width)
{
	if (short_coefficients != NULL)
		return;

	int n, m;
	float val, max_val = 0.00001F;
	for (m = 0; m < num_components; m++)
		for (n = 0; n < num_dependencies; n++)
		{
			if (dependencies[n] == NULL)
				continue;
			val = coefficients[m*num_dependencies + n];
			if (val > max_val)
				max_val = val;
			else if (val < -max_val)
				max_val = -val;
		}

	short_coefficients = new kdu_int16[num_components*num_dependencies];
	short_accumulator = new kdu_int32[width];

	float factor = 1.0F;
	for (short_downshift = 0;
		((factor*max_val) <= 16383.0F) && (short_downshift < 16);
		factor *= 2.0F)
		short_downshift++;
	for (m = 0; m < num_components; m++)
		for (n = 0; n < num_dependencies; n++)
		{
			if (dependencies[n] == NULL)
			{
				short_coefficients[m*num_dependencies + n] = 0;
				continue;
			}
			val = coefficients[m*num_dependencies + n] * factor;
			int ival = (int)floor(val + 0.5);
			if (ival >= (1 << 15))
				ival = (1 << 15) - 1;
			else if (ival < -(1 << 15))
				ival = -(1 << 15);
			short_coefficients[m*num_dependencies + n] = (kdu_int16)ival;
		}
}

/*****************************************************************************/
/*          kd_multi_matrix_block::create_short_inverse_coefficients         */
/*****************************************************************************/

void
kd_multi_matrix_block::create_short_inverse_coefficients(int width)
{
	if (short_coefficients != NULL)
		return;

	int m, n;
	float val, max_val = 0.00001F;
	for (m = 0; m < num_dependencies; m++)
	{
		if (dependencies[m] == NULL)
			continue;
		for (n = 0; n < num_components; n++)
		{
			val = inverse_coefficients[m*num_components + n];
			if (val > max_val)
				max_val = val;
			else if (val < -max_val)
				max_val = -val;
		}
	}

	short_coefficients = new kdu_int16[num_dependencies*num_components];
	short_accumulator = new kdu_int32[width];

	float factor = 1.0F;
	for (short_downshift = 0;
		((factor*max_val) <= 16383.0F) && (short_downshift < 16);
		factor *= 2.0F)
		short_downshift++;

	for (m = 0; m < num_dependencies; m++)
		for (n = 0; n < num_components; n++)
		{
			if (dependencies[m] == NULL)
				val = 0.0F;
			else
				val = inverse_coefficients[m*num_components + n] * factor;
			int ival = (int)floor(val + 0.5);
			if (ival >= (1 << 15))
				ival = (1 << 15) - 1;
			else if (ival < -(1 << 15))
				ival = -(1 << 15);
			short_coefficients[m*num_components + n] = (kdu_int16)ival;
		}
}


/* ========================================================================= */
/*                           kd_multi_rxform_block                           */
/* ========================================================================= */

/*****************************************************************************/
/*                      kd_multi_rxform_block::initialize                    */
/*****************************************************************************/

void
kd_multi_rxform_block::initialize(int stage_idx, int block_idx,
	kdu_tile tile, int num_block_inputs, int num_block_outputs,
	kd_multi_collection *input_collection,
	kd_multi_collection *output_collection,
	kd_multi_transform *owner)
{
	int N = num_block_inputs;
	assert(N >= num_block_outputs); // Some outputs might not be used
	int *scratch = owner->get_scratch_ints(4 * N); // Safe upper bound
	int *input_indices = scratch;
	int *output_indices = input_indices + N;
	int *rev_offsets = output_indices + N;
	int *active_outputs = rev_offsets + N;

	int n, num_stage_inputs, num_stage_outputs;
	tile.get_mct_block_info(stage_idx, block_idx, num_stage_inputs,
		num_stage_outputs, num_block_inputs, num_block_outputs,
		input_indices, output_indices, NULL, rev_offsets);
	assert((num_stage_inputs == input_collection->num_components) &&
		(num_stage_outputs == output_collection->num_components));

	num_components = num_dependencies = N;
	components = new kd_multi_line[N];
	dependencies = new kd_multi_line *[N];
	memset(dependencies, 0, sizeof(kd_multi_line *)*(size_t)N);
	int num_coeffs = N * (N + 1);
	coefficients = new kdu_int32[num_coeffs];
	tile.get_mct_rxform_info(stage_idx, block_idx, coefficients, active_outputs);

	// See if the coefficients can be represented as short integers
	bool need_precise = false;
	for (n = 0; n < num_coeffs; n++)
		if ((coefficients[n] > 0x7FFF) || (coefficients[n] < -0x7FFF))
			need_precise = true;

	for (n = 0; n < N; n++)
	{
		dependencies[n] = input_collection->components[input_indices[n]];
		if (dependencies[n] == NULL)
			continue;
		dependencies[n]->num_consumers++;
		dependencies[n]->reversible = true;
		if (need_precise)
			dependencies[n]->need_precise = true;
	}

	for (n = 0; n < N; n++)
	{
		kd_multi_line *line = components + n;
		line->block = this;
		line->reversible = true;
		line->need_precise = need_precise;
	}

	for (n = 0; n < num_block_outputs; n++)
	{
		kd_multi_line *line = components + active_outputs[n];
		output_collection->components[output_indices[n]] = line;
		line->rev_offset = rev_offsets[n];
	}
}

/*****************************************************************************/
/*                kd_multi_rxform_block::prepare_for_inversion               */
/*****************************************************************************/

const char *
kd_multi_rxform_block::prepare_for_inversion()
{
	int n;

	// Need only check that all components will be available.
	for (n = 0; n < num_components; n++)
		if (components[n].num_consumers < 1)
			return "Reversible decorrelation transform block cannot be "
			"inverted unless all of its outputs can be computed by "
			"downstream transform blocks, or by the application supplying "
			"them.";
	outstanding_consumers = num_components;
	return NULL;
}

/*****************************************************************************/
/*                  kd_multi_rxform_block::perform_transform                 */
/*****************************************************************************/

void
kd_multi_rxform_block::perform_transform()
{
	int N = num_components;
	int m, n;

	// Start by initializing the `components' array with values from the
	// input `dependencies'.
	assert(num_dependencies == N);
	for (m = 0; m < N; m++)
	{
		kd_multi_line *line = components + m;
		if (dependencies[m] == NULL)
			line->reset(0, 0.0F);
		else
			line->copy(dependencies[m], 0, 0.0F);
	}

	// Execute the reversible transform steps
	for (n = 0; n <= N; n++)
	{ // Walk through the columns of the coefficient matrix
		int dst_idx = (N - 1) - (n % N);
		kd_multi_line *line = components + dst_idx;
		int w, width = line->line.get_width();
		if (accumulator == NULL)
			accumulator = new kdu_int32[width];
		kdu_int32 divisor = coefficients[dst_idx*(N + 1) + n];
		kdu_int32 abs_divisor = divisor;
		if ((n == N) && (divisor < 0))
			abs_divisor = -divisor;
		kdu_int32 downshift = 0;
		while ((1 << downshift) < abs_divisor)
			downshift++;
		if ((1 << downshift) != abs_divisor)
		{
			KDU_ERROR(e, 0x23090501); e <<
				KDU_TXT("Multi-component reversible decorrelation transforms "
					"must have exact positive powers of 2 for the divisors which "
					"are used to scale and round the update terms.  The offending "
					"divisor is ") << divisor << ".";
		}
		kdu_int32 offset = abs_divisor >> 1;
		for (w = 0; w < width; w++)
			accumulator[w] = offset;
		if (line->line.get_buf32() != NULL)
		{ // Processing with 32-bit integers
			for (m = 0; m < N; m++)
			{
				if (m == dst_idx)
					continue;
				kdu_int32 factor = coefficients[m*(N + 1) + n];
				if (factor == 0)
					continue;
				kdu_sample32 *src = components[m].line.get_buf32();
				for (w = 0; w < width; w++)
					accumulator[w] += src[w].ival * factor;
			}
			kdu_sample32 *dst = line->line.get_buf32();
			if (divisor < 0)
			{ // In this case, we require a sign change
				assert(n == N);
				for (w = 0; w < width; w++)
					dst[w].ival = -(dst[w].ival - (accumulator[w] >> downshift));
			}
			else
			{ // Usual case, with a positive divisor
				for (w = 0; w < width; w++)
					dst[w].ival -= (accumulator[w] >> downshift);
			}
		}
		else
		{ // Processing with 16-bit integers
			for (m = 0; m < N; m++)
			{
				if (m == dst_idx)
					continue;
				kdu_int32 factor = coefficients[m*(N + 1) + n];
				if (factor == 0)
					continue;
				kdu_sample16 *src = components[m].line.get_buf16();
				for (w = 0; w < width; w++)
					accumulator[w] += ((kdu_int32)src[w].ival) * factor;
			}
			kdu_sample16 *dst = line->line.get_buf16();
			if (divisor < 0)
			{ // In this case, we require a sign change
				assert(n == N);
				for (w = 0; w < width; w++)
					dst[w].ival =
					-(dst[w].ival - (kdu_int16)(accumulator[w] >> downshift));
			}
			else
			{ // Usual case, with a positive divisor
				for (w = 0; w < width; w++)
					dst[w].ival -= (kdu_int16)(accumulator[w] >> downshift);
			}
		}
	}

	// Finally apply offsets, as required
	for (m = 0; m < N; m++)
	{
		kd_multi_line *line = components + m;
		line->apply_offset(line->rev_offset, 0.0F);
	}
}

/*****************************************************************************/
/*                   kd_multi_rxform_block::perform_inverse                  */
/*****************************************************************************/

void
kd_multi_rxform_block::perform_inverse()
{
	int N = num_components;
	int m, n;

	// Execute the transform steps in reverse order
	for (n = N; n >= 0; n--)
	{ // Walk through the columns of the coefficient matrix
		int dst_idx = (N - 1) - (n % N);
		kd_multi_line *line = components + dst_idx;
		int w, width = line->line.get_width();
		if (accumulator == NULL)
			accumulator = new kdu_int32[width];
		kdu_int32 divisor = coefficients[dst_idx*(N + 1) + n];
		kdu_int32 abs_divisor = divisor;
		if ((n == N) && (divisor < 0))
			abs_divisor = -divisor;
		kdu_int32 downshift = 0;
		while ((1 << downshift) < abs_divisor)
			downshift++;
		if ((1 << downshift) != abs_divisor)
		{
			KDU_ERROR(e, 0x23090502); e <<
				KDU_TXT("Multi-component reversible decorrelation transforms "
					"must have exact positive powers of 2 for the divisors which "
					"are used to scale and round the update terms.  The offending "
					"divisor is ") << divisor << ".";
		}
		kdu_int32 offset = abs_divisor >> 1;
		for (w = 0; w < width; w++)
			accumulator[w] = offset;
		if (line->line.get_buf32() != NULL)
		{ // Processing with 32-bit integers
			for (m = 0; m < N; m++)
			{
				if (m == dst_idx)
					continue;
				kdu_int32 factor = coefficients[m*(N + 1) + n];
				if (factor == 0)
					continue;
				kdu_sample32 *src = components[m].line.get_buf32();
				for (w = 0; w < width; w++)
					accumulator[w] += src[w].ival * factor;
			}
			kdu_sample32 *dst = line->line.get_buf32();
			if (divisor < 0)
			{ // In this case, we require a sign change
				assert(n == N);
				for (w = 0; w < width; w++)
					dst[w].ival = (-dst[w].ival) + (accumulator[w] >> downshift);
			}
			else
			{ // Usual case, with a positive divisor
				for (w = 0; w < width; w++)
					dst[w].ival += (accumulator[w] >> downshift);
			}
		}
		else
		{ // Processing with 16-bit integers
			for (m = 0; m < N; m++)
			{
				if (m == dst_idx)
					continue;
				kdu_int32 factor = coefficients[m*(N + 1) + n];
				if (factor == 0)
					continue;
				kdu_sample16 *src = components[m].line.get_buf16();
				for (w = 0; w < width; w++)
					accumulator[w] += ((kdu_int32)src[w].ival) * factor;
			}
			kdu_sample16 *dst = line->line.get_buf16();
			if (divisor < 0)
			{ // In this case, we require a sign change
				assert(n == N);
				for (w = 0; w < width; w++)
					dst[w].ival = (-dst[w].ival) +
					(kdu_int16)(accumulator[w] >> downshift);
			}
			else
			{ // Usual case, with a positive divisor
				for (w = 0; w < width; w++)
					dst[w].ival += (kdu_int16)(accumulator[w] >> downshift);
			}
		}
	}

	// Finally, write the results to any non-NULL dependencies, applying
	// offsets as required
	for (m = 0; m < N; m++)
	{
		kd_multi_line *line = components + m;
		kd_multi_line *dep = dependencies[m];
		if (dep != NULL)
			dep->copy(line, -dep->rev_offset, 0.0F);
	}
}


/* ========================================================================= */
/*                         kd_multi_dependency_block                         */
/* ========================================================================= */

/*****************************************************************************/
/*                    kd_multi_dependency_block::initialize                  */
/*****************************************************************************/

void
kd_multi_dependency_block::initialize(int stage_idx, int block_idx,
	kdu_tile tile, int num_block_inputs, int num_block_outputs,
	kd_multi_collection *input_collection,
	kd_multi_collection *output_collection,
	kd_multi_transform *owner)
{
	int N = num_block_inputs;
	assert(N >= num_block_outputs); // Some outputs might not be used
	int *scratch = owner->get_scratch_ints(3 * N); // Safe upper bound
	int *input_indices = scratch;
	int *output_indices = input_indices + N;
	int *active_outputs = output_indices + N;
	int m, n, num_stage_inputs, num_stage_outputs;
	tile.get_mct_block_info(stage_idx, block_idx, num_stage_inputs,
		num_stage_outputs, num_block_inputs, num_block_outputs,
		input_indices, output_indices, NULL, NULL);
	assert((num_stage_inputs == input_collection->num_components) &&
		(num_stage_outputs == output_collection->num_components));

	num_components = num_dependencies = N;
	components = new kd_multi_line[N];
	dependencies = new kd_multi_line *[N];
	memset(dependencies, 0, sizeof(kd_multi_line *)*(size_t)N);
	if (is_reversible)
	{
		i_matrix = new int[N*N];
		i_offsets = new int[N];
		int num_triang_coeffs = (N*(N + 1) / 2) - 1;
		int *i_cf = i_matrix + (N*N - num_triang_coeffs);
		tile.get_mct_dependency_info(stage_idx, block_idx, is_reversible,
			NULL, NULL, i_cf, i_offsets,
			active_outputs);
		assert(is_reversible);

		// Reorganize triangular coefficients into a square matrix
		for (m = 0; m < N; m++)
		{ // Scan rows of matrix
			for (n = 0; n < m; n++)
				i_matrix[m*N + n] = *(i_cf++);
			i_matrix[m*N + m] = (m == 0) ? 1 : (*(i_cf++));
			for (n = m + 1; n < N; n++)
				i_matrix[m*N + n] = 0;
		}
		assert((i_cf - i_matrix) == (N*N));
	}
	else
	{
		f_matrix = new float[N*N];
		f_offsets = new float[N];
		int num_triang_coeffs = (N*(N - 1)) / 2;
		float *f_cf = f_matrix + (N*N - num_triang_coeffs);
		tile.get_mct_dependency_info(stage_idx, block_idx, is_reversible,
			f_cf, f_offsets, NULL, NULL,
			active_outputs);
		assert(!is_reversible);

		// Reorganize triangular coefficients into a square matrix
		for (m = 0; m < N; m++)
		{ // Scan rows of matrix
			for (n = 0; n < m; n++)
				f_matrix[m*N + n] = *(f_cf++);
			for (; n < N; n++)
				f_matrix[m*N + n] = 0.0F;
		}
		assert((f_cf - f_matrix) == (N*N));
	}

	// For reversible transforms, see if the coefficients can be represented
	// as short integers; if not, we will need to use a more precise
	// sample representation.
	bool need_precise = false;
	if (is_reversible)
		for (n = 0; n < (N*N); n++)
			if ((i_matrix[n] > 0x7FFF) || (i_matrix[n] < -0x7FFF))
				need_precise = true;

	// Configure dependencies
	for (n = 0; n < N; n++)
	{
		dependencies[n] = input_collection->components[input_indices[n]];
		if (dependencies[n] == NULL)
			continue;
		dependencies[n]->num_consumers++;
		if (is_reversible)
			dependencies[n]->reversible = true;
		if (need_precise)
			dependencies[n]->need_precise = true;
	}

	// Configure transform components
	for (n = 0; n < N; n++)
	{
		kd_multi_line *line = components + n;
		line->block = this;
		line->need_precise = need_precise;
		line->reversible = is_reversible;
		line->need_irreversible = !is_reversible;
	}

	// Configure outputs; note that we leave the output offsets equal to 0, since
	// offsets are applied prior to dependency transforms -- they are captured
	// by the `f_offsets' and `i_offsets' members.  There may still be output
	// offsets, applied by the framework, to account for the fact that final
	// output components should have a signed representation.  Both types of
	// offsets need to be taken into account when applying the transform.
	for (n = 0; n < num_block_outputs; n++)
	{
		kd_multi_line *line = components + active_outputs[n];
		output_collection->components[output_indices[n]] = line;
	}
}

/*****************************************************************************/
/*              kd_multi_dependency_block::prepare_for_inversion             */
/*****************************************************************************/

const char *
kd_multi_dependency_block::prepare_for_inversion()
{
	int n;

	// Need only check that all components will be available.
	for (n = 0; n < num_components; n++)
		if (components[n].num_consumers < 1)
			return "Dependency transform block cannot be inverted or partially "
			"inverted unless a contiguous prefix of the output components "
			"can be computed by downstream transform blocks, or by the "
			"application supplying them.";

	for (n = 0; n < num_dependencies; n++)
		if ((!is_reversible) && (dependencies[n] != NULL) &&
			dependencies[n]->reversible)
			return "Encountered an irreversible dependency transform block "
			"which operates on reversible codestream sample data.  "
			"While we allow such transforms to be processed during "
			"decompression, it is unreasonable to generate reversibly "
			"compressed component samples using an irreversible inverse "
			"multi-component transform during compression.  Kakadu will "
			"not invert this transform during compression.  This can "
			"prevent the compression process from proceeding if there are "
			"no other paths back from the MCT output components to the "
			"codestream components.";

	outstanding_consumers = num_components;
	return NULL;
}

/*****************************************************************************/
/*             kd_multi_dependency_block::normalize_coefficients             */
/*****************************************************************************/

void
kd_multi_dependency_block::normalize_coefficients()
{
	if (is_reversible)
		return; // Only need to normalize for irreversible transforms

	int n, m, N = num_components;  assert(N == num_dependencies);
	bool need_precise = false;
	for (n = 0; n < N; n++)
	{
		kd_multi_line *dep = dependencies[n];
		kd_multi_line *line = components + n;
		if (line->bit_depth == 0)
		{
			need_precise = true; // Better not risk fixed-point overflow
			if (dep != NULL)
				line->bit_depth = dep->bit_depth; // Avoid scaling input samples
		}
		else if ((dep != NULL) && (dep->bit_depth == 0))
			need_precise = true;
		if (line->need_precise)
			need_precise = true;

		if (line->bit_depth > 0)
		{
			// Scale terms which use this line as an input
			float source_factor = (float)(1 << line->bit_depth);
			for (m = n + 1; m < N; m++)
				f_matrix[m*N + n] *= source_factor;

			// Scale terms which have this line as an output
			float target_factor = 1.0F / source_factor;
			for (m = 0; m < n; m++)
				f_matrix[n*N + m] *= target_factor;

			// Scale floating point offsets
			f_offsets[m] *= target_factor;
		}
	}

	if (need_precise)
		for (n = 0; n < N; n++)
		{
			components[n].need_precise = true;
			if (dependencies[n] != NULL)
				dependencies[n]->need_precise = true;
		}
}

/*****************************************************************************/
/*               kd_multi_dependency_block::create_short_matrix              */
/*****************************************************************************/

void
kd_multi_dependency_block::create_short_matrix()
{
	if ((short_matrix != NULL) || is_reversible)
		return;

	int m, n, N = num_components;  assert(N == num_dependencies);
	float val, max_val = 0.0F;
	for (m = 0; m < N; m++)
		for (n = 0; n < m; n++)
		{
			val = f_matrix[m*N + n];
			if (val > max_val)
				max_val = val;
			else if (val < -max_val)
				max_val = -val;
		}

	short_matrix = new kdu_int16[N*N];

	float factor = 1.0F;
	for (short_downshift = 0;
		((factor*max_val) <= 16383.0F) && (short_downshift < 16);
		factor *= 2.0F)
		short_downshift++;
	for (m = 0; m < N; m++)
	{
		for (n = 0; n < m; n++)
		{
			val = f_matrix[m*N + n] * factor;
			int ival = (int)floor(val + 0.5);
			if (ival >= (1 << 15))
				ival = (1 << 15) - 1;
			else if (ival < -(1 << 15))
				ival = -(1 << 15);
			short_matrix[m*N + n] = (kdu_int16)ival;
		}
		for (; n < N; n++)
			short_matrix[m*N + n] = 0;
	}
}

/*****************************************************************************/
/*                kd_multi_dependency_block::perform_transform               */
/*****************************************************************************/

void
kd_multi_dependency_block::perform_transform()
{
	int N = num_components;
	int m, n;

	assert(num_dependencies == N);
	for (m = 0; m < N; m++)
	{
		kd_multi_line *line = components + m;
		kd_multi_line *dep = dependencies[m];
		int w, width = line->line.get_width();
		if (!is_reversible)
		{
			if (dep == NULL)
				line->reset(0, f_offsets[m]);
			else
				line->copy(dep, 0, f_offsets[m]);
			if (m == 0)
				continue;

			if (line->line.get_buf32() != NULL)
			{ // Perform 32-bit floating point irreversible processing
				kdu_sample32 *dst = line->line.get_buf32();
				for (n = 0; n < m; n++)
				{
					kdu_sample32 *src = components[n].line.get_buf32();
					float factor = f_matrix[m*N + n];
					if (factor == 0.0F)
						continue;
					for (w = 0; w < width; w++)
						dst[w].fval += src[w].fval * factor;
				}
			}
			else
			{ // Perform 16-bit fixed point irreversible processing
				if (accumulator == NULL)
					accumulator = new kdu_int32[width];
				if (short_matrix == NULL)
					create_short_matrix();
				kdu_sample16 *dst = line->line.get_buf16();
				int downshift = short_downshift;
				kdu_int32 offset = (1 << downshift) >> 1; // Rounding offset
				for (w = 0; w < width; w++)
					accumulator[w] = offset;
				for (n = 0; n < m; n++)
				{
					kdu_sample16 *src = components[n].line.get_buf16();
					int factor = short_matrix[m*N + n];
					if (factor == 0)
						continue;
					for (w = 0; w < width; w++)
						accumulator[w] += src[w].ival * factor;
				}
				for (w = 0; w < width; w++)
					dst[w].ival += (kdu_int16)(accumulator[w] >> downshift);
			}
		}
		else
		{
			if (dep == NULL)
				line->reset(i_offsets[m], 0.0F);
			else
				line->copy(dep, i_offsets[m], 0.0F);
			if (m == 0)
				continue;

			int downshift, divisor = i_matrix[m*N + m];
			for (downshift = 0; (1 << downshift) < divisor; downshift++);
			if ((1 << downshift) != divisor)
			{
				KDU_ERROR(e, 0x23090500); e <<
					KDU_TXT("Multi-component reversible dependency transforms "
						"must have exact positive powers of 2 on the diagonal "
						"of their triangular coefficient matrix; these are the "
						"divisors used to scale and round the prediction "
						"terms.  The offending divisor is ") << divisor << ".";
			}
			kdu_int32 offset = ((1 << downshift) >> 1);
			if (accumulator == NULL)
				accumulator = new kdu_int32[width];
			for (w = 0; w < width; w++)
				accumulator[w] = offset;

			if (line->line.get_buf32() != NULL)
			{ // Perform 32-bit integer reversible processing
				kdu_sample32 *dst = line->line.get_buf32();
				for (n = 0; n < m; n++)
				{
					kdu_sample32 *src = components[n].line.get_buf32();
					int factor = i_matrix[m*N + n];
					if (factor == 0)
						continue;
					for (w = 0; w < width; w++)
						accumulator[w] += src[w].ival * factor;
				}
				for (w = 0; w < width; w++)
					dst[w].ival += accumulator[w] >> downshift;
			}
			else
			{ // Perform 16-bit integer reversible processing
				kdu_sample16 *dst = line->line.get_buf16();
				for (n = 0; n < m; n++)
				{
					kdu_sample16 *src = components[n].line.get_buf16();
					int factor = i_matrix[m*N + n];
					if (factor == 0)
						continue;
					for (w = 0; w < width; w++)
						accumulator[w] += src[w].ival * factor;
				}
				for (w = 0; w < width; w++)
					dst[w].ival += (kdu_int16)(accumulator[w] >> downshift);
			}
		}
	}

	// Finally add in any component offsets required for the output components
	for (m = 0; m < N; m++)
	{
		kd_multi_line *line = components + m;
		line->apply_offset(line->rev_offset, line->irrev_offset);
	}
}

/*****************************************************************************/
/*                kd_multi_dependency_block::perform_inverse                 */
/*****************************************************************************/

void
kd_multi_dependency_block::perform_inverse()
{
	int N = num_components;
	int m, n;

	assert(num_dependencies == N);
	for (m = N - 1; m >= 0; m--)
	{
		kd_multi_line *line = components + m;
		kd_multi_line *dep = dependencies[m];
		int w, width = line->line.get_width();
		if (!is_reversible)
		{
			if (line->line.get_buf32() != NULL)
			{ // Perform 32-bit floating point irreversible processing
				kdu_sample32 *dst = line->line.get_buf32();
				for (n = 0; n < m; n++)
				{
					kdu_sample32 *src = components[n].line.get_buf32();
					float factor = f_matrix[m*N + n];
					if (factor == 0.0F)
						continue;
					for (w = 0; w < width; w++)
						dst[w].fval -= src[w].fval * factor;
				}
			}
			else if (m > 0)
			{ // Perform 16-bit fixed point irreversible processing
				if (short_matrix == NULL)
					create_short_matrix();
				kdu_sample16 *dst = line->line.get_buf16();
				int downshift = short_downshift;
				kdu_int32 offset = (1 << downshift) >> 1; // Rounding offset
				if (accumulator == NULL)
					accumulator = new kdu_int32[width];
				for (w = 0; w < width; w++)
					accumulator[w] = offset;
				for (n = 0; n < m; n++)
				{
					kdu_sample16 *src = components[n].line.get_buf16();
					int factor = short_matrix[m*N + n];
					if (factor == 0)
						continue;
					for (w = 0; w < width; w++)
						accumulator[w] += src[w].ival * factor;
				}
				for (w = 0; w < width; w++)
					dst[w].ival -= (kdu_int16)(accumulator[w] >> downshift);
			}
			if (dep != NULL)
			{
				assert(!(dep->reversible || line->reversible));
				float scale = (1 << line->bit_depth) / (float)(1 << dep->bit_depth);
				dep->copy(line, 0, -dep->irrev_offset - scale * f_offsets[m]);
			}
		}
		else
		{
			if (m > 0)
			{
				int downshift, divisor = i_matrix[m*N + m];
				for (downshift = 0; (1 << downshift) < divisor; downshift++);
				if ((1 << downshift) != divisor)
				{
					KDU_ERROR(e, 0x23090503); e <<
						KDU_TXT("Multi-component reversible dependency transforms "
							"must have exact positive powers of 2 on the diagonal "
							"of their triangular coefficient matrix; these are the "
							"divisors used to scale and round the prediction "
							"terms.  The offending divisor is ") << divisor << ".";
				}
				kdu_int32 offset = ((1 << downshift) >> 1);
				if (accumulator == NULL)
					accumulator = new kdu_int32[width];
				for (w = 0; w < width; w++)
					accumulator[w] = offset;

				if (line->line.get_buf32() != NULL)
				{ // Perform 32-bit integer reversible processing
					kdu_sample32 *dst = line->line.get_buf32();
					for (n = 0; n < m; n++)
					{
						kdu_sample32 *src = components[n].line.get_buf32();
						int factor = i_matrix[m*N + n];
						if (factor == 0)
							continue;
						for (w = 0; w < width; w++)
							accumulator[w] += src[w].ival * factor;
					}
					for (w = 0; w < width; w++)
						dst[w].ival -= accumulator[w] >> downshift;
				}
				else
				{ // Perform 16-bit integer reversible processing
					kdu_sample16 *dst = line->line.get_buf16();
					for (n = 0; n < m; n++)
					{
						kdu_sample16 *src = components[n].line.get_buf16();
						int factor = i_matrix[m*N + n];
						if (factor == 0)
							continue;
						for (w = 0; w < width; w++)
							accumulator[w] += src[w].ival * factor;
					}
					for (w = 0; w < width; w++)
						dst[w].ival -= (kdu_int16)(accumulator[w] >> downshift);
				}
			}
			if (dep != NULL)
				dep->copy(line, -dep->rev_offset - i_offsets[m], 0.0F);
		}
	}
}


/* ========================================================================= */
/*                            kd_multi_dwt_block                             */
/* ========================================================================= */

/*****************************************************************************/
/*                       kd_multi_dwt_block::initialize                      */
/*****************************************************************************/

void
kd_multi_dwt_block::initialize(int stage_idx, int block_idx,
	kdu_tile tile, int num_block_inputs, int num_block_outputs,
	kd_multi_collection *input_collection,
	kd_multi_collection *output_collection,
	kd_multi_transform *owner)
{
	int *scratch =
		owner->get_scratch_ints(2 * num_block_inputs + 3 * num_block_outputs);
	int *input_indices = scratch;
	int *active_inputs = input_indices + num_block_inputs;
	int *output_indices = active_inputs + num_block_inputs;
	int *active_outputs = output_indices + num_block_outputs;
	int *rev_offsets = active_outputs + num_block_outputs;
	float *irrev_offsets = owner->get_scratch_floats(num_block_outputs);
	int num_stage_inputs, num_stage_outputs;
	tile.get_mct_block_info(stage_idx, block_idx, num_stage_inputs,
		num_stage_outputs, num_block_inputs, num_block_outputs,
		input_indices, output_indices,
		irrev_offsets, rev_offsets);
	assert((num_stage_inputs == input_collection->num_components) &&
		(num_stage_outputs == output_collection->num_components));

	// Find and record DWT-specific parameters
	int n, s, canvas_min, canvas_lim;
	const float *coefficients = NULL;
	const kdu_kernel_step_info *step_info =
		tile.get_mct_dwt_info(stage_idx, block_idx, is_reversible, num_levels,
			canvas_min, canvas_lim, num_steps, symmetric,
			sym_extension, coefficients,
			active_inputs, active_outputs);
	assert((step_info != NULL) && (num_levels > 0));
	num_coefficients = max_step_length = 0;
	steps = new kd_lifting_step[num_steps];
	for (s = 0; s < num_steps; s++)
	{
		steps[s].step_idx = (kdu_byte)s;
		steps[s].support_length = (kdu_byte)step_info[s].support_length;
		steps[s].downshift = (kdu_byte)step_info[s].downshift;
		steps[s].extend = 0; // Not used
		steps[s].support_min = (kdu_int16)step_info[s].support_min;
		steps[s].rounding_offset = (kdu_int16)step_info[s].rounding_offset;
		steps[s].coeffs = NULL;   steps[s].icoeffs = NULL;
		steps[s].add_shorts_first = false;
		steps[s].kernel_id = (kdu_byte)Ckernels_ATK;
		steps[s].reversible = is_reversible;
		num_coefficients += step_info[s].support_length;
		if (max_step_length < step_info[s].support_length)
			max_step_length = step_info[s].support_length;
	}
	src_bufs32 = new kdu_sample32 *[max_step_length];
	i_coefficients = new int[num_coefficients];
	f_coefficients = new float[num_coefficients];
	int next_coeff_idx = 0;
	for (s = 0; s < num_steps; s++)
	{
		steps[s].coeffs = f_coefficients + next_coeff_idx;
		steps[s].icoeffs = i_coefficients + next_coeff_idx;
		float fact, max_factor = 0.4F;
		for (n = 0; n < (int)steps[s].support_length; n++)
		{
			steps[s].coeffs[n] = fact = coefficients[next_coeff_idx + n];
			if (fact > max_factor)
				max_factor = fact;
			else if (fact < -max_factor)
				max_factor = -fact;
		}
		if (!is_reversible)
		{ // Find appropriate downshift and rounding_offset values
			for (steps[s].downshift = 16; max_factor >= 0.499F; max_factor *= 0.5F)
				steps[s].downshift--;
			steps[s].rounding_offset = (kdu_int16)((1 << steps[s].downshift) >> 1);
		}
		fact = (float)(1 << steps[s].downshift);
		for (n = 0; n < (int)steps[s].support_length; n++)
			steps[s].icoeffs[n] = (int)floor(0.5 + steps[s].coeffs[n] * fact);
		next_coeff_idx += steps[s].support_length;
	}

	// Find the region occupied by the active outputs
	int region_min = canvas_lim, region_lim = canvas_min;
	for (n = 0; n < num_block_outputs; n++)
	{
		int loc = active_outputs[n] + canvas_min;
		assert((loc >= canvas_min) && (loc < canvas_lim));
		if (loc < region_min)
			region_min = loc;
		if (loc >= region_lim)
			region_lim = loc + 1;
	}

	// Create the initial `kd_multi_dwt_levels' structure.
	kdu_kernels kernels;
	kernels.init(num_steps, step_info, coefficients, symmetric, sym_extension,
		is_reversible);

	levels = new kd_multi_dwt_level[num_levels];
	int lev_idx;
	double range = 1.0; // Assumed range of samples prior to DWT analysis
	float low_scale, high_scale; // Nominal scaling factors to apply after steps
	kernels.get_lifting_factors(n, low_scale, high_scale);
	assert(n == num_steps);
	int low_support_min, low_support_max, high_support_min, high_support_max;
	kernels.get_impulse_response(KDU_SYNTHESIS_LOW, n, // n used as dummy var
		&low_support_min, &low_support_max);
	kernels.get_impulse_response(KDU_SYNTHESIS_HIGH, n, // n used as dummy var
		&high_support_min, &high_support_max);
	for (lev_idx = num_levels - 1; lev_idx >= 0; lev_idx--)
	{
		kd_multi_dwt_level *lev = levels + lev_idx;
		lev->canvas_min = canvas_min;  lev->canvas_size = canvas_lim - canvas_min;
		int low_min = region_min - low_support_max;  low_min += (low_min & 1);
		int high_min = region_min - high_support_max;  high_min += 1 - (high_min & 1);
		int low_lim = region_lim - low_support_min;  low_lim -= 1 - (low_lim & 1);
		int high_lim = region_lim - high_support_min;  high_lim -= (high_lim & 1);
		region_min = (low_min < high_min) ? low_min : high_min;
		region_lim = (low_lim > high_lim) ? low_lim : high_lim;
		while ((region_min < canvas_min) || (region_lim > canvas_lim))
		{ // Reflect required samples which lie outside the canvas back into
		  // the interior of the canvas, using the symmetric extension rule;
		  // this could be overly conservative if the DWT kernel uses constant
		  // boundary extension instead.
			if (lev->canvas_size < 2)
			{ // This loop may iterate forever, since symmetric extension will
			  // not bring external locations into the canvas region.
				region_min = canvas_min; region_lim = canvas_lim;
			}
			if (region_min < canvas_min)
			{
				int refl = 2 * canvas_min - region_min;
				if (refl >= region_lim)
					region_lim = refl + 1;
				region_min = canvas_min;
			}
			if (region_lim > canvas_lim)
			{
				int refl = 2 * (canvas_lim - 1) - (region_lim - 1);
				if (refl < region_min)
					region_min = refl;
				region_lim = canvas_lim;
			}
		}
		lev->region_min = region_min;  lev->region_size = region_lim - region_min;
		lev->components = new kd_multi_line *[lev->region_size];
		for (n = 0; n < lev->region_size; n++)
			lev->components[n] = NULL;
		lev->dependencies = new kd_multi_line **[lev->region_size];
		for (n = 0; n < lev->region_size; n++)
			lev->dependencies[n] = NULL;

		// Setup subband gains and normalization
		lev->normalizing_shift = 0;
		if (is_reversible)
		{
			lev->low_range = 1.0F;
			lev->high_range = 2.0F;
		}
		else if (lev->canvas_size < 2)
		{ // Unit length irreversible signals are just passed through unaltered
			lev->low_range = (float)range;
			lev->high_range = (float)range;
		}
		else
		{
			int depth = num_levels - 1 - lev_idx;
			double lval, hval, bibo_max;
			const double *bibo_gains =
				kernels.get_bibo_gains(depth, 0, NULL, lval, hval);
			for (bibo_max = 1.0; n < num_steps; n++)
				if (bibo_gains[n] > bibo_max)
					bibo_max = bibo_gains[n];
			bibo_max *= range;
			double low_range = range / low_scale;   // Ranges we will have if we
			double high_range = range / high_scale; // do not scale after lifting
			double overflow_limit = 1.0 * (double)(1 << (16 - KDU_FIX_POINT));
			while (bibo_max > 0.75*overflow_limit)
			{
				lev->normalizing_shift++;
				low_range *= 0.5;
				high_range *= 0.5;
				bibo_max *= 0.5;
			}
			lev->low_range = (float)low_range;
			lev->high_range = (float)high_range;
			range = low_range; // Passed on to the next level
		}

		// Adjust canvas/region dimensions for next lower level and use the
		// information to size the low- and high-pass subbands
		canvas_min = (canvas_min + 1) >> 1;  canvas_lim = (canvas_lim + 1) >> 1;
		region_min = (region_min + 1) >> 1;  region_lim = (region_lim + 1) >> 1;
		lev->canvas_low_size = canvas_lim - canvas_min;
		lev->canvas_high_size = lev->canvas_size - lev->canvas_low_size;
		lev->region_low_size = region_lim - region_min;
		lev->region_high_size = lev->region_size - lev->region_low_size;
	}

	// Assign level component and dependency pointers.  To do this, we make
	// a first pass through the DWT levels, assigning dummy addresses to the
	// `kd_multi_dwt_level::dependencies' and `kd_multi_dwt_level::components'
	// array entries.  We then allocate the global `components' and
	// `dependencies' arrays and make a second pass through the levels to
	// convert these dummy addresses into valid addresses.
	int comp_idx = 0;
	kd_multi_line *dummy_comp_base = NULL;
	for (lev_idx = num_levels - 1; lev_idx >= 0; lev_idx--)
	{
		kd_multi_dwt_level *lev = levels + lev_idx;
		if (lev_idx == (num_levels - 1))
		{ // Top level; each component needs its own line.
			for (n = 0; n < lev->region_size; n++)
				lev->components[n] = dummy_comp_base + (comp_idx++);
		}
		else
		{ // For lower levels, we re-use low-pass lines from the next higher
		  // level wherever possible.
			for (n = 0; n < lev->region_size; n++)
			{
				int loc = lev->region_min + n; // Absolute location in level
				loc += loc; // Absolute location in next higher level
				int idx = loc - lev[1].region_min;
				if ((idx < 0) || (idx >= lev[1].region_size))
					lev->components[n] = dummy_comp_base + (comp_idx++);
				else
					lev->components[n] = lev[1].components[idx];
			}
		}
	}
	num_components = comp_idx;
	components = new kd_multi_line[num_components];

	kd_multi_line **dummy_dep_base = NULL;
	for (n = 0; n < num_components; n++)
	{
		kd_multi_line *line = components + n;
		line->block = this;
		line->reversible = is_reversible;
		line->need_irreversible = !is_reversible;
	}

	int dep_idx = 0;
	for (lev_idx = 0; lev_idx < num_levels; lev_idx++)
	{
		kd_multi_dwt_level *lev = levels + lev_idx;
		if (lev_idx == 0)
			for (n = 0; n < lev->region_low_size; n++)
				lev->dependencies[(lev->region_min & 1) + 2 * n] =
				dummy_dep_base + (dep_idx++);
		for (n = 0; n < lev->region_high_size; n++)
			lev->dependencies[1 - (lev->region_min & 1) + 2 * n] =
			dummy_dep_base + (dep_idx++);
	}
	num_dependencies = dep_idx;
	dependencies = new kd_multi_line *[num_dependencies];
	for (n = 0; n < num_dependencies; n++)
		dependencies[n] = NULL;

	for (lev_idx = 0; lev_idx < num_levels; lev_idx++)
	{
		kd_multi_dwt_level *lev = levels + lev_idx;
		for (n = 0; n < lev->region_size; n++)
		{
			lev->components[n] = components +
				(lev->components[n] - dummy_comp_base);
			if ((lev_idx == 0) || ((lev->region_min + n) & 1))
				lev->dependencies[n] = dependencies +
				(lev->dependencies[n] - dummy_dep_base);
			else
				assert(lev->dependencies[n] == NULL);
		}
	}

	// Now bind output components and input dependency components
	kd_multi_dwt_level *top_level = levels + (num_levels - 1);
	for (n = 0; n < num_block_outputs; n++)
	{
		int loc = active_outputs[n] + top_level->canvas_min;
		int idx = loc - top_level->region_min;
		assert((idx >= 0) && (idx < top_level->region_size));
		kd_multi_line *line = top_level->components[idx];
		assert(line == (components + idx));
		output_collection->components[output_indices[n]] = line;
		line->rev_offset = rev_offsets[n];
		line->irrev_offset = irrev_offsets[n];
	}
	for (n = 0; n < num_block_inputs; n++)
	{
		kd_multi_line **dep_ref = NULL;
		int loc = active_inputs[n];
		for (lev_idx = 0; lev_idx < num_levels; lev_idx++)
		{
			kd_multi_dwt_level *lev = levels + lev_idx;
			if (lev_idx == 0)
			{
				if (loc < lev->canvas_low_size)
				{ // This is a low-pass subband sample in the current level
					int abs_loc = lev->canvas_min + (lev->canvas_min & 1) + 2 * loc;
					int rel_loc = abs_loc - lev->region_min;
					if ((rel_loc >= 0) && (rel_loc < lev->region_size))
						dep_ref = lev->dependencies[rel_loc];
					break;
				}
				loc -= lev->canvas_low_size;
			}
			if (loc < lev->canvas_high_size)
			{ // This is a high-pass subband sample in the current level
				int abs_loc = lev->canvas_min + 1 - (lev->canvas_min & 1) + 2 * loc;
				int rel_loc = abs_loc - lev->region_min;
				if ((rel_loc >= 0) && (rel_loc < lev->region_size))
					dep_ref = lev->dependencies[rel_loc];
				break;
			}
			loc -= lev->canvas_high_size;
		}
		assert(dep_ref != NULL);
		*dep_ref = input_collection->components[input_indices[n]];
		if (*dep_ref == NULL)
			continue;
		(*dep_ref)->num_consumers++;
		if (is_reversible)
			(*dep_ref)->reversible = true;
	}
}

/*****************************************************************************/
/*                  kd_multi_dwt_block::prepare_for_inversion                */
/*****************************************************************************/

const char *
kd_multi_dwt_block::prepare_for_inversion()
{
	assert(num_levels > 0);
	kd_multi_dwt_level *lev = levels + num_levels - 1;
	int n;

	// Need only check that all components will be available.
	for (n = 0; n < lev->canvas_size; n++)
	{
		if ((lev->region_min != lev->canvas_min) ||
			(lev->region_size != lev->canvas_size) ||
			(lev->components[n]->num_consumers < 1))
			return "DWT transform block cannot be inverted unless all "
			"output components can be computed by downstream "
			"transform blocks in the multi-component transform "
			"network, or by the application supplying them.";
	}
	outstanding_consumers = lev->canvas_size;

	for (n = 0; n < num_dependencies; n++)
		if ((!is_reversible) && (dependencies[n] != NULL) &&
			dependencies[n]->reversible)
			return "Encountered an irreversible DWT transform block "
			"which operates on reversible codestream sample data.  "
			"While we allow such transforms to be processed during "
			"decompression, it is unreasonable to generate reversibly "
			"compressed component samples using an irreversible inverse "
			"multi-component transform during compression.  Kakadu will "
			"not invert this transform during compression.  This can "
			"prevent the compression process from proceeding if there are "
			"no other paths back from the MCT output components to the "
			"codestream components.";

	return NULL;
}

/*****************************************************************************/
/*                 kd_multi_dwt_block::normalize_coefficients                */
/*****************************************************************************/

void
kd_multi_dwt_block::normalize_coefficients()
{
	if (is_reversible)
		return; // Only need to normalize for irreversible transforms

	bool need_precise = false;
	int n, max_bit_depth = 0;
	for (n = 0; n < num_components; n++)
	{
		kd_multi_line *line = components + n;
		if (line->bit_depth > max_bit_depth)
			max_bit_depth = line->bit_depth;
		if (line->need_precise)
			need_precise = true;
	}
	if (max_bit_depth == 0)
		need_precise = true;
	for (n = 0; n < num_dependencies; n++)
	{
		kd_multi_line *dep = dependencies[n];
		if (dep == NULL)
			continue;
		if (dep->need_precise || (dep->bit_depth == 0))
			need_precise = true;
	}
	for (n = 0; n < num_components; n++)
	{
		kd_multi_line *line = components + n;
		line->need_precise = need_precise;
		assert(line->need_irreversible);
		if (line->bit_depth == 0)
			line->bit_depth = max_bit_depth;
		else if (line->bit_depth != max_bit_depth)
		{
			KDU_ERROR(e, 0x28090500); e <<
				KDU_TXT("Inconsistent bit-depths encountered amongst output "
					"image components produced by a DWT transform block embedded "
					"inside the multi-component transform network.  All output "
					"(i.e., synthesized) components produced by a single DWT block "
					"must be declared with the same bit-depth.  Anything else makes "
					"no sense, so Kakadu does not bother trying to accommodate this "
					"case.");
		}
	}
	for (n = 0; n < num_dependencies; n++)
		if (dependencies[n] != NULL)
			dependencies[n]->need_precise = need_precise;
}

/*****************************************************************************/
/*                  kd_multi_dwt_block::propagate_bit_depths                 */
/*****************************************************************************/

bool
kd_multi_dwt_block::propagate_bit_depths(bool need_input_bit_depth,
	bool need_output_bit_depth)
{
	if (!(need_input_bit_depth || need_output_bit_depth))
		return false;

	bool any_change = false;

	int n, out_depth = 0;
	for (n = 0; n < num_components; n++)
		if (components[n].bit_depth != 0)
		{
			if ((out_depth != 0) && (out_depth != components[n].bit_depth))
				return false;
			out_depth = components[n].bit_depth;
		}

	if (out_depth == 0)
	{
		if (need_input_bit_depth)
			return false;
		int max_ll_depth = 0, min_ll_depth = 0;
		for (n = 0; n < levels->region_size; n++)
		{
			if (levels->dependencies[n] == NULL)
				continue;
			kd_multi_line *dep = *(levels->dependencies[n]);
			if ((dep != NULL) && (dep->bit_depth != 0))
			{
				if (dep->bit_depth > max_ll_depth)
					max_ll_depth = dep->bit_depth;
				if ((min_ll_depth == 0) || (dep->bit_depth < min_ll_depth))
					min_ll_depth = dep->bit_depth;
			}
		}
		if ((min_ll_depth > 0) || (max_ll_depth == min_ll_depth))
			out_depth = min_ll_depth;
		else
			return false;
	}

	if (need_output_bit_depth)
		for (n = 0; n < num_components; n++)
			if (components[n].bit_depth == 0)
			{
				components[n].bit_depth = out_depth;
				any_change = true;
			}

	if (need_input_bit_depth)
		for (int lev_idx = 0; lev_idx < num_levels; lev_idx++)
		{
			kd_multi_dwt_level *lev = levels + lev_idx;
			for (n = 0; n < lev->region_size; n++)
			{
				if (lev->dependencies[n] == NULL)
					continue;
				kd_multi_line *dep = *(lev->dependencies[n]);
				if ((dep != NULL) && (dep->bit_depth == 0))
				{
					any_change = true;
					dep->bit_depth = out_depth + ((lev_idx == 0) ? 0 : 1);
				}
			}
		}

	return any_change;
}

/*****************************************************************************/
/*                    kd_multi_dwt_block::perform_transform                  */
/*****************************************************************************/

void
kd_multi_dwt_block::perform_transform()
{
	int n, lev_idx;
	for (lev_idx = 0; lev_idx < num_levels; lev_idx++)
	{
		kd_multi_dwt_level *lev = levels + lev_idx;

		// Start by transferring the subband data from the dependencies
		for (n = 0; n < lev->region_size; n++)
		{
			if (lev->dependencies[n] == NULL)
				continue;
			kd_multi_line *line = lev->components[n];
			kd_multi_line *dep = *(lev->dependencies[n]);
			if (dep == NULL)
				line->reset(0, 0.0F);
			else if (is_reversible)
				line->copy(dep, 0, 0.0F);
			else
			{ // Copy irreversible samples with appropriate scaling
				int w, width = line->size.x;
				float range_expansion;
				if ((n + lev->region_min) & 1)
					range_expansion = lev->high_range*0.5F;
				// MCT definition expects high-pass subbands to have a
				// nominal range twice as large as the synthesized data
				else
					range_expansion = lev->low_range;
				if (line->line.get_buf32() != NULL)
				{ // Working with 32-bit samples
					if (dep->reversible)
					{ // Convert reversible integers to irreversible floats
						float scale = range_expansion / (float)(1 << line->bit_depth);
						kdu_sample32 *sp = dep->line.get_buf32();
						kdu_sample32 *dp = line->line.get_buf32();
						for (w = 0; w < width; w++)
							dp[w].fval = sp[w].ival * scale;
					}
					else
					{ // Scale irreversible floats
						float scale = range_expansion *
							(float)(1 << dep->bit_depth) / (float)(1 << line->bit_depth);
						kdu_sample32 *sp = dep->line.get_buf32();
						kdu_sample32 *dp = line->line.get_buf32();
						for (w = 0; w < width; w++)
							dp[w].fval = sp[w].fval * scale;
					}
				}
				else
				{ // Working with 16-bit samples
					float scale = range_expansion / (float)(1 << line->bit_depth);
					if (dep->reversible)
						scale *= (1 << KDU_FIX_POINT);
					else
						scale *= (float)(1 << dep->bit_depth);
					int downshift = 0;
					for (; scale < 16383.0F; scale *= 2.0F, downshift++);
					for (; scale > 32767.0F; scale *= 0.5F, downshift--);
					kdu_int16 i_scale = (kdu_int16)floor(0.5 + scale);
					kdu_sample16 *sp = dep->line.get_buf16();
					kdu_sample16 *dp = line->line.get_buf16();
					if (downshift >= 0)
					{
						int val, offset = (1 << downshift) >> 1;
						for (w = 0; w < width; w++)
						{
							val = sp[w].ival;  val *= i_scale;
							val = (val + offset) >> downshift;
							dp[w].ival = (kdu_int16)val;
						}
					}
					else
					{ // Very unusual case
						int upshift = -downshift;
						for (w = 0; w < width; w++)
							dp[w].ival = (kdu_int16)
							((sp[w].ival*i_scale) << upshift);
					}
				}
			}
		}

		// Now for subband synthesis in this level
		if (lev->canvas_size == 1)
		{ // DWT synthesis consists of simply copying the single output line
		  // from the corresponding subband line, possibly with a scaling
		  // factor of 2.  Of course, the transform is performed in-place here,
		  // so no actual copying is required.
			if ((lev->region_min & 1) && is_reversible)
			{ // Halve the size of reversible samples
				kd_multi_line *line = lev->components[0];
				kdu_sample16 *sp16 = line->line.get_buf16();
				kdu_sample32 *sp32 = line->line.get_buf32();
				int w, width = line->size.x;
				if (sp16 != NULL)
					for (w = 0; w < width; w++)
						sp16[w].ival >>= 1;
				else
					for (w = 0; w < width; w++)
						sp32[w].ival >>= 1;
			}
			assert(lev->normalizing_shift == 0);
		}
		else
		{
			int r_min = lev->region_min;
			int r_max = r_min + lev->region_size - 1;
			for (int s = num_steps - 1; s >= 0; s--)
			{
				kd_lifting_step *step = steps + s;
				int parity = 1 - (s & 1); // Parity of the lines being updated here
				for (n = (lev->region_min^parity) & 1; n < lev->region_size; n += 2)
				{
					kd_multi_line *line = lev->components[n];
					int width = line->size.x;
					kdu_sample16 *dst16 = line->line.get_buf16();
					kdu_sample32 *dst32 = line->line.get_buf32();
					int src_idx = ((n + lev->region_min) ^ 1) + 2 * step->support_min;
					for (int i = 0; i < step->support_length; i++, src_idx += 2)
					{ // Fill in the source buffers
						int k = src_idx;
						while ((k < r_min) || (k > r_max))
							if (k < r_min)
								k = (sym_extension) ? (2 * r_min - k) : (r_min + ((r_min^k) & 1));
							else
								k = (sym_extension) ? (2 * r_max - k) : (r_max - ((r_max^k) & 1));
						k -= lev->region_min;
						assert((k >= 0) && (k < lev->region_size));
						if (dst16 != NULL)
							src_bufs16[i] = lev->components[k]->line.get_buf16();
						else
							src_bufs32[i] = lev->components[k]->line.get_buf32();
					}
					if (dst16 != NULL)
						perform_synthesis_lifting_step(step, src_bufs16, dst16, dst16,
							width, 0);
					else if (dst32 != NULL)
						perform_synthesis_lifting_step(step, src_bufs32, dst32, dst32,
							width, 0);
					else
						assert(0);
				}
			}

			if ((!is_reversible) && (lev->normalizing_shift > 0))
			{ // Need to upshift the synthesized samples before moving to the
			  // next level.
				for (n = 0; n < lev->region_size; n++)
				{
					kd_multi_line *line = lev->components[n];
					int w, width = line->size.x;
					if (line->line.get_buf32() != NULL)
					{
						kdu_sample32 *sp = line->line.get_buf32();
						float scale = (float)(1 << lev->normalizing_shift);
						for (w = 0; w < width; w++)
							sp[w].fval *= scale;
					}
					else
					{
						kdu_sample16 *sp = line->line.get_buf16();
						int upshift = lev->normalizing_shift;
						for (w = 0; w < width; w++)
							sp[w].ival <<= upshift;
					}
				}
			}
		}

		// Finally, apply any required offsets to the synthesized results
		if (lev_idx < (num_levels - 1))
			continue; // Final results appear only at the highest DWT level
		for (n = 0; n < lev->region_size; n++)
		{
			kd_multi_line *line = lev->components[n];
			line->apply_offset(line->rev_offset, line->irrev_offset);
		}
	}
}

/*****************************************************************************/
/*                    kd_multi_dwt_block::perform_inverse                    */
/*****************************************************************************/

void
kd_multi_dwt_block::perform_inverse()
{
	int n, lev_idx;
	for (lev_idx = num_levels - 1; lev_idx >= 0; lev_idx--)
	{
		kd_multi_dwt_level *lev = levels + lev_idx;

		// Start with subband analysis in this level
		if (lev->canvas_size == 1)
		{ // DWT analysis consists of simply copying the single input line to
		  // the corresponding subband line, possibly with a scaling factor
		  // of 2.  Of course, the transform is performed in-place here, so
		  // no actual copying is required.
			assert(lev->normalizing_shift == 0);
			if ((lev->canvas_min & 1) && is_reversible)
			{ // Double the size of reversible samples
				kd_multi_line *line = lev->components[0];
				kdu_sample16 *dp16 = line->line.get_buf16();
				kdu_sample32 *dp32 = line->line.get_buf32();
				int w, width = line->size.x;
				if (dp16 != NULL)
					for (w = 0; w < width; w++)
						dp16[w].ival <<= 1;
				else
					for (w = 0; w < width; w++)
						dp32[w].ival <<= 1;
			}
		}
		else
		{
			if ((!is_reversible) && (lev->normalizing_shift > 0))
			{ // Need to downshift the input samples before performing
			  // subband analysis
				for (n = 0; n < lev->region_size; n++)
				{
					kd_multi_line *line = lev->components[n];
					int w, width = line->size.x;
					if (line->line.get_buf32() != NULL)
					{
						kdu_sample32 *sp = line->line.get_buf32();
						float scale = 1.0F / (float)(1 << lev->normalizing_shift);
						for (w = 0; w < width; w++)
							sp[w].fval *= scale;
					}
					else
					{
						kdu_sample16 *sp = line->line.get_buf16();
						int downshift = lev->normalizing_shift;
						kdu_int16 offset = (kdu_int16)(1 << (downshift - 1));
						for (w = 0; w < width; w++)
							sp[w].ival = (sp[w].ival + offset) >> downshift;
					}
				}
			}

			// Now for the lifting steps
			int c_min = lev->canvas_min;
			int c_max = c_min + lev->canvas_size - 1;
			for (int s = 0; s < num_steps; s++)
			{
				kd_lifting_step *step = steps + s;
				int parity = 1 - (s & 1); // Parity of the lines being updated here
				for (n = (lev->region_min^parity) & 1; n < lev->region_size; n += 2)
				{
					kd_multi_line *line = lev->components[n];
					int width = line->size.x;
					kdu_sample16 *dst16 = line->line.get_buf16();
					kdu_sample32 *dst32 = line->line.get_buf32();
					int src_idx = ((n + lev->region_min) ^ 1) + 2 * step->support_min;
					for (int i = 0; i < step->support_length; i++, src_idx += 2)
					{ // Fill in the source buffers
						int k = src_idx;
						while ((k < c_min) || (k > c_max))
							if (k < c_min)
								k = (sym_extension) ? (2 * c_min - k) : (c_min + ((c_min^k) & 1));
							else
								k = (sym_extension) ? (2 * c_max - k) : (c_max - ((c_max^k) & 1));
						k -= lev->region_min;
						assert((k >= 0) && (k < lev->region_size));
						if (dst16 != NULL)
							src_bufs16[i] = lev->components[k]->line.get_buf16();
						else
							src_bufs32[i] = lev->components[k]->line.get_buf32();
					}
					if (dst16 != NULL)
						perform_analysis_lifting_step(step, src_bufs16, dst16, dst16,
							width, 0);
					else if (dst32 != NULL)
						perform_analysis_lifting_step(step, src_bufs32, dst32, dst32,
							width, 0);
					else
						assert(0);
				}
			}
		}

		// Finish by transferring the subband data to the dependencies
		for (n = 0; n < lev->region_size; n++)
		{
			if (lev->dependencies[n] == NULL)
				continue;
			kd_multi_line *line = lev->components[n];
			kd_multi_line *dep = *(lev->dependencies[n]);
			if (dep == NULL)
				continue;
			if (is_reversible)
				dep->copy(line, -dep->rev_offset, -dep->irrev_offset);
			else
			{ // Copy irreversible samples with appropriate scaling
				int w, width = line->size.x;
				float range_expansion;
				if ((n + lev->region_min) & 1)
					range_expansion = lev->high_range*0.5F;
				// MCT definition expects high-pass subbands to have a
				// nominal range twice as large as the synthesized data
				else
					range_expansion = lev->low_range;
				assert(!dep->reversible);
				float scale =
					(1 << line->bit_depth) / (range_expansion * (1 << dep->bit_depth));
				if (line->line.get_buf32() != NULL)
				{ // Scale irreversible floats
					float off = dep->irrev_offset;
					kdu_sample32 *sp = line->line.get_buf32();
					kdu_sample32 *dp = dep->line.get_buf32();
					for (w = 0; w < width; w++)
						dp[w].fval = sp[w].fval*scale - off;
				}
				else
				{ // Working with 16-bit samples
					int downshift = 0;
					for (; scale < 16383.0F; scale *= 2.0F, downshift++);
					for (; scale > 32767.0F; scale *= 0.5F, downshift--);
					kdu_int16 i_scale = (kdu_int16)floor(0.5 + scale);
					kdu_sample16 *sp = line->line.get_buf16();
					kdu_sample16 *dp = dep->line.get_buf16();
					int val, offset = (int)
						floor(0.5 + dep->irrev_offset*(1 << KDU_FIX_POINT));
					if (downshift >= 0)
					{
						offset = ((1 << downshift) >> 1) - (offset << downshift);
						for (w = 0; w < width; w++)
						{
							val = sp[w].ival;  val *= i_scale;
							val = (val + offset) >> downshift;
							dp[w].ival = (kdu_int16)val;
						}
					}
					else
					{ // Very unusual case
						int upshift = -downshift;
						for (w = 0; w < width; w++)
							dp[w].ival = (kdu_int16)
							(((sp[w].ival*i_scale) << upshift) - offset);
					}
				}
			}
		}
	}
}
