/******************************************************************************/
// File: kdu_stripe_decompressor.h [scope = APPS/SUPPORT]
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
   Defines the `kdu_stripe_decompressor' object, a high level, versatile
facility for decompressing images in memory by stripes.  The application
provides stripe buffers, of any desired size and passes them to the object to
be filled with decompressed image component samples.  The object takes care of
all the other details to optimally sequence the decompression tasks.  This
allows the image to be decompressed in one hit, into a memory buffer, or to be
recovered incrementally into application-defined stripe buffers.  Provides
an easy way to use Kakadu without having to know much about the JPEG2000, but
advanced developers may still wish to use the lower level mechanisms to avoid
memory copying, or for customed sequencing of the decompression machinery.
*******************************************************************************/

#ifndef KDU_STRIPE_DECOMPRESSOR_H
#define KDU_STRIPE_DECOMPRESSOR_H

#include "kdu_compressed.h"
#include "kdu_sample_processing.h"

// Objects declared here:
class kdu_stripe_decompressor;

// Declared elsewhere:
struct kdsd_tile;
struct kdsd_component_state;

/******************************************************************************/
/*                         kdu_stripe_decompressor                            */
/******************************************************************************/

class kdu_stripe_decompressor {
  /* [BIND: reference]
     [SYNOPSIS]
       This object provides a high level interface to the Kakadu decompression
       machinery, which is capable of satisfying the needs of most developers
       while providing essentially a one-function-call solution for simple
       applications.  Most new developers will probably wish to base their
       decompression applications either upon this object, or the
       `kdu_region_decompressor' object.
       [//]
       It should be noted, however, that some performance benefits can be
       obtained by directly interfacing with the `kdu_multi_synthesis' object
       or, at an even lower level, directly creating your own `kdu_synthesis'
       and/or `kdu_decoder' objects, from which to pull individual image
       lines -- these approaches can often avoid unnecessary copying and
       level shifting of image samples.  Nevertheless, there has been a lot
       of demand for a dead-simple, yet also powerful interface to Kakadu,
       and this object is intended to fill that requirement.  In fact, the
       various objects found in the "support" directory
       (`kdu_stripe_compressor', `kdu_stripe_decompressor' and
       `kdu_region_decompressor') are all aimed at meeting the needs of 90% of
       the applications using Kakadu.  That is not to say that these objects
       are all that is required.  You still need to open streams of one
       form or another and create a `kdu_codestream' interface.
       [//]
       In a typical decompression application based on this object, you will
       need to do the following:
       [>>] Create a `kdu_codestream' object.
       [>>] Optionally use one of the `kdu_codestream::apply_input_restrictions'
            functions to adjust the portion of the original compressed
            image that you want to recover -- you can also use these functions
            to configure the set of image components you want decompressed
            and whether or not you want any multi-component transforms to be
            inverted.
       [>>] Initialize the `kdu_stripe_decompressor' object, by calling
            `kdu_stripe_decompressor::start'.
       [>>] Pull image stripes from `kdu_stripe_decompressor::pull_stripe' until
            the image is fully decompressed (you can do it all in one go, into
            a memory buffer of your choice, if you like).
       [>>] Call `kdu_stripe_decompressor::finish' (not strictly necessary).
       [>>] Call `kdu_codestream::destroy'.
       [//]
       For a tuturial example of how to use the present object in a typical
       application, consult the Kakadu demo application,
       "kdu_buffered_decompress".
       [//]
       It is worth noting that this object is built directly on top of the
       services offered by `kdu_multi_synthesis', so for a thorough
       understanding of how things work, you might like to consult the
       documentation for that object as well.
       [//]
       Most notably, the image components manipulated by this object are
       those which are described by the `kdu_codestream' machinery as
       output image components, as opposed to codestream image components.
       For a discussion of the differences between codestream and output
       image components, see the second form of the
       `kdu_codestream::apply_input_restrictions' function.  For our
       purposes here, however, it is sufficient to note that output
       components are derived from codestream components by applying any
       multi-component (or decorrelating colour) transforms.  Output
       components are the image components which the content creator
       intends to be rendered.  Note, however, that if the component access
       mode is set to `KDU_WANT_CODESTREAM_COMPONENTS'
       instead of `KDU_WANT_OUTPUT_COMPONENTS' (see
       `kdu_codestream::apply_input_restrictions'), the codestream image
       components will appear to be the output components, so no loss of
       flexibility is incurred by insisting that this object processes
       output components.
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
    KDU_AUX_EXPORT kdu_stripe_decompressor();
      /* [SYNOPSIS]
           All the real initialization is done within `start'.  You may
           use a single `kdu_stripe_decompressor' object to process multiple
           images, bracketing each use by calls to `start' and `finish'.
      */
    ~kdu_stripe_decompressor() { finish(); }
      /* [SYNOPSIS]
           Calls `finish' to do all the internal state cleanup.
      */
    KDU_AUX_EXPORT void
      start(kdu_codestream codestream, bool force_precise=false,
            bool want_fastest=false, kdu_thread_env *env=NULL,
            kdu_thread_queue *env_queue=NULL, int env_dbuf_height=0);
      /* [SYNOPSIS]
           Call this function to initialize the object for decompression.  Each
           call to `start' must be matched by a call to `finish', but you may
           re-use the object to process subsequent images, if you like.
         [ARG: codestream]
           Interface to a `kdu_codestream' object whose `create' function has
           already been called.  Before passing the code-stream to this
           function, you might like to alter the geometry by calling
           `kdu_codestream::change_appearance', or you might like to restrict
           the spatial region, image components or number of layers which
           will appear to be present during decompression, by calling
           one of the `kdu_codestream::apply_input_restrictions' functions.
         [ARG: force_precise]
           If true, 32-bit internal representations are used by the
           decompression engines created by this object, regardless of the
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
         [ARG: env]
           This argument is used to establish multi-threaded processing.  For
           a discussion of the multi-threaded processing features offered
           by the present object, see the introductory comments to
           `kdu_stripe_decompressor'.  We remind you here, however, that
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
           [//]
           One application for a non-NULL `env_queue' might be one which
           processes two frames of a video sequence in parallel.  There
           can be some benefit to doing this, since it can avoid the small
           amount of thread idle time which often appears at the end of
           the last call to the `process' function prior to `finish'.  In
           this case, each concurrent frame would have its own `env_queue', and
           its own `kdu_stripe_decompressor' object.  Moreover, the
           `env_queue' associated with a given `kdu_stripe_decompressor'
           object can be used to run a job which invokes the `start',
           `process' and `finish' member functions.  In this case, however,
           it is particularly important that the `start', `process' and
           `finish' functions all be called from within the execution of a
           single job, since otherwise there is no guarantee that they would
           all be executed from the same thread, whose importance has
           already been stated above.
         [ARG: env_dbuf_height]
           This argument may be used to introduce and control parallelism
           in the DWT processing steps, allowing you to distribute the
           load associated with multiple tile-components across multiple
           threads. In the simplest case, this argument is 0, and parallel
           processing applies only to the block decoding processes.  For
           a small number of processors, this is usually sufficient to keep
           all CPU's active.  If this argument is non-zero, however, the
           `kdu_multi_synthesis' objects on which all processing is based,
           are created with `double_buffering' equal to true and a
           `processing_stripe_height' equal to the value supplied for this
           argument.  See `kdu_multi_synthesis::create' for a more
           comprehensive discussion of double buffering principles and
           guidelines.
      */
    KDU_AUX_EXPORT bool finish();
      /* [SYNOPSIS]
           Each call to `start' must be bracketed by a call to `finish', if
           you wish to re-use the object to decompress further code-streams.
           Calling this function is not strictly necessary if you intend to
           use the function only once.
         [RETURNS]
           True only if all available image data was recovered using the
           `pull_stripe' function.
      */
    KDU_AUX_EXPORT bool
      get_recommended_stripe_heights(int preferred_min_height,
                                     int absolute_max_height,
                                     int stripe_heights[],
                                     int *max_stripe_heights);
      /* [SYNOPSIS]
           Convenience function, provides recommended stripe heights for the
           most efficient use of the `pull_stripe' function, subject to
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
           depending on how much data has already been retrieved for each
           image component by previous calls to `pull_stripe'.  However,
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
           recommended stripe height for that component.  Note that the number
           of image components is the value returned by
           `kdu_codestream::get_num_components' with its optional
           `want_output_comps' argument set to true.
         [ARG: max_stripe_heights]
           If non-NULL, this argument points to an array with one entry for
           each image component, which receives an upper bound on the stripe
           height which this function will ever recommend for that component.
           There is no upper bound on the stripe height you can actually use
           in a call to `pull_stripe', only an upper bound on the
           recommendations which this function will produce as it is called
           from time to time.  Thus, if you intend to use this function to
           guide your stripe selection, the `max_stripe_heights' information
           might prove useful in pre-allocating storage for stripe buffers
           provided by your application.
      */
    KDU_AUX_EXPORT bool
      pull_stripe(kdu_byte *stripe_bufs[], int stripe_heights[],
                  int *sample_gaps=NULL, int *row_gaps=NULL,
                  int *precisions=NULL);
      /* [SYNOPSIS]
           Decompresses new vertical stripes of samples from each image
           component.  The number of entries in each of the arrays here is
           equal to the number of image components, as returned by
           `kdu_codestream::get_num_components' with its optional
           `want_output_comps' argument set to true -- note that this value
           is affected by calls to `kdu_codestream::apply_input_restrictions'
           which may have been made prior to supplying the `kdu_codestream'
           object to `start'.   Each stripe spans the entire width of its
           image component, which must be no larger than the ratio between the
           corresponding entries in the `row_gaps' and `sample_gaps' arrays.
           [//]
           Each successive call to this function advances the vertical position
           within each image component by the number of lines identified within
           the `stripe_heights' array.  Although components nominally advance
           from the top to the bottom, if `kdu_codestream::change_appearance'
           was used to flip the appearance of the vertical dimension, the
           function actually advances the true underlying image
           components from the bottom up to the top.  This is exactly what one
           should expect from the description of
           `kdu_codestream::change_appearance' and requires no special
           processing in the implemenation of the present object.
           [//]
           Although considerable flexibility is offered with regard to stripe
           heights, there are a number of constraints.  As a general rule, you
           should endeavour to advance the various image components in a
           proportional way, when processing incrementally (as opposed to
           decompressing the entire image into a single buffer, with a single
           call to this function).  What this means is that the stripe height
           for each component should, ideally, be inversely proportional to its
           vertical sub-sampling factor.  If you do not intend to decompress
           the components in a proportional fashion, the following notes should
           be taken into account:
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
           [//]
           In addition to the constraints and guidelines mentioned above
           regarding the selection of suitable stripe heights, it is worth
           noting that the efficiency (computational and memory efficiency)
           with which image data is decompressed depends upon how your
           stripe heights interact with image tiling.  If the image is
           untiled, you are generally best off working with small stripes,
           unless your application naturally provides larger stripe buffers.
           If, however, the image is tiled, then the implementation is most
           efficient if your stripes happen to be aligned on vertical tile
           boundaries.  To simplify the determination of suitable stripe
           heights (all other things being equal), the present object
           provides a convenient utility, `get_recommended_stripe_heights',
           which you can call at any time.
           [//]
           To understand the interpretation of the sample byte values retrieved
           by this function, consult the comments appearing with the
           `precisions' argument below.  Other forms of the overloaded
           `pull_stripe' function are provided to allow for the accurate
           representation of higher precision image samples.
         [RETURNS]
           True until all samples of all image components have been
           decompressed and returned, at which point the function returns false.
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
           number of lines to be decompressed for that component in the present
           call.  All entries must be non-negative.  See the discussion above,
           on the various constraints and guidelines which may exist regarding
           stripe heights and their interaction with tiling and sub-sampling.
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
           component.  The precision identifies the number of significant bits
           used to represent each sample.  If this value is less than 8, the
           remaining most significant bits in each byte will be set to 0.
           [//]
           There is no implied connection between the precision values, P, and
           the bit-depth, B, of each image component, as found in the
           code-stream's SIZ marker segment, and returned via
           `kdu_codestream::get_bit_depth'.  The original image sample
           bit-depth, B, may be larger or smaller than the value of P supplied
           via the `precisions' argument.  In any event, the most significant
           bit of the P-bit integer represented by each sample byte is aligned
           with the most significant bit of the B-bit integers associated
           with the original compressed image components.
           [//]
           These conventions, provide the application with tremendous
           flexibility in how it chooses to represent image sample values.
           Suppose, for example, that the original image sample precision for
           some component is only B=1 bit, as recorded in the code-stream
           main header.  If the value of P provided by the `precisions' array
           is set to 1, the bi-level image information is written into the
           least significant bit of each byte supplied to this function.  On
           the other hand, if the value of P is 8, the bi-level image
           information is written into the most significant bit of each byte.
           [//]
           The sample values recovered using this function are always unsigned,
           regardless of whether or not the original image samples had a
           signed or unsigned representation (as recorded in the SIZ marker
           segment, and returned via `kdu_codestream::get_bit_depth').  If
           the original samples were signed, or the application requires a
           signed representation for other reasons, the application is
           responsible for level adjusting the data returned here, subtracting
           2^{P-1} from the unsigned values.
      */
    KDU_AUX_EXPORT bool
      pull_stripe(kdu_byte *buffer, int stripe_heights[],
                  int *sample_offsets=NULL, int *sample_gaps=NULL,
                  int *row_gaps=NULL, int *precisions=NULL);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `pull_stripe' function,
           except in the following respect:
           [>>] The stripe samples for all image components are located
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
           True until all samples of all image components have been
           decompressed and returned, at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `pull_stripe' function.
         [ARG: sample_offsets]
           Array with one entry for each image component, identifying the
           position of the first sample of that component within the `buffer'
           array.  If this argument is NULL, the implied sample offsets are
           `sample_offsets'[c] = c -- i.e., samples are tightly interleaved.  In
           this case, the interpretation of a NULL `sample_gaps' array is
           modified to match the tight interleaving assumption.
         [ARG: sample_gaps]
           See description of the first form of the `pull_stripe' function.  If
           NULL, the sample gaps for all image components are taken to be
           1, which means that the organization of `buffer' must be
           either line- or component- interleaved.  The only exception to this
           is if `sample_offsets' is also NULL, in which case, the sample
           gaps all default to the number of image components, corresponding
           to a sample-interleaved organization.
         [ARG: row_gaps]
           See description of the first form of the `pull_stripe' function.  If
           NULL, the lines of each component stripe buffer are assumed to
           be contiguous, meaning that the organization of `buffer' must
           be either component- or sample-interleaved.
         [ARG: precisions]
           See description of the first form of the `pull_stripe' function.
      */
    KDU_AUX_EXPORT bool
      pull_stripe(kdu_int16 *stripe_bufs[], int stripe_heights[],
                  int *sample_gaps=NULL, int *row_gaps=NULL,
                  int *precisions=NULL, bool *is_signed=NULL);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `pull_stripe' function,
           except in the following respects:
           [>>] The stripe samples for each image component are written with
                a 16-bit representation; as with other forms of the
                `pull_stripe' function, the actual number of bits of this
                representation which are used is given by the `precisions'
                argument, but all 16 bits may be used (this is the default).
           [>>] The default representation for each recovered sample value is
                signed, but the application may explicitly identify whether
                or not each component is to have a signed or unsigned
                representation.  Note that there is no required connection
                between the `Ssigned' attribute managed by `siz_params' and
                the application's decision to request signed or unsigned data
                from the present function.  If the original data for component
                c was unsigned, the application may choose to request signed
                sample values here, or vice-versa.
         [RETURNS]
           True until all samples of all image components have been
           decompressed and returned, at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `pull_stripe' function.
         [ARG: sample_gaps]
           See description of the first form of the `pull_stripe' function.
         [ARG: row_gaps]
           See description of the first form of the `pull_stripe' function.
         [ARG: precisions]
           See description of the first form of the `pull_stripe' function,
           but note these two changes: the precision for any component may be
           as large as 16 (this is the default, if `precisions' is NULL);
           and the samples all have a nominally signed representation (not the
           unsigned representation assumed by the first form of the function),
           unless otherwise indicated by a non-NULL `is_signed' argument.
         [ARG: is_signed]
           If NULL, the samples recovered for each component, c, will
           have a signed representation in the range -2^{`precisions'[c]-1} to
           2^{`precisions'[c]-1}-1.  Otherwise, this argument points to
           an array with one element for each component.  If `is_signed'[c]
           is true, the default signed representation is used for that
           component; if false, the component samples have an
           unsigned representation in the range 0 to 2^{`precisions'[c]}-1.
           What this means is that the function adds 2^{`precisions[c]'-1}
           to the samples of any component for which `is_signed'[c] is false.
           It is allowable to have `precisions'[c]=16 even if `is_signed'[c] is
           false, although this means that the `kdu_int16' sample values
           are really being used to store unsigned short integers
           (`kdu_uint16').
      */
    KDU_AUX_EXPORT bool
      pull_stripe(kdu_int16 *buffer, int stripe_heights[],
                  int *sample_offsets=NULL, int *sample_gaps=NULL,
                  int *row_gaps=NULL, int *precisions=NULL,
                  bool *is_signed=NULL);
      /* [SYNOPSIS]
           Same as the third form of the overloaded `pull_stripe' function,
           except that all component buffers are found within the single
           supplied `buffer'.  Specifically, sample values have a
           16-bit signed (but possibly unsigned, depending upon the
           `is_signed' argument) representation, rather than an 8-bit unsigned
           representation.  As with the second form of the function, this
           fourth form is provided primarily to facilitate automatic
           construction of Java language bindings by the "kdu_hyperdoc"
           utility.
         [RETURNS]
           True until all samples of all image components have been
           decompressed and returned, at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `pull_stripe' function.
         [ARG: sample_offsets]
           Array with one entry for each image component, identifying the
           position of the first sample of that component within the `buffer'
           array.  If this argument is NULL, the implied sample offsets are
           `sample_offsets'[c] = c -- i.e., samples are tightly interleaved.  In
           this case, the interpretation of a NULL `sample_gaps' array is
           modified to match the tight interleaving assumption.
         [ARG: sample_gaps]
           See description of the first form of the `pull_stripe' function.  If
           NULL, the sample gaps for all image components are taken to be
           1, which means that the organization of `buffer' must be
           either line- or component- interleaved.  The only exception to this
           is if `sample_offsets' is also NULL, in which case, the sample
           gaps all default to the number of image components, corresponding
           to a sample-interleaved organization.
         [ARG: row_gaps]
           See description of the first form of the `pull_stripe' function.  If
           NULL, the lines of each component stripe buffers are assumed to
           be contiguous, meaning that the organization of `buffer' must
           be either component- or sample-interleaved.
         [ARG: precisions]
           See description of the third form of the `pull_stripe' function.
         [ARG: is_signed]
           See description of the third form of the `pull_stripe' function.
      */
    KDU_AUX_EXPORT bool
      pull_stripe(kdu_int32 *stripe_bufs[], int stripe_heights[],
                  int *sample_gaps=NULL, int *row_gaps=NULL,
                  int *precisions=NULL, bool *is_signed=NULL);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `pull_stripe' function,
           except that stripe samples for each image component are provided
           with a 32-bit representation; as with other forms of the function,
           the actual number of bits of this representation which are
           used is given by the `precisions' argument, but all 32 bits may
           be used (this is the default).
         [RETURNS]
           True until all samples of all image components have been
           decompressed and returned, at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `pull_stripe' function.
         [ARG: sample_gaps]
           See description of the first form of the `pull_stripe' function.
         [ARG: row_gaps]
           See description of the first form of the `pull_stripe' function.
         [ARG: precisions]
           See description of the first form of the `pull_stripe' function,
           but note these two changes: the precision for any component may be
           as large as 32 (this is the default, if `precisions' is NULL);
           and the samples all have a nominally signed representation (not the
           unsigned representation assumed by the first form of the function),
           unless otherwise indicated by a non-NULL `is_signed' argument.
         [ARG: is_signed]
           If NULL, the samples recovered for each component, c, will
           have a signed representation in the range -2^{`precisions'[c]-1} to
           2^{`precisions'[c]-1}-1.  Otherwise, this argument points to
           an array with one element for each component.  If `is_signed'[c]
           is true, the default signed representation is used for that
           component; if false, the component samples have an
           unsigned representation in the range 0 to 2^{`precisions'[c]}-1.
           What this means is that the function adds 2^{`precisions[c]'-1}
           to the samples of any component for which `is_signed'[c] is false.
           It is allowable to have `precisions'[c]=32 even if `is_signed'[c] is
           false, although this means that the `kdu_int32' sample values
           are really being used to store unsigned integers (`kdu_uint32').
      */
    KDU_AUX_EXPORT bool
      pull_stripe(kdu_int32 *buffer, int stripe_heights[],
                  int *sample_offsets=NULL, int *sample_gaps=NULL,
                  int *row_gaps=NULL, int *precisions=NULL,
                  bool *is_signed=NULL);
      /* [SYNOPSIS]
           Same as the fifth form of the overloaded `pull_stripe' function,
           except that all component buffers are found within the single
           supplied `buffer'.  As with the second form of the function,
           this sixth form is provided primarily to facilitate automatic
           construction of Java language bindings by the "kdu_hyperdoc"
           utility.
         [RETURNS]
           True until all samples of all image components have been
           decompressed and returned, at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `pull_stripe' function.
         [ARG: sample_offsets]
           Array with one entry for each image component, identifying the
           position of the first sample of that component within the `buffer'
           array.  If this argument is NULL, the implied sample offsets are
           `sample_offsets'[c] = c -- i.e., samples are tightly interleaved.  In
           this case, the interpretation of a NULL `sample_gaps' array is
           modified to match the tight interleaving assumption.
         [ARG: sample_gaps]
           See description of the first form of the `pull_stripe' function.  If
           NULL, the sample gaps for all image components are taken to be
           1, which means that the organization of `buffer' must be
           either line- or component- interleaved.  The only exception to this
           is if `sample_offsets' is also NULL, in which case, the sample
           gaps all default to the number of image components, corresponding
           to a sample-interleaved organization.
         [ARG: row_gaps]
           See description of the first form of the `pull_stripe' function.  If
           NULL, the lines of each component stripe buffers are assumed to
           be contiguous, meaning that the organization of `buffer' must
           be either component- or sample-interleaved.
         [ARG: precisions]
           See description of the fifth form of the `pull_stripe' function.
         [ARG: is_signed]
           See description of the fifth form of the `pull_stripe' function.
      */
    KDU_AUX_EXPORT bool
      pull_stripe(float *stripe_bufs[], int stripe_heights[],
                  int *sample_gaps=NULL, int *row_gaps=NULL,
                  int *precisions=NULL, bool *is_signed=NULL);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `pull_stripe' function,
           except that stripe samples for each image component are provided
           with a floating point representation.  In this case, the
           interpretation of the `precisions' member is slightly different,
           as explained below.
         [RETURNS]
           True until all samples of all image components have been
           decompressed and returned, at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `pull_stripe' function.
         [ARG: sample_gaps]
           See description of the first form of the `pull_stripe' function.
         [ARG: row_gaps]
           See description of the first form of the `pull_stripe' function.
         [ARG: precisions]
           If NULL, all component samples are deemed to have a nominal range
           of 1.0; that is, signed values lie in the range -0.5 to +0.5,
           while unsigned values lie in the range 0.0 to 1.0; equivalently,
           the precision is taken to be P=0.  Otherwise, the argument points
           to an array with one precision value for each component.  The
           precision value, P, identifies the nominal range of the samples
           which are produced, such that signed values range from
           -2^{P-1} to +2^{P-1}, while unsigned values range from 0 to 2^P.
           [//]
           The value of P, provided by the `precisions' argument may be
           the same, larger or smaller than the actual bit-depth, B, of
           the corresponding image component, as provided by the
           `Sprecision' attribute (or the `Mprecision' attribute) managed
           by the `siz_params' object passed to `kdu_codestream::create'.  The
           relationship between samples represented at bit-depth B and the
           floating point quantities generated by this function is that the
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
           [//]
           There is no guarantee that returned component samples will lie
           entirely within the range dictated by the corresponding B-bit
           integers, or even within the nominal range.  This is because
           the function does not perform any clipping of out-of-range
           values, and the impact of quantization effects in the subband
           domain is hard to quantify precisely in the image domain.
           [//]
           It is worth noting that this function, unlike its predecessors,
           allows P to take both negative and positive values.  For
           implementation reasons, though, we restrict precisions to take
           values in the range -64 to +64.
         [ARG: is_signed]
           If NULL, the samples returned for each component, c, will have a
           signed representation, with a nominal range from
           -2^{`precisions'[c]-1} to +2^{`precisions'[c]-1}.  Otherwise, this
           argument points to an array with one element for each component.  If
           `is_signed'[c] is true, the default signed representation is adopted
           for that component; if false, the component samples are assigned
           an unsigned representation, with a nominal range from 0.0 to
           2^{`precisions'[c]}.  What this means is that the function adds
           2^{`precisions[c]'-1} to the samples of any component for which
           `is_signed'[c] is false, before returning them -- if `precisions'
           is NULL, 0.5 is added.
      */
    KDU_AUX_EXPORT bool
      pull_stripe(float *buffer, int stripe_heights[],
                  int *sample_offsets=NULL, int *sample_gaps=NULL,
                  int *row_gaps=NULL, int *precisions=NULL,
                  bool *is_signed=NULL);
      /* [SYNOPSIS]
           Same as the seventh form of the overloaded `pull_stripe' function,
           except that all component buffers are found within the single
           supplied `buffer'.  As with the second, fourth and sixth forms of
           the function, this eighth form is provided primarily to facilitate
           automatic construction of Java language bindings by the
           "kdu_hyperdoc" utility.
         [RETURNS]
           True until all samples of all image components have been
           decompressed and returned, at which point the function returns false.
         [ARG: stripe_heights]
           See description of the first form of the `pull_stripe' function.
         [ARG: sample_offsets]
           Array with one entry for each image component, identifying the
           position of the first sample of that component within the `buffer'
           array.  If this argument is NULL, the implied sample offsets are
           `sample_offsets'[c] = c -- i.e., samples are tightly interleaved.  In
           this case, the interpretation of a NULL `sample_gaps' array is
           modified to match the tight interleaving assumption.
         [ARG: sample_gaps]
           See description of the first form of the `pull_stripe' function.  If
           NULL, the sample gaps for all image components are taken to be
           1, which means that the organization of `buffer' must be
           either line- or component- interleaved.  The only exception to this
           is if `sample_offsets' is also NULL, in which case, the sample
           gaps all default to the number of image components, corresponding
           to a sample-interleaved organization.
         [ARG: row_gaps]
           See description of the first form of the `pull_stripe' function.  If
           NULL, the lines of each component stripe buffers are assumed to
           be contiguous, meaning that the organization of `buffer' must
           be either component- or sample-interleaved.
         [ARG: precisions]
           See description of the seventh form of the `pull_stripe' function.
         [ARG: is_signed]
           See description of the seventh form of the `pull_stripe' function.
      */
  private: // Helper functions
    kdsd_tile *get_new_tile(); // Uses the free list, if possible
    void release_tile(kdsd_tile *tile); // Releases the tile to the free list
    bool pull_common(); // Common part of `pull_stripe' funcs
  private: // Data
    kdu_codestream codestream;
    bool force_precise;
    bool want_fastest;
    bool all_done; // When all samples have been processed
    int num_components;
    kdsd_component_state *comp_states;
    kdu_coords left_tile_idx; // Indices of left-most tile in current row
    kdu_coords num_tiles; // Tiles wide and remaining tiles vertically.
    kdsd_tile *partial_tiles;
    kdsd_tile *free_tiles;
    kdu_thread_env *env; // NULL, if multi-threaded environment not used
    kdu_thread_queue *env_queue; // Used only with `env'
    int env_dbuf_height; // Used only with `env'
  };

#endif // KDU_STRIPE_DECOMPRESSOR_H
