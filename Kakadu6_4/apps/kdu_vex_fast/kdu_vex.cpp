/*****************************************************************************/
// File: kdu_vex.cpp [scope = APPS/VEX_FAST]
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
   Implements the objects defined in "vex_fast_local.h".
******************************************************************************/

#include <string.h>
#include <stdio.h> // so we can use `sscanf' for arg parsing.
#include <assert.h>
#include "kdu_vex.h"

// Set things up for the inclusion of assembler optimized routines
// for specific architectures.  The reason for this is to exploit
// the availability of SIMD type instructions to greatly improve the
// efficiency with which sample values can be converted, truncated and mapped.
#if (defined KDU_PENTIUM_MSVC) || (defined KDU_X86_INTRINSICS)
#  ifndef KDU_NO_SSE
#    define VEX_SIMD_OPTIMIZATIONS
#    include <emmintrin.h>
#  endif
#endif


/* ========================================================================= */
/*                            INTERNAL FUNCTIONS                             */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                      kd_set_threadname                             */
/*****************************************************************************/

#if (defined _WIN32 && defined _DEBUG && defined _MSC_VER && (_MSC_VER>=1300))
static void
  kd_set_threadname(LPCSTR thread_name)
{
  struct {
      DWORD dwType; // must be 0x1000
      LPCSTR szName; // pointer to name (in user addr space)
      DWORD dwThreadID; // thread ID (-1=caller thread)
      DWORD dwFlags; // reserved for future use, must be zero
    } info;
  info.dwType = 0x1000;
  info.szName = thread_name;
  info.dwThreadID = -1;
  info.dwFlags = 0;
  __try {
      RaiseException(0x406D1388,0,sizeof(info)/sizeof(DWORD),
                     (ULONG_PTR *) &info );
    }
  __except(EXCEPTION_CONTINUE_EXECUTION) { }
}
#endif // .NET debug compilation

/*****************************************************************************/
/* INLINE                 simd_xfer_shorts_bytes_signed                      */
/*****************************************************************************/
#ifdef VEX_SIMD_OPTIMIZATIONS
static inline void
  simd_xfer_shorts_bytes_signed(kdu_sample16 *src, kdu_byte *dst, int width,
                                int downshift)
  /* We only call this function if `src' and `dst' are 16-byte aligned,
     `width' is a multiple of 16 bytes and `downshift' is >= 0. */
{
  __m128i *sp = (__m128i *) src;
  __m128i *dp = (__m128i *) dst;
  __m128i shift = _mm_cvtsi32_si128(downshift);
  __m128i offset = _mm_set1_epi16((1<<downshift)>>1);
  for (; width > 0; width-=16, sp+=2, dp++)
    {
      __m128i val0=sp[0], val1=sp[1];
      val0 = _mm_add_epi16(val0,offset);
      val1 = _mm_add_epi16(val1,offset);
      val0 = _mm_sra_epi16(val0,shift);
      val1 = _mm_sra_epi16(val1,shift);
      _mm_stream_si128(dp,_mm_packs_epi16(val0,val1));
           // Write around the L2 cache
    }
}
#endif // VEX_SIMD_OPTIMIZATIONS

/*****************************************************************************/
/* INLINE               simd_xfer_shorts_bytes_unsigned                      */
/*****************************************************************************/
#ifdef VEX_SIMD_OPTIMIZATIONS
static inline void
  simd_xfer_shorts_bytes_unsigned(kdu_sample16 *src, kdu_byte *dst, int width,
                                  int downshift)
  /* We only call this function if `src' and `dst' are 16-byte aligned,
     `width' is a multiple of 16 bytes and `downshift' is >= 0. */
{
  __m128i *sp = (__m128i *) src;
  __m128i *dp = (__m128i *) dst;
  __m128i shift = _mm_cvtsi32_si128(downshift);
  __m128i offset = _mm_set1_epi16((128<<downshift) + ((1<<downshift)>>1));
  for (; width > 0; width-=16, sp+=2, dp++)
    {
      __m128i val0=sp[0], val1=sp[1];
      val0 = _mm_add_epi16(val0,offset);
      val1 = _mm_add_epi16(val1,offset);
      val0 = _mm_sra_epi16(val0,shift);
      val1 = _mm_sra_epi16(val1,shift);
      _mm_stream_si128(dp,_mm_packus_epi16(val0,val1));
           // Write around the L2 cache
    }
}
#endif // VEX_SIMD_OPTIMIZATIONS

/*****************************************************************************/
/* INLINE                    simd_rgb_shorts_to_argb8                        */
/*****************************************************************************/
#ifdef VEX_SIMD_OPTIMIZATIONS
static inline void
  simd_rgb_shorts_to_argb8(kdu_sample16 *red_src, kdu_sample16 *green_src,
                           kdu_sample16 *blue_src, kdu_byte *blue_dst,
                           int width, int downshift)
  /* We only call this function if `red_src', `green_src', `blue_src' and
     `blue_dst' are 16-byte aligned, `downshift' >= 0 and `width' is a
     multiple of 4 -- or at least no problems will arise if we write
     multiples of 4 pixels at a time. */
{
  __m128i *rp = (__m128i *) red_src;
  __m128i *gp = (__m128i *) green_src;
  __m128i *bp = (__m128i *) blue_src;
  __m128i *dp = (__m128i *) blue_dst;
  __m128i shift = _mm_cvtsi32_si128(downshift);
  __m128i offset = _mm_set1_epi16((128<<downshift) + ((1<<downshift)>>1));
  int quads = (width+3)>>2;
  int s, sextets = quads >> 2;
  for (s=0; s < sextets; s++)
    { // Generate output pixels in multiples of 16 (64 bytes) at a time
      __m128i val0=rp[2*s], val1=rp[2*s+1];
      val0 = _mm_add_epi16(val0,offset);
      val1 = _mm_add_epi16(val1,offset);
      val0 = _mm_sra_epi16(val0,shift);
      val1 = _mm_sra_epi16(val1,shift);
      __m128i red = _mm_packus_epi16(val0,val1);
      val0=gp[2*s], val1=gp[2*s+1];
      val0 = _mm_add_epi16(val0,offset);
      val1 = _mm_add_epi16(val1,offset);
      val0 = _mm_sra_epi16(val0,shift);
      val1 = _mm_sra_epi16(val1,shift);
      __m128i green = _mm_packus_epi16(val0,val1);
      val0=bp[2*s], val1=bp[2*s+1];
      val0 = _mm_add_epi16(val0,offset);
      val1 = _mm_add_epi16(val1,offset);
      val0 = _mm_sra_epi16(val0,shift);
      val1 = _mm_sra_epi16(val1,shift);
      __m128i blue = _mm_packus_epi16(val0,val1);
      __m128i blue_green = _mm_unpacklo_epi8(blue,green);
      __m128i red_alpha = _mm_unpacklo_epi8(red,red);

      // Write outputs around the L2 cache
      _mm_stream_si128(dp+4*s+0,_mm_unpacklo_epi16(blue_green,red_alpha));
      _mm_stream_si128(dp+4*s+1,_mm_unpackhi_epi16(blue_green,red_alpha));
      blue_green = _mm_unpackhi_epi8(blue,green);
      red_alpha = _mm_unpackhi_epi8(red,red);
      _mm_stream_si128(dp+4*s+2,_mm_unpacklo_epi16(blue_green,red_alpha));
      _mm_stream_si128(dp+4*s+3,_mm_unpackhi_epi16(blue_green,red_alpha));
    }
  quads -= sextets<<2;
  for (int c=s<<1; quads > 0; quads-=2, c++)
    { // Generate output pixels in multiples of 8 (32 bytes) at a time
      __m128i val0=rp[c];
      val0 = _mm_add_epi16(val0,offset);
      val0 = _mm_sra_epi16(val0,shift);
      __m128i red = _mm_packus_epi16(val0,val0);
      val0=gp[c];
      val0 = _mm_add_epi16(val0,offset);
      val0 = _mm_sra_epi16(val0,shift);
      __m128i green = _mm_packus_epi16(val0,val0);
      val0=bp[c];
      val0 = _mm_add_epi16(val0,offset);
      val0 = _mm_sra_epi16(val0,shift);
      __m128i blue = _mm_packus_epi16(val0,val0);
      __m128i blue_green = _mm_unpacklo_epi8(blue,green);
      __m128i red_alpha = _mm_unpacklo_epi8(red,red);
      _mm_stream_si128(dp+2*c+0,_mm_unpacklo_epi16(blue_green,red_alpha));
      if (quads > 1)
        _mm_stream_si128(dp+2*c+1,_mm_unpackhi_epi16(blue_green,red_alpha));
    }
}
#endif // VEX_SIMD_OPTIMIZATIONS

/*****************************************************************************/
/* INLINE                   simd_mono_shorts_to_argb8                        */
/*****************************************************************************/
#ifdef VEX_SIMD_OPTIMIZATIONS
static inline void
  simd_mono_shorts_to_argb8(kdu_sample16 *src, kdu_byte *dst,
                           int width, int downshift)
/* We only call this function if `src' is 16-byte aligned, `downshift' >= 0
   and `width' is a multiple of 4 -- or at least no problems will arise if
   we write multiples of 4 pixels at a time. */
{
  __m128i *sp = (__m128i *) src;
  __m128i *dp = (__m128i *) dst;
  __m128i shift = _mm_cvtsi32_si128(downshift);
  __m128i offset = _mm_set1_epi16((128<<downshift) + ((1<<downshift)>>1));
  int quads = (width+3)>>2;
  int s, sextets = quads >> 2;
  for (s=0; s < sextets; s++)
    { // Generate output pixels in multiples of 16 (64 bytes) at a time
      __m128i val0=sp[2*s], val1=sp[2*s+1];
      val0 = _mm_add_epi16(val0,offset);
      val1 = _mm_add_epi16(val1,offset);
      val0 = _mm_sra_epi16(val0,shift);
      val1 = _mm_sra_epi16(val1,shift);
      __m128i lum = _mm_packus_epi16(val0,val1);
      __m128i lum_x2 = _mm_unpacklo_epi8(lum,lum);

      // Write outputs around the L2 cache
      _mm_stream_si128(dp+4*s+0,_mm_unpacklo_epi16(lum_x2,lum_x2));
      _mm_stream_si128(dp+4*s+1,_mm_unpackhi_epi16(lum_x2,lum_x2));
      lum_x2 = _mm_unpackhi_epi8(lum,lum);
      _mm_stream_si128(dp+4*s+2,_mm_unpacklo_epi16(lum_x2,lum_x2));
      _mm_stream_si128(dp+4*s+3,_mm_unpackhi_epi16(lum_x2,lum_x2));
    }
  quads -= sextets<<2;
  for (int c=s<<1; quads > 0; quads-=2, c++)
    { // Generate output pixels in multiples of 8 (32 bytes) at a time
      __m128i val0=sp[c];
      val0 = _mm_add_epi16(val0,offset);
      val0 = _mm_sra_epi16(val0,shift);
      __m128i lum = _mm_packus_epi16(val0,val0);
      __m128i lum_x2 = _mm_unpacklo_epi8(lum,lum);
      _mm_stream_si128(dp+2*c+0,_mm_unpacklo_epi16(lum_x2,lum_x2));
      if (quads > 1)
        _mm_stream_si128(dp+2*c+1,_mm_unpackhi_epi16(lum_x2,lum_x2));
    }
}
#endif // VEX_SIMD_OPTIMIZATIONS

/*****************************************************************************/
/* STATIC                    transfer_line_to_bytes                          */
/*****************************************************************************/

static void
  transfer_line_to_bytes(kdu_line_buf &line, kdu_byte *dst, int width,
                         int precision, bool is_signed, int sample_gap=1)
{
  kdu_byte *dp = dst;
  kdu_int32 val;
  if (line.get_buf16() != NULL)
    {
      kdu_sample16 *sp = line.get_buf16();
      if (line.is_absolute() && (precision <= 8))
        {
          int upshift = 8-precision;
          if (is_signed)
            for (; width--; dp+=sample_gap, sp++)
              {
                val = ((kdu_int32) sp->ival) << upshift;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp+=sample_gap, sp++)
              {
                val = ((kdu_int32) sp->ival) << upshift;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
      else if (line.is_absolute())
        {
          int downshift = precision-8;
          kdu_int32 offset = (1<<downshift) >> 1;
          if (is_signed)
            for (; width--; dp+=sample_gap, sp++)
              {
                val = (((kdu_int32) sp->ival) + offset) >> downshift;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp+=sample_gap, sp++)
              {
                val = (((kdu_int32) sp->ival) + offset) >> downshift;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
      else
        {
          kdu_int32 offset = 1 << (KDU_FIX_POINT-1);
          offset += (1 << (KDU_FIX_POINT-8)) >> 1;
          if (is_signed)
            for (; width--; dp+=sample_gap, sp++)
              {
                val = (((kdu_int32) sp->ival) + offset) >> (KDU_FIX_POINT-8);
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp+=sample_gap, sp++)
              {
                val = (((kdu_int32) sp->ival) + offset) >> (KDU_FIX_POINT-8);
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
    }
  else
    {
      kdu_sample32 *sp = line.get_buf32();
      if (line.is_absolute())
        {
          assert(precision >= 8);
          int downshift = precision-8;
          kdu_int32 offset = (1<<downshift)-1;
          offset += 128 << downshift;
          if (is_signed)
            for (; width--; dp+=sample_gap, sp++)
              {
                val = (sp->ival + offset) >> downshift;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp+=sample_gap, sp++)
              {
                val = (sp->ival + offset) >> downshift;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
      else
        {
          if (is_signed)
            for (; width--; dp+=sample_gap, sp++)
              {
                val = (kdu_int32)(sp->fval * (float)(1<<24));
                val = (val + (1<<15)) >> 16;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp+=sample_gap, sp++)
              {
                val = (kdu_int32)(sp->fval * (float)(1<<24));
                val = (val + (1<<15)) >> 16;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
    }
}

/*****************************************************************************/
/* STATIC                    transfer_line_to_words                          */
/*****************************************************************************/

static void
  transfer_line_to_words(kdu_line_buf &line, void *dst, int width,
                         int precision, bool is_signed)
{
  kdu_int16 *dp = (kdu_int16 *) dst;
  kdu_int32 val;
  if (line.get_buf16() != NULL)
    {
      kdu_sample16 *sp = line.get_buf16();
      if (line.is_absolute())
        {
          int upshift = 16-precision;
          if (is_signed)
            for (; width--; sp++, dp++)
              {
                val = ((kdu_int32) sp->ival) << upshift;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; sp++, dp++)
              {
                val = ((kdu_int32) sp->ival) << upshift;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16) val;
              }
        }
      else
        {
          if (is_signed)
            for (; width--; sp++, dp++)
              {
                val = ((kdu_int32) sp->ival) << (16-KDU_FIX_POINT);
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; sp++, dp++)
              {
                val = ((kdu_int32) sp->ival) << (16-KDU_FIX_POINT);
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16) val;
              }
        }
    }
  else
    {
      kdu_sample32 *sp = line.get_buf32();
      if (line.is_absolute() && (precision > 16))
        {
          int downshift = precision - 16;
          kdu_int32 offset = (1<<downshift) - 1;
          offset += 0x8000 << downshift;
          if (is_signed)
            for (; width--; sp++, dp++)
              {
                val = (sp->ival + offset) >> downshift;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; sp++, dp++)
              {
                val = (sp->ival + offset) >> downshift;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
        }
      else if (line.is_absolute())
        {
          int upshift = 16-precision;
          if (is_signed)
            for (; width--; sp++, dp++)
              {
                val = sp->ival << upshift;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; sp++, dp++)
              {
                val = sp->ival << upshift;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16) val;
              }
        }
      else
        {
          if (is_signed)
            for (; width--; dp++, sp++)
              {
                val = (kdu_int32)(sp->fval * (float)(1<<24));
                val = (val + (1<<7)) >> 8;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; dp++, sp++)
              {
                val = (kdu_int32)(sp->fval * (float)(1<<24));
                val = (val + (1<<7)) >> 8;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16) val;
              }
        }
    }
}


/* ========================================================================= */
/*                               vex_processor                               */
/* ========================================================================= */

/*****************************************************************************/
/*                           vex_processor::reset                            */
/*****************************************************************************/

void
  vex_processor::reset()
{
  env = NULL;
  frame = NULL;
  block_truncation_factor = 0;
  double_buffering = false;
  processing_stripe_height = 0;
  num_components = 0;
  if (comp_info != NULL)
    delete[] comp_info;
  comp_info = NULL;
  current_tile = next_tile = NULL;
  env_batch_idx = 0;
  tiles[0].reset();
  tiles[1].reset();
}

/*****************************************************************************/
/*                        vex_processor::start_frame                         */
/*****************************************************************************/

void
  vex_processor::start_frame(vex_frame *frame, kdu_thread_env *env,
                             int double_buffering_height,
                             kdu_int32 truncation)
{
  int c;

  this->env = env;
  this->frame = frame;
  double_buffering = false;
  processing_stripe_height = 1;
  if ((env != NULL) && (double_buffering_height > 0))
    {
      double_buffering = true;
      processing_stripe_height = double_buffering_height;
    }
  num_components = frame->codestream.get_num_components(true);
  comp_info = new vex_comp_info[num_components];
  for (c=0; c < num_components; c++)
    {
      vex_comp_info *comp = comp_info + c;
      frame->codestream.get_dims(c,comp->comp_dims);
      kdu_coords subs; frame->codestream.get_subsampling(c,subs);
      comp->count_val = subs.y;
    }
  size_t comp_boff = 0;
  for (c=0; c < num_components; c++)
    {
      vex_comp_info *comp = comp_info + c;
      if (frame->stride != 0)
        { // Targetting an interleaved display buffer
          assert(frame->sample_bytes == 4);
          if (num_components == 1)
            comp->comp_boff = 0;
          else if (c == 3)
            comp->comp_boff = 3; // Alpha channel
          else
            comp->comp_boff = 2-c; // R, G or B channel
        }
      else
        { // Targetting a buffer which holds components one after the other
          comp->comp_boff = comp_boff;
          comp_boff += frame->sample_bytes * ((size_t) comp->comp_dims.area());
        }
    }
  assert((frame->stride != 0) || (comp_boff == frame->frame_bytes));
  if (current_tile == NULL)
    { // First frame opened since initialization; set up tile pointers
      current_tile = tiles;
      if (env == NULL)
        next_tile = NULL;
      else
        {
          next_tile = tiles+1;
          current_tile->next = next_tile;
          next_tile->next = current_tile;
        }
    }
  else if (env != NULL)
    next_tile = (current_tile = next_tile)->next;
  if ((truncation >= 0) && (truncation != block_truncation_factor))
    block_truncation_factor = truncation;
  frame->codestream.set_block_truncation(block_truncation_factor);
  if (!current_tile->is_active())
    init_tile(current_tile,frame);
  else
    assert((env != NULL) && (current_tile->frame == frame));
}

/*****************************************************************************/
/*                       vex_processor::process_frame                        */
/*****************************************************************************/

bool
  vex_processor::process_frame(int max_lines, vex_frame *next_frame,
                               kdu_int32 truncation)
{
  if (!current_tile->is_active())
    return true; // We should not have to get here, since it means that a
                 // previous call in the same frame already returned true.
  if ((truncation >= 0) && (truncation != block_truncation_factor))
    {
      block_truncation_factor = truncation;
      current_tile->frame->codestream.set_block_truncation(truncation);
      if (next_tile->is_active() && (next_tile->frame != current_tile->frame))
        next_tile->frame->codestream.set_block_truncation(truncation);
    }
  if ((env != NULL) && !next_tile->is_active())
    {
      if (!current_tile->last_in_frame)
        init_tile(next_tile,current_tile);
      else if (next_frame != NULL)
        {
          next_frame->codestream.set_block_truncation(block_truncation_factor);
          init_tile(next_tile,next_frame);
        }
    }

  // Now process the tile
  bool tile_done=false;
  int c, num_lines=0;
  while ((!tile_done) && (num_lines < max_lines))
    { // Decompress a line for each component whose counter is 0
      tile_done = true;
      kdu_line_buf *lines[4]={NULL,NULL,NULL,NULL};
      bool expand_mono_to_argb8 = false;
      for (c=0; c < num_components; c++)
        {
          vex_tcomp *comp = current_tile->components + c;
          if (comp->tile_rows_left == 0)
            continue;
          tile_done = false;
          comp->counter--;
          if (comp->counter >= 0)
            continue;
          comp->counter += comp_info[c].count_val;
          comp->tile_rows_left--;
          num_lines++;
          lines[c] = current_tile->engine.get_line(c,env);
#ifdef VEX_SIMD_OPTIMIZATIONS
          if (comp->simd_xfer_shorts_bytes && (kdu_mmx_level >= 2) &&
              (frame->stride == 0))
            {
              if (!frame->is_signed)
                simd_xfer_shorts_bytes_unsigned(lines[c]->get_buf16(),
                                                frame->buffer+comp->boff,
                                                comp->tile_size.x,
                                                comp->simd_xfer_downshift);
              else
                simd_xfer_shorts_bytes_signed(lines[c]->get_buf16(),
                                              frame->buffer+comp->boff,
                                              comp->tile_size.x,
                                              comp->simd_xfer_downshift);
              comp->boff += (size_t)(comp->comp_size.x);
              continue;
            }
#endif // VEX_SIMD_OPTIMIZATIONS
          if (frame->stride != 0)
            {
              assert(frame->sample_bytes <= 4);
#ifdef VEX_SIMD_OPTIMIZATIONS
              if (current_tile->simd_rgb_shorts_to_argb8 &&
                  (kdu_mmx_level >= 2))
                {
                  if (c == 2)
                    simd_rgb_shorts_to_argb8(lines[0]->get_buf16(),
                                             lines[1]->get_buf16(),
                                             lines[2]->get_buf16(),
                                             frame->buffer+comp->boff,
                                             comp->tile_size.x,
                                             current_tile->simd_downshift);
                }
              else if (current_tile->simd_mono_shorts_to_argb8 &&
                       (kdu_mmx_level >= 2))
                simd_mono_shorts_to_argb8(lines[0]->get_buf16(),
                                          frame->buffer+comp->boff,
                                          comp->tile_size.x,
                                          current_tile->simd_downshift);
              else
#endif // VEX_SIMD_OPTIMIZATIONS
                {
                  transfer_line_to_bytes(*(lines[c]),frame->buffer+comp->boff,
                                         comp->tile_size.x,comp->precision,
                                         false,(int) frame->sample_bytes);
                  expand_mono_to_argb8 = (num_components == 1);
                }
              comp->boff += frame->stride;
            }
          else if (frame->sample_bytes == 1)
            {
              transfer_line_to_bytes(*(lines[c]),frame->buffer+comp->boff,
                       comp->tile_size.x,comp->precision,frame->is_signed);
              comp->boff += (size_t)(comp->comp_size.x);
            }
          else if (frame->sample_bytes == 2)
            {
              transfer_line_to_words(*(lines[c]),frame->buffer+comp->boff,
                       comp->tile_size.x,comp->precision,frame->is_signed);
              comp->boff += (size_t)(comp->comp_size.x << 1);
            }
          else
            { kdu_error e; e << "This application currently supports "
              "output precisions of at most 16 bits per sample.  This could "
              "easily be extended to 32 bits if required."; }
        }
      if (expand_mono_to_argb8)
        { // We get here if we have written luminance information to the
          // blue channel of an ARGB frame buffer -- we need to copy this
          // to the other channels.
          assert(frame->stride != 0);
          kdu_byte *bp = (frame->buffer+current_tile->components[0].boff)
                        - frame->stride;
          for (int c=current_tile->components[0].tile_size.x; c>0; c--, bp+=4)
            {
              kdu_int32 val = *bp;
              val |= (val<<8);  val |= (val<<16);
              *((kdu_int32 *) bp) = val;
            }
        }
    }

  // See if we are done with this tile, and possibly with the entire frame
  if (tile_done)
    {
      close_tile(current_tile);
      if (current_tile->last_in_frame)
        return true; // The frame is complete
      if (env != NULL)
        next_tile = (current_tile = next_tile)->next;
      else
        init_tile(current_tile,current_tile);
    }
  return false;
}

/*****************************************************************************/
/*                    vex_processor::init_tile (frame)                       */
/*****************************************************************************/

void
  vex_processor::init_tile(vex_tile *tile, vex_frame *frame)
{
  assert(!(tile->engine.exists() || tile->ifc.exists()));
  tile->frame = frame;
  frame->codestream.get_valid_tiles(tile->valid_indices);
  tile->idx = kdu_coords();
  tile->last_in_frame = (tile->valid_indices.size == kdu_coords(1,1));
  tile->ifc = frame->codestream.open_tile(tile->valid_indices.pos,env);
  if (tile->components == NULL)
    tile->components = new vex_tcomp[num_components];
  if (env != NULL)
    tile->env_queue = env->add_queue(NULL,NULL,"tile root",env_batch_idx++);
  tile->engine.create(frame->codestream,tile->ifc,false,false,true,
                      processing_stripe_height,env,tile->env_queue,
                      double_buffering);
  for (int c=0; c < num_components; c++)
    {
      vex_tcomp *comp = tile->components + c;
      comp->comp_size = comp_info[c].comp_dims.size;
      comp->tile_size = tile->engine.get_size(c);
      comp->counter = 0;
      comp->precision = frame->codestream.get_bit_depth(c,true);
      comp->tile_rows_left = comp->tile_size.y;
      comp->boff = comp->next_tile_boff = comp_info[c].comp_boff;
      if (tile->idx.x < (tile->valid_indices.size.x-1))
        comp->next_tile_boff += frame->sample_bytes *
          ((size_t) comp->tile_size.x);
      else if (frame->stride == 0)
        comp->next_tile_boff += frame->sample_bytes *
          (((size_t) comp->tile_size.x) +
           ((size_t)(comp->tile_size.y-1))*((size_t) comp->comp_size.x));
      else
        comp->next_tile_boff += (frame->stride * (size_t) comp->tile_size.y) -
          frame->sample_bytes * (size_t)(comp->comp_size.x-comp->tile_size.x);
      if (comp->next_tile_boff > (frame->frame_bytes+3))
        { kdu_error e; e << "All frames in the compressed video sequence "
          "should have the same dimensions for each image component.  "
          "Encountered a frame for which one image component "
          "is larger than the corresponding component in the first frame."; }
      bool is_absolute = tile->engine.is_line_absolute(c);
      comp->using_shorts = !tile->engine.is_line_precise(c);
      comp->simd_xfer_downshift =
        (is_absolute)?(comp->precision-8):(KDU_FIX_POINT-8);
      comp->simd_xfer_shorts_bytes = (frame->stride == 0) &&
        comp->using_shorts && frame->fully_aligned &&
        (frame->sample_bytes == 1) && (comp->simd_xfer_downshift >= 0) &&
        (((comp->boff | comp->tile_size.x) & 15) == 0);
    }
  tile->simd_downshift = tile->components[0].simd_xfer_downshift;
  tile->simd_rgb_shorts_to_argb8 = (frame->stride != 0) &&
    frame->fully_aligned && (num_components == 3) &&
    tile->components[0].using_shorts && tile->components[1].using_shorts &&
    tile->components[2].using_shorts && (tile->simd_downshift >= 0) &&
    (tile->components[1].simd_xfer_downshift == tile->simd_downshift) &&
    (tile->components[2].simd_xfer_downshift == tile->simd_downshift);
  tile->simd_mono_shorts_to_argb8 = (frame->stride != 0) &&
    frame->fully_aligned && (num_components == 1) &&
    tile->components[0].using_shorts && (tile->simd_downshift >= 0);
}

/*****************************************************************************/
/*                     vex_processor::init_tile (tile)                       */
/*****************************************************************************/

bool
  vex_processor::init_tile(vex_tile *tile, vex_tile *ref_tile)
{
  assert(!(tile->engine.exists() || tile->ifc.exists()));
  if (ref_tile->last_in_frame)
    return false;
  vex_frame *frame = tile->frame = ref_tile->frame;
  tile->valid_indices = ref_tile->valid_indices;
  tile->idx = ref_tile->idx;
  tile->idx.x++;
  if (tile->idx.x >= tile->valid_indices.size.x)
    {
      tile->idx.x = 0;
      tile->idx.y++;
      assert(tile->idx.y < tile->valid_indices.size.y);
    }
  tile->last_in_frame =
    (tile->idx.x == (tile->valid_indices.size.x-1)) &&
    (tile->idx.y == (tile->valid_indices.size.y-1));
  tile->ifc =
    frame->codestream.open_tile(tile->valid_indices.pos+tile->idx,env);
  if (tile->components == NULL)
    tile->components = new vex_tcomp[num_components];
  if (env != NULL)
    tile->env_queue = env->add_queue(NULL,NULL,"tile root",env_batch_idx++);
  tile->engine.create(frame->codestream,tile->ifc,false,false,true,
                      processing_stripe_height,env,tile->env_queue,
                      double_buffering);
  for (int c=0; c < num_components; c++)
    {
      vex_tcomp *comp = tile->components + c;
      vex_tcomp *ref_comp = ref_tile->components + c;
      comp->comp_size = ref_comp->comp_size;
      comp->tile_size = tile->engine.get_size(c);
      comp->counter = 0;
      comp->precision = ref_comp->precision;
      comp->tile_rows_left = comp->tile_size.y;
      comp->boff = comp->next_tile_boff = ref_comp->next_tile_boff;
      if (tile->idx.x < (tile->valid_indices.size.x-1))
        comp->next_tile_boff += frame->sample_bytes *
          ((size_t) comp->tile_size.x);
      else if (frame->stride == 0)
        comp->next_tile_boff += frame->sample_bytes *
          (((size_t) comp->tile_size.x) +
           ((size_t)(comp->tile_size.y-1)) * ((size_t) comp->comp_size.x));
      else
        comp->next_tile_boff += (frame->stride * (size_t) comp->tile_size.y) -
          frame->sample_bytes * (size_t)(comp->comp_size.x-comp->tile_size.x);
      if (comp->next_tile_boff > (frame->frame_bytes+3))
        { kdu_error e; e << "All frames in the compressed video sequence "
          "should have the same dimensions for each image component.  "
          "Encountered a frame for which one image component "
          "is larger than the corresponding component in the first frame."; }
      bool is_absolute = tile->engine.is_line_absolute(c);
      comp->using_shorts = !tile->engine.is_line_precise(c);
      comp->simd_xfer_downshift =
        (is_absolute)?(comp->precision-8):(KDU_FIX_POINT-8);
      comp->simd_xfer_shorts_bytes = (frame->stride == 0) &&
        frame->fully_aligned && comp->using_shorts &&
        (frame->sample_bytes == 1) && (comp->simd_xfer_downshift >= 0) &&
        (((comp->boff | comp->tile_size.x) & 15) == 0);
    }
  tile->simd_downshift = tile->components[0].simd_xfer_downshift;
  tile->simd_rgb_shorts_to_argb8 = (frame->stride != 0) &&
    frame->fully_aligned && (num_components == 3) &&
    tile->components[0].using_shorts && tile->components[1].using_shorts &&
    tile->components[2].using_shorts && (tile->simd_downshift >= 0) &&
    (tile->components[1].simd_xfer_downshift == tile->simd_downshift) &&
    (tile->components[2].simd_xfer_downshift == tile->simd_downshift);
  tile->simd_mono_shorts_to_argb8 = (frame->stride != 0) &&
    frame->fully_aligned && (num_components == 1) &&
    tile->components[0].using_shorts && (tile->simd_downshift >= 0);
  return true;
}

/*****************************************************************************/
/*                        vex_processor::close_tile                          */
/*****************************************************************************/

void
  vex_processor::close_tile(vex_tile *tile)
{
  if (env != NULL)
    env->terminate(tile->env_queue,false);
  tile->engine.destroy();
  tile->ifc.close(env);
}


/* ========================================================================= */
/*                              vex_frame_queue                              */
/* ========================================================================= */

/*****************************************************************************/
/*                            vex_frame_queue::init                          */
/*****************************************************************************/

bool
  vex_frame_queue::init(mj2_video_source *source, int discard_levels,
                        int max_components, int repeat_factor,
                        int max_active_frames, bool want_display)
{
  if (this->source != NULL)
    { kdu_error e; e << "Attempting to call `vex_frame_queue::init' on an "
      "object which is already initialized."; }

  this->discard_levels = discard_levels;
  source->seek_to_frame(0);
  if (source->open_image() < 0)
    return false;
  kdu_codestream codestream;
  codestream.create(source); // Create temporary codestream
  source->close_image(); // Temporary codestream does not use source further
  source->seek_to_frame(0);
  codestream.apply_input_restrictions(0,max_components,discard_levels,0,NULL);
  num_components = codestream.get_num_components();
  if (want_display & (num_components != 1) && (num_components != 3) &&
      (num_components != 4))
    { kdu_error e; e << "The `-display' option is only available for "
      "displaying 1, 3 or 4 component frames.  You might need to use the "
      "`-components' argument to limit the number of components."; }
  is_signed = (!want_display) && codestream.get_signed(0);
  precision = codestream.get_bit_depth(0);

  codestream.get_dims(-1,frame_dims);
  kdu_coords min=frame_dims.pos, lim=min+frame_dims.size;
  min.x = (min.x+1)>>discard_levels;  min.y = (min.y+1)>>discard_levels;
  lim.x = (lim.x+1)>>discard_levels;  lim.y = (lim.y+1)>>discard_levels;
  frame_dims.pos = min; frame_dims.size = lim - min;
  if (component_subs != NULL)
    { delete[] component_subs; component_subs = NULL; }
  component_subs = new kdu_coords[num_components];
  if (component_dims != NULL)
    { delete[] component_dims; component_dims = NULL; }
  component_dims = new kdu_dims[num_components];

  if (want_display)
    {
      frame_format = VEX_FRAME_FORMAT_XRGB;
      sample_bytes = 4;
    }
  else
    {
      frame_format = VEX_FRAME_FORMAT_CONTIG;
      sample_bytes = 1;
      if (precision > 8)
        sample_bytes = 2;
      if (precision > 16)
        sample_bytes = 4;
    }
  for (int c=0; c < num_components; c++)
    {
      codestream.get_subsampling(c,component_subs[c],true);
      component_subs[c].x >>= discard_levels;
      component_subs[c].y >>= discard_levels;
      codestream.get_dims(c,component_dims[c],true);
      if ((frame_format != VEX_FRAME_FORMAT_CONTIG) && (c > 0) &&
          (component_subs[0] != component_subs[c]))
        { kdu_error e; e << "The `-display' argument can be used only "
          "when all image components have identical dimensions.  This is "
          "not a limitation of JPEG2000 or Kakadu, but it helps to keep "
          "this demo application as simple as possible.  Try limiting "
          "the number of components to be displayed using the "
          "`-components' argument."; }
    }
  codestream.destroy(); // Destroy temporary codestream

  repeat_count = repeat_factor;
  next_frame_to_open = 0;
  this->max_active_frames = max_active_frames;
  if (!(mutex.create() &&
        active_waiting.create(true) &&
        active_ready.create(false)))
    { kdu_error e; e << "Unable to create thread synchronization objects.  "
      "Perhaps this application was not compiled against a suitable "
      "threads implementation."; }
  this->source = source; // Marks the source as initialized
  return true;
}

/*****************************************************************************/
/*                      vex_frame_queue::~vex_frame_queue                    */
/*****************************************************************************/

vex_frame_queue::~vex_frame_queue()
{
  mutex.destroy();
  active_waiting.destroy();
  active_ready.destroy();
  if (component_subs != NULL)
    delete[] component_subs;
  if (component_dims != NULL)
    delete[] component_dims;
  while ((active_tail=active_head) != NULL)
    { active_head=active_tail->next; delete active_tail; }
  while ((processed_tail=processed_head) != NULL)
    { processed_head=processed_tail->next; delete processed_tail; }
  vex_frame *frm;
  while ((frm=recycled_frames) != NULL)
    { recycled_frames=frm->next; delete frm; }
}

/*****************************************************************************/
/*                    vex_frame_queue::get_processed_frame                   */
/*****************************************************************************/

vex_frame *
  vex_frame_queue::get_processed_frame(vex_frame_memory_allocator *allocator)
{
  mutex.lock();
  while ((num_active_frames < max_active_frames) && !exhausted)
    { // Need to load another frame
      vex_frame *frm = recycled_frames;
      if (frm == NULL)
        {
          frm = new vex_frame;
          frm->next = recycled_frames;
          recycled_frames = frm;
          frm->stride = frm->frame_bytes = 0;
          frm->fully_aligned = false;
          frm->sample_bytes = this->sample_bytes;
          frm->precision = this->precision;
          frm->is_signed = this->is_signed;
          frm->buffer_handle = NULL; // For the moment
          frm->buffer = NULL;
          frm->allocator = allocator;
        }
      assert(!frm->box);
      mutex.unlock();
      if (frm->buffer_handle == NULL)
        frm->buffer_handle =
          allocator->allocate_mem(num_components,component_dims,sample_bytes,
                                  frame_format,frm->buffer,frm->frame_bytes,
                                  frm->stride,frm->fully_aligned);
      while ((!source->seek_to_frame(next_frame_to_open)) ||
             (source->open_stream(0,&(frm->box)) != next_frame_to_open))
        {
          repeat_count--;
          if ((repeat_count == 0) || (next_frame_to_open == 0))
            { exhausted = true; break; }
          next_frame_to_open = 0;
        }
      if (!frm->box)
        {
          mutex.lock();
          break;
        }
      next_frame_to_open++;
      if (!frm->box.load_in_memory(INT_MAX))
        { kdu_error e; e << "Unable to load compressed frame into memory -- "
          "it might be too big."; }
      if (!frm->codestream)
        {
          frm->codestream.create(&(frm->box));
          frm->codestream.enable_restart();
        }
      else
        frm->codestream.restart(&(frm->box));
      frm->codestream.apply_input_restrictions(0,num_components,
                                               discard_levels,0,NULL);
      mutex.lock();
      recycled_frames = frm->next;
      frm->next = NULL;
      frm->waiting = true;
      if (active_tail == NULL)
        active_head = active_tail = frm;
      else
        active_tail = active_tail->next = frm;
      num_active_frames++;
      if (num_waiting_frames == 0)
        active_waiting.set();
      num_waiting_frames++;
    }
  if ((active_head == NULL) || terminated)
    {
      mutex.unlock();
      return NULL;
    }
  while ((active_head->waiting || (active_head->engine != NULL)) &&
         !exception_raised)
    active_ready.wait(mutex);
  vex_frame *result = NULL;
  if (!exception_raised)
    {
      result = active_head;
      if ((active_head = result->next) == NULL)
        active_tail = NULL;
      result->next = NULL;
      num_active_frames--;
      if (processed_tail == NULL)
        processed_head = processed_tail = result;
      else
        processed_tail = processed_tail->next = result;
    }
  mutex.unlock();
  if (exception_raised)
    kdu_rethrow(last_exception_code);
  return result;
}

/*****************************************************************************/
/*                   vex_frame_queue::recycle_processed_frame                */
/*****************************************************************************/

void
  vex_frame_queue::recycle_processed_frame(vex_frame *frame)
{
  if (frame != processed_head)
    { kdu_error e; e << "Calls to `vex_frame_queue::recycle_processed_frame' "
      "must recycle frames in the same order in which they were originally "
      "retrieved via `vex_frame_queue::get_processed_frame'."; }
  mutex.lock();
  if ((processed_head = frame->next) == NULL)
    processed_tail = NULL;
  frame->next = recycled_frames;
  recycled_frames = frame;
  mutex.unlock();
}

/*****************************************************************************/
/*                     vex_frame_queue::get_frame_to_process                 */
/*****************************************************************************/

vex_frame *
  vex_frame_queue::get_frame_to_process(vex_engine *engine, bool blocking)
{
  vex_frame *result=NULL;

  mutex.lock();
  while ((num_waiting_frames == 0) && blocking && !(exhausted || terminated))
    {
      active_waiting.reset();
      active_waiting.wait(mutex);
    }
  if ((num_waiting_frames > 0) && !terminated)
    {
      for (result=active_head; result != NULL; result=result->next)
        if (result->waiting)
          break;
      if (result == NULL)
        { mutex.unlock();
          kdu_error e; e << "Fatal internal error in `vex_frame_queue'.  "
            "Call to `vex_frame_queue::get_frame_to_process' encountered "
            "a corrupted queue.";
        }
      result->waiting = false;
      result->engine = engine;
      num_waiting_frames--;
    }
  mutex.unlock();
  return result;
}

/*****************************************************************************/
/*                       vex_frame_queue::frame_processed                    */
/*****************************************************************************/

void
  vex_frame_queue::frame_processed(vex_frame *frame)
{
  assert((frame->engine != NULL) && !frame->waiting);
  frame->box.close(); // In case it is not already closed
  mutex.lock();
  frame->engine = NULL;
  active_ready.set();
  mutex.unlock();
}

/*****************************************************************************/
/*                       vex_frame_queue::frame_aborted                      */
/*****************************************************************************/

void
  vex_frame_queue::frame_aborted(vex_frame *frame,
                                 kdu_exception exception_code)
{
  assert((frame->engine != NULL) && !frame->waiting);
  mutex.lock();
  exception_raised = true;
  last_exception_code = exception_code;
  frame->aborted = true;
  frame->engine = NULL;
  active_ready.set();
  mutex.unlock();
}

/*****************************************************************************/
/*                        vex_frame_queue::terminate                         */
/*****************************************************************************/

void
  vex_frame_queue::terminate()
{
  mutex.lock();
  terminated = true;
  active_waiting.set(); // Starts the wakeup process for blocked engine threads
  mutex.unlock();
}

/*****************************************************************************/
/*                       vex_frame_queue::allocate_mem                       */
/*****************************************************************************/

void *
  vex_frame_queue::allocate_mem(int num_components, kdu_dims *component_dims,
                                size_t sample_bytes, int format,
                                kdu_byte * &buffer, size_t &frame_bytes,
                                size_t &stride, bool &fully_aligned)
{
  int c;
  buffer = NULL;
  if (format == VEX_FRAME_FORMAT_CONTIG)
    {
      stride = 0;
      fully_aligned = true; // May be proved wrong later
      frame_bytes = 0;
      for (c=0; c < num_components; c++)
        {
          frame_bytes += sample_bytes * (size_t) component_dims[c].area();
          if (component_dims[c].size.x & 15)
            fully_aligned = false;
        }
    }
  else if (format == VEX_FRAME_FORMAT_XRGB)
    {
      if ((num_components > 4) || (sample_bytes != 4))
        { kdu_error e;
          e << "Invalid call to `vex_frame_queue::allocate_mem'."; }
      stride = ((size_t) component_dims[0].size.x)<<2;
      stride += (16-stride) & 15;
      frame_bytes = stride * (size_t) component_dims[0].size.y;
      fully_aligned = true;
    }
  else
    { kdu_error e; e << "Invalid call to `vex_frame_queue::allocate_mem'."; }

  kdu_byte *handle =
    (kdu_byte *) malloc(frame_bytes+15); // Allow for alignment
  if (handle == NULL)
    { kdu_error e; e << "Insufficient memory available to allocate "
      "frame buffers."; }
  buffer = handle + ((-(int)(_addr_to_kdu_long(handle))) & 15);
  return handle;
}

/*****************************************************************************/
/*                       vex_frame_queue::destroy_mem                        */
/*****************************************************************************/

void
  vex_frame_queue::destroy_mem(void *handle)
{
  free(handle);
}

/* ========================================================================= */
/*                                vex_engine                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                             engine_startproc                              */
/*****************************************************************************/

kdu_thread_startproc_result
  KDU_THREAD_STARTPROC_CALL_CONVENTION engine_startproc(void *param)
{
  vex_engine *self = (vex_engine *) param;
#if (defined _WIN32 && defined _DEBUG && defined _MSC_VER && (_MSC_VER>=1300))
  kd_set_threadname(self->master_thread_name);
#endif // .NET debug compilation
  if (self->num_threads <= 1)
    self->run_single_threaded();
  else
    self->run_multi_threaded();
  return KDU_THREAD_STARTPROC_ZERO_RESULT;
}

/*****************************************************************************/
/*                              vex_engine::startup                          */
/*****************************************************************************/

void
  vex_engine::startup(vex_frame_queue *queue, int engine_idx,
                      int num_threads, kdu_long cpu_affinity,
                      int thread_concurrency)
{
  sprintf(master_thread_name,"Master thread for engine %d",engine_idx);
  this->thread_concurrency = thread_concurrency;
  this->num_threads = num_threads;
  if (cpu_affinity < 0)
    cpu_affinity = 0; // Threads can run on any CPU
  this->cpu_affinity = cpu_affinity;

  if (num_threads < 1)
    { kdu_error e;
      e << "Engine " << engine_idx << " is not assigned any threads -- "
           "looks like an implementation error."; }
  this->queue = queue;
  if (!master_thread.create(engine_startproc,(void *) this))
    { shutdown(false);
      kdu_error e;
      e << "Unable to start master thread for engine " << engine_idx << "."; }
}

/*****************************************************************************/
/*                              vex_engine::shutdown                         */
/*****************************************************************************/

void
  vex_engine::shutdown(bool graceful)
{
  if (queue == NULL)
    return;
  if (graceful)
    graceful_shutdown_requested = true;
  else
    immediate_shutdown_requested = true;
  if (blocked)
    queue->terminate();
  if (master_thread.exists())
    master_thread.destroy(); // Blocks the caller until the thread terminates
  processor.reset();
  queue = NULL;
  blocked = graceful_shutdown_requested = immediate_shutdown_requested = false;
  num_threads = 0;
  cpu_affinity = 0;
  master_thread_name[0] = '\0';
}

/*****************************************************************************/
/*                       vex_engine::run_single_threaded                     */
/*****************************************************************************/

void
  vex_engine::run_single_threaded()
{
  if (cpu_affinity != 0)
    master_thread.set_cpu_affinity(cpu_affinity);
  while (!(immediate_shutdown_requested || graceful_shutdown_requested))
    {
      blocked = true;
      vex_frame *frame = queue->get_frame_to_process(this,true);
      blocked = false;
      if ((frame == NULL) || immediate_shutdown_requested)
        break;
      try {
          processor.start_frame(frame,NULL,0,queue->get_truncation_factor());
          do {
              if (immediate_shutdown_requested)
                break;
            } while (!processor.process_frame(256,NULL,
                                              queue->get_truncation_factor()));
        }
      catch (kdu_exception exc) {
          queue->frame_aborted(frame,exc);
          break;
        }
      catch (std::bad_alloc &) {
          queue->frame_aborted(frame,KDU_MEMORY_EXCEPTION);
          break;
        }
      if (immediate_shutdown_requested)
        break;
      queue->frame_processed(frame);
    }
}

/*****************************************************************************/
/*                       vex_engine::run_multi_threaded                      */
/*****************************************************************************/

void
  vex_engine::run_multi_threaded()
{
  kdu_thread_env env;
  env.create(cpu_affinity);
  for (int nt=1; nt < num_threads; nt++)
    if (!env.add_thread(thread_concurrency))
      num_threads = nt;

  vex_frame *frame=NULL, *next_frame=NULL;
  while (!immediate_shutdown_requested)
    {
      frame = next_frame;   next_frame = NULL;
      if ((frame == NULL) && !graceful_shutdown_requested)
        {
          blocked = true;
          frame = queue->get_frame_to_process(this,true);
          blocked = false;
        }
      if ((frame == NULL) || immediate_shutdown_requested)
        break;
      try {
          processor.start_frame(frame,&env,0,queue->get_truncation_factor());
          do {
              if (immediate_shutdown_requested)
                break;
              if ((next_frame == NULL) && !graceful_shutdown_requested)
                next_frame = queue->get_frame_to_process(this,false);
            } while (!processor.process_frame(256,next_frame,
                                              queue->get_truncation_factor()));
        }
      catch (kdu_exception exc) {
          queue->frame_aborted(frame,exc);
          break;
        }
      catch (std::bad_alloc &) {
          queue->frame_aborted(frame,KDU_MEMORY_EXCEPTION);
          break;
        }
      if (immediate_shutdown_requested)
        break;
      queue->frame_processed(frame);
    }
  env.destroy();
}
