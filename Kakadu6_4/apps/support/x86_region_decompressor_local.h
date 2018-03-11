/*****************************************************************************/
// File: msvc_region_decompressor_local.h [scope = CORESYS/TRANSFORMS]
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
   Implements critical sample precision conversion and mapping functions
from `kdu_region_decompressor', using MMX/SSE/SSE2 intrinsics.  These can
be compiled under GCC or .NET and are compatible with both 32-bit and 64-bit
builds.  Note, however, that the SSE/SSE2 intrinsics require 16-byte
stack alignment, which might not be guaranteed by GCC builds if the
code is called from outside (e.g., from a Java Virtual Machine, which
has poorer stack alignment).  This is an ongoing problem with GCC, but
it does not affect stand-alone applications.
******************************************************************************/

#ifndef X86_REGION_DECOMPRESSOR_LOCAL_H
#define X86_REGION_DECOMPRESSOR_LOCAL_H

#include <emmintrin.h>
#ifndef KDU_NO_SSSE3
#  include <tmmintrin.h>
#  define kdrd_alignr_ps(_a,_b,_sh) \
     _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(_a), \
                                      _mm_castps_si128(_b),_sh))
#endif // KDU_NO_SSSE3

/*****************************************************************************/
/* INLINE                     simd_convert_fix16                             */
/*****************************************************************************/

inline bool
  simd_convert_fix16(kdu_sample16 *src, kdu_int16 *dst, int num_samples,
                     int upshift, int offset)
  /* Converts each `src' sample to `dst' sample by upshifting by `upshift'
     or downshifting by -`upshift' after adding `offset', whichever is most
     appropriate.  Does not assume that `src' or `dst' are 16-byte aligned
     on entry, but does assume that `dst' is large enough to accommodate
     writing aligned 128-bit words. */
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (num_samples < 32))
    return false; // Not worth special processing
  int lead=(-((_addr_to_kdu_int32(dst))>>1))&7; // Initial non-aligned samples
  num_samples -= lead;
  if (upshift < 0)
    {
      for (; lead > 0; lead--, src++, dst++)
        *dst = (src->ival+offset) >> (-upshift);
      __m128i vec_offset = _mm_set1_epi16((kdu_int16) offset);
      __m128i shift = _mm_cvtsi32_si128(-upshift);
      for (int c=0; c < num_samples; c+=8)
        {
          __m128i val = _mm_loadu_si128((__m128i *)(src+c));
          val = _mm_add_epi16(val,vec_offset);
          val = _mm_sra_epi16(val,shift);
          *((__m128i *)(dst+c)) = val;
        }
    }
  else
    {
      for (; lead > 0; lead--, src++, dst++)
        *dst = src->ival << upshift;      
      __m128i shift = _mm_cvtsi32_si128(upshift);
      for (int c=0; c < num_samples; c+=8)
        {
          __m128i val = _mm_loadu_si128((__m128i *)(src+c));
          val = _mm_sll_epi16(val,shift);
          *((__m128i *)(dst+c)) = val;
        }
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                  simd_vert_resampling_float                        */
/*****************************************************************************/

inline bool
  simd_vert_resampling_float(int length, kdu_line_buf *src[],
                             kdu_line_buf *dst, int kernel_length,
                             float *kernel)
  /* This function implements disciplined vertical resampling on packed
     16-byte vectors of 4 floating point values at a time.  The function
     generates `length' outputs, writing them into the `dst' line.  Each
     output is formed by taking the inner product between a 6-sample `kernel'
     and corresponding entries from each of the 6 lines referenced from the
     `src' array.  The `kernel' actually consists of 6 quad-floats, where
     all floats in each quad are identical. */
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if (kdu_mmx_level < 2)
    return false; // No support for SSE2
  
  if (kernel_length == 2)
    {
      float *sp0=(float *)(src[2]->get_buf32());
      float *sp1=(float *)(src[3]->get_buf32());
      float *dp =(float *)(dst->get_buf32());
      __m128 *kern = (__m128 *) kernel;
      __m128 k0=kern[0], k1=kern[1];
      for (int n=0; n < length; n+=4)
        {
          __m128 val = _mm_mul_ps(*((__m128 *)(sp0+n)),k0);
          val = _mm_add_ps(val,_mm_mul_ps(*((__m128 *)(sp1+n)),k1));
          *((__m128 *)(dp+n)) = val;
        }      
    }
  else
    {
      assert(kernel_length == 6);
      float *sp0=(float *)(src[0]->get_buf32());
      float *sp1=(float *)(src[1]->get_buf32());
      float *sp2=(float *)(src[2]->get_buf32());
      float *sp3=(float *)(src[3]->get_buf32());
      float *sp4=(float *)(src[4]->get_buf32());
      float *sp5=(float *)(src[5]->get_buf32());
      float *dp =(float *)(dst->get_buf32());
      __m128 *kern = (__m128 *) kernel;
      __m128 k0=kern[0], k1=kern[1], k2=kern[2],
             k3=kern[3], k4=kern[4], k5=kern[5];
      for (int n=0; n < length; n+=4)
        {
          __m128 val = _mm_mul_ps(*((__m128 *)(sp0+n)),k0);
          val = _mm_add_ps(val,_mm_mul_ps(*((__m128 *)(sp1+n)),k1));
          val = _mm_add_ps(val,_mm_mul_ps(*((__m128 *)(sp2+n)),k2));
          val = _mm_add_ps(val,_mm_mul_ps(*((__m128 *)(sp3+n)),k3));
          val = _mm_add_ps(val,_mm_mul_ps(*((__m128 *)(sp4+n)),k4));
          val = _mm_add_ps(val,_mm_mul_ps(*((__m128 *)(sp5+n)),k5));
          *((__m128 *)(dp+n)) = val;
        }
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                  simd_vert_resampling_fix16                        */
/*****************************************************************************/

inline bool
  simd_vert_resampling_fix16(int length, kdu_line_buf *src[],
                             kdu_line_buf *dst, int kernel_length,
                             kdu_int16 *kernel)
  /* Same as `simd_vert_resampling_float', except that it processes
     packed 16-bit integers (8 at a time), with a nominal precision of
     KDU_FIX_POINT.  The interpolation `kernel' in this case consists of
     6 vectors of 8 identical 16-bit integers each, corresponding to the
     true interpolation kernel taps scaled by -(2^15). */
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if (kdu_mmx_level < 2)
    return false; // No support for SSE2
  
  if (kernel_length == 2)
    {
      kdu_int16 *sp0=(kdu_int16 *)(src[2]->get_buf16());
      kdu_int16 *sp1=(kdu_int16 *)(src[3]->get_buf16());
      kdu_int16 *dp = (kdu_int16 *)(dst->get_buf16());
      if (kernel[8] == 0)
        { // Can just copy from sp0 to dp
          for (int n=0; n < length; n++)
            dp[n] = sp0[n];
        }
      else
        { 
          __m128i *kern = (__m128i *) kernel;
          __m128i k0=kern[0], k1=kern[1];
          for (int n=0; n < length; n+=8)
            { 
              __m128i val, sum=_mm_setzero_si128();
              val = *((__m128i *)(sp0+n)); val = _mm_adds_epi16(val,val);
              val = _mm_mulhi_epi16(val,k0); sum = _mm_sub_epi16(sum,val);
              val = *((__m128i *)(sp1+n)); val = _mm_adds_epi16(val,val);
              val = _mm_mulhi_epi16(val,k1); sum = _mm_sub_epi16(sum,val);
              *((__m128i *)(dp+n)) = sum;
            }
        }
    }
  else
    {
      assert(kernel_length == 6);
      kdu_int16 *sp0=(kdu_int16 *)(src[0]->get_buf16());
      kdu_int16 *sp1=(kdu_int16 *)(src[1]->get_buf16());
      kdu_int16 *sp2=(kdu_int16 *)(src[2]->get_buf16());
      kdu_int16 *sp3=(kdu_int16 *)(src[3]->get_buf16());
      kdu_int16 *sp4=(kdu_int16 *)(src[4]->get_buf16());
      kdu_int16 *sp5=(kdu_int16 *)(src[5]->get_buf16());
      kdu_int16 *dp = (kdu_int16 *)(dst->get_buf16());
      __m128i *kern = (__m128i *) kernel;
      __m128i k0=kern[0], k1=kern[1], k2=kern[2],
              k3=kern[3], k4=kern[4], k5=kern[5];
      for (int n=0; n < length; n+=8)
        {
          __m128i val, sum=_mm_setzero_si128();
          val = *((__m128i *)(sp0+n)); val = _mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,k0); sum = _mm_sub_epi16(sum,val);
          val = *((__m128i *)(sp1+n)); val = _mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,k1); sum = _mm_sub_epi16(sum,val);
          val = *((__m128i *)(sp2+n)); val = _mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,k2); sum = _mm_sub_epi16(sum,val);
          val = *((__m128i *)(sp3+n)); val = _mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,k3); sum = _mm_sub_epi16(sum,val);
          val = *((__m128i *)(sp4+n)); val = _mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,k4); sum = _mm_sub_epi16(sum,val);
          val = *((__m128i *)(sp5+n)); val = _mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,k5); sum = _mm_sub_epi16(sum,val);
          *((__m128i *)(dp+n)) = sum;
        }
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                  simd_horz_resampling_float                        */
/*****************************************************************************/

inline bool
  simd_horz_resampling_float(int length, kdu_line_buf *src,
                             kdu_line_buf *dst, kdu_int64 phase,
                             kdu_int64 num, int den, int pshift,
                             float **kernels, int kernel_length, int leadin)
  /* This function implements disciplined horizontal resampling for floating
     point sample data.  The function generates `length' outputs, writing them
     into the `dst' line.  The function generates outputs 4 at a time, using
     packed SIMD arithmetic.  Each of the 4 outputs at dst[m] (m=0,1,2,3) are
     obtained by taking the inner product between (kernels[p])[m+4n]
     (n=0,...,kernel_length-1) and src[m-leadin+n] (n=0,...,kernel_length-1),
     where p is equal to (`phase'+`off')>>`pshift' and `off' is equal to
     (1<<`pshift')/2.  After generating each group of 4 samples in `dst',
     the `phase' value is incremented by 4*`num'.  If this leaves
     `phase' >= `den', we subtract `den' from `phase' and increment the
     `src' pointer -- we may need to do this multiple times -- after which
     we are ready to generate the next group of 4 samples for `dst' using
     the same inner product formulation. */
{
#ifdef KDU_NO_SSE
  return false;
#else // Compilation support at least for SSE2
  if (kdu_mmx_level < 2)
    return false; // No run-time support even for SSE2
  int off = (1<<pshift)>>1;
  num <<= 2;
  float *sp_base = (float *) src->get_buf32();
  __m128 *dp = (__m128 *) dst->get_buf32();
  if (leadin == 0)
    { // In this case, we have to expand `kernel_length' successive input
      // samples each into 4 duplicate copies before applying the SIMD
      // arithmetic.
      if (kernel_length > 4)
        return false; // Not worth handling this unlikely case here
      assert(kernel_length >= 3);
      for (; length > 0; length-=4, dp++)
        { 
          __m128 *kern = (__m128 *) kernels[(int)((phase+off)>>pshift)];
          phase += num;
          int adj = (int)(phase / den);
          __m128 ival = _mm_loadu_ps(sp_base);
          __m128 val, sum;
          sp_base += adj;
          phase -= ((kdu_int64) adj) * den;
          val = _mm_shuffle_ps(ival,ival,0x00);
          sum = _mm_mul_ps(val,kern[0]);
          val = _mm_shuffle_ps(ival,ival,0x55); 
          sum = _mm_add_ps(sum,_mm_mul_ps(val,kern[1]));
          val = _mm_shuffle_ps(ival,ival,0xAA);
          sum = _mm_add_ps(sum,_mm_mul_ps(val,kern[2]));
          if (kernel_length > 3)
            { 
              val = _mm_shuffle_ps(ival,ival,0xFF);
              sum = _mm_add_ps(sum,_mm_mul_ps(val,kern[3]));
            }
          *dp = sum;
        }
      return true;
    }
  
  sp_base -= leadin;
# ifndef KDU_NO_SSSE3
  if (kdu_mmx_level >= 4)
    { // Full run-time support for SSSE3
      for (; length > 0; length-=4, dp++)
        {
          __m128 *kern = (__m128 *) kernels[(int)((phase+off)>>pshift)];
          phase += num;
          int kl, adj = (int)(phase / den);
          float *sp = sp_base; // Note; this is not aligned
          sp_base += adj;
          phase -= ((kdu_int64) adj) * den;
      
          __m128 val, val1, val2, sum=_mm_setzero_ps();
          val1 = _mm_loadu_ps(sp); sp += 4;
          for (kl=kernel_length; kl > 3; kl-=4, kern+=4)
            {
              val2 = _mm_loadu_ps(sp); sp += 4;
              val = _mm_mul_ps(val1,kern[0]); sum = _mm_add_ps(sum,val);
              val = kdrd_alignr_ps(val2,val1,4);
              val = _mm_mul_ps(val,kern[1]); sum = _mm_add_ps(sum,val);
              val = kdrd_alignr_ps(val2,val1,8);
              val = _mm_mul_ps(val,kern[2]); sum = _mm_add_ps(sum,val);
              val = kdrd_alignr_ps(val2,val1,12);
              val = _mm_mul_ps(val,kern[3]); sum = _mm_add_ps(sum,val);
              val1 = val2;
            }
          if (kl > 0)
            {
              val = _mm_mul_ps(val1,kern[0]); sum = _mm_add_ps(sum,val);
              if (kl > 1)
                {
                  val2 = _mm_loadu_ps(sp);
                  val = kdrd_alignr_ps(val2,val1,4);
                  val = _mm_mul_ps(val,kern[1]); sum = _mm_add_ps(sum,val);
                  if (kl > 2)
                    {
                      val = kdrd_alignr_ps(val2,val1,8);
                      val = _mm_mul_ps(val,kern[2]); sum = _mm_add_ps(sum,val);
                    }
                }
            }
          *dp = sum;
        }
      return true;
    }
# endif // End of SSSE3 specific code
  for (; length > 0; length-=4, dp++)
    {
      __m128 *kern = (__m128 *) kernels[(int)((phase+off)>>pshift)];
      phase += num;
      int kl, adj = (int)(phase / den);
      float *sp = sp_base; // Note; this is not aligned
      sp_base += adj;
      phase -= ((kdu_int64) adj) * den;
      
      __m128 val, sum=_mm_setzero_ps();
      for (kl=kernel_length; kl > 3; kl-=4, kern+=4, sp+=4)
        {
          val = _mm_loadu_ps(sp+0);
          val = _mm_mul_ps(val,kern[0]); sum = _mm_add_ps(sum,val);
          val = _mm_loadu_ps(sp+1);
          val = _mm_mul_ps(val,kern[1]); sum = _mm_add_ps(sum,val);
          val = _mm_loadu_ps(sp+2);
          val = _mm_mul_ps(val,kern[2]); sum = _mm_add_ps(sum,val);
          val = _mm_loadu_ps(sp+3);
          val = _mm_mul_ps(val,kern[3]); sum = _mm_add_ps(sum,val);
        }
      if (kl > 0)
        {
          val = _mm_loadu_ps(sp+0);
          val = _mm_mul_ps(val,kern[0]); sum = _mm_add_ps(sum,val);
          if (kl > 1)
            {
              val = _mm_loadu_ps(sp+1);
              val = _mm_mul_ps(val,kern[1]); sum = _mm_add_ps(sum,val);
              if (kl > 2)
                {
                  val = _mm_loadu_ps(sp+2);
                  val = _mm_mul_ps(val,kern[2]); sum = _mm_add_ps(sum,val);
                }
            }
        }
      *dp = sum;
    }
  return true;
#endif // Compilation support for SSE2
}

/*****************************************************************************/
/* INLINE                  simd_horz_resampling_fix16                        */
/*****************************************************************************/

inline bool
  simd_horz_resampling_fix16(int length, kdu_line_buf *src,
                             kdu_line_buf *dst, kdu_int64 phase,
                             kdu_int64 num, int den, int pshift,
                             kdu_int16 **kernels, int kernel_length,
                             int leadin)
{
#ifdef KDU_NO_SSE
  return false;
#else // Compilation support at least for SSE2
  if (kdu_mmx_level < 2)
    return false; // No run-time support even for SSE2
  int off = (1<<pshift)>>1;
  num <<= 3;
  kdu_int16 *sp_base = (kdu_int16 *) src->get_buf16();
  __m128i *dp = (__m128i *) dst->get_buf16();

  if (leadin == 0)
    { // In this case, we have to expand `kernel_length' successive input
      // samples each into 8 duplicate copies before applying the SIMD
      // arithmetic.
      if (kernel_length > 6)
        return false; // Not worth handling this unlikely case here
      assert(kernel_length >= 3);
      for (; length > 0; length-=8, dp++)
        { 
          __m128i *kern = (__m128i *) kernels[(int)((phase+off)>>pshift)];
          phase += num;
          int adj = (int)(phase / den);
          __m128i val, ival = _mm_loadu_si128((__m128i *) sp_base);
          ival = _mm_adds_epi16(ival,ival);
          sp_base += adj;
          __m128i sum = _mm_setzero_si128();
          phase -= ((kdu_int64) adj) * den;
          val=_mm_shuffle_epi32(_mm_shufflelo_epi16(ival,0x00),0x00);
          sum=_mm_sub_epi16(sum,_mm_mulhi_epi16(val,kern[0]));
          val=_mm_shuffle_epi32(_mm_shufflelo_epi16(ival,0x55),0x00);
          sum=_mm_sub_epi16(sum,_mm_mulhi_epi16(val,kern[1]));
          val=_mm_shuffle_epi32(_mm_shufflelo_epi16(ival,0xAA),0x00);
          sum=_mm_sub_epi16(sum,_mm_mulhi_epi16(val,kern[2]));
          if (kernel_length > 3)
            { 
              val=_mm_shuffle_epi32(_mm_shufflelo_epi16(ival,0xFF),0x00);
              sum=_mm_sub_epi16(sum,_mm_mulhi_epi16(val,kern[3]));
              if (kernel_length > 4)
                { 
                  val=_mm_shuffle_epi32(_mm_shufflehi_epi16(ival,0x00),0xAA);
                  sum=_mm_sub_epi16(sum,_mm_mulhi_epi16(val,kern[4]));
                  if (kernel_length > 5)
                    {
                    val=_mm_shuffle_epi32(_mm_shufflehi_epi16(ival,0x55),0xAA);
                    sum=_mm_sub_epi16(sum,_mm_mulhi_epi16(val,kern[5]));
                    }
                }
            }
          *dp = sum;
        }
      return true;
    }
  
  sp_base -= leadin; 
# ifndef KDU_NO_SSSE3
  if (kdu_mmx_level >= 4)
    { // Full run-time support for SSSE3
      for (; length > 0; length-=8, dp++)
        {
          __m128i *kern = (__m128i *) kernels[(int)((phase+off)>>pshift)];
          phase += num;
          int kl, adj = (int)(phase / den);
          __m128i *sp = (__m128i *) sp_base; // Note; this is not aligned
          sp_base += adj;
          phase -= ((kdu_int64) adj) * den;      
          __m128i val, val1, val2, sum=_mm_setzero_si128();
          val1 = _mm_loadu_si128(sp++); val1 = _mm_adds_epi16(val1,val1);
          for (kl=kernel_length; kl > 7; kl-=8, kern+=8)
            {
              val2= _mm_loadu_si128(sp++); val2 = _mm_adds_epi16(val2,val2);
              val = _mm_mulhi_epi16(val1,kern[0]); sum=_mm_sub_epi16(sum,val);
              val = _mm_alignr_epi8(val2,val1,2);
              val = _mm_mulhi_epi16(val,kern[1]); sum=_mm_sub_epi16(sum,val);
              val = _mm_alignr_epi8(val2,val1,4);
              val = _mm_mulhi_epi16(val,kern[2]); sum=_mm_sub_epi16(sum,val);
              val = _mm_alignr_epi8(val2,val1,6);
              val = _mm_mulhi_epi16(val,kern[3]); sum=_mm_sub_epi16(sum,val);
              val = _mm_alignr_epi8(val2,val1,8);
              val = _mm_mulhi_epi16(val,kern[4]); sum=_mm_sub_epi16(sum,val);
              val = _mm_alignr_epi8(val2,val1,10);
              val = _mm_mulhi_epi16(val,kern[5]); sum=_mm_sub_epi16(sum,val);
              val = _mm_alignr_epi8(val2,val1,12);
              val = _mm_mulhi_epi16(val,kern[6]); sum=_mm_sub_epi16(sum,val);
              val = _mm_alignr_epi8(val2,val1,14);
              val = _mm_mulhi_epi16(val,kern[7]); sum=_mm_sub_epi16(sum,val);
              val1 = val2;
            }
          if (kl > 0)
            {
              val = _mm_mulhi_epi16(val1,kern[0]); sum=_mm_sub_epi16(sum,val);
              if (kl > 1)
                {
                  val2 = _mm_loadu_si128(sp); val2 = _mm_adds_epi16(val2,val2);
                  val = _mm_alignr_epi8(val2,val1,2);
                  val = _mm_mulhi_epi16(val,kern[1]);
                  sum = _mm_sub_epi16(sum,val);
                  if (kl > 2)
                    {
                      val = _mm_alignr_epi8(val2,val1,4);
                      val = _mm_mulhi_epi16(val,kern[2]);
                      sum = _mm_sub_epi16(sum,val);
                      if (kl > 3)
                        {
                          val = _mm_alignr_epi8(val2,val1,6);
                          val = _mm_mulhi_epi16(val,kern[3]);
                          sum = _mm_sub_epi16(sum,val);
                          if (kl > 4)
                            {
                              val = _mm_alignr_epi8(val2,val1,8);
                              val = _mm_mulhi_epi16(val,kern[4]);
                              sum = _mm_sub_epi16(sum,val);
                              if (kl > 5)
                                {
                                  val = _mm_alignr_epi8(val2,val1,10);
                                  val = _mm_mulhi_epi16(val,kern[5]);
                                  sum = _mm_sub_epi16(sum,val);
                                  if (kl > 6)
                                    {
                                      val = _mm_alignr_epi8(val2,val1,12);    
                                      val = _mm_mulhi_epi16(val,kern[6]);
                                      sum = _mm_sub_epi16(sum,val);
                                    }
                                }
                            }
                        }
                    }
                }
            }
          *dp = sum;
        }
      return true;
    }
# endif // End of SSSE3 specific code
  for (; length > 0; length-=8, dp++)
    {
      __m128i *kern = (__m128i *) kernels[(int)((phase+off)>>pshift)];
      phase += num;
      int kl, adj = (int)(phase / den);
      kdu_int16 *sp = sp_base;
      sp_base += adj;
      phase -= ((kdu_int64) adj) * den;      
      __m128i val, sum=_mm_setzero_si128();
      for (kl=kernel_length; kl > 7; kl-=8, kern+=8, sp+=8)
        {
          val=_mm_loadu_si128((__m128i *)(sp+0)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[0]); sum=_mm_sub_epi16(sum,val);
          val=_mm_loadu_si128((__m128i *)(sp+1)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[1]); sum=_mm_sub_epi16(sum,val);
          val=_mm_loadu_si128((__m128i *)(sp+2)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[2]); sum=_mm_sub_epi16(sum,val);
          val=_mm_loadu_si128((__m128i *)(sp+3)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[3]); sum=_mm_sub_epi16(sum,val);
          val=_mm_loadu_si128((__m128i *)(sp+4)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[4]); sum=_mm_sub_epi16(sum,val);
          val=_mm_loadu_si128((__m128i *)(sp+5)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[5]); sum=_mm_sub_epi16(sum,val);
          val=_mm_loadu_si128((__m128i *)(sp+6)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[6]); sum=_mm_sub_epi16(sum,val);
          val=_mm_loadu_si128((__m128i *)(sp+7)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[7]); sum=_mm_sub_epi16(sum,val);
        }
      if (kl > 0)
        {
          val=_mm_loadu_si128((__m128i *)(sp+0)); val=_mm_adds_epi16(val,val);
          val = _mm_mulhi_epi16(val,kern[0]); sum=_mm_sub_epi16(sum,val);
          if (kl > 1)
            {
              val=_mm_loadu_si128((__m128i *)(sp+1));
              val=_mm_adds_epi16(val,val);
              val = _mm_mulhi_epi16(val,kern[1]);
              sum = _mm_sub_epi16(sum,val);
              if (kl > 2)
                {
                  val=_mm_loadu_si128((__m128i *)(sp+2));
                  val=_mm_adds_epi16(val,val);
                  val = _mm_mulhi_epi16(val,kern[2]);
                  sum = _mm_sub_epi16(sum,val);
                  if (kl > 3)
                    {
                      val=_mm_loadu_si128((__m128i *)(sp+3));
                      val=_mm_adds_epi16(val,val);
                      val = _mm_mulhi_epi16(val,kern[3]);
                      sum = _mm_sub_epi16(sum,val);
                      if (kl > 4)
                        {
                          val=_mm_loadu_si128((__m128i *)(sp+4));
                          val=_mm_adds_epi16(val,val);
                          val = _mm_mulhi_epi16(val,kern[4]);
                          sum = _mm_sub_epi16(sum,val);
                          if (kl > 5)
                            {
                              val=_mm_loadu_si128((__m128i *)(sp+5));
                              val=_mm_adds_epi16(val,val);
                              val = _mm_mulhi_epi16(val,kern[5]);
                              sum = _mm_sub_epi16(sum,val);
                              if (kl > 6)
                                {
                                  val=_mm_loadu_si128((__m128i *)(sp+6));
                                  val=_mm_adds_epi16(val,val);
                                  val = _mm_mulhi_epi16(val,kern[6]);
                                  sum = _mm_sub_epi16(sum,val);
                                }
                            }
                        }
                    }
                }
            }
        }
      *dp = sum;
    }
  return true;
#endif // Compilation support for SSE2
}

/*****************************************************************************/
/* INLINE                   simd_apply_white_stretch                         */
/*****************************************************************************/

inline bool
  simd_apply_white_stretch(kdu_sample16 *src, kdu_sample16 *dst,
                           int num_samples, int stretch_residual)
  /* Takes advantage of the alignment properties of `kdu_line_buf' buffers,
     to efficiently implement the white stretching algorithm for 16-bit
     channel buffers. */
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  if ((kdu_mmx_level < 2) || (num_samples < 1))
    return false; // Not worth special processing
  kdu_int32 stretch_offset = -((-(stretch_residual<<(KDU_FIX_POINT-1))) >> 16);
  num_samples = (num_samples+7)>>3;
  if (stretch_residual <= 0x7FFF)
    { // Use full multiplication-based approach
      __m128i factor = _mm_set1_epi16((kdu_int16) stretch_residual);
      __m128i offset = _mm_set1_epi16((kdu_int16) stretch_offset);
      __m128i *sp = (__m128i *) src;
      __m128i *dp = (__m128i *) dst;
      for (int c=0; c < num_samples; c++)
        {
          __m128i val = sp[c];
          __m128i residual = _mm_mulhi_epi16(val,factor);
          val = _mm_add_epi16(val,offset);
          dp[c] = _mm_add_epi16(val,residual);
        }
    }
  else
    { // Large stretch residual -- can only happen with 1-bit original data
      int diff=(1<<16)-((int) stretch_residual), downshift = 1;
      while ((diff & 0x8000) == 0)
        { diff <<= 1; downshift++; }
      __m128i shift = _mm_cvtsi32_si128(downshift);
      __m128i offset = _mm_set1_epi16((kdu_int16) stretch_offset);
      __m128i *sp = (__m128i *) src;
      __m128i *dp = (__m128i *) dst;
      for (int c=0; c < num_samples; c++)
        {
          __m128i val = sp[c];
          __m128i twice_val = _mm_add_epi16(val,val);
          __m128i shifted_val = _mm_sra_epi16(val,shift);
          val = _mm_sub_epi16(twice_val,shifted_val);
          dp[c] = _mm_add_epi16(val,offset);
        }
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE             simd_transfer_to_consecutive_bytes                     */
/*****************************************************************************/

inline bool
  simd_transfer_to_consecutive_bytes(kdu_sample16 *src, kdu_byte *dst,
                                     int num_samples, kdu_int16 offset,
                                     int downshift, kdu_int16 mask)
{
  if ((kdu_mmx_level < 2) || (num_samples < 16))
    return false;

  kdu_int32 vec_offset = offset;  vec_offset += vec_offset << 16;
  kdu_int32 vec_max = (kdu_int32)(~mask);  vec_max += vec_max << 16;
  int c, num_blocks = num_samples >> 4; // Number of whole 16-sample blocks
  __m128i *sp = (__m128i *) src;
  __m128i *dp = (__m128i *) dst;
  __m128i voff = _mm_cvtsi32_si128(vec_offset);
  voff = _mm_shuffle_epi32(voff,0);
  __m128i vmax = _mm_cvtsi32_si128(vec_max);
  vmax = _mm_shuffle_epi32(vmax,0);
  __m128i vmin = _mm_setzero_si128();
  __m128i shift = _mm_cvtsi32_si128(downshift);
  for (c=0; c < num_blocks; c++)
    {
      __m128i low = _mm_load_si128(sp+2*c);
      low = _mm_add_epi16(low,voff); // Add the offset
      low = _mm_sra_epi16(low,shift); // Apply the downshift
      low = _mm_max_epi16(low,vmin); // Clip to min value of 0
      low = _mm_min_epi16(low,vmax); // Clip to max value of `mask'
      __m128i high = _mm_load_si128(sp+2*c+1);
      high = _mm_add_epi16(high,voff); // Add the offset
      high = _mm_sra_epi16(high,shift); // Apply the downshift
      high = _mm_max_epi16(high,vmin); // Clip to min value of 0
      high = _mm_min_epi16(high,vmax); // Clip to max value of `mask'
      __m128i packed = _mm_packus_epi16(low,high);
      _mm_storeu_si128(dp+c,packed);
    }
  for (c=num_blocks<<4; c < num_samples; c++)
    {
      kdu_int16 val = (src[c].ival+offset)>>downshift;
      if (val & mask)
        val = (val < 0)?0:~mask;
      dst[c] = (kdu_byte) val;
    }
  return true;
}

/*****************************************************************************/
/* INLINE                  simd_interleaved_transfer                         */
/*****************************************************************************/

inline bool
  simd_interleaved_transfer(kdu_byte *dst_base, kdu_byte channel_mask[],
                            int num_pixels, int num_channels, int prec_bits,
                            kdu_line_buf *src_line0, kdu_line_buf *src_line1,
                            kdu_line_buf *src_line2, kdu_line_buf *src_line3,
                            bool fill_alpha)
  /* This function performs the transfer of processed short integers to
     output channel bytes all at once, assuming interleaved output channels.
     Notionally, there are 4 output channels, whose first samples are at
     locations `dst_base' + k, for k=0,1,2,3.  The corresponding source
     line buffers are identified by `src_line0' through `src_line3'.  The
     actual number of real output channels is identified by `num_channels';
     these are the ones with location `dst_base'+k where `channel_mask'[k]
     is 0xFF.  All other locations have `channel_mask'[k]=0 -- nothing will
     be written to those locations, but the corresponding `src_line'
     argument is still usable (although not unique).  This allows for
     simple generic implementations.  If `fill_alpha' is true, the last
     channel is simply to be filled with 0xFF; it's mask is irrelevant
     and there is no need to read from the corresponding source line. */
{
#ifdef KDU_NO_SSE
  return false;
#else // !KDU_NO_SSE
  assert(num_channels <= 4);
  int cm_val = ((int *) channel_mask)[0]; // Load all 4 mask bytes into 1 dword
  if ((kdu_mmx_level < 2) || ((cm_val & 0x00FFFFFF) != 0x00FFFFFF))
    return false; // Present implementation assumes first 3 channels are used
  cm_val >>= 24;  // Keep just the last source mask byte
  int downshift = KDU_FIX_POINT-prec_bits;
  kdu_int16 offset = (1<<downshift)>>1; // Rounding offset for the downshift
  offset += (1<<KDU_FIX_POINT)>>1; // Convert from signed to unsigned
  kdu_int16 mask = (kdu_int16)((-1) << prec_bits);
  kdu_int32 vec_offset = offset;  vec_offset += vec_offset << 16;
  kdu_int32 vec_max = (kdu_int32)(~mask);  vec_max += vec_max << 16;

  kdu_sample16 *src0 = src_line0->get_buf16();
  kdu_sample16 *src1 = src_line1->get_buf16();
  kdu_sample16 *src2 = src_line2->get_buf16();
  kdu_sample16 *src3 = src_line3->get_buf16();

  int c, num_blocks = num_pixels>>3;
  __m128i *dp  = (__m128i *) dst_base;
  __m128i *sp0 = (__m128i *) src0;
  __m128i *sp1 = (__m128i *) src1;
  __m128i *sp2 = (__m128i *) src2;
  __m128i shift = _mm_cvtsi32_si128(downshift);
  __m128i voff = _mm_cvtsi32_si128(vec_offset);
  voff = _mm_shuffle_epi32(voff,0); // Create 8 replicas of offset in `voff'
  __m128i vmax = _mm_cvtsi32_si128(vec_max);
  vmax = _mm_shuffle_epi32(vmax,0); // Create 8 replicas of max value in `vmax'
  if (fill_alpha)
    { // Three source channels available; overwrite alpha channel with FF
      __m128i vmin = _mm_setzero_si128();
      __m128i mask = _mm_setzero_si128(); // Avoid compiler warnings
      mask = _mm_cmpeq_epi16(mask,mask); // Set all bits to 1
      mask = _mm_slli_epi16(mask,8); // Leaves 0xFF in MSB of each 16-bit word
      for (c=0; c < num_blocks; c++)
        {
          __m128i val0 = _mm_load_si128(sp0+c);
          val0 = _mm_add_epi16(val0,voff); // Add the offset
          val0 = _mm_sra_epi16(val0,shift); // Apply the downshift
          val0 = _mm_max_epi16(val0,vmin); // Clip to min value of 0
          val0 = _mm_min_epi16(val0,vmax); // Clip to max value of `mask'
          __m128i val1 = _mm_load_si128(sp1+c);
          val1 = _mm_add_epi16(val1,voff); // Add the offset
          val1 = _mm_sra_epi16(val1,shift); // Apply the downshift
          val1 = _mm_max_epi16(val1,vmin); // Clip to min value of 0
          val1 = _mm_min_epi16(val1,vmax); // Clip to max value of `mask'
          val1 = _mm_slli_epi16(val1,8);
          val1 = val0 = _mm_or_si128(val0,val1); // Interleave val0 and val1
          __m128i val2 = _mm_load_si128(sp2+c);
          val2 = _mm_add_epi16(val2,voff); // Add the offset
          val2 = _mm_sra_epi16(val2,shift); // Apply the downshift
          val2 = _mm_max_epi16(val2,vmin); // Clip to min value of 0
          val2 = _mm_min_epi16(val2,vmax); // Clip to max value of `mask'
          val2 = _mm_or_si128(val2,mask); // Fill alpha channel with FF

          val0 = _mm_unpacklo_epi16(val0,val2);
          _mm_storeu_si128(dp+2*c,val0); // Write low pixel*4

          val1 = _mm_unpackhi_epi16(val1,val2);
          _mm_storeu_si128(dp+2*c+1,val1); // Write high pixel*4
        }
    }
  else if (cm_val == 0)
    { // Three source channels available; leave the alpha channel unchanged
      __m128i mask = _mm_setzero_si128(); // Avoid compiler warnings
      mask = _mm_cmpeq_epi16(mask,mask); // Set all bits to 1
      mask = _mm_slli_epi32(mask,24); // Leaves 0xFF in MSB of each 32-bit word
      for (c=0; c < num_blocks; c++)
        {
          __m128i multival = _mm_setzero_si128();
          __m128i val0 = _mm_load_si128(sp0+c);
          val0 = _mm_add_epi16(val0,voff); // Add the offset
          val0 = _mm_sra_epi16(val0,shift); // Apply the downshift
          val0 = _mm_max_epi16(val0,multival); // Clip to min value of 0
          val0 = _mm_min_epi16(val0,vmax); // Clip to max value of `mask'
          __m128i val1 = _mm_load_si128(sp1+c);
          val1 = _mm_add_epi16(val1,voff); // Add the offset
          val1 = _mm_sra_epi16(val1,shift); // Apply the downshift
          val1 = _mm_max_epi16(val1,multival); // Clip to min value of 0
          val1 = _mm_min_epi16(val1,vmax); // Clip to max value of `mask'
          val1 = _mm_slli_epi16(val1,8);
          val1 = val0 = _mm_or_si128(val0,val1); // Interleave val0 and val1
          __m128i val2 = _mm_load_si128(sp2+c);
          val2 = _mm_add_epi16(val2,voff); // Add the offset
          val2 = _mm_sra_epi16(val2,shift); // Apply the downshift
          val2 = _mm_max_epi16(val2,multival); // Clip to min value of 0
          val2 = _mm_min_epi16(val2,vmax); // Clip to max value of `mask'

          multival = _mm_loadu_si128(dp+2*c); // Load existing low pixel*4
          multival = _mm_and_si128(multival,mask); // Mask off all but alpha
          val0 = _mm_unpacklo_epi16(val0,val2);
          val0 = _mm_or_si128(val0,multival); // Blend existing alpha into val0
          _mm_storeu_si128(dp+2*c,val0); // Write new low pixel*4

          multival = _mm_loadu_si128(dp+2*c+1); // Load existing high pixel*4
          multival = _mm_and_si128(multival,mask); // Mask off all but alpha
          val1 = _mm_unpackhi_epi16(val1,val2);
          val1 = _mm_or_si128(val1,multival); // Blend existing alpha into val0
          _mm_storeu_si128(dp+2*c+1,val1); // Write new high pixel*4
        }
    }
  else
    { // Four source channels available
      __m128i vmin = _mm_setzero_si128();
      __m128i *sp3 = (__m128i *) src3;
      for (c=0; c < num_blocks; c++)
        {
          __m128i val0 = _mm_load_si128(sp0+c);
          val0 = _mm_add_epi16(val0,voff); // Add the offset
          val0 = _mm_sra_epi16(val0,shift); // Apply the downshift
          val0 = _mm_max_epi16(val0,vmin); // Clip to min value of 0
          val0 = _mm_min_epi16(val0,vmax); // Clip to max value of `mask'
          __m128i val1 = _mm_load_si128(sp1+c);
          val1 = _mm_add_epi16(val1,voff); // Add the offset
          val1 = _mm_sra_epi16(val1,shift); // Apply the downshift
          val1 = _mm_max_epi16(val1,vmin); // Clip to min value of 0
          val1 = _mm_min_epi16(val1,vmax); // Clip to max value of `mask'
          val1 = _mm_slli_epi16(val1,8);
          val1 = val0 = _mm_or_si128(val0,val1); // Interleave val0 and val1
          __m128i val2 = _mm_load_si128(sp2+c);
          val2 = _mm_add_epi16(val2,voff); // Add the offset
          val2 = _mm_sra_epi16(val2,shift); // Apply the downshift
          val2 = _mm_max_epi16(val2,vmin); // Clip to min value of 0
          val2 = _mm_min_epi16(val2,vmax); // Clip to max value of `mask'
          __m128i val3 = _mm_load_si128(sp3+c);
          val3 = _mm_add_epi16(val3,voff); // Add the offset
          val3 = _mm_sra_epi16(val3,shift); // Apply the downshift
          val3 = _mm_max_epi16(val3,vmin); // Clip to min value of 0
          val3 = _mm_min_epi16(val3,vmax); // Clip to max value of `mask'
          val3 = _mm_slli_epi16(val3,8);
          val2 = _mm_or_si128(val2,val3);

          val0 = _mm_unpacklo_epi16(val0,val2);
          _mm_storeu_si128(dp+2*c,val0); // Write low pixel*4

          val1 = _mm_unpackhi_epi16(val1,val2);
          _mm_storeu_si128(dp+2*c+1,val1); // Write high pixel*4
        }
    }

  for (c=num_blocks<<3; c < num_pixels; c++)
    { // Need to generate trailing samples one-by-one
      kdu_int16 val;

      val = (src0[c].ival+offset) >> downshift;
      if (val & mask)
        val = (val < 0)?0:~mask;
      dst_base[4*c+0] = (kdu_byte) val;

      val = (src1[c].ival+offset) >> downshift;
      if (val & mask)
        val = (val < 0)?0:~mask;
      dst_base[4*c+1] = (kdu_byte) val;

      val = (src2[c].ival+offset) >> downshift;
      if (val & mask)
        val = (val < 0)?0:~mask;
      dst_base[4*c+2] = (kdu_byte) val;
      
      if (fill_alpha)
        dst_base[4*c+3] = 0xFF;
      else if (cm_val)
        {
          val = (src3[c].ival+offset) >> downshift;
          if (val & mask)
            val = (val < 0)?0:~mask;
          dst_base[4*c+3] = (kdu_byte) val;
        }
    }

  return true;
#endif // !KDU_NO_SSE
}

#endif // X86_REGION_DECOMPRESSOR_LOCAL_H
