/******************************************************************************/
// File: stripe_compressor_local.h [scope = APPS/SUPPORT]
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
   Local definitions used in the implementation of the
`kdu_stripe_compressor' object.
*******************************************************************************/

#ifndef STRIPE_COMPRESSOR_LOCAL_H
#define STRIPE_COMPRESSOR_LOCAL_H

#include "kdu_stripe_compressor.h"

// Objects declared here:
struct kdsc_component_state;
struct kdsc_component;
struct kdsc_tile;
class kdsc_flush_worker;

/******************************************************************************/
/*                            kdsc_component_state                            */
/******************************************************************************/

struct kdsc_component_state {
  public: // Member functions
    void update(kdu_coords next_tile_idx, kdu_codestream codestream,
                bool all_done);
      /* Called immediately after processing stripe data for a row of tiles.
         Adjusts the values of `remaining_tile_height' and `stripe_height'
         accordingly, while also updating the `buf8', `buf16', `buf32' or
         `buf_float' pointers to address the start of the next row of tiles.
         If the tile height is reduced to 0, and `all_done' is false, the
         function also uses the code-stream interface and `next_tile_idx'
         argument to figure out the vertical height of the next row of tiles,
         installing this value in the `remaining_tile_height' member. */
  public: // Data
    int comp_idx;
    int pos_x; // x-coord of left-most sample in the component
    int width; // Full width of the component
    int original_precision; // Precision recorded in SIZ marker segment
    kdu_byte *buf8; // NULL, or pointer to first row in current left-most tile
    kdu_int16 *buf16; // Same as `buf8', but for 16-bit buffers
    kdu_int32 *buf32; // Same as `buf8', but for 32-bit buffers
    float *buf_float; // Same as `buf8', but for floating-point buffers
    int row_gap, sample_gap, precision; // Values supplied by `push_stripe'
    bool is_signed; // Value supplied by `push_stripe' (always false for `buf8')
    int stripe_height; // Remaining height in the current stripe
    int remaining_tile_height; // See below
    int max_tile_height;
    int max_recommended_stripe_height;
  };
  /* Notes:
       `stripe_height' holds the total number of rows in the current stripe
     which have not yet been fully processed.  This value is updated at the
     end of each row of tiles, by subtracting the smaller of `stripe_height'
     and `remaining_tile_height'.
       `remaining_tile_height' holds the number of rows in the current
     row of tiles, which have not yet been fully processed.  This value
     is updated at the end of each row of tiles, by subtracting the smaller
     of `stripe_height' and `remaining_tile_height'.
       `max_tile_height' is the maximum height of any tile in the image.
     In practice, this is the maximum of the heights of the first and
     second vertical tiles, if there are multiple tiles.
       `max_recommended_stripe_height' remembers the value returned by the
     first call to `kdu_stripe_compressor::get_recommended_stripe_heights'. */

/******************************************************************************/
/*                              kdsc_component                                */
/******************************************************************************/

struct kdsc_component {
    // Manages processing of a single tile-component.
  public: // Data configured by `kdsc_tile::init' for a new tile
    kdu_coords size; // Tile width, by tile rows left in tile
    int horizontal_offset; // From left edge of image component
    int ratio_counter; // See below
  public: // Data configured by `kdsc_tile::init' for a new or existing tile
    int stripe_rows_left; // Counts down from `stripe_rows' during processing
    int sample_gap; // For current stripe being processed by `push_stripe'
    int row_gap; // For current stripe being processed by `push_stripe'
    int precision; // For current stripe being processed by `push_stripe'
    bool is_signed; // For current stripe being processed by `push_stripe'
    kdu_byte *buf8; // See below
    kdu_int16 *buf16; // See below
    kdu_int32 *buf32; // See below
    float *buf_float; // See below
  public: // Data configured by `kdu_stripe_compressor::get_new_tile'
    int original_precision; // Original sample precision
    int vert_subsampling; // Vertical sub-sampling factor of this component
    int count_delta; // See below
  };
  /* Notes:
       The `stripe_rows_left' member holds the number of rows in the current
     stripe, which are still to be processed by this tile-component.
       Only one of the `buf8', `buf16', `buf32' or `buf_float' members will
     be non-NULL, depending on whether the stripe data is available as bytes
     or words.  The relevant member points to the start of the first
     unprocessed row of the current tile-component.
       The `ratio_counter' is initialized to 0 at the start of each stripe
     and decremented by `count_delta' each time we consider processing this
     component within the tile.  Once the counter becomes negative, a new row
     from the stripe is processed and the ratio counter is subsequently
     incremented by `vert_subsampling'.  The value of the `count_delta'
     member is the minimum of the vertical sub-sampling factors for all
     components.  This policy ensures that we process the components in a
     proportional way. */

/******************************************************************************/
/*                                kdsc_tile                                   */
/******************************************************************************/

struct kdsc_tile {
  public: // Member functions
    kdsc_tile() { components=NULL; next=NULL; tile_queue=NULL; }
    ~kdsc_tile()
      {
        if (components != NULL) delete[] components;
        if (engine.exists()) engine.destroy();
      }
    void init(kdu_coords idx, kdu_codestream codestream,
              kdsc_component_state *comp_states, bool force_precise,
              bool want_fastest, kdu_thread_env *env,
              kdu_thread_queue *env_queue, int env_dbuf_height);
      /* Initializes a new or partially completed tile, with a new set of
         stripe buffers, and associated parameters. If the `tile' interface
         is empty, a new code-stream tile is opened, and the various members
         of this structure and its constituent components are initialized.
         Otherwise, the tile already exists and we are supplying additional
         stripe samples.
            The pointers supplied by the `buf8' or `buf16' member of each
         element in the `comp_states' array, whichever is non-NULL,
         correspond to the first sample in what remains of the current
         stripe, within each component.  This first sample is aligned at the
         left edge of the image.  The function determines the amount by which
         the buffer should be advanced to find the first sample in the current
         tile, by comparing the horizontal location of each tile-component,
         as returned by `kdu_tile_comp::get_dims', with the values in the
         `pos_x' members of the `kdsc_component_state' entries in the
         `comp_states' array. */
    bool process(kdu_thread_env *env);
      /* Processes all the stripe rows available to tile-components in the
         current tile, returning true if the tile is completed and false
         otherwise.  Before returning true, this function destroys the various
         tile-component processing engines and line buffers and restarts the
         `allocator' object.  Always call this function right after `init'. */
  public: // Data
    kdu_tile tile;
    kdu_multi_analysis engine;
    kdsc_tile *next; // Next free tile, or next partially completed tile.
    kdu_thread_queue *tile_queue;
  public: // Data configured by `kdu_stripe_compressor::get_new_tile'.
    int num_components;
    kdsc_component *components; // Array of tile-components
  };

/*****************************************************************************/
/*                             kdsc_flush_worker                             */
/*****************************************************************************/

class kdsc_flush_worker : public kdu_worker {
  public: // Member functions
    kdsc_flush_worker() { codestream=kdu_codestream();  ready_failed=false; }
    void init(kdu_codestream codestream, kdu_long *layer_bytes,
              int num_layer_specs, kdu_uint16 *layer_thresholds=NULL,
              bool trim_to_rate=true, bool record_in_comseg=true,
              double tolerance=0.0)
      { // Simply initializes the values of the parameters which
        // will be passed to `kdu_codestream::flush' each time it is called
        // as a registered multi-threaded job.
        this->codestream = codestream;
        this->layer_bytes = layer_bytes;
        this->num_layer_specs = num_layer_specs;
        this->layer_thresholds = layer_thresholds;
        this->trim_to_rate = trim_to_rate;
        this->record_in_comseg = record_in_comseg;
        this->tolerance = tolerance;
        this->ready_failed=false;
      }
    bool is_free() { return !codestream; }
      /* False if `init' has been called, but the flush job has not yet
         been invoked. */
    bool ready_for_flush_failed()
      { bool result = ready_failed;  ready_failed=false;  return result; }
      /* True if the last call to `do_job' found that the call to
         `codestream.ready_for_flush()' failed.  This condition is cleared
         by this function and also by `init'. */
    virtual void do_job(kdu_thread_entity *ent, int job_idx)
      {
        kdu_thread_env *env = (kdu_thread_env *) ent;
        if (!codestream) return;
        if (codestream.ready_for_flush(env))
          codestream.flush(layer_bytes,num_layer_specs,layer_thresholds,trim_to_rate,
                           record_in_comseg,tolerance,env);
        else
          ready_failed = true;
        codestream = kdu_codestream(); // Mark as "free" for another `init'
      }
  private: // Data
    kdu_codestream codestream;
    kdu_long *layer_bytes;
    int num_layer_specs;
    kdu_uint16 *layer_thresholds;
    bool trim_to_rate;
    bool record_in_comseg;
    double tolerance;
    bool ready_failed;
  };

#endif // STRIPE_COMPRESSOR_LOCAL_H
