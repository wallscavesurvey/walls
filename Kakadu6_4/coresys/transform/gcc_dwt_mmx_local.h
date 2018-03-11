/*****************************************************************************/
// File: gcc_dwt_mmx_local.h [scope = CORESYS/TRANSFORMS]
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
   Implements various critical functions for DWT analysis and synthesis using
MMX/SSE/SSE2 instructions, realized through explicit inline assembly
instructions.
******************************************************************************/

#ifndef GCC_DWT_MMX_LOCAL_H
#define GCC_DWT_MMX_LOCAL_H

#define W97_FACT_0 ((float) -1.586134342)
#define W97_FACT_1 ((float) -0.052980118)
#define W97_FACT_2 ((float)  0.882911075)
#define W97_FACT_3 ((float)  0.443506852)

static kdu_int16 simd_w97_rem[4] =
  {(kdu_int16) floor(0.5 + (W97_FACT_0+2.0)*(double)(1<<16)),
   (kdu_int16) floor(0.5 + W97_FACT_1*(double)(1<<19)),
   (kdu_int16) floor(0.5 + (W97_FACT_2-1.0)*(double)(1<<16)),
   (kdu_int16) floor(0.5 + W97_FACT_3*(double)(1<<16))};
static kdu_int16 simd_w97_preoff[4] =
  {(kdu_int16) floor(0.5 + 0.5/(W97_FACT_0+2.0)),
   0,
   (kdu_int16) floor(0.5 + 0.5/(W97_FACT_2-1.0)),
   (kdu_int16) floor(0.5 + 0.5/W97_FACT_3)};


/* ========================================================================= */
/*                      Define Convenient MMX Macros                        */
/* ========================================================================= */

#define MOVQ_read(_dst,_src) __asm__ volatile \
  ("movq %0, %%" #_dst : : "m" (_src))
#define MOVQ_xfer(_dst,_src) __asm__ volatile \
  ("movq %%" #_src ", %%" #_dst : :)
#define MOVQ_write(_dst,_src) __asm__ volatile \
  ("movq %%" #_src ", %0" : "=m" (_dst) :)

#define MOVD_read(_dst,_src) __asm__ volatile \
  ("movd %0, %%" #_dst : : "m" (_src))
#define MOVD_write(_dst,_src) __asm__ volatile \
  ("movd %%" #_src ", %0" : "=m" (_dst) :)

#define MOVDQA_read(_dst,_src) __asm__ volatile \
  ("movdqa %0, %%" #_dst : : "m" (_src))
#define MOVDQA_xfer(_dst,_src) __asm__ volatile \
  ("movdqa %%" #_src ", %%" #_dst : :)
#define MOVDQA_write(_dst,_src) __asm__ volatile \
  ("movdqa %%" #_src ", %0" : "=m" (_dst) :)

#define MOVDQU_read(_dst,_src) __asm__ volatile \
  ("movdqu %0, %%" #_dst : : "m" (_src))
#define MOVDQU_xfer(_dst,_src) __asm__ volatile \
  ("movdqu %%" #_src ", %%" #_dst : :)
#define MOVDQU_write(_dst,_src) __asm__ volatile \
  ("movdqu %%" #_src ", %0" : "=m" (_dst) :)

#define MMX_OP_reg(_op,_dst,_src) __asm__ volatile \
  (#_op " %%" #_src ", %%" #_dst : :)
#define MMX_OP_mem(_op,_dst,_src) __asm__ volatile \
  (#_op " %0, %%" #_dst : : "m" (_src))
#define MMX_OP_imm(_op,_dst,_imm) __asm__ volatile \
  (#_op " %0, %%" #_dst : : "i" (_imm))

#define EMMS() __asm__ __volatile__("emms")

#define PAND_reg(_tgt,_in) MMX_OP_reg(pand,_tgt,_in)
#define PAND_mem(_tgt,_in) MMX_OP_mem(pand,_tgt,_in)

#define PXOR_reg(_tgt,_in) MMX_OP_reg(pxor,_tgt,_in)
#define PXOR_mem(_tgt,_in) MMX_OP_mem(pxor,_tgt,_in)

#define PADDW_reg(_tgt,_in) MMX_OP_reg(paddw,_tgt,_in)
#define PADDW_mem(_tgt,_in) MMX_OP_mem(paddw,_tgt,_in)

#define PSUBW_reg(_tgt,_in) MMX_OP_reg(psubw,_tgt,_in)
#define PSUBW_mem(_tgt,_in) MMX_OP_mem(psubw,_tgt,_in)

#define PADDD_reg(_tgt,_in) MMX_OP_reg(paddd,_tgt,_in)
#define PADDD_mem(_tgt,_in) MMX_OP_mem(paddd,_tgt,_in)

#define PMULHW_reg(_tgt,_in) MMX_OP_reg(pmulhw,_tgt,_in)
#define PMULHW_mem(_tgt,_in) MMX_OP_mem(pmulhw,_tgt,_in)

#define PMADDWD_reg(_tgt,_in) MMX_OP_reg(pmaddwd,_tgt,_in)
#define PMADDWD_mem(_tgt,_in) MMX_OP_mem(pmaddwd,_tgt,_in)

#define PSRAW_imm(_tgt,_val) MMX_OP_imm(psraw,_tgt,_val)
#define PSRAW_reg(_tgt,_val) MMX_OP_reg(psraw,_tgt,_val)

#define PSRAD_imm(_tgt,_val) MMX_OP_imm(psrad,_tgt,_val)
#define PSRAD_reg(_tgt,_val) MMX_OP_reg(psrad,_tgt,_val)

#define PSRLD_imm(_tgt,_val) MMX_OP_imm(psrld,_tgt,_val)
#define PSRLD_reg(_tgt,_val) MMX_OP_reg(psrld,_tgt,_val)

#define PSLLW_imm(_tgt,_val) MMX_OP_imm(psllw,_tgt,_val)
#define PSLLW_reg(_tgt,_val) MMX_OP_reg(psllw,_tgt,_val)

#define PSLLD_imm(_tgt,_val) MMX_OP_imm(pslld,_tgt,_val)
#define PSLLD_reg(_tgt,_val) MMX_OP_reg(pslld,_tgt,_val)

#define PACKSSDW_reg(_tgt,_in) MMX_OP_reg(packssdw,_tgt,_in)
#define PACKSSDW_mem(_tgt,_in) MMX_OP_mem(packssdw,_tgt,_in)

#define PUNPCKLWD_reg(_tgt,_in) MMX_OP_reg(punpcklwd,_tgt,_in)
#define PUNPCKLWD_mem(_tgt,_in) MMX_OP_mem(punpcklwd,_tgt,_in)

#define PUNPCKHWD_reg(_tgt,_in) MMX_OP_reg(punpckhwd,_tgt,_in)
#define PUNPCKHWD_mem(_tgt,_in) MMX_OP_mem(punpckhwd,_tgt,_in)

#define PCMPEQW_reg(_tgt,_val) MMX_OP_reg(pcmpeqw,_tgt,_val)
#define PCMPEQD_reg(_tgt,_val) MMX_OP_reg(pcmpeqd,_tgt,_val)


/* ========================================================================= */
/*                  MMX/SSE functions for 16-bit Samples                     */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                     simd_2tap_h_synth                              */
/*****************************************************************************/

static inline bool
  simd_2tap_h_synth(register kdu_int16 *src, register kdu_int16 *dst,
                    register int samples, kd_lifting_step *step)
  /* Efficient general purpose horizontal lifting step for lifting steps
     with at most 2 taps.  If the lifting step has only one input tap, the
     implementation costs as much as if it had 2 taps.  This routine is about
     50% slower than the special-purpose routines which handle Part-1 kernels
     only.  However, it is still a great deal faster than direct
     implementation of the lifting steps. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length == 1) || (step->support_length == 2));
  if (kdu_mmx_level < 2)
    return false;

  union {
      kdu_int16 word[8];
      kdu_int32 dword[4];
    } vec_lambda;
  kdu_int32 vec_offset[4];
  int downshift = step->downshift;
  vec_lambda.word[0] = step->icoeffs[0];
  vec_lambda.word[1] = (step->support_length==2)?(step->icoeffs[1]):0;
  vec_offset[0] = step->rounding_offset;
  vec_lambda.dword[3] = vec_lambda.dword[2] =
    vec_lambda.dword[1] = vec_lambda.dword[0];
  vec_offset[3] = vec_offset[2] = vec_offset[1] = vec_offset[0];

  MOVDQU_read(XMM0,*vec_lambda.dword);
  MOVDQU_read(XMM1,*vec_offset);
  MOVD_read(XMM2,downshift);
  PCMPEQW_reg(XMM7,XMM7);          // Set XMM7 to 1's
  PSRLD_imm(XMM7,16);              // Leaves mask for low words in each dword
  MOVDQU_read(XMM3,*(src));
  MOVDQU_read(XMM4,*(src+1));
  for (register int c=0; c < samples; c+=8)
    {
      PMADDWD_reg(XMM3,XMM0);
      PMADDWD_reg(XMM4,XMM0);
      MOVDQA_read(XMM6,*(dst+c));
      PADDD_reg(XMM3,XMM1);
      PADDD_reg(XMM4,XMM1);
      PSRAD_reg(XMM3,XMM2);
      PSRAD_reg(XMM4,XMM2);
      PAND_reg(XMM3,XMM7);   // Zero out high words of XMM3
      PSLLD_imm(XMM4,16);    // Move XMM4 results into high words
      PSUBW_reg(XMM6,XMM3);
      MOVDQU_read(XMM3,*(src+c+8));
      PSUBW_reg(XMM6,XMM4);
      MOVDQU_read(XMM4,*(src+c+9));
      MOVDQA_write(*(dst+c),XMM6);
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_4tap_h_synth                              */
/*****************************************************************************/

static inline bool
  simd_4tap_h_synth(register kdu_int16 *src, register kdu_int16 *dst,
                    register int samples, kd_lifting_step *step)
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length >= 3) || (step->support_length <= 4));
  if (kdu_mmx_level < 2)
    return false;

  union {
      kdu_int16 word[8];
      kdu_int32 dword[4];
    } vec_lambda1, vec_lambda2;

  kdu_int32 vec_offset[4];
  int downshift = step->downshift;
  vec_lambda1.word[0] = step->icoeffs[0];
  vec_lambda1.word[1] = step->icoeffs[1];
  vec_lambda2.word[0] = step->icoeffs[2];
  vec_lambda2.word[1] = (step->support_length==4)?(step->icoeffs[3]):0;
  vec_offset[0] = step->rounding_offset;
  vec_lambda1.dword[3] = vec_lambda1.dword[2] =
    vec_lambda1.dword[1] = vec_lambda1.dword[0];
  vec_lambda2.dword[3] = vec_lambda2.dword[2] =
    vec_lambda2.dword[1] = vec_lambda2.dword[0];
  vec_offset[3] = vec_offset[2] = vec_offset[1] = vec_offset[0];

  MOVDQU_read(XMM0,*vec_lambda1.dword);
  MOVDQU_read(XMM1,*vec_lambda2.dword);
  MOVDQU_read(XMM2,*vec_offset);
  MOVD_read(XMM3,downshift);
  PCMPEQW_reg(XMM7,XMM7);        // Set XMM7 to 1's
  PSRLD_imm(XMM7,16);            // Leaves mask for low words in each dword
  for (register int c=0; c < samples; c+=8)
    {
      MOVDQU_read(XMM4,*(src+c));
      MOVDQU_read(XMM5,*(src+c+2));
      PMADDWD_reg(XMM4,XMM0); // Multiply by first two lifting step coefficients
      PMADDWD_reg(XMM5,XMM1); // Multiply by second two lifting step coefficients
      MOVDQU_read(XMM6,*(src+c+1));
      PADDD_reg(XMM4,XMM5);   // Add products
      PADDD_reg(XMM4,XMM2);   // Add rounding offset
      MOVDQU_read(XMM5,*(src+c+3));
      PSRAD_reg(XMM4,XMM3);   // Downshift to get results in low-order words
      PMADDWD_reg(XMM6,XMM0); // Multiply by first two lifting step coefficients
      PMADDWD_reg(XMM5,XMM1); // Multiply by second two lifting step coefficients
      PADDD_reg(XMM6,XMM5);   // Add products
      MOVDQA_read(XMM5,*(dst+c));
      PADDD_reg(XMM6,XMM2);   // Add rounding offset
      PSRAD_reg(XMM6,XMM3);   // Downshift to get results in low-order words
      PAND_reg(XMM4,XMM7);    // Zero out high words of XMM4
      PSLLD_imm(XMM6,16);     // Move XMM6 results into high words
      PSUBW_reg(XMM5,XMM4);
      PSUBW_reg(XMM5,XMM6);
      MOVDQA_write(*(dst+c),XMM5);
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_2tap_v_synth                              */
/*****************************************************************************/

static inline bool
  simd_2tap_v_synth(register kdu_int16 *src1, register kdu_int16 *src2,
                    register kdu_int16 *dst_in, register kdu_int16 *dst_out,
                    register int samples, kd_lifting_step *step)
  /* Efficient general purpose vertical lifting step for lifting steps
     with at most 2 taps.  If the lifting step has only one input tap, the
     implementation costs as much as if it had 2 taps and `src2' is expected
     to point to a valid address (typically identical to `src1').  This
     routine is about 50% slower than the special-purpose routines which
     handle Part-1 kernels only.  However, it is still a great deal faster
     than direct implementation of the lifting steps. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length == 1) || (step->support_length == 2));
  if (kdu_mmx_level < 2)
    return false;

  union {
      kdu_int16 word[8];
      kdu_int32 dword[4];
    } vec_lambda;
  kdu_int32 vec_offset[4];
  int downshift = step->downshift;
  vec_lambda.word[0] = step->icoeffs[0];
  vec_lambda.word[1] = (step->support_length==2)?(step->icoeffs[1]):0;
  vec_offset[0] = step->rounding_offset;
  vec_lambda.dword[3] = vec_lambda.dword[2] =
    vec_lambda.dword[1] = vec_lambda.dword[0];
  vec_offset[3] = vec_offset[2] = vec_offset[1] = vec_offset[0];

  MOVDQU_read(XMM0,*vec_lambda.dword);
  MOVDQU_read(XMM1,*vec_offset);
  MOVD_read(XMM2,downshift);
  MOVDQA_read(XMM3,*src1);
  MOVDQA_read(XMM4,*src2);
  MOVDQA_xfer(XMM5,XMM3);
  for (register int c=0; c < samples; c+=8)
    {
      MOVDQA_read(XMM6,*(dst_in+c));
      PUNPCKHWD_reg(XMM3,XMM4);
      PUNPCKLWD_reg(XMM5,XMM4);
      PMADDWD_reg(XMM3,XMM0);
      PMADDWD_reg(XMM5,XMM0);
      PADDD_reg(XMM3,XMM1);
      PADDD_reg(XMM5,XMM1);
      PSRAD_reg(XMM3,XMM2);
      PSRAD_reg(XMM5,XMM2);
      MOVDQA_read(XMM4,*(src2+c+8)); // Read ahead next 8 source 1 words
      PACKSSDW_reg(XMM5,XMM3);
      MOVDQA_read(XMM3,*(src1+c+8)); // Read ahead next 8 source 2 words
      PSUBW_reg(XMM6,XMM5);
      MOVDQA_write(*(dst_out+c),XMM6);
      MOVDQA_xfer(XMM5,XMM3);
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_4tap_v_synth                              */
/*****************************************************************************/

static inline bool
  simd_4tap_v_synth(register kdu_int16 *src1, register kdu_int16 *src2,
                    kdu_int16 *src3, kdu_int16 *src4,
                    register kdu_int16 *dst_in, register kdu_int16 *dst_out,
                    register int samples, kd_lifting_step *step)
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length >= 3) || (step->support_length <= 4));
  if (kdu_mmx_level < 2)
    return false;

  union {
      kdu_int16 word[8];
      kdu_int32 dword[4];
    } vec_lambda1, vec_lambda2;

  kdu_int32 vec_offset[4];
  int downshift = step->downshift;
  vec_lambda1.word[0] = step->icoeffs[0];
  vec_lambda1.word[1] = step->icoeffs[1];
  vec_lambda2.word[0] = step->icoeffs[2];
  vec_lambda2.word[1] = (step->support_length==4)?(step->icoeffs[3]):0;
  vec_offset[0] = step->rounding_offset;
  vec_lambda1.dword[3] = vec_lambda1.dword[2] =
    vec_lambda1.dword[1] = vec_lambda1.dword[0];
  vec_lambda2.dword[3] = vec_lambda2.dword[2] =
    vec_lambda2.dword[1] = vec_lambda2.dword[0];
  vec_offset[3] = vec_offset[2] = vec_offset[1] = vec_offset[0];

  MOVDQU_read(XMM0,*vec_lambda1.dword);
  MOVDQU_read(XMM1,*vec_lambda2.dword);
  MOVDQU_read(XMM2,*vec_offset);
  for (register int c=0; c < samples; c+=8)
    {
      MOVDQA_read(XMM3,*(src1+c));
      MOVDQA_read(XMM4,*(src2+c));
      MOVDQA_xfer(XMM5,XMM3);
      PUNPCKHWD_reg(XMM3,XMM4);
      PUNPCKLWD_reg(XMM5,XMM4);
      PMADDWD_reg(XMM3,XMM0);
      MOVDQA_read(XMM6,*(src3+c));
      PMADDWD_reg(XMM5,XMM0);
      MOVDQA_read(XMM7,*(src4+c));
      MOVDQA_xfer(XMM4,XMM6);
      PUNPCKHWD_reg(XMM6,XMM7);
      PUNPCKLWD_reg(XMM4,XMM7);
      PMADDWD_reg(XMM6,XMM1);
      PMADDWD_reg(XMM4,XMM1);
      MOVD_read(XMM7,downshift);
      PADDD_reg(XMM3,XMM6);  // Form first sum of products
      PADDD_reg(XMM5,XMM4);  // Form second sum of products

      MOVDQA_read(XMM6,*(dst_in+c));
      PADDD_reg(XMM3,XMM2);  // Add offset
      PADDD_reg(XMM5,XMM2);  // Add offset
      PSRAD_reg(XMM3,XMM7);  // Downshift
      PSRAD_reg(XMM5,XMM7);  // Downshift

      PACKSSDW_reg(XMM5,XMM3);
      PSUBW_reg(XMM6,XMM5);
      MOVDQA_write(*(dst_out+c),XMM6);
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                    simd_W5X3_h_analysis                            */
/*****************************************************************************/

static inline bool
  simd_W5X3_h_analysis(register kdu_int16 *src, register kdu_int16 *dst,
                       register int samples, kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
  if (kdu_mmx_level < 1)
    return false;

  kdu_int16 vec_offset[4];
  vec_offset[0] = vec_offset[1] = vec_offset[2] =
    vec_offset[3] = (kdu_int16)((1<<step->downshift)>>1);
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));

  // Implementation based on 64-bit operands using only MMX instructions
  MOVQ_read(MM0,*(vec_offset));
  MOVD_read(MM1,downshift);
  if (int_coeff > 0)
    for (register int c=0; c < samples; c+=4)
      {
        MOVQ_xfer(MM2,MM0);         // start with the offset
        PADDW_mem(MM2,*(src+c));   // add 1'st source sample
        PADDW_mem(MM2,*(src+c+1)); // add 2'nd source sample
        MOVQ_read(MM3,*(dst+c));
        PSRAW_reg(MM2,MM1);         // shift right by the `downshift' value
        PADDW_reg(MM3,MM2);        // add to dest sample
        MOVQ_write(*(dst+c),MM3);
      }
  else
    for (register int c=0; c < samples; c+=4)
      {
        MOVQ_xfer(MM2,MM0);         // start with the offset
        PSUBW_mem(MM2,*(src+c));   // subtract 1'st source sample
        PSUBW_mem(MM2,*(src+c+1)); // subtract 2'nd source sample
        MOVQ_read(MM3,*(dst+c));
        PSRAW_reg(MM2,MM1);         // shift right by the `downshift' value
        PADDW_reg(MM3,MM2);        // add to dest sample
        MOVQ_write(*(dst+c),MM3);
      }
  EMMS(); // Clear MMX registers for use by FPU
  return true;
}

/*****************************************************************************/
/* INLINE                      simd_W5X3_h_synth                             */
/*****************************************************************************/

static inline bool
  simd_W5X3_h_synth(register kdu_int16 *src, register kdu_int16 *dst,
                    register int samples, kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
  if (kdu_mmx_level < 1)
    return false;

  kdu_int16 vec_offset[4];
  vec_offset[0] = vec_offset[1] = vec_offset[2] =
    vec_offset[3] = (kdu_int16)((1<<step->downshift)>>1);
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));

  // Implementation based on 64-bit operands using only MMX instructions
  MOVQ_read(MM0,*(vec_offset));
  MOVD_read(MM1,downshift);
  if (int_coeff > 0)
    for (register int c=0; c < samples; c+=4)
      {
        MOVQ_xfer(MM2,MM0);         // start with the offset
        PADDW_mem(MM2,*(src+c));   // add 1'st source sample
        PADDW_mem(MM2,*(src+c+1)); // add 2'nd source sample
        MOVQ_read(MM3,*(dst+c));
        PSRAW_reg(MM2,MM1);         // shift right by the `downshift' value
        PSUBW_reg(MM3,MM2);        // subtract from dest sample
        MOVQ_write(*(dst+c),MM3);
      }
  else
    for (register int c=0; c < samples; c+=4)
      {
        MOVQ_xfer(MM2,MM0);         // start with the offset
        PSUBW_mem(MM2,*(src+c));   // subtract 1'st source sample
        PSUBW_mem(MM2,*(src+c+1)); // subtract 2'nd source sample
        MOVQ_read(MM3,*(dst+c));
        PSRAW_reg(MM2,MM1);         // shift right by the `downshift' value
        PSUBW_reg(MM3,MM2);        // subtract from dest sample
        MOVQ_write(*(dst+c),MM3);
      }
  EMMS(); // Clear MMX registers for use by FPU
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_W5X3_v_analysis                             */
/*****************************************************************************/

static inline bool
  simd_W5X3_v_analysis(register kdu_int16 *src1, register kdu_int16 *src2,
                       register kdu_int16 *dst_in, register kdu_int16 *dst_out,
                       register int samples, kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
  if (kdu_mmx_level < 1)
    return false;

  kdu_int16 vec_offset[8];
  vec_offset[0] = (kdu_int16)((1<<step->downshift)>>1);
  for (int k=1; k < 8; k++)
    vec_offset[k] = vec_offset[0];
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));

  // Implementation based on 64-bit operands using only MMX instructions
  MOVQ_read(MM0,*vec_offset);
  MOVD_read(MM1,downshift);
  if (int_coeff > 0)
    for (register int c=0; c < samples; c+=4)
      {
        MOVQ_xfer(MM2,MM0);        // start with the offset
        PADDW_mem(MM2,*(src1+c)); // add 1'st source sample
        PADDW_mem(MM2,*(src2+c)); // add 2'nd source sample
        MOVQ_read(MM3,*(dst_in+c));
        PSRAW_reg(MM2,MM1);        // shift right by the `downshift' value
        PADDW_reg(MM3,MM2);       // add to dest sample
        MOVQ_write(*(dst_out+c),MM3);
      }
  else
    for (register int c=0; c < samples; c+=4)
      {
        MOVQ_xfer(MM2,MM0);        // start with the offset
        PSUBW_mem(MM2,*(src1+c)); // subtract 1'st source sample
        PSUBW_mem(MM2,*(src2+c)); // subtract 2'nd source sample
        MOVQ_read(MM3,*(dst_in+c));
        PSRAW_reg(MM2,MM1);        // shift right by the `downshift' value
        PADDW_reg(MM3,MM2);       // add to dest sample
        MOVQ_write(*(dst_out+c),MM3);
      }
  EMMS(); // Clear MMX registers for use by FPU
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W5X3_v_synth                              */
/*****************************************************************************/

static inline bool
  simd_W5X3_v_synth(register kdu_int16 *src1, register kdu_int16 *src2,
                    register kdu_int16 *dst_in, register kdu_int16 *dst_out,
                    register int samples, kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
  if (kdu_mmx_level < 1)
    return false;
  kdu_int16 vec_offset[8];
  vec_offset[0] = (kdu_int16)((1<<step->downshift)>>1);
  for (int k=1; k < 8; k++)
    vec_offset[k] = vec_offset[0];
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));

#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // Implementation based on 128-bit operands using SSE/SSE2 instructions
      MOVDQU_read(XMM0,*vec_offset);
      MOVD_read(XMM1,downshift);
      if (int_coeff > 0)
        for (register int c=0; c < samples; c+=8)
          {
            MOVDQA_xfer(XMM2,XMM0);     // start with the offset
            PADDW_mem(XMM2,*(src1+c)); // add 1'st source sample
            PADDW_mem(XMM2,*(src2+c)); // add 2'nd source sample
            MOVDQA_read(XMM3,*(dst_in+c));
            PSRAW_reg(XMM2,XMM1);       // shift right by the `downshift' value
            PSUBW_reg(XMM3,XMM2);      // subtract from dest sample
            MOVDQA_write(*(dst_out+c),XMM3);
          }
      else
        for (register int c=0; c < samples; c+=8)
          {
            MOVDQA_xfer(XMM2,XMM0);     // start with the offset
            PSUBW_mem(XMM2,*(src1+c)); // subtract 1'st source sample
            PSUBW_mem(XMM2,*(src2+c)); // subtract 2'nd source sample
            MOVDQA_read(XMM3,*(dst_in+c));
            PSRAW_reg(XMM2,XMM1);       // shift right by the `downshift' value
            PSUBW_reg(XMM3,XMM2);      // subtract from dest sample
            MOVDQA_write(*(dst_out+c),XMM3);
          }
    }
  else
#endif // !KDU_NO_SSE
    { // Implementation based on 64-bit operands using only MMX instructions
      MOVQ_read(MM0,*vec_offset);
      MOVD_read(MM1,downshift);
      if (int_coeff > 0)
        for (register int c=0; c < samples; c+=4)
          {
            MOVQ_xfer(MM2,MM0);         // start with the offset
            PADDW_mem(MM2,*(src1+c));  // add 1'st source sample
            PADDW_mem(MM2,*(src2+c));  // add 2'nd source sample
            MOVQ_read(MM3,*(dst_in+c));
            PSRAW_reg(MM2,MM1);         // shift right by the `downshift' value
            PSUBW_reg(MM3,MM2);        // subtract from dest sample
            MOVQ_write(*(dst_out+c),MM3);
          }
      else
        for (register int c=0; c < samples; c+=4)
          {
            MOVQ_xfer(MM2,MM0);         // start with the offset
            PSUBW_mem(MM2,*(src1+c));  // subtract 1'st source sample
            PSUBW_mem(MM2,*(src2+c));  // subtract 2'nd source sample
            MOVQ_read(MM3,*(dst_in+c));
            PSRAW_reg(MM2,MM1);         // shift rigth by the `downshift' value
            PSUBW_reg(MM3,MM2);        // subtract from dest sample
            MOVQ_write(*(dst_out+c),MM3);
          }
      EMMS(); // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_W9X7_h_analysis                             */
/*****************************************************************************/

static inline bool
  simd_W9X7_h_analysis(register kdu_int16 *src, register kdu_int16 *dst,
                       register int samples, kd_lifting_step *step)
  /* Special function to implement the irreversible 9/7 transform. */
{
  if (kdu_mmx_level < 1)
    return false;
  int step_idx = step->step_idx;
  assert((step_idx >= 0) && (step_idx < 4));
  kdu_int16 vec_lambda[8], vec_preoff[8];
  vec_lambda[0] = simd_w97_rem[step_idx];
  vec_preoff[0] = simd_w97_preoff[step_idx];
  for (int k=1; k < 8; k++)
    {
      vec_lambda[k] = vec_lambda[0];
      vec_preoff[k] = vec_preoff[0];
    }

  // Implementation based on 64-bit operands using only MMX instructions
  MOVQ_read(MM0,*vec_lambda);
  MOVQ_read(MM1,*vec_preoff);
  if (step_idx == 0)
    {
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM2,*(src+c));  // Start with source sample 1
          PADDW_mem(MM2,*(src+c+1)); // Add source sample 2
          MOVQ_read(MM3,*(dst+c));
          PSUBW_reg(MM3,MM2);      // Here is a -1 contribution
          PSUBW_reg(MM3,MM2);      // Here is another -1 contribution
          PADDW_reg(MM2,MM1);      // Add pre-offset for rounding
          PMULHW_reg(MM2,MM0);      // Multiply by lambda and discard 16 LSB's
          PADDW_reg(MM3,MM2);      // Final contribution
          MOVQ_write(*(dst+c),MM3);
        }
    }
  else if (step_idx == 1)
    {
      PXOR_reg(MM6,MM6);    // Set MM6 to 0's
      PCMPEQW_reg(MM7,MM7); // Set MM7 to 1's
      PSUBW_reg(MM6,MM7);   // Leaves each word in MM6 equal to 1
      PSLLW_imm(MM6,2);     // Leave each word in MM6 = 4 (rounding offset)
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM2,*(src+c));  // Get source samples 1 to MM2
          PMULHW_reg(MM2,MM0);      // Multiply by lambda and discard 16 LSB's
          PXOR_reg(MM4,MM4);
          PSUBW_mem(MM4,*(src+c+1)); // Get -ve source samples 2 to MM4
          PMULHW_reg(MM4,MM0);      // Multiply by lambda and discard 16 LSB's
          MOVQ_read(MM3,*(dst+c));
          PSUBW_reg(MM2,MM4);      // Subtract from non-negated scaled source samples
          PADDW_reg(MM2,MM6);      // Add post-offset for rounding
          PSRAW_imm(MM2,3);         // Shift result to the right by 3
          PADDW_reg(MM3,MM2);      // Update destination samples
          MOVQ_write(*(dst+c),MM3);
        }
    }
  else if (step_idx == 2)
    {
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM2,*(src+c));  // Start with source sample 1
          PADDW_mem(MM2,*(src+c+1)); // Add source sample 2
          MOVQ_read(MM3,*(dst+c));
          PADDW_reg(MM3,MM2);      // Here is a +1 contribution
          PADDW_reg(MM2,MM1);      // Add pre-offset for rounding
          PMULHW_reg(MM2,MM0);      // Multiply by lambda and discard 16 LSB's
          PADDW_reg(MM3,MM2);      // Final contribution
          MOVQ_write(*(dst+c),MM3);
        }
    }
  else
    {
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM2,*(src+c));  // Start with source sample 1
          PADDW_mem(MM2,*(src+c+1)); // Add source sample 2
          MOVQ_read(MM3,*(dst+c));
          PADDW_reg(MM2,MM1);      // Add pre-offset for rounding
          PMULHW_reg(MM2,MM0);      // Multiply by lambda and discard 16 LSB's
          PADDW_reg(MM3,MM2);      // Final contribution
          MOVQ_write(*(dst+c),MM3);
        }
    }
  EMMS(); // Clear MMX registers for use by FPU
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W9X7_h_synth                              */
/*****************************************************************************/

static inline bool
  simd_W9X7_h_synth(register kdu_int16 *src, register kdu_int16 *dst,
                    register int samples, kd_lifting_step *step)
  /* Special function to implement the irreversible 9/7 transform. */
{
  if (kdu_mmx_level < 1)
    return false;
  int step_idx = step->step_idx;
  assert((step_idx >= 0) && (step_idx < 4));
  kdu_int16 vec_lambda[8], vec_preoff[8];
  vec_lambda[0] = simd_w97_rem[step_idx];
  vec_preoff[0] = simd_w97_preoff[step_idx];
  for (int k=1; k < 8; k++)
    {
      vec_lambda[k] = vec_lambda[0];
      vec_preoff[k] = vec_preoff[0];
    }

#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // Implementation based on 128-bit operands, using SSE/SSE2 instructions
      register kdu_int16 *src2 = src+1;
      if (_addr_to_kdu_int32(src) & 0x0F)
        { // Swap source pointers so as to ensure that `src' is 16-byte aligned
          src++; src2--;
        }
      MOVDQU_read(XMM0,*vec_lambda);
      MOVDQU_read(XMM1,*vec_preoff);
      if (step_idx == 0)
        {
          MOVDQA_read(XMM2,*src);    // Load initial 8 source 1 samples
          MOVDQU_read(XMM5,*src2);   // Get initial 8 source 2 samples
          for (register int c=0; c < samples; c+=8)
            {
              PADDW_reg(XMM2,XMM5);
              MOVDQA_read(XMM3,*(dst+c));
              PADDW_reg(XMM3,XMM2); // Here is a -1 contribution
              PADDW_reg(XMM3,XMM2); // Here is another -1 contribution
              MOVDQU_read(XMM5,*(src2+c+8)); // Load 8 source 2 samples ahead
              PADDW_reg(XMM2,XMM1); // Add pre-offset for rounding
              PMULHW_reg(XMM2,XMM0); // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(XMM3,XMM2); // Final contribution
              MOVDQA_read(XMM2,*(src+c+8)); // Load next 8 source 1 samples ahead
              MOVDQA_write(*(dst+c),XMM3);
            }
        }
      else if (step_idx == 1)
        {
          PXOR_reg(XMM6,XMM6);       // Set XMM6 to 0's
          PCMPEQW_reg(XMM7,XMM7);    // Set XMM7 to 1's
          PSUBW_reg(XMM6,XMM7);      // Leaves each word in XMM6 equal to 1
          PSLLW_imm(XMM6,2);         // Leave each word in XMM6=4 (round offset)
          MOVDQU_read(XMM5,*src2);   // Get initial 8 source 2 samples
          for (register int c=0; c < samples; c+=8)
            {
              PXOR_reg(XMM2,XMM2);
              MOVDQA_read(XMM4,*(src+c)); // Get 8 source 1 samples to XMM4
              PSUBW_reg(XMM2,XMM5); // Get -ve source 2 samples in XMM2
              MOVDQA_read(XMM3,*(dst+c));
              PMULHW_reg(XMM2,XMM0); // x lambda and discard 16 LSB's
              PMULHW_reg(XMM4,XMM0); // x lambda and discard 16 LSB's
              MOVDQU_read(XMM5,*(src2+c+8)); // Load next 8 source 2 samples ahead
              PSUBW_reg(XMM4,XMM2); // Subtract neg from non-negated product
              PADDW_reg(XMM4,XMM6); // Add post-offset for rounding
              PSRAW_imm(XMM4,3);     // Shift result to the right by 3
              PSUBW_reg(XMM3,XMM4); // Update destination samples
              MOVDQA_write(*(dst+c),XMM3);
            }
        }
      else if (step_idx == 2)
        {
          MOVDQA_read(XMM2,*src);    // Load initial 8 source 1 samples
          MOVDQU_read(XMM5,*src2);   // Load initial 8 source 2 samples
          for (register int c=0; c < samples; c+=8)
            {
              PADDW_reg(XMM2,XMM5);
              MOVDQA_read(XMM3,*(dst+c));
              PSUBW_reg(XMM3,XMM2); // Here is a +1 contribution
              PADDW_reg(XMM2,XMM1); // Add pre-offset for rounding
              MOVDQU_read(XMM5,*(src2+c+8)); // Load next 8 source 2 samples ahead
              PMULHW_reg(XMM2,XMM0); // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(XMM3,XMM2); // Final contribution
              MOVDQA_read(XMM2,*(src+c+8)); // Load next 8 source 1 samples ahead
              MOVDQA_write(*(dst+c),XMM3);
            }
        }
      else
        {
          MOVDQA_read(XMM2,*src);    // Load initial 8 source 1 samples
          MOVDQU_read(XMM5,*src2);   // Load initial 8 source 2 samples
          for (register int c=0; c < samples; c+=8)
            {
              PADDW_reg(XMM2,XMM5);
              MOVDQA_read(XMM3,*(dst+c));
              PADDW_reg(XMM2,XMM1); // Add pre-offset for rounding
              MOVDQU_read(XMM5,*(src2+c+8)); // Load next 8 source 2 samples ahead
              PMULHW_reg(XMM2,XMM0); // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(XMM3,XMM2); // Final contribution
              MOVDQA_read(XMM2,*(src+c+8)); // Load next 8 source 1 samples ahead
              MOVDQA_write(*(dst+c),XMM3);
            }
        }
    }
  else
#endif // !KDU_NO_SSE
    { // Implementation based on 64-bit operands using only MMX instructions
      MOVQ_read(MM0,*vec_lambda);
      MOVQ_read(MM1,*vec_preoff);
      if (step_idx == 0)
        {
          for (register int c=0; c < samples; c+=4)
            {
              MOVQ_read(MM2,*(src+c));  // Start with source sample 1
              PADDW_mem(MM2,*(src+c+1)); // Add source sample 2
              MOVQ_read(MM3,*(dst+c));
              PADDW_reg(MM3,MM2);      // Here is a -1 contribution
              PADDW_reg(MM3,MM2);      // Here is another -1 contribution
              PADDW_reg(MM2,MM1);      // Add pre-offset for rounding
              PMULHW_reg(MM2,MM0);      // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(MM3,MM2);      // Final contribution
              MOVQ_write(*(dst+c),MM3);
            }
        }
      else if (step_idx == 1)
        {
          PXOR_reg(MM6,MM6);    // Set MM6 to 0's
          PCMPEQW_reg(MM7,MM7); // Set MM7 to 1's
          PSUBW_reg(MM6,MM7);   // Leaves each word in MM6 equal to 1
          PSLLW_imm(MM6,2);     // Leave each word in MM6 = 4 (rounding offset)
          for (register int c=0; c < samples; c+=4)
            {
              MOVQ_read(MM2,*(src+c));  // Get source samples 1 to MM2
              PMULHW_reg(MM2,MM0);      // Multiply by lambda and discard 16 LSB's
              PXOR_reg(MM4,MM4);
              PSUBW_mem(MM4,*(src+c+1)); // Get -ve source samples 2 to MM4
              PMULHW_reg(MM4,MM0);      // Multiply by lambda and discard 16 LSB's
              MOVQ_read(MM3,*(dst+c));
              PSUBW_reg(MM2,MM4);      // Subtract from non-negated scaled source samples
              PADDW_reg(MM2,MM6);      // Add post-offset for rounding
              PSRAW_imm(MM2,3);         // Shift result to the right by 3
              PSUBW_reg(MM3,MM2);      // Update destination samples
              MOVQ_write(*(dst+c),MM3);
            }
        }
      else if (step_idx == 2)
        {
          for (register int c=0; c < samples; c+=4)
            {
              MOVQ_read(MM2,*(src+c));  // Start with source sample 1
              PADDW_mem(MM2,*(src+c+1)); // Add source sample 2
              MOVQ_read(MM3,*(dst+c));
              PSUBW_reg(MM3,MM2);      // Here is a +1 contribution
              PADDW_reg(MM2,MM1);      // Add pre-offset for rounding
              PMULHW_reg(MM2,MM0);      // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(MM3,MM2);      // Final contribution
              MOVQ_write(*(dst+c),MM3);
            }
        }
      else
        {
          for (register int c=0; c < samples; c+=4)
            {
              MOVQ_read(MM2,*(src+c));  // Start with source sample 1
              PADDW_mem(MM2,*(src+c+1)); // Add source sample 2
              MOVQ_read(MM3,*(dst+c));
              PADDW_reg(MM2,MM1);      // Add pre-offset for rounding
              PMULHW_reg(MM2,MM0);      // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(MM3,MM2);      // Final contribution
              MOVQ_write(*(dst+c),MM3);
            }
        }
      EMMS(); // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_W9X7_v_analysis                             */
/*****************************************************************************/

static inline bool
  simd_W9X7_v_analysis(register kdu_int16 *src1, register kdu_int16 *src2,
                       register kdu_int16 *dst_in, register kdu_int16 *dst_out,
                       register int samples, kd_lifting_step *step)
  /* Special function to implement the irreversible 9/7 transform */
{
  if (kdu_mmx_level < 1)
    return false;
  int step_idx = step->step_idx;
  assert((step_idx >= 0) && (step_idx < 4));
  kdu_int16 vec_lambda[8], vec_preoff[8];
  vec_lambda[0] = simd_w97_rem[step_idx];
  vec_preoff[0] = simd_w97_preoff[step_idx];
  for (int k=1; k < 8; k++)
    {
      vec_lambda[k] = vec_lambda[0];
      vec_preoff[k] = vec_preoff[0];
    }

  // Implementation based on 64-bit operands, using only MMX instructions
  MOVQ_read(MM0,*vec_lambda);
  MOVQ_read(MM1,*vec_preoff);
  if (step_idx == 0)
    {
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM2,*(src1+c));  // Start with source sample 1
          PADDW_mem(MM2,*(src2+c)); // Add source sample 2
          MOVQ_read(MM3,*(dst_in+c));
          PSUBW_reg(MM3,MM2);       // Here is a -1 contribution
          PSUBW_reg(MM3,MM2);       // Here is another -1 contribution
          PADDW_reg(MM2,MM1);       // Add pre-offset for rounding
          PMULHW_reg(MM2,MM0);       // Multiply by lambda and discard 16 LSB's
          PADDW_reg(MM3,MM2);       // Final contribution
          MOVQ_write(*(dst_out+c),MM3);
        }
    }
  else if (step_idx == 1)
    {
      PXOR_reg(MM6,MM6);       // Set MM6 to 0's
      PCMPEQW_reg(MM7,MM7);    // Set MM7 to 1's
      PSUBW_reg(MM6,MM7);      // Leaves each word in MM6 equal to 1
      PSLLW_imm(MM6,2);        // Leave each word in MM6 = 4 (rounding offset)
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM2,*(src1+c));  // Get source samples 1 to MM2
          PMULHW_reg(MM2,MM0);       // Multiply by lambda and discard 16 LSB's
          PXOR_reg(MM4,MM4);
          PSUBW_mem(MM4,*(src2+c)); // Get -ve source samples 2 to MM4
          PMULHW_reg(MM4,MM0);       // Multiply by lambda and discard 16 LSB's
          MOVQ_read(MM3,*(dst_in+c));
          PSUBW_reg(MM2,MM4);       // Subtract from non-negated scaled source samples
          PADDW_reg(MM2,MM6);       // Add post-offset for rounding
          PSRAW_imm(MM2,3);          // Shift result to the right by 3
          PADDW_reg(MM3,MM2);       // Update destination samples
          MOVQ_write(*(dst_out+c),MM3);
        }
    }
  else if (step_idx == 2)
    {
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM2,*(src1+c));  // Start with source sample 1
          PADDW_mem(MM2,*(src2+c)); // Add source sample 2
          MOVQ_read(MM3,*(dst_in+c));
          PADDW_reg(MM3,MM2);       // Here is a +1 contribution
          PADDW_reg(MM2,MM1);       // Add pre-offset for rounding
          PMULHW_reg(MM2,MM0);       // Multiply by lambda and discard 16 LSB's
          PADDW_reg(MM3,MM2);       // Final contribution
          MOVQ_write(*(dst_out+c),MM3);
        }
    }
  else
    {
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM2,*(src1+c));  // Start with source sample 1
          PADDW_mem(MM2,*(src2+c)); // Add source sample 2
          MOVQ_read(MM3,*(dst_in+c));
          PADDW_reg(MM2,MM1);       // Add pre-offset for rounding
          PMULHW_reg(MM2,MM0);       // Multiply by lambda and discard 16 LSB's
          PADDW_reg(MM3,MM2);       // Final contribution
          MOVQ_write(*(dst_out+c),MM3);
        }
    }
  EMMS(); // Clear MMX registers for use by FPU
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W9X7_v_synth                              */
/*****************************************************************************/

static inline bool
  simd_W9X7_v_synth(register kdu_int16 *src1, register kdu_int16 *src2,
                    register kdu_int16 *dst_in, register kdu_int16 *dst_out,
                    register int samples, kd_lifting_step *step)
  /* Special function to implement the irreversible 9/7 transform */
{
  if (kdu_mmx_level < 1)
    return false;
  int step_idx = step->step_idx;
  assert((step_idx >= 0) && (step_idx < 4));
  kdu_int16 vec_lambda[8], vec_preoff[8];
  vec_lambda[0] = simd_w97_rem[step_idx];
  vec_preoff[0] = simd_w97_preoff[step_idx];
  for (int k=1; k < 8; k++)
    {
      vec_lambda[k] = vec_lambda[0];
      vec_preoff[k] = vec_preoff[0];
    }

#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // Implementation based on 128-bit operands, using SSE/SSE2 instructions
      MOVDQU_read(XMM0,*vec_lambda);
      MOVDQU_read(XMM1,*vec_preoff);
      if (step_idx == 0)
        {
          MOVDQA_read(XMM2,*src1); // Load initial 8 source 1 samples
          PADDW_mem(XMM2,*src2);  // Add in initial 8 source 2 samples
          for (register int c=0; c < samples; c+=8)
            {
              MOVDQA_read(XMM3,*(dst_in+c));
              PADDW_reg(XMM3,XMM2); // Here is a -1 contribution
              PADDW_reg(XMM3,XMM2); // Here is another -1 contribution
              PADDW_reg(XMM2,XMM1); // Add pre-offset for rounding
              PMULHW_reg(XMM2,XMM0); // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(XMM3,XMM2); // Final contribution
              MOVDQA_read(XMM2,*(src1+c+8)); // Load 8 source 1 samples ahead
              PADDW_mem(XMM2,*(src2+c+8));  // Add 8 source 2 samples ahead
              MOVDQA_write(*(dst_out+c),XMM3);
            }
        }
      else if (step_idx == 1)
        {
          PXOR_reg(XMM6,XMM6);    // Set XMM6 to 0's
          PCMPEQW_reg(XMM7,XMM7); // Set XMM7 to 1's
          PSUBW_reg(XMM6,XMM7);   // Leaves each word in MM6 equal to 1
          PSLLW_imm(XMM6,2);      // Leave each word in XMM6 = 4 (rounding offset)
          PXOR_reg(XMM4,XMM4);
          PSUBW_mem(XMM4,*src2); // Load 8 negated source 2 samples
          for (register int c=0; c < samples; c+=8)
            {
              PMULHW_reg(XMM4,XMM0); // -src2 samples times lambda, discarding 16 LSBs
              MOVDQA_read(XMM2,*(src1+c)); // Load 8 source 1 samples
              PMULHW_reg(XMM2,XMM0); // src1 samples times lambda, discarding 16 LSBs
              MOVDQA_read(XMM3,*(dst_in+c));
              PSUBW_reg(XMM2,XMM4); // Subtract neg product from non-negated product
              PXOR_reg(XMM4,XMM4);
              PSUBW_mem(XMM4,*(src2+c+8)); // Load 8 negated source 2 samples ahead
              PADDW_reg(XMM2,XMM6); // Add post-offset for rounding
              PSRAW_imm(XMM2,3);     // Shift result to the right by 3
              PSUBW_reg(XMM3,XMM2); // Update destination samples
              MOVDQA_write(*(dst_out+c),XMM3);
            }
        }
      else if (step_idx == 2)
        {
          MOVDQA_read(XMM2,*src1); // Load initial 8 source 1 samples
          PADDW_mem(XMM2,*src2);  // Add in initial 8 source 2 samples
          for (register int c=0; c < samples; c+=8)
            {
              MOVDQA_read(XMM3,*(dst_in+c));
              PSUBW_reg(XMM3,XMM2); // Here is a +1 contribution
              PADDW_reg(XMM2,XMM1); // Add pre-offset for rounding
              PMULHW_reg(XMM2,XMM0); // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(XMM3,XMM2); // Final contribution
              MOVDQA_read(XMM2,*(src1+c+8)); // Load next 8 source 1 words ahead
              PADDW_mem(XMM2,*(src2+c+8));  // Add in next 8 source 2 words ahead
              MOVDQA_write(*(dst_out+c),XMM3);
            }
        }
      else
        {
          MOVDQA_read(XMM2,*src1); // Load initial 8 source 1 samples
          PADDW_mem(XMM2,*src2);  // Add in initial 8 source 2 samples
          for (register int c=0; c < samples; c+=8)
            {
              MOVDQA_read(XMM3,*(dst_in+c));
              PADDW_reg(XMM2,XMM1); // Add pre-offset for rounding
              PMULHW_reg(XMM2,XMM0); // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(XMM3,XMM2); // Final contribution
              MOVDQA_read(XMM2,*(src1+c+8)); // Load next 8 source 1 words ahead
              PADDW_mem(XMM2,*(src2+c+8));  // Add in next 8 source 2 words ahead
              MOVDQA_write(*(dst_out+c),XMM3);
            }
        }
    }
  else
#endif // !KDU_NO_SSE
    { // Implementation based on 64-bit operands, using only MMX instructions
      MOVQ_read(MM0,*vec_lambda);
      MOVQ_read(MM1,*vec_preoff);
      if (step_idx == 0)
        {
          for (register int c=0; c < samples; c+=4)
            {
              MOVQ_read(MM2,*(src1+c));  // Start with source sample 1
              PADDW_mem(MM2,*(src2+c)); // Add source sample 2
              MOVQ_read(MM3,*(dst_in+c));
              PADDW_reg(MM3,MM2);   // Here is a -1 contribution
              PADDW_reg(MM3,MM2);   // Here is another -1 contribution
              PADDW_reg(MM2,MM1);   // Add pre-offset for rounding
              PMULHW_reg(MM2,MM0);   // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(MM3,MM2);   // Final contribution
              MOVQ_write(*(dst_out+c),MM3);
            }
        }
      else if (step_idx == 1)
        {
          PXOR_reg(MM6,MM6);    // Set MM6 to 0's
          PCMPEQW_reg(MM7,MM7); // Set MM7 to 1's
          PSUBW_reg(MM6,MM7);   // Leaves each word in MM6 equal to 1
          PSLLW_imm(MM6,2);     // Leave each word in MM6 = 4 (rounding offset)
          for (register int c=0; c < samples; c+=4)
            {
              MOVQ_read(MM2,*(src1+c));  // Get source samples 1 to MM2
              PMULHW_reg(MM2,MM0);   // Multiply by lambda and discard 16 LSB's
              PXOR_reg(MM4,MM4);
              PSUBW_mem(MM4,*(src2+c)); // Get -ve source samples 2 to MM4
              PMULHW_reg(MM4,MM0);   // Multiply by lambda and discard 16 LSB's
              MOVQ_read(MM3,*(dst_in+c));
              PSUBW_reg(MM2,MM4);   // Subtract from non-negated scaled source samples
              PADDW_reg(MM2,MM6);   // Add post-offset for rounding
              PSRAW_imm(MM2,3);      // Shift result to the right by 3
              PSUBW_reg(MM3,MM2);   // Update destination samples
              MOVQ_write(*(dst_out+c),MM3);
            }
        }
      else if (step_idx == 2)
        {
          for (register int c=0; c < samples; c+=4)
            {
              MOVQ_read(MM2,*(src1+c));  // Start with source sample 1
              PADDW_mem(MM2,*(src2+c)); // Add source sample 2
              MOVQ_read(MM3,*(dst_in+c));
              PSUBW_reg(MM3,MM2);   // Here is a +1 contribution
              PADDW_reg(MM2,MM1);   // Add pre-offset for rounding
              PMULHW_reg(MM2,MM0);   // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(MM3,MM2);   // Final contribution
              MOVQ_write(*(dst_out+c),MM3);
            }
        }
      else
        {
          for (register int c=0; c < samples; c+=4)
            {
              MOVQ_read(MM2,*(src1+c));  // Start with source sample 1
              PADDW_mem(MM2,*(src2+c)); // Add source sample 2
              MOVQ_read(MM3,*(dst_in+c));
              PADDW_reg(MM2,MM1);   // Add pre-offset for rounding
              PMULHW_reg(MM2,MM0);   // Multiply by lambda and discard 16 LSB's
              PSUBW_reg(MM3,MM2);   // Final contribution
              MOVQ_write(*(dst_out+c),MM3);
            }
        }
      EMMS();  // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                       simd_deinterleave                            */
/*****************************************************************************/

static inline bool
  simd_deinterleave(register kdu_int16 *src, register kdu_int16 *dst1,
                    register kdu_int16 *dst2, register int pairs)
{
  if (kdu_mmx_level < 1)
    return false;
  if (pairs <= 0)
    return true;

  // Implementation based on 64-bit operands, using only MMX instructions
  for (; pairs > 0; pairs-=4, src+=8, dst1+=4, dst2+=4)
    {
      MOVQ_read(MM0,*src);     // Load first source quad
      MOVQ_read(MM1,*(src+4)); // Load second source quad
      MOVQ_xfer(MM2,MM0);      // Make a copy of first source quad
      MOVQ_xfer(MM3,MM1);      // Make a copy of second source quad
      PSLLD_imm(MM0,16);
      PSRAD_imm(MM0,16);       // Leaves sign extended copies of words 0 and 2
      PSLLD_imm(MM1,16);
      PSRAD_imm(MM1,16);       // Leaves sign extended copies of words 4 and 6
      PACKSSDW_reg(MM0,MM1);   // Leaves words 0,2,4,6 in MM0
      MOVQ_write(*dst1,MM0);   // Save even indexed words in destination 1
      PSRAD_imm(MM2,16);       // Leaves sign extended copies of words 1 and 3
      PSRAD_imm(MM3,16);       // Leaves sign extended copies of words 5 and 7
      PACKSSDW_reg(MM2,MM3);   // Leaves words 1,3,5,7 in MM2
      MOVQ_write(*dst2,MM2);   // Save odd indexed words in destination 2
    }
  EMMS(); // Clear MMX registers for use by FPU
  return true;
}

/*****************************************************************************/
/* INLINE                        simd_interleave                             */
/*****************************************************************************/

static inline bool
  simd_interleave(register kdu_int16 *src1, register kdu_int16 *src2,
                  register kdu_int16 *dst, register int pairs)
{
  if (kdu_mmx_level < 1)
    return false;
  if (pairs <= 0)
    return true;
#ifndef KDU_NO_SSE
  if ((kdu_mmx_level >= 2) && (pairs >= 32))
    { // Implementation based on 128-bit operands, using SSE/SSE2 instructions
      if (_addr_to_kdu_int32(src1) & 8)
        { // Source addresses are 8-byte aligned, but not 16-byte aligned
          MOVDQA_read(XMM0,*(src1-4));
          MOVDQA_read(XMM1,*(src2-4));
          PUNPCKHWD_reg(XMM0,XMM1);
          MOVDQA_write(*dst,XMM0);
          src1 += 4; src2 += 4; dst += 8; pairs -= 4;
        }
      for (; pairs > 4; pairs-=8, src1+=8, src2+=8, dst+=16)
        {
          MOVDQA_read(XMM0,*src1);
          MOVDQA_xfer(XMM2,XMM0);
          MOVDQA_read(XMM1,*src2);
          PUNPCKLWD_reg(XMM2,XMM1);
          MOVDQA_write(*dst,XMM2);
          PUNPCKHWD_reg(XMM0,XMM1);
          MOVDQA_write(*(dst+8),XMM0);
        }
      if (pairs > 0)
        { // Need to generate one more group of 8 outputs (4 pairs)
          MOVDQA_read(XMM0,*src1);
          MOVDQA_xfer(XMM2,XMM0);
          MOVDQA_read(XMM1,*src2);
          PUNPCKLWD_reg(XMM2,XMM1);
          MOVDQA_write(*dst,XMM2);
        }
    }
  else
#endif // !KDU_NO_SSE
    { // Implementation based on 64-bit operands, using MMX instructions only
      for (; pairs > 0; pairs-=4, src1+=4, src2+=4, dst+=8)
        {
          MOVQ_read(MM0,*src1);
          MOVQ_xfer(MM2,MM0);
          MOVQ_read(MM1,*src2);
          PUNPCKLWD_reg(MM2,MM1);
          MOVQ_write(*dst,MM2);
          PUNPCKHWD_reg(MM0,MM1);
          MOVQ_write(*(dst+4),MM0);
        }
      EMMS(); // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                  simd_downshifted_deinterleave                     */
/*****************************************************************************/

static inline bool
  simd_downshifted_deinterleave(register kdu_int16 *src,
                                register kdu_int16 *dst1,
                                register kdu_int16 *dst2,
                                register int pairs, int downshift)
{
  if (kdu_mmx_level < 1)
    return false;
  if (pairs <= 0)
    return true;

  kdu_int16 vec_offset[8];
  vec_offset[0] = (kdu_int16)((1<<downshift)>>1);
  for (int k=1; k < 8; k++)
    vec_offset[k] = vec_offset[0];

  // Implementation based on 64-bit operands, using only MMX instructions
  MOVD_read(MM4,downshift);
  MOVQ_read(MM5,*vec_offset);
  for (; pairs > 0; pairs-=4, src+=8, dst1+=4, dst2+=4)
    {
      MOVQ_read(MM0,*src);     // Load first source quad
      MOVQ_read(MM1,*(src+4)); // Load second source quad
      PADDW_reg(MM0,MM5);     // Add the offset to first quad
      PSRAW_reg(MM0,MM4);      // Apply downshift to first quad
      PADDW_reg(MM1,MM5);     // Add the offset to second quad
      PSRAW_reg(MM1,MM4);      // Apply the downshift to second quad
      MOVQ_xfer(MM2,MM0);      // Make a copy of first shifted quad
      MOVQ_xfer(MM3,MM1);      // Make a copy of second shifted quad
      PSLLD_imm(MM0,16);
      PSRAD_imm(MM0,16);       // Leaves sign extended copies of words 0 and 2
      PSLLD_imm(MM1,16);
      PSRAD_imm(MM1,16);       // Leaves sign extended copies of words 4 and 6
      PACKSSDW_reg(MM0,MM1);   // Leaves words 0,2,4,6 in MM0
      MOVQ_write(*dst1,MM0);   // Save even indexed words in destination 1
      PSRAD_imm(MM2,16);       // Leaves sign extended copies of words 1 and 3
      PSRAD_imm(MM3,16);       // Leaves sign extended copies of words 5 and 7
      PACKSSDW_reg(MM2,MM3);   // Leaves words 1,3,5,7 in MM2
      MOVQ_write(*dst2,MM2);   // Save odd indexed words in destination 2
    }
  EMMS(); // Clear MMX registers for use by FPU
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_upshifted_interleave                        */
/*****************************************************************************/

static inline bool
  simd_upshifted_interleave(register kdu_int16 *src1,
                            register kdu_int16 *src2,
                            register kdu_int16 *dst,
                            register int pairs, int upshift)
{
  if (kdu_mmx_level < 1)
    return false;
  if (pairs <= 0)
    return true;
#ifndef KDU_NO_SSE
  if ((kdu_mmx_level >= 2) && (pairs >= 32))
    { // Implementation based on 128-bit operands, using SSE/SSE2 instructions
      MOVD_read(XMM3,upshift);
      if (_addr_to_kdu_int32(src1) & 8)
        { // source addresses are 8-byte aligned, but not 16-byte aligned
          MOVDQA_read(XMM0,*(src1-4));
          PSLLW_reg(XMM0,XMM3);
          MOVDQA_read(XMM1,*(src2-4));
          PSLLW_reg(XMM1,XMM3);
          PUNPCKHWD_reg(XMM0,XMM1);
          MOVDQA_write(*dst,XMM0);
          src1+=4; src2+=4; dst+=8; pairs-=4;
        }
      for (; pairs > 4; pairs-=8, src1+=8, src2+=8, dst+=16)
        {
          MOVDQA_read(XMM0,*src1);
          PSLLW_reg(XMM0,XMM3);
          MOVDQA_xfer(XMM2,XMM0);
          MOVDQA_read(XMM1,*src2);
          PSLLW_reg(XMM1,XMM3);
          PUNPCKLWD_reg(XMM2,XMM1);
          MOVDQA_write(*dst,XMM2);
          PUNPCKHWD_reg(XMM0,XMM1);
          MOVDQA_write(*(dst+8),XMM0);
        }
      if (pairs > 0)
        { // Need to generate one more group of 8 outputs (4 pairs)
          MOVDQA_read(XMM0,*src1);
          PSLLW_reg(XMM0,XMM3);
          MOVDQA_xfer(XMM2,XMM0);
          MOVDQA_read(XMM1,*src2);
          PSLLW_reg(XMM1,XMM3);
          PUNPCKLWD_reg(XMM2,XMM1);
          MOVDQA_write(*dst,XMM2);
        }
    }
  else
#endif // !KDU_NO_SSE
    { // Implementation based on 64-bit operands, using only MMX instructions
      MOVD_read(MM3,upshift);
      for (; pairs > 0; pairs-=4, src1+=4, src2+=4, dst+=8)
        {
          MOVQ_read(MM0,*src1);
          PSLLW_reg(MM0,MM3);
          MOVQ_xfer(MM2,MM0);
          MOVQ_read(MM1,*src2);
          PSLLW_reg(MM1,MM3);
          PUNPCKLWD_reg(MM2,MM1);
          MOVQ_write(*dst,MM2);
          PUNPCKHWD_reg(MM0,MM1);
          MOVQ_write(*(dst+4),MM0);
        }
      EMMS(); // Clear MMX registers for use by FPU
    }
  return true;
}

/* ========================================================================= */
/*                  MMX/SSE functions for 32-bit Samples                     */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                     simd_2tap_h_irrev32                            */
/*****************************************************************************/

static inline bool
  simd_2tap_h_irrev32(float *src, float *dst, int samples,
                      kd_lifting_step *step, bool synthesis)
{
  return false;
}

/*****************************************************************************/
/* INLINE                     simd_4tap_h_irrev32                            */
/*****************************************************************************/

static inline bool
  simd_4tap_h_irrev32(float *src, float *dst, int samples,
                      kd_lifting_step *step, bool synthesis)
{
  return false;
}

/*****************************************************************************/
/* INLINE                     simd_2tap_v_irrev32                            */
/*****************************************************************************/

static inline bool
  simd_2tap_v_irrev32(float *src0, float *src1, float *dst_in, float *dst_out,
                      int samples, kd_lifting_step *step, bool synthesis)
{
  return false;
}

/*****************************************************************************/
/* INLINE                     simd_4tap_v_irrev32                            */
/*****************************************************************************/

static inline bool
  simd_4tap_v_irrev32(float *src0, float *src1, float *src2, float *src3,
                      float *dst_in, float *dst_out,
                      int samples, kd_lifting_step *step, bool synthesis)
{
  return false;
}

/*****************************************************************************/
/* INLINE                   simd_W5X3_h_analysis32                           */
/*****************************************************************************/

static inline bool
  simd_W5X3_h_analysis32(kdu_int32 *src, kdu_int32 *dst, int samples,
                         kd_lifting_step *step)
{
  return false;
}

/*****************************************************************************/
/* INLINE                    simd_W5X3_h_synth32                             */
/*****************************************************************************/

static inline bool
  simd_W5X3_h_synth32(kdu_int32 *src, kdu_int32 *dst, int samples,
                      kd_lifting_step *step)
{
  return false;
}

/*****************************************************************************/
/* INLINE                   simd_W5X3_v_analysis32                           */
/*****************************************************************************/

static inline bool
  simd_W5X3_v_analysis32(kdu_int32 *src1, kdu_int32 *src2,
                         kdu_int32 *dst_in, kdu_int32 *dst_out, int samples,
                         kd_lifting_step *step)
{
  return false;
}

/*****************************************************************************/
/* INLINE                    simd_W5X3_v_synth32                             */
/*****************************************************************************/

static inline bool
  simd_W5X3_v_synth32(kdu_int32 *src1, kdu_int32 *src2,
                      kdu_int32 *dst_in, kdu_int32 *dst_out, int samples,
                      kd_lifting_step *step)
{
  return false;
}

/*****************************************************************************/
/* INLINE                        simd_interleave                             */
/*****************************************************************************/

static inline bool
  simd_interleave(kdu_int32 *src1, kdu_int32 *src2, kdu_int32 *dst, int pairs)
{
  return false;
}

#endif // GCC_DWT_MMX_LOCAL_H
