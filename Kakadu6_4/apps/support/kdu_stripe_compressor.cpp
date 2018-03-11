/******************************************************************************/
// File: kdu_stripe_compressor.cpp [scope = APPS/SUPPORT]
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
   Implements the `kdu_stripe_compressor' object.
*******************************************************************************/

#include <assert.h>
#include "kdu_messaging.h"
#include "stripe_compressor_local.h"

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
     kdu_error _name("E(kdu_stripe_compressor.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(kdu_stripe_compressor.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu Stripe Compressor:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu Stripe Compressor:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


/* ========================================================================== */
/*                            Internal Functions                              */
/* ========================================================================== */

/******************************************************************************/
/* INLINE                       transfer_bytes                                */
/******************************************************************************/

static inline void
  transfer_bytes(kdu_line_buf &dest, kdu_byte *src, int num_samples,
                 int sample_gap, int src_bits, int original_bits)
{
  if (dest.get_buf16() != NULL)
    {
      kdu_sample16 *dp = dest.get_buf16();
      kdu_int16 off = ((kdu_int16)(1<<src_bits))>>1;
      kdu_int16 mask = ~((kdu_int16)((-1)<<src_bits));
      if (!dest.is_absolute())
        {
          int shift = KDU_FIX_POINT - src_bits; assert(shift >= 0);
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_int16) *src) & mask) - off) << shift;
        }
      else if (src_bits < original_bits)
        { // Reversible processing; source buffer has too few bits
          int shift = original_bits - src_bits;
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_int16) *src) & mask) - off) << shift;
        }
      else if (src_bits > original_bits)
        { // Reversible processing; source buffer has too many bits
          int shift = src_bits - original_bits;
          off -= (1<<shift)>>1; // For rounded down-shifting
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_int16) *src) & mask) - off) >> shift;
        }
      else
        { // Reversible processing, `src_bits'=`original_bits'
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = (((kdu_int16) *src) & mask) - off;
        }
    }
  else
    {
      kdu_sample32 *dp = dest.get_buf32();
      kdu_int32 off = ((kdu_int32)(1<<src_bits))>>1;
      kdu_int32 mask = ~((kdu_int32)((-1)<<src_bits));
      if (!dest.is_absolute())
        {
          float scale = 1.0F / (float)(1<<src_bits);
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->fval = scale * (float)((((kdu_int32) *src) & mask) - off);
        }
      else if (src_bits < original_bits)
        { // Reversible processing; source buffer has too few bits
          int shift = original_bits - src_bits;
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_int32) *src) & mask) - off) << shift;
        }
      else if (src_bits > original_bits)
        { // Reversible processing; source buffer has too many bits
          int shift = src_bits - original_bits;
          off -= (1<<shift)>>1; // For rounded down-shifting
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_int32) *src) & mask) - off) >> shift;
        }
      else
        { // Reversible processing, `src_bits'=`original_bits'
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = (((kdu_int32) *src) & mask) - off;
        }
    }
}

/******************************************************************************/
/* INLINE                       transfer_words                                */
/******************************************************************************/

static inline void
  transfer_words(kdu_line_buf &dest, kdu_int16 *src, int num_samples,
                 int sample_gap, int src_bits, int original_bits,
                 bool is_signed)
{
  if (dest.get_buf16() != NULL)
    {
      kdu_sample16 *dp = dest.get_buf16();
      int upshift = 16-src_bits; assert(upshift >= 0);
      if (!dest.is_absolute())
        {
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = ((*src) << upshift) >> (16-KDU_FIX_POINT);
          else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = (((*src) << upshift) - 0x8000) >> (16-KDU_FIX_POINT);
        }
      else
        { // Reversible processing
          int downshift = 16-original_bits; assert(downshift >= 0);
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = ((*src) << upshift) >> downshift;
          else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = (((*src) << upshift) - 0x8000) >> downshift;
        }
    }
  else
    {
      kdu_sample32 *dp = dest.get_buf32();
      int upshift = 32-src_bits; assert(upshift >= 0);
      if (!dest.is_absolute())
        {
          float scale = 1.0F / (((float)(1<<16)) * ((float)(1<<16)));
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->fval = scale * (float)(((kdu_int32) *src)<<upshift);
          else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->fval = scale * (float)((((kdu_int32) *src)<<upshift)-(1<<31));
        }
      else
        {
          int downshift = 32-original_bits; assert(downshift >= 0);
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = (((kdu_int32) *src)<<upshift) >> downshift;
          else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = ((((kdu_int32) *src)<<upshift)-(1<<31)) >> downshift;
        }
    }
}

/******************************************************************************/
/* INLINE                      transfer_dwords                                */
/******************************************************************************/

static inline void
  transfer_dwords(kdu_line_buf &dest, kdu_int32 *src, int num_samples,
                  int sample_gap, int src_bits, int original_bits,
                  bool is_signed)
{
  if (dest.get_buf16() != NULL)
    {
      kdu_sample16 *dp = dest.get_buf16();
      int upshift = 32-src_bits; assert(upshift >= 0);
      if (!dest.is_absolute())
        {
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = (kdu_int16)
                (((*src) << upshift) >> (32-KDU_FIX_POINT));
          else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = (kdu_int16)
                ((((*src) << upshift)-0x80000000) >> (32-KDU_FIX_POINT));
        }
      else
        { // Reversible processing
          int downshift = 32-original_bits; assert(downshift >= 0);
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = (kdu_int16)
                (((*src) << upshift) >> downshift);
          else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = (kdu_int16)
                ((((*src) << upshift) - 0x80000000) >> downshift);
        }
    }
  else
    {
      kdu_sample32 *dp = dest.get_buf32();
      int upshift = 32-src_bits; assert(upshift >= 0);
      if (!dest.is_absolute())
        {
          float scale = 1.0F / (((float)(1<<16)) * ((float)(1<<16)));
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->fval = scale * (float)((*src)<<upshift);
          else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->fval = scale * (float)(((*src)<<upshift)-(1<<31));
        }
      else
        {
          int downshift = 32-original_bits; assert(downshift >= 0);
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = ((*src)<<upshift) >> downshift;
          else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
              dp->ival = (((*src)<<upshift)-(1<<31)) >> downshift;
        }
    }
}

/******************************************************************************/
/* INLINE                      transfer_floats                                */
/******************************************************************************/

static inline void
  transfer_floats(kdu_line_buf &dest, float *src, int num_samples,
                  int sample_gap, int src_bits, int original_bits,
                  bool is_signed)
{
  float src_scale = 1.0F; // Amount required to scale src to unit dynamic range
  while (src_bits < -16)
    { src_bits += 16;  src_scale *= (float)(1<<16); }
  while (src_bits > 0)
    { src_bits -= 16;  src_scale *= 1.0F / (float)(1<<16); }
  src_scale *= (float)(1<<(-src_bits));

  int dst_bits = (dest.is_absolute())?original_bits:KDU_FIX_POINT;
  float dst_scale = 1.0F; // Amount to scale from unit range to dst
  while (dst_bits > 16)
    { dst_bits -= 16;  dst_scale *= (float)(1<<16); }
  dst_scale *= (float)(1<<dst_bits);

  if (dest.get_buf16() != NULL)
    {
      float scale = dst_scale * src_scale;
      float offset = 0.5F - ((is_signed)?(0.5F*dst_scale):0.0F);
      kdu_sample16 *dp = dest.get_buf16();
      for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
        dp->ival = (kdu_int16)(((*src) * scale) + offset);
    }
  else
    {
      kdu_sample32 *dp = dest.get_buf32();
      if (dest.is_absolute())
        {
          float scale = dst_scale * src_scale;
          float offset = 0.5F - ((is_signed)?(0.5F*dst_scale):0.0F);
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = (kdu_int32)(((*src) * scale) + offset);
        }
      else
        {
          float offset = (is_signed)?(-0.5F):0.0F;
          for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->fval = ((*src) * src_scale) + offset;
        }
    }
}


/* ========================================================================== */
/*                           kdsc_component_state                             */
/* ========================================================================== */

/******************************************************************************/
/*                       kdsc_component_state::update                         */
/******************************************************************************/

void
  kdsc_component_state::update(kdu_coords next_tile_idx,
                               kdu_codestream codestream, bool all_done)
{
  int increment = stripe_height;
  if (increment > remaining_tile_height)
    increment = remaining_tile_height;
  stripe_height -= increment;
  remaining_tile_height -= increment;
  int adj = increment*row_gap;
  if (buf8 != NULL)
    buf8 += adj;
  else if (buf16 != NULL)
    buf16 += adj;
  else if (buf32 != NULL)
    buf32 += adj;
  else if (buf_float != NULL)
    buf_float += adj;
  if ((remaining_tile_height > 0) || all_done)
    return;
  kdu_dims dims; codestream.get_tile_dims(next_tile_idx,comp_idx,dims,true);
  remaining_tile_height = dims.size.y;
}


/* ========================================================================== */
/*                                kdsc_tile                                   */
/* ========================================================================== */

/******************************************************************************/
/*                             kdsc_tile::init                                */
/******************************************************************************/

void
  kdsc_tile::init(kdu_coords idx, kdu_codestream codestream,
                  kdsc_component_state *comp_states, bool force_precise,
                  bool want_fastest, kdu_thread_env *env,
                  kdu_thread_queue *env_queue, int env_dbuf_height)
{
  int c;
  if (!tile.exists())
    {
      tile = codestream.open_tile(idx,env);
      tile.set_components_of_interest(num_components);
      tile_queue = NULL;
      if (env != NULL)
        tile_queue = env->add_queue(NULL,env_queue,"Tile processor");
      bool double_buffering = (env != NULL) && (env_dbuf_height > 0);
      engine.create(codestream,tile,force_precise,NULL,want_fastest,
                    (double_buffering)?env_dbuf_height:1,env,tile_queue,
                    double_buffering);
      for (c=0; c < num_components; c++)
        {
          kdsc_component *comp = components + c;
          kdsc_component_state *cs = comp_states + c;
          comp->size = engine.get_size(c);
          kdu_dims dims;  codestream.get_tile_dims(idx,c,dims,true);
          comp->horizontal_offset = dims.pos.x - cs->pos_x;
          assert((comp->size == dims.size) && (comp->horizontal_offset >= 0));
          comp->ratio_counter = 0;
          comp->stripe_rows_left = 0;
        }
    }

  // Now go through the components, assigning buffers and counters
  for (c=0; c < num_components; c++)
    {
      kdsc_component *comp = components + c;
      kdsc_component_state *cs = comp_states + c;
      assert(comp->stripe_rows_left == 0);
      assert(cs->remaining_tile_height == comp->size.y);
      comp->stripe_rows_left = cs->stripe_height;
      if (comp->stripe_rows_left > comp->size.y)
        comp->stripe_rows_left = comp->size.y;
      comp->sample_gap = cs->sample_gap;
      comp->row_gap = cs->row_gap;
      comp->precision = cs->precision;
      comp->is_signed = cs->is_signed;
      comp->buf8 = cs->buf8;
      comp->buf16 = cs->buf16;
      comp->buf32 = cs->buf32;
      comp->buf_float = cs->buf_float;
      int adj = comp->horizontal_offset * comp->sample_gap;
      if (comp->buf8 != NULL)
        comp->buf8 += adj;
      else if (comp->buf16 != NULL)
        comp->buf16 += adj;
      else if (comp->buf32 != NULL)
        comp->buf32 += adj;
      else if (comp->buf_float != NULL)
        comp->buf_float += adj;
      else
        assert(0);
    }
}

/******************************************************************************/
/*                            kdsc_tile::process                              */
/******************************************************************************/

bool
  kdsc_tile::process(kdu_thread_env *env)
{
  int c;
  bool tile_complete = false;
  bool done = false;
  while (!done)
    {
      done = true;
      tile_complete = true;
      for (c=0; c < num_components; c++)
        {
          kdsc_component *comp = components + c;
          if (comp->size.y > 0)
            tile_complete = false;
          if (comp->stripe_rows_left == 0)
            continue;
          done = false;
          comp->ratio_counter -= comp->count_delta;
          if (comp->ratio_counter >= 0)
            continue;

          comp->size.y--;
          comp->stripe_rows_left--;
          comp->ratio_counter += comp->vert_subsampling;

          kdu_line_buf *line = engine.exchange_line(c,NULL,env);
          assert(line != NULL);
          if (comp->buf8 != NULL)
            {
              transfer_bytes(*line,comp->buf8,comp->size.x,
                             comp->sample_gap,comp->precision,
                             comp->original_precision);
              comp->buf8 += comp->row_gap;
            }
          else if (comp->buf16 != NULL)
            {
              transfer_words(*line,comp->buf16,comp->size.x,
                             comp->sample_gap,comp->precision,
                             comp->original_precision,comp->is_signed);
              comp->buf16 += comp->row_gap;
            }
          else if (comp->buf32 != NULL)
            {
              transfer_dwords(*line,comp->buf32,comp->size.x,
                              comp->sample_gap,comp->precision,
                              comp->original_precision,comp->is_signed);
              comp->buf32 += comp->row_gap;
            }
          else if (comp->buf_float != NULL)
            {
              transfer_floats(*line,comp->buf_float,comp->size.x,
                              comp->sample_gap,comp->precision,
                              comp->original_precision,comp->is_signed);
              comp->buf_float += comp->row_gap;
            }
          else
            assert(0);
          engine.exchange_line(c,line,env);
        }
    }
  if (!tile_complete)
    return false;

  // Clean up
  engine.destroy(env); // Calls `kdu_thread_env::terminate' for us
  tile.close(env);
  tile_queue = NULL;
  tile = kdu_tile(NULL);
  return true;
}


/* ========================================================================== */
/*                           kdu_stripe_compressor                            */
/* ========================================================================== */

/******************************************************************************/
/*                   kdu_stripe_compressor::get_new_tile                      */
/******************************************************************************/

kdsc_tile *
  kdu_stripe_compressor::get_new_tile()
{
  kdsc_tile *tp = free_tiles;
  if (tp == NULL)
    {
      int c, min_subsampling;
      tp = new kdsc_tile;
      tp->num_components = num_components;
      tp->components = new kdsc_component[num_components];
      for (c=0; c < num_components; c++)
        {
          tp->components[c].original_precision =
            comp_states[c].original_precision;
          kdu_coords subs; codestream.get_subsampling(c,subs,true);
          tp->components[c].vert_subsampling = subs.y;
          if ((c == 0) || (subs.y < min_subsampling))
            min_subsampling = subs.y;
        }
      for (c=0; c < num_components; c++)
        tp->components[c].count_delta = min_subsampling;
    }
  else
    free_tiles = tp->next;
  tp->next = NULL;
  return tp;
}

/******************************************************************************/
/*                   kdu_stripe_compressor::release_tile                      */
/******************************************************************************/

void
  kdu_stripe_compressor::release_tile(kdsc_tile *tp)
{
  tp->next = free_tiles;
  free_tiles = tp;
}

/******************************************************************************/
/*                kdu_stripe_compressor::kdu_stripe_compressor                */
/******************************************************************************/

kdu_stripe_compressor::kdu_stripe_compressor()
{
  flush_layer_specs = 0;
  flush_sizes = NULL;
  flush_slopes = NULL;
  force_precise = false;
  want_fastest = false;
  all_done = true;
  num_components = 0;
  left_tile_idx = num_tiles = kdu_coords(0,0);
  partial_tiles = free_tiles = NULL;
  lines_since_flush = 0;
  flush_on_tile_boundary = false;
  comp_states = NULL;
  env = NULL; env_queue = NULL; env_dbuf_height = 0;
  env_flush_worker = NULL;
  env_unconfirmed_flush = false;
}

/******************************************************************************/
/*                        kdu_stripe_compressor::start                        */
/******************************************************************************/

void
  kdu_stripe_compressor::start(kdu_codestream codestream, int num_layer_specs,
                               const kdu_long *layer_sizes,
                               const kdu_uint16 *layer_slopes,
                               kdu_uint16 min_slope_threshold,
                               bool no_prediction, bool force_precise,
                               bool record_layer_info_in_comment,
                               double size_tolerance, int num_components,
                               bool want_fastest, kdu_thread_env *env,
                               kdu_thread_queue *env_queue, int env_dbuf_height)
{
  assert((partial_tiles == NULL) && (free_tiles == NULL) &&
         (this->flush_sizes == NULL) && (this->flush_slopes == NULL) &&
         (this->comp_states == NULL) && !this->codestream.exists() &&
         (this->env == NULL));

  // Start by getting some preliminary parameters
  this->codestream = codestream;
  this->record_layer_info_in_comment = record_layer_info_in_comment;
  this->size_tolerance = size_tolerance;
  this->force_precise = force_precise;
  this->want_fastest = want_fastest;
  this->num_components = codestream.get_num_components(true);
  kdu_dims tile_indices; codestream.get_valid_tiles(tile_indices);
  this->num_tiles = tile_indices.size;
  this->left_tile_idx = tile_indices.pos;

  // Next, determine the number of components we are actually going to process
  if (num_components > 0)
    {
      if (num_components > this->num_components)
        { KDU_ERROR_DEV(e,0x06090500); e <<
            KDU_TXT("The optional `num_components' argument supplied to "
            "`kdu_stripe_compressor::start' may not exceed the number of "
            "output components declared by the codestream header.");
        }
      this->num_components = num_components;
    }
  num_components = this->num_components;

  // Now process the rate and slope specifications
  int n;
  kdu_params *cod = codestream.access_siz()->access_cluster(COD_params);
  assert(cod != NULL);
  if (!cod->get(Clayers,0,0,flush_layer_specs))
    flush_layer_specs = 0;
  if (num_layer_specs > 0)
    {
      if (flush_layer_specs == 0)
        {
          flush_layer_specs = num_layer_specs;
          cod->set(Clayers,0,0,num_layer_specs);
        }
      if (flush_layer_specs > num_layer_specs)
        flush_layer_specs = num_layer_specs;
      flush_sizes = new kdu_long[flush_layer_specs];
      flush_slopes = new kdu_uint16[flush_layer_specs];
      for (n=0; n < flush_layer_specs; n++)
        {
          flush_sizes[n] = (layer_sizes != NULL)?(layer_sizes[n]):0;
          flush_slopes[n] = (layer_slopes != NULL)?(layer_slopes[n]):0;
        }
    }
  else
    {
      if (flush_layer_specs == 0)
        {
          flush_layer_specs = 1;
          cod->set(Clayers,0,0,flush_layer_specs);
        }
      flush_sizes = new kdu_long[flush_layer_specs];
      flush_slopes = new kdu_uint16[flush_layer_specs];
      for (n=0; n < flush_layer_specs; n++)
        { flush_sizes[n] = 0; flush_slopes[n] = 0; }
    }

  // Install block truncation prediction mechanisms, as appropriate
  if (!no_prediction)
    {
      if (min_slope_threshold != 0)
        codestream.set_min_slope_threshold(min_slope_threshold);
      else if ((layer_sizes != NULL) && (num_layer_specs > 0) &&
               (layer_sizes[num_layer_specs-1] > 0))
        codestream.set_max_bytes(layer_sizes[num_layer_specs-1]);
      else if ((layer_slopes != NULL) && (num_layer_specs > 0))
        codestream.set_min_slope_threshold(layer_slopes[num_layer_specs-1]);
    }

  // Finalize preparation for compression
  codestream.access_siz()->finalize_all();
  all_done = false;
  lines_since_flush = 0;
  flush_on_tile_boundary = false;
  comp_states = new kdsc_component_state[num_components];
  for (n=0; n < num_components; n++)
    {
      kdsc_component_state *cs = comp_states + n;
      cs->comp_idx = n;
      kdu_dims dims; codestream.get_dims(n,dims,true);
      cs->pos_x = dims.pos.x; // Values used by `kdsc_tile::init'
      cs->width = dims.size.x;
      cs->original_precision = codestream.get_bit_depth(n,true);
      if (cs->original_precision < 0)
        cs->original_precision = -cs->original_precision;
      cs->row_gap = cs->sample_gap = cs->precision = 0;
      cs->buf8=NULL; cs->buf16=NULL; cs->buf32=NULL; cs->buf_float=NULL;
      cs->stripe_height = 0;
      kdu_coords idx = tile_indices.pos;
      codestream.get_tile_dims(idx,n,dims,true);
      cs->remaining_tile_height = dims.size.y;
      cs->max_tile_height = dims.size.y;
      if (tile_indices.size.y > 1)
        {
          idx.y++;
          codestream.get_tile_dims(idx,n,dims,true);
          if (dims.size.y > cs->max_tile_height)
            cs->max_tile_height = dims.size.y;
        }
      cs->max_recommended_stripe_height = 0; // Until we assign one
    }

  // Install multi-threaded environment
  this->env = env;
  this->env_queue = env_queue;
  this->env_dbuf_height = env_dbuf_height;
  if (env != NULL)
    env_flush_worker = new kdsc_flush_worker;
  else
    assert(env_flush_worker == NULL);
  env_unconfirmed_flush = false;
}

/******************************************************************************/
/*                       kdu_stripe_compressor::finish                        */
/******************************************************************************/

bool
  kdu_stripe_compressor::finish(int num_layer_specs,
                                kdu_long *layer_sizes, kdu_uint16 *layer_slopes)
{
  if (env != NULL)
    { // Terminate the multi-threaded processing queues
      env->terminate(env_queue,true);
      env = NULL;  env_queue = NULL;  env_dbuf_height = 0;
    }
  if (env_flush_worker != NULL)
    {
      delete env_flush_worker;
      env_flush_worker = NULL;
      env_unconfirmed_flush = false;
    }

  if (!codestream.exists())
    {
      assert((layer_sizes == NULL) && (layer_slopes == NULL));
      return false;
    }
  if (all_done)
    {
      if (codestream.ready_for_flush()) // Else, incremental flush did it all
        codestream.flush(flush_sizes,flush_layer_specs,flush_slopes,
                         true,record_layer_info_in_comment,size_tolerance);
    }

  int n;
  for (n=0; (n < num_layer_specs) && (n < flush_layer_specs); n++)
    {
      if (layer_sizes != NULL)
        layer_sizes[n] = flush_sizes[n];
      if (layer_slopes != NULL)
        layer_slopes[n] = flush_slopes[n];
    }
  for (; n < num_layer_specs; n++)
    {
      if (layer_sizes != NULL)
        layer_sizes[n] = 0;
      if (layer_slopes != NULL)
        layer_slopes[n] = 0;
    }

  flush_layer_specs = 0;
  if (flush_sizes != NULL)
    delete[] flush_sizes;
  flush_sizes = NULL;
  if (flush_slopes != NULL)
    delete[] flush_slopes;
  flush_slopes = NULL;
  if (comp_states != NULL)
    delete[] comp_states;
  comp_states = NULL;
  codestream = kdu_codestream(); // Make the interface empty
  kdsc_tile *tp;
  while ((tp=partial_tiles) != NULL)
    {
      partial_tiles = tp->next;
      delete tp;
    }
  while ((tp=free_tiles) != NULL)
    {
      free_tiles = tp->next;
      delete tp;
    }
  return all_done;
}

/******************************************************************************/
/*           kdu_stripe_compressor::get_recommended_stripe_heights            */
/******************************************************************************/

bool
  kdu_stripe_compressor::get_recommended_stripe_heights(int preferred_min,
                                                        int absolute_max,
                                                        int rec_heights[],
                                                        int *max_heights)
{
  if (preferred_min < 1)
    preferred_min = 1;
  if (absolute_max < preferred_min)
    absolute_max = preferred_min;
  if (!codestream.exists())
    { KDU_ERROR_DEV(e,1); e <<
        KDU_TXT("You may not call `kdu_stripe_compressor's "
        "`get_recommended_stripe_heights' function without first calling the "
        "`start' function.");
    }
  int c, max_val;

  if (comp_states[0].max_recommended_stripe_height==0)
    { // Need to assign max recommended stripe heights, based on max tile size
      for (max_val=0, c=0; c < num_components; c++)
        {
          comp_states[c].max_recommended_stripe_height =
            comp_states[c].max_tile_height;
          if (comp_states[c].max_tile_height > max_val)
            max_val = comp_states[c].max_tile_height;
        }
      int limit = (num_tiles.x==1)?preferred_min:absolute_max;
      if (limit < max_val)
        {
          int scale = 1 + ((max_val-1) / limit);
          for (c=0; c < num_components; c++)
            {
              comp_states[c].max_recommended_stripe_height =
                1 + (comp_states[c].max_tile_height / scale);
              if (comp_states[c].max_recommended_stripe_height > limit)
                comp_states[c].max_recommended_stripe_height = limit;
            }
        }
    }

  for (max_val=0, c=0; c < num_components; c++)
    {
      kdsc_component_state *cs = comp_states + c;
      rec_heights[c] = cs->remaining_tile_height;
      if (rec_heights[c] > max_val)
        max_val = rec_heights[c];
      if (max_heights != NULL)
        max_heights[c] = cs->max_recommended_stripe_height;
    }
  int limit = (num_tiles.x==1)?preferred_min:absolute_max;
  if (limit < max_val)
    {
      int scale = 1 + ((max_val-1) / limit);
      for (c=0; c < num_components; c++)
        rec_heights[c] = 1 + (rec_heights[c] / scale);
    }
  for (c=0; c < num_components; c++)
    {
      if (rec_heights[c] > comp_states[c].max_recommended_stripe_height)
        rec_heights[c] = comp_states[c].max_recommended_stripe_height;
      if (rec_heights[c] > comp_states[c].remaining_tile_height)
        rec_heights[c] = comp_states[c].remaining_tile_height;
    }
  return (num_tiles.x > 1);
}

/******************************************************************************/
/*                   kdu_stripe_compressor::push_stripe (bytes)               */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_stripe(kdu_byte *stripe_bufs[],
                                     int heights[], int *sample_gaps,
                                     int *row_gaps, int *precisions,
                                     int flush_period)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsc_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf8 = stripe_bufs[c];
      cs->buf16=NULL;  cs->buf32=NULL;  cs->buf_float=NULL;
      cs->stripe_height = heights[c];
      cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?8:(precisions[c]);
      cs->is_signed = false;
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 8) cs->precision = 8;
    }
  return push_common(flush_period);
}

/******************************************************************************/
/*                 kdu_stripe_compressor::push_stripe (bytes, single buf)     */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_stripe(kdu_byte *buffer, int heights[],
                                     int *sample_offsets, int *sample_gaps,
                                     int *row_gaps, int *precisions,
                                     int flush_period)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsc_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf8 = buffer + ((sample_offsets==NULL)?c:(sample_offsets[c]));
      cs->buf16=NULL;  cs->buf32=NULL;  cs->buf_float=NULL;
      cs->stripe_height = heights[c];
      if ((sample_offsets == NULL) && (sample_gaps == NULL))
        cs->sample_gap = num_components;
      else
        cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?8:(precisions[c]);
      cs->is_signed = false;
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 8) cs->precision = 8;
    }
  return push_common(flush_period);
}

/******************************************************************************/
/*                 kdu_stripe_compressor::push_stripe (words)                 */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_stripe(kdu_int16 *stripe_bufs[],
                                     int heights[], int *sample_gaps,
                                     int *row_gaps, int *precisions,
                                     bool *is_signed, int flush_period)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsc_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf16 = stripe_bufs[c];
      cs->buf8=NULL;  cs->buf32=NULL;  cs->buf_float=NULL;
      cs->stripe_height = heights[c];
      cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?16:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 16) cs->precision = 16;
    }
  return push_common(flush_period);
}

/******************************************************************************/
/*              kdu_stripe_compressor::push_stripe (words, single buf)        */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_stripe(kdu_int16 *buffer, int heights[],
                                     int *sample_offsets, int *sample_gaps,
                                     int *row_gaps, int *precisions,
                                     bool *is_signed, int flush_period)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsc_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf16 = buffer + ((sample_offsets==NULL)?c:(sample_offsets[c]));
      cs->buf8=NULL;  cs->buf32=NULL;  cs->buf_float=NULL;
      cs->stripe_height = heights[c];
      if ((sample_offsets == NULL) && (sample_gaps == NULL))
        cs->sample_gap = num_components;
      else
        cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?16:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 16) cs->precision = 16;
    }
  return push_common(flush_period);
}

/******************************************************************************/
/*                 kdu_stripe_compressor::push_stripe (dwords)                */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_stripe(kdu_int32 *stripe_bufs[],
                                     int heights[], int *sample_gaps,
                                     int *row_gaps, int *precisions,
                                     bool *is_signed, int flush_period)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsc_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf32 = stripe_bufs[c];
      cs->buf8=NULL;  cs->buf16=NULL;  cs->buf_float=NULL;
      cs->stripe_height = heights[c];
      cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?32:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 32) cs->precision = 32;
    }
  return push_common(flush_period);
}

/******************************************************************************/
/*              kdu_stripe_compressor::push_stripe (dwords, single buf)       */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_stripe(kdu_int32 *buffer, int heights[],
                                     int *sample_offsets, int *sample_gaps,
                                     int *row_gaps, int *precisions,
                                     bool *is_signed, int flush_period)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsc_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf32 = buffer + ((sample_offsets==NULL)?c:(sample_offsets[c]));
      cs->buf8=NULL;  cs->buf16=NULL;  cs->buf_float=NULL;
      cs->stripe_height = heights[c];
      if ((sample_offsets == NULL) && (sample_gaps == NULL))
        cs->sample_gap = num_components;
      else
        cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?32:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 32) cs->precision = 32;
    }
  return push_common(flush_period);
}

/******************************************************************************/
/*                 kdu_stripe_compressor::push_stripe (floats)                */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_stripe(float *stripe_bufs[],
                                     int heights[], int *sample_gaps,
                                     int *row_gaps, int *precisions,
                                     bool *is_signed, int flush_period)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsc_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_float = stripe_bufs[c];
      cs->buf8=NULL;  cs->buf16=NULL;  cs->buf32=NULL;
      cs->stripe_height = heights[c];
      cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?0:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < -64) cs->precision = -64;
      if (cs->precision > 64) cs->precision = 64;
    }
  return push_common(flush_period);
}

/******************************************************************************/
/*              kdu_stripe_compressor::push_stripe (floats, single buf)       */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_stripe(float *buffer, int heights[],
                                     int *sample_offsets, int *sample_gaps,
                                     int *row_gaps, int *precisions,
                                     bool *is_signed, int flush_period)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsc_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_float = buffer + ((sample_offsets==NULL)?c:(sample_offsets[c]));
      cs->buf8=NULL;  cs->buf16=NULL;  cs->buf32=NULL;
      cs->stripe_height = heights[c];
      if ((sample_offsets == NULL) && (sample_gaps == NULL))
        cs->sample_gap = num_components;
      else
        cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?0:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < -64) cs->precision = -64;
      if (cs->precision > 64) cs->precision = 64;
    }
  return push_common(flush_period);
}

/******************************************************************************/
/*                     kdu_stripe_compressor::push_common                     */
/******************************************************************************/

bool
  kdu_stripe_compressor::push_common(int flush_period)
{
  int c;
  bool push_complete = false;
  lines_since_flush += comp_states[0].stripe_height;
  while (!push_complete)
    {
      int t;
      kdu_coords tile_idx;
      kdsc_tile *tp, *next_tp;

      bool skip_tile_initialization = false;
      if ((env != NULL) && flush_on_tile_boundary && (partial_tiles==NULL))
        { // We must have been pushing in fractions of a tile; otherwise,
          // `flush_on_tile_boundary' would never have been set.  So we might
          // as well open an instance of each horizontal tile here, and then
          // register the flush worker before pushing any data into the tiles.
          // The reason for opening an instance of each tile first is that
          // this operation cannot be executed simultaneously with flushing
          // so that registering the flush job first could cause the processing
          // machinery to be stalled.
          skip_tile_initialization = true;
          assert(env_flush_worker->is_free() && !env_unconfirmed_flush);
          for (tile_idx=left_tile_idx, t=0; t < num_tiles.x; t++, tile_idx.x++)
            {
              if (t==0)
                partial_tiles = tp = get_new_tile();
              else
                tp = tp->next = get_new_tile();
              tp->init(tile_idx,codestream,comp_states,force_precise,
                       want_fastest,env,env_queue,env_dbuf_height);
            }
          env_flush_worker->init(codestream,flush_sizes,
                                 flush_layer_specs,flush_slopes,
                                 true,record_layer_info_in_comment,
                                 size_tolerance);
          env->register_synchronized_job(env_flush_worker,NULL,true);
          env_unconfirmed_flush = true;
          flush_on_tile_boundary = false;
          lines_since_flush = comp_states[0].stripe_height;
                  // Start counting again from the start of the tile.
        }

      next_tp=NULL;
      tp = partial_tiles;
      partial_tiles = NULL;
      tile_idx = left_tile_idx;
      for (t=num_tiles.x; t > 0; t--, tile_idx.x++, tp=next_tp)
        {
          if (!skip_tile_initialization)
            {
              if (tp == NULL)
                tp = get_new_tile();
              tp->init(tile_idx,codestream,comp_states,force_precise,
                       want_fastest,env,env_queue,env_dbuf_height);
            }
          if (tp->process(env))
            { // Tile is completed
              next_tp = tp->next;
              release_tile(tp);
            }
          else
            { // Not enough data to complete tile
              if (partial_tiles == NULL)
                partial_tiles = tp; // Must be first one
              if ((t > 1) && ((next_tp = tp->next) == NULL))
                next_tp = tp->next = get_new_tile(); // Build on the list
            }
        }

      // See if the entire row of tiles is complete or not
      if (partial_tiles == NULL)
        {
          left_tile_idx.y++; num_tiles.y--;
          all_done = (num_tiles.y == 0);
        }
      push_complete = true;
      for (c=0; c < num_components; c++)
        {
          comp_states[c].update(left_tile_idx,codestream,all_done);
          if (comp_states[c].stripe_height > 0)
            push_complete = false;
        }
      if ((partial_tiles != NULL) && !push_complete)
        { KDU_ERROR_DEV(e,2); e <<
            KDU_TXT("Inappropriate use of the `kdu_stripe_compressor' "
            "object.  Image component samples must not be pushed into this "
            "object in such disproportionate fashion as to require the object "
            "to maintain multiple rows of open tile pointers!  See description "
            "of the `kdu_stripe_compressor::push_line' interface function for "
            "more details on how to use it correctly.");
        }
    }

  if (all_done)
    return false;

  if (flush_period <= 0)
    return true;

  // If we get here, incremental flushing may be required
  if (env_unconfirmed_flush && env_flush_worker->is_free())
    {
      env_unconfirmed_flush = false;
      if (env_flush_worker->ready_for_flush_failed())
        lines_since_flush += flush_period - (flush_period>>3);
           // Need to add an extra flush call.  Do this by artificially adding
           // `flush_period', but let the flush clock slip by `flush_period'/8 so
           // that future flush attempts are more likely to succeed.
    }
  if (env_unconfirmed_flush)
    return true; // Don't even think about installing the flush worker until
                 // we can confirm that the last installed flush worker has executed.

  if (partial_tiles != NULL)
    { // See if it is worth waiting until we reach the tile boundary
      int rem = partial_tiles->components->size.y;
      if ((rem < (flush_period>>2)) || flush_on_tile_boundary)
        {
          flush_on_tile_boundary = true;
          return true; // Wait until the tile boundary before flushing
        }
    }

  if (env == NULL)
    { // Simple single-threaded incremental flush algorithm
      if ((lines_since_flush >= flush_period) || flush_on_tile_boundary)
        {
          if (codestream.ready_for_flush())
            {
              codestream.flush(flush_sizes,flush_layer_specs,flush_slopes,
                               true,record_layer_info_in_comment,
                               size_tolerance);
              lines_since_flush -= flush_period;
            }
          else
            { // Not ready yet; try again in a little while
              lines_since_flush -= (flush_period >> 3);
            }
          flush_on_tile_boundary = false;
        }
    }
  else
    { // Multi-threaded incremental flush algorithm
      if ((lines_since_flush >= flush_period) && !flush_on_tile_boundary)
        { // Note: tile boundary flushes are performed right after the next
          // set of tiles have been opened.  This maximizes the likelihood
          // that flush processing can be overlapped with background data
          // compression processing.
          assert(env_flush_worker->is_free());
          env_flush_worker->init(codestream,flush_sizes,
                                 flush_layer_specs,flush_slopes,
                                 true,record_layer_info_in_comment,
                                 size_tolerance);
          env->register_synchronized_job(env_flush_worker,NULL,true);
          env_unconfirmed_flush = true;
          lines_since_flush -= flush_period;
        }
    }

  return true;
}
