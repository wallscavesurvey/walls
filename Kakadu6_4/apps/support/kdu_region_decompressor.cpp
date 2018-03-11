/******************************************************************************/
// File: kdu_region_decompressor.cpp [scope = APPS/SUPPORT]
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
   Implements the incremental, region-based decompression services of the
"kdu_region_decompressor" object.  These services should prove useful
to many interactive applications which require JPEG2000 rendering capabilities.
*******************************************************************************/

#include <assert.h>
#include <string.h>
#include <math.h> // Just used for pre-computing interpolation kernels
#include "kdu_arch.h"
#include "kdu_utils.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"

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
     kdu_error _name("E(kdu_region_decompressor.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(kdu_region_decompressor.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu Region Decompressor:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu Region Decompressor:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers

// Set things up for the inclusion of assembler optimized routines
// for specific architectures.  The reason for this is to exploit
// the availability of SIMD type instructions to greatly improve the
// efficiency with which sample values can be converted, truncated and mapped.
#if ((defined KDU_PENTIUM_MSVC) && (defined _WIN64))
# undef KDU_PENTIUM_MSVC
# ifndef KDU_X86_INTRINSICS
#   define KDU_X86_INTRINSICS // Use portable intrinsics instead
# endif
#endif // KDU_PENTIUM_MSVC && _WIN64

#ifndef KDU_NO_SSE
#  if defined KDU_X86_INTRINSICS
#    define KDU_SIMD_OPTIMIZATIONS
#    include "x86_region_decompressor_local.h" // Contains SIMD intrinsics
#  elif defined KDU_PENTIUM_MSVC
#    define KDU_SIMD_OPTIMIZATIONS
#    include "msvc_region_decompressor_local.h" // Contains asm commands in-line
#  endif
#endif // !KDU_NO_SSE

#include "region_decompressor_local.h"


/* ========================================================================== */
/*                      Shared Tables Initialized Once                        */
/* ========================================================================== */

/* ========================================================================== */
/*                            Internal Functions                              */
/* ========================================================================== */

/******************************************************************************/
/* STATIC                    reduce_ratio_to_ints                             */
/******************************************************************************/

static bool
  reduce_ratio_to_ints(kdu_long &num, kdu_long &den)
  /* Divides `num' and `den' by their common factors until both are reduced
     to values which can be represented as 32-bit signed integers, or else
     no further common factors can be found.  In the latter case the
     function returns false. */
{
  if ((num <= 0) || (den <= 0))
    return false;
  if ((num % den) == 0)
    { num = num / den;  den = 1; }

  kdu_long test_fac = 2;
  while ((num > 0x7FFFFFFF) || (den > 0x7FFFFFFF))
    {
      while (((num % test_fac) != 0) || ((den % test_fac) != 0))
        {
          test_fac++;
          if ((test_fac >= num) || (test_fac >= den))
            return false;
        }
      num = num / test_fac;
      den = den / test_fac;
    }
  return true;
}

/******************************************************************************/
/* STATIC                   find_canvas_cover_dims                            */
/******************************************************************************/

static kdu_dims
  find_canvas_cover_dims(kdu_dims render_dims, kdu_codestream codestream,
                         kdrd_channel *channels, int num_channels,
                         bool on_transformed_canvas)
  /* This function returns the location and dimension of the region on the
     codestream's canvas coordinate system, required to render the region
     identified by `render_dims'.  If `on_transformed_canvas' is true, the
     resulting canvas cover region is to be described on the transformed
     canvas associated with an prevailing geometric transformations associated
     with the most recent call to `codestream.change_appearance'.  This is
     appropriate if the region is to be compared with regions returned by
     `codestream.get_tile_dims', or in fact just about any other codestream
     dimension reporting function, apart from `codestream.map_region'.  If
     you want a region suitable for supplying to
     `codestream.apply_input_restrictions', it must be prepared in a manner
     which is independent of geometric transformations, and this is done using
     `codestream.map_region' -- in this case, set `on_transformed_canvas'
     to false. */
{
  kdu_coords canvas_min, canvas_lim;
  for (int c=0; c < num_channels; c++)
    {
      kdrd_channel *chan = channels + c;
      kdu_long num, den, aln;
      kdu_coords min = render_dims.pos;
      kdu_coords max = min + render_dims.size - kdu_coords(1,1);
      
      num = chan->sampling_numerator.x;
      den = chan->sampling_denominator.x;
      aln = chan->source_alignment.x;
      aln += ((chan->boxcar_size.x-1)*den) / (2*chan->boxcar_size.x);
      min.x = long_floor_ratio(num*min.x-aln,den);
      max.x = long_ceil_ratio(num*max.x-aln,den);
      if (chan->sampling_numerator.x != chan->sampling_denominator.x)
        { min.x -= 2; max.x += 3; }
      
      num = chan->sampling_numerator.y;
      den = chan->sampling_denominator.y;
      aln = chan->source_alignment.y;
      aln += ((chan->boxcar_size.y-1)*den) / (2*chan->boxcar_size.y);
      min.y = long_floor_ratio(num*min.y-aln,den);
      max.y = long_ceil_ratio(num*max.y-aln,den);
      if (chan->sampling_numerator.y != chan->sampling_denominator.y)
        { min.y -= 2; max.y += 3; }
      
      // Convert to canvas coords
      kdu_dims chan_region;
      chan_region.pos = min;
      chan_region.size = max-min+kdu_coords(1,1);
      chan_region.pos.x *= chan->boxcar_size.x;
      chan_region.size.x *= chan->boxcar_size.x;
      chan_region.pos.y *= chan->boxcar_size.y;
      chan_region.size.y *= chan->boxcar_size.y;      
      
      kdu_coords lim;
      if (on_transformed_canvas)
        {
          kdu_coords subs;
          codestream.get_subsampling(chan->source->rel_comp_idx,subs,true);
          min = chan_region.pos;
          lim = min + chan_region.size;
          min.x *= subs.x;   min.y *= subs.y;
          lim.x *= subs.x;   lim.y *= subs.y;
        }
      else
        {
          kdu_dims canvas_region;
          codestream.map_region(chan->source->rel_comp_idx,chan_region,
                                canvas_region,true);
          min = canvas_region.pos;
          lim = min + canvas_region.size;
        }
      if ((c == 0) || (min.x < canvas_min.x))
        canvas_min.x = min.x;
      if ((c == 0) || (min.y < canvas_min.y))
        canvas_min.y = min.y;
      if ((c == 0) || (lim.x > canvas_lim.x))
        canvas_lim.x = lim.x;
      if ((c == 0) || (lim.y > canvas_lim.y))
        canvas_lim.y = lim.y;
    }
  kdu_dims result;
  result.pos = canvas_min;
  result.size = canvas_lim-canvas_min;
  return result;
}

/******************************************************************************/
/* STATIC                       reset_line_buf                                */
/******************************************************************************/

static void                    
  reset_line_buf(kdu_line_buf *buf)
{
  int num_samples = buf->get_width();
  if (buf->get_buf32() != NULL)
    {
      kdu_sample32 *sp = buf->get_buf32();
      if (buf->is_absolute())
        while (num_samples--)
          (sp++)->ival = 0;
      else
        while (num_samples--)
          (sp++)->fval = 0.0F;
    }
  else
    {
      kdu_sample16 *sp = buf->get_buf16();
      while (num_samples--)
        (sp++)->ival = 0;
    }
}

/*****************************************************************************/
/* STATIC             convert_samples_to_palette_indices                     */
/*****************************************************************************/

static void
  convert_samples_to_palette_indices(kdu_line_buf *line, int bit_depth,
                                     bool is_signed, int palette_bits,
                                     kdu_line_buf *indices,
                                     int dst_offset)
{
  int i, width=line->get_width();
  kdu_sample16 *dp = indices->get_buf16();
  assert((dp != NULL) && indices->is_absolute() &&
         (indices->get_width() >= (width+dst_offset)));
  dp += dst_offset;
  kdu_sample16 *sp16 = line->get_buf16();
  kdu_sample32 *sp32 = line->get_buf32();
  
  if (line->is_absolute())
    { // Convert absolute source integers to absolute palette indices
      if (sp16 != NULL)
        {
          kdu_int16 offset = (kdu_int16)((is_signed)?0:((1<<bit_depth)>>1));
          kdu_int16 val, mask = ((kdu_int16)(-1))<<palette_bits;
          for (i=0; i < width; i++)
            {
              val = sp16[i].ival + offset;
              if (val & mask)
                val = (val<0)?0:(~mask);
              dp[i].ival = val;
            }
        }
      else if (sp32 != NULL)
        {
          kdu_int32 offset = (is_signed)?0:((1<<bit_depth)>>1);
          kdu_int32 val, mask = ((kdu_int32)(-1))<<palette_bits;
          for (i=0; i < width; i++)
            {
              val = sp32[i].ival + offset;
              if (val & mask)
                val = (val<0)?0:(~mask);
              dp[i].ival = (kdu_int16) val;
            }
        }
      else
        assert(0);
    }
  else
    { // Convert fixed/floating point samples to absolute palette indices
      if (sp16 != NULL)
        {
          kdu_int16 offset = (kdu_int16)((is_signed)?0:((1<<KDU_FIX_POINT)>>1));
          int downshift = KDU_FIX_POINT-palette_bits; assert(downshift > 0);
          offset += (kdu_int16)((1<<downshift)>>1);
          kdu_int16 val, mask = ((kdu_int16)(-1))<<palette_bits;
          for (i=0; i < width; i++)
            {
              val = (sp16[i].ival + offset) >> downshift;
              if (val & mask)
                val = (val<0)?0:(~mask);
              dp[i].ival = val;
            }
        }
      else if (sp32 != NULL)
        {
          float scale = (float)(1<<palette_bits);
          float offset = 0.5F + ((is_signed)?0.0F:(0.5F*scale));
          kdu_int32 val, mask = ((kdu_int32)(-1))<<palette_bits;
          for (i=0; i < width; i++)
            {
              val = (kdu_int32)((sp32[i].fval * scale) + offset);
              if (val & mask)
                val = (val<0)?0:(~mask);
              dp[i].ival = (kdu_int16) val;
            }
        }
      else
        assert(0);
    }
}

/******************************************************************************/
/* STATIC                    perform_palette_map                              */
/******************************************************************************/

static void
  perform_palette_map(kdu_line_buf *src, kdu_int16 *dst, kdu_sample16 *lut,
                      int num_samples, int missing_source_samples)
  /* This function uses the 16-bit `src' samples as indices into the palette
     lookup table, writing the results to `dst'.  The number of output
     samples to be generated is `num_samples', but we note that some
     samples may be missing from the start and/or end of the `src' buffer,
     in which case sample replication is employed.  It can also happen that
     `missing_source_samples' is -ve, meaning that some initial samples
     from the `src' buffer are to be skipped. */
{
  int src_len = src->get_width();
  if (src_len == 0)
    { // Pathalogical case; no source data at all
      for (; num_samples > 0; num_samples--)
        *(dst++) = 0;
      return;
    }
  kdu_int16 *sp = (kdu_int16 *) src->get_buf16();
  kdu_int16 val = sp[0];
  if (missing_source_samples < 0)
    {
      sp -= missing_source_samples;
      src_len += missing_source_samples; 
      missing_source_samples = 0;
      val = (src_len > 0)?sp[0]:(sp[src_len-1]);
    }
  val = lut[val].ival;
  if (missing_source_samples >= num_samples)
    missing_source_samples = num_samples-1;
  num_samples -= missing_source_samples;
  for (; missing_source_samples > 0; missing_source_samples--)
    *(dst++) = val;
  if (src_len > num_samples)
    src_len = num_samples;
  num_samples -= src_len;
  for (; src_len > 0; src_len--)
    *(dst++) = lut[*(sp++)].ival;
  for (val=dst[-1]; num_samples > 0; num_samples--)
    *(dst++) = val;
}

/******************************************************************************/
/* STATIC                     map_and_integrate                               */
/******************************************************************************/

static void
  map_and_integrate(kdu_line_buf *src, kdu_int32 *dst, kdu_sample16 *lut,
                    int num_cells, int missing_source_samples,
                    int cell_size, int precision)
  /* This function is very similar to `perform_palette_map', except for
     three things.  Firstly, the `lut' outputs may need to be downshifted to
     accommodate a `precision' value which is less than KDU_FIX_POINT (it will
     never be more).  Secondly, the `lut' outputs are accumulated into the
     `dst' buffer, rather than being copied.  Finally, each set of `cell_size'
     consecutive values are added together before being accumulated
     into a single `dst' entry.  After accounting for any
     `missing_source_samples' and sufficiently replicating the last sample
     in the `src' line, the source data may be considered to consist of
     exactly `num_cells' full accumulation cells where the `dst'
     array is considered to have `num_cells' entries. */
{
  int src_len = src->get_width();
  if (src_len == 0)
    { // Pathalogical case; no source data at all
      for (; num_cells > 0; num_cells--)
        *(dst++) = 0;
      return;
    }
  kdu_int16 *sp = (kdu_int16 *) src->get_buf16();
  kdu_int32 val = sp[0];
  if (missing_source_samples < 0)
    {
      sp -= missing_source_samples;
      src_len += missing_source_samples; 
      missing_source_samples = 0;
      val = (src_len > 0)?sp[0]:(sp[src_len-1]);
    }
  int shift = KDU_FIX_POINT-precision;
  assert(shift >= 0);
  kdu_int32 offset = (1<<shift)>>1;
  
  int needed_samples = num_cells*cell_size;
  val = (lut[val].ival + offset)>>shift;
  if (missing_source_samples >= needed_samples)
    missing_source_samples = needed_samples-1;
  needed_samples -= missing_source_samples;
  int cell_counter = cell_size;
  for (; missing_source_samples > 0; missing_source_samples--, cell_counter--)
    {
      if (cell_counter == 0)
        { dst++; cell_counter=cell_size; }
      *dst += val;
    }
  if (src_len > needed_samples)
    src_len = needed_samples;
  needed_samples -= src_len;
  if (shift == 0)
    { // Fast implementation for common case of no shifting
      for (; src_len > 0; src_len--, cell_counter--)
        {
          if (cell_counter == 0)
            { dst++; cell_counter=cell_size; }
          *dst += (val = lut[*(sp++)].ival);
        }      
    }
  else
    {
      for (; src_len > 0; src_len--, cell_counter--)
        {
          if (cell_counter == 0)
            { dst++; cell_counter=cell_size; }
          *dst += (val = (lut[*(sp++)].ival + offset)>>shift);
        }
    }
  for (; needed_samples > 0; needed_samples--, cell_counter--)
    {
      if (cell_counter == 0)
        { dst++; cell_counter=cell_size; }
      *dst += val;
    }
}

/******************************************************************************/
/* STATIC                     convert_and_copy16                              */
/******************************************************************************/

static void
  convert_and_copy16(kdu_line_buf *lines[], int num_lines, int src_precision,
                     kdu_int16 *dst, int num_samples, int missing_src_samples)
  /* This function uses the samples found by concatenating all the line
     buffers in the `lines' array to generate `num_samples' values for the
     `dst' array, stored as fixed-point 16-bit integers, with KDU_FIX_POINT
     fraction bits.  Some of the required input samples may be missing on
     the left (as identified by `missing_src_samples') and/or on the right.
     Such samples are synthesized by boundary replication.  It can also
     happen that `missing_src_samples' is -ve, meaning that some initial
     samples from the input `lines' are to be skipped.  You should note that
     all of the line buffers in the `lines' array are guaranteed to be
     non-empty; however, it could just possibly happen that `num_lines'=0,
     in which case there is no data at all. */
{
  if (num_lines == 0)
    { // Pathalogical case
      for (; num_samples > 0; num_samples--)
        *(dst++) = 0;
      return;      
    }
  
  // Next skip over source samples as required
  kdu_line_buf *src = *(lines++); num_lines--;
  int src_len = src->get_width();
  int n=0; // Use this to index the samples in `src'
  while (missing_src_samples < 0)
    {
      n = -missing_src_samples;
      if (n < src_len)
        missing_src_samples = 0;
      else if (num_lines > 0)
        {
          missing_src_samples += src_len;
          src = *(lines++); num_lines--; src_len=src->get_width();
        }
      else
        { n = src_len-1; missing_src_samples=0; }
    }
  if (missing_src_samples >= num_samples)
    missing_src_samples = num_samples-1;

  // Now perform the sample conversion process
  kdu_int16 val;
  while (num_samples > 0)
    {
      kdu_sample16 *sp16 = src->get_buf16();
      kdu_sample32 *sp32 = src->get_buf32();
      num_samples += n; // Don't worry; we'll effectively take n away again
      dst -= n; // Allows us to address source and dest samples with n
      kdu_int16 *dp = dst;
      num_samples -= missing_src_samples;
      dst += missing_src_samples;
      if (src_len > num_samples)
        src_len = num_samples;
      dst += src_len;
      num_samples -= src_len;
      
      int upshift = KDU_FIX_POINT -
        ((src->is_absolute())?src_precision:KDU_FIX_POINT);
      int downshift = (upshift >= 0)?0:-upshift;
      kdu_int32 offset = (1<<downshift)>>1;
      if (sp16 != NULL)
        {
          if (missing_src_samples > 0)
            { // Generate a single value and replicate it
              val = (upshift > 0)?
                (sp16[0].ival<<upshift):((sp16[0].ival+offset)>>downshift);
              for (; missing_src_samples > 0; missing_src_samples--)
                *(dp++) = val;
            }
#ifdef KDU_SIMD_OPTIMIZATIONS
          if (!simd_convert_fix16(sp16+n,dp+n,src_len-n,upshift,offset))
#endif
            {
              if (upshift == 0)
                for (; n < src_len; n++)
                  dp[n] = sp16[n].ival;
              else if (upshift > 0)
                for (; n < src_len; n++)
                  dp[n] = sp16[n].ival << upshift;
              else
                for (; n < src_len; n++)
                  dp[n] = (sp16[n].ival+offset) >> downshift;
            }
        }
      else if (sp32 != NULL)
        {
          if (missing_src_samples > 0)
            {
              if (src->is_absolute())
                val = (kdu_int16)((upshift > 0)?(sp32[0].ival<<upshift):
                                  ((sp32[0].ival+offset)>>downshift));
              else
                val = (kdu_int16)(sp32[0].fval * (1<<KDU_FIX_POINT));
              for (; missing_src_samples > 0; missing_src_samples--)
                *(dp++) = val;
            }
          if (src->is_absolute())
            {
              if (upshift >= 0)
                for (; n < src_len; n++)
                  dp[n] = (kdu_int16)(sp32[n].ival << upshift);
              else
                for (; n < src_len; n++)
                  dp[n] = (kdu_int16)((sp32[n].ival+offset) >> downshift);
            }
          else
            for (; n < src_len; n++)
              {
                float fval = sp32[n].fval * (1<<KDU_FIX_POINT);
                dp[n] = (fval>=0.0F)?
                  ((kdu_int16)(fval+0.5F)):(-(kdu_int16)(-fval+0.5F));
              }
        }

      // Advance to next line
      if (num_lines == 0)
        break; // All out of data
      src = *(lines++); num_lines--; src_len = src->get_width(); n = 0;
    }
  
  // Perform right edge padding as required
  for (val=dst[-1]; num_samples > 0; num_samples--)
    *(dst++) = val;
}

/******************************************************************************/
/* STATIC                     convert_and_copy32                              */
/******************************************************************************/

static void
  convert_and_copy32(kdu_line_buf *lines[], int num_lines, int src_precision,
                     kdu_int32 *dst, int num_samples, int missing_src_samples,
                     int dst_precision32)
  /* Same as `convert_and_copy16', except that 32-bit integers are written
     with the indicated precision. */
{
  if (num_lines == 0)
    { // Pathalogical case
      for (; num_samples > 0; num_samples--)
        *(dst++) = 0;
      return;      
    }
  
  // Next skip over source samples as required
  kdu_line_buf *src = *(lines++); num_lines--;
  int src_len = src->get_width();
  int n=0; // Use this to index the samples in `src'
  while (missing_src_samples < 0)
    {
      n = -missing_src_samples;
      if (n < src_len)
        missing_src_samples = 0;
      else if (num_lines > 0)
        {
          missing_src_samples += src_len;
          src = *(lines++); num_lines--; src_len=src->get_width();
        }
      else
        { n = src_len-1; missing_src_samples=0; }
    }
  if (missing_src_samples >= num_samples)
    missing_src_samples = num_samples-1;
  
  // Now perform the sample conversion process
  kdu_int32 val;
  while (num_samples > 0)
    {
      kdu_sample16 *sp16 = src->get_buf16();
      kdu_sample32 *sp32 = src->get_buf32();
      num_samples += n; // Don't worry; we'll effectively take n away again
      dst -= n; // Allows us to address source and dest samples with n
      kdu_int32 *dp = dst;
      num_samples -= missing_src_samples;
      dst += missing_src_samples;
      if (src_len > num_samples)
        src_len = num_samples;
      dst += src_len;
      num_samples -= src_len;
      
      int upshift = dst_precision32 -
        ((src->is_absolute())?src_precision:KDU_FIX_POINT);
      int downshift = (upshift >= 0)?0:-upshift;
      kdu_int32 offset = (1<<downshift)>>1;
      if (sp16 != NULL)
        {
          if (missing_src_samples > 0)
            { // Generate a single value and replicate it
              val = sp16[0].ival;
              val = (upshift > 0)?(val<<upshift):((val+offset)>>downshift);
              for (; missing_src_samples > 0; missing_src_samples--)
                *(dp++) = val;
            }
          if (upshift >= 0)
            for (; n < src_len; n++)
              dp[n] = ((kdu_int32) sp16[n].ival) << upshift;
          else
            for (; n < src_len; n++)
              dp[n] = (sp16[n].ival+offset) >> downshift;
        }
      else if (sp32 != NULL)
        {
          float scale = (float)(1<<dst_precision32);
          if (missing_src_samples > 0)
            {
              if (src->is_absolute())
                val = (upshift > 0)?
                  (sp32[0].ival<<upshift):((sp32[0].ival+offset)>>downshift);
              else
                val = (kdu_int32)(sp32[0].fval * scale);
              for (; missing_src_samples > 0; missing_src_samples--)
                *(dp++) = val;              
            }
          if (src->is_absolute())
            {
              if (upshift >= 0)
                for (; n < src_len; n++)
                  dp[n] = sp32[n].ival << upshift;
              else
                for (; n < src_len; n++)
                  dp[n] = (sp32[n].ival+offset) >> downshift;
            }         
          else
            for (; n < src_len; n++)
              {
                float fval = sp32[n].fval * scale;
                dp[n] = (fval>=0.0F)?
                  ((kdu_int32)(fval+0.5F)):(-(kdu_int32)(-fval+0.5F));
              }
        }
      
      // Advance to next line
      if (num_lines == 0)
        break; // All out of data
      src = *(lines++); num_lines--; src_len = src->get_width(); n = 0;
    }
  
  // Perform right edge padding as required
  for (val=dst[-1]; num_samples > 0; num_samples--)
    *(dst++) = val;
}

/******************************************************************************/
/* STATIC                    convert_and_copy_float                           */
/******************************************************************************/

static void
  convert_and_copy_float(kdu_line_buf *lines[], int num_lines,
                         int src_precision, float *dst,
                         int num_samples, int missing_src_samples)
  /* Same as `convert_and_copy16', except that floating point values are
     written, with a nominal range of -0.5 to +0.5. */
{
  if (num_lines == 0)
    { // Pathalogical case
      for (; num_samples > 0; num_samples--)
        *(dst++) = 0.0F;
      return;      
    }
  
  // Next skip over source samples as required
  kdu_line_buf *src = *(lines++); num_lines--;
  int src_len = src->get_width();
  int n=0; // Use this to index the samples in `src'
  while (missing_src_samples < 0)
    {
      n = -missing_src_samples;
      if (n < src_len)
        missing_src_samples = 0;
      else if (num_lines > 0)
        {
          missing_src_samples += src_len;
          src = *(lines++); num_lines--; src_len=src->get_width();
        }
      else
        { n = src_len-1; missing_src_samples=0; }
    }
  if (missing_src_samples >= num_samples)
    missing_src_samples = num_samples-1;
  
  // Now perform the sample conversion process
  float val;
  while (num_samples > 0)
    {
      kdu_sample16 *sp16 = src->get_buf16();
      kdu_sample32 *sp32 = src->get_buf32();
      num_samples += n; // Don't worry; we'll effectively take n away again
      dst -= n; // Allows us to address source and dest samples with n
      float *dp = dst;
      num_samples -= missing_src_samples;
      dst += missing_src_samples;
      if (src_len > num_samples)
        src_len = num_samples;
      dst += src_len;
      num_samples -= src_len;
      
      float scale = 1.0F;
      if (src->is_absolute())
        scale = 1.0F / (1<<src_precision);
      else if (sp16 != NULL)
        scale = 1.0F / (1<<KDU_FIX_POINT);
      if (sp16 != NULL)
        {
          if (missing_src_samples > 0)
            { // Generate a single value and replicate it
              val = sp16[0].ival * scale;
              for (; missing_src_samples > 0; missing_src_samples--)
                *(dp++) = val;
            }
          for (; n < src_len; n++)
            dp[n] = sp16[n].ival * scale;
        }
      else if (sp32 != NULL)
        {
          if (missing_src_samples > 0)
            {
              val=(src->is_absolute())?(sp32[0].ival*scale):sp32[0].fval;
              for (; missing_src_samples > 0; missing_src_samples--)
                *(dp++) = val;              
            }
          if (src->is_absolute())
            for (; n < src_len; n++)
              dp[n] = sp32[n].ival * scale;
          else
            for (; n < src_len; n++)
              dp[n] = sp32[n].fval;
        }
      
      // Advance to next line
      if (num_lines == 0)
        break; // All out of data
      src = *(lines++); num_lines--; src_len = src->get_width(); n = 0;
    }
  
  // Perform right edge padding as required
  for (val=dst[-1]; num_samples > 0; num_samples--)
    *(dst++) = val;
}

/******************************************************************************/
/* STATIC                      convert_and_add32                              */
/******************************************************************************/

static void
  convert_and_add32(kdu_line_buf **lines, int num_lines,
                    int src_precision, kdu_int32 *dst,
                    int num_cells, int missing_src_samples,
                    int cell_size, int dst_precision32) 
  /* This function is the same as `convert_and_copy32', except that the
     converted sample values are accumulated into the `dst' buffer, rather
     than being copied.  In fact, each set of `cell_size' consecutive values
     are added together before being accumulated into a single `dst' entry.
     After accounting for any `missing_src_samples' and sufficiently replicating
     the last available source sample, the source data may be considered to
     consist of exactly `num_cells' full accumulation cells, where the `dst'
     array is considered to have `num_cells' entries. */
{
  if (num_lines == 0)
    return; // Nothing to accumulate
  
  // Next skip over source samples as required
  kdu_line_buf *src = *(lines++); num_lines--;
  int src_len = src->get_width();
  int n=0; // Use this to index the samples in `src'
  while (missing_src_samples < 0)
    {
      n = -missing_src_samples;
      if (n < src_len)
        missing_src_samples = 0;
      else if (num_lines > 0)
        {
          missing_src_samples += src_len;
          src = *(lines++); num_lines--; src_len=src->get_width();
        }
      else
        { n = src_len-1; missing_src_samples=0; }
    }
  
  int needed_samples = num_cells*cell_size;
  if (missing_src_samples >= needed_samples)
    missing_src_samples = needed_samples-1;
  
  // Now perform the sample conversion and accumulation process
  kdu_int32 val=0;
  int ccounter = cell_size;
  while (needed_samples > 0)
    {
      kdu_sample16 *sp16 = src->get_buf16();
      kdu_sample32 *sp32 = src->get_buf32();
      needed_samples += n; // Don't worry; we'll effectively take n away again
      needed_samples -= missing_src_samples;
      if (src_len > needed_samples)
        src_len = needed_samples;
      needed_samples -= src_len;
      
      int upshift = dst_precision32 -
        ((src->is_absolute())?src_precision:KDU_FIX_POINT);
      int downshift = (upshift >= 0)?0:-upshift;
      kdu_int32 offset = (1<<downshift)>>1;
      if (sp16 != NULL)
        {
          if (missing_src_samples > 0)
            { // Generate a single value and replicate it
              val = sp16[0].ival;
              val = (upshift > 0)?(val<<upshift):((val+offset)>>downshift);
              for (; missing_src_samples > 0; missing_src_samples--, ccounter--)
                {
                  if (ccounter == 0)
                    { dst++; ccounter=cell_size; }
                  *dst += val;
                }
            }
          if (upshift >= 0)
            for (; n < src_len; n++, ccounter--)
              {
                if (ccounter == 0)
                  { dst++; ccounter=cell_size; }
                *dst += (val = ((kdu_int32) sp16[n].ival) << upshift);
              }
          else
            for (; n < src_len; n++, ccounter--)
              {
                if (ccounter == 0)
                  { dst++; ccounter=cell_size; }
                *dst += (val = (sp16[n].ival+offset) >> downshift);
              }
        }
      else if (sp32 != NULL)
        {
          float scale = (float)(1<<dst_precision32);
          if (missing_src_samples > 0)
            {
              if (src->is_absolute())
                val = (upshift > 0)?
                  (sp32[0].ival<<upshift):((sp32[0].ival+offset)>>downshift);
              else
                val = (kdu_int32)(sp32[0].fval * scale);
              for (; missing_src_samples > 0; missing_src_samples--, ccounter--)
                {
                  if (ccounter == 0)
                    { dst++; ccounter=cell_size; }
                  *dst += val;
                }
            }
          if (src->is_absolute())
            {
              if (upshift >= 0)
                for (; n < src_len; n++, ccounter--)
                  {
                    if (ccounter == 0)
                      { dst++; ccounter=cell_size; }
                    *dst += (val = sp32[n].ival << upshift);
                  }
              else
                for (; n < src_len; n++, ccounter--)
                  {
                    if (ccounter == 0)
                      { dst++; ccounter=cell_size; }
                    *dst += (val = (sp32[n].ival+offset) >> downshift);
                  }
            }         
          else
            for (; n < src_len; n++, ccounter--)
              {
                if (ccounter == 0)
                  { dst++; ccounter=cell_size; }
                float fval = sp32[n].fval * scale;
                val = (fval>=0.0F)?
                  ((kdu_int32)(fval+0.5F)):(-(kdu_int32)(-fval+0.5F));
                *dst += val;
              }
        }
      
      // Advance to next line
      if (num_lines == 0)
        break; // All out of data
      src = *(lines++); num_lines--; src_len = src->get_width(); n = 0;
    }
  
  for (; needed_samples > 0; needed_samples--, ccounter--)
    {
      if (ccounter == 0)
        { dst++; ccounter=cell_size; }
      *dst += val;
    }
}

/******************************************************************************/
/* STATIC                    convert_and_add_float                            */
/******************************************************************************/

static void
  convert_and_add_float(kdu_line_buf **lines, int num_lines,
                        int src_precision, float *dst,
                        int num_cells, int missing_src_samples,
                        int cell_size, float scale) 
  /* This function is the same as `convert_and_add32' except that the
     target cells have a floating point representation with a nominal range
     from -0.5 to +0.5 and the converted sample values which are accumulated
     into this buffer are scaled so to have a nominal range from
     -scale/2 to +scale/2. */
{
  if (num_lines == 0)
    return; // Nothing to accumulate
  
  // Next skip over source samples as required
  kdu_line_buf *src = *(lines++); num_lines--;
  int src_len = src->get_width();
  int n=0; // Use this to index the samples in `src'
  while (missing_src_samples < 0)
    {
      n = -missing_src_samples;
      if (n < src_len)
        missing_src_samples = 0;
      else if (num_lines > 0)
        {
          missing_src_samples += src_len;
          src = *(lines++); num_lines--; src_len=src->get_width();
        }
      else
        { n = src_len-1; missing_src_samples=0; }
    }
  
  int needed_samples = num_cells*cell_size;
  if (missing_src_samples >= needed_samples)
    missing_src_samples = needed_samples-1;
  
  // Now perform the sample conversion and accumulation process
  float val=0.0F;
  int ccounter = cell_size;
  while (needed_samples > 0)
    {
      kdu_sample16 *sp16 = src->get_buf16();
      kdu_sample32 *sp32 = src->get_buf32();
      needed_samples += n; // Don't worry; we'll effectively take n away again
      needed_samples -= missing_src_samples;
      if (src_len > needed_samples)
        src_len = needed_samples;
      needed_samples -= src_len;

      if (src->is_absolute())
        scale *= 1.0F / (1<<src_precision);
      else if (sp16 != NULL)
        scale *= 1.0F / (1<<KDU_FIX_POINT);
      if (sp16 != NULL)
        {
          if (missing_src_samples > 0)
            { // Generate a single value and replicate it
              val = sp16[0].ival * scale;
              for (; missing_src_samples > 0; missing_src_samples--, ccounter--)
                {
                  if (ccounter == 0)
                    { dst++; ccounter=cell_size; }
                  *dst += val;
                }
            }
          for (; n < src_len; n++, ccounter--)
            {
              if (ccounter == 0)
                { dst++; ccounter=cell_size; }
              *dst += (val = sp16[n].ival * scale);
            }
        }
      else if (sp32 != NULL)
        {
          if (missing_src_samples > 0)
            {
              val=(src->is_absolute())?(sp32[0].ival*scale):sp32[0].fval*scale;
              for (; missing_src_samples > 0; missing_src_samples--, ccounter--)
                {
                  if (ccounter == 0)
                    { dst++; ccounter=cell_size; }
                  *dst += val;
                }
            }
          if (src->is_absolute())
            for (; n < src_len; n++, ccounter--)
              {
                if (ccounter == 0)
                  { dst++; ccounter=cell_size; }
                *dst += (val = sp32[n].ival * scale);
              }
          else
            for (; n < src_len; n++, ccounter--)
              {
                if (ccounter == 0)
                  { dst++; ccounter=cell_size; }
                *dst += (val = sp32[n].fval * scale);
              }
        }
      
      // Advance to next line
      if (num_lines == 0)
        break; // All out of data
      src = *(lines++); num_lines--; src_len = src->get_width(); n = 0;
    }
  
  for (; needed_samples > 0; needed_samples--, ccounter--)
    {
      if (ccounter == 0)
        { dst++; ccounter=cell_size; }
      *dst += val;
    }
}

/******************************************************************************/
/* STATIC                  do_boxcar_renormalization                          */
/******************************************************************************/

static void
  do_boxcar_renormalization(kdu_int32 *src, kdu_int16 *dst,
                            int dst_len, int in_precision)
  /* This function is used to renormalize the cells created by boxcar
     accumulation of integer source samples.  After boxcar integration, the
     accumulated values are stored in the `src' array, having `in_precision'
     bits which must be shifted back down to KDU_FIX_POINT precision bits.
     The `src' and `dst' arrays occupy the same block of memory, with some
     offsets so that it is safe to perform the transformation here in place,
     so long as we walk forwards through the data. */
{
  int shift = in_precision - KDU_FIX_POINT;
  assert(shift > 0);
  kdu_int32 offset = (1<<shift)>>1;
  for (; dst_len > 0; dst_len--)
    *(dst++) = (kdu_int16)((*(src++) + offset) >> shift);
}

/******************************************************************************/
/* STATIC                  do_horz_resampling_float                           */
/******************************************************************************/

static void
  do_horz_resampling_float(int length, kdu_line_buf *src, kdu_line_buf *dst,
                           int phase, int num, int den, int pshift,
                           int kernel_length, float *kernels[])
  /* This function implements disciplined horizontal resampling for floating
     point sample data.  The function generates `length' outputs, writing them
     into the `dst' line.
        If `kernel_length' is 6, each output is formed by taking the inner
     product between a 6-sample kernel and 6 samples from the `src' line.  The
     first output (at `dst'[0]) is formed by taking an inner product with
     the source samples `src'[-2] throught `src'[3] with (`kernels'[p])[0]
     through (`kernels'[p])[6], where p is equal to (`phase'+`off')>>`pshift'
     and `off' is equal to (1<<`pshift')/2.  After generating each sample in
     `dst', the `phase' value is incremented by `num'.  If this leaves
     `phase' >= `den', we subtract `den' from `phase' and increment the
     `src' pointer -- we may need to do this multiple times -- after which
     we are ready to generate the next sample for `dst'.
        Otherwise, `kernel_length' is 2 and each output is formed by taking
     the inner product between a 2-tap kernel and 2 samples from the `src'
     line; the first such output is formed by taking an inner product with
     the source samples `src'[0] and `src'[1].  In this special case, the
     `kernels' array actually holds 4 kernels, having lengths 2, 3, 4 and 5,
     which can be used to simultaneously form 4 successive output samples.
     The function can (but does not need to) take advantage of this, allowing
     the phase to be evaluated up to 4 times less frequently than would
     otherwise be required. */
{
  int off = (1<<pshift)>>1;
  float *sp = (float *) src->get_buf32();
  float *dp = (float *) dst->get_buf32();
  if (kernel_length == 6)
    {
      for (; length > 0; length--, dp++)
        {
          float *kern = kernels[((kdu_uint32)(phase+off))>>pshift];
          phase += num;
          *dp = sp[-2]*kern[0] + sp[-1]*kern[1] + sp[0]*kern[2] +
          sp[1]*kern[3] + sp[2]*kern[4] + sp[3]*kern[5];
          while (((kdu_uint32) phase) >= ((kdu_uint32) den))
            { phase -= den; sp++; }
        }
    }
  else
    { 
      assert(kernel_length == 2);

      // Note: we could provide targeted code fragments for various
      // special cases here.  In particular if `phase'=0 and `den'=2*`num' or
      // `den'=4*`num', the implementation can be particularly trivial.

      if (den < (INT_MAX>>2))
        { // Just to be on the safe side
          int num4 = num<<2;
          for (; length > 3; length-=4, dp+=4)
            {
              float *kern = kernels[((kdu_uint32)(phase+off))>>pshift];
              phase += num4;
              dp[0] = sp[0]*kern[0] + sp[1]*kern[1];
              dp[1] = sp[0]*kern[2] + sp[1]*kern[3] + sp[2]*kern[4];
              dp[2] = sp[0]*kern[5] + sp[1]*kern[6] + sp[2]*kern[7]
                    + sp[3]*kern[8];
              dp[3] = sp[0]*kern[9] + sp[1]*kern[10] + sp[2]*kern[11]
                    + sp[3]*kern[12]+ sp[4]*kern[13];
              while (((kdu_uint32) phase) >= ((kdu_uint32) den))
                { phase -= den; sp++; }
            }
        }
      for (; length > 0; length--, dp++)
        {
          float *kern = kernels[((kdu_uint32)(phase+off))>>pshift];
          phase += num; *dp = sp[0]*kern[0] + sp[1]*kern[1];
          if (((kdu_uint32) phase) >= ((kdu_uint32) den))
            { phase -= den; sp++; } // Can only happen once, because 2-tap
                                    // filters are only used for expansion
        }
    }
}

/******************************************************************************/
/* STATIC                  do_horz_resampling_fix16                           */
/******************************************************************************/

static void
  do_horz_resampling_fix16(int length, kdu_line_buf *src, kdu_line_buf *dst,
                           int phase, int  num, int den, int pshift,
                           int kernel_length, kdu_int32 *kernels[])
  /* This function is the same as `do_horz_resampling_float', except that
     it processes 16-bit integers with a nominal precision of KDU_FIX_POINT.
     The interpolation kernels in this case have a negated fixed-point
     representation with 15 fraction bits -- this means that after taking the
     integer-valued inner product, the result v must be negated and divided
     by 2^15, leaving (16384-v)>>15. */
{
  int off = (1<<pshift)>>1;
  kdu_int16 *sp = (kdu_int16 *) src->get_buf16();
  kdu_int16 *dp = (kdu_int16 *) dst->get_buf16();
  if (kernel_length == 6)
    { 
      for (; length > 0; length--, dp++)
        { 
          kdu_int32 sum, *kern = kernels[((kdu_uint32)(phase+off))>>pshift];
          phase += num;
          sum = sp[-2]*kern[0] + sp[-1]*kern[1] + sp[0]*kern[2] +
                sp[1]*kern[3] + sp[2]*kern[4] + sp[3]*kern[5];
          *dp = (kdu_int16)(((1<<14)-sum)>>15);
          while (((kdu_uint32) phase) >= ((kdu_uint32) den))
            { phase -= den; sp++; }
        }
    }
  else
    {
      assert(kernel_length == 2);
      if (den < (INT_MAX>>2))
        { // Just to be on the safe side
          int num4 = num<<2;
          if ((phase == 0) && (num4 == (den+den)))
            { // Special case of aligned interpolation by 2
              int k0=(*kernels)[2], k1=(*kernels)[3];
              for (; length > 3; length-=4, dp+=4, sp+=2)
                { 
                  dp[0] = sp[0];
                  dp[1] = (kdu_int16)(((1<<14) - sp[0]*k0 - sp[1]*k1)>>15);
                  dp[2] = sp[1];
                  dp[3] = (kdu_int16)(((1<<14) - sp[1]*k0 - sp[2]*k1)>>15);
                }
            }
          else if ((phase == 0) && (num4 == den))
            { // Special case of aligned interpolation by 4
              int k0=(*kernels)[2], k1=(*kernels)[3];
              int k2=(*kernels)[5], k3=(*kernels)[6];
              int k4=(*kernels)[9], k5=(*kernels)[10];
              for (; length > 3; length-=4, dp+=4, sp+=2)
                { 
                  dp[0] = sp[0];
                  dp[1] = (kdu_int16)(((1<<14) - sp[0]*k0 - sp[1]*k1)>>15);
                  dp[2] = (kdu_int16)(((1<<14) - sp[0]*k2 - sp[1]*k3)>>15);
                  dp[3] = (kdu_int16)(((1<<14) - sp[0]*k4 - sp[1]*k5)>>15);                  
                }
            }
          else
            { // General case
              for (; length > 3; length-=4, dp+=4)
                { 
                  kdu_int32 sum;
                  kdu_int32 *kern=kernels[((kdu_uint32)(phase+off))>>pshift];
                  phase += num4;
                  sum = sp[0]*kern[0] + sp[1]*kern[1];
                  dp[0] = (kdu_int16)(((1<<14)-sum)>>15);
                  sum = sp[0]*kern[2] + sp[1]*kern[3] + sp[2]*kern[4];
                  dp[1] = (kdu_int16)(((1<<14)-sum)>>15);
                  sum = sp[0]*kern[5] + sp[1]*kern[6] + sp[2]*kern[7]
                      + sp[3]*kern[8];
                  dp[2] = (kdu_int16)(((1<<14)-sum)>>15);
                  sum = sp[0]*kern[9] + sp[1]*kern[10] + sp[2]*kern[11]
                      + sp[3]*kern[12] + sp[4]*kern[13];
                  dp[3] = (kdu_int16)(((1<<14)-sum)>>15);
                  while (((kdu_uint32) phase) >= ((kdu_uint32) den))
                    { phase -= den; sp++; }
                }
            }
        }
      for (; length > 0; length--, dp++)
        {
          kdu_int32 sum, *kern = kernels[((kdu_uint32)(phase+off))>>pshift];
          phase += num; sum = sp[0]*kern[0] + sp[1]*kern[1];
          *dp = (kdu_int16)(((1<<14)-sum)>>15);
          if (((kdu_uint32) phase) >= ((kdu_uint32) den))
            { phase -= den; sp++; } // Can only happen once, because 2-tap
                                    // filters are only used for expansion
        }
    }
}

/******************************************************************************/
/* STATIC                  do_vert_resampling_float                           */
/******************************************************************************/

static void
  do_vert_resampling_float(int length, kdu_line_buf *src[], kdu_line_buf *dst,
                           int kernel_length, float *kernel)
  /* This function implements disciplined vertical resampling for floating
     point sample data.  The function generates `length' outputs, writing them
     into the `dst' line.  Each output is formed by taking the inner product
     between a 6-sample `kernel' and corresponding entries from each of the
     6 lines referenced from the `src' array.  If `kernel_length'=2, the
     first 2 and last 2 lines are ignored and the central lines are formed
     using only `kernel'[0] and `kernel'[1]. */
{
  if (kernel_length == 6)
    { 
      float *sp0=(float *)(src[0]->get_buf32());
      float *sp1=(float *)(src[1]->get_buf32());
      float *sp2=(float *)(src[2]->get_buf32());
      float *sp3=(float *)(src[3]->get_buf32());
      float *sp4=(float *)(src[4]->get_buf32());
      float *sp5=(float *)(src[5]->get_buf32());
      float k0=kernel[0], k1=kernel[1], k2=kernel[2],
            k3=kernel[3], k4=kernel[4], k5=kernel[5];
      float *dp = (float *)(dst->get_buf32());
      for (int n=0; n < length; n++)
        dp[n] = (sp0[n]*k0 + sp1[n]*k1 + sp2[n]*k2 +
                 sp3[n]*k3 + sp4[n]*k4 + sp5[n]*k5);
    }
  else
    {
      assert(kernel_length == 2);
      float *sp0=(float *)(src[2]->get_buf32());
      float *sp1=(float *)(src[3]->get_buf32());
      float k0=kernel[0], k1=kernel[1];
      float *dp = (float *)(dst->get_buf32());
      for (int n=0; n < length; n++)
        dp[n] = sp0[n]*k0 + sp1[n]*k1;
    }
}

/******************************************************************************/
/* STATIC                  do_vert_resampling_fix16                           */
/******************************************************************************/

static void
  do_vert_resampling_fix16(int length, kdu_line_buf *src[], kdu_line_buf *dst,
                           int kernel_length, kdu_int32 *kernel)
  /* This function is the same as `do_vert_resampling_float', except that
     it processes 16-bit integers with a nominal precision of KDU_FIX_POINT.
     The interpolation `kernel' in this case have a negated fixed-point
     representation with 15 fraction bits -- this means that after taking the
     integer-valued inner product, the result v must be negated and divided
     by 2^15, leaving (16384-v)>>15. */
{
  if (kernel_length == 6)
    { 
      kdu_int16 *sp0=(kdu_int16 *)(src[0]->get_buf16());
      kdu_int16 *sp1=(kdu_int16 *)(src[1]->get_buf16());
      kdu_int16 *sp2=(kdu_int16 *)(src[2]->get_buf16());
      kdu_int16 *sp3=(kdu_int16 *)(src[3]->get_buf16());
      kdu_int16 *sp4=(kdu_int16 *)(src[4]->get_buf16());
      kdu_int16 *sp5=(kdu_int16 *)(src[5]->get_buf16());
      kdu_int32 k0=kernel[0], k1=kernel[1], k2=kernel[2],
                k3=kernel[3], k4=kernel[4], k5=kernel[5];
      kdu_int16 *dp = (kdu_int16 *)(dst->get_buf16());
      for (int n=0; n < length; n++)
        { 
          kdu_int32 sum = (sp0[n]*k0 + sp1[n]*k1 + sp2[n]*k2 +
                           sp3[n]*k3 + sp4[n]*k4 + sp5[n]*k5);
          dp[n] = ((1<<14)-sum)>>15;
        }
    }
  else
    {
      assert(kernel_length == 2);
      kdu_int16 *sp0=(kdu_int16 *)(src[2]->get_buf16());
      kdu_int16 *sp1=(kdu_int16 *)(src[3]->get_buf16());
      kdu_int32 k0=kernel[0], k1=kernel[1];
      kdu_int16 *dp = (kdu_int16 *)(dst->get_buf16());
      if (k1 == 0)
        memcpy(dp,sp0,(size_t)(length<<1));
      else if (k0 == 0)
        memcpy(dp,sp1,(size_t)(length<<1));
      else if (k0 == k1)
        for (int n=0; n < length; n++)
          dp[n] = (kdu_int16)((((int) sp0[n]) + ((int) sp1[n]) + 1) >> 1);
      else
        for (int n=0; n < length; n++)
          { 
            kdu_int32 sum = sp0[n]*k0 + sp1[n]*k1;
            dp[n] = ((1<<14)-sum)>>15;
          }
    }
}

/******************************************************************************/
/* STATIC                    apply_white_stretch                              */
/******************************************************************************/

static void
  apply_white_stretch(kdu_line_buf *in, kdu_line_buf *out, int num_cols,
                      kdu_uint16 stretch_residual)
{
  kdu_sample16 *sp = in->get_buf16();
  kdu_sample16 *dp = out->get_buf16();
  assert((sp != NULL) && (dp != NULL) &&
         (num_cols <= in->get_width()) && (num_cols <= out->get_width()));
#ifdef KDU_SIMD_OPTIMIZATIONS
  if (!simd_apply_white_stretch(sp,dp,num_cols,stretch_residual))
#endif // KDU_SIMD_OPTIMIZATIONS
    {
      kdu_int32 val, stretch_factor=stretch_residual;
      kdu_int32 offset = -((-(stretch_residual<<(KDU_FIX_POINT-1))) >> 16);
      for (; num_cols > 0; num_cols--, sp++,dp++)
        {
          val = sp->ival;
          val += ((val*stretch_factor)>>16) + offset;
          dp->ival = (kdu_int16) val;
        }
    }
}

/******************************************************************************/
/* STATIC                 transfer_fixed_point (bytes)                        */
/******************************************************************************/

static void
  transfer_fixed_point(kdu_line_buf *src, int src_precision32,
                       int skip_samples, int num_samples, int gap,
                       kdu_byte *dest, int precision_bits, bool leave_signed)
  /* Transfers source samples from the supplied line buffer into the output
     byte buffer, spacing successive output samples apart by `gap' bytes.
     Only `num_samples' samples are transferred, skipping over the initial
     `skip_samples' from the `src' buffer.  The source data has one of the
     following three representations: 1) signed 16-bit fixed-point with
     KDU_FIX_POINT fraction bits; 2) 32-bit floating point in the range
     -0.5 to +0.5; and 3) signed 32-bit absolute integers with a nominal
     bit-depth of `src_precision32' bits.  `precision_bits' indicates the
     precision of the unsigned integers to be written into the `dest' bytes. */
{
  assert((num_samples+skip_samples) <= src->get_width());
  if (src->get_buf16() != NULL)
    { // Transferring from 16-bit fixed-point integers
      kdu_sample16 *sp = src->get_buf16() + skip_samples;
      assert((sp != NULL) && !src->is_absolute());
      if (precision_bits <= 8)
        { // Normal case, in which we want to fit the data completely within
          // the 8-bit output samples
          int downshift = KDU_FIX_POINT-precision_bits;
          kdu_int16 val, offset, mask;
          offset = (1<<downshift)>>1; // Rounding offset for the downshift
          offset += (1<<KDU_FIX_POINT)>>1; // Convert from signed to unsigned
          mask = (kdu_int16)((-1) << precision_bits);
          if (leave_signed)
            { // Unusual case in which we want a signed result
              kdu_int16 post_offset = (kdu_int16)((1<<precision_bits)>>1);
              for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                {
                  val = (sp->ival+offset)>>downshift;
                  if (val & mask)
                    val = (val < 0)?0:~mask;
                  *dest = (kdu_byte)(val-post_offset);
                }
            }
          else
            { // Typical case of conversion to 8-bit result
#ifdef KDU_SIMD_OPTIMIZATIONS
              if (gap == 1)
                {
                  for (; (skip_samples & 0x0F) && (num_samples > 0);
                       num_samples--, skip_samples++, sp++, dest++)
                    {
                      val = (sp->ival+offset)>>downshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_byte) val;                    
                    }
                  if (simd_transfer_to_consecutive_bytes(sp,dest,num_samples,
                                                         offset,downshift,mask))
                    num_samples = 0;
                }
#endif // KDU_SIMD_OPTIMIZATIONS
              for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                {
                  val = (sp->ival+offset)>>downshift;
                  if (val & mask)
                    val = (val < 0)?0:~mask;
                  *dest = (kdu_byte) val;
                }
            }
        }
      else
        { // Unusual case in which the output precision is insufficient.  In
          // this case, clipping should be based upon the 8-bit output
          // precision after conversion to a signed or unsigned representation
          // as appropriate.
          int upshift=0, downshift = KDU_FIX_POINT-precision_bits;
          if (downshift < 0)
            { upshift=-downshift; downshift=0; }
          kdu_int16 min, max, val, offset=(kdu_int16)((1<<downshift)>>1);
          if (leave_signed)
            {
              min = (kdu_int16)(-128>>upshift);
              max = (kdu_int16)(127>>upshift);
            }
          else
            {
              offset += (1<<KDU_FIX_POINT)>>1;
              min = 0;  max = (kdu_int16)(255>>upshift);
            }
          for (; num_samples > 0; num_samples--, sp++, dest+=gap)
            {
              val = (sp->ival+offset) >> downshift;
              if (val < min)
                val = min;
              else if (val > max)
                val = max;
              val <<= upshift;
              *dest = (kdu_byte) val;
            }
        }
    }
  else if (src->is_absolute())
    { // Transferring from 32-bit integer samples
      kdu_sample32 *sp = src->get_buf32() + skip_samples;
      if (precision_bits <= 8)
        { // Normal case, in which we want to fit the data completely within
          // the 8-bit output samples
          int downshift = src_precision32-precision_bits;
          kdu_int32 val, mask = (kdu_int32)((-1) << precision_bits);
          kdu_int32 offset=(1<<src_precision32)>>1;// Convert signed to unsigned
          if (downshift >= 0)
            { // Normal case
              offset += (1<<downshift)>>1; // Rounding offset for the downshift
              if (leave_signed)
                {
                  kdu_int32 post_offset = ((1<<precision_bits)>>1);
                  for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                    {
                      val = (sp->ival+offset)>>downshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_byte)(val-post_offset);
                    }
                }
              else
                { // Typical case of conversion to unsigned result
                  for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                    {
                      val = (sp->ival+offset)>>downshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_byte) val;
                    }
                }
            }
          else
            { // Unusual case, in which original precision is less than 8
              int upshift = -downshift;
              if (leave_signed)
                {
                  kdu_int32 post_offset = ((1<<precision_bits)>>1);
                  for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                    {
                      val = (sp->ival+offset)<<upshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_byte)(val-post_offset);
                    }
                }
              else
                { // Conversion to unsigned result
                  for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                    {
                      val = (sp->ival+offset)<<upshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_byte) val;
                    }
                }
            }
        }
      else
        { // Unusual case in which the output precision is insufficient.  In
          // this case, clipping should be based upon the 8-bit output
          // precision after conversion to a signed or unsigned representation
          // as appropriate.
          int upshift=0, downshift = src_precision32-precision_bits;
          if (downshift < 0)
            { upshift=-downshift; downshift=0; }
          kdu_int32 min, max, val, offset=(kdu_int32)((1<<downshift)>>1);
          if (leave_signed)
            {
              min = (-128>>upshift);
              max = (127>>upshift);
            }
          else
            {
              offset += (1<<src_precision32)>>1;
              min = 0;  max = (255>>upshift);
            }
          for (; num_samples > 0; num_samples--, sp++, dest+=gap)
            {
              val = (sp->ival+offset) >> downshift;
              if (val < min)
                val = min;
              else if (val > max)
                val = max;
              val <<= upshift;
              *dest = (kdu_byte) val;
            }
        }
    }
  else
    { // Transferring from floating point samples
      kdu_sample32 *sp = src->get_buf32() + skip_samples;
      float scale = (float)(1<<precision_bits);
      float offset = 0.5F + 0.5F/scale; // Rounding offset + signed to unsigned
      kdu_int32 val, mask=(kdu_int32)((-1)<<precision_bits);
      kdu_uint32 post_offset = (kdu_int32)((1<<precision_bits)>>1);
      if (precision_bits > 8)
        { // Adjust mask, offset and post_offset to clip for 8 bits
          mask = 0xFFFFFF00;
          if (leave_signed)
            offset = 128.5F/scale;
          post_offset = 128;
        }
      if (leave_signed)
        {
          for (; num_samples > 0; num_samples--, sp++, dest+=gap)
            {
              val = (kdu_int32)((sp->fval + offset)*scale);
              if (val & mask)
                val = (val < 0)?0:~mask;
              *dest = (kdu_byte)(val-post_offset);
            }
        }
      else
        {
          for (; num_samples > 0; num_samples--, sp++, dest+=gap)
            {
              val = (kdu_int32)((sp->fval + offset)*scale);
              if (val & mask)
                val = (val < 0)?0:~mask;
              *dest = (kdu_byte) val;
            }                
        }
    }
}

/******************************************************************************/
/* STATIC                 transfer_fixed_point (words)                        */
/******************************************************************************/

static void
  transfer_fixed_point(kdu_line_buf *src, int src_precision32,
                       int skip_samples, int num_samples, int gap,
                       kdu_uint16 *dest, int precision_bits,
                       bool leave_signed)
  /* Transfers source samples from the supplied line buffer into the output
     16-bit buffer, spacing successive output samples apart by `gap' bytes.
     Only `num_samples' samples are transferred, skipping over the initial
     `skip_samples' from the `src' buffer.  The source data has one of the
     following three representations: 1) signed 16-bit fixed-point with
     KDU_FIX_POINT fraction bits; 2) 32-bit floating point in the range
     -0.5 to +0.5; and 3) signed 32-bit absolute integers with a nominal
     bit-depth of `src_precision32' bits.  `precision_bits' indicates the
     precision of the unsigned integers to be written into the `dest' bytes. */
{
  assert((skip_samples+num_samples) <= src->get_width());
  if (src->get_buf16() != NULL)
    { // Transferring from 16-bit fixed-point samples
      kdu_sample16 *sp = src->get_buf16() + skip_samples;
      assert((sp != NULL) && !src->is_absolute());
      int downshift = KDU_FIX_POINT-precision_bits;
      kdu_int16 val, offset, mask;
      if (downshift >= 0)
        {
          offset = (1<<downshift)>>1; // Rounding offset for the downshift
          offset += (1<<KDU_FIX_POINT)>>1; // Conversion from signed to unsigned
          mask = (kdu_int16)((-1) << precision_bits);
          if (leave_signed)
            {
              kdu_int16 post_offset = (kdu_int16)((1<<precision_bits)>>1);
              for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                {
                  val = (sp->ival+offset)>>downshift;
                  if (val & mask)
                    val = (val < 0)?0:~mask;
                  *dest = (kdu_uint16)(val-post_offset);
                }
            }
          else
            { // Typical case
              for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                {
                  val = (sp->ival+offset)>>downshift;
                  if (val & mask)
                    val = (val < 0)?0:~mask;
                  *dest = (kdu_uint16) val;
                }
            }
        }
      else if (precision_bits <= 16)
        { // Need to upshift, but result still fits in 16 bits
          int upshift = -downshift;
          mask = (kdu_int16)((-1) << KDU_FIX_POINT); // Apply the mask first
          offset = (1<<KDU_FIX_POINT)>>1; // Conversion from signed to unsigned
          if (leave_signed)
            {
              for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                {
                  val = sp->ival+offset;
                  if (val & mask)
                    val = (val < 0)?0:~mask;
                  *dest = (kdu_uint16)((val-offset) << upshift);
                }
            }
          else
            {
              for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                {
                  val = sp->ival+offset;
                  if (val & mask)
                    val = (val < 0)?0:~mask;
                  *dest = (kdu_uint16)(val << upshift);
                }
            }
        }
      else
        { // Unusual case in which the output precision is insufficient.  In
          // this case, clipping should be based upon the 16-bit output
          // precision after conversion to a signed or unsigned representation
          // as appropriate.
          int upshift = -downshift;
          kdu_int32 min, max, val, offset=0;
          if (leave_signed)
            {
              min = (-(1<<15)) >> upshift;
              max = ((1<<15)-1) >> upshift;
            }
          else
            {
              offset += (1<<KDU_FIX_POINT)>>1;
              min = 0;  max = ((1<<16)-1) >> upshift;
            }
          for (; num_samples > 0; num_samples--, sp++, dest+=gap)
            {
              val = sp->ival + offset;
              if (val < min)
                val = min;
              else if (val > max)
                val = max;
              val <<= upshift;
              *dest = (kdu_uint16) val;
            }
        }
    }
  else if (src->is_absolute())
    { // Transferring from 32-bit integer samples
      kdu_sample32 *sp = src->get_buf32() + skip_samples;
      if (precision_bits <= 16)
        { // Normal case, in which we want to fit the data completely within
          // the 16-bit output samples
          int downshift = src_precision32-precision_bits;
          kdu_int32 val, mask = (kdu_int32)((-1) << precision_bits);
          kdu_int32 offset=(1<<src_precision32)>>1;// Convert signed to unsigned
          if (downshift >= 0)
            { // Normal case
              offset += (1<<downshift)>>1; // Rounding offset for the downshift
              if (leave_signed)
                {
                  kdu_int32 post_offset = (1<<precision_bits)>>1;
                  for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                    {
                      val = (sp->ival+offset)>>downshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_uint16)(val-post_offset);
                    }
                }
              else
                { // Typical case
                  for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                    {
                      val = (sp->ival+offset)>>downshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_uint16) val;
                    }
                }
            }
          else
            { // Unusual case, in which original precision is less than 16
              int upshift = -downshift;
              if (leave_signed)
                {
                  kdu_int32 post_offset = (1<<precision_bits)>>1;
                  for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                    {
                      val = (sp->ival+offset)<<upshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_uint16)(val-post_offset);
                    }
                }
              else
                {
                  for (; num_samples > 0; num_samples--, sp++, dest+=gap)
                    {
                      val = (sp->ival+offset)<<upshift;
                      if (val & mask)
                        val = (val < 0)?0:~mask;
                      *dest = (kdu_uint16) val;
                    }                  
                }
            }
        }
      else
        { // Unusual case in which the output precision is insufficient.  In
          // this case, clipping should be based upon the 16-bit output
          // precision after conversion to a signed or unsigned representation
          // as appropriate.
          int upshift=0, downshift = src_precision32-precision_bits;
          if (downshift < 0)
            { upshift=-downshift; downshift=0; }
          kdu_int32 min, max, val, offset=(kdu_int32)((1<<downshift)>>1);
          if (leave_signed)
            {
              min = (-(1<<15)) >> upshift;
              max = ((1<<15)-1) >> upshift;
            }
          else
            {
              offset += (1<<src_precision32)>>1;
              min = 0;  max = ((1<<16)-1) >> upshift;
            }
          for (; num_samples > 0; num_samples--, sp++, dest+=gap)
            {
              val = (sp->ival+offset) >> downshift;
              if (val < min)
                val = min;
              else if (val > max)
                val = max;
              val <<= upshift;
              *dest = (kdu_uint16) val;
            }
        }
    }
  else
    { // Transferring from floating point samples
      kdu_sample32 *sp = src->get_buf32() + skip_samples;
      float scale = (float)(1<<precision_bits);
      float offset = 0.5F + 0.5F/scale; // Rounding offset + signed to unsigned
      kdu_int32 val, mask=(kdu_int32)((-1)<<precision_bits);
      kdu_uint32 post_offset = (kdu_int32)((1<<precision_bits)>>1);
      if (precision_bits > 16)
        { // Adjust mask, offset and post_offset to clip for 16 bits
          mask = 0xFFFF0000;
          if (leave_signed)
            offset = ((1<<15)+0.5F)/scale;
          post_offset = (1<<15);
        }
      if (leave_signed)
        {
          for (; num_samples > 0; num_samples--, sp++, dest+=gap)
            {
              val = (kdu_int32)((sp->fval + offset)*scale);
              if (val & mask)
                val = (val < 0)?0:~mask;
              *dest = (kdu_uint16)(val-post_offset);
            }
        }
      else
        {
          for (; num_samples > 0; num_samples--, sp++, dest+=gap)
            {
              val = (kdu_int32)((sp->fval + offset)*scale);
              if (val & mask)
                val = (val < 0)?0:~mask;
              *dest = (kdu_uint16) val;
            }                
        }
    }
}

/******************************************************************************/
/* STATIC                       transfer_floats                               */
/******************************************************************************/

static void
  transfer_floats(kdu_line_buf *src, int src_precision32,
                  int skip_samples, int num_samples, int gap,
                  float *dest, int precision_bits, bool leave_signed,
                  float stretch_source_factor)
  /* Transfers source samples from the supplied line buffer into the output
     floating-point buffer, spacing successive output samples apart by `gap'
     bytes.  Only `num_samples' samples are transferred, skipping over the
     initial `skip_samples' from the `src' buffer.  The source data has one of
     the following three representations: 1) signed 16-bit fixed-point with
     KDU_FIX_POINT fraction bits; 2) 32-bit floating point in the range
     -0.5 to +0.5; and 3) signed 32-bit absolute integers with a nominal
     bit-depth of `src_precision32' bits.
        `precision_bits' identifies the target nominal range for the output
     samples.  Specifically, the samples are normalized to the range
     -2^{P-1} to (2^{P-1}-1), if `leave_signed' is true, or to the range
     0 to (2^P - 1), if `leave_signed' is false.  If `precision_bits' is 0,
     these ranges become -0.5 to +0.5 and 0 to 1.0, respectively.
        `stretch_source_factor' tells us how much the input samples need to
     be "stretched" by in order to exactly fit a nominal range of -2^{P-1} to
     2^{P-1} (inclusive), where P is the precision associated with the
     source representation.  This factor is usually very close to 1.0. */
{
  float min_val=0.0F, max_val=1.0F;
  if (precision_bits > 0)
    {
      for (; precision_bits > 30; precision_bits-=30)
        max_val *= (float)(1<<30);
      max_val *= (float)(1<<precision_bits);
      max_val -= 1.0F;
    }
  float scale = max_val * stretch_source_factor;
  float offset = 0.5F * scale;
  if (leave_signed)
    {
      min_val = -0.5F*(max_val+1.0F);
      max_val += min_val;
      offset += min_val;
    }
  
  assert((skip_samples+num_samples) <= src->get_width());
  if (src->get_buf16() != NULL)
    { // Transferring from 16-bit fixed-point samples
      kdu_sample16 *sp = src->get_buf16() + skip_samples;
      assert((sp != NULL) && !src->is_absolute());
      scale *= 1.0F / (1<<KDU_FIX_POINT);
      for (; num_samples > 0; num_samples--, sp++, dest+=gap)
        {
          float val = sp->ival*scale + offset;
          if (val < min_val)
            *dest = min_val;
          else if (val > max_val)
            *dest = max_val;
          else
            *dest = val;
        }
    }
  else if (src->is_absolute())
    { // Transferring from 32-bit integer samples
      kdu_sample32 *sp = src->get_buf32() + skip_samples;
      scale *= 1.0F / (1<<src_precision32);
      for (; num_samples > 0; num_samples--, sp++, dest+=gap)
        {
          float val = sp->ival*scale + offset;
          if (val < min_val)
            *dest = min_val;
          else if (val > max_val)
            *dest = max_val;
          else
            *dest = val;
        }
    }
  else
    { // Transferring from floating point samples
      kdu_sample32 *sp = src->get_buf32() + skip_samples;
      for (; num_samples > 0; num_samples--, sp++, dest+=gap)
        {
          float val = sp->fval*scale + offset;
          if (val < min_val)
            *dest = min_val;
          else if (val > max_val)
            *dest = max_val;
          else
            *dest = val;
        }
    }
}


/* ========================================================================== */
/*                            kdrd_interp_kernels                             */
/* ========================================================================== */

#ifndef M_PI
#  define M_PI 3.1415926535
#endif

/******************************************************************************/
/*                         kdrd_interp_kernels::init                          */
/******************************************************************************/

void
  kdrd_interp_kernels::init(float expansion_factor, float max_overshoot,
                            float zero_overshoot_threshold)
{
  if (max_overshoot < 0.0F)
    max_overshoot = 0.0F;
  assert(expansion_factor > 0.0F);
  int kernel_len = 6;
  if (expansion_factor > 1.0F)
    { 
      if ((max_overshoot == 0.0F) ||
          (expansion_factor >= zero_overshoot_threshold))
        { max_overshoot = 0.0F; kernel_len = 2; }
      else
        max_overshoot*=(expansion_factor-1.0F)/(zero_overshoot_threshold-1.0F);
    }
  if ((expansion_factor == this->target_expansion_factor) &&
      (max_overshoot == this->derived_max_overshoot) &&
      (kernel_len == this->kernel_length))
    return;
  this->target_expansion_factor = expansion_factor;
  this->derived_max_overshoot = max_overshoot;
  this->simd_kernel_type = KDRD_SIMD_KERNEL_NONE;
  this->simd_kernel_length = simd_horz_leadin = 0;
  this->kernel_length = kernel_len;
  this->kernel_coeffs = (kernel_len == 2)?14:kernel_len;
  float bw = (expansion_factor < 1.0F)?expansion_factor:1.0F;

  // Start by generating the floating-point kernels
  int k, n;
  float *kernel = float_kernels;
  if (kernel_length == 2)
    { // Special case, setting up 2-tap kernels with horizontal extension
      float rate = 1.0F / expansion_factor;
      for (k=0; k < 33; k++, kernel+=KDRD_INTERP_KERNEL_STRIDE)
        { 
          float x, sigma = k * (1.0F/32.0F);
          int ncoeffs=2, lim_n=2;
          for (n=0; ncoeffs <= 5; ncoeffs++, lim_n+=ncoeffs, sigma+=rate)
            {
              for (x=sigma; x > 1.0F; n++, x-=1.0F)
                kernel[n] = 0.0F;
              kernel[n++] = 1.0F - x;
              kernel[n++] = x;
              for (; n < lim_n; n++)
                kernel[n] = 0.0F;
            }
          assert((n <= KDRD_INTERP_KERNEL_STRIDE) && (n == kernel_coeffs));
        }
    }
  else
    { // Regular case, setting up 6-tap kernels
      assert(kernel_length == 6);
      // Generate the first half of the floating point kernels first
      for (k=0; k <= 16; k++, kernel+=KDRD_INTERP_KERNEL_STRIDE)
        {
          float gain=0.0F, sigma = k * (1.0F/32.0F);
          for (n=0; n < 6; n++)
            {
              double x = (n-2.0F-sigma)*M_PI;
              if ((x > -0.0001) && (x < 0.0001))
                kernel[n] = (float) bw;
              else
                kernel[n] = (float)(sin(bw*x)/x);
              kernel[n] *= 1.0F + (float) cos(x * 1.0/3.0);
              gain += kernel[n];
            }
          gain = 1.0F / gain;
          float step_overshoot = 0.0F, ovs_acc=0.0F;
          for (n=0; n < 6; n++)
            { 
              kernel[n] *= gain;
              ovs_acc += kernel[n];
              if (ovs_acc < -step_overshoot)
                step_overshoot = -ovs_acc;
              else if (ovs_acc > (1.0F+step_overshoot))
                step_overshoot = ovs_acc - 1.0F;
            }
          if (step_overshoot > max_overshoot)
            { // Form a weighted average of the windowed sinc kernel and the
              // 2-tap kernel whose coefficients are all positive (has no step
              // overshoot).
              float frac = max_overshoot / step_overshoot; // Fraction to keep
              for (n=0; n < 6; n++)
                kernel[n] *= frac;
              kernel[2] += (1.0F-frac)*(1.0F-sigma);
              kernel[3] += (1.0F-frac)*sigma;
              for (step_overshoot=0.0F, ovs_acc=0.0F, n=0; n < 6; n++)
                {
                  ovs_acc += kernel[n];
                  if (ovs_acc < -step_overshoot)
                    step_overshoot = -ovs_acc;
                  else if (ovs_acc > (1.0F+step_overshoot))
                    step_overshoot = ovs_acc - 1.0F;
                }
              assert((step_overshoot < (max_overshoot+0.001F)) &&
                     (step_overshoot > (max_overshoot-0.001F)));
            }
        }
  
      // Now generate the second half of the floating point kernels by mirror
      // imaging the first half
      float *ref_kernel = kernel-2*KDRD_INTERP_KERNEL_STRIDE+kernel_length-1;
      for (k=0; k <= 16; k++, kernel+=KDRD_INTERP_KERNEL_STRIDE)
        for (n=0; n < kernel_length; n++)
          kernel[n] = ref_kernel[-n];
    }
  
  // Finally generate the fixed-point kernels
  kernel = float_kernels;
  kdu_int32 *kernel16 = fix16_kernels;
  for (k=0; k < 33; k++,
       kernel+=KDRD_INTERP_KERNEL_STRIDE, kernel16+=KDRD_INTERP_KERNEL_STRIDE)
    for (n=0; n < kernel_coeffs; n++)
      kernel16[n] = -((kdu_int32) floor(0.5 + kernel[n]*(1<<15)));
}

/******************************************************************************/
/*                         kdrd_interp_kernels::copy                          */
/******************************************************************************/

bool
  kdrd_interp_kernels::copy(kdrd_interp_kernels &src, float expansion_factor,
                            float max_overshoot, float zero_overshoot_threshold)
{
  if (max_overshoot < 0.0F)
    max_overshoot = 0.0F;
  assert(expansion_factor > 0.0F);
  int kernel_len=6;
  if (expansion_factor > 1.0F)
    { 
      if ((max_overshoot == 0.0F) ||
          (expansion_factor >= zero_overshoot_threshold))
        { max_overshoot = 0.0F; kernel_length = 2; }
      else
        max_overshoot*=(expansion_factor-1.0F)/(zero_overshoot_threshold-1.0F);
    }
  if ((expansion_factor == this->target_expansion_factor) &&
      (max_overshoot == this->derived_max_overshoot) &&
      (kernel_len == this->kernel_length))
    return true;
  if ((max_overshoot < (0.95F*src.derived_max_overshoot)) ||
      (max_overshoot > (1.05F*src.derived_max_overshoot)))
    return false;
  if ((src.target_expansion_factor < (0.95F*src.target_expansion_factor)) ||
      (src.target_expansion_factor > (1.05F*src.target_expansion_factor)))
    return false;
  if (src.kernel_length != kernel_len)
    return false; // Kernels for two-tap expansion are in a special form
  this->target_expansion_factor = expansion_factor;
  this->derived_max_overshoot = src.derived_max_overshoot;
  memcpy(this->float_kernels,src.float_kernels,
         sizeof(float)*33*KDRD_INTERP_KERNEL_STRIDE);
  memcpy(this->fix16_kernels,src.fix16_kernels,
         sizeof(kdu_int32)*33*KDRD_INTERP_KERNEL_STRIDE);
  this->simd_kernel_length = src.simd_kernel_length;
  this->simd_horz_leadin = src.simd_horz_leadin;
  this->kernel_length = src.kernel_length;
  this->kernel_coeffs = src.kernel_coeffs;
  this->simd_kernel_type = KDRD_SIMD_KERNEL_NONE;
  return true;
}

/******************************************************************************/
/*                   kdrd_interp_kernels::get_simd_kernel                     */
/******************************************************************************/
#ifdef KDU_SIMD_OPTIMIZATIONS
void *
  kdrd_interp_kernels::get_simd_kernel(int type, int which)
{
  int n;
  double rate;
  if (target_expansion_factor <= 0.0F)
    { assert(0); rate = 2.99; }
  else
    rate = 1.0 / target_expansion_factor;
  if (rate >= 3.0)
    { assert(0); rate = 2.99; }
  assert(type != KDRD_SIMD_KERNEL_NONE);
  if (type != this->simd_kernel_type)
    {
      simd_kernel_type = type;
      simd_kernels_initialized = 0;
      if ((type == KDRD_SIMD_KERNEL_VERT_FLOATS) ||
          (type == KDRD_SIMD_KERNEL_VERT_FIX16))
        { simd_kernel_length = kernel_length; simd_horz_leadin = 0; }
      else if (type == KDRD_SIMD_KERNEL_HORZ_FLOATS)
        {
          if (kernel_length == 2)
            {
              assert(rate < 1.0);
              simd_horz_leadin = 0;
              simd_kernel_length = 3 + (int)(rate*3.0);
            }
          else if (rate < 1.0)
            {
              assert(kernel_length == 6);
              simd_horz_leadin = 3 + (int)((1.0-rate)*3.0);
              assert(simd_horz_leadin <= 5);
              simd_kernel_length = simd_horz_leadin + 4;
            }
          else
            {
              assert(kernel_length == 6);
              simd_horz_leadin = 2;
              simd_kernel_length = 7 + (int)((rate-1.0)*3.0);
            }
        }
      else if (type == KDRD_SIMD_KERNEL_HORZ_FIX16)
        {
          if (kernel_length == 2)
            {
              assert(rate <= 1.0);
              simd_horz_leadin = 0;
              simd_kernel_length = 3 + (int)(rate*7.0);
            }
          else if (rate < 1.0)
            {
              assert(kernel_length == 6);
              simd_horz_leadin = 3 + (int)((1.0-rate)*7.0);
              assert(simd_horz_leadin <= 9);
              simd_kernel_length = simd_horz_leadin + 4;
            }
          else
            {
              assert(kernel_length == 6);
              simd_horz_leadin = 2;
              simd_kernel_length = 7 + (int)((rate-1.0)*7.0);
              assert(simd_kernel_length <= 20);
            }
        }
      else
        assert(0);
      if (simd_kernel_length > 20)
        { assert(0); simd_kernel_length = 20; } // For release mode safety
      kdu_int32 *storage = this->simd_block;
      kdu_int32 addr = _addr_to_kdu_int32(storage);
      storage += (4 - (addr >> 2)) & 3;
      assert((_addr_to_kdu_int32(storage) & 15) == 0);
      for (n=0; n < 33; n++, storage += 4*simd_kernel_length)
        simd_kernels[n] = storage;
    }
  if ((simd_kernels_initialized >> which) & 1)
    return simd_kernels[which]; // Already initialized
  
  if (type == KDRD_SIMD_KERNEL_VERT_FLOATS)
    {
      float *dp = (float *) simd_kernels[which];
      float *sp = float_kernels + KDRD_INTERP_KERNEL_STRIDE*which;
      for (n=0; n < kernel_length; n++, dp+=4)
        dp[0] = dp[1] = dp[2] = dp[3] = sp[n];
    }
  else if (type == KDRD_SIMD_KERNEL_VERT_FIX16)
    {
      kdu_int16 *dp = (kdu_int16 *) simd_kernels[which];
      kdu_int32 *sp = fix16_kernels + KDRD_INTERP_KERNEL_STRIDE*which;
      for (n=0; n < kernel_length; n++, dp+=8)
        {
          kdu_int16 val = (kdu_int16) sp[n];
          assert(sp[n] == (kdu_int32) val);
          dp[0] = dp[1] = dp[2] = dp[3] = dp[4] = dp[5] = dp[6] = dp[7] = val;
        }
    }
  else if (type == KDRD_SIMD_KERNEL_HORZ_FLOATS)
    {
      float *dpp = (float *) simd_kernels[which];
      int m, offset, k=which;
      double real_pos = which*(1.0/32.0);
      if (kernel_length == 6)
        { 
          offset = simd_horz_leadin-2;
          real_pos += offset;
          for (m=0; m < 4; m++, real_pos+=rate-1.0, dpp++)
            {
              if (m > 0)
                {
                  offset = (int) real_pos;
                  k = (int)((real_pos - offset)*32.0 + 0.5);
                }
              assert((offset >= 0) && (offset <= (simd_kernel_length-6)) &&
                     (k >= 0) && (k <= 32));
              float *dp=dpp, *sp = float_kernels + KDRD_INTERP_KERNEL_STRIDE*k;
              for (n=0; n < offset; n++, dp+=4)
                *dp = 0.0F;
              for (k=0; k < 6; k++, dp+=4)
                *dp = sp[k];
              for (n+=6; n < simd_kernel_length; n++, dp+=4)
                *dp = 0.0F;
            }
        }
      else
        {
          assert((kernel_length == 2) && (rate < 1.0));
          offset = 0;
          for (m=0; m < 4; m++, real_pos+=rate, dpp++)
            {
              if (m > 0)
                {
                  offset = (int) real_pos;
                  k = (int)((real_pos-offset)*32.0 + 0.5);
                }
              float *dp=dpp, *sp = float_kernels + KDRD_INTERP_KERNEL_STRIDE*k;
              for (n=0; n < offset; n++, dp+=4)
                *dp = 0.0F;
              dp[0] = sp[0];  dp[4] = sp[1];
              for (dp+=8, n+=2; n < simd_kernel_length; n++, dp+=4)
                *dp = 0.0F;
              assert(n == simd_kernel_length);
            }          
        }
    }
  else
    {
      kdu_int16 *dpp = (kdu_int16 *) simd_kernels[which];
      int m, offset, k=which;
      double real_pos = which*(1.0/32.0);
      if (kernel_length == 6)
        { 
          offset = simd_horz_leadin-2;
          real_pos += offset;
          for (m=0; m < 8; m++, real_pos+=rate-1.0, dpp++)
            {
              if (m > 0)
                {
                  offset = (int) real_pos;
                  k = (int)((real_pos - offset)*32.0 + 0.5);
                }
              assert((offset >= 0) && (offset <= (simd_kernel_length-6)) &&
                     (k >= 0) && (k <= 32));
              kdu_int32 *sp = fix16_kernels + KDRD_INTERP_KERNEL_STRIDE*k;
              kdu_int16 *dp=dpp;
              for (n=0; n < offset; n++, dp+=8)
                *dp = 0;
              for (k=0; k < 6; k++, dp+=8)
                *dp = (kdu_int16) sp[k];
              for (n+=6; n < simd_kernel_length; n++, dp+=8)
                *dp = 0;
            }
        }
      else
        {
          assert((kernel_length == 2) && (rate < 1.0));
          offset = 0;
          for (m=0; m < 8; m++, real_pos+=rate, dpp++)
            {
              if (m > 0)
                {
                  offset = (int) real_pos;
                  k = (int)((real_pos - offset)*32.0 + 0.5);
                }
              kdu_int32 *sp = fix16_kernels + KDRD_INTERP_KERNEL_STRIDE*k;
              kdu_int16 *dp=dpp;
              for (n=0; n < offset; n++, dp+=8)
                *dp = 0;
              dp[0] = (kdu_int16) sp[0];  dp[8] = (kdu_int16) sp[1];
              for (dp+=16, n+=2; n < simd_kernel_length; n++, dp+=8)
                *dp = 0;
            }
        }
    }
  
  simd_kernels_initialized |= ((kdu_int64) 1) << which;
  return simd_kernels[which];
}
#endif // KDU_SIMD_OPTIMIZATIONS


/* ========================================================================== */
/*                           kdu_channel_mapping                             */
/* ========================================================================== */

/******************************************************************************/
/*                 kdu_channel_mapping::kdu_channel_mapping                   */
/******************************************************************************/

kdu_channel_mapping::kdu_channel_mapping()
{
  num_channels = 0;
  source_components = NULL; palette = NULL; palette_bit_depth = NULL;
  default_rendering_precision = NULL;
  default_rendering_signed = NULL;
  clear();
}

/******************************************************************************/
/*                        kdu_channel_mapping::clear                          */
/******************************************************************************/

void
  kdu_channel_mapping::clear()
{
  if (palette != NULL)
    {
      for (int c=0; c < num_channels; c++)
        if (palette[c] != NULL)
          delete[] palette[c];
      delete[] palette;
    }
  palette = NULL;
  if (palette_bit_depth != NULL)
    delete[] palette_bit_depth;
  palette_bit_depth = NULL;
  if (source_components != NULL)
    delete[] source_components;
  source_components = NULL;
  if (default_rendering_precision != NULL)
    delete[] default_rendering_precision;
  default_rendering_precision = NULL;
  if (default_rendering_signed != NULL)
    delete[] default_rendering_signed;
  default_rendering_signed = NULL;
  num_channels = num_colour_channels = 0; palette_bits = 0;
  colour_converter.clear();
}

/******************************************************************************/
/*                   kdu_channel_mapping::set_num_channels                    */
/******************************************************************************/

void
  kdu_channel_mapping::set_num_channels(int num)
{
  assert(num >= 0);
  if (num_channels >= num)
    {
      num_channels = num;
      return;
    }
  
  int *new_comps = new int[num];
  int *new_precs = new int[num];
  bool *new_signed = new bool[num];
  int c = 0;
  if (source_components != NULL)
    {
      for (c=0; (c < num_channels) && (c < num); c++)
        {
          new_comps[c] = source_components[c];
          new_precs[c] = default_rendering_precision[c];
          new_signed[c] = default_rendering_signed[c];
        }
      delete[] source_components;
      delete[] default_rendering_precision;
      delete[] default_rendering_signed;
    }
  source_components = new_comps;
  default_rendering_precision = new_precs;
  default_rendering_signed = new_signed;
  for (; c < num; c++)
    {
      source_components[c] = -1;
      default_rendering_precision[c] = 8;
      default_rendering_signed[c] = false;
    }

  kdu_sample16 **new_palette = new kdu_sample16 *[num];
  memset(new_palette,0,sizeof(kdu_sample16 *)*(size_t)num);
  c = 0;
  if (palette != NULL)
    {
      for (c=0; (c < num_channels) && (c < num); c++)
        new_palette[c] = palette[c];
      for (int d=c; d < num_channels; d++)
        if (palette[d] != NULL)
          delete[] palette[d];
      delete[] palette;
    }
  palette = new_palette;
  
  int *new_palette_bit_depth = new int[num];
  memset(new_palette_bit_depth,0,sizeof(int)*(size_t)num);
  c = 0;
  if (palette_bit_depth != NULL)
    {
      for (c=0; (c < num_channels) && (c < num); c++)
        new_palette_bit_depth[c] = palette_bit_depth[c];
      delete[] palette_bit_depth;
    }
  palette_bit_depth = new_palette_bit_depth;
  num_channels = num;
}

/******************************************************************************/
/*                    kdu_channel_mapping::configure (raw)                    */
/******************************************************************************/

bool
  kdu_channel_mapping::configure(kdu_codestream codestream)
{
  clear();
  set_num_channels((codestream.get_num_components(true) >= 3)?3:1);
  kdu_coords ref_subs; codestream.get_subsampling(0,ref_subs,true);
  int c;
  for (c=0; c < num_channels; c++)
    {
      source_components[c] = c;
      default_rendering_precision[c] = codestream.get_bit_depth(c,true);
      default_rendering_signed[c] = codestream.get_signed(c,true);
      kdu_coords subs; codestream.get_subsampling(c,subs,true);
      if (subs != ref_subs)
        break;
    }
  if (c < num_channels)
    num_channels = 1;
  num_colour_channels = num_channels;
  return true;
}

/******************************************************************************/
/*                    kdu_channel_mapping::configure (JPX)                    */
/******************************************************************************/

bool
  kdu_channel_mapping::configure(jp2_colour colr, jp2_channels chnl,
                                 int codestream_idx, jp2_palette plt,
                                 jp2_dimensions codestream_dimensions)
{
  clear();
  if (!colour_converter.init(colr))
    return false;
  set_num_channels(chnl.get_num_colours());
  num_colour_channels = num_channels;
  if (num_channels <= 0)
    { KDU_ERROR(e,0); e <<
        KDU_TXT("JP2 object supplied to "
        "`kdu_channel_mapping::configure' has no colour channels!");
    }

  int c;
  for (c=0; c < num_channels; c++)
    {
      int lut_idx, stream;
      chnl.get_colour_mapping(c,source_components[c],lut_idx,stream);
      if (stream != codestream_idx)
        {
          clear();
          return false;
        }
      if (lut_idx >= 0)
        { // Set up palette lookup table
          int i, num_entries = plt.get_num_entries();
          assert(num_entries <= 1024);
          palette_bits = 1;
          while ((1<<palette_bits) < num_entries)
            palette_bits++;
          assert(palette[c] == NULL);
          palette[c] = new kdu_sample16[(int)(1<<palette_bits)];
          plt.get_lut(lut_idx,palette[c]);
          for (i=num_entries; i < (1<<palette_bits); i++)
            (palette[c])[i] = (palette[c])[num_entries-1];
          palette_bit_depth[c] = plt.get_bit_depth(lut_idx);
          default_rendering_precision[c] = plt.get_bit_depth(lut_idx);
          default_rendering_signed[c] = plt.get_signed(lut_idx);
        }
      else
        {
          palette_bit_depth[c] = 0;
          default_rendering_precision[c] =
            codestream_dimensions.get_bit_depth(source_components[c]);
          default_rendering_signed[c] =
            codestream_dimensions.get_signed(source_components[c]);
        }
    }
  return true;  
}

/******************************************************************************/
/*                    kdu_channel_mapping::configure (jp2)                    */
/******************************************************************************/

bool
  kdu_channel_mapping::configure(jp2_source *jp2_in, bool ignore_alpha)
{
  jp2_channels chnl = jp2_in->access_channels();
  jp2_palette plt = jp2_in->access_palette();
  jp2_colour colr = jp2_in->access_colour();
  jp2_dimensions dims = jp2_in->access_dimensions();
  if (!configure(colr,chnl,0,plt,dims))
    { KDU_ERROR(e,1); e <<
        KDU_TXT("Cannot perform colour conversion from the colour "
        "description embedded in a JP2 (or JP2-compatible) data source, to the "
        "sRGB colour space.  This should not happen with truly JP2-compatible "
        "descriptions.");
    }
  if (!ignore_alpha)
    add_alpha_to_configuration(chnl,0,plt,dims);
  return true;
}

/******************************************************************************/
/*              kdu_channel_mapping::add_alpha_to_configuration               */
/******************************************************************************/

bool
  kdu_channel_mapping::add_alpha_to_configuration(jp2_channels chnl,
                                 int codestream_idx, jp2_palette plt,
                                 jp2_dimensions codestream_dimensions,
                                 bool ignore_premultiplied_alpha)
{
  int scan_channels = chnl.get_num_colours();
  set_num_channels(num_colour_channels);
  int c, alpha_comp_idx=-1, alpha_lut_idx=-1;
  for (c=0; c < scan_channels; c++)
    {
      int lut_idx, tmp_idx, stream;
      if (chnl.get_opacity_mapping(c,tmp_idx,lut_idx,stream) &&
          (stream == codestream_idx))
        { // See if we can find a consistent alpha channel
          if (c == 0)
            { alpha_comp_idx = tmp_idx; alpha_lut_idx = lut_idx; }
          else if ((alpha_comp_idx != tmp_idx) && (alpha_lut_idx != lut_idx))
            alpha_comp_idx = alpha_lut_idx = -1; // Channels use different alpha
        }
      else
        alpha_comp_idx = alpha_lut_idx = -1; // Not all channels have alpha
    }

  if ((alpha_comp_idx < 0) && !ignore_premultiplied_alpha)
    for (c=0; c < scan_channels; c++)
      {
        int lut_idx, tmp_idx, stream;
        if (chnl.get_premult_mapping(c,tmp_idx,lut_idx,stream) &&
            (stream == codestream_idx))
          { // See if we can find a consistent alpha channel
            if (c == 0)
              { alpha_comp_idx = tmp_idx; alpha_lut_idx = lut_idx; }
            else if ((alpha_comp_idx != tmp_idx) && (alpha_lut_idx != lut_idx))
              alpha_comp_idx=alpha_lut_idx=-1; // Channels use different alpha
          }
        else
          alpha_comp_idx = alpha_lut_idx = -1; // Not all channels have alpha
      }

  if (alpha_comp_idx < 0)
    return false;

  set_num_channels(num_colour_channels+1);
  c = num_colour_channels; // Index of entry for the alpha channel
  source_components[c] = alpha_comp_idx;
  if (alpha_lut_idx >= 0)
    {
      int i, num_entries = plt.get_num_entries();
      assert(num_entries <= 1024);
      palette_bits = 1;
      while ((1<<palette_bits) < num_entries)
        palette_bits++;
      assert(palette[c] == NULL);
      palette[c] = new kdu_sample16[(int)(1<<palette_bits)];
      plt.get_lut(alpha_lut_idx,palette[c]);
      for (i=num_entries; i < (1<<palette_bits); i++)
        (palette[c])[i] = (palette[c])[num_entries-1];
      palette_bit_depth[c] = plt.get_bit_depth(alpha_lut_idx);
      default_rendering_precision[c] = plt.get_bit_depth(alpha_lut_idx);
      default_rendering_signed[c] = plt.get_signed(alpha_lut_idx);
    }
  else
    {
      palette_bit_depth[c] = 0;
      default_rendering_precision[c] =
        codestream_dimensions.get_bit_depth(alpha_comp_idx);
      default_rendering_signed[c] =
        codestream_dimensions.get_signed(alpha_comp_idx);
    }
  return true;
}


/* ========================================================================== */
/*                         kdu_region_decompressor                            */
/* ========================================================================== */

/******************************************************************************/
/*             kdu_region_decompressor::kdu_region_decompressor               */
/******************************************************************************/

kdu_region_decompressor::kdu_region_decompressor()
{
  precise = false;
  fastest = false;
  white_stretch_precision = 0;
  zero_overshoot_interp_threshold = 2;
  max_interp_overshoot = 0.4F;
  env=NULL;
  env_queue = NULL;
  next_queue_bank_idx = 0;
  tile_banks = new kdrd_tile_bank[2];
  current_bank = background_bank = NULL;
  codestream_failure = false;
  codestream_failure_exception = KDU_NULL_EXCEPTION;
  discard_levels = 0;
  max_channels = num_channels = num_colour_channels = 0;
  channels = NULL;
  colour_converter = NULL;
  max_components = num_components = 0;
  components = NULL;
  component_indices = NULL;
  max_channel_bufs = num_channel_bufs = 0;
  channel_bufs = NULL;
}

/******************************************************************************/
/*             kdu_region_decompressor::~kdu_region_decompressor              */
/******************************************************************************/

kdu_region_decompressor::~kdu_region_decompressor()
{
  codestream_failure = true; // Force premature termination, if required
  finish();
  if (components != NULL)
    delete[] components;
  if (component_indices != NULL)
    delete[] component_indices;
  if (channels != NULL)
    delete[] channels;
  if (channel_bufs != NULL)
    delete[] channel_bufs;
  if (tile_banks != NULL)
    delete[] tile_banks;
}

/******************************************************************************/
/*                 kdu_region_decompressor::find_render_dims                  */
/******************************************************************************/

kdu_dims
  kdu_region_decompressor::find_render_dims(kdu_dims codestream_dims,
                                kdu_coords ref_comp_subs,
                                kdu_coords ref_comp_expand_numerator,
                                kdu_coords ref_comp_expand_denominator)
{
  kdu_long val, num, den;
  kdu_coords min = codestream_dims.pos;
  kdu_coords lim = min + codestream_dims.size;
  min.x = long_ceil_ratio(min.x,ref_comp_subs.x);
  lim.x = long_ceil_ratio(lim.x,ref_comp_subs.x); 
  min.y = long_ceil_ratio(min.y,ref_comp_subs.y);
  lim.y = long_ceil_ratio(lim.y,ref_comp_subs.y);

  kdu_coords HxD = ref_comp_expand_numerator;
  HxD.x = (HxD.x-1)>>1;  HxD.y = (HxD.y-1)>>1;

  num = ref_comp_expand_numerator.x; den = ref_comp_expand_denominator.x;
  val = num*min.x;  val -= HxD.x;
  min.x = long_ceil_ratio(val,den);
  val = num*lim.x;  val -= HxD.x;
  lim.x = long_ceil_ratio(val,den);

  num = ref_comp_expand_numerator.y; den = ref_comp_expand_denominator.y;
  val = num*min.y;  val -= HxD.y;
  min.y = long_ceil_ratio(val,den);
  val = num*lim.y;  val -= HxD.y;
  lim.y = long_ceil_ratio(val,den);

  kdu_dims render_dims;
  render_dims.pos = min;
  render_dims.size = lim - min;
  
  return render_dims;
}

/******************************************************************************/
/*              kdu_region_decompressor::find_codestream_point                */
/******************************************************************************/

kdu_coords
  kdu_region_decompressor::find_codestream_point(kdu_coords render_point,
                                kdu_coords ref_comp_subs,
                                kdu_coords ref_comp_expand_numerator,
                                kdu_coords ref_comp_expand_denominator)
{
  kdu_long num, den, val;
  kdu_coords result, HxD=ref_comp_expand_numerator;
  HxD.x = (HxD.x-1)>>1;  HxD.y = (HxD.y-1)>>1;
  
  num = ref_comp_expand_numerator.x; den = ref_comp_expand_denominator.x;
  val = den * render_point.x + HxD.x;
  result.x = long_floor_ratio(val,num) * ref_comp_subs.x;
  num = ref_comp_expand_numerator.y; den = ref_comp_expand_denominator.y;
  val = den * render_point.y + HxD.y;
  result.y = long_floor_ratio(val,num) * ref_comp_subs.y;
  
  return result;
}

/******************************************************************************/
/*                kdu_region_decompressor::find_render_point                  */
/******************************************************************************/

kdu_coords
  kdu_region_decompressor::find_render_point(kdu_coords render_point,
                                kdu_coords ref_comp_subs,
                                kdu_coords ref_comp_expand_numerator,
                                kdu_coords ref_comp_expand_denominator)
{
  kdu_long num, den, val;
  kdu_coords result, ref_point, HxD = ref_comp_expand_numerator;
  HxD.x = (HxD.x-1)>>1;  HxD.y = (HxD.y-1)>>1;

  num = render_point.x; den = ref_comp_subs.x; num += num-den; den += den;
  ref_point.x = long_ceil_ratio(num,den);
  num = render_point.y; den = ref_comp_subs.y; num += num-den; den += den;
  ref_point.y = long_ceil_ratio(num,den);

  num = ref_comp_expand_numerator.x; den = ref_comp_expand_denominator.x;
  val = ref_point.x; val *= num; val -= HxD.x; val += val+num; den += den;
  result.x = long_floor_ratio(val,den);
  num = ref_comp_expand_numerator.y; den = ref_comp_expand_denominator.y;
  val = ref_point.y; val *= num; val -= HxD.y; val += val+num; den += den;
  result.y = long_floor_ratio(val,den);
  
  return result;
}  

/******************************************************************************/
/*           kdu_region_decompressor::find_codestream_cover_dims              */
/******************************************************************************/

kdu_dims
  kdu_region_decompressor::find_codestream_cover_dims(kdu_dims render_dims,
                                kdu_coords ref_comp_subs,
                                kdu_coords ref_comp_expand_numerator,
                                kdu_coords ref_comp_expand_denominator)
{
  kdu_long num, num_x2, den, val;
  kdu_coords min, lim, HxD = ref_comp_expand_numerator;
  HxD.x = (HxD.x-1)>>1;  HxD.y = (HxD.y-1)>>1;
  
  min = render_dims.pos; lim = min + render_dims.size;
  
  num = ref_comp_expand_numerator.x; den = ref_comp_expand_denominator.x;
  val = min.x; val *= den; val += HxD.x; val += val-num; num_x2 = num+num;
  min.x = long_ceil_ratio(val,num_x2);
  val = lim.x; val *= den; val += HxD.x; val += val-num;
  lim.x = long_ceil_ratio(val,num_x2);
  
  num = ref_comp_expand_numerator.y; den = ref_comp_expand_denominator.y;
  val = min.y; val *= den; val += HxD.y; val += val-num; num_x2 = num+num;
  min.y = long_ceil_ratio(val,num_x2);
  val = lim.y; val *= den; val += HxD.y; val += val-num;
  lim.y = long_ceil_ratio(val,num_x2);
  
  min.x = min.x * ref_comp_subs.x + 1 - ((ref_comp_subs.x+1)>>1);
  lim.x = lim.x * ref_comp_subs.x + 1 - ((ref_comp_subs.x+1)>>1);
  min.y = min.y * ref_comp_subs.y + 1 - ((ref_comp_subs.y+1)>>1);
  lim.y = lim.y * ref_comp_subs.y + 1 - ((ref_comp_subs.y+1)>>1);
  
  kdu_dims result; result.pos = min;  result.size = lim-min;
  return result;
}  

/******************************************************************************/
/*                 kdu_region_decompressor::set_num_channels                  */
/******************************************************************************/

void
  kdu_region_decompressor::set_num_channels(int num)
{
  if (num > max_channels)
    {
      int new_max_channels = num;
      kdrd_channel *new_channels =
        new kdrd_channel[new_max_channels];
      if (channels != NULL)
        delete[] channels;
      channels = new_channels;      
      max_channels = new_max_channels;
    }
  num_channels = num_colour_channels = num;
  for (int c=0; c < num_channels; c++)
    channels[c].init();
}

/******************************************************************************/
/*                   kdu_region_decompressor::add_component                   */
/******************************************************************************/

kdrd_component *
  kdu_region_decompressor::add_component(int comp_idx)
{
  int n;
  for (n=0; n < num_components; n++)
    if (component_indices[n] == comp_idx)
      return components + n;
  if (num_components == max_components)
    { // Allocate a new array
      int new_max_comps = max_components + num_components + 1; // Grow fast
      kdrd_component *new_comps = new kdrd_component[new_max_comps];
      for (n=0; n < num_components; n++)
        new_comps[n] = components[n];
      if (components != NULL)
        {
          for (int k=0; k < num_channels; k++)
            if (channels[k].source != NULL)
              { // Fix up pointers
                int off = (int)(channels[k].source-components);
                assert((off >= 0) && (off < num_components));
                channels[k].source = new_comps + off;
              }
          delete[] components;
        }
      components = new_comps;

      int *new_indices = new int[new_max_comps];
      for (n=0; n < num_components; n++)
        new_indices[n] = component_indices[n];
      if (component_indices != NULL)
        delete[] component_indices;
      component_indices = new_indices;
    
      max_components = new_max_comps;
    }

  n = num_components++;
  component_indices[n] = comp_idx;
  components[n].init(n);
  return components + n;
}

/******************************************************************************/
/*            kdu_region_decompressor::get_safe_expansion_factors             */
/******************************************************************************/

void
  kdu_region_decompressor::get_safe_expansion_factors(kdu_codestream codestream,
                           kdu_channel_mapping *mapping, int single_component,
                           int discard_levels, double &min_prod,
                           double &max_x, double &max_y,
                           kdu_component_access_mode access_mode)
{
  min_prod = max_x = max_y = 1.0;
  int ref_idx;
  if (mapping == NULL)
    ref_idx = single_component;
  else if (mapping->num_channels > 0)
    ref_idx = mapping->source_components[0];
  else
    return;
  
  codestream.apply_input_restrictions(0,0,discard_levels,0,NULL,access_mode);
  int n=0, comp_idx=ref_idx;
  kdu_coords ref_subs, this_subs;
  codestream.get_subsampling(ref_idx,ref_subs,true);
  double ref_prod = ((double) ref_subs.x) * ((double) ref_subs.y);
  do {
    codestream.get_subsampling(comp_idx,this_subs,true);
    double this_prod = ((double) this_subs.x) * ((double) this_subs.y);
    if ((min_prod*this_prod) > ref_prod)
      min_prod = ref_prod / this_prod;
  } while ((mapping != NULL) && ((++n) < mapping->num_channels) &&
           ((comp_idx=mapping->source_components[n]) >= 0));
  min_prod *= 1.0 / (1 << (KDU_FIX_POINT+9));

  kdu_dims ref_dims;
  codestream.get_dims(ref_idx,ref_dims,true);
  double safe_lim = (double) 0x70000000;
  if (safe_lim > (double) ref_dims.size.x)
    max_x = safe_lim / (double) ref_dims.size.x;
  if (safe_lim > (double) ref_dims.size.y)
    max_y = safe_lim / (double) ref_dims.size.y;
}

/******************************************************************************/
/*              kdu_region_decompressor::get_rendered_image_dims              */
/******************************************************************************/

kdu_dims
  kdu_region_decompressor::get_rendered_image_dims(kdu_codestream codestream,
                           kdu_channel_mapping *mapping, int single_component,
                           int discard_levels, kdu_coords expand_numerator,
                           kdu_coords expand_denominator,
                           kdu_component_access_mode access_mode)
{
  if (this->codestream.exists())
    { KDU_ERROR_DEV(e,2); e <<
        KDU_TXT("The `kdu_region_decompressor::get_rendered_image_dims' "
        "function should not be called with a `codestream' argument between "
        "calls to `kdu_region_decompressor::start' and "
        "`kdu_region_decompressor::finish'."); }
  int ref_idx;
  if (mapping == NULL)
    ref_idx = single_component;
  else if (mapping->num_channels > 0)
    ref_idx = mapping->source_components[0];
  else
    return kdu_dims();

  kdu_dims canvas_dims; codestream.get_dims(-1,canvas_dims,true);
  kdu_coords ref_subs; codestream.get_subsampling(ref_idx,ref_subs,true);
  return find_render_dims(canvas_dims,ref_subs,
                          expand_numerator,expand_denominator);
}

/******************************************************************************/
/*                      kdu_region_decompressor::start                        */
/******************************************************************************/

bool
  kdu_region_decompressor::start(kdu_codestream codestream,
                                 kdu_channel_mapping *mapping,
                                 int single_component, int discard_levels,
                                 int max_layers, kdu_dims region,
                                 kdu_coords expand_numerator,
                                 kdu_coords expand_denominator, bool precise,
                                 kdu_component_access_mode access_mode,
                                 bool fastest, kdu_thread_env *env,
                                 kdu_thread_queue *env_queue)
{
  int c;

  if ((tile_banks[0].num_tiles > 0) || (tile_banks[1].num_tiles > 0))
    { KDU_ERROR_DEV(e,0x10010701); e <<
        KDU_TXT("Attempting to call `kdu_region_decompressor::start' without "
        "first invoking the `kdu_region_decompressor::finish' to finish a "
        "previously installed region.");
    }
  this->precise = precise;
  this->fastest = fastest;
  this->env = env;
  this->env_queue = env_queue;
  this->next_queue_bank_idx = 0;
  this->codestream = codestream;
  codestream_failure = false;
  this->discard_levels = discard_levels;
  num_components = 0;

  colour_converter = NULL;
  full_render_dims.pos = full_render_dims.size = kdu_coords(0,0);

  try { // Protect the following code
      // Set up components and channels.
      if (mapping == NULL)
        {
          set_num_channels(1);
          channels[0].source = add_component(single_component);
          channels[0].lut = NULL;
        }
      else
        {
          if (mapping->num_channels < 1)
            { KDU_ERROR(e,3); e <<
                KDU_TXT("The `kdu_channel_mapping' object supplied to "
                "`kdu_region_decompressor::start' does not define any channels "
                "at all.");
            }
          set_num_channels(mapping->num_channels);
          num_colour_channels = mapping->num_colour_channels;
          if (num_colour_channels > num_channels)
            { KDU_ERROR(e,4); e <<
                KDU_TXT("The `kdu_channel_mapping' object supplied to "
                "`kdu_region_decompressor::start' specifies more colour "
                "channels than the total number of channels.");
            }
          colour_converter = mapping->get_colour_converter();
          if (!(colour_converter->exists() &&
                colour_converter->is_non_trivial()))
            colour_converter = NULL;
          for (c=0; c < mapping->num_channels; c++)
            {
              channels[c].source = add_component(mapping->source_components[c]);
              channels[c].lut =
                (mapping->palette_bits <= 0)?NULL:mapping->palette[c];
              if (channels[c].lut != NULL)
                channels[c].source->palette_bits = mapping->palette_bits;
              channels[c].native_precision =
                mapping->default_rendering_precision[c];
              channels[c].native_signed =
                mapping->default_rendering_signed[c];
            }
        }

      // Configure component sampling and data representation information.
      if ((expand_denominator.x < 1) || (expand_denominator.y < 1))
        { KDU_ERROR_DEV(e,5); e <<
            KDU_TXT("Invalid expansion ratio supplied to "
            "`kdu_region_decompressor::start'.  The numerator and denominator "
            "terms expressed by the `expand_numerator' and "
            "`expand_denominator' arguments must be strictly positive.");
        }

      codestream.apply_input_restrictions(num_components,component_indices,
                                          discard_levels,max_layers,NULL,
                                          access_mode);
      for (c=0; c < num_components; c++)
        {
          kdrd_component *comp = components + c;
          comp->bit_depth = codestream.get_bit_depth(comp->rel_comp_idx,true);
          comp->is_signed = codestream.get_signed(comp->rel_comp_idx,true);
          comp->num_line_users = 0; // Adjust when scanning the channels below
        }
          
      // Configure the channels for precision and white stretch
      for (c=0; c < num_channels; c++)
        {
          if (channels[c].native_precision == 0)
            {
              channels[c].native_precision = channels[c].source->bit_depth;
              channels[c].native_signed = channels[c].source->is_signed;
            }
          channels[c].stretch_residual = 0;
          channels[c].stretch_source_factor = 1.0F;
          if ((channels[c].lut != NULL) && (mapping->palette_bit_depth[c]<=0))
            continue; // Can't reliably stretch palette outputs, since
                      // we can't be sure of the palette output precision here.
          channels[c].source->num_line_users++;
          int B = (white_stretch_precision>16)?16:white_stretch_precision;
          int P = channels[c].source->bit_depth;
          if (channels[c].lut != NULL)
            P = mapping->palette_bit_depth[c];
          if (B > P)
            { /* Evaluate:  2^16 * [(1-2^{-B}) / (1-2^{-P}) - 1]
                            = 2^16 * (2^{-P} - 2^{-B}) / (1-2^{-P})
                            = 2^16 * (2^{B-P} - 1) / (2^B - 2^{B-P}} */
              int num_val = ((1<<(B-P)) - 1) << 16;
              int den_val = (1<<B) - (1<<(B-P));
              channels[c].stretch_residual = (kdu_uint16)(num_val/den_val);
              P = B; // B is new precision after white stretch
            }
          if (P < 31)
            channels[c].stretch_source_factor = 1.0F / (1.0F - (1.0F/(1<<P)));
        }
    
      // Find sampling parameters
      kdrd_component *ref_comp = channels[0].source;
      kdu_coords ref_subs, this_subs;
      codestream.get_subsampling(ref_comp->rel_comp_idx,ref_subs,true);
      for (c=0; c < num_channels; c++)
        {
          kdrd_component *comp = channels[c].source;
          codestream.get_subsampling(comp->rel_comp_idx,this_subs,true);

          kdu_long num_x, num_y, den_x, den_y;
          num_x = ((kdu_long) expand_numerator.x ) * ((kdu_long) this_subs.x);
          den_x = ((kdu_long) expand_denominator.x ) * ((kdu_long) ref_subs.x);
          num_y = ((kdu_long) expand_numerator.y) * ((kdu_long) this_subs.y);
          den_y = ((kdu_long) expand_denominator.y) * ((kdu_long) ref_subs.y);
          
          if (num_x != den_x)
            while ((num_x < 32) && (den_x < (1<<30)))
              { num_x+=num_x; den_x+=den_x; }
          if (num_y != den_y)
            while ((num_y < 32) && (den_y < (1<<30)))
              { num_y+=num_y; den_y+=den_y; }
          
          kdu_coords boxcar_radix;
          while (den_x > (3*num_x))
            { boxcar_radix.x++; num_x+=num_x; }
          while (den_y > (3*num_y))
            { boxcar_radix.y++; num_y+=num_y; }
          
          if (num_x == den_x)
            num_x = den_x = 1;
          if (num_y == den_y)
            num_y = den_y = 1;
          
          if (!(reduce_ratio_to_ints(num_x,den_x) &&
                reduce_ratio_to_ints(num_y,den_y)))
            { KDU_ERROR_DEV(e,7); e <<
                KDU_TXT("Unable to represent all component "
                "expansion factors as rational numbers whose numerator and "
                "denominator can both be expressed as 32-bit signed "
                "integers.  This is a very unusual condition.");
            }
          kdu_coords phase_shift;
          while ((((kdu_long) 1)<<(phase_shift.x+6)) < num_x)
            phase_shift.x++;
          while ((((kdu_long) 1)<<(phase_shift.y+6)) < num_y)
            phase_shift.y++;
          assert((((num_x-1) >> phase_shift.x) < 64) &&
                 (((num_y-1) >> phase_shift.y) < 64));
          
          channels[c].boxcar_log_size = boxcar_radix.x+boxcar_radix.y;
          if (channels[c].boxcar_log_size > (KDU_FIX_POINT+9))
            { KDU_ERROR_DEV(e,0x15090901); e <<
              KDU_TXT("The `expand_numerator' and `expand_denominator' "
                      "parameters supplied to `kdu_region_decompressor::start' "
                      "represent a truly massive reduction in resolution "
                      "through subsampling (on the order of many millions).  "
                      "Apart from being quite impractical, such large "
                      "subsampling factors violate internal implementation "
                      "requirements.");
            }
          channels[c].boxcar_size.x = (1 << boxcar_radix.x);
          channels[c].boxcar_size.y = (1 << boxcar_radix.y);
          channels[c].sampling_numerator.x = (int) den_x;
          channels[c].sampling_denominator.x = (int) num_x;
          channels[c].sampling_numerator.y = (int) den_y;
          channels[c].sampling_denominator.y = (int) num_y;
          codestream.get_relative_registration(comp->rel_comp_idx,
                                               ref_comp->rel_comp_idx,
                                               channels[c].sampling_denominator,
                                               channels[c].source_alignment,
                                               true);
          channels[c].source_alignment.x =
            (channels[c].source_alignment.x +
             ((1<<boxcar_radix.x)>>1)) >> boxcar_radix.x;
          channels[c].source_alignment.y =
            (channels[c].source_alignment.y +
             ((1<<boxcar_radix.y)>>1)) >> boxcar_radix.y;
          channels[c].sampling_phase_shift = phase_shift;
          
          // Now generate interpolation kernels
          float ratio, thld=(float)zero_overshoot_interp_threshold;
          if (channels[c].sampling_denominator.x !=
              channels[c].sampling_numerator.x) 
            {
              ratio = (((float) channels[c].sampling_denominator.x) /
                       ((float) channels[c].sampling_numerator.x));
            if ((c == 0) ||
                !channels[c].h_kernels.copy(channels[c-1].h_kernels,ratio,
                                            max_interp_overshoot,thld))
              channels[c].h_kernels.init(ratio,max_interp_overshoot,thld);
            }
          if (channels[c].sampling_denominator.y !=
              channels[c].sampling_numerator.y) 
            {
              ratio = (((float) channels[c].sampling_denominator.y) /
                       ((float) channels[c].sampling_numerator.y));
              if ((!channels[c].v_kernels.copy(channels[c].h_kernels,ratio,
                                               max_interp_overshoot,thld)) &&
                  ((c == 0) ||
                   !channels[c].v_kernels.copy(channels[c-1].v_kernels,ratio,
                                               max_interp_overshoot,thld)))
                channels[c].v_kernels.init(ratio,max_interp_overshoot,thld);
            }
          
          // Finally we can fill out the phase tables
          int i;
          for (i=0; i < 65; i++)
            {
              double sigma;
              int min_phase, max_phase;
              
              min_phase = (i<<phase_shift.x)-((1<<phase_shift.x)>>1);
              max_phase = ((i+1)<<phase_shift.x)-1-((1<<phase_shift.x)>>1);
              if (min_phase < 0)
                min_phase = 0;
              if (max_phase >= channels[c].sampling_denominator.x)
                max_phase = channels[c].sampling_denominator.x-1;
              sigma = (((double)(min_phase+max_phase)) /
                       (2.0*channels[c].sampling_denominator.x));
              channels[c].horz_phase_table[i] = (kdu_uint16)(sigma*32+0.5);
              if (channels[c].horz_phase_table[i] > 32)
                channels[c].horz_phase_table[i] = 32;
              
              min_phase = (i<<phase_shift.y)-((1<<phase_shift.y)>>1);
              max_phase = ((i+1)<<phase_shift.y)-1-((1<<phase_shift.y)>>1);
              if (min_phase < 0)
                min_phase = 0;
              if (max_phase >= channels[c].sampling_denominator.y)
                max_phase = channels[c].sampling_denominator.y-1;
              sigma = (((double)(min_phase+max_phase)) /
                       (2.0*channels[c].sampling_denominator.y));
              channels[c].vert_phase_table[i] = (kdu_uint16)(sigma*32+0.5);
              if (channels[c].vert_phase_table[i] > 32)
                channels[c].vert_phase_table[i] = 32;
            }
        }

      // Apply level/layer restrictions and find `full_render_dims'
      codestream.apply_input_restrictions(num_components,component_indices,
                                          discard_levels,max_layers,NULL,
                                          access_mode);
      kdu_dims canvas_dims; codestream.get_dims(-1,canvas_dims,true);
      full_render_dims =
        find_render_dims(canvas_dims,ref_subs,
                         expand_numerator,expand_denominator);
      if ((full_render_dims & region) != region)
        { KDU_ERROR_DEV(e,9); e <<
            KDU_TXT("The `region' passed into "
            "`kdu_region_decompressor::start' does not lie fully within the "
            "region occupied by the full image on the rendering canvas.  The "
            "error is probably connected with a misunderstanding of the "
            "way in which codestream dimensions are mapped to rendering "
            "coordinates through the rational upsampling process offered by "
            "the `kdu_region_decompressor' object.  It is best to use "
            "`kdu_region_decompressor::get_rendered_image_dims' to find the "
            "full image dimensions on the rendering canvas.");
        }
      this->original_expand_numerator = expand_numerator;
      this->original_expand_denominator = expand_denominator;

      // Find a region on the code-stream canvas which provides sufficient
      // coverage for all image components, taking into account interpolation
      // kernel supports and boxcar integration processes.
      kdu_dims canvas_region =
        find_canvas_cover_dims(region,codestream,channels,num_channels,false);
      codestream.apply_input_restrictions(num_components,component_indices,
                                          discard_levels,max_layers,
                                          &canvas_region,access_mode);

      // Prepare for processing tiles within the region.
      codestream.get_valid_tiles(valid_tiles);
      next_tile_idx = valid_tiles.pos;
    }
  catch (kdu_exception exc) {
      codestream_failure_exception = exc;
      if (env != NULL)
        env->handle_exception(exc);
      finish();
      return false;
    }
  catch (std::bad_alloc &) {
      codestream_failure_exception = KDU_MEMORY_EXCEPTION;
      if (env != NULL)
        env->handle_exception(KDU_MEMORY_EXCEPTION);
      finish();
      return false;
    }  
  return true;
}

/******************************************************************************/
/*                 kdu_region_decompressor::start_tile_bank                   */
/******************************************************************************/

bool
  kdu_region_decompressor::start_tile_bank(kdrd_tile_bank *bank,
                                           kdu_long suggested_tile_mem,
                                           kdu_dims incomplete_region)
{
  assert(bank->num_tiles == 0); // Make sure it is not currently open
  bank->env_queue = NULL;
  bank->queue_bank_idx = 0;
  bank->freshly_created = true;

  // Start by finding out how many tiles will be in the bank, and the region
  // they occupy on the canvas
  kdrd_component *ref_comp = channels[0].source;
  if (suggested_tile_mem < 1)
    suggested_tile_mem = 1;
  kdu_dims canvas_cover_dims =
    find_canvas_cover_dims(incomplete_region,codestream,
                           channels,num_channels,true);
  int num_tiles=0, mem_height=100;
  kdu_long half_suggested_tile_mem = suggested_tile_mem >> 1;
  kdu_coords idx;
  int tiles_left_on_row = valid_tiles.pos.x+valid_tiles.size.x-next_tile_idx.x;
  while (((next_tile_idx.y-valid_tiles.pos.y) < valid_tiles.size.y) &&
         ((next_tile_idx.x-valid_tiles.pos.x) < valid_tiles.size.x) &&
         (suggested_tile_mem > 0))
    {
      idx = next_tile_idx;   next_tile_idx.x++;
      kdu_dims full_dims, dims;
      codestream.get_tile_dims(idx,-1,full_dims,true);
      if (!full_dims.intersects(canvas_cover_dims))
        { // Discard tile immediately
          kdu_tile tt=codestream.open_tile(idx,env);
          if (tt.exists()) tt.close(env);
          continue;
        }
      codestream.get_tile_dims(idx,ref_comp->rel_comp_idx,dims,true);
      if (num_tiles == 0)
        {
          bank->dims = dims;
          bank->first_tile_idx = idx;
        }
      else
        bank->dims.size.x += dims.size.x;
      if (dims.size.y < mem_height)
        mem_height = dims.size.y;
      suggested_tile_mem -= dims.size.x * mem_height;
      num_tiles++;
      tiles_left_on_row--;
      if (tiles_left_on_row < num_tiles)
        { // We seem to be more than half way through
          if ((suggested_tile_mem < half_suggested_tile_mem) &&
              (tiles_left_on_row > 2))
            break; // Come back and finish the remaining tiles next time
          if (tiles_left_on_row <= 2)
            suggested_tile_mem = 1; // Make sure we include these tiles
        }
    }

  if ((next_tile_idx.x-valid_tiles.pos.x) == valid_tiles.size.x)
    { // Advance `next_tile_idx' to the start of the next row of tiles
      next_tile_idx.x = valid_tiles.pos.x;
      next_tile_idx.y++;
    }

  if (num_tiles == 0)
    return true; // Nothing to do; must have discarded some tiles

  // Allocate the necessary storage in the `kdrd_tile_bank' object
  if (num_tiles > bank->max_tiles)
    {
      if (bank->tiles != NULL)
        { delete[] bank->tiles; bank->tiles = NULL; }
      if (bank->engines != NULL)
        { delete[] bank->engines; bank->engines = NULL; }
      bank->max_tiles = num_tiles;
      bank->tiles = new kdu_tile[bank->max_tiles];
      bank->engines = new kdu_multi_synthesis[bank->max_tiles];
    }
  bank->num_tiles = num_tiles; // Only now is it completely safe to record
                      // `num_tiles' in `bank' -- in case of thrown exceptions

  // Now open all the tiles, checking whether or not an error will occur
  int tnum;
  for (idx=bank->first_tile_idx, tnum=0; tnum < num_tiles; tnum++, idx.x++)
    bank->tiles[tnum] = codestream.open_tile(idx,env);
  if ((codestream.get_min_dwt_levels() < discard_levels) ||
      !codestream.can_flip(true))
    {
      for (tnum=0; tnum < num_tiles; tnum++)
        bank->tiles[tnum].close(env);
      bank->num_tiles = 0;
      return false;
    }
  
  // Create processing queue (if required) and all tile processing engines
  if (env != NULL)
    bank->env_queue =
      env->add_queue(NULL,env_queue,"`kdu_region_decompressor' tile bank",
                     (bank->queue_bank_idx=next_queue_bank_idx++));
  int processing_stripe_height = 1;
  bool double_buffering = false;
  if ((env != NULL) && (bank->dims.size.y >= 64))
    { // Assign a heuristic double buffering height
      double_buffering = true;
      processing_stripe_height = 32;
    }
  for (tnum=0; tnum < num_tiles; tnum++)
    bank->engines[tnum].create(codestream,bank->tiles[tnum],precise,false,
                               fastest,processing_stripe_height,
                               env,bank->env_queue,double_buffering);
  return true;
}

/******************************************************************************/
/*                  kdu_region_decompressor::close_tile_bank                  */
/******************************************************************************/

void
  kdu_region_decompressor::close_tile_bank(kdrd_tile_bank *bank)
{
  if (bank->num_tiles == 0)
    return;

  int tnum;
  if ((env != NULL) && (bank->env_queue != NULL))
    env->terminate(bank->env_queue,false);
  bank->env_queue = NULL;
  for (tnum=0; tnum < bank->num_tiles; tnum++)
    if ((!codestream_failure) && bank->tiles[tnum].exists())
      {
        try {
            bank->tiles[tnum].close(env);
          }
      catch (kdu_exception exc) {
          codestream_failure = true;
          codestream_failure_exception = exc;
          if (env != NULL)
            env->handle_exception(exc);
        }
      catch (std::bad_alloc &) {
          codestream_failure = true;
          codestream_failure_exception = KDU_MEMORY_EXCEPTION;
          if (env != NULL)
            env->handle_exception(KDU_MEMORY_EXCEPTION);
        }
      }
  for (tnum=0; tnum < bank->num_tiles; tnum++)
    if (bank->engines[tnum].exists())
      bank->engines[tnum].destroy();
  bank->num_tiles = 0;
}

/******************************************************************************/
/*             kdu_region_decompressor::make_tile_bank_current                */
/******************************************************************************/

void
  kdu_region_decompressor::make_tile_bank_current(kdrd_tile_bank *bank,
                                                  kdu_dims incomplete_region)
{
  assert(bank->num_tiles > 0);
  current_bank = bank;

  // Establish rendering dimensions for the current tile bank.  The mapping
  // between rendering dimensions and actual code-stream dimensions is
  // invariably based upon the first channel as the reference component.
  render_dims = find_render_dims(bank->dims,kdu_coords(1,1),
                                 original_expand_numerator,
                                 original_expand_denominator);
  render_dims &= incomplete_region;

  // Fill in tile-bank specific fields for each component and pre-allocate
  // component line buffers.
  int c, w;
  aux_allocator.restart();
  for (c=0; c < num_components; c++)
    {
      kdrd_component *comp = components + c;
      codestream.get_tile_dims(bank->first_tile_idx,
                               comp->rel_comp_idx,comp->dims,true);
      if (bank->num_tiles > 1)
        {
          kdu_coords last_tile_idx = bank->first_tile_idx;
          last_tile_idx.x += bank->num_tiles-1;
          kdu_dims last_tile_dims;
          codestream.get_tile_dims(last_tile_idx,
                                   comp->rel_comp_idx,last_tile_dims,true);
          assert((last_tile_dims.pos.y == comp->dims.pos.y) &&
                 (last_tile_dims.size.y == comp->dims.size.y));
          comp->dims.size.x =
            last_tile_dims.pos.x+last_tile_dims.size.x-comp->dims.pos.x;
        }
      comp->new_line_samples = 0;
      comp->needed_line_samples = comp->dims.size.x;
      comp->num_tiles = 0;
      comp->have_fix16 = comp->have_compatible16 = comp->have_floats = false;
      if ((comp->dims.size.y > 0) && (comp->num_line_users > 0))
        {
          comp->num_tiles = bank->num_tiles;
          if (comp->num_tiles > comp->max_tiles)
            {
              comp->max_tiles = comp->num_tiles;
              if (comp->tile_lines != NULL)
                { delete[] comp->tile_lines; comp->tile_lines = NULL; }
              comp->tile_lines = new kdu_line_buf *[comp->max_tiles];
              for (w=0; w < comp->max_tiles; w++)
                comp->tile_lines[w] = NULL;
            }
          for (w=0; w < bank->num_tiles; w++)
            if (bank->engines[w].is_line_precise(comp->rel_comp_idx))
              {
                if (!bank->engines[w].is_line_absolute(comp->rel_comp_idx))
                  comp->have_floats = true;
              }
            else if (!bank->engines[w].is_line_absolute(comp->rel_comp_idx))
              comp->have_fix16 = true;
            else if (comp->bit_depth <= KDU_FIX_POINT)
              comp->have_compatible16 = true;
        }
      comp->indices.destroy(); // Works even if never allocated
      if ((comp->palette_bits > 0) || (comp->num_tiles == 0))
        comp->indices.pre_create(&aux_allocator,comp->dims.size.x,
                                 true,true,0,0);
    }
  
  // Now fill in the tile-bank specific state variables for each channel,
  // pre-allocating channel line buffer resources.
  for (c=0; c < num_channels; c++)
    {
      kdrd_channel *chan = channels + c;
      kdrd_component *comp = chan->source;
      chan->using_shorts = true;
      chan->using_floats = false;
      if ((colour_converter == NULL) && (chan->lut == NULL) &&
          (chan->stretch_residual == 0) &&
          !(comp->have_fix16 || comp->have_compatible16))
        {
          chan->using_shorts = false;
          chan->using_floats = true;
          if ((chan->sampling_numerator == chan->sampling_denominator) &&
              !comp->have_floats)
            chan->using_floats = false;
        }
      
      if (chan->using_floats)
        chan->in_precision = 0;
      else if (chan->using_shorts)
        {
          chan->in_precision = KDU_FIX_POINT + chan->boxcar_log_size;
          if (chan->in_precision > (16+KDU_FIX_POINT))
            chan->in_precision = 16+KDU_FIX_POINT;          
        }
      else
        chan->in_precision = comp->bit_depth;

      // Calculate the minimum and maximum indices for boxcar cells
      // (same as image component samples if there is no boxcar accumulation)
      // associated with `render_dims', along with the sampling phase
      // for the upper left hand corner of `render_dims'.
      kdu_long val, num, den, aln;
      kdu_coords min = render_dims.pos;
      kdu_coords max = min + render_dims.size - kdu_coords(1,1);
      
      num = chan->sampling_numerator.x;
      den = chan->sampling_denominator.x;
      aln = chan->source_alignment.x;
      aln += ((chan->boxcar_size.x-1)*den) / (2*chan->boxcar_size.x);
      min.x = long_floor_ratio((val=num*min.x-aln),den);
      chan->sampling_phase.x = (int)(val-min.x*den);
      max.x = long_floor_ratio(num*max.x-aln,den);

      num = chan->sampling_numerator.y;
      den = chan->sampling_denominator.y;
      aln = chan->source_alignment.y;
      aln += ((chan->boxcar_size.y-1)*den) / (2*chan->boxcar_size.y);
      min.y = long_floor_ratio((val=num*min.y-aln),den); // Rounding
      chan->sampling_phase.y = (int)(val-min.y*den);
      max.y = long_floor_ratio(num*max.y-aln,den);
      
      chan->in_line_start = 0;
      chan->in_line_length = 1+max.x-min.x;
      chan->out_line_length = render_dims.size.x;
      if (chan->sampling_numerator.x != chan->sampling_denominator.x)
        { // Allow for 5-tap interpolation kernels
          min.x -= 2;
          chan->in_line_start = -2;
          chan->in_line_length += 5;
        }
      if (chan->sampling_numerator.y != chan->sampling_denominator.y)
        min.y -= 2;

      chan->missing.x = comp->dims.pos.x - min.x*chan->boxcar_size.x;
      chan->missing.y = comp->dims.pos.y - min.y*chan->boxcar_size.y;
      
      chan->boxcar_lines_left = chan->boxcar_size.y;
      
      chan->num_valid_vlines = 0;
      chan->in_line = chan->horz_line = chan->out_line = NULL;
      for (w=0; w < KDRD_CHANNEL_VLINES; w++)
        chan->vlines[w] = NULL;
      chan->line_bufs_used = 0;
      int line_buf_width = chan->in_line_length+chan->in_line_start;
      int line_buf_lead = -chan->in_line_start;
      int min_line_buf_width = line_buf_width;
      int min_line_buf_lead = line_buf_lead;
#ifdef KDU_SIMD_OPTIMIZATIONS
      if (chan->sampling_numerator.x != chan->sampling_denominator.x)
        {
          if (chan->using_floats)
            { min_line_buf_lead += 3; min_line_buf_width += 9; }
          else
            { min_line_buf_lead += 7; min_line_buf_width += 21; }
        }
#endif
      if ((chan->in_precision > KDU_FIX_POINT) && chan->using_shorts)
        line_buf_width += line_buf_width + line_buf_lead;
      if (line_buf_width < chan->out_line_length)
        line_buf_width = chan->out_line_length;
      if (line_buf_lead < min_line_buf_lead)
        line_buf_lead = min_line_buf_lead;
      if (line_buf_width < min_line_buf_width)
        line_buf_width = min_line_buf_width;
      for (w=0; w < KDRD_CHANNEL_LINE_BUFS; w++)
        chan->line_bufs[w].destroy();
      for (w=0; w < KDRD_CHANNEL_LINE_BUFS; w++)
        chan->line_bufs[w].pre_create(&aux_allocator,line_buf_width,
                                      !(chan->using_shorts||chan->using_floats),
                                      chan->using_shorts,line_buf_lead,0);
      
      chan->can_use_component_samples_directly = (chan->lut == NULL) &&
        (comp->num_tiles==1) && (chan->missing.x == 0) &&
        (chan->sampling_numerator == chan->sampling_denominator) &&
        (chan->out_line_length <= comp->dims.size.x) &&
        (chan->using_shorts == comp->have_fix16) &&
        (chan->using_floats == comp->have_floats);
    }

  // Perform final resource allocation
  aux_allocator.finalize();
  for (c=0; c < num_components; c++)
    {
      kdrd_component *comp = components + c;
      if ((comp->palette_bits > 0) || (comp->num_tiles == 0))
        {
          comp->indices.create();
          if (comp->dims.size.y == 0)
            reset_line_buf(&(comp->indices));
        }
    }
  for (c=0; c < num_channels; c++)
    {
      kdrd_channel *chan = channels + c;
      for (w=0; w < KDRD_CHANNEL_LINE_BUFS; w++)
        {
          chan->line_bufs[w].create();
#ifdef KDU_SIMD_OPTIMIZATIONS
          if (chan->using_floats &&
              (chan->sampling_numerator != chan->sampling_denominator))
            { // Initialize floats, so extensions don't create exceptions
              kdu_sample32 *sp = chan->line_bufs[w].get_buf32();
              int width = chan->line_bufs[w].get_width();
              width += (4-width)&3; // Round to a multiple of 4
              if (chan->sampling_numerator.x != chan->sampling_denominator.x)
                { sp -= 6; width += 6; }
              for (; width > 0; width--, sp++)
                sp->fval = 0.0F;
            }
#endif // KDU_SIMD_OPTIMIZATIONS
        }
    }
  
  // Fill out the interpolation kernel lookup tables
  for (c=0; c < num_channels; c++)
    {
      kdrd_channel *chan = channels + c;
      if (chan->sampling_numerator.x != chan->sampling_denominator.x)
        for (w=0; w < 65; w++)
          {
            int sigma_x32 = chan->horz_phase_table[w];
            assert(sigma_x32 <= 32);
#ifdef KDU_SIMD_OPTIMIZATIONS
            if (chan->using_floats)
              chan->simd_horz_interp_kernels[w] =
                chan->h_kernels.get_simd_kernel(KDRD_SIMD_KERNEL_HORZ_FLOATS,
                                                sigma_x32);
            else
              chan->simd_horz_interp_kernels[w] =
                chan->h_kernels.get_simd_kernel(KDRD_SIMD_KERNEL_HORZ_FIX16,
                                                sigma_x32);
#endif // KDU_SIMD_OPTIMIZATIONS
            int idx = sigma_x32 * KDRD_INTERP_KERNEL_STRIDE;
            if (chan->using_floats)
              chan->horz_interp_kernels[w] = chan->h_kernels.float_kernels+idx;
            else
              chan->horz_interp_kernels[w] = chan->h_kernels.fix16_kernels+idx;
          }
          
      if (chan->sampling_numerator.y != chan->sampling_denominator.y)
        for (w=0; w < 65; w++)
          {
            int sigma_x32 = chan->vert_phase_table[w];
            assert(sigma_x32 <= 32);
#ifdef KDU_SIMD_OPTIMIZATIONS
            if (chan->using_floats)
              chan->simd_vert_interp_kernels[w] =
                chan->v_kernels.get_simd_kernel(KDRD_SIMD_KERNEL_VERT_FLOATS,
                                                sigma_x32);
            else
              chan->simd_vert_interp_kernels[w] =
                chan->v_kernels.get_simd_kernel(KDRD_SIMD_KERNEL_VERT_FIX16,
                                                sigma_x32);
#endif // KDU_SIMD_OPTIMIZATIONS
            int idx = sigma_x32 * KDRD_INTERP_KERNEL_STRIDE;
            if (chan->using_floats)
              chan->vert_interp_kernels[w] = chan->v_kernels.float_kernels+idx;
            else
              chan->vert_interp_kernels[w] = chan->v_kernels.fix16_kernels+idx;
          }
    }
}

/******************************************************************************/
/*                       kdu_region_decompressor::finish                      */
/******************************************************************************/

bool
  kdu_region_decompressor::finish(kdu_exception *exc)
{
  bool success;
  int b, c;

  // Make sure all tile banks are closed and their queues terminated
  if (current_bank != NULL)
    { // Make sure we close this one first, in case there are two open banks
      close_tile_bank(current_bank);
    }
  if (tile_banks != NULL)
    for (b=0; b < 2; b++)
      close_tile_bank(tile_banks+b);
  current_bank = background_bank = NULL;

  success = !codestream_failure;
  if ((exc != NULL) && !success)
    *exc = codestream_failure_exception;
  codestream_failure = false;
  env = NULL;
  env_queue = NULL;

  // Reset any resource pointers which may no longer be valid
  for (c=0; c < num_components; c++)
    components[c].init(0);
  for (c=0; c < num_channels; c++)
    channels[c].init();
  codestream = kdu_codestream(); // Invalidate the internal pointer for safety
  aux_allocator.restart(); // Get ready to use the allocation object again.
  full_render_dims.pos = full_render_dims.size = kdu_coords(0,0);
  num_components = num_channels = 0;
  return success;
}

/******************************************************************************/
/*                  kdu_region_decompressor::process (bytes)                  */
/******************************************************************************/

bool
  kdu_region_decompressor::process(kdu_byte **chan_bufs,
                                   bool expand_monochrome,
                                   int pixel_gap, kdu_coords buffer_origin,
                                   int row_gap, int suggested_increment,
                                   int max_region_pixels,
                                   kdu_dims &incomplete_region,
                                   kdu_dims &new_region, int precision_bits,
                                   bool measure_row_gap_in_pixels)
{
  if (num_colour_channels != 1)
    expand_monochrome = false;
  num_channel_bufs = num_channels + ((expand_monochrome)?2:0);
  if (num_channel_bufs > max_channel_bufs)
    {
      max_channel_bufs = num_channel_bufs;
      if (channel_bufs != NULL)
        { delete[] channel_bufs; channel_bufs = NULL; }
      channel_bufs = new kdu_byte *[max_channel_bufs];
    }
  for (int c=0; c < num_channel_bufs; c++)
    channel_bufs[c] = chan_bufs[c];
  if (measure_row_gap_in_pixels)
    row_gap *= pixel_gap;
  return process_generic(1,pixel_gap,buffer_origin,row_gap,suggested_increment,
                         max_region_pixels,incomplete_region,new_region,
                         precision_bits);
}

/******************************************************************************/
/*                  kdu_region_decompressor::process (words)                  */
/******************************************************************************/

bool
  kdu_region_decompressor::process(kdu_uint16 **chan_bufs,
                                   bool expand_monochrome,
                                   int pixel_gap, kdu_coords buffer_origin,
                                   int row_gap, int suggested_increment,
                                   int max_region_pixels,
                                   kdu_dims &incomplete_region,
                                   kdu_dims &new_region, int precision_bits,
                                   bool measure_row_gap_in_pixels)
{
  if (num_colour_channels != 1)
    expand_monochrome = false;
  num_channel_bufs = num_channels + ((expand_monochrome)?2:0);
  if (num_channel_bufs > max_channel_bufs)
    {
      max_channel_bufs = num_channel_bufs;
      if (channel_bufs != NULL)
        { delete[] channel_bufs; channel_bufs = NULL; }
      channel_bufs = new kdu_byte *[max_channel_bufs];
    }
  for (int c=0; c < num_channel_bufs; c++)
    channel_bufs[c] = (kdu_byte *) chan_bufs[c];
  if (measure_row_gap_in_pixels)
    row_gap *= pixel_gap;
  return process_generic(2,pixel_gap,buffer_origin,row_gap,suggested_increment,
                         max_region_pixels,incomplete_region,new_region,
                         precision_bits);
}

/******************************************************************************/
/*                 kdu_region_decompressor::process (floats)                  */
/******************************************************************************/

bool 
  kdu_region_decompressor::process(float **chan_bufs, bool expand_monochrome,
                                   int pixel_gap, kdu_coords buffer_origin,
                                   int row_gap, int suggested_increment,
                                   int max_region_pixels,
                                   kdu_dims &incomplete_region,
                                   kdu_dims &new_region,
                                   bool normalize,
                                   bool measure_row_gap_in_pixels)
{
  if (num_colour_channels != 1)
    expand_monochrome = false;
  num_channel_bufs = num_channels + ((expand_monochrome)?2:0);
  if (num_channel_bufs > max_channel_bufs)
    {
      max_channel_bufs = num_channel_bufs;
      if (channel_bufs != NULL)
        { delete[] channel_bufs; channel_bufs = NULL; }
      channel_bufs = new kdu_byte *[max_channel_bufs];
    }
  for (int c=0; c < num_channel_bufs; c++)
    channel_bufs[c] = (kdu_byte *)(chan_bufs[c]);
  if (measure_row_gap_in_pixels)
    row_gap *= pixel_gap;
  return process_generic(4,pixel_gap,buffer_origin,row_gap,suggested_increment,
                         max_region_pixels,incomplete_region,new_region,
                         (normalize)?1:0);
}

/******************************************************************************/
/*                 kdu_region_decompressor::process (packed)                  */
/******************************************************************************/

bool
  kdu_region_decompressor::process(kdu_int32 *buffer,
                                   kdu_coords buffer_origin,
                                   int row_gap, int suggested_increment,
                                   int max_region_pixels,
                                   kdu_dims &incomplete_region,
                                   kdu_dims &new_region)
{
  if (num_colour_channels == 2)
    { KDU_ERROR_DEV(e,0x12060600); e <<
        KDU_TXT("The convenient, packed 32-bit integer version of "
        "`kdu_region_decompressor::process' may not be used if the number "
        "of colour channels equals 2.");
    }

  num_channel_bufs = num_colour_channels + 1;
  if (num_colour_channels == 1)
    num_channel_bufs += 2;

  if (max_channel_bufs < num_channel_bufs)
    {
      max_channel_bufs = num_channel_bufs;
      if (channel_bufs != NULL)
        { delete[] channel_bufs; channel_bufs = NULL; }
      channel_bufs = new kdu_byte *[max_channel_bufs];
    }
  kdu_byte *buf = (kdu_byte *) buffer;
  int c, test_endian = 1;
  if (((kdu_byte *) &test_endian)[0] == 0)
    { // Big-endian byte order
      channel_bufs[0] = buf+1;
      channel_bufs[1] = buf+2;
      channel_bufs[2] = buf+3;
      for (c=3; c < num_colour_channels; c++)
        channel_bufs[c] = NULL;
      channel_bufs[c++] = buf+0;
    }
  else
    { // Little-endian byte order
      channel_bufs[0] = buf+2;
      channel_bufs[1] = buf+1;
      channel_bufs[2] = buf+0;
      for (c=3; c < num_colour_channels; c++)
        channel_bufs[c] = NULL;
      channel_bufs[c++] = buf+3;
    }
  assert(c <= num_channel_bufs);
  for (; c < num_channel_bufs; c++)
    channel_bufs[c] = NULL;

  bool result =
    process_generic(1,4,buffer_origin,row_gap*4,suggested_increment,
                    max_region_pixels,incomplete_region,new_region,8,
                    (num_channels==num_colour_channels)?1:0);
  return result;
}

/******************************************************************************/
/*            kdu_region_decompressor::process (interleaved bytes)            */
/******************************************************************************/

bool
  kdu_region_decompressor::process(kdu_byte *buffer,
                                   int *chan_offsets,
                                   int pixel_gap,
                                   kdu_coords buffer_origin,
                                   int row_gap,
                                   int suggested_increment,
                                   int max_region_pixels,
                                   kdu_dims &incomplete_region,
                                   kdu_dims &new_region,
                                   int precision_bits,
                                   bool measure_row_gap_in_pixels,
                                   int expand_monochrome, int fill_alpha)
{
  num_channel_bufs = num_channels;
  if ((num_colour_channels == 1) && (expand_monochrome > 1))
    num_channel_bufs += expand_monochrome-1;
  fill_alpha -= (num_channels-num_colour_channels);
  if (fill_alpha < 0)
    fill_alpha = 0;
  else
    num_channel_bufs += fill_alpha;
  if (num_channel_bufs > max_channel_bufs)
    {
      max_channel_bufs = num_channel_bufs;
      if (channel_bufs != NULL)
        { delete[] channel_bufs; channel_bufs = NULL; }
      channel_bufs = new kdu_byte *[max_channel_bufs];
    }
  for (int c = 0; c < num_channel_bufs; c++)
    channel_bufs[c] = buffer + chan_offsets[c];
  if (measure_row_gap_in_pixels)
    row_gap *= pixel_gap;
  bool result;
  result=process_generic(1,pixel_gap,buffer_origin,row_gap,suggested_increment,
                         max_region_pixels,incomplete_region,new_region,
                         precision_bits,fill_alpha);
  return result;
}

/******************************************************************************/
/*            kdu_region_decompressor::process (interleaved words)            */
/******************************************************************************/

bool 
  kdu_region_decompressor::process(kdu_uint16 *buffer,
                                   int *chan_offsets,
                                   int pixel_gap,
                                   kdu_coords buffer_origin,
                                   int row_gap,
                                   int suggested_increment,
                                   int max_region_pixels,
                                   kdu_dims &incomplete_region,
                                   kdu_dims &new_region,
                                   int precision_bits,
                                   bool measure_row_gap_in_pixels,
                                   int expand_monochrome, int fill_alpha)
{
  num_channel_bufs = num_channels;
  if ((num_colour_channels == 1) && (expand_monochrome > 1))
    num_channel_bufs += expand_monochrome-1;
  fill_alpha -= (num_channels-num_colour_channels);
  if (fill_alpha < 0)
    fill_alpha = 0;
  else
    num_channel_bufs += fill_alpha;
  if (num_channel_bufs > max_channel_bufs)
    {
      max_channel_bufs = num_channel_bufs;
      if (channel_bufs != NULL)
        { delete[] channel_bufs; channel_bufs = NULL; }
      channel_bufs = new kdu_byte *[max_channel_bufs];
    }
  for (int c = 0; c < num_channel_bufs; c++)
    channel_bufs[c] = (kdu_byte *)(buffer + chan_offsets[c]);
  if (measure_row_gap_in_pixels)
    row_gap *= pixel_gap;
  return process_generic(2,pixel_gap,buffer_origin,row_gap,suggested_increment,
                         max_region_pixels,incomplete_region,new_region,
                         precision_bits,fill_alpha);
}

/******************************************************************************/
/*           kdu_region_decompressor::process ( interleaved floats)           */
/******************************************************************************/

bool 
kdu_region_decompressor::process(float *buffer, int *chan_offsets,
                                 int pixel_gap, kdu_coords buffer_origin,
                                 int row_gap, int suggested_increment,
                                 int max_region_pixels,
                                 kdu_dims &incomplete_region,
                                 kdu_dims &new_region, bool normalize,
                                 bool measure_row_gap_in_pixels,
                                 int expand_monochrome, int fill_alpha)
{
  num_channel_bufs = num_channels;
  if ((num_colour_channels == 1) && (expand_monochrome > 1))
    num_channel_bufs += expand_monochrome-1;
  fill_alpha -= (num_channels-num_colour_channels);
  if (fill_alpha < 0)
    fill_alpha = 0;
  else
    num_channel_bufs += fill_alpha;
  if (num_channel_bufs > max_channel_bufs)
    {
      max_channel_bufs = num_channel_bufs;
      if (channel_bufs != NULL)
        { delete[] channel_bufs; channel_bufs = NULL; }
      channel_bufs = new kdu_byte *[max_channel_bufs];
    }
  for (int c=0; c < num_channel_bufs; c++)
    channel_bufs[c] = (kdu_byte *)(buffer + chan_offsets[c]);
  if (measure_row_gap_in_pixels)
    row_gap *= pixel_gap;
  return process_generic(4,pixel_gap,buffer_origin,row_gap,suggested_increment,
                         max_region_pixels,incomplete_region,new_region,
                         ((normalize)?1:0),fill_alpha);
}

/******************************************************************************/
/*                  kdu_region_decompressor::process_generic                  */
/******************************************************************************/

bool
  kdu_region_decompressor::process_generic(int sample_bytes, int pixel_gap,
                                           kdu_coords buffer_origin,
                                           int row_gap, int suggested_increment,
                                           int max_region_pixels,
                                           kdu_dims &incomplete_region,
                                           kdu_dims &new_region,
                                           int precision_bits, int fill_alpha)
{
  new_region.size = kdu_coords(0,0); // In case we decompress nothing
  if (codestream_failure || !incomplete_region)
    return false;

  try { // Protect, in case a fatal error is generated by the decompressor
      int suggested_ref_comp_samples = suggested_increment;
      if ((suggested_increment <= 0) && (row_gap == 0))
        { // Need to consider `max_region_pixels' argument
          kdu_long num = channels[0].sampling_numerator.x;
          num *= channels[0].sampling_numerator.y;
          kdu_long den = channels[0].sampling_denominator.x;
          den *= channels[0].sampling_denominator.y;
          double scale = ((double) num) / ((double) den);
          suggested_ref_comp_samples = 1 + (int)(scale * max_region_pixels);
        }
    
      if ((current_bank == NULL) && (background_bank != NULL))
        {
          make_tile_bank_current(background_bank,incomplete_region);
          background_bank = NULL;
        }
      if (current_bank == NULL)
        {
          kdrd_tile_bank *new_bank = tile_banks + 0;
          if (!start_tile_bank(new_bank,suggested_ref_comp_samples,
                               incomplete_region))
            return false; // Trying to discard too many resolution levels or
                          // else trying to flip an unflippable image
          if (new_bank->num_tiles == 0)
            { // Opened at least one tile, but all such tiles
              // were found to lie outside the `conservative_ref_comp_region'
              // and so were closed again immediately.
              if ((next_tile_idx.x == valid_tiles.pos.x) &&
                  (next_tile_idx.y >= (valid_tiles.pos.y+valid_tiles.size.y)))
                { // No more tiles can lie within the `incomplete_region'
                  incomplete_region.pos.y += incomplete_region.size.y;
                  incomplete_region.size.y = 0;
                  return false;
                }
              return true;
            }
          make_tile_bank_current(new_bank,incomplete_region);
        }
      if ((env != NULL) && (background_bank == NULL) &&
          (next_tile_idx.y < (valid_tiles.pos.y+valid_tiles.size.y)))
        {
          background_bank = tile_banks + ((current_bank==tile_banks)?1:0);
          if (!start_tile_bank(background_bank,suggested_ref_comp_samples,
                               incomplete_region))
            return false; // Trying to discard too many resolution levels or
                          // else trying to flip an unflippable image
          if (background_bank->num_tiles == 0)
            { // Opened at least one tile, but all such tiles were found to
              // lie outside the `conservative_ref_comp_region' and so
              // were closed again immediately. No problem; just don't do any
              // background processing at present.
              background_bank = NULL;
            }
        }

      bool last_bank_in_row =
        ((render_dims.pos.x+render_dims.size.x) >=
         (incomplete_region.pos.x+incomplete_region.size.x));
      bool first_bank_in_new_row = current_bank->freshly_created &&
        (render_dims.pos.x <= incomplete_region.pos.x);
      if ((last_bank_in_row || first_bank_in_new_row) &&
          (render_dims.pos.y > incomplete_region.pos.y))
        { // Incomplete region must have shrunk horizontally and the
          // application has no way of knowing that we have already rendered
          // some initial lines of the new, narrower incomplete region.
          int y = render_dims.pos.y - incomplete_region.pos.y;
          incomplete_region.size.y -= y;
          incomplete_region.pos.y += y;
        }
      kdu_dims incomplete_bank_region = render_dims & incomplete_region;
      if (!incomplete_bank_region)
        { // No intersection between current tile bank and incomplete region.
          close_tile_bank(current_bank);
          current_bank = NULL;
                // When we consider multiple concurrent banks later on, we will
                // need to modify the code here.
          return true; // Let the caller get back to us for more tile processing
        }
      current_bank->freshly_created = false; // We are about to decompress data
    
      // Determine an appropriate number of rendered output lines to process
      // before returning.  Note that some or all of these lines might not
      // intersect with the incomplete region.
      new_region = incomplete_bank_region;
      new_region.size.y = 0;
      kdu_long new_lines =
        1 + (suggested_ref_comp_samples / current_bank->dims.size.x);
          // Starts off as a bound on the number of decompressed lines
      kdu_long den = channels[0].sampling_denominator.y;
      kdu_long num = channels[0].sampling_numerator.y;
      if (den > num)
        new_lines = (new_lines * den) / num;
          // Now `new_lines' is a bound on the number of rendered lines
      if (new_lines > (kdu_long) incomplete_bank_region.size.y)
        new_lines = incomplete_bank_region.size.y;
      if ((row_gap == 0) &&
          ((new_lines*(kdu_long)new_region.size.x)>(kdu_long)max_region_pixels))
        new_lines = max_region_pixels / new_region.size.x;
      if (new_lines <= 0)
        { KDU_ERROR_DEV(e,13); e <<
            KDU_TXT("Channel buffers supplied to "
            "`kdr_region_decompressor::process' are too small to "
            "accommodate even a single line of the new region "
            "to be decompressed.  You should be careful to ensure that the "
            "buffers are at least as large as the width indicated by the "
            "`incomplete_region' argument passed to the `process' "
            "function.  Also, be sure to identify the buffer sizes "
            "correctly through the `max_region_pixels' argument supplied "
            "to that function.");
        }

      // Align buffers at start of new region and set `row_gap' if necessary
      int c;
      if (row_gap == 0)
        row_gap = new_region.size.x * pixel_gap;
      else
        {
          size_t buf_offset = ((size_t) sample_bytes) *
            (((size_t)(new_region.pos.y-buffer_origin.y)) * (size_t) row_gap +
             ((size_t)(new_region.pos.x-buffer_origin.x)) * (size_t) pixel_gap);
          for (c=0; c < num_channel_bufs; c++)
            if (channel_bufs[c] != NULL)
              channel_bufs[c] += buf_offset;
        }
      row_gap *= sample_bytes;

      int num_active_channels = num_channel_bufs - fill_alpha;
#ifdef KDU_SIMD_OPTIMIZATIONS
      // Set up fast processing for interleaved buffers under simple, yet
      // common conditions
      kdu_byte *fast_base = NULL; // Non-NULL if fast processing possible
      kdu_byte fast_mask[4] = {0,0,0,0}; // FF if byte is to be written
      kdu_byte fast_source[4] = {0,0,0,0}; // Idx of src channel to use
      if ((pixel_gap == 4) && (sample_bytes == 1) && (num_channel_bufs == 4) &&
          (precision_bits > 0) && (precision_bits <= 8) &&
          (new_region.pos.x == render_dims.pos.x) && (fill_alpha <= 1))
        {
          fast_base = (kdu_byte *)
            _kdu_long_to_addr(_addr_to_kdu_long(channel_bufs[0]) &
                              ~((kdu_long) 3));
          for (c=0; c < num_channel_bufs; c++)
            {
              if ((c < num_channels) && !channels[c].using_shorts)
                break; // Fast processing only if source samples 16 bits wide
              int dst_idx = (int)(channel_bufs[c] - fast_base);
              if ((dst_idx < 0) || (dst_idx > 3))
                break; // Fast processing only for aligned & interleaved bufs
              int src_idx = c;
              if ((num_active_channels > num_channels) && (c != 0))
                {
                  assert(num_active_channels == (num_channels+2));
                  src_idx -= ((c==1)?1:2);
                }
              if (fill_alpha && (c >= num_active_channels))
                src_idx = 0; // Any valid source; we won't use it.
              fast_mask[dst_idx] = (kdu_byte) 0xFF;
              fast_source[dst_idx] = (kdu_byte) src_idx;
            }
          if (c < num_channel_bufs)
            fast_base = NULL; // Cannot use fast processing
        }
#endif // KDU_SIMD_OPTIMIZATIONS

      // Determine and process new region.
      int skip_cols = new_region.pos.x - render_dims.pos.x;
      int num_cols = new_region.size.x;
      while (new_lines > 0)
        {
          // Decompress new image component lines as necessary.
          bool anything_needed = false;
          for (c=0; c < num_components; c++)
            {
              kdrd_component *comp = components + c;
              if (comp->needed_line_samples > 0)
                { // Vertical interpolation process needs more data
                  if (comp->dims.size.y <= 0)
                    { // This tile never had any lines in this component.  We
                      // will just use the zeroed `indices' buffer
                      assert(comp->num_tiles == 0);
                      comp->new_line_samples = comp->needed_line_samples;
                      comp->needed_line_samples = 0;
                    }
                  else
                    {
                      comp->new_line_samples = comp->non_empty_tile_lines = 0;
                      anything_needed = true;
                    }
                }
            }
          if (anything_needed)
            {
              int tnum;
              for (tnum=0; tnum < current_bank->num_tiles; tnum++)
                {
                  kdu_multi_synthesis *engine = current_bank->engines + tnum;
                  for (c=0; c < num_components; c++)
                    {
                      kdrd_component *comp = components + c;
                      if (comp->needed_line_samples <= comp->new_line_samples)
                        continue;
                      kdu_line_buf *line =
                        engine->get_line(comp->rel_comp_idx,env);
                      if ((line == NULL) || (line->get_width() == 0))
                        continue;
                      if (comp->num_line_users > 0)
                        comp->tile_lines[comp->non_empty_tile_lines++] = line;
                      if (comp->palette_bits > 0)
                        convert_samples_to_palette_indices(line,
                                         comp->bit_depth,comp->is_signed,
                                         comp->palette_bits,&(comp->indices),
                                         comp->new_line_samples);
                      comp->new_line_samples += line->get_width();
                   }
                }
              for (c=0; c < num_components; c++)
                {
                  kdrd_component *comp = components + c;
                  if (comp->needed_line_samples > 0)
                    {
                      assert(comp->new_line_samples==comp->needed_line_samples);
                      comp->needed_line_samples = 0;
                      comp->dims.size.y--;
                      comp->dims.pos.y++;
                    }
                }
            }

          // Perform horizontal interpolation/mapping operations for channel
          // lines whose component lines were recently created
          for (c=0; c < num_channels; c++)
            {
              kdrd_channel *chan = channels + c;
              kdrd_component *comp = chan->source;
              if (comp->new_line_samples == 0)
                continue;
              if ((chan->out_line != NULL) || (chan->horz_line != NULL))
                continue; // Already have something ready for this channel
              if (chan->missing.y < 0)
                { // This channel does not need the most recently generated line
                  chan->missing.y++;
                  continue;
                }
              
              if (chan->can_use_component_samples_directly)
                {
                  assert((comp->num_tiles == 1) && (chan->missing.x == 0));
                  chan->horz_line = comp->tile_lines[0];
                }
              else
                {
                  if (chan->in_line == NULL)
                    chan->in_line = chan->get_free_line();
                  int dst_min = chan->in_line_start;
                  int dst_len = chan->in_line_length;
                  kdu_line_buf *idx_line = &comp->indices;
                  kdu_line_buf **src_lines = comp->tile_lines;
                  int num_src_lines = comp->non_empty_tile_lines;
                  if (comp->num_tiles == 0)
                    { // Use the all-zero line stored in `comp->indices'
                      num_src_lines = 1; src_lines = &idx_line;
                    }
                  if (chan->lut != NULL)
                    {
                      assert(chan->using_shorts);
                      kdu_int16 *dst16=(kdu_int16 *)chan->in_line->get_buf16();
                      dst16 += dst_min;
                      if (chan->boxcar_log_size == 0)
                        {
                          assert(chan->in_precision == KDU_FIX_POINT);
                          perform_palette_map(idx_line,dst16,chan->lut,dst_len,
                                              chan->missing.x);
                        }
                      else
                        {
                          assert(chan->in_precision > KDU_FIX_POINT);
                          kdu_int32 *dst32 = (kdu_int32 *)(dst16-(dst_min&1));
                          if (chan->boxcar_lines_left == chan->boxcar_size.y)
                            memset(dst32,0,((size_t) dst_len)<<2);
                          map_and_integrate(idx_line,dst32,chan->lut,dst_len,
                                      chan->missing.x,chan->boxcar_size.x,
                                      chan->in_precision-chan->boxcar_log_size);
                        }
                    }
                  else if (chan->using_shorts)
                    {
                      kdu_int16 *dst16=(kdu_int16 *)chan->in_line->get_buf16();
                      dst16 += dst_min;
                      if (chan->boxcar_log_size == 0)
                        {
                          assert(chan->in_precision == KDU_FIX_POINT);
                          convert_and_copy16(src_lines,num_src_lines,
                                      comp->bit_depth,dst16,dst_len,
                                      chan->missing.x);
                        }
                      else
                        {
                          assert(chan->in_precision > KDU_FIX_POINT);
                          kdu_int32 *dst32 = (kdu_int32 *)(dst16-(dst_min&1));
                          if (chan->boxcar_lines_left == chan->boxcar_size.y)
                            memset(dst32,0,((size_t) dst_len)<<2);
                          convert_and_add32(src_lines,num_src_lines,
                                      comp->bit_depth,dst32,dst_len,
                                      chan->missing.x,chan->boxcar_size.x,
                                      chan->in_precision-chan->boxcar_log_size);
                        }
                    }
                  else if (chan->using_floats)
                    {
                      float *dst32=(float *)chan->in_line->get_buf32();
                      assert((dst32 != NULL) && !chan->in_line->is_absolute());
                      dst32 += dst_min;
                      if (chan->boxcar_log_size == 0)
                        convert_and_copy_float(src_lines,num_src_lines,
                                               comp->bit_depth,dst32,dst_len,
                                               chan->missing.x);
                      else
                        {
                          if (chan->boxcar_lines_left == chan->boxcar_size.y)
                            for (float *dp=dst32; dp < (dst32+dst_len); dp++)
                              *dp = 0.0F;
                          convert_and_add_float(src_lines,num_src_lines,
                                          comp->bit_depth,dst32,dst_len,
                                          chan->missing.x,chan->boxcar_size.x,
                                          1.0F/(1<<chan->boxcar_log_size));
                        }
                    }
                  else
                    { // 32-bit integer target; no processing here.
                      assert((chan->boxcar_log_size == 0) && (dst_min == 0));
                      kdu_int32 *dst32=(kdu_int32 *)chan->in_line->get_buf32();
                      assert((dst32 != NULL) && chan->in_line->is_absolute());
                      convert_and_copy32(src_lines,num_src_lines,
                                         comp->bit_depth,dst32,dst_len,
                                         chan->missing.x,chan->in_precision);
                    }
                  
                  chan->boxcar_lines_left--;
                  if (chan->boxcar_lines_left > 0)
                    continue;
                  
                  if ((chan->boxcar_log_size > 0) && chan->using_shorts)
                    {
                      kdu_int16 *dst16=(kdu_int16 *)chan->in_line->get_buf16();
                      dst16 += dst_min;
                      kdu_int32 *src32 = (kdu_int32 *)(dst16-(dst_min&1));
                      do_boxcar_renormalization(src32,dst16,dst_len,
                                                chan->in_precision);
                    }

                  if (chan->sampling_numerator.x==chan->sampling_denominator.x)
                    chan->horz_line = chan->in_line;
                  else
                    { // Perform disciplined horizontal resampling
                      chan->horz_line = chan->get_free_line();
                      if (chan->using_floats)
                        {
#ifdef KDU_SIMD_OPTIMIZATIONS
                          if (!simd_horz_resampling_float(chan->out_line_length,
                                   chan->in_line,chan->horz_line,
                                   chan->sampling_phase.x,
                                   chan->sampling_numerator.x,
                                   chan->sampling_denominator.x,
                                   chan->sampling_phase_shift.x,
                                   (float **)chan->simd_horz_interp_kernels,
                                   chan->h_kernels.simd_kernel_length,
                                   chan->h_kernels.simd_horz_leadin))
#endif // KDU_SIMD_OPTIMIZATIONS
                          do_horz_resampling_float(chan->out_line_length,
                                   chan->in_line,chan->horz_line,
                                   chan->sampling_phase.x,
                                   chan->sampling_numerator.x,
                                   chan->sampling_denominator.x,
                                   chan->sampling_phase_shift.x,
                                   chan->h_kernels.kernel_length,
                                   (float **)chan->horz_interp_kernels);
                        }
                      else
                        {
#ifdef KDU_SIMD_OPTIMIZATIONS
                          if (!simd_horz_resampling_fix16(chan->out_line_length,
                                   chan->in_line,chan->horz_line,
                                   chan->sampling_phase.x,
                                   chan->sampling_numerator.x,
                                   chan->sampling_denominator.x,
                                   chan->sampling_phase_shift.x,
                                   (kdu_int16 **)chan->simd_horz_interp_kernels,
                                   chan->h_kernels.simd_kernel_length,
                                   chan->h_kernels.simd_horz_leadin))
#endif // KDU_SIMD_OPTIMIZATIONS                            
                          do_horz_resampling_fix16(chan->out_line_length,
                                   chan->in_line,chan->horz_line,
                                   chan->sampling_phase.x,
                                   chan->sampling_numerator.x,
                                   chan->sampling_denominator.x,
                                   chan->sampling_phase_shift.x,
                                   chan->h_kernels.kernel_length,
                                   (kdu_int32 **)chan->horz_interp_kernels);
                        }
                      chan->recycle_line(chan->in_line);
                    }
                  chan->in_line = NULL;
                }
            }
                    
          // Now generate channel output lines wherever we can and see whether
          // or not all channels are ready.
          bool all_channels_ready = true;
          for (c=0; c < num_channels; c++)
            {
              kdrd_channel *chan = channels + c;
              if (chan->out_line != NULL)
                continue;
              if (chan->horz_line == NULL)
                { all_channels_ready = false; continue; }
              if (chan->sampling_numerator.y == chan->sampling_denominator.y)
                {
                  chan->out_line = chan->horz_line;
                  chan->horz_line = NULL;
                }
              else
                { // Perform disciplined vertical resampling
                  if (chan->num_valid_vlines < KDRD_CHANNEL_VLINES)
                    chan->vlines[chan->num_valid_vlines++] = chan->horz_line;
                  if (chan->num_valid_vlines < KDRD_CHANNEL_VLINES)
                    chan->horz_line = NULL;
                  else
                    { // Generate an output line now
                      chan->out_line = chan->get_free_line();
                      int s = chan->sampling_phase_shift.y;
                      kdu_uint32 p =
                        ((kdu_uint32)(chan->sampling_phase.y+((1<<s)>>1)))>>s;
                      if (chan->using_floats)
                        {
#ifdef KDU_SIMD_OPTIMIZATIONS
                          if (!simd_vert_resampling_float(chan->out_line_length,
                                     chan->vlines,chan->out_line,
                                     chan->v_kernels.kernel_length,(float *)
                                     (chan->simd_vert_interp_kernels[p])))
#endif // KDU_SIMD_OPTIMIZATIONS
                          do_vert_resampling_float(chan->out_line_length,
                                     chan->vlines,chan->out_line,
                                     chan->v_kernels.kernel_length,(float *)
                                     (chan->vert_interp_kernels[p]));
                        }
                      else
                        {
#ifdef KDU_SIMD_OPTIMIZATIONS
                          if (!simd_vert_resampling_fix16(chan->out_line_length,
                                     chan->vlines,chan->out_line,
                                     chan->v_kernels.kernel_length,(kdu_int16 *)
                                     (chan->simd_vert_interp_kernels[p])))
#endif // KDU_SIMD_OPTIMIZATIONS
                          do_vert_resampling_fix16(chan->out_line_length,
                                     chan->vlines,chan->out_line,
                                     chan->v_kernels.kernel_length,(kdu_int32 *)
                                     (chan->vert_interp_kernels[p]));
                        }
                      chan->sampling_phase.y += chan->sampling_numerator.y;
                      while (((kdu_uint32) chan->sampling_phase.y) >=
                             ((kdu_uint32) chan->sampling_denominator.y))
                        {
                          chan->horz_line = NULL; // So we make a new one
                          chan->sampling_phase.y-=chan->sampling_denominator.y;
                          chan->recycle_line(chan->vlines[0]);
                          chan->num_valid_vlines--;
                          for (int w=0; w < chan->num_valid_vlines; w++)
                            chan->vlines[w] = chan->vlines[w+1];
                          assert(chan->num_valid_vlines > 0);
                            // Otherwise should be using boxcar subsampling
                            // to avoid over-extending the vertical interp
                            // kernels.
                          chan->vlines[chan->num_valid_vlines] = NULL;
                        }
                    }
                }
              if (chan->out_line == NULL)
                { all_channels_ready = false; continue; }
              if (chan->stretch_residual > 0)
                {
                  assert(chan->using_shorts);
                  if ((chan->source->num_tiles > 0) &&
                      (chan->out_line == chan->source->tile_lines[0]))
                    { // Don't do white stretching in place
                      chan->out_line = chan->get_free_line();
                      apply_white_stretch(chan->source->tile_lines[0],
                                          chan->out_line,chan->out_line_length,
                                          chan->stretch_residual);
                    }
                  else
                    apply_white_stretch(chan->out_line,chan->out_line,
                                        chan->out_line_length,
                                        chan->stretch_residual);
                }
            }
          
          // Mark the source component lines which we have fully consumed
          // and determine the components from which we need more
          // decompressed samples.
          for (c=0; c < num_channels; c++)
            {
              kdrd_channel *chan = channels+c;
              kdrd_component *comp = chan->source;
              if (comp->new_line_samples > 0)
                {
                  if (chan->missing.y > 0)
                    chan->missing.y--; // Keep using the same samples
                  else if (comp->dims.size.y > 0)
                    comp->new_line_samples = 0;
                }
              if ((chan->horz_line == NULL) &&
                  ((chan->out_line == NULL) || all_channels_ready))
                {
                  if (chan->in_line == NULL)
                    chan->boxcar_lines_left = chan->boxcar_size.y;
                  if (comp->new_line_samples == 0)
                    comp->needed_line_samples = comp->dims.size.x;
                }
            }
          
          if (!all_channels_ready)
            continue;
          
          if (render_dims.pos.y == incomplete_bank_region.pos.y)
            { // This line has non-empty intersection with the incomplete region
              if (colour_converter != NULL)
                { // In this case, we apply `colour_converter' once for each
                  // vertical output line.  We need to do this if the channels
                  // have different vertical interpolation properties.
                  if (num_colour_channels < 3)
                    colour_converter->convert_lum(*(channels[0].out_line),
                                                  render_dims.size.x);
                  else if (num_colour_channels == 3)
                    colour_converter->convert_rgb(*(channels[0].out_line),
                                                  *(channels[1].out_line),
                                                  *(channels[2].out_line),
                                                  render_dims.size.x);
                  else
                    colour_converter->convert_rgb4(*(channels[0].out_line),
                                                   *(channels[1].out_line),
                                                   *(channels[2].out_line),
                                                   *(channels[3].out_line),
                                                   render_dims.size.x);
                }

#ifdef KDU_SIMD_OPTIMIZATIONS
              if ((fast_base != NULL) &&
                  simd_interleaved_transfer(fast_base,fast_mask,num_cols,
                                            num_channel_bufs,precision_bits,
                                            channels[fast_source[0]].out_line,
                                            channels[fast_source[1]].out_line,
                                            channels[fast_source[2]].out_line,
                                            channels[fast_source[3]].out_line,
                                            (fill_alpha != 0)))
                fast_base += row_gap;
              else
#endif // KDU_SIMD_OPTIMIZATIONS
              for (c=0; c < num_channel_bufs; c++)
                {
                  if (channel_bufs[c] == NULL)
                    continue;
                  kdrd_channel *chan = channels + c;
                  if ((num_active_channels > num_channels) && (c != 0))
                    {
                      if (c <= (num_active_channels-num_channels))
                        chan = channels;
                      else
                        chan -= (num_active_channels-num_channels);
                    }

                  kdrd_component *comp = chan->source;
                  if (c >= num_active_channels)
                    {
                      assert(fill_alpha > 0);
                      if (sample_bytes == 1)
                        {
                          kdu_byte fill_val = 0xFF;
                          if ((precision_bits > 0) && (precision_bits < 8))
                            fill_val = (kdu_byte)((1<<precision_bits)-1);
                          kdu_byte *dp = channel_bufs[c];
                          for (int n=num_cols; n > 0; n--, dp+=pixel_gap)
                            *dp = fill_val;
                        }
                      else if (sample_bytes == 2)
                        {
                          kdu_uint16 fill_val = 0xFFFF;
                          if ((precision_bits > 0) && (precision_bits < 16))
                            fill_val = (kdu_uint16)((1<<precision_bits)-1);
                          kdu_uint16 *dp = (kdu_uint16 *)(channel_bufs[c]);
                          for (int n=num_cols; n > 0; n--, dp+=pixel_gap)
                            *dp = fill_val;
                        }
                      else
                        {
                          float *dp = (float *)(channel_bufs[c]);
                          for (int n=num_cols; n > 0; n--, dp+=pixel_gap)
                            *dp = 1.0F;
                        }
                    }
                  else if (precision_bits == 0)
                    { // Derive the precision information from `chan'
                      if (sample_bytes == 1)
                        transfer_fixed_point(chan->out_line,comp->bit_depth,
                                 skip_cols,num_cols,pixel_gap,channel_bufs[c],
                                 chan->native_precision,chan->native_signed);
                      else if (sample_bytes == 2)
                        transfer_fixed_point(chan->out_line,comp->bit_depth,
                                 skip_cols,num_cols,pixel_gap,
                                 (kdu_uint16 *)(channel_bufs[c]),
                                 chan->native_precision,chan->native_signed);
                      else if (sample_bytes == 4)
                        transfer_floats(chan->out_line,comp->bit_depth,
                                 skip_cols,num_cols,pixel_gap,
                                 (float *)(channel_bufs[c]),
                                 chan->native_precision,chan->native_signed,
                                 chan->stretch_source_factor);
                      else
                        assert(0);
                    }
                  else
                    { // Typical case; write all samples as unsigned data
                      // with the same precision
                      if (sample_bytes == 1)
                        transfer_fixed_point(chan->out_line,comp->bit_depth,
                                 skip_cols,num_cols,pixel_gap,channel_bufs[c],
                                 precision_bits,false);
                      else if (sample_bytes == 2)
                        transfer_fixed_point(chan->out_line,comp->bit_depth,
                                 skip_cols,num_cols,pixel_gap,
                                 (kdu_uint16 *)(channel_bufs[c]),
                                 precision_bits,false);
                      else if (sample_bytes == 4)
                        transfer_floats(chan->out_line,comp->bit_depth,
                                 skip_cols,num_cols,pixel_gap,
                                 (float *)(channel_bufs[c]),
                                 0,false,chan->stretch_source_factor);
                      else
                        assert(0);
                    }
                }

              for (c=0; c < num_channel_bufs; c++)
                if (channel_bufs[c] != NULL)
                  channel_bufs[c] += row_gap;

              incomplete_bank_region.pos.y++;
              incomplete_bank_region.size.y--;
              new_region.size.y++; // Transferred data region grows by one row.
              if (last_bank_in_row)
                {
                  int y = (render_dims.pos.y+1) - incomplete_region.pos.y;
                  assert(y > 0);
                  incomplete_region.pos.y += y;
                  incomplete_region.size.y -= y;
                }
            }
          
          // Mark all the channel output lines as consumed so we can generate
          // some new ones.
          for (c=0; c < num_channels; c++)
            {
              channels[c].recycle_line(channels[c].out_line);
              channels[c].out_line = NULL;
            }

          new_lines--;
          render_dims.pos.y++;
          render_dims.size.y--;
        }
    
      if (!incomplete_bank_region)
        { // Done all the processing we need for this tile.
          close_tile_bank(current_bank);
          current_bank = NULL;
          return true;
        }
    }
  catch (kdu_exception exc)
    {
      codestream_failure = true;
      codestream_failure_exception = exc;
      if (env != NULL)
        env->handle_exception(exc);
      return false;
    }
  catch (std::bad_alloc &)
    {
      codestream_failure = true;
      codestream_failure_exception = KDU_MEMORY_EXCEPTION;
      if (env != NULL)
        env->handle_exception(KDU_MEMORY_EXCEPTION);
      return false;
    }
  return true;
}
