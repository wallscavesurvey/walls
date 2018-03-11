/*****************************************************************************/
// File: mq_decoder.cpp [scope = CORESYS/CODING]
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
   Implements the "mq_decoder" object interfaces defined in "mq_decoder.h".
The implementation is desiged to be almost entirely independent of the rest
of the Kakadu system.  It is easily tested and exploited in isolation.
******************************************************************************/

#include "mq_decoder.h"

kdu_int32 mq_decoder::p_bar_table[47] =
    { 0x5601, 0x3401, 0x1801, 0x0AC1, 0x0521, 0x0221,
      0x5601, 0x5401, 0x4801, 0x3801, 0x3001, 0x2401, 0x1C01, 0x1601,
      0x5601, 0x5401, 0x5101, 0x4801, 0x3801, 0x3401, 0x3001, 0x2801,
      0x2401, 0x2201, 0x1C01, 0x1801, 0x1601, 0x1401, 0x1201, 0x1101,
      0x0AC1, 0x09C1, 0x08A1, 0x0521, 0x0441, 0x02A1, 0x0221, 0x0141,
      0x0111, 0x0085, 0x0049, 0x0025, 0x0015, 0x0009, 0x0005, 0x0001,
      0x5601 };

mqd_transition mq_decoder::transition_table[94];

static void initialize_transition_table();
static class mq_decoder_local_init {
    public: mq_decoder_local_init()
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
      mq_decoder::transition_table[n].mps.init(new_Sigma,new_s);
      // Now build the LPS transition state.
      new_Sigma = Sigma_lps[Sigma];
      new_s = s;
      if ((mq_decoder::p_bar_table[Sigma] == 0x5601) && (Sigma != 46))
        new_s = 1-s;
      mq_decoder::transition_table[n].lps.init(new_Sigma,new_s);
    }
}

/*****************************************************************************/
/*                             mq_decoder::start                             */
/*****************************************************************************/

void
  mq_decoder::start(kdu_byte *buffer, int segment_length, bool MQ_segment)
{
  assert(!active);
  assert(buf_start == NULL);
  assert((buffer != NULL) && (segment_length >= 0));
  active = true;
  this->MQ_segment = MQ_segment;
  buf_start = buf_next = buffer;
  this->segment_length = segment_length;
  overwritten_bytes[0] = buffer[segment_length];
  overwritten_bytes[1] = buffer[segment_length+1];
  buffer[segment_length] = buffer[segment_length+1] = 0xFF; // Force termination
  checked_out = false;
  if (MQ_segment)
    {
      S = 0;
      temp = 0; C = 0; t = 0;
      fill_lsbs();
      C <<= t;
      fill_lsbs();
      C <<= 7;
      t -= 7;
      A = MQD_A_MIN;
    }
  else
    {
      t=0; temp = 0;
    }
}

/*****************************************************************************/
/*                             mq_decoder::finish                            */
/*****************************************************************************/

bool
  mq_decoder::finish(bool check_erterm)
{
  bool failure = false;

  assert(active && !checked_out);
  if (check_erterm && MQ_segment)
    { // Check correct termination of MQ segment.
      if (buf_next <= (buf_start+segment_length))
        failure = true; // We never read the terminating marker code
      else
        {
          assert(buf_next == (buf_start+segment_length+1));
          S++; // Include first FF of marker code as a synthesized FF.
          if (t == 0)
            { S++; t = 8; }
          if ((S < 2) || (S > 3) || ((C>>(24-t)) != 0))
            failure = true;
        }
    }
  else if (check_erterm)
    { // Check correct termination of raw segment.
      kdu_int32 x = 0x55;
      if ((buf_next < (buf_start+segment_length)) &&
          (temp == 0xFF) && (t == 0))
        { // Last byte may have been created by bit stuff.
          temp = *(buf_next++); t = 8; x = 0x2A;
        }
      if ((buf_next != (buf_start+segment_length)) ||
          ((temp & ~((-1)<<t)) != (x>>(8-t))))
        failure = true;
    }

  buf_start[segment_length] = overwritten_bytes[0];
  buf_start[segment_length+1] = overwritten_bytes[1];
  active = false;
  buf_start = buf_next = NULL;
  return !failure;
}

/*****************************************************************************/
/*                           mq_decoder::fill_lsbs                           */
/*****************************************************************************/

void
  mq_decoder::fill_lsbs()
{
  if (temp == 0xFF)
    { // Need to check for termination
      temp = *(buf_next++);
      if (temp > 0x8F)
        { // Termination marker found.
          temp = 0xFF;
          buf_next--;
          t = 8;
          S++; // Increment S for each synthesized FF after 1'st byte of marker
        }
      else
        { // Bit stuff found
          temp <<= 1;
          t = 7;
        }
    }
  else
    {
      temp = *(buf_next++);
      t = 8;
    }
  C += temp;
}

/*****************************************************************************/
/*                           mq_decoder::mq_decode                           */
/*****************************************************************************/

void
  mq_decoder::mq_decode(kdu_int32 &symbol, mqd_state &state)
  /* Direct implementation of the first version given in Section 17.1.1 of the
     book by Taubman and Marcellin.  The macro implements the second
     version of the decoding algorithm, which is substantially less
     fathomable. */
{
  assert(MQ_segment && active && !checked_out);

  symbol = state.p_bar_mps & 1; // Set to MPS for now.
  kdu_int32 shifted_p_bar = state.p_bar_mps - symbol;
  A -= shifted_p_bar;
  if (C >= shifted_p_bar)
    { // Upper sub-interval selected
      C -= shifted_p_bar;
      if (A < MQD_A_MIN)
        { // Need renormalization and perhaps conditional exchange
          if (A < shifted_p_bar)
            { // Conditional exchange; LPS decoded
              symbol = 1-symbol;
              state = state.transition->lps;
            }
          else
            { // MPS decoded
              state = state.transition->mps;
            }
          do {
              if (t == 0)
                fill_lsbs();
              A += A; C += C; t--;
            } while (A < MQD_A_MIN);
        }
    }
  else
    { // Lower sub-interval selected; renormalization is inevitable
      if (A < shifted_p_bar)
        { // Conditional exchange; MPS decoded
          state = state.transition->mps;
        }
      else
        { // LPS decoded
          symbol = 1-symbol;
          state = state.transition->lps;
        }
      A = shifted_p_bar;
      do {
          if (t == 0)
            fill_lsbs();
          A += A; C += C; t--;
        } while (A < MQD_A_MIN);
    }
}

/*****************************************************************************/
/*                         mq_decoder::mq_decode_run                         */
/*****************************************************************************/

void
  mq_decoder::mq_decode_run(kdu_int32 &run)
{
  assert(MQ_segment && active && !checked_out);
  mqd_state state; state.init(46,0);
  kdu_int32 symbol;
  mq_decode(symbol,state);
  run = symbol + symbol;
  mq_decode(symbol,state);
  run += symbol;
}

/*****************************************************************************/
/*                           mq_decoder::raw_decode                          */
/*****************************************************************************/

void
  mq_decoder::raw_decode(kdu_int32 &symbol)
{
  assert((!MQ_segment) && active && !checked_out);

  if (t == 0)
    {
      if (temp == 0xFF)
        { // Need to check for terminating marker.
          temp = *(buf_next++);
          if (temp > 0x8F)
            { temp = 0xFF; buf_next--; t = 8; }
          else
            t = 7;
        }
      else
        { temp = *(buf_next++); t = 8; }
    }
  t--;
  symbol = (temp>>t)&1;
}
