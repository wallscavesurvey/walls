/*****************************************************************************/
// File: v_expand_local.h [scope = APPS/V_DECOMPRESSOR]
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

#ifndef V_EXPAND_LOCAL_H
#define V_EXPAND_LOCAL_H

/*****************************************************************************/
/*                              kdv_decompressor                             */
/*****************************************************************************/

class kdv_decompressor {
  public: // Member functions
    kdv_decompressor(kdu_codestream codestream, int precision, bool is_signed,
                     kdu_thread_env *env=NULL, int double_buffering_height=0);
      /* The decompressor object's structure and parameters are derived from
         the supplied `codestream' object; however, the sample precision and
         signed/unsigned properties of the data to be written to the VIX file
         are supplied separately, so as to ensure that they will be consistent
         with the VIX file header which has already been written.
            If non-NULL, the `env' argument specifies a multi-threading
         environment to be used for multi-threaded processing, in place of
         the default single-processing implementation.  This is useful for
         accelerating the throughput on machines with multiple physical
         processors.
            The `double_buffering_height' argument is useful only for
         multi-threaded environments.  If non-zero, this argument is passed
         to `kdu_multi_synthesis::create' as the `processing_stripe_height'
         value, with `double_buffering' set to true.  In this case, DWT
         processing for individual tile-components can proceed in separate
         threads, which provides additional opportunities for parallelism
         over and above those afforded by processing code-blocks in different
         threads.  The option is most likely of interest only for systems
         with a significant number of physical processors (e.g., 4). */
    ~kdv_decompressor()
      {
        delete[] buffer;
        if (comp_info != NULL) delete[] comp_info;
      }
    void process_frame(kdu_codestream codestream,
                       kdu_codestream next_codestream=kdu_codestream());
      /* Decompresses the supplied codestream (must already have been created
         with a valid compressed data source as input), into an internally
         allocated buffer.
            The `next_codestream' argument is of interest only
         in multi-threading environments.  In particular, it provides a means
         to keep processing resources active even when we come to the end of
         processing a current codestream (even while we are storing
         decompressed results via the `store_frame' function).  In the current
         implementation, the initial block decoding jobs for the
         `next_codestream' are automatically activated when processing of the
         current `codestream' leaves the system under-utilized, in the sense
         defined by `kdu_thread_entity::add_queues'.  A more general
         implementation might allow for the complete background processing of
         a next codestream while data is being stored.  However, this would
         require the provision of an additional frame buffer to avoid
         overwriting the data being stored with newly decompressed data for
         the next frame.
            As with `codestream', any valid `next_codestream' must be supplied
         in a created state, with a valid compressed data source as input.
         Moreover, an error will be generated if the next call to
         `process_frame' uses a codestream other than the `next_codestream'
         supplied here. */
    void store_frame(FILE *file);
      /* Stores the most recent decompressed video image to the supplied
         VIX file. */
  private: // Declarations
    struct kdv_comp_info { // Stores fixed component-wide information
        kdu_dims comp_dims; // Dimensions and location of component
        int count_val; // Vertical sub-sampling factor for component
        kdu_byte *comp_buf; // Points into master buffer
      };
    struct kdv_tcomp {
        kdu_coords comp_size; // Obtained from `kdv_comp_info::comp_dims'
        kdu_coords tile_size; // Size of current tile-component
        kdu_byte *bp; // Points to next row in component buffer for the tile
        kdu_byte *next_tile_bp; // Points to start of next tile in codestream
        int precision; // # bits from tile-component stored in VIX sample MSB's
        int counter;
        int tile_rows_left; // Rows which have not yet been processed
      };
    struct kdv_tile {
      public: // Members
        kdv_tile() { components=NULL; env_queue=NULL; next=NULL; }
        ~kdv_tile()
          { if (components != NULL) delete[] components;
            if (ifc.exists()) ifc.close();
            if (engine.exists()) engine.destroy(); }
        bool is_active() { return engine.exists(); }
      public: // Data
        kdu_codestream codestream;
        kdu_dims valid_indices;
        kdu_coords idx; // Relative index, within `valid_indices' range
        kdu_tile ifc;
        kdu_multi_synthesis engine;
        kdv_tcomp *components; // One for each component produced by `engine'
        kdu_thread_queue *env_queue;
        kdv_tile *next; // Points to the next tile in a circular buffer
      };
  private: // Helper functions
    bool init_tile(kdv_tile *tile, kdu_codestream codestream);
      /* Initializes the `tile' object to reference the first valid tile
         of the supplied `codestream', updating or even creating all internal
         members as appropriate.  The function even creates the internal
         `comp_info' array if it does not currently exist.  The function
         returns false if it cannot open a valid tile for some reason -- e.g.,
         if `codestream' does not exist.  The `tile' object must not be
         active when this function is called, meaning that
         `kdv_tile::is_active' must return false. */
    bool init_tile(kdv_tile *tile, kdv_tile *ref_tile);
      /* This second form of the `init_tile' function initializes the object
         referenced by `tile' to use the codestream which is currently
         associated with `ref_tile', except that the next tile within
         that codestream is opened.  If there is no such tile, the function
         returns false, leaving the object in the inactive state (see
         `kdv_tile::is_active').  Note that the `ref_tile' object can be
         either active or inactive, so long as it has a valid `codestream'.
         In fact, if `ref_tile' is inactive, it can refer to the same object
         as `tile'. */
    void close_tile(kdv_tile *tile);
      /* This function does nothing unless `tile' is currently active
         (see `kdv_tile::is_active').  In that case, the function destroys
         the internal `kdv_tile::engine' object and closes any open codestream
         tile.  It leaves the `kdv_tile::codestream' and `kdv_tile::idx'
         members alone, however, so that they can be used to find and open
         the next tile in the same codestream, if one exists.
            This function is not invoked from the present object's destructor,
         since it executes the `kdu_thread::terminate' function in
         multi-threaded environments.  This is safe only if the application
         has not itself invoked `kdu_thread::terminate'. */
  private: // Data
    kdu_thread_env *env; // Used only for multi-threaded processing
    bool double_buffering; // Derived from `double_buffering_height'
    int processing_stripe_height; // Derived from `double_buffering_height'
    bool is_signed; // True if VIX file is to have signed samples
    int sample_bytes; // Bytes per VIX sample
    int frame_bytes; // Total bytes in VIX frame
    kdu_byte *buffer; // Complete frame buffer, organized by components
    int num_components;
    kdv_comp_info *comp_info; // Fixed information for each component
    kdv_tile tiles[2];
    kdv_tile *current_tile; // Points to 1 of the objects in the `tiles' array
    kdv_tile *next_tile; // Used only for multi-threaded processing (see below)
    kdu_long env_batch_idx; // Only for multi-threaded processing (see below)
  };
  /* For multi-threaded applications (i.e., when `env' is non-NULL), the
     implementation manages up to two `kdv_tile' objects.  The `current_tile'
     pointer refers to the one from which data is currently being decompressed
     into the frame buffer.  The `next_tile' pointer refers to either the next
     tile in the current frame, or the first tile in the next codestream; its
     `engine' has been started, which means that there are code-block decoding
     tasks which can be performed in the background by Kakadu's thread
     scheduler.  However, these jobs will not be performed until the system
     becomes under-utilized, in the sense described by the
     `kdu_thread_entity::add_queue' function.  This is managed through a
     `env_batch_idx' counter, which increments each time a new tile queue is
     created. */

#endif // V_EXPAND_LOCAL_H
