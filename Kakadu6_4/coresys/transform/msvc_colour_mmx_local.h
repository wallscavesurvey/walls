/*****************************************************************************/
// File: msvc_colour_mmx_local.h [scope = CORESYS/TRANSFORMS]
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
   Provides MMX implementations of the colour transformations -- both
reversible (RCT) and irreversible (ICT = RGB to YCbCr) -- following the
MSVC conventins.  You can use this implementation only for 32-bit builds
on MSVC or .NET.  For 64-bit .NET compilation, use the more portable
implementation found in "x86_colour_local.h".
******************************************************************************/
#ifndef MSVC_COLOUR_MMX_LOCAL_H
#define MSVC_COLOUR_MMX_LOCAL_H

static kdu_int16 *create_vector_constants(); // Forward declaration
static kdu_int16 _vec_store[64 + 7];
static kdu_int16 *vec_aligned = create_vector_constants();
static kdu_int16 *vec_alphaR = vec_aligned + 0;
static kdu_int16 *vec_alphaB = vec_aligned + 8;
static kdu_int16 *vec_CBfact = vec_aligned + 16;
static kdu_int16 *vec_CRfact = vec_aligned + 24;
static kdu_int16 *vec_CRfactR = vec_aligned + 32;
static kdu_int16 *vec_CBfactB = vec_aligned + 40;
static kdu_int16 *vec_CRfactG = vec_aligned + 48;
static kdu_int16 *vec_CBfactG = vec_aligned + 56;

static kdu_int16 *
create_vector_constants()
{
	kdu_int16 alphaR = (kdu_int16)(0.299 * (1 << 16));
	kdu_int16 alphaB = (kdu_int16)(0.114 * (1 << 16));
	kdu_int16 CBfact = (kdu_int16)(0.4356659 * (1 << 16)); // Actual value is 1-0.4356659
	kdu_int16 CRfact = (kdu_int16)(0.2867332 * (1 << 16)); // Actual value is 1-0.2867332

	kdu_int16 CRfactR = (kdu_int16)(0.402 * (1 << 16)); // Actual factor is 1.402
	kdu_int16 CBfactB = (kdu_int16)(-0.228 * (1 << 16)); // Actual factor is 1.772
	kdu_int16 CRfactG = (kdu_int16)(0.285864 * (1 << 16)); // Actual factor is -0.714136
	kdu_int16 CBfactG = (kdu_int16)(-0.344136 * (1 << 16)); // Actual factor is -0.344136

	kdu_int16 *buf = _vec_store;
	int k, alignment = _addr_to_kdu_int32(buf) & 0x0F;
	assert((alignment & 1) == 0);
	alignment >>= 1; // Convert to words
	buf += ((8 - alignment) & 7);

	for (k = 0; k < 8; k++)
	{
		buf[k] = alphaR;      buf[k + 8] = alphaB;
		buf[k + 16] = CBfact;   buf[k + 24] = CRfact;
		buf[k + 32] = CRfactR;  buf[k + 40] = CBfactB;
		buf[k + 48] = CRfactG;  buf[k + 56] = CBfactG;
	}
	return buf;
}

/* ========================================================================= */
/*                        Now for the MMX functions                          */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                          simd_ict                                  */
/*****************************************************************************/

inline bool
simd_ict(kdu_int16 *src1, kdu_int16 *src2, kdu_int16 *src3, int samples)
{
	if (kdu_mmx_level < 1)
		return false;
	if (samples <= 0)
		return true;

	assert(vec_CRfact == (vec_CBfact + 8)); // Assumed for the code below to work
	__asm
	{
		XOR EDI, EDI      // Zero sample counter
		MOV EDX, samples  // Upper limit for sample counter
		MOV ESI, vec_alphaR;
		MOVQ MM5, [ESI]
			MOV ESI, vec_alphaB
			MOVQ MM6, [ESI]
			MOV ESI, vec_CBfact
			MOV EAX, src1
			MOV ECX, src3
			MOV EBX, src2 // Load this last (.NET 2008 uses EBX for debug stack frame)
			PCMPEQW MM7, MM7  // Fill MM7 with 1's
			PSRLW MM7, 15     // Leaves each word in MM7 equal to 1
			loop_ict:
		MOVQ MM1, [EAX + 2 * EDI] // Load red; need to use this multiple times
			MOVQ MM2, [ECX + 2 * EDI] // Load blue; need to use this multiple times
			MOVQ MM3, MM7         // Load a quad-word of 1's
			PADDW MM3, MM3        // Double to get a quad-word of 2's
			MOVQ MM0, MM3         // Move the pre-offset of 2 into MM0
			PADDSW MM0, MM1       // Form the pre-offset red channel in MM0
			PMULHW MM0, MM5       // Form red contribution to Y in MM0
			PADDW MM3, MM3        // 4 is good pre-offset for multiplication by alphaB
			PADDSW MM3, MM2       // Form the pre-offset blue channel in MM3
			PMULHW MM3, MM6       // Form blue contribution to Y in MM0
			PADDSW MM0, MM3       // Add blue contribution to red contribution
			MOVQ MM3, MM5
			PADDW MM3, MM6        // Gets alphaR+alphaB to MM3
			MOVQ MM4, [EBX + 2 * EDI] // Load green channel to MM4; we only need it once
			PADDSW MM4, MM7       // Pre-offset the green channel by 1
			PMULHW MM3, MM4       // Creates green * (alphaR+alphaB) in MM3
			PSUBSW MM4, MM7       // Remove the pre-offset from the green channel
			PSUBSW MM4, MM3       // Forms green * (1-alphaR-alphaB) in MM4
			PADDSW MM0, MM4       // Forms luminance in MM0
			MOVQ[EAX + 2 * EDI], MM0 // Write out luminance
			PSUBSW MM1, MM0       // Forms R-Y in MM1
			PSUBSW MM2, MM0       // Forms B-Y in MM2
			MOVQ MM0, MM1         // Copy R-Y to MM0
			PADDSW MM1, MM7       // Add a pre-offset of 1
			PADDSW MM1, MM7       // Make the pre-offset 2
			PMULHW MM1, [ESI + 16]  // Loads `q_CRfact'
			PSUBSW MM0, MM1       // Forms (1-CRfact)*(R-Y) in MM0
			MOVQ[ECX + 2 * EDI], MM0 // Write out the CR channel
			MOVQ MM0, MM2         // Copy B-Y to MM0
			PADDSW MM2, MM7       // Add a pre-offset of 1
			PMULHW MM2, [ESI]     // Loads `q_CBfact'
			PSUBSW MM0, MM2       // Forms (1-CBfact)*(B-Y) in MM0
			MOVQ[EBX + 2 * EDI], MM0 // Write out the CB channel
			ADD EDI, 4
			CMP EDI, EDX
			JL loop_ict
			EMMS // Clear MMX registers for use by FPU
	}
	return true;
}

/*****************************************************************************/
/* INLINE                      simd_inverse_ict                              */
/*****************************************************************************/

inline bool
simd_inverse_ict(kdu_int16 *src1, kdu_int16 *src2, kdu_int16 *src3,
	int samples)
{
	if (kdu_mmx_level < 1)
		return false;
	if (samples <= 0)
		return true;

	assert(vec_CBfactG == (vec_CRfactG + 8)); // Assumed for the code below to work

#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__asm {
			XOR EDI, EDI          // Zero sample counter
			MOV EDX, samples      // Sample counter limit
			PCMPEQW XMM4, XMM4      // Fill XMM4 with 1's
			PSRLW XMM4, 15          // Set each word in XMM4 equal to 1
			MOVQ XMM5, XMM4
			PADDW XMM5, XMM5        // Set each word in XMM5 equal to 2
			MOV ESI, vec_CRfactR
			MOVDQA XMM6, [ESI]
			MOV ESI, vec_CBfactB
			MOVDQA XMM7, [ESI]
			MOV ESI, vec_CRfactG
			MOV EAX, src1
			MOV ECX, src3
			MOV EBX, src2 // Load last (.NET 2008 uses EBX for debug stack frame)
			loop_inverse_ict128 :
			MOVDQA XMM0, [EAX + 2 * EDI]  // Load luminance (Y)
				MOVDQA XMM1, [ECX + 2 * EDI]  // Load chrominance (Cr)
				MOVDQA XMM2, XMM1         // Prepare to form Red output in XMM2
				PADDSW XMM2, XMM4         // +1 here approximates add 2^15 then divide by 2^16
				PMULHW XMM2, XMM6         // Multiply by 0.402*2^16 (CRfactR) & divide by 2^16
				PADDSW XMM2, XMM1         // Add Cr again to make factor equivalent to 1.402
				PADDSW XMM2, XMM0         // Add in luminance to get Red
				MOVDQA[EAX + 2 * EDI], XMM2  // Save Red channel
				MOVDQA XMM2, XMM1         // Prepare to form Cr*(-0.714136) in XMM2
				PADDSW XMM2, XMM5         // +2 here approximates add 2^15 then divide by 2^16
				PMULHW XMM2, [ESI]        // Multiply by 0.285864*2^16 (CRfactG) & divide by 2^16
				PSUBSW XMM2, XMM1         // Subtract Cr leaves us with the desired result
				PADDSW XMM2, XMM0         // Add Y to scaled Cr forms most of Green in MM2
				MOVDQA XMM1, [EBX + 2 * EDI]  // Load chrominance (Cb)
				MOVDQA XMM3, XMM1         // Prepare to form Blue output in XMM3
				PSUBSW XMM3, XMM5         // +2 here approximates add 2^15 then divide by 2^16
				PMULHW XMM3, XMM7         // Multiply by -0.228*2^16 (CBfactB) & divide by 2^16
				PADDSW XMM3, XMM1         // Gets 0.772*Cb to XMM3
				PADDSW XMM3, XMM1         // Gets 1.772*Cb to XMM3
				PADDSW XMM3, XMM0         // Add in luminance to get Blue
				MOVDQA[ECX + 2 * EDI], XMM3  // Save Blue channel
				PSUBSW XMM1, XMM5         // +2 here approximates add 2^15 then divide by 2^16
				PMULHW XMM1, [ESI + 16]     // Multiply by -0.344136*2^16 (CBfactG) and divide by 2^16
				PADDSW XMM2, XMM1         // Completes the Green channel in XMM2
				MOVDQA[EBX + 2 * EDI], XMM2
				ADD EDI, 8
				CMP EDI, EDX
				JL loop_inverse_ict128
		}
	}
	else
#endif // !KDU_NO_SSE
	{ // 64-bit implementation, using only MMX instructions
		__asm {
			XOR EDI, EDI          // Zero sample counter
			MOV EDX, samples      // Sample counter limit
			PCMPEQW MM4, MM4      // Fill MM4 with 1's
			PSRLW MM4, 15         // Set each word in MM4 equal to 1
			MOVQ MM5, MM4
			PADDW MM5, MM5        // Set each word in MM5 equal to 2
			MOV ESI, vec_CRfactR
			MOVQ MM6, [ESI]
			MOV ESI, vec_CBfactB
			MOVQ MM7, [ESI]
			MOV ESI, vec_CRfactG
			MOV EAX, src1
			MOV ECX, src3
			MOV EBX, src2 // Load last (.NET 2008 uses EBX for debug stack frame)
			loop_inverse_ict64 :
			MOVQ MM0, [EAX + 2 * EDI] // Load luminance (Y)
				MOVQ MM1, [ECX + 2 * EDI] // Load chrominance (Cr)
				MOVQ MM2, MM1         // Prepare to form Red output in MM2
				PADDSW MM2, MM4       // +1 here approximates add 2^15 then divide by 2^16
				PMULHW MM2, MM6       // Multiply by 0.402*2^16 (CRfactR) & divide by 2^16
				PADDSW MM2, MM1       // Add Cr again to make factor equivalent to 1.402
				PADDSW MM2, MM0       // Add in luminance to get Red
				MOVQ[EAX + 2 * EDI], MM2 // Save Red channel
				MOVQ MM2, MM1         // Prepare to form Cr*(-0.714136) in MM2
				PADDSW MM2, MM5       // +2 here approximates add 2^15 then divide by 2^16
				PMULHW MM2, [ESI]     // Multiply by 0.285864*2^16 (CRfactG) & divide by 2^16
				PSUBSW MM2, MM1       // Subtract Cr leaves us with the desired result
				PADDSW MM2, MM0       // Add Y to scaled Cr forms most of Green in MM2
				MOVQ MM1, [EBX + 2 * EDI] // Load chrominance (Cb)
				MOVQ MM3, MM1         // Prepare to form Blue output in MM3
				PSUBSW MM3, MM5       // +2 here approximates add 2^15 then divide by 2^16
				PMULHW MM3, MM7       // Multiply by -0.228*2^16 (CBfactB) & divide by 2^16
				PADDSW MM3, MM1       // Gets 0.772*Cb to MM3
				PADDSW MM3, MM1       // Gets 1.772*Cb to MM3
				PADDSW MM3, MM0       // Add in luminance to get Blue
				MOVQ[ECX + 2 * EDI], MM3 // Save Blue channel
				PSUBSW MM1, MM5       // +2 here approximates add 2^15 then divide by 2^16
				PMULHW MM1, [ESI + 16]  // Multiply by -0.344136*2^16 (CBfactG) and divide by 2^16
				PADDSW MM2, MM1       // Completes the Green channel in MM2
				MOVQ[EBX + 2 * EDI], MM2
				ADD EDI, 4
				CMP EDI, EDX
				JL loop_inverse_ict64
				EMMS // Clear MMX registers for use by FPU
		}
	}
	return true;
}

/*****************************************************************************/
/* INLINE                           simd_rct                                 */
/*****************************************************************************/

inline bool
simd_rct(kdu_int16 *src1, kdu_int16 *src2, kdu_int16 *src3, int samples)
{
	if (kdu_mmx_level < 1)
		return false;
	if (samples <= 0)
		return true;

	__asm
	{
		XOR EDI, EDI          // Initialize sample counter to 0
		MOV EDX, samples      // Upper bound for sample counter
		MOV EAX, src1
		MOV ECX, src3
		MOV EBX, src2 // Load last (.NET 2008 uses EBX for debug stack frame)
		loop_rct :
		MOVQ MM1, [EAX + 2 * EDI] // Load red (R)
			MOVQ MM2, [EBX + 2 * EDI] // Load green (G)
			MOVQ MM3, [ECX + 2 * EDI] // Load blue (B)
			MOVQ MM0, MM1         // Get R to MM0
			PADDSW MM0, MM3       // Add B to MM0
			PADDSW MM0, MM2
			PADDSW MM0, MM2       // Add 2*G to MM0
			PSRAW MM0, 2          // Forms (R+2*G+B)>>2
			MOVQ[EAX + 2 * EDI], MM0 // Write out Y channel
			PSUBSW MM3, MM2       // Subtract G from B
			MOVQ[EBX + 2 * EDI], MM3 // Write out Db channel
			PSUBSW MM1, MM2       // Subtract G from R
			MOVQ[ECX + 2 * EDI], MM1 // Write out Dr channel
			ADD EDI, 4
			CMP EDI, EDX
			JL loop_rct
			EMMS // Clear MMX registers for use by FPU
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
	if (kdu_mmx_level < 1)
		return false;
	if (samples <= 0)
		return true;

#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation using SSE/SSE2 instructions
		__asm {
			XOR EDI, EDI        // Initialize sample counter to 0
			MOV EDX, samples      // Set upper bound for sample counter
			MOV EAX, src1
			MOV ECX, src3
			MOV EBX, src2 // Load last (.NET 2008 uses EBX for debug stack frame)
			loop_inverse_rct128 :
			MOVDQA XMM1, [EBX + 2 * EDI] // Load chrominance (Db)
				MOVDQA XMM2, [ECX + 2 * EDI] // Load chrominance (Dr)
				MOVDQA XMM3, XMM1
				PADDSW XMM3, XMM2
				PSRAW XMM3, 2            // Forms (Db+DR)>>2
				MOVDQA XMM0, [EAX + 2 * EDI] // Load luminance (Y).
				PSUBSW XMM0, XMM3        // Convert Y to Green channel
				MOVDQA[EBX + 2 * EDI], XMM0
				PADDSW XMM2, XMM0        // Convert Dr to Red channel
				MOVDQA[EAX + 2 * EDI], XMM2
				PADDSW XMM1, XMM0        // Convert Db to Blue channel
				MOVDQA[ECX + 2 * EDI], XMM1
				ADD EDI, 8
				CMP EDI, EDX
				JL loop_inverse_rct128
		}
	}
	else
#endif // !KDU_NO_SSE
	{ // 64-bit implementation using only MMX instructions
		__asm {
			XOR EDI, EDI        // Initialize sample counter to 0
			MOV EDX, samples      // Set upper bound for sample counter
			MOV EAX, src1
			MOV ECX, src3
			MOV EBX, src2 // Load last (.NET 2008 uses EBX for debug stack frame)
			loop_inverse_rct64 :
			MOVQ MM1, [EBX + 2 * EDI] // Load chrominance (Db)
				MOVQ MM2, [ECX + 2 * EDI] // Load chrominance (Dr)
				MOVQ MM3, MM1
				PADDSW MM3, MM2
				PSRAW MM3, 2          // Forms (Db+DR)>>2
				MOVQ MM0, [EAX + 2 * EDI] // Load luminance (Y).
				PSUBSW MM0, MM3       // Convert Y to Green channel
				MOVQ[EBX + 2 * EDI], MM0
				PADDSW MM2, MM0       // Convert Dr to Red channel
				MOVQ[EAX + 2 * EDI], MM2
				PADDSW MM1, MM0       // Convert Db to Blue channel
				MOVQ[ECX + 2 * EDI], MM1
				ADD EDI, 4
				CMP EDI, EDX
				JL loop_inverse_rct64
				EMMS // Clear MMX registers for use by FPU
		}
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

#endif // MSVC_COLOUR_MMX_LOCAL_H
