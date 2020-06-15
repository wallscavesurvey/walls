/*****************************************************************************/
// File: x86_dwt_local.h [scope = CORESYS/TRANSFORMS]
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
MMX/SSE/SSE2 intrinsics.  These can be compiled under GCC or .NET and are
compatible with both 32-bit and 64-bit builds.  Note, however, that the
SSE/SSE2 intrinsics require 16-byte stack alignment, which might not be
guaranteed by GCC builds if the code is called from outside (e.g., from a
Java Virtual Machine, which has poorer stack alignment).  This is an ongoing
problem with GCC, but it does not affect stand-alone applications.  As a
result, you are recommended to use "gcc_dwt_mmx_local.h" for both
32-bit and 64-bit GCC builds, since it imposes no stack alignment
requirements.  For MSVC builds you can use "msvc_dwt_mmx_local.h".  For
.NET builds, you will need the implementation in this file for 64-bit builds,
although both this file and "msvc_dwt_mmx_local.h" will work for 32-bit
builds.
******************************************************************************/

#ifndef X86_DWT_LOCAL_H
#define X86_DWT_LOCAL_H

#include <emmintrin.h>

#define W97_FACT_0 ((float) -1.586134342)
#define W97_FACT_1 ((float) -0.052980118)
#define W97_FACT_2 ((float)  0.882911075)
#define W97_FACT_3 ((float)  0.443506852)

static kdu_int16 simd_w97_rem[4] =
{ (kdu_int16)floor(0.5 + (W97_FACT_0 + 2.0)*(double)(1 << 16)),
 (kdu_int16)floor(0.5 + W97_FACT_1 * (double)(1 << 19)),
 (kdu_int16)floor(0.5 + (W97_FACT_2 - 1.0)*(double)(1 << 16)),
 (kdu_int16)floor(0.5 + W97_FACT_3 * (double)(1 << 16)) };
static kdu_int16 simd_w97_preoff[4] =
{ (kdu_int16)floor(0.5 + 0.5 / (W97_FACT_0 + 2.0)),
 0,
 (kdu_int16)floor(0.5 + 0.5 / (W97_FACT_2 - 1.0)),
 (kdu_int16)floor(0.5 + 0.5 / W97_FACT_3) };


/* ========================================================================= */
/*                  MMX/SSE functions for 16-bit samples                     */
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

	kdu_int32 lambda_coeffs = ((kdu_int32)step->icoeffs[0]) & 0x0000FFFF;
	if (step->support_length == 2)
		lambda_coeffs |= ((kdu_int32)step->icoeffs[1]) << 16;

	__m128i vec_lambda = _mm_set1_epi32(lambda_coeffs);
	__m128i vec_offset = _mm_set1_epi32(step->rounding_offset);
	__m128i downshift = _mm_cvtsi32_si128(step->downshift);
	__m128i mask = _mm_setzero_si128(); // Avoid compiler warnings
	mask = _mm_cmpeq_epi32(mask, mask); // Fill register with 1's
	mask = _mm_srli_epi32(mask, 16); // Leaves mask for low words in each dword
	for (register int c = 0; c < samples; c += 8)
	{
		__m128i val0 = _mm_loadu_si128((__m128i *)(src + c));
		__m128i val1 = _mm_loadu_si128((__m128i *)(src + c + 1));
		val0 = _mm_madd_epi16(val0, vec_lambda);
		val0 = _mm_add_epi32(val0, vec_offset);
		val0 = _mm_sra_epi32(val0, downshift);
		val1 = _mm_madd_epi16(val1, vec_lambda);
		val1 = _mm_add_epi32(val1, vec_offset);
		val1 = _mm_sra_epi32(val1, downshift);
		__m128i tgt = *((__m128i *)(dst + c));
		val0 = _mm_and_si128(val0, mask); // Zero out high words of `val0'
		val1 = _mm_slli_epi32(val1, 16); // Move `val1' results into high words
		tgt = _mm_sub_epi16(tgt, val0);
		tgt = _mm_sub_epi16(tgt, val1);
		*((__m128i *)(dst + c)) = tgt;
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

	kdu_int32 lambda_coeffs0 = ((kdu_int32)step->icoeffs[0]) & 0x0000FFFF;
	lambda_coeffs0 |= ((kdu_int32)step->icoeffs[1]) << 16;
	kdu_int32 lambda_coeffs2 = ((kdu_int32)step->icoeffs[2]) & 0x0000FFFF;
	if (step->support_length == 4)
		lambda_coeffs2 |= ((kdu_int32)step->icoeffs[3]) << 16;

	__m128i downshift = _mm_cvtsi32_si128(step->downshift);
	__m128i vec_offset = _mm_set1_epi32(step->rounding_offset);
	__m128i vec_lambda0 = _mm_set1_epi32(lambda_coeffs0);
	__m128i vec_lambda2 = _mm_set1_epi32(lambda_coeffs2);
	__m128i mask = _mm_setzero_si128(); // Avoid compiler warnings
	mask = _mm_cmpeq_epi32(mask, mask); // Fill register with 1's
	mask = _mm_srli_epi32(mask, 16); // Leaves mask for low words in each dword
	for (register int c = 0; c < samples; c += 8)
	{
		__m128i val0 = _mm_loadu_si128((__m128i *)(src + c));
		__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 2));
		val0 = _mm_madd_epi16(val0, vec_lambda0);
		val2 = _mm_madd_epi16(val2, vec_lambda2);
		val0 = _mm_add_epi32(val0, val2); // Add products
		val0 = _mm_add_epi32(val0, vec_offset); // Add rounding offset
		val0 = _mm_sra_epi32(val0, downshift); // Shift -> results in low words
		val0 = _mm_and_si128(val0, mask); // Zero out high order words
		__m128i val1 = _mm_loadu_si128((__m128i *)(src + c + 1));
		__m128i val3 = _mm_loadu_si128((__m128i *)(src + c + 3));
		val1 = _mm_madd_epi16(val1, vec_lambda0);
		val3 = _mm_madd_epi16(val3, vec_lambda2);
		val1 = _mm_add_epi32(val1, val3); // Add products
		val1 = _mm_add_epi32(val1, vec_offset); // Add rounding offset
		val1 = _mm_sra_epi32(val1, downshift); // Shift -> results in low words
		val1 = _mm_slli_epi32(val1, 16); // Move result to high order words
		__m128i tgt = *((__m128i *)(dst + c)); // Load target value
		val0 = _mm_or_si128(val0, val1); // Put the low and high words together
		tgt = _mm_sub_epi16(tgt, val0);
		*((__m128i *)(dst + c)) = tgt;
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

	kdu_int32 lambda_coeffs = ((kdu_int32)step->icoeffs[0]) & 0x0000FFFF;
	if (step->support_length == 2)
		lambda_coeffs |= ((kdu_int32)step->icoeffs[1]) << 16;

	__m128i vec_lambda = _mm_set1_epi32(lambda_coeffs);
	__m128i vec_offset = _mm_set1_epi32(step->rounding_offset);
	__m128i downshift = _mm_cvtsi32_si128(step->downshift);
	__m128i *sp1 = (__m128i *) src1;
	__m128i *sp2 = (__m128i *) src2;
	__m128i *dp_in = (__m128i *) dst_in;
	__m128i *dp_out = (__m128i *) dst_out;
	samples = (samples + 7) >> 3;
	for (register int c = 0; c < samples; c++)
	{
		__m128i val1 = sp1[c];
		__m128i val2 = sp2[c];
		__m128i high = _mm_unpackhi_epi16(val1, val2);
		__m128i low = _mm_unpacklo_epi16(val1, val2);
		high = _mm_madd_epi16(high, vec_lambda);
		high = _mm_add_epi32(high, vec_offset);
		high = _mm_sra_epi32(high, downshift);
		low = _mm_madd_epi16(low, vec_lambda);
		low = _mm_add_epi32(low, vec_offset);
		low = _mm_sra_epi32(low, downshift);
		__m128i tgt = dp_in[c];
		__m128i subtend = _mm_packs_epi32(low, high);
		tgt = _mm_sub_epi16(tgt, subtend);
		dp_out[c] = tgt;
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

	kdu_int32 lambda_coeffs0 = ((kdu_int32)step->icoeffs[0]) & 0x0000FFFF;
	lambda_coeffs0 |= ((kdu_int32)step->icoeffs[1]) << 16;
	kdu_int32 lambda_coeffs2 = ((kdu_int32)step->icoeffs[2]) & 0x0000FFFF;
	if (step->support_length == 4)
		lambda_coeffs2 |= ((kdu_int32)step->icoeffs[3]) << 16;

	__m128i downshift = _mm_cvtsi32_si128(step->downshift);
	__m128i vec_offset = _mm_set1_epi32(step->rounding_offset);
	__m128i vec_lambda0 = _mm_set1_epi32(lambda_coeffs0);
	__m128i vec_lambda2 = _mm_set1_epi32(lambda_coeffs2);
	for (register int c = 0; c < samples; c += 8)
	{
		__m128i val1 = _mm_load_si128((__m128i *)(src1 + c));
		__m128i val2 = _mm_load_si128((__m128i *)(src2 + c));
		__m128i high0 = _mm_unpackhi_epi16(val1, val2);
		__m128i low0 = _mm_unpacklo_epi16(val1, val2);
		high0 = _mm_madd_epi16(high0, vec_lambda0);
		low0 = _mm_madd_epi16(low0, vec_lambda0);

		__m128i val3 = _mm_load_si128((__m128i *)(src3 + c));
		__m128i val4 = _mm_load_si128((__m128i *)(src4 + c));
		__m128i high1 = _mm_unpackhi_epi16(val3, val4);
		__m128i low1 = _mm_unpacklo_epi16(val3, val4);
		high1 = _mm_madd_epi16(high1, vec_lambda2);
		low1 = _mm_madd_epi16(low1, vec_lambda2);

		__m128i high = _mm_add_epi32(high0, high1);
		high = _mm_add_epi32(high, vec_offset); // Add rounding offset
		high = _mm_sra_epi32(high, downshift);
		__m128i low = _mm_add_epi32(low0, low1);
		low = _mm_add_epi32(low, vec_offset); // Add rounding offset
		low = _mm_sra_epi32(low, downshift);

		__m128i tgt = *((__m128i *)(dst_in + c));
		__m128i subtend = _mm_packs_epi32(low, high);
		tgt = _mm_sub_epi16(tgt, subtend);
		*((__m128i *)(dst_out + c)) = tgt;
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
	assert((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i vec_offset = _mm_set1_epi16((kdu_int16)((1 << step->downshift) >> 1));
		__m128i downshift = _mm_cvtsi32_si128(step->downshift);
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val1 = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				__m128i val = _mm_add_epi16(val1, vec_offset);
				val = _mm_add_epi16(val, val2);
				val = _mm_sra_epi16(val, downshift);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_add_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val1 = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				__m128i val = vec_offset; // Start with offset
				val = _mm_sub_epi16(val, val1);    // Subtract source 1
				val = _mm_sub_epi16(val, val2);  // Subtract source 2
				val = _mm_sra_epi16(val, downshift);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_add_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		else
			assert(0);
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 vec_offset = _mm_set1_pi16((kdu_int16)((1 << step->downshift) >> 1));
		__m64 downshift = _mm_cvtsi32_si64(step->downshift);
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = vec_offset; // Start with offset
				val = _mm_add_pi16(val, *((__m64 *)(src + c)));    // Add source 1
				val = _mm_add_pi16(val, *((__m64 *)(src + c + 1)));  // Add source 2
				val = _mm_sra_pi16(val, downshift);
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_add_pi16(tgt, val);
				*((__m64 *)(dst + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = vec_offset; // Start with offset
				val = _mm_sub_pi16(val, *((__m64 *)(src + c)));    // Subtract source 1
				val = _mm_sub_pi16(val, *((__m64 *)(src + c + 1)));  // Subtract source 2
				val = _mm_sra_pi16(val, downshift);
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_add_pi16(tgt, val);
				*((__m64 *)(dst + c)) = tgt;
			}
		else
			assert(0);
		_mm_empty();
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
}

/*****************************************************************************/
/* INLINE                      simd_W5X3_h_synth                             */
/*****************************************************************************/

static inline bool
simd_W5X3_h_synth(register kdu_int16 *src, register kdu_int16 *dst,
	register int samples, kd_lifting_step *step)
	/* Special function to implement the reversible 5/3 transform */
{
	assert((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i vec_offset = _mm_set1_epi16((kdu_int16)((1 << step->downshift) >> 1));
		__m128i downshift = _mm_cvtsi32_si128(step->downshift);
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val1 = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				__m128i val = _mm_add_epi16(val1, vec_offset);
				val = _mm_add_epi16(val, val2);
				val = _mm_sra_epi16(val, downshift);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val1 = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				__m128i val = vec_offset; // Start with offset
				val = _mm_sub_epi16(val, val1);    // Subtract source 1
				val = _mm_sub_epi16(val, val2);  // Subtract source 2
				val = _mm_sra_epi16(val, downshift);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		else
			assert(0);
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 vec_offset = _mm_set1_pi16((kdu_int16)((1 << step->downshift) >> 1));
		__m64 downshift = _mm_cvtsi32_si64(step->downshift);
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = vec_offset; // Start with offset
				val = _mm_add_pi16(val, *((__m64 *)(src + c)));    // Add source 1
				val = _mm_add_pi16(val, *((__m64 *)(src + c + 1)));  // Add source 2
				val = _mm_sra_pi16(val, downshift);
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = vec_offset; // Start with offset
				val = _mm_sub_pi16(val, *((__m64 *)(src + c)));    // Subtract source 1
				val = _mm_sub_pi16(val, *((__m64 *)(src + c + 1)));  // Subtract source 2
				val = _mm_sra_pi16(val, downshift);
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst + c)) = tgt;
			}
		else
			assert(0);
		_mm_empty();
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
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
	assert((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i downshift = _mm_cvtsi32_si128(step->downshift);
		__m128i vec_offset = _mm_set1_epi16((kdu_int16)((1 << step->downshift) >> 1));
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = vec_offset;  // Start with the offset
				val = _mm_add_epi16(val, *((__m128i *)(src1 + c))); // Add source 1
				val = _mm_add_epi16(val, *((__m128i *)(src2 + c))); // Add source 2
				val = _mm_sra_epi16(val, downshift);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_add_epi16(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = vec_offset;  // Start with the offset
				val = _mm_sub_epi16(val, *((__m128i *)(src1 + c))); // Subtract source 1
				val = _mm_sub_epi16(val, *((__m128i *)(src2 + c))); // Subtract source 2
				val = _mm_sra_epi16(val, downshift);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_add_epi16(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		else
			assert(0);
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 downshift = _mm_cvtsi32_si64(step->downshift);
		__m64 vec_offset = _mm_set1_pi16((kdu_int16)((1 << step->downshift) >> 1));
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = vec_offset;  // Start with the offset
				val = _mm_add_pi16(val, *((__m64 *)(src1 + c))); // Add source 1
				val = _mm_add_pi16(val, *((__m64 *)(src2 + c))); // Add source 2
				val = _mm_sra_pi16(val, downshift);
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_add_pi16(tgt, val);
				*((__m64 *)(dst_out + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = vec_offset;  // Start with the offset
				val = _mm_sub_pi16(val, *((__m64 *)(src1 + c))); // Subtract source 1
				val = _mm_sub_pi16(val, *((__m64 *)(src2 + c))); // Subtract source 2
				val = _mm_sra_pi16(val, downshift);
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_add_pi16(tgt, val);
				*((__m64 *)(dst_out + c)) = tgt;
			}
		else
			assert(0);
		_mm_empty();
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
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
	assert((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i downshift = _mm_cvtsi32_si128(step->downshift);
		__m128i vec_offset = _mm_set1_epi16((kdu_int16)((1 << step->downshift) >> 1));
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = vec_offset;  // Start with the offset
				val = _mm_add_epi16(val, *((__m128i *)(src1 + c))); // Add source 1
				val = _mm_add_epi16(val, *((__m128i *)(src2 + c))); // Add source 2
				val = _mm_sra_epi16(val, downshift);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = vec_offset;  // Start with the offset
				val = _mm_sub_epi16(val, *((__m128i *)(src1 + c))); // Subtract src 1
				val = _mm_sub_epi16(val, *((__m128i *)(src2 + c))); // Subtract src 2
				val = _mm_sra_epi16(val, downshift);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		else
			assert(0);
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 downshift = _mm_cvtsi32_si64(step->downshift);
		__m64 vec_offset = _mm_set1_pi16((kdu_int16)((1 << step->downshift) >> 1));
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = vec_offset;  // Start with the offset
				val = _mm_add_pi16(val, *((__m64 *)(src1 + c))); // Add source 1
				val = _mm_add_pi16(val, *((__m64 *)(src2 + c))); // Add source 2
				val = _mm_sra_pi16(val, downshift);
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst_out + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = vec_offset;  // Start with the offset
				val = _mm_sub_pi16(val, *((__m64 *)(src1 + c))); // Subtract source 1
				val = _mm_sub_pi16(val, *((__m64 *)(src2 + c))); // Subtract source 2
				val = _mm_sra_pi16(val, downshift);
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst_out + c)) = tgt;
			}
		else
			assert(0);
		_mm_empty();
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
}

/*****************************************************************************/
/* INLINE                   simd_W9X7_h_analysis                             */
/*****************************************************************************/

static inline bool
simd_W9X7_h_analysis(register kdu_int16 *src, register kdu_int16 *dst,
	register int samples, kd_lifting_step *step)
	/* Special function to implement the irreversible 9/7 transform. */
{
	int step_idx = step->step_idx;
	assert((step_idx >= 0) && (step_idx < 4));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i vec_lambda = _mm_set1_epi16(simd_w97_rem[step_idx]);
		__m128i vec_offset = _mm_set1_epi16(simd_w97_preoff[step_idx]);
		if (step_idx == 0)
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				val = _mm_add_epi16(val, val2);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_sub_epi16(tgt, val); // Here is a -1 contribution
				tgt = _mm_sub_epi16(tgt, val); // Another -1 contribution
				val = _mm_add_epi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_epi16(tgt, val); // Final contribution
				*((__m128i *)(dst + c)) = tgt;
			}
		}
		else if (step_idx == 1)
		{
			__m128i roff = _mm_setzero_si128();
			__m128i tmp = _mm_cmpeq_epi16(roff, roff); // Set to all 1's
			roff = _mm_sub_epi16(roff, tmp); // Leaves each word in `roff' = 1
			roff = _mm_slli_epi16(roff, 2); // Leave each word in `roff' = 4
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i tmp = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val1 = _mm_setzero_si128();
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				val2 = _mm_mulhi_epi16(val2, vec_lambda);
				val1 = _mm_sub_epi16(val1, tmp);
				val1 = _mm_mulhi_epi16(val1, vec_lambda);
				__m128i val = _mm_sub_epi16(val2, val1); // +ve minus -ve source
				val = _mm_add_epi16(val, roff); // Add rounding offset
				val = _mm_srai_epi16(val, 3);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_add_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		}
		else if (step_idx == 2)
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				val = _mm_add_epi16(val, val2);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_add_epi16(tgt, val); // Here is a +1 contribution
				val = _mm_add_epi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda);
				tgt = _mm_add_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		}
		else
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				val = _mm_add_epi16(val, val2);
				__m128i tgt = *((__m128i *)(dst + c));
				val = _mm_add_epi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda);
				tgt = _mm_add_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		}
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 vec_lambda = _mm_set1_pi16(simd_w97_rem[step_idx]);
		__m64 vec_offset = _mm_set1_pi16(simd_w97_preoff[step_idx]);
		if (step_idx == 0)
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src + c));
				val = _mm_add_pi16(val, *((__m64 *)(src + c + 1)));
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_sub_pi16(tgt, val); // Here is a -1 contribution
				tgt = _mm_sub_pi16(tgt, val); // Another -1 contribution
				val = _mm_add_pi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_pi16(tgt, val); // Final contribution
				*((__m64 *)(dst + c)) = tgt;
			}
		}
		else if (step_idx == 1)
		{
			__m64 roff = _mm_setzero_si64();
			__m64 tmp = _mm_cmpeq_pi16(roff, roff); // Set to all 1's
			roff = _mm_sub_pi16(roff, tmp); // Leaves each word in `roff' equal to 1
			roff = _mm_slli_pi16(roff, 2); // Leave each word in `roff' = 4
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val1 = *((__m64 *)(src + c));                 // Get +ve source 1
				val1 = _mm_mulhi_pi16(val1, vec_lambda); // * lambda & discard 16 LSBs
				__m64 val2 = _mm_setzero_si64();
				val2 = _mm_sub_pi16(val2, *((__m64 *)(src + c + 1))); // Get -ve source 2
				val2 = _mm_mulhi_pi16(val2, vec_lambda); // * lambda & discard 16 LSBs
				__m64 val = _mm_sub_pi16(val1, val2); // Subract -ve from +ve source
				val = _mm_add_pi16(val, roff); // Add rounding offset
				val = _mm_srai_pi16(val, 3); // Shift result to the right by 3
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_add_pi16(tgt, val); // Update destination samples
				*((__m64 *)(dst + c)) = tgt;
			}
		}
		else if (step_idx == 2)
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src + c));
				val = _mm_add_pi16(val, *((__m64 *)(src + c + 1)));
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_add_pi16(tgt, val); // Here is a +1 contribution
				val = _mm_add_pi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_pi16(tgt, val); // Final contribution
				*((__m64 *)(dst + c)) = tgt;
			}
		}
		else
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src + c));
				val = _mm_add_pi16(val, *((__m64 *)(src + c + 1)));
				__m64 tgt = *((__m64 *)(dst + c));
				val = _mm_add_pi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_pi16(tgt, val); // Final contribution
				*((__m64 *)(dst + c)) = tgt;
			}
		}
		_mm_empty(); // Clear MMX registers for use by FPU
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
}

/*****************************************************************************/
/* INLINE                     simd_W9X7_h_synth                              */
/*****************************************************************************/

static inline bool
simd_W9X7_h_synth(register kdu_int16 *src, register kdu_int16 *dst,
	register int samples, kd_lifting_step *step)
	/* Special function to implement the irreversible 9/7 transform. */
{
	int step_idx = step->step_idx;
	assert((step_idx >= 0) && (step_idx < 4));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i vec_lambda = _mm_set1_epi16(simd_w97_rem[step_idx]);
		__m128i vec_offset = _mm_set1_epi16(simd_w97_preoff[step_idx]);
		if (step_idx == 0)
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				val = _mm_add_epi16(val, val2);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_add_epi16(tgt, val); // Here is a -1 contribution
				tgt = _mm_add_epi16(tgt, val); // Another -1 contribution
				val = _mm_add_epi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda);
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		}
		else if (step_idx == 1)
		{
			__m128i roff = _mm_setzero_si128();
			__m128i tmp = _mm_cmpeq_epi16(roff, roff); // Set to all 1's
			roff = _mm_sub_epi16(roff, tmp); // Leaves each word in `roff' = 1
			roff = _mm_slli_epi16(roff, 2); // Leave each word in `roff' = 4
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i tmp = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val1 = _mm_setzero_si128();
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				val2 = _mm_mulhi_epi16(val2, vec_lambda);
				val1 = _mm_sub_epi16(val1, tmp);
				val1 = _mm_mulhi_epi16(val1, vec_lambda);
				__m128i val = _mm_sub_epi16(val2, val1); // +ve minus -ve source
				val = _mm_add_epi16(val, roff); // Add rounding offset
				val = _mm_srai_epi16(val, 3);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		}
		else if (step_idx == 2)
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				val = _mm_add_epi16(val, val2);
				__m128i tgt = *((__m128i *)(dst + c));
				tgt = _mm_sub_epi16(tgt, val); // Here is a +1 contribution
				val = _mm_add_epi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda);
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		}
		else
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = _mm_loadu_si128((__m128i *)(src + c));
				__m128i val2 = _mm_loadu_si128((__m128i *)(src + c + 1));
				val = _mm_add_epi16(val, val2);
				__m128i tgt = *((__m128i *)(dst + c));
				val = _mm_add_epi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda);
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst + c)) = tgt;
			}
		}
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 vec_lambda = _mm_set1_pi16(simd_w97_rem[step_idx]);
		__m64 vec_offset = _mm_set1_pi16(simd_w97_preoff[step_idx]);
		if (step_idx == 0)
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src + c));
				val = _mm_add_pi16(val, *((__m64 *)(src + c + 1)));
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_add_pi16(tgt, val); // Here is a -1 contribution
				tgt = _mm_add_pi16(tgt, val); // Another -1 contribution
				val = _mm_add_pi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda);
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst + c)) = tgt;
			}
		}
		else if (step_idx == 1)
		{
			__m64 roff = _mm_setzero_si64();
			__m64 tmp = _mm_cmpeq_pi16(roff, roff); // Set to all 1's
			roff = _mm_sub_pi16(roff, tmp); // Leaves each word in `roff' = 1
			roff = _mm_slli_pi16(roff, 2); // Leave each word in `roff' = 4
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val1 = *((__m64 *)(src + c));
				val1 = _mm_mulhi_pi16(val1, vec_lambda);
				__m64 val2 = _mm_setzero_si64();
				val2 = _mm_sub_pi16(val2, *((__m64 *)(src + c + 1)));
				val2 = _mm_mulhi_pi16(val2, vec_lambda);
				__m64 val = _mm_sub_pi16(val1, val2); // +ve minus -ve source
				val = _mm_add_pi16(val, roff); // Add rounding offset
				val = _mm_srai_pi16(val, 3);
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst + c)) = tgt;
			}
		}
		else if (step_idx == 2)
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src + c));
				val = _mm_add_pi16(val, *((__m64 *)(src + c + 1)));
				__m64 tgt = *((__m64 *)(dst + c));
				tgt = _mm_sub_pi16(tgt, val); // Here is a +1 contribution
				val = _mm_add_pi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda);
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst + c)) = tgt;
			}
		}
		else
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src + c));
				val = _mm_add_pi16(val, *((__m64 *)(src + c + 1)));
				__m64 tgt = *((__m64 *)(dst + c));
				val = _mm_add_pi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda);
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst + c)) = tgt;
			}
		}
		_mm_empty(); // Clear MMX registers for use by FPU
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
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
	int step_idx = step->step_idx;
	assert((step_idx >= 0) && (step_idx < 4));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i vec_lambda = _mm_set1_epi16(simd_w97_rem[step_idx]);
		__m128i vec_offset = _mm_set1_epi16(simd_w97_preoff[step_idx]);
		if (step_idx == 0)
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = *((__m128i *)(src1 + c));
				val = _mm_add_epi16(val, *((__m128i *)(src2 + c)));
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_sub_epi16(tgt, val); // Here is a -1 contribution
				tgt = _mm_sub_epi16(tgt, val); // Another -1 contribution
				val = _mm_add_epi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_epi16(tgt, val); // Final contribution
				*((__m128i *)(dst_out + c)) = tgt;
			}
		}
		else if (step_idx == 1)
		{
			__m128i roff = _mm_setzero_si128();
			__m128i tmp = _mm_cmpeq_epi16(roff, roff); // Set to all 1's
			roff = _mm_sub_epi16(roff, tmp); // Leaves each word in `roff' equal to 1
			roff = _mm_slli_epi16(roff, 2); // Leave each word in `roff' = 4
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val1 = *((__m128i *)(src1 + c));      // Get +ve source 1
				val1 = _mm_mulhi_epi16(val1, vec_lambda); // * lambda & discard 16 LSBs
				__m128i val2 = _mm_setzero_si128();
				val2 = _mm_sub_epi16(val2, *((__m128i *)(src2 + c))); // Get -ve source 2
				val2 = _mm_mulhi_epi16(val2, vec_lambda); // * lambda & discard 16 LSBs
				__m128i val = _mm_sub_epi16(val1, val2); // Subract -ve from +ve source
				val = _mm_add_epi16(val, roff); // Add rounding offset
				val = _mm_srai_epi16(val, 3); // Shift result to the right by 3
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_add_epi16(tgt, val); // Update destination samples
				*((__m128i *)(dst_out + c)) = tgt;
			}
		}
		else if (step_idx == 2)
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = *((__m128i *)(src1 + c));
				val = _mm_add_epi16(val, *((__m128i *)(src2 + c)));
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_add_epi16(tgt, val); // Here is a +1 contribution
				val = _mm_add_epi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_epi16(tgt, val); // Final contribution
				*((__m128i *)(dst_out + c)) = tgt;
			}
		}
		else
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = *((__m128i *)(src1 + c));
				val = _mm_add_epi16(val, *((__m128i *)(src2 + c)));
				__m128i tgt = *((__m128i *)(dst_in + c));
				val = _mm_add_epi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_epi16(tgt, val); // Final contribution
				*((__m128i *)(dst_out + c)) = tgt;
			}
		}
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 vec_lambda = _mm_set1_pi16(simd_w97_rem[step_idx]);
		__m64 vec_offset = _mm_set1_pi16(simd_w97_preoff[step_idx]);
		if (step_idx == 0)
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src1 + c));
				val = _mm_add_pi16(val, *((__m64 *)(src2 + c)));
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_sub_pi16(tgt, val); // Here is a -1 contribution
				tgt = _mm_sub_pi16(tgt, val); // Another -1 contribution
				val = _mm_add_pi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_pi16(tgt, val); // Final contribution
				*((__m64 *)(dst_out + c)) = tgt;
			}
		}
		else if (step_idx == 1)
		{
			__m64 roff = _mm_setzero_si64();
			__m64 tmp = _mm_cmpeq_pi16(roff, roff); // Set to all 1's
			roff = _mm_sub_pi16(roff, tmp); // Leaves each word in `roff' equal to 1
			roff = _mm_slli_pi16(roff, 2); // Leave each word in `roff' = 4
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val1 = *((__m64 *)(src1 + c));               // Get +ve source 1
				val1 = _mm_mulhi_pi16(val1, vec_lambda); // * lambda & discard 16 LSBs
				__m64 val2 = _mm_setzero_si64();
				val2 = _mm_sub_pi16(val2, *((__m64 *)(src2 + c))); // Get -ve source 2
				val2 = _mm_mulhi_pi16(val2, vec_lambda); // * lambda & discard 16 LSBs
				__m64 val = _mm_sub_pi16(val1, val2); // Subract -ve from +ve source
				val = _mm_add_pi16(val, roff); // Add rounding offset
				val = _mm_srai_pi16(val, 3); // Shift result to the right by 3
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_add_pi16(tgt, val); // Update destination samples
				*((__m64 *)(dst_out + c)) = tgt;
			}
		}
		else if (step_idx == 2)
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src1 + c));
				val = _mm_add_pi16(val, *((__m64 *)(src2 + c)));
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_add_pi16(tgt, val); // Here is a +1 contribution
				val = _mm_add_pi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_pi16(tgt, val); // Final contribution
				*((__m64 *)(dst_out + c)) = tgt;
			}
		}
		else
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src1 + c));
				val = _mm_add_pi16(val, *((__m64 *)(src2 + c)));
				__m64 tgt = *((__m64 *)(dst_in + c));
				val = _mm_add_pi16(val, vec_offset); // Add the rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda); // * lambda & discard 16 LSBs
				tgt = _mm_add_pi16(tgt, val); // Final contribution
				*((__m64 *)(dst_out + c)) = tgt;
			}
		}
		_mm_empty(); // Clear MMX registers for use by FPU
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
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
	int step_idx = step->step_idx;
	assert((step_idx >= 0) && (step_idx < 4));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i vec_lambda = _mm_set1_epi16(simd_w97_rem[step_idx]);
		__m128i vec_offset = _mm_set1_epi16(simd_w97_preoff[step_idx]);
		if (step_idx == 0)
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = *((__m128i *)(src1 + c));
				val = _mm_add_epi16(val, *((__m128i *)(src2 + c)));
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_add_epi16(tgt, val); // Here is a -1 contribution
				tgt = _mm_add_epi16(tgt, val); // Another -1 contribution
				val = _mm_add_epi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda);
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		}
		else if (step_idx == 1)
		{
			__m128i roff = _mm_setzero_si128();
			__m128i tmp = _mm_cmpeq_epi16(roff, roff); // Set to all 1's
			roff = _mm_sub_epi16(roff, tmp); // Leaves each word in `roff' = 1
			roff = _mm_slli_epi16(roff, 2); // Leave each word in `roff' = 4
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val1 = *((__m128i *)(src1 + c));
				val1 = _mm_mulhi_epi16(val1, vec_lambda);
				__m128i val2 = _mm_setzero_si128();
				val2 = _mm_sub_epi16(val2, *((__m128i *)(src2 + c)));
				val2 = _mm_mulhi_epi16(val2, vec_lambda);
				__m128i val = _mm_sub_epi16(val1, val2); // +ve minus -ve source
				val = _mm_add_epi16(val, roff); // Add rounding offset
				val = _mm_srai_epi16(val, 3);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		}
		else if (step_idx == 2)
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = *((__m128i *)(src1 + c));
				val = _mm_add_epi16(val, *((__m128i *)(src2 + c)));
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_sub_epi16(tgt, val); // Here is a +1 contribution
				val = _mm_add_epi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda);
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		}
		else
		{
			for (register int c = 0; c < samples; c += 8)
			{
				__m128i val = *((__m128i *)(src1 + c));
				val = _mm_add_epi16(val, *((__m128i *)(src2 + c)));
				__m128i tgt = *((__m128i *)(dst_in + c));
				val = _mm_add_epi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_epi16(val, vec_lambda);
				tgt = _mm_sub_epi16(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		}
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 vec_lambda = _mm_set1_pi16(simd_w97_rem[step_idx]);
		__m64 vec_offset = _mm_set1_pi16(simd_w97_preoff[step_idx]);
		if (step_idx == 0)
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src1 + c));
				val = _mm_add_pi16(val, *((__m64 *)(src2 + c)));
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_add_pi16(tgt, val); // Here is a -1 contribution
				tgt = _mm_add_pi16(tgt, val); // Another -1 contribution
				val = _mm_add_pi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda);
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst_out + c)) = tgt;
			}
		}
		else if (step_idx == 1)
		{
			__m64 roff = _mm_setzero_si64();
			__m64 tmp = _mm_cmpeq_pi16(roff, roff); // Set to all 1's
			roff = _mm_sub_pi16(roff, tmp); // Leaves each word in `roff' = 1
			roff = _mm_slli_pi16(roff, 2); // Leave each word in `roff' = 4
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val1 = *((__m64 *)(src1 + c));
				val1 = _mm_mulhi_pi16(val1, vec_lambda);
				__m64 val2 = _mm_setzero_si64();
				val2 = _mm_sub_pi16(val2, *((__m64 *)(src2 + c)));
				val2 = _mm_mulhi_pi16(val2, vec_lambda);
				__m64 val = _mm_sub_pi16(val1, val2); // +ve minus -ve source
				val = _mm_add_pi16(val, roff); // Add rounding offset
				val = _mm_srai_pi16(val, 3);
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst_out + c)) = tgt;
			}
		}
		else if (step_idx == 2)
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src1 + c));
				val = _mm_add_pi16(val, *((__m64 *)(src2 + c)));
				__m64 tgt = *((__m64 *)(dst_in + c));
				tgt = _mm_sub_pi16(tgt, val); // Here is a +1 contribution
				val = _mm_add_pi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda);
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst_out + c)) = tgt;
			}
		}
		else
		{
			for (register int c = 0; c < samples; c += 4)
			{
				__m64 val = *((__m64 *)(src1 + c));
				val = _mm_add_pi16(val, *((__m64 *)(src2 + c)));
				__m64 tgt = *((__m64 *)(dst_in + c));
				val = _mm_add_pi16(val, vec_offset); // Add rounding pre-offset
				val = _mm_mulhi_pi16(val, vec_lambda);
				tgt = _mm_sub_pi16(tgt, val);
				*((__m64 *)(dst_out + c)) = tgt;
			}
		}
		_mm_empty(); // Clear MMX registers for use by FPU
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
}

/*****************************************************************************/
/* INLINE                       simd_deinterleave                            */
/*****************************************************************************/

static inline bool
simd_deinterleave(register kdu_int16 *src, register kdu_int16 *dst1,
	register kdu_int16 *dst2, register int pairs)
{
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i *sp = (__m128i *) src;
		__m128i *dp1 = (__m128i *) dst1;
		__m128i *dp2 = (__m128i *) dst2;
		for (; pairs > 4; pairs -= 8, sp += 2, dp1++, dp2++)
		{
			__m128i val1 = sp[0];
			__m128i val2 = sp[1];
			__m128i low1 = _mm_slli_epi32(val1, 16);
			low1 = _mm_srai_epi32(low1, 16);
			__m128i low2 = _mm_slli_epi32(val2, 16);
			low2 = _mm_srai_epi32(low2, 16);
			*dp1 = _mm_packs_epi32(low1, low2);
			__m128i high1 = _mm_srai_epi32(val1, 16);
			__m128i high2 = _mm_srai_epi32(val2, 16);
			*dp2 = _mm_packs_epi32(high1, high2);
		}
		if (pairs > 0)
		{ // Need to read one more group of 4 pairs
			__m128i val1 = sp[0];
			__m128i low1 = _mm_slli_epi32(val1, 16);
			low1 = _mm_srai_epi32(low1, 16);
			*dp1 = _mm_packs_epi32(low1, low1);
			__m128i high1 = _mm_srai_epi32(val1, 16);
			*dp2 = _mm_packs_epi32(high1, high1);
		}
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 *sp = (__m64 *) src;
		__m64 *dp1 = (__m64 *) dst1;
		__m64 *dp2 = (__m64 *) dst2;
		for (; pairs > 0; pairs -= 4, sp += 2, dp1++, dp2++)
		{
			__m64 val1 = sp[0];
			__m64 val2 = sp[1];
			__m64 low1 = _mm_slli_pi32(val1, 16);
			low1 = _mm_srai_pi32(low1, 16); // Leaves sign extended words 0 & 2
			__m64 low2 = _mm_slli_pi32(val2, 16);
			low2 = _mm_srai_pi32(low2, 16); // Leaves sign extended words 4 & 6
			*dp1 = _mm_packs_pi32(low1, low2); // Packs and saves words 0,2,4,6
			__m64 high1 = _mm_srai_pi32(val1, 16); // Leaves sign extended words 1 & 3
			__m64 high2 = _mm_srai_pi32(val2, 16); // Leaves sign extended words 5 & 7
			*dp2 = _mm_packs_pi32(high1, high2); // Packs and saves words 1,3,5,7
		}
		_mm_empty(); // Clear MMX registers for use by FPU
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
}

/*****************************************************************************/
/* INLINE                        simd_interleave                             */
/*****************************************************************************/

static inline bool
simd_interleave(register kdu_int16 *src1, register kdu_int16 *src2,
	register kdu_int16 *dst, register int pairs)
{
#ifndef KDU_NO_SSE
	if ((kdu_mmx_level >= 2) && (pairs >= 32))
	{ // Implementation based on 128-bit operands, using SSE/SSE2 instructions
		if (_addr_to_kdu_int32(src1) & 8)
		{ // Source addresses are 8-byte aligned, but not 16-byte aligned
			__m128i val1 = *((__m128i *)(src1 - 4));
			__m128i val2 = *((__m128i *)(src2 - 4));
			*((__m128i *) dst) = _mm_unpackhi_epi16(val1, val2);
			src1 += 4; src2 += 4; dst += 8; pairs -= 4;
		}
		__m128i *sp1 = (__m128i *) src1;
		__m128i *sp2 = (__m128i *) src2;
		__m128i *dp = (__m128i *) dst;
		for (; pairs > 4; pairs -= 8, sp1++, sp2++, dp += 2)
		{
			__m128i val1 = *sp1;
			__m128i val2 = *sp2;
			dp[0] = _mm_unpacklo_epi16(val1, val2);
			dp[1] = _mm_unpackhi_epi16(val1, val2);
		}
		if (pairs > 0)
		{ // Need to generate one more group of 8 outputs (4 pairs)
			__m128i val1 = *sp1;
			__m128i val2 = *sp2;
			dp[0] = _mm_unpacklo_epi16(val1, val2);
		}
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 *sp1 = (__m64 *) src1;
		__m64 *sp2 = (__m64 *) src2;
		__m64 *dp = (__m64 *) dst;
		for (; pairs > 0; pairs -= 4, sp1++, sp2++, dp += 2)
		{
			__m64 val1 = *sp1;
			__m64 val2 = *sp2;
			dp[0] = _mm_unpacklo_pi16(val1, val2);
			dp[1] = _mm_unpackhi_pi16(val1, val2);
		}
		_mm_empty(); // Clear MMX registers for use by FPU
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
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
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i shift = _mm_cvtsi32_si128(downshift);
		__m128i vec_offset = _mm_set1_epi16((kdu_int16)((1 << downshift) >> 1));
		__m128i *sp = (__m128i *) src;
		__m128i *dp1 = (__m128i *) dst1;
		__m128i *dp2 = (__m128i *) dst2;
		for (; pairs > 4; pairs -= 8, sp += 2, dp1++, dp2++)
		{
			__m128i val1 = sp[0];
			val1 = _mm_add_epi16(val1, vec_offset);
			val1 = _mm_sra_epi16(val1, shift);
			__m128i val2 = sp[1];
			val2 = _mm_add_epi16(val2, vec_offset);
			val2 = _mm_sra_epi16(val2, shift);
			__m128i low1 = _mm_slli_epi32(val1, 16);
			low1 = _mm_srai_epi32(low1, 16);
			__m128i low2 = _mm_slli_epi32(val2, 16);
			low2 = _mm_srai_epi32(low2, 16);
			*dp1 = _mm_packs_epi32(low1, low2);
			__m128i high1 = _mm_srai_epi32(val1, 16);
			__m128i high2 = _mm_srai_epi32(val2, 16);
			*dp2 = _mm_packs_epi32(high1, high2);
		}
		if (pairs > 0)
		{ // Need to read one more group of 4 pairs
			__m128i val1 = sp[0];
			val1 = _mm_add_epi16(val1, vec_offset);
			val1 = _mm_sra_epi16(val1, shift);
			__m128i low1 = _mm_slli_epi32(val1, 16);
			low1 = _mm_srai_epi32(low1, 16);
			*dp1 = _mm_packs_epi32(low1, low1);
			__m128i high1 = _mm_srai_epi32(val1, 16);
			*dp2 = _mm_packs_epi32(high1, high1);
		}
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 shift = _mm_cvtsi32_si64(downshift);
		__m64 vec_offset = _mm_set1_pi16((kdu_int16)((1 << downshift) >> 1));
		__m64 *sp = (__m64 *) src;
		__m64 *dp1 = (__m64 *) dst1;
		__m64 *dp2 = (__m64 *) dst2;
		for (; pairs > 0; pairs -= 4, sp += 2, dp1++, dp2++)
		{
			__m64 val1 = sp[0];
			val1 = _mm_add_pi16(val1, vec_offset);
			val1 = _mm_sra_pi16(val1, shift);
			__m64 val2 = sp[1];
			val2 = _mm_add_pi16(val2, vec_offset);
			val2 = _mm_sra_pi16(val2, shift);
			__m64 low1 = _mm_slli_pi32(val1, 16);
			low1 = _mm_srai_pi32(low1, 16); // Leaves sign extended words 0 & 2
			__m64 low2 = _mm_slli_pi32(val2, 16);
			low2 = _mm_srai_pi32(low2, 16); // Leaves sign extended words 4 & 6
			*dp1 = _mm_packs_pi32(low1, low2); // Packs and saves words 0,2,4,6
			__m64 high1 = _mm_srai_pi32(val1, 16); // Leaves sign extended words 1 & 3
			__m64 high2 = _mm_srai_pi32(val2, 16); // Leaves sign extended words 5 & 7
			*dp2 = _mm_packs_pi32(high1, high2); // Packs and saves words 1,3,5,7
		}
		_mm_empty(); // Clear MMX registers for use by FPU
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
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
#ifndef KDU_NO_SSE
	if ((kdu_mmx_level >= 2) && (pairs >= 32))
	{ // Implementation based on 128-bit operands, using SSE/SSE2 instructions
		__m128i shift = _mm_cvtsi32_si128(upshift);
		if (_addr_to_kdu_int32(src1) & 8)
		{ // Source addresses are 8-byte aligned, but not 16-byte aligned
			__m128i val1 = *((__m128i *)(src1 - 4));
			val1 = _mm_sll_epi16(val1, shift);
			__m128i val2 = *((__m128i *)(src2 - 4));
			val2 = _mm_sll_epi16(val2, shift);
			*((__m128i *) dst) = _mm_unpackhi_epi16(val1, val2);
			src1 += 4; src2 += 4; dst += 8; pairs -= 4;
		}
		__m128i *sp1 = (__m128i *) src1;
		__m128i *sp2 = (__m128i *) src2;
		__m128i *dp = (__m128i *) dst;
		for (; pairs > 4; pairs -= 8, sp1++, sp2++, dp += 2)
		{
			__m128i val1 = *sp1;
			val1 = _mm_sll_epi16(val1, shift);
			__m128i val2 = *sp2;
			val2 = _mm_sll_epi16(val2, shift);
			dp[0] = _mm_unpacklo_epi16(val1, val2);
			dp[1] = _mm_unpackhi_epi16(val1, val2);
		}
		if (pairs > 0)
		{ // Need to generate one more group of 8 outputs (4 pairs)
			__m128i val1 = *sp1;
			val1 = _mm_sll_epi16(val1, shift);
			__m128i val2 = *sp2;
			val2 = _mm_sll_epi16(val2, shift);
			dp[0] = _mm_unpacklo_epi16(val1, val2);
		}
		return true;
	}
#endif // !KDU_NO_SSE
#ifndef KDU_NO_MMX64
	if (kdu_mmx_level >= 1)
	{ // 64-bit implementation, using MMX instructions only
		__m64 shift = _mm_cvtsi32_si64(upshift);
		__m64 *sp1 = (__m64 *) src1;
		__m64 *sp2 = (__m64 *) src2;
		__m64 *dp = (__m64 *) dst;
		for (; pairs > 0; pairs -= 4, sp1++, sp2++, dp += 2)
		{
			__m64 val1 = *sp1;
			val1 = _mm_sll_pi16(val1, shift);
			__m64 val2 = *sp2;
			val2 = _mm_sll_pi16(val2, shift);
			dp[0] = _mm_unpacklo_pi16(val1, val2);
			dp[1] = _mm_unpackhi_pi16(val1, val2);
		}
		_mm_empty(); // Clear MMX registers for use by FPU
		return true;
	}
#endif // !KDU_NO_MMX64
	return false;
}

/* ========================================================================= */
/*                  MMX/SSE functions for 32-bit samples                     */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                    simd_2tap_h_irrev32                             */
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

	int quad_bytes = ((samples + 3) & ~3) << 2;
	float lambda0 = step->coeffs[0];
	float lambda1 = (step->support_length == 2) ? (step->coeffs[1]) : 0.0F;
	register __m128 *dp = (__m128 *) dst; // Always aligned
	register __m128 *dp_lim = (__m128 *)(((kdu_byte *)dp) + quad_bytes);
	__m128 val0 = _mm_loadu_ps(src); // Pre-load first operand
	__m128 val1 = _mm_loadu_ps(src + 1); // Pre-load second operand
	__m128 vec_lambda0, vec_lambda1;
	if (synthesis)
	{
		vec_lambda0 = _mm_set1_ps(-lambda0); vec_lambda1 = _mm_set1_ps(-lambda1);
	}
	else
	{
		vec_lambda0 = _mm_set1_ps(lambda0); vec_lambda1 = _mm_set1_ps(lambda1);
	}
	for (; dp < dp_lim; dp++, src += 4)
	{
		__m128 prod0 = _mm_mul_ps(val0, vec_lambda0);
		__m128 prod1 = _mm_mul_ps(val1, vec_lambda1);
		__m128 tgt = *dp;
		val0 = _mm_loadu_ps(src + 4);
		prod0 = _mm_add_ps(prod0, prod1);
		val1 = _mm_loadu_ps(src + 5);
		*dp = _mm_add_ps(tgt, prod0);
	}
	return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                    simd_4tap_h_irrev32                             */
/*****************************************************************************/

static inline bool
simd_4tap_h_irrev32(float *src, float *dst, int samples,
	kd_lifting_step *step, bool synthesis)
	/* Similar to `simd_4tap_h_synth', except this function works with 32-bit
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

	int quad_bytes = ((samples + 3) & ~3) << 2;
	float lambda0 = step->coeffs[0];
	float lambda1 = step->coeffs[1];
	float lambda2 = step->coeffs[2];
	float lambda3 = (step->support_length == 4) ? (step->coeffs[3]) : 0.0F;
	register __m128 *dp = (__m128 *) dst; // Always aligned
	register __m128 *dp_lim = (__m128 *)(((kdu_byte *)dp) + quad_bytes);
	__m128 val0 = _mm_loadu_ps(src); // Pre-load first operand
	__m128 val1 = _mm_loadu_ps(src + 1); // Pre-load second operand
	__m128 val2 = _mm_loadu_ps(src + 2); // Pre-load second operand
	__m128 val3 = _mm_loadu_ps(src + 3); // Pre-load second operand
	__m128 vec_lambda0, vec_lambda1, vec_lambda2, vec_lambda3;
	if (synthesis)
	{
		vec_lambda0 = _mm_set1_ps(-lambda0); vec_lambda1 = _mm_set1_ps(-lambda1);
		vec_lambda2 = _mm_set1_ps(-lambda2); vec_lambda3 = _mm_set1_ps(-lambda3);
	}
	else
	{
		vec_lambda0 = _mm_set1_ps(lambda0); vec_lambda1 = _mm_set1_ps(lambda1);
		vec_lambda2 = _mm_set1_ps(lambda2); vec_lambda3 = _mm_set1_ps(lambda3);
	}
	for (; dp < dp_lim; dp++, src += 4)
	{
		__m128 prod0 = _mm_mul_ps(val0, vec_lambda0);
		__m128 prod1 = _mm_mul_ps(val1, vec_lambda1);
		prod0 = _mm_add_ps(prod0, prod1);
		__m128 prod2 = _mm_mul_ps(val2, vec_lambda2);
		__m128 prod3 = _mm_mul_ps(val3, vec_lambda3);
		__m128 tgt = *dp;
		prod2 = _mm_add_ps(prod2, prod3);
		val0 = _mm_loadu_ps(src + 4);
		val1 = _mm_loadu_ps(src + 5);
		prod0 = _mm_add_ps(prod0, prod2);
		val2 = _mm_loadu_ps(src + 6);
		*dp = _mm_add_ps(tgt, prod0);
		val3 = _mm_loadu_ps(src + 7);
	}
	return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                    simd_2tap_v_irrev32                             */
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
	float lambda0 = step->coeffs[0];
	float lambda1 = (step->support_length == 2) ? (step->coeffs[1]) : 0.0F;
	__m128 *sp0 = (__m128 *) src0;
	__m128 *sp1 = (__m128 *) src1;
	__m128 *dp_in = (__m128 *) dst_in;
	__m128 *dp_out = (__m128 *) dst_out;
	__m128 val0 = sp0[0];
	__m128 val1 = sp1[0];
	__m128 vec_lambda0, vec_lambda1;
	if (synthesis)
	{
		vec_lambda0 = _mm_set1_ps(-lambda0); vec_lambda1 = _mm_set1_ps(-lambda1);
	}
	else
	{
		vec_lambda0 = _mm_set1_ps(lambda0); vec_lambda1 = _mm_set1_ps(lambda1);
	}
	samples = (samples + 3) >> 2;
	for (int c = 0; c < samples; c++)
	{
		__m128 tgt = dp_in[c];
		__m128 prod0 = _mm_mul_ps(val0, vec_lambda0);
		__m128 prod1 = _mm_mul_ps(val1, vec_lambda1);
		prod0 = _mm_add_ps(prod0, prod1);
		val0 = sp0[c + 1];
		val1 = sp1[c + 1];
		dp_out[c] = _mm_add_ps(tgt, prod0);
	}
	return true;
#endif // !KDU_NO_SSE
}

/*****************************************************************************/
/* INLINE                    simd_4tap_v_irrev32                             */
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
	assert((step->support_length >= 3) && (step->support_length <= 4));
	if (kdu_mmx_level < 2)
		return false;
	float lambda0 = step->coeffs[0];
	float lambda1 = step->coeffs[1];
	float lambda2 = step->coeffs[2];
	float lambda3 = (step->support_length == 4) ? (step->coeffs[3]) : 0.0F;
	__m128 *sp0 = (__m128 *) src0;
	__m128 *sp1 = (__m128 *) src1;
	__m128 *sp2 = (__m128 *) src2;
	__m128 *sp3 = (__m128 *) src3;
	__m128 *dp_in = (__m128 *) dst_in;
	__m128 *dp_out = (__m128 *) dst_out;
	__m128 val0 = sp0[0];
	__m128 val1 = sp1[0];
	__m128 val2 = sp2[0];
	__m128 val3 = sp3[0];
	__m128 vec_lambda0, vec_lambda1, vec_lambda2, vec_lambda3;
	if (synthesis)
	{
		vec_lambda0 = _mm_set1_ps(-lambda0); vec_lambda1 = _mm_set1_ps(-lambda1);
		vec_lambda2 = _mm_set1_ps(-lambda2); vec_lambda3 = _mm_set1_ps(-lambda3);
	}
	else
	{
		vec_lambda0 = _mm_set1_ps(lambda0); vec_lambda1 = _mm_set1_ps(lambda1);
		vec_lambda2 = _mm_set1_ps(lambda2); vec_lambda3 = _mm_set1_ps(lambda3);
	}
	samples = (samples + 3) >> 2;
	for (int c = 0; c < samples; c++)
	{
		__m128 tgt = dp_in[c];
		__m128 prod0 = _mm_mul_ps(val0, vec_lambda0);
		__m128 prod1 = _mm_mul_ps(val1, vec_lambda1);
		prod0 = _mm_add_ps(prod0, prod1);
		__m128 prod2 = _mm_mul_ps(val2, vec_lambda2);
		__m128 prod3 = _mm_mul_ps(val3, vec_lambda3);
		prod2 = _mm_add_ps(prod2, prod3);
		val0 = sp0[c + 1];
		val1 = sp1[c + 1];
		prod0 = _mm_add_ps(prod0, prod2);
		val2 = sp2[c + 1];
		val3 = sp3[c + 1];
		dp_out[c] = _mm_add_ps(tgt, prod0);
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
	/* Special function to implement the reversible 5/3 transform with
	   32-bit integer operands. */
{
	assert((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		int quad_bytes = ((samples + 3) & ~3) << 2;
		int src_aligned = ((_addr_to_kdu_int32(src) & 0x0F) == 0);
		__m128i vec_offset = _mm_set1_epi32((1 << step->downshift) >> 1);
		register __m128i *dp = (__m128i *) dst; // Always aligned
		register __m128i *dp_lim = (__m128i *)(((kdu_byte *)dp) + quad_bytes);
		register __m128i *sp_a = (__m128i *) src; // Aligned pointer
		register __m128i *sp_u = (__m128i *)(src + 1); // Unaligned pointer
		__m128i downshift = _mm_cvtsi32_si128(step->downshift);
		if (!src_aligned)
		{ // Make sure `src_u' holds the unaligned of the two source addresses
			sp_a = (__m128i *)(src + 1); // Aligned pointer
			sp_u = (__m128i *) src; // Unaligned pointer
		}
		__m128i val_u = _mm_loadu_si128(sp_u); // Preload unaligned dqword
		if (step->icoeffs[0] == 1)
			for (; dp < dp_lim; dp++)
			{
				__m128i val = *(sp_a++);
				__m128i tgt = *dp;
				val = _mm_add_epi32(val, vec_offset);
				val = _mm_add_epi32(val, val_u);
				val_u = _mm_loadu_si128(++sp_u); // Pre-load unaligned dqword
				val = _mm_sra_epi32(val, downshift);
				*dp = _mm_add_epi32(tgt, val);
			}
		else
			for (; dp < dp_lim; dp++)
			{
				__m128i val = *(sp_a++);
				__m128i tgt = *dp;
				val = _mm_sub_epi32(vec_offset, val);
				val = _mm_sub_epi32(val, val_u);
				val_u = _mm_loadu_si128(++sp_u); // Pre-load unaligned dqword
				val = _mm_sra_epi32(val, downshift);
				*dp = _mm_add_epi32(tgt, val);
			}
		return true;
	}
#endif // !KDU_NO_SSE
	return false;
}

/*****************************************************************************/
/* INLINE                     simd_W5X3_h_synth32                            */
/*****************************************************************************/

static inline bool
simd_W5X3_h_synth32(kdu_int32 *src, kdu_int32 *dst, int samples,
	kd_lifting_step *step)
	/* Special function to implement the reversible 5/3 transform with
	   32-bit integer operands. */
{
	assert((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		int quad_bytes = ((samples + 3) & ~3) << 2;
		int src_aligned = ((_addr_to_kdu_int32(src) & 0x0F) == 0);
		__m128i vec_offset = _mm_set1_epi32((1 << step->downshift) >> 1);
		register __m128i *dp = (__m128i *) dst; // Always aligned
		register __m128i *dp_lim = (__m128i *)(((kdu_byte *)dp) + quad_bytes);
		register __m128i *sp_a = (__m128i *) src; // Aligned pointer
		register __m128i *sp_u = (__m128i *)(src + 1); // Unaligned pointer
		__m128i downshift = _mm_cvtsi32_si128(step->downshift);
		if (!src_aligned)
		{ // Make sure `src_u' holds the unaligned of the two source addresses
			sp_a = (__m128i *)(src + 1); // Aligned pointer
			sp_u = (__m128i *) src; // Unaligned pointer
		}
		__m128i val_u = _mm_loadu_si128(sp_u); // Preload unaligned dqword
		if (step->icoeffs[0] == 1)
			for (; dp < dp_lim; dp++)
			{
				__m128i val = *(sp_a++);
				__m128i tgt = *dp;
				val = _mm_add_epi32(val, vec_offset);
				val = _mm_add_epi32(val, val_u);
				val_u = _mm_loadu_si128(++sp_u); // Pre-load unaligned dqword
				val = _mm_sra_epi32(val, downshift);
				*dp = _mm_sub_epi32(tgt, val);
			}
		else
			for (; dp < dp_lim; dp++)
			{
				__m128i val = *(sp_a++);
				__m128i tgt = *dp;
				val = _mm_sub_epi32(vec_offset, val);
				val = _mm_sub_epi32(val, val_u);
				val_u = _mm_loadu_si128(++sp_u); // Pre-load unaligned dqword
				val = _mm_sra_epi32(val, downshift);
				*dp = _mm_sub_epi32(tgt, val);
			}
		return true;
	}
#endif // !KDU_NO_SSE
	return false;
}

/*****************************************************************************/
/* INLINE                  simd_W5X3_v_analysis32                            */
/*****************************************************************************/

static inline bool
simd_W5X3_v_analysis32(kdu_int32 *src1, kdu_int32 *src2,
	kdu_int32 *dst_in, kdu_int32 *dst_out,
	int samples, kd_lifting_step *step)
	/* As above, but for 32-bit integer sample values. */
{
	assert((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i vec_offset = _mm_set1_epi32((1 << step->downshift) >> 1);
		__m128i downshift = _mm_cvtsi32_si128(step->downshift);
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m128i val = vec_offset;  // Start with the offset
				val = _mm_add_epi32(val, *((__m128i *)(src1 + c))); // Add source 1
				val = _mm_add_epi32(val, *((__m128i *)(src2 + c))); // Add source 2
				val = _mm_sra_epi32(val, downshift);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_add_epi32(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m128i val = vec_offset;  // Start with the offset
				val = _mm_sub_epi32(val, *((__m128i *)(src1 + c))); // Subtract src 1
				val = _mm_sub_epi32(val, *((__m128i *)(src2 + c))); // Subtract src 2
				val = _mm_sra_epi32(val, downshift);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_add_epi32(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		else
			assert(0);
		return true;
	}
#endif // !KDU_NO_SSE
	return false;
}

/*****************************************************************************/
/* INLINE                    simd_W5X3_v_synth32                             */
/*****************************************************************************/

static inline bool
simd_W5X3_v_synth32(kdu_int32 *src1, kdu_int32 *src2,
	kdu_int32 *dst_in, kdu_int32 *dst_out,
	int samples, kd_lifting_step *step)
	/* As above, but for 32-bit integer sample values. */
{
	assert((step->support_length == 2) && (step->icoeffs[0] == step->icoeffs[1]));
#ifndef KDU_NO_SSE
	if (kdu_mmx_level >= 2)
	{ // 128-bit implementation, using SSE/SSE2 instructions
		__m128i vec_offset = _mm_set1_epi32((1 << step->downshift) >> 1);
		__m128i downshift = _mm_cvtsi32_si128(step->downshift);
		if (step->icoeffs[0] == 1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m128i val = vec_offset;  // Start with the offset
				val = _mm_add_epi32(val, *((__m128i *)(src1 + c))); // Add source 1
				val = _mm_add_epi32(val, *((__m128i *)(src2 + c))); // Add source 2
				val = _mm_sra_epi32(val, downshift);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_sub_epi32(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		else if (step->icoeffs[0] == -1)
			for (register int c = 0; c < samples; c += 4)
			{
				__m128i val = vec_offset;  // Start with the offset
				val = _mm_sub_epi32(val, *((__m128i *)(src1 + c))); // Subtract src 1
				val = _mm_sub_epi32(val, *((__m128i *)(src2 + c))); // Subtract src 2
				val = _mm_sra_epi32(val, downshift);
				__m128i tgt = *((__m128i *)(dst_in + c));
				tgt = _mm_sub_epi32(tgt, val);
				*((__m128i *)(dst_out + c)) = tgt;
			}
		else
			assert(0);
		return true;
	}
#endif // !KDU_NO_SSE
	return false;
}

/*****************************************************************************/
/* INLINE                        simd_interleave                             */
/*****************************************************************************/

static inline bool
simd_interleave(kdu_int32 *src1, kdu_int32 *src2, kdu_int32 *dst, int pairs)
{
#ifndef KDU_NO_SSE
	if ((kdu_mmx_level >= 2) && (pairs >= 8))
	{ // Implementation based on 128-bit operands, using SSE/SSE2 instructions
		bool odd_start = ((_addr_to_kdu_int32(src1) & 8) != 0);
		if (odd_start)
		{ // Source addresses are 8-byte aligned, but not 16-byte aligned
			src1 += 2;  src2 += 2; dst += 4; pairs -= 2;
		}
		__m128i *sp1 = (__m128i *) src1;
		__m128i *sp2 = (__m128i *) src2;
		__m128i *dp = (__m128i *) dst;
		if (odd_start)
			dp[-1] = _mm_unpackhi_epi32(sp1[-1], sp2[-1]);
		for (; pairs > 2; pairs -= 4, sp1++, sp2++, dp += 2)
		{
			__m128i val1 = *sp1;
			__m128i val2 = *sp2;
			dp[0] = _mm_unpacklo_epi32(val1, val2);
			dp[1] = _mm_unpackhi_epi32(val1, val2);
		}
		if (pairs > 0)
			dp[0] = _mm_unpacklo_epi32(sp1[0], sp2[0]); // Odd tail
		return true;
	}
#endif // !KDU_NO_SSE
	return false;
}

#endif // X86_DWT_LOCAL_H
