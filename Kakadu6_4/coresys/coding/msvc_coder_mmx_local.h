/*****************************************************************************/
// File: msvc_coder_mmx_local.h [scope = CORESYS/CODING]
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
#ifndef MSVC_CODER_MMX_LOCAL_H
#define MSVC_CODER_MMX_LOCAL_H

/* ========================================================================= */
/*                        Now for the MMX functions                          */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                   simd_xfer_decoded_block                          */
/*****************************************************************************/

static inline bool
simd_xfer_decoded_block(kdu_int32 *src, kdu_sample16 **dst_refs,
	int dst_offset, int width, int height,
	bool reversible, int K_max, float delta)
{
	if (kdu_mmx_level < 2)
		return false;
	assert(((width == 32) || (width == 64)) && (height > 0));
	if (reversible)
	{
		__asm {
			MOV EAX, 31
			SUB EAX, K_max
			MOVD XMM0, EAX      // This is the downshift value
			MOV ESI, src
			MOV EAX, height     // EAX is the vertical counter
			MOV EBX, dst_offset
			ADD EBX, EBX        // EBX holds the row offset
			MOV EDX, dst_refs   // EDX points to the next output row
			PCMPEQD XMM2, XMM2  // Fill XMM2 with 1's
			PCMPEQD XMM1, XMM1  // Also fill XMM1 with 1's
			PSRLD XMM1, 31      // Set each dword in XMM1 equal to 1
			MOVD XMM3, K_max    // Temporary value
			PSLLD XMM2, XMM3    // Set XMM2 dwords to have 1+downshift 1's in MSBs
			POR XMM1, XMM2      // XMM1 now holds the amount we have to add after
			   // inverting the bits of a downshifted sign-mag quantity which
			   // was negative, to restore the correct 2's complement value.
			   outer_loop_reversible :
			MOV EDI, [EDX]
				ADD EDI, EBX        // Get the address of the next output sample
				MOV ECX, width
				inner_loop_reversible :
			MOVDQA XMM4, [ESI]
				PXOR XMM5, XMM5
				PCMPGTD XMM5, XMM4  // Fills dwords in XMM5 with 1s if XMM4 dwords < 0
				PXOR XMM4, XMM5
				PSRAD XMM4, XMM0
				PAND XMM5, XMM1     // Leave the bits we need to add
				PADDD XMM4, XMM5    // Finish conversion from sign-mag to 2's comp

				MOVDQA XMM6, [ESI + 16]
				PXOR XMM7, XMM7
				PCMPGTD XMM7, XMM6  // Fills dwords in XMM7 with 1s if XMM6 dwords < 0
				PXOR XMM6, XMM7
				PSRAD XMM6, XMM0    // Apply the downshift value
				PAND XMM7, XMM1     // Leave the bits we need to add
				PADDD XMM6, XMM7    // Finish conversion from sign-mag to 2's comp

				PACKSSDW XMM4, XMM6 // Get packed 8 words
				MOVDQA[EDI], XMM4  // Write result
				ADD ESI, 32
				ADD EDI, 16
				SUB ECX, 8
				JG inner_loop_reversible
				ADD EDX, 4          // Point to next output row
				SUB EAX, 1
				JG outer_loop_reversible
		}
	}
	else
	{
		float fscale = delta * (float)(1 << KDU_FIX_POINT);
		if (K_max <= 31)
			fscale /= (float)(1 << (31 - K_max));
		else
			fscale *= (float)(1 << (K_max - 31));
		float vec_scale[4] = { fscale,fscale,fscale,fscale };
		int mxcsr_orig, mxcsr_cur;
		__asm STMXCSR mxcsr_orig;
		mxcsr_cur = mxcsr_orig & ~(3 << 13); // Reset rounding control bits
		__asm {
			LDMXCSR mxcsr_cur;
			MOV ESI, src
				MOV EAX, height     // EAX is the vertical counter
				MOV EBX, dst_offset
				ADD EBX, EBX        // EBX holds the row offset
				MOV EDX, dst_refs   // EDX points to the next output row
				MOVUPS XMM0, vec_scale
				PCMPEQD XMM2, XMM2  // Fill XMM2 with 1's
				PCMPEQD XMM1, XMM1  // Also fill XMM1 with 1's
				PSRLD XMM1, 31      // Set each dword in XMM1 equal to 1
				PSLLD XMM2, 31      // Set each dword in XMM2 equal to 0x80000000
				POR XMM1, XMM2      // XMM1 now holds 0x80000001

				outer_loop_irreversible :
			MOV EDI, [EDX]
				ADD EDI, EBX        // Get the address of the next output sample
				MOV ECX, width
				inner_loop_irreversible :
			MOVDQA XMM4, [ESI]
				PXOR XMM5, XMM5
				PCMPGTD XMM5, XMM4  // Fills dwords in XMM5 with 1s if XMM4 dwords < 0
				PXOR XMM4, XMM5
				PAND XMM5, XMM1     // AND off all but MSB and LSB in each dword
				PADDD XMM4, XMM5    // Finish conversion from sign-mag to 2's comp
				CVTDQ2PS XMM4, XMM4 // Convert dwords to single precision floats
				MULPS XMM4, XMM0    // Scale by adjusted quantization step size
				CVTPS2DQ XMM4, XMM4

				MOVDQA XMM6, [ESI + 16]
				PXOR XMM7, XMM7
				PCMPGTD XMM7, XMM6  // Fills dwords in XMM7 with 1s if XMM6 dwords < 0
				PXOR XMM6, XMM7
				PAND XMM7, XMM1     // AND off all but MSB and LSB in each word
				PADDD XMM6, XMM7    // Finish conversion from sign-mag to 2's comp
				CVTDQ2PS XMM6, XMM6 // Convert dwords to single precision floats
				MULPS XMM6, XMM0    // Scale by adjusted quantization step size
				CVTPS2DQ XMM6, XMM6

				PACKSSDW XMM4, XMM6 // Get packed 8 words
				MOVDQA[EDI], XMM4  // Write result
				ADD ESI, 32
				ADD EDI, 16
				SUB ECX, 8
				JG inner_loop_irreversible
				ADD EDX, 4          // Point to next output row
				SUB EAX, 1
				JG outer_loop_irreversible

				LDMXCSR mxcsr_orig;// Restore rounding control bits
		}
	}
	return true;
}

#endif // MSVC_CODER_MMX_LOCAL_H
