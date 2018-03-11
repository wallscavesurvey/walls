/*****************************************************************************/
// File: x86_region_compositor_local.h [scope = CORESYS/TRANSFORMS]
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
   Implements critical layer composition and alpha blending functions using
MMX intrinsics.  These can be compiled under GCC or .NET and are compatible
with both 32-bit and 64-bit builds.
******************************************************************************/

#ifndef X86_REGION_COMPOSITOR_LOCAL_H
#define X86_REGION_COMPOSITOR_LOCAL_H

#include <emmintrin.h>

/* ========================================================================= */
/*                        Now for the MMX functions                          */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                    simd_erase_region (32-bit)                      */
/*****************************************************************************/

inline bool
  simd_erase_region(kdu_uint32 *dst, int height, int width, int row_gap,
                    kdu_uint32 erase)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 8))
    return false;
  __m128i val = _mm_set1_epi32((kdu_int32) erase);
  for (; height > 0; height--, dst+=row_gap)
    {
      kdu_uint32 *dp = dst;
      int left = (-(_addr_to_kdu_int32(dp) >> 2)) & 3;
      int octets = (width-left)>>3;
      int c, right = width - left - (octets<<3);
      for (c=left; c > 0; c--)
        *(dp++) = erase;
      for (c=octets; c > 0; c--, dp+=8)
        { ((__m128i *) dp)[0] = val; ((__m128i *) dp)[1] = val; }
      for (c=right; c > 0; c--)
        *(dp++) = erase;
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                simd_erase_region (floating point)                  */
/*****************************************************************************/

inline bool
  simd_erase_region(float *dst, int height, int width, int row_gap,
                    float erase[])
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 8) ||
      (_addr_to_kdu_int32(dst) & 15) || (row_gap & 3))
    return false;
  __m128 val = _mm_loadu_ps(erase);
  for (; height > 0; height--, dst+=row_gap)
    {
      __m128 *dp = (__m128 *) dst;
      for (int c=width; c > 0; c--, dp++)
        *dp = val;
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                  simd_copy_region (32-bit)                         */
/*****************************************************************************/

inline bool
  simd_copy_region(kdu_uint32 *dst, kdu_uint32 *src,
                   int height, int width, int dst_row_gap, int src_row_gap)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 8))
    return false;
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      kdu_uint32 *dp = dst;
      kdu_uint32 *sp = src;
      int left = (-(_addr_to_kdu_int32(dp) >> 2)) & 3;
      int octets = (width-left)>>3;
      int c, right = width - left - (octets<<3);
      for (c=left; c > 0; c--)
        *(dp++) = *(sp++);
      for (c=octets; c > 0; c--, dp+=8, sp+=8)
        {
          __m128i val0 = _mm_loadu_si128((__m128i *) sp);
          __m128i val1 = _mm_loadu_si128((__m128i *)(sp+4));
          ((__m128i *) dp)[0] = val0;
          ((__m128i *) dp)[1] = val1;
        }
      for (c=right; c > 0; c--)
        *(dp++) = *(sp++);
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE               simd_copy_region (floating point)                    */
/*****************************************************************************/

inline bool
  simd_copy_region(float *dst, float *src,
                   int height, int width, int dst_row_gap, int src_row_gap)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 8) ||
      (_addr_to_kdu_int32(dst) & 15) || (_addr_to_kdu_int32(src) & 15) ||
      (src_row_gap & 3) || (dst_row_gap & 3))
    return false;
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      __m128 *dp = (__m128 *) dst;
      __m128 *sp = (__m128 *) src;
      for (int c=width; c > 0; c--, dp++, sp++)
        *dp = *sp;
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                 simd_blend_region (32-bit)                         */
/*****************************************************************************/

inline bool
  simd_blend_region(kdu_uint32 *dst, kdu_uint32 *src,
                    int height, int width, int dst_row_gap, int src_row_gap)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 4))
    return false;

  // Create all-zero double quad-word
  __m128i zero = _mm_setzero_si128();

  // Create a mask containing 0x00FF in the alpha word position of each
  // quad-word.  We will use this in computing the adjusted value of the
  // background alpha after blending -- correct blending modifies both alpha
  // and colour channels.
  __m128i mask = _mm_cmpeq_epi16(zero,zero);
  mask = _mm_slli_epi64(mask,56);
  mask = _mm_srli_epi64(mask,8);

  // Now for the processing loop
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      kdu_uint32 *sp = src;
      kdu_uint32 *dp = dst;
      int left = (-(_addr_to_kdu_int32(dp) >> 2)) & 3;
      int c, quads = (width-left)>>2;
      int right = width - left - (quads<<2);
      for (c=left; c > 0; c--, sp++, dp++)
        {
          // Load 1 source pixel and 1 target pixel
          __m128i src_val = _mm_cvtsi32_si128((kdu_int32) *sp);
          __m128i dst_val = _mm_cvtsi32_si128((kdu_int32) *dp);

          // Find normalized alpha factor in the range 0 to 2^14, inclusive
          __m128i alpha = _mm_srli_epi32(src_val,24); // Get alpha only
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14.

          // Unpack source and target pixels into words
          src_val = _mm_unpacklo_epi8(src_val,zero);
          dst_val = _mm_unpacklo_epi8(dst_val,zero);

          // Copy the alpha factor into all word positions
          __m128i factors = _mm_shufflelo_epi16(alpha,0);

          // Get difference between source and target values, then scale and
          // add this difference back into the target value; the only twist
          // here is for the alpha component, where we take 255 - target.
          src_val = _mm_or_si128(src_val,mask); // Set's source alpha to 255
          __m128i diff = _mm_sub_epi16(src_val,dst_val);
          diff = _mm_slli_epi16(diff,2); // Because max alpha factor is 2^14
          diff = _mm_mulhi_epi16(diff,factors);
          dst_val = _mm_add_epi16(dst_val,diff);

          // Finally, pack words into bytes and save the pixel
          dst_val = _mm_packus_epi16(dst_val,dst_val);
          *dp = (kdu_uint32) _mm_cvtsi128_si32(dst_val);
        }
      for (c=quads; c > 0; c--, sp+=4, dp+=4)
        {
          // Load 4 source pixels and 4 target pixels
          __m128i src_val = _mm_loadu_si128((__m128i *) sp);
          __m128i dst_val = *((__m128i *) dp);

          // Find normalized alpha factors from 4 source pels
          __m128i alpha = _mm_srli_epi32(src_val,24);
                     // Leaves 8-bit alpha only in each pixel's DWORD
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14

          // Unpack source and target pixels into words
          __m128i src_low = _mm_unpacklo_epi8(src_val,zero);
          __m128i src_high = _mm_unpackhi_epi8(src_val,zero);
          __m128i dst_low = _mm_unpacklo_epi8(dst_val,zero);
          __m128i dst_high = _mm_unpackhi_epi8(dst_val,zero);

          // Unpack and arrange alpha factors so that red, green, blue and
          // alpha word positions all have the same alpha factor.
          __m128i factors_low = _mm_unpacklo_epi32(alpha,zero);
          __m128i factors_high = _mm_unpackhi_epi32(alpha,zero);
          factors_low = _mm_shufflelo_epi16(factors_low,0);
          factors_low = _mm_shufflehi_epi16(factors_low,0);
          factors_high = _mm_shufflelo_epi16(factors_high,0);
          factors_high = _mm_shufflehi_epi16(factors_high,0);

          // Get difference between source and target values, then scale and
          // add this difference back into the target value; the only twist
          // here is for the alpha component, where we take 255 - target.
          src_low = _mm_or_si128(src_low,mask); // Sets source alpha to 255
          src_high = _mm_or_si128(src_high,mask); // Sets source alpha to 255
          __m128i diff = _mm_sub_epi16(src_low,dst_low);
          diff = _mm_slli_epi16(diff,2); // Because max alpha factor is 2^14
          diff = _mm_mulhi_epi16(diff,factors_low);
          dst_low = _mm_add_epi16(dst_low,diff);
          diff = _mm_sub_epi16(src_high,dst_high);
          diff = _mm_slli_epi16(diff,2); // Because max alpha factor is 2^14
          diff = _mm_mulhi_epi16(diff,factors_high);
          dst_high = _mm_add_epi16(dst_high,diff);
    
          // Finally, pack `dst_low' and `dst_high' into bytes and save
          *((__m128i *) dp) = _mm_packus_epi16(dst_low,dst_high);
        }
      for (c=right; c > 0; c--, sp++, dp++)
        {
          // Load 1 source pixel and 1 target pixel
          __m128i src_val = _mm_cvtsi32_si128((kdu_int32) *sp);
          __m128i dst_val = _mm_cvtsi32_si128((kdu_int32) *dp);

          // Find normalized alpha factor in the range 0 to 2^14, inclusive
          __m128i alpha = _mm_srli_epi32(src_val,24); // Get alpha only
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14.

          // Unpack source and target pixel into words
          src_val = _mm_unpacklo_epi8(src_val,zero);
          dst_val = _mm_unpacklo_epi8(dst_val,zero);

          // Copy the alpha factor into all word positions
          __m128i factors = _mm_shufflelo_epi16(alpha,0);

          // Get difference between source and target values, then scale and
          // add this difference back into the target value; the only twist
          // here is for the alpha component, where we take 255 - target.
          src_val = _mm_or_si128(src_val,mask); // Set's source alpha to 255
          __m128i diff = _mm_sub_epi16(src_val,dst_val);
          diff = _mm_slli_epi16(diff,2); // Because max alpha factor is 2^14
          diff = _mm_mulhi_epi16(diff,factors);
          dst_val = _mm_add_epi16(dst_val,diff);

          // Finally, pack words into bytes and save the pixel
          dst_val = _mm_packus_epi16(dst_val,dst_val);
          *dp = (kdu_uint32) _mm_cvtsi128_si32(dst_val);
        }
    }
  return true;
#endif //!KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE             simd_blend_region (floating point)                     */
/*****************************************************************************/

inline bool
  simd_blend_region(float *dst, float *src,
                    int height, int width, int dst_row_gap, int src_row_gap)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 4) ||
      (_addr_to_kdu_int32(dst) & 15) || (_addr_to_kdu_int32(src) & 15) ||
      (src_row_gap & 3) || (dst_row_gap & 3))
    return false;
  
  __m128 one_val = _mm_set1_ps(1.0F);
  
  // Now for the processing loop
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      __m128 *sp = (__m128 *) src;
      __m128 *dp = (__m128 *) dst;
      for (int c=width; c > 0; c--, sp++, dp++)
        {
          __m128 src_val = *sp;
          __m128 dst_val = *dp;
          __m128 alpha = _mm_shuffle_ps(src_val,src_val,0); // Replicates alpha
          src_val = _mm_move_ss(src_val,one_val);
          __m128 diff = _mm_sub_ps(src_val,dst_val);
          diff = _mm_mul_ps(diff,alpha);
          *dp = _mm_add_ps(dst_val,diff);
        }
    }
  return true;
#endif //!KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE              simd_scaled_blend_region (32-bit)                     */
/*****************************************************************************/

inline bool
  simd_scaled_blend_region(kdu_uint32 *dst, kdu_uint32 *src,
                           int height, int width, int dst_row_gap,
                           int src_row_gap, kdu_int16 alpha_factor_x128)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 4))
    return false;

  // Create all-zero double quad-word
  __m128i zero = _mm_setzero_si128();

  // Create a mask containing 0x00FF in the alpha word position of each
  // quad-word.  We will use this in computing the adjusted value of the
  // background alpha after blending -- correct blending modifies both alpha
  // and colour channels.
  __m128i mask = _mm_cmpeq_epi16(zero,zero);
  mask = _mm_slli_epi64(mask,56);
  mask = _mm_srli_epi64(mask,8);
  
  // Create an XOR mask to handle negative alpha factors
  __m128i xor_mask = zero;
  if (alpha_factor_x128 < 0)
    {
      alpha_factor_x128 = -alpha_factor_x128;
      xor_mask = _mm_set1_epi32(0x00FFFFFF);
    }
  
  // Create 4 copies of -`alpha_factor_x128' in a 128-bit vector
  __m128i neg_alpha_scale = _mm_set1_epi32(-alpha_factor_x128);

  // Now for the processing loop
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      kdu_uint32 *sp = src;
      kdu_uint32 *dp = dst;
      int left = (-(_addr_to_kdu_int32(dp) >> 2)) & 3;
      int c, quads = (width-left)>>2;
      int right = width - left - (quads<<2);
      for (c=left; c > 0; c--, sp++, dp++)
        {
          // Load 1 source pixel and 1 target pixel
          __m128i src_val = _mm_cvtsi32_si128((kdu_int32) *sp);
          __m128i dst_val = _mm_cvtsi32_si128((kdu_int32) *dp);
          src_val = _mm_xor_si128(src_val,xor_mask);

          // Find normalized alpha factor in the range 0 to 2^14, inclusive
          __m128i alpha = _mm_srli_epi32(src_val,24); // Get alpha only
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14.
          
          // Scale and clip the normalized alpha values
          alpha = _mm_madd_epi16(alpha,neg_alpha_scale);
          alpha = _mm_srai_epi32(alpha,6); // Nom. range of alpha -> 0 to -2^15
          alpha = _mm_packs_epi32(alpha,alpha); // Saturates and packs, 2 copies

          // Unpack source and target pixels into words
          src_val = _mm_unpacklo_epi8(src_val,zero);
          dst_val = _mm_unpacklo_epi8(dst_val,zero);

          // Copy the alpha factor into all word positions
          __m128i factors = _mm_shufflelo_epi16(alpha,0);

          // Get difference between source and target values, then scale and
          // add this difference back into the target value; the only twist
          // here is for the alpha component, where we take 255 - target.
          src_val = _mm_or_si128(src_val,mask); // Set's source alpha to 255
          __m128i diff = _mm_sub_epi16(src_val,dst_val);
          diff = _mm_add_epi16(diff,diff); // Because max alpha factor is 2^15
          diff = _mm_mulhi_epi16(diff,factors);
          dst_val = _mm_sub_epi16(dst_val,diff); // Subtract because alpha -ve

          // Finally, pack words into bytes and save the pixel
          dst_val = _mm_packus_epi16(dst_val,dst_val);
          *dp = (kdu_uint32) _mm_cvtsi128_si32(dst_val);
        }
      for (c=quads; c > 0; c--, sp+=4, dp+=4)
        {
          // Load 4 source pixels and 4 target pixels
          __m128i src_val = _mm_loadu_si128((__m128i *) sp);
          __m128i dst_val = *((__m128i *) dp);
          src_val = _mm_xor_si128(src_val,xor_mask);

          // Find normalized alpha factors from 4 source pels
          __m128i alpha = _mm_srli_epi32(src_val,24);
                     // Leaves 8-bit alpha only in each pixel's DWORD
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14
          
          // Scale and clip the normalized alpha values
          alpha = _mm_madd_epi16(alpha,neg_alpha_scale);
          alpha = _mm_srai_epi32(alpha,6); // Nom. range of alpha -> 0 to -2^15
          alpha = _mm_packs_epi32(alpha,alpha); // Saturates and packs, 2 copies
          
          // Unpack source and target pixels into words
          __m128i src_low = _mm_unpacklo_epi8(src_val,zero);
          __m128i src_high = _mm_unpackhi_epi8(src_val,zero);
          __m128i dst_low = _mm_unpacklo_epi8(dst_val,zero);
          __m128i dst_high = _mm_unpackhi_epi8(dst_val,zero);

          // Unpack and arrange alpha factors so that red, green, blue and
          // alpha word positions all have the same alpha factor.
          __m128i factors_low, factors_high;
          factors_low = _mm_shufflelo_epi16(alpha,0x00);
          factors_low = _mm_shufflehi_epi16(factors_low,0x55);
          factors_high = _mm_shufflelo_epi16(alpha,0xAA);
          factors_high = _mm_shufflehi_epi16(factors_high,0xFF);

          // Get difference between source and target values, then scale and
          // add this difference back into the target value; the only twist
          // here is for the alpha component, where we take 255 - target.
          src_low = _mm_or_si128(src_low,mask); // Sets source alpha to 255
          src_high = _mm_or_si128(src_high,mask); // Sets source alpha to 255
          __m128i diff = _mm_sub_epi16(src_low,dst_low);
          diff = _mm_add_epi16(diff,diff); // Because max alpha factor is 2^15
          diff = _mm_mulhi_epi16(diff,factors_low);
          dst_low = _mm_sub_epi16(dst_low,diff); // Subtract because alpha -ve
          diff = _mm_sub_epi16(src_high,dst_high);
          diff = _mm_add_epi16(diff,diff); // Because max alpha factor is 2^15
          diff = _mm_mulhi_epi16(diff,factors_high);
          dst_high = _mm_sub_epi16(dst_high,diff); // Subtract because alpha -ve
    
          // Finally, pack `dst_low' and `dst_high' into bytes and save
          *((__m128i *) dp) = _mm_packus_epi16(dst_low,dst_high);
        }
      for (c=right; c > 0; c--, sp++, dp++)
        {
          // Load 1 source pixel and 1 target pixel
          __m128i src_val = _mm_cvtsi32_si128((kdu_int32) *sp);
          __m128i dst_val = _mm_cvtsi32_si128((kdu_int32) *dp);
          src_val = _mm_xor_si128(src_val,xor_mask);
        
          // Find normalized alpha factor in the range 0 to 2^14, inclusive
          __m128i alpha = _mm_srli_epi32(src_val,24); // Get alpha only
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14.

          // Scale and clip the normalized alpha values
          alpha = _mm_madd_epi16(alpha,neg_alpha_scale);
          alpha = _mm_srai_epi32(alpha,6); // Nom. range of alpha -> 0 to -2^15
          alpha = _mm_packs_epi32(alpha,alpha); // Saturates and packs, 2 copies
          
          // Unpack source and target pixel into words
          src_val = _mm_unpacklo_epi8(src_val,zero);
          dst_val = _mm_unpacklo_epi8(dst_val,zero);

          // Copy the alpha factor into all word positions
          __m128i factors = _mm_shufflelo_epi16(alpha,0);

          // Get difference between source and target values, then scale and
          // add this difference back into the target value; the only twist
          // here is for the alpha component, where we take 255 - target.
          src_val = _mm_or_si128(src_val,mask); // Set's source alpha to 255
          __m128i diff = _mm_sub_epi16(src_val,dst_val);
          diff = _mm_add_epi16(diff,diff); // Because max alpha factor is 2^15
          diff = _mm_mulhi_epi16(diff,factors);
          dst_val = _mm_sub_epi16(dst_val,diff); // Subtract because alpha -ve

          // Finally, pack words into bytes and save the pixel
          dst_val = _mm_packus_epi16(dst_val,dst_val);
          *dp = (kdu_uint32) _mm_cvtsi128_si32(dst_val);
        }
    }
  return true;
#endif //!KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE           simd_scaled_blend_region (floating point)                */
/*****************************************************************************/

inline bool
  simd_scaled_blend_region(float *dst, float *src,
                           int height, int width, int dst_row_gap,
                           int src_row_gap, float alpha_factor)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 4) ||
      (_addr_to_kdu_int32(dst) & 15) || (_addr_to_kdu_int32(src) & 15) ||
      (src_row_gap & 3) || (dst_row_gap & 3))
    return false;
  
  __m128 one_val = _mm_set1_ps(1.0F);
  __m128 zero_val = _mm_set1_ps(0.0F);
  __m128 alpha_fact = _mm_set1_ps(alpha_factor);
  
  // Now for the processing loop
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      __m128 *sp = (__m128 *) src;
      __m128 *dp = (__m128 *) dst;
      for (int c=width; c > 0; c--, sp++, dp++)
        {
          __m128 src_val = *sp;
          __m128 dst_val = *dp;
          __m128 alpha = _mm_shuffle_ps(src_val,src_val,0); // Replicates alpha
          alpha = _mm_mul_ps(alpha,alpha_fact);
          src_val = _mm_move_ss(src_val,one_val);
          __m128 diff = _mm_sub_ps(src_val,dst_val);
          diff = _mm_mul_ps(diff,alpha);
          dst_val = _mm_add_ps(dst_val,diff);
          dst_val = _mm_min_ps(dst_val,one_val);
          *dp = _mm_max_ps(dst_val,zero_val);
        }
    }
  return true;
#endif //!KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE         simd_scaled_inv_blend_region (floating point)              */
/*****************************************************************************/

inline bool
  simd_scaled_inv_blend_region(float *dst, float *src,
                               int height, int width, int dst_row_gap,
                               int src_row_gap, float alpha_factor)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 4) ||
      (_addr_to_kdu_int32(dst) & 15) || (_addr_to_kdu_int32(src) & 15) ||
      (src_row_gap & 3) || (dst_row_gap & 3))
    return false;
  
  __m128 one_val = _mm_set1_ps(1.0F);
  __m128 zero_val = _mm_set1_ps(0.0F);
  __m128 alpha_fact = _mm_set1_ps(alpha_factor);
  
  // Now for the processing loop
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      __m128 *sp = (__m128 *) src;
      __m128 *dp = (__m128 *) dst;
      for (int c=width; c > 0; c--, sp++, dp++)
        {
          __m128 src_val = *sp;
          __m128 dst_val = *dp;
          __m128 alpha = _mm_shuffle_ps(src_val,src_val,0); // Replicates alpha
          alpha = _mm_mul_ps(alpha,alpha_fact);
          src_val = _mm_move_ss(src_val,zero_val); // Zero out the alpha value
                        // so that 1-src_val will hold 1 in the alpha channel.
              // Now form diff = (1-src_val) - dst_val = 1 - (src_val+dst_val)
          __m128 minus_diff = _mm_sub_ps(_mm_add_ps(src_val,dst_val),one_val);
          minus_diff = _mm_mul_ps(minus_diff,alpha);
          dst_val = _mm_sub_ps(dst_val,minus_diff);
          dst_val = _mm_min_ps(dst_val,one_val);
          *dp = _mm_max_ps(dst_val,zero_val);
        }
    }
  return true;
#endif //!KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE             simd_blend_premult_region (32-bit)                     */
/*****************************************************************************/

inline bool
  simd_blend_premult_region(kdu_uint32 *dst, kdu_uint32 *src,
                            int height, int width, int dst_row_gap,
                            int src_row_gap)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 4))
    return false;

  // Create all-zero double quad-word
  __m128i zero = _mm_setzero_si128();

  // Now for the processing loop
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      kdu_uint32 *sp = src;
      kdu_uint32 *dp = dst;
      int left = (-(_addr_to_kdu_int32(dp) >> 2)) & 3;
      int c, quads = (width-left)>>2;
      int right = width - left - (quads<<2);
      for (c=left; c > 0; c--, sp++, dp++)
        {
          // Load 1 source pixel and 1 target pixel
          __m128i src_val = _mm_cvtsi32_si128((kdu_int32) *sp);
          __m128i dst_val = _mm_cvtsi32_si128((kdu_int32) *dp);

          // Find normalized alpha factor in the range 0 to 2^14, inclusive
          __m128i alpha = _mm_srli_epi32(src_val,24); // Get alpha only
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14.

          // Unpack source and target pixel into words
          src_val = _mm_unpacklo_epi8(src_val,zero);
          dst_val = _mm_unpacklo_epi8(dst_val,zero);

          // Copy alpha factor into red, green, blue and alpha word positions
          __m128i factors = _mm_shufflelo_epi16(alpha,0);

          // Add source and target pixels and then subtract the alpha-scaled
          // target pixel.
          src_val = _mm_add_epi16(src_val,dst_val);
          dst_val = _mm_slli_epi16(dst_val,2); // Because max factor is 2^14
          dst_val = _mm_mulhi_epi16(dst_val,factors);
          src_val = _mm_sub_epi16(src_val,dst_val);

          // Finally, pack words into bytes and save the pixel
          src_val = _mm_packus_epi16(src_val,src_val);
          *dp = (kdu_uint32) _mm_cvtsi128_si32(src_val);
        }
      for (c=quads; c > 0; c--, sp+=4, dp+=4)
        {
          // Load 4 source pixels and 4 target pixels
          __m128i src_val = _mm_loadu_si128((__m128i *) sp);
          __m128i dst_val = *((__m128i *) dp);

          // Find normalized alpha factors from 4 source pels
          __m128i alpha = _mm_srli_epi32(src_val,24);
                     // Leaves 8-bit alpha only in each pixel's DWORD
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14

          // Unpack source and target pixels into words
          __m128i src_low = _mm_unpacklo_epi8(src_val,zero);
          __m128i src_high = _mm_unpackhi_epi8(src_val,zero);
          __m128i dst_low = _mm_unpacklo_epi8(dst_val,zero);
          __m128i dst_high = _mm_unpackhi_epi8(dst_val,zero);

          // Unpack and copy alpha factors into the red, green, blue and
          // alpha word positions.
          __m128i factors_low = _mm_unpacklo_epi32(alpha,zero);
          __m128i factors_high = _mm_unpackhi_epi32(alpha,zero);
          factors_low = _mm_shufflelo_epi16(factors_low,0);
          factors_low = _mm_shufflehi_epi16(factors_low,0);
          factors_high = _mm_shufflelo_epi16(factors_high,0);
          factors_high = _mm_shufflehi_epi16(factors_high,0);

          // Add source and target pixels and then subtract the alpha-scaled
          // target pixels.
          src_low = _mm_add_epi16(src_low,dst_low);
          dst_low = _mm_slli_epi16(dst_low,2); // Because max factor is 2^14
          dst_low = _mm_mulhi_epi16(dst_low,factors_low);
          src_low = _mm_sub_epi16(src_low,dst_low);
          src_high = _mm_add_epi16(src_high,dst_high);
          dst_high = _mm_slli_epi16(dst_high,2); // Because max factor is 2^14
          dst_high = _mm_mulhi_epi16(dst_high,factors_high);
          src_high = _mm_sub_epi16(src_high,dst_high);
    
          // Finally, pack `dst_low' and `dst_high' into bytes and save
          *((__m128i *) dp) = _mm_packus_epi16(src_low,src_high);
        }
      for (c=right; c > 0; c--, sp++, dp++)
        {
          // Load 1 source pixel and 1 target pixel
          __m128i src_val = _mm_cvtsi32_si128((kdu_int32) *sp);
          __m128i dst_val = _mm_cvtsi32_si128((kdu_int32) *dp);

          // Find normalized alpha factor in the range 0 to 2^14, inclusive
          __m128i alpha = _mm_srli_epi32(src_val,24); // Get alpha only
          __m128i alpha_shift = _mm_slli_epi32(alpha,7);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha_shift = _mm_slli_epi32(alpha_shift,8);
          alpha = _mm_add_epi32(alpha,alpha_shift);
          alpha = _mm_srli_epi32(alpha,9); // Leave max alpha = 2^14.

          // Unpack source and target pixel into words
          src_val = _mm_unpacklo_epi8(src_val,zero);
          dst_val = _mm_unpacklo_epi8(dst_val,zero);

          // Copy alpha factor into red, green, blue and alpha word positions
          __m128i factors = _mm_shufflelo_epi16(alpha,0);

          // Add source and target pixels and then subtract the alpha-scaled
          // target pixel.
          src_val = _mm_add_epi16(src_val,dst_val);
          dst_val = _mm_slli_epi16(dst_val,2); // Because max factor is 2^14
          dst_val = _mm_mulhi_epi16(dst_val,factors);
          src_val = _mm_sub_epi16(src_val,dst_val);

          // Finally, pack words into bytes and save the pixel
          src_val = _mm_packus_epi16(src_val,src_val);
          *dp = (kdu_uint32) _mm_cvtsi128_si32(src_val);
        }
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE           simd_blend_premult_region (floating point)               */
/*****************************************************************************/

inline bool
  simd_blend_premult_region(float *dst, float *src,
                            int height, int width, int dst_row_gap,
                            int src_row_gap)
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (width < 4) ||
      (_addr_to_kdu_int32(dst) & 15) || (_addr_to_kdu_int32(src) & 15) ||
      (src_row_gap & 3) || (dst_row_gap & 3))
    return false;
  
  __m128 zero_val = _mm_set1_ps(0.0F);
  
  // Now for the processing loop
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      __m128 *sp = (__m128 *) src;
      __m128 *dp = (__m128 *) dst;
      for (int c=width; c > 0; c--, sp++, dp++)
        {
          __m128 src_val = *sp;
          __m128 dst_val = *dp;
          __m128 alpha = _mm_shuffle_ps(src_val,src_val,0); // Replicates alpha
          src_val = _mm_add_ps(src_val,dst_val);
          dst_val = _mm_mul_ps(dst_val,alpha);
          src_val = _mm_sub_ps(src_val,dst_val);
          *dp = _mm_max_ps(src_val,zero_val);
        }
    }
  return true;
#endif //!KDU_NO_SSE
}

#endif // X86_REGION_COMPOSITOR_LOCAL_H
