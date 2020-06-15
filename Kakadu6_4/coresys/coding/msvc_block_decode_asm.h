/*****************************************************************************/
// File: msvc_block_decode_asm.h [scope = CORESYS/CODING]
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
   Provides implementations of the principle block decoding pass functions
(significance propagation, magnitude refinement and cleanup) for the X86 family
of processors.  The assembly code should work on any member of the Intel X86
family which provides support for MMX (the test for MMX might fail on CPU's
more ancient than the 80486).  The couple of places where MMX instructions are
used here could easily be eliminated if necessary.  The code is fairly heavily
optimized, although there may be innovative ways to squeeze a few more percent
out of the throughput.  Interestingly, a decent optimizing C++ compiler can
get within about 80% of the speed of these functions when compiling the
carefully optimized machine independent versions in "block_decoder.cpp".  One
advantage of the versions given here is that the code size is reduced by
a factor of more than 2, which may improve L1 cache utilization.
   For other processors, the assembly code here may still prove useful as
a guide.  In particular, it provides a rough indication of the number of
instructions which are required and suggests appropriate register assignments.
Here, we use only the 6 truly general purpose registers left over by the X86
family, which is rather limiting.
******************************************************************************/

#ifndef MSVC_BLOCK_DECODE_ASM_H
#define MSVC_BLOCK_DECODE_ASM_H

/* ========================================================================= */
/*                   Assembler Optimized MQ Decoding Macros                  */
/* ========================================================================= */

/*****************************************************************************/
/* __ASM                     ASM_MQ_NON_CDP_DECODE                           */
/*****************************************************************************/

  /* This macro contains code which implements non-CDP (not Common Decode
	 Path) MQ symbol decoding steps.  It may be included directly wherever
	 the steps are required or else it may be included inside a fast-call
	 function.
		The internal MQ state variables manipulated here are expected to reside
	 in registers or on the stack (referenced via the EBP register).  The
	 relevant stack variables have the names C_var, D_var, t_var, temp_var
	 and store_var.
		On entry, register EAX contains the symbol value associated with MPS
	 decoding.  The function should not load this value directly, but simply
	 flip the bit if the symbol turns out to be an LPS.  ECX contains the
	 address of the context state entry which is being manipulated.  EDX and
	 ESI contain the current values of D_var and A_var, respectively.
	 The EBX register is undefined and may be used without risk of corrupting
	 anything else.
		On exit, register EAX should contain the symbol value associated
	 with MPS decoding.  Note that this symbol must be equal to the entry value
	 of EAX if an MPS was decoded (regardless of whether this agrees with
	 the MPS value stored at [ECX].  There is no need to preserve the original
	 value of ECX. */

#define ASM_MQ_NON_CDP_DECODE                                                \
  __asm MOV EBX,[ECX] /* Get 'p_bar_mps' to EBX */                           \
  __asm ADD ESI,EDX /* Add D to the value of A, stored in ESI */             \
  __asm BTR EBX,0 /* Discard MPS, leaving just 'p_bar'. */                   \
  __asm ADD EDX,C_var /* Get C+D to EDX */                                   \
  __asm JL non_cdp_Clt0                                                      \
    /* Upper sub-interval selected */                                        \
      __asm CMP ESI,EBX /* Compare A with p_bar */                           \
      __asm JGE non_cdp_Cge0_AgeP                                            \
        /* Conditional exchange; LPS is decoded. */                          \
          __asm XOR EAX,1                                                    \
          __asm MOV EBX,[ECX+4] /* Get pointer to `state->transition' */     \
          __asm MOVQ MM0,[EBX+8] /* Load `state->transition->lps' */         \
          __asm MOVQ [ECX],MM0 /* Save back to *state */                     \
          __asm JMP non_cdp_renorm                                           \
      __asm non_cdp_Cge0_AgeP:                                               \
        /* MPS is decoded. */                                                \
          __asm MOV EBX,[ECX+4] /* Get pointer to `state->transition' */     \
          __asm MOVQ MM0,[EBX] /* Load `state->transition->mps' */           \
          __asm MOVQ [ECX],MM0 /* Save back to *state */                     \
          __asm JMP non_cdp_renorm                                           \
  __asm non_cdp_Clt0:                                                        \
    /* Lower sub-interval selected */                                        \
      __asm ADD EDX,EBX /* Add `p_bar' to C in EDX */                        \
      __asm CMP ESI,EBX /* Compare A with p_bar */                           \
      __asm MOV ESI,EBX /* Move `p_bar' into `A' */                          \
      __asm JGE non_cdp_Clt0_AgeP                                            \
        /* Conditional exchange; MPS is decoded */                           \
          __asm MOV EBX,[ECX+4] /* Get pointer to `state->transition' */     \
          __asm MOVQ MM0,[EBX] /* Load `state->transition->mps' */           \
          __asm MOVQ [ECX],MM0 /* Save back to *state */                     \
          __asm JMP non_cdp_renorm                                           \
      __asm non_cdp_Clt0_AgeP:                                               \
        /* LPS is decoded. */                                                \
          __asm XOR EAX,1                                                    \
          __asm MOV EBX,[ECX+4] /* Get pointer to `state->transition' */     \
          __asm MOVQ MM0,[EBX+8] /* Load `state->transition->lps' */         \
          __asm MOVQ [ECX],MM0 /* Save back to *state */                     \
  __asm non_cdp_renorm:                                                      \
  /* At this point, ESI holds A, EDX holds C and ECX and EBX are free */     \
  __asm MOV EBX,t_var                                                        \
  __asm non_cdp_renorm_once:                                                 \
    /* Come back here repeatedly until we have shifted enough bits out. */   \
      __asm TEST EBX,EBX                                                     \
      __asm JNZ non_cdp_have_more_bits                                       \
        /* Begin `fill_lsbs' procedure */                                    \
          __asm MOV ECX,temp_var                                             \
          __asm CMP ECX,0xFF                                                 \
          __asm JNZ non_cdp_no_ff                                            \
            __asm MOV EBX,store_var                                          \
            __asm MOVZX ECX,byte ptr [EBX] /* New `temp' in ECX */           \
            __asm CMP ECX,0x8F                                               \
            __asm JLE non_cdp_temp_le8F                                      \
              /* Reached a termination marker.  Don't update `store_var' */  \
              __asm MOV ECX,0xFF                                             \
              __asm MOV EBX,S_var                                            \
              __asm ADD EBX,1                                                \
              __asm MOV S_var,EBX                                            \
              __asm MOV EBX,8 /* New value of t */                           \
              __asm JMP non_cdp_complete_fill_lsbs                           \
            __asm non_cdp_temp_le8F:                                         \
            /* No termination marker, but bit stuff required */              \
              __asm ADD EDX,ECX /* Add `temp' once; another later */         \
              __asm ADD EBX,1                                                \
              __asm MOV store_var,EBX /* Increments the `store' pointer */   \
              __asm MOV EBX,7 /* New value of t. */                          \
              __asm JMP non_cdp_complete_fill_lsbs                           \
          __asm non_cdp_no_ff:                                               \
            __asm MOV EBX,store_var                                          \
            __asm MOVZX ECX,byte ptr [EBX] /* New `temp' in ECX */           \
            __asm ADD EBX,1                                                  \
            __asm MOV store_var,EBX /* Increments the `store' pointer */     \
            __asm MOV EBX,8 /* New value of t. */                            \
          __asm non_cdp_complete_fill_lsbs:                                  \
          __asm ADD EDX,ECX /* Add `temp' to C */                            \
          __asm MOV temp_var,ECX /* Save `temp' */                           \
      /* End `fill_lsbs' procedure */                                        \
      __asm non_cdp_have_more_bits:                                          \
      __asm ADD ESI,ESI /* Shift A register left */                          \
      __asm ADD EDX,EDX /* Shift C register left */                          \
      __asm SUB EBX,1   /* Decrement t */                                    \
      __asm TEST ESI,0x00800000 /* Compare with MQD_A_MIN (test bit 23) */   \
      __asm JZ non_cdp_renorm_once                                           \
  __asm MOV t_var,EBX /* Save t */                                           \
  /* Renormalization is complete.  Recompute D and save A and C. */          \
  __asm MOV EBX,ESI /* Load EBX with A */                                    \
  __asm SUB EBX,0x00800000 /* Subtract MQD_A_MIN */                          \
  __asm CMP EDX,EBX /* Compare C with A-MQD_A_MIN */                         \
  __asm CMOVL EBX,EDX /* Move C to ESI if C < A-MQD_A_MIN */                 \
  __asm SUB ESI,EBX /* Subtract D from A */                                  \
  __asm SUB EDX,EBX /* Subtract D from C */                                  \
  __asm MOV C_var,EDX                                                        \
  __asm MOV EDX,EBX /* Move D back into the EDX register */ 


	 /*****************************************************************************/
	 /* __ASM                       ASM_MQ_DECODE_RUN                             */
	 /*****************************************************************************/

	   /* Similar to ASM_MQ_NON_CDP_DECODE, except that the full decoding is
		  performed, returning a 2-bit run length in EAX, and ECX is free to be
		  overwritten. */

#define ASM_MQ_DECODE_RUN                                                    \
  __asm XOR EAX,EAX                                                          \
  __asm MOV ECX,C_var                                                        \
  __asm SUB EDX,0x00560100                                                   \
  __asm JGE run_bit0                                                         \
    /* Non-CDP decoding for the first bit */                                 \
    __asm ADD ESI,EDX /* Add D to A in ESI */                                \
    __asm ADD ECX,EDX /* Add D to C in ECX */                                \
    __asm JL run_bit1_Clt0                                                   \
      /* Upper sub-interval selected */                                      \
      __asm CMP ESI,0x00560100                                               \
      __asm JGE run_bit1_renorm                                              \
      __asm MOV EAX,2                                                        \
      __asm JMP run_bit1_renorm                                              \
    __asm run_bit1_Clt0:                                                     \
      /* Lower sub-interval selected */                                      \
      __asm ADD ECX,0x00560100                                               \
      __asm CMP ESI,0x00560100                                               \
      __asm MOV ESI,0x00560100                                               \
      __asm JL run_bit1_renorm                                               \
      __asm MOV EAX,2                                                        \
    __asm run_bit1_renorm:                                                   \
    /* At this point, EBX holds A, ECX holds C and EDX and ESI are free */   \
    __asm MOV EBX,t_var                                                      \
    __asm run_bit1_renorm_once:                                              \
      /* Come back here repeatedly until we have shifted enough bits out. */ \
      __asm TEST EBX,EBX                                                     \
      __asm JNZ run_bit1_have_more_bits                                      \
        /* Begin `fill_lsbs' procedure */                                    \
          __asm MOV EDX,temp_var                                             \
          __asm CMP EDX,0xFF                                                 \
          __asm JNZ run_bit1_no_ff                                           \
            __asm MOV EBX,store_var                                          \
            __asm MOVZX EDX,byte ptr [EBX] /* New `temp' in EDX */           \
            __asm CMP EDX,0x8F                                               \
            __asm JLE run_bit1_temp_le8F                                     \
              /* Reached a termination marker.  Don't update `store_var' */  \
              __asm MOV EDX,0xFF                                             \
              __asm MOV EBX,S_var                                            \
              __asm ADD EBX,1                                                \
              __asm MOV S_var,EBX                                            \
              __asm MOV EBX,8 /* New value of t */                           \
              __asm JMP run_bit1_complete_fill_lsbs                          \
            __asm run_bit1_temp_le8F:                                        \
            /* No termination marker, but bit stuff required */              \
              __asm ADD ECX,EDX /* Add `temp' once; another later */         \
              __asm ADD EBX,1                                                \
              __asm MOV store_var,EBX /* Increments the `store' pointer */   \
              __asm MOV EBX,7 /* New value of t. */                          \
              __asm JMP run_bit1_complete_fill_lsbs                          \
          __asm run_bit1_no_ff:                                              \
            __asm MOV EBX,store_var                                          \
            __asm MOVZX EDX,byte ptr [EBX] /* New `temp' in EDX */           \
            __asm ADD EBX,1                                                  \
            __asm MOV store_var,EBX /* Increments the `store' pointer */     \
            __asm MOV EBX,8 /* New value of t. */                            \
          __asm run_bit1_complete_fill_lsbs:                                 \
          __asm ADD ECX,EDX /* Add `temp' to C */                            \
          __asm MOV temp_var,EDX /* Save `temp' */                           \
      /* End `fill_lsbs' procedure */                                        \
      __asm run_bit1_have_more_bits:                                         \
      __asm ADD ESI,ESI /* Shift A register left */                          \
      __asm ADD ECX,ECX /* Shift C register left */                          \
      __asm SUB EBX,1   /* Decrement t */                                    \
      __asm TEST ESI,0x00800000 /* Compare with MQD_A_MIN (test bit 23) */   \
      __asm JZ run_bit1_renorm_once                                          \
    __asm MOV t_var,EBX /* Save t */                                         \
    /* Renormalization is complete.  Recompute D and save A and C. */        \
    __asm MOV EDX,ESI /* Load EDX with A */                                  \
    __asm SUB EDX,0x00800000 /* Subtract MQD_A_MIN */                        \
    __asm CMP ECX,EDX                                                        \
    __asm CMOVL EDX,ECX /* Move C to EDX if C < A-MQD_A_MIN */               \
    __asm SUB ESI,EDX /* Subtract D from A */                                \
    __asm SUB ECX,EDX /* Subtract D from C */                                \
  /************************************************************************/ \
  __asm run_bit0:                                                            \
  __asm SUB EDX,0x00560100                                                   \
  __asm JGE run_finished                                                     \
    /* Non-CDP decoding for the second bit */                                \
    __asm ADD ESI,EDX /* Add D to A */                                       \
    __asm ADD ECX,EDX /* Add D to C */                                       \
    __asm JL run_bit0_Clt0                                                   \
      /* Upper sub-interval selected */                                      \
      __asm CMP ESI,0x00560100                                               \
      __asm JGE run_bit0_renorm                                              \
      __asm ADD EAX,1                                                        \
      __asm JMP run_bit0_renorm                                              \
    __asm run_bit0_Clt0:                                                     \
      /* Lower sub-interval selected */                                      \
      __asm ADD ECX,0x00560100                                               \
      __asm CMP ESI,0x00560100                                               \
      __asm MOV ESI,0x00560100                                               \
      __asm JL run_bit0_renorm                                               \
      __asm ADD EAX,1                                                        \
    __asm run_bit0_renorm:                                                   \
    /* At this point, EBX holds A, ECX holds C and EDX and ESI are free */   \
    __asm MOV EBX,t_var                                                      \
    __asm run_bit0_renorm_once:                                              \
      /* Come back here repeatedly until we have shifted enough bits out. */ \
      __asm TEST EBX,EBX                                                     \
      __asm JNZ run_bit0_have_more_bits                                      \
        /* Begin `fill_lsbs' procedure */                                    \
          __asm MOV EDX,temp_var                                             \
          __asm CMP EDX,0xFF                                                 \
          __asm JNZ run_bit0_no_ff                                           \
            __asm MOV EBX,store_var                                          \
            __asm MOVZX EDX,byte ptr [EBX] /* New `temp' in EDX */           \
            __asm CMP EDX,0x8F                                               \
            __asm JLE run_bit0_temp_le8F                                     \
              /* Reached a termination marker.  Don't update `store_var' */  \
              __asm MOV EDX,0xFF                                             \
              __asm MOV EBX,S_var                                            \
              __asm ADD EBX,1                                                \
              __asm MOV S_var,EBX                                            \
              __asm MOV EBX,8 /* New value of t */                           \
              __asm JMP run_bit0_complete_fill_lsbs                          \
            __asm run_bit0_temp_le8F:                                        \
            /* No termination marker, but bit stuff required */              \
              __asm ADD ECX,EDX /* Add `temp' once; another later */         \
              __asm ADD EBX,1                                                \
              __asm MOV store_var,EBX /* Increments the `store' pointer */   \
              __asm MOV EBX,7 /* New value of t. */                          \
              __asm JMP run_bit0_complete_fill_lsbs                          \
          __asm run_bit0_no_ff:                                              \
            __asm MOV EBX,store_var                                          \
            __asm MOVZX EDX,byte ptr [EBX] /* New `temp' in EDX */           \
            __asm ADD EBX,1                                                  \
            __asm MOV store_var,EBX /* Increments the `store' pointer */     \
            __asm MOV EBX,8 /* New value of t. */                            \
          __asm run_bit0_complete_fill_lsbs:                                 \
          __asm ADD ECX,EDX /* Add `temp' to C */                            \
          __asm MOV temp_var,EDX /* Save `temp' */                           \
      /* End `fill_lsbs' procedure */                                        \
      __asm run_bit0_have_more_bits:                                         \
      __asm ADD ESI,ESI /* Shift A register left */                          \
      __asm ADD ECX,ECX /* Shift C register left */                          \
      __asm SUB EBX,1   /* Decrement t */                                    \
      __asm TEST ESI,0x00800000 /* Compare with MQD_A_MIN (test bit 23) */   \
      __asm JZ run_bit0_renorm_once                                          \
    __asm MOV t_var,EBX /* Save t */                                         \
    /* Renormalization is complete.  Recompute D and save A and C. */        \
    __asm MOV EDX,ESI /* Load EDX with A */                                  \
    __asm SUB EDX,0x00800000 /* Subtract MQD_A_MIN */                        \
    __asm CMP ECX,EDX                                                        \
    __asm CMOVL EDX,ECX /* Move C to EDX if C < A-MQD_A_MIN */               \
    __asm SUB ESI,EDX                                                        \
    __asm SUB ECX,EDX                                                        \
  __asm run_finished:                                                        \
  __asm MOV C_var,ECX


		  /* ========================================================================= */
		  /*                            Coding Pass Functions                          */
		  /* ========================================================================= */

		  /*****************************************************************************/
		  /* STATIC                    asm_decode_sig_prop_pass                        */
		  /*****************************************************************************/

static void
asm_decode_sig_prop_pass(mq_decoder &coder, mqd_state states[],
	int p, bool causal, int orientation,
	kdu_int32 *samples, kdu_int32 *contexts,
	int width, int num_stripes, int context_row_gap)
{
	if ((num_stripes <= 0) || (width <= 0))
		return;
	kdu_int32 A_var, C_var, D_var, t_var, temp_var;
	kdu_byte *store_var;
	int S_var;
	coder.check_out(A_var, C_var, D_var, t_var, temp_var, store_var, S_var);
	kdu_byte *sig_lut = significance_luts[orientation];
	kdu_int32 one_point_five = 1 << p; one_point_five += (one_point_five >> 1);
	int is_causal = (causal) ? 1 : 0;
	context_row_gap <<= 2; // Convert to number of bytes
	width <<= 2; // Convert to number of bytes
	  /* The following offsets are used to convert a context word pointer
		 to a sample pointer.  The offsets are correct for the first
		 stripe and must be adjusted from stripe to stripe by the addition
		 of 3*width-EXTRA_DECODE_CWORDS. */
	int cp_to_sp0 = (int)((samples - contexts) << 2); // Offset for row 0
	int cp_to_sp1 = cp_to_sp0 + width; // Offset for row 1
	int cp_to_sp2 = cp_to_sp1 + width; // Offset for row 2
	int cp_to_sp3 = cp_to_sp2 + width; // Offset for row 3
	int cp_to_sp_adjust = 3 * width - 4 * EXTRA_DECODE_CWORDS;
	kdu_int32 *stripe_end; // Points to context word beyond end of current stripe

	__asm {
		MOV EDI, contexts // Set EDI aside to hold the context word pointer
		MOV EDX, D_var // Set EDX aside to hold the critical MQ variable, D
		MOV ESI, A_var // Set ESI aside to hold the MQ coder's A register
		next_stripe :
		MOV EAX, width
			ADD EAX, EDI
			MOV stripe_end, EAX // Set end pointer for this stripe
			next_stripe_column :
		MOV ECX, [EDI]
			TEST ECX, ECX
			JNZ row0
			speedup_loop :
		// Invoke speedup trick to skip over runs of all-zero neighbourhoods
		MOV ECX, [EDI + 12]
			ADD EDI, 12
			TEST ECX, ECX
			JZ speedup_loop
			SUB EDI, 8
			CMP EDI, stripe_end
			JNZ next_stripe_column
			JMP advance_stripe
			row0 :
		// Test for coding pass membership
		MOV EBX, ECX
			AND EBX, __NBRHD_MASK * 1
			JZ row1
			TEST ECX, __SIG_PROP_MEMBER_MASK * 1
			JNZ row1
			OR ECX, __PI_BIT * 1
			MOV[EDI], ECX // Save context word indicating that sample belongs to pass
			// Find pointer to the relevant context state record
			AND ECX, __NBRHD_MASK
			ADD ECX, sig_lut
			MOVZX ECX, byte ptr[ECX]
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX]
			// decode symbol
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE significance_test0
			CALL mq_non_cdp
			significance_test0 :
		MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX // If symbol != 0
			JZ row1
			// If we get here, we know that stripe row 0 is newly significant
			AND ECX, __CHI_BIT / 8 + __SIGMA_CC_BIT / 8 + __CHI_BIT * 8 + __SIGMA_CC_BIT * 8
			SHR ECX, 1
			MOV EAX, [EDI - 4] // Get context word on left
			AND EAX, __CHI_BIT + __SIGMA_CC_BIT
			SHR EAX, 2
			ADD ECX, EAX
			MOV EAX, [EDI + 4] // Get context word on right
			AND EAX, __CHI_BIT + __SIGMA_CC_BIT
			ADD ECX, EAX
			MOV EAX, ECX
			SHR EAX, CHI_POS - 1 - SIGMA_CC_POS
			OR ECX, EAX
			AND ECX, 0x000000FF
			MOVZX ECX, byte ptr[sign_lut + ECX]
			MOV EAX, ECX // Save the sign LUT result in EAX
			SHR ECX, 1 // Get context label part of the LUT result
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX + 8 * KAPPA_SIGN_BASE] // ECX has pointer to context state
			MOV EBX, [ECX] // Get `p_bar_mps' to EBX
			XOR EAX, EBX
			AND EAX, 1 // Leaves EAX with the MPS sign value in its LSB.
			BTR EBX, 0 // Leaves `p_bar' in EBX
			SUB EDX, EBX
			JGE row0_broadcast
			CALL mq_non_cdp
			row0_broadcast : // Process newly significant sample; sign is in LSB of EAX
				  // Broadcast significance and sign to last row of previous stripe
		MOV EBX, is_causal
			TEST EBX, EBX
			JNZ row0_skip_non_causal
			MOV EBX, EDI
			SUB EBX, context_row_gap
			MOV ECX, [EBX - 4]
			OR ECX, __SIGMA_BR_BIT * 512
			MOV[EBX - 4], ECX
			MOV ECX, EAX
			SHL ECX, NEXT_CHI_POS
			OR ECX, [EBX]
			OR ECX, __SIGMA_BC_BIT * 512
			MOV[EBX], ECX
			MOV ECX, [EBX + 4]
			OR ECX, __SIGMA_BL_BIT * 512
			MOV[EBX + 4], ECX
			row0_skip_non_causal :
		// Broadcast significance to left and right neighbours
		MOV ECX, [EDI - 4]
			OR ECX, __SIGMA_CR_BIT * 1
			MOV[EDI - 4], ECX
			MOV ECX, [EDI + 4]
			OR ECX, __SIGMA_CL_BIT * 1
			MOV[EDI + 4], ECX
			// Recover and update context word, using ECX register
			MOV ECX, EAX
			SHL ECX, CHI_POS
			OR ECX, [EDI]
			OR ECX, __SIGMA_CC_BIT * 1
			MOV[EDI], ECX
			// Write newly significant sample value
			SHL EAX, 31 // Get sign bit of result into right position
			ADD EAX, one_point_five
			MOV EBX, cp_to_sp0
			MOV[EDI + EBX], EAX
			row1 :
		// Test for coding pass membership
		MOV EBX, ECX
			AND EBX, __NBRHD_MASK * 8
			JZ row2
			TEST ECX, __SIG_PROP_MEMBER_MASK * 8
			JNZ row2
			OR ECX, __PI_BIT * 8
			MOV[EDI], ECX // Save context word indicating that sample belongs to pass
			// Find pointer to the relevant context state record
			SHR ECX, 3
			AND ECX, __NBRHD_MASK
			ADD ECX, sig_lut
			MOVZX ECX, byte ptr[ECX]
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX]
			// Decode symbol
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE significance_test1
			CALL mq_non_cdp
			significance_test1 :
		MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX // If symbol != 0
			JZ row2
			// If we get here, we know that stripe row 1 is newly significant
			AND ECX, __CHI_BIT + __SIGMA_CC_BIT + __CHI_BIT * 64 + __SIGMA_CC_BIT * 64
			SHR ECX, 4
			MOV EAX, [EDI - 4] // Get context word on left
			AND EAX, __CHI_BIT * 8 + __SIGMA_CC_BIT * 8
			SHR EAX, 5
			ADD ECX, EAX
			MOV EAX, [EDI + 4] // Get context word on right
			AND EAX, __CHI_BIT * 8 + __SIGMA_CC_BIT * 8
			SHR EAX, 3
			ADD ECX, EAX
			MOV EAX, ECX
			SHR EAX, CHI_POS - 1 - SIGMA_CC_POS
			OR ECX, EAX
			AND ECX, 0x000000FF
			MOVZX ECX, byte ptr[sign_lut + ECX]
			MOV EAX, ECX // Save the sign LUT result in EAX
			SHR ECX, 1 // Get context label part of the LUT result
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX + 8 * KAPPA_SIGN_BASE] // ECX has pointer to context state
			MOV EBX, [ECX] // Get `p_bar_mps' to EBX
			XOR EAX, EBX
			AND EAX, 1 // Leaves EAX with the MPS sign value in its LSB.
			BTR EBX, 0 // Leaves `p_bar' in EBX
			SUB EDX, EBX
			JGE row1_broadcast
			CALL mq_non_cdp
			row1_broadcast : // Process newly significant sample; sign is in LSB of EAX
				  // Broadcast significance to left and right neighbours
		MOV ECX, [EDI - 4]
			OR ECX, __SIGMA_CR_BIT * 8
			MOV[EDI - 4], ECX
			MOV ECX, [EDI + 4]
			OR ECX, __SIGMA_CL_BIT * 8
			MOV[EDI + 4], ECX
			// Recover and update context word, using ECX register
			MOV ECX, EAX
			SHL ECX, CHI_POS + 3
			OR ECX, [EDI]
			OR ECX, __SIGMA_CC_BIT * 8
			MOV[EDI], ECX
			// Write new significant sample value
			SHL EAX, 31 // Get sign bit of result into right position
			ADD EAX, one_point_five
			MOV EBX, cp_to_sp1
			MOV[EDI + EBX], EAX
			row2 :
		// Test for coding pass membership
		MOV EBX, ECX
			AND EBX, __NBRHD_MASK * 64
			JZ row3
			TEST ECX, __SIG_PROP_MEMBER_MASK * 64
			JNZ row3
			OR ECX, __PI_BIT * 64
			MOV[EDI], ECX // Save context word indicating that sample belongs to pass
			// Find pointer to the relevant context state record
			SHR ECX, 6
			AND ECX, __NBRHD_MASK
			ADD ECX, sig_lut
			MOVZX ECX, byte ptr[ECX]
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX]
			// Decode symbol
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE significance_test2
			CALL mq_non_cdp
			significance_test2 :
		MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX // If symbol != 0
			JZ row3
			// If we get here, we know that stripe row 2 is newly significant
			AND ECX, __CHI_BIT * 8 + __SIGMA_CC_BIT * 8 + __CHI_BIT * 512 + __SIGMA_CC_BIT * 512
			SHR ECX, 7
			MOV EAX, [EDI - 4] // Get context word on left
			AND EAX, __CHI_BIT * 64 + __SIGMA_CC_BIT * 64
			SHR EAX, 8
			ADD ECX, EAX
			MOV EAX, [EDI + 4] // Get context word on right
			AND EAX, __CHI_BIT * 64 + __SIGMA_CC_BIT * 64
			SHR EAX, 6
			ADD ECX, EAX
			MOV EAX, ECX
			SHR EAX, CHI_POS - 1 - SIGMA_CC_POS
			OR ECX, EAX
			AND ECX, 0x000000FF
			MOVZX ECX, byte ptr[sign_lut + ECX]
			MOV EAX, ECX // Save the sign LUT result in EAX
			SHR ECX, 1 // Get context label part of the LUT result
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX + 8 * KAPPA_SIGN_BASE] // ECX has pointer to context state
			MOV EBX, [ECX] // Get `p_bar_mps' to EBX
			XOR EAX, EBX
			AND EAX, 1 // Leaves EAX with the MPS sign value in its LSB.
			BTR EBX, 0 // Leaves `p_bar' in EBX
			SUB EDX, EBX
			JGE row2_broadcast
			CALL mq_non_cdp
			row2_broadcast : // Process newly significant sample; sign is in LSB of EAX
				  // Broadcast significance to left and right neighbours
		MOV ECX, [EDI - 4]
			OR ECX, __SIGMA_CR_BIT * 64
			MOV[EDI - 4], ECX
			MOV ECX, [EDI + 4]
			OR ECX, __SIGMA_CL_BIT * 64
			MOV[EDI + 4], ECX
			// Recover and update context word, using ECX register
			MOV ECX, EAX
			SHL ECX, CHI_POS + 6
			OR ECX, [EDI]
			OR ECX, __SIGMA_CC_BIT * 64
			MOV[EDI], ECX
			// Write new significant sample value
			SHL EAX, 31 // Get sign bit of result into right position
			ADD EAX, one_point_five
			MOV EBX, cp_to_sp2
			MOV[EDI + EBX], EAX
			row3 :
		// Test for coding pass membership
		MOV EBX, ECX
			AND EBX, __NBRHD_MASK * 512
			JZ advance_stripe_column
			TEST ECX, __SIG_PROP_MEMBER_MASK * 512
			JNZ advance_stripe_column
			OR ECX, __PI_BIT * 512
			MOV[EDI], ECX // Save context word indicating that sample belongs to pass
			// Find pointer to the relevant context state record
			SHR ECX, 9
			AND ECX, __NBRHD_MASK
			ADD ECX, sig_lut
			MOVZX ECX, byte ptr[ECX]
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX]
			// Decode symbol
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE significance_test3
			CALL mq_non_cdp
			significance_test3 :
		MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX // If symbol != 0
			JZ advance_stripe_column
			// If we get here, we know that stripe row 3 is newly significant
			MOV EBX, ECX // Make copy of ECX for exceptional sign processing
			AND ECX, __CHI_BIT * 64 + __SIGMA_CC_BIT * 64 + __SIGMA_CC_BIT * 4096
			SHR ECX, 10
			TEST EBX, EBX
			JGE no_exceptional_sign_processing
			OR ECX, __CHI_BIT * 4
			no_exceptional_sign_processing:
		MOV EAX, [EDI - 4] // Get context word on left
			AND EAX, __CHI_BIT * 512 + __SIGMA_CC_BIT * 512
			SHR EAX, 11
			ADD ECX, EAX
			MOV EAX, [EDI + 4] // Get context word on right
			AND EAX, __CHI_BIT * 512 + __SIGMA_CC_BIT * 512
			SHR EAX, 9
			ADD ECX, EAX
			MOV EAX, ECX
			SHR EAX, CHI_POS - 1 - SIGMA_CC_POS
			OR ECX, EAX
			AND ECX, 0x000000FF
			MOVZX ECX, byte ptr[sign_lut + ECX]
			MOV EAX, ECX // Save the sign LUT result in EAX
			SHR ECX, 1 // Get context label part of the LUT result
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX + 8 * KAPPA_SIGN_BASE] // ECX has pointer to context state
			MOV EBX, [ECX] // Get `p_bar_mps' to EBX
			XOR EAX, EBX
			AND EAX, 1 // Leaves EAX with the MPS sign value in its LSB.
			BTR EBX, 0 // Leaves `p_bar' in EBX
			SUB EDX, EBX
			JGE row3_broadcast
			CALL mq_non_cdp
			row3_broadcast : // Process newly significant sample; sign is in LSB of EAX
				  // Broadcast significance and sign to first row of next stripe
		MOV EBX, EDI
			ADD EBX, context_row_gap
			MOV ECX, [EBX - 4]
			OR ECX, __SIGMA_TR_BIT
			MOV[EBX - 4], ECX
			MOV ECX, EAX
			SHL ECX, PREV_CHI_POS
			OR ECX, [EBX]
			OR ECX, __SIGMA_TC_BIT
			MOV[EBX], ECX
			MOV ECX, [EBX + 4]
			OR ECX, __SIGMA_TL_BIT
			MOV[EBX + 4], ECX
			// Broadcast significance to left and right neighbours
			MOV ECX, [EDI - 4]
			OR ECX, __SIGMA_CR_BIT * 512
			MOV[EDI - 4], ECX
			MOV ECX, [EDI + 4]
			OR ECX, __SIGMA_CL_BIT * 512
			MOV[EDI + 4], ECX
			// Recover and update context word, using ECX register
			MOV ECX, EAX
			SHL ECX, CHI_POS + 9
			OR ECX, [EDI]
			OR ECX, __SIGMA_CC_BIT * 512
			MOV[EDI], ECX
			// Write new significant sample value
			SHL EAX, 31 // Get sign bit of result into right position
			ADD EAX, one_point_five
			MOV EBX, cp_to_sp3
			MOV[EDI + EBX], EAX
			advance_stripe_column :
		ADD EDI, 4
			CMP EDI, stripe_end
			JNZ next_stripe_column
			advance_stripe :
		ADD EDI, EXTRA_DECODE_CWORDS * 4
			MOV EAX, cp_to_sp_adjust
			MOV EBX, cp_to_sp0
			ADD EBX, EAX
			MOV cp_to_sp0, EBX
			MOV EBX, cp_to_sp1
			ADD EBX, EAX
			MOV cp_to_sp1, EBX
			MOV EBX, cp_to_sp2
			ADD EBX, EAX
			MOV cp_to_sp2, EBX
			MOV EBX, cp_to_sp3
			ADD EBX, EAX
			MOV cp_to_sp3, EBX
			MOV EAX, num_stripes
			SUB EAX, 1
			MOV num_stripes, EAX
			JNZ next_stripe
			JMP finished
			// We insert local fast function calls here
			mq_non_cdp :
		ASM_MQ_NON_CDP_DECODE
			ret // Return from fast function call
			finished :
		MOV D_var, EDX // Save the D register ready for `coder.check_in'
			MOV A_var, ESI // Save the A register ready for `coder.check_in'
			EMMS // Restore the FPU registers from any MMX calls which we may have used
	}

	coder.check_in(A_var, C_var, D_var, t_var, temp_var, store_var, S_var);
}

/*****************************************************************************/
/* STATIC                     asm_decode_mag_ref_pass                        */
/*****************************************************************************/

static void
asm_decode_mag_ref_pass(mq_decoder &coder, mqd_state states[],
	int p, bool causal, kdu_int32 *samples,
	kdu_int32 *contexts, int width, int num_stripes,
	int context_row_gap)
{
	if ((num_stripes <= 0) || (width <= 0))
		return;
	kdu_int32 A_var, C_var, D_var, t_var, temp_var;
	kdu_byte *store_var;
	int S_var;
	coder.check_out(A_var, C_var, D_var, t_var, temp_var, store_var, S_var);
	kdu_int32 half_lsb = (1 << p) >> 1;
	context_row_gap <<= 2; // Convert to number of bytes
	width <<= 2; // Convert to number of bytes
	  /* The following offsets are used to convert a context word pointer
		 to a sample pointer.  The offsets are correct for the first
		 stripe and must be adjusted from stripe to stripe by the addition
		 of 3*width-EXTRA_DECODE_CWORDS. */
	int cp_to_sp0 = (int)((samples - contexts) << 2); // Offset for row 0
	int cp_to_sp1 = cp_to_sp0 + width; // Offset for row 1
	int cp_to_sp2 = cp_to_sp1 + width; // Offset for row 2
	int cp_to_sp3 = cp_to_sp2 + width; // Offset for row 3
	int cp_to_sp_adjust = 3 * width - 4 * EXTRA_DECODE_CWORDS;
	kdu_int32 *stripe_end; // Points to context word beyond end of current stripe
	kdu_int32 mag_mask = -1; mag_mask <<= (p + 2); mag_mask &= KDU_INT32_MAX;
	states += KAPPA_MAG_BASE;
	mqd_state *states_2 = states + 2;

	__asm {
		MOV EDI, contexts // Set EDI aside to hold the context word pointer
		MOV EDX, D_var // Set EDX aside to hold the critical MQ variable, D
		MOV ESI, A_var // Set ESI aside to hold the MQ coder's A register
		next_stripe :
		MOV EAX, width
			ADD EAX, EDI
			MOV stripe_end, EAX // Set end pointer for this stripe
			next_stripe_column :
		MOV ECX, [EDI]
			TEST ECX, __MU_BIT + __MU_BIT * 8 + __MU_BIT * 64 + __MU_BIT * 512
			JNZ row0
			speedup_loop :
		// Invoke speedup trick to skip over runs of all-zero neighbourhoods
		MOV ECX, [EDI + 8]
			ADD EDI, 8
			TEST ECX, ECX
			JZ speedup_loop
			SUB EDI, 4
			CMP EDI, stripe_end
			JNZ next_stripe_column
			JMP advance_stripe
			row0 :
		// Test for coding pass membership
		TEST ECX, __MU_BIT * 1
			JZ row1
			// Find pointer to the relevant context state record
			MOV EBX, cp_to_sp0
			MOV EAX, [EDI + EBX]
			TEST EAX, mag_mask
			JZ row0_first_mag_ref
			MOV ECX, states_2
			JMP row0_decoding
			row0_first_mag_ref :
		TEST ECX, __NBRHD_MASK * 1
			MOV ECX, states
			JZ row0_decoding
			ADD ECX, 8
			row0_decoding : // Now we are ready to decode the symbol; ECX points to context
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE row0_decoded
			CALL mq_non_cdp
			MOV EBX, cp_to_sp0 // Reload the sample offset
			row0_decoded : // Ready to update sample; EBX holds address offset from EDI
		MOV CL, byte ptr[p]
			XOR EAX, 1
			SHL EAX, CL
			XOR EAX, [EDI + EBX]
			OR EAX, half_lsb
			MOV[EDI + EBX], EAX
			MOV ECX, [EDI]
			row1:
		// Test for coding pass membership
		TEST ECX, __MU_BIT * 8
			JZ row2
			// Find pointer to the relevant context state record
			MOV EBX, cp_to_sp1
			MOV EAX, [EDI + EBX]
			TEST EAX, mag_mask
			JZ row1_first_mag_ref
			MOV ECX, states_2
			JMP row1_decoding
			row1_first_mag_ref :
		TEST ECX, __NBRHD_MASK * 8
			MOV ECX, states
			JZ row1_decoding
			ADD ECX, 8
			row1_decoding : // Now we are ready to decode the symbol; ECX points to context
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE row1_decoded
			CALL mq_non_cdp
			MOV EBX, cp_to_sp1 // Reload the sample offset
			row1_decoded : // Ready to update sample; EBX holds address offset from EDI
		MOV CL, byte ptr[p]
			XOR EAX, 1
			SHL EAX, CL
			XOR EAX, [EDI + EBX]
			OR EAX, half_lsb
			MOV[EDI + EBX], EAX
			MOV ECX, [EDI]
			row2:
		// Test for coding pass membership
		TEST ECX, __MU_BIT * 64
			JZ row3
			// Find pointer to the relevant context state record
			MOV EBX, cp_to_sp2
			MOV EAX, [EDI + EBX]
			TEST EAX, mag_mask
			JZ row2_first_mag_ref
			MOV ECX, states_2
			JMP row2_decoding
			row2_first_mag_ref :
		TEST ECX, __NBRHD_MASK * 64
			MOV ECX, states
			JZ row2_decoding
			ADD ECX, 8
			row2_decoding : // Now we are ready to decode the symbol; ECX points to context
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE row2_decoded
			CALL mq_non_cdp
			MOV EBX, cp_to_sp2 // Reload the sample offset
			row2_decoded : // Ready to update sample; EBX holds address offset from EDI
		MOV CL, byte ptr[p]
			XOR EAX, 1
			SHL EAX, CL
			XOR EAX, [EDI + EBX]
			OR EAX, half_lsb
			MOV[EDI + EBX], EAX
			MOV ECX, [EDI]
			row3:
		// Test for coding pass membership
		TEST ECX, __MU_BIT * 512
			JZ advance_stripe_column
			// Find pointer to the relevant context state record
			MOV EBX, cp_to_sp3
			MOV EAX, [EDI + EBX]
			TEST EAX, mag_mask
			JZ row3_first_mag_ref
			MOV ECX, states_2
			JMP row3_decoding
			row3_first_mag_ref :
		TEST ECX, __NBRHD_MASK * 512
			MOV ECX, states
			JZ row3_decoding
			ADD ECX, 8
			row3_decoding : // Now we are ready to decode the symbol; ECX points to context
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE row3_decoded
			CALL mq_non_cdp
			MOV EBX, cp_to_sp3 // Reload the sample offset
			row3_decoded : // Ready to update sample; EBX holds address offset from EDI
		MOV CL, byte ptr[p]
			XOR EAX, 1
			SHL EAX, CL
			XOR EAX, [EDI + EBX]
			OR EAX, half_lsb
			MOV[EDI + EBX], EAX
			advance_stripe_column :
		ADD EDI, 4
			CMP EDI, stripe_end
			JNZ next_stripe_column
			advance_stripe :
		ADD EDI, EXTRA_DECODE_CWORDS * 4
			MOV EAX, cp_to_sp_adjust
			MOV EBX, cp_to_sp0
			ADD EBX, EAX
			MOV cp_to_sp0, EBX
			MOV EBX, cp_to_sp1
			ADD EBX, EAX
			MOV cp_to_sp1, EBX
			MOV EBX, cp_to_sp2
			ADD EBX, EAX
			MOV cp_to_sp2, EBX
			MOV EBX, cp_to_sp3
			ADD EBX, EAX
			MOV cp_to_sp3, EBX
			MOV EAX, num_stripes
			SUB EAX, 1
			MOV num_stripes, EAX
			JNZ next_stripe
			JMP finished
			// We insert local fast function calls here
			mq_non_cdp :
		ASM_MQ_NON_CDP_DECODE
			ret // Return from fast function call
			finished :
		MOV D_var, EDX // Save the D register ready for `coder.check_in'
			MOV A_var, ESI // Save the A register ready for `coder.check_in'
			EMMS // Restore the FPU registers from any MMX calls which we may have used
	}

	coder.check_in(A_var, C_var, D_var, t_var, temp_var, store_var, S_var);
}

/*****************************************************************************/
/* STATIC                    asm_decode_cleanup_pass                         */
/*****************************************************************************/

static void
asm_decode_cleanup_pass(mq_decoder &coder, mqd_state states[],
	int p, bool causal, int orientation,
	kdu_int32 *samples, kdu_int32 *contexts,
	int width, int num_stripes, int context_row_gap)
{
	if ((num_stripes <= 0) || (width <= 0))
		return;
	kdu_int32 A_var, C_var, D_var, t_var, temp_var;
	kdu_byte *store_var;
	int S_var;
	coder.check_out(A_var, C_var, D_var, t_var, temp_var, store_var, S_var);
	kdu_byte *sig_lut = significance_luts[orientation];
	kdu_int32 one_point_five = 1 << p; one_point_five += (one_point_five >> 1);
	int is_causal = (causal) ? 1 : 0;
	context_row_gap <<= 2; // Convert to number of bytes
	width <<= 2; // Convert to number of bytes
	  /* The following offsets are used to convert a context word pointer
		 to a sample pointer.  The offsets are correct for the first
		 stripe and must be adjusted from stripe to stripe by the addition
		 of 3*width-EXTRA_DECODE_CWORDS. */
	int cp_to_sp0 = (int)((samples - contexts) << 2); // Offset for row 0
	int cp_to_sp1 = cp_to_sp0 + width; // Offset for row 1
	int cp_to_sp2 = cp_to_sp1 + width; // Offset for row 2
	int cp_to_sp3 = cp_to_sp2 + width; // Offset for row 3
	int cp_to_sp_adjust = 3 * width - 4 * EXTRA_DECODE_CWORDS;
	kdu_int32 *stripe_end; // Points to context word beyond end of current stripe

	__asm {
		MOV EDI, contexts // Set EDI aside to hold the context word pointer
		MOV EDX, D_var // Set EDX aside to hold the critical MQ variable, D
		MOV ESI, A_var // Set ESI aside to hold the MQ coder's A register
		next_stripe :
		MOV EAX, width
			ADD EAX, EDI
			MOV stripe_end, EAX // Set end pointer for this stripe
			next_stripe_column :
		MOV ECX, [EDI]
			TEST ECX, ECX
			JNZ row0
			// Enter the run mode.
			MOV ECX, states
			ADD ECX, KAPPA_RUN_BASE * 8 // Gets address of run context to ECX
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			TEST EAX, 1
			JNZ decode_single_symbol_with_mps1
			// Try to skip over 4 columns at once
			MOV EBX, [EDI + 12]
			TEST EBX, EBX
			JNZ decode_single_symbol_with_mps0
			SHL EAX, 2
			SUB EDX, EAX
			JL multi_symbol_decode_failed
			ADD EDI, 16
			CMP EDI, stripe_end
			JNZ next_stripe_column
			JMP advance_stripe
			multi_symbol_decode_failed :
		ADD EDX, EAX
			SHR EAX, 2
			decode_single_symbol_with_mps0 :
			SUB EDX, EAX
			JGE uninterrupted_run // MPS decoded and it is 0
			CALL mq_non_cdp
			TEST EAX, 1
			JNZ run_interruption
			uninterrupted_run :
		ADD EDI, 4
			CMP EDI, stripe_end
			JNZ next_stripe_column
			JMP advance_stripe
			decode_single_symbol_with_mps1 :
		SUB EDX, EAX
			ADD EDX, 1 // Compensate for the non-zero MPS bit in EAX
			JGE run_interruption // MPS decoded but it is 1
			CALL mq_non_cdp
			TEST EAX, 1
			JZ uninterrupted_run
			run_interruption :
		ASM_MQ_DECODE_RUN  // Returns with the run in the 2 LSB's of EAX
			MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX
			JZ row0_significant
			JP row3_significant // Parity means there are an even number of 1's (run=3)
			TEST EAX, 1
			JNZ row1_significant
			JMP row2_significant
			row0 :
		// Test for coding pass membership
		TEST ECX, __CLEANUP_MEMBER_MASK * 1
			JNZ row1
			// Find pointer to the relevant context state record
			AND ECX, __NBRHD_MASK
			ADD ECX, sig_lut
			MOVZX ECX, byte ptr[ECX]
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX]
			// decode symbol
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE significance_test0
			CALL mq_non_cdp
			significance_test0 :
		MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX // If symbol != 0
			JZ row1
			row0_significant :
		// If we get here, we know that stripe row 0 is newly significant
		AND ECX, __CHI_BIT / 8 + __SIGMA_CC_BIT / 8 + __CHI_BIT * 8 + __SIGMA_CC_BIT * 8
			SHR ECX, 1
			MOV EAX, [EDI - 4] // Get context word on left
			AND EAX, __CHI_BIT + __SIGMA_CC_BIT
			SHR EAX, 2
			ADD ECX, EAX
			MOV EAX, [EDI + 4] // Get context word on right
			AND EAX, __CHI_BIT + __SIGMA_CC_BIT
			ADD ECX, EAX
			MOV EAX, ECX
			SHR EAX, CHI_POS - 1 - SIGMA_CC_POS
			OR ECX, EAX
			AND ECX, 0x000000FF
			MOVZX ECX, byte ptr[sign_lut + ECX]
			MOV EAX, ECX // Save the sign LUT result in EAX
			SHR ECX, 1 // Get context label part of the LUT result
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX + 8 * KAPPA_SIGN_BASE] // ECX has pointer to context state
			MOV EBX, [ECX] // Get `p_bar_mps' to EBX
			XOR EAX, EBX
			AND EAX, 1 // Leaves EAX with the MPS sign value in its LSB.
			BTR EBX, 0 // Leaves `p_bar' in EBX
			SUB EDX, EBX
			JGE row0_broadcast
			CALL mq_non_cdp
			row0_broadcast : // Process newly significant sample; sign is in LSB of EAX
				  // Broadcast significance and sign to last row of previous stripe
		MOV EBX, is_causal
			TEST EBX, EBX
			JNZ row0_skip_non_causal
			MOV EBX, EDI
			SUB EBX, context_row_gap
			MOV ECX, [EBX - 4]
			OR ECX, __SIGMA_BR_BIT * 512
			MOV[EBX - 4], ECX
			MOV ECX, EAX
			SHL ECX, NEXT_CHI_POS
			OR ECX, [EBX]
			OR ECX, __SIGMA_BC_BIT * 512
			MOV[EBX], ECX
			MOV ECX, [EBX + 4]
			OR ECX, __SIGMA_BL_BIT * 512
			MOV[EBX + 4], ECX
			row0_skip_non_causal :
		// Broadcast significance to left and right neighbours
		MOV ECX, [EDI - 4]
			OR ECX, __SIGMA_CR_BIT * 1
			MOV[EDI - 4], ECX
			MOV ECX, [EDI + 4]
			OR ECX, __SIGMA_CL_BIT * 1
			MOV[EDI + 4], ECX
			// Recover and update context word, using ECX register
			MOV ECX, EAX
			SHL ECX, CHI_POS
			OR ECX, [EDI]
			OR ECX, __SIGMA_CC_BIT * 1
			MOV[EDI], ECX
			// Write newly significant sample value
			SHL EAX, 31 // Get sign bit of result into right position
			ADD EAX, one_point_five
			MOV EBX, cp_to_sp0
			MOV[EDI + EBX], EAX
			row1 :
		// Test for coding pass membership
		TEST ECX, __CLEANUP_MEMBER_MASK * 8
			JNZ row2
			// Find pointer to the relevant context state record
			SHR ECX, 3
			AND ECX, __NBRHD_MASK
			ADD ECX, sig_lut
			MOVZX ECX, byte ptr[ECX]
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX]
			// Decode symbol
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE significance_test1
			CALL mq_non_cdp
			significance_test1 :
		MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX // If symbol != 0
			JZ row2
			row1_significant :
		// If we get here, we know that stripe row 1 is newly significant
		AND ECX, __CHI_BIT + __SIGMA_CC_BIT + __CHI_BIT * 64 + __SIGMA_CC_BIT * 64
			SHR ECX, 4
			MOV EAX, [EDI - 4] // Get context word on left
			AND EAX, __CHI_BIT * 8 + __SIGMA_CC_BIT * 8
			SHR EAX, 5
			ADD ECX, EAX
			MOV EAX, [EDI + 4] // Get context word on right
			AND EAX, __CHI_BIT * 8 + __SIGMA_CC_BIT * 8
			SHR EAX, 3
			ADD ECX, EAX
			MOV EAX, ECX
			SHR EAX, CHI_POS - 1 - SIGMA_CC_POS
			OR ECX, EAX
			AND ECX, 0x000000FF
			MOVZX ECX, byte ptr[sign_lut + ECX]
			MOV EAX, ECX // Save the sign LUT result in EAX
			SHR ECX, 1 // Get context label part of the LUT result
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX + 8 * KAPPA_SIGN_BASE] // ECX has pointer to context state
			MOV EBX, [ECX] // Get `p_bar_mps' to EBX
			XOR EAX, EBX
			AND EAX, 1 // Leaves EAX with the MPS sign value in its LSB.
			BTR EBX, 0 // Leaves `p_bar' in EBX
			SUB EDX, EBX
			JGE row1_broadcast
			CALL mq_non_cdp
			row1_broadcast : // Process newly significant sample; sign is in LSB of EAX
				  // Broadcast significance to left and right neighbours
		MOV ECX, [EDI - 4]
			OR ECX, __SIGMA_CR_BIT * 8
			MOV[EDI - 4], ECX
			MOV ECX, [EDI + 4]
			OR ECX, __SIGMA_CL_BIT * 8
			MOV[EDI + 4], ECX
			// Recover and update context word, using ECX register
			MOV ECX, EAX
			SHL ECX, CHI_POS + 3
			OR ECX, [EDI]
			OR ECX, __SIGMA_CC_BIT * 8
			MOV[EDI], ECX
			// Write new significant sample value
			SHL EAX, 31 // Get sign bit of result into right position
			ADD EAX, one_point_five
			MOV EBX, cp_to_sp1
			MOV[EDI + EBX], EAX
			row2 :
		// Test for coding pass membership
		TEST ECX, __CLEANUP_MEMBER_MASK * 64
			JNZ row3
			// Find pointer to the relevant context state record
			SHR ECX, 6
			AND ECX, __NBRHD_MASK
			ADD ECX, sig_lut
			MOVZX ECX, byte ptr[ECX]
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX]
			// Decode symbol
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE significance_test2
			CALL mq_non_cdp
			significance_test2 :
		MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX // If symbol != 0
			JZ row3
			row2_significant :
		// If we get here, we know that stripe row 2 is newly significant
		AND ECX, __CHI_BIT * 8 + __SIGMA_CC_BIT * 8 + __CHI_BIT * 512 + __SIGMA_CC_BIT * 512
			SHR ECX, 7
			MOV EAX, [EDI - 4] // Get context word on left
			AND EAX, __CHI_BIT * 64 + __SIGMA_CC_BIT * 64
			SHR EAX, 8
			ADD ECX, EAX
			MOV EAX, [EDI + 4] // Get context word on right
			AND EAX, __CHI_BIT * 64 + __SIGMA_CC_BIT * 64
			SHR EAX, 6
			ADD ECX, EAX
			MOV EAX, ECX
			SHR EAX, CHI_POS - 1 - SIGMA_CC_POS
			OR ECX, EAX
			AND ECX, 0x000000FF
			MOVZX ECX, byte ptr[sign_lut + ECX]
			MOV EAX, ECX // Save the sign LUT result in EAX
			SHR ECX, 1 // Get context label part of the LUT result
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX + 8 * KAPPA_SIGN_BASE] // ECX has pointer to context state
			MOV EBX, [ECX] // Get `p_bar_mps' to EBX
			XOR EAX, EBX
			AND EAX, 1 // Leaves EAX with the MPS sign value in its LSB.
			BTR EBX, 0 // Leaves `p_bar' in EBX
			SUB EDX, EBX
			JGE row2_broadcast
			CALL mq_non_cdp
			row2_broadcast : // Process newly significant sample; sign is in LSB of EAX
				  // Broadcast significance to left and right neighbours
		MOV ECX, [EDI - 4]
			OR ECX, __SIGMA_CR_BIT * 64
			MOV[EDI - 4], ECX
			MOV ECX, [EDI + 4]
			OR ECX, __SIGMA_CL_BIT * 64
			MOV[EDI + 4], ECX
			// Recover and update context word, using ECX register
			MOV ECX, EAX
			SHL ECX, CHI_POS + 6
			OR ECX, [EDI]
			OR ECX, __SIGMA_CC_BIT * 64
			MOV[EDI], ECX
			// Write new significant sample value
			SHL EAX, 31 // Get sign bit of result into right position
			ADD EAX, one_point_five
			MOV EBX, cp_to_sp2
			MOV[EDI + EBX], EAX
			row3 :
		// Test for coding pass membership
		TEST ECX, __CLEANUP_MEMBER_MASK * 512
			JNZ advance_stripe_column
			// Find pointer to the relevant context state record
			SHR ECX, 9
			AND ECX, __NBRHD_MASK
			ADD ECX, sig_lut
			MOVZX ECX, byte ptr[ECX]
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX]
			// Decode symbol
			MOV EAX, [ECX] // Get `p_bar_mps' to EAX
			SUB EDX, EAX
			AND EAX, 1 // Get MPS identity in LSB of EAX
			ADD EDX, EAX // Correct for the effect of the MPS
			JGE significance_test3
			CALL mq_non_cdp
			significance_test3 :
		MOV ECX, [EDI] // Get context word back again
			TEST EAX, EAX // If symbol != 0
			JZ advance_stripe_column
			row3_significant :
		// If we get here, we know that stripe row 3 is newly significant
		MOV EBX, ECX // Make copy of ECX for exceptional sign processing
			AND ECX, __CHI_BIT * 64 + __SIGMA_CC_BIT * 64 + __SIGMA_CC_BIT * 4096
			SHR ECX, 10
			TEST EBX, EBX
			JGE no_exceptional_sign_processing
			OR ECX, __CHI_BIT * 4
			no_exceptional_sign_processing:
		MOV EAX, [EDI - 4] // Get context word on left
			AND EAX, __CHI_BIT * 512 + __SIGMA_CC_BIT * 512
			SHR EAX, 11
			ADD ECX, EAX
			MOV EAX, [EDI + 4] // Get context word on right
			AND EAX, __CHI_BIT * 512 + __SIGMA_CC_BIT * 512
			SHR EAX, 9
			ADD ECX, EAX
			MOV EAX, ECX
			SHR EAX, CHI_POS - 1 - SIGMA_CC_POS
			OR ECX, EAX
			AND ECX, 0x000000FF
			MOVZX ECX, byte ptr[sign_lut + ECX]
			MOV EAX, ECX // Save the sign LUT result in EAX
			SHR ECX, 1 // Get context label part of the LUT result
			MOV EBX, states
			LEA ECX, [EBX + 8 * ECX + 8 * KAPPA_SIGN_BASE] // ECX has pointer to context state
			MOV EBX, [ECX] // Get `p_bar_mps' to EBX
			XOR EAX, EBX
			AND EAX, 1 // Leaves EAX with the MPS sign value in its LSB.
			BTR EBX, 0 // Leaves `p_bar' in EBX
			SUB EDX, EBX
			JGE row3_broadcast
			CALL mq_non_cdp
			row3_broadcast : // Process newly significant sample; sign is in LSB of EAX
				  // Broadcast significance and sign to first row of next stripe
		MOV EBX, EDI
			ADD EBX, context_row_gap
			MOV ECX, [EBX - 4]
			OR ECX, __SIGMA_TR_BIT
			MOV[EBX - 4], ECX
			MOV ECX, EAX
			SHL ECX, PREV_CHI_POS
			OR ECX, [EBX]
			OR ECX, __SIGMA_TC_BIT
			MOV[EBX], ECX
			MOV ECX, [EBX + 4]
			OR ECX, __SIGMA_TL_BIT
			MOV[EBX + 4], ECX
			// Broadcast significance to left and right neighbours
			MOV ECX, [EDI - 4]
			OR ECX, __SIGMA_CR_BIT * 512
			MOV[EDI - 4], ECX
			MOV ECX, [EDI + 4]
			OR ECX, __SIGMA_CL_BIT * 512
			MOV[EDI + 4], ECX
			// Recover and update context word, using ECX register
			MOV ECX, EAX
			SHL ECX, CHI_POS + 9
			OR ECX, [EDI]
			OR ECX, __SIGMA_CC_BIT * 512
			MOV[EDI], ECX
			// Write new significant sample value
			SHL EAX, 31 // Get sign bit of result into right position
			ADD EAX, one_point_five
			MOV EBX, cp_to_sp3
			MOV[EDI + EBX], EAX
			advance_stripe_column :
		MOV EBX, ECX
			SHL EBX, MU_POS - SIGMA_CC_POS
			AND EBX, __MU_BIT + __MU_BIT * 8 + __MU_BIT * 64 + __MU_BIT * 512
			OR ECX, EBX
			AND ECX, 0xFFFFFFFF - __PI_BIT - __PI_BIT * 8 - __PI_BIT * 64 - __PI_BIT * 512
			MOV[EDI], ECX
			ADD EDI, 4
			CMP EDI, stripe_end
			JNZ next_stripe_column
			advance_stripe :
		ADD EDI, EXTRA_DECODE_CWORDS * 4
			MOV EAX, cp_to_sp_adjust
			MOV EBX, cp_to_sp0
			ADD EBX, EAX
			MOV cp_to_sp0, EBX
			MOV EBX, cp_to_sp1
			ADD EBX, EAX
			MOV cp_to_sp1, EBX
			MOV EBX, cp_to_sp2
			ADD EBX, EAX
			MOV cp_to_sp2, EBX
			MOV EBX, cp_to_sp3
			ADD EBX, EAX
			MOV cp_to_sp3, EBX
			MOV EAX, num_stripes
			SUB EAX, 1
			MOV num_stripes, EAX
			JNZ next_stripe
			JMP finished
			// We insert local fast function calls here
			mq_non_cdp :
		ASM_MQ_NON_CDP_DECODE
			ret // Return from fast function call
			finished :
		MOV D_var, EDX // Save the D register ready for `coder.check_in'
			MOV A_var, ESI // Save the A register ready for `coder.check_in'
			EMMS // Restore the FPU registers from any MMX calls which we have used
	}

	coder.check_in(A_var, C_var, D_var, t_var, temp_var, store_var, S_var);
}

#endif // MSVC_BLOCK_DECODE_ASM_H
