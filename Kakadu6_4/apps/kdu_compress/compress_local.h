/*****************************************************************************/
// File: compress_local.h [scope = APPS/COMPRESSOR]
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
   Local definitions used by "kdu_compressor.cpp".  The "kdc_flow_control"
object may be of interest to a variety of different applications.
******************************************************************************/

#ifndef COMPRESS_LOCAL_H
#define COMPRESS_LOCAL_H

// Core includes
#include "kdu_elementary.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "kdu_roi_processing.h"
// Application includes
#include "kdu_image.h"

// Defined here:
class kdc_file_binding; // Temp storage for input files supplied on cmd line
class kdc_flow_control; // Implements data flow control across components
class kdc_flush_worker; // Implements asynchronous incremental flushing for
                        // multi-threaded compression environments.

/*****************************************************************************/
/*                             kdc_file_binding                              */
/*****************************************************************************/

class kdc_file_binding {
  public: // Member functions
    kdc_file_binding(char *string, int len, kdu_long offset)
      /* `len' is the length of the name string, while `offset' is the
         number of leading bytes in the file to skip over. */
      {
        fname = new char[len+1]; fname[len] = '\0';
        strncpy(fname,string,(size_t) len);
        num_components = first_comp_idx = 0;
        this->offset = offset;  next = NULL;
      }
    ~kdc_file_binding() // Destroys the entire list for convenience.
      { delete[] fname;
        if (reader.exists()) reader.destroy();
        if (next != NULL) delete next; }
  public: // Data -- all data public in this local object.
    char *fname;
    int num_components, first_comp_idx;
    kdu_dims cropping;
    kdu_image_in reader;
    kdu_long offset;
    kdc_file_binding *next;
  };

/*****************************************************************************/
/*                             kdc_flow_control                              */
/*****************************************************************************/

class kdc_flow_control {
  /* Each tile-component may be compressed independently and in any
     order; however, the sequence in which tile-component lines are
     decompressed clearly has an impact on the amount of buffering required
     for image file I/O and possibly the amount of code-stream buffering. This
     object is used to stage the tile-component processing steps so that
     the image is compressed from top to bottom in a manner consistent
     with the overall geometry and hence, in most cases, minimum system
     buffering requirements. */
  public: // Member functions
    kdc_flow_control(kdc_file_binding *files, kdu_codestream codestream,
                     int x_tnum, bool allow_shorts,
                     kdu_roi_image *roi_source=NULL,
                     int dwt_stripe_height=1,
                     bool dwt_double_buffering=false,
                     kdu_thread_env *env=NULL,
                     kdu_thread_queue *env_queue=NULL);
      /* Constructs a flow control object for one tile.  The caller might
         maintain an array of pointers to such objects, one for each
         horizontally adjacent tile.  The `x_tnum' argument identifies which
         horizontal tile the object is being created to represent, with 0 for
         the first apparent horizontal tile.  The `files' list provides
         references to the `image_in' objects from which image lines will be
         retrieved for compression.  The `files' pointer may be NULL if
         desired,  in which case image samples must be supplied via the
         `access_compressor_line' member function.  If `allow_shorts' is
         false, 32-bit data representations will be selected for all
         processing; otherwise, the constructor will select a 16- or a 32-bit
         representation on the basis of the image component bit-depths.
         The `roi_source' argument, if non-NULL, provides access to a
         `kdu_roi_image' interface whose `acquire_node' member function
         is to be used to propagate ROI mask information down the
         tile-component processing hierarchy.
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
    ~kdc_flow_control();
      /* Note: if you created the object for multi-threaded processing, you
         must not destroy it until after you have at least called
         `kdu_thread_entity::synchronize' -- preferable,
         `kdu_thread_entity::terminate' or `kdu_thread_entity::destroy'. */
    bool advance_components(kdu_thread_env *env=NULL);
      /* Causes the line position for this tile to be advanced in every
         image component, by an amount which should cause at least one
         image line to become active.  Each such active image line is read
         from the relevant image file, unless there are no image files, in
         which case the caller should use the `access_compressor_line' function
         to fill in the contents of the active image lines.  The function
         returns false once all image lines in the current tile have been
         compressed, at which point the user will normally issue an
         `advance_tile' call.
            The `env' argument must be non-NULL if and only if the object
         was created for multi-threaded processing, in which case it should
         identify the calling thread. */
    kdu_line_buf *access_compressor_line(int comp_idx);
      /* This function may be called after `advance_components' to access any
         newly read image lines prior to compression.  This allows the caller
         to modify the image sample values, which may be essential if there
         are no file reading objects (i.e., a NULL `files' argument was given
         to the constructor).  The function returns NULL if the indicated
         component does not currently have an active line.  The number of
         components with active lines, following a call to
         `advance_components', may be as little as 1. */
    void process_components(kdu_thread_env *env=NULL);
      /* This function must be called after every call to `advance_components'
         which returns true.  It performs any required colour conversions and
         sends the active image lines to the relevant tile-component
         compression engines.  Upon return, the object is prepared to receive
         another call to its `advance_components' member function.
            The `env' argument must be non-NULL if and only if the object
         was created for multi-threaded processing, in which case it should
         identify the calling thread. */
    bool advance_tile(kdu_thread_env *env=NULL);
      /* Moves on to the next vertical tile at the same horizontal tile
         position, returning false if all tiles have been processed.  Note
         that the function automatically invokes the existing tile's
         `kdu_tile::close' member function.
            If the object was constructed for multi-threaded processing, you
         must supply a non-NULL `env' argument here to identify the calling
         thread.  This allows the function to first synchronize all
         processing associated with the current tile, before advancing to
         a new one. */
    int get_max_remaining_lines()
      { /* Finds the maximum number of lines from any given image component
           which have still to be written before this tile is complete. */
        int result = 0;
        for (int c=0; c < num_components; c++)
          if (components[c].remaining_lines > result)
            result = components[c].remaining_lines;
        return result;
      }
    int get_buffer_memory() { return (int) max_buffer_memory; }
      /* Returns the maximum number of buffer memory bytes used by the
         current object for sample data processing.  This includes all memory
         used by the DWT implementation and for intermediate buffering of
         subband samples between the DWT and the block coding system.  The
         maximum is taken over all tiles which this object has been used
         to process. */
    double percent_pushed()
      { /* Returns the percentage of the current tile's lines which have been
           pushed in so far. */
        double remaining=0.0, initial=0.0;
        for (int c=0; c < num_components; c++)
          { remaining += (double)(components[c].remaining_lines);
            initial += (double)(components[c].initial_lines); }
        return (initial==0.0)?100.0:100.0*(1.0-remaining/initial);
      }
  private: // Data

    struct kdc_component_flow_control {
      public: // Data
        kdu_image_in reader;
        int vert_subsampling;
        int ratio_counter; /* Initialized to 0, decremented by `count_delta';
                              when < 0, a new line must be processed, after
                              which it is incremented by `vert_subsampling'. */
        int initial_lines;
        int remaining_lines;
        kdu_line_buf *line;
      };

    kdu_codestream codestream;
    kdu_dims valid_tile_indices;
    kdu_coords tile_idx;
    int x_tnum; // Starts at 0 for the first tile.
    kdu_tile tile;
    int num_components;
    kdc_component_flow_control *components;
    bool allow_shorts;
    bool dwt_double_buffering;
    int dwt_stripe_height;
    int count_delta; // Holds the minimum of the `vert_subsampling' fields.
    kdu_roi_image *roi_image;
    kdu_multi_analysis engine;
    kdu_long max_buffer_memory;
    kdu_thread_queue *env_queue;
  };

/*****************************************************************************/
/*                             kdc_flush_worker                              */
/*****************************************************************************/

class kdc_flush_worker : public kdu_worker {
  public: // Member functions
    kdc_flush_worker() { codestream=kdu_codestream();  ready_failed=false; }
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

#endif // COMPRESS_LOCAL_H
