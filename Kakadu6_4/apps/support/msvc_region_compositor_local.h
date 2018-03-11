/*****************************************************************************/
// File: msvc_region_compositor_local.h [scope = CORESYS/TRANSFORMS]
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
MMX in-line instructions, following the MSVC conventions.
******************************************************************************/

#ifndef MSVC_REGION_COMPOSITOR_LOCAL_H
#define MSVC_REGION_COMPOSITOR_LOCAL_H

/* ========================================================================= */
/*                        Now for the MMX functions                          */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                 simd_erase_region (32-bit)                         */
/*****************************************************************************/

inline bool
  simd_erase_region(kdu_uint32 *dst, int height, int width, int row_gap,
                    kdu_uint32 erase)
{
  if ((kdu_mmx_level < 1) || (width < 8))
    return false;
  kdu_uint32 d_erase[2] = { erase, erase };
  for (; height > 0; height--, dst+=row_gap)
    {
      kdu_uint32 *dpp = dst;
      int left = (_addr_to_kdu_int32(dpp) >> 2) & 1;
      int octets = (width-left)>>3;
      int c, right = width - left - (octets<<3);
      if (left)
        *(dpp++) = erase;
      if (octets > 0)
        {
          __asm
            {
              MOV  ECX,octets // Set up counter used for looping
              MOV  EDX,dpp
              MOVQ MM0,d_erase
loop_erase_region:
              MOVQ [EDX],MM0
              MOVQ [EDX+8],MM0
              MOVQ [EDX+16],MM0
              MOVQ [EDX+24],MM0
              ADD EDX,32
              SUB ECX,1
              JNZ loop_erase_region
              MOV dpp,EDX
            }
        }
      for (c=right; c > 0; c--)
        *(dpp++) = erase;
    }

  __asm EMMS
  return true;
}

/*****************************************************************************/
/* INLINE             simd_erase_region (floating point)                     */
/*****************************************************************************/

inline bool
  simd_erase_region(float *dst, int height, int width, int row_gap,
                    float erase[])
{
  return false;
}
  
/*****************************************************************************/
/* INLINE                 simd_copy_region (32-bit)                          */
/*****************************************************************************/

inline bool
  simd_copy_region(kdu_uint32 *dst, kdu_uint32 *src,
                   int height, int width, int dst_row_gap, int src_row_gap)
{
  if ((kdu_mmx_level < 1) || (width < 8))
    return false;
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      kdu_uint32 *dpp = dst;
      kdu_uint32 *spp = src;
      int left = (_addr_to_kdu_int32(dpp) >> 2) & 1;
      int octets = (width-left)>>3;
      int c, right = width - left - (octets<<3);
      if (left)
        *(dpp++) = *(spp++);
      if (octets > 0)
        {
          __asm
            {
              MOV  ECX,octets // Set up counter used for looping
              MOV  EAX,spp
              MOV  EDX,dpp
loop_erase_region:
              MOVQ MM0,[EAX]
              MOVQ [EDX],MM0
              MOVQ MM1,[EAX+8]
              MOVQ [EDX+8],MM1
              MOVQ MM2,[EAX+16]
              MOVQ [EDX+16],MM2
              MOVQ MM3,[EAX+24]
              MOVQ [EDX+24],MM3
              ADD EAX,32
              ADD EDX,32
              SUB ECX,1
              JNZ loop_erase_region
              MOV spp,EAX
              MOV dpp,EDX
            }
        }
      for (c=right; c > 0; c--)
        *(dpp++) = *(spp++);
    }

  __asm EMMS
  return true;
}

/*****************************************************************************/
/* INLINE               simd_copy_region (floating point)                    */
/*****************************************************************************/

inline bool
  simd_copy_region(float *dst, float *src,
                   int height, int width, int dst_row_gap, int src_row_gap)
{
  return false;
}

/*****************************************************************************/
/* INLINE                  simd_blend_region (32-bit)                        */
/*****************************************************************************/

inline bool
  simd_blend_region(kdu_uint32 *dst, kdu_uint32 *src,
                    int height, int width, int dst_row_gap, int src_row_gap)
{
  if ((kdu_mmx_level < 1) || (width < 4))
    return false;
  kdu_int16 *alpha_lut = kdrc_alpha_lut4;
  __asm { // Create mask of 0x00FF in alpha word position of quad-word MM7
      PCMPEQW MM7,MM7 // Creates a mask equal to FFFFFFFFFFFFFFFF
      PSLLQ MM7,56    // Leaves mask equal to    FF00000000000000
      PSRLQ MM7,8     // Leaves mask equal to    00FF000000000000
    }
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      kdu_uint32 *dpp = dst;
      kdu_uint32 *spp = src;
      int left = (_addr_to_kdu_int32(dpp) >> 2) & 1;
      int doublets = (width-left)>>1;
      int right = width - left - (doublets<<1);
      if (left)
        { // Process single non-aligned pixel
          __asm
            {
              MOV  EAX,spp
              MOV  EBX,alpha_lut // Get base address of alpha lut
              MOV  EDX,dpp
              // Load source pels and unpack into words
              PXOR MM0,MM0 // Set MM0 to 0
              MOVD MM1,[EAX]
              PUNPCKLBW MM1,MM0
              // Load target pels and unpack into words
              MOVD MM3,[EDX]
              PUNPCKLBW MM3,MM0
              // Load alpha scale factors
              MOVQ MM5,MM1
              PSRLQ MM5,48
              MOVD ESI,MM5
              MOVQ MM5,[EBX+ESI*8]
              // Change source alpha value to 255 for correct alpha adjustment
              POR MM1,MM7
              // Get `diff' = source-target in MM1
              PSUBW MM1,MM3
              // Scale `diff' by alpha and add into `target'
              PSLLW MM1,2 // Because alpha values scaled by 2^14
              PMULHW MM1,MM5 // Scale by alpha and divide by 2^16
              PADDW MM3,MM1
              // Finally, pack MM3 and MM4 back into a single quad-word & save
              PACKUSWB MM3,MM0
              MOVD [EDX],MM3
            }
          dpp++; spp++;
        }
      if (doublets > 0)
        {
          __asm
            {
              MOV  ECX,doublets // Set up counter used for looping
              MOV  EBX,alpha_lut // Get base address of alpha lut
              MOV  EAX,spp
              MOV  EDX,dpp
              PXOR MM0,MM0 // Set MM0 to 0
loop_erase_region:
              // Load source pels and unpack into words
              MOVQ MM1,[EAX]
              MOVQ MM2,MM1
              PUNPCKHBW MM1,MM0
              PUNPCKLBW MM2,MM0

              // Load target pels and unpack into words
              MOVQ MM3,[EDX]
              MOVQ MM4,MM3
              PUNPCKHBW MM3,MM0
              PUNPCKLBW MM4,MM0

              // Load alpha scale factors
              MOVQ MM5,MM1    // If you are prepared to use SSE instructions,
              PSRLQ MM5,48    // these 6 lines of code may be replaced by the
              MOVD ESI,MM5    // following 2 lines of code:
              MOVQ MM6,MM2    //      PEXTRW ESI,MM1,3
              PSRLQ MM6,48    //      PEXTRW EDI,MM2,3
              MOVD EDI,MM6    //
              MOVQ MM5,[EBX+ESI*8]
              MOVQ MM6,[EBX+EDI*8]

              // Change source alpha values to 255 for correct alpha adjustment
              POR MM1,MM7
              POR MM2,MM7

              // Get `diff' = source-target in MM1 and MM2
              PSUBW MM1,MM3
              PSUBW MM2,MM4

              // Scale `diff' by alpha and add into `target'
              PSLLW MM1,2 // Because alpha values scaled by 2^14
              PSLLW MM2,2
              PMULHW MM1,MM5 // Scale by alpha and divide by 2^16
              PMULHW MM2,MM6 // Scale by alpha and divide by 2^16
              PADDW MM3,MM1
              PADDW MM4,MM2

              // Finally, pack MM3 and MM4 back into a single quad-word & save
              PACKUSWB MM4,MM3
              MOVQ [EDX],MM4
              ADD EAX,8
              ADD EDX,8
              SUB ECX,1
              JNZ loop_erase_region
              MOV spp,EAX
              MOV dpp,EDX
            }
        }
      if (right)
        { // Process single non-aligned pixel
          __asm
            {
              MOV  EAX,spp
              MOV  EBX,alpha_lut // Get base address of alpha lut
              MOV  EDX,dpp
              // Load source pels and unpack into words
              PXOR MM0,MM0 // Set MM0 to 0
              MOVD MM1,[EAX]
              PUNPCKLBW MM1,MM0
              // Load target pels and unpack into words
              MOVD MM3,[EDX]
              PUNPCKLBW MM3,MM0
              // Load alpha scale factors
              MOVQ MM5,MM1
              PSRLQ MM5,48
              MOVD ESI,MM5
              MOVQ MM5,[EBX+ESI*8]
              // Change source alpha value to 255 for correct alpha adjustment
              POR MM1,MM7
              // Get `diff' = source-target in MM1
              PSUBW MM1,MM3
              // Scale `diff' by alpha and add into `target'
              PSLLW MM1,2 // Because alpha values scaled by 2^14
              PMULHW MM1,MM5 // Scale by alpha and divide by 2^16
              PADDW MM3,MM1
              // Finally, pack MM3 and MM4 back into a single quad-word & save
              PACKUSWB MM3,MM0
              MOVD [EDX],MM3
            }
        }
    }

  __asm EMMS
  return true;
}

/*****************************************************************************/
/* INLINE             simd_blend_region (floating point)                     */
/*****************************************************************************/

inline bool
  simd_blend_region(float *dst, float *src,
                    int height, int width, int dst_row_gap, int src_row_gap)
{
  return false;
}

/*****************************************************************************/
/* INLINE              simd_scaled_blend_region (32-bit)                     */
/*****************************************************************************/

inline bool
  simd_scaled_blend_region(kdu_uint32 *dst, kdu_uint32 *src,
                           int height, int width, int dst_row_gap,
                           int src_row_gap, kdu_int16 alpha_factor_x128)
{
  return false;
}

/*****************************************************************************/
/* INLINE           simd_scaled_blend_region (floating point)                */
/*****************************************************************************/

inline bool
  simd_scaled_blend_region(float *dst, float *src,
                           int height, int width, int dst_row_gap,
                           int src_row_gap, float alpha_factor)
{
  return false;
}

/*****************************************************************************/
/* INLINE         simd_scaled_inv_blend_region (floating point)              */
/*****************************************************************************/

inline bool
  simd_scaled_inv_blend_region(float *dst, float *src,
                               int height, int width, int dst_row_gap,
                               int src_row_gap, float alpha_factor)
{
  return false;
}

/*****************************************************************************/
/* INLINE             simd_blend_premult_region (32-bit)                     */
/*****************************************************************************/

inline bool
  simd_blend_premult_region(kdu_uint32 *dst, kdu_uint32 *src,
                            int height, int width, int dst_row_gap,
                            int src_row_gap)
{
  if ((kdu_mmx_level < 1) || (width < 4))
    return false;
  kdu_int16 *alpha_lut = kdrc_alpha_lut4;
  for (; height > 0; height--, dst+=dst_row_gap, src+=src_row_gap)
    {
      kdu_uint32 *dpp = dst;
      kdu_uint32 *spp = src;
      int left = (_addr_to_kdu_int32(dpp) >> 2) & 1;
      int doublets = (width-left)>>1;
      int right = width - left - (doublets<<1);
      if (left)
        { // Process single non-aligned pixel
          __asm
            {
              MOV  EAX,spp
              MOV  EBX,alpha_lut // Get base address of alpha lut
              MOV  EDX,dpp
              // Load source pels and unpack into words
              PXOR MM0,MM0 // Set MM0 to 0
              MOVD MM1,[EAX]
              PUNPCKLBW MM1,MM0
              // Load target pels and unpack into words
              MOVD MM3,[EDX]
              PUNPCKLBW MM3,MM0
              // Load alpha scale factors
              MOVQ MM5,MM1
              PSRLQ MM5,48
              MOVD ESI,MM5
              MOVQ MM5,[EBX+ESI*8]
              // Add source and target pixels and then subtract the
              // alpha-scaled target pixel.
              PADDW MM1,MM3
              PSLLW MM3,2 // Because max alpha factor is 2^14
              PMULHW MM3,MM5
              PSUBW MM1,MM3
              // Finally, pack MM1 into bytes and save
              PACKUSWB MM1,MM0
              MOVD [EDX],MM1
            }
          dpp++; spp++;
        }
      if (doublets > 0)
        {
          __asm
            {
              MOV  ECX,doublets // Set up counter used for looping
              MOV  EBX,alpha_lut // Get base address of alpha lut
              MOV  EAX,spp
              MOV  EDX,dpp
              PXOR MM0,MM0 // Set MM0 to 0
loop_erase_region:
              // Load source pels and unpack into words
              MOVQ MM1,[EAX]
              MOVQ MM2,MM1
              PUNPCKHBW MM1,MM0
              PUNPCKLBW MM2,MM0
              // Load target pels and unpack into words
              MOVQ MM3,[EDX]
              MOVQ MM4,MM3
              PUNPCKHBW MM3,MM0
              PUNPCKLBW MM4,MM0
              // Load alpha scale factors
              MOVQ MM5,MM1
              PSRLQ MM5,48
              MOVD ESI,MM5
              MOVQ MM6,MM2
              PSRLQ MM6,48
              MOVD EDI,MM6    //
              MOVQ MM5,[EBX+ESI*8]
              MOVQ MM6,[EBX+EDI*8]
              // Add source and target pixels and then subtract the
              // alpha-scaled target pixel.
              PADDW MM1,MM3
              PSLLW MM3,2 // Because max alpha factor is 2^14
              PMULHW MM3,MM5
              PSUBW MM1,MM3
              PADDW MM2,MM4
              PSLLW MM4,2 // Because max alpha factor is 2^14
              PMULHW MM4,MM6
              PSUBW MM2,MM4
              // Finally, pack MM1 and MM2 back into a single quad-word & save
              PACKUSWB MM2,MM1
              MOVQ [EDX],MM2
              ADD EAX,8
              ADD EDX,8
              SUB ECX,1
              JNZ loop_erase_region
              MOV spp,EAX
              MOV dpp,EDX
            }
        }
      if (right)
        { // Process single non-aligned pixel
          __asm
            {
              MOV  EAX,spp
              MOV  EBX,alpha_lut // Get base address of alpha lut
              MOV  EDX,dpp
              // Load source pels and unpack into words
              PXOR MM0,MM0 // Set MM0 to 0
              MOVD MM1,[EAX]
              PUNPCKLBW MM1,MM0
              // Load target pels and unpack into words
              MOVD MM3,[EDX]
              PUNPCKLBW MM3,MM0
              // Load alpha scale factors
              MOVQ MM5,MM1
              PSRLQ MM5,48
              MOVD ESI,MM5
              MOVQ MM5,[EBX+ESI*8]
              // Add source and target pixels and then subtract the
              // alpha-scaled target pixel.
              PADDW MM1,MM3
              PSLLW MM3,2 // Because max alpha factor is 2^14
              PMULHW MM3,MM5
              PSUBW MM1,MM3
              // Finally, pack MM1 into bytes and save
              PACKUSWB MM1,MM0
              MOVD [EDX],MM1
            }
        }
    }

  __asm EMMS
  return true;
}

/*****************************************************************************/
/* INLINE           simd_blend_premult_region (floating point)               */
/*****************************************************************************/

inline bool
  simd_blend_premult_region(float *dst, float *src,
                            int height, int width, int dst_row_gap,
                            int src_row_gap)
{
  return false;
}

#endif // MSVC_REGION_COMPOSITOR_LOCAL_H
