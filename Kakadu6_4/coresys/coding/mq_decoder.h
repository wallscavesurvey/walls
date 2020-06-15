/*****************************************************************************/
// File: mq_decoder.h [scope = CORESYS/CODING]
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
   Defines interfaces for the low-level binary symbol operations associated
with the MQ decoder.  Includes, interfaces to support the arithetic coder bypass
mode as well.  Also includes macros which may be used as substitutes for the
decoding functions.  Both the macros and the functions achieve the same
result, although the macro implementation is more carefully tuned for speed,
while the function implementation is somewhat more dydactic.
******************************************************************************/

#ifndef MQ_DECODER_H
#define MQ_DECODER_H

#include <assert.h>
#include "kdu_messaging.h"

// Defined here:

struct mqd_state;
struct mqd_transition;
class mq_decoder;

/* ========================================================================= */
/*                                Constants                                  */
/* ========================================================================= */

#define MQD_A_MIN ((kdu_int32)(1<<23)) // Lower bound for A register.

/* ========================================================================= */
/*                      Class and Structure Definitions                      */
/* ========================================================================= */

/*****************************************************************************/
/*                                 mqd_state                                 */
/*****************************************************************************/

struct mqd_state {
public: // Member functions
	void init(int Sigma, kdu_int32 s); // Inline implementation appears later
	  /* `Sigma' is in the range 0 to 46 and `s' is the MPS identity. */
public: // Data
	kdu_int32 p_bar_mps; // Holds `p_bar' * 2^8 + `s' (`s' is the MPS: 0 or 1)
	mqd_transition *transition;
};
/* Notes:
	  This structure manages the state of the probability estimation state
   machine for a single coding context.  The representation is redundant,
   of course, since all that is required is the value of `Sigma', in the
   range 0 to 46, and the value of MPS, `s' (0 or 1).  However, this expanded
   representation avoids unnecessary de-referencing steps by the MQ
   decoder and so can significantly increase throughput.
	  The `transition' pointer holds the address of the entry in
   `mq_decoder::transition_table' which corresponds to this element. There
   are 92 entries in the transition table, whose indices have the form
   `idx'=2*`Sigma'+s.  If renormalization occurs while decoding an MPS,
   the state is updated according to state=state.transition->mps; if
   renormalization occurs while decoding an LPS, the state is updated
   according to state=state.transition->lps.  The transition table is
   carefully constructed to ensure that all information is mapped correctly
   by this simple operation. */

   /*****************************************************************************/
   /*                                mqd_transition                             */
   /*****************************************************************************/

struct mqd_transition {
	/* See the definition of `mqd_state' for an explanation of this structure. */
	mqd_state mps;
	mqd_state lps;
};

/*****************************************************************************/
/*                                  mq_decoder                               */
/*****************************************************************************/

class mq_decoder {
	/* This object can be used for both MQ and raw codeword segments. */
public: // Member functions
	mq_decoder()
	{
		active = false; buf_start = buf_next = NULL;
	}
	void start(kdu_byte *start, int segment_length, bool MQ_segment);
	/* Start decoding a new MQ or raw codeword segment.  On entry, `buffer'
	   points to the first byte of the segment and `segment_length' indicates
	   the total number of bytes in the segment.  Note that the buffer
	   must be long enough to accommodate 2 extra bytes beyond the stated
	   length.  During start up, the values of these 2 extra bytes are
	   stored internally and overwritten with a termination marker.  This
	   has the effect of halving the number of tests which must be performed
	   while consuming bytes. The original values of these two bytes are
	   restored by `finish' call. */
	bool finish(bool check_erterm = false);
	/* Each `start' call should be matched by a `finish' call.  The
	   function returns true, unless an error condition was detected.
	   There is no way to detect an error unless we can assume something
	   about the termination policy used by the encoder. When the predictable
	   termination policy (ERTERM mode switch) has been used by the encoder,
	   the `check_erterm' argument may be set to true and the function
	   will perform the relevant tests. */
public: // Functions to check out state information for use with fast macros
	void check_out(kdu_int32 &A, kdu_int32 &C, kdu_int32 &D,
		kdu_int32 &t, kdu_int32 &temp, kdu_byte * &store, int &S)
	{ // Use this form for MQ codeword segments.
		assert(active && (!checked_out) && MQ_segment); checked_out = true;
		A = this->A; C = this->C;
		D = A - MQD_A_MIN; D = (C < D) ? C : D; A -= D; C -= D;
		t = this->t; temp = this->temp; store = this->buf_next; S = this->S;
	}
	void check_out(kdu_int32 &t, kdu_int32 &temp, kdu_byte * &store)
	{ // Use this form for raw codeword segments.
		assert(active && (!checked_out) && !MQ_segment); checked_out = true;
		t = this->t; temp = this->temp; store = this->buf_next;
	}
	void check_in(kdu_int32 A, kdu_int32 C, kdu_int32 D,
		kdu_int32 t, kdu_int32 temp, kdu_byte *store, int S)
	{ // Use this form for MQ codeword segments.
		assert(active && checked_out && MQ_segment); checked_out = false;
		this->A = A + D; this->C = C + D;
		this->t = t; this->temp = temp; this->buf_next = store; this->S = S;
	}
	void check_in(kdu_int32 t, kdu_int32 temp, kdu_byte *store)
	{ // Use this form for raw codeword segments.
		assert(active && checked_out && !MQ_segment); checked_out = false;
		this->t = t; this->temp = temp; this->buf_next = store;
	}
public: // Encoding functions. Note: use macros for the highest throughput
	void mq_decode(kdu_int32 &symbol, mqd_state &state);
	void mq_decode_run(kdu_int32 &run); // Decodes 2 bit run length, MSB first
	void raw_decode(kdu_int32 &symbol);
private:
	void fill_lsbs();
	/* Used by the `mq_decode' member function. */
public: // Probability estimation state machine.
	static kdu_int32 p_bar_table[47]; // Normalized LPS probabilities
	static mqd_transition transition_table[94]; // See defn of `mqd_transition'
private: // Data
	kdu_int32 A; // The 8 MSB's and 8 LSB's of this word are guaranteed to be 0
	kdu_int32 C; // The 8 MSB's of this word are guaranteed to be 0
	kdu_int32 t;   // This is "t_bar" in the book
	kdu_int32 temp; // This is "T_bar" in the book
	kdu_byte *buf_start, *buf_next;
	int S; // Number of synthesized FF's
	bool checked_out;
	bool MQ_segment;
	bool active;
	int segment_length;
	kdu_byte overwritten_bytes[2];
};

/*****************************************************************************/
/* INLINE                       mqd_state::init                              */
/*****************************************************************************/

inline void
mqd_state::init(int Sigma, kdu_int32 s)
{
	assert((Sigma >= 0) && (Sigma <= 46) && (s == (s & 1)));
	p_bar_mps = (mq_decoder::p_bar_table[Sigma] << 8) + s;
	transition = mq_decoder::transition_table + ((Sigma << 1) + s);
}


/* ========================================================================= */
/*                           Fast Decoding Macros                            */
/* ========================================================================= */

/* Note: although in-lining is preferable, most compilers fail to realize the
   full speed potential of macros.  These are critical to the overall system
   throughput and also to the size of the code fragment which gets executed
   inside the coding pass loops. */

   /*****************************************************************************/
   /* MACRO                        _mqd_fill_lsbs_                              */
   /*****************************************************************************/

	 /* Implements the Fill-LSBs procedure described in Section 12.1.3 of the
		book by Taubman and Marcellin.  The implementation differs only in that
		we can be certain that the codeword segment will be terminated by a
		marker code in the range 0xFF90 through 0xFFFF.  This is because a
		synthetic marker is temporarily inserted at the end of the segment. */

#define _mqd_fill_lsbs_(C,t,temp,store,S)                                     \
  {                                                                           \
    t = 8;                                                                    \
    if (temp == 0xFF)                                                         \
      { /* Only need to check for termination inside here. */                 \
        temp = *(store++);                                                    \
        if (temp > 0x8F) /* Termination marker: remain here indefinitely. */  \
          { temp = 0xFF; store--; S++; }                                      \
        else                                                                  \
          { t = 7; C += temp; } /* This way, we add two copies of `temp' */   \
      }                                                                       \
    else                                                                      \
      temp = *(store++);                                                      \
    C += temp;                                                                \
  }

		/*****************************************************************************/
		/* MACRO                           _mq_decode_                               */
		/*****************************************************************************/

		  /* Implementation of the CDP (Common Decoding Path) optimized algorithm
			 suggested in Section 17.1.1 of the book by Taubman and Marcellin.
				The idea is that D accumulates the p_bar quantities associated with
			 consecutive CDP symbols, whose subtraction from both A and C is delayed
			 until the next non-CDP symbol occurs.  C_min holds the minimum of C and
			 (A-2^15), from the point when the last non-CDP symbol was coded.  Thus,
			 so long as D <= C_min, an MPS is decoded without renormalization (and
			 hence without conditional exchange either).
				On return, `symbol' holds either 1 or 0. */

				/* If your compiler and architecture support the allocation of registers
				   for critical variables, it is recommended that `symbol', `state',
				   `D', `A' and `C' be allocated registers, in that order. */

#define _mq_decode_(symbol,state,A,C,D,t,temp,store,S)                        \
  {                                                                           \
    symbol = (state).p_bar_mps;                                               \
    D -= symbol; symbol &= 1; D += symbol;                                    \
    if (D < 0)                                                                \
      { /* Non-CDP decoding follows. Renormalization is inevitable. */        \
        A += D; C += D;                                                       \
        D = (state).p_bar_mps - symbol;                                       \
        if (C >= 0) /* True if and only if C_active >= 0 */                   \
          { /* Upper sub-interval selected; must have A < A_min. */           \
            assert (A < MQD_A_MIN);                                           \
            if (A < D)                                                        \
              { /* Conditional exchange; LPS is decoded */                    \
                symbol = 1-symbol;                                            \
                state = (state).transition->lps;                              \
              }                                                               \
            else                                                              \
              { /* MPS is decoded */                                          \
                state = (state).transition->mps;                              \
              }                                                               \
          }                                                                   \
        else                                                                  \
          { /* Lower sub-interval selected */                                 \
            C += D; /* Put back p_bar */                                      \
            if (A < D)                                                        \
              { /* Conditional exchange; MPS is decoded */                    \
                state = (state).transition->mps;                              \
              }                                                               \
            else                                                              \
              { /* LPS is decoded. */                                         \
                symbol = 1-symbol;                                            \
                state = (state).transition->lps;                              \
              }                                                               \
            A = D;  /* Remeber that D is p_bar here. */                       \
          }                                                                   \
        assert(A < MQD_A_MIN); /* Need renormalization */                     \
        do {                                                                  \
            if (t == 0)                                                       \
              _mqd_fill_lsbs_(C,t,temp,store,S);                              \
            A += A; C += C; t--;                                              \
          } while (A < MQD_A_MIN);                                            \
        D = A-MQD_A_MIN;                                                      \
        if (C < D)                                                            \
          D = C;                                                              \
        A -= D; C -= D; /* We will add D back again at the next non-CDP. */   \
      }                                                                       \
  }

				   /*****************************************************************************/
				   /* MACRO                        _mq_decode_run_                              */
				   /*****************************************************************************/

					 /* Specialization of _mq_decode_ to the case where the state is equal to
						the special non-adaptive MQ coder state (last state in transition
						table); decodes two symbols to recover a 2-bit run length, with the
						first symbol forming the most significant bit of the run. */

#define _mq_decode_run_(run,A,C,D,t,temp,store,S)                             \
  {                                                                           \
    run = 0;                                                                  \
    D -= 0x00560100;                                                          \
    if (D < 0)                                                                \
      { /* Non-CDP decoding follows. Renormalization is inevitable. */        \
        A += D; C += D;                                                       \
        if (C >= 0) /* True if and only if C_active >= 0 */                   \
          { /* Upper sub-interval selected; must have A < A_min. */           \
            assert (A < MQD_A_MIN);                                           \
            if (A < 0x00560100)                                               \
              run = 2;                                                        \
          }                                                                   \
        else                                                                  \
          { /* Lower sub-interval selected */                                 \
            C += 0x00560100; /* Put back p_bar */                             \
            if (A >= 0x00560100)                                              \
              run = 2;                                                        \
            A = 0x00560100;                                                   \
          }                                                                   \
        assert(A < MQD_A_MIN); /* Need renormalization */                     \
        do {                                                                  \
            if (t == 0)                                                       \
              _mqd_fill_lsbs_(C,t,temp,store,S);                              \
            A += A; C += C; t--;                                              \
          } while (A < MQD_A_MIN);                                            \
        D = A-MQD_A_MIN;                                                      \
        if (C < D)                                                            \
          D = C;                                                              \
        A -= D; C -= D; /* We will add D back again at the next non-CDP. */   \
      }                                                                       \
    D -= 0x00560100;                                                          \
    if (D < 0)                                                                \
      { /* Non-CDP decoding follows. Renormalization is inevitable. */        \
        A += D; C += D;                                                       \
        if (C >= 0) /* True if and only if C_active >= 0 */                   \
          { /* Upper sub-interval selected; must have A < A_min. */           \
            assert (A < MQD_A_MIN);                                           \
            if (A < 0x00560100)                                               \
              run++;                                                          \
          }                                                                   \
        else                                                                  \
          { /* Lower sub-interval selected */                                 \
            C += 0x00560100; /* Put back p_bar */                             \
            if (A >= 0x00560100)                                              \
              run++;                                                          \
            A = 0x00560100;                                                   \
          }                                                                   \
        assert(A < MQD_A_MIN); /* Need renormalization */                     \
        do {                                                                  \
            if (t == 0)                                                       \
              _mqd_fill_lsbs_(C,t,temp,store,S);                              \
            A += A; C += C; t--;                                              \
          } while (A < MQD_A_MIN);                                            \
        D = A-MQD_A_MIN;                                                      \
        if (C < D)                                                            \
          D = C;                                                              \
        A -= D; C -= D; /* We will add D back again at the next non-CDP. */   \
      }                                                                       \
  }

						/*****************************************************************************/
						/* MACRO                          _raw_decode_                               */
						/*****************************************************************************/

#define _raw_decode_(symbol,t,temp,store)                                     \
  {                                                                           \
    if (t == 0)                                                               \
      {                                                                       \
        t = 8;                                                                \
        if (temp == 0xFF)                                                     \
          { /* Need to check for terminating marker. */                       \
            temp = *(store++);                                                \
            if (temp > 0x8F)                                                  \
              { temp = 0xFF; store--; }                                       \
            else                                                              \
              t = 7;                                                          \
          }                                                                   \
        else                                                                  \
          temp = *(store++);                                                  \
      }                                                                       \
    t--;                                                                      \
    symbol = (temp>>t) & 1;                                                   \
  }

#endif // MQ_DECODER_H
