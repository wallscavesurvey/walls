/*****************************************************************************/
// File: kdu_sample_processing.h [scope = CORESYS/COMMON]
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
   Uniform interface to sample data processing services: DWT analysis; DWT
synthesis; subband sample encoding and decoding; colour and multi-component
transformation.  Here, we consider the encoder and decoder objects to be
sample data processing objects, since they accept or produce unquantized
subband samples.  They build on top of the block coding services defined in
"kdu_block_coding.h", adding quantization, ROI adjustments, appearance
transformations, and buffering to interface with the DWT.
******************************************************************************/

#ifndef KDU_SAMPLE_PROCESSING_H
#define KDU_SAMPLE_PROCESSING_H

#include <assert.h>
#include "kdu_messaging.h"
#include "kdu_compressed.h"

// Defined here:
union kdu_sample32;
union kdu_sample16;
class kdu_sample_allocator;
class kdu_line_buf;

class kdu_push_ifc_base;
class kdu_push_ifc;
class kdu_pull_ifc_base;
class kdu_pull_ifc;
class kdu_analysis;
class kdu_synthesis;
class kdu_encoder;
class kdu_decoder;
class kd_multi_analysis_base;
class kd_multi_synthesis_base;
class kdu_multi_analysis;
class kdu_multi_synthesis;

// Defined elsewhere:
class kdu_roi_node;
class kdu_roi_image;


/* ========================================================================= */
/*                     Class and Structure Definitions                       */
/* ========================================================================= */

/*****************************************************************************/
/*                              kdu_sample32                                 */
/*****************************************************************************/

union kdu_sample32 {
    float fval;
      /* [SYNOPSIS]
           Normalized floating point representation of an image or subband
           sample subject to irreversible compression.  See the description
           of `kdu_line_buf' for more on this.
      */
    kdu_int32 ival;
      /* [SYNOPSIS]
           Absolute 32-bit integer representation of an image or subband
           sample subject to reversible compression.  See the description
           of `kdu_line_buf' for more on this.
      */
  };

/*****************************************************************************/
/*                              kdu_sample16                                 */
/*****************************************************************************/

#define KDU_FIX_POINT ((int) 13) // Num frac bits in a 16-bit fixed-point value

union kdu_sample16 {
  kdu_int16 ival;
    /* [SYNOPSIS]
         Normalized (fixed point) or absolute (integer) 16-bit representation
         of an image or subband sample subject to irreversible (normalized)
         or reversible (integer) compression.  See the description of
         `kdu_line_buf' for more on this.
    */
  };

/*****************************************************************************/
/*                          kdu_sample_allocator                             */
/*****************************************************************************/

class kdu_sample_allocator {
  /* [BIND: reference]
     [SYNOPSIS]
     This object serves to prevent excessive memory fragmentation.
     Pre-allocation requests are made by all clients which wish to use
     its services, after which a single block of memory is allocated and
     the actual allocation requests must be made -- these must be identical
     to the pre-allocation requests.  Memory chunks allocated by this
     object to a client may not be individually returned or recycled, since
     this incurs overhead and fragmentation.
     [//]
     The object may be re-used (after a call to its `restart' member) once
     all memory served out is no longer in use.  When re-used, the object
     makes every attempt to avoid destroying and re-allocating its memory
     block.  This avoids memory fragmentation and allocation overhead when
     processing images with multiple tiles or when used for video
     applications.
  */
  public: // Member functions
    kdu_sample_allocator()
      { bytes_reserved = bytes_used = buffer_size = 0;
        pre_creation_phase=true; buffer = buf_handle = NULL; }
      /* [SYNOPSIS] Creates an empty object. */
    ~kdu_sample_allocator()
      { if (buf_handle != NULL) delete[] buf_handle; }
      /* [SYNOPSIS]
           Destroys all storage served from this object via its
           `alloc16' and `alloc32' functions.
      */
    void restart() { bytes_reserved=bytes_used=0; pre_creation_phase=true; }
      /* [SYNOPSIS]
           Invalidates all memory resources served up using `alloc16' or
           `alloc32' and starts the pre-allocation phase from scratch.  The
           internal resources are sized during subsequent calls to `pre_alloc'
           and memory is re-allocated, if necessary, during the next call to
           `finalize', based on these pre-allocation requests alone.
      */
    void pre_alloc(bool use_shorts, int before, int after, int num_requests)
      {
      /* [SYNOPSIS]
           Reserves enough storage for `num_requests' later calls to `alloc16'
           (if `use_shorts' is true) or `alloc32' (if `use_shorts' is false).
           Space is reserved such that each of these `num_requests' allocations
           can return an appropriately aligned pointer to an array which offers
           entries at locations n in the range -`before' <= n < `after', where
           each entry is of type `kdu_sample16' (if `use_shorts'=true) or
           `kdu_sample32' (if `use_shorts'=false).
      */
        assert(pre_creation_phase);
        before+=before; after+=after; // Two bytes per sample
        if (!use_shorts)
          { before+=before; after+=after; } // Four bytes per sample
        bytes_reserved += num_requests*(((15+before)&~15) + ((15+after)&~15));
      }
    void finalize()
      {
      /* [SYNOPSIS]
           Call this function after all pre-allocation (calls to `pre_alloc')
           has been completed.  It performs the actual allocation of heap
           memory, if necessary.
      */
        assert(pre_creation_phase); pre_creation_phase = false;
        if (bytes_reserved > buffer_size)
          { // Otherwise, use the previously allocated buffer.
            buffer_size = bytes_reserved;
            if (buf_handle != NULL) delete[] buf_handle;
            buffer = buf_handle = new kdu_byte[buffer_size+24];
            if (_addr_to_kdu_int32(buffer) & 8)
              buffer += 8; // Align on 16-byte boundary
          }
        assert((bytes_reserved == 0) || (buffer != NULL));
      }
    kdu_sample16 *alloc16(int before, int after)
      {
      /* [SYNOPSIS]
           Allocate an array with space for `before' entries prior to the
           returned pointer and `after' entries after the returned pointer
           (including the entry to which the pointer refers).  The returned
           pointer is guaranteed to be aligned on a 16-byte boundary.
           The pointer may be interpreted as the location of an array which
           can accept signed indices, n, in the range -`before' <= n < `after'.
           [//]
           Before calling this function, you must have pre-allocated sufficient
           space through calls to `pre_alloc' and you must have called the
           `finalize' member function.
      */
        assert(!pre_creation_phase);
        before = (before+7)&~7; // Round up to nearest multiple of 16 bytes.
        after = (after+7)&~7; // Round up to nearest multiple of 16 bytes.
        kdu_sample16 *result = (kdu_sample16 *)(buffer+bytes_used);
        result += before; bytes_used += (before+after)<<1;
        assert(bytes_used <= bytes_reserved);
        return result;
      }
    kdu_sample32 *alloc32(int before, int after)
      {
      /* [SYNOPSIS]
           Same as `alloc16', except that it allocates storage for arrays of
           32-bit quantities.  Again, the returned pointer is guaranteed to
           be aligned on a 16-byte boundary.
      */
        assert(!pre_creation_phase);
        before = (before+3)&~3; // Round up to nearest multiple of 16 bytes.
        after = (after+3)&~3; // Round up to nearest multiple of 16 bytes.
        kdu_sample32 *result = (kdu_sample32 *)(buffer+bytes_used);
        result += before; bytes_used += (before+after)<<2;
        assert(bytes_used <= bytes_reserved);
        return result;
      }
    int get_size()
      {
      /* [SYNOPSIS]
           For memory consumption statistics.  Returns the total amount of
           heap memory allocated by this object.  The heap memory is allocated
           within calls to `finalize'; it either grows or stays the same in
           each successive call to `finalize', such calls being interspersed
           by invocations of the `restart' member.
      */
        return buffer_size;
      }
  private: // Data
    bool pre_creation_phase; // True if in the pre-creation phase.
    int bytes_reserved;
    int bytes_used;
    int buffer_size; // Must be >= `bytes_reserved' except in precreation phase
    kdu_byte *buffer;
    kdu_byte *buf_handle; // Handle for deallocating buffer
  };

/*****************************************************************************/
/*                              kdu_line_buf                                 */
/*****************************************************************************/

#define KD_LINE_BUF_ABSOLUTE ((kdu_byte) 1)
#define KD_LINE_BUF_SHORTS   ((kdu_byte) 2)

class kdu_line_buf {
  /* [BIND: copy]
     [SYNOPSIS]
     Instances of this structure manage the buffering of a single line of
     sample values, whether image samples or subband samples.  For the
     reversible path, samples must be absolute 32- or 16-bit integers.  For
     irreversible processing, samples have a normalized representation, where
     the nominal range of the data is typically -0.5 to +0.5 (actual nominal
     ranges for the data supplied to a `kdu_encoder' or `kdu_analysis' object
     or retrieved from a `kdu_decoder' or `kdu_synthesis' object may be
     explicitly set in the relevant constructor).  These normalized quantities
     may have either a true 32-bit floating point representation or a 16-bit
     fixed-point representation.  In the latter case, the least significant
     `KDU_FIX_POINT' bits of the integer are interpreted as binary fraction
     bits, so that 2^{KDU_FIX_POINT} represents a normalized value of 1.0.
     [//]
     The object always allocates space to allow access to a given number
     of samples before the first nominal sample (the one at index 0) and a
     given number of samples beyond the last nominal sample (the one at
     index `width'-1).  These extension limits are explained in the
     comments appearing with `pre_create'.
  */
  // --------------------------------------------------------------------------
  public: // Life cycle functions
    kdu_line_buf() { destroy(); }
    void pre_create(kdu_sample_allocator *allocator,
                    int width, bool absolute, bool use_shorts,
                    int extend_left=2, int extend_right=2)
      {
      /* [SYNOPSIS]
           Declares the characteristics of the internal storage which will
           later be created by `create'.  If `use_shorts' is true, the sample
           values will have 16 bits each and normalized values will use a
           fixed point representation with KDU_FIX_POINT fraction bits.
           Otherwise, the sample values have 32 bits each and normalized
           values use a true floating point representation.
           [//]
           This function essentially calls `allocator->pre_create', requesting
           enough storage for a line with `width' samples, providing for legal
           accesses up to `extend_left' samples before the beginning of the
           line and `extend_right' samples beyond the end of the line.
           [//]
           Moreover, the returned line buffer is aligned on a 16-byte
           boundary, the `extend_left' and `extend_right' values are rounded
           up to the nearest multiple of 16 bytes, and the length of the
           right-extended buffer is also rounded up to a multiple of 16 bytes.
           Finally, it is possible to read at least 16 bytes beyond the
           end of the extended region, although writes to these extra 16
           bytes might overwrite data belonging to a different line.
         [ARG: allocator]
           Pointer to the object which will later be used to complete the
           allocation of storage for the line.  The pointer is saved
           internally until such time as the `create' function is called, so
           you must be careful not to delete this object.  You must also be
           careful to call its `kdu_sample_allocator::finalize' function
           before calling `create'.
         [ARG: width]
           Nominal width of (number of samples in) the line.  Note that space
           reserved for access to `extend_left' samples to the left and
           `extend_right' samples to the right.  Moreover, additional
           samples may often be accessed to the left and right of the nominal
           line boundaries due to the alignment policy discussed above.
         [ARG: absolute]
           If true, the sample values in the line buffer are to be used in
           JPEG2000's reversible processing path, which works with absolute
           integers.  otherwise, the line is prepared for use with the
           irreversible processing path, which works with normalized
           (floating or fixed point) quantities.
         [ARG: use_shorts]
           If true, space is allocated for 16-bit sample values
           (array entries will be of type `kdu_sample16').  Otherwise, the
           line buffer will hold samples of type `kdu_sample32'.
         [ARG: extend_left]
           This quantity should be small, since it will be represented
           internally using 8-bit numbers, after rounding up to an
           appropriately aligned value.  It would be unusual to select
           values larger than 16 or perhaps 32.
         [ARG: extend_right]
           This quantity should be small, since it will be represented
           internally using 8-bit numbers, after rounding up to an
           appropriately aligned value.  It would be unusual to select
           values larger than 16 or perhaps 32.
      */
        assert((!pre_created) && (this->allocator == NULL)); this->width=width;
        flags = (use_shorts)?KD_LINE_BUF_SHORTS:0; 
        flags |= (absolute)?KD_LINE_BUF_ABSOLUTE:0;
        this->allocator = allocator;
        buf_before = (kdu_byte) extend_left; // Rounded up automatically
        buf_after = (kdu_byte)
          (extend_right + ((-extend_right) & ((use_shorts)?7:3)));
        allocator->pre_alloc(use_shorts,buf_before,width+buf_after,1);
        allocator->pre_alloc(use_shorts,buf_before,width+buf_after,1);
        pre_created = true;
      }
    void create()
      {
      /* [SYNOPSIS]
           Finalizes creation of storage which was initiated by `pre_create'.
           Does nothing at all if the `pre_create' function was not called,
           or the object was previously created and has not been destroyed.
           Otherwise, you may not call this function until the
           `kdu_sample_allocator' object supplied to `pre_create' has had
           its `finalize' member function called.
      */
        if (!pre_created)
          return;
        pre_created = false;
        if (flags & KD_LINE_BUF_SHORTS)
          buf16 = allocator->alloc16(buf_before,width+buf_after);
        else
          buf32 = allocator->alloc32(buf_before,width+buf_after);
      }
    int check_status()
      {
      /* [SYNOPSIS]
           Returns 1 if `create' has been called, -1 if only `pre_create' has
           been called, 0 if neither has been called since the object was
           constructed or since the last call to `destroy'.
      */
        if (pre_created) return -1;
        else if (allocator != NULL) return 1;
        else return 0;
      }
    void destroy()
      {
      /* [SYNOPSIS]
           Restores the object to its uninitialized state ready for a new
           `pre_create' call.  Does not actually destroy any storage, since
           the `kdu_sample_allocator' object from which storage is served
           does not permit individual deallocation of storage blocks.
      */
        width=0; flags=0; pre_created=false;
        allocator=NULL; buf16=NULL; buf32=NULL;
      }
  // --------------------------------------------------------------------------
  public: // Access functions
    kdu_sample32 *get_buf32()
      {
      /* [SYNOPSIS]
           Returns NULL if the sample values are of type `kdu_sample16'
           instead of `kdu_sample32', or the buffer has not yet been
           created.  Otherwise, returns an array which supports accesses
           at least with indices in the range 0 to `width'-1, but typically
           beyond these bounds -- see `pre_create' for an explanation of
           extended access bounds.
      */
        return (flags & KD_LINE_BUF_SHORTS)?NULL:buf32;
      }
    kdu_sample16 *get_buf16()
      {
      /* [SYNOPSIS]
           Returns NULL if the sample values are of type `kdu_sample32'
           instead of `kdu_sample16', or the buffer has not yet been
           created.  Otherwise, returns an array which supports accesses
           at least with indices in the range 0 to `width'-1, but typically
           beyond these bounds -- see `pre_create' for an explanation of
           extended access bounds.
      */
        return (flags & KD_LINE_BUF_SHORTS)?buf16:NULL;
      }
    bool get_floats(float *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 32-bit normalized
           (floating point) sample representation.  Otherwise, returns true
           and copies the floating point samples into the supplied `buffer'.
           The first copied sample is `first_idx' positions from the start
           of the line.  There may be little or no checking that the
           sample range represented by `first_idx' and `num_samples' is
           legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf32'.
      */
        if (flags & (KD_LINE_BUF_ABSOLUTE | KD_LINE_BUF_ABSOLUTE))
          return false;
        for (int i=0; i < num_samples; i++)
          buffer[i] = buf32[first_idx+i].fval;
        return true;
      }
    bool set_floats(float *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 32-bit normalized
           (floating point) sample representation.  Otherwise, returns true
           and copies the floating point samples from the supplied `buffer'.
           The first sample in `buffer' is stored `first_idx' positions from
           the start of the line.  There may be little or no checking that
           the sample range represented by `first_idx' and `num_samples' is
           legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf32'.
      */
        if (flags & (KD_LINE_BUF_ABSOLUTE | KD_LINE_BUF_ABSOLUTE))
          return false;
        for (int i=0; i < num_samples; i++)
          buf32[i+first_idx].fval = buffer[i];
        return true;
      }
    bool get_ints(kdu_int32 *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 32-bit absolute
           (integer) sample representation.  Otherwise, returns true
           and copies the integer samples into the supplied `buffer'.
           The first copied sample is `first_idx' positions from the start
           of the line.  There may be little or no checking that the
           sample range represented by `first_idx' and `num_samples' is
           legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf32'.
      */
        if ((flags & KD_LINE_BUF_SHORTS) || !(flags & KD_LINE_BUF_ABSOLUTE))
          return false;
        for (int i=0; i < num_samples; i++)
          buffer[i] = buf32[first_idx+i].ival;
        return true;
      }
    bool set_ints(kdu_int32 *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 32-bit absolute
           (integer) sample representation.  Otherwise, returns true
           and copies the integer samples from the supplied `buffer'.
           The first sample in `buffer' is stored `first_idx' positions from
           the start of the line.  There may be little or no checking that
           the sample range represented by `first_idx' and `num_samples' is
           legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf32'.
      */
        if ((flags & KD_LINE_BUF_SHORTS) || !(flags & KD_LINE_BUF_ABSOLUTE))
          return false;
        for (int i=0; i < num_samples; i++)
          buf32[i+first_idx].ival = buffer[i];
        return true;
      }
    bool get_ints(kdu_int16 *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 16-bit sample
           representation (either fixed point or absolute integers).
           Otherwise, returns true and copies the 16-bit samples into the
           supplied `buffer'.  The first copied sample is `first_idx'
           positions from the start of the line.  There may be little or no
           checking that the sample range represented by `first_idx' and
           `num_samples' is legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf16'.
      */
        if (!(flags & KD_LINE_BUF_SHORTS))
          return false;
        for (int i=0; i < num_samples; i++)
          buffer[i] = buf16[first_idx+i].ival;
        return true;
      }
    bool set_ints(kdu_int16 *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 16-bit sample
           representation (either fixed point or absolute integers).
           Otherwise, returns true and copies the integer samples from the
           supplied `buffer'.  The first sample in `buffer' is stored
           `first_idx' positions from the start of the line.  There may be
           little or no checking that the sample range represented by
           `first_idx' and `num_samples' is legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf16'.
      */
        if (!(flags & KD_LINE_BUF_SHORTS))
          return false;
        for (int i=0; i < num_samples; i++)
          buf16[i+first_idx].ival = buffer[i];
        return true;
      }
    int get_width()
      {
      /* [SYNOPSIS]
           Returns 0 if the object has not been created.  Otherwise, returns
           the nominal width of the line.  Remember that the arrays returned
           by `get_buf16' and `get_buf32' are typically larger than the actual
           line width, as explained in the comments appearing with the
           `pre_create' function.
      */
        return width;
      }
    bool is_absolute() { return ((flags & KD_LINE_BUF_ABSOLUTE) != 0); }
      /* [SYNOPSIS]
           Returns true if the sample values managed by this object represent
           absolute integers, for use with JPEG2000's reversible processing
           path.
      */
  // --------------------------------------------------------------------------
  private: // Data -- should occupy 12 bytes on a 32-bit machine.
    int width; // Number of samples in buffer.
    kdu_byte buf_before, buf_after;
    kdu_byte flags; // Bit 0 set if absolute_ints;  Bit 1 set if use_shorts
    bool pre_created; // True if `pre_create' called but `create' still pending
    union {
      kdu_sample32 *buf32;
      kdu_sample16 *buf16;
      kdu_sample_allocator *allocator;
      };
  };

/*****************************************************************************/
/*                            kdu_push_ifc_base                              */
/*****************************************************************************/

class kdu_push_ifc_base {
  protected:
    friend class kdu_push_ifc;
    virtual ~kdu_push_ifc_base() { return; }
    virtual void push(kdu_line_buf &line, kdu_thread_env *env) = 0;
  };

/*****************************************************************************/
/*                               kdu_push_ifc                                */
/*****************************************************************************/

class kdu_push_ifc {
  /* [BIND: reference]
     [SYNOPSIS]
     All classes which support `push' calls derive from this class so
     that the caller may remain ignorant of the specific type of object to
     which samples are being delivered.  The purpose of derivation is usually
     just to introduce the correct constructor.  The objects are actually just
     interfaces to an appropriate object created by the relevant derived
     class's constructor.  The interface directs `push' calls to the internal
     object in a manner which should incur no cost.
     [//]
     The interface objects may be copied at will; the internal object will
     not be destroyed when an interface goes out of scope.  Consequently,
     the interface objects do not have meaningful destructors.  Instead,
     to destroy the internal object, the `destroy' member function must be
     called explicitly.
  */
  public: // Member functions
    kdu_push_ifc() { state = NULL; }
      /* [SYNOPSIS]
           Creates an empty interface.  You must assign the interface to
           one of the derived objects such as `kdu_encoder' or `kdu_analysis'
           before you may use the `push' function.
      */
    void destroy()
      {
        if (state != NULL) delete state;
        state = NULL;
      }
      /* [SYNOPSIS]
           Automatically destroys all objects which were created by the
           relevant derived object's constructor, including lower level
           DWT analysis stages and encoding objects.  Upon return, the
           interface will be empty, meaning that `exists' returns
           false.
      */
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
           Returns false until the object is assigned to one constructed
           by one of the derived classes, `kdu_analysis' or `kdu_encoder'.
           Also returns false after `destroy' returns.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
           Opposite of `exists', returning true if the object has not been
           assigned to one of the more derived objects, `kdu_analysis' or
           `kdu_encoder', or after a call to `destroy'.
      */
    kdu_push_ifc &operator=(kdu_analysis rhs);
      /* [SYNOPSIS]
           It is safe to directly assign a `kdu_analysis' object to a
           `kdu_push_ifc' object, since both are essentially just references
           to an anonymous internal object.
      */
    kdu_push_ifc &operator=(kdu_encoder rhs);
      /* [SYNOPSIS]
           It is safe to directly assign a `kdu_encoder' object to a
           `kdu_push_ifc' object, since both are essentially just references
           to an anonymous internal object.
      */
    void push(kdu_line_buf &line, kdu_thread_env *env=NULL)
      { /* [SYNOPSIS]
             Delivers all samples from the `line' buffer across the interface.
           [ARG: env]
             If the object was constructed for multi-threaded processing
             (see the constructors for `kdu_analysis' and `kdu_encoder'),
             you MUST pass a non-NULL `env' argument in here, identifying
             the thread which is performing the `push' call.  Otherwise,
             the `env' argument should be ignored.
        */
        state->push(line,env);
      }
    void push(kdu_line_buf &line, bool allow_exchange)
      { /* [SYNOPSIS]
             This version of the `push' function is provided only for
             backward compatibility with Kakadu versions 4.5 and earlier. */
        state->push(line,NULL);
      }
  protected: // Data
    kdu_push_ifc_base *state;
  };

/*****************************************************************************/
/*                            kdu_pull_ifc_base                              */
/*****************************************************************************/

class kdu_pull_ifc_base {
  protected:
    friend class kdu_pull_ifc;
    virtual ~kdu_pull_ifc_base() { return; }
    virtual void start(kdu_thread_env *env) = 0;
    virtual void pull(kdu_line_buf &line, kdu_thread_env *env) = 0;
  };

/*****************************************************************************/
/*                               kdu_pull_ifc                                */
/*****************************************************************************/

class kdu_pull_ifc {
  /* [BIND: reference]
     [SYNOPSIS]
     All classes which support `pull' calls derive from this class so
     that the caller may remain ignorant of the specific type of object from
     which samples are to be recovered.  The purpose of derivation is usually
     just to introduce the correct constructor.  The objects are actually just
     interfaces to an appropriate object created by the relevant derived
     class's constructor.  The interface directs `pull' calls to the internal
     object in a manner which should incur no cost.
     [//]
     The interface objects may be copied at will; the internal object will
     not be destroyed when an interface goes out of scope.  Consequently,
     the interface objects do not have meaningful destructors.  Instead,
     to destroy the internal object, the `destroy' member function must be
     called explicitly.
  */
  public: // Member functions
    kdu_pull_ifc() { state = NULL; }
      /* [SYNOPSIS]
           Creates an empty interface.  You must assign the interface to
           one of the derived objects such as `kdu_decoder' or `kdu_synthesis'
           before you may use the `pull' function.
      */
    void destroy()
      {
        if (state != NULL) delete state;
        state = NULL;
      }
      /* [SYNOPSIS]
           Automatically destroys all objects which were created by this
           relevant derived object's constructor, including lower level
           DWT synthesis stages and decoding objects.  Upon return, the
           interface will be empty, meaning that `exists' returns false.
      */
    void start(kdu_thread_env *env)
      { /* [SYNOPSIS]
             This function may be called at any point after construction
             of a `kdu_synthesis' or `kdu_decoder' object, once you have
             invoked the `kdu_sample_allocator::finalize' function on the
             `kdu_sample_allocator' object used during construction.  In
             particular, this means that you will not be creating any
             further objects to share the storage offered by the sample
             allocator.
             [//]
             It is never necessary to call this function, since it will be
             invoked automatically, if required, when `pull' is first called.
             Indeed for applications which are not multi-threaded (i.e.,
             when `env' is NULL) it is pretty pointless to call this function,
             but you can if you like.
             [//]
             For multi-threaded applications (i.e., when `env' is non-NULL),
             this function enables you to get the most benefit from
             multi-threading, since it allows the code-block processing
             associated with any number of tile-component-subbands to be
             started immediately.  By contrast, the `pull' function will
             not be invoked on a subband's `kdu_decoder' object until
             dependencies have been satisfied in other subbands, which
             cannot generally happen until a full row of code-blocks have
             been decoded in a first subband.
        */
        state->start(env);
      }
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
           Returns false until the object is assigned to one constructed
           by one of the derived classes, `kdu_synthesis' or `kdu_decoder'.
           Also returns false after `destroy' returns.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
           Opposite of `exists', returning true if the object has not been
           assigned to one of the more derived objects, `kdu_synthesis' or
           `kdu_decoder', or after a call to `destroy'.
      */
    kdu_pull_ifc &operator=(kdu_synthesis rhs);
      /* [SYNOPSIS]
           It is safe to directly assign a `kdu_synthesis' object to a
           `kdu_pull_ifc' object, since both are essentially just references
           to an anonymous internal object.
      */
    kdu_pull_ifc &operator=(kdu_decoder rhs);
      /* [SYNOPSIS]
           It is safe to directly assign a `kdu_decoder' object to a
           `kdu_pull_ifc' object, since both are essentially just references
           to an anonymous internal object.
      */
    void pull(kdu_line_buf &line, kdu_thread_env *env=NULL)
      { /* [SYNOPSIS]
             Fills out the supplied `line' buffer's sample values before
             returning.
           [ARG: env]
             If the object was constructed for multi-threaded processing
             (see the constructors for `kdu_synthesis' and `kdu_decoder'),
             you MUST pass a non-NULL `env' argument in here, identifying
             the thread which is performing the `pull' call.  Otherwise,
             the `env' argument should be ignored.
        */
        state->pull(line,env);
      }
    void pull(kdu_line_buf &line, bool allow_exchange)
      { /* [SYNOPSIS]
             This version of the `pull' function is provided only for
             backward compatibility with Kakadu versions 4.5 and earlier. */
        state->pull(line,NULL);
      }
  protected: // Data
    kdu_pull_ifc_base *state;
  };

/*****************************************************************************/
/*                              kdu_analysis                                 */
/*****************************************************************************/

class kdu_analysis : public kdu_push_ifc {
  /* [BIND: reference]
     [SYNOPSIS]
     Implements the subband analysis processes associated with a single
     DWT node (see `kdu_node').  A complete DWT decomposition tree is built
     from a collection of these objects, each containing a reference to
     the next stage.   The complete DWT tree and all required `kdu_encoder'
     objects may be created by a single call to the constructor,
     `kdu_analysis::kdu_analysis'.
  */
  public: // Member functions
    KDU_EXPORT
      kdu_analysis(kdu_node node, kdu_sample_allocator *allocator,
                   bool use_shorts, float normalization=1.0F,
                   kdu_roi_node *roi=NULL, kdu_thread_env *env=NULL,
                   kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Constructing an instance of this class for the primary node of
           a tile-component's highest visible resolution, will cause the
           constructor to recursively create instances of the class for
           each successive DWT stage and also for the block encoding process.
           [//]
           The recursive construction process supports all wavelet
           decomposition structures allowed by the JPEG2000 standard,
           including packet wavelet transforms, and transforms with
           different horizontal and vertical downsampling factors.  The
           `node' object used to construct the top level `kdu_analysis'
           object will typically be the primary node of a `kdu_resolution'
           object, obtained by calling `kdu_resolution::access_node'.  In
           fact, for backward compatibility with Kakadu versions 4.5 and
           earlier, a second constructor is provided, which does just this.
           [//]
           From Kakadu v5.1, this function takes optional `env' and
           `env_queue' arguments to support a variety of multi-threaded
           processing paradigms, to leverage the capabilities of
           multi-processor platforms.  To see how this works, consult the
           description of these arguments below.
         [ARG: node]
           Interface to the DWT decomposition node for which the object is
           being created.  The analysis stage decomposes the image entering
           that node into one subband for each of the node's children.
           If the child node is a leaf (a final subband), a `kdu_encoder'
           object is created to receive the data produced in that subband.
           Otherwise, another `kdu_analysis' object is recursively
           constructed to process the subband data produced by the present
           node.
         [ARG: allocator]
           A `kdu_sample_allocator' object whose `finalize' member function
           has not yet been called must be supplied for pre-allocation of the
           various sample buffering arrays.  This same allocator will be
           shared by the entire DWT tree and by the `kdu_encoder' objects at
           its leaves.
         [ARG: use_shorts]
           Indicates whether 16-bit or 32-bit data representations are to be
           used.  The same type of representation must be used throughput the
           DWT processing chain and line buffers pushed into the DWT engine
           must use this representation.
         [ARG: normalization]
           Ignored for reversibly transformed data.  In the irreversible case,
           it indicates that the nominal range of data pushed into the
           `kdu_push_ifc::push' function will be from -0.5*R to 0.5*R, where
           R is the value of the `normalization' argument.  This
           capability is provided primarily to allow normalization steps to
           be skipped or approximated with simple powers of 2 during lifting
           implementations of the DWT; the factors can be folded into
           quantization step sizes.  The best way to use the normalization
           argument will generally depend upon the implementation of the DWT.
         [ARG: roi]
           If non-NULL, this argument points to an appropriately
           derived ROI node object, which may be used to recover region of
           interest mask information for the present tile-component.  In this
           case, the present function will automatically construct an ROI
           processing tree to provide access to derived ROI information in
           each individual subband.  The `roi::release' function will
           be called when the present object is destroyed -- possibly
           sooner (if it can be determined that ROI information is no
           longer required).
         [ARG: env]
           Supply a non-NULL argument here if you want to enable
           multi-threaded processing.  After creating a cooperating
           thread group by following the procedure outlined in the
           description of `kdu_thread_env', any one of the threads may
           be used to construct this processing engine.  However, you
           MUST TAKE CARE to create all objects which share the same
           `allocator' object from the same thread.
           [//]
           Separate processing queues will automatically be created for each
           subband, allowing multiple threads to be scheduled
           simultaneously to process code-block data for the corresponding
           tile-component.  Also, multiple tile-components may be processed
           concurrently and the available thread resources will be allocated
           amongst the total collection of job queues as required, in such
           a way as to encourage memory locality while keeping all threads
           active as much of the time as possible.
           [//]
           By and large, you can use this object in exactly the same way
           when `env' is non-NULL as you would with a NULL `env' argument.
           That is, the use of multi-threaded processing can be largely
           transparent.  However, you must remember the following two
           points:
           [>>] You must supply a non-NULL `env' argument to the
                `kdu_push_ifc::push' function -- it need not refer to the
                same thread as the one used to create the object here, though.
           [>>] You cannot rely upon all processing being complete until you
                invoke the `kdu_thread_entity::synchronize' or
                `kdu_thread_entity::terminate' function.  If you are
                considering using the `kdu_thread_entity::synchronize'
                function to synchronize all processing prior to running
                some job (e.g., incremental flushing with
                `kdu_codestream::flush'), you might want to
                seriously consider the advantages of registering a
                synchronized job to do this in the background -- see
                `kdu_thread_entity::register_synchronized_job'.
         [ARG: env_queue]
           This argument is ignored unless `env' is non-NULL.  When `env'
           is non-NULL, all job queues which are created inside this object
           are added as sub-queues of `env_queue' (or as top-level queues
           if `env_queue' is NULL).  Typically, `env_queue' will be used
           to run the jobs which call the `kdu_push_ifc::push' function, so
           that the DWT analysis tasks associated with different
           tile-components can be assigned to different processing threads.
           However, it is not necessary to run jobs from `env_queue'.
           [//]
           One advantage of supplying a non-NULL `env_queue' is that it
           provides you with a single hook for synchronizing and/or
           terminating all the job queues which are created by this object
           and its descendants -- see `kdu_thread_entity::synchronize',
           `kdu_thread_entity::register_synchronized_job' and
           `kdu_thread_entity::terminate' for more on this.
           [//]
           A second advantage of supplying a non-NULL `env_queue' is that
           it can improve the efficiency of Kakadu's thread scheduling
           algorithm.  This algorithm tries to keep each thread working in
           the same queue as it was previously working, so long as there
           are jobs.  Failing this, it tries to assign the thread to a
           queue with the closest possible common ancestor to the one it
           is currently working in.  Thus, by supplying a unique `env_queue'
           when calling this constructor, you can be sure that threads which
           are processing this tile-component will continue to do so until
           there is no more work to be done -- this reduces the likelihood
           of cache misses, since all of the working memory associated with
           a tile-component is allocated in a single contiguous block,
           managed by the `allocator' object.
      */
    KDU_EXPORT
      kdu_analysis(kdu_resolution resolution,kdu_sample_allocator *allocator,
                   bool use_shorts, float normalization=1.0,
                   kdu_roi_node *roi=NULL, kdu_thread_env *env=NULL,
                   kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Same as the first form of the constructor, but the required
           `kdu_node' interface is recovered from `resolution.access_node'.
         [ARG: resolution]
           Interface to the top visible resolution level from which
           DWT analysis is to be performed.  For analysis, this should almost
           invariably be the actual top level resolution of a tile-component.
           For synthesis, the corresponding constructor might be supplied
           a lower resolution object in order to obtain partial synthesis
           to that resolution.
      */
  };

/*****************************************************************************/
/*                              kdu_synthesis                                */
/*****************************************************************************/

class kdu_synthesis : public kdu_pull_ifc {
  /* [BIND: reference]
     [SYNOPSIS]
     Implements the subband synthesis processes associated with a single
     DWT node (see `kdu_node').  A complete DWT synthesis tree is built
     from a collection of these objects, each containing a reference to
     the next stage.   The complete DWT tree and all required `kdu_decoder'
     objects may be created by a single call to the constructor,
     `kdu_synthesis::kdu_synthesis'.
  */
  public: // Member functions
    KDU_EXPORT
      kdu_synthesis(kdu_node node, kdu_sample_allocator *allocator,
                    bool use_shorts, float normalization=1.0F,
                    int pull_offset=0, kdu_thread_env *env=NULL,
                    kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Constructing an instance of this class for the primary node of
           a tile-component's highest visible resolution, will cause the
           constructor to recursively create instances of the class for
           each successive DWT stage and also for the block decoding process.
           [//]
           The recursive construction process supports all wavelet
           decomposition structures allowed by the JPEG2000 standard,
           including packet wavelet transforms, and transforms with
           different horizontal and vertical downsampling factors.  The
           `node' object used to construct the top level `kdu_synthesis'
           object will typically be the primary node of a `kdu_resolution'
           object, obtained by calling `kdu_resolution::access_node'.  In
           fact, for backward compatibility with Kakadu versions 4.5 and
           earlier, a second constructor is provided, which does just this.
           [//]
           From Kakadu v5.1, this function takes optional `env' and
           `env_queue' arguments to support a variety of multi-threaded
           processing paradigms, to leverage the capabilities of
           multi-processor platforms.  To see how this works, consult the
           description of these arguments below.  To initiate processing
           as soon as possible, you might like to call `kdu_pull_ifc::start'
           once you have finished creating all objects which share the
           supplied `allocator' object and invoked its
           `kdu_sample_allocator::finalize' function.  Otherwise, background
           processing (on other threads) will not commence until the first
           call to `kdu_pull_ifc::pull'.
         [ARG: node]
           Interface to the DWT decomposition node for which the object is
           being created.  The synthesis stage reconstructs the image
           associated with that node by combining the subband images
           produced by each of the node's children.  If the child node is
           a leaf (a final subband), a `kdu_decoder' object is created to
           recover the data for that subband.  Otherwise, another
           `kdu_synthesis' object is recursively constructed to retrieve
           the child's subband data.
         [ARG: allocator]
           A `kdu_sample_allocator' object whose `finalize' member function
           has not yet been called must be supplied for pre-allocation of the
           various sample buffering arrays.  This same allocator will be
           shared by the entire DWT tree and by the `kdu_decoder' objects at
           its leaves.
         [ARG: use_shorts]
           Indicates whether 16-bit or 32-bit data representations are to be
           used.  The same type of representation must be used throughput the
           DWT processing chain and line buffers pulled from the DWT synthesis
           engine must use this representation.
         [ARG: normalization]
           Ignored for reversibly transformed data.  In the irreversible case,
           it indicates that the nominal range of data recovered from the
           `kdu_pull_ifc::pull' function will be from -0.5*R to 0.5*R, where
           R is the value of the `normalization' argument.  This
           capability is provided primarily to allow normalization steps to
           be skipped or approximated with simple powers of 2 during lifting
           implementations of the DWT; the factors can be folded into
           quantization step sizes.  The best way to use the normalization
           argument will generally depend upon the implementation of the DWT.
         [ARG: pull_offset]
           Applications should leave this argument set to 0.  The internal
           implementation uses this to maintain horizontal alignment
           properties for efficient memory access, when synthesizing a
           region of interest within the image.  The first `pull_offset'
           entries in each `kdu_line_buf' object supplied to the `pull'
           function are not used; the function should write the requested
           sample values into the remainder of the line buffer, whose
           width (`kdu_line_buf::get_width') is guaranteed to be `pull_offset'
           samples larger than the width of the region in that subband.
           In any event, offsets should be small, since the internal
           representation stores them and various derived quantities
           using 8-bit fields to keep the memory footprint as small
           as possible.
         [ARG: env]
           Supply a non-NULL argument here if you want to enable
           multi-threaded processing.  After creating a cooperating
           thread group by following the procedure outlined in the
           description of `kdu_thread_env', any one of the threads may
           be used to construct this processing engine.  However, you
           MUST TAKE CARE to create all objects which share the same
           `allocator' object from the same thread.
           [//]
           Separate processing queues will automatically be created for each
           subband, allowing multiple threads to be scheduled simultaneously
           to process code-block samples for the corresponding
           tile-component.  Also, multiple tile-components may be processed
           concurrently and the available thread resources will be allocated
           amongst the total collection of job queues as required, in such
           a way as to encourage memory locality while keeping all threads
           active as much of the time as possible.
           [//]
           By and large, you can use this object in exactly the same way
           when `env' is non-NULL as you would with a NULL `env' argument.
           That is, the use of multi-threaded processing can be largely
           transparent.  However, you must remember the following two
           points:
           [>>] You must supply a non-NULL `env' argument to the
                `kdu_pull_ifc::pull' function -- it need not refer to the
                same thread as the one used to create the object here, though.
           [>>] You cannot rely upon all processing being complete until you
                invoke the `kdu_thread_entity::synchronize' or
                `kdu_thread_entity::terminate' function.
         [ARG: env_queue]
           This argument is ignored unless `env' is non-NULL.  When `env'
           is non-NULL, all job queues which are created inside this object
           are added as sub-queues of `env_queue' (or as top-level queues
           if `env_queue' is NULL).  Typically, `env_queue' will be used
           to run the jobs which call the `kdu_pull_ifc::pull' function, so
           that the DWT synthesis tasks associated with different
           tile-components can be assigned to different processing threads.
           However, it is not necessary to run jobs from `env_queue'.
           [//]
           One advantage of supplying a non-NULL `env_queue' is that it
           provides you with a single hook for synchronizing and/or
           terminating all the job queues which are created by this object
           and its descendants -- see `kdu_thread_entity::synchronize',
           `kdu_thread_entity::register_synchronized_job' and
           `kdu_thread_entity::terminate' for more on this.
           [//]
           A second advantage of supplying a non-NULL `env_queue' is that
           it can improve the efficiency of Kakadu's thread scheduling
           algorithm.  This algorithm tries to keep each thread working in
           the same queue as it was previously working, so long as there
           are jobs.  Failing this, it tries to assign the thread to a
           queue with the closest possible common ancestor to the one it
           is currently working in.  Thus, by supplying a unique `env_queue'
           when calling this constructor, you can be sure that threads which
           are processing this tile-component will continue to do so until
           there is no more work to be done -- this reduces the likelihood
           of cache misses, since all of the working memory associated with
           a tile-component is allocated in a single contiguous block,
           managed by the `allocator' object.
      */
    KDU_EXPORT
      kdu_synthesis(kdu_resolution resolution, kdu_sample_allocator *allocator,
                    bool use_shorts, float normalization=1.0F,
                    kdu_thread_env *env=NULL,
                    kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Same as the first form of the constructor, but the required
           `kdu_node' interface is recovered from `resolution.access_node'.
         [ARG: resolution]
           Interface to the top visible resolution level to which DWT synthesis
           is to be performed.  This need not necessarily be the
           highest available resolution, so that partial synthesis to some
           lower resolution is supported -- in fact, common in Kakadu.
      */
  };

/*****************************************************************************/
/*                              kdu_encoder                                  */
/*****************************************************************************/

class kdu_encoder: public kdu_push_ifc {
    /* [BIND: reference]
       [SYNTHESIS]
       Implements quantization and block encoding for a single subband,
       inside a single tile-component.
    */
  public: // Member functions
    KDU_EXPORT
      kdu_encoder(kdu_subband subband, kdu_sample_allocator *allocator,
                  bool use_shorts, float normalization=1.0F,
                  kdu_roi_node *roi=NULL, kdu_thread_env *env=NULL,
                  kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Informs the encoder that data supplied via its `kdu_push_ifc::push'
           function will have a nominal range from -0.5*R to +0.5*R where R
           is the value of `normalization'.  The `roi' argument, if non-NULL,
           provides an appropriately derived `kdu_roi_node' object whose
           `kdu_roi_node::pull' function may be used to recover ROI mask
           information for this subband.  Its `kdu_roi::release' function will
           be called when the encoder is destroyed -- possibly sooner, if it
           can be determined that ROI information is no longer required.
           [//]
           From Kakadu v5.1, this function takes optional `env' and
           `env_queue' arguments to support a variety of multi-threaded
           processing paradigms, to leverage the capabilities of
           multi-processor platforms.  To see how this works, consult the
           description of these arguments below.
         [ARG: env]
           If non-NULL, the behaviour of the underlying
           `kdu_push_ifc::push' function is changed radically.  In
           particular, a job queue is created by this constructor, to enable
           asynchronous multi-threaded processing of the code-block samples.
           Once sufficient lines have been pushed to the subband to enable
           the encoding of a row of code-blocks, the processing of these
           code-blocks is not done immediately, as it is if `env' is NULL.
           Instead, one or more jobs are added to the mentioned queue,
           to be serviced by any available thread in the group to which
           `env' belongs.  You should remember the following three
           points:
           [>>] All objects which share the same `allocator' must be created
                using the same thread.  Thereafter, the actual processing
                may proceed on different threads.
           [>>] You must supply a non-NULL `env' argument to the
                `kdu_push_ifc::push' function -- it need not refer to the
                same thread as the one used to create the object here, though.
           [>>] You cannot rely upon all processing being complete until you
                invoke the `kdu_thread_entity::synchronize' or
                `kdu_thread_entity::terminate' function.
         [ARG: env_queue]
           If `env' is NULL, this argument is ignored; otherwise, the job
           queue which is created by this constructor will be made a
           sub-queue of any supplied `env_queue'.  If `env_queue' is NULL,
           the queue created to process code-blocks within this
           tile-component-subband will be a top-level queue in the
           multi-threaded queue hierarchy.
      */
  };

/*****************************************************************************/
/*                              kdu_decoder                                  */
/*****************************************************************************/

class kdu_decoder: public kdu_pull_ifc {
    /* [BIND: reference]
       [SYNOPSIS]
       Implements the block decoding for a single subband, inside a single
       tile-component.
    */
  public: // Member functions
    KDU_EXPORT
      kdu_decoder(kdu_subband subband, kdu_sample_allocator *allocator,
                  bool use_shorts, float normalization=1.0F,
                  int pull_offset=0, kdu_thread_env *env=NULL,
                  kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Informs the decoder that data retrieved via its `kdu_pull_ifc::pull'
           function should have a nominal range from -0.5*R to +0.5*R, where
           R is the value of `normalization'.
           [//]
           The `pull_offset' member should be left as zero when invoking
           this constructor directly from an application.  Internally,
           however, when a `kdu_decoder' object must be constructed within
           a `kdu_synthesis' object, the `pull_offset' value may be set to
           a non-zero value to ensure alignment properties required for
           efficient memory access during horizontal DWT synthesis.  When
           this happens, the width of the line buffer supplied to `pull',
           as returned via `kdu_line_buf::get_width' will be `pull_offset'
           samples larger than the actual width of the subband data being
           requested, and the data will be written starting from location
           `pull_offset', rather than location 0.
           [//]
           From Kakadu v5.1, this function takes optional `env' and
           `env_queue' arguments to support a variety of multi-threaded
           processing paradigms, to leverage the capabilities of
           multi-processor platforms.  To see how this works, consult the
           description of these arguments below.  Note, in particular,
           however, that to get the most out of multi-threaded processing,
           you should call `kdu_pull_ifc::start' once you have finished
           creating all objects which share the supplied `allocator'
           object and invoked its `kdu_sample_allocator::finalize' function.
         [ARG: env]
           If non-NULL, the behaviour of the underlying
           `kdu_pull_ifc::pull' function is changed radically.  In
           particular, a job queue is created by this constructor, to enable
           asynchronous multi-threaded processing of the code-block samples.
           Processing of code-blocks commences once the first call to
           `kdu_pull_ifc::pull' or `kdu_pull_ifc::start' arrives.  The latter
           approach is preferred, since it allows parallel processing of
           the various subbands in a tile-component to commence immediately
           without waiting for DWT dependencies to be satisfied.  You should
           remember the following two points:
           [>>] All objects which share the same `allocator' must be created
                using the same thread.  Thereafter, the actual processing
                may proceed on different threads.
           [>>] You must supply a non-NULL `env' argument to the
                `kdu_pull_ifc::pull' function -- it need not refer to the
                same thread as the one used to create the object here, though.
         [ARG: env_queue]
           If `env' is NULL, this argument is ignored; otherwise, the job
           queue which is created by this constructor will be made a
           sub-queue of any supplied `env_queue'.  If `env_queue' is NULL,
           the queue created to process code-blocks within this
           tile-component-subband will be a top-level queue in the
           multi-threaded queue hierarchy, which is less desirable for
           reasons explained in connection with the `kdu_synthesis'
           object's constructor.
      */
  };

/*****************************************************************************/
/*                          kd_multi_analysis_base                           */
/*****************************************************************************/

class kd_multi_analysis_base {
  public: // Member functions
    virtual ~kd_multi_analysis_base() { return; }
    virtual void terminate_queues(kdu_thread_env *env) = 0;
      /* Called prior to the destructor if `env' is non-NULL, this function
         terminates the queues created for multi-threading. */
    virtual kdu_coords get_size(int comp_idx) = 0;
    virtual kdu_line_buf *
      exchange_line(int comp_idx, kdu_line_buf *written,
                    kdu_thread_env *env) = 0;
    virtual bool is_line_precise(int comp_idx) = 0;
    virtual bool is_line_absolute(int comp_idx) = 0;
  };

/*****************************************************************************/
/*                           kdu_multi_analysis                              */
/*****************************************************************************/

class kdu_multi_analysis {
    /* [BIND: interface]
       [SYNOPSIS]
         This powerful object generalizes the functionality of `kdu_analysis'
         to the processing of multiple image components, allowing all the
         data for a tile to be managed by a single object.  The object
         creates the `kdu_analysis' objects required to process each
         codestream image component, but it also implements Part-1
         colour decorrelation transforms and Part-2 generalized
         multi-component transforms, as required.
         [//]
         Objects of this class serve as interfaces.  The constructor
         simply creates an empty interface, and there is no meaningful
         destructor.  This means that you may copy and transfer objects
         of this class at will, without any impact on internal resources.
         To create a meaningful insance of the internal machine, you must
         use the `create' member.  To destroy the internal machine you
         must use the `destroy' member.
    */
    public: // Member functions
      kdu_multi_analysis() { state = NULL; }
        /* [SYNOPSIS]
             Leaves the interface empty, meaning that the `exists' member
             returns false.
        */
      bool exists() { return (state != NULL); }
        /* [SYNOPSIS]
             Returns true after `create' has been called; you may also
             copy an interface whose `create' function has been called. */
      bool operator!() { return (state == NULL); }
        /* [SYNOPSIS]
             Opposite of `exists', returning false if the object represents
             an interface to instantiated internal processing machinery.
        */
      KDU_EXPORT kdu_long
        create(kdu_codestream codestream, kdu_tile tile,
               bool force_precise=false, kdu_roi_image *roi=NULL,
               bool want_fastest=false, int processing_stripe_height=1,
               kdu_thread_env *env=NULL, kdu_thread_queue *env_queue=NULL,
               bool double_buffering=false);
        /* [SYNOPSIS]
             Use this function to create an instance of the internal
             processing machinery, for compressing data associated with
             the supplied open tile interface.  Until you call this
             function (or copy another object which has been created), the
             `exists' function will return false.
             [//]
             It is worth noting that multi-component transformations
             performed by this function are affected by any previous
             calls to `kdu_tile::set_components_of_interest'.  In
             particular, you need only supply those components which have
             been marked of interest via the `exchange_line' function.
             Those components marked as uninteresting are ignored -- you can
             pass them in via `exchange_line' if you like, but they will
             have no impact on the way in which codestream components
             are generated and subjected to spatial wavelet transformation
             and coding.
             [//]
             If insufficient components are currently marked as being
             of interest (i.e., too many components were excluded in a
             previous call to `kdu_tile::set_components_of_interest'), the
             present object might not be able to find a way of inverting
             the multi-component transformation network, so as to work back
             to codestream image components.  In this case, an informative
             error message will be generated through `kdu_error'.
             [//]
             From Kakadu v5.1, this function takes optional `env' and
             `env_queue' arguments to support a variety of multi-threaded
             processing paradigms, to leverage the capabilities of
             multi-processor platforms.  To see how this works, consult the
             description of these arguments below.  Also, play close attention
             to the use of `env' arguments with the `exchange_line' and
             `destroy' functions.
           [RETURNS]
             Returns the number of bytes which have been allocated internally
             for the processing of multi-component transformations,
             spatial wavelet transforms and intermediate buffering between
             the wavelet and block coder engines.  Essentially, this
             includes all memory resources, except for those managed by the
             `kdu_codestream' machinery (for structural information and
             code-block bit-streams).  The latter information can be recovered
             by querying the `kdu_codestream::get_compressed_data_memory' and
             `kdu_codestream::get_compressed_state_memory' functions.
           [ARG: force_precise]
             If this is true, the internal machinery will be configured to
             work with 32-bit representations for all image component samples,
             all the way through to the production of spatial subbands
             for encoding.  Otherwise, the internal machinery will determine
             a suitable representation precision, making every attempt to
             use lower precision implementations, which are faster and consume
             less memory, where this does not unduly compromise quality.
           [ARG: want_fastest]
             This argument represents the opposite extreme in precision
             selection to that represented by `force_precise'.  The argument
             is ignored if `force_precise' is true.  If `want_fastest' is
             true and `force_precise' is false, the function selects the
             lowest internal precision possible, even if this does sacrifice
             image quality.  This also generally results in the fastest
             possible implementation.  The `want_fastest' option usually
             makes more sense for decompression (see
             `kdu_multi_synthesis::create') than it does for compression.
             Even in compression applications, however, it can make
             sense to perform irreversible compression of high bit-depth
             imagery using a lower internal precision, if you know that
             you are not going to be targeting a very high compressed
             bit-rate.  It can happen, for example, that you want to
             compress 16-bit imagery, with no more fidelity than you
             would be interested in typically for 8-bit imagery.  In that
             case, you might as well use the same internal implementation
             precision you would use for 8 or 10-bit/sample imagery if
             speed is important to you.
             [//]
             Essentially, this argument forces the selection of the lower
             precision 16-bit internal representations, for all processing
             which involves irreversible transforms -- reversible transforms
             must currently be implemented using the required precision,
             or else numerical overflow might result.  
           [ARG: roi]
             A non-NULL object may be passed in via this argument to
             allow for region-of-interest driven encoding.  Note carefully,
             however, that the component indices supplied to the
             `kdu_roi_image::acquire_node' function correspond to
             codestream image components.  It is up to you to ensure that
             the correct geometry is returned for each codestream image
             component, in the event that the source image components do not
             map directly (component for component) to codestream image
             components.
           [ARG: processing_stripe_height]
             This argument must be strictly positive.  It identifies the
             number of row buffers maintained internally for each codestream
             component.  As codestream component lines are generated they
             are temporarily buffered until the stripe buffer is full.  At
             that point all of the buffered rows are passed consecutively
             to the spatial DWT analysis and block encoding processes.  Larger
             stripe buffers have the potential to reduce context switching
             overhead when working with a large number of components or
             a large number of tiles.  On the other hand, the price you
             pay for this is increased memory consumption.  For
             single-threaded processing, a stripe height of 1 is usually
             fine (although perhaps not quite optimal).  For multi-threaded
             processing with the `double_buffering' option set to true,
             however, it is advisable to use a larger stripe height
             (e.g., 8 or even 16) since the thread context switching
             overhead is considerably higher than that of individual
             function calls within a single thread.
           [ARG: env]
             Supply a non-NULL argument here if you want to enable
             multi-threaded processing.  After creating a cooperating
             thread group by following the procedure outlined in the
             description of `kdu_thread_env', any one of the threads may
             be used to construct this processing engine.  Separate
             processing queues will automatically be created for each
             image component.  If `double_buffering' is set to true, these
             queues will also be used to schedule the spatial wavelet
             transform operations associated with each image component
             as jobs to be processed asynchronously by different threads.
             Regardless of whether `double_buffering' is true or false,
             within each tile-component, separate queues are created to allow
             simultaneous processing of code-blocks from different subbands.
             [//]
             By and large, you can use this object in exactly the same way
             when `env' is non-NULL as you would with a NULL `env' argument.
             That is, the use of multi-threaded processing can be largely
             transparent.  However, you must remember the following two
             points:
             [>>] You must supply a non-NULL `env' argument to the
                  `exchange_line' function -- it need not refer to the same
                  thread as the one used to create the object here, though.
             [>>] You cannot rely upon all processing being complete until you
                  invoke the `kdu_thread_entity::synchronize' or
                  `kdu_thread_entity::terminate' function, or until you
                  call `destroy' with a non-NULL `env' argument.  If you are
                  considering using the `kdu_thread_entity::synchronize'
                  function to synchronize all processing prior to running
                  some job (e.g., incremental flushing with
                  `kdu_codestream::flush'), you might want to
                  seriously consider the advantages of registering a
                  synchronized job to do this in the background -- see
                  `kdu_thread_entity::register_synchronized_job'.
           [ARG: env_queue]
             This argument is ignored unless `env' is non-NULL.  When `env'
             is non-NULL, all job queues which are created inside this object
             are added as sub-queues of `env_queue' (or as top-level queues
             if `env_queue' is NULL).  If you do supply a non-NULL `env_queue',
             it is best to use a separate `env_queue' (obtained using
             `kdu_thread_entity::add_queue') for each tile processing engine
             which you want to keep active concurrently.  That way, you can
             individually wait upon completion of the activities within the
             engine, using `kdu_thread_entity::synchronize'; also, with a
             separate high-level queue for each tile engine, you can terminate
             all of the queues created for that engine, without affecting any
             others, by passing the `env_queue' argument supplied here into
             `kdu_thread_entity::terminate'.
             [//]
             Another advantage of supplying a separate `env_queue' for each
             tile processing engine is that it can improve the efficiency
             of Kakadu's thread scheduling algorithm.  This algorithm tries to
             keep each thread working in the same queue as it was previously
             working, so long as there are jobs.  Failing this, it tries to
             assign the thread to a queue with the closest possible common
             ancestor to the one it is currently working in.  Thus, by
             supplying a unique `env_queue' when calling this constructor,
             you can be sure that threads which are processing this tile
             will continue to do so until there is no more work to be done --
             this reduces the likelihood of cache misses, since all of the
             working memory associated with a tile is allocated in a single
             contiguous block, managed by the `allocator' object.
           [ARG: double_buffering]
             This argument is ignored unless `env' is non-NULL.  It allows
             you to determine whether the spatial DWT operations associated
             with each codestream image component should be
             executed as asynchronous jobs (via `kdu_thread_entity::add_jobs')
             or synchronously when new imagery is supplied via the
             `exchange_line' function.
             [//]
             In the latter case (i.e., `double_buffering'=false), the thread
             which calls `exchange_line' performs all multi-component and
             spatial transform processing, leaving only the CPU-intensive block
             coding operations for other threads.  This may leave these
             extra threads occasionally idle, particularly in systems which
             have a large number of physical processors.  Nevertheless,
             it is the simplest way to make good use of parallel processing.
             [//]
             If you set `double_buffering' to true, two sets of stripe buffers
             will be maintained for each codestream image component, each
             having `processing_stripe_height' rows.  New lines can be pushed
             to one stripe buffer while the other is being processed
             asynchronously by other threads.  This approach has the
             maximum potential to exploit the availability of parallel
             processing resources.  However, you must be careful to select
             a good value for `processing_stripe_height', since scheduling
             jobs onto threads incurs a non-negligible overhead.  This is
             particularly important if you have very narrow tiles.  One
             rough guide is to arrange for H*W to be around 2048 samples
             or more, where W is the tile width and H is the value of
             `processing_stripe_height'.  Thus, for small tiles of, say,
             128x128, you might select a stripe height of 16 rows.  In
             any case, with `double_buffering' set to true, it is a good
             idea (and not particularly costly in terms of memory, relative
             to other parts of the system) to set the stripe height to at
             least 8 rows.
             [//]
             At the end of the day, to get the most out of a multi-processor
             platform, you should be prepared to play around with the value of
             `processing_stripe_height' and compare the performance with that
             obtained when `double_buffering' is false.
        */
      void destroy(kdu_thread_env *env=NULL)
        {
          if (state != NULL)
            { state->terminate_queues(env); delete state; }
          state = NULL;
        }
        /* [SYNOPSIS]
             Use this function to destroy the internal processing machine
             created using `create'.  The function may be invoked on any
             copy of the original object whose `create' function was called,
             so be careful.
           [ARG: env]
             If this argument is non-NULL, you are inviting the current
             function to execute `kdu_thread_entity::terminate' on the
             `env_queue' which was originally supplied to `create', with
             a `leave_root' argument of true.  This causes all sub-queues
             of the `env_queue' supplied to `create' to be destroyed, after
             first waiting for all their jobs to complete.  This can be
             a very convenient way of ensuring proper synchronization and
             cleaning up all resources created by the `create' function.
             However, you need to be aware of the following caveats:
             [>>] If you have already used `kdu_thread_entity::terminate' to
                  destroy these same queues, or even the containing
                  `env_queue' which was passed to `create', you must supply
                  a NULL argument for `env' here, to avoid attempts to wait
                  on a deleted queue.
             [>>] If the `env_queue' supplied to `create' has other sub-queues,
                  in addition to those created by the `create' function,
                  passing a non-NULL `env' argument into the present function
                  will synchronize and delete those sub-queues as well -- of
                  course, this might be dangerous.  In particular, if the
                  `env_queue' argument supplied to `create' was NULL and you
                  supply a non-NULL `env' argument here, all queues in the
                  entire multi-threaded processing group will be synchronized
                  and deleted inside this function.  Of course, this might
                  well be exactly what you want.
        */
      kdu_coords get_size(int comp_idx) { return state->get_size(comp_idx); }
        /* [SYNOPSIS]
             This is a convenience function to return the size of the image
             component identified by `comp_idx', as seen within the present
             tile.  The same information may be obtained by invoking
             `kdu_codestream::get_tile_dims' with its `want_output_comps'
             argument set to true.
        */
      kdu_line_buf *exchange_line(int comp_idx, kdu_line_buf *written,
                                  kdu_thread_env *env=NULL)
        { return state->exchange_line(comp_idx,written,env); }
        /* [SYNOPSIS]
             Use this function to exchange image data with the processing
             engine.  If `written' is NULL, you are only asking for access
             to a line buffer, into which to write a new line of image
             data for the component in question.  Once you have written
             to the supplied line buffer, you pass it back as the `written'
             argument in a subsequent call to this function.  Regardless
             of whether `written' is NULL, the function returns a pointer
             to the single internal line buffer which it maintains for each
             original image component, if and only if that line buffer is
             currently available for writing.  It will not be available if
             the internal machinery is waiting for a line of another component
             before it can process the data which has already been supplied.
             Thus, if a newly written line can be processed immediately, the
             function will return a non-NULL pointer even in the call with
             `written' non-NULL.  If it must wait for other component lines
             to arrive, however, it will return NULL.  Once returning non-NULL,
             the function will continue to return the same line buffer at
             least until the next call which supplies a non-NULL `written'
             argument.  This is because the current line number is incremented
             only by calls which supply a non-NULL `written' argument.
             [//]
             Note that all lines processed by this function should have a
             signed representation, regardless of whether or not
             `kdu_codestream::get_signed' reports that they the components
             are signed.
           [RETURNS]
             Non-NULL if a line is available for writing.  That same line
             should be passed back to the function as its `written' argument
             in a subsequent call to the function (not necessarily the next
             one) in order to advance to a new line.  If the function returns
             NULL, you may have reached the end of the tile (you
             should know this), or else the object may be waiting for you
             to supply new lines for other image components which must
             be processed together with this one.
           [ARG: comp_idx]
             Index of the component for which a line is being written or
             requested.  This index must lie in the range 0 to Cs-1, where
             Cs is the `num_source_components' value supplied to
             `create'.
           [ARG: written]
             If non-NULL, this argument must be identical to the line buffer
             which was previously returned by the function, using the same
             `comp_idx' value.  In this case, the line is deemed to contain
             valid image data and the internal line counter for this component
             will be incremented before the function returns.  Otherwise,
             you are just asking the function to give you access to the
             internal line buffer so that you can write to it.
           [ARG: env]
             Must be non-NULL if and only if a non-NULL `env' argument was
             passed into `create'.  Any non-NULL `env' argument must identify
             the calling thread, which need not necessarily be the one used
             to create the object in the first place.
        */
      bool is_line_precise(int comp_idx)
        { return state->is_line_precise(comp_idx); }
        /* [SYNOPSIS]
             Returns true if the indicated line has been assigned a
             precise (32-bit) representation by the `create' function.
             Otherwise, calls to `exchange_line' will return lines which
             have a 16-bit representation.  This function is provided as
             a courtesy so that applications which need to allocate
             auxiliary lines with compatible precisions will be able to
             do so.
        */
      bool is_line_absolute(int comp_idx)
        { return state->is_line_absolute(comp_idx); }
        /* [SYNOPSIS]
             Returns true if the indicated line has been assigned a
             reversible (i.e., absolute integer) representation by the
             `create' function.  Otherwise, calls to `exchange_line' will
             return lines whose `kdu_line_buf::is_absolute' function
             returns false.  This function is provided as a courtesy, so
             that applications can know ahead of time what the type of the
             data associated with a line will be.  In the presence of
             multi-component transforms, this can be non-trivial to figure
             out based solely on the output component index.
        */
    private: // Data
      kd_multi_analysis_base *state;
  };

/*****************************************************************************/
/*                         kd_multi_synthesis_base                           */
/*****************************************************************************/

class kd_multi_synthesis_base {
  public: // Member functions
    virtual ~kd_multi_synthesis_base() { return; }
    virtual void terminate_queues(kdu_thread_env *env) = 0;
      /* Called prior to the destructor if `env' is non-NULL, this function
         terminates the queues created for multi-threading. */
    virtual kdu_coords get_size(int comp_idx) = 0;
    virtual kdu_line_buf *get_line(int comp_idx, kdu_thread_env *env) = 0;
    virtual bool is_line_precise(int comp_idx) = 0;
    virtual bool is_line_absolute(int comp_idx) = 0;
  };

/*****************************************************************************/
/*                           kdu_multi_synthesis                             */
/*****************************************************************************/

class kdu_multi_synthesis {
    /* [BIND: interface]
       [SYNOPSIS]
         This powerful object generalizes the functionality of `kdu_synthesis'
         to the processing of multiple image components, allowing all the
         data for a tile (or any subset thereof) to be reconstructed by a
         single object.  The object creates the `kdu_synthesis' objects
         required to process each required codestream image component, but it
         also inverts Part-1 colour decorrelation transforms (RCT and ICT)
         and Part-2 generalized multi-component transforms, as required.
         [//]
         Objects of this class serve as interfaces.  The constructor
         simply creates an empty interface, and there is no meaningful
         destructor.  This means that you may copy and transfer objects
         of this class at will, without any impact on internal resources.
         To create a meaningful insance of the internal machine, you must
         use the `create' member.  To destroy the internal machine you
         must use the `destroy' member.
    */
    public: // Member functions
      kdu_multi_synthesis() { state = NULL; }
        /* [SYNOPSIS]
             Leaves the interface empty, meaning that the `exists' member
             returns false.
        */
      bool exists() { return (state != NULL); }
        /* [SYNOPSIS]
             Returns true after `create' has been called; you may also
             copy an interface whose `create' function has been called. */
      bool operator!() { return (state == NULL); }
        /* [SYNOPSIS]
             Opposite of `exists', returning false if the object represents
             an interface to instantiated internal processing machinery.
        */
      KDU_EXPORT kdu_long
        create(kdu_codestream codestream, kdu_tile tile,
               bool force_precise=false, bool skip_ycc=false,
               bool want_fastest=false, int processing_stripe_height=1,
               kdu_thread_env *env=NULL, kdu_thread_queue *env_queue=NULL,
               bool double_buffering=false);
        /* [SYNOPSIS]
             Use this function to create an instance of the internal
             processing machinery, for decompressing data associated with
             the supplied open tile interface.  Until you call this
             function (or copy another object which has been created), the
             `exists' function will return false.
             [//]
             Note carefully that the output components which will be
             decompressed are affected by any previous calls to
             `kdu_codestream::apply_input_restrictions'.  You may use either
             of the two forms of that function to modify the set of output
             components which appear to be present.  If called with an
             `access_mode' argument of `KDU_WANT_CODESTREAM_COMPONENTS', the
             present object will present codestream image components as though
             they were the final output image components.  If, however,
             `kdu_codestream::apply_input_restrictions' was called with a
             component `access_mode' argument of `KDU_WANT_OUTPUT_COMPONENTS',
             or if it has never been called, the present object will present
             output components in their fullest form, after processing by any
             required inverse multi-component decomposition, if necessary.  In
             either case, the set of components which is presented is
             identical to that which appears via the various `kdu_codestream'
             interface functions, such as `kdu_codestream::get_num_components',
             `kdu_codestream::get_bit_depth', and so forth, in each case
             with the optional `want_output_comps' argument set to true.
             [//]
             It is also worth noting that the behaviour of this function is
             affected by calls to `kdu_tile::set_components_of_interest'.
             In particular, any of the apparent output components which have
             been identified as uninteresting, will not be generated by the
             multi-component transformation network -- they will, instead,
             appear to contain constant sample values.   Of course, you will
             probably not want to access these constant components, or
             else you would not have marked them as uninteresting; however,
             you can access them if you wish without incurring any processing
             overhead.
             [//]
             From Kakadu v5.1, this function takes optional `env' and
             `env_queue' arguments to support a variety of multi-threaded
             processing paradigms, to leverage the capabilities of
             multi-processor platforms.  To see how this works, consult the
             description of these arguments below.  Also, play close attention
             to the use of `env' arguments with the `get_line' and
             `destroy' functions.
           [RETURNS]
             Returns the number of bytes which have been allocated internally
             for the processing of multi-component transformations,
             spatial wavelet transforms and intermediate buffering between
             the wavelet and block coder engines.  Essentially, this
             includes all memory resources, except for those managed by the
             `kdu_codestream' machinery (for structural information and
             code-block bit-streams).  The latter information can be recovered
             by querying the `kdu_codestream::get_compressed_data_memory' and
             `kdu_codestream::get_compressed_state_memory' functions.
           [ARG: force_precise]
             If this is true, the internal machinery will be configured to
             work with 32-bit representations for all image component samples,
             all the way through to the production of spatial subbands
             for encoding.  Otherwise, the internal machinery will determine
             a suitable representation precision, making every attempt to
             use lower precision implementations, which are faster and consume
             less memory, where this does not unduly compromise quality.
             See also the definition of `want_fastest'.
           [ARG: want_fastest]
             This argument represents the opposite extreme in precision
             selection to that represented by `force_precise'.  The argument
             is ignored if `force_precise' is true.  If `want_fastest' is
             true and `force_precise' is false, the function selects the
             lowest internal precision possible, even if this does sacrifice
             image quality.  This also generally results in the fastest
             possible implementation.  The `want_fastest' option is generally
             appropriate when rendering to 8-bit display devices.  Essentially,
             this argument forces the selection of the lower precision
             16-bit internal representations, for all processing which
             involves irreversible transforms -- reversible transforms
             must currently be implemented using the required precision,
             or else numerical overflow might result.  This means, for
             example, that a reversibly compressed 16-bit/sample image
             must be decompressed using 32-bit precision integers, whereas
             a 16-bit/sample image which was irreversibly compressed can
             be decompressed with an accuracy commensurate with perhaps
             11 or 12 bit imagery.  This is generally fine for display or
             projection.
           [ARG: skip_ycc]
             Be sure to leave this argument false unless you are quite sure
             that you want to retrieve raw codestream components without
             inverting any Part-1 decorrelating transform (inverse RCT or
             inverse ICT) which might otherwise be involved.  For this to
             make sense, you should be sure that the
             `kdu_codestream::apply_input_restrictions' function has been
             used to set the component access mode to
             `KDU_WANT_CODESTREAM_COMPONENTS' rather than
             `KDU_WANT_OUTPUT_COMPONENTS'.
           [ARG: processing_stripe_height]
             This argument must be strictly positive.  It identifies the
             number of row buffers maintained internally for each codestream
             component.  Codestream component lines are pulled directly off
             the relevant stripe buffer until it is empty.  At that point,
             the block decoding and spatial DWT synthesis engines are used to
             generate all stripe rows consecutively.  Larger
             stripe buffers have the potential to reduce context switching
             overhead when working with a large number of components or
             a large number of tiles.  On the other hand, the price you
             pay for this is increased memory consumption.  For
             single-threaded processing, a stripe height of 1 is usually
             fine (although perhaps not quite optimal).  For multi-threaded
             processing with the `double_buffering' option set to true,
             however, it is advisable to use a larger stripe height
             (e.g., 8 or even 16) since the thread context switching
             overhead is considerably higher than that of individual
             function calls within a single thread.
           [ARG: env]
             Supply a non-NULL argument here if you want to enable
             multi-threaded processing.  After creating a cooperating
             thread group by following the procedure outlined in the
             description of `kdu_thread_env', any one of the threads may
             be used to construct this processing engine.  Separate
             processing queues will automatically be created for each
             image component.
             [//]
             If `double_buffering' is set to true, these queues will also
             be used to schedule the spatial wavelet transform operations
             associated with each image component as jobs to be processed
             asynchronously by different threads.  Regardless of whether
             `double_buffering' is true or false, within each tile-component,
             separate queues are created to allow simultaneous processing of
             code-blocks from different subbands.
             [//]
             By and large, you can use this object in exactly the same way
             when `env' is non-NULL as you would with a NULL `env' argument.
             That is, the use of multi-threaded processing can be largely
             transparent.  However, you must remember the following two
             points:
             [>>] You must supply a non-NULL `env' argument to the
                  `get_line' function -- it need not refer to the same
                  thread as the one used to create the object here, though.
             [>>] Whereas single-threaded processing commences only with the
                  first call to `pull_line', multi-threaded processing
                  may have already commenced before this function returns.
                  That is, other threads may be working in the background
                  to decode code-blocks, perform DWT synthesis and so forth,
                  and this may start happening even before the present
                  function returns.  Of course, this is exactly what you
                  want, to fully exploit the availability of multiple
                  processing resources.
           [ARG: env_queue]
             This argument is ignored unless `env' is non-NULL.  When `env'
             is non-NULL, all job queues which are created inside this object
             are added as sub-queues of `env_queue' (or as top-level queues
             if `env_queue' is NULL).  If you do supply a non-NULL `env_queue',
             it is best to use a separate `env_queue' (obtained using
             `kdu_thread_entity::add_queue') for each tile processing engine
             which you want to keep active concurrently.  That way, you can
             individually wait upon completion of the activities within the
             engine, using `kdu_thread_entity::synchronize'; also, with a
             separate high-level queue for each tile engine, you can terminate
             all of the queues created for that engine, without affecting any
             others, by passing the `env_queue' argument supplied here into
             `kdu_thread_entity::terminate'.
             [//]
             Another advantage of supplying a separate `env_queue' for each
             tile processing engine is that it can improve the efficiency
             of Kakadu's thread scheduling algorithm.  This algorithm tries to
             keep each thread working in the same queue as it was previously
             working, so long as there are jobs.  Failing this, it tries to
             assign the thread to a queue with the closest possible common
             ancestor to the one it is currently working in.  Thus, by
             supplying a unique `env_queue' when calling this constructor,
             you can be sure that threads which are processing this tile
             will continue to do so until there is no more work to be done --
             this reduces the likelihood of cache misses, since all of the
             working memory associated with a tile is allocated in a single
             contiguous block, managed by the `allocator' object.
           [ARG: double_buffering]
             This argument is ignored unless `env' is non-NULL.  It allows
             you to determine whether the spatial DWT operations associated
             with each codestream image component should be
             executed as asynchronous jobs (via `kdu_thread_entity::add_jobs')
             or synchronously when new imagery is requested via the
             `get_line' function.
             [//]
             In the latter case (i.e., `double_buffering'=false), the thread
             which calls `get_line' performs all multi-component and
             spatial transform processing, leaving only the CPU-intensive block
             decoding operations for other threads.  This may leave these
             extra threads occasionally idle, particularly in systems which
             have a large number of physical processors.  Nevertheless,
             it is the simplest way to make good use of parallel processing.
             [//]
             If you set `double_buffering' to true, two sets of stripe buffers
             will be maintained for each codestream image component, each
             having `processing_stripe_height' rows.  New lines can be pulled
             from one stripe buffer while the other is being processed
             asynchronously by other threads.  This approach has the
             maximum potential to exploit the availability of parallel
             processing resources.  However, you must be careful to select
             a good value for `processing_stripe_height', since scheduling
             jobs onto threads incurs a non-negligible overhead.  This is
             particularly important if you have very narrow tiles.  One
             rough guide is to arrange for H*W to be around 2048 samples
             or more, where W is the tile width and H is the value of
             `processing_stripe_height'.  Thus, for small tiles of, say,
             128x128, you might select a stripe height of 16 rows.  In
             any case, with `double_buffering' set to true, it is a good
             idea (and not particularly costly in terms of memory, relative
             to other parts of the system) to set the stripe height to at
             least 8 rows.
             [//]
             At the end of the day, to get the most out of a multi-processor
             platform, you should be prepared to play around with the value
             of `processing_stripe_height' and compare the performance with
             that obtained when `double_buffering' is false.
        */
      void destroy(kdu_thread_env *env=NULL)
        {
          if (state != NULL)
            { state->terminate_queues(env); delete state; }
          state = NULL;
        }
        /* [SYNOPSIS]
             Use this function to destroy the internal processing machine
             created using `create'.  The function may be invoked on any
             copy of the original object whose `create' function was called,
             so be careful.
           [ARG: env]
             If this argument is non-NULL, you are inviting the current
             function to execute `kdu_thread_entity::terminate' on the
             `env_queue' which was originally supplied to `create', with
             a `leave_root' argument of true.  This causes all sub-queues
             of the `env_queue' supplied to `create' to be destroyed, after
             first waiting for all their jobs to complete.  This can be
             a very convenient way of ensuring proper synchronization and
             cleaning up all resources created by the `create' function.
             However, you need to be aware of the following caveats:
             [>>] If you have already used `kdu_thread_entity::terminate' to
                  destroy these same queues, or even the containing
                  `env_queue' which was passed to `create', you must supply
                  a NULL argument for `env' here, to avoid attempts to wait
                  on a deleted queue.
             [>>] If the `env_queue' supplied to `create' has other sub-queues,
                  in addition to those created by the `create' function,
                  passing a non-NULL `env' argument into the present function
                  will synchronize and delete those sub-queues as well -- of
                  course, this might be dangerous.  In particular, if the
                  `env_queue' argument supplied to `create' was NULL and you
                  supply a non-NULL `env' argument here, all queues in the
                  entire multi-threaded processing group will be synchronized
                  and deleted inside this function.  Of course, this might
                  well be exactly what you want.
        */
      kdu_coords get_size(int comp_idx) { return state->get_size(comp_idx); }
        /* [SYNOPSIS]
             This is a convenience function to return the size of the image
             component identified by `comp_idx', as seen within the present
             tile.  The same information may be obtained by invoking
             `kdu_codestream::get_tile_dims' with its `want_output_comps'
             argument set to true.
        */
      kdu_line_buf *get_line(int comp_idx, kdu_thread_env *env=NULL)
        { return state->get_line(comp_idx,env); }
        /* [SYNOPSIS]
             Use this function to get the next line of image data from
             the indicated component.  The function will return NULL if
             you have already reached the end of the tile, or if the next
             component cannot be decompressed without first retrieving
             new lines from one or more other components.  This latter
             condition may arise if the components are coupled through a
             multi-component transform, in which case the components must
             be accessed in an interleaved fashion -- otherwise, the object
             would need to allocate additional internal buffering resources.
             If there is a component that you are not interested in, you
             should declare that using either
             `kdu_codestream::apply_input_restrictions' and/or
             `kdu_tile::set_components_of_interest', before creating
             the present object.
             [//]
             Note that all lines returned by this function have a signed
             representation, regardless of whether or not
             `kdu_codestream::get_signed' reports that the components are
             signed.  In most cases, this minimizes the number of memory
             accesses which are required, deferring any required offsets
             until rendering (or saving to a file).
           [RETURNS]
             Non-NULL if a new decompressed line is available for the
             indicated component.  Each call to this function which returns
             a non-NULL pointer causes an internal line counter to be
             incremented for the component in question.
           [ARG: comp_idx]
             Index of the component for which a new line is being requested.
             This index musts lie in the range 0 to Co-1, where Co is
             the value returned by `kdu_codestream::get_num_components',
             with its `want_output_comps' argument set to true.  The number
             of these components may be affected by calls to
             `kdu_codestream::apply_input_restrictions' -- such calls must
             have been made prior to the point at which this object's
             `create' function was called.
           [ARG: env]
             Must be non-NULL if and only if a non-NULL `env' argument was
             passed into `create'.  Any non-NULL `env' argument must identify
             the calling thread, which need not necessarily be the one used
             to create the object in the first place.
        */
      bool is_line_precise(int comp_idx)
        { return state->is_line_precise(comp_idx); }
        /* [SYNOPSIS]
             Returns true if the indicated line has been assigned a
             precise (32-bit) representation by the `create' function.
             Otherwise, calls to `get_line' will return lines which
             have a 16-bit representation.  This function is provided as
             a courtesy so that applications which need to allocate
             auxiliary lines with compatible precisions will be able to
             do so.
        */
      bool is_line_absolute(int comp_idx)
        { return state->is_line_absolute(comp_idx); }
        /* [SYNOPSIS]
             Returns true if the indicated line has been assigned a
             reversible (i.e., absolute integer) representation by the
             `create' function.  Otherwise, calls to `exchange_line' will
             return lines whose `kdu_line_buf::is_absolute' function
             returns false.  This function is provided as a courtesy, so
             that applications can know ahead of time what the type of the
             data associated with a line will be.  In the presence of
             multi-component transforms, this can be non-trivial to figure
             out based solely on the output component index.
        */
    private: // Data
      kd_multi_synthesis_base *state;
  };

/*****************************************************************************/
/*                    Base Casting Assignment Operators                      */
/*****************************************************************************/

inline kdu_push_ifc &kdu_push_ifc::operator=(kdu_analysis rhs)
  { state = rhs.state; return *this; }
inline kdu_push_ifc &kdu_push_ifc::operator=(kdu_encoder rhs)
  { state = rhs.state; return *this; }

inline kdu_pull_ifc &kdu_pull_ifc::operator=(kdu_synthesis rhs)
  { state = rhs.state; return *this; }
inline kdu_pull_ifc &kdu_pull_ifc::operator=(kdu_decoder rhs)
  { state = rhs.state; return *this; }


/* ========================================================================= */
/*                     External Function Declarations                        */
/* ========================================================================= */

extern KDU_EXPORT void
  kdu_convert_rgb_to_ycc(kdu_line_buf &c1, kdu_line_buf &c2, kdu_line_buf &c3);
  /* [SYNOPSIS]
       The line buffers must be compatible with respect to dimensions and data
       type.  The forward ICT (RGB to YCbCr transform) is performed if the data
       is normalized (i.e. `kdu_line_buf::is_absolute' returns false).
       Otherwise, the RCT is performed.
  */
extern KDU_EXPORT void
  kdu_convert_ycc_to_rgb(kdu_line_buf &c1, kdu_line_buf &c2, kdu_line_buf &c3,
                         int width=-1);
  /* [SYNOPSIS]
       Inverts the effects of the forward transform performed by
       `kdu_convert_rgb_to_ycc'.  If `width' is negative, the number of
       samples in each line is determined from the line buffers themselves.
       Otherwise, only the first `width' samples in each line are actually
       processed.
  */

#endif // KDU_SAMPLE_PROCESSING
