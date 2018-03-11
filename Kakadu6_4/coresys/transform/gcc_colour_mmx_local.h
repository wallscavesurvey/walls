/*****************************************************************************/
// File: gcc_colour_mmx_local.h [scope = CORESYS/TRANSFORMS]
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
MMX/SSE/SSE2 instructions, realized through GCC inline assembly.  You can
use this implementation for both 32-bit and 64-bit GCC builds.
******************************************************************************/

#ifndef GCC_COLOUR_MMX_LOCAL_H
#define GCC_COLOUR_MMX_LOCAL_H

static kdu_int16 *create_vector_constants(); // Forward declaration
static kdu_int16 _vec_store[64+7];
static kdu_int16 *vec_aligned = create_vector_constants();
static kdu_int16 *vec_alphaR  = vec_aligned+0;
static kdu_int16 *vec_alphaB  = vec_aligned+8;
static kdu_int16 *vec_CBfact  = vec_aligned+16;
static kdu_int16 *vec_CRfact  = vec_aligned+24;
static kdu_int16 *vec_CRfactR = vec_aligned+32;
static kdu_int16 *vec_CBfactB = vec_aligned+40;
static kdu_int16 *vec_CRfactG = vec_aligned+48;
static kdu_int16 *vec_CBfactG = vec_aligned+56;

static kdu_int16 *
  create_vector_constants()
{
  kdu_int16 alphaR = (kdu_int16)(0.299 * (1<<16));
  kdu_int16 alphaB = (kdu_int16)(0.114 * (1<<16));
  kdu_int16 CBfact = (kdu_int16)(0.4356659 * (1<<16)); // Actual value is 1-0.4356659
  kdu_int16 CRfact = (kdu_int16)(0.2867332 * (1<<16)); // Actual value is 1-0.2867332
  
  kdu_int16 CRfactR = (kdu_int16)(0.402 * (1<<16)); // Actual factor is 1.402
  kdu_int16 CBfactB = (kdu_int16)(-0.228 * (1<<16)); // Actual factor is 1.772
  kdu_int16 CRfactG = (kdu_int16)(0.285864 * (1<<16)); // Actual factor is -0.714136
  kdu_int16 CBfactG = (kdu_int16)(-0.344136 * (1<<16)); // Actual factor is -0.344136

  kdu_int16 *buf = _vec_store;
  int k, alignment = _addr_to_kdu_int32(buf) & 0x0F;
  assert((alignment & 1) == 0);
  alignment >>= 1; // Convert to words
  buf += ((8-alignment) & 7);
  
  for (k=0; k < 8; k++)
    {
      buf[k] = alphaR;      buf[k+8] = alphaB;
      buf[k+16] = CBfact;   buf[k+24] = CRfact;
      buf[k+32] = CRfactR;  buf[k+40] = CBfactB;
      buf[k+48] = CRfactG;  buf[k+56] = CBfactG;
    }
  return buf;
}


/* ========================================================================= */
/*                      Define Convenient MMX Macros                        */
/* ========================================================================= */

#define MOVQ_read(_dst,_src) __asm__ volatile \
  ("movq %0, %%" #_dst : : "m" (_src))
#define MOVQ_xfer(_dst,_src) __asm__ volatile \
  ("movq %%" #_src ", %%" #_dst : : )
#define MOVQ_write(_dst,_src) __asm__ volatile \
  ("movq %%" #_src ", %0" : "=m" (_dst) :)

#define MOVDQA_read(_dst,_src) __asm__ volatile \
  ("movdqa %0, %%" #_dst : : "m" (_src))
#define MOVDQA_xfer(_dst,_src) __asm__ volatile \
  ("movdqa %%" #_src ", %%" #_dst : : )
#define MOVDQA_write(_dst,_src) __asm__ volatile \
  ("movdqa %%" #_src ", %0" : "=m" (_dst) :)

#define MOVDQU_read(_dst,_src) __asm__ volatile \
  ("movdqu %0, %%" #_dst : : "m" (_src))
#define MOVDQU_xfer(_dst,_src) __asm__ volatile \
  ("movdqu %%" #_src ", %%" #_dst : : )
#define MOVDQU_write(_dst,_src) __asm__ volatile \
  ("movdqu %%" #_src ", %0" : "=m" (_dst) :)

#define MMX_OP_reg(_op,_dst,_src) __asm__ volatile \
  (#_op " %%" #_src ", %%" #_dst : : )
#define MMX_OP_mem(_op,_dst,_src) __asm__ volatile \
  (#_op " %0, %%" #_dst : : "m" (_src))
#define MMX_OP_imm(_op,_dst,_imm) __asm__ volatile \
  (#_op " %0, %%" #_dst : : "i" (_imm))

#define EMMS() __asm__ __volatile__("emms")

#define PADDW_reg(_tgt,_in) MMX_OP_reg(paddw,_tgt,_in)
#define PADDW_mem(_tgt,_in) MMX_OP_mem(paddw,_tgt,_in)

#define PSUBW_reg(_tgt,_in) MMX_OP_reg(psubw,_tgt,_in)
#define PSUBW_mem(_tgt,_in) MMX_OP_mem(psubw,_tgt,_in)

#define PADDSW_reg(_tgt,_in) MMX_OP_reg(paddsw,_tgt,_in)
#define PADDSW_mem(_tgt,_in) MMX_OP_mem(paddsw,_tgt,_in)

#define PSUBSW_reg(_tgt,_in) MMX_OP_reg(psubsw,_tgt,_in)
#define PSUBSW_mem(_tgt,_in) MMX_OP_mem(psubsw,_tgt,_in)

#define PMULHW_reg(_tgt,_in) MMX_OP_reg(pmulhw,_tgt,_in)
#define PMULHW_mem(_tgt,_in) MMX_OP_mem(pmulhw,_tgt,_in)

#define PSRAW_imm(_tgt,_val) MMX_OP_imm(psraw,_tgt,_val)
#define PSRLW_imm(_tgt,_val) MMX_OP_imm(psrlw,_tgt,_val)

#define PCMPEQW_reg(_tgt,_in) MMX_OP_reg(pcmpeqw,_tgt,_in)


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
  if (kdu_mmx_level < 1)
    return false;

  assert(vec_CRfact==(vec_CBfact+8)); // Assumed for the code below to work
  register kdu_int16 *fact = vec_alphaR;
  MOVQ_read(MM5,*fact);
  fact = vec_alphaB;
  MOVQ_read(MM6,*fact);
  PCMPEQW_reg(MM7,MM7);      // Fill MM7 with 1's
  PSRLW_imm(MM7,15);         // Leaves each word in MM7 equal to 1
  fact = vec_CBfact;
  for (register int c=0; c < samples; c+=4)
    {
      MOVQ_read(MM1,*(src1+c)); // Load red; we will need to use this multiple times
      MOVQ_read(MM2,*(src3+c)); // Load blue; we will need to use this multiple times
      MOVQ_xfer(MM3,MM7);   // Load a quad-word of 1's
      PADDW_reg(MM3,MM3);   // Double to get a quad-word of 2's
      MOVQ_xfer(MM0,MM3);   // Move the pre-offset of 2 into MM0
      PADDSW_reg(MM0,MM1);  // Form the pre-offset red channel in MM0
      PMULHW_reg(MM0,MM5);  // Form red contribution to Y in MM0
      PADDW_reg(MM3,MM3);   // 4 is a good pre-offset for multiplication by alphaB
      PADDSW_reg(MM3,MM2);  // Form the pre-offset blue channel in MM3
      PMULHW_reg(MM3,MM6);  // Form blue contribution to Y in MM0
      PADDSW_reg(MM0,MM3);  // Add blue contribution to red contribution
      MOVQ_xfer(MM3,MM5);
      PADDW_reg(MM3,MM6);   // Gets alphaR+alphaB to MM3
      MOVQ_read(MM4,*(src2+c)); // Load green channel to MM4; we only need it once
      PADDSW_reg(MM4,MM7);  // Pre-offset the green channel by 1
      PMULHW_reg(MM3,MM4);  // Creates green * (alphaR+alphaB) in MM3
      PSUBSW_reg(MM4,MM7);  // Remove the pre-offset from the green channel
      PSUBSW_reg(MM4,MM3);  // Forms green * (1-alphaR-alphaB) in MM4
      PADDSW_reg(MM0,MM4);  // Forms luminance in MM0
      MOVQ_write(*(src1+c),MM0);// Write out luminance
      PSUBSW_reg(MM1,MM0);  // Forms R-Y in MM1
      PSUBSW_reg(MM2,MM0);  // Forms B-Y in MM2
      MOVQ_xfer(MM0,MM1);   // Copy R-Y to MM0
      PADDSW_reg(MM1,MM7);  // Add a pre-offset of 1
      PADDSW_reg(MM1,MM7);  // Make the pre-offset 2
      PMULHW_mem(MM1,*(fact+8)); // Multiply by vec_CRfact
      PSUBSW_reg(MM0,MM1);  // Forms (1-CRfact)*(R-Y) in MM0
      MOVQ_write(*(src3+c),MM0);// Write out the CR channel
      MOVQ_xfer(MM0,MM2);   // Copy B-Y to MM0
      PADDSW_reg(MM2,MM7);  // Add a pre-offset of 1
      PMULHW_mem(MM2,*fact); // Load vec_CBfact
      PSUBSW_reg(MM0,MM2);  // Forms (1-CBfact)*(B-Y) in MM0
      MOVQ_write(*(src2+c),MM0);// Write out the CB channel
    }
  EMMS();
  return true;
}

/*****************************************************************************/
/* INLINE                      simd_inverse_ict                              */
/*****************************************************************************/

inline bool
  simd_inverse_ict(register kdu_int16 *src1,
                   register kdu_int16 *src2,
                   register kdu_int16 *src3, register int samples)
{
  if (kdu_mmx_level < 1)
    return false;

  assert(vec_CBfactG==(vec_CRfactG+8)); // Assumed for the code below to work

#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // 128-bit implementation, using SSE/SSE2 instructions
      PCMPEQW_reg(XMM4,XMM4);     // Fill XMM4 with 1's
      PSRLW_imm(XMM4,15);         // Set each word in XMM4 equal to 1
      MOVDQA_xfer(XMM5,XMM4);
      PADDW_reg(XMM5,XMM5);       // Set each word in XMM5 equal to 2
      register kdu_int16 *fact = vec_CRfactR;
      MOVDQA_read(XMM6,*fact);
      fact = vec_CBfactB;
      MOVDQA_read(XMM7,*fact);
      fact = vec_CRfactG;
      for (register int c=0; c < samples; c+=8)
        {
          MOVDQA_read(XMM0,*(src1+c));  // Load luminance (Y)
          MOVDQA_read(XMM1,*(src3+c));  // Load chrominance (Cr)
          MOVDQA_xfer(XMM2,XMM1);       // Prepare to form Red output in XMM2
          PADDSW_reg(XMM2,XMM4);        // +1 here approximates add 2^15 then divide by 2^16
          PMULHW_reg(XMM2,XMM6);        // Multiply by 0.402*2^16 (CRfactR) & divide by 2^16
          PADDSW_reg(XMM2,XMM1);        // Add Cr again to make factor equivalent to 1.402
          PADDSW_reg(XMM2,XMM0);        // Add in luminance to get Red
          MOVDQA_write(*(src1+c),XMM2); // Save Red channel
          MOVDQA_xfer(XMM2,XMM1);       // Prepare to form Cr*(-0.714136) in XMM2
          PADDSW_reg(XMM2,XMM5);        // +2 here approximates add 2^15 then divide by 2^16
          PMULHW_mem(XMM2,*fact);       // Multiply by 0.285864*2^16 (CRfactG) & divide by 2^16
          PSUBSW_reg(XMM2,XMM1);        // Subtract Cr leaves us with the desired result
          PADDSW_reg(XMM2,XMM0);        // Add Y to scaled Cr forms most of Green in MM2
          MOVDQA_read(XMM1,*(src2+c));  // Load chrominance (Cb)
          MOVDQA_xfer(XMM3,XMM1);       // Prepare to form Blue output in XMM3
          PSUBSW_reg(XMM3,XMM5);        // +2 here approximates add 2^15 then divide by 2^16
          PMULHW_reg(XMM3,XMM7);        // Multiply by -0.228*2^16 (CBfactB) & divide by 2^16
          PADDSW_reg(XMM3,XMM1);        // Gets 0.772*Cb to XMM3
          PADDSW_reg(XMM3,XMM1);        // Gets 1.772*Cb to XMM3
          PADDSW_reg(XMM3,XMM0);        // Add in luminance to get Blue
          MOVDQA_write(*(src3+c),XMM3); // Save Blue channel
          PSUBSW_reg(XMM1,XMM5);        // +2 here approximates add 2^15 then divide by 2^16
          PMULHW_mem(XMM1,*(fact+8));   // Multiply by -0.344136*2^16 (CBfactG) and divide by 2^16
          PADDSW_reg(XMM2,XMM1);        // Completes the Green channel in XMM2
          MOVDQA_write(*(src2+c),XMM2);
        }
    }
  else
#endif // !KDU_NO_SSE
    { // 64-bit implementation, using MMX instructions only
      PCMPEQW_reg(MM4,MM4);     // Fill MM4 with 1's
      PSRLW_imm(MM4,15);        // Set each word in MM4 equal to 1
      MOVQ_xfer(MM5,MM4);
      PADDW_reg(MM5,MM5);       // Set each word in MM5 equal to 2
      register kdu_int16 *fact = vec_CRfactR;
      MOVQ_read(MM6,*fact);
      fact = vec_CBfactB;
      MOVQ_read(MM7,*fact);
      fact = vec_CRfactG;
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM0,*(src1+c)); // Load luminance (Y)
          MOVQ_read(MM1,*(src3+c)); // Load chrominance (Cr)
          MOVQ_xfer(MM2,MM1);   // Prepare to form Red output in MM2
          PADDSW_reg(MM2,MM4);  // +1 here similar to adding 2^15 before dividing by 2^16
          PMULHW_reg(MM2,MM6);  // Multiply by 0.402*2^16 (CRfactR) and divide by 2^16
          PADDSW_reg(MM2,MM1);  // Add Cr again to make the factor equivalent to 1.402
          PADDSW_reg(MM2,MM0);  // Add in luminance to get Red
          MOVQ_write(*(src1+c),MM2);// Save Red channel
          MOVQ_xfer(MM2,MM1);   // Prepare to form Cr*(-0.714136) in MM2 (will free MM1)
          PADDSW_reg(MM2,MM5);  // +2 here similar to adding 2^15 before dividing by 2^16
          PMULHW_mem(MM2,*fact); // Multiply by 0.285864*2^16 and divide by 2^16
          PSUBSW_reg(MM2,MM1);  // Subtract Cr leaves us with the desired result
          PADDSW_reg(MM2,MM0);  // Add Y to scaled Cr forms most of Green result in MM2
          MOVQ_read(MM1,*(src2+c)); // Load chrominance (Cb)
          MOVQ_xfer(MM3,MM1);   // Prepare to form Blue output in MM3
          PSUBSW_reg(MM3,MM5);  // +2 here similar to adding 2^15 before dividing by 2^16
          PMULHW_reg(MM3,MM7);  // Multiply by -0.228*2^16 (CBfactB) and divide by 2^16
          PADDSW_reg(MM3,MM1);  // Gets 0.772*Cb to MM3
          PADDSW_reg(MM3,MM1);  // Gets 1.772*Cb to MM3
          PADDSW_reg(MM3,MM0);  // Add in luminance to get Blue
          MOVQ_write(*(src3+c),MM3);// Save Blue channel
          PSUBSW_reg(MM1,MM5);  // +2 here similar to adding 2^15 before dividing by 2^16
          PMULHW_mem(MM1,*(fact+8)); // Multiply by -0.344136*2^16 and divide by 2^16
          PADDSW_reg(MM2,MM1); // Completes the Green channel in MM2
          MOVQ_write(*(src2+c),MM2);
        }
      EMMS(); // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                           simd_rct                                 */
/*****************************************************************************/

inline bool
  simd_rct(register kdu_int16 *src1,
           register kdu_int16 *src2,
           register kdu_int16 *src3, register int samples)
{
  if (kdu_mmx_level < 1)
    return false;

  for (register int c=0; c < samples; c+=4)
    {
      MOVQ_read(MM1,*(src1+c)); // Load red (R)
      MOVQ_read(MM2,*(src2+c)); // Load green (G)
      MOVQ_read(MM3,*(src3+c)); // Load blue (B)
      MOVQ_xfer(MM0,MM1);   // Get R to MM0
      PADDSW_reg(MM0,MM3);  // Add B to MM0
      PADDSW_reg(MM0,MM2);
      PADDSW_reg(MM0,MM2);  // Add 2*G to MM0
      PSRAW_imm(MM0,2);     // Forms (R+2*G+B)>>2
      MOVQ_write(*(src1+c),MM0);// Write out Y channel
      PSUBSW_reg(MM3,MM2);  // Subtract G from B
      MOVQ_write(*(src2+c),MM3);// Write out Db channel
      PSUBSW_reg(MM1,MM2);  // Subtract G from R
      MOVQ_write(*(src3+c),MM1);// Write out Dr channel
    }
  EMMS(); // Clear MMX registers for use by FPU
  return true;
}

/*****************************************************************************/
/* INLINE                       simd_inverse_rct                             */
/*****************************************************************************/

inline bool
  simd_inverse_rct(register kdu_int16 *src1,
                   register kdu_int16 *src2,
                   register kdu_int16 *src3, register int samples)
{
  if (kdu_mmx_level < 1)
    return false;

#ifndef KDU_NO_SSE
  if (kdu_mmx_level >= 2)
    { // 128-bit implementation using SSE/SSE2 instructions
      for (register int c=0; c < samples; c+=8)
        {
          MOVDQA_read(XMM1,*(src2+c)); // Load chrominance (Db)
          MOVDQA_read(XMM2,*(src3+c)); // Load chrominance (Dr)
          MOVDQA_xfer(XMM3,XMM1);
          PADDSW_reg(XMM3,XMM2);
          PSRAW_imm(XMM3,2);           // Forms (Db+DR)>>2
          MOVDQA_read(XMM0,*(src1+c)); // Load luminance (Y).
          PSUBSW_reg(XMM0,XMM3);       // Convert Y to Green channel
          MOVDQA_write(*(src2+c),XMM0);
          PADDSW_reg(XMM2,XMM0);       // Convert Dr to Red channel
          MOVDQA_write(*(src1+c),XMM2);
          PADDSW_reg(XMM1,XMM0);       // Convert Db to Blue channel
          MOVDQA_write(*(src3+c),XMM1);
        }
    }
  else
#endif // !KDU_NO_SSE
    { // 64-bit implementation using only MMX instructions
      for (register int c=0; c < samples; c+=4)
        {
          MOVQ_read(MM1,*(src2+c)); // Load chrominance (Db)
          MOVQ_read(MM2,*(src3+c)); // Load chrominance (Dr)
          MOVQ_xfer(MM3,MM1);
          PADDSW_reg(MM3,MM2);
          PSRAW_imm(MM3,2);         // Forms (Db+DR)>>2
          MOVQ_read(MM0,*(src1+c)); // Load luminance (Y).
          PSUBSW_reg(MM0,MM3);      // Convert Y to Green channel
          MOVQ_write(*(src2+c),MM0);
          PADDSW_reg(MM2,MM0);      // Convert Dr to Red channel
          MOVQ_write(*(src1+c),MM2);
          PADDSW_reg(MM1,MM0);      // Convert Db to Blue channel
          MOVQ_write(*(src3+c),MM1);
        }
      EMMS(); // Clear MMX registers for use by FPU
    }
  return true;
}

/*****************************************************************************/
/* INLINE                          simd_ict                                  */
/*****************************************************************************/

inline bool
  simd_ict(float *src1, float *src2, float *src3, int samples)
  /* As above, but for floating point sample values. */
{
  return false;
}

/*****************************************************************************/
/* INLINE                      simd_inverse_ict                              */
/*****************************************************************************/

inline bool
  simd_inverse_ict(float *src1, float *src2, float *src3, int samples)
  /* As above, but for floating point sample values. */
{
  return false;
}

/*****************************************************************************/
/* INLINE                           simd_rct                                 */
/*****************************************************************************/

inline bool
  simd_rct(kdu_int32 *src1, kdu_int32 *src2, kdu_int32 *src3, int samples)
  /* Same as above function, except that it operates on 32-bit integers. */
{
  return false;
}

/*****************************************************************************/
/* INLINE                       simd_inverse_rct                             */
/*****************************************************************************/

inline bool
  simd_inverse_rct(kdu_int32 *src1, kdu_int32 *src2, kdu_int32 *src3,
                   int samples)
  /* Same as above function, except that it operates on 32-bit integers. */
{
  return false;
}

#endif // GCC_COLOUR_MMX_LOCAL_H
