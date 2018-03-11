/*****************************************************************************/
// File: clientx_local.h [scope = APPS/KDU_CLIENT]
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
  Private definitions used in the implementation of the "kdu_clientx" class.
******************************************************************************/

#ifndef CLIENTX_LOCAL_H
#define CLIENTX_LOCAL_H

#include "kdu_clientx.h"

// Defined here:
struct kdcx_stream_mapping;
struct kdcx_layer_member;
struct kdcx_layer_mapping;
struct kdcx_comp_instruction;
struct kdcx_comp_mapping;

/*****************************************************************************/
/*                           kdcx_stream_mapping                             */
/*****************************************************************************/

struct kdcx_stream_mapping {
  /* This object represents information which might be recovered from a
     codestream header box, which is required for mapping codestream
     contexts to codestreams and components. */
  public: // Member functions
    kdcx_stream_mapping()
      {
        is_complete = false;
        num_channels = num_components = 0;
        component_indices = NULL;
      }
    ~kdcx_stream_mapping()
      { if (component_indices != NULL) delete[] component_indices; }
    void parse_ihdr_box(jp2_input_box *box);
      /* Parses to the end of the image header box leaving it open, using
         the information to set the `size' and `num_components' members. */
    void parse_cmap_box(jp2_input_box *box);
      /* Parses to the end of the component mapping box leaving it open,
         using the information to set the `num_channels' and
         `component_indices' members. */
    void finish(kdcx_stream_mapping *defaults);
      /* Fills in any information not already parsed, by using defaults
         (parsed from within the JP2 header box) as appropriate. */
  public: // Data
    bool is_complete; // True once all boxes parsed and `finish' called
    jp2_input_box jpch_box; // Used to keep track of an incomplete jpch box
    jp2_input_box jpch_sub; // Used to keep track of an incomplete sub-box
    kdu_coords size;
    int num_components;
    int num_channels;
    int *component_indices; // Lists component indices for each channel
  };

/*****************************************************************************/
/*                           kdcx_layer_member                               */
/*****************************************************************************/

struct kdcx_layer_member {
  public: // Member functions
    kdcx_layer_member()
      { codestream_idx = num_components = 0; component_indices = NULL; }
    ~kdcx_layer_member()
      { if (component_indices != NULL) delete[] component_indices; }
  public: // Data
    int codestream_idx;
    kdu_coords reg_subsampling;
    kdu_coords reg_offset;
    int num_components;
    int *component_indices;
  };

/*****************************************************************************/
/*                           kdcx_layer_mapping                              */
/*****************************************************************************/

struct kdcx_layer_mapping {
  public: // Member functions
    kdcx_layer_mapping()
      {
        is_complete = false;
        num_members = num_channels = num_colours = 0;
        have_opct_box = have_opacity_channel = false;
        layer_idx = -1; members=NULL;  channel_indices=NULL;
      }
    kdcx_layer_mapping(int layer_idx)
      {
        is_complete = false;
        num_members = num_channels = num_colours = 0;
        have_opct_box = have_opacity_channel = false;
        this->layer_idx = layer_idx; members=NULL; channel_indices=NULL;
      }
    ~kdcx_layer_mapping()
      {
        if (members != NULL) delete[] members;
        if (channel_indices != NULL) delete[] channel_indices;
      }
    void parse_cdef_box(jp2_input_box *box);
      /* Parses to the end of the channel definition box leaving it open, using
         the information to set the `num_channels' and `channel_indices'
         members. */
    void parse_creg_box(jp2_input_box *box);
      /* Parses to the end of the codestream registration box, leaving it open,
         using the information to set the `num_members' and `members'
         members. */
    void parse_colr_box(jp2_input_box *box);
      /* Parses to the end of the colour description box leaving it open,
         using the information to set the `num_colours' member if possible.
         Once any colour description box is encountered, the value of
         `num_colours' is set to a non-zero value -- either the actual
         number of colours (if the box can be parsed) or -1 (if the box
         cannot be parsed). */
    void parse_opct_box(jp2_input_box *box);
      /* Parses the opacity box, leaving it open.  Sets `have_opct_box' to
         true.  Also sets `have_opacity_channel' to true if the box has
         an OTyp field equal to 0 or 1.  In this case, there will be no
         channel definition box, but `num_colours' must be augmented by 1
         to get the number of channels used. */
    void finish(kdcx_layer_mapping *defaults, kdu_clientx *owner);
      /* Fills in uninitialized members with appropriate defaults and
         determines the `kdcx_layer_member::num_components' values and
         `kdcx_layer_member::component_indices' arrays, based on the
         available information. */
  public: // Data
    bool is_complete; // True once all boxes parsed and `finish' called
    jp2_input_box jplh_box; // Used to keep track of an incomplete jplh box
    jp2_input_box jplh_sub; // Used to keep track of an incomplete sub-box
    jp2_input_box cgrp_box; // Used to keep track of an incomplete cgrp box
    jp2_input_box cgrp_sub; // Used to keep track of an incomplete colr box
    int layer_idx;
    kdu_coords layer_size;
    kdu_coords reg_precision;
    int num_members;
    kdcx_layer_member *members;
    bool have_opct_box; // If opacity box was found in the layer header
    bool have_opacity_channel; // If true, `num_channels'=1+`num_colours'
    int num_colours; // Used when `num_channels' is not known.
    int num_channels;
    int *channel_indices; // Sorted into increasing order
  };

/*****************************************************************************/
/*                          kdcx_comp_instruction                            */
/*****************************************************************************/

struct kdcx_comp_instruction {
  public: // Data
    kdu_dims source_dims; // After cropping
    kdu_dims target_dims; // On final rendering grid
  };
  /* Notes:
       If `source_dims' has zero area, the source JPX compositing layer is to
     be used without any cropping.  Otherwise, it is cropped by
     `source_dims.pos' from the upper left hand corner, and to a size of
     `source_dims.size' on the compositing layer registration grid.
       If `target_dims' has zero area, the compositing layer is to be
     placed on the rendering canvas without scaling.  The size is derived
     either from `source_dims.size' or, if `source_dims' has zero area, from
     the original compositing layer dimensions. */

/*****************************************************************************/
/*                            kdcx_comp_mapping                              */
/*****************************************************************************/

struct kdcx_comp_mapping {
  public: // Member functions
    kdcx_comp_mapping()
      {
        is_complete = false;
        num_comp_sets = max_comp_sets = 0;
        comp_set_starts = NULL;
        num_comp_instructions = max_comp_instructions = 0;
        comp_instructions = NULL;
      }
    ~kdcx_comp_mapping()
      {
        if (comp_set_starts != NULL) delete[] comp_set_starts;
        if (comp_instructions != NULL) delete[] comp_instructions;
      }
    void parse_copt_box(jp2_input_box *box);
      /* Parses to the end of the composition options box, leaving it open,
         and using the contents to set the `composited_size' member. */
    void parse_iset_box(jp2_input_box *box);
      /* Parses to the end of the composition instruction box, leaving it open,
         and using the contents to add a new composition instruction set to
         the `comp_set_starts' array and to parse its instructions into the
         `comp_instructions' array, adjusting the relevant counters. */
  public: // Data
    bool is_complete;
    jp2_input_box comp_box; // Used to keep track of an incomplete comp box
    jp2_input_box comp_sub; // Used to keep track of an incomplete sub-box
    kdu_coords composited_size; // Valid only if `num_comp_sets' > 0
    int num_comp_sets; // Num valid elts in `comp_set_starts' array
    int max_comp_sets; // Max elts in `comp_set_starts' array
    int *comp_set_starts; // Index of first instruction in each comp set
    int num_comp_instructions; //  Num valid elts in `comp_instructions' array
    int max_comp_instructions; // Max elts in `comp_instructions' array
    kdcx_comp_instruction *comp_instructions; // Array of all instructions
  };

#endif // CLIENTX_LOCAL_H

