/*****************************************************************************/
// File: mj2.h [scope = APPS/COMPRESSED-IO]
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
   Defines classes which work together with those defined in "jp2.h"
to implement the services required by readers and writers of the
MJP2 (Motion JPEG2000) file format.  Appropriate `kdu_compressed_source'
and `kdu_compressed_target' derived objects are defined here which may
be used to interface the `kdu_codestream' object and all related
JPEG2000 code-stream functionality with motion video content.
******************************************************************************/
#ifndef MJ2_H
#define MJ2_H

#include "kdu_video_io.h"
#include "jp2.h"

// Classes defined here.
class mj2_source;
class mj2_video_source;
class mj2_target;
class mj2_video_target;

// Classes defined elsewhere.
class mj_video_track;
#ifdef KDU_MAKE_FAKE_OPAQUE_OBJS_FOR_MNI
  class mj_movie { int __v_; };
#else
  class mj_movie;
#endif // KDU_MAKE_FAKE_OPAQUE_OBJS_FOR_MNI

/*****************************************************************************/
/*                             mj2_video_source                              */
/*****************************************************************************/

#define MJ2_GRAPHICS_COPY            ((kdu_int16) 0x0000)
#define MJ2_GRAPHICS_BLUE_SCREEN     ((kdu_int16) 0x0024)
#define MJ2_GRAPHICS_ALPHA           ((kdu_int16) 0x0100)
#define MJ2_GRAPHICS_PREMULT_ALPHA   ((kdu_int16) 0x0101)
#define MJ2_GRAPHICS_COMPONENT_ALPHA ((kdu_int16) 0x0110)

class mj2_video_source : public kdu_compressed_video_source {
  /* [BIND: reference]
     [SYNOPSIS]
       Manages access to a single video track within an MJ2 presentation.
       Use the `mj2_source::access_video_track' function to access an
       instance of this class, which may then be used as a compressed
       data source (on a frame-by-frame basis) in calls to
       `kdu_codestream::create'.
  */
  // --------------------------------------------------------------------------
  public: // Functions specific to Motion JPEG2000 video tracks
    KDU_AUX_EXPORT kdu_uint32 get_track_idx();
      /* [SYNOPSIS]
           Returns the track index associated with the current video
           track.  Valid track indices are always non-zero.
      */
    KDU_AUX_EXPORT kdu_int16 get_compositing_order();
      /* [SYNOPSIS]
           Identifies the front-to-back position of the current video track
           when compositing multiple tracks.  Smaller values are further
           forward (closer to the viewer) than larger values.  Both positive
           and negative values may occur, within the limits of the 16-bit
           signed integer data type.
      */
    KDU_AUX_EXPORT kdu_int16
      get_graphics_mode(kdu_int16 &op_red, kdu_int16 &op_green,
                        kdu_int16 &op_blue);
      /* [SYNOPSIS]
           Identifies the raster operations which should be used to combine
           this track with others on the display.  Tracks are added in
           the compositing order indicated by `get_compositing_order'.
         [RETURNS]
           One of the following values:
           [>>] `MJ2_GRAPHICS_COPY' -> copy.  Note that in this case, any
                opacity channels specified via the `jp2_channels' object
                returned via `access_channels' are to be ignored.
           [>>] `MJ2_GRAPHICS_BLUE_SCREEN' -> blue screen the image using
                the supplied op-colour.  As above, any opacity channels
                identified via the `jp2_channels' object returned via
                `access_channels' are to be ignored.
           [>>] `MJ2_GRAPHICS_ALPHA' -> alpha blend with existing contents.
                The alpha channel is determined by invoking the
                `jp2_channels::get_opacity_mapping' function on the object
                returned via `access_channels'.
           [>>] `MJ2_GRAPHICS_PREMULT_ALPHA' -> alpha blend, where compressed
                data has already been blended with black.  The premultiplied
                alpha blending channel is determined by invoking the
                `jp2_channels::get_premult_mapping' function on the object
                returned via `access_channels'.
           [>>] `MJ2_GRAPHICS_COMPONENT_ALPHA' -> alpha blend for all
                cases other than that in which a single alpha channel is
                applied to all intensity channels uniformly.  This includes
                the case in which multiple alpha channels are used, or a
                single alpha channel is applied to only some of the intensity
                channels.  The `jp2_channels' object returned by
                `access_channels' provides details of the actual alpha
                blending which is required.
      */
    KDU_AUX_EXPORT kdu_int16 get_graphics_mode();
      /* [SYNOPSIS]
           Same as first form of the overloaded `get_graphics_mode' function,
           except that no op-colour information is returned.  If the return
           value is `MJ2_GRAPHICS_BLUE_SCREEN', you will have to use the
           first form to obtain the op-colour for blue screening.
      */
    KDU_AUX_EXPORT void
      get_geometry(double &presentation_width,
                   double &presentation_height,
                   double matrix[], bool for_movie=true);
      /* [SYNOPSIS]
           This function provides information regarding the geometric
           transformations which should be applied to the decompressed
           imagery associated with this track.  The `presentation_width'
           and `presentation_height' values represent conceptual dimensions
           to which the frame samples are to be scaled, so as to produce
           a presentation frame anchored at the origin.  This presentation
           frame is then subjected to affine transformation, using
           parameters described via the 9-element `matrix'.  The matrix
           is actually capable of representing perspective transformations,
           but is restricted to affine operations only by the
           Motion JPEG2000 standard.  The `for_movie' argument is used
           to select whether the geometric transformations described by
           this function should be those described in the track header
           alone (`for_movie'=false) or include also any additional
           transformations specified for the whole movie (`for_movie'=true).
         [ARG: presentation_width]
           Used to return a conceptual presentation width to which each
           frame is scaled prior to applying the `matrix'.  We say that this
           is "conceptual", because the effect of such scaling can always be
           folded into the matrix' itself, so it is unlikely that a good
           rendering implementation will actually scale first and then apply
           the affine transform.
         [ARG: presentation_height]
           Used to return a conceptual presentation height, to which each
           frame is scaled prior to applying the `matrix'.  The same
           considerations apply as those discussed for `presentation_width'.
         [ARG: matrix]
           3x3 matrix, stored in raster order as (a,c,x), (b,d,y), (u,v,w),
           where each point (p,q) on the surface of the presentation image
           is subjected to the following transformation:
           [>>] (m,n,z)^t = M * (p,q,1)^t  -- here, "^t" denotes transpose
                and M is the matrix whose rows are (a,c,x), (b,d,y) and
                (u,v,w).
           [//]
           The transformed point lies at x' = m/z and y'=n/z.  Although
           this matrix has the capability to represent a general perspective
           transformation of the frame, the Motion JPEG2000 standard
           insists that u=0, v=0 and w=1, so that z=1 and the transform
           is always affine.
         [ARG: for_movie]
           If false, the geomtric transformations described by this
           function are resticted to those identified in the track header.
           Otherwise, the geometric transformations include any additional
           matrix transformation described in the global movie header.
      */
    KDU_AUX_EXPORT void
      get_cardinal_geometry(kdu_dims &pre_dims, bool &transpose,
                            bool &vflip, bool &hflip,
                            bool for_movie=true);
      /* [SYNOPSIS]
           This function plays a similar role to `get_geometry', except
           that the geometric transforms are approximated, if necessary, by
           ones which produce an image aligned with the cardinal (horizontal
           and vertical) axes.  In practice, there should be few if any
           movies for which this requires any kind of approximation.
           [//]
           To achieve the (approximated) geometric transformation, each
           original video frame should be scaled and offset, as required,
           to fit within the region represented by `pre_dims'.  The
           resulting frame, having these dimensions, should then be
           flipped and/or transposed according to the flags returned via
           `hflip', `vflip' and `transpose'.  Applying these flags to
           `pre_dims.to_apparent' will yield the dimensions and location
           of the frame, as it is to be presented on the rendered track
           or the complete rendered movie, depending on whether
           `for_movie' is false or true.
         [ARG: pre_dims]
           Used to return the location and dimensions of each frame after
           scaling and offsetting, prior to the application of the geometric
           transformation flags returned via `transpose', `vflip' and
           `hflip'.
         [ARG: transpose]
           Used to return the transpose flag.  If true, vertical
           coordinates should be exchanged with horizontal during rendering
           of the frame.
         [ARG: vflip]
           Used to return the vflip flag.  If true, the image should be
           vertically flipped during rendering.  If `transpose' and
           `vflip' are both true, the transpose operation should be
           applied first.
         [ARG: hflip]
           Used to return the hflip flag.  If true, the image should be
           horizontally flipped during rendering.  If `transpose' and
           `hflip' are both true, the transpose operation should be
           applied first.
         [ARG: for_movie]
           If false, the geomtric transformations described by this
           function are resticted to those identified in the track header.
           Otherwise, the geometric transformations include any additional
           matrix transformation described in the global movie header.
      */
    KDU_AUX_EXPORT jp2_dimensions access_dimensions();
      /* [SYNOPSIS]
           Returns an object which may be used to access the information
           recorded in the JP2 Image Header (ihdr) and Bits Per Component
           (bpcc) boxes.
      */
    KDU_AUX_EXPORT jp2_resolution access_resolution();
      /* [SYNOPSIS]
           Returns an object which may be used to access aspect-ratio,
           capture and suggested display resolution information, for
           assistance in some rendering applications.
      */
    KDU_AUX_EXPORT jp2_palette access_palette();
      /* [SYNOPSIS]
           Returns an object which may be used to access any information
           recorded in the JP2 "Palette" box.  If there is none, the object's
           `jp2_palette::get_num_luts' function will return 0.  This
           information is fixed for all frames in the video track.
      */
    KDU_AUX_EXPORT jp2_channels access_channels();
      /* [SYNOPSIS]
           Returns an object which may be used to access information
           recorded in the JP2 "Component Mapping" and "Channel Definition"
           boxes.  The information from both boxes is merged into a uniform
           set of channel mapping rules, accessed through the returned
           `jp2_channels' object.  This information is fixed for all frames
           in the video track.
      */
    KDU_AUX_EXPORT jp2_colour access_colour();
      /* [SYNOPSIS]
           Returns an object which may be used to access information
           recorded in the JP2 "Color" box, which indicates the interpretation
           of colour image data for rendering purposes.  The returned
           `jp2_colour' object also provides convenient colour transformation
           functions, to convert data which uses a custom ICC profile into
           one of the standard rendering spaces.  The colour information
           is fixed for all frames in the video track.
      */
    KDU_AUX_EXPORT int get_stream_idx(int field_idx);
      /* [SYNOPSIS]
           Returns the unique index of the codestream which would be
           opened by a call to `open_stream'.  The codestreams which
           make up a Motion JPEG2000 file are assigned unique indices
           starting from 0 by assigning the first N1 indices to the first
           track, the next N2 indices to the second track, and so forth,
           where N1 is the number of frames belonging to the track,
           multiplied by 1 (progressive) or 2 (interlaced) depending on
           the number of fields per frame for the track.
           [//]
           The function returns -1 if a unique codestream index cannot
           be assigned -- e.g., because the relevant frame or field does
           not exist.
           [//]
           If the ultimate data source is a dynamic cache, this function
           can also return -1 if insufficient information is currently
           available to deduce the absolute codestream index of
           the first codestream in the track.  However, this situation will
           certainly change after the first successful call to `open_stream'
           or `open_image'.
           [//]
           Note that the returned codestream index uniquely
           identifies the track, frame and field -- see
           `mj2_source::find_stream'.  However, it is
           possible that multiple tracks or frames share a single
           codestream.  In this case, the single resource will have
           different codestream indices, depending on the track and frame
           which use it.
      */
    KDU_AUX_EXPORT virtual jp2_input_box *access_image_box();
      /* [SYNOPSIS]
           Returns NULL unless an image is open (see `open_image').  If so,
           the function returns a pointer to the internal `jp2_input_box'
           object which represents the codestream for the current image.
      */
    KDU_AUX_EXPORT virtual int
      open_stream(int field_idx, jp2_input_box *input_box,
                  kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This function is provided to allow you to access multiple
           codestreams simultaneously.  Rather than opening the object's
           own internal `jp2_input_box' object, the function opens the
           supplied `input_box' at the start of the relevant codestream.
           You need not close this box before invoking the `seek_to_frame'
           function to advance to another frame.  You can open as many
           codestreams as you like in this way.  However, you may not invoke
           the present function while the internal `jp2_input_box' is
           open -- i.e., between calls to `open_image' and `close_image'.
           [//]
           The frame whose codestream is opened by this function is the
           one which would otherwise be used by the next call to `open_image'.
           However, neither the field nor the frame index are advanced by
           calling this function.  In order to open a different frame, you
           will generally use the `seek_to_frame' function first.  The
           particular field to be opened within the frame is identified by
           the `field_idx' argument, which may hold 0 or 1.  The
           interpretation of this argument is unaffected by any calls to
           `set_field_mode'.
         [RETURNS]
           The frame index associated with the opened codestream box,
           or -1 if the requested field does not exist or if the frame
           which would be accessed by the next call to `open_image' does
           not exist.
         [ARG: field_idx]
           0 for the first field in the frame; 1 for the second field in
           the frame.  Other values will result in a return value of -1.
           A value of 1 will also produce a return value of -1 if the
           video is not interlaced.
         [ARG: input_box]
           Pointer to a box which is not currently open.  Box is open upon
           return unless the function's return value is negative.
         [ARG: env]
           This argument is provided for multi-threading environments.  Since
           this particular function provides you with the opportunity to
           access multiple video frames concurrently, it is possible that
           I/O conflicts will occur where the frames are used by multiple
           threads.  Virtually all such conflicts are blocked by supplying
           a `kdu_thread_env' pointer to the relevant `kdu_codestream',
           `kdu_multi_synthesis' or other parsing, reading and rendering
           objects offered by Kakadu.  However, the simple action of opening
           the frame stream is not thread safe unless you also supply the
           `kdu_thread_env' object to this function here.
      */
    KDU_AUX_EXPORT bool
      can_open_stream(int field_idx, bool need_main_header=true);
      /* [SYNOPSIS]
           This function returns true if a call to `open_stream' with the
           same `field_idx' argument would succeed and any required main
           codestream header can be read.  The function is provided for
           applications which may need to deal with caching data sources.
           Specifically, if the ultimate `jp2_family_src' object from which
           data is being sourced is a dynamic cache, it is possible that
           insufficient information currently exists to locate the codestream
           required for the requested field.  It is also possible that the
           codestream can be referenced, but its main header is not yet
           fully available in the cache.  If `need_main_header' is false, the
           function returns true so long as the codestream can be accessed,
           regardless of whether its main header is fully available yet.
           [//]
           For non-cacheing data sources, the function will always return true
           unless the requested field does not exist.
      */
  // --------------------------------------------------------------------------
  public: // Overrides for functions declared in `kdu_compressed_video_source'
    KDU_AUX_EXPORT virtual kdu_uint32 get_timescale();
      /* [SYNOPSIS]
           Gets the number of ticks per second, which defines the time scale
           used to describe frame periods.  See `get_frame_period'.
      */
    KDU_AUX_EXPORT virtual kdu_field_order get_field_order();
      /* [SYNOPSIS]
           See `kdu_compressed_video_source::get_field_order'.  Interlaced
           video is supported by the Motion JPEG2000 file format.
         [RETURNS]
           One of KDU_FIELDS_NONE, KDU_FIELDS_TOP_FIRST or
           KDU_FIELDS_TOP_SECOND.
      */
    KDU_AUX_EXPORT virtual void set_field_mode(int which);
      /* [SYNOPSIS]
           See `kdu_compressed_video_source::set_field_mode'.  Interlaced
           video is supported by the Motion JPEG2000 file format.
      */
    KDU_AUX_EXPORT virtual int get_num_frames();
      /* [SYNOPSIS]
           Returns the total number of frames in the track. In the case of
           an interlaced track (`get_field_order' returns one of
           KDU_FIELDS_TOP_FIRST or KDU_FIELDS_TOP_SECOND), there are two
           images (fields) in every frame; this function returns the number
           of frames, not the number of images.
      */
    KDU_AUX_EXPORT virtual bool seek_to_frame(int frame_idx);
      /* [SYNOPSIS]
           Call this function to set the index (starts from 0) of the frame
           to be opened by the next call to `open_image'.  The function
           should behave correctly even if an image is already open.  You may
           use `get_num_frames' to determine the number of frames which
           are available.  If the video is interlaced and the field mode
           (see `set_field_mode') is 2, the next call to `open_image'
           will open the first field of the indicated frame.
         [RETURNS]
           False if the frame does not exist.
      */
    KDU_AUX_EXPORT virtual kdu_long get_duration();
      /* [SYNOPSIS]
           Returns the total duration of the video track, measured in
           the time scale (ticks per second) identified by the `get_timescale'
           function.
      */
    KDU_AUX_EXPORT virtual int time_to_frame(kdu_long time_instant);
      /* [SYNOPSIS]
           Use this function to determine the frame whose period includes
           the supplied `time_instant', measured in the time scale (ticks
           per second) identified  by the `get_timescale' function.
           If `time_instant' exceeds the duration of the video track, the
           function returns the index of the last frame in the video.
           If `time_instant' refers to a time prior to the start of
           the video sequence, the function returns 0.
      */
    KDU_AUX_EXPORT virtual kdu_long get_frame_instant();
      /* [SYNOPSIS]
           See `kdu_compressed_video_source::get_frame_instant'.
      */
    KDU_AUX_EXPORT virtual int open_image();
      /* [SYNOPSIS]
           See `kdu_compressed_video_source::open_image' for details.  Note,
           however, that an error message will be generated (through
           `kdu_error') if you attempt to open more than one image
           simultaneously from the same Motion JPEG2000 source.  This
           means that only one track can have an open image at any given
           time.  If you need to open multiple codestreams at once, use
           the `open_stream' member function.
      */
    KDU_AUX_EXPORT virtual void close_image();
      /* [SYNOPSIS]
           Each successful call to `open_image' must be bracketed by a call to
           `close_image'.  Does nothing if no image (field or frame) is
           currently open.
      */
    KDU_AUX_EXPORT virtual kdu_long get_frame_period();
      /* [SYNOPSIS]
           Returns the number of ticks associated with the frame to which
           the currently open image belongs, or would belong if `open_image'
           has still to be called.  If no image is open and the the call
           to `open_image' would fail, the function returns 0.  If this
           happens, you can rectify the problem by seeking to a valid
           frame before calling the function again.  The number of ticks per
           second is identified by the `get_timescale' function.  If the
           video is interlaced, there are two images (fields) in each frame
           period.
      */
  // --------------------------------------------------------------------------
  public: // Overrides for functions declared in `kdu_compressed_source'
    KDU_AUX_EXPORT virtual int read(kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           Used by `kdu_codestream' to read compressed data from an open image.
           An error is generated through `kdu_error' if no image is open
           when this function is invoked.  The function will not read past
           the data associated with the currently open image.
           See `kdu_compressed_source::read' for an explanation.
      */
    KDU_AUX_EXPORT virtual int get_capabilities();
      /* [SYNOPSIS]
           For an introduction to source capabilities, consult the comments
           appearing with the declaration of `kdu_compressed_source'.
           Although capabilities can (in theory) change on a per-image basis,
           it is hard to imagine an application where this could reasonably
           occur.
      */
    KDU_AUX_EXPORT virtual bool seek(kdu_long offset);
      /* [SNOPSIS]
           See the description of `kdu_compressed_source::seek', but note that
           the `seek' functionality implemented here deliberately prevents
           seeking beyond the bounds of the currently open image.  An
           error message is generated (through `kdu_error') if no image is
           open when this function is invoked.  Note also that the default
           seek origin is the start of the JPEG2000 code-stream associated
           with the currently open image.
      */
    KDU_AUX_EXPORT virtual kdu_long get_pos();
      /* [SYNOPSIS]
           See `kdu_compressed_source::get_pos' for an explanation.
           As with `seek', there must be an open image and
           positions are expressed relative to the start of the
           code-stream associated with that image.
      */
  // --------------------------------------------------------------------------
  private: // Data
    friend class mj_video_track;
    mj_video_track *state;
  };

/*****************************************************************************/
/*                                mj2_source                                 */
/*****************************************************************************/

#define MJ2_TRACK_NON_EXISTENT ((int) 0)    // See `mj2_source::get_track_type'
#define MJ2_TRACK_MAY_EXIST    ((int) -1)   // See `mj2_source::get_track_type'
#define MJ2_TRACK_IS_VIDEO     ((int) 1)    // See `mj2_source::get_track_type'
#define MJ2_TRACK_IS_OTHER     ((int) 1000) // See `mj2_source::get_track_type'

class mj2_source {
  /* [BIND: reference]
     [SYNOPSIS]
       Supports reading and random access into MJ2 (Motion JPEG2000) files.
       The `mj2_source' object is able to manage one or more video tracks,
       each of which is associated with an object of the `mj2_video_source'
       class, which is derived from `kdu_compressed_source'.  These
       video track objects form legitimate compressed data sources which
       may be passed to `kdu_codestream::create'.
       [//]
       Objects of this class are merely interfaces to an internal
       implementation object.  Copying an `mj2_source' object simply
       duplicates the reference to this internal object.  For this reason,
       `mj2_source' has no meaningful destructor.  To destroy the internal
       object you must invoke the explicit `close' function.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle functions
    mj2_source() { state = NULL; }
      /* [SYNOPSIS]
           Constructs an empty interface -- one whose `exists' function
           returns false.  Use `open' to open a new MJ2 file.
      */
    mj2_source(mj_movie *state) { this->state = state; }
      /* [SYNOPSIS]
           Applications have no meaningful way to invoke this constructor
           except with a NULL argument.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns true if there is an open file associated with the object.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if there is an open file
           associated with the object.
      */
    KDU_AUX_EXPORT int
      open(jp2_family_src *src, bool return_if_incompatible=false);
      /* [SYNOPSIS]
           This function opens an MJ2 compatible data source.  If the data
           source does not advertise compatibility with the Motion JPEG2000
           standard, an error will be generated through `kdu_error', unless
           `return_if_incompatible' is true.
           [//]
           It is illegal to invoke this function on an object which has
           previously been opened, but has not yet been closed.
           [//]
           It is important to realize that reading from a Motion JPEG2000
           source generally requires seeking, so non-seekable `jp2_family_src'
           objects should not be used.
           [//]
           The current implementation of this object does not support dynamic
           caching data sources (those whose `jp2_family_src::uses_cache'
           function returns true).  However, all of the interface functions
           are designed to support sources for which the data in a cache might
           not yet be available.  This allows us to upgrade the internal
           implementation later without affecting the design of applications
           which use it.
         [RETURNS]
           Three possible values, as follows:
           [>>] 0 if insufficient information is available from the `src'
                object to complete the opening operation.  In practice,
                this can occur only if the `src' object is fueled by a
                dynamic cache (a `kdu_cache' object).
           [>>] 1 if successful.
           [>>] -1 if the data source does not advertise compatibility with
                the Motion JPEG2000 standard.  This value will not
                be returned unless `return_if_incompatible' is true.  If it
                is returned, the object will be left in the closed state.
         [ARG: src]
           A previously opened `jp2_family_src' object.  Note that you
           must explicitly close or destroy that object, once you are done
           using it.  The present object's `close' function will not do
           this for you.
         [ARG: return_if_incompatible]
           If false, an error will be generated through `kdu_error' if the
           data source does not advertise compatibility with the Motion JPEG2000
           specification.  If true, incompatibility will not generate an
           error, but the function will return -1, leaving the application
           free to pass the `src' object to another file format reader.
      */
    KDU_AUX_EXPORT void close();
      /* [SYNOPSIS]
           Destroys the internal object and resets the state of the interface
           so that `exists' returns false.  It is safe to close an object
           which was never opened.
      */
    KDU_AUX_EXPORT jp2_family_src *get_ultimate_src();
      /* [SYNOPSIS]
           Returns a pointer to the `src' object which was passed to `open',
           or NULL if the object is not currently open.
      */
  // --------------------------------------------------------------------------
  public: // Global properties
    KDU_AUX_EXPORT kdu_dims get_movie_dims();
      /* [SYNOPSIS]
           Returns the location and dimensions of the smallest
           rectangular region which contains all video tracks when
           rendered to the movie surface according to the transformations
           described by their `mj2_video_source::get_cardinal_geometry'
           functions.
      */
  // --------------------------------------------------------------------------
  public: // Track management functions
    KDU_AUX_EXPORT kdu_uint32 get_next_track(kdu_uint32 prev_track_idx);
      /* [SYNOPSIS]
           Returns the index of the first track (any kind of track) whose
           index exceeds `prev_track_idx', or 0 if there is none.  Since
           the first valid track index is 1, supplying 0 for the
           `prev_track_idx' argument ensures that the first valid track
           index will be returned.  To determine whether the track is a
           video track or not, you may call `access_video_track' and see
           if the returned pointer is NULL or not.
      */
    KDU_AUX_EXPORT int get_track_type(kdu_uint32 track_idx);
      /* [SYNOPSIS]
           This function may be used to determine the existence and nature
           of a Motion JPEG2000 track, before actually attempting to access
           it.  The function is also useful when working with cacheing data
           sources, since it allows the caller to determine whether or not
           the track might still be encountered if more data were added to
           the cache.
         [RETURNS]
           The following return types are defined:
           [>>] `MJ2_TRACK_IS_VIDEO'
                -- in this case, `access_video_track' will succeed.
           [>>] `MJ2_TRACK_IS_OTHER'
                -- in this case, the track exists but does not have one of the
                   defined types.  The set of defined types may increase as the
                   implementation of this object becomes more comprehensive
                   in the future.
           [>>] `MJ2_TRACK_NON_EXISTENT'
                -- this return value means that the track definitely does not
                   exist.
           [>>] `MJ2_TRACK_MAY_EXIST'
                -- this return value means that the track has not yet been
                   encountered, but not enough data is yet available from a
                   cacheing data source to determine the identity of all tracks.
      */
    KDU_AUX_EXPORT mj2_video_source *
      access_video_track(kdu_uint32 track_idx);
      /* [SYNOPSIS]
           Returns a pointer to an internal video track resource, from
           which compressed data and rendering attributes can be recovered.
           The function returns NULL if the supplied track index refers to
           a non-existent track, or one which does not contain video
           information.
      */
    KDU_AUX_EXPORT bool
      find_stream(int stream_idx, kdu_uint32 &track_idx,
                  int &frame_idx, int &field_idx);
      /* [SYNOPSIS]
           Finds the track containing the codestream whose absolute index
           is supplied by `stream_idx', returning the corresponding frame and
           field indices within that track via the `frame_idx' and
           `field_idx' arguments.
           [//]
           If the function is unable to determine whether the stream exists,
           the function returns false.  This may happen if insufficient
           information is available from a dynamic cache.  Otherwise, the
           function returns true, even if the stream is invalid.  An invalid
           stream can be identified by the fact that the returned `track_idx'
           value will be 0.
         [RETURNS]
           False if insufficient information exists to determine whether or
           not the stream exists.  This condition may change in the future
           if more data is added to a dynamic cache.  If the function returns
           true, the values of `track_idx', `frame_idx' and `field_idx' will be
           set appropriately.
         [ARG: track_idx]
           Used to return the index of the track to which the stream belongs.
           If the codestream index is invalid (i.e., it does not belong to any
           track), `track_idx' is set to 0.
         [ARG: stream_idx]
           Absolute codestream index -- see `mj2_video_source::get_stream_idx'
           for an explanation of this.
         [ARG: frame_idx]
           Set to the index (starting from 0) of the frame to which the
           codestream belongs within its track -- unaffected if the
           function returns 0.
         [ARG: field_idx]
           Set to the index (0 or 1) of the field to which the codestream
           belongs -- unaffected if the function returns 0.
      */
    KDU_AUX_EXPORT bool count_codestreams(int &count);
      /* [SYNOPSIS]
           If the number of codestreams in the MJ2 data source is already
           known, this function returns true, writing that number into `count'.
           Otherwise, the function attempts to parse further into the data
           source in order to count the number of codestreams which are
           available.  If it is able to deduce the number of codestreams,
           it again returns true, writing the number of codestreams into `count'.
           [//]
           If the function is unable to parse to a point at which the number
           of codestreams can be known, it returns false, writing the
           number of codestreams encountered up to that point into the
           `count' argument.
           [//]
           Each video track whose header can be parsed contributes either
           1 or 2 codestreams for each frame, depending on whether it is
           progressive or interlaced (interlaced frames each have two
           codestreams, one per field).
         [RETURNS]
           False if it is possible that more codestreams remain in the
           data source, but the function is unable to parse any further
           into the source at this point.  This generally means that the
           underlying `jp2_family_src' object is fueled by a dynamic cache
           (i.e., a `kdu_cache' object).
      */
  // --------------------------------------------------------------------------
  private: // Data
    mj_movie *state;
  };

/*****************************************************************************/
/*                             mj2_video_target                              */
/*****************************************************************************/

class mj2_video_target : public kdu_compressed_video_target {
  /* [BIND: reference]
     [SYNOPSIS]
       Manages access to a single video track within an MJ2 presentation.
       Use the `mj2_target::add_video_track' function to create an
       instance of this class, which may then be used as a compressed
       data target (on a frame-by-frame basis) in calls to
       `kdu_codestream::create'.
  */
  // --------------------------------------------------------------------------
  public: // Functions specific to Motion JPEG2000 video tracks
    KDU_AUX_EXPORT kdu_uint32 get_track_idx();
      /* [SYNOPSIS]
           Returns the track index associated with the current video
           track.  Valid track indices are always non-zero.
      */
    KDU_AUX_EXPORT void set_compositing_order(kdu_int16 layer_idx);
      /* [SYNOPSIS]
           Sets the front-to-back position of the current video track.  Smaller
           values are further forward (closer to the viewer) than larger
           values.  By default, all tracks have a compositing layer index of 0,
           with negative values used to identify tracks closer to the
           foreground and positive values used to identify tracks further to
           the background.
      */
    KDU_AUX_EXPORT void
      set_graphics_mode(kdu_int16 graphics_mode, kdu_int16 op_red=0,
                        kdu_int16 op_green=0, kdu_int16 op_blue=0);
      /* [SYNOPSIS]
           Indicates the raster operations which should be used to combine
           this track with others on the display.  Tracks are added in
           the compositing order established by `set_compositing_order'.
           By default, compositing involves a direct copy; however, this
           function may be used to set other blending strategies.
         [ARG: graphics_mode]
           May take any of the following values:
           [>>] `MJ2_GRAPHICS_COPY' -> copy.  Note that in this case, any
                opacity channels specified via the `jp2_channels' object
                returned via `access_channels' will be ignored.
           [>>] `MJ2_GRAPHICS_BLUE_SCREEN' -> blue screen the image using
                the supplied op-colour.  As above, any opacity channels
                specified via the `jp2_channels' object returned via
                `access_channels' will be ignored.
           [>>] `MJ2_GRAPHICS_ALPHA' -> alpha blend with existing contents.
                The alpha channel must be specified by invoking the
                `jp2_channels::set_opacity_mapping' function on the object
                returned via `access_channels'.
           [>>] `MJ2_GRAPHICS_PREMULT_ALPHA' -> alpha blend, where compressed
                data has already been blended with black.  The premultiplied
                alpha blending channel must be specified by invoking the
                `jp2_channels::set_premult_mapping' function on the object
                returned via `access_channels'.
           [>>] `MJ2_GRAPHICS_COMPONENT_ALPHA' -> alpha blend for all
                cases other than that in which a single alpha channel is
                applied to all intensity channels uniformly.  This includes
                the case in which multiple alpha channels are used, or a
                single alpha channel is applied to only some of the intensity
                channels.  The `jp2_channels' object returned by
                `access_channels' provides details of the actual alpha
                blending which is required.
      */
    KDU_AUX_EXPORT jp2_colour access_colour();
      /* [SYNOPSIS]
           Provides an interface which may be used for setting up the
           information in the JP2 "Color" box.  The colour information
           is fixed for all frames in the video track.
           [//]
           You ARE REQUIRED to perform this initialization.
      */
    KDU_AUX_EXPORT jp2_palette access_palette();
      /* [SYNOPSIS]
           Provides an interface which may be used for setting up a
           JP2 "Palette" box.  Palette information
           is fixed for all frames in the video track.
           [//]
           It is NOT NECESSARY to access or initialize palette
           information; the default behaviour is to not use a palette.
      */
    KDU_AUX_EXPORT jp2_channels access_channels();
      /* [SYNOPSIS]
           Provides an interface which may be used to specify the
           relationship between code-stream image components and colour
           reproduction channels (colour intensity channels, opacity
           channels, and pre-multiplied opacity channels).  This information
           is used to construct appropriate JP2 "Component Mapping" and
           "Channel Definition" boxes.  The information
           is fixed for all frames in the video track.
           [//]
           It is NOT NECESSARY to access or initialize the `jp2_channels'
           object directly; the default behaviour is to assign the colour
           intensity channels to the initial code-stream image components.
           That is, the first code-stream component is assigned to the
           luminance or red intensity, the second is assigned to the
           green intensity, and the third is assigned to the blue intensity.
           The mandatory information configured in the `jp2_colour' object
           is used to determine whether a luminance or RGB colour space is
           involved.
      */
    KDU_AUX_EXPORT jp2_resolution access_resolution();
      /* [SYNOPSIS]
           Provides an interface which may be used to specify the physical
           resolution and aspect ratio for the frame data in this video
           track.  You are not required to provide this information.
      */
    KDU_AUX_EXPORT void set_timescale(kdu_uint32 ticks_per_second);
      /* [SYNOPSIS]
           Sets the number of ticks per second which defines the time scale
           used to describe frame periods for the track.  As an example, the
           most natural time scale for NTSC video involves 30000 ticks per
           second, with a frame-to-frame period of 1001 ticks.
           [//]
           This function must be called prior to the first call to
           `open_image' and it may not be called again, thereafter.
      */
    KDU_AUX_EXPORT void set_field_order(kdu_field_order order);
      /* [SYNOPSIS]
           For interlaced video tracks, call this function to indicate
           the order of the two fields.  If `order' is KDU_FIELDS_NONE,
           each frame has a progressive scan, with only one field (this is
           the default).  Otherwise, `order' must be one of
           KDU_FIELDS_TOP_FIRST or KDU_FIELDS_TOP_SECOND.  In the former
           case, the first field in temporal sequence also holds the first
           line in its frame.  Otherwise, the second field of each frame
           holds the top line of the frame.
           [//]
           It is illegal to call this function after the first call to
           `open_image'.
      */
    KDU_AUX_EXPORT void set_max_frames_per_chunk(kdu_uint32 max_frames);
      /* [SYNOPSIS]
           Sets an upper bound for the number of frames which can be stored
           in a single chunk within the Motion JPEG2000 file.  The internal
           machinery may well select a smaller chunk size to satisfy
           constraints on the maximum duration of a chunk, and any other
           internal resource constraints.
           [//]
           The chunk size determines the granularity with which tracks are
           interleaved, which affects the efficiency or memory requirements
           associated with simultaneous track playback.  By default, the
           maximum chunk size is set to 1 frame.
           [//]
           Note that interlaced frames each consist of two fields, in which
           case the maximum number of fields per chunk is twice the value of
           `max_frames'.
      */
    KDU_AUX_EXPORT void set_frame_period(kdu_long num_ticks);
      /* [SYNOPSIS]
           Sets the number of ticks of the current time scale (as set by
           `set_timescale') between frames.  The frame period may be changed
           at any point, but an initial value must be set before the first
           call to `open_image'.  If the video is interlaced, the frame
           period refers to the complete interlaced frame, with each image
           (field) occupying half of the total frame period.  If the frame
           period is changed part way into a frame, the value installed when
           the frame is first commenced is the one used for that frame.
      */
  // --------------------------------------------------------------------------
  public: // Overrides for functions declared in `kdu_compressed_video_target'
    KDU_AUX_EXPORT virtual void open_image();
      /* [SYNOPSIS]
           Call this function to initiate the generation of a new video frame
           or field.  In non-interlaced (progressive) mode, each frame consists
           of a single image.  However, if `set_field_order' has been
           used to configure the video track for interlacing, there are two
           images (fields) per frame.
           [//]
           After calling this function, the present object may be passed into
           `kdu_codestream::create' to generate the JPEG2000 code-stream
           representing the open video image.  Once the code-stream has been
           fully generated (usually performed by `kdu_codestream::flush'),
           the image must be closed using `close_image'.  A new video image
           can then be opened.
      */
    KDU_AUX_EXPORT virtual void close_image(kdu_codestream codestream);
      /* [SYNOPSIS]
           Each call to `open_image' must be bracketed by a call to
           `close_image'.  The caller must supply a non-empty `codestream'
           interface, which was used to generate the compressed data for
           the image (field or frame) just being closed.  Its member functions
           will be used to determine dimensional parameters for internal
           initialization of some important Motion JPEG2000 box fields; it may
           also be used to perform various consistency checks on the fields
           belonging to the same track.
      */
  // --------------------------------------------------------------------------
  public: // Overrides for functions declared in `kdu_compressed_target'
    KDU_AUX_EXPORT virtual bool write(const kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           Used by `kdu_codestream' to write compressed data to an open field.
           An error is generated through `kdu_error' if no field is open
           when this function is invoked.  See `kdu_compressed_target::write'
           for more information.
      */
  // --------------------------------------------------------------------------
  private: // Data
    friend class mj_video_track;
    mj_video_track *state;
  };

/*****************************************************************************/
/*                                mj2_target                                 */
/*****************************************************************************/

class mj2_target {
  /* [BIND: interface]
     [SYNOPSIS]
       Supports the generation of MJ2 (Motion JPEG2000) files.  The
       `mj2_target' object is able to manage one or more video tracks,
       each of which is associated with an object of the `mj2_video_target'
       class, which is derived from `kdu_compressed_target'.  These
       video track objects form legitimate compressed data targets to
       pass to `kdu_codestream::create'.
       [//]
       Objects of this class are merely interfaces to an internal
       implementation object.  Copying an `mj2_target' object simply
       duplicates the reference to this internal object.  For this reason,
       `mj2_target' has no meaningful destructor.  To destroy the internal
       object you must invoke the explicit `close' function.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle functions
    mj2_target() { state = NULL; }
      /* [SYNOPSIS]
           Constructs an empty interface -- one whose `exists' function
           returns false.  Use `open' to start a new MJ2 file.
      */
    mj2_target(mj_movie *state) { this->state = state; }
      /* [SYNOPSIS]
           Applications have no meaningful way of invoking this constructor
           except with a NULL argument.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns true if there is an open file associated with the object.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if there is an open file
           associated with the object.
      */
    KDU_AUX_EXPORT void open(jp2_family_tgt *tgt);
      /* [SYNOPSIS]
           Opens the object to write its output to the indicated `tgt' object.
      */
    KDU_AUX_EXPORT void close();
      /* [SYNOPSIS]
           Flushes all active tracks and appends the Motion JPEG2000 meta-data
           (index tables, rendering information, timing information, etc.) to
           the end of the file.  Destroys the internal object and resets the
           state of the interface so that `exists' returns false.  It is safe
           to close an object which was never opened.
      */
  // --------------------------------------------------------------------------
  public: // Track management functions
    KDU_AUX_EXPORT mj2_video_target *add_video_track();
      /* [SYNOPSIS]
           Returns a pointer to an internal video track resource, to
           which compressed data and rendering attributes can be written.
           Each call to this function creates a new video track, with a
           distinct track index.  You may determine the track index using
           `mj2_video_target::get_track_idx'.
           [//]
           At a minimum you will need to configure the colour space
           information (see `mj2_video_target::access_colour') and the
           frame-to-frame period (see `mj2_video_target::set_frame_period')
           for the new video track, before writing compressed frames
           (or fields), with the aid of the `mj2_video_target::open_image'
           and `mj2_video_target::close_image' functions.
      */
  // --------------------------------------------------------------------------
  private: // Data
    mj_movie *state;
  };

#endif // MJ2_H
