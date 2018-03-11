/*****************************************************************************/
// File: mq_encoder.h [scope = CORESYS/CODING]
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
with the MQ coder.  Includes, interfaces to support the arithetic coder bypass
mode as well.  Also includes macros which may be used as substitutes for the
coding functions.  Both the macros and the functions achieve the same
result, although the macro implementation is more carefully tuned for speed,
while the function implementation is somewhat more dydactic.
******************************************************************************/

#ifndef MQ_ENCODER_H
#define MQ_ENCODER_H

#include <assert.h>
#include "kdu_messaging.h"

// Defined here:

struct mqe_state;
struct mqe_transition;
class mq_encoder;

/* ========================================================================= */
/*                                Constants                                  */
/* ========================================================================= */

#define MQE_SPACER 3

#define MQE_CARRY_POS (16+MQE_SPACER+8)
#define MQE_CARRY_BIT ((kdu_int32)(1<<MQE_CARRY_POS))

#define MQE_PARTIAL_POS (16+MQE_SPACER)
#define MQE_PARTIAL_CLEAR (~(((kdu_int32)(-1))<<MQE_PARTIAL_POS))

#define MQE_MSBS_POS (MQE_PARTIAL_POS+1)
#define MQE_MSBS_CLEAR (~(((kdu_int32)(-1))<<MQE_MSBS_POS))

#define MQE_A_MIN ((kdu_int32)(1<<15))


/* ========================================================================= */
/*                             Class Definitions                             */
/* ========================================================================= */

/*****************************************************************************/
/*                                 mqe_state                                 */
/*****************************************************************************/

struct mqe_state {
  public: // Member functions
    void init(int Sigma, kdu_int32 s); // Inline implementation appears later
      /* `Sigma' is in the range 0 to 46 and `s' is the MPS identity. */
  public: // Data
    kdu_int32 mps_p_bar; // Holds `p_bar' + `s'*2^31 (`s' is the MPS; 0 or 1)
    mqe_transition *transition;
  };
  /* Notes:
        This structure manages the state of the probability estimation state
     machine for a single coding context.  The representation is redundant,
     of course, since all that is required is the vaue of `Sigma', in the
     range 0 through 46, and the value of the MPS, `s' (0 or 1).  However,
     this expanded representation avoids unnecessary de-referencing steps
     by the MP encoder and so can significantly increase throughput.
        The `transition' pointer holds the address of the entry in
     `mq_encoder::transition_table' which corresponds to this element.  There
     are 92 entries in the transition table, whose indices have the form
     `idx'=2*`Sigma'+s.  If renormalization occurs while encoding an MPS,
     the state is updated according to state=state.transition->mps; if
     renormalization occurs while encoding an LPS, the state is updated
     according to state=state.transition->lps.  The transition table is
     carefully constructed to ensure that all information is mapped correctly
     by this simple operation.
        The trick of keeping the MPS identity in the most significant bit
     (i.e., the sign bit) of the `mps_p_bar' member, allows the encoder to
     be implemented with a most common symbol path having only one comparison
     and very few other operations. */

/*****************************************************************************/
/*                                mqe_transition                             */
/*****************************************************************************/

struct mqe_transition {
  /* See the definition of `mqe_state' for an explanation of this structure. */
    mqe_state mps;
    mqe_state lps;
  };

/*****************************************************************************/
/*                                  mq_encoder                               */
/*****************************************************************************/

class mq_encoder {
  /* This object can be used for both MQ and raw codeword segments. */
  public: // Member functions
    mq_encoder()
      { active = false; buf_start=buf_next=NULL; prev=next=NULL; }
    void start(kdu_byte *buffer, bool MQ_segment);
      /* Start a new MQ or raw codeword segment.  On entry, `buffer' points
         to the location to which the first code byte will be written.  Note
         carefully, however, that the MQ codeword segments actually require
         access to the location immediately prior to the `buffer' pointer.
         The value at that location is temporarily saved and restored only when
         the `terminate' member function is called.  To allow access to
         `buffer'[-1], the caller should take care in allocating storage
         for coded bytes.
            The buffer is required to be large enough to hold the compressed
         output.  If the user determines that the buffer may need to grow,
         the `augment_buffer' function should be used in the manner described
         below.  This policy allows buffer size tests and potential function
         calls to be eliminated from all time-critical code. */
    int get_bytes_used()
      { /* Returns the total number of bytes from the original `buffer'
           supplied in the `start' call which have been used by this object
           and any previous objects which it continues.  The function is
           provided only to facilitate determination by the user as to whether
           or not buffer space will need to be augmented.  There is no
           attempt to determine truncation lengths. */
        assert((!checked_out) && (buf_start != NULL));
        return (int)(buf_next-buf_start);
      }
    void augment_buffer(kdu_byte *old_handle, kdu_byte *new_handle)
      { /* Use this if you need to increase the size of the buffer.  You must
           first create the new buffer and copy the old buffer's contents into
           the new buffer.  Then call this function for every `mqe_coder'
           object which is using the buffer. The object will alter its internal
           pointers accordingly.   The `old_handle' and `new_handle' pointers
           should generally point to the first allocated location in each of
           the original and new buffers. */
        if (buf_start == NULL)
          return; // Encoder finished working with buffer
        assert((!checked_out) && (old_handle != NULL) && (new_handle != NULL));
        buf_start += (new_handle-old_handle);
        buf_next += (new_handle-old_handle);
      }
    void continues(mq_encoder *previous)
      { /* Use this function to continue a coding segment beyond the current
           coding pass.  This sets up internal references between the
           coding objects which manage the same codeword segment, so that
           truncation point computation may be performed later. */
        assert((!active) && (buf_start == NULL));
        assert(previous->active); // Can't continue an inactive segment.
        assert(!previous->checked_out);
        *this = *previous;
        prev = previous; previous->next = this;
        previous->active = false;
      }
    int get_incremental_length(bool &final)
      { /* This function returns the number of additional code bytes generated
           by this object, beyond the point of continuation from any
           previous object in a continuation list.  If possible, the function
           invokes the truncation point optimization algorithm for this
           object and all objects which it continues and have not already
           had their truncation points determined.  This will not be possible
           if the object is still active or it has been continued by another
           object which has generated less than 5 additional bytes so far.
           If truncation point optimization is not possible, the `final'
           argument is set to false and the function returns a reasonable
           lower bound (not quite a guaranteed bound) for the length, which
           is the number of bytes which have actually been put onto the
           byte buffer beyond the point of continuation (or the start of the
           buffer).  The return value is guaranteed to be non-negative. */
        if (!truncation_point_found)
          {
            mq_encoder *scan_ahead = this;
            while (scan_ahead->next != NULL) scan_ahead = scan_ahead->next;
            mq_encoder *scan_back = scan_ahead;
            for (; scan_back != NULL; scan_back=scan_back->prev)
              {
                if (scan_back->truncation_point_found)
                  break;
                if ((!scan_back->active) &&
                    ((scan_ahead->buf_next - scan_back->buf_next) >= 5))
                  scan_back->find_truncation_point(scan_ahead->buf_next);
              }
          }
        final = truncation_point_found;
        if (prev == NULL)
          return (int)(buf_next - buf_start);
        else
          return (int)(buf_next - prev->buf_next);
      }
    kdu_byte *terminate(bool optimal);
      /* This function is quite involved.  It flushes the state of the coder
         and then determines a suitable truncation length.  If `optimal' is
         true, it terminates to the smallest possible codeword segment which
         will result in correct decoding of the coded symbols.
            If the codeword segment has been continued across multiple coding
         objects, this function may be invoked only on the last such object.
         It automatically visits the other objects and forces optimal
         truncation length computation if this has not already been done
         (see "get_incremental_length" for a discussion of early
         length computation).  This guarantees that their `finish' functions
         will return correct lengths.
            For a thorough discussion of optimal termination and length
         computation, see Section 12.3.2 of the book by Taubman and Marcellin.
            If `optimal' is false, the function uses the predictable
         termination algorithms expected by error resilient decoders. See
         Section 12.4 of the book by Taubman and Marcellin for a discussion
         of error resilient termination for MQ and raw codeword segments.
            The function returns a pointer to the next free location in
         the byte buffer; this may be used to start another codeword
         segment. */
    void finish()
      { /* Invoke this function on the last element in a codeword segment
           (the same one which you invoked `terminate' on) to reset all
           objects in the segment in preparation for later calls to the
           `start' member function. */
        assert((!active) && (next == NULL));
        mq_encoder *scan, *next_scan;
        for (scan=this; scan != NULL; scan = next_scan)
          {
            assert(!scan->active);
            next_scan = scan->prev;
            scan->next = scan->prev = NULL;
            scan->truncation_point_found = false;
            scan->buf_start = scan->buf_next = NULL;
          }
      }
  public: // Functions to check out state information for use with fast macros
    void check_out(kdu_int32 &A, kdu_int32 &C,
                   kdu_int32 &t, kdu_int32 &temp, kdu_byte * &store)
      { // Use this form for MQ codeword segments.
        assert(active && (!checked_out) && MQ_segment); checked_out = true;
        A = this->A; C = this->C;
        t = this->t; temp = this->temp; store = this->buf_next;
      }
    void check_out(kdu_int32 &t, kdu_int32 &temp, kdu_byte * &store)
      { // Use this form for raw codeword segments.
        assert(active && (!checked_out) && !MQ_segment); checked_out = true;
        t = this->t; temp = this->temp; store = this->buf_next;
      }
    void check_in(kdu_int32 A, kdu_int32 C,
                  kdu_int32 t, kdu_int32 temp, kdu_byte *store)
      { // Use this form for MQ codeword segments.
        assert(active && checked_out && MQ_segment); checked_out = false;
        this->A = A; this->C = C;
        this->t = t; this->temp = temp; this->buf_next = store;
      }
    void check_in(kdu_int32 t, kdu_int32 temp, kdu_byte *store)
      { // Use this form for raw codeword segments.
        assert(active && checked_out && !MQ_segment); checked_out = false;
        this->t = t; this->temp = temp; this->buf_next = store;
      }
  public: // Encoding functions. Note: use macros for the highest throughput
    void mq_encode(kdu_int32 symbol, mqe_state &state);
      /* Note that symbol must be 0 or KDU_INT32_MIN */
    void mq_encode_run(kdu_int32 run); // Encodes a 2 bit run length, MSB first
    void raw_encode(kdu_int32 symbol);
      /* Note that symbol must be 0 or 1. */
  private:
    void transfer_byte();
      /* Used by the `mq_encode' member function. */
  public: // Probability estimation state machine.
    static kdu_int32 p_bar_table[47]; // Normalized LPS probabilities
    static mqe_transition transition_table[94]; // See def'n of `mqe_transition'
  private: // Internal functions
    void find_truncation_point(kdu_byte *limit);
      /* This function is automatically invoked from within `terminate' to
         determine optimal truncation points for continued codeword segments
         (it may also be applied to non-continued segments to find the
         minimal segment length).  The function may also be invoked from within
         the `get_final_length' member function.
            The `limit' argument points to the location in the byte buffer
         which immediately follows the last byte in the already terminated
         codeword segment.  Legal access is permitted only to bytes preceding
         this location. */
  private: // Data
    kdu_int32 A; // Only the least significant 16 bits of A are used
    kdu_int32 C; // Holds a 28 bit number (including the carry bit)
    kdu_int32 t;   // This is "t_bar" in the book
    kdu_int32 temp; // This is "T_bar" in the book
    kdu_byte *buf_start, *buf_next;
    kdu_byte overwritten_byte;
    bool checked_out;
    bool MQ_segment;
    bool active;
    bool truncation_point_found;
    mq_encoder *prev; // Point from which coding continues in same segment
    mq_encoder *next; // Point to which coding continues in next segment
  };

/*****************************************************************************/
/* INLINE                       mqe_state::init                              */
/*****************************************************************************/

inline void
  mqe_state::init(int Sigma, kdu_int32 s)
{
  assert((Sigma >= 0) && (Sigma <= 46) && (s == (s&1)));
  mps_p_bar = mq_encoder::p_bar_table[Sigma] + (s<<31);
  transition = mq_encoder::transition_table + ((Sigma<<1) + s);
}


/* ========================================================================= */
/*                            Fast Coding Macros                             */
/* ========================================================================= */

/* Note: although in-lining is preferable, most compilers fail to realize the
   full speed potential of macros.  These are critical to the overall system
   throughput and also to the size of the code fragment which gets executed
   inside the coding pass loops. */

/*****************************************************************************/
/* MACRO                        _mqe_transfer_byte_                          */
/*****************************************************************************/

 /* The following macro implements the Transfer-Byte procedure from
    Section 12.1.2 of the book by Taubman and Marcellin.  The implementation,
    however, is carefully optimized to pair down the number of test and the
    overall number of instructions.  In particular, we test for the two
    conditions which can cause a bit-stuffing operation simultaneously. */

#define _mqe_transfer_byte_(C,t,temp,store)                                   \
  {                                                                           \
    temp += C>>MQE_CARRY_POS; /* Saves comparisons to add the carry first */  \
    if (temp >= 0xFF)                                                         \
      {                                                                       \
        *(store++) = (kdu_byte) 0xFF;                                         \
        temp = (temp | 0xFF)>>1; /* Holds 0x7F unless had FF and a carry */   \
        temp &= (C>>MQE_MSBS_POS); /* Clear carry if temp not initially FF */ \
        C &= MQE_MSBS_CLEAR;                                                  \
        t = 7;                                                                \
      }                                                                       \
    else                                                                      \
      {                                                                       \
        *(store++) = (kdu_byte) temp;                                         \
        temp = (C>>MQE_PARTIAL_POS) & 0xFF; /* Still need to clear carry */   \
        C &= MQE_PARTIAL_CLEAR;                                               \
        t = 8;                                                                \
      }                                                                       \
  }

/*****************************************************************************/
/* MACRO                            _mq_encode_                              */
/*****************************************************************************/

  /* The following macro implements the MQ-Encode procedure described in
     Section 12.1.2 of the book by Taubman and Marcellin.  The main
     optimization trick is that described in Section 17.1.1, whereby
     the test for renormalization appears first; this helps because conditional
     exchange can occur only during renormalization.  Beyond the implementation
     tricks suggested in Section 17.1.1, we are able to combine the tests
     for an MPS symbol and a renormalization event into a single test by
     using a quantity which we shall call `lps_p_bar'.  The most significant
     bit of this 32-bit word is 1 if the symbol being coded is an LPS.  The
     15 LSB's hold the normalized MPS probability, (`p_bar' in the book).
     This trick relies upon the fact that A-p_bar must be a strictly positive
     quantity at all times, since A >= 2^15 and p_bar < 2^15 are both
     guaranteed.
        To minimize the number of variables which are required, storage for
     `lps_p_bar' is shared with the incoming symbol.  The user should take
     careful note that the value of the symbol variable has no well-defined
     value upon return from the function. This may cause problems if the
     caller intends to re-use the symbol value.
        If your compiler and architecture support the allocation of registers
     for critical variables, it is recommended that `lps_p_bar', `state',
     `A' and `C' be allocated registers, in that order. */

#define _mq_encode_(lps_p_bar,state,A,C,t,temp,store)                         \
  { /* Note: incoming symbol must be in `lps_p_bar' as 0 or KDU_INT32_MIN */  \
    assert((lps_p_bar == 0) || (lps_p_bar == KDU_INT32_MIN));                 \
    lps_p_bar ^= (state).mps_p_bar;                                           \
    A -= lps_p_bar;                                                           \
    if (A >= MQE_A_MIN)                                                       \
      { /* Symbol is MPS and (A-p_bar) >= 2^15, meaning no renormalization */ \
        C += lps_p_bar;                                                       \
      }                                                                       \
    else                                                                      \
      { /* Renormalization is inevitable and conditional exchange possible */ \
        if (lps_p_bar >= 0)                                                   \
          { /* Symbol is MPS, but A-p_bar < 2^15. */                          \
            if (A < lps_p_bar)                                                \
              A = lps_p_bar; /* Conditional exchange */                       \
            else                                                              \
              C = C + lps_p_bar;                                              \
            state = (state).transition->mps;                                  \
          }                                                                   \
        else                                                                  \
          { /* Symbol is LPS, so A-p_bar is necessarily less than 2^15 */     \
            A &= KDU_INT32_MAX;                                               \
            lps_p_bar &= KDU_INT32_MAX;                                       \
            if (A < lps_p_bar)                                                \
              C = C + lps_p_bar; /* Conditional exchange */                   \
            else                                                              \
              A = lps_p_bar;                                                  \
            state = (state).transition->lps;                                  \
          }                                                                   \
        assert(A < MQE_A_MIN);                                                \
        do {                                                                  \
            A += A; C += C;                                                   \
            if ((--t)==0)                                                     \
              _mqe_transfer_byte_(C,t,temp,store);                            \
          } while (A < MQE_A_MIN);                                            \
      }                                                                       \
  }

/*****************************************************************************/
/* MACRO                        _mq_encode_run_                              */
/*****************************************************************************/

  /* Specialization of _mq_encode_ to the case where the state is equal to
     the special non-adaptive MQ coder state (last state in transition
     table); encodes a 2-bit run length, starting from the motr significant
     bit. */

#define _mq_encode_run_(run,A,C,t,temp,store)                                 \
  {                                                                           \
    assert(run == (run&3));                                                   \
    A -= 0x5601;                                                              \
    if (((run & 2)==0) && (A >= MQE_A_MIN))                                   \
      { /* Symbol is MPS and no renormalization is required. */               \
        C += 0x5601;                                                          \
      }                                                                       \
    else                                                                      \
      { /* Renormalization is required. */                                    \
        if ((run & 2) == 0)                                                   \
          { /* Symbol is an MPS */                                            \
            if (A < 0x5601)                                                   \
              A = 0x5601; /* Conditional exchange */                          \
            else                                                              \
              C += 0x5601;                                                    \
          }                                                                   \
        else                                                                  \
          { /* Symbol is an LPS */                                            \
            if (A < 0x5601)                                                   \
              C += 0x5601; /* Conditional exchange */                         \
            else                                                              \
              A = 0x5601;                                                     \
          }                                                                   \
        do {                                                                  \
            A += A; C += C; t--;                                              \
            if (t == 0)                                                       \
              _mqe_transfer_byte_(C,t,temp,store);                            \
          } while (A < MQE_A_MIN);                                            \
      }                                                                       \
    A -= 0x5601;                                                              \
    if (((run & 1)==0) && (A >= MQE_A_MIN))                                   \
      { /* Symbol is MPS and no renormalization is required. */               \
        C += 0x5601;                                                          \
      }                                                                       \
    else                                                                      \
      { /* Renormalization is required. */                                    \
        if ((run & 1) == 0)                                                   \
          { /* Symbol is an MPS */                                            \
            if (A < 0x5601)                                                   \
              A = 0x5601; /* Conditional exchange */                          \
            else                                                              \
              C += 0x5601;                                                    \
          }                                                                   \
        else                                                                  \
          { /* Symbol is an LPS */                                            \
            if (A < 0x5601)                                                   \
              C += 0x5601; /* Conditional exchange */                         \
            else                                                              \
              A = 0x5601;                                                     \
          }                                                                   \
        do {                                                                  \
            A += A; C += C; t--;                                              \
            if (t == 0)                                                       \
              _mqe_transfer_byte_(C,t,temp,store);                            \
          } while (A < MQE_A_MIN);                                            \
      }                                                                       \
  }

/*****************************************************************************/
/* MACRO                            _raw_encode_                             */
/*****************************************************************************/

  /* By contrast with MQ coding, symbols here must be either 0 or 1
     (not 0 or KDU_INT32_MIN). */

#define _raw_encode_(symbol,t,temp,store)                                     \
  {                                                                           \
    assert((symbol == 0) || (symbol == 1));                                   \
    if (t==0)                                                                 \
      {                                                                       \
        *(store++) = (kdu_byte) temp;                                         \
        temp = (temp+1)>>8; /* Evaluates to 1 if `temp' was FF, else 0. */    \
        t = 8-temp;                                                           \
        temp = 0;                                                             \
      }                                                                       \
    temp += temp + symbol;                                                    \
    t--;                                                                      \
  }

#endif // MQ_ENCODER_H
