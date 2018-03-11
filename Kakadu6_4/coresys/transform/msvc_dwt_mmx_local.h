/*****************************************************************************/
// File: msvc_dwt_mmx_local.h [scope = CORESYS/TRANSFORMS]
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
in-line MMX assembler code.
******************************************************************************/

#ifndef MSVC_DWT_MMX_LOCAL_H
#define MSVC_DWT_MMX_LOCAL_H

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
/*                  MMX/SSE functions for 16-bit samples                     */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                     simd_2tap_h_synth                              */
/*****************************************************************************/

static inline bool
  simd_2tap_h_synth(kdu_int16 *src, kdu_int16 *dst, int samples,
                    kd_lifting_step *step)
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
  if (samples <= 0)
    return true;

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

  __asm {
      XOR ECX,ECX     // Zero sample counter
      MOV EDX,samples // Limit for ECX
      MOV EAX,src
      MOV EDI,dst
      MOVDQU XMM0,vec_lambda
      MOVDQU XMM1,vec_offset
      MOVD XMM2,downshift
      PCMPEQW XMM7,XMM7          // Set XMM7 to 1's
      PSRLD XMM7,16              // Leaves mask for low words in each dword
      MOVDQU XMM3,[EAX]
      MOVDQU XMM4,[EAX+2]
loop_src_aligned:
      PMADDWD XMM3,XMM0
      PMADDWD XMM4,XMM0
      MOVDQA XMM6,[EDI+2*ECX]
      PADDD XMM3,XMM1
      PADDD XMM4,XMM1
      PSRAD XMM3,XMM2
      PSRAD XMM4,XMM2
      PAND XMM3,XMM7   // Zero out high words of XMM3
      PSLLD XMM4,16    // Move XMM4 results into high words
      PSUBW XMM6,XMM3
      MOVDQU XMM3,[EAX+2*ECX+16]
      PSUBW XMM6,XMM4
      MOVDQU XMM4,[EAX+2*ECX+18]
      MOVDQA [EDI+2*ECX],XMM6
      ADD ECX,8
      CMP ECX,EDX
      JL loop_src_aligned
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_4tap_h_synth                              */
/*****************************************************************************/

static inline bool
  simd_4tap_h_synth(kdu_int16 *src, kdu_int16 *dst, int samples,
                    kd_lifting_step *step)
  /* Efficient general purpose horizontal lifting step for lifting steps
     with 3 or 4 taps.  If the lifting step has only three input tap, the
     implementation costs as much as if it had 4 taps. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length >= 3) || (step->support_length <= 4));
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

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

  __asm {
      XOR ECX,ECX     // Zero sample counter
      MOV EDX,samples // Limit for ECX
      MOV EAX,src
      MOV EDI,dst
      MOVDQU XMM0,vec_lambda1
      MOVDQU XMM1,vec_lambda2
      MOVDQU XMM2,vec_offset
      MOVD XMM3,downshift
      PCMPEQW XMM7,XMM7          // Set XMM7 to 1's
      PSRLD XMM7,16              // Leaves mask for low words in each dword
loop_src_aligned:
      MOVDQU XMM4,[EAX+2*ECX]
      MOVDQU XMM5,[EAX+2*ECX+4]
      PMADDWD XMM4,XMM0  // Multiply by first two lifting step coefficients
      PMADDWD XMM5,XMM1  // Multiply by second two lifting step coefficients
      MOVDQU XMM6,[EAX+2*ECX+2]
      PADDD XMM4,XMM5    // Add products
      PADDD XMM4,XMM2    // Add rounding offset
      MOVDQU XMM5,[EAX+2*ECX+6]
      PSRAD XMM4,XMM3    // Downshift to get results in low-order words
      PMADDWD XMM6,XMM0  // Multiply by first two lifting step coefficients
      PMADDWD XMM5,XMM1  // Multiply by second two lifting step coefficients
      PADDD XMM6,XMM5    // Add products
      MOVDQA XMM5,[EDI+2*ECX]
      PADDD XMM6,XMM2    // Add rounding offset
      PSRAD XMM6,XMM3    // Downshift to get results in low-order words
      PAND XMM4,XMM7   // Zero out high words of XMM4
      PSLLD XMM6,16    // Move XMM6 results into high words
      PSUBW XMM5,XMM4
      PSUBW XMM5,XMM6
      MOVDQA [EDI+2*ECX],XMM5
      ADD ECX,8
      CMP ECX,EDX
      JL loop_src_aligned
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_2tap_v_synth                              */
/*****************************************************************************/

static inline bool
  simd_2tap_v_synth(kdu_int16 *src1, kdu_int16 *src2,
                    kdu_int16 *dst_in, kdu_int16 *dst_out, int samples,
                    kd_lifting_step *step)
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
  if (samples <= 0)
    return true;

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

  __asm {
      XOR ECX,ECX     // Zero sample counter
      MOV EDX,samples // Limit for ECX
      MOV ESI,dst_in
      MOV EDI,dst_out
      MOVDQU XMM0,vec_lambda
      MOVDQU XMM1,vec_offset
      MOVD XMM2,downshift
      MOV EAX,src1
      MOV EBX,src2 // Load last (.NET 2008 uses EBX for debug stack frame)
      MOVDQA XMM3,[EAX]
      MOVDQA XMM4,[EBX]
      MOVDQA XMM5,XMM3
loop_2tap_synth128:
      MOVDQA XMM6,[ESI+2*ECX]
      PUNPCKHWD XMM3,XMM4
      PUNPCKLWD XMM5,XMM4
      PMADDWD XMM3,XMM0
      PMADDWD XMM5,XMM0
      PADDD XMM3,XMM1
      PADDD XMM5,XMM1
      PSRAD XMM3,XMM2
      PSRAD XMM5,XMM2
      MOVDQA XMM4,[EBX+2*ECX+16] // Read ahead next 8 source 1 words
      PACKSSDW XMM5,XMM3
      MOVDQA XMM3,[EAX+2*ECX+16] // Read ahead next 8 source 2 words
      PSUBW XMM6,XMM5
      MOVDQA [EDI+2*ECX],XMM6
      MOVDQA XMM5,XMM3
      ADD ECX,8
      CMP ECX,EDX
      JL loop_2tap_synth128
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_4tap_v_synth                              */
/*****************************************************************************/

static inline bool
  simd_4tap_v_synth(kdu_int16 *src1, kdu_int16 *src2,
                    kdu_int16 *src3, kdu_int16 *src4,
                    kdu_int16 *dst_in, kdu_int16 *dst_out, int samples,
                    kd_lifting_step *step)
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length >= 3) || (step->support_length <= 4));
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

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

  __asm {
      XOR ECX,ECX     // Zero sample counter
      MOV ESI,dst_in
      MOV EDI,dst_out
      MOVDQU XMM0,vec_lambda1
      MOVDQU XMM1,vec_lambda2
      MOVDQU XMM2,vec_offset
      MOV EAX,src1
      MOV EBX,src2 // Load last (.NET 2008 uses EBX for debug stack frame)
loop_4tap_synth128:
      MOVDQA XMM3,[EAX+2*ECX]
      MOVDQA XMM4,[EBX+2*ECX]
      MOVDQA XMM5,XMM3
      PUNPCKHWD XMM3,XMM4
      PUNPCKLWD XMM5,XMM4
      mov EDX,src3
      PMADDWD XMM3,XMM0
      MOVDQA XMM6,[EDX+2*ECX]
      PMADDWD XMM5,XMM0

      mov EDX,src4
      MOVDQA XMM7,[EDX+2*ECX]
      MOVDQA XMM4,XMM6
      PUNPCKHWD XMM6,XMM7
      PUNPCKLWD XMM4,XMM7
      PMADDWD XMM6,XMM1
      PMADDWD XMM4,XMM1
      MOVD XMM7,downshift
      PADDD XMM3,XMM6  // Form first sum of products
      PADDD XMM5,XMM4  // Form second sum of products

      MOVDQA XMM6,[ESI+2*ECX]
      PADDD XMM3,XMM2  // Add offset
      PADDD XMM5,XMM2  // Add offset
      PSRAD XMM3,XMM7  // Downshift
      PSRAD XMM5,XMM7  // Downshift

      PACKSSDW XMM5,XMM3
      PSUBW XMM6,XMM5
      MOVDQA [EDI+2*ECX],XMM6
      ADD ECX,8
      CMP ECX,samples
      JL loop_4tap_synth128
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                    simd_W5X3_h_analysis                            */
/*****************************************************************************/

static inline bool
  simd_W5X3_h_analysis(kdu_int16 *src, kdu_int16 *dst, int samples,
                       kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
  if (kdu_mmx_level < 1)
    return false;
  if (samples <= 0)
    return true;

  kdu_int16 vec_offset[4];
  vec_offset[0] = vec_offset[1] = vec_offset[2] =
    vec_offset[3] = (kdu_int16)((1<<step->downshift)>>1);
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));

  // Implementation based on 64-bit operands using only MMX instructions
  __asm {
        XOR ECX,ECX     // Zero out sample counter
        MOV EDX,samples // Limit for sample counter
        MOV EAX,src
        MOV EDI,dst
        MOVQ MM0,vec_offset
        MOVD MM1,downshift
        MOV EBX,int_coeff // Load last (.NET 2008 uses EBX for dbg stack frame)
        CMP EBX,1
        JNE loop_minus1_analysis64
loop_plus1_analysis64:
          MOVQ MM2,MM0               // start with the offset
          PADDW MM2,[EAX+2*ECX]     // add 1'st source sample
          PADDW MM2,[EAX+2*ECX+2]   // add 2'nd source sample
          MOVQ MM3,[EDI+2*ECX]
          PSRAW MM2,MM1              // shift rigth by the `downshift' value
          PADDW MM3,MM2             // add to dest sample
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_plus1_analysis64
        JMP end_analysis64
loop_minus1_analysis64:
          MOVQ MM2,MM0               // start with the offset
          PSUBW MM2,[EAX+2*ECX]     // subtract 1'st source sample
          PSUBW MM2,[EAX+2*ECX+2]   // subtract 2'nd source sample
          MOVQ MM3,[EDI+2*ECX]
          PSRAW MM2,MM1              // shift rigth by the `downshift' value
          PADDW MM3,MM2             // add to dest sample
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_minus1_analysis64
end_analysis64:
        EMMS // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W5X3_h_synth                              */
/*****************************************************************************/

static inline bool
  simd_W5X3_h_synth(kdu_int16 *src, kdu_int16 *dst, int samples,
                    kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
  if (kdu_mmx_level < 1)
    return false;
  if (samples <= 0)
    return true;

  kdu_int16 vec_offset[4];
  vec_offset[0] = vec_offset[1] = vec_offset[2] =
    vec_offset[3] = (kdu_int16)((1<<step->downshift)>>1);
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));

  // Implementation based on 64-bit operands using only MMX instructions
  __asm {
        XOR ECX,ECX     // Zero out sample counter
        MOV EDX,samples // Limit for sample counter
        MOV EAX,src
        MOV EDI,dst
        MOVQ MM0,vec_offset
        MOVD MM1,downshift
        MOV EBX,int_coeff // Load last (.NET 2008 uses EBX for dbg stack frame)
        CMP EBX,1
        JNE loop_minus1_synth64
loop_plus1_synth64:
          MOVQ MM2,MM0               // start with the offset
          PADDW MM2,[EAX+2*ECX]     // add 1'st source sample
          PADDW MM2,[EAX+2*ECX+2]   // add 2'nd source sample
          MOVQ MM3,[EDI+2*ECX]
          PSRAW MM2,MM1              // shift rigth by the `downshift' value
          PSUBW MM3,MM2             // subtract from dest sample
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_plus1_synth64
        JMP end_synth64
loop_minus1_synth64:
          MOVQ MM2,MM0               // start with the offset
          PSUBW MM2,[EAX+2*ECX]     // subtract 1'st source sample
          PSUBW MM2,[EAX+2*ECX+2]   // subtract 2'nd source sample
          MOVQ MM3,[EDI+2*ECX]
          PSRAW MM2,MM1              // shift rigth by the `downshift' value
          PSUBW MM3,MM2             // subtract from dest sample
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_minus1_synth64
end_synth64:
        EMMS // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_W5X3_v_analysis                             */
/*****************************************************************************/

static inline bool
  simd_W5X3_v_analysis(kdu_int16 *src1, kdu_int16 *src2,
                       kdu_int16 *dst_in, kdu_int16 *dst_out, int samples,
                       kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
  if (kdu_mmx_level < 1)
    return false;
  if (samples <= 0)
    return true;

  kdu_int16 vec_offset[8];
  vec_offset[0] = (kdu_int16)((1<<step->downshift)>>1);
  for (int k=1; k < 8; k++)
    vec_offset[k] = vec_offset[0];
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));

  // Implementation based on 64-bit operands using only MMX instructions
  __asm {
        MOV EDX,samples // Limit for ECX
        MOV ESI,dst_in
        MOV EDI,dst_out
        MOVQ MM0,vec_offset
        MOVD MM1,downshift
        MOV ECX,int_coeff
        MOV EAX,src1
        MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
        CMP ECX,1
        MOV ECX,0  // Set up sample counter, without altering status flags
        JNE loop_minus1_analysis64
loop_plus1_analysis64:
          MOVQ MM2,MM0             // start with the offset
          PADDW MM2,[EAX+2*ECX]   // add 1'st source sample
          PADDW MM2,[EBX+2*ECX]   // add 2'nd source sample
          MOVQ MM3,[ESI+2*ECX]
          PSRAW MM2,MM1            // shift rigth by the `downshift' value
          PADDW MM3,MM2           // add to dest sample
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_plus1_analysis64
        JMP end_analysis64
loop_minus1_analysis64:
          MOVQ MM2,MM0             // start with the offset
          PSUBW MM2,[EAX+2*ECX]   // subtract 1'st source sample
          PSUBW MM2,[EBX+2*ECX]   // subtract 2'nd source sample
          MOVQ MM3,[ESI+2*ECX]
          PSRAW MM2,MM1            // shift rigth by the `downshift' value
          PADDW MM3,MM2           // add to dest sample
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_minus1_analysis64
end_analysis64:
        EMMS // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W5X3_v_synth                              */
/*****************************************************************************/

static inline bool
  simd_W5X3_v_synth(kdu_int16 *src1, kdu_int16 *src2,
                    kdu_int16 *dst_in, kdu_int16 *dst_out, int samples,
                    kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
  if (kdu_mmx_level < 1)
    return false;
  if (samples <= 0)
    return true;

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
      __asm {
        MOV EDX,samples // Limit for ECX
        MOV ESI,dst_in
        MOV EDI,dst_out
        MOVDQU XMM0,vec_offset
        MOVD XMM1,downshift
        MOV ECX,int_coeff
        MOV EAX,src1
        MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
        CMP ECX,1
        MOV ECX,0  // Set up sample counter, without altering status flags
        JNE loop_minus1_synth128
loop_plus1_synth128:
          MOVDQA XMM2,XMM0          // start with the offset
          PADDW XMM2,[EAX+2*ECX]   // add 1'st source sample
          PADDW XMM2,[EBX+2*ECX]   // add 2'nd source sample
          MOVDQA XMM3,[ESI+2*ECX]
          PSRAW XMM2,XMM1           // shift rigth by the `downshift' value
          PSUBW XMM3,XMM2          // subtract from dest sample
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_plus1_synth128
        JMP end_synth128
loop_minus1_synth128:
          MOVDQA XMM2,XMM0          // start with the offset
          PSUBW XMM2,[EAX+2*ECX]   // subtract 1'st source sample
          PSUBW XMM2,[EBX+2*ECX]   // subtract 2'nd source sample
          MOVDQA XMM3,[ESI+2*ECX]
          PSRAW XMM2,XMM1           // shift rigth by the `downshift' value
          PSUBW XMM3,XMM2          // subtract from dest sample
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_minus1_synth128
end_synth128:
      }
    }
  else
#endif // !KDU_NO_SSE
    { // Implementation based on 64-bit operands using only MMX instructions
      __asm {
        MOV EDX,samples // Limit for ECX
        MOV ESI,dst_in
        MOV EDI,dst_out
        MOVQ MM0,vec_offset
        MOVD MM1,downshift
        MOV ECX,int_coeff
        MOV EAX,src1
        MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
        CMP ECX,1
        MOV ECX,0  // Set up sample counter, without altering status flags
        JNE loop_minus1_synth64
loop_plus1_synth64:
          MOVQ MM2,MM0             // start with the offset
          PADDW MM2,[EAX+2*ECX]   // add 1'st source sample
          PADDW MM2,[EBX+2*ECX]   // add 2'nd source sample
          MOVQ MM3,[ESI+2*ECX]
          PSRAW MM2,MM1            // shift rigth by the `downshift' value
          PSUBW MM3,MM2           // subtract from dest sample
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_plus1_synth64
        JMP end_synth64
loop_minus1_synth64:
          MOVQ MM2,MM0             // start with the offset
          PSUBW MM2,[EAX+2*ECX]   // subtract 1'st source sample
          PSUBW MM2,[EBX+2*ECX]   // subtract 2'nd source sample
          MOVQ MM3,[ESI+2*ECX]
          PSRAW MM2,MM1            // shift rigth by the `downshift' value
          PSUBW MM3,MM2           // subtract from dest sample
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_minus1_synth64
end_synth64:
        EMMS // Clear MMX registers for use by FPU
      }
    }
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_W9X7_h_analysis                             */
/*****************************************************************************/

static inline bool
  simd_W9X7_h_analysis(kdu_int16 *src, kdu_int16 *dst,
                       int samples, kd_lifting_step *step)
  /* Special function to implement the irreversible 9/7 transform. */
{
  if (kdu_mmx_level < 1)
    return false;
  if (samples <= 0)
    return true;
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
  __asm {         
          MOV EDX,samples // Set limit for sample counter
          MOV EAX,src
          MOV EDI,dst
          MOVQ MM0,vec_lambda
          MOVQ MM1,vec_preoff
          MOV ECX,step_idx
          CMP ECX,3
          JZ start_step3_analysis64
          CMP ECX,2
          JZ start_step2_analysis64
          CMP ECX,1
          JZ start_step1_analysis64

          XOR ECX,ECX   // Not really necessary
loop_step0_analysis64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EAX+2*ECX+2]   // Add source sample 2
          MOVQ MM3,[EDI+2*ECX]
          PSUBW MM3,MM2     // Here is a -1 contribution
          PSUBW MM3,MM2     // Here is another -1 contribution
          PADDW MM2,MM1     // Add pre-offset for rounding
          PMULHW MM2,MM0     // Multiply by lambda and discard 16 LSB's
          PADDW MM3,MM2     // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step0_analysis64
          JMP end_analysis64

start_step1_analysis64:
          XOR ECX,ECX     // Zero sample counter
          PXOR MM6,MM6    // Set MM6 to 0's
          PCMPEQW MM7,MM7 // Set MM7 to 1's
          PSUBW MM6,MM7   // Leaves each word in MM6 equal to 1
          PSLLW MM6,2     // Leave each word in MM6 = 4 (rounding offset)
loop_step1_analysis64:
          MOVQ MM2,[EAX+2*ECX]   // Get source samples 1 to MM2
          PMULHW MM2,MM0         // Multiply by lambda and discard 16 LSB's
          PXOR MM4,MM4
          PSUBW MM4,[EAX+2*ECX+2] // Get -ve source samples 2 to MM4
          PMULHW MM4,MM0     // Multiply by lambda and discard 16 LSB's
          MOVQ MM3,[EDI+2*ECX]
          PSUBW MM2,MM4     // Subtract from non-negated scaled source samples
          PADDW MM2,MM6     // Add post-offset for rounding
          PSRAW MM2,3        // Shift result to the right by 3
          PADDW MM3,MM2     // Update destination samples
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step1_analysis64
          JMP end_analysis64

start_step2_analysis64:
          XOR ECX,ECX     // Zero sample counter
loop_step2_analysis64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EAX+2*ECX+2]   // Add source sample 2
          MOVQ MM3,[EDI+2*ECX]
          PADDW MM3,MM2     // Here is a +1 contribution
          PADDW MM2,MM1     // Add pre-offset for rounding
          PMULHW MM2,MM0     // Multiply by lambda and discard 16 LSB's
          PADDW MM3,MM2     // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step2_analysis64
          JMP end_analysis64

start_step3_analysis64:
          XOR ECX,ECX     // Zero sample counter
loop_step3_analysis64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EAX+2*ECX+2] // Add source sample 2
          MOVQ MM3,[EDI+2*ECX]
          PADDW MM2,MM1           // Add pre-offset for rounding
          PMULHW MM2,MM0           // Multiply by lambda and discard 16 LSB's
          PADDW MM3,MM2           // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step3_analysis64

end_analysis64:
          EMMS               // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W9X7_h_synth                              */
/*****************************************************************************/

static inline bool
  simd_W9X7_h_synth(kdu_int16 *src, kdu_int16 *dst,
                    int samples, kd_lifting_step *step)
  /* Special function to implement the irreversible 9/7 transform. */
{
  if (kdu_mmx_level < 1)
    return false;
  if (samples <= 0)
    return true;
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
      __asm
        {         
          MOV EDI,dst
          MOVDQU XMM0,vec_lambda
          MOVDQU XMM1,vec_preoff
          MOV ECX,step_idx
          MOV EDX,samples // Set limit for sample counter
          MOV EAX,src     // EAX points to first source sample
          MOV EBX,EAX
          ADD EBX,2       // EBX points to second source sample
          TEST EAX,0Fh    // See if EAX is 16-byte aligned
          JZ eax_aligned
          XCHG EAX,EBX    // Make sure EAX is the aligned address
eax_aligned:
          CMP ECX,3
          JZ start_step3_synth128
          CMP ECX,2
          JZ start_step2_synth128
          CMP ECX,1
          JZ start_step1_synth128

          // Invert Lifting Step 0
          XOR ECX,ECX                // Not really necessary
          MOVDQA XMM2,[EAX]          // Load initial 8 source 1 samples
          MOVDQU XMM5,[EBX]          // Get initial 8 source 2 samples
loop_step0_synth128:
          PADDW XMM2,XMM5
          MOVDQA XMM3,[EDI+2*ECX]
          PADDW XMM3,XMM2           // Here is a -1 contribution
          PADDW XMM3,XMM2           // Here is another -1 contribution
          MOVDQU XMM5,[EBX+2*ECX+16] // Load 8 source 2 samples ahead
          PADDW XMM2,XMM1           // Add pre-offset for rounding
          PMULHW XMM2,XMM0           // Multiply by lambda and discard 16 LSB's
          PSUBW XMM3,XMM2           // Final contribution
          MOVDQA XMM2,[EAX+2*ECX+16] // Load next 8 source 1 samples ahead
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_step0_synth128
          JMP end_synth128

start_step1_synth128:
          // Invert Lifting Step 1
          XOR ECX,ECX                // Zero sample counter
          PXOR XMM6,XMM6             // Set XMM6 to 0's
          PCMPEQW XMM7,XMM7          // Set XMM7 to 1's
          PSUBW XMM6,XMM7            // Leaves each word in XMM6 equal to 1
          PSLLW XMM6,2               // Leave each word in XMM6=4 (round offset)
          MOVDQU XMM5,[EBX]          // Get initial 8 source 2 samples
loop_step1_synth128:
          PXOR XMM2,XMM2
          MOVDQA XMM4,[EAX+2*ECX]    // Get 8 source 1 samples to XMM4
          PSUBW XMM2,XMM5           // Get -ve source 2 samples in XMM2
          MOVDQA XMM3,[EDI+2*ECX]
          PMULHW XMM2,XMM0           // x lambda and discard 16 LSB's
          PMULHW XMM4,XMM0           // x lambda and discard 16 LSB's
          MOVDQU XMM5,[EBX+2*ECX+16] // Load next 8 source 2 samples ahead
          PSUBW XMM4,XMM2           // Subtract neg from non-negated product
          PADDW XMM4,XMM6           // Add post-offset for rounding
          PSRAW XMM4,3               // Shift result to the right by 3
          PSUBW XMM3,XMM4           // Update destination samples
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_step1_synth128
          JMP end_synth128

start_step2_synth128:
          // Invert Lifting Step 2
          XOR ECX,ECX                // Zero sample counter
          MOVDQA XMM2,[EAX]          // Load initial 8 source 1 samples
          MOVDQU XMM5,[EBX]          // Load initial 8 source 2 samples
loop_step2_synth128:
          PADDW XMM2,XMM5
          MOVDQA XMM3,[EDI+2*ECX]
          PSUBW XMM3,XMM2           // Here is a +1 contribution
          PADDW XMM2,XMM1           // Add pre-offset for rounding
          MOVDQU XMM5,[EBX+2*ECX+16] // Load next 8 source 2 samples ahead
          PMULHW XMM2,XMM0           // Multiply by lambda and discard 16 LSB's
          PSUBW XMM3,XMM2           // Final contribution
          MOVDQA XMM2,[EAX+2*ECX+16] // Load next 8 source 1 samples ahead
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_step2_synth128
          JMP end_synth128

start_step3_synth128:
          // Invert Lifting Step 3
          XOR ECX,ECX                // Zero sample counter
          MOVDQA XMM2,[EAX]          // Load initial 8 source 1 samples
          MOVDQU XMM5,[EBX]          // Load initial 8 source 2 samples
loop_step3_synth128:
          PADDW XMM2,XMM5
          MOVDQA XMM3,[EDI+2*ECX]
          PADDW XMM2,XMM1           // Add pre-offset for rounding
          MOVDQU XMM5,[EBX+2*ECX+16] // Load next 8 source 2 samples ahead
          PMULHW XMM2,XMM0           // Multiply by lambda and discard 16 LSB's
          PSUBW XMM3,XMM2           // Final contribution
          MOVDQA XMM2,[EAX+2*ECX+16] // Load next 8 source 1 samples ahead
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_step3_synth128
end_synth128:
        }
    }
  else
#endif // !KDU_NO_SSE
    { // Implementation based on 64-bit operands using only MMX instructions
      __asm
        {         
          MOV EDX,samples // Set limit for sample counter
          MOV EAX,src
          MOV EDI,dst
          MOVQ MM0,vec_lambda
          MOVQ MM1,vec_preoff
          MOV ECX,step_idx
          CMP ECX,3
          JZ start_step3_synth64
          CMP ECX,2
          JZ start_step2_synth64
          CMP ECX,1
          JZ start_step1_synth64

          XOR ECX,ECX   // Not really necessary
loop_step0_synth64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EAX+2*ECX+2]   // Add source sample 2
          MOVQ MM3,[EDI+2*ECX]
          PADDW MM3,MM2     // Here is a -1 contribution
          PADDW MM3,MM2     // Here is another -1 contribution
          PADDW MM2,MM1     // Add pre-offset for rounding
          PMULHW MM2,MM0     // Multiply by lambda and discard 16 LSB's
          PSUBW MM3,MM2     // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step0_synth64
          JMP end_synth64

start_step1_synth64:
          XOR ECX,ECX     // Zero sample counter
          PXOR MM6,MM6    // Set MM6 to 0's
          PCMPEQW MM7,MM7 // Set MM7 to 1's
          PSUBW MM6,MM7   // Leaves each word in MM6 equal to 1
          PSLLW MM6,2     // Leave each word in MM6 = 4 (rounding offset)
loop_step1_synth64:
          MOVQ MM2,[EAX+2*ECX]   // Get source samples 1 to MM2
          PMULHW MM2,MM0         // Multiply by lambda and discard 16 LSB's
          PXOR MM4,MM4
          PSUBW MM4,[EAX+2*ECX+2] // Get -ve source samples 2 to MM4
          PMULHW MM4,MM0     // Multiply by lambda and discard 16 LSB's
          MOVQ MM3,[EDI+2*ECX]
          PSUBW MM2,MM4     // Subtract from non-negated scaled source samples
          PADDW MM2,MM6     // Add post-offset for rounding
          PSRAW MM2,3        // Shift result to the right by 3
          PSUBW MM3,MM2     // Update destination samples
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step1_synth64
          JMP end_synth64

start_step2_synth64:
          XOR ECX,ECX     // Zero sample counter
loop_step2_synth64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EAX+2*ECX+2]   // Add source sample 2
          MOVQ MM3,[EDI+2*ECX]
          PSUBW MM3,MM2     // Here is a +1 contribution
          PADDW MM2,MM1     // Add pre-offset for rounding
          PMULHW MM2,MM0     // Multiply by lambda and discard 16 LSB's
          PSUBW MM3,MM2     // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step2_synth64
          JMP end_synth64

start_step3_synth64:
          XOR ECX,ECX     // Zero sample counter
loop_step3_synth64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EAX+2*ECX+2]   // Add source sample 2
          MOVQ MM3,[EDI+2*ECX]
          PADDW MM2,MM1           // Add pre-offset for rounding
          PMULHW MM2,MM0           // Multiply by lambda and discard 16 LSB's
          PSUBW MM3,MM2           // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step3_synth64

end_synth64:
          EMMS               // Clear MMX registers for use by FPU
        }
    }
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_W9X7_v_analysis                             */
/*****************************************************************************/

static inline bool
  simd_W9X7_v_analysis(kdu_int16 *src1, kdu_int16 *src2,
                       kdu_int16 *dst_in, kdu_int16 *dst_out, int samples,
                       kd_lifting_step *step)
  /* Special function to implement the irreversible 9/7 transform */
{
  if (kdu_mmx_level < 1)
    return false;
  if (samples <= 0)
    return true;
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
  __asm {         
          MOV EDX,samples // Set limit for sample counter
          MOV ESI,dst_in
          MOV EDI,dst_out
          MOVQ MM0,vec_lambda
          MOVQ MM1,vec_preoff
          MOV ECX,step_idx
          MOV EAX,src1
          MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
          CMP ECX,3
          JZ start_step3_analysis64
          CMP ECX,2
          JZ start_step2_analysis64
          CMP ECX,1
          JZ start_step1_analysis64

          XOR ECX,ECX   // Not really necessary
loop_step0_analysis64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EBX+2*ECX]   // Add source sample 2
          MOVQ MM3,[ESI+2*ECX]
          PSUBW MM3,MM2     // Here is a -1 contribution
          PSUBW MM3,MM2     // Here is another -1 contribution
          PADDW MM2,MM1     // Add pre-offset for rounding
          PMULHW MM2,MM0     // Multiply by lambda and discard 16 LSB's
          PADDW MM3,MM2     // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step0_analysis64
          JMP end_analysis64

start_step1_analysis64:
          XOR ECX,ECX     // Zero sample counter
          PXOR MM6,MM6    // Set MM6 to 0's
          PCMPEQW MM7,MM7 // Set MM7 to 1's
          PSUBW MM6,MM7   // Leaves each word in MM6 equal to 1
          PSLLW MM6,2     // Leave each word in MM6 = 4 (rounding offset)
loop_step1_analysis64:
          MOVQ MM2,[EAX+2*ECX]   // Get source samples 1 to MM2
          PMULHW MM2,MM0         // Multiply by lambda and discard 16 LSB's
          PXOR MM4,MM4
          PSUBW MM4,[EBX+2*ECX] // Get -ve source samples 2 to MM4
          PMULHW MM4,MM0     // Multiply by lambda and discard 16 LSB's
          MOVQ MM3,[ESI+2*ECX]
          PSUBW MM2,MM4     // Subtract from non-negated scaled source samples
          PADDW MM2,MM6     // Add post-offset for rounding
          PSRAW MM2,3        // Shift result to the right by 3
          PADDW MM3,MM2     // Update destination samples
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step1_analysis64
          JMP end_analysis64

start_step2_analysis64:
          XOR ECX,ECX     // Zero sample counter
loop_step2_analysis64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EBX+2*ECX]   // Add source sample 2
          MOVQ MM3,[ESI+2*ECX]
          PADDW MM3,MM2     // Here is a +1 contribution
          PADDW MM2,MM1     // Add pre-offset for rounding
          PMULHW MM2,MM0     // Multiply by lambda and discard 16 LSB's
          PADDW MM3,MM2     // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step2_analysis64
          JMP end_analysis64

start_step3_analysis64:
          XOR ECX,ECX     // Zero sample counter
loop_step3_analysis64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EBX+2*ECX]   // Add source sample 2
          MOVQ MM3,[ESI+2*ECX]
          PADDW MM2,MM1           // Add pre-offset for rounding
          PMULHW MM2,MM0           // Multiply by lambda and discard 16 LSB's
          PADDW MM3,MM2           // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step3_analysis64

end_analysis64:
          EMMS               // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W9X7_v_synth                              */
/*****************************************************************************/

static inline bool
  simd_W9X7_v_synth(kdu_int16 *src1, kdu_int16 *src2,
                    kdu_int16 *dst_in, kdu_int16 *dst_out, int samples,
                    kd_lifting_step *step)
  /* Special function to implement the irreversible 9/7 transform */
{
  if (kdu_mmx_level < 1)
    return false;
  if (samples <= 0)
    return true;
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
      __asm
        {         
          MOV EDX,samples // Set limit for sample counter
          MOV ESI,dst_in
          MOV EDI,dst_out
          MOVDQU XMM0,vec_lambda
          MOVDQU XMM1,vec_preoff
          MOV ECX,step_idx
          MOV EAX,src1
          MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
          CMP ECX,3
          JZ start_step3_synth128
          CMP ECX,2
          JZ start_step2_synth128
          CMP ECX,1
          JZ start_step1_synth128

          XOR ECX,ECX   // Not really necessary
          MOVDQA XMM2,[EAX]    // Load initial 8 source 1 samples
          PADDW XMM2,[EBX]    // Add in initial 8 source 2 samples
loop_step0_synth128:
          MOVDQA XMM3,[ESI+2*ECX]
          PADDW XMM3,XMM2     // Here is a -1 contribution
          PADDW XMM3,XMM2     // Here is another -1 contribution
          PADDW XMM2,XMM1     // Add pre-offset for rounding
          PMULHW XMM2,XMM0     // Multiply by lambda and discard 16 LSB's
          PSUBW XMM3,XMM2     // Final contribution
          MOVDQA XMM2,[EAX+2*ECX+16]   // Load 8 source 1 samples ahead
          PADDW XMM2,[EBX+2*ECX+16]   // Add 8 source 2 samples ahead
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_step0_synth128
          JMP end_synth128

start_step1_synth128:
          XOR ECX,ECX       // Zero sample counter
          PXOR XMM6,XMM6    // Set XMM6 to 0's
          PCMPEQW XMM7,XMM7 // Set XMM7 to 1's
          PSUBW XMM6,XMM7   // Leaves each word in MM6 equal to 1
          PSLLW XMM6,2      // Leave each word in XMM6 = 4 (rounding offset)
          PXOR XMM4,XMM4
          PSUBW XMM4,[EBX] // Load 8 negated source 2 samples
loop_step1_synth128:
          PMULHW XMM4,XMM0  // -src2 samples times lambda, discarding 16 LSBs
          MOVDQA XMM2,[EAX+2*ECX] // Load 8 source 1 samples
          PMULHW XMM2,XMM0  // src1 samples times lambda, discarding 16 LSBs
          MOVDQA XMM3,[ESI+2*ECX]
          PSUBW XMM2,XMM4  // Subtract neg product from non-negated product
          PXOR XMM4,XMM4
          PSUBW XMM4,[EBX+2*ECX+16] // Load 8 negated source 2 samples ahead
          PADDW XMM2,XMM6  // Add post-offset for rounding
          PSRAW XMM2,3      // Shift result to the right by 3
          PSUBW XMM3,XMM2  // Update destination samples
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_step1_synth128
          JMP end_synth128

start_step2_synth128:
          XOR ECX,ECX          // Zero sample counter
          MOVDQA XMM2,[EAX]    // Load initial 8 source 1 samples
          PADDW XMM2,[EBX]    // Add in initial 8 source 2 samples
loop_step2_synth128:
          MOVDQA XMM3,[ESI+2*ECX]
          PSUBW XMM3,XMM2     // Here is a +1 contribution
          PADDW XMM2,XMM1     // Add pre-offset for rounding
          PMULHW XMM2,XMM0     // Multiply by lambda and discard 16 LSB's
          PSUBW XMM3,XMM2     // Final contribution
          MOVDQA XMM2,[EAX+2*ECX+16]   // Load next 8 source 1 words ahead
          PADDW XMM2,[EBX+2*ECX+16]   // Add in next 8 source 2 words ahead
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_step2_synth128
          JMP end_synth128

start_step3_synth128:
          XOR ECX,ECX           // Zero sample counter
          MOVDQA XMM2,[EAX]     // Load initial 8 source 1 samples
          PADDW XMM2,[EBX]     // Add in initial 8 source 2 samples
loop_step3_synth128:
          MOVDQA XMM3,[ESI+2*ECX]
          PADDW XMM2,XMM1      // Add pre-offset for rounding
          PMULHW XMM2,XMM0      // Multiply by lambda and discard 16 LSB's
          PSUBW XMM3,XMM2      // Final contribution
          MOVDQA XMM2,[EAX+2*ECX+16]   // Load next 8 source 1 words ahead
          PADDW XMM2,[EBX+2*ECX+16]   // Add in next 8 source 2 words ahead
          MOVDQA [EDI+2*ECX],XMM3
          ADD ECX,8
          CMP ECX,EDX
          JL loop_step3_synth128
end_synth128:
        }
    }
  else
#endif // !KDU_NO_SSE
    { // Implementation based on 64-bit operands, using only MMX instructions
      __asm
        {         
          MOV EDX,samples // Set limit for sample counter
          MOV ESI,dst_in
          MOV EDI,dst_out
          MOVQ MM0,vec_lambda
          MOVQ MM1,vec_preoff
          MOV ECX,step_idx
          MOV EAX,src1
          MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
          CMP ECX,3
          JZ start_step3_synth64
          CMP ECX,2
          JZ start_step2_synth64
          CMP ECX,1
          JZ start_step1_synth64

          XOR ECX,ECX   // Not really necessary
loop_step0_synth64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EBX+2*ECX]   // Add source sample 2
          MOVQ MM3,[ESI+2*ECX]
          PADDW MM3,MM2     // Here is a -1 contribution
          PADDW MM3,MM2     // Here is another -1 contribution
          PADDW MM2,MM1     // Add pre-offset for rounding
          PMULHW MM2,MM0     // Multiply by lambda and discard 16 LSB's
          PSUBW MM3,MM2     // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step0_synth64
          JMP end_synth64

start_step1_synth64:
          XOR ECX,ECX     // Zero sample counter
          PXOR MM6,MM6    // Set MM6 to 0's
          PCMPEQW MM7,MM7 // Set MM7 to 1's
          PSUBW MM6,MM7   // Leaves each word in MM6 equal to 1
          PSLLW MM6,2     // Leave each word in MM6 = 4 (rounding offset)
loop_step1_synth64:
          MOVQ MM2,[EAX+2*ECX]   // Get source samples 1 to MM2
          PMULHW MM2,MM0         // Multiply by lambda and discard 16 LSB's
          PXOR MM4,MM4
          PSUBW MM4,[EBX+2*ECX] // Get -ve source samples 2 to MM4
          PMULHW MM4,MM0     // Multiply by lambda and discard 16 LSB's
          MOVQ MM3,[ESI+2*ECX]
          PSUBW MM2,MM4     // Subtract from non-negated scaled source samples
          PADDW MM2,MM6     // Add post-offset for rounding
          PSRAW MM2,3        // Shift result to the right by 3
          PSUBW MM3,MM2     // Update destination samples
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step1_synth64
          JMP end_synth64

start_step2_synth64:
          XOR ECX,ECX     // Zero sample counter
loop_step2_synth64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EBX+2*ECX]   // Add source sample 2
          MOVQ MM3,[ESI+2*ECX]
          PSUBW MM3,MM2     // Here is a +1 contribution
          PADDW MM2,MM1     // Add pre-offset for rounding
          PMULHW MM2,MM0     // Multiply by lambda and discard 16 LSB's
          PSUBW MM3,MM2     // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step2_synth64
          JMP end_synth64

start_step3_synth64:
          XOR ECX,ECX     // Zero sample counter
loop_step3_synth64:
          MOVQ MM2,[EAX+2*ECX]     // Start with source sample 1
          PADDW MM2,[EBX+2*ECX]   // Add source sample 2
          MOVQ MM3,[ESI+2*ECX]
          PADDW MM2,MM1           // Add pre-offset for rounding
          PMULHW MM2,MM0           // Multiply by lambda and discard 16 LSB's
          PSUBW MM3,MM2           // Final contribution
          MOVQ [EDI+2*ECX],MM3
          ADD ECX,4
          CMP ECX,EDX
          JL loop_step3_synth64

end_synth64:
          EMMS               // Clear MMX registers for use by FPU
        }
    }
  return true;
}

/*****************************************************************************/
/* INLINE                       simd_deinterleave                            */
/*****************************************************************************/

static inline bool
  simd_deinterleave(kdu_int16 *src, kdu_int16 *dst1, kdu_int16 *dst2,
                    int pairs)
{
  if (kdu_mmx_level < 1)
    return false;
  if (pairs <= 0)
    return true;

  // Implementation based on 64-bit operands, using only MMX instructions
  __asm {
          MOV ECX,pairs      // Set up counter used for looping
          MOV EAX,src
          MOV EDX,dst2
          MOV EBX,dst1 // Load last (.NET 2008 uses EBX for dbg stack frame)
loop_dilv64:
          MOVQ MM0,[EAX]      // Load first source quad
          MOVQ MM1,[EAX+8]    // Load second source quad
          MOVQ MM2,MM0        // Make a copy of first source quad
          MOVQ MM3,MM1        // Make a copy of second source quad
          PSLLD MM0,16
          PSRAD MM0,16        // Leaves sign extended copies of words 0 and 2
          PSLLD MM1,16
          PSRAD MM1,16        // Leaves sign extended copies of words 4 and 6
          PACKSSDW MM0,MM1    // Leaves words 0,2,4,6 in MM0
          MOVQ [EBX],MM0      // Save even indexed words in destination 1
          PSRAD MM2,16        // Leaves sign extended copies of words 1 and 3
          PSRAD MM3,16        // Leaves sign extended copies of words 5 and 7
          PACKSSDW MM2,MM3    // Leaves words 1,3,5,7 in MM2
          MOVQ [EDX],MM2      // Save odd indexed words in destination 2
          ADD EAX,16
          ADD EBX,8
          ADD EDX,8
          SUB ECX,4
          JG loop_dilv64
          EMMS // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                        simd_interleave                             */
/*****************************************************************************/

static inline bool
  simd_interleave(kdu_int16 *src1, kdu_int16 *src2, kdu_int16 *dst, int pairs)
{
  if (kdu_mmx_level < 1)
    return false;
  if (pairs <= 0)
    return true;

  __asm
    {
      MOV ECX,pairs      // Set up counter used for looping
      MOV EDX,dst
      MOV EAX,src1
      MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
#ifndef KDU_NO_SSE
      MOV EDI,kdu_mmx_level
      CMP EDI,2
      JL loop_ilv64
      CMP ECX,32 // 128-bit processing not worthwhile unless >= 32 pairs exist
      JL loop_ilv64

      // If we get here, we can use the implementation based on 128-bit
      // operands, using SSE/SSE2 instructions
      SUB ECX,8 // Halts 128-bit processing when 1 to 8 pairs are left
      TEST EAX,8 // See if source address is 16-byte aligned
      JZ loop_ilv128  // If not, source words must be 8-byte aligned
        MOVDQA XMM0,[EAX-8]
        MOVDQA XMM1,[EBX-8]
        PUNPCKHWD XMM0,XMM1
        MOVDQA [EDX],XMM0
        ADD EAX,8
        ADD EBX,8
        ADD EDX,16
        SUB ECX,4
loop_ilv128:
      MOVDQA XMM0,[EAX]
      MOVDQA XMM2,XMM0
      MOVDQA XMM1,[EBX]
      PUNPCKLWD XMM2,XMM1
      MOVDQA [EDX],XMM2
      PUNPCKHWD XMM0,XMM1
      MOVDQA [EDX+16],XMM0
      ADD EAX,16
      ADD EBX,16
      ADD EDX,32
      SUB ECX,8
      JG loop_ilv128
      ADD ECX,8
#endif // !KDU_NO_SSE
loop_ilv64:
      MOVQ MM0,[EAX]
      MOVQ MM2,MM0
      MOVQ MM1,[EBX]
      PUNPCKLWD MM2,MM1
      MOVQ [EDX],MM2
      PUNPCKHWD MM0,MM1
      MOVQ [EDX+8],MM0
      ADD EAX,8
      ADD EBX,8
      ADD EDX,16
      SUB ECX,4
      JG loop_ilv64
      EMMS // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                  simd_downshifted_deinterleave                     */
/*****************************************************************************/

static inline bool
  simd_downshifted_deinterleave(kdu_int16 *src, kdu_int16 *dst1,
                                kdu_int16 *dst2, int pairs, int downshift)
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
  __asm {
          MOV ECX,pairs      // Set up counter used for looping
          MOV EAX,src
          MOVD MM4,downshift
          MOVQ MM5,vec_offset
          MOV EDX,dst2
          MOV EBX,dst1 // Load last (.NET 2008 uses EBX for dbg stack frame)
loop_shift_dilv64:
          MOVQ MM0,[EAX]      // Load first source quad
          MOVQ MM1,[EAX+8]    // Load second source quad
          PADDW MM0,MM5      // Add the offset to first quad
          PSRAW MM0,MM4       // Apply downshift to first quad
          PADDW MM1,MM5      // Add the offset to second quad
          PSRAW MM1,MM4       // Apply the downshift to second quad
          MOVQ MM2,MM0        // Make a copy of first shifted quad
          MOVQ MM3,MM1        // Make a copy of second shifted quad
          PSLLD MM0,16
          PSRAD MM0,16        // Leaves sign extended copies of words 0 and 2
          PSLLD MM1,16
          PSRAD MM1,16        // Leaves sign extended copies of words 4 and 6
          PACKSSDW MM0,MM1    // Leaves words 0,2,4,6 in MM0
          MOVQ [EBX],MM0      // Save even indexed words in destination 1
          PSRAD MM2,16        // Leaves sign extended copies of words 1 and 3
          PSRAD MM3,16        // Leaves sign extended copies of words 5 and 7
          PACKSSDW MM2,MM3    // Leaves words 1,3,5,7 in MM2
          MOVQ [EDX],MM2      // Save odd indexed words in destination 2
          ADD EAX,16
          ADD EBX,8
          ADD EDX,8
          SUB ECX,4
          JG loop_shift_dilv64
          EMMS // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_upshifted_interleave                        */
/*****************************************************************************/

static inline bool
  simd_upshifted_interleave(kdu_int16 *src1, kdu_int16 *src2,
                            kdu_int16 *dst, int pairs, int upshift)
{
  if (kdu_mmx_level < 1)
    return false;
  if (pairs <= 0)
    return true;

  __asm
    {
      MOV ECX,pairs      // Set up counter used for looping
      MOV EDX,dst
      MOVD MM3,upshift
      MOV EAX,src1
      MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
#ifndef KDU_NO_SSE
      MOV EDI,kdu_mmx_level
      CMP EDI,2
      JL loop_shift_ilv64
      CMP ECX,32 // 128-bit processing not worthwhile unless >= 32 pairs exist
      JL loop_shift_ilv64

      // If we get here, we can use the implementation based on 128-bit
      // operands, using SSE/SSE2 instructions
      MOVD XMM3,upshift
      SUB ECX,8 // Halts 128-bit processing when <= 8 pairs are left
      TEST EAX,8 // See if source address is 16-byte aligned
      JZ loop_shift_ilv128  // If not, source words must be 8-byte aligned
        MOVDQA XMM0,[EAX-8]
        PSLLW XMM0,XMM3
        MOVDQA XMM1,[EBX-8]
        PSLLW XMM1,XMM3
        PUNPCKHWD XMM0,XMM1
        MOVDQA [EDX],XMM0
        ADD EAX,8
        ADD EBX,8
        ADD EDX,16
        SUB ECX,4
loop_shift_ilv128:
      MOVDQA XMM0,[EAX]
      PSLLW XMM0,XMM3
      MOVDQA XMM2,XMM0
      MOVDQA XMM1,[EBX]
      PSLLW XMM1,XMM3
      PUNPCKLWD XMM2,XMM1
      MOVDQA [EDX],XMM2
      PUNPCKHWD XMM0,XMM1
      MOVDQA [EDX+16],XMM0
      ADD EAX,16
      ADD EBX,16
      ADD EDX,32
      SUB ECX,8
      JG loop_shift_ilv128
      ADD ECX,8  // Restore the amount that we subtracted earlier
#endif // !KDU_NO_SSE
loop_shift_ilv64:
      MOVQ MM0,[EAX]
      PSLLW MM0,MM3
      MOVQ MM2,MM0
      MOVQ MM1,[EBX]
      PSLLW MM1,MM3
      PUNPCKLWD MM2,MM1
      MOVQ [EDX],MM2
      PUNPCKHWD MM0,MM1
      MOVQ [EDX+8],MM0
      ADD EAX,8
      ADD EBX,8
      ADD EDX,16
      SUB ECX,4
      JG loop_shift_ilv64
      EMMS // Clear MMX registers for use by FPU
    }
  return true;
}


/* ========================================================================= */
/*                  MMX/SSE functions for 32-bit samples                     */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                     simd_2tap_h_irrev32                            */
/*****************************************************************************/

static inline bool
  simd_2tap_h_irrev32(float *src, float *dst, int samples,
                      kd_lifting_step *step, bool synthesis)
  /* Similar to `simd_2tap_h_synth', except this function works with 32-bit
     floating point sample values and can handle both irreversible synthesis
     and irreversible analysis -- the `synthesis' argument determines which
     is used. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length == 1) || (step->support_length == 2));
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

  float lambda0 = step->coeffs[0];
  float lambda1 = (step->support_length==2)?(step->coeffs[1]):0.0F;
  if (synthesis)
    { lambda0 = -lambda0;  lambda1 = -lambda1; }
  __asm {
      MOVSS XMM0, lambda0
      MOVSS XMM1, lambda1
      PUNPCKLDQ XMM0,XMM0
      PUNPCKLDQ XMM1,XMM1
      PUNPCKLQDQ XMM0,XMM0
      PUNPCKLQDQ XMM1,XMM1
      MOV ESI,src
      MOV EDI,dst

      MOV EDX,samples // Limit for ECX
      XOR ECX,ECX        // Zero sample counter
      MOVUPS XMM4,[ESI]
      MOVUPS XMM5,[ESI+4]
loop_dqword:
      MULPS XMM4, XMM0
      MULPS XMM5, XMM1
      MOVAPS XMM2, [EDI+4*ECX]
      ADDPS XMM5, XMM4
      MOVUPS XMM4, [ESI+4*ECX+16]
      ADDPS XMM2, XMM5
      MOVUPS XMM5, [ESI+4*ECX+20]
      MOVAPS [EDI+4*ECX],XMM2
      ADD ECX,4
      CMP ECX,EDX
      JL loop_dqword
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_4tap_h_irrev32                            */
/*****************************************************************************/

static inline bool
  simd_4tap_h_irrev32(float *src, float *dst, int samples,
                      kd_lifting_step *step, bool synthesis)
  /* Similar to `simd_2tap_h_synth', except this function works with 32-bit
     floating point sample values and can handle both irreversible synthesis
     and irreversible analysis -- the `synthesis' argument determines which
     is used. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length >= 3) && (step->support_length <= 4));
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

  float lambda0 = step->coeffs[0];
  float lambda1 = step->coeffs[1];
  float lambda2 = step->coeffs[2];
  float lambda3 = (step->support_length==4)?(step->coeffs[3]):0.0F;
  if (synthesis)
    { lambda0 = -lambda0;  lambda1 = -lambda1;
      lambda2 = -lambda2;  lambda3 = -lambda3; }
  __asm {
      MOVSS XMM0, lambda0
      MOVSS XMM1, lambda1
      MOVSS XMM2, lambda2
      MOVSS XMM3, lambda3
      PUNPCKLDQ XMM0,XMM0
      PUNPCKLDQ XMM1,XMM1
      PUNPCKLDQ XMM2,XMM2
      PUNPCKLDQ XMM3,XMM3
      PUNPCKLQDQ XMM0,XMM0
      PUNPCKLQDQ XMM1,XMM1
      PUNPCKLQDQ XMM2,XMM2
      PUNPCKLQDQ XMM3,XMM3
      MOV ESI,src
      MOV EDI,dst
      MOV EDX,samples // Limit for ECX
      XOR ECX,ECX        // Zero sample counter
      MOVUPS XMM4,[ESI]
      MOVUPS XMM5,[ESI+4]
      MOVUPS XMM6,[ESI+8]
      MOVUPS XMM7,[ESI+12]
loop_dqword:
      MULPS XMM4, XMM0
      MULPS XMM5, XMM1
      ADDPS XMM4, XMM5
      MOVAPS XMM5, [EDI+4*ECX]
      MULPS XMM6, XMM2
      ADDPS XMM5, XMM4
      MOVUPS XMM4, [ESI+4*ECX+16]
      MULPS XMM7, XMM3
      ADDPS XMM6, XMM5
      MOVUPS XMM5, [ESI+4*ECX+20]
      ADDPS XMM7, XMM6
      MOVUPS XMM6, [ESI+4*ECX+24]
      MOVAPS [EDI+4*ECX], XMM7
      MOVUPS XMM7, [ESI+4*ECX+28]
      ADD ECX,4
      CMP ECX,EDX
      JL loop_dqword
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_2tap_v_irrev32                            */
/*****************************************************************************/

static inline bool
  simd_2tap_v_irrev32(float *src0, float *src1, float *dst_in, float *dst_out,
                      int samples, kd_lifting_step *step, bool synthesis)
  /* Similar to `simd_2tap_v_synth', except this function works with 32-bit
     floating point sample values and can handle both irreversible synthesis
     and irreversible analysis -- the `synthesis' argument determines which
     is used. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length == 1) || (step->support_length == 2));
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;
  float lambda0 = step->coeffs[0];
  float lambda1 = (step->support_length==2)?(step->coeffs[1]):0.0F;
  if (synthesis)
    { lambda0 = -lambda0;  lambda1 = -lambda1; }
  __asm {
      MOVSS XMM0, lambda0
      MOVSS XMM1, lambda1
      PUNPCKLDQ XMM0,XMM0
      PUNPCKLDQ XMM1,XMM1
      PUNPCKLQDQ XMM0,XMM0
      PUNPCKLQDQ XMM1,XMM1
      XOR ECX,ECX     // Zero sample counter
      MOV EDX,samples // Limit for ECX
      MOV ESI,dst_in
      MOV EDI,dst_out
      MOV EAX,src0
      MOV EBX,src1 // Load last (.NET 2008 uses EBX for dbg stack frame)
      MOVAPS XMM4,[EAX]
      MOVAPS XMM5,[EBX]
loop_dqword:
      MULPS XMM4, XMM0
      MULPS XMM5, XMM1
      MOVAPS XMM2, [ESI+4*ECX]
      ADDPS XMM5, XMM4
      MOVAPS XMM4, [EAX+4*ECX+16]
      ADDPS XMM2, XMM5
      MOVAPS XMM5, [EBX+4*ECX+16]
      MOVAPS [EDI+4*ECX], XMM2
      ADD ECX,4
      CMP ECX,EDX
      JL loop_dqword
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                     simd_4tap_v_irrev32                            */
/*****************************************************************************/

static inline bool
  simd_4tap_v_irrev32(float *src0, float *src1, float *src2, float *src3,
                      float *dst_in, float *dst_out,
                      int samples, kd_lifting_step *step, bool synthesis)
  /* Similar to `simd_4tap_v_synth', except this function works with 32-bit
     floating point sample values and can handle both irreversible synthesis
     and irreversible analysis -- the `synthesis' argument determines which
     is used. */
{
#ifdef KDU_NO_SSE
  return false;
#else
  assert((step->support_length >= 3) || (step->support_length <= 4));
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

  float lambda0 = step->coeffs[0];
  float lambda1 = step->coeffs[1];
  float lambda2 = step->coeffs[2];
  float lambda3 = (step->support_length==4)?(step->coeffs[3]):0.0F;
  if (synthesis)
    { lambda0 = -lambda0;  lambda1 = -lambda1;
      lambda2 = -lambda2;  lambda3 = -lambda3; }
  __asm {
      MOVSS XMM0, lambda0
      MOVSS XMM1, lambda1
      MOVSS XMM2, lambda2
      MOVSS XMM3, lambda3
      PUNPCKLDQ XMM0,XMM0
      PUNPCKLDQ XMM1,XMM1
      PUNPCKLDQ XMM2,XMM2
      PUNPCKLDQ XMM3,XMM3
      PUNPCKLQDQ XMM0,XMM0
      PUNPCKLQDQ XMM1,XMM1
      PUNPCKLQDQ XMM2,XMM2
      PUNPCKLQDQ XMM3,XMM3
      XOR ECX,ECX     // Zero sample counter
      MOV EDX, src2
      MOV ESI,dst_in
      MOV EDI,dst_out
      MOV EAX,src0
      MOV EBX,src1 // Load last (.NET 2008 uses EBX for dbg stack frame)
loop_dqword:
      MOVUPS XMM4, [EAX+4*ECX]
      MOVUPS XMM5, [EBX+4*ECX]
      MOVUPS XMM6, [EDX+4*ECX]
      MOV EDX, src3
      MULPS XMM4, XMM0
      MULPS XMM5, XMM1
      MULPS XMM6, XMM2
      MOVUPS XMM7, [EDX+4*ECX]
      ADDPS XMM5, XMM4
      MOVAPS XMM4, [ESI+4*ECX]
      MULPS XMM7, XMM3
      ADDPS XMM6, XMM5
      ADDPS XMM6, XMM4
      ADDPS XMM7, XMM6
      MOVAPS [EDI+4*ECX], XMM7
      ADD ECX,4
      MOV EDX, src2
      CMP ECX,samples
      JL loop_dqword
    }
  return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                   simd_W5X3_h_analysis32                           */
/*****************************************************************************/

static inline bool
  simd_W5X3_h_analysis32(kdu_int32 *src, kdu_int32 *dst, int samples,
                         kd_lifting_step *step)
  /* As above, but for 32-bit integer sample values. */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
#ifndef KDU_NO_SSE
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

  kdu_int32 vec_offset[4];
  vec_offset[0] = (kdu_int32)((1<<step->downshift)>>1);
  for (int k=1; k < 4; k++)
    vec_offset[k] = vec_offset[0];
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));
  __asm
    {
      MOVDQU XMM0,vec_offset
      MOVD XMM1,downshift
      MOV EDX,samples // Set limit for sample counter
      MOV EDI,dst
      MOV EAX,src     // EAX points to first source sample
      MOV ECX,int_coeff
      MOV EBX,EAX
      ADD EBX,4       // EBX points to second source sample
      TEST EAX,0Fh    // See if EAX is 16-byte aligned
      JZ eax_aligned
      XCHG EAX,EBX    // Make sure EAX is the aligned address
eax_aligned:
      CMP ECX,1
      JNE start_minus1

      XOR ECX,ECX     // Initialize the sample counter
      MOVDQA XMM2,[EAX+4*ECX]    // Load initial 4 source 1 samples
      MOVDQU XMM5,[EBX+4*ECX]    // Get initial 4 source 2 samples
loop_plus1:
      MOVDQA XMM7,XMM0             // Start with the offset
      PADDD XMM7,XMM2
      MOVDQA XMM2,[EAX+4*ECX+16]  // Pre-load aligned dqword
      PADDD XMM7,XMM5
      MOVDQU XMM5,[EBX+4*ECX+16]
      MOVDQA XMM3,[EDI+4*ECX]
      PSRAD XMM7,XMM1
      PADDD XMM3,XMM7
      MOVDQA [EDI+4*ECX],XMM3
      ADD ECX,4
      CMP ECX,EDX
      JL loop_plus1
      JMP done

start_minus1:
      // Invert Lifting Step 1
      XOR ECX,ECX                // Zero sample counter
      MOVDQA XMM2,[EAX+4*ECX]    // Load initial 4 source 1 samples
      MOVDQU XMM5,[EBX+4*ECX]    // Get initial 4 source 2 samples
loop_minus1:
      MOVDQA XMM7,XMM0             // Start with the offset
      PSUBD XMM7,XMM2
      MOVDQA XMM2,[EAX+4*ECX+16]  // Pre-load aligned octet
      PSUBD XMM7,XMM5
      MOVDQU XMM5,[EBX+4*ECX+16]
      MOVDQA XMM3,[EDI+4*ECX]
      PSRAD XMM7,XMM1
      PADDD XMM3,XMM7
      MOVDQA [EDI+4*ECX],XMM3
      ADD ECX,4
      CMP ECX,EDX
      JL loop_minus1
done:
    }
#endif // KDU_NO_SSE
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W5X3_h_synth32                            */
/*****************************************************************************/

static inline bool
  simd_W5X3_h_synth32(kdu_int32 *src, kdu_int32 *dst, int samples,
                      kd_lifting_step *step)
  /* As above, but for 32-bit integer sample values. */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
#ifndef KDU_NO_SSE
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

  kdu_int32 vec_offset[4];
  vec_offset[0] = (kdu_int32)((1<<step->downshift)>>1);
  for (int k=1; k < 4; k++)
    vec_offset[k] = vec_offset[0];
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));
  __asm
    {
      MOVDQU XMM0,vec_offset
      MOVD XMM1,downshift
      MOV EDX,samples // Set limit for sample counter
      MOV EDI,dst
      MOV EAX,src     // EAX points to first source sample
      MOV ECX,int_coeff
      MOV EBX,EAX
      ADD EBX,4       // EBX points to second source sample
      TEST EAX,0Fh    // See if EAX is 16-byte aligned
      JZ eax_aligned
      XCHG EAX,EBX    // Make sure EAX is the aligned address
eax_aligned:
      CMP ECX,1
      JNE start_minus1_synth128

      XOR ECX,ECX     // Initialize the sample counter
      MOVDQA XMM2,[EAX+4*ECX]    // Load initial 4 source 1 samples
      MOVDQU XMM5,[EBX+4*ECX]    // Get initial 4 source 2 samples
loop_plus1_synth128:
      MOVDQA XMM7,XMM0             // Start with the offset
      PADDD XMM7,XMM2
      MOVDQA XMM2,[EAX+4*ECX+16]  // Pre-load aligned dqword
      PADDD XMM7,XMM5
      MOVDQU XMM5,[EBX+4*ECX+16]
      MOVDQA XMM3,[EDI+4*ECX]
      PSRAD XMM7,XMM1
      PSUBD XMM3,XMM7
      MOVDQA [EDI+4*ECX],XMM3
      ADD ECX,4
      CMP ECX,EDX
      JL loop_plus1_synth128
      JMP end_synth128

start_minus1_synth128:
      // Invert Lifting Step 1
      XOR ECX,ECX                // Zero sample counter
      MOVDQA XMM2,[EAX+4*ECX]    // Load initial 4 source 1 samples
      MOVDQU XMM5,[EBX+4*ECX]    // Get initial 4 source 2 samples
loop_minus1_synth128:
      MOVDQA XMM7,XMM0             // Start with the offset
      PSUBD XMM7,XMM2
      MOVDQA XMM2,[EAX+4*ECX+16]  // Pre-load aligned octet
      PSUBD XMM7,XMM5
      MOVDQU XMM5,[EBX+4*ECX+16]
      MOVDQA XMM3,[EDI+4*ECX]
      PSRAD XMM7,XMM1
      PSUBD XMM3,XMM7
      MOVDQA [EDI+4*ECX],XMM3
      ADD ECX,4
      CMP ECX,EDX
      JL loop_minus1_synth128
end_synth128:
    }
#endif // KDU_NO_SSE
  return true;
}

/*****************************************************************************/
/* INLINE                   simd_W5X3_v_analysis32                           */
/*****************************************************************************/

static inline bool
  simd_W5X3_v_analysis32(kdu_int32 *src1, kdu_int32 *src2,
                         kdu_int32 *dst_in, kdu_int32 *dst_out, int samples,
                         kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
#ifndef KDU_NO_SSE
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

  kdu_int32 vec_offset[4];
  vec_offset[0] = (kdu_int32)((1<<step->downshift)>>1);
  for (int k=1; k < 4; k++)
    vec_offset[k] = vec_offset[0];
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));
  __asm {
      MOV EDX,samples // Limit for ECX
      MOV ESI,dst_in
      MOV EDI,dst_out
      MOVDQU XMM0,vec_offset
      MOVD XMM1,downshift
      MOV ECX,int_coeff
      MOV EAX,src1
      MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
      CMP ECX,1
      MOV ECX,0  // Set up sample counter, without altering status flags
      JNE loop_minus1
loop_plus1:
      MOVDQA XMM2,XMM0         // start with the offset
      PADDD XMM2,[EAX+4*ECX]   // add 1'st source sample
      PADDD XMM2,[EBX+4*ECX]   // add 2'nd source sample
      MOVDQA XMM3,[ESI+4*ECX]
      PSRAD XMM2,XMM1          // shift rigth by the `downshift' value
      PADDD XMM3,XMM2          // add to dest sample
      MOVDQA [EDI+4*ECX],XMM3
      ADD ECX,4
      CMP ECX,EDX
      JL loop_plus1
      JMP done
loop_minus1:
      MOVDQA XMM2,XMM0         // start with the offset
      PSUBD XMM2,[EAX+4*ECX]   // subtract 1'st source sample
      PSUBD XMM2,[EBX+4*ECX]   // subtract 2'nd source sample
      MOVDQA XMM3,[ESI+4*ECX]
      PSRAD XMM2,XMM1          // shift rigth by the `downshift' value
      PADDD XMM3,XMM2          // add to dest sample
      MOVDQA [EDI+4*ECX],XMM3
      ADD ECX,4
      CMP ECX,EDX
      JL loop_minus1
done:
    }
#endif KDU_NO_SSE
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_W5X3_v_synth32                            */
/*****************************************************************************/

static inline bool
  simd_W5X3_v_synth32(kdu_int32 *src1, kdu_int32 *src2,
                      kdu_int32 *dst_in, kdu_int32 *dst_out, int samples,
                      kd_lifting_step *step)
  /* Special function to implement the reversible 5/3 transform */
{
  assert((step->support_length == 2) && (step->icoeffs[0]==step->icoeffs[1]));
#ifndef KDU_NO_SSE
  if (kdu_mmx_level < 2)
    return false;
  if (samples <= 0)
    return true;

  kdu_int32 vec_offset[4];
  vec_offset[0] = (kdu_int32)((1<<step->downshift)>>1);
  for (int k=1; k < 4; k++)
    vec_offset[k] = vec_offset[0];
  int int_coeff = step->icoeffs[0];
  int downshift = step->downshift;
  assert((int_coeff == 1) || (int_coeff == -1));
  __asm {
      MOV EDX,samples // Limit for ECX
      MOV ESI,dst_in
      MOV EDI,dst_out
      MOVDQU XMM0,vec_offset
      MOVD XMM1,downshift
      MOV ECX,int_coeff
      MOV EAX,src1
      MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)
      CMP ECX,1
      MOV ECX,0  // Set up sample counter, without altering status flags
      JNE loop_minus1
loop_plus1:
      MOVDQA XMM2,XMM0         // start with the offset
      PADDD XMM2,[EAX+4*ECX]   // add 1'st source sample
      PADDD XMM2,[EBX+4*ECX]   // add 2'nd source sample
      MOVDQA XMM3,[ESI+4*ECX]
      PSRAD XMM2,XMM1          // shift rigth by the `downshift' value
      PSUBD XMM3,XMM2          // subtract from dest sample
      MOVDQA [EDI+4*ECX],XMM3
      ADD ECX,4
      CMP ECX,EDX
      JL loop_plus1
      JMP done
loop_minus1:
      MOVDQA XMM2,XMM0         // start with the offset
      PSUBD XMM2,[EAX+4*ECX]   // subtract 1'st source sample
      PSUBD XMM2,[EBX+4*ECX]   // subtract 2'nd source sample
      MOVDQA XMM3,[ESI+4*ECX]
      PSRAD XMM2,XMM1          // shift rigth by the `downshift' value
      PSUBD XMM3,XMM2          // subtract from dest sample
      MOVDQA [EDI+4*ECX],XMM3
      ADD ECX,4
      CMP ECX,EDX
      JL loop_minus1
done:
    }
#endif KDU_NO_SSE
  return true;
}

/*****************************************************************************/
/* INLINE                        simd_interleave                             */
/*****************************************************************************/

static inline bool
  simd_interleave(kdu_int32 *src1, kdu_int32 *src2, kdu_int32 *dst, int pairs)
{
#ifndef KDU_NO_SSE
  if (kdu_mmx_level < 2)
    return false;
  if (pairs <= 8)
    return false;

  __asm {
      MOV ECX,pairs      // Set up counter used for looping
      MOV EDX,dst
      MOV EAX,src1
      MOV EBX,src2 // Load last (.NET 2008 uses EBX for dbg stack frame)

      // If we get here, we can use the implementation based on 128-bit
      // operands, using SSE/SSE2 instructions
      SUB ECX,2 // Halts full dqword processing when 2 or less pairs remain
      TEST EAX,8 // See if source address is 16-byte aligned
      JZ loop_full  // If not, source words must be 8-byte aligned
        MOVDQA XMM0,[EAX-8]
        MOVDQA XMM1,[EBX-8]
        PUNPCKHDQ XMM0,XMM1
        MOVDQA [EDX],XMM0
        ADD EAX,8
        ADD EBX,8
        ADD EDX,16
        SUB ECX,2
loop_full:
      MOVDQA XMM0,[EAX]
      MOVDQA XMM2,XMM0
      MOVDQA XMM1,[EBX]
      PUNPCKLDQ XMM2,XMM1
      MOVDQA [EDX],XMM2
      PUNPCKHDQ XMM0,XMM1
      MOVDQA [EDX+16],XMM0
      ADD EAX,16
      ADD EBX,16
      ADD EDX,32
      SUB ECX,4
      JG loop_full
      ADD ECX,2
      TEST ECX, ECX
      JLE done
      MOVDQA XMM0,[EAX]
      MOVDQA XMM1,[EBX]
      PUNPCKLDQ XMM0,XMM1
      MOVDQA [EDX],XMM0
done:
    }
  return true;
#endif // !KDU_NO_SSE
  return false;
}

#endif // MSVC_DWT_MMX_LOCAL_H
