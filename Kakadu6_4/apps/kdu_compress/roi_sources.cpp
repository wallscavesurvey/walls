/*****************************************************************************/
// File: roi_sources.cpp [scope = APPS/COMPRESSOR]
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
   Implements one very simple and one quite sophisticated mechanism for
supplying ROI mask information to a compression application.  The examples
here should be sufficient to allow developers to bring ROI mask generation
capabilities into a wide range of applications, including those involving
interactive identification of the region of interest.
******************************************************************************/
#include <assert.h>
#include <string.h>
#include "kdu_messaging.h"
#include "kdu_utils.h"
#include "roi_sources.h"

using namespace std;


/* ========================================================================= */
/*                           eat_white_and_comments                          */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                    eat_white_and_comments                          */
/*****************************************************************************/

static inline void
  eat_white_and_comments(istream &stream)
{
  char ch;
  bool in_comment;

  in_comment = false;
  while (!(stream.get(ch)).fail())
    if (ch == '#')
      in_comment = true;
    else if (ch == '\n')
      in_comment = false;
    else if ((!in_comment) && (ch != ' ') && (ch != '\t') && (ch != '\r'))
      {
        stream.putback(ch);
        return;
      }
}


/* ========================================================================= */
/*                              kd_roi_rect_node                             */
/* ========================================================================= */

/*****************************************************************************/
/*                           kd_roi_rect_node::pull                          */
/*****************************************************************************/

void
  kd_roi_rect_node::pull(kdu_byte buf[], int width)
{
  assert((width == tile_dims.size.x) && (tile_dims.size.y > 0));
  if ((!roi_dims) || (roi_dims.pos.y > tile_dims.pos.y))
    memset(buf,0,(size_t) width); // Mark as background.
  else
    {
      assert(roi_dims.pos.y == tile_dims.pos.y);
      int count = roi_dims.pos.x - tile_dims.pos.x;
      for (width-=count; count > 0; count--)
        *(buf++) = 0;
      count = roi_dims.size.x;
      assert((count > 0) && (width >= count));
      for (width-=count; count > 0; count--)
        *(buf++) = 255;
      for (; width > 0; width--)
        *(buf++) = 0;
      roi_dims.pos.y++;
      roi_dims.size.y--;
    }
  tile_dims.pos.y++;
  tile_dims.size.y--;
}


/* ========================================================================= */
/*                                kdu_roi_rect                               */
/* ========================================================================= */

/*****************************************************************************/
/*                          kdu_roi_rect::kdu_roi_rect                       */
/*****************************************************************************/

kdu_roi_rect::kdu_roi_rect(kdu_codestream codestream, kdu_dims region)
{
  num_components = codestream.get_num_components();
  comp_regions = new kdu_dims[num_components];
  for (int c=0; c < num_components; c++)
    {
      kdu_coords min = region.pos;
      kdu_coords lim = min + region.size;
      kdu_coords subs; codestream.get_subsampling(c,subs);
      min.x = ceil_ratio(min.x,subs.x);
      min.y = ceil_ratio(min.y,subs.y);
      lim.x = ceil_ratio(lim.x,subs.x);
      lim.y = ceil_ratio(lim.y,subs.y);
      comp_regions[c].pos = min;
      comp_regions[c].size = lim-min;
    }
}


/* ========================================================================= */
/*                             kd_roi_graphics_node                          */
/* ========================================================================= */

/*****************************************************************************/
/*                 kd_roi_graphics_node::~kd_roi_graphics_node               */
/*****************************************************************************/

kd_roi_graphics_node::~kd_roi_graphics_node()
{
  assert(rows_left_in_tile == 0); // All clients must release object first.
  if (first_line != NULL)
    {
      last_line->next = free_lines;
      free_lines = first_line;
    }
  while ((first_line=free_lines) != NULL)
    {
      free_lines = first_line->next;
      delete[] ((kdu_byte *) first_line);
    }
}

/*****************************************************************************/
/*                         kd_roi_graphics_node::release                     */
/*****************************************************************************/

void
  kd_roi_graphics_node::release()
{
  if (full_dims.size.x == 0)
    return;
  while ((rows_left_in_tile > 0) && (first_line != NULL))
    {
      assert(outstanding_released_rows == 0);
      int repeat = first_line->repeat_factor;
      if (repeat > rows_left_in_tile)
        repeat = rows_left_in_tile;
      rows_left_in_tile -= repeat;
      first_line->repeat_factor -= repeat;
      if (first_line->repeat_factor == 0)
        recycle_first_line();
    }
  outstanding_released_rows += rows_left_in_tile;
  rows_left_in_tile = 0;
}

/*****************************************************************************/
/*                          kd_roi_graphics_node::pull                       */
/*****************************************************************************/

void
  kd_roi_graphics_node::pull(kdu_byte buf[], int width)
{
  assert(width == full_dims.size.x);
  if (width == 0)
    return;
  assert(rows_left_in_tile > 0);
  while ((outstanding_released_rows > 0) || (first_line == NULL))
    source->advance();
  assert(first_line->repeat_factor > 0);
  memcpy(buf,first_line->buf,(size_t) width);
  first_line->repeat_factor--;
  if (first_line->repeat_factor == 0)
    recycle_first_line();
  rows_left_in_tile--;
}


/* ========================================================================= */
/*                              kdu_roi_graphics                             */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdu_roi_graphics::kdu_roi_graphics                  */
/*****************************************************************************/

kdu_roi_graphics::kdu_roi_graphics(kdu_codestream codestream,
                                   char *fname, float threshold)
{
  char ch, magic[3];
  int max_val;

  in.open(fname,ios::in|ios::binary);
  if (in.fail())
    { kdu_error e;
      e << "Unable to open input image file, \"" << fname << "\"."; }
  in.get(magic,3);
  if (strcmp(magic,"P5") != 0)
    { kdu_error e; e << "Only the PGM file format is currently accepted for "
      "ROI mask images."; }
  eat_white_and_comments(in); in >> in_width;
  eat_white_and_comments(in); in >> in_height;
  eat_white_and_comments(in); in >> max_val;
  if (!in)
    { kdu_error e; e << "Image file \"" << fname << "\" does not appear to "
      "have a valid PGM header."; }
  while (!(in.get(ch)).fail())
    if ((ch == '\n') || (ch == ' '))
      break;
  this->threshold = (kdu_byte)(threshold*255);

  num_components = codestream.get_num_components();
  components = new roi_graphics_component[num_components];
  kdu_dims tile_indices; codestream.get_valid_tiles(tile_indices);
  for (int c=0; c < num_components; c++)
    {
      roi_graphics_component *comp = components + c;
      kdu_dims comp_dims; codestream.get_dims(c,comp_dims);
      comp->width = comp_dims.size.x;
      comp->height = comp_dims.size.y;
      comp->tiles_wide = tile_indices.size.x;
      comp->hor_tiles = new kd_roi_graphics_node[comp->tiles_wide];
      comp->vert_decr = in_height;
      comp->vert_incr = comp->height;
      comp->vert_state = comp->vert_incr >> 1;
      comp->hor_decr = in_width;
      comp->hor_incr = comp->width;
      comp->hor_state = comp->hor_incr >> 1;
      int max_hor_repeat = ceil_ratio(comp->hor_incr,comp->hor_decr);
      comp->line_buf = new kdu_byte[comp->width+max_hor_repeat];
      for (int n=0; n < comp->tiles_wide; n++)
        {
          kd_roi_graphics_node *node = comp->hor_tiles + n;
          node->source = this;
          kdu_coords tile_idx = tile_indices.pos; tile_idx.x += n;
          codestream.get_tile_dims(tile_idx,c,node->full_dims);
          node->full_dims.size.y = comp->height;
        }
    }
  line_buf = new kdu_byte[in_width];
}

/*****************************************************************************/
/*                     kdu_roi_graphics::~kdu_roi_graphics                   */
/*****************************************************************************/

kdu_roi_graphics::~kdu_roi_graphics()
{
  if (in.is_open())
    in.close();
  if (line_buf != NULL)
    delete[] line_buf;
  if (components != NULL)
    {
      for (int c=0; c < num_components; c++)
        {
          roi_graphics_component *comp = components + c;
          if (comp->hor_tiles != NULL)
            delete[] comp->hor_tiles;
          if (comp->line_buf != NULL)
            delete[] comp->line_buf;
        }
      delete[] components;
    }
}

/*****************************************************************************/
/*                       kdu_roi_graphics::acquire_node                      */
/*****************************************************************************/

kdu_roi_node *
  kdu_roi_graphics::acquire_node(int comp_idx, kdu_dims tile_region)
{
  int n;

  assert((comp_idx >= 0) && (comp_idx < num_components));
  roi_graphics_component *comp = components + comp_idx;
  kd_roi_graphics_node *node = comp->hor_tiles;
  for (n=0; n < comp->tiles_wide; n++, node++)
    if ((node->full_dims.pos.x == tile_region.pos.x) &&
        (node->full_dims.size.x == tile_region.size.x))
      break;
  if (n == comp->tiles_wide)
    { kdu_error e; e << "Attempting to access non-existent tile-component "
      "through `kdu_roi_graphics::acquire_node'!"; }
  if ((node->rows_left_in_tile > 0) ||
      (node->full_dims.pos.y != tile_region.pos.y))
    { kdu_error e; e << "Tile-components with the same horizontal tile index "
      "for which ROI nodes are acquired with `kdu_roi_graphics::acquire_node' "
      "must be accessed in order from top to bottom, releasing the previous "
      "(vertically adjacent) tile before accessing the next one in the same "
      "component."; }
  assert(node->full_dims.size.y >= tile_region.size.y);
  node->rows_left_in_tile = tile_region.size.y;
  node->full_dims.pos.y += tile_region.size.y;
  node->full_dims.size.y -= tile_region.size.y;
  if (node->full_dims.size.x == 0)
    node->rows_left_in_tile = 0;
  return node;
}

/*****************************************************************************/
/*                         kdu_roi_graphics::advance                         */
/*****************************************************************************/

void
  kdu_roi_graphics::advance()
{
  int n;
  kdu_byte *sp, *dp;

  // Read a new line from the graphics file.
  assert(in_height > 0);
  if ((int)(in.read((char *) line_buf,in_width)).gcount() != in_width)
    { kdu_error e; e << "Image file used to acquire ROI mask regions has "
      "terminated prematurely!"; }
  in_height--;

  // Threshold the new line.
  for (sp=line_buf, n=in_width; n > 0; n--, sp++)
    *sp = (*sp > threshold)?255:0;

  // Now transfer the contents to each image component.
  for (int c=0; c < num_components; c++)
    {
      roi_graphics_component *comp = components + c;

      // First determine the vertical repeat factor (if 0, we can just move on)
      int repeat_factor = 0;
      while (comp->vert_state >= 0)
        {
          comp->vert_state -= comp->vert_decr;
          repeat_factor++;
        }
      comp->vert_state += comp->vert_incr;
      if (repeat_factor > comp->height)
        repeat_factor = comp->height;
      if (in_height == 0)
        repeat_factor = comp->height; // There will be no more rows.
      if (repeat_factor == 0)
        continue;
      comp->height -= repeat_factor;

      // Now horizontally resample the input line
      int incr=comp->hor_incr;
      int decr=comp->hor_decr;
      int state=comp->hor_state;
      int n_out = comp->width;
      kdu_byte val=0;
      for (sp=line_buf, dp=comp->line_buf, n=in_width; n > 0; n--, state+=incr)
        for (val=*(sp++); state >= 0; state-=decr, n_out--)
          *(dp++) = val;
      for (; n_out > 0; n_out--)
        *(dp++) = val;

      // Finally, transfer the contents into each horizontal tile object.
      sp = comp->line_buf;
      for (n=0; n < comp->tiles_wide; n++)
        {
          kd_roi_graphics_node *node = comp->hor_tiles + n;
          n_out = node->full_dims.size.x;
          dp = node->add_line(repeat_factor);
          if (dp == NULL)
            sp += n_out; // This tile row has already been released.
          else
            for (; n_out > 0; n_out--)
              *(dp++) = *(sp++);
        }
    }
}
