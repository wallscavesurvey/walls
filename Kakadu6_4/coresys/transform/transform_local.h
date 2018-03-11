/*****************************************************************************/
// File: transform_local.h [scope = CORESYS/TRANSFORMS]
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
   Provides local definitions common to both the DWT analysis and the DWT
synthesis implementations in "analysis.cpp" and "synthesis.cpp".
******************************************************************************/

#ifndef TRANSFORM_LOCAL_H
#define TRANSFORM_LOCAL_H

#include <assert.h>
#include <math.h>
#include "kdu_messaging.h"
#include "kdu_sample_processing.h"

// Defined here:
struct kd_vlift_line;
class kd_vlift_queue;
struct kd_lifting_step;


/*****************************************************************************/
/*                              kd_vlift_line                                */
/*****************************************************************************/

struct kd_vlift_line {
  public: // function members
    void pre_create(kdu_sample_allocator *allocator, int even_width,
                    int odd_width, bool absolute, bool use_shorts,
                    int neg_extent, int pos_extent, bool both_cosets)
      {
        cosets[0].pre_create(allocator,even_width,absolute,use_shorts,
                             neg_extent,pos_extent-even_width);
        if (both_cosets)
          cosets[1].pre_create(allocator,odd_width,absolute,use_shorts,
                               neg_extent,pos_extent-odd_width);
      }
    void create()
      { cosets[0].create(); cosets[1].create(); }
    void destroy()
      { cosets[0].destroy(); cosets[1].destroy(); }
  public: // data members
    kdu_line_buf cosets[2];
    kd_vlift_line *next; // For building linked lists of lines
  };
  /* Notes:
        Instances of this structure represent a single line.  If the line
     will eventually be subjected to horizontal decomposition, it is
     separated into two cosets, corresponding to the even- and odd-indexed
     samples.  During subband analysis, it is more efficient to perform
     this separation into cosets right away, before the vertical filtering
     which precedes the horizontal transformation itself.  Similarly,
     during subband synthesis, it is preferable to leave the lines
     produced by horizontal synthesis in this de-interleaved form, until
     after vertical synthesis.
        If no horizontal transform step is to be used, only the first
     (even) coset is actually used.  To facilitate this, the `pre_create'
     function is supplied with the width of each of the two cosets
     separately.
        The `neg_extent' and `pos_extent' arguments represent the range
     of indices which should be supported on each coset buffer.  In particular,
     each coset's buffer should support access with indices from
     -`neg_extent' to +`pos_extent', where `pos_extent' >= `even_width'
     and `pos_extent' >= `odd_width'. */

/*****************************************************************************/
/*                              kd_vlift_queue                               */
/*****************************************************************************/

class kd_vlift_queue {
  //---------------------------------------------------------------------------
  public: // Lifecycle functions
    void init(int y_min, int y_max, int queue_idx, bool symmetric_extension,
              int max_source_request_idx)
      { /* For the interpretation of most of the arguments, see the notes
           below, describe the member variables with the same names.
           The `max_source_request_idx' argument identifies the location of
           the last line which will ever be requested via `access_source'.
           This is used to determine the `recycling_lim' value. */
        assert(((max_source_request_idx ^ queue_idx) & 1) == 0);
        this->queue_idx = (kdu_byte) queue_idx;
        this->y_min = y_min;  this->y_max = y_max;
        head_idx = source_pos = update_pos = y_min - 1000; // Ridiculous value
        this->symmetric_extension = symmetric_extension;
        if ((!symmetric_extension) || (max_source_request_idx < y_max))
          recycling_lim = y_max - 1; // Keep last row only
        else
          recycling_lim = 2*y_max - max_source_request_idx;
        if (queue_idx < 0)
          source_pos = recycling_lim = y_max+2; // Block use of `access_source'
        tail_idx = head_idx - 2;
        head = tail = NULL;
      }
  //---------------------------------------------------------------------------
  public: // Queue management functions
    void source_done()
      { /* Call this function once you know that there will be no more calls
           to `access_source'.  This sets `recycling_lim' and `source_pos'
           both beyond the value of `y_max' so that they no longer influence
           the behaviour of the stripe recycling machinery or the conditions
           under which `test_update' will return true. */
        source_pos = recycling_lim = y_max + 2;
          // We won't bother trying to recycle stripes here, since this should
          // happen soon enough on the next call to `access_update'.
      }
    void push_line(int idx, kd_vlift_line *line, kd_vlift_line * &free_list)
      { /* Used to push new lines into the queue.  The `idx' argument is
           the absolute location of this line.  It must have the same
           parity (LSB) as the `queue_idx' identifier for this queue. */
        assert(((idx ^ queue_idx) & 1) == 0);
        if ((idx < source_pos) && (idx < update_pos))
          { // Recycle line immediately; it will never contribute.  Also
            // recycle any existing lines on the queue.
            line->next = free_list;  free_list = line;
            while ((tail=head) != NULL)
              { head=tail->next;  tail->next=free_list;  free_list=tail; }
            return;
          }
        line->next = NULL;
        if (tail == NULL)
          { head = tail = line;  head_idx = idx;  }
        else
          { assert(idx == (tail_idx+2));  tail = tail->next = line; }
        tail_idx = idx;
      }
    bool test_update(int idx, bool need_exclusive_use)
      { /* Returns true if the line identified by `idx' is currently available.
           Also sets the `update_pos' member equal to `idx' to make it clear
           that no calls to `access_update' will attempt to access earlier
           rows than this.  If `need_exclusive_use' is true and the
           line identified by `idx' might be required by future calls to
           `access_source', the function returns false.  For this function
           to work correctly in conjunction with the `need_exclusive_use'
           option, you must remember to call `source_done' once you have
           finished all calls to `access_source'. */
        update_pos = idx;
        return ((idx >= head_idx) && (idx <= tail_idx) &&
                ((!need_exclusive_use) ||
                 ((idx < source_pos) && (idx < recycling_lim))));
      }
    kd_vlift_line *access_update(int idx, kd_vlift_line * &free_list)
      { /* Use this function to access the next line which needs to be
           updated by lifting step `queue_idx'+1.   The index of the line
           is given by `idx', which must have the same parity as `queue_idx'.
           If the function succeeds, it advances its internal record of the
           next line to be accessed in subsequent calls. */
        assert((((idx ^ queue_idx) & 1) == 0) && (idx >= update_pos));
        update_pos = idx; // At least this large
        if ((idx < head_idx) || (idx > tail_idx))
          return NULL;
        kd_vlift_line *result = head;
        for (; idx > head_idx; idx-=2, result=result->next);
        assert(result != NULL);
        update_pos += 2;
        while ((head_idx < source_pos) && (head_idx < update_pos) &&
               (head != NULL) && (head_idx < recycling_lim))
          { head_idx+=2;  kd_vlift_line *new_head = head->next;
            head->next = free_list;  free_list = head;
            if ((head = new_head) == NULL) tail = NULL;
          }
        return result;
      }
    bool access_source(int idx, int num, kd_vlift_line *lines[],
                       kd_vlift_line * &free_list)
      { /* Use this function to access the source lines for lifting step
           `queue_idx'.  The number of required lines is given by `num'
           and the location of the first line is given by `idx'.  The function
           returns false if one or more of the requested lines is unavailable.
           Otherwise, it returns true and also advances the internal
           `source_pos' pointer to `idx'+2.  You may not invoke this
           function for the queue whose `queue_index' value is -1.
              Perhaps the most important feature of this function is that
           it implements the boundary extension policy automatically.  This
           means that the range of requested lines can lie outside the range
           of actual available lines. */
        assert((((idx ^ queue_idx) & 1) == 0) && (idx >= source_pos));
        source_pos = idx; // At least this large
        num--;  idx += num<<1; // Find index of last requested line; allows
                              // us to detect failure as early as possible
        if ((idx > tail_idx) && (idx <= y_max))
          return false; // Almost all failures are detected here, in practice
        int k;
        for (lines+=num; num >= 0; idx-=2, num--, lines--)
          {
            k = idx;
            while ((k < y_min) || (k > y_max))
              if (k < y_min)
                k = (symmetric_extension)?(2*y_min-k):(y_min+((y_min^k)&1));
              else
                k = (symmetric_extension)?(2*y_max-k):(y_max-((y_max^k)&1));
            if ((k < head_idx) || (k > tail_idx)) return false;
            for (*lines=head; k > head_idx; k-=2, *lines=(*lines)->next);
          }
        source_pos += 2; // Advance `source_pos' when successful
        while ((head_idx < update_pos) && (head_idx < source_pos) &&
               (head != NULL) && (head_idx < recycling_lim))
          {
            head_idx+=2;  kd_vlift_line *new_head = head->next;
            head->next = free_list;  free_list = head;
            if ((head = new_head) == NULL) tail = NULL;
          }
        return true;
      }
  //---------------------------------------------------------------------------
  public: // Queue management simulation functions
    void simulate_push_line(int idx, int &used_lines)
      { /* Used in the simulation phase to discover buffer requirements. */
        assert(((idx ^ queue_idx) & 1) == 0);
        if ((idx < source_pos) && (idx < update_pos))
          { // Recycle line immediatel.  It will never contribute.
            used_lines--;  return;
          }
        if (tail_idx < head_idx)
          head_idx = idx;
        tail_idx = idx;
      }
    bool simulate_access_update(int idx, int &used_lines)
      { /* Used in the simulation phase to discover buffer requirements.
           Returns false if `access_update' would have returned NULL.
           `used_lines' represents the total number of lines which would be
           allocated from the free list during real processing. */
        assert((((idx ^ queue_idx) & 1) == 0) && (idx >= update_pos));
        update_pos = idx; // At least this large
        if ((idx < head_idx) || (idx > tail_idx))
          return false;
        update_pos += 2;
        while ((head_idx < update_pos) && (head_idx < source_pos) &&
               (tail_idx >= head_idx) && (head_idx < recycling_lim))
          { head_idx+=2; used_lines--; }
        return true;
      }
    bool simulate_access_source(int idx, int num, int &used_lines)
      { /* Used in the simulation phase to discover buffer requirements.
           `used_lines' represents the total number of lines which would
           be allocated from the free list during real processing. */
        assert((((idx ^ queue_idx) & 1) == 0) && (idx >= source_pos));
        source_pos = idx; // At least this large
        num--;  idx += num<<1; // Find index of last requested line; allows
                               // us to detect failure as early as possible
        int k;
        for (; num >= 0; idx-=2, num--)
          {
            k = idx;
            while ((k < y_min) || (k > y_max))
              if (k < y_min)
                k = (symmetric_extension)?(2*y_min-k):(y_min+((y_min^k)&1));
              else
                k = (symmetric_extension)?(2*y_max-k):(y_max-((y_max^k)&1));
            if ((k < head_idx) || (k > tail_idx)) return false;
          }
        source_pos += 2; // Advance `source_pos' when successful
        while ((head_idx < update_pos) && (head_idx < source_pos) &&
               (tail_idx >= head_idx) && (head_idx < recycling_lim))
          { head_idx+=2; used_lines--; }
        return true;
      }
  //---------------------------------------------------------------------------
  private: // Data
    int y_min; // Absolute location of first line in image at this DWT node
    int y_max; // Absolute location of last line in image at this DWT node
    kd_vlift_line *head; // Upper-most line in the queue
    kd_vlift_line *tail; // Lower-most line in the queue
    int head_idx; // Absolute location of first line in the queue
    int tail_idx; // Absolute location of last line in the queue
    int source_pos; // See below
    int update_pos; // See below
    int recycling_lim; // See below
    bool symmetric_extension; // Extension method used by lifting steps
    kdu_byte queue_idx;
  };
  /* Notes:
       This object maintains a queue of lines, which form the basis for
     incremental processing of vertical lifting steps.  There are exactly
     N+1 such queues, where N is the number of lifting steps.  The
     arrangement of input queues and lifting steps is illustrated below
     for the case of 4 lifting steps, so as to facilitate conceptualization:

        ----------------> Q0     -----------> Q2      ---------->
                        /   \    |          /   \     |
                  STEP0      STEP1     STEP2      STEP3
                 /    |           \   /    |           \
        ----> Q(-1)   ------------> Q1     ------------> Q3 ---->

     The queues are labeled Q(-1) through Q(N-1), where even-indexed queues
     hold even-indexed image lines and odd-indexed queues hold odd-indexed
     image lines.  The `queue_index' member thus takes on values from -1 to
     N-1.  Lifting step k takes a weighted combination of lines from
     queue Qk and adds the result into a line from queue Q(k-1), pushing
     the result onto the tail of queue Q(k+1).
        All line locations are expressed as absolute indices in the
     coordinate system of the node in the DWT tree which is being transformed.
     The `y_min' and `y_max' members identify the inclusive lower and
     upper bounds of line positions which belong to the entire
     tile-component at this DWT node.  Only even or odd locations
     within this range can actually be found within the queue, depending
     on whether `queue_idx' is even or odd.
        `head_idx' member identifies the location of the first line currently
     managed by the queue -- it must have the same parity as `queue_idx'.
     The last line in the queue has location `head_idx'+2*(`num_lines'-1).
        The `source_pos' member of Q(k) holds the location of the first line
     which will be required by STEP(k) when it next produces a
     result.  The `update_pos' member of Q(k) holds the location of the next
     line to be updated by STEP(k+1).  Lines from the head of the queue
     are automatically recycled so as to ensure that one of these two
     members is always equal to `head_idx'.
        The one exception to the above recycling principle is that no line
     whose location equals or exceeds `recycling_lim' will be automatically
     recycled.  This condition is required to implement the symmetric
     boundary extension property for certain transform kernels, since
     symmetric extension can cause the need for later access to a line
     which would otherwise have been recycled already.
        Before concluding, it is worth describing how the queues and lifting
     steps interact during synthesis.  This is shown in the diagram below.

        <---------------- Q0 <--       -------<-- Q2 <--       -----<--
                        /      |     /         /       |    /
                  STEP0        STEP1      STEP2        STEP3
                  |    \            \     |    \            \
        <---- Q(-1)      --------<-- Q1 <--      --------<-- Q3 ---<--
  */

/*****************************************************************************/
/*                             kd_lifting_step                               */
/*****************************************************************************/

struct kd_lifting_step {
    kdu_byte step_idx; // Runs from 0 to N-1 where N is the number of steps
    kdu_byte support_length;
    kdu_byte downshift;
    kdu_byte extend; // Used only for horizontal lifting steps
    kdu_int16 support_min;
    kdu_int16 rounding_offset;
    float *coeffs; // Valid indices run from 0 to `support_length'
    int *icoeffs; // Valid indices run from 0 to `support_length'
    bool add_shorts_first; // See below
    bool reversible;
    kdu_byte kernel_id; // One of Ckernels_W5X3, Ckernels_W9X7 or Ckernels_ATK
  };
  /* Notes:
       If `step_idx' is even, this lifting step updates odd-indexed (high-pass)
     samples, based on even-indexed input samples.  If `step_idx' is odd, it
     updates even-indexed (low-pass) samples, based on odd-indexed input
     samples.
       The `support_min' and `support_length' members identify the values of
     Ns and Ls such that the lifting step implements
     X_s[2k+1-p]=TRUNC(sum_{Ns<=n<Ls+Ns} Cs[n]*X_{s-1}[2k+p+2n]).  If the
     step is to be implemented using floating point arithmetic, the `TRUNC'
     operator does nothing and Cs[Ns+n] = `coeffs'[n].  If the step is to be
     implemented using integer arithmetic, Cs[Ns+n] = `icoeffs'[n]/2^downshift
     and TRUNC(x) = floor(x + `rounding_offset' / 2^downshift).
       The `extend' member identifies the amount by which the source sequence
     must be extended to the left and to the right in order to avoid treating
     boundaries specially during horizontal lifting.  This depends on both
     the lifting step kernel's region of support (`support_min' and
     `support_length') and the parity of the first and last samples on each
     horizontal line.  Although the left and right extensions are often
     different, it is much more convenient to just keep the maximum of the
     two values.
       The `add_shorts_first' member is true if the lifting implementation for
     16-bit samples adds pairs of input samples together, creating a 16-bit
     quantity prior to multiplication.  In this case, the sample normalization
     processes must also ensure that the 16-bit sum will not overflow. */


/* ========================================================================= */
/*                            EXTERNAL FUNCTIONS                             */
/* ========================================================================= */

/*****************************************************************************/
/*                  perform_synthesis_lifting_step (16-bit)                  */
/*****************************************************************************/

extern void
  perform_synthesis_lifting_step(kd_lifting_step *step,
                                 kdu_sample16 *src_bufs[],
                                 kdu_sample16 *in, kdu_sample16 *out,
                                 int width, int start_loc);
  /* This function is used both by `kd_synthesis' and `kd_multi_synthesis'
     to implement a single synthesis lifting step; in the former case it
     is used to implement vertical wavelet syntheis, while in the latter case
     it is used to implement DWT synthesis operations where they are
     involved in multi-component transformation networks.  This particular
     function uses 16-bit sample values.
        The function performs the same lifting step on all samples found
     within a line.  Specifically, the function updates the samples in the
     line referenced by `in', writing the updated result to the line
     referenced by `out', based upon the source lines referenced by
     `src_bufs'[n], for 0 <= n < N, where N is the value of
     `step->support_length'.
        All of the line buffers are guaranteed to be aligned on 16-byte
     boundaries, but the first valid sample is given by `start_loc' while
     the number of valid samples is given by `width'.  The implementation
     may process additional samples to the left or to the right of this
     valid region, to the extent required by 16-byte aligned vector
     processing instructions.
        All remaining information required to implement the lifting step is
     provided by the `step' object, including whether the sample values
     are represented reversibly or irreversibly.
        Note that the `in' buffer may be identical to the `out' buffer.
     Note also that the contents of the `src_bufs' array may be
     overwritten by this function, depending on its implementation. */

/*****************************************************************************/
/*                  perform_synthesis_lifting_step (32-bit)                  */
/*****************************************************************************/

extern void
  perform_synthesis_lifting_step(kd_lifting_step *step,
                                 kdu_sample32 *src_bufs[],
                                 kdu_sample32 *in, kdu_sample32 *out,
                                 int width, int start_loc);
  /* Same as the above function, but this one works with 32-bit sample
     values. */

/*****************************************************************************/
/*                   perform_analysis_lifting_step (16-bit)                  */
/*****************************************************************************/

extern void
  perform_analysis_lifting_step(kd_lifting_step *step,
                                kdu_sample16 *src_bufs[],
                                kdu_sample16 *in, kdu_sample16 *out,
                                int width, int start_loc);
  /* Same as the corresponding `perform_synthesis_lifting_step' function,
     except that this function performs the analysis operation -- i.e., it
     adds the source contributions to the input lines rather than subtracting
     them. */

/*****************************************************************************/
/*                   perform_analysis_lifting_step (32-bit)                  */
/*****************************************************************************/

extern void
  perform_analysis_lifting_step(kd_lifting_step *step,
                                kdu_sample32 *src_bufs[],
                                kdu_sample32 *in, kdu_sample32 *out,
                                int width, int start_loc);
  /* Same as the corresponding `perform_synthesis_lifting_step' function,
     except that this function performs the analysis operation -- i.e., it
     adds the source contributions to the input lines rather than subtracting
     them. */

#endif // TRANSFORM_LOCAL_H
