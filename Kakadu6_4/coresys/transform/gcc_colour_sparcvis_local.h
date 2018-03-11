/*****************************************************************************/
// File: gcc_colour_sparcvis_local.h [scope = CORESYS/TRANSFORMS]
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
   Provides implementations of the colour transformations -- both reversible
(RCT) and irreversible (ICT = RGB to YCbCr) -- exploiting the UltraSPARC
processor's VIS SIMD instructions.
******************************************************************************/
#ifndef GCC_COLOUR_SPARCVIS_LOCAL_H
#define GCC_COLOUR_SPARCVIS_LOCAL_H

/* ========================================================================= */
/*                          Convenient Definitions                           */
/* ========================================================================= */

typedef double kdvis_d64; // Use for 64-bit VIS registers

union kdvis_4x16 {
    kdvis_d64 d64;
    kdu_int16 v[4]; // Most significant word has idx 0: SPARC is big-endian
  };

typedef float kdvis_f32; // Use for 32-bit VIS registers

union kdvis_4x8 {
    kdvis_f32 f32;
    kdu_byte v[4]; // Must significant byte has idx 0; SPARC is big-endian
  };


/* ========================================================================= */
/*                        Now for the VIS functions                          */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                         simd_ict                                   */
/*****************************************************************************/

inline bool
  simd_ict(kdu_int16 *src1, kdu_int16 *src2, kdu_int16 *src3, int samples)
{
  if (samples <= 0)
    return true;
  if (!kdu_sparcvis_exists)
    return false;

  kdvis_4x16 q_alphaR, q_alphaB, q_alphaG, q_CBfact, q_CRfact;
  q_alphaR.v[0] = q_alphaR.v[1] = q_alphaR.v[2] = q_alphaR.v[3] =
    (kdu_int16)(0.299 * (1<<16));
  q_alphaB.v[0] = q_alphaB.v[1] = q_alphaB.v[2] = q_alphaB.v[3] =
    (kdu_int16)(0.114 * (1<<16));
  q_alphaG.v[0] = q_alphaG.v[1] = q_alphaG.v[2] = q_alphaG.v[3] =
    q_alphaR.v[0] + q_alphaB.v[0]; // Actual factor is 1-alphaR-alphaB
  q_CBfact.v[0] = q_CBfact.v[1] = q_CBfact.v[2] = q_CBfact.v[3] =
    (kdu_int16)(0.4356659 * (1<<16)); // Actual value is 1-0.4356659
  q_CRfact.v[0] = q_CRfact.v[1] = q_CRfact.v[2] = q_CRfact.v[3] =
    (kdu_int16)(0.2867332 * (1<<16)); // Actual value is 1-0.2867332
  register kdvis_d64 alphaR  = q_alphaR.d64;
  register kdvis_d64 alphaB  = q_alphaB.d64;
  register kdvis_d64 alphaG  = q_alphaG.d64;
  register kdvis_d64 CBfact = q_CBfact.d64;
  register kdvis_d64 CRfact = q_CRfact.d64;
  register kdvis_d64 *sp1 = (kdvis_d64 *) src1;
  register kdvis_d64 *sp2 = (kdvis_d64 *) src2;
  register kdvis_d64 *sp3 = (kdvis_d64 *) src3;
  register kdvis_d64 y, cr, cb, red, green, blue, hi1, hi2, lo1, lo2;

  int quads = (samples+3)>>2;
  while (quads--)
    {
      red   = *sp1;
      blue  = *sp3;
      green = *sp2;

      // Form the red and blue contributions to Y
      asm("fmul8sux16 %1, %2, %0": "=e"(hi1):  "e"(red),   "e"(alphaR));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo1):  "e"(red),   "e"(alphaR));
      asm("fmul8sux16 %1, %2, %0": "=e"(hi2):  "e"(blue),  "e"(alphaB));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo2):  "e"(blue),  "e"(alphaB));
      asm("fpadd16 %1, %2, %0":    "=e"(y):    "e"(hi1),   "e"(lo1));

      // Form the green contribution to Y (factor is 1-alphaG)
      asm("fmul8sux16 %1, %2, %0": "=e"(hi1):  "e"(green), "e"(alphaG));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo1):  "e"(green), "e"(alphaG));

      // Accumulate and write out y
      asm("fpadd16 %1, %2, %0":    "=e"(y):    "e"(y),     "e"(hi2));
      asm("fpadd16 %1, %2, %0":    "=e"(y):    "e"(y),     "e"(lo2));
      asm("fpsub16 %1, %2, %0":    "=e"(y):    "e"(y),     "e"(hi1));
      asm("fpsub16 %1, %2, %0":    "=e"(y):    "e"(y),     "e"(lo1));
      asm("fpadd16 %1, %2, %0":    "=e"(y):    "e"(y),     "e"(green));
      *(sp1++) = y;

      // Form (red-Y)*(1-CRfact) in cr and write it out
      asm("fpsub16 %1, %2, %0":    "=e"(cr):   "e"(red),   "e"(y));
      asm("fmul8sux16 %1, %2, %0": "=e"(hi1):  "e"(cr),    "e"(CRfact));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo1):  "e"(cr),    "e"(CRfact));
      asm("fpsub16 %1, %2, %0":    "=e"(cr):   "e"(cr),    "e"(hi1));
      asm("fpsub16 %1, %2, %0":    "=e"(cr):   "e"(cr),    "e"(lo1));
      *(sp3++) = cr;

      // Form (blue-Y)*(1-CBfact) in cb and write it out
      asm("fpsub16 %1, %2, %0":    "=e"(cb):   "e"(blue),   "e"(y));
      asm("fmul8sux16 %1, %2, %0": "=e"(hi2):  "e"(cb),    "e"(CBfact));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo2):  "e"(cb),    "e"(CBfact));
      asm("fpsub16 %1, %2, %0":    "=e"(cb):   "e"(cb),    "e"(hi2));
      asm("fpsub16 %1, %2, %0":    "=e"(cb):   "e"(cb),    "e"(lo2));
      *(sp2++) = cb;
    }
  return true;
}

/*****************************************************************************/
/* INLINE                     simd_inverse_ict                               */
/*****************************************************************************/

inline bool
  simd_inverse_ict(kdu_int16 *src1, kdu_int16 *src2, kdu_int16 *src3,
                   int samples)
{
  if (samples <= 0)
    return true;
  if (!kdu_sparcvis_exists)
    return false;

  kdvis_4x16 q_CRfactR, q_CBfactB, q_CRfactG, q_CBfactG;
  q_CRfactR.v[0] = q_CRfactR.v[1] = q_CRfactR.v[2] = q_CRfactR.v[3] =
    (kdu_int16)(0.402 * (1<<16)); // Actual factor is 1.402
  q_CBfactB.v[0] = q_CBfactB.v[1] = q_CBfactB.v[2] = q_CBfactB.v[3] =
    (kdu_int16)(-0.228 * (1<<16)); // Actual factor is 1.772
  q_CRfactG.v[0] = q_CRfactG.v[1] = q_CRfactG.v[2] = q_CRfactG.v[3] =
    (kdu_int16)(0.285864 * (1<<16)); // Actual factor is -0.714136
  q_CBfactG.v[0] = q_CBfactG.v[1] = q_CBfactG.v[2] = q_CBfactG.v[3] =
    (kdu_int16)(-0.344136 * (1<<16)); // Actual factor is -0.344136
  register kdvis_d64 CR_R = q_CRfactR.d64;
  register kdvis_d64 CB_B = q_CBfactB.d64;
  register kdvis_d64 CR_G = q_CRfactG.d64;
  register kdvis_d64 CB_G = q_CBfactG.d64;
  register kdvis_d64 *sp1 = (kdvis_d64 *) src1;
  register kdvis_d64 *sp2 = (kdvis_d64 *) src2;
  register kdvis_d64 *sp3 = (kdvis_d64 *) src3;
  register kdvis_d64 y, cr, cb, red, green, blue, hi, lo;

  int quads = (samples+3)>>2;
  while (quads--)
    {
      cr = *sp3; // Load Cr chrominance channel
      cb = *sp2;
      y  = *sp1; // Load luminance

      // Form 1.402 (1 + CR_R) times `cr' in `red'
      asm("fmul8sux16 %1, %2, %0": "=e"(hi):  "e"(cr),  "e"(CR_R));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo):  "e"(cr),  "e"(CR_R));
      asm("fpadd16 %1, %2, %0":    "=e"(red): "e"(hi),  "e"(cr));
      asm("fpadd16 %1, %2, %0":    "=e"(red): "e"(red), "e"(lo));

      // Add in luminance to get final red value, and save
      asm("fpadd16 %1, %2, %0":    "=e"(red): "e"(red), "e"(y));
      *(sp1++) = red;

      // Form 1.772 (2 + CB_B) times `cb' in `blue'
      asm("fmul8sux16 %1, %2, %0": "=e"(hi)  : "e"(cb),   "e"(CB_B));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo)  : "e"(cb),   "e"(CB_B));
      asm("fpadd16 %1, %2, %0":    "=e"(blue): "e"(cb),   "e"(cb));
      asm("fpadd16 %1, %2, %0":    "=e"(blue): "e"(blue), "e"(hi));
      asm("fpadd16 %1, %2, %0":    "=e"(blue): "e"(blue), "e"(lo));

      // Add in luminance to get final blue value, and save
      asm("fpadd16 %1, %2, %0":    "=e"(blue): "e"(blue), "e"(y));
      *(sp3++) = blue;

      // Form -0.714136 (-1 + CR_G) * `cr' in `green'
      asm("fmul8sux16 %1, %2, %0": "=e"(hi)   : "e"(cr),    "e"(CR_G));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo)   : "e"(cr),    "e"(CR_G));
      asm("fpsub16 %1, %2, %0":    "=e"(green): "e"(hi),    "e"(cr));
      asm("fpadd16 %1, %2, %0":    "=e"(green): "e"(green), "e"(lo));

      // Add -0.344136 (CB_G) * `cb' + y to `green' and save
      asm("fmul8sux16 %1, %2, %0": "=e"(hi)   : "e"(cb),    "e"(CB_G));
      asm("fpadd16 %1, %2, %0":    "=e"(green): "e"(green), "e"(y));
      asm("fmul8ulx16 %1, %2, %0": "=e"(lo)   : "e"(cb),    "e"(CB_G));
      asm("fpadd16 %1, %2, %0":    "=e"(green): "e"(green), "e"(hi));
      asm("fpadd16 %1, %2, %0":    "=e"(green): "e"(green), "e"(lo));
      *(sp2++) = green;
    }
  return true;
}

/*****************************************************************************/
/* INLINE                           simd_rct                                 */
/*****************************************************************************/

inline bool
  simd_rct(kdu_int16 *src1, kdu_int16 *src2, kdu_int16 *src3, int samples)
{
  if (samples <= 0)
    return true;
  if (!kdu_sparcvis_exists)
    return false;

  kdvis_4x16 q_mask;
  q_mask.v[0] = q_mask.v[1] = q_mask.v[2] = q_mask.v[3] = 0xFFFC;
  register kdvis_d64 mask_lsbs = q_mask.d64;
  kdvis_4x8 q_scale;
  q_scale.v[0] = q_scale.v[1] = q_scale.v[2] = q_scale.v[3] = 0x40;
  register kdvis_f32 scale = q_scale.f32;
  register kdvis_d64 *sp1 = (kdvis_d64 *) src1;
  register kdvis_d64 *sp2 = (kdvis_d64 *) src2;
  register kdvis_d64 *sp3 = (kdvis_d64 *) src3;
  register kdvis_d64 y, db, dr, red, green, blue;

  int quads = (samples+3)>>2;
  while (quads--)
    {
      green = *sp2;
      blue  = *sp3;
      red   = *sp1;

      // Form y = floor((red+2*green+blue) / 4) and save
      asm("fpadd16 %1, %2, %0" :   "=e"(y):  "e"(green), "e"(green));
      asm("fpadd16 %1, %2, %0" :   "=e"(y):  "e"(y),     "e"(blue));
      asm("fpadd16 %1, %2, %0" :   "=e"(y):  "e"(y),     "e"(red));
      asm("fand %1, %2, %0"    :   "=e"(y):  "e"(y),     "e"(mask_lsbs));
      asm("fmul8x16 %1, %2, %0":   "=e"(y):  "f"(scale), "e"(y));
      *(sp1++) = y;

      // Subtract green from blue to form db
      asm("fpsub16 %1, %2, %0" :   "=e"(db): "e"(blue),  "e"(green));
      *(sp2++) = db;

      // Subtract green from red to form dr
      asm("fpsub16 %1, %2, %0" :   "=e"(dr): "e"(red),   "e"(green));
      *(sp3++) = dr;
    }
  return true;
}

/*****************************************************************************/
/* INLINE                       simd_inverse_rct                             */
/*****************************************************************************/

inline bool
  simd_inverse_rct(kdu_int16 *src1, kdu_int16 *src2, kdu_int16 *src3,
                   int samples)
{
  if (samples <= 0)
    return true;
  if (!kdu_altivec_exists)
    return false;

  kdvis_4x16 q_mask;
  q_mask.v[0] = q_mask.v[1] = q_mask.v[2] = q_mask.v[3] = 0xFFFC;
  register kdvis_d64 mask_lsbs = q_mask.d64;
  kdvis_4x8 q_scale;
  q_scale.v[0] = q_scale.v[1] = q_scale.v[2] = q_scale.v[3] = 0x40;
  register kdvis_f32 scale = q_scale.f32;
  register kdvis_d64 *sp1 = (kdvis_d64 *) src1;
  register kdvis_d64 *sp2 = (kdvis_d64 *) src2;
  register kdvis_d64 *sp3 = (kdvis_d64 *) src3;
  register kdvis_d64 y, db, dr, red, green, blue;

  int quads = (samples+3)>>2;
  while (quads--)
    {
      db = *sp2;
      dr = *sp3;
      y  = *sp1;

      // Form floor((`db'+`dr') / 4) in `green'
      asm("fpadd16 %1, %2, %0" :    "=e"(green): "e"(db),    "e"(dr));
      asm("fand %1, %2, %0"    :    "=e"(green): "e"(green), "e"(mask_lsbs));
      asm("fmul8x16 %1, %2, %0":    "=e"(green): "f"(scale), "e"(green));

      // Negate and add in luminance, then save green channel
      asm("fpsub16 %1, %2, %0" :    "=e"(green): "e"(y),     "e"(green));
      *(sp2++) = green;

      // Form red channel and save
      asm("fpadd16 %1, %2, %0" :    "=e"(red): "e"(green),   "e"(dr));
      *(sp1++) = red;

      // Form blue channel and save
      asm("fpadd16 %1, %2, %0" :    "=e"(blue): "e"(green),   "e"(db));
      *(sp3++) = blue;
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

#endif // GCC_COLOUR_SPARCVIS_LOCAL_H
