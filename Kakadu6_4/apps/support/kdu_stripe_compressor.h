/******************************************************************************/
// File: kdu_stripe_compressor.h [scope = APPS/SUPPORT]
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
   Defines the `kdu_stripe_compressor' object, a high level, versatile facility
for compressing images in memory by stripes.  The app fills stripe buffers, of
any desired size and passes them to the object, which takes care of all the
other details to optimally sequence the compression tasks.  This allows
the image to be processed in one hit, from a memory buffer, or to be
processed progressively from application-defined stripe buffers.  Provides
an easy way to use Kakadu without having to know much about the JPEG2000, but
advanced developers may still wish to use the lower level mechanisms to avoid
memory copying, or for customed sequencing of the processing machinery.
*******************************************************************************/

#ifndef KDU_STRIPE_COMPRESSOR_H
#define KDU_STRIPE_COMPRESSOR_H

#include "kdu_compressed.h"
#include "kdu_sample_processing.h"

// Objects declared here:
class kdu_stripe_compressor;

// Declared elsewhere:
struct kdsc_tile;
struct kdsc_component_state;
class kdsc_flush_worker;

/******************************************************************************/
/*                          kdu_stripe_compressor                             */
/******************************************************************************/

class kdu_stripe_compressor {
  /* [BIND: reference]
     [SYNOPSIS]
       This object provides a high level interface to the Kakadu compression
       machinery, which is capable of satisfying the needs of most developers
       while providing essentially a one-function-call solution for simple
       applications.  Most new developers will probably wish to base their
       compression applications upon this object.
       [//]
       It should be noted, however, that some performance benefits can be
       obtained by creating compression engines yourself and directly passing
       them `kdu_line_buf' lines, since this can often avoid unnecessary
       copying and level shifting of image samples.  Nevertheless, there
       has been a lot of demand for a dead-simple, yet also powerful interface
       to Kakadu, and this object is intended to fill that requirement.  In
       fact, the various objects found in the "support" directory
       (`kdu_stripe_compressor', `kdu_stripe_decompressor' and
       `kdu_region_decompressor') are aimed at meeting the needs of 90% of
       the applications using Kakadu.  That is not to say that these objects
       are all that is required.  You still need to open streams of one
       form or another and create a `kdu_codestream' interface.
       [//]
       In a typical compression application based on this object, you will
       need to do the following:
       [>>] Create a `kdu_codestream' object;
       [>>] Use the interface recovered using `kdu_codestream::access_siz' to
            install any custom compression parameters you have in mind, unless
            you are happy with all the defaults;
       [>>] Initialize the `kdu_stripe_compressor' object, by calling
            `kdu_stripe_compressor::start'.
       [>>] Push image stripes into `kdu_stripe_compressor::push_stripe' until
            the image is fully compressed (you can do it all in one go, from
            a memory buffer of your choice, if you like);
       [>>] Call `kdu_stripe_compressor::finish'.
       [>>] Call `kdu_codestream::destroy'.
       [//]
       For a tuturial example of how to use the present object in a typical
       application, consult the Kakadu demo application,
       "kdu_buffered_compress".
       [//]
       It is worth noting that this object is built directly on top of the
       services offered by `kdu_multi_analysis', so for a thorough
       understanding of how things work, you might like to consult the
       documentation for that object as well.
       [//]
       Most notably, the image components which are supplied to the
       `push_stripe' function are those which are known (during
       decompression) as multi-component output components (or just
       output components).  This means that the present object inverts
       any Part 2 multi-component transformation network, which may be
       involved.
       [//]
       From Kakadu version 5.1, this object offers multi-threaded processing
       capabilities for enhanced throughput.  These capabilities are based
       upon the options for multi-threaded processing offered by the
       underlying `kdu_multi_analysis' object and the `kdu_analysis' and
       `kdu_encoder' objects which it, in turn, uses.  Multi-threaded
       processing provides the greatest benefit on platforms with multiple
       physical CPU's, or where CPU's offer hyperthreading capabilities.
       Interestingly, although hyper-threading is often reported as offering
       relatively little gain, Kakadu's multi-threading model is typically
       able to squeeze 30-50% speedup out of processors which offer
       hyperthreading, in addition to the benefits which can be reaped from
       true multi-processor (or multi-core) architectures.
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
       to the core processing objects such as `kdu_multi_analysis', which
       allow calls from any thread.  In exchange, however, you get simplicity.
       In particular, you only need to pass the `kdu_thread_env' object into
       the `start' function, after which the object remembers the thread
       reference for you.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdu_stripe_compressor();
      /* [SYNOPSIS]
           All the real initialization is done within `start'.  You may
           use a single `kdu_stripe_compressor' object to compress multiple
           images, bracketing each use by calls to `start' and `finish'.
      */
    ~kdu_stripe_compressor() { finish(); }
      /* [SYNOPSIS]
           Calls `finish' to do all the internal state cleanup.
      */
    KDU_AUX_EXPORT void
      start(kdu_codestream codestream, int num_layer_specs=0,
            const kdu_long *layer_sizes=NULL,
            const kdu_uint16 *layer_slopes=NULL,
            kdu_uint16 min_slope_threshold=0,
            bool no_prediction=false, bool force_precise=false,
            bool record_layer_info_in_comment=true,
            double size_tolerance=0.0, int num_components=0,
            bool want_fastest=false, kdu_thread_env *env=NULL,
            kdu_thread_queue *env_queue=NULL, int env_dbuf_height=0);
      /* [SYNOPSIS]
           Call this function to initialize the object for compression.  Each
           call to `start' must be matched by a call to `finish', but you may
           re-use the object to compress subsequent images, if you like.
         [ARG: codestream]
           Interface to a `kdu_codestream' object whose `create' function has
           already been called.  The `kdu_params::finalize_all' function should
           not be called by the application; it will be invoked from within
           the present function, possibly after making some final adjustments
           to coding parameter attributes which have not been configured by
           the application.
         [ARG: num_layer_specs]
           If this argument is equal to 0, the number of quality layers to
           build into the code-stream is recovered from the `Clayers' coding
           parameter attribute, which the application may have been configured
           prior to calling this function.  If the `Clayers' attribute was not
           set, it will default to 1 when the `kdu_params::finalize_all'
           function is called from within this function.  If the present
           argument is greater than 0, and the `Clayers' attribute has not
           already been set, it will be set equal to the value of
           `num_layer_specs'.  Regardless of the final number of code-stream
           quality layers which is used, the `num_layer_specs' argument
           provides the number of entries in the `layer_sizes' and
           `layer_slopes' arrays, if non-NULL.  These arrays, if provided,
           allow the application to specify the properties of the quality
           layers.  If no layer sizes or slopes are specified, a logarithmically
           spaced set of quality layers will be constructed, following the
           conventions described with the `kdu_codestream::flush' function.
         [ARG: layer_sizes]
           If non-NULL, this argument points to an array with `num_layer_specs'
           entries, containing the cumulative number of bytes from the
           start of the code-stream to the end of each quality layer, if the
           code-stream were to be arranged in layer progressive order.  The
           code-stream may be arranged in a very different order, of course,
           but that has no impact on the sizes of the layers, as controlled
           by this argument.  If the actual number of quality layers,
           as specified by the `Clayers' attribute, is smaller than
           `num_layer_specs', not all of the entries in this array will be
           used.  If the actual number of quality layers is larger than
           `num_layer_specs', the additional quality layers will be empty.
           This argument is ignored if `num_layer_specs' is 0.
         [ARG: layer_slopes]
           If non-NULL, this argument points to an array with `num_layer_specs'
           entries, containing the distortion-length slope thresholds associated
           with each quality layer.  This argument is ignored if `layer_sizes'
           is non-NULL, or `num_layer_specs' is 0.  For an explanation of
           the logarithmic representation used for distortion length slope
           thresholds in Kakadu, see the comments associated with the
           `kdu_codestream::flush' function.
         [ARG: min_slope_threshold]
           If this argument is non-zero, the
           `kdu_codestream::set_min_slope_threshold' function will be used
           to apply this slope threshold prior to compression.  As explained
           in connection with that function, this can help to speed up the
           compression process significantly.  Although the application could
           invoke `kdu_codestream::set_min_slope_threshold' itself, providing
           a non-zero argument here will prevent the present function from
           calling `kdu_codestream::set_max_bytes' if the `layer_sizes' array
           is non-NULL.  More precisely, the function follows the following
           set of rules in determining what speedup features to apply:
           [>>] If `no_prediction' is true, no speedup features will be applied;
           [>>] else, if `min_slope_threshold' is non-zero, the value
                will be supplied to `kdu_codestream::set_min_slope_threshold';
           [>>] else, if `layer_sizes' is non-NULL, the last entry in the
                `layer_sizes' array will be passed to
                `kdu_codestream::set_max_bytes';
           [>>] else, if `layer_slopes' is non-NULL, the last entry in the
                `layer_slopes' array will be passed to
                `kdu_codestream::set_min_slope_threshold'.
         [ARG: no_prediction]
           If true, neither the `kdu_codestream::set_max_bytes' function, nor
           the `kdu_codestream::set_min_slope_threshold' function will be
           invoked.  Applications should set this argument to true only if
           they want to adopt a very conservative stance in relation to
           maximizing image quality at the expense of compression speed.  For
           typical images, Kakadu's code-block truncation prediction mechanisms
           have no impact on image quality at all, while saving processing
           time.
         [ARG: force_precise]
           If true, 32-bit internal representations are used by the
           compression engines created by this object, regardless of the
           precision of the image samples reported by
           `kdu_codestream::get_bit_depth'.
         [ARG: want_fastest]
           If this argument is true and `force_precise' is false, the function
           selects a 16-bit internal representation (usually leads to the
           fastest processing) even if this will result in reduced image
           quality, at least for irreversible processing.  For image
           components which require reversible compression, the 32-bit
           representation must be selected if the image sample precision
           is too high, or else numerical overflow might occur.
         [ARG: record_layer_info_in_comment]
           If true, the rate-distortion slope and the target number of bytes
           associated with each quality layer will be recorded in a COM
           (comment) marker segment in the main code-stream header.  This
           can be very useful for applications which wish to process the
           code-stream later in a manner which depends upon the interpretation
           of the quality layers.  For this reason, you should generally
           set this argument to true, unless you want to get the smallest
           possible file size when compressing small images.  For more
           information on this option, consult the comments appearing with
           its namesake in `kdu_codestream::flush'.
         [ARG: size_tolerance]
           This argument is ignored unless layering is controlled by
           cumulative layer sizes supplied via a `layer_sizes' array.  In
           this case, it may be used to trade accuracy for speed when
           determining the distortion-length slopes which achieve the target
           layer sizes as closely as possible.  In particular, the algorithm
           will finish once it has found a distortion-length slope which
           yields a size in the range target*(1-tolerance) <= size <= target,
           where target is the target size for the relevant layer.  If no
           such slope can be found, the layer is assigned a slope such that
           the size is as close as possible to the target, without exceeding it.
         [ARG: num_components]
           If zero, the number of image components to be supplied to the
           `push_stripe' function is identical to the value returned by
           `kdu_codestream::get_num_components', with its optional
           `want_output_comps' argument set to true.  However, you may
           supply a smaller number of image components during compression,
           if you think that these provide sufficient information to
           generate all codestream image components.  This can happen
           where a Part 2 multi-component transformation defines more
           MCT output components than there are codestream image components.
           Then, during compression, it may be possible to invert the
           defined multi-component transform network by supplying only a
           subset of the MCT output components as source components (the
           components supplied to `push_stripe').  If a non-zero
           `num_components' argument is supplied, this is the number of
           components you will push to `push_stripe' -- if there are not
           sufficient components, the machinery will generate an appropriate
           error message through `kdu_error'.  For more on this, consult the
           description of the `kdu_multi_analysis' object on which this
           is built.
         [ARG: env]
           This argument is used to establish multi-threaded processing.  For
           a discussion of the multi-threaded processing features offered
           by the present object, see the introductory comments to
           `kdu_stripe_compressor'.  We remind you here, however, that
           all calls to `start', `process' and `finish' must be executed
           from the same thread, which is identified only in this function.
           If you re-use the object to process a subsequent image, you may
           change threads between the two uses, passing the appropriate
           appropriate `kdu_thread_env' reference in each call to `start'.
           For the single-threaded processing model used prior to Kakadu
           version 5.1, set this argument to NULL.
         [ARG: env_queue]
           This argument is ignored unless `env' is non-NULL, in which case
           a non-NULL `env_queue' means that all multi-threaded processing
           queues created inside the present object, by calls to `process',
           should be created as sub-queues of the identified `env_queue'.
         [ARG: env_dbuf_height]
           This argument may be used to introduce and control parallelism
           in the DWT processing steps, allowing you to distribute the
           load associated with multiple tile-components across multiple
           threads. In the simplest case, this argument is 0, and parallel
           processing applies only to the block encoding processes.  For
           a small number of processors, this is usually sufficient to keep
           all CPU's active.  If this argument is non-zero, however, the
           `kdu_multi_analysis' objects on which all processing is based,
           are created with `double_buffering' equal to true and a
           `processing_stripe_height' equal to the value supplied for this
           argument.  See `kdu_multi_analysis::create' for a more
           comprehensive discussion of double buffering principles and
           guidelines.
      */
    KDU_AUX_EXPORT bool
      finish(int num_layer_specs=0,
             kdu_long *layer_sizes=NULL, kdu_uint16 *layer_slopes=NULL);
      /* [SYNOPSIS]
           Each call to `start' must be bracketed by a call to `finish', for
           the compression cycle to be completed.
           [//]
           If you did not push all required image data into the `push_stripe'
           function before calling `finish', the function returns false.  If
           this happens, the compressed image is not completely generated, but
           the internal machinery is cleaned up, so you cannot continue to
           push more stripes in.  This means that a new call to `start' can be
           issued regardless of whether `finish' returns true or false.
           You should watch the value returned by `push_stripe' if you have
           any doubts as to whether sufficient image data has been supplied.
         [RETURNS]
           True only if the compressed image is complete.  Otherwise,
           insufficent image data was pushed in via `push_stripe', but the
           internal machinery is cleaned up anyway.
         [ARG: num_layer_specs]
           Identifies the number of entries in the `layer_sizes' and/or
           `layer_slopes' arrays, if non-NULL.
         [ARG: layer_sizes]
           If non-NULL, the final cumulative sizes of each code-stream
           quality layer will be written into this array.  At most
           `num_layer_specs' cumulative layer sizes will be written.  If the
           actual number of quality layers whose sizes are known is less
           than `num_layer_specs', the additional entries are set to 0.
         [ARG: layer_slopes]
           Same as `layer_sizes' but used to receive final values of the
           distortion length slope thresholds associated with each layer.
           Again, if `num_layer_specs' exceeds the number of layers for
           which slope information is available, the additional entries
           will be set to 0.
      */
    KDU_AUX_EXPORT bool
      get_recommended_stripe_heights(int preferred_min_height,
                                     int absolute_max_height,
                                     int stripe_heights[],
                                     int *max_stripe_heights);
      /* [SYNOPSIS]
           Convenience function, provides recommended stripe heights for the
           most efficient use of the `push_stripe' function, subject to
           some guidelines provided by the application.
           [//]
           If the image is vertically tiled, the function recommends stripe
           heights which will advance each component to the next vertical tile
           boundary.  If any of these exceed `absolute_max_height', the
           function scales back the recommendation.  In either event, the
           function returns true, meaning that this is a well-informed
           recommendation and doing anything else may result in less
           efficient processing.
           [//]
           If the image is not tiled (no vertical tile boundaries), the function
           returns small stripe heights which will result in processing the
           image components in a manner which is roughly proportional to their
           dimensions.  In this case, the function returns false, since there
           are no serious efficiency implications to selecting quite
           different stripe heights.  The stripe height recommendations in
           this case are usually governed by the `preferred_min_height'
           argument.
           [//]
           In either case, the recommendations may change from time to time,
           depending on how much data has already been supplied for each
           image component by previous calls to `push_stripe'.  However,
           the function will never recommend the use of stripe heights larger
           than those returned via the `max_stripe_heights' array.  These
           maximum recommendations are determined the first time the function
           is called after `start'.  New values will only be computed if
           the object is re-used by calling `start' again, after `finish'.
         [RETURNS]
           True if the recommendation should be taken particularly seriously,
           meaning there will be efficiency implications to selecting different
           stripe heights.
         [ARG: preferred_min_height]
           Preferred minimum value for the recommended stripe height of the
           image component which has the largest stripe height.  This value is
           principally of interest where the image is not vertically tiled.
         [ARG: absolute_max_height]
           Maximum value which will be recommended for the stripe height of
           any image component.
         [ARG: stripe_heights]
           Array with one entry for each image component, which receives the
           recommended stripe height for that component.
         [ARG: max_stripe_heights]
           If non-NULL, this argument points to an array with one entry for
           each image component, which receives an upper bound on the stripe
           height which this function will ever recommend for that component.
           Note that the number of image components is the value returned by
           `kdu_codestream::get_num_components' with its optional
           `want_output_comps' argument set to true.
           There is no upper bound on the stripe height you can actually use
           in a call to `push_stripe', only an upper bound on the
           recommendations which this function will produce as it is called
           from time to time.  Thus, if you intend to use this function to
           guide your stripe selection, the `max_stripe_heights' information
           might prove useful in pre-allocating storage for stripe buffers
           provided by your application.
      */
    KDU_AUX_EXPORT bool
      push_stripe(kdu_byte *stripe_bufs[], int stripe_heights[],
                  int *sample_gaps=NULL, int *row_gaps=NULL,
                  int *precisions=NULL, int flush_period=0);
      /* [SYNOPSIS]
           Supplies vertical stripes of samples for each image component.  The
           number of entries in each of the arrays here is equal to the
           number of image components, as returned by
           `kdu_codestream::get_num_components' with its optional
           `want_output_comps' argument set to true.  Each stripe spans the
           entire width of its image component, which must be no larger than
           the ratio between the corresponding entries in the `row_gaps' and
           `sample_gaps' arrays.
           [//]
           Each successive call to this function advances the vertical position
           within each image component by the number of lines identified within
           the `stripe_heights' array.  To properly compress the image, you
           must eventually advance all components to the bottom.  At this
           point, the present function returns false (no more lines needed
           in any component) and a subsequent call to `finish' will return
           true.
           [//]
           Note that although components nominally advance from the top to
           the bottom, if `kdu_codestream::change_appearance' was used to
           flip the appearance of the vertical dimension, the supplied data
           actually advances the true underlying image components from the
           bottom up to the top.  This is exactly what one should expect from
           the description of `kdu_codestream::change_appearance' and requires
           no special processing in the implemenation of the present object.
           [//]
           Although considerable flexibility is offered with regard to stripe
           heights, there are a number of constraints.  As a general rule,
           you should attempt to advance the various image components in a
           proportional way, when processing incrementally (as opposed to
           supplying the entire image in a single call to this function).  What
           this means is that the stripe height for each component should,
           ideally, be inversely proportional to its vertical sub-sampling
           factor.  If you do not intend to do this for any reason, the
           following notes should be taken into account:
           [>>] If the image happens to be tiled, then you must follow
                the proportional processing guideline at least to the extent
                that no component should fall sufficiently far behind the rest
                that the object would need to maintain multiple open tile rows
                simultaneously.
           [>>] If a code-stream colour transform (ICT or RCT) is being used,
                you must follow the proportional processing guideline at least
                to the extent that the same stripe height must be used for the
                first three components (otherwise, internal application of the
                colour transform would not be possible).
           [>>] Similar to colour transforms, if a Part-2 multi-component
                transform is being used, you must follow the proportional
                processing guidelines at least to the extent that the same
                stripe height must be used for components which are combined
                by the multi-component transform.
           [>>] Regardless of the above constraints, the selection of
                proportional stripe heights improves the reliability of the
                block truncation prediction algorithm associated with calls to
                `kdu_codestream::set_max_bytes'.  To understand the conditions
                under which that function will be called, consult the comments
                appearing with the `min_slope_threshold' argument in the
                `start' function. If prediction is disabled or
                `kdu_codestream::set_min_slope_threshold' was called from
                within `start' (e.g., because you supplied a non-zero
                `min_slope_threshold' argument), there is no need to worry
                about pushing stripe heights which are proportional to the
                corresponding image component heights.
           [//]
           In addition to the constraints and guidelines mentioned above
           regarding the selection of suitable stripe heights, it is worth
           noting that the efficiency (computational and memory efficiency)
           with which image data is compressed depends upon how your
           stripe heights interact with image tiling.  If the image is
           untiled, you are generally best off passing small stripes, unless
           your application naturally provides larger stripe buffers.  If,
           however, the image is tiled, then the implementation is most
           efficient if your stripes happen to be aligned on vertical tile
           boundaries.  To simplify the determination of suitable stripe
           heights (all other things being equal), the present object
           provides a convenient utility, `get_recommended_stripe_heights',
           which you can call at any time.  Alternatively, just push in
           whatever stripes your application produces naturally.
           [//]
           To understand the interpretation of the sample bytes passed to
           this function, consult the comments appearing with the `precisions'
           argument below.  Other forms of the overloaded `push_stripe'
           function are provided to allow for compression of higher precision
           image samples.
         [RETURNS]
           True until all samples for all image components have been pushed in,
           at which point the function returns false.
         [ARG: stripe_bufs]
           Array with one entry for each image component, containing a pointer
           to a buffer which holds the stripe samples for that component.
           The pointers may all point into a single common buffer managed by
           the application, or they might point to separate buffers.  This,
           together with the information contained in the `sample_gaps' and
           `row_gaps' arrays allows the application to implement a wide
           variety of different stripe buffering strategies.  The entries
           (pointers) in the `stripe_bufs' array are not modified by this
           function.
         [ARG: stripe_heights]
           Array with one entry for each image component, identifying the
           number of lines being supplied for that component in the present
           call.  All entries must be non-negative.  See the extensive
           discussion above, on the various constraints and guidelines which
           may exist regarding stripe heights and their interaction with
           tiling and sub-sampling.
         [ARG: sample_gaps]
           Array containing one entry for each image component, identifying the
           separation between horizontally adjacent samples within the
           corresponding stripe buffer found in the `stripe_bufs' array.  If
           this argument is NULL, all component stripe buffers are assumed to
           have a sample gap of 1.
         [ARG: row_gaps]
           Array containing one entry for each image component, identifying
           the separation between vertically adjacent samples within the
           corresponding stripe buffer found in the `stripe_bufs' array.  If
           this argument is NULL, all component stripe buffers are assumed to
           hold contiguous lines from their respective components.
         [ARG: precisions]
           If NULL, all component precisions are deemed to be 8; otherwise, the
           argument points to an array with a single precision value for each
           component.  The precision identifies the number of least
           significant bits which are actually used in each sample.
           If this value is less than 8, one or more most significant bits
           of each byte will be ignored.
           [//]
           There is no implied connection between the precision values, P, and
           the bit-depth of each image component, as provided by the
           `Sprecision' attribute managed by the `siz_params' object passed to
           `kdu_codestream::create'.  The original image sample
           bit-depth may be larger or smaller than the value of P, supplied via
           the `precisions' argument.  In any event, the most significant bit
           of the P-bit integer represented by each sample byte is aligned with
           the most significant bit of image sample words which we actually
           compress internally.  Zero padding and discarding of excess least
           significant bits is applied as required.
           [//]
           These conventions, provide the application with tremendous
           flexibility in how it chooses to represent image sample values.
           Suppose, for example, that the original image sample precision for
           some component is only 1 bit, as represented by the `Sprecision'
           attribute managed by the `siz_params' object (this is the
           bit-depth value actually recorded in the code-stream).  If
           the value of P provided by the `precisions' array is set to 1, the
           bi-level image information is embedded in the least significant bit
           of each byte supplied to this function.  On the other hand, if the
           value of P is 8, the bi-level image information is embedded
           in the most significant bit of each byte.
           [//]
           The sample values supplied to this function are always unsigned,
           regardless of whether or not the `Ssigned' attribute managed
           by `siz_params' identifies an image component as having an
           originally signed representation.  In this, relatively unlikely,
           event, the application is responsible for level adjusting the
           original sample values, by adding 2^{P-1} to each originally signed
           value.
         [ARG: flush_period]
           This argument may be used to control Kakadu's incremental code-stream
           flushing machinery, where applicable.  If 0, no incremental
           flushing will be attempted.  Otherwise, the function checks to
           see if at least this number of lines have been pushed in for
           the first image component, since the last successful call to
           `kdu_codestream::flush' (or since the call to `start').  If so,
           an attempt is made to flush the code-stream again, after first
           calling `kdu_codestream::ready_for_flush' to see whether a new
           call to `kdu_codestream::flush' will succeed.  Incremental flushing
           is possible only under a very narrow set of conditions on the
           packet progression order and precinct dimensions.  For a detailed
           discussion of these conditions, you should carefully review the
           comments provided with `kdu_codestream::flush' before trying to
           use this feature.
      */
    KDU_AUX_EXPORT bool
      push_stripe(kdu_byte *buffer, int stripe_heights[],
                  int *sample_offsets=NULL, int *sample_gaps=NULL,
                  int *row_gaps=NULL, int *precisions=NULL,
                  int flush_period=0);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `push_stripe' function,
           except in the following respect:
           [>>] The stripe samples for all image components must be located
                within a single array, given by the `buffer' argument.  The
                location of the first sample of each component stripe within
                this single array is given by the corresponding entry in the
                `sample_offsets' array.
           [//]
           This form of the function is no more useful (in fact less general)
           than the first form, but is more suitable for the automatic
           construction of Java language bindings by the "kdu_hyperdoc"
           utility.  It can also be more convenient to use when the
           application uses an interleaved buffer.
         [RETURNS]
           True until all samples for all image components have been pushed in,
           at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `push_stripe' function.
         [ARG: sample_offsets]
           Array with one entry for each image component, identifying the
           position of the first sample of that component within the `buffer'
           array.  If this argument is NULL, the implied sample offsets are
           `sample_offsets'[c] = c -- i.e., samples are tightly interleaved.  In
           this case, the interpretation of a NULL `sample_gaps' array is
           modified to match the tight interleaving assumption.
         [ARG: sample_gaps]
           See description of the first form of the `push_stripe' function.  If
           NULL, the sample gaps for all image components are taken to be
           1, which means that the organization of `buffer' must be
           either line- or component- interleaved.  The only exception to this
           is if `sample_offsets' is also NULL, in which case, the sample
           gaps all default to the number of image components, corresponding
           to a sample-interleaved organization.
         [ARG: row_gaps]
           See description of the first form of the `push_stripe' function.  If
           NULL, the lines of each component stripe buffer are assumed to
           be contiguous, meaning that the organization of `buffer' must
           be either component- or sample-interleaved.
         [ARG: precisions]
           See description of the first form of the `push_stripe' function.
         [ARG: flush_period]
           See description of the first form of the `push_stripe' function.
      */
    KDU_AUX_EXPORT bool
      push_stripe(kdu_int16 *stripe_bufs[], int stripe_heights[],
                  int *sample_gaps=NULL, int *row_gaps=NULL,
                  int *precisions=NULL, bool *is_signed=NULL,
                  int flush_period=0);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `push_stripe' function,
           except in the following respects:
           [>>] The stripe samples for each image component are provided with
                a 16-bit representation; as with other forms of the
                `push_stripe' function, the actual number of bits of this
                representation which are used is given by the `precisions'
                argument, but all 16 bits may be used (this is the default).
           [>>] The default representation for each supplied sample value is
                signed, but the application may explicitly identify whether
                or not each component has a signed or unsigned representation.
                Note that there is no required connection between the `Ssigned'
                attribute managed by `siz_params' and the application's
                decision to supply signed or unsigned data to the present
                function.  If the original data for component c was unsigned,
                the application may choose to supply signed sample values here,
                in which case it is responsible for first subtracting
                2^{`precisions'[c]-1} from each sample of image component c.
                If the application supplies unsigned data (setting
                `is_signed'[c] to true), the present function will subtract
                2^{`precisions'[c]-1} from the corresponding sample values.
         [RETURNS]
           True until all samples for all image components have been pushed in,
           at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `push_stripe' function.
         [ARG: sample_gaps]
           See description of the first form of the `push_stripe' function.
         [ARG: row_gaps]
           See description of the first form of the `push_stripe' function.
         [ARG: precisions]
           See description of the first form of the `push_stripe' function,
           but note these two changes: the precision for any component may be
           as large as 16 (this is the default, if `precisions' is NULL);
           and the samples all have a nominally signed representation (not the
           unsigned representation assumed by the first form of the function),
           unless otherwise indicated by a non-NULL `is_signed' argument.
         [ARG: is_signed]
           If NULL, the supplied samples for each component, c, are assumed to
           have a signed representation in the range -2^{`precisions'[c]-1} to
           2^{`precisions'[c]-1}-1.  Otherwise, this argument points to
           an array with one element for each component.  If `is_signed'[c]
           is true, the default signed representation is assumed for that
           component; if false, the component samples are assumed to have an
           unsigned representation in the range 0 to 2^{`precisions'[c]}-1.
           What this means is that the function subtracts 2^{`precisions[c]'-1}
           from the samples of any component for which `is_signed'[c] is false.
           It is allowable to have `precisions'[c]=16 even if `is_signed'[c] is
           false, meaning that the input words are treated as though they were
           16-bit unsigned words.
         [ARG: flush_period]
           See description of the first form of the `push_stripe' function.
      */
    KDU_AUX_EXPORT bool
      push_stripe(kdu_int16 *buffer, int stripe_heights[],
                  int *sample_offsets=NULL, int *sample_gaps=NULL,
                  int *row_gaps=NULL, int *precisions=NULL,
                  bool *is_signed=NULL, int flush_period=0);
      /* [SYNOPSIS]
           Same as the third form of the overloaded `push_stripe' function,
           except that all component buffers are found within the single
           supplied `buffer'.  Specifically, sample values have a
           16-bit signed (but possibly unsigned, depending on the `is_signed'
           argument) representation, rather than an 8-bit unsigned
           representation.  As with the second form of the function, this
           fourth form is provided primarily to facilitate automatic
           construction of Java language bindings by the "kdu_hyperdoc"
           utility.
         [RETURNS]
           True until all samples for all image components have been pushed in,
           at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `push_stripe' function.
         [ARG: sample_offsets]
           Array with one entry for each image component, identifying the
           position of the first sample of that component within the `buffer'
           array.  If this argument is NULL, the implied sample offsets are
           `sample_offsets'[c] = c -- i.e., samples are tightly interleaved.  In
           this case, the interpretation of a NULL `sample_gaps' array is
           modified to match the tight interleaving assumption.
         [ARG: sample_gaps]
           See description of the first form of the `push_stripe' function.  If
           NULL, the sample gaps for all image components are taken to be
           1, which means that the organization of `buffer' must be
           either line- or component- interleaved.  The only exception to this
           is if `sample_offsets' is also NULL, in which case, the sample
           gaps all default to the number of image components, corresponding
           to a sample-interleaved organization.
         [ARG: row_gaps]
           See description of the first form of the `push_stripe' function.  If
           NULL, the lines of each component stripe buffers are assumed to
           be contiguous, meaning that the organization of `buffer' must
           be either component- or sample-interleaved.
         [ARG: precisions]
           See description of the third form of the `push_stripe' function.
         [ARG: is_signed]
           See description of the third form of the `push_stripe' function.
         [ARG: flush_period]
           See description of the first form of the `push_stripe' function.
      */
    KDU_AUX_EXPORT bool
      push_stripe(kdu_int32 *stripe_bufs[], int stripe_heights[],
                  int *sample_gaps=NULL, int *row_gaps=NULL,
                  int *precisions=NULL, bool *is_signed=NULL,
                  int flush_period=0);
      /* [SYNOPSIS]
           Same as the third form of the overloaded `push_stripe' function,
           except that stripe samples for each image component are provided
           with a 32-bit representation; as with other forms of the function,
           the actual number of bits of this representation which are used is
           given by the `precisions' argument, but all 32 bits may be used
           (this is the default).
         [RETURNS]
           True until all samples for all image components have been pushed in,
           at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `push_stripe' function.
         [ARG: sample_gaps]
           See description of the first form of the `push_stripe' function.
         [ARG: row_gaps]
           See description of the first form of the `push_stripe' function.
         [ARG: precisions]
           See description of the first form of the `push_stripe' function,
           but note these two changes: the precision for any component may be
           as large as 32 (this is the default, if `precisions' is NULL);
           and the samples all have a nominally signed representation (not the
           unsigned representation assumed by the first form of the function),
           unless otherwise indicated by a non-NULL `is_signed' argument.
         [ARG: is_signed]
           If NULL, the supplied samples for each component, c, are assumed to
           have a signed representation in the range -2^{`precisions'[c]-1} to
           2^{`precisions'[c]-1}-1.  Otherwise, this argument points to
           an array with one element for each component.  If `is_signed'[c]
           is true, the default signed representation is assumed for that
           component; if false, the component samples are assumed to have an
           unsigned representation in the range 0 to 2^{`precisions'[c]}-1.
           What this means is that the function subtracts 2^{`precisions[c]'-1}
           from the samples of any component for which `is_signed'[c] is false.
           It is allowable to have `precisions'[c]=32 even if `is_signed'[c] is
           false, meaning that the input words are treated as though they were
           32-bit unsigned words.
         [ARG: flush_period]
           See description of the first form of the `push_stripe' function.
      */
    KDU_AUX_EXPORT bool
      push_stripe(kdu_int32 *buffer, int stripe_heights[],
                  int *sample_offsets=NULL, int *sample_gaps=NULL,
                  int *row_gaps=NULL, int *precisions=NULL,
                  bool *is_signed=NULL, int flush_period=0);
      /* [SYNOPSIS]
           Same as the fifth form of the overloaded `push_stripe' function,
           except that all component buffers are found within the single
           supplied `buffer'.  As with the second and fourth forms of the
           function, this sixth form is provided primarily to facilitate
           automatic construction of Java language bindings by the
           "kdu_hyperdoc" utility.
         [RETURNS]
           True until all samples for all image components have been pushed in,
           at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `push_stripe' function.
         [ARG: sample_offsets]
           Array with one entry for each image component, identifying the
           position of the first sample of that component within the `buffer'
           array.  If this argument is NULL, the implied sample offsets are
           `sample_offsets'[c] = c -- i.e., samples are tightly interleaved.  In
           this case, the interpretation of a NULL `sample_gaps' array is
           modified to match the tight interleaving assumption.
         [ARG: sample_gaps]
           See description of the first form of the `push_stripe' function.  If
           NULL, the sample gaps for all image components are taken to be
           1, which means that the organization of `buffer' must be
           either line- or component- interleaved.  The only exception to this
           is if `sample_offsets' is also NULL, in which case, the sample
           gaps all default to the number of image components, corresponding
           to a sample-interleaved organization.
         [ARG: row_gaps]
           See description of the first form of the `push_stripe' function.  If
           NULL, the lines of each component stripe buffers are assumed to
           be contiguous, meaning that the organization of `buffer' must
           be either component- or sample-interleaved.
         [ARG: precisions]
           See description of the fifth form of the `push_stripe' function.
         [ARG: is_signed]
           See description of the fifth form of the `push_stripe' function.
         [ARG: flush_period]
           See description of the first form of the `push_stripe' function.
      */
    KDU_AUX_EXPORT bool
      push_stripe(float *stripe_bufs[], int stripe_heights[],
                  int *sample_gaps=NULL, int *row_gaps=NULL,
                  int *precisions=NULL, bool *is_signed=NULL,
                  int flush_period=0);
      /* [SYNOPSIS]
           Same as the third form of the overloaded `push_stripe' function,
           except that stripe samples for each image component are provided
           with a floating point representation.  In this case, the
           interpretation of the `precisions' member is slightly different,
           as explained below.
         [RETURNS]
           True until all samples for all image components have been pushed in,
           at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `push_stripe' function.
         [ARG: sample_gaps]
           See description of the first form of the `push_stripe' function.
         [ARG: row_gaps]
           See description of the first form of the `push_stripe' function.
         [ARG: precisions]
           If NULL, all component samples are deemed to have a nominal range
           of 1.0; that is, signed values lie in the range -0.5 to +0.5,
           while unsigned values lie in the range 0.0 to 1.0; equivalently,
           the precision is taken to be P=0.  Otherwise, the argument points
           to an array with one precision value for each component.  The
           precision value, P, identifies the nominal range of the input
           samples, such that signed values range from -2^{P-1} to +2^{P-1},
           while unsigned values range from 0 to 2^P.
           [//]
           The value of P, provided by the `precisions' argument may be
           the same, larger or smaller than the actual bit-depth, B, of
           the corresponding image component, as provided by the
           `Sprecision' attribute (or the `Mprecision' attribute) managed
           by the `siz_params' object passed to `kdu_codestream::create'.  The
           relationship between samples represented at bit-depth B and the
           floating point quantities supplied to this function is that the
           latter are understood to have been scaled by the value 2^{P-B}.
           [//]
           While this scaling factor seems quite natural, you should pay
           particular attention to its implications for small values of B.
           For example, when P=1 and B=1, the nominal range of unsigned
           floating point quantities is from 0 to 2, while the actual
           range of 1-bit sample values is obviously from 0 to 1.  Thus,
           the maximum "white" value actually occurs when the floating point
           quantity equals 1.0 (half its nominal maximum value).  For signed
           floating point representations, the implications are even less
           intuitive, with the maximum integer value achieved when the
           floating point sample value is 0.0.  More generally, although the
           nominal range of the floating point component sample values is of
           size 2^P, a small upper fraction -- 2^{-B} -- of this nominal range
           lies beyond the range which can be represented by B-bit samples.
           You can use this "invalid" portion of the nominal range if you
           like, but values may be clipped during decompression.  To minimize
           the impact of this small "invalid" fraction of the nominal range,
           you might choose to set the image bit-depth, B, to a large value
           when compressing data which you really think of as floating
           point data.  This will also help to minimize the effective
           quantization error introduced by reversible compression, if
           used, although irreversible compression makes a lot more sense
           if you are working with floating point samples.
           [//]
           It is worth noting that this function, unlike its predecessors,
           allows P to take both negative and positive values.  For
           implementation reasons, though, we restrict precisions to take
           values in the range -64 to +64.  Also unlike its predecessors,
           this function does not limit the range of input samples.  To a
           certain extent, therefore, you can get away with exceeding the
           nominal dynamic range, without causing overflow.  This extent is
           determined by the number of guard bits.  However, overflow problems
           might be encountered in some decompressor implementations if you
           take too many liberties here.
         [ARG: is_signed]
           If NULL, the supplied samples for each component, c, are assumed to
           have a signed representation, with a nominal range from
           -2^{`precisions'[c]-1} to +2^{`precisions'[c]-1}.  Otherwise, this
           argument points to an array with one element for each component.  If
           `is_signed'[c] is true, the default signed representation is assumed
           for that component; if false, the component samples are assumed to
           have an unsigned representation, with a nominal range from 0.0 to
           2^{`precisions'[c]}.  What this means is that the function subtracts
           2^{`precisions[c]'-1} from the samples of any component for which
           `is_signed'[c] is false -- if `precisions' is NULL, 0.5 is
           subtracted.
         [ARG: flush_period]
           See description of the first form of the `push_stripe' function.
      */
    KDU_AUX_EXPORT bool
      push_stripe(float *buffer, int stripe_heights[],
                  int *sample_offsets=NULL, int *sample_gaps=NULL,
                  int *row_gaps=NULL, int *precisions=NULL,
                  bool *is_signed=NULL, int flush_period=0);
      /* [SYNOPSIS]
           Same as the seventh form of the overloaded `push_stripe' function,
           except that all component buffers are found within the single
           supplied `buffer'.  As with the second, fourth and sixth forms
           of the function, this eighth form is provided primarily to
           facilitate automatic construction of Java language bindings by the
           "kdu_hyperdoc" utility.
         [RETURNS]
           True until all samples for all image components have been pushed in,
           at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `push_stripe' function.
         [ARG: sample_offsets]
           Array with one entry for each image component, identifying the
           position of the first sample of that component within the `buffer'
           array.  If this argument is NULL, the implied sample offsets are
           `sample_offsets'[c] = c -- i.e., samples are tightly interleaved.  In
           this case, the interpretation of a NULL `sample_gaps' array is
           modified to match the tight interleaving assumption.
         [ARG: sample_gaps]
           See description of the first form of the `push_stripe' function.  If
           NULL, the sample gaps for all image components are taken to be
           1, which means that the organization of `buffer' must be
           either line- or component- interleaved.  The only exception to this
           is if `sample_offsets' is also NULL, in which case, the sample
           gaps all default to the number of image components, corresponding
           to a sample-interleaved organization.
         [ARG: row_gaps]
           See description of the first form of the `push_stripe' function.  If
           NULL, the lines of each component stripe buffers are assumed to
           be contiguous, meaning that the organization of `buffer' must
           be either component- or sample-interleaved.
         [ARG: precisions]
           See description of the seventh form of the `push_stripe' function.
         [ARG: is_signed]
           See description of the seventh form of the `push_stripe' function.
         [ARG: flush_period]
           See description of the first form of the `push_stripe' function.
      */
  private: // Helper functions
    kdsc_tile *get_new_tile(); // Uses the free list, if possible
    void release_tile(kdsc_tile *tile); // Releases the tile to the free list
    bool push_common(int flush_period); // Common part of `push_stripe' funcs
  private: // Data
    kdu_codestream codestream;
    int flush_layer_specs; // Num specs provided in `flush' calls
    kdu_long *flush_sizes; // Layer sizes provided in `flush' calls
    kdu_uint16 *flush_slopes; // Layer slopes array provided in `flush' calls
    double size_tolerance; // Value supplied by `start'
    bool record_layer_info_in_comment; // Value supplied by `start'
    bool force_precise;
    bool want_fastest;
    bool all_done; // When all samples have been processed
    int num_components;
    kdsc_component_state *comp_states;
    kdu_coords left_tile_idx; // Indices of left-most tile in current row
    kdu_coords num_tiles; // Tiles wide and remaining tiles vertically.
    kdsc_tile *partial_tiles;
    kdsc_tile *free_tiles;
    int lines_since_flush; // Lines of component 0 pushed in since last flush
    bool flush_on_tile_boundary; // Want flush at start of next tile row
    kdu_thread_env *env; // NULL, if multi-threaded environment not used
    kdu_thread_queue *env_queue; // Used only with `env'
    int env_dbuf_height; // Used only with `env'
    kdsc_flush_worker *env_flush_worker; // Used only with `env'
    bool env_unconfirmed_flush; // If true, we are in the period
       // immediately following the registration of an intent to flush the
       // codestream, but we still do not know if the flush was successful
       // (i.e., we don't know if `codestream.ready_for_flush' returned true
       // or false inside `env_flush_worker::do_job').
  };


#endif // KDU_STRIPE_COMPRESSOR_H
