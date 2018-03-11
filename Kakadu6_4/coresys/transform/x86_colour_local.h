/*****************************************************************************/
// File: x86_colour_local.h [scope = CORESYS/TRANSFORMS]
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
   Implements the forward and reverse colour transformations -- both the
reversible (RCT) and the irreversible (ICT = RGB to YCbCr) -- using
MMX/SSE/SSE2 intrinsics.  These can be compiled under GCC or .NET and are
compatible with both 32-bit and 64-bit builds.  Note, however, that the
SSE/SSE2 intrinsics require 16-byte stack alignment, which might not be
guaranteed by GCC builds if the code is called from outside (e.g., from a
Java Virtual Machine, which has poorer stack alignment).  This is an ongoing
problem with GCC, but it does not affect stand-alone applications.  As a
result, you are recommended to use "gcc_colour_mmx_local.h" for both
32-bit and 64-bit GCC builds, since it imposes no stack alignment
requirements.  For MSVC builds you will need the implementation
in this file for 64-bit builds, although both this file and
"msvc_colour_mmx_local.h" will work for 32-bit builds.
******************************************************************************/

#ifndef X86_COLOUR_LOCAL_H
#define X86_COLOUR_LOCAL_H

#include <emmintrin.h>

static bool initialize_vector_constants(); // Forward declaration
#ifdef KDU_NO_MMX64
  __m128i vec128_alphaR;
  __m128i vec128_alphaB;
  __m128i vec128_alphaRplusB;
  __m128i vec128_CBfact;
  __m128i vec128_CRfact;
  __m128i vec128_CRfactR;
  __m128i vec128_CBfactB;
  __m128i vec128_CRfactG;
  __m128i vec128_CBfactG;
#else // MMX and SSE2
  static union {
      __m128i vec128_alphaR;
      __m64 vec64_alphaR;
    };
  static union {
      __m128i vec128_alphaB;
      __m64 vec64_alphaB;
    };
  static union {
      __m128i vec128_alphaRplusB;
      __m64 vec64_alphaRplusB;
    };
  static union {
      __m128i vec128_CBfact;
      __m64 vec64_CBfact;
    };
  static union {
      __m128i vec128_CRfact;
      __m64 vec64_CRfact;
    };
  static union {
      __m128i vec128_CRfactR;
      __m64 vec64_CRfactR;
    };
  static union {
      __m128i vec128_CBfactB;
      __m64 vec64_CBfactB;
    };
  static union {
      __m128i vec128_CRfactG;
      __m64 vec64_CRfactG;
    };
  static union {
      __m128i vec128_CBfactG;
      __m64 vec64_CBfactG;
    };
#endif // MMX and SSE2

__m128 ps128_alphaR;
__m128 ps128_alphaB;
__m128 ps128_alphaG;
__m128 ps128_CBfact;
__m128 ps128_CRfact;
__m128 ps128_CRfactR;
__m128 ps128_CBfactB;
__m128 ps128_neg_CRfactG;
__m128 ps128_neg_CBfactG;

static bool vec_initialized = initialize_vector_constants();

static bool initialize_vector_constants()
{
  kdu_int16 alphaR=(kdu_int16)(0.299 * (1<<16));
  kdu_int16 alphaB=(kdu_int16)(0.114 * (1<<16));
  kdu_int16 CBfact=(kdu_int16)(0.4356659*(1<<16)); // Actual value is 1-0.4356659
  kdu_int16 CRfact=(kdu_int16)(0.2867332*(1<<16)); // Actual value is 1-0.2867332
  
  kdu_int16 CRfactR=(kdu_int16)(0.402*(1<<16)); // Actual factor is 1.402
  kdu_int16 CBfactB=(kdu_int16)(-0.228*(1<<16)); // Actual factor is 1.772
  kdu_int16 CRfactG=(kdu_int16)(0.285864*(1<<16)); // Actual factor is -0.714136
  kdu_int16 CBfactG=(kdu_int16)(-0.344136*(1<<16)); // Actual factor is -0.344136

  vec128_alphaR      = _mm_set1_epi16(alphaR);
  vec128_alphaB      = _mm_set1_epi16(alphaB);
  vec128_alphaRplusB = _mm_set1_epi16(alphaR+alphaB);
  vec128_CBfact      = _mm_set1_epi16(CBfact);
  vec128_CRfact      = _mm_set1_epi16(CRfact);
  vec128_CRfactR     = _mm_set1_epi16(CRfactR);
  vec128_CBfactB     = _mm_set1_epi16(CBfactB);
  vec128_CRfactG     = _mm_set1_epi16(CRfactG);
  vec128_CBfactG     = _mm_set1_epi16(CBfactG);

  ps128_alphaR      = _mm_set1_ps((float) ALPHA_R);
  ps128_alphaB      = _mm_set1_ps((float) ALPHA_B);
  ps128_alphaG      = _mm_set1_ps((float) ALPHA_G);
  ps128_CBfact      = _mm_set1_ps((float) CB_FACT);
  ps128_CRfact      = _mm_set1_ps((float) CR_FACT);
  ps128_CBfactB     = _mm_set1_ps((float) CB_FACT_B);
  ps128_CRfactR     = _mm_set1_ps((float) CR_FACT_R);
  ps128_neg_CBfactG = _mm_set1_ps((float) -CB_FACT_G);
  ps128_neg_CRfactG = _mm_set1_ps((float) -CR_FACT_G);

  return true;
}


/* ========================================================================= */
/*                        Now for the MMX functions                          */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                          simd_ict                                  */
/*****************************************************************************/

inline bool
  simd_ict(register kdu_int16 *src1,
           register kdu_int16 *src2,
           register kdu_int16 *src3, register int samples)
{
#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // 128-bit implementation, using SSE/SSE2 instructions
      __m128i *sp1 = (__m128i *) src1;
      __m128i *sp2 = (__m128i *) src2;
      __m128i *sp3 = (__m128i *) src3;
      __m128i ones = _mm_set1_epi16(1);
      __m128i alpha_r = vec128_alphaR;
      __m128i alpha_b = vec128_alphaB;
      __m128i alpha_r_plus_b = vec128_alphaRplusB;
      __m128i cb_fact = vec128_CBfact;
      __m128i cr_fact = vec128_CRfact;
      samples = (samples+7)>>3;
      for (register int c=0; c < samples; c++)
        {
          __m128i red = sp1[c];
          __m128i blue = sp3[c];
          __m128i tmp = _mm_adds_epi16(ones,ones); // Make a pre-offset of 2
          __m128i y = _mm_adds_epi16(red,tmp);  // Form pre-offset red channel
          y = _mm_mulhi_epi16(y,alpha_r); // Red contribution to Y
          tmp = _mm_add_epi16(tmp,tmp); // Form pre-offset of 4 in `tmp'
          tmp = _mm_adds_epi16(tmp,blue);
          tmp = _mm_mulhi_epi16(tmp,alpha_b); // Blue contribution to Y
          y = _mm_adds_epi16(y,tmp); // Add red and blue contributions to Y.
          __m128i green = sp2[c];
          tmp = _mm_adds_epi16(green,ones); // Add pre-offset of 1 to the green channel
          tmp = _mm_mulhi_epi16(tmp,alpha_r_plus_b); // Green * (alphaR+alphaB)
          green = _mm_subs_epi16(green,tmp); // Forms green * (1-alphaR-alphaB)
          y = _mm_adds_epi16(y,green); // Forms final luminance channel
          sp1[c] = y;
          blue = _mm_subs_epi16(blue,y); // Forms Blue-Y in `blue'
          tmp = _mm_adds_epi16(blue,ones); // Add a pre-offset of 1 to Blue-Y
          tmp = _mm_mulhi_epi16(tmp,cb_fact);
          sp2[c] = _mm_subs_epi16(blue,tmp); // Forms CB = (blue-Y)*(1-CBfact)
          red = _mm_subs_epi16(red,y); // Forms Red-Y in `red'
          tmp = _mm_adds_epi16(red,ones); // Add a pre-offset of 1 to Red-Y
          tmp = _mm_adds_epi16(tmp,ones); // Make the pre-offset equal to 2
          tmp = _mm_mulhi_epi16(tmp,cr_fact); // Multiply (Red-Y) * CRfact
          sp3[c] = _mm_subs_epi16(red,tmp); // Forms CR = (red-Y)*(1-CRfact)
        }
      return true;
    }
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
  if (kdu_mmx_level >= 1)
    { // 64-bit implementation, using MMX instructions only
      __m64 *sp1 = (__m64 *) src1;
      __m64 *sp2 = (__m64 *) src2;
      __m64 *sp3 = (__m64 *) src3;
      __m64 ones = _mm_set1_pi16(1);
      samples = (samples+3)>>2;
      for (register int c=0; c < samples; c++)
        {
          __m64 red = sp1[c];
          __m64 blue = sp3[c];
          __m64 tmp = _mm_adds_pi16(ones,ones); // Make a pre-offset of 2
          __m64 y = _mm_adds_pi16(red,tmp);  // Form pre-offset red channel
          y = _mm_mulhi_pi16(y,vec64_alphaR); // Red contribution to Y
          tmp = _mm_add_pi16(tmp,tmp); // Form pre-offset of 4 in `tmp'
          tmp = _mm_adds_pi16(tmp,blue);
          tmp = _mm_mulhi_pi16(tmp,vec64_alphaB); // Blue contribution to Y
          y = _mm_adds_pi16(y,tmp); // Add red and blue contributions to Y.
          __m64 green = sp2[c];
          tmp = _mm_adds_pi16(green,ones); // Add pre-offset of 1 to the green channel
          tmp = _mm_mulhi_pi16(tmp,vec64_alphaRplusB); // Green * (alphaR+alphaB)
          green = _mm_subs_pi16(green,tmp); // Forms green * (1-alphaR-alphaB)
          y = _mm_adds_pi16(y,green); // Forms final luminance channel
          sp1[c] = y;
          blue = _mm_subs_pi16(blue,y); // Forms Blue-Y in `blue'
          tmp = _mm_adds_pi16(blue,ones); // Add a pre-offset of 1 to Blue-Y
          tmp = _mm_mulhi_pi16(tmp,vec64_CBfact);
          sp2[c] = _mm_subs_pi16(blue,tmp); // Forms CB = (blue-Y)*(1-CBfact)
          red = _mm_subs_pi16(red,y); // Forms Red-Y in `red'
          tmp = _mm_adds_pi16(red,ones); // Add a pre-offset of 1 to Red-Y
          tmp = _mm_adds_pi16(tmp,ones); // Make the pre-offset equal to 2
          tmp = _mm_mulhi_pi16(tmp,vec64_CRfact); // Multiply (Red-Y) * CRfact
          sp3[c] = _mm_subs_pi16(red,tmp); // Forms CR = (red-Y)*(1-CRfact)
        }
      _mm_empty();
      return true;
    }
#endif // !KDU_NO_MMX64
  return false;
}

/*****************************************************************************/
/* INLINE                          simd_ict                                  */
/*****************************************************************************/

inline bool
  simd_ict(float *src1, float *src2, float *src3, int samples)
  /* As above, but for floating point sample values. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  if (kdu_mmx_level < 2)
    return false;
  __m128 *sp1 = (__m128 *) src1;
  __m128 *sp2 = (__m128 *) src2;
  __m128 *sp3 = (__m128 *) src3;
  __m128 alpha_r = ps128_alphaR;
  __m128 alpha_b = ps128_alphaB;
  __m128 alpha_g = ps128_alphaG;
  __m128 cb_fact = ps128_CBfact;
  __m128 cr_fact = ps128_CRfact;
  samples = (samples+3)>>2;
  for (int c=0; c < samples; c++)
    {
      __m128 green = sp2[c];
      __m128 y = _mm_mul_ps(green,alpha_g);
      __m128 red = sp1[c];
      __m128 blue = sp3[c];
      y = _mm_add_ps(y,_mm_mul_ps(red,alpha_r));
      y = _mm_add_ps(y,_mm_mul_ps(blue,alpha_b));
      sp1[c] = y;
      blue = _mm_sub_ps(blue,y);
      sp2[c] = _mm_mul_ps(blue,cb_fact);
      red = _mm_sub_ps(red,y);
      sp3[c] = _mm_mul_ps(red,cr_fact);
    }
  return true;
#endif // !KDU_NO_SSE
  return false;
}

/*****************************************************************************/
/* INLINE                      simd_inverse_ict                              */
/*****************************************************************************/

inline bool
  simd_inverse_ict(register kdu_int16 *src1,
                   register kdu_int16 *src2,
                   register kdu_int16 *src3, register int samples)
{
#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // 128-bit implementation, using SSE/SSE2 instructions
      __m128i ones = _mm_set1_epi16(1); // Set 1's in each 16-bit word
      __m128i twos = _mm_add_epi16(ones,ones); // Set 2's in each 16-bit word
      __m128i *sp1 = (__m128i *) src1;
      __m128i *sp2 = (__m128i *) src2;
      __m128i *sp3 = (__m128i *) src3;
      __m128i cr_fact_r = vec128_CRfactR;
      __m128i cr_fact_g = vec128_CRfactG;
      __m128i cb_fact_b = vec128_CBfactB;
      __m128i cb_fact_g = vec128_CBfactG;
      samples = (samples+7)>>3;
      for (register int c=0; c < samples; c++)
        {
          __m128i y = sp1[c];
          __m128i cr = sp3[c];
          __m128i red = _mm_adds_epi16(cr,ones); // Add pre-offset to CR
          red = _mm_mulhi_epi16(red,cr_fact_r); // Multiply by 0.402*2^16 (CRfactR) & divide by 2^16
          red = _mm_adds_epi16(red,cr); // Add CR again to make factor equivalent to 1.402
          sp1[c] = _mm_adds_epi16(red,y); // Add in luminance to get red & save
          __m128i green = _mm_adds_epi16(cr,twos); // Pre-offset of 2
          green = _mm_mulhi_epi16(green,cr_fact_g); // Multiply by 0.285864*2^16 (CRfactG) & divide by 2^16
          green = _mm_subs_epi16(green,cr); // Leaves the correct multiple of CR
          green = _mm_adds_epi16(green,y); // Y + scaled CR forms most of green
          __m128i cb = sp2[c];
          __m128i blue = _mm_subs_epi16(cb,twos); // Pre-offset of -2
          blue = _mm_mulhi_epi16(blue,cb_fact_b); // Multiply by -0.228*2^16 (CBfactB) & divide by 2^16
          blue = _mm_adds_epi16(blue,cb); // Gets 0.772*Cb in blue
          blue = _mm_adds_epi16(blue,cb); // Gets 1.772*Cb in blue
          sp3[c] = _mm_adds_epi16(blue,y); // Add in luminance to get blue & save
          cb = _mm_subs_epi16(cb,twos); // Pre-offset of -2
          cb = _mm_mulhi_epi16(cb,cb_fact_g); // Multiply by -0.344136*2^16 (CBfactG) and divide by 2^16
          sp2[c] = _mm_adds_epi16(green,cb); // Complete and save the green channel
        }
      return true;
    }
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
  if (kdu_mmx_level >= 1)
    { // 64-bit implementation, using MMX instructions only
      __m64 ones = _mm_set1_pi16(1); // Set 1's in each 16-bit word
      __m64 twos = _mm_add_pi16(ones,ones); // Set 2's in each 16-bit word
      __m64 *sp1 = (__m64 *) src1;
      __m64 *sp2 = (__m64 *) src2;
      __m64 *sp3 = (__m64 *) src3;
      samples = (samples+3)>>2;
      for (register int c=0; c < samples; c++)
        {
          __m64 y = sp1[c];
          __m64 cr = sp3[c];
          __m64 red = _mm_adds_pi16(cr,ones); // Add pre-offset to CR
          red = _mm_mulhi_pi16(red,vec64_CRfactR); // Multiply by 0.402*2^16 (CRfactR) & divide by 2^16
          red = _mm_adds_pi16(red,cr); // Add CR again to make factor equivalent to 1.402
          sp1[c] = _mm_adds_pi16(red,y); // Add in luminance to get red & save
          __m64 green = _mm_adds_pi16(cr,twos); // Pre-offset of 2
          green = _mm_mulhi_pi16(green,vec64_CRfactG); // Multiply by 0.285864*2^16 (CRfactG) & divide by 2^16
          green = _mm_subs_pi16(green,cr); // Leaves the correct multiple of CR
          green = _mm_adds_pi16(green,y); // Y + scaled CR forms most of green
          __m64 cb = sp2[c];
          __m64 blue = _mm_subs_pi16(cb,twos); // Pre-offset of -2
          blue = _mm_mulhi_pi16(blue,vec64_CBfactB); // Multiply by -0.228*2^16 (CBfactB) & divide by 2^16
          blue = _mm_adds_pi16(blue,cb); // Gets 0.772*Cb in blue
          blue = _mm_adds_pi16(blue,cb); // Gets 1.772*Cb in blue
          sp3[c] = _mm_adds_pi16(blue,y); // Add in luminance to get blue & save
          cb = _mm_subs_pi16(cb,twos); // Pre-offset of -2
          cb = _mm_mulhi_pi16(cb,vec64_CBfactG); // Multiply by -0.344136*2^16 (CBfactG) and divide by 2^16
          sp2[c] = _mm_adds_pi16(green,cb); // Complete and save the green channel
        }
      _mm_empty(); // Clear MMX registers for use by FPU
      return true;
    }
#endif // !KDU_NO_MMX64
  return false;
}

/*****************************************************************************/
/* INLINE                      simd_inverse_ict                              */
/*****************************************************************************/

inline bool
  simd_inverse_ict(float *src1, float *src2, float *src3, int samples)
  /* As above, but for floating point sample values. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  if (kdu_mmx_level < 2)
    return false;
  __m128 *sp1 = (__m128 *) src1;
  __m128 *sp2 = (__m128 *) src2;
  __m128 *sp3 = (__m128 *) src3;
  __m128 cr_fact_r = ps128_CRfactR;
  __m128 neg_cr_fact_g = ps128_neg_CRfactG;
  __m128 cb_fact_b = ps128_CBfactB;
  __m128 neg_cb_fact_g = ps128_neg_CBfactG;
  samples = (samples+3)>>2;
  for (int c=0; c < samples; c++)
    {
      __m128 y = sp1[c];
      __m128 cr = sp3[c];
      __m128 red = _mm_mul_ps(cr,cr_fact_r);
      sp1[c] = _mm_add_ps(red,y); // Add in luminance to get red & save
      __m128 green = _mm_mul_ps(cr,neg_cr_fact_g);
      green = _mm_add_ps(green,y); // Y + scaled CR forms most of green
      __m128 cb = sp2[c];
      __m128 blue = _mm_mul_ps(cb,cb_fact_b);
      sp3[c] = _mm_add_ps(blue,y); // Add in luminance to get blue & save
      cb = _mm_mul_ps(cb,neg_cb_fact_g);
      sp2[c] = _mm_add_ps(green,cb); // Complete and save the green channel
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                           simd_rct                                 */
/*****************************************************************************/

inline bool
  simd_rct(register kdu_int16 *src1,
           register kdu_int16 *src2,
           register kdu_int16 *src3, register int samples)
{
#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // 128-bit implementation, using SSE/SSE2 instructions
      samples = (samples+7)>>3;
      __m128i *sp1 = (__m128i *) src1;
      __m128i *sp2 = (__m128i *) src2;
      __m128i *sp3 = (__m128i *) src3;
      for (register int c=0; c < samples; c++)
        {
          __m128i red = sp1[c];
          __m128i green = sp2[c];
          __m128i blue = sp3[c];
          __m128i y = _mm_adds_epi16(red,blue);
          y = _mm_adds_epi16(y,green);
          y = _mm_adds_epi16(y,green); // Now have 2*G + R + B
          sp1[c] = _mm_srai_epi16(y,2); // Save Y = (2*G + R + B)>>2
          sp2[c] = _mm_subs_epi16(blue,green); // Save Db = B-G
          sp3[c] = _mm_subs_epi16(red,green); // Save Dr = R-G
        }
      return true;
    }
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
  if (kdu_mmx_level >= 1)
    { // 64-bit implementation, using MMX instructions only
      samples = (samples+3)>>2;
      __m64 *sp1 = (__m64 *) src1;
      __m64 *sp2 = (__m64 *) src2;
      __m64 *sp3 = (__m64 *) src3;
      for (register int c=0; c < samples; c++)
        {
          __m64 red = sp1[c];
          __m64 green = sp2[c];
          __m64 blue = sp3[c];
          __m64 y = _mm_adds_pi16(red,blue);
          y = _mm_adds_pi16(y,green);
          y = _mm_adds_pi16(y,green); // Now have 2*G + R + B
          sp1[c] = _mm_srai_pi16(y,2); // Save Y = (2*G + R + B)>>2
          sp2[c] = _mm_subs_pi16(blue,green); // Save Db = B-G
          sp3[c] = _mm_subs_pi16(red,green); // Save Dr = R-G
        }
      _mm_empty(); // Clear MMX registers for use by FPU
      return true;
    }
#endif // !KDU_NO_MMX64
  return false;
}

/*****************************************************************************/
/* INLINE                           simd_rct                                 */
/*****************************************************************************/

inline bool
  simd_rct(register kdu_int32 *src1,
           register kdu_int32 *src2,
           register kdu_int32 *src3, register int samples)
  /* Same as above function, except that it operates on 32-bit integers. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  if (kdu_mmx_level < 2)
    return false;
  samples = (samples+3)>>2;
  __m128i *sp1 = (__m128i *) src1;
  __m128i *sp2 = (__m128i *) src2;
  __m128i *sp3 = (__m128i *) src3;
  for (register int c=0; c < samples; c++)
    {
      __m128i red = sp1[c];
      __m128i green = sp2[c];
      __m128i blue = sp3[c];
      __m128i y = _mm_add_epi32(red,blue);
      y = _mm_add_epi32(y,green);
      y = _mm_add_epi32(y,green); // Now have 2*G + R + B
      sp1[c] = _mm_srai_epi32(y,2); // Save Y = (2*G + R + B)>>2
      sp2[c] = _mm_sub_epi32(blue,green); // Save Db = B-G
      sp3[c] = _mm_sub_epi32(red,green); // Save Dr = R-G
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                       simd_inverse_rct                             */
/*****************************************************************************/

inline bool
  simd_inverse_rct(register kdu_int16 *src1,
                   register kdu_int16 *src2,
                   register kdu_int16 *src3, register int samples)
{
#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // 128-bit implementation, using SSE/SSE2 instructions
      __m128i *sp1 = (__m128i *) src1;
      __m128i *sp2 = (__m128i *) src2;
      __m128i *sp3 = (__m128i *) src3;
      samples = (samples+7)>>3;
      for (register int c=0; c < samples; c++)
        {
          __m128i db = sp2[c];
          __m128i dr = sp3[c];
          __m128i y = sp1[c];
          __m128i tmp = _mm_adds_epi16(db,dr);
          tmp = _mm_srai_epi16(tmp,2); // Forms (Db+Dr)>>2
          __m128i green = _mm_subs_epi16(y,tmp);
          sp2[c] = green;
          sp1[c] = _mm_adds_epi16(dr,green);
          sp3[c] = _mm_adds_epi16(db,green);
        }
      return true;
    }
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
  if (kdu_mmx_level >= 1)
    { // 64-bit implementation, using MMX instructions only
      __m64 *sp1 = (__m64 *) src1;
      __m64 *sp2 = (__m64 *) src2;
      __m64 *sp3 = (__m64 *) src3;
      samples = (samples+3)>>2;
      for (register int c=0; c < samples; c++)
        {
          __m64 db = sp2[c];
          __m64 dr = sp3[c];
          __m64 y = sp1[c];
          __m64 tmp = _mm_adds_pi16(db,dr);
          tmp = _mm_srai_pi16(tmp,2); // Forms (Db+Dr)>>2
          __m64 green = _mm_subs_pi16(y,tmp);
          sp2[c] = green;
          sp1[c] = _mm_adds_pi16(dr,green);
          sp3[c] = _mm_adds_pi16(db,green);
        }
      _mm_empty(); // Clear MMX registers for use by FPU
      return true;
    }
#endif // !KDU_NO_MMX64
  return false;
}

/*****************************************************************************/
/* INLINE                       simd_inverse_rct                             */
/*****************************************************************************/

inline bool
  simd_inverse_rct(register kdu_int32 *src1,
                   register kdu_int32 *src2,
                   register kdu_int32 *src3, register int samples)
  /* Same as above function, except that it operates on 32-bit integers. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  if (kdu_mmx_level < 2)
    return false;
  __m128i *sp1 = (__m128i *) src1;
  __m128i *sp2 = (__m128i *) src2;
  __m128i *sp3 = (__m128i *) src3;
  samples = (samples+3)>>2;
  for (register int c=0; c < samples; c++)
    {
      __m128i db = sp2[c];
      __m128i dr = sp3[c];
      __m128i y = sp1[c];
      __m128i tmp = _mm_add_epi32(db,dr);
      tmp = _mm_srai_epi32(tmp,2); // Forms (Db+Dr)>>2
      __m128i green = _mm_sub_epi32(y,tmp);
      sp2[c] = green;
      sp1[c] = _mm_add_epi32(dr,green);
      sp3[c] = _mm_add_epi32(db,green);
    }
  return true;
#endif // !KDU_NO_SSE
}

#endif // X86_COLOUR_LOCAL_H
