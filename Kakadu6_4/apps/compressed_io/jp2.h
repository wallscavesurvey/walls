/*****************************************************************************/
// File: jp2.h [scope = APPS/COMPRESSED-IO]
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
/*****************************************************************************
Description:
   Defines classes which together embody the capabilities expected of
conformant readers and writers of the JP2 file format.  The `jp2_source'
and `jp2_target' classes define objects which derive from the abstract
compressed I/O base classes `kdu_compressed_source' and
`kdu_compressed_target'. These may be used to construct `kdu_codestream'
objects for generating or decompressing the embedded JPEG2000 code-stream box.
Methods are also provided to describe, interpret and process the
code-stream image samples in a conformant manner.
   The definitions provided here are also used to build services for the
JPX and MJ2 file formats.  Specific extensions for these file formats are
defined in "jpx.h" and "mj2.h".
******************************************************************************/

#ifndef JP2_H
#define JP2_H
#include <assert.h>
#include "kdu_elementary.h"
#include "kdu_compressed.h"
#include "kdu_params.h"
#include "kdu_sample_processing.h"
#include "kdu_cache.h"

// Classes defined here
class jp2_family_src;
class jp2_family_tgt;
class jp2_locator;
class jp2_input_box;
class jp2_output_box;

class jp2_dimensions;
class jp2_palette;
class jp2_channels;
class jp2_colour;
class jp2_colour_converter;
class jp2_resolution;

class jp2_source;
class jp2_target;
class jp2_data_references;

// Classes defined elsewhere
class j2_dimensions;
class j2_palette;
class j2_channels;
class j2_colour;
class j2_colour_converter;
class j2_resolution;
class jp2_header;
class j2_data_references;
struct jx_metanode;


/* ========================================================================= */
/*                            Inline Functions                               */
/* ========================================================================= */

/*****************************************************************************/
/*                             jp2_4cc_to_int                                */
/*****************************************************************************/

inline kdu_uint32
  jp2_4cc_to_int(const char *string)
{
  assert((string[0] != '\0') && (string[1] != '\0') &&
         (string[2] != '\0') && (string[3] != '\0') && (string[4] == '\0'));
  kdu_uint32 result = ((kdu_byte) string[0]);
  result = (result<<8) + ((kdu_byte) string[1]);
  result = (result<<8) + ((kdu_byte) string[2]);
  result = (result<<8) + ((kdu_byte) string[3]);
  return result;
}


/* ========================================================================= */
/*                              Box Type 4CC's                               */
/* ========================================================================= */

// These will occupy space in any source file which includes this header.

// The following boxes are fully defined by the JP2 file format
#define jp2_signature_4cc             ((kdu_uint32) 0x6a502020)
#define jp2_file_type_4cc             ((kdu_uint32) 0x66747970)
#define jp2_header_4cc                ((kdu_uint32) 0x6a703268)
#define jp2_image_header_4cc          ((kdu_uint32) 0x69686472)
#define jp2_bits_per_component_4cc    ((kdu_uint32) 0x62706363)
#define jp2_colour_4cc                ((kdu_uint32) 0x636f6c72)
#define jp2_palette_4cc               ((kdu_uint32) 0x70636c72)
#define jp2_component_mapping_4cc     ((kdu_uint32) 0x636d6170)
#define jp2_channel_definition_4cc    ((kdu_uint32) 0x63646566)
#define jp2_resolution_4cc            ((kdu_uint32) 0x72657320)
#define jp2_capture_resolution_4cc    ((kdu_uint32) 0x72657363)
#define jp2_display_resolution_4cc    ((kdu_uint32) 0x72657364)
#define jp2_codestream_4cc            ((kdu_uint32) 0x6a703263)

// The following boxes play critical roles in JPX
#define jp2_dtbl_4cc                  ((kdu_uint32) 0x6474626c)
#define jp2_data_entry_url_4cc        ((kdu_uint32) 0x75726c20)
#define jp2_fragment_table_4cc        ((kdu_uint32) 0x6674626c)
#define jp2_fragment_list_4cc         ((kdu_uint32) 0x666c7374)
#define jp2_cross_reference_4cc       ((kdu_uint32) 0x63726566)
#define jp2_reader_requirements_4cc   ((kdu_uint32) 0x72726571)
#define jp2_codestream_header_4cc     ((kdu_uint32) 0x6a706368)
#define jp2_desired_reproductions_4cc ((kdu_uint32) 0x64726570)
#define jp2_compositing_layer_hdr_4cc ((kdu_uint32) 0x6a706c68)
#define jp2_registration_4cc          ((kdu_uint32) 0x63726567)
#define jp2_opacity_4cc               ((kdu_uint32) 0x6f706374)
#define jp2_colour_group_4cc          ((kdu_uint32) 0x63677270)
#define jp2_composition_4cc           ((kdu_uint32) 0x636f6d70)
#define jp2_comp_options_4cc          ((kdu_uint32) 0x636f7074)
#define jp2_comp_instruction_set_4cc  ((kdu_uint32) 0x696e7374)

// The following boxes are used to describe non-essential metadata
#define jp2_iprights_4cc              ((kdu_uint32) 0x6a703269)
#define jp2_uuid_4cc                  ((kdu_uint32) 0x75756964)
#define jp2_uuid_info_4cc             ((kdu_uint32) 0x75696e66)
#define jp2_label_4cc                 ((kdu_uint32) 0x6c626c20)
#define jp2_xml_4cc                   ((kdu_uint32) 0x786d6c20)
#define jp2_number_list_4cc           ((kdu_uint32) 0x6e6c7374)
#define jp2_roi_description_4cc       ((kdu_uint32) 0x726f6964)
#define jp2_association_4cc           ((kdu_uint32) 0x61736f63)

// The following additional boxes are used by Motion JPEG2000
#define mj2_movie_4cc                ((kdu_uint32) 0x6d6f6f76)
#define mj2_movie_header_4cc         ((kdu_uint32) 0x6d766864)
#define mj2_track_4cc                ((kdu_uint32) 0x7472616b)
#define mj2_track_header_4cc         ((kdu_uint32) 0x746b6864)
#define mj2_media_4cc                ((kdu_uint32) 0x6d646961)
#define mj2_media_header_4cc         ((kdu_uint32) 0x6d646864)
#define mj2_media_header_typo_4cc    ((kdu_uint32) 0x6d686472)
#define mj2_media_handler_4cc        ((kdu_uint32) 0x68646c72)
#define mj2_media_information_4cc    ((kdu_uint32) 0x6d696e66)
#define mj2_video_media_header_4cc   ((kdu_uint32) 0x766d6864)
#define mj2_video_handler_4cc        ((kdu_uint32) 0x76696465)
#define mj2_data_information_4cc     ((kdu_uint32) 0x64696e66)
#define mj2_data_reference_4cc       ((kdu_uint32) 0x64726566)
#define mj2_url_4cc                  ((kdu_uint32) 0x75726c20)
#define mj2_sample_table_4cc         ((kdu_uint32) 0x7374626c)
#define mj2_sample_description_4cc   ((kdu_uint32) 0x73747364)
#define mj2_visual_sample_entry_4cc  ((kdu_uint32) 0x6d6a7032)
#define mj2_field_coding_4cc         ((kdu_uint32) 0x6669656c)

#define mj2_sample_size_4cc          ((kdu_uint32) 0x7374737a)
#define mj2_sample_to_chunk_4cc      ((kdu_uint32) 0x73747363)
#define mj2_chunk_offset_4cc         ((kdu_uint32) 0x7374636f)
#define mj2_chunk_offset64_4cc       ((kdu_uint32) 0x636f3634)
#define mj2_time_to_sample_4cc       ((kdu_uint32) 0x73747473)

// Opaque boxes
#define jp2_mdat_4cc                  ((kdu_uint32) 0x6d646174)
#define jp2_free_4cc                  ((kdu_uint32) 0x66726565)
#define mj2_skip_4cc                  ((kdu_uint32) 0x736b6970)

// The following boxes come from JPIP
#define jp2_placeholder_4cc           ((kdu_uint32) 0x70686c64)


/* ========================================================================= */
/*                            External Functions                             */
/* ========================================================================= */

extern KDU_AUX_EXPORT bool
  jp2_is_superbox(kdu_uint32 box_type);
  /* [SYNOPSIS]
       Returns true if `box_type' is the type of a known super-box, defined
       by any of the JP2-family file formats.
  */

/* ========================================================================= */
/*                                 Classes                                   */
/* ========================================================================= */

/*****************************************************************************/
/*                             jp2_family_src                                */
/*****************************************************************************/

class jp2_family_src {
  /* [BIND: reference]
     [SYNOPSIS]
       This object manages interaction with any data source conforming to
       the JP2 family file format.  To interact with such a source, you first
       call one of the overloaded `jp2_family_src::open' functions, depending
       on where the data is coming from (file, other compressed source, or
       dynamic cache).  You then pass a pointer to the `jp2_family_src' object
       into `jp2_input_box' or any of the higher level objects designed
       to manage JP2-family sources, including `jp2_source' and `mj2_source'.
       [//]
       Importantly, you can have multiple portals to boxes in a single
       `jp2_family_src' object.  Thus, you can pass the `jp2_family_src' object
       into `jp2_source::open' to gain access to JP2 header information and
       the first contiguous code-stream box, but you can concurrently open
       boxes within the source directly by passing the same object into
       another `jp2_input_box' object.  This allows you to walk through
       boxes in the source, recursing into sub-boxes as desired.  In this way,
       you can open additional code-streams (if any), examine IPR, UUID, or
       XML boxes, etc.
       [//]
       Note that you should not directly assign a `jp2_family_src' object to
       another, since it is not an interface.
  */
  public: // Member functions
    jp2_family_src()
      { fp_name=NULL; fp=NULL; indirect=NULL; cache=NULL; last_bin_class=-1;
        last_read_pos=last_bin_id=-1; last_bin_codestream=-1;
        last_id = 0; seekable = false;
      }
      /* [SYNOPSIS]
           You must use one of the `open' functions to create a legitimate
           source for use with `jp2_input_box::open' or any of the other
           higher level interfaces to JP2 family sources.
      */
    virtual ~jp2_family_src() { close(); }
      /* [SYNOPSIS]
           Calls `close', so to ensure exception-safe processing, it is best
           to make sure that you construct this object before constructing
           any object which uses it, and to destroy them in reverse order.
      */
    jp2_family_src &operator=(jp2_family_src &rhs)
      { assert(0); return *this; }
      /* [SYNOPSIS]
           This assignment operator serves to prevent actual copying of one
           `jp2_family_src' object to another.  In debug mode, it raises an
           assertion.
      */
    bool exists() { return ((fp!=NULL) || (indirect!=NULL) || (cache!=NULL)); }
      /* [SYNOPSIS]
           Returns true if the object has been opened and has not yet been
           closed.
      */
    bool operator!() { return !exists(); }
      /* [SYNOPSIS]
           Opposite of `exists'.
      */
    KDU_AUX_EXPORT virtual void open(const char *fname, bool allow_seeks=true);
      /* [SYNOPSIS]
           Open a JP2-family file, having the indicated name.  The function
           does not actually read anything from the file, and no assumption
           is made concerning the file structure other than the fact that
           it should consist of a sequence of boxes, each of which may have
           sub-boxes, where the box structure conforms to that used by the
           JP2 family file formats such as JP2, JPX, JPM and MJ2.
           [//]
           Generates an error through `kdu_error' if the file cannot be opened.
         [ARG: fname]
           Relative path name of file to be opened.
         [ARG: allow_seeks]
           If false, seeking within the source will not be permitted.
           Disabling seeking means that any `jp2_input_box' objects which
           are opened from the present object will not list
           `KDU_SOURCE_CAP_SEEKABLE' amongst their capabilities.  This
           has no effect unless embedded code-stream(s) contains TLM
           and/or PLT marker segments, in which case the ability to seek
           within the file can save a lot of memory when working
           with large images, but this may come at the expense of some loss
           in speed if we know ahead of time that we want to decompress
           the entire image(s).  Perhaps more significantly, if the source
           is not seekable, you will only be able to open boxes one at a time,
           and then only in strict linear order.
      */
    KDU_AUX_EXPORT virtual void open(kdu_compressed_source *indirect);
      /* [SYNOPSIS]
           Same as the first `open' function, but derives its input from the
           supplied `indirect' object's `kdu_compressed_source::read' function.
           The `indirect' object must support the `KDU_SOURCE_CAP_SEQUENTIAL'
           capability.  It is seekable if and only if it also supports the
           `KDU_SOURCE_CAP_SEEKABLE' capability, as returned by
           `indirect->get_capabilities'.
      */
    KDU_AUX_EXPORT virtual void open(kdu_cache *cache);
      /* [SYNOPSIS]
           With this form of the overloaded `open' function, the JP2 source
           data is recovered from the supplied caching data source.  In this
           case, `jp2_input_box' objects opened directly or indirectly using
           the present object may have contents which grow dynamically as
           the cache contents expand.  Moreover, in this case the
           `jp2_input_box::get_capabilities' function associated with any
           open contiguous code-stream box will advertise the
           `KDU_SOURCE_CAP_CACHED' attribute, while all other box types will
           advertise `KDU_SOURCE_CAP_SEQUENTIAL' and `KDU_SOURCE_CAP_SEEKABLE'.
      */
    KDU_AUX_EXPORT virtual void close();
      /* [SYNOPSIS]
           Also called by the destructor.  Do not call this function until
           all objects using the source have themselves been closed.
      */
    bool uses_cache() { return (cache != NULL); }
      /* [SYNOPSIS]
           Returns true if the object was opened using a dynamic cache, i.e.
           an object derived from `kdu_cache'.
      */
    bool is_top_level_complete()
      { bool is_complete;
        if (cache == NULL)
          return true;
        cache->get_databin_length(KDU_META_DATABIN,0,0,&is_complete);
        return is_complete;
      }
      /* [SYNOPSIS]
           Returns true if the object was opened using a dynamic cache, whose
           metadata-bin 0 is now complete, or if it was not opened using a
           dynamic cache.
      */
    int get_id() { return last_id; }
      /* [SYNOPSIS]
           This function can be useful in allowing other JP2-family
           objects to determine whether or not the `jp2_family_src' object
           has been closed and re-opened.  Each time `open' is called, the
           ID value returned by this function is incremented.
      */
    const char *get_filename() { return fp_name; }
      /* [SYNOPSIS]
           Returns NULL unless the object has been opened from a named file,
           via the first form of the `open' function.  In the latter case,
           the returned string represents a copy of the file name passed
           into that function.
      */  
    virtual void acquire_lock() { return; }
      /* [SYNOPSIS]
           Override this function to lock a mutex resource if your application
           requires access to the `jp2_family_src' from multiple threads.  For
           example, you may wish to open multiple boxes simultaneously from
           different threads.
      */
    virtual void release_lock() { return; }
      /* [SYNOPSIS]
           Override this function to release the mutex resource locked by
           `acquire_lock'.
      */
  private: // Data
    friend class jp2_input_box;
    friend class jpx_input_box;
    char *fp_name; // Name of file associated with `fp', else NULL
    FILE *fp; // For file-based sources
    kdu_compressed_source *indirect; // When using another Kakadu source object
    kdu_cache *cache; // When using a cached data-source

    kdu_long last_read_pos;
    kdu_long last_bin_id; // Only for `kdu_cache' sources; -ve if not active
    kdu_long last_bin_codestream; // Only `kdu_cache' sources; -ve if not used
    kdu_int32 last_bin_class; // Only for `kdu_cache' sources

    bool seekable; // True if the source is seekable, or cached
    int last_id;
  };

/*****************************************************************************/
/*                        jp2_threadsafe_family_src                          */
/*****************************************************************************/

class jp2_threadsafe_family_src : public jp2_family_src {
  /* [BIND: reference]
     [SYNOPSIS]
       Derived version of `jp2_family_src' which provides a meaningful
       implementation of `jp2_family_src::acquire_lock' and
       `jp2_family_src::release_lock' so that all access to the underlying
       data source from `jp2_input_box' objects is thread safe.
  */
  public: // Member functions
    jp2_threadsafe_family_src() { mutex.create(); }
    ~jp2_threadsafe_family_src() { mutex.destroy(); }
    virtual void acquire_lock() { mutex.lock(); }
      /* [SYNOPSIS]
           Takes an exclusive lock.
      */
    virtual void release_lock() { mutex.unlock(); }
      /* [SYNOPSIS]
           Releases the lock taken by `acquire_lock'.
      */
  private: // Data
    kdu_mutex mutex;
  };

/*****************************************************************************/
/*                               jp2_locator                                 */
/*****************************************************************************/

class jp2_locator {
  /* [BIND: copy]
     [SYNOPSIS]
       This object is returned by `jp2_input_box::get_locator' and by
       `jp2_source::open' as a locator which can be used to mark and
       subsequently open any JP2 box (or sub-box) offered by a `jp2_source'
       object.  The locator is supplied to the `jp2_input_box::open' function.
       [//]
       The contents of the object might not have meaning to the application.
  */
  public: // Member functions
    jp2_locator() { set_file_pos(0); }
      /* [SYNOPSIS]
           Creates a null locator, which can be used to open the first
           JP2 box in any JP2-family source, using `jp2_input_box::open'.
           Immediately after construction, the `is_null' function returns
           true.  There are two ways to obtain another locator, as follows:
           [>>] Invoke `jp2_input_box::get_locator' on an existing open box.
           [>>] Use `set_file_pos' to set the locator to reference an absolute
                location in an original file (works even if the file is being
                served up incrementally in an unpredictable order).
      */
    bool is_null()
      { return ((bin_id<0)?(file_pos==0):((bin_id==0)&&(bin_pos==0))); }
      /* [SYNOPSIS]
           Returns true if the locator points to the start of the file.  This
           condition holds when the object is first constructed.
      */
    bool operator!() { return is_null(); }
      /* [SYNOPSIS]
           Same as `is_null'.
      */
    kdu_long get_file_pos() { return file_pos; }
      /* [SYNOPSIS]
           Returns the absolute location of the box to which this locator
           refers, within the original file to which it belongs.  If the
           file is being delivered by a remote server to a dynamically
           evolving cache, accessed via a `kdu_cache' object, a linear
           file structure may not exist locally.  Nevertheless, even
           in this case, the available data is generally referenced to an
           original file so that absolute locations can usually be
           established.  The only exception to this is where a JP2 box
           has been synthesized by the server (stream equivalent) or is
           being served as an incremental code-stream (one whose open
           `jp2_input_box' can be passed to `kdu_codestream::create').  In
           these circumstances, the box does not have a file location and
           this function will return -1.
           [//]
           The file position corresponds to the first byte of the relevant
           box header, not the box contents, which appear after its header.
      */
    void set_file_pos(kdu_long pos)
      { file_pos=pos; bin_id=-1; bin_pos=0;  }
      /* [SYNOPSIS]
           You may use this function to configure the locator to point to
           a particular location within the original file.  This location
           must correspond to the first byte of some box header.  You may
           then open the box by invoking `jp2_input_box::open', using the
           locator.  In some cases, the data source is not a file, but
           a dynamic cache (if `kd_data_src' was opened using a `kdu_cache'
           object).  In this case, the `jp2_input_box::open' call will
           attempt to resolve the data-bin associated with the indicated
           file position, augmenting the information in the present object
           to reflect the data-bin identifier and the location of the box
           within its data-bin.  If insufficient information is currently
           available in the cache to perform such resolution, the attempt
           to open the JP2 box may fail, but may later succeed once the
           cache contents have been augmented.
      */
    kdu_long get_databin_id() { return bin_id; }
      /* [SYNOPSIS]
           Returns the identity of the cache databin in which this locator is
           found, or else -1 if the location does not belong to a dynamic
           cache.
      */
    kdu_long get_databin_pos() {return bin_pos; }
      /* [SYNOPSIS]
           Returns the location associated with this locator, relative to
           the start of the databin returned by `get_databin_id'.  If there
           is no databin, the value returned by this function is meaningless
           but should generally be 0.
      */
    bool operator==(jp2_locator &rhs)
      {
        if (bin_id < 0)
          return (file_pos == rhs.file_pos);
        else
          return ((bin_id == rhs.bin_id) && (bin_pos == rhs.bin_pos));
      }
  private: // Data
    friend class j2_source;
    friend class jp2_input_box;
    friend struct jx_metanode;

    kdu_long file_pos; // -1 if the box has no place in the original file
    kdu_long bin_id; // -1 if there is no data-bin involved
    kdu_long bin_pos;
  };
  /* Notes:
        The `file_pos' member identifies the location of the first byte of
     the box header within its file.  If the data source is a dynamic cache,
     this member refers to the location of the box within the original file
     (i.e., it has been dynamically created by the server), in which case
     this member will be equal to -1.
        The `bin_id' and `bin_pos' members are used only for cacheing data
     sources.  They indicate the meta data-bin to which the box header belongs
     and the location of the box header within that meta data-bin.  If the
     box header appears within a placeholder box, `bin_id' identifies the
     data-bin which contains the placeholder, and `bin_pos' identifies the
     location of the placeholder box header (the actual box header must be
     recovered from the placeholder's contents).
  */

/*****************************************************************************/
/*                               jp2_input_box                               */
/*****************************************************************************/

class jp2_input_box : public kdu_compressed_source {
  /* [BIND: reference]
     [SYNOPSIS]
       Provides full access to a single JP2 family box.  The object implements
       the interface members of its virtual base class `kdu_compressed_source',
       so that any open contiguous code-stream box may be passed directly
       to `kdu_codestream::create' to gain full access to its embedded
       code-stream.  For other box types, the `read' member may be used
       directly to read from the box.
       [//]
       It is not safe to assign one `jp2_input_box' object directly
       to another.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle functions
    KDU_AUX_EXPORT jp2_input_box();
      /* [SYNOPSIS]
           After construction, you must call `open' to associate the object
           with a particular `jp2_family_src' object, either directly (using
           the first form of the `open' function) or indirectly, as a sub-box
           of another box which is associated with the `jp2_family_src' object.
      */
    KDU_AUX_EXPORT virtual ~jp2_input_box();
      /* [SYNOPSIS]
           Closes the box if it is open.  It is safe to close boxes out
           of order, or even after their underlying `jp2_family_src'
           object has disappeared; however, sub-boxes should not be closed
           after the super-box used to open them has been deleted from memory.
      */
    jp2_input_box &operator=(jp2_input_box &rhs)
      { assert(0); return *this; }
      /* [SYNOPSIS]
           This assignment operator serves to prevent actual copying of one
           `jp2_input_box' object to another.  In debug mode, it raises an
           assertion.
      */
    bool exists() { return is_open; }
      /* [SYNOPSIS]
           Returns true if the box has been opened, but not yet closed.
      */
    bool operator!() { return !is_open; }
      /* [SYNOPSIS]
           Opposite of `exists'.
      */
    KDU_AUX_EXPORT virtual bool
      open(jp2_family_src *src, jp2_locator locator=jp2_locator());
      /* [SYNOPSIS]
           Opens the box to read from the `src' object.  If `src' was opened
           using a seekable file, seekable compressed data source, or a
           dynamic cache (`kdu_cache') object, multiple boxes may be
           simultaneously opened using the same `jp2_family_src' object.
           Moreover, they may be read in any order, or in interleaved fashion.
           The internal machinery maintains information about the current read
           state and manages all required context switching, but for
           multi-threaded applications you should consider supplying
           implementing the synchronization funcions,
           `jp2_family_src::acquire_lock' and `jp2_family_src::release_lock' in
           a class derived from `jp2_family_src'.
           [//]
           Every open box has a unique `locator' value.  If you have visited
           a box before (e.g., during an initial pass through the file), you
           may open the box again using the locator you found at that time
           by invoking `jp2_input_box::get_locator'.  Using a locator, you
           may open any box using this function, not just top level boxes.
           You may think of the `locator' as the address of the box within
           its file, even though the information source might be a
           `kdu_cache' object which has no linear file structure.  Even
           in this case, a mechanism is provided (behind the scenes) for
           maintaining the association between each JP2 box and a location
           in an original file.  These locations are managed by the
           `jp2_locator::get_file_pos' and `jp2_locator::set_file_pos'
           functions.
           [//]
           An error may be generated through `kdu_error' if a fundamental
           problem is detected.  If the `jp2_family_src' object is using a
           cache as its data source, however, it is possible that the cache
           may not yet have sufficient information to open the box.  In this
           case, the function returns false, leaving the box closed (`exists'
           remains false).  Any of the following conditions may cause this
           to happen:
           [>>] All or part of the box's header might not yet have been
                loaded into the cache.
           [>>] The cache might not contain sufficient information to
                determine the cache data-bin to which the box belongs.  This
                may happen if the supplied `locator' object identifies only 
                an original file location (set using
                `jp2_locator::set_file_pos'), in which case it may be
                necessary to walk through multiple data-bins to determine
                where the dynamic representation of that box will be; and
                some of the intermediate bins might not yet be available.
           [>>] The box might be represented by a placeholder box, whose
                contents are not yet fully available -- the placeholder
                box's contents must be read before the actual location of
                the box's contents can be determined.
           [//]
           If any of the above occurs, the function returns false with the box
           still closed.  Trying again later, however, once the cache contents
           have been augmented, may result in success (`exists' becomes true).
         [RETURNS]
           True if a new box was opened, else false.
         [ARG: src]
           Must point to an open `jp2_family_src' object, which must not be
           closed until after the present object is closed (or destroyed).
         [ARG: locator]
           To open the very first box in the JP2 source, you may supply the
           empty (or null) locator, which is created by the `jp2_locator'
           object's constructor.  Other values for this argument may be
           obtained by invoking `get_locator' on this or another
           `jp2_input_box' object, or by using `jp2_locator::set_file_pos'.
           The same box (or sub-box) may be opened as often as desired, by
           different `jp2_input_box' objects if you like, simply by passing
           the relevant locator to this function.
      */
    KDU_AUX_EXPORT virtual bool
      open(jp2_input_box *super_box);
      /* [SYNOPSIS]
           Opens the box as a sub-box of the indicated `super_box'.  You
           may think of the present box and the super-box as sharing a common
           "read pointer" so that once the present box is closed, reading
           may continue within the super-box from the point where we left off.
           The super-box is "locked" so long as any sub-box is open; an
           error will be generated if any attempt is made to read from a
           super-box which is locked in this way.  While the sub-box is being
           read, values returned by `super_box->get_remaining_bytes' or
           `super_box->get_pos' are unstable and cannot be trusted.
           [//]
           If this function fails to open a new box, the function returns
           false, leaving the box closed (the `exists' function will return
           false).  This may happen if one of the following conditions occur:
           [>>] The super-box is exhausted, meaning that it has no remaining
                content bytes at all; or
           [>>] The super-box contains insufficient contents for the moment
                to open a new box, but it may contain sufficient bytes in
                the future as more data becomes available.  This is only
                possible if the `jp2_family_src' object uses a cache for its
                information source, since the cache may be updated
                dynamically.  This case may be distinguished from the
                former case by checking the super-box's `get_remaining_bytes'
                member, which will be 0 if and only if the super-box has
                actually been exhausted.
         [RETURNS]
           True if a new box was opened, else false.
      */
    KDU_AUX_EXPORT virtual bool open_next();
      /* [SYNOPSIS]
           This function may be used to open the next box in sequence, after
           the one which was last closed.  If the box was last opened as a
           sub-box of another `jp2_input_box' object, the function attempts
           to open a new sibling sub-box, failing (leaving the box in the
           closed state and returning false) if the containing
           super-box is exhausted.
           [//]
           If the box was last opened directly, using a `locator' to navigate
           inside the `jp2_family_src' object, this function opens the next box
           in sequence, failing (leaving the box in the closed state and
           returning false) only if the source is exhausted.  Note
           that the behaviour could be misleading in this case, if the
           `locator' supplied to the first form of the `open' function did not
           correspond to a top level box, since we might then walk right over
           the containing super-box boundary without noticing. It is your
           responsibility to use this function in a meaningful way.
           [//]
           If the function fails to open a new box, the `exists' member will
           return false.  While this usually means that the JP2 source, or the
           containing super-box is exhausted, it could also occur if the
           unerlying `jp2_family_src' object uses a dynamic cache (`kdu_cache'
           or a suitably derived object).  In that case, the present function
           may be called repeatedly, with the possibility of succeeding at
           a later point when the cache contents have been augmented.
      */
    KDU_AUX_EXPORT virtual bool close();
      /* [SYNOPSIS]
           Implements `kdu_compressed_source::close'.
           It is safe to call this function at any time, even if the box
           is not currently open or if its super-box has somehow been
           closed or the underlying `jp2_family_src' object has ceased to
           exist (usually an error condition, but still safe).  However,
           a super-box's members may be accessed by this function, so you
           should make sure that you do not destroy the super-box used to
           open a sub-box, until after the sub-box has at least closed.
         [RETURNS]
           True if the contents of the box were completely consumed by calls
           to `read', or by opening and closing of sub-boxes.
      */
    KDU_AUX_EXPORT virtual void transplant(jp2_input_box &src);
      /* [SYNOPSIS]
           This function may be called only if the `src' object represents an
           open box, and the present object does not currently represent an
           open box.  It opens the present object so as to present the same
           state and read position as the `src' object.  The only difference
           between the present object's new state and the old `src' object's
           new state is that the present object will not retain any connection
           with a containing super-box.  The `src' object is then closed.
           [//]
           The function sees a lot of use in the implementation of the
           `jpx_source' object and its offspring.
      */
    bool has_caching_source() { return (src != NULL) && (src->cache != NULL); }
      /* [SYNOPSIS]
           Indicates whether or not the ultimate source of data is a
           `kdu_cache' object, which was used to open the `jp2_family_src'
           object passed into the present object's `open' function.  This
           information can be useful, since the behaviour of a number of
           other functions can differ if the ultimate source of data is a
           cache.  In particular, an incomplete cache may cause a function
           to fail (e.g. return false) even though it will later be able
           to succeed when the cache contents have been augmented.
      */
  // --------------------------------------------------------------------------
  public: // Identification functions
    kdu_uint32 get_box_type() { return box_type; }
      /* [SYNOPSIS]
           Returns the box type signature, expressed as a 32-bit integer,
           unless the box could not be opened, in which case 0 is returned.
           This is also the condition which causes `exists' to return false. */
    jp2_locator get_locator() { return locator; }
      /* [SYNOPSIS]
           Returns the unique locator of the currently open box, but note that
           the return value is invalid if the box is not currently open.
           [//]
           A valid locator may be passed to the first form of the `open'
           function to open another `jp2_input_box' object which references
           this same box, at any point in the future.
      */
    jp2_locator get_contents_locator(kdu_int32 *class_id=NULL)
      { jp2_locator result;
        result.file_pos = locator.file_pos+original_header_length;
        result.bin_id = bin_id; result.bin_pos = contents_start;
        if (class_id != NULL) *class_id = this->bin_class;
        return result; }
      /* [SYNOPSIS]
           Returns a locator for the first byte of this box's contents.
           The return value may be invalid if the box is not open.
           The file position (as returned by `jp2_locator::get_file_pos') of
           the returned locator should differ from that of the locator
           returned by `get_locator' by exactly the header length, as
           returned by `get_box_header_length'.  However, the databin-id
           of the returned locator (see `jp2_locator::get_databin_id') may
           be very different in the event that the ultimate source of data
           is a dynamic cache and the box is recovered from a placeholder,
           which identifies a different data-bin for the box contents.
         [ARG: class_id]
           If non-NULL, this is used to return the class of any data-bin
           associated with the returned locator.  The data-bin's id and class
           may be used together with the relevant location within the
           data-bin in a call to `kdu_cache::set_read_scope'.  The main
           application for this function, however, is in the construction
           of JPIP client metadata requests.
      */
    int get_box_header_length()
      { return (box_type==0)?0:((int) original_header_length); }
      /* [SYNOPSIS]
           Returns 0 if the box is not open.  Otherwise returns the number
           of bytes used to represent the box header.  This information
           may be used together with that returned by `get_locator' to
           recover the location of the start of the box's contents.
      */
    KDU_AUX_EXPORT virtual int get_capabilities() { return capabilities; }
      /* [SYNOPSIS]
           Returns the logical OR of one or more capability flags, whose
           values are identified by the macros, `KDU_SOURCE_CAP_SEQUENTIAL',
           `KDU_SOURCE_CAP_SEEKABLE', `KDU_SOURCE_CAP_IN_MEMORY' and
           `KDU_SOURCE_CAP_CACHED'.  These flags are defined in connection
           with the base class's virtual member function,
           `kdu_compressed_source::get_capabilities', which
           is implemented here.
           [//]
           Note that `KDU_SOURCE_CAP_CACHED' can only be offered by
           contiguous code-stream boxes, and only if the original
           `jp2_box_src' object from which this box was opened (directly or
           indirectly) was opened with a `kdu_cache' object as its
           information source.
           [//]
           Note also that `KDU_SOURCE_CAP_IN_MEMORY' can be offered only
           if the `load_in_memory' function has been called, or if the
           box is a sub-box of another box which already offers
           `KDU_SOURCE_CAP_IN_MEMORY'.
      */
  // --------------------------------------------------------------------------
  public: // Navigation functions
    kdu_long get_remaining_bytes()
      { return (rubber_length)?
               (-1):((contents_lim-pos)+partial_word_bytes); }
      /* [SYNOPSIS]
           Returns the number of bytes in the box's contents which lie beyond
           the current read pointer.  Adding this value to that returned
           by `get_pos' yields the box's total length.  However, if the
           box's length cannot be determined a priori, this function will
           return a negative value.
           [//]
           The behaviour of the function is undefined if the box is not open.
         [RETURNS]
           Returns -1 if the box has a rubber length, meaning that its length
           field indicated that the box extends to the end of the JP2 source.
           This return value will be converted to a non-negative value once
           the end of the box is actually encountered by attempting to read
           or seek past it.
      */
    KDU_AUX_EXPORT kdu_long get_box_bytes();
      /* [SYNOPSIS]
           Returns the total number of bytes in the currently open box,
           including the box header, not just its contents.  If the box
           has a rubber length, the function can only report the number
           of bytes which have actually been read from the box, starting
           from the first byte of the box header.  Returns 0 if the box
           is not open.
      */
    KDU_AUX_EXPORT bool is_complete();
      /* [SYNOPSIS]
           This function is provided to substantially simply the processing
           of boxes which are recovered from dynamic caches, i.e., where
           the underlying `jp2_family_src' object was opened using a
           `kdu_cache' object as its information source.  It indicates
           whether or not the cache contains sufficient contents to completely
           read this box.  If the underlying data source is a file, or a
           `kdu_compressed_source' object, rather than a cache, the function
           always returns true, meaning that it is safe to plunge immediately
           into reading the box's contents.
           [//]
           Complex cache structures may not allow easy determination of whether
           or not sub-boxes of the present box are also completely available
           in the cache.  For this reason, the function only tells you
           definitively about the completeness of a box which has no
           sub-boxes.  For boxes with super-boxes, completeness means at least
           that you can walk the sub-boxes, but you may generally need
           to test the completeness of each sub-box separately, and so-forth.
      */
    KDU_AUX_EXPORT virtual bool seek(kdu_long offset);
      /* [SNOPSIS]
           See the description of `kdu_compressed_source::seek', but note that
           the `seek' functionality implemented here deliberately prevents
           seeking beyond the bounds of the box.  The `offset' is expressed
           relative to the first byte in the contents of the box.
      */
    virtual kdu_long get_pos()
      { return (pos-contents_start)-partial_word_bytes; }
      /* [SYNOPSIS]
           See `kdu_compressed_source::get_pos' for an explanation.
           Note that the returned value identifies the location of the next
           byte to be read, relative to the start of the box contents.
      */
  // --------------------------------------------------------------------------
  public: // Read functions
    KDU_AUX_EXPORT virtual bool load_in_memory(int max_bytes);
      /* [SYNOPSIS]
           This function attempts to load the entire contents of the box
           into an internal dynamically allocated block of memory.  If
           successful, the `get_capabilities' function will report the
           following capability flags: `KDU_SOURCE_CAP_SEQUENTIAL',
           `KDU_SOURCE_CAP_SEEKABLE' and `KDU_SOURCE_CAP_IN_MEMORY'.
           Moreover, the `access_memory' function can then be used to
           directly access the memory block, as an alternative to using
           the `seek' and `read' functions.
           [//]
           The function may fail if, for example, the ultimate source of
           information is a `kdu_compressed_source' object which does not
           offer `KDU_SOURCE_CAP_SEQUENTIAL'.  While the conditions under
           which the source cannot be loaded directly into an internal block
           of memory may be rather technical, you need not worry about the
           details.  If the function succeeds, it will return true; otherwise
           it returns false and you must use access the box contents via
           the `read' function.
           [//]
           If the function succeeds, the internal memory block spans the
           extent of the box contents, but does not include its header.  It
           is deleted when the box is closed (or destroyed).
           [//]
           You may, if you like, call this function after the box has
           already been read, in whole or in part, so long as it has not
           yet been closed.  In this case, however, the function is not
           likely to return true unless the source is seekable.
         [RETURNS]
           True if and only if a memory block was allocated and loaded with
           the box contents (either by this function, or else by a previous
           call to this function in the present box or a super-box).
           Whether or not the function returns true, you can always seek
           within the box and read its contents using the `seek' and `read'
           functions.  However, only in the case that the function returns
           true can you directly access the loaded box contents via the
           `access_memory' function.
           [//]
           In the special case where the ultimate data source terminates
           prematurely, prior to the nominal end of the box, the present
           function will still allocate a memory block and return true
           (assuming that this is what it would have done if the data source
           did not terminate prematurely).  However, the internal length
           of the box will be adjusted to reflect the amount of data which is
           actually available.  If your application needs to know whether
           or not premature termination has occurred, you can call the
           `get_box_bytes' function before and after the present function,
           comparing the two results.
         [ARG: max_bytes]
           Specifies a limit on the size of the memory block that you are
           prepared to allocate in order for this function to succeed.  If
           the length of the box contents exceeds this value, the function
           returns false without allocating any memory.
      */
    KDU_AUX_EXPORT virtual kdu_byte *
      access_memory(kdu_long &pos, kdu_byte * &lim);
      /* [SYNOPSIS]
           See `kdu_compressed_source::access_memory' for an explanation.
      */
    KDU_AUX_EXPORT virtual int
      read(kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           See `kdu_compressed_source::read' for an explanation.
           Note that when multiple `jp2_input_box' objects are opened using
           the same `jp2_family_src' object, they may be read in any order, so
           long as the underlying data source supports
           `KDU_SOURCE_CAP_SEEKABLE' or `KDU_SOURCE_CAP_CACHED'.  However,
           arbitration between multiple boxes is not inherently thread safe,
           meaning that you should not access different boxes of a single
           `jp2_family_src' object from different threads, unless that
           object has been made thread safe by implementing its
           `jp2_family_src::acquire_lock' and `jp2_family_src::release_lock'
           functions.
         [RETURNS]
           May return a value smaller than `num_bytes' if the container
           (a super-box, or the underlying data source) is exhausted.  When
           this happens, `get_remaining_bytes' will return 0.
           [//]
           May also return a value smaller than `num_bytes' if the
           underlying `jp2_family_src' object was opened using
           a `kdu_cache' data source, in which case the relevant cache
           entries may not yet be complete.  In this case, subsequent
           calls to the function may be able to retrieve additional data
           once the cache has been augmented.  This condition may be
           distinguished from that of a truly exhausted box by the fact
           that `get_remaining_bytes' will either return -1 (rubber length
           box) or a positive value (total box length is known to be larger
           than the number of bytes currently available).
      */
    KDU_AUX_EXPORT bool read(kdu_uint32 &dword);
      /* [SYNOPSIS]
           Reads an unsigned big-endian 4 byte word from the box, returning
           false if the source is exhausted or insufficient data is currently
           available from a dynamic cache.
      */
    bool read(kdu_int32 &dword) { return read(*((kdu_uint32 *) &dword)); }
      /* [SYNOPSIS]
           Reads a signed big-endian 4 byte integer from the box, returning
           false if the source is exhausted or insufficient data is currently
           available from a dynamic cache.
      */
    KDU_AUX_EXPORT bool read(kdu_uint16 &word);
      /* [BIND: no-java]
         [SYNOPSIS]
           Reads an unsigned big-endian 2 byte word from the box, returning
           false if the source is exhausted or insufficient data is currently
           available from a dynamic cache.
      */
    bool read(kdu_int16 &word) { return read(*((kdu_uint16 *) &word)); }
      /* [SYNOPSIS]
           Reads a signed big-endian 2 byte integer from the box, returning
           false if the source is exhausted or insufficient data is currently
           available from a dynamic cache.
      */
    bool read(kdu_byte &byte)
      { return (read(&byte,1) == 1); }
      /* [SYNOPSIS]
           Reads a single byte from the box, returning false if the source
           is exhausted or insufficient data is currently available from a
           dynamic cache.
      */
  // --------------------------------------------------------------------------
  public: // Read scoping functions (for cacheing sources only)
    KDU_AUX_EXPORT virtual bool set_tileheader_scope(int tnum, int num_tiles);
      /* [SYNOPSIS]
           Implements `kdu_compressed_source::set_tileheader_scope'.
      */
    KDU_AUX_EXPORT virtual bool set_precinct_scope(kdu_long unique_id);
      /* [SYNOPSIS]
           Implements `kdu_compressed_source::set_precinct_scope'.
      */
    KDU_AUX_EXPORT virtual bool
      set_codestream_scope(kdu_long codestream_id, bool need_main_header=true);
      /* [SYNOPSIS]
           This function may be used only with caching data sources and then
           only if the box is currently open, having a box-type of
           `jp2_codestream_4cc' (i.e., a contiguous code-stream box).
           [//]
           If the object supports the `KDU_SOURCE_CAP_CACHED' capability and
           the supplied `codestream_id' lies in the range of code-stream
           identifiers offered by the placeholder box which was used to
           construct this interface, the function sets the current read scope
           to the main header of the indicated code-stream.
         [RETURNS]
           The function returns true only if the above conditions are satisfied
           and, either the whole of the main header is available in the cache,
           or the `need_main_header' argument is false (not the default).
      */
  // --------------------------------------------------------------------------
  private: // Helper functions
    void reset_header_reading_state()
      {
        box_type = 0; rubber_length = is_open = false;
        original_box_length=original_header_length=original_pos_offset = 0;
        next_box_offset = 0; codestream_min=codestream_lim=codestream_id = -1;
      }
      /* Called at the start of `read_box_header' or at any point in the
         midst of box header reading, at which the reading process fails.
         Resetting these variables ensures that a fresh attempt to read the
         header once more data is available from a dynamic cache will not
         be affected by a previously unsuccessful read attempt. */
    KDU_AUX_EXPORT bool read_box_header(bool prefer_originals=false);
      /* This function uses the `locator' member to locate the start of the
         box header.  It starts reading from that location, consuming either
         an original box header, or a placeholder box which references the
         box header.  If it is unable to finish reading either the box header
         or the placeholder box, the function returns false and leaves the
         object with 'is_open'=false.  Otherwise, the function returns true,
         leaving the object with `is_open'=true and filling in all member
         variables from `box_type' through to `rubber_length'.  On successful
         return, `pos' and `codestream_id' also reference valid locations for
         further box reading operations to proceed.
            If `prefer_originals' is true, wherever a placeholder box is
         involved, the original box will be opened, regardless of whether or
         not stream equivalents or incremental code-streams exist.  Otherwise,
         the stream equivalents are preferred.
            If the function successfully reads a placeholder, which offers
         no access to the original box header at all, the function returns
         true, but with `box_type' set to 0. */
  protected: // Location and reading context
    jp2_locator locator;
    jp2_input_box *super_box;
    jp2_family_src *src;
    kdu_byte *contents_block; // Non-NULL if box contents are in memory
    kdu_byte *contents_handle; // Memory block to be deleted on close, if any
  protected: // Details parsed from box header or placeholder
    kdu_uint32 box_type;
    kdu_long original_box_length;
    kdu_long original_header_length;
    kdu_long original_pos_offset; // See below
    kdu_long next_box_offset; // Either box length or placeholder length
    kdu_long contents_start; // First byte of contents relative to container
    kdu_long contents_lim; // Beyond last byte of contents.
    kdu_long bin_id; // For cacheing sources
    kdu_long codestream_min; // First incremental code-stream ID
    kdu_long codestream_lim; // Beyond last incremental code-stream ID
    kdu_int32 bin_class;
    bool can_dereference_contents; // If contents belong to orig box in file
    bool rubber_length; // If box length is unbounded
  protected: // Current state
    bool is_open;
    bool is_locked; // False if we have the read focus
    int capabilities; // Returned by `get_capabilities'.
    kdu_long pos; // Current location: `contents_start' <= pos < `contents_lim'
    kdu_long codestream_id; // Only for cache bins other than KDU_META_DATABIN
  protected: // Buffering resources
#   define J2_INPUT_MAX_BUFFER_BYTES 24
    kdu_byte buffer[J2_INPUT_MAX_BUFFER_BYTES];
    int partial_word_bytes; // If part way through reading a word.
  };
  /* Notes:
       The `original_pos_offset' member holds the amount which should be
     added to `pos'-`contents_start' to get the displacement of the `pos'
     location relative to the start of the box contents within the original
     file (i.e., if placeholders were not used).  This member is adjusted
     when a sub-box is closed.  If the sub-box involved placeholders, the
     difference between the sub-box's original length and its placeholder
     length is added to its containing super-box's `original_pos_offset'
     member. */

/*****************************************************************************/
/*                              jp2_family_tgt                               */
/*****************************************************************************/

class jp2_family_tgt {
  /* [BIND: reference]
     [SYNOPSIS]
       This object manages output of files or data streams conforming to the
       JP2 family file format.  To generate such a data stream, you first
       call one of the overloaded `jp2_family_tgt::open' functions, depending
       on where the data is to be sent (file, other compressed target).  You
       then pass a pointer to the `jp2_family_tgt' object into `jp2_output_box'
       or any of the higher level objects designed to manage JP2-family
       targets, including `jp2_target' and `mj2_target'.
       [//]
       While the usage for `jp2_family_tgt' reflects that of `jp2_family_src',
       you should be aware that only one box is allowed to be active at a
       time when generating a JP2 family target.  By contrast, `jp2_family_src'
       objects can manage multiple simultaneously open input boxes.
       [//]
       Note that you should not directly assign a `jp2_family_tgt' object to
       another, since it is not an interface.
  */
  public: // Member functions
    jp2_family_tgt()
      { fp=NULL; indirect=NULL; last_write_pos=0;
        opened_for_simulation = has_rubber_box=false; }
      /* [SYNOPSIS]
           You must use one of the `open' functions to create a legitimate
           target for use with `jp2_output_box::open' or any of the other
           higher level objects used to generate members of the JP2
           family of file formats.
      */
    virtual ~jp2_family_tgt() { close(); }
      /* [SYNOPSIS]
           Calls `close', so to ensure exception-safe processing, it is best
           to make sure that you construct this object before constructing
           any object which uses it, and to destroy them in reverse order.
      */
    jp2_family_tgt &operator=(jp2_family_tgt &rhs)
      { assert(0); return *this; }
      /* [SYNOPSIS]
           This assignment operator serves to prevent actual copying of one
           `jp2_family_tgt' object to another.  In debug mode, it raises an
           assertion.
      */
    bool exists() { return ((fp != NULL) || (indirect != NULL)); }
      /* [SYNOPSIS]
           Returns true if the object has been opened and has not yet been
           closed.
      */
    bool operator!() { return !exists(); }
      /* [SYNOPSIS]
           Opposite of `exists'.
      */
    KDU_AUX_EXPORT void open(const char *fname);
      /* [SYNOPSIS]
           Open a JP2-family file, having the indicated name.  The function
           does not actually write anything from the file.
           [//]
           Generates an error through `kdu_error' if the file cannot be opened.
         [ARG: fname]
           Relative path name of file to be opened.
      */
    KDU_AUX_EXPORT void open(kdu_compressed_target *indirect);
      /* [SYNOPSIS]
           Same as the first `open' function, but delivers its output to the
           supplied `indirect' object's `kdu_compressed_target::write'
           function.
      */
    KDU_AUX_EXPORT void open(kdu_long simulated_start_pos);
      /* [SYNOPSIS]
           This function opens a compressed target which does not actually
           write any data anywhere, but it does keep track of the last file
           position to which data was written, so that the `get_bytes_written'
           function returns the sum of the `simulated_start_pos' argument
           and the number of byte actually written by `jp2_output_box' objects
           which are opened with this target.
      */
    kdu_long get_bytes_written() { return last_write_pos; }
      /* [SYNOPSIS]
           Returns the number of bytes written to the object since it was
           last opened.  The value remains valid after the object is closed.
      */
    KDU_AUX_EXPORT void close();
      /* [SYNOPSIS]
           Also called by the destructor.  Do not call this function until
           all objects using the target have themselves been closed.
      */
  private: // Data
    friend class jp2_output_box;
    FILE *fp; // For file-based sources
    kdu_compressed_target *indirect; // When using another Kakadu target object
    bool opened_for_simulation; // If true, the above members are both NULL
    kdu_long last_write_pos; // Last write position
    bool has_rubber_box; // True if a rubber-length box has been closed
  };

/*****************************************************************************/
/*                             jp2_output_box                                */
/*****************************************************************************/

class jp2_output_box : public kdu_compressed_target {
  /* [BIND: reference]
     [SYNOPSIS]
       Provides capabilities required to write JP2 family boxes and sub-boxes
       to a `jp2_family_tgt' object.
       [//]
       It is not safe to assign one `jp2_input_box' object directly
       to another.
  */
  public: // Member functions
    KDU_AUX_EXPORT jp2_output_box();
      /* [SYNOPSIS]
           After construction, you must call `open' to associate the object
           with a particular `jp2_family_tgt' object, either directly (using
           the first form of the `open' function) or indirectly, as a sub-box
           of another box which is associated with the `jp2_family_tgt' object.
      */
    KDU_AUX_EXPORT virtual ~jp2_output_box();
      /* [SYNOPSIS]
           Note that the destructor does not close an open box, since
           destroying boxes after the `jp2_family_tgt' or after a
           super-box could cause memory faults during exception processing.
           However, this function does destroy internal buffering resources.
      */
    jp2_output_box &operator=(jp2_output_box &rhs)
      { assert(0); return *this; }
      /* [SYNOPSIS]
           This assignment operator serves to prevent actual copying of one
           `jp2_output_box' object to another.  In debug mode, it raises an
           assertion.
      */
    bool exists() { return (box_type != 0); }
      /* [SYNOPSIS]
           Returns true if the box has been opened, but not yet closed.
      */
    bool operator!() { return (box_type == 0); }
      /* [SYNOPSIS]
           Opposite of `exists'.
      */
    KDU_AUX_EXPORT virtual void
      open(jp2_family_tgt *tgt, kdu_uint32 box_type, bool rubber_length=false);
      /* [SYNOPSIS]
           Opens a new output box with the indicated box_type (usually a 4
           character code).  If the `rubber_length' argument is false, the
           contents of the box will be buffered so that the final length of
           the box may be determined and written as the first field of the
           box.  Otherwise, the box will pass its contents directly through
           to the `tgt' object as they appear.  Note that a box may be given
           a rubber length after it is opened using `set_rubber_length'. */
    KDU_AUX_EXPORT virtual void
      open(jp2_output_box *super_box, kdu_uint32 box_type,
           bool rubber_length=false);
      /* [SYNOPSIS]
           Same as the first form of the `open' function, but opens a new
           box to reside within the existing open super-box.  You may not
           write to the super-box again until this box has been closed.
      */
    KDU_AUX_EXPORT virtual void
      open(kdu_uint32 box_type) { open((jp2_family_tgt *)NULL,box_type); }
      /* [SYNOPSIS]
           This special version of the overloaded `open' function may be used
           to open a top-level box which has no associated `jp2_family_tgt'
           into which to dump its contents.  The contents of the box will
           be buffered up as required, but never written.  Before closing
           the box, however, you can always call `get_contents' to
           gain access to the box's internal buffer.
      */
    KDU_AUX_EXPORT virtual void
      open_next(kdu_uint32 box_type, bool rubber_length=false);
      /* [SYNOPSIS]
           Opens a new box immediately after the last one which was opened.
           If the last box was opened as a sub-box of some super-box, this
           function causes a new sub-box to be opened.
      */
    kdu_uint32 get_box_type() { return box_type; }
      /* [SYNOPSIS]
           Returns the box type signature, expressed as a 32-bit integer,
           unless the box is not currently open, in which case 0 is returned.
           This is also the condition which causes `exists' to return false. */
    KDU_AUX_EXPORT kdu_long get_box_length();
      /* [SYNOPSIS]
           Returns the number of bytes in the box, including its header.
           If this function is called before all bytes in the box have been
           written, the header contribution may be inaccurately calculated,
           since boxes with size greater than or equal to 2^32 require 16-byte
           headers instead of 8-byte headers.  Usually, you will wait until
           you are about to close the box (or even after you have closed
           the box) before calling this function.
      */
    KDU_AUX_EXPORT int get_header_length();
      /* [SYNOPSIS]
           Returns the number of bytes in the box header.  If this function
           is called before all bytes in the box have been written,
           the returned length may be too small, sice boxes with size greater
           than or equal to 2^32 require 16-byte headers instead of 8-byte
           headers.  Usually, you will wait until you are about to close he
           box (or even after you have closed the box) before calling this
           function.
      */
    const kdu_byte *get_contents(kdu_long &contents_length)
      {
        if ((box_type == 0) || write_immediately || output_failed)
          { contents_length = 0; return NULL; }
        contents_length = (restore_size > 0)?restore_size:cur_size;
        return buffer;
      }
      /* [SYNOPSIS]
           This function may be used to gain READ-ONLY access to the
           current contents of a box which is being generated.  If any
           of the box contents have been written already, or the box is
           not open, the function returns NULL.  Also, you should generally
           consider the returned buffer to be invalid (and inaccessible)
           after any subsequent operation on the box, including subsequent
           calls to `write', `set_rubber_length' or `set_target_size'.
         [RETURNS]
           NULL if the box is closed, or if any of the box contents have
           already been written to a target or super-box.  Otherwise, the
           function returns a pointer to the first byte of the box
           contents which have been buffered up internally.  You may
           read from this buffer, but you should not write to it.
         [ARG: contents_length]
           Used to return the total number of bytes which can be read
           from the returned buffer.  This is the number of bytes which
           have been written to the box so far, including the contents of
           any sub-boxes, which have been fully written.
      */
    KDU_AUX_EXPORT void set_rubber_length();
      /* [SYNOPSIS]
           Signals to the box that it can flush any buffered data and pass all
           future body bytes directly to the output from now on.  This call is
           legal only when the box is already open.  The box will
           have a zero-valued length field and must be the last box in the
           data stream.  This function is generally invoked whenever a
           sub-box is created with rubber length or is assigned rubber length
           by one of its own sub-boxes.
      */
    KDU_AUX_EXPORT virtual void set_target_size(kdu_long num_bytes);
      /* [SYNOPSIS]
           Implements `kdu_compresssed_target::set_target_size', flushing any
           buffered box contents immediately after writing the header, using
           the indicated length.
         [ARG: num_bytes]
           Total number of bytes in the box contents (not including the
           header).  Some or all of these may already have been written.  If
           the box has been assigned a rubber length, so that the header
           has already been written, this function has no effect.
      */
    KDU_AUX_EXPORT virtual void write_header_last();
      /* [SYNOPSIS]
           This function is provided to facilitate the generation of very
           long boxes, where the box length cannot be determined in advance,
           and we do not wish to buffer the box's contents.  Currently, an
           error will be generated unless the underlying `jp2_family_tgt'
           object supports repositioning.  For this, it must either have
           been opened with a file as the target, or it must have been
           opened with a `kdu_compressed_target' object which itself supports
           repositioning via `kdu_compressed_target::start_rewrite' and
           `kdu_compressed_target::end_rewrite'.
           [//]
           Since the function might not succeed if the above conditions are
           not met, you should try not to use it unless absolutely
           necessary.  The `mj2_target' object uses it to write movies,
           since the entire movie must be written first, before the
           length can be determined to write the box header.  Contiguous
           codestream boxes embedded in a JPX file may need to be written using
           this function, since the length of the codestream cannot generally
           be known ahead of time, and the JPX file may embody multiple
           codestreams.
      */
    KDU_AUX_EXPORT virtual bool close();
      /* [SYNOPSIS]
           Flushes the contents of the box to the output.  If the box had
           rubber length, there will be nothing to flush.  If the output
           device is (or was) unable to accept all of the box contents, this
           function returns false.
      */
    KDU_AUX_EXPORT virtual bool start_rewrite(kdu_long backtrack);
      /* [SYNOPSIS]
           Implements `kdu_compressed_target::start_rewrite'.  Most
           `jp2_output_box' objects can support rewriting.  This is
           certainly true if the box contents are being buffered in memory.
           It is also true for top-level boxes whose `jp2_family_tgt'
           object offers repositioning services (either because was opened
           with a file, or because it was opened with another
           `jp2_compressed_target' object which itself supports
           rewriting).  However, there can be circumstances where JP2
           boxes will not support rewriting.  For example, if a box's
           ultimate target does not support repositioning and the box's
           length has already been signalled using `set_target_size', calls
           to the present function will have to return false.
      */
    KDU_AUX_EXPORT virtual bool end_rewrite();
      /* [SYNOPSIS]
           Implements `kdu_compressed_target::end_rewrite'.
      */
    KDU_AUX_EXPORT virtual bool write(const kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           Implements `kdu_compressed_target::write', writing the indicated
           number of bytes to the body of the box, from the supplied buffer.
           Returns false if the output device is unable to receive all of the
           supplied data (e.g., disk full, or overwriting a rewrite section).
      */
    KDU_AUX_EXPORT bool write(kdu_uint32 dword);
      /* [SYNOPSIS]
           Writes a big-endian 32-bit unsigned integer to the body of the box,
           returning false if the output device is unable to receive all of
           the supplied data for some reason.
      */
    bool write(kdu_int32 dword) { return write((kdu_uint32) dword); }
      /* [SYNOPSIS]
           Writes a big-endian 32-bit signed integer to the body of the box,
           returning false if the output device is unable to receive all of
           the supplied data for some reason.
      */
    KDU_AUX_EXPORT bool write(kdu_uint16 word);
      /* [BIND: no-java]
         [SYNOPSIS]
           Writes a big-endian 16-bit unsigned integer to the body of the box,
           returning false if the output device is unable to receive all of
           the supplied data for some reason.
      */
    bool write(kdu_int16 word) { return write((kdu_uint16) word); }
      /* [SYNOPSIS]
           Writes a big-endian 16-bit signed integer to the body of the box,
           returning false if the output device is unable to receive all of
           the supplied data for some reason.
      */
    bool write(kdu_byte byte)
      { return write(&byte,1); }
      /* [SYNOPSIS]
           Writes a single byte to the body of the box, returning false if the
           output device is unable to receive all of the supplied data for some
           reason.
      */
  private: // Helper functions
    void write_header();
  private: // Data
    kdu_uint32 box_type; // 0 if box does not exist or has been closed.
    bool rubber_length;
    jp2_family_tgt *tgt;
    jp2_output_box *super_box;
    size_t buffer_size;
    kdu_long cur_size, box_size; // `box_size' = -1 unless actually known
    kdu_long restore_size; // -1 unless in a rewrite section
    kdu_byte *buffer;
    bool output_failed; // True if `tgt' device failed to absorb all data
    bool write_immediately; // True if we do not need any more buffering
    bool write_header_on_close; // If `write_header_last' was called.
  };

/*****************************************************************************/
/*                              jp2_dimensions                               */
/*****************************************************************************/

#define JP2_COMPRESSION_TYPE_NONE     ((int) 0)
#define JP2_COMPRESSION_TYPE_MH       ((int) 1)
#define JP2_COMPRESSION_TYPE_MR       ((int) 2)
#define JP2_COMPRESSION_TYPE_MMR      ((int) 3)
#define JP2_COMPRESSION_TYPE_JBIG_B   ((int) 4)
#define JP2_COMPRESSION_TYPE_JPEG     ((int) 5)
#define JP2_COMPRESSION_TYPE_JLS      ((int) 6)
#define JP2_COMPRESSION_TYPE_JPEG2000 ((int) 7)
#define JP2_COMPRESSION_TYPE_JBIG2    ((int) 8)
#define JP2_COMPRESSION_TYPE_JBIG     ((int) 9)

class jp2_dimensions {
  /* [BIND: interface]
     [SYNOPSIS]
       Manages dimensional attributes recorded in the "Image Header Box" of
       a JP2/JPX/MJ2 file (and perhaps other JP2-family file formats).
       Although this box is mandatory, the information which it records is
       redundant with respect to that provided by an embedded JPEG2000
       code-stream, and also sufficiently incomplete as to be of only limited
       use.  Applications are advised to use the dimensional attributes
       offered by JPEG2000 code-streams, except where the embedded
       code-stream does not conform to the JPEG2000 set of standards (e.g.,
       it might be a JPEG or JBIG code-stream).  The `get_compression_type'
       function may be used to determine whether or not the code-stream
       type is JPEG2000.
       [//]
       Objects of this class are merely interfaces to an internal
       object, which cannot be directly created by an application.
       Creation and destruction of the internal object, as well as saving
       and reading of the associated JP2 boxes are capabilities reserved
       for the internal machinery associated with the file format manager
       which provides the interface.
       [//]
       Objects which can provide a non-empty `jp2_dimensions' interface
       include `jp2_source', `jp2_target', `jpx_codestream_source',
       `jp2_codestream_target', `mj2_video_source' and `mj2_video_target'.
  */
  public: // Lifecycle member functions
    jp2_dimensions() { state = NULL; }
    jp2_dimensions(j2_dimensions *state) { this->state = state; }
      /* [SYNOPSIS]
           Do not call this function yourself.  See the `jp2_dimensions'
           overview for more on how to get access to a non-empty interface.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns false if the interface is empty, meaning that it is not
           currently associated with any internal implementation.  See the
           `jp2_dimensions' overview for more on how to get access to a
           non-empty interface.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS] Opposite of `exists', returning true if the interface is
         empty.
      */
  // --------------------------------------------------------------------------
  public: // Initialization member functions
    KDU_AUX_EXPORT void copy(jp2_dimensions src);
      /* [SYNOPSIS]
           Use instead of `init' to copy the dimensions from the `src'
           object to another.
      */
    KDU_AUX_EXPORT void
      init(kdu_coords size, int num_components, bool unknown_space=true,
           int compression_type=JP2_COMPRESSION_TYPE_JPEG2000);
      /* [SYNOPSIS]
           Initializes the internal object, specifying the dimensions occupied
           by the image on the canvas and the number of image components.
           It is illegal to call this function unless there is an internal
           object (`exists' must return true) and that object has not yet
           been initialized.  To complete the initialization, precision
           information must be supplied for each image component (see below).
           [//]
           It is often more convenient to perform the configuration with a
           single call to the second form of this overloaded function, which
           uses the code-stream's `siz_params' object to recover the relevant
           information.
           [//]
           While the `init' function provides all information required to
           write the JP2 image header box, it does not provide enough
           information to determine the codestream profile or whether or
           not it uses specific Part-2 extensions.  This information is
           required to reliably determine compatibility information for
           JPX files and whether or not the codestream can be legally
           embedded within a JP2 or MJ2 file.  For this reason, you are
           strongly encouraged to call the `finalize_compatibility'
           function at an appropriate point.
         [ARG: size]
           Holds the height and width of the image region on the canvas.
           Specifically, these are exactly the dimensions returned by
           `kdu_codestream::get_dims' when invoked with a negative `comp_idx'
           argument.  Note, however, that the `kdu_codestream::get_dims'
           function also identifies the location of the image region on the
           canvas -- an important property which is not recorded here.
           The limited dimensional information recoverable from the JP2
           "Image Header" box is almost entirely useless, since it cannot be
           mapped to image component dimensions without considering
           sub-sampling factors and the precise location of the image region
           on the canvas.  It may be useful in establishing the size of a
           scanned page when used in conjunction with explicit capture
           resolution information.  It may also be useful for describing
           images which are not compressed at all (i.e., if `compression_type' 
           is `JP2_COMPRESSION_TYPE_NONE').
         [ARG: num_components]
           Number of image components.  Although there will often be one
           image component for each colour channel, the number of image
           components can be either smaller than (see `jp2_palette') or
           larger than (if some components are not used to represent colour
           channels) the number of colour channels.
         [ARG: unknown_space]
           True if the colour space information accessed via a
           `jp2_colour' object which uses this code-stream is not known to
           be correct.  This is often the case in practice, where an sRGB
           space is often assumed in the absence of any specific knowledge
           concerning the primaries or gamma correction.  It is not all that
           clear why this attribute is included in the "Image Header" box,
           but so be-it.
         [ARG: compression_type]
           For JPEG2000 code-streams, use `JP2_COMPRESSION_TYPE_JPEG2000'.
           For a description of other values, see the description of the
           `get_compression_type' function.
      */
    KDU_AUX_EXPORT void
      init(siz_params *siz, bool unknown_space=true);
      /* [SYNOPSIS]
           Initializes the object, based upon the information which
           will be included in the JPEG2000 code-stream SIZ marker segment.
           This is the best way to ensure consistency between the redundant
           JP2 "Image Header" box and the embedded code-stream.
           [//]
           As with the first form of the `init' function, you are
           strongly encouraged to call the `finalize_compatibility'
           function at an appropriate point after the present function,
           so as to extract codestream profile and extension information
           required to correctly initialize compatibility information for
           the containing file format.
      */
    KDU_AUX_EXPORT void
      finalize_compatibility(kdu_params *root);
      /* [SYNOPSIS]
           This function has no impact over the generation of a valid
           Image Header Box.  However, it provides important information to
           allow the correct generation of File Type and Reader Requirements
           boxes for a JPX file and also to determine the legality of JP2
           and MJ2 files.  These processes require knowledge of the
           profile and/or specific Part-2 extensions used by the associated
           codestream.  This information, in turn, requires access to the
           finalized parameter attributes associated with the codestream's
           main header.  Since this information might not be available when
           the `jp2_dimensions' object is first initialized, this separate
           function allows the information to be acquired later.
           [//]
           When writing a JP2/MJ2/JPX file, this function should be called
           with the root of the parameter sub-system returned by
           `kdu_codestream::access_siz', after the `kdu_params::finalize_all'
           function has been used to finalize all main header attributes,
           at least.  It should be called before the relevant file header
           is written.  If you do not call this function, the generated
           file might be written with misleading File Type and/or
           Reader Requirements boxes.
           [//]
           When reading a JP2/MJ2/JPX file, there is no need to ever call
           this function, unless you are going to use the present object
           to initialize another `jp2_dimensions' object using the
           `copy' member function.  In this case, you should call the
           present function prior to `copy' if you wish to include the copied
           dimensional information in a JP2/MJ2/JPX file with correctly
           generated File Type and Reader Requirements boxes.
           [//]
           If the codestream associated with this object is not a JPEG2000
           codestream, you will not be calling this function.
      */
    KDU_AUX_EXPORT void
      finalize_compatibility(int profile, bool is_jpx_baseline);
      /* [SYNOPSIS]
           Use in place of the first form of the `finalize_compatibility'
           function if you explicitly know the compatibility information.
           Pay close attention to the definitions of `is_part1' and
           `is_baseline' given below, or else let the first form of the
           function figure them out for you.  The present function presents
           a useful alternative if you wish to write header information
           before actually creating a `kdu_codestream' object and configuring
           it with the coding parameters that the first form of the function
           uses to figure out compatibility information.
         [ARG: profile]
           Set this argument to the value reported by the `siz_params' object's
           `Sprofile' attribute in the relevant codestream.
         [ARG: is_jpx_baseline]
           This argument is relevant only if `profile' is `Sprofile_PART2',
           meaning that Part-2 codestream features are being used.  In that
           case, this argument indicates whether the collection of Part-2
           codestream features used is compatible with the definition of
           "JPX baseline".  Specifically, for JPX baseline compatibility, at
           most the `Sextensions_MCT' and/or `Sextensions_CURVE' features can
           be used (as returned by the `Sextensions' attribute of
           `siz_params').  Moreover, the only allowable MCT (multi-component
           transform) extensions are those which involve a single transform
           stage, containing a single transform block (component-collection),
           which implements an irreversible matrix transform.
      */
    KDU_AUX_EXPORT void
      finalize_compatibility(jp2_dimensions src);
      /* [SYNOPSIS]
           Same as first form of the `finalize_compatibility' function, but
           copies the compatibility information from the `src' object,
           rather than deriving it from codestream parameter interformation.
      */
    KDU_AUX_EXPORT void
      set_precision(int component_idx, int bit_depth, bool is_signed=false);
      /* [SYNOPSIS]
           Sets the bit-depth and signed/unsigned characteristics of the
           indicated image component.  This function must be used after
           the first form of the overloaded `init' function, to complete
           the initialization of each individual image component.  Its
           use may be avoided by employing the second form of the `init'
           function.
           [//]
           Note that the JP2/JPX file formats were really designed with
           unsigned representations in mind.  In some cases, the use of
           signed representations is illegal and in others it may create
           ambiguity.  Users are strongly encouraged to adopt unsigned
           representations when using these file formats.
         [ARG: component_idx]
           Must be in the range 0 through `num_components'-1.
         [ARG: bit_depth]
           Number of bits per sample.  This information is identical to
           that recorded in the `Sprecision' attribute of a `cod_params'
           parameter object.  Legal bit-depths can be in the range 1 through
           38; however, applications are ill-advised to select bit-depths
           larger than about 28, since otherwise reversible compression or
           decompression on 32-bit architectures might not be achievable.
         [ARG: is_signed]
           True if the component's sample values have a signed representation
           with a nominal range of -2^{B-1} to 2^{B-1}-1 where B is the
           value of the `bit_depth' argument.  Otherwise, the sample values
           have a nominally unsigned representation in the range 0 to 2^B-1.
           This information is identical to that recorded in the `Ssigned'
           attribute of a `cod_params' paramter object.
      */
    KDU_AUX_EXPORT void
      set_ipr_box_available();
      /* [SYNOPSIS]
           Call this function after `init' if you know that you will also
           be writing an IPR (Intellectual Property Rights) box for the
           codestream whose dimensions are described by this object.
      */

  // --------------------------------------------------------------------------
  public: // Access member functions.
    KDU_AUX_EXPORT kdu_coords get_size();
      /* [SYNOPSIS]
           Returns the size of the image region on the canvas.  This
           information is almost entirely useless by itself, unless the
           image compression type happens to be `JP2_COMPRESSION_TYPE_NONE'.
           Moreover, it is redundant with the more complete and useful
           information embedded in the code-stream's SIZ marker segment.  If
           the size is needed, it should generally be obtained by invoking
           the more reliable and informative `kdu_codestream::get_dims' member
           function, assuming that the application is willing to construct a
           `kdu_codestream' object.
           [//]
           See `init' for more information.
      */
    KDU_AUX_EXPORT int get_num_components();
      /* [SYNOPSIS]
           Returns the number of code-stream image components.  As with
           `get_size', this information is best provided by the
           `kdu_codestream' object.
      */
    KDU_AUX_EXPORT bool colour_space_known();
      /* [SYNOPSIS]
           Returns false if the colour space information provided by the
           `jp2_colour' object is unreliable.  See `init' for more information.
      */
    KDU_AUX_EXPORT int get_bit_depth(int component_idx);
      /* [SYNOPSIS]
           Returns the bit-depth of the indicated image component, which may
           be anywhere in the range 1 through 38!  As with `get_size', this
           information is best provided by the `kdu_codestream' object.
         [ARG: component_idx]
           Component indices start form 0.
      */
    KDU_AUX_EXPORT bool get_signed(int component_idx);
      /* [SYNOPSIS]
           Indicates whether the indicated image component has a signed
           representation.  A false return value means that the quantities
           are unsigned.  Note that all quantities processed by the core
           Kakadu system are signed quantities.  To obtain unsigned values,
           an offset of half the dynamic range must be added to them.  In
           most cases, this is the appropriate action to take before
           subjecting the data to any palette mapping, lookup tables, colour
           conversion, etc.  However, if this function returns true, the
           data should be left in its signed representation and all of the
           above operations should be performed on the signed quantities.
           For lookup table operations, this will mean that negative values
           must be clipped to 0.
           [//]
           As suggested by the foregoing discussion, the JP2 file format was
           really designed primarily with unsigned representations in mind.
           Some things make no sense, are clumsy or else are illegal unless
           the samples have an unsigned representation.
         [ARG: component_idx]
           Component indices start form 0.
      */
    KDU_AUX_EXPORT int get_compression_type();
      /* [SYNOPSIS]
           Returns an integer identifier which should match one of the
           compression types described below.  Note, the Kakadu tool set does
           not attempt to provide explicit support for anything other than
           JPEG2000 code-streams.
           [>>] `JP2_COMPRESSION_TYPE_NONE':
                The image data is uncompressed, with samples stored in
                component-interleaved fashion.  All components must have
                the same bit-depth and the same dimensions.  Each scan
                line commences on a byte boundary, but subsequent samples
                are bit packed so that at most the trailing bits of the last
                byte in each scan line are unused.
           [>>] `JP2_COMPRESSION_TYPE_MH':
                The basic algorithm in ITU-T Rec. T.4, known as modified
                Huffman coding.  This option is valid only for bi-level
                imagery.
           [>>] `JP2_COMPRESSION_TYPE_MR':
                The basic algorithm in ITU-T Rec. T.4, known as modified
                READ.  This option is valid only for bi-level imagery.
           [>>] `JP2_COMPRESSION_TYPE_MMR':
                The basic algorithm in ITU-T Rec. T.6, known as modified
                modified READ.  This option is valid only for bi-level imagery.
           [>>] `JP2_COMPRESSION_TYPE_JBIG_B':
                JBIG compression; use this value only for bi-level imagery.
           [>>] `JP2_COMPRESSION_TYPE_JPEG':
                JPEG compression.
           [>>] `JP2_COMPRESSION_TYPE_JLS':
                JPEG-LS compression.
           [>>] `JP2_COMPRESSION_TYPE_JPEG2000':
                Any valid JPEG2000 code-stream, including Part 1 and Part 2
                code-streams.
           [>>] `JP2_COMPRESSION_TYPE_JBIG2':
                The JBIG2 algorithm.
           [>>] `JP2_COMPRESSION_TYPE_JBIG':
                Any code-stream allowed by the original JBIG standard,
                including multi-level imagery.
      */
    KDU_AUX_EXPORT bool is_ipr_box_available();
      /* [SYNOPSIS]
           Returns true if the `ipr_box_available' flag is set, meaning that
           an IPR (Intellectual Property Rights) box is present in the
           JP2/JPX data source, describing the ownership of the imagery
           in the codestream whose dimensions are being described by this
           object.
      */
  // --------------------------------------------------------------------------
  private: // Data
    j2_dimensions *state;
  };

/*****************************************************************************/
/*                               jp2_palette                                 */
/*****************************************************************************/

class jp2_palette {
  /* [BIND: interface]
     [SYNOPSIS]
       Manages colour palettes wherever they are used in JP2-family files.
       If present, this information is stored in a JP2 "Palette" box.
       [//]
       The principle intent of palettes is to facilitate the efficient
       representation of images with very few distinct colours, where a
       single image component provides indices into the palette.  However,
       a JP2 palette is really just a collection of lookup tables, with
       the restriction that all lookup tables have the same number of
       entries.  It is legal to apply any of the lookup tables to any
       of the code-stream image components in order to create new mapped
       channel values.  Any or all of these mapped channels may then be
       associated with particular colour reproduction functions.
       These associations are described by the JP2 "Component Mapping" and
       "Channel Definition" boxes (possibly with the help of a "Codestream
       Registration" box), whose information is collectively accessed and
       manipulated via the `jp2_channels' interface.
       [//]
       Objects of this class are merely interfaces to an internal
       object, which cannot be directly created by an application.
       Creation and destruction of the internal object, as well as saving
       and reading of the associated JP2 boxes are capabilities reserved
       for the internal machinery associated with the file format manager
       which provides the interface.
       [//]
       Objects which can provide a non-empty `jp2_palette' interface
       include `jp2_source', `jp2_target', `jpx_codestream_source',
       `jpx_codestream_target', `mj2_video_source' and `mj2_video_target'.
  */
  public: // Lifecycle member functions
    jp2_palette() { state = NULL; }
    jp2_palette(j2_palette *state) { this->state = state; }
      /* [SYNOPSIS]
           Do not call this function yourself.    See the `jp2_palette'
           overview for more on how to get access to a non-empty interface.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns false if the interface is empty, meaning that it is not
           currently associated with any internal implementation.  See the
           `jp2_palette' overview for more on how to get access to a
           non-empty interface.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS] Opposite of `exists', returning true if the interface is
         empty.
      */
  // --------------------------------------------------------------------------
  public: // Initialization member functions
    KDU_AUX_EXPORT void copy(jp2_palette src);
      /* [SYNOPSIS]
           Use instead of `init' to copy the palette from one object to
           another.
      */
    KDU_AUX_EXPORT void init(int num_luts, int num_entries);
      /* [SYNOPSIS]
           Initializes the internal palette object to have the indicated
           number of lookup tables, each with the indicated number of entries.
           It is illegal to call this function unless there is an internal
           object (`exists' must return true) and that object has not yet
           been initialized.
           [//]
           To complete the initialization, the individual component lookup
           tables must be configured using `set_lut'.
      */
    KDU_AUX_EXPORT void
      set_lut(int lut_idx, kdu_int32 *lut, int bit_depth,
              bool is_signed=false);
      /* [SYNOPSIS]
           Completes the initialization of a particular palette lookup table,
           as identified by `lut_idx'.
         [ARG: lut_idx]
           Lookup table indices run from 0 through to `num_luts'-1, where
           `num_luts' is the number of lookup tables, as supplied in the
           call to `init'.
         [ARG: lut]
           This array must have at least `num_entries' entries, where
           `num_entries' is the number of entries (common to all lookup
           tables), as supplied in the call to `init'.  The entries of this
           array must lie in the range -2^{`bit_depth'-1} to
           2^{`bit_depth'-1}-1, if `is_signed' is true.  Otherwise, the
           LUT entries must lie in the range 0 to 2^{`bit_depth'}-1.
         [ARG: bit_depth]
           Precision associated with the sample values produced by applying
           this lookup table to map any particular source image component.
           Affects the interpretation of the entries in the `lut' array, as
           described above.
         [ARG: is_signed]
           Indicates whether the sample values produced by applying
           this lookup table have a signed or an unsigned representation.
           Affects the interpretation of the entries in the `lut' array,
           as described above.
      */
  // --------------------------------------------------------------------------
  public: // Access member functions
    KDU_AUX_EXPORT int get_num_entries();
      /* [SYNOPSIS]
           The input component samples are to be  clipped to the range
           0 through `entries'-1, where `entries' is the value returned
           by this function.  All lookup tables have exactly the same number
           of entries.
         [RETURNS]
           0 if no palette is being used.
      */
    KDU_AUX_EXPORT int get_num_luts();
      /* [SYNOPSIS]
           Returns the number of separate lookup tables managed by the
           palette.  Note that the standard does not insist that all (or
           indeed any) of these lookup tables actually be used to map
           code-stream image components.  Nor does it prevent any given
           LUT from being used multiple times, to map different code-stream
           image component samples.
         [RETURNS]
           0 if no palette is being used.
      */
    KDU_AUX_EXPORT int get_bit_depth(int lut_idx);
      /* [SYNOPSIS]
           Returns the bit-depth of the indicated lookup table.
         [ARG: lut_idx]
           Must be in the range 0 through `num_luts'-1, where `num_luts'
           is the value returned by `get_num_luts'.
      */
    KDU_AUX_EXPORT bool get_signed(int lut_idx);
      /* [SYNOPSIS]
           Returns true if the indicated lookup table has a signed
           representation.  Otherwise, returns false.  Note that
           Palette LUT outputs are always converted to signed representations
           anyway, for consistency with Kakadu's general principle of using
           signed representations for all internal data processing operations.
           [//]
           For this reason, there are not many applications which should
           need to know whether the representation is signed or
           unsigned, except when writing the de-palettized samples
           to a file which is capable of representing either signed
           or unsigned quantites.
         [ARG: lut_idx]
           Must be in the range 0 through `num_luts'-1, where `num_luts'
           is the value returned by `get_num_luts'.
      */
    KDU_AUX_EXPORT void get_lut(int lut_idx, float lut[]);
      /* [SYNOPSIS]
           Offsets and normalizes the original information for the indicated
           palette lookup table, writing the result to the supplied `lut'
           array.  The `lut' entries are filled out as signed, normalized
           quantities in the range -0.5 to +0.5.
         [ARG: lut_idx]
           Must be in the range 0 through `num_luts'-1, where `num_luts'
           is the value returned by `get_num_luts'.
         [ARG: lut]
           Regardless of whether `get_signed' returns true or false, the
           entries in this array are filled out with signed data,
           normalized to the range -0.5 to +0.5.  The normalization is such
           that -0.5 corresponds to a value of -2^{B-1} and +0.5 corresponds
           to a value of +2^{B-1}, where B is the bit-depth returned by
           `get_bit_depth'; unsigned LUT outputs may be obtained by adding
           +0.5 to each entry.
      */
    KDU_AUX_EXPORT void get_lut(int lut_idx, kdu_sample16 lut[]);
      /* [SYNOPSIS]
           Offsets and normalizes the original information for the indicated
           palette lookup table, writing the result to the supplied `lut'
           array.  The `lut' entries are filled out as signed, normalized
           quantities in the range -2^{KDU_FIX_POINT-1} to
           +2^{KDU_FIX_POINT-1}.  The selection of this particular
           representation is intended to facilitate interaction with the
           sample data processing operations conducted by the objects
           defined in "kdu_sample_processing.h".  See `kdu_line_buf' and
           `kdu_convert_ycc_to_rgb' for example.
         [ARG: lut_idx]
           Must be in the range 0 through `num_luts'-1, where `num_luts'
           is the value returned by `get_num_luts'.
         [ARG: lut]
           Regardless of whether `get_signed' returns true or false, the
           entries in this array are filled out with signed data,
           normalized to the range -2^{KDU_FIX_POINT-1} to
           +2^{KDU_FIX_POINT-1}.  The normalization is such
           that the lower bound here corresponds to an actual LUT value of
           -2^{B-1} and the upper bound corresponds to an actual LUT
           value of +2^{B-1}, where B is the bit-depth returned by
           `get_bit_depth'; unsigned LUT outputs may be obtained by adding
           2^{KDU_FIX_POINT-1} to each entry.
      */
  // --------------------------------------------------------------------------
  private: // Embedded state
    j2_palette *state;
  };

/*****************************************************************************/
/*                              jp2_channels                                 */
/*****************************************************************************/

class jp2_channels {
  /* [BIND: interface]
     [SYNOPSIS]
       Provides a uniform, simplified interface to the information
       associated with the JP2 "Component Mapping" and "Channel Definition"
       boxes, the JPX "Opacity" box and, to a limited extent, the JPX
       "Codestream Registration" box, as well as implementing the default
       bindings which apply where one or more of these boxes is missing.
       Together, this information serves to identify the relationship between
       each "colour" reproduction channel of a JP2 image (or a JPX compositing
       layer) and the underlying code-stream image components which generate
       it.
       [//]
       Reproduction channels may be grouped into the following three
       categories:
       [>>] Colour intensity channels, the number of which depends
            upon the colour space: 1 for monochrome; 3 for RGB and related
            colour spaces; 4 for CMYK; etc.
       [>>] Opacity channels, often called alpha channels.  A separate
            opacity channel may be defined for each colour intensity channel,
            although they will often all be the same.
       [>>] Pre-multiplied opacity channels (sometimes called pre-multiplied
            alpha).  Again, each colour intensity channel may have its own
            distinct pre-multiplied opacity channels.
       [//]
       The actual interpretation of the colour intensity channels in terms of
       a specific colour space is provided by "Colour Specification" boxes,
       whose contents are managed by the `jp2_colour' interface.
       [//]
       Objects of this class are merely interfaces to an internal
       object, which cannot be directly created by an application.
       Creation and destruction of the internal object, as well as saving
       and reading of the associated JP2 boxes are capabilities reserved
       for the internal machinery associated with the file format manager
       which provides the interface.
       [//]
       Objects which can provide a non-empty `jp2_channels' interface
       include `jp2_source', `jp2_target', `jpx_layer_source',
       `jpx_layer_target', `mj2_video_source' and `mj2_video_target'.
  */
  public: // Lifecycle member functions
    jp2_channels() { state = NULL; }
    jp2_channels(j2_channels *state) { this->state = state; }
      /* [SYNOPSIS]
           Do not call this function yourself.  See the `jp2_channels'
           overview for more on how to get access to a non-empty interface.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns false if the interface is empty, meaning that it is not
           currently associated with any internal implementation.  See the
           `jp2_channels' overview for more on how to get access to a
           non-empty interface.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS] Opposite of `exists', returning true if the interface is
         empty.
      */
  // --------------------------------------------------------------------------
  public: // Initialization member functions
    KDU_AUX_EXPORT void copy(jp2_channels src);
      /* [SYNOPSIS]
           Use instead of `init' to copy the channel definitions from one
           object to another.
      */
    KDU_AUX_EXPORT void init(int num_colours);
      /* [SYNOPSIS]
           Initializes the internal object for use with the indicated number
           of colour channels.  It is illegal to call this function unless
           there is an internal object (`exists' must return true) and that
           object has not yet been initialized.
           [//]
           To complete the initialization process, use the
           `set_colour_mapping', `set_opacity_mapping' and
           `set_premult_mapping' functions.
         [ARG: num_colours]
           The number of colours must agree with the colour space described
           by the associated `jp2_colour' object.
      */
    KDU_AUX_EXPORT void
      set_colour_mapping(int colour_idx, int codestream_component,
                         int lut_idx=-1, int codestream_idx=0);
      /* [SYNOPSIS]
           This function is used, together with `set_opacity_mapping' and
           `set_premult_mapping', to establish the relationship, if any,
           between code-stream image components and the reproduction
           functions of colour intensity, opacity and pre-multiplied opacity.
           The present function is most important, establishing the source
           of colour intensity information.
           [//]
           Each colour's intensity may be directly assigned to some
           code-stream image component.  Alternatively, it may be associated
           with the output of a palette lookup table (LUT), together with a
           code-stream image component which provides indices into the LUT.
           [//]
           The `colour_idx' argument identifies the particular intensity
           channel whose mapping is being defined.  A complete `jp2_channels'
           object must have an intensity mapping for each colour channel.
           However, the `init' function automatically installs a default
           mapping which directly associates the first `num_colours'
           code-stream components of code-stream 0 with the `num_colours'
           intensity channels.  If this is what you want, there is no need
           to explicitly call the present function.
         [ARG: colour_idx]
           The meaning of this argument depends upon the colour space.  Some
           common examples are:
           [>>] For monochrome (greyscale) imagery, the only legal value is 0.
           [>>] For RGB spaces, a colour index of 0 means red, 1 means green
                and 2 mean blue.
           [>>] For CMYK, 0 means cyan, 1 means magenta, 2 means yellow and
                3 means black.
         [ARG: codestream_component]
           Identifies the image component (starting from 0) of the
           code-stream identified by `codestream_idx' which is used to
           create the relevant colour intensity channel, either by direct
           assignment or after mapping through a palette LUT.
         [ARG: lut_idx]
           Set to -1 (the default) if the code-stream image component is to
           be assigned directly to the relevant colour intensity channel.
           Otherwise, the code-stream component samples are indices to a
           lookup table, where `lut_idx' gives the index of the lookup
           table (starting from 0), as it appears in the `jp2_palette'
           object.
         [ARG: codestream_idx]
           Index of the code-stream whose components and palette lookup
           tables (if required) are used to generate this colour intensity
           channel.  The meaning of the index for various file formats is
           as follows:
           [>>] For JP2 files, there is only one code-stream
                which can be interpreted by the file reader and it must have
                index 0.
           [>>] For JPX files, index 0 refers to the first code-stream
                (first contiguous code-stream or fragment table) and the
                first code-stream header box (if any) in the file.  Subsequent
                code-stream / code-stream header box pairs are identified by
                consecutive integers.  Note that the same code-stream may
                be used to define intensity channels in different JPX
                compositing layers.
           [>>] For MJ2 files, all code-streams must have the same
                reproduction rules and the index used here must be 0.
      */
    KDU_AUX_EXPORT void
      set_opacity_mapping(int colour_idx, int codestream_component,
                          int lut_idx=-1, int codestream_idx=0);
      /* [SYNOPSIS]
           This function is used, together with `set_colour_mapping' and
           `set_premult_mapping', to establish the relationship, if any,
           between the code-stream image components and the reproduction
           functions of colour intensity, opacity and pre-multiplied opacity.
           The present function may be used to provide opacity definitions.
           There is no obligation to provide opacity mappings; indeed, the
           default configuration created by `init' provides no opacity
           information.
           [//]
           Each opacity channel (there can be one for each
           colour) may be directly assigned to some code-stream component.
           Alternatively, it may be associated with the output of a palette
           lookup table (LUT), together with a code-stream image component
           which provides indices into the LUT.
         [ARG: colour_idx]
           Identifies the colour intensity channel whose opacity is being
           described.  The meaning of this argument is identical to that
           explained in connection with the `set_colour_mapping' function.
         [ARG: codestream_component]
           Identifies the code-stream image component (starting from 0)
           which is used to create the relevant opacity
           channel, either by direct assignment or after mapping
           through a palette LUT.
         [ARG: lut_idx]
           Set to -1 (the default) if the code-stream image component is to
           be assigned directly to the opacity channel.  Otherwise, the
           code-stream component samples are indices to a lookup table, where
           `lut_idx' gives the index of the lookup table (starting from 0),
           as it appears in the `jp2_palette' object.
         [ARG: codestream_idx]
           Index of the code-stream whose components and palette lookup
           tables (if required) are used to generate this opacity
           channel.  See the discussion which accompanies the
           `set_colour_mapping' function for more details on the meaning
           of this index and how it should be used with different file
           formats.
      */
    KDU_AUX_EXPORT void
      set_premult_mapping(int colour_idx, int codestream_component,
                          int lut_idx=-1, int codestream_idx=0);
      /* [SYNOPSIS]
           This function is used, together with `set_colour_mapping' and
           `set_opacity_mapping', to establish the relationship, if any,
           between the code-stream image components and the reproduction
           functions of colour intensity, opacity and pre-multiplied opacity.
           The present function may be used to provide pre-multiplied opacity
           definitions.  There is no obligation to provide pre-multiplied
           opacity mappings; indeed, the default configuration created by
           `init' provides no opacity or pre-multiplied opacity information.
           [//]
           Each pre-multiplied opacity channel (there can be one for each
           colour) may be directly assigned to some code-stream component.
           Alternatively, it may be associated with the output of a palette
           lookup table (LUT), together with a code-stream image component
           which provides indices into the LUT.
         [ARG: colour_idx]
           Identifies the colour intensity channel whose pre-multiplied
           opacity is being described.  The meaning of this argument is
           identical to that explained in connection with the
           `set_colour_mapping' function.
         [ARG: codestream_component]
           Identifies the code-stream image component (starting from 0)
           which is used to create the relevant
           pre-multiplied opacity channel, either by direct assignment or
           after mapping through a palette LUT.
         [ARG: lut_idx]
           Set to -1 (the default) if the code-stream image component is to
           be assigned directly to the pre-multiplied opacity channel.
           Otherwise, the code-stream component samples are indices to a
           lookup table, where `lut_idx' gives the index of the lookup table
           (starting from 0), as it appears in the `jp2_palette' object.
         [ARG: codestream_idx]
           Index of the code-stream whose components and palette lookup
           tables (if required) are used to generate this pre-multiplied
           opacity channel.  See the discussion which accompanies the
           `set_colour_mapping' function for more details on the meaning
           of this index and how it should be used with different file
           formats.
      */
    KDU_AUX_EXPORT void
      set_chroma_key(int colour_idx, kdu_int32 key_val);
      /* [SYNOPSIS]
           You may use this function to specify a chroma key.  Any pixel whose
           colour matches the chroma key exactly is understood to be fully
           transparent.  Note carefully that many potential dangers exist
           with chroma keys.  The main danger is that JPEG2000 code-streams
           are usually designed with lossy decoding in mind.  Even if the
           original image is represented losslessly, truly lossless decoding
           may not be possible during interactive remote image browsing, until
           all data has been received from a server.  Also, it is perfectly
           acceptable for a compliant decoder to discard some of the originally
           encoded data, resulting in a lossy rendition.  Furthermore,
           reconstruction at reduced resolutions is a desirable capability
           for general purpose decoders, which generally prevents reliable
           recovery of chroma keys.  Finally, unless the reversible processing
           path has been used, uncertainties on the order of a few digital
           counts typically arise as a result of numerical implementation
           differences between different compliant decoders.
           [//]
           For all of the above reasons, applications should be strongly
           discouraged from using this function.  If it is used, the
           information must be recorded in a JPX "Opacity" (opct) box, which
           places restrictions on other component mapping possibilities.
           One immediate restriction is that you may not define opacity or
           pre-multiplied opacity channels in addition to the chroma key.
           If a chroma key value is defined for any one colour intensity
           channel, a chroma key value must be defined for all `num_colours'
           colour intensity channels, where `num_colours' is the value
           supplied to `init'.
         [ARG: colour_idx]
           Identifies the colour intensity channel whose chroma key is
           being supplied.  The meaning of this argument is
           identical to that explained in connection with the
           `set_colour_mapping' function.
         [ARG: key_val]
           Defines the chroma key value.  If the colour intensity channel
           is derived directly from an image component, the bit-depth and
           signed/unsigned properties of the chroma key value are those
           of the corresponding image component.  If the colour intensity
           channel is derived from a palette LUT, the bit-depth and
           signed/unsigned properties of the chroma key value are those of
           the corresponding palette lookup table (see `jp2_palette').
      */
  // --------------------------------------------------------------------------
  public: // Access member functions
    KDU_AUX_EXPORT int get_num_colours();
      /* [SYNOPSIS]
           Returns 1 for monochrome imagery, 3 for most colour imagery,
           etc.  For more info, consult the comments appearing with the
           `init' function.
      */
    KDU_AUX_EXPORT bool
      get_colour_mapping(int colour_idx, int &codestream_component,
                         int &lut_idx, int &codestream_idx);
      /* [SYNOPSIS]
           This function is used, together with `get_opacity_mapping' and
           `get_premult_mapping', to determine the relationship, if any,
           between code-stream image components and the reproduction
           functions of colour intensity, opacity and pre-multiplied opacity.
           The present function is most important, identifying the source
           of colour intensity information.
           [//]
           Each colour's intensity may be directly assigned to some
           code-stream component.  Alternatively, it may be associated with
           the output of a palette lookup table (LUT), together with a
           code-stream image component which provides indices into the
           LUT.
         [RETURNS]
           True if `colour_idx' identifies a legal colour index.
         [ARG: colour_idx]
           See the `set_colour_mapping' function for a description of this
           argument.
         [ARG: codestream_component]
           Used to retrieve the index of the code-stream image component
           (starting from 0) which is used to create the identified colour
           intensity channel, either by direct assignment or after mapping
           through a palette LUT.
         [ARG: lut_idx]
           Set to -1 (the default) if the code-stream image component is
           assigned directly to the identified colour intensity channel.
           Otherwise, the code-stream component samples are indices to a
           lookup table, where `lut_idx' gives the index of the lookup
           table (starting from 0), as it appears in the `jp2_palette'
           object.
         [ARG: codestream_idx]
           Used to retrieve the index of the code-stream whose components
           and palette lookup tables (if required) are used to generate the
           identified colour intensity channel.  See the discussion which
           accompanies the `set_colour_mapping' function for more details
           on the meaning of this index in different file formats.
      */
    KDU_AUX_EXPORT bool
      get_opacity_mapping(int colour_idx, int &codestream_component,
                          int &lut_idx, int &codestream_idx);
      /* [SYNOPSIS]
           This function is used, together with `get_colour_mapping' and
           `get_premult_mapping', to determine the relationship, if any,
           between code-stream image components and the reproduction
           functions of colour intensity, opacity and pre-multiplied opacity.
           The present function identifies the source of opacity information,
           if any, for each colour.
           [//]
           Each colour's opacity may be directly assigned to some
           code-stream component.  Alternatively, it may be associated with
           the output of a palette lookup table (LUT), together with a
           code-stream image component which provides indices into the LUT.
         [RETURNS]
           True if a mapping is available for the relevant opacity channel.
         [ARG: colour_idx]
           See the `set_colour_mapping' function for a description of this
           argument.
         [ARG: codestream_component]
           Used to retrieve the code-stream image component (starting from 0)
           which is used to create the identified opacity channel,
           either by direct assignment or after mapping through a palette LUT.
         [ARG: lut_idx]
           Set to -1 (the default) if the code-stream image component is
           assigned directly to the opacity channel.
           Otherwise, the code-stream component samples are indices to a
           lookup table, where `lut_idx' gives the index of the lookup
           table (starting from 0), as it appears in the `jp2_palette'
           object.
         [ARG: codestream_idx]
           Used to retrieve the index of the code-stream whose components
           and palette lookup tables (if required) are used to generate the
           identified opacity channel.  See the discussion which
           accompanies the `set_colour_mapping' function for more details
           on the meaning of this index in different file formats.
      */
    KDU_AUX_EXPORT bool
      get_premult_mapping(int colour_idx, int &codestream_component,
                          int &lut_idx, int &codestream_idx);
      /* [SYNOPSIS]
           This function is used, together with `get_colour_mapping' and
           `get_opacity_mapping', to determine the relationship, if any,
           between code-stream image components and the reproduction
           functions of colour intensity, opacity and pre-multiplied opacity.
           The present function identifies the source of pre-multiplied
           opacity information, if any, for each colour.
           [//]
           Each colour's pre-multiplied opacity may be directly assigned to
           some code-stream component.  Alternatively, it may be associated
           with the output of a palette lookup table (LUT), together with a
           code-stream image component which provides indices into the LUT.
         [RETURNS]
           True if a mapping is available for the relevant pre-multiplied
           opacity channel.
         [ARG: colour_idx]
           See the `set_colour_mapping' function for a description of this
           argument.
         [ARG: codestream_component]
           Used to retrieve the code-stream image component (starting from 0)
           which is used to create the relevant pre-multiplied opacity channel,
           either by direct assignment or after mapping through a palette LUT.
         [ARG: lut_idx]
           Set to -1 (the default) if the code-stream image component is
           assigned directly to the pre-multiplied opacity channel.
           Otherwise, the code-stream component samples are indices to a
           lookup table, where `lut_idx' gives the index of the lookup
           table (starting from 0), as it appears in the `jp2_palette'
           object.
         [ARG: codestream_idx]
           Used to retrieve the index of the code-stream whose components
           and palette lookup tables (if required) are used to generate the
           identified pre-multiplied opacity channel.  See the discussion which
           accompanies the `set_colour_mapping' function for more details
           on the meaning of this index in different file formats.
      */
    KDU_AUX_EXPORT bool
      get_chroma_key(int colour_idx, kdu_int32 &key_val);
      /* [SYNOPSIS]
           Retrieves the chroma key value, if any, associated with the
           indicated colour intensity channel.  For a discussion of chroma
           keys, their dangers and limitations, consult the comments
           appearing with the `set_chroma_key' function.
         [RETURNS]
           True if the colour description includes a chroma key.  If so,
           all colour channels will have a chroma key value, and there
           will be no opacity or pre-multiplied opacity descriptions.
         [ARG: colour_idx]
           See the `set_colour_mapping' function for a description of this
           argument.
         [ARG: key_val]
           Used to return the chroma key value.  See the `set_chroma_key'
           function for a description of the key value.
      */
  // --------------------------------------------------------------------------
  private: // Data
    j2_channels *state;
  };

/*****************************************************************************/
/*                             jp2_resolution                                */
/*****************************************************************************/

class jp2_resolution {
  /* [BIND: interface]
     [SYNOPSIS]
       Manages simple geometric information which may be used to guide
       image rendering: specifically, pixel aspect ratios; original
       image capture resolution; and recommended image display resolution.
       [//]
       Objects of this class are merely interfaces to an internal
       object, which cannot be directly created by an application.
       Creation and destruction of the internal object, as well as saving
       and reading of the associated JP2 boxes are capabilities reserved
       for the internal machinery associated with the file format manager
       which provides the interface.
       [//]
       Objects which can provide a non-empty `jp2_resolution' interface
       include `jp2_source', `jp2_target', `jpx_layer_source',
       `jpx_layer_target', `mj2_video_source' and `mj2_video_target'.
  */
  public: // Lifecycle member functions
    jp2_resolution() { state = NULL; }
    jp2_resolution(j2_resolution *state) { this->state = state; }
      /* [SYNOPSIS]
           Do not call this function yourself.  See the `jp2_resolution'
           overview for more on how to get access to a non-empty interface.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns false if the interface is empty, meaning that it is not
           currently associated with any internal implementation.  See the
           `jp2_resolution' overview for more on how to get access to a
           non-empty interface.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS] Opposite of `exists', returning true if the interface is
         empty.
      */
  // --------------------------------------------------------------------------
  public: // Initialization member functions
    KDU_AUX_EXPORT void copy(jp2_resolution src);
      /* [SYNOPSIS]
           Use instead of `init' to copy the channel definitions from one
           object to another.
      */
    KDU_AUX_EXPORT void init(float aspect_ratio);
      /* [SYNOPSIS]
           Initializes the contents of the internal object, setting the aspect
           ratio (vertical grid spacing divided by horizontal grid spacing)
           for display and capture resolutions to the indicated value.
           [//]
           It is illegal to call this function unless there is an internal
           object (`exists' must return true) and that object has not yet
           been initialized.
      */
    KDU_AUX_EXPORT void
      set_different_capture_aspect_ratio(float aspect_ratio);
      /* [SYNOPSIS]
           Sets the capture aspect ratio to be different to the display
           aspect ratio (as set up by `init').  This is rarely required,
           since it makes little sense for the aspect ratios to differ.
      */
    KDU_AUX_EXPORT void
      set_resolution(float resolution, bool for_display=true);
      /* [SYNOPSIS]
           Sets the vertical resolution in high resolution canvas grid
           points per metre.  If `for_display' is true, this is the default
           display resolution; oherwise, it is the capture resolution.  There
           is no need to explicitly specify either of these resolutions.  If
           no resolution is specified the relevant box will not be written in
           the JP2/JPX file, except when a non-unity aspect ratio is selected.
           In the latter case, a default display resolution box will be
           created having a default vertical display resolution of 1 grid point
           per metre.
      */
  // --------------------------------------------------------------------------
  public: // Access member functions
    KDU_AUX_EXPORT float get_aspect_ratio(bool for_display=true);
      /* [SYNOPSIS]
           Always returns a positive aspect ratio representing the vertical
           grid spacing divided by the horizontal grid spacing on the
           code-stream's high resolution canvas.  If `for_display' is false,
           the capture aspect ratio is returned instead of the display aspect
           ratio, although these should be identical in any reasonable use of
           the relevant JP2 boxes.
      */
    KDU_AUX_EXPORT float get_resolution(bool for_display=true);
      /* [SYNOPSIS]
           Returns a positive value if and only if explicit resolution
           information is available.  If `for_display' is true, the return
           value identifies the default vertical display resolution
           (in canvas grid points per metre).  Otherwise, the return value is
           the vertical capture resolution (again in canvas grid points per
           metre).  In both cases, the returned values may be multiplied by the
           aspect ratio to recover horizontal resolution.
      */
  // --------------------------------------------------------------------------
  private: // Data
    j2_resolution *state;
  };

/*****************************************************************************/
/*                            jp2_colour_space                               */
/*****************************************************************************/

enum jp2_colour_space {
    JP2_bilevel1_SPACE =0,
    JP2_YCbCr1_SPACE   =1,
    JP2_YCbCr2_SPACE   =3,
    JP2_YCbCr3_SPACE   =4,
    JP2_PhotoYCC_SPACE =9,
    JP2_CMY_SPACE      =11,
    JP2_CMYK_SPACE     =12,
    JP2_YCCK_SPACE     =13,
    JP2_CIELab_SPACE   =14,
    JP2_bilevel2_SPACE =15,
    JP2_sRGB_SPACE     =16,// The standard RGB space
    JP2_sLUM_SPACE     =17,// The standard luminance space (same gamma as sRGB)
    JP2_sYCC_SPACE     =18,// YCC derivative of sRGB
    JP2_CIEJab_SPACE   =19,
    JP2_esRGB_SPACE    =20,
    JP2_ROMMRGB_SPACE  =21,
    JP2_YPbPr60_SPACE  =22,
    JP2_YPbPr50_SPACE  =23,
    JP2_esYCC_SPACE    =24,

    JP2_iccLUM_SPACE   =100,// Mono space described by restricted ICC profile
    JP2_iccRGB_SPACE   =101,// 3-colour space given by restricted ICC profile

    JP2_iccANY_SPACE   =102,// Unrestricted ICC profile (any number of colours)
    JP2_vendor_SPACE   =200 // Vendor-defined colour space
  };

#define JP2_CIE_DAY  ((((kdu_uint32) 'C')<<24) + (((kdu_uint32) 'T')<<16))
#define JP2_CIE_D50  ((kdu_uint32) 0x00443530)
#define JP2_CIE_D65  ((kdu_uint32) 0x00443635)
#define JP2_CIE_D75  ((kdu_uint32) 0x00443735)
#define JP2_CIE_SA   ((kdu_uint32) 0x00005341)
#define JP2_CIE_SC   ((kdu_uint32) 0x00005343)
#define JP2_CIE_F2   ((kdu_uint32) 0x00004632)
#define JP2_CIE_F7   ((kdu_uint32) 0x00004637)
#define JP2_CIE_F11  ((kdu_uint32) 0x00463131)

/*****************************************************************************/
/*                               jp2_colour                                  */
/*****************************************************************************/

class jp2_colour {
  /* [BIND: interface]
     [SYNOPSIS]
       Manages colour descriptions for the colour intensity channels
       described by the `jp2_channels' interface.  These descriptions are
       stored in the JP2 "Colour Specification" box.
       [//]
       Note carefully that from Kakadu v4.1 onwards, this object no longer
       provides any colour conversion capabilities of its own.  Instead,
       those capabilities are offered by the separate `jp2_colour_converter'
       object, which may be initialized using a `jp2_colour' object.  This
       allows for multiple conversion strategies and thread-safe management
       of the conversion state information.
       [//]
       The basic JP2 file format supports three enumerated colour spaces
       (`JP2_sRGB_SPACE', `JP2_sLUM_SPACE' and `JP2_sYCC_SPACE'), together
       with restricted ICC-profiles for specifying more general monochrome
       and 3-colour "RGB-like" spaces.
       [//]
       In addition to the JP2 colour spaces, the object is able to manage the
       much richer set of colour descriptions offered by the JPX file format.
       These include a much larger set of enumerated colour spaces, arbitrary
       ICC profiles, and vendor-specific colour spaces.
       [//]
       Objects of this class are merely interfaces to an internal
       object, which cannot be directly created by an application.
       Creation and destruction of the internal object, as well as saving
       and reading of the associated JP2 boxes are capabilities reserved
       for the internal machinery associated with the file format manager
       which provides the interface.
       [//]
       Objects which can provide a non-empty `jp2_colour' interface
       include `jp2_source', `jp2_target', `jpx_layer_source',
       `jpx_layer_target', `mj2_video_source' and `mj2_video_target'.
  */
  public: // Lifecycle member functions
    jp2_colour() { state = NULL; }
    jp2_colour(j2_colour *state) { this->state = state; }
      /* [SYNOPSIS]
           Do not call this function yourself.  See the `jp2_colour'
           overview for more on how to get access to a non-empty interface.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns false if the interface is empty, meaning that it is not
           currently associated with any internal implementation.  See the
           `jp2_colour' overview for more on how to get access to a
           non-empty interface.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS] Opposite of `exists', returning true if the interface is
         empty.
      */
  // --------------------------------------------------------------------------
  public: // Initialization member functions
    KDU_AUX_EXPORT void copy(jp2_colour src);
      /* [SYNOPSIS]
           Use instead of `init' to copy the channel definitions from one
           object to another.
      */
    KDU_AUX_EXPORT void init(jp2_colour_space space);
      /* [SYNOPSIS]
           Initializes the internal colour object with one of the enumerated
           colour spaces identified by `space'.  It is illegal to call this
           function unless there is an internal object (`exists' must return
           true) and that object has not yet been initialized.
           [//]
           To specify a `JP2_CIELab_SPACE' or `JP2_CIEJab_SPACE' with
           anything other than the default parameters, use the second
           form of this overloaded function.
           [//]
           To specify a colour space via an embedded ICC profile, use the
           third form of this overloaded function.
           [//]
           To specify a vendor-specific colour space, use the fourth
           form of this overloaded function.
         [ARG: space]
           Must be one of the following (JP2/JPX enumerated colour space
           codes are also given to resolve any possible lack of clarity for
           those who have copies of the standards documents):
           [>>] `JP2_bilevel1_SPACE' (EnumCS=0) -- used to indicate a bi-level
                image in which 0 means white and 1 means black.
           [>>] `JP2_YCbCr1_SPACE' (EnumCS=1) -- commonly used for data that
                originated from a video signal.
                   (*) The primaries are based on ITU-R Rec. BT709-4, having
                chromaticities (Rx,Ry)=(0.640,0.330), (Gx,Gy)=(0.300,0.600)
                and (Bx,By)=(0.150,0.060), with a whitepoint of
                (Wx,Wy)=(0.3127,0.3290) under CIE illuminant
                D65, and a gamma function with parameters gamma=1/0.45 and
                beta=0.099.
                   (*) The opponent transform which maps gamma-corrected
                RGB to Y1, Cb1 and Cr1 is identical to the ICT (irreversible
                colour transform) defined by JPEG2000 Part 1, except that the
                Y1, Cb1 and Cr1 channels have been scaled so as to occupy
                less than the full nominal dynamic range.  In particular,
                writing Y, Cb and Cr for the values which would be produced
                by the ICT, with Y in the range 0 to 1, and Cb and Cr in the
                range -0.5 to +0.5, we have Y1 = (219*Y'+16) / 256;
                Cb1 = Cb' * 224/256; and Cr1 = Cr' * 224/256.  As an example,
                to express Y1, Cb1 and Cr1 as 8-bit unsigned numbers, the
                normalized quantities are multiplied by 256, and the
                chrominance quantities are then offset by 128.
           [>>] `JP2_YCbCr2_SPACE' (EnumCS=3) -- commmonly used for data
                that originated as RGB, but has been converted to luminance
                and chrominance samples.
                   (*) The primaries are based on ITU-R Rec. BT601-5, having
                chromaticities (Rx,Ry)=(0.630,0.340), (Gx,Gy)=(0.310,0.595) and
                (Bx,By)=(0.155,0.070), with a whitepoint of
                (Wx,Wy)=(0.3127,0.3290) under CIE illuminant D65, and with a
                gamma function governed by parameters gamma=1/0.45 and
                beta=0.099.  Note that this is the same gamma as the colour
                space above, but with different RGB primaries.
                   (*) The opponent colour transform used to map RGB to Y2, Cb2
                and Cr2 values is identical to the ICT (irreversible colour
                transform) defined by JPEG2000 Part 1, without any additional
                scaling.
           [>>] `JP2_YCbCr3_SPACE' (EnumCS=4) -- commonly used for data
                that originated from a video signal.
                   (*) The primaries are based on ITU-R Rec. BT601-5, having
                exactly the same chromaticities, whitepoint and gamma as
                `JP2_YCbCr2_SPACE'.
                   (*) The opponent colour transform used to map RGB to Y3,
                Cb3 and Cr3 values is identical to the ICT (irreversible colour
                transform) defined by JPEG2000 Part 1, except that it includes
                the additional scaling factors described above for the
                `JP2_YCbCr1_SPACE'.
           [>>] `JP2_PhotoYCC_SPACE' (EnumCS=9) -- this is the colour encoding
                method used in the Photo CD system.
                   (*) The primaries are based on ITU-R Rec. BT.709, having
                exactly the same chromaticities, whitepoint and gamma as
                `JP2_YCbCr1_SPACE'.
                   (*) The opponent colour transform used to map RGB to Y,
                C1 and C2 values is identical to the ICT (irreversible colour
                transform) defined by JPEG2000 Part 1, but the coefficients
                are scaled and offset in an unusual manner.  In particular,
                treating the Y component as an unsigned quantity in the range
                0 to 1, and the Cb and Cr components as signed quantities in
                the range -0.5 to +0.5, the Y component is scaled by 0.7133,
                while the Cb and Cr components are both scaled and offset.
                The Cb component is scaled by 0.7711 and then offset by the
                addition of +0.1094, while the Cr component is scaled by
                0.7428 and offset by the addition of +0.0352.
           [>>] `JP2_CMY_SPACE' (EnumCS=11) -- A Cyan-Magenta-Yellow colour
                space suitable for printing.  Not formally defined -- device
                dependent.  Values of 0 mean zero ink coverage, while the
                maximum nominal value corresponds to an ink coverage of 100%
                for the relevant colourant.
           [>>] `JP2_CMYK_SPACE' (EnumCS=12) -- Same as `JP2_CMY_SPACE' but
                with an extra channel for black ink.
           [>>] `JP2_YCCK_SPACE' (EnumCS=13) -- This is an opponent
                representation for the `JP2_CMYK_SPACE'.  The first 3
                components (C, M and Y) are first complemented to form
                pseudo-RGB coordinates.  Specifically, if C, M and Y are
                regarded as numbers in the range 0 to 1, complementing
                them forms R=1-C, G=1-M and B=1-Y.  The ICT (irreversible
                colour transform) defined by JPEG2000 Part 1 code-streams
                is then applied to the complemented colour channels to
                form luminance and chrominance channels.  The black (K)
                channel is unaffected by the above transformations.
           [>>] `JP2_CIELab_SPACE' (EnumCS=14) -- This is the CIE Lab
                colour space.  If you choose to use this form of the `init'
                function, you will get the default Lab parameters, as
                described in the documentation provided with the second
                form of the `init' function.
           [>>] `JP2_bilevel2_SPACE' (EnumCS=15) -- Same as
                `JP2_bilevel1_SPACE', exept that a value of 1 means white
                and 0 means black.
           [>>] `JP2_sRGB_SPACE' (EnumCS=16) -- This is the standard RGB space
                used by JP2 files.  Try to use this colour space if you have a
                choice, since that will encourage JP2-compatibility.
                   (*) The primaries are those of ITU-R Rec. BT709, having
                the same chromaticities and whitepoint as `JP2_YCbCr1_SPACE',
                except that the gamma function uses parameters gamma=2.4
                and beta=0.055, which are different from those defined by
                Rec. BT709.  These are defined in IEC 61966-2-1 (sRGB).
           [>>] `JP2_sLUM_SPACE' (EnumCS=17) -- This is the standard luminance
                space used by JP2 files.  Try to use this space if you have
                a choice of luminance specifications, since that will
                encourage JP2-compatibility.
                   (*) Uses a gamma function with gamma=2.4 and beta=0.055,
                which is the same as that defined by IEC 61966-2-1 for the
                sRGB space.
           [>>] `JP2_sYCC_SPACE' (EnumCS=18) -- This is the standard opponent
                colour space used by JP2 files.  Try to use this space if you
                have a choice of opponent colour specifications, since that
                will encourage JP2-compatibility.
                   (*) The primaries are those of ITU-R Rec. BT709, except
                that the gamma function uses parameters gamma=2.4 and
                beta=0.055, exactly as for `JP2_sRGB_SPACE'.
                   (*) The opponent colour transform used to map RGB to Y,
                C1 and C2 values is identical to the ICT (irreversible colour
                transform) defined by JPEG2000 Part 1, without any additional
                scaling.
           [>>] `JP2_CIEJab_SPACE' (EnumCS=19) -- This colour space is based
                on CIECAM97s (CIE Publication 131).  If you choose to use this
                form of the `init' function, you will get the default Jab
                parameters, as described in the documentation provided with
                the second form of the `init' function.
           [>>] `JP2_esRGB_SPACE' (EnumCS=20) -- This is an extended form of
                the sRGB colour space, designed to allow colours with a larger
                gamut than that of sRGB to be represented.  The specification
                may be found in PIMA 7667; however, the extension is simple
                enough to understand:
                   (*) The underlying primaries and gamma function are the
                same as those of `JP2_sRGB_SPACE' (i.e., Rec. 709 primaries
                with a different gamma of 2.4 and beta=0.055).  However,
                negative and abnormally large values for the linear R, G and
                B primary intensities are considered legal.  The gamma function
                processes negative channel values by processing their
                magnitude in the usual fashion and passing the sign across
                the gamma function -- that is, the gamma function is defined
                to be and odd (symmetric) function of its input.
                   (*) Regarding the gamma corrected channel intensities as
                unsigned quantities with a nominal range of 0 to 1 (remember
                that negative and abnormally large values are allowed, though),
                these values are scaled by 0.5 and then offset by adding
                exactly 0.375.
           [>>] `JP2_ROMMRGB_SPACE' (EnumCS=21) -- As defined in PIMA 7666.
           [>>] `JP2_YPbPr60_SPACE' (EnumCS=22) -- Opponent colour space
                used by HDTV systems.
                   (*) The primaries have chromaticities (0.67,0.33),
                       (0.21,0.71) and (0.15,0.06), with a D65 whitepoint
                       (0.3127,0.3290), and gamma=1/0.45, with beta=0.099.
                   (*) The opponent colour transform used to map RGB to Y,
                Pb and Pr values yields: Y'=0.2122*R+0.7013*G+0.0865*B;
                Pb'=-0.1162*R-0.3838*G+0.5*B; Pr'=0.5*R-0.4451*G-0.0549*B,
                where Y, Pb and Pr are obtained by scaling and offsetting
                Y', Pb' and Pr'.  Exactly as for the `JP2_YCbCr1_SPACE', if
                R, G and B lie in the nominal range 0 to 1, so that Y' has
                the same range and Pb' and Pr' lie in the range -0.5 to +0.5,
                the actual opponent channels are scaled such that
                Y = (219*Y'+16) / 256, Pb = Pb' * 224/256 and
                Pr = Pr' * 224/256.  This ensures that when these quantities
                are represented as unsigned 8-bit numbers, they will lie in
                the range 16 to 235 (for Y) and 16 to 240 (for Pb and Pr).
           [>>] `JP2_YPbPr50_SPACE' (EnumCS=23) -- The standard indicates
                that this space has a slightly different opponent colour
                transform than `JP2_YPbPr60_SPACE'; however, I have been
                unable to confirm this.  All information I have been able
                to lay my hands on concerning YPbPr provides the definition
                given above; it may be that the text in the standard was
                borrowed from the SPIFF documentation, and that this had
                become outdated.
           [>>] `JP2_esYCC_SPACE' (EnumCS=24) -- This is an extended form of
                sYCC, whose definition may be found in Annex B of PIMA 7667.
                Although the basic principles are similar to those associated
                with esRGB, this colour space is NOT obtained simply by
                applying the irreversible colour transform to the esRGB
                coordinates.  Instead, the following procedure is used to
                obtain esYCC coordinates.
                   (*) The colour space is based on the same primaries and
                gamma function as sRGB and esRGB (i.e., Rec. 709 primaries,
                with the different gamma of 2.4, with beta=0.055).  As with
                esRGB, the linear primary RGB coordinates are allowed to take
                on negative and abnormally large values.
                   (*) The opponent transform is obtained by first applying
                the ICT (irreversible colour transform) defined by JPEG2000
                Part 1 to the above-mentioned primaries, and then scaling the
                Cb and Cr coordinates by 0.5.  Once this is done, negative
                or abnormally large values for the luminance coordinate are
                clipped so that the extended gamut information is retained only
                in the chrominance channels.
      */
    KDU_AUX_EXPORT void
      init(jp2_colour_space space, int Lrange, int Loff, int Lbits,
           int Arange, int Aoff, int Abits,
           int Brange, int Boff, int Bbits,
           kdu_uint32 illuminant=JP2_CIE_D50,
           kdu_uint16 temperature=5000);
      /* [SYNOPSIS]
           This function may be used to initialize the `jp2_colour' object
           with a `JP2_CIELab_SPACE' or `JP2_CIEJab_SPACE' colour space
           description which has non-default parameters.
           [//]
           To understand the role of the various range and offset parameters
           we will start by describing CIE Lab.  The CIE equations for this
           colour space start by defining a collection of normalized
           linear primaries, X'=X/X0, Y'=Y/Y0 and Z'=Z/Z0, all in the range
           0 to 1, where X0, Y0 and Z0 are the whitepoint tri-stimulus
           responses for the relevant illuminant.  These are subjected to
           gamma correction, with a gamma value of 3.0 and beta=0.16.  Then
           L, a* and b* coordinates are formed from
           [>>] L = 100*Y' -- has a nominal range from 0 to 100;
           [>>] a* = 431*(X'-Y') -- has a nominal range from -85 to +85;
           [>>] b* = 172.4*(Y'-Z') -- has a nominal range from -75 to +125
           [//]
           To represent these quantities digitally, we define scaled and
           offset coordinates L', a' and b', each of which has a nominal range
           from 0 to 1.  To represent these quantities as n-bit numbers, we
           simply multiply by (2^n-1).  Specifically, we write
           [>>] L' = L / `Lrange'  + `Loff'/(2^`Lbits'-1)
           [>>] a' = a* / `Arange' + `Aoff'/(2^`Abits'-1)
           [>>] a' = a* / `Brange' + `Boff'/(2^`Bbits'-1)
           [//]
           In the above, `Lbits', `Abits' and `Bbits' are the actual precision,
           as reported by the `jp2_dimensions' object, which is associated with
           each of the 3 colour channels.
           [//]
           The weakness of this representation is obvious, since the offset
           terms should have been recorded as real-valued ratios, rather than
           integers whose meaning depends on the bit-depth.  The bit-depth is
           not recorded with colour descriptions, so the meaning of a colour
           description is not self contained -- a most regrettable oversight
           on the part of JPX!!
           [//]
           Unlike other JP2/JPX design weaknesses which we have been able
           to hide through the design of appropriate Kakadu interfaces, this
           one cannot safely be concealed.  We force the application to
           supply both the integer-valued offets and the bit-depths of
           each component here.  The bit-depths will be automatically
           checked against those identified by the `jp2_dimensions'
           object, after untangling the various interconnections between
           colour channels and code-stream components, and any inconsistency
           will result in an error being generated (through `kdu_error')
           when the relevant file header is being written.
           [//]
           We turn now to the Jab space.  This colour space is based on
           CIECAM97, which actually describes a colour appearance model
           rather than a colour space.  Jab is an encoding of 3 of the
           parameters from this colour appearance model, from which any
           of the other parameters may be recovered if desired.  Unfortunately,
           CIECAM97 may be subject to major revision, so it is not sure how
           stable any Jab image encoding can be expected to be.  A revised
           CIECAM97 proposal by Mark Fairchild in 2000 describes various
           choices which could be used for the a and b coordinates.  For
           these reasons, we do not commit ourselves to a more thorough
           explanation here and we do not implement an colour transforms
           based on CIEJab.
         [ARG: space]
           Must be one of `JP2_CIELab_SPACE' or `JP2_CIEJab_SPACE'.
         [ARG: Lrange]
           Range of luminance (L or J) coordinate.  Default value is 100.
         [ARG: Loff]
           Works together with `Lbits' to identify an offset for the
           luminance (L or J) coordinate.  Default is 0.
         [ARG: Lbits]
           Precision of the sample values used to represent the luminance
           channel.  This value must agree with the actual value returned
           by the relevant `jp2_dimensions' object's
           `jp2_dimensions::get_bit_depth' function.  The need to include
           a value here is the result of poor design in the JPX file format.
         [ARG: Arange]
           Range of the a* coordinate.  Default value is 170 for Lab and 255
           for Jab.
         [ARG: Aoff]
           Works together with `Abits' to identify an offset for the
           a* coordinate.  Default is 2^{`Abits'-1}.
         [ARG: Abits]
           Precision of the sample values used to represent the a*
           channel.  This value must agree with the actual value returned
           by the relevant `jp2_dimensions' object's
           `jp2_dimensions::get_bit_depth' function.  The need to include
           a value here is the result of poor design in the JPX file format.
         [ARG: Brange]
           Range of the b* coordinate.  Default value is 200 for Lab and
           255 for Jab.
         [ARG: Boff]
           Works together with `Bbits' to identify an offset for the
           a* coordinate.  Default is 0.75*2^{`Bbits'-1} for Lab and
           2^{`Bbits'-1} for Jab.
         [ARG: Bbits]
           Precision of the sample values used to represent the b*
           channel.  This value must agree with the actual value returned
           by the relevant `jp2_dimensions' object's
           `jp2_dimensions::get_bit_depth' function.  The need to include
           a value here is the result of poor design in the JPX file format.
         [ARG: illuminant]
           Identifies the illuminant for the `JP2_CIELab_SPACE'.  This
           argument is ignored if `space'=`JP2_CIEJab_SPACE'.  Rather than
           specifying the illuminant through its chromaticity coordinates,
           an enumeration is used.  The possible values are as follows:
           [>>] `JP2_CIE_DAY' -- CIE daylight, qualified by `temperature'
           [>>] `JP2_CIE_D50' -- CIE standard daylight at 5000K
           [>>] `JP2_CIE_D65' -- CIE standard daylight at 6500K
           [>>] `JP2_CIE_D75' -- CIE standard daylight at 7500K
           [>>] `JP2_CIE_SA'  -- CIE standard illuminant A (tungsten)
           [>>] `JP2_CIE_SC'  -- CIE standard illuminant C (??)
           [>>] `JP2_CIE_F2'  -- CIE standard fluorescent F2
           [>>] `JP2_CIE_F7'  -- CIE standard fluorescent F7
           [>>] `JP2_CIE_F11' -- CIE standard fluorescent F11
         [ARG: temperature]
           Ignored unless `space' = `JP2_CIELab_SPACE' and
           `illuminant' = `JP2_CIE_DAY'.  Gives the colour temperature for
           a CIE standard daylight (these are all defined by three power
           spectral basis functions and an equation which converts the
           temperature to weights on these three basis functions), measured
           in degrees Kelvin.
      */
    KDU_AUX_EXPORT void init(kdu_byte *icc_profile);
      /* [SYNOPSIS]
           Same as the first form of the overloaded `init' function, except
           that the colour space is defined by an embedded ICC profile,
           supplied via the buffer pointed to by the `icc_profile'
           argument.  The internal object makes a copy of this buffer.
           [//]
           The standard ICC header in this buffer identifies its length and
           other attributes, from which the object determines whether it
           conforms to the restricted ICC profile or not.  It is best, if
           possible, to provide at least one colour space description which
           conforms to the simple ICC profile, or one of the enumerated 
           colour spaces, `JP2_sRGB_SPACE', `JP2_sYCC_SPACE' or
           `JP2_sLUM_SPACE', since these are JP2-compatible.
           [//]
           To conform to the restricted ICC profile compatible with JP2, the
           profile must identify a matrixed 3-colour input space (not a
           monitor space, even though these are often functionally equivalent),
           defined with respect to the XYZ Profile Connection Space (PCS_XYZ).
      */
    KDU_AUX_EXPORT void
      init(kdu_byte uuid[], int data_bytes, kdu_byte data[]);
      /* [SYNOPSIS]
           Initializes the `jp2_colour' object to hold a vendor-specific
           description of the colour space.  The vendor specific colour
           space is identified by a 16-byte (128-bit) UUID, plus an optional
           array of parameters.  When constructing defintions for
           vendor-specific colour spaces, developers would do well to avoid
           the mistakes described above in connection with the
           `JP2_CIELab_SPACE' and `JP2_CIEJab_SPACE' definitions.  Specifically
           you should try to create definitions which are self-contained
           and do not inherently rely upon knowledge of the bit-depth at which
           the component samples are represented.  It is always possible to
           do this.
         [ARG: uuid]
           A 16-byte array (128-bit number).
         [ARG: data_bytes]
           Length of the `data' array.
         [ARG: data]
           Data containing parameters required to complete the colour
           space definition identified by the `uuid'.
      */
    KDU_AUX_EXPORT void
      init(double gamma, double beta=0.0F, int num_points=100);
      /* [SYNOPSIS]
           Same as the third form of this overloaded function, except
           that a restricted ICC profile is automatically generated to
           describe a luminance space.
           [//]
           The most commonly used form of the gamma function is not
           a pure power law, but may be described by two parameters,
           gamma and beta, as explained in Chapter 1 of the book by
           Taubman and Marcellin.  For reference, the sRGB space has
           `gamma'=2.4 and `beta'=0.055, while the NTSC RGB space
           has `gamma'=2.2 and `beta'=0.099.  If `beta'=0, the gamma
           function is a pure power law and can be efficiently
           represented by the ICC profile.  Unfortunately, this
           form is not recommended, since inversion is ill conditioned
           and can substantially amplify quantization artifacts.  For
           the more general and more useful case where beta is non-zero,
           the function must be represented through tabulated values.
           These are uniformly spaced in the non-linear (gamma corrected)
           space.  The number of tabulated points may be explicitly
           controlled using the `num_points' argument.
         [ARG: gamma]
           See above.  Note that the function does not currently accept
           gamma values less than 1.
         [ARG: beta]
           See above.  Note that beta must be in the range 0 (inclusive)
           to 1 (exclusive).
         [ARG: num_points]
           Used only if `beta'!=0, in which case the gamma function must
           be described through tabulated values.
      */
    KDU_AUX_EXPORT void
      init(const double xy_red[], const double xy_green[],
           const double xy_blue[], double gamma, double beta=0.0,
           int num_points=100, bool reference_is_D50=false);
      /* [SYNOPSIS]
           Same as the third form of this overloaded function, except
           that a restricted ICC profile is automatically generated to
           describe an RGB space having primaries, reference illuminant
           and gamma function given by the various parameters.
           [//]
           The reference primaries of the colour space are defined by
           x-y chromaticity coordinates (xy_red[0],xy_red[1]),
           (xy_green[0],xy_green[1]) and (xy_blue[0],xy_blue[1]).  The
           reference illuminant is either D50 or D65 (default), depending
           on the value of the `reference_is_D50' argument.  Equal quantities
           of the reference primaries produce the adapted whitepoint of
           this illuminant.
           [//]
           For a description of the `gamma' and `beta' parameters, consult
           the description found with the third form of this overloaded
           function.
         [ARG: xy_red]
           2-element array containing the red (first) channel's chromaticity
           coordinates.
         [ARG: xy_green]
           2-element array containing the green (second) channel's chromaticity
           coordinates.
         [ARG: xy_blue]
           2-element array containing the blue (third) channel's chromaticity
           coordinates.
         [ARG: gamma]
           See above.  Note that the function does not currently accept
           gamma values less than 1.
         [ARG: beta]
           See above.  Note that beta must be in the range 0 (inclusive)
           to 1 (exclusive).
         [ARG: num_points]
           Used only if `beta' != 0, in which case the gamma function must
           be described through tabulated values.
         [ARG: reference_is_D50]
           If true, the reference illuminant is D50 and equal quantities of
           the three channel values produce the adapted whitepoint for this
           illuminant.  Otherwise, the reference illuminant is D65, and equal
           quantities of the three channel values produce the adapted
           whitepoint for D65.
      */
  // --------------------------------------------------------------------------
  public: // Access member functions
    KDU_AUX_EXPORT int get_num_colours();
      /* [SYNOPSIS]
           Returns the number of colour channels associated with this
           colour space definition.  All colour spaces associated with a
           given image or compositing layer must have the same number of
           colours and this number must agree with the value returned by
           `jp2_channels::get_num_colours'.  For vendor-specific colour
           spaces, this function may return 0.
      */
    KDU_AUX_EXPORT jp2_colour_space get_space();
      /* [SYNOPSIS]
           If the return value is `JP2_iccLUM_SPACE', `JP2_iccRGB_SPACE',
           the colour space is defined by a restricted ICC profile.  If
           it is `JP2_iccANY_SPACE', the colour space is defined by a
           general ICC profile.  In both cases, further information may
           be obtained using `get_icc_profile'.
           [//]
           If the return value is `JP2_CIELab_SPACE' or `JP2_CIEJab_SPACE',
           additional parameters must be recovered via the
           `get_lab_params' or `get_jab_params' functions, as appropriate.
           [//]
           If the return value is `JP2_vendor_SPACE', the UUID and any
           associated parameters must be recovered via the `get_vendor_params'
           function.
           [//]
           Otherwise, the function returns one of the enumerated colour
           spaces, `JP2_bilevel1_SPACE', `JP2_YCbCr1_SPACE',
           `JP2_YCbCr2_SPACE', `JP2_YCbCr3_SPACE', `JP2_PhotoYCC_SPACE',
           `JP2_CMY_SPACE', `JP2_CMYK_SPACE', `JP2_YCCK_SPACE',
           `JP2_bilevel2_SPACE', `JP2_sRGB_SPACE', `JP2_sLUM_SPACE',
           `JP2_sYCC_SPACE', `JP2_esRGB_SPACE', `JP2_ROMMRGB_SPACE',
           `JP2_YPbPr60_SPACE', `JP2_YPbPr50_SPACE' or `JP2_esYCC_SPACE'.
           The definitions of these various colour spaces are supplied with
           the first form of the `init' function.
      */
    KDU_AUX_EXPORT bool is_opponent_space();
      /* [SYNOPSIS]
           Returns true if the colour space identified by the `get_space'
           function is an opponent colour space.  Opponent colour spaces
           are those which represent the primaries (usually gamma corrected)
           using a weighted average (luminance channel) and two colour
           differences (chrominance channels.
      */
    KDU_AUX_EXPORT int get_precedence();
      /* [SYNOPSIS]
           Returns the precedence value assigned to this colour
           description within its JP2/JPX colour description (`colr') box.
           Legal values lie in the range -128 to +127, with larger values
           corresponding to colour descriptions which should generally
           be considered first when attempting to render the colour data.
      */
    KDU_AUX_EXPORT kdu_byte get_approximation_level();
      /* [SYNOPSIS]
           Returns the approx value assigned to this colour description
           within its JP2/JPX colour description (`colr') box.  Only 4
           values are defined for this value by the JP2/JPX standard, and these
           are:
           [>>] 0 -- This value is to be used with JP2 files; it should not
                     strictly be used in JPX files, but readers should be
                     tolerant, since there are a vast number of reasons for
                     using JPX files which have nothing whatsoever to do with
                     colour understanding.  For example, a JPX file may very
                     reasonably be constructed by merging images from
                     multiple JP2 files, so how can the merging utility
                     possibly infer higher knowledge about the colour space
                     approximation level.
           [>>] 1 -- This colour description is an accurate representation of
                     the colour space actually used.
           [>>] 2 -- This colour description is a very high quality
                     approximation to the actual colour space.
           [>>] 3 -- This colour description is a reasonable approximation
                     to the actual colour space.
           [>>] 4 -- This colour description is a poor approximation to the
                     actual colour space.
           [//]
           All other values are reserved.
      */
    KDU_AUX_EXPORT kdu_byte *get_icc_profile(int *num_bytes=NULL);
      /* [SYNOPSIS]
           Returns a pointer to a block of memory which holds the embedded
           ICC profile.  Returns NULL if the colour specification does not
           involve an embedded ICC profile (as indicated by
           `get_space').
           [//]
           The returned block of memory, if any, belongs to the
           internal object; no attempt should be made to
           delete it or to use it beyond the lifetime of the
           object which originally supplied this interface.
         [ARG: num_bytes]
           If not NULL, this argument is used to return the length of the
           profile -- written to *`num_bytes'.  It is also possible to
           determine the length of an ICC profile from its own header,
           although this might not be convenient for some applications.
      */
    KDU_AUX_EXPORT bool
      get_lab_params(int &Lrange, int &Loff, int &Lbits,
                     int &Arange, int &Aoff, int &Abits,
                     int &Brange, int &Boff, int &Bbits,
                     kdu_uint32 &illuminant, kdu_uint16 &temperature);
      /* [SYNOPSIS]
           Call this function to recover parameters if `get_space' returns
           `JP2_CIELab_SPACE'.  The meaning of the values returned via the
           various arguments is identical to the meaning of the values
           supplied to their namesakes in the second form of the `init'
           function.
         [RETURNS]
           Returns false if `get_space' returns anything other than
           `JP2_CIELab_SPACE'.
      */
    KDU_AUX_EXPORT bool
      get_jab_params(int &Lrange, int &Loff, int &Lbits,
                     int &Arange, int &Aoff, int &Abits,
                     int &Brange, int &Boff, int &Bbits);
      /* [SYNOPSIS]
           Call this function to recover parameters if `get_space' returns
           `JP2_CIEJab_SPACE'.  The meaning of the values returned via the
           various arguments is identical to the meaning of the values
           supplied to their namesakes in the second form of the `init'
           function.
         [RETURNS]
           Returns false if `get_space' returns anything other than
           `JP2_CIEJab_SPACE'.
      */
    KDU_AUX_EXPORT bool check_cie_default();
      /* [SYNOPSIS]
           Returns true if the colour space is `JP2_CIELab_SPACE' or
           `JP2_CIEJab_SPACE' and the parameters returned by `get_lab_space'
           or `get_jab_space' (as appropriate) correspond to the default
           parameters identified by the second form of the `init' function.
      */
    KDU_AUX_EXPORT bool get_vendor_uuid(kdu_byte uuid[]);
      /* [SYNOPSIS]
           If `get_space' returns `JP2_vendor_SPACE', this function returns
           true after copying the UUID for the vendor colour space into
           `uuid'.  Otherwise, the function returns false, without touching
           `uuid'.
      */
    KDU_AUX_EXPORT kdu_byte *get_vendor_data(int *num_bytes=NULL);
      /* [SYNOPSIS]
           If `get_space' returns `JP2_vendor_SPACE', this function returns
           a pointer to the internal buffer used to represent the vendor
           colour space (everything after the 16-byte UUID in the `colr' box's
           METHDATA field).  Otherwise, the function returns a NULL pointer.
           [//]
           The returned block of memory, if any, belongs to the
           internal object; no attempt should be made to
           delete it or to use it beyond the lifetime of the
           object which originally supplied this interface.
         [ARG: num_bytes]
           If not NULL, this argument is used to return the length of the
           buffer -- written to *`num_bytes'.
      */
  // --------------------------------------------------------------------------
  private: // Data
    friend class jp2_colour_converter;
    j2_colour *state;
  };

/*****************************************************************************/
/*                          jp2_colour_converter                             */
/*****************************************************************************/

class jp2_colour_converter {
  /* [BIND: reference]
     [SYNOPSIS]
       Manages a colour conversion scheme.  Prior to Kakadu v4.1, colour
       conversion was offered directly by the `jp2_colour' object, but now
       that we are supporting the much richer family of colour descriptions
       offered by JPX, it is helpful to separate the task of colour conversion
       out into a separate object, which is initialized using the `jp2_colour'
       interface.  This has the important benefit of allowing multiple
       colour conversion strategies to be associated with a single
       `jp2_colour' description.  It also means that a single colour
       description may be safely shared amongst multiple threads of execution.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle and initialization functions
    jp2_colour_converter() { state = NULL; }
    ~jp2_colour_converter() { clear(); }
    KDU_AUX_EXPORT void clear();
      /* [SYNOPSIS]
           Deletes any colour conversion state information which may have
           been installed by `init', so that `exists' returns true.
      */
    KDU_AUX_EXPORT bool
      init(jp2_colour colour, bool use_wide_gamut=false,
           bool prefer_fast_approximations=false);
      /* [SYNOPSIS]
           Currently, this is the only initialization function.  It always
           initializes the object to convert from the colour space described
           by `colour' to sRGB (or to a luminance space with the sRGB gamma
           function), if possible.  In the future, we may provide for the
           capability to convert between two more general colour descriptions.
           [//]
           Note that it is legal to call this function again, even if it
           has been successfully called in the past.  This will have the
           effect of deleting the existing conversion specifications
           and attempting to install new ones.
           [//]
           The following limitations currently exist:
           [>>] The only enumerated colour space described by the JPX
                standard which is not supported here is `JP2_CIEJab_SPACE'.
                It is unclear how stable this colour space really is, since
                major revisions to the colour appearance model in CIECAM97
                have been proposed at least once.
           [>>] Only some of the general ICC colour descriptions associated
                with the `JP2_anyICC_SPACE' colour space can be converted.
                In particular, 3D lookup tables are not supported.
         [RETURNS]
           False if the requested colour conversion operation is not supported
           by the current implementation.  Although we endeavour to support
           the vast majority of JPX-compatible colour spaces, we do not
           currently support conversion from ICC profiles which use 3D
           lookup tables, or the `JP2_CIEJab_SPACE'.  Of course, it is also
           not possible to support vendor-specific colour descriptions here.
         [ARG: use_wide_gamut]
           Set this argument to true if you would like to avoid (or minimize
           the likelihood of) gamut truncation during the application of
           non-linear tone curves.  Many colour transformations require sample
           values to be mapped through a tone curve back to a representation
           which is linear with respect to radiance, wherein linear
           transformations are performed, and the results are subsequently
           transformed again through a non-linear tone curve (typically the
           sRGB gamma curve).  The tone curves are implemented using lookup
           tables and the inputs to each lookup table are clipped to the
           nominal range of the colour samples.  In most cases, the impact
           of this clipping is negligible, but it may affect the gamut if
           the source and output colour spaces have significantly different
           gamuts.  Especially for wide gamut input colour descriptions, the
           use of a wide gamut mapping is desirable.  The current
           implementation uses lookup tables which are twice as large
           as they nominally need to be when wide gamut processing is
           requested.
         [ARG: prefer_fast_approximations]
           Set this to true if you don't care too much about exact colour
           reproduction, but speed is more important (e.g., for video
           applications).  In this case, an approximate opponent transform
           might be used, or primary transformations might be avoided where
           the source and output primary chromaticities are
           sufficiently similar.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns true if the last call to `init' returned true.  If
           `init' has never been called, this function will return false.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if the object has been
           initialized.
      */
    KDU_AUX_EXPORT bool get_wide_gamut();
      /* [SYNOPSIS]
           Returns true if the last call to `init' was successful and
           specified the use of a wide gamut.
      */
    KDU_AUX_EXPORT bool is_approximate();
      /* [SYNOPSIS]
           Returns true if the last call to `init' was successful and the
           associated colour transformations are only approximate.  This
           may happen if the call to `init' indicated that fast approximations
           are preferred, or if an exact conversion cannot be performed.
           For example, if the original colour space is `JP2_CMYK_SPACE'
           or `JP2_CMY_SPACE', there is not sufficient information to
           know exactly how to convert to sRGB, but inverting the C, M and
           Y components should give a recognizable rendition.
      */
    KDU_AUX_EXPORT bool is_non_trivial();
      /* [SYNOPSIS]
           Returns true if the colour conversion operations is non-trivial,
           meaning that calls to `convert_lum' or `convert_rgb' (as
           appropriate) will not return immediately.
      */
  // --------------------------------------------------------------------------
  public: // Conversion functions
    KDU_AUX_EXPORT bool convert_lum(kdu_line_buf &line, int width=-1);
      /* [SYNOPSIS]
           Implements any linear or non-linear mapping required to obtain
           standard `JP2_sLUM_SPACE' samples from a single channel colour
           description.  The function may be used only if the `jp2_colour'
           object passed to `init' has exactly 1 colour channel.
           If `jp2_colour::get_space' already returns `JP2_sLUM_SPACE'
           this function may be called, but will do nothing.
           [//]
           The function compensates for differences between the tone
           reproduction curve of the input luminance data and the
           standard sRGB tone reproduction curve (gamma) used by
           `JP2_sLUM_SPACE'.
           [//]
           All conversions are performed in-situ, overwriting the original
           contents of the `line' buffer.
         [RETURNS]
           False if conversion is not possible.  This happens only if
           `init' returned false, or was never called.
         [ARG: line]
           Although `kdu_line_buf' objects can represent sample values in
           four different ways, the present function requires the data to
           have a 16-bit normalized (fixed point) representation, with a
           nominal dynamic range of -0.5 to 0.5 (offsets will be added and
           later removed, as necessary, to create unsigned data for
           processing by tone reproduction curves).  The least significant
           `KDU_FIX_POINT' bits of each 16-bit integer are the fraction bits
           of the fixed point representation.  The selection of this
           representation is intended to facilitate interaction with the
           sample data processing operations conducted by the objects
           defined in "kdu_sample_processing.h" -- see, for example,
           `kdu_synthesis' or `kdu_decoder'.
         [ARG: width]
           If negative, the function processes all `line.get_width' samples
           in the `line' buffer.  Otherwise, only the first `width' samples
           are processed (there must be at least this many samples.
      */
    KDU_AUX_EXPORT bool
      convert_rgb(kdu_line_buf &red, kdu_line_buf &green,
                  kdu_line_buf &blue, int width=-1);
      /* [SYNOPSIS]
           Implements any colour conversions required to obtain the standard
           `JP2_sRGB_SPACE' colour space from another 3 channel colour
           space.  4 channel colour spaces such as `JP2_CMYK_SPACE' or
           `JP2_YCCK_SPACE' are "converted" to an RGB rendition using only
           their first 3 colour channels, but see `convert_rgb4', for more
           options in the case of 4-colour spaces.  The function may be used
           only if the `jp2_colour' object passed to `init' describes at
           least 3 colour channels.  If `jp2_colour::get_space' already
           returns `JP2_sRGB_SPACE', this function may be called, but will
           do nothing.
           [//]
           All conversions are performed in-situ, overwriting the original
           contents of the `red', `green' and `blue' buffers.
           [//]
           Although `kdu_line_buf' objects can represent sample values in
           four different ways, the present function requires the data to
           have a 16-bit normalized (fixed point) representation, with a
           nominal dynamic range of -0.5 to 0.5 (offsets will be added and
           later removed, as necessary, to create unsigned data for
           processing by tone reproduction curves).  The least significant
           `KDU_FIX_POINT' bits of each 16-bit integer are the fraction bits
           of the fixed point representation.  The selection of this
           representation is intended to facilitate interaction with the
           sample data processing operations conducted by the objects
           defined in "kdu_sample_processing.h" -- see, for example,
           `kdu_synthesis' or `kdu_decoder'.
         [RETURNS]
           False if conversion is not possible.  This happens only if the
           call to `init' returned false, or if `init' was never called.
         [ARG: red]
           Holds sample values for the first colour channel on input.  Upon
           return, the values are overwritten with the colour corrected red
           data of the output sRGB space.  Note that all samples have the
           signed, 16-bit fixed point representation with `KDU_FIX_POINT'
           fraction bits, as described above.
         [ARG: green]
           As for `red', but holds the second colour channel
           on input and the sRGB green samples on output.
         [ARG: blue]
           As for `red', but holds the third colour channel
           on input and the sRGB blue samples on output.
         [ARG: width]
           If negative, the function processes all samples in the line
           buffers (they must all have the same length in this case,
           as reported by `kdu_line_buf::get_width').  Otherwise, only the
           first `width' samples of each line buffer are processed and the
           buffers may have different lengths so long as each has
           sufficient samples.
      */
    KDU_AUX_EXPORT bool
      convert_rgb4(kdu_line_buf &red, kdu_line_buf &green,
                   kdu_line_buf &blue, kdu_line_buf &extra,
                   int width=-1);
      /* [SYNOPSIS]
           Same as `convert_rgb', except that an `extra' line buffer is
           supplied.  This allows all 4 channels of a 4-channel colour
           space to be converted to RGB.  Upon return, the `red', `green'
           and `blue' line buffers contain converted RGB data, whereas
           the contents of the `extra' line buffer are left unchanged.
      */
  // --------------------------------------------------------------------------
  private: // Data
    j2_colour_converter *state;
};

/*****************************************************************************/
/*                               jp2_source                                  */
/*****************************************************************************/

class jp2_source : public jp2_input_box {
  /* [BIND: reference]
     [SYNOPSIS]
       Provides full support for interacting with JP2 files.  You may wish
       to use the `jpx_source', since it is able to handle both JP2 files
       and the much more general JPX files in a consistent manner. Note
       that the present object has changed in two important ways since
       Kakadu v3.4.
       [>>] The JP2 source must now be opened indirectly by first opening a
            `jp2_family_src' object and then passing it to the `open' function.
            You must then call the `read_header' member to complete the
            process of opening the JP2 source, reading its header and
            preparing to read from the embedded contiguous code-stream.
       [>>] Whereas this object used to be an interface to an hidden internal
            object, so that copying the interface served simply to duplicate
            the reference to the internal object, this is no longer the case.
            Assigning one `jp2_source' object to another is now strictly
            prohibited, and you should pass only references to such objects
            around inside your application.  This is unlikely to impact
            many applications.
       [//]
       An important benefit of the changes described above is that it is
       possible to pass the same `jp2_family_src' object to multiple
       `jp2_input_box' or `jp2_input_box'-derived objects, so that the
       JP2 source can be examined in a variety of different ways.  In some
       cases, multiple code-streams may be available from a single source,
       and each of these can be opened and passed to `kdu_codestream::create'
       now.  A second benefit, is that the `read_header' method can be used
       with caching data sources whose cache entries evolve over time; it may
       be called multiple times, until sufficient information is available
       to deduce all relevant header information.
  */
  // --------------------------------------------------------------------------
  public: // Member functions
    KDU_AUX_EXPORT jp2_source();
    KDU_AUX_EXPORT virtual ~jp2_source();
    jp2_source &operator=(jp2_source &rhs) { assert(0); return *this; }
      /* [SYNOPSIS]
           This assignment operator serves to prevent actual copying of one
           `jp2_source' object to another.  In debug mode, it raises an
           assertion.
      */
    KDU_AUX_EXPORT virtual bool
      open(jp2_family_src *src, jp2_locator loc=jp2_locator());
      /* [SYNOPSIS]
           Overrides `jp2_input_box::open' to provide functionality
           required to check whether or not the `src' object is different
           from one with which the object was previously opened.  This is
           necessary to allow the object to be used to open a new source,
           since `close' does not actually dissociate the object from
           the JP2 source; it only closes a single JP2 box.
      */
    virtual bool open(jp2_input_box *super_box) { return false; }
      /* [SYNOPSIS]
           Overrides the second form of the `jp2_input_box::open' function.
           The function invariably returns false, since JP2 files cannot
           be found as sub-boxes of other JP2 boxes.
      */
    KDU_AUX_EXPORT bool read_header();
      /* [SYNOPSIS]
           Reads the JP2 file up until the first contiguous code-stream
           box, leaving that box open for reading and returning true once
           the main code-stream header is available for parsing.  If the
           header has already been read, this function does nothing, but
           simply returns true immediately.  If the ultimate source of
           information is a dynamic cache, which does not currently have
           enough information to parse all necessary header boxes, the
           function may return false.  A false return value simply means that
           the function must be called again later when the cache contents
           have been augmented.  If a parsing error occurs, the function will
           not return at all, but will generate an error through
           `kdu_error' -- of course, you can arrange to have exceptions
           thrown and caught in this case.  Regardless of whether the ultimate
           data source is a cache or not, as soon as this function returns
           true, it is safe to pass the object directly into the
           `kdu_codestream::create' function.
      */
    kdu_long get_header_bytes() { return header_bytes; }
      /* [SYNOPSIS]
           Returns the total number of bytes in the file which have been
           consumed by the `read_header' function.  This includes all
           boxes prior to the first contiguous code-stream box and also
           the header of that box.
           [//]
           If the ultimate information source is a dynamic cache, rather than
           a file, or a `kdu_compressed_source' object, the value returned
           by this function may not be meaningful.
      */
  // --------------------------------------------------------------------------
  public: // JP2 specific access functions
    KDU_AUX_EXPORT jp2_dimensions access_dimensions();
      /* [SYNOPSIS]
           Returns an object which may be used to access the information
           recorded in the JP2 Image Header (ihdr) and Bits Per Component
           (bpcc) boxes.
           [//]
           Returns an empty interface until the conditions required for
           successful completion of `read_header' have been met.
      */
    KDU_AUX_EXPORT jp2_palette access_palette();
      /* [SYNOPSIS]
           Returns an object which may be used to access any information
           recorded in the JP2 "Palette" box.  If there is none, the object's
           `jp2_palette::get_num_luts' function will return 0.
           [//]
           Returns an empty interface until the conditions required for
           successful completion of `read_header' have been met.
      */
    KDU_AUX_EXPORT jp2_channels access_channels();
      /* [SYNOPSIS]
           Returns an object which may be used to access information
           recorded in the JP2 "Component Mapping" and "Channel Definition"
           boxes.  The information from both boxes is merged into a uniform
           set of channel mapping rules, accessed through the returned
           `jp2_channels' object.
           [//]
           Returns an empty interface until the conditions required for
           successful completion of `read_header' have been met.
      */
    KDU_AUX_EXPORT jp2_colour access_colour();
      /* [SYNOPSIS]
           Returns an object which may be used to access information
           recorded in the JP2 "Color" box, which indicates the interpretation
           of colour image data for rendering purposes.  The returned
           `jp2_colour' object also provides convenient colour transformation
           functions, to convert data which uses a custom ICC profile into
           one of the standard rendering spaces.
           [//]
           Returns an empty interface until the conditions required for
           successful completion of `read_header' have been met.
      */
    KDU_AUX_EXPORT jp2_resolution access_resolution();
      /* [SYNOPSIS]
           Returns an object which may be used to access aspect-ratio,
           capture and suggested display resolution information, for
           assistance in some rendering applications.
           [//]
           Returns an empty interface until the conditions required for
           successful completion of `read_header' have been met.
      */
  private: // Data
    bool signature_complete;
    bool file_type_complete;
    bool header_complete;
    bool codestream_found;
    bool codestream_ready; // If main header is available
    jp2_header *header;
    kdu_long header_bytes;
    jp2_family_src *check_src;
    int check_src_id;
  };

/*****************************************************************************/
/*                               jp2_target                                  */
/*****************************************************************************/

class jp2_target : public jp2_output_box {
  /* [BIND: reference]
     [SYNOPSIS]
       Provides full support for generating or copying JP2 files.
       After calling `jp2_target::open', you may pass the object directly
       to `kdu_codestream::create'.  Note, however, that you should not
       invoke `kdu_codestream::flush' until after you have called
       `write_header' and `open_codestream'.  In general, the sequence
       of operations used to generate a JP2 file is as follows:
       [>>] Create and open a `jp2_family_tgt' object.
       [>>] Pass this object to `jp2_target::open' (not the base object's
            `jp2_output_box::open' function).
       [>>] Pass the open `jp2_target' object into `kdu_codestream::create'
       [>>] Configure header attributes by using the present object's access
            functions and initializing the various attributes
            (`jp2_dimensions', `jp2_colour', `jp2_channels', etc.) as
            appropriate.
       [>>] Call `write_header' to write the JP2 header.
       [>>] Optionally write additional boxes, opening them using the base
            object's `jp2_output_box::open_next' function, writing their
            contents (or sub-boxes) and closing them using
            `jp2_output_box::close'.
       [>>] Call `open_codestream' prior to any call to
            `kdu_codestream::flush'.
       [>>] Call the base object's `jp2_output_box::close' function.
       [>>] Optionally write additional boxes or code-streams by using
            `jp2_output_box::open_next' and/or `open_codestream', closing
            the relevant boxes once you are done with them.
       [>>] Call `jp2_family_tgt::close' when you are completely finished.
       [//]
       Note that objects of this class are no longer merely interfaces to an
       internal implementation object.  For this reason, it is now illegal
       to directly assign a `jp2_target' object to another `jp2_target' object.
       [//]
       Also note that the base object's `jp2_output_box::close' function does
       not close the file (or other target device).  It simply closes the
       currently open output box.  To close the file, you need to call
       `jp2_family_tgt::close'.
       [//]
       From Kakadu Version 4.1 the new `jpx_target' object is available
       for creating instances of the much more general JPX file format.
       You may wish to create JP2 compatible JPX files rather than just
       JP2 files.
  */
  public: // Lifecycle functions
    KDU_AUX_EXPORT jp2_target();
      /* [SYNOPSIS]
           Constructs an empty interface -- one whose `exists' function
           returns false.  Use `open' to start a new JP2 file.
      */
    KDU_AUX_EXPORT virtual ~jp2_target();
    jp2_target &operator=(jp2_target &rhs) { assert(0); return *this; }
      /* [SYNOPSIS]
           This assignment operator serves to prevent actual copying of one
           `jp2_target' object to another.  In debug mode, it raises an
           assertion.
      */
    KDU_AUX_EXPORT virtual void open(jp2_family_tgt *tgt);
      /* [SYNOPSIS]
           Do not use the base object's `jp2_output_box::open' functions to
           open a JP2 file.  Instead, use this function, which registers the
           `tgt' object internally. */
    virtual void
      open(jp2_family_tgt *tgt, kdu_uint32 box_type, bool rubber_length=false)
        { jp2_output_box::open(tgt,box_type,rubber_length); }
      /* [SYNOPSIS]
           This definition is provided just to keep compilers happy, rather
           than reporting that the base function is hidden by the presence of a
           different `open' function in the derived object.  You are not
           expected to call this function directly, but it is called internally
           from the first form of the `open' function.
      */
    virtual void
      open(jp2_output_box *super_box, kdu_uint32 box_type,
           bool rubber_length=false) { assert(0); }
      /* [SYNOPSIS]
           This function is provided only to avoid compiler warnings about
           functions in the base object `jp2_output_box' being hidden.
           However, the base object's `open' functions are supposed to be
           hidden, so calls to this function simply generate an assertion
           failure.
      */
    virtual void
      open(kdu_uint32 box_type) { assert(0); }
      /* [SYNOPSIS]
           This function is provided only to avoid compiler warnings about
           functions in the base object `jp2_output_box' being hidden.
           However, the base object's `open' functions are supposed to be
           hidden, so calls to this function simply generate an assertion
           failure.
      */
    KDU_AUX_EXPORT void write_header();
      /* [SYNOPSIS]
           Writes the JP2 file header, up until, but not including the
           contiguous code-stream box.  This function leaves the
           underlying `jp2_output_box' object closed so that you can
           use the base object's `jp2_output_box::open_next' function to open
           any custom JP2 boxes you wish to write after the header, but
           before the contiguous code-stream box.  In any event, you cannot
           pass the present object to `kdu_codestream::create' until you
           have called `write_header' and `open_codestream'.
      */
    KDU_AUX_EXPORT void open_codestream(bool rubber_length=true);
      /* [SYNOPSIS]
           Call this function after `write_header'.  Optionally, you may open
           and write some of your own boxes, being sure to close them again,
           before you call the present function.  This function opens a
           contiguous code-stream box, after which you may invoke
           `kdu_codestream::flush' on the `kdu_codestream' object into whose
           `kdu_codestream::create' function the present object was passed.
           To close the contiguous code-stream box, you must explicitly call
           the `jp2_output_box::close' function (it is a virtual function).
           [//]
           The `rubber_length' argument indicates whether or not you want the
           length of the contiguous code-stream box to be written explicitly.
           If the box has a rubber length, its length field will be set to 0
           and the box is expected to continue to the end of the file.
           Otherwise, the box's contents must be buffered internally, which
           might take a lot of memory, so setting `rubber_length' to true
           is usually the best approach.
           [//]
           It is possible to write multiple code-streams to the file.  To
           do this, close the first code-stream (`jp2_output_box::close') and
           then call the present function a second time, and so forth.
           However, note carefully that only the last code-stream box may
           have a rubber length.
           [//]
           It is possible to open other boxes after the code-stream, using
           the base object's `jp2_output_box::open_next' function.  Again,
           note that this is only possible if the code-stream box had a
           rubber length.
      */
  // --------------------------------------------------------------------------
  public: // Access to attributes which may need to be set.
    KDU_AUX_EXPORT jp2_dimensions access_dimensions();
      /* [SYNOPSIS]
           Provides an interface which may be used for setting up the
           information in the JP2 Image Header and Bits Per Component boxes.
           [//]
           You ARE REQUIRED to complete this initialization before calling
           `write_header'.  The most convenient way to initialize the
           dimensions is usually to use the second form of the overloaded
           `jp2_dimensions::init' function, passing in the finalized
           `siz_params' object returned by `kdu_codestream::access_siz'.
      */
    KDU_AUX_EXPORT jp2_colour access_colour();
      /* [SYNOPSIS]
           Provides an interface which may be used for setting up the
           information in the JP2 "Color" box.
           [//]
           You ARE REQUIRED to complete this initialization before calling
           `write_header'.
      */
    KDU_AUX_EXPORT jp2_palette access_palette();
      /* [SYNOPSIS]
           Provides an interface which may be used for setting up a
           JP2 "Palette" box.
           [//]
           It is NOT NECESSARY to access or initialize any palette
           information; the default behaviour is to not use a palette.
      */
    KDU_AUX_EXPORT jp2_channels access_channels();
      /* [SYNOPSIS]
           Provides an interface which may be used to specify the
           relationship between code-stream image components and colour
           reproduction channels (colour intensity channels, opacity
           channels, and pre-multiplied opacity channels).  This information
           is used to construct appropriate JP2 "Component Mapping" and
           "Channel Definition" boxes.
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
           Provides an interface which may be used to specify the
           aspect ratio and physical resolution of the resolution canvas
           grid.
           [//]
           It is NOT NECESSARY to access or initialize the `jp2_resolution'
           object.
      */
  // --------------------------------------------------------------------------
  private: // Data
    jp2_header *header;
    jp2_family_tgt *tgt;
    bool header_written;
  };

/*****************************************************************************/
/*                            jp2_data_references                            */
/*****************************************************************************/

class jp2_data_references {
  /* [BIND: interface]
     [SYNOPSIS]
       Manages a list of URL's which may be referenced from within some
       JP2-family data sources (e.g. JPX and MJ2) as containers of data
       which is not actually embedded in the main file.  This information
       is recorded in at most one JPX data reference (`dtbl') box or
       MJ2 data references box (`dref').
       [//]
       Objects of this class are merely interfaces to an internal
       object, which cannot be directly created by an application.
       Creation and destruction of the internal object, as well as saving
       and reading of the associated data reference box are capabilities
       reserved for the internal machinery associated with the file format
       manager which provides the interface.
       [//]
       Objects which can provide a non-empty `jp2_data_references' interface
       include `jpx_source' and `jpx_target'.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle member functions
    jp2_data_references() { state = NULL; }
    jp2_data_references(j2_data_references *state) { this->state = state; }
      /* [SYNOPSIS]
           Do not call this function yourself.  See the `jp2_data_references'
           overview for more on how to get access to a non-empty interface.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns false if the interface is empty, meaning that it is not
           currently associated with any internal implementation.  See the
           `jp2_data_references' overview for more on how to get access to a
           non-empty interface.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS] Opposite of `exists', returning true if the interface is
         empty.
      */
  // --------------------------------------------------------------------------
  public: // Initialization member functions
    KDU_AUX_EXPORT int add_url(const char *url, int url_idx=0);
      /* [SYNOPSIS]
           Use this function to add a new URL to the list maintained by
           the data reference object.  If `url_idx'=0, a URL index
           is assigned automatically.  In this case, if the URL is not
           unique, the returned index may refer to an existing entry.  Also,
           in this case, if `url' is NULL or holds an empty string, the URL
           is considered to refer to the file which contains the data
           references box, and so the returned index will be 0.
           [//]
           If the supplied `url_idx' argument is greater than 0, the
           function adds the URL as having this index, regardless of
           whether it is NULL, an empty string, or non-unique.  This
           may cause overwriting of an existing URL which was previously
           added with the same index.
         [ARG: url]
           If NULL, the entry refers to the current file.  Otherwise, any
           string supplied here will be recorded as-is in the relevant
           data-references box (`dtbl' or `dref'), without any attempt to
           verify that it is a valid URL.  For URL's which refer to the
           local file system, you are advised to use the `add_file_url'
           function instead, since it always creates valid URL's conforming
           to RFC2396.
      */
    KDU_AUX_EXPORT int add_file_url(const char *pathname, int url_idx=0);
      /* [SYNOPSIS]
           Same as `add_url', except that this function creates a valid
           URL from the supplied `pathname'.  To do this, the function
           prepends a "file:///" protocol prefix and performs all required
           hex-hex encoding of characters which are not URI-legal, in
           accordance with RFC2396.
           [//]
           The `pathname' is assumed to be relative unless it commences with
           one of the prefixes "/", "\", "<D>:/" or "<D>:\", where "<D>" refers "
           to a single letter (this form is offered to accommodate
           MS-DOS style file names).  Relative file names are assigned the
           additional prefix "./" unless they already commence with a ".".
           Thus, the relative pathname "test" would be stored as
           "file:///./test".  For absolute pathnames, the initial "/" or
           "\" character is removed before prepending the "file:///" prefix.
      */
  // --------------------------------------------------------------------------
  public: // Access member functions.
    KDU_AUX_EXPORT int get_num_urls();
      /* [SYNOPSIS]
           Returns the number of URL's stored in the object.  Indices
           supplied to `get_url' should lie in the range 1 to N where
           N is the value returned by this function.
      */
    KDU_AUX_EXPORT int find_url(const char *url);
      /* [SYNOPSIS]
           Searches for the supplied `url' string, returning its index if
           found and 0 otherwise.  Note that the 0 index is specially
           reserved for the main file, which contains the data reference
           box.  This does not correspond to any URL string.
      */
    KDU_AUX_EXPORT const char *get_url(int idx);
      /* [SYNOPSIS]
           Returns the string associated with the `idx'th entry.  The
           function returns NULL if `idx' is less than 0 or greater than
           the value returned by `get_num_urls', indicating an illegal
           value.  If `idx' is 0, the function returns an empty string,
           meaning that the reference is to the main file which contains
           the data reference box.  Any other URL which happens
           to refer back to the same file (this can happen) will also
           have an empty string returned by this function.
      */
    KDU_AUX_EXPORT const char *get_file_url(int idx);
      /* [SYNOPSIS]
           This function is similar to `get_url', with the added feature that
           it checks whether the URL that would be returned can be construed
           as a local file.  If not, the function returns NULL.  Otherwise,
           it returns the relevant file's pathname, stripping out any
           "file://" prefix and decoding any hex-hex encoded characters
           (typically, but not necessarily, spaces in the original file name).
           [//]
           Although hex-hex decoding is performed on-demand, once a file
           pathname has been recovered for any particular URL `idx', the
           string is remembered internally.  As a result, this function
           always returns exactly the same string for each `idx' value.
           [//]
           If the returned pathname commences with the prefix "./" it is a
           relative path name -- relative to the path in which the current
           file is found.  Otherwise, it should be interpreted as
           an absolute path name.
      */
  // --------------------------------------------------------------------------
  private: // Data
    j2_data_references *state;
  };

#endif // JP2_H
