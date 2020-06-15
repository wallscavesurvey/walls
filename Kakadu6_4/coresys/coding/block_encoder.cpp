/*****************************************************************************/
// File: block_encoder.cpp [scope = CORESYS/CODING]
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
   Implements the embedded block coding algorithm, including distortion
estimation and R-D covex hull analysis, in addition to the coding
passes themselves.  The low level services offered by the MQ arithmetic coder
appear in "mq_encoder.cpp" and "mq_encoder.h".
******************************************************************************/

#include <math.h>
#include <string.h>
#include <assert.h>
#include "kdu_messaging.h"
#include "kdu_block_coding.h"
#include "block_coding_common.h"
#include "mq_encoder.h"

static kdu_byte *significance_luts[4] =
{ lh_sig_lut, hl_sig_lut, lh_sig_lut, hh_sig_lut };

#define DISTORTION_LSBS 5
#define SIGNIFICANCE_DISTORTIONS (1<<DISTORTION_LSBS)
#define REFINEMENT_DISTORTIONS (1<<(DISTORTION_LSBS+1))

static kdu_int32 significance_distortion_lut[SIGNIFICANCE_DISTORTIONS];
static kdu_int32 significance_distortion_lut_lossless[SIGNIFICANCE_DISTORTIONS];
static kdu_int32 refinement_distortion_lut[REFINEMENT_DISTORTIONS];
static kdu_int32 refinement_distortion_lut_lossless[REFINEMENT_DISTORTIONS];

#define EXTRA_ENCODE_CWORDS 3 // Number of extra context-words between stripes.
#define MAX_POSSIBLE_PASSES (31*3-2)

/* ========================================================================= */
/*                   Local Class and Structure Definitions                   */
/* ========================================================================= */

/*****************************************************************************/
/*                             kd_block_encoder                              */
/*****************************************************************************/

class kd_block_encoder : public kdu_block_encoder_base {
	/* Although we can supply a constructor and a virtual destructor in the
	   future, we have no need for these for the moment. */
protected:
	void encode(kdu_block *block, bool reversible, double msb_wmse,
		kdu_uint16 estimated_slope_threshold);
};

/* ========================================================================= */
/*            Initialization of Distortion Estimation Tables                 */
/* ========================================================================= */

static void initialize_significance_distortion_luts();
static void initialize_refinement_distortion_luts();
static class encoder_local_init {
public: encoder_local_init()
{
	initialize_significance_distortion_luts();
	initialize_refinement_distortion_luts();
}
} _do_it;

/*****************************************************************************/
/* STATIC          initialize_significance_distortion_luts                   */
/*****************************************************************************/

#define KD_DISTORTION_LUT_SCALE ((double)(1<<16))

static void
initialize_significance_distortion_luts()
{
	for (kdu_int32 n = 0; n < SIGNIFICANCE_DISTORTIONS; n++)
	{
		kdu_int32 idx = n | (1 << DISTORTION_LSBS);
		double v_tilde = ((double)idx) / ((double)(1 << DISTORTION_LSBS));
		assert((v_tilde >= 1.0) && (v_tilde < 2.0));
		double sqe_before = v_tilde * v_tilde;
		double sqe_after = (v_tilde - 1.5)*(v_tilde - 1.5);
		significance_distortion_lut[n] = (int)
			floor(0.5 + KD_DISTORTION_LUT_SCALE * (sqe_before - sqe_after));
		significance_distortion_lut_lossless[n] = (int)
			floor(0.5 + KD_DISTORTION_LUT_SCALE * sqe_before);
	}
}

/*****************************************************************************/
/* STATIC            initialize_refinement_distortion_luts                   */
/*****************************************************************************/

static void
initialize_refinement_distortion_luts()
{
	for (kdu_int32 n = 0; n < REFINEMENT_DISTORTIONS; n++)
	{
		double v_tilde = ((double)n) / ((double)(1 << DISTORTION_LSBS));
		assert(v_tilde < 2.0);
		double sqe_before = (v_tilde - 1.0)*(v_tilde - 1.0);
		v_tilde = (n >> DISTORTION_LSBS) ? (v_tilde - 1.0) : v_tilde;
		assert((v_tilde >= 0.0) && (v_tilde < 1.0));
		double sqe_after = (v_tilde - 0.5)*(v_tilde - 0.5);
		refinement_distortion_lut[n] = (int)
			floor(0.5 + KD_DISTORTION_LUT_SCALE * (sqe_before - sqe_after));
		refinement_distortion_lut_lossless[n] = (int)
			floor(0.5 + KD_DISTORTION_LUT_SCALE * sqe_before);
	}
}


/* ========================================================================= */
/*             Binding of MQ and Raw Symbol Coding Services                  */
/* ========================================================================= */

static inline
void reset_states(mqe_state states[])
/* `states' is assumed to be the 18-element context state table.
   See Table 12.1 in the book by Taubman and Marcellin */
{
	for (int n = 0; n < 18; n++)
		states[n].init(0, 0);
	states[KAPPA_SIG_BASE].init(4, 0);
	states[KAPPA_RUN_BASE].init(3, 0);
}


#define USE_FAST_MACROS // Comment this out if you want functions instead.

#ifdef USE_FAST_MACROS
#  define _mq_check_out_(coder)                                     \
     register kdu_int32 A; register kdu_int32 C; register kdu_int32 t; \
     kdu_int32 temp; kdu_byte *store;                                 \
     coder.check_out(A,C,t,temp,store)
#  define _mq_check_in_(coder)                                      \
     coder.check_in(A,C,t,temp,store)
#  define _mq_enc_(coder,symbol,state)                              \
     _mq_encode_(symbol,state,A,C,t,temp,store)
#  define _mq_enc_run_(coder,run)                                   \
     _mq_encode_run_(run,A,C,t,temp,store)

#  define _raw_check_out_(coder)                                    \
     register kdu_int32 t; register kdu_int32 temp; kdu_byte *store;   \
     coder.check_out(t,temp,store)
#  define _raw_check_in_(coder)                                     \
     coder.check_in(t,temp,store)
#  define _raw_enc_(coder,symbol)                                   \
     _raw_encode_(symbol,t,temp,store)
#else // Do not use fast macros
#  define _mq_check_out_(coder)
#  define _mq_check_in_(coder)
#  define _mq_enc_(coder,symbol,state) coder.mq_encode(symbol,state)
#  define _mq_enc_run_(coder,run) coder.mq_encode_run(run)

#  define _raw_check_out_(coder)
#  define _raw_check_in_(coder)
#  define _raw_enc_(coder,symbol) coder.raw_encode(symbol)
#endif // USE_FAST_MACROS

/* The coding pass functions defined below all return a 32-bit integer,
   which represents the normalized reduction in MSE associated with the
   coded symbols.  Specifically, the MSE whose reduction is returned is
   equal to 2^16 * sum_i (x_i/2^p - x_i_hat/2^p)^2 where x_i denotes the
   integer sample values in the `samples' array and x_i_hat denotes the
   quantized representation available from the current coding pass and all
   previous coding passes, assuming a mid-point reconstruction rule.
	  The mid-point reconstruction rule satisfies x_i_hat = (q_i+1/2)*Delta
   where q_i denotes the quantization indices and Delta is the quantization
   step size.  This rule is modified only if the `lossless_pass' argument is
   true, which is permitted only when symbols coded in the coding pass
   result in a lossless representation of the corresponding subband samples.
   Of course, this can only happen in the last bit-plane when the reversible
   compression path is being used.  In this case, the function uses the fact
   that all coded symbols have 0 distortion.
	  It should be noted that the MSE reduction can be negative, meaning
   that the coding of symbols actually increases distortion. */


   /* ========================================================================= */
   /*                           Coding pass functions                           */
   /* ========================================================================= */

   /*****************************************************************************/
   /* STATIC                   encode_sig_prop_pass_raw                         */
   /*****************************************************************************/

static kdu_int32
encode_sig_prop_pass_raw(mq_encoder &coder, int p, bool causal,
	kdu_int32 *samples, kdu_int32 *contexts,
	int width, int num_stripes, int context_row_gap,
	bool lossless_pass)
{
	/* Ideally, register storage is available for 9 32-bit integers. Two
	   are declared inside the "_raw_check_out_" macro.  The order of priority
	   for these registers corresponds roughly to the order in which their
	   declarations appear below.  Unfortunately, none of these register
	   requests are likely to be honored by the register-starved X86 family
	   of processors, but the register declarations may prove useful to
	   compilers for other architectures or for hand optimizations of
	   assembly code. */
	register kdu_int32 *cp = contexts;
	register int c;
	register kdu_int32 cword;
	_raw_check_out_(coder); // Declares t and temp as registers.
	register kdu_int32 sym;
	register kdu_int32 val;
	register kdu_int32 *sp = samples;
	register kdu_int32 shift = 31 - p; assert(shift > 0);
	int r, width_by2 = width + width, width_by3 = width_by2 + width;
	kdu_int32 distortion_change = 0;
	kdu_int32 *distortion_lut = significance_distortion_lut;
	if (lossless_pass)
		distortion_lut = significance_distortion_lut_lossless;

	assert((context_row_gap - width) == EXTRA_ENCODE_CWORDS);
	for (r = num_stripes; r > 0; r--, cp += EXTRA_ENCODE_CWORDS, sp += width_by3)
		for (c = width; c > 0; c--, sp++, cp++)
		{
			if (*cp == 0)
				continue;
			cword = *cp;
			if ((cword & (NBRHD_MASK << 0)) && !(cword & (SIG_PROP_MEMBER_MASK << 0)))
			{ // Process first row of stripe column (row 0)
				val = sp[0] << shift; // Move bit p to sign bit.
				sym = (kdu_int32)(((kdu_uint32)val) >> 31); // Move bit into LSB
				_raw_enc_(coder, sym);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
				{
					cword |= (PI_BIT << 0); goto row_1;
				}
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = sp[0];
				sym = (kdu_int32)(((kdu_uint32)sym) >> 31); // Move sign into LSB
				_raw_enc_(coder, sym);
				// Broadcast neighbourhood context changes
				if (!causal)
				{
					cp[-context_row_gap - 1] |= (SIGMA_BR_BIT << 9);
					cp[-context_row_gap] |= (SIGMA_BC_BIT << 9) | (sym << NEXT_CHI_POS);
					cp[-context_row_gap + 1] |= (SIGMA_BL_BIT << 9);
				}
				cp[-1] |= (SIGMA_CR_BIT << 0);
				cp[1] |= (SIGMA_CL_BIT << 0);
				cword |= (SIGMA_CC_BIT << 0) | (PI_BIT << 0) | (sym << CHI_POS);
			}
		row_1:
			if ((cword & (NBRHD_MASK << 3)) && !(cword & (SIG_PROP_MEMBER_MASK << 3)))
			{ // Process second row of stripe column (row 1)
				val = sp[width] << shift; // Move bit p to sign bit.
				sym = (kdu_int32)(((kdu_uint32)val) >> 31); // Move bit into LSB
				_raw_enc_(coder, sym);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
				{
					cword |= (PI_BIT << 3); goto row_2;
				}
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = sp[width];
				sym = (kdu_int32)(((kdu_uint32)sym) >> 31); // Move sign into LSB
				_raw_enc_(coder, sym);
				// Broadcast neighbourhood context changes
				cp[-1] |= (SIGMA_CR_BIT << 3);
				cp[1] |= (SIGMA_CL_BIT << 3);
				cword |= (SIGMA_CC_BIT << 3) | (PI_BIT << 3) | (sym << (CHI_POS + 3));
			}
		row_2:
			if ((cword & (NBRHD_MASK << 6)) && !(cword & (SIG_PROP_MEMBER_MASK << 6)))
			{ // Process third row of stripe column (row 2)
				val = sp[width_by2] << shift; // Move bit p to sign bit.
				sym = (kdu_int32)(((kdu_uint32)val) >> 31); // Move bit into LSB
				_raw_enc_(coder, sym);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
				{
					cword |= (PI_BIT << 6); goto row_3;
				}
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = sp[width_by2];
				sym = (kdu_int32)(((kdu_uint32)sym) >> 31); // Move sign into LSB
				_raw_enc_(coder, sym);
				// Broadcast neighbourhood context changes
				cp[-1] |= (SIGMA_CR_BIT << 6);
				cp[1] |= (SIGMA_CL_BIT << 6);
				cword |= (SIGMA_CC_BIT << 6) | (PI_BIT << 6) | (sym << (CHI_POS + 6));
			}
		row_3:
			if ((cword & (NBRHD_MASK << 9)) && !(cword & (SIG_PROP_MEMBER_MASK << 9)))
			{ // Process fourth row of stripe column (row 3)
				val = sp[width_by3] << shift; // Move bit p to sign bit.
				sym = (kdu_int32)(((kdu_uint32)val) >> 31); // Move bit into LSB
				_raw_enc_(coder, sym);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
				{
					cword |= (PI_BIT << 9); goto done;
				}
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = sp[width_by3];
				sym = (kdu_int32)(((kdu_uint32)sym) >> 31); // Move sign into LSB
				_raw_enc_(coder, sym);
				// Broadcast neighbourhood context changes
				cp[context_row_gap - 1] |= SIGMA_TR_BIT;
				cp[context_row_gap] |= SIGMA_TC_BIT | (sym << PREV_CHI_POS);
				cp[context_row_gap + 1] |= SIGMA_TL_BIT;
				cp[-1] |= (SIGMA_CR_BIT << 9);
				cp[1] |= (SIGMA_CL_BIT << 9);
				cword |= (SIGMA_CC_BIT << 9) | (PI_BIT << 9) | (sym << (CHI_POS + 9));
			}
		done:
			*cp = cword;
		}
	_raw_check_in_(coder);
	return distortion_change;
}

/*****************************************************************************/
/* STATIC                    encode_sig_prop_pass                            */
/*****************************************************************************/

static kdu_int32
encode_sig_prop_pass(mq_encoder &coder, mqe_state states[],
	int p, bool causal, int orientation,
	kdu_int32 *samples, kdu_int32 *contexts,
	int width, int num_stripes, int context_row_gap,
	bool lossless_pass)
{
	/* Ideally, register storage is available for 12 32-bit integers. Three
	   are declared inside the "_mq_check_out_" macro.  The order of priority
	   for these registers corresponds roughly to the order in which their
	   declarations appear below.  Unfortunately, none of these register
	   requests are likely to be honored by the register-starved X86 family
	   of processors, but the register declarations may prove useful to
	   compilers for other architectures or for hand optimizations of
	   assembly code. */
	register kdu_int32 *cp = contexts;
	register int c;
	register kdu_int32 cword;
	_mq_check_out_(coder); // Declares A, C, and t as registers.
	register kdu_int32 sym;
	register kdu_int32 val;
	register kdu_int32 *sp = samples;
	register kdu_int32 shift = 31 - p; assert(shift > 0);
	register  kdu_byte *sig_lut = significance_luts[orientation];
	register mqe_state *state_ref;
	int r, width_by2 = width + width, width_by3 = width_by2 + width;
	kdu_int32 distortion_change = 0;
	kdu_int32 *distortion_lut = significance_distortion_lut;
	if (lossless_pass)
		distortion_lut = significance_distortion_lut_lossless;

	assert((context_row_gap - width) == EXTRA_ENCODE_CWORDS);
	for (r = num_stripes; r > 0; r--, cp += EXTRA_ENCODE_CWORDS, sp += width_by3)
		for (c = width; c > 0; c--, sp++, cp++)
		{
			if (*cp == 0)
			{ // Invoke speedup trick to skip over runs of all-0 neighbourhoods
				for (cp += 3; *cp == 0; cp += 3, c -= 3, sp += 3);
				cp -= 3;
				continue;
			}
			cword = *cp;
			if ((cword & (NBRHD_MASK << 0)) && !(cword & (SIG_PROP_MEMBER_MASK << 0)))
			{ // Process first row of stripe column (row 0)
				state_ref = states + KAPPA_SIG_BASE + sig_lut[cword & NBRHD_MASK];
				val = sp[0] << shift; // Move bit p to sign bit.
				sym = val & KDU_INT32_MIN;
				_mq_enc_(coder, sym, *state_ref);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
				{
					cword |= (PI_BIT << 0); goto row_1;
				}
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = cword & ((CHI_BIT >> 3) | (SIGMA_CC_BIT >> 3) |
					(CHI_BIT << 3) | (SIGMA_CC_BIT << 3));
				sym >>= 1; // Shift down so that top sigma bit has address 0
				sym |= (cp[-1] & ((CHI_BIT << 0) | (SIGMA_CC_BIT << 0))) >> (1 + 1);
				sym |= (cp[1] & ((CHI_BIT << 0) | (SIGMA_CC_BIT << 0))) >> (1 - 1);
				sym |= (sym >> (CHI_POS - 1 - SIGMA_CC_POS)); // Interleave chi & sigma
				val = sign_lut[sym & 0x000000FF];
				state_ref = states + KAPPA_SIGN_BASE + (val >> 1);
				sym = val << 31; // Get sign flipping to `sym'
				val = sp[0] & KDU_INT32_MIN; // Get the sign bit
				sym ^= val; // Moves flipped sign bit into `sym'
				_mq_enc_(coder, sym, *state_ref);
				// Broadcast neighbourhood context changes; sign bit is in `val'
				cp[-1] |= (SIGMA_CR_BIT << 0);
				cp[1] |= (SIGMA_CL_BIT << 0);
				if (val < 0)
				{
					cword |= (SIGMA_CC_BIT << 0) | (PI_BIT << 0) | (CHI_BIT << 0);
					if (!causal)
					{
						cp[-context_row_gap - 1] |= (SIGMA_BR_BIT << 9);
						cp[-context_row_gap] |= (SIGMA_BC_BIT << 9) | NEXT_CHI_BIT;
						cp[-context_row_gap + 1] |= (SIGMA_BL_BIT << 9);
					}
				}
				else
				{
					cword |= (SIGMA_CC_BIT << 0) | (PI_BIT << 0);
					if (!causal)
					{
						cp[-context_row_gap - 1] |= (SIGMA_BR_BIT << 9);
						cp[-context_row_gap] |= (SIGMA_BC_BIT << 9);
						cp[-context_row_gap + 1] |= (SIGMA_BL_BIT << 9);
					}
				}
			}
		row_1:
			if ((cword & (NBRHD_MASK << 3)) && !(cword & (SIG_PROP_MEMBER_MASK << 3)))
			{ // Process second row of stripe column (row 1)
				state_ref = states + KAPPA_SIG_BASE + sig_lut[(cword >> 3) & NBRHD_MASK];
				val = sp[width] << shift; // Move bit p to sign bit.
				sym = val & KDU_INT32_MIN;
				_mq_enc_(coder, sym, *state_ref);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
				{
					cword |= (PI_BIT << 3); goto row_2;
				}
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = cword & ((CHI_BIT << 0) | (SIGMA_CC_BIT << 0) |
					(CHI_BIT << 6) | (SIGMA_CC_BIT << 6));
				sym >>= 4; // Shift down so that top sigma bit has address 0
				sym |= (cp[-1] & ((CHI_BIT << 3) | (SIGMA_CC_BIT << 3))) >> (4 + 1);
				sym |= (cp[1] & ((CHI_BIT << 3) | (SIGMA_CC_BIT << 3))) >> (4 - 1);
				sym |= (sym >> (CHI_POS - 1 - SIGMA_CC_POS)); // Interleave chi & sigma
				val = sign_lut[sym & 0x000000FF];
				state_ref = states + KAPPA_SIGN_BASE + (val >> 1);
				sym = val << 31; // Get sign flipping to `sym'
				val = sp[width] & KDU_INT32_MIN; // Get the sign bit
				sym ^= val; // Moves flipped sign bit into `sym'
				_mq_enc_(coder, sym, *state_ref);
				// Broadcast neighbourhood context changes; sign bit is in `val'
				cp[-1] |= (SIGMA_CR_BIT << 3);
				cp[1] |= (SIGMA_CL_BIT << 3);
				cword |= (SIGMA_CC_BIT << 3) | (PI_BIT << 3);
				val = (kdu_int32)(((kdu_uint32)val) >> (31 - (CHI_POS + 3))); // SRL
				cword |= val;
			}
		row_2:
			if ((cword & (NBRHD_MASK << 6)) && !(cword & (SIG_PROP_MEMBER_MASK << 6)))
			{ // Process third row of stripe column (row 2)
				state_ref = states + KAPPA_SIG_BASE + sig_lut[(cword >> 6) & NBRHD_MASK];
				val = sp[width_by2] << shift; // Move bit p to sign bit.
				sym = val & KDU_INT32_MIN;
				_mq_enc_(coder, sym, *state_ref);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
				{
					cword |= (PI_BIT << 6); goto row_3;
				}
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = cword & ((CHI_BIT << 3) | (SIGMA_CC_BIT << 3) |
					(CHI_BIT << 9) | (SIGMA_CC_BIT << 9));
				sym >>= 7; // Shift down so that top sigma bit has address 0
				sym |= (cp[-1] & ((CHI_BIT << 6) | (SIGMA_CC_BIT << 6))) >> (7 + 1);
				sym |= (cp[1] & ((CHI_BIT << 6) | (SIGMA_CC_BIT << 6))) >> (7 - 1);
				sym |= (sym >> (CHI_POS - 1 - SIGMA_CC_POS)); // Interleave chi & sigma
				val = sign_lut[sym & 0x000000FF];
				state_ref = states + KAPPA_SIGN_BASE + (val >> 1);
				sym = val << 31; // Get sign flipping to `sym'
				val = sp[width_by2] & KDU_INT32_MIN; // Get the sign bit
				sym ^= val; // Moves flipped sign bit into `sym'
				_mq_enc_(coder, sym, *state_ref);
				// Broadcast neighbourhood context changes; sign bit is in `val'
				cp[-1] |= (SIGMA_CR_BIT << 6);
				cp[1] |= (SIGMA_CL_BIT << 6);
				cword |= (SIGMA_CC_BIT << 6) | (PI_BIT << 6);
				val = (kdu_int32)(((kdu_uint32)val) >> (31 - (CHI_POS + 6))); // SRL
				cword |= val;
			}
		row_3:
			if ((cword & (NBRHD_MASK << 9)) && !(cword & (SIG_PROP_MEMBER_MASK << 9)))
			{ // Process fourth row of stripe column (row 3)
				state_ref = states + KAPPA_SIG_BASE + sig_lut[(cword >> 9) & NBRHD_MASK];
				val = sp[width_by3] << shift; // Move bit p to sign bit.
				sym = val & KDU_INT32_MIN;
				_mq_enc_(coder, sym, *state_ref);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
				{
					cword |= (PI_BIT << 9); goto done;
				}
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = cword & ((CHI_BIT << 6) | (SIGMA_CC_BIT << 6) |
					0 | (SIGMA_CC_BIT << 12));
				sym >>= 10; // Shift down so that top sigma bit has address 0
				if (cword < 0) // Use the fact that NEXT_CHI_BIT = 31
					sym |= CHI_BIT << (12 - 10);
				sym |= (cp[-1] & ((CHI_BIT << 9) | (SIGMA_CC_BIT << 9))) >> (10 + 1);
				sym |= (cp[1] & ((CHI_BIT << 9) | (SIGMA_CC_BIT << 9))) >> (10 - 1);
				sym |= (sym >> (CHI_POS - 1 - SIGMA_CC_POS)); // Interleave chi & sigma
				val = sign_lut[sym & 0x000000FF];
				state_ref = states + KAPPA_SIGN_BASE + (val >> 1);
				sym = val << 31; // Get sign flipping to `sym'
				val = sp[width_by3] & KDU_INT32_MIN; // Get the sign bit
				sym ^= val; // Moves flipped sign bit into `sym'
				_mq_enc_(coder, sym, *state_ref);
				// Broadcast neighbourhood context changes; sign bit is in `val'
				cp[context_row_gap - 1] |= SIGMA_TR_BIT;
				cp[context_row_gap + 1] |= SIGMA_TL_BIT;
				cp[-1] |= (SIGMA_CR_BIT << 9);
				cp[1] |= (SIGMA_CL_BIT << 9);
				if (val < 0)
				{
					cp[context_row_gap] |= SIGMA_TC_BIT | PREV_CHI_BIT;
					cword |= (SIGMA_CC_BIT << 9) | (PI_BIT << 9) | (CHI_BIT << 9);
				}
				else
				{
					cp[context_row_gap] |= SIGMA_TC_BIT;
					cword |= (SIGMA_CC_BIT << 9) | (PI_BIT << 9);
				}
			}
		done:
			*cp = cword;
		}
	_mq_check_in_(coder);
	return distortion_change;
}

/*****************************************************************************/
/* STATIC                    encode_mag_ref_pass_raw                         */
/*****************************************************************************/

static kdu_int32
encode_mag_ref_pass_raw(mq_encoder &coder, int p, bool causal,
	kdu_int32 *samples, kdu_int32 *contexts,
	int width, int num_stripes, int context_row_gap,
	bool lossless_pass)
{
	/* Ideally, register storage is available for 9 32-bit integers.
	   Two 32-bit integers are declared inside the "_raw_check_out_" macro.
	   The order of priority for these registers corresponds roughly to the
	   order in which their declarations appear below.  Unfortunately, none
	   of these register requests are likely to be honored by the
	   register-starved X86 family of processors, but the register
	   declarations may prove useful to compilers for other architectures or
	   for hand optimizations of assembly code. */
	register kdu_int32 *cp = contexts;
	register int c;
	register kdu_int32 cword;
	_raw_check_out_(coder); // Declares t and temp as registers.
	register kdu_int32 *sp = samples;
	register kdu_int32 sym;
	register kdu_int32 val;
	register kdu_int32 shift = 31 - p; // Shift to get new mag bit to sign position
	int r, width_by2 = width + width, width_by3 = width_by2 + width;
	kdu_int32 distortion_change = 0;
	kdu_int32 *distortion_lut = refinement_distortion_lut;
	if (lossless_pass)
		distortion_lut = refinement_distortion_lut_lossless;

	assert((context_row_gap - width) == EXTRA_ENCODE_CWORDS);
	for (r = num_stripes; r > 0; r--, cp += EXTRA_ENCODE_CWORDS, sp += width_by3)
		for (c = width; c > 0; c--, sp++, cp++)
		{
			if ((*cp & ((MU_BIT << 0) | (MU_BIT << 3) | (MU_BIT << 6) | (MU_BIT << 9))) == 0)
			{ // Invoke speedup trick to skip over runs of all-0 neighbourhoods
				for (cp += 2; *cp == 0; cp += 2, c -= 2, sp += 2);
				cp -= 2;
				continue;
			}
			cword = *cp;
			if (cword & (MU_BIT << 0))
			{ // Process first row of stripe column
				val = sp[0];
				val <<= shift; // Get new magnitude bit to sign position.
				sym = (kdu_int32)(((kdu_uint32)val) >> 31);
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (REFINEMENT_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode magnitude bit
				_raw_enc_(coder, sym);
			}
			if (cword & (MU_BIT << 3))
			{ // Process second row of stripe column
				val = sp[width];
				val <<= shift; // Get new magnitude bit to sign position.
				sym = (kdu_int32)(((kdu_uint32)val) >> 31);
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (REFINEMENT_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode magnitude bit
				_raw_enc_(coder, sym);
			}
			if (cword & (MU_BIT << 6))
			{ // Process third row of stripe column
				val = sp[width_by2];
				val <<= shift; // Get new magnitude bit to sign position.
				sym = (kdu_int32)(((kdu_uint32)val) >> 31);
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (REFINEMENT_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode magnitude bit
				_raw_enc_(coder, sym);
			}
			if (cword & (MU_BIT << 9))
			{ // Process fourth row of stripe column
				val = sp[width_by3];
				val <<= shift; // Get new magnitude bit to sign position.
				sym = (kdu_int32)(((kdu_uint32)val) >> 31);
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (REFINEMENT_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode magnitude bit
				_raw_enc_(coder, sym);
			}
		}
	_raw_check_in_(coder);
	return distortion_change;
}

/*****************************************************************************/
/* STATIC                     encode_mag_ref_pass                            */
/*****************************************************************************/

static kdu_int32
encode_mag_ref_pass(mq_encoder &coder, mqe_state states[],
	int p, bool causal, kdu_int32 *samples,
	kdu_int32 *contexts, int width, int num_stripes,
	int context_row_gap, bool lossless_pass)
{
	/* Ideally, register storage is available for 12 32-bit integers.
	   Three 32-bit integers are declared inside the "_mq_check_out_" macro.
	   The order of priority for these registers corresponds roughly to the
	   order in which their declarations appear below.  Unfortunately, none
	   of these register requests are likely to be honored by the
	   register-starved X86 family of processors, but the register
	   declarations may prove useful to compilers for other architectures or
	   for hand optimizations of assembly code. */
	register kdu_int32 *cp = contexts;
	register int c;
	register kdu_int32 cword;
	_mq_check_out_(coder); // Declares A, C and t as registers.
	register kdu_int32 *sp = samples;
	register mqe_state *state_ref;
	register kdu_int32 sym;
	register kdu_int32 val;
	register kdu_int32 shift = 31 - p; // Shift to get new mag bit to sign position
	register kdu_int32 refined_mask = (((kdu_int32)(-1)) << (p + 2)) & KDU_INT32_MAX;
	int r, width_by2 = width + width, width_by3 = width_by2 + width;
	kdu_int32 distortion_change = 0;
	kdu_int32 *distortion_lut = refinement_distortion_lut;
	if (lossless_pass)
		distortion_lut = refinement_distortion_lut_lossless;

	states += KAPPA_MAG_BASE;
	assert((context_row_gap - width) == EXTRA_ENCODE_CWORDS);
	for (r = num_stripes; r > 0; r--, cp += EXTRA_ENCODE_CWORDS, sp += width_by3)
		for (c = width; c > 0; c--, sp++, cp++)
		{
			if ((*cp & ((MU_BIT << 0) | (MU_BIT << 3) | (MU_BIT << 6) | (MU_BIT << 9))) == 0)
			{ // Invoke speedup trick to skip over runs of all-0 neighbourhoods
				for (cp += 2; *cp == 0; cp += 2, c -= 2, sp += 2);
				cp -= 2;
				continue;
			}
			cword = *cp;
			if (cword & (MU_BIT << 0))
			{ // Process first row of stripe column
				val = sp[0];
				// Get coding context
				state_ref = states;
				if (!(val & refined_mask))
				{ // This is the first magnitude refinement step
					if (cword & (NBRHD_MASK << 0))
						state_ref++;
				}
				else
					state_ref += 2;
				val <<= shift; // Get new magnitude bit to sign position.
				sym = val & KDU_INT32_MIN;
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (REFINEMENT_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode magnitude bit
				_mq_enc_(coder, sym, *state_ref);
			}
			if (cword & (MU_BIT << 3))
			{ // Process second row of stripe column
				val = sp[width];
				// Get coding context
				state_ref = states;
				if (!(val & refined_mask))
				{ // This is the first magnitude refinement step
					if (cword & (NBRHD_MASK << 3))
						state_ref++;
				}
				else
					state_ref += 2;
				val <<= shift; // Get new magnitude bit to sign position.
				sym = val & KDU_INT32_MIN;
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (REFINEMENT_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode magnitude bit
				_mq_enc_(coder, sym, *state_ref);
			}
			if (cword & (MU_BIT << 6))
			{ // Process third row of stripe column
				val = sp[width_by2];
				// Get coding context
				state_ref = states;
				if (!(val & refined_mask))
				{ // This is the first magnitude refinement step
					if (cword & (NBRHD_MASK << 6))
						state_ref++;
				}
				else
					state_ref += 2;
				val <<= shift; // Get new magnitude bit to sign position.
				sym = val & KDU_INT32_MIN;
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (REFINEMENT_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode magnitude bit
				_mq_enc_(coder, sym, *state_ref);
			}
			if (cword & (MU_BIT << 9))
			{ // Process fourth row of stripe column
				val = sp[width_by3];
				// Get coding context
				state_ref = states;
				if (!(val & refined_mask))
				{ // This is the first magnitude refinement step
					if (cword & (NBRHD_MASK << 9))
						state_ref++;
				}
				else
					state_ref += 2;
				val <<= shift; // Get new magnitude bit to sign position.
				sym = val & KDU_INT32_MIN;
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (REFINEMENT_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode magnitude bit
				_mq_enc_(coder, sym, *state_ref);
			}
		}
	_mq_check_in_(coder);
	return distortion_change;
}

/*****************************************************************************/
/* STATIC                     encode_cleanup_pass                            */
/*****************************************************************************/

static kdu_int32
encode_cleanup_pass(mq_encoder &coder, mqe_state states[],
	int p, bool causal, int orientation,
	kdu_int32 *samples, kdu_int32 *contexts,
	int width, int num_stripes, int context_row_gap,
	bool lossless_pass)
{
	/* Ideally, register storage is available for 12 32-bit integers. Three
	   are declared inside the "_mq_check_out_" macro.  The order of priority
	   for these registers corresponds roughly to the order in which their
	   declarations appear below.  Unfortunately, none of these register
	   requests are likely to be honored by the register-starved X86 family
	   of processors, but the register declarations may prove useful to
	   compilers for other architectures or for hand optimizations of
	   assembly code. */
	register kdu_int32 *cp = contexts;
	register int c;
	register kdu_int32 cword;
	_mq_check_out_(coder); // Declares A, C, and t as registers.
	register kdu_int32 sym;
	register kdu_int32 val;
	register kdu_int32 *sp = samples;
	register kdu_int32 shift = 31 - p; assert(shift > 0);
	register  kdu_byte *sig_lut = significance_luts[orientation];
	register mqe_state *state_ref;
	int r, width_by2 = width + width, width_by3 = width_by2 + width;
	kdu_int32 distortion_change = 0;
	kdu_int32 *distortion_lut = significance_distortion_lut;
	if (lossless_pass)
		distortion_lut = significance_distortion_lut_lossless;

	assert((context_row_gap - width) == EXTRA_ENCODE_CWORDS);
	for (r = num_stripes; r > 0; r--, cp += EXTRA_ENCODE_CWORDS, sp += width_by3)
		for (c = width; c > 0; c--, sp++, cp++)
		{
			if (*cp == 0)
			{ // Enter the run mode
				sym = 0; val = -1;
				if ((sp[0] << shift) < 0)
				{
					val = 0; sym = KDU_INT32_MIN;
				}
				else if ((sp[width] << shift) < 0)
				{
					val = 1; sym = KDU_INT32_MIN;
				}
				else if ((sp[width_by2] << shift) < 0)
				{
					val = 2; sym = KDU_INT32_MIN;
				}
				else if ((sp[width_by3] << shift) < 0)
				{
					val = 3; sym = KDU_INT32_MIN;
				}
				state_ref = states + KAPPA_RUN_BASE;
				_mq_enc_(coder, sym, *state_ref);
				if (val < 0)
					continue;
				_mq_enc_run_(coder, val);
				cword = *cp;
				switch (val) {
				case 0: val = sp[0] << shift; goto row_0_significant;
				case 1: val = sp[width] << shift; goto row_1_significant;
				case 2: val = sp[width_by2] << shift; goto row_2_significant;
				case 3: val = sp[width_by3] << shift; goto row_3_significant;
				}
			}
			cword = *cp;
			if (!(cword & (CLEANUP_MEMBER_MASK << 0)))
			{ // Process first row of stripe column (row 0)
				state_ref = states + KAPPA_SIG_BASE + sig_lut[cword & NBRHD_MASK];
				val = sp[0] << shift;
				sym = val & KDU_INT32_MIN;
				_mq_enc_(coder, sym, *state_ref);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
					goto row_1;
			row_0_significant:
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = cword & ((CHI_BIT >> 3) | (SIGMA_CC_BIT >> 3) |
					(CHI_BIT << 3) | (SIGMA_CC_BIT << 3));
				sym >>= 1; // Shift down so that top sigma bit has address 0
				sym |= (cp[-1] & ((CHI_BIT << 0) | (SIGMA_CC_BIT << 0))) >> (1 + 1);
				sym |= (cp[1] & ((CHI_BIT << 0) | (SIGMA_CC_BIT << 0))) >> (1 - 1);
				sym |= (sym >> (CHI_POS - 1 - SIGMA_CC_POS)); // Interleave chi & sigma
				val = sign_lut[sym & 0x000000FF];
				state_ref = states + KAPPA_SIGN_BASE + (val >> 1);
				sym = val << 31; // Get sign flipping to `sym'
				val = sp[0] & KDU_INT32_MIN; // Get the sign bit
				sym ^= val; // Moves flipped sign bit into `sym'
				_mq_enc_(coder, sym, *state_ref);
				// Broadcast neighbourhood context changes; sign bit is in `val'
				cp[-1] |= (SIGMA_CR_BIT << 0);
				cp[1] |= (SIGMA_CL_BIT << 0);
				if (val < 0)
				{
					cword |= (SIGMA_CC_BIT << 0) | (CHI_BIT << 0);
					if (!causal)
					{
						cp[-context_row_gap - 1] |= (SIGMA_BR_BIT << 9);
						cp[-context_row_gap] |= (SIGMA_BC_BIT << 9) | NEXT_CHI_BIT;
						cp[-context_row_gap + 1] |= (SIGMA_BL_BIT << 9);
					}
				}
				else
				{
					cword |= (SIGMA_CC_BIT << 0);
					if (!causal)
					{
						cp[-context_row_gap - 1] |= (SIGMA_BR_BIT << 9);
						cp[-context_row_gap] |= (SIGMA_BC_BIT << 9);
						cp[-context_row_gap + 1] |= (SIGMA_BL_BIT << 9);
					}
				}
			}
		row_1:
			if (!(cword & (CLEANUP_MEMBER_MASK << 3)))
			{ // Process second row of stripe column (row 1)
				state_ref = states + KAPPA_SIG_BASE + sig_lut[(cword >> 3) & NBRHD_MASK];
				val = sp[width] << shift;
				sym = val & KDU_INT32_MIN;
				_mq_enc_(coder, sym, *state_ref);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
					goto row_2;
			row_1_significant:
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = cword & ((CHI_BIT << 0) | (SIGMA_CC_BIT << 0) |
					(CHI_BIT << 6) | (SIGMA_CC_BIT << 6));
				sym >>= 4; // Shift down so that top sigma bit has address 0
				sym |= (cp[-1] & ((CHI_BIT << 3) | (SIGMA_CC_BIT << 3))) >> (4 + 1);
				sym |= (cp[1] & ((CHI_BIT << 3) | (SIGMA_CC_BIT << 3))) >> (4 - 1);
				sym |= (sym >> (CHI_POS - 1 - SIGMA_CC_POS)); // Interleave chi & sigma
				val = sign_lut[sym & 0x000000FF];
				state_ref = states + KAPPA_SIGN_BASE + (val >> 1);
				sym = val << 31; // Get sign flipping to `sym'
				val = sp[width] & KDU_INT32_MIN; // Get the sign bit
				sym ^= val; // Moves flipped sign bit into `sym'
				_mq_enc_(coder, sym, *state_ref);
				// Broadcast neighbourhood context changes; sign bit is in `val'
				cp[-1] |= (SIGMA_CR_BIT << 3);
				cp[1] |= (SIGMA_CL_BIT << 3);
				cword |= (SIGMA_CC_BIT << 3);
				val = (kdu_int32)(((kdu_uint32)val) >> (31 - (CHI_POS + 3))); // SRL
				cword |= val;
			}
		row_2:
			if (!(cword & (CLEANUP_MEMBER_MASK << 6)))
			{ // Process third row of stripe column (row 2)
				state_ref = states + KAPPA_SIG_BASE + sig_lut[(cword >> 6) & NBRHD_MASK];
				val = sp[width_by2] << shift;
				sym = val & KDU_INT32_MIN;
				_mq_enc_(coder, sym, *state_ref);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
					goto row_3;
			row_2_significant:
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Encode sign bit
				sym = cword & ((CHI_BIT << 3) | (SIGMA_CC_BIT << 3) |
					(CHI_BIT << 9) | (SIGMA_CC_BIT << 9));
				sym >>= 7; // Shift down so that top sigma bit has address 0
				sym |= (cp[-1] & ((CHI_BIT << 6) | (SIGMA_CC_BIT << 6))) >> (7 + 1);
				sym |= (cp[1] & ((CHI_BIT << 6) | (SIGMA_CC_BIT << 6))) >> (7 - 1);
				sym |= (sym >> (CHI_POS - 1 - SIGMA_CC_POS)); // Interleave chi & sigma
				val = sign_lut[sym & 0x000000FF];
				state_ref = states + KAPPA_SIGN_BASE + (val >> 1);
				sym = val << 31; // Get sign flipping to `sym'
				val = sp[width_by2] & KDU_INT32_MIN; // Get the sign bit
				sym ^= val; // Moves flipped sign bit into `sym'
				_mq_enc_(coder, sym, *state_ref);
				// Broadcast neighbourhood context changes; sign bit is in `val'
				cp[-1] |= (SIGMA_CR_BIT << 6);
				cp[1] |= (SIGMA_CL_BIT << 6);
				cword |= (SIGMA_CC_BIT << 6);
				val = (kdu_int32)(((kdu_uint32)val) >> (31 - (CHI_POS + 6))); // SRL
				cword |= val;
			}
		row_3:
			if (!(cword & (CLEANUP_MEMBER_MASK << 9)))
			{ // Process fourth row of stripe column (row 3)
				state_ref = states + KAPPA_SIG_BASE + sig_lut[(cword >> 9) & NBRHD_MASK];
				val = sp[width_by3] << shift;
				sym = val & KDU_INT32_MIN;
				_mq_enc_(coder, sym, *state_ref);
				if (val >= 0) // New magnitude bit was 0, so still insignificant
					goto done;
			row_3_significant:
				// Compute distortion change
				val = (val >> (31 - DISTORTION_LSBS)) & (SIGNIFICANCE_DISTORTIONS - 1);
				distortion_change += distortion_lut[val];
				// Decode sign bit
				sym = cword & ((CHI_BIT << 6) | (SIGMA_CC_BIT << 6) |
					0 | (SIGMA_CC_BIT << 12));
				sym >>= 10; // Shift down so that top sigma bit has address 0
				if (cword < 0) // Use the fact that NEXT_CHI_BIT = 31
					sym |= CHI_BIT << (12 - 10);
				sym |= (cp[-1] & ((CHI_BIT << 9) | (SIGMA_CC_BIT << 9))) >> (10 + 1);
				sym |= (cp[1] & ((CHI_BIT << 9) | (SIGMA_CC_BIT << 9))) >> (10 - 1);
				sym |= (sym >> (CHI_POS - 1 - SIGMA_CC_POS)); // Interleave chi & sigma
				val = sign_lut[sym & 0x000000FF];
				state_ref = states + KAPPA_SIGN_BASE + (val >> 1);
				sym = val << 31; // Get sign flipping to `sym'
				val = sp[width_by3] & KDU_INT32_MIN; // Get the sign bit
				sym ^= val; // Moves flipped sign bit into `sym'
				_mq_enc_(coder, sym, *state_ref);
				// Broadcast neighbourhood context changes; sign bit is in `val'
				cp[context_row_gap - 1] |= SIGMA_TR_BIT;
				cp[context_row_gap + 1] |= SIGMA_TL_BIT;
				cp[-1] |= (SIGMA_CR_BIT << 9);
				cp[1] |= (SIGMA_CL_BIT << 9);
				if (val < 0)
				{
					cp[context_row_gap] |= SIGMA_TC_BIT | PREV_CHI_BIT;
					cword |= (SIGMA_CC_BIT << 9) | (CHI_BIT << 9);
				}
				else
				{
					cp[context_row_gap] |= SIGMA_TC_BIT;
					cword |= (SIGMA_CC_BIT << 9);
				}
			}
		done:
			cword |= (cword << (MU_POS - SIGMA_CC_POS)) &
				((MU_BIT << 0) | (MU_BIT << 3) | (MU_BIT << 6) | (MU_BIT << 9));
			cword &= ~((PI_BIT << 0) | (PI_BIT << 3) | (PI_BIT << 6) | (PI_BIT << 9));
			*cp = cword;
		}
	_mq_check_in_(coder);
	return distortion_change;
}

/* ========================================================================= */
/*               Distortion-Length Slope Calculating Functions               */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                         slope_to_log                               */
/*****************************************************************************/

static inline kdu_uint16
slope_to_log(double lambda)
{
	double max_slope = pow(2.0, 64.0); // Maximum slope we can represent
	double log_scale = 256.0 / log(2.0); // Convert log values to 8.8 fixed point
	double log_slope = lambda / max_slope;
	log_slope = (log_slope > 1.0) ? 1.0 : log_slope;
	log_slope = (log(log_slope) * log_scale) + (double)(1 << 16);
	if (log_slope > (double)0xFFFF)
		return 0xFFFF;
	else if (log_slope < 2.0)
		return 2;
	else
		return (kdu_uint16)log_slope;
}

/*****************************************************************************/
/* INLINE                       slope_from_log                               */
/*****************************************************************************/

static inline double
slope_from_log(kdu_uint16 slope)
{
	double max_slope = pow(2.0, 64.0);
	double log_scale = log(2.0) / 256.0;
	double log_slope = ((double)slope) - ((double)(1 << 16));
	log_slope *= log_scale;
	return exp(log_slope) * max_slope;
}

/*****************************************************************************/
/* STATIC                      find_convex_hull                              */
/*****************************************************************************/

static void
find_convex_hull(int pass_lengths[], double pass_wmse_changes[],
	kdu_uint16 pass_slopes[], int num_passes)
{
	int z;

	for (z = 0; z < num_passes; z++)
	{ /* Find the value, lambda(z), defined in equation (8.4) of the book by
		 Taubman and Marcellin.  Every coding pass which contributes to the
		 convex hull set has this value for its distortion-length slope;
		 however, not all coding passes which have a positive lambda(z) value
		 actually contribute to the convex hull.  The points which do not
		 contribute are weeded out next. */
		double lambda_z = -1.0; // Marks initial condition.
		double delta_L = 0.0, delta_D = 0.0;
		int t, min_t = (z > 9) ? (z - 9) : 0;
		for (t = z; t >= min_t; t--)
		{
			delta_L += (double)pass_lengths[t];
			delta_D += pass_wmse_changes[t];
			if (delta_D <= 0.0)
			{ // This pass cannot contribute to convex hull
				lambda_z = -1.0; break;
			}
			else if ((delta_L > 0.0) &&
				((lambda_z < 0.0) || ((delta_L*lambda_z) > delta_D)))
				lambda_z = delta_D / delta_L;
		}
		if (lambda_z <= 0.0)
			pass_slopes[z] = 0;
		else
			pass_slopes[z] = slope_to_log(lambda_z);
	}

	/* Now weed out the non-contributing passes. */
	for (z = 0; z < num_passes; z++)
	{
		for (int t = z + 1; t < num_passes; t++)
			if (pass_slopes[t] >= pass_slopes[z])
			{
				pass_slopes[z] = 0; break;
			}
	}
}


/* ========================================================================= */
/*                           kdu_block_encoder                               */
/* ========================================================================= */

/*****************************************************************************/
/*                  kdu_block_encoder::kdu_block_encoder                     */
/*****************************************************************************/

kdu_block_encoder::kdu_block_encoder()
{
	state = new kd_block_encoder;
}


/* ========================================================================= */
/*                            kd_block_encoder                               */
/* ========================================================================= */

/*****************************************************************************/
/*                         kd_block_encoder::encode                          */
/*****************************************************************************/

void
kd_block_encoder::encode(kdu_block *block, bool reversible, double msb_wmse,
	kdu_uint16 estimated_threshold)
{
	double estimated_slope_threshold = -1.0;
	if ((estimated_threshold > 1) && (msb_wmse > 0.0) &&
		(block->orientation != LL_BAND))
		estimated_slope_threshold = slope_from_log(estimated_threshold);
	/* Note: the exclusion of the LL band above is important, since LL
	   subband blocks do not generally have zero mean samples (far from
	   it) so that an approximately uniform block can yield distortion
	   which varies in an unpredictable manner with the number of coded
	   bit-planes.  These unpredictable variations can lead to a large
	   number of consecutive coding passes not lying on the convex hull
	   of the R-D curve -- a condition which would cause the coder
	   to quit prematurely if an estimated slope threshold were given. */

	   /* Allocate space on the stack for a number of largish quantities.  These
		  could be placed elsewhere. */
	double pass_wmse_changes[MAX_POSSIBLE_PASSES];
	mqe_state states[18];
	mq_encoder pass_encoders[MAX_POSSIBLE_PASSES];
	// Get dimensions.
	int num_cols = block->size.x;
	int num_rows = block->size.y;
	int num_stripes = (num_rows + 3) >> 2;
	int context_row_gap = num_cols + EXTRA_ENCODE_CWORDS;
	int num_context_words = (num_stripes + 2)*context_row_gap + 1;

	// Prepare enough storage.
	assert(block->max_samples >= ((num_stripes << 2)*num_cols));
	if (block->max_contexts < num_context_words)
		block->set_max_contexts((num_context_words > 1600) ? num_context_words : 1600);
	kdu_int32 *samples = block->sample_buffer;
	kdu_int32 *context_words = block->context_buffer + context_row_gap + 1;

	// Determine the actual number of passes which we can accomodate.

	int full_reversible_passes = block->num_passes;
	// For reversibility, need all these passes recorded in code-stream.
	if (block->num_passes > ((31 - block->missing_msbs) * 3 - 2))
		block->num_passes = (31 - block->missing_msbs) * 3 - 2;
	if (block->num_passes < 0)
		block->num_passes = 0;

	// Make sure we have sufficient resources to process this number of passes

	if (block->max_passes < block->num_passes)
		block->set_max_passes(block->num_passes + 10, false); // Allocate a few extra

	  // Timing loop starts here.
	int cpu_counter = block->start_timing();
	do {
		// Initialize contexts, marking OOB (Out Of Bounds) locations
		memset(context_words - 1, 0, (size_t)((num_stripes*context_row_gap + 1) << 2));
		if (num_rows & 3)
		{
			kdu_int32 oob_marker;
			if ((num_rows & 3) == 1) // Last 3 rows of last stripe unoccupied
				oob_marker = (OOB_MARKER << 3) | (OOB_MARKER << 6) | (OOB_MARKER << 9);
			else if ((num_rows & 3) == 2) // Last 2 rows of last stripe unused
				oob_marker = (OOB_MARKER << 6) | (OOB_MARKER << 9);
			else
				oob_marker = (OOB_MARKER << 9);
			kdu_int32 *cp = context_words + (num_stripes - 1)*context_row_gap;
			for (int k = num_cols; k > 0; k--)
				*(cp++) = oob_marker;
		}
		if (context_row_gap > num_cols)
		{ // Initialize the extra context words between lines to OOB
			kdu_int32 oob_marker =
				OOB_MARKER | (OOB_MARKER << 3) | (OOB_MARKER << 6) | (OOB_MARKER << 9);
			assert(context_row_gap >= (num_cols + 3));
			kdu_int32 *cp = context_words + num_cols;
			for (int k = num_stripes; k > 0; k--, cp += context_row_gap)
				cp[0] = cp[1] = cp[2] = oob_marker; // Need 3 OOB words after line
		}

		double pass_wmse_scale = msb_wmse / KD_DISTORTION_LUT_SCALE;
		pass_wmse_scale = pass_wmse_scale / ((double)(1 << 16));
		for (int i = block->missing_msbs; i > 0; i--)
			pass_wmse_scale *= 0.25;
		int p_max = 30 - block->missing_msbs; // Index of most significant plane
		int p = p_max; // Bit-plane counter
		int z = 0; // Coding pass index
		int k = 2; // Coding pass category; start with cleanup pass
		int segment_passes = 0; // Num coding passes in current codeword segment.
		kdu_byte *seg_buf = block->byte_buffer; // Start of current segment
		int segment_bytes = 0; // Bytes consumed so far by this codeword segment
		int first_unsized_z = 0; // First pass whose length is not yet known
		int available_bytes = block->max_bytes; // For current & future segments
		bool bypass = false;
		bool causal = (block->modes & Cmodes_CAUSAL) != 0;
		bool optimize = !(block->modes & Cmodes_ERTERM);

		for (; z < block->num_passes; z++, k++)
		{
			if (k == 3)
			{ // Move on to next bit-plane.
				k = 0; p--;
				pass_wmse_scale *= 0.25;
			}

			// See if we need to augment byte buffer resources to safely continue
			if ((available_bytes - segment_bytes) < 4096)
			{ // We could build a much more thorough test for sufficiency.
				assert(available_bytes >= segment_bytes); // else already overrun
				kdu_byte *old_handle = block->byte_buffer;
				block->set_max_bytes(block->max_bytes + 8192, true);
				available_bytes += 8192;
				kdu_byte *new_handle = block->byte_buffer;
				for (int i = 0; i < z; i++)
					pass_encoders[i].augment_buffer(old_handle, new_handle);
				seg_buf = new_handle + (seg_buf - old_handle);
			}

			// Either start a new codeword segment, or continue an earlier one.
			if (segment_passes == 0)
			{ // Need to start a new codeword segment.
				segment_passes = block->num_passes;
				if (block->modes & Cmodes_BYPASS)
				{
					if (z < 10)
						segment_passes = 10 - z;
					else if (k == 2) // Cleanup pass.
					{
						segment_passes = 1; bypass = false;
					}
					else
					{
						segment_passes = 2;
						bypass = true;
					}
				}
				if (block->modes & Cmodes_RESTART)
					segment_passes = 1;
				if ((z + segment_passes) > block->num_passes)
					segment_passes = block->num_passes - z;
				pass_encoders[z].start(seg_buf, !bypass);
			}
			else
				pass_encoders[z].continues(pass_encoders + z - 1);

			// Encoding steps for the pass
			kdu_int32 distortion_change;
			bool lossless_pass = reversible && ((31 - p) == block->K_max_prime);

			if ((z == 0) || (block->modes & Cmodes_RESET))
				reset_states(states);
			if ((k == 0) && !bypass)
				distortion_change =
				encode_sig_prop_pass(pass_encoders[z],
					states, p, causal, block->orientation,
					samples, context_words,
					num_cols, num_stripes, context_row_gap,
					lossless_pass);
			else if (k == 0)
				distortion_change =
				encode_sig_prop_pass_raw(pass_encoders[z],
					p, causal, samples, context_words,
					num_cols, num_stripes, context_row_gap,
					lossless_pass);
			else if ((k == 1) && !bypass)
				distortion_change =
				encode_mag_ref_pass(pass_encoders[z],
					states, p, causal, samples, context_words,
					num_cols, num_stripes, context_row_gap,
					lossless_pass);
			else if (k == 1)
				distortion_change =
				encode_mag_ref_pass_raw(pass_encoders[z],
					p, causal, samples, context_words,
					num_cols, num_stripes, context_row_gap,
					lossless_pass);
			else
				distortion_change =
				encode_cleanup_pass(pass_encoders[z],
					states, p, causal, block->orientation,
					samples, context_words,
					num_cols, num_stripes, context_row_gap,
					lossless_pass);
			pass_wmse_changes[z] = pass_wmse_scale * distortion_change;
			if ((block->modes & Cmodes_SEGMARK) && (k == 2))
			{
				kdu_int32 segmark = 0x0A;
				pass_encoders[z].mq_encode_run(segmark >> 2);
				pass_encoders[z].mq_encode_run(segmark & 3);
			}

			// Update codeword segment status.
			segment_passes--;
			segment_bytes = pass_encoders[z].get_bytes_used();
			if (segment_passes == 0)
			{
				kdu_byte *new_buf = pass_encoders[z].terminate(optimize);
				available_bytes -= (int)(new_buf - seg_buf);
				seg_buf = new_buf;
				segment_bytes = 0;
			}

			{ // Incrementally determine and process truncation lengths
				int t;
				bool final;
				for (t = first_unsized_z; t <= z; t++)
				{
					block->pass_lengths[t] =
						pass_encoders[t].get_incremental_length(final);
					if (final)
					{
						assert(first_unsized_z == t); first_unsized_z++;
					}
				}
			}

			if (estimated_slope_threshold > 0.0)
			{ // See if we can finish up early to save time and memory.
			  /* The heuristic is as follows:
				   1) We examine potential truncation points (coding passes),
					  t <= z, finding the distortion length slopes, S_t,
					  which would arise at those points if they were to lie
					  on the convex hull.  This depends only upon distortion
					  and length information for coding passes <= t.
				   2) Let I=[z0,z] be the smallest interval which contains
					  three potential convex hull points, t1 < t2 < t3.
					  By "potential convex hull points", we simply mean that
					  S_t1 >= S_t2 >= S_t3 > 0.  If no such interval exists,
					  set z0=0 and I=[0,z].
				   3) We terminate coding here if all the following conditions
					  hold:
					  a) Interval I contains at least 3 points.
					  b) Interval I contains at least 2 potential convex hull
						 points.
					  c) For each t in I=[z0,z], S_t < alpha(z-t) * S_min,
						 where S_min is the value of `expected_slope_threshold'
						 and alpha(q) = 3^floor(q/3).  This means that
						 alpha(z-t) = 1 for t in {z,z-1,z-2};
						 alpha(z-t) = 3 for t in {z-3,z-4,z-5}; etc.
						 The basic idea is to raise the slope threshold by
						 3 each time we move to another bit-plane.  The base
						 of 3 represents a degree of conservatism, since at
						 high bit-rates we expect the slope to change by a
						 factor of 4 for each extra bit-plane.
			  */
				int num_hull_points = 0;
				int u, z0;
				double last_deltaD, last_deltaL;
				double deltaD, deltaL, best_deltaD, best_deltaL;
				for (z0 = z; (z0 >= 0) && (num_hull_points < 3); z0--)
				{
					for (deltaL = 0.0, deltaD = 0.0, u = z0;
						(u >= 0) && (u > (z0 - 7)); u--)
					{
						deltaL += block->pass_lengths[u];
						deltaD += pass_wmse_changes[u];
						if ((u == z0) ||
							((best_deltaD*deltaL) > (deltaD*best_deltaL)))
						{
							best_deltaD = deltaD;  best_deltaL = deltaL;
						}
					}
					double ref = estimated_slope_threshold * best_deltaL;
					for (u = z - z0; u > 2; u -= 3)
						ref *= 3.0;
					if (best_deltaD > ref)
					{ // Failed condition 3.c
						num_hull_points = 0; // Prevent truncation
						break;
					}
					if ((best_deltaD > 0.0) && (best_deltaL > 0.0))
					{ // Might be a potential convex hull point
						if ((num_hull_points > 0) &&
							((best_deltaD*last_deltaL) <=
							(last_deltaD*best_deltaL)))
							continue; // Cannot be a convex hull point
						last_deltaD = best_deltaD;
						last_deltaL = best_deltaL;
						num_hull_points++;
					}
				}
				if ((num_hull_points >= 2) && ((z - z0) >= 3))
				{
					block->num_passes = z + 1; // No point in coding more passes.
					if (segment_passes > 0)
					{ // Need to terminate the segment
						pass_encoders[z].terminate(optimize);
						for (u = first_unsized_z; u <= z; u++)
						{
							bool final;
							block->pass_lengths[u] =
								pass_encoders[u].get_incremental_length(final);
							assert(final);
						}
						first_unsized_z = u;
						segment_passes = 0;
					}
				}
			}

			if (segment_passes == 0)
			{ // Finish cleaning up the completed codeword segment.
				assert(first_unsized_z == (z + 1));
				pass_encoders[z].finish();
			}
		}

		assert(segment_passes == 0);
		assert(first_unsized_z == block->num_passes);

		if (msb_wmse > 0.0)
		{ /* Form the convex hull set.  We could have done most of the work
			 incrementally as the lengths became available -- see above.
			 However, there is little value to this in the present
			 implementation.  A hardware implementation may prefer the
			 incremental calculation approach for all quantities, so that
			 slopes and lengths can be dispatched in a covnenient interleaved
			 fashion to memory as they become available. */
			find_convex_hull(block->pass_lengths, pass_wmse_changes,
				block->pass_slopes, block->num_passes);
			if (reversible && (block->num_passes > 0) &&
				(block->num_passes == full_reversible_passes))
			{ /* Must force last coding pass onto convex hull.  Otherwise, a
				 decompressor using something other than the mid-point rounding
				 rule can fail to achieve lossless decompression. */
				z = full_reversible_passes - 1;
				if (block->pass_slopes[z] == 0)
					block->pass_slopes[z] = 1; /* Works because
						  `find_convex_hull_slopes' never generates slopes of 1. */
			}
		}
	} while ((--cpu_counter) > 0);
	block->finish_timing();
}
