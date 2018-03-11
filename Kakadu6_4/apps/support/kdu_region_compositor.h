/******************************************************************************/
// File: kdu_region_compositor.h [scope = APPS/SUPPORT]
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
   Builds upon the functionality offered by `kdu_region_decompressor' to offer
general compositing services for JPX/JP2 files, MJ2 files and raw codestreams.
This function wraps up even more of the operations required to construct a
highly sophisiticated and efficient browser for JPEG2000 files (local and
remotely served via JPIP), doing most of the real work of the "kdu_show"
application.  The implementation is entirely platform independent.
*******************************************************************************/
#ifndef KDU_REGION_COMPOSITOR_H
#define KDU_REGION_COMPOSITOR_H

#include <string.h>
#include "jpx.h"
#include "mj2.h"

// Objects declared here
class kdu_ilayer_ref;
class kdu_istream_ref;
class kdu_compositor_buf;
class kdu_overlay_params;
class kdu_region_compositor;

// Objects declared elsewhere
struct kdrc_queue;
class kdrc_overlay;
class kdrc_overlay;
class kdrc_stream;
class kdrc_layer;
class kdrc_refresh;
struct kdrc_overlay_expression;

#define KDU_COMPOSIT_DEFER_REGION ((int) 1)
#define KDU_COMPOSIT_DEFER_PROCESSING ((int) 2)

/******************************************************************************/
/*                               kdu_ilayer_ref                               */
/******************************************************************************/

class kdu_ilayer_ref {
  /* [BIND: copy]
     [SYNOPSIS]
       Instances of this class are used to robustly pass references to
       imagery layers within the `kdu_region_compositor'.  Only null
       instances of this class can be created by an application -- these
       are instances for which `is_null' returns true.  All non-trivial
       instances are created exclusively by the `kdu_region_compositor'
       object.
       [//]
       The term "imagery layer" (abbreviated "ilayer") refers roughly to
       a layer in the composited image managed by `kdu_region_compositor'.
       Successive layers are painted on top of each other (often with
       alpha blending).  We no longer use the term "compositing layer" to
       refer to these layers because that term has a special meaning within
       the JPX file format, yet the same compositing layer can actually be
       used to construct multiple imagery layers within the same composition
       (composited, for example at different locations, or with different
       scaling factors or orientations).
       [//]
       In practice, the current implementation uses an integer
       of type `kdu_long' assigning these integers in a manner which
       ensures uniqueness amongst all istreams managed by the
       `kdu_region_compositor' and minimizes the likelihood that an istream
       which has been destroyed will have the same reference value as
       another istream which has since been created.
  */
  public: // Member functions
    kdu_ilayer_ref() { ref = 0; }
    bool is_null() const { return (ref==0); }
    bool exists() const { return (ref!=0); }
    bool operator!() const { return is_null(); }
    bool operator==(const kdu_ilayer_ref &rhs) const { return (rhs.ref==ref); }
    bool operator!=(const kdu_ilayer_ref &rhs) const { return (rhs.ref!=ref); }
  private: // Data
    friend class kdu_region_compositor;
    kdu_long ref;
  };

/******************************************************************************/
/*                               kdu_istream_ref                               */
/******************************************************************************/

class kdu_istream_ref {
  /* [BIND: copy]
     [SYNOPSIS]
       Instances of this class are used to robustly pass references to
       imagery streams within the `kdu_region_compositor'.  Only null
       instances of this class can be created by an application -- these
       are instances for which `is_null' returns true.  All non-trivial
       instances are created exclusively by the `kdu_region_compositor'
       object.
       [//]
       The term "imagery stream" (abbreviated "istream") refers to a
       single codestream within the context of a specific imagery layer
       (ilayer).  Each istream is associated with a single ilayer and a
       single JPEG2000 codestream; however, each ilayer may involve multiple
       istreams (since different codestreams can be used to render its various
       colour channels); and any given JPEG2000 codestream may provide colour
       channel samples for more than one imagery layer within a composition. 
       [//]
       In practice, the current implementation uses an integer
       of type `kdu_long' assigning these integers in a manner which
       ensures uniqueness amongst all istreams managed by the
       `kdu_region_compositor' and minimizes the likelihood that an istream
       which has been destroyed will have the same reference value as
       another istream which has since been created.
  */
  public: // Member functions
    kdu_istream_ref() { ref = 0; }
    bool is_null() const { return (ref==0); }
    bool exists() const { return (ref!=0); }
    bool operator!() const { return is_null(); }
    bool operator==(const kdu_istream_ref &rhs) const { return (rhs.ref==ref); }
    bool operator!=(const kdu_istream_ref &rhs) const { return (rhs.ref!=ref); }
  private: // Data
    friend class kdu_region_compositor;
    kdu_long ref;
  };

/******************************************************************************/
/*                             kdu_compositor_buf                             */
/******************************************************************************/

class kdu_compositor_buf {
  /* [BIND: reference]
     [SYNOPSIS]
       This object is used to allocate and manage image buffers within
       `kdu_region_compositor'.  The purpose of managing buffers through
       a dynamic object is to allow advanced types of buffering in which
       the address of the buffer may change each time it is used.  This
       can happen, for example, if buffers are allocated so as to represent
       resources on a display frame buffer.
  */
  public: // Member functions
    kdu_compositor_buf()
      {
        internal=locked_for_read=locked_for_write=false;
        read_access_allowed=true; buf=NULL; float_buf=NULL; row_gap=0;
      }
    virtual ~kdu_compositor_buf()
      {
        if (buf != NULL) delete[] buf;
        if (float_buf != NULL) delete[] float_buf;
      }
      /* [SYNOPSIS]
           If you are providing your own buffers, based on a derived version
           of this class, beware that the internal `buf' and `float_buf'
           arrays will be passed to the C++ `delete' operator if non-NULL.
           If you have allocated these using anything other than `new', be
           sure to override the destructor and set them to NULL before
           exiting your version of the function.
      */
    KDU_AUX_EXPORT void init(kdu_uint32 *buf, int row_gap)
      {
        assert(this->float_buf == NULL);          
        this->buf = buf;  this->row_gap = row_gap;
      }
     /* [SYNOPSIS]
          Either this function, or `init_float' should be called immediately
          after construction, when no buffer has yet been installed.  This
          particular function is used to configure the `kdu_compositor_buf'
          to work with a 32-bit pixel representation, corresponding to 8
          bits per sample, in the order Alpha (most significant byte), Red,
          Green and Blue (least significant byte).
          [//]
          As a general rule, any other changes in the `buf' array should
          be handled by overriding `lock_buf' and/or `set_read_accessibility'.
          [//]
          You can, however, change the internal buffer by calling this function
          again, at any point when a call to `kdu_region_compositor::process'
          is not in progress.  This is not recommended, but if you must do it
          for some reason, be sure not to mix floating-point and 32-bit
          buffers.
        [ARG: buf]
          [BIND: donate]
          This argument supplies the physical buffer which is to be used
          by this object.  The argument is marked as "BIND: donate" for the
          purpose of binding to other computer languages.  In C# and Visual
          Basic, the `buf' argument is of type `IntPtr' which might be
          obtained, for example, by a call to `Bitmap.LockBits'.  In Java,
          donated arrays are bound as type `long', which could potentially be
          used to represent an opaque pointer, assuming you have some other
          object (typically with a native interface) which is prepared to
          return a pointer to a valid block of memory.  Of course, incorrect
          use of this function could be dangerous.
        [ARG: row_gap]
          Indicates the gap between consecutive rows, measured in 4-byte
          pixels.
     */
    KDU_AUX_EXPORT void init_float(float *float_buf, int row_gap)
      {
        assert(this->buf == NULL);          
        this->float_buf = float_buf;  this->row_gap = row_gap;
      }
     /* [SYNOPSIS]
          Either this function, or `init' should be called immediately after
          construction, when no buffer has yet been installed.  This
          particular function is used to configure the `kdu_compositor_buf'
          to work with a floating-point pixel representation, in which each
          pixel consists of exactly 4 floating point samples, starting with
          Alpha, the Red, Green and finally Blue.
          [//]
          Note that floating-point samples are all assumed to have a nominal
          range of 0.0 to 1.0 -- inclusive.
          [//]
          In every other respect, this function is the same as the
          `init' function.
        [ARG: float_buf]
          [BIND: donate]
          This argument supplies the physical buffer which is to be used
          by this object.  The argument is marked as "BIND: donate" for the
          purpose of binding to other computer languages.  In C# and Visual
          Basic, the `float_buf' argument is of type `IntPtr'.  In Java,
          donated arrays are bound as type `long', which could potentially be
          used to represent an opaque pointer, assuming you have some other
          object (typically with a native interface) which is prepared to
          return a pointer to a valid block of memory.  Of course, incorrect
          use of this function could be dangerous.
        [ARG: row_gap]
          Indicates the gap between consecutive rows, measured in samples
          (not pixels).
      */  
    bool is_read_access_allowed()
      { return read_access_allowed; }
      /* [SYNOPSIS]
           This function is used internally by `kdu_region_compositor'
           in places where reading from a buffer is optional -- e.g.,
           to copy overlapping data from one region to another when the
           buffer surface is shifted.  If the function returns false,
           read-access is not available and so the optional activity will
           not be performed.
      */
    virtual bool set_read_accessibility(bool read_access_required)
      {
        read_access_allowed = true; // Default implementation always gives
        return true;    // read access.  Generally, you should do the same,
           // even if `read_access_required' is false, except in special
           // circumstances where the application knows that it is better to
           // allocate fast write-only memory (e.g., a display buffer).
      }
      /* [SYNOPSIS]
           This function may be used to alter the type of memory access
           which can be performed on the buffer.  If read access was not
           previously allowed, but is now required, memory resources may
           need to be allocated in a different way.
           [//]
           If the memory manager is unable to retain the contents of a
           buffer which was originally allocated as write-only, but must
           now be available for read-write access, it must return
           false.  This informs the caller that the buffer's contents must
           be marked as invalid so that they will be regenerated by
           subsequent calls to `kdu_region_compositor::process'.
         [RETURNS]
           False if the buffer's contents must be regenerated.  This
           only happens if `read_access_required' is true and the memory
           manager cannot preserve the contents of the originally allocated
           buffer.
      */
    kdu_uint32 *get_buf(int &row_gap, bool read_write)
      { /* [SYNOPSIS]
             This function is used internally by `kdu_region_compositor'
             to lock buffers before using them.  The persistence of an
             image buffer is not assumed between calls to
             `kdu_region_compositor::process', but the contents of the
             buffer are expected to be persistent.
             [//]
             This function returns NULL if the buffer does not have a 32-bit
             pixel organization.  In that case, the caller may invoke
             `get_float_buf' to see if it has a floating point representation.
        */
        if (buf == NULL) return NULL;
        if ((!locked_for_write) || (read_write && !locked_for_read))
          lock_buf(read_write);
        row_gap = this->row_gap; return buf;
      }
    float *get_float_buf(int &row_gap, bool read_write)
      { /* [SYNOPSIS]
             Same as `get_buf', except that this function is used to retrieve
             buffers which have a floating point organization, as instantiated
             by the `init_float' function.
           [ARG: row_gap]
             For this function, `row_gap' returns the separation between
             successive rows, measured in samples (not pixels) -- each sample
             is a single floating point number.
        */
        if (float_buf == NULL) return float_buf;
        if ((!locked_for_write) || (read_write && !locked_for_read))
          lock_buf(read_write);
        row_gap = this->row_gap; return float_buf;        
      }
    bool get_region(kdu_dims src_region, kdu_int32 tgt_buf[],
                    int tgt_offset=0, int tgt_row_gap=0)
      {
        if (buf == NULL) return false;
        src_region &= accessible_region;  assert(tgt_offset >= 0);
        if (tgt_row_gap == 0) tgt_row_gap = src_region.size.x;
        assert(this->buf != NULL);
        kdu_uint32 *src_buf = buf + src_region.pos.x+src_region.pos.y*row_gap;
        int m=src_region.size.y;
        size_t nbytes = ((size_t) src_region.size.x) << 2;
        for (; m > 0; m--, src_buf+=row_gap, tgt_buf+=tgt_row_gap)
          memcpy(tgt_buf,src_buf,nbytes);
        return true;
      }
      /* [SYNOPSIS]
           This function is provided mainly for binding to foreign languages
           (particularly Java), where it is not possible to access the array
           returned via `get_buf' and where no possibility exists to donate an
           array which is accessible to that language via the `init' function.
           In that case, the only option left is to copy the contents of the
           buffer into another array which is understood in the foreign
           language.
         [RETURNS]
           False if the internal buffer does not have a 32-bit pixel
           organization.  In that case, you should perhaps be calling
           the `get_float_region' function.
         [ARG: src_region]
           Region on the source buffer which you want to copy, measured
           relative to the start of the buffer -- i.e., not an absolute
           region.  If part of the specified region lies off the buffer,
           the missing portion will not be transferred.
         [ARG: tgt_buf]
           Buffer into which the results are to be written.  Note that the
           buffer type is defined as `kdu_int32' rather than `kdu_uint32'
           as a convenience for Java bindings, where no 32-bit
           unsigned representation exists.  Java image processing applications
           commonly work with 32-bit signed pixel representations.
         [ARG: tgt_offset]
           Number of pixels from the start of the supplied `tgt_buf' array
           at which you would like to start writing the data.  This must not
           be negative, of course.
         [ARG: tgt_row_gap]
           Separation between rows in the target buffer, measured in pixels.
           This cannot be negative.  If zero, the pixel corresponding to the
           `src_region' will be written contiguously into the buffer.
      */
    bool get_float_region(kdu_dims src_region, float tgt_buf[],
                          int tgt_offset=0, int tgt_row_gap=0)
      {
        if (float_buf == NULL) return false;
        src_region &= accessible_region;  assert(tgt_offset >= 0);
        if (tgt_row_gap == 0) tgt_row_gap = src_region.size.x<<2;
        assert(this->buf != NULL);
        float *src_buf = float_buf + src_region.pos.x+src_region.pos.y*row_gap;
        int m=src_region.size.y;
        size_t nbytes = ((size_t) src_region.size.x) << 4;
        for (; m > 0; m--, src_buf+=row_gap, tgt_buf+=tgt_row_gap)
          memcpy(tgt_buf,src_buf,nbytes);
        return true;
      }
      /* [SYNOPSIS]
           This function is provided mainly for binding to foreign languages
           (particularly Java), where it is not possible to access the array
           returned via `get_buf' and where no possibility exists to donate an
           array which is accessible to that language via the `init' function.
           In that case, the only option left is to copy the contents of the
           buffer into another array which is understood in the foreign
           language.
         [RETURNS]
           False if the internal buffer does not have a floating-point
           organization, with 4 samples per pixel.  In that case, you should
           perhaps be calling the `get_region' function.
         [ARG: src_region]
           Region on the source buffer which you want to copy, measured
           relative to the start of the buffer -- i.e., not an absolute
           region.  If part of the specified region lies off the buffer,
           the missing portion will not be transferred.
         [ARG: tgt_buf]
           Buffer into which the results are to be written.  Note that the
           buffer consists of 4 floating point samples per pixel, in the
           order Alpha, Red, Green and finally Blue.
         [ARG: tgt_offset]
           Number of pixels from the start of the supplied `tgt_buf' array
           at which you would like to start writing the data.  This must not
           be negative, of course.
         [ARG: tgt_row_gap]
           Separation between rows in the target buffer, measured in samples.
           This cannot be negative.  If zero, the pixel corresponding to the
           `src_region' will be written contiguously into the buffer.
      */
  protected:
    virtual void lock_buf(bool read_write)
      {
        assert(read_access_allowed || !read_write);
        locked_for_read = true; locked_for_write = read_write;
      }
      /* [SYNOPSIS]
           This function is called by `get_buf' if it fails to find
           the buffer available for use.  You may need to override this
           function to create buffers with special types of behaviour.
         [ARG: read_write]
           If true, the caller needs to be able to read from and write to
           the buffer.  Otherwise, only write access is required.
      */
  private: // Members used only inside `kdu_region_compositor'
    friend class kdu_region_compositor;
    kdu_dims accessible_region; // Region which can be accessed via the
             // `get_region' function -- this is used for access checking.
    bool internal; // True if `kdu_region_compositor::allocate_buffer' returned
             // NULL, meaning that the object was constructed by
             // `kdu_region_compositor::internal_allocate_buffer' and must be
             // deleted using `kdu_region_compositor::internal_delete_buffer'.
  protected: // Data
    bool read_access_allowed;
    bool locked_for_read, locked_for_write;
    kdu_uint32 *buf; // Used for most applications; samples in range [0, 255]
    float *float_buf; // Used for high precision buffers; samples in [0.0, 1.0]
    int row_gap;
  };

/******************************************************************************/
/*                             kdu_overlay_params                             */
/******************************************************************************/

class kdu_overlay_params {
   /* [BIND: reference]
      [SYNOPSIS]
        This structure provides additional information which may be used by
        the `kdu_region_compositor::paint_overlay' and
        `kdu_region_compositor::custom_paint_overlay' functions to paint
        overlay data for a metadata node.  The information provided here is
        all optional, except that any procedure which involves painting overlay
        content outside the region defined by a metadata node must respect the
        maximum border size returned by `get_max_painting_border'.
   */
  //----------------------------------------------------------------------------
  public: // Member functions
    int get_codestream_idx() { return codestream_idx; }
      /* [SYNOPSIS]
           Returns the identity of the codestream with which the overlay
           information is associated.
      */
    int get_compositing_layer_idx() { return compositing_layer_idx; }
      /* [SYNOPSIS]
           Returns the identity of the compositing layer with which the overlay
           is associated.  Each overlay is associated with exactly one
           compositing layer.
      */
    int get_max_painting_border() { return max_painting_border; }
      /* [SYNOPSIS]
           Returns a copy of the `max_painting_border' parameter supplied in the
           most recent call to `kdu_region_compositor::configure_overlays'.
           The interpretation of this parameter is the maximum amount by
           which the painted overlay content may exceed the boundaries of
           the mapped ROI `node', in each direction, as measured on the
           compositing reference grid.
           [//]
           Overlay data for any given node' may optionally be painted with
           a smaller border, if desired.  The border is used internally to
           size the `bounding_region' passed to
           `kdu_region_compositor::paint_overlay' and
           `kdu_region_compositor::custom_paint_overlay', and also to
           determine which overlay segments intersect with a node's region
           on an overlay buffer, so you should not paint with a larger border.
      */
    int get_num_aux_params() { return num_cur_aux_params; }
      /* [SYNOPSIS]
          Returns the number of auxiliary painting parameters supplied via
          the most recent call to `kdu_region_compositor::configure_overlays'.
          If you attempt to access more than this number of parameters using
          the `get_aux_param' function, you will be returned 0.
      */
    kdu_uint32 get_aux_param(int n)
      { return ((n>=0) && (n<num_cur_aux_params))?cur_aux_params[n]:0; }
      /* [SYNOPSIS]
           Returns a copy of the `n'th auxiliary painting parameter supplied via
           the most recent call to `kdu_region_compositor::configure_overlays'.
           Implementations of `kdu_region_compositor::paint_overlay' or
           `kdu_region_compositor::custom_paint_overlay' may use
           this parameter to adjust the appearance of the overlay image.
           These auxiliary painting parameters are used extensively by the
           default `kdu_region_compositor::paint_overlay' function; moreover,
           a custom implementation may call the default version of the
           function after temporarily replacing the auxiliary painting
           parameters with the aid of `push_aux_params'.
      */
    void push_aux_params(const kdu_uint32 *aux_params, int num_aux_params)
      {
        if (num_aux_params < 0) num_aux_params = 0; // Just in case
        if (num_aux_params > max_tmp_aux_params)
          { 
            max_tmp_aux_params = num_aux_params;
            if (tmp_aux_params!=NULL)
              { delete[] tmp_aux_params; tmp_aux_params = NULL; }
            tmp_aux_params = new kdu_uint32[max_tmp_aux_params];
          }
        num_cur_aux_params = num_aux_params; cur_aux_params=tmp_aux_params;
        memcpy(tmp_aux_params,aux_params,((size_t) num_aux_params)<<2);
      }
      /* [SYNOPSIS]
           You can use this function to temporarily replace the auxiliary
           painting parameters, as seen by the `get_num_aux_params' and
           `get_aux_params' functions.  This can be useful when calling
           the `kdu_region_compositor::paint_overlay' function from within
           an overridden `kdu_region_compositor::custom_paint_overlay'.  Note
           that `pop_aux_params' is automatically invoked after a call to
           `kdu_region_compositor::custom_paint_overlay' returns so that
           the information you record using this function is only temporary.
           [//]
           Despite the name of this function, there is no internal "stack" of
           auxiliary parameters.  Calling `push_aux_params' a second time
           will provide new replacement values for the auxiliary painting
           parameters, but a single call to `restore_aux_params' will remove
           all such replacements, restoring the original parameters passed
           to `kdu_region_compositor::configure_overlays'.
           [//]
           The `tmp_aux_params' array is copied internally, primarily to
           facilitate the use of this function with other non-native language
           bindings such as Java.
      */
    void restore_aux_params()
      { cur_aux_params=orig_aux_params;
        num_cur_aux_params=num_orig_aux_params; }
      /* [SYNOPSIS]
           Restores the original auxiliary painting parameters, as passed
           to `kdu_region_compositor::configure_overlays'.
      */
    KDU_AUX_EXPORT void configure_ring_points(int stride, int radius);
      /* [SYNOPSIS]
           If you intend to use `get_ring_points', call this function first
           with the `stride' associated with the buffer you intend to paint
           via your overlay painting routine and the `radius' with which you
           intend to draw rings.  The function configures internal lookup
           tables so as to minimize computation in the painting of border
           rings -- see `get_ring_points' for more info. Calls to this
           function need not necessarily do anything, since the object caches
           the ring lookup tables it generated in the past.
         [ARG: radius]
           This value should not exceed the value returned by
           `get_max_painting_border', as ring lookup tables are generated
           only for radii in the range 0 to the maximum painting border.
      */
    const int *get_ring_points(int min_y, int max_y, int &num_vals)
      {
        assert((max_y >= -cur_radius) && (min_y <= cur_radius));
        if (max_y >= cur_radius)
          num_vals = cur_ring_prefices[2*cur_radius];
        else
          num_vals = cur_ring_prefices[max_y + cur_radius];
        if (min_y <= -cur_radius)
          return cur_ring_points;
        int missing = cur_ring_prefices[min_y + cur_radius - 1];
        num_vals -= missing; return cur_ring_points+2*missing;
      }
      /* [SYNOPSIS]
           This function provides a somewhat user-friendly interface to
           internally lookup tables which are generated using the
           `configure_ring_points' function.  The purpose of the function
           is to facilitate the painting of smooth graduated borders (based
           on the other painting parameters) around regions of interest
           when drawing overlay shapes.
           [//]
           The function returns an array consisting of `num_vals' pairs of
           integers, where `num_vals' is set by the function.  Each pair of
           integers identifies the location of a single point on a
           semi-circular ring, having the radius supplied in the most recent
           call to `configure_ring_points'.  The collection of
           points identified by this function for rings of radius R=1 through
           T are guaranteed to fill in the entire right half of a circular
           region of radius T (with the exception of the centre point itself).
           [>>] The first integer in each pair holds the horizontal
                displacement of the ring point in question, relative to the
                centre of the circle (the displacement is always non-negative).
           [>>] The second integer in each pair holds the vertical displacement
                of the ring point in question, relative to the centre of the
                circle, multiplied by the `stride' value supplied to the
                `configure_ring_points' function.  Vertical displacements
                can be either positive or negative, since the ring comes from
                the full half circle formed by removing only those points to
                the left of the centre.
         [ARG: min_y]
           The `min_y' and `max_y' arguments identify inclusive lower and
           upper bounds on the ring points which are desired.  These values
           define an interval [`min_y',`max_y'].  The function assumes
           that `min_y' is no larger than R and `max_y' is no smaller than
           -R, where R is the radius supplied in the most recent call to
           `configure_ring_points'.
         [ARG: max_y]
           See `min_y'.
      */
    KDU_AUX_EXPORT jpx_roi *
      map_jpx_regions(const jpx_roi *regions, int num_regions,
                      kdu_coords image_offset, kdu_coords subsampling,
                      bool transpose, bool vflip, bool hflip,
                      kdu_coords expansion_numerator,
                      kdu_coords expansion_denominator,
                      kdu_coords compositing_offset);
      /* [SYNOPSIS]
           This function conveniently performs all the coordinate mapping
           operations prescribed by the `custom_paint_overlay' function,
           returning an array with `num_regions' entries, corresponding to
           transformed versions of the entries supplied with the `regions'
           array on input.  The copies belong to an internal array managed
           by the present object.  You may feel free to modify them further
           if required.
      */
  //----------------------------------------------------------------------------
  private: // Data
    kdu_overlay_params()
      { memset(this,0,sizeof(*this)); codestream_idx=compositing_layer_idx=-1; }
    ~kdu_overlay_params()
      {
        if (tmp_aux_params != NULL) delete[] tmp_aux_params;
        if (ring_handle != NULL) delete[] ring_handle;
        if (roi_buf != NULL) delete[] roi_buf;
      }
  private: // Data
    friend class kdrc_overlay;
    int codestream_idx;
    int compositing_layer_idx;
    int max_painting_border;
    int num_cur_aux_params;
    const kdu_uint32 *cur_aux_params;
    int num_orig_aux_params;
    const kdu_uint32 *orig_aux_params;
    int max_tmp_aux_params; // Size of array below
    kdu_uint32 *tmp_aux_params; // Array for storing `push_aux_params' data
    int cur_stride;
    int cur_radius;
    const int *cur_ring_prefices;
    const int *cur_ring_points;
    int *all_ring_points; // Complete lookup table
    int *all_ring_prefices; // Prefices for radius 0, 1, 2, ...
    int *ring_handle; // Handle to memory block allocated for ring LUT's
    int max_rois; // Max entries in `roi_buf'
    jpx_roi *roi_buf; // Used to implement `map_jpx_regions'
  };
  /* Notes:
       There are 2*R+1 ring prefices for the ring of radius R.  Thus,
     `all_ring_prefices' contains 1 entry for radius 0, 3 entries for radius 1,
     then 5 entries for radius 2, etc.
       The first ring prefix for radius R holds the total number of ring points
     of radius R which have y coordinate <= -R.  The second ring prefix for
     radius R holds the total number of ring points of radius R which have y
     coordinate <= 1-R.  The last ring prefix for radius R holds the total
     number of ring points of radius R which have y coordinate <= R. */

/******************************************************************************/
/*                             kdu_region_compositor                          */
/******************************************************************************/

#define KDU_COMPOSITOR_SCALE_TOO_SMALL ((int) 1)
#define KDU_COMPOSITOR_CANNOT_FLIP     ((int) 2)
#define KDU_COMPOSITOR_SCALE_TOO_LARGE ((int) 4)
#define KDU_COMPOSITOR_TRY_SCALE_AGAIN ((int) 8)

class kdu_region_compositor {
  /* [BIND: reference]
     [SYNOPSIS]
       An object of this class provides a complete system for managing
       dynamic decompression, rendering and composition of JPEG2000
       compressed imagery.  This very powerful object is the workhorse
       of the "kdu_show" application, abstracting virtually all platform
       independent functionality.
       [//]
       The object can handle rendering of everything from a single component
       of a single raw code-stream, to the composition of multiple
       compositing layers from a JPX file or of multiple video tracks from
       an MJ2 (Motion JPEG2000) file.  Moreover, it supports efficient
       scaling, rotation and dynamic region-based rendering.
       [//]
       The implementation is built on top of `kdu_region_decompressor' which
       supports only a single code-stream.  Unlike that function, however,
       this object also abstracts all the machinery required for dynamic
       panning of a buffered surface over the composited image region.
       [//]
       When working with JPX sources, you may dynamically build up a
       composited image surface by adding and removing imagery layers.
       [//]
       When working with MJ2 sources, you may dynamically build up a
       composited movie frame by adding frames from individual video tracks.
       In fact, there is a conceptual similarity between video tracks in
       MJ2 and compositing layers in JPX.
       [//]
       The present object's composition rules are not limited to those which
       may apply to MJ2 or JPX.  For this reason, we will use the general
       term "imagery layer" (or "ilayer") to refer to a layer in the
       overall composition.  The composition consists of an ordered collection
       of ilayers, each of which derives its colour channels from one or
       more JPEG2000 codestreams, after possible application of any
       or all of the following operations: cropping, scaling, rotation,
       flipping, and translation.
  */
  //----------------------------------------------------------------------------
  public: // Lifecycle member functions
    KDU_AUX_EXPORT
      kdu_region_compositor(kdu_thread_env *env=NULL,
                            kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           If you use this constructor, you must issue a subsequent call to
           `create' before using other member functions.  The `env' and
           `env_queue' arguments are used to initialize the multi-threading
           state which may subsequently be changed using the `set_thread_env'
           function -- see that function for an explanation.
      */
    KDU_AUX_EXPORT
      kdu_region_compositor(kdu_compressed_source *source,
                            int persistent_cache_threshold=256000);
      /* [SYNOPSIS]  See `create'.  */
    KDU_AUX_EXPORT
      kdu_region_compositor(jpx_source *source,
                            int persistent_cache_threshold=256000);
      /* [SYNOPSIS]
           See `create'. */
    KDU_AUX_EXPORT
      kdu_region_compositor(mj2_source *source,
                            int persistent_cache_threshold=256000);
      /* [SYNOPSIS] See `create'. */
    virtual ~kdu_region_compositor() { pre_destroy(); }
      /* [SYNOPSIS]
           Note that when implementing a derived class, you should generally
           call `pre_destroy' from that class's destructor, to ensure
           proper sequencing of the destruction operations.
      */
    KDU_AUX_EXPORT virtual void pre_destroy();
      /* [SYNOPSIS]
           Does all the work of the destructor which may rely upon calling
           virtual functions which are overridden in a derived class.  In
           particular, all buffering resources are cleaned up here so that
           `delete_buffer', which may have been implemented in a derived
           class, will not be called after the derived portion has been
           destroyed.
           [//]
           When implementing a derived class, you should call this function
           from the derived class's destructor.
      */
    KDU_AUX_EXPORT void
      create(kdu_compressed_source *source,
             int persistent_cache_threshold=256000);
      /* [SYNOPSIS]
           Configures the object to use a raw code-stream source.  If the
           object is currently in-use, this function generates an error.
         [ARG: persistent_cache_threshold]
           If non-negative, each `kdu_codestream' object created within this
           object will automatically be put in the persistent mode (see
           `kdu_codestream::set_persistent') and assigned a cache threshold
           equal to the indicated value.  You should always select
           this option, unless you do not plan to use the compositor
           interactively.  If you only intend to use one composition
           configuration and you only intend to call `set_scale' and
           `set_buffer_surface' once prior to doing all processing, you can
           get away with non-persistent code-streams, which will save on
           memory.
      */
    KDU_AUX_EXPORT void
      create(jpx_source *source, int persistent_cache_threshold=256000);
      /* [SYNOPSIS]
           Creates the object to use a JPX/JP2 compatible data source.  If the
           object is currently in-use, this function generates an error.
         [ARG: persistent_cache_threshold]
           If non-negative, each `kdu_codestream' object created within this
           object will automatically be put in the persistent mode (see
           `kdu_codestream::set_persistent') and assigned a cache threshold
           equal to the indicated value.  You should always select
           this option, unless you do not plan to use the compositor
           interactively.  If you only intend to use one composition
           configuration and you only intend to call `set_scale' and
           `set_buffer_surface' once prior to doing all processing, you can
           get away with non-persistent code-streams, which will save on
           memory.
      */
    KDU_AUX_EXPORT void
      create(mj2_source *source, int persistent_cache_threshold=256000);
      /* [SYNOPSIS]
           Creates the object to use an MJ2 data source.  If the
           object is currently in-use, this function generates an error.
         [ARG: persistent_cache_threshold]
           If non-negative, each `kdu_codestream' object created within this
           object will automatically be put in the persistent mode (see
           `kdu_codestream::set_persistent') and assigned a cache threshold
           equal to the indicated value.  You should always select
           this option, unless you do not plan to use the compositor
           interactively.  If you only intend to use one composition
           configuration and you only intend to call `set_scale' and
           `set_buffer_surface' once prior to doing all processing within
           any given codestream, you can get away with non-persistent
           code-streams, which will save on memory.
      */
    KDU_AUX_EXPORT void set_error_level(int error_level);
      /* [SYNOPSIS]
           Specifies the degree of error detection/resilience to be associated
           with the Kakadu code-stream management machinery.  The same error
           level will be associated with all code-streams opened within the
           supplied `jpx_source' object, including those code-streams which
           are currently open and those which may be opened in the future.
           [//]
           The following values are defined:
           [>>] 0 -- minimal error checking (see `kdu_codestream::set_fast');
           [>>] 1 -- fussy mode, without error recovery (see
                `kdu_codestream::set_fussy');
           [>>] 2 -- resilient mode, without the SOP assumption (see
                `kdu_codestream::set_resilient');
           [>>] 3 -- resilient mode, with the SOP assumption (see
                `kdu_codestream::set_resilient').
           [//]
           If you never call this function, all code-streams will be opened
           in the "fast" mode (`error_level'=0).
      */
    void set_process_aggregation_threshold(float threshold)
      {
        if (threshold < 0.0F)
          this->process_aggregation_threshold = 0.0F;
        else if (threshold < 1.0F)
          this->process_aggregation_threshold = threshold;
        else
          this->process_aggregation_threshold = 1.0F;
      }
      /* [SYNOPSIS]
           You can use this function to control the aggregation of newly
           processed regions within the `process' function.  If the
           aggregation `threshold' is >= 1.0, no aggregation will take
           place and the rectangular regions returned by `process' will
           normally contain entirely new data (to the extent that this can
           be determined by the internal implementation).
           [//]
           Otherwise, the `process' function may aggregate the regions
           produced by processing steps even if they are disjoint, returning
           a `new_region' which incorporates non-novel content between such
           disjoint regions.  In this case, the area occupied by supposedly
           novel content should be no smaller than `threshold' times the area
           of the `new_region' returned by `process'.
      */
    void set_surface_initialization_mode(bool pre_initialize)
      {
        initialize_surfaces_on_next_refresh =
          (pre_initialize && can_skip_surface_initialization);
        can_skip_surface_initialization = !pre_initialize;
      }
      /* [SYNOPSIS]
           This function was introduced for the first time in v5.0 so that
           video rendering applications can proceed with the maximum possible
           efficiency.  However, you may find it useful in other applications.
           [//]
           By default, whenever a new frame buffer is allocated, its
           contents are initialized to an appropriate default state.  This
           means that the buffer retrieved via `get_composition_buffer' can
           be painted to a display, even before the imagery has been fully
           decompressed.  Although initializing the buffer does not take long,
           this overhead can noticeably slow down video rendering applications.
           Moreover, when motion video is being displayed, the buffer surface
           is always fully rendered prior to being displayed, so as to avoid
           tearing.  For these applications, or any application in which you
           know that the `process' function will be called until rendering is
           complete before displaying anything, it is a good idea to call this
           function with `pre_initialize' set to false.
           [//]
           You can change the pre-initialization mode at any time.  However,
           changes will not be reflected until new buffers are allocated or
           the `refresh' function is called.  If you want to switch from
           a real-time video rendering mode to incremental rendering of a
           single frame or image, it is a good idea to first call this
           function with `pre_initialize' to true and then call `refresh',
           so as to be sure that surface buffers which are currently only
           partially rendered can be meaningfully displayed.  The call to
           `refresh' can be safely omitted, of course, if
           `is_processing_complete' already returns true.
           [//]
           If this all seems to hard for you, and you don't care about the
           last iota of speed, you don't need to bother with this function
           at all.
      */
  //----------------------------------------------------------------------------
  public: // Functions used to build and tear down compositions
    KDU_AUX_EXPORT kdu_ilayer_ref
      add_ilayer(int layer_src, kdu_dims full_source_dims,
                 kdu_dims full_target_dims, bool transpose=false,
                 bool vflip=false, bool hflip=false,
                 int frame_idx=0, int field_handling=2);
      /* [SYNOPSIS]
           You must add at least one "imagery layer" (ilayer) to form a
           valid composition.  This is done using one of the following
           functions:
           [>>] `add_ilayer' adds imagery from JP2/JPX/MJ2 data sources or a
                raw codestream data source.
           [>>] `add_primative_ilayer' adds imagery corresponding to a
                codestream from any source, ignoring any file-format
                dependent rendering information.  This method can be used
                to render a single image component from any codestream,
                but it can also be used to render a multi-channel image
                based on the first 3 codestream components (if they exist
                and have compatible dimensions).
           [>>] `set_frame' constructs all the imagery layers required to
                build up a complete JPX animation frame.
           [//]
           You can add ilayers to or remove them from the composition at any
           time, although you should call `get_composition_buffer' again after
           you have finished modifying the layer configuration to get a buffer
           which will legitimately represent the composed imagery.
           [//]
           The new ilayer is placed at the end of the compositing list.
           Where multiple layers are to be composited together, they are
           rendered onto the composition surface in the order given by this
           list.
           [//]
           For JPX data sources, the `layer_src' argument is interpreted as
           the zero-based JPX compositing layer index (as used by such
           functions as `jpx_souce::access_layer').  For simple JP2 files and
           raw codestreams, the only valid `layer_src' value is 0.  For
           MJ2 data sources, `layer_src' is interpreted as the MJ2 track index
           (as returned by `mj2_video_source::get_track_idx') minus 1, so that
           a value of zero (rather than 1) references the first track.
           [//]
           Note that this function may cause an error to be generated through
           `kdu_error' if composition is not possible (e.g. due to colour
           conversion problems), or if an error occurs while examining the
           embedded code-streams.  For many applications, the implementation
           of `kdu_error' will throw an exception.
         [RETURNS]
           An instance of the opaque `kdu_ilayer_ref' class which provides
           sufficient information to uniquely identify the newly created
           imagery layer.  In most cases, failure to add a new imagery layer
           (e.g., due to invalid arguments) results in an error being generated
           through `kdu_error', which typically throws an exception -- in
           any case, there will be no return value.
           [//]
           In the special case (and only this case) where a JPX/JP2/MJ2 data
           source is served by a dynamic cache which does not yet contain
           sufficient information to open the relevant compositing layer
           and/or all relevant codestream main headers, the function returns
           a null `kdu_ilayer_ref' instance -- i.e., `kdu_ilayer_ref::is_null'
           will yield true.
         [ARG: layer_src]
           For JPX data sources, this is the index (starting from 0) of a
           compositing layer within that source.  For JP2 files and
           raw code-streams, this argument must be 0.  For MJ2 files, this
           is the zero-based track index (i.e., 1 less than the value returned
           by `mj2_video_source::get_track_idx' for the same track).
         [ARG: full_source_dims]
           This argument identifies the portion of the original imagery
           represented by `layer_src' which is to be used to construct the
           new ilayer.   To use this argument effectively you need to know
           how the dimensions of the original imagery (prior to any cropping)
           are assessed.
           [>>] If the data source is a raw codestream, the dimensions of the
                original imagery are those of the first image component,
                rather than those of the high resolution codestream canvas --
                note that this is an exceptional case.
           [>>] For JP2/JPX sources, the dimensions of the original imagery
                are those of the JPX compositing layer's reference grid.  The
                JPX compositing layer reference grid is often identical to
                the high resolution canvas of its code-stream (or
                code-streams).  More generally, however, the JPX compositing
                layer reference grid is related to the code-stream canvas
                grids in the manner identified by the
                `jpx_layer_source::get_codestream_registration' function.
           [>>] For MJ2 sources, the dimensions of the original imagery are
                those obtained via `mj2_video_source::access_dimensions', which
                should be identical to the dimensions of each codestream in
                the track, as expressed on its high resolution canvas.
           [//]
           For simple applications, you can supply an empty region for this
           argument (i.e., one for which `full_source_dims.is_empty' returns
           true).  In this case, the `full_source_dims' argument is effectively
           replaced with one which represents all of the original imagery --
           i.e., `full_source_dims.pos' is taken to be (0,0) and
           `full_source_dims.size' is taken to be the dimensions of the
           original imagery, as explained above.
         [ARG: full_target_dims]
           This argument identifies the region of the composited image onto
           which this ilayer is to be composited. Scaling may be required to
           match `full_target_dims.size' to `full_source_dims.size'.
           The coordinates of `full_target_dims' are expressed relative to
           the composited image which would be produced if `set_scale' were
           called with a global `scale' factor of 1.0 and global
           transposition and flipping flags all set to false.
           [//]
           For simple applications, you can supply an empty region for this
           argument (i.e., one for which `full_target_dims.is_empty' returns
           true).  In this case, the `full_target_dims.pos' value is
           effectively replaced with (0,0) and `full_target_dims.size' is
           effectively replaced with `full_source_dims.size', after
           replacing an initially empty `full_source_dims' with default
           values in the manner described above, and allowing for
           the effect of the `transpose' argument.  What this means is that
           the source image region will be composited directly as-is,
           anchored at the top left corner of the composited image.
         [ARG: transpose]
           This argument may be used together with `vflip' and `hflip' to
           adjust the geometry of the ilayer's source material prior to scaling
           and offsetting it to the `full_target_dims' region.  This means
           that `full_source_dims' refers to the source imagery prior
           to any geometric corrections, while `full_target_dims' refers to
           the composited imagery, after applying geometric corrections.
           [//]
           The geometric transformations performed here are equivalent to
           those described for the `kdu_codestream::change_appearance'
           function.  Note that these geometric transformations are in
           addition to any global transformations (conceptually
           post-composition transformations) which might be specified by
           `set_scale'.
           [//]
           Note that the geometric transformations
           provided by `transpose', `vflip' and `hflip' provide a useful
           means of incorporating the track-specific geometric tranformations
           recovered via `mj2_video_source::get_cardinal_geometry'
         [ARG: vflip]
           See `transpose'.
         [ARG: hflip]
           See `transpose'.
         [ARG: frame_idx]
           For non-MJ2 data sources, this argument is ignored (effectively
           forced to 0).  For MJ2 data sources, this argument specifies the
           frame number (starting from 0) within the track identified via
           `layer_src'.  If the frame does not exist, the function generates
           an error.
           [//]
           Note that it is legal to composit multiple ilayers corresponding
           to separate frames from the same track, but this would not be
           the normal way to render an MJ2 data source.       
         [ARG: field_handling]
           Ignored unless the object is using an MJ2 data source.
           This argument specifies the way in which fields are to be handled
           where the frames are interlaced.  The argument is ignored if the
           frames are progressive.  The following values are defined:
           [>>] 0 -- render the frame from field 0 only
           [>>] 1 -- render the frame from field 1 only
           [>>] 2 -- render the frame from field 0 and field 1 (interlaced)
           [>>] 3 -- render the frame from field 1 of the current frame and
           field 0 of the next frame (interlaced), or from field 1
           alone if this is already the last frame.
      */
    KDU_AUX_EXPORT bool
      change_ilayer_frame(kdu_ilayer_ref ilayer_ref, int frame_idx);
      /* [SYNOPSIS]
           This function provides an efficient mechanism for changing the
           particular frame which is currently being used to render the
           indicated ilayer ("imagery layer").  The function may be used
           only when the ilayer was created from an MJ2 data source.
           [//]
           The advantage of using this function (as opposed to removing the
           ilayer and adding it again with a new frame index) is that no
           further calls to `set_scale' or `set_buffer_surface' are required
           to prepare the rendering process.  Furthermore, the internal
           machinery will attempt to reuse codestream structures in an
           efficient way, rather than creating them from scratch.  It is
           also worth noting that this function does not change the
           identity of the ilayer (as given by `ilayer_ref') whereas
           removing the ilayer ad adding a new one would necessarily cause
           a new `kdu_ilayer_ref' instance to be assigned.
           [//]
           Typically, the `ilayer_ref' argument is the value returned by the
           relevant call to `add_ilayer', but you may also obtain valid
           `kdu_ilayer_ref' instances using other functions in the current
           object's API.  If `ilayer_ref' does not correspond to a currently
           active ilayer, this function will generate an error through
           `kdu_error'.
         [RETURNS]
           False if the requested frame does not exist or cannot yet be
           opened because the source is served by a dynamic cache, which
           does not yet have sufficient information.  In this case, the
           object will continue to produce content based on the original
           frame, but subsequent calls to `refresh' and `set_scale' will
           attempt to make the frame change which was requested here, if more
           information has since become available in the cache -- it is not
           necessary, therefore, to call this function again if a false
           return was produced while using a valid frame index.
         [ARG: ilayer_ref]
           Typically, the `kdu_ilayer_ref' instance returned by
           `add_ilayer'.
         [ARG: frame_idx]
           Same interpretation as in `add_ilayer'.
      */
    KDU_AUX_EXPORT kdu_ilayer_ref
      add_primitive_ilayer(int stream_src, int &single_component_idx,
                           kdu_component_access_mode single_access_mode,
                           kdu_dims full_source_dims,
                           kdu_dims full_target_dims, bool transpose=false,
                           bool vflip=false, bool hflip=false);
      /* [SYNOPSIS]
           The present function is virtually identical to `add_ilayer',
           except that it adds an imagery layer which renders its content
           directly from a codestream, without any file-format provided
           rendering information.  The function can be used to render a
           single image component, or (if possible) an RGB image from the
           first three image components.  In the specific case where the
           present object's data source is a raw codestream (as opposed to
           an MJ2, JP2 or JPX file) and a -ve `single_component_idx' value
           is supplied, this function does the same thing as `add_ilayer'.
           [//]
           The `stream_src' argument identifies the codestream in question
           using the conventions outlined below.  This argument must be 0
           if the data source is a raw codestream or simple JP2 file, but may
           take on other values for MJ2 and JPX data sources, which can
           contain many codestreams.
           [//]
           The optional `single_access_mode' argument determines whether
           a requested single image component index should be considered as
           referring to a final output component (after applying any colour
           transforms or Part 2 multi-component transforms defined at the
           codestream level) or a raw codestream component (obtained after
           decoding and inverse spatial wavelet transformation).  In the
           former case (`KDU_WANT_OUTPUT_COMPONENTS'), additional components
           may need to be decompressed internally, so that the requested
           component can be reconstructed.
           [//]
           As with any of the functions which change the composition, you
           should be sure at least to call `get_composition_buffer' after
           invoking this function.
         [RETURNS]
           A null `kdu_ilayer_ref' instance (one whose `kdu_ilayer_ref:is_null'
           function returns true) if the code-stream cannot yet be opened.
           This happens only when a JPX/JP2/MJ2 data source is served by a
           dynamic cache which does not yet have enough information to
           actually open the code-stream, or to verify whether or not it
           exists.  If the requested code-stream is known not to exist, the
           function generates an appropriate error through `kdu_error',
           rather than returning.
         [ARG: stream_src]
           If the data source is a raw codestream, `stream_src' must be 0.  If
           the data source is JP2/JPX, `stream_src' is the positional index
           of the codestream (starting from 0) within the source.  If the
           data source is MJ2, `stream_src' is the unique codestream index
           whose interpretation is described in connection with
           `mj2_video_source::get_stream_idx'.
         [ARG: single_component_idx]
           If you want the function to try to create an RGB imagery layer
           formed from the first 3 image components of the codestream if
           reasonable (i.e., if there are at least 3 components and they
           all have the same dimensions), pass a value of -1 in for this
           argument -- the function will not alter the supplied value in this
           case.
           [//]
           Otherwise, pass the zero-based index of a single image component
           which is to be rendered all by itself as a monochrome imagery layer.
           Upon successful return, this argument holds the index of the image
           component which is actually used; this may be different to the
           supplied value if that value was outside the range of available
           components for the code-stream.
         [ARG: single_access_mode]
           Allows you to specify whether the single image component identified
           by a non-negative `single_component_idx' argument is to
           be considered a codestream image component (i.e., prior to
           application of any inverse multi-component transform) or an output
           image component (i.e., at the output of any inverse multi-component
           transform).
         [ARG: full_source_dims]
           Same as in `add_ilayer', but note that the dimensions of the
           original imagery for a single image component are those of the
           image component itself, as returned by `kdu_codestream::get_dims',
           not the image dimensions expressed on high resolution codestream
           canvas.
         [ARG: full_target_dims]
           As in `add_ilayer'.
         [ARG: transpose]
           As in `add_ilayer'.
         [ARG: vflip]
           See `transpose'.
         [ARG: hflip]
           See `transpose'.
       */
    KDU_AUX_EXPORT bool
      remove_ilayer(kdu_ilayer_ref ilayer_ref, bool permanent);
      /* [SYNOPSIS]
           Use this function to remove imagery layers from the current
           composition and/or to permanently destroy the resources allocated
           to layers.  You can simultaneously remove all layers by passing
           in a null `kdu_ilayer_ref' instance -- i.e., one whose
           `kdu_ilayer_ref::is_null' function returns true.  The
           `kdu_ilayer_ref' object's default constructor creates such an
           instance.
           [//]
           Unused ilayers can also be implicitly removed in the process of
           adding new ilayers, by using the `set_frame' function.
           [//]
           As with any of the functions which change the composition, you
           should be sure at least to call `get_composition_buffer' after
           invoking this function.
         [RETURNS]
           True if anything was removed permanently or any active layer was
           moved to the inactive list.
         [ARG: permanent]
           If true, the ilayer is permanently destroyed, cleaning up its
           internal resources (they may be substantial).
           [//]
           Otherwise, the ilayer is moved onto an inactive list, from which
           it may be retrieved later with the `add_ilayer'
           function (or indirectly using `set_frame').  The ilayer may later
           be permanently removed from the inactive list by calling the
           present function at any time, with `permanent' set to true.  In
           addition, layers which have been inactive for some time may be
           destroyed by calling `cull_inactive_ilayers'.
         [ARG: ilayer_ref]
           If this is a null `kdu_ilayer_ref' instance (i.e., if
           `ilayer_ref.is_null' reports true), all ilayers are simultaneously
           removed.  Otherwise, only the ilayer (if any) which matches
           `ilayer_ref' is affected.  It is OK to supply an `ilayer_ref'
           which no longer corresponds to an active or inactive
           imagery layer, in which case the function does nothing.
      */
    KDU_AUX_EXPORT void cull_inactive_ilayers(int max_inactive);
      /* [SYNOPSIS]
           This function may be used to remove some of the least recently
           used ilayers from the internal inactive list.  Imagery layers
           are added to this list when `remove_ilayer' is called
           with a `permanent' argument of false.  Old ilayers are also moved
           to the inactive list when `set_frame' is used to advance to a
           new frame in a JPX animation.
         [ARG: max_inactive]
           Number of most recently active ilayers to leave on the
           inactive list.
      */
    KDU_AUX_EXPORT void set_frame(jpx_frame_expander *expander);
      /* [SYNOPSIS]
           Use this function to add an entire set of imagery layers,
           corresponding to the compositing layers obtained by invoking
           `jpx_frame_expander::get_member' on the supplied `expander' object.
           The `expander' object's `jpx_frame_expander::construct' function
           should have already been called (generally with its
           `follow_persistence' argument set to true) to find the compositing
           layers and compositing instructions associated with the a complete
           animation frame.  Moreover, the call to
           `jpx_frame_expander::construct' must have returned true, meaning
           that all required compositing layers are available in the JPX
           data source.  Note, however, that some of the codestream main
           headers might not yet be available.  This is acceptable when
           opening a frame.
           [//]
           The function essentially just adds all of the relevant compositing
           layers in turn, as though you had explicitly called
           `add_ilayer', moving all layers which are no longer
           required to the inactive list.  However, it also uses the
           JPX data source's `jpx_composition' object to recover the
           dimensions of the compositing surface, placing the imagery layers
           of the frame onto this surface.
           [//]
           Since the function does not permanently destroy any of the
           compositing layers which may have previously been in use, you
           may wish to invoke `cull_inactive_ilayers' after each
           successful return from the present function.
           [//]
           As with any of the functions which change the composition, you
           should be sure at least to call `get_composition_buffer' after
           invoking this function.
      */
    KDU_AUX_EXPORT bool
      waiting_for_stream_headers();
      /* [SYNOPSIS]
           This function is provided primarily because the `set_frame' function
           is able to succeed even if the relevant codestream headers are not
           yet available (when the ultimate source of compressed data is a
           compressed cache).  This is because the size of the composited frame
           is determined separately from the dimensions of the individual
           compositing layers and codestreams.  However, if the codestream
           headers are not yet available, some other functions provided by the
           object may not be able to compute the required information -- e.g.,
           `map_region' and `inverse_map_region'.
           [//]
           If the present function returns false, you can be sure that all
           stream headers are loaded and sufficient information is available to
           map regions, access the codestreams and so forth.  If not, you will
           have to wait until more information arrives in the cache and invoke
           `refresh', after which the stream headers may be available, so that
           this function may be able to return false.
      */
  //----------------------------------------------------------------------------
  public: // Functions for adjusting/querying the composed image appearance
    KDU_AUX_EXPORT void
      set_scale(bool transpose, bool vflip, bool hflip, float scale);
      /* [SYNOPSIS]
           Sets any rotation, flipping or transposition to be performed,
           and the scale at which the image is to be composed.
           The interpretation of the `transpose', `hflip' and `vflip' arguments
           is identical to that described in conjunction with the
           `kdu_codestream::change_appearance' function.
           [//]
           After calling this function, and before calling `process', you
           must call `set_buffer_surface' to identify the portion of the
           image you want rendered.  You must not assume that the image
           buffer returned by a previous call to `get_composition_buffer'
           is still valid, so you should generally call that function again
           also to get a handle to the buffer which will hold the composited
           image.
           [//]
           Note that the current function does not actually verify that the
           scale parameters are compatible with the composition being
           constructed.  It merely sets some internal state information
           which will be used to adjust the internal configuration on the
           next call to `get_composition_buffer', `get_total_composition_dims'
           or `process'.  Any of these functions may return a NULL/false/empty
           response if the scale parameters were found to be invalid during
           any step which they executed internally (this could happen half way
           through processing a region, if a tile-component with insufficient
           DWT levels is encountered, for example).
         [ARG: scale]
           A value larger than 1 implies that the image should be composed
           at a larger (zoomed in) size than the nominal size associated with
           `add_ilayer' and related functions.  Conversely, a value smaller
           than 1 implies a zoomed out representation.
           [//]
           Note that the actual scale factors which must be applied to
           individual codestream image components may be quite different,
           since their composition rules may require additional scaling
           factors.
           [//]
           Note also that the optimal scale at which to render an individual
           codestream is a positive integer or a reciprocal power of 2.
           Since you may not know how the global scale parameter supplied
           here translates into the scale factor used for a particular
           codestream of interest, this object provides a separate function,
           `find_optimal_scale', which may be used to guide the selection
           of optimal scale factors, based on the codestream content which
           is being used to render the information in a particular region
           of interest.  That function also allows you to discover what
           limit was encountered if `check_invalid_scale_code' revealed a
           problem with the currently installed scaling factor.
      */
    KDU_AUX_EXPORT float
      find_optimal_scale(kdu_dims region, float scale_anchor,
                         float min_scale, float max_scale,
                         kdu_istream_ref *istream_ref=NULL,
                         int *component_idx=NULL,
                         bool avoid_subsampling=false);
      /* [SYNOPSIS]
           The principle purpose of this function is to find good scale
           factors to supply to `set_scale' so as to maximize rendering
           efficiency by selecting a scale which corresponds to a native
           resolution from the codestream.
           [//]
           A second purpose of this function is to discover whether or not
           an intended scaling factor is acceptable to the internal machinery.
           The `kdu_region_compositor' object is able to scale imagery up and
           down by massive amounts, but there are limits.  If such a limit is
           encountered, the `get_total_composition_dims' and/or
           `get_composition_buffer' functions will fail, leaving information
           about the nature of the failure behind to be recovered by a call
           to `check_invalid_scale_code'.  The caller can then invoke the
           present function to discover the closest acceptable scaling factor
           to that which caused the problem.
           [//]
           Optimal scaling factors are those which scale the principle
           codestream image component within a region by either a positive
           integer or a reciprocal power of 2 (out to the number
           of available DWT levels for the relevant codestream).  The
           principle codestream image component is generally the first
           component involved in colour intensity reproduction for the
           imagery layer which contributes the largest visible area to
           the supplied `region'.  The following special cases apply:
           [>>] If a valid scale has not yet been set (i.e. if
                `get_total_composition_dims' returns false), the `region'
                argument cannot be used, since the region must be assessed
                relative to an existing scale/geometry configuration
                created by the last call to `set_scale'.  In this case,
                the principle codestream is derived from the top-most
                imagery layer, regardless of its size.
           [>>] If no imagery layer contributes to the supplied `region',
                even though a valid scale has already been installed, we say
                that there are no optimal scaling factors.  Then, in
                accordance with the rules outlined below, the function returns
                the nearest value to `scale_anchor' which lies in the range
                from `min_scale' to `max_scale'.
           [//]
           If no optimal scaling factors lie in the range from `min_scale'
           to `max_scale', the function returns the nearest acceptable scaling
           factor to `scale_anchor'.  If multiple optimal scaling factors lie
           in the range, the function returns the one which is closest to
           `scale_anchor'.
         [ARG: region]
           The function looks for the imagery layer which has the largest
           visible area in this region, on which to base its scaling
           recommendations.  If the supplied `region' is empty, or if a
           valid scale has not previously been installed (i.e., if
           `get_total_composition_dims' returns false), the top-most
           compositing layer (or raw codestream) is used to compute the
           sizing information.
         [ARG: istream_ref]
           If non-NULL, this argument is used to return an instance of the
           `kdu_istream_ref' class which uniquely identifies the codestream
           and imagery layer which have been used to compute the optimal scale.
           The associated ilayer is the one which was found to occupy the
           largest visible area within `region'.  If `region' is empty or a
           valid scale has not previously been installed (i.e., if
           `get_total_composition_dims' returns false), the istream reference
           returned via this argument identifies the primary codestream of
           the top-most imagery layer, regardless of its area.  If the
           function was unable to find any relevant imagery layer, the
           function sets *`istream_ref' to a null istream reference -- i.e.,
           one whose `kdu_istream_ref::is_null' member returns true.
         [ARG: component_idx]
           If non-NULL, this argument is used to return the index of the
           particular image component within the selected codestream, on which
           all scaling recommendations and other information is based.  As
           mentioned above, this "principle" image component is generally the
           one responsible for producing the first input channel to the colour
           rendering process.  If the function was unable to find any relevant
           imagery layer, this value will be set to -1.
           [//]
           The component index returned here represents an output image
           component (see `kdu_codestream::apply_input_restrictions') except
           in the case where the relevant imagery layer was constructed
           using `add_primitive_ilayer', with an `access_mode' argument of
           `KDU_WANT_CODESTREAM_COMPONENTS'.
         [ARG: avoid_subsampling]
           If false, the function considers all scaling factors to be legal
           unless they would break some part of the internal implementation
           (values which are vastly too small or vastly too large).  If
           this argument is true, however, the function limits the set of
           legal return values to those scaling factors which can be
           implemented without resorting to sub-sampling decompressed imagery,
           or at least not doing this with the principle image component of
           the principle codestream.
      */
    KDU_AUX_EXPORT void set_buffer_surface(kdu_dims region,
                                           kdu_int32 background=-1);
      /* [SYNOPSIS]
           Use this function to change the location, size and/or background
           colour of the buffered region within the complete composited image.
           [//]
           Note that the actual buffer region might be different to that
           specified, if the one supplied does not lie fully within the
           composited image region.  Call `get_composition_buffer' to find out
           the actual buffer region.
           [//]
           Note also that any buffer previously recovered using
           `get_composition_buffer' will no longer be valid.  You must call
           that function again to obtain a valid buffer.
         [ARG: background]
           You can use this optional argument to control the colour (and alpha)
           of the background onto which imagery is painted.  The background
           colour is of interest when the imagery is alpha blended.  The
           background alpha is of interest if you would like to be able to
           blend the the final composited image onto yet another buffer
           in your application.  As of Kakadu version 6.3, buffers may have
           either a 32-bit/pixel organization or a floating-point organization.
           In the former case, Alpha, red, green and blue components all
           lie in the range 0 to 255 and the `background' value holds
           Alpha in the most significant byte, followed by red, green and then
           blue in the least significant byte.  In the floating-point case,
           Alpha, red, green and blue components all lie in the range 0.0 to
           1.0 and the four 8-bit values provided by `background' are divided
           by 255 to obtain the floating-point background.
           [//]
           To understand what happens to background alpha values during
           composition, let M be the maximum of the relevant representation
           (1.0 for floating-point, 255 for integers).  If opaque imagery is
           composited over some region of the background, the alpha value in
           that region will be changed to M.  If, however, partially
           transparent imagery is blended over some region, having an alpha
           value of Ai, and the background (supplied here) has an alpha value
           of Ab, the alpha value in that location will be changed to
           [>>]   M - [(M-Ab) * (M-Ai) / M]
           [//]
           This means, for example, that fully transparent imagery, composed on
           top of a background will not alter the alpha value of the background.
           It also means that the alpha value will be M (opaque) if either
           the background or the composed imagery are opaque.
      */
    int check_invalid_scale_code() { return invalid_scale_code; }
      /* [SYNOPSIS]
           This function allows you to figure out what went wrong if any
           of the functions `process', `get_total_composition_dims' or
           `get_composition_buffer' returns a false or NULL result.  As
           noted in connection with those functions, this can happen if
           the scale requested by the most recent call to `set_scale' is too
           large, too small, or its impact on the overall rendering dimensions
           needs to change slightly, given the limited number of DWT levels
           which can be discarded during decompression of some relevant
           tile-component.  Alternatively, it may happen because the requested
           rendering process calls for geometric flipping of one or more image
           surfaces, and this operation cannot be performed dynamically
           during decompression, due to the use of adverse packet wavelet
           decomposition structures.  These conditions are identified by
           the presence of one or more of the following error flags in
           the returned word:
           [>>] `KDU_COMPOSITOR_SCALE_TOO_SMALL' -- remedy: increase the scale.
           [>>] `KDU_COMPOSITOR_SCALE_TOO_LARGE' -- remedy: decrease the scale.
           [>>] `KDU_COMPOSITOR_TRY_SCALE_AGAIN' -- remedy: re-invoke
                `get_total_composition_dims' and `get_composition_buffer',
                since dimensions need to change slightly for the current scale.
           [>>] `KDU_COMPOSITOR_CANNOT_FLIP' -- remedy: try an alternate
                rendering geometry.
           [//]
           If you need to change the scale in response to
           `KDU_COMPOSITOR_SCALE_TOO_SMALL' or `KDU_COMPOSITOR_SCALE_TOO_LARGE',
           you can use the `find_optimal_scale' function to determine the
           closest acceptable scaling factor to the one you are currently
           using, by passing the current scale factor as the `min_scale',
           `max_scale' and `scale_anchor' arguments to that function.
           [//]
           If the return value is 0, none of these problems has been detected.
           It is worth noting, however, that the `invalid_scale_code' value
           returned here is reset to 0 each time `set_scale' is called, after
           which an attempt must be made to call one of
           `process', `get_total_composition_dims' or `get_composition_buffer'
           before an error code can be detected.
           [//]
           It is also worth noting that rendering might proceed fine within
           some region, without generating an invalid scale code, but this
           may change later when we come to rendering a different region.
           This is because the adverse conditions responsible for the code
           may vary from tile-component to tile-component within JPEG2000
           codestreams.
       */
    KDU_AUX_EXPORT bool get_total_composition_dims(kdu_dims &dims);
      /* [SYNOPSIS]
           Retrieves the location and size of the total composited image, based
           on the current set of compositing layers (or the raw image
           component) together with the information supplied to `set_scale'.
           This is the region within which the composition buffer's
           region may roam (for interactive panning).
           [//]
           Note that the origin of the composition buffer might not lie at
           (0,0). In fact, it almost certainly will not lie at (0,0) if the
           image has been re-oriented.
         [RETURNS]
           False if composition cannot be performed at the current scale or
           with the current flipping requirements.  These conditions may not
           be uncovered until some processing has been performed, since it may
           not be until that point that some tile-component of some code-stream
           is found to have insufficient DWT levels.  If this happens, you
           should call `check_invalid_scale_code' and make appropriate
           changes in the scale or rendering geometry before proceeding.
      */
    KDU_AUX_EXPORT kdu_compositor_buf *
      get_composition_buffer(kdu_dims &region);
      /* [SYNOPSIS]
           Returns a pointer to the buffer which stores composited imagery,
           along with information about the region occupied by the buffer
           on the composited image.  To recover the origin of the composited
           image, use `get_total_composition_dims'.
           [//]
           You should call this function again, whenever you do anything
           which may affect the geometry of the composited image.
           Specifically, you should call this function again
           after any call to `set_buffer_surface', `set_scale',
           `add_ilayer', `add_primitive_ilayer', `set_frame' or
           `remove_ilayer'.
           [//]
           Note that the buffer returned by this function may or may not
           be the current working buffer which is affected by calls to
           `process'.  In particular, if `push_composition_buffer' has been
           used to create a non-empty queue of fully composed composition
           buffers, the present function returns a pointer to the head of
           the queue.  If the queue is empty, the returned pointer refers
           to the current working buffer.  Whether or not the composition
           buffer queue is empty may itself be determined by calling
           `inspect_buffer_queue' with an `elt' argument of 0.
         [RETURNS]
           NULL if composition cannot be performed at the current scale, or
           with the current flipping requirements -- see the description of
           `get_total_composition_dims' for more on this, particularly with
           reference to the need to call `check_invalid_scale_code'.
           [//]
           Otherwise, the returned buffer has one of the following two
           organizations:
           [>>] If `kdu_compositor_buf::get_buf' returns non-NULL, the buffer
                uses 4 bytes to represent each pixel.  The most significant
                byte in each word represents alpha, followed by red, green and
                blue which is in the least significant byte of the word.
           [>>] If `kdu_compositor_buf::get_float_buf' returns non-NULL, the
                buffer uses 4 floating point values to represent each pixel.
                The first one is alpha, followed by red, green and then blue.
                All sample values have been scaled so to a nominal range of
                0.0 to 1.0, where the maximum amplitude for a colour plane
                is precisely 1.0.
      */
    KDU_AUX_EXPORT bool
      push_composition_buffer(kdu_long stamp, int id);
      /* [SYNOPSIS]
           This function is used together with `get_composition_buffer'
           and `pop_composition_buffer' to manage an internal queue of
           processed composition buffers.  This service may be used to
           implement a jitter-absorbtion buffer for video applications, in
           which case the `stamp' argument might hold a timestamp, identifying
           the point at which the generated frame should actually be displayed.
           [//]
           Only completely processed composition buffers may be pushed onto
           the tail of the queue.  That means that `is_processing_complete'
           should return true before this function will succeed.  Otherwise,
           the present function will simply return false and do nothing.
           [//]
           If the function succeeds (returns true), the processed surface
           buffer is appended to the tail of the internal queue and
           a new buffer is allocated for subsequent processing operations.
           The new buffer is marked as empty so that the next call to
           `is_processing_complete' will return false, and future calls to
           `process' are required to paint it.
           [//]
           The internal management of composition buffers may be visualized
           as follows:
           [>>] Q_1 (head)  Q_2 ... Q_N (tail)  W
           [//]
           Here, the processed composition buffer queue consists of N
           elements and W is the current working buffer, which may or
           may not be completely processed.  When `push_composition_buffer'
           succeeds, W becomes the new tail of the queue, and a new W is
           allocated.
           [//]
           The `pop_composition_buffer' function is used to remove the head
           of the queue, while `inspect_composition_buffer' is used to
           examine the existence and the `stamp' and `id' values of any of the
           N elements currently on the queue.
           [//]
           When the queue is non-empty, `get_composition_buffer' actually
           returns a pointer to the head of the queue (without removing it).
           When the queue is empty, however, the `get_composition_buffer'
           function just returns a pointer to the working buffer, W.
           [//]
           If the location or size of the current buffer region is changed
           by a call to `set_buffer_surface', the queue will automatically
           be deleted, leaving only a working buffer, W, with the appropriate
           dimensions.  The same thing happens if `set_scale' is called, or if
           any adjustments are made to the composition (e.g., through
           `add_ilayer', `add_primitive_ilayer', `remove_ilayer' or
           `set_frame') which affect the scale or
           buffer surface region which is being rendered.  This ensures that
           all elements of the queue always represent an identically sized
           and positioned surface region.  However, changes in the elements
           used to build the compositing buffer which do not affect the
           current scale or buffer region will leave the composition queue
           intact.
         [RETURNS]
           True if a valid, fully composed buffer is available (i.e., if
           `is_processing_complete' returns true).  Otherwise, the function
           returns false and does nothing.
         [ARG: stamp]
           Arbitrary value stored along with the queued buffer, which can be
           recovered using `inspect_composition_queue'.  When implementing a
           jitter-absorbtion buffer, this will typically be a timestamp.
         [ARG: id]
           Arbitrary identifier stored along with the queued buffer, which
           can be recovered using `inspect_composition_queue'.  This might
           be a frame index, or a reference to additional information stored
           by the application.
      */
    KDU_AUX_EXPORT bool
      pop_composition_buffer();
      /* [SYNOPSIS]
           Removes the head of the composition buffer queue (see
           `push_composition_buffer'), returning false if the queue was
           already empty.  If the function returns true, any pointer
           returned via a previous call to `get_composition_buffer' should
           be treated as invalid.
      */
    KDU_AUX_EXPORT bool
      inspect_composition_queue(int elt, kdu_long *stamp=NULL, int *id=NULL);
      /* [SYNOPSIS]
           Use this function to inspect specific elements on the processed
           composition buffer queue.  For a discussion of this queue, see
           the discussion accompanying the `push_composition_buffer' function.
         [RETURNS]
           False if the element identified by `elt' does not exist.  If
           `elt'=0 and the function returns false, the composition buffer
           queue is empty.
         [ARG: elt]
           Identifies the particular queue element to be inspected.  A value
           of 0 corresponds to the head of the queue.  A value of N-1
           corresponds to the tail of the queue, where the queue holds N
           elements.
         [ARG: stamp]
           If non-NULL and the function returns true, this argument is used
           to return the stamp value which was supplied in the corresponding
           call to `push_composition_buffer'.
         [ARG: id]
           If non-NULL and the function returns true, this argument is used
           to return the identifier which was supplied in the corresponding
           call to `push_composition_buffer'.
      */
    KDU_AUX_EXPORT void flush_composition_queue();
      /* [SYNOPSIS]
           Removes all elements from the jitter-absorbtion queue managed
           by `push_composition_buffer' and `pop_composition_buffer'.
      */
    KDU_AUX_EXPORT void set_max_quality_layers(int quality_layers);
      /* [SYNOPSIS]
           Sets the maximum number of quality layers to be decompressed
           within any given code-stream.  By default, all quality layers
           will be decompressed.  Note that this function does not actually
           affect the imagery which has already been decompressed.  Nor
           does it cause that imagery to be decompressed again from scratch
           in future calls to `process'.  To ensure that this happens, you
           should call `refresh' before further calls to `process'.
      */
    KDU_AUX_EXPORT int get_max_available_quality_layers();
      /* [SYNOPSIS]
           Returns the maximum number of quality layers which are available
           (even if not yet loaded into a dynamic cache) from any code-stream
           currently in use.  This value is useful when determining a suitable
           value to be supplied to `set_max_quality_layers'.  Returns 0 if
           there are no code-streams currently in use.
      */
  //----------------------------------------------------------------------------
  public: // Functions used to control dynamic processing
    KDU_AUX_EXPORT virtual kdu_thread_env *
      set_thread_env(kdu_thread_env *env, kdu_thread_queue *env_queue);
      /* [SYNOPSIS]
           From Kakadu version 5.1, this function offers the option of
           multi-threaded processing, which allows enhanced throughput on 
           multi-processor (or hyperthreading) platforms.  Multi-threaded
           processing may be useful even if there is only one physical
           (or virtual) processor, since it allows decompression work to
           continue while the main application is blocked on an I/O
           condition or other event which does not involve the CPU's resources.
           [//]
           To introduce multi-threaded processing, invoke this function you
           have simply to create a suitable `kdu_thread_env' environment by
           following the instructions found with the definition of
           `kdu_thread_env', and then pass the object into this function.
           [//]
           In the simplest case, the owner of your multi-threaded processing
           group is the one which calls all of the `kdu_region_compositor'
           interface functions.  In which case the `env' object should belong
           to this owning thread and there is no need to do anything more,
           other than invoke `kdu_thread_entity::destroy' once you are
           completely finished using the multi-threaded environment.
           [//]
           It is possible, however, to have one of the auxiliary worker threads
           within your thread group access the `kdu_region_compositor' object.
           This can be useful for video or stereo processing applications,
           where separate `kdu_region_compositor' objects are created to
           manage two different image buffers.  In this case, you may create
           separate queues for each of the objects, and have the various calls
           to `kdu_region_compositor' delivered from jobs which are run on the
           relevant queue.  This allows for parallel processing of multiple
           image composition tasks, which can be helpful in minimizing the
           amount of thread idle time in environments with many processors.
           When operating in this way, however, you must observe the following
           strict requirements:
           [>>] The thread identified by the `env' object supplied to this
                function must be the only one which is used to call any of
                this object's interface functions, from that point until
                the present function is invoked again.
           [>>] Where this function is used to identify that a new thread will
                be calling the object's interface functions (i.e., where the
                `env' argument identifies a different thread to the previous
                one), you must be quite sure that all internal processing has
                stopped.  This can be achieved by ensuring that whenever a
                job on a thread queue needs to use the present object's
                interface functions, it does not return until either
                `is_processing_complete' returns true or `halt_processing' has
                been called.  Later, if another job is run on a different
                thread, it will be able to successfully register itself as the
                new user of the object's interface functions.  Each such
                job should call this function as its first task, to identify
                the context from which calls to `process' and other functions
                will be delivered.
         [RETURNS]
           NULL if this function has never been called before, or else the
           `env' value which was supplied in the last call to this function.
           If the return value differs from the supplied `env' argument,
           access control to this object's member function is assumed to
           be getting assigned to a new thread of execution.  This is legal
           only if there is no work which is still in progress under the
           previous thread environment -- a condition which can be avoided
           by having the previous access thread call `halt_processing' or
           ensure that `is_processing_complete' returns true before
           releasing controll.  Failure to observe this constraint will
           result in the delivery of a suitable error message through
           `kdu_error'.
         [ARG: env]
           NULL if multi-threaded processing is to be disabled.  Otherwise,
           this argument identifies the thread which will henceforth have
           exclusive access to the object's member functions.  As mentioned
           above, the thread with access rights to this object's member
           functions may be changed only when there is no outstanding
           processing (i.e., when `is_processing_complete' returns true or
           when no call to `process' has occurred since the last call to
           `halt_processing').
           [//]
           As a convenience to the user, if `env' points to an object whose
           `kdu_thread_entity::exists' function returns false, the behaviour
           will be the same as if `env' were NULL.
         [ARG: env_queue]
           The queue referenced by this argument will be passed through to
           `kdu_region_decompressor::start' whenever that function is called
           henceforth.  The `env_queue' may be NULL, as discussed in connection
           with `kdu_region_decompressor::start'.  You would normally only
           use a non-NULL `env_queue' if you intended to manage multiple
           `kdu_region_compositor' object's concurrently, in which case you
           would do this by accessing them from within jobs registered on the
           relevant object's `env_queue'.  The application would then
           synchronize its access to composited results by passing the
           relevant queue into the `kdu_thread_entity::synchronize' function
           or `kdu_thread_entity::register_synchronized_job' function.
      */
    KDU_AUX_EXPORT virtual bool
      process(int suggested_increment, kdu_dims &new_region, int flags=0);
      /* [SYNOPSIS]
           Call this function regularly (e.g. during an idle processing loop,
           or within a tight loop in a processing thread) until
           `is_processing_complete' returns false.  You will generally need
           to start calling the function again after making any changes to
           the imagery layers, scale, orientation, overlays, or
           buffered region.  You may use the returned `new_region' to
           control all painting in your image application, except where the
           screen must be repainted after being overwritten by an independent
           event.
           [//]
           Note that the `new_region' returned by this function may contain
           aggregated novel regions which do not completely cover the
           rectangle identified by `new_region'; the extent of novelty
           required is determined by an aggregation threshold which you may
           customize through calls to `set_process_aggregation_threshold'.
           Smaller thresholds encourage more aggregation so that a large number
           of disjoint processed regions can be returned via a smaller number
           of calls to this function -- this may be desirable if the region
           returned by this function is used to update a graphics display
           dynamically and if such updates incur significant processing/delay
           overhead.
           [//]
           The `flags' argument provides you with a variety of ways to
           customize the behaviour of this function with regard to
           processing sequence.
           [//]
           This function may catch and rethrow an integer exception generated
           by a `kdu_error' handler if an error occurred in processing the
           underlying code-stream.
         [RETURNS]
           False only under one of the following conditions.  In all cases,
           `new_region' is guaranteed to be empty upon return.
           [>>] All processing for the current compositing buffer surface
                was already complete before this function entered, so that
                nothing was processed.  In this case, `is_processing_complete'
                would have returned true before the call to this function.
           [>>] During processing, the function encountered a code-stream
                tile-component which needed to be flipped, yet did not
                support flipping due to the use of certain types of packet
                wavelet decomposition structures; this can be verified by
                calling `check_invalid_scale_code'.  In response, the
                application may wish to change the geometric orientation
                under which it attempts to render the imagery.
           [>>] During processing, the function discovered that the rendering
                scale is too small or too large.  These conditions should be
                extraordinarily large from Kakadu version 6.3 onwards, since
                the underlying `kdu_region_decompressor' object now supports
                almost arbitrary resampling of the imagery.  Nevertheless,
                these conditions can also be discovered by calling
                `check_invalid_scale_code'; in response to one of these
                conditions, the application may wish to increase or decrease
                the scale, as appropriate, and recommence rendering.
           [>>] During processing, the function discovered that there are
                fewer DWT levels available in some tile-component than it
                could determine initially.  If this happens, the way in
                which the rendering is performed may need to be changed, and
                this in turn may slightly change the dimensions associated
                with the current scale.  This condition can be detected again
                by calling `check_invalid_scale_code'; in response, the
                application should invoke `get_total_composition_dims'
                again (it need not change the scale) as well as
                `get_composition_buffer', then continue calling the
                `process' function.
         [ARG: suggested_increment]
           The meaning of this argument is identical to its namesake in the
           `kdu_region_decompressor' object.
         [ARG: new_region]
           Upon successful return, this record holds the dimensions of a
           new region.  An interactive application may choose to repaint this
           region immediately.  It is possible that the `new_region' will be
           empty even though the function returns true, meaning that some
           processing has been done to advance the internal state of the
           rendering/compositing machinery, but this has not resulted in any
           new visible imagery.  It is actually possible that the internal
           rendering machinery needs to go back and 
         [ARG: flags]
           Any combination of the following:
           [>>] `KDU_COMPOSIT_DEFER_REGION' -- If this flag is present, the
                function will return with an empty region unless there is no
                more internal processing to perform.  In this ase, the
                function internally accumulates the regions which have
                been processed.  Each time you invoke the function you can
                turn this flag on or off.  For example, the application
                might call `process' with this flag present until some
                amount of time has passed, at which point it judges
                that some visual updates might be expected by the user and
                leaves the flag off so as to recover the largest rectangular
                region which covers imagery processed so far, updating the
                corresponding region on a display buffer.  Regardless of
                this flag, if there is no other internal work to do, the
                function will start returning processed regions, subject to
                the accumulation threshold discussed above, until the
                application knows about all processed regions.  As a general
                rule, including this flag can help reduce redundant processing
                and minimize the number of distinct regions an application
                needs to update on the display.
           [>>] `KDU_COMPOSIT_DEFER_PROCESSING' -- If this flag is present,
                the function will not perform any internal decompression
                processing.  This can be useful if the application wants
                to update the display with all regions accumulated from
                previous calls to `process' or `invalidate_rect' without
                incurring processing delays from content which still remains
                to be decompressed.  The typical application of this flag
                which is envisaged involves a sequence of calls to the
                `process' function which specify `KDU_COMPOSIT_DEFER_REGION'
                alone until some time limit expires, followed by one or more
                calls which specify the `KDU_COMPOSIT_DEFER_PROCESSING' until
                the function returns with an empty `new_region', and so forth.
                Note that this flag is ignored if `KDU_COMPOSIT_DEFER_REGION'
                is present, since otherwise all activities would be deferred.
                Note also that the `process' function may return true
                indefinitely if `KDU_COMPOSIT_DEFER_PROCESSING' is continuously
                asserted, since no decompression processing occurs so that
                processing can never complete.  Of course, once the
                function returns with an empty `new_region', it can be
                expected to continue to do so until it is called without the
                `KDU_COMPOSIT_DEFER_PROCESSING' flag.
      */
    bool is_processing_complete()
      { return processing_complete && !composition_invalid; }
      /* [SYNOPSIS]
           Returns true if all processing for the current composition buffer
           surface is complete, meaning that further calls to `process' will
           do nothing.  This situation may change if any change is made to
           the set of buffer surface position, the scale or orientation, the
           number of quality layers to be rendered, or the set (or order) of
           imagery layers to be composed.
      */
    KDU_AUX_EXPORT bool is_codestream_processing_complete();
      /* [SYNOPSIS]
           Similar to `is_processing_complete' but returns true if all
           outstanding codestream-related processing is complete, even if
           some overlay or composition processing remains to be done.  One
           reason why you might want to use this function is to determine
           the earliest point at which codestreams can be accessed and
           safely manipuated via `access_codestream' -- you should, in
           general, call `halt_processing' before accessing codestreams
           outside the compositor machinery, but doing so may cause some
           partially computed results to be discarded unless you can be
           sure that codestream processing is complete.
      */
    KDU_AUX_EXPORT bool refresh(bool *new_imagery=NULL);
      /* [SYNOPSIS]
           Call this function to force the buffer surfaces to be rendered
           and composited again, from the raw codestream data.  This is
           particularly useful when operating with a data source which is
           fed by a dynamic cache whose contents might have expanded since
           the buffer surface was last rendered and composited.
           [//]
           No processing actually takes place within this function; it simply
           schedules the processing to take place in subsequent calls to the
           `process' function.
           [//]
           The call is ignored if the object was constructed with a
           negative `persistent_cache_threshold' argument, since non-persistent
           code-streams do not allow any image regions to be decompressed
           over again.
           [//]
           If one or more of the imagery layers involved in a frame
           constructed by `set_frame' did not previously have sufficient
           codestream main header data to be completely initialized, the
           present function tries to initialize those imagery layers.  If
           this is successful, the image dimensions, buffer surface and
           dimensions last retrieved via `get_total_composition_dims' and
           `get_composition_buffer' may be invalid, so the function returns
           false, informing the caller that it should update its record of
           the image dimensions and buffer surfaces.
         [RETURNS]
           False if the image dimensions, buffer surface and dimensions
           retrieved via a previous call to `get_total_composition_dims'
           and/or `get_composition_buffer' can no longer be trusted.
         [ARG: new_imagery]
           If this argument is non-NULL, the function does not refresh anything
           unless at least one imagery layer involved in a frame constructed
           by `set_frame' is completely initialized for the first time, or a
           pending request to advance an MJ2 video frame can be granted, as
           a result of invoking this function.  Moreover, the value of
           *`new_imagery' is set to true if indeed anything was refreshed and
           false otherwise.
           [//]
           The main reason for providing this argument is to support
           applications in which the region compositor is fed from a dynamic
           cache (typically a JPIP media browser).  It is often desirable
           for the application to re-render content as soon as possible once
           a new compositing layer or video frame is available in the cache,
           but we don't want to refresh the entire display (possibly
           consisting of a massive number of compositing layers) every time
           the cache contents expand unless we know that there are indeed
           newly available codestreams for rendering imagery.
      */
    KDU_AUX_EXPORT void invalidate_rect(kdu_dims region)
      {
        region &= buffer_region;
        if (!region.is_empty())
          find_completed_rects(region,active_layers,0);
        processing_complete = false;
      }
      /* [SYNOPSIS]
           Causes `region' to be added to the internal machinery which keeps
           track of rectangular regions which need to be included in regions
           returned by `process', before `is_processing_complete' can return
           true.  The explicit invalidation of regions at most causes them to
           be recomposited onto a compositing surface -- this is quite
           different to `refresh', which results in complete re-rendering of
           the content.
           [//]
           The function is provided for the benefit of applications which
           need to repaint some portion of a composition to their display
           buffer, but want the repainting to be driven by returns from the
           `process' function so that a region whose underlying imagery has
           yet to be decompressed and rendered will not be repainted
           prematurely.
      */
    KDU_AUX_EXPORT void halt_processing();
      /* [SYNOPSIS]
           Call this function if you want to use any of the internal
           codestreams in a manner which may interfere with internal
           processing.  For example, you may wish to change the appearance,
           open tiles and so forth.  The effects of this call will be entirely
           lost on the next call to `process'.  A similar effect can be
           obtained by calling `refresh', except that this will result in
           everything being decompressed again from scratch in subsequent calls
           to `refresh'.  The present call may discard some partially processed
           results, but leaves all completely processed image regions marked
           as completely processed.
           [//]
           As an alternative to abruptly halting all processing, you can
           instead issue calls to `process' until
           `is_codestream_processing_complete' returns true.  Even though
           there may be additional processing left to perform (overlay
           painting, compositing layers onto the buffer surface, or perhaps
           processing the effects of outstanding `invalidate_rect' calls),
           once `is_codestream_processing_complete' returns true, you can
           safely manipulate codestreams yourself.
      */
  //----------------------------------------------------------------------------
  public: // Functions used to identify the elements of the composited image
          // and map locations/regions.
    KDU_AUX_EXPORT int get_num_ilayers();
      /* [SYNOPSIS]
           Returns the number of active imagery layers involved with
           the current composition.
       */
    KDU_AUX_EXPORT kdu_ilayer_ref
      get_next_ilayer(kdu_ilayer_ref last_ilayer_ref,
                      int layer_src=-1, int direct_codestream_idx=-1);
      /* [SYNOPSIS]
           You may use this function to walk through the ilayers which are
           currently active in the `kdu_region_compositor' object.  The
           composited image consists of an ordered set of imagery layers
           (or "ilayers") starting from the top-most layer and working to the
           background.
           [//]
           The richest file format supported by the `kdu_region_compositor'
           is the JPX format and it is easy to think of ilayers as
           synonymous with JPX compositing layers.  However, there is
           an important difference.  A JPX compositing layer merely  provides
           imagery for composition, whereas an ilayer is an actual
           layer in the composition.  ilayers may be formed by cropping,
           scaling and re-orienting JPX compositing layers, and multiple
           active ilayers may actually use the same JPX compositing layer.
           For MJ2 data sources, the most natural association is between
           ilayers and video tracks, since the MJ2 file format envisages that
           a video presentation may be formed by compositing an ordered set
           of video tracks.  However, the ilayer concept is more general in
           that multiple ilayers may actually share the same video track,
           potentially with different frame numbers, source cropping, scaling
           or re-orientation parameters.
           [//]
           This funcion is closely related to `get_next_istream', which
           is used to walk through the available istreams, rather than ilayers.
           istreams provide the connection between ilayers and JPEG2000
           codestreams, but are necessarily distinct because an ilayer may
           actually draw its colour channels from multiple codestreams.
           Unlike `get_next_istream', the present function provides no
           means for including "inactive" imagery layers (see
           `cull_inactive_ilayers') in the list of returned entities.
         [RETURNS]
           Opaque reference which may be passed to many other member functions
           offered by `kdu_region_compositor' (e.g., `get_ilayer_info'), or
           used with the present function to advance to the next ilayer.
           If there are no more ilayers which match the constraints imposed
           by the last two arguments, the function returns a "null"
           reference (i.e., an instance of the `kdu_ilayer_ref' class whose
           `kdu_ilayer_ref::is_null' function returns true).
         [ARG: last_ilayer_ref]
           If `last_ilayer_ref.is_null' is true, the function finds the top-most
           ilayer that satisfies any constraints imposed by the last two
           arguments.  You can readily instantiate such a "null" instance of
           the `kdu_ilayer_ref' class using its default constructor.
           [//]
           Otherwise, the function looks for the top-most ilayer which satisfies
           the constraints imposed by the last two arguments and lies below
           the ilayer identified by `last_ilayer_ref'.  If none exists (or
           if `last_ilayer_ref' does not correspond to an existing ilayer,
           the function returns a "null" ilayer reference.
         [ARG: layer_src]
           If -ve, this argument imposes no restrictions on the type of
           ilayers which the function will return.  Otherwise, the function
           will only return ilayers which were created using `add_ilayer'
           (with this same `layer_src' value) or `set_frame'.  For JPX
           data sources, the `layer_src' value is interpreted as the
           zero-based compositing layer index.  For MJ2 data sources, the
           `layer_src' value is interpreted as the zero-based track index
           (i.e., 1 less than the MJ2 track index returned via
           `mj2_video_source::get_track_idx').  For a simple JP2 file or a
           raw codestream source, the only non-negative value for this
           argument which can match an ilayer is 0.
         [ARG: direct_codestream_idx]
           If -ve, this argument imposes no restrictions on the type of
           ilayers which the function will return.  Otherwise, the function
           will return only ilayers which were created to render codestream
           image components directly, as opposed to using file format
           colour/channel/palette information.  These are the ilayers created
           using the `add_primitive_ilayer' function.  In the special in
           which the data source is a raw codestream, a `direct_codestream_idx'
           value of 0 will match the ilayer created using either
           `add_ilayer' or `add_primitive_ilayer', because both functions
           create ilayers which render the codestream image components as-is.
           This is explained also with those functions.
       */
    KDU_AUX_EXPORT kdu_istream_ref
      get_next_istream(kdu_istream_ref last_istream_ref,
                       bool only_active_istreams=true,
                       bool no_duplicates=false,
                       int codestream_idx=-1);
      /* [SYNOPSIS]
           You may use this function to walk through the istreams which are
           currently available in the `kdu_region_compositor' object.  An
           "istream" (or "imagery stream") corresponds to a single JPEG2000
           codestream, within the context of a single imagery layer in the
           composition.  A single codestream may be used by multiple imagery
           layers, in which case each use corresponds to a distinct istream,
           even if the underlying codestream resource is actually shared
           between such distinct uses for efficiency.  A single imagery
           layer usually builds its imagery from a single istream, but in
           some cases multiple codestreams may be involved (JPX compositing
           layers, for example, can draw each of their colour/opacity channels
           from different codestreams).
           [//]
           If you are using the present function to enumerate all of the
           actual underlying codestreams which are in use, you may wish to
           set the `no_duplicates' argument to true.
           [//]
           The function can be used to walk through not only those istreams
           which are part of the current composition, but also those which
           are associated with inactive ilayers that have not yet been culled
           (see `cull_inactive_ilayers').  This latter type of istream is
           identified as "inactive".  Normally, you would not be interested
           in scanning through the inactive istreams, but you can add them
           into the mix by setting the `only_active_istreams' argument to false.
           [//]
           If you like, you can also narrow the search performed by this
           function, so that it only includes codestreams with the absolute
           index given by `codestream_idx'.  This absolute codestream index
           has exactly the same interpretation as in the `add_primitive_ilayer'
           function.
         [RETURNS]
           Opaque reference which may be passed to many other member functions
           offered by `kdu_region_compositor' (e.g., `get_istream_info'), or
           used with the present function to advance to the next istream.
           If there are no more istreams which match the constraints imposed
           by the `codestream_idx' argument, the function returns a "null"
           reference (i.e., an instance of the `kdu_istream_ref' class whose
           `kdu_istream_ref::is_null' function returns true).
         [ARG: last_istream_ref]
           If `last_istream_ref.is_null' is true, the function finds the first
           istream (in some sequence, that is consistent but not well defined)
           that satisfies any constraints imposed by the `codestream_idx'
           argument.  You can readily instantiate such a "null" instance of the
           `kdu_istream_ref' class using its default constructor.
           [//]
           Otherwise, the function looks for the first istream which satisfies
           the constraints imposed by `codestream_idx' and follows the istream
           identified by `last_istream_ref' (in the same order defined above).
           If none exists (or `last_istream_ref' does not correspond to an
           existing istream, the function returns a "null" istream reference.
         [ARG: only_active_istreams]
           If true, the function only returns istreams which are associated
           with a currently active ilayer.  Active ilayers are those which
           are involved in the current composition.  If you want to include
           those istreams which are associated with inactive ilayers (those
           that have not yet been culled by a call to `cull_inactive_ilayers'),
           set this argument to false.
         [ARG: no_duplicates]
           If true, the sequence of istreams visited by this function (with
           each successive call using the previous call's return for its
           `last_istream_ref' argument) excludes all but one istream which
           is associated with any given physical codestream resource
           internally.  This can be useful if you wish to gather statistics
           regarding the physical codestream resources currently being managed.
         [ARG: codestream_idx]
           If -ve, this argument does not restrict the set of istreams which
           can be returned.  Otherwise, the function limits its search to
           istreams which have the indicated absolute codestream index.  If
           the data source for this object is a raw codestream, the only
           codestream index which can exist is 0.  If the data source is
           a `jpx_source' object (handles JP2 and JPX files), the codestream
           index is the positional index (starting from 0) of the codestream
           within the source, as returned by `jpx_layer_source::get_layer_id'.
           If the data source is an `mj2_source' object (handles MJ2 files),
           the codestream index is the unique index whose interpretation is
           described in connection with the `mj2_video_source::get_stream_idx'
           function.
      */
    KDU_AUX_EXPORT kdu_ilayer_ref
      get_next_visible_ilayer(kdu_ilayer_ref last_ilayer_ref, kdu_dims region);
      /* [SYNOPSIS]
           Similar to `get_next_ilayer', you may use this function to
           walk through the various ilayers which are currently open
           within the object.  In this case, however, only those codestreams
           which contribute to the reconstruction of the supplied `region'
           are visited and returned.
           [//]
           If no scale has been successfully installed using
           `set_scale' (i.e., if `get_total_composition_dims' returns false),
           no ilayer can possibly be visible within the supplied
           `region', since a rendering coordinate system has not yet been
           established -- in such a case, the function must return a
           "null" instance of the `kdu_ilayer_ref' class.
         [RETURNS]
           Opaque ilayer reference which may be passed to various other
           member functions, including `get_ilayer_info'.
         [ARG: last_ilayer_ref]
           If `last_ilayer_ref.is_null' is true, the function finds the top-most
           ilayer that is visible within `region'.  You can readily instantiate
           such a "null" instance of the `kdu_ilayer_ref' class using its
           default constructor.
           [//]
           Otherwise, the function looks for the top-most ilayer which 
           lies below the ilayer identified by `last_ilayer_ref' and is
           visible within `region'.  If none exists (or
           if `last_ilayer_ref' does not correspond to an existing ilayer,
           the function returns a "null" ilayer reference.
         [ARG: region]
           Region within which visibility is to be assessed, described
           with respect to the same rendering coordinate system as that used
           by `set_buffer_surface'.
      */
    KDU_AUX_EXPORT kdu_codestream
      access_codestream(kdu_istream_ref istream_ref);
      /* [SYNOPSIS]
           Use this function with the `kdu_istream_ref' instance returned by a
           previous call to `get_next_istream' or any other member function
           which provides references to istreams.  The function returns an
           interface to the actual JPEG2000 code-stream being used by the
           istream.
           [//]
           Note that this function automatically halts any internal
           processing which is using the identified codestream, so that you
           will be free to change its appearance via the
           `kdu_codestream::apply_input_restrictions' and/or
           `kdu_codestream::change_appearance' functions.  When you receive
           the codestream, however, its appearance is in an unknown state, so
           any operations you wish to perform which depend upon the codestream
           appearance should be preceded by calls to the above-mentioned
           functions.
           [//]
           As a general rule of thumb, you should not continue to access the
           codestream interface returned by this function after subsequent
           invocation of any of the `kdu_region_compositor' object's
           interface functions.  This is by far the safest policy.  In
           particular, you should definitely refrain from calling the
           `process' or `set_scale' functions while continuing to use the
           returned codestream interface.
           [//]
           Since frequent halting and restarting of internal processing can
           be computationally expensive, it is best to avoid calling this
           function too frequently, or to do so only when the object is in
           the idle state (as indicated by the `is_processing_complete'
           function).
           [//]
           If `istream_ref' is not a valid reference to an istream currently
           managed by the `kdu_region_compositor', the function returns an
           empty codestream interface -- i.e., one whose
           `kdu_codestream::exists' member function returns false.
      */
    KDU_AUX_EXPORT int
      get_istream_info(kdu_istream_ref istream_ref, int &codestream_idx,
                       kdu_ilayer_ref *ilayer_ref=NULL,
                       int components_in_use[]=NULL,
                       int max_components_in_use=4,
                       int *principle_component_idx=NULL,
                       float *principle_component_scale_x=NULL,
                       float *principle_component_scale_y=NULL,
                       bool *transpose=NULL, bool *vflip=NULL,
                       bool *hflip=NULL);
      /* [SYNOPSIS]
           Use this function with the `kdu_istream_ref' instance returned by
           a previous call to `get_next_istream', or any other function which
           returns an istream reference, to obtain information
           concerning the associated codestream, within the context of its
           containing imagery layer.
         [RETURNS]
           0 if `istream_ref' fails to identify any valid istream, or if it
           identifies an istream which is not currently active, in the sense
           identified by `get_next_istream'.  The only thing you can do with
           inactive istreams is invoke `access_codestream' to learn about the
           inactive codestream resource.
           [//]
           Otherwise, the function returns an integer N (or -N -- see below),
           where N is the number of distinct image components from this
           codestream which are used by the istream.  In the special case
           where the underlying istream is being accessed directly (must have
           been created using `add_primitive_ilayer') and its image components
           are being rendered directly as "codestream components", as opposed
           to "output image components", this function returns a -ve integer
           -N. Output image components are those obtained after application
           of any inverse multi-component transform, corresponding to a
           `kdu_component_access_mode' value of `KDU_WANT_OUTPUT_COMPONENTS',
           as passed to `add_primitive_ilayer', while codestream image
           components correspond to the value `KDU_WANT_CODESTREAM_COMPONENTS'.
           In most cases, applications will be using output image components.
         [ARG: istream_ref]
           If this is a "null" instance or is not found to correspond to any
           active istream, the function returns false and does not modify any
           of the arguments.
         [ARG: codestream_idx]
           Used to return the absolute index of the JPEG2000 code-stream
           associated with this layer codestream, as it appears within
           its data source.
         [ARG: ilayer_ref]
           Used to return an instance of the `ilayer_ref' class which
           identifies the imagery layer to which this istream belongs.
           Each and every istream is associated with exactly one ilayer.
           You can discover more information about the istream by querying
           its ilayer via the `get_ilayer_info' function.
         [ARG: components_in_use]
           If non-NULL, this argument must point to an array with at least
           `max_components_in_use' entries.  The array's entries are filled
           out with the the zero-based indices of the image components
           which are actually being used within this code-stream.
           [//]
           If the function returns a positive integer N, the first N array
           entries will be filled out with N distinct non-negative integers,
           identifying the indices of the output image components which are
           in use -- note, however, that at most `max_components_in_use'
           etries will actually be written, even if N is larger.  If N is
           smaller than `max_components_in_use', the last
           `max_components_in_use'-N entries in the array will be set to -1.
           [//]
           If the function returns a negative integer -N, the first N array
           entries will be filled out with N distinct non-negative integers,
           identifying the indices of the codestream image components which
           are in use.  Again, the number of written entries will not exceed
           `max_components_in_use' and if N is smaller than
           `max_components_in_use', the final `max_components_in_use'-N entries
           will be set to -1.
           [//]
           To understand the distinction between output components and
           codestream components (the ones produced by inverse spatial
           wavelet transformation), see the description of the
           `kdu_codestream::apply_input_restrictions' function.  It suffices
           here to note that the set of output components
           which are being used may be larger or smaller than the corresponding
           set of codestream image components.  If `add_primitive_ilayer' was
           used to create this istream and it was supplied with an `access_mode'
           argument of `KDU_WANT_CODESTREAM_COMPONENTS', the imagery is
           rendered directly using one or more codestream image components;
           otherwise, the components in question are output image components.
         [ARG: max_components_in_use]
           Provides the size of the array supplied to the `components_in_use'
           argument.  The current implementation will never need a value greater
           than 4 to return all relevant information, since at most 3 colour
           channels and 1 opacity channel are employed.  However, in the
           future, it is possible that separate opacity channels might be
           supported.
         [ARG: principle_component_idx]
           If non-NULL, this argument is used to return the index of the
           image component of this codestream whose dimensions form the
           basis of scaling decisions for rendering purposes.  If there
           are multiple components being used, this will generally be the
           component which is used to generate the first colour channel
           supplied to any colour mapping process.  The interpretation of
           this component index is identical to that of the component indices
           returned via the `components_in_use' argument (depends on whether
           the function return value is +ve or -ve).
         [ARG: principle_component_scale_x]
           If non-NULL, this argument is used to return the amount by which
           the principle image component is horizontally scaled to obtain
           its contributions to the composited result, assuming that
           `set_scale' is called with a `scale' value of 1.0.  The actual
           component scaling factor can be deduced by multiplying the
           returned value by the global scaling factor supplied to
           `set_scale'.  The value returned here does not rely upon a
           successful call to `set_scale' ever having been made and is
           independent of any geometric transformations requested in a
           call to `set_scale'.  Thus, if you are really rendering
           (or intending to render) the surface in a transposed fashion,
           you should swap the horizontal and vertical scales returned
           by this function in order to obtain scaling factors which are
           appropriate for your intended rendering orientation.
         [ARG: principle_component_scale_y]
           Same as `principle_component_scale_x', but used to return
           the vertical scaling factor associated with the principle
           image component.
         [ARG: transpose]
           As for `principle_component_scale_x', this argument (if non-NULL) is
           used to return an indication of whether the codestream would be
           subjected to transposition, in the event that `scale' is called
           without any additional transposition or flipping -- i.e., nominal
           scale and geometry.  This is the same value which was passed in for
           the `transpose' argument of `add_ilayer' if the istream and its
           ilayer were created directly using that function.
         [ARG: vflip]
           As for `transpose', but this argument (if non-NULL) returns
           information about vertical flipping at the nominal composition
           scale and geometry -- this is the same value which was passed in for
           the `vflip' argument to `add_ilayer' if the istream and its ilayer
           were created directly using that function.
         [ARG: hflip]
           As for `transpose', but this argument (if non-NULL) returns
           information about horizontal flipping at the nominal composition
           scale and geometry -- this is the same value which was passed in for
           the `hflip' argument to `add_ilayer' if the istream and its ilayer
           were created directly using that function.
      */
    KDU_AUX_EXPORT int
      get_ilayer_info(kdu_ilayer_ref ilayer_ref, int &layer_src,
                      int &direct_codestream_idx, bool &is_opaque,
                      int *frame_idx=NULL, int *field_handling=NULL);
      /* [SYNOPSIS]
           Use this function, together with `get_ilayer_stream' to return
           information about a specific imagery layer, as obtained via
           `get_next_ilayer' or any other function provided by this object
           which provides imagery layer references.
         [RETURNS]
           The number of distinct istreams which are employed by this ilayer,
           or else 0 if `ilayer_ref' does not correspond to any ilayer which
           forms part of the current composition.  If the return value is S
           you can retrieve references to each of the S istreams by passing
           indices 0 through S-1 to `get_ilayer_istream'.
         [ARG: ilayer_ref]
           Identifies the imagery layer in question.
         [ARG: layer_src]
           Used to return the identity of the JPX compositing layer or MJ2
           video track which was used to form this ilayer (or zero if the
           `kdu_region_compositor' object's data source is a raw codestream).
           This value is identical to the `layer_src' argument supplied in the
           relevant call (if any) to `add_ilayer'.
           [//]
           The value returned by this argument is -1 if and only if the ilayer
           was created using `add_primitive_ilayer', except in the event that
           the data source is a raw codestream `add_primtive_ilayer' was
           invoked in a manner which is equivalent to using `add_ilayer'
           (see that function for an explanation of this special case).
         [ARG: direct_codestream_idx]
           Used to return the absolute index of a specific codestream which
           this ilayer is rendering directly, without the use of
           colour/channel/palette information from a surrounding file format.
           [//]
           Returns -1 unless `add_primitive_ilayer' was used to create the
           ilayer, or `add_ilayer' was used with a raw codestream data source
           (since in this special case both functions create ilayers which
           render codestream data without surrounding file-format supplied
           rendering instructions).
         [ARG: is_opaque]
           Returns true if this ilayer has no opacity information.
         [ARG: frame_idx]
           If non-NULL, this argument is used to return the zero-based frame
           index associated with the ilayer -- always 0 unless the data source
           is MJ2.
         [ARG: field_handling]
           If non-NULL, this argument is used to return the field handling
           mode associated with the ilayer, as supplied in the original call
           to `add_ilayer' -- always 0 if the data source is not MJ2.
      */
    KDU_AUX_EXPORT kdu_istream_ref
      get_ilayer_stream(kdu_ilayer_ref ilayer_ref, int which,
                        int codestream_idx=-1);
      /* [SYNOPSIS]
           Use this function to obtain references to the istreams which are
           associated with an ilayer.  You can use the `which' argument to
           enumerate the istreams, starting from the ilayer's primary stream
           at `which'=0.  Alternatively, by supplying a -ve value for `which',
           you can access the istream (if any) which has a specific absolute
           codestream index.
         [RETURNS]
           A "null" istreams reference (one whose `kdu_istream_ref::is_null'
           function returns true) unless `ilayer_ref' identifies an ilayer
           which is part of the current composition and either:
           1) `which' lies in the range 0 to S-1 where S is the value returned
           by `get_ilayer_info'; or 2) `which' is -ve and `codestream_idx'
           identifies the absolute index of one of the codestreams which is
           used by the ilayer.
         [ARG: which]
           The `which' argument can take on values of 0 through S-1 where S
           is the number of istreams returned by `get_ilayer_info'.  The
           first istream (corresponding to `which'=0) is considered the
           layer's primary istream.  If `which' is -ve, the `codestream_idx'
           argument is used to locate the codestream of interest.
         [ARG: codestream_idx]
           Ignored unless `which' < 0, in which case the function looks for
           an istream which belongs to the imagery layer identified by
           `ilayer_ref' and has the indicated absolute codestream index.
      */
    KDU_AUX_EXPORT bool
      get_codestream_packets(kdu_istream_ref istream_ref,
                    kdu_dims region, kdu_long &visible_precinct_samples,
                    kdu_long &visible_packet_samples,
                    kdu_long &max_visible_packet_samples);
      /* [SYNOPSIS]
           It is best to wait until all codestream processing is complete
           before invoking this function, since it will halt any current
           processing which uses the codestream -- frequently halting and
           restarting the processing within a codestream can cause
           considerable computational overhead.  Consider using the
           `is_codestream_processing_complete' function to determine whether
           or not invoking this function will cause such inefficiencies.
           [//]
           This function may be used to discover the degree to which
           codestream packets which are relevant to the visible portion
           of `region' are available for decompression.  This information, in
           turn, may be used as a measure of the amount of relevant information
           which has been loaded into a dynamic cache, during remote browsing
           with JPIP, for example.  To obtain this information, the
           function uses `kdu_resolution::get_precinct_samples' and
           `kdu_resolution::get_precinct_packets',scanning the precincts
           which are relevant to the supplied `region' according to their
           visible area.  The `region' argument is expressed with the same
           rendering coordinate system as that associated with
           `get_composition_buffer', but the sample counts returned by the
           last three arguments represent relevant actual JPEG2000 subband
           samples.  Samples produced by the codestream
           are said to be visible if they are not covered by any opaque
           composition layer which is closer to the foreground.  A foreground
           layer is opaque if it has no alpha blending channel.
           [//]
           The value returned via `visible_precinct_samples' is intended
           to represent the total number of subband samples which
           contribute to the reconstruction of any visible samples within
           `region'.  While it will normally hold exactly this value, you
           should note that some samples may be counted multiple times if
           there are partially covering foreground compositing layers.  This
           is because the function internally segments the visible portion of
           `region' into a collection of disjoint rectangles (this is always
           possible) and then figures out the sample counts for each region
           separately, adding the results.  Since the DWT is expansive,
           wherever more than one adjacent rectangle is required to cover the
           region, some samples will be counted more than once.
           [//]
           The value returned via `visible_packet_samples' is similar to
           that returned via `visible_precinct_samples', except that each
           subband sample, n, which contributes to the visible portion of
           `region', contributes Pn to the `visible_packet_samples' count,
           where Pn is the number of packets which are currently available
           for the precinct to which it belongs, from the compressed data
           source.  This value is recovered using
           `kdu_resolution::get_precinct_packets'.
           [//]
           The value returned via `max_visible_packet_samples' is similar to
           that returned via `visible_precinct_samples', except that each
           subband sample, n, which contributes to the visible portion of
           `region', contributes Ln to the `visible_packet_samples' count,
           where Ln is the maximum number of packets which could potentially
           become available for the precinct to which it belongs.  This
           value is recovered using `kdu_tile::get_num_layers'.
           [//]
           Where samples are counted multiple times (as described
           above), they are counted multiple times in the computation of
           all three sample counters, so that the ratio between
           `visible_packet_samples' and `max_visible_packet_samples' will be
           1 if and only if all possible packets are currently available for
           all precincts containing subband samples which are involved in the
           reconstruction of the visible portion of `region'.
         [RETURNS]
           False if the istream identified by `istream_ref' makes no visible
           contribution to `region'.
         [ARG: istream_ref]
           Reference obtained by `get_next_istream', `get_ilayer_stream',
           or any of the other functions offered by `kdu_region_compositor'
           which can provide istream references.
         [ARG: region]
           Region of interest, expressed within the same rendering coordinate
           system as that used by `set_buffer_surface'.
      */
    KDU_AUX_EXPORT kdu_ilayer_ref
      find_point(kdu_coords point, int enumerator=0,
                 float visibility_threshold=-1.0F);
      /* [SYNOPSIS]
           This function can be used to enumerate all the imagery layers
           in which `point' is contained and visible with respect to the
           `visibility_threshold'.  If `enumerator' is 0, the function
           returns information for the top-most such ilayer.  In general,
           the function skips over the top-most `enumerator' such ilayers and
           returns the next one, if there is one.  If so, the function returns
           a non-null instance of the `kdu_ilayer_ref' class to identify the
           relevant ilayer; otherwise the returned object is a "null" reference,
           meaning that its `kdu_ilayer_ref::is_null' member returns true.
           [//]
           Note that the presence of overlaying imagery layers tends
           to render the `point' less visible (if not invisible) in an
           underlying ilayer so that the `visibility_threshold' might not be
           satisfied.  The `point' is also less visible within an imagery
           layer if that ilayer has low opacity at the location in question.
           In general, the visibility of `point' within an ilayer L is taken
           to be V = O_L * Prod_{l < L} (1-O_L), where O_l denotes the opacity
           of ilayer l at location `point' (normalized to a maximum opacity of
           1) and the set {l < L} denotes ilayers which lie above ilayer L.
           With this definition in mind, V lies in the range 0 to 1.  If
           V > `visibility_threshold', the visibility threshold is deemed
           to be satisfied.  If an ilayer has not yet been completely rendered,
           so that the opacity at `point' is unknown, the opacity will
           usually be taken to be 1.0 -- there may be exceptions if you
           are rendering without buffer initialization (i.e., if
           `set_buffer_surface_initialization_mode' has been called with
           false as the argument).
         [ARG: point]
           Location of a point on the compositing grid, expressed using the
           same coordinate system as that associated with `set_buffer_surface'.
         [ARG: enumerator]
           You can use this argument to walk through the matching compositing
           layers from top-most to bottom-most, until there are no more.
         [ARG: visibility_threshold]
           As mentioned above, the visibility V at location `point' must
           be greater than `visibility_threshold' for `point' to
           be considered visible in a layer.  V is equal to the normalized
           opacity within that layer, multiplied by product of 1 minus the
           normalized opacity of all covering layers.
      */
    KDU_AUX_EXPORT kdu_istream_ref
      map_region(kdu_dims &region, kdu_istream_ref istream_ref);
      /* [SYNOPSIS]
           This function attempts to map the region found, on entry, in the
           `region' argument to the high resolution grid of some codestream.
           [//]
           If `istream_ref' is a "null" istream reference, the function
           first finds the uppermost imagery layer whose contents are visible
           within the supplied region; if one is found, the function returns
           a reference to that ilayer's primary istream and the region is
           mapped onto the corresponding codestream's high resolution canvas.
           [//]
           If `istream_ref' is a valid reference to an existing istream,
           the function returns the same `istream_ref' value after mapping
           `region' onto the relevant high resolution codestream canvas.
           In this second case, the mapping is performed regardless of
           whether the imagery layer to which the istream belongs is visible
           within he supplied `region'.
           [//]
           Note that the `region' supplied on entry is expressed within the
           same coordinate system as that supplied to `set_buffer_surface',
           while the region returned on exit is expressed on the codestream's
           high resolution canvas, after undoing the effects of rotation,
           flipping and scaling, which depend upon the imagery layer to
           which the istream belongs.  The returned region is offset so that
           its location is expressed relative to the upper left hand corner of
           the codestream's image region, which is not necessarily
           the canvas origin.  This is exactly the way in which
           JPX ROI description boxes express regions of interest.
           [//]
           The region mapping process is ultimately achieved by mapping the
           upper left and lower right corner points in `region' to their
           corresponding locations on the high resolution codestream canvas
           (using `kdu_region_decompressor::find_codestream_point') and using
           these mapped points to delineate the mapped region.  This
           approach ensures that a non-empty input `region' will be mapped
           to a non-empty output `region'.  It also means that you can use
           this function to map individual points from the compositing grid
           to any desired codestream's high resolution grid -- when doing so,
           the rounding conventions that are used will be those of
           `kdu_region_decompressor::find_codesream_point'.
         [RETURNS]
           If `istream_ref' is a non-null reference to an active istream
           and the supplied `region' is non-empty and `set_scale' has been
           successfully used to install a valid rendering scale and geometry,
           the function always performs the mapping of `region' and the return
           value will be identical to `istream_ref'.
           [//]
           If `istream_ref.is_null' is true and the function discovers an
           imagery layer which is visible inside `region', it returns a
           reference to its primary istream and performs the mapping of
           `region'.
           [//]
           Otherwise, no mapping is performed and the function returns a "null"
           `kdu_istream_ref' instance.
         [ARG: istream_ref]
           If you want the function to find the most appropriate imagery layer
           for you automatically (along with its primary istream), pass a
           "null" reference for this argument (i.e., one for which
           `istream_ref.is_null' returns true).  If you want the mapping
           performed for a specific istream (i.e., a specific codestream, as
           used in a specific imagery layer), you can supply its identifying
           reference here.
      */
    KDU_AUX_EXPORT kdu_dims
      inverse_map_region(kdu_dims region, kdu_istream_ref istream_ref);
      /* [SYNOPSIS]
           This function essentially performs the inverse mapping of
           `map_region'.  The `region' argument identifies a region on
           the identified codestream's high resolution grid, except that
           the location of the region is expressed relative to the upper
           left hand corner of the codestream's image region on that grid
           (usually, but not necessarily the same as the canvas origin).
           [//]
           The returned region idenifies the location and dimensions
           of the original region, as it appears within the coordinate
           system of the composited image, as used by `set_buffer_surface'.
           [//]
           In the special case where `istream_ref' is a "null" istream
           reference (i.e., `istream_ref.is_null' returns true), the `region'
           is interpreted as a region on the current composited surface,
           corresponding to a rendering scale of 1.0 and with the original
           geometry.  In this case, the function corrects only for rendering
           scale and geometry flags (flips and transposition).
           [//]
           Note that the interpretation of regions (particularly for up-scaled
           compositions) was a little loose prior to Kakadu version 6.4 and
           has since been fixed.  The returned region is now evaluated using
           `kdu_region_decompressor::find_render_cover_dims', so as to find
           the tightest region on the composited surface which covers all
           locations to which codestream locations inside `region' are
           mapped during the painting of metadata overlays.
         [RETURNS]
           An empty region if `istream_ref' is neither a "null" reference nor
           a reference to any istream involved in the current composition.
           An empty region is also returned if valid scale/orientation
           information has not yet been installed via a successful call to
           `set_scale'.
           [//]
           Otherwise, the returned region is guaranteed to be non-empty so
           long as `region' was non-empty.  This is because the machinery used
           to map regions is `kdu_region_decompressor::find_render_cover_dims'.
           [//]
           If the `region' argument contains exactly one point, the returned
           region is also guaranteed to contain exactly one point, which is
           effectively that point which would be returned via
           `kdu_region_decompressor::find_render_point' (with appropriate
           adjustments to account for the composition process).  This can
           be a useful feature.
         [ARG: region]
           If empty, the region is interpreted as the relevant codestream's
           entire image region (or the entire composition if `istream_ref'
           is a "null" instance of the `kdu_istream_ref' class.
         [ARG: istream_ref]
           Identifies the istream for which you want the mapping to be
           performed.  In the special case where this is a "null" istream
           reference, the mapping will be performed at the composition
           (rather than codestream) level, which corresponds to the kind of
           mapping you would want to perform for a JPX ROI description box
           which is not associated with any specific codestream.
      */
    KDU_AUX_EXPORT kdu_dims
      find_ilayer_region(kdu_ilayer_ref ilayer_ref, bool apply_cropping);
      /* [SYNOPSIS]
           Returns the location and dimensions of the indicated imagery
           layer, as it appears on the composited image.  The returned
           dimensions are expressed using the same coordinate system as that
           associated with `set_buffer_surface'.
         [RETURNS]
           An empty region if `ilayer_ref' does not identify an imagery layer
           in the current composition.
         [ARG: ilayer_ref]
           You may obtain a valid `ilayer_ref' instance from `get_next_ilayer'
           or any of a range of other functions offered by the
           `kdu_region_compositor' object.
         [ARG: apply_cropping]
           If true, the returned region corresponds to the portion of the
           underlying codestream image which actually contributes to the
           imagery layer.  Note, however, that the imagery may have been
           cropped before being placed on the compositing surface.  If this
           argument is false, the function returns the size of the complete
           underlying codestream, along with the location of its upper left
           hand corner, as if the cropping had not been performed, but all
           scaling and geometric adjustments are as before.  In this case, the
           returned region need not lie wholly within the composited image
           region returned by `get_total_composition_dims'.
      */
    KDU_AUX_EXPORT kdu_dims
      find_istream_region(kdu_istream_ref istream_ref, bool apply_cropping);
      /* [SYNOPSIS]
           Returns the location and dimensions of the indicated istream,
           as it appears on the composited image.  The returned
           dimensions are expressed using the same coordinate system as that
           associated with `set_buffer_surface'.
         [RETURNS]
           An empty region if `istream_ref' does not identify an active
           istream in the current composition.
         [ARG: istream_ref]
           You may obtain a valid `istream_ref' instance from `get_next_istream'
           or any of a range of other functions offered by the
           `kdu_region_compositor' object.
         [ARG: apply_cropping]
           If true, the returned region corresponds to the portion of the
           underlying codestream image which actually contributes to the
           imagery layer.  Note, however, that the imagery may have been
           cropped before being placed on the compositing surface.  If this
           argument is false, the function returns the size of the complete
           underlying codestream, along with the location of its upper left
           hand corner, as if the cropping had not been performed, but all
           scaling and geometric adjustments are as before.  In this case, the
           returned region need not lie wholly within the composited image
           region returned by `get_total_composition_dims'.
      */
    KDU_AUX_EXPORT bool
      find_compatible_jpip_window(kdu_coords &fsiz, kdu_dims &roi_dims,
                                  int &round_direction, kdu_dims region);
      /* [SYNOPSIS]
           This function is provided to simplify the construction of
           JPIP window-of-interest requests which are compatible with the
           the current composition configuration.
           [//]
           If there is insufficient information available to determine the
           composited frame dimensions -- i.e., if `get_total_composition_dims'
           would fail -- the function returns false and does nothing.
           [//]
           Otherwise, the function attempts to determine the most appropriate
           JPIP window-of-interest request to reflect the current scale
           and the supplied `region'.  The window of interest is returned via
           the first 3 arguments.  If `fsiz' argument is used to return
           the size of the full image which should be used in a JPIP request.
           This, together with `round_direction', affect the image resolution
           that a JPIP server deems to be contained within the request.  In
           the simplest case, `fsiz' can be equal to the dimensions returned
           by `get_total_composition_dims' and `round_direction' can be 0,
           meaning "round-to-nearest".  However, one or more codestreams
           are being sub-sampled as part of the rendering process, the
           `round_direction' may be set to 1, meaning "round-up", and the
           `fsiz' dimensions are adjusted so that the round-up policy does
           not cause redundant information to be included in the JPIP server's
           response.  The function's behaviour is most interesting when
           a composited frame is being formed from multiple codestreams,
           some of which may be undergoing sub-sampling (or even up-sampling)
           by various amounts in each direction.  For such cases, the `fsiz'
           and `round_direction' values need to be selected with some care so
           as to allow a JPIP server to respond to the relevant
           codestream-context request with all relevant data and as little
           redundant data as possible.
           [//]
           If `region' is empty, `roi_dims' will be set to refer to the entire
           image region whose dimensions are returned via `fsiz'.  Otherwise,
           `region' is interpreted with respect to the composited image
           coordinate system (as used by `get_total_composition_dims' and
           `set_buffer_surface') and converted into a corresponding region,
           expressed with respect to `fsiz'.
           [//]
           Note that the region supplied in any recent call to
           `set_buffer_surface' is irrelevant to the behaviour of this
           function.  However, `region' may well be a region of interest
           that we have passed to (or intend to pass to) `set_buffer_surface'.
           [//]
           It is worth noting that when this function is used to construct
           JPIP requests, the `get_codestream_packets' function returns
           statistics for the same content that is being requested of a JPIP
           server.  This means that the completion of the server's response
           should coincide with the event that the `visible_packet_samples'
           and `max_visible_packet_samples' values returned by the
           `get_codestream_packets' function are identical for all visible
           codestreams -- assuming the same `region' is supplied to that
           function.
           [//]
           You should also note that the information returned by this function
           may not be stable until the `waiting_for_stream_headers' function
           returns false.  You should generally issue a call to `refresh'
           after each receipt of new information from a JPIP server, until
           that function returns false, updating your JPIP requests based upon
           potentially new information returned by this function.
         [RETURNS]
           True if sufficient information has been configured for a meaningful
           window of interest to be returned via `fsiz', `roi_dims' and
           `round_direction'.
         [ARG: fsiz]
           Used to return the full size of the requested resolution, to be
           supplied as the "fsiz" JPIP request field.  This may or may not be
           the same as the full composited image dimensions, as returned by
           `get_total_composition_dims'.  It is also expressed using the
           original image geometry, which may be different from that associated
           with the composited image coordinate system, if `set_scale' was
           asked to transpose the image.
         [ARG: roi_dims]
           Used to return the origin and size of a region of interest within
           the image, to be supplied via the "roff" and "rsiz" JPIP request
           fields.  This is derived from `region' (unless `region' is empty,
           in which case `roi_dims' is set to refer to the entire image).
           Note, however, that `region' is expressed using a potentially
           different geometry (as supplied to `set_scale') and a potentially
           different scale (if `fsiz' does not have the same dimensions as
           the composited image).
         [ARG: round_direction]
           Used to return the rounding direction, to be supplied along with
           `fsiz' in the "fsiz" JPIP request field.  A value of 0 means
           "round-to-nearest", while 1 means "round-up".  No other values will
           be used.
         [ARG: region]
           Either an empty region (see above) or else a region of interest,
           expressed on the composited image coordinate system.  This may or
           may not be the region supplied in a current or future call to
           `set_buffer_surface'.
      */
  //----------------------------------------------------------------------------
  public: // Functions used to manage metadata overlays
    KDU_AUX_EXPORT void
      configure_overlays(bool enable, int min_display_size=8,
                         float blending_factor=1.0F, int max_painting_border=5,
                         jpx_metanode dependency=jpx_metanode(),
                         int dependency_effect=0,
                         const kdu_uint32 *aux_params=NULL,
                         int num_aux_params=0);
      /* [SYNOPSIS]
           Use this function to enable or disable the rendering of spatially
           sensitive metadata as overlay information on the compositing
           surface.  When enabled, overlay information is generated as
           required during calls to `process' and folded into the composited
           result.  If overlay surfaces were previously active, disabling
           the overlay functionality may cause the next call to `process'
           to return a `new_region' which is the size of the entire composited
           image, so that your application will know that it needs to repaint
           the whole thing.
           [//]
           Overlays are available only when working with JPX data sources.
           [//]
           The `dependency' and `dependency_effect' arguments allow you to
           restrict overlay information to just those regions of interest
           which are related (or not related) to one or more specific entities
           in the metadata hierarchy.  See below for a detailed
           description of how such relationships are evaluated and how
           `dependency' metanodes supplied to this function is used to
           build up internal dependency expressions.
           [//]
           The `blending_factor' argument allows you to control the visibility
           of overlays in a manner which does not require the overlay content
           to be regenerated, facilitating the efficient implementation of
           some very useful visual effects.
           [//]
           Most of the parameters passed in here are passed along to the
           `custom_paint_overlay' function, which may be overridden to
           provide custom painting of overlays.  By default, however, that
           function does nothing and the internal `paint_overlay' function
           is invoked, receiving the same parameters.  Their interpretation
           is discussed below.
         [ARG: enable]
           If true, overlays are enabled (or remain enabled).  If false,
           overlays are disabled and the remaining arguments are ignored.
         [ARG: min_display_size]
           Specifies the minimum size occupied by the overlay's bounding
           box on the composited surface, at the prevailing scale.  If an
           overlay has a bounding box which will be rendered with both
           dimensions smaller than `min_display_size', it will not be
           rendered at all.  Setting a modest value for this parameter, say
           in the range 4 to 8, helps reduce clutter when viewing an image
           with a lot of metadata.  On the other hand, this can make it hard
           to locate tiny regions of significance when zoomed out to a small
           scale.  This parameter is handled outside of `paint_overlay' and
           `custom_paint_overlay' -- if you change it, the internal machinery
           may determine that some regions of the composited image surface
           need to be regenerated.
         [ARG: blending_factor]
           This argument is used to scale the alpha channel of any overlay
           buffer before blending the overlay with the underlying imagery.
           Any blending factor may be used, including factors
           which are much larger than 1, since the alpha values are clipped
           after scaling.  One implication of this is that large
           blending factors can render barely visible portions of a painted
           overlay in stark relief.  The multiplication itself is performed
           on-the-fly during composition, without actually affecting the
           overlay buffer contents.  This means that you can
           modulate the `blending_factor' without having to regenerate any
           metadata overlay content.  After changing this parameter,
           subsequent calls to `process' will generally recompose all regions
           in which metadata overlay content exists.
           [//]
           Negative blending factors are also permitted and can prove very
           useful.  If the blending factor is negative, alpha values are
           multiplied by the absolute value of the blending factor and clipped
           (as above), but the colour channels of the overlay pixels are
           inverted.  For 32-bit/pixel buffer representations, this means
           that red R becomes 255-R, green G becomes 255-G and blue B becomes
           255-B.  For higher precision buffer organizations, the principle
           is applied in the most natural way (so, for floating point colour
           channels in the range 0 to 1, red R becomes 1-R, green G becomes
           1-G and blue B becomes 1-B, etc.).  All of this is done on-the-fly
           during rendering.
           [//]
           By supplying 0 for this parameter, you can temporarily turn off
           overlays, while retaining the overlay buffers and metadata content.
           This ensures that overlays can be quickly and efficiently retrieved
           in the future, while also ensuring that the `search_overlays'
           function returns the same results that it would if overlay
           information were being painted.
           [//]
           Note that this parameter is not actually passed along to the
           `paint_overlay' and `custom_paint_overlays' function, since its
           effect is handled through the way in which painted overlay
           content is blended onto the composited image surface.
         [ARG: max_painting_border]
           This parameter determines the maximum extent by which overlay
           information may be painted beyond the region occupied by a JPX
           region of interest node.  The painting border is measured in
           pixels on the compositing reference grid and extends in all
           directions away from the ROI node to be painted.  The parameter
           is passed to `paint_overlay' and `custom_paint_overlay' via a
           `kdu_overlay_params' object, but there is no obligation for the
           overlay painting implementation to actually use a border of this
           size -- so long as this border size is not exceeded.  The
           `max_painting_border' is actually used to determine the region which
           may be affected by the painting of overlay information for a
           region of interest, so that the painting procedure can be invoked
           correctly.
           [//]
           Each time you call this function with a different value for the
           `max_painting_border' argument, all active metadata is discarded and
           must be rediscovered (and repainted) from scratch.  This can
           potentially be a costly exercise, so if you want to implement a
           scheme which modulates the border size associated with overlay
           content, it would be more efficient to leave the value of
           `max_painting_border' fixed and instead use the `aux_params' to
           modulate the actual border size, within the bounds established by
           `max_painting_border'.
         [ARG: dependency]
           This argument is used in conjunction with `dependency_effect' to
           build up a sum-of-products expression which is tested to determine
           which regions of interest should be revealed within the metadata
           overlay.  Each term in the sum-of-products expression involves a
           test between the region of interest (ROI) in question and a single
           `dependency' metanode.  The test is performed by invoking the
           powerful `jpx_metanode::find_path_to' function to see if a path
           can be found from the ROI description metanode to the `dependency'
           metanode.  The `jpx_metanode::find_path_to' function is configured
           to examine paths whose initial descending component includes
           descendants and "reverse alternate parent links", whose
           final ascending component includes parents and "reverse alternate
           child links", and whose path nodes may not belong to any other
           region of interest.  In this way, the paths which are examined are
           those which can make the ROI visible to the `dependency' metanode by
           following descendants and alternate child links downwards and parent
           or alternate parent links upwards.  This is a semantically
           meaningful way to identify an ROI as being of interest to the
           `dependency' node, while excluding paths which pass through other
           (inherently more relevant) regions of interest.
         [ARG: dependency_effect]
           This argument is used to cancel and/or build up a sum-of-products
           expression which is evaluated for each candidate region of interest
           to determine whether or not it should be rendered to the overlay
           plane.  The possible values are as follows:
           [>>] 0 erases the internal sum-of-products expression if
                `dependency' is an empty interface; otherwise creates a new
                sum-of-products expression consisting of the single
                condition "Is Related to `dependency'".
           [>>] 1 adds a new "OR" term to the sum-of-products expression being
                constructed internally, with the condition "Is Related to
                `dependency'".  Does nothing if `dependency' is an empty
                interface.
           [>>] -1 adds a new "OR" term to the sum-of-products expression being
                constructed internally, with the condition "Is Not Related to
                `dependency'".  Does nothing if `dependency' is an empty
                interface.
           [>>] 2 adds the condition "And Is Related to `dependency'" to the
                most recent term in the sum-of-products expression being
                constructed internally.  Does nothing if `dependency' is an
                empty interface.
           [>>] -2 adds the condition "And Is Not Related to `dependency'" to
                the most recent term in the sum-of-products expression being
                constructed internally.  Does nothing if `dependency' is an
                empty interface.
           [>>] All other values cause the internal sum-of-products expression
                to be left unchanged.
           [//]
           For example, the expression "(Is related to A and B) or (is not
           related to C)" can be formed by calling this function three times,
           with `dependency' and `dependency_effect' equal to (A,0), (B,2)
           and (C,-1), respectively.
         [ARG: aux_params]
           Array with `num_aux_params' integer parameters which are passed
           along to the `paint_overlay' and `custom_paint_overlay' functions
           via an `kdu_overlay_params' object.
           [//]
           The internal implementation of the `paint_overlay' function
           interprets any supplied auxiliary parameters as follows (note
           that this interpretation extends but remains backward compatible
           with earlier interpretations):
           [>>] The parameters are organized into M segments (or "rows"),
                denoted m=1,2,...,M.
           [>>] Each row m specifies a border size Bm, a sequence of colours
                denoted ARGB_m0, ARGB_m1, ..., ARGB_mBm, and a threshold Tm.
           [>>] If the collection of regions defined by the ROI description
                box which is being painted have an average "width" which is
                greater than or equal to T1, the border size and colours
                associated with row 1 are used to paint the entire ROI.
                Otherwise, if the average ROI "width" is greater than or equal
                to T2, the border size and colours associated with row 2 are
                used; and so forth.
           [>>] The average ROI "width" is assessed by applying the
                `jpx_roi::measure_span' function to each region provided by the
                ROI description box and forming a weighted average of the
                returned widths, weighted according to the corresponding
                lengths -- note that `jpx_roi::measure_span' returns results
                which are completely invariant to arbitrary orientations.  Note
                also that the widths and lengths are assessed after transforming
                the region coordinates according to the current rendering scale
                and geometry.
           [>>] ARGB colour values have their alpha (blending) value in the
                most significant byte, followed by red, green and then blue
                in the least significant byte.  When painting overlays for
                higher precision renditions (e.g., with floating-point buffers)
                the relevant colour and alpha components are scaled in the
                most natural manner.
           [>>] If any row m specifies a border size of Bm=0, no further rows
                are considered and there is no threshold value Tm, so the only
                parameter following Bm is the interior colour ARGB_0.
           [>>] For all other rows m, ARGB_m0 denotes the colour to be used
                for painting the interior of the region of interest;
                ARGB_m1 denotes the colour used for painting the inner-most
                border pixels; and so forth out to ARGB_mBm which is used for
                painting the outer-most border pixels.
           [>>] If any `num_aux_params' is such that the available set of
                parameters terminates part way through row m, the following
                policy is applied to automatically complete the row.  If the
                border size is missing, it defaults to Bm=1 (for m>1) or
                Bm=`get_max_painting_border' (for m=1).  If the interior
                colour is missing, it defaults to ARGB_m0=0 (totally
                transparent).  If the interior border colour is missing,
                it default to ARGB_m1=0xFF0000FF (solid blue).  If any
                subsequent border colours ARGB_mb are missing (1 < b <= Bm),
                the default border colour is obtained by replicating the last
                available border colour but decreasing the alpha component
                linearly towards 0 with b.  If the threshold Tm is missing,
                it is taken to be 0 (this threshold is satisfied by all
                possible values of ROI "width").
           [//]
           Each time you call this function with a different set of auxiliary
           overlay parameters, all active metadata is scheduled for
           repainting with the new parameters.  The actual painting occurs
           during calls to `process'.
         [ARG: num_aux_params]
           Number of elements in the `aux_params' array.  Both arguments
           are ignored unless `aux_params' is non-NULL and `num_aux_params'
           is positive.
      */
    KDU_AUX_EXPORT void update_overlays(bool start_from_scratch);
      /* [SYNOPSIS]
           Call this function if you have made any changes in the metadata
           associated with a JPX data source and want them to be reflected
           on overlays folded into the composited image.  This function does
           not actually paint any overlay data itself, but it does schedule
           the updates to be performed in subsequent calls to the `process'
           function.
           [//]
           You SHOULD call this function with `start_from_scratch' set to true,
           after using `jpx_metanode::delete' to delete one or more metadata
           nodes.  If you have only added new metadata, it is sufficient to
           call this function with `start_from_scratch' set to false.
         [ARG: start_from_scratch]
           If true, all overlay surfaces will be erased and all previously
           discovered metadata will be treated as invalid.  Subsequent calls
           to `process' will then generate the metadata overlays from
           scratch.  In fact, the next call to `process' may return a new
           region which represents the entire buffer surface, forcing the
           application to repaint the entire buffer.
      */
    KDU_AUX_EXPORT jpx_metanode
      search_overlays(kdu_coords point, kdu_istream_ref &istream_ref,
                      float visibility_threshold);
      /* [SYNOPSIS]
           This function searches through all overlay metadata, returning
           a non-empty interface only if a node is found whose region (or
           regions) of interest include the supplied point.  The function
           returns an empty interface if overlays are not currently enabled,
           or if the relevant metanode is not currently visible, as determined
           by the opacity of imagery layers which may lie on top of an imagery
           layer whose codestream actually contains the metadata.
           [//]
           The `visibility_threshold' has a similar interpretation to that
           adopted for the `find_point' function, except that the opacity of
           the ilayer in which an overlay is actually found is irrelevant --
           all that matters is whether overlaying ilayers render an overlay
           invisible at `point', with respect to the `visibility_threshold'.
           [//]
           Note carefully that this function does not distinguish between
           metadata which is visible within the overlay or hidden, due
           to dependency information which may have been passed to the
           most recent call to `configure_overlays'.  Hidden metadata
           corresponds to regions of interest which would be displayed were
           not for the fact that they do not pass the dependency tests
           described in connection with `configure_overlays'.  The fact that
           hidden metadata is searched by this function can be very helpful
           to an interactive user.
         [RETURNS]
           An empty interface unless a match is found, in which case the
           returned object is represented by an ROI description box -- i.e.,
           `jpx_metanode::get_num_regions' is guaranteed to return a value
           greater than 0.
         [ARG: point]
           Location of a point on the compositing grid, expressed using the
           same coordinate system as that associated with `set_buffer_surface'.
         [ARG: istream_ref]
           Used to return an instance of the `kdu_istream_ref' class which
           uniquely identifies the istream of the top-most imagery layer in
           which region-of-interest metadata is found, which contains `point'
           and for which the `visibility_threshold' is satisfied.  The imagery
           layer itself, along with the codestream index against which the
           metadata's region of interest is registed, can be found by passing
           `ilayer_ref' to `get_istream_info'.
         [ARG: visibility_threshold]
           Metadata overlays in a ilayer L are assigned a visibility V_L at
           location `point', where V_L = Prod_{l < L} (1 - O_l).  Here,
           O_l is the opacity associated with an overlaying ilayer, l,
           normalized to the range 0 to 1. Overlay metadata is considered
           visible in ilayer L if V_L > `visibility_threshold'.
           If an ilayer has not yet been completely rendered,
           so that the opacity at `point' is unknown, the opacity will
           usually be taken to be 1.0 -- there may be exceptions if you
           are rendering without buffer initialization (i.e., if
           `set_buffer_surface_initialization_mode' has been called with
           false as the argument).
       */
    KDU_AUX_EXPORT bool
      get_overlay_info(int &total_roi_nodes, int &hidden_roi_nodes);
      /* [SYNOPSIS]
           You can use this function to discover summary information about
           the state of the overlay rendering system.  If overlays are
           not enabled, the function returns false and does not touch the
           `total_roi_nodes' or `hidden_roi_nodes' variables.  Otherwise,
           the function returns true, setting the value of `total_roi_nodes'
           to the total number of internal references to JPX ROI metanodes
           for which calls to `custom_paint_overlay' might be made if
           overlays were refreshed from scratch, and `hidden_roi_nodes' to
           the number of these internal references to JPX ROI metanodes
           for which calls to `custom_paint_overlay' would not be made
           because they do not match the dependency information supplied
           via calls to `configure_overlays'.
      */ 
    virtual bool
      custom_paint_overlay(kdu_compositor_buf *buffer, kdu_dims buffer_region,
                           kdu_dims bounding_region, jpx_metanode node,
                           kdu_overlay_params *painting_params,
                           kdu_coords image_offset, kdu_coords subsampling,
                           bool transpose, bool vflip, bool hflip,
                           kdu_coords expansion_numerator,
                           kdu_coords expansion_denominator,
                           kdu_coords compositing_offset) { return false; }
      /* [SYNOPSIS]
         [BIND: callback]
           This function is called from the internal machinery whenever the
           overlay for some spatially sensitive metadata needs to be painted.
           The default implementation returns false, meaning that overlays
           should be painted using the regular `paint_overlay' function.
           To implement a custom overlay painter, you should override this
           present function in a derived class and return true.  In that
           case, `paint_overlay' will not be called.  It is also possible
           to directly override the `paint_overlay' function, but this does
           not provide a solution for alternate language bindings and is thus
           not recommended.  The present function can be implemented in any
           Java, C#, Visual Basic or other managed class derived from the
           `kdu_region_compositor' object's language binding.
           [//]
           Overlays are available only when working with JPX data sources.
           The function is called separately for each `jpx_metanode' which
           corresponds to a JPX region of interest which is relevant to the
           underlying imagery -- in practice, the function may be called
           multiple times for any given compositing layer and region of
           interest which intersects with that compositing layer, because
           overlay information is painted separately over tiles, whose
           dimensions and boundaries are determined internally so as to
           maximize efficiency for various processes.
           [//]
           The remainder of the description below applies if you intend to
           implement an appropriate painting process yourself, from scratch.
           However, you can also directly invoke `paint_overlay' to do the
           painting.  This could be interesting if you want to apply some
           specific filtering procedure based on the metadata, to determine
           how or if an overlay should be painted.  You could also augment
           the information painted by `paint_overlay' with additional
           textual or other semantic decorations.  One particularly
           simple, yet interesting possibility is to dynamically reconfigure
           the behaviour of `paint_overlay' based on the underlying metadata
           descended from `node' by temporarily modifying the auxiliary
           painting parameters using `kdu_overlay_params::push_aux_params'.
           [//]
           The `buffer' argument provides a raster scan overlay buffer,
           which covers the current rendering region within a particular
           compositing layer.  `buffer_region' identifies the region
           occupied by this buffer, relative to the compositing reference
           grid.  However, you should confine your painting activities to
           the intersection between this `buffer_region' and the
           `bounding_region'.
           [//]
           While detailed descriptions of the various arguments appear
           below, it is worth describing the geometric mapping process
           up front.  Region sensitive metadata is always associated with
           specific codestreams, via an ROI description box.  The ROI
           description box describes the regions on the codestream's
           high resolution canvas, with locations offset relative to the
           upper left hand corner of the image region on this canvas (the
           image region might not necessarily start at the canvas origin).
           By contrast, the overlay buffer's coordinate system is referred
           to a compositing reference grid, which depends upon compositing
           instructions in the JPX data source, as well as the current
           scale and orientation of the rendering surface.  To map from an
           ROI description region, `R', to the compositing reference grid,
           the following steps are required:
           [>>] Add `image_offset' to all locations in `R'.  This translates
                the region to one which is correctly registered against the
                codestream's high resolution canvas.
           [>>] Invoke `kdu_coords::to_apparent' on each location in `R'
                (offset as above), passing in the `transpose', `vflip' and
                `hflip' arguments.
           [>>] Convert the points generated as above using the static
                `kdu_region_decompressor::find_render_point' function,
                passing in the `expand_numerator' and `expand_denominator'
                arguments, along with the `subsampling' argument, after
                first invoking `subsampling.transpose()' if `transpose'
                is true -- this is necessary because the `subsampling'
                argument passed to the present function is expressed with
                respect to the original codestream geometry (not the apparent
                codestream geometry).
           [>>] Subtract `compositing_offset' from the locations produced,
                as above.
           [//]
           Note that the `kdu_overlay_params::map_jpx_regions' function can
           be used to perform the above transformations on all regions of
           interest described by a JPX ROI box -- this is almost certainly
           what you will want to do.
         [RETURNS]
           True if the overlay painting process for `node' has been handled
           by this function (even if nothing was actually painted) so that
           the default `paint_overlay' function need not be called.
         [ARG: buffer]
           Pointer to the relevant overlay buffer.  The size and location
           of this buffer, as it appears on the compositing surface, are
           described by the `buffer_region' argument.  The overlay buffer
           has Alpha, R, G and B components and is alpha blended with the
           image component associated with this overlay.  When painting the
           buffer, it is your responsibility to write Alpha, R, G and B values
           at each desired location.  You are not responsible for performing
           the alpha blending itself -- indeed alpha can be additionally
           scaled during the overlay blending process to implement simple
           effects.
           [//]
           If the buffer has a 32-bit pixel organization,
           `buffer->get_buf' returns a non-NULL pointer, and the organization
           of the four channels within each 32-bit word is as follows:
           [>>] the most significant byte of the word holds the alpha value;
           [>>] the second most significant byte holds the red channel value;
           [>>] the next byte holds the green channel value; and
           [>>] the least significant byte holds the blue channel value.
           [//]
           If the buffer has a floating-point organization,
           `buffer->get_float_buf' returns a non-NULL pointer, and each
           pixel consists of four successive floating point quantities, in
           the order Alpha, red, green and then blue. In this case, all
           quantities have a range of 0.0 to 1.0; in particular, opaque
           overlay content should have an alpha value of 0.0.
         [ARG: buffer_region]
           Region occupied by the overlay buffer, expressed on the compositing
           reference grid -- this is generally a subset of the region occupied
           by the associated compositing layer, since each compositing layer
           can have its own overlay buffer. 
         [ARG: bounding_region]
           Bounding rectangle, within which overlay data should be correctly
           painted.  For consistent appearance, you should paint overlay data
           only within the intersection between `buffer_region' and
           `bounding_region'.  This allows the internal implementation to
           split metadata into smaller segments, where some regions of interest
           may span multiple segments.  Segmentation allows metadata lists
           to be ordered based on region size, without excessive sorting cost,
           and ordering ensures more consistent appearance.  Segmentation also
           allows more efficient re-use of rendered metadata as a compositing
           viewport is panned around.  If you paint outside the
           `bounding_region', there is a chance that you overwrite some
           metadata overlay content which was supposed to be painted on top
           of the current content.
         [ARG: node]
           This is guaranteed to represent an ROI description box.  Use the
           `jpx_metanode::get_regions' member (for example) to recover the
           geometrical properties of the region.  Use other
           `jpx_metanode' members to examine any descendant nodes, describing
           metadata which is associated with this spatial region.  The
           implementation might potentially look for label boxes, XML boxes
           or UUID boxes (with associated URL's) in the descendants of `node'.
         [ARG: painting_params]
           This argument always points to a valid object, whose member
           functions return additional (optional) information to the overlay
           painting procedure.  The object also provides useful services to
           greatly facilitate the painting of complex regions of interest,
           including general those composed of general quadrilaterals.  See
           the definition of `kdu_overlay_params' for more on the exact
           nature of this additional information.  You should pay particular
           attention to the maximum border size returned by
           `painting_params->get_max_painting_border' if you intend to
           paint outside the region of interest defined by `node'.
         [ARG: image_offset]
           See the coordinate transformation steps described in the
           introduction to this function.
         [ARG: subsampling]
           See the coordinate transformation steps described in the
           introduction to this function.
         [ARG: transpose]
           See the coordinate transformation steps described in the
           introduction to this function.
         [ARG: vflip]
           See the coordinate transformation steps described in the
           introduction to this function.
         [ARG: hflip]
           See the coordinate transformation steps described in the
           introduction to this function.
         [ARG: expansion_numerator]
           See the coordinate transformation steps described in the
           introduction to this function.
         [ARG: expansion_denominator]
           See the coordinate transformation steps described in the
           introduction to this function.
         [ARG: compositing_offset]
           See the coordinate transformation steps described in the
           introduction to this function.
      */
  protected:
    virtual void
      paint_overlay(kdu_compositor_buf *buffer, kdu_dims buffer_region,
                    kdu_dims bounding_region, jpx_metanode node,
                    kdu_overlay_params *painting_params,
                    kdu_coords image_offset, kdu_coords subsampling,
                    bool transpose, bool vflip, bool hflip,
                    kdu_coords expansion_numerator,
                    kdu_coords expansion_denominator,
                    kdu_coords compositing_offset);
      /* [SYNOPSIS]
           See `custom_paint_overlay' for a description.  The present
           function implements the default overlay painting policy, which
           simply paints shaded boxes or ellipses, as appropriate, onto the
           overlay buffer, for each regoin of an ROI metanode.  More
           sophisticated implementations may override this function, or
           (preferably) override the `custom_paint_overlay' function, to
           paint more interesting overlays, possibly incorporating text
           labels.  If you wish to do this from a foreign language, such
           as Java, C# or Visual Basic, you must override the
           `custom_paint_overlay' function.
      */
  //----------------------------------------------------------------------------
  public: // Virtual functions to be overridden
    virtual kdu_compositor_buf *
      allocate_buffer(kdu_coords min_size, kdu_coords &actual_size,
                      bool read_access_required) { return NULL; }
      /* [SYNOPSIS]
         [BIND: callback]
           Override this function to provide your own image buffer
           allocation.  For example, you may want to allocate the 
           image buffer as part of a system resource from which rendering
           to a display can be accomplished more easily or efficiently.
           [//]
           Alternatively, you might want to allocate buffers with
           floating-point samples instead of 32-bit integer pixels, using
           the `kdu_compositor_buf::init_float' function.  This is a
           particularly important reason for overriding this function.  As
           of Kakadu version 6.3, all of the `kdu_region_compositor' object's
           internal machinery supports floating-point precision buffers,
           in addition to the original 8-bit/sample representation associated
           with 32-bit/pixel buffers.  The way to access this functionality
           is by overriding the present function and consistently allocating
           floating-point buffers.
           [//]
           You may, if you like, allocate a larger buffer than the one
           requested, returning the actual buffer size via the `actual_size'
           argument.  This reduces the likelihood that reallocation will be
           necessary during interactive viewing.
           [//]
           The `read_access_required' argument is provided to support
           efficient use of special purpose memory resources which might
           only support writing.  If the internal machinery only intends
           to write to the buffer, it sets this flag to false.  It can
           happen that a buffer which was originally allocated as write-only
           must later be given read access, due to unforeseen changes.
           For example, a compositing layer which was originally rendered
           directly onto the frame might later need to be composed with
           other compositing layers (due to changes in the frame contents)
           for which read access is required.  Whenever the access type
           changes, `kdu_compositor_buf::set_read_accessibility' will be
           called.  The buffer manager has the option to allocate memory in
           a different way whenever the access type changes; it also has the
           option to instruct the internal machinery to regenerate the buffer
           when this happens, as explained in the description accompanying
           `kdu_compositor_buf::set_read_accessibility'.
           [//]
           For maximum memory access efficiency, when overriding this function,
           you should attempt to allocate `kdu_compositor_buf' objects whose
           referenced memory buffer is aligned on a 16-byte boundary.  For
           example, you might allocate a buffer which is somewhat larger than
           required, so that the `kdu_compositor_buf::buf' member can be
           rounded up to the nearest 16-byte boundary.
         [RETURNS]
           If you do not intend to allocate the buffer yourself, the function
           should return NULL (this is what the default implementation does),
           in which case the buffer will be internally allocated and internally
           deleted, without any call to `delete_buffer'.
      */
    virtual void delete_buffer(kdu_compositor_buf *buf) { return; }
      /* [SYNOPSIS]
         [BIND: callback]
           Override this function to deallocate any resources which you
           allocated using an overridden `allocate_buffer' implementation.
           The function will not be called if the buffer was allocated
           internally due to a NULL return from `allocate_buffer'.
      */
  private: // Helper functions
    friend class kdrc_stream;
    friend class kdrc_layer;
    friend class kdrc_overlay;
    void init(kdu_thread_env *env, kdu_thread_queue *env_queue);
      /* Called as the first step by each constructor. */
    void set_layer_buffer_surfaces();
      /* Called either within `set_buffer_surface' or, if a valid composition
         was not valid when that function was called, from within
         `update_composition'.  This function scans backwards through the
         compositing layers (i.e., starting with the upper-most layer and
         working back to the background), invoking their respective
         `kdrc_layer::set_buffer_surface' functions.  In the process, however,
         the function also calculates the portion of the buffer region which
         is completely covered by foreground layers, adjusting the buffer
         region supplied to the lower layers to reflect the fact that they
         may be partially or fully obscured.  This saves processing unnecessary
         imagery.  The function also sets `processing_complete' to false. */
    bool update_composition();
      /* Called from within `process', `get_total_composition_dims' or
         `get_composition_buffer' if any change had previously been made to
         the set of compositing layers or the compositing scale/orientation.
         This is where the scale of each individual compositing layer is set,
         where the dimensions of the final composition are determined, and
         where a separate compositing buffer may be allocated. */
    void donate_compositing_buffer(kdu_compositor_buf *buffer,
                                   kdu_dims buffer_region,
                                   kdu_coords buffer_size);
      /* This function is called from `kdrc_layer::set_buffer_surface' or
         `kdrc_layer::update_overlay' if it is discovered that a separate
         compositing buffer will need to be used after all, to accommodate
         the newly discovered need for metadata overlay information. */
    bool retract_compositing_buffer(kdu_coords &buffer_size);
      /* This function is called from inside `kdrc_layer::process_overlay' if
         the overlay buffer has been recently disabled so that a separate
         compositing buffer might no longer be required.  The function
         returns true if the caller can reclaim the global compositing buffer
         for its own layer buffer, deallocating its separate layer buffer.
         If the function returns false, the caller should leave the separate
         compositing buffer alone. */
    bool find_completed_rects(kdu_dims &region, kdrc_layer *start_layer,
                              int start_elt,
                              kdrc_layer **first_intersecting_layer=NULL,
                              kdrc_layer **last_intersecting_layer=NULL);
      /* This function is invoked only if there is a separate compositing
         buffer.  It scans through all region processing elements, starting
         from the compositing layer identified by `start_layer' and working
         upwards to the last compositing layer; within each layer, the function
         examines up to two streams as well as metadata overlays, each of which
         might be the subject of future processing.  The `start_elt'
         argument identifies the stream which we should start examining
         in `start_layer' (0 or 1); a value of 2 indicates that we should
         be examining overlays -- this is done last.
           If the function determines that future processing jobs will
         produce results overlapping with `region', the incomplete portions of
         `region' are removed and the function continues processing the
         remaining complete portions of `region' until all layers and layer
         elements have been examined; where the removal of incomplete regions
         leaves multiple rectangular regions (this can happen at any stage in
         the processing), the function must be recursively invoked on each
         piece.  At the end of this potentially recursive process, the
         original `region' has been decomposed into a collection of supposedly
         complete rectangular regions (actually, the implementation needs to
         take a conservative position sometimes -- it is not a disaster to
         identify a region as complete even if it is not).  At most one of
         these supposedly completed regions can be returned immediately to the
         application via the `kdu_region_compositor::process' call which
         originally invoked this function; the other branches in the recursion
         add their completely composited rectangular regions to the refresh
         manager.
           The last two arguments are used to enhance the efficiency of the
         composition process, as performed by `kdu_region_compositor::process'.
         If either or both of these arguments is NULL, all completed rectangles
         are added to the refresh manager and the function returns false.
         Otherwise, one of the completed regions (if there are any) is returned
         via `region' and the first and last layers with which an intersection
         was discovered are returned via *`first_intersecting_layer' and
         *`last_intersecting_layer'.  In this latter case, the function returns
         true if a completed region is being returned via `region'. */
    kdu_istream_ref assign_new_istream_ref();
    kdu_ilayer_ref assign_new_ilayer_ref();
    kdrc_stream *add_active_stream(int codestream_idx,
                                   int colour_init_src,
                                   bool single_component_only,
                                   bool alpha_only);
      /* This function is used by `kdrc_layer' to add istreams to an ilayer.
         The function first searches the `streams' list for an existing
         inactive istream which matches the request parameters; if one is
         found, it is detached from its existing layer and returned for
         use by the caller.  Otherwise, the function creates a new istream
         from scratch.
            The `codestream_idx' argument identifies the absolute index of
         the codestream within its data source -- note that for MJ2 data
         sources, this index effectively identifies the track index, the frame
         index and the field index.
            The `colour_init_src' argument has the same interpretation as the
         `kdrc_stream::colour_init_src' member.  Specifically, it is negative
         if the caller wants an "istream" which accesses the codestream
         components directly; if we want an "istream" which provides
         colour/channel/palette mapping according to the specifications in a
         JPX data source, this argument holds the zero-based index of the
         relevant JPX compositing layer; similarly, if we want an "istream"
         which provides colour/channel/palette mapping according to the
         specifications in an MJ2 data source, the `colour_init_src'
         argument holds the relevant MJ2 track index (minus 1, so that it is
         zero-based).
            In the case where `colour_init_src' < 0, the `single_component_only'
         argument indicates whether or not it is sufficient to find an
         "istream" which can support single-component rendering -- any istream
         with the correct codestream index can support this.  If
         `single_component_only' is false, an istream must be found (or
         created) which has been initialized to render imagery (potentially
         colour imagery) directly from a raw codestream.
         Otherwise, an existing istream which was initialized to render
         imagery based on JPX or MJ2 specifications may be temporarily
         borrowed to render a single image component by putting it into
         "single component mode".  The `single_component_only' argument is
         ignored if `colour_init_src' >= 0. */
    kdu_compositor_buf *
      internal_allocate_buffer(kdu_coords min_size, kdu_coords &actual_size,
                               bool read_access_required);
      /* This function is used internally to manage the allocation process.
         It calls the overridable callback function `allocate_buffer', doing
         its own allocation only if the latter returns NULL. */
    void internal_delete_buffer(kdu_compositor_buf *buf);
      /* This function is used internally to manage the deletion process.  If
         the buffer was allocated internally, it is deleted internally.
         Otherwise, if the buffer was allocated using `allocate_buffer', it
         is passed to the overridable callback function, `delete_buffer'. */
  private: // Data sources
    kdu_compressed_source *raw_src; // Installed by the 1st constructor
    jpx_source *jpx_src; // Installed by the 2nd constructor
    mj2_source *mj2_src; // Installed by the 3rd constructor
    jpx_input_box single_component_box; // Used to open individual codestream
  private: // Configuration parameters
    int error_level;
    bool persistent_codestreams;
    int codestream_cache_threshold;
    float process_aggregation_threshold;
  private: // Current composition state
    bool have_valid_scale;
    int max_quality_layers;
    bool vflip, hflip, transpose;
    float scale;
    int invalid_scale_code; // Error code from last scale-dependent failure
    kdu_dims fixed_composition_dims; // If compositing confined to a fixed frame
    kdu_dims default_composition_dims; // For frame, if can't initialize layers
    kdu_dims total_composition_dims;
    kdu_compositor_buf *composition_buffer; // NULL if first layer is only one
    kdu_dims buffer_region;
    kdu_coords buffer_size; // Actual dimensions of memory buffer
    kdu_uint32 buffer_background;
    bool processing_complete; // If the buffered region is fully composited
    bool composition_invalid; // If layers, scale or buffer surface changed
    bool can_skip_surface_initialization;
    bool initialize_surfaces_on_next_refresh;
    bool enable_overlays;
    int overlay_log2_segment_size;
    int overlay_min_display_size;
    kdu_int16 overlay_factor_x128; // 128 * real-valued blending factor
    int overlay_max_painting_border;
    kdrc_overlay_expression *overlay_dependencies;
    int overlay_num_aux_params;
    kdu_uint32 *overlay_aux_params; // Copy of array in `configure_overlays'
  private: // Resource lists
    kdrc_queue *queue_head; // Least recent element on jitter-absorbtion queue
    kdrc_queue *queue_tail; // Most recent element on jitter-absorbtion queue
    kdrc_queue *queue_free; // Recycling list for queue elements.
    kdrc_layer *active_layers; // Doubly-linked active compositing layers list
    kdrc_layer *last_active_layer; // Tail of doubly-linked active layers list
    kdrc_layer *inactive_layers; // Singly-linked list of layers not in use
    kdrc_stream *streams; // Singly-linked list of all istreams
    kdrc_refresh *refresh_mgr; // Manages regions which must be refreshed via
                         // calls to `process' and would not otherwise be
                         // refreshed in the course of decompression processing
    kdu_ilayer_ref last_assigned_ilayer_ref;
    kdu_istream_ref last_assigned_istream_ref;
  private: // Multi-threading
    kdu_thread_env *env;
    kdu_thread_queue *env_queue;
  };

#endif // KDU_REGION_COMPOSITOR_H
