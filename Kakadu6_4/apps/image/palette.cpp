/*****************************************************************************/
// File: palette.cpp [scope = APPS/IMAGE-IO]
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
   Implements the `kdu_rgb8_palette::rearrange' function, which attempts
to find an optimal permutation of the colour palette entries so that
the palette indices can be efficiently compressed and lossy compression of
these palette indices should produce a reasonable image.  Of course, the
algorithm is sub-optimal (NP complete problem) and it can also be time
consuming.  The focus on the present implementation is making sure that
if the palette is already well ordered, it will not be disturbed by the
optimization algorithm.
   You must include this file only if "image_in.cpp" is used.
******************************************************************************/

#include <assert.h>
#include "kdu_messaging.h"
#include "kdu_image.h"
#include <iostream>

/* ========================================================================= */
/*                             Local Definitions                             */
/* ========================================================================= */

struct palette_entry {
  public: // Handy functions
    void make(int original_idx, kdu_rgb8_palette *palette)
      {
        this->original_idx = original_idx;
        red = palette->red[original_idx];
        green = palette->green[original_idx];
        blue = palette->blue[original_idx];
      }
    int distance(palette_entry *rhs)
      {
        int result, diff;
        diff = red - rhs->red; result = (diff < 0)?(-diff):diff;
        diff = green - rhs->green; result += (diff < 0)?(-diff):diff;
        diff = blue - rhs->blue; result += (diff < 0)?(-diff):diff;
        return result;
      }
    int neighbour_distance()
      {
        int dist = 0;
        if (prev != NULL)
          dist += distance(prev);
        if (next != NULL)
          dist += distance(next);
        return dist;
      }
    void insert(palette_entry *before)
      {
        if (before == NULL)
          { prev = next = NULL; return; }
        prev = before;
        next = before->next;
        if (prev != NULL)
          prev->next = this;
        if (next != NULL)
          next->prev = this;
      }
    palette_entry *extract(palette_entry *head)
      /* Removes the entry from the list, returning a pointer to the new
         head of the list -- it may change. */
      {
        if (next != NULL)
          next->prev = this->prev;
        if (prev == NULL)
          {
            assert(this == head);
            head = next;
          }
        else
          prev->next = this->next;
        prev = next = NULL;
        return head;
      }
  public: // Data
    palette_entry *next, *prev;
    int original_idx;
    kdu_int32 red, green, blue;
  };


/* ========================================================================= */
/*                              kdu_rgb8_palette                             */
/* ========================================================================= */

/*****************************************************************************/
/*                         kdu_rgb8_palette::rearrange                       */
/*****************************************************************************/

void
  kdu_rgb8_palette::rearrange(kdu_byte *map)
{
  int n, total_entries=(1<<input_bits);
  palette_entry entries[256];

  { kdu_warning w;
    w << "Optimizing palette ...\n";
    w << "\tUse `-no_palette' to avoid nasty palettization effects when "
         "reconstruction is anything but lossless.  `-no_palette' also saves "
         "the time consuming palette reordering step.";
  }

  // First put all entries into a single list, in the original order.
  palette_entry *head = entries;
  palette_entry *scan = NULL;
  for (n=0; n < total_entries; n++)
    {
      entries[n].make(n,this);
      entries[n].insert(scan);
      scan = entries + n;
    }

  // Now increment a simple reshuffling algorithm
  do {
      // Implement one reshuffle if possible
      for (scan=head; scan != NULL; scan=scan->next)
        {
          int scan_dist = (scan->next == NULL)?0:scan->distance(scan->next);
          palette_entry *test, *next;
          for (test=head; test != NULL; test=next)
            { // See if we can improve matters by inserting `test' after `scan'
              next = test->next;
              if ((test == scan) || (test == scan->next))
                continue;
              int test_dist = test->neighbour_distance();
              int extra_dist = scan->distance(test);
              if (scan->next != NULL)
                extra_dist += test->distance(scan->next);
              if ((test->next != NULL) && (test->prev != NULL))
                extra_dist += test->prev->distance(test->next);
              extra_dist -= test_dist;
              extra_dist -= scan_dist;
              if (extra_dist < 0)
                { // The operation is favourable.
                  head = test->extract(head);
                  test->insert(scan);
                  break;
                }
            }
          if (test != NULL)
            break;
        }
    } while (scan != NULL);

  // Finally, generate the new palette and permutation map
  for (n=0, scan=head; scan != NULL; scan=scan->next, n++)
    {
      map[scan->original_idx] = n;
      red[n] = scan->red;
      green[n] = scan->green;
      blue[n] = scan->blue;
    }
  std::cout << "    Done optimizing palette\n";
}
