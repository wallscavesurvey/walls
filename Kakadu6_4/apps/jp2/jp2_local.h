/*****************************************************************************/
// File: jp2_local.h [scope = APPS/JP2]
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
   Defines local classes used by the internal JP2 file format processing
machinery implemented in "jp2.cpp".  You should respect the local
nature of this header file and not include it directly from an application
or any other part of the Kakadu system (not in the APPS/JP2 scope).
******************************************************************************/
#ifndef JP2_LOCAL_H
#define JP2_LOCAL_H

#include <assert.h>
#include "jp2.h"
#include "jp2_shared.h"

// Defined here
class j2_colour_converter;
class j2_icc_profile;
class j2_header;
class j2_target;


/*****************************************************************************/
/*                              j2_icc_profile                               */
/*****************************************************************************/

class j2_icc_profile {
  public: // Member functions
    j2_icc_profile()
      {
        buffer = NULL; num_buffer_bytes = 0; num_colours = 0; num_tags = 0;
        pcs_is_xyz = uses_3d_luts = false;
        profile_is_input = profile_is_display = profile_is_output = false;
      }
    ~j2_icc_profile()
      { if (buffer != NULL) delete[] buffer; }
    void init(kdu_byte *profile_buf, bool donate_buffer=false);
        // If `donate_buffer' is true, no internal copy of the buffer is made
    int get_num_colours()
      { return num_colours; }
    kdu_byte *get_profile_buf(int *num_bytes=NULL)
      { if (num_bytes != NULL)
          *num_bytes = num_buffer_bytes;
        return buffer; }
    bool is_restricted()
      { return (pcs_is_xyz && ((num_colours == 1) || (num_colours == 3)) &&
                (profile_is_input || profile_is_display) && !uses_3d_luts); }
      /* Returns true if the ICC profile conforms to the restricted class
         of ICC profiles allowed within baseline JP2 files. */
    bool get_lut(int channel_idx, float lut[], int index_bits);
    bool get_matrix(float matrix3x3[]);
      /* These functions are used to recover the tone reproduction curves
         (for 1 colour and 3 colour spaces) and the PCS matrix (for 3 colour
         spaces) for the purpose of performing colour conversion.  The
         functions return false if the relevant matrices do not exist, which
         generally means that Kakadu's `jp2_colour' object will not be able
         to offer colour conversion services. */
  private: /* Read functions: offset in bytes relative to start of buffer;
              returns false if requested address is beyond end of buffer. */
    bool read(kdu_byte &val, int offset)
      { if (offset >= num_buffer_bytes) return false;
        val = buffer[offset];
        return true; }
    bool read(kdu_uint16 &val, int offset)
      { if (offset >= (num_buffer_bytes-1)) return false;
        val = buffer[offset]; val = (val<<8) + buffer[offset+1];
        return true; }
    bool read(kdu_uint32 &val, int offset)
      { if (offset >= (num_buffer_bytes-3)) return false;
        val = buffer[offset]; val = (val<<8) + buffer[offset+1];
        val = (val<<8) + buffer[offset+2]; val = (val<<8) + buffer[offset+3];
        return true; }
  private: // Other helper functions
    int get_curve_data_offset(int tag_offset, int tag_length);
      /* Checks the tag signature and length information, generating
         an error if anything is wrong.  Then returns the offset of
         the data portion of the curve (starts with the number of
         points identifer). */
    int get_xyz_data_offset(int tag_offset, int tag_length);
      /* Checks the tag signature and length information, generating
         an error if anything is wrong.  Then returns the offset of
         the first XYZ number in the tag. */
  private: // Data
    kdu_byte *buffer;
    int num_buffer_bytes;
    int num_colours; // must be either 1 or 3.
    int num_tags;
    bool pcs_is_xyz; // Currently can only convert if the PCS is XYZ
    bool profile_is_input; // Can convert input profiles, unless using 3D LUTs
    bool profile_is_display; // Can convert display profiles
    bool profile_is_output; // Cannot currently convert output profiles
    bool uses_3d_luts; // If true, we cannot convert
    int trc_offsets[3]; // Offset to data portion of each trc curve.
    int colorant_offsets[3]; // Offset to the XYZ data.
  };

/*****************************************************************************/
/*                            j2_colour_converter                            */
/*****************************************************************************/

class j2_colour_converter {
  public: // Member functions
    j2_colour_converter(j2_colour *colour, bool use_wide_gamut,
                        bool prefer_fast_approximations);
      /* Upon return, if colour conversion was not possible, the
         `num_conversion_colours' member will be 0.  The caller may then
         delete the created object. */
    ~j2_colour_converter();
  private: // Helper functions
    void configure_YCbCr_transform(double yoff, double yscale,
                 double cboff, double cbscale, double croff, double crscale);
      /* Sets up an opponent transform which is identical to the Part1
         ICT (irreversible colour transform) except that the channels are
         scaled by the values `yscale', `cbscale' and `crscale', and they
         are then offset by the addition of `yoff', `cboff' and `croff' to
         obtain the channel values which are represented by the compressed
         image.  The configured opponent transform actually serves to invert
         this relationship, working back to RGB. */
    void configure_YPbPr_transform(double yoff, double yscale,
                 double cboff, double cbscale, double croff, double crscale);
      /* Same as `configure_YCbCr_transform', except that the underlying
         normalized opponent transform is that adopted by SMPTE-240M for
         the YPbPr colour space used in HDTV systems, instead of the YCbCr
         transform used for the JPEG2000 Part1 ICT. */
    void configure_d65_primary_transform(const double xy_red[],
                   const double xy_green[], const double xy_blue[],
                   double gamma, double beta);
      /* Sets up the tone curves, srgb curve and primary transform matrix
         to represent the transformations required to map non-linear RGB
         primaries with the indicated gamma function and chromaticities to
         the sRGB primaries.  Note that (xy_red[0],xy_red[1]),
         (xy_green[0],xy_green[1]) and (xy_blue[0],xy_blue[1]) are the
         red, green and blue chromaticities, respectively.  All colorimetry is
         matched to the D65 illuminant here so no appearance transformation
         is required to match the sRGB illuminant (it is also D65).  All
         representations have the D65 whitepoint. */
    bool configure_icc_primary_transform(j2_colour *colour);
      /* Uses the embedded ICC profile to configure the tone curves and
         primary transform matrix to convert from compressed colour channels
         to sRGB channels.  Returns false, if conversion from the embedded
         ICC profile is not possible or sufficiently reliable. */
    bool configure_lab_transform(j2_colour *colour);
      /* Uses the `range', `offset', `illuminant' and `temperature' members
         to configure the Lab opponent and primary transformation members to
         provide conversion from Lab channel data to sRGB.  Returns false if
         the conversion cannot be found -- this currently happens only if
         an illuminant other than D50 or D65 is specified. */
  private: // Data
    friend class jp2_colour_converter;
    bool wide_gamut; // If lookup tables have extended ranges (2x #indices)
    int lut_idx_bits; // Tone curve LUT's use `lut_idx_bits' bits
    int num_conversion_colours;
    bool conversion_is_approximate;
    bool skip_opponent_transform; // True if next four members can be ignored
    bool use_ict; // True if opponent xform is identical to the Part 1 ICT
    bool have_k; // True if a fourth (K=black) channel is available.
    float opponent_offset_f[3]; // Subtract these from YCC data first
    kdu_int32 opponent_offset[3]; // Fixed-point version of the above vector
    float opponent_matrix_f[9]; // Conversion from YCC to non-linear RGB
    kdu_int32 opponent_matrix[9]; // Fixed-point version of the above matrix
    bool skip_primary_transform; // True if following matrix is the identity
    kdu_int16 *tone_curves[3]; // LUTs to convert from non-linear to linear RGB
    bool skip_primary_matrix; // True if following matrix is the identity
    float primary_matrix_f[9]; // Conversion from linear RGB to linear sRGB
    kdu_int32 primary_matrix[9]; // Fixed-point version of the above matrix
    kdu_int16 *srgb_curve; // One LUT to convert linear sRGB to sRGB
    kdu_int16 *lum_curve;
  };
  /* Notes:
     If a single channel (luminance) colour space has been used, at most the
     `lum_curve' member will be non-NULL -- see below.
        All floating point matrices and offsets are based on the assumption
     that all channel samples have a nominal range from -0.5 to +0.5.  The
     actual channel samples may exceed this nominal range by some margin
     since (for example) 16-bit fixed-point quantities have nominally only
     KDU_FIX_POINT bits (current value of KDU_FIX_POINT is 13 bits).  In
     this case, no clipping is performed until the very end of the colour
     conversion chain, except where intermediate quantities must be clipped
     to avoid overflow problems.  Note that the nominal range from -0.5 to
     +0.5 means that the nominal level for zero intensity is -0.5, not 0.
     Note also, though, that some colour spaces have additional offsets and
     range reduction factors to allow for the representation of out-of-gamut
     colours.  For such colour spaces, the value associated with black might
     be larger than -0.5, but computation is performed for values over the
     full range from -0.5 to +0.5.
         Working with the floating point quantities only, for the moment,
     colour data is processed as follows to convert it to sRGB.
     First, we add the `opponent_offset_f' vector to the input colour vector
     (the vector has dimension 3).  Next, we multiply on the left by
     `opponent_matrix_f', interpreting the 9 values as a raster-scan of the
     3x3 inverse opponent transform matrix.  The values produced by this
     matrix represent channel primaries (usually gamma corrected) which are
     also in the range -0.5 to +0.5.  These are passed through the tone
     curves, which map the channel primaries to linear channel primaries,
     this time with a nominal range from 0 to 1.  The linear primaries are
     then multiplied on the left by `primary_matrix_f', which produces linear
     sRGB primary channels, again in the range 0 to 1, appearance corrected
     for D65.  Finally, the linear sRGB primaries are subjected to the
     standard sRGB tone reproduction curve.
        We now describe how the above operations are implemented in fixed-point
     arithmetic, and with integer lookup tables.  The channel samples are
     now interpreted as fixed-point quantities having KDU_FIX_POINT nominal
     fraction bits.  Thus, the integer value X must be divided by
     2^`KDU_FIX_POINT' in order to obtain the floating-point value x, which
     has the nominal range -0.5 to +0.5.
        The entries in `opponent_offset' are obtained by scaling the entries
     of `opponent_offset_f' by 2^`KDU_FIX_POINT'.  The entries of
     `opponent_matrix' are obtained by scaling the entries in
     `opponent_matrix_f' all by 2^12.  After integer multiplication and
     addition using these integer matrix quantities, the resulting integers
     should be divided by 2^12, leaving quantities with a nominal range
     from -2^{KDU_FIX_POINT-1} to +2^{KDU_FIX_POINT-1}-1.
        If `skip_opponent_xform' is false, the above operations can be skipped
     (offsets are all 0 and the matrix is the identity).
        If `use_ict' is true, the above operations represent the regular
     irreversible colour transform (ICT) of JPEG2000 Part 1 and so they
     are implemented by calling the (generally much more efficient)
     `kdu_convert_ycc_to_rgb' function.
        The `tone_curves' member contains one tone curve for each channel,
     although if any entry is NULL, the tone curve referenced by
     `tone_curves'[0] should be used.  Each tone curve behaves as a lookup
     table with 2^KDU_FIX_POINT entries.  The values produced at the output
     of the opponent transform serve as indices to these lookup tables, after
     the addition of 2^{KDU_FIX_POINT-1}. Values which exceed 2^KDU_FIX_POINT-1
     are clipped.   Negative values have their amplitude processed by the tone
     curve, while the sign information is preserved.  This allows out-of-gamut
     colours to be preserved, which is important for some colour spaces.
        The outputs of the tone curves have a nominal range from 0 to
     2^KDU_FIX_POINT-1, and the values of the `primary_matrix' are obtained
     by scaling the values of the `primary_matrix_f' matrix by 2^12. The
     outputs of that matrix are divided by 2^12, leaving integers with a
     nominal range from 0 to 2^KDU_FIX_POINT-1; again, values which are out
     of range may occur.  The outputs are clipped to KDU_FIX_POINT-bit
     unsigned integers and used as indices to the `srgb_curve' lookup tables,
     the outputs of which are signed fixed-point sRGB colour samples with
     KDU_FIX_POINT fraction bits, with a nominal range from
     -2^{KDU_FIX_POINT-1} to 2^{KDU_FIX_POINT-1}.
        If `skip_primary_transform' is true, none of the above operations are
     performed.
        If `skip_primary_matrix' is true, the `primary_matrix' transform
     is skipped and `srgb_curve' should be NULL.  In this case, the effects
     of the tone curves and srgb_curve are combined into the tone curves,
     whose outputs are signed fixed-point quantities with KDU_FIX_POINT
     fraction bits.
        The above transformations are applied only when producing transformed
     RGB data.  For luminance data, where `num_colours'=1, it is sufficient
     to do everything with a single lookup table.  In this case, the
     `lum_curve' array is used.  If this array is NULL, no transformation
     whatsoever is required.  Otherwise, we first add 2^{`KDU_FIX_POINT'-1}
     to the input samples, clip the resulting values to unsigned integers
     in the range 0 to 2^{`KDU_FIX_POINT'}-1, and use these as indices to
     the lookup table.  The output from the table is again a signed
     fixed-point quantity, with a nominal range from -2^{`KDU_FIX_POINT'-1}
     to +2^{`KDU_FIX_POINT'-1}-1. */

/*****************************************************************************/
/*                                 j2_header                                 */
/*****************************************************************************/

class j2_header {
  private: // Data
    friend class jp2_header;
    jp2_input_box sub_box; // Used for processing sub-boxes of the header box.
    jp2_input_box *hdr; // Points to box supplied to `read' until it succeeds
    j2_dimensions dimensions;
    j2_colour colour;
    j2_palette palette;
    j2_component_map component_map;
    j2_channels channels;
    j2_resolution resolution;
  };

#endif // JP2_LOCAL_H
