/*****************************************************************************/
// File: block_coding_common.h [scope = CORESYS/CODING]
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
   Provides definitions common to the block encoder and block decoder.  These
are local definitions, not to be included from any other scope.  They reflect
one particular implementation approach to implementing the bit-plane coding
primitives.
******************************************************************************/

#ifndef BLOCK_CODING_COMMON_H
#define BLOCK_CODING_COMMON_H

#include "kdu_elementary.h"

/*****************************************************************************/
/*                           Context State Labels                            */
/*****************************************************************************/

#define KAPPA_SIG_BASE   0 // First of 9 labels, kappa_sig -- this must stay 0!
#define KAPPA_RUN_BASE   9 // First and only label, kappa_run
#define KAPPA_SIGN_BASE 10 // First of 5 labels, kappa_sign
#define KAPPA_MAG_BASE  15 // First of 3 labels, kappa_run
#define KAPPA_NUM_STATES 18 // Total number of probability states

/*****************************************************************************/
/*                            Context Word Bits                              */
/*****************************************************************************/

  /* The block coder makes heavy use of a 32-bit context word structure which
	 maintains partially broadcast state information for a full stripe column.
	 To appreciate state broadcasting and the various merits of the context
	 word structure employed here, refer to Section 17.1.2 in the book by
	 Taubman and Marcellin.  The context word structure is identical to that
	 depicted in Figure 17.2 of the book and the naming conventions used here
	 are designed to coincide with the symbols used in the figure. */

	 /* The following 9 flag bits are used to construct significance coding
		context labels (denoted kappa_sig in the book). The bit positions
		of these flags for the first, second, third and fourth sample in each
		stripe column are obtained by adding 0, 3, 6 or 9 to the following
		constants.  The 18 least significant bit positions of the 32-bit
		context word are thus occupied by these significance flags. */
#define SIGMA_TL_POS  0 // Indicates significance of top-left neighbour
#define SIGMA_TC_POS  1 // Indicates significance of top-centre neighbour
#define SIGMA_TR_POS  2 // Indicates significance of top-right neighbour
#define SIGMA_CL_POS  3 // Indicates significance of centre-left neighbour
#define SIGMA_CC_POS  4 // Indicates significance of current sample
#define SIGMA_CR_POS  5 // Indicates significance of centre-right neighbour
#define SIGMA_BL_POS  6 // Indicates significance of bottom-left neighbour
#define SIGMA_BC_POS  7 // Indicates significance of bottom-centre neighbour
#define SIGMA_BR_POS  8 // Indicates significance of bottom-right neighbour

#define SIGMA_TL_BIT  ((kdu_int32)(1<<SIGMA_TL_POS))
#define SIGMA_TC_BIT  ((kdu_int32)(1<<SIGMA_TC_POS))
#define SIGMA_TR_BIT  ((kdu_int32)(1<<SIGMA_TR_POS))
#define SIGMA_CL_BIT  ((kdu_int32)(1<<SIGMA_CL_POS))
#define SIGMA_CC_BIT  ((kdu_int32)(1<<SIGMA_CC_POS))
#define SIGMA_CR_BIT  ((kdu_int32)(1<<SIGMA_CR_POS))
#define SIGMA_BL_BIT  ((kdu_int32)(1<<SIGMA_BL_POS))
#define SIGMA_BC_BIT  ((kdu_int32)(1<<SIGMA_BC_POS))
#define SIGMA_BR_BIT  ((kdu_int32)(1<<SIGMA_BR_POS))

		/* The following bit mask may be used to determine whether the first sample
		   in a stripe column has any significant neighbours.  Shifting the mask
		   by 3, 6 and 9 yields neighbourhood masks for the second, third and
		   fourth samples in the stripe column. */
#define NBRHD_MASK  (SIGMA_TL_BIT | SIGMA_TC_BIT | SIGMA_TR_BIT | \
                     SIGMA_CL_BIT |                SIGMA_CR_BIT | \
                     SIGMA_BL_BIT | SIGMA_BC_BIT | SIGMA_BR_BIT)

		   /* The following three flag bits are used to identify the sign of any
			  significant sample in a stripe column, and also to control coding
			  pass membership in an efficient manner, including the exclusion of
			  samples which are outside the code-block boundaries, i.e.
			  "out-of-bounds" (OOB). The bit positions identified below correspond
			  to the flags for the first sample in each stripe.  Add 3, 6 and 9 to
			  these to obtain the flag bit positions for subsequent stripe rows.
			  The fact that consecutive stripe rows have all flag bits (including
			  significance) spaced apart by 3 bit positions, greatly facilitates
			  efficient manipulation.  For an explanation of coding pass membership
			  assessment and  OOB identification using these flags, refer to
			  Table 17.1 in the book by Taubman and Marcellin. */
#define CHI_POS  21 // 0 if insignificant or positive; 1 for negative.
#define MU_POS   19 // 1 if sample is to be coded in mag-ref pass.
#define PI_POS   20 // 1 if sample was coded in sig-prop pass.

#define CHI_BIT  ((kdu_int32)(1<<CHI_POS))
#define MU_BIT   ((kdu_int32)(1<<MU_POS))
#define PI_BIT   ((kdu_int32)(1<<PI_POS))

#define OOB_MARKER (CHI_BIT) // set this bit to prevent inclusion in any pass
#define SIG_PROP_MEMBER_MASK (SIGMA_CC_BIT | CHI_BIT) // Needs both bits off
#define CLEANUP_MEMBER_MASK  (SIGMA_CC_BIT | PI_BIT | CHI_BIT) // Need all 0

			  /* The following are the only bits free in the 32 bit word.  They are
				 used to store the sign of the sample immediately above (previous)
				 the first sample in the stripe column and immediately below
				 (next) the last sample in the stripe column.  This greatly simplifies
				 the construction of sign coding contexts when necessary.
				 Note that the NEXT_CHI bit follows the same progression, in groups of 3,
				 as all of the other flags, so that it is separated by 3 bit positions
				 from the sign of the last sample in the stripe column.  It is,
				 unfortunately, not possible to preserve this relationship for the
				 PREV_CHI bit, which slightly complicates sign coding for the first
				 sample in each quad. */

#define PREV_CHI_POS   18
#define NEXT_CHI_POS   31

#define PREV_CHI_BIT   ((kdu_int32)(1<<PREV_CHI_POS))
#define NEXT_CHI_BIT   ((kdu_int32)(1<<NEXT_CHI_POS))

				 /* The following identify the 8 flag bits in the index supplied to
					the sign coding lookup table.  They are derived from the sign bits
					in three `quad_flags' words using simple logical operations. */

#define UP_NBR_SIGMA_POS    0
#define UP_NBR_CHI_POS      1
#define LEFT_NBR_SIGMA_POS  2
#define LEFT_NBR_CHI_POS    3
#define RIGHT_NBR_SIGMA_POS 4
#define RIGHT_NBR_CHI_POS   5
#define DOWN_NBR_SIGMA_POS  6
#define DOWN_NBR_CHI_POS    7

					/* ========================================================================= */
					/*                  Assembler Friendly Versions of Flag Macros               */
					/* ========================================================================= */

#define __SIGMA_TL_BIT  0x00000001
#define __SIGMA_TC_BIT  0x00000002
#define __SIGMA_TR_BIT  0x00000004
#define __SIGMA_CL_BIT  0x00000008
#define __SIGMA_CC_BIT  0x00000010
#define __SIGMA_CR_BIT  0x00000020
#define __SIGMA_BL_BIT  0x00000040
#define __SIGMA_BC_BIT  0x00000080
#define __SIGMA_BR_BIT  0x00000100

#define __NBRHD_MASK    0x000001EF  // 9 LSB's bits set, except bit 4
#define __CHI_BIT       0x00200000  // Bit 21
#define __MU_BIT        0x00080000  // Bit 19
#define __PI_BIT        0x00100000  // Bit 20

#define __SIG_PROP_MEMBER_MASK  0x00200010 // CC_BIT and CHI_BIT set
#define __CLEANUP_MEMBER_MASK   0x00300010 // CC_BIT and CHI_BIT and PI_BIT set

#define __PREV_CHI_BIT  0x00040000  // Bit 18
#define __NEXT_CHI_BIT  0x80000000  // Bit 31

/* ========================================================================= */
/*                                External Storage                           */
/* ========================================================================= */

extern kdu_byte hl_sig_lut[512];
extern kdu_byte lh_sig_lut[512];
extern kdu_byte hh_sig_lut[512];
extern kdu_byte sign_lut[256];

#endif // BLOCK_CODING_COMMON_H
