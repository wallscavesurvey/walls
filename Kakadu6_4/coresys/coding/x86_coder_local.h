/*****************************************************************************/
// File: x86_coder_local.h [scope = CORESYS/CODING]
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
   Provides SSE2 implementations to accelerate the conversion and transfer of
data between the block coder and DWT line-based processing engine.  The
implementation in this file is based on MMX/SSE/SSE2 intrinsics.  These
can be compiled under GCC or .NET and are compatible with both 32-bit and
64-bit builds.  Note, however, that the SSE/SSE2 intrinsics require 16-byte
stack alignment, which might not be guaranteed by GCC builds if the code is
called from outside (e.g., from a Java Virtual Machine, which has poorer
stack alignment).  This is an ongoing problem with GCC, although it does not
affect stand-alone applications.  As a result, you are recommended to use
"gcc_coder_mmx_local.h" for both 32-bit and 64-bit GCC builds, since it
imposes no stack alignment requirements.  For MSVC builds you can use
"msvc_coder_mmx_local.h".  For .NET builds, you will need the implementation
in this file for 64-bit builds, although both this file and
"msvc_coder_mmx_local.h" will work for 32-bit builds.
******************************************************************************/
#ifndef X86_CODER_LOCAL_H
#define X86_CODER_LOCAL_H

#include <emmintrin.h>

/* ========================================================================= */
/*                       Now for the MMX/SSE functions                       */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                   simd_xfer_decoded_block                          */
/*****************************************************************************/

bool
  simd_xfer_decoded_block(register kdu_int32 *src,
                          register kdu_sample16 **dst_refs,
                          register int dst_offset,
                          int width,
                          register int height,
                          bool reversible, int K_max, float delta)
{
  if (kdu_mmx_level < 2)
    return false;
  assert((width == 32) || (width == 64));
  if (reversible)
    {
      register kdu_sample16 *dst;

      __m128i downshift = _mm_cvtsi32_si128(31-K_max);
      __m128i comp = _mm_setzero_si128(); // Avoid compiler warnings
      comp = _mm_cmpeq_epi32(comp,comp); // Fill with FF's
      __m128i ones = _mm_srli_epi32(comp,31); // Set each DWORD equal to 1
      __m128i kmax = _mm_cvtsi32_si128(K_max);
      comp = _mm_sll_epi32(comp,kmax); // Leaves 1+downshift 1's in MSB's
      comp = _mm_or_si128(comp,ones); // `comp' now holds the amount we have to
             // add after inverting the bits of a downshifted sign-mag quantity
             // which was negative, to restore the correct 2's complement value.
      for (; height > 0; height--, dst_refs++)
        {
          dst = *dst_refs + dst_offset;
          for (register int c=width; c > 0; c-=8, src+=8, dst+=8)
            {
              __m128i val1 = *((__m128i *) src);
              __m128i ref = _mm_setzero_si128();
              ref = _mm_cmpgt_epi32(ref,val1); // Fills DWORDS with 1's if val<0
              val1 = _mm_xor_si128(val1,ref);
              val1 = _mm_sra_epi32(val1,downshift);
              ref = _mm_and_si128(ref,comp); // Leave the bits we need to add
              val1 = _mm_add_epi32(val1,ref); // Finish conversion to 2's comp

              __m128i val2 = *((__m128i *)(src+4));
              ref = _mm_setzero_si128();
              ref = _mm_cmpgt_epi32(ref,val2); // Fills DWORDS with 1's if val<0
              val2 = _mm_xor_si128(val2,ref);
              val2 = _mm_sra_epi32(val2,downshift);
              ref = _mm_and_si128(ref,comp); // Leave the bits we need to add
              val2 = _mm_add_epi32(val2,ref); // Finish conversion to 2's comp

              *((__m128i *) dst) = _mm_packs_epi32(val1,val2);
            }
        }
    }
  else
    {
      float fscale = delta * (float)(1<<KDU_FIX_POINT);
      if (K_max <= 31)
        fscale /= (float)(1<<(31-K_max));
      else
        fscale *= (float)(1<<(K_max-31));

      int mxcsr_orig = _mm_getcsr();
      int mxcsr_cur = mxcsr_orig & ~(3<<13); // Reset rounding control bits
      _mm_setcsr(mxcsr_cur);

      register kdu_sample16 *dst;
      __m128 vec_scale = _mm_load1_ps(&fscale);
      __m128i comp = _mm_setzero_si128(); // Avoid compiler warnings
      __m128i ones = _mm_cmpeq_epi32(comp,comp); // Fill with FF's
      comp = _mm_slli_epi32(ones,31); // Each DWORD  now holds 0x80000000
      ones = _mm_srli_epi32(ones,31); // Each DWORD now holds 1
      comp = _mm_or_si128(comp,ones); // Each DWORD now holds 0x800000001
      for (; height > 0; height--, dst_refs++)
        {
          dst = *dst_refs + dst_offset;
          for (register int c=width; c > 0; c-=8, src+=8, dst+=8)
            {
              __m128i val1 = *((__m128i *) src);
              __m128i ref = _mm_setzero_si128();
              ref = _mm_cmpgt_epi32(ref,val1); // Fills DWORDS with 1's if val<0
              val1 = _mm_xor_si128(val1,ref);
              ref = _mm_and_si128(ref,comp); // Leave the bits we need to add
              val1 = _mm_add_epi32(val1,ref); // Finish conversion to 2's comp
              __m128 fval1 = _mm_cvtepi32_ps(val1);
              fval1 = _mm_mul_ps(fval1,vec_scale);
              val1 = _mm_cvtps_epi32(fval1);

              __m128i val2 = *((__m128i *)(src+4));
              ref = _mm_setzero_si128();
              ref = _mm_cmpgt_epi32(ref,val2); // Fills DWORDS with 1's if val<0
              val2 = _mm_xor_si128(val2,ref);
              ref = _mm_and_si128(ref,comp); // Leave the bits we need to add
              val2 = _mm_add_epi32(val2,ref); // Finish conversion to 2's comp
              __m128 fval2 = _mm_cvtepi32_ps(val2);
              fval2 = _mm_mul_ps(fval2,vec_scale);
              val2 = _mm_cvtps_epi32(fval2);

              *((__m128i *) dst) = _mm_packs_epi32(val1,val2);
            }
        }


      _mm_setcsr(mxcsr_orig); // Restore rounding control bits
    }
  return true;
}

#endif // X86_CODER_LOCAL_H
