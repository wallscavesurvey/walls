/*****************************************************************************/
// File: kdu_kernels.h [scope = CORESYS/COMMON]
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
   Defines services for deriving important properties of DWT kernels from
the relevant identifier.  The implementation of the `kdu_kernels' class may
easily be extended to any odd-length symmetric DWT kernels without affecting
the way in which the derived information is accessed and interpreted by the
rest of the system.
******************************************************************************/

#ifndef KDU_KERNELS_H
#define KDU_KERNELS_H

#include <assert.h>
#include <stdlib.h>

#include "kdu_params.h"

struct kdu_kernel_step_info;
class kdu_kernels;

/*****************************************************************************/
/* ENUM                          kdu_kernel_type                             */
/*****************************************************************************/

enum kdu_kernel_type {
    KDU_ANALYSIS_LOW=0,
    KDU_ANALYSIS_HIGH=1,
    KDU_SYNTHESIS_LOW=2,
    KDU_SYNTHESIS_HIGH=3
  };

/*****************************************************************************/
/*                             kdu_kernel_step_info                          */
/*****************************************************************************/

struct kdu_kernel_step_info {
  /* [SYNOPSIS]
       This structure is used by `kdu_kernels::get_step_info' to return
       a description of each lifting step, excluding the coefficient
       values themselves.
       [//]
       Let Cs[n] denote the coefficients for lifting step s and let Ls
       denote the corresponding length.  The lifting step is implemented
       as follows:
       [>>] X_s[2k+1-p] = sum_{Ns <= n <= Ps}  Cs[n] * X_{s-1}[2k+p+2n]
       [//]
       Here, p = (s & 1) is the parity of the lifting step.  Even lifting
       steps update the odd indexed subsequence based on the even indexed
       subsequence (vice-versa for odd lifting steps).  The first
       analysis lifting step is always assigned an index of s=0.
       [//]
       The coefficient indices run from Ns to Ps, where Ls = Ps+1-Ns.
  */
  //---------------------------------------------------------------------------
  public: // Functions
    kdu_kernel_step_info()
      { support_length = support_min = 0;  downshift = rounding_offset = 0; }
  public: // Data
    int support_length;
      /* [SYNOPSIS] Value of Ls -- see above description. */
    int support_min;
      /* [SYNOPSIS] Value of Ns (usually <= 0) -- see above description. */
    int downshift; // Used only for reversible kernels
      /* [SYNOPSIS]
           Value of the downshift to be applied for reversible transforms.
           This value is also returned by `get_lifting_downshift'.  The
           value is guaranteed to be non-negative.
      */
    int rounding_offset;
      /* [SYNOPSIS]
           Value of the rounding offset to be added prior to downshifting
           using the `downshift' value -- relevant only for reversible
           transforms.
      */
  };

/*****************************************************************************/
/*                                 kdu_kernels                               */
/*****************************************************************************/

class kdu_kernels {
  /* [SYNOPSIS]
     Service object which provides various useful summary quantities
     concerning the DWT and its kernels. The object currently accepts
     only the two kernels defined by Part 1 of the JPEG2000 standard, but
     may be trivially extended to work with any odd-length symmetric DWT
     kernels.
  */
  //---------------------------------------------------------------------------
  public: // Creation/Initialization/Destruction functions
    kdu_kernels() { clear(); }
      /* [SYNOPSIS]
           If you use this form of the constructor, you must follow it
           by a call to `init'.
      */
    kdu_kernels(int kernel_id, bool reversible)
      { clear(); init(kernel_id,reversible); }
      /* [SYNOPSIS]
           Invokes the first form of the `init' function.  For more general
           DWT kernels, you should construct the object and then explicitly
           invoke the second form of the `init' function.
      */
    ~kdu_kernels() { reset(); }
    void init(int kernel_id, bool reversible);
      /* [SYNOPSIS]
           Initialize the object to work with one of the two standard DWT
           kernels defined by Part-1 of the JPEG2000 standard.
           For more general kernels, use the second form of the `init'
           function.
           [//]
           If the object was already initialized, it is first reset to the
           empty state before re-initialization.
         [ARG: kernel_id]
           Must be one of `Ckernels_W5X3' or `Ckernels_W9X7'.
         [ARG: reversible]
           Only affects subband normalization gains.
           The gains are both set to 0 if `reversible' is true --
           see `get_lifting_factors'.
      */
    void init(int num_steps, const kdu_kernel_step_info *info,
              const float *coefficients, bool symmetric,
              bool symmetric_extension, bool reversible);
      /* [SYNOPSIS]
           Use this form of the `init' function to initialize an arbitrary
           transform kernel, as recorded in an ATK marker segment.  The
           `info[0].support_length' coefficients associated with the first
           analysis lifting step appear first in the `coefficients' array,
           followed by the `info[1].support_length' coefficients of the
           second lifting step, and so forth.  In the case of reversible
           transforms, the floating-point coefficients must be multiplied
           by 2 to the power of the corresponding
           `kdu_kernel_step_info::downshift', in order to recover the
           integer-valued coefficients which are actually used in a
           reversible implementation.
      */
  //---------------------------------------------------------------------------
  public: // Access function
    bool is_reversible() { return reversible; }
      /* [SYNOPSIS]
           Returns true if the wavelet kernel with which the object was
           constructed is reversible.
      */
    bool get_symmetric_extension() { return symmetric_extension; }
      /* [SYNOPSIS]
           Returns true if the symmetric boundary extension policy is
           to be used at image boundaries within each lifting step.   The
           centre of symmetry in this case is the boundary sample location
           within an interleaved representation in which low-pass samples
           occupy the even indexed locations and high-pass samples occupy
           the odd indexed locations.
           [//]
           If the function returns false, the sample-replication is to
           be used at the boundaries within each lifting step.
      */
    bool is_symmetric() { return this->symmetric; }
      /* [SYNOPSIS]
           Returns true if and only if `get_symmetric_extension' returns
           true and all lifting step filters are symmetric, with even length,
           satisfying
           [>>] Ns = -floor((Ls+p-1)/2) and Ps = floor((Ls-p)/2); and
           [>>] Cs[-Ns+n] = Cs[Ps-n] for 0 <= n < Ls.
           [//]
           where Ns and Ps are the lower and upper bounds of the lifting
           step coefficient arrays, Cs, as described in the comments
           accompanying `get_step_info'.
           [//]
           The wavelet kernels allowed by Part-1 of the JPEG2000 standard
           both have `is_symmetric' true and `max_length'=2.  More generally,
           the `is_symmetric' flag is true for all wavelet kernels with
           whole-sample-symmetric filters; these are treated as a special
           class in Part-2 of the JPEG2000 standard.  This class tends to
           have particularly efficient implementation properties for a
           given filter length.
      */
    int get_id() { return kernel_id; }
      /* [SYNOPSIS]
           Identifies the particular set of wavelet kernels which are being
           used, returning -1 if the wavelet kernel does not have its own
           unique identifier -- in that case, the second form of the
           constructor was used to install the kernels.
         [RETURNS]
           One of `Ckernels_W5X3' or `Ckernels_W9X7', unless the first form
           of the `init' function was not used, in which case the function
           returns `Ckernels_ATK' (i.e., -1).
      */
    const kdu_kernel_step_info *
      get_step_info(int &num_steps, int &max_length)
        {
          num_steps = this->num_steps;  max_length = this->max_step_length;
          return step_info;
        }
      /* [SYNOPSIS]
           This function may be used to discover the number of lifting
           steps, the region of support for each lifting step, and other
           related attributes.  The returned array is an internal resource.
           Its contents are not to be changed.  It contains one entry
           for each lifting step, starting from s=0, representing the
           steps required to perform the subband analysis decomposition.
      */
    const float *
      get_lifting_factors(int &num_steps, float &dc_scale, float &nyq_scale)
        {
          num_steps = this->num_steps; dc_scale = this->low_scale;
          nyq_scale = this->high_scale; return lifting_factors;
        }
      /* [SYNOPSIS]
           Returns a complete lifting factorization of the kernels.
           [//]
           If the kernels are symmetric, with odd least dissimilar lengths
           (lengths differ by 2), the lifting steps will involve symmetric
           2-tap filters, characterized by a single factor each.  These
           factors are given by the first `num_steps' entries in the
           returned array, establishing backward compatibility with
           earlier versions of Kakadu which supported only the DWT kernels
           defined by Part-1 of the JPEG2000 standard.
           [//]
           More generally, the lifting step s is characterized
           by Ls coefficients, Cs[n], where n lies in the range
           Ns <= n <= Ps, as described in conjunction with the
           `kdu_kernel_step_info' structure -- see also, `get_step_info'.
           The first `num_steps' entries in the returned array contain the
           coefficients Cs[Ns] for each lifting steps, s. The next `num_steps'
           entries in the returned array contain the coefficients Cs[Ns+1]
           for each lifting step, s, and so forth.  Thus, the returned array
           contains `num_steps'*Lmax entries, where Lmax is the `max_length'
           value returned via `get_step_info', and coefficient Cs[n] may
           be found in the entry at location (n-Ns)*`num_steps'.
         [RETURNS]
           Array of lifting step coefficients for the analysis transform.  For
           the synthesis transform, the steps are reversed and the
           coefficients negated.  The array is guaranteed to contain
           `num_steps'*`max_length' entries, where `max_length' is the
           value returned via the `get_lifting_lengths' function.  Unused
           entries will be 0.
         [ARG: num_steps]
           Used to return the number of lifting steps.
         [ARG: dc_scale]
           Used to return a scale factor by which the low-pass subband
           samples must be multiplied after all lifting steps are complete,
           during subband analysis.  For reversible transforms, this factor
           is guaranteed to be equal to 1.0.  For irreversible transforms, the
           scale factor is computed so as to ensure that the DC gain of the
           low-pass analysis kernel is equal to 1.0.
         [ARG: nyq_scale]
           Similar to `dc_scale', but applied to the high-pass subband.  For
           reversible transforms the factor is guaranteed to be 1.0, while for
           irreversible transforms, it is computed so as to ensure that the
           gain of the high-pass analysis kernel at the Nyquist frequency is
           equal to 1.0.
      */
    bool get_lifting_downshift(int step_idx, int &downshift)
      {
      /* [SYNOPSIS]
           Determines a downshift value for use with the lifting step
           coefficients in implementing reversible transforms.  Returns false
           if the transform is irreversible.  Otherwise returns true and sets
           the `downshift' argument.
           [//]
           This function is provided only for backward compatibility with
           Kakadu versions 4.5 and earlier.  Additional information is
           returned via the `get_step_info' function.
         [RETURNS]
           False if the transform is irreversible; otherwise, true.
         [ARG: step_idx]
           Identifies the specific lifting step (starting
           from 0) for which the downshift is being requested.
         [ARG: downshift]
           Unaffected unless the transform is reversible, in
           which case it is used to return the smallest non-negative integer
           such that the lifting factor for the indicated lifting step is an
           integer multiple of 2^{-`downshift'}.
      */
        assert((step_idx >= 0) && (step_idx < num_steps));
        if (!reversible)
          return false;
        downshift = step_info[step_idx].downshift;
        return true;
      }
    float *get_impulse_response(kdu_kernel_type which, int &half_length,
                                int *support_min=NULL, int *support_max=NULL);
      /* [SYNOPSIS]
           Returns the equivalent linearized impulse responses of the DWT
           kernels.  The returned array may be addressed with indices
           running from -H to +H where H is the value returned by
           `half_length'.
           [//]
           If `is_symmetric' returns true, the filter impulse
           response returned here is guaranteed to have odd length and
           be symmetric about the origin.  Otherwise, the returned array
           is not guaranteed to exhibit any particular symmetry properties.
           [//]
           For non-symmetric filters, [-H,H] is only an upper bound
           to the support of the wavelet kernel, since some of the extreme
           locations on the left or right might be 0.  If so, they will be
           exactly equal to 0.0F.  The exact region of support may
           be discovered by supplying non-NULL `support_min' and
           `support_max' arguments (see below).
         [RETURNS]
           An array whose entries run from n=-H to n=+H, where H is the value
           returned via `half_length'.  Index n=0 corresponds to the "central"
           tap of the filter's impulse response.  Note carefully, that the
           impulse responses are defined with respect to a slightly
           different downsampling/upsampling convention than the one you
           might be used to.  Specifically, the downsampling operation
           associated with the high-pass subband retains the odd-indexed
           samples, while the downsampling operation associated with the
           low-pass subband retains the even-indexed subbands.  Similarly,
           the upsampling operation associated with the high-pass subband
           places the subband samples at location m into the odd
           location, 2*m+1, while the upsampling operation associated with
           the high-pass subband places the subband samples at location m
           into the even location, 2*m.
           [//]
           The returned array contains a true impulse response.  If
           analysis is to be implemented as a weighted moving average (i.e.,
           as an inner product), you will need to reverse the corresponding
           impulse responses to recover the weights.  Of course, this is
           only important if the filters do not have the odd-length
           symmetric property (i.e., if `is_symmetric' returns false).
         [ARG: which]
           Indicates which impulse response is required (analysis or
           synthesis, low- or high- pass).  Valid values are
           `KDU_ANALYSIS_LOW', `KDU_ANALYSIS_HIGH', `KDU_SYNTHESIS_LOW' and
           `KDU_SYNTHESIS_HIGH'.
         [ARG: half_length]
           Used to return the value of the parameter H, such that the
           relevant impulse response runs from -H to +H.  If the impulse
           response does not have odd length, at least one trailing
           coefficient must be 0.  The exact region of support may be
           learned by supplying non-NULL `support_min' and `support_max'
           arguments.
         [ARG: support_min]
           If non-NULL, this argument is used to return the actual lower
           bound of the filter impulse response's region of support.
           Specifically, it is used to return the value I (typically, but
           not necessarily less than 0) such that the support is
           [I,J].  Note that this region is contained within [-H,H] where
           H is the value returned via `half_length'.
         [ARG: support_max]
           If non-NULL, this argument is used to return the actual upper
           bound of the filter impulse response's region of support.
           Specifically, it is used to return the value J (typically, but
           not necessarily greater than 0) such that the support is [I,J].
      */
    double get_energy_gain(int initial_lowpass_stages,
                           int num_extra_stages, bool extra_stage_high[]);
      /* [SYNOPSIS]
           Computes the energy gain factor associated with the one dimensional
           synthesis basis vectors (sequences) for samples in a general
           packet wavelet transform.  The corresponding 1D DWT analysis
           operation can be understood in terms of an initial
           `initial_lowpass_stages' low-pass filtering and downsampling
           operations, followed by an additional `num_extra_stages'
           decomposition steps, in which we take the low-pass subband if
           the corresponding entry in the `extra_stage_high' array is
           false, or the high-pass subband if the corresponding entry in the
           `extra_stage_high' array is true.
           [//]
           The function does not explicitly compute the analysis or
           synthesis vectors for more than a certain number of DWT levels,
           beyond which point it relies upon the fact that the vectors
           converge to samplings of underlying continuous wavelet and
           scaling functions.  This allows a large number of DWT levels
           to be specified without consuming inordinate amounts of memory
           calculating the kernels.  For an explanation of these convergence
           properties, see Section 17.3 of the book by Taubman and
           Marcellin.
           [//]
           To determine the energy gain for 2D subbands, the relevant
           1D energy gains for each direction have simply to be multiplied.
         [RETURNS]
           The energy gain factor.
         [ARG: initial_lowpass_stages]
           Must be >= 0.  This should typically be a small value (at most
           3 should be sufficient to accommodate all the custom decomposition
           styles supported by Part-2 of the JPEG2000 standard).
         [ARG: num_extra_stages]
           Must be in the range 0 to 3.
         [ARG: extra_stage_high]
           Array with `num_extra_stages' entries.  May be NULL only if
           `num_extra_stages' is 0.
       */
    double get_bibo_gain(int initial_lowpass_stages, int num_extra_stages,
                         bool extra_stage_high[]);
      /* [SYNOPSIS]
           Same as `get_energy_gain', except that it computes the BIBO
           (Bounded Input Bounded Output) gain of the analysis operator
           which transforms a 1D input sequence into the subband described
           by the function's three arguments.  As for `get_energy_gain',
           the `num_extra_stages' argument must be in the range 0 to 3.
      */
    double *get_bibo_gains(int initial_lowpass_stages,
                           int num_extra_stages, bool extra_stage_high[],
                           double &low_gain, double &high_gain);
      /* [SYNOPSIS]
           Computes the BIBO (Bounded-Input-Bounded-Output) analysis gain
           from a 1D transform's input sequence to the subbands and
           intermediate lifting step results produced in a given subband
           decomposition stage.  The stage in question is preceded by
           `initial_lowpass_stages' low-pass filtering and decimation stages
           and `num_extra_stages' additional stages or low- or high-pass
           filtering and decimation, where the corresponding entries in
           the `extra_stage_high' array indicate whether the low- or high-pass
           branch is taken in each of those extra stages.  The stage in
           question, for which the BIBO gains are being explicitly calculated,
           immediate follows those initial low-pass stages and extra low- or
           high-pass stages.  Thus, the BIBO gain calculations for the first
           stage in a dyadic DWT are obtained by setting
           `initial_lowpass_stages' and `num_extra_stages' both equal to 0.
           [//]
           The BIBO gain of a linear system is the ratio between the
           maximum absolute value of the output sequence and the maximum
           absolute value of the input sequence.  As such the BIBO gain figures
           are useful for determining the optimal placement of the binary
           point in fixed point DWT implementations.
           [//]
           Note that beyond a certain number of initial lowpass DWT levels,
           the function does not explicitly calculate the analysis vectors and
           their BIBO gains.  Instead, it relies upon the fact that the
           analysis vectors converge to samplings of the dual wavelet and
           scaling functions which are continuous.  This enables it to
           accurately predict gains for very deep DWT's without expending
           substantial memory resources.  For a discussion of these
           properties, see Section 17.3 of the book by Taubman and Marcellin.
         [RETURNS]
           Pointer to an array with one entry per lifting step, whose entries
           hold the BIBO gain from the input sequence through to the output
           of that lifting step in the subband transform stage in question
           (see above for a discussion of the transform stages which precede
           the one in question).
         [ARG: initial_lowpass_stages]
           Must be >= 0.
         [ARG: num_extra_stages]
           Must be >= 0.  This should typically be a small value (at most
           3 should be sufficient to accommodate all the custom decomposition
           styles supported by Part-2 of the JPEG2000 standard).
         [ARG: extra_stage_high]
           Array with `num_extra_stages' entries.  May be NULL only if
           `num_extra_stages' is 0.
         [ARG: low_gain]
           Used to return the BIBO gain from the input sequence to the
           low-pass subband produced by the stage in question.
         [ARG: high_gain]
           Used to return the BIBO gain from the input sequence to the
           high-pass subband produced by the stage in question.
      */
  //---------------------------------------------------------------------------
  public: // Old-style interface functions
    double get_energy_gain(kdu_kernel_type which, int level_idx)
      {
        bool is_high = (which==KDU_SYNTHESIS_LOW)?false:true;
        if (level_idx == 0) return ((is_high)?0.0:1.0);
        return get_energy_gain(level_idx-1,1,&is_high);
      }
      /* [SYNOPSIS]
           This is the old energy gain calculation function interface,
           dating from the era when Kakadu supported only JPEG2000 Part-1
           decomposition structures.  It simply invokes the new more
           general version of the function which supports wavelet packet
           transforms.
         [RETURNS]
           The energy gain factor.
         [ARG: which]
           Must be one of KDU_SYNTHESIS_LOW or KDU_SYNTHESIS_HIGH.
         [ARG: level_idx]
           A value of 0 refers to no transform at all, in which case the
           function returns 1.0 for low-pass and 0.0 for high-pass kernels.
           Otherwise, this argument identifies the level (or depth) of the
           subband within the DWT.
      */
    double *get_bibo_gains(int level_idx, double &low_gain, double &high_gain)
      {
        if (level_idx == 0)
          { low_gain=1.0;  high_gain=0.0;  return NULL; }
        return get_bibo_gains(level_idx-1,0,NULL,low_gain,high_gain);
      }
      /* [SYNOPSIS]
           This is the old BIBO gain calculation function interface,
           dating from the era when Kakadu supported only JPEG2000 Part-1
           decomposition structures.  It simply invokes the new more
           general version of the function which supports wavelet packet
           transforms.
         [RETURNS]
           Pointer to an array with one entry per lifting step, whose entries
           hold the BIBO gain from the input sequence through to the output
           of that lifting step in the indicated DWT level.  If
           `level_idx' is 0, the function returns NULL.
         [ARG: level_idx]
           The particular level in the DWT pyramid at which the BIBO gain is
           being calculated.  A value of 0 refers to no transform at all.
         [ARG: low_gain]
           Used to return the BIBO gain from the input sequence to the
           low-pass subbands at DWT level `level_idx'.  If `level_idx' is 0,
           the function returns 1.0.
         [ARG: high_gain]
           Used to return the BIBO gain from the input sequence to the
           high-pass subband at DWT level `level_idx'.  If `level_idx' is 0,
           the function returns 0.0.
      */
  //---------------------------------------------------------------------------
  private: // Helper functions
    void clear(); // Called by both constructors to reset data members
    void reset(); // Deletes all storage and then calls `clear'
    void derive_taps_and_gains(); // Called at the end of `init'
    void enlarge_work_buffers(int min_work_L);
      /* If necessary, this function re-allocates the work buffers, `work1'
         and `work2', copying any existing contents to the enlarged buffers. */
    int expand_and_convolve(float **src, int src_L, float *taps,
                            int taps_L, float **dst);
      /* This function is used to build the analysis and synthesis vectors
         associated with particular branches in the iterated DWT.  To do so,
         the *`src' buffer is upsampled by 2, inserting 0's in the missing
         positions, and then convolved by the impulse response in `taps'.  The
         result is written to `dst'.  On input the `src' buffer runs from
         -`src_L' to +`src_L'.  Similarly, the `taps' impulse response runs
         from -`taps_L' to +`taps_L'.  The function returns a value L, such
         that the created `dst' vector runs from -L to +L.  Both *`src' and
         *`dst' are actually one or another of the internal work buffers,
         `work1' and `work2'.  If L is going to exceed the current value of
         `work_L', the work buffers are re-allocated and any existing
         contents are also copied to the enlarged buffers. */
  //---------------------------------------------------------------------------
  private: // Internal structures
    struct kd_energy_gain_record {
        int initial_stages; // -ve if not initialized yet
        double energy_gain;
      };
    struct kd_bibo_gain_record {
        int initial_stages; // -ve if not initialized yet
        double bibo_gain;
      };
  //---------------------------------------------------------------------------
  private: // Original filter parameters
    int kernel_id;
    bool reversible;
    bool symmetric;
    bool symmetric_extension;
    int num_steps;
    int max_step_length;
    kdu_kernel_step_info *step_info;
    float *lifting_factors;
  private: // Derived filter data
    float low_scale, high_scale;

    int low_analysis_L, low_analysis_min, low_analysis_max; // H, I and J
    float *low_analysis_taps; // 2-sided array from -H to +H

    int high_analysis_L, high_analysis_min, high_analysis_max; // H, I and J
    float *high_analysis_taps; // 2-sided array from -H to +H

    int low_synthesis_L, low_synthesis_min, low_synthesis_max; // H, I and J
    float *low_synthesis_taps; // 2-sided array from -H to +H

    int high_synthesis_L, high_synthesis_min, high_synthesis_max; // H, I and J
    float *high_synthesis_taps; // 2-sided array from -H to +H

  private: // Parameters for DWT gain calculations
    double *bibo_step_gains; // Array with `num_steps' entries.
    int max_initial_lowpass_stages; // max levels to expand explicitly
    int work_L; // Used to hold expanded analysis or synthesis vectors
    float *work1, *work2; // Both 2-sided arrays from -`work_L' to +`work_L'
    kd_energy_gain_record energy_gain_records[15];
    kd_bibo_gain_record bibo_gain_records[15];
      // Records the most recently calculated gain values for all possible
      // combinations of the `extra_stages' and `extra_stage_high'
      // arguments to `get_bibo_gain' and `get_energy_gain' for which
      // 0 <= `extra_stages' <= 3.
  };

#endif // KDU_KERNELS_H
