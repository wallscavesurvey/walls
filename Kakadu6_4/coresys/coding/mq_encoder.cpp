/*****************************************************************************/
// File: mq_encoder.cpp [scope = CORESYS/CODING]
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
   Implements the "mq_encoder" object interfaces defined in "mq_encoder.h".
The implementation is desiged to be almost entirely independent of the rest
of the Kakadu system.  It is easily tested and exploited in isolation.
******************************************************************************/

#include "mq_encoder.h"

kdu_int32 mq_encoder::p_bar_table[47] =
    { 0x5601, 0x3401, 0x1801, 0x0AC1, 0x0521, 0x0221,
      0x5601, 0x5401, 0x4801, 0x3801, 0x3001, 0x2401, 0x1C01, 0x1601,
      0x5601, 0x5401, 0x5101, 0x4801, 0x3801, 0x3401, 0x3001, 0x2801,
      0x2401, 0x2201, 0x1C01, 0x1801, 0x1601, 0x1401, 0x1201, 0x1101,
      0x0AC1, 0x09C1, 0x08A1, 0x0521, 0x0441, 0x02A1, 0x0221, 0x0141,
      0x0111, 0x0085, 0x0049, 0x0025, 0x0015, 0x0009, 0x0005, 0x0001,
      0x5601 };

mqe_transition mq_encoder::transition_table[94];

static void initialize_transition_table();
static class mq_encoder_local_init {
    public: mq_encoder_local_init()
              { initialize_transition_table(); }
  } _do_it;

/*****************************************************************************/
/* STATIC                  initialize_transition_table                       */
/*****************************************************************************/

static void
  initialize_transition_table()
{
  int Sigma_mps[47] =
    { 1, 2, 3, 4, 5,38, 7, 8, 9,10,11,12,13,29,15,16,17,18,19,20,21,22,23,24,
     25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,45,46};
  int Sigma_lps[47] =
    { 1, 6, 9,12,29,33, 6,14,14,14,17,18,20,21,14,14,15,16,17,18,19,19,20,21,
     22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,46};

  for (kdu_int32 n=0; n < 94; n++)
    {
      kdu_int32 s, new_s;
      int Sigma, new_Sigma;

      s = n & 1;
      Sigma = n >> 1;

      // Build MPS transition state first.
      new_Sigma = Sigma_mps[Sigma];
      new_s = s; // No switch on MPS transition.
      mq_encoder::transition_table[n].mps.init(new_Sigma,new_s);
      // Now build the LPS transition state.
      new_Sigma = Sigma_lps[Sigma];
      new_s = s;
      if ((mq_encoder::p_bar_table[Sigma] == 0x5601) && (Sigma != 46))
        new_s = 1-s;
      mq_encoder::transition_table[n].lps.init(new_Sigma,new_s);
    }
}

/*****************************************************************************/
/*                             mq_encoder::start                             */
/*****************************************************************************/

void
  mq_encoder::start(kdu_byte *buffer, bool MQ_segment)
{
  assert(!active);
  assert(buf_start == NULL);
  assert((prev == NULL) && (next == NULL));
  assert(buffer != NULL);
  active = true;
  truncation_point_found = false;
  this->MQ_segment = MQ_segment;
  buf_start = buffer; // First byte will always be discarded
  checked_out = false;
  if (MQ_segment)
    {
      A = MQE_A_MIN;  C = 0; t=12; temp=0;
      buf_next = buf_start - 1;
      overwritten_byte = *buf_next;
    }
  else
    {
      buf_next = buf_start;
      t=8; temp = 0;
    }
}

/*****************************************************************************/
/*                            mq_encoder::terminate                          */
/*****************************************************************************/

kdu_byte *
  mq_encoder::terminate(bool optimal)
{
  kdu_byte *buf_limit;

  assert(active);
  assert(!checked_out);
  if (MQ_segment)
    { /* Perform easy MQ terminating flush (see Section 12.3.1 of the book
         by Taubman and Marcellin). */
      kdu_int32 save_A = A; // Need these quantities for later calculating
      kdu_int32 save_C = C; // optimal truncation lengths.
      kdu_int32 save_t = t;
      kdu_int32 save_temp = temp;
      kdu_byte *save_buf_next = buf_next;

      int n_bits = MQE_CARRY_POS-15-t; // Number of bits we need to flush.
      C <<= t; // Move the next 8 available bits into the partial byte.
      while (n_bits > 0)
        {
          transfer_byte();
          n_bits -= t; // New value of t is the number of bits just transferred
          C <<= t;
        }
      transfer_byte(); // Flush the temp byte buffer.
      buf_limit = buf_next;
      if (optimal)
        { // Restore previous state so that `find_truncation_point' works.
          A = save_A; C = save_C; t = save_t; temp = save_temp;
          buf_next = save_buf_next;
        }
      // Prepare to restore the overwritten byte.
      buf_start[-1] = overwritten_byte;
    }
  else
    { // Perform raw terminating flush.
      kdu_int32 save_t = t;
      kdu_int32 save_temp = temp;
      kdu_byte *save_buf_next = buf_next;
      if (optimal)
        { // For optimal truncation later on, pad with 1's.
          if (t != 8)
            {
              for (; t > 0; t--)
                temp = (temp<<1) + 1;
              *(buf_next++) = (kdu_byte) temp;
            }
        }
      else
        { // For error resilient termination, pad with alternating 0/1 sequence
          if (temp == 0xFF)
            { // Safest to include stuff bit for resilient raw termination
              assert(t==0);
              *(buf_next++) = (kdu_byte) temp;
              temp = 0;
              t = 7;
            }
          if (t != 8)
            {
              for (int pad=0; t > 0; t--, pad=1-pad)
                temp = (temp<<1) + pad;
              *(buf_next++) = (kdu_byte) temp;
            }
        }
      buf_limit = buf_next;
      if (optimal)
        { // Restore previous state so that `find_truncation_point' works.
          t = save_t; temp = save_temp; buf_next = save_buf_next;
        }
    }

  mq_encoder *scan = this;
  while (scan->prev != NULL)
    scan = scan->prev;
  for (; scan != this; scan=scan->next)
    if (!scan->truncation_point_found)
      scan->find_truncation_point(buf_limit);

  active = false;

  if (optimal)
    this->find_truncation_point(buf_limit);
  else
    { // Discard any trailing FF, but that is all.
      assert(buf_next == buf_limit);
      if ((buf_next > buf_start) && (buf_next[-1] == 0xFF))
        buf_next--;
      truncation_point_found = true;
    }

  return buf_next;
}

/*****************************************************************************/
/*                      mq_encoder::find_truncation_point                    */
/*****************************************************************************/

void
  mq_encoder::find_truncation_point(kdu_byte *limit)
{
  assert(!active);
  assert(!truncation_point_found);

  if (MQ_segment)
    { /* Find F_min using the algorithm developed in Section 12.3.2 of the
         book by Taubman and Marcellin.  You will have a lot of trouble
         understanding how this algorithm works without reading the book.  To
         deal with the limitations of a 32-bit word length, we represent the
         quantities Cr-R_F and Ar+Cr-R_F as follows:
            Cr-R_F = C_low + C_high*2^{27}
            Cr+Ar-R_F = CplusA_low + CplusA_high*2^{27}
         During the iterative loop to find F_min, each iteration shifts
         the quantities, Cr-R_F and Cr+Ar-R_F up by s bit positions before
         performing the relevant termination tests.  These modifications have
         no impact on the outcome of the algorithm described in the book, but
         are simpler to implement in the context of our split numeric
         representation. */
      kdu_byte save_byte = buf_start[-1];
      buf_start[-1] = 0; // Need this for algorithm to work in all cases

      kdu_int32 C_low = C<<t;
      kdu_int32 CplusA_low = (C+A)<<t;
      kdu_int32 C_high = temp;
      kdu_int32 CplusA_high = temp;
      if (C_low & MQE_CARRY_BIT)
        { C_high++; C_low -= MQE_CARRY_BIT; }
      if (CplusA_low & MQE_CARRY_BIT)
        { CplusA_high++; CplusA_low -= MQE_CARRY_BIT; }

      int s = 8;
      int F_min = 0;

      while ((C_high > 0xFF) || (CplusA_high <= 0xFF))
        {
          assert(buf_next < limit);
          F_min++;

          /* Augment R_F (Bear in mind that the high quantities always
             hold the most significant 8 bits of the S_F-bit nominal
             word sizes.  Instead of successively decreasing S_F by s,
             we are leaving S_F the same and successively up-shifting
             the words by s -- same thing. */

          temp = *(buf_next++);
          C_high -= temp << (8-s);
          CplusA_high -= temp << (8-s);

          // Shift Cr-R_F up by s bit positions
          C_high <<= s;
          C_high += C_low >> (MQE_CARRY_POS-s);
          C_low <<= s;
          C_low &= ~(((kdu_int32)(-1))<<MQE_CARRY_POS);

          // Shift Cr+Ar-R_F up by s bit positions
          CplusA_high <<= s;
          CplusA_high += CplusA_low >> (MQE_CARRY_POS-s);
          CplusA_low <<= s;
          CplusA_low &= ~(((kdu_int32)(-1))<<MQE_CARRY_POS);

          // Find new value of s
          if (temp == 0xFF)
            s = 7;
          else
            s = 8;
        }
      assert(F_min <= 5);
      buf_start[-1] = save_byte;
    }
  else
    { /* Raw segment.  Nothing to do other than marking the existence of any
         partially completed byte and then executing the common code below to
         discard trailing strings of 1's, which will be synthesized by the
         decoder. */
      if (t != 8)
        buf_next++;
    }

  // Discard trailing FF's or FF7F pairs.

  if ((buf_next > buf_start) && (buf_next[-1] == 0xFF))
    buf_next--;
  while (((buf_next-buf_start) >= 2)  &&
         (buf_next[-1] == 0x7F) && (buf_next[-2] == 0xFF))
    buf_next -= 2;

  truncation_point_found = true;
}

/*****************************************************************************/
/*                           mq_encoder::transfer_byte                       */
/*****************************************************************************/

void
  mq_encoder::transfer_byte()
  /* Direct implementation of the Transfer-Byte procedure in Section 12.1.2
     of the book by Taubman and Marcellin. */
{
  assert(!checked_out);
  if (temp == 0xFF)
    { // Can't propagate carry past this byte; need a bit stuff
      *(buf_next++) = (kdu_byte) temp;
      temp = C >> MQE_MSBS_POS; // Transfer 7 bits plus any carry bit
      C &= MQE_MSBS_CLEAR;
      t = 7;
    }
  else
    {
      temp += (C>>MQE_CARRY_POS) & 1; // Propagate any carry bit from C to temp
      C &= ~MQE_CARRY_BIT; // Reset the carry bit
      *(buf_next++) = (kdu_byte) temp;
      if (temp == 0xFF) // Decoder will see this as a bit stuff
        {
          temp = (C>>MQE_MSBS_POS);
          C &= MQE_MSBS_CLEAR;
          t = 7;
        }
      else
        {
          temp = (C>>MQE_PARTIAL_POS);
          C &= MQE_PARTIAL_CLEAR;
          t = 8;
        }
    }
}

/*****************************************************************************/
/*                            mq_encoder::mq_encode                          */
/*****************************************************************************/

void
  mq_encoder::mq_encode(kdu_int32 symbol, mqe_state &state)
  /* Direct implementation of the algorithm given in Section 17.1.1 of the
     book by Taubman and Marcellin. */
{
  assert(MQ_segment && active && (!checked_out) &&
         ((symbol==0) || (symbol==KDU_INT32_MIN)));

  kdu_int32 p_bar = state.mps_p_bar & 0x7FFF;
  A -= p_bar;
  if (((symbol ^ state.mps_p_bar) & KDU_INT32_MIN) == 0)
    { // Coding an MPS
      if (A >= MQE_A_MIN)
        { // No renormalization and hence no conditional exchange
          C += p_bar;
        }
      else
        { // Renormalization is required.
          if (A < p_bar)
            A = p_bar; // Conditional exchange
          else
            C += p_bar;
          state = state.transition->mps;
          do {
              A += A; C += C; t--;
              if (t == 0)
                transfer_byte();
            } while (A < MQE_A_MIN);
        }
    }
  else
    { // Coding an LPS; renormalization is inevitable
      if (A < p_bar)
        C += p_bar; // Conditional exchange
      else
        A = p_bar;
      state = state.transition->lps;
      do {
          A += A; C += C; t--;
          if (t == 0)
            transfer_byte();
        } while (A < MQE_A_MIN);
    }
}

/*****************************************************************************/
/*                         mq_encoder::mq_encode_run                         */
/*****************************************************************************/

void
  mq_encoder::mq_encode_run(kdu_int32 run)
{
  assert(MQ_segment && active && !checked_out);
  mqe_state state; state.init(46,0);
  mq_encode((run&2)<<30,state);
  mq_encode(run<<31,state);
}

/*****************************************************************************/
/*                           mq_encoder::raw_encode                          */
/*****************************************************************************/

void
  mq_encoder::raw_encode(kdu_int32 symbol)
{
  assert((!MQ_segment) && active && (!checked_out) &&
         ((symbol == 0) || (symbol == 1)));
  if (t == 0)
    {
      *(buf_next++) = (kdu_byte) temp;
      t = (temp == 0xFF)?7:8;
      temp = 0;
    }
  temp = (temp<<1) + symbol;
  t--;
}
