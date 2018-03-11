/*****************************************************************************/
// File: gcc_coder_mmx_local.h [scope = CORESYS/CODING]
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
   Provides SSE implementations to accelerate the conversion and transfer of
data between the block coder and DWT line-based processing engine.
******************************************************************************/
#ifndef GCC_CODER_MMX_LOCAL_H
#define GCC_CODER_MMX_LOCAL_H

/* ========================================================================= */
/*                      Define Convenient MMX Macros                        */
/* ========================================================================= */

#define MOVD_read(_dst,_src) __asm__ volatile \
  ("movd %0, %%" #_dst : : "m" (_src))

#define MOVDQA_read(_dst,_src) __asm__ volatile \
  ("movdqa %0, %%" #_dst : : "m" (_src))
#define MOVDQA_xfer(_dst,_src) __asm__ volatile \
  ("movdqa %%" #_src ", %%" #_dst : : )
#define MOVDQA_write(_dst,_src) __asm__ volatile \
  ("movdqa %%" #_src ", %0" : "=m" (_dst) :)

#define MOVDQU_read(_dst,_src) __asm__ volatile \
  ("movdqu %0, %%" #_dst : : "m" (_src))
#define MOVUPS_read(_dst,_src) __asm__ volatile \
  ("movups %0, %%" #_dst : : "m" (_src))

#define MMX_OP_reg(_op,_dst,_src) __asm__ volatile \
  (#_op " %%" #_src ", %%" #_dst : : )
#define MMX_OP_mem(_op,_dst,_src) __asm__ volatile \
  (#_op " %0, %%" #_dst : : "m" (_src))
#define MMX_OP_imm(_op,_dst,_imm) __asm__ volatile \
  (#_op " %0, %%" #_dst : : "i" (_imm))

#define STMXCSR(_dst) __asm__ volatile ("stmxcsr %0" : "=m" (_dst) :)
#define LDMXCSR(_src) __asm__ volatile ("ldmxcsr %0" :  : "m" (_src))

#define PCMPEQD_reg(_tgt,_in) MMX_OP_reg(pcmpeqd,_tgt,_in)
#define PCMPGTD_reg(_tgt,_in) MMX_OP_reg(pcmpgtd,_tgt,_in)

#define PSRAD_imm(_tgt,_val) MMX_OP_imm(psrad,_tgt,_val)
#define PSRAD_reg(_tgt,_val) MMX_OP_reg(psrad,_tgt,_val)

#define PSRLD_imm(_tgt,_val) MMX_OP_imm(psrld,_tgt,_val)
#define PSRLD_reg(_tgt,_val) MMX_OP_reg(psrld,_tgt,_val)

#define PSLLD_imm(_tgt,_val) MMX_OP_imm(pslld,_tgt,_val)
#define PSLLD_reg(_tgt,_val) MMX_OP_reg(pslld,_tgt,_val)

#define POR_reg(_tgt,_in) MMX_OP_reg(por,_tgt,_in)
#define PXOR_reg(_tgt,_in) MMX_OP_reg(pxor,_tgt,_in)
#define PAND_reg(_tgt,_in) MMX_OP_reg(pand,_tgt,_in)
#define PADDD_reg(_tgt,_in) MMX_OP_reg(paddd,_tgt,_in)

#define PACKSSDW_reg(_tgt,_in) MMX_OP_reg(packssdw,_tgt,_in)

#define CVTDQ2PS_reg(_tgt,_in) MMX_OP_reg(cvtdq2ps,_tgt,_in)
#define CVTPS2DQ_reg(_tgt,_in) MMX_OP_reg(cvtps2dq,_tgt,_in)
#define MULPS_reg(_tgt,_in) MMX_OP_reg(mulps,_tgt,_in)


/* ========================================================================= */
/*                        Now for the MMX functions                          */
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
      int downshift = 31 - K_max;
      MOVD_read(XMM0,downshift);
      PCMPEQD_reg(XMM2,XMM2); // Fill XMM2 with 1's
      PCMPEQD_reg(XMM1,XMM1); // Also fill XMM1 with 1's
      PSRLD_imm(XMM1,31);     // Set each dword in XMM1 equal to 1
      MOVD_read(XMM3,K_max);  // Temporary value
      PSLLD_reg(XMM2,XMM3);   // Set XMM2 dwords to have 1+downshift 1's in MSBs
      POR_reg(XMM1,XMM2);     // XMM1 now holds the amount we have to add after
             // inverting the bits of a downshifted sign-mag quantity which
             // was negative, to restore the correct 2's complement value.
      for (; height > 0; height--, dst_refs++)
        {
          dst = *dst_refs + dst_offset;
          for (register int c=width; c > 0; c-=8, src+=8, dst+=8)
            {
              MOVDQA_read(XMM4,*src);
              PXOR_reg(XMM5,XMM5);
              PCMPGTD_reg(XMM5,XMM4); // Fills dwords in XMM5 with 1s if XMM4 dwords < 0
              PXOR_reg(XMM4,XMM5);
              PSRAD_reg(XMM4,XMM0);
              PAND_reg(XMM5,XMM1);    // Leave the bits we need to add
              PADDD_reg(XMM4,XMM5);   // Finish conversion from sign-mag to 2's comp

              MOVDQA_read(XMM6,*(src+4));
              PXOR_reg(XMM7,XMM7);
              PCMPGTD_reg(XMM7,XMM6); // Fills dwords in XMM7 with 1s if XMM6 dwords < 0
              PXOR_reg(XMM6,XMM7);
              PSRAD_reg(XMM6,XMM0);   // Apply the downshift value
              PAND_reg(XMM7,XMM1);    // Leave the bits we need to add
              PADDD_reg(XMM6,XMM7);   // Finish conversion from sign-mag to 2's comp

              PACKSSDW_reg(XMM4,XMM6); // Get packed 8 words
              MOVDQA_write(*dst,XMM4); // Write result
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
      float vec_scale[4] = {fscale,fscale,fscale,fscale};
      int mxcsr_orig, mxcsr_cur;
      STMXCSR(mxcsr_orig);
      mxcsr_cur = mxcsr_orig & ~(3<<13); // Reset rounding control bits
      LDMXCSR(mxcsr_cur);
      register kdu_sample16 *dst;
      MOVUPS_read(XMM0,*vec_scale);
      PCMPEQD_reg(XMM2,XMM2);  // Fill XMM2 with 1's
      PCMPEQD_reg(XMM1,XMM1);  // Also fill XMM1 with 1's
      PSRLD_imm(XMM1,31);      // Set each dword in XMM1 equal to 1
      PSLLD_imm(XMM2,31);      // Set each dword in XMM2 equal to 0x80000000
      POR_reg(XMM1,XMM2);      // XMM1 now holds 0x80000001
      for (; height > 0; height--, dst_refs++)
        {
          dst = *dst_refs + dst_offset;
          for (register int c=width; c > 0; c-=8, src+=8, dst+=8)
            {
              MOVDQA_read(XMM4,*src);
              PXOR_reg(XMM5,XMM5);
              PCMPGTD_reg(XMM5,XMM4);  // Fills dwords in XMM5 with 1s if XMM4 dwords < 0
              PXOR_reg(XMM4,XMM5);
              PAND_reg(XMM5,XMM1);     // AND off all but MSB and LSB in each dword
              PADDD_reg(XMM4,XMM5);    // Finish conversion from sign-mag to 2's comp
              CVTDQ2PS_reg(XMM4,XMM4); // Convert dwords to single precision floats
              MULPS_reg(XMM4,XMM0);    // Scale by adjusted quantization step size
              CVTPS2DQ_reg(XMM4,XMM4);

              MOVDQA_read(XMM6,*(src+4));
              PXOR_reg(XMM7,XMM7);
              PCMPGTD_reg(XMM7,XMM6);  // Fills dwords in XMM7 with 1s if XMM6 dwords < 0
              PXOR_reg(XMM6,XMM7);
              PAND_reg(XMM7,XMM1);     // AND off all but MSB and LSB in each word
              PADDD_reg(XMM6,XMM7);    // Finish conversion from sign-mag to 2's comp
              CVTDQ2PS_reg(XMM6,XMM6); // Convert dwords to single precision floats
              MULPS_reg(XMM6,XMM0);    // Scale by adjusted quantization step size
              CVTPS2DQ_reg(XMM6,XMM6);

              PACKSSDW_reg(XMM4,XMM6); // Get packed 8 words
              MOVDQA_write(*dst,XMM4);  // Write result
            }
        }
      LDMXCSR(mxcsr_orig);// Restore rounding control bits
    }
  return true;
}

#endif // GCC_CODER_MMX_LOCAL_H
