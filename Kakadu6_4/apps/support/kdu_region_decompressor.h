/******************************************************************************/
// File: kdu_region_decompressor.h [scope = APPS/SUPPORT]
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
   Defines incremental, robust, region-based decompression services through
the `kdu_region_decompressor' object.  These services should prove useful
to many interactive applications which require JPEG2000 rendering capabilities.
*******************************************************************************/

#ifndef KDU_REGION_DECOMPRESSOR_H
#define KDU_REGION_DECOMPRESSOR_H

#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "jp2.h"

// Objects declared here
struct kdu_channel_mapping;
class kdu_region_decompressor;

// Objects declared elsewhere
struct kdrd_tile_bank;
struct kdrd_channel;
struct kdrd_component;

/******************************************************************************/
/*                             kdu_channel_mapping                            */
/******************************************************************************/

struct kdu_channel_mapping {
  /* [BIND: reference]
     [SYNOPSIS]
     This structure provides all information required to express the
     relationship between code-stream image components and the colour
     channels to be produced during rendering.  In the simplest case, each
     colour channel (red, green and blue, or luminance) is directly
     assigned to a single code-stream component.  More generally, component
     samples may need to be mapped through a pallete lookup table, or a
     colour space transformation might be required (e.g., the components or
     palette outputs might describe the image in terms of a custom colour
     space defined through an ICC profile).
     [//]
     The purpose of this structure is to capture the reproduction functions
     required for correct colour reproduction, so that they can be passed to
     the `kdr_region_decompressor::start' function.
     [//]
     This structure also serves to capture any information concerning opacity
     (alpha) channels.
  */
  //----------------------------------------------------------------------------
  public: // Member functions
    KDU_AUX_EXPORT kdu_channel_mapping();
      /* [SYNOPSIS]
           Constructs an empty mapping object.  Use the `configure' function
           or else fill out the data members of this structure manually before
           passing the object into `kdu_region_decompressor'.  To return to the
           empty state, use the `clear' function.
      */
    ~kdu_channel_mapping() { clear(); }
      /* [SYNOPSIS]
           Calls `clear', which will delete any lookup tables
           referenced from the `palette' array.
      */
    KDU_AUX_EXPORT void clear();
      /* [SYNOPSIS]
           Returns all data members to the empty state created by the
           constructor, deleting any lookup tables which may have been
           installed in the `palette' array.
      */
    KDU_AUX_EXPORT bool configure(kdu_codestream codestream);
      /* [SYNOPSIS]
           Configures the channel mapping information based upon the
           dimensions of the supplied raw code-stream.  Since no explicit
           channel mapping information is available from a wrapping file
           format, the function assumes that the first 3 output image
           components represent red, green and blue colour channels, unless
           they have different dimensions or there are fewer than 3 components,
           in which case the function treats the first component as a
           luminance channel and ignores the others.
         [RETURNS]
           All versions of this overloaded function return true if successful.
           This version always returns true.
      */
    KDU_AUX_EXPORT bool
      configure(jp2_colour colour, jp2_channels channels,
                int codestream_idx, jp2_palette palette,
                jp2_dimensions codestream_dimensions);
      /* [SYNOPSIS]
           Configures the channel mapping information based on the supplied
           colour, palette and channel binding information.  The object is
           configured only for colour rendering, regardless of whether the
           `channels' object identifies the existence of opacity channels.
           However, you may augment the configuration with alpha information
           at a later time by calling `add_alpha_to_configuration'.
         [ARG: codestream_idx]
           A JPX source may have multiple codestreams associated with a given
           compositing layer.  This argument identifies the index of the
           codestream which is to be used with the present configuration when
           it is supplied to `kdu_region_decompressor'.  The identifier is
           compared against the codestream identifiers returned via
           `channels.get_colour_mapping' and `channels.get_opacity_mapping'.
         [ARG: palette]
           Used to supply the `jp2_palette' interface associated with the
           codestream which is identified by `codestream_idx'.  For JP2 files,
           there is only one recognized palette and one recognized codestream
           having index 0.  For JPX files, each codestream is associated with
           its own palette.
         [ARG: codestream_dimensions]
           Used to supply the `jp2_dimensions' interface associated with the
           codestream which is identified by `codestream_idx'.  For JP2 files,
           there is only one recognized codestream having index 0.  For JPX
           files, each codestream has its own set of dimensions.  The
           present object uses the `codestream_dimensions' interface only
           to obtain default rendering precision and signed/unsigned
           information to use in the event that
           `kdu_region_decompressor::process' is invoked using a
           `precision_bits' argument of 0.
         [RETURNS]
           All versions of this overloaded function return true if successful.
           This version returns false only if the colour space cannot be
           rendered.  This gives the caller an opportunity to provide an
           alternate colour space for rendering.  JPX data sources, for example,
           may provide several related colour descriptions for a single
           compositing layer.  If any other error occurs, the function
           may invoke `kdu_error' rather than returninig -- this in turn
           may throw an exception which the caller can catch.
      */
    KDU_AUX_EXPORT bool configure(jp2_source *jp2_in, bool ignore_alpha);
      /* [SYNOPSIS]
           Simplified interface to the second form of the `configure' function
           above, based on a simple JP2 data source.  Automatically invokes
           `add_alpha_to_configuration' if `ignore_alpha' is false.  Note
           that `add_alpha_to_configuration' is invoked with the
           `ignore_premultiplied_alpha' argument set to true.  If you want
           to add both regular opacity and premultiplied opacity channels,
           you should call the `add_alpha_to_configuration' function
           explicitly.
         [RETURNS]
           All versions of this overloaded function return true if successful.
           This version always returns true.  If an error occurs, or the
           colour space cannot be converted, it generates an error through
           `kdu_error' rather than returning false.  If you would like the
           function to return false when the colour space cannot be rendered,
           use the second form of the `configure' function instead.
      */
    KDU_AUX_EXPORT bool
      add_alpha_to_configuration(jp2_channels channels, int codestream_idx,
                                 jp2_palette palette,
                                 jp2_dimensions codestream_dimensions,
                                 bool ignore_premultiplied_alpha=true);
      /* [SYNOPSIS]
           Unlike the `configure' functions, this function does not call
           `clear' automatically.  Instead, it tries to add an alpha channel
           to whatever configuration already exists.  It is legal (although
           not useful) to call this function even if an alpha channel has
           already been configured, since the function strips back the current
           configuration to include only the colour channels before looking
           to add an alpha channel.  Note, however, that it is perfectly
           acceptable (and quite useful) to add an alpha channel to a
           configuration which has just been cleared, so that only alpha
           information is recovered, without any colour information.
           [//]
           If a single alpha component cannot be found in the `channels'
           object, the function returns false.  This happens if there is no
           alpha (opacity) information, or if multiple distinct alpha channels
           are identified by `channels', or if the single alpha channel does
           not use the indicated codestream.  The `palette' must be provided
           since an alpha channel may be formed by passing the relevant image
           component samples through one of the palette lookup tables.
           [//]
           JP2/JPX files may identify alpha (opacity) information as
           premultiplied or not premultiplied.  These are sometimes known
           as associated and unassociated alpha.  The
           `ignore_premultiplied_alpha' argument controls which form of
           alpha information you are interested in adding, as indicated
           in the argument description below.  For compatibility with
           earlier versions of the function, this argument defaults to true.
         [RETURNS]
           True only if there is exactly one opacity channel and it is based
           on the indicated codestream.  Otherwise, the function returns
           false, adding no channels to the configuration.
         [ARG: ignore_premultiplied_alpha]
           If true, only unassociated alpha channels are examined, as
           reported by the `jp2_channels::get_opacity_mapping' function.
           Otherwise, both regular opacity and premultiplied opacity are
           considered, as reported by the `jp2_channels::get_premult_mapping'
           function.  To discover which type of alpha is being installed,
           you can call the function first with `ignore_premultiplied_alpha'
           set to true and then, if this call returns false, again with
           `ignore_premultiplied_alpha' set to false.
      */
  //----------------------------------------------------------------------------
  public: // Data
    int num_channels;
      /* [SYNOPSIS]
           Total number of channels to render, including colour channels and
           opacity channels.  The `configure' function will set this member
           to 1 or 3 if there is no alpha channel, and 2 or 4 if there is
           an alpha channel, depending on whether the source is monochrome
           or colour.  However, you may manually configure any number of
           channels you like.
      */
    int get_num_channels() { return num_channels; }
      /* [SYNOPSIS] Returns the value of the public member variable,
         `num_channels'.  Useful for function-only language bindings. */
    KDU_AUX_EXPORT void set_num_channels(int num);
      /* [SYNOPSIS]
           Convenience function for allocating the `source_components' and
           `palette' arrays required to hold the channel properties.  Also
           sets the `num_channels' member.  Copies any existing channel
           data, so that you can build up a channel description progressively
           by calling this function multiple times.  Initializes all new
           `source_components' entries to -1, all new `palette' entries
           to NULL, all new `default_rendering_precision' entries to 8 and
           all new `default_rendering_signed' entries to false.
      */
    int num_colour_channels;
      /* [SYNOPSIS]
           Indicates the number of initial channels which are used to describe
           pixel colour.  This might be smaller than `num_channels' if, for
           example, opacity channels are to be rendered.  All channels are
           processed in the same way, except in the event that colour space
           conversion is required.
      */
    int get_num_colour_channels() { return num_colour_channels; }
      /* [SYNOPSIS] Returns the value of the public member variable,
         `num_colour_channels'.  Useful for function-only language bindings. */
    int *source_components;
      /* [SYNOPSIS]
           Array holding the indices of the codestream's output image
           components which are used to form each of the colour channels.
           There must be one entry for each channel, although multiple channels
           may reference the same component.  Also, the mapping between source
           component samples and channel sample values need not be direct.
      */
    int get_source_component(int n)
      { return ((n>=0) && (n<num_channels))?source_components[n]:-1; }
      /* [SYNOPSIS]
           Returns entry `n' of the public `source_components' array,
           or -1 if `n' lies outside the range 0 to `num_channels'-1.
           This function is useful for function-only language bindings. */
    int *default_rendering_precision;
      /* [SYNOPSIS]
           Indicates the default precision with which to represent the sample
           values returned by the `kdu_region_decompressor::process' function
           in the event that it is invoked with a `precision_bits' argument
           of 0.  A separate precision is provided for each channel.  The
           `configure' functions set these entries up with the bit-depth
           values available from the file format or code-stream headers, as
           appropriate, but you can change them to suit the needs of your
           application. */
    bool *default_rendering_signed;
      /* [SYNOPSIS]
           Similar to `default_rendering_precision', but used to identify
           whether or not each channel's samples should be rendered as signed
           quantities when the `kdu_region_decompressor::process' function
           is invoked with a zero-valued `precision_bits' argument.  The
           `configure' functions set these entries up with values recovered
           from the file format or code-stream headers, as appropriate, but
           you can change them to suit the needs of your application.
      */
    int get_default_rendering_precision(int n)
      { return ((n>=0) && (n<num_channels))?default_rendering_precision[n]:0; }
      /* [SYNOPSIS]
           Returns entry `n' of the public `default_rendering_precision'
           array, or 0 if `n' lies outside the range 0 to `num_channels'-1.
           This function is useful for function-only language bindings. */
    bool get_default_rendering_signed(int n)
      { return ((n>=0) && (n<num_channels))?default_rendering_signed[n]:false; }
      /* [SYNOPSIS]
           Returns entry `n' of the public `default_rendering_signed'
           array, or false if `n' lies outside the range 0 to `num_channels'-1.
           This function is useful for function-only language bindings. */
    int palette_bits;
      /* [SYNOPSIS]
           Number of index bits used with any palette lookup tables found in
           the `palette' array.
      */
    kdu_sample16 **palette;
      /* [SYNOPSIS]
           Array with `num_channels' entries, each of which is either NULL or
           else a pointer to a lookup table with 2^{`palette_bits'} entries.
           [//]
           Note carefully that each lookup table must have a unique buffer,
           even if all lookup tables hold identical contents.  The buffer must
           be allocated using `new', since it will automatically be de-allocated
           using `delete' when the present object is destroyed, or its `clear'
           function is called.
           [//]
           If `palette_bits' is non-zero and one or more entries in the
           `palette' array are non-NULL, the corresponding colour channels
           are recovered by scaling the relevant code-stream image component
           samples (see `source_components') to the range 0 through
           2^{`palette_bits'}-1 and applying them (as indices) to the relevant
           lookup tables.  If the code-stream image component has an unsigned
           representation (this is common), the signed samples recovered from
           `kdu_synthesis' or `kdu_decoder' will be level adjusted to unsigned
           values before applying them as indices to a palette lookup table.
           [//]
           The entries in any palette lookup table are 16-bit fixed point
           values, having KDU_FIX_POINT fraction bits and representing
           normalized quantities having the nominal range of -0.5 to +0.5.
      */
    int *palette_bit_depth;
      /* [SYNOPSIS]
           Array with `num_channels' entries, each of which holds either 0
           or the bit-depth that was originally associated with the
           corresponding `palette' lookup table.  For each channel index c,
           in the range 0 to `num_channels'-1, if `palette'[c] is NULL, the
           value of `palette_bit_depth'[c] is ignored.  For  channels c, which
           do involve a palette, the value of `palette_bit_depth'[c] is the
           bit-depth B, associated with the palette outputs, as originally
           expressed within a `jp2_palette' object. What this means is that
           palette values in the range -2^{B-1} to (2^{B-1} - 1) have been
           scaled by 2^{KDU_FIX_POINT - B} to obtain the values in the
           corresponding `palette' lookup table.  The value of B is important
           only if the palette values need to be stretched to precisely fit
           a given range.  This is because the scaled values in the `palette'
           lookup table occupy the range -2^{P-1} to
           (2^{P-1} - 2^{KDU_FIX_POINT-B}) which is very weakly dependent upon
           B.  If `palette_bit_depth'[c] is 0, B is taken to be infinite,
           meaning that no stretching will ever be performed.  Stretching is
           only relevant if the `kdu_region_decompressor::set_white_stretch'
           function is passed a value larger than the original palette
           bit-depth, or if the floating point version of the
           `kdu_region_decompressor::process' function is to be employed.
      */
    jp2_colour_converter colour_converter;
      /* [SYNOPSIS]
           Initialized to an empty interface (`colour_converter.exists' returns
           false), you may call `colour_converter.init' to provide colour
           transformation capabilities.  This object is used by reference
           within `kdu_region_decompressor', so you should not alter its
           state while still engaged in region processing.
           [//]
           This object is initialized by the 2'nd and 3'rd forms of the
           `configure' function to convert the JP2 or JPX colour representation
           to sRGB, if possible.
      */
    jp2_colour_converter *get_colour_converter() { return &colour_converter; }
      /* [SYNOPSIS]
            Returns a pointer to the public member variable `colour_converter';
            useful for function-only language bindings.
      */
  };

/******************************************************************************/
/*                            kdu_region_decompressor                         */
/******************************************************************************/

class kdu_region_decompressor {
  /* [BIND: reference]
     [SYNOPSIS]
       Objects of this class provide a powerful mechanism for interacting with
       JPEG2000 compressed imagery.  They are particularly suitable for
       applications requiring interactive decompression, such as browsers
       and image editors, although there may be many other applications for
       the object.  The object abstracts all details associated with opening
       and closing tiles, colour transformations, interpolation (possibly
       by different amounts for each code-stream component) and determining
       the elements which are required to recover a given region of interest
       within the image.
       [//]
       The object also manages all state information required
       to process any selected image region (at any given scale) incrementally
       through multiple invocations of the `process' function.  This allows
       the CPU-intensive decompression operations to be interleaved with other
       tasks (e.g., user event processing) to maintain the responsiveness
       of an interactive application.
       [//]
       The implementation here is entirely platform independent, even though
       it may often be embedded inside applications which contain platform
       dependent code to manage graphical user interfaces.
       [//]
       From Kakadu version 5.1, this object offers multi-threaded processing
       capabilities for enhanced throughput.  These capabilities are based
       upon the options for multi-threaded processing offered by the
       underlying `kdu_multi_synthesis' object and the `kdu_synthesis' and
       `kdu_decoder' objects which it, in turn, uses.  Multi-threaded
       processing provides the greatest benefit on platforms with multiple
       physical CPU's, or where CPU's offer hyperthreading capabilities.
       Interestingly, although hyper-threading is often reported as offering
       relatively little gain, Kakadu's multi-threading model is typically
       able to squeeze 30-50% speedup out of processors which offer
       hyperthreading, in addition to the benefits which can be reaped from
       true multi-processor (or multi-core) architectures.  Even on platforms
       which do not offer either multiple CPU's or a single hyperthreading
       CPU, multi-threaded processing could be beneficial, depending on other
       bottlenecks which your decompressed imagery might encounter -- this is
       because underlying decompression tasks can proceed while other steps
       might be blocked on I/O conditions, for example.
       [//]
       To take advantage of multi-threading, you need to create a
       `kdu_thread_env' object, add a suitable number of working threads to
       it (see comments appearing with the definition of `kdu_thread_env') and
       pass it into the `start' function.  You can re-use this `kdu_thread_env'
       object as often as you like -- that is, you need not tear down and
       recreate the collaborating multi-threaded environment between calls to
       `finish' and `start'.  Multi-threading could not be much simpler.  The
       only thing you do need to remember is that all calls to `start',
       `process' and `finish' should be executed from the same thread -- the
       one identified by the `kdu_thread_env' reference passed to `start'.
       This constraint represents a slight loss of flexibility with respect
       to the core processing objects such as `kdu_multi_synthesis', which
       allow calls from any thread.  In exchange, however, you get simplicity.
       In particular, you only need to pass the `kdu_thread_env' object into
       the `start' function, after which the object remembers the thread
       reference for you.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdu_region_decompressor();
    KDU_AUX_EXPORT virtual ~kdu_region_decompressor();
      /* [SYNOPSIS]
           Deallocates any resources which might have been left behind if
           a `finish' call was still pending when the object was destroyed.
      */
    KDU_AUX_EXPORT void
      get_safe_expansion_factors(kdu_codestream codestream,
         kdu_channel_mapping *mapping, int single_component, int discard_levels,
         double &min_prod, double &max_x, double &max_y,
         kdu_component_access_mode access_mode=KDU_WANT_OUTPUT_COMPONENTS);
      /* [SYNOPSIS]
           As explained in connection with the `start' function, the internal
           implementation is unable to handle truly massive resolution
           reductions.  This function may be used to discover a safe lower
           bound for the amount of expansion (`expand_numerator' /
           `expand_denominator') passed to `start'.  This lower bound is
           returned via `min_prod', which holds a safe minimum value for the
           product of (`expand_numerator.x' * `expand_numerator.y') /
           (`expand_denominator.x' * `expand_denominator.y') in a subsequent
           call to `start' or `get_rendered_image_dims'.  The bound is quite
           conservative, leaving a large margin for error in the approximation
           of real scaling factors by rational numbers (numerator /
           denominator).
           [//]
           It is also possible that the `expand_numerator' and
           `expand_denominator' parameters supplied to `start' or
           `get_rendered_image_dims' represent too large an expansion factor,
           so that one or both dimensions approach or exceed 2^31.  The
           internal coordinate management logic and resampling algorithm can
           fail under these circumstances.  The `max_x' and `max_y'
           parameter are used to return safe upper bounds on the rational
           expansion factor represented by the `expand_numerator' and
           `expand_denominator' parameters passed to `start'.
      */
    KDU_AUX_EXPORT static kdu_dims
      find_render_dims(kdu_dims codestream_region, kdu_coords ref_comp_subs,
                       kdu_coords ref_comp_expand_numerator,
                       kdu_coords ref_comp_expand_denominator);
      /* [SYNOPSIS]
           Find the region on the rendering grid which is associated with
           a supplied `codestream_region'.  The latter is expressed on the
           high resolution codestream canvas.  The function first identifies
           the corresponding region on the reference image component, whose
           sub-sampling factors (relative to the high resolution codestream
           canvas) are given by `ref_comp_subs'.  The function then applies
           the rational expansion factors given by `ref_comp_expand_numerator'
           and `ref_comp_expand_denominator'.
           [//]
           The region mapping conventions here are identical to those described
           for the `start' and `get_rendered_image_dims' functions.  In fact,
           this function is the single place in which dimensioning of
           rendered imagery is performed.  The function is declared static, so
           it can be used by other objects or applications without an
           instantiated instance of the `kdu_region_decompressor' class.
           [//]
           The specific coordinate transformations employed by this function
           are as follows.  Let [E,F), [E',F') and [Er,Fr) define half-open
           intervals on the high-resolution codestream canvas, the reference
           image component, and the rendering grid, respectively.  These
           intervals represent either the horizontal or vertical ordinate
           for the respective grid -- i.e., the same transformations apply
           in each direction.  Further, let N and D represent the values of
           `ref_comp_expand_numerator' and `ref_comp_expand_denominator' in
           the relevant direction, and let S denote the value of
           `ref_comp_subs' in the relevant direction.
           [>>] The function first applies the normal mapping between the high
                resolution codestream canvas and the reference image component,
                setting: E' = ceil(E/S) and F' = ceil(F/S).
           [>>] The function then applies the following the rational expansion
                factors as follows: Er = ceil(E'*N/D-H) and Fr = ceil(F'*N/D-H).
                Here, H is an approximately "half pixel" offset, given by
                H = floor((N-1)/2) / D.
           [//]
           The coordinate mapping process described above can be interpreted
           as follows.  Let x be the location of a sample on the reference
           image component.  This sample has the location x*S on the high
           resolution codestream canvas and belongs to the region [E,F) if
           and only if E <= x*S < F.  Although the rational expansion factors
           can be contractive (i.e., N can be smaller than D), we will refer
           to the transformation from [E',F') to [Er,Fr) as "expansion".
           During this "expansion" process, the sample at location x is
           considered to occupy the region [x*N/D-H,(x+1)*N/D-H) on the
           rendering grid.  The regions associated with each successive
           integer-valued x are thus disjoint and contiguous -- note that
           for large N, H is almost exactly equal to 0.5*(N/D).  In this way,
           the region on the rendering grid which is occupied by image
           component samples within the interval [E',F') is given by the
           half-open interval [E'*(N/D)-H,F'*(N/D)-H).  The determined range
           of rendering grid points [Er,Fr) is exactly the set of grid points
           whose integer locations fall within the above range.
      */
    KDU_AUX_EXPORT static kdu_coords
      find_codestream_point(kdu_coords render_point, kdu_coords ref_comp_subs,
                            kdu_coords ref_comp_expand_numerator,
                            kdu_coords ref_comp_expand_denominator);
      /* [SYNOPSIS]
           This function provides a uniform procedure for identifying
           a representative location on the high resolution codestream canvas
           which corresponds to a location (`render_point') on the rendering
           grid.
           [//]
           Considering the conventions implemented by the `find_render_dims'
           function, a sample on the reference component, at location x
           (consider this as either the horizontal or vertical ordinate of
           the location) is considered to cover the half-open interval
           [x*N/D-H,(x+1)*N/D-H) on the reference grid.  Here, N and D are
           taken from the relevant ordinate of `ref_comp_expand_numerator'
           and `ref_comp_expand_denominator', respectively, and
           H = floor((N-1)/2) / D.  There is exactly one location x associated
           the `render_point' X.  This location satisfies x <= (X+H)*D/N < x+1,
           so x = floor((X+H)*D/N).
           [//]
           Assuming symmetric wavelet kernels, the location x on the reference
           image component has its centre of mass at location x*S on the
           high resolution codestream canvas, where S is the reference
           component sub-sampling factor found in the relevant ordinate of
           `ref_comp_subs'.  Regardless of wavelet kernel symmetry, it is
           appealing to adopt a policy in which locations within content
           rendered from sub-sampled data can only be associated with high
           resolution codestream canvas locations which are multiples of the
           sub-sampling factor.  For this reason, the horizontal and vertical
           components of the returned `kdu_coords' object are set to
           S * floor((X+H)*D/N), where S, X, N, D and H are obtained from
           the horizontal (resp. vertical) components of the supplied
           arguments, as described above.
      */
    KDU_AUX_EXPORT static kdu_coords
      find_render_point(kdu_coords codestream_point, kdu_coords ref_comp_subs,
                        kdu_coords ref_comp_expand_numerator,
                        kdu_coords ref_comp_expand_denominator);
      /* [SYNOPSIS]
           This function returns a single representative location on the
           rendering grid, corresponding to `codestream_point' on the
           high resolution codestream canvas.
           [//]
           The function first locates the `codestream_point' on the
           coordinate system of a "reference" image component which has the
           sub-sampling factors supplied by `ref_comp_subs'.  It then
           applies the expansion factors represented by
           `ref_comp_expand_numerator' and `ref_comp_expand_denominator' to
           determine the region on the rendering grid which is associated
           with the relevant sample on the reference image component.  Finally,
           the centroid of this region is taken to be the representative
           location on the rendering grid.
           [//]
           The specific transformations employed by this function are as
           follows.  Let X denote the horizontal or vertical ordinate of
           `codestream_point' (the same transformations are applied in each
           direction).  Assuming symmetric wavelet kernels, the closest
           corresponding point x on the reference image component is
           obtained from x = ceil(X/S - 0.5) = ceil((2*X-S)/(2*S)).
           [//]
           Considering the conventions described in conjunction with the
           `find_render_dims' function, the reference component sample at
           location x covers the half-open interval [x*N/D-H,(x+1)*N/D-H) on
           the reference grid.  Here, N and D are the relevant ordinates of
           `ref_comp_expand_numerator' and `ref_comp_expand_denominator',
           respectively, and H = floor((N-1)/2) / D.  The present function
           returns the centroid of this region, rounded to the "nearest"
           integer.  Specifically, the function returns
           Xr = floor((2*x+1)*N/(2*D) - H). 
      */
    KDU_AUX_EXPORT static kdu_dims
      find_render_cover_dims(kdu_dims codestream_dims,
                             kdu_coords ref_comp_subs,
                             kdu_coords ref_comp_expand_numerator,
                             kdu_coords ref_comp_expand_denominator)
        {
          kdu_coords tl_point = codestream_dims.pos;
          kdu_coords br_point = tl_point+codestream_dims.size-kdu_coords(1,1);
          tl_point = find_render_point(tl_point,ref_comp_subs,
                        ref_comp_expand_numerator,ref_comp_expand_denominator);
          br_point = find_render_point(br_point,ref_comp_subs,
                        ref_comp_expand_numerator,ref_comp_expand_denominator);
          kdu_dims result;  result.pos=tl_point;
          result.size = br_point-tl_point + kdu_coords(1,1);
          return result;
        }
      /* [SYNOPSIS]
           Returns the smallest region on the rendering grid which includes
           the locations which would be returned by `find_render_point' when
           invoked with every location within `codestream_dims'.  This is
           almost, but not quite exactly identical to `find_render_dims'.  The
           difference is subtle but important.  The `find_render_dims' function
           has the property that disjoint regions on the high resolution
           codestream canvas produce disjoint regions on the rendering grid.
           Unfortunately, though, this necessarily means that some individual
           points on the high resolution canvas may occupy an empty region on
           the rendering grid (this must be the case whenever the reference
           subsampling factors given by `ref_comp_subs' are greater than 1).
           However, the `find_render_point' function necessarily returns a
           rendering grid point for every point on the high resolution
           codestream canvas.  Where an application needs to be sure that
           all such points are included within a region on the rendering grid,
           this function should be used in preference to `find_render_dims'.
           On the other hand, the preservation of disjoint regions by
           `find_render_dims' is considered an important attribute for
           sizing rendered content within the `start' function.
      */
    KDU_AUX_EXPORT static kdu_dims
      find_codestream_cover_dims(kdu_dims render_dims,
                                 kdu_coords ref_comp_subs,
                                 kdu_coords ref_comp_expand_numerator,
                                 kdu_coords ref_comp_expand_denominator);
      /* [SYNOPSIS]
           This function finds the tightest region on the high resolution
           codestream canvas which includes all locations which would be mapped
           into the `render_dims' region by the `find_render_point' function.
           [//]
           Let x be the horizontal or vertical ordinate of a location on the
           reference image component.  As discussed in conjunction with
           `find_render_point', this location maps to the location Xr on the
           rendering grid, where Xr = floor((2*x+1)*N/(2*D) - H), where N and D
           are the corresponding ordinates from `ref_comp_expand_numerator'
           and `ref_comp_expand_denominator' and H = floor((N-1)/2) / D.
           Now let [Er,Fr) denote the half-open interval representing the
           corresponding ordinate from `render_dims'.
           [//]
           Write x_min for the smallest x for which Xr >= Er.  That is, x_min
           is the smallest x for which (2*x+1)*N/(2*D)-H >= Er; equivalently,
           x >= ((Er+H)*2D - N) / 2N.  So x_min = ceil(((Er+H)*2D-N)/2N).
           Write x_max for the largest x for which Xr <= Fr-1.  That is, x_max
           is the largest x for which (2*x+1)*N/(2*D)-H < Fr; equivalently,
           x < ((Fr+H)*2D - N) / 2N. So x_max = ceil(((Fr+H)*2D-N)/2N)-1.
           [//]
           From the above, we see that the range of locations on the reference
           grid which map to the half-open interval [Er,Fr) can be written [e,f)
           where e = ceil(((Er+H)*2D-N)/2N) and f = ceil(((Fr+H)*2D-N)/2N).
           Now write [E,F) for the range of locations on the high resolution
           codestream canvas which map to [e,f), using the rounding conventions
           adoped by `find_render_point'.  These are the locations X, such that
           x = ceil(X/S - 0.5) lies in [e,f), where S is the relevant ordinate
           from `ref_comp_subs'.  It follows that E is the minimum X such
           that ceil(X/S - 0.5) >= e; equivalently, E is the minimum X such
           that X/S - 0.5 > (e-1) -- i.e., X > (e-0.5)*S.  So
           E = floor((e-0.5)*S) + 1 = e*S + 1 - ceil(S/2).  Similarly,
           F-1 is the maximum X such that ceil(X/S - 0.5) <= f-1; equivalently,
           F-1 is the maximum X such that X/S - 0.5 <= f-1 -- i.e.,
           X <= S*(f-0.5).  So F = 1 + floor(S*(f-0.5)) = f*S + 1 - ceil(S/2).
      */
    KDU_AUX_EXPORT kdu_dims
      get_rendered_image_dims(kdu_codestream codestream,
           kdu_channel_mapping *mapping, int single_component,
           int discard_levels, kdu_coords expand_numerator,
           kdu_coords expand_denominator=kdu_coords(1,1),
           kdu_component_access_mode access_mode=KDU_WANT_OUTPUT_COMPONENTS);
      /* [SYNOPSIS]
           This function may be used to determine the size of the complete
           image on the rendering canvas, based upon the supplied component
           mapping rules, number of discarded resolution levels and rational
           expansion factors.  For further explanation of the rendering
           canvas and these various parameters, see the `start' function.
           The present function is provided to assist implementators in
           ensuring that the `region' they pass to `start' will indeed lie
           within the span of the image after all appropriate level
           discarding and expansion has taken place.
           [//]
           The function may not be called between `start' and `finish' -- i.e.
           while the object is open for processing.
      */
    kdu_dims get_rendered_image_dims() { return full_render_dims; }
      /* [SYNOPSIS]
           This version of the `get_rendered_image_dims' function returns the
           location and dimensions of the complete image on the rendering
           canvas, based on the parameters which were supplied to `start'.
           If the function is called prior to `start' or after `finish', it
           will return an empty region.
      */
    void set_white_stretch(int white_stretch_precision)
      { this->white_stretch_precision = white_stretch_precision; }
      /* [SYNOPSIS]
           For predictable behaviour, you should call this function only while
           the object is inactive -- i.e., before the first call to `start',
           or after the most recent call to `finish' and before any subsequent
           call to `start'.
           [//]
           The function sets up the "white stretch" mode which will become
           active when `start' is next called.  The same "white stretch" mode
           will be used in each subsequent call to `start' unless the present
           function is used to change it.  We could have added the information
           supplied by this function to the argument list accepted by `start',
           but the number of optional arguments offered by that function has
           already become large enough.
           [//]
           So, what is this "white stretch" mode, and what does it have to
           do with precision?  To explain this, we begin by explaining what
           happens if you set `white_stretch_precision' to 0 -- this is the
           default, which also corresponds to the object's behaviour prior
           to Kakadu Version 5.2 for backward compatibility.  Suppose the
           original image samples which were compressed had a precision of
           P bits/sample, as recorded in the codestream and/or reported by
           the `jp2_dimensions' interface to a JP2/JPX source.  The actual
           precision may differ from component to component, of course, but
           let's keep things simple for the moment).
           [//]
           For the purpose of colour transformations and conversion to
           different rendering precisions (as requested in the relevant call
           to `process'), Kakadu applies the uniform interpretation that
           unsigned quantities range from 0 (black) to 2^P (max intensity);
           signed quantities are assumed to lie in the nominal range of
           -2^{P-1} to 2^{P-1}.  Thus, for example:
           [>>] to render P=12 bit samples into a B=8 bit buffer, the object
                simply divides by 16 (downshifts by 4=P-B);
           [>>] to render P=4 bit samples into a B=8 bit buffer, the object
                multiplies by 16 (upshifts by 4=B-P); and
           [>>] to render P=1 bit samples into a B=8 bit buffer, the object
                multiplies by 128 (upshifts by 7=B-P).
           [//]
           This last example, reveals the weakness of a pure shifting scheme
           particularly well, since the maximum attainable value in the 8-bit
           rendering buffer from a 1-bit source will be 128, as opposed to the
           expected 255, leading to images which are darker than would be
           suspected.  Nevertheless, this policy has merits.  One important
           merit is that the original sample values are preserved exactly,
           so long as the rendering buffer has precision B >= P.  The
           policy also has minimal computational complexity and produces
           visually excellent results except for very low bit-depth images.
           Moreover, very low bit-depth images are often used to index
           a colour palette, which performs the conversion from low to high
           precision in exactly the manner prescribed by the image content
           creator.
           [//]
           Nevertheless, it is possible that low bit-depth image components
           are used without a colour palette and that applications will want
           to render them to higher bit-depth displays.  The most obvious
           example of this is palette-less bi-level images, but another
           example is presented by images which have a low precision associated
           alpha (opacity) channel.  To render such images in the most
           natural way, unsigned sample values with P < B should ideally be
           stretched by (1-2^{-B})/(1-2^{-P}), prior to rendering, in
           recognition of the fact that the maximum display output value is
           (1-2^{-B}) times the assumed range of 2^B, while the maximum
           intended component sample value (during content creation) was
           probably (1-2^{-P}) times the assumed range of 2^P.
           It is not entirely clear whether the same interpretations are
           reasonable for signed sample values, but the function extends the
           policy to both signed and unsigned samples by fixing the lower
           bound of the nominal range and stretching the length of the range
           by (1-2^{-B})/(1-2^{-P}).  In fact, the internal representation of
           all component samples is signed so as to optimally exploit the
           dynamic range of the available word sizes.
           [//]
           To facilitate the stretching procedure described above, the
           present function allows you to specify the value of B that you
           would like to be applied in the stretching process.  This is the
           value of the `white_stretch_precision' argument.  Normally, B
           will coincide with the value of the `precision_bits' argument
           you intend to supply to the `process' function (often B=8).
           However, you can use a different value.  In particular, using a
           smaller value for B here will reduce the likelihood that white
           stretching is applied (also reducing the accuracy of the stretch,
           of course), which may represent a useful computational saving
           for your application.  Also, when the target rendering precision is
           greater than 8 bits, it is unclear whether your application will
           want stretching or not -- if so, stretching to B=8 bits might be
           quite accurate enough for you, differing from the optimal stretch
           value by at most 1 part in 256.  The important thing to note is
           that stretching will occur only when the component sample precision
           P is less than B, so 8-bit original samples will be completely
           unchanged if you specify B <= 8.  In particular, the default
           value of B=0 means that no stretching will ever be applied.
           [//]
           As a final note, we point out that white stretching adds unnecessary
           complexity if you intend to use the floating point version of the
           `process' function.  This is because that function always performs
           white stretching to the associated precision -- see that function
           for more information.
         [ARG: white_stretch_precision]
           This argument holds the target stretching precision, B, explained
           in the discussion above.  White stretching will be applied only
           to image components (or JP2/JPX/MJ2 palette outputs) whose
           nominated precision P is less than B.
      */
    void set_interpolation_behaviour(float max_overshoot,
                                     int zero_overshoot_threshold)
      {
        if (zero_overshoot_threshold < 1) zero_overshoot_threshold = 1;
        if (max_overshoot < 0.0F) max_overshoot = 0.0F;
        this->max_interp_overshoot = max_overshoot;
        this->zero_overshoot_interp_threshold = zero_overshoot_threshold;
      }
      /* [SYNOPSIS]
           This function allows you to customize the way in which interpolation
           is performed when the `expand_denominator' and `expand_numerator'
           arguments supplied to `start' are not identical.  Interpolation is
           generally performed using 6-tap kernels which are optimized to
           approximate ideal band-limited filters whose bandwidth is
           a fraction BW of the full Nyquist bandwidth, where BW=max{ratio,1.0}
           and `ratio' is the expansion ratio determined from
           the `expand_denominator' and `expand_numerator' arguments passed to
           `start'.  Unfortunately, these interpolation operators generally
           have BIBO (Bounded-Input-Bounded-Output) gains significantly greater
           than 1, which means that they can expand the dynamic range of certain
           types of inputs.  The effect on hard step edges can be particularly
           annoying, especially under large expansion factors.
           [//]
           This function allows you to suppress such overshoot/undershoot
           artefacts based upon the amount of expansion which is occurring.
           The internal mechanism adjusts the interpolation kernels
           so that the maximum amount of undershoot or overshoot associated
           with interpolation of a step edge, is limited to at most
           `max_overshoot' (as a fraction of the dynamic range of the edge).
           Moreover, this limit on the maximum overshoot/undershoot is
           linearly decreased as a function of the expansion `ratio',
           starting from an expansion ratio of 1.0 and continuing until the
           expansion ratio reaches the `zero_overshoot_threshold' value.  At
           that point, the interpolation strategy reduces to bi-linear
           interpolation; i.e., the individual interpolation kernels used for
           interpolation in the horizontal and vertical directions are reduced
           to 2 taps with positive coefficients.  Such kernels have a BIBO
           gain which is exactly equal to their DC gain of 1.0.
           [//]
           By specifying a `zero_overshoot_threshold' which is less than
           or equal to 1, you can force the use of bi-linear interpolation for
           all expansive interpolation processes.  One side effect of this is
           that the interpolation may be somewhat faster (even much faster),
           depending upon the underlying machine architecture and details of
           the available SIMD accelerations which are available.
           [//]
           Changes associated with calls to this function may not have any
           effect until the next call to `start'.  If the function is never
           called, the current default values for the two parameters are
           `max_overshoot'=0.4 and `zero_overshoot_threshold'=2.
      */
    KDU_AUX_EXPORT bool
      start(kdu_codestream codestream, kdu_channel_mapping *mapping,
           int single_component, int discard_levels, int max_layers,
           kdu_dims region, kdu_coords expand_numerator,
           kdu_coords expand_denominator=kdu_coords(1,1), bool precise=false,
           kdu_component_access_mode access_mode=KDU_WANT_OUTPUT_COMPONENTS,
           bool fastest=false, kdu_thread_env *env=NULL,
           kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Prepares to decompress a new region of interest.  The actual
           decompression work is performed incrementally through successive
           calls to `process' and terminated by a call to `finish'.  This
           incremental processing strategy allows the decompression work to
           be interleaved with other tasks, e.g. to preserve the
           responsiveness of an interactive application.  There is no need
           to process the entire region of interest established by the
           present function call; `finish' may be called at any time, and
           processing restarted with a new region of interest.  This is
           particularly helpful in interactive applications, where an
           impatient user's interests may change before processing of an
           outstanding region is complete.
           [//]
           If `mapping' is NULL, a single image component is to be
           decompressed, whose index is identified by `single_component'.
           Otherwise, one or more image components must be decompressed and
           subjected to potentially quite complex mapping rules to generate
           channels for display; the relevant components and mapping
           rules are identified by the `mapping' object.
           [//]
           The `region' argument identifies the image region which is to be
           decompressed.  This region is defined with respect to a
           `rendering canvas'.  The rendering canvas might be identical to the
           high resolution canvas associated with the code-stream, but it is
           often different -- see below.
           [//]
           The `expand_numerator' and `expand_denominator' arguments identify
           the amount by which the first channel described by `mapping' (or
           the single image component if `mapping' is NULL) should be
           expanded to obtain its representation on the rendering canvas.
           To be more precise, let Cr be the index of the reference image
           component (the first one identified by `mapping' or the
           single image component if `mapping' is NULL).  Also, let (Px,Py)
           and (Sx,Sy) denote the upper left hand corner and the dimensions,
           respectively, of the region returned by `kdu_codestream::get_dims',
           for component Cr.  Note that these dimensions take into account
           the effects of the `discard_levels' argument, as well as any
           orientation adjustments created by calls to
           `kdu_codestream::change_appearance'.  The location and dimensions
           of the image on the rendering canvas are then given by
                    ( ceil(Px*Nx/Dx-Hx), ceil(Py*Ny/Dy-Hy) )
           and
                    ( ceil((Px+Sx)*Nx/Dx-Hx)-ceil(Px*Nx/Dx-Hx),
                      ceil((Py+Sy)*Ny/Dy-Hy)-ceil(Py*Ny/Dy-Hy) )
           respectively, where (Nx,Ny) are found in `expand_numerator',
           (Dx,Dy) are found in `expand_denominator', and (Hx,Hy) are
           intended to represent approximately "half pixel" offsets.
           Specifically, Hx=floor((Nx-1)/2)/Dx and Hy=floor((Ny-1)/2)/Dy.
           Since the above formulae can be tricky to reproduce precisely,
           the `get_rendered_image_dims' function is provided to learn the
           exact location and dimensions of the image on the rendering canvas.
           Moreover, since you may wish to provide corresponding
           transformations for other purposes (e.g., finding the locations on
           the rendering grid of regions of interest expressed in codestream
           canvas coordinates, or vice-versa), the `find_render_dims'
           function is provided for general use, along with related functions
           `find_codestream_point' and `find_render_point'.
           [//]
           You are required to ensure only that the `expand_numerator'
           and `expand_denominator' coordinates are strictly positive.
           From Kakadu version 6.2.2 it is no longer necessary to ensure
           that the `expand_numerator' coordinates are at least as large as
           `expand_denominator'.  In fact, you can use these parameters to
           implement almost any amount of expansion or reduction,
           but as a result you should use `get_rendered_image_dims' first to
           verify that you are not reducing the image down to nothing.
           [//]
           In the extreme case, if you select `expand_denominator' values
           which are vastly larger than the `expand_numerator', an error may
           be generated through `kdu_error'.  For example, if the product of
           the x and y members of `expand_denominator' exceeds the product
           of the `expand_numerator' x and y values by more than 2^24, an
           error is likely to be generated.  This is because such massive
           reduction factors may exceed the dynamic range of the internal
           implementation.  The actual limit on the amount of resolution
           reduction which can be realized depends upon factors such as the
           relative sub-sampling of the various image components required
           to construct colour channels.
           [//]
           As of Kakadu version 6.2.2, expansion and reduction are implemented
           in a disciplined manner, using aliasing suppressing interpolation
           kernels, designed using rigorous criteria.  We no longer use
           pixel replication or bilinear interpolation, which produce poor
           image quality.  However, we do allow you to configure some aspects
           of the interpolation process using the `set_interpolation_behaviour'
           function -- this allows you to suppress ringing artifacts which can
           be noticeable around step edges, especially under large expansion
           factors.
           [//]
           Note carefully that this function calls the
           `kdu_codestream::apply_input_restrictions' function, which will
           destroy any restrictions you may previously have imposed.  This
           may also alter the current component access mode interpretation.
           For this reason, the function provides you with a separate
           `access_mode' argument which you can set to one of
           `KDU_WANT_CODESTREAM_COMPONENTS' or `KDU_WANT_OUTPUT_COMPONENTS',
           to control the way in which image component indices should be
           interpreted and whether or not multi-component transformations
           defined at the level of the code-stream should be performed.
         [RETURNS]
           False if a fatal error occurred and an exception (usually generated
           from within the error handler associated with `kdu_error') was
           caught.  In this case, you need not call `finish', but you should
           generally destroy the codestream interface passed in here.
           [//]
           You can avoid errors which might be generated by inappropriate
           `expand_denominator'/`expand_numerator' parameters by first
           calling `get_safe_expansion_factors'.
         [ARG: codestream]
           Interface to the internal code-stream management machinery.  Must
           have been created (see `codestream.create') for input.
         [ARG: mapping]
           Points to a structure whose contents identify the relationship
           between image components and the channels to be rendered.  The
           interpretation of these image components depends upon the
           `access_mode' argument.  Any or all of the image
           components may be involved in rendering channels; these might be
           subjected to a palette lookup operation and/or specific colour space
           transformations.  The channels need not all represent colour, but
           the initial `mapping->num_colour_channels' channels do represent
           the colour of a pixel.
           [//]
           If the `mapping' pointer is NULL, only one image component is to
           be rendered, as a monochrome image; the relevant component's
           index is supplied via the `single_component' argument.
         [ARG: single_component]
           Ignored, unless `mapping' is NULL, in which case this argument
           identifies the image component (starting from 0) which
           is to be rendered as a monochrome image.  The interpretation of
           this image component index depends upon the `access_mode'
           argument.
         [ARG: discard_levels]
           Indicates the number of highest resolution levels to be discarded
           from each image component's DWT.  Each discarded level typically
           halves the dimensions of each image component, although note that
           JPEG2000 Part-2 supports downsampling factor styles in which
           only one of the two dimensions might be halved between levels.
           Recall that the rendering canvas is determined by applying the
           expansion factors represented by `expand_numerator' and
           `expand_denominator' to the dimensions of the reference image
           component, as it appears after taking the number of discarded
           resolution levels (and any appearance changes) into account.  Thus,
           each additional discarded resolution level serves to reduce the
           dimensions of the entire image as it would appear on the rendering
           canvas.
         [ARG: max_layers]
           Maximum number of quality layers to use when decompressing
           code-stream image components for rendering.  The actual number of
           layers which are available might be smaller or larger than this
           limit and may vary from tile to tile.
         [ARG: region]
           Location and dimensions of the new region of interest, expressed
           on the rendering canvas.  This region should be located
           within the region returned by `get_rendered_image_dims'.
         [ARG: expand_numerator]
           Numerator of the rational expansion factors which are applied to
           the reference image component.
         [ARG: expand_denominator]
           Denominator of the rational expansion factors which are applied to
           the reference image component.
         [ARG: precise]
           Setting this argument to true encourages the implementation to
           use higher precision internal representations when decompressing
           image components.  The precision of the internal representation
           is not directly coupled to the particular version of the overloaded
           `process' function which is to be used.  The lower precision
           `process' functions may be used with higher precision internal
           computations, or vice-versa, although it is natural to couple
           higher precision computations with calls to a higher precision
           `process' function.
           [//]
           Note that a higher precision internal representation may be adopted
           even if `precise' is false, if it is deemed to be necessary for
           correct decompression of some particular image component.  Note
           also that higher precision can be maintained throughout the entire
           process, only for channels whose contents are taken directly from
           decompressed image components.  If any palette lookup, or colour
           space conversion operations are required, the internal precision
           for those channels will be reduced (at the point of conversion) to
           16-bit fixed-point with `KDU_FIX_POINT' fraction bits -- due to
           current limitations in the `jp2_colour_converter' object.
           [//]
           Of course, most developers may remain blissfully ignorant of such
           subtleties, since they relate only to internal representations and
           approximations.
         [ARG: fastest]
           Setting this argument to true encourages the implementation to use
           lower precision internal representations when decompressing image
           components, even if this results in the loss of image quality.
           The argument is ignored unless `precise' is false.  The argument
           is essentially passed directly to `kdu_multi_synthesis::create',
           along with the `precises' argument, as that function's
           `want_fastest' and `force_precise' arguments, respectively.
         [ARG: access_mode]
           This argument is passed directly through to the
           `kdu_codestream::apply_input_restrictions' function.  It thus affects
           the way in which image components are interpreted, as found in the
           `single_component_idx' and `mapping' arguments.  For a detailed
           discussion of image component interpretations, consult the second
           form of the `kdu_codestream::apply_input_restrictions' function.
           It suffices here to mention that this argument must take one of
           the values `KDU_WANT_CODESTREAM_COMPONENTS' or
           `KDU_WANT_OUTPUT_COMPONENTS' -- for more applications the most
           appropriate value is `KDU_WANT_OUTPUT_COMPONENTS' since these
           correspond to the declared intentions of the original codestream
           creator.
         [ARG: env]
           This argument is used to establish multi-threaded processing.  For
           a discussion of the multi-threaded processing features offered
           by the present object, see the introductory comments to
           `kdu_region_decompressor'.  We remind you here, however, that
           all calls to `start', `process' and `finish' must be executed
           from the same thread, which is identified only in this function.
           For the single-threaded processing model used prior to Kakadu
           version 5.1, set this argument to NULL.
         [ARG: env_queue]
           This argument is ignored unless `env' is non-NULL, in which case
           a non-NULL `env_queue' means that all multi-threaded processing
           queues created inside the present object, by calls to `process',
           should be created as sub-queues of the identified `env_queue'.
           [//]
           One application for a non-NULL `env_queue' might be one which
           processes two frames of a video sequence in parallel.  There
           can be some benefit to doing this, since it can avoid the small
           amount of thread idle time which often appears at the end of
           the last call to the `process' function prior to `finish'.  In
           this case, each concurrent frame would have its own `env_queue', and
           its own `kdu_region_decompressor' object.  Moreover, the
           `env_queue' associated with a given `kdu_region_decompressor'
           object can be used to run a job which invokes the `start',
           `process' and `finish' member functions.  In this case, however,
           it is particularly important that the `start', `process' and
           `finish' functions all be called from within the execution of a
           single job, since otherwise there is no guarantee that they would
           all be executed from the same thread, whose importance has
           already been stated above.
      */
    KDU_AUX_EXPORT bool
      process(kdu_byte **channel_bufs, bool expand_monochrome,
              int pixel_gap, kdu_coords buffer_origin, int row_gap,
              int suggested_increment, int max_region_pixels,
              kdu_dims &incomplete_region, kdu_dims &new_region,
              int precision_bits=8, bool measure_row_gap_in_pixels=true);
      /* [SYNOPSIS]
           This powerful function is the workhorse of a typical interactive
           image rendering application.  It is used to incrementally decompress
           an active region into identified portions of one or more
           application-supplied buffers.  Each call to the function always
           decompresses some whole number of lines of one or more horizontally
           adjacent tiles, aiming to produce roughly the number of samples
           suggested by the `suggested_increment' argument, unless that value
           is smaller than a single line of the current tile, or larger than
           the number of samples in a row of horizontally adjacent tiles.
           The newly rendered samples are guaranteed to belong to a rectangular
           region -- the function returns this region via the
           `new_region' argument.  This, and all other regions manipulated
           by the function are expressed relative to the `rendering canvas'
           (the coordinate system associated with the `region' argument
           supplied to the `start' member function).
           [//]
           Decompressed samples are automatically colour transformed,
           clipped, level adjusted, interpolated and colour appearance
           transformed, as necessary.  The result is a collection of
           rendered image pixels, each of which has the number of channels
           described by the `kdu_channel_mapping::num_channels' member of
           the `kdu_channel_mapping' object passed to `start' (or just one
           channel, if no `kdu_channel_mapping' object was passed to `start').
           The initial `kdu_channel_mapping::num_colour_channels' of these
           describe the pixel colour, while any remaining channels describe
           auxiliary properties such as opacity.  Other than these few
           constraints, the properties of the channels are entirely determined
           by the way in which the application configures the
           `kdu_channel_mapping' object.
           [//]
           The rendered channel samples are written to the buffers supplied
           via the `channel_bufs' array.  If `expand_monochrome' is false,
           this array must have exactly one entry for each of the channels
           described by the `kdu_channel_mapping' object supplied to `start'.
           The entries may all point to offset locations within a single
           channel-interleaved rendering buffer, or else they may point to
           distinct buffers for each channel; this allows the application to
           render to buffers with a variety of interleaving conventions.
           [//]
           If `expand_monochrome' is true and the number of colour channels
           (see `kdu_channel_mapping::num_colour_channels') is exactly equal
           to 1, the function automatically copies the single (monochrome)
           colour channel into 3 identical colour channels whose buffers
           appear as the first three entries in the `channel_bufs' array.
           This is a convenience feature to support direct rendering of
           monochrome images into 24- or 32-bpp display buffers, with the
           same ease as full colour images.  Your application is not obliged
           to use this feature, of course.
           [//]
           Each buffer referenced by the `channel_bufs' array has horizontally
           adjacent pixels separated by `pixel_gap'.  Regarding vertical
           organization, however, two distinct configurations are supported.
           [//]
           If `row_gap' is 0, successive rows of the region written into
           `new_region' are concatenated within each channe buffer, so that
           the row gap is effectively equal to `new_region.size.x' -- it is
           determined by the particular dimensions of the region processed
           by the function.  In this case, the `buffer_origin' argument is
           ignored.
           [//]
           If `row_gap' is non-zero, each channel buffer points to the location
           identified by `buffer_origin' (on the rendering canvas), and each
           successive row of the buffer is separated by the amount determined
           by `row_gap'.  In this case, it is the application's responsibility
           to ensure that the buffers will not be overwritten if any samples
           from the `incomplete_region' are written onto the buffer, taking
           the `buffer_origin' into account.  In particular, the
           `buffer_origin' must not lie beyond the first row or first column
           of the `incomplete_region'.  Note that the interpretation of
           `row_gap' depends upon the `measure_row_gap_in_pixels' argument.
           [//]
           On entry, the `incomplete_region' structure identifies the subset
           of the original region supplied to `start', which has not yet been
           decompressed and is still relevant to the application.  The function
           uses this information to avoid unnecessary processing of tiles
           which are no longer relevant, unless these tiles are already opened
           and being processed.
           [//]
           On exit, the upper boundary of the `incomplete_region' is updated
           to reflect any completely decompressed rows.  Once the region
           becomes empty, all processing is complete and future calls will
           return false.
           [//]
           The function may return true, with `new_region' empty.  This can
           happen, for example, when skipping over unnecessary tile or
           group of tiles.  The intent is to avoid the possibility that the
           caller might be forced to wait for an unbounded number of tiles to
           be loaded (possibly from disk) while hunting for one which has a
           non-empty intersection with the `incomplete_region'.  In general,
           the current implementation limits the number of new tiles which
           will be opened to one row of horizontally adjacent tiles.  In this
           way, a number of calls may be required before the function will
           return with `new_region' non-empty.
           [//]
           If the code-stream is found to offer insufficient DWT levels to
           support the `discard_levels' argument supplied to `start', the
           present function will return false, after which you should invoke
           `finish' and potentially start again with a different number of
           discard levels.
           [//]
           If the underlying code-stream is found to be sufficiently corrupt
           that the decompression process must be aborted, the current function
           will catch any exceptions thrown from an application supplied
           `kdu_error' handler, returning false prematurely.  This condition
           will be evident from the fact that `incomplete_region' is non-empty.
           You should still call `finish' and watch the return value from that
           function.
         [RETURNS]
           False if the `incomplete_region' becomes empty as a result of this
           call, or if a required tile is found to offer insufficient DWT
           levels to support the `discard_levels' values originally supplied to
           `start', or if an internal error occurs during code-stream data
           processing and an exception was thrown by the application-supplied
           `kdu_error' handler.  In any case, the correct response to a
           false return value is to call `finish' and check its return value.
           [//]
           If `finish' returns false, an error has occurred and
           you must close the `kdu_codestream' object (possibly re-opening it
           for subsequent processing, perhaps in a more resilient mode -- see
           `kdu_codestream::set_resilient').
           [//]
           If `finish' returns true, the incomplete region might not have
           been completed if the present function found that you were
           attempting to discard too many resolution levels or to flip an
           image which cannot be flipped, due to the use of certain packet
           wavelet decomposition structures.  To check for the former
           possibility, you should always check the value returned by
           `kdu_codestream::get_min_dwt_levels' after `finish' returns true.
           To check for the second possibility, you should also
           test the value returned by `kdu_codestream::can_flip', possibly
           adjusting the appearance of the codestream (via
           `kdu_codestream::change_appearance') before further processing.
           Note that the values returned by `kdu_codestream::get_min_dwt_levels'
           and `kdu_codestream::can_flip' can become progressively more
           restrictive over time, as more tiles are visited.
           [//]
           Note carefully that this function may return true, even though it has
           decompressed no new data of interest to the application (`new_region'
           empty).  This is because a limited number of new tiles are opened
           during each call, and these tiles might not have any intersection
           with the current `incomplete_region' -- the application might have
           reduced the incomplete region to reflect changing interests.
         [ARG: channel_bufs]
           Array with `kdu_channel_mapping::num_channels' entries, or
           `kdu_channel_mapping::num_channels'+2 entries.  The latter applies
           only if `expand_monochrome' is true and
           `kdu_channel_mapping::num_colour_channels' is equal to 1.  If
           no `kdu_channel_mapping' object was passed to `start', the number
           of channel buffers is equal to 1 (if `expand_monochrome' is false)
           or 3 (if `expand_monochrome' is true).  The entries in the
           `channel_bufs' array are not modified by this function.
           [//]
           If any entry in the array is NULL, that channel will effectively
           be skipped.  This can be useful, for example, if the value of
           `kdu_channel_mapping::num_colour_channels' is larger than the
           number of channels produced by a colour transform supplied by
           `kdu_channel_mapping::colour_transform' -- for example, a CMYK
           colour space (`kdu_channel_mapping::num_colour_channels'=4)
           might be converted to an sRGB space, so that the 4'th colour
           channel, after conversion, becomes meaningless.
         [ARG: expand_monochrome]
           If true and the number of colour channels identified by
           `kdu_channel_mapping::num_colour_channels' is 1, or if no
           `kdu_channel_mapping' object was passed to `start', the function
           automatically copies the colour channel data into the first
           three buffers supplied via the `channel_bufs' array, and the second
           real channel, if any, corresponds to the fourth entry in the
           `channel_bufs' array.
         [ARG: pixel_gap]
           Separation between consecutive pixels, in each of the channel
           buffers supplied via the `channel_bufs' argument.
         [ARG: buffer_origin]
           Location, in rendering canvas coordinates, of the upper left hand
           corner pixel in each channel buffer supplied via the `channel_bufs'
           argument, unless `row_gap' is 0, in which case, this argument is
           ignored.
         [ARG: row_gap]
           If zero, rendered image lines will simply be concatenated into the
           channel buffers supplied via `channel_bufs', so that the line
           spacing is given by the value written into `new_region.size.x'
           upon return.  In this case, `buffer_origin' is ignored.  Otherwise,
           this argument identifies the separation between vertically adjacent
           pixels within each of the channel buffers.  If
           `measure_row_gap_in_pixels' is true, the number of samples
           between consecutive buffer lines is `row_gap'*`pixel_gap'.
           Otherwise, it is just `row_gap'.
         [ARG: suggested_increment]
           Suggested number of samples to decompress from the code-stream
           component associated with the first channel (or the single
           image component) before returning.  Of course, there will often
           be other image components which must be decompressed as well, in
           order to reconstruct colour imagery; the number of these samples
           which will be decompressed is adjusted in a commensurate fashion.
           Note that the decompressed samples may be subject to interpolation
           (if the `expand_numerator' and `expand_denominator' arguments
           supplied to the `start' member function represent expansion
           factors which are larger than 1); the `suggested_increment' value
           refers to the number of decompressed samples prior to any such
           interpolation.  Note also that the function is free to make up its
           own mind exactly how many samples it will process in the current
           call, which may vary from 0 to the entire `incomplete_region'.
           [//]
           For interactive applications, typical values for the
           `suggested_increment' argument might range from tens of thousands,
           to hundreds of thousands.
           [//]
           If `row_gap' is 0, and the present argument is 0, the only
           constraint will be that imposed by `max_region_pixels'.
         [ARG: max_region_pixels]
           Maximum number of pixels which can be written to any channel buffer
           provided via the `channel_bufs' argument.  This argument is
           ignored unless `row_gap' is 0, since only in that case is the
           number of pixels which can be written governed solely by the size
           of the buffers.  An error will be generated (through `kdu_error') if
           the supplied limit is too small to accommodate even a single line
           from the new region.  For this reason, you should be careful to
           ensure that `max_region_pixels' is at least as large as
           `incomplete_region.size.x'.
         [ARG: incomplete_region]
           Region on the rendering canvas which is still required by the
           application.  This region should be a subset of the region of
           interest originally supplied to `start'.  However, it may be much
           smaller.  The function works internally with a bank of horizontally
           adjacent tiles, which may range from a single tile to an entire
           row of tiles (or part of a row of tiles).  From within the current
           tile bank, the function decompresses lines as required
           to fill out the incomplete region, discarding any rows from already
           open tiles which do not intersect with the incomplete region.  On
           exit, the first row in the incomplete region will be moved down
           to reflect any completely decompressed image rows.  Note, however,
           that the function may decompress image data and return with
           `new_region' non-empty, without actually adjusting the incomplete
           region.  This happens when the function decompresses tile data
           which intersects with the incomplete region, but one or more tiles
           remain to the right of that region.  Generally speaking, the function
           advances the top row of the incomplete region only when it
           decompresses data from right-most tiles which intersect with that
           region, or when it detects that the identity of the right-most
           tile has changed (due to the width of the incomplete region
           shrinking) and that the new right-most tile has already been
           decompressed.
         [ARG: new_region]
           Used to return the location and dimensions of the region of the
           image which was actually decompressed and written into the channel
           buffers supplied via the `channel_bufs' argument.  The region's
           size and location are all expressed relative to the same rendering
           canvas as the `incomplete_region' and `buffer_origin' arguments.
           Note that the region might be empty, even though processing is not
           yet complete.
         [ARG: precision_bits]
           Indicates the precision of the unsigned integers represented by
           each sample in each buffer supplied via the `channel_bufs' argument.
           This value need not bear any special relationship to the original
           bit-depth of the compressed image samples.  The rendered sample
           values are written into the buffer as B-bit unsigned integers,
           where B is the value of `precision_bits' and the most significant
           bits of the B-bit integer correspond to the most significant bits
           of the original image samples.  Normally, the value of this argument
           will be 8 so that the rendered data is always normalized for display
           on an 8-bit/sample device.  There may be more interest in selecting
           different precisions when using the second form of the overloaded
           `process' function.
           [//]
           From Kakadu v4.3, it is possible to supply a special value of 0
           for this argument.  In this case, a "default" set of precisions
           will be used (one for each channel).  If a `kdu_channel_mapping'
           object was supplied to `start', that object's
           `kdu_channel_mapping::default_rendering_precision' and
           `kdu_channel_mapping::default_rendering_signed' arrays are used
           to derive the default channel precisions, as well as per-channel
           information about whether the samples should be rendered as
           unsigned quantities or as 2's complement 8-bit integers.  That
           information, in turn, is typically initialized by one of the
           `kdu_channel_mapping::configure' functions to represent the
           native bit-depths and signed/unsigned properties of the original
           image samples (or palette indices); however, it may be explicitly
           overridden by the application.  This gives you enormous flexibility
           in choosing the way you want rendered sample bits to be
           represented.  If no `kdu_channel_mapping' object was supplied to
           `start', the default rendering precision and signed/unsigned
           characteristics are derived from the original properties of the
           image samples represented by the code-stream.
           [//]
           It is worth noting that the rendering precision, B, can actually
           exceed 8, either because `precision_bits' > 8, or because
           `precision_bits'=0 and the default rendering precisions, derived
           in the above-mentioned manner, exceed 8.  In this case, the
           function automatically truncates the rendered values to fit
           within the 8-bit representation associated with the `channel_bufs'
           array(s).  If B <= 8, the rendered values are truncated to fit
           within the B bits.  Where 2's complement output samples are
           written, they are truncated in the natural way and sign extended.
         [ARG: measure_row_gap_in_pixels]
           If true, `row_gap' is interpreted as the number of whole pixels
           between consecutive rows in the buffer.  Otherwise, it is interpreted
           as the number of samples only.  The latter form can be useful when
           working with image buffers having alignment constraints which are
           not based on whole pixels (e.g., Windows bitmap buffers).
      */
    KDU_AUX_EXPORT bool
      process(kdu_uint16 **channel_bufs, bool expand_monochrome,
              int pixel_gap, kdu_coords buffer_origin, int row_gap,
              int suggested_increment, int max_region_pixels,
              kdu_dims &incomplete_region, kdu_dims &new_region,
              int precision_bits=16, bool measure_row_gap_in_pixels=true);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `process' function, except
           that the channel buffers each hold 16-bit unsigned quantities.  As
           before, the `precision_bits' argument is used to control the
           representation written into each output sample.  If it is 0, default
           precisions are obtained, either from the `kdu_channel_mapping'
           object or from the codestream, as appropriate, and these defaults
           might also cause the 16-bit output samples to hold 2's complement
           signed quantities.  Also, as before, the precision requested
           explicitly or implicitly (via `precision_bits'=0) may exceed 16.
           In each case, the most natural truncation procedures are
           employed for out-of-range values, following the general
           strategy outlined in the comments accompanying the
           `precision_bits' argument in the first form of the `process'
           function.
      */
    KDU_AUX_EXPORT bool
      process(float **channel_buffer, bool expand_monochrome,
              int pixel_gap, kdu_coords buffer_origin, int row_gap,
              int suggested_increment, int max_region_pixels,
              kdu_dims &incomplete_region, kdu_dims &new_region,
              bool normalize=true, bool measure_row_gap_in_pixels=true);
      /* [SYNOPSIS]
           Same as the first and second forms of the overloaded `process'
           function, except that the channel buffers employ a floating point
           representation.  As with those functions, it is possible to select
           arbitrary `channel_offsets' so that the organization of `buffer'
           need not necessarily be interleaved component-by-component.
           [//]
           To get data with sufficient accuracy to deserve a floating point
           representation, you might like to set the `precise' argument to
           true in the call to `start'.
           [//]
           One important difference between this function and the
           integer-based `process' functions is that some form of white
           stretching is always employed.  Specifically, the dynamic range
           of the original sample values used to create each channel is
           stretched to full dynamic range associated with the floating point
           result.  If `normalize' is true, this output dynamic range is from
           0 to 1.0, inclusive; thus, if original samples had an 8-bit dynamic
           range (as originally compressed), they would be divided by 255.0
           when writing to the floating point `buffer' -- not divided by 256.
           If `normalize' is false, the output dynamic range depends upon the
           default rendering precision information found in the
           `kdu_channel_mapping' object supplied to `start'.  For more on this,
           see the `normalize' argument.  Considering that white stretching
           is performed automatically, you are advised to use 0 for the
           white stretch precision supplied to `set_white_stretch' -- this is
           the default value, so you don't need to call `set_white_stretch' at
           all.  If you do happen to have supplied a non-trivial value to
           `set_white_stretch' everything will still work, but some precision
           may be lost (white stretching imposes constraints on the internal
           representation) and there will be unnecessary computation.
         [ARG: normalize]
           If true, the `buffer' samples are all normalized to lie within
           the closed interval [0,1].  Specifically, the signed,
           decompressed (and possibly resampled) values associated with
           each channel are notionally clipped to the range -2^{B-1} to
           +2^{B-1}-1 and then offset by 2^{B-1} and divided by (2^B - 1),
           where B is the original bit-depth indicated in the codestream, for
           the image component used to generate the channel in question.
           If a colour palette is involved, the value of B in the above
           formulation is the nominal bit-depth associated with the palette.
           Similarly, if white stretching has been requested, the value of B
           in the above formulation is the larger of the actual component
           bit-depth and the white stretch precision supplied to
           `set_white_stretch'.  As noted above, however, you are strongly
           advised not to use `set_white_stretch' if you are intending to
           retrieve floating point data from the `process' function.
           [//]
           If the `normalize' argument is false, the behaviour is similar to
           that described above, except that the B-bit input data is stretched
           into the range [Pmin, Pmax], where Pmin and Pmax are determined
           from the `kdu_channel_mapping::default_rendering_precision' and
           `kdu_channel_mapping::default_rendering_signed' arrays found in
           the `kdu_channel_mapping' object supplied to `start'.  Specifically,
           if P is the default rendering precision for the channel, as
           determined from the `kdu_channel_mapping' object, and the default
           rendering information identifies signed sample values,
           Pmin=-2^{P-1} and Pmax=2^{P-1}-1.  Similarly, if the default
           rendering for the channel is unsigned, Pmin=0 and Pmax=(2^P)-1.
           If `kdu_channel_mapping::default_rendering_precision' specifies a
           precision P=0, we have [Pmin,Pmax]=[-0.5,0.5] and [0,1] for the
           signed and unsigned cases respectively.  Thus, `normalize'=true is
           identical to the case in which `normalize'=false and the default
           rendering information in `kdu_channel_mapping' identifies an
           unsigned representation with precision 0 -- this can still be
           useful because `kdu_channel_mapping' can supply different default
           rendering specifications for each channel.
           [//]
           Note that the default rendering information in `kdu_channel_mapping'
           is initialized by the `kdu_channel_mapping::configure' function
           to represent the native bit-depths and signed/unsigned
           properties of the original image samples (or palette indices);
           however, this information can always be set explicitly by the
           application.  If no `kdu_channel_mapping' object was supplied to
           `start', the default rendering precision and signed/unsigned
           characteristics are derived from the original properties of the
           image samples represented by the code-stream.
      */
    KDU_AUX_EXPORT bool
      process(kdu_int32 *buffer, kdu_coords buffer_origin,
              int row_gap, int suggested_increment, int max_region_pixels,
              kdu_dims &incomplete_region, kdu_dims &new_region);
      /* [SYNOPSIS]
           This function is equivalent to invoking the first form of the
           overloaded `process' function, with four 8-bit channel buffers
           and `precision_bits'=8.
           [>>] The 1st channel buffer (nominally RED) corresponds to the 2nd
                most significant byte of each integer in the `buffer'.
           [>>] The 2nd channel buffer (nominally GREEN) corresponds to the 2nd
                least significant byte of each integer in the `buffer'.
           [>>] The 3rd channel buffer (nominally BLUE) corresponds to the least
                significant byte of each integer in the `buffer'.
           [>>] The 4th channel buffer (nominally ALPHA) corresponds to the most
                significant byte of each integer in the `buffer'.
           [//]
           If the source has only one colour channel, the second and third
           channel buffers are copied from the first channel buffer (same as
           the `expand_monochrome' feature found in other forms of the `process'
           function).
           [//]
           If there are more than 3 colour channels, only the first 3 will
           be transferred to the supplied `buffer'.
           [//]
           If there is no alpha information, the fourth channel buffer (most
           significant byte of each integer) is set to 0xFF.
           [//]
           Apart from convenience, one reason for providing this function
           in addition to the other, more general forms of the `process'
           function, is that it is readily mapped to alternate language
           bindings, such as Java.
           [//]
           Another important reason for providing this form of the `process'
           function is that missing alpha data is always synthesized (with
           0xFF), so that a full 32-bit word is aways written for each pixel.
           This can significantly improve memory access efficiency on some
           common platforms.  It also allows for efficient internal
           implementations in which the rendered channel data is written
           in aligned 16-byte chunks wherever possible, without any need
           to mask off unused intermediate values.  To fully exploit this
           capability, you are strongly recommended to suppy a 16-byte
           aligned `buffer'.
      */
    KDU_AUX_EXPORT bool
      process(kdu_byte *buffer, int *channel_offsets,
              int pixel_gap, kdu_coords buffer_origin, int row_gap,
              int suggested_increment, int max_region_pixels,
              kdu_dims &incomplete_region, kdu_dims &new_region,
              int precision_bits=8, bool measure_row_gap_in_pixels=true,
              int expand_monochrome=0, int fill_alpha=0);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `process' function, except
           that the channel buffers are interleaved into a single buffer.  It
           is actually possible to select arbitrary `channel_offsets'
           so that the organization of `buffer' need not necessarily be
           interleaved component-by-component.
           [//]
           One reason for providing this function in addition to the second
           form of the `process' function, is that this version is readily
           mapped to alternate language bindings, such as Java.
           [//]
           The other reason for providing this function is to provide a
           convenient method to fill in additional entries in an interleaved
           buffer with reasonable values.  This is achieved
           with the aid of the `expand_monochrome' and `fill_alpha'
           arguments.
         [ARG: channel_offsets]
           Array with at least `kdu_channel_mapping::num_channels' entries.
           Each entry specifies the offset in samples to the first sample of
           the associated channel data.  The number of entries in this array
           may need to be larger than the number of actual available channels,
           depending upon the `expand_monochrome' and `fill_alpha' arguments,
           as described below.
         [ARG: expand_monochrome]
           If the number of colour channels is 1, as determined from the
           value of `kdu_channel_mapping::num_colour_channels' in the
           `mapping' object passed to `start', or by the fact that no
           `mapping' object at all was passed to `start', this argument
           may be used to expand the single colour channel into a total of
           `expand_monochrome' copies.  Specifically, if `expand_monochrome'
           exceeds 1, an additional `expand_monochrome'-1 copies of the
           single available colour channel are created -- these are expected
           to correspond to entries 1 through `expand_monochrome'-1 in the
           `channel_offsets' array.  Any additional (non-colour) channels
           which are being decompressed are then considered to correspond to
           entries starting from index `expand_monochrome' in the
           `channel_offsets' array.
           [//]
           If, however, there are multiple colour channels in the original
           description supplied to `start', this argument is ignored.  For
           this case, it is not at all obvious how extra colour components
           ought to be synthesized -- the application should perhaps arrange
           for the `kdu_channel_mapping' object passed to `start' to
           describe the additional colour channels, along with a suitable
           colour transform.
         [ARG: fill_alpha]
           This argument may be used to synthesize "opaque" alpha channels,
           where no corresponding alpha description is available.  Specifically,
           if `fill_alpha' exceeds the number of non-colour channels
           specified in the description supplied to `start' (this is
           0 if no `mapping' object is supplied to `start'; otherwise, it is
           the difference between `kdu_channel_mapping::num_channels' and
           `kdu_channel_mapping::num_colour_channels'), additional channels
           are synthesized as required and filled with the maximum
           value associated with the numeric representation (depends on
           `precision_bits').  In this case, the non-colour channels written
           by this funtion correspond to entries C through C+`fill_alpha'-1 in
           the `channel_offsets' array, where C is the number of colour
           channels being written (taking into account the effect of
           `expand_monochrome').
           [//]
           Note that there is no default information in the
           `kdu_channel_mapping' object supplied to `start' from which to
           determine a fill value, in the case that `precision_bits' is 0.
           For this reason, the fill value if taken to be 0xFF if
           `precision_bits' is not in the range 1 to 7.
       */
    KDU_AUX_EXPORT bool
      process(kdu_uint16 *buffer, int *channel_offsets,
              int pixel_gap, kdu_coords buffer_origin, int row_gap,
              int suggested_increment, int max_region_pixels,
              kdu_dims &incomplete_region, kdu_dims &new_region,
              int precision_bits=16, bool measure_row_gap_in_pixels=true,
              int expand_monochrome=0, int fill_alpha=0);
      /* [SYNOPSIS]
           Same as the second form of the overloaded `process' function, except
           that the channel buffers are interleaved into a single buffer.  It
           is actually possible to select arbitrary `channel_offsets'
           so that the organization of `buffer' need not necessarily be
           interleaved component-by-component.
           [//]
           One reason for providing this function in addition to the second
           form of the `process' function, is that this version is readily
           mapped to alternate language bindings, such as Java.
           [//]
           The other reason for providing this function is to provide a
           convenient method to fill in additional entries in an interleaved
           buffer with reasonable values.  This is achieved
           with the aid of the `expand_monochrome' and `fill_alpha'
           arguments.
         [ARG: channel_offsets]
           Array with at least `kdu_channel_mapping::num_channels' entries.
           Each entry specifies the offset in samples to the first sample of
           the associated channel data.  The number of entries in this array
           may need to be larger than the number of actual available channels,
           depending upon the `expand_monochrome' and `fill_alpha' arguments,
           as described below.
         [ARG: expand_monochrome]
           If the number of colour channels is 1, as determined from the
           value of `kdu_channel_mapping::num_colour_channels' in the
           `mapping' object passed to `start', or by the fact that no
           `mapping' object at all was passed to `start', this argument
           may be used to expand the single colour channel into a total of
           `expand_monochrome' copies.  Specifically, if `expand_monochrome'
           exceeds 1, an additional `expand_monochrome'-1 copies of the
           single available colour channel are created -- these are expected
           to correspond to entries 1 through `expand_monochrome'-1 in the
           `channel_offsets' array.  Any additional (non-colour) channels
           which are being decompressed are then considered to correspond to
           entries starting from index `expand_monochrome' in the
           `channel_offsets' array.
           [//]
           If, however, there are multiple colour channels in the original
           description supplied to `start', this argument is ignored.  For
           this case, it is not at all obvious how extra colour components
           ought to be synthesized -- the application should perhaps arrange
           for the `kdu_channel_mapping' object passed to `start' to
           describe the additional colour channels, along with a suitable
           colour transform.
         [ARG: fill_alpha]
           This argument may be used to synthesize "opaque" alpha channels,
           where no corresponding alpha description is available.  Specifically,
           if `fill_alpha' exceeds the number of non-colour channels
           specified in the description supplied to `start' (this is
           0 if no `mapping' object is supplied to `start'; otherwise, it is
           the difference between `kdu_channel_mapping::num_channels' and
           `kdu_channel_mapping::num_colour_channels'), additional channels
           are synthesized as required and filled with the maximum
           value associated with the numeric representation (depends on
           `precision_bits').  In this case, the non-colour channels written
           by this funtion correspond to entries C through C+`fill_alpha'-1 in
           the `channel_offsets' array, where C is the number of colour
           channels being written (taking into account the effect of
           `expand_monochrome').
           [//]
           Note that there is no default information in the
           `kdu_channel_mapping' object supplied to `start' from which to
           determine a fill value, in the case that `precision_bits' is 0.
           For this reason, the fill value if taken to be 0xFFFF if
           `precision_bits' is not in the range 1 to 15.
       */
    KDU_AUX_EXPORT bool
      process(float *buffer, int *channel_offsets,
              int pixel_gap, kdu_coords buffer_origin, int row_gap,
              int suggested_increment, int max_region_pixels,
              kdu_dims &incomplete_region, kdu_dims &new_region,
              bool normalize=true, bool measure_row_gap_in_pixels=true,
              int expand_monochrome=0, int fill_alpha=0);
      /* [SYNOPSIS]
           Same as the third form of the overloaded `process' function, except
           that the channel buffers are interleaved into a single buffer.  It
           is actually possible to select arbitrary `channel_offsets'
           so that the organization of `buffer' need not necessarily be
           interleaved component-by-component.
           [//]
           One reason for providing this function in addition to the third
           form of the `process' function, is that this version is readily
           mapped to alternate language bindings, such as Java.
           [//]
           The other reason for providing this function is to provide a
           convenient method to fill in additional entries in an interleaved
           buffer with reasonable values.  This is achieved
           with the aid of the `expand_monochrome' and `fill_alpha'
           arguments.
         [ARG: channel_offsets]
           Array with at least `kdu_channel_mapping::num_channels' entries.
           Each entry specifies the offset in samples to the first sample of
           the associated channel data.  The number of entries in this array
           may need to be larger than the number of actual available channels,
           depending upon the `expand_monochrome' and `fill_alpha' arguments,
           as described below.
         [ARG: expand_monochrome]
           If the number of colour channels is 1, as determined from the
           value of `kdu_channel_mapping::num_colour_channels' in the
           `mapping' object passed to `start', or by the fact that no
           `mapping' object at all was passed to `start', this argument
           may be used to expand the single colour channel into a total of
           `expand_monochrome' copies.  Specifically, if `expand_monochrome'
           exceeds 1, an additional `expand_monochrome'-1 copies of the
           single available colour channel are created -- these are expected
           to correspond to entries 1 through `expand_monochrome'-1 in the
           `channel_offsets' array.  Any additional (non-colour) channels
           which are being decompressed are then considered to correspond to
           entries starting from index `expand_monochrome' in the
           `channel_offsets' array.
           [//]
           If, however, there are multiple colour channels in the original
           description supplied to `start', this argument is ignored.  For
           this case, it is not at all obvious how extra colour components
           ought to be synthesized -- the application should perhaps arrange
           for the `kdu_channel_mapping' object passed to `start' to
           describe the additional colour channels, along with a suitable
           colour transform.
         [ARG: fill_alpha]
           This argument may be used to synthesize "opaque" alpha channels,
           where no corresponding alpha description is available.  Specifically,
           if `fill_alpha' exceeds the number of non-colour channels
           specified in the description supplied to `start' (this is
           0 if no `mapping' object is supplied to `start'; otherwise, it is
           the difference between `kdu_channel_mapping::num_channels' and
           `kdu_channel_mapping::num_colour_channels'), additional channels
           are synthesized as required and filled with the value 1.0.  In this
           case, the non-colour channels written by this funtion correspond to
           entries C through C+`fill_alpha'-1 in the `channel_offsets' array,
           where C is the number of colour channels being written (taking into
           account the effect of `expand_monochrome').
           [//]
           Note that there is no default information in the
           `kdu_channel_mapping' object supplied to `start' from which to
           determine a fill value, in the case that `normalize' is false.
           For this reason, the fill value if always taken to be 1.0F.
      */
    KDU_AUX_EXPORT bool finish(kdu_exception *failure_exception=NULL);
      /* [SYNOPSIS]
           Every call to `start' must be matched by a call to `finish';
           however, you may call `finish' prematurely.  This allows processing
           to be terminated on a region whose intersection with a display
           window has become too small to justify the effort.
         [RETURNS]
           If the function returns false, a fatal error has occurred in the
           underlying codestream management machinery and you must destroy
           the codestream object (use `kdu_codestream::destroy').  You will
           probably also have to close the relevant compressed data source
           (e.g., using `kdu_compressed_source::close').  This should clean
           up all the resources correctly, in preparation for subsequently
           opening a new code-stream for further decompression and rendering
           work.
           [//]
           Otherwise, if the `process' function returned false leaving a
           non-empty incompletely processed region, one of two non-fatal
           errors has occurred:
           [>>] The number of `discard_levels' supplied to `start' has been
                found to exceed the number of DWT levels offered by some tile.
           [>>] The codestream was configured to flip the image geometry, but
                a Part-2 packet wavelet decomposition structure has been
                employed in some tile which is fundamentally non-flippable
                (only some of the Part-2 packet decomposition styles have
                this property).
           [//]
           The correct response to either of these events is documented in
           the description of the return value from `process'.  Note that
           the `start' function may always be re-invoked with a smaller number
           of `discard_levels' and a larger value of `expand_denominator', to
           synthesize the required resolution.
         [ARG: failure_exception]
           If non-NULL and the function returns false, this argument is used
           to return a copy of the Kakadu exception which was caught (probably
           during an earlier call to `process').  In addition to exceptions of
           type `kdu_exception', exceptions of type `std::bad_alloc' are also
           caught and converted into the special value, `KDU_MEMORY_EXCEPTION',
           so that they can be passed across this interface.  If your code
           rethrows the exception, it may be best to test for this special
           value and rethrow such exceptions as `std::bad_alloc()'.
      */
  protected: // Implementation helpers which might be useful to extended classes
    bool process_generic(int sample_bytes, int pixel_gap,
                         kdu_coords buffer_origin, int row_gap,
                         int suggested_increment, int max_region_pixels,
                         kdu_dims &incomplete_region, kdu_dims &new_region,
                         int precision_bits, int fill_alpha=0);
      /* Implements the 8-bit, 16-bit and floating-point `process' functions,
         where the buffer pointers are stored in the internal `channel_bufs'
         array.  `sample_bytes' must be equal to 1 (for 8-bit samples),
         2 (for 16-bit samples) or 4 (for floating-point samples).  Note that
         `row_gap' here is measured in samples, not pixels.  If `fill_alpha'
         is non-zero, the final `fill_alpha' channel buffers are to be filled
         with an opaque alpha value (maximum value associated with the
         relevant representation), rather than deriving it from rendered
         imagery. */
  private: // Implementation helpers
    void set_num_channels(int num);
      /* Convenience function to allocate and initialize the `channels' array
         as required to manage a total of `num' channels.  Sets `num_channels'
         and `num_colour_channels' both equal to `num' before returning.
         Also, initializes the `kdrd_channel::native_precision' value to 0
         and `kdrd_channel::native_signed' to false for each channel. */
    kdrd_component *add_component(int comp_idx);
      /* Adds a new component to the `components' array, if necessary, returning
         a pointer to that component. */
    bool start_tile_bank(kdrd_tile_bank *bank, kdu_long suggested_tile_mem,
                         kdu_dims incomplete_region);
      /* This function uses `suggested_tile_mem' to determine the number
         of new tiles which should be opened as part of the new tile bank.  The
         value of `suggested_tile_mem' represents the suggested total memory
         associated with open tiles, where memory is estimated (very crudely)
         as the tile width multiplied by the minimum of 100 and the tile
         height.  The function opens horizontally adjacent tiles until the
         suggested tile memory is exhausted, except that it exercises some
         intelligence in avoiding a situation in which only a few small tiles
         are left on a row.
         [//]
         On entry, the supplied `bank' must have its `num_tiles' member set
         to 0, meaning that it is closed.  Tiles are opened starting from
         the tile identified by the `next_tile_idx' member, which is
         adjusted as appropriate.
         [//]
         Note that this function does not make any adjustments to the
         `render_dims' member or any of the members in the `components' or
         `channels' arrays.  To prepare the state of those objects, the
         `make_tile_bank_current' function must be invoked on a tile-bank
         which has already been started.
         [//]
         If the function is unable to open new tiles due to restrictions on
         the number of DWT levels or decomposition-structure-imposed
         restrictions on the allowable flipping modes, the function returns
         false.
         [//]
         The last argument is used to identify tiles which have no overlap
         with the current `incomplete_region'.  The supplied region is
         identical to that passed into the `process' functions on entry.  If
         some tiles that would normally be opened are found to lie outside
         this region, they are closed immediately and not included in the
         count recorded in `kdrd_tile_bank::num_tiles'.  As a result, the
         function may return with `bank->num_tiles'=0, which is not a failure
         condition, but simply means that there is nothing to decompress right
         now, at least until the application calls `process' again. */
    void close_tile_bank(kdrd_tile_bank *bank);
      /* Use this function to close all tiles and tile-processing engines
         associated with the tile-bank.  This is normally the current tile
         bank, unless processing is being terminated prematurely. */
    void make_tile_bank_current(kdrd_tile_bank *bank,
                                kdu_dims incomplete_region);
      /* Call this function any time after starting a tile-bank, to make it
         the current tile-bank and appropriately configure the `render_dims'
         and various tile-specific members of the `components' and `channels'
         arrays. */
  private: // Data
    bool precise;
    bool fastest;
    int white_stretch_precision; // Persistent mode setting
    int zero_overshoot_interp_threshold; // See `set_interpolation_behaviour'
    float max_interp_overshoot; // See `set_interpolation_behaviour'
    kdu_thread_env *env; // NULL for single-threaded processing
    kdu_thread_queue *env_queue; // Passed to `start'
    kdu_long next_queue_bank_idx; // Used by `start_tile_bank'
    kdrd_tile_bank *tile_banks; // Array of 2 tile banks
    kdrd_tile_bank *current_bank; // Points to one of the `tile_banks'
    kdrd_tile_bank *background_bank; // Non-NULL if next bank has been started
    kdu_codestream codestream;
    bool codestream_failure; // True if an exception generated in `process'.
    kdu_exception codestream_failure_exception; // Save exception until `finish'
    int discard_levels; // Value supplied in last call to `start'
    kdu_dims valid_tiles;
    kdu_coords next_tile_idx; // Index of next tile, not yet opened in any bank
    kdu_sample_allocator aux_allocator; // For palette indices, interp bufs, etc
    kdu_coords original_expand_numerator; // Copied from call to `start'
    kdu_coords original_expand_denominator; // Copied from call to `start'
    kdu_dims full_render_dims; // Dimensions of whole image on rendering canvas
    kdu_dims render_dims; // Dimensions of current tile bank on rendering canvas
    int max_channels; // So we can tell if `channels' array needs reallocating
    int num_channels;
    int num_colour_channels;
    kdrd_channel *channels;
    jp2_colour_converter *colour_converter; // For colour conversion ops
    int max_components; // So we can tell if `components' needs re-allocating
    int num_components; // Num valid elements in each of the next two arrays
    kdrd_component *components;
    int *component_indices; // Used with `codestream.apply_input_restrictions'
    int max_channel_bufs; // Size of `channel_bufs' array
    int num_channel_bufs; // Set from within `process'
    kdu_byte **channel_bufs; // Holds buffers passed to `process'
  };
  /* Implementation related notes:
        The object is capable of working with a single tile at a time, or
     a larger bank of concurrent tiles.  The choice between these options,
     depends on the size of the tiles, compared to the surface being rendered
     and the suggested processing increment supplied in calls to `process'.
     The `kdrd_tile_bank' structure is used to maintain a single bank of
     simultaneously open (active) tiles.  These must all be horizontally
     adjacent.  It is possible to start one bank of tiles in the background
     while another is being processed, which can be useful when a
     multi-threading environment is available, to minimize the likelihood
     that processor resources become idle.  Most of the rendering operations
     performed outside of decompression itself work on the current tile bank
     as a whole, rather than individual tiles.  For this reason, the dynamic
     state information found in `render_dims' and the various members of the
     `components' and `channels' arrays reflect the dimensions and other
     properties of the current tile bank, rather than just one tile (of
     course a tile bank may consist of only 1 tile, but it may consist of
     an entire row of tiles, or any collection of horizontally adjacent tiles).
        The `render_dims' member identifies the dimensions and location of the
     region associated with the `current_tile_bank', expressed on the
     rendering canvas coordinate system (not the code-stream canvas coordinate
     system). Whenever a new line of channel data is produced, the
     `render_dims.pos.y' field is incremented and `render_dims.size.y' is
     decremented.
        If the 16-bit version of the `process' function is invoked,
     `channel_bufs' actually holds pointers to 16-bit buffers, cast to byte
     pointers.  This allows a single internal implementation of the various
     externally visible `process' functions.
  */

#endif // KDU_REGION_DECOMPRESSOR_H
