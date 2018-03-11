/*****************************************************************************/
// File: kdu_image.h [scope = APPS/IMAGE-IO]
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
   Not central to the Kakadu framework, this file defines generic image
file I/O interfaces, which can be used to isolate file-based applications from
the details of particular image file formats.
   Supports both bottom-up and top-down image file organizations with
comparable ease.
******************************************************************************/

#ifndef KDU_IMAGE
#define KDU_IMAGE

#include <assert.h>
#include <stdlib.h> // To get `abs'
#include "kdu_elementary.h"
#include "jpx.h" // Required only to exchange meta-data with files

// Defined here:

class kdu_image_dims;
struct kdu_rgb8_palette;
class kdu_image_in_base;
class kdu_image_in;
class kdu_image_out_base;
class kdu_image_out;

// Defined elsewhere.

class kdu_line_buf;

/*****************************************************************************/
/*                              kdu_image_dims                               */
/*****************************************************************************/

class kdu_image_dims {
  /* [SYNOPSIS]
       Container for dimensions, precisions and signed/unsigned characteristics
       of one or more image components, along with resolution information,
       if available.  An object of this type must be passed to
       `kdu_image_out::kdu_image_out'.
       [//]
       In addition to component dimensions, this object also provides
       facilities for exchanging display resolution information with the
       compression/decompression application, as well as methods for
       exchanging additional meta-data and knowledge (if any) about the
       colour space.  These methods appear first, below.
  */
  public: // Member functions
    kdu_image_dims()
      {
        num_components = max_components = 0; data = NULL;
        num_forced_precisions=max_forced_precisions=0; forced_precisions=NULL;
        resolution_units_known=false; resolution_aspect_known=false;
        xpels_per_metre = ypels_per_metre = 0.0; ref_width = ref_height = 1.0;
        extra_box_head = extra_box_tail = NULL;
        has_premultiplied_alpha = has_unassociated_alpha = false;
        num_colours = colour_space_confidence = 0;
        colour_space = JP2_iccLUM_SPACE; // Any valid monochrome space will do
      }
    ~kdu_image_dims()
      {
        if (data != NULL) delete[] data;
        if (forced_precisions != NULL) delete[] forced_precisions;
        while ((extra_box_tail=extra_box_head) != NULL)
          { extra_box_head=extra_box_tail->next; delete extra_box_tail; }
      }
    void set_resolution(double ref_width, double ref_height, bool units_known,
                        double xpels_per_metre, double ypels_per_metre)
      { /* [SYNOPSIS]
             Used to set physical display resolution information, if known.
             Both the X and Y resolution values must be strictly positive.  If
             `units_known' is false, only the ratio between these quantities
             has any meaning.  In any case, the X and Y resolution values are
             expressed relative to an image resolution whose dimensions are
             given by `ref_width' and `ref_height'; these resolutions are
             scaled to match the actual dimensions of any individual image
             component.
        */
        assert((xpels_per_metre > 0.0) && (ypels_per_metre > 0.0) &&
               (ref_width > 0.0) && (ref_height > 0.0));
        this->ref_width = ref_width;  this->ref_height = ref_height;
        this->xpels_per_metre = xpels_per_metre;
        this->ypels_per_metre = ypels_per_metre;
        this->resolution_units_known = units_known;
        this->resolution_aspect_known = true;
      }
    bool get_resolution(int comp_idx, bool &units_known,
                        double &xpels_per_metre, double &ypels_per_metre)
      { /* [SYNOPSIS]
             Used to retrieve physical display resolution information, if any
             is available.  The resolution information is scaled to match the
             dimensions of the component identified by `comp_idx'.  The
             function returns false if there is no resolution information.
             Otherwise (returns true), the X and Y resolution values are
             returned as strictly positive quantities and `units_known' is
             set to true or false; in the latter case, only the ratio between
             the X and Y resolutions has any physical significance.
        */
        if ((comp_idx<0) || (comp_idx >= num_components) ||
            !resolution_aspect_known)
          { xpels_per_metre=ypels_per_metre=0.0; return (units_known=false); }
        double scale_x = ((double) get_width(comp_idx)) / ref_width;
        double scale_y = ((double) get_height(comp_idx)) / ref_height;
        xpels_per_metre = this->xpels_per_metre * scale_x;
        ypels_per_metre = this->ypels_per_metre * scale_y;
        units_known = this->resolution_units_known;
        return true;
      }
    bool get_resolution_scale_factors(int comp_idx,
                                      double &scale_x, double &scale_y)
      { /* [SYNOPSIS]
             Used to retrieve the amount by which the original image sampling
             density has been scaled due to resolution reduction.  This
             information is recovered by taking the ratio of the actual
             dimensions for component `comp_idx', as returned by `get_width'
             and `get_height', and the reference dimensions supplied to
             `set_resolution'.  This should be a good indication of
             the amount by which image size has been reduced (the factors
             should generally be <= 1) due to reconstruction at a reduced
             resolution.  The situation may be a little murky for JPX files,
             since the reference dimensions supplied to `set_resolution'
             belong to the JPX compositing layer, yet the "kdu_expand" demo
             application only does decompression of codestream data -- in
             exceptional circumstances, compositing layers may have a
             different sampling density to the codestream(s) from which
             they are composed.
             [//]
             Note that the `scale_x' and `scale_y' parameters may differ if
             the image is being decompressed after discarding one or more
             decomposition levels from a Part-2 wavelet decomposition with
             non-uniform decimation properties.
             [//]
             The function returns true if `set_resolution' was successfully
             invoked in the past; otherwise, it sets `scale_x' and `scale_y'
             to 1 and returns false, indicating that the scaling factors
             cannot actually be known.
        */
        if ((comp_idx < 0) || (comp_idx >= num_components) ||
            !resolution_aspect_known)
          { scale_x = scale_y = 1.0; return false; }
        scale_x = ((double) get_width(comp_idx)) / ref_width;
        scale_y = ((double) get_height(comp_idx)) / ref_height;
        return true;
      }
    void set_colour_info(int num_colours,
                         bool has_premultiplied_alpha=false,
                         bool has_unassociated_alpha=false,
                         int colour_space_confidence=0,
                         jp2_colour_space colour_space=JP2_iccLUM_SPACE)
      { /* [SYNOPSIS]
             This function may be used to specify some knowledge that you
             have of the colour space and/or presence of alpha components.
           [ARG: num_colours]
             This is the only non-default argument.  You must at least know
             the number of colour channels and this value must, of course,
             be compatible with the number of image components that you
             have.
             [//]
             If this value is 0, you are declaring that you know
             nothing about the colour or alpha properties of the component
             samples.  You may use the function with a 0 argument in this
             way to remove any previously declared knowledge of the colour
             representation.
           [ARG: has_premultiplied_alpha]
             Set this to true if you know that the image component which
             immediately follows the last colour component holds premultiplied
             alpha samples.
           [ARG: has_unassociated_alpha]
             Set this to true if you know that the image component which
             immediately follows the last colour component holds alpha
             samples which have not been premultiplied by the colour
             intensity samples.  Of course, you cannot set both this
             argument and `has_premultiplied_alpha' to true.
           [ARG: colour_space_confidence]
             If 0 or less, you have no idea what the actual colour space is
             and the `colour_space' argument will be ignored.  If 2 or more,
             you know the JP2/JPX name of the colour space, which is given
             by the `colour_space' argument.  A value of 1 means that you
             have a reasonable idea of the colour space, but cannot be
             certain that it precisely matches the value supplied by the
             `colour_space' argument.
        */
        this->num_colours = num_colours;
        this->has_premultiplied_alpha = has_premultiplied_alpha;
        this->has_unassociated_alpha = has_unassociated_alpha;
        this->colour_space_confidence = colour_space_confidence;
        this->colour_space = colour_space;
      }
    int get_colour_info(bool &has_premultiplied_alpha,
                        bool &has_unassociated_alpha,
                        int  &colour_space_confidence,
                        jp2_colour_space &colour_space)
      { /* [SYNOPSIS]
             This function may be used to retrieve any knowledge which might
             be available concerning the colour space and/or presence of
             alpha components.
           [RETURNS]
             0 if nothing is known about the colour or alpha properties of
             the image component samples.  Otherwise, the returned value
             represents the number of colour channels.
           [ARG: has_premultiplied_alpha]
             Used to return an indication of whether or not there is a
             pre-multiplied alpha channel.  See `set_colour_info' for an
             explanation.
           [ARG: has_unassociated_alpha]
             Used to return an indication of whether or not there is an
             unassociated alpha channel.  See `set_colour_info' for an
             explanation.
           [ARG: colour_space_confidence]
             Used to return the colour space confidence.  See `set_colour_info'
             for an explanation.
           [ARG: colour_space]
             The value returned here is meaningful only if the
             `colour_space_confidence' value is greater than 0.
        */
        has_premultiplied_alpha = this->has_premultiplied_alpha;
        has_unassociated_alpha = this->has_unassociated_alpha;
        colour_space_confidence = this->colour_space_confidence;
        if (num_colours == 0)
          { has_premultiplied_alpha = has_unassociated_alpha = false;
            colour_space_confidence = 0; }
        colour_space = this->colour_space;
        return num_colours;
      }
    jp2_output_box *add_source_metadata(kdu_uint32 box_type)
      { /* [SYNOPSIS]
             When reading the header of a rich image file format, such as
             TIFF, additional meta-data may be discovered which could be
             passed to a JP2/JPX compressed image.  This may be done
             by calling this function to create a new open `jp2_output_box'
             into which the reader can write box contents.
             [//]
             Be sure not to invoke `jp2_output_box::close', since that will
             destroy the box's contents.
        */
        jp_box_list *elt = new jp_box_list;  elt->next = NULL;
        if (extra_box_tail == NULL)
          extra_box_head = extra_box_tail = elt;
        else
          extra_box_tail = extra_box_tail->next = elt;
        elt->box.open(box_type);
        return &(elt->box);
      }
    jp2_output_box *get_source_metadata(int idx)
      { /* [SYNOPSIS]
             Use this function to retrieve any extra meta-data boxes which
             have been added by the file format reader.  These boxes may
             be copied into a JP2/JPX file which is under construction, to
             record the original file's meta-data.
           [RETURNS]
             NULL if the number of boxes added via `add_source_metadata'
             is less than or equal to `idx'.
        */
        jp_box_list *scan=extra_box_head;
        for (; (idx > 0) && (scan != NULL); idx--, scan=scan->next);
        return ((scan==NULL)?NULL:(&scan->box));
      }
    void set_meta_manager(jpx_meta_manager meta_manager)
      { this->meta_manager = meta_manager; }
      /* [SYNOPSIS]
           You may use this function when writing a file which is decompressed
           from a JP2/JPX file, to supply the `jpx_source' object's
           meta-manager.  This allows the file-format writing engine to
           search for additional meta-data which might be used to
           construct more meaningful headers.  This is most useful when
           writing to rich image file formats such as TIFF.
      */
    jpx_meta_manager get_meta_manager() { return meta_manager; }
      /* [SYNOPSIS]
           This function may be used when constructing an image file header
           which is capable of storing additional meta-data.  If the
           returned interface exists (see `jpx_meta_manager::exists'), it
           has been supplied by the decompression application via
           `set_meta_manager' to allow searching for extra meta-data in an
           original JP2/JPX source.
      */
    void append_component()
      {
      /* [SYNOPSIS]
           This function adds a new component, replicating its dimensions
           and precision information, based on the last available
           component -- if there is none, all parameters for the new
           component are set to 0.  The parameters may later be overwritten
           using the `add_component' function.
      */
        int i;
        if (num_components == max_components)
          {
            max_components += 10;
            int *tdata=new int[max_components*7];
            for (i=0; i < 7*num_components; i++)
              tdata[i] = data[i];
            if (data != NULL) delete[] data;
            data = tdata;
          }
        if (num_components == 0)
          for (i=0; i < 7; i++)
            data[i] = 0;
        else
          for (i=0; i < 7; i++)
            data[num_components*7+i] = data[num_components*7+i-7];
        num_components++;
      }
    void add_component(int height, int width, int bit_depth, bool is_signed,
                       int comp_idx=-1)
      {
      /* [SYNOPSIS]
           If `comp_idx' is < 0, this function appends a new component to
           the evolving description.  Otherwise, the function appends as
           many components as required to ensure that the component with
           index `comp_idx' exists, and then writes or overwrites its
           contents with the supplied values.  Where new components must
           be created, the `append_component' function is used.  That
           function replicates the properties of any previous components
           for which information is available.
      */
        assert(bit_depth > 0);
        if (comp_idx < 0)
          comp_idx = num_components;
        while (num_components <= comp_idx)
          append_component();
        data[comp_idx*7+0] = height;  data[comp_idx*7+1] = width;
        data[comp_idx*7+2] = (is_signed)?(-bit_depth):bit_depth;
      }
    void set_cropping(int y_off, int x_off, int height, int width,
                      int comp_idx)
      {
      /* [SYNOPSIS]
           This function is useful only when reading input files.  If the
           file reader encounters a non-trivial cropping specification, it
           can configure itself to read only the relevant portion of the
           image and to set the dimensions of each image component to reflect
           such cropping.
           [//]
           As with `add_component', this function adds sufficient components
           to the description to make sure that the component with index
           `comp_idx' is available, replicating the dimensions and precision
           information, if any, of those compponents as it does so.
      */
        if (comp_idx < 0)
          comp_idx = num_components;
        while (num_components <= comp_idx)
          append_component();
        data[comp_idx*7+3] = y_off;  data[comp_idx*7+4] = x_off;
        data[comp_idx*7+5] = height; data[comp_idx*7+6] = width;
      }
    void set_bit_depth(int comp_idx, int bit_depth)
      {
      /* [SYNOPSIS]
           An image file reading module can use this function to change the
           bit-depth of an image component which has already been added, using
           this function.  This is mainly provided to simplify the
           implementation of precision forcing, as described in connection
           with the `get_forced_precision' function.  The signed/unsigned
           attributes are unaffected by this function.  The function expects
           the component to exist already.
      */
        if ((comp_idx < 0) || (comp_idx >= num_components) || (bit_depth < 1))
          { assert(0); return; }
        if (get_signed(comp_idx))
          data[7*comp_idx+2] = -bit_depth;
        else
          data[7*comp_idx+2] = bit_depth;
      }
    int get_num_components() { return num_components; }
      /* [SYNOPSIS]
           Returns the total number of image components for which
           descriptions are available.
      */
    int get_height(int comp_idx)
      { assert((comp_idx >= 0) && (comp_idx < num_components));
        return data[7*comp_idx+0]; }
      /* [SYNOPSIS]
           Retrieves the height of the indicated image component (component
           indices start from 0).
      */
    int get_width(int comp_idx)
      { assert((comp_idx >= 0) && (comp_idx < num_components));
        return data[7*comp_idx+1]; }
      /* [SYNOPSIS]
           Retrieves the width of the indicated image component (component
           indices start from 0).
      */
    bool get_cropping(int &y_off, int &x_off, int &height, int &width,
                      int comp_idx)
      {
      /* [SYNOPSIS]
           Use this function when implementing a file reader, to determine
           whether or not cropping has been requested.  If cropping is
           supported, the implementation should use the cropping parameters
           to modify the dimensions supplied to `add_component'.  If this
           cannot be done for any reason, the application should be able
           to deduce that cropping has not been applied, because the
           dimensions of the cropped region will differ from those of the
           image and so it can generate an informative error.
      */
        if (comp_idx >= num_components)
          comp_idx = num_components-1;
        if ((comp_idx<0) || (data[7*comp_idx+5]<1) || (data[7*comp_idx+6]<1))
          return false;
        y_off = data[7*comp_idx+3];  x_off = data[7*comp_idx+4];
        height = data[7*comp_idx+5]; width = data[7*comp_idx+6];
        return true;
      }
    int get_bit_depth(int comp_idx)
      { assert((comp_idx >= 0) && (comp_idx < num_components));
        return abs(data[7*comp_idx+2]); }
      /* [SYNOPSIS]
           Retrieves the bit-depth (precision) of the indicated image
           component (component indices start from 0).
      */
    bool get_signed(int comp_idx)
      { assert((comp_idx >= 0) && (comp_idx < num_components));
        return (data[7*comp_idx+2] < 0); }
      /* [SYNOPSIS]
           Returns true if the indicated image component holds signed
           information, with a nominal range from -2^{B-1} to +2^{B-1}-1,
           where B is the bit-depth returned by `get_bit_depth'.  Otherwise,
           the function returns false and the nominal data range is from 0
           to 2^B.
      */
    void set_forced_precision(int comp_idx, int precision,
                              bool align_lsbs=true)
      { int c;
        assert(comp_idx >= 0);
        if (precision < 0)
          precision = 0;
        if (comp_idx >= max_forced_precisions)
          { max_forced_precisions += 1 + max_forced_precisions;
            int *precs = new int[max_forced_precisions];
            for (c=0; c < num_forced_precisions; c++)
              precs[c] = forced_precisions[c];
            if (forced_precisions != NULL) delete[] forced_precisions;
            forced_precisions = precs;
          }
        for (c=num_forced_precisions; c <= comp_idx; c++)
          forced_precisions[c] = forced_precisions[c-1];
        num_forced_precisions = c;
        forced_precisions[comp_idx] = (align_lsbs)?precision:-precision;
      }
      /* [SYNOPSIS]
           Use this function to set information about component precisions
           you want to force an image file reader or writer to use.  For
           more information about the interpretation of forced precisions,
           see the `get_forced_precision' function.
      */
    int get_forced_precision(int comp_idx, bool &align_lsbs)
      {
        if ((comp_idx < 0) || (num_forced_precisions == 0))
          return 0;
        if (comp_idx >= num_forced_precisions)
          comp_idx = num_forced_precisions-1;
        if (forced_precisions[comp_idx] < 0)
          { align_lsbs = false; return -forced_precisions[comp_idx]; }
        else
          { align_lsbs = true; return forced_precisions[comp_idx]; }
      }
      /* [SYNOPSIS]
           This function returns 0 if no forced precision information is
           available (i.e., `set_forced_precision' has not been called).
           Otherwise, it returns the forced precision associated with the
           indicated component, replicating the last component for which
           a forced precision has been set, if required.  Note that a value
           of 0 means do nothing; such a value may have been explicitly set
           in a call to `set_forced_precision'.
           [//]
           If an image file reader supports precision forcing, it should
           interpret a non-zero value returned by this function as the
           bit-depth of the sample values which should be supplied in
           response to a call to `kdu_image_in::get'.  When the image
           reader is initialized, it may use the value returned by
           `get_bit_depth' if required, to determine the precision of the
           data samples recorded in the file (this is only needed for raw
           files).  However, it should explicitly change this value
           (via `set_bit_depth') to the forced precision.  The `align_lsbs'
           value identifies two possible precision forcing modes as follows:
           [>>] If `align_lsbs' is true, the sample values read from the
                image file are forced to fit into the identified precision
                without any scaling.  This means that a value of 1 in the
                original file will remain 1 after precision forcing.  Values
                which exceed the range which can be represented by the
                forced precision should be clipped -- as opposed to just
                ignoring the most significant bits.
           [>>] If `align_lsbs' is false, the sample values read from the
                image file are scaled by a power of 2, such that the
                most significant bit in the original representation corresponds
                to the most significant bit in the representation produced
                by precision forcing.  If this involves the synthesis of
                additional LSB's, they should be set to 0; if it involves the
                elimination of original LSB's, rounding to the nearest
                representable value should be employed.
           [//]
           If an image file writer supports precision forcing, it should
           interpret a non-zero value returned by this function as the
           bit-depth of the samples to be recorded in the output file.
           In this case, the value returned by `get_bit_depth' represents
           the actual precision of the data supplied via the
           `kdu_image_out::put' function.  Again, the behaviour of the file
           writer depends upon the `align_lsbs' value, as follows:
           [>>] If `align_lsbs' is true, the sample values written to the
                output file are not scaled, meaning that a value of 1 (with
                respect to the `get_bit_depth' precision) should be written
                as a 1 (with respect to the forced precision used in the
                output file).  If the `get_bit_depth' value is larger than
                the forced precision, some values may need to be clipped to
                the range which can be accommodated by the `forced_precision'.
           [>>] If `align_lsbs' is false, the sample values written to the
                output file are scaled, so that the most significant bit of
                the representation used to supply data via
                `kdu_image_out::put' is aligned with the most significant bit
                of the forced precision used to actually write the data.
                Such scaling is always by a power of 2; rounding to the
                nearest representable value should be employed if LSB's are
                discarded during the shifting process.
      */
  private: // Structures -- required only to exchange metadata with compressor
      struct jp_box_list {
          jp2_output_box box;
          jp_box_list *next;
        };
  private: // Private data
    int num_components;
    int max_components;
    int *data;
    int num_forced_precisions;
    int max_forced_precisions;
    int *forced_precisions;
    double ref_width, ref_height; // Reference dimensions for resolution info
    double xpels_per_metre, ypels_per_metre;
    bool resolution_aspect_known, resolution_units_known;
    int num_colours; // 0 if not known
    bool has_premultiplied_alpha, has_unassociated_alpha;
    int colour_space_confidence; // 0 if not known
    jp2_colour_space colour_space;
    jp_box_list *extra_box_head, *extra_box_tail;
    jpx_meta_manager meta_manager;
  };
  /* Notes:
        The `data' array holds 7 integers for each component.  The first
     integer is the height; the second is the width; the third has a
     magnitude which is equal to the bit-depth and a sign which is
     negative for signed data and positive for unsigned data.  The last four
     integers are cropping specifications (y_off, x_off, height, width),
     which will be zero unless cropping is required.
        The `forced_precisions' array holds `num_forced_precisions' entries,
     recording any information supplied by the user concerning the forcing
     of component precisions.  The entries in this array are extended by
     replication, as required.
        The `ref_width', `ref_height', `xpels_per_metre', `ypels_per_metre',
     `resolution_aspect_known' and `resolution_units_known' members are used
     to record global resolution information, if available.  The `ref_width'
     and `ref_height' members identify the dimensions of the full resolution
     image, with respect to which the resolution is specified.  If
     `resolution_aspect_known' is true, both `xpels_per_metre' and
     `ypels_per_metre' must be strictly positive and their ratio conveys
     the aspect ratio of the physical display pixels.  If
     `resolution_units_known' is true, both `xpels_per_metre' and
     `ypels_per_metre' must be strictly positive and their values
     convey the actual display pixel resolution associated with the
     image whose dimensions are `ref_width' and `ref_height'.  Obviously, if
     `resolution_units_known' is true, `resolution_aspect_known' must also
     be true, but the converse need not hold.  If both flags are false, the
     values of `xpels_per_metre' and `ypels_per_metre' should both be
     equal to 0.0.
        The remaining parameters do not strictly relate to image dimensions,
     but they represent a convenient way to communicate any additional
     known attributes between image file I/O functions and the compressed
     file format manager.  They are particularly useful for communicating
     with richer file formats such as TIFF. */


/*****************************************************************************/
/*                             kdu_rgb8_palette                              */
/*****************************************************************************/

struct kdu_rgb8_palette {
    /* [SYNOPSIS]
       This simple structure may be used to convey information about an
       RGB colour palette, indexed by up to 8 bit component sample values.
       If the index has more than 8 bits, it is probably better to compress
       the de-palettized colour samples rather than the palette index.
    */
  public: // Member functions
    kdu_rgb8_palette()
      { input_bits = output_bits = source_component = 0; }
    bool exists() { return (input_bits > 0) && (output_bits > 0); }
      /* [SYNOPSIS]
           Returns true if both the `input_bits' and the `output_bits'
           member variables have been set to non-zero values.
      */
    bool operator!() { return !exists(); }
      /* [SYNOPSIS] Opposite of `exists'. */
    bool is_monochrome()
      { for (int n=0; n < (1<<input_bits); n++)
          if ((red[n] != green[n]) || (red[n] != blue[n]))
            return false;
        return true; }
      /* [SYNOPSIS]
           Returns true if the red, green and blue channels are identical
           in every palette entry.
      */
    void rearrange(kdu_byte *map);
      /* [SYNOPSIS]
           Searches for a permutation of the palette entries which will
           optimize neighbourhood properties. Specifically, tries to minimize
           the sum of the distances between adjacent palette entries where
           distance is assessed as the sum of the absolute values of the
           colour component differences.
           [//]
           The task is nothing other than the well-known travelling salesman
           problem, which is known to be NP-complete.  Consequently, the
           solution will generally be sub-optimal.  The algorithm deliberately
           starts with the original palette order and tries to incrementally
           improve upon it, so that in the event that the palette already has
           good neighbourhood properties (e.g., a previously optimized
           palette, a "heat" map or something like that), the function will
           execute quickly and without damaging the original palette's
           neighbourhood properties.
           [//]
           The permutation is written into the supplied `map' array, which is
           understood as a lookup table whose inputs are the original palette
           indices and whose outputs are the new palette indices.
      */
  public: // Data
    int input_bits;
      /* [SYNOPSIS]
           Precision of the indices mapped by this palette.  Must be
           no larger than 8.
      */
    int output_bits;
      /* [SYNOPSIS]
           Identifies the bit-depth of the palette colour values in the `red',
           `green' and `blue' arrays.  May not exceed 31.
      */
    int source_component;
      /* [SYNOPSIS]
           Should be set to the index (starting from 0) of the
           JPEG2000 image component which holds palette indices.
      */
    kdu_int32 red[256];
      /* [SYNOPSIS]
           The first 2^{`input_bits'} entries in this array hold valid
           palette entries for the red colour channel.  These entries are
           all expected to be unsigned quantities; the reason for selecting
           a signed integer data type to hold these unsigned quantities
           is only to simplify interaction with the `jp2_palette' object.
      */
    kdu_int32 green[256];
      /* [SYNOPSIS] See the description of the `red' array. */
    kdu_int32 blue[256];
      /* [SYNOPSIS] See the description of the `red' array. */
  };

/*****************************************************************************/
/*                              kdu_image_in                                 */
/*****************************************************************************/

class kdu_image_in_base {
  /* Pure virtual base class. Provides an interface to derived classes which
     each support reading of a specific file type. */
  public: // Single interface function.
    virtual ~kdu_image_in_base() {}
    virtual bool get(int comp_idx, kdu_line_buf &line, int x_tnum) = 0;
  };

class kdu_image_in {
  /* [SYNOPSIS]
     Support for reading a wide range of image file formats is NOT a
     significant objective for the Kakadu tools.  Although the
     present object is easily extended to work with any number of different
     file formats, we realize that most application developers will prefer
     to use their own (probably much more sophisticated) tools for
     interacting with image files -- some applications might not need to
     work with uncompressed image files at all.
     [//]
     This class provides a uniform interface to any number of image readers,
     supporting different image file formats.  Specific file format readers
     are selected and created by the constructor and destroyed using the
     explicit `destroy' function.  The `kdu_image_in' object itself contains
     only a pointer to the internal reader object.  Copying a `kdu_image_in'
     object has no impact on the internal image reader, but simply serves to
     provide another interface to it.
     [//]
     An individual image file may provide one or more image components.
     When multiple components are available, each component may be accessed
     independently, reading lines one by one.  The object manages any
     buffering required to cater for applications which read the components
     at different rates, although this is not recommended.  It is also
     possible to access image component lines in disjoint segments, known
     as tile-lines.  This capability is provided to assist tile-based
     compression configurations.
  */
  public: // Member functions
    kdu_image_in() { in = NULL; }
      /* [SYNOPSIS]
           Constructs an empty interface -- one whose `exists' function
           returns false.  To open an image file, you must either use
           the second form of the constructor or else assign another,
           non-empty, `kdu_image_in' interface to the current one.
      */
    kdu_image_in(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
                 bool &vflip, kdu_rgb8_palette *palette=NULL,
                 kdu_long skip_bytes=0, bool quiet=false);
      /* [SYNOPSIS]
           Attempts to open a file with the indicated name, generating an
           error message (through `kdu_error') if unsuccessful.  It
           automatically creates and installs an appropriate internal
           file reading object, based on the file suffix or some other
           property (e.g., a magic number found at the start of the file).
           The method used to ascertain the file format and the set of
           file formats which are supported is entirely dependent
           on the implementation.
           [//]
           Component indices for the image components represented by the
           file start from the value supplied via the `next_comp_idx'
           argument.  These component indices are used to access
           dimensional and other parameters for the `dims' object.  Upon
           return, the `next_comp_idx' is updated to include the
           image components contributed by this file.
           [//]
           The `dims' object provides the central mechanism for exchanging
           information about component dimensions.  For raw files, the
           component dimension and precision information is recovered from
           this object in order to discover the structure of the file, using
           `dims.append_components' as required to replicate the information
           available for previous components.  If insufficient information
           can be retrieved in this fashion (i.e., this only happens if
           `dims' was empty on entry), the function generates an appropriate
           error.
           [//]
           For non-raw files, information in the image file header is used
           to configure the relevant component dimensions and precision
           information in the `dims' object.
         [ARG: fname]
           Relative path name of the image file to be opened.
         [ARG: dims]
           Object through which image component dimensions, bit-depth and
           signed/unsigned characteristics are exchanged with the
           application, along with any display resolution information
           which might be available.
         [ARG: next_comp_idx]
           Identifies the component index assigned to the first image
           component (starting from 0) in the file.  Upon return, the
           value of this argument is augmented by the number of image
           components available from the file.  Once all image files have
           been opened, the value of `next_comp_idx' (assuming it was
           initially 0) will hold the total number of image components.
         [ARG: vflip]
           This argument is provided to support bottom-up image file
           formats, such as the well-known BMP format. The value of `vflip'
           is set to true if the image lines are going to be supplied in
           bottom-up fashion, allowing the compressor to configure itself
           for processing the data in reverse order (see
           `kdu_codestream::change_appearance' for more on this).
         [ARG: palette]
           May be non-NULL if the JPEG2000 compressed file format supports
           signalling of colour palette information (JP2 files provide such
           support).  BMP and TIFF files, for example, may contain palettized
           data.  In this case, the `palette' object's entries are filled out
           to reflect the index of the image component to which the palette is
           applied, along with the palette itself.  Palettes which require
           more than 8-bit indices should be expanded directly by the file
           reader.  If the `palette.input_bits' is set to a non-zero
           value already, the palette is in use and a new palette cannot be
           created for the present image file.  In this case, or if the
           `palette' argument is NULL, the image sample values are to be
           depalettized by the image reader, which will normally provide
           a full set of colour channels.
         [ARG: skip_bytes]
           Number of initial bytes from the file to be ignored.
         [ARG: quiet]
           Suppresses reporting of special image features which might
           be encountered.
      */
    void destroy() { assert(in != NULL); delete in; in = NULL; }
      /* [SYNOPSIS]
           Instances of the `kdu_image_in' class are only interfaces to the
           underlying image reading object.  As such, they may be copied at
           will.  To avoid the underlying object being destroyed when an
           interface to it goes out of scope, we provide an explicit
           destruction function.  This is simpler and more obvious, if
           slightly more dangerous than reference counting.
           [//]
           Generates a warning message if one or more image lines were not
           consumed from any component.
      */
    bool exists() { return (in==NULL)?false:true; }
      /* [SYNOPSIS]
           Returns false if the interface is empty -- this happens after
           using the default (no arguments) constructor or after a call to
           `destroy'.
      */
    bool operator!() { return (in==NULL)?true:false; }
      /* [SYNOPSIS]
           Opposite of `exists', returning true if the interface is empty.
      */
    bool get(int comp_idx, kdu_line_buf &line, int x_tnum)
      { return in->get(comp_idx,line,x_tnum); }
      /* [SYNOPSIS]
           Retrieves a new tile-line from the image component identified by
           `comp_idx', or returns false if the input file is exhausted.
           [//]
           `x_tnum' must hold the horizontal tile index (starting from 0).
           This is used to determine how much of each line has already been
           read and so control internal line buffering.  We leave line
           buffering to the image reading object itself, since it knows the
           smallest amount of memory which can actually be buffered; however,
           to simplify line buffering, we insist on the following rules which
           should not prove burdensome:
           [>>] For each line, tiles should be read from left to right;
           [>>] The number of tiles read from earlier lines should be at
                least as large as the number of tiles read from later
                lines; and
           [>>] When multiple components are managed by the same object,
                all components should be read for a given tile within a given
                line before moving to the next tile of that line and the
                components should be read in order.
           [>>] It can happen that some tiles have no samples whatsoever.
                For safety, these tiles should still be read from the `get'
                function.
           [//]
           Sample values in the `line' buffer are always signed quantities.
           For normalized (floating or fixed point) sample values, the nominal
           range is from -0.5 to 0.5, while for absolute integer values, the
           range is from -2^{B-1} to 2^{B-1}, where B is the bit-depth.  The
           function takes care of renormalizing, and conversion between signed
           and unsigned data types.
         [ARG: comp_idx]
           Must lie within the range of valid component indices for
           this object.  This range runs consecutively from the value of
           the `next_comp_idx' argument passed to the constructor,
           `kdu_image_in::kdu_image_in'.
         [ARG: line]
           The supplied object must have already been created.  The
           `line.get_buf16' or `line.get_buf32' function is
           used to access the line's internal buffer and fill it with
           samples recovered from the image.  The actual number of samples
           associated with the tile-line being requested is given by
           the `line.get_width' function.
         [ARG: x_tnum]
           Horizontal tile index associated with the current tile line.  Tile
           indices start from 0 -- see above.
      */
  private: // Data
    class kdu_image_in_base *in;
  };

/*****************************************************************************/
/*                             kdu_image_out                                 */
/*****************************************************************************/

class kdu_image_out_base {
  /* Pure virtual base class. Provides an interface to derived classes which
     each support writing of a specific file type. */
  public: // Single interface function.
    virtual ~kdu_image_out_base() {}
    virtual void put(int comp_idx, kdu_line_buf &line, int x_tnum) = 0;
  };

class kdu_image_out {
  /* [SYNOPSIS]
     Support for writing a wide range of image file formats is NOT a
     significant objective for the Kakadu tools.  Although the
     present object is easily extended to work with any number of different
     file formats, we realize that most application developers will prefer
     to use their own (probably much more sophisticated) tools for
     interacting with image files -- some applications might not need to
     work with uncompressed image files at all.
     [//]
     This class provides a uniform interface to any number of image writers,
     supporting different image file formats.  Specific file format writers
     are selected and created by the constructor (based on the file name
     suffix) and destroyed using the explicit `destroy' function.  The
     `kdu_image_out' object itself contains only a pointer to the internal
     writer object.  Copying a `kdu_image_out' object has no impact on the
     internal image writer, but simply serves to provide another interface
     to it.
     [//]
     An individual image file may store one or more image components.
     When multiple components are stored, each component may be accessed
     independently, writing lines one by one.  The object manages any
     buffering required to cater for applications which write the components
     at different rates, although this is not recommended.  It is also
     possible to write image component lines in disjoint segments, known
     as tile-lines.  This capability is provided to assist in efficient
     decompression of tiled compressed imagery.
  */
  public: // Member functions
    kdu_image_out() { out = NULL; }
      /* [SYNOPSIS]
           Constructs an empty interface -- one whose `exists' function
           returns false.  To open an image file, you must either use
           the second form of the constructor or else assign another,
           non-empty, `kdu_image_out' interface to the current one.
      */
    kdu_image_out(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
                  bool &vflip, bool quiet=false);
      /* [SYNOPSIS]
           Attempts to open a file with the indicated name, generating an
           error message (through `kdu_error') if unsuccessful.  It
           automatically creates and installs an appropriate internal
           file writing object, based on the file suffix.
           [//]
           Component indices for the image components represented by the
           file start from the value supplied via the `next_comp_idx'
           argument.  These component indices are used to recover dimensions
           and other properties from `dims'.
           [//]
           The dimensions of the image components, along with their
           bit-depths and signed/unsigned characteristics are all deduced
           from the `dims' argument.  This object may also be used to
           recover resolution information which may potentially be
           available.  Note the difference between the methods used to pass
           dimensions to/from the `kdu_image_in' object and the
           `kdu_image_out' object.  The former needs to take advantage of
           the rich capabilities offered by the `siz_params' class to
           extrapolated limited information which may be present concerning
           dimensions (e.g., for raw files), while the present object is not
           constructed until all dimensional information is already available
           from a JPEG2000 code-stream.
           [//]
           Also, when compressing image data, all original image components
           must be available from files, while decompressors may choose to
           decompress only a limited set of image components, or may need to
           map components through expansive palettes, introducing fundamental
           differences between the information represented by the code-stream
           SIZ marker segment and the dimensions of generated image files.
         [ARG: fname]
           Relative path name of the image file to be opened.
         [ARG: dims]
           If this object contains insufficient information, a terminal
           error is generated through `kdu_error'.
           be recorded (if they can be determined).
         [ARG: next_comp_idx]
           Identifies the component index assigned to the first image
           component (starting from 0) in the file.  Upon return, the
           value of this argument is augmented by the number of image
           components available from the file.
         [ARG: vflip]
           This argument is provided to support bottom-up image file
           formats, such as the well-known BMP format. The value of `vflip'
           is set to true if the image lines must be supplied in
           bottom-up fashion.  To see how a decompressor can do this
           efficiently, see `kdu_codestream::change_appearance'.
         [ARG: quiet]
           Suppresses reporting of special image features which might
           be encountered.
      */
    void destroy()
      { assert(out != NULL); delete out; out = NULL; }
      /* [SYNOPSIS]
           Instances of the `kdu_image_out' class are only interfaces to the
           underlying image writing object.  As such, they may be copied at
           will.  To avoid the underlying object being destroyed when an
           interface to it goes out of scope, we provide an explicit
           destruction function.  This is simpler and more obvious, if
           slightly more dangerous than reference counting.
           [//]
           Generates a warning message if one or more image lines were not
           written to any component.
      */
    bool exists() { return (out==NULL)?false:true; }
      /* [SYNOPSIS]
           Returns false if the interface is empty -- this happens after
           using the default (no arguments) constructor or after a call to
           `destroy'.
      */
    bool operator!() { return (out==NULL)?true:false; }
      /* [SYNOPSIS]
           Opposite of `exists', returning true if the interface is empty.
      */
    void put(int comp_idx, kdu_line_buf &line, int x_tnum)
      { out->put(comp_idx,line,x_tnum); }
      /* [SYNOPSIS]
           Writes a new tile-line to the image component identified by
           `comp_idx', generating an error through `kdu_error' if the
           component is exhausted.
           [//]
           `x_tnum' must hold the horizontal tile index (starting from 0).
           This is used to determine how much of each line has already been
           written and so control internal line buffering.  We leave line
           buffering to the image writing object itself, since it knows the
           smallest amount of memory which can actually be buffered; however,
           to simplify line buffering, we insist on the following rules which
           should not prove burdensome:
           [>>] For each line, tiles should be written from left to right;
           [>>] The number of tiles written to earlier lines should be at
                least as large as the number of tiles written to later
                lines;
           [>>] When multiple components are managed by the same object,
                all components should be written for a given tile within a
                given line before moving to the next tile of that line and
                the components should be written in order.
           [>>] It can happen that some tiles have no samples whatsoever.
                For safety, these tiles should still be delivered to the `put'
                function.
           [//]
           Sample values in the `line' buffer are always signed quantities.
           For normalized (floating or fixed point) sample values, the nominal
           range is from -0.5 to 0.5, while for absolute integer values, the
           range is from -2^{B-1} to 2^{B-1}, where B is the bit-depth.  The
           function takes care of renormalizing, truncating, rounding and
           conversion between signed and unsigned data types.
         [ARG: comp_idx]
           Must lie within the range of valid component indices for
           this object.  This range runs consecutively from the value of
           the `next_comp_idx' argument passed to the constructor,
           `kdu_image_out::kdu_image_out'.
         [ARG: line]
           The `line.get_buf16' or `line.get_buf32' function is
           used to access the line's internal buffer with the samples to
           be written.  The actual number of samples associated with the
           tile-line being delivered is recovered using the `line.get_width'
           function.
         [ARG: x_tnum]
           Horizontal tile index associated with the current tile line.  Tile
           indices start from 0 -- see above.
      */
  private: // Data
    class kdu_image_out_base *out;
  };

#endif // KDU_IMAGE
