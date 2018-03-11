/*****************************************************************************/
// File: image_local.h [scope = APPS/IMAGE-IO]
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
   Local class definitions used by the implementation in "image_in.cpp" and
"image_out.cpp".  These should not be included from any other scope.
******************************************************************************/

#ifndef IMAGE_LOCAL_H
#define IMAGE_LOCAL_H

#include <stdio.h> // C I/O functions can be quite a bit faster than C++ ones
#include "kdu_elementary.h"
#include "kdu_image.h"
#include "kdu_file_io.h"
#include "kdu_tiff.h" // Required only for writing TIFF files

// Defined here.

struct image_line_buf;
class pgm_in;
class pgm_out;
class ppm_in;
class ppm_out;
class raw_in;
class raw_out;
struct bmp_header;
class bmp_in;
class bmp_out;
class tif_in;

#define KDU_IPTC_TAG_MARKER ((kdu_byte) 0x1c)

/*****************************************************************************/
/*                            image_line_buf                                 */
/*****************************************************************************/

struct image_line_buf {
  public: // Member functions
    image_line_buf(int width, int sample_bytes, int min_bytes=0)
      {
        this->width = width;
        this->sample_bytes = sample_bytes;
        this->buf_bytes = width*sample_bytes;
        if ((min_bytes > 0) && (buf_bytes < min_bytes)) buf_bytes = min_bytes;
        this->buf = new kdu_byte[buf_bytes];
        next = NULL;
        accessed_samples = 0;
        next_x_tnum = 0;
      }
    ~image_line_buf()
      { delete[] buf; }
  public:  // Data
    kdu_byte *buf;
    int buf_bytes;
    int sample_bytes;
    int width;
    int accessed_samples;
    int next_x_tnum;
    image_line_buf *next;
  };
  /* Notes:
     This structure provides a convenient mechanism for buffering
     tile-component lines as they are generated or consumed.  The `width'
     field indicates the number of samples in the line, while `sample_bytes'
     indicates the number of bytes used to represent each sample (the most
     significant byte always comes first).  The `accessed_samples' field
     indicates the number of samples which have already been read from or
     written into the line.  The `next_x_tnum' field holds the value of the
     `x_tnum' argument which should be expected in the next `get' or `put'
     call relevant to this line.  The `next' field may be used to build a
     linked list of buffered image lines. */

/*****************************************************************************/
/*                             class pbm_in                                  */
/*****************************************************************************/

class pbm_in : public kdu_image_in_base {
  public: // Member functions
    pbm_in(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
           kdu_rgb8_palette *palette, kdu_long skip_bytes);
    ~pbm_in();
    bool get(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Data
    int comp_idx;
    int rows, cols;
    image_line_buf *incomplete_lines; // Points to first incomplete line.
    image_line_buf *free_lines; // List of line buffers not currently in use.
    int num_unread_rows;
    FILE *in;
    bool forced_align_lsbs; // Relevant only if `forced_prec' is non-zero
    int forced_prec; // Non-zero if the precision has been forced
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
  };

/*****************************************************************************/
/*                             class pgm_in                                  */
/*****************************************************************************/

class pgm_in : public kdu_image_in_base {
  public: // Member functions
    pgm_in(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
           kdu_long skip_bytes);
    ~pgm_in();
    bool get(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Data
    int comp_idx;
    int rows, cols;
    image_line_buf *incomplete_lines; // Points to first incomplete line.
    image_line_buf *free_lines; // List of line buffers not currently in use.
    int num_unread_rows;
    int forced_prec; // Non-zero if the precision has been forced
    bool forced_align_lsbs; // Irrelevant unless `forced_prec' is non-zero
    FILE *in;
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
  };

/*****************************************************************************/
/*                             class pgm_out                                 */
/*****************************************************************************/

class pgm_out : public kdu_image_out_base {
  public: // Member functions
    pgm_out(const char *fname, kdu_image_dims &dims, int &next_comp_idx);
    ~pgm_out();
    void put(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Data
    int comp_idx;
    int rows, cols;
    int precision;
    int orig_precision; // Same as above unless `precision' is being forced
    bool forced_align_lsbs; // Irrelevant if `precision' == `orig_precision'
    bool orig_signed; // True if original representation was signed
    image_line_buf *incomplete_lines; // Points to first incomplete line.
    image_line_buf *free_lines; // List of line buffers not currently in use.
    int num_unwritten_rows;
    FILE *out;
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
  };

/*****************************************************************************/
/*                             class ppm_in                                  */
/*****************************************************************************/

class ppm_in : public kdu_image_in_base {
  public: // Member functions
    ppm_in(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
           kdu_long skip_bytes);
    ~ppm_in();
    bool get(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Data
    int first_comp_idx;
    int rows, cols;
    image_line_buf *incomplete_lines; // Each "sample" consists of 3 bytes.
    image_line_buf *free_lines;
    int num_unread_rows;
    FILE *in;
    int forced_prec[3]; // Non-zero if the precision has been forced
    bool forced_align_lsbs[3]; // Irrelevant unless `forced_prec' val non-zero
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
  };

/*****************************************************************************/
/*                             class ppm_out                                 */
/*****************************************************************************/

class ppm_out : public kdu_image_out_base {
  public: // Member functions
    ppm_out(const char *fname, kdu_image_dims &dims, int &next_comp_idx);
    ~ppm_out();
    void put(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Data
    int first_comp_idx;
    int rows, cols;
    int precision[3];
    int orig_precision[3]; // Same as above unless `precision' is being forced
    bool forced_align_lsbs[3]; // Irrelevant if `precision'=`orig_precision'
    bool orig_signed; // True if original representation was signed
    image_line_buf *incomplete_lines; // Each "sample" consists of 3 bytes.
    image_line_buf *free_lines;
    int num_unwritten_rows;
    FILE *out;
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
  };

/*****************************************************************************/
/*                             class raw_in                                  */
/*****************************************************************************/

class raw_in : public kdu_image_in_base {
  public: // Member functions
    raw_in(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
           kdu_long skip_bytes, bool littlendian);
    ~raw_in();
    bool get(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Data
    int comp_idx;
    int rows, cols;
    int precision;
    int sample_bytes;
    bool is_signed;
    image_line_buf *incomplete_lines;
    image_line_buf *free_lines;
    int num_unread_rows;
    FILE *in;
    int forced_prec; // Non-zero if precision is being forced
    bool forced_align_lsbs; // Irrelevant unless `forced_prec' is non-zero
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
    bool littlendian;
  };

/*****************************************************************************/
/*                             class raw_out                                 */
/*****************************************************************************/

class raw_out : public kdu_image_out_base {
  public: // Member functions
    raw_out(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
            bool littlendian);
    ~raw_out();
    void put(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Data
    int comp_idx;
    int rows, cols;
    int precision;
    int orig_precision; // Equals above unless `precision' is being forced
    int sample_bytes;
    bool forced_align_lsbs; // Irrelevant if `precision' = `orig_precision'
    bool is_signed;
    image_line_buf *incomplete_lines;
    image_line_buf *free_lines;
    int num_unwritten_rows;
    FILE *out;
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
    bool littlendian;
  };

/*****************************************************************************/
/*                              bmp_header                                   */
/*****************************************************************************/

struct bmp_header {
    kdu_uint32 size; // Size of this structure: must be 40
    kdu_int32 width; // Image width
    kdu_int32 height; // Image height; -ve means top to bottom.
    kdu_uint32 planes_bits; // Planes in 16 LSB's (must be 1); bits in 16 MSB's
    kdu_uint32 compression; // Only accept 0 here (uncompressed RGB data)
    kdu_uint32 image_size; // Can be 0
    kdu_int32 xpels_per_metre; // We ignore these
    kdu_int32 ypels_per_metre; // We ignore these
    kdu_uint32 num_colours_used; // Entries in colour table (0 = use default)
    kdu_uint32 num_colours_important; // 0 means all colours are important.
  };
  /* Notes:
        This header structure must be preceded by a 14 byte field, whose
     first 2 bytes contain the string, "BM", and whose next 4 bytes contain
     the length of the entire file.  The next 4 bytes must be 0. The final
     4 bytes provides an offset from the start of the file to the first byte
     of image sample data.
        If the bit_count is 1, 4 or 8, the structure must be followed by
     a colour lookup table, with 4 bytes per entry, the first 3 of which
     identify the blue, green and red intensities, respectively. */

/*****************************************************************************/
/*                             class bmp_in                                  */
/*****************************************************************************/

class bmp_in : public kdu_image_in_base {
  public: // Member functions
    bmp_in(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
           bool &vflip, kdu_rgb8_palette *palette, kdu_long skip_bytes);
    ~bmp_in();
    bool get(int comp_idx, kdu_line_buf &line, int x_tnum);
  private:
    void map_palette_index_bytes(kdu_byte *buf, bool absolute);
    void map_palette_index_nibbles(kdu_byte *buf, bool absolute);
    void map_palette_index_bits(kdu_byte *buf, bool absolute);
  private: // Data
    int first_comp_idx;
    int num_components;
    bool bytes, nibbles, bits; // Only when reading palette data
    bool expand_palette; // True if palette is to be applied while reading.
    kdu_byte map[1024];
    int precision; // Precision of samples after any palette mapping
    int rows, cols;
    int line_bytes; // Number of bytes in a single line of the BMP file.
    image_line_buf *incomplete_lines; // Each "sample" represents a full pixel
    image_line_buf *free_lines;
    int num_unread_rows;
    FILE *in;
    int forced_prec[4];
    bool forced_align_lsbs[4];
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
  };
  /* Notes:
        `num_components' is either 1, 3 or 4.  It holds the number of image
     components produced by the reader.  If  `bytes', `nibbles' or `bits'
     is true, the file contains palette indices, which must be unpacked
     (except when the indices are bytes) and mapped.
        If `expand_palette' is true, the palette indices must be mapped
     directly to RGB values and the `map' array holds interleaved palette
     information.  Each group of four bytes corresponds to a single palette
     index: the first byte is the blue colour value, the second is green, the
     third is red.  In this case, `num_components' may be 3 or 1.  If 1,
     the palette is monochrome and only one component will be expanded.
        If `expand_palette' is false, the number of components must be 1.  In
     this case, the palette indices themselves are to be compressed as a
     single image component; however, they must first be subjected to a
     permutation mapping, which rearranges the palette in a manner more
     amenable to compression.  In this case, the palette indices are applied
     directly to the `map' lookup table, which returns the mapped index in
     its low order 1, 4 or 8 bits, as appropriate. */

/*****************************************************************************/
/*                             class bmp_out                                 */
/*****************************************************************************/

class bmp_out : public kdu_image_out_base {
  public: // Member functions
    bmp_out(const char *fname, kdu_image_dims &dims, int &next_comp_idx);
    ~bmp_out();
    void put(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Data
    int first_comp_idx;
    int num_components;
    int rows, cols;
    int alignment_bytes; // Number of 0's at end of each line.
    int precision[3];
    int orig_precision[3]; // Same as above unless `precision' is being forced
    bool forced_align_lsbs[3]; // Ignored if `precision' == `orig_precision'
    bool orig_signed; // True if original representation was signed
    image_line_buf *incomplete_lines; // Each "sample" represents a full pixel
    image_line_buf *free_lines;
    int num_unwritten_rows;
    FILE *out;
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
  };

/*****************************************************************************/
/*                             class tif_out                                 */
/*****************************************************************************/

class tif_out : public kdu_image_out_base {
  public: // Member functions
    tif_out(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
            bool quiet);
    ~tif_out();
    void put(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Helper functions
    void perform_buffer_pack(kdu_byte *buf);
  private: // Data
    int first_comp_idx;
    int num_components;
    int rows, cols;
    int precision;
    bool forced_align_lsbs;
    int *orig_precision; // All equal to above unless `precision' is forced.
    bool *is_signed; // One entry for each component
    int scanline_width, unpacked_samples;
    int sample_bytes, pixel_bytes, row_bytes;
    bool pre_pack_littlendian; // Scanline byte order prior to packing
    image_line_buf *incomplete_lines; // Each "sample" represents a full pixel
    image_line_buf *free_lines;
    int num_unwritten_rows;
    kdu_simple_file_target out;
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
  };

#ifdef KDU_INCLUDE_TIFF
#  include "tiffio.h"
#endif // KDU_INCLUDE_TIFF

/*****************************************************************************/
/*                             class tif_in                                  */
/*****************************************************************************/

class tif_in : public kdu_image_in_base {
  public: // Member functions
    tif_in(const char *fname, kdu_image_dims &dims, int &next_comp_idx,
           kdu_rgb8_palette *palette, kdu_long skip_bytes, bool quiet);
    ~tif_in();
    bool get(int comp_idx, kdu_line_buf &line, int x_tnum);
  private: // Helper functions
    void perform_buffer_unpack(kdu_byte *src, kdu_byte *dst, int x_tile_idx);
      /* Works on a single chunk line.  `x_tile_idx' is the absolute index
         of the horizontal tile (starting from 0 for the first tile in the
         original image) from which the data was sourced.  This affects the
         number of samples and bytes which are processed. */
    void perform_sample_remap(kdu_byte *buf, int x_tile_idx);
      /* Works on a single chunk line, affecting only the first component.
         Again, `x_tile_idx' is the absolute index of the horizontal tile
         (starting from 0 for the first tile in the original image) from
         which the data was sourced. */
    void perform_palette_expand(kdu_byte *buf);
      /* Works on an entire scan-line */
  //--------------------------------------------------------------------------
  private: // Members describing the organization of the TIFF data
    bool *is_signed; // One entry for each image component
    double *float_minvals; // When TIFF file contains floating-point samples
    double *float_maxvals; // When TIFF file contains floating-point samples
    kdu_uint16 bitspersample;
    kdu_uint16 samplesperpixel;
    int chunkline_samples; // Number of unpacked samples in a row of a "chunk"
    int chunkline_bytes; // Number of raw bytes in a single row of a "chunk"
    int chunks_across; // Number of chunks across the original image width
    bool need_buffer_unpack; // True if `bitspersample' is not 8, 16 or 32.
    bool expand_palette; // True if palette is to be applied while reading.
    bool remap_samples; // True if palette is to be re-arranged.
    kdu_uint32 tile_width; // Image width if not tiled
    kdu_uint32 tile_height; // Strip height if not tiled
    int tiles_across; // 1 if not tiled
    int tiles_down; // Number of vertical strips if not tiled
    kdu_long *chunk_offsets; // NULL if `libtiff_in' used -- see below.
    int chunks_per_component; // Always equal to `tiles_across'*`tiles_down'
    kdu_simple_file_source src;
    int rows_at_a_time; // Number of rows to read at a time
    bool horizontally_tiled_tiff; // If horizontally tiled
#ifdef KDU_INCLUDE_TIFF
    TIFF *libtiff_in;
    kdu_byte *chunk_buf; // Used to buffer entire chunks, where necessary
#endif // KDU_INCLUDE_TIFF
  //--------------------------------------------------------------------------
  private: // Members which are affected by (or support) cropping
    int skip_rows; // Number of initial rows to skip from original image
    int skip_cols; // Number of initial cols to skip from original image
    int skip_tiles_across; // Same as `skip_cols'/`tile_width'
    int first_tile_skip_cols; // Columns to skip in the left-most tile
    int first_tile_width; // Number of columns used from left-most tile
    int last_tile_width; // Number of columns used from right-most tile
    int used_tiles_across; // Number of horizontal tiles we actually need
    int first_chunkline_skip_samples;
    int first_chunkline_samples; // For left-most tile
    int last_chunkline_samples;  // For right-most tile
    int first_chunkline_skip_bytes; // Bytes to skip over in left-most tile
    int first_chunkline_bytes; // For left-most tile
    int last_chunkline_bytes;  // For right-most tile
    int total_chunkline_bytes; // Total chunkline bytes in `used_tiles_across'
    kdu_long last_seek_addr; // So we can easily seek over unneeded data
  //--------------------------------------------------------------------------
  private: // Members which describe the image samples after they have been
           // extracted from the TIFF file and transformed, as required.
    int first_comp_idx;
    int num_components; // May be > `samplesperpixel' if there is a palette
    int precision;
    int *forced_prec; // One entry per component; non-zero if precision forced
    bool *forced_align_lsbs; // Ignored unless `forced_prec' val is non-zero
    int sample_bytes; // After expanding any palette
    int pixel_gap; // Gap between pixels (in bytes) after expanding any palette
    bool planar_organization; // See below
    bool invert_first_component; // If min value corresponds to white samples.
    kdu_byte map[1024]; // Used when expanding a palette
    int rows, cols;  // Dimensions after applying any cropping
    int num_unread_rows; // Always starts at `rows', even with cropping
    image_line_buf *incomplete_lines; // Each "sample" represents a full pixel
    image_line_buf *free_lines;
    int initial_non_empty_tiles; // tnum >= this implies empty; 0 until we know
    bool post_unpack_littlendian; // Byte order of data in `image_line_buf's
  };
  /* Notes:
        For TIFF files which use a contiguous (interleaved) organization, the
     meaning of a "chunk" is a single tile (or vertical strip, for non-tiled
     files).  For TIFF files which use a planar organization, the meaning of a
     "chunk" is a single image component (plane) of a single tile (or strip).
     The `chunk_offsets' array contains the locations in the TIFF source file
     at which each chunk is located.  Where a planar organization is used,
     the locations of all tiles (or strips) for component 0 appear first,
     followed by the locations of all chunks associated with component 1
     and so forth.
        The `planar_organization' field identifies the organization of each
     `image_line_buf' entry in the list headed by `incomplete_lines'.  If
     `planar_organization' is false, each `image_line_buf' line contains
     interleaved samples from all the components.  Otherwise, each
     `image_line_buf' line contains all samples from component 0,
     followed by all samples from component 1, and so forth. */

#endif // IMAGE_LOCAL_H
