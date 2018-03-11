/******************************************************************************/
// File: region_decompressor_local.h [scope = APPS/SUPPORT]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/******************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/******************************************************************************/
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
/*******************************************************************************
Description:
   Provides local definitions for the implementation of the
`kdu_region_decompressor' object.
*******************************************************************************/

#ifndef REGION_DECOMPRESSOR_LOCAL_H
#define REGION_DECOMPRESSOR_LOCAL_H

#include <string.h>
#include "kdu_region_decompressor.h"

// Declared here
struct kdrd_interp_kernels;
struct kdrd_tile_bank;
struct kdrd_component;
struct kdrd_channel;

/******************************************************************************/
/*                            kdrd_interp_kernels                             */
/******************************************************************************/

#define KDRD_INTERP_KERNEL_STRIDE     14
#define KDRD_SIMD_KERNEL_NONE         0
#define KDRD_SIMD_KERNEL_VERT_FLOATS  1
#define KDRD_SIMD_KERNEL_VERT_FIX16   2
#define KDRD_SIMD_KERNEL_HORZ_FLOATS  3
#define KDRD_SIMD_KERNEL_HORZ_FIX16   4

struct kdrd_interp_kernels {
   public: // Member functions
     kdrd_interp_kernels()
      { target_expansion_factor=derived_max_overshoot=-1.0F;
        simd_kernel_length=simd_horz_leadin=0; kernel_length=6;
        simd_kernel_type=KDRD_SIMD_KERNEL_NONE; }
     void init(float expansion_factor, float max_overshoot,
               float zero_overshoot_threshold);
       /* If the arguments supplied here agree with the object's internal
          state, the function does nothing.  Otherwise, the function
          initializes the interpolation kernels for use with the supplied
          `expansion_factor' -- this is the ratio of input sample spacing
          to interpolated sample spacing, where values > 1 correspond to
          expansion of the source data.  Specifically, the normalized bandwidth
          of the interpolation kernels is BW = min{1,`expansion_factor'} and
          the kernels are obtained by windowing and normalizing the function
          sinc((n-sigma)*BW), where sigma is the relevant kernel's centre of
          mass, which ranges from 0.0 to 1.0.  The window has region of support
          covering -2 <= n <= 3.
             The function additionally limits the BIBO gain of the generated
          interpolation kernels, in such a way as to limit overshoot/undershoot
          when interpolating step edges.  The limit is `max_overshoot'
          times the height of the step edge in question for expansion factors
          which are <= 1.  For larger expansion factors, the `max_overshoot'
          value is linearly decreased in such a way that it becomes 0 at
          the point where `expansion_factor' >= `zero_overshoot_threshold'.
          The way in which the maximum overshoot/undershoot is limited is
          by mixing the 6-tap windowed sinc interpolation kernel with a
          2-tap (bi-linear) kernel.  In the extreme case, where `max_overshoot'
          is 0, or `expansion_factor' >= `zero_overshoot_threshold', all
          generated kernels will actually have only two non-zero taps.  This
          case is represented in a special way to facilitate efficient
          implementation. */
     bool copy(kdrd_interp_kernels &src, float expansion_factor,
               float max_overshoot, float zero_overshoot_threshold);
       /* If the arguments agree with the object's internal state, the
          function returns true, doing nothing.  Otherwise, if the arguments
          agree sufficiently well with the `src' object's initialized state,
          that object is copied and the function returns true.  Otherwise,
          the function returns false. */
#ifdef KDU_SIMD_OPTIMIZATIONS
     void *get_simd_kernel(int type, int i);
       /* Call this function to prepare the internal `simd_kernels' array
          such that each entry in the array points to an appropriate kernel
          (array index identifies the centre of mass, in the range 0.0 to 1.0,
          in steps of 1/32), in accordance with the supplied `type', if this
          has not already been done, and to initialize the `i'th kernel (if
          not already initialized) for use in resampling, returning a pointer
          to this kernel.  The interpretation of the `type' argument is as
          follows:
          [>>] `KDRD_SIMD_KERNEL_VERT_FLOATS' -- in this case `simd_kernels[i]'
               points to an array of `kernel_length' groups of 4 floats, where
               each group of 4 floats is identical, being equal to the
               corresponding coefficient from the original kernel.  The
               `simd_kernel_length' value in this case is identical to
               `kernel_length'.
          [>>] `KDRD_SIMD_KERNEL_VERT_FIX16' -- in this case, `simd_kernels[i]'
               points to `kernel_length' groups of 8 16-bit words, where each
               group of 8 words is identical, being equal to the corresponding
               fixed-point coefficient from the original kernel.  Again, in
               this case `simd_kernel_length' and `kernel_length' are identical.
          [>>] `KDRD_SIMD_KERNEL_HORZ_FLOATS' -- in this case, `simd_kernels[i]'
               points to `simd_kernel_length' groups of 4 floats, which
               together form 4 kernels q[n,m] (m=0,1,2,3; n=0,1,...), which
               are used to form four floating point outputs y[m]. Let R
               denote the reciprocal of the `target_expansion_factor' member.
                  If `kernel_length'=6 and R >= 1, the output y[m] is formed
               from the inner product \sum_n q[n,m]x[n+m-2]; in this case,
               q[n,0] is the interpolation kernel for the relevant shift, padded
               with trailing zeros, while q[n,m] is obtained by adding
               (R-1)*m to the shift (sigma_i) associated with index i, and
               positioning the start of the 6-tap interpolation kernel,
               corresponding to sigma = FRAC(sigma_i+(R-1)*m), at location
               floor(sigma_i+(R-1)*m), padding the unfilled positions with
               zeros.  Since R is strictly less than 3, sigma_i+(R-1)*m
               is strictly less than 7, so the start of the last kernel
               cannot exceed position 6.  This ensures that the aligned kernels
               all fit within 12 coefficients, so `simd_kernel_length' will be
               no larger than 12.  The `simd_horz_leadin' member will be
               equal to 2 for this case, reflecting the fact that the first
               sample required to form y[m] is x[n+m-2].
                  If `kernel_length'=6 and R < 1, the output y[m] is
               formed from the inner product \sum_n q[n,m]x[n+m-L], where L
               is the value of `simd_horz_leadin'; in this case, q[n,0] is
               the interpolation kernel for the relevant shift, padded
               with L-2 leading zeros.  q[n,m] is obtained by subtracting
               (1-R)*m from the shift sigma_i and positioning the interpolation
               kernel which has centre of mass sigma = FRAC(sigma_i-(1-R)*m)
               at location L - 2 + floor(sigma_i-(1-R)*m), padding the
               unfilled positions with zeros.  Since R > 0, sigma_i-(1-R)*m is
               strictly greater than -3, and the start of the last kernel
               commences at position L-5 or greater.  This means that L need be
               no larger than 5.  Note, however, that if R is close to 1, the
               value of L could be closer to 2, allowing smaller overall SIMD
               kernel lengths.  With the maximum value of 5 for
               L=`simd_horz_leadin', `simd_kernel_length' takes its maximum
               value of 9.
                  Otherwise we must have `kernel_length'=2 and R<1 (expansion).
               In this case, the output y[m] is formed from the inner product
               \sum_n q[n,m]x[n], where q[n,0] is the interpolation kernel
               for the relevant shift, padded with `simd_kernel_length'-2
               trailing zeros.  q[n,m] is obtained by adding R*m to the shift
               sigma_i and positioning the interpolation kernel which has centre
               of mass sigma = FRAC(sigma_i+R*m) at location floor(sigma_i+R*m),
               padding unfilled positions with zeros.  Since R<1 and m<=3, the
               `simd_kernel_length' value will never exceed 5.  However, for
               small R (large expansion factors), `simd_kernel_length' may be
               as small as 3.  Note that `simd_horz_leadin' is always 0 in
               this case.
          [>>] `KDRD_SIMD_KERNEL_HORZ_FIX16' -- in this case, `simd_kernels[i]'
               points to `simd_kernel_length' groups of 8 16-bit words,
               which together form 8 kernels q[n,m] (m=0,1,...,7; n=0,1,...),
               which are used to form 8 outputs y[m].  The approach is similar
               to that described above for the floating-point case, except that
               the equations are as follows:
                  If `kernel_length'=6 and R >= 1, the output y[m] is formed
               from the inner product \sum_n q[n,m]x[n+m-2] and q[n,m] is
               obtained by positioning the relevant 6-tap interpolation kernel
               at location floor(sigma_i+(R-1)*m), which cannot exceed 14.  In
               this case, the `simd_kernel_length' value will never be larger
               than 20, but could be as small as 7 in some cases.  For this
               case, as before, `simd_horz_leadin' = 2.
                  If `kernel_length'=6 and R < 1, the output y[m] is formed
               from the inner product \sum_n q[n,m]x[n+m-L] and q[n,m] is
               obtained by positioning the relevant 6-tap interpolation kernel
               at location L - 2 + floor(sigma_i-(1-R)*m).  In this case,
               `simd_kernel_length' takes a maximum value of 13 when
               L=`simd_horz_leadin' takes its maximum value of 9.  For values
               of R close to 1, L can be closer to 2 leading to SIMD kernel
               lengths which can be as small as 7.
                  Otherwise we must have `kernel_length'=2 and R<1 (expansion).
               In this case, the output y[m] is formed from the inner product
               \sum_n q[n,m]x[n], where q[n,0] is the interpolation kernel
               for the relevant shift, padded with `simd_kernel_length'-2
               trailing zeros.  q[n,m] is obtained by adding R*m to the shift
               sigma_i and positioning the interpolation kernel which has centre
               of mass sigma = FRAC(sigma_i+R*m) at location floor(sigma_i+R*m),
               padding unfilled positions with zeros.  Since R<1 and m<=7, the
               `simd_kernel_length' value will never exceed 9.  However, for
               small R (large expansion factors), `simd_kernel_length' may be
               as small as 3.  Note that `simd_horz_leadin' is always 0 in
               this case.
        */
#endif // KDU_SIMD_OPTIMIZATIONS
   public: // Data
     float target_expansion_factor; // As supplied to `init'
     float derived_max_overshoot; // Maximum overshoot for this set of kernels
     float float_kernels[33*KDRD_INTERP_KERNEL_STRIDE]; // See below
     kdu_int32 fix16_kernels[33*KDRD_INTERP_KERNEL_STRIDE]; // Same but * -2^15
     int kernel_length; // 6 or 2 -- see below
     int kernel_coeffs; // 6 or 14 -- see below
     int simd_kernel_length; // See `get_simd_kernel'
     int simd_horz_leadin; // 0 if vertical, 3 if R >= 1, 6 or 10 if R < 1
  private:
     int simd_kernel_type; // Type of simd_kernels created, if any (so far)
#ifdef KDU_SIMD_OPTIMIZATIONS
     kdu_int64 simd_kernels_initialized; // 1 flag bit for each kernel
     void *simd_kernels[33]; // 16-byte aligned pointers into `simd_block'
     kdu_int32 simd_block[33*20*4+3];
#endif // KDU_SIMD_OPTIMIZATIONS
 };
 /* Notes:
       The `target_expansion_factor' keeps track of the expansion factor for
    which this object was initialized.  The expansion factor may be less than
    or greater than 1; it affects both the bandwidths of the designed kernels
    and also the structure of horizontally extended kernels -- see below.
       The `derived_max_overshoot' value represents the upper bound on the
    relative overshoot/undershoot associated with interpolation of step edges.
    This is the value that was used to design the interpolation kernels found
    in this object.
       The `float_kernels' array holds 33 interpolation kernels, corresponding
    to kernels whose centre of mass, sigma, is uniformly distributed over the
    interval from 0.0 to 1.0, relative to the first of the two central
    coefficients; there are (`filter_length'-2)/2 coefficients before this one.
    The kernel coefficients are separated by `KDRD_INTERP_KERNEL_STRIDE' which
    must, of course, be large enough to accommodate `kernel_length'.  In the
    case where `kernel_length'=6, there are only 6 coefficients in this array
    for each kernel and so `kernel_coeffs'=6 and the last
    `KDRD_INTERP_KERNEL_STRIDE'-`kernel_length' entries in each block of
    `KDRD_INTERP_KERNEL_STRIDE' are left uninitialized.  In the case where
    `kernel_length'=2, it is guaranteed that `target_expansion_factor' > 1
    (the `init' and `copy' functions ensure that this is always the case) and
    the first 2 coefficients of the i'th `KDRD_INTERP_KERNEL_STRIDE'-length
    block hold the values 1-sigma_i and sigma_i, where sigma_i=i/32.0.  In
    this case, however, `kernel_coeffs'=14 and the remaining 12 coefficients
    of the i'th kernel block are initialized to hold kernels q[n,m] of
    length 3 (m=1), 4 (m=2) and 5 (m=3), such that taking the m'th successive
    output sample can be formed from y[m] = sum_{0 <= n < 2+m} x[n]q[n,m].
    These extra kernels correspond to shifts sigma_i+R*m.  This allows
    a direct implementation of the horizontal interpolation process to
    rapidly compute up to 4 outputs together before determining a new kernel. */

/******************************************************************************/
/*                               kdrd_tile_bank                               */
/******************************************************************************/

struct kdrd_tile_bank {
  public: // Construction/destruction
    kdrd_tile_bank()
      {
        max_tiles=num_tiles=0; tiles=NULL; engines=NULL;
        env_queue=NULL; queue_bank_idx=0; freshly_created=false;
      }
    ~kdrd_tile_bank()
      {
        if (tiles != NULL) delete[] tiles;
        if (engines != NULL) delete[] engines;
      }
  public: // Data
    int max_tiles; // So that `tiles' and `engines' arrays can be reallocated
    int num_tiles; // 0 if the bank is not currently in use
    kdu_coords first_tile_idx; // Absolute index of first tile in bank
    kdu_dims dims; // Region occupied on ref component's coordinate system
    kdu_tile *tiles; // Array of `max_tiles' tile interfaces
    kdu_multi_synthesis *engines; // Array of `max_tiles' synthesis engines
    kdu_thread_queue *env_queue; // Queue for these tiles, if multi-threading
    kdu_long queue_bank_idx; // Index supplied to `kdu_thread_env::add_queue'
    bool freshly_created; // True only when the bank has just been created by
      // `kdu_region_decompressor::start_tile_bank' and has not yet been used
      // to decompress or render any data.
  };

/******************************************************************************/
/*                               kdrd_component                               */
/******************************************************************************/

struct kdrd_component {
  public: // Member functions
    kdrd_component() { max_tiles=0; tile_lines=NULL; }
    ~kdrd_component() { if (tile_lines != NULL) delete[] tile_lines; }
    void init(int relative_component_index)
      {
        this->rel_comp_idx = relative_component_index;
        bit_depth = 0; is_signed=false; palette_bits = 0;
        num_line_users = needed_line_samples = new_line_samples = 0;
        dims = kdu_dims(); indices.destroy(); num_tiles=non_empty_tile_lines=0;
        for (int t=0; t < max_tiles; t++)
          tile_lines[t] = NULL;
        have_fix16 = have_compatible16 = have_floats = false;
      }
  public: // Data
    int rel_comp_idx; // Index to be used after `apply_input_restrictions'
    int bit_depth;
    bool is_signed;
    int palette_bits; // See below
    int num_line_users; // Num channels using the `tile_line' entries
    int needed_line_samples; // Used for state information in `process_generic'
    int new_line_samples; // Number of newly decompressed samples in `line'
    kdu_dims dims; // Remainder of current tile-bank region; see notes below.
    kdu_coords alignment; // See below
    int num_tiles, max_tiles, non_empty_tile_lines;
    kdu_line_buf **tile_lines; // Array with `max_tiles' entries
    kdu_line_buf indices; // See notes below
    bool have_fix16; // If any of the tile-lines uses 16-bit fixed point
    bool have_compatible16; // If a tile-line can be converted to fix16 w/o loss
    bool have_floats; // If any of the tile-lines uses floats
  };
  /* Notes:
        Most members of this structure are filled out by the call to
     `kdu_region_decompressor::start', after which they remain unaffected
     by calls to the `kdu_region_decompressor::process' function.  The dynamic
     members are as follows:
            `new_line_samples', `dims', `tile_lines', `num_tiles', `max_tiles',
            `indices', `have_shorts' and `have_floats'.
        If `palette_bits' is non-zero, the `indices' buffer will be non-empty
     (its `exists' member will return true) and the code-stream sample values
     will be converted to palette indices immediately after (or during)
     decompression.
        The `tile_lines' array is used to keep track of the decompressed
     lines from each of the horizontally adjacent tiles in the current
     tile-bank.  The `max_tiles' member is used to keep track of the number
     of entries available in the `tile_lines' array, while `num_tiles' keeps
     track of the number of tiles being used for the current tile-bank.  The
     `num_tiles' value can be 0 if the `indices' buffer is being used.
        The entries in `tile_lines' are arranged such that all non-empty lines
     appear together at the start; the `non_empty_tile_lines' member is used
     to count these non-empty lines as they are being generated by the
     tile synthesis engines.
 */

/******************************************************************************/
/*                                kdrd_channel                                */
/******************************************************************************/

#define KDRD_CHANNEL_VLINES 6    // Size of the `vlines' member array
#define KDRD_CHANNEL_LINE_BUFS 7 // Size of the `line_bufs' member array

struct kdrd_channel {
  public: // Member functions
    void init()
      {
        source=NULL; lut=NULL; in_line = horz_line = out_line=NULL;
        in_precision = in_line_start = in_line_length = out_line_length = 0;
        can_use_component_samples_directly=false;
        for (int k=0; k < KDRD_CHANNEL_LINE_BUFS; k++) line_bufs[k].destroy();
        line_bufs_used = 0;
        memset(vlines,0,sizeof(kdu_line_buf *)*KDRD_CHANNEL_VLINES);
        native_precision=0; native_signed=false; using_shorts=false;
        stretch_residual = num_valid_vlines = 0; stretch_source_factor=1.0F;
        sampling_numerator = sampling_denominator = kdu_coords(1,1);
        sampling_phase = sampling_phase_shift = kdu_coords(0,0);
        memset(horz_interp_kernels,0,sizeof(void *)*65);
        memset(vert_interp_kernels,0,sizeof(void *)*65);
#ifdef KDU_SIMD_OPTIMIZATIONS
        memset(simd_horz_interp_kernels,0,sizeof(void *)*65);
        memset(simd_vert_interp_kernels,0,sizeof(void *)*65);
#endif
        boxcar_size = kdu_coords(1,1); missing = kdu_coords(0,0);
        boxcar_lines_left = 0;
      }
    kdu_line_buf *get_free_line()
      {
        int idx=0, mask=~line_bufs_used; // Get availability bit mask
        assert((mask & 0x07F) != 0); // Otherwise all buffers used!
        if ((mask & 15) == 0) { idx+=4; mask>>=4; }
        if ((mask & 3) == 0) { idx += 2; mask>>=2; }
        if ((mask & 1) == 0) idx++;
        assert(idx < KDRD_CHANNEL_LINE_BUFS);
        line_bufs_used |= (1<<idx);
        return line_bufs+idx;
      }
    void recycle_line(kdu_line_buf *line)
      {
        int idx=(int)(line-line_bufs);
        if ((idx >= 0) && (idx < KDRD_CHANNEL_LINE_BUFS))
          line_bufs_used &= ~(1<<idx);
      }
  public: // Resources, transformations and representation info
    kdrd_component *source; // Source component for this channel.
    kdu_sample16 *lut; // Palette mapping LUT.  NULL if no palette.
    kdu_line_buf *in_line; // For boxcar integration/conversion/realigment
    kdu_line_buf *horz_line; // Set to NULL only when we need a new one
    kdu_line_buf *vlines[KDRD_CHANNEL_VLINES];
    kdu_line_buf *out_line; // NULL until we have a valid unconsumed output
    kdu_line_buf line_bufs[KDRD_CHANNEL_LINE_BUFS];
    int in_precision; // Precision prior to boxcar renormalization
    int in_line_start, in_line_length, out_line_length;
    int boxcar_log_size; // Log_2(boxcar_size.x*boxcar_size.y)
    bool can_use_component_samples_directly; // See below
    int line_bufs_used; // One flag bit for each entry in `line_bufs'
    int native_precision; // Used if `kdu_region_decompressor::convert'
    bool native_signed;   // supplies a `precision_bits' argument of 0.
    bool using_shorts; // If `line_bufs' have a short representation
    bool using_floats; // If `line_bufs' have a floating point representation
    kdu_uint16 stretch_residual; // See below
    float stretch_source_factor; // See below
  public: // Coordinates and state variables
    kdu_coords source_alignment;
    int num_valid_vlines;
    kdu_coords sampling_numerator;
    kdu_coords sampling_denominator;
    kdu_coords sampling_phase;
    kdu_coords sampling_phase_shift;
    kdu_coords boxcar_size; // Guaranteed to be powers of 2
    kdu_coords missing;
    int boxcar_lines_left;
  public: // Lookup tables used to implement efficient kernel selection; these
          // are all indexed by `sampling_phase' >> `sampling_phase_shift'.
    void *horz_interp_kernels[65];
    void *vert_interp_kernels[65];
#ifdef KDU_SIMD_OPTIMIZATIONS
    void *simd_horz_interp_kernels[65];
    void *simd_vert_interp_kernels[65];
#endif
    kdu_uint16 horz_phase_table[65]; // Output sample location w.r.t. nearest
    kdu_uint16 vert_phase_table[65]; // src sample x (src sample spacing)/32
    kdrd_interp_kernels v_kernels;
    kdrd_interp_kernels h_kernels;
  };
  /* Notes:
        Except in the case where no processing is performed and no conversions
     are required for any reason, the channel buffers maintained by this
     structure have one of three possible representations:
     [S] 16-bit fixed-point, with KDU_FIX_POINT fraction bits, is used as
         much as possible.  This representation is always used if there is
         a palette `lut' or colour conversion is required.  The `using_shorts'
         flag is set if this representation is employed.
     [F] 32-bit floating-point, with a nominal range of -0.5 to +0.5.  The
         `using_floats' flag is set if this representation is employed.
     [I] 32-bit integers, with the original image component bit-depth, as
         given by `source->bit_depth'.  This is the least used mode; it may
         not be used if there is any resampling (including boxcar
         integration).
     The value of `in_precision' is used to record the precision associated
     with `in_line' before any boxcar renormalization.  For the [F]
     representation, `in_precision' always holds 0.  If there is no
     boxcar integration, `in_precision' holds KDU_FIX_POINT [S] or
     `source->bit_depth' [I].  If there is boxcar integration, only the
     [S] or [F] representations are valid; in the latter case, `in_precision'
     is 0, as mentioned; for [S], `in_precision' is increased beyond
     KDU_FIX_POINT to accommodate accumulation with as little pre-shifting
     as possible, and the buffers are allocated with double width so that
     they can be temporarily type-cast to 32-bit integers for the purpose
     of accumulating boxcar samples without overflow, prior to normalization.
        The conversion from decoded image components to an output channel
     buffer (referenced by `out_buf') involves some or all of the following
     steps.  As these steps are being performed, each of `in_line', `horz_line'
     and `out_line' may transition from NULL to non-NULL and back again to
     keep track of the processing state.
       a) Component values are subjected to any palette `lut' first, if
          required -- the output of this stage is always written to an
          `in_line' buffer and `using_shorts' must be true.
       b) Component values or `lut' outputs may be subjected to a coarse
          "boxcar" sub-sampling process, in which horizontally and/or
          vertically adjacent samples are accumulated in an `in_line'
          buffer.  This is done to implement large sub-sampling factors only,
          and is always followed by a more rigorous subsampling process in
          which the resolution will be reduced by at most a factor of 4,
          using appropriate anti-aliasing interpolation kernels.  Note that
          boxcar integration cells are always aligned at multiples of the
          boxcar cell size, on the canvas coordinate system associated with
          the `source' component.
       c) If neither of the above steps were performed, but raw component
          samples do not have the same representation as the channel line
          buffers, or there are multiple tiles in the tile-bank, or horizontal
          or vertical resampling is required, the source samples are
          transferred to an `in_line' buffer.
       d) In this step, horizontal resolution expansion/reduction processing
          is applied to the samples in `in_line' and the result written to
          the samples in `horz_line'.  If no horizontal processing is required,
          `horz_line' might be identical to `in_line' or even the original line
          of source component samples.
       e) If vertical resolution expansion/reduction is required, the
          vertical filter buffer implemented by `vlines' is rotated by one
          line and `horz_line' becomes the most recent line in this vertical
          buffer; `out_line' is then set to a separate free buffer line and
          vertical processing is performed to generate its samples.  If no
          vertical processing is required, `out_line' is the same as
          `horz_line'.
       f) If `stretch_residual' > 0, the white stretching policy described
          in connection with `kdu_region_decompressor::set_white_stretch' is
          applied to the data in `horz_line'.  If the source
          `bit_depth', P, is greater than or equal to the value of
          `kdu_region_decompressor::white_stretch_precision', B, the value of
          `stretch_residual' will be 0.  Otherwise, `stretch_residual' is set to
          floor(2^{16} * ((1-2^{-B})/(1-2^{-P}) - 1)), which
          necessarily lies in the range 0 to 0xFFFF.  The white stretching
          policy may then be implemented by adding (x*`stretch_residual')/2^16
          to each sample, x, after converting to an unsigned representation.
          In practice, we perform the conversions on signed quantities by
          introducing appropriate offsets.  If white stretching is required,
          the [S] representation must be used.
       g) Once completed `out_line' buffers are available for all channels,
          any required colour transformation is performed in-place on the
          channel `out_line' buffers.  If colour transformation is required,
          the [S] representation must be used.
     [//]
     We turn our attention now to dimensions and coordinates.  The following
     descripion is written from the perspective that horizontal and vertical
     resampling will be required.  Variations are fairly obvious for cases
     in which either or both operation are not required.
     [>>] The `source_alignment' member records the effect of any image
          component registration offset on the shifts which must be
          implemented during interpolation.  These shifts are expressed
          in multiples of boxcar cells, relative to the `sampling_denominator'.
     [>>] `num_valid_vlines' identifies the number of initial entries in the
          `vlines' buffer which hold valid data.  During vertical resampling,
          this value needs to reach 6 before a new output line can be
          generated.
     [>>] The `sampling_numerator' and `sampling_denominator' members
          dictate the expansion/reduction factors to be applied in each
          direction after any boxcar accumulation, while `sampling_phase'
          identifies the amount of horizonal shift associated with the
          first column of `out_line' and the amount of vertical shift
          associated with the current `out_line' being generated.  More
          specifically, if the spacing between `in_line' samples is
          taken to be 1, the spacing between interpolated output samples
          is equal to `sampling_numerator'/`sampling_denominator'.  The
          phase values are set up so as to always hold non-negative quantities
          in the range 0 to `sampling_denominator'-1, but the notional
          displacement of a sample with phase P and denominator D, relative
          to the "nearest" `in_line' sample is given by
                  sigma = P / D
          The horizontal phase parameter is set so that the first sample in
          the `in_line' is the one which is nearest to (but not past) the
          first sample in `horz_line', while the vertical phase parameter is
          set up so that the third line in the `vlines' buffer is the one which
          is "nearest" to (but not pat) the `out_line' being generated.  Each
          time a new line is generated the `sampling_phase.y' value is
          incremented by `sampling_numerator.y', after which it is brought
          back into the range 0 to `sampling_denominator.y'-1 by shuffling
          lines in the `vlines' buffer and decrementing `num_valid_vlines',
          as required, subtracting `sampling_denominator.y' each time.
     [>>] In practice, we need to reduce the phase index P to an interpolation
          kernel, and we don't want to use explicit division to do this.
          Instead, we use (P + 2^{S-1}) >> S to index one of the lookup tables
          `horz_interp_kernels' or `vert_interp_kernels', as appropriate,
          where S, the value of `sampling_phase_shift', is chosen as small as
          possible such that 2^S > D/64.  The `sampling_numerator' and
          `sampling_denominator' values are scaled, if required, to ensure
          that the denominator is always greater than or equal to 32, unless
          this cannot be done without risking overflow, so as to minimize
          any loss of accuracy which may be incurred by the shift+lookup
          strategy for interpolation kernel selection.  After the quantization
          associated with this indexing strategy, some phases P which are close
          to D may be better represented with sigma=1.0 than the next available
          smaller value.  Thus, even though P is guaranteed to lie in the range
          0 to D-1, we maintain interpolation kernels with centres of mass
          which are distributed over the full range from 0.0 to 1.0.
     [>>] Each boxcar sample in `in_line' has cell size `boxcar_size.x' by
          `boxcar_size.y'.  In practice, some initial source rows might not
          be available for accumulation; these are indicate by
          `missing.y'.  Similarly, some initial source columns might
          not be available and these are indicated by `missing.x'.
          When a new line of component samples becomes available, the
          `missing.y' parameter is examined to determine whether this
          row should be counted multiple times, effectively implementing
          boundary extrapolation -- the value of `missing.y' is
          decremented to reflect any additional contributions, but we note
          that the value can be as large or even larger than `boxcar_size.y',
          in which case the boundary extrapolation extends across multiple
          lines of boxcar accumulation.  Similar considerations apply to the
          re-use of a first sample in each source line in accordance with the
          value of `missing.x'.  It is also worth noting that the `missing.x'
          and `missing.y' values may be negative if a channel does not actually
          need some of the available source component samples/lines.
     [>>] The `boxcar_lines_left' member keeps track of the number of source
          lines which have yet to be accumulated to form a complete
          `in_line'.  This value is always initialized to `boxcar_size.y',
          regardless of the value of `missing', which means that when
          initial source rows are replicated to accommodate `missing.y',
          the replication count must be subtracted from `boxcar_lines_left'.
     [>>] The `in_line_start' and `in_line_length' members identify
          the range of sample indices which must be filled out for
          `in_line'.  `in_line_start' will be equal to -2 if horizontal
          resampling is required (otherwise it is 0), since horizontal
          interpolation kernels extend 2 samples to the left and 3 samples
          to the right (from an inner product perspective), for a total
          length of 6 taps.  The `in_line_length' member holds the total
          number of samples which must be filled out for the `in_line',
          starting from the one identified by `in_line_start'.
     [//]
     The `stretch_source_factor' member is used only when responding to a
     call to the floating point `kdu_region_decompressor::process' function.
     It holds the ratio 2^B / (2^B - 1) where B is `source->bit_depth', unless
     white stretching or palette lookup is involved, in which case it is
     adjusted to represent the effective bit-depth associated with the
     target of any white stretching or palette lookup table.
   */

#endif // REGION_DECOMPRESSOR_LOCAL_H
