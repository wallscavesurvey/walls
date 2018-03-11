/*****************************************************************************/
// File: jp2.h [scope = APPS/JP2]
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
   Defines classes and other elements which are shared by the implementations
of multiple JP2-family file formats, including JP2, JPX, JPM and MJ2.
These definitions could be promoted to a very public level, but there
is little point in doing so.  For the moment then, they are provided here
in a semi-local header file.
******************************************************************************/
#ifndef JP2_SHARED_H
#define JP2_SHARED_H

#include <stdio.h> // The C I/O functions can be a lot faster than C++ ones.
#include <assert.h>
#include "jp2.h"

/* Note Carefully:
      If you want to be able to use the "kdu_text_extractor" tool to
   extract text from calls to `kdu_error' and `kdu_warning' so that it
   can be separately registered (possibly in a variety of different
   languages), you should carefully preserve the form of the definitions
   below, starting from #ifdef KDU_CUSTOM_TEXT and extending to the
   definitions of KDU_WARNING_DEV and KDU_ERROR_DEV.  All of these
   definitions are expected by the current, reasonably inflexible
   implementation of "kdu_text_extractor".
      The only things you should change when these definitions are ported to
   different source files are the strings found inside the `kdu_error'
   and `kdu_warning' constructors.  These strings may be arbitrarily
   defined, as far as "kdu_text_extractor" is concerned, except that they
   must not occupy more than one line of text.
      When defining these macros in header files, be sure to undefine
   them at the end of the header.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("E(jp2_shared.h)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(jp2_shared.h)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu File Format Support:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu File Format Support:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


// Classes defined here
class j2_dimensions;
class j2_palette;
class j2_component_map;
class j2_channels;
class j2_resolution;
class j2_colour;
class jp2_header;
class j2_data_references;

// Classes defined elsewhere
class j2_icc_profile;
class j2_header;


/* ========================================================================= */
/*                                 Classes                                   */
/* ========================================================================= */

/*****************************************************************************/
/*                               j2_dimensions                               */
/*****************************************************************************/

class j2_dimensions {
  public: // Member functions
    j2_dimensions()
      {
        compression_type=-1; profile=2; is_jpxb_compatible=true;
        num_components=0; bit_depths=NULL;
      }
    ~j2_dimensions()
      { if (bit_depths != NULL) delete[] bit_depths; }
    bool is_initialized() { return (compression_type >= 0); }
      /* Returns true if the box has been initialized, either by
         the `j2_dimensions::init' or by `jp2_dimensions::init'. */
    bool compare(j2_dimensions *src);
      /* Returns true if the object's contents are identical to those of
         the `src' object. */
    void copy(j2_dimensions *src);
      /* Copies the contents of `src' to the present object, which must not
         have been initialized yet. */
    void init(jp2_input_box *ihrd_box);
      /* Initializes the object from the information recorded in an image
         header box.  Note that the construction might not be complete
         (`finalize' may generate an error) if components have different
         bit-depths or signed/unsigned characteristics (see below).  Note
         also that the function closes the `ihdr_box' when done. */
    void process_bpcc_box(jp2_input_box *bpcc_box);
      /* This function is called if a bits per component box is encountered
         while parsing a JP2/JPX file.  The function closes the box before
         returning.  It is legal to call this function at any time and any
         number of times -- this can be useful when parsing JPX codestream
         header boxes. */
    void finalize();
      /* Checks that the object has been completely initialized.  Generates
         an appropriate error otherwise. */
    void save_boxes(jp2_output_box *super_box);
      /* Creates an image header box and, if necessary, a bits per component
         box, saving them as sub-boxes of the supplied `super_box'. */
    int get_compression_type(int &profile)
      { profile = this->profile; return compression_type; }
      /* Used to complete compatibility information for JP2 and JPX files.
         If the `profile' information is not yet known
         (`jp2_dimensions::finalize_compatibility' has not yet been
         successfully called), the returned `profile' will be negative. */
    int get_jpxb_compatible() { return is_jpxb_compatible; }
      /* Used to complete compatibility information for JPX files. */
  private: // Data
    friend class jp2_dimensions;
    kdu_coords size;
    int compression_type;
    int profile;
    bool is_jpxb_compatible;
    int num_components;
    bool colour_space_unknown, ipr_box_available;
    int *bit_depths;
  };
  /* Notes:
        The `bit_depths' array holds one element for each image component
     with the magnitude identifying the actual number of bits used to
     represent the samples.  Negative values mean that the relevant
     component has a signed representation. */

/*****************************************************************************/
/*                                 j2_palette                                */
/*****************************************************************************/

class j2_palette {
  public: // Member functions
    j2_palette()
      {
        initialized = false; num_components = 0; num_entries = 0;
        bit_depths = NULL; luts = NULL;
      }
    ~j2_palette()
      {
        if (bit_depths != NULL) delete[] bit_depths;
        if (luts != NULL)
          {
            for (int c=0; c < num_components; c++)
              delete[] luts[c];
            delete[] luts;
          }
      }
    bool is_initialized() { return initialized; }
      /* Returns true if `init' or `jp2_palette::init' has been used. */
    bool compare(j2_palette *src);
      /* Returns true if the object's contents are identical to those of
         the `src' object. */
    void copy(j2_palette *src);
      /* Copies the contents of `src' to the present object, which must not
         have been initialized yet. */
    void init(jp2_input_box *pclr_box);
      /* Initializes the object from the information recorded in a palette
         box.  Note that the function closes the `pclr_box' when done. */
    void finalize();
      /* Checks that the object has been completely initialized.  Generates
         an appropriate error otherwise. */
    void save_box(jp2_output_box *super_box);
      /* Creates a palette box and saves it as a sub-box of the supplied
         `super-box'.  Does nothing if no palette information has been set. */
  private: // Data
    friend class jp2_palette;
    bool initialized;
    int num_components;
    int num_entries;
    int *bit_depths; // Magnitude identifies bit-depth; -ve values mean signed
    kdu_int32 **luts; // One LUT array for each component.
  };
  /* Notes:
        The values stored in the LUT's all have a signed representation and
     the binary values are all located in the most significant bit positions
     of the 32-bit signed words.
        The `bit_depths' entries should never specify bit-depths in excess of
     32 bits.  If the original values have a larger bit-depth, some of the
     least significant bits will be discarded to make them fit into the 32-bit
     word size. */

/*****************************************************************************/
/*                              j2_component_map                             */
/*****************************************************************************/

class j2_component_map {
  public: // Member functions
    j2_component_map()
      {
        max_cmap_channels = num_cmap_channels = 0; cmap_channels=NULL;
        use_cmap_box = false;
      }
    ~j2_component_map()
      { if (cmap_channels != NULL) delete[] cmap_channels; }
    bool is_initialized() { return (num_cmap_channels > 0); }
      /* Returns true if the object has been initialized either by the
         `init' function (which reads a cmap box) or by at least one
         call to `add_cmap_channel' (used when writing a JP2/JPX/MJ2 file). */
    bool compare(j2_component_map *src);
      /* Returns true if the object's contents are identical to those of
         the `src' object. */
    void copy(j2_component_map *src);
      /* Copies the contents of `src' to the present object, which must not
         have been initialized yet. */
    void init(jp2_input_box *cmap_box);
      /* Initializes the object from the information recorded in a "Component
         Mapping" box.  The function closes the `cmap_box' when done.  You
         should not call `add_cmap_channel' to add any further cmap-channel
         definitions to the object. */
    void save_box(jp2_output_box *super_box, bool force_generation=false);
      /* Creates a "Component Mapping" box, saving it as a sub-box of the
         supplied `super_box'.  If there is no palette box, the function
         does not write any component mapping box unless `force_generation'
         is true.  The `force_generation' flag must be set to true when
         writing a codestream header box which contains no palette box of
         its own, if the default JP2 header box contains a palette box and
         its component mapping box differs from the present one.  These
         conventions seem to be the only way to write a JPX file which
         conforms to the standard. */
    int add_cmap_channel(int component_idx, int lut_idx=-1);
      /* You don't call this function directly, but is is used by
         `j2_channels::add_cmap_channels' to create cmap-channels as
         required to implement the channel bindings associated with a
         `j2_channels' object.  The function searches through the existing
         set of cmap-channels, for one which uses the indicated
         component and palette LUT indices.  If none is found, a new
         cmap-channel is appended to the internal list.  In any event,
         the function returns the index of the cmap-channel. */
    int get_num_cmap_channels() { return num_cmap_channels; }
      /* Returns the number of cmap-channels described by this object.  For
         reliable results, do not call this function until after `finalize'. */
    int get_cmap_component(int cmap_channel)
      { verify_channel_query_idx(cmap_channel);
        return cmap_channels[cmap_channel].component_idx; }
    int get_cmap_lut(int cmap_channel)
      { verify_channel_query_idx(cmap_channel);
        return cmap_channels[cmap_channel].lut_idx; }
    int get_cmap_bit_depth(int cmap_channel)
      { verify_channel_query_idx(cmap_channel);
        return cmap_channels[cmap_channel].bit_depth; }
    bool get_cmap_signed(int cmap_channel)
      { verify_channel_query_idx(cmap_channel);
        return cmap_channels[cmap_channel].is_signed; }
      /* You should not need to call these two functions yourself; they are
         used by `j2_channels::find_cmap_channels' to complete the description
         of each colour channel when reading a JP2/JPX/MJ2 file.  The last
         two are also used by `j2_channels::add_cmap_channels' to recover
         bit-depth and signed/unsigned information.  None of the functions
         should be called until after `finalize' has been invoked. */
    void finalize(j2_dimensions *dimensions, j2_palette *palette);
      /* When reading a JP2/JPX file, this function should be called once
         all boxes associated with the JP2 header (JP2 files) or codestream
         header (JPX files) box have been parsed, passing in the
         `j2_dimensions' and `j2_palette' boxes which are to be used for
         the codestream in question.  When writing a JP2/JPX file, the
         function should be called immediately before any call to the
         `add_cmap_channel' function.  In practice, this means that it must
         be called before the present object can be passed to
         `j2_channels::add_cmap_channels'.
            The function extracts information about the bit-depth of each
         cmap-channel and checks for legal constructs.  The function checks
         and sets the state of the `use_cmap_box' flag based on whether or not
         there is a Palette box.  If a Component mapping box was read by
         `init', the `use_cmap_box' will have been set to true and the present
         function checks to see if a Palette box is indeed present; if not,
         the file is illegal and an appropriate error is generated through
         `kdu_error'.  For cases where a Component Mapping box was not read
         or will not be written, the present function fills in the default set
         of cmap-channel bindings, so that each image component is associated
         with a cmap-channel of the same index.
            The function leaves behind copies of the `dimensions' and
         `palette' pointers so that subsequent calls to `add_cmap_channel'
         during JP2/JPX file creation will be able to recover bit-depth
         information for each of the cmap-channels. */
  private: // Helper functions
    void verify_channel_query_idx(int idx)
      {
        if ((idx < 0) || (idx >= num_cmap_channels))
          { KDU_ERROR(e,0); e <<
              KDU_TXT("Attempting to associate a reproduction "
              "function (e.g., colour intensity, opacity, etc.) with a "
              "non-existent image channel in a JP2-family file.  The problem "
              "may be a missing or invalid Component Mapping (cmap) box, or "
              "a corrupt or illegal Channel Definitions (cdef) box.");
          }
        assert(dimensions.exists()); // If not, you have forgotten to call
                                     // `finalize' first.
      }
  private: // Structure definitions
      struct j2_cmap_channel {
          int component_idx;
          int lut_idx;
          int bit_depth; // Filled in by `finalize'
          bool is_signed; // Filled in by `finalize'
        };
  private: // Data
    bool use_cmap_box; // True if a cmap box was parsed, or is to be written
    jp2_dimensions dimensions; // Filled in by `finalize'
    jp2_palette palette; // Filled in by `finalize'
    int max_cmap_channels; // Num elements in the `cmap_channels' array
    int num_cmap_channels; // Num valid elements in the `cmap_channels' array
    j2_cmap_channel *cmap_channels;
  };
  /* Notes:
       This object serves to describe what we term "cmap-channels".  The
     purpose of "cmap-channels" is to augment the collection of image
     components offered by a code-stream with additional components which
     are generated by means of palette lookup tables.  Each cmap-channel
     is described by the `j2_cmap_channel' structure, in terms of a
     component index and an lut_idx.  If the component is mapped directly
     to the channel, the `lut_idx' member should be negative.  Otherwise
     `lut_idx' represents the index (starting from 0) of the lookup table
     found within a corresponding `j2_palette' object.
       In JPX files, both the default JP2 header and each codestream
     header box may contain a "Component Mapping" (cmap) box.  The
     cmap-channels described here are used by the `j2_channels' object
     to build a relationship between the colour channels within each
     compositing layer and the image components and palette lookup tables
     used to create the samples for those colour channels.  Whereas
     each `j2_component_map' object is associated with exactly one
     code-stream, the `j2_channels' object may use cmap-channels from
     multiple code-streams.  To create the appropriate associations while
     reading or creating a JPX file, the relevant `j2_cmap_channel'
     objects must be supplied to `j2_channels::associate_cmap' in exactly
     the same order as the corresponding code-streams appear within the
     "Codestream Registration" box.
  */

/*****************************************************************************/
/*                                j2_channels                                */
/*****************************************************************************/

class j2_channels {
  public: // Member functions
    j2_channels()
      {
        max_colours=num_colours=resolved_cmap_channels=0; channels=NULL;
        have_chroma_key = opct_opacity = opct_premult = false;
        chroma_key_buf = NULL; chroma_key_len = 0;
      }
    ~j2_channels()
      {
        if (channels != NULL) delete[] channels;
        if (chroma_key_buf != NULL) delete[] chroma_key_buf;
      }
    bool is_initialized() { return (num_colours > 0); }
      /* Returns true if the object has been initialized either by
         `j2_channels::init' or by `jp2_channels::init'. */
    bool needs_opacity_box() { return have_chroma_key; }
      /* Returns true if the channel bindings will need to be stored using
         an opacity (opct) box, regardless of whether the `save_box' is
         called with its `avoid_opct_if_possible' argument equal to true or
         false.  This test forms an important part of the test to see if
         a JPX file will be JP2 compatible. */
    bool uses_palette_colour();
      /* Returns true if any of the colour channels uses a palette
         lookup table.  Ignores the opacity information. */
    bool has_opacity();
      /* Returns true if any of the channels has associated opacity
         information (not premultiplied opacity). */
    bool has_premultiplied_opacity();
      /* Returns true if any of the channels has associated pre-multiplied
         opacity information. */
    bool compare(j2_channels *src);
      /* Returns true if the box which would be written using `save_box' is
         identical to that which would be written using `src->save_box'.
         Note that this object also manages information derived from the
         `j2_component_map' object, which is written into its own separate
         box.  That information is not compared here. */
    void copy(j2_channels *src);
      /* Copies the contents of `src' to the present object, which must not
         have been initialized yet. */
    void init(jp2_input_box *cdef_or_opct_box);
      /* Initializes the object from the information recorded in either a
         "Channel Definitions" (cdef) box, or an "Opacity" (opct) box.  The
         function closes the box when done.  Subsequently, you should use the
         `find_cmap_channels' function to link the information recovered from
         the "Channel Definitions" box with information found in the relevant
         "Component Mapping" boxes. */
    void save_box(jp2_output_box *super_box, bool avoid_opct_if_possible);
      /* Stores the channel bindings represented by this object in a
         JP2 "Channel Definitions" (cdef) box or, if possible, within a
         JPX "Opacity" (opct) box.  If the channel mapping rules correspond
         to those defined as defaults, no box will be written at all.  It
         is important that the file format writer call `finalize' and
         then `add_cmap_channels' before invoking this function.
         [//]
         If `avoid_opct_if_possible' is true, the function will not attempt
         to store the relevant information in the more compact "Opacity" box.
         This flag should generally be asserted when writing JP2 files
         and when writing the first compositing layer box of a JPX file
         so as to maximize compatibility with JP2. */
    void add_cmap_channels(j2_component_map *map, int codestream_idx);
      /* Use this function to build and discover a complete set of
         associations between colour reproduction channels and cmap-channels
         during file writing.  When writing a JPX file, the `j2_component_map'
         object associated with each successive code-stream used by the
         relevant compositing layer should be passed into this function, in
         the same order that the code-streams will appear within the
         "Codestream Registration" box, if any.  These calls should be
         performed after the call to `finalize', but prior to `save_box'.
         Calls to `j2_component_map::finalize' should be delayed until after
         the present function has been called. */
    void find_cmap_channels(j2_component_map *map, int codestream_idx);
      /* Use this function to incorporate information from individual
         code-streams' "Component Mapping" boxes after reading all relevant
         boxes.  This function should be called after both
         `j2_component_map::finalize' and `j2_channels::finalize'.  It should
         be called once for each codestream which is associated with the
         compositing layer to which this `j2_channels' object belongs, in
         the same order as those codestreams appear within a codestream
         registration box, if any.  If the internal object is found still
         to have 0 colour channels, the `finalize' function must have been
         invoked using a `num_colours' argument of 0, which can only happen
         if the only colour space definition is an inscrutinable
         vendor-specific colour space.  In this case, the present function
         will first invoke `finalize' with a number of colours which is
         equal to the number of components associated with the indicated
         codestream.  This is the best guess we can make. */
    bool all_cmap_channels_found();
      /* Use this function to check that all explicitly or implicitly
         defined channels have been mapped to some codestream image
         component. */
    void finalize(int num_colours, bool writing);
      /* When reading a JP2/JPX file, the `writing' argument should be false,
         and this function should be called once the relevant boxes have been
         parsed, but before the `find_cmap_channels' function is called.  If
         the object has not already been initialized through the reading of a
         Channel Definitions (cdef) or Opacity (opct) box, it will
         automatically be initialized here to associate the first
         `num_colours' cmap-channels with the corresponding colour intensity
         channels.
            When writing a JP2/JPX file, this function should be called with
         `writing' equal to true, prior to the `add_cmap_channels' function,
         which in turn should be called prior to the `save_box' function.  It
         serves to verify that the number of colours is consistent with the
         value passed to `jp2_channels::init' and that all colour intensity
         channels have been associated with some image component of some
         codestream; if not, they are associated with the initial set of
         components of codestream 0. */
    int get_bit_depth(int c)
      { // Used by `j2_colour::finalize'
        assert(c < num_colours);
        return channels[c].bit_depth;
      }
  private: // Structure definitions
      struct j2_channel {
          j2_channel()
            {
              for (int i=0; i < 3; i++)
                {
                  cmap_channel[i] = codestream_idx[i] =
                    component_idx[i] = lut_idx[i] = -1;
                  all_channels[i] = false;
                }
              chroma_key = 0; bit_depth = -1; is_signed = false; 
            }
          int cmap_channel[3]; // See below
          int codestream_idx[3];
          int component_idx[3];
          int lut_idx[3];
          bool all_channels[3]; // If definition holds for all colour channels
          kdu_int32 chroma_key; // Returned by `jp2_channels::get_chroma_key'
          int bit_depth; // Used for parsing/writing chroma key data
          bool is_signed; // Used for parsing/writing chroma key data
        };
  private: // Data
    friend class jp2_channels;
    friend class jx_registration;
    int max_colours;
    int num_colours;
    j2_channel *channels;
    bool have_chroma_key; // If false, `j2_channel::chroma_key' is ignored
    bool opct_opacity; // If an "opct" box was read, having OTyp=0
    bool opct_premult; // If an "opct" box was read, having OTyp=1
    int resolved_cmap_channels; // See below
    int chroma_key_len; // Length of `chroma_key_buf'
    kdu_byte *chroma_key_buf; // See below
  };
  /* Notes:
        The `j2_channel' structure describes 3 types of channel mappings
     for each colour channel.  These three types of mappings correspond
     to the 3 entries in each array member of this structure.  The first
     element in each array is used to describe the colour intensity; the
     second is used to describe opacity; and the third is used to describe
     pre-multiplied opacity.  Invalid entries are always represented by
     negative values.
        After reading a "Channel Definitions" or "Opacity" box, the
     `j2_channel::component_idx' and `j2_channel::lut_index' entries will
     all be negative.  They are filled in later by calls to the
     `j2_channels::find_cmap_channels' function.
        When preparing to write a JP2 or JPX file, the application's calls
     to `jp2_channels::init', `jp2_channels::set_colour_mapping',
     `jp2_channels::set_opacity_mapping' and
     `jp2_channels::set_premult_mapping' serve to set entries in the
     `j2_channel::codestream_idx', `j2_channel::component_idx' and
     `j2_channel::lut_idx' arrays, but the `j2_channel::cmap_channel'
     entries are not filled in until the file writer calls `add_cmap_channels'
     immediately prior to writing the "Channel Definitions" or
     "Opacity" box.
        The `resolved_cmap_channels' function is used to maintain state
     information between calls to `add_cmap_channels' or `find_cmap_channels'.
     This is needed because the cmap-channels associated with consecutive
     code-streams which are referenced by the present object are essentially
     stacked on top of one another.
        The `chroma_key_buf' member will be non-NULL only if an 'opct'
     box is parsed from a JPX file and that box contains a chroma-key
     description.  The chroma-key description cannot be unpacked until the
     precision of each colour intensity channel is known, which will not
     happen until later when the `find_cmap_channels' function is called.
     Until that point, the packed chroma-key data is stored in the
     `chroma_key_buf' array points to the start of that buffer.  The chroma
     key description is unpacked during the call to `find_cmap_channels'
     in which all channel descriptions are finally resolved.
  */

/*****************************************************************************/
/*                               j2_resolution                               */
/*****************************************************************************/

class j2_resolution {
  public: // Member functions
    j2_resolution()
      { display_ratio = capture_ratio = 0.0F;
        display_res = capture_res = 0.0F; }
      /* Does the work of the parallel `jp2_resolution::init' function. */
    bool is_initialized() { return (display_ratio > 0.0F); }
      /* Returns true if the object has been initialized either by
         `j2_resolution::init' or by `jp2_resolution::init'. */
    bool compare(j2_resolution *src)
      {
        return ((display_ratio == src->display_ratio) &&
                (capture_ratio == src->capture_ratio) &&
                (display_res == src->display_res) &&
                (capture_res == src->capture_res));
      }
      /* Returns true if the object's contents are identical to those of
         the `src' object. */
    void copy(j2_resolution *src)
      { display_ratio=src->display_ratio; capture_ratio=src->capture_ratio;
        display_res=src->display_res; capture_res=src->capture_res; }
      /* Copies the contents of `src' to the present object, which must not
         have been initialized yet. */
    void init(float aspect_ratio = 1.0F);
    bool init(jp2_input_box *res_box);
      /* Initializes the object from the information recorded in a resolution
         box.  Note that the function closes the `res_box' and returns true
         when done. If the function finds that one or more sub-boxes is
         incomplete (can only happen with a caching information source
         derived from `kdu_cache2'), the function returns false, resetting
         `res_box' to point to the beginning of the resolution box, but
         leaving it open.  This allows the function to be called again when
         there is more information available in the cache. */
    void finalize();
      /* If the object has not been explicitly initialized by a previous
         call to `init' or `jp2_resolution::init', this function fills in
         default resolution parameters. */
    void save_box(jp2_output_box *super_box);
      /* Creates a resolution box and appropriate sub-boxes inside the
         supplied `super-box'. */
  private: // Helper functions
    void parse_sub_box(jp2_input_box *box);
      /* Parses resolution values from either type of resolution sub-box.
         Closes the box for us. */
    void save_sub_box(jp2_output_box *super_box, kdu_uint32 box_type,
                      double v_res, double h_res);
      /* Creates a capture or display resolution box and writes the supplied
         vertical and horizontal resolution values into that box. */
  private: // Data
    friend class jp2_resolution;
    float display_ratio;
    float capture_ratio;
    float display_res; // > 0 if and only if display info available
    float capture_res; // > 0 if and only if capture info available
  };

/*****************************************************************************/
/*                                 j2_colour                                 */
/*****************************************************************************/

class j2_colour {
  public: // Member functions
    j2_colour();
    ~j2_colour();
    bool is_initialized() { return initialized; }
      /* Returns true if the object has been initialized either by
         `j2_colour::init' or by `jp2_colour::init'. */
    bool compare(j2_colour *src);
      /* Returns true if the object's contents are identical to those of
         the `src' object. */
    void copy(j2_colour *src);
      /* Copies the contents of `src' to the present object, which must not
         have been initialized yet. */
    void init(jp2_input_box *colr_box);
      /* Initializes the object from the information recorded in a colour
         description box.  Note that the constructor closes the
         `colr_box' when done.  If a colour box has already been encountered,
         the current box is closed immediately and its contents ignored. */
    void finalize(j2_channels *channels);
      /* Checks that the object has been correctly initialized, generating
         an error if not.  Also extracts bit-depth information and the
         actual number of colours (if we did not know the number already)
         from the supplied `channels' object.  The `channels' object must
         have been finalized and must also have had its `add_cmap_channels'
         or `find_cmap_channels' function called sufficiently often to
         find all the channel associations, along with precision information
         for all of the channels. */
    void save_box(jp2_output_box *super_box);
      /* Creates a colour description box and saves it as a sub-box of the
         supplied `super-box'. */
    bool is_jp2_compatible();
      /* Returns true if the colour representation embedded in this object is
         compatible with the limited set of colour descriptions supported by
         the elementary JP2 file format. */
    int get_num_colours() { return num_colours; }
      /* This function should return the correct number of colours at any
         point after `init' or `jp2_colour::init' has been called.  There
         is no need to wait until `finalize' has been called.  This is
         important, since `finalize' needs to be supplied with a finalized
         `j2_channels' object, and that object requires the number of
         colours, as returned here.  The function may return 0 only if the
         object has not been initialized, or a vendor colour space has
         been defined -- vendor colour spaces are inscrutinable. */
    friend class jp2_colour;
    friend class j2_colour_converter;
    bool initialized; // If `init' or `jp2_colour::init' has been called.
    jp2_colour_space space;
    int num_colours; // If `space_valid', can be 0 only if vendor colour space
    int precision[3]; // Number of bits for first 3 channel components
  private: // Data members specific to ICC and vendor profiles
    j2_icc_profile *icc_profile;
    kdu_byte vendor_uuid[16];
    int vendor_buf_length;
    kdu_byte *vendor_buf;
  private: // Data members specific to Lab and Jab colour spaces
    int range[3], offset[3]; // Lrange, Loff, etc.
    kdu_uint32 illuminant;
    kdu_uint16 temperature;
  public: // Links for including in a list
    int precedence; // Configured by `init' or `jpx_layer_target::add_colour'
    kdu_byte approx; // Configured by `init' or `jpx_layer_target::add_colour'.
    j2_colour *next;
  };

/*****************************************************************************/
/*                               jp2_header                                  */
/*****************************************************************************/

class jp2_header {
  /* This object is used to parse and generate JP2 files and MJ2 video
     tracks.  JPX files use there own mechanisms to manage the elements
     which might appear within a JP2 image header found within a JPX
     file. */
  public: // Lifecycle member functions
    jp2_header(); // Allocates storage on the heap
    jp2_header(const jp2_header &rhs); // Deliberately left unimplemented
    jp2_header &operator=(jp2_header &rhs); // Deliberately not implemented
    ~jp2_header(); // Destroys the internal heap storage.
  public: // Header transfer functions
    bool read(jp2_input_box *input);
      /* [SYNOPSIS]
           Parses information from an open JP2 Header box.  The supplied
           `input' box must already have a type code equal to
           `jp2_header_4cc'.  Generates an error (through `kdu_error') if
           a parsing error occurs.
           [//]
           Upon successful completion, the function automatically closes the
           open `input' box which is received on entry and returns true.
           [//]
           If the `input' box derives its information from a `kdu_cache2'
           object, one or more of the required sub-boxes might not yet be
           available from the cache.  In this event, the function returns
           false, leaving the `input' box open.  In this case, you may call
           the function again, with the same open `input' box, not altering
           its state in any way, once more information is available in the
           cache.  You may continue calling the function in this way, until
           it returns true, or a parsing error is encountered, causing
           a message to be delivered through `kdu_error'.
      */
    void write(jp2_output_box *open_box);
      /* [SYNOPSIS]
           Writes the contents of a JP2 header, including all sub-boxes, into
           the supplied `open_box'.  Note carefully, that unlike most
           box writing functions which form part of the Kakadu support for
           the JP2 file format family, this function accepts a box which has
           already been opened and assigned the relevant box type-code,
           `jp2_header_4cc'.  The function does not check the type code and
           it does not close the box upon exit.  The reason for this
           behavioural variation is that JP2 header boxes appear at the
           top level of a JP2 file, but at subordinate levels in other
           JP2-family file formats, such as MJ2. */
  public: // Access to component data interfaces.
    bool is_jp2_compatible();
      /* [SYNOPSIS]
           This function can only reliably be used if either of the
           `jp2_dimensions::init' functions was used to configure the
           `jp2_dimensions' information during file generation.
      */
    jp2_dimensions access_dimensions();
      /* [SYNOPSIS]
           Provides an interface which may be used for setting up the
           information in the JP2 "Image Header" box.
           [//]
           You ARE REQUIRED to complete this initialization before calling
           `write'.  The most convenient way to initialize the
           dimensions is usually to use the second form of the overloaded
           `jp2_dimensions::init' function, passing in the finalized
           `siz_params' object returned by `kdu_codestream::access_siz'.
      */
    jp2_colour access_colour();
      /* [SYNOPSIS]
           Provides an interface which may be used for setting up the
           information in the JP2 "Color" box.
           [//]
           You ARE REQUIRED to complete this initialization before calling
           `write'.
      */
    jp2_palette access_palette();
      /* [SYNOPSIS]
           Provides an interface which may be used for setting up a
           JP2 "Palette" box.
           [//]
           It is NOT NECESSARY to access or initialize any palette
           information; the default behaviour is to not use a palette.
      */
    jp2_channels access_channels();
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
    jp2_resolution access_resolution();
      /* [SYNOPSIS]
           Provides an interface which may be used to specify the
           aspect ratio and physical resolution of the resolution canvas
           grid.
           [//]
           It is NOT NECESSARY to access or initialize the `jp2_resolution'
           object.
      */
  private: // Data
    j2_header *state;    
  };

/*****************************************************************************/
/*                             j2_data_references                            */
/*****************************************************************************/

class j2_data_references {
  public: // Member functions
    j2_data_references() { num_refs = max_refs = 0;  refs = file_refs = NULL; }
    ~j2_data_references();
    void init(jp2_input_box *box);
      /* Initializes the object from the information recorded in the
         supplied open `box', which may be of type `dtbl' or `dref'.
         Closes the box before returning. */
    void save_box(jp2_output_box *box);
      /* Writes to the supplied open data references `box'.  The format
         of the data which is written depends upon whether its type is
         `dtbl' or `dref'.  The box is automatically closed before the
         function returns. */
  private: // data
    friend class jp2_data_references;
    int num_refs; // Number of valid entries in the `refs' array.
    int max_refs; // Size of `refs' array
    char **refs;
    char **file_refs;
  };


/* ========================================================================= */
/*                         Other JP2-family Constants                        */
/* ========================================================================= */

static const kdu_uint32 jp2_signature       = 0x0D0A870A;
static const kdu_uint32 jp2_brand           = jp2_4cc_to_int("jp2 ");
static const kdu_uint32 jpx_brand           = jp2_4cc_to_int("jpx ");
static const kdu_uint32 jpx_baseline_brand  = jp2_4cc_to_int("jpxb");
static const kdu_uint32 mj2_brand           = jp2_4cc_to_int("mjp2");


#undef KDU_ERROR
#undef KDU_ERROR_DEV
#undef KDU_WARNING
#undef KDU_WARNING_DEV
#undef KDU_TXT

#endif // JP2_SHARED_H
