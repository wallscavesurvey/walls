/*****************************************************************************/
// File: multi_transform_local.h [scope = CORESYS/TRANSFORMS]
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
   Local definitions used by "multi_transform.cpp".  These are not to be
included from any other scope.
******************************************************************************/

#ifndef MULTI_TRANSFORM_LOCAL_H
#define MULTI_TRANSFORM_LOCAL_H

#include <assert.h>
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "transform_local.h"

// Defined here:
struct kd_multi_line;
struct kd_multi_collection;
class kd_multi_component;
class kd_multi_block;
class kd_multi_null_block;
class kd_multi_matrix_block;
class kd_multi_dependency_block;
class kd_multi_block;
class kd_multi_null_block;
class kd_multi_matrix_block;
class kd_multi_rxform_block;
class kd_multi_dependency_block;
struct kd_multi_dwt_level;
class kd_multi_dwt_block;
class kd_multi_transform;
class kd_multi_synthesis;
class kd_multi_analysis;


/*****************************************************************************/
/*                              kd_multi_line                                */
/*****************************************************************************/

struct kd_multi_line {
public: // Member functions
	kd_multi_line()
	{
		row_idx = -1;  num_consumers = outstanding_consumers = 0;
		reversible = need_irreversible = need_precise = is_constant = false;
		bit_depth = 0;  rev_offset = 0;  irrev_offset = 0.0F;
		bypass = NULL;  block = NULL;  collection_idx = -1;
	}
	void initialize() { reset(rev_offset, irrev_offset); }
	/* Do not call this function until after the `line' member has been
	   created.  Sets the line's contents equal to the value of the
	   `rev_offset' or `irrev_offset' member, depending on the
	   `reversible' flag.  Normally, this function need only be called
	   if `is_constant' is true. */
	void reset(int rev_off, float irrev_off);
	/* Sets the contents of the line to `rev_off' if it has a reversible
	   representation, else `irrev_off'. */
	void apply_offset(int rev_off, float irrev_off);
	/* Adds the offset defined by `rev_off' or `irrev_off', whichever
	   is appropriate, to the embedded `line's sample values.  Ignores the
	   internal `rev_offset' and `irrev_offset' members, although these may
	   well be the values used to derive the `rev_off' and `irrev_off'
	   arguments. */
	void copy(kd_multi_line *src, int rev_offset, float irrev_offset);
	/* Copies the contents of `src' to the present line.  The function
	   ignores any offsets which may be recorded in either object, using
	   the offset and scaling factors supplied via the function's
	   arguments instead (of course, these may be derived from one or
	   both objects' members).  Both lines must have the same width.
	   If the target line is reversible, the source line must also be
	   reversible, the `irrev_offset' argument is ignored and `rev_offset'
	   is added to the source sample values as they are copied.  If the
	   target line is irreversible, the `rev_offset' argument is ignored
	   and `irrev_offset' is added to the source samples after applying any
	   required conversions: if the source samples are also irreversible,
	   conversion means scaling by 2^{src->bit_depth} / 2^{this->bit_depth};
	   if the source samples are reversible, conversion means scaling by
	   1 / 2^{this->bit_depth}. */
public: // Data
	kdu_line_buf line;
	kdu_coords size; // Size of intermediate component to which line belongs
	int row_idx; // First valid row is 0 -- initialized to -1
	int num_consumers; // See below
	union {
		int outstanding_consumers; // See below
		bool waiting_for_inversion; // See below
	};
	bool reversible;   // True if producer or consumer needs reversible data
	bool need_irreversible; // If producer or consumer needs irreversible data
	bool need_precise; // If producer or consumer needs 32-bit samples
	bool is_constant; // True if there is no generator for this line
	int bit_depth; // 0 if not a codestream or final output component
	int rev_offset;
	float irrev_offset;
	kd_multi_line *bypass; // See below
	kd_multi_block *block;
	int collection_idx; // See below
};
/* Notes:
	  Each `kd_multi_line' object represents a single intermediate component.
   We describe the flow of information in terms of generators and consumers,
   taking the perspective of the decompressor, since this is the more
   complex case.  During compression, however, the roles of generator
   and consumer are interchanged.
	  The line associated with an intermediate component has at most one
   generator, but potentially multiple consumers.  Consumers include
   transform blocks and the application, which is a consumer for
   all final output components.  During decompression, the line cannot be
   advanced until all consumers have finished using its contents -- i.e.,
   until `outstanding_consumers' goes to 0.  When it does, a new line is
   generated, `row_idx' is advanced, and `outstanding_consumers' is reset
   to `num_consumers'.  During compression, the `outstanding_consumers'
   member is not used.  In this case, the `waiting_for_inversion' flag is
   involved -- it is set to true if the row identified by `row_idx' has
   been written by a consumer, but has not yet been used by the transform
   block.
	  If `block' is non-NULL, the line is generated by the referenced
   transform block.  Otherwise, the line either corresponds to a
   codestream image component or a constant component (`is_constant' is
   true).  For codestream components, the `collection_idx' identifies the
   index of the entry in the `kd_component_collection::components'
   array within the codestream component collection (maintained by
   `kd_multi_transform') which points to this line.  This is the only
   piece of information required to pull or push a new line to the
   relevant codestream component processing engine.  In all other cases,
   `collection_idx' will be negative.
	  If `bypass' is non-NULL, all accesses to this line should be
   treated as accesses to the line identified by `bypass'.  In this
   case, all offsets are collapsed into the line referenced by `bypass'
   so that no arithmetic is required here.  Also, in this case the
   `line' resource remains empty.  `bypass' pointers are found during
   a transform optimization phase; they serve principally to avoid
   line copying when implementing null transforms.
	  Note that no line which has `is_constant' set will be bypassed.  Also
   note that all line references found in `kd_multi_block::dependencies'
   or `kd_multi_collection::components' arrays are adjusted to ensure that
   they do not point to lines which have been bypassed -- they always
   point to the lines which actually have the source data of interest.
   This simplifies the processing of data. */

   /*****************************************************************************/
   /*                              kd_multi_block                               */
   /*****************************************************************************/

class kd_multi_block {
public: // Member functions
	kd_multi_block()
	{
		is_null_transform = true; // Changed by derived constructors
		num_components = 0;  components = NULL;  next = prev = NULL;
		num_dependencies = num_available_dependencies = 0; dependencies = NULL;
		outstanding_consumers = 0;
	}
	virtual ~kd_multi_block()
	{
		if (components != NULL) delete[] components;
		if (dependencies != NULL) delete[] dependencies;
	}
	virtual void
		initialize(int stage_idx, int block_idx, kdu_tile tile,
			int num_block_inputs, int num_block_outputs,
			kd_multi_collection *input_collection,
			kd_multi_collection *output_collection,
			kd_multi_transform *owner) = 0;
	/* This function does most of the work of configuring the transform
	   block.  It creates the `components' and `dependencies' array and
	   inserts output component references into the `output_collection'.
	   It initializes all transform coefficients and offset vectors based
	   directly on the parameters supplied via the `kdu_tile' interface --
	   these are based on the assumption that imagery flows through the
	   processing system at the bit-depths declared in the codestream's
	   `CBD' and `SIZ' marker segments.  Later, once these bit-depths
	   have been discovered, the `normalize_coefficients' function should
	   be called to adapt the transform coefficients and offset vectors
	   to Kakadu's normalization conventions -- as advertized in the
	   description of `kdu_multi_analysis' and `kdu_multi_synthesis'.
	   Note, however, that normalization only impacts irreversible
	   processing. */
	virtual void normalize_coefficients() { return; }
	/* This function is called after initialization and after the
	   bit-depth information associated with codestream and output image
	   components has been propagated as far as possible into the
	   network.  The function need not be implemented by null transforms
	   or reversible transforms.  For irreversible transforms which
	   actually do some processing, however, the transform coefficients
	   may need to be normalized to account for the fact that irreversible
	   image sample representations have a nominal range of 1, whereas
	   the coefficients have been initialized based on the assumption
	   that they have a nominal range of 2^B where B is the relevant
	   bit-depth.  The normalization of line offsets, recorded in
	   `kd_multi_line::irrev_offset' is handled by the general framework
	   in `kd_multi_transform::construct'.  In fact, this is guaranteed
	   to have been done prior to calling the present function. */
	virtual bool propagate_bit_depths(bool need_input_bit_depth,
		bool need_output_bit_depth)
	{
		return false;
	}
	/* This function is called to see if it is possible to infer the
	   bit-depths of one or more dependencies from the bit-depths of
	   the block's output components, or vice-versa.  In many cases, this
	   is a non-trivial problem, so for most block types, the function
	   immediately returns false, meaning that nothing is changed.  If,
	   however, the bit-depth of any input or any output can be
	   inferred, the function returns true.  For DWT blocks this can
	   often be done.  The `need_input_bit_depth' argument is true if
	   one or more dependencies has an unknown bit-depth, while the
	   `need_output_bit_depth' argument is true if one or more
	   block output components has an unknown bit-depth. */
	virtual void perform_transform() { return; }
	/* This function need not be implemented by null transforms.  It
	   is called automatically by the framework when `outstanding_consumers'
	   is 0 and all dependency lines have become available.  The function
	   should write appropriate values to all non-constant output lines
	   (the ones in the `components' array), applying any required
	   offsets.  The function should not adjust the state information in the
	   base object (e.g., `outstanding_consumers') or any of the output
	   lines, since this is done generically by the caller, for all
	   transform types. */
	virtual const char *prepare_for_inversion()
	{
		return "Unimplemented multi-component transform block inversion "
			"procedure.";
	}
	/* If the transform block can be inverted, based on the entries in the
	   `components' array which have non-zero `num_consumers' values
	   (remember that consumers for the forward transform are generators
	   for its inverse), the function returns NULL.  Otherwise, the
	   function returns a constant text string, explaining why inversion
	   was not possible.  If this ends up causing the multi-component
	   transform network to be non-invertible, the reason message will
	   be used to provide additional explanation to the user.  In any case,
	   if the function returns non-NULL, no attempt will be made to call
	   this object's `perform_inverse' function and the framework will
	   decrement the `num_consumers' count associated with all of its
	   dependencies and define all of its components to be constant --
	   meaning that downstream transform blocks need not write to them.
		  If the transform block can be inverted, this function should
	   leave the `num_outstanding_consumers' member equal to the number
	   of lines in the `components' array which must be filled in by
	   the application (or by inverting a downstream transform block)
	   before the `perform_inverse' function can be executed. */
	virtual void perform_inverse() { return; }
	/* This function need not be implemented by null transforms.  It is
	   called automatically by the framework when `outstanding_consumers'
	   is 0 and all dependency lines have become free for writing.  The
	   function will only be called if `prepare_for_inversion' previously
	   returned true (during network configuration).  The function is
	   expected to write the results to those lines referenced by its
	   `dependencies' array, skipping over those whose reference is NULL,
	   and subtracting whatever offsets are specified by the lines
	   which are written. */
public: // Data
	bool is_null_transform; // See below
	int num_components; // Number of lines produce/manipuated by this block
	kd_multi_line *components;
	int num_dependencies;
	kd_multi_line **dependencies;
	int num_available_dependencies;
	int outstanding_consumers; // Sum of all outstanding line consumers
	kd_multi_block *next;
	kd_multi_block *prev;
};
/* Notes:
	  This class manages a single transform block in the multi-component
   transformation process (forwards or backwards).  As with
   `kd_multi_line', the names `generator', `consumer' and `dependency'
   are selected with the decompressor (synthesis) in mind, since this is
   the more complex case.  During compression, the information flows in
   the opposite direction, but the names are not changed (for convenience).
	  Each transform block "owns" a set of component lines, as its
   resources.  In most cases, these correspond to the output components,
   which are passed along to the next transform stage, or as final
   decompressed output components.  However, some transform blocks may
   need a larger set of working components than the number of output
   (or even input) components which they produce (or consume).  In any
   event, these owned component lines are described by the `num_components'
   and `components' members.
	  The `num_dependencies' and `dependencies' fields identify the
   collection of input lines which are required for this transform block
   to produce output lines (during compression, the roles are reversed,
   so that `dependencies' actually identifies the lines which should be
   filled in when the output lines all become available and are processed;
   as mentioned, though, we describe things here primarily from the
   decompressor's perspective).  The pointers in the `dependencies'
   array refer either to codestream component lines or lines produced as
   outputs by other transform blocks.  It is possible that one or more
   entries in the `dependencies' array contains a NULL pointer.  In this
   case, the input line should be taken to be identically 0 for the
   purpose of performing the transform.
	  The `num_available_dependencies' member keeps track of the number of
   initial lines from the `dependencies' array which are already available
   for processing.  During decompression, this means that their
   `kd_multi_line::row_idx' field is equal to the index of the
   next row which needs to be generated by this block.  During compression
   (where the transform block must be inverted), this means that their
   `kd_multi_line::waiting_for_inversion' member is false.  In either case,
   the value of this member must reach `num_dependencies' before a new set
   of lines may be processed by the transform engine.
	  The `outstanding_consumers' member holds the sum of the
   `kd_multi_line::outstanding_consumers' members from all lines in the
   `components' array.  This value must reach zero before a new set of
   lines can be processed by the transform engine.
	  If `is_null_transform' is true, the transform block does no actual
   processing.  Instead, the first `num_dependencies' of the lines in
   `components' are directly taken from their corresponding dependencies
   and any remaining lines in the `components' array have their
   `is_constant' flag set to true.  In this case, the
   `outstanding_consumers' and `num_available_dependencies' members
   should be ignored. */

   /*****************************************************************************/
   /*                            kd_multi_null_block                            */
   /*****************************************************************************/

class kd_multi_null_block : public kd_multi_block {
public: // Member functions
	kd_multi_null_block()
	{
		is_null_transform = true;
	}
	virtual void
		initialize(int stage_idx, int block_idx, kdu_tile tile,
			int num_block_inputs, int num_block_outputs,
			kd_multi_collection *input_collection,
			kd_multi_collection *output_collection,
			kd_multi_transform *owner);
};

/*****************************************************************************/
/*                           kd_multi_matrix_block                           */
/*****************************************************************************/

class kd_multi_matrix_block : public kd_multi_block {
public: // Member functions
	kd_multi_matrix_block()
	{
		is_null_transform = false;
		coefficients = inverse_coefficients = NULL;
		short_coefficients = NULL;  short_accumulator = NULL;  work = NULL;
	}
	virtual ~kd_multi_matrix_block()
	{
		if (coefficients != NULL) delete[] coefficients;
		if (inverse_coefficients != NULL) delete[] inverse_coefficients;
		if (short_coefficients != NULL) delete[] short_coefficients;
		if (short_accumulator != NULL) delete[] short_accumulator;
		if (work != NULL) delete[] work;
	}
	virtual void
		initialize(int stage_idx, int block_idx, kdu_tile tile,
			int num_block_inputs, int num_block_outputs,
			kd_multi_collection *input_collection,
			kd_multi_collection *output_collection,
			kd_multi_transform *owner);
	virtual void normalize_coefficients();
	virtual void perform_transform();
	virtual const char *prepare_for_inversion();
	virtual void perform_inverse();
private: // Helper functions
	void create_short_coefficients(int width);
	/* This function creates the `short_coefficients' to represent
	   the `coefficients' array in fixed-point, choosing an appropriate
	   value for `short_downshift'.  It also creates the `short_accumulator'
	   array for holding intermediate results. */
	void create_short_inverse_coefficients(int width);
	/* Same as `create_short_coefficients', but creates the
	   `short_coefficients' array to hold a fixed-point representation of
	   the `inverse_coefficients' array, rather than the `coefficients'
	   array. */
private: // Data
	float *coefficients;
	float *inverse_coefficients; // Created by `prepare_for_inversion'
	kdu_int16 *short_coefficients;
	kdu_int32 *short_accumulator; // Working buffer for fixed-point processing
	int short_downshift;
	double *work; // Working memory for creating inverse transforms
};

/*****************************************************************************/
/*                           kd_multi_rxform_block                           */
/*****************************************************************************/

class kd_multi_rxform_block : public kd_multi_block {
public: // Member functions
	kd_multi_rxform_block()
	{
		is_null_transform = false;
		coefficients = NULL;  accumulator = NULL;
	}
	virtual ~kd_multi_rxform_block()
	{
		if (coefficients != NULL) delete[] coefficients;
		if (accumulator != NULL) delete[] accumulator;
	}
	virtual void
		initialize(int stage_idx, int block_idx, kdu_tile tile,
			int num_block_inputs, int num_block_outputs,
			kd_multi_collection *input_collection,
			kd_multi_collection *output_collection,
			kd_multi_transform *owner);
	virtual void perform_transform();
	virtual const char *prepare_for_inversion();
	virtual void perform_inverse();
private: // Data
	int *coefficients;
	kdu_int32 *accumulator;
};

/*****************************************************************************/
/*                         kd_multi_dependency_block                         */
/*****************************************************************************/

class kd_multi_dependency_block : public kd_multi_block {
public: // Member functions
	kd_multi_dependency_block(bool is_reversible)
	{
		is_null_transform = false;  this->is_reversible = is_reversible;
		num_coeffs = 0; short_matrix = NULL;  short_downshift = 0; accumulator = NULL;
		i_matrix = i_offsets = NULL;  f_matrix = f_offsets = NULL;
	}
	virtual ~kd_multi_dependency_block()
	{
		if (i_matrix != NULL) delete[] i_matrix;
		if (i_offsets != NULL) delete[] i_offsets;
		if (f_matrix != NULL) delete[] f_matrix;
		if (f_offsets != NULL) delete[] f_offsets;
		if (short_matrix != NULL) delete[] short_matrix;
		if (accumulator != NULL) delete[] accumulator;
	}
	virtual void
		initialize(int stage_idx, int block_idx, kdu_tile tile,
			int num_block_inputs, int num_block_outputs,
			kd_multi_collection *input_collection,
			kd_multi_collection *output_collection,
			kd_multi_transform *owner);
	virtual void normalize_coefficients();
	virtual void perform_transform();
	virtual const char *prepare_for_inversion();
	virtual void perform_inverse();
private: // Helper functions
	void create_short_matrix();
private: // Data
	bool is_reversible;
	int num_coeffs;
	int *i_matrix, *i_offsets;    // Note: matrices are square, even if only
	float *f_matrix, *f_offsets;  // lower triangular part is used
	kdu_int16 *short_matrix; // See below
	int short_downshift;
	kdu_int32 *accumulator;
};

/*****************************************************************************/
/*                            kd_multi_dwt_level                             */
/*****************************************************************************/

struct kd_multi_dwt_level {
public: // Member functions
	kd_multi_dwt_level()
	{
		canvas_min = canvas_size = canvas_low_size = canvas_high_size = 0;
		region_min = region_size = region_low_size = region_high_size = 0;
		components = NULL;  dependencies = NULL;
	}
	~kd_multi_dwt_level()
	{
		if (components != NULL) delete[] components;
		if (dependencies != NULL) delete[] dependencies;
	}
public: // data
	int canvas_min, canvas_size, canvas_low_size, canvas_high_size;
	int region_min, region_size, region_low_size, region_high_size;
	kd_multi_line **components; // See below
	kd_multi_line ***dependencies; // See below
	int normalizing_shift; // See below
	float low_range, high_range; // See below
};
/* Notes:
	  The `canvas_min' and `canvas_size' members define the region occupied
   by the complete DWT on its own 1D canvas.  `canvas_low_size' and
   `canvas_high_size' hold the number of low- and high-pass subband samples
   found within this region.
	  The `region_min', `region_size', `region_low_size' and
   `region_high_size' members play the same roles as `canvas_min' through
   `canvas_high_size', except for a region of interest within the canvas;
   the region of interest is derived from the set of active image components
   identified by `kdu_tile::get_mct_dwt_info'.
	  The `components' array here contains `region_size' pointers into the
   `kd_multi_dwt_block' object's `components' array.
	  The `dependencies' array contains `region_size' pointers
   into the `kd_multi_dwt_block' object's `dependencies' array.  NULL
   entries in this array correspond to lines which are derived from the
   next lower DWT stage (during DWT synthesis -- i.e., decompression);
   non-NULL pointers which point to NULL entries in the
   `kd_multi_dwt_block::dependencies' array correspond to lines which must
   be initialized to 0 prior to DWT synthesis; otherwise, the relevant
   dependency (input) is to be copied to the corresponding `components'
   line prior to DWT synthesis at this level.
	  The `normalizing_shift' member is the amount to downshift the inputs
   to this level during analysis or upshift the outputs from this level
   during synthesis so as to avoid dynamic range violations during
   fixed-point irreversible processing.
	  The `low_range' and `high_range' members identify the amount by which
   low and high subband samples produced at this level (during analysis)
   are scaled, relative to the unit dynamic range that would be expected
   if the DWT were applied using low- and high-pass analysis filters with
   unit pass-band gain, assuming that the input components all have a
   unit dynamic range.  These scaling factors are of interest only when
   working with irreversible transforms, since they affect the amount by
   which samples must be scaled as they are transferred between the
   lines referenced via the `components' and `dependencies' arrays.  These
   range values depart from 1.0 only because we prefer to avoid introducing
   subband gains into each DWT stage. */

   /*****************************************************************************/
   /*                            kd_multi_dwt_block                             */
   /*****************************************************************************/

class kd_multi_dwt_block : public kd_multi_block {
public: // Member functions
	kd_multi_dwt_block()
	{
		is_null_transform = false;  num_levels = 0;  levels = NULL;
		num_steps = max_step_length = 0;  steps = NULL;
		num_coefficients = 0;  f_coefficients = NULL;  i_coefficients = NULL;
		src_bufs32 = NULL;  assert(src_bufs16 == NULL);
	}
	virtual ~kd_multi_dwt_block()
	{
		if (levels != NULL) delete[] levels;
		if (steps != NULL) delete[] steps;
		if (f_coefficients != NULL) delete[] f_coefficients;
		if (i_coefficients != NULL) delete[] i_coefficients;
		if (src_bufs32 != NULL) { delete[] src_bufs32;  src_bufs32 = NULL; }
		assert(src_bufs16 == NULL); // `src_bufs32' & `src_bufs16' are aliased
	}
	virtual void
		initialize(int stage_idx, int block_idx, kdu_tile tile,
			int num_block_inputs, int num_block_outputs,
			kd_multi_collection *input_collection,
			kd_multi_collection *output_collection,
			kd_multi_transform *owner);
	virtual void normalize_coefficients();
	virtual bool propagate_bit_depths(bool need_input_bit_depth,
		bool need_output_bit_depth);
	virtual void perform_transform();
	virtual const char *prepare_for_inversion();
	virtual void perform_inverse();
private: // Data
	int num_levels;
	kd_multi_dwt_level *levels; // See below
	bool is_reversible;
	bool symmetric, sym_extension;
	int num_steps;
	kd_lifting_step *steps;
	int num_coefficients; // Total number of lifting step coefficients
	float *f_coefficients; // Resource referenced by `kd_lifting_step::fcoeffs'
	int *i_coefficients; // Resource referenced by `kd_lifting_step::icoeffs'
	int max_step_length; // Longest lifting step support
	union {
		kdu_sample32 **src_bufs32;   // These have `max_step_length' entries
		kdu_sample16 **src_bufs16;
	};
};
/* Notes:
	 The `levels' array runs in reverse order, starting from the lowest
   DWT level (i.e., the one containing the L-band).
*/

/*****************************************************************************/
/*                           kd_multi_collection                             */
/*****************************************************************************/

struct kd_multi_collection {
public: // Member functions
	kd_multi_collection()
	{
		num_components = 0;  components = NULL;  next = prev = NULL;
	}
	~kd_multi_collection()
	{
		delete[] components;
	}
public: // Data
	int num_components;
	kd_multi_line **components;
	kd_multi_collection *next;
	kd_multi_collection *prev;
};
/* Notes:
	  This structure is used to manage the collection of components at
   the interface between two transform stages, or at the input to the
   first codestream stage or output from the last codestream stage.  Lists
   of component collections start at the codestream components and end
   at the output image components, produced during decompression.  The
   interpretation is the same during compression, but the output
   components become inputs components and vice-versa.
	  The `components' array contains `num_components' pointers to the
   lines which represent each component in the collection.  Most of these
   lines are "owned" by `kd_multi_block' objects.  However, if the
   referenced line's `kd_multi_line::block' member is NULL, the line
   belongs either to the `kd_multi_transform::constant_output_lines' array
   or else it belongs to the `kd_multi_transform::codestream_components'
   array.  The latter is used exclusively by the first collection in
   the `kd_multi_transform' object -- the one referenced by
   `kd_multi_transform::codestream_collection'. */

   /*****************************************************************************/
   /*                            kd_multi_component                             */
   /*****************************************************************************/

class kd_multi_component : public kdu_worker {
public: // Member functions
	kd_multi_component()
	{
		comp_idx = 0;  env_queue = NULL;  double_buffering = false;
		next_available_row = first_row_in_progress = 0;
		num_available_rows = num_rows_in_progress = 0;
		max_available_rows = num_buffer_rows = 0;  buffer = NULL;
	}
	virtual ~kd_multi_component()
	{
		if (pull_ifc.exists()) pull_ifc.destroy();
		if (push_ifc.exists()) push_ifc.destroy();
		if (buffer != NULL) delete[] buffer;
	}
	virtual void do_job(kdu_thread_entity *ent, int job_idx)
	{
		kdu_thread_env *env = (kdu_thread_env *)ent;
		for (int k = 0; k < num_rows_in_progress; k++)
			if (pull_ifc.exists())
				pull_ifc.pull(buffer[first_row_in_progress + k], env);
			else
				push_ifc.push(buffer[first_row_in_progress + k], env);
	}
public: // Data
	int comp_idx;
	kdu_thread_queue *env_queue;
	kd_multi_line line;
	bool double_buffering;
	int next_available_row;
	int num_available_rows;
	int first_row_in_progress;
	int num_rows_in_progress;
	int max_available_rows;
	int num_buffer_rows;
	kdu_line_buf *buffer;
	kdu_pull_ifc pull_ifc; // Engine used during synthesis
	kdu_push_ifc push_ifc; // Engine used during analysis
};
/* Notes:
	 This structure is used to form the `codestream_components' array
   in `kd_multi_transform'.  It manages a collection of `kd_multi_line'
   objects, which represent a stripe within the associated codestream
   image component.  The various members have the following interpretation:
	 * `comp_idx' is the index of the corresponding codestream image
   component, as required by the `kdu_tile::access_component' function.
   This is not necessarily the ordinal position of the present structure
   within the `kd_multi_transform::codestream_components' array, since
   elements in the array follow the same order as the elements of the
   `kd_multi_collection::components' array in the object referenced by
   `kd_multi_transform::codestream_collection'.
	 * `env_queue' is NULL unless multi-threaded processing is being
   used, in which case `env_queue' is a unique (i.e., component-specific)
   sub-queue of the queue referenced by `kd_multi_transform::env_queue'.
	 * `line' is used to maintain the state of the current line being
   referenced from the `kd_multi_transform::codestream_collection' object.
   Its internal `line.line' member is actually a copy of one of the line
   buffers in the `buffer' array.
	 * `next_available_row' is the index, within the `buffer' array, of
   the next line to be accessed as `line'.
	 * `num_available_rows' identifies the number of remaining rows
   in the `buffer' array.  When this value reaches 0, codestream component
   processing is required (i.e., DWT analysis/synthesis operations and
   block encoding/decoding).
	 * `first_row_in_progress' and `num_rows_in_progress' together identify
   a section of the elements in the `buffer' which currently belong to an
   active job on the `env_queue'.  If `num_rows_in_progress' is > 0 when
   the multi-component processing processes run out of available lines
   (i.e., `num_available_rows' goes to 0), the relevant thread will
   wait for completion of the `env_queue', then mark its rows in progress
   as completed rows.
	 * `max_available_rows' is the maximum value which `num_available_rows'
   can ever hold.  It is the maximum number of codestream component rows
   which can be processed consecutively.
	 * `double_buffering' can be true only if `env_queue' is non-NULL.  If
   false, `num_rows_in_progress' will always be 0.  If true, the `buffer'
   array actually holds twice as many line buffers as `max_available_rows'
   would suggest, allowing one set to be available to the multi-component
   transform, while the other is subject to asynchronous processing by other
   threads, as a job on the `env_queue'.
	 * `num_buffer_rows' holds either `max_available_rows' or twice this
   value, depending on whether or not `double_buffering' is true.
	 * `buffer' holds `num_buffer_rows' line buffers. */

	 /*****************************************************************************/
	 /*                            kd_multi_transform                             */
	 /*****************************************************************************/

class kd_multi_transform {
	/* Common base for `kd_multi_analysis' and `kd_multi_synthesis'. */
public: // Member functions
	kd_multi_transform()
	{
		use_ycc = false; xform_blocks = block_tail = NULL;
		codestream_collection = output_collection = NULL;
		constant_output_lines = NULL; codestream_components = NULL;
		env_queue = NULL;  max_scratch_ints = max_scratch_floats = 0;
		scratch_ints = NULL;  scratch_floats = NULL;
	}
	virtual ~kd_multi_transform();
	int *get_scratch_ints(int num_elts);
	float *get_scratch_floats(int num_elts);
protected: // Utility functions
	void construct(kdu_codestream codestream, kdu_tile tile,
		bool force_precise, bool skip_ycc, bool want_fastest,
		int processing_stripe_height, kdu_thread_env *env,
		kdu_thread_queue *env_queue, bool double_buffering);
	/* Does almost all the work of `kd_multi_analysis::create' and
	   `kd_multi_synthesis::create'. */
	void create_resources();
	/* Called from within `kd_multi_analysis::create' or
	   `kd_multi_synthesis::create' after first calling `construct' and
	   then creating all `kdu_push_ifc'- and `kdu_pull_ifc'-derived engines.
	   This function pre-creates all the `kdu_line_buf' resources, then
	   finalizes the `allocator' object and finalizes the creation
	   of all `kdu_line_buf' resources.  The function also initializes
	   the contents of all lines which have no generator. */
private: // Helper functions for `construct'
	bool propagate_knowledge(bool force_precise,
		bool force_hanging_constants_to_reversible);
	/* This function is called from `construct' once all transform blocks
	   and lines have been constructed.  At this point, the bit-depth of
	   each codestream component line and each output component line should
	   be known; all other lines should have a 0 bit-depth.  Also, at this
	   point, the `reversible', `need_irreversible' and `need_precise'
	   flags might have been set only for some lines -- typically
	   codestream component lines, or lines produced by transform blocks
	   with known requirements.  Finally, dimensions should be known only
	   for output image components and codestream image components.  Each
	   time this function is called, it passes through the network,
	   propagating the available knowledge wherever possible.  If nothing
	   changes, the function returns false and does not need to be called
	   again.  Otherwise, it must be invoked iteratively.
		 The final argument should initially be false.  However, this may
	   lead to some constant lines remaining in an indeterminate state
	   with regard to reversibility.  A second iterative invokation of
	   the function with `force_hanging_constants_to_reversible' will
	   then fix the problem. */
protected: // Data
	bool use_ycc;
	kd_multi_block *xform_blocks;
	kd_multi_block *block_tail; // Tail of `xform_blocks' list
	kd_multi_component *codestream_components;
	kd_multi_collection *codestream_collection; // Linked list of collections
	kd_multi_collection *output_collection;
	kd_multi_line *constant_output_lines;
	kdu_sample_allocator allocator;
	kdu_thread_queue *env_queue; // Parent of the above queues.
private:
	int max_scratch_ints;
	int *scratch_ints;
	int max_scratch_floats;
	float *scratch_floats;
};

/*****************************************************************************/
/*                            kd_multi_synthesis                             */
/*****************************************************************************/

class kd_multi_synthesis : public kd_multi_synthesis_base,
	public kd_multi_transform {
public: // Member functions
	kd_multi_synthesis() { output_row_counters = NULL; }
	virtual ~kd_multi_synthesis();
	virtual void terminate_queues(kdu_thread_env *env);
	kdu_long create(kdu_codestream codestream, kdu_tile tile,
		bool force_precise, bool skip_ycc, bool want_fastest,
		int processing_stripe_height, kdu_thread_env *env,
		kdu_thread_queue *env_queue, bool double_buffering);
	virtual kdu_coords get_size(int comp_idx);
	virtual kdu_line_buf *get_line(int comp_idx, kdu_thread_env *env);
	virtual bool is_line_precise(int comp_idx);
	virtual bool is_line_absolute(int comp_idx);
private: // Helper functions
	kdu_line_buf *get_line(kd_multi_line *line, int tgt_row_idx,
		kdu_thread_env *env);
	/* Recursive function which does all the work of the public `get_line'
	   function. */
private: // Data
	int *output_row_counters;
};

/*****************************************************************************/
/*                             kd_multi_analysis                             */
/*****************************************************************************/

class kd_multi_analysis : public kd_multi_analysis_base,
	public kd_multi_transform {
public: // Member functions
	kd_multi_analysis() { source_row_counters = NULL; }
	virtual ~kd_multi_analysis();
	virtual void terminate_queues(kdu_thread_env *env);
	kdu_long create(kdu_codestream codestream, kdu_tile tile,
		bool force_precise, kdu_roi_image *roi, bool want_fastest,
		int processing_stripe_height, kdu_thread_env *env,
		kdu_thread_queue *env_queue, bool double_buffering);
	virtual kdu_coords get_size(int comp_idx);
	virtual kdu_line_buf *
		exchange_line(int comp_idx, kdu_line_buf *written, kdu_thread_env *env);
	virtual bool is_line_precise(int comp_idx);
	virtual bool is_line_absolute(int comp_idx);
private: // Helper functions
	void prepare_network_for_inversion();
	/* This function determines whether or not the set of transform blocks
	   is sufficient to produce all codestream components.  If
	   not, an error will be generated.  Where there are multiple ways to
	   generate a given codestream component, no decision is taken here
	   regarding which method will be used -- the outcome in this case
	   may depend on the order in which the transform blocks are inverted,
	   which depends on the order in which the application pushes image
	   data into the `kdu_multi_analysis' interface. */
	void advance_line(kd_multi_line *line, int new_row_idx,
		kdu_thread_env *env);
	/* This function is the workhorse for the public `exchange_line'
	   function.  The function should be called only after writing new
	   contents to the `line->line' buffer.  The present function
	   increments `line->row_idx' to reflect the location of the
	   row which has just been written -- the first time the function
	   is called, `row_idx' will be incremented from -1 to 0.  This
	   new value should be identical to the `new_row_idx' argument.
	   If possible, the function runs the `perform_inverse' function of
	   any transform block which owns the line; for codestream component
	   lines, the function pushes the line directly to the relevant
	   component's processing engine.  If any of these functions succeed,
	   the `waiting_for_inversion' member is set to false.  Otherwise, it
	   is set to true.
		  Whenever a transform block can be inverted, the function
	   recursively invokes itself on each of its dependencies.  Note also
	   that where a dependency has multiple consumers, if the line is found
	   to have already been written by another consumer, the relevant
	   `kd_multi_block::dependencies' entry is set to NULL here, before
	   invoking `kd_multi_block::perform_inverse'.  This means that the
	   latter function is always free to write directly to its dependency
	   line buffers, whenever it is invoked.  It also means that the
	   present function will never be invoked on a line which reports
	   having more than one consumer. */
private: // Data
	int *source_row_counters;
};



#endif // MULTI_TRANSFORM_LOCAL_H
