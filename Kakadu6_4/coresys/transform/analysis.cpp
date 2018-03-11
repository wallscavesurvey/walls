/*****************************************************************************/
// File: analysis.cpp [scope = CORESYS/TRANSFORMS]
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
   Implements the forward DWT (subband/wavelet analysis).  The implementation
uses lifting to reduce memory and processing, while keeping as much of the
implementation as possible common to both the reversible and the irreversible
processing paths.  The implementation is generic to the extent that it
supports any odd-length symmetric wavelet kernels -- although only 3 are
currently accepted by the "kdu_kernels" object.
******************************************************************************/

#include <assert.h>
#include <string.h>
#include <math.h>
#include "kdu_arch.h" // Include architecture-specific info for speed-ups.
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "kdu_kernels.h"
#include "analysis_local.h"

// Set things up for the inclusion of assembler optimized routines
// for specific architectures.  The reason for this is to exploit
// the availability of SIMD type instructions on many modern processors.
#if ((defined KDU_PENTIUM_MSVC) && (defined _WIN64))
# undef KDU_PENTIUM_MSVC
# ifndef KDU_X86_INTRINSICS
#   define KDU_X86_INTRINSICS // Use portable intrinsics instead
# endif
#endif // KDU_PENTIUM_MSVC && _WIN64

#if (defined _WIN64) && !(defined KDU_NO_MMX64)
#  define KDU_NO_MMX64
#endif // _WIN64 && !KDU_NO_MMX64

#if defined KDU_X86_INTRINSICS
#  define KDU_SIMD_OPTIMIZATIONS
#  include "x86_dwt_local.h" // For 32-bit or 64-bit builds, GCC or .NET
#elif defined KDU_PENTIUM_MSVC
#  define KDU_SIMD_OPTIMIZATIONS
#  include "msvc_dwt_mmx_local.h" // Only for 32-bit builds, MSVC or .NET
#elif defined KDU_PENTIUM_GCC
#  define KDU_SIMD_OPTIMIZATIONS
#  include "gcc_dwt_mmx_local.h" // For 32-bit or 64-bit builds in GCC
#elif defined KDU_SPARCVIS_GCC
#  define KDU_SIMD_OPTIMIZATIONS
#  include "gcc_dwt_sparcvis_local.h" // Contains asm commands in-line
#elif defined KDU_ALTIVEC_GCC
#  define KDU_SIMD_OPTIMIZATIONS
#  include "gcc_dwt_altivec_local.h" // Contains Altivec PIM code in-line
#endif


/* ========================================================================= */
/*                               kdu_analysis                                */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdu_analysis::kdu_analysis (node)                    */
/*****************************************************************************/

kdu_analysis::kdu_analysis(kdu_node node,
                           kdu_sample_allocator *allocator,
                           bool use_shorts, float normalization,
                           kdu_roi_node *roi, kdu_thread_env *env,
                           kdu_thread_queue *env_queue)
{
  kd_analysis *obj = new kd_analysis;
  state = obj;
  obj->init(node,allocator,use_shorts,normalization,roi,env,env_queue);
}

/*****************************************************************************/
/*                      kdu_analysis::kdu_analysis (resolution)              */
/*****************************************************************************/

kdu_analysis::kdu_analysis(kdu_resolution resolution,
                           kdu_sample_allocator *allocator,
                           bool use_shorts, float normalization,
                           kdu_roi_node *roi, kdu_thread_env *env,
                           kdu_thread_queue *env_queue)
{
  kd_analysis *obj = new kd_analysis;
  state = obj;
  obj->init(resolution.access_node(),allocator,use_shorts,
            normalization,roi,env,env_queue);
}


/* ========================================================================= */
/*                              kd_analysis                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                           kd_analysis::init                               */
/*****************************************************************************/

void
  kd_analysis::init(kdu_node node, kdu_sample_allocator *allocator,
                    bool use_shorts, float normalization, kdu_roi_node *roi,
                    kdu_thread_env *env, kdu_thread_queue *env_queue)
{
  kdu_resolution res = node.access_resolution();
  if ((roi != NULL) && !res.propagate_roi())
    {
      roi->release();
      roi = NULL;
    }
  if (roi != NULL)
    roi_level.create(node,roi);

  this->reversible = res.get_reversible();
  this->use_shorts = use_shorts;
  this->initialized = false;
  this->free_lines = NULL;
  this->queues = NULL;
  this->vert_source_vlines = NULL;
  this->vert_source_bufs = NULL;
  this->vert_steps = this->hor_steps = NULL;
  this->num_vert_steps = this->num_hor_steps = 0;
  this->vert_symmetric_extension = this->hor_symmetric_extension = false;
  int kernel_id = node.get_kernel_id();
  int directions = node.get_directions();
  this->horizontal_first = reversible &&
    ((directions & (KDU_NODE_DECOMP_BOTH | KDU_NODE_TRANSPOSED)) ==
     (KDU_NODE_DECOMP_BOTH | KDU_NODE_TRANSPOSED));  
  coeff_handle = NULL;
  icoeff_handle = NULL;
  source_buf_handle = NULL;
  line_handle = NULL;
  vert_step_handle = hor_step_handle = NULL;
  queue_handle = NULL;

  // Get output dimensions
  kdu_dims dims;  node.get_dims(dims);
  kdu_coords min, max;
  min = dims.pos; max = min + dims.size; max.x--; max.y--;
  y_min = min.y; y_max = max.y;
  x_min = min.x;  x_max = max.x;

  empty = !dims;
  unit_height = (y_min==y_max);
  unit_width = (x_min==x_max);

  if (empty)
    return;

  // Get basic structure
  kdu_node child[4] =
    {node.access_child(LL_BAND), node.access_child(HL_BAND),
     node.access_child(LH_BAND), node.access_child(HH_BAND)};

  assert(child[LL_BAND].exists()); // We should not be a leaf node here
  vert_xform_exists = child[LH_BAND].exists();
  hor_xform_exists = child[HL_BAND].exists();

  const kdu_kernel_step_info *vert_step_info=NULL, *hor_step_info=NULL;

  int s, n;
  int max_kernel_support = 0;
  int total_coeffs = 0;
  float vert_low_gain=1.0F, vert_high_gain=1.0F;
  float hor_low_gain=1.0F, hor_high_gain=1.0F;
  if (vert_xform_exists)
    {
      int low_min, low_max, high_min, high_max, max_step_length;
      vert_step_info =
        node.get_kernel_info(num_vert_steps,vert_low_gain,vert_high_gain,
                             vert_symmetric,vert_symmetric_extension,
                             low_min,low_max,high_min,high_max,true);
      max_kernel_support =
        ((low_max > high_max)?low_max:high_max) -
        ((low_min < high_min)?low_min:high_min);
      if (num_vert_steps <= 4)
        vert_steps = step_store;
      else
        vert_steps = vert_step_handle = new kd_lifting_step[num_vert_steps];
      for (max_step_length=0, s=0; s < num_vert_steps; s++)
        {
          vert_steps[s].support_min = (kdu_int16)
            vert_step_info[s].support_min;
          vert_steps[s].support_length = (kdu_byte)
            vert_step_info[s].support_length;
          vert_steps[s].downshift = (kdu_byte)
            vert_step_info[s].downshift;
          vert_steps[s].rounding_offset = (kdu_int16)
            vert_step_info[s].rounding_offset;
          vert_steps[s].kernel_id = (kdu_byte) kernel_id;
          vert_steps[s].reversible = reversible;
          if (max_step_length < (int)(vert_step_info[s].support_length))
            max_step_length = vert_step_info[s].support_length;
          total_coeffs += vert_step_info[s].support_length;
        }
      if (max_step_length <= 4)
        vert_source_bufs = source_buf_store;
      else
        vert_source_bufs = source_buf_handle = new void *[2*max_step_length];
      vert_source_vlines =
        (kd_vlift_line **)(vert_source_bufs+max_step_length);
    }
  if (hor_xform_exists)
    {
      int low_min, low_max, high_min, high_max;
      hor_step_info =
        node.get_kernel_info(num_hor_steps,hor_low_gain,hor_high_gain,
                             hor_symmetric,hor_symmetric_extension,
                             low_min,low_max,high_min,high_max,false);
      if (hor_step_info == vert_step_info)
        hor_steps = vert_steps;
      else if ((num_hor_steps+num_vert_steps) <= 4)
        hor_steps = step_store + num_vert_steps;
      else
        hor_steps = hor_step_handle = new kd_lifting_step[num_hor_steps];
      if (hor_steps != vert_steps)
        for (s=0; s < num_hor_steps; s++)
          {
            hor_steps[s].support_min = (kdu_int16)
              hor_step_info[s].support_min;
            hor_steps[s].support_length = (kdu_byte)
              hor_step_info[s].support_length;
            hor_steps[s].downshift = (kdu_byte)
              hor_step_info[s].downshift;
            hor_steps[s].rounding_offset = (kdu_int16)
              hor_step_info[s].rounding_offset;
            hor_steps[s].kernel_id = (kdu_byte) kernel_id;
            hor_steps[s].reversible = reversible;
            total_coeffs += hor_step_info[s].support_length;
          }

      coset_width[0] = ((max.x+2)>>1) - ((min.x+1)>>1);
      coset_width[1] = ((max.x+1)>>1) - (min.x>>1);
    }
  else
    {
      coset_width[0] = max.x+1-min.x;
      coset_width[1] = 0;
    }

  // Next, allocate coefficient storage and initialize lifting steps
  float *coeff_next = coeff_store;
  int *icoeff_next = icoeff_store;
  if (total_coeffs > 8)
    {
      coeff_next = coeff_handle = new float[total_coeffs];
      icoeff_next = icoeff_handle = new int[total_coeffs];
    }
  if (vert_xform_exists)
    {
      const float *coefficients = node.get_kernel_coefficients(true);
      for (s=0; s < num_vert_steps; s++)
        {
          kd_lifting_step *step = vert_steps + s;
          step->step_idx = (kdu_byte) s;

          step->coeffs = coeff_next;
          step->icoeffs = icoeff_next;
          step->add_shorts_first = false;
#ifdef KDU_SIMD_OPTIMIZATIONS
          if ((kernel_id == Ckernels_W9X7) && (s != 1) &&
              ((kdu_mmx_level>0)||kdu_sparcvis_exists||kdu_altivec_exists))
            step->add_shorts_first = true;
#endif // KDU_SIMD_OPTIMIZATIONS
          float fact, max_factor = 0.4F;
          for (n=0; n < step->support_length; n++)
            {
              step->coeffs[n] = fact = coefficients[n];
              if (fact > max_factor)
                max_factor = fact;
              else if (fact < -max_factor)
                max_factor = -fact;
            }
          if (!reversible)
            { // Find appropriate downshift and rounding_offset values
              for (step->downshift=16; max_factor >= 0.499F; max_factor*=0.5F)
                step->downshift--;
              if (step->downshift > 15)
                step->rounding_offset = KDU_INT16_MAX;
              else
                step->rounding_offset = (kdu_int16)(1<<(step->downshift-1));
            }
          fact = (float)(1<<step->downshift);
          for (n=0; n < step->support_length; n++)
            step->icoeffs[n] = (int) floor(0.5 + step->coeffs[n] * fact);
          coeff_next += step->support_length;
          icoeff_next += step->support_length;
          coefficients += step->support_length;

          step->extend = 0; // Not used in the vertical direction
        }
    }

  int max_extend=0;
  if (hor_xform_exists)
    {
      const float *coefficients = node.get_kernel_coefficients(false);
      for (s=0; s < num_hor_steps; s++)
        {
          kd_lifting_step *step = hor_steps + s;
          if (hor_steps != vert_steps)
            {
              step->step_idx = (kdu_byte) s;
              step->coeffs = coeff_next;
              step->icoeffs = icoeff_next;
              step->add_shorts_first = false;
#ifdef KDU_SIMD_OPTIMIZATIONS
              if ((kernel_id == Ckernels_W9X7) && (s != 1) &&
                  ((kdu_mmx_level>0)||kdu_sparcvis_exists||kdu_altivec_exists))
                step->add_shorts_first = true;
#endif // KDU_SIMD_OPTIMIZATIONS
              float fact, max_factor = 0.4F;
              for (n=0; n < step->support_length; n++)
                {
                  step->coeffs[n] = fact = coefficients[n];
                  if (fact > max_factor)
                    max_factor = fact;
                  else if (fact < -max_factor)
                    max_factor = -fact;
                }
              if (!reversible)
                { // Find appropriate downshift and rounding_offset values
                  for (step->downshift=16;
                       max_factor >= 0.499F; max_factor*=0.5F)
                    step->downshift--;
                  if (step->downshift > 15)
                    step->rounding_offset = KDU_INT16_MAX;
                  else
                    step->rounding_offset = (kdu_int16)(1<<(step->downshift-1));
                }
              fact = (float)(1<<step->downshift);
              for (n=0; n < step->support_length; n++)
                step->icoeffs[n] = (int) floor(0.5 + step->coeffs[n] * fact);
              coeff_next += step->support_length;
              icoeff_next += step->support_length;
            }
          else
            { // Horizontal and vertical lifting steps should be identical
              for (n=0; n < step->support_length; n++)
                assert(step->coeffs[n] == coefficients[n]);
            }

          coefficients += step->support_length;

          int extend_left = -step->support_min;
          int extend_right = step->support_min-1 + (int) step->support_length;
          if (x_min & 1)
            extend_left += (s & 1)?(-1):1;
          if (!(x_max & 1))
            extend_right += (s & 1)?1:(-1);
          int extend = (extend_left > extend_right)?extend_left:extend_right;
          extend = (extend < 0)?0:extend;
          if (extend > max_extend)
            max_extend = extend;
          assert(extend < 256);
          step->extend = (kdu_byte) extend;
        }
    }

  // Create line queues
  if ((num_vert_steps > 0) && !unit_height)
    {
      if (num_vert_steps <= 4)
        queues = queue_store;
      else
        queues = queue_handle = new kd_vlift_queue[num_vert_steps+1];
      queues += 1; // So we can access first queue with index -1
    }

  // Create `step_next_row_pos' array
  step_next_row_pos = NULL;
  if (num_vert_steps > 0)
    {
      if (num_vert_steps <= 4)
        step_next_row_pos = next_row_pos_store;
      else
        step_next_row_pos = next_row_pos_handle = new int[num_vert_steps+1];
    }

  // Find allocation parameters for internal line buffers
  int buf_neg_extent = max_extend;
  int buf_pos_extent = coset_width[0];
  if (buf_pos_extent < coset_width[1])
    buf_pos_extent = coset_width[1];
  buf_pos_extent += max_extend;

  // Simulate vertical lifting process to find number of line buffers required
  int required_line_buffers = 1; // Need this if no vertical transform
  if ((num_vert_steps > 0) && !unit_height)
    required_line_buffers = simulate_vertical_lifting(max_kernel_support);
  if (required_line_buffers > 6)
    line_handle = new kd_vlift_line[required_line_buffers];
  for (free_lines=NULL, n=0; n < required_line_buffers; n++)
    {
      if (line_handle == NULL)
        { line_store[n].next = free_lines;  free_lines = line_store + n; }
      else
        { line_handle[n].next = free_lines; free_lines = line_handle + n; }
      free_lines->pre_create(allocator,coset_width[0],coset_width[1],
                             reversible,use_shorts,
                             buf_neg_extent,buf_pos_extent,hor_xform_exists);
    }

  // Set up vertical counters and queues
  y_next = y_min;
  if (queues != NULL)
    for (s=-1; s < num_vert_steps; s++)
      {
        int max_source_request_idx = y_max - ((y_max ^ s) & 1);
        if (s >= 0)
          max_source_request_idx = y_max - ((y_max ^ s) & 1) +
            2*(vert_steps[s].support_min - 1 +
               (int) vert_steps[s].support_length);
        queues[s].init(y_min,y_max,s,vert_symmetric_extension,
                       max_source_request_idx);
        if ((s >= 0) && (vert_steps[s].support_length <= 0))
          queues[s].source_done();
      }
  if (vert_xform_exists)
    for (s=0; s < (((int) num_vert_steps)+1); s++)
      step_next_row_pos[s] = y_min + 1 - ((y_min ^ s) & 1);

  // Now determine the normalizing downshift and subband nominal ranges.
  float child_ranges[4] =
    {normalization, normalization, normalization, normalization};
  normalizing_downshift = 0;
  if (!reversible)
    {
      int ns;
      const float *vert_bibo_gains = node.get_bibo_gains(ns,true);
      assert(ns == num_vert_steps);
      const float *hor_bibo_gains = node.get_bibo_gains(ns,false);
      assert(ns == num_hor_steps);

      float bibo_max = normalization; // Not a true BIBO gain; this value just
                                       // means leave normalization alone.

      // Find BIBO gains from vertical analysis steps (these are done first)
      if ((num_vert_steps > 0) && !unit_height)
        {
          child_ranges[LL_BAND] /= vert_low_gain;
          child_ranges[HL_BAND] /= vert_low_gain;
          child_ranges[LH_BAND] /= vert_high_gain;
          child_ranges[HH_BAND] /= vert_high_gain;

          float bibo_prev, bibo_in, bibo_out;

          // Find cumulative horizontal gain to input of node.
          bibo_prev = hor_bibo_gains[0] * normalization;
          for (bibo_in=bibo_prev, n=0; n < num_vert_steps; n++,
               bibo_in=bibo_out)
            {
              bibo_out = bibo_prev * vert_bibo_gains[n+1];
              if (bibo_out > bibo_max)
                bibo_max = bibo_out; // Fit representation to lifting outputs
              if (use_shorts && vert_steps[n].add_shorts_first)
                {
                  bibo_in *= 2.0; // Avoid overflow during pre-accumulation
                  if (bibo_in > bibo_max)
                    bibo_max = bibo_in;
                }
            }
        }

      // Find BIBO gains from horizonal analysis steps
      if ((num_hor_steps > 0) && !unit_width)
        {
          child_ranges[LL_BAND] /= hor_low_gain;
          child_ranges[HL_BAND] /= hor_high_gain;
          child_ranges[LH_BAND] /= hor_low_gain;
          child_ranges[HH_BAND] /= hor_high_gain;

          float bibo_prev, bibo_in, bibo_out;

          // Find maximum gain at output of vertical decomposition, if any
          bibo_prev = vert_bibo_gains[num_vert_steps];
          if ((num_vert_steps > 0) &&
              (vert_bibo_gains[num_vert_steps-1] > bibo_prev))
            bibo_prev = vert_bibo_gains[num_vert_steps-1];
          bibo_prev *= normalization; // Account for fact that gains reported
                   // by `kdu_node::get_bibo_gains' assume `normalization'=1

          for (bibo_in=bibo_prev, n=0; n < num_hor_steps; n++,
               bibo_in=bibo_out)
            {
              bibo_out = bibo_prev * hor_bibo_gains[n+1];
              if (bibo_out > bibo_max)
                bibo_max = bibo_out; // Fit representation to lifting outputs
              if (use_shorts && hor_steps[n].add_shorts_first)
                {
                  bibo_in *= 2.0; // Avoid overflow during pre-accumulation
                  if (bibo_in > bibo_max)
                    bibo_max = bibo_in;
                }
            }
        }

      if (use_shorts)
        {
          float overflow_limit = 1.0F * (float)(1<<(16-KDU_FIX_POINT));
            // This is the largest numeric range which can be represented in
            // our signed 16-bit fixed-point representation without overflow.
          while (bibo_max > 0.95*overflow_limit)
            { // Leave a little extra headroom to allow for approximations in
              // the numerical BIBO gain calculations.
              normalizing_downshift++;
              for (int b=0; b < 4; b++)
                child_ranges[b] *= 0.5F;
              bibo_max *= 0.5;
            }
        }
    }

  // Finally, create the subband interfaces recursively.
  for (n=0; n < 4; n++)
    {
      if (!child[n])
        continue;
      kdu_roi_node *roi_child = NULL;
      if (roi != NULL)
        roi_child = roi_level.acquire_node(n);
      if (!child[n].access_child(LL_BAND))
        subbands[n] = kdu_encoder(child[n].access_subband(),allocator,
                                  use_shorts,child_ranges[n],roi_child,
                                  env,env_queue);
      else
        subbands[n] = kdu_analysis(child[n],allocator,use_shorts,
                                   child_ranges[n],roi_child,
                                   env,env_queue);
    }
}

/*****************************************************************************/
/*                        kd_analysis::~kd_analysis                          */
/*****************************************************************************/

kd_analysis::~kd_analysis()
{
  int n;
  for (n=0; n < 4; n++)
    if (subbands[n].exists())
      subbands[n].destroy();
  if (roi_level.exists())
    roi_level.destroy(); // Important to do this last, giving descendants a
                         // chance to call the `release' function on their
                         // `roi_node' interfaces.
  if (coeff_handle != NULL)
    delete[] coeff_handle;
  if (icoeff_handle != NULL)
    delete[] icoeff_handle;
  if (source_buf_handle != NULL)
    delete[] source_buf_handle;
  if (line_handle != NULL)
    delete[] line_handle;
  if (vert_step_handle != NULL)
    delete[] vert_step_handle;
  if (hor_step_handle != NULL)
    delete[] hor_step_handle;
  if (queue_handle != NULL)
    delete[] queue_handle;
  if (next_row_pos_handle != NULL)
    delete[] next_row_pos_handle;
}

/*****************************************************************************/
/*                  kd_analysis::simulate_vertical_lifting                   */
/*****************************************************************************/

int
  kd_analysis::simulate_vertical_lifting(int support_max)
{
  int s, s_max, src_idx;
  int max_used_lines=0, used_lines=0;
  kd_lifting_step *step;

  // Find simple upper bounds to avoid wasting too much simulation effort
  int sim_y_max = y_max;
  int delta = sim_y_max - (y_min + support_max + 2);
  if (delta > 0)
    sim_y_max -= delta & ~1;

  // Set up counters for simulation
  y_next = y_min;
  for (s=-1; s < num_vert_steps; s++)
    {
      int max_source_request_idx = sim_y_max - ((sim_y_max ^ s) & 1);
      if (s >= 0)
        max_source_request_idx = sim_y_max - ((sim_y_max ^ s) & 1) +
          2*(vert_steps[s].support_min - 1 +
             (int) vert_steps[s].support_length);
      queues[s].init(y_next,sim_y_max,s,vert_symmetric_extension,
                     max_source_request_idx);
      if ((s >= 0) && (vert_steps[s].support_length <= 0))
        queues[s].source_done();
    }
  for (s=0; s <= num_vert_steps; s++)
    step_next_row_pos[s] = y_min + 1 - ((y_min ^ s) & 1);

  // Run the simulator
  for (; y_next <= sim_y_max; y_next++)
    { // Simulate lines being pushed in one by one
      used_lines++; // Simulate getting new input line from the free list
      if (used_lines > max_used_lines)
        max_used_lines = used_lines;
      queues[-(y_next & 1)].simulate_push_line(y_next,used_lines);

      s_max = 1-(y_next&1); // Most advanced lifting step which might
          // now be able to advance for the first time, assuming no earlier
          // ones can advance
      bool generated_something = true;
      while (generated_something)
        {
          generated_something = false;
          for (s=0; (s <= s_max) && (s < num_vert_steps); s++)
            {
              step = vert_steps + s;
              src_idx = (step_next_row_pos[s] ^ 1) + 2*step->support_min;
              if (!queues[s-1].test_update(step_next_row_pos[s],false))
                continue; // Can't access line this step needs to update
              if ((step->support_length > 0) &&
                  !queues[s].simulate_access_source(src_idx,
                                            step->support_length,used_lines))
                continue; // Can't access all source lines required by step
              queues[s-1].simulate_access_update(step_next_row_pos[s],
                                                 used_lines);
              used_lines++; // Simulate getting new line from the free list
              if (used_lines > max_used_lines)
                max_used_lines = used_lines;
              if (s == (num_vert_steps-1))
                { // Last lifting step may have produced a result
                  used_lines--; // Put the output line back on the free list
                }
              else
                {
                  queues[s+1].simulate_push_line(step_next_row_pos[s],
                                                 used_lines);
                  s_max = s+2;
                }
              step_next_row_pos[s] += 2;
              generated_something = true;
              if (step_next_row_pos[s] > sim_y_max)
                queues[s].source_done();
            }
          // Finish off by seeing if we can pull an output line from the
          // vertical subband which is used as the source for the last
          // lifting step -- we need to wait until the line is no longer
          // required as a lifting step source, since we will be doing
          // in-place horizontal lifting on it.
          s = num_vert_steps;
          if (queues[s-1].test_update(step_next_row_pos[s],true))
            {
              queues[s-1].simulate_access_update(step_next_row_pos[s],
                                                 used_lines);
              step_next_row_pos[s] += 2;
              generated_something = true;
            }
        } // End of `while (generated_something)' loop
    }

  return max_used_lines;
}

/*****************************************************************************/
/*                            kd_analysis::push                              */
/*****************************************************************************/

void
  kd_analysis::push(kdu_line_buf &line, kdu_thread_env *env)
{
  assert(y_next <= y_max);
  assert(reversible == line.is_absolute());
  if (empty)
    { // in case horizontal extent is 0, but vertical extent non-zero
      y_next++;
      return;
    }

  kd_vlift_line *vline_in, *vline_out;
  if (!initialized)
    { // Finish creating all the buffers.
      if (env != NULL)
        env->acquire_lock(KD_THREADLOCK_ALLOCATOR);
      for (vline_in=free_lines; vline_in != NULL; vline_in=vline_in->next)
        vline_in->create();
      initialized = true;
      if (env != NULL)
        env->release_lock(KD_THREADLOCK_ALLOCATOR);
    }

  // Get new line buffer and push it into the relevant input queue, if there
  // is a vertical transform.
  vline_in = free_lines;   assert(vline_in != NULL);
  kd_vlift_queue *queue = NULL;
  int vsub_parity = (vert_xform_exists)?(y_next & 1):0; // vert subband parity
  if ((num_vert_steps > 0) && !unit_height)
    {
      free_lines = vline_in->next;  vline_in->next = NULL;
      queue = queues - vsub_parity;
      queue->push_line(y_next,vline_in,free_lines);
    }
  y_next++;

  // Copy the samples from `line' to `vline', de-interleaving even and odd
  // sub-sequences and supplying any normalizing downshifts, as required.
  assert(line.get_width() == (coset_width[0]+coset_width[1]));
  int c = x_min & 1; // Index of first coset to be de-interleaved.
  int k = (line.get_width()+1)>>1; // May move one extra sample.
  if (!hor_xform_exists)
    { // Just transfer contents to coset[0]
      if (!use_shorts)
        { // Working with 32-bit data
          kdu_sample32 *sp = line.get_buf32();
          kdu_sample32 *dp = vline_in->cosets[0].get_buf32();
          assert(normalizing_downshift == 0);
          for (; k--; sp+=2, dp+=2)
            { dp[0] = sp[0];  dp[1] = sp[1]; }
        }
      else
        { // Working with 16-bit data
          kdu_sample16 *sp = line.get_buf16();
          kdu_sample16 *dp = vline_in->cosets[0].get_buf16();
          if (normalizing_downshift == 0)
            for (; k--; sp+=2, dp+=2)
              { dp[0] = sp[0]; dp[1] = sp[1]; }
          else
            {
              kdu_int16 offset = (kdu_int16)((1<<normalizing_downshift)>>1);
              for (; k--; sp+=2, dp+=2)
                {
                  dp[0].ival = (sp[0].ival+offset) >> normalizing_downshift;
                  dp[1].ival = (sp[1].ival+offset) >> normalizing_downshift;
                }
            }
        }
    }
  else
    { // De-interleave into low- and high-pass cosets
      if (!use_shorts)
        { // Working with 32-bit data
          kdu_sample32 *sp = line.get_buf32();
          kdu_sample32 *dp1 = vline_in->cosets[c].get_buf32();
          kdu_sample32 *dp2 = vline_in->cosets[1-c].get_buf32();
          assert(normalizing_downshift == 0);
          for (; k--; sp+=2, dp1++, dp2++)
            { *dp1 = sp[0]; *dp2 = sp[1]; }
        }
      else
        { // Working with 16-bit data
          kdu_sample16 *sp = line.get_buf16();
          kdu_sample16 *dp1 = vline_in->cosets[c].get_buf16();
          kdu_sample16 *dp2 = vline_in->cosets[1-c].get_buf16();
          if (normalizing_downshift == 0)
            {
#ifdef KDU_SIMD_OPTIMIZATIONS
              if (!simd_deinterleave(&(sp->ival),&(dp1->ival),&(dp2->ival),k))
#endif // KDU_SIMD_OPTIMIZATIONS
                for (; k--; sp+=2, dp1++, dp2++)
                  { *dp1 = sp[0]; *dp2 = sp[1]; }
            }
          else
            {
#ifdef KDU_SIMD_OPTIMIZATIONS
              if (!simd_downshifted_deinterleave(&(sp->ival),&(dp1->ival),
                                      &(dp2->ival),k,normalizing_downshift))
#endif // KDU_SIMD_OPTIMIZATIONS
                {
                  kdu_int16 offset=(kdu_int16)((1<<normalizing_downshift)>>1);
                  for (; k--; sp+=2, dp1++, dp2++)
                    {
                      dp1->ival = (sp[0].ival+offset) >> normalizing_downshift;
                      dp2->ival = (sp[1].ival+offset) >> normalizing_downshift;
                    }
                }
            }
        }
    }

  // Now perform the transform
  if (unit_height && reversible && vsub_parity)
    { // Reversible transform of a single odd-indexed sample -- double int vals
      assert(vert_xform_exists);
      if (!use_shorts)
        { // Working with 32-bit data
          kdu_sample32 *dp;
          for (c=0; c < 2; c++)
            for (dp=vline_in->cosets[c].get_buf32(),
                 k=vline_in->cosets[c].get_width(); k--; dp++)
              dp->ival <<= 1;
        }
      else
        { // Working with 16-bit data
          kdu_sample16 *dp;
          for (c=0; c < 2; c++)
            for (dp=vline_in->cosets[c].get_buf16(),
                 k=vline_in->cosets[c].get_width(); k--; dp++)
              dp->ival <<= 1;
        }
    }

  if (queue == NULL)
    horizontal_analysis(vline_in,vsub_parity,env,false);
  else
    { // Need to perform the vertical transform
      if (horizontal_first)
        horizontal_analysis(vline_in,vsub_parity,env,true);
      kd_lifting_step *step;
      assert((num_vert_steps > 0) && !unit_height);
      int src_idx, s;
      int s_max = 1-vsub_parity; // Most advanced lifting step which might
          // now be able to advance for the first time, assuming no earlier
          // ones can advance
      bool generated_something = true;
      while (generated_something)
        {
          generated_something = false;
          for (s=0; (s <= s_max) && (s < num_vert_steps); s++)
            {
              step = vert_steps + s;
              vsub_parity = 1 - (s & 1); // Parity of output line for this step
              src_idx = (step_next_row_pos[s] ^ 1) + 2*step->support_min;
              vline_in = vline_out = NULL;
              if (!queues[s-1].test_update(step_next_row_pos[s],false))
                continue; // Can't access line this step needs to update
              if ((step->support_length > 0) &&
                  !queues[s].access_source(src_idx,step->support_length,
                                           vert_source_vlines,free_lines))
                continue; // Can't access all source lines required by step
              vline_in =
                queues[s-1].access_update(step_next_row_pos[s],free_lines);
              vline_out = free_lines;  assert(vline_out != NULL);
              if (step->support_length <= 0)
                {
                  if (use_shorts)
                    for (c=0; c < 2; c++)
                      memcpy(vline_out->cosets[c].get_buf16(),
                             vline_in->cosets[c].get_buf16(),
                             (size_t)(coset_width[c]<<1));
                  else
                    for (c=0; c < 2; c++)
                      memcpy(vline_out->cosets[c].get_buf32(),
                             vline_in->cosets[c].get_buf32(),
                             (size_t)(coset_width[c]<<2));
                }
              else
                {
                  int k;
                  for (c=0; c < 2; c++)
                    {
                      if (coset_width[c] == 0)
                        continue;
                      if (use_shorts)
                        {
                          kdu_sample16 **bf=(kdu_sample16 **) vert_source_bufs;
                          for (k=0; k < (int) step->support_length; k++)
                            bf[k]=vert_source_vlines[k]->cosets[c].get_buf16();
                          perform_analysis_lifting_step(step,bf,
                                      vline_in->cosets[c].get_buf16(),
                                      vline_out->cosets[c].get_buf16(),
                                      coset_width[c],0);
                        }
                      else
                        {
                          kdu_sample32 **bf=(kdu_sample32 **) vert_source_bufs;
                          for (k=0; k < (int) step->support_length; k++)
                            bf[k]=vert_source_vlines[k]->cosets[c].get_buf32();
                          perform_analysis_lifting_step(step,bf,
                                      vline_in->cosets[c].get_buf32(),
                                      vline_out->cosets[c].get_buf32(),
                                      coset_width[c],0);
                        }
                    }
                }

              if (s == (num_vert_steps-1))
                { // Last lifting step produced a result
                  horizontal_analysis(vline_out,vsub_parity,env,false);
                }
              else
                { // Output from this step is pushed into next input queue
                  free_lines = vline_out->next; // Strip it from free list
                  vline_out->next = NULL;
                  queues[s+1].push_line(step_next_row_pos[s],vline_out,
                                        free_lines);
                  s_max = s + 2;
                }

              generated_something = true;
              step_next_row_pos[s] += 2;
              if (step_next_row_pos[s] > y_max)
                queues[s].source_done();
            }
          // Finish off by seeing if we can pull an output line from the
          // vertical subband which is used as the lifting step source for
          // the last lifting step
          s = num_vert_steps;
          if (queues[s-1].test_update(step_next_row_pos[s],true))
            {
              vline_out =
                queues[s-1].access_update(step_next_row_pos[s],free_lines);
              assert(vline_out != NULL);
              step_next_row_pos[s] += 2;
              generated_something = true;
              horizontal_analysis(vline_out,(s+1)&1,env,false);
            }
        } // End of `while(generated_something)' loop
    }
}

/*****************************************************************************/
/*                    kd_analysis::horizontal_analysis                       */
/*****************************************************************************/

void
  kd_analysis::horizontal_analysis(kd_vlift_line *line, int vert_parity,
                                   kdu_thread_env *env, bool pre_vertical)
{
  assert((coset_width[0] == line->cosets[0].get_width()) &&
         (coset_width[1] == line->cosets[1].get_width()));

  int s, c, k, width;

  if ((num_hor_steps == 0) || unit_width)
    { // No transformation required
      if (!pre_vertical)
        { 
          if (unit_width && (num_hor_steps > 0) && reversible && (x_min & 1))
            { // Double odd-indexed line sample before writing out 
              assert(hor_xform_exists);
              if (!use_shorts)
                line->cosets[1].get_buf32()->ival <<= 1;
              else
                line->cosets[1].get_buf16()->ival <<= 1;
            }
          for (c=0; c < 2; c++)
            if (coset_width[c] > 0)
              subbands[2*vert_parity+c].push(line->cosets[c],env);
        }
      return;
    }
  
  if (horizontal_first && !pre_vertical)
    { // We have already done the horizontal transform; just push samples out
      for (c=0; c < 2; c++)
        subbands[2*vert_parity+c].push(line->cosets[c],env);
      return;
    }
  
  // If we get here, we need to do the horizontal transform
  for (s=0; s < num_hor_steps; s++)
    { 
      kd_lifting_step *step = hor_steps + s;
      if (step->support_length == 0)
        continue;
      c = 1 - (s & 1); // Coset which is updated by this lifting step
      width = coset_width[c];
      
      if (use_shorts)
        { // Processing 16-bit samples
          kdu_sample16 *sp=line->cosets[1-c].get_buf16();
          kdu_sample16 *dp=line->cosets[c].get_buf16();
          kdu_sample16 *esp=sp+coset_width[1-c]-1; // End of source buffer
          if (!hor_symmetric_extension)
            for (k=1; k <= step->extend; k++)
              { sp[-k] = sp[0];  esp[k] = esp[0]; }
          else
            for (k=1; k <= step->extend; k++)
              { // Apply symmetric extension policy
                sp[-k] = sp[k - ((x_min^s) & 1)];
                esp[k] = esp[-k + ((x_max^s) & 1)];
              }
          if (x_min & 1)
            sp -= c+c-1;
          sp += step->support_min;
          
#ifdef KDU_SIMD_OPTIMIZATIONS
          if ((step->kernel_id == (kdu_byte) Ckernels_W5X3) &&
              simd_W5X3_h_analysis(&(sp->ival),&(dp->ival),width,step))
            continue;
          else if ((step->kernel_id == (kdu_byte) Ckernels_W9X7) &&
                   simd_W9X7_h_analysis(&(sp->ival),&(dp->ival),
                                        width,step))
            continue;
#endif // KDU_SIMD_OPTIMIZATIONS
          if ((step->support_length==2) &&
              (step->icoeffs[0]==step->icoeffs[1]))
            { // Special case for symmetric least-dissimilar filters
              kdu_int32 downshift = step->downshift;
              kdu_int32 offset = step->rounding_offset;
              kdu_int32 val, i_lambda = step->icoeffs[0];
              if (i_lambda == 1)
                for (k=0; k < width; k++)
                  { val = sp[k].ival;  val += sp[k+1].ival;
                    dp[k].ival += (kdu_int16)((offset+val)>>downshift); }
              else if (i_lambda == -1)
                for (k=0; k < width; k++)
                  { val = sp[k].ival;  val += sp[k+1].ival;
                    dp[k].ival += (kdu_int16)((offset-val)>>downshift); }
              else
                for (k=0; k < width; k++)
                  { val = sp[k].ival;  val += sp[k+1].ival;
                    dp[k].ival +=
                      (kdu_int16)((offset+i_lambda*val)>>downshift); }
            }
          else
            { // Generic 16-bit implementation
              int t, support=step->support_length;
              kdu_int32 downshift = step->downshift;
              kdu_int32 offset = step->rounding_offset;
              int sum, *cpp = step->icoeffs;
              for (k=0; k < width; k++, sp++)
                { 
                  for (sum=offset, t=0; t < support; t++)
                    sum += cpp[t] * sp[t].ival;
                  dp[k].ival += (kdu_int16)(sum >> downshift);
                }
            }
        }
      else
        { // Processing 32-bit samples
          kdu_sample32 *sp=line->cosets[1-c].get_buf32();
          kdu_sample32 *dp=line->cosets[c].get_buf32();
          kdu_sample32 *esp=sp+coset_width[1-c]-1; // End of source buffer
          if (!hor_symmetric_extension)
            for (k=1; k <= step->extend; k++)
              { sp[-k] = sp[0];  esp[k] = esp[0]; }
          else
            for (k=1; k <= step->extend; k++)
              { // Apply symmetric extension policy
                sp[-k] = sp[k - ((x_min^s) & 1)];
                esp[k] = esp[-k + ((x_max^s) & 1)];
              }
          if (x_min & 1)
            sp -= c+c-1;
          sp += step->support_min;
          
#ifdef KDU_SIMD_OPTIMIZATIONS
          if ((step->kernel_id == (kdu_byte) Ckernels_W5X3) &&
              simd_W5X3_h_analysis32(&(sp->ival),&(dp->ival),width,step))
            continue;
          else if (!reversible)
            { 
              if (step->support_length <= 2)
                { 
                  if (simd_2tap_h_irrev32(&(sp->fval),&(dp->fval),width,
                                          step,false))
                    continue;
                }
              else if (step->support_length <= 4)
                { 
                  if (simd_4tap_h_irrev32(&(sp->fval),&(dp->fval),width,
                                          step,false))
                    continue;
                }
            }
#endif // KDU_SIMD_OPTIMIZATIONS
          if ((step->support_length==2) &&
              (step->coeffs[0]==step->coeffs[1]))
            { // Special case for symmetric least-dissimilar filters
              if (!reversible)
                { 
                  float lambda = step->coeffs[0];
                  for (k=0; k < width; k++)
                    dp[k].fval += lambda*(sp[k].fval+sp[k+1].fval);
                }
              else
                { 
                  kdu_int32 downshift = step->downshift;
                  kdu_int32 offset = step->rounding_offset;
                  kdu_int32 i_lambda = step->icoeffs[0];
                  if (i_lambda == 1)
                    for (k=0; k < width; k++)
                      dp[k].ival +=
                        ((offset+sp[k].ival+sp[k+1].ival)>>downshift);
                  else if (i_lambda == -1)
                    for (k=0; k < width; k++)
                      dp[k].ival +=
                        ((offset-sp[k].ival-sp[k+1].ival)>>downshift);
                  else
                    for (k=0; k < width; k++)
                      dp[k].ival +=
                        ((offset +
                          i_lambda*(sp[k].ival+sp[k+1].ival))>>downshift);
                }
            }
          else
            { // Generic 32-bit lifting step implementation
              int t, support=step->support_length;
              if (!reversible)
                { 
                  float sum, *cpp = step->coeffs;
                  for (k=0; k < width; k++, sp++)
                    { 
                      for (sum=0.0F, t=0; t < support; t++)
                        sum += cpp[t] * sp[t].fval;
                      dp[k].fval += sum;
                    }
                }
              else
                { 
                  kdu_int32 downshift = step->downshift;
                  kdu_int32 offset = step->rounding_offset;
                  int sum, *cpp = step->icoeffs;
                  for (k=0; k < width; k++, sp++)
                    { 
                      for (sum=offset, t=0; t < support; t++)
                        sum += cpp[t] * sp[t].ival;
                      dp[k].ival += (sum >> downshift);
                    }
                }
            }
        }
    }

  if (!horizontal_first)
    { // Push subband lines out.
      for (c=0; c < 2; c++)
        subbands[2*vert_parity+c].push(line->cosets[c],env);
    }
}


/* ========================================================================= */
/*                           EXTERNAL FUNCTIONS                              */
/* ========================================================================= */

/*****************************************************************************/
/* EXTERN             perform_analysis_lifting_step (32-bit)                 */
/*****************************************************************************/

void
  perform_analysis_lifting_step(kd_lifting_step *step,
                                kdu_sample32 *src_ptrs[],
                                kdu_sample32 *in, kdu_sample32 *out,
                                int width, int x_off)
{
  int k;

  if (width <= 0)
    return;
  while (x_off > 4)
    { x_off -= 4; in += 4; out += 4; }
  width += x_off;

#ifdef KDU_SIMD_OPTIMIZATIONS
  if (step->kernel_id == (kdu_byte) Ckernels_W5X3)
    {
      if (simd_W5X3_v_analysis32(&(src_ptrs[0]->ival),&(src_ptrs[1]->ival),
                                 &(in->ival),&(out->ival),width,step))
       return;
    }
  else if (!step->reversible)
    {
      if (step->support_length <= 2)
        {
          if (simd_2tap_v_irrev32(&(src_ptrs[0]->fval),
                                  &(src_ptrs[step->support_length-1]->fval),
                                  &(in->fval),&(out->fval),width,step,false))
            return;
        }
      else if (step->support_length <= 4)
        {
          if (simd_4tap_v_irrev32(&(src_ptrs[0]->fval),&(src_ptrs[1]->fval),
                                  &(src_ptrs[2]->fval),
                                  &(src_ptrs[step->support_length-1]->fval),
                                  &(in->fval),&(out->fval),width,step,false))
            return;
        }
    }
#endif // KDU_SIMD_OPTIMIZATIONS
  if ((step->support_length==2) && (step->coeffs[0]==step->coeffs[1]))
    { // Special implementation for symmetric least-dissimilar filters
      kdu_sample32 *sp1=src_ptrs[0], *sp2=src_ptrs[1];
      if (!step->reversible)
        {
          float lambda = step->coeffs[0];
          for (k=x_off; k < width; k++)
            out[k].fval = in[k].fval + lambda*(sp1[k].fval+sp2[k].fval);
        }
      else
        {
          kdu_int32 downshift = step->downshift;
          kdu_int32 offset = step->rounding_offset;
          kdu_int32 i_lambda = step->icoeffs[0];
          if (i_lambda == 1)
            for (k=x_off; k < width; k++)
              out[k].ival = in[k].ival +
                ((offset + sp1[k].ival + sp2[k].ival) >> downshift);
          else if (i_lambda == -1)
            for (k=x_off; k < width; k++)
              out[k].ival = in[k].ival +
                ((offset - sp1[k].ival - sp2[k].ival) >> downshift);
          else
            for (k=x_off; k < width; k++)
              out[k].ival = in[k].ival +
                ((offset + i_lambda*(sp1[k].ival+sp2[k].ival))>>downshift);
        }
    }
  else
    { // More general 32-bit processing
      int t;
      if (!step->reversible)
        {
          for (t=0; t < step->support_length; t++, in=out)
            {
              kdu_sample32 *sp = src_ptrs[t];
              float lambda = step->coeffs[t];
              for (k=x_off; k < width; k++)
                out[k].fval = in[k].fval + lambda*sp[k].fval;
            }
        }
      else
        {
          kdu_int32 downshift = step->downshift;
          kdu_int32 offset = step->rounding_offset;
          int sum, *fpp, support=step->support_length;
          for (k=x_off; k < width; k++)
            {
              for (fpp=step->icoeffs, sum=offset, t=0; t < support; t++)
                sum += fpp[t] * (src_ptrs[t])[k].ival;
              out[k].ival = in[k].ival + (sum >> downshift);
            }
        }
    }
}

/*****************************************************************************/
/* EXTERN             perform_analysis_lifting_step (16-bit)                 */
/*****************************************************************************/

void
  perform_analysis_lifting_step(kd_lifting_step *step,
                                kdu_sample16 *src_ptrs[],
                                kdu_sample16 *in, kdu_sample16 *out,
                                int width, int x_off)
{
  int k;

  if (width <= 0)
    return;
  while (x_off > 8)
    { x_off -= 8; in += 8; out += 8; }
  width += x_off;

#ifdef KDU_SIMD_OPTIMIZATIONS
  if ((step->kernel_id == (kdu_byte) Ckernels_W5X3) &&
    simd_W5X3_v_analysis(&(src_ptrs[0]->ival),&(src_ptrs[1]->ival),
                         &(in->ival),&(out->ival),width,step))
     return;
  else if ((step->kernel_id == (kdu_byte) Ckernels_W9X7) &&
           simd_W9X7_v_analysis(&(src_ptrs[0]->ival),&(src_ptrs[1]->ival),
                                &(in->ival),&(out->ival),width,step))
    return;
#endif // KDU_SIMD_OPTIMIZATIONS
  if ((step->support_length==2) && (step->icoeffs[0]==step->icoeffs[1]))
    {
      kdu_sample16 *sp1=src_ptrs[0], *sp2=src_ptrs[1];
      kdu_int32 downshift = step->downshift;
      kdu_int32 offset = (1<<downshift)>>1;
      kdu_int32 val, i_lambda=step->icoeffs[0];
      if (i_lambda == 1)
        for (k=x_off; k < width; k++)
          {
            val = sp1[k].ival;  val += sp2[k].ival;
            out[k].ival = in[k].ival + (kdu_int16)((offset+val)>>downshift);
          }
      else if (i_lambda == -1)
        for (k=x_off; k < width; k++)
          {
            val = sp1[k].ival;  val += sp2[k].ival;
            out[k].ival = in[k].ival + (kdu_int16)((offset-val) >> downshift);
          }
      else
        for (k=x_off; k < width; k++)
          {
            val = sp1[k].ival;  val += sp2[k].ival;  val *= i_lambda;
            out[k].ival = in[k].ival + (kdu_int16)((offset+val) >> downshift);
          }
    }
  else
    { // More general 16-bit processing
      kdu_int32 downshift = step->downshift;
      kdu_int32 offset = step->rounding_offset;
      int t, sum, *fpp, support=step->support_length;
      for (k=x_off; k < width; k++)
        {
          for (fpp=step->icoeffs, sum=offset, t=0; t < support; t++)
            sum += fpp[t] * (src_ptrs[t])[k].ival;
          out[k].ival = in[k].ival + (kdu_int16)(sum >> downshift);
        }
    }
}
