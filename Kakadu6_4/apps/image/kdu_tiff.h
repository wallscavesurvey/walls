/*****************************************************************************/
// File: KDU_TIFF.h [scope = APPS/IMAGE-IO]
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
   Defines class `kdu_tiffdir' which can be used to parse, expand, generate
and write TIFF directory information.
******************************************************************************/

#ifndef KDU_TIFF_H
#define KDU_TIFF_H
#include "kdu_compressed.h"
#include "kdu_messaging.h"

// Classes defined here
struct kd_tifftag;
class kdu_tiffdir;

/* ========================================================================= */
/*                      Some Common TIFF tag_type Codes                      */
/* ========================================================================= */

#define KDU_TIFFTAG_Artist             ((kdu_uint32) 0x013B0002)
#define KDU_TIFFTAG_Copyright          ((kdu_uint32) 0x82980002)
#define KDU_TIFFTAG_HostComputer       ((kdu_uint32) 0x013C0002)
#define KDU_TIFFTAG_ImageDescription   ((kdu_uint32) 0x010E0002)
#define KDU_TIFFTAG_Make               ((kdu_uint32) 0x010F0002)
#define KDU_TIFFTAG_Model              ((kdu_uint32) 0x01100002)
#define KDU_TIFFTAG_Software           ((kdu_uint32) 0x01310002)


#define KDU_TIFFTAG_BitsPerSample      ((kdu_uint32) 0x01020003)

#define KDU_TIFFTAG_ColorMap           ((kdu_uint32) 0x01400003)

#define KDU_TIFFTAG_Compression        ((kdu_uint32) 0x01030003)
#    define KDU_TIFF_Compression_NONE                  ((kdu_uint16) 1)
#    define KDU_TIFF_Compression_CCITT                 ((kdu_uint16) 2)
#    define KDU_TIFF_Compression_PACKBITS              ((kdu_uint16) 32773)

#define KDU_TIFFTAG_ExtraSamples       ((kdu_uint32) 0x01520003)
#    define KDU_TIFF_ExtraSamples_UNDEFINED            ((kdu_uint16) 0)
#    define KDU_TIFF_ExtraSamples_PREMULTIPLIED_ALPHA  ((kdu_uint16) 1)
#    define KDU_TIFF_ExtraSamples_UNASSOCIATED_ALPHA   ((kdu_uint16) 1)

#define KDU_TIFFTAG_FillOrder          ((kdu_uint32) 0x010A0003)
#    define KDU_TIFF_FillOrder_MSB_FIRST               ((kdu_uint16) 1)
#    define KDU_TIFF_FillOrder_LSB_FIRST               ((kdu_uint16) 2)

#define KDU_TIFFTAG_GrayResponseCurve  ((kdu_uint32) 0x01230003)

#define KDU_TIFFTAG_GrayResponseUnit   ((kdu_uint32) 0x01220003)

#define KDU_TIFFTAG_ImageHeight16      ((kdu_uint32) 0x01010003)
#define KDU_TIFFTAG_ImageHeight32      ((kdu_uint32) 0x01010004)

#define KDU_TIFFTAG_ImageWidth16       ((kdu_uint32) 0x01000003)
#define KDU_TIFFTAG_ImageWidth32       ((kdu_uint32) 0x01000004)

#define KDU_TIFFTAG_InkSet             ((kdu_uint32) 0x014C0003)
#    define  KDU_TIFF_InkSet_CMYK                      ((kdu_uint16) 1)
#    define  KDU_TIFF_InkSet_NotCMYK                   ((kdu_uint16) 2)

#define KDU_TIFFTAG_MaxSampleValue     ((kdu_uint32) 0x01190003)
#define KDU_TIFFTAG_MinSampleValue     ((kdu_uint32) 0x01180003)

#define KDU_TIFFTAG_NumberOfInks       ((kdu_uint32) 0x014E0003)

#define KDU_TIFFTAG_PhotometricInterp  ((kdu_uint32) 0x01060003)
#    define KDU_TIFF_PhotometricInterp_WHITEISZERO     ((kdu_uint16) 0)
#    define KDU_TIFF_PhotometricInterp_BLACKISZERO     ((kdu_uint16) 1)
#    define KDU_TIFF_PhotometricInterp_RGB             ((kdu_uint16) 2)
#    define KDU_TIFF_PhotometricInterp_PALETTE         ((kdu_uint16) 3)
#    define KDU_TIFF_PhotometricInterp_TRANSPARENCY    ((kdu_uint16) 4)
#    define KDU_TIFF_PhotometricInterp_SEPARATED       ((kdu_uint16) 5)
#    define KDU_TIFF_PhotometricInterp_YCbCr           ((kdu_uint16) 6)
#    define KDU_TIFF_PhotometricInterp_CIELab          ((kdu_uint16) 8)

#define KDU_TIFFTAG_PlanarConfig       ((kdu_uint32) 0x011C0003)
#    define KDU_TIFF_PlanarConfig_CONTIG               ((kdu_uint16) 1)
#    define KDU_TIFF_PlanarConfig_PLANAR               ((kdu_uint16) 2)

#define KDU_TIFFTAG_ResolutionUnit     ((kdu_uint32) 0x01280003)
#    define KDU_TIFF_ResolutionUnit_NONE               ((kdu_uint16) 1)
#    define KDU_TIFF_ResolutionUnit_INCH               ((kdu_uint16) 2)
#    define KDU_TIFF_ResolutionUnit_CM                 ((kdu_uint16) 3)

#define KDU_TIFFTAG_RowsPerStrip16     ((kdu_uint32) 0x01160003)
#define KDU_TIFFTAG_RowsPerStrip32     ((kdu_uint32) 0x01160004)

#define KDU_TIFFTAG_SampleFormat       ((kdu_uint32) 0x01530003)
#    define KDU_TIFF_SampleFormat_UNSIGNED             ((kdu_uint16) 1)
#    define KDU_TIFF_SampleFormat_SIGNED               ((kdu_uint16) 2)
#    define KDU_TIFF_SampleFormat_FLOAT                ((kdu_uint16) 3)
#    define KDU_TIFF_SampleFormat_UNDEFINED            ((kdu_uint16) 3)

#define KDU_TIFFTAG_SamplesPerPixel    ((kdu_uint32) 0x01150003)

#define KDU_TIFFTAG_SminSampleValueF   ((kdu_uint32) 0x0154000B)
#define KDU_TIFFTAG_SminSampleValueD   ((kdu_uint32) 0x0154000C)

#define KDU_TIFFTAG_SmaxSampleValueF   ((kdu_uint32) 0x0155000B)
#define KDU_TIFFTAG_SmaxSampleValueD   ((kdu_uint32) 0x0155000C)

#define KDU_TIFFTAG_StripByteCounts16  ((kdu_uint32) 0x01170003)
#define KDU_TIFFTAG_StripByteCounts32  ((kdu_uint32) 0x01170004)
#define KDU_TIFFTAG_StripByteCounts64  ((kdu_uint32) 0x01170010)

#define KDU_TIFFTAG_StripOffsets16     ((kdu_uint32) 0x01110003)
#define KDU_TIFFTAG_StripOffsets32     ((kdu_uint32) 0x01110004)
#define KDU_TIFFTAG_StripOffsets64     ((kdu_uint32) 0x01110010)

#define KDU_TIFFTAG_TileWidth16        ((kdu_uint32) 0x01420003)
#define KDU_TIFFTAG_TileWidth32        ((kdu_uint32) 0x01420004)

#define KDU_TIFFTAG_TileHeight16       ((kdu_uint32) 0x01430003)
#define KDU_TIFFTAG_TileHeight32       ((kdu_uint32) 0x01430004)

#define KDU_TIFFTAG_TileOffsets32      ((kdu_uint32) 0x01440004)
#define KDU_TIFFTAG_TileOffsets64      ((kdu_uint32) 0x01440010)

#define KDU_TIFFTAG_XResolution        ((kdu_uint32) 0x011A0005)

#define KDU_TIFFTAG_YResolution        ((kdu_uint32) 0x011B0005)


/*****************************************************************************/
/*                                kd_tifftag                                 */
/*****************************************************************************/

struct kd_tifftag {
  private: // Member functions
     friend class kdu_tiffdir;
     kd_tifftag()
       {
         tag_type=bytes_per_field=0;
         num_fields=num_bytes=read_ptr=max_bytes=0;
         location=0; out_buf=NULL; next=NULL;
       }
     ~kd_tifftag()
       { if (out_buf != NULL) delete[] out_buf; }
  private: // Data
     kdu_uint32 tag_type;
     kdu_uint32 bytes_per_field; // Property of the `tag_type'
     kdu_long num_fields;
     kdu_long num_bytes;
     union {
         kdu_long location; // If `num_bytes' > 4 (8 for bigtiff)
         kdu_byte data[8]; // If `num_bytes' <= 4 (8 for bigtiff)
       };
     kdu_long read_ptr; // Byte offset into next byte to be read
     kdu_long max_bytes; // Max entries in `out_buf'
     kdu_byte *out_buf; // Only for locally created tags
     kd_tifftag *next; // For building a simple linked list
  };

/*****************************************************************************/
/*                               kdu_tiffdir                                 */
/*****************************************************************************/

class kdu_tiffdir {
  /* [BIND: reference]
     [SYNOPSIS]
       This object provides a simple means to interact with TIFF (Tagged Image
       File Format).  The object allows reading, writing and editing of TIFF
       files.
       [//]
       The object ultimately reads from the generic `kdu_compressed_source'
       interface and writes to the generic `kdu_compressed_target'
       interface.  This allows for simple file I/O, memory-based I/O, or
       I/O based on JP2 boxes, for example.  In particular, this provides a
       simple means for reading/writing TIFF files which are themselves
       embedded in JP2 boxes.  This can be useful for a variety of purposes.
       [//]
       To open an existing TIFF file, use the `opendir' function.  You
       may subsequently read tags from the open TIFF directory, but you
       may also create new tags, delete tags or overwrite existing tags.
       [//]
       To create a new TIFF file, use the `init' function.  You may
       subsequently create tags (optionally deleting tags you have created).
       [//]
       Regardless of whether you started with a call to `opendir' or
       `init', you may always write the current state of the TIFF
       directory structure using the `writedir' function.  To write a
       complete TIFF file, you will need to use `write_header' and `writedir',
       but you will also need to write the actual image data.  You may
       choose to write the TIFF directory structure either before or
       after the image data, but you will need to be sure to pass the
       correct `dir_offset' to `writedir' and `write_header'.  You are
       responsible for writing the image data yourself, and you will
       need to include tags (i.e., `KDU_TIFFTAG_StripByteCounts' and
       `KDU_TIFFTAG_StripOffsets' tags) to identify the corresponding strip
       offset/length values -- this means that you will need to know a little
       bit (but not too much) about the TIFF file format yourself.  It would
       be quite easy to extend this class to make these operations transparent
       to the application, but only at the cost of some flexibility in the
       way the file gets generated.
       [//]
       TIFF tags are identified here via 32-bit integers which record both
       the 16-bit tag (most significant 16 bits) and the 16-bit type code
       (least significant 16 bits).  Together, we refer to these quantities
       as a `tag_type'.  The following type codes are recognized:
       [>>] 1 = `kdu_byte'
       [>>] 2 = "ascii" -- char string (null terminator included in length)
       [>>] 3 = `kdu_uint16'
       [>>] 4 = `kdu_uint32'
       [>>] 5 = "rational" -- 2 x `kdu_uint32'; numerator -> denominator
       [>>] 6 = "signed-char"
       [>>] 7 = "undefined" -- opaque array of bytes
       [>>] 8 = `kdu_int16'
       [>>] 9 = `kdu_int32'
       [>>] 10 = "rational" -- 2 x `kdu_int32'; numerator -> denominator
       [>>] 11 = float
       [>>] 12 = double
       [>>] 16 = 64-bit unsigned (represented with `kdu_long')
       [>>] 17 = 64-bit signed (represented with `kdu_long')
       [//]
  */
    public: // Member functions
      kdu_tiffdir()
        {
          is_open=littlendian=bigtiff=false; tags=NULL;
          int endian_test=1;
          native_littlendian = (((kdu_byte *) &endian_test)[0] != 0);
          src = NULL; src_read_ptr = 0;
        }
      ~kdu_tiffdir() { close(); }
      bool exists() { return is_open; }
        /* [SYNOPSIS]
             Returns true if `init' or `opendir' have been called.  Return
             value becomes false after `close' is called.
        */
      bool operator!() { return !exists(); }
        /* [SYNOPSIS]
             Opposite of `exists'.
        */
      KDU_AUX_EXPORT void init(bool littlendian, bool bigtiff=false);
        /* [SYNOPSIS]
             Use this function to create a new TIFF directory, discarding
             any existing contents -- the function implicitly calls `close'
             first.
           [ARG: littlendian]
             If true, the byte order for the new TIFF directory is
             little-endian (i.e., low order bytes come before
             high order bytes in multi-byte numeric quantities).  Otherwise,
             the byte order is big-endian.
             [//]
             If you are unsure what the native byte order on your machine is,
             you can call the `is_native_littlendian' member function.
             Note, however, that the `read_tag' and `write_tag' functions
             automatically perform all required byte order conversions.
           [ARG: bigtiff]
             If true, the file will be written using the BigTIFF format, which
             has a 16-byte header (instead of 8 bytes), 20-byte tags
             (instead of 12 bytes), a slightly different directory
             header/trailer, and allows 64-bit integer data types to be used
             for StripOffsets, StripByteCounts, TileOffsets and TileByteCounts.
        */
      KDU_AUX_EXPORT bool opendir(kdu_compressed_source *src);
        /* [SYNOPSIS]
             Use this function to open an existing TIFF file, reading its
             directory contents.  The function records only the directory
             entries permanently in memory.  To access TIFF tags from
             this directory, use the `open_tag' or one of the `read_tag'
             functions.  You may edit the open directory by using the
             `delete_tag', `create_tag' or `write_tag' functions.
             [//]
             The function implicitly invokes `close' first, so as to
             discard the contents of any tags you may previously have
             written.
             [//]
             The supplied `src' object need not necessarily be positioned
             at the start of its underlying stream, but it must be positioned
             at the start of the TIFF header within the stream.  The
             internal implementation first recovers the current source
             position by invoking `src->get_pos' -- this is addded as an
             offset to all offsets specified in the TIFF structure.
           [RETURNS]
             False if a valid TIFF header could not be found, or if
             `src' does not support seeking, in which case
             the object remains in the closed state (`exists' returns false).
        */
      KDU_AUX_EXPORT int
        write_header(kdu_compressed_target *tgt, kdu_long dir_offset);
        /* [SYNOPSIS]
             Use this function to write a TIFF file header, based upon
             the current byte order (as supplied to `init' or recovered
             from `opendir').  It is your responsibility to supply
             the location at which the actual TIFF directory will be
             written, expressed relative to the start of the header.
             This same value must be supplied to `writedir'.  You are
             not obliged to call the present function, if you have
             other means of knowing that the TIFF header has been
             written -- e.g., you might be addending or writing over
             an existing TIFF file.
           [RETURNS]
             Number of bytes actually written, which should be 8, unless
             attempts to write to the `tgt' object failed, or the object
             is not open (`exists' returns false).
        */
      KDU_AUX_EXPORT bool
        writedir(kdu_compressed_target *tgt, kdu_long dir_offset);
        /* [SYNOPSIS]
             Use this function to write the TIFF directory.  You may
             write the TIFF directory either or before or after the
             imagery, but writing it before the imagery implies that
             you must have been able to determine the strip offsets
             and lengths associated with the imagery, since these
             are part of the TIFF structure which gets written
             (via the `KDU_TIFFTAG_StripByteCounts' and
             `KDU_TIFFTAG_StripOffsets' tags).
             [//]
             It is important to realize that this function writes both
             the TIFF directory itself and the contents of all TIFF
             tags.  Some of these may have their contents within the
             directory; otherwise, they are written immediately after
             the directory.  The function computes all relevant offsets
             and lengths for the tag contents, inserting them into the
             directory structure.  To do so, however, it needs to know
             the location at which the directory is being written,
             relative to the start of the TIFF file, as communicated by
             the `dir_offset' argument.
           [RETURNS]
             False if attempts to write to the supplied `tgt' object
             failed, or if the object is not open (`exists' returns false).
           [ARG: dir_offset]
             Location of the base of the TIFF directory, relative to
             the start of the file.  The `tgt' object is assumed to
             be positioned at this location on entry.
        */
      bool is_littlendian() { return littlendian; }
        /* [SYNOPSIS]
             Indicates whether the currently open TIFF directory uses the
             little-endian byte order or not (big-endian).
        */
      bool is_native_littlendian() { return native_littlendian; }
        /* [SYNOPSIS]
             Indicates whether the native byte order is little-endian or
             not (big-endian).  The function always returns the same
             result, regardless of whether or not `init' has been called.
        */
      KDU_AUX_EXPORT void close();
        /* [SYNOPSIS]
             Use this function to close the internal machinery, clearing
             all contents.  This function is automatically called by
             `init' and also from the object's destructor, so you will
             rarely need to invoke it yourself.
        */
      KDU_AUX_EXPORT kdu_long get_dirlength();
        /* [SYNOPSIS]
             Returns the exact number of bytes which would be written if
             `writedir' were called now.  This includes the space occupied
             by the TIFF directory itself, as well as all TIFF tags whose
             contents must appear outside the directory.
        */
      KDU_AUX_EXPORT kdu_long get_taglength(kdu_uint32 tag_type);
        /* [SYNOPSIS]
             Returns the number of fields in the tag with the supplied
             `tag_type'.  0 is returned if the tag does not
             currently exist.  Note that the return value is essentially
             equal to the number of bytes in the tag body, divided by the
             number of bytes used to represent each field -- the latter
             quantity is determined from the data type, which is identified
             by the least significant 2 bytes of the `tag_type'.
             [//]
             If the tag in question was created using the `create_tag'
             function, the returned value is deduced from the amount of
             data which has currently been written to the tag.
           [ARG: tag_type]
             See the introductory comments to this `kdu_tiffdir' object for
             a discussion of `tag_type' codes.
        */
      KDU_AUX_EXPORT int get_fieldlength(kdu_uint32 tag_type);
        /* [SYNOPSIS]
             Returns the number of bytes per field (i.e., per tag value),
             based on the least significant 16 bits of the supplied
             `tag_type'.  Essentially, this function just translates the
             data type values described in the introductory comments to
             this `kdu_tiffdir' object.  The total number of bytes in the
             body of a TIFF tag may be found by multiplying this quantity
             by the value returned by `get_taglength'.
             [//]
             Note that the result of this function does not depend in
             any way upon the state of the object -- in particular, the
             tag does not need to exist.
           [RETURNS]
             0 if the `tag_type' has an unrecognized data type code.
             Otherwise, returns the number of bytes used for values of
             this type.
        */
      KDU_AUX_EXPORT bool delete_tag(kdu_uint32 tag_type);
        /* [SYNOPSIS]
             Delete the tag indicated by `tag_type', returning false if
             there was no such tag to delete.  This function may be applied
             to objects which were opened using either `opendir' or `init'.
           [ARG: tag_type]
             See the introductory comments to this `kdu_tiffdir' object for
             a discussion of `tag_type' codes.
        */
      KDU_AUX_EXPORT kdu_uint32 open_tag(kdu_uint32 tag_type);
        /* [SYNOPSIS]
             This function accomplishes two roles: 1) it searches for a tag
             of interest, returning 0 if it cannot be found, or else the
             tag-type code; and 2) it resets the internal "read pointer"
             to the start of the tag, so that the next call to `read_tag'
             will start again from the beginning of the tag's contents.
             In most cases, you don't actually need to call this function,
             since the `read_tag' functions automatically start reading
             from the start of the tag if it has not previously been read.
             [//]
             You may have as many tags open simultaneously as you like
             and you may read from them in any order you choose.
             [//]
             You may open and read from tags which have been locally
             created, as well as those which are derived from input files.
           [RETURNS]
             The 0 if the tag does not exist; otherwise, the actual tag-type
             code is returned.
           [ARG: tag_type]
             See the introductory comments to this `kdu_tiffdir' object for
             a discussion of `tag_type' codes.  Note, however, that in
             the case of this function (and this function only), the
             least significant 16 bits of `tag_type' may be 0.  In this
             case, the caller is asking for any tag whose 16-bit tag
             code is given by the most significant 16 bits of `tag_type'.
             This method may be used to request tags which can be used with
             multiple data types.
         */
      KDU_AUX_EXPORT kdu_long
        read_tag(kdu_uint32 tag_type, kdu_long length, kdu_byte data[]);
        /* [SYNOPSIS]
             If the tag is not already open, `open_tag' is implicitly called
             first.  The function reads from the last position read.
             This means that reads are cumulative.  To start reading again
             from the beginning, just call `open_tag' explicitly.
             [//]
             The present function may be used to read from TIFF tags of any
             type, as raw unstructured bytes.  Note, however, that this could
             leave the tag's "read pointer" at a position which is not a
             multiple of the field size (see `get_fieldlength').  If this
             happens, subsequent use of any of the other `read_tag' functions
             could generate an error (through `kdu_error').
             [//]
             You may open and read from tags which have been locally
             created, as well as those which are derived from input files.
           [RETURNS]
             Actual number of bytes read.
           [ARG: length]
             Number of bytes to read.
           [ARG: data]
             Array containing at least `length' entries, into which the
             data is written.
        */
      KDU_AUX_EXPORT kdu_long
        read_tag(kdu_uint32 tag_type, kdu_long length, kdu_uint16 data[]);
        /* [SYNOPSIS]
             Same as the first form of `read_tag', except that this function
             may only be used with tags whose data type (least significant
             two bytes of the `tag_type') is one of 3 or 8.  These are the
             data types for which each entry is 2 bytes long.  Attempts to
             read anything else will generate an error through `kdu_error'.
             [//]
             Note that this function adjusts the byte order of each field
             read in accordance with the native machine byte order, so that
             the returned 16-bit integers will be correctly interpreted.
             [//]
             To correctly recover signed 2's complement quantities (data type
             8) using this function, it is sufficient to cast the returned
             `data' values to type `kdu_int16'.
           [RETURNS]
             Actual number of 16-bit words read.
           [ARG: length]
             Number of 16-bit words to read.
           [ARG: data]
             Array containing at least `length' entries, into which the
             data is written.
        */
      KDU_AUX_EXPORT kdu_long
        read_tag(kdu_uint32 tag_type, kdu_long length, kdu_uint32 data[]);
        /* [SYNOPSIS]
           [BIND: no-bind]
             Same as the second form of `read_tag', except that this function
             may be used with tags whose data type (least significant
             two bytes of the `tag_type') is one of 3, 4, 8 or 9.  These are
             the data types for which each entry is 2 or 4 byte integer.
             Attempts to read anything else will generate an error through
             `kdu_error'.
             [//]
             If the `tag_type' utilizes only 2 byte integers, the
             representation is automatically expanded, using either signed
             or unsigned extension, depending on whether the underlying data
             type is signed.
             [//]
             To correctly recover signed 2's complement quantities (data types
             8 and 9) using this function, it is sufficient to cast the
             returned `data' values to type `kdu_int32'.
             [//]
             Use the 64-bit version of this function with foreign language
             bindings.
           [ARG: length]
             Number of 32-bit words to read.
           [ARG: data]
             Array containing at least `length' entries, into which the
             data is written.
        */
      KDU_AUX_EXPORT kdu_long
        read_tag(kdu_uint32 tag_type, kdu_long length, double data[]);
        /* [SYNOPSIS]
             Same as the second form of `read_tag', except that this function
             may be used only with tags whose data type (least significant
             two bytes of the `tag_type') is one of 5, 10, 11 or 12.  These
             are the data types for which each entry represents a floating
             point quantity, either as a signed/unsigned rational, or as
             an IEEE single/double precision float.  Attempting to read
             anything else will generate an error through `kdu_error'.
             [//]
             Note that this function correctly handles byte order and
             precision conversions.
           [RETURNS]
             Actual number of values read.
           [ARG: length]
             Number of doubles to read.
           [ARG: data]
             Array containing at least `length' entries, into which the
             data is written.
        */
      KDU_AUX_EXPORT kdu_long
        read_tag(kdu_uint32 tag_type, kdu_long length, kdu_long data[]);
        /* [SYNOPSIS]
             Same as the second form of `read_tag', except that this function
             may be used with tags whose data type (least significant
             two bytes of the `tag_type') is one of 3, 4, 8 or 9, 16 or 17.
             These are the data types for which each entry is a 2, 4 or 8 byte
             integer.  Attempts to read anything else will generate an error
             through `kdu_error'.
             [//]
             If the `tag_type' utilizes only 2 or 4 byte integers, the
             representation is automatically expanded, using either signed
             or unsigned extension, depending on whether the underlying data
             type is signed.
           [ARG: length]
             Number of 64-bit words to read.
           [ARG: data]
             Array containing at least `length' entries, into which the
             data is written.
        */
      KDU_AUX_EXPORT void create_tag(kdu_uint32 tag_type);
        /* [SYNOPSIS]
             Use this function to create a new tag with the indicated type,
             or to overwrite an existing tag of that type.  For obvious
             reasons, this function automatically resets the internal
             "read pointer," used by the `read_tag' functions, to 0.
             [//]
             Strictly speaking, this function is not necessary, since calls
             to `write_tag' automatically create the tag if required.
             [//]
             You may have as many open tags as you like, writing to them
             in any order you choose.  All writes append data to the tag.
             [//]
             It is illegal to create two tags with the same TIFF tag code
             (most significant 2 bytes of the `tag_type' argument), but
             different data types (least significant 2 bytes of the
             `tag_type' code).  Attempting to do so will generate an error
             through `kdu_error'.
           [ARG: tag_type]
             See the introductory comments to this `kdu_tiffdir' object for
             a discussion of `tag_type' codes.
        */
      KDU_AUX_EXPORT void
        write_tag(kdu_uint32 tag_type, int length, kdu_byte data[]);
        /* [SYNOPSIS]
             If the tag does not already exist, `create_tag' is implicitly
             called first.  The function writes from the last position that
             was written.  This means that writes are cumulative.  To start
             writing again from the beginning, just call `create_tag'
             explicitly.
             [//]
             The present function may be used to write to TIFF tags of any
             type, as raw unstructured bytes.  Note, however, that this could
             leave the tag's "write pointer" at a position which is not a
             multiple of the field size (see `get_fieldlength').  If this
             happens, subsequent use of any of the other `write_tag' functions
             could generate an error (through `kdu_error').
           [ARG: length]
             Number of bytes to write.
           [ARG: data]
             Array containing at least `length' entries, from which the
             data is written.
        */
      KDU_AUX_EXPORT void
        write_tag(kdu_uint32 tag_type, int length, kdu_uint16 data[]);
        /* [SYNOPSIS]
             Same as the first form of `write_tag', except that this function
             may only be used with tags whose data type (least significant
             two bytes of the `tag_type') is one of 3 or 8.  These are the
             data types for which each entry is 2 bytes long.  Attempts to
             write anything else will generate an error through `kdu_error'.
             [//]
             Note that this function adjusts the byte order of each field
             written in accordance with the native machine byte order, so that
             the returned 16-bit integers will be correctly interpreted.
             [//]
             To pass signed 2's complement quantities (data type
             8) using this function, it is sufficient to cast the `data'
             array to one of type `kdu_int16'.
           [ARG: length]
             Number of 16-bit words to write.
           [ARG: data]
             Array containing at least `length' entries, from which the
             data is written.
        */
      KDU_AUX_EXPORT void
        write_tag(kdu_uint32 tag_type, int length, kdu_uint32 data[]);
        /* [SYNOPSIS]
           [BIND: no-bind]
             Same as the second form of `write_tag', except that this function
             may be used only with tags whose data type (least significant
             two bytes of the `tag_type') is one of 4 or 9.  These are the
             data types for which each entry is a 4 byte integer.  Attempts to
             write anything else will generate an error through `kdu_error'.
             [//]
             Note that this function adjusts the byte order of each field
             written in accordance with the native machine byte order, so that
             the supplied 32-bit integers will be correctly interpreted.
             [//]
             To pass signed 2's complement quantities (data type
             9) using this function, it is sufficient to cast the `data'
             array to one of type `kdu_int32'.
             [//]
             Use the 64-bit version of this function with foreign language
             bindings.
           [ARG: length]
             Number of 32-bit words to write.
           [ARG: data]
             Array containing at least `length' entries, from which the
             data is written.
        */
      KDU_AUX_EXPORT void
        write_tag(kdu_uint32 tag_type, int length, double data[]);
        /* [SYNOPSIS]
             Same as the second form of `write_tag', except that this function
             may be used only with tags whose data type (least significant
             two bytes of the `tag_type') is one of 5, 10, 11 or 12.  These
             are the data types for which each entry represents a floating
             point quantity, either as a signed/unsigned rational, or as
             an IEEE single/double precision float.
             [//]
             Note that this function correctly handles byte order and
             precision conversions.
           [ARG: length]
             Number of double to write.
           [ARG: data]
             Array containing at least `length' entries, from which the
             data is written.
        */
      KDU_AUX_EXPORT void
        write_tag(kdu_uint32 tag_type, int length, kdu_long data[]);
        /* [SYNOPSIS]
             Same as the second form of `write_tag', except that this function
             may be use with tags whose data type (least significant
             two bytes of the `tag_type') is any of 3, 8, 4, 9, 16 or 17.
             These are the data types for which each entry is an integer
             (signed or unsigned) with 2, 4 or 8 byte precision.  Attempts to
             write anything else will generate an error through `kdu_error'.
             [//]
             When writing 2-byte integers, only the least significant 16 bits
             of each `data' value are used; similarly, when writing 4-byte
             integers, only the least significant 32 bits are used.  No
             truncation is employed so wrap-around problems may occur if
             your `data' cannot be properly represented using the data type
             associated with the `tag_type' argument.
             [//]
             Note that this function adjusts the byte order of each field
             written in accordance with the native machine byte order, so that
             the supplied 64-bit integers will be correctly interpreted.
           [ARG: length]
             Number of 64-bit words to write.
           [ARG: data]
             Array containing at least `length' entries, from which the
             data is written.
        */
      void write_tag(kdu_uint32 tag_type, kdu_uint16 val16)
        { write_tag(tag_type,1,&val16); }
        /* [SYNOPSIS]
           Convenience function to write a single 16-bit tag value.  See
           the second form of the `write_tag' function for more info. */
      void write_tag(kdu_uint32 tag_type, kdu_uint32 val32)
        { write_tag(tag_type,1,&val32); }
        /* [SYNOPSIS]
           [BIND: no-bind]
           Convenience function to write a single 32-bit tag value.  See
           the third form of the `write_tag' function for more info.  Use
           the 64-bit version with foreign language bindings. */
      void write_tag(kdu_uint32 tag_type, double valdbl)
        { write_tag(tag_type,1,&valdbl); }
        /* [SYNOPSIS]
           Convenience function to write a single double precision tag value.
           See the fourth form of the `write_tag' function for more info. */
      void write_tag(kdu_uint32 tag_type, kdu_long val64)
        { write_tag(tag_type,1,&val64); }
        /* [SYNOPSIS]
           Convenience function to write a single 64-bit tag value.
           See the fifth form of the `write_tag' function for more info.
           Note that this function may be used with tags whose data type
           (least significant two bytes of the `tag_type') is any of 3, 8,
           4, 9, 16 or 17.  It is the preferred function to use for
           single-samle writing through foreign language bindings, where
           argument data-type alone might not be sufficient to resolve
           ambiguity.
        */
  KDU_AUX_EXPORT void copy_tag(kdu_tiffdir &src, kdu_uint32 tag_type);
        /* [SYNOPSIS]
             This function provides a convenient mechanism for copying
             tags from one `kdu_tiffdir' object to another.  The function
             does nothing if the indicated tag does not exist in the `src'
             object.
        */
    private: // Helper functions
      kd_tifftag *find_tag(kdu_uint32 tag_type);
      void read_bytes(kdu_byte *buf, kdu_long num_bytes, kdu_long pos,
                      kdu_uint32 tag_type);
        /* This function is central to all tag reading operations.  It
           generates an error through `kdu_error' if insufficient bytes can
           be read.  The `tag_type' argument is supplied only to assist in
           generating meaningful error messages.  The `pos' argument identifies
           the actual location of the data to be read, within the `src'
           stream. */
    private: // Data
      bool is_open;
      bool littlendian; // If byte order for TIFF file is little-endian
      bool native_littlendian; // If native byte order is little-endian
      bool bigtiff; // Uses 8-byte offsets, 16 byte header & modified tags
      kd_tifftag *tags; // Linked list of tags
      kdu_compressed_source *src; // Used for reading tags
      kdu_long src_read_ptr; // Allows seeks to be avoided
  };

#endif // KDU_TIFF_H
