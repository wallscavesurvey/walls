/*****************************************************************************/
// File: encoder.cpp [scope = CORESYS/CODING]
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
   Implements the functionality offered by the "kdu_encoder" object defined
in "kdu_sample_processing.h".  Includes quantization, subband sample buffering
and geometric appearance transformations.
******************************************************************************/

#include <math.h>
#include <string.h>
#include <assert.h>
#include "kdu_threads.h"
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "kdu_block_coding.h"
#include "kdu_kernels.h"
#include "kdu_roi_processing.h"

/* Note Carefully:
      If you want to be able to use the "kdu_text_extractor" tool to
   extract text from calls to `kdu_error' and `kdu_warning' so that it
   can be separately registered (possibly in a variety of different
   languages), you should carefully preserve the form of the definitions
   below, starting from #ifdef KDU_CUSTOM_TEXT and extending to the
   definitions of KDU_WARNING_DEV and KDU_ERROR_DEV.  All of these
   definitions are expected by the current, reasonably inflexible
   implementation of "kdu_text_extractor".
      The only things you should change when these definitions are ported to
   different source files are the strings found inside the `kdu_error'
   and `kdu_warning' constructors.  These strings may be arbitrarily
   defined, as far as "kdu_text_extractor" is concerned, except that they
   must not occupy more than one line of text.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
   kdu_error _name("E(encoder.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
   kdu_warning _name("W(encoder.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) kdu_error _name("Kakadu Core Error:\n");
#  define KDU_WARNING(_name,_id) kdu_warning _name("Kakadu Core Warning:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


/* ========================================================================= */
/*                      Class and Structure Definitions                      */
/* ========================================================================= */

/*****************************************************************************/
/*                                kd_encoder                                 */
/*****************************************************************************/

class kd_encoder : public kdu_push_ifc_base, public kdu_worker {
  public: // Member functions
    kd_encoder()
      { allocator=NULL; initialized=false; roi_node=NULL; queue=NULL;
        lines16=NULL; lines32=NULL; roi_lines=NULL; }
    void init(kdu_subband band, kdu_sample_allocator *allocator,
              bool use_shorts, float normalization, kdu_roi_node *roi,
              kdu_thread_env *env, kdu_thread_queue *env_queue);
  protected: // These functions implement their namesakes in the base class
    virtual ~kd_encoder();
    virtual void push(kdu_line_buf &line, kdu_thread_env *env);
    virtual void do_job(kdu_thread_entity *ent, int job_idx);
      /* Each row of code-blocks is partitioned into `num_jobs_per_row' jobs,
         and the code-block row together with the range of code-block indices
         within that row which are to be processed by the job are deduced
         from `job_idx' alone.  The `block_indices' array is considered
         static and the `job_idx' is used to locate the block indices which
         are to be processed. */
  private: // Data
    kdu_block_encoder block_encoder;
    kdu_subband band;
    kdu_int16 K_max; // Maximum magnitude bit-planes, exclucing ROI shift
    kdu_int16 K_max_prime; // Maximum magnitude bit-planes, including ROI shift
    bool reversible;
    bool initialized; // True once line buffers allocated in first `push' call
    kdu_byte alignment_offset; // See below
    kdu_byte num_jobs_per_row; // Num parallel block encoding jobs per row
    float delta; // For irreversible compression only.
    float msb_wmse; // Normalized weighted MSE associated with first mag bit
    float roi_weight; // Multiply `msb_wmse' by this for ROI foreground blocks
    kdu_dims block_indices; // Range of block indices -- fixed by `init'.
    int subband_rows, subband_cols;
    kdu_uint16 secondary_seq; // See below
    kdu_int16 first_block_width;
    kdu_int16 nominal_block_width;
    kdu_int16 nominal_block_height;
    kdu_int16 buffer_height; // > `nominal_block_height' if double buffered
    kdu_int16 next_free_line; // idx in `lines' of next line targeted by `push'
    kdu_int16 free_line_lim; // 1 + idx of last free line available to `push'
    int push_block_row; // Identifies the row of code-blocks `push' is using
    kdu_sample_allocator *allocator;
    kdu_roi_node *roi_node;
    kdu_sample16 **lines16; // NULL or array of `nominal_block_height' pointers
    kdu_sample32 **lines32; // NULL or array of `nominal_block_height' pointers
    kdu_byte **roi_lines; // NULL or array of `nominal_block_height' pointers
    kdu_thread_queue *queue; // Used only for multi-threaded processing
  };
  /* Notes:
        Only one of the `lines16' and `lines32' members may be non-NULL,
     depending on whether the transform will be pushing 16- or 32-bit sample
     values to us.  The relevant array is pre-created during construction and
     actually allocated from the single block of memory associated with the
     `allocator' object during the first `push' call.  Similar considerations
     apply to the `roi_lines' array, which is used only if `roi_source' is
     non-NULL.
        The various line buffers may support either single buffering or
     double buffering.  In the former case,
     `buffer_height' <= `nominal_block_height' and only one buffer "bank"
     is used (see below).  Double buffering is implicitly identified by
     the case `buffer_height' > `nominal_block_height', regardless of
     the height of the first row of code-blocks in a subband.  Double
     buffering is of interest only in conjunction with multi-threading
     (i.e., a non-NULL `queue').  Since double buffering is costly, we
     do not apply it to all subbands.  A typical strategy is to double
     buffer only one subband in each resolution level.
        The `secondary_seq' member is passed to `kdu_thread_env::add_jobs'.
     It is non-zero if and only if we are using double buffering.  In that
     case, the value is set to 1 + the resolution level to which this
     subband belongs (LL subband corresponds to resolution level 0).
        The various line buffers are organized into either one or two
     "banks"; since the single buffering case is obvious, we consider here
     only the double buffered scenario, in which there are two banks,
     identified as Bank 0 and Bank 1.  Bank 0 always starts at
     line 0, while bank 1 starts at line `nominal_block_height'.
        The `do_job' function uses its `job_idx' argument to figure out
     which bank its code-block jobs belong to, together with the
     code-blocks to be processed within that bank, noting that each
     code-block row is partitioned into `num_jobs_per_row' jobs.  The
     particular code-block indices of relevance can be computed from the
     `block_indices' range (this does not evolve over time), together with
     the job index.  In this way, the only dynamic quantity which is
     used by `do_job' to locate and dimension its source data is the job
     index itself.  It is worth emphasizing that the `subband_rows' counter
     is neither accessed nor updated by the `do_job' function.
        The `next_free_line' and `free_line_lim' members identify the
     row (within the various line buffers) to be used by the next call
     to `push', along with an exclusive upper bound on this index.  The
     interval [`next_free_line',`free_line_lim') inevitably corresponds
     to a subset of one bank.  Once `next_free_line' reaches
     `free_line_lim', we have completed a new bank of pushed subband
     samples.  When a call to `push' produces this condition, the function
     does not return until it has launched the corresponding block encoding
     jobs.  When a call to `push' encounters this condition on entry, it
     must first wait for a new bank to be freed up, by calling
     `kdu_thread_entity::process_jobs'.
        The `alignment_offset' member maintains state information between
     the constructor and the point at which the line buffers are actually
     initialized.  The allocator provides line buffers which are aligned
     on 16-byte boundaries, but what we want for optimal memory transactions
     is for the code-block boundaries to be 16-byte aligned.  The
     `alignment_offset' value gives the number of samples at the start of
     each allocated line buffer which should be skipped in order to ensure
     that code-block boundaries are 16-byte aligned.  In the event that the
     subband does not contain any horizontal code-block boundaries, this
     offset will be 0. */


/* ========================================================================= */
/*                                kdu_encoder                                */
/* ========================================================================= */

/*****************************************************************************/
/*                          kdu_encoder::kdu_encoder                         */
/*****************************************************************************/

kdu_encoder::kdu_encoder(kdu_subband band, kdu_sample_allocator *allocator,
                         bool use_shorts, float normalization,
                         kdu_roi_node *roi, kdu_thread_env *env,
                         kdu_thread_queue *env_queue)
  // In the future, we may create separate, optimized objects for each kernel.
{
  kd_encoder *enc = new kd_encoder;
  state = enc;
  enc->init(band,allocator,use_shorts,normalization,roi,env,env_queue);
}

/* ========================================================================= */
/*                               kd_encoder                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                            kd_encoder::init                               */
/*****************************************************************************/

void
  kd_encoder::init(kdu_subband band, kdu_sample_allocator *allocator,
                   bool use_shorts, float normalization,
                   kdu_roi_node *roi, kdu_thread_env *env,
                   kdu_thread_queue *env_queue)
{
  assert((this->allocator == NULL) && (this->queue == NULL));
  this->band = band;
  this->roi_node = roi;
  
  K_max = (kdu_int16) band.get_K_max();
  K_max_prime = (kdu_int16) band.get_K_max_prime();
  reversible = band.get_reversible();
  initialized = false;
  delta = band.get_delta() * normalization;
  msb_wmse = band.get_msb_wmse();
  roi_weight = 1.0F;
  bool have_roi_weight = band.get_roi_weight(roi_weight);

  kdu_dims dims;
  band.get_dims(dims);
  kdu_coords nominal_block_size, first_block_size;
  band.get_block_size(nominal_block_size,first_block_size);
  band.get_valid_blocks(block_indices);

  subband_cols = dims.size.x;
  subband_rows = dims.size.y;
  first_block_width = (kdu_int16) first_block_size.x;
  nominal_block_width = (kdu_int16) nominal_block_size.x;
  nominal_block_height = (kdu_int16) nominal_block_size.y;

  if ((env != NULL) && (subband_cols > 0) && (subband_rows > 0))
    queue = env->add_queue(this,env_queue,"block encoder");

  // Determine how to partition a row of code-blocks into encoding jobs
  num_jobs_per_row = 1;
  if ((queue != NULL) && (env->get_num_threads() > 1))
    { // See if we should split up the code-blocks on a row
      kdu_long block_row_samples = subband_cols;
      if (subband_rows < (int) nominal_block_height)
        block_row_samples *= subband_rows;
      else
        block_row_samples *= nominal_block_height;
      int num_per_row = (int)(block_row_samples / 8192);
      assert(num_per_row <= block_indices.size.x);
      if (num_per_row > 32)
        num_per_row = 32;
      else if (num_per_row < 1)
        num_per_row = 1;
      num_jobs_per_row = (kdu_byte) num_per_row;
    }

  // Determine whether or not to use double buffering, and hence buffer height
  secondary_seq = 0; // Changed further down if we settle on double buffering
  buffer_height = nominal_block_height;
  if (subband_rows <= (int) buffer_height)
    buffer_height = (kdu_int16) subband_rows;
  else if ((queue != NULL) && (env->get_num_threads() > 1))
    { // We might want to use double buffering in this case.
      int ideal_double_buffered_bands = // ceil(9/num_jobs_per_row)
                                        1 + (8/ num_jobs_per_row);      
      if (band.get_band_idx() <= ideal_double_buffered_bands)
        { // Decided to use double buffering for this subband.  Note that,
          // except for the LL subband, band indices start at 1.
          if ((subband_rows-first_block_size.y) < (int) nominal_block_height)
            buffer_height += (kdu_int16)(subband_rows-first_block_size.y);
          else
            buffer_height += nominal_block_height;
          secondary_seq = (kdu_uint16)
            (64 - band.access_resolution().get_dwt_level());
        }
    }

  // Set up counters
  next_free_line = 0;
  free_line_lim = (kdu_int16) first_block_size.y;
  push_block_row = 0;

  // Establish buffer configuration
  alignment_offset = 0;
  if (first_block_size.x < subband_cols)
    alignment_offset = (kdu_byte)
      ((use_shorts)?((-first_block_size.x) & 7):((-first_block_size.x) & 3));

  lines16 = NULL;
  lines32 = NULL;
  roi_lines = NULL;
  this->allocator = NULL;
  if (!dims)
    { subband_rows = 0; return; }
  this->allocator = allocator;
  allocator->pre_alloc(use_shorts,0,subband_cols + (int) alignment_offset,
                       buffer_height);
  if (use_shorts)
    lines16 = new kdu_sample16 *[buffer_height];
  else
    lines32 = new kdu_sample32 *[buffer_height];
  if (roi_node != NULL)
    {
      if ((K_max_prime == K_max) && !have_roi_weight)
        {
          roi_node->release();
          roi_node = NULL;
          return;
        }
      allocator->pre_alloc(true,0,(subband_cols+1)>>1,buffer_height);
      roi_lines = new kdu_byte *[buffer_height];
    }
}

/*****************************************************************************/
/*                        kd_encoder::~kd_encoder                            */
/*****************************************************************************/

kd_encoder::~kd_encoder()
{
  if (lines16 != NULL)
    delete[] lines16;
  if (lines32 != NULL)
    delete[] lines32;
  if (roi_lines != NULL)
    delete[] roi_lines;
  if (roi_node != NULL)
    roi_node->release();
}

/*****************************************************************************/
/*                             kd_encoder::push                              */
/*****************************************************************************/

void
  kd_encoder::push(kdu_line_buf &line, kdu_thread_env *env)
{
  if (line.get_width() == 0)
    return;

  assert((queue == NULL) || (env != NULL));
  if (!initialized)
    { // Allocate all lines -- they will be aligned and contiguous in memory.
      if (env != NULL)
        env->acquire_lock(KD_THREADLOCK_ALLOCATOR);
      int n;
      if (lines16 != NULL)
        for (n=0; n < buffer_height; n++)
          lines16[n] = allocator->alloc16(0,subband_cols+(int)alignment_offset)
                     + alignment_offset;
      else
        for (n=0; n < buffer_height; n++)
          lines32[n] = allocator->alloc32(0,subband_cols+(int)alignment_offset)
                     + alignment_offset;
      if (roi_lines != NULL)
        for (n=0; n < buffer_height; n++)
          roi_lines[n] = (kdu_byte *)
            allocator->alloc16(0,(subband_cols+1)>>1);
      initialized = true;
      if (env != NULL)
        env->release_lock(KD_THREADLOCK_ALLOCATOR);
    }

  assert(subband_rows > 0);

  if (next_free_line == free_line_lim)
    { // This condition can only occur when operating in multi-threaded mode.
      // It means we need to wait for some existing block encoding jobs to
      // complete, adding ourself to the list of processors to help this
      // happen as quickly as possible.
      assert(queue != NULL);
      env->process_jobs(queue); // Waits only for primary jobs; secondary
                // jobs (produced by double buffering) may remain on the queue.
      if ((free_line_lim > nominal_block_height) ||
          (buffer_height <= nominal_block_height))
        next_free_line = 0; // Use low memory bank
      else
        next_free_line = nominal_block_height; // Use high memory bank
      if (subband_rows < (int) nominal_block_height)
        free_line_lim = next_free_line + (kdu_int16) subband_rows;
      else
        free_line_lim = next_free_line + nominal_block_height;
      assert(free_line_lim <= buffer_height);
    }

  // Transfer data
  assert(line.get_width() == subband_cols);
  if (lines32 != NULL)
    memcpy(lines32[next_free_line],line.get_buf32(),
           (size_t)(subband_cols<<2));
  else
    memcpy(lines16[next_free_line],line.get_buf16(),
           (size_t)(subband_cols<<1));
  if (roi_node != NULL)
    {
      if (env != NULL)
        env->acquire_lock(KD_THREADLOCK_ROI);
      roi_node->pull(roi_lines[next_free_line],subband_cols);
      if (env != NULL)
        env->release_lock(KD_THREADLOCK_ROI);
    }

  // Update the buffer state, flushing if necessary.
  subband_rows--;
  next_free_line++;
  if (next_free_line == free_line_lim)
    { // We have completely generated a row of blocks; either encode the
      // row immediately, or queue up block encoding jobs to do it in the
      // background.
      if (queue != NULL)
        {
          env->add_jobs(queue,num_jobs_per_row,(subband_rows==0),secondary_seq);
                  // Note: in the double buffered case, new jobs are pushed
                  // onto the queue in the "secondary" service state.  Jobs
                  // from the previous memory bank which have still not been
                  // processed will automatically be elevated to the "primary"
                  // service state by this call.
          if ((secondary_seq != 0) && (push_block_row == 0))
            { // Special processing for first row of blocks; memory bank 1 is
              // still free to push in more rows.
              assert(free_line_lim <= nominal_block_height);
              next_free_line = nominal_block_height;
            }
          else
            return; // This shortcut does not update `push_block_row', but we
                    // don't need it in this case.
        }
      else
        {
          assert(num_jobs_per_row == 1);
          do_job(env,push_block_row);
          next_free_line = 0; // Memory bank 0 is now free for use.
        }
      push_block_row++;
      if (subband_rows < (int) nominal_block_height)
        free_line_lim = next_free_line + (kdu_int16) subband_rows;
      else
        free_line_lim = next_free_line + nominal_block_height;
      assert(free_line_lim <= buffer_height);
    }
}

/*****************************************************************************/
/*                            kd_encoder::do_job                             */
/*****************************************************************************/

void
  kd_encoder::do_job(kdu_thread_entity *ent, int job_idx)
{
  kdu_thread_env *env = (kdu_thread_env *) ent;
  int jobs_per_row = num_jobs_per_row;
  int blocks_remaining = block_indices.size.x;
  int offset = 0; // Horizontal offset into line buffers
  kdu_coords idx = block_indices.pos; // Index of first block to process
  kdu_sample32 **buf32 = lines32; // First buffer line to process (32-bit)
  kdu_sample16 **buf16 = lines16; // First buffer line to process (16-bit)
  kdu_byte **roi = roi_lines; // First ROI line to process

  // Start by using `job_idx' to find where we are
  int encoder_block_row = job_idx / jobs_per_row;
  if ((encoder_block_row & 1) && (buffer_height > nominal_block_height))
    { // Select memory bank 1
      if (buf32 != NULL) buf32 += nominal_block_height;
      if (buf16 != NULL) buf16 += nominal_block_height;
      if (roi != NULL) roi += nominal_block_height;
    }
  idx.y += encoder_block_row;
  assert(idx.y < (block_indices.pos.y + block_indices.size.y));
  if (jobs_per_row > 1)
    {
      int job_idx_in_row = job_idx - (encoder_block_row*jobs_per_row);
      int skip_blocks = (blocks_remaining*job_idx_in_row) / jobs_per_row;
      blocks_remaining =
        ((blocks_remaining*(job_idx_in_row+1))/jobs_per_row)-skip_blocks;
      assert(blocks_remaining > 0);
      if (skip_blocks > 0)
        {
          idx.x += skip_blocks;
          offset += first_block_width +
            (skip_blocks-1) * (int) nominal_block_width;
        }
    }

  // Now scan through the blocks to process
  kdu_coords xfer_size;
  kdu_block *block;
  kdu_uint16 estimated_slope_threshold =
    band.get_conservative_slope_threshold();

  for (; blocks_remaining > 0; blocks_remaining--,
       idx.x++, offset+=xfer_size.x)
    {
      // Open the block and make sure we have enough sample buffer storage.
      block = band.open_block(idx,NULL,env);
      int num_stripes = (block->size.y+3) >> 2;
      int num_samples = (num_stripes<<2) * block->size.x;
      assert(num_samples > 0);
      if (block->max_samples < num_samples)
        block->set_max_samples((num_samples>4096)?num_samples:4096);
      
      /* Now quantize and transfer samples to the block, observing any
         required geometric transformations. */
      xfer_size = block->size;
      assert((xfer_size.x == block->region.size.x) &&
             (xfer_size.y == block->region.size.y) &&
             (0 == block->region.pos.x) && (0 == block->region.pos.y));
      if (block->transpose)
        xfer_size.transpose();
      assert((xfer_size.x+offset) <= subband_cols);

      int m, n;
      kdu_int32 *dp, *dpp = block->sample_buffer;
      kdu_int32 or_val = 0; // Logical OR of the sample values in this block
      int row_gap = block->size.x;
      int m_start = 0, m_inc = 1, n_start=offset, n_inc = 1;
      if (block->vflip)
        { m_start += xfer_size.y-1; m_inc = -1; }
      if (block->hflip)
        { n_start += xfer_size.x-1; n_inc = -1; }

      // First transfer the sample data
      if (buf32 != NULL)
        { // Working with 32-bit data types.
          kdu_sample32 *sp, **spp = buf32+m_start;
          if (reversible)
            { // Source data is 32-bit absolute integers.
              kdu_int32 val;
              kdu_int32 upshift = 31-K_max;
              if (upshift < 0)
                { KDU_ERROR(e,1); e <<
                    KDU_TXT("Insufficient implementation "
                    "precision available for true reversible compression!");
                }
              if (!block->transpose)
                for (m=xfer_size.y; m--; spp+=m_inc, dpp+=row_gap)
                  for (sp=(*spp)+n_start, dp=dpp,
                       n=xfer_size.x; n--; dp++, sp+=n_inc)
                    {
                      val = sp->ival;
                      if (val < 0)
                        *dp = ((-val)<<upshift) | KDU_INT32_MIN;
                      else
                        *dp = val<<upshift;
                      or_val |= *dp;
                    }
              else
                for (m=xfer_size.y; m--; spp+=m_inc, dpp++)
                  for (sp=(*spp)+n_start, dp=dpp,
                       n=xfer_size.x; n--; dp+=row_gap, sp+=n_inc)
                    {
                      val = sp->ival;
                      if (val < 0)
                        *dp = ((-val)<<upshift) | KDU_INT32_MIN;
                      else
                        *dp = val<<upshift;
                      or_val |= *dp;
                    }
            }
          else
            { // Source data is true floating point values.
              float val;
              float scale = (1.0F / delta);
              if (K_max <= 31)
                scale *= (float)(1<<(31-K_max));
              else
                scale /= (float)(1<<(K_max-31)); // Can't decode all planes
              if (!block->transpose)
                for (m=xfer_size.y; m--; spp+=m_inc, dpp+=row_gap)
                  for (sp=(*spp)+n_start, dp=dpp,
                       n=xfer_size.x; n--; dp++, sp+=n_inc)
                    {
                      val = scale * sp->fval;
                      if (val < 0.0F)
                        *dp = ((kdu_int32)(-val)) | KDU_INT32_MIN;
                      else
                        *dp = (kdu_int32) val;
                      or_val |= *dp;
                    }
              else
                for (m=xfer_size.y; m--; spp+=m_inc, dpp++)
                  for (sp=(*spp)+n_start, dp=dpp,
                       n=xfer_size.x; n--; dp+=row_gap, sp+=n_inc)
                    {
                      val = scale * sp->fval;
                      if (val < 0.0F)
                        *dp = ((kdu_int32)(-val)) | KDU_INT32_MIN;
                      else
                        *dp = (kdu_int32) val;
                      or_val |= *dp;
                    }
            }
        }
      else
        { // Working with 16-bit source data.
          kdu_sample16 *sp, **spp=buf16+m_start;
          if (reversible)
            { // Source data is 16-bit absolute integers.
              kdu_int32 val;
              kdu_int32 upshift = 31-K_max;
              assert(upshift >= 0); // Otherwise should not have chosen 16 bits
              if (!block->transpose)
                for (m=xfer_size.y; m--; spp+=m_inc, dpp+=row_gap)
                  for (sp=(*spp)+n_start, dp=dpp,
                       n=xfer_size.x; n--; dp++, sp+=n_inc)
                    {
                      val = sp->ival;
                      if (val < 0)
                        *dp = ((-val)<<upshift) | KDU_INT32_MIN;
                      else
                        *dp = val<<upshift;
                      or_val |= *dp;
                    }
              else
                for (m=xfer_size.y; m--; spp+=m_inc, dpp++)
                  for (sp=(*spp)+n_start, dp=dpp,
                       n=xfer_size.x; n--; dp+=row_gap, sp+=n_inc)
                    {
                      val = sp->ival;
                      if (val < 0)
                        *dp = ((-val)<<upshift) | KDU_INT32_MIN;
                      else
                        *dp = val<<upshift;
                      or_val |= *dp;
                    }
            }
          else
            { // Source data is 16-bit fixed point integers.
              float fscale = 1.0F / (delta * (float)(1<<KDU_FIX_POINT));
              if (K_max <= 31)
                fscale *= (float)(1<<(31-K_max));
              else
                fscale /= (float)(1<<(K_max-31));
              kdu_int32 val, scale = (kdu_int32)(fscale+0.5F);
              if (!block->transpose)
                for (m=xfer_size.y; m--; spp+=m_inc, dpp+=row_gap)
                  for (sp=(*spp)+n_start, dp=dpp,
                       n=xfer_size.x; n--; dp++, sp+=n_inc)
                    {
                      val = sp->ival; val *= scale;
                      if (val < 0.0F)
                        val = (-val) | KDU_INT32_MIN;
                      *dp = val;
                      or_val |= val;
                    }
              else
                for (m=xfer_size.y; m--; spp+=m_inc, dpp++)
                  for (sp=(*spp)+n_start, dp=dpp,
                       n=xfer_size.x; n--; dp+=row_gap, sp+=n_inc)
                    {
                      val = sp->ival; val *= scale;
                      if (val < 0.0F)
                        val = (-val) | KDU_INT32_MIN;
                      *dp = val;
                      or_val |= val;
                    }
            }
        }

      // Now check to see if any ROI up-shift has been specified.  If so,
      // we need to zero out sufficient LSB's to ensure that the foreground
      // and background regions do not get confused.
      if (K_max_prime > K_max)
        {
          dpp = block->sample_buffer;
          kdu_int32 mask = ((kdu_int32)(-1)) << (31-K_max);
          if ((K_max_prime - K_max) < K_max)
            { KDU_ERROR(e,2); e <<
                KDU_TXT("You have selected too small a value for "
                "the ROI up-shift parameter.  The up-shift should be "
                "at least as large as the largest number of magnitude "
                "bit-planes in any subband; otherwise, the foreground and "
                "background regions might not be properly distinguished by "
                "the decompressor.");
            }
          if (!block->transpose)
            for (m=xfer_size.y; m--; dpp+=row_gap)
              for (dp=dpp, n=xfer_size.x; n--; dp++)
                *dp &= mask;
          else
            for (m=xfer_size.x; m--; dpp+=row_gap)
              for (dp=dpp, n=xfer_size.y; n--; dp++)
                *dp &= mask;
        }

      // Now transfer any ROI information which may be available.
      bool have_background = false; // If no background, code less bit-planes.
      bool scale_wmse = false; // If true, scale WMSE to account for shifting.
      if ((roi != NULL) && (K_max_prime != K_max))
        {
          scale_wmse = true; // Background will be shifted down at least
          dpp = block->sample_buffer;
          kdu_byte *sp, **spp=roi+m_start;
          kdu_int32 val;
          kdu_int32 downshift = K_max_prime - K_max;
          assert(downshift >= K_max);
          bool have_foreground = false; // If no foreground, downshift `or_val'
          if (!block->transpose)
            {
              for (m=xfer_size.y; m--; spp+=m_inc, dpp+=row_gap)
                for (sp=(*spp)+n_start, dp=dpp,
                     n=xfer_size.x; n--; dp++, sp+=n_inc)
                  if (*sp == 0)
                    { // Adjust background samples down.
                      have_background = true;
                      val = *dp;
                      *dp = (val & KDU_INT32_MIN)
                          | ((val & KDU_INT32_MAX) >> downshift);
                    }
                  else
                    have_foreground = true;
            }
          else
            {
              for (m=xfer_size.y; m--; spp+=m_inc, dpp++)
                for (sp=(*spp)+n_start, dp=dpp,
                     n=xfer_size.x; n--; dp+=row_gap, sp+=n_inc)
                  if (*sp == 0)
                    { // Adjust background samples down.
                      have_background = true;
                      val = *dp;
                      *dp = (val & KDU_INT32_MIN)
                          | ((val & KDU_INT32_MAX) >> downshift);
                    }
                  else
                    have_foreground = true;
            }
          if (!have_foreground)
            or_val = (or_val & KDU_INT32_MAX) >> downshift;
        }
      else if (roi != NULL)
        {
          kdu_byte *sp, **spp=roi+m_start;
          for (m=xfer_size.y; m--; spp+=m_inc)
            for (sp=(*spp)+n_start, n=xfer_size.x; n--; sp+=n_inc)
              if (*sp != 0)
                { // Treat whole block as foreground if it intersects with ROI
                  scale_wmse = true; m=0; break;
                }
        }
      else
        scale_wmse = true; // Everything belongs to foreground

      // Finally, we can encode the block.
      int K = (have_background)?K_max_prime:K_max;
      if (K > 30)
        {
          if (reversible && (K_max_prime > K_max) &&
              !block->insufficient_precision_detected)
            {
              block->insufficient_precision_detected = true;
              KDU_WARNING(w,0); w <<
                KDU_TXT("The ROI shift (`Rshift' attribute) which you "
                "are using is too large to ensure truly lossless recovery of "
                "both the foreground and the background regions, at least by "
                "Kakadu -- other compliant implementations may give up much "
                "earlier.  You might like to consider using the `Rweight' "
                "attribute instead of `Rshift' -- a 32x32 code-block size "
                "(not the default) is recommended in this case and `Rweight' "
                "should be set to around 2 to the power of the `Rshift' value "
                "you would have used.");
            }
        }
      K = (K>31)?31:K;
      or_val &= KDU_INT32_MAX;
      if (or_val == 0)
        block->missing_msbs = 31;
      else
        for (block->missing_msbs=0, or_val<<=1; or_val >= 0; or_val<<=1)
          block->missing_msbs++;
      if (block->missing_msbs >= K)
        {
          block->missing_msbs = K;
          block->num_passes = 0;
        }
      else
        {
          K -= block->missing_msbs;
          block->num_passes = 3*K-2;
        }

      double block_msb_wmse = (scale_wmse)?(msb_wmse*roi_weight):msb_wmse;
      block_encoder.encode(block,reversible,block_msb_wmse,
                           estimated_slope_threshold);
      band.close_block(block,env);
    }
}
