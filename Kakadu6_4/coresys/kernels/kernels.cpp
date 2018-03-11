/*****************************************************************************/
// File: kernels.cpp [scope = CORESYS/DWT-KERNELS]
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
   Implements the services defined by "kdu_kernels.h"
******************************************************************************/

#include <assert.h>
#include <math.h>
#include <string.h>
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_kernels.h"

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
   kdu_error _name("E(kernels.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
   kdu_warning _name("W(kernels.cpp)",_id);
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
/*                                kdu_kernels                                */
/* ========================================================================= */

/*****************************************************************************/
/*                            kdu_kernels::clear                             */
/*****************************************************************************/

void
  kdu_kernels::clear()
{
  kernel_id = -1;
  reversible = false;
  symmetric = false;
  symmetric_extension = false;
  num_steps = max_step_length = 0;
  step_info = NULL;
  lifting_factors = NULL;
  low_scale = high_scale = 1.0F;
  low_analysis_L = low_analysis_min = low_analysis_max = 0;
  high_analysis_L = high_analysis_min = high_analysis_max = 0;
  low_synthesis_L = low_synthesis_min = low_synthesis_max = 0;
  high_synthesis_L = high_synthesis_min = high_synthesis_max = 0;
  low_synthesis_taps = high_synthesis_taps = NULL;
  low_analysis_taps = high_analysis_taps = NULL;
  bibo_step_gains = NULL;
  max_initial_lowpass_stages = 4;
  work_L = -1;
  work1 = work2 = NULL;
  for (int n=0; n < 15; n++)
    {
      energy_gain_records[n].initial_stages = -1;
      energy_gain_records[n].energy_gain = 0.0;
      bibo_gain_records[n].initial_stages = -1;
      bibo_gain_records[n].bibo_gain = 0.0;
    }
}

/*****************************************************************************/
/*                            kdu_kernels::reset                             */
/*****************************************************************************/

void
  kdu_kernels::reset()
{
  if (step_info != NULL)
    delete[] step_info;
  if (lifting_factors != NULL)
    delete[] lifting_factors;
  if (low_analysis_taps != NULL)
    delete[] (low_analysis_taps-low_analysis_L);
  if (high_analysis_taps != NULL)
    delete[] (high_analysis_taps-high_analysis_L);
  if (low_synthesis_taps != NULL)
    delete[] (low_synthesis_taps-low_synthesis_L);
  if (high_synthesis_taps != NULL)
    delete[] (high_synthesis_taps-high_synthesis_L);
  if (work1 != NULL)
    delete[] (work1-work_L);
  if (work2 != NULL)
    delete[] (work2-work_L);
  if (bibo_step_gains != NULL)
    delete[] bibo_step_gains;
  clear();
}

/*****************************************************************************/
/*                     kdu_kernels::enlarge_work_buffers                     */
/*****************************************************************************/

void
  kdu_kernels::enlarge_work_buffers(int min_work_L)
{
  if (work_L >= min_work_L)
    return;
  float *w1 = (new float[2*min_work_L+1]) + min_work_L;
  float *w2 = (new float[2*min_work_L+1]) + min_work_L;
  if (work1 != NULL)
    {
      memcpy(w1-work_L,work1-work_L,sizeof(float)*(size_t)(2*work_L+1));
      delete[] (work1 - work_L);
      work1 = NULL;
    }
  if (work2 != NULL)
    {
      memcpy(w2-work_L,work2-work_L,sizeof(float)*(size_t)(2*work_L+1));
      delete[] (work2 - work_L);
      work2 = NULL;
    }
  work1 = w1;
  work2 = w2;
  work_L = min_work_L;
}

/*****************************************************************************/
/*                     kdu_kernels::expand_and_convolve                      */
/*****************************************************************************/

int
  kdu_kernels::expand_and_convolve(float **src, int src_L,
                                   float *taps, int taps_L, float **dst)
{
  int dst_L = 2*src_L + taps_L;
  enlarge_work_buffers(dst_L);
  float val, *dpp, *sp = *src, *dp = *dst;
  assert((sp == work1) || (sp == work2));
  assert((dp == work1) || (dp == work2));
  assert(dst_L <= work_L);

  int n, k;
  for (n=-dst_L; n <= dst_L; n++)
    dp[n] = 0.0F;
  for (k=-src_L; k <= src_L; k++)
    {
      val = sp[k];
      for (dpp=dp+(2*k), n=-taps_L; n <= taps_L; n++)
        dpp[n] += val * taps[n];
    }

  return dst_L;
}

/*****************************************************************************/
/*                    kdu_kernels::init (Part-1 Kernels)                     */
/*****************************************************************************/

void
  kdu_kernels::init(int kernel_id, bool reversible)
{
  reset();
  this->kernel_id = kernel_id;
  this->reversible = reversible;
  this->symmetric = true;
  this->symmetric_extension = true;
  if (kernel_id == Ckernels_W5X3)
    {
      num_steps = 2;
      max_step_length = 2;
      step_info = new kdu_kernel_step_info[num_steps];
      step_info[0].support_length = step_info[1].support_length = 2;
      lifting_factors = new float[2*num_steps];
      lifting_factors[0] = lifting_factors[2] = -0.5F;
      lifting_factors[1] = lifting_factors[3] = 0.25F;
      if (reversible)
        {
          step_info[0].downshift = 1;
          step_info[1].downshift = 2;
          step_info[0].rounding_offset = 1;
          step_info[1].rounding_offset = 2;
        }
    }
  else if (kernel_id == Ckernels_W9X7)
    {
      num_steps = 4;
      max_step_length = 2;
      step_info = new kdu_kernel_step_info[num_steps];
      step_info[0].support_length = step_info[1].support_length =
        step_info[2].support_length = step_info[3].support_length = 2;
      lifting_factors = new float[2*num_steps];
      if (reversible)
        { KDU_ERROR(e,0); e <<
            KDU_TXT("The W9X7 kernel may not be used for reversible "
            "compression!");
        }
      lifting_factors[0] = lifting_factors[4] = (float) -1.586134342;
      lifting_factors[1] = lifting_factors[5] = (float) -0.052980118;
      lifting_factors[2] = lifting_factors[6] = (float) 0.882911075;
      lifting_factors[3] = lifting_factors[7] = (float)  0.443506852;
    }
  else
    { KDU_ERROR_DEV(e,1); e <<
        KDU_TXT("Illegal DWT kernel ID used to construct a "
        "`kdu_kernels' object.");
    }

  for (int n=0; n < num_steps; n++)
    step_info[n].support_min =
      -((step_info[n].support_length + (n & 1) - 1) >> 1);

  derive_taps_and_gains();
}

/*****************************************************************************/
/*                  kdu_kernels::init (Arbitrary Kernels)                    */
/*****************************************************************************/

void
  kdu_kernels::init(int num_steps, const kdu_kernel_step_info *info,
                    const float *coefficients, bool symmetric,
                    bool symmetric_extension, bool reversible)
{
  int s, n;

  reset();
  this->kernel_id = Ckernels_ATK;
  this->reversible = reversible;
  this->symmetric = symmetric;
  this->symmetric_extension = symmetric_extension;
  this->num_steps = num_steps;
  max_step_length = 0;
  step_info = new kdu_kernel_step_info[num_steps];
  for (s=0; s < num_steps; s++)
    {
      step_info[s] = info[s];
      if (info[s].support_length > max_step_length)
        max_step_length = info[s].support_length;
    }
  lifting_factors = new float[max_step_length*num_steps];
  for (s=0; s < num_steps; s++)
    {
      int Ls = step_info[s].support_length;
      for (n=0; n < Ls; n++)
        lifting_factors[s + n*num_steps] = *(coefficients++);
      for (; n < max_step_length; n++)
        lifting_factors[s + n*num_steps] = 0.0F;
    }

  derive_taps_and_gains();
}

/*****************************************************************************/
/*                    kdu_kernels::derive_taps_and_gains                     */
/*****************************************************************************/

void
  kdu_kernels::derive_taps_and_gains()
{
  bibo_step_gains = new double[num_steps];

  // Allocate initial work buffers of sufficient size
  enlarge_work_buffers(max_step_length * num_steps);

  // Fill in the impulse responses, without normalization
  int n, s, which;
  int branch_min[2], branch_max[2]; // Min/max indices for even/odd branches
  float val, *fp, *branch_buf[2] = {work1,work2};
  for (which=0; which < 2; which++)
    { // Scan through the two different subbands

      // First, evaluate the synthesis impulse response for band `which'
      (branch_buf[which])[0] = 1.0F;
      branch_min[which] = branch_max[which] = 0; // Impulse on this branch
      branch_min[1-which] = 1;  branch_max[1-which] = -1; // Empty branch
      for (s=num_steps-1; s >= 0; s--)
        {
          int i, step_parity = s & 1; // Identifies the source branch
          if (branch_max[step_parity] < branch_min[step_parity])
            continue;
          int Ns = step_info[s].support_min;
          int Ps =  step_info[s].support_length - 1 + Ns;
              // Cs[n] has coefficients in the range Ns <= n <= Ps
              // Impulse response of lifting filter is Cs[-n]
          if ((n=branch_max[step_parity]-Ns) > branch_max[1-step_parity])
            while (branch_max[1-step_parity] < n)
              (branch_buf[1-step_parity])[++branch_max[1-step_parity]] = 0.0F;
          if ((n=branch_min[step_parity]-Ps) < branch_min[1-step_parity])
            while (branch_min[1-step_parity] > n)
              (branch_buf[1-step_parity])[--branch_min[1-step_parity]] = 0.0F;
          assert((branch_min[1-step_parity] >= -work_L) &&
                 (branch_max[1-step_parity] <= work_L));
          for (i=branch_min[step_parity]; i <= branch_max[step_parity]; i++)
            {
              val = (branch_buf[step_parity])[i];
              fp = lifting_factors + s;
              for (n=Ns; n <= Ps; n++, fp+=num_steps)
                (branch_buf[1-step_parity])[i-n] -= val * *fp;
            }
        }

      // Now the odd and even coefficients of the impulse response are in
      // the two branches -- don't forget that the impulse was at location
      // `which' -- we want the impulse response expressed relative to that
      // location

      // Find the length of the impulse response
      int impulse_min=100, impulse_max=-100; // Ridiculous values to start
      for (s=0; s < 2; s++)
        {
          if (branch_max[s] < branch_min[s])
            continue;
          if ((n = 2*branch_max[s]+s-which) > impulse_max)
            impulse_max = n;
          if ((n = 2*branch_min[s]+s-which) < impulse_min)
            impulse_min = n;
        }
      int impulse_L = // Find H, such that support contained in [-H,H]
        ((impulse_min+impulse_max) < 0)?(-impulse_min):(impulse_max);

      // De-interleave the branch buffers into the synthesis impulse response
      // buffer
      float *synthesis = new float[2*impulse_L+1];
      synthesis += impulse_L;
      for (n=-impulse_L; n <= impulse_L; n++)
        synthesis[n] = 0.0F;

      for (s=0; s < 2; s++)
        for (n=branch_min[s]; n <= branch_max[s]; n++)
          synthesis[2*n+s-which] = (branch_buf[s])[n];

      // Deduce analysis impulse response from the synthesis impulse response
      float *analysis = new float[2*impulse_L+1];
      analysis += impulse_L;
      for (n=-impulse_L; n <= impulse_L; n++)
        analysis[n] = (n&1)?(-synthesis[n]):(synthesis[n]);

      if (which == 0)
        { // Low-pass synthesis response was found
          low_synthesis_min = high_analysis_min = impulse_min;
          low_synthesis_max = high_analysis_max = impulse_max;
          low_synthesis_L = high_analysis_L = impulse_L;
          low_synthesis_taps = synthesis;
          high_analysis_taps = analysis;
        }
      else
        { // High-pass synthesis response was found
          high_synthesis_min = low_analysis_min = impulse_min;
          high_synthesis_max = low_analysis_max = impulse_max;
          high_synthesis_L = low_analysis_L = impulse_L;
          high_synthesis_taps = synthesis;
          low_analysis_taps = analysis;
        }
    }

  // Deduce scaling factors and normalize filter taps.
  if (reversible)
    { low_scale = high_scale = 1.0F; return; }

  float gain;

  for (gain=0.0F, n=-low_analysis_L; n <= low_analysis_L; n++)
    gain += low_analysis_taps[n];
  low_scale = 1.0F / gain;
  for (n=-low_analysis_L; n <= low_analysis_L; n++)
    low_analysis_taps[n] *= low_scale;
  for (n=-low_synthesis_L; n <= low_synthesis_L; n++)
    low_synthesis_taps[n] *= gain;

  for (gain=0.0F, n=-high_analysis_L; n <= high_analysis_L; n++)
    gain += (n & 1)?(-high_analysis_taps[n]):(high_analysis_taps[n]);
  high_scale = 1.0F / gain;
  for (n=-high_analysis_L; n <= high_analysis_L; n++)
    high_analysis_taps[n] *= high_scale;
  for (n=-high_synthesis_L; n <= high_synthesis_L; n++)
    high_synthesis_taps[n] *= gain;
}

/*****************************************************************************/
/*                     kdu_kernels::get_impulse_response                     */
/*****************************************************************************/

float *
  kdu_kernels::get_impulse_response(kdu_kernel_type which, int &half_length,
                                    int *support_min, int *support_max)
{
  switch (which) {
    case KDU_ANALYSIS_LOW:
      if (support_min != NULL) *support_min = low_analysis_min;
      if (support_max != NULL) *support_max = low_analysis_max;
      half_length = low_analysis_L;
      return low_analysis_taps;
    case KDU_ANALYSIS_HIGH:
      if (support_min != NULL) *support_min = high_analysis_min;
      if (support_max != NULL) *support_max = high_analysis_max;
      half_length = high_analysis_L;
      return high_analysis_taps;
    case KDU_SYNTHESIS_LOW:
      if (support_min != NULL) *support_min = low_synthesis_min;
      if (support_max != NULL) *support_max = low_synthesis_max;
      half_length = low_synthesis_L;
      return low_synthesis_taps;
    case KDU_SYNTHESIS_HIGH:
      if (support_min != NULL) *support_min = high_synthesis_min;
      if (support_max != NULL) *support_max = high_synthesis_max;
      half_length = high_synthesis_L;
      return high_synthesis_taps;
    default:
      assert(0);
    }
  return NULL;
}

/*****************************************************************************/
/*                        kdu_kernels::get_energy_gain                       */
/*****************************************************************************/

double
  kdu_kernels::get_energy_gain(int initial_lowpass_stages,
                               int num_extra_stages, bool extra_stage_high[])
{
  assert((num_extra_stages >= 0) && (num_extra_stages <= 3));
  int n, record_idx = (1 << num_extra_stages) - 1;
  for (n=0; n < num_extra_stages; n++)
    record_idx += (extra_stage_high[n])?(1<<n):0;
  kd_energy_gain_record *rec = NULL;
  if (record_idx < 15)
    rec = energy_gain_records + record_idx;

  int calculated_initial_lowpass_stages = initial_lowpass_stages;
  double interp_factor = 1.0;
  while (calculated_initial_lowpass_stages > max_initial_lowpass_stages)
    {
      calculated_initial_lowpass_stages--;
      interp_factor *= 2.0;
    }
  if ((rec != NULL) &&
      (rec->initial_stages == calculated_initial_lowpass_stages))
    return (rec->energy_gain * interp_factor);

  int half_len;
  float **wtmp, **wp[2] = {&work1,&work2};
  work1[0] = 1.0F;  half_len = 0;
  for (n=num_extra_stages-1; n >= 0; n--)
    {
      if (extra_stage_high[n])
        half_len = expand_and_convolve(wp[0],half_len,high_synthesis_taps,
                                       high_synthesis_L,wp[1]);
      else
        half_len = expand_and_convolve(wp[0],half_len,low_synthesis_taps,
                                       low_synthesis_L,wp[1]);
      wtmp = wp[0]; wp[0] = wp[1];  wp[1] = wtmp;
    }
  for (n=calculated_initial_lowpass_stages; n > 0; n--)
    {
      half_len = expand_and_convolve(wp[0],half_len,low_synthesis_taps,
                                     low_synthesis_L,wp[1]);
      wtmp = wp[0]; wp[0] = wp[1];  wp[1] = wtmp;
    }
  assert(half_len <= work_L);

  float *src = *(wp[0]);
  double val, energy = 0.0;
  for (n=-half_len; n <= half_len; n++)
    {
      val = src[n];
      energy += val*val;
    }

  if ((rec != NULL) &&
      (rec->initial_stages < calculated_initial_lowpass_stages))
    {
      rec->initial_stages = calculated_initial_lowpass_stages;
      rec->energy_gain = energy;
    }
  return energy * interp_factor;
}

/*****************************************************************************/
/*                         kdu_kernels::get_bibo_gain                        */
/*****************************************************************************/

double
  kdu_kernels::get_bibo_gain(int initial_lowpass_stages,
                             int num_extra_stages, bool extra_stage_high[])
{
  assert((num_extra_stages >= 0) && (num_extra_stages <= 3));
  int n, record_idx = (1 << num_extra_stages) - 1;
  for (n=0; n < num_extra_stages; n++)
    record_idx += (extra_stage_high[n])?(1<<n):0;
  kd_bibo_gain_record *rec = NULL;
  if (record_idx < 15)
    rec = bibo_gain_records + record_idx;

  if (initial_lowpass_stages > max_initial_lowpass_stages)
    initial_lowpass_stages = max_initial_lowpass_stages;
  if ((rec != NULL) && (rec->initial_stages == initial_lowpass_stages))
    return rec->bibo_gain;

  // If we get here, we need to calculate the BIBO gain.  We will use the
  // `get_bibo_gains' function to calculate both low and high-pass gains
  // at once, storing both of them in the records if allowed.
  double low_gain, high_gain, bibo_gain;
  if (num_extra_stages > 0)
    {
      get_bibo_gains(initial_lowpass_stages,num_extra_stages-1,
                     extra_stage_high,low_gain,high_gain);
      if (extra_stage_high[num_extra_stages-1])
        {
          bibo_gain = high_gain;
          if (rec != NULL)
            {
              if (rec->initial_stages < initial_lowpass_stages)
                {
                  rec->initial_stages = initial_lowpass_stages;
                  rec->bibo_gain = high_gain;
                }
              rec -= (int)(1<<(num_extra_stages-1));
                      // Rec with low-pass final branch
              if (rec->initial_stages < initial_lowpass_stages)
                {
                  rec->initial_stages = initial_lowpass_stages;
                  rec->bibo_gain = low_gain;
                }
              else if (rec->initial_stages == initial_lowpass_stages)
                assert(rec->bibo_gain == low_gain);
            }
        }
      else
        {
          bibo_gain = low_gain;
          if (rec != NULL)
            {
              if (rec->initial_stages < initial_lowpass_stages)
                {
                  rec->initial_stages = initial_lowpass_stages;
                  rec->bibo_gain = low_gain;
                }
              rec += (int)(1<<(num_extra_stages-1));
                      // Rec with hi-pass final branch
              if (rec->initial_stages < initial_lowpass_stages)
                {
                  rec->initial_stages = initial_lowpass_stages;
                  rec->bibo_gain = high_gain;
                }
              else if (rec->initial_stages == initial_lowpass_stages)
                assert(rec->bibo_gain == high_gain);
            }
        }
    }
  else if (initial_lowpass_stages > 0)
    {
      get_bibo_gains(initial_lowpass_stages-1,0,NULL,low_gain,high_gain);
      assert((record_idx == 0) && (rec != NULL));
      bibo_gain = low_gain;
      if (rec->initial_stages < initial_lowpass_stages)
        {
          rec->initial_stages = initial_lowpass_stages;
          rec->bibo_gain = bibo_gain;
        }
    }
  else
    bibo_gain = 1.0;

  return bibo_gain;
}

/*****************************************************************************/
/*                         kdu_kernels::get_bibo_gains                       */
/*****************************************************************************/

double *
  kdu_kernels::get_bibo_gains(int initial_lowpass_stages,
                              int extra_stages, bool extra_stage_high[],
                              double &low_gain, double &high_gain)
{
  if (initial_lowpass_stages > max_initial_lowpass_stages)
    initial_lowpass_stages = max_initial_lowpass_stages;

  enlarge_work_buffers(1); // Make sure we have at least something
  float *work_low=work1, *work_high=work2;

  // In the seqel, `work_low' will hold the analysis kernel used to compute
  // the even sub-sequence entry at location 0, while `work_high' will hold the
  // analysis kernels used to compute the odd sub-sequence entry at location
  // 1.  The lifting procedure is followed to alternately update these
  // analysis kernels.

  int k, lev, low_min, low_max, high_min, high_max, gap;
  int total_levels = initial_lowpass_stages + extra_stages + 1;

  // Initialize analysis vectors and gains for a 1 level lazy wavelet
  work_low[0] = 1.0F;
  low_min = low_max = 0;
  high_min = 0; high_max = 0;
  low_gain = high_gain = 1.0;

  for (gap=1, lev=0; lev < total_levels; lev++, gap<<=1)
    { // Work through the levels
      bool keep_low = (lev <= initial_lowpass_stages) ||
                      !extra_stage_high[lev-1-initial_lowpass_stages];

      if (keep_low)
        { /* Copy the low analysis vector from the last level to the high
             analysis vector for the current level. */
          for (k=low_min; k <= low_max; k++)
            work_high[k] = work_low[k];
          high_min = low_min;  high_max = low_max;
          high_gain = low_gain;
        }
      else
        { /* Copy the high analysis vector from the last level to the low
             analysis vector for the current level. */
          for (k=high_min; k <= high_max; k++)
            work_low[k] = work_high[k];
          low_min = high_min;  low_max = high_max;
          low_gain = high_gain;
        }

      // See if we need to resize the work buffers
      int safe_L = ((low_max+low_min) > 0)?low_max:(-low_min);
      safe_L += gap*max_step_length*num_steps;
      if (safe_L > work_L)
        {
          enlarge_work_buffers(safe_L);
          work_low = work1;   work_high = work2;
        }

      for (int step=0; step < num_steps; step+=2)
        { // Work through the lifting steps in this level
          if (low_min <= low_max)
            { // Updating the odd sub-sequence analysis kernel
              int n;
              int Ns = step_info[step].support_min;
              int Ps =  step_info[step].support_length - 1 + Ns;
                // Cs[n] has coefficients in the range Ns <= n <= Ps
              float *fp = lifting_factors + step;
              for (n=low_min+(2*Ns-1)*gap; n < high_min; )
                work_high[--high_min] = 0.0F;
              for (n=low_max+(2*Ps-1)*gap; n > high_max; )
                work_high[++high_max] = 0.0F;
              for (n=Ns; n <= Ps; n++, fp+=num_steps)
                for (k=low_min; k <= low_max; k++)
                  work_high[k+(2*n-1)*gap] += work_low[k] * fp[0];

              for (high_gain=0.0, k=high_min; k <= high_max; k++)
                high_gain += fabs(work_high[k]);
              bibo_step_gains[step] = high_gain;
            }

          if ((high_min <= high_max) && ((step+1) < num_steps))
            { // Now update the even sub-sequence analysis kernel
              int n;
              int Ns = step_info[step+1].support_min;
              int Ps = step_info[step+1].support_length - 1 + Ns;
                // Cs[n] has coefficients in the range Ns <= n <= Ps
              float *fp = lifting_factors + (step+1);
              for (n=high_min+(2*Ns+1)*gap; n < low_min; )
                work_low[--low_min] = 0.0F;
              for (n=high_max+(2*Ps+1)*gap; n > low_max; )
                work_low[++low_max] = 0.0F;
              for (n=Ns; n <= Ps; n++, fp+=num_steps)
                for (k=high_min; k <= high_max; k++)
                  work_low[k+(2*n+1)*gap] += work_high[k] * fp[0];

              for (low_gain=0.0, k=low_min; k <= low_max; k++)
                low_gain += fabs(work_low[k]);
              bibo_step_gains[step+1] = low_gain;
            }
        }

      // Now incorporate the subband scaling factors
      for (k=high_min; k <= high_max; k++)
        work_high[k] *= high_scale;
      high_gain *= high_scale;
      for (k=low_min; k <= low_max; k++)
        work_low[k] *= low_scale;
      low_gain *= low_scale;
    }

  return bibo_step_gains;
}
