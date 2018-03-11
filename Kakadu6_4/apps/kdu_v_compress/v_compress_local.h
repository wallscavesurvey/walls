/*****************************************************************************/
// File: v_compress_local.h [scope = APPS/V_COMPRESSOR]
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
   Local class declarations for "kdu_v_compress.cpp".
******************************************************************************/

#ifndef V_COMPRESS_LOCAL_H
#define V_COMPRESS_LOCAL_H

/*****************************************************************************/
/*                               kdv_compressor                              */
/*****************************************************************************/

class kdv_compressor {
  public: // Member functions
    kdv_compressor(kdu_codestream codestream, int sample_bytes,
                   bool native_order, bool is_signed,
                   int num_layer_specs, kdu_thread_env *env=NULL);
      /* The compressor object's structure and parameters are derived from
         the supplied `codestream' object; however, that object need not
         ever be used to actually generate compressed data.  The only
         requirement is that the `codestream' objects passed into the
         `process_frame' function all have the same structure.
            If non-NULL, the `env' argument specifies a multi-threading
         environment to be used for multi-threaded processing, in place of
         the default single-processing implementation.  This is useful for
         accelerating the throughput on machines with multiple physical
         processors. */
    ~kdv_compressor()
      {
        if (buffer != NULL)
          delete[] buffer;
        if (components != NULL)
          delete[] components;
        if (layer_bytes != NULL)
          delete[] layer_bytes;
        if (layer_thresholds != NULL)
          delete[] layer_thresholds;
      }
    bool load_frame(FILE *fp);
      /* Returns true if a new frame can be loaded.  Generates a warning
         message if only part of a frame remains in the file. */
    kdu_uint16 process_frame(kdu_compressed_video_target *video,
                             kdu_codestream codestream,
                             kdu_long layer_bytes[],
                             kdu_uint16 layer_thresholds[],
                             bool trim_to_rate, bool no_info);
      /* Processes the video data currently loaded into the buffer,
         compressing it into a new field within the supplied `video' object.
         Upon entry, the `video' object must already have an open image
         (i.e., `video->open_image' must be called outside this function).
         Similarly, the caller is responsible for closing the open image
         once this function has returned.  The `codestream' object must
         already have been created for output by the caller; its `flush'
         function is called from within the present function, immediately
         before returning.
            Note that the size of the `layer_bytes' and `layer_thresholds'
         arrays must match the `num_layer_specs' argument supplied to the
         constructor.  The contents of these arrays are copied internally,
         but the original values are not modified by the present function.
            The function returns the distortion-length slope threshold
         selected by the rate control algorithm for the final quality layer.
         This value may be used to provide a bound for the generation of
         coding passes in the next video frame, thereby saving computation in
         the CPU-intensive block encoding phase.
            The `trim_to_rate' argument is passed through to
         `kdu_codestream::flush'.  If true, that function will perform more
         careful rate allocation, at the expense of some computation. */
  private: // Declarations

    struct kdv_component {
      public: // Component-wide data
        kdu_dims comp_dims; // Total size of this component
        kdu_byte *comp_buf; // Points to 1st sample of this component in buffer
        int count_val; // Vertical sub-sampling factor for component
      public: // Tile-component specific data managed here
        kdu_dims tile_dims; // Size of current tile-component
        kdu_byte *bp; // Points to next row in current tile
        int counter; // Counts down from `count_val' to 0. Process line when 0
        int precision; // Number of MSBs used from each VIX sample word
        kdu_push_ifc engine; // Sample processing compression engine
        kdu_line_buf line; // Manages a single line buffer
        kdu_thread_queue *env_queue; // Used only for multi-threaded processing
      };

  private: // Data
    kdu_thread_env *env; // Used only for multi-threaded processing
    int sample_bytes; // 1, 2 or 4
    bool native_order; // If false, VIX sample words must be byte reversed
    bool is_signed; // If true, VIX samples have a signed representation
    int frame_bytes; // Total bytes in the frame, as stored in the VIX file
    kdu_byte *buffer; // Single buffer for the entire frame
    kdu_sample_allocator allocator; // Manages all sample processing memory
    int num_components;
    kdv_component *components;
    int num_layer_specs;
    kdu_long *layer_bytes;
    kdu_uint16 *layer_thresholds;
  };

#endif // V_COMPRESS_LOCAL_H
