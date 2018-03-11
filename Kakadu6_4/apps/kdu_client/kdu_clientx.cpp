/*****************************************************************************/
// File: kdu_clientx.cpp [scope = APPS/KDU_CLIENT]
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
  Implements the `kdu_clientx' client context translator object
******************************************************************************/

#include <assert.h>
#include <math.h>
#include "kdu_utils.h"
#include "clientx_local.h"
#include "jp2_shared.h"

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
     kdu_error _name("E(kdu_clientx.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(kdu_clientx.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu Client:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu Client:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


/* ========================================================================= */
/*                             kdcx_stream_mapping                           */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdcx_stream_mapping::parse_ihdr_box                  */
/*****************************************************************************/

void
  kdcx_stream_mapping::parse_ihdr_box(jp2_input_box *ihdr)
{
  if (num_components > 0)
    return; // Already parsed one
  kdu_uint32 height, width;
  kdu_uint16 nc;
  if (ihdr->read(height) && ihdr->read(width) && ihdr->read(nc))
    {
      num_components = (int) nc;
      size.x = (int) width;
      size.y = (int) height;
    }
}

/*****************************************************************************/
/*                      kdcx_stream_mapping::parse_cmap_box                  */
/*****************************************************************************/

void
  kdcx_stream_mapping::parse_cmap_box(jp2_input_box *cmap)
{
  if (component_indices != NULL)
    return; // Already parsed one
  num_channels = ((int) cmap->get_remaining_bytes()) >> 2;
  component_indices = new int[num_channels];
  for (int n=0; n < num_channels; n++)
    {
      kdu_uint16 cmp;
      kdu_byte mtyp, pcol;
      if (!(cmap->read(cmp) && cmap->read(mtyp) && cmap->read(pcol)))
        component_indices[n] = 0; // Easy way to defer error condition
      else
        component_indices[n] = (int) cmp;
    }
}

/*****************************************************************************/
/*                          kdcx_stream_mapping::finish                      */
/*****************************************************************************/

void
  kdcx_stream_mapping::finish(kdcx_stream_mapping *defaults)
{
  if (jpch_box.exists())
    return; // Can't finish yet
  if (num_components == 0)
    {
      num_components = defaults->num_components;
      size = defaults->size;
    }
  if (component_indices == NULL)
    {
      if (defaults->component_indices != NULL)
        { // Copy defaults
          num_channels = defaults->num_channels;
          component_indices = new int[num_channels];
          for (int n=0; n < num_channels; n++)
            component_indices[n] = defaults->component_indices[n];
        }
      else
        { // Components in 1-1 correspondence with channels
          num_channels = num_components;
          component_indices = new int[num_channels];
          for (int n=0; n < num_channels; n++)
            component_indices[n] = n;
        }
    }
  is_complete = true;
}


/* ========================================================================= */
/*                             kdcx_layer_mapping                            */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdcx_layer_mapping::parse_cdef_box                   */
/*****************************************************************************/

void
  kdcx_layer_mapping::parse_cdef_box(jp2_input_box *cdef)
{
  if (channel_indices != NULL)
    return; // Already parsed one
  kdu_uint16 num_descriptions;
  if (!cdef->read(num_descriptions))
    return;
  num_channels = (int) num_descriptions;
  channel_indices = new int[num_channels];

  int n, k;
  for (n=0; n < num_channels; n++)
    {
      kdu_uint16 channel_idx, typ, assoc;
      if (!(cdef->read(channel_idx) && cdef->read(typ) && cdef->read(assoc)))
        channel_indices[n] = 0; // Easiest way to defer error condition
      else
        channel_indices[n] = (int) channel_idx;
    }

  // Sort the channel indices into increasing order
  bool done = false;
  while (!done)
    { // Use simple bubble sort
      done = true;
      for (n=1; n < num_channels; n++)
        if (channel_indices[n] < channel_indices[n-1])
          { // Swap
            done = false;
            k = channel_indices[n];
            channel_indices[n] = channel_indices[n-1];
            channel_indices[n-1] = k;
          }
    }

  // Remove redundant elements
  for (n=1; n < num_channels; n++)
    if (channel_indices[n] == channel_indices[n-1])
      { // Found redundant channel
        num_channels--;
        for (k=n; k < num_channels; k++)
          channel_indices[k] = channel_indices[k+1];
      }
}

/*****************************************************************************/
/*                      kdcx_layer_mapping::parse_creg_box                   */
/*****************************************************************************/

void
  kdcx_layer_mapping::parse_creg_box(jp2_input_box *creg)
{
  if (members != NULL)
    return;
  kdu_uint16 xs, ys;
  if (!(creg->read(xs) && creg->read(ys) && (xs > 0) && (ys > 0)))
    { KDU_ERROR(e,0); e <<
        KDU_TXT("Malformed codestream registration box encountered "
        "in JPX compositing layer header box.");
    }
  reg_precision.x = (int) xs;
  reg_precision.y = (int) ys;
  num_members = ((int) creg->get_remaining_bytes()) / 6;
  members = new kdcx_layer_member[num_members];
  for (int n=0; n < num_members; n++)
    {
      kdcx_layer_member *mem = members + n;
      kdu_uint16 cdn;
      kdu_byte xr, yr, xo, yo;
      if (!(creg->read(cdn) && creg->read(xr) && creg->read(yr) &&
            creg->read(xo) && creg->read(yo) && (xr > 0) && (yr > 0)))
        { KDU_ERROR(e,1); e <<
            KDU_TXT("Malformed codestream registration box "
            "encountered in JPX compositing layer header box.");
        }
      mem->codestream_idx = (int) cdn;
      mem->reg_offset.x = (int) xo;
      mem->reg_offset.y = (int) yo;
      mem->reg_subsampling.x = (int) xr;
      mem->reg_subsampling.y = (int) yr;
    }
}

/*****************************************************************************/
/*                      kdcx_layer_mapping::parse_opct_box                   */
/*****************************************************************************/

void
  kdcx_layer_mapping::parse_opct_box(jp2_input_box *opct)
{
  have_opct_box = true;
  kdu_byte otyp;
  if (!opct->read(otyp))
    return;
  if (otyp < 2)
    have_opacity_channel = true;
}

/*****************************************************************************/
/*                      kdcx_layer_mapping::parse_colr_box                   */
/*****************************************************************************/

void
  kdcx_layer_mapping::parse_colr_box(jp2_input_box *colr)
{
  if (num_colours > 0)
    return; // Already have a value.
  assert(colr->get_box_type() == jp2_colour_4cc);
  kdu_byte meth, prec_val, approx;
  if (!(colr->read(meth) && colr->read(prec_val) && colr->read(approx) &&
        (approx <= 4) && (meth >= 1) && (meth <= 4)))
    return; // Malformed colour description box.
  kdu_uint32 enum_cs;
  if ((meth == 1) && colr->read(enum_cs))
    switch (enum_cs) {
      case 0:  num_colours=1; break; // JP2_bilevel1_SPACE
      case 1:  num_colours=3; break; // space=JP2_YCbCr1_SPACE
      case 3:  num_colours=3; break; // space=JP2_YCbCr2_SPACE
      case 4:  num_colours=3; break; // space=JP2_YCbCr3_SPACE
      case 9:  num_colours=3; break; // space=JP2_PhotoYCC_SPACE
      case 11: num_colours=3; break; // space=JP2_CMY_SPACE
      case 12: num_colours=4; break; // space=JP2_CMYK_SPACE
      case 13: num_colours=4; break; // space=JP2_YCCK_SPACE
      case 14: num_colours=3; break; // space=JP2_CIELab_SPACE
      case 15: num_colours=1; break; // space=JP2_bilevel2_SPACE
      case 16: num_colours=3; break; // space=JP2_sRGB_SPACE
      case 17: num_colours=1; break; // space=JP2_sLUM_SPACE
      case 18: num_colours=3; break; // space=JP2_sYCC_SPACE
      case 19: num_colours=3; break; // space=JP2_CIEJab_SPACE
      case 20: num_colours=3; break; // space=JP2_esRGB_SPACE
      case 21: num_colours=3; break; // space=JP2_ROMMRGB_SPACE
      case 22: num_colours=3; break; // space=JP2_YPbPr60_SPACE
      case 23: num_colours=3; break; // space=JP2_YPbPr50_SPACE
      case 24: num_colours=3; break; // space=JP2_esYCC_SPACE
      default: // Unrecognized colour space.
        return;
      }
}

/*****************************************************************************/
/*                        kdcx_layer_mapping::finish                         */
/*****************************************************************************/

void
  kdcx_layer_mapping::finish(kdcx_layer_mapping *defaults, kdu_clientx *owner)
{
  if (jplh_box.exists())
    return; // Can't finish yet
  if (members == NULL)
    { // One codestream whose index is equal to that of the compositing layer
      assert(layer_idx >= 0);
      num_members = 1;
      members = new kdcx_layer_member[1];
      members->codestream_idx = layer_idx;
      reg_precision = kdu_coords(1,1);
      members->reg_subsampling = reg_precision;
    }

  layer_size = kdu_coords(0,0); // Find layer size by intersecting its members
  int n, m, total_channels=0;
  for (m=0; m < num_members; m++)
    {
      kdcx_layer_member *mem = members + m;
      if (mem->codestream_idx >= owner->num_codestreams)
        { KDU_ERROR(e,2); e <<
            KDU_TXT("Invalid codestream registration box found in "
            "compositing layer header box: attempting to use a non "
            "existent codestream.");
        }
      kdcx_stream_mapping *stream = owner->stream_refs[mem->codestream_idx];
      kdu_coords stream_size = stream->size;
      stream_size.x *= mem->reg_subsampling.x;
      stream_size.y *= mem->reg_subsampling.y;
      stream_size += mem->reg_offset;
      if ((m == 0) || (stream_size.x < layer_size.x))
        layer_size.x = stream_size.x;
      if ((m == 0) || (stream_size.y < layer_size.y))
        layer_size.y = stream_size.y;
      total_channels += stream->num_channels;
    }
  layer_size.x = ceil_ratio(layer_size.x,reg_precision.x);
  layer_size.y = ceil_ratio(layer_size.y,reg_precision.y);

  if (num_colours == 0)
    num_colours = defaults->num_colours; // Inherit any colour box info
  if (num_colours <= 0)
    { // No information about number of colours could be deduced; let's have
      // a reasonably conservative guess
      num_colours = total_channels;
      if (num_colours > 3)
        num_colours = 3;
      if (num_colours < 2)
        num_colours = 1;
    }

  if (channel_indices == NULL)
    {
      if ((defaults->channel_indices != NULL) && !have_opct_box)
        { // Copy default cdef info
          num_channels = defaults->num_channels;
          channel_indices = new int[num_channels];
          for (n=0; n < num_channels; n++)
            channel_indices[n] = defaults->channel_indices[n];
        }
      else
        { // Must be using initial set of `num_colours' channels
          num_channels = num_colours + ((have_opacity_channel)?1:0);
          if (num_channels > total_channels)
            num_channels = total_channels;
          channel_indices = new int[num_channels];
          for (n=0; n < num_channels; n++)
            channel_indices[n] = n;
        }
    }

  // Determine the image components in each codestream member
  total_channels = 0;
  int chan = 0; // Indexes first unused entry in `channel_indices' array
  for (m=0; m < num_members; m++)
    {
      kdcx_layer_member *mem = members + m;
      kdcx_stream_mapping *stream = owner->stream_refs[mem->codestream_idx];
      assert((mem->num_components == 0) && (mem->component_indices == NULL));
      if (chan == num_channels)
        break; // No more channels, and hence no more member components
      mem->component_indices = new int[stream->num_channels]; // Max value
      for (n=0; n < stream->num_channels; n++, total_channels++)
        {
          if (chan == num_channels)
            break;
          if (channel_indices[chan] == total_channels)
            { // Add corresponding image component
              mem->component_indices[mem->num_components++] =
                stream->component_indices[n];
              chan++;
            }
          else
            assert(channel_indices[chan] > total_channels);
        }
    }

  is_complete = true;
}


/* ========================================================================= */
/*                              kdcx_comp_mapping                            */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdcx_comp_mapping::parse_copt_box                    */
/*****************************************************************************/

void
  kdcx_comp_mapping::parse_copt_box(jp2_input_box *box)
{
  composited_size = kdu_coords(0,0);
  kdu_uint32 height, width;
  if (!(box->read(height) && box->read(width)))
    return; // Leaves `composited_size' 0, so rest of comp box is ignored
  composited_size.x = (int) width;
  composited_size.y = (int) height;
}

/*****************************************************************************/
/*                      kdcx_comp_mapping::parse_iset_box                    */
/*****************************************************************************/

void
  kdcx_comp_mapping::parse_iset_box(jp2_input_box *box)
{
  int n;
  if ((composited_size.x <= 0) || (composited_size.y <= 0))
    return; // Failed to parse a valid `copt' box previously

  kdu_uint16 flags, rept;
  kdu_uint32 tick;
  if (!(box->read(flags) && box->read(rept) && box->read(tick)))
    { KDU_ERROR(e,3); e <<
        KDU_TXT("Malformed Instruction Set box found in JPX data source.");
    }

  if (num_comp_sets == max_comp_sets)
    { // Augment the array
      int new_max_sets = max_comp_sets + num_comp_sets + 8;
      int *tmp_starts = new int[new_max_sets];
      for (n=0; n < num_comp_sets; n++)
        tmp_starts[n] = comp_set_starts[n];
      if (comp_set_starts != NULL)
        delete[] comp_set_starts;
      comp_set_starts = tmp_starts;
      max_comp_sets = new_max_sets;
    }
  comp_set_starts[num_comp_sets++] = num_comp_instructions;
  
  kdu_dims source_dims, target_dims;
  bool have_target_pos = ((flags & 1) != 0);
  bool have_target_size = ((flags & 2) != 0);
  bool have_life_persist = ((flags & 4) != 0);
  bool have_source_region = ((flags & 32) != 0);
  if (!(have_target_pos || have_target_size ||
        have_life_persist || have_source_region))
    return; // No new instructions in this set

  while (1)
    {
      if (have_target_pos)
        {
          kdu_uint32 X0, Y0;
          if (!(box->read(X0) && box->read(Y0)))
            return; // No more instructions
          target_dims.pos.x = (int) X0;
          target_dims.pos.y = (int) Y0;
        }
      else
        target_dims.pos = kdu_coords(0,0);

      if (have_target_size)
        {
          kdu_uint32 XS, YS;
          if (!(box->read(XS) && box->read(YS)))
            {
              if (!have_target_pos)
                return; // No more instructions
              KDU_ERROR(e,4); e <<
                KDU_TXT("Malformed Instruction Set box found in JPX data "
                "source.");
            }
          target_dims.size.x = (int) XS;
          target_dims.size.y = (int) YS;
        }
      else
        target_dims.size = kdu_coords(0,0);

      if (have_life_persist)
        {
          kdu_uint32 life, next_reuse;
          if (!(box->read(life) && box->read(next_reuse)))
            {
              if (!(have_target_pos || have_target_size))
                return; // No more instructions
              KDU_ERROR(e,5); e <<
                KDU_TXT("Malformed Instruction Set box found in JPX data "
                "source.");
            }
        }
      
      if (have_source_region)
        {
          kdu_uint32 XC, YC, WC, HC;
          if (!(box->read(XC) && box->read(YC) &&
                box->read(WC) && box->read(HC)))
            {
              if (!(have_life_persist || have_target_pos || have_target_size))
                return; // No more instructions
              KDU_ERROR(e,6); e <<
                KDU_TXT("Malformed Instruction Set box found in JPX "
                "data source.");
            }
          source_dims.pos.x = (int) XC;
          source_dims.pos.y = (int) YC;
          source_dims.size.x = (int) WC;
          source_dims.size.y = (int) HC;
        }
      else
        source_dims.size = source_dims.pos = kdu_coords(0,0);

      if (num_comp_instructions == max_comp_instructions)
        { // Augment array
          int new_max_insts = max_comp_instructions + num_comp_instructions+8;
          kdcx_comp_instruction *tmp_inst =
            new kdcx_comp_instruction[new_max_insts];
          for (n=0; n < num_comp_instructions; n++)
            tmp_inst[n] = comp_instructions[n];
          if (comp_instructions != NULL)
            delete[] comp_instructions;
          comp_instructions = tmp_inst;
          max_comp_instructions = new_max_insts;
        }
      comp_instructions[num_comp_instructions].source_dims = source_dims;
      comp_instructions[num_comp_instructions].target_dims = target_dims;
      num_comp_instructions++;
    }
}


/* ========================================================================= */
/*                                 kdu_clientx                               */
/* ========================================================================= */

/*****************************************************************************/
/*                           kdu_clientx::kdu_clientx                        */
/*****************************************************************************/

kdu_clientx::kdu_clientx()
{
  started = false;
  not_compatible = false;
  is_jp2 = false;
  top_level_complete = false;
  defaults_complete = false;
  stream_defaults = NULL;
  layer_defaults = NULL;
  composition = NULL;
  num_codestreams = max_codestreams = num_jp2c_or_frag = num_jpch = 0;
  stream_refs = NULL;
  num_compositing_layers = max_compositing_layers = num_jplh = 0;
  layer_refs = NULL;
}

/*****************************************************************************/
/*                          kdu_clientx::~kdu_clientx                        */
/*****************************************************************************/

kdu_clientx::~kdu_clientx()
{
  close();
}

/*****************************************************************************/
/*                              kdu_clientx::close                           */
/*****************************************************************************/

void
  kdu_clientx::close()
{
  int n;
  for (n=0; n < num_codestreams; n++)
    delete stream_refs[n];
  if (stream_refs != NULL)
    delete[] stream_refs;
  for (n=0; n < num_compositing_layers; n++)
    delete layer_refs[n];
  if (layer_refs != NULL)
    delete[] layer_refs;
  if (stream_defaults != NULL)
    delete stream_defaults;
  if (layer_defaults != NULL)
    delete layer_defaults;
  if (composition != NULL)
    delete composition;

  jp2h_sub.close();
  top_box.close();
  src.close();
  started = false;
  not_compatible = false;
  is_jp2 = false;
  top_level_complete = false;
  defaults_complete = false;
  stream_defaults = NULL;
  layer_defaults = NULL;
  composition = NULL;
  num_codestreams = max_codestreams = num_jp2c_or_frag = num_jpch = 0;
  stream_refs = NULL;
  num_compositing_layers = max_compositing_layers = num_jplh = 0;
  layer_refs = NULL;
}

/*****************************************************************************/
/*                             kdu_clientx::update                           */
/*****************************************************************************/

bool
  kdu_clientx::update()
{
  if (not_compatible)
    return false;

  bool any_change = false;
  if (!started)
    { // Opening for the first time
      if (!src.exists())
        src.open(&aux_cache);
      if (!top_box.open(&src))
        return false; // Need to come back later
      if (top_box.get_box_type() != jp2_signature_4cc)
        { not_compatible = true; return false; }
      top_box.close();
      if (stream_defaults == NULL)
        stream_defaults = new kdcx_stream_mapping;
      if (layer_defaults == NULL)
        layer_defaults = new kdcx_layer_mapping;
      if (composition == NULL)
        composition = new kdcx_comp_mapping;
      started = true;
    }
  
  // Parse as many top-level boxes as possible
  while (!top_level_complete)
    { 
      if ((!top_box) && src.is_top_level_complete() && !top_box.open_next())
        top_level_complete = defaults_complete = true;
      if (!top_box)
        break; // Have to come back and continue parsing later
      kdu_uint32 box_type = top_box.get_box_type();
      if ((box_type == jp2_codestream_4cc) ||
          (box_type == jp2_fragment_table_4cc))
        { // Need to be able to skip over incomplete codestream boxes
          add_stream(num_jp2c_or_frag++);
          top_box.close();
          continue;
        }
      if (!top_box.is_complete())
        break; // Have to come back and continue parsing later
      if (box_type == jp2_file_type_4cc)
        {
          kdu_uint32 brand, minor_version, compat;
          top_box.read(brand); top_box.read(minor_version);
          is_jp2 = not_compatible = true; // Until proven otherwise
          while (top_box.read(compat))
            if (compat == jp2_brand)
              not_compatible = false;
            else if ((compat == jpx_brand) || (compat == jpx_baseline_brand))
              not_compatible = is_jp2 = false;
          top_box.close();
          if (not_compatible)
            { is_jp2 = false; return false; }
          any_change = true;
        }
      else if (box_type == jp2_header_4cc)
        {
          while (jp2h_sub.exists() || jp2h_sub.open(&top_box))
            {
              if (!jp2h_sub.is_complete())
                return any_change; // Don't parse any further until header done
              if (jp2h_sub.get_box_type() == jp2_image_header_4cc)
                stream_defaults->parse_ihdr_box(&jp2h_sub);
              else if (jp2h_sub.get_box_type() == jp2_component_mapping_4cc)
                stream_defaults->parse_cmap_box(&jp2h_sub);
              else if (jp2h_sub.get_box_type() == jp2_channel_definition_4cc)
                layer_defaults->parse_cdef_box(&jp2h_sub);
              else if (jp2h_sub.get_box_type() == jp2_colour_4cc)
                layer_defaults->parse_colr_box(&jp2h_sub);
              jp2h_sub.close();
            }
          top_box.close();
          defaults_complete = true;
          any_change = true;
        }
      else if ((box_type == jp2_composition_4cc) &&
               (!composition->is_complete) && !composition->comp_box)
        {
          composition->comp_box.transplant(top_box);
          assert(!top_box);
        }
      else if (box_type == jp2_codestream_header_4cc)
        {
          kdcx_stream_mapping *str = add_stream(num_jpch++);
          str->jpch_box.transplant(top_box);
          assert(!top_box);
        }
      else if (box_type == jp2_compositing_layer_hdr_4cc)
        {
          kdcx_layer_mapping *lyr = add_layer(num_jplh++);
          lyr->jplh_box.transplant(top_box);
          assert(!top_box);
        }
      else 
        top_box.close();
    }

  // See if we can parse further into a composition box
  if (composition->comp_box.exists() && composition->comp_box.is_complete())
    {
      while (composition->comp_sub.exists() ||
             composition->comp_sub.open(&composition->comp_box))
        {
          if (!composition->comp_sub.is_complete())
            break;
          if (composition->comp_sub.get_box_type() ==
              jp2_comp_options_4cc)
            composition->parse_copt_box(&composition->comp_sub);
          else if (composition->comp_sub.get_box_type() ==
                   jp2_comp_instruction_set_4cc)
            composition->parse_iset_box(&composition->comp_sub);
          composition->comp_sub.close();
        }
      if (!composition->comp_sub)
        {
          composition->is_complete = true;
          any_change = true;
          composition->comp_box.close();
        }
    }

  // See if we can complete any codestream header box
  if (defaults_complete)
    {
      for (int n=0; n < num_codestreams; n++)
        {
          kdcx_stream_mapping *str = stream_refs[n];
          if (str->is_complete)
            continue;
          if (str->jpch_box.exists())
            {
              if (!str->jpch_box.is_complete())
                continue; // Can't finish this one yet
              while (str->jpch_sub.exists() ||
                     str->jpch_sub.open(&str->jpch_box))
                {
                  if (!str->jpch_sub.is_complete())
                    break;
                  if (str->jpch_sub.get_box_type() == jp2_image_header_4cc)
                    str->parse_ihdr_box(&str->jpch_sub);
                  else if (str->jpch_sub.get_box_type() ==
                           jp2_component_mapping_4cc)
                    str->parse_cmap_box(&str->jpch_sub);
                  str->jpch_sub.close();
                }
              if (!str->jpch_sub)
                {
                  str->jpch_box.close();
                  str->finish(stream_defaults);
                }
            }
          else if (top_level_complete)
            str->finish(stream_defaults);
          if (str->is_complete)
            any_change = true;
        }
    }

  // See if we can complete any compositing layer header box
  if (defaults_complete)
    {
      for (int n=0; n < num_compositing_layers; n++)
        {
          kdcx_layer_mapping *lyr = layer_refs[n];
          if (lyr->is_complete)
            continue;
          if (lyr->jplh_box.exists())
            {
              if (!lyr->jplh_box.is_complete())
                continue; // Can't finish this one yet
              while (lyr->jplh_sub.exists() ||
                     lyr->jplh_sub.open(&lyr->jplh_box))
                {
                  if (!lyr->jplh_sub.is_complete())
                    break;
                  if (lyr->jplh_sub.get_box_type() ==
                      jp2_channel_definition_4cc)
                    lyr->parse_cdef_box(&lyr->jplh_sub);
                  else if (lyr->jplh_sub.get_box_type() ==
                           jp2_registration_4cc)
                    lyr->parse_creg_box(&lyr->jplh_sub);
                  else if (lyr->jplh_sub.get_box_type() ==
                           jp2_opacity_4cc)
                    lyr->parse_opct_box(&lyr->jplh_sub);
                  else if ((lyr->jplh_sub.get_box_type() ==
                            jp2_colour_group_4cc) && !lyr->cgrp_box)
                    {
                      lyr->cgrp_box.transplant(lyr->jplh_sub);
                      assert(!lyr->jplh_sub);
                    }
                  lyr->jplh_sub.close();
                }
              if (!lyr->jplh_sub)
                {
                  lyr->jplh_box.close();
                  if (!lyr->cgrp_box)
                    lyr->finish(layer_defaults,this);
                }
            }

          if (lyr->cgrp_box.exists())
            {
              if (!lyr->cgrp_box.is_complete())
                continue; // Can't finish yet
              while (lyr->cgrp_sub.exists() ||
                     lyr->cgrp_sub.open(&lyr->cgrp_box))
                {
                  if (!lyr->cgrp_sub.is_complete())
                    break;
                  if (lyr->cgrp_sub.get_box_type() == jp2_colour_4cc)
                    lyr->parse_colr_box(&lyr->cgrp_sub);
                  lyr->cgrp_sub.close();
                }
              if (!lyr->cgrp_sub)
                {
                  lyr->cgrp_box.close();
                  if (!lyr->jplh_box)
                    lyr->finish(layer_defaults,this);
                }
            }

          if ((!lyr->jplh_box) && (!lyr->cgrp_box) && (!lyr->is_complete) &&
              top_level_complete)
            lyr->finish(layer_defaults,this);
          if (lyr->is_complete)
            any_change = true;
        }
    }

  return any_change;
}

/*****************************************************************************/
/*                    kdu_clientx::get_num_context_members                    */
/*****************************************************************************/

int
  kdu_clientx::get_num_context_members(int context_type, int context_idx,
                                       int remapping_ids[])
{
  if ((context_type != KDU_JPIP_CONTEXT_JPXL) || (context_idx < 0) ||
      (context_idx >= num_compositing_layers))
    return 0;
  kdcx_layer_mapping *layer = layer_refs[context_idx];
  if ((remapping_ids[0] >= 0) || (remapping_ids[1] >= 0))
    { // See if the instruction reference is valid
      int iset = remapping_ids[0];
      int inum = remapping_ids[1];
      if ((iset < 0) || (iset >= composition->num_comp_sets) || (inum < 0) ||
          ((inum + composition->comp_set_starts[iset]) >=
           composition->num_comp_instructions) ||
          (composition->composited_size.x < 1) ||
          (composition->composited_size.y < 1))
        remapping_ids[0] = remapping_ids[1] = -1; // Invalid instruction set
    }
  return layer->num_members;
}

/*****************************************************************************/
/*                   kdu_clientx::get_context_codestream                     */
/*****************************************************************************/

int
  kdu_clientx::get_context_codestream(int context_type, int context_idx,
                                      int remapping_ids[], int member_idx)
{
  if ((context_type != KDU_JPIP_CONTEXT_JPXL) || (context_idx < 0) ||
      (context_idx >= num_compositing_layers))
    return -1;
  kdcx_layer_mapping *layer = layer_refs[context_idx];
  if ((member_idx < 0) || (member_idx >= layer->num_members))
    return -1;
  return layer->members[member_idx].codestream_idx;
}

/*****************************************************************************/
/*                   kdu_clientx::get_context_components                     */
/*****************************************************************************/

const int *
  kdu_clientx::get_context_components(int context_type, int context_idx,
                                      int remapping_ids[], int member_idx,
                                      int &num_components)
{
  num_components = 0;
  if ((context_type != KDU_JPIP_CONTEXT_JPXL) || (context_idx < 0) ||
      (context_idx >= num_compositing_layers))
    return NULL;
  kdcx_layer_mapping *layer = layer_refs[context_idx];
  if ((member_idx < 0) || (member_idx >= layer->num_members))
    return NULL;
  num_components = layer->members[member_idx].num_components;
  return layer->members[member_idx].component_indices;
}

/*****************************************************************************/
/*                  kdu_clientx::perform_context_remapping                   */
/*****************************************************************************/

bool
  kdu_clientx::perform_context_remapping(int context_type, int context_idx,
                                         int remapping_ids[], int member_idx,
                                         kdu_coords &resolution,
                                         kdu_dims &region)
{
  if ((context_type != KDU_JPIP_CONTEXT_JPXL) || (context_idx < 0) ||
      (context_idx >= num_compositing_layers))
    return false;
  kdcx_layer_mapping *layer = layer_refs[context_idx];
  if ((member_idx < 0) || (member_idx >= layer->num_members))
    return false;
  kdcx_layer_member *member = layer->members + member_idx;
  kdcx_stream_mapping *stream = stream_refs[member->codestream_idx];

  if ((layer->layer_size.x <= 0) || (layer->layer_size.y <= 0))
    { region.size = region.pos = kdu_coords(0,0); }
  else if ((remapping_ids[0] < 0) || (remapping_ids[1] < 0) ||
           (composition->num_comp_instructions < 1) ||
           (composition->composited_size.x <= 0) ||
           (composition->composited_size.y <= 0))
    { // View window expressed relative to compositing layer registration grid
      region.pos.x -= (int)
        ceil((double) member->reg_offset.x * (double) resolution.x /
             ((double) layer->reg_precision.x * (double) layer->layer_size.x));
      resolution.x = (int) 
        ceil(((double) resolution.x) *
             ((double) stream->size.x * (double) member->reg_subsampling.x) /
             ((double) layer->reg_precision.x * (double) layer->layer_size.x));
      region.pos.y -= (int)
        ceil((double) member->reg_offset.y * (double) resolution.y /
             ((double) layer->reg_precision.y * (double) layer->layer_size.y));
      resolution.y = (int) 
        ceil(((double) resolution.y) *
             ((double) stream->size.y * (double) member->reg_subsampling.y) /
             ((double) layer->reg_precision.y * (double) layer->layer_size.y));
    }
  else
    { // View window expressed relative to composition grid
      int iset = remapping_ids[0];
      int inum = remapping_ids[1];
      if ((iset >= 0) && (iset < composition->num_comp_sets))
        inum += composition->comp_set_starts[iset];
      if (inum < 0)
        inum = 0; // Just in case
      if (inum >= composition->num_comp_instructions)
        inum = composition->num_comp_instructions-1;
      kdcx_comp_instruction *inst = composition->comp_instructions + inum;
      kdu_coords comp_size = composition->composited_size;

      kdu_dims source_dims = inst->source_dims;
      kdu_dims target_dims = inst->target_dims;
      if (!source_dims)
        source_dims.size = layer->layer_size;
      if (!target_dims)
        target_dims.size = source_dims.size;

      double alpha, beta;
      kdu_coords region_lim = region.pos+region.size;
      int min_val, lim_val;

      alpha = ((double) resolution.x) / ((double) comp_size.x);
      beta =  ((double) target_dims.size.x) / ((double) source_dims.size.x);
      min_val = (int) floor(alpha * target_dims.pos.x);
      lim_val = (int) ceil(alpha * (target_dims.pos.x + target_dims.size.x));
      if (min_val > region.pos.x)
        region.pos.x = min_val;
      if (lim_val < region_lim.x)
        region_lim.x = lim_val;
      region.size.x = region_lim.x - region.pos.x;
      region.pos.x -= (int)
        ceil(alpha * (target_dims.pos.x -
                      beta * (source_dims.pos.x -
                              ((double) member->reg_offset.x) /
                              ((double) layer->reg_precision.x))));
      resolution.x = (int)
        ceil(alpha * beta * ((double) stream->size.x) *
             ((double) member->reg_subsampling.x) /
             ((double) layer->reg_precision.x));

      alpha = ((double) resolution.y) / ((double) comp_size.y);
      beta =  ((double) target_dims.size.y) / ((double) source_dims.size.y);
      min_val = (int) floor(alpha * target_dims.pos.y);
      lim_val = (int) ceil(alpha * (target_dims.pos.y + target_dims.size.y));
      if (min_val > region.pos.y)
        region.pos.y = min_val;
      if (lim_val < region_lim.y)
        region_lim.y = lim_val;
      region.size.y = region_lim.y - region.pos.y;
      region.pos.y -= (int)
        ceil(alpha * (target_dims.pos.y -
                      beta * (source_dims.pos.y -
                              ((double) member->reg_offset.y) /
                              ((double) layer->reg_precision.y))));
      resolution.y = (int)
        ceil(alpha * beta * ((double) stream->size.y) *
             ((double) member->reg_subsampling.y) /
             ((double) layer->reg_precision.y));
    }

  // Adjust region to ensure that it fits inside the codestream resolution
  kdu_coords lim = region.pos + region.size;
  if (region.pos.x < 0)
    region.pos.x = 0;
  if (region.pos.y < 0)
    region.pos.y = 0;
  if (lim.x > resolution.x)
    lim.x = resolution.x;
  if (lim.y > resolution.y)
    lim.y = resolution.y;
  region.size = lim - region.pos;
  if (region.size.x < 0)
    region.size.x = 0;
  if (region.size.y < 0)
    region.size.y = 0;

  return true;
}

/*****************************************************************************/
/*                           kdu_clientx::add_stream                         */
/*****************************************************************************/

kdcx_stream_mapping *
  kdu_clientx::add_stream(int idx)
{
  if (idx >= num_codestreams)
    { // Need to allocate new stream
      if (idx >= max_codestreams)
        { // Need to reallocate references array
          int new_max_streams = max_codestreams + idx + 8;
          kdcx_stream_mapping **refs =
            new kdcx_stream_mapping *[new_max_streams];
          memset(refs,0,sizeof(kdcx_stream_mapping *)*(size_t)new_max_streams);
          for (int n=0; n < num_codestreams; n++)
            refs[n] = stream_refs[n];
          if (stream_refs != NULL)
            delete[] stream_refs;
          stream_refs = refs;
          max_codestreams = new_max_streams;
        }
      while (num_codestreams <= idx)
        stream_refs[num_codestreams++] = new kdcx_stream_mapping;
    }
  return stream_refs[idx];
}

/*****************************************************************************/
/*                           kdu_clientx::add_layer                          */
/*****************************************************************************/

kdcx_layer_mapping *
  kdu_clientx::add_layer(int idx)
{
  if (idx >= num_compositing_layers)
    { // Need to allocate new layer
      if (idx >= max_compositing_layers)
        { // Need to reallocate references array
          int new_max_layers = max_compositing_layers + idx + 8;
          kdcx_layer_mapping **refs = new kdcx_layer_mapping *[new_max_layers];
          memset(refs,0,sizeof(kdcx_layer_mapping *)*(size_t)new_max_layers);
          for (int n=0; n < num_compositing_layers; n++)
            refs[n] = layer_refs[n];
          if (layer_refs != NULL)
            delete[] layer_refs;
          layer_refs = refs;
          max_compositing_layers = new_max_layers;
        }
      while (num_compositing_layers <= idx)
        {
          layer_refs[num_compositing_layers] =
            new kdcx_layer_mapping(num_compositing_layers);
          num_compositing_layers++;
        }
    }
  
  return layer_refs[idx];
}
