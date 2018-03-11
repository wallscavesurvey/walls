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
from `kdu_region_decompressor', using MMX/SSE/SSE2 instructions, following
the MSVC in-line assembly conventions.
******************************************************************************/

#ifndef MSVC_REGION_DECOMPRESSOR_LOCAL_H
#define MSVC_REGION_DECOMPRESSOR_LOCAL_H

/*****************************************************************************/
/* INLINE                      simd_convert_fix16                            */
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
  if ((kdu_mmx_level < 2) || (num_samples < 16))
    return false; // Not worth special processing
  int lead=(-((_addr_to_kdu_int32(dst))>>1))&7; // Initial non-aligned samples
  for (; lead > 0; lead--, num_samples--, src++, dst++)
    *dst = (upshift>0)?(src->ival<<upshift):((src->ival+offset)>>(-upshift));
  __asm {
      MOV ESI,src
      MOV EDI,dst
      MOV EDX,num_samples
      XOR ECX,ECX
      MOV EAX,upshift
      TEST EAX,EAX
      JG process_upshift
      // If we get here, we are operating with a downshift
      NEG EAX
      MOVD XMM0,EAX
      PCMPEQW XMM3,XMM3 // Set XMM3 to all 1's
      PXOR XMM2,XMM2    // Set XMM2 to 0
      PSUBW XMM2, XMM3  // Leaves each word of XMM2 equal to 1
      PSLLW XMM2,XMM0   // Leaves each word of XMM2 equal to 1<<(-upshift)
      PSRAW XMM2,1      // Leaves the pre-shifting offset in each word of XMM2
      MOVDQU XMM1,[ESI] // Pre-load first source sample
      PADDW XMM1,XMM2
      PSRAW XMM1,XMM0
loop_downshift:
      MOVDQA [EDI+2*ECX],XMM1
      MOVDQU XMM1,[ESI+2*ECX+16] // Load next block in advance
      PADDW XMM1,XMM2
      PSRAW XMM1,XMM0
      ADD ECX,8
      CMP ECX,EDX
      JL loop_downshift
      JMP finish
process_upshift:
      MOVD XMM0,EAX
      MOVDQU XMM1,[ESI] // Pre-load first source sample
      PSLLW XMM1,XMM0
loop_upshift:
      MOVDQA [EDI+2*ECX],XMM1
      MOVDQU XMM1,[ESI+2*ECX+16] // Load next block in advance
      PSLLW XMM1,XMM0
      ADD ECX,8
      CMP ECX,EDX
      JL loop_upshift
finish:
    }
  return true;
}

/*****************************************************************************/
/* INLINE                  simd_vert_resampling_float                        */
/*****************************************************************************/

inline bool
  simd_vert_resampling_float(int length, kdu_line_buf *src[],
                             kdu_line_buf *dst, int kernel_length,
                             float *kernel)
{
  return false; // Only implemented in "x86_region_decompressor.h" at present
}

/*****************************************************************************/
/* INLINE                  simd_vert_resampling_fix16                        */
/*****************************************************************************/

inline bool
  simd_vert_resampling_fix16(int length, kdu_line_buf *src[],
                             kdu_line_buf *dst, int kernel_length,
                             kdu_int16 *kernel)
{
  return false; // Only implemented in "x86_region_decompressor.h" at present
}

/*****************************************************************************/
/* INLINE                  simd_horz_resampling_float                        */
/*****************************************************************************/

inline bool
  simd_horz_resampling_float(int length, kdu_line_buf *src,
                             kdu_line_buf *dst, kdu_int64 phase,
                             kdu_int64 num, int den, int pshift,
                             float **kernels, int kernel_length, int leadin)
{
  return false; // Only implemented in "x86_region_decompressor.h" at present
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
  return false; // Only implemented in "x86_region_decompressor.h" at present
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
  if ((kdu_mmx_level < 2) || (num_samples < 1))
    return false; // Not worth special processing
  kdu_int32 stretch_offset = -((-(stretch_residual<<(KDU_FIX_POINT-1))) >> 16);
  if (stretch_residual <= 0x7FFF)
    { // Use full multiplication-based approach
      __asm {
          MOV ESI,src
          MOV EDI,dst
          MOV EDX,num_samples
          XOR ECX,ECX
          MOVD XMM0,stretch_residual; // Load factor into word 0
          PUNPCKLWD XMM0,XMM0 // Get copy of the factor into word 1
          PUNPCKLDQ XMM0,XMM0 // Get copies of the factor into words 2 and 3
          MOVDQA XMM1,XMM0
          PSLLDQ XMM0,8
          POR XMM0,XMM1 // Finally, 8 copies of `stretch_residual' are in XMM0
          MOVD XMM1,stretch_offset // Load the stretch offset into word 0
          PUNPCKLWD XMM1,XMM1 // Get copy of the offset into word 1
          PUNPCKLDQ XMM1,XMM1 // Get copies of the offset into words 2 and 3
          MOVDQA XMM2,XMM1
          PSLLDQ XMM1,8
          POR XMM1,XMM2 // Finally, 8 copies of `offset' are in XMM1
loop_small_stretch:
          MOVDQA XMM3,[ESI+2*ECX] // Read 8 samples
          MOVDQA XMM4,XMM3
          PMULHW XMM4,XMM0 // Multiply by stretch factor and divide by 2^16
          PADDW XMM3,XMM1 // Add in stretch offset
          PADDW XMM3,XMM4 // Combine results
          MOVDQA [EDI+2*ECX],XMM3 // Write results
          ADD ECX,8
          CMP ECX,EDX
          JL loop_small_stretch
        }
    }
  else
    { // Large stretch residual -- can only happen with 1-bit original data
      int diff=(1<<16)-((int) stretch_residual), downshift = 1;
      while ((diff & 0x8000) == 0)
        { diff <<= 1; downshift++; }
      __asm {
          MOV ESI,src
          MOV EDI,dst
          MOV EDX,num_samples
          XOR ECX,ECX
          MOVD XMM0,downshift;
          MOVD XMM1,stretch_offset // Load the stretch offset into word 0
          PUNPCKLWD XMM1,XMM1 // Get copy of the offset into word 1
          PUNPCKLDQ XMM1,XMM1 // Get copies of the offset into words 2 and 3
          MOVDQA XMM2,XMM1
          PSLLDQ XMM1,8
          POR XMM1,XMM2 // Finally, 8 copies of `offset' are in XMM1
loop_large_stretch:
          MOVDQA XMM3,[ESI+2*ECX] // Read 8 samples
          MOVDQA XMM4,XMM3
          PSRAW XMM3,XMM0 // Downshift source samples
          PADDW XMM4,XMM4 // Double original source samples
          PADDW XMM4,XMM1 // Add in the offset
          PSUBW XMM4,XMM3 // Subtract the shifted version
          MOVDQA [EDI+2*ECX],XMM4 // Write results
          ADD ECX,8
          CMP ECX,EDX
          JL loop_large_stretch
        }
    }
  return true;
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

  kdu_uint32 vec_offset = offset;  vec_offset += vec_offset << 16;
  kdu_uint32 vec_max = (kdu_uint32)(~mask);  vec_max += vec_max << 16;
  int k = 0;
  __asm {
      MOVD XMM0,downshift
      MOVD XMM1,vec_offset
      PSHUFD XMM1,XMM1,0 // Create 8 replicas of offset in XMM1
      PXOR XMM2,XMM2     // Create lower bound (0) in XMM2
      MOVD XMM3,vec_max
      PSHUFD XMM3,XMM3,0 // Create 8 replicas of max value in XMM3
      XOR ECX,ECX // Sample counter
      MOV EDX,num_samples // Sample counter limit
      SUB EDX,16
      MOV ESI,src
      MOV EDI,dst
loop_start:
      MOVDQA XMM4,[ESI+2*ECX] // Load first 8 source samples
      PADDW XMM4,XMM1 // Add the offset
      PSRAW XMM4,XMM0 // Apply the downshift
      MOVDQA XMM5,[ESI+2*ECX+16] // Load second 8 source samples
      PMAXSW XMM4,XMM2 // Clip to min value of 0
      PMINSW XMM4,XMM3 // Clip to max value of `mask'
      PADDW XMM5,XMM1 // Add the offset
      PSRAW XMM5,XMM0 // Apply the downshift
      PMAXSW XMM5,XMM2 // Clip to min value of 0
      PMINSW XMM5,XMM3 // Clip to max value of `mask'
      PACKUSWB XMM4,XMM5
      MOVDQU [EDI+ECX],XMM4 // Will be aligned if the app aligned its buffers
      ADD ECX,16 // We have just processed 16 samples
      CMP ECX,EDX
      JLE loop_start
      MOV k,ECX
    }

  for (; k < num_samples; k++)
    {
      kdu_int16 val = (src[k].ival+offset)>>downshift;
      if (val & mask)
        val = (val < 0)?0:~mask;
      dst[k] = (kdu_byte) val;
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
  assert(num_channels <= 4);
  int cm_val = ((int *) channel_mask)[0]; // Load all 4 mask bytes into 1 dword
  if ((kdu_mmx_level < 2) || ((cm_val & 0x00FFFFFF) != 0x00FFFFFF))
    return false; // Present implementation assumes first 3 channels are used
  cm_val >>= 24;  // Keep just the most last source mask byte
  int fa_val = (fill_alpha)?1:0;
  int downshift = KDU_FIX_POINT-prec_bits;
  kdu_int16 offset = (1<<downshift)>>1; // Rounding offset for the downshift
  offset += (1<<KDU_FIX_POINT)>>1; // Convert from signed to unsigned
  kdu_int16 mask = (kdu_int16)((-1) << prec_bits);
  kdu_uint32 vec_offset = offset;  vec_offset += vec_offset << 16;
  kdu_uint32 vec_max = (kdu_uint32)(~mask);  vec_max += vec_max << 16;

  kdu_sample16 *src0 = src_line0->get_buf16();
  kdu_sample16 *src1 = src_line1->get_buf16();
  kdu_sample16 *src2 = src_line2->get_buf16();
  kdu_sample16 *src3 = src_line3->get_buf16();

  num_pixels -= 8; // Makes it easier to perform comparisons in the code below
  int k=0; // Accessible version of the pixel counter

  __asm {
      MOV EDI,dst_base // EDI holds the target address -- required by MASKMOV
      MOV EAX,src0
      MOV EBX,src1
      MOV ECX,src2
      MOVD XMM0,downshift
      MOVD XMM1,vec_offset
      PSHUFD XMM1,XMM1,0 // Create 8 replicas of offset in XMM1
      PXOR XMM2,XMM2     // Create lower bound (0) in XMM2
      MOVD XMM3,vec_max
      PSHUFD XMM3,XMM3,0 // Create 8 replicas of max value in XMM3
      XOR ESI,ESI        // This becomes a zero-based pixel counter

      MOV EDX,fa_val
      TEST EDX,EDX
      JNZ process_fill
      MOV EDX,cm_val
      TEST EDX,EDX
      JNZ process_four

      // If we get here, only the first three channels are used -- quite common
      MOV EDX,num_pixels
      PCMPEQW XMM7,XMM7
      PSLLD XMM7,24     // Create mask for bytes which are to be left unchanged
loop_start_three:
      CMP ESI,EDX
      JG finish_off     // Less than 8 pixels left to be generated
      MOVDQA XMM5,[EBX+2*ESI] // Load second channel source to XMM5
      PADDW XMM5,XMM1   // Add the offset
      PSRAW XMM5,XMM0   // Apply the downshift
      MOVDQA XMM4,[EAX+2*ESI] // Load first channel source to XMM4
      PMAXSW XMM5,XMM2  // Clip to min value of 0
      PMINSW XMM5,XMM3  // Clip to max value of `mask'
      PSLLW XMM5,8
      PADDW XMM4,XMM1   // Add the offset
      PSRAW XMM4,XMM0   // Apply the downshift
      MOVDQA XMM6,[ECX+2*ESI] // Load third channel source to XMM6
      PMAXSW XMM4,XMM2  // Clip to min value of 0
      PMINSW XMM4,XMM3  // Clip to max value of `mask'
      POR XMM4,XMM5     // XMM4 now holds the 1'st pair of interleaved outputs
      POR XMM5,XMM4     // XMM5 now holds the same result as XMM4
      PADDW XMM6,XMM1   // Add the offset
      PSRAW XMM6,XMM0   // Apply the downshift
      PMAXSW XMM6,XMM2  // Clip to min value of 0
      PMINSW XMM6,XMM3  // Clip to max value of `mask'

      MOVDQU XMM2,[EDI+4*ESI] // Borrow XMM2 to load existing data
      PUNPCKLWD XMM4,XMM6
      PAND XMM2,XMM7
      POR XMM4,XMM2
      MOVDQU [EDI+4*ESI],XMM4

      MOVDQU XMM2,[EDI+4*ESI+16]
      PUNPCKHWD XMM5,XMM6
      PAND XMM2,XMM7
      POR XMM5,XMM2
      MOVDQU [EDI+4*ESI+16],XMM5
      PXOR XMM2,XMM2  // Restore role of XMM2 as lower bound (0) value

      ADD ESI,8
      JMP loop_start_three

process_fill:
      // If we get here, the first three channels are derived from source
      // data and the fourth (most significant output byte) is set to 0xFF.
      // This implements the alpha filling feature for the second form of
      // the `kdu_region_decompressor::process' function.
      MOV EDX,num_pixels
      PCMPEQW XMM7,XMM7
      PSLLW XMM7,8     // Leaves 0xFF in most significant byte of each word
loop_start_fill:
      CMP ESI,EDX
      JG finish_off     // Less than 8 pixels left to be generated
      MOVDQA XMM5,[EBX+2*ESI] // Load second channel source to XMM5
      PADDW XMM5,XMM1   // Add the offset
      PSRAW XMM5,XMM0   // Apply the downshift
      MOVDQA XMM4,[EAX+2*ESI] // Load first channel source to XMM4
      PMAXSW XMM5,XMM2  // Clip to min value of 0
      PMINSW XMM5,XMM3  // Clip to max value of `mask'
      PSLLW XMM5,8
      PADDW XMM4,XMM1   // Add the offset
      PSRAW XMM4,XMM0   // Apply the downshift
      MOVDQA XMM6,[ECX+2*ESI] // Load third channel source to XMM6
      PMAXSW XMM4,XMM2  // Clip to min value of 0
      PMINSW XMM4,XMM3  // Clip to max value of `mask'
      POR XMM4,XMM5     // XMM4 now holds the 1'st pair of interleaved outputs
      POR XMM5,XMM4     // XMM5 now holds the same result as XMM4
      PADDW XMM6,XMM1   // Add the offset
      PSRAW XMM6,XMM0   // Apply the downshift
      PMAXSW XMM6,XMM2  // Clip to min value of 0
      PMINSW XMM6,XMM3  // Clip to max value of `mask'
      POR XMM6,XMM7

      PUNPCKLWD XMM4,XMM6
      MOVDQU [EDI+4*ESI],XMM4
      PUNPCKHWD XMM5,XMM6
      MOVDQU [EDI+4*ESI+16],XMM5

      ADD ESI,8
      JMP loop_start_fill

process_four:
      // If we get here, all four channels are used
      MOV EDX,src3
loop_start_four:
      CMP ESI,num_pixels
      JG finish_off     // Less than 8 pixels left to be generated

      MOVDQA XMM5,[EBX+2*ESI] // Load second channel source to XMM5
      PADDW XMM5,XMM1   // Add the offset
      PSRAW XMM5,XMM0   // Apply the downshift
      MOVDQA XMM4,[EAX+2*ESI] // Load first channel source to XMM4
      PMAXSW XMM5,XMM2  // Clip to min value of 0
      PMINSW XMM5,XMM3  // Clip to max value of `mask'
      PSLLW XMM5,8
      PADDW XMM4,XMM1   // Add the offset
      PSRAW XMM4,XMM0   // Apply the downshift
      MOVDQA XMM6,[EDX+2*ESI] // Load fourth channel source to XMM6
      PMAXSW XMM4,XMM2  // Clip to min value of 0
      PMINSW XMM4,XMM3  // Clip to max value of `mask'
      POR XMM4,XMM5     // XMM4 now holds the 1'st pair of interleaved outputs
      POR XMM5,XMM4     // XMM5 also holds the 1'st pair of interleaved outputs
      PADDW XMM6,XMM1   // Add the offset
      PSRAW XMM6,XMM0   // Apply the downshift
      MOVDQA XMM7,[ECX+2*ESI] // Load third channel source to XMM7
      PMAXSW XMM6,XMM2  // Clip to min value of 0
      PMINSW XMM6,XMM3  // Clip to max value of `mask'
      PSLLW XMM6,8
      PADDW XMM7,XMM1   // Add the offset
      PSRAW XMM7,XMM0   // Apply the downshift
      PMAXSW XMM7,XMM2  // Clip to min value of 0
      PMINSW XMM7,XMM3  // Clip to max value of `mask'
      POR XMM6,XMM7     // XMM6 now holds the 2'nd pair of interleaved outputs

      PUNPCKLWD XMM4,XMM6 // Form first 16 output bytes in XMM4
      MOVDQU [EDI+4*ESI],XMM4
      PUNPCKHWD XMM5,XMM6 // Form second 16 output bytes in XMM5
      MOVDQU [EDI+4*ESI+16],XMM5

      ADD ESI,8
      JMP loop_start_four
finish_off:
      MOV k,ESI
    }

  for (num_pixels+=8; k < num_pixels; k++)
    { // Need to generate trailing samples one-by-one
      kdu_int16 val;

      val = (src0[k].ival+offset) >> downshift;
      if (val & mask)
        val = (val < 0)?0:~mask;
      dst_base[4*k+0] = (kdu_byte) val;

      val = (src1[k].ival+offset) >> downshift;
      if (val & mask)
        val = (val < 0)?0:~mask;
      dst_base[4*k+1] = (kdu_byte) val;

      val = (src2[k].ival+offset) >> downshift;
      if (val & mask)
        val = (val < 0)?0:~mask;
      dst_base[4*k+2] = (kdu_byte) val;
      
      if (fa_val)
        dst_base[4*k+3] = 0xFF;
      else if (cm_val)
        {
          val = (src3[k].ival+offset) >> downshift;
          if (val & mask)
            val = (val < 0)?0:~mask;
          dst_base[4*k+3] = (kdu_byte) val;
        }
    }

  return true;
}

#endif // MSVC_REGION_DECOMPRESSOR_LOCAL_H
