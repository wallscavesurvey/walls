/*****************************************************************************/
// File: roi_sources.h [scope = APPS/COMPRESSOR]
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
/******************************************************************************
Description:
   Defines two different types of source objects (both derived from
"kdu_roi_image") for supplying ROI mask information.  The first is perhaps
the simplest derivation of "kdu_roi_image" capable of doing anything useful:
it works with a single rectangular region.  The second is a much more
sophisticated region delivery engine, which uses an auxiliary image to
determine the foreground regions and automatically provides all of the
resampling required to map the geometry of the auxiliary image into that
of each image component.  This second object should be studied closely
by developers interested in delivering ROI encoded imagery from commercial
applications, since it should be easily adapted to work with other region
specification methods (e.g., interactive region specification by a
clinical specialist).
******************************************************************************/
#ifndef ROI_SOURCES_H
#define ROI_SOURCES_H

#include <fstream>

#include "kdu_compressed.h"
#include "kdu_roi_processing.h"
#include "kdu_image.h"

// Defined here:
class kd_roi_rect_node; // Derived node class served by `kdu_roi_rect'
class kdu_roi_rect; // Simple support for rectangular ROI region specification
class kd_roi_graphics_node; // Derived node class served by `kdu_roi_graphics'
class kdu_roi_graphics; // Support for ROI regions derived from graphic images


/*****************************************************************************/
/*                              kd_roi_rect_node                             */
/*****************************************************************************/

class kd_roi_rect_node : public kdu_roi_node {
  public: // Member functions
    kd_roi_rect_node(kdu_dims tile_region, kdu_dims roi_region)
      { tile_dims = tile_region;
        roi_dims = roi_region & tile_region; }
    void release()
      /* In this simplest incarnation of an ROI source node, the granting
         agent (`kdu_roi_rect') does not keep track of the nodes it grants
         via its `acquire_node' interface; therefore, the node's own `release'
         function must destroy the resource.  More sophisticated ROI sources
         will not usually do this. */
      { delete this; }
    void pull(kdu_byte buf[], int width);
  private: // Data
    kdu_dims tile_dims; // Vert coord advances as lines are pulled
    kdu_dims roi_dims; // Vert coord advances when intersecting line pulled
  };

/*****************************************************************************/
/*                                kdu_roi_rect                               */
/*****************************************************************************/

class kdu_roi_rect : public kdu_roi_image {
  /* [SYNOPSIS]
     Serves ROI mask information through the standardized interfaces offered
     by the base class, `kdu_roi_image', for the simple case in which there
     is only one foreground region, having a rectangular geometry.  Although
     somewhat useful in its own right, the main purpose of this object is
     to provide the simplest possible example of implementing the services
     advertised by the abstract base class, `kdu_roi_image'.
     [//]
     For a more sophisticated example, refer to `kdu_roi_graphics'.
  */
  public: // Member functions
    kdu_roi_rect(kdu_codestream codestream, kdu_dims region);
      /* [SYNOPSIS]
           Accepts a single rectangular region, specified relative to the
           high resolution code-stream canvas coordinate system.  This will
           be the foreground region associated with all ROI mask generation
           activities.  It is transformed into a region on each of the
           individual image components by applying the usual coordinate
           transformation rules.
         [ARG: codestream]
           Master interface to the internal code-stream management
           machinery associated with the image being compressed.
         [ARG: region]
           The supplied region should incorporate the effects of any
           prevailing geometric transformations, which may have been set
           up by calls to `codestream.change_appearance'.  As a result,
           if `region' were set to that returned by `codestream.get_dims'
           (called with a negative `comp_idx' argument), the foreground
           would correspond exactly to the full image region on the canvas.
      */
    ~kdu_roi_rect()
      { if (comp_regions != NULL) delete[] comp_regions; }
    kdu_roi_node *acquire_node(int comp_idx, kdu_dims tile_region)
      { /* [SYNOPSIS] See the description of `kdu_roi_image::acquire_node'. */
        assert((comp_idx >= 0) && (comp_idx < num_components));
        return new kd_roi_rect_node(tile_region,comp_regions[comp_idx]);
      }
  private: // Data
    int num_components;
    kdu_dims *comp_regions;
  };

/*****************************************************************************/
/*                            kd_roi_graphics_node                           */
/*****************************************************************************/

class kd_roi_graphics_node : public kdu_roi_node {
  public: // Member functions
    kd_roi_graphics_node()
      { rows_left_in_tile = outstanding_released_rows = 0;
        first_line = last_line = free_lines = NULL; }
    virtual ~kd_roi_graphics_node(); // No need for virtual -- keeps gcc happy
    void release();
    void pull(kdu_byte buf[], int width);
  private: // Data
    friend class kdu_roi_graphics;

      struct roi_node_line {
          int repeat_factor; // Number of times this line is to be re-used.
          roi_node_line *next;
          kdu_byte buf[1];
        };

    kdu_roi_graphics *source;
    kdu_dims full_dims; // Location of vertical full tile-stripe.
    int rows_left_in_tile; // Number of rows left in current tile.
    int outstanding_released_rows; // Rows to be discarded when available
    roi_node_line *first_line, *last_line, *free_lines;
  private: // Convenience function
    kdu_byte *add_line(int repeat_factor)
      { // Returns NULL if there is no need to write anything.
        if (full_dims.size.x == 0)
          return NULL;
        if (outstanding_released_rows > 0)
          { outstanding_released_rows--; return NULL; }
        if (free_lines == NULL)
          {
            free_lines = (roi_node_line *)
              new kdu_byte[sizeof(roi_node_line)+full_dims.size.x];
            free_lines->next = NULL;
          }
        roi_node_line *tmp = free_lines; free_lines = tmp->next;
        tmp->next = NULL;
        if (last_line == NULL)
          first_line = last_line = tmp;
        else
          last_line = last_line->next = tmp;
        tmp->repeat_factor = repeat_factor;
        return tmp->buf;
      }
    void recycle_first_line()
      {
        roi_node_line *tmp = first_line->next; first_line->next = free_lines;
        free_lines = first_line; first_line = tmp;
        if (first_line == NULL) last_line = NULL;
      }
  };
  /* Notes:
        This object actually managed ROI information for a full vertical
     stripe of tiles in any given component.  The `full_dims' member indicates
     the location and dimensions of the set of all tiles in the component
     which have the same horizontal tile index, while `rows_left_in_tile'
     indicates the number of rows left in the current tile.  Only one of the
     vertically adjacent tiles may be accessed at a time through the base
     `kdu_roi_node' interface.  The `release' function must be called before
     a subsequent tile in the stripe can be acquired through
     `kdu_roi_graphics_image::acquire'.
        The `first_line' and `last_line' members point to the first and last
     element in a linked list of repeatable line buffers.  Each line buffer
     has a `repeat_factor' indicating the number of `pull' calls which should
     return the same line.  Once the `repeat_factor' reaches 0, the line
     is recycled via a `free_lines' heap. */

/*****************************************************************************/
/*                              kdu_roi_graphics                             */
/*****************************************************************************/

class kdu_roi_graphics : public kdu_roi_image {
  /* [SYNOPSIS]
     Serves ROI mask information through the standardized interfaces offered
     by the base class, `kdu_roi_image'.  In this case, the foreground region
     is determined by thresholding an auxiliary monochrome image, stretching
     or shrinking it to match the geometry of the actual image being
     compressed.
  */
  public: // Member functions
    kdu_roi_graphics(kdu_codestream codestream, char *fname, float threshold);
      /* [SYNOPSIS]
           Accepts the name of a monochrome image file and a single
           threshold.  The image represented by this file is stretched
           (interpolated) or shrunk (decimated) to match the geometry
           of the actual image being compressed (as represented by the
           `codestream') and its sample values are thresholded.  Locations
           where the threshold is exceeded are deemed to belong to the
           foreground, while the remainder is interpreted as background.
           [//]
           Note that the mask image (the one being thresholded) is read
           incrementally, buffering the smallest amount of data required
           to satisfy the demand for ROI information, which filters
           through from the leaves of the compression tree (`kdu_encoder'
           objects) all the way up through the DWT analysis system.
         [ARG: codestream]
           Master interface to the internal code-stream management
           machinery associated with the image being compressed.
         [ARG: fname]
           Name of the mask image, to be opened and used to determine the
           foreground region.  Currently, only binary PGM files are
           accepted, although it would be trivial to add support for other
           file formats.  Note that the dimensions of the mask image are
           arbitrary.  The width and the height are independently stretched
           to the dimensions of the image which is actually being compressed.
         [ARG: threshold]
           Foreground threshold, expressed as a normalized quantity in the
           range 0 to 1, where 0 means black and 1 means white.  Foreground
           locations must have sample values which strictly exceed the
           threshold, meaning that a threshold of 0 will produce foreground
           everywhere the mask image's samples are non-zero, while a
           threshold of 1 will produce an empty foreground.
      */
    ~kdu_roi_graphics();
    kdu_roi_node *acquire_node(int comp_idx, kdu_dims tile_region);
      /* [SYNOPSIS] See the description of `kdu_roi_image::acquire_node'. */
  private: // Functions
    friend class kd_roi_graphics_node;
    void advance(); // Called when another row is required.
  private: // Data

      struct roi_graphics_component {
          int width;
          int height; // Rows remaining in this component.
          int tiles_wide;
          kd_roi_graphics_node *hor_tiles;
          int hor_decr, hor_incr, hor_state;
          int vert_decr, vert_incr, vert_state;
          kdu_byte *line_buf; // A bit bigger at the end to allow sloppy coding
        };
        /* Notes: Each time a new image row is read in, `vert_state' is
           decremented by `vert_decr' until it becomes negative, with
           each such decrement corresponding to a repeated copy of the row
           for the image component (there could be none if the input image
           has a higher sampling density than the image component, although
           this is less likely).  After that, `vert_state' is incremented by
           `vert_incr'.  The same approach is used to perform nearest
           neighbour interpolation in the horizontal direction, with
           `hor_state' representing the initial state value. */

    int num_components;
    roi_graphics_component *components;
    std::ifstream in;
    int in_width;
    int in_height; // Rows remaining in image.
    kdu_byte threshold;
    kdu_byte *line_buf;
  };

#endif // ROI_SOURCES_H
