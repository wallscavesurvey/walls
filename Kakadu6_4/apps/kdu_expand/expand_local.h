/*****************************************************************************/
// File: expand_local.h [scope = APPS/DECOMPRESSOR]
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
   Local definitions used by "kdu_expand.cpp".  The "kde_flow_control"
object may be of interest to a variety of different applications.
******************************************************************************/

#ifndef EXPAND_LOCAL_H
#define EXPAND_LOCAL_H

// Core includes
#include "kdu_elementary.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "jp2.h"
// Application includes
#include "kdu_image.h"

// Defined here:

class kde_file_binding;
class kde_flow_control;

/*****************************************************************************/
/*                              kde_file_binding                             */
/*****************************************************************************/

class kde_file_binding {
  public: // Member functions
    kde_file_binding(char *string, int len) // `len' is length of `string'
      { fname = new char[len+1]; fname[len] = '\0';
        strncpy(fname,string,(size_t) len);
        num_channels = first_channel_idx = 0; next = NULL; }
    ~kde_file_binding() // Destroys the entire list for convenience.
      { delete[] fname;
        if (writer.exists()) writer.destroy();
        if (next != NULL) delete next; }
  public: // Data -- all data public in this local object.
    char *fname;
    int num_channels, first_channel_idx;
    kdu_image_out writer;
    kde_file_binding *next;
  };

/*****************************************************************************/
/*                             kde_flow_control                              */
/*****************************************************************************/

class kde_flow_control {
  /* Each tile-component may be decompressed independently and in any
     order; however, the sequence in which tile-component lines are
     decompressed clearly has an impact on the amount of buffering required
     for image file I/O and possibly the amount of code-stream buffering. This
     object is used to stage the tile-component processing steps so that
     the image is decompressed from top to bottom in a manner consistent
     with the overall geometry and hence, in most cases, minimum system
     buffering requirements. */
  public: // Member functions
    kde_flow_control(kde_file_binding *files, int num_channels,
                     bool last_channel_is_alpha, bool alpha_is_premultiplied,
                     int num_components, const int *component_indices,
                     kdu_codestream codestream, int x_tnum, bool allow_shorts,
                     jp2_channels channels = jp2_channels(NULL),
                     jp2_palette palette = jp2_palette(NULL),
                     bool skip_ycc=false, int dwt_stripe_height=1,
                     bool dwt_double_buffering=false,
                     kdu_thread_env *env=NULL,
                     kdu_thread_queue *env_queue=NULL);
      /* Constructs a flow control object for one tile.  The caller might
         maintain an array of pointers to such objects, one for each
         horizontally adjacent tile.
            The `files' list provides references to the `image_out' objects
         to which the decompressed image lines will be written.  The `files'
         pointer may be NULL if desired, in which case nothing will be written,
         but the decompressed image lines may still be accessed incrementally
         using the `access_decompressed_line' member function.
            The `num_channels' argument identifies the number of decompressed
         output channels to be produced.  If `channels' and `palette' are
         non-existent, there is a 1-1 correspondence between channels and
         image components.  Otherwise, there are at most 3 channels, which
         are interpreted as the colour channels, and the image components may
         be mapped to channels through a palette.
            The `num_components' and `component_indices' arguments are used
         to associate the components referenced by a `jp2_channels' object
         (if any) with those which are actually presented by the
         `kdu_codestream' interface.  These might not be identical because
         the caller has used `kdu_codestream::apply_input_restrictions' to
         restrict the set of visible components to just those which are
         actually required to render the channels.  This process may cause
         renumbering of the component indices.  To assist in recovering the
         correct associations, the restricted number of components is
         given by `num_components' and the `component_indices' array is
         the one which was passed to the second form of the
         `kdu_codestream::apply_input_restrictions' function.  If there is
         no `jp2_channels' interface, `num_channels' and `num_components'
         must be identical and the channels are in 1-1 correspondence with
         the components.  However, if there is a `jp2_channels' interface,
         `num_codestreams' should be the number of components which are
         actually used by the required channels, and the `component_indices'
         array must be scanned internally to match up the original component
         indices identified by the `jp2_channels' object.
            The `x_tnum' argument identifies which horizontal tile the object
         is being created to represent, with 0 for the first apparent
         horizontal tile.
            If `allow_shorts' is false, 32-bit data representations will be
         selected for all processing; otherwise, the constructor will select
         a 16- or a 32-bit representation on the basis of the image component
         bit-depths.  Note, however, that when decompressing an image
         component which is used to map image samples through a palette, a
         16-bit fixed point representation will be used whenever the
         irreversible processing path is involved.  This avoids unnecessary
         conversion to and from floating point representations.
            The `channels' and `palette' objects may be non-empty
         (i.e., their `exists' member functions should return true) only if a
         JP2/JPX compatible file is being decompressed and the channels are
         to be interpreted as colour channels rather than image components.
            The `skip_ycc' flag is passed straight through to the
         `kdu_multi_synthesis::create' function.  If set to true, any
         codestream RCT/ICT colour decorrelation transform will not be
         inverted.
            The `env' and `env_queue' optional arguments should be
         non-NULL only if you want to enable multi-threaded processing -- a
         feature introduced in Kakadu version 5.1.  In this case, `env'
         identifies the calling thread within the multi-threaded environment
         and `env_queue' is the queue which should be passed to
         `kdu_multi_analysis::create' -- this queue is unique for each
         horizontal tile, so it can be used to synchronize processing in the
         tile and to delete thread processing resources related to the tile
         when it is time to move to a new vertical row of tiles -- see
         `advance_tile'. */
    ~kde_flow_control();
    void collect_layer_stats(int num_layers, int num_resolutions,
                             int max_discard_levels,
                             kdu_long *resolution_layer_bytes,
                             kdu_long *resolution_layer_packets,
                             kdu_long *resolution_max_packets,
                             kdu_thread_env *env);
      /* Call this function right after construction if you want to be
         informed about the number of parsed packet bytes and the number of
         parsed packets, corresponding to each quality layer and each of
         a range of resolutions.  The function causes the supplied arrays
         to be stored internally and updated once each tile has been
         completely processed.  The arrays contents should generally be
         initialized to zero prior to calling this function, since their
         entries are simply augmented by calling the
         `kdu_tile::get_parsed_packet_stats' function at the end of each tile.
            The `resolution_layer_bytes' and `resolution_layer_packets'
         arrays are of size `num_resolutions' * `num_layers'.  The
         first `num_layers' entries are updated to reflect the compressed
         packet lengths (header + body) and number of packets, corresponding
         to all packets parsed for the lowest resolution.  This is the
         resolution associated with `max_discard_levels'.  The next
         `num_layers' entries correspond to the next higher resolution, and
         so forth.  Although it is legal to specify `num_resolutions' greater
         than `max_discard_levels'+1, the results for -ve numbers of discard
         levels will be identical to those for 0 discard levels.
            The `resolution_max_packets' array has `num_resolutions'
         entries.  It's elements accumulate the return values from the calls
         to `kdu_tile::get_parsed_packet_stats' in each successive
         resolution.
            The entries of the supplied arrays are all updated by the main
         application thread, so you can pass the same arrays to multiple
         flow control objects to accumulate all of their results. */
    bool advance_components(kdu_thread_env *env=NULL);
      /* Causes the line position for this tile to be advanced in every
         image component, by an amount which should cause at least one
         image line to be synthesized in at least one component and will not
         cause any more than one image line to be synthesized in any component.
         Synthesized image lines are colour converted, if necessary, and
         then written out to the relevant image files.   The function returns
         false once all image lines have been synthesized for the current tile,
         at which point the user will normally issue an `advance_tile' call.
            The `env' argument must be non-NULL if and only if the object
         was created for multi-threaded processing, in which case it should
         identify the calling thread. */
    kdu_line_buf *access_decompressed_line(int channel_idx);
      /* This function may be called after `advance_components' to access any
         newly decompressed image lines.  This allows the caller to modify the
         contents of the decompressed data if desired, or to send it to
         an alternate destination (other than, or in addition to an image
         file writing object).  The function returns NULL if no new data is
         available for the indicated channel (component or mapped colour
         channel). */
    void process_components();
      /* This function must be called after every call to `advance_components'
         which returns true.  It writes the decompressed image lines to the
         file writing objects supplied during construction (unless the
         constructor's `files' argument was NULL) and prepares the object to
         receive another call to its `advance_components' member function. */
    bool advance_tile(kdu_thread_env *env=NULL);
      /* Moves on to the next vertical tile at the same horizontal tile
         position, returning false if all tiles have been processed.  Note
         that the function automatically invokes the existing tile's
         `kdu_tile::close' member function.
            The `env' argument must be non-NULL if and only if the object
         was created for multi-threaded processing, in which case it should
         identify the calling thread. */
    int get_buffer_memory() { return (int) max_buffer_memory; }
      /* Returns the maximum number of buffer memory bytes used by the
         current object for sample data processing.  This includes all memory
         used by the DWT implementation and for intermediate buffering of
         subband samples between the DWT and the block coding system.  The
         maximum is taken over all tiles which this object has been used
         to process. */
    double percent_pulled()
      { /* Returns the percentage of the current tile's lines which have been
           pulled out so far. */
        double remaining=0.0, initial=0.0;
        for (int c=0; c < num_components; c++)
          { remaining += (double)(components[c].remaining_lines);
            initial += (double)(components[c].initial_lines); }
        return (initial==0.0)?100.0:100.0*(1.0-remaining/initial);
      }
  
  public: // Local definitions: some compilers complain if these are private
    struct kde_component_flow_control {
        bool is_signed; // True if original data had a signed representation
        int bit_depth; // Bit-depth of original data.
        bool mapped_by_channel; // True if needed by a channel
        int palette_bits; // Num bits in palette index.  0 if no palette used.
        int width;
        int vert_subsampling;
        int ratio_counter; /* Initialized to 0, decremented by `count_delta';
                              when < 0, a new line must be processed, after
                              which it is incremented by `vert_subsampling'. */
        int initial_lines;
        int remaining_lines;
        kdu_line_buf *line; // Line retrieved from `kdu_multi_synthesis'.
        kdu_line_buf palette_indices; // Created if `palette_bits' > 0;
      };

    struct kde_channel {
        kde_component_flow_control *source_component;
        int width; // Same as `source_component->width'
        kdu_sample16 *lut; // NULL unless a palette mapping is used.
        kdu_line_buf palette_output; // Created only for use with palettes.
        kdu_image_out writer;
      };
      /* Notes:
            When a palette is used, the `lut' array is non-NULL and points
         to an array with 2^{palette_bits} entries, where `palette_bits' is
         found in the `source_component' object referenced from this channel.
         The actual number of palette entries need not be a power of 2, in
         which case some number of final `lut' entries will all hold the same
         value.  When palette mapping is involved, the `source_component's
         `palette_indices' line buffer will be created, having a 16-bit
         representation, into which the palette indices will be written.
         The palette output is always 16-bit fixed point data.  This
         simplifies the palette mapping process somewhat. */

  private: // Data
    kdu_codestream codestream;
    kdu_dims valid_tile_indices;
    kdu_coords tile_idx;
    int x_tnum; // Starts from 0 for the first valid tile.
    kdu_tile tile;
    int num_channels;
    kde_channel *channels;
    int num_components;
    const int *component_indices; // Array passed to the constructor
    bool allow_shorts; // Value supplied to the constructor
    bool skip_ycc; // Value supplied to the constructor
    bool dwt_double_buffering;
    int dwt_stripe_height;
    kde_component_flow_control *components;
    int count_delta; // Holds the minimum of the `vert_subsampling' fields.
    kdu_multi_synthesis engine;
    kdu_sample_allocator aux_allocator; // Used to allocate palette buffers
    kdu_long max_buffer_memory;
    kdu_thread_queue *env_queue;
  private: // Data members used to keep track of parsed packet statistics
    int stats_num_layers;
    int stats_num_resolutions;
    int stats_max_discard_levels;
    kdu_long *stats_resolution_layer_bytes;
    kdu_long *stats_resolution_layer_packets;
    kdu_long *stats_resolution_max_packets;
  };

#endif // EXPAND_LOCAL_H
